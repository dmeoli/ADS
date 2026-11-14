/*--------------------------------------------------------------------------*/
/*--------------------------- File HashedTreap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The treap that stores no priorities (exercise 7.6 of Morin), beyond the
 * book: the treap of Seidel and Aragon [see `Treap.h`] whose priority of a
 * node is the simple tabulation hash of its address [see
 * `RandomHashing.h`], computed when needed, so that a node keeps only its
 * pair and its two links. It is a DICTIONARY [see `ADT.h`] with the
 * `find(x)` of an SSet (`lowerBound()`), and its operations are those of
 * `Treap`: an insertion goes down to a leaf and rotates the new node up while
 * its priority beats its parent's, a removal rotates the node down until it
 * has one child at most.
 *
 * For the analysis of the treap to hold the priorities must behave as random
 * and independent of the keys, which a fixed function of the addresses does
 * not guarantee; the proof of Lemma 7.1 of Morin needs that, among any
 * \f$ k \f$ nodes, each has the largest priority with probability
 * \f$ O( 1 / k ) \f$ (the min-wise independence of the discussion of
 * section 7.3 of Morin), and the simple tabulation hashing of words of 8
 * characters, its tables drawn from the seed, gives each of \f$ k \f$
 * addresses the extreme hash with probability \f$ ( 1 \pm \epsilon ) / k \f$,
 * \f$ \epsilon = O( \log^2 k / k^{1/8} ) \f$ (Theorem 3 of Pătraşcu and
 * Thorup, for the minimum, and the maximum is the minimum of the
 * complements), i.e., \f$ O( 1 / k ) \f$; hence the expected depth
 * stays \f$ O( \log n ) \f$, whatever the keys and the order of the
 * operations, as long as they do not depend on the tables. The shape depends
 * on the addresses that the allocator gives, so that two runs build the same
 * set into different trees.
 *
 * Where it is meant to differ from the `Treap` of Morin, for the benchmarks to
 * check: a node is smaller by the priority and the parent, which may make it
 * fit a smaller class of the allocator; the price is a hash of 8 lookups in a
 * table of 16 KB, two per level climbed by the new node, which stops at the
 * first parent of larger priority (fewer than 2 rotations on average with
 * independent priorities), and two per rotation of a removal.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the key is there
 * and `remove()` that it is not; the invariant is the order of the keys and
 * the heap order of the hashes, an \f$ O( n ) \f$ check at the level 3. The
 * operations are defined in `HashedTreap.tpp`.
 *
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see M. Pătraşcu, M. Thorup, The power of simple tabulation hashing,
 *      Journal of the ACM 59(3), 14:1-14:50, 2012,
 *      doi:10.1145/2220357.2220361
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, sections 7.2 and 7.3 and
 *      exercise 7.6
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

#ifndef __ADS_HashedTreap
 #define __ADS_HashedTreap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"

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
/*--------------------------- CLASS HashedTreap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a treap whose priorities are the hashes of the addresses
/// of the nodes
template< std::totally_ordered K , class V >
class HashedTreap {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty treap, the tables of its hash drawn from the seed
 explicit HashedTreap( std::uint64_t seed = 1 ) : f_h( seed ) {}

 /// the value of \f$ k \f$, or nothing, in \f$ O( \log n ) \f$ expected time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$, in \f$ O( \log n ) \f$ expected time,
 /// by the insertion of `Treap::insert()` with \f$ p( u ) =
 /// h( \mathrm{address}( u ) ) \f$
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if it is there, in \f$ O( \log n ) \f$ expected
 /// time, by the removal of `Treap::remove()` with \f$ p( u ) =
 /// h( \mathrm{address}( u ) ) \f$
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the height, \f$ -1 \f$ if empty, in \f$ O( n ) \f$
 [[nodiscard]] int height( void ) const;

 /// the smallest key not smaller than \f$ k \f$ and its value, or nothing
 /// (the `find(x)` of an SSet of Morin)
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
  const K & k ) const;

 /// the address of the key \f$ k \f$, nil if absent, in \f$ O( \log n ) \f$
 /// expected time: the `find( x )` of a USet of Morin [see `DictionarySet`];
 /// valid until the key is removed
 [[nodiscard]] const K * findKey( const K & k ) const;

 /// the address of the smallest key not smaller than \f$ k \f$, nil if none,
 /// in \f$ O( \log n ) \f$ expected time: the `find( x )` of an SSet of Morin
 /// [see `DictionarySet`]; valid until the key is removed
 [[nodiscard]] const K * lowerBoundKey( const K & k ) const;

 /// `f( k , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a node: its pair and its children, no priority
 struct Node {
  K key;                           ///< the key
  V value;                         ///< the value
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
 using const_iterator = PathIterator< HashedTreap , Shape >;
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

 /// the priority of \f$ u \f$, the hash of its address
 std::uint64_t priority( const Node * u ) const {
  return( f_h( std::uint64_t( reinterpret_cast< std::uintptr_t >( u ) ) ) );
  }
 static void rotateRight( Ptr & t );
 static void rotateLeft( Ptr & t );
 int insert( Ptr & t , const K & k , const V & v );
 bool remove( Ptr & t , const K & k );
 static int height( const Ptr & t );
 bool valid( const Ptr & t , const K * lo , const K * hi ) const;

 /// the node of the smallest key not smaller than \f$ k \f$, nil if none
 [[nodiscard]] const Node * lowerBoundNode( const K & k ) const;

 Ptr f_root;                  ///< the root
 std::size_t f_size = 0;      ///< the number of keys
 TabulationHash f_h;          ///< the hash of the addresses

 };  // end( class HashedTreap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/HashedTreap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HashedTreap.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File HashedTreap.h -------------------------*/
/*--------------------------------------------------------------------------*/
