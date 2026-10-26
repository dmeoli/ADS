/*--------------------------------------------------------------------------*/
/*-------------------------- File PositionIndex.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The index of the positions of objects in a vector, beyond the book: a
 * hash table from the address of an object to its position, of 32 bits,
 * in a vector that holds it, e.g., the constraints or the variables of a
 * model, whose positions change when some of them are erased. Besides the
 * operations of a dictionary, renumber() takes the sorted positions just
 * erased from the vector and, in one pass over the table, drops the
 * objects that were there and moves every other position p to
 * \f$ p - |\{ e \in E : e < p \}| \f$, its place in the vector after the
 * erasure of E.
 *
 * The table is FlatHashTable [see FlatHashTable.h], from the address to
 * the position, with the hash of ADS, so that a search, an insertion and a
 * removal cost what they cost there: in the common case one group of 16
 * control bytes and one line of the pairs, compared all at once.
 *
 * renumber() walks all the pairs in the order of the places, which reads the
 * memory of the table once and in order, and gives each key its new position:
 * \f$ \mathrm{rank}( p ) \f$, the erased positions below p, is the count of
 * the erased positions before the word of 64 bits of p in a bit vector of them
 * plus the population count of the bits of that word below p, in
 * \f$ O( 1 ) \f$ time and with no branch [Jacobson], and a position past the
 * largest erased one, e, moves by m, the number of the erased ones; the word
 * and its count sit side by side, so that a rank costs one access to the
 * memory, which for \f$ e \le 2^{22} \f$ is a table of at most 1 MB. The keys
 * of the erased positions are gathered on the way and removed afterwards. The
 * whole of it takes \f$ O( N + m + e / 64 ) \f$ time for N places and
 * \f$ e / 4 \f$ bytes of scratch, which stays with the index for the next
 * call.
 *
 * The contracts [see Contract.h]: no key is nil, the positions are below
 * \f$ 2^{32} - 1 \f$, and the positions given to renumber() are strictly
 * increasing. The operations are defined in PositionIndex.tpp.
 *
 * \see G. Jacobson, Space-efficient static trees and graphs, 30th Annual
 *      Symposium on Foundations of Computer Science (FOCS 1989), 549-554,
 *      1989, doi:10.1109/SFCS.1989.63533 (rank by the counts of blocks)
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

#ifndef __ADS_PositionIndex
 #define __ADS_PositionIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PositionIndex ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the positions of objects of type T, by their addresses
template< class T , class H = FastHash< const T * > >
class PositionIndex {

/*--------------------------------------------------------------------------*/
 public:

 using Position = std::uint32_t;     ///< the type of the positions

 /// the position of a key absent
 static constexpr Position NONE = std::numeric_limits< Position >::max();

 /// an empty index with room for n keys
 explicit PositionIndex( std::size_t n = 0 , H h = H() )
  : f_table( n , std::move( h ) ) {}

/*--------------------------------------------------------------------------*/

 /// the position of p, NONE if absent
 [[nodiscard]] Position find( const T * p ) const;

 /// true if p is in the index
 [[nodiscard]] bool contains( const T * p ) const;

 /// gives p the position i; true if p was absent, false if its position
 /// was replaced
 /// \pre p is not nil and i < NONE
 bool insert( const T * p , Position i );

 /// removes p; false if it was absent
 bool erase( const T * p );

 /// the index of the vector v of pointers (any range with v[ i ]): v[ i ]
 /// at the position i, all the other keys gone
 /// \pre no v[ i ] is nil, and v has fewer than NONE items
 template< std::ranges::random_access_range R >
 void assign( const R & v );

 /// the erasure of the positions E from the vector: the keys at a
 /// position in E go away, and each other position p becomes p minus the
 /// number of the positions of E below p
 /// \pre E is strictly increasing
 void renumber( std::span< const Position > E );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_table.size() ); }

 [[nodiscard]] bool empty( void ) const { return( f_table.empty() ); }

 /// the number of places of the table
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_table.capacity() );
  }

 /// room for n keys with no rebuild
 void reserve( std::size_t n ) { f_table.reserve( n ); }

 /// removes all the keys, keeping the places
 void clear( void ) { f_table.clear(); }

 /// f( p , i ) for each key p at the position i, in the order of the
 /// places
 template< class F >
 void forEach( F && f ) const { f_table.forEach( f ); }

 void swap( PositionIndex & o ) noexcept {
  f_table.swap( o.f_table );
  f_words.swap( o.f_words );
  f_drop.swap( o.f_drop );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a word of the positions erased and the count of those before it
 struct Word {
  std::uint64_t bits;
  Position rank;
  };

 FlatHashTable< const T * , Position , H > f_table;   ///< the positions
 std::vector< Word > f_words;      ///< the positions erased
 std::vector< const T * > f_drop;  ///< the keys of the positions erased

 };  // end( class PositionIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PositionIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PositionIndex.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PositionIndex.h ------------------------*/
/*--------------------------------------------------------------------------*/
