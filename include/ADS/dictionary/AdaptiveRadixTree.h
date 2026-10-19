/*--------------------------------------------------------------------------*/
/*------------------------ File AdaptiveRadixTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The adaptive radix tree of Leis, Kemper and Neumann on strings, beyond
 * the book: the engineered ordered DICTIONARY of ADS [see ADT.h] whose keys
 * are strings, the one that the benchmark of the tries compares with the
 * HAT-trie of tsl, with libart and with the B-trees of strings, while Trie
 * and RadixTree [see Trie.h] stay the tries of Fredkin and of Morrison.
 *
 * A node at depth d stands for the prefixes of d bytes of its keys and
 * branches on their byte d, and it adapts its layout to the number of its
 * children, so that a node is never much larger than its pointers:
 *
 * - Node4 and Node16 keep up to 4 and 16 bytes sorted beside the pointers
 *   to the children of the same places; a Node16 is searched by one
 *   comparison of vectors of SSE2 (or NEON), the 16 bytes against the byte
 *   of the key at once;
 * - Node48 keeps an index of 256 bytes, the place of the child of each
 *   byte among its 48 pointers (0 for none);
 * - Node256 keeps the 256 pointers, the byte of the key being the place.
 *
 * A node grows to the next layout when it is full and shrinks to the
 * previous one when it holds fewer than a quarter of its places (3 in a
 * Node16, 12 in a Node48, 37 in a Node256), with a slack against the
 * repeated changes of layout on an insertion and a removal. A chain of
 * nodes with a single child is compressed into the prefix of the first
 * node that branches (the path compression): the prefix is the length of
 * the chain and its first 8 bytes, a search skipping the bytes past 8
 * (the optimistic compression of the paper, the hybrid of its section
 * III.E) and comparing the whole key at the leaf, where every search ends.
 * A leaf holds the whole key, in one allocation with its value; the key
 * that ends at a node, a prefix of all the keys below it, hangs from the
 * node apart from the children, so that any string, null bytes included,
 * can be a key. A search, an insertion and a removal take \f$ O( k ) \f$
 * time for a key of k bytes, whatever the number of the keys, with a
 * branch of \f$ O( 1 ) \f$ per node; the nodes take \f$ O( n ) \f$ words
 * for n keys, since every node branches or ends a key, and at most 52
 * bytes per key on the worst case of the paper (the leaves aside).
 *
 * The iterators visit the pairs ( key , value ) by increasing keys (a
 * key before the longer ones that begin with it, the children of a node
 * by their bytes), as a string_view on the key of the leaf and a constant
 * reference to its value, with a stack of the nodes on the way; forEach()
 * and forPrefix() visit all the pairs, and those whose key begins with a
 * prefix, by a recursion with no stack, and find() gives a value to change
 * in place. An insertion or a removal invalidates the iterators. The
 * internal nodes come from pools [see ObjectPool.h] on huge pages, the
 * leaves from the allocator.
 *
 * The contracts [see Contract.h]: after insert() the key is there and
 * after remove() it is not; the invariant is that the keys of the leaves
 * extend the paths that lead to them, that every node but the root
 * branches or ends a key, that the bytes of a node are sorted and its
 * layout the one of its count, and that the leaves are size(), an O( n )
 * check at the level 3 on the trees of up to 2048 keys. The operations
 * are defined in AdaptiveRadixTree.tpp.
 *
 * \see V. Leis, A. Kemper, T. Neumann, The adaptive radix tree: ARTful
 *      indexing for main-memory databases, Proceedings of ICDE 2013,
 *      38-49, 2013, doi:10.1109/ICDE.2013.6544812
 * \see D. R. Morrison, PATRICIA: practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
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

#ifndef __ADS_AdaptiveRadixTree
 #define __ADS_AdaptiveRadixTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS AdaptiveRadixTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered map from strings to values of type V on an adaptive radix
/// tree
template< class V >
class AdaptiveRadixTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::string;   ///< the type of the keys
 using Value = V;           ///< the type of the values

 AdaptiveRadixTree( void ) = default;

 AdaptiveRadixTree( const AdaptiveRadixTree & o );

 AdaptiveRadixTree( AdaptiveRadixTree && o ) noexcept;

 AdaptiveRadixTree & operator=( AdaptiveRadixTree o ) noexcept;

 ~AdaptiveRadixTree();

/*--------------------------------------------------------------------------*/

 /// the value of k, if there
 [[nodiscard]] std::optional< V > lookup( std::string_view k ) const;

 /// the value of k, to change in place, or nullptr
 [[nodiscard]] const V * find( std::string_view k ) const;

 [[nodiscard]] V * find( std::string_view k );

 /// gives k the value v; true if k was not there
 bool insert( std::string_view k , const V & v );

 /// removes k; true if it was there
 bool remove( std::string_view k );

 [[nodiscard]] bool contains( std::string_view k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 void clear( void );

 void swap( AdaptiveRadixTree & o ) noexcept;

 /// f( key , value ) on each pair, by increasing keys
 template< class F >
 void forEach( F && f ) const;

 /// f( key , value ) on each pair whose key begins with p, by increasing
 /// keys
 template< class F >
 void forPrefix( std::string_view p , F && f ) const;

 /// the keys that begin with p, in increasing order
 [[nodiscard]] std::vector< std::string > keysWithPrefix(
                                                  std::string_view p ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr unsigned MAXP = 8;   ///< the bytes of a prefix kept

 enum Type : std::uint8_t { N4 , N16 , N48 , N256 };

 /// a leaf: the value, and the key after it in the same allocation
 struct Leaf {
  V value;
  std::uint32_t len;
  [[nodiscard]] const char * key( void ) const {
   return( reinterpret_cast< const char * >( this + 1 ) );
   }
  [[nodiscard]] std::string_view view( void ) const {
   return( std::string_view( key() , len ) );
   }
  };

 /// what all the internal nodes begin with
 struct Node {
  Type type;
  std::uint16_t count = 0;              ///< the children
  std::uint32_t plen = 0;               ///< the length of the prefix
  unsigned char prefix[ MAXP ];         ///< its first MAXP bytes
  Leaf * term = nullptr;                ///< the key that ends here
  explicit Node( Type t ) : type( t ) {}
  };

 struct Node4 : Node {
  unsigned char key[ 4 ];
  void * child[ 4 ];
  Node4( void ) : Node( N4 ) {}
  };

 struct Node16 : Node {
  alignas( 16 ) unsigned char key[ 16 ];
  void * child[ 16 ];
  Node16( void ) : Node( N16 ) {}
  };

 struct Node48 : Node {
  unsigned char index[ 256 ];           ///< place + 1 of a byte, or 0
  void * child[ 48 ];
  Node48( void ) : Node( N48 ) { std::memset( index , 0 , 256 ); }
  };

 struct Node256 : Node {
  void * child[ 256 ];
  Node256( void ) : Node( N256 ) {
   for( auto & c : child ) c = nullptr;
   }
  };

 /// a pointer to a child is a leaf if its lowest bit is set
 [[nodiscard]] static bool isLeaf( const void * p ) {
  return( reinterpret_cast< std::uintptr_t >( p ) & 1 );
  }
 [[nodiscard]] static Leaf * asLeaf( const void * p ) {
  return( reinterpret_cast< Leaf * >( reinterpret_cast< std::uintptr_t >(
                                       p ) - 1 ) );
  }
 [[nodiscard]] static void * tag( Leaf * l ) {
  return( reinterpret_cast< void * >( reinterpret_cast< std::uintptr_t >(
                                       l ) + 1 ) );
  }

 /// the memory of the leaves: slots of 16 to 512 bytes in steps of 16, a
 /// list of the free slots for each size, cut from blocks that double up
 /// to 2 MB (on huge pages [see HugePageAllocator.h]); larger leaves come
 /// from the allocator
 class Slab {
  public:
  Slab( void ) = default;
  Slab( const Slab & ) = delete;
  Slab & operator=( const Slab & ) = delete;
  ~Slab() { release(); }
  [[nodiscard]] void * take( std::size_t bytes );
  void give( void * p , std::size_t bytes ) noexcept;
  void release( void ) noexcept;
  void swap( Slab & o ) noexcept;
  private:
  static constexpr std::size_t STEP = 16 , CLASSES = 32;
  struct Free { Free * next; };
  std::vector< std::pair< char * , std::size_t > > f_blocks;
  char * f_cur = nullptr;
  char * f_end = nullptr;
  Free * f_free[ CLASSES + 1 ] = {};
  };

 [[nodiscard]] Leaf * newLeaf( std::string_view k , const V & v );

 void freeLeaf( Leaf * l ) noexcept;

 template< class N >
 [[nodiscard]] N * make( void );

 void freeNode( Node * n ) noexcept;

 [[nodiscard]] static void * const * child( const Node * n ,
                                            unsigned char c );

 [[nodiscard]] static void * * child( Node * n , unsigned char c );

 [[nodiscard]] static const Leaf * minimum( const void * p );

 [[nodiscard]] static std::uint32_t mismatch( const Node * n ,
                                              std::string_view k ,
                                              std::size_t d );

 void addChild( void * * ref , Node * n , unsigned char c , void * x );

 /// the leaf at *ref, on the way of k down to depth d, and a new leaf of
 /// k under a new node where their keys part
 void split( void * * ref , std::size_t d , std::string_view k ,
             const V & v );

 void removeChild( void * * ref , Node * n , unsigned char c ,
                   void * * slot );

 void shrink( void * * ref , Node * n );

 [[nodiscard]] const Leaf * search( std::string_view k ) const;

 /// prefetches the lines of p that a search of the byte c reads, whatever
 /// the layout of p, while its first line is on its way
 static void ahead( const void * p , unsigned char c );

 /// prefetches the children of a Node4 or of a Node16, which a visit reads
 /// next
 static void children( const Node * n );

 void destroy( void * p ) noexcept;

 template< class F >
 static void visit( const void * p , F & f );

 template< class F >
 static void each( const Node * n , F && f );

 [[nodiscard]] bool valid( void ) const;

 [[nodiscard]] bool valid( const void * p , std::string & path ,
                           std::size_t & leaves , bool root ) const;

 void * f_root = nullptr;               ///< a node or a tagged leaf
 std::size_t f_size = 0;                ///< the keys
 ObjectPool< Node4 > f_n4;              ///< the slots of the nodes
 ObjectPool< Node16 > f_n16;
 ObjectPool< Node48 > f_n48;
 ObjectPool< Node256 > f_n256;
 Slab f_slab;                           ///< the memory of the leaves

/*------------------------- THE STANDARD INTERFACE -------------------------*/
 public:

 // a constant view of the pairs ( key , value ) by increasing keys,
 // forward, a stack of the nodes on the way to a leaf

 class const_iterator;

 using value_type = std::pair< std::string_view , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const;
 [[nodiscard]] const_iterator end( void ) const;
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 class const_iterator {
  public:
  using value_type = std::pair< std::string_view , const V & >;
  using reference = value_type;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;

  reference operator*( void ) const {
   return( reference( f_leaf->view() , f_leaf->value ) );
   }

  const_iterator & operator++( void ) {
   f_leaf = step( f_S );
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( f_leaf == o.f_leaf );
   }

  private:
  friend class AdaptiveRadixTree;

  /// a node on the way and the next place to look at, -1 for its term
  struct Frame {
   const Node * node;
   int next;
   };

  explicit const_iterator( const void * root ) {
   if( ! root )
    return;
   if( isLeaf( root ) ) {
    f_leaf = asLeaf( root );
    return;
    }
   f_S.push_back( Frame{ static_cast< const Node * >( root ) , -1 } );
   f_leaf = step( f_S );
   }

  /// the next leaf of the walk of the stack S, nullptr at its end
  static const Leaf * step( std::vector< Frame > & S );

  std::vector< Frame > f_S;           ///< the nodes on the way
  const Leaf * f_leaf = nullptr;      ///< the current leaf, or nullptr

 };  // end( class const_iterator )

 };  // end( class AdaptiveRadixTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/AdaptiveRadixTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AdaptiveRadixTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File AdaptiveRadixTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
