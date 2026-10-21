/*--------------------------------------------------------------------------*/
/*-------------------------- File concurrent.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the queues between threads: SPSCQueue of ADS against,
 * when found, boost::lockfree::spsc_queue, rigtorp::SPSCQueue
 * (ADS_HAS_RIGTORP), moodycamel::ReaderWriterQueue (ADS_HAS_MOODYCAMEL)
 * and folly::ProducerConsumerQueue (ADS_HAS_FOLLY), and MPSCInbox of ADS
 * against boost::lockfree::queue and moodycamel::ConcurrentQueue, each on
 * two threads pinned to the first two processors that the process may use
 * when it starts (e.g., taskset -c 14,15), read once before any thread is
 * pinned:
 *
 * - "spsc", case throughput: a producer pushes n integers into a queue of
 *   1024 places, retrying when it is full, while a consumer pops them,
 *   retrying when it is empty (ns per item, the time of the consumer;
 *   moodycamel::ReaderWriterQueue is built for 1023 items, which gives
 *   its block the same 1024 places);
 *   case pingpong: n round trips of an integer through two queues (ns per
 *   round trip);
 * - "mpsc", case throughput: one producer sends n integers, one consumer
 *   takes them (ns per item): MPSCInbox and MPSCQueue of ADS against
 *   boost::lockfree::queue and moodycamel::ConcurrentQueue, with and
 *   without its tokens of producer and consumer; the inbox takes all the
 *   messages at once, the others pop by batches of up to 256 where they
 *   can.
 *
 * - "mpmc", case throughput: a producer pushes n integers into a queue of
 *   1024 places of many producers and many consumers, one by one and
 *   retrying when it is full, while a consumer pops them (ns per item):
 *   MPMCQueue of ADS against boost::lockfree::queue of fixed capacity and
 *   moodycamel::ConcurrentQueue with no tokens;
 * - "steal", the deques of work stealing: WorkStealingDeque of ADS against
 *   std::deque under a std::mutex; case owner: n pushes and n takes of
 *   the owner alone (ns per operation); case thief: the owner pushes n
 *   integers while a thief steals them (ns per item);
 * - "wake", the consumer that sleeps until an item comes: SPSCQueue,
 *   MPSCQueue and MPSCInbox of ADS with their waits (waitPop(),
 *   waitTake(), the producer calling notify()) against a consumer that
 *   polls and sleeps 100 us when the queue is empty, std::deque under a
 *   std::mutex with a std::condition_variable, and
 *   moodycamel::BlockingReaderWriterQueue and BlockingConcurrentQueue
 *   (ADS_HAS_MOODYCAMEL); case latency: 2000 items, one every 50 us, each
 *   carrying the time of its push (ns from the push to the consumer, the
 *   median); case idle: the processor time of a consumer that waits 200
 *   ms on an empty queue (us per second of wait); case throughput: n items
 *   with a notification each (ns per item).
 *
 * Each cell is the median of r runs; the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   concurrent [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/concurrent/MPSCInbox.h"
#ifndef ADS_BENCH_OLD
 #include "ADS/concurrent/MPMCQueue.h"
 #include "ADS/concurrent/WorkStealingDeque.h"
#endif
#ifndef ADS_BENCH_OLD
 #include "ADS/concurrent/MPSCQueue.h"
#endif
#include "ADS/concurrent/SPSCQueue.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <time.h>

#if __has_include( <boost/lockfree/spsc_queue.hpp> )
 #include <boost/lockfree/queue.hpp>
 #include <boost/lockfree/spsc_queue.hpp>
 #define ADS_HAS_BOOST 1
#endif
#ifdef ADS_HAS_RIGTORP
 #include "rigtorp/SPSCQueue.h"
#endif
#ifdef ADS_HAS_MOODYCAMEL
 #include "concurrentqueue.h"
 #include "readerwriterqueue.h"
 #include "blockingconcurrentqueue.h"
#endif
#ifdef ADS_HAS_FOLLY
 #include "folly/ProducerConsumerQueue.h"
#endif

#include <atomic>
#include <pthread.h>
#include <sched.h>
#include <thread>

using namespace ADS::bench;
using Item = std::uint64_t;

namespace {

/// the k-th processor that the process could use when it started (read
/// once: a thread pinned to one processor would see only that one, and so
/// would the threads it creates)
int cpu( int k )
{
 static const std::vector< int > C = [] {
  cpu_set_t s;
  CPU_ZERO( &s );
  sched_getaffinity( 0 , sizeof( s ) , &s );
  std::vector< int > v;
  for( int c = 0 ; c < CPU_SETSIZE ; ++c )
   if( CPU_ISSET( c , &s ) ) v.push_back( c );
  return( v );
  }();
 return( C[ std::size_t( k ) % C.size() ] );
 }

/// the calling thread on the processor c
void pin( int c )
{
 cpu_set_t s;
 CPU_ZERO( &s );
 CPU_SET( c , &s );
 pthread_setaffinity_np( pthread_self() , sizeof( s ) , &s );
 }

/// the time in ns per item of a producer thread running produce() while
/// the calling thread, the consumer, runs consume(); the queue that make()
/// builds is not timed
template< class M , class P , class C >
double transfer( std::size_t n , int r , M && make , P && produce ,
                 C && consume )
{
 return( median( make , [ & ]( auto & q ) {
  std::atomic< bool > go{ false };
  std::thread t( [ & ] {
   pin( cpu( 1 ) );
   while( ! go.load( std::memory_order_acquire ) ) {}
   produce( *q , n );
   } );
  pin( cpu( 0 ) );
  go.store( true , std::memory_order_release );
  sink = sink + consume( *q , n );
  t.join(); } , r , n ) );
 }

/// the spsc cells of a queue that make() builds, by push( q , x ) and
/// pop( q , x ), both false when the queue is full or empty
template< class M , class Push , class Pop >
void spsc( const std::string & who , std::size_t n , int r , M && make ,
           Push && push , Pop && pop )
{
 print( "spsc" , "throughput" , n , who , transfer( n , r , make ,
         [ & ]( auto & q , std::size_t m ) {
          for( std::size_t i = 0 ; i < m ; ++i )
           while( ! push( q , Item( i ) ) ) {} } ,
         [ & ]( auto & q , std::size_t m ) {
          Item s = 0 , x;
          for( std::size_t i = 0 ; i < m ; ++i ) {
           while( ! pop( q , x ) ) {}
           s += x;
           }
          return( s ); } ) );
 const std::size_t k = n / 10;              // the round trips, slower
 print( "spsc" , "pingpong" , k , who , median( [ & ](){
         return( std::pair( make() , make() ) ); } , [ & ]( auto & Q ) {
          auto & a = *Q.first;
          auto & b = *Q.second;
          std::thread t( [ & ] {
           pin( cpu( 1 ) );
           Item x;
           for( std::size_t i = 0 ; i < k ; ++i ) {
            while( ! pop( a , x ) ) {}
            while( ! push( b , x + 1 ) ) {}
            }
           } );
          pin( cpu( 0 ) );
          Item x = 0;
          for( std::size_t i = 0 ; i < k ; ++i ) {
           while( ! push( a , x ) ) {}
           while( ! pop( b , x ) ) {}
           }
          t.join();
          sink = sink + x; } , r , k ) );
 }

/// now, in ns of the steady clock
Item nowNs( void )
{
 using namespace std::chrono;
 return( Item( duration_cast< nanoseconds >(
                steady_clock::now().time_since_epoch() ).count() ) );
 }

/// the processor time of the calling thread, in ns
Item cpuNs( void )
{
 timespec t;
 clock_gettime( CLOCK_THREAD_CPUTIME_ID , &t );
 return( Item( t.tv_sec ) * 1000000000u + Item( t.tv_nsec ) );
 }

/// the wake cells of a queue that make() builds, send( q , x ) pushing x
/// and waking the consumer, recv( q ) sleeping until an item and taking it
template< class M , class S , class R >
void wake( const std::string & who , std::size_t n , int r , M && make ,
           S && send , R && recv )
{
 const int k = 2000;                        // the latency: one item every
 std::vector< double > lat;                 // 50 us, stamped
 for( int run = 0 ; run < r ; ++run ) {
  auto q = make();
  std::vector< double > d( k );
  std::thread t( [ & ] {
   pin( cpu( 1 ) );
   for( int i = 0 ; i < k ; ++i ) {
    std::this_thread::sleep_for( std::chrono::microseconds( 50 ) );
    send( *q , nowNs() );
    }
   } );
  pin( cpu( 0 ) );
  for( int i = 0 ; i < k ; ++i ) {
   const Item x = recv( *q );
   d[ i ] = double( nowNs() - x );
   }
  t.join();
  std::nth_element( d.begin() , d.begin() + k / 2 , d.end() );
  lat.push_back( d[ k / 2 ] );
  }
 std::nth_element( lat.begin() , lat.begin() + r / 2 , lat.end() );
 print( "wake" , "latency" , std::size_t( k ) , who , lat[ r / 2 ] );
 std::vector< double > idle;                // the processor time of 200
 for( int run = 0 ; run < r ; ++run ) {     // ms of wait
  auto q = make();
  double used = 0;
  std::thread c( [ & ] {
   pin( cpu( 0 ) );
   const Item a = cpuNs();
   sink = sink + recv( *q );
   used = double( cpuNs() - a );
   } );
  pin( cpu( 1 ) );
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  send( *q , 1 );
  c.join();
  idle.push_back( used / 1000.0 / 0.2 );    // us per second
  }
 std::nth_element( idle.begin() , idle.begin() + r / 2 , idle.end() );
 print( "wake" , "idle" , 1 , who , idle[ r / 2 ] );
 print( "wake" , "throughput" , n , who , transfer( n , r , make ,
         [ & ]( auto & q , std::size_t m ) {
          for( std::size_t i = 0 ; i < m ; ++i ) send( q , Item( i ) ); } ,
         [ & ]( auto & q , std::size_t m ) {
          Item s = 0;
          for( std::size_t i = 0 ; i < m ; ++i ) s += recv( q );
          return( s ); } ) );
 }

/// std::deque under a lock, with a condition variable
struct LockedQueue {
 std::mutex m;
 std::condition_variable cv;
 std::deque< Item > d;
 };

#ifndef ADS_BENCH_OLD
/// MPSCQueue with the handle of its one producer
struct OneProducer {
 ADS::MPSCQueue< Item > q;
 ADS::MPSCQueue< Item >::Producer p = q.producer();
 };
#endif

}  // end( namespace )

int main( int argc , char ** argv )
{
 (void) cpu( 0 );                           // the processors, before any pin
 const Options o( argc , argv , { 1000000 , 10000000 } );
 const std::size_t cap = 1024;
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
#ifdef ADS_HAS_BOOST
  using BoostSpsc = boost::lockfree::spsc_queue< Item >;
  for( const char * who : { "boost::lockfree::spsc_queue" ,
                            "boost::lockfree::spsc_queue A/A" } )
   spsc( who , n , r , [ & ](){
         return( std::make_unique< BoostSpsc >( cap ) ); } ,
         []( BoostSpsc & q , Item x ) { return( q.push( x ) ); } ,
         []( BoostSpsc & q , Item & x ) { return( q.pop( x ) ); } );
#endif
#ifdef ADS_HAS_RIGTORP
  using Rig = rigtorp::SPSCQueue< Item >;
  spsc( "rigtorp::SPSCQueue" , n , r , [ & ](){
        return( std::make_unique< Rig >( cap ) ); } ,
        []( Rig & q , Item x ) { return( q.try_push( x ) ); } ,
        []( Rig & q , Item & x ) {
         Item * p = q.front();
         if( ! p ) return( false );
         x = *p;
         q.pop();
         return( true ); } );
#endif
#ifdef ADS_HAS_MOODYCAMEL
  using Rwq = moodycamel::ReaderWriterQueue< Item >;
  spsc( "moodycamel::ReaderWriterQueue" , n , r , [ & ](){
        return( std::make_unique< Rwq >( cap - 1 ) ); } ,   // cap places
        []( Rwq & q , Item x ) { return( q.try_enqueue( x ) ); } ,
        []( Rwq & q , Item & x ) { return( q.try_dequeue( x ) ); } );
#endif
#ifdef ADS_HAS_FOLLY
  using Pcq = folly::ProducerConsumerQueue< Item >;
  spsc( "folly::ProducerConsumerQueue" , n , r , [ & ](){
        return( std::make_unique< Pcq >( std::uint32_t( cap + 1 ) ) ); } ,
        []( Pcq & q , Item x ) { return( q.write( x ) ); } ,
        []( Pcq & q , Item & x ) { return( q.read( x ) ); } );
#endif
  using Ads = ADS::SPSCQueue< Item >;
  spsc( "ADS::SPSCQueue" , n , r , [ & ](){
        return( std::make_unique< Ads >( cap ) ); } ,
        []( Ads & q , Item x ) { return( q.push( x ) ); } ,
        []( Ads & q , Item & x ) {
#ifdef ADS_BENCH_OLD
         auto v = q.pop();                  // no tryPop() before
         if( v ) x = *v;
         return( v.has_value() );
#else
         return( q.tryPop( x ) );
#endif
         } );

  // ----- mpsc, one producer
  auto sendAll = []( auto & q , std::size_t m ) {
   for( std::size_t i = 0 ; i < m ; ++i ) q.push( Item( i ) ); };
#ifdef ADS_HAS_BOOST
  for( const char * who : { "boost::lockfree::queue" ,
                            "boost::lockfree::queue A/A" } )
   print( "mpsc" , "throughput" , n , who , transfer( n , r , [ & ](){
           return( std::make_unique< boost::lockfree::queue< Item > >(
                    cap ) ); } , []( auto & q , std::size_t m ) {
            for( std::size_t i = 0 ; i < m ; ++i ) q.push( Item( i ) ); } ,
           []( auto & q , std::size_t m ) {
            Item s = 0 , x;
            for( std::size_t got = 0 ; got < m ; )
             if( q.pop( x ) ) {
              s += x;
              ++got;
              }
            return( s ); } ) );
#endif
#ifdef ADS_HAS_MOODYCAMEL
  print( "mpsc" , "throughput" , n , "moodycamel::ConcurrentQueue" ,
         transfer( n , r , [ & ](){
          return( std::make_unique< moodycamel::ConcurrentQueue< Item > >(
                   cap ) ); } , []( auto & q , std::size_t m ) {
           for( std::size_t i = 0 ; i < m ; ++i ) q.enqueue( Item( i ) ); } ,
          []( auto & q , std::size_t m ) {
           Item s = 0 , B[ 256 ];
           for( std::size_t got = 0 ; got < m ; ) {
            const std::size_t k = q.try_dequeue_bulk( B , 256 );
            for( std::size_t i = 0 ; i < k ; ++i ) s += B[ i ];
            got += k;
            }
           return( s ); } ) );
#endif
#ifdef ADS_HAS_MOODYCAMEL
  {
   using CQ = moodycamel::ConcurrentQueue< Item >;
   print( "mpsc" , "throughput" , n , "moodycamel::ConcurrentQueue tokens" ,
          transfer( n , r , [ & ](){
            return( std::make_unique< CQ >( cap ) ); } ,
           []( CQ & q , std::size_t m ) {
            moodycamel::ProducerToken t( q );
            for( std::size_t i = 0 ; i < m ; ++i ) q.enqueue( t , Item( i ) );
            } , []( CQ & q , std::size_t m ) {
            moodycamel::ConsumerToken t( q );
            Item s = 0 , B[ 256 ];
            for( std::size_t got = 0 ; got < m ; ) {
             const std::size_t k = q.try_dequeue_bulk( t , B , 256 );
             for( std::size_t i = 0 ; i < k ; ++i ) s += B[ i ];
             got += k;
             }
            return( s ); } ) );
   }
#endif
#ifndef ADS_BENCH_OLD
  {
   using AQ = ADS::MPSCQueue< Item >;
   print( "mpsc" , "throughput" , n , "ADS::MPSCQueue" , transfer( n , r ,
           [](){ return( std::make_unique< AQ >() ); } ,
           []( AQ & q , std::size_t m ) {
            auto p = q.producer();
            for( std::size_t i = 0 ; i < m ; ++i ) p.push( Item( i ) ); } ,
           []( AQ & q , std::size_t m ) {
            Item s = 0 , B[ 256 ];
            for( std::size_t got = 0 ; got < m ; ) {
             const std::size_t k = q.popInto( B , 256 );
             for( std::size_t i = 0 ; i < k ; ++i ) s += B[ i ];
             got += k;
             }
            return( s ); } ) );
   }
#endif
  // ----- mpmc, one producer and one consumer on the same queue type
  auto mpmc = [ & ]( const std::string & who , auto make , auto push ,
                     auto pop ) {
   print( "mpmc" , "throughput" , n , who , transfer( n , r , make ,
           [ & ]( auto & q , std::size_t m ) {
            for( std::size_t i = 0 ; i < m ; ++i )
             while( ! push( q , Item( i ) ) ) {} } ,
           [ & ]( auto & q , std::size_t m ) {
            Item s = 0 , x;
            for( std::size_t i = 0 ; i < m ; ++i ) {
             while( ! pop( q , x ) ) {}
             s += x;
             }
            return( s ); } ) ); };
#ifdef ADS_HAS_BOOST
  for( const char * who : { "boost::lockfree::queue" ,
                            "boost::lockfree::queue A/A" } )
   mpmc( who , [ & ](){
          return( std::make_unique< boost::lockfree::queue< Item ,
                   boost::lockfree::capacity< 1024 > > >() ); } ,
         []( auto & q , Item x ) { return( q.bounded_push( x ) ); } ,
         []( auto & q , Item & x ) { return( q.pop( x ) ); } );
#endif
#ifdef ADS_HAS_MOODYCAMEL
  mpmc( "moodycamel::ConcurrentQueue" , [ & ](){
         return( std::make_unique< moodycamel::ConcurrentQueue< Item > >(
                  cap ) ); } ,
        []( auto & q , Item x ) { return( q.try_enqueue( x ) ); } ,
        []( auto & q , Item & x ) { return( q.try_dequeue( x ) ); } );
#endif
#ifndef ADS_BENCH_OLD
  mpmc( "ADS::MPMCQueue" , [ & ](){
         return( std::make_unique< ADS::MPMCQueue< Item > >( cap ) ); } ,
        []( auto & q , Item x ) { return( q.push( x ) ); } ,
        []( auto & q , Item & x ) { return( q.tryPop( x ) ); } );
#endif
  print( "mpsc" , "throughput" , n , "ADS::MPSCInbox" , transfer( n , r ,
          [](){ return( std::make_unique< ADS::MPSCInbox< Item > >() ); } ,
          sendAll , []( auto & q , std::size_t m ) {
           Item s = 0;
           for( std::size_t got = 0 ; got < m ; ) {
            const auto B = q.take();
            for( Item x : B ) s += x;
            got += B.size();
            }
           return( s ); } ) );
  }
 // ----- the deques of work stealing
 {
  const std::size_t n = o.sizes.front();
  const int r = o.runs;
  struct Locked {
   std::mutex m;
   std::deque< Item > d;
   void push( Item x ) {
    std::lock_guard< std::mutex > g( m );
    d.push_back( x ); }
   bool take( Item & x ) {
    std::lock_guard< std::mutex > g( m );
    if( d.empty() ) return( false );
    x = d.back();
    d.pop_back();
    return( true ); }
   bool steal( Item & x ) {
    std::lock_guard< std::mutex > g( m );
    if( d.empty() ) return( false );
    x = d.front();
    d.pop_front();
    return( true ); }
   };
  auto cells = [ & ]( const std::string & who , auto make , auto push ,
                      auto take , auto steal ) {
   print( "steal" , "owner" , n , who , median( make , [ & ]( auto & q ) {
           Item s = 0 , x;
           for( std::size_t i = 0 ; i < n ; ++i ) push( *q , Item( i ) );
           while( take( *q , x ) ) s += x;
           sink = sink + s; } , r , 2 * n ) );
   print( "steal" , "thief" , n , who , transfer( n , r , make ,
           [ & ]( auto & q , std::size_t m ) {
            for( std::size_t i = 0 ; i < m ; ++i ) push( q , Item( i ) ); } ,
           [ & ]( auto & q , std::size_t m ) {
            Item s = 0 , x;
            for( std::size_t got = 0 ; got < m ; )
             if( steal( q , x ) ) {
              s += x;
              ++got;
              }
            return( s ); } ) ); };
  for( const char * who : { "std::deque + std::mutex" ,
                            "std::deque + std::mutex A/A" } )
   cells( who , [](){ return( std::make_unique< Locked >() ); } ,
          []( Locked & q , Item x ) { q.push( x ); } ,
          []( Locked & q , Item & x ) { return( q.take( x ) ); } ,
          []( Locked & q , Item & x ) { return( q.steal( x ) ); } );
#ifndef ADS_BENCH_OLD
  using WS = ADS::WorkStealingDeque< Item >;
  cells( "ADS::WorkStealingDeque" , [](){
          return( std::make_unique< WS >() ); } ,
         []( WS & q , Item x ) { q.push( x ); } ,
         []( WS & q , Item & x ) {
          auto v = q.take();
          if( v ) x = *v;
          return( v.has_value() ); } ,
         []( WS & q , Item & x ) { return( q.steal( x ) ); } );
#endif
  }
 // ----- the consumer that sleeps
 const std::size_t n = o.sizes.front();
 const int r = o.runs;
#ifndef ADS_BENCH_OLD                       // no waits before
 wake( "ADS::SPSCQueue waitPop" , n , r , [](){
        return( std::make_unique< ADS::SPSCQueue< Item > >( 1024 ) ); } ,
       []( auto & q , Item x ) {
        while( ! q.push( x ) ) {}
        q.notify(); } ,
       []( auto & q ) { Item x; q.waitPop( x ); return( x ); } );
 wake( "ADS::MPSCQueue waitPop" , n , r , [](){
        return( std::make_unique< OneProducer >() ); } ,
       []( OneProducer & q , Item x ) {
        q.p.push( x );
        q.p.notify(); } ,
       []( OneProducer & q ) { Item x; q.q.waitPop( x ); return( x ); } );
 wake( "ADS::MPSCInbox waitTake" , n , r , [](){
        return( std::make_unique< ADS::MPSCInbox< Item > >() ); } ,
       []( auto & q , Item x ) { q.push( x ); } ,
       []( auto & q ) {                     // the batch, one item here
        static thread_local std::vector< Item > B;
        static thread_local std::size_t at = 0;
        if( at == B.size() ) {
         B = q.waitTake();
         at = 0;
         }
        return( B[ at++ ] ); } );
#endif
 wake( "poll + sleep 100 us" , n , r , [](){
        return( std::make_unique< ADS::SPSCQueue< Item > >( 1024 ) ); } ,
       []( auto & q , Item x ) { while( ! q.push( x ) ) {} } ,
       []( auto & q ) {
        Item x;
        while( ! q.tryPop( x ) )
         std::this_thread::sleep_for( std::chrono::microseconds( 100 ) );
        return( x ); } );
 wake( "std::condition_variable" , n , r , [](){
        return( std::make_unique< LockedQueue >() ); } ,
       []( LockedQueue & q , Item x ) {
        {
         std::lock_guard< std::mutex > g( q.m );
         q.d.push_back( x );
         }
        q.cv.notify_one(); } ,
       []( LockedQueue & q ) {
        std::unique_lock< std::mutex > g( q.m );
        q.cv.wait( g , [ & ] { return( ! q.d.empty() ); } );
        const Item x = q.d.front();
        q.d.pop_front();
        return( x ); } );
#ifdef ADS_HAS_MOODYCAMEL
 wake( "moodycamel::BlockingReaderWriterQueue" , n , r , [](){
        return( std::make_unique< moodycamel::BlockingReaderWriterQueue<
                 Item > >( 1024 ) ); } ,
       []( auto & q , Item x ) { q.enqueue( x ); } ,
       []( auto & q ) { Item x; q.wait_dequeue( x ); return( x ); } );
 wake( "moodycamel::BlockingConcurrentQueue" , n , r , [](){
        return( std::make_unique< moodycamel::BlockingConcurrentQueue<
                 Item > >( 1024 ) ); } ,
       []( auto & q , Item x ) { q.enqueue( x ); } ,
       []( auto & q ) { Item x; q.wait_dequeue( x ); return( x ); } );
#endif
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File concurrent.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
