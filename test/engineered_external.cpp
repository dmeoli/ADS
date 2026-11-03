/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_external.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the files of records on the disk, beyond the book: RecordFile at
 * its limits (a file that is not there, an empty one, records appended by
 * pieces, records larger than the buffer of a scan), RecordWriter with a
 * buffer of one record, flushed by its destructor and kept by the thousand
 * at once, TempDirectory removed with its files, and the errors of a path
 * that cannot be written, at the level 3 of the contracts.
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

#include "ADS/external/RecordFile.h"

#include <array>
#include <cstdint>
#include <filesystem>
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

namespace {

/// a record of 40 bytes, with padding inside, as a node of a search
struct Rec {
 std::uint64_t key;
 std::uint32_t g;
 std::uint8_t op;
 std::array< std::uint64_t , 3 > tail;
 };

/// the record of the key \f$ k \f$
Rec rec( std::uint64_t k )
{
 return( Rec{ k , std::uint32_t( k * 3 ) , std::uint8_t( k % 7 ) ,
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
 CHECK( f.size() == 0 );                       // not on the disk
 CHECK( f.empty() );
 CHECK( f.readAll().empty() );
 std::size_t calls = 0;
 f.forEach( [ & ]( std::uint64_t ) { ++calls; } );
 CHECK( calls == 0 );
 f.append( nullptr , 0 );                      // makes it, empty
 CHECK( std::filesystem::exists( f.path() ) );
 CHECK( f.empty() );
 f.remove();
 CHECK( ! std::filesystem::exists( f.path() ) );
 f.remove();                                   // gone already: nothing
 f.clear();                                    // made again, empty
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
  std::vector< Rec > v( rng() % 300 );         // pieces of 0 to 299
  for( auto & x : v )
   x = rec( rng() );
  f.append( v.data() , v.size() );
  all.insert( all.end() , v.begin() , v.end() );
  CHECK( f.size() == all.size() );
  }
 for( std::size_t buffer : { std::size_t( 1 ) , std::size_t( 7 ) ,
                             all.size() , all.size() + 1 ,
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
 RecordFile< Rec > g( f.path() );              // the same file, reopened
 CHECK( g.size() == all.size() );
 f.clear();
 CHECK( f.empty() && g.empty() );
 CHECK_THROWS( f.forEach( []( const Rec & ) {} , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "record-writer" )
{
 TempDirectory dir;
 RecordFile< std::uint32_t > f( dir / "w.dat" );
 {
  RecordWriter< std::uint32_t > w( f , 1 );   // a write per record
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
  }                                            // flushed by m only
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
  TempDirectory other;                         // a name of its own
  CHECK( other.path() != p );
  std::filesystem::create_directories( dir / "sub" );
  RecordFile< int > f( dir.path() / "sub" / "x.dat" );
  const int x[] = { 1 , 2 , 3 };
  f.append( x , 3 );
  TempDirectory moved( std::move( other ) );   // removed once, by moved
  q = moved.path();
  CHECK( std::filesystem::is_directory( q ) );
  }
 CHECK( ! std::filesystem::exists( p ) );       // with what it held
 CHECK( ! std::filesystem::exists( q ) );
 TempDirectory outer;                           // a parent not there yet
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
 RecordFile< int > d( dir.path() );            // a directory, not a file
 CHECK_THROWS( d.append( &x , 1 ) , std::runtime_error );
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
