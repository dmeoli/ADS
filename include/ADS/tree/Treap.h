/*--------------------------------------------------------------------------*/
/*------------------------------ File Treap.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The treap of Seidel and Aragon, beyond the book: a binary search tree on
 * the keys that is also a heap on random priorities given to the keys when
 * they are inserted, so that its shape is that of a binary search tree
 * built by inserting the keys in a random order, whatever the order of the
 * operations; the expected depth of a node is \f$ O( \log n ) \f$, and an
 * insertion or a removal takes \f$ O( \log n ) \f$ expected time and makes
 * fewer than 2 rotations on average. An insertion puts the key in a leaf
 * and rotates it up while its priority beats its parent's, a removal
 * rotates the key down until it is a leaf. It is a DICTIONARY [see
 * `ADT.h`]; the priorities come from SplitMix64 [see `RandomHashing.h`],
 * seeded, so that a run is reproducible.
 *
 * Each node keeps also the size of its subtree, so that the treap has the
 * operations based on join [see `JoinAlgorithms.h`]: `split()`, `join()`,
 * `join2()`, `extractRange()`, the union, the intersection and the
 * difference with another treap (those of Blelloch and Reid-Miller), the
 * batches of insertions and of removals, `filter()` and `mapReduce()`,
 * sequential or on the threads of an executor; its join merges the two
 * spines by the priorities, in \f$ O( \log n ) \f$ expected time.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is
 * there and `remove()` that it is not; the invariant is the order of the
 * keys, the sizes and the heap order of the priorities, an \f$ O( n ) \f$
 * check at the level 3. The operations are defined in `Treap.tpp`.
 *
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see G. L. Steele, D. Lea, C. H. Flood, Fast splittable pseudorandom
 *      number generators, Proceedings of OOPSLA 2014, 453-472, 2014,
 *      doi:10.1145/2660193.2660195 (SplitMix64)
 * \see G. E. Blelloch, M. Reid-Miller, Fast set operations using treaps,
 *      Proceedings of the 10th ACM Symposium on Parallel Algorithms and
 *      Architectures (SPAA), 16-26, 1998, doi:10.1145/277651.277660
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

#ifndef __ADS_Treap
 #define __ADS_Treap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/tree/JoinAlgorithms.h"

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
/*------------------------------ CLASS Treap -------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V >
class Treap;

/// the adaptor of the nodes of a `Treap` [see `JoinAdaptor`]
template< std::totally_ordered K , class V >
struct TreapNodes;

/// a DICTIONARY on a treap, with the operations based on join [see
/// `Joinable`]
template< std::totally_ordered K , class V >
class Treap
 : public Joinable< Treap< K , V > , TreapNodes< K , V > , K , V > {

 friend struct TreapNodes< K , V >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty treap, its priorities drawn from the seed
 explicit Treap( std::uint64_t seed = 1 );

 /// the value of \f$ k \f$, or nothing, by the descent of a binary search
 /// tree, in \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ expected time
 ///
 /// In pseudocode (Seidel and Aragon, beyond the book), \f$ p \f$ the
 /// priorities, a new key rotated up while its priority beats its
 /// parent's:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( t , k , v ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then}\ t \gets
 ///     \text{a new node of } ( k , v ) \text{ and a random priority};
 ///     \ \textbf{return} \cr
 /// 3 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then} \cr
 /// 4 & \qquad \mathrm{insert}( t.\mathit{left} , k , v ) \cr
 /// 5 & \qquad \textbf{if}\ p( t.\mathit{left} ) > p( t )\ \textbf{then}
 ///     \ \mathrm{rotateRight}( t ) \cr
 /// 6 & \quad \textbf{else if}\ t.\mathit{key} < k\ \textbf{then}
 ///     \ \text{the mirror of the lines 4 and 5} \cr
 /// 7 & \quad \textbf{else}\ t.\mathit{value} \gets v
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ expected
 /// time
 ///
 /// In pseudocode (Seidel and Aragon, beyond the book), the key rotated
 /// down, the child of higher priority going up, until it has one child at
 /// most:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( t , k ) \cr
 /// 2 & \quad \textbf{if}\ t = \textbf{nil}\ \textbf{then return} \cr
 /// 3 & \quad \textbf{if}\ k < t.\mathit{key}\ \textbf{then}
 ///     \ \mathrm{remove}( t.\mathit{left} , k ) \cr
 /// 4 & \quad \textbf{else if}\ t.\mathit{key} < k\ \textbf{then}
 ///     \ \mathrm{remove}( t.\mathit{right} , k ) \cr
 /// 5 & \quad \textbf{else if}\ t \text{ has one child at most}
 ///     \ \textbf{then}\ t \gets \text{that child} \cr
 /// 6 & \quad \textbf{else if}\ p( t.\mathit{left} ) > p( t.\mathit{right}
 ///     )\ \textbf{then}\ \mathrm{rotateRight}( t );\ \mathrm{remove}(
 ///     t.\mathit{right} , k ) \cr
 /// 7 & \quad \textbf{else}\ \mathrm{rotateLeft}( t );\ \mathrm{remove}(
 ///     t.\mathit{left} , k )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the height, -1 if empty
 [[nodiscard]] int height( void ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node, with its priority and the size of its subtree
 struct Node {
  K key;                         ///< the key
  V value;                       ///< the value
  std::uint64_t priority;        ///< the random priority
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
 using const_iterator = PathIterator< Treap , Shape >;
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
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 bool insert( Ptr & t , const K & k , const V & v );
 static bool remove( Ptr & t , const K & k );
 static int height( const Ptr & t );
 template< class F >
 static void visit( const Ptr & t , F & f );
 static bool valid( const Ptr & t , const K * lo , const K * hi );

 Ptr f_root;
 std::size_t f_size = 0;
 SplitMix64 f_g;

 };  // end( class Treap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/Treap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Treap.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Treap.h ----------------------------*/
/*--------------------------------------------------------------------------*/
