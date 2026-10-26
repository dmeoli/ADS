/*--------------------------------------------------------------------------*/
/*---------------------------- File BPlusTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B+-tree of order m of the reality check of chapter 6, the search tree
 * of the file systems and of the databases, whose nodes are pages of the
 * secondary memory.
 *
 * An internal node has f children, \f$ \lceil m / 2 \rceil \leq f \leq m \f$
 * (the root at least 2), and f - 1 keys k[ 0 ] , ... , k[ f - 2 ]: the child
 * p[ i ] holds the keys in ( k[ i - 1 ] , k[ i ] ], p[ 0 ] those up to k[ 0 ]
 * and p[ f - 1 ] those larger than k[ f - 2 ]. The values are only in the
 * leaves, which hold up to m keys each, are all on the same level and are
 * linked in a list in the order of the keys, so that the keys in an interval
 * are read in \f$ O( \log n + k ) \f$ time [see range()]. The height is \f$ O(
 * \log_{\lceil m / 2 \rceil} n ) \f$: with pages of 4 KB and m = 256 a few
 * levels index billions of keys.
 *
 * The search goes down to the first child whose key is not smaller than the
 * one looked for. The insertion goes into the leaf of the key; a leaf with m
 * keys already splits its m + 1 keys between a new leaf on its left, with
 * the first \f$ \lceil ( m + 1 ) / 2 \rceil \f$, and itself, and the largest
 * key of the new leaf goes into the parent next to it; a parent with m + 1
 * children splits in the same way, the middle key going one level up, up to
 * the root, which may split and give a new root with two children. The book
 * describes the removal only as "a series of similar cases" and so it is
 * not here.
 *
 * The contracts [see Contract.h]: the order is m > 2, as the book
 * requires, and the invariant is the shape of the tree, checked after each
 * insertion [see shaped()]. The operations are defined in BPlusTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 6.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
 * \see R. Bayer, E. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see D. Comer, The ubiquitous B-tree, ACM Computing Surveys 11(2),
 *      121-137, 1979, doi:10.1145/356770.356776
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_BPlusTree
 #define __ADS_BPlusTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BPlusTree -------------------------------*/
/*--------------------------------------------------------------------------*/

/// a B+-tree of order m, without the removal
template< std::totally_ordered K , class V >
class BPlusTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// an empty B+-tree of order m > 2, the root a leaf
 explicit BPlusTree( int m );

 BPlusTree( const BPlusTree & ) = delete;
 BPlusTree & operator=( const BPlusTree & ) = delete;

 ~BPlusTree();

/*--------------------------------------------------------------------------*/

 /// the value of the key x, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & x ) const;

 /// associates the value v to the key x
 void insert( const K & x , const V & v );

 [[nodiscard]] std::size_t size( void ) const;

 /// the order m
 [[nodiscard]] int order( void ) const;

 /// the pairs with keys in [ A , B ], read along the list of the leaves
 [[nodiscard]] std::vector< std::pair< K , V > > range( const K & A ,
                                                       const K & B ) const;

 /// the keys of the nodes, level by level from the root, each level from
 /// left to right: the shape of the tree, as in Figura 6.7
 [[nodiscard]] std::vector< std::vector< std::vector< K > > > levels( void )
  const;

 /// checks the shape [see shaped()] whatever the level of the contracts;
 /// it throws contract_violation if it fails
 void check( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  explicit Node( bool l ) : is_leaf( l ) {}
  bool is_leaf;                      ///< true if a leaf
  std::vector< K > k;                ///< the keys
  std::vector< Node * > p;           ///< the children, if internal
  std::vector< V > v;                ///< the values, if a leaf
  Node * parent = nullptr;           ///< the parent
  Node * prev = nullptr;             ///< the previous leaf
  Node * next = nullptr;             ///< the next leaf
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional along the chain of the leaves

 class const_iterator {
  public:
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  const_iterator( const BPlusTree * t , const Node * f , std::size_t i )
   : f_t( t ) , f_f( f ) , f_i( i ) {}

  reference operator*( void ) const {
   return( reference( f_f->k[ f_i ] , f_f->v[ f_i ] ) );
   }

  const_iterator & operator++( void ) {
   if( ++f_i < f_f->k.size() ) return( *this );
   f_f = f_f->next;
   f_i = 0;
   while( f_f && f_f->k.empty() ) f_f = f_f->next;
   return( *this );
   }

  const_iterator & operator--( void ) {
   if( f_f && ( f_i > 0 ) ) {
    --f_i;
    return( *this );
    }
   f_f = f_f ? f_f->prev : f_t->lastLeaf();   // from the end, the last
   while( f_f->k.empty() ) f_f = f_f->prev;
   f_i = f_f->k.size() - 1;
   return( *this );
   }

  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  bool operator==( const const_iterator & o ) const {
   return( ( f_f == o.f_f ) && ( f_i == o.f_i ) );
   }

  private:
  const BPlusTree * f_t = nullptr;
  const Node * f_f = nullptr;               ///< the leaf, nil at the end
  std::size_t f_i = 0;
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  const Node * f = firstLeaf();
  while( f && f->k.empty() ) f = f->next;
  return( const_iterator( this , f , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , nullptr , 0 ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the leftmost and the rightmost leaf
 [[nodiscard]] const Node * firstLeaf( void ) const {
  const Node * x = f_root;
  while( x && ! x->is_leaf ) x = x->p.front();
  return( x );
  }
 [[nodiscard]] const Node * lastLeaf( void ) const {
  const Node * x = f_root;
  while( x && ! x->is_leaf ) x = x->p.back();
  return( x );
  }

 /// the leaf where the key x is, or would be
 Node * leaf( const K & x ) const;

 /// inserts the new node g, with largest key s, just before its sibling f
 /// in their parent, splitting the parents that get m + 1 children
 void up( Node * f , Node * g , const K & s );

 /// the shape: the keys in order and in their intervals, the number of
 /// children, the leaves on one level and linked in order, \f$ O( n ) \f$
 [[nodiscard]] bool shaped( void ) const;

 [[nodiscard]] bool shaped( const Node * u , const K * lo , const K * hi ,
                            int d , int & depth ,
                            const Node * & last ) const;

 static void destroy( Node * u );

 int f_m;                     ///< the order
 Node * f_root;               ///< the root
 std::size_t f_size = 0;      ///< the number of keys

 };  // end( class BPlusTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/BPlusTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BPlusTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BPlusTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
