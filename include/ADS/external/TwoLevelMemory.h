/*--------------------------------------------------------------------------*/
/*------------------------- File TwoLevelMemory.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-level memory model of Aggarwal and Vitter, simulated so that the
 * algorithms of the external memory can be run and their I/Os counted: an
 * internal memory of \f$ M \f$ items and a disk of unbounded size, read and
 * written by blocks of \f$ B \f$ items, an I/O moving one block. The time of
 * the computation in memory is free; what is counted is the number of reads
 * and the number of writes.
 *
 * - `TwoLevelMemory` holds \f$ B \f$ and \f$ M \f$, the two counters of the
 *   I/Os and the occupation of the memory: every buffer and every array that
 *   an algorithm keeps in memory is allocated from it, by a `MemoryLease`,
 *   and an allocation beyond \f$ M \f$ violates a precondition, so that a
 *   test of an algorithm checks at once its result, its I/Os and the memory
 *   it used (`peak()`);
 * - `DiskFile` is a file on the disk, a sequence of items laid out in blocks
 *   of \f$ B \f$ items, the last one possibly partial; `readBlock()` and
 *   `appendBlock()` are the only accesses to its items, one I/O each, and
 *   `contents()` is the view without I/Os, for the checks of the tests;
 * - `DiskReader` reads a file through a buffer of one block: `next()` scans
 *   it in order, \f$ \lceil n / B \rceil \f$ reads for \f$ n \f$ items, and
 *   `at( i )` reads any item, loading its block only when it is not the one
 *   in the buffer, so that a sorted set of \f$ m \f$ positions costs at most
 *   \f$ \min\{ m , \lceil n / B \rceil \} \f$ reads;
 * - `DiskWriter` appends to a file through a buffer of one block, a write
 *   per full block and one for the last partial block.
 *
 * The bounds of the model are those of Ferragina's book (chapter 1,
 * Introduction, and chapter 5, Sorting Atomic Items): a scan of \f$ n \f$
 * items takes \f$ \Theta( n / B ) \f$ I/Os, a sort
 * \f$ \Theta( ( n / B ) \log_{M / B} ( n / M ) ) \f$ [see
 * `ExternalSorting.h`], and an algorithm that follows pointers on the disk
 * pays up to one I/O per pointer, which is what the external algorithms of
 * this library avoid. The files are not containers with iterators on
 * purpose: an iterator would read the items without paying the I/Os.
 *
 * The contracts [see `Contract.h`]: \f$ B \ge 1 \f$ and \f$ M \ge 2 B \f$;
 * the memory in use never exceeds \f$ M \f$; a block read exists; a block is
 * appended only after full blocks; a file and its reader or writer share
 * the memory. The functions are defined in `TwoLevelMemory.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 1, Introduction, and 5, Sorting Atomic Items,
 *      doi:10.1017/9781009128933
 * \see A. Aggarwal, J. S. Vitter, The input/output complexity of sorting
 *      and related problems, Communications of the ACM 31(9), 1116-1127,
 *      1988, doi:10.1145/48529.48535
 * \see J. S. Vitter, External memory algorithms and data structures:
 *      dealing with massive data, ACM Computing Surveys 33(2), 209-271,
 *      2001, doi:10.1145/384192.384193
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

#ifndef __ADS_TwoLevelMemory
 #define __ADS_TwoLevelMemory
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS TwoLevelMemory ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the parameters \f$ B \f$ and \f$ M \f$ of the model, the counters of the
/// I/Os and the occupation of the internal memory
class TwoLevelMemory {

/*--------------------------------------------------------------------------*/
 public:

 /// a memory of \f$ M \f$ items and blocks of \f$ B \f$ items
 /// \pre \f$ B \ge 1 \f$ and \f$ M \ge 2 B \f$
 TwoLevelMemory( std::size_t B , std::size_t M );

 /// no copy, since the counters belong to one simulation
 TwoLevelMemory( const TwoLevelMemory & ) = delete;
 /// no copy, since the counters belong to one simulation
 TwoLevelMemory & operator=( const TwoLevelMemory & ) = delete;

 /// \f$ B \f$, the items of a block
 [[nodiscard]] std::size_t blockSize( void ) const;

 /// \f$ M \f$, the items of the internal memory
 [[nodiscard]] std::size_t memorySize( void ) const;

 /// \f$ \lceil n / B \rceil \f$, the blocks of \f$ n \f$ items
 [[nodiscard]] std::size_t blocks( std::size_t n ) const;

 /// the reads counted so far
 [[nodiscard]] std::uint64_t reads( void ) const;

 /// the writes counted so far
 [[nodiscard]] std::uint64_t writes( void ) const;

 /// the I/Os counted so far, `reads()` plus `writes()`
 [[nodiscard]] std::uint64_t ios( void ) const;

 /// sets the counters of the I/Os to 0, and the peak to the memory in use
 void resetCounters( void );

 /// takes \f$ k \f$ items of the internal memory
 /// \pre `used()` plus \f$ k \f$ is at most \f$ M \f$
 void allocate( std::size_t k );

 /// gives back \f$ k \f$ items of the internal memory
 /// \pre \f$ k \f$ is at most `used()`
 void release( std::size_t k );

 /// the items of the internal memory in use
 [[nodiscard]] std::size_t used( void ) const;

 /// the largest `used()` since the construction or `resetCounters()`
 [[nodiscard]] std::size_t peak( void ) const;

 /// one more read: called by `DiskFile` only
 void countRead( void );

 /// one more write: called by `DiskFile` only
 void countWrite( void );

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_B;                ///< the items of a block
 std::size_t f_M;                ///< the items of the internal memory
 std::uint64_t f_reads = 0;    ///< the reads counted
 std::uint64_t f_writes = 0;   ///< the writes counted
 std::size_t f_used = 0;         ///< the items of memory in use
 std::size_t f_peak = 0;         ///< the largest `f_used`

 };  // end( class TwoLevelMemory )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MemoryLease ----------------------------*/
/*--------------------------------------------------------------------------*/

/// \f$ k \f$ items of the internal memory, taken on construction and given
/// back on destruction
class MemoryLease {

/*--------------------------------------------------------------------------*/
 public:

 /// the lease of \f$ k \f$ items of `mem`
 /// \pre `mem.used()` plus \f$ k \f$ is at most \f$ M \f$
 MemoryLease( TwoLevelMemory & mem , std::size_t k );

 /// no copy, since the items would be given back twice
 MemoryLease( const MemoryLease & ) = delete;
 /// no copy, since the items would be given back twice
 MemoryLease & operator=( const MemoryLease & ) = delete;

 /// gives the items back
 ~MemoryLease();

 /// \f$ k \f$, the items taken
 [[nodiscard]] std::size_t size( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 TwoLevelMemory * f_mem;   ///< the memory the items come from
 std::size_t f_k;          ///< the items taken

 };  // end( class MemoryLease )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DiskFile ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a file of items on the disk of the model, in blocks of \f$ B \f$ items
template< class T >
class DiskFile {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;          ///< the type of the items
 using size_type = std::size_t;  ///< the type of the sizes

 /// an empty file
 explicit DiskFile( TwoLevelMemory & mem );

 /// the file of the items, laid out with no I/O: the input of a problem
 DiskFile( TwoLevelMemory & mem , std::vector< T > items );

 /// the file moved, no I/O
 DiskFile( DiskFile && ) noexcept = default;

 /// a rename on the disk, no I/O
 /// \pre the two files share the memory
 DiskFile & operator=( DiskFile && other );

 /// no copy, which would duplicate the file without paying its I/Os
 DiskFile( const DiskFile & ) = delete;
 /// no copy, which would duplicate the file without paying its I/Os
 DiskFile & operator=( const DiskFile & ) = delete;

 /// the memory of the model that the file is on
 [[nodiscard]] TwoLevelMemory & memory( void ) const;

 /// the number of items of the file
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the file has no item
 [[nodiscard]] bool empty( void ) const;

 /// the blocks of the file, \f$ \lceil \mathrm{size}() / B \rceil \f$
 [[nodiscard]] std::size_t blocks( void ) const;

 /// one read: the block \f$ b \f$ copied to `dest`, which has room for
 /// \f$ B \f$ items; the number of its items
 /// \pre \f$ b \f$ is below `blocks()`
 std::size_t readBlock( std::size_t b , T * dest ) const;

 /// one write: the \f$ n \f$ items of `src` appended as a new block
 /// \pre \f$ 1 \le n \le B \f$, and `size()` a multiple of \f$ B \f$
 void appendBlock( const T * src , std::size_t n );

 /// empties the file, no I/O
 void clear( void );

 /// the items, no I/O: for the checks, not for the algorithms
 [[nodiscard]] const std::vector< T > & contents( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 TwoLevelMemory * f_mem;    ///< the memory of the model
 std::vector< T > f_items;  ///< the items, in the order of the file

 };  // end( class DiskFile )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskReader -----------------------------*/
/*--------------------------------------------------------------------------*/

/// reads a file through a buffer of one block, \f$ B \f$ items of the
/// memory
template< class T >
class DiskReader {

/*--------------------------------------------------------------------------*/
 public:

 /// the reader of `f`, its buffer taken from the memory of `f`
 /// \pre \f$ B \f$ items of memory are free
 explicit DiskReader( const DiskFile< T > & f );

 /// no copy, since the buffer is leased from the memory
 DiskReader( const DiskReader & ) = delete;
 /// no copy, since the buffer is leased from the memory
 DiskReader & operator=( const DiskReader & ) = delete;

 /// whether some items are not yet scanned
 [[nodiscard]] bool hasNext( void ) const;

 /// the next item of the scan, its block read if not in the buffer
 /// \pre `hasNext()`
 const T & peek( void );

 /// the next item of the scan, and the scan moves past it
 /// \pre `hasNext()`
 T next( void );

 /// the place of the scan
 [[nodiscard]] std::size_t position( void ) const;

 /// the item \f$ i \f$, its block read if not in the buffer; the scan goes
 /// on from \f$ i + 1 \f$
 /// \pre \f$ i \f$ is below the size of the file
 const T & at( std::size_t i );

/*--------------------------------------------------------------------------*/
 private:

 /// the block \f$ b \f$ into the buffer, one read unless it is there
 void load( std::size_t b );

 const DiskFile< T > * f_file;    ///< the file read
 MemoryLease f_lease;             ///< the memory of the buffer
 std::vector< T > f_buf;          ///< the block in memory
 std::size_t f_block;             ///< its number, or \f$ -1 \f$
 std::size_t f_pos = 0;           ///< the next item of the scan

 };  // end( class DiskReader )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskWriter -----------------------------*/
/*--------------------------------------------------------------------------*/

/// appends to a file through a buffer of one block, \f$ B \f$ items of the
/// memory; the last partial block is written by `close()` or by the
/// destructor
template< class T >
class DiskWriter {

/*--------------------------------------------------------------------------*/
 public:

 /// the writer at the end of `f`, its buffer taken from the memory of `f`
 /// \pre \f$ B \f$ items of memory are free, and the size of `f` a multiple
 ///      of \f$ B \f$
 explicit DiskWriter( DiskFile< T > & f );

 /// no copy, since the buffer is leased from the memory
 DiskWriter( const DiskWriter & ) = delete;
 /// no copy, since the buffer is leased from the memory
 DiskWriter & operator=( const DiskWriter & ) = delete;

 /// `close()`, if not closed yet
 ~DiskWriter();

 /// appends \f$ x \f$, writing the buffer when it is full
 /// \pre the writer is not closed
 void push( const T & x );

 /// writes the partial block, if any; no `push()` after it
 void close( void );

/*--------------------------------------------------------------------------*/
 private:

 DiskFile< T > * f_file;   ///< the file written
 MemoryLease f_lease;      ///< the memory of the buffer
 std::vector< T > f_buf;   ///< the items not written yet
 bool f_closed = false;    ///< whether `close()` was called

 };  // end( class DiskWriter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/TwoLevelMemory.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TwoLevelMemory.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File TwoLevelMemory.h ------------------------*/
/*--------------------------------------------------------------------------*/
