/*--------------------------------------------------------------------------*/
/*--------------------------- File RecordFile.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Files of records on a real disk, for the algorithms whose data do not fit
 * in the internal memory and are read and written by sequential scans only
 * (e.g., the searches with delayed duplicate detection, whose open and
 * closed lists are kept on the disk): where `TwoLevelMemory.h` simulates the
 * model of Aggarwal and Vitter to count its I/Os, these files are on the
 * file system of the machine, and what they cost is the time of the disk.
 *
 * - `RecordFile` is a file of records of a trivially copyable type `T`,
 *   written as their bytes (hence readable only on a machine with the same
 *   layout of `T`): `append()` adds records at its end, `forEach()` scans
 *   them in order through a buffer, `readAll()` loads them, `clear()`
 *   empties the file and `remove()` deletes it;
 * - `RecordWriter` appends to a `RecordFile` through a buffer of records,
 *   which it writes when it is full and at `flush()` or at its destruction;
 *   the file is opened only to write a buffer, and closed after it, so that
 *   a program may keep a writer for each of many files (one per bucket of a
 *   search, say) without holding as many descriptors of the system;
 * - `TempDirectory` is a directory of its own, made in the temporary
 *   directory of the system (or in a given one) and removed with all that
 *   it holds when the object is destroyed, where the files of a run go.
 *
 * A scan of \f$ n \f$ records through a buffer of \f$ b \f$ records makes
 * \f$ \lceil n / b \rceil \f$ reads of the system, and the writer as many
 * writes; with buffers of some MB each read and each write moves a large
 * contiguous piece of the file, which is what a disk (or an SSD) does at
 * its best throughput [see `theory_external`]. An error of the file system
 * (a file that cannot be opened, a disk that is full) throws
 * `std::runtime_error` with the name of the file.
 *
 * The contracts [see `Contract.h`]: a buffer of at least one record; a
 * pointer to the records to append unless there are none; the size of a
 * file grows by the records appended. The functions are defined in
 * `RecordFile.tpp`.
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

#ifndef __ADS_RecordFile
 #define __ADS_RecordFile
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RecordFile ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a file of records of a trivially copyable type on the disk

template< class T >
 requires std::is_trivially_copyable_v< T >
class RecordFile {

/*--------------------------------------------------------------------------*/

 public:

 using value_type = T;           ///< the type of the records
 using size_type = std::size_t;  ///< the type of the sizes

 /// the records of a buffer of 1 MB, the default of the scans and of the
 /// writers
 static constexpr std::size_t defaultBuffer =
  sizeof( T ) >= ( std::size_t( 1 ) << 20 ) ? 1
                                           : ( std::size_t( 1 ) << 20 ) /
                                             sizeof( T );

 /// the file at `path`, whose records are those already there, if any
 explicit RecordFile( std::filesystem::path path );

 /// the path of the file
 [[nodiscard]] const std::filesystem::path & path( void ) const;

 /// the number of records of the file, 0 if it does not exist
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the file has no record
 [[nodiscard]] bool empty( void ) const;

 /// the \f$ n \f$ records of `src` appended at the end of the file, which
 /// is made if it does not exist
 /// \pre `src` is not null unless \f$ n = 0 \f$
 /// \post `size()` grows by \f$ n \f$
 void append( const T * src , std::size_t n );

 /// \f$ f( x ) \f$ for each record \f$ x \f$ of the file, in order, read
 /// through a buffer of `buffer` records
 /// \pre `buffer` \f$ \ge 1 \f$
 template< class F >
 void forEach( F && f , std::size_t buffer = defaultBuffer ) const;

 /// all the records of the file, in order
 [[nodiscard]] std::vector< T > readAll( void ) const;

 /// the file emptied, and made if it does not exist
 /// \post `empty()`
 void clear( void );

 /// the file deleted from the disk, if it is there
 /// \post `empty()`
 void remove( void );

/*--------------------------------------------------------------------------*/

 private:

 std::filesystem::path f_path;   ///< where the file is

 };  // end( class RecordFile )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RecordWriter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// appends to a RecordFile through a buffer of records, the file opened
/// only to write the buffer

template< class T >
 requires std::is_trivially_copyable_v< T >
class RecordWriter {

/*--------------------------------------------------------------------------*/

 public:

 /// the writer at the end of `file` (which must outlive it), with a buffer
 /// of `buffer` records
 /// \pre `buffer` \f$ \ge 1 \f$
 explicit RecordWriter( RecordFile< T > & file ,
                        std::size_t buffer = RecordFile< T >::defaultBuffer );

 /// the writer moved, its buffer with it
 RecordWriter( RecordWriter && ) noexcept = default;

 /// no copy, since the records of the buffer would be written twice
 RecordWriter( const RecordWriter & ) = delete;

 /// no copy, since the records of the buffer would be written twice
 RecordWriter & operator=( const RecordWriter & ) = delete;

 /// `flush()`, an error of the file system being dropped: a program that
 /// must see it calls `flush()` before
 ~RecordWriter();

 /// appends \f$ x \f$, writing the buffer when it is full
 void push( const T & x );

 /// the records of the buffer written to the file
 /// \post `pending()` is 0
 void flush( void );

 /// the records in the buffer, not yet in the file
 [[nodiscard]] std::size_t pending( void ) const;

 /// the file written
 [[nodiscard]] RecordFile< T > & file( void ) const;

/*--------------------------------------------------------------------------*/

 private:

 RecordFile< T > * f_file;   ///< the file written
 std::size_t f_cap;          ///< the records of a full buffer
 std::vector< T > f_buf;     ///< the records not written yet

 };  // end( class RecordWriter )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS TempDirectory ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a directory of its own, removed with what it holds at its destruction

class TempDirectory {

/*--------------------------------------------------------------------------*/

 public:

 /// a new directory, of a name not taken, in `parent` (the temporary
 /// directory of the system if empty), its name beginning by `prefix`
 explicit TempDirectory( const std::filesystem::path & parent = {} ,
                         const std::string & prefix = "ads-" );

 /// the directory moved: the moved one removes nothing
 TempDirectory( TempDirectory && other ) noexcept;

 /// no copy, since the directory would be removed twice
 TempDirectory( const TempDirectory & ) = delete;

 /// no copy, since the directory would be removed twice
 TempDirectory & operator=( const TempDirectory & ) = delete;

 /// the directory removed with all that it holds, errors dropped
 ~TempDirectory();

 /// the path of the directory
 [[nodiscard]] const std::filesystem::path & path( void ) const;

 /// the path of the file `name` in the directory
 [[nodiscard]] std::filesystem::path operator/( const std::string & name )
  const;

/*--------------------------------------------------------------------------*/

 private:

 std::filesystem::path f_path;   ///< the directory, empty once moved

 };  // end( class TempDirectory )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/RecordFile.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RecordFile.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RecordFile.h --------------------------*/
/*--------------------------------------------------------------------------*/
