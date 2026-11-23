/*--------------------------------------------------------------------------*/
/*------------------------- File PmaCompetitors.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The packed memory arrays of their authors, the competitors of
 * PackedMemoryArray and CompressedPackedMemoryArray in bench/ordered.cpp
 * and bench/joins.cpp: the CPMA of wheatman/Packed-Memory-Array with the
 * heads in the order of Eytzinger, uncompressed (spmae_settings) and
 * compressed (scpmae_settings), on keys of 64 bits, compiled in the unit
 * pma_authors.cpp from the sources in the directory of CPMA, since it
 * includes its own ParlayLib, which would clash with the one of PAM. Its
 * batches run on the workers of ParlayLib (PARLAY_NUM_THREADS). The unit
 * defines the functions below, which the benchmarks call; the build defines
 * ADS_HAS_CPMA where it compiles it. The key 0 is kept apart by that code,
 * which the benchmarks never draw.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#ifndef __ADS_PmaCompetitors
 #define __ADS_PmaCompetitors

#include <cstddef>
#include <cstdint>

namespace pma_authors
{

/// an empty set, compressed or not
void * make( bool compressed );
/// the set of the sorted distinct keys \f$ x \f$
void * build( bool compressed , std::uint64_t * x , std::size_t n );
void destroy( void * s , bool compressed );
bool insert( void * s , bool compressed , std::uint64_t k );
bool remove( void * s , bool compressed , std::uint64_t k );
bool has( const void * s , bool compressed , std::uint64_t k );
/// the inserted (or removed) keys of the batch \f$ x \f$, which it sorts
std::uint64_t insertBatch( void * s , bool compressed , std::uint64_t * x ,
			   std::size_t n , bool sorted );
std::uint64_t removeBatch( void * s , bool compressed , std::uint64_t * x ,
			   std::size_t n , bool sorted );
/// the sum of the \f$ c \f$ keys from the least \f$ \ge k \f$, and the
/// least such key alone if \f$ c = 1 \f$
std::uint64_t scan( const void * s , bool compressed , std::uint64_t k ,
		    std::size_t c );
/// the sum of the keys plus their number
std::uint64_t sum( const void * s , bool compressed );
std::size_t bytes( const void * s , bool compressed );

}  // end( namespace pma_authors )

#endif /* PmaCompetitors.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PmaCompetitors.h ------------------------*/
/*--------------------------------------------------------------------------*/
