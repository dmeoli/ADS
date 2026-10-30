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
 * the distributed minimum, in rounds and while the processes poll; the
 * messages of bytes with tags (empty, longer than a batch), those of a
 * codec (strings, vectors) and two kinds in a variant; the stop by one
 * process, by all, by the last one and by none, every message handed once
 * by a poll or by leftovers().
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
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <variant>
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

namespace {

/// the bytes of the message k of the process q: its length and its content
std::vector< std::byte > bytesOf( std::size_t q , std::uint64_t k )
{
 const std::size_t n = ( k % 50 == 7 ) ? 100000 : std::size_t( k * 7 % 40 );
 std::vector< std::byte > b( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  b[ i ] = std::byte( ( q * 31 + k + i ) & 0xFF );
 return( b );
 }

/// a checksum of the bytes of a message of tag t
std::uint64_t sumOf( std::uint32_t t , std::span< const std::byte > b )
{
 std::uint64_t s = 1469598103934665603ull ^ t;
 for( const std::byte x : b )
  s = ( s ^ std::uint64_t( x ) ) * 1099511628211ull;
 return( s ^ ( std::uint64_t( b.size() ) << 40 ) );
 }

}  // end( namespace )

// messages of bytes with three tags among the processes, each to a random
// process (itself included): empty messages, messages longer than a batch,
// batches of 64 bytes and of 64 KB; the counts and the checksums of each
// tag known in advance
ADS_TEST( "mpi-bytes" )
{
 MPIExecutor ex;
 const std::size_t P = ex.size() , me = ex.rank();
 for( std::size_t b : { 64 , 65536 } ) {
  const std::uint64_t N = 700;
  std::uint64_t expect[ 3 ] = { 0 , 0 , 0 } , sumExpect[ 3 ] = { 0 , 0 , 0 };
  for( std::size_t q = 0 ; q < P ; ++q ) {
   std::mt19937_64 g( 13 * q + b );
   for( std::uint64_t k = 0 ; k < N ; ++k )
    if( g() % P == me ) {
     const auto t = std::uint32_t( k % 3 );
     ++expect[ t ];
     sumExpect[ t ] += sumOf( t , bytesOf( q , k ) );
     }
   }
  MPIByteMailbox mb( ex.comm() , b );
  std::uint64_t got[ 3 ] = { 0 , 0 , 0 } , sum[ 3 ] = { 0 , 0 , 0 };
  bool bad = false;
  auto take = [ & ]( std::uint32_t t , std::span< const std::byte > x ) {
   if( t >= 3 ) {
    bad = true;
    return;
    }
   ++got[ t ];
   sum[ t ] += sumOf( t , x );
   };
  std::mt19937_64 g( 13 * me + b );
  for( std::uint64_t k = 0 ; k < N ; ++k ) {
   mb.send( std::size_t( g() % P ) , std::uint32_t( k % 3 ) ,
            bytesOf( me , k ) );
   if( k % 37 == 0 )
    mb.poll( take );
   }
  while( ! mb.quiescent() )
   mb.poll( take );
  CHECK( ! bad );
  for( int t = 0 ; t < 3 ; ++t ) {
   CHECK( got[ t ] == expect[ t ] );
   CHECK( sum[ t ] == sumExpect[ t ] );
   }
  CHECK( mb.sent() == N );
  CHECK( mb.received() == expect[ 0 ] + expect[ 1 ] + expect[ 2 ] );
  CHECK( mb.leftovers( take ) == 0 );
  }
 }

namespace {

/// two kinds of trivially copyable messages
struct Ping {
 std::uint64_t id;
 };
struct Pair {
 std::uint32_t a , b;
 };

}  // end( namespace )

// messages that are not trivially copyable, by MPICodec: strings (some
// empty) and vectors of integers; two kinds in a variant on the typed path
ADS_TEST( "mpi-codec" )
{
 MPIExecutor ex;
 const std::size_t P = ex.size() , me = ex.rank();
 const std::uint64_t N = 500;
 auto text = []( std::size_t q , std::uint64_t k ) {
  return( std::string( std::size_t( k % 9 ) , char( 'a' + ( q + k ) % 26 ) )
          + std::to_string( q * 1000 + k ) );
  };
 std::uint64_t expect = 0 , lenExpect = 0;
 for( std::size_t q = 0 ; q < P ; ++q )
  for( std::uint64_t k = 0 ; k < N ; ++k )
   if( ( q + k ) % P == me ) {
    ++expect;
    lenExpect += text( q , k ).size();
    }
 {
  MPIMailbox< std::string > mb( ex.comm() , 128 );
  std::uint64_t got = 0 , len = 0;
  bool bad = false;
  auto take = [ & ]( std::string && s ) {
   ++got;
   len += s.size();
   // the number at the end names the sender and the message
   const std::size_t at = s.find_first_of( "0123456789" );
   const std::uint64_t id = std::stoull( s.substr( at ) );
   if( s != text( id / 1000 , id % 1000 ) )
    bad = true;
   };
  for( std::uint64_t k = 0 ; k < N ; ++k ) {
   mb.send( ( me + k ) % P , text( me , k ) );
   if( k % 11 == 0 )
    mb.poll( take );
   }
  while( ! mb.quiescent() )
   mb.poll( take );
  CHECK( ! bad );
  CHECK( ( got == expect ) && ( len == lenExpect ) );
  CHECK( mb.leftovers( take ) == 0 );
  }
 {
  MPIMailbox< std::vector< std::uint32_t > > mb( ex.comm() , 256 );
  std::uint64_t got = 0 , sum = 0 , sumExpect = 0;
  for( std::size_t q = 0 ; q < P ; ++q )
   for( std::uint64_t k = 0 ; k < N ; ++k )
    if( ( q + k ) % P == me )
     sumExpect += ( k % 5 ) * ( q + k );
  auto take = [ & ]( std::vector< std::uint32_t > && v ) {
   ++got;
   for( auto x : v )
    sum += x;
   };
  for( std::uint64_t k = 0 ; k < N ; ++k )
   mb.send( ( me + k ) % P ,
            std::vector< std::uint32_t >( k % 5 , std::uint32_t( me + k ) ) );
  while( ! mb.quiescent() )
   mb.poll( take );
  CHECK( ( got == expect ) && ( sum == sumExpect ) );
  }
 {
  using Kind = std::variant< Ping , Pair >;
  MPIMailbox< Kind > mb( ex.comm() , 16 );
  std::uint64_t pings = 0 , pairs = 0 , sum = 0 , sumExpect = 0;
  for( std::size_t q = 0 ; q < P ; ++q )
   for( std::uint64_t k = 0 ; k < N ; ++k )
    if( ( q + k ) % P == me )
     sumExpect += ( k % 2 ) ? k : ( q + 2 * k );
  for( std::uint64_t k = 0 ; k < N ; ++k )
   if( k % 2 )
    mb.send( ( me + k ) % P , Kind( Ping{ k } ) );
   else
    mb.send( ( me + k ) % P , Kind( Pair{ std::uint32_t( me ) ,
                                           std::uint32_t( 2 * k ) } ) );
  auto take = [ & ]( const Kind & x ) {
   if( const auto * p = std::get_if< Ping >( &x ) ) {
    ++pings;
    sum += p->id;
    }
   else {
    ++pairs;
    sum += std::get< Pair >( x ).a + std::get< Pair >( x ).b;
    }
   };
  while( ! mb.quiescent() )
   mb.poll( take );
  CHECK( ( pings + pairs == expect ) && ( sum == sumExpect ) );
  }
 }

namespace {

/// the traffic of N random messages per process over a mailbox M of
/// batches of b, stopped by the processes of the mask `who` (bit q for the
/// rank q) after `after` sends of theirs; every message sent is handed once,
/// by a poll or by leftovers(), as the counts and the sums over all the
/// processes tell
template< class M , class Make , class Key >
bool stopRun( std::size_t b , std::uint64_t who , std::uint64_t after ,
              Make make , Key key )
{
 MPIExecutor ex;
 const std::size_t P = ex.size() , me = ex.rank();
 const std::uint64_t N = 3000;
 M mb( ex.comm() , b );
 std::uint64_t sends = 0 , sendSum = 0 , got = 0 , gotSum = 0;
 auto take = [ & ]( const auto & x ) {
  ++got;
  gotSum += key( x );
  };
 const bool stopper = ( me < 64 ) && ( ( who >> me ) & 1 );
 std::mt19937_64 g( 17 * me + b );
 for( std::uint64_t k = 0 ; ( k < N ) && ! mb.stopped() ; ++k ) {
  const std::uint64_t id = me * N + k;
  mb.send( std::size_t( g() % P ) , make( id ) );
  ++sends;
  sendSum += id;
  if( stopper && ( sends == after ) )
   mb.stop();
  if( k % 13 == 0 )
   mb.poll( take );
  }
 while( ! mb.quiescent() )
  mb.poll( take );
 const bool stopped = mb.stopped();
 const std::uint64_t polled = got;
 const std::size_t left = mb.leftovers( take );
 const bool ok = ( left == got - polled ) && ( mb.terminated() ) &&
                 ( sumAll( sends ) == sumAll( got ) ) &&
                 ( sumAll( sendSum ) == sumAll( gotSum ) ) &&
                 ( ( who == 0 ) ? ( ( left == 0 ) && ! stopped ) : true ) &&
                 ( stopped || ! ( stopper && ( sends >= after ) ) );
 return( sumAll( ok ? 0 : 1 ) == 0 );
 }

}  // end( namespace )

// the stop: by the process 0 after few and many sends, by all at once, by
// the last one, and no stop; on the typed path, on the bytes and on a codec
ADS_TEST( "mpi-stop" )
{
 MPIExecutor ex;
 const std::uint64_t last = std::uint64_t( 1 ) << ( ex.size() - 1 );
 for( std::size_t b : { 1 , 64 } )
  for( auto [ who , after ] : { std::pair< std::uint64_t ,
                                           std::uint64_t >( 1 , 1 ) ,
                                { 1 , 500 } , { ~0ull , 100 } ,
                                { last , 2000 } , { 0 , 0 } } ) {
   CHECK( ( stopRun< MPIMailbox< std::uint64_t > >( b , who , after ,
             []( std::uint64_t id ) { return( id ); } ,
             []( std::uint64_t x ) { return( x ); } ) ) );
   CHECK( ( stopRun< MPIMailbox< std::string > >( b * 16 , who , after ,
             []( std::uint64_t id ) { return( std::to_string( id ) ); } ,
             []( const std::string & s ) {
              return( std::uint64_t( std::stoull( s ) ) ); } ) ) );
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
