/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_runtime.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the runtime of the long-lived workers, beyond the book, at the
 * level 3 of the contracts: run() and the locks of every executor compiled
 * in (the workers meet at a barrier, so that a run whose calls did not all
 * run at the same time fails instead of hanging; a job nested in a worker;
 * no worker, one worker, an exception), the families of channels (one
 * worker sending to itself, empty batches, a mesh of capacity 2 that takes
 * part of a batch, the order of the messages of each pair of workers, the
 * counts of random traffic), the farm with feedback (no task, a tree of
 * tasks against its sequential sum, a master that stops it, the farm of no
 * worker, an exception of a worker), and the mailbox with the detection of
 * the termination on every family and batch (no work at all, all the work
 * on one worker, random trees spread by hashing, against their sequential
 * count, checking that no worker sees the end before the last node).
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

#include "ADS/parallel/Channels.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"
#include "ADS/parallel/Farm.h"
#include "ADS/parallel/Mailbox.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

#if defined( ADS_WITH_TBB )
/// the threads that the tests of TBB may ask for, at most 4
std::size_t tbbThreads( void )
{
 return( std::clamp< std::size_t >( std::thread::hardware_concurrency() ,
                                    1 , 4 ) );
 }
#endif

/// f( ex ) on each executor with workers: Sequential, pools of 1, 2, 3 and
/// 8 threads, a pool whose workers sleep at once, and the executors of the
/// libraries compiled in
template< class F >
void onEach( F && f )
{
 Sequential s;
 f( s );
 for( std::size_t p : { 1 , 2 , 3 , 8 } ) {
  ThreadPool pool( p );
  f( pool );
  }
 {
  ThreadPool lazy( 4 , 0 );
  f( lazy );
  }
#if defined( _OPENMP )
 OpenMPExecutor omp( 4 );
 f( omp );
#endif
#if defined( ADS_WITH_FASTFLOW )
 FastFlowExecutor ff( 3 , false );
 f( ff );
#endif
#if defined( ADS_WITH_TBB )
 TBBExecutor tbb( tbbThreads() );
 f( tbb );
#endif
 }

/// a barrier of p workers that fails after 20 s instead of hanging
class Meet {
 public:
 explicit Meet( std::size_t p ) : f_p( p ) {}
 void wait( void ) {
  f_in.fetch_add( 1 );
  const auto end = std::chrono::steady_clock::now() +
                   std::chrono::seconds( 20 );
  while( f_in.load() < f_p ) {
   if( std::chrono::steady_clock::now() > end )
    throw( ADS::test::Failure( "the workers did not run together" ) );
   std::this_thread::yield();
   }
  }
 private:
 std::size_t f_p;
 std::atomic< std::size_t > f_in{ 0 };
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

/// the children of a node of the random tree of the seed: 0 to 3 of them
/// above the depth d, none at d
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

/// the nodes of the random tree of the seed, counted on one thread
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

/// the size of an executor with run(): its threads, 1 for Sequential
template< class E >
std::size_t workersOf( E & ex )
{
 return( ex.concurrency() );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the p calls of run() at the same time, each once; fewer workers than
// threads; none; a job nested in each worker; an exception after the
// barrier, thrown again once all have returned; runs one after another
ADS_TEST( "worker-run" )
{
 onEach( []( auto & ex ) {
  static_assert( WorkerExecutor< std::remove_reference_t< decltype( ex ) > > );
  const std::size_t P = workersOf( ex );
  for( std::size_t p : { P , std::size_t( 1 ) , std::size_t( 0 ) ,
                         ( P + 1 ) / 2 } )
   for( int round = 0 ; round < 3 ; ++round ) {
    Meet meet( p );
    std::vector< std::atomic< int > > calls( p );
    std::vector< long > sums( p , 0 );
    ex.run( p , [ & ]( std::size_t i ) {
     calls[ i ].fetch_add( 1 );
     meet.wait();
     // a job nested in the worker, on the same executor
     sums[ i ] = parallelReduce( ex , 0 , 5000 , 0L ,
                                 [ & ]( std::size_t lo , std::size_t hi ) {
                                  long s = 0;
                                  for( std::size_t j = lo ; j < hi ; ++j )
                                   s += long( j + i );
                                  return( s ); } ,
                                 std::plus<>() , 64 );
     } );
    for( std::size_t i = 0 ; i < p ; ++i ) {
     CHECK( calls[ i ].load() == 1 );
     CHECK( sums[ i ] == 4999L * 5000L / 2 + 5000L * long( i ) );
     }
    }
  if( P >= 2 ) {
   Meet meet( P );
   std::atomic< std::size_t > done{ 0 };
   CHECK_THROWS( ex.run( P , [ & ]( std::size_t i ) {
                  meet.wait();
                  if( i == 1 )
                   throw( std::runtime_error( "worker" ) );
                  done.fetch_add( 1 );
                  } ) , std::runtime_error );
   CHECK( done.load() == P - 1 );
   }
  } );
 }

// the lock of each executor: increments of a shared counter by all the
// workers, none lost
ADS_TEST( "worker-locks" )
{
 onEach( []( auto & ex ) {
  using E = std::remove_reference_t< decltype( ex ) >;
  typename E::lock_type lock;
  const std::size_t p = workersOf( ex );
  long counter = 0;
  ex.run( p , [ & ]( std::size_t ) {
   for( int k = 0 ; k < 20000 ; ++k ) {
    std::lock_guard< typename E::lock_type > g( lock );
    ++counter;
    }
   } );
  CHECK( counter == 20000L * long( p ) );
  } );
}

namespace {

/// a message of the tests of the channels: its sender, its receiver and
/// its number among those of the pair
struct Msg {
 std::uint32_t from = 0;
 std::uint32_t to = 0;
 std::uint64_t seq = 0;
 };

/// one worker: empty batches, a push to itself, a drain of nothing; the
/// bounded mesh pushes what fits
template< class C >
void channelsAlone( C & c )
{
 const std::vector< Msg > none;
 CHECK( c.tryPush( 0 , 0 , std::span< const Msg >( none ) ) == 0 );
 CHECK( c.drain( 0 , []( Msg && ) { throw( 0 ); } ) == 0 );
 std::vector< Msg > m( 5 );
 for( std::size_t i = 0 ; i < m.size() ; ++i )
  m[ i ].seq = i;
 const std::size_t k = c.tryPush( 0 , 0 , std::span< const Msg >( m ) );
 CHECK( C::bounded ? ( k >= 1 ) : ( k == 5 ) );
 std::uint64_t next = 0;
 auto port = c.port( 0 );
 CHECK( port.drain( [ & ]( Msg && x ) { CHECK( x.seq == next++ ); } ) == k );
 CHECK( next == k );
 CHECK( port.drain( []( Msg && ) {} ) == 0 );
 }

/// p workers on a pool send random batches to random workers, retrying
/// what a bounded family refuses and draining while they send, until each
/// has received all its messages; the messages of each pair in order
template< class C , class ... A >
void channelsTraffic( std::size_t p , std::uint64_t seed , A ... args )
{
 C c( p , args... );
 const std::size_t N = 3000;
 // the messages of each pair, known in advance
 std::vector< std::vector< std::uint32_t > > dest( p );
 std::vector< std::uint64_t > expect( p , 0 );
 for( std::size_t i = 0 ; i < p ; ++i ) {
  std::mt19937_64 g( seed + i );
  for( std::size_t k = 0 ; k < N ; ++k ) {
   dest[ i ].push_back( std::uint32_t( g() % p ) );
   ++expect[ dest[ i ].back() ];
   }
  }
 ThreadPool pool( p );
 std::vector< std::vector< std::uint64_t > > seen( p ,
                                   std::vector< std::uint64_t >( p , 0 ) );
 std::atomic< bool > bad{ false };
 pool.run( p , [ & ]( std::size_t me ) {
  std::mt19937_64 g( seed * 7 + me );
  std::vector< std::uint64_t > seq( p , 0 );
  std::uint64_t got = 0;
  auto take = [ & ]( Msg && x ) {
   if( ( x.to != me ) || ( x.seq != seen[ me ][ x.from ]++ ) )
    bad.store( true );
   ++got;
   };
  std::size_t k = 0;
  std::vector< Msg > batch;
  while( k < N ) {
   // a batch of 0 to 7 messages to the destination of the next one
   const std::uint32_t to = dest[ me ][ k ];
   const std::size_t b = std::size_t( g() % 8 );
   batch.clear();
   while( ( batch.size() < b ) && ( k < N ) && ( dest[ me ][ k ] == to ) ) {
    batch.push_back( Msg{ std::uint32_t( me ) , to , seq[ to ]++ } );
    ++k;
    }
   std::size_t sent = 0;
   while( sent < batch.size() ) {
    sent += c.tryPush( me , to , std::span< const Msg >( batch ).subspan(
                                                                 sent ) );
    c.drain( me , take );
    }
   if( b == 0 )
    c.drain( me , take );
   }
  const auto end = std::chrono::steady_clock::now() +
                   std::chrono::seconds( 20 );
  while( got < expect[ me ] ) {
   if( c.drain( me , take ) == 0 ) {
    if( std::chrono::steady_clock::now() > end ) {
     bad.store( true );
     return;
     }
    std::this_thread::yield();
    }
   }
  } );
 CHECK( ! bad.load() );
 for( std::size_t i = 0 ; i < p ; ++i ) {
  std::uint64_t s = 0;
  for( std::size_t j = 0 ; j < p ; ++j )
   s += seen[ i ][ j ];
  CHECK( s == expect[ i ] );
  }
 }

/// the tests of a family, with the arguments of its constructor
template< class C , class ... A >
void channelsFamily( A ... args )
{
 static_assert( ChannelFamily< C > );
 {
  C c( 1 , args... );
  channelsAlone( c );
  }
 for( std::size_t p : { 1 , 2 , 3 , 5 } )
  for( std::uint64_t seed : { 1 , 2 } )
   channelsTraffic< C >( p , seed , args... );
 }

}  // end( namespace )

// each family alone and under random traffic of 1 to 5 workers
ADS_TEST( "channels" )
{
 channelsFamily< LockedChannels< Msg > >();
 channelsFamily< LockedChannels< Msg , ThreadPool::lock_type > >();
#if defined( _OPENMP )
 channelsFamily< LockedChannels< Msg , OpenMPLock > >();
#endif
#if defined( ADS_WITH_TBB )
 channelsFamily< LockedChannels< Msg , TBBExecutor::lock_type > >();
#endif
#if defined( ADS_WITH_FASTFLOW )
 channelsFamily< LockedChannels< Msg , FastFlowLock > >();
#endif
 channelsFamily< InboxChannels< Msg > >();
 channelsFamily< LaneChannels< Msg > >();
 channelsFamily< LaneChannels< Msg , 2 > >();
 channelsFamily< MeshChannels< Msg > >( std::size_t( 2 ) );
 channelsFamily< MeshChannels< Msg > >( std::size_t( 1024 ) );
 channelsFamily< DefaultChannels< Msg , ThreadPool > >();
 // the bounded mesh refuses what does not fit
 MeshChannels< Msg > mesh( 2 , 2 );
 const std::vector< Msg > five( 5 );
 CHECK( mesh.tryPush( 0 , 1 , std::span< const Msg >( five ) ) == 2 );
 CHECK( mesh.tryPush( 0 , 1 , std::span< const Msg >( five ) ) == 0 );
 CHECK( mesh.tryPush( 1 , 1 , std::span< const Msg >( five ) ) == 2 );
 CHECK( mesh.drain( 1 , []( Msg && ) {} ) == 4 );
 CHECK( mesh.drain( 0 , []( Msg && ) {} ) == 0 );
 }

// the tree of tasks 1 .. L, each task t with the children 2t and 2t + 1,
// against its sum; no task; a master that stops after K results; an
// exception of a worker
ADS_TEST( "feedback-farm" )
{
 onEach( []( auto & ex ) {
#if defined( ADS_WITH_FASTFLOW )
  using E = std::remove_reference_t< decltype( ex ) >;
  constexpr bool ff = std::is_same_v< E , FastFlowExecutor >;
#else
  constexpr bool ff = false;
#endif
  const std::size_t P = ex.concurrency();
  for( std::size_t w : { std::size_t( 0 ) , ff ? P : P - 1 } ) {
   const long L = 5000;
   long sum = 0 , results = 0;
   feedbackFarm< long , long >( ex , w ,
    []( auto emit ) { emit( 1L ); } ,
    [ & ]( long r , auto emit ) {
     sum += r;
     ++results;
     if( 2 * r <= L )
      emit( 2 * r );
     if( 2 * r + 1 <= L )
      emit( 2 * r + 1 );
     return( true ); } ,
    []( long t ) { return( t ); } );
   CHECK( results == L );
   CHECK( sum == L * ( L + 1 ) / 2 );
   // no task
   bool called = false;
   feedbackFarm< long , long >( ex , w , []( auto ) {} ,
    [ & ]( long , auto ) { called = true; return( true ); } ,
    []( long t ) { return( t ); } );
   CHECK( ! called );
   // stopped after K results, with many tasks left
   const long K = 37;
   results = 0;
   feedbackFarm< long , long >( ex , w ,
    []( auto emit ) { for( long t = 0 ; t < 1000 ; ++t ) emit( t ); } ,
    [ & ]( long , auto ) { return( ++results < K ); } ,
    []( long t ) { return( t + 1 ); } );
   CHECK( results == K );
   // an exception of a worker
   CHECK_THROWS( ( feedbackFarm< long , long >( ex , w ,
    []( auto emit ) { for( long t = 0 ; t < 100 ; ++t ) emit( t ); } ,
    []( long , auto ) { return( true ); } ,
    []( long t ) -> long {
     if( t == 42 )
      throw( std::runtime_error( "work" ) );
     return( t ); } ) ) , std::runtime_error );
   }
  } );
 }

namespace {

/// the random tree of the seed explored by the p workers of ex over the
/// mailbox of the channels C with batches of b messages, each node sent to
/// the worker that owns its key; every worker checks, when it sees the
/// end, that all the nodes were explored
template< class C , class E , class ... A >
void mailboxTree( E & ex , std::size_t p , std::size_t b , std::uint32_t d ,
                  std::uint64_t seed , bool root , A ... args )
{
 const std::uint64_t n = root ? treeSize( d , seed ) : 0;
 Mailbox< Node , C > mb( p , b , args... );
 std::atomic< std::uint64_t > done{ 0 };
 std::vector< std::uint64_t > atEnd( p , ~0ull );
 ex.run( p , [ & ]( std::size_t me ) {
  std::vector< Node > stack;
  if( root && ( ownerOf( seed , p ) == me ) )
   stack.push_back( Node{ seed , 0 } );
  unsigned k = 0;
  for( ;; ) {
   mb.poll( me , [ & ]( Node && v ) { stack.push_back( v ); } );
   if( ! stack.empty() ) {
    for( int j = 0 ; ( j < 16 ) && ! stack.empty() ; ++j ) {
     const Node v = stack.back();
     stack.pop_back();
     done.fetch_add( 1 , std::memory_order_relaxed );
     children( v , d , seed , [ & ]( Node c ) {
      const std::size_t o = ownerOf( c.key , p );
      if( o == me )
       stack.push_back( c );
      else
       mb.send( me , o , c );
      } );
     }
    k = 0;
    continue;
    }
   if( mb.quiescent( me ) )
    break;
   detail::spinPause( k );
   }
  atEnd[ me ] = done.load( std::memory_order_relaxed );
  } );
 CHECK( done.load() == n );
 for( std::size_t i = 0 ; i < p ; ++i )
  CHECK( atEnd[ i ] == n );
 CHECK( mb.terminated() );
 std::uint64_t sent = 0 , received = 0;
 for( std::size_t i = 0 ; i < p ; ++i ) {
  sent += mb.sent( i );
  received += mb.received( i );
  }
 CHECK( sent == received );
 }

/// the trees of a family on pools of 1, 2 and 4 workers, batches of 1, 7
/// and 64, with no root and with random roots
template< class C , class ... A >
void mailboxFamily( A ... args )
{
 for( std::size_t p : { 1 , 2 , 4 } ) {
  ThreadPool pool( p );
  for( std::size_t b : { 1 , 7 , 64 } ) {
   mailboxTree< C >( pool , p , b , 12 , 5 , false , args... );
   for( std::uint64_t seed : { 1 , 2 , 3 } )
    mailboxTree< C >( pool , p , b , 12 , seed , true , args... );
   }
  }
 }

}  // end( namespace )

// the termination of the trees on every family, and on every executor with
// the default family of its own
ADS_TEST( "mailbox-termination" )
{
 mailboxFamily< LockedChannels< Node > >();
 mailboxFamily< InboxChannels< Node > >();
 mailboxFamily< LaneChannels< Node > >();
 mailboxFamily< MeshChannels< Node > >( std::size_t( 2 ) );
 mailboxFamily< MeshChannels< Node > >( std::size_t( 1024 ) );
 onEach( []( auto & ex ) {
  using E = std::remove_reference_t< decltype( ex ) >;
  const std::size_t p = ex.concurrency();
  mailboxTree< DefaultChannels< Node , E > >( ex , p , 8 , 12 , 9 , true );
  mailboxTree< DefaultChannels< Node , E > >( ex , p , 8 , 12 , 9 , false );
  } );
 }

// the owners of the keys: in range, all of them used, one worker
ADS_TEST( "owner-of" )
{
 std::set< std::size_t > used;
 for( std::uint64_t k = 0 ; k < 1000 ; ++k ) {
  const std::size_t o = ownerOf( mix( k ) , 7 );
  CHECK( o < 7 );
  used.insert( o );
  CHECK( ownerOf( k , 1 ) == 0 );
  }
 CHECK( used.size() == 7 );
 CHECK( ownerOf( 10 , 4 , []( int k ) { return( std::size_t( k * 3 ) ); } )
        == 2 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_runtime.cpp -------------------*/
/*--------------------------------------------------------------------------*/
