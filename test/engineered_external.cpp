/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_external.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the files of records on the disk, beyond the book: RecordFile at
 * its limits (a file that is not there, an empty one, records appended by
 * pieces, records larger than the buffer of a scan), RecordWriter with a
 * buffer of one record, flushed by its destructor and kept by the thousand
 * at once, TempDirectory removed with its files, the errors of a path
 * that cannot be written and a record left partial at the end of a file,
 * and MappedFile on a missing file, on an empty one and through its copies
 * and its offers, at the level 3 of the contracts.
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

#include "ADS/external/MappedFile.h"
#include "ADS/external/RecordFile.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- TESTS ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/// a record of 40 bytes, with padding inside, as a node of a search
struct Rec
{
 std::uint64_t key;
 std::uint32_t g;
 std::uint8_t op;
 std::array< std::uint64_t , 3 > tail;
 };

/// the record of the key \f$ k \f$
Rec rec( std::uint64_t k )
{
 return(
   Rec{ k , std::uint32_t( k * 3 ) , std::uint8_t( k % 7 ) ,
	{ k , ~k , k * k } } );
 }

/// whether the two records hold the same fields
bool same( const Rec & a , const Rec & b )
{
 return( ( a.key == b.key ) && ( a.g == b.g ) && ( a.op == b.op ) &&
	  ( a.tail == b.tail ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-file-empty" )
{
 TempDirectory dir;
 RecordFile< std::uint64_t > f( dir / "none.dat" );
 CHECK( f.size() == 0 );  // not on the disk
 CHECK( f.empty() );
 CHECK( f.readAll().empty() );
 std::size_t calls = 0;
 f.forEach( [ & ]( std::uint64_t ) { ++calls; } );
 CHECK( calls == 0 );
 f.append( nullptr , 0 );  // makes it, empty
 CHECK( std::filesystem::exists( f.path() ) );
 CHECK( f.empty() );
 f.remove();
 CHECK( ! std::filesystem::exists( f.path() ) );
 f.remove();  // gone already: nothing
 f.clear();   // made again, empty
 CHECK( std::filesystem::exists( f.path() ) && f.empty() );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-file-append" )
{
 TempDirectory dir;
 RecordFile< Rec > f( dir / "recs.dat" );
 std::vector< Rec > all;
 std::mt19937_64 rng( 7 );
 for( std::size_t piece = 0 ; piece < 50 ; ++piece ) {
  std::vector< Rec > v( rng() % 300 );  // pieces of 0 to 299
  for( auto & x : v )
   x = rec( rng() );
  f.append( v.data() , v.size() );
  all.insert( all.end() , v.begin() , v.end() );
  CHECK( f.size() == all.size() );
  }
 for( std::size_t buffer :
      { std::size_t( 1 ) , std::size_t( 7 ) , all.size() , all.size() + 1 ,
	RecordFile< Rec >::defaultBuffer } ) {
  std::size_t i = 0;
  bool ok = true;
  f.forEach( [ & ]( const Rec & x ) { ok = ok && same( x , all[ i++ ] ); } ,
	     buffer );
  CHECK( ok && ( i == all.size() ) );
  }
 const auto back = f.readAll();
 CHECK( back.size() == all.size() );
 for( std::size_t i = 0 ; i < all.size() ; ++i )
  CHECK( same( back[ i ] , all[ i ] ) );
 RecordFile< Rec > g( f.path() );  // the same file, reopened
 CHECK( g.size() == all.size() );
 f.clear();
 CHECK( f.empty() && g.empty() );
 CHECK_THROWS( f.forEach( []( const Rec & ) { } , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-writer" )
{
 TempDirectory dir;
 RecordFile< std::uint32_t > f( dir / "w.dat" );
 {
  RecordWriter< std::uint32_t > w( f , 1 );  // a write per record
  for( std::uint32_t i = 0 ; i < 100 ; ++i ) {
   w.push( i );
   CHECK( w.pending() == 0 );
   CHECK( f.size() == i + 1 );
   }
  }
 {
  RecordWriter< std::uint32_t > w( f , 64 );
  for( std::uint32_t i = 100 ; i < 1000 ; ++i )
   w.push( i );
  CHECK( w.pending() == 900 % 64 );
  CHECK( f.size() == 100 + 900 - 900 % 64 );
  RecordWriter< std::uint32_t > m( std::move( w ) );  // the buffer moves
  CHECK( m.pending() == 900 % 64 );
  }  // flushed by m only
 const auto v = f.readAll();
 std::vector< std::uint32_t > expect( 1000 );
 std::iota( expect.begin() , expect.end() , 0u );
 CHECK( v == expect );
 CHECK_THROWS( RecordWriter< std::uint32_t >( f , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-writer-many-files" )
{
 // more writers than the descriptors a process may hold (1024 by default
 // on Linux), each with records in its buffer: a file is opened only to
 // write a buffer
 TempDirectory dir;
 constexpr std::size_t n = 3000;
 std::vector< RecordFile< std::uint64_t > > files;
 files.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  files.emplace_back( dir / ( "b" + std::to_string( i ) + ".dat" ) );
 std::vector< RecordWriter< std::uint64_t > > ws;
 ws.reserve( n );
 for( auto & f : files )
  ws.emplace_back( f , 4 );
 for( std::uint64_t k = 0 ; k < 10 * n ; ++k )
  ws[ k % n ].push( k );
 for( auto & w : ws )
  w.flush();
 bool ok = true;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const auto v = files[ i ].readAll();
  ok = ok && ( v.size() == 10 );
  for( std::size_t j = 0 ; ok && ( j < v.size() ) ; ++j )
   ok = ( v[ j ] == i + j * n );
  }
 CHECK( ok );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-file-large-records" )
{
 // records of 2 MB, larger than the buffer of 1 MB: one record per read
 using Big = std::array< std::uint64_t , 1 << 18 >;
 static_assert( RecordFile< Big >::defaultBuffer == 1 );
 TempDirectory dir;
 RecordFile< Big > f( dir / "big.dat" );
 std::vector< Big > v( 3 );
 for( std::size_t r = 0 ; r < v.size() ; ++r )
  for( std::size_t i = 0 ; i < v[ r ].size() ; ++i )
   v[ r ][ i ] = r * 1000003 + i;
 f.append( v.data() , v.size() );
 CHECK( f.size() == 3 );
 std::size_t r = 0;
 bool ok = true;
 f.forEach( [ & ]( const Big & x ) { ok = ok && ( x == v[ r++ ] ); } );
 CHECK( ok && ( r == 3 ) );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "temp-directory" )
{
 std::filesystem::path p , q;
 {
  TempDirectory dir( {} , "ads-test-" );
  p = dir.path();
  CHECK( std::filesystem::is_directory( p ) );
  CHECK( p.filename().string().rfind( "ads-test-" , 0 ) == 0 );
  TempDirectory other;  // a name of its own
  CHECK( other.path() != p );
  std::filesystem::create_directories( dir / "sub" );
  RecordFile< int > f( dir.path() / "sub" / "x.dat" );
  const int x[] = { 1 , 2 , 3 };
  f.append( x , 3 );
  TempDirectory moved( std::move( other ) );  // removed once, by moved
  q = moved.path();
  CHECK( std::filesystem::is_directory( q ) );
  }
 CHECK( ! std::filesystem::exists( p ) );  // with what it held
 CHECK( ! std::filesystem::exists( q ) );
 TempDirectory outer;  // a parent not there yet
 {
  TempDirectory nested( outer.path() / "a" / "b" );
  CHECK( std::filesystem::is_directory( nested.path() ) );
  }
 CHECK( std::filesystem::is_directory( outer.path() / "a" / "b" ) );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-file-errors" )
{
 TempDirectory dir;
 RecordFile< int > f( dir / "missing-dir" / "x.dat" );  // no such directory
 const int x = 1;
 CHECK_THROWS( f.append( &x , 1 ) , std::runtime_error );
 CHECK_THROWS( f.clear() , std::runtime_error );
 CHECK_THROWS( f.append( nullptr , 1 ) , contract_violation );
 RecordFile< int > d( dir.path() );  // a directory, not a file
 CHECK_THROWS( d.append( &x , 1 ) , std::runtime_error );
 }

/*--------------------------------------------------------------------------*/

/// the bytes \f$ b \f$ written as the whole file at \f$ p \f$
static void writeBytes( const std::filesystem::path & p ,
			const std::string & b )
{
 std::ofstream out( p , std::ios::binary | std::ios::trunc );
 out.write( b.data() , std::streamsize( b.size() ) );
 }

ADS_TEST( "record-file-partial-record" )
{
 TempDirectory dir;
 RecordFile< std::uint32_t > f( dir / "partial.dat" );
 const std::uint32_t a[] = { 11 , 22 };
 f.append( a , 2 );
 // 2 bytes of a third record, as an interrupted write leaves them
 std::ofstream( f.path() , std::ios::binary | std::ios::app ).write( "xy" ,
								     2 );
 CHECK( std::filesystem::file_size( f.path() ) == 10 );
 CHECK( f.size() == 2 );
 CHECK( f.readAll() == std::vector< std::uint32_t >( { 11 , 22 } ) );
 std::size_t calls = 0;
 f.forEach( [ & ]( std::uint32_t ) { ++calls; } , 1 );
 CHECK( calls == 2 );
 // an append goes after the whole records, the partial one dropped
 const std::uint32_t c = 33;
 f.append( &c , 1 );
 CHECK( f.size() == 3 );
 CHECK( f.readAll() == std::vector< std::uint32_t >( { 11 , 22 , 33 } ) );
 {
  RecordWriter< std::uint32_t > w( f , 4 );
  std::ofstream( f.path() , std::ios::binary | std::ios::app ).write( "z" ,
								      1 );
  w.push( 44 );
  }
 CHECK( f.readAll() == std::vector< std::uint32_t >( { 11 , 22 , 33 , 44 } ) );
 // a file shorter than one record has none
 writeBytes( f.path() , "abc" );
 CHECK( f.size() == 0 && f.empty() && f.readAll().empty() );
 f.append( &c , 1 );
 CHECK( f.readAll() == std::vector< std::uint32_t >( { 33 } ) );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-file-contracts" )
{
 TempDirectory dir;
 RecordFile< int > f( dir / "c.dat" );
 const int x[] = { 1 , 2 , 3 };
 f.append( x , 3 );
 CHECK_THROWS( f.forEach( []( int ) { } , 0 ) , contract_violation );
 CHECK_THROWS( RecordWriter< int >( f , 0 ) , contract_violation );
 // a copy of a RecordFile names the same file
 RecordFile< int > g = f;
 g.append( x , 1 );
 CHECK( f.size() == 4 && g.readAll() == f.readAll() );
 // a moved writer writes its buffer once, by its new owner
 {
  RecordWriter< int > w( f , 8 );
  w.push( 7 );
  RecordWriter< int > v( std::move( w ) );
  CHECK( v.pending() == 1 );
  }
 CHECK( f.size() == 5 && f.readAll().back() == 7 );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "mapped-file" )
{
 TempDirectory dir;
 // no file: no mapping and nothing held
 const MappedFile none;
 CHECK( ! none && none.bytes().empty() && ! none.mapped() );
 CHECK( ! none.holds( &none ) && none.take( 1 ) == nullptr );
 CHECK_THROWS( none.offer( nullptr , 1 ) , contract_violation );
 CHECK_THROWS( MappedFile( dir / "missing.ads" ) , PersistError );
 // a file of 0 bytes: a file, with no byte, mapped or not
 writeBytes( dir / "empty.ads" , "" );
 const MappedFile e( dir / "empty.ads" );
 CHECK( bool( e ) && e.bytes().empty() && ! e.holds( &e ) );
 CHECK( e.take( 0 ) == nullptr );
 CHECK_THROWS( e.offer( &dir , 1 ) , contract_violation );
 // a file of 5000 bytes, on a page if mapped, on a line of the cache if
 // read
 std::string b( 5000 , '\0' );
 for( std::size_t i = 0 ; i < b.size() ; ++i )
  b[ i ] = char( i * 7 );
 writeBytes( dir / "data.ads" , b );
 MappedFile m( dir / "data.ads" );
 const auto s = m.bytes();
 CHECK( s.size() == b.size() &&
	std::memcmp( s.data() , b.data() , b.size() ) == 0 );
 const auto at = reinterpret_cast< std::uintptr_t >( s.data() );
 CHECK( at % ( m.mapped() ? 4096 : 64 ) == 0 );
 CHECK( m.holds( s.data() ) && m.holds( s.data() + s.size() - 1 ) );
 CHECK( ! m.holds( s.data() + s.size() ) );
 // a write stays in the process
 s[ 0 ] = std::byte( 0x5A );
 CHECK( MappedFile( dir / "data.ads" ).bytes()[ 0 ] == std::byte( b[ 0 ] ) );
 // an offer is taken once, and only at its size
 m.offer( s.data() + 64 , 128 );
 CHECK( m.take( 64 ) == nullptr );
 CHECK( m.take( 128 ) == s.data() + 64 && m.take( 128 ) == nullptr );
 CHECK_THROWS( m.offer( &dir , 8 ) , contract_violation );
 // the copies share the mapping, which outlives the first of them
 MappedFile c = m;
 CHECK( c == m && c.bytes().data() == s.data() );
 c.offer( s.data() , 8 );  // one offer, seen by every copy
 CHECK( m.take( 8 ) == s.data() );
 const MappedFile & self = c;
 c = self;  // a copy onto itself keeps the mapping
 CHECK( c == m );
 MappedFile v = std::move( m );
 CHECK( ! m && v == c );
 m = std::move( v );
 CHECK( m == c && ! v );
 m = MappedFile();
 CHECK( ! m && c.bytes()[ 0 ] == std::byte( 0x5A ) );
 c = e;  // the last copy released, the other mapping shared
 CHECK( c == e && c.bytes().empty() );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_external.cpp --------------------*/
/*--------------------------------------------------------------------------*/
