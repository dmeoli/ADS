/*--------------------------------------------------------------------------*/
/*------------------------ File interop_columnar.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tests of the columnar formats of other libraries: the columns of
 * Apache Arrow by its C Data Interface [see `ADS/interop/arrow/Arrow.h`],
 * read and written by nanoarrow where it is found; the portable format of
 * Roaring [see `ADS/set/RoaringBitmap.h`], against bytes written by
 * CRoaring and, where CRoaring is found, against the library itself [see
 * `ADS/interop/croaring/CRoaring.h`]; the vectors of sdsl-lite 3 and the
 * archives of cereal that both libraries share [see
 * `ADS/interop/sdsl/Sdsl.h`]. A test whose library is not compiled in
 * passes with nothing to check, and says so.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/interop/arrow/Arrow.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined( ADS_WITH_NANOARROW )
 #include <nanoarrow/nanoarrow.h>
#endif

#if defined( ADS_WITH_CROARING )
 #include "ADS/interop/croaring/CRoaring.h"
#endif

#if defined( ADS_WITH_SDSL )
 #include "ADS/interop/sdsl/Sdsl.h"

 #include <sdsl/rank_support_v5.hpp>
#endif

#if defined( ADS_WITH_SDSL ) && defined( ADS_WITH_CEREAL )
 #include "ADS/interop/cereal/Cereal.h"

 #include <cereal/archives/binary.hpp>
 #include <cereal/archives/json.hpp>
 #include <cereal/archives/portable_binary.hpp>
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

using namespace ADS;

/// a message for the tests whose library is not compiled in
[[maybe_unused]] void notCompiled( const char * what )
{
 std::cout << "  (" << what << " not compiled in: nothing to check)"
	   << std::endl;
 }

/// \f$ n \f$ random bits, a 1 with probability \f$ 1 / k \f$
BitVector randomBits( std::size_t n , unsigned k , unsigned seed )
{
 std::mt19937 r( seed );
 BitVector b( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  if( r() % k == 0 )
   b.set( i );
 return( b );
 }

/// the bit \f$ i \f$ of a buffer of booleans of Arrow
bool arrowBit( const void * p , std::size_t i )
{
 return( ( static_cast< const unsigned char * >( p )[ i / 8 ] >> ( i % 8 ) ) &
	  1 );
 }

/// the two structures released, if not yet
void release( ArrowArray & a , ArrowSchema & s )
{
 if( a.release )
  a.release( &a );
 if( s.release )
  s.release( &s );
 }

/// the bytes of a string of hexadecimal pairs
std::vector< std::byte > bytesOf( const char * h )
{
 std::vector< std::byte > v;
 for( std::istringstream is( h ) ; ; ) {
  unsigned x;
  if( ! ( is >> std::hex >> x ) )
   break;
  v.push_back( std::byte( x ) );
  }
 return( v );
 }

/// the 64 bits of FNV-1a of the bytes \f$ b \f$
std::uint64_t fnv( std::span< const std::byte > b )
{
 std::uint64_t h = 14695981039346656037ULL;
 for( const std::byte c : b ) {
  h ^= std::to_integer< std::uint64_t >( c );
  h *= 1099511628211ULL;
  }
 return( h );
 }

/// the set of the files bitmapwithoutruns.bin and bitmapwithruns.bin of
/// the specification of the format (and of CRoaring): the multiples of
/// 1000 below 100000, the multiples of 3 from 300000 below 600000, the
/// integers from 700000 below 800000
std::vector< std::uint32_t > specSample( void )
{
 std::vector< std::uint32_t > x;
 for( std::uint32_t k = 0 ; k < 100000 ; k += 1000 )
  x.push_back( k );
 for( std::uint32_t k = 100000 ; k < 200000 ; ++k )
  x.push_back( 3 * k );
 for( std::uint32_t k = 700000 ; k < 800000 ; ++k )
  x.push_back( k );
 return( x );
 }

/// random sets of 32 bits, with arrays, bitmaps and long intervals
std::vector< std::vector< std::uint32_t > > randomSets( void )
{
 std::vector< std::vector< std::uint32_t > > S{ {} , { 0 } , { 0xFFFFFFFFu } };
 std::mt19937 r( 7 );
 for( int t = 0 ; t < 6 ; ++t ) {
  std::vector< std::uint32_t > x;
  const std::uint32_t chunks = 1 + r() % 9;
  for( std::uint32_t c = 0 ; c < chunks ; ++c ) {
   const std::uint32_t key = r() % 65536 , kind = r() % 3;
   const std::uint32_t base = key << 16;
   if( kind == 0 )  // an array
    for( int k = 0 ; k < 1 + int( r() % 3000 ) ; ++k )
     x.push_back( base | ( r() & 0xFFFF ) );
   else if( kind == 1 )  // a bitmap
    for( std::uint32_t v = 0 ; v < 65536 ; ++v ) {
     if( r() % 3 == 0 )
      x.push_back( base | v );
     }
   else {  // intervals
    std::uint32_t v = r() % 100;
    while( v < 65000 ) {
     const std::uint32_t l = 1 + r() % 3000;
     for( std::uint32_t k = v ; k < std::min( v + l , 65536u ) ; ++k )
      x.push_back( base | k );
     v += l + 1 + r() % 500;
     }
    }
   }
  std::sort( x.begin() , x.end() );
  x.erase( std::unique( x.begin() , x.end() ) , x.end() );
  S.push_back( std::move( x ) );
  }
 return( S );
 }

/// the set of the items \f$ x \f$, strictly increasing
RoaringBitmap setOf( const std::vector< std::uint32_t > & x )
{
 return( RoaringBitmap( x.data() , x.size() ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- ARROW ----------------------------------*/
/*--------------------------------------------------------------------------*/

// a BitVector as a column of booleans, without copy, and back
ADS_TEST( "arrow-bit-vector" )
{
 for( const std::size_t n : { 0 , 1 , 63 , 64 , 65 , 1000 , 4097 } ) {
  const BitVector b = randomBits( n , 3 , unsigned( n + 1 ) );
  auto p = std::make_shared< const BitVector >( b );
  ArrowArray a;
  ArrowSchema s;
  exportArrow( p , &a , &s , "bits" );
  CHECK( std::string( s.format ) == "b" && std::string( s.name ) == "bits" );
  CHECK( s.n_children == 0 && s.dictionary == nullptr && s.flags == 0 );
  CHECK( a.length == std::int64_t( n ) && a.null_count == 0 && a.offset == 0 &&
	 a.n_buffers == 2 && a.n_children == 0 );
  CHECK( a.buffers[ 0 ] == nullptr && a.buffers[ 1 ] != nullptr );
  if constexpr( std::endian::native == std::endian::little )
   if( n > 0 )
    CHECK( a.buffers[ 1 ] == p->words().data() );  // no copy
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( arrowBit( a.buffers[ 1 ] , i ) == b[ i ] );
  CHECK( toBitVector( a , s ) == b );
  CHECK( p.use_count() == 2 );  // the array keeps it alive
  release( a , s );
  CHECK( a.release == nullptr && s.release == nullptr );
  CHECK( p.use_count() == 1 );
  }
 // a vector moved in, and a column of Arrow with an offset of 3 bits
 BitVector b = randomBits( 300 , 2 , 9 );
 const BitVector c = b;
 ArrowArray a;
 ArrowSchema s;
 exportArrow( std::move( b ) , &a , &s );
 a.offset = 3;
 a.length = 200;
 const BitVector d = toBitVector( a , s );
 CHECK( d.size() == 200 );
 for( std::size_t i = 0 ; i < 200 ; ++i )
  CHECK( d[ i ] == c[ i + 3 ] );
 a.offset = 64;  // whole bytes
 CHECK( toBitVector( a , s ).toString() == c.toString().substr( 64 , 200 ) );
 release( a , s );
 }

// the PackedArray of each width as a column of unsigned integers
ADS_TEST( "arrow-packed-array" )
{
 auto check = []< unsigned B >( const char * format ) {
  std::mt19937 r( B );
  PackedArray< B > p( 0 );
  for( int i = 0 ; i < 777 ; ++i )
   p.push_back( r() & PackedArray< B >::MAX );
  ArrowArray a;
  ArrowSchema s;
  exportArrow( p , &a , &s );
  CHECK( std::string( s.format ) == format && a.length == 777 );
  for( std::size_t i = 0 ; i < p.size() ; ++i ) {
   std::uint64_t v;
   if constexpr( B == 1 )
    v = arrowBit( a.buffers[ 1 ] , i );
   else if constexpr( B <= 8 )
    v = static_cast< const std::uint8_t * >( a.buffers[ 1 ] )[ i ];
   else if constexpr( B == 16 )
    v = static_cast< const std::uint16_t * >( a.buffers[ 1 ] )[ i ];
   else
    v = static_cast< const std::uint32_t * >( a.buffers[ 1 ] )[ i ];
   CHECK( v == p[ i ] );
   }
  CHECK( toPackedArray< B >( a , s ) == p );
  release( a , s );
  };
 check.template operator()< 1 >( "b" );
 check.template operator()< 2 >( "C" );
 check.template operator()< 4 >( "C" );
 check.template operator()< 8 >( "C" );
 check.template operator()< 16 >( "S" );
 check.template operator()< 32 >( "I" );
 // a column of wider integers into narrow fields: refused, or clamped
 ArrowArray a;
 ArrowSchema s;
 exportArrow( std::vector< std::int64_t >{ 1 , 2 , 300 } , &a , &s );
 CHECK_THROWS( toPackedArray< 8 >( a , s ) , std::invalid_argument );
 const auto q = toPackedArray< 8 , true >( a , s );
 CHECK( q[ 0 ] == 1 && q[ 2 ] == 255 && q.clamped() == 1 );
 CHECK( toPackedArray< 16 >( a , s )[ 2 ] == 300 );
 release( a , s );
 exportArrow( std::vector< std::int32_t >{ 1 , -2 } , &a , &s );
 CHECK_THROWS( toPackedArray< 8 >( a , s ) , std::invalid_argument );
 release( a , s );
 }

// the sequences of numbers, without copy where contiguous
ADS_TEST( "arrow-sequences" )
{
 auto check = []< class T >( const char * format ) {
  std::vector< T > v;
  for( int i = 0 ; i < 100 ; ++i )
   v.push_back( T( i % 50 ) );
  auto p = std::make_shared< std::vector< T > >( v );
  ArrowArray a;
  ArrowSchema s;
  exportArrow( p , &a , &s );
  CHECK( std::string( s.format ) == format && a.buffers[ 1 ] == p->data() );
  CHECK( toVector< T >( a , s ) == v );
  {
   ArrowColumn< T > c( &a , s );  // taken without copy
   CHECK( a.release == nullptr && c.values().data() == p->data() );
   CHECK( std::vector< T >( c.begin() , c.end() ) == v );
   CHECK( p.use_count() == 2 );
   }
  CHECK( p.use_count() == 1 );  // released with the column
  s.release( &s );
  };
 check.template operator()< std::int8_t >( "c" );
 check.template operator()< std::uint8_t >( "C" );
 check.template operator()< std::int16_t >( "s" );
 check.template operator()< std::uint16_t >( "S" );
 check.template operator()< std::int32_t >( "i" );
 check.template operator()< std::uint32_t >( "I" );
 check.template operator()< std::int64_t >( "l" );
 check.template operator()< std::uint64_t >( "L" );
 check.template operator()< float >( "f" );
 check.template operator()< double >( "g" );
 // a sequence that is not contiguous, copied; an empty one; a view
 ArrowArray a;
 ArrowSchema s;
 exportArrow( std::deque< double >{ 1.5 , 2.5 , 3.5 } , &a , &s );
 CHECK( toVector< double >( a , s ) ==
	std::vector< double >( { 1.5 , 2.5 , 3.5 } ) );
 CHECK_THROWS( toVector< float >( a , s ) , std::invalid_argument );
 CHECK_THROWS( ArrowColumn< std::int64_t >( &a , s ) , std::invalid_argument );
 release( a , s );
 exportArrow( std::vector< int >() , &a , &s );
 CHECK( a.length == 0 && a.buffers[ 1 ] != nullptr );
 CHECK( toVector< int >( a , s ).empty() );
 release( a , s );
 const std::vector< std::uint16_t > w{ 4 , 5 , 6 };
 exportArrow( std::span< const std::uint16_t >( w ) , &a , &s );
 CHECK( a.buffers[ 1 ] == w.data() );
 release( a , s );
 // a column with nulls, or released, is refused
 std::vector< std::int32_t > x{ 7 , 8 , 9 };
 exportArrow( x , &a , &s );
 const unsigned char valid = 0x5;  // the item 1 null
 const void * buffers[ 2 ] = { &valid , a.buffers[ 1 ] };
 ArrowArray n = a;
 n.buffers = buffers;
 n.null_count = 1;
 CHECK_THROWS( toVector< std::int32_t >( n , s ) , std::invalid_argument );
 n.null_count = -1;  // not known, so counted
 CHECK_THROWS( toVector< std::int32_t >( n , s ) , std::invalid_argument );
 release( a , s );
 CHECK_THROWS( toVector< std::int32_t >( a , s ) , std::invalid_argument );
 }

// the columns of ADS read by nanoarrow, and those of nanoarrow by ADS
ADS_TEST( "arrow-nanoarrow" )
{
#if defined( ADS_WITH_NANOARROW )
 auto view = []( ArrowArray & a , ArrowSchema & s , auto && use ) {
  ArrowError e;
  ArrowArrayView v;
  CHECK( ArrowArrayViewInitFromSchema( &v , &s , &e ) == NANOARROW_OK );
  CHECK( ArrowArrayViewSetArray( &v , &a , &e ) == NANOARROW_OK );
  CHECK( ArrowArrayViewValidate( &v , NANOARROW_VALIDATION_LEVEL_FULL , &e ) ==
	 NANOARROW_OK );
  use( v );
  ArrowArrayViewReset( &v );
  release( a , s );
  };
 ArrowArray a;
 ArrowSchema s;
 const BitVector b = randomBits( 1001 , 3 , 5 );
 exportArrow( b , &a , &s );
 view( a , s , [ & ]( ArrowArrayView & v ) {
  CHECK( v.storage_type == NANOARROW_TYPE_BOOL && v.length == 1001 );
  for( std::size_t i = 0 ; i < b.size() ; ++i )
   CHECK( ArrowArrayViewGetIntUnsafe( &v , std::int64_t( i ) ) == b[ i ] );
  } );
 PackedArray< 4 > p( 0 );
 for( unsigned i = 0 ; i < 99 ; ++i )
  p.push_back( i % 16 );
 exportArrow( p , &a , &s );
 view( a , s , [ & ]( ArrowArrayView & v ) {
  CHECK( v.storage_type == NANOARROW_TYPE_UINT8 );
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   CHECK( ArrowArrayViewGetUIntUnsafe( &v , std::int64_t( i ) ) == p[ i ] );
  } );
 const std::vector< double > d{ 0.5 , -1.25 , 3e10 };
 exportArrow( d , &a , &s );
 view( a , s , [ & ]( ArrowArrayView & v ) {
  CHECK( v.storage_type == NANOARROW_TYPE_DOUBLE );
  for( std::size_t i = 0 ; i < d.size() ; ++i )
   CHECK( ArrowArrayViewGetDoubleUnsafe( &v , std::int64_t( i ) ) == d[ i ] );
  } );
 // built by nanoarrow, read by ADS
 auto build = [ & ]( ArrowType t , const std::vector< std::int64_t > & x ,
		     bool null ) {
  ArrowError e;
  CHECK( ArrowArrayInitFromType( &a , t ) == NANOARROW_OK );
  CHECK( ArrowArrayStartAppending( &a ) == NANOARROW_OK );
  for( const auto v : x )
   CHECK( ArrowArrayAppendInt( &a , v ) == NANOARROW_OK );
  if( null )
   CHECK( ArrowArrayAppendNull( &a , 1 ) == NANOARROW_OK );
  CHECK( ArrowArrayFinishBuildingDefault( &a , &e ) == NANOARROW_OK );
  CHECK( ArrowSchemaInitFromType( &s , t ) == NANOARROW_OK );
  };
 const std::vector< std::int64_t > x{ 5 , 0 , 7 , 1 , 1 , 0 , 1 , 0 , 1 };
 build( NANOARROW_TYPE_INT32 , x , false );
 {
  ArrowColumn< std::int32_t > c( &a , s );
  CHECK( c.size() == x.size() && c[ 2 ] == 7 );
  }
 s.release( &s );
 build( NANOARROW_TYPE_UINT16 , x , false );
 CHECK( toPackedArray< 4 >( a , s )[ 2 ] == 7 );
 CHECK( toVector< std::uint16_t >( a , s )[ 0 ] == 5 );
 release( a , s );
 const std::vector< std::int64_t > y( x.begin() + 3 , x.end() );
 build( NANOARROW_TYPE_BOOL , y , false );
 CHECK( toBitVector( a , s ).toString() == "110101" );
 CHECK( toPackedArray< 1 >( a , s )[ 2 ] == 0 );
 release( a , s );
 build( NANOARROW_TYPE_INT64 , x , true );
 CHECK_THROWS( toVector< std::int64_t >( a , s ) , std::invalid_argument );
 release( a , s );
#else
 notCompiled( "nanoarrow" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- ROARING ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the portable format, written and read, against bytes of CRoaring
ADS_TEST( "roaring-portable" )
{
 // { 1 , 2 , 65539 } as CRoaring writes it: the cookie 12346, 2
 // containers, their chunks and counts less 1, their offsets 24 and 28
 const auto small = bytesOf( "3A 30 00 00 02 00 00 00 00 00 01 00 01 00 00 00"
			     " 18 00 00 00 1C 00 00 00 01 00 02 00 03 00" );
 const RoaringBitmap a = setOf( { 1 , 2 , 0x10003 } );
 CHECK( a.portableSizeInBytes() == small.size() );
 CHECK( a.writePortable() == small );
 std::size_t used = 0;
 CHECK( RoaringBitmap::readPortable( small , &used ).items() == a.items() &&
	used == small.size() );
 // [ 10 , 20 ] and 65541 as CRoaring writes them after
 // roaring_bitmap_run_optimize(): the cookie 12347 with 2 containers less
 // 1, the bitmap of the runs, no offset, one run, one array
 const auto runs = bytesOf( "3B 30 01 00 01 00 00 0A 00 01 00 00 00 01 00"
			    " 0A 00 0A 00 05 00" );
 std::vector< std::uint32_t > x;
 for( std::uint32_t k = 10 ; k <= 20 ; ++k )
  x.push_back( k );
 x.push_back( 0x10005 );
 CHECK( RoaringBitmap::readPortable( runs , &used ).items() == x &&
	used == runs.size() );
 // the sample of the specification, written as the file
 // bitmapwithoutruns.bin of CRoaring (72616 bytes, of this digest), and
 // the random sets
 const auto spec = setOf( specSample() );
 const auto w = spec.writePortable();
 CHECK( w.size() == 72616 && fnv( w ) == 0xd680d2b8a7927062ULL );
 for( const auto & y : randomSets() ) {
  const RoaringBitmap s = setOf( y );
  const auto v = s.writePortable();
  CHECK( v.size() == s.portableSizeInBytes() );
  CHECK( RoaringBitmap::readPortable( v , &used ).items() == y &&
	 used == v.size() );
  std::vector< std::byte > z = v;  // followed by other bytes
  z.resize( z.size() + 5 , std::byte( 0xAB ) );
  CHECK( RoaringBitmap::readPortable( z , &used ).size() == y.size() &&
	 used == v.size() );
  }
 // every truncation, and corrupted fields, refused
 for( const auto & b : { small , runs } )
  for( std::size_t k = 0 ; k < b.size() ; ++k )
   CHECK_THROWS( RoaringBitmap::readPortable(
		   std::span< const std::byte >( b.data() , k ) ) ,
		 PersistError );
 for( std::size_t k = 0 ; k < w.size() ; k += 1009 )
  CHECK_THROWS(
    RoaringBitmap::readPortable( std::span< const std::byte >( w.data() ,
							       k ) ) ,
    PersistError );
 auto bad = [ & ]( std::vector< std::byte > b , std::size_t i , unsigned v ,
		   std::size_t j = 0 , unsigned u = 0 ) {
  b[ i ] = std::byte( v );
  if( j > 0 )
   b[ j ] = std::byte( u );
  CHECK_THROWS( RoaringBitmap::readPortable( b ) , PersistError );
  };
 bad( small , 0 , 0x00 );            // the cookie
 bad( small , 8 , 0x02 );            // the chunks not increasing
 bad( small , 26 , 0x01 );           // the items of an array not increasing
 bad( runs , 7 , 0x0B );             // a count unlike the items of the runs
 bad( runs , 16 , 0xFF , 18 , 0xFF );  // a run past the chunk
 }

// the sets of ADS to and from the bitmaps of CRoaring
ADS_TEST( "roaring-croaring" )
{
#if defined( ADS_WITH_CROARING )
 auto bytes = []( const roaring_bitmap_t * r ) {
  std::vector< std::byte > b( roaring_bitmap_portable_size_in_bytes( r ) );
  roaring_bitmap_portable_serialize( r ,
				     reinterpret_cast< char * >( b.data() ) );
  return( b );
  };
 // the sample of the specification, as the files bitmapwithoutruns.bin
 // and bitmapwithruns.bin of CRoaring hold it (of these digests)
 const auto x = specSample();
 roaring_bitmap_t * r = roaring_bitmap_of_ptr( x.size() , x.data() );
 CHECK( fnv( bytes( r ) ) == 0xd680d2b8a7927062ULL );  // without runs
 CHECK( setOf( x ).writePortable() == bytes( r ) );
 CHECK( roaring_bitmap_run_optimize( r ) );
 const auto withRuns = bytes( r );
 CHECK( fnv( withRuns ) == 0x2368597bb180cad5ULL );  // bitmapwithruns.bin
 CHECK( RoaringBitmap::readPortable( withRuns ).items() == x );
 roaring_bitmap_free( r );
 for( const auto & y : randomSets() ) {
  roaring_bitmap_t * c = roaring_bitmap_of_ptr( y.size() , y.data() );
  roaring_bitmap_run_optimize( c );
  const RoaringBitmap s = fromCRoaring( c );  // runs read
  CHECK( s.items() == y );
  roaring_bitmap_t * d = toCRoaring( s );
  CHECK( roaring_bitmap_equals( c , d ) );
  roaring_bitmap_free( d );
  roaring_bitmap_free( c );
  const roaring::Roaring R = toRoaring( setOf( y ) );
  CHECK( R.cardinality() == y.size() && fromCRoaring( R ).items() == y );
  }
#else
 notCompiled( "CRoaring" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- SDSL-LITE -------------------------------*/
/*--------------------------------------------------------------------------*/

// the vectors of bits and of fields to and from those of sdsl-lite
ADS_TEST( "sdsl-vectors" )
{
#if defined( ADS_WITH_SDSL )
 for( const std::size_t n : { 0 , 1 , 64 , 65 , 5000 } ) {
  const BitVector b = randomBits( n , 4 , unsigned( n + 3 ) );
  const sdsl::bit_vector v = ADS::toSdsl( b );
  CHECK( v.size() == n );
  for( std::size_t i = 0 ; i < n ; ++i )
   CHECK( v[ i ] == b[ i ] );
  CHECK( ADS::toBitVector( v ) == b );
  // a rank of sdsl over the bits of ADS, and one of ADS over those of sdsl
  sdsl::rank_support_v5< 1 > r( &v );
  const RankSelect q( ADS::toBitVector( v ) );
  for( std::size_t i = 0 ; i <= n ; i += 7 )
   CHECK( r.rank( i ) == q.rank1( i ) );
  }
 auto check = []< unsigned B >( void ) {
  std::mt19937 r( B );
  PackedArray< B > p( 0 );
  for( int i = 0 ; i < 1234 ; ++i )
   p.push_back( r() & PackedArray< B >::MAX );
  const sdsl::int_vector< B > v = ADS::toSdsl( p );
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   CHECK( std::uint64_t( v[ i ] ) == p[ i ] );
  CHECK( ADS::toPackedArray< B >( v ) == p );
  sdsl::int_vector<> d( p.size() , 0 , B );  // the width at run time
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   d[ i ] = p[ i ];
  CHECK( ADS::toPackedArray< B >( d ) == p );
  };
 check.template operator()< 1 >();
 check.template operator()< 2 >();
 check.template operator()< 4 >();
 check.template operator()< 8 >();
 check.template operator()< 16 >();
 check.template operator()< 32 >();
 // another width, field by field
 sdsl::int_vector<> e( 10 , 0 , 5 );
 for( std::size_t i = 0 ; i < 10 ; ++i )
  e[ i ] = 3 * i;
 CHECK( ADS::toPackedArray< 8 >( e )[ 9 ] == 27 );
 CHECK_THROWS( ADS::toPackedArray< 4 >( e ) , std::invalid_argument );
 CHECK( ( ADS::toPackedArray< 4 , true >( e )[ 9 ] ) == 15 );
#else
 notCompiled( "sdsl-lite" );
#endif
 }

// a BitVector saved by ADS in an archive of cereal and loaded by sdsl-lite,
// and the converse
ADS_TEST( "sdsl-cereal" )
{
#if defined( ADS_WITH_SDSL ) && defined( ADS_WITH_CEREAL )
 auto check = []< class O , class I >( void ) {
  for( const std::size_t n : { 0 , 1 , 64 , 100 , 3001 } ) {
   const BitVector b = randomBits( n , 3 , unsigned( n + 11 ) );
   std::stringstream s1;
   {
    O a( s1 );
    a( b );
    }
   sdsl::bit_vector v;
   {
    I a( s1 );
    a( v );
    }
   CHECK( ADS::toBitVector( v ) == b );
   std::stringstream s2;
   {
    O a( s2 );
    a( v );
    }
   BitVector c;
   {
    I a( s2 );
    a( c );
    }
   CHECK( c == b );
   }
  };
 check.template
 operator()< cereal::BinaryOutputArchive , cereal::BinaryInputArchive >();
 check.template operator()< cereal::PortableBinaryOutputArchive ,
			    cereal::PortableBinaryInputArchive >();
 check.template
 operator()< cereal::JSONOutputArchive , cereal::JSONInputArchive >();
#else
 notCompiled( "sdsl-lite and cereal" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File interop_columnar.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
