/*--------------------------------------------------------------------------*/
/*----------------------- File RandomizedSearchTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The randomized binary search tree of Martínez and Roura (exercise 7.14 of
 * Morin), beyond the book: a binary search tree that stores no priorities
 * but the size of each subtree, and stays a random binary search tree, i.e.,
 * of the shape that the insertions of its keys in a uniformly random order
 * give, whatever the order of the operations; the expected depth of a node
 * is \f$ O( \log n ) \f$, as in a treap [see `Treap.h`], and the sizes give
 * the access by rank for free (`select()`, `rank()`).
 *
 * An insertion into the subtree \f$ t \f$ of \f$ m \f$ nodes puts the new key
 * at its root with probability \f$ 1 / ( m + 1 ) \f$, splitting \f$ t \f$ by
 * the key into its two subtrees, and goes down into the left or the right
 * subtree otherwise: the new key is the root of a random tree with exactly
 * that probability, the event that its priority, in a treap, beats the
 * \f$ m \f$ others. A removal replaces the node by the join of its two
 * subtrees, of sizes \f$ a \f$ and \f$ b \f$, whose root is the root of the
 * left one with probability \f$ a / ( a + b ) \f$ and of the right one
 * otherwise, recursively along the two spines. Morin describes the same
 * choices made by rotations (section 7.3): a new leaf rotated up to the root
 * of the subtree where the coin falls, a node rotated down by the left or by
 * the right with the probabilities of the sizes; the split and the join are
 * the original form of Martínez and Roura, with no rotation.
 *
 * The random choices are words of `SplitMix64` [see `RandomHashing.h`] turned
 * into an integer below \f$ m + 1 \f$ by `uniformBelow()` [see
 * `Rearrangements.h`], with no bias. It is a DICTIONARY [see `ADT.h`] with the
 * `find(x)` of an SSet (`lowerBound()`).
 *
 * Where it is meant to compare with the `Treap` of Morin, for the benchmarks
 * to check (exercise 7.14 asks for this comparison): a node has a size
 * instead of a priority, the same space, and the access by rank comes with
 * it; an insertion draws a random number per level until the coin falls,
 * \f$ O( 1 ) \f$ expected at each level but \f$ O( \log n ) \f$ in all,
 * against one per insertion in a treap, which is where it should lose.
 *
 * The contracts [see `Contract.h`]: `select()` needs a rank below `size()`;
 * `insert()` ensures that the key is there and `remove()` that it is not; the
 * invariant is the order of the keys and the sizes, an \f$ O( n ) \f$ check at
 * the level 3. The operations are defined in `RandomizedSearchTree.tpp`.
 *
 * \see C. Martínez, S. Roura, Randomized binary search trees, Journal of the
 *      ACM 45(2), 288-323, 1998, doi:10.1145/274787.274812
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, section 7.3 and exercise 7.14,
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

#ifndef __ADS_RandomizedSearchTree
 #define __ADS_RandomizedSearchTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/sequence/Rearrangements.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS RandomizedSearchTree -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a randomized binary search tree of Martínez and Roura
template< std::totally_ordered K , class V >
class RandomizedSearchTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty tree, its random choices drawn from the seed
 explicit RandomizedSearchTree( std::uint64_t seed = 1 ) : f_g( seed ) {}

 /// the value of \f$ k \f$, or nothing, in \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Martínez and Roura, beyond the book), \f$ s \f$ the
 /// sizes, after a search that found \f$ k \f$ absent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( t , k , v ) \cr
 /// 2 & \quad \textbf{if}\ \mathrm{below}( s( t ) + 1 ) = 0\ \textbf{then} \cr
 /// 3 & \qquad ( a , b ) \gets \mathrm{split}( t , k );\ \textbf{return}
 ///     \ \text{a node of } ( k , v ) \text{ over } a \text{ and } b \cr
 /// 4 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then}
 ///     \ t.\mathit{left} \gets \mathrm{insert}( t.\mathit{left} , k , v ) \cr
 /// 5 & \quad \textbf{else}\ t.\mathit{right} \gets \mathrm{insert}(
 ///     t.\mathit{right} , k , v ) \cr
 /// 6 & \quad s( t ) \gets s( t ) + 1;\ \textbf{return}\ t \cr
 /// 7 & \textbf{function}\ \mathrm{split}( t , k ) \cr
 /// 8 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return}
 ///     \ ( \textbf{nil} , \textbf{nil} ) \cr
 /// 9 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then}\ ( a , b )
 ///     \gets \mathrm{split}( t.\mathit{left} , k );\ t.\mathit{left} \gets
 ///     b;\ \textbf{return}\ ( a , t ) \cr
 /// 10 & \quad ( a , b ) \gets \mathrm{split}( t.\mathit{right} , k );
 ///     \ t.\mathit{right} \gets a;\ \textbf{return}\ ( t , b )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Martínez and Roura, beyond the book), the node of
 /// \f$ k \f$ replaced by the join of its subtrees:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{join}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ a = \textbf{nil}\ \textbf{or}\ b = \textbf{nil}
 ///     \ \textbf{then return}\ \text{the other} \cr
 /// 3 & \quad \textbf{if}\ \mathrm{below}( s( a ) + s( b ) ) < s( a )
 ///     \ \textbf{then}\ a.\mathit{right} \gets \mathrm{join}(
 ///     a.\mathit{right} , b );\ \textbf{return}\ a \cr
 /// 4 & \quad b.\mathit{left} \gets \mathrm{join}( a , b.\mathit{left} );
 ///     \ \textbf{return}\ b
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( sz( f_root ) ); }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] int height( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// the pair of rank \f$ i \f$, by the sizes, as `Treap::select()`
 /// \pre \f$ i < n \f$
 [[nodiscard]] std::pair< K , V > select( std::size_t i ) const;

 /// the number of keys smaller than \f$ k \f$, by the sizes
 [[nodiscard]] std::size_t rank( const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with the size of its subtree
 struct Node {
  K key;                           ///< the key
  V value;                         ///< the value
  std::size_t s = 1;               ///< the nodes of the subtree
  std::unique_ptr< Node > left ,   ///< the left child
                          right;   ///< the right child
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

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
 using const_iterator = PathIterator< RandomizedSearchTree , Shape >;
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

 static std::size_t sz( const Ptr & t ) { return( t ? t->s : 0 ); }
 static void update( Ptr & t ) { t->s = 1 + sz( t->left ) + sz( t->right ); }
 Node * find( const K & k ) const;
 void insert( Ptr & t , Ptr & u );
 static std::pair< Ptr , Ptr > split( Ptr t , const K & k );
 Ptr join( Ptr a , Ptr b );
 bool remove( Ptr & t , const K & k );
 static int height( const Ptr & t );
 static bool valid( const Ptr & t , const K * lo , const K * hi );

 Ptr f_root;                  ///< the root
 SplitMix64 f_g;              ///< the generator of the random choices

 };  // end( class RandomizedSearchTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/RandomizedSearchTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomizedSearchTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File RandomizedSearchTree.h --------------------*/
/*--------------------------------------------------------------------------*/
