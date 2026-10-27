/*--------------------------------------------------------------------------*/
/*----------------------------- File AVLTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The AVL tree of Adelson-Velsky and Landis, beyond the book [see
 * `SearchTree.h` for the Red-Black tree of chapter 6]: a binary search tree
 * whose every node has subtrees of heights differing by at most 1, so that
 * its height is below \f$ 1.4405 \log_2( n + 2 ) \f$, a little more
 * balanced than a Red-Black tree (whose bound is
 * \f$ 2 \log_2( n + 1 ) \f$), at the price of more rotations on the
 * updates. Each node keeps its height; an insertion or a removal goes down
 * recursively and, on the way back, restores the balance of each node by a
 * single or a double rotation, in \f$ O( \log n ) \f$ time. It is a
 * DICTIONARY [see `ADT.h`] with `lookup()`, `insert()` and `remove()`, and
 * `forEach()` visits the pairs by increasing keys.
 *
 * Each node keeps also the size of its subtree, so that the AVL tree has
 * the operations based on join [see `JoinAlgorithms.h`]: `split()`,
 * `join()`, `join2()`, `extractRange()`, the union, the intersection and
 * the difference with another tree, the batches of insertions and of
 * removals, `filter()` and `mapReduce()`, sequential or on the threads of
 * an executor; its join goes down the spine of the higher tree to a
 * subtree of the height of the other, in \f$ O( | h_l - h_r | + 1 ) \f$
 * time.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there and `remove()` that it is not; the invariant is the order of the
 * keys, the heights and the sizes stored and the balance of every node, an
 * \f$ O( n ) \f$ check at the level 3. The operations are defined in
 * `AVLTree.tpp`.
 *
 * \see G. M. Adelson-Velsky, E. M. Landis, An algorithm for the
 *      organization of information, Soviet Mathematics Doklady 3,
 *      1259-1263, 1962
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.3
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

#ifndef __ADS_AVLTree
 #define __ADS_AVLTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS AVLTree -------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V >
class AVLTree;

/// the adaptor of the nodes of an `AVLTree` [see `JoinAdaptor`]
template< std::totally_ordered K , class V >
struct AVLTreeNodes;

/// a DICTIONARY on an AVL tree, with the operations based on join [see
/// `Joinable`]
template< std::totally_ordered K , class V >
class AVLTree
 : public Joinable< AVLTree< K , V > , AVLTreeNodes< K , V > , K , V > {

 friend struct AVLTreeNodes< K , V >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the value of \f$ k \f$, or nothing, by the descent of a binary search
 /// tree, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Knuth, section 6.2.3, beyond the book), recursive,
 /// \f$ h( \textbf{nil} ) = -1 \f$, and the balance of a node restored by a
 /// single or a double rotation on the way back:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( t , k , v ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then}\ t \gets
 ///     \text{a new node of } ( k , v );\ \textbf{return} \cr
 /// 3 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then}
 ///     \ \mathrm{insert}( t.\mathit{left} , k , v ) \cr
 /// 4 & \quad \textbf{else if}\ t.\mathit{key} < k\ \textbf{then}
 ///     \ \mathrm{insert}( t.\mathit{right} , k , v ) \cr
 /// 5 & \quad \textbf{else}\ t.\mathit{value} \gets v \cr
 /// 6 & \quad \mathrm{balance}( t ) \cr
 /// 7 & \textbf{procedure}\ \mathrm{balance}( t ) \cr
 /// 8 & \quad \text{update the height and the size of } t \cr
 /// 9 & \quad \textbf{if}\ h( t.\mathit{left} ) - h( t.\mathit{right} ) > 1
 ///     \ \textbf{then} \cr
 /// 10 & \qquad \textbf{if}\ h( t.\mathit{left}.\mathit{left} ) < h(
 ///     t.\mathit{left}.\mathit{right} )\ \textbf{then}\ \mathrm{rotateLeft}(
 ///     t.\mathit{left} ) \cr
 /// 11 & \qquad \mathrm{rotateRight}( t ) \cr
 /// 12 & \quad \textbf{else if}\ h( t.\mathit{right} ) - h(
 ///     t.\mathit{left} ) > 1\ \textbf{then}\ \text{the mirror of the }
 ///     \text{lines 10 and 11}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Knuth, section 6.2.3, beyond the book), the removal of
 /// a binary search tree with the `balance()` of `insert()` on the way
 /// back:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( t , k ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
 /// 3 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then}
 ///     \ \mathrm{remove}( t.\mathit{left} , k ) \cr
 /// 4 & \quad \textbf{else if}\ t.\mathit{key} < k\ \textbf{then}
 ///     \ \mathrm{remove}( t.\mathit{right} , k ) \cr
 /// 5 & \quad \textbf{else if}\ t \text{ has one child at most}
 ///     \ \textbf{then}\ t \gets \text{that child};\ \textbf{return} \cr
 /// 6 & \quad \textbf{else} \cr
 /// 7 & \qquad s \gets \text{the node of the smallest key of }
 ///     t.\mathit{right} \cr
 /// 8 & \qquad ( t.\mathit{key} , t.\mathit{value} ) \gets (
 ///     s.\mathit{key} , s.\mathit{value} );\ \mathrm{remove}(
 ///     t.\mathit{right} , s.\mathit{key} ) \cr
 /// 9 & \quad \mathrm{balance}( t )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there is no key
 [[nodiscard]] bool empty( void ) const;

 /// the height of the tree, -1 if it is empty
 [[nodiscard]] int height( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with the height and the size of its subtree
 struct Node {
  K key;                         ///< the key
  V value;                       ///< the value
  int h = 0;                     ///< the height of the subtree
  std::size_t s = 1;             ///< the nodes of the subtree
  std::unique_ptr< Node > left ,   ///< the left child
                          right;   ///< the right child
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, along the path from the root (the nodes have no parent)

 /// the shape of the tree, for `PathIterator`
 struct Shape {
  /// the root of the tree `o`
  static auto root( const auto & o ) { return( o.f_root.get() ); }
  /// the left child of the node `n`
  static auto left( const auto * n ) { return( n->left.get() ); }
  /// the right child of the node `n`
  static auto right( const auto * n ) { return( n->right.get() ); }
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
 using const_iterator = PathIterator< AVLTree , Shape >;
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
 using Ptr = std::unique_ptr< Node >;

 static int h( const Ptr & t ) { return( t ? t->h : -1 ); }
 static std::size_t sz( const Ptr & t ) { return( t ? t->s : 0 ); }
 static void update( Ptr & t );
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 static void balance( Ptr & t );
 bool insert( Ptr & t , const K & k , const V & v );
 bool remove( Ptr & t , const K & k );
 template< class F >
 static void visit( const Ptr & t , F & f );

 /// the order, the heights, the sizes and the balance of the subtree,
 /// \f$ O( n ) \f$
 static bool valid( const Ptr & t , const K * lo , const K * hi );

 Ptr f_root;              ///< the root
 std::size_t f_size = 0;  ///< the number of keys

 };  // end( class AVLTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/AVLTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AVLTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File AVLTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
