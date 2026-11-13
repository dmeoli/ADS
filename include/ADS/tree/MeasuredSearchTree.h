/*--------------------------------------------------------------------------*/
/*------------------------ File MeasuredSearchTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search tree of Morin whose nodes keep the size, the depth and
 * the height of their subtree (exercise 6.19 of Morin), beyond the book: a
 * DICTIONARY [see `ADT.h`] with the `find(x)` of an SSet (`lowerBound()`) and
 * the `getLE(x)` of exercise 6.14, whose insertions and removals keep the
 * three measures of every node exact for a constant factor on the plain
 * tree, \f$ O( h ) \f$ time.
 *
 * An insertion adds a leaf of depth one more than its parent, and only the
 * nodes on its path change their size (by 1) and possibly their height,
 * which is recomputed from the children going up. The removal is the one
 * point that needs care: the splice of the book lifts a whole subtree by
 * one level, which would change the depth of all its nodes. Here the node
 * that goes is always a leaf: the key removed is replaced by its successor
 * (or its predecessor, if there is no right subtree), whose node is then
 * emptied in the same way, and each step goes down, so that the chain ends
 * at a leaf after at most \f$ h \f$ steps; no node changes its depth, and the
 * sizes and the heights change only on the path of the leaf. A rotation
 * could keep the size and the height but not the depth, which changes in a
 * whole subtree (exercise 7.7 of Morin), and that is why the tree has none.
 *
 * Morin gives no code for it; against his `BinarySearchTree`, which it
 * extends, an insertion and a removal walk their path back up once, and a
 * removal moves at most \f$ h \f$ pairs instead of one, the price of
 * depths that never change.
 *
 * The contracts [see `Contract.h`]: after `insert()` the key is there and
 * after `remove()` it is not; the invariant, an \f$ O( n ) \f$ check at the
 * level 3, is the order of the keys, the parents and the three measures of
 * every node. The operations are defined in `MeasuredSearchTree.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 6.2 and exercises 6.14 and 6.19,
 *      doi:10.15215/aupress/9781927356388.01
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

#ifndef __ADS_MeasuredSearchTree
 #define __ADS_MeasuredSearchTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MeasuredSearchTree ------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a binary search tree whose nodes keep the size, the
/// depth and the height of their subtree
template< std::totally_ordered K , class V >
class MeasuredSearchTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the measures of a node
 struct Measures {
  std::size_t size;      ///< the nodes of its subtree
  std::size_t depth;     ///< the edges from the root to it
  long height;           ///< the height of its subtree, 0 for a leaf
  };

 /// the empty tree
 MeasuredSearchTree( void ) = default;

 /// no copy, since the nodes are linked by pointers
 MeasuredSearchTree( const MeasuredSearchTree & ) = delete;

 /// no copy, since the nodes are linked by pointers
 MeasuredSearchTree & operator=( const MeasuredSearchTree & ) = delete;

 /// frees the nodes
 ~MeasuredSearchTree();

 /// the value of \f$ k \f$, or nothing, in \f$ O( h ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( h ) \f$ time
 ///
 /// In pseudocode (exercise 6.19 of Morin), \f$ s \f$, \f$ d \f$ and
 /// \f$ h \f$ the three measures:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad p \gets \text{the last node of the search of } k \text{;
 ///     if it holds } k \text{, its value} \gets v;\ \textbf{return} \cr
 /// 3 & \quad u \gets \text{a new leaf of } ( k , v ) \text{ under } p;
 ///     \ u.s \gets 1;\ u.h \gets 0;\ u.d \gets p.d + 1 \cr
 /// 4 & \quad \textbf{for}\ w \gets p\ \textbf{up to the root do}\ w.s
 ///     \gets w.s + 1;\ w.h \gets 1 + \max\{ w.\mathit{left}.h ,
 ///     w.\mathit{right}.h \}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( h ) \f$ time
 ///
 /// In pseudocode (exercise 6.19 of Morin, beyond the book), the pair of
 /// the node replaced by that of its successor or predecessor until a
 /// leaf is reached:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad u \gets \text{the node of } k;\ \textbf{if}\ u =
 ///     \textbf{nil}\ \textbf{then return} \cr
 /// 3 & \quad \textbf{while}\ u \text{ is not a leaf}\ \textbf{do} \cr
 /// 4 & \qquad w \gets \text{the leftmost node below } u.\mathit{right}
 ///     \text{, or the rightmost below } u.\mathit{left} \cr
 /// 5 & \qquad u.\mathit{pair} \gets w.\mathit{pair};\ u \gets w \cr
 /// 6 & \quad p \gets u.\mathit{parent};\ \text{free the leaf } u \cr
 /// 7 & \quad \textbf{for}\ w \gets p\ \textbf{up to the root do}\ w.s
 ///     \gets w.s - 1;\ w.h \gets 1 + \max\{ w.\mathit{left}.h ,
 ///     w.\mathit{right}.h \}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the height, \f$ -1 \f$ if empty, read at the root in \f$ O( 1 ) \f$
 [[nodiscard]] long height( void ) const;

 /// the measures of the node of \f$ k \f$, or nothing, read in it
 [[nodiscard]] std::optional< Measures > measures( const K & k ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet), in \f$ O( h ) \f$ time
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// the pairs whose keys are not larger than \f$ x \f$, in increasing order,
 /// by `lessOrEqual()`, in \f$ O( n' + h ) \f$ time for \f$ n' \f$ pairs (the
 /// `getLE(x)` of exercise 6.14 of Morin)
 [[nodiscard]] std::vector< std::pair< K , V > > getLE( const K & x ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with its three measures
 struct Node {
  K key;                       ///< the key
  V value;                     ///< the value
  Node * left = nullptr;       ///< the left child
  Node * right = nullptr;      ///< the right child
  Node * parent = nullptr;     ///< the parent
  std::size_t size = 1;        ///< the nodes of the subtree
  std::size_t depth = 0;       ///< the depth
  long height = 0;             ///< the height of the subtree
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for `PathIterator`
 struct Shape {
  /// the root of the tree `o`
  static auto root( const auto & o ) { return( o.f_root ); }
  /// the left child of the node `n`
  static auto left( const auto * n ) { return( n->left ); }
  /// the right child of the node `n`
  static auto right( const auto * n ) { return( n->right ); }
  /// the pair of the node `n`
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 /// the pairs of references to a key and its value
 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = value_type;             ///< a pair, by value
 using const_reference = value_type;       ///< a pair, by value
 /// the constant iterator by increasing key, along the path from the root
 using const_iterator = PathIterator< MeasuredSearchTree , Shape >;
 using iterator = const_iterator;          ///< the constant iterator

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the node of \f$ k \f$, or the last node of its search, nil if empty
 Node * findLast( const K & k ) const;

 /// the height of \f$ u \f$ from its children
 static long fresh( const Node * u ) {
  return( 1 + std::max( u->left ? u->left->height : -1L ,
                        u->right ? u->right->height : -1L ) );
  }

 /// the measures exact everywhere and the keys in order, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;     ///< the root
 std::size_t f_size = 0;      ///< the number of keys

 };  // end( class MeasuredSearchTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/MeasuredSearchTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MeasuredSearchTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File MeasuredSearchTree.h ---------------------*/
/*--------------------------------------------------------------------------*/
