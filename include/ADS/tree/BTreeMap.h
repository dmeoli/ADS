/*--------------------------------------------------------------------------*/
/*---------------------------- File BTreeMap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered map on a B+-tree in main memory, beyond the book: the
 * engineered ordered DICTIONARY of ADS [see ADT.h], the one that the
 * benchmark of the ordered dictionaries compares with std::map,
 * absl::btree_map and tlx::btree_map, while BPlusTree [see BPlusTree.h]
 * stays the tree of section 6.3 and BTree [see BTree.h] that of Bayer and
 * McCreight.
 *
 * The pairs are in the leaves, which hold up to L of them, the keys in an
 * array and the values in another one, so that a search reads the keys
 * alone, and which are linked in a list in both directions; an internal
 * node holds up to I keys and I + 1 children, the child c[ i ] having the
 * keys in ( k[ i - 1 ] , k[ i ] ], i.e., k[ i ] is not smaller than the
 * keys below c[ i ] and smaller than those below c[ i + 1 ]. L and I come
 * from the bytes B of a node (512 by default, 8 lines of cache), L = B /
 * ( sizeof( K ) + sizeof( V ) ) and I = B / ( sizeof( K ) + sizeof( void
 * * ) ), each between 4 and 255. The nodes keep no parent and no height:
 * the height is that of the tree, and an operation that climbs back keeps
 * the path of its descent.
 *
 * The place of a key in a node is the number of the keys smaller than it.
 * On the arithmetic keys under std::less, the unused places of a node hold
 * the largest value of the type (its infinity for the floating types), so
 * that the count runs on all the places at once, with no branch and no
 * dependence on the count of the node, as a few comparisons of vectors
 * [see CountLess.h] (four of AVX-512 for 32 keys of 64 bits), which beats
 * a binary search on a few lines of cache, whose branches are mispredicted
 * half of the times. On the other keys the place is a binary search. On
 * the way down, the lines of the keys of the next node are all prefetched
 * at once, so that their misses overlap instead of following one another,
 * as in the prefetching B+-trees of Chen, Gibbons and Mowry; a search of a
 * value prefetches the values of the leaf too, whose line is read right
 * after the keys. The nodes come from two pools [see ObjectPool.h] on
 * huge pages, so that a descent misses the TLB once per 2 MB rather than
 * once per node, and the internal nodes of a level are linked in a list,
 * as the leaves are.

 * An insertion into a full node splits it in two halves, the largest key
 * of the left one going up into the parent, which may split in turn, up to
 * the root. An insertion past the largest key of the tree, or before the
 * smallest one, splits the node so that the old one stays full and the new
 * one gets the new key alone (the trick of the B-trees of Abseil and of
 * the bulk loaders): the keys inserted in increasing order fill all the
 * nodes, instead of half of them. A removal that leaves a node with fewer
 * than half of its places full either merges it with a sibling, if the
 * two fit in one node, or takes from the sibling half of the difference,
 * so that the two end up balanced; a merge may leave the parent short in
 * turn, and a root with a single child gives way to it. Hence every node
 * but those of the leftmost and of the rightmost paths, and those the
 * removals have not touched since a split, is at least half full, and the
 * height is \f$ O( \log n ) \f$; all the operations take \f$ O( ( L + I )
 * \log n ) \f$ time, a few lines of cache per level. build() loads a
 * sorted sequence bottom up in O( n ), all the nodes full.
 *
 * The iterators are a leaf, a place in it and the parent of the leaf,
 * bidirectional, on constant pairs of references ( key , value ); find()
 * gives a value to change in place. An iterator steps to the next leaf
 * through the children of the parent (and through the list of the
 * parents), so that it prefetches the leaf 4 places ahead with no chase of
 * the links between the leaves, whose misses would otherwise follow one
 * another in a scan of a tree larger than the cache. An insertion or a
 * removal invalidates all the iterators. Keys and values must be default
 * constructible, the places of a node being built with the node, and a
 * tree that shrinks keeps the memory of its pools until it is cleared.

 * The contracts [see Contract.h]: after insert() the key is there and
 * after remove() it is not; build() needs strictly increasing keys, an
 * audit; the invariant is the order of the keys in the tree and their
 * bounds, the depth of the leaves, the list of the leaves, the count and
 * the padding of the unused places, an O( n ) check at the level 3 on the
 * trees of up to 4096 pairs. The operations are defined in BTreeMap.tpp.
 *
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see D. Comer, The ubiquitous B-tree, ACM Computing Surveys 11(2),
 *      121-137, 1979, doi:10.1145/356770.356776
 * \see S. Chen, P. B. Gibbons, T. C. Mowry, Improving index performance
 *      through prefetching, Proceedings of SIGMOD 2001, 235-246, 2001,
 *      doi:10.1145/375663.375688
 * \see G. Graefe, Modern B-tree techniques, Foundations and Trends in
 *      Databases 3(4), 203-402, 2011, doi:10.1561/1900000028
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_BTreeMap
 #define __ADS_BTreeMap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/search/CountLess.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BTreeMap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered map on a B+-tree of nodes of B bytes, by the order less()
template< class K , class V , class Less = std::less< K > ,
          std::size_t B = 512 >
 requires std::default_initializable< K > &&
          std::default_initializable< V >
class BTreeMap {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the pairs of a leaf
 static constexpr std::size_t L =
  std::clamp< std::size_t >( B / ( sizeof( K ) + sizeof( V ) ) , 4 , 255 );

 /// the keys of an internal node
 static constexpr std::size_t I =
  std::clamp< std::size_t >( B / ( sizeof( K ) + sizeof( void * ) ) , 4 ,
                             255 );

 explicit BTreeMap( Less less = Less() );

 BTreeMap( const BTreeMap & o );

 BTreeMap( BTreeMap && o ) noexcept;

 BTreeMap & operator=( BTreeMap o ) noexcept;

 ~BTreeMap();

/*--------------------------------------------------------------------------*/

 /// the value of k, if there
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// the value of k, to change in place, or nullptr
 [[nodiscard]] const V * find( const K & k ) const;

 [[nodiscard]] V * find( const K & k );

 /// gives k the value v; true if k was not there
 bool insert( const K & k , const V & v );

 /// removes k; true if it was there
 bool remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the levels of the tree, 0 if empty, 1 if a single leaf
 [[nodiscard]] int height( void ) const;

 void clear( void );

 /// replaces the pairs with those of [ first , last ), pairs ( key ,
 /// value ) by strictly increasing keys, all the nodes full
 /// \pre the keys are strictly increasing (an audit)
 template< std::input_iterator It >
 void build( It first , It last );

 /// f( key , value ) on each pair, by increasing keys
 template< class F >
 void forEach( F && f ) const;

 /// f( key , value ) on each pair of key in [ lo , hi ), by increasing
 /// keys
 template< class F >
 void forRange( const K & lo , const K & hi , F && f ) const;

 void swap( BTreeMap & o ) noexcept;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing keys,
 // bidirectional, a leaf and a place in it

 class const_iterator;

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const;
 [[nodiscard]] const_iterator end( void ) const;
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the first pair whose key is not smaller than k
 [[nodiscard]] const_iterator lowerBound( const K & k ) const;

 /// the first pair whose key is larger than k
 [[nodiscard]] const_iterator upperBound( const K & k ) const;

 /// removes the pair of it, and gives the one after it
 const_iterator erase( const_iterator it );

/*--------------------------------------------------------------------------*/
 private:

 /// the keys are counted on all the places of a node, padded
 static constexpr bool FAST = std::is_arithmetic_v< K > &&
  ( std::is_same_v< Less , std::less< K > > ||
    std::is_same_v< Less , std::less<> > );

 /// the key of the unused places, larger than every key
 [[nodiscard]] static constexpr K pad( void ) {
  if constexpr( std::numeric_limits< K >::has_infinity )
   return( std::numeric_limits< K >::infinity() );
  else
   return( std::numeric_limits< K >::max() );
  }

 struct alignas( 64 ) Leaf {
  K k[ L ];                        ///< the keys
  V v[ L ];                        ///< the values
  Leaf * prev = nullptr;           ///< the leaf before
  Leaf * next = nullptr;           ///< the leaf after
  std::uint32_t n = 0;             ///< the pairs
  };

 struct alignas( 64 ) Inner {
  K k[ I ];                        ///< the bounds of the children
  void * c[ I + 1 ];               ///< the children
  Inner * prev = nullptr;          ///< the node before, on its level
  Inner * next = nullptr;          ///< the node after, on its level
  std::uint32_t n = 0;             ///< the keys, the children - 1
  };

 /// the place of an internal node on a path, and the child taken
 struct Step {
  Inner * u;
  std::uint32_t i;
  };

 static constexpr int DEPTH = 64;  ///< the largest height


 template< class N >
 [[nodiscard]] N * make( void );

 void free( Leaf * x ) noexcept;

 void free( Inner * x ) noexcept;

 static void clean( K * a , std::size_t from , std::size_t to );

 static void fetch( const void * x , std::size_t bytes );

 template< std::size_t C >
 [[nodiscard]] std::size_t rank( const K * a , std::uint32_t n ,
                                 const K & x ) const;

 template< bool Values >
 [[nodiscard]] const Leaf * descend( const K & x ,
                                     const Inner * * up = nullptr ,
                                     std::uint32_t * at = nullptr ) const;

 /// the parent of the leftmost leaf, or of the rightmost one
 [[nodiscard]] const Inner * edge( bool right ) const;

 [[nodiscard]] const Leaf * descend( const K & x , Step * path ) const;

 [[nodiscard]] bool equal( const K & a , const K & b ) const;

 void split( Leaf * x , std::uint32_t p , const K & k , const V & v ,
             Step * path , int d );

 void climb( K s , void * r , Step * path , int d , int edge );

 void shrink( Leaf * x , Step * path , int d );

 void shrink( Inner * x , Step * path , int d );

 void destroy( void * x , int h ) noexcept;

 [[nodiscard]] bool valid( void ) const;

 [[nodiscard]] bool valid( const void * x , int h , const K * lo ,
                           const K * hi , const Leaf * & last ,
                           std::vector< const Inner * > & left ,
                           std::size_t & count ) const;

 void * f_root = nullptr;          ///< the root, a leaf if f_height is 1
 Leaf * f_first = nullptr;         ///< the leftmost leaf
 Leaf * f_last = nullptr;          ///< the rightmost leaf
 std::size_t f_size = 0;           ///< the pairs
 int f_height = 0;                 ///< the levels
 [[no_unique_address]] Less f_less;   ///< the order of the keys
 ObjectPool< Leaf > f_leaves;      ///< the slots of the leaves
 ObjectPool< Inner > f_inners;     ///< the slots of the internal nodes

/*--------------------------------------------------------------------------*/
 public:

 class const_iterator {
  public:
  using value_type = std::pair< const K & , const V & >;
  using reference = value_type;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;

  reference operator*( void ) const {
   return( reference( f_l->k[ f_i ] , f_l->v[ f_i ] ) );
   }

  const_iterator & operator++( void ) {
   if( ++f_i == f_n ) [[unlikely]]
    step();
   return( *this );
   }

  const_iterator & operator--( void ) {
   if( ! f_l ) {                            // from the end
    f_l = f_t->f_last;
    f_p = f_t->edge( true );
    f_c = f_p ? f_p->n : 0;
    f_i = f_n = f_l->n;
    }
   else if( f_i == 0 ) {
    if( f_p ) {
     if( f_c == 0 ) {
      f_p = f_p->prev;
      f_c = f_p->n;
      }
     else
      --f_c;
     f_l = static_cast< const Leaf * >( f_p->c[ f_c ] );
     }
    else
     f_l = f_l->prev;
    f_i = f_n = f_l->n;
    }
   --f_i;
   return( *this );
   }

  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( ( f_l == o.f_l ) && ( f_i == o.f_i ) );
   }

  private:
  friend class BTreeMap;

  /// the leaves this many places ahead are prefetched
  static constexpr std::uint32_t AHEAD = 4;

  const_iterator( const BTreeMap * t , const Leaf * l , std::uint32_t i ,
                  const Inner * p , std::uint32_t c )
   : f_t( t ) , f_l( l ) , f_p( p ) , f_i( i ) , f_n( l ? l->n : 0 ) ,
     f_c( c ) {
   if( f_l && ( f_i == f_n ) )              // past a leaf: the next one
    step();
   }

  /// to the first pair of the next leaf, through the parents, whose
  /// children give the leaves ahead to prefetch with no chase of the
  /// links between the leaves
  void step( void ) {
   f_i = 0;
   if( f_p ) {
    if( f_c == f_p->n ) {
     f_p = f_p->next;
     f_c = 0;
     if( f_p && f_p->next )
      fetch( f_p->next , sizeof( Inner ) );
     }
    else
     ++f_c;
    if( f_p ) {
     f_l = static_cast< const Leaf * >( f_p->c[ f_c ] );
     const Inner * q = f_p;
     std::uint32_t a = f_c + AHEAD;
     if( a > q->n ) {
      a -= q->n + 1;
      q = q->next;
      }
     if( q && ( a <= q->n ) )
      fetch( q->c[ a ] , sizeof( Leaf ) );
     }
    else
     f_l = nullptr;
    }
   else
    f_l = f_l->next;
   f_n = f_l ? f_l->n : 0;
   }

  const BTreeMap * f_t = nullptr;   ///< the tree
  const Leaf * f_l = nullptr;       ///< the leaf, nullptr at the end
  const Inner * f_p = nullptr;      ///< its parent, nullptr if the root
  std::uint32_t f_i = 0;            ///< the place in the leaf
  std::uint32_t f_n = 0;            ///< the pairs of the leaf
  std::uint32_t f_c = 0;            ///< the child of the parent

 };  // end( class const_iterator )

 };  // end( class BTreeMap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BTreeMap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BTreeMap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BTreeMap.h ----------------------------*/
/*--------------------------------------------------------------------------*/
