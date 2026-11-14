/*--------------------------------------------------------------------------*/
/*---------------------------- File BlockStore.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The store of blocks of section 14.1 of Morin's book, beyond the book: a
 * disk simulated in memory, an array of blocks of type `T` addressed by
 * their index, on which `readBlock( i )` and `writeBlock( i , b )` are the
 * I/Os of the external memory model [see `TwoLevelMemory.h`], counted one
 * each; `placeBlock( b )` writes a new block where a block freed by
 * `freeBlock( i )` was, or at the end, so that the indices of the blocks in
 * use stay below their largest number. A block is copied when it is read
 * and when it is written, as it would be between the disk and the memory,
 * so that what a structure keeps of a block between two I/Os is its own
 * copy, which a test of the internal memory can count. If a
 * `TwoLevelMemory` is given, its counters count the I/Os too, so that the
 * structures on the blocks and the algorithms on the files of `DiskFile`
 * share one count.
 *
 * Against the code of Morin's book: the blocks are values, where the book
 * keeps pointers to the nodes, so that a node changed after its read is
 * changed on the disk with no write, which hides the I/Os that a test of
 * the model would count; the I/Os are counted; and `placeBlock()` writes
 * the block also in a free place, where the code of the book takes the
 * free index by `remove( size() )`, one past the last of its stack, and
 * does not store the block there. `peek()` reads a block with no I/O, for
 * the checks of the contracts only.
 *
 * The contracts [see `Contract.h`]: a block read, written, freed or peeked
 * is in use. The operations are defined in `BlockStore.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, section 14.1
 * \see A. Aggarwal, J. S. Vitter, The input/output complexity of sorting
 *      and related problems, Communications of the ACM 31(9), 1116-1127,
 *      1988, doi:10.1145/48529.48535
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

#ifndef __ADS_BlockStore
 #define __ADS_BlockStore
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/TwoLevelMemory.h"

#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BlockStore -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a disk of blocks of type `T`, read and written one at a time
template< class T >
class BlockStore {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty store, whose I/Os are also counted by `mem`, if given
 explicit BlockStore( TwoLevelMemory * mem = nullptr ) : f_mem( mem ) {}

 /// a copy of the block \f$ i \f$, one read
 /// \pre the block \f$ i \f$ is in use
 [[nodiscard]] T readBlock( std::size_t i ) const;

 /// \f$ b \f$ written in the block \f$ i \f$, one write
 /// \pre the block \f$ i \f$ is in use
 void writeBlock( std::size_t i , const T & b );

 /// \f$ b \f$ written in a free block, one write; its index
 std::size_t placeBlock( const T & b );

 /// the block \f$ i \f$ given back
 /// \pre the block \f$ i \f$ is in use
 void freeBlock( std::size_t i );

 /// the block \f$ i \f$, with no I/O, for the checks
 /// \pre the block \f$ i \f$ is in use
 [[nodiscard]] const T & peek( std::size_t i ) const;

 /// the number of blocks in use
 [[nodiscard]] std::size_t blocks( void ) const {
  return( f_B.size() - f_free.size() );
  }

 /// the reads so far
 [[nodiscard]] std::uint64_t reads( void ) const { return( f_reads ); }

 /// the writes so far
 [[nodiscard]] std::uint64_t writes( void ) const { return( f_writes ); }

 /// the counters back to 0
 void resetCounters( void ) { f_reads = f_writes = 0; }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< T > f_B;                 ///< the blocks
 std::vector< bool > f_used;           ///< which ones are in use
 std::vector< std::size_t > f_free;    ///< the free ones, a stack
 TwoLevelMemory * f_mem;               ///< the model that counts too
 mutable std::uint64_t f_reads = 0;    ///< the reads
 std::uint64_t f_writes = 0;           ///< the writes

 };  // end( class BlockStore )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/BlockStore.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BlockStore.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BlockStore.h --------------------------*/
/*--------------------------------------------------------------------------*/
