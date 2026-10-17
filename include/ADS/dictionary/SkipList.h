/*--------------------------------------------------------------------------*/
/*---------------------------- File SkipList.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The skip list of Pugh, beyond the book: a sorted list of the keys with
 * express lanes, each node being in the levels 0 to L - 1, L drawn with
 * probability \f$ 2^{-L} \f$, and each level a sorted list of its nodes;
 * a search goes along the highest level and drops a level before passing
 * the key, in \f$ O( \log n ) \f$ expected time, and an insertion or a
 * removal links or unlinks the node on its levels. It is a DICTIONARY [see
 * ADT.h] as simple as a list and, with the right levels, as fast as a
 * balanced tree, whose balance is in the random levels rather than in
 * rotations; the generator is SplitMix64 [see RandomHashing.h], seeded, so
 * that a run is reproducible.
 *
 * The contracts [see Contract.h]: insert() ensures that the key is there
 * and remove() that it is not; the invariant is that each level is sorted
 * and a sublist of the level below, an O( n log n ) check at the level 3.
 * The operations are defined in SkipList.tpp.
 *
 * \see W. Pugh, Skip lists: a probabilistic alternative to balanced trees,
 *      Communications of the ACM 33(6), 668-676, 1990,
 *      doi:10.1145/78973.78977
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_SkipList
 #define __ADS_SkipList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SkipList ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a skip list
template< std::totally_ordered K , class V >
class SkipList {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 explicit SkipList( std::uint64_t seed = 1 );

 SkipList( const SkipList & ) = delete;
 SkipList & operator=( const SkipList & ) = delete;

 /// the nodes of other, which is left empty
 SkipList( SkipList && other ) noexcept;

 ~SkipList();

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the number of levels in use
 [[nodiscard]] int levels( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kMax = 32;   ///< the largest number of levels

 struct Node {
  K key;
  V value;
  std::vector< Node * > next;      ///< the successor on each level
  };

 /// the last node before k on each level, in update
 Node * before( const K & k , Node * update[ kMax ] ) const;

 /// each level sorted and a sublist of the one below, O( n log n )
 [[nodiscard]] bool sorted( void ) const;

 std::vector< Node * > f_head;     ///< the first node on each level
 int f_levels = 0;
 std::size_t f_size = 0;
 SplitMix64 f_g;

 };  // end( class SkipList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/SkipList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SkipList.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SkipList.h ---------------------------*/
/*--------------------------------------------------------------------------*/
