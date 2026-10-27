/*--------------------------------------------------------------------------*/
/*---------------------------- File JoinTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary search trees balanced by join alone, and the augmented maps,
 * beyond the book: `JoinTree< K , V , B , G >` is a DICTIONARY [see
 * `ADT.h`] of keys `K` and values `V` whose only balancing operation is the
 * join of the scheme `B` [see `JoinAlgorithms.h`], `AVLBalance`,
 * `RedBlackBalance`, `WeightBalance` or `TreapBalance`, so that the
 * insertions and the removals are a split or a descent followed by joins,
 * and the bulk operations of `Joinable` (union, intersection, difference,
 * batches of insertions and of removals, filter, map and reduce, extraction
 * of a range) are those of the four schemes alike. A node keeps the size of
 * its subtree and one word for its scheme: the height plus 1 for AVL, the
 * black height and the color for Red-Black, the priority for the treap
 * (the weight is the size); `JoinAVLTree`, `JoinRedBlackTree`,
 * `WeightBalancedTree` and `JoinTreap` name the four.
 *
 * The augmentation `G` is a monoid on a type `G::Value`, of identity
 * `G::identity()` and associative operation `G::combine()`, and a map
 * `G::from( k , v )` of a pair into it [see `Augmentation`]. A node
 * \f$ u \f$ keeps the augmented value of its subtree \f$ T_u \f$,
 * \f[
 *   \mathit{aug}( u ) = \mathit{aug}( u.\mathit{left} ) \otimes
 *   \mathit{from}( u.k , u.v ) \otimes \mathit{aug}( u.\mathit{right} ) ,
 * \f]
 * the combination of the values of its pairs by increasing keys, which
 * need not be commutative; `connect()` recomputes it, so that every join,
 * and every operation built on join, keeps it at the cost of one
 * combination per node touched. `augValue()` is that of the root, in
 * \f$ O( 1 ) \f$; `augRange( lo , hi )` combines the values of the
 * \f$ O( \log n ) \f$ pieces that cover the keys of
 * \f$ [ \mathit{lo} , \mathit{hi} ] \f$: from the node where the paths to
 * \f$ \mathit{lo} \f$ and to \f$ \mathit{hi} \f$ part, the pairs met on the
 * path to \f$ \mathit{lo} \f$ with the right subtrees it leaves behind, the
 * pair of the node, and the left subtrees and the pairs of the path to
 * \f$ \mathit{hi} \f$; `augPrefix( k )` does the same on the path to
 * \f$ k \f$ alone, and `augVisit()` gives the pieces themselves, in order,
 * to a function (the range tree counts on them [see `RangeTree.h`]).
 * `SumAugmentation` and `MaxAugmentation` are the sums and the maxima of
 * the values, and `AugmentedMap` the map augmented by `G` and balanced by
 * weight, as in PAM; the interval tree [see `IntervalTree.h`] and the range
 * tree are augmented maps.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there and `remove()` that it is not; the invariant is the order of the
 * keys, the sizes, the augmented values (when `G::Value` is comparable) and
 * the balance of the scheme (the heights of AVL; no red node with a red
 * child and the same black height on every path, the root black, for
 * Red-Black; \f$ \alpha ( w_l + w_r ) \le \min( w_l , w_r ) \f$ for the
 * weights of the two children of every node; the heap order of the
 * priorities of the treap), an \f$ O( n ) \f$ check at the level 3. The
 * operations are defined in `JoinTree.tpp`.
 *
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Joinable parallel balanced
 *      binary trees, ACM Transactions on Parallel Computing 9(2), 1-41,
 *      2022, doi:10.1145/3512769
 * \see Y. Sun, D. Ferizovic, G. E. Blelloch, PAM: parallel augmented maps,
 *      Proceedings of the 23rd ACM SIGPLAN Symposium on Principles and
 *      Practice of Parallel Programming (PPoPP), 290-304, 2018,
 *      doi:10.1145/3178487.3178509
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, chapter 14 (augmenting the
 *      data structures)
 * \see J. Nievergelt, E. M. Reingold, Binary search trees of bounded
 *      balance, SIAM Journal on Computing 2(1), 33-43, 1973,
 *      doi:10.1137/0202005
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

#ifndef __ADS_JoinTree
 #define __ADS_JoinTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ SCHEMES AND AUGMENTATIONS -----------------------*/
/*--------------------------------------------------------------------------*/

struct AVLBalance {};        ///< the AVL trees [see `AVLJoin`]
struct RedBlackBalance {};   ///< the Red-Black trees [see `RedBlackJoin`]
struct WeightBalance {};     ///< the weight-balanced trees
struct TreapBalance {};      ///< the treaps [see `TreapJoin`]

/// an augmentation of a map from `K` to `V`: a monoid on `G::Value`, of
/// identity `G::identity()` and associative operation
/// `G::combine( a , b )`, and the value `G::from( k , v )` of a pair; the
/// augmented value of a subtree is the combination of the values of its
/// pairs by increasing keys
template< class G , class K , class V >
concept Augmentation = requires( const K & k , const V & v ,
                                 const typename G::Value & a ) {
 { G::identity() } -> std::convertible_to< typename G::Value >;
 { G::from( k , v ) } -> std::convertible_to< typename G::Value >;
 { G::combine( a , a ) } -> std::convertible_to< typename G::Value >;
 };

/// no augmentation: nothing is kept, nothing computed
struct NoAugmentation {
 /// the empty value
 struct Value {
  /// all equal
  bool operator==( const Value & ) const = default;
  };
 /// the empty value
 static Value identity( void ) { return( Value() ); }
 /// the empty value
 template< class K , class V >
 static Value from( const K & , const V & ) { return( Value() ); }
 /// the empty value
 static Value combine( const Value & , const Value & ) { return( Value() ); }
 };

/// the sum of the values
template< class V >
struct SumAugmentation {
 using Value = V;   ///< the sums, of the type of the values
 /// 0, the value made by `V()`
 static Value identity( void ) { return( V() ); }
 /// the value \f$ v \f$ of the pair
 template< class K >
 static Value from( const K & , const V & v ) { return( v ); }
 /// \f$ a + b \f$
 static Value combine( const Value & a , const Value & b ) {
  return( a + b );
  }
 };

/// the largest of the values (from the smallest value of the type)
template< class V >
struct MaxAugmentation {
 using Value = V;   ///< the maxima, of the type of the values
 /// the smallest value of the type
 static Value identity( void ) {
  return( std::numeric_limits< V >::lowest() );
  }
 /// the value \f$ v \f$ of the pair
 template< class K >
 static Value from( const K & , const V & v ) { return( v ); }
 /// \f$ \max( a , b ) \f$
 static Value combine( const Value & a , const Value & b ) {
  return( a < b ? b : a );
  }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS JoinTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a binary search tree balanced by the join of `B` alone,
/// augmented by `G`
template< std::totally_ordered K , class V , class B = AVLBalance ,
          class G = NoAugmentation >
 requires Augmentation< G , K , V >
class JoinTree;

/// the adaptor of the nodes of a `JoinTree` [see `JoinAdaptor`]
template< std::totally_ordered K , class V , class B , class G >
struct JoinTreeNodes;

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
class JoinTree
 : public Joinable< JoinTree< K , V , B , G > ,
                    JoinTreeNodes< K , V , B , G > , K , V > {

 friend struct JoinTreeNodes< K , V , B , G >;
 using Nodes = JoinTreeNodes< K , V , B , G >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values
 using AugValue = typename G::Value;   ///< the type of the augmentation

 /// a node: what the navigation below reads
 struct Node {
  Node * left = nullptr;        ///< the left child
  Node * right = nullptr;       ///< the right child
  std::size_t size = 1;         ///< the nodes of the subtree
  /// the height plus 1 (AVL), the black height times 2 plus 1 if red
  /// (Red-Black), the priority (treap)
  std::uint64_t meta = 0;
  [[no_unique_address]] AugValue aug{};   ///< the augmented value
  K key;                        ///< the key
  V value;                      ///< the value
  };

 /// an empty tree; the seed of the priorities, for a treap
 explicit JoinTree( std::uint64_t seed = 1 );

 /// a copy, node by node with the same shape, in \f$ O( n ) \f$ time
 JoinTree( const JoinTree & o );

 /// the nodes of `o`, which is left empty, in \f$ O( 1 ) \f$ time
 JoinTree( JoinTree && o ) noexcept;

 /// the nodes of `o`, by copy and swap
 JoinTree & operator=( JoinTree o ) noexcept;

 /// frees the nodes
 ~JoinTree();

 /// exchanges the nodes with `o`
 void swap( JoinTree & o ) noexcept;

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of \f$ k \f$, or nothing, by the descent of a binary search
 /// tree, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the
 /// insertion by joins of `JoinAlgorithms::insert()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad T \gets \mathrm{insert}( T , \text{a node alone of } ( k , v )
 ///     )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (Blelloch, Ferizovic and Sun, beyond the book), the
 /// removal by joins of `JoinAlgorithms::remove()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( k ) \cr
 /// 2 & \quad T \gets \mathrm{remove}( T , k )
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

 /// removes all the pairs
 void clear( void );

 /// the height of the tree, -1 if empty, in \f$ O( n ) \f$ time
 [[nodiscard]] int height( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
/*------------------------- AUGMENTED QUERIES ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the augmented value of the whole map, in \f$ O( 1 ) \f$ time
 [[nodiscard]] AugValue augValue( void ) const;

 /// the augmented value of the pairs with keys in
 /// \f$ [ \mathit{lo} , \mathit{hi} ] \f$, in \f$ O( \log n ) \f$ calls of
 /// `combine()`, the combination of the pieces of `augVisit()`
 [[nodiscard]] AugValue augRange( const K & lo , const K & hi ) const;

 /// the augmented value of the pairs with keys \f$ \le k \f$, in
 /// \f$ O( \log n ) \f$ calls of `combine()`
 ///
 /// In pseudocode (Sun, Ferizovic and Blelloch, beyond the book), on the
 /// path to \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{augPrefix}( k ) \cr
 /// 2 & \quad a \gets \mathrm{identity}();\ u \gets T \cr
 /// 3 & \quad \textbf{while}\ u \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ k < u.\mathit{key}\ \textbf{then}\ u \gets
 ///     u.\mathit{left} \cr
 /// 5 & \qquad \textbf{else}\ a \gets a \otimes \mathit{aug}( u.\mathit{left}
 ///     ) \otimes \mathrm{from}( u );\ u \gets u.\mathit{right} \cr
 /// 6 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 [[nodiscard]] AugValue augPrefix( const K & k ) const;

 /// `f( a )` on the augmented values \f$ a \f$ of the \f$ O( \log n ) \f$
 /// pieces (whole subtrees and single pairs) whose union is the set of the
 /// keys in \f$ [ \mathit{lo} , \mathit{hi} ] \f$, by increasing keys
 ///
 /// In pseudocode (Sun, Ferizovic and Blelloch, beyond the book), \f$ t \f$
 /// the node where the paths to \f$ \mathit{lo} \f$ and to
 /// \f$ \mathit{hi} \f$ part:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{augVisit}( \mathit{lo} , \mathit{hi} , f
 ///     ) \cr
 /// 2 & \quad t \gets \text{the first node of the descent from the root }
 ///     \text{with } \mathit{lo} \le t.\mathit{key} \le \mathit{hi} \cr
 /// 3 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
 /// 4 & \quad P \gets \text{the nodes } u \text{ with } u.\mathit{key} \ge
 ///     \mathit{lo} \text{ on the path to } \mathit{lo} \text{ in }
 ///     t.\mathit{left} \cr
 /// 5 & \quad \textbf{foreach}\ u \in P \text{, by increasing keys}
 ///     \ \textbf{do}\ f( \mathrm{from}( u ) );\ f( \mathit{aug}(
 ///     u.\mathit{right} ) ) \cr
 /// 6 & \quad f( \mathrm{from}( t ) ) \cr
 /// 7 & \quad \textbf{foreach}\ u \text{ with } u.\mathit{key} \le
 ///     \mathit{hi} \text{ on the path to } \mathit{hi} \text{ in }
 ///     t.\mathit{right} \text{, in order}\ \textbf{do} \cr
 /// 8 & \qquad f( \mathit{aug}( u.\mathit{left} ) );\ f( \mathrm{from}( u )
 ///     )
 /// \end{array}}
 /// \f]
 template< class F >
 void augVisit( const K & lo , const K & hi , F && f ) const;

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the root, `nullptr` if empty; the nodes must not be changed
 [[nodiscard]] const Node * root( void ) const { return( f_root ); }

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
 using const_iterator = PathIterator< JoinTree , Shape >;
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

/*--------------------------------------------------------------------------*/
 private:

 /// the order, the sizes, the augmented values and the balance of the
 /// scheme, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;   ///< the root
 SplitMix64 f_g;            ///< the priorities of a treap

 };  // end( class JoinTree )

/*--------------------------------------------------------------------------*/

/// the AVL tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinAVLTree = JoinTree< K , V , AVLBalance , G >;

/// the Red-Black tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinRedBlackTree = JoinTree< K , V , RedBlackBalance , G >;

/// the tree balanced by weight, in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using WeightBalancedTree = JoinTree< K , V , WeightBalance , G >;

/// the treap whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinTreap = JoinTree< K , V , TreapBalance , G >;

/// the map augmented by G, balanced by weight as in PAM
template< std::totally_ordered K , class V , class G >
using AugmentedMap = JoinTree< K , V , WeightBalance , G >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/JoinTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* JoinTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File JoinTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
