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

// the Fibonacci heap against a vector of the items present
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
 }

// Figura 11.1: G2 has the optimal tree T2 of distances 0 , 0 , 1 , -2,
// which satisfies the conditions of Bellman, while the feasible T1 does
// not; G1 has a negative cycle
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
 }

// the prototype with each structure, against Floyd-Warshall with costs not
// negative, and against dagShortestPaths() on acyclic graphs with costs of
// any sign
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
 }

// Dijkstra, Johnson, Fredman-Tarjan: with costs not negative each node is
// extracted once, by increasing distance
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
 for( int n : { 5 , 8 , 12 } ) {
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

// Pape-D'Esopo on the graph of Figura 11.3 extracts 0 , 1 , 2 , 1 , 3
ADS_TEST( "sec11.8.pape" )
{
 CHECK( papeDEsopo( make( 4 , G2 ) , 0 ).order ==
        Ints( { 0 , 1 , 2 , 1 , 3 } ) );
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
// O( n m ), against Floyd-Warshall
ADS_TEST( "ex11.2" )
{
 CHECK( bellmanFordMoore( make( 4 , G1 ) , 0 ).negativeCycle );
 CHECK( ! bellmanFordMoore( make( 4 , G2 ) , 0 ).negativeCycle );
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
// paths enumerated
ADS_TEST( "ex11.3" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 7;
  ListGraph< double > G( n , true ) , L( n , true );
  std::uniform_real_distribution< double > r( 0.05 , 1.0 );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v && g() % 3 == 0 ) {
     const double x = r( g );
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
// but from s, against the enumeration
ADS_TEST( "ex11.4" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 1 + g() % 10;
  const Iv I = random_intervals( n , g ) , J = with_sentinels( I );
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
// but from s, against the enumeration
ADS_TEST( "ex11.5" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + g() % 9;
  const Iv I = random_intervals( n , g ) , J = with_sentinels( I );
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
// nodes u_IN and u_OUT per interval, against the enumeration
ADS_TEST( "ex11.6" )
{
 std::mt19937 g( 6 );
 int feasible = 0;
 for( int t = 0 ; t < 150 ; ++t ) {
  const int n = 1 + g() % 8;
  const Iv I = random_intervals( n , g ) , J = with_sentinels( I );
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
  int best = INFI;
  for( int m = 1 ; m < ( 1 << n ) ; ++m ) {
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
// distance adds it; against the counts on the tight edges of Floyd-Warshall
ADS_TEST( "ex11.7" )
{
 for( unsigned t = 0 ; t < 30 ; ++t ) {
  const auto g = random_graph( 20 , 0.25 , 1 , 3 , false , t );
  const int n = 20;
  Ints d( n , INFI );
  std::vector< long > cnt( n , 0 );
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
  std::vector< long > ref( n , 0 );
  ref[ 0 ] = 1;
  for( int u : by )
   if( D[ 0 ][ u ] < INFI )
    g.forEachEdge( u , [ & ]( int v , int w ) {
     if( D[ 0 ][ u ] + w == D[ 0 ][ v ] ) ref[ v ] += ref[ u ]; } );
  CHECK( cnt == ref );
  }
 }

// the path from r with the shortest edge: the rule of the solution,
// d[ v ] = min( w( u , v ) , d[ u ] ), labels each node with the shortest
// edge of a path that reaches it; against the edges ( a , b ) with a
// reachable from r and v reachable from b
ADS_TEST( "ex11.8" )
{
 for( unsigned t = 0 ; t < 30 ; ++t ) {
  const auto g = random_graph( 15 , 0.15 , 1 , 100 , false , t );
  const int n = 15;
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
  }
 }

// costs on the nodes: w'( u , v ) = w( v ), and w( r ) added to every
// distance; against Floyd-Warshall on the nodes
ADS_TEST( "ex11.9" )
{
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 12;
  Ints c( n );
  for( auto & x : c ) x = g() % 20;
  const auto base = random_graph( n , 0.2 , 0 , 0 , false , t );
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
