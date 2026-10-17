/*--------------------------------------------------------------------------*/
/*--------------------------- File priority.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the priority queues: the heaps and the bucket queue of
 * ADS against std::priority_queue and, if Boost is found, the d-ary,
 * Fibonacci and pairing heaps of Boost.Heap, on the "hold" model of the
 * simulations and of Dijkstra's algorithm: n insertions of random
 * priorities in [ 0 , n ), then n steps that extract the minimum p and
 * insert p + U( 0 , 1000 ), then n extractions. The priorities never fall
 * below the last minimum, which the bucket queue needs.
 *
 * Each cell is the median of r runs of a phase, in nanoseconds per
 * operation (a step of the hold is one operation); the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   priority [ -r runs ] [ -s seed ] n1 n2 ... *
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

#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/TreePriorityQueue.h"

#if __has_include( <boost/heap/d_ary_heap.hpp> )
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/fibonacci_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #define ADS_HAS_BOOST 1
#endif

#include <functional>
#include <queue>
#include <utility>

using namespace ADS::bench;
using Pair = std::pair< int , int >;        // ( priority , item )

namespace {

/// the three phases on the queue that make() builds, by the operations
/// push( q , p , x ) and pop( q ) (the priority of the minimum, removed)
template< class M , class Push , class Pop >
void run( const std::string & who , std::size_t n ,
          const std::vector< int > & P , const std::vector< int > & U , int r ,
          M && make , Push && push , Pop && pop )
{
 auto fill = [ & ]( void ) {
  auto q = make();
  for( std::size_t i = 0 ; i < n ; ++i ) push( q , P[ i ] , int( i ) );
  return( q );
  };
 auto hold = [ & ]( auto & q ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   push( q , pop( q ) + U[ i ] , int( i ) );
  };
 print( "priority" , "fill" , n , who , median( [ & ](){ return( 0 ); } ,
         [ & ]( int ) { auto q = fill(); sink = sink + pop( q ); } , r ,
         n ) );
 print( "priority" , "hold" , n , who , median( fill , hold , r , n ) );
 print( "priority" , "drain" , n , who , median( [ & ](){
         auto q = fill();
         hold( q );
         return( q ); } , [ & ]( auto & q ) {
          long s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s += pop( q );
          sink = sink + std::uint64_t( s ); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  std::vector< int > P( n ) , U( n );
  for( auto & p : P ) p = int( g() % n );
  for( auto & u : U ) u = int( g() % 1001 );
  const int r = o.runs;
  using StdQ = std::priority_queue< Pair , std::vector< Pair > ,
                                    std::greater< Pair > >;
  auto stdPush = []( auto & q , int p , int x ) { q.emplace( p , x ); };
  auto stdPop = []( auto & q ) {
   const int p = q.top().first;
   q.pop();
   return( p ); };
  for( const char * who : { "std::priority_queue" ,
                            "std::priority_queue A/A" } )
   run( who , n , P , U , r , [](){ return( StdQ() ); } , stdPush , stdPop );
#ifdef ADS_HAS_BOOST
  using Greater = boost::heap::compare< std::greater< Pair > >;
  run( "boost::heap::d_ary_heap d=4" , n , P , U , r , [](){
       return( boost::heap::d_ary_heap< Pair , boost::heap::arity< 4 > ,
                                        Greater >() ); } , stdPush , stdPop );
  run( "boost::heap::fibonacci_heap" , n , P , U , r , [](){
       return( boost::heap::fibonacci_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
  run( "boost::heap::pairing_heap" , n , P , U , r , [](){
       return( boost::heap::pairing_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
#endif
  run( "ADS::DaryHeap d=4" , n , P , U , r , [](){
       return( ADS::DaryHeap< Pair , std::less< Pair > , 4 >() ); } ,
       []( auto & q , int p , int x ) { q.push( Pair( p , x ) ); } ,
       []( auto & q ) {
        const int p = q.top().first;
        q.pop();
        return( p ); } );
  auto adsPush = []( auto & q , int p , int x ) { q.insert( x , p ); };
  auto adsPop = []( auto & q ) {
   const int p = q.minPriority();
   q.deleteMin();
   return( p ); };
  run( "ADS::HeapPriorityQueue" , n , P , U , r , [ n ](){
       return( ADS::HeapPriorityQueue< int , int >( int( n ) ) ); } ,
       adsPush , adsPop );
  run( "ADS::LinkedHeapPriorityQueue" , n , P , U , r , [](){
       return( ADS::LinkedHeapPriorityQueue< int , int >() ); } , adsPush ,
       adsPop );
  run( "ADS::FibonacciHeapPriorityQueue" , n , P , U , r , [](){
       return( ADS::FibonacciHeapPriorityQueue< int , int >() ); } ,
       adsPush , adsPop );
  run( "ADS::TreePriorityQueue" , n , P , U , r , [](){
       return( ADS::TreePriorityQueue< int , int >() ); } , adsPush ,
       []( auto & q ) {
        const int p = ( *q.begin() ).first.first;
        q.deleteMin();
        return( p ); } );
  run( "ADS::BucketQueue" , n , P , U , r , [](){
       return( ADS::BucketQueue< int >() ); } ,
       []( auto & q , int p , int x ) { q.push( x , p , 0 ); } ,
       []( auto & q ) {
        const int p = int( q.topF() );
        q.pop();
        return( p ); } );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File priority.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
