/*--------------------------------------------------------------------------*/
/*--------------------------- File SearchTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search tree (ABR) and the Red-Black tree of the book, as a
 * DICTIONARY [see `ADT.h`] from keys to values.
 *
 * In a binary search tree the keys in the left subtree of a node \f$ u \f$
 * are smaller than the key of \f$ u \f$, and those in the right subtree
 * larger, so that the visit in symmetric order examines the keys in
 * increasing order; `lookupNode()`, `insertNode()` and `removeNode()` go
 * down one path and take \f$ O( h ) \f$ time, \f$ h \f$ being the height of
 * the tree, which is \f$ O( n ) \f$ in the worst case.
 *
 * A Red-Black tree is a binary search tree whose nodes are red or black,
 * with the constraints (V1) the root is black, (V2) the leaves are black,
 * (V3) both children of a red node are black, (V4) all the paths from a
 * node to the leaves below it have the same number of black nodes, its
 * black height \f$ b( u ) \f$. The subtree of \f$ u \f$ has at least
 * \f$ 2^{b( u )} - 1 \f$ internal nodes (Theorem 6.1), no path from the
 * root to a leaf is longer than twice another one (Theorem 6.2), and the
 * height is
 * \f[
 *   h \leq 2 \log_2( n + 1 )
 * \f]
 * (Theorem 6.3). The procedures are those of the binary search tree, plus
 * `balanceInsert()` after an insertion and `balanceDelete()` after the
 * removal of a black node, which restore the constraints with
 * \f$ O( \log n ) \f$ changes of color and at most 3 rotations, in
 * \f$ O( \log n ) \f$ time.
 *
 * The two trees are one class, `SearchTree< K , V , RB >`, the procedures
 * of the book written once and the two balancing calls under
 * `if constexpr( RB )` where the book "highlights them in black";
 * `BinarySearchTree< K , V >` and `RedBlackTree< K , V >` are its two
 * instances. The leaves are one sentinel node, black, which the book
 * allows, shared by all the trees of the type and never written, so that
 * the trees that a split makes, or that two threads join, may point to it
 * at once. Four points of the pseudocode of the book are mended, each
 * marked in the code: `balanceDelete()` is given the parent of \f$ t \f$,
 * since \f$ t \f$ may be the sentinel, whose parent the book writes when
 * the removed node had no children; it stops when \f$ t \f$ is the root
 * rather than when \f$ t \f$ is the old root \f$ T \f$, which a rotation
 * of the case (1) may have moved down; the case (4) climbs to the root
 * instead of reading the old \f$ T \f$; and the Red-Black `insertNode()`
 * returns the root also when the key was there already.
 *
 * Each node keeps also the size of its subtree, which the insertions, the
 * removals and the rotations update, so that the Red-Black tree has the
 * operations based on join [see `JoinAlgorithms.h`]: `split()`, `join()`,
 * `join2()`, `extractRange()`, the union, the intersection and the
 * difference with another tree, the batches of insertions and of removals,
 * `filter()` and `mapReduce()`, sequential or on the threads of an
 * executor; its join is that of the Red-Black trees of Tarjan, by the
 * black heights. The binary search tree, whose height may be \f$ n \f$, has
 * none of them.
 *
 * The contracts [see `Contract.h`]: after `insert()` the key is there and
 * after `remove()` it is not; `key()`, `value()`, `successorNode()` and
 * `predecessorNode()` need a node; the invariant is the order of the keys,
 * the parents, the sizes and, for a Red-Black tree, the constraints
 * (V1)-(V4) [see `valid()`]. The operations are defined in
 * `SearchTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 6.1 and 6.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
 * \see R. Bayer, Symmetric binary B-trees: data structure and maintenance
 *      algorithms, Acta Informatica 1(4), 290-306, 1972,
 *      doi:10.1007/BF00289509
 * \see L. J. Guibas, R. Sedgewick, A dichromatic framework for balanced
 *      trees, Proceedings of the 19th FOCS, 8-21, 1978,
 *      doi:10.1109/SFCS.1978.3
 * \see R. E. Tarjan, Data Structures and Network Algorithms, SIAM, 1983,
 *      chapter 4 (the join of the Red-Black trees),
 *      doi:10.1137/1.9781611970265
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Just join for parallel
 *      ordered sets, Proceedings of the 28th ACM Symposium on Parallelism
 *      in Algorithms and Architectures (SPAA), 253-264, 2016,
 *      doi:10.1145/2935764.2935768
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

#ifndef __ADS_SearchTree
 #define __ADS_SearchTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SearchTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the color of a node of a Red-Black tree
enum class Color : unsigned char { RED ,     ///< a red node
                                   BLACK     ///< a black node
                                   };

template< std::totally_ordered K , class V , bool RB >
class SearchTree;

/// the adaptor of the nodes of a Red-Black tree [see `JoinAdaptor`]
template< std::totally_ordered K , class V , bool RB >
struct SearchTreeNodes;

/// the base of a binary search tree that is not balanced: no operation
/// based on join, whose recursion would be as deep as the tree
struct NotJoinable {};

/// a binary search tree, balanced as a Red-Black tree if `RB`, and then
/// with the operations based on join [see `Joinable`]
template< std::totally_ordered K , class V , bool RB >
class SearchTree
 : public std::conditional_t< RB ,
                              Joinable< SearchTree< K , V , RB > ,
                                        SearchTreeNodes< K , V , RB > ,
                                        K , V > ,
                              NotJoinable > {

 friend struct SearchTreeNodes< K , V , RB >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the links of a node; the sentinel leaf is a Link and nothing more
 struct Link {
  Link * parent;                ///< the parent
  Link * left;                  ///< the left child
  Link * right;                 ///< the right child
  Color color = Color::BLACK;   ///< the color, read only if `RB`
  std::size_t size = 0;         ///< the nodes of the subtree, 0 for nil
  };

 /// a node of the tree, with its key and its value
 struct Node : Link {
  K key;     ///< the key
  V value;   ///< the value
  };

 /// an empty tree
 SearchTree( void );

 /// a copy, node by node with the same shape, in \f$ O( n ) \f$ time
 SearchTree( const SearchTree & o );

 /// the nodes of `o`, which is left empty, in \f$ O( 1 ) \f$ time
 SearchTree( SearchTree && o ) noexcept;

 /// the nodes of `o`, by copy and swap
 SearchTree & operator=( SearchTree o ) noexcept;

 /// frees the nodes
 ~SearchTree();

 /// exchanges the nodes with `o`
 void swap( SearchTree & o ) noexcept;

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of the key \f$ k \f$, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$, in \f$ O( h ) \f$
 /// time
 ///
 /// In pseudocode (sections 6.1 and 6.2), the insertion of the binary
 /// search tree, `balanceInsert()` for a Red-Black tree, and the rotation
 /// of figure 6.4 (`rotateRight()` its mirror):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insertNode}( T , x , v ) \cr
 /// 2 & \quad p \gets \textbf{nil};\ u \gets T \cr
 /// 3 & \quad \textbf{while}\ u \ne \textbf{nil}\ \textbf{and}\ u.\mathit{key}
 ///     \ne x\ \textbf{do}\ p \gets u;\ u \gets \text{the child of } u
 ///     \text{ on the side of } x \cr
 /// 4 & \quad \textbf{if}\ u \ne \textbf{nil}\ \textbf{then}\ u.\mathit{value}
 ///     \gets v;\ \textbf{return}\ T \cr
 /// 5 & \quad n \gets \text{a new node of } ( x , v ) \text{, the child of }
 ///     p \text{ on the side of } x \cr
 /// 6 & \quad \textbf{if}\ \mathit{RB}\ \textbf{then}
 ///     \ \mathrm{balanceInsert}( n );\ \textbf{return}\ \text{the root} \cr
 /// 7 & \quad \textbf{if}\ p = \textbf{nil}\ \textbf{then return}\ n
 ///     \ \textbf{else return}\ T \cr
 /// 8 & \textbf{procedure}\ \mathrm{balanceInsert}( t ) \cr
 /// 9 & \quad t.\mathit{color} \gets \text{red} \cr
 /// 10 & \quad \textbf{while}\ t \ne \textbf{nil}\ \textbf{do} \cr
 /// 11 & \qquad p \gets t.\mathit{parent};\ n \gets p.\mathit{parent};\ z
 ///     \gets \text{the other child of } n \cr
 /// 12 & \qquad \textbf{if}\ p = \textbf{nil}\ \textbf{then}
 ///     \ t.\mathit{color} \gets \text{black};\ t \gets \textbf{nil}\quad
 ///     \triangleright\ \text{(1)} \cr
 /// 13 & \qquad \textbf{else if}\ p \text{ is black}\ \textbf{then}\ t
 ///     \gets \textbf{nil}\quad \triangleright\ \text{(2)} \cr
 /// 14 & \qquad \textbf{else if}\ z \text{ is red}\ \textbf{then}\quad
 ///     \triangleright\ \text{(3)} \cr
 /// 15 & \qquad\quad p.\mathit{color} \gets z.\mathit{color} \gets
 ///     \text{black};\ n.\mathit{color} \gets \text{red};\ t \gets n \cr
 /// 16 & \qquad \textbf{else if}\ t = p.\mathit{right}\ \textbf{and}\ p =
 ///     n.\mathit{left}\ \textbf{then}\ \mathrm{rotateLeft}( p );\ t \gets
 ///     p\quad \triangleright\ \text{(4a), and (4b) its mirror} \cr
 /// 17 & \qquad \textbf{else} \cr
 /// 18 & \qquad\quad \text{rotate } n \text{ away from } t \text{ (right }
 ///     \text{if } p = n.\mathit{left} \text{)}\quad \triangleright
 ///     \ \text{(5a), (5b)} \cr
 /// 19 & \qquad\quad p.\mathit{color} \gets \text{black};
 ///     \ n.\mathit{color} \gets \text{red};\ t \gets \textbf{nil} \cr
 /// 20 & \textbf{function}\ \mathrm{rotateLeft}( x ) \cr
 /// 21 & \quad y \gets x.\mathit{right};\ x.\mathit{right} \gets
 ///     y.\mathit{left};\ y.\mathit{left} \gets x \cr
 /// 22 & \quad \text{the parent of } x \text{ takes } y \text{ as its child;}
 ///     \ \text{update the parents and the sizes of } x \text{ and } y \cr
 /// 23 & \quad \textbf{return}\ y
 /// \end{array}}
 /// \f]
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if present, in \f$ O( h ) \f$ time
 ///
 /// In pseudocode (sections 6.1 and 6.2), the removal of the binary search
 /// tree and `balanceDelete()` for a Red-Black tree, the cases (1) to (4)
 /// with \f$ t \f$ a left child of \f$ p \f$ and their mirrors (5) to (8):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{removeNode}( T , x ) \cr
 /// 2 & \quad u \gets \mathrm{lookupNode}( T , x );\ \textbf{if}\ u =
 ///     \textbf{nil}\ \textbf{then return}\ T \cr
 /// 3 & \quad \textbf{if}\ u \text{ has two children}\ \textbf{then}\quad
 ///     \triangleright\ \text{(3): the successor } s \text{ in its place} \cr
 /// 4 & \qquad s \gets \mathrm{min}( u.\mathit{right} );\ ( u.\mathit{key} ,
 ///     u.\mathit{value} ) \gets ( s.\mathit{key} , s.\mathit{value} );\ u
 ///     \gets s \cr
 /// 5 & \quad t \gets \text{the only child of } u \text{, or }
 ///     \textbf{nil}\quad \triangleright\ \text{(1), (2)} \cr
 /// 6 & \quad \text{hang } t \text{ in the place of } u \text{ under }
 ///     u.\mathit{parent} \cr
 /// 7 & \quad \textbf{if}\ \mathit{RB}\ \textbf{and}\ u \text{ is black}
 ///     \ \textbf{then}\ \mathrm{balanceDelete}( t , u.\mathit{parent} ) \cr
 /// 8 & \quad \text{free } u;\ \textbf{return}\ \text{the root} \cr
 /// 9 & \textbf{procedure}\ \mathrm{balanceDelete}( t , p ) \cr
 /// 10 & \quad \textbf{while}\ p \ne \textbf{nil}\ \textbf{and}\ t \text{ is }
 ///     \text{black}\ \textbf{do} \cr
 /// 11 & \qquad f \gets p.\mathit{right};\ ( a , b ) \gets (
 ///     f.\mathit{left} , f.\mathit{right} )\quad \triangleright
 ///     \ \text{sibling and nephews} \cr
 /// 12 & \qquad \textbf{if}\ f \text{ is red}\ \textbf{then}
 ///     \ p.\mathit{color} \gets \text{red};\ f.\mathit{color} \gets
 ///     \text{black};\ \mathrm{rotateLeft}( p )\quad \triangleright
 ///     \ \text{(1)} \cr
 /// 13 & \qquad \textbf{else if}\ a \text{ and } b \text{ are black}
 ///     \ \textbf{then}\ f.\mathit{color} \gets \text{red};\ t \gets p;\ p
 ///     \gets t.\mathit{parent}\quad \triangleright\ \text{(2)} \cr
 /// 14 & \qquad \textbf{else if}\ b \text{ is black}\ \textbf{then}
 ///     \ a.\mathit{color} \gets \text{black};\ f.\mathit{color} \gets
 ///     \text{red};\ \mathrm{rotateRight}( f )\quad \triangleright
 ///     \ \text{(3)} \cr
 /// 15 & \qquad \textbf{else}\quad \triangleright\ \text{(4)} \cr
 /// 16 & \qquad\quad f.\mathit{color} \gets p.\mathit{color};
 ///     \ p.\mathit{color} \gets b.\mathit{color} \gets \text{black};
 ///     \ \mathrm{rotateLeft}( p ) \cr
 /// 17 & \qquad\quad t \gets \text{the root};\ p \gets \textbf{nil} \cr
 /// 18 & \quad \textbf{if}\ t \ne \textbf{nil}\ \textbf{then}
 ///     \ t.\mathit{color} \gets \text{black}
 /// \end{array}}
 /// \f]
 void remove( const K & k );

 /// true if the key \f$ k \f$ is present
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const;

 /// removes all the nodes
 void clear( void );

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/
 /// the root, from which the nodes can be walked as in the book, along the
 /// fields `parent`, `left` and `right` of a `Link`, until the leaf,
 /// `isNil()`; they must not be changed
 [[nodiscard]] Link * root( void ) const;

 /// true if \f$ u \f$ is the sentinel leaf (or the parent of the root)
 [[nodiscard]] bool isNil( const Link * u ) const;

 /// the key of the node \f$ u \f$
 /// \pre \f$ u \f$ is a node, not the sentinel leaf, which has no key
 ///      (only nil is checked, as a static function cannot know the
 ///      sentinel)
 [[nodiscard]] static const K & key( const Link * u );

 /// the value of the node \f$ u \f$
 /// \pre \f$ u \f$ is a node, not the sentinel leaf
 [[nodiscard]] static const V & value( const Link * u );

 /// the node of the key \f$ x \f$, nil if absent, in \f$ O( h ) \f$ time
 ///
 /// In pseudocode (section 6.1):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookupNode}( T , x ) \cr
 /// 2 & \quad \textbf{while}\ T \ne \textbf{nil}\ \textbf{and}\ T.\mathit{key}
 ///     \ne x\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ x < T.\mathit{key}\ \textbf{then}\ T \gets
 ///     T.\mathit{left}\ \textbf{else}\ T \gets T.\mathit{right} \cr
 /// 4 & \quad \textbf{return}\ T
 /// \end{array}}
 /// \f]
 [[nodiscard]] Link * lookupNode( const K & x ) const;

 /// the node of the smallest key, nil if the tree is empty, by the left
 /// path from the root
 [[nodiscard]] Link * min( void ) const;

 /// the node of the largest key, nil if the tree is empty, by the right
 /// path from the root
 [[nodiscard]] Link * max( void ) const;

 /// the node of the next key, nil after the largest one and from nil, in
 /// \f$ O( h ) \f$ time
 ///
 /// In pseudocode (section 6.1), by the links to the parents:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{successorNode}( t ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}\ t \cr
 /// 3 & \quad \textbf{if}\ t.\mathit{right} \ne \textbf{nil}
 ///     \ \textbf{then return}\ \mathrm{min}( t.\mathit{right} ) \cr
 /// 4 & \quad p \gets t.\mathit{parent} \cr
 /// 5 & \quad \textbf{while}\ p \ne \textbf{nil}\ \textbf{and}\ t =
 ///     p.\mathit{right}\ \textbf{do}\ t \gets p;\ p \gets p.\mathit{parent}
 ///     \cr
 /// 6 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ t \f$ is not a null pointer
 [[nodiscard]] Link * successorNode( Link * t ) const;

 /// the foreach of the book: `f( k , v )` on the pairs in increasing order
 /// of the keys, \f$ O( n ) \f$ time, by the visit in symmetric order
 template< class F >
 void forEach( F && f ) const;

 /// the node of the previous key, nil before the first one and from nil,
 /// `successorNode()` with left and right swapped
 /// \pre \f$ t \f$ is not a null pointer
 [[nodiscard]] Link * predecessorNode( Link * t ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional by the links to the parents, the sentinel leaf being the
 // end

 /// the constant iterator on the pairs, by increasing key
 class const_iterator {
  public:
  /// a pair of references to a key and its value
  using reference = std::pair< const K & , const V & >;
  using value_type = reference;             ///< the pairs read
  using difference_type = std::ptrdiff_t;   ///< the type of the distances
  /// bidirectional for the concepts of C++20
  using iterator_concept = std::bidirectional_iterator_tag;
  /// input for the older algorithms, since `*it` is a value
  using iterator_category = std::input_iterator_tag;

  /// an iterator on no tree
  const_iterator( void ) = default;

  /// on the node \f$ x \f$ of the tree \f$ t \f$
  const_iterator( const SearchTree * t , Link * x ) : f_t( t ) , f_x( x ) {}

  /// the pair of the node
  reference operator*( void ) const {
   return( reference( f_t->key( f_x ) , f_t->value( f_x ) ) );
   }
  /// to the next key, `successorNode()`
  const_iterator & operator++( void ) {
   f_x = f_t->successorNode( f_x );
   return( *this );
   }
  /// to the previous key, from the end to the largest one
  const_iterator & operator--( void ) {
   f_x = f_t->isNil( f_x ) ? f_t->max() : f_t->predecessorNode( f_x );
   return( *this );
   }
  /// to the next key, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// to the previous key, returning the iterator before the step
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  /// equal on the same node
  bool operator==( const const_iterator & o ) const {
   return( f_x == o.f_x );
   }

  /// the node, the sentinel at the end
  [[nodiscard]] Link * node( void ) const { return( f_x ); }

  private:
  const SearchTree * f_t = nullptr;
  Link * f_x = nullptr;
  };

 /// the pairs of references to a key and its value
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , min() ) );
  }
 /// the iterator past the largest key, the sentinel
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , nil() ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the order of the keys, the parents, the number of nodes, the sizes of
 /// the subtrees and, if `RB`, the constraints (V1)-(V4), \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 [[nodiscard]] Link * nil( void ) const;

 static K & key_ref( Link * u );

 static V & value_ref( Link * u );

 Link * new_node( const K & x , const V & v );

 /// the copy of the subtree \f$ u \f$ of `o`, hung under \f$ p \f$; iterative
 Link * copy( const SearchTree & o , Link * u , Link * p );

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.1 ------------------------*/
/*--------------------------------------------------------------------------*/

 Link * lookupNode( Link * T , const K & x ) const;

 Link * min( Link * T ) const;

 Link * max( Link * T ) const;

 /// \f$ u \f$ becomes the child of \f$ p \f$ on the side of \f$ x \f$; the
 /// sentinel takes the parent too, as `balanceDelete()` reads it
 void link( Link * p , Link * u , const K & x );

 Link * insertNode( Link * T , const K & x , const V & v );

 Link * removeNode( Link * T , const K & x0 );

/*--------------------------------------------------------------------------*/
/*------------------- THE PROCEDURES OF SECTION 6.2 ------------------------*/
/*--------------------------------------------------------------------------*/

 /// the right child \f$ y \f$ of \f$ x \f$ goes up, \f$ x \f$ down (figure
 /// 6.4)
 Link * rotateLeft( Link * x );

 /// `rotateLeft()` with left and right swapped
 Link * rotateRight( Link * x );

 static constexpr Color RED = Color::RED;
 static constexpr Color BLACK = Color::BLACK;

 void balanceInsert( Link * t );

 /// the cases (1) to (4) with \f$ t \f$ a left child of \f$ p \f$, and their
 /// mirrors (5) to (8), the same code with left and right swapped
 void balanceDelete( Link * t , Link * p );

/*--------------------------------------------------------------------------*/

 /// the sentinel leaf, one for all the trees of the type, never written
 static Link s_nil;

 Link * f_root = &s_nil;          ///< the root, the sentinel if empty
 std::size_t f_size = 0;          ///< the number of keys

 };  // end( class SearchTree )

/*--------------------------------------------------------------------------*/

/// the binary search tree (ABR) of section 6.1
template< std::totally_ordered K , class V >
using BinarySearchTree = SearchTree< K , V , false >;

/// the Red-Black tree of section 6.2
template< std::totally_ordered K , class V >
using RedBlackTree = SearchTree< K , V , true >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/SearchTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SearchTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SearchTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
