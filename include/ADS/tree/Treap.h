/*--------------------------------------------------------------------------*/
/*------------------------------ File Treap.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The treap of Seidel and Aragon, beyond the book: a binary search tree
 * on the keys that is also a heap on random priorities given to the keys
 * when they are inserted, so that its shape is that of a binary search tree
 * built by inserting the keys in a random order, whatever the order of the
 * operations; the expected depth of a node is \f$ O( \log n ) \f$, and an
 * insertion or a removal takes \f$ O( \log n ) \f$ expected time and
 * makes fewer than 2 rotations on average. An insertion puts the key in a
 * leaf and rotates it up while its priority beats its parent's, a removal
 * rotates the key down until it is a leaf. It is a DICTIONARY [see
 * ADT.h]; the priorities come from SplitMix64 [see RandomHashing.h],
 * seeded, so that a run is reproducible.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there and
 * remove() that it is not; the invariant is the order of the keys and the heap
 * order of the priorities, an \f$ O( n ) \f$ check at the level 3. The
 * operations are defined in Treap.tpp.
 *
 * \see R. Seidel, C. R. Aragon, Randomized search trees, Algorithmica
 *      16(4-5), 464-497, 1996, doi:10.1007/BF01940876
 * \see G. L. Steele, D. Lea, C. H. Flood, Fast splittable pseudorandom
 *      number generators, Proceedings of OOPSLA 2014, 453-472, 2014,
 *      doi:10.1145/2660193.2660195 (SplitMix64)
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

/// the adaptor of the nodes of a Treap [see JoinAdaptor]
template< std::totally_ordered K , class V >
struct TreapNodes;

/// a DICTIONARY on a treap, with the operations based on join [see
/// Joinable]
template< std::totally_ordered K , class V >
class Treap
 : public Joinable< Treap< K , V > , TreapNodes< K , V > , K , V > {

 friend struct TreapNodes< K , V >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 explicit Treap( std::uint64_t seed = 1 );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the height, -1 if empty
 [[nodiscard]] int height( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  K key;
  V value;
  std::uint64_t priority;
  std::size_t s = 1;             ///< the nodes of the subtree
  std::unique_ptr< Node > left , right;
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key,
 // bidirectional, along the path from the root (the nodes have no parent)

 /// the shape of the tree, for PathIterator
 struct Shape {
  static auto root( const auto & o ) { return( o.f_root.get() ); }
  static auto left( const auto * n ) { return( n->left.get() ); }
  static auto right( const auto * n ) { return( n->right.get() ); }
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using const_iterator = PathIterator< Treap , Shape >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
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
