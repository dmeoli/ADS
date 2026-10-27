/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_mpi.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the sort over the processes of MPI, beyond the book, at the
 * level 3 of the contracts; not in tests.txt, since it needs MPI: CMake
 * builds it with ADS_WITH_MPI and runs it on 1, 2, 3 and 4 processes, the
 * makefile by make test-mpi. Each case gathers the parts of all the
 * processes in the order of their ranks and compares them with the items
 * of all sorted by std::sort: random keys, all equal (the runs of equal
 * splitters), few distinct, processes with no item, one process with all
 * of them, sizes that differ among the processes, records sorted by a key
 * of theirs; with one thread per process and with a pool of 2.
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

#include "ADS/distributed/DistributedSort.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// the vectors of all the processes, concatenated in the order of the
/// ranks, on every process
template< class T >
std::vector< T > gatherAll( const std::vector< T > & A )
{
 int P = 1;
 MPI_Comm_size( MPI_COMM_WORLD , &P );
 const int bytes = int( A.size() * sizeof( T ) );
 std::vector< int > count( static_cast< std::size_t >( P ) ) ,
                    at( static_cast< std::size_t >( P ) , 0 );
 MPI_Allgather( &bytes , 1 , MPI_INT , count.data() , 1 , MPI_INT ,
                MPI_COMM_WORLD );
 int total = 0;
 for( int q = 0 ; q < P ; ++q ) {
  at[ std::size_t( q ) ] = total;
  total += count[ std::size_t( q ) ];
  }
 std::vector< T > all( std::size_t( total ) / sizeof( T ) );
 MPI_Allgatherv( A.data() , bytes , MPI_BYTE , all.data() , count.data() ,
                 at.data() , MPI_BYTE , MPI_COMM_WORLD );
 return( all );
 }

struct Rec {
 std::uint32_t key , pos;
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "distributed-sort" )
{
 int P = 1 , me = 0;
 MPI_Comm_size( MPI_COMM_WORLD , &P );
 MPI_Comm_rank( MPI_COMM_WORLD , &me );
 ThreadPool pool( 2 );
 for( int threads : { 1 , 2 } )
  for( int pattern = 0 ; pattern < 6 ; ++pattern ) {
   std::mt19937_64 g( 100 * unsigned( me ) + unsigned( pattern ) );
   std::size_t n = 50000 + 1000 * std::size_t( me );
   if( ( pattern == 3 ) && ( me % 2 == 1 ) ) n = 0;   // some empty
   if( pattern == 4 ) n = me == 0 ? 120000 : 0;       // all on one
   std::vector< std::uint64_t > A( n );
   for( auto & x : A )
    x = pattern == 1 ? 7 : ( pattern == 2 ? g() % 5 : g() );
   const auto before = gatherAll( A );
   if( threads == 1 )
    distributed::sort( MPI_COMM_WORLD , A );
   else
    distributed::sort( pool , MPI_COMM_WORLD , A );
   CHECK( std::is_sorted( A.begin() , A.end() ) );
   auto after = gatherAll( A );
   auto ref = before;
   std::sort( ref.begin() , ref.end() );
   CHECK( after == ref );
   // records by key, with a comparison of their own
   if( pattern == 5 ) {
    std::vector< Rec > R( n );
    for( std::size_t i = 0 ; i < n ; ++i )
     R[ i ] = Rec{ std::uint32_t( g() % 1000 ) , std::uint32_t( i ) };
    auto by = []( const Rec & a , const Rec & b ) { return( a.key < b.key ); };
    const auto rb = gatherAll( R );
    distributed::sort( MPI_COMM_WORLD , R , by );
    const auto ra = gatherAll( R );
    CHECK( ( ra.size() == rb.size() ) &&
           std::is_sorted( ra.begin() , ra.end() , by ) );
    std::vector< std::uint64_t > ka , kb;
    for( const auto & r : ra ) ka.push_back( r.key );
    for( const auto & r : rb ) kb.push_back( r.key );
    std::sort( kb.begin() , kb.end() );
    CHECK( ka == kb );
    }
   }
 }

// the bytes of a process past the counts of int of MPI: a piece, a
// displacement or the bytes received beyond INT_MAX do not fit unless the
// counts are those of 64 bits of MPI 4, and then sort() stops in every
// process instead of passing MPI a number wrapped around
ADS_TEST( "distributed-sort-counts" )
{
 using distributed::detail::fits;
 const std::size_t top = std::size_t( std::numeric_limits< int >::max() );
 bool ok = true;
 CHECK( fits< int >( 0 , ok ) == 0 && ok );
 CHECK( fits< int >( top , ok ) == std::numeric_limits< int >::max() && ok );
 CHECK( fits< int >( top + 1 , ok ) == 0 && ! ok );
 ok = true;
 CHECK( fits< int >( std::size_t( 3 ) << 31 , ok ) == 0 && ! ok );
 ok = true;
 using Count = distributed::detail::Count;
 using Displacement = distributed::detail::Displacement;
 const bool large = sizeof( Count ) >= 8;
 CHECK( large == ( sizeof( Displacement ) >= 8 ) );
 const auto c = fits< Count >( top + 1 , ok );
 const auto d = fits< Displacement >( top + 1 , ok );
 CHECK( ok == large );
 CHECK( large ? ( std::size_t( c ) == top + 1 &&
                  std::size_t( d ) == top + 1 ) : ( c == 0 && d == 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 int provided = 0;
 MPI_Init_thread( &argc , &argv , MPI_THREAD_FUNNELED , &provided );
 const int r = ADS::test::main( argc , argv );
 MPI_Finalize();
 return( r );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_mpi.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
