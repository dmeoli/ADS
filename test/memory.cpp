/*--------------------------------------------------------------------------*/
/*---------------------------- File memory.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered memory beyond the arena and the pool: the large
 * blocks kept by a thread for the requests of the same size, at the level
 * 3 of the contracts.
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

#include <cstdint>
#include <cstring>
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

// the large blocks given back are kept by the thread and given again to a
// request of the same size, aligned to a huge page, with their contents
// writable; a request of another size takes a new block; no more than 16
// blocks and 64 MB in all are kept; a small block is not kept; another
// thread keeps its own blocks, freed when it ends; a vector and a pool
// built twice reuse the blocks
ADS_TEST( "huge-page-cache" )
{
 using A = HugePageAllocator< std::uint64_t >;
 A a;
 const std::size_t n = ( std::size_t( 4 ) << 20 ) / 8;   // 4 MB
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
