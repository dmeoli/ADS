/*--------------------------------------------------------------------------*/
/*------------------------------ File Arrow.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sequences of ADS as the columns of Apache Arrow, by its C Data
 * Interface (https://arrow.apache.org/docs/format/CDataInterface.html):
 * two structures of C, `ArrowSchema` (the type of a column) and
 * `ArrowArray` (its buffers), each with a callback `release` that frees
 * what its producer keeps alive, through which pyarrow, Polars, DuckDB,
 * pandas and nanoarrow hand columns to one another within a process with
 * no copy. The specification froze them as an ABI and asks a producer
 * to copy their definition rather than to depend on Arrow, which this
 * header does under the guard `ARROW_C_DATA_INTERFACE` that the
 * specification prescribes, so that a unit that also includes Arrow or
 * nanoarrow sees them once; hence this header needs no library and no
 * option.
 *
 * `exportArrow( x , &array , &schema )` fills the two structures with a
 * column of no nulls, whose buffer of values is the memory of \f$ x \f$
 * itself, kept alive by a `std::shared_ptr` in the private data of the
 * array until its consumer calls `release`: a `BitVector` as a column of
 * booleans (format `b`), since Arrow numbers the bit \f$ i \f$ as the bit
 * \f$ i \bmod 8 \f$ of the byte \f$ \lfloor i / 8 \rfloor \f$, which is the
 * layout of the words of `BitVector` on a little-endian processor; a
 * `PackedArray< B >` of fields of 8, 16 or 32 bits as a column of unsigned
 * integers of that width (`C`, `S`, `I`), and of 1 bit as one of booleans;
 * a contiguous sequence of numbers (`std::vector`, `std::array` or any
 * contiguous range of integers of 8 to 64 bits, `float` or `double`) as a
 * column of their type (`c`, `C`, `s`, `S`, `i`, `I`, `l`, `L`, `f`, `g`);
 * a view, such as a `std::span`, is kept alive as a view, so that the
 * memory it shows must outlive the array.
 * The values are copied only where the layouts differ: a `PackedArray` of
 * 2 or 4 bits, decoded into bytes, a sequence that is not contiguous (a
 * `TieredVector`, a `std::deque`), and, on a big-endian processor, the
 * words of a `BitVector` and of a `PackedArray`, whose bytes come in the
 * reverse order there (the C Data Interface keeps the numbers in the
 * order of the processor, and the bits of a boolean column always from
 * the least significant one of a byte).
 *
 * The converse takes a column of Arrow, which is in the memory of its
 * producer: `ArrowColumn< T >` takes the array without copy (as the
 * specification allows, by moving the structure and marking the source
 * released) and shows its values as a `std::span< const T >`, the input
 * of a build of ADS (a wavelet matrix, an Elias-Fano sequence), releasing
 * it when destroyed; `toBitVector()`, `toPackedArray< B >()` and
 * `toVector< T >()` copy a column into a structure of ADS, which owns its
 * words. A column with nulls, of another type, or with children is
 * refused with `std::invalid_argument`; the schema is only read, and its
 * release is left to its owner.
 *
 * This header is of the set interop: no entry point of ADS includes it.
 *
 * \see Apache Arrow, The Arrow C data interface,
 *      https://arrow.apache.org/docs/format/CDataInterface.html
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

#ifndef __ADS_Arrow
 #define __ADS_Arrow
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/parallel/Executor.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/PackedArray.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------ THE ARROW C DATA INTERFACE ----------------------*/
/*--------------------------------------------------------------------------*/

// the two structures as the specification gives them, under its guard;
// the macros ARROW_FLAG_* are left to Arrow and nanoarrow, the latter
// taking the first of them as the guard of its own definitions

#ifndef ARROW_C_DATA_INTERFACE
 #define ARROW_C_DATA_INTERFACE

extern "C" {

/// the type of a column of Arrow
struct ArrowSchema
{
 const char * format;              ///< the type, as a string
 const char * name;                ///< the name of the field, or null
 const char * metadata;            ///< the metadata, or null
 std::int64_t flags;               ///< the flags of the field
 std::int64_t n_children;          ///< the types of the children
 struct ArrowSchema ** children;   ///< the types of the children
 struct ArrowSchema * dictionary;  ///< the type of the dictionary, or null
 void ( *release )( struct ArrowSchema * );  ///< null once released
 void * private_data;                        ///< of the producer
 };

/// the buffers of a column of Arrow
struct ArrowArray
{
 std::int64_t length;             ///< the items
 std::int64_t null_count;         ///< the nulls, -1 if not known
 std::int64_t offset;             ///< the first item in the buffers
 std::int64_t n_buffers;          ///< the buffers
 std::int64_t n_children;         ///< the children
 const void ** buffers;           ///< the buffers, the validity first
 struct ArrowArray ** children;   ///< the children
 struct ArrowArray * dictionary;  ///< the dictionary, or null
 void ( *release )( struct ArrowArray * );  ///< null once released
 void * private_data;                       ///< of the producer
 };

 }  // end( extern "C" )

#endif /* ARROW_C_DATA_INTERFACE */

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- THE TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the numbers that are the values of a column of Arrow: the integers of
/// 8, 16, 32 and 64 bits, `float` and `double`
template< class T >
concept ArrowNumber =
  ( std::is_integral_v< T > && ! std::is_same_v< T , bool > &&
    ( sizeof( T ) == 1 || sizeof( T ) == 2 || sizeof( T ) == 4 ||
      sizeof( T ) == 8 ) ) ||
  std::is_same_v< T , float > || std::is_same_v< T , double >;

/// the format of Arrow of the numbers of type \f$ T \f$
template< ArrowNumber T >
[[nodiscard]] constexpr const char * arrowFormat( void )
{
 if constexpr( std::is_same_v< T , float > )
  return( "f" );
 else if constexpr( std::is_same_v< T , double > )
  return( "g" );
 else if constexpr( std::is_signed_v< T > )
  return( sizeof( T ) == 1   ? "c"
	   : sizeof( T ) == 2 ? "s"
	   : sizeof( T ) == 4 ? "i"
			      : "l" );
 else
  return( sizeof( T ) == 1   ? "C"
	   : sizeof( T ) == 2 ? "S"
	   : sizeof( T ) == 4 ? "I"
			      : "L" );
 }

namespace detail
{

/// whether \f$ X \f$ is a `PackedArray`
template< class X >
struct IsPackedArray : std::false_type
{ };

template< unsigned B , bool S >
struct IsPackedArray< PackedArray< B , S > > : std::true_type
{ };

/// the sequences of numbers, which are exported as columns of their type
template< class X >
concept ArrowSequence = std::ranges::sized_range< const X > &&
			ArrowNumber< std::ranges::range_value_t< const X > >;

}  // end( namespace detail )

/// what `exportArrow()` exports: a `BitVector`, a `PackedArray` or a
/// sized sequence of numbers
template< class X >
concept ArrowExportable =
  std::is_same_v< X , BitVector > || detail::IsPackedArray< X >::value ||
  detail::ArrowSequence< X >;

namespace detail
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ArrowKeep ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the private data of an array exported by ADS: the structure it shows,
/// the values copied where the layouts differ, and its two buffers
struct ArrowKeep
{
 std::shared_ptr< const void > owner;               ///< the structure exported
 std::vector< std::uint64_t > copy;                 ///< the values, if copied
 const void * buffers[ 2 ] = { nullptr , nullptr };  ///< no validity
 };

/// a word of zeros, the buffer of an empty column
inline constexpr std::uint64_t kArrowEmpty = 0;

/// the callback `release` of an array exported by ADS
inline void arrowReleaseArray( ArrowArray * a ) noexcept
{
 delete static_cast< ArrowKeep * >( a->private_data );
 a->private_data = nullptr;
 a->release = nullptr;
 }

/// the callback `release` of a schema exported by ADS, whose private data
/// is its name
inline void arrowReleaseSchema( ArrowSchema * s ) noexcept
{
 delete static_cast< std::string * >( s->private_data );
 s->private_data = nullptr;
 s->release = nullptr;
 }

/// the \f$ n \f$ values of type \f$ U \f$ copied into the words of `k`, the
/// value \f$ i \f$ given by \f$ f( i ) \f$
template< class U , class F >
inline void arrowCopy( ArrowKeep & k , std::size_t n , F && f )
{
 k.copy.assign( ( n * sizeof( U ) + 7 ) / 8 , 0 );
 auto * p = reinterpret_cast< unsigned char * >( k.copy.data() );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const U v = U( f( i ) );
  std::memcpy( p + i * sizeof( U ) , &v , sizeof( U ) );
  }
 }

/// the words \f$ w \f$ copied into those of `k` in the order of the bytes
/// of a little-endian processor
inline void arrowLittleWords( ArrowKeep & k , std::span< const Word > w )
{
 k.copy.assign( w.begin() , w.end() );
 if constexpr( std::endian::native != std::endian::little )
  for( auto & x : k.copy )
   x = byteSwap( x );
 }

/// whether the column \f$ a \f$ has a null
[[nodiscard]] inline bool arrowHasNulls( const ArrowArray & a )
{
 if( ( a.null_count == 0 ) || ( a.n_buffers < 1 ) ||
     ( a.buffers[ 0 ] == nullptr ) )
  return( false );
 if( a.null_count > 0 )
  return( true );
 const auto * v = static_cast< const unsigned char * >( a.buffers[ 0 ] );
 for( std::int64_t i = a.offset ; i < a.offset + a.length ; ++i )
  if( ! ( ( v[ i / 8 ] >> ( i % 8 ) ) & 1 ) )  // not known: counted
   return( true );
 return( false );
 }

/// the column \f$ a \f$ of type `s`, checked: not released, of two buffers
/// and of the format \f$ f \f$ (any of the integers if \f$ f \f$ is null),
/// with no child and no null
inline void arrowCheck( const ArrowArray & a , const ArrowSchema & s ,
			const char * f , const char * who )
{
 const auto fail = [ & ]( const char * what ) {
  throw( std::invalid_argument( std::string( "ADS::" ) + who + ": " + what ) );
  };
 if( ( a.release == nullptr ) || ( s.release == nullptr ) )
  fail( "a column of Arrow already released" );
 const std::string_view g = s.format ? s.format : "";
 if( f ? ( g != f )
       : ( g.size() != 1 || std::string_view( "cCsSiIlL" ).find( g[ 0 ] ) ==
			      std::string_view::npos ) )
  fail( "a column of Arrow of another type" );
 if( ( a.n_buffers != 2 ) || ( a.n_children != 0 ) || ( s.n_children != 0 ) ||
     ( a.length < 0 ) || ( a.offset < 0 ) ||
     ( a.length > 0 && a.buffers[ 1 ] == nullptr ) )
  fail( "a column of Arrow with no buffer of values, or with children" );
 if( arrowHasNulls( a ) )
  fail( "a column of Arrow with nulls" );
 }

/// `use( i , x )` on each value \f$ x \f$ of the column of integers
/// \f$ a \f$ of the format \f$ f \f$, in its own type, \f$ i \f$ its
/// place from the offset
template< class F >
inline void arrowIntegers( const ArrowArray & a , char f , F && use )
{
 const void * p = a.buffers[ 1 ];
 const std::size_t o = std::size_t( a.offset ) , n = std::size_t( a.length );
 auto each = [ & ]< class U >( const U * v ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   use( i , v[ o + i ] );
  };
 switch( f ) {
  case 'c':
   each( static_cast< const std::int8_t * >( p ) );
   break;
  case 'C':
   each( static_cast< const std::uint8_t * >( p ) );
   break;
  case 's':
   each( static_cast< const std::int16_t * >( p ) );
   break;
  case 'S':
   each( static_cast< const std::uint16_t * >( p ) );
   break;
  case 'i':
   each( static_cast< const std::int32_t * >( p ) );
   break;
  case 'I':
   each( static_cast< const std::uint32_t * >( p ) );
   break;
  case 'l':
   each( static_cast< const std::int64_t * >( p ) );
   break;
  default:
   each( static_cast< const std::uint64_t * >( p ) );
   break;
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EXPORT -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure \f$ *x \f$ as a column of Arrow of no nulls, named
/// \f$ \mathit{name} \f$, in \f$ *a \f$ and \f$ *s \f$, which its consumer
/// releases; the values are those of \f$ *x \f$, which the array keeps
/// alive (copied where the layouts differ, see above)
/// \pre \f$ a \f$ and \f$ s \f$ not null, \f$ *x \f$ not changed until the
///      array is released
template< class X >
 requires ArrowExportable< std::remove_const_t< X > >
void exportArrow( std::shared_ptr< X > x , ArrowArray * a , ArrowSchema * s ,
		  std::string_view name = "" )
{
 using Y = std::remove_const_t< X >;
 ADS_PRE( ( x != nullptr ) && ( a != nullptr ) && ( s != nullptr ) );
 constexpr bool little = std::endian::native == std::endian::little;
 auto k = std::make_unique< detail::ArrowKeep >();
 const void * data = nullptr;
 const char * format = nullptr;
 const std::size_t n = std::ranges::size( *x );
 if constexpr( std::is_same_v< Y , BitVector > ) {
  format = "b";
  if constexpr( little )
   data = x->words().data();
  else {
   detail::arrowLittleWords( *k , x->words() );
   data = k->copy.data();
   }
  }
 else if constexpr( detail::IsPackedArray< Y >::value ) {
  constexpr unsigned B = 64 / Y::PER;
  if constexpr( B == 1 )
   format = "b";
  else if constexpr( B <= 8 )
   format = "C";
  else if constexpr( B == 16 )
   format = "S";
  else
   format = "I";
  if constexpr( ( B == 1 || B >= 8 ) && little )
   data = x->words().data();
  else if constexpr( B == 1 ) {
   detail::arrowLittleWords( *k , x->words() );
   data = k->copy.data();
   }
  else {
   using U = std::conditional_t<
     ( B <= 8 ) , std::uint8_t ,
     std::conditional_t< ( B == 16 ) , std::uint16_t , std::uint32_t > >;
   detail::arrowCopy< U >(
     *k , n , [ & ]( std::size_t i ) { return( x->get( i ) ); } );
   data = k->copy.data();
   }
  }
 else {
  using T = std::ranges::range_value_t< const Y >;
  format = arrowFormat< T >();
  if constexpr( std::ranges::contiguous_range< const Y > )
   data = std::ranges::data( *x );
  else {
   auto it = std::ranges::begin( *x );
   detail::arrowCopy< T >( *k , n ,
			   [ & ]( std::size_t ) { return( *it++ ); } );
   data = k->copy.data();
   }
  }
 if( data == nullptr )  // an empty column, whose buffer is still there
  data = &detail::kArrowEmpty;
 auto nm = std::make_unique< std::string >( name );
 k->owner = std::shared_ptr< const void >( std::move( x ) );
 k->buffers[ 1 ] = data;
 *s = ArrowSchema{ format ,  nm->c_str() , nullptr , 0 ,
		   0 ,       nullptr ,     nullptr ,
		   &detail::arrowReleaseSchema ,
		   nm.get() };
 *a = ArrowArray{
  std::int64_t( n ) ,          0 ,      0 , 2 , 0 , k->buffers , nullptr ,
  nullptr ,
  &detail::arrowReleaseArray , k.get()
  };
 nm.release();
 k.release();
 }

/// the structure \f$ x \f$ as a column of Arrow of no nulls, named
/// \f$ \mathit{name} \f$, in \f$ *a \f$ and \f$ *s \f$: \f$ x \f$ is moved
/// into the array, which keeps it alive until its consumer releases it
template< class X >
 requires ArrowExportable< X >
void exportArrow( X x , ArrowArray * a , ArrowSchema * s ,
		  std::string_view name = "" )
{
 exportArrow( std::make_shared< const X >( std::move( x ) ) , a , s , name );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArrowColumn ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a column of Arrow of numbers of type \f$ T \f$, taken without copy: its
/// values stay in the memory of its producer, which this object releases
/// when destroyed
template< ArrowNumber T >
class ArrowColumn
{

/*--------------------------------------------------------------------------*/
 public:
 /// the column \f$ *a \f$ of type \f$ s \f$, moved here: \f$ *a \f$ is
 /// left released, and \f$ s \f$ is only read
 /// \throws std::invalid_argument if the column is released, has nulls or
 ///         children, or is not of the format of \f$ T \f$
 ArrowColumn( ArrowArray * a , const ArrowSchema & s ) {
  detail::arrowCheck( *a , s , arrowFormat< T >() , "ArrowColumn" );
  f_a = *a;
  a->release = nullptr;  // moved, as the specification allows
  }

 /// no copy, since the column has one owner
 ArrowColumn( const ArrowColumn & ) = delete;

 /// no copy, since the column has one owner
 ArrowColumn & operator=( const ArrowColumn & ) = delete;

 /// the column of \f$ o \f$, which is left with none
 ArrowColumn( ArrowColumn && o ) noexcept : f_a( o.f_a ) {
  o.f_a.release = nullptr;
  }

 /// the column of \f$ o \f$, which is left with none, this one released
 ArrowColumn & operator=( ArrowColumn && o ) noexcept {
  if( this != &o ) {
   reset();
   f_a = o.f_a;
   o.f_a.release = nullptr;
   }
  return( *this );
  }

 /// the column released by its producer
 ~ArrowColumn( void ) { reset(); }

 /// the values, in the memory of the producer
 [[nodiscard]] std::span< const T > values( void ) const {
  if( f_a.release == nullptr || f_a.length == 0 )
   return( std::span< const T >() );
  return( std::span< const T >( static_cast< const T * >( f_a.buffers[ 1 ] ) +
				   std::size_t( f_a.offset ) ,
				 std::size_t( f_a.length ) ) );
  }

 /// the number of the values
 [[nodiscard]] std::size_t size( void ) const { return( values().size() ); }

 /// the value \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] T operator[]( std::size_t i ) const {
  ADS_PRE( i < size() );
  return( values()[ i ] );
  }

 /// the first value
 [[nodiscard]] const T * begin( void ) const { return( values().data() ); }

 /// past the last value
 [[nodiscard]] const T * end( void ) const {
  return( values().data() + size() );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the column released, if any
 void reset( void ) noexcept {
  if( f_a.release )
   f_a.release( &f_a );
  f_a.release = nullptr;
  }

 ArrowArray f_a{};  ///< the column, released if `release` is null

 };  // end( class ArrowColumn )

/*--------------------------------------------------------------------------*/
/*------------------------------- THE IMPORT -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the column of booleans \f$ a \f$ of type \f$ s \f$ copied into a
/// `BitVector`; \f$ a \f$ and \f$ s \f$ are only read
/// \throws std::invalid_argument if the column is released, has nulls or
///         children, or is not of booleans
inline BitVector toBitVector( const ArrowArray & a , const ArrowSchema & s )
{
 detail::arrowCheck( a , s , "b" , "toBitVector" );
 const std::size_t n = std::size_t( a.length ) , o = std::size_t( a.offset );
 const auto * p = static_cast< const unsigned char * >( a.buffers[ 1 ] );
 std::vector< Word > w( ( n + 63 ) / 64 , 0 );
 if( o % 8 == 0 ) {  // whole bytes, in the order of a little-endian word
  if( n > 0 )
   std::memcpy( w.data() , p + o / 8 , ( n + 7 ) / 8 );
  if constexpr( std::endian::native != std::endian::little )
   for( auto & x : w )
    x = detail::byteSwap( x );
  }
 else
  for( std::size_t i = 0 ; i < n ; ++i )
   if( ( p[ ( o + i ) / 8 ] >> ( ( o + i ) % 8 ) ) & 1 )
    w[ i / 64 ] |= Word( 1 ) << ( i % 64 );
 return( BitVector( w , n ) );  // the bits past n cleared
 }

/// the column \f$ a \f$ of type \f$ s \f$ copied into a `PackedArray< B ,
/// S >`: a column of booleans if \f$ B = 1 \f$, or of integers of any width,
/// each value clamped to \f$ 2^B - 1 \f$ if \f$ S \f$; \f$ a \f$ and
/// \f$ s \f$ are only read
/// \throws std::invalid_argument if the column is released, has nulls or
///         children, is not of integers (or booleans), or has a value
///         negative or, unless \f$ S \f$, of more than \f$ B \f$ bits
template< unsigned B , bool S = false >
[[nodiscard]] PackedArray< B , S > toPackedArray( const ArrowArray & a ,
						 const ArrowSchema & s )
{
 using P = PackedArray< B , S >;
 if constexpr( B == 1 )
  if( s.format && std::string_view( s.format ) == "b" ) {
   const BitVector b = toBitVector( a , s );
   Sequential q;
   return( P( q , b.size() , [ & ]( std::size_t i ) {
    return( typename P::Word( b[ i ] ) );
    } ) );
   }
 detail::arrowCheck( a , s , nullptr , "toPackedArray" );
 std::vector< typename P::Word > v( std::size_t( a.length ) );
 detail::arrowIntegers( a , s.format[ 0 ] , [ & ]( std::size_t i , auto x ) {
  if constexpr( std::is_signed_v< decltype( x ) > )
   if( x < 0 )
    throw( std::invalid_argument( "ADS::toPackedArray: a negative value" ) );
  v[ i ] = typename P::Word( x );
  if constexpr( ! S )
   if( v[ i ] > P::MAX )
    throw( std::invalid_argument( "ADS::toPackedArray: a value of more "
				  "bits than a field" ) );
  } );
 Sequential q;
 return( P( q , v.size() , [ & ]( std::size_t i ) { return( v[ i ] ); } ) );
 }

/// the column \f$ a \f$ of numbers of type \f$ T \f$, of type \f$ s \f$,
/// copied into a `std::vector`; \f$ a \f$ and \f$ s \f$ are only read
/// \throws std::invalid_argument if the column is released, has nulls or
///         children, or is not of the format of \f$ T \f$
template< ArrowNumber T >
[[nodiscard]] std::vector< T > toVector( const ArrowArray & a ,
					 const ArrowSchema & s )
{
 detail::arrowCheck( a , s , arrowFormat< T >() , "toVector" );
 if( a.length == 0 )
  return( std::vector< T >() );
 const T * p =
   static_cast< const T * >( a.buffers[ 1 ] ) + std::size_t( a.offset );
 return( std::vector< T >( p , p + a.length ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Arrow.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Arrow.h ----------------------------*/
/*--------------------------------------------------------------------------*/
