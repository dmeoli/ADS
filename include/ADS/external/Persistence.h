/*--------------------------------------------------------------------------*/
/*--------------------------- File Persistence.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The persistence of the static structures of ADS, beyond the book: a
 * structure built once (a succinct index, a filter, a minimal perfect hash
 * function, a graph in CSR form) is written to a file, a stream or a
 * buffer by `store()`, and read back by `restore()`, or opened in place by
 * `restoreMapped()`, which maps the file [see `MappedFile.h`] and makes
 * the large arrays of the structure views over its bytes, with no copy and
 * no parsing, so that an index of any size is opened in \f$ O( 1 ) \f$
 * time and its pages are read from the disk at their first access.
 *
 * A structure is `Persistent` when it names itself, `persistName` (with a
 * `persistVersion`, 1 if absent, raised when its layout changes, and a
 * `persistTag()` of its parameters, 0 if absent), and lists its members in
 * one member template `persist( A & a )`, called with the archive that
 * writes them and with the one that reads them (`A::loading` tells which):
 * `a( x , y , ... )` writes or reads the members in order, and
 * `a.require( c , what )`, on a reading, rejects a file whose members
 * break an invariant \f$ c \f$ of the structure. The same member serves the
 * archives of cereal [see `interop/cereal/Cereal.h`, included by a program
 * that uses them].
 *
 * The format, version 1, is
 * - a header of 128 bytes, little-endian: the magic number
 *   `\x89 ADS \r \n \x1a \n` (which a transfer as text or a truncation to 7
 *   bits breaks), the version of the format (16 bits), the byte order of
 *   the payload (8 bits, 1 little and 2 big), the version of the structure
 *   (32 bits), its tag (64 bits), and its name (32 bits of length and at
 *   most 100 characters);
 * - the payload, the members in the order of `persist()`: a number (an
 *   integer of any width, a `bool`, an enumeration, a `float` or a
 *   `double`) in 8 bytes; an array of numbers (a `std::vector` of them) as
 *   its size and the bytes of an item in 8 bytes each, then, from the next
 *   multiple of 64 bytes of the file, its items in their own width, and 0s
 *   up to a multiple of 8; an array of other items as its size and the
 *   items; a `std::array`, a `std::pair` and a `std::string` by their
 *   items; a nested structure as its version and its members;
 * - a trailer of 32 bytes, little-endian: the bytes of the payload, the
 *   checksum of the header and of the payload, 8 bytes of 0s, and the magic
 *   number `ADS-END\n`.
 * The payload is in the byte order of the machine that wrote it (or in the
 * one asked of `store()`), and a reader of the other order swaps the bytes
 * of each number; a file mapped on a machine of its order is read in
 * place, one of the other order is copied. Since an array starts on a
 * multiple of 64 bytes of the file, and a mapping on a page, the views of
 * a mapped file are aligned to the lines of the cache, as the words of a
 * `BitVector` are.
 *
 * The checksum is that of the rounds of xxHash64 (Collet,
 * https://github.com/Cyan4973/xxHash) on four lanes
 * of 64 bits, in the order of the words of the file, at about one cycle a
 * word. A `restore()` computes it while it reads, and rejects a file whose
 * checksum differs; `restoreMapped()` checks the magic numbers, the sizes
 * of the header, of the payload and of the trailer (in \f$ O( 1 ) \f$
 * time, which rejects a truncated file) and the invariants that the
 * structures require, but computes the checksum only if asked, since that
 * reads the whole file. A file that is not of ADS, holds another structure
 * or another version of it, is truncated, has a wrong checksum or breaks
 * an invariant throws `PersistError` with a message that says which, and
 * no member is ever read past the end of the payload.
 *
 * The contracts [see `Contract.h`]: none beyond those of the structures;
 * the errors of the files are exceptions, not contracts, since a file is
 * an input of the program. The functions are defined in
 * `Persistence.tpp`.
 *
 * \see S. Gog, T. Beller, A. Moffat, M. Petri, From theory to practice:
 *      plug and play with succinct data structures, Proc. SEA 2014, LNCS
 *      8504, 326-337, 2014, doi:10.1007/978-3-319-07959-2_28
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

#ifndef __ADS_Persistence
 #define __ADS_Persistence
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Portable.h"
#include "ADS/external/MappedFile.h"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the byte order of the payload of a file
enum class ByteOrder : std::uint8_t {
 little = 1 ,  ///< the least significant byte first
 big = 2 ,     ///< the most significant byte first
 /// the order of the machine
 native = std::endian::native == std::endian::big ? 2 : 1
 };

namespace detail
{

/// the archive with which `Persistent` checks that a class lists its
/// members, never called
struct PersistProbe
{
 static constexpr bool loading = false;
 template< class... X >
 void operator()( X &... );
 void require( bool , const char * ) const;
 };

/// a class that lists its members in `persist()`
template< class T >
concept HasPersist = requires( T & x , PersistProbe & a )
{ x.persist( a ); };

/// a class made empty by its default constructor, or by `persistEmpty()`
template< class T >
concept PersistMakeable = std::move_constructible< T > &&
			  ( std::default_initializable< T > || requires {
			   { T::persistEmpty() } -> std::same_as< T >;
			   } );

/// a class that gives its name in a file
template< class T >
concept PersistNamed = requires {
 { T::persistName } -> std::convertible_to< std::string_view >;
 };

/// the version of a persistent class, 1 if it does not give one
template< class T >
[[nodiscard]] constexpr std::uint32_t persistVersion( void )
{
 if constexpr( requires { T::persistVersion; } )
  return( std::uint32_t( T::persistVersion ) );
 else
  return( 1 );
 }

/// the tag of the parameters of a persistent class, 0 if it gives none
template< class T >
[[nodiscard]] constexpr std::uint64_t persistTag( void )
{
 if constexpr( requires { T::persistTag(); } )
  return( std::uint64_t( T::persistTag() ) );
 else
  return( 0 );
 }

/// an allocator that can make a vector over the bytes of a mapped file
/// [see `MappedFile.h`], as `detail::LineAllocator` does
template< class A >
concept AdoptsMapping = requires { typename A::adopts_mapping; } &&
			std::constructible_from< A , MappedFile >;

/// the checksum of a file: the rounds of xxHash64 on four lanes of the
/// words of 64 bits read in little-endian order, of a length that is a
/// multiple of 8 bytes (a tail is padded with 0s)
class PersistChecksum
{
 public:
 /// the bytes \f$ [ p , p + m ) \f$ that follow those already summed
 void update( const std::byte * p , std::size_t m );

 /// the checksum of the bytes summed
 [[nodiscard]] std::uint64_t value( void ) const;

 private:
 static constexpr std::uint64_t P1 = 0x9E3779B185EBCA87ULL ,
				P2 = 0xC2B2AE3D27D4EB4FULL ,
				P3 = 0x165667B19E3779F9ULL ,
				P4 = 0x85EBCA77C2B2AE63ULL;

 void round( std::uint64_t w );

 std::uint64_t f_acc[ 4 ] = { P1 + P2 , P2 , 0 , 0 - P1 };
 unsigned f_lane = 0;                  ///< the lane of the next word
 std::uint64_t f_bytes = 0;            ///< the bytes summed
 std::array< std::byte , 8 > f_tail{};  ///< a word begun
 unsigned f_ntail = 0;                 ///< its bytes
 };

/// the numbers of the files: the magic numbers, the version of the
/// format, and the sizes of the header and of the trailer
struct PersistFormat
{
 static constexpr std::array< unsigned char , 8 > magic = { 0x89 , 'A' ,  'D' ,
							   'S' ,  '\r' , '\n' ,
							   0x1A , '\n' };
 static constexpr std::array< unsigned char , 8 > endMagic = { 'A' , 'D' ,
							       'S' ,
							      '-' , 'E' , 'N' ,
							      'D' , '\n' };
 static constexpr std::uint16_t version = 1;
 static constexpr std::size_t header = 128 , trailer = 32 , name = 100 ,
			      align = 64;
 };

/// the bytes of the \f$ n \f$ numbers at \f$ p \f$ reversed, each in
/// place
template< class E >
void persistSwap( E * p , std::size_t n );

/// an empty \f$ T \f$, to be read: its `persistEmpty()` if it has one,
/// else the one of its default constructor
template< class T >
[[nodiscard]] T persistBlank( void )
{
 if constexpr( requires { T::persistEmpty(); } )
  return( T::persistEmpty() );
 else
  return( T() );
 }

/// the header and the trailer of a file, around the members of a
/// structure [see `store()` and `restore()`]
struct PersistIO;

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- CONCEPT Persistent --------------------------*/
/*--------------------------------------------------------------------------*/

/// a structure that `store()` writes and `restore()` reads: it names
/// itself, lists its members in `persist()`, and is made empty by its
/// default constructor or, if it has none, by its `persistEmpty()`
template< class T >
concept Persistent = detail::PersistMakeable< T > &&
		     detail::PersistNamed< T > && detail::HasPersist< T >;

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PersistWriter ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the archive that writes the members of a structure [see `store()`]
class PersistWriter
{

/*--------------------------------------------------------------------------*/
 public:
 /// a writer, never a reader
 static constexpr bool loading = false;

 /// the writer of a payload in the order \f$ o \f$ to the stream
 /// \f$ s \f$
 PersistWriter( std::ostream & s , ByteOrder o );

 /// the writer of a payload in the order \f$ o \f$ appended to the bytes
 /// \f$ b \f$
 PersistWriter( std::vector< std::byte > & b , ByteOrder o );

 /// the members \f$ x \f$ written in order
 template< class... X >
 void operator()( X &... x ) {
  ( item( x ) , ... );
  }

 /// nothing, since the members written are those of a structure
 void require( bool , const char * ) const noexcept { }

/*--------------------------------------------------------------------------*/
 private:
 friend struct detail::PersistIO;  // which writes the header and trailer

 /// the header of the structure \f$ T \f$
 template< class T >
 void header( void );

 /// the trailer, after the payload
 void trailer( void );

 /// the bytes \f$ [ p , p + m ) \f$, summed unless they are the trailer
 void bytes( const void * p , std::size_t m , bool sum = true );

 /// 0s up to a multiple of \f$ a \f$ bytes of the file
 void pad( std::size_t a );

 /// the word \f$ w \f$, in the order of the payload
 void word( std::uint64_t w );

 /// the \f$ n \f$ numbers at \f$ p \f$, in the order of the payload
 template< class E >
 void numbers( const E * p , std::size_t n );

 /// a member
 template< class T >
 void item( T & x );

 std::ostream * f_s = nullptr;              ///< the stream, or
 std::vector< std::byte > * f_b = nullptr;  ///< the bytes
 ByteOrder f_o;                             ///< the order of the payload
 detail::PersistChecksum f_sum;
 std::uint64_t f_off = 0;  ///< the bytes written of the file

 };  // end( class PersistWriter )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PersistReader ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the archive that reads the members of a structure [see `restore()`]
class PersistReader
{

/*--------------------------------------------------------------------------*/
 public:
 /// a reader, never a writer
 static constexpr bool loading = true;

 /// the reader of the stream \f$ s \f$, named \f$ w \f$ in the errors
 PersistReader( std::istream & s , std::string w );

 /// the reader of the bytes \f$ b \f$, named \f$ w \f$ in the errors, which
 /// make its arrays views over them if they lie in the mapping \f$ m \f$
 PersistReader( std::span< const std::byte > b , std::string w ,
		MappedFile m = MappedFile() );

 /// the members \f$ x \f$ read in order
 template< class... X >
 void operator()( X &... x ) {
  ( item( x ) , ... );
  }

 /// the file rejected, as corrupted in `what`, unless \f$ c \f$
 void require( bool c , const char * what ) const {
  if( ! c ) [[unlikely]]
   fail( std::string( "corrupted: " ) + what );
  }

/*--------------------------------------------------------------------------*/
 private:
 friend struct detail::PersistIO;  // which reads the header and trailer

 /// the error \f$ m \f$ of the source
 [[noreturn]] void fail( const std::string & m ) const;

 /// the header, checked against the structure \f$ T \f$
 template< class T >
 void header( void );

 /// the trailer of the bytes, checked against their size (and their
 /// checksum, if \f$ \mathit{verify} \f$), before the payload
 void trailerFirst( bool verify );

 /// the trailer of the stream, checked against what was read
 void trailerLast( void );

 /// the end of the payload checked against the place read
 void ended( void ) const;

 /// \f$ m \f$ bytes into \f$ p \f$, summed unless they are the trailer
 void bytes( void * p , std::size_t m , bool sum = true );

 /// the bytes past the place up to a multiple of \f$ a \f$ of the file
 void skip( std::size_t a );

 /// a word, in the order of the payload
 [[nodiscard]] std::uint64_t word( void );

 /// the bytes left in the payload, of a buffer (the most, of a stream)
 [[nodiscard]] std::uint64_t left( void ) const;

 /// a member
 template< class T >
 void item( T & x );

 /// an array of numbers
 template< class E , class A >
 void numbers( std::vector< E , A > & v );

 std::istream * f_s = nullptr;      ///< the stream, or
 std::span< const std::byte > f_b;  ///< the bytes
 MappedFile f_map;                  ///< their mapping, if any
 std::string f_where;               ///< the name of the source
 bool f_swap = false;               ///< whether the order is foreign
 bool f_sumOn = true;               ///< whether the checksum is computed
 detail::PersistChecksum f_sum;
 std::uint64_t f_off = 0;                    ///< the bytes read of the file
 std::uint64_t f_end = ~std::uint64_t( 0 );  ///< the end of the payload

 };  // end( class PersistReader )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure \f$ x \f$ written to the stream \f$ s \f$, its payload
/// in the order \f$ o \f$
/// \throws PersistError if the stream fails
template< Persistent T >
void store( const T & x , std::ostream & s , ByteOrder o = ByteOrder::native );

/// the structure \f$ x \f$ written to the file \f$ p \f$, made or
/// replaced, its payload in the order \f$ o \f$
/// \throws PersistError if the file cannot be written
template< Persistent T >
void store( const T & x , const std::filesystem::path & p ,
	    ByteOrder o = ByteOrder::native );

/// the bytes of the structure \f$ x \f$, as in a file, its payload in the
/// order \f$ o \f$
template< Persistent T >
[[nodiscard]] std::vector< std::byte >
store( const T & x , ByteOrder o = ByteOrder::native );

/// the structure read from the stream \f$ s \f$, its checksum checked
/// \throws PersistError if the stream does not hold a valid \f$ T \f$
template< Persistent T >
[[nodiscard]] T restore( std::istream & s );

/// the structure read from the bytes \f$ b \f$, its checksum checked
/// \throws PersistError if the bytes are not a valid \f$ T \f$
template< Persistent T >
[[nodiscard]] T restore( std::span< const std::byte > b );

/// the structure read from the file \f$ p \f$, its checksum checked
/// \throws PersistError if the file does not hold a valid \f$ T \f$
template< Persistent T >
[[nodiscard]] T restore( const std::filesystem::path & p );

/// the structure of the file \f$ p \f$ mapped in memory, its arrays views
/// over the mapping (copies if the file is of the other byte order, or the
/// array is not on the allocator of the views), its checksum checked if
/// \f$ \mathit{verify} \f$; the mapping lives as long as a view of it
/// \throws PersistError if the file does not hold a valid \f$ T \f$
template< Persistent T >
[[nodiscard]] T restoreMapped( const std::filesystem::path & p ,
			       bool verify = false );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/Persistence.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Persistence.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Persistence.h -------------------------*/
/*--------------------------------------------------------------------------*/
