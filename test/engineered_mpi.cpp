/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_mpi.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the sort and of the runtime over the processes of MPI, beyond
 * the book, at the level 3 of the contracts; not in tests.txt, since it
 * needs MPI: CMake builds it with ADS_WITH_MPI and runs it on 1, 2, 3 and 4
 * processes, the makefile by make test-mpi. Each case of the sort gathers
 * the parts of all the processes in the order of their ranks and compares
 * them with the items of all sorted by std::sort: random keys, all equal
 * (the runs of equal splitters), few distinct, processes with no item, one
 * process with all of them, sizes that differ among the processes, records
 * sorted by a key of theirs; with one thread per process and with a pool
 * of 2. The runtime: the executor of the processes; the mailbox under
 * random traffic of known counts, batches of 1 and of many messages,
 * messages of a process to itself; the termination of random trees spread
 * over the processes by hashing, against their sequential count, with no
 * process seeing the end before the last node, and with no work at all;
 * the distributed minimum, in rounds and while the processes poll.
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
#include "ADS/distributed/MPIExecutor.h"
#include "ADS/distributed/MPIMailbox.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
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

/// the mixing of 64 bits of SplitMix64
std::uint64_t mix( std::uint64_t x )
{
 x += 0x9E3779B97F4A7C15ull;
 x = ( x ^ ( x >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
 x = ( x ^ ( x >> 27 ) ) * 0x94D049BB133111EBull;
 return( x ^ ( x >> 31 ) );
 }

/// a node of a random tree: its key and its depth
struct Node {
 std::uint64_t key;
 std::uint32_t depth;
 };

/// the children of a node of the random tree of the seed, 0 to 4 of them
/// above the depth d
template< class F >
void children( const Node & v , std::uint32_t d , std::uint64_t seed ,
               F && f )
{
 if( v.depth >= d )
  return;
 const std::uint64_t h = mix( v.key ^ seed );
 const unsigned c = unsigned( h & 3 ) + ( v.depth < 3 ? 1 : 0 );
 for( unsigned j = 0 ; j < c ; ++j )
  f( Node{ mix( h + j + 1 ) , v.depth + 1 } );
 }

/// the nodes of the random tree of the seed, counted by one process
std::uint64_t treeSize( std::uint32_t d , std::uint64_t seed )
{
 std::vector< Node > stack{ Node{ seed , 0 } };
 std::uint64_t n = 0;
 while( ! stack.empty() ) {
  const Node v = stack.back();
  stack.pop_back();
  ++n;
  children( v , d , seed , [ & ]( Node c ) { stack.push_back( c ); } );
  }
 return( n );
 }

/// the sum over the processes
std::uint64_t sumAll( std::uint64_t x )
{
 std::uint64_t s = 0;
 MPI_Allreduce( &x , &s , 1 , MPI_UINT64_T , MPI_SUM , MPI_COMM_WORLD );
 return( s );
 }

/// the random tree of the seed explored by the processes over a mailbox
/// with batches of b messages; true if no process saw the end before the
/// last node, and the nodes are those of the tree
bool mailboxTree( std::size_t b , std::uint32_t d , std::uint64_t seed ,
                  bool root )
{
 MPIExecutor ex;
 const std::size_t P = ex.size() , me = ex.rank();
 MPIMailbox< Node > mb( ex.comm() , b );
 std::vector< Node > stack;
 if( root && ( ownerOf( seed , P ) == me ) )
  stack.push_back( Node{ seed , 0 } );
 std::uint64_t done = 0;
 while( true ) {
  mb.poll( [ & ]( const Node & v ) { stack.push_back( v ); } );
  if( ! stack.empty() ) {
   for( int j = 0 ; ( j < 16 ) && ! stack.empty() ; ++j ) {
    const Node v = stack.back();
    stack.pop_back();
    ++done;
    children( v , d , seed , [ & ]( Node c ) {
     const std::size_t o = ownerOf( c.key , P );
     if( o == me )
      stack.push_back( c );
     else
      mb.send( o , c );
     } );
    }
   continue;
   }
  if( mb.quiescent() )
   break;
  }
 // nothing may come after the end
 const std::uint64_t atEnd = done;
 const std::uint64_t late = mb.poll( []( const Node & ) {} );
 const std::uint64_t n = root ? treeSize( d , seed ) : 0;
 const bool ok = ( late == 0 ) && ( mb.waves() >= 2 ) &&
                 ( sumAll( atEnd ) == n ) &&
                 ( sumAll( mb.sent() ) == sumAll( mb.received() ) );
 return( sumAll( ok ? 0 : 1 ) == 0 );
 }

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

// the executor of the processes: rank and size as MPI gives them, one
// call of run() per process, and a barrier
ADS_TEST( "mpi-executor" )
{
 MPIExecutor ex;
 int P = 0 , me = 0;
 MPI_Comm_size( MPI_COMM_WORLD , &P );
 MPI_Comm_rank( MPI_COMM_WORLD , &me );
 CHECK( ex.size() == std::size_t( P ) );
 CHECK( ex.rank() == std::size_t( me ) );
 std::size_t calls = 0 , who = 99;
 ex.run( [ & ]( std::size_t i ) { ++calls; who = i; } );
 CHECK( ( calls == 1 ) && ( who == ex.rank() ) );
 ex.barrier();
 CHECK( sumAll( calls ) == std::uint64_t( P ) );
 }

// random traffic among the processes, each message to a random process
// (itself included), the counts known in advance; batches of 1, 5 and 256
ADS_TEST( "mpi-mailbox" )
{
 MPIExecutor ex;
 const std::size_t P = ex.size() , me = ex.rank();
 for( std::size_t b : { 1 , 5 , 256 } ) {
  const std::uint64_t N = 2000;
  // the messages each process expects, from the generators of all
  std::uint64_t expect = 0 , sumExpect = 0;
  for( std::size_t q = 0 ; q < P ; ++q ) {
   std::mt19937_64 g( 11 * q + b );
   for( std::uint64_t k = 0 ; k < N ; ++k ) {
    const std::uint64_t to = g() % P;
    if( to == me ) {
     ++expect;
     sumExpect += q * N + k;
     }
    }
   }
  MPIMailbox< std::uint64_t > mb( ex.comm() , b );
  std::uint64_t got = 0 , sum = 0;
  auto take = [ & ]( const std::uint64_t & x ) { ++got; sum += x; };
  std::mt19937_64 g( 11 * me + b );
  for( std::uint64_t k = 0 ; k < N ; ++k ) {
   mb.send( std::size_t( g() % P ) , me * N + k );
   if( k % 97 == 0 )
    mb.poll( take );
   }
  while( ! mb.quiescent() )
   mb.poll( take );
  CHECK( got == expect );
  CHECK( sum == sumExpect );
  CHECK( mb.sent() == N );
  CHECK( mb.received() == expect );
  }
 }

// the termination of random trees spread over the processes, with batches
// of 1, 8 and 256; with no root, no work at all
ADS_TEST( "mpi-termination" )
{
 for( std::size_t b : { 1 , 8 , 256 } ) {
  CHECK( mailboxTree( b , 10 , 7 , false ) );
  for( std::uint64_t seed : { 1 , 2 , 3 } )
   CHECK( mailboxTree( b , 12 , seed , true ) );
  }
 }

// the distributed minimum: rounds of wait(), a round tested while the
// processes poll, values of integers and of doubles
ADS_TEST( "mpi-min" )
{
 MPIExecutor ex;
 const long me = long( ex.rank() ) , P = long( ex.size() );
 DistributedMin< long > m;
 CHECK( ! m.test().has_value() );
 for( long round = 0 ; round < 5 ; ++round ) {
  CHECK( m.start( 100 - round + ( me * 7 ) % 13 ) );
  CHECK( ! m.start( 0 ) );                 // one in flight
  long low = 100 - round + 0;
  for( long q = 0 ; q < P ; ++q )
   low = std::min( low , 100 - round + ( q * 7 ) % 13 );
  CHECK( m.wait() == low );
  CHECK( ! m.inFlight() );
  }
 DistributedMin< double > d;
 CHECK( d.start( 0.5 + double( P - 1 - me ) ) );
 std::optional< double > r;
 while( ! ( r = d.test() ) )
  ;
 CHECK( *r == 0.5 );
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
