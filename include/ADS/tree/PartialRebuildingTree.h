/*--------------------------------------------------------------------------*/
/*---------------------- File PartialRebuildingTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Three trees balanced by partial rebuilding, from exercises 8.8 to 8.10 of
 * Morin, beyond the book: binary search trees whose insertions and removals
 * are those of the plain tree, followed by the rebuilding of the subtree of
 * the highest node of the path that a rule selects into a perfectly balanced
 * one [see `ScapegoatTree.h` for the rebuild]. Each node keeps the size of
 * its subtree, updated along the path. They are DICTIONARY [see `ADT.h`]
 * with the `find(x)` of an SSet (`lowerBound()`), and the policy is the
 * parameter of `PartialRebuildingTree`:
 *
 * - `WeightRebuildTree` (exercise 8.8): every node but the root has
 *   \f$ s( u ) \le \frac23 s( u.\mathit{parent} ) \f$; a node whose child
 *   breaks it is rebuilt. Hence the sizes shrink by \f$ 3 / 2 \f$ per level
 *   and the height is at most \f$ \log_{3/2} n \f$. After a rebuild of a
 *   subtree of \f$ s \f$ nodes its children hold at most \f$ s / 2 \f$ of
 *   them, and they must reach \f$ 2 s' / 3 \f$, \f$ s' \f$ the size then,
 *   before the next rebuild of the same node, i.e.,
 *   \f$ \Omega( s ) \f$ operations go through it in between; charging the
 *   rebuild to them, each operation pays \f$ O( 1 ) \f$ for each of the
 *   \f$ O( \log n ) \f$ nodes of its path, \f$ O( \log n ) \f$ amortized
 *   time. These are the trees of Overmars and van Leeuwen, balanced by
 *   weight and maintained by partial rebuilding rather than by rotations
 *   [see `JoinTree.h` for those by rotations].
 * - `CountdownTree` (exercise 8.9): a node has a timer, decremented by each
 *   operation whose path goes through it and set to
 *   \f$ \lfloor s( u ) / 3 \rfloor \f$ when its subtree is rebuilt; the
 *   highest node whose timer runs out is rebuilt. A node rebuilt with
 *   \f$ s \f$ nodes sees at most \f$ s / 3 \f$ operations before its next
 *   rebuild, so that a child of it holds at most \f$ \frac34 \f$ of its
 *   subtree at any time and the height is at most \f$ \log_{4/3} n + O( 1 )
 *   \f$; the rebuild costs \f$ O( s ) \f$, paid by those \f$ s / 3 \f$
 *   operations, \f$ O( 1 ) \f$ per operation and per level, hence
 *   \f$ O( \log n ) \f$ amortized time.
 * - `DynamiteTree` (exercise 8.10): each node of the path of an operation
 *   explodes with probability \f$ 1 / s( u ) \f$, its size after the
 *   operation, and the highest one that explodes is rebuilt. The expected
 *   cost of the explosions of a node is \f$ s \cdot 1 / s = O( 1 ) \f$ per
 *   operation through it, and a node rebuilt with \f$ s \f$ nodes survives
 *   the \f$ s / 3 \f$ operations it takes to unbalance it past
 *   \f$ \frac34 \f$ with probability at most \f$ e^{-1/4} \f$, which gives
 *   a depth \f$ O( \log n ) \f$ in expectation and \f$ O( \log n ) \f$
 *   expected time per operation.
 *
 * The analyses are in \ref theory_tree_partial. The random choices of
 * `DynamiteTree` come from `SplitMix64` [see `RandomHashing.h`] by
 * `uniformBelow()` [see `RandomGenerators.h`].
 *
 * Morin gives no code for them: the benchmarks compare them with
 * `ScapegoatTree`, which pays its rebuilds by \f$ q \f$ and needs no field
 * in the nodes, while these keep a size (and a timer) in each node.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is there
 * and `remove()` that it is not; the invariant, an \f$ O( n ) \f$ check at the
 * level 3, is the order of the keys, the sizes and, for `WeightRebuildTree`,
 * the balance of the weights. The operations are defined in
 * `PartialRebuildingTree.tpp`.
 *
 * \see M. H. Overmars, J. van Leeuwen, Dynamic multi-dimensional data
 *      structures based on quad- and k-d trees, Acta Informatica 17(3),
 *      267-285, 1982, doi:10.1007/BF00264354
 * \see M. H. Overmars, The Design of Dynamic Data Structures, Lecture Notes
 *      in Computer Science 156, Springer, 1983, doi:10.1007/BFb0014927
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, chapter 8 and exercises 8.8 to 8.10,
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

#ifndef __ADS_PartialRebuildingTree
 #define __ADS_PartialRebuildingTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/tree/ScapegoatTree.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the rules that select the subtree to rebuild
enum class RebuildPolicy {
 WEIGHT ,           ///< \f$ s( u ) \le \frac23 s( u.\mathit{parent} ) \f$
 COUNTDOWN ,        ///< a timer set to \f$ s( u ) / 3 \f$ that runs out
 DYNAMITE           ///< an explosion with probability \f$ 1 / s( u ) \f$
 };

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS PartialRebuildingTree -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a binary search tree balanced by partial rebuilding by
/// the rule `P`
template< std::totally_ordered K , class V , RebuildPolicy P >
class PartialRebuildingTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty tree, its random choices (if any) drawn from the seed
 explicit PartialRebuildingTree( std::uint64_t seed = 1 ) : f_g( seed ) {}

 /// no copy, since the nodes are linked by pointers
 PartialRebuildingTree( const PartialRebuildingTree & ) = delete;

 /// no copy, since the nodes are linked by pointers
 PartialRebuildingTree & operator=( const PartialRebuildingTree & ) =
  delete;

 /// the nodes of `other`, which is left empty
 PartialRebuildingTree( PartialRebuildingTree && other ) noexcept;

 /// frees the nodes
 ~PartialRebuildingTree();

 /// the value of \f$ k \f$, or nothing, in \f$ O( h ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ amortized time
 /// (expected for `DynamiteTree`)
 ///
 /// In pseudocode (exercises 8.8 to 8.10 of Morin), \f$ P \f$ the path from
 /// the root to the new leaf:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \text{the insertion of a binary search tree, along } P \cr
 /// 3 & \quad \textbf{foreach}\ u \in P\ \textbf{do}\ s( u ) \gets s( u ) +
 ///     1 \cr
 /// 4 & \quad w \gets \text{the first node of } P \text{ selected by the}
 ///     \text{ rule} \cr
 /// 5 & \quad \textbf{if}\ w \ne \textbf{nil}\ \textbf{then}
 ///     \ \mathrm{rebuild}( w )
 /// \end{array}}
 /// \f]
 /// where the rule selects \f$ u \f$ with a child of size above
 /// \f$ \frac23 s( u ) \f$ (`WEIGHT`), \f$ u \f$ whose timer, decremented,
 /// is 0 (`COUNTDOWN`), \f$ u \f$ with probability \f$ 1 / s( u ) \f$
 /// (`DYNAMITE`)
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ amortized time
 /// (expected for `DynamiteTree`), by the removal of a binary search tree
 /// and the rule of `insert()` on its path
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const {
  return( f_root ? f_root->s : 0 );
  }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] long height( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// the address of the key \f$ k \f$, nil if absent, in \f$ O( \log n ) \f$
 /// time: the `find( x )` of a USet of Morin [see `DictionarySet`]; valid
 /// until the key is removed
 [[nodiscard]] const K * findKey( const K & k ) const;

 /// the address of the smallest key not smaller than \f$ k \f$, nil if none,
 /// in \f$ O( \log n ) \f$ time: the `find( x )` of an SSet of Morin [see
 /// `DictionarySet`]; valid until the key is removed
 [[nodiscard]] const K * lowerBoundKey( const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

 /// the number of rebuilds so far
 [[nodiscard]] std::size_t rebuilds( void ) const { return( f_rebuilds ); }

 /// the nodes of all the rebuilds so far, the work they took
 [[nodiscard]] std::size_t rebuiltNodes( void ) const { return( f_work ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the timer of a node of `CountdownTree`
 struct Timer { long t = 0; };

 /// no timer
 struct NoTimer {};

 /// a node: its pair, its children, its size and, for `CountdownTree`, its
 /// timer
 struct Node
  : std::conditional_t< P == RebuildPolicy::COUNTDOWN , Timer , NoTimer > {
  K key;                       ///< the key
  V value;                     ///< the value
  Node * left = nullptr;       ///< the left child
  Node * right = nullptr;      ///< the right child
  std::size_t s = 1;           ///< the nodes of the subtree
  /// the node of a pair
  Node( const K & k , const V & v ) : key( k ) , value( v ) {}
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
 using const_iterator = PathIterator< PartialRebuildingTree , Shape >;
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

 static std::size_t sz( const Node * u ) { return( u ? u->s : 0 ); }

 /// the rule on the path, top down, and the rebuild of the first node it
 /// selects
 void rebalance( void );

 /// the subtree of \f$ f_{\mathit{path}}[ i ] \f$ rebuilt in its place
 void rebuild( std::size_t i );

 /// the order, the sizes and the balance of the weights
 [[nodiscard]] bool valid( void ) const;

 /// the node of the smallest key not smaller than \f$ k \f$, nil if none
 [[nodiscard]] const Node * lowerBoundNode( const K & k ) const;

 Node * f_root = nullptr;          ///< the root
 std::vector< Node * > f_path;     ///< the path of the last operation
 std::vector< Node * > f_buf;      ///< the buffer of the rebuilds
 SplitMix64 f_g;                   ///< the random choices of `DYNAMITE`
 std::size_t f_rebuilds = 0;       ///< the rebuilds
 std::size_t f_work = 0;           ///< the nodes rebuilt

 };  // end( class PartialRebuildingTree )

/// the tree of exercise 8.8 of Morin, balanced by weight by rebuilds
template< std::totally_ordered K , class V >
using WeightRebuildTree = PartialRebuildingTree< K , V ,
                                                 RebuildPolicy::WEIGHT >;

/// the `CountdownTree` of exercise 8.9 of Morin
template< std::totally_ordered K , class V >
using CountdownTree = PartialRebuildingTree< K , V ,
                                             RebuildPolicy::COUNTDOWN >;

/// the `DynamiteTree` of exercise 8.10 of Morin
template< std::totally_ordered K , class V >
using DynamiteTree = PartialRebuildingTree< K , V ,
                                            RebuildPolicy::DYNAMITE >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/PartialRebuildingTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PartialRebuildingTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File PartialRebuildingTree.h --------------------*/
/*--------------------------------------------------------------------------*/
