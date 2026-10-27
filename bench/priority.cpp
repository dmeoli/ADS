/*--------------------------------------------------------------------------*/
/*--------------------------- File priority.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the priority queues: the heaps, the sequence heap, the
 * bucket queues and the radix heap of ADS against std::priority_queue,
 * against the d-ary, binomial, Fibonacci, pairing and skew heaps of
 * Boost.Heap if Boost is found, and against the d-ary heaps and the radix
 * heap of tlx and the radix heap of radix-heap (iwiwi) when their headers
 * are found (PRIORITY in the makefile), in two models:
 *
 * - the "hold" model of the simulations: n insertions of random priorities
 *   in [ 0 , n ) (fill), then n steps that extract the minimum p and insert
 *   p + U( 0 , 1000 ) (hold), then n extractions (drain); the priorities
 *   never fall below the last minimum, which the bucket queue and the
 *   radix heaps need;
 * - the double-ended queues, the min-max heap and the interval heap,
 *   against std::multiset: n insertions (fill), then n steps that remove
 *   the best or the worst item, in turn, and insert a random one (both
 *   ends), then n removals from both ends in turn (both drain);
 * - Dijkstra's algorithm on a random graph of n nodes and 4 n arcs of
 *   weights U( 1 , 1000 ) (dijkstra): the queues with handles lower the
 *   priority of a node by their decrease-key, the others push it again and
 *   skip the stale copies (lazy deletion); all must give the same
 *   distances;
 * - the pushes and pops of best-first searches by the keys f = g + h and
 *   g, least f first, then largest g, then last pushed, recorded once and
 *   replayed on the bucket queue and on two heaps, which must pop the same
 *   items: on a grid of side 2 sqrt( n ) with 20% of its cells blocked, 4
 *   neighbours, unit costs, h the Manhattan distance to a corner walled
 *   off, so that the search visits the whole component and g reaches a few
 *   thousands, with one queue (grid 1 queue) and with 64 queues served in
 *   turn, each node pushed to the queue of a hash of it, as a parallel
 *   search sends its nodes to the workers (grid 64 queues: each queue
 *   holds few items per f, and its least layer empties and fills again
 *   all the time), and on the 15-puzzle from a random walk of 200 moves,
 *   up to n pops, h the Manhattan distance, where g stays below 80
 *   (15-puzzle 1 queue, 15-puzzle 64 queues).
 *
 * Each cell is the median of r runs of a phase, in nanoseconds per
 * operation (a step of the hold is one operation, a node of the graph one
 * of dijkstra, a push or a pop one of the searches); the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   priority [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/DialQueue.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/MinMaxHeap.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/priority/SequenceHeap.h"
#include "ADS/priority/TreePriorityQueue.h"

#if __has_include( <boost/heap/d_ary_heap.hpp> )
 #include <boost/heap/binomial_heap.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/fibonacci_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/heap/skew_heap.hpp>
 #define ADS_HAS_BOOST 1
#endif
#if __has_include( <tlx/container/radix_heap.hpp> )
 #include <tlx/container/d_ary_addressable_int_heap.hpp>
 #include <tlx/container/d_ary_heap.hpp>
 #include <tlx/container/radix_heap.hpp>
 #define ADS_HAS_TLX 1
#endif
#if __has_include( <radix_heap.h> )
 #include <radix_heap.h>
 #define ADS_HAS_IWIWI 1
#endif

#include <climits>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <queue>
#include <set>
#include <unordered_map>
#include <utility>

using namespace ADS::bench;
using Pair = std::pair< int , int >;        // ( priority , item )

namespace {

/*--------------------------------------------------------------------------*/
/*---------------------------- THE HOLD MODEL ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the three phases on the queue that make() builds, by the operations
/// push( q , p , x ) and pop( q ) (the priority of the minimum, removed)
template< class M , class Push , class Pop >
void run( const std::string & who , std::size_t n ,
          const std::vector< int > & P , const std::vector< int > & U , int r ,
          M && make , Push && push , Pop && pop )
{
 if( skip( who ) )
  return;
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

/*--------------------------------------------------------------------------*/
/*------------------------------ BOTH ENDS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the three phases of both ends on the queue that make() builds, by
/// push( q , x ), low( q ) and high( q ) (the best and the worst removed)
template< class M , class Push , class Low , class High >
void ends( const std::string & who , std::size_t n ,
           const std::vector< int > & P , int r , M && make , Push && push ,
           Low && low , High && high )
{
 if( skip( who ) )
  return;
 auto fill = [ & ]( void ) {
  auto q = make();
  for( std::size_t i = 0 ; i < n ; ++i ) push( q , P[ i ] );
  return( q );
  };
 print( "priority" , "fill both ends" , n , who , median(
         [ & ](){ return( 0 ); } , [ & ]( int ) {
          auto q = fill();
          sink = sink + std::uint64_t( low( q ) ); } , r , n ) );
 auto hold = [ & ]( auto & q ) {
  std::uint64_t s = 0;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   s += std::uint64_t( i % 2 ? high( q ) : low( q ) );
   push( q , P[ n - 1 - i ] );
   }
  sink = sink + s; };
 print( "priority" , "hold both ends" , n , who , median( fill , hold , r ,
                                                           n ) );
 print( "priority" , "drain both ends" , n , who , median( [ & ](){
         auto q = fill();
         hold( q );
         return( q ); } , [ & ]( auto & q ) {
          std::uint64_t s = 0;
          for( std::size_t i = 0 ; i < n ; ++i )
           s += std::uint64_t( i % 2 ? high( q ) : low( q ) );
          sink = sink + s; } , r , n ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- DIJKSTRA ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph in arrays: the arcs of v are [ first[ v ] , first[ v + 1 ] )
struct Graph {
 std::vector< unsigned > first , head;
 std::vector< int > w;
 };

/// n nodes, 4 random arcs out of each, weights U( 1 , 1000 )
Graph graph( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed + 7 );
 Graph G;
 G.first.resize( n + 1 );
 for( std::size_t v = 0 ; v < n ; ++v ) {
  G.first[ v ] = unsigned( G.head.size() );
  for( int k = 0 ; k < 4 ; ++k ) {
   G.head.push_back( unsigned( g() % n ) );
   G.w.push_back( int( 1 + g() % 1000 ) );
   }
  }
 G.first[ n ] = unsigned( G.head.size() );
 return( G );
 }

/// the sum of the finite distances, which every queue must give
std::uint64_t checksum( const std::vector< int > & d )
{
 std::uint64_t s = 0;
 for( int x : d )
  if( x != INT_MAX ) s += std::uint64_t( x ) + 1;
 return( s );
 }

/// Dijkstra's algorithm from the node 0 with lazy deletion: push( q , d , v )
/// and pop( q , d , v ), which removes the minimum into d and v
template< class Q , class Push , class Pop , class Empty >
std::uint64_t lazy( const Graph & G , Q & q , Push && push , Pop && pop ,
                    Empty && empty )
{
 std::vector< int > dist( G.first.size() - 1 , INT_MAX );
 dist[ 0 ] = 0;
 push( q , 0 , 0u );
 while( ! empty( q ) ) {
  int d;
  unsigned v;
  pop( q , d , v );
  if( d > dist[ v ] ) continue;             // a stale copy
  for( unsigned a = G.first[ v ] ; a < G.first[ v + 1 ] ; ++a ) {
   const unsigned u = G.head[ a ];
   const int e = d + G.w[ a ];
   if( e < dist[ u ] ) {
    dist[ u ] = e;
    push( q , e , u );
    }
   }
  }
 return( checksum( dist ) );
 }

/// Dijkstra's algorithm from the node 0 with the handles H of the queue:
/// ins( q , d , v ) gives a handle, dec( q , h , d , v ) lowers it to d,
/// pop( q , d , v ) removes the minimum into d and v
template< class H , class Q , class Ins , class Dec , class Pop ,
          class Empty >
std::uint64_t handles( const Graph & G , Q & q , Ins && ins , Dec && dec ,
                       Pop && pop , Empty && empty )
{
 const std::size_t n = G.first.size() - 1;
 std::vector< int > dist( n , INT_MAX );
 std::vector< H > h( n );
 std::vector< char > in( n , 0 );
 dist[ 0 ] = 0;
 h[ 0 ] = ins( q , 0 , 0u );
 in[ 0 ] = 1;
 while( ! empty( q ) ) {
  int d;
  unsigned v;
  pop( q , d , v );
  in[ v ] = 2;                              // settled
  for( unsigned a = G.first[ v ] ; a < G.first[ v + 1 ] ; ++a ) {
   const unsigned u = G.head[ a ];
   const int e = d + G.w[ a ];
   if( e < dist[ u ] ) {
    dist[ u ] = e;
    if( in[ u ] == 1 )
     dec( q , h[ u ] , e , u );
    else {
     h[ u ] = ins( q , e , u );
     in[ u ] = 1;
     }
    }
   }
  }
 return( checksum( dist ) );
 }

/// the reference distances, those of the first queue timed
std::uint64_t reference = 0;

/// times f( G ), a run of Dijkstra's algorithm, and checks its distances
template< class F >
void dijkstra( const std::string & who , const Graph & G , int r , F && f )
{
 if( skip( who ) )
  return;
 const std::size_t n = G.first.size() - 1;
 print( "priority" , "dijkstra" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          const std::uint64_t s = f( G );
          if( ! reference ) reference = s;
          if( s != reference ) {
           std::fprintf( stderr , "%s: wrong distances\n" , who.c_str() );
           std::exit( 1 );
           }
          sink = sink + s; } , r , n ) );
 }

/// the lazy Dijkstra on a queue of pairs ( priority , node ) of the
/// interface of std::priority_queue, the least on top
template< class Q >
std::uint64_t stdLazy( const Graph & G )
{
 Q q;
 return( lazy( G , q , []( Q & q , int d , unsigned v ) {
                q.push( std::pair< int , unsigned >( d , v ) ); } ,
               []( Q & q , int & d , unsigned & v ) {
                d = q.top().first;
                v = q.top().second;
                q.pop(); } ,
               []( Q & q ) { return( q.empty() ); } ) );
 }

/// Dijkstra on a priority queue of ADS with handles
template< class Q , class M >
std::uint64_t adsHandles( const Graph & G , M && make )
{
 Q q = make();
 return( handles< typename Q::Handle >( G , q ,
          []( Q & q , int d , unsigned v ) { return( q.insert( v , d ) ); } ,
          []( Q & q , typename Q::Handle h , int d , unsigned ) {
           q.decrease( h , d ); } ,
          []( Q & q , int & d , unsigned & v ) {
           d = q.minPriority();
           v = q.deleteMin(); } ,
          []( Q & q ) { return( q.empty() ); } ) );
 }

#ifdef ADS_HAS_BOOST
/// Dijkstra on a mutable heap of Boost of pairs, the least on top by
/// std::greater, a lower distance an increase of the priority
template< class Q >
std::uint64_t boostHandles( const Graph & G )
{
 using P = std::pair< int , unsigned >;
 Q q;
 return( handles< typename Q::handle_type >( G , q ,
          []( Q & q , int d , unsigned v ) {
           return( q.push( P( d , v ) ) ); } ,
          []( Q & q , typename Q::handle_type h , int d , unsigned v ) {
           q.increase( h , P( d , v ) ); } ,
          []( Q & q , int & d , unsigned & v ) {
           d = q.top().first;
           v = q.top().second;
           q.pop(); } ,
          []( Q & q ) { return( q.empty() ); } ) );
 }
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- BEST-FIRST SEARCHES ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a step of a search: x pushed to the queue q with the keys f and g, or
/// popped from it if g is NONE
struct Step {
 unsigned q , x , f , g;
 };

constexpr unsigned NONE = UINT_MAX;

/// the queue of the node v among P
unsigned owner( std::uint64_t v , unsigned P )
{
 return( unsigned( ( ( v * 0x9E3779B97F4A7C15ull ) >> 32 ) % P ) );
 }

/// the steps of a best-first search on P queues served in turn, a pop from
/// each that has items, until the queues empty, the goal comes out or
/// cap pops: start() gives the first node with its h, expand( v , g ,
/// out ) calls out( u , h ) for each neighbour u of v, goal( v ) tells
/// the goal; a node is pushed again when its g improves, and its stale
/// copies skipped
template< class Start , class Expand , class Goal >
std::vector< Step > bestFirst( unsigned P , std::size_t cap , Start && start ,
                               Expand && expand , Goal && goal )
{
 std::vector< ADS::BucketQueue< unsigned , unsigned > > Q( P );
 std::vector< unsigned > best;
 std::vector< Step > T;
 auto push = [ & ]( unsigned u , unsigned g , unsigned h ) {
  if( u >= best.size() ) best.resize( u + 1 , NONE );
  if( g >= best[ u ] ) return;
  best[ u ] = g;
  const unsigned q = owner( u , P );
  Q[ q ].push( u , g + h , g );
  T.push_back( Step{ q , u , g + h , g } );
  };
 const auto [ s , h ] = start();
 push( s , 0 , h );
 for( std::size_t pops = 0 ; pops < cap ; ) {
  bool any = false;
  for( unsigned q = 0 ; q < P ; ++q ) {
   if( Q[ q ].empty() ) continue;
   any = true;
   const auto e = Q[ q ].popEntry();
   T.push_back( Step{ q , e.item , 0 , NONE } );
   ++pops;
   if( e.g > best[ e.item ] ) continue;     // a stale copy
   if( goal( e.item ) ) return( T );
   expand( e.item , [ & ]( unsigned u , unsigned h ) {
    push( u , e.g + 1 , h ); } );
   }
  if( ! any ) break;
  }
 return( T );
 }

/// the search on a grid of side c, 20% of the cells blocked, from the
/// corner 0 to the opposite one, walled off
std::vector< Step > gridSearch( unsigned c , unsigned P , unsigned seed )
{
 std::mt19937_64 g( seed + 11 );
 const std::size_t N = std::size_t( c ) * c;
 std::vector< char > blocked( N );
 for( auto & b : blocked ) b = ( g() % 5 == 0 );
 blocked[ 0 ] = 0;
 blocked[ N - 2 ] = blocked[ N - 1 - c ] = 1;   // the goal walled off
 auto h = [ c ]( std::size_t v ) {
  return( unsigned( ( c - 1 - v % c ) + ( c - 1 - v / c ) ) ); };
 return( bestFirst( P , std::size_t( -1 ) ,
  [ & ](){ return( std::pair< unsigned , unsigned >( 0 , h( 0 ) ) ); } ,
  [ & ]( unsigned v , auto && out ) {
   const unsigned x = v % c , y = v / c;
   if( x > 0 && ! blocked[ v - 1 ] ) out( v - 1 , h( v - 1 ) );
   if( x + 1 < c && ! blocked[ v + 1 ] ) out( v + 1 , h( v + 1 ) );
   if( y > 0 && ! blocked[ v - c ] ) out( v - c , h( v - c ) );
   if( y + 1 < c && ! blocked[ v + c ] ) out( v + c , h( v + c ) ); } ,
  [ N ]( unsigned v ) { return( v == N - 1 ); } ) );
 }

/// the search on the 15-puzzle from a random walk of 200 moves from the
/// goal, up to cap pops; a state is 16 nibbles, the tile of each place
std::vector< Step > puzzleSearch( std::size_t cap , unsigned P ,
                                  unsigned seed )
{
 using State = std::uint64_t;
 auto tile = []( State s , int i ) { return( int( ( s >> ( 4 * i ) ) & 15 ) );
  };
 auto blank = [ & ]( State s ) {
  int i = 0;
  while( tile( s , i ) ) ++i;
  return( i ); };
 auto slide = [ & ]( State s , int b , int t ) {    // the tile at t to b
  const State v = State( tile( s , t ) );
  return( ( s & ~( State( 15 ) << ( 4 * t ) ) ) | ( v << ( 4 * b ) ) ); };
 auto h = [ & ]( State s ) {
  unsigned m = 0;
  for( int i = 0 ; i < 16 ; ++i )
   if( const int t = tile( s , i ) - 1 ; t >= 0 )
    m += unsigned( std::abs( t % 4 - i % 4 ) + std::abs( t / 4 - i / 4 ) );
  return( m ); };
 auto moves = []( int b , int * to ) {
  int k = 0;
  if( b % 4 ) to[ k++ ] = b - 1;
  if( b % 4 < 3 ) to[ k++ ] = b + 1;
  if( b >= 4 ) to[ k++ ] = b - 4;
  if( b < 12 ) to[ k++ ] = b + 4;
  return( k ); };
 State goal = 0;
 for( int i = 0 ; i < 15 ; ++i ) goal |= State( i + 1 ) << ( 4 * i );
 std::mt19937_64 g( seed + 13 );
 State s = goal;
 for( int i = 0 , last = -1 ; i < 200 ; ++i ) {
  int to[ 4 ] , t;
  const int b = blank( s ) , k = moves( b , to );
  do t = to[ g() % unsigned( k ) ]; while( t == last );
  last = b;
  s = slide( s , b , t );
  }
 std::unordered_map< State , unsigned > id;
 std::vector< State > state;
 auto number = [ & ]( State x ) {
  const auto [ it , added ] = id.try_emplace( x , unsigned( state.size() ) );
  if( added ) state.push_back( x );
  return( it->second ); };
 return( bestFirst( P , cap ,
  [ & ](){ return( std::pair< unsigned , unsigned >( number( s ) ,
                                                      h( s ) ) ); } ,
  [ & ]( unsigned v , auto && out ) {
   const State x = state[ v ];
   int to[ 4 ];
   const int b = blank( x ) , k = moves( b , to );
   for( int i = 0 ; i < k ; ++i ) {
    const State y = slide( x , b , to[ i ] );
    out( number( y ) , h( y ) );
    } } ,
  [ & ]( unsigned v ) { return( state[ v ] == goal ); } ) );
 }

/// a node of the heaps: its keys, its time of push and itself
struct Node {
 unsigned f , g , t , x;
 };

/// whether a comes out before b: least f, largest g, last pushed
struct NodeFirst {
 bool operator()( const Node & a , const Node & b ) const {
  return( ( a.f < b.f ) || ( ( a.f == b.f ) && ( ( a.g > b.g ) ||
          ( ( a.g == b.g ) && ( a.t > b.t ) ) ) ) );
  }
 };

/// replays the steps T on P queues that make() builds, by push( q , x , f
/// , g , t ) and pop( q ), which must give the items of the steps
template< class M , class Push , class Pop >
void replay( const char * what , const std::string & who , std::size_t n ,
             const std::vector< Step > & T , unsigned P , int r , M && make ,
             Push && push , Pop && pop )
{
 if( skip( who ) )
  return;
 print( "priority" , what , n , who , median( [ & ](){
         std::vector< decltype( make() ) > Q;
         for( unsigned q = 0 ; q < P ; ++q ) Q.push_back( make() );
         return( Q ); } , [ & ]( auto & Q ) {
          unsigned t = 0;
          bool same = true;
          for( const Step & s : T )
           if( s.g == NONE ) same &= ( pop( Q[ s.q ] ) == s.x );
           else push( Q[ s.q ] , s.x , s.f , s.g , t++ );
          if( ! same ) {
           std::fprintf( stderr , "%s: wrong order\n" , who.c_str() );
           std::exit( 1 );
           } } , r , T.size() ) );
 }

/// the structures of the searches: the bucket queue, its A/A, two heaps
const char * const searchers[] = { "ADS::BucketQueue" ,
                                   "ADS::BucketQueue A/A" ,
                                   "std::priority_queue" ,
                                   "ADS::DaryHeap d=4" };

/// the searches of the steps that make() records, which it records only if
/// a structure is to be measured
template< class M >
void searches( const char * what , std::size_t n , unsigned P , int r ,
               M && make )
{
 bool any = false;
 for( const char * who : searchers )
  any = ! skip( who ) || any;
 if( ! any )
  return;
 const std::vector< Step > T = make();
 for( const char * who : { searchers[ 0 ] , searchers[ 1 ] } )
  replay( what , who , n , T , P , r , [](){
          return( ADS::BucketQueue< unsigned , unsigned >() ); } ,
          []( auto & q , unsigned x , unsigned f , unsigned g , unsigned ) {
           q.push( x , f , g ); } ,
          []( auto & q ) { return( q.pop() ); } );
 auto push = []( auto & q , unsigned x , unsigned f , unsigned g ,
                 unsigned t ) { q.push( Node{ f , g , t , x } ); };
 auto pop = []( auto & q ) {
  const unsigned x = q.top().x;
  q.pop();
  return( x ); };
 replay( what , searchers[ 2 ] , n , T , P , r , [](){
         auto later = []( const Node & a , const Node & b ) {
          return( NodeFirst()( b , a ) ); };
         return( std::priority_queue< Node , std::vector< Node > ,
                                      decltype( later ) >( later ) ); } ,
         push , pop );
 replay( what , searchers[ 3 ] , n , T , P , r , [](){
         return( ADS::DaryHeap< Node , NodeFirst , 4 >() ); } , push , pop );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  std::vector< int > P( n ) , U( n );
  for( auto & p : P ) p = int( g() % n );
  for( auto & u : U ) u = int( g() % 1001 );
  const int r = o.runs;

  // the hold model

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
  run( "boost::heap::binomial_heap" , n , P , U , r , [](){
       return( boost::heap::binomial_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
  run( "boost::heap::fibonacci_heap" , n , P , U , r , [](){
       return( boost::heap::fibonacci_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
  run( "boost::heap::pairing_heap" , n , P , U , r , [](){
       return( boost::heap::pairing_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
  run( "boost::heap::skew_heap" , n , P , U , r , [](){
       return( boost::heap::skew_heap< Pair , Greater >() ); } ,
       stdPush , stdPop );
#endif
#ifdef ADS_HAS_TLX
  run( "tlx::DAryHeap d=4" , n , P , U , r , [](){
       return( tlx::DAryHeap< Pair , 4 , std::less< Pair > >() ); } ,
       []( auto & q , int p , int x ) { q.push( Pair( p , x ) ); } ,
       stdPop );
  run( "tlx::RadixHeap" , n , P , U , r , [](){
       return( tlx::RadixHeapPair< unsigned , int >() ); } ,
       []( auto & q , int p , int x ) {
        q.emplace_keyfirst( unsigned( p ) , x ); } ,
       []( auto & q ) {
        const int p = int( q.top().first );
        q.pop();
        return( p ); } );
#endif
#ifdef ADS_HAS_IWIWI
  run( "radix_heap::pair_radix_heap" , n , P , U , r , [](){
       return( radix_heap::pair_radix_heap< unsigned , int >() ); } ,
       []( auto & q , int p , int x ) { q.push( unsigned( p ) , x ); } ,
       []( auto & q ) {
        const int p = int( q.top_key() );
        q.pop();
        return( p ); } );
#endif
  run( "ADS::DaryHeap d=4" , n , P , U , r , [](){
       return( ADS::DaryHeap< Pair , std::less< Pair > , 4 >() ); } ,
       []( auto & q , int p , int x ) { q.push( Pair( p , x ) ); } ,
       []( auto & q ) {
        const int p = q.top().first;
        q.pop();
        return( p ); } );
  run( "ADS::SequenceHeap" , n , P , U , r , [](){
       return( ADS::SequenceHeap< Pair >() ); } ,
       []( auto & q , int p , int x ) { q.push( Pair( p , x ) ); } ,
       []( auto & q ) { return( q.pop().first ); } );
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
  run( "ADS::PairingHeap" , n , P , U , r , [](){
       return( ADS::PairingHeap< int , int >() ); } , adsPush , adsPop );
  run( "ADS::PairingHeap aux" , n , P , U , r , [](){
       return( ADS::PairingHeap< int , int , true >() ); } , adsPush ,
       adsPop );
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
  run( "ADS::RadixHeap" , n , P , U , r , [](){
       return( ADS::RadixHeap< int , unsigned >() ); } ,
       []( auto & q , int p , int x ) { q.push( x , unsigned( p ) ); } ,
       []( auto & q ) {
        const int p = int( q.topKey() );
        q.pop();
        return( p ); } );

  run( "ADS::DialQueue" , n , P , U , r , [](){
       return( ADS::DialQueue< int , unsigned >( 1023 ) ); } ,
       []( auto & q , int p , int x ) { q.push( x , unsigned( p ) ); } ,
       []( auto & q ) {
        const int p = int( q.topKey() );
        q.pop();
        return( p ); } );

  // both ends

  for( const char * who : { "std::multiset" , "std::multiset A/A" } )
   ends( who , n , P , r , [](){ return( std::multiset< int >() ); } ,
         []( auto & q , int x ) { q.insert( x ); } ,
         []( auto & q ) {
          const int x = *q.begin();
          q.erase( q.begin() );
          return( x ); } ,
         []( auto & q ) {
          const auto it = std::prev( q.end() );
          const int x = *it;
          q.erase( it );
          return( x ); } );
  auto dePush = []( auto & q , int x ) { q.push( x ); };
  auto deLow = []( auto & q ) { return( q.popMin() ); };
  auto deHigh = []( auto & q ) { return( q.popMax() ); };
  ends( "ADS::MinMaxHeap" , n , P , r , [](){
        return( ADS::MinMaxHeap< int >() ); } , dePush , deLow , deHigh );
  ends( "ADS::IntervalHeap" , n , P , r , [](){
        return( ADS::IntervalHeap< int >() ); } , dePush , deLow , deHigh );

  // Dijkstra's algorithm

  const Graph G = graph( n , o.seed );
  reference = 0;
  using DPair = std::pair< int , unsigned >;
  for( const char * who : { "std::priority_queue" ,
                            "std::priority_queue A/A" } )
   dijkstra( who , G , r , stdLazy< std::priority_queue< DPair ,
             std::vector< DPair > , std::greater< DPair > > > );
#ifdef ADS_HAS_BOOST
  using DGreater = boost::heap::compare< std::greater< DPair > >;
  using M = boost::heap::mutable_< true >;
  dijkstra( "boost::heap::d_ary_heap d=4" , G , r , boostHandles<
            boost::heap::d_ary_heap< DPair , boost::heap::arity< 4 > ,
                                     DGreater , M > > );
  dijkstra( "boost::heap::binomial_heap" , G , r , boostHandles<
            boost::heap::binomial_heap< DPair , DGreater > > );
  dijkstra( "boost::heap::fibonacci_heap" , G , r , boostHandles<
            boost::heap::fibonacci_heap< DPair , DGreater > > );
  dijkstra( "boost::heap::pairing_heap" , G , r , boostHandles<
            boost::heap::pairing_heap< DPair , DGreater > > );
  dijkstra( "boost::heap::skew_heap" , G , r , boostHandles<
            boost::heap::skew_heap< DPair , DGreater , M > > );
#endif
#ifdef ADS_HAS_TLX
  dijkstra( "tlx::DAryHeap d=4" , G , r , stdLazy< tlx::DAryHeap< DPair ,
            4 , std::less< DPair > > > );
  dijkstra( "tlx::DAryAddressableIntHeap d=4" , G , r ,
            []( const Graph & G ) {
   const std::size_t n = G.first.size() - 1;
   std::vector< int > dist( n , INT_MAX );
   struct Less {
    const int * d;
    bool operator()( unsigned a , unsigned b ) const {
     return( d[ a ] < d[ b ] ); }
    };
   tlx::DAryAddressableIntHeap< unsigned , 4 , Less > q( Less{
                                                   dist.data() } );
   dist[ 0 ] = 0;
   q.push( 0u );
   while( ! q.empty() ) {
    const unsigned v = q.top();
    q.pop();
    for( unsigned a = G.first[ v ] ; a < G.first[ v + 1 ] ; ++a ) {
     const unsigned u = G.head[ a ];
     const int e = dist[ v ] + G.w[ a ];
     if( e < dist[ u ] ) {
      dist[ u ] = e;
      q.update( u );
      }
     }
    }
   return( checksum( dist ) ); } );
  dijkstra( "tlx::RadixHeap" , G , r , []( const Graph & G ) {
   tlx::RadixHeapPair< unsigned , unsigned > q;
   return( lazy( G , q , []( auto & q , int d , unsigned v ) {
                  q.emplace_keyfirst( unsigned( d ) , v ); } ,
                 []( auto & q , int & d , unsigned & v ) {
                  d = int( q.top().first );
                  v = q.top().second;
                  q.pop(); } ,
                 []( auto & q ) { return( q.empty() ); } ) ); } );
#endif
#ifdef ADS_HAS_IWIWI
  dijkstra( "radix_heap::pair_radix_heap" , G , r , []( const Graph & G ) {
   radix_heap::pair_radix_heap< unsigned , unsigned > q;
   return( lazy( G , q , []( auto & q , int d , unsigned v ) {
                  q.push( unsigned( d ) , v ); } ,
                 []( auto & q , int & d , unsigned & v ) {
                  d = int( q.top_key() );
                  v = q.top_value();
                  q.pop(); } ,
                 []( auto & q ) { return( q.empty() ); } ) ); } );
#endif
  dijkstra( "ADS::DaryHeap d=4" , G , r , stdLazy< ADS::DaryHeap< DPair ,
            std::less< DPair > , 4 > > );
  dijkstra( "ADS::SequenceHeap" , G , r , stdLazy< ADS::SequenceHeap<
            DPair > > );
  dijkstra( "ADS::HeapPriorityQueue" , G , r , []( const Graph & G ) {
   using Q = ADS::HeapPriorityQueue< unsigned , int >;
   return( adsHandles< Q >( G , [ & ](){
            return( Q( int( G.first.size() ) ) ); } ) ); } );
  dijkstra( "ADS::FibonacciHeapPriorityQueue" , G , r ,
            []( const Graph & G ) {
   using Q = ADS::FibonacciHeapPriorityQueue< unsigned , int >;
   return( adsHandles< Q >( G , [](){ return( Q() ); } ) ); } );
  dijkstra( "ADS::PairingHeap" , G , r , []( const Graph & G ) {
   using Q = ADS::PairingHeap< unsigned , int >;
   return( adsHandles< Q >( G , [](){ return( Q() ); } ) ); } );
  dijkstra( "ADS::PairingHeap aux" , G , r , []( const Graph & G ) {
   using Q = ADS::PairingHeap< unsigned , int , true >;
   return( adsHandles< Q >( G , [](){ return( Q() ); } ) ); } );
  dijkstra( "ADS::DialQueue" , G , r , []( const Graph & G ) {
   ADS::DialQueue< unsigned , unsigned > q( 1000 );
   return( lazy( G , q , []( auto & q , int d , unsigned v ) {
                  q.push( v , unsigned( d ) ); } ,
                 []( auto & q , int & d , unsigned & v ) {
                  d = int( q.topKey() );
                  v = q.pop(); } ,
                 []( auto & q ) { return( q.empty() ); } ) ); } );
  dijkstra( "ADS::RadixHeap" , G , r , []( const Graph & G ) {
   ADS::RadixHeap< unsigned , unsigned > q;
   return( lazy( G , q , []( auto & q , int d , unsigned v ) {
                  q.push( v , unsigned( d ) ); } ,
                 []( auto & q , int & d , unsigned & v ) {
                  d = int( q.topKey() );
                  v = q.pop(); } ,
                 []( auto & q ) { return( q.empty() ); } ) ); } );

  // the best-first searches

  const unsigned c = unsigned( 2 * std::sqrt( double( n ) ) );
  for( unsigned P : { 1u , 64u } ) {
   const std::string grid = "grid " + std::to_string( P ) +
                            ( P > 1 ? " queues" : " queue" );
   const std::string puzzle = "15-puzzle " + std::to_string( P ) +
                              ( P > 1 ? " queues" : " queue" );
   searches( grid.c_str() , n , P , r , [ & ](){
             return( gridSearch( c , P , o.seed ) ); } );
   searches( puzzle.c_str() , n , P , r , [ & ](){
             return( puzzleSearch( n , P , o.seed ) ); } );
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File priority.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
