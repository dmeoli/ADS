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
 * (e.g., taskset -c 14,15):
 *
 * - "spsc", case throughput: a producer pushes n integers into a queue of
 *   1024 places, retrying when it is full, while a consumer pops them,
 *   retrying when it is empty (ns per item, the time of the consumer);
 *   case pingpong: n round trips of an integer through two queues (ns per
 *   round trip);
 * - "mpsc", case throughput: one producer sends n integers, one consumer
 *   takes them (ns per item); the inbox takes all the messages at once,
 *   the others pop by batches of up to 256 where they can.
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
#include "ADS/concurrent/SPSCQueue.h"

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

/// the k-th processor that the process may use
int cpu( int k )
{
 cpu_set_t s;
 CPU_ZERO( &s );
 sched_getaffinity( 0 , sizeof( s ) , &s );
 for( int c = 0 , seen = 0 ; c < CPU_SETSIZE ; ++c )
  if( CPU_ISSET( c , &s ) && ( seen++ == k ) )
   return( c );
 return( 0 );
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

}  // end( namespace )

int main( int argc , char ** argv )
{
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
        return( std::make_unique< Rwq >( cap ) ); } ,
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
        []( Ads & q , Item & x ) { return( q.tryPop( x ) ); } );

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
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File concurrent.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
