/*--------------------------------------------------------------------------*/
/*------------------------- File TwoLevelMemory.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-level memory model of Aggarwal and Vitter, simulated so that the
 * algorithms of the external memory can be run and their I/Os counted: an
 * internal memory of M items and a disk of unbounded size, read and
 * written by blocks of B items, an I/O moving one block. The time of the
 * computation in memory is free; what is counted is the number of reads
 * and the number of writes.
 *
 * - TwoLevelMemory holds B and M, the two counters of the I/Os and the
 *   occupation of the memory: every buffer and every array that an
 *   algorithm keeps in memory is allocated from it, by a MemoryLease, and
 *   an allocation beyond M violates a precondition, so that a test of an
 *   algorithm checks at once its result, its I/Os and the memory it used
 *   (peak());
 * - DiskFile is a file on the disk, a sequence of items laid out in blocks
 *   of B items, the last one possibly partial; readBlock() and
 *   appendBlock() are the only accesses to its items, one I/O each, and
 *   contents() is the view without I/Os, for the checks of the tests;
 * - DiskReader reads a file through a buffer of one block: next() scans it
 *   in order, ceil( n / B ) reads for n items, and at( i ) reads any item,
 *   loading its block only when it is not the one in the buffer, so that a
 *   sorted set of m positions costs at most \f$ \min\{ m , \lceil n / B
 *   \rceil \} \f$ reads;
 * - DiskWriter appends to a file through a buffer of one block, a write
 *   per full block and one for the last partial block.
 *
 * The bounds of the model are those of the notes: a scan of n items takes
 * \f$ \Theta( n / B ) \f$ I/Os, a sort \f$ \Theta( ( n / B ) \log_{M / B}
 * ( n / M ) ) \f$ [see ExternalSorting.h], and an algorithm that follows
 * pointers on the disk pays up to one I/O per pointer, which is what the
 * external algorithms of this library avoid. The files are not containers
 * with iterators on purpose: an iterator would read the items without
 * paying the I/Os.
 *
 * The contracts [see Contract.h]: \f$ B \geq 1 \f$ and \f$ M \geq 2 B \f$;
 * the memory in use never exceeds M; a block read exists; a block is
 * appended only after full blocks; a file and its reader or writer share
 * the memory. The functions are defined in TwoLevelMemory.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 1 and 5
 * \see A. Aggarwal, J. S. Vitter, The input/output complexity of sorting
 *      and related problems, Communications of the ACM 31(9), 1116-1127,
 *      1988, doi:10.1145/48529.48535
 * \see J. S. Vitter, External memory algorithms and data structures:
 *      dealing with massive data, ACM Computing Surveys 33(2), 209-271,
 *      2001, doi:10.1145/384192.384193
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

/// the parameters B and M of the model, the counters of the I/Os and the
/// occupation of the internal memory
class TwoLevelMemory {

/*--------------------------------------------------------------------------*/
 public:

 /// a memory of M items and blocks of B items
 /// \pre B >= 1 and M >= 2 B
 TwoLevelMemory( std::size_t B , std::size_t M );

 TwoLevelMemory( const TwoLevelMemory & ) = delete;
 TwoLevelMemory & operator=( const TwoLevelMemory & ) = delete;

 /// B, the items of a block
 [[nodiscard]] std::size_t blockSize( void ) const;

 /// M, the items of the internal memory
 [[nodiscard]] std::size_t memorySize( void ) const;

 /// ceil( n / B ), the blocks of n items
 [[nodiscard]] std::size_t blocks( std::size_t n ) const;

 [[nodiscard]] std::uint64_t reads( void ) const;

 [[nodiscard]] std::uint64_t writes( void ) const;

 /// reads() + writes()
 [[nodiscard]] std::uint64_t ios( void ) const;

 /// sets the counters of the I/Os to 0, and the peak to the memory in use
 void resetCounters( void );

 /// takes k items of the internal memory
 /// \pre used() + k <= M
 void allocate( std::size_t k );

 /// gives back k items of the internal memory
 /// \pre k <= used()
 void release( std::size_t k );

 /// the items of the internal memory in use
 [[nodiscard]] std::size_t used( void ) const;

 /// the largest used() since the construction or resetCounters()
 [[nodiscard]] std::size_t peak( void ) const;

 /// one more read, or one more write: called by DiskFile only
 void countRead( void );
 void countWrite( void );

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_B;                ///< the items of a block
 std::size_t f_M;                ///< the items of the internal memory
 std::uint64_t f_reads = 0;
 std::uint64_t f_writes = 0;
 std::size_t f_used = 0;         ///< the items of memory in use
 std::size_t f_peak = 0;         ///< the largest f_used

 };  // end( class TwoLevelMemory )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MemoryLease ----------------------------*/
/*--------------------------------------------------------------------------*/

/// k items of the internal memory, taken on construction and given back on
/// destruction
class MemoryLease {

/*--------------------------------------------------------------------------*/
 public:

 /// \pre mem.used() + k <= M
 MemoryLease( TwoLevelMemory & mem , std::size_t k );

 MemoryLease( const MemoryLease & ) = delete;
 MemoryLease & operator=( const MemoryLease & ) = delete;

 ~MemoryLease();

 [[nodiscard]] std::size_t size( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 TwoLevelMemory * f_mem;
 std::size_t f_k;

 };  // end( class MemoryLease )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DiskFile ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a file of items on the disk of the model, in blocks of B items
template< class T >
class DiskFile {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;
 using size_type = std::size_t;

 /// an empty file
 explicit DiskFile( TwoLevelMemory & mem );

 /// the file of the items, laid out with no I/O: the input of a problem
 DiskFile( TwoLevelMemory & mem , std::vector< T > items );

 DiskFile( DiskFile && ) noexcept = default;

 /// a rename on the disk, no I/O
 /// \pre the two files share the memory
 DiskFile & operator=( DiskFile && other );

 DiskFile( const DiskFile & ) = delete;
 DiskFile & operator=( const DiskFile & ) = delete;

 [[nodiscard]] TwoLevelMemory & memory( void ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the blocks of the file, ceil( size() / B )
 [[nodiscard]] std::size_t blocks( void ) const;

 /// one read: the block b copied to dest, which has room for B items; the
 /// number of its items
 /// \pre b < blocks()
 std::size_t readBlock( std::size_t b , T * dest ) const;

 /// one write: the n items of src appended as a new block
 /// \pre 1 <= n <= B and size() a multiple of B
 void appendBlock( const T * src , std::size_t n );

 /// empties the file, no I/O
 void clear( void );

 /// the items, no I/O: for the checks, not for the algorithms
 [[nodiscard]] const std::vector< T > & contents( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 TwoLevelMemory * f_mem;
 std::vector< T > f_items;

 };  // end( class DiskFile )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskReader -----------------------------*/
/*--------------------------------------------------------------------------*/

/// reads a file through a buffer of one block, B items of the memory
template< class T >
class DiskReader {

/*--------------------------------------------------------------------------*/
 public:

 /// the buffer is taken from the memory of f
 /// \pre B items of memory are free
 explicit DiskReader( const DiskFile< T > & f );

 DiskReader( const DiskReader & ) = delete;
 DiskReader & operator=( const DiskReader & ) = delete;

 /// the items not yet scanned
 [[nodiscard]] bool hasNext( void ) const;

 /// the next item of the scan, its block read if not in the buffer
 /// \pre hasNext()
 const T & peek( void );

 /// the next item of the scan, and the scan moves past it
 /// \pre hasNext()
 T next( void );

 /// the place of the scan
 [[nodiscard]] std::size_t position( void ) const;

 /// the item i, its block read if not in the buffer; the scan goes on
 /// from i + 1
 /// \pre i < the size of the file
 const T & at( std::size_t i );

/*--------------------------------------------------------------------------*/
 private:

 void load( std::size_t b );

 const DiskFile< T > * f_file;
 MemoryLease f_lease;
 std::vector< T > f_buf;          ///< the block in memory
 std::size_t f_block;             ///< its number, or -1
 std::size_t f_pos = 0;           ///< the next item of the scan

 };  // end( class DiskReader )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskWriter -----------------------------*/
/*--------------------------------------------------------------------------*/

/// appends to a file through a buffer of one block, B items of the memory;
/// the last partial block is written by close() or by the destructor
template< class T >
class DiskWriter {

/*--------------------------------------------------------------------------*/
 public:

 /// \pre B items of memory are free, and the size of f a multiple of B
 explicit DiskWriter( DiskFile< T > & f );

 DiskWriter( const DiskWriter & ) = delete;
 DiskWriter & operator=( const DiskWriter & ) = delete;

 ~DiskWriter();

 /// appends x, writing the buffer when it is full
 /// \pre the writer is not closed
 void push( const T & x );

 /// writes the partial block, if any; no push() after it
 void close( void );

/*--------------------------------------------------------------------------*/
 private:

 DiskFile< T > * f_file;
 MemoryLease f_lease;
 std::vector< T > f_buf;
 bool f_closed = false;

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
