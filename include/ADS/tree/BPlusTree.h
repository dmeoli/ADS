/*--------------------------------------------------------------------------*/
/*---------------------------- File BPlusTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B+-tree of order \f$ m \f$ of the reality check of chapter 6, the
 * search tree of the file systems and of the databases, whose nodes are
 * pages of the secondary memory.
 *
 * An internal node has \f$ f \f$ children,
 * \f$ \lceil m / 2 \rceil \leq f \leq m \f$ (the root at least 2), and
 * \f$ f - 1 \f$ keys \f$ k[ 0 ] , \ldots , k[ f - 2 ] \f$: the child
 * \f$ p[ i ] \f$ holds the keys in \f$ ( k[ i - 1 ] , k[ i ] ] \f$,
 * \f$ p[ 0 ] \f$ those up to \f$ k[ 0 ] \f$ and \f$ p[ f - 1 ] \f$ those
 * larger than \f$ k[ f - 2 ] \f$. The values are only in the leaves, which
 * hold up to \f$ m \f$ keys each, are all on the same level and are linked in
 * a list in the order of the keys, so that the \f$ k \f$ keys in an interval
 * are read in \f$ O( \log n + k ) \f$ time [see `range()`]. The height is
 * \f$ O( \log_{\lceil m / 2 \rceil} n ) \f$: with pages of 4 KB and
 * \f$ m = 256 \f$ a few levels index billions of keys.
 *
 * The search goes down to the first child whose key is not smaller than the
 * one looked for. The insertion goes into the leaf of the key; a leaf with
 * \f$ m \f$ keys already splits its \f$ m + 1 \f$ keys between a new leaf on
 * its left, with the first \f$ \lceil ( m + 1 ) / 2 \rceil \f$, and itself,
 * and the largest key of the new leaf goes into the parent next to it; a
 * parent with \f$ m + 1 \f$ children splits in the same way, the middle key
 * going one level up, up to the root, which may split and give a new root
 * with two children. The book describes the removal only as a series of
 * cases similar to those of the insertion, and so it is not here [see
 * `BTree.h` and `BTreeMap.h` for the removal from a B-tree].
 *
 * The contracts [see `Contract.h`]: the order is \f$ m > 2 \f$, as the book
 * requires, and the invariant is the shape of the tree, checked after each
 * insertion [see `shaped()`]. The operations are defined in
 * `BPlusTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 6.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
 * \see R. Bayer, E. M. McCreight, Organization and maintenance of large
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

/// a B+-tree of order \f$ m \f$, without the removal
template< std::totally_ordered K , class V >
class BPlusTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// an empty B+-tree of order \f$ m > 2 \f$, the root a leaf
 /// \pre \f$ m > 2 \f$
 explicit BPlusTree( int m );

 /// no copy, since the nodes are linked by pointers
 BPlusTree( const BPlusTree & ) = delete;

 /// no copy, since the nodes are linked by pointers
 BPlusTree & operator=( const BPlusTree & ) = delete;

 /// frees the nodes
 ~BPlusTree();

/*--------------------------------------------------------------------------*/

 /// the value of the key \f$ x \f$, or nothing
 ///
 /// In pseudocode (section 6.3), \f$ u.k[ 0 .. f - 2 ] \f$ the keys and
 /// \f$ u.p[ 0 .. f - 1 ] \f$ the children of an internal node \f$ u \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( x ) \cr
 /// 2 & \quad u \gets \mathrm{leaf}( x ) \cr
 /// 3 & \quad i \gets \text{the first place of } u \text{ with } u.k[ i ]
 ///     \ge x \cr
 /// 4 & \quad \textbf{if}\ i \text{ is a place of } u\ \textbf{and}\ u.k[
 ///     i ] = x\ \textbf{then return}\ u.v[ i ]\ \textbf{else return nil} \cr
 /// 5 & \textbf{function}\ \mathrm{leaf}( x )\quad \triangleright
 ///     \ \text{the leaf where } x \text{ is, or would be} \cr
 /// 6 & \quad u \gets \mathit{root} \cr
 /// 7 & \quad \textbf{while}\ u \text{ is not a leaf}\ \textbf{do}\ u
 ///     \gets u.p[ i ] \text{, } i \text{ the first place with } u.k[ i ]
 ///     \ge x \cr
 /// 8 & \quad \textbf{return}\ u
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( const K & x ) const;

 /// associates the value \f$ v \f$ to the key \f$ x \f$
 ///
 /// In pseudocode (section 6.3), the splits going up from the leaf as far
 /// as they are needed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x , v ) \cr
 /// 2 & \quad f \gets \mathrm{leaf}( x ) \cr
 /// 3 & \quad \textbf{if}\ x \text{ is in } f\ \textbf{then}
 ///     \ \text{give it the value } v;\ \textbf{return} \cr
 /// 4 & \quad \text{put } ( x , v ) \text{ in } f \text{ in the order of }
 ///     \text{the keys} \cr
 /// 5 & \quad \textbf{if}\ f \text{ has } m \text{ keys or fewer}
 ///     \ \textbf{then return} \cr
 /// 6 & \quad g \gets \text{a new leaf with the first }
 ///     \lceil ( m + 1 ) / 2 \rceil \text{ pairs of } f \text{, linked }
 ///     \text{before } f \cr
 /// 7 & \quad \mathrm{up}( f , g , \text{the largest key of } g ) \cr
 /// 8 & \textbf{procedure}\ \mathrm{up}( f , g , s )\quad \triangleright
 ///     \ g \text{ goes before its sibling } f \cr
 /// 9 & \quad b \gets \text{the parent of } f \cr
 /// 10 & \quad \textbf{if}\ b = \textbf{nil}\ \textbf{then}
 ///     \ \mathit{root} \gets \text{a node with the key } s \text{ and the }
 ///     \text{children } g , f;\ \textbf{return} \cr
 /// 11 & \quad \text{put } g \text{ before } f \text{ in } b \text{, and } s
 ///     \text{ before the key that follows } g \cr
 /// 12 & \quad \textbf{if}\ b \text{ has } m \text{ children or fewer}
 ///     \ \textbf{then return} \cr
 /// 13 & \quad c \gets \text{a new node with the first }
 ///     \lceil ( m + 1 ) / 2 \rceil \text{ children of } b \text{ and the }
 ///     \text{keys between them} \cr
 /// 14 & \quad \mathrm{up}( b , c , \text{the key that separated } c
 ///     \text{ from the rest of } b )
 /// \end{array}}
 /// \f]
 void insert( const K & x , const V & v );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the order \f$ m \f$
 [[nodiscard]] int order( void ) const;

 /// the pairs with keys in \f$ [ A , B ] \f$, read along the list of the
 /// leaves, \f$ O( \log n + k ) \f$ for \f$ k \f$ pairs
 ///
 /// In pseudocode (section 6.3), the leaf of \f$ A \f$ and then the
 /// following ones:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( A , B ) \cr
 /// 2 & \quad \mathit{out} \gets \text{the empty sequence};\ f \gets
 ///     \mathrm{leaf}( A ) \cr
 /// 3 & \quad \textbf{while}\ f \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{foreach}\ ( k , v ) \text{ of } f \text{, in order}
 ///     \ \textbf{do} \cr
 /// 5 & \qquad\quad \textbf{if}\ k > B\ \textbf{then return}
 ///     \ \mathit{out} \cr
 /// 6 & \qquad\quad \textbf{if}\ k \ge A\ \textbf{then}\ \text{append }
 ///     ( k , v ) \text{ to } \mathit{out} \cr
 /// 7 & \qquad f \gets f.\mathit{next} \cr
 /// 8 & \quad \textbf{return}\ \mathit{out}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< std::pair< K , V > > range( const K & A ,
                                                       const K & B ) const;

 /// the keys of the nodes, level by level from the root, each level from
 /// left to right: the shape of the tree, as in figure 6.7, by a visit in
 /// breadth
 [[nodiscard]] std::vector< std::vector< std::vector< K > > > levels( void )
  const;

 /// checks the shape [see `shaped()`] whatever the level of the contracts;
 /// it throws `contract_violation` if it fails
 void check( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, internal or a leaf
 struct Node {
  /// an empty node, a leaf if \f$ l \f$
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

 /// the iterator on the pairs by increasing keys, a leaf and a place in it
 class const_iterator {
  public:
  /// a pair of constant references, the key and its value
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;             ///< the pairs
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// the category of the iterator for the concepts of C++20, bidirectional
  using iterator_concept = std::bidirectional_iterator_tag;
  /// input for the older algorithms, since `*it` is a value
  using iterator_category = std::input_iterator_tag;

  /// an iterator on no tree
  const_iterator( void ) = default;

  /// on the place \f$ i \f$ of the leaf \f$ f \f$ of the tree \f$ t \f$,
  /// the end if \f$ f \f$ is nil
  const_iterator( const BPlusTree * t , const Node * f , std::size_t i )
   : f_t( t ) , f_f( f ) , f_i( i ) {}

  /// the pair of the place
  reference operator*( void ) const {
   return( reference( f_f->k[ f_i ] , f_f->v[ f_i ] ) );
   }

  /// to the next pair, along the list of the leaves
  const_iterator & operator++( void ) {
   if( ++f_i < f_f->k.size() ) return( *this );
   f_f = f_f->next;
   f_i = 0;
   while( f_f && f_f->k.empty() ) f_f = f_f->next;
   return( *this );
   }

  /// to the previous pair, from the end to the last pair
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

  /// to the next pair, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// to the previous pair, returning the iterator before the step
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  /// equal on the same place of the same leaf
  bool operator==( const const_iterator & o ) const {
   return( ( f_f == o.f_f ) && ( f_i == o.f_i ) );
   }

  private:
  const BPlusTree * f_t = nullptr;          ///< the tree
  const Node * f_f = nullptr;               ///< the leaf, nil at the end
  std::size_t f_i = 0;                      ///< the place in the leaf
  };

 /// the pairs of constant references ( key , value )
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  const Node * f = firstLeaf();
  while( f && f->k.empty() ) f = f->next;
  return( const_iterator( this , f , 0 ) );
  }
 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , nullptr , 0 ) );
  }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the leftmost leaf
 [[nodiscard]] const Node * firstLeaf( void ) const {
  const Node * x = f_root;
  while( x && ! x->is_leaf ) x = x->p.front();
  return( x );
  }
 /// the rightmost leaf
 [[nodiscard]] const Node * lastLeaf( void ) const {
  const Node * x = f_root;
  while( x && ! x->is_leaf ) x = x->p.back();
  return( x );
  }

 /// the leaf where the key \f$ x \f$ is, or would be
 Node * leaf( const K & x ) const;

 /// inserts the new node \f$ g \f$, with largest key \f$ s \f$, just before
 /// its sibling \f$ f \f$ in their parent, splitting the parents that get
 /// \f$ m + 1 \f$ children
 void up( Node * f , Node * g , const K & s );

 /// the shape: the keys in order and in their intervals, the number of
 /// children, the leaves on one level and linked in order, \f$ O( n ) \f$
 [[nodiscard]] bool shaped( void ) const;

 /// the shape of the subtree of \f$ u \f$, its keys in
 /// \f$ ( \mathit{lo} , \mathit{hi} ] \f$, at the depth \f$ d \f$
 [[nodiscard]] bool shaped( const Node * u , const K * lo , const K * hi ,
                            int d , int & depth ,
                            const Node * & last ) const;

 /// frees the nodes of the subtree of \f$ u \f$
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
