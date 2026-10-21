/*--------------------------------------------------------------------------*/
/*---------------------------- File memory.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered memory beyond the arena and the pool: the large
 * blocks kept by a thread for the requests of the same size, and the
 * allocator by size classes, at the level 3 of the contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/memory/SizeClassAllocator.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <random>
#include <thread>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the blocks of 8 MB or more given back are kept by the thread and given
// again to a request of the same size, aligned to a huge page, with their
// contents writable; a request of another size takes a new block; no
// more than 16 blocks and 64 MB in all are kept; a small block is not
// kept; another
// thread keeps its own blocks, freed when it ends; a vector and a pool
// built twice reuse the blocks
ADS_TEST( "huge-page-cache" )
{
 using A = HugePageAllocator< std::uint64_t >;
 A a;
 const std::size_t n = ( std::size_t( 8 ) << 20 ) / 8;   // 8 MB
 std::uint64_t * p = a.allocate( n );
 std::memset( p , 1 , n * 8 );
 a.deallocate( p , n );
 std::uint64_t * q = a.allocate( n );       // the same block again
 CHECK( q == p && reinterpret_cast< std::uintptr_t >( q ) % A::PAGE == 0 );
 q[ n - 1 ] = 7;
 std::uint64_t * r = a.allocate( 2 * n );   // another size: a new one
 CHECK( r != q );
 a.deallocate( q , n );
 a.deallocate( r , 2 * n );
 std::vector< std::uint64_t * > V;          // more than 16 blocks
 for( int i = 0 ; i < 20 ; ++i )
  V.push_back( a.allocate( A::PAGE / 8 ) );
 for( auto * v : V )
  a.deallocate( v , A::PAGE / 8 );
 const std::size_t big = ( std::size_t( 80 ) << 20 ) / 8;   // over 64 MB
 std::uint64_t * b = a.allocate( big );
 a.deallocate( b , big );
 std::uint64_t * s = a.allocate( 16 );      // a small block
 a.deallocate( s , 16 );
 std::thread t( [](){                       // a thread of its own
  A c;
  std::uint64_t * x = c.allocate( ( std::size_t( 2 ) << 20 ) / 8 );
  c.deallocate( x , ( std::size_t( 2 ) << 20 ) / 8 );
  } );
 t.join();
 for( int round = 0 ; round < 2 ; ++round ) {   // built twice
  std::vector< std::uint64_t , A > W;
  for( std::uint64_t i = 0 ; i < ( std::uint64_t( 1 ) << 20 ) ; ++i )
   W.push_back( i );
  CHECK( W[ 12345 ] == 12345 );
  ObjectPool< std::uint64_t > P;
  for( int i = 0 ; i < 300000 ; ++i )
   CHECK( *P.make( std::uint64_t( i ) ) == std::uint64_t( i ) );
  }
 }

// the allocator by size classes: each class the least not below its
// requests, its size the class of itself, the waste below 25% past 128
// bytes; random requests of 1 to 40000 bytes (beyond the classes too)
// and releases, each block filled with its number and checked when given
// back, so that two blocks never overlap; the alignment to 16 bytes; a
// block given back taken again first; a move, and release()
ADS_TEST( "size-class-allocator" )
{
 using S = SizeClassAllocator;
 for( unsigned c = 0 ; c < 41 ; ++c )
  CHECK( S::sizeClass( S::classSize( c ) ) == c );
 for( std::size_t n = 1 ; n <= S::LARGEST ; ++n ) {
  const std::size_t z = S::classSize( S::sizeClass( n ) );
  CHECK( z >= n && ( n <= 128 ? z - n < 16 : 4 * ( z - n ) < z ) );
  CHECK( ( n == 1 ) || ( S::sizeClass( n - 1 ) <= S::sizeClass( n ) ) );
  }
 CHECK_THROWS( S().allocate( 0 ) , contract_violation );
 S A;
 struct Live {
  unsigned char * p;
  std::size_t n;
  unsigned char tag;
  };
 std::vector< Live > L;
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 20000 ; ++t )
  if( L.empty() || ( g() % 3 ) ) {
   const std::size_t n = ( g() % 10 ) ? 1 + g() % 600 : 1 + g() % 40000;
   auto * p = static_cast< unsigned char * >( A.allocate( n ) );
   CHECK( ( n <= 8 ) || ( reinterpret_cast< std::uintptr_t >( p ) % 16 ==
                          0 ) );
   const auto tag = static_cast< unsigned char >( t );
   std::memset( p , tag , n );
   L.push_back( { p , n , tag } );
   }
  else {
   const std::size_t i = g() % L.size();
   for( std::size_t k = 0 ; k < L[ i ].n ; ++k )
    if( L[ i ].p[ k ] != L[ i ].tag ) {
     CHECK( false );
     break;
     }
   A.deallocate( L[ i ].p , L[ i ].n );
   L[ i ] = L.back();
   L.pop_back();
   }
 for( const Live & l : L )
  A.deallocate( l.p , l.n );
 void * q = A.allocate( 100 );               // the last one given back
 A.deallocate( q , 100 );
 CHECK( A.allocate( 100 ) == q );
 S B( std::move( A ) );                      // a move
 CHECK( A.bytes() == 0 && B.bytes() > 0 );
 B.deallocate( q , 100 );
 B.release();
 CHECK( B.bytes() == 0 );
 void * r = B.allocate( 7 );                 // usable after release()
 B.deallocate( r , 7 );
 }

// the raw slots of ObjectPool: allocate() gives a slot not built, the
// caller builds and destroys its object, deallocate() takes it back for
// the next allocate() or make(), the slot given back last first; size()
// counts the slots given
ADS_TEST( "pool-raw-slots" )
{
 ObjectPool< std::string > P;
 std::string * a = P.allocate();
 ::new( static_cast< void * >( a ) ) std::string( 50 , 'a' );
 std::string * b = P.make( 40 , 'b' );
 CHECK( P.size() == 2 && *a == std::string( 50 , 'a' ) );
 std::destroy_at( a );
 P.deallocate( a );
 CHECK( P.size() == 1 );
 std::string * c = P.make( "c" );            // the slot of a again
 CHECK( c == a && *c == "c" );
 P.free( b );
 P.free( c );
 CHECK( P.size() == 0 );
 CHECK_THROWS( P.deallocate( nullptr ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File memory.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
