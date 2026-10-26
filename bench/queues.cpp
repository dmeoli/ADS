/*--------------------------------------------------------------------------*/
/*---------------------------- File queues.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the queues, of the double ended queues and of the
 * stacks of ADS against std::deque (behind std::queue and std::stack),
 * std::vector and, if Boost is found, boost::circular_buffer (its capacity
 * doubled when full) and boost::container::devector, on n integers of 64
 * bits:
 *
 * - "queue": n insertions at the back and then n removals from the front
 *   (case fill-drain, ns per item), and n steps that insert one item and
 *   remove one on a queue of 1000 items (case steady, ns per step);
 * - "deque": n random operations at random ends, three insertions for two
 *   removals (case mixed, ns per operation);
 * - "stack": n insertions on the top and then n removals (case push-pop,
 *   ns per item).
 *
 * The book's ArrayQueue and ArrayStack get a capacity of n at the start,
 * which they need. The memory freed goes back to the heap of malloc and
 * never to the system (mallopt), so that no structure pays the faults of
 * pages that another one did not, depending on the order of the cells and
 * on the thresholds that malloc adapts. Each cell is the median of r runs;
 * the contracts are off (ADS_CONTRACTS=0).
 *
 *   queues [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/MultiStack.h"
#ifndef ADS_BENCH_OLD
 #include "ADS/queue/RingDeque.h"
#endif

#if __has_include( <boost/circular_buffer.hpp> )
 #include <boost/circular_buffer.hpp>
 #include <boost/container/devector.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <deque>
#include <malloc.h>
#include <queue>
#include <stack>

using namespace ADS::bench;
using Key = std::uint64_t;

namespace {

#ifdef ADS_HAS_BOOST
/// a circular_buffer whose capacity doubles when full
struct GrowingBuffer {
 boost::circular_buffer< Key > b;
 GrowingBuffer( void ) : b( 8 ) {}
 void back( Key k ) {
  if( b.full() ) b.set_capacity( 2 * b.capacity() );
  b.push_back( k );
  }
 void front( Key k ) {
  if( b.full() ) b.set_capacity( 2 * b.capacity() );
  b.push_front( k );
  }
 };
#endif

/// the queue cells on the queue that make() builds, by push( q , k ) and
/// pop( q ), which returns the item removed
template< class M , class Push , class Pop >
void queue( const std::string & who , std::size_t n , int r , M && make ,
            Push && push , Pop && pop )
{
 if( skip( who ) )
  return;
 print( "queue" , "fill-drain" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto q = make( n );
          for( std::size_t i = 0 ; i < n ; ++i ) push( q , Key( i ) );
          Key s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s += pop( q );
          sink = sink + s; } , r , n ) );
 print( "queue" , "steady" , n , who , median( [ & ](){
         auto q = make( 1001 );
         for( std::size_t i = 0 ; i < 1000 ; ++i ) push( q , Key( i ) );
         return( q ); } , [ & ]( auto & q ) {
          Key s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) {
           push( q , Key( i ) );
           s += pop( q );
           }
          sink = sink + s; } , r , n ) );
 }

/// the deque cell: op[ i ] is 0 , 1 (insertions at the back and at the
/// front), 2 , 3 (removals), a removal on an empty deque skipped
template< class M , class F >
void deque( const std::string & who , const std::vector< char > & op ,
            int r , M && make , F && apply )
{
 if( skip( who ) )
  return;
 print( "deque" , "mixed" , op.size() , who , median( make ,
         [ & ]( auto & d ) {
         Key s = 0;
         for( std::size_t i = 0 ; i < op.size() ; ++i )
          s += apply( d , op[ i ] , Key( i ) );
         sink = sink + s; } , r , op.size() ) );
 }

/// the stack cell
template< class M , class Push , class Pop >
void stack( const std::string & who , std::size_t n , int r , M && make ,
            Push && push , Pop && pop )
{
 if( skip( who ) )
  return;
 print( "stack" , "push-pop" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto q = make( n );
          for( std::size_t i = 0 ; i < n ; ++i ) push( q , Key( i ) );
          Key s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s += pop( q );
          sink = sink + s; } , r , n ) );
 }

/// the deque cell of a structure with the names of std
template< class D >
Key stdApply( D & d , char o , Key k )
{
 switch( o ) {
  case( 0 ): d.push_back( k ); return( 0 );
  case( 1 ): d.push_front( k ); return( 0 );
  case( 2 ): {
   if( d.empty() ) return( 0 );
   const Key x = d.back();
   d.pop_back();
   return( x );
   }
  default: {
   if( d.empty() ) return( 0 );
   const Key x = d.front();
   d.pop_front();
   return( x );
   }
  }
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 mallopt( M_MMAP_THRESHOLD , 1 << 30 );    // the heap only, never trimmed
 mallopt( M_TRIM_THRESHOLD , 1 << 30 );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  // ----- queue
  auto stdPush = []( auto & q , Key k ) { q.push_back( k ); };
  auto stdPop = []( auto & q ) {
   const Key x = q.front();
   q.pop_front();
   return( x ); };
  for( const char * who : { "std::deque" , "std::deque A/A" } )
   queue( who , n , r , []( std::size_t ) { return( std::deque< Key >() ); } ,
          stdPush , stdPop );
#ifdef ADS_HAS_BOOST
  queue( "boost::circular_buffer" , n , r ,
         []( std::size_t ) { return( GrowingBuffer() ); } ,
         []( auto & q , Key k ) { q.back( k ); } , []( auto & q ) {
          const Key x = q.b.front();
          q.b.pop_front();
          return( x ); } );
  queue( "boost::container::devector" , n , r , []( std::size_t ) {
         return( boost::container::devector< Key >() ); } , stdPush ,
         stdPop );
#endif
  queue( "ADS::ArrayQueue" , n , r , []( std::size_t m ) {
         return( ADS::ArrayQueue< Key >( m ) ); } ,
         []( auto & q , Key k ) { q.enqueue( k ); } ,
         []( auto & q ) { return( q.dequeue() ); } );
  queue( "ADS::LinkedQueue" , n , r , []( std::size_t ) {
         return( ADS::LinkedQueue< Key >() ); } ,
         []( auto & q , Key k ) { q.enqueue( k ); } ,
         []( auto & q ) { return( q.dequeue() ); } );
  queue( "ADS::Dequeue" , n , r , []( std::size_t ) {
         return( ADS::Dequeue< Key >() ); } ,
         []( auto & q , Key k ) { q.insertTail( k ); } ,
         []( auto & q ) { return( q.removeHead() ); } );
#ifndef ADS_BENCH_OLD
  queue( "ADS::RingDeque" , n , r , []( std::size_t ) {
         return( ADS::RingDeque< Key >() ); } , stdPush , stdPop );
#endif

  // ----- deque
  std::mt19937_64 g( o.seed );
  std::vector< char > op( n );
  for( auto & c : op ) {
   const unsigned u = unsigned( g() % 5 );  // 3 insertions for 2 removals
   c = char( u < 3 ? u % 2 : 2 + u % 2 );
   }
  for( const char * who : { "std::deque" , "std::deque A/A" } )
   deque( who , op , r , [](){ return( std::deque< Key >() ); } ,
          stdApply< std::deque< Key > > );
#ifdef ADS_HAS_BOOST
  deque( "boost::container::devector" , op , r , [](){
         return( boost::container::devector< Key >() ); } ,
         stdApply< boost::container::devector< Key > > );
  deque( "boost::circular_buffer" , op , r , [](){
         return( GrowingBuffer() ); } , []( GrowingBuffer & d , char c ,
                                            Key k ) {
          if( c == 0 ) { d.back( k ); return( Key( 0 ) ); }
          if( c == 1 ) { d.front( k ); return( Key( 0 ) ); }
          return( stdApply( d.b , c , k ) ); } );
#endif
  deque( "ADS::Dequeue" , op , r , [](){ return( ADS::Dequeue< Key >() ); } ,
         []( ADS::Dequeue< Key > & d , char c , Key k ) {
          switch( c ) {
           case( 0 ): d.insertTail( k ); return( Key( 0 ) );
           case( 1 ): d.insertHead( k ); return( Key( 0 ) );
           case( 2 ): return( d.begin() == d.end() ? 0 : d.removeTail() );
           default: return( d.begin() == d.end() ? 0 : d.removeHead() );
           } } );
#ifndef ADS_BENCH_OLD
  deque( "ADS::RingDeque" , op , r , [](){
         return( ADS::RingDeque< Key >() ); } ,
         stdApply< ADS::RingDeque< Key > > );
#endif

  // ----- stack
  for( const char * who : { "std::vector" , "std::vector A/A" } )
   stack( who , n , r , []( std::size_t ) {
          return( std::stack< Key , std::vector< Key > >() ); } ,
          []( auto & s , Key k ) { s.push( k ); } , []( auto & s ) {
           const Key x = s.top();
           s.pop();
           return( x ); } );
  stack( "std::deque" , n , r , []( std::size_t ) {
         return( std::stack< Key >() ); } ,
         []( auto & s , Key k ) { s.push( k ); } , []( auto & s ) {
          const Key x = s.top();
          s.pop();
          return( x ); } );
#ifdef ADS_HAS_BOOST
  stack( "boost::container::devector" , n , r , []( std::size_t ) {
         return( boost::container::devector< Key >() ); } ,
         []( auto & s , Key k ) { s.push_back( k ); } , []( auto & s ) {
          const Key x = s.back();
          s.pop_back();
          return( x ); } );
#endif
  stack( "ADS::ArrayStack" , n , r , []( std::size_t m ) {
         return( ADS::ArrayStack< Key >( m ) ); } ,
         []( auto & s , Key k ) { s.push( k ); } ,
         []( auto & s ) { return( s.pop() ); } );
  stack( "ADS::MultiStack" , n , r , []( std::size_t ) {
         return( ADS::MultiStack< Key >( 1 ) ); } ,
         []( auto & s , Key k ) { s.push( 0 , k ); } ,
         []( auto & s ) { return( s.pop( 0 ) ); } );
#ifndef ADS_BENCH_OLD
  stack( "ADS::RingDeque" , n , r , []( std::size_t ) {
         return( ADS::RingDeque< Key >() ); } ,
         []( auto & s , Key k ) { s.push_back( k ); } , []( auto & s ) {
          const Key x = s.back();
          s.pop_back();
          return( x ); } );
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File queues.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
