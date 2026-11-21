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
 * count, checking that no worker sees the end before the last node); the
 * try_lock() of every lock, isRunCaller() on every executor, the indices
 * of the workers and of the master of the farm; the stop of the mailbox by
 * a worker and by another thread racing with the sends, every message
 * handed once by a poll or by leftovers(), and the sends that never wait
 * on full queues; the coalescing of the outboxes (the runs of a key closed
 * by a refused merge, the map emptied by a push that takes part of a batch
 * and kept by one that takes nothing, the merges after a stop) and the
 * shortest paths by many workers with merges on every family, against
 * Dijkstra's; the audit at a quiescent point by a Shadow and by a
 * predicate of the consumer, which catch a corrupted state.
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

#include "ADS/Modification.h"
#include "ADS/concurrent/StealablePriorityQueue.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FlatHashTable.h"
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
#include <barrier>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

#if defined( ADS_WITH_TBB )
/// the threads that the tests of TBB may ask for, at most 4
std::size_t tbbThreads( void )
{
 return(
   std::clamp< std::size_t >( std::thread::hardware_concurrency() , 1 , 4 ) );
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
  ThreadPool lazy( 4 , 0 , 0 );  // workers that sleep at once
  f( lazy );
  }
 {
  ThreadPool brief( 4 , 0 , 1000000 );  // a budget from 0 to 1 ms
  f( brief );
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
class Meet
{
 public:
 explicit Meet( std::size_t p ) : f_p( p ) { }
 void wait( void ) {
  f_in.fetch_add( 1 );
  const auto end =
    std::chrono::steady_clock::now() + std::chrono::seconds( 20 );
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
struct Node
{
 std::uint64_t key;
 std::uint32_t depth;
 };

/// the children of a node of the random tree of the seed: 0 to 3 of them
/// above the depth d, none at d
template< class F >
void children( const Node & v , std::uint32_t d , std::uint64_t seed , F && f )
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
  for( std::size_t p :
       { P , std::size_t( 1 ) , std::size_t( 0 ) , ( P + 1 ) / 2 } )
   for( int round = 0 ; round < 3 ; ++round ) {
    Meet meet( p );
    std::vector< std::atomic< int > > calls( p );
    std::vector< long > sums( p , 0 );
    ex.run( p , [ & ]( std::size_t i ) {
     calls[ i ].fetch_add( 1 );
     meet.wait();
     // a job nested in the worker, on the same executor
     sums[ i ] = parallelReduce(
       ex , 0 , 5000 , 0L ,
       [ & ]( std::size_t lo , std::size_t hi ) {
	long s = 0;
	for( std::size_t j = lo ; j < hi ; ++j )
	 s += long( j + i );
	return( s );
        } ,
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
   CHECK_THROWS( ex.run( P ,
			 [ & ]( std::size_t i ) {
			  meet.wait();
			  if( i == 1 )
			   throw( std::runtime_error( "worker" ) );
			  done.fetch_add( 1 );
			  } ) ,
		 std::runtime_error );
   CHECK( done.load() == P - 1 );
   }
  } );
 }

// the lock of each executor: increments of a shared counter by all the
// workers, none lost; try_lock() refused while another thread holds the
// lock and taken once it is free; workers that skip the section when it
// is busy, each success counted once
ADS_TEST( "worker-locks" )
{
 onEach( []( auto & ex ) {
  using E = std::remove_reference_t< decltype( ex ) >;
  using L = typename E::lock_type;
  L lock;
  const std::size_t p = workersOf( ex );
  long counter = 0;
  ex.run( p , [ & ]( std::size_t ) {
   for( int k = 0 ; k < 20000 ; ++k ) {
    std::lock_guard< L > g( lock );
    ++counter;
    }
   } );
  CHECK( counter == 20000L * long( p ) );
  // held by this thread: refused to another one
  lock.lock();
  bool other = true;
  std::thread( [ & ] { other = lock.try_lock(); } ).join();
  CHECK( ! other );
  lock.unlock();
  std::thread( [ & ] {
   other = lock.try_lock();
   if( other )
    lock.unlock();
   } )
    .join();
  CHECK( other );
  CHECK( lock.try_lock() );
  lock.unlock();
  // the scheme that skips a busy section
  long inside = 0;
  std::atomic< long > taken{ 0 } , skipped{ 0 };
  ex.run( p , [ & ]( std::size_t ) {
   for( int k = 0 ; k < 20000 ; ++k )
    if( lock.try_lock() ) {
     ++inside;
     taken.fetch_add( 1 , std::memory_order_relaxed );
     lock.unlock();
     }
    else
     skipped.fetch_add( 1 , std::memory_order_relaxed );
   } );
  CHECK( inside == taken.load() );
  CHECK( taken.load() + skipped.load() == 20000L * long( p ) );
  CHECK( ( p > 1 ) || ( skipped.load() == 0 ) );
  } );
 }

// the two loops of FastFlowExecutor, each built at its first call: bulk()
// and run() in turn on an executor that spins, the spinning threads put to
// sleep before each run; an executor of one worker builds none of them
ADS_TEST( "fastflow-loops" )
{
#if defined( ADS_WITH_FASTFLOW )
 for( bool spin : { true , false } ) {
  FastFlowExecutor ex( 3 , spin );
  for( int round = 0 ; round < 4 ; ++round ) {
   std::vector< long > v( 1000 , 0 );
   ex.bulk( v.size() , [ & ]( std::size_t i ) { v[ i ] = long( i ); } );
   long s = 0;
   for( long x : v )
    s += x;
   CHECK( s == 999L * 1000L / 2 );
   Meet meet( 3 );
   std::vector< int > calls( 3 , 0 );
   ex.run( 3 , [ & ]( std::size_t i ) {
    meet.wait();
    ++calls[ i ];
    } );
   CHECK( ( calls[ 0 ] == 1 ) && ( calls[ 1 ] == 1 ) && ( calls[ 2 ] == 1 ) );
   }
  }
 FastFlowExecutor one( 1 );
 int k = 0;
 one.run( 1 , [ & ]( std::size_t ) { ++k; } );
 one.bulk( 5 , [ & ]( std::size_t ) { ++k; } );
 CHECK( k == 6 );
#endif
 }

// many calls of bulk() of sizes from 2 to beyond the workers, on an executor
// that spins, each of which must wake up all the workers that its barrier
// waits for, and must not change their number
ADS_TEST( "fastflow-bulk-sizes" )
{
#if defined( ADS_WITH_FASTFLOW )
 constexpr std::size_t p = 4;
 FastFlowExecutor ex( p , true );
 std::vector< long > v( 2 * p );
 long s = 0 , expected = 0;
 for( long call = 0 ; call < 200000 ; ++call ) {
  const std::size_t n = 2 + std::size_t( call ) % ( 2 * p - 1 );
  ex.bulk( n , [ & ]( std::size_t i ) { v[ i ] = long( i ) + call; } );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   s += v[ i ];
   expected += long( i ) + call;
   }
  }
 CHECK( s == expected );
#endif
 }

// isRunCaller(): false outside a run; true in the calls that run on the
// caller, i.e., f( 0 ) on Sequential, the pool and OpenMP, none on
// FastFlow with more than one worker, at most one on oneTBB; true again in
// a run nested in a worker, and false once it has returned
ADS_TEST( "run-caller" )
{
 CHECK( ! isRunCaller() );
 onEach( []( auto & ex ) {
  using E [[maybe_unused]] = std::remove_reference_t< decltype( ex ) >;
  const std::size_t P = workersOf( ex );
  for( std::size_t p : { P , std::size_t( 1 ) } ) {
   std::vector< char > caller( p , 0 );
   std::vector< char > nested( p , 0 ) , after( p , 1 );
   Meet meet( p );
   ex.run( p , [ & ]( std::size_t i ) {
    meet.wait();  // all at the same time
    caller[ i ] = isRunCaller() ? 1 : 0;
    const bool before = isRunCaller();
    Sequential s;
    s.run( 1 , [ & ]( std::size_t ) { nested[ i ] = isRunCaller(); } );
    after[ i ] = ( isRunCaller() == before ) ? 1 : 0;
    } );
   CHECK( ! isRunCaller() );
   std::size_t n = 0;
   for( std::size_t i = 0 ; i < p ; ++i ) {
    n += std::size_t( caller[ i ] );
    CHECK( nested[ i ] );
    CHECK( after[ i ] );
    }
#if defined( ADS_WITH_FASTFLOW )
   if constexpr( std::is_same_v< E , FastFlowExecutor > ) {
    CHECK( n == ( p == 1 ? 1u : 0u ) );
    continue;
    }
#endif
#if defined( ADS_WITH_TBB )
   if constexpr( std::is_same_v< E , TBBExecutor > ) {
    CHECK( n <= 1 );
    continue;
    }
#endif
   CHECK( ( n == 1 ) && caller[ 0 ] );
   }
  } );
 CHECK( ! isRunCaller() );
 }

namespace
{

/// a message of the tests of the channels: its sender, its receiver and
/// its number among those of the pair
struct Msg
{
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
 CHECK( port.drain( []( Msg && ) { } ) == 0 );
 }

/// p workers on a pool send random batches to random workers, retrying
/// what a bounded family refuses and draining while they send, until each
/// has received all its messages; the messages of each pair in order
template< class C , class... A >
void channelsTraffic( std::size_t p , std::uint64_t seed , A... args )
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
 std::vector< std::vector< std::uint64_t > > seen(
   p , std::vector< std::uint64_t >( p , 0 ) );
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
    sent +=
      c.tryPush( me , to , std::span< const Msg >( batch ).subspan( sent ) );
    c.drain( me , take );
    }
   if( b == 0 )
    c.drain( me , take );
   }
  const auto end =
    std::chrono::steady_clock::now() + std::chrono::seconds( 20 );
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
template< class C , class... A >
void channelsFamily( A... args )
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
 CHECK( mesh.drain( 1 , []( Msg && ) { } ) == 4 );
 CHECK( mesh.drain( 0 , []( Msg && ) { } ) == 0 );
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
   feedbackFarm< long , long >(
     ex , w , []( auto emit ) { emit( 1L ); } ,
     [ & ]( long r , auto emit ) {
      sum += r;
      ++results;
      if( 2 * r <= L )
       emit( 2 * r );
      if( 2 * r + 1 <= L )
       emit( 2 * r + 1 );
      return( true );
      } ,
     []( long t ) { return( t ); } );
   CHECK( results == L );
   CHECK( sum == L * ( L + 1 ) / 2 );
   // no task
   bool called = false;
   feedbackFarm< long , long >(
     ex , w , []( auto ) { } ,
     [ & ]( long , auto ) {
      called = true;
      return( true );
      } ,
     []( long t ) { return( t ); } );
   CHECK( ! called );
   // stopped after K results, with many tasks left
   const long K = 37;
   results = 0;
   feedbackFarm< long , long >(
     ex , w ,
     []( auto emit ) {
      for( long t = 0 ; t < 1000 ; ++t )
       emit( t );
      } ,
     [ & ]( long , auto ) { return( ++results < K ); } ,
     []( long t ) { return( t + 1 ); } );
   CHECK( results == K );
   // an exception of a worker
   CHECK_THROWS( ( feedbackFarm< long , long >(
		   ex , w ,
		   []( auto emit ) {
		    for( long t = 0 ; t < 100 ; ++t )
		     emit( t );
		    } ,
		   []( long , auto ) { return( true ); } ,
		   []( long t ) -> long {
		    if( t == 42 )
		     throw( std::runtime_error( "work" ) );
		    return( t );
		    } ) ) ,
		 std::runtime_error );
   }
  } );
 }

// the indices of the farm: work( t , i ) with i in [ 0 , w ), each index on
// one thread for the whole farm and distinct indices on distinct threads;
// start( emit , m ) with m = w, the master on a thread of its own, the
// caller on the emulations of the pool, of OpenMP and of Sequential; the
// tree of tasks still summed right
ADS_TEST( "farm-indices" )
{
 onEach( []( auto & ex ) {
  using E [[maybe_unused]] = std::remove_reference_t< decltype( ex ) >;
#if defined( ADS_WITH_FASTFLOW )
  constexpr bool ff = std::is_same_v< E , FastFlowExecutor >;
#else
  constexpr bool ff = false;
#endif
#if defined( ADS_WITH_TBB )
  constexpr bool tbb = std::is_same_v< E , TBBExecutor >;
#else
  constexpr bool tbb = false;
#endif
  const std::size_t P = ex.concurrency();
  for( std::size_t w : { std::size_t( 0 ) , ff ? P : P - 1 } ) {
   const std::size_t slots = std::max< std::size_t >( w , 1 );
   std::mutex m;
   std::vector< std::set< std::thread::id > > who( slots );
   std::thread::id master;
   std::size_t index = 99;
   bool masterCaller = false;
   std::atomic< bool > bad{ false };
   const long L = 3000;
   long sum = 0;
   feedbackFarm< long , long >(
     ex , w ,
     [ & ]( auto emit , std::size_t i ) {
      index = i;
      master = std::this_thread::get_id();
      masterCaller = isRunCaller();
      emit( 1L );
      } ,
     [ & ]( long r , auto emit ) {
      sum += r;
      if( 2 * r <= L )
       emit( 2 * r );
      if( 2 * r + 1 <= L )
       emit( 2 * r + 1 );
      return( true );
      } ,
     [ & ]( long t , std::size_t i ) {
      if( i >= slots )
       bad.store( true );
      else {
       std::lock_guard< std::mutex > g( m );
       who[ i ].insert( std::this_thread::get_id() );
       }
      return( t );
      } );
   CHECK( ! bad.load() );
   CHECK( sum == L * ( L + 1 ) / 2 );
   CHECK( index == w );
   std::set< std::thread::id > all;
   std::size_t used = 0;
   for( const auto & s : who ) {
    CHECK( s.size() <= 1 );
    used += s.size();
    all.insert( s.begin() , s.end() );
    }
   CHECK( all.size() == used );
   if( w == 0 ) {
    CHECK( masterCaller );
    CHECK( ( used == 1 ) &&
	   ( *who[ 0 ].begin() == std::this_thread::get_id() ) );
    }
   else {
    CHECK( all.count( master ) == 0 );
    if( ff )
     CHECK( ! masterCaller );
    else if( ! tbb )
     CHECK( masterCaller && ( master == std::this_thread::get_id() ) );
    }
   }
  } );
 }

namespace
{

/// the random tree of the seed explored by the p workers of ex over the
/// mailbox of the channels C with batches of b messages, each node sent to
/// the worker that owns its key; every worker checks, when it sees the
/// end, that all the nodes were explored
template< class C , class E , class... A >
void mailboxTree( E & ex , std::size_t p , std::size_t b , std::uint32_t d ,
		  std::uint64_t seed , bool root , A... args )
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
  for( ; ; ) {
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
  CHECK( mb.batches( i ) <= mb.sent( i ) );
  CHECK( ( mb.sent( i ) == 0 ) || ( mb.batches( i ) >= 1 ) );
  }
 CHECK( sent == received );
 // nothing is left after a termination
 CHECK( mb.leftovers( []( Node && ) { } ) == 0 );
 }

/// the random tree of the seed explored by the p workers of a pool over the
/// mailbox of the channels C with batches of b messages, stopped by the
/// worker 0 after it has explored `limit` nodes, or by another thread
/// after `delay` microseconds, racing with the sends; every message sent
/// is handed once, by a poll or by leftovers(), as the counts and the sums
/// of the keys of the sends tell
template< class C , class... A >
void mailboxStop( std::size_t p , std::size_t b , std::uint64_t seed ,
		  long limit ,
		  long delay , A... args )
{
 const std::uint32_t d = 14;
 Mailbox< Node , C > mb( p , b , args... );
 std::atomic< std::uint64_t > sends{ 0 } , sendSum{ 0 } , polls{ 0 } ,
   pollSum{ 0 };
 ThreadPool pool( p );
 std::thread stopper;
 if( delay >= 0 )
  stopper = std::thread( [ & ] {
   std::this_thread::sleep_for( std::chrono::microseconds( delay ) );
   mb.stop();
   } );
 pool.run( p , [ & ]( std::size_t me ) {
  std::vector< Node > stack;
  if( ownerOf( seed , p ) == me )
   stack.push_back( Node{ seed , 0 } );
  std::uint64_t n = 0 , s = 0 , k = 0 , ks = 0;
  long done = 0;
  unsigned spin = 0;
  auto take = [ & ]( Node && v ) {
   ++k;
   ks += mix( v.key );
   stack.push_back( v );
   };
  while( ! mb.stopped() ) {
   mb.poll( me , take );
   if( ! stack.empty() ) {
    for( int j = 0 ; ( j < 16 ) && ! stack.empty() ; ++j ) {
     const Node v = stack.back();
     stack.pop_back();
     if( ( me == 0 ) && ( ++done == limit ) )
      mb.stop();
     children( v , d , seed , [ & ]( Node c ) {
      const std::size_t o = ownerOf( c.key , p );
      if( o == me )
       stack.push_back( c );
      else {
       ++n;
       s += mix( c.key );
       mb.send( me , o , c );
       }
      } );
     }
    spin = 0;
    continue;
    }
   if( mb.quiescent( me ) )
    break;
   detail::spinPause( spin );
   }
  sends.fetch_add( n );
  sendSum.fetch_add( s );
  polls.fetch_add( k );
  pollSum.fetch_add( ks );
  } );
 if( stopper.joinable() )
  stopper.join();
 CHECK( mb.terminated() );
 for( std::size_t i = 0 ; i < p ; ++i )
  CHECK( mb.quiescent( i ) || ! mb.stopped() );
 std::uint64_t left = 0 , leftSum = 0;
 const std::size_t got = mb.leftovers( [ & ]( Node && v ) {
  ++left;
  leftSum += mix( v.key );
  } );
 CHECK( got == left );
 CHECK( sends.load() == polls.load() + left );
 CHECK( sendSum.load() == pollSum.load() + leftSum );
 CHECK( mb.leftovers( []( Node && ) { } ) == 0 );
 std::uint64_t pushed = 0 , received = 0;
 for( std::size_t i = 0 ; i < p ; ++i ) {
  pushed += mb.sent( i );
  received += mb.received( i );
  }
 CHECK( received == polls.load() );
 CHECK( pushed <= sends.load() );
 }

/// the stops of a family on 1, 2 and 4 workers, batches of 1 and 16, by a
/// worker after few and many nodes, at once, and by another thread
template< class C , class... A >
void stopFamily( A... args )
{
 for( std::size_t p : { 1 , 2 , 4 } )
  for( std::size_t b : { 1 , 16 } ) {
   for( long limit : { 1L , 50L , 5000L } )
    mailboxStop< C >( p , b , 3 + limit , limit , -1 , args... );
   for( long delay : { 0L , 200L , 2000L } )
    mailboxStop< C >( p , b , 7 + delay , -1 , delay , args... );
   }
 }

/// the trees of a family on pools of 1, 2 and 4 workers, batches of 1, 7
/// and 64, with no root and with random roots
template< class C , class... A >
void mailboxFamily( A... args )
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

/// the random trees of the seeds 1 , 2 , ... explored one after the other
/// by p threads on one mailbox of the channels C with batches of b
/// messages, a phase per tree, separated by a std::barrier whose
/// completion checks the phase and rearms the mailbox for the next: all the
/// workers active, or only the owner of the root; every third phase is
/// stopped by the worker 0 after a few nodes, and its messages recovered by
/// leftovers() before the mailbox is rearmed
template< class C , class... A >
void mailboxPhases( std::size_t p , std::size_t b , A... args )
{
 const std::uint32_t d = 11;
 const int phases = 12;
 Mailbox< Node , C > mb( p , b , args... );
 std::atomic< std::uint64_t > done{ 0 } , sends{ 0 } , sendSum{ 0 } ,
   polls{ 0 } ,
   pollSum{ 0 };
 int phase = 0 , bad = 0;
 auto stopped = []( int t ) { return( t % 3 == 2 ); };
 auto seed = []( int t ) { return( std::uint64_t( t + 1 ) ); };
 auto check = [ & ]( void ) noexcept {  // the phase, by one thread
  std::uint64_t left = 0 , leftSum = 0;
  mb.leftovers( [ & ]( Node && v ) {
   ++left;
   leftSum += mix( v.key );
   } );
  if( stopped( phase ) )
   bad += ! mb.stopped() || ( sends != polls + left ) ||
	  ( sendSum != pollSum + leftSum );
  else
   bad += mb.stopped() || ( left != 0 ) || ( sends != polls ) ||
	  ( sendSum != pollSum ) || ( done != treeSize( d , seed( phase ) ) );
  done = sends = sendSum = polls = pollSum = 0;
  const std::size_t owner = ownerOf( seed( ++phase ) , p );
  if( phase % 2 )
   mb.rearm( [ & ]( std::size_t me ) { return( me == owner ); } );
  else
   mb.rearm();
  bad += mb.stopped() || mb.terminated();
  };
 std::barrier sync( std::ptrdiff_t( p ) , check );
 std::vector< std::thread > T;
 for( std::size_t me = 0 ; me < p ; ++me )
  T.emplace_back( [ & , me ] {
   for( int t = 0 ; t < phases ; ++t ) {
    std::vector< Node > stack;
    if( ownerOf( seed( t ) , p ) == me )
     stack.push_back( Node{ seed( t ) , 0 } );
    std::uint64_t n = 0 , s = 0 , k = 0 , ks = 0 , x = 0;
    long work = 0;
    unsigned spin = 0;
    auto take = [ & ]( Node && v ) {
     ++k;
     ks += mix( v.key );
     stack.push_back( v );
     };
    for( ; ; ) {
     mb.poll( me , take );
     if( ! stack.empty() && ! mb.stopped() ) {
      for( int j = 0 ; ( j < 16 ) && ! stack.empty() ; ++j ) {
       const Node v = stack.back();
       stack.pop_back();
       ++x;
       if( ( me == 0 ) && stopped( t ) && ( ++work == 40 ) )
	mb.stop();
       children( v , d , seed( t ) , [ & ]( Node c ) {
	const std::size_t o = ownerOf( c.key , p );
	if( o == me )
	 stack.push_back( c );
	else {
	 ++n;
	 s += mix( c.key );
	 mb.send( me , o , c );
	 }
        } );
       }
      spin = 0;
      continue;
      }
     if( mb.quiescent( me ) )
      break;
     detail::spinPause( spin );
     }
    done += x;
    sends += n;
    sendSum += s;
    polls += k;
    pollSum += ks;
    sync.arrive_and_wait();
    }
   } );
 for( auto & t : T )
  t.join();
 CHECK( bad == 0 );
 CHECK( phase == phases );
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

// the stop on every family, the mesh also of capacity 2, whose full queues
// leave messages in the outboxes when the stop comes
ADS_TEST( "mailbox-stop" )
{
 stopFamily< LockedChannels< Node > >();
 stopFamily< InboxChannels< Node > >();
 stopFamily< LaneChannels< Node > >();
 stopFamily< MeshChannels< Node > >( std::size_t( 2 ) );
 stopFamily< MeshChannels< Node > >( std::size_t( 1024 ) );
 // a stop before any work, and one called twice
 Mailbox< Node > mb( 3 , 4 );
 mb.stop();
 mb.stop();
 CHECK( mb.stopped() && mb.terminated() );
 for( std::size_t i = 0 ; i < 3 ; ++i ) {
  CHECK( mb.quiescent( i ) );
  mb.send( i , ( i + 1 ) % 3 , Node{ i , 0 } );
  CHECK( mb.poll( i , []( Node && ) { } ) == 0 );
  CHECK( ! mb.flush( i ) );
  CHECK( mb.sent( i ) == 0 );
  }
 std::uint64_t keys = 0;
 CHECK( mb.leftovers( [ & ]( Node && v ) { keys += v.key + 1; } ) == 3 );
 CHECK( keys == 6 );
 }

// the sends never wait: a mesh of capacity 2 between two workers, batches
// of 4, driven by one thread; the part of a batch that does not fit stays
// in the outbox, send() tries it again only a batch later, flush() and
// quiescent() say false while something is left, and the messages come in
// their order, each once
ADS_TEST( "mailbox-full" )
{
 Mailbox< std::uint64_t > mb( 2 , 4 , std::size_t( 2 ) );
 std::vector< std::uint64_t > got;
 auto take = [ & ]( std::uint64_t && x ) { got.push_back( x ); };
 for( std::uint64_t k = 0 ; k < 4 ; ++k )
  mb.send( 0 , 1 , k );
 CHECK( ( mb.sent( 0 ) == 2 ) && ( mb.batches( 0 ) == 1 ) );
 for( std::uint64_t k = 4 ; k < 8 ; ++k )
  mb.send( 0 , 1 , k );  // a push at 6 left, refused
 CHECK( ( mb.sent( 0 ) == 2 ) && ( mb.batches( 0 ) == 1 ) );
 CHECK( ! mb.flush( 0 ) );
 CHECK( ! mb.quiescent( 0 ) );
 CHECK( ! mb.terminated() );
 CHECK( mb.poll( 1 , take ) == 2 );
 int rounds = 0;
 while( ! mb.flush( 0 ) ) {
  CHECK( mb.poll( 1 , take ) == 2 );
  ++rounds;
  }
 CHECK( rounds == 2 );
 CHECK( ( mb.sent( 0 ) == 8 ) && ( mb.batches( 0 ) == 4 ) );
 CHECK( ! mb.quiescent( 0 ) );  // passive, two on their way
 CHECK( ! mb.terminated() );
 CHECK( mb.poll( 1 , take ) == 2 );
 CHECK( mb.quiescent( 1 ) && mb.quiescent( 0 ) && mb.terminated() );
 CHECK( got.size() == 8 );
 for( std::size_t i = 0 ; i < got.size() ; ++i )
  CHECK( got[ i ] == i );
 CHECK( mb.received( 1 ) == 8 );
 CHECK( mb.leftovers( []( std::uint64_t && ) { } ) == 0 );
 }

// one mailbox for a sequence of computations: the phases of mailboxPhases
// on every family, 1 to 4 threads, batches of 1 and 16, the mesh also of
// capacity 2; then, on one thread, rearm() refused while an outbox holds a
// message, while a worker is active, and after a stop with messages in an
// inbox that leftovers() has not taken (the audit), and accepted after the
// end, with the workers active and passive as asked
ADS_TEST( "mailbox-rearm" )
{
 for( std::size_t p : { 1 , 2 , 4 } )
  for( std::size_t b : { 1 , 16 } ) {
   mailboxPhases< LockedChannels< Node > >( p , b );
   mailboxPhases< InboxChannels< Node > >( p , b );
   mailboxPhases< LaneChannels< Node > >( p , b );
   mailboxPhases< MeshChannels< Node > >( p , b , std::size_t( 2 ) );
   mailboxPhases< MeshChannels< Node > >( p , b , std::size_t( 1024 ) );
   }
 Mailbox< std::uint64_t > mb( 2 , 4 );
 auto none = []( std::uint64_t && ) { };
 mb.send( 0 , 1 , 7 );  // in the outbox
 CHECK_THROWS( mb.rearm() , contract_violation );
 CHECK( ! mb.quiescent( 0 ) );                    // pushed, 0 passive
 CHECK_THROWS( mb.rearm() , contract_violation );  // 1 active
 CHECK( mb.poll( 1 , none ) == 1 );
 CHECK( mb.quiescent( 1 ) && mb.quiescent( 0 ) && mb.terminated() );
 mb.rearm( []( std::size_t me ) { return( me == 1 ); } );
 CHECK( ! mb.terminated() && ! mb.quiescent( 0 ) );  // 0 passive
 mb.send( 1 , 0 , 8 );
 CHECK( mb.flush( 1 ) && ! mb.quiescent( 0 ) );
 CHECK( mb.poll( 0 , none ) == 1 );  // 0 active by receiving
 CHECK( ! mb.quiescent( 0 ) );
 CHECK( mb.quiescent( 1 ) && mb.terminated() );
 mb.rearm();
 mb.send( 0 , 1 , 9 );
 CHECK( mb.flush( 0 ) );
 mb.stop();  // 9 left in the inbox of 1
 CHECK_THROWS( mb.rearm() , contract_violation );
 mb.rearm();  // the audit took it
 CHECK( ! mb.stopped() && ( mb.leftovers( none ) == 0 ) );
 mb.send( 1 , 0 , 10 );
 mb.stop();
 CHECK( mb.leftovers( none ) == 1 );  // from the outbox of 1
 mb.rearm();
 CHECK( ! mb.stopped() && ! mb.terminated() && ! mb.quiescent( 0 ) );
 CHECK( mb.quiescent( 1 ) && mb.terminated() );
 CHECK( ( mb.sent( 0 ) == 2 ) && ( mb.sent( 1 ) == 1 ) &&
	( mb.received( 0 ) == 1 ) && ( mb.received( 1 ) == 1 ) );
 }

namespace
{

/// a message of the shortest paths: a relaxation of the vertex v to the
/// distance d, or a probe of v (kind 1), whose order with the relaxations
/// of v the merge keeps
struct Label
{
 std::uint32_t v;
 std::uint64_t d;
 std::uint8_t kind;
 };

/// the key of a label, its vertex
struct LabelHead
{
 std::uint32_t operator()( const Label & x ) const { return( x.v ); }
 };

/// the merge of two relaxations of one vertex by the least distance; a
/// probe is never merged
bool lowest( Label & p , const Label & x )
{
 if( ( p.kind != 0 ) || ( x.kind != 0 ) )
  return( false );
 p.d = std::min( p.d , x.d );
 return( true );
 }

/// a mailbox of labels coalesced by their vertex on the channels C, by the
/// map M (void for the default)
template< class C , class M = void >
using LabelBox = Mailbox< Label , C , CoalesceBy< LabelHead , M > >;

/// no distance
constexpr std::uint64_t kFar = ~std::uint64_t( 0 );

/// a random digraph of n vertices, 1 to 4 arcs out of each, of costs 1 to 9
using Arcs =
  std::vector< std::vector< std::pair< std::uint32_t , std::uint64_t > > >;

Arcs randomArcs( std::uint32_t n , std::uint64_t seed )
{
 Arcs a( n );
 std::mt19937_64 g( seed );
 for( std::uint32_t v = 0 ; v < n ; ++v )
  for( std::uint64_t j = 1 + g() % 4 ; j > 0 ; --j )
   a[ v ].emplace_back( std::uint32_t( g() % n ) , 1 + g() % 9 );
 return( a );
 }

/// the distances from s by Dijkstra, on one thread
std::vector< std::uint64_t > dijkstra( const Arcs & a , std::uint32_t s )
{
 std::vector< std::uint64_t > d( a.size() , kFar );
 using Item = std::pair< std::uint64_t , std::uint32_t >;
 std::priority_queue< Item , std::vector< Item > , std::greater<> > q;
 d[ s ] = 0;
 q.emplace( 0 , s );
 while( ! q.empty() ) {
  const auto [ dv , v ] = q.top();
  q.pop();
  if( dv > d[ v ] )
   continue;
  for( const auto & [ w , c ] : a[ v ] )
   if( dv + c < d[ w ] ) {
    d[ w ] = dv + c;
    q.emplace( d[ w ] , w );
    }
  }
 return( d );
 }

/// what a worker of the shortest paths counts
struct PathCounts
{
 std::uint64_t sends = 0;  ///< the calls of send()
 std::uint64_t taken = 0;  ///< the messages handed by poll()
 };

/// the loop of the worker me of p over the mailbox mb of labels, the
/// distances from s by relaxations in any order (label correcting): lower(
/// v , d ) lowers the distance of a vertex of me, true if it was larger,
/// dist( v ) is that distance; the worker 0 stops the mailbox after `stop`
/// vertices (never if negative); every seventh send is followed by a probe
template< class MB , class Lower , class Dist >
PathCounts pathWorker( MB & mb , const Arcs & a , std::uint32_t s ,
		       std::size_t me , long stop , Lower && lower ,
		       Dist && dist )
{
 const std::size_t p = mb.size();
 PathCounts c;
 std::vector< std::uint32_t > stack;
 if( ( ownerOf( s , p ) == me ) && lower( s , 0 ) )
  stack.push_back( s );
 long done = 0;
 unsigned spin = 0;
 auto take = [ & ]( Label && x ) {
  ++c.taken;
  if( ( x.kind == 0 ) && lower( x.v , x.d ) )
   stack.push_back( x.v );
  };
 for( ; ; ) {
  mb.poll( me , take );
  if( ! stack.empty() && ! mb.stopped() ) {
   for( int j = 0 ; ( j < 16 ) && ! stack.empty() ; ++j ) {
    const std::uint32_t v = stack.back();
    stack.pop_back();
    if( ( me == 0 ) && ( ++done == stop ) )
     mb.stop();
    for( const auto & [ w , cost ] : a[ v ] ) {
     const std::uint64_t nd = dist( v ) + cost;
     const std::size_t o = ownerOf( w , p );
     if( o == me ) {
      if( lower( w , nd ) )
       stack.push_back( w );
      continue;
      }
     mb.send( me , o , Label{ w , nd , 0 } , lowest );
     if( ++c.sends % 7 == 0 ) {
      mb.send( me , o , Label{ w , 0 , 1 } , lowest );
      ++c.sends;
      }
     }
    }
   spin = 0;
   continue;
   }
  if( mb.quiescent( me ) )
   break;
  detail::spinPause( spin );
  }
 return( c );
 }

/// the distances from 0 on a random digraph by the p workers of a pool over
/// a mailbox of the channels C with batches of b messages, coalesced: the
/// same as Dijkstra's, and every message sent handed once or merged; with
/// a stop by the worker 0, every message sent handed once, by a poll or by
/// leftovers(), or merged; the merges, summed into *merged
template< class C , class M = void , class... A >
void coalescedPaths( std::size_t p , std::size_t b , std::uint64_t seed ,
		     long stop , std::uint64_t * merged , A... args )
{
 const std::uint32_t n = 400;
 const Arcs a = randomArcs( n , seed );
 const auto ref = dijkstra( a , 0 );
 LabelBox< C , M > mb( p , b , args... );
 std::vector< std::uint64_t > d( n , kFar );  // d[ v ] of its owner only
 std::atomic< std::uint64_t > sends{ 0 } , taken{ 0 };
 ThreadPool pool( p );
 pool.run( p , [ & ]( std::size_t me ) {
  const PathCounts c = pathWorker(
    mb , a , 0 , me , stop ,
    [ & ]( std::uint32_t v , std::uint64_t x ) {
     if( x >= d[ v ] )
      return( false );
     d[ v ] = x;
     return( true );
     } ,
    [ & ]( std::uint32_t v ) { return( d[ v ] ); } );
  sends += c.sends;
  taken += c.taken;
  } );
 std::uint64_t got = 0 , pushed = 0 , m = 0;
 for( std::size_t i = 0 ; i < p ; ++i ) {
  got += mb.received( i );
  pushed += mb.sent( i );
  m += mb.merged( i );
  }
 const std::uint64_t left = mb.leftovers( []( Label && ) { } );
 CHECK( got == taken.load() );
 CHECK( sends.load() == got + left + m );
 CHECK( pushed <= sends.load() - m );
 if( stop < 0 ) {
  CHECK( left == 0 );
  CHECK( pushed == got );
  CHECK( d == ref );
  }
 *merged += m;
 }

/// the paths of a family on 1, 2 and 4 workers, batches of 1, 16 and 64,
/// to the end and stopped after few and many vertices, by the map M (void
/// for the default); some merge
template< class C , class M = void , class... A >
void coalescedFamily( A... args )
{
 std::uint64_t merged = 0;
 for( std::size_t p : { 1 , 2 , 4 } )
  for( std::size_t b : { 1 , 16 , 64 } )
   for( std::uint64_t seed : { 1 , 2 } ) {
    coalescedPaths< C , M >( p , b , seed , -1 , &merged , args... );
    for( long stop : { 1L , 30L } )
     coalescedPaths< C , M >( p , b , seed + 10 , stop , &merged , args... );
    }
 CHECK( merged > 0 );
 }

/// the distance of a vertex as an observer keeps it, incrementally: the
/// state of the consumer
struct LeastLabel
{
 std::vector< std::uint64_t > d;
 void on( const Label & e ) { d[ e.v ] = std::min( d[ e.v ] , e.d ); }
 };

/// the distances recomputed from scratch from all the labels seen
struct LabelLog
{
 std::size_t n = 0;
 std::vector< Label > log;
 void on( const Label & e ) { log.push_back( e ); }
 [[nodiscard]] std::vector< std::uint64_t > distances( void ) const {
  std::vector< std::uint64_t > d( n , kFar );
  for( const Label & e : log )
   d[ e.v ] = std::min( d[ e.v ] , e.d );
  return( d );
  }
 };

/// the phases of the shortest paths from the sources 0 , 7 , 14 , ... on
/// one random digraph, by p threads on one mailbox of the channels C with
/// batches of b messages, coalesced, the distances of each worker kept by
/// LeastLabel under a Shadow with LabelLog; the completion of a
/// std::barrier audits the phase by audit() before it rearms the mailbox,
/// with the predicate of each worker: the shadow agrees, and the distances
/// of its vertices are Dijkstra's. The phase 1 corrupts a distance of the
/// incremental state, which the shadow sees, the phase 2 one of both, which
/// only Dijkstra's sees, the phase 3 is stopped, its messages recovered by
/// leftovers() and audited by the shadow alone, which holds; the others
/// are kept as they are, and the audit holds
template< class C , class... A >
void auditedPhases( std::size_t p , std::size_t b , A... args )
{
 const std::uint32_t n = 300;
 const int phases = 6;
 const Arcs a = randomArcs( n , 5 );
 using Sd = Shadow< LeastLabel , LabelLog >;
 std::vector< LeastLabel > inc( p );
 std::vector< LabelLog > scr( p );
 std::vector< Sd > sd;
 for( std::size_t me = 0 ; me < p ; ++me ) {
  inc[ me ].d.assign( n , kFar );
  scr[ me ].n = n;
  sd.emplace_back( inc[ me ] , scr[ me ] );
  }
 LabelBox< C > mb( p , b , args... );
 auto source = []( int t ) { return( std::uint32_t( 7 * t ) ); };
 auto stopped = []( int t ) { return( t == 3 ); };
 auto both = []( auto & o ) {
  if constexpr( std::is_same_v< std::decay_t< decltype( o ) > , LeastLabel > )
   return( o.d );
  else
   return( o.distances() );
  };
 int phase = 0 , bad = 0;
 std::vector< std::uint64_t > ref = dijkstra( a , source( 0 ) );
 auto exact = [ & ]( std::size_t me ) {
  for( std::uint32_t v = 0 ; v < n ; ++v )
   if( ( ownerOf( v , p ) == me ) && ( inc[ me ].d[ v ] != ref[ v ] ) )
    return( false );
  return( true );
  };
 auto check = [ & ]( void ) noexcept {  // the phase, by one thread
  bool fired = false;
  try {
   if( stopped( phase ) ) {
    mb.leftovers( []( Label && ) { } );
    mb.audit( [ & ]( std::size_t me ) { return( sd[ me ].agree( both ) ); } );
    }
   else {
    // a vertex reached by its owner, other than the source
    std::uint32_t v = source( phase ) + 1;
    while( ( ref[ v % n ] == kFar ) || ( ref[ v % n ] == 0 ) )
     ++v;
    v %= n;
    const std::size_t o = ownerOf( v , p );
    if( phase == 1 )
     inc[ o ].d[ v ] -= 1;
    if( phase == 2 )
     sd[ o ].on( Label{ v , ref[ v ] - 1 , 0 } );
    mb.audit( [ & ]( std::size_t me ) {
     return( sd[ me ].agree( both ) && exact( me ) );
     } );
    }
   }
  catch( const contract_violation & ) {
   fired = true;
   }
  bad += fired != ( ( phase == 1 ) || ( phase == 2 ) );
  for( std::size_t me = 0 ; me < p ; ++me ) {
   inc[ me ].d.assign( n , kFar );
   scr[ me ].log.clear();
   }
  ref = dijkstra( a , source( ++phase ) );
  mb.rearm();
  };
 std::barrier sync( std::ptrdiff_t( p ) , check );
 std::vector< std::thread > T;
 for( std::size_t me = 0 ; me < p ; ++me )
  T.emplace_back( [ & , me ] {
   for( int t = 0 ; t < phases ; ++t ) {
    auto lower = [ & ]( std::uint32_t v , std::uint64_t x ) {
     if( x >= inc[ me ].d[ v ] )
      return( false );
     sd[ me ].on( Label{ v , x , 0 } );
     return( true );
     };
    pathWorker( mb , a , source( t ) , me , stopped( t ) ? 25 : -1 , lower ,
		[ & ]( std::uint32_t v ) { return( inc[ me ].d[ v ] ); } );
    sync.arrive_and_wait();
    }
   } );
 for( auto & t : T )
  t.join();
 CHECK( bad == 0 );
 CHECK( phase == phases );
 }

}  // end( namespace )

// the coalescing of the outboxes on one thread: two relaxations of a
// vertex merged into the least, a probe that closes the run of its vertex,
// a merge with no result, the map emptied by a push and by leftovers();
// on a mesh of capacity 2, a push that takes part of the outbox empties
// the map, so that no merge reaches a place shifted, and one that takes
// nothing keeps it; a merge after a stop, the counts of merged() and
// sent(), and the plain send(), which closes the run of its key
/// the coalescing of one worker in an outbox by the map M (void for the
/// default)
template< class M >
void coalescedUnit( void )
{
 std::vector< Label > got;
 auto take = [ & ]( Label && x ) { got.push_back( x ); };
 auto is = [ & ]( std::size_t i , std::uint32_t v , std::uint64_t d ,
		  std::uint8_t k ) {
  return( ( got[ i ].v == v ) && ( got[ i ].d == d ) &&
	   ( got[ i ].kind == k ) );
  };
 {
  LabelBox< LockedChannels< Label > , M > mb( 2 , 4 );
  mb.send( 0 , 1 , Label{ 5 , 10 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 5 , 7 , 0 } , lowest );  // merged
  mb.send( 0 , 1 , Label{ 6 , 3 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 5 , 0 , 1 } , lowest );  // a probe, kept
  CHECK( ( mb.merged( 0 ) == 1 ) && ( mb.sent( 0 ) == 0 ) );
  mb.send( 0 , 1 , Label{ 5 , 3 , 0 } , lowest );  // after the probe
  CHECK( mb.sent( 0 ) == 4 );                 // pushed, 4 messages
  mb.send( 0 , 1 , Label{ 5 , 2 , 0 } , lowest );  // the map is empty
  mb.send( 0 , 1 , Label{ 5 , 1 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 6 , 9 , 0 } );  // unknown to the map
  mb.send( 0 , 1 , Label{ 6 , 8 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 6 , 4 , 0 } ,
	   []( Label & q , const Label & x ) { q.d += x.d; } );
  CHECK( mb.merged( 0 ) == 3 );
  CHECK( mb.flush( 0 ) );
  CHECK( mb.poll( 1 , take ) == 7 );
  CHECK( got.size() == 7 );
  CHECK( is( 0 , 5 , 7 , 0 ) && is( 1 , 6 , 3 , 0 ) && is( 2 , 5 , 0 , 1 ) &&
	 is( 3 , 5 , 3 , 0 ) && is( 4 , 5 , 1 , 0 ) && is( 5 , 6 , 9 , 0 ) &&
	 is( 6 , 6 , 12 , 0 ) );
  CHECK( ( mb.sent( 0 ) == 7 ) && ( mb.received( 1 ) == 7 ) );
  CHECK( ! mb.quiescent( 0 ) && mb.quiescent( 1 ) && mb.quiescent( 0 ) &&
	 mb.terminated() );
  // after a stop the merges go on in the outbox, and leftovers() hands
  // what they left and empties the map
  mb.rearm();
  mb.stop();
  mb.send( 1 , 0 , Label{ 2 , 9 , 0 } , lowest );
  mb.send( 1 , 0 , Label{ 2 , 4 , 0 } , lowest );
  got.clear();
  CHECK( mb.leftovers( take ) == 1 );
  CHECK( is( 0 , 2 , 4 , 0 ) && ( mb.merged( 1 ) == 1 ) );
  mb.rearm();
  mb.send( 1 , 0 , Label{ 2 , 8 , 0 } , lowest );  // no stale place
  CHECK( mb.flush( 1 ) );
  got.clear();
  CHECK( ( mb.poll( 0 , take ) == 1 ) && is( 0 , 2 , 8 , 0 ) );
  }
 {
  // a mesh of capacity 2, batches of 4: the push at 4 takes 1 and 2
  LabelBox< MeshChannels< Label > , M > mb( 2 , 4 , std::size_t( 2 ) );
  for( std::uint32_t v = 1 ; v <= 4 ; ++v )
   mb.send( 0 , 1 , Label{ v , 10 * v , 0 } , lowest );
  CHECK( mb.sent( 0 ) == 2 );
  mb.send( 0 , 1 , Label{ 3 , 5 , 0 } , lowest );  // places shifted
  CHECK( mb.merged( 0 ) == 0 );
  mb.send( 0 , 1 , Label{ 3 , 4 , 0 } , lowest );  // into the last 3
  CHECK( mb.merged( 0 ) == 1 );
  mb.send( 0 , 1 , Label{ 5 , 50 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 6 , 60 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 7 , 70 , 0 } , lowest );  // 6: a push refused
  CHECK( mb.sent( 0 ) == 2 );
  mb.send( 0 , 1 , Label{ 5 , 1 , 0 } , lowest );  // the map was kept
  CHECK( mb.merged( 0 ) == 2 );
  got.clear();
  while( ! mb.flush( 0 ) )
   mb.poll( 1 , take );
  mb.poll( 1 , take );
  CHECK( got.size() == 8 );
  CHECK( is( 0 , 1 , 10 , 0 ) && is( 1 , 2 , 20 , 0 ) && is( 2 , 3 , 30 ,
							     0 ) &&
	 is( 3 , 4 , 40 , 0 ) && is( 4 , 3 , 4 , 0 ) && is( 5 , 5 , 1 , 0 ) &&
	 is( 6 , 6 , 60 , 0 ) && is( 7 , 7 , 70 , 0 ) );
  CHECK( ( mb.sent( 0 ) == 8 ) && ( mb.received( 1 ) == 8 ) );
  }
 {
  // a plain send closes the run of its key: the relaxation after an
  // invalidation stays after it, and merges with the next ones
  LabelBox< LockedChannels< Label > , M > mb( 2 , 64 );
  mb.send( 0 , 1 , Label{ 6 , 8 , 0 } , lowest );
  mb.send( 0 , 1 , Label{ 6 , 0 , 1 } );          // the invalidation
  mb.send( 0 , 1 , Label{ 6 , 4 , 0 } , lowest );  // not into 8
  mb.send( 0 , 1 , Label{ 6 , 2 , 0 } , lowest );  // into 4
  mb.send( 0 , 1 , Label{ 7 , 5 , 1 } );          // a key never merged
  mb.send( 0 , 1 , Label{ 7 , 3 , 0 } , lowest );
  CHECK( mb.merged( 0 ) == 1 );
  got.clear();
  CHECK( mb.flush( 0 ) && ( mb.poll( 1 , take ) == 5 ) );
  CHECK( is( 0 , 6 , 8 , 0 ) && is( 1 , 6 , 0 , 1 ) && is( 2 , 6 , 2 , 0 ) &&
	 is( 3 , 7 , 5 , 1 ) && is( 4 , 7 , 3 , 0 ) );
  }
 }

ADS_TEST( "mailbox-coalesce" )
{
 using Dense = DenseHashMap< std::uint32_t , std::size_t >;
 using Flat = FlatHashTable< std::uint32_t , std::size_t >;
 static_assert( MailboxCoalescing< NoCoalescing , Label > &&
		MailboxCoalescing< CoalesceBy< LabelHead > , Label > &&
		MailboxCoalescing< CoalesceBy< LabelHead , Dense > , Label > &&
		MailboxCoalescing< CoalesceBy< LabelHead , Flat > , Label > &&
		! MailboxCoalescing< CoalesceBy< LabelHead > , int > );
 coalescedUnit< void >();
 coalescedUnit< Dense >();
 coalescedUnit< Flat >();
 }

// the coalescing by many workers: the shortest paths by relaxations in any
// order, merged by the least distance with probes in between, on every
// family, the mesh also of capacity 2 (pushes of part of a batch), and
// there also by the two flat maps, to the end against Dijkstra and
// stopped, every message handed once or merged
ADS_TEST( "mailbox-coalesce-paths" )
{
 coalescedFamily< LockedChannels< Label > >();
 coalescedFamily< InboxChannels< Label > >();
 coalescedFamily< LaneChannels< Label > >();
 coalescedFamily< MeshChannels< Label > >( std::size_t( 2 ) );
 coalescedFamily< MeshChannels< Label > >( std::size_t( 1024 ) );
 coalescedFamily< MeshChannels< Label > ,
		  DenseHashMap< std::uint32_t , std::size_t > >(
   std::size_t( 2 ) );
 coalescedFamily< MeshChannels< Label > ,
		  FlatHashTable< std::uint32_t , std::size_t > >(
   std::size_t( 2 ) );
 }

// the audit at a quiescent point: the phases of auditedPhases on every
// family, the predicate that fails on a corrupted state caught; on one
// thread, audit() refused while an outbox holds a message, while a worker
// is active and after a stop with messages in an inbox, and a predicate
// false of no worker caught
ADS_TEST( "mailbox-audit" )
{
 for( std::size_t p : { 1 , 2 , 4 } )
  for( std::size_t b : { 1 , 16 } ) {
   auditedPhases< LockedChannels< Label > >( p , b );
   auditedPhases< InboxChannels< Label > >( p , b );
   auditedPhases< LaneChannels< Label > >( p , b );
   auditedPhases< MeshChannels< Label > >( p , b , std::size_t( 2 ) );
   }
 Mailbox< std::uint64_t > mb( 2 , 4 );
 auto none = []( std::uint64_t && ) { };
 auto yes = []( void ) { return( true ); };
 mb.send( 0 , 1 , 7 );  // in the outbox
 CHECK_THROWS( mb.audit( yes ) , contract_violation );
 CHECK( ! mb.quiescent( 0 ) );  // pushed, 1 active
 CHECK_THROWS( mb.audit( yes ) , contract_violation );
 CHECK( mb.poll( 1 , none ) == 1 );
 CHECK( mb.quiescent( 1 ) && mb.quiescent( 0 ) );
 mb.audit( yes );
 mb.audit( []( std::size_t me ) { return( me < 2 ); } );
 CHECK_THROWS( mb.audit( []( void ) { return( false ); } ) ,
	       contract_violation );
 CHECK_THROWS( mb.audit( []( std::size_t me ) { return( me == 0 ); } ) ,
	       contract_violation );
 mb.rearm();
 mb.send( 0 , 1 , 9 );
 CHECK( mb.flush( 0 ) );
 mb.stop();  // 9 left in the inbox of 1
 CHECK_THROWS( mb.audit( yes ) , contract_violation );  // drains it
 mb.audit( yes );
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
 CHECK( ownerOf( 10 , 4 , []( int k ) { return( std::size_t( k * 3 ) ); } ) ==
	2 );
 }

namespace
{

/// the frontier of a best-first search of the nodes of the random tree
using Frontier = StealingFrontier< std::uint64_t , Node >;

/// the counts of a search on a frontier
struct Counts
{
 std::atomic< std::uint64_t > pushed{ 0 } , popped{ 0 } , sum{ 0 } , seen{ 0 };
 std::atomic< bool > up{ true };  // the keys of each worker
 };

/// the best-first search of the random tree of depth d on the frontier F
/// run by ex, the key of a child its parent's plus 1 to 64; the worker
/// that pops the node `limit` of all stops the search there, or throws if
/// `boom`
template< class E >
void search( E & ex , Frontier & F , std::uint32_t d , std::uint64_t seed ,
	     bool root , Counts & c , long limit = -1 , bool boom = false )
{
 if( root ) {
  F.push( 0 , 0 , Node{ seed , 0 } );
  c.pushed.fetch_add( 1 );
  }
 F.run( ex , [ & ]( std::size_t i ) {
  std::uint64_t last = 0 , n = 0 , s = 0 , pushed = 0;
  bool up = true;
  while( auto e = F.pop( i ) ) {
   ++n;
   s += mix( e->item.key );
   if( long( c.seen.fetch_add( 1 ) + 1 ) == limit ) {
    c.popped.fetch_add( n );
    c.sum.fetch_add( s );
    c.pushed.fetch_add( pushed );
    n = s = pushed = 0;
    if( boom )
     throw( std::runtime_error( "a node that throws" ) );
    F.stop();
    }
   up = up && ( e->key >= last );
   last = e->key;
   children( e->item , d , seed , [ & ]( Node v ) {
    F.push( i , e->key + 1 + ( v.key & 63 ) , v );
    ++pushed;
    } );
   }
  c.popped.fetch_add( n );
  c.sum.fetch_add( s );
  c.pushed.fetch_add( pushed );
  if( ! up )
   c.up = false;
  } );
 }

/// the sum of the mixed keys of the nodes of the random tree, on one thread
std::uint64_t treeSum( std::uint32_t d , std::uint64_t seed )
{
 std::vector< Node > stack{ Node{ seed , 0 } };
 std::uint64_t s = 0;
 while( ! stack.empty() ) {
  const Node v = stack.back();
  stack.pop_back();
  s += mix( v.key );
  children( v , d , seed , [ & ]( Node c ) { stack.push_back( c ); } );
  }
 return( s );
 }

}  // end( namespace )

// the best-first search on the stealing frontier on every executor: every
// node of the tree once, all of them pushed by the worker that holds the
// root, so that the others start by stealing from one victim; a search
// with no root, which ends at once; on one worker, the order of the keys
// never decreases, the exact order of a sequential search
ADS_TEST( "stealing-frontier" )
{
 onEach( []( auto & ex ) {
  const std::size_t p = workersOf( ex );
  for( std::uint64_t seed : { 1 , 2 } ) {
   const std::uint32_t d = 14;
   Frontier F( p , seed );
   Counts c;
   search( ex , F , d , seed , true , c );
   CHECK( F.terminated() && ! F.stopped() );
   CHECK( c.popped.load() == treeSize( d , seed ) );
   CHECK( c.pushed.load() == c.popped.load() );
   CHECK( c.sum.load() == treeSum( d , seed ) );
   CHECK( ( p > 1 ) || c.up.load() );
   CHECK( F.leftovers( []( Frontier::Entry && ) { } ) == 0 );
   CHECK( ( p > 1 ) || ( F.steals() == 0 ) );
   }
  Frontier E( p );
  Counts c;
  search( ex , E , 5 , 3 , false , c );
  CHECK( E.terminated() && ( c.popped.load() == 0 ) );
  } );
 CHECK_THROWS( Frontier( 0 ) , contract_violation );
 Frontier F( 2 );
 CHECK_THROWS( F.push( 2 , 0 , Node{ 0 , 0 } ) , contract_violation );
 CHECK_THROWS( F.pop( 2 ) , contract_violation );
 }

// the stop of the search by a worker after 1, 50 and 3000 nodes, and an
// exception thrown there, which run() throws again once the others have
// stopped: every node pushed is either popped or handed to leftovers(),
// once
ADS_TEST( "stealing-frontier-stop" )
{
 onEach( []( auto & ex ) {
  const std::size_t p = workersOf( ex );
  for( bool boom : { false , true } )
   for( long limit : { 1L , 50L , 3000L } ) {
    Frontier F( p , 7 + limit );
    Counts c;
    bool threw = false;
    try {
     search( ex , F , 14 , 5 , true , c , limit , boom );
     }
    catch( const std::runtime_error & ) {
     threw = true;
     }
    CHECK( threw == boom );
    CHECK( F.stopped() );
    std::uint64_t left = 0;
    const std::size_t got =
      F.leftovers( [ & ]( Frontier::Entry && ) { ++left; } );
    CHECK( got == left );
    CHECK( c.pushed.load() == c.popped.load() + left );
    CHECK( F.leftovers( []( Frontier::Entry && ) { } ) == 0 );
    }
  } );
 }

// the stealing frontier with bounds on every executor, a branch-and-bound
// of the random tree: the key of a child its depth, deepest first with the
// order of the pushes as a tie, its bound its parent's plus 1 to 64; a
// worker that finds the least bound of its queue above a cutoff drops the
// whole queue by drain(), each pair drained with a bound not below it and
// none left: every node pushed is popped or drained, once, and the search
// ends
ADS_TEST( "stealing-frontier-bound" )
{
 using Key = std::pair< std::int64_t , std::uint64_t >;
 using F = StealingFrontier< Key , Node , std::less< Key > , std::uint64_t >;
 onEach( []( auto & ex ) {
  const std::size_t p = workersOf( ex );
  for( std::uint64_t cutoff :
       { std::uint64_t( 40 ) , std::uint64_t( 400 ) , ~std::uint64_t( 0 ) } ) {
   F f( p , 11 );
   std::atomic< std::uint64_t > pushed{ 1 } , popped{ 0 } , drained{ 0 } ,
     stamp{ 0 };
   std::atomic< bool > right{ true };
   f.push( 0 , Key( 0 , 0 ) , 0 , Node{ 5 , 0 } );
   f.run( ex , [ & ]( std::size_t i ) {
    while( auto e = f.pop( i ) ) {
     popped.fetch_add( 1 );
     const std::uint64_t b = e->bound;
     children( e->item , 14 , 5 , [ & ]( Node v ) {
      const std::uint64_t s = stamp.fetch_add( 1 );
      f.push( i , Key( -std::int64_t( v.depth ) , ~s ) ,
	      b + 1 + ( v.key & 63 ) ,
	      v );
      pushed.fetch_add( 1 );
      } );
     const auto m = f.minBound( i );
     if( m && ( *m > cutoff ) ) {
      drained.fetch_add( f.drain( i , [ & ]( F::Entry && d ) {
       if( d.bound < *m )  // below the least bound
	right = false;
       } ) );
      if( f.minBound( i ) || ! f.queue( i ).empty() )
       right = false;  // the drain left a pair
      }
     }
    } );
   CHECK( f.terminated() && right.load() );
   CHECK( pushed.load() == popped.load() + drained.load() );
   CHECK( ( cutoff != ~std::uint64_t( 0 ) ) ||
	  ( popped.load() == treeSize( 14 , 5 ) ) );
   CHECK( f.leftovers( []( F::Entry && ) { } ) == 0 );
   }
  } );
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
