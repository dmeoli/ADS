/*--------------------------------------------------------------------------*/
/*----------------------------- File ch11.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 11, Strutture di dati e progettazione di algoritmi: the
 * Fibonacci heap against a reference, the theorem of Bellman on the graphs
 * of Figura 11.1, the prototype with each of its six structures against
 * Floyd-Warshall and against the acyclic graphs, the orders of extraction
 * of the examples of Bellman-Ford-Moore, of the stack and of Pape-D'Esopo,
 * and the exercises 11.1 to 11.10, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/graph/ListGraph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using WEdges = std::vector< std::tuple< int , int , int > >;
using WG = ListGraph< int >;

static_assert( PriorityQueue< FibonacciHeapPriorityQueue< int > > );

constexpr int INFI = infinity< int >();

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the oriented graph of n nodes and of the edges ( u , v , w )
template< class W = int >
ListGraph< W > make( int n ,
                     const std::vector< std::tuple< int , int , W > > & E )
{
 ListGraph< W > g( n , true );
 for( auto [ u , v , w ] : E )
  g.insertEdge( u , v , w );
 return( g );
 }

/// Figura 11.1 (b) and 11.3 (a), r = 0; Figura 11.1 (a) has the cost 2 on
/// ( 3 , 2 ), and so the negative cycle 1 , 3 , 2 , 1
const WEdges G2 = { { 0 , 1 , 3 } , { 0 , 2 , 1 } , { 1 , 3 , -2 } ,
                    { 2 , 1 , -1 } , { 3 , 2 , 4 } };
const WEdges G1 = { { 0 , 1 , 3 } , { 0 , 2 , 1 } , { 1 , 3 , -2 } ,
                    { 2 , 1 , -1 } , { 3 , 2 , 2 } };

/// a random graph, each edge with probability p and a cost in
/// [ lo , hi ]; acyclic, by a random order, if dag
WG random_graph( int n , double p , int lo , int hi , bool dag ,
                 unsigned seed )
{
 std::mt19937 g( seed );
 Ints pi( n );
 for( int i = 0 ; i < n ; ++i ) pi[ i ] = i;
 std::shuffle( pi.begin() , pi.end() , g );
 std::bernoulli_distribution b( p );
 std::uniform_int_distribution< int > w( lo , hi );
 WG G( n , true );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( ( i != j ) && ( ! dag || i < j ) && b( g ) )
    G.insertEdge( pi[ i ] , pi[ j ] , w( g ) );
 return( G );
 }

/// Floyd-Warshall: D[ u ][ v ], and true in neg[ u ] if u is on a cycle of
/// negative cost
std::vector< std::vector< long > > floyd( const WG & g )
{
 const int n = g.bound();
 const long I = std::numeric_limits< long >::max() / 4;
 std::vector< std::vector< long > > D( n , std::vector< long >( n , I ) );
 for( int u = 0 ; u < n ; ++u ) {
  D[ u ][ u ] = 0;
  g.forEachEdge( u , [ & ]( int v , int w ) {
   D[ u ][ v ] = std::min( D[ u ][ v ] , long( w ) ); } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( D[ i ][ k ] < I && D[ k ][ j ] < I )
     D[ i ][ j ] = std::min( D[ i ][ j ] , D[ i ][ k ] + D[ k ][ j ] );
 for( auto & r : D )
  for( auto & x : r )
   if( x >= I / 2 ) x = INFI;
 return( D );
 }

/// the conditions of Bellman (Teorema 11.1) on the tree of the result
template< class G , class D >
bool bellman( const G & g , int r , const ShortestPathTree< D > & R )
{
 const D INF = infinity< D >();
 bool ok = ( R.d[ r ] == 0 );
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   if( R.d[ u ] == INF )
    return;
   ok = ok && ( R.d[ u ] + w >= R.d[ v ] );
   if( R.T[ v ] == u )
    ok = ok && ( R.d[ v ] == R.d[ u ] + w );
   } );
 return( ok );
 }

const Frontier all[] = { Frontier::LIST , Frontier::HEAP ,
                         Frontier::FIBONACCI , Frontier::QUEUE ,
                         Frontier::STACK , Frontier::DEQUE };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the Fibonacci heap against a vector of the items present; one item, a
// decrease to the same priority and of the min itself, all the priorities
// equal, the extremes of int, and the contracts on an empty heap and on a
// decrease that increases
ADS_TEST( "sec11.5.fibonacci" )
{
 std::mt19937 g( 1 );
 FibonacciHeapPriorityQueue< int > q;
 std::map< int , std::pair< FibonacciHeapPriorityQueue< int >::Handle ,
                            int > > in;
 int next = 0;
 for( int o = 0 ; o < 50000 ; ++o ) {
  const int what = g() % 10;
  if( ( what < 4 ) && ( in.size() < 2000 ) ) {
   const int p = g() % 100000;
   in.emplace( next , std::pair( q.insert( next , p ) , p ) );
   ++next;
   }
  else if( ( what < 7 ) && ! in.empty() ) {
   auto it = in.begin();
   std::advance( it , g() % in.size() );
   const int p = it->second.second - int( g() % 500 );
   q.decrease( it->second.first , p );
   it->second.second = p;
   }
  else if( ! in.empty() ) {
   int best = in.begin()->second.second;
   for( const auto & [ x , hp ] : in )
    best = std::min( best , hp.second );
   CHECK( q.minPriority() == best );
   const int x = q.deleteMin();
   CHECK( in.at( x ).second == best );
   in.erase( x );
   }
  CHECK( q.size() == int( in.size() ) );
  }
 CHECK_THROWS( ( FibonacciHeapPriorityQueue< int >().deleteMin() ) ,
               std::logic_error );
 CHECK_THROWS( ( FibonacciHeapPriorityQueue< int >().min() ) ,
               std::logic_error );
 CHECK_THROWS( ( FibonacciHeapPriorityQueue< int >().minPriority() ) ,
               std::logic_error );

 // one item, in and out, and the heap empty again and reusable
 FibonacciHeapPriorityQueue< int > one;
 CHECK( one.isEmpty() && one.size() == 0 );
 auto h1 = one.insert( 7 , 3 );
 CHECK( one.min() == 7 && one.minPriority() == 3 && one.size() == 1 );
 one.decrease( h1 , 3 );                         // to the same priority
 one.decrease( h1 , -4 );                         // the min itself
 CHECK( one.minPriority() == -4 && one.deleteMin() == 7 && one.isEmpty() );
 one.insert( 8 , 1 );
 CHECK( one.deleteMin() == 8 && one.isEmpty() );

 // a decrease to a larger priority and a null handle break the contract
 auto h2 = one.insert( 1 , 5 );
 CHECK_THROWS( one.decrease( h2 , 6 ) , std::logic_error );
 CHECK_THROWS( one.decrease( nullptr , 0 ) , std::logic_error );
 one.deleteMin();

 // all the priorities equal, and the extreme values of int: the ties all
 // come out, the extremes first and last
 constexpr int MAXI = std::numeric_limits< int >::max();
 constexpr int MINI = std::numeric_limits< int >::min();
 FibonacciHeapPriorityQueue< int > eq;
 std::vector< FibonacciHeapPriorityQueue< int >::Handle > hs;
 for( int i = 0 ; i < 100 ; ++i )
  hs.push_back( eq.insert( i , 5 ) );
 eq.insert( 100 , MAXI );
 eq.insert( 101 , MINI );
 CHECK( eq.deleteMin() == 101 );
 std::set< int > out;
 for( int i = 0 ; i < 50 ; ++i ) {               // a consolidated forest
  CHECK( eq.minPriority() == 5 );
  out.insert( eq.deleteMin() );
  }
 // each item left down to the minimum of int, which cuts it (and cascades
 // the cuts) out of the trees of the consolidation, then out of the heap
 for( int i = 0 ; i < 100 ; ++i )
  if( ! out.count( i ) ) {
   eq.decrease( hs[ i ] , MINI );
   CHECK( eq.minPriority() == MINI && eq.deleteMin() == i );
   out.insert( i );
   }
 CHECK( out.size() == 100 && eq.size() == 1 );
 CHECK( eq.minPriority() == MAXI && eq.deleteMin() == 100 && eq.isEmpty() );
 }

// Figura 11.1: G2 has the optimal tree T2 of distances 0 , 0 , 1 , -2,
// which satisfies the conditions of Bellman, while the feasible T1 does
// not; G1 has a negative cycle; isShortestPathTree() rejects a tree broken
// in each of its conditions, and an unreachable node with a parent
ADS_TEST( "sec11.1.bellman" )
{
 const auto g = make( 4 , G2 );
 const auto R = bellmanFordMoore( g , 0 );
 CHECK( ! R.negativeCycle && R.d == Ints( { 0 , 0 , 1 , -2 } ) );
 CHECK( R.T == Ints( { NIL , 2 , 0 , 1 } ) && bellman( g , 0 , R ) );
 ShortestPathTree< int > T1;
 T1.T = { NIL , 0 , 0 , 1 };
 T1.d = { 0 , 3 , 1 , 1 };
 CHECK( ! bellman( g , 0 , T1 ) );            // ( 2 , 1 ) violates it
 CHECK( bellmanFordMoore( make( 4 , G1 ) , 0 ).negativeCycle );
 // isShortestPathTree(): R and T1, then the trees broken one way each
 CHECK( isShortestPathTree( g , 0 , R ) && ! isShortestPathTree( g , 0 ,
                                                                 T1 ) );
 auto B = R;
 B.d.pop_back();                                       // the wrong size
 CHECK( ! isShortestPathTree( g , 0 , B ) );
 B = R; B.T[ 0 ] = 2;                                   // r with a parent
 CHECK( ! isShortestPathTree( g , 0 , B ) );
 B = R; B.d[ 0 ] = 1;                                   // d[ r ] != 0
 CHECK( ! isShortestPathTree( g , 0 , B ) );
 B = R; B.T[ 3 ] = 0;                                   // no edge ( 0 , 3 )
 CHECK( ! isShortestPathTree( g , 0 , B ) );
 B = R; B.d[ 3 ] = INFI; B.T[ 3 ] = NIL;        // reachable, left at infinity
 CHECK( ! isShortestPathTree( g , 0 , B ) );
 // an unreachable node has infinity and no parent, and only that
 const auto u = make( 2 , WEdges() );
 CHECK( isShortestPathTree( u , 0 , dijkstra( u , 0 ) ) );
 B = dijkstra( u , 0 ); B.T[ 1 ] = 0;
 CHECK( ! isShortestPathTree( u , 0 , B ) );
 }

// the prototype with each structure, against Floyd-Warshall with costs not
// negative, and against dagShortestPaths() on acyclic graphs with costs of
// any sign; then on one node, an unreachable node, cycles of cost 0, a
// deleted node, an unoriented graph and costs in double, r not a node, and
// the audits on a negative cycle and on a cycle given to dagShortestPaths()
ADS_TEST( "sec11.2.prototype" )
{
 for( unsigned s = 0 ; s < 30 ; ++s ) {
  const auto g = random_graph( 25 , 0.15 , 0 , 20 , false , s );
  const auto D = floyd( g );
  for( Frontier f : all ) {
   const auto R = shortestPaths( g , 0 , f );
   for( int v = 0 ; v < 25 ; ++v )
    CHECK( R.d[ v ] == D[ 0 ][ v ] );
   CHECK( bellman( g , 0 , R ) );
   }
  const auto h = random_graph( 20 , 0.3 , -10 , 10 , true , s );
  for( int r = 0 ; r < 20 ; r += 7 ) {
   const auto A = dagShortestPaths( h , r );
   for( Frontier f : all )
    CHECK( shortestPaths( h , r , f ).d == A.d );
   }
  }

 // the degenerate graphs, with each structure
 const auto one = make( 1 , WEdges() ) , two = make( 2 , WEdges() );
 const auto zero = make( 3 , WEdges( { { 0 , 1 , 0 } , { 1 , 0 , 0 } ,
                                       { 1 , 2 , 0 } , { 2 , 1 , 0 } } ) );
 auto holed = make( 4 , WEdges( { { 0 , 1 , 5 } , { 1 , 3 , 1 } ,
                                  { 0 , 2 , 1 } , { 2 , 3 , 1 } } ) );
 holed.deleteNode( 2 );                       // a name left unused
 WG und( 4 , false );                         // unoriented, costs >= 0
 und.insertEdge( 0 , 1 , 4 ); und.insertEdge( 1 , 2 , 1 );
 und.insertEdge( 0 , 2 , 7 );
 ListGraph< double > dbl( 3 , true );         // infinity() is a true inf
 dbl.insertEdge( 0 , 1 , 0.5 );
 for( Frontier f : all ) {
  auto R = shortestPaths( one , 0 , f );       // one node, no edge
  CHECK( R.d == Ints( { 0 } ) && R.T == Ints( { NIL } ) &&
         R.order == Ints( { 0 } ) && ! R.negativeCycle );
  R = shortestPaths( two , 1 , f );            // r the last, 0 unreachable
  CHECK( R.d == Ints( { INFI , 0 } ) && R.T == Ints( { NIL , NIL } ) &&
         R.order == Ints( { 1 } ) );
  R = shortestPaths( zero , 2 , f );           // cycles of cost 0 end
  CHECK( R.d == Ints( { 0 , 0 , 0 } ) && R.T[ 2 ] == NIL &&
         isShortestPathTree( zero , 2 , R ) );
  R = shortestPaths( holed , 0 , f );          // the hole at infinity
  CHECK( R.d == Ints( { 0 , 5 , INFI , 6 } ) && R.T[ 2 ] == NIL &&
         R.T[ 3 ] == 1 );
  R = shortestPaths( und , 0 , f );            // both directions
  CHECK( R.d == Ints( { 0 , 4 , 5 , INFI } ) && R.T[ 2 ] == 1 );
  CHECK( shortestPaths( und , 2 , f ).d == Ints( { 5 , 1 , 0 , INFI } ) );
  const auto Q = shortestPaths( dbl , 0 , f );
  CHECK( Q.d[ 1 ] == 0.5 && std::isinf( Q.d[ 2 ] ) && Q.T[ 2 ] == NIL );
  // r not a node: out of range on both sides, or deleted
  CHECK_THROWS( shortestPaths( one , -1 , f ) , std::logic_error );
  CHECK_THROWS( shortestPaths( one , 1 , f ) , std::logic_error );
  CHECK_THROWS( shortestPaths( holed , 2 , f ) , std::logic_error );
  }
 CHECK( dagShortestPaths( one , 0 ).d == Ints( { 0 } ) );
 CHECK( dagShortestPaths( holed , 0 ).d == Ints( { 0 , 5 , INFI , 6 } ) );
 CHECK_THROWS( dagShortestPaths( holed , 2 ) , std::logic_error );
 CHECK_THROWS( dagShortestPaths( two , 2 ) , std::logic_error );

 // the audits: a negative cycle reachable from r for a frontier other than
 // QUEUE, which reports it instead, and a cycle for dagShortestPaths(),
 // even of cost 0
 const auto neg = make( 4 , G1 );
 for( Frontier f : all )
  if( f == Frontier::QUEUE )
   CHECK( shortestPaths( neg , 0 , f ).negativeCycle );
  else
   CHECK_THROWS( shortestPaths( neg , 0 , f ) , std::logic_error );
 CHECK_THROWS( dagShortestPaths( zero , 0 ) , std::logic_error );
 }

// Dijkstra, Johnson, Fredman-Tarjan: with costs not negative each node is
// extracted once, by increasing distance, also when all the costs are 0
// (all ties) and on a complete graph of costs 1 (each node a child of r)
ADS_TEST( "sec11.3.dijkstra" )
{
 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const auto g = random_graph( 60 , 0.08 , 0 , 50 , false , s );
  for( Frontier f : { Frontier::LIST , Frontier::HEAP ,
                      Frontier::FIBONACCI } ) {
   const auto R = shortestPaths( g , 0 , f );
   const int reach = int( std::count_if( R.d.begin() , R.d.end() ,
                          []( int x ) { return( x < INFI ); } ) );
   CHECK( int( R.order.size() ) == reach );
   for( std::size_t i = 1 ; i < R.order.size() ; ++i )
    CHECK( R.d[ R.order[ i - 1 ] ] <= R.d[ R.order[ i ] ] );
   }
  }
 // all the costs 0, all ties: still each node once, all at distance 0;
 // a complete graph of costs 1, all at distance 1 from r
 const auto z = random_graph( 30 , 0.2 , 0 , 0 , false , 7 );
 const auto c = random_graph( 30 , 1.0 , 1 , 1 , false , 7 );
 for( Frontier f : { Frontier::LIST , Frontier::HEAP ,
                     Frontier::FIBONACCI } ) {
  const auto R = shortestPaths( z , 0 , f );
  std::set< int > seen( R.order.begin() , R.order.end() );
  CHECK( seen.size() == R.order.size() );
  for( int v : R.order )
   CHECK( R.d[ v ] == 0 );
  const auto C = shortestPaths( c , 5 , f );
  CHECK( C.order.size() == 30 && C.order[ 0 ] == 5 );
  for( int v = 0 ; v < 30 ; ++v )
   CHECK( C.d[ v ] == ( v == 5 ? 0 : 1 ) && C.T[ v ] == ( v == 5 ? NIL : 5 ) );
  }
 }

// Figura 11.3: Bellman-Ford-Moore extracts 0 (pass 0), 1 , 2 (pass 1),
// 3 , 1 (pass 2), 3 (pass 3)
ADS_TEST( "sec11.6.bellman-ford-moore" )
{
 const auto R = bellmanFordMoore( make( 4 , G2 ) , 0 );
 CHECK( R.order == Ints( { 0 , 1 , 2 , 3 , 1 , 3 } ) );
 // a node is extracted at most n times without negative cycles
 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const auto h = random_graph( 30 , 0.2 , -10 , 30 , true , s );
  const auto Q = bellmanFordMoore( h , 0 );
  std::map< int , int > c;
  for( int u : Q.order )
   CHECK( ++c[ u ] <= 30 );
  }
 }

// Figura 11.4: on G_n, ( u , v ) for u < v of cost
// 2^( n - u - 2 ) - 2^( n - v - 1 ) + v - u, a stack extracts node u
// 2^( u - 1 ) times, 2^( n - 1 ) extractions in all, while a queue
// extracts each node once
ADS_TEST( "sec11.7.stack" )
{
 for( int n : { 2 , 3 , 5 , 8 , 12 } ) {       // n = 2: a single edge
  WEdges E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    E.emplace_back( u , v , ( 1 << ( n - u - 2 ) ) - ( 1 << ( n - v - 1 ) ) +
                            v - u );
  const auto g = make( n , E );
  const auto S = shortestPaths( g , 0 , Frontier::STACK );
  CHECK( int( S.order.size() ) == ( 1 << ( n - 1 ) ) );
  CHECK( std::count( S.order.begin() , S.order.end() , n - 1 ) ==
         ( 1 << ( n - 2 ) ) );
  CHECK( int( bellmanFordMoore( g , 0 ).order.size() ) == n );
  Ints d( n );
  for( int v = 0 ; v < n ; ++v ) d[ v ] = v;
  CHECK( S.d == d );
  if( n == 5 )                                  // the column of the figure
   CHECK( S.order == Ints( { 0 , 4 , 3 , 4 , 2 , 4 , 3 , 4 , 1 , 4 , 3 ,
                             4 , 2 , 4 , 3 , 4 } ) );
  }
 }

// Pape-D'Esopo on the graph of Figura 11.3 extracts 0 , 1 , 2 , 1 , 3;
// a node improved after its extraction goes back at the head of S
ADS_TEST( "sec11.8.pape" )
{
 CHECK( papeDEsopo( make( 4 , G2 ) , 0 ).order ==
        Ints( { 0 , 1 , 2 , 1 , 3 } ) );
 // a node improved while in S stays where it is, one improved after its
 // extraction goes back at the head, before a node still waiting: 0 queues
 // 3 , 2 , 1 , 4 at the tail, 3 improves 1 in place, 1 improves 2 already
 // extracted, which comes out again before 4
 const auto g = make( 5 , WEdges( { { 0 , 3 , 1 } , { 0 , 2 , 5 } ,
                                    { 0 , 1 , 9 } , { 0 , 4 , 9 } ,
                                    { 3 , 1 , 1 } , { 1 , 2 , 1 } } ) );
 const auto R = papeDEsopo( g , 0 );
 CHECK( R.order == Ints( { 0 , 3 , 2 , 1 , 2 , 4 } ) );
 CHECK( R.d == Ints( { 0 , 2 , 3 , 1 , 9 } ) &&
        isShortestPathTree( g , 0 , R ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// Dijkstra with negative costs: the version that never extracts a node
// again (a set of closed nodes) gives a wrong distance, the prototype puts
// the node back in S and stays correct
ADS_TEST( "ex11.1" )
{
 const auto g = make( 3 , WEdges( { { 0 , 1 , 1 } , { 0 , 2 , 2 } ,
                                    { 2 , 1 , -2 } } ) );
 Ints d( 3 , INFI );
 std::vector< bool > closed( 3 , false );
 d[ 0 ] = 0;
 for( int k = 0 ; k < 3 ; ++k ) {
  int u = -1;
  for( int v = 0 ; v < 3 ; ++v )
   if( ! closed[ v ] && d[ v ] < INFI && ( u < 0 || d[ v ] < d[ u ] ) )
    u = v;
  closed[ u ] = true;
  g.forEachEdge( u , [ & ]( int v , int w ) {
   if( ! closed[ v ] && d[ u ] + w < d[ v ] ) d[ v ] = d[ u ] + w; } );
  }
 CHECK( d[ 1 ] == 1 );                                    // wrong
 const auto R = dijkstra( g , 0 );
 CHECK( R.d[ 1 ] == 0 && R.order == Ints( { 0 , 1 , 2 , 1 } ) );
 }

// a negative cycle reachable from r, found by Bellman-Ford-Moore in
// O( n m ), against Floyd-Warshall and reachesNegativeCycle(); a cycle of
// two nodes, one not reachable from r, one of cost exactly 0, one node
ADS_TEST( "ex11.2" )
{
 CHECK( bellmanFordMoore( make( 4 , G1 ) , 0 ).negativeCycle );
 CHECK( ! bellmanFordMoore( make( 4 , G2 ) , 0 ).negativeCycle );
 CHECK( reachesNegativeCycle( make( 4 , G1 ) , 0 ) );
 CHECK( ! reachesNegativeCycle( make( 4 , G2 ) , 0 ) );
 const auto two = make( 3 , WEdges( { { 0 , 1 , 1 } , { 1 , 2 , -3 } ,
                                      { 2 , 1 , 2 } } ) );
 CHECK( bellmanFordMoore( two , 0 ).negativeCycle );
 CHECK( reachesNegativeCycle( two , 0 ) );
 CHECK( bellmanFordMoore( two , 1 ).negativeCycle );    // r on the cycle
 const auto away = make( 3 , WEdges( { { 1 , 2 , -3 } , { 2 , 1 , 2 } ,
                                       { 1 , 0 , 1 } } ) );
 const auto A = bellmanFordMoore( away , 0 );         // not from r = 0
 CHECK( ! A.negativeCycle && A.d == Ints( { 0 , INFI , INFI } ) );
 CHECK( ! reachesNegativeCycle( away , 0 ) && reachesNegativeCycle( away ,
                                                                    2 ) );
 const auto zero = make( 3 , WEdges( { { 0 , 1 , 1 } , { 1 , 2 , -2 } ,
                                       { 2 , 1 , 2 } } ) );
 const auto Z = bellmanFordMoore( zero , 0 );          // cost exactly 0
 CHECK( ! Z.negativeCycle && Z.d == Ints( { 0 , 1 , -1 } ) );
 CHECK( ! reachesNegativeCycle( zero , 0 ) );
 CHECK( ! reachesNegativeCycle( make( 1 , WEdges() ) , 0 ) );
 for( unsigned s = 0 ; s < 40 ; ++s ) {
  const auto g = random_graph( 12 , 0.2 , -5 , 15 , false , s );
  const auto D = floyd( g );
  bool neg = false;
  for( int v = 0 ; v < 12 ; ++v )
   neg = neg || ( D[ 0 ][ v ] < INFI && D[ v ][ v ] < 0 );
  CHECK( bellmanFordMoore( g , 0 ).negativeCycle == neg );
  }
 }

// the most reliable path, the largest product of the reliabilities: the
// shortest path with the costs -log r( u , v ) >= 0, against the simple
// paths enumerated, also with all the reliabilities 1 (all the costs 0)
// and with the target unreachable (reliability 0)
ADS_TEST( "ex11.3" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 7;
  ListGraph< double > G( n , true ) , L( n , true );
  std::uniform_real_distribution< double > r( 0.05 , 1.0 );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v && t != 1 && g() % 3 == 0 ) {      // t = 1: no edge
     const double x = ( t == 0 ) ? 1.0 : r( g );    // t = 0: all sure
     G.insertEdge( u , v , x );
     L.insertEdge( u , v , -std::log( x ) );
     }
  double best = 0;
  std::vector< bool > on( n , false );
  std::function< void( int , double ) > dfs = [ & ]( int u , double p ) {
   if( u == n - 1 ) { best = std::max( best , p ); return; }
   on[ u ] = true;
   G.forEachEdge( u , [ & ]( int v , double x ) {
    if( ! on[ v ] ) dfs( v , p * x ); } );
   on[ u ] = false;
   };
  dfs( 0 , 1 );
  const auto R = johnson( L , 0 );
  const double got = R.d[ n - 1 ] == infinity< double >() ? 0 :
                     std::exp( -R.d[ n - 1 ] );
  CHECK( std::abs( got - best ) <= 1e-9 );
  }
 }

/// intervals with distinct ends, sorted by left end
using Iv = std::vector< std::pair< int , int > >;

Iv random_intervals( int n , std::mt19937 & g )
{
 Ints ends( 2 * n );
 for( int i = 0 ; i < 2 * n ; ++i ) ends[ i ] = i;
 std::shuffle( ends.begin() , ends.end() , g );
 Iv I( n );
 for( int i = 0 ; i < n ; ++i )
  I[ i ] = { std::min( ends[ 2 * i ] , ends[ 2 * i + 1 ] ) ,
             std::max( ends[ 2 * i ] , ends[ 2 * i + 1 ] ) };
 std::sort( I.begin() , I.end() );
 return( I );
 }

bool meet( std::pair< int , int > x , std::pair< int , int > y )
{
 return( x.first <= y.second && y.first <= x.second );
 }

/// the degenerate families: none, one, all disjoint, all nested, a chain
/// of crossings, all through one point; then k random ones of at most m
std::vector< Iv > interval_cases( int k , int m , std::mt19937 & g )
{
 std::vector< Iv > C = { {} , { { 0 , 1 } } ,
                         { { 0 , 1 } , { 2 , 3 } , { 4 , 5 } , { 6 , 7 } } ,
                         { { 0 , 9 } , { 1 , 8 } , { 2 , 7 } , { 3 , 6 } ,
                           { 4 , 5 } } ,
                         { { 0 , 2 } , { 1 , 4 } , { 3 , 6 } , { 5 , 8 } ,
                           { 7 , 9 } } ,
                         { { 0 , 5 } , { 1 , 6 } , { 2 , 7 } , { 3 , 8 } ,
                           { 4 , 9 } } };
 for( int t = 0 ; t < k ; ++t )
  C.push_back( random_intervals( 1 + int( g() % m ) , g ) );
 return( C );
 }

/// the intervals with the sentinels s = [ -2 , -1 ] first and t after all
Iv with_sentinels( const Iv & I )
{
 Iv J = { { -2 , -1 } };
 J.insert( J.end() , I.begin() , I.end() );
 J.push_back( { 1000 , 1001 } );
 return( J );
 }

// the largest independent set of intervals: the shortest path from s to t
// on the acyclic graph of the edges ( u , v ) with b_u < a_v, of cost -1
// but from s, against the enumeration, on the degenerate families of
// interval_cases() and on random ones
ADS_TEST( "ex11.4" )
{
 std::mt19937 g( 4 );
 for( const Iv & I : interval_cases( 50 , 10 , g ) ) {
  const int n = int( I.size() );
  const Iv J = with_sentinels( I );
  const int N = n + 2;
  WG G( N , true );
  for( int u = 0 ; u < N ; ++u )
   for( int v = 0 ; v < N ; ++v )
    if( J[ u ].second < J[ v ].first )
     G.insertEdge( u , v , u == 0 ? 0 : -1 );
  const int got = -dagShortestPaths( G , 0 ).d[ N - 1 ];
  int best = 0;
  for( int m = 0 ; m < ( 1 << n ) ; ++m ) {
   bool ok = true;
   for( int i = 0 ; i < n && ok ; ++i )
    for( int j = i + 1 ; j < n && ok ; ++j )
     if( ( m >> i & 1 ) && ( m >> j & 1 ) && meet( I[ i ] , I[ j ] ) )
      ok = false;
   if( ok ) best = std::max( best , std::popcount( unsigned( m ) ) );
   }
  CHECK( got == best );
  }
 }

/// P_u = { k : a_u < a_k < b_u < b_k }, the intervals that cross u to its
/// right; Q_u = { k : a_k > b_u, no h with b_u < a_h < b_h < a_k }, the
/// intervals after u with none between; C_u = { k : a_u < a_k < b_k < b_u }
bool inP( const Iv & J , int u , int k )
{
 return( J[ u ].first < J[ k ].first && J[ k ].first < J[ u ].second &&
         J[ u ].second < J[ k ].second );
 }

bool inQ( const Iv & J , int u , int k )
{
 if( ! ( J[ k ].first > J[ u ].second ) )
  return( false );
 for( std::size_t h = 0 ; h < J.size() ; ++h )
  if( J[ u ].second < J[ h ].first && J[ h ].second < J[ k ].first )
   return( false );
 return( true );
 }

bool inC( const Iv & J , int u , int k )
{
 return( J[ u ].first < J[ k ].first && J[ k ].second < J[ u ].second );
 }

// the smallest dominating set of intervals: the shortest path from s to t
// on the acyclic graph of the edges ( u , v ), v in P_u or Q_u, of cost 1
// but from s, against the enumeration, on the degenerate families of
// interval_cases() and on random ones
ADS_TEST( "ex11.5" )
{
 std::mt19937 g( 5 );
 for( const Iv & I : interval_cases( 100 , 9 , g ) ) {
  const int n = int( I.size() );
  const Iv J = with_sentinels( I );
  const int N = n + 2;
  WG G( N , true );
  for( int u = 0 ; u < N ; ++u )
   for( int v = 0 ; v < N ; ++v )
    if( u != v && ( inP( J , u , v ) || inQ( J , u , v ) ) )
     G.insertEdge( u , v , u == 0 ? 0 : 1 );
  const int got = dagShortestPaths( G , 0 ).d[ N - 1 ];
  int best = n;
  for( int m = 1 ; m < ( 1 << n ) ; ++m ) {
   bool ok = true;
   for( int i = 0 ; i < n && ok ; ++i ) {
    if( m >> i & 1 ) continue;
    bool dom = false;
    for( int j = 0 ; j < n ; ++j )
     dom = dom || ( ( m >> j & 1 ) && meet( I[ i ] , I[ j ] ) );
    ok = dom;
    }
   if( ok ) best = std::min( best , std::popcount( unsigned( m ) ) );
   }
  CHECK( got == best );
  }
 }

// the total dominating set of least weight: the graph of the solution, two
// nodes u_IN and u_OUT per interval, against the enumeration, on the
// families of interval_cases() (none: the empty set; one interval or all
// disjoint: no solution) and on random ones
ADS_TEST( "ex11.6" )
{
 std::mt19937 g( 6 );
 int feasible = 0;
 for( const Iv & I : interval_cases( 150 , 8 , g ) ) {
  const int n = int( I.size() );
  const Iv J = with_sentinels( I );
  Ints w( n + 2 , 0 );
  for( int u = 1 ; u <= n ; ++u ) w[ u ] = g() % 10;
  // s = 0 and t = 2 n + 2 stand for both their IN and OUT nodes
  const int N = 2 * ( n + 2 ) , s = 0 , tt = N - 1;
  auto IN = [ & ]( int u ) {
   return( u == 0 ? s : u == n + 1 ? tt : 2 * u ); };
  auto OUT = [ & ]( int u ) {
   return( u == 0 ? s : u == n + 1 ? tt : 2 * u + 1 ); };
  WG G( N , true );
  auto edge = [ & ]( int x , int y , int c ) {
   if( x == y ) return;
   const auto old = G.weight( x , y );
   if( ! old || c < *old ) G.insertEdge( x , y , c );
   };
  for( int u = 0 ; u <= n + 1 ; ++u )
   for( int v = 0 ; v <= n + 1 ; ++v ) {
    if( u == v ) continue;
    if( inP( J , u , v ) ) {
     edge( IN( u ) , IN( v ) , w[ u ] );                     // (1)
     edge( OUT( u ) , IN( v ) , w[ u ] );                    // (3)
     }
    if( inQ( J , u , v ) ) {
     edge( IN( u ) , OUT( v ) , w[ u ] );                    // (2)
     int k = -1;
     for( int h = 1 ; h <= n ; ++h )
      if( inC( J , u , h ) && ( k < 0 || w[ h ] < w[ k ] ) ) k = h;
     if( k >= 0 )
      edge( OUT( u ) , OUT( v ) , w[ u ] + w[ k ] );         // (4)
     }
    }
  const int got = dagShortestPaths( G , s ).d[ tt ];
  int best = INFI;                     // the empty set when there is none
  for( int m = 0 ; m < ( 1 << n ) ; ++m ) {
   bool ok = true;
   for( int i = 0 ; i < n && ok ; ++i ) {
    bool dom = false;
    for( int j = 0 ; j < n ; ++j )
     dom = dom || ( j != i && ( m >> j & 1 ) && meet( I[ i ] , I[ j ] ) );
    ok = dom;
    }
   if( ok ) {
    int c = 0;
    for( int i = 0 ; i < n ; ++i ) if( m >> i & 1 ) c += w[ i + 1 ];
    best = std::min( best , c );
    }
   }
  feasible += ( best < INFI );
  CHECK( got == best );
  }
 CHECK( feasible > 50 );
 }

// the number of shortest paths from s, positive costs: in Dijkstra a node
// improved from u takes the count of u, one reached again at the same
// distance adds it; against the counts on the tight edges of Floyd-Warshall,
// on random graphs, s alone, unreachable nodes (count 0) and a chain of 40
// diamonds, all ties, with 2^40 paths to its end
ADS_TEST( "ex11.7" )
{
 std::vector< WG > G;
 for( unsigned t = 0 ; t < 30 ; ++t )
  G.push_back( random_graph( 20 , 0.25 , 1 , 3 , false , t ) );
 G.push_back( make( 1 , WEdges() ) );                       // s alone
 G.push_back( make( 3 , WEdges( { { 1 , 2 , 1 } } ) ) );    // unreachable
 WEdges E;                                  // 40 diamonds: 2^40 paths
 for( int i = 0 ; i < 40 ; ++i )
  for( int k : { 1 , 2 } ) {
   E.emplace_back( 3 * i , 3 * i + k , 1 );
   E.emplace_back( 3 * i + k , 3 * i + 3 , 1 );
   }
 G.push_back( make( 121 , E ) );
 for( const auto & g : G ) {
  const int n = g.bound();
  Ints d( n , INFI );
  std::vector< long long > cnt( n , 0 );
  HeapPriorityQueue< int > S( n );
  std::vector< HeapPriorityQueue< int >::Handle > h( n , nullptr );
  std::vector< bool > done( n , false );
  d[ 0 ] = 0; cnt[ 0 ] = 1; h[ 0 ] = S.insert( 0 , 0 );
  while( ! S.isEmpty() ) {
   const int u = S.deleteMin();
   done[ u ] = true;
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( d[ u ] + w < d[ v ] ) {
     const bool in = d[ v ] < INFI;
     d[ v ] = d[ u ] + w;
     cnt[ v ] = cnt[ u ];
     if( in )
      S.decrease( h[ v ] , d[ v ] );
     else
      h[ v ] = S.insert( v , d[ v ] );
     }
    else if( d[ u ] + w == d[ v ] )
     cnt[ v ] += cnt[ u ];
    } );
   }
  const auto D = floyd( g );
  Ints by( n );
  for( int i = 0 ; i < n ; ++i ) by[ i ] = i;
  std::sort( by.begin() , by.end() , [ & ]( int a , int b ) {
   return( D[ 0 ][ a ] < D[ 0 ][ b ] ); } );
  std::vector< long long > ref( n , 0 );
  ref[ 0 ] = 1;
  for( int u : by )
   if( D[ 0 ][ u ] < INFI )
    g.forEachEdge( u , [ & ]( int v , int w ) {
     if( D[ 0 ][ u ] + w == D[ 0 ][ v ] ) ref[ v ] += ref[ u ]; } );
  CHECK( cnt == ref );
  if( n == 1 )
   CHECK( cnt == std::vector< long long >( { 1 } ) );
  if( n == 3 )
   CHECK( cnt == std::vector< long long >( { 1 , 0 , 0 } ) );
  if( n == 121 )                     // past 2^31, in a long long
   CHECK( cnt[ 120 ] == ( 1LL << 40 ) && d[ 120 ] == 80 );
  }
 }

// the path from r with the shortest edge: the rule of the solution,
// d[ v ] = min( w( u , v ) , d[ u ] ), labels each node with the shortest
// edge of a path that reaches it; against the edges ( a , b ) with a
// reachable from r and v reachable from b; r alone and no edge (all at
// infinity), and a cycle through r (r labelled too)
ADS_TEST( "ex11.8" )
{
 std::vector< WG > G;
 for( unsigned t = 0 ; t < 30 ; ++t )
  G.push_back( random_graph( 15 , 0.15 , 1 , 100 , false , t ) );
 G.push_back( make( 1 , WEdges() ) );                       // r alone
 G.push_back( make( 4 , WEdges() ) );                       // no edge
 G.push_back( make( 3 , WEdges( { { 0 , 1 , 5 } , { 1 , 2 , 3 } ,
                                  { 2 , 0 , 7 } } ) ) );   // back to r
 for( const auto & g : G ) {
  const int n = g.bound();
  Ints d( n , INFI );
  ArrayQueue< int > Q( n );
  std::vector< bool > in( n , false );
  Q.enqueue( 0 ); in[ 0 ] = true;
  while( ! Q.isEmpty() ) {
   const int u = Q.dequeue();
   in[ u ] = false;
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( std::min( w , d[ u ] ) < d[ v ] ) {
     d[ v ] = std::min( w , d[ u ] );
     if( ! in[ v ] ) { Q.enqueue( v ); in[ v ] = true; }
     }
    } );
   }
  const auto D = floyd( g );
  for( int v = 0 ; v < n ; ++v ) {
   int ref = INFI;
   for( int a = 0 ; a < n ; ++a )
    g.forEachEdge( a , [ & ]( int b , int w ) {
     if( D[ 0 ][ a ] < INFI && D[ b ][ v ] < INFI ) ref = std::min( ref , w );
     } );
   CHECK( d[ v ] == ref );
   }
  if( n <= 4 )                          // r unlabelled but on a cycle
   CHECK( d == ( n == 3 ? Ints( { 3 , 3 , 3 } ) : Ints( n , INFI ) ) );
  }
 }

// costs on the nodes: w'( u , v ) = w( v ), and w( r ) added to every
// distance; against Floyd-Warshall on the nodes, also with no edge and
// with all the costs 0
ADS_TEST( "ex11.9" )
{
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 12;
  Ints c( n );
  for( auto & x : c ) x = ( t == 1 ) ? 0 : int( g() % 20 );   // all 0
  const auto base = random_graph( n , t == 0 ? 0.0 : 0.2 , 0 , 0 , false ,
                                  t );                     // t = 0: no edge
  WG h( n , true );
  for( int u = 0 ; u < n ; ++u )
   base.forEachAdj( u , [ & ]( int v ) { h.insertEdge( u , v , c[ v ] ); } );
  const auto R = johnson( h , 0 );
  Ints nd( n , INFI );                     // the costs of the nodes on paths
  nd[ 0 ] = c[ 0 ];
  for( int k = 0 ; k < n ; ++k )
   for( int u = 0 ; u < n ; ++u )
    if( nd[ u ] < INFI )
     base.forEachAdj( u , [ & ]( int v ) {
      nd[ v ] = std::min( nd[ v ] , nd[ u ] + c[ v ] ); } );
  for( int v = 0 ; v < n ; ++v )
   CHECK( R.d[ v ] == INFI ? nd[ v ] == INFI : R.d[ v ] + c[ 0 ] == nd[ v ] );
  }
 }

// the squares of the costs do not keep the shortest paths: ( s , t ) of
// cost 2 against s , a , b , t of costs 1, which squared cost 4 and 3
ADS_TEST( "ex11.10" )
{
 const auto g = make( 4 , WEdges( { { 0 , 3 , 2 } , { 0 , 1 , 1 } ,
                                    { 1 , 2 , 1 } , { 2 , 3 , 1 } } ) );
 const auto q = make( 4 , WEdges( { { 0 , 3 , 4 } , { 0 , 1 , 1 } ,
                                    { 1 , 2 , 1 } , { 2 , 3 , 1 } } ) );
 const auto R = johnson( g , 0 ) , S = johnson( q , 0 );
 CHECK( R.d[ 3 ] == 2 && R.T[ 3 ] == 0 );
 CHECK( S.d[ 3 ] == 3 && S.T[ 3 ] == 2 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch11.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
