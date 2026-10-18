/*--------------------------------------------------------------------------*/
/*------------------------ File exam_collezionib.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the exercises of the collections of chapters 11 to 15 of the
 * book and of the various ones: the shortest paths and their variants,
 * divide et impera, dynamic programming, the greedy algorithms on spanning
 * trees, the networks of flow, and a few problems on vectors and on an
 * augmented search tree. Each exercise is a test named after it, checked
 * on the examples of its text, against an oracle (a brute force, the
 * standard library, or another algorithm of ADS) on random instances with
 * a fixed seed, and on its edge cases. Where ADS already has the algorithm,
 * the test calls it on the exercise instead of rewriting it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 11 to 15,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
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

#include "ADS/algorithm/Subsequences.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/search/Selection.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/CountingSort.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/WordBreak.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Arcs3 = std::vector< std::tuple< int , int , long > >;
using Longs = std::vector< long >;
using WGraph = ListGraph< int >;
using DGraph = ListGraph< double >;

/// an infinite cost that can still be added to a small one
constexpr long LINF = std::numeric_limits< long >::max() / 4;

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer drawn uniformly from [ lo , hi ]
int draw( std::mt19937 & g , int lo , int hi )
{
 return( std::uniform_int_distribution< int >( lo , hi )( g ) );
 }

/// a vector of n integers drawn from [ lo , hi ]
Ints drawInts( std::mt19937 & g , int n , int lo , int hi )
{
 Ints A( n );
 for( int & x : A ) x = draw( g , lo , hi );
 return( A );
 }

/// an oriented graph of n nodes, each edge there with probability p (in
/// percent), of weight in [ lo , hi ]
WGraph randomGraph( std::mt19937 & g , int n , int p , int lo , int hi )
{
 WGraph G( n );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && ( draw( g , 1 , 100 ) <= p ) )
    G.insertEdge( u , v , draw( g , lo , hi ) );
 return( G );
 }

/// f( P ) on each simple path P from s to t of the graph, by backtrack
template< class G , class F >
void simplePaths( const G & g , int s , int t , F && f )
{
 std::vector< bool > on( g.bound() , false );
 Ints P = { s };
 on[ s ] = true;
 std::function< void( int ) > rec = [ & ]( int u ) {
  if( u == t ) {
   f( P );
   return;
   }
  for( int v : g.adj( u ) )
   if( ! on[ v ] ) {
    on[ v ] = true;
    P.push_back( v );
    rec( v );
    P.pop_back();
    on[ v ] = false;
    }
  };
 rec( s );
 }

/// the weight of the path P, by the weights of its edges
template< class G >
typename G::Weight pathWeight( const G & g , const Ints & P )
{
 typename G::Weight w{};
 for( std::size_t i = 1 ; i < P.size() ; ++i )
  w = w + *g.weight( P[ i - 1 ] , P[ i ] );
 return( w );
 }

/// the closure of reachability of a graph: R[ u ][ v ] if v is reachable
/// from u, by a visit from each node
template< class G >
std::vector< std::vector< bool > > reach( const G & g )
{
 const int n = g.bound();
 std::vector< std::vector< bool > > R( n , std::vector< bool >( n ) );
 for( int u : g.V() ) {
  const auto d = distance( g , u ).first;
  for( int v : g.V() ) R[ u ][ v ] = ( d[ v ] != ADS::INF );
  }
 return( R );
 }

/// the capacity of a minimum cut between the nodes with S[ v ] = 1 and
/// those with S[ v ] = 2, by trying every side of the other nodes; the
/// edges are ( u , v , c )
long minCutBrute( int n , const Arcs3 & E ,
                  const Ints & S )
{
 Ints free;
 for( int v = 0 ; v < n ; ++v )
  if( S[ v ] == 0 ) free.push_back( v );
 long best = LINF;
 for( std::uint64_t mask = 0 ; mask < ( std::uint64_t( 1 ) << free.size() ) ;
      ++mask ) {
  std::vector< bool > side( n );           // true on the side of s
  for( int v = 0 ; v < n ; ++v ) side[ v ] = ( S[ v ] == 1 );
  for( std::size_t i = 0 ; i < free.size() ; ++i )
   side[ free[ i ] ] = ( mask >> i ) & 1;
  long c = 0;
  for( auto [ u , v , w ] : E )
   if( side[ u ] && ! side[ v ] ) c += w;
  best = std::min( best , c );
  }
 return( best );
 }

/// the maximum flow from s to t of the edges ( u , v , c )
long maxFlowOf( int n , const Arcs3 & E ,
                int s , int t )
{
 FlowNetwork F( n );
 for( auto [ u , v , c ] : E ) F.addEdge( u , v , c );
 return( F.maxFlow( s , t ) );
 }

/// the minimum cut from s to t of the edges, by brute force
long minCutBrute( int n , const Arcs3 & E ,
                  int s , int t )
{
 Ints S( n , 0 );
 S[ s ] = 1;
 S[ t ] = 2;
 return( minCutBrute( n , E , S ) );
 }

}  // end( namespace )

namespace collezioni {

/*--------------------------------------------------------------------------*/
/*------------------- COLLECTION 11, SHORTEST PATHS ------------------------*/
/*--------------------------------------------------------------------------*/

// collezione 11-strutture, 1.2: Algoritmo di Bellman-Ford-Moore

/// negativeCycle(): whether the oriented graph has a cycle of negative
/// weight anywhere. Bellman-Ford-Moore from a virtual source joined to
/// every node by an edge of weight 0, i.e., d[ u ] = 0 for each u at the
/// start; after n - 1 rounds of relaxation of all the edges the distances
/// are final unless a negative cycle exists, so a change in the n-th round
/// proves one; O( n m ) time, Theta( n ) space
template< Graph G >
bool negativeCycle( const G & g )
{
 const int n = g.n();
 if( n == 0 ) return( false );
 std::vector< long > d( g.bound() , 0 );
 for( int k = 1 ; k <= n ; ++k ) {
  bool changed = false;
  for( int u : g.V() )
   g.forEachEdge( u , [ & ]( int v , const typename G::Weight & w ) {
    if( d[ u ] + long( w ) < d[ v ] ) {
     d[ v ] = d[ u ] + long( w );
     changed = true;
     }
    } );
  if( ! changed ) return( false );
  }
 return( true );
 }

// collezione 11-strutture, 1.3: Affidabilita

/// mostReliable(): a path from s to t whose product of the reliabilities
/// r( u , v ) in [ 0 , 1 ] is the largest, and that product. The costs
/// -log r of the edges with r > 0 are not negative and turn the product
/// into a sum, so Dijkstra (on a heap) finds the path; O( m log n ). If no
/// path of edges with r > 0 reaches t, the result is empty with product 0
/// (any path there is has reliability 0)
/// \pre s and t are nodes, each reliability in [ 0 , 1 ]
std::pair< Ints , double > mostReliable( const DGraph & g , int s , int t )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) );
 DGraph c( g.bound() );
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , double r ) {
   ADS_PRE( ( r >= 0 ) && ( r <= 1 ) );
   if( r > 0 ) c.insertEdge( u , v , r == 1 ? 0.0 : -std::log( r ) );
   } );
 const auto R = johnson( c , s );
 if( R.d[ t ] == infinity< double >() ) return( std::pair( Ints() , 0.0 ) );
 return( std::pair( path( s , t , R.T ) , std::exp( -R.d[ t ] ) ) );
 }

// collezione 11-strutture, 1.4: Numero di cammini minimi

/// pathCount(): the number of shortest paths from s to t, the lengths
/// positive. Dijkstra on a vector of distances: a relaxation that lowers
/// d[ v ] gives v the paths of u, one that equals it adds them, and since
/// the lengths are positive each predecessor of v on a shortest path is
/// extracted before v; O( n^2 + m ) time
/// \pre s and t are nodes, the lengths positive
std::uint64_t pathCount( const WGraph & g , int s , int t )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) );
 const int n = g.bound();
 std::vector< long > d( n , LINF );
 std::vector< std::uint64_t > count( n , 0 );
 std::vector< bool > done( n , false );
 d[ s ] = 0;
 count[ s ] = 1;
 for( ; ; ) {
  int u = -1;
  for( int x : g.V() )
   if( ! done[ x ] && ( d[ x ] < LINF ) &&
       ( ( u < 0 ) || ( d[ x ] < d[ u ] ) ) )
    u = x;
  if( u < 0 ) break;
  done[ u ] = true;
  g.forEachEdge( u , [ & ]( int v , int w ) {
   ADS_PRE( w > 0 );
   if( d[ u ] + w < d[ v ] ) {
    d[ v ] = d[ u ] + w;
    count[ v ] = count[ u ];
    }
   else if( d[ u ] + w == d[ v ] ) count[ v ] += count[ u ];
   } );
  }
 return( count[ t ] );
 }

// collezione 11-strutture, 1.5: Cammino con arco piu corto

/// shortestEdgePath(): a path from r to s through the shortest edge among
/// those that lie on some path from r to s, empty if there is none. A visit
/// from r gives the nodes reached from r, one from s in the transposed
/// graph those that reach s; ( u , v ) can be on a path iff u is in the
/// first set and v in the second; the path goes from r to u along the tree
/// of the first visit, then ( u , v ), then from v to s along the tree of
/// the second; O( n + m ) time. The path may repeat nodes, as in the text
/// \pre r and s are nodes
Ints shortestEdgePath( const WGraph & g , int r , int s )
{
 ADS_PRE( g.hasNode( r ) && g.hasNode( s ) );
 const auto [ dr , pr ] = distance( g , r );
 const auto [ ds , ps ] = distance( transpose( g ) , s );
 int bu = NIL , bv = NIL , bw = 0;
 for( int u : g.V() )
  if( dr[ u ] != ADS::INF )
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( ( ds[ v ] != ADS::INF ) && ( ( bu == NIL ) || ( w < bw ) ) ) {
     bu = u;
     bv = v;
     bw = w;
     }
    } );
 if( bu == NIL ) return( Ints() );
 Ints P = path( r , bu , pr );
 for( int x = bv ; ; x = ps[ x ] ) {       // the parents lead to s in g
  P.push_back( x );
  if( x == s ) break;
  }
 return( P );
 }

}  // end( namespace collezioni )

/*--------------------------------------------------------------------------*/

// collezione 11-strutture, 1.2: a negative cycle anywhere, against the
// diagonal of Floyd-Warshall and against reachesNegativeCycle() from each
// node, on random graphs with weights of both signs
ADS_TEST( "collezioni-11-strutture-1-2" )
{
 using collezioni::negativeCycle;
 CHECK( ! negativeCycle( WGraph( 0 ) ) && ! negativeCycle( WGraph( 1 ) ) );
 WGraph c( 3 );
 c.insertEdge( 0 , 1 , 1 );
 c.insertEdge( 1 , 0 , -1 );               // weight 0: not negative
 CHECK( ! negativeCycle( c ) );
 c.insertEdge( 2 , 1 , -5 );               // 2 reaches no cycle
 CHECK( ! negativeCycle( c ) );
 c.insertEdge( 1 , 0 , -2 );
 CHECK( negativeCycle( c ) );
 std::mt19937 g( 112 );
 int yes = 0;
 for( int t = 0 ; t < 400 ; ++t ) {
  const WGraph G = randomGraph( g , draw( g , 1 , 7 ) , 30 , -4 , 9 );
  bool any = false;
  for( int r : G.V() ) any = any || reachesNegativeCycle( G , r );
  const bool nc = negativeCycle( G );
  CHECK( nc == any );
  CHECK( nc == floydWarshall( G ).negativeCycle );
  yes += nc;
  }
 CHECK( ( yes > 20 ) && ( yes < 380 ) );   // both answers were tried
 }

// collezione 11-strutture, 1.3: the most reliable path against the
// product of every simple path, on small random graphs
ADS_TEST( "collezioni-11-strutture-1-3" )
{
 DGraph e( 3 );
 e.insertEdge( 0 , 1 , 0.5 );
 e.insertEdge( 1 , 2 , 0.5 );
 e.insertEdge( 0 , 2 , 0.2 );              // 0.25 by 1 beats 0.2
 auto [ P , q ] = collezioni::mostReliable( e , 0 , 2 );
 CHECK( ( P == Ints( { 0 , 1 , 2 } ) ) && ( std::abs( q - 0.25 ) < 1e-12 ) );
 CHECK( collezioni::mostReliable( e , 2 , 0 ).first.empty() );
 CHECK( collezioni::mostReliable( e , 1 , 1 ) == std::pair( Ints{ 1 } ,
                                                             1.0 ) );
 std::mt19937 g( 113 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = draw( g , 2 , 6 );
  DGraph G( n );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && ( draw( g , 0 , 2 ) == 0 ) )
     G.insertEdge( u , v , draw( g , 0 , 10 ) / 10.0 );
  const int s = draw( g , 0 , n - 1 ) , d = draw( g , 0 , n - 1 );
  double best = 0;
  simplePaths( G , s , d , [ & ]( const Ints & Q ) {
   double r = 1;
   for( std::size_t i = 1 ; i < Q.size() ; ++i )
    r *= *G.weight( Q[ i - 1 ] , Q[ i ] );
   best = std::max( best , r );
   } );
  const auto [ R , p ] = collezioni::mostReliable( G , s , d );
  CHECK( std::abs( p - best ) < 1e-9 );
  if( ! R.empty() ) {
   double r = 1;
   for( std::size_t i = 1 ; i < R.size() ; ++i )
    r *= *G.weight( R[ i - 1 ] , R[ i ] );
   CHECK( ( R.front() == s ) && ( R.back() == d ) &&
          ( std::abs( r - p ) < 1e-9 ) );
   }
  }
 }

// collezione 11-strutture, 1.4: the number of shortest paths against the
// enumeration of the simple paths, the lengths in [ 1 , 3 ] so that ties
// are frequent; on a grid of k x k unit edges they are C( 2k - 2 , k - 1 )
ADS_TEST( "collezioni-11-strutture-1-4" )
{
 const int k = 6;
 WGraph grid( k * k );
 for( int i = 0 ; i < k ; ++i )
  for( int j = 0 ; j < k ; ++j ) {
   if( i + 1 < k ) grid.insertEdge( i * k + j , ( i + 1 ) * k + j , 1 );
   if( j + 1 < k ) grid.insertEdge( i * k + j , i * k + j + 1 , 1 );
   }
 CHECK( collezioni::pathCount( grid , 0 , k * k - 1 ) == 252 );
 CHECK( collezioni::pathCount( grid , k * k - 1 , 0 ) == 0 );
 CHECK( collezioni::pathCount( grid , 7 , 7 ) == 1 );
 std::mt19937 g( 114 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const WGraph G = randomGraph( g , draw( g , 1 , 7 ) , 40 , 1 , 3 );
  const int s = draw( g , 0 , G.n() - 1 ) , d = draw( g , 0 , G.n() - 1 );
  long best = LINF;
  std::uint64_t count = 0;
  simplePaths( G , s , d , [ & ]( const Ints & Q ) {
   const long w = pathWeight( G , Q );
   if( w < best ) { best = w; count = 0; }
   if( w == best ) ++count;
   } );
  CHECK( collezioni::pathCount( G , s , d ) == count );
  }
 }

// collezione 11-strutture, 1.5: the path found is a path of the graph from
// r to s through an edge of the least weight among the edges ( u , v )
// with u reachable from r and s from v
ADS_TEST( "collezioni-11-strutture-1-5" )
{
 WGraph e( 4 );
 e.insertEdge( 0 , 1 , 5 );
 e.insertEdge( 1 , 3 , 5 );
 e.insertEdge( 1 , 2 , 1 );                // 2 is a dead end
 e.insertEdge( 3 , 0 , 2 );                // on the way back to 0
 CHECK( collezioni::shortestEdgePath( e , 0 , 3 ) ==
        Ints( { 0 , 1 , 3 , 0 , 1 , 3 } ) );
 CHECK( collezioni::shortestEdgePath( e , 2 , 0 ).empty() );
 std::mt19937 g( 115 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const WGraph G = randomGraph( g , draw( g , 1 , 8 ) , 25 , 1 , 20 );
  const int r = draw( g , 0 , G.n() - 1 ) , s = draw( g , 0 , G.n() - 1 );
  const auto R = reach( G );
  int best = ADS::INF;
  for( int u : G.V() )
   G.forEachEdge( u , [ & ]( int v , int w ) {
    if( R[ r ][ u ] && R[ v ][ s ] ) best = std::min( best , w );
    } );
  const Ints P = collezioni::shortestEdgePath( G , r , s );
  if( best == ADS::INF ) {
   CHECK( P.empty() );
   continue;
   }
  CHECK( ( P.size() >= 2 ) && ( P.front() == r ) && ( P.back() == s ) );
  int least = ADS::INF;
  for( std::size_t i = 1 ; i < P.size() ; ++i ) {
   CHECK( G.hasEdge( P[ i - 1 ] , P[ i ] ) );
   least = std::min( least , *G.weight( P[ i - 1 ] , P[ i ] ) );
   }
  CHECK( least == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------- COLLECTION 12, DIVIDE ET IMPERA ----------------------*/
/*--------------------------------------------------------------------------*/

namespace collezioni {

// collezione 12-divide, 1.2: Analisi Quicksort

/// the Quicksort of the book on A[ start .. end ], whose perno() moves to
/// the left only the items smaller than the pivot A[ start ]; the
/// comparisons made are added to *cmp
void quickSortCounted( Ints & A , int start , int end , long & cmp )
{
 if( start >= end ) return;
 const int x = A[ start ];
 int j = start;
 for( int i = start + 1 ; i <= end ; ++i ) {
  ++cmp;
  if( A[ i ] < x ) std::swap( A[ i ] , A[ ++j ] );
  }
 std::swap( A[ start ] , A[ j ] );
 quickSortCounted( A , start , j - 1 , cmp );
 quickSortCounted( A , j + 1 , end , cmp );
 }

// collezione 12-divide, 1.5: Prodotto di numeri complessi

/// complexProduct(): ( a + b i ) ( c + d i ) with three products, m1 = a c,
/// m2 = b d, m3 = ( a + b ) ( c + d ), the real part m1 - m2 and the
/// imaginary one m3 - m1 - m2; 3 products and 5 sums
template< class T >
std::pair< T , T > complexProduct( const T & a , const T & b , const T & c ,
                                   const T & d )
{
 const T m1 = a * c , m2 = b * d , m3 = ( a + b ) * ( c + d );
 return( std::pair( m1 - m2 , m3 - m1 - m2 ) );
 }

// collezione 12-divide, 1.6: Chi manca?

/// missing(): the value of 0 , ... , n missing from the sorted A of n
/// distinct values, the first place i with A[ i ] != i, by binary search;
/// Theta( log n )
/// \pre A is sorted, its values distinct in [ 0 , n ]
int missing( const Ints & A )
{
 int i = 0 , j = int( A.size() );
 while( i < j ) {
  const int m = ( i + j ) / 2;
  if( A[ m ] == m ) i = m + 1;
  else j = m;
  }
 return( i );
 }

// collezione 12-divide, 1.7: Punto fisso

/// fixedPoint(): whether the sorted A of distinct integers has A[ i ] = i;
/// A[ i ] - i does not decrease, so a binary search finds its zero;
/// Theta( log n )
/// \pre A is sorted, its values distinct
bool fixedPoint( const Ints & A )
{
 int i = 0 , j = int( A.size() ) - 1;
 while( i <= j ) {
  const int m = ( i + j ) / 2;
  if( A[ m ] == m ) return( true );
  if( A[ m ] < m ) i = m + 1;
  else j = m - 1;
  }
 return( false );
 }

// collezione 12-divide, 1.8: Ricerca in vettori unimodulari

/// unimodalMin(): the minimum of A, strictly decreasing and then strictly
/// increasing (either part possibly empty): it is in the half of the
/// smaller of A[ m ] and A[ m + 1 ]; Theta( log n )
/// \pre A is not empty and unimodal
int unimodalMin( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 std::size_t start = 0 , end = A.size() - 1;
 while( start < end ) {
  const std::size_t m = ( start + end ) / 2;
  if( A[ m ] < A[ m + 1 ] ) end = m;
  else start = m + 1;
  }
 return( A[ start ] );
 }

// collezione 12-divide, 1.9: Massima somma di sottovettori

/// a subvector A[ start .. end ] and its sum
struct Sub {
 int start , end;
 long sum;
 };

/// maxSumCubic(): the non-empty subvector of largest sum, by summing each
/// one; Theta( n^3 )
/// \pre A is not empty
Sub maxSumCubic( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 Sub best{ 0 , 0 , A[ 0 ] };
 for( int i = 0 ; i < int( A.size() ) ; ++i )
  for( int j = i ; j < int( A.size() ) ; ++j ) {
   long s = 0;
   for( int k = i ; k <= j ; ++k ) s += A[ k ];
   if( s > best.sum ) best = Sub{ i , j , s };
   }
 return( best );
 }

/// maxSumQuadratic(): the same, the sum from i kept while j grows;
/// Theta( n^2 )
/// \pre A is not empty
Sub maxSumQuadratic( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 Sub best{ 0 , 0 , A[ 0 ] };
 for( int i = 0 ; i < int( A.size() ) ; ++i ) {
  long s = 0;
  for( int j = i ; j < int( A.size() ) ; ++j ) {
   s += A[ j ];
   if( s > best.sum ) best = Sub{ i , j , s };
   }
  }
 return( best );
 }

/// the best subvector of A[ start .. end ]: the best of the left half, of
/// the right one, or of those across the middle, the best suffix of the
/// left half joined to the best prefix of the right one
Sub maxSumRec( const Ints & A , int start , int end )
{
 if( start == end ) return( Sub{ start , start , A[ start ] } );
 const int mid = ( start + end ) / 2;
 const Sub l = maxSumRec( A , start , mid ) , r = maxSumRec( A , mid + 1 ,
                                                            end );
 long s = 0 , bl = std::numeric_limits< long >::min() , br = bl;
 int i0 = mid , j0 = mid + 1;
 for( int i = mid ; i >= start ; --i ) {
  s += A[ i ];
  if( s > bl ) { bl = s; i0 = i; }
  }
 s = 0;
 for( int j = mid + 1 ; j <= end ; ++j ) {
  s += A[ j ];
  if( s > br ) { br = s; j0 = j; }
  }
 const Sub c{ i0 , j0 , bl + br };
 if( ( l.sum >= r.sum ) && ( l.sum >= c.sum ) ) return( l );
 return( r.sum >= c.sum ? r : c );
 }

/// maxSum(): the same by divide et impera; Theta( n log n )
/// \pre A is not empty
Sub maxSum( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 return( maxSumRec( A , 0 , int( A.size() ) - 1 ) );
 }

// collezione 12-divide, 1.10: Valore maggioritario

/// whether x is in more than half of the places of A
bool isMajority( const Ints & A , int x )
{
 return( 2 * std::count( A.begin() , A.end() , x ) > long( A.size() ) );
 }

/// majoritySort(): whether a value is in more than half of the places of
/// A: if so, it is the median of the sorted A (heapsort); Theta( n log n )
/// \pre A is not empty
bool majoritySort( Ints A )
{
 ADS_PRE( ! A.empty() );
 heapsort( A );
 return( isMajority( A , A[ A.size() / 2 ] ) );
 }

/// majoritySelect(): the same, the candidate of rank floor( n / 2 ) + 1
/// found by the selection in linear time; Theta( n )
/// \pre A is not empty
bool majoritySelect( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 Ints B = A;
 return( isMajority( A , medianOfMedians( B , A.size() / 2 + 1 ) ) );
 }

// collezione 12-divide, 1.11: Poligoni

/// highest(): the place of the highest vertex of a convex polygon whose
/// vertices, counterclockwise, start from the lowest one: along them y
/// grows and then decreases, so the binary search of a peak finds it;
/// Theta( log n )
/// \pre at least 3 vertices, the first the lowest, the y distinct
int highest( const std::vector< std::pair< double , double > > & V )
{
 ADS_PRE( V.size() >= 3 );
 int i = 0 , j = int( V.size() ) - 1;
 while( i < j ) {
  const int m = ( i + j ) / 2;
  if( V[ m ].second < V[ m + 1 ].second ) i = m + 1;
  else j = m;
  }
 return( i );
 }

// collezione 12-divide, 1.12: Algorithms-R-us

/// findIncrease(): a place i with A[ i ] < A[ i + 1 ], keeping A[ start ] <
/// A[ end ] on the part [ start , end ] that is halved; Theta( log n )
/// \pre at least two items, A[ 0 ] < A[ n - 1 ]
int findIncrease( const Ints & A )
{
 ADS_PRE( ( A.size() >= 2 ) && ( A.front() < A.back() ) );
 int start = 0 , end = int( A.size() ) - 1;
 while( start + 1 < end ) {
  const int mid = ( start + end ) / 2;
  if( A[ start ] < A[ mid ] ) end = mid;
  else start = mid;
  }
 return( start );
 }

// collezione 12-divide, 1.13: Samarcanda

/// the minimum, the maximum and the best gain P[ y ] - P[ x ], x < y, of a
/// part of the prices
struct Gain {
 int min , max;
 long gain;
 };

/// the gain of P[ start .. end ]: that of either half, or the maximum of
/// the right one minus the minimum of the left one
Gain bestGainRec( const Ints & P , int start , int end )
{
 if( start == end )
  return( Gain{ P[ start ] , P[ start ] ,
                std::numeric_limits< long >::min() } );
 const int mid = ( start + end ) / 2;
 const Gain l = bestGainRec( P , start , mid ) ,
            r = bestGainRec( P , mid + 1 , end );
 return( Gain{ std::min( l.min , r.min ) , std::max( l.max , r.max ) ,
               std::max( { l.gain , r.gain , long( r.max ) - l.min } ) } );
 }

/// bestGain(): the largest P[ y ] - P[ x ] with x < y, possibly negative,
/// by divide et impera; T( n ) = 2 T( n / 2 ) + Theta( 1 ) = Theta( n )
/// \pre at least two prices
long bestGain( const Ints & P )
{
 ADS_PRE( P.size() >= 2 );
 return( bestGainRec( P , 0 , int( P.size() ) - 1 ).gain );
 }

// collezione 12-divide, 1.14: Sottovettori sottili

/// the longest thin subvector of V[ start .. end ]: the best of the halves
/// or one across the middle, found by two indices on the minima and the
/// maxima of the suffixes of the left half and of the prefixes of the
/// right one, kept in mn and mx shared by all the calls
int thinRec( const Ints & V , long C , int start , int end , Ints & mn ,
             Ints & mx )
{
 if( start == end ) return( 1 );
 const int mid = ( start + end ) / 2;
 int best = std::max( thinRec( V , C , start , mid , mn , mx ) ,
                      thinRec( V , C , mid + 1 , end , mn , mx ) );
 mn[ mid ] = mx[ mid ] = V[ mid ];
 for( int i = mid - 1 ; i >= start ; --i ) {
  mn[ i ] = std::min( mn[ i + 1 ] , V[ i ] );
  mx[ i ] = std::max( mx[ i + 1 ] , V[ i ] );
  }
 mn[ mid + 1 ] = mx[ mid + 1 ] = V[ mid + 1 ];
 for( int j = mid + 2 ; j <= end ; ++j ) {
  mn[ j ] = std::min( mn[ j - 1 ] , V[ j ] );
  mx[ j ] = std::max( mx[ j - 1 ] , V[ j ] );
  }
 int right = mid;                          // the last place on the right
 for( int left = start ; left <= mid ; ++left ) {
  while( ( right < end ) &&
         ( long( std::max( mx[ left ] , mx[ right + 1 ] ) ) -
           std::min( mn[ left ] , mn[ right + 1 ] ) <= C ) )
   ++right;
  if( right > mid ) best = std::max( best , right - left + 1 );
  }
 return( best );
 }

/// thinSubarray(): the largest length of a subvector whose maximum minus
/// minimum is at most C; T( n ) = 2 T( n / 2 ) + Theta( n ) =
/// Theta( n log n )
/// \pre V is not empty, C >= 0
int thinSubarray( const Ints & V , long C )
{
 ADS_PRE( ! V.empty() && ( C >= 0 ) );
 Ints mn( V.size() ) , mx( V.size() );
 return( thinRec( V , C , 0 , int( V.size() ) - 1 , mn , mx ) );
 }

}  // end( namespace collezioni )

namespace {

/// an integer that counts its products
struct Counted {
 std::int64_t v;
 static inline long products = 0;
 };

Counted operator*( Counted a , Counted b )
{
 ++Counted::products;
 return( Counted{ a.v * b.v } );
 }

Counted operator+( Counted a , Counted b ) { return( Counted{ a.v + b.v } ); }

Counted operator-( Counted a , Counted b ) { return( Counted{ a.v - b.v } ); }

/// a vector of n distinct integers in increasing order, from [ lo , hi ]
Ints drawSortedDistinct( std::mt19937 & g , int n , int lo , int hi )
{
 std::set< int > S;
 while( int( S.size() ) < n ) S.insert( draw( g , lo , hi ) );
 return( Ints( S.begin() , S.end() ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

// collezione 12-divide, 1.1: Merge Sort and Quicksort of ADS on a list of
// pointers, against std::stable_sort; Merge Sort is stable
ADS_TEST( "collezioni-12-divide-1-1" )
{
 using P = std::pair< int , int >;
 const auto first = []( const P & a , const P & b ) {
  return( a.first < b.first ); };
 std::mt19937 g( 121 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< P > A( draw( g , 0 , 60 ) );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   A[ i ] = P( draw( g , 0 , t % 3 == 0 ? 0 : 9 ) , int( i ) );
  List< P > L( A.begin() , A.end() ) , Q = L;
  mergeSort( L , first );
  quickSort( Q , first );
  std::stable_sort( A.begin() , A.end() , first );
  CHECK( std::vector< P >( L.begin() , L.end() ) == A );
  std::vector< P > B( Q.begin() , Q.end() );
  CHECK( std::is_sorted( B.begin() , B.end() , first ) );
  std::sort( B.begin() , B.end() );
  std::sort( A.begin() , A.end() );
  CHECK( B == A );
  }
 }

// collezione 12-divide, 1.2: on n equal keys the Quicksort of the book
// leaves the pivot in the first place, and makes n ( n - 1 ) / 2
// comparisons; the comparisons of quickSort() of ADS grow by 4 when n
// doubles, Theta( n^2 ), while on distinct keys they are far fewer
ADS_TEST( "collezioni-12-divide-1-2" )
{
 for( int n : { 0 , 1 , 2 , 10 , 100 , 500 } ) {
  Ints A( n , 7 );
  long cmp = 0;
  collezioni::quickSortCounted( A , 0 , n - 1 , cmp );
  CHECK( cmp == long( n ) * ( n - 1 ) / 2 );
  }
 std::mt19937 g( 122 );
 Ints D( 500 );
 std::iota( D.begin() , D.end() , 0 );
 std::shuffle( D.begin() , D.end() , g );
 long cmp = 0;
 collezioni::quickSortCounted( D , 0 , 499 , cmp );
 CHECK( std::is_sorted( D.begin() , D.end() ) && ( cmp < 500 * 499 / 8 ) );
 const auto count = []( int n ) {
  Ints A( n , 7 );
  long c = 0;
  quickSort( A , [ & ]( int a , int b ) { ++c; return( a < b ); } );
  return( double( c ) );
  };
 const double r = count( 800 ) / count( 400 );
 CHECK( ( r > 3.6 ) && ( r < 4.4 ) );
 }

// collezione 12-divide, 1.3: the iterative Merge Sort of ADS against
// std::stable_sort, the sizes not only powers of two
ADS_TEST( "collezioni-12-divide-1-3" )
{
 using P = std::pair< int , int >;
 const auto first = []( const P & a , const P & b ) {
  return( a.first < b.first ); };
 std::mt19937 g( 123 );
 for( int n = 0 ; n <= 130 ; ++n )
  for( int t = 0 ; t < 3 ; ++t ) {
   std::vector< P > A( n );
   for( int i = 0 ; i < n ; ++i )
    A[ i ] = P( draw( g , 0 , t == 0 ? 3 : 1000 ) , i );
   std::vector< P > B = A;
   mergeSortBottomUp( A , first );
   std::stable_sort( B.begin() , B.end() , first );
   CHECK( A == B );
   }
 }

// collezione 12-divide, 1.5: three products, against std::complex, on
// random integers and on the largest ones whose products fit
ADS_TEST( "collezioni-12-divide-1-5" )
{
 std::mt19937 g( 125 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int a = draw( g , -1000 , 1000 ) , b = draw( g , -1000 , 1000 ) ,
            c = draw( g , -1000 , 1000 ) , d = draw( g , -1000 , 1000 );
  Counted::products = 0;
  const auto [ re , im ] = collezioni::complexProduct(
                          Counted{ a } , Counted{ b } , Counted{ c } ,
                          Counted{ d } );
  CHECK( Counted::products == 3 );
  const std::complex< double > z = std::complex< double >( a , b ) *
                                   std::complex< double >( c , d );
  CHECK( ( double( re.v ) == z.real() ) && ( double( im.v ) == z.imag() ) );
  }
 const std::int64_t M = std::int64_t( 1 ) << 30;
 const auto [ re , im ] = collezioni::complexProduct( M , M , M , -M );
 CHECK( ( re == 2 * M * M ) && ( im == 0 ) );
 CHECK( collezioni::complexProduct( 0 , 1 , 0 , 1 ) == std::pair( -1 , 0 ) );
 }

// collezione 12-divide, 1.6: every value missing from every n up to 64,
// against the formula n ( n + 1 ) / 2 minus the sum
ADS_TEST( "collezioni-12-divide-1-6" )
{
 for( int n = 0 ; n <= 64 ; ++n )
  for( int k = 0 ; k <= n ; ++k ) {
   Ints A;
   for( int v = 0 ; v <= n ; ++v )
    if( v != k ) A.push_back( v );
   CHECK( collezioni::missing( A ) == k );
   CHECK( n * ( n + 1 ) / 2 - std::accumulate( A.begin() , A.end() , 0 ) ==
          k );
   }
 }

// collezione 12-divide, 1.7: the binary search against a linear scan on
// sorted distinct integers, negative ones included
ADS_TEST( "collezioni-12-divide-1-7" )
{
 CHECK( ! collezioni::fixedPoint( Ints() ) );
 CHECK( collezioni::fixedPoint( Ints{ 0 } ) );
 CHECK( ! collezioni::fixedPoint( Ints{ 1 } ) );
 std::mt19937 g( 127 );
 int yes = 0;
 for( int t = 0 ; t < 2000 ; ++t ) {
  const int n = draw( g , 1 , 30 );
  const Ints A = drawSortedDistinct( g , n , -20 , 40 );
  bool any = false;
  for( int i = 0 ; i < n ; ++i ) any = any || ( A[ i ] == i );
  CHECK( collezioni::fixedPoint( A ) == any );
  yes += any;
  }
 CHECK( ( yes > 100 ) && ( yes < 1900 ) );
 }

// collezione 12-divide, 1.8: the minimum of random unimodal vectors, the
// smaller values put at random before or after the minimum
ADS_TEST( "collezioni-12-divide-1-8" )
{
 CHECK( collezioni::unimodalMin( Ints{ 5 } ) == 5 );
 CHECK( collezioni::unimodalMin( Ints{ 1 , 2 , 3 } ) == 1 );
 CHECK( collezioni::unimodalMin( Ints{ 3 , 2 , 1 } ) == 1 );
 std::mt19937 g( 128 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  const Ints S = drawSortedDistinct( g , draw( g , 1 , 40 ) , -100 , 100 );
  Ints L , R;
  for( std::size_t i = 1 ; i < S.size() ; ++i )
   ( draw( g , 0 , 1 ) ? L : R ).push_back( S[ i ] );
  Ints A( L.rbegin() , L.rend() );
  A.push_back( S[ 0 ] );
  A.insert( A.end() , R.begin() , R.end() );
  CHECK( collezioni::unimodalMin( A ) == S[ 0 ] );
  }
 }

// collezione 12-divide, 1.9: the three versions agree on the sum, their
// places give it, and it is the sum of Kadane in ADS (the largest item if
// all are negative, since ADS allows the empty subvector)
ADS_TEST( "collezioni-12-divide-1-9" )
{
 const Ints E = { -2 , 1 , -3 , 4 , -1 , 2 , 1 , -5 , 4 };
 const collezioni::Sub s = collezioni::maxSum( E );
 CHECK( ( s.start == 3 ) && ( s.end == 6 ) && ( s.sum == 6 ) );
 std::mt19937 g( 129 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Ints A = drawInts( g , draw( g , 1 , 25 ) , -9 , t % 5 ? 9 : -1 );
  const collezioni::Sub a = collezioni::maxSumCubic( A ) ,
                        b = collezioni::maxSumQuadratic( A ) ,
                        c = collezioni::maxSum( A );
  CHECK( ( a.sum == b.sum ) && ( b.sum == c.sum ) );
  for( const auto & x : { a , b , c } )
   CHECK( ( 0 <= x.start ) && ( x.start <= x.end ) &&
          ( x.end < int( A.size() ) ) &&
          ( std::accumulate( A.begin() + x.start , A.begin() + x.end + 1 ,
                             0L ) == x.sum ) );
  const int top = *std::max_element( A.begin() , A.end() );
  CHECK( c.sum == ( top < 0 ? top : maxSubarraySum( A ) ) );
  }
 }

// collezione 12-divide, 1.10: both tests of majority against the counts
// of an unordered_map
ADS_TEST( "collezioni-12-divide-1-10" )
{
 CHECK( collezioni::majoritySort( Ints{ 4 } ) &&
        collezioni::majoritySelect( Ints{ 4 } ) );
 CHECK( ! collezioni::majoritySort( Ints{ 1 , 2 } ) &&
        ! collezioni::majoritySelect( Ints{ 1 , 2 } ) );
 CHECK( collezioni::majoritySelect( Ints{ 2 , 1 , 2 } ) );
 std::mt19937 g( 1210 );
 int yes = 0;
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints A = drawInts( g , draw( g , 1 , 30 ) , 0 , draw( g , 1 , 3 ) );
  std::unordered_map< int , int > f;
  int top = 0;
  for( int x : A ) top = std::max( top , ++f[ x ] );
  const bool maj = 2 * top > int( A.size() );
  CHECK( collezioni::majoritySort( A ) == maj );
  CHECK( collezioni::majoritySelect( A ) == maj );
  yes += maj;
  }
 CHECK( ( yes > 100 ) && ( yes < 900 ) );
 }

// collezione 12-divide, 1.11: the highest vertex of random convex polygons,
// points of an ellipse at random angles, against a linear scan
ADS_TEST( "collezioni-12-divide-1-11" )
{
 std::mt19937 g( 1211 );
 std::uniform_real_distribution< double > U( 0 , 2 * std::numbers::pi );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int n = draw( g , 3 , 40 );
  std::vector< double > a( n );
  for( double & x : a ) x = U( g );
  std::sort( a.begin() , a.end() );        // counterclockwise
  std::vector< std::pair< double , double > > V;
  for( double x : a )
   V.emplace_back( 3 * std::cos( x ) , 2 * std::sin( x ) + 0.1 * std::cos(
                   x ) );
  const auto low = std::min_element( V.begin() , V.end() , []( auto p ,
                     auto q ) { return( p.second < q.second ); } );
  std::rotate( V.begin() , low , V.end() );
  const auto high = std::max_element( V.begin() , V.end() , []( auto p ,
                      auto q ) { return( p.second < q.second ); } );
  CHECK( collezioni::highest( V ) == int( high - V.begin() ) );
  }
 const std::vector< std::pair< double , double > > T = { { 0 , 0 } ,
                                                       { 1 , 1 } ,
                                                       { -1 , 2 } };
 CHECK( collezioni::highest( T ) == 2 );
 }

// collezione 12-divide, 1.12: the place found has A[ i ] < A[ i + 1 ], on
// random vectors with A[ 0 ] < A[ n - 1 ]
ADS_TEST( "collezioni-12-divide-1-12" )
{
 CHECK( collezioni::findIncrease( Ints{ 1 , 2 } ) == 0 );
 std::mt19937 g( 1212 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  Ints A = drawInts( g , draw( g , 2 , 40 ) , 0 , t % 2 ? 5 : 100 );
  if( A.front() >= A.back() ) {
   A.front() = -1;
   A.back() = 101;
   }
  const int i = collezioni::findIncrease( A );
  CHECK( ( i >= 0 ) && ( i + 1 < int( A.size() ) ) &&
         ( A[ i ] < A[ i + 1 ] ) );
  }
 }

// collezione 12-divide, 1.13: the best gain against all the pairs x < y,
// negative when the prices only fall
ADS_TEST( "collezioni-12-divide-1-13" )
{
 CHECK( collezioni::bestGain( Ints{ 5 , 3 } ) == -2 );
 CHECK( collezioni::bestGain( Ints{ 3 , 5 } ) == 2 );
 CHECK( collezioni::bestGain( Ints{ 7 , 7 , 7 } ) == 0 );
 std::mt19937 g( 1213 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints P = drawInts( g , draw( g , 2 , 40 ) , 0 , 100 );
  long best = std::numeric_limits< long >::min();
  for( std::size_t x = 0 ; x < P.size() ; ++x )
   for( std::size_t y = x + 1 ; y < P.size() ; ++y )
    best = std::max( best , long( P[ y ] ) - P[ x ] );
  CHECK( collezioni::bestGain( P ) == best );
  }
 const int M = std::numeric_limits< int >::max();
 CHECK( collezioni::bestGain( Ints{ -M , M } ) == 2L * M );
 }

// collezione 12-divide, 1.14: the longest thin subvector against all the
// subvectors with their running minimum and maximum
ADS_TEST( "collezioni-12-divide-1-14" )
{
 CHECK( collezioni::thinSubarray( Ints{ 4 } , 0 ) == 1 );
 CHECK( collezioni::thinSubarray( Ints( 9 , 2 ) , 0 ) == 9 );
 CHECK( collezioni::thinSubarray( Ints{ 1 , 5 , 2 , 3 , 9 } , 2 ) == 2 );
 const int M = std::numeric_limits< int >::max();
 CHECK( collezioni::thinSubarray( Ints{ -M , M , M } , 0 ) == 2 );
 std::mt19937 g( 1214 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints V = drawInts( g , draw( g , 1 , 40 ) , 0 , 20 );
  const long C = draw( g , 0 , 8 );
  int best = 0;
  for( std::size_t i = 0 ; i < V.size() ; ++i ) {
   int lo = V[ i ] , hi = V[ i ];
   for( std::size_t j = i ; j < V.size() ; ++j ) {
    lo = std::min( lo , V[ j ] );
    hi = std::max( hi , V[ j ] );
    if( hi - lo <= C ) best = std::max( best , int( j - i + 1 ) );
    }
   }
  CHECK( collezioni::thinSubarray( V , C ) == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ COLLECTION 13, DYNAMIC PROGRAMMING --------------------*/
/*--------------------------------------------------------------------------*/

namespace collezioni {

/// no path, the -infinity of the longest paths
constexpr int NOPATH = std::numeric_limits< int >::min();

// collezione 13-pd, 1.1: k-occorrenza

/// an operation of an approximate occurrence: 'M' copies the character c
/// of T, 'S' substitutes the one of T with c of P, 'D' deletes c from T,
/// 'I' inserts c of P
struct EditOp {
 int kind , c;
 };

/// the walk back of printApproxRec() from DP[ i ][ h ]: the diagonal if
/// the table came from it, else the deletion of T[ h - 1 ], else the
/// insertion of P[ i - 1 ]; the operations go to out in order
void printApproxRec( const std::string & P , const std::string & T ,
                     const Table & DP , std::size_t i , std::size_t h ,
                     std::vector< EditOp > & out )
{
 if( i == 0 ) return;
 if( h > 0 ) {
  const int delta = ( P[ i - 1 ] == T[ h - 1 ] ) ? 0 : 1;
  if( DP[ i ][ h ] == DP[ i - 1 ][ h - 1 ] + delta ) {
   printApproxRec( P , T , DP , i - 1 , h - 1 , out );
   out.push_back( EditOp{ delta == 0 ? 'M' : 'S' ,
                          delta == 0 ? T[ h - 1 ] : P[ i - 1 ] } );
   return;
   }
  if( DP[ i ][ h ] == DP[ i ][ h - 1 ] + 1 ) {
   printApproxRec( P , T , DP , i , h - 1 , out );
   out.push_back( EditOp{ 'D' , T[ h - 1 ] } );
   return;
   }
  }
 printApproxRec( P , T , DP , i - 1 , h , out );
 out.push_back( EditOp{ 'I' , P[ i - 1 ] } );
 }

/// printApprox(): the operations of the occurrence of P with the fewest
/// errors that ends at T[ j ], from the table DP of the approximate
/// matching; O( m + n )
/// \pre DP is matchingTable( P , T ), j < n
std::vector< EditOp > printApprox( const std::string & P ,
                                   const std::string & T , const Table & DP ,
                                   std::size_t j )
{
 ADS_PRE( ( j < T.size() ) && ( DP.size() == P.size() + 1 ) );
 std::vector< EditOp > out;
 out.reserve( P.size() + j + 1 );          // at most m + j + 1 operations
 printApproxRec( P , T , DP , P.size() , j + 1 , out );
 return( out );
 }

// collezione 13-pd, 1.2: Algoritmo di Bellman, Ford e Moore

/// shortestPathDP(): the parents and the distances from r (LINF if not
/// reachable) by the recurrence on the number of edges: the round k reads
/// the distances of the round k - 1 and writes a copy of them; O( n m )
/// time, O( n ) space
/// \pre r is a node, no negative cycle reachable from r
std::pair< Ints , Longs > shortestPathDP( const WGraph & g , int r )
{
 ADS_PRE( g.hasNode( r ) );
 Longs d( g.bound() , LINF );
 Ints T( g.bound() , NIL );
 d[ r ] = 0;
 for( int k = 1 ; k < g.n() ; ++k ) {
  Longs next = d;
  for( int u : g.V() )
   if( d[ u ] < LINF )
    g.forEachEdge( u , [ & ]( int v , int w ) {
     if( d[ u ] + w < next[ v ] ) {
      next[ v ] = d[ u ] + w;
      T[ v ] = u;
      }
     } );
  d = std::move( next );
  }
 return( std::pair( T , d ) );
 }

// collezione 13-pd, 1.3: Cammini minimi su grafi aciclici

/// shortestPathDAG(): the parents and the distances from r of a graph
/// whose edges ( u , v ) have u < v, the order of the names being
/// topological; O( n + m )
/// \pre r is a node, each edge goes to a larger name
std::pair< Ints , Longs > shortestPathDAG( const WGraph & g , int r )
{
 ADS_PRE( g.hasNode( r ) );
 Longs d( g.bound() , LINF );
 Ints T( g.bound() , NIL );
 d[ r ] = 0;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , int w ) {
   ADS_PRE( u < v );
   if( ( d[ u ] < LINF ) && ( d[ u ] + w < d[ v ] ) ) {
    d[ v ] = d[ u ] + w;
    T[ v ] = u;
    }
   } );
 return( std::pair( T , d ) );
 }

// collezione 13-pd, 1.4 and 1.5: Cammini massimi in DAG

/// longestPaths(): the largest weight of a path from s to each node of an
/// acyclic graph (NOPATH if not reachable), the weight of an edge 1 if
/// unit, else its own: the nodes in topological order, each relaxing
/// d[ v ] = max( d[ v ] , d[ u ] + w ); O( n + m )
/// \pre s is a node, the graph acyclic
Ints longestPaths( const WGraph & g , int s , bool unit )
{
 ADS_PRE( g.hasNode( s ) );
 Ints d( g.bound() , NOPATH );
 d[ s ] = 0;
 for( int u : topsort( g ) )
  if( d[ u ] != NOPATH )
   g.forEachEdge( u , [ & ]( int v , int w ) {
    d[ v ] = std::max( d[ v ] , d[ u ] + ( unit ? 1 : w ) );
    } );
 return( d );
 }

// collezione 13-pd, 1.6: Cammini minimi dinamici

/// insertEdgeAPSP(): the distances D of all the pairs (LINF if none) after
/// the insertion of the edge ( u , v ) of weight w: a new shortest path
/// from x to y is x ~> u -> v ~> y, so D[ x ][ y ] = min( D[ x ][ y ] ,
/// D[ x ][ u ] + w + D[ v ][ y ] ), with the column of u and the row of v
/// copied first; Theta( n^2 ) time, Theta( n ) space. Throws
/// std::domain_error if the edge closes a negative cycle. A deletion has
/// no such update: D does not say which pairs used the edge, nor their
/// second best paths
/// \pre u != v are places of D
void insertEdgeAPSP( std::vector< Longs > & D , int u , int v , long w )
{
 const int n = int( D.size() );
 ADS_PRE( ( 0 <= u ) && ( u < n ) && ( 0 <= v ) && ( v < n ) && ( u != v ) );
 if( ( D[ v ][ u ] < LINF ) && ( D[ v ][ u ] + w < 0 ) )
  throw( std::domain_error( "insertEdgeAPSP: a negative cycle" ) );
 Longs toU( n ) , fromV = D[ v ];
 for( int x = 0 ; x < n ; ++x ) toU[ x ] = D[ x ][ u ];
 for( int x = 0 ; x < n ; ++x )
  if( toU[ x ] < LINF )
   for( int y = 0 ; y < n ; ++y )
    if( fromV[ y ] < LINF )
     D[ x ][ y ] = std::min( D[ x ][ y ] , toU[ x ] + w + fromV[ y ] );
 }

// collezione 13-pd, 1.9: Massima somma di sottovettori in tempo lineare

/// maxSumLinear(): the non-empty subvector of largest sum and its places,
/// by Kadane: the best sum ending at i is A[ i ] or the best one ending at
/// i - 1 plus A[ i ]; Theta( n ) time, Theta( 1 ) space
/// \pre A is not empty
Sub maxSumLinear( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 Sub best{ 0 , 0 , A[ 0 ] };
 long here = A[ 0 ];
 int start = 0;
 for( int i = 1 ; i < int( A.size() ) ; ++i ) {
  if( A[ i ] > here + A[ i ] ) {
   here = A[ i ];
   start = i;
   }
  else here += A[ i ];
  if( here > best.sum ) best = Sub{ start , i , here };
  }
 return( best );
 }

// collezione 13-pd, 1.11: Mosse su scacchiera

/// searchPath(): the largest profit of a pawn going from the bottom row
/// to the top one, up, up-left or up-right: DP[ r ][ c ] = P[ r ][ c ] +
/// the best of the three cells below, two rows kept; Theta( n^2 ) time,
/// Theta( n ) space
/// \pre P is square and not empty
long searchPath( const std::vector< Ints > & P )
{
 const int n = int( P.size() );
 ADS_PRE( n > 0 );
 Longs DP( P[ 0 ].begin() , P[ 0 ].end() ) , next( n );
 for( int r = 1 ; r < n ; ++r ) {
  for( int c = 0 ; c < n ; ++c ) {
   long b = DP[ c ];
   if( c > 0 ) b = std::max( b , DP[ c - 1 ] );
   if( c + 1 < n ) b = std::max( b , DP[ c + 1 ] );
   next[ c ] = P[ r ][ c ] + b;
   }
  std::swap( DP , next );
  }
 return( *std::max_element( DP.begin() , DP.end() ) );
 }

// collezione 13-pd, 1.12: Batterie

/// minCostStops(): the least cost of the batteries to reach the exit
/// dist[ n ] (whose cost is 0), LINF if impossible: DP[ i ] = cost[ i ] if
/// dist[ i ] <= R, else cost[ i ] plus the least DP[ j ] of a j < i within
/// R; O( n^2 ) time, O( n ) space. The text sets DP[ i ] = +inf after the
/// direct case, losing it; here +inf is only the start of the other case
/// \pre dist strictly increasing and positive, |cost| = |dist|
long minCostStops( const Longs & dist , const Longs & cost , long R )
{
 ADS_PRE( ! dist.empty() && ( dist.size() == cost.size() ) );
 const int n = int( dist.size() ) - 1;
 Longs DP( n + 1 , LINF );
 for( int i = 0 ; i <= n ; ++i ) {
  if( dist[ i ] <= R ) {
   DP[ i ] = cost[ i ];
   continue;
   }
  for( int j = i - 1 ; ( j >= 0 ) && ( dist[ i ] - dist[ j ] <= R ) ; --j )
   if( DP[ j ] < LINF ) DP[ i ] = std::min( DP[ i ] , DP[ j ] + cost[ i ] );
  }
 return( DP[ n ] );
 }

// collezione 13-pd, 1.13: Come passare l'esame!

/// lazyStudent(): the problems that reach at least V points in the least
/// time, and that time (LINF if impossible): DP[ i ][ v ], the least time
/// to get v points from the first i problems, is the least of DP[ i - 1 ][
/// v ] and time[ i - 1 ] + DP[ i - 1 ][ max( 0 , v - score[ i - 1 ] ) ];
/// the set walks the table back; Theta( n V )
/// \pre |score| = |time|, scores and times not negative, V >= 0
std::pair< Ints , long > lazyStudent( const Ints & score , const Ints & time ,
                                      int V )
{
 ADS_PRE( ( score.size() == time.size() ) && ( V >= 0 ) );
 const int n = int( score.size() );
 std::vector< Longs > DP( n + 1 , Longs( V + 1 , LINF ) );
 for( int i = 0 ; i <= n ; ++i ) DP[ i ][ 0 ] = 0;
 for( int i = 1 ; i <= n ; ++i )
  for( int v = 1 ; v <= V ; ++v ) {
   const long prev = DP[ i - 1 ][ std::max( 0 , v - score[ i - 1 ] ) ];
   DP[ i ][ v ] = std::min( DP[ i - 1 ][ v ] ,
                            prev < LINF ? time[ i - 1 ] + prev : LINF );
   }
 Ints S;
 if( DP[ n ][ V ] < LINF )
  for( int i = n , v = V ; ( i > 0 ) && ( v > 0 ) ; --i )
   if( DP[ i ][ v ] != DP[ i - 1 ][ v ] ) {
    S.push_back( i - 1 );
    v = std::max( 0 , v - score[ i - 1 ] );
    }
 return( std::pair( S , DP[ n ][ V ] ) );
 }

/// lazyFractional(): the least time to reach V points when a problem done
/// in part gives a part of its points: by increasing time / score, the
/// problems are done in full and the last one in the part needed, as in
/// the fractional knapsack; O( n log n ); infinity if the points are not
/// enough
/// \pre |score| = |time|, scores positive, times not negative, V >= 0
double lazyFractional( const Ints & score , const Ints & time , int V )
{
 ADS_PRE( score.size() == time.size() );
 Ints by( score.size() );
 std::iota( by.begin() , by.end() , 0 );
 std::sort( by.begin() , by.end() , [ & ]( int a , int b ) {
  return( long( time[ a ] ) * score[ b ] < long( time[ b ] ) * score[ a ] );
  } );
 double need = V , t = 0;
 for( int i : by ) {
  if( need <= 0 ) break;
  const double f = std::min( 1.0 , need / score[ i ] );
  t += f * time[ i ];
  need -= f * score[ i ];
  }
 return( need > 1e-9 ? std::numeric_limits< double >::infinity() : t );
 }

// collezione 13-pd, 1.14: Insieme indipendente di intervalli pesati

/// computePredecessor(): pred[ i ], 1 <= i <= n, the largest j < i with
/// f[ j - 1 ] <= s[ i - 1 ], 0 if none, for intervals sorted by f; a binary
/// search for each, which goes on to the right after a success so that
/// equal ends give the last one; O( n log n )
/// \pre |s| = |f|, f not decreasing
Ints computePredecessor( const Ints & s , const Ints & f )
{
 ADS_PRE( ( s.size() == f.size() ) &&
          std::is_sorted( f.begin() , f.end() ) );
 const int n = int( s.size() );
 Ints pred( n + 1 , 0 );
 for( int i = 1 ; i <= n ; ++i ) {
  int left = 0 , right = i - 2;
  while( left <= right ) {
   const int mid = ( left + right ) / 2;
   if( f[ mid ] <= s[ i - 1 ] ) {
    pred[ i ] = mid + 1;
    left = mid + 1;
    }
   else right = mid - 1;
   }
  }
 return( pred );
 }

// collezione 13-pd, 1.15: Palinsesto

/// schedule(): the songs of total duration at most N of largest total
/// rating, and that rating, the 0-1 knapsack: DP[ i ][ t ] is the best
/// rating of the first i songs in t minutes; Theta( n N )
/// \pre |g| = |d|, d >= 0, N >= 0
std::pair< Ints , double > schedule( const std::vector< double > & g ,
                                     const Ints & d , int N )
{
 ADS_PRE( ( g.size() == d.size() ) && ( N >= 0 ) );
 const int n = int( g.size() );
 std::vector< std::vector< double > > DP( n + 1 ,
                                         std::vector< double >( N + 1 , 0 ) );
 for( int i = 1 ; i <= n ; ++i )
  for( int t = 0 ; t <= N ; ++t ) {
   DP[ i ][ t ] = DP[ i - 1 ][ t ];
   if( d[ i - 1 ] <= t )
    DP[ i ][ t ] = std::max( DP[ i ][ t ] ,
                             DP[ i - 1 ][ t - d[ i - 1 ] ] + g[ i - 1 ] );
   }
 Ints S;
 for( int i = n , t = N ; i > 0 ; --i )
  if( DP[ i ][ t ] != DP[ i - 1 ][ t ] ) {
   S.push_back( i - 1 );
   t -= d[ i - 1 ];
   }
 return( std::pair( S , DP[ n ][ N ] ) );
 }

// collezione 13-pd, 1.17: LCS di lunghezza k

/// lcsAtLeast(): whether X and Y have a common subsequence of length at
/// least k, by the recurrence of the LCS on two rows; Theta( n m ) time
bool lcsAtLeast( const std::string & X , const std::string & Y , int k )
{
 Ints prev( Y.size() + 1 , 0 ) , cur( Y.size() + 1 , 0 );
 for( std::size_t i = 1 ; i <= X.size() ; ++i ) {
  for( std::size_t j = 1 ; j <= Y.size() ; ++j )
   cur[ j ] = ( X[ i - 1 ] == Y[ j - 1 ] ) ? prev[ j - 1 ] + 1
                                           : std::max( prev[ j ] ,
                                                       cur[ j - 1 ] );
  std::swap( prev , cur );
  }
 return( prev[ Y.size() ] >= k );
 }

// collezione 13-pd, 1.18: Stringhe palindrome

/// the least insertions that make s[ i .. j ] a palindrome, memoized in DP
int minpalRec( const std::string & s , int i , int j ,
               std::vector< Ints > & DP )
{
 if( i >= j ) return( 0 );
 int & r = DP[ i ][ j ];
 if( r < 0 )
  r = ( s[ i ] == s[ j ] ) ? minpalRec( s , i + 1 , j - 1 , DP )
                           : 1 + std::min( minpalRec( s , i + 1 , j , DP ) ,
                                           minpalRec( s , i , j - 1 , DP ) );
 return( r );
 }

/// minpal(): the least characters to insert into s to make it a
/// palindrome: 0 on a part of at most one character, that of the inner
/// part if the ends are equal, else one more than the best of dropping
/// either end; Theta( n^2 ) time and space
int minpal( const std::string & s )
{
 const int n = int( s.size() );
 std::vector< Ints > DP( n , Ints( n , -1 ) );
 return( minpalRec( s , 0 , n - 1 , DP ) );
 }

// collezione 13-pd, 1.19: Abbasso Trenitalia!

/// railway(): the least cost of the n trips, each by its fare or by a pass
/// of cost X that covers 30 days from the day it is bought, and the trips
/// on whose days the passes are bought: next[ i ] is the first trip not
/// covered by a pass bought on d[ i ], and DP[ i ] = min( f[ i ] + DP[ i +
/// 1 ] , X + DP[ next[ i ] ] ); O( n )
/// \pre d not decreasing, |d| = |f|, n >= 1
std::pair< long , Ints > railway( const Ints & d , const Ints & f , long X )
{
 const int n = int( d.size() );
 ADS_PRE( ( n >= 1 ) && ( f.size() == d.size() ) &&
          std::is_sorted( d.begin() , d.end() ) );
 Ints next( n );
 for( int i = 0 , j = 0 ; i < n ; ++i ) {
  j = std::max( j , i + 1 );
  while( ( j < n ) && ( d[ j ] < d[ i ] + 30 ) ) ++j;
  next[ i ] = j;
  }
 Longs DP( n + 1 , 0 );
 std::vector< bool > pass( n );
 for( int i = n - 1 ; i >= 0 ; --i ) {
  pass[ i ] = ( X + DP[ next[ i ] ] <= f[ i ] + DP[ i + 1 ] );
  DP[ i ] = pass[ i ] ? X + DP[ next[ i ] ] : f[ i ] + DP[ i + 1 ];
  }
 Ints buy;
 for( int i = 0 ; i < n ; )
  if( pass[ i ] ) {
   buy.push_back( i );
   i = next[ i ];
   }
  else ++i;
 return( std::pair( DP[ 0 ] , buy ) );
 }

// collezione 13-pd, 1.20: Data center

/// datacenter(): the most data processed in n days, x[ i ] arriving on the
/// day i, s[ a ] the capacity a days after a reboot, which takes a day:
/// DP[ i ][ a ] = max( DP[ i + 1 ][ 0 ] , min( x[ i ] , s[ a ] ) + DP[ i +
/// 1 ][ a + 1 ] ), DP[ n ][ a ] = 0; Theta( n^2 ) time and space
/// \pre |x| = |s| = n
long datacenter( const Ints & x , const Ints & s )
{
 ADS_PRE( x.size() == s.size() );
 const int n = int( x.size() );
 std::vector< Longs > DP( n + 1 , Longs( n + 1 , 0 ) );
 for( int i = n - 1 ; i >= 0 ; --i )
  for( int a = 0 ; a <= i ; ++a )
   DP[ i ][ a ] = std::max( DP[ i + 1 ][ 0 ] , std::min( x[ i ] , s[ a ] ) +
                                              DP[ i + 1 ][ a + 1 ] );
 return( DP[ 0 ][ 0 ] );
 }

// collezione 13-pd, 1.21: Stringhe primitive

/// countWays(): the number of ways to write X as a concatenation of
/// strings of S: DP[ i ], the ways of the suffix from i, is the sum of
/// DP[ i + |s| ] over the strings s of S that occur at i, DP[ n ] = 1;
/// O( n sum |s| ) time, O( n ) space
/// \pre the strings of S are not empty
std::uint64_t countWays( const std::string & X ,
                         const std::vector< std::string > & S )
{
 const std::size_t n = X.size();
 std::vector< std::uint64_t > DP( n + 1 , 0 );
 DP[ n ] = 1;
 for( std::size_t i = n ; i-- > 0 ; )
  for( const std::string & s : S ) {
   ADS_PRE( ! s.empty() );
   if( ( i + s.size() <= n ) && ( X.compare( i , s.size() , s ) == 0 ) )
    DP[ i ] += DP[ i + s.size() ];
   }
 return( DP[ 0 ] );
 }

// collezione 13-pd, 1.22: Alberi pieni

/// fullTrees(): the number of full binary trees of n nodes: 0 if n is
/// even, 1 if n = 1, else the sum over the odd sizes j of the left subtree
/// of T[ j ] T[ n - 1 - j ]; Theta( n^2 ) time, Theta( n ) space
/// \pre 0 < n <= 69, so that the result fits in 64 bits
std::uint64_t fullTrees( int n )
{
 ADS_PRE( ( n > 0 ) && ( n <= 69 ) );
 std::vector< std::uint64_t > T( n + 1 , 0 );
 T[ 1 ] = 1;
 for( int k = 3 ; k <= n ; k += 2 )
  for( int j = 1 ; j <= k - 2 ; j += 2 ) T[ k ] += T[ j ] * T[ k - 1 - j ];
 return( T[ n ] );
 }

}  // end( namespace collezioni )

namespace {

/// a random acyclic graph: the edges ( u , v ) with u < v, each with
/// probability p percent, of weight in [ lo , hi ]; the names permuted if
/// shuffle
WGraph randomDag( std::mt19937 & g , int n , int p , int lo , int hi ,
                  bool shuffle )
{
 Ints name( n );
 std::iota( name.begin() , name.end() , 0 );
 if( shuffle ) std::shuffle( name.begin() , name.end() , g );
 WGraph G( n );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( draw( g , 1 , 100 ) <= p )
    G.insertEdge( name[ u ] , name[ v ] , draw( g , lo , hi ) );
 return( G );
 }

/// the largest weight of a path from s to each node, by enumeration
Ints longestBrute( const WGraph & G , int s , bool unit )
{
 Ints d( G.bound() , collezioni::NOPATH );
 std::function< void( int , int ) > rec = [ & ]( int u , int w ) {
  d[ u ] = std::max( d[ u ] , w );
  G.forEachEdge( u , [ & ]( int v , int c ) {
   rec( v , w + ( unit ? 1 : c ) ); } );
  };
 rec( s , 0 );
 return( d );
 }

/// a random string of n characters among the first k letters
std::string drawString( std::mt19937 & g , int n , int k )
{
 std::string s( n , 'a' );
 for( char & c : s ) c = char( 'a' + draw( g , 0 , k - 1 ) );
 return( s );
 }

/// the distances as long, LINF for infinity< int >()
Longs widen( const std::vector< int > & d )
{
 Longs r( d.size() );
 for( std::size_t i = 0 ; i < d.size() ; ++i )
  r[ i ] = ( d[ i ] == infinity< int >() ) ? LINF : d[ i ];
 return( r );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

// collezione 13-pd, 1.1: the operations rebuild P from the part of T they
// read, their errors are the entry of the table, and that part is at edit
// distance DP[ m ][ j + 1 ] from P
ADS_TEST( "collezioni-13-pd-1-1" )
{
 const std::string P = "ab" , T = "xaxb";
 const auto ops = collezioni::printApprox( P , T , matchingTable( P , T ) ,
                                           3 );
 CHECK( ( ops.size() == 2 ) && ( ops[ 0 ].kind == 'S' ) &&
        ( ops[ 0 ].c == 'a' ) && ( ops[ 1 ].kind == 'M' ) );   // in "xb"
 std::mt19937 g( 131 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string P = drawString( g , draw( g , 1 , 6 ) , 3 ) ,
                    T = drawString( g , draw( g , 1 , 12 ) , 3 );
  const Table DP = matchingTable( P , T );
  for( std::size_t j = 0 ; j < T.size() ; ++j ) {
   std::string out;
   std::size_t read = 0;
   int errors = 0;
   for( const auto & o : collezioni::printApprox( P , T , DP , j ) ) {
    if( o.kind != 'I' ) ++read;
    if( o.kind != 'D' ) out += char( o.c );
    if( o.kind != 'M' ) ++errors;
    }
   CHECK( ( out == P ) && ( read <= j + 1 ) );
   CHECK( errors == DP[ P.size() ][ j + 1 ] );
   const std::size_t start = j + 1 - read;
   CHECK( editDistance( P , T.substr( start , read ) ) == errors );
   const std::size_t s = occurrence( DP , P , T , j + 1 );
   CHECK( editDistance( P , T.substr( s , j + 1 - s ) ) == errors );
   }
  }
 }

// collezione 13-pd, 1.2: the rounds with a copy against Bellman-Ford-Moore
// of ADS, with weights of both signs and no negative cycle from r; the
// parents form a tree of shortest paths
ADS_TEST( "collezioni-13-pd-1-2" )
{
 std::mt19937 g( 132 );
 int tried = 0;
 for( int t = 0 ; t < 600 ; ++t ) {
  const WGraph G = randomGraph( g , draw( g , 1 , 8 ) , 35 , -3 , 9 );
  const int r = draw( g , 0 , G.n() - 1 );
  if( reachesNegativeCycle( G , r ) ) continue;
  ++tried;
  const auto [ T , d ] = collezioni::shortestPathDP( G , r );
  const auto R = bellmanFordMoore( G , r );
  CHECK( d == widen( R.d ) );
  ShortestPathTree< int > S;
  S.T = T;
  for( long x : d ) S.d.push_back( x == LINF ? infinity< int >() : int( x ) );
  CHECK( isShortestPathTree( G , r , S ) );
  }
 CHECK( tried > 300 );
 const auto [ T , d ] = collezioni::shortestPathDP( WGraph( 1 ) , 0 );
 CHECK( ( T == Ints{ NIL } ) && ( d == Longs{ 0 } ) );
 }

// collezione 13-pd, 1.3: the numeric order as topological order, against
// dagShortestPaths() of ADS, with negative weights
ADS_TEST( "collezioni-13-pd-1-3" )
{
 std::mt19937 g( 133 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const WGraph G = randomDag( g , draw( g , 1 , 10 ) , 40 , -9 , 9 , false );
  const int r = draw( g , 0 , G.n() - 1 );
  const auto [ T , d ] = collezioni::shortestPathDAG( G , r );
  CHECK( d == widen( dagShortestPaths( G , r ).d ) );
  for( int v : G.V() )
   if( ( v != r ) && ( d[ v ] < LINF ) )
    CHECK( d[ v ] == d[ T[ v ] ] + *G.weight( T[ v ] , v ) );
  }
 }

// collezione 13-pd, 1.4: the most edges of a path from s to each node,
// against longestPathDag() of ADS and the enumeration of the paths
ADS_TEST( "collezioni-13-pd-1-4" )
{
 std::mt19937 g( 134 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const WGraph G = randomDag( g , draw( g , 1 , 9 ) , 35 , 1 , 1 , true );
  const int n = G.n() , s = draw( g , 0 , n - 1 );
  const Ints d = collezioni::longestPaths( G , s , true );
  CHECK( d == longestBrute( G , s , true ) );
  Adjacency A( n , std::vector< bool >( n ) );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) ) A[ u ][ v ] = true;
  for( int v = 0 ; v < n ; ++v )
   CHECK( longestPathDag( A , s , v ) ==
          ( d[ v ] == collezioni::NOPATH ? -1 : d[ v ] ) );
  }
 }

// collezione 13-pd, 1.5: the heaviest path, against dagShortestPaths() of
// ADS on the weights negated and the enumeration of the paths
ADS_TEST( "collezioni-13-pd-1-5" )
{
 std::mt19937 g( 135 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const WGraph G = randomDag( g , draw( g , 1 , 9 ) , 35 , -9 , 9 , true );
  const int n = G.n() , s = draw( g , 0 , n - 1 );
  const Ints d = collezioni::longestPaths( G , s , false );
  CHECK( d == longestBrute( G , s , false ) );
  WGraph N( n );
  for( int u = 0 ; u < n ; ++u )
   G.forEachEdge( u , [ & ]( int v , int w ) { N.insertEdge( u , v , -w ); } );
  const auto R = dagShortestPaths( N , s );
  for( int v = 0 ; v < n ; ++v )
   CHECK( ( d[ v ] == collezioni::NOPATH ) ? ( R.d[ v ] == infinity< int >() )
                                           : ( R.d[ v ] == -d[ v ] ) );
  }
 }

// collezione 13-pd, 1.6: after the insertion of a new edge the distances
// are those of Floyd-Warshall on the new graph, as with addEdge() of ADS;
// an edge that closes a negative cycle throws, and Floyd-Warshall finds
// the cycle
ADS_TEST( "collezioni-13-pd-1-6" )
{
 const auto table = []( const WGraph & G ) {
  const auto R = floydWarshall( G );
  std::vector< Longs > D;
  for( const auto & row : R.d ) D.push_back( widen( row ) );
  return( D );
  };
 std::mt19937 g( 136 );
 int thrown = 0;
 for( int t = 0 ; t < 600 ; ++t ) {
  WGraph G = randomGraph( g , draw( g , 2 , 7 ) , 30 , -2 , 9 );
  auto R = floydWarshall( G );
  if( R.negativeCycle ) continue;
  std::vector< Longs > D = table( G );
  const int u = draw( g , 0 , G.n() - 1 ) , v = draw( g , 0 , G.n() - 1 );
  if( ( u == v ) || G.hasEdge( u , v ) ) continue;
  const int w = draw( g , -8 , 8 );
  G.insertEdge( u , v , w );
  try {
   collezioni::insertEdgeAPSP( D , u , v , w );
   }
  catch( const std::domain_error & ) {
   CHECK( floydWarshall( G ).negativeCycle );
   ++thrown;
   continue;
   }
  CHECK( ! floydWarshall( G ).negativeCycle && ( D == table( G ) ) );
  addEdge( R , u , v , w );
  for( int x = 0 ; x < G.n() ; ++x ) CHECK( widen( R.d[ x ] ) == D[ x ] );
  }
 CHECK( thrown > 10 );
 }

// collezione 13-pd, 1.7: lis() of ADS on the example of the text, whose
// longest increasing subsequences have 4 items, and against the
// enumeration of the subsequences
ADS_TEST( "collezioni-13-pd-1-7" )
{
 const Ints V = { 9 , 15 , 3 , 6 , 4 , 2 , 5 , 10 , 3 };
 const Ints L = lis( V );
 CHECK( ( L.size() == 4 ) && ( L.back() == 10 ) );
 CHECK( lis( Ints{ 3 , 4 , 5 , 10 } ) == Ints( { 3 , 4 , 5 , 10 } ) );
 CHECK( lis( Ints{ 7 , 7 , 7 } ).size() == 1 );
 std::mt19937 g( 137 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Ints A = drawInts( g , draw( g , 1 , 12 ) , 0 , 9 );
  const int n = int( A.size() );
  std::size_t best = 0;
  for( int mask = 1 ; mask < ( 1 << n ) ; ++mask ) {
   Ints S;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) S.push_back( A[ i ] );
   if( std::adjacent_find( S.begin() , S.end() , std::greater_equal<>() ) ==
       S.end() )
    best = std::max( best , S.size() );
   }
  const Ints R = lis( A );
  CHECK( R.size() == best );
  std::size_t k = 0;                       // R is a subsequence of A
  for( int x : A )
   if( ( k < R.size() ) && ( R[ k ] == x ) ) ++k;
  CHECK( k == R.size() );
  }
 }

// collezione 13-pd, 1.8: the requests of Harry Potter by maxinterval() of
// ADS, against all the sets of requests
ADS_TEST( "collezioni-13-pd-1-8" )
{
 using I = WeightedInterval< long >;
 const auto [ S , w ] = maxinterval( std::vector< I >{ { 0 , 3 , 5 } ,
                                                      { 3 , 6 , 5 } ,
                                                      { 1 , 5 , 9 } } );
 CHECK( ( w == 10 ) && ( S.size() == 2 ) );   // half-open: [0,3) , [3,6)
 std::mt19937 g( 138 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< I > R( draw( g , 0 , 11 ) );
  for( I & x : R ) {
   x.a = draw( g , 0 , 15 );
   x.b = x.a + draw( g , 1 , 6 );
   x.w = draw( g , 0 , 9 );
   }
  const int n = int( R.size() );
  long best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   long s = 0;
   bool ok = true;
   for( int i = 0 ; i < n ; ++i )
    for( int j = i + 1 ; j < n ; ++j )
     if( ( mask >> i & 1 ) && ( mask >> j & 1 ) &&
         ( R[ i ].a < R[ j ].b ) && ( R[ j ].a < R[ i ].b ) )
      ok = false;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) s += R[ i ].w;
   if( ok ) best = std::max( best , s );
   }
  const auto [ P , v ] = maxinterval( R );
  long s = 0;
  for( std::size_t i = 0 ; i < P.size() ; ++i ) {
   s += R[ P[ i ] ].w;
   for( std::size_t j = i + 1 ; j < P.size() ; ++j )
    CHECK( ( R[ P[ i ] ].b <= R[ P[ j ] ].a ) ||
           ( R[ P[ j ] ].b <= R[ P[ i ] ].a ) );
   }
  CHECK( ( v == best ) && ( s == best ) );
  }
 }

// collezione 13-pd, 1.9: Kadane with the places, against all the
// subvectors
ADS_TEST( "collezioni-13-pd-1-9" )
{
 CHECK( collezioni::maxSumLinear( Ints{ -3 } ).sum == -3 );
 const collezioni::Sub e = collezioni::maxSumLinear( Ints{ -1 , 3 , -1 ,
                                                           2 , -9 } );
 CHECK( ( e.start == 1 ) && ( e.end == 3 ) && ( e.sum == 4 ) );
 std::mt19937 g( 139 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints A = drawInts( g , draw( g , 1 , 30 ) , -9 , t % 4 ? 9 : -1 );
  const collezioni::Sub s = collezioni::maxSumLinear( A );
  CHECK( s.sum == collezioni::maxSumQuadratic( A ).sum );
  CHECK( ( s.start <= s.end ) &&
         ( std::accumulate( A.begin() + s.start , A.begin() + s.end + 1 ,
                            0L ) == s.sum ) );
  }
 }

// collezione 13-pd, 1.10: wordBreak() of ADS on the example of the text
// and against a recursive enumeration; the lookups are at most
// n ( n + 1 ) / 2
ADS_TEST( "collezioni-13-pd-1-10" )
{
 const std::set< std::string > it = { "era" , "una" , "notte" , "buia" ,
                                      "e" , "tempestosa" , "not" , "te" };
 const auto in = [ & ]( const std::string & w ) {
  return( it.count( w ) > 0 ); };
 const std::string s = "eraunanottebuiaetempestosa";
 long lookups = 0;
 const auto W = wordBreak( s , in , std::string::npos , &lookups );
 CHECK( W && ( std::accumulate( W->begin() , W->end() , std::string() ) ==
               s ) );
 for( const auto & w : *W ) CHECK( in( w ) );
 CHECK( lookups <= long( s.size() * ( s.size() + 1 ) / 2 ) );
 CHECK( ! wordBreak( std::string( "eraunanottx" ) , in ) );
 CHECK( wordBreak( std::string() , in ) );
 std::mt19937 g( 1310 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::set< std::string > D;
  for( int k = draw( g , 1 , 4 ) ; k > 0 ; --k )
   D.insert( drawString( g , draw( g , 1 , 3 ) , 2 ) );
  const std::string x = drawString( g , draw( g , 0 , 12 ) , 2 );
  std::function< bool( std::size_t ) > can = [ & ]( std::size_t i ) {
   if( i == x.size() ) return( true );
   for( const auto & w : D )
    if( ( x.compare( i , w.size() , w ) == 0 ) && can( i + w.size() ) )
     return( true );
   return( false );
   };
  const auto R = wordBreak( x , [ & ]( const std::string & w ) {
   return( D.count( w ) > 0 ); } );
  CHECK( bool( R ) == can( 0 ) );
  }
 }

// collezione 13-pd, 1.11: the example of the text, 33, and the
// enumeration of all the paths on small boards
ADS_TEST( "collezioni-13-pd-1-11" )
{
 const std::vector< Ints > P = { { 6 , 7 , 4 , 7 , 8 } ,
                                 { 7 , 6 , 1 , 1 , 4 } ,
                                 { 3 , 5 , 7 , 8 , 2 } ,
                                 { 2 , 6 , 7 , 0 , 2 } ,
                                 { 7 , 3 , 5 , 6 , 1 } };
 CHECK( collezioni::searchPath( P ) == 33 );
 CHECK( collezioni::searchPath( { { 4 } } ) == 4 );
 std::mt19937 g( 1311 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = draw( g , 1 , 6 );
  std::vector< Ints > Q( n );
  for( Ints & r : Q ) r = drawInts( g , n , -5 , 9 );
  long best = std::numeric_limits< long >::min();
  std::function< void( int , int , long ) > rec = [ & ]( int r , int c ,
                                                         long s ) {
   s += Q[ r ][ c ];
   if( r == n - 1 ) { best = std::max( best , s ); return; }
   for( int dc = -1 ; dc <= 1 ; ++dc )
    if( ( c + dc >= 0 ) && ( c + dc < n ) ) rec( r + 1 , c + dc , s );
   };
  for( int c = 0 ; c < n ; ++c ) rec( 0 , c , 0 );
  CHECK( collezioni::searchPath( Q ) == best );
  }
 }

// collezione 13-pd, 1.12: the cost of the stops against all the sets of
// stations whose gaps are at most R
ADS_TEST( "collezioni-13-pd-1-12" )
{
 CHECK( collezioni::minCostStops( { 5 } , { 0 } , 5 ) == 0 );
 CHECK( collezioni::minCostStops( { 6 } , { 0 } , 5 ) == LINF );
 CHECK( collezioni::minCostStops( { 3 , 6 } , { 4 , 0 } , 5 ) == 4 );
 std::mt19937 g( 1312 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = draw( g , 0 , 10 );
  Longs dist , cost;
  for( long x = 0 ; int( dist.size() ) <= n ; ) {
   x += draw( g , 1 , 6 );
   dist.push_back( x );
   cost.push_back( draw( g , 0 , 9 ) );
   }
  cost[ n ] = 0;
  const long R = draw( g , 3 , 12 );
  long best = LINF;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   long last = 0 , c = 0;
   bool ok = true;
   for( int i = 0 ; i <= n ; ++i )
    if( ( i == n ) || ( mask >> i & 1 ) ) {
     ok = ok && ( dist[ i ] - last <= R );
     last = dist[ i ];
     c += cost[ i ];
     }
   if( ok ) best = std::min( best , c );
   }
  CHECK( collezioni::minCostStops( dist , cost , R ) == best );
  }
 }

// collezione 13-pd, 1.13/2: the least time against all the sets of
// problems, the set found reaches V in that time, and the time is the
// total minus the best knapsack() of ADS of the problems left out, of
// capacity the points that can be given up (on the instances with no
// score 0: knapsack() never takes an object of volume 0)
ADS_TEST( "collezioni-13-pd-1-13-2" )
{
 CHECK( collezioni::lazyStudent( { 5 , 4 , 3 } , { 10 , 3 , 3 } , 6 ) ==
        std::pair( Ints{ 2 , 1 } , 6L ) );
 CHECK( collezioni::lazyStudent( { 1 } , { 1 } , 2 ).second == LINF );
 CHECK( collezioni::lazyStudent( {} , {} , 0 ).second == 0 );
 std::mt19937 g( 1313 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = draw( g , 0 , 12 );
  const Ints score = drawInts( g , n , 0 , 8 ) , time = drawInts( g , n , 0 ,
                                                                    20 );
  const int S = std::accumulate( score.begin() , score.end() , 0 ) ,
            T = std::accumulate( time.begin() , time.end() , 0 );
  const int V = draw( g , 0 , S + 3 );
  long best = LINF;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   long p = 0 , c = 0;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) { p += score[ i ]; c += time[ i ]; }
   if( p >= V ) best = std::min( best , c );
   }
  const auto [ Set , c ] = collezioni::lazyStudent( score , time , V );
  CHECK( c == best );
  if( best == LINF ) continue;
  long p = 0 , q = 0;
  for( int i : Set ) { p += score[ i ]; q += time[ i ]; }
  CHECK( ( p >= V ) && ( q == c ) );
  if( std::count( score.begin() , score.end() , 0 ) == 0 )
   CHECK( c == T - knapsack( Longs( time.begin() , time.end() ) ,
                             Longs( score.begin() , score.end() ) ,
                             S - V ).first );
  }
 }

// collezione 13-pd, 1.13/4: with partial credit the least time is the
// total minus the best fractionalKnapsack() of ADS of the problems left
// out, and it is at most the time of the whole problems
ADS_TEST( "collezioni-13-pd-1-13-4" )
{
 CHECK( collezioni::lazyFractional( { 10 , 10 } , { 10 , 50 } , 15 ) ==
        35.0 );
 CHECK( collezioni::lazyFractional( { 4 } , { 8 } , 0 ) == 0.0 );
 CHECK( std::isinf( collezioni::lazyFractional( { 4 } , { 8 } , 5 ) ) );
 std::mt19937 g( 1314 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = draw( g , 1 , 10 );
  const Ints score = drawInts( g , n , 1 , 8 ) , time = drawInts( g , n , 0 ,
                                                                    20 );
  const int S = std::accumulate( score.begin() , score.end() , 0 ) ,
            T = std::accumulate( time.begin() , time.end() , 0 );
  const int V = draw( g , 0 , S );
  const double f = collezioni::lazyFractional( score , time , V );
  const std::vector< double > x = fractionalKnapsack(
         std::vector< double >( time.begin() , time.end() ) ,
         std::vector< double >( score.begin() , score.end() ) , S - V );
  double out = 0;
  for( int i = 0 ; i < n ; ++i ) out += x[ i ] * time[ i ];
  CHECK( std::abs( f - ( T - out ) ) < 1e-9 );
  CHECK( f <= collezioni::lazyStudent( score , time , V ).second + 1e-9 );
  }
 }

// collezione 13-pd, 1.14: the binary searches against a linear scan, with
// many equal ends
ADS_TEST( "collezioni-13-pd-1-14" )
{
 CHECK( collezioni::computePredecessor( {} , {} ) == Ints{ 0 } );
 CHECK( collezioni::computePredecessor( { 0 , 1 , 3 } , { 2 , 3 , 4 } ) ==
        Ints( { 0 , 0 , 0 , 2 } ) );
 std::mt19937 g( 1315 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int n = draw( g , 0 , 20 );
  Ints f = drawInts( g , n , 1 , 12 );
  std::sort( f.begin() , f.end() );
  Ints s( n );
  for( int i = 0 ; i < n ; ++i )
   s[ i ] = f[ i ] - draw( g , 1 , std::min( f[ i ] , 5 ) );
  Ints pred( n + 1 , 0 );
  for( int i = 1 ; i <= n ; ++i )
   for( int j = 1 ; j < i ; ++j )
    if( f[ j - 1 ] <= s[ i - 1 ] ) pred[ i ] = j;
  CHECK( collezioni::computePredecessor( s , f ) == pred );
  }
 }

// collezione 13-pd, 1.15: the schedule against all the sets of songs and
// against knapsack() of ADS on the ratings in hundredths (on the
// instances with no duration 0, which knapsack() never takes)
ADS_TEST( "collezioni-13-pd-1-15" )
{
 const auto [ S0 , r0 ] = collezioni::schedule( { 0.5 , 0.4 , 0.3 } ,
                                                { 3 , 2 , 2 } , 4 );
 CHECK( ( S0 == Ints( { 2 , 1 } ) ) && ( std::abs( r0 - 0.7 ) < 1e-12 ) );
 CHECK( collezioni::schedule( { 0.9 } , { 5 } , 4 ).second == 0 );
 std::mt19937 g( 1316 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = draw( g , 0 , 12 ) , N = draw( g , 0 , 30 );
  const Ints h = drawInts( g , n , 0 , 100 ) , d = drawInts( g , n , 0 , 9 );
  std::vector< double > r( n );
  for( int i = 0 ; i < n ; ++i ) r[ i ] = h[ i ] / 100.0;
  long best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   long p = 0 , v = 0;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) { p += h[ i ]; v += d[ i ]; }
   if( v <= N ) best = std::max( best , p );
   }
  const auto [ S , v ] = collezioni::schedule( r , d , N );
  long p = 0 , w = 0;
  for( int i : S ) { p += h[ i ]; w += d[ i ]; }
  CHECK( ( w <= N ) && ( p == best ) && ( std::abs( v - best / 100.0 ) <
                                          1e-9 ) );
  if( std::count( d.begin() , d.end() , 0 ) == 0 )
   CHECK( knapsack( Longs( h.begin() , h.end() ) ,
                    Longs( d.begin() , d.end() ) , N ).first == best );
  }
 }

// collezione 13-pd, 1.17: the two rows against lcsTable() of ADS and the
// enumeration of the subsequences of the shorter string
ADS_TEST( "collezioni-13-pd-1-17" )
{
 CHECK( collezioni::lcsAtLeast( "" , "abc" , 0 ) );
 CHECK( ! collezioni::lcsAtLeast( "" , "abc" , 1 ) );
 CHECK( collezioni::lcsAtLeast( "abcbdab" , "bdcaba" , 4 ) &&
        ! collezioni::lcsAtLeast( "abcbdab" , "bdcaba" , 5 ) );
 std::mt19937 g( 1317 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string X = drawString( g , draw( g , 0 , 8 ) , 3 ) ,
                    Y = drawString( g , draw( g , 0 , 10 ) , 3 );
  const int L = lcsTable( X , Y ).back().back();
  int best = 0;
  for( int mask = 0 ; mask < ( 1 << X.size() ) ; ++mask ) {
   std::string sub;
   for( std::size_t i = 0 ; i < X.size() ; ++i )
    if( mask >> i & 1 ) sub += X[ i ];
   std::size_t k = 0;
   for( char c : Y )
    if( ( k < sub.size() ) && ( sub[ k ] == c ) ) ++k;
   if( k == sub.size() ) best = std::max( best , int( sub.size() ) );
   }
  CHECK( L == best );
  for( int k = 0 ; k <= 11 ; ++k )
   CHECK( collezioni::lcsAtLeast( X , Y , k ) == ( best >= k ) );
  }
 }

// collezione 13-pd, 1.18: the examples of the text, casacca 2 and anta 1,
// and n minus the LCS of s and its reverse by lcs() of ADS, which is n
// minus the longest palindromic subsequence, enumerated
ADS_TEST( "collezioni-13-pd-1-18" )
{
 CHECK( collezioni::minpal( "casacca" ) == 2 );
 CHECK( collezioni::minpal( "anta" ) == 1 );
 CHECK( collezioni::minpal( "x" ) == 0 && collezioni::minpal( "" ) == 0 );
 std::mt19937 g( 1318 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string s = drawString( g , draw( g , 1 , 11 ) , 3 );
  const std::string r( s.rbegin() , s.rend() );
  const int n = int( s.size() );
  CHECK( collezioni::minpal( s ) == n - int( lcs( s , r ).size() ) );
  int best = 0;
  for( int mask = 1 ; mask < ( 1 << n ) ; ++mask ) {
   std::string sub;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) sub += s[ i ];
   if( std::equal( sub.begin() , sub.end() , sub.rbegin() ) )
    best = std::max( best , int( sub.size() ) );
   }
  CHECK( collezioni::minpal( s ) == n - best );
  }
 }

// collezione 13-pd, 1.19: the least cost against all the sets of days of
// purchase, and the passes printed cost what the result says
ADS_TEST( "collezioni-13-pd-1-19" )
{
 const auto [ c0 , b0 ] = collezioni::railway( { 1 , 10 , 29 , 31 } ,
                                               { 20 , 20 , 20 , 5 } , 50 );
 CHECK( ( c0 == 55 ) && ( b0 == Ints{ 0 } ) );
 CHECK( collezioni::railway( { 3 } , { 7 } , 7 ).first == 7 );
 std::mt19937 g( 1319 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = draw( g , 1 , 11 );
  Ints d = drawInts( g , n , 0 , 90 ) , f = drawInts( g , n , 1 , 30 );
  std::sort( d.begin() , d.end() );
  const long X = draw( g , 10 , 80 );
  const auto cost = [ & ]( const Ints & buy ) {
   long c = X * long( buy.size() );
   for( int j = 0 ; j < n ; ++j ) {
    bool covered = false;
    for( int i : buy )
     covered = covered || ( ( d[ i ] <= d[ j ] ) && ( d[ j ] < d[ i ] + 30 ) );
    if( ! covered ) c += f[ j ];
    }
   return( c );
   };
  long best = LINF;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   Ints buy;
   for( int i = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) buy.push_back( i );
   best = std::min( best , cost( buy ) );
   }
  const auto [ c , buy ] = collezioni::railway( d , f , X );
  CHECK( ( c == best ) && ( cost( buy ) == c ) );
  }
 }

// collezione 13-pd, 1.20: the example of the text, 19 by a reboot on the
// day 1, and all the sets of days of reboot
ADS_TEST( "collezioni-13-pd-1-20" )
{
 CHECK( collezioni::datacenter( { 10 , 1 , 7 , 7 } , { 8 , 4 , 2 , 1 } ) ==
        19 );
 CHECK( collezioni::datacenter( { 5 } , { 3 } ) == 3 );
 CHECK( collezioni::datacenter( {} , {} ) == 0 );
 std::mt19937 g( 1320 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 1 , 12 );
  const Ints x = drawInts( g , n , 0 , 20 );
  Ints s = drawSortedDistinct( g , n , 1 , 40 );
  std::reverse( s.begin() , s.end() );     // strictly decreasing
  long best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   long done = 0;
   for( int i = 0 , a = 0 ; i < n ; ++i )
    if( mask >> i & 1 ) a = 0;
    else done += std::min( x[ i ] , s[ a++ ] );
   best = std::max( best , done );
   }
  CHECK( collezioni::datacenter( x , s ) == best );
  }
 }

// collezione 13-pd, 1.21: the examples of the text, 3 and 0 ways, and a
// recursive enumeration of the decompositions
ADS_TEST( "collezioni-13-pd-1-21" )
{
 const std::vector< std::string > S = { "01" , "10" , "011" , "101" };
 CHECK( collezioni::countWays( "0111010101" , S ) == 3 );
 CHECK( collezioni::countWays( "0110001" , S ) == 0 );
 CHECK( collezioni::countWays( "" , S ) == 1 );
 CHECK( collezioni::countWays( std::string( 60 , '0' ) , { "0" , "00" } ) ==
        fibonacci( 60 ) );
 std::mt19937 g( 1321 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::vector< std::string > P;
  for( int k = draw( g , 1 , 4 ) ; k > 0 ; --k )
   P.push_back( drawString( g , draw( g , 1 , 3 ) , 2 ) );
  const std::string x = drawString( g , draw( g , 0 , 12 ) , 2 );
  std::function< std::uint64_t( std::size_t ) > ways = [ & ](
                                                      std::size_t i ) {
   std::uint64_t w = ( i == x.size() );
   for( const auto & p : P )
    if( x.compare( i , p.size() , p ) == 0 && i + p.size() <= x.size() )
     w += ways( i + p.size() );
   return( w );
   };
  CHECK( collezioni::countWays( x , P ) == ways( 0 ) );
  }
 }

// collezione 13-pd, 1.22: the full trees of 2k + 1 nodes are the Catalan
// number C( 2k , k ) / ( k + 1 ), none of an even number (up to 61 nodes,
// since C( 60 , 30 ) fits in 64 bits, and the largest n = 69); and the binary
// trees of n nodes, enumerated as the strings of balanced parentheses by
// nestedParentheses() of ADS, of which the full ones are counted
ADS_TEST( "collezioni-13-pd-1-22" )
{
 CHECK( collezioni::fullTrees( 1 ) == 1 && collezioni::fullTrees( 2 ) == 0 );
 CHECK( collezioni::fullTrees( 7 ) == 5 );
 CHECK( collezioni::fullTrees( 69 ) == 812944042149730764ULL );
 const auto C = tartaglia( 60 );
 for( int k = 0 ; k <= 30 ; ++k ) {
  CHECK( collezioni::fullTrees( 2 * k + 1 ) == C[ 2 * k ][ k ] / ( k + 1 ) );
  if( k > 0 ) CHECK( collezioni::fullTrees( 2 * k ) == 0 );
  }
 for( int n = 1 ; n <= 11 ; ++n ) {
  // ( L ) R is the root with the left subtree L and the right one R
  std::uint64_t full = 0;
  nestedParentheses( n , [ & ]( const auto & w ) {
   std::size_t i = 0;
   std::function< int( void ) > tree = [ & ]( void ) -> int {
    // the children of the root that starts at w[ i ], -1 if not full
    if( ( i == w.size() ) || ( w[ i ] == ')' ) )
     return( 0 );                          // the empty tree
    ++i;
    const int l = tree();
    ++i;                                   // the ')'
    const int r = tree();
    if( ( l < 0 ) || ( r < 0 ) || ( ( l == 0 ) != ( r == 0 ) ) ) return( -1 );
    return( 1 + l + r );
    };
   full += ( tree() > 0 );
   return( false );                        // go on
   } );
  CHECK( full == collezioni::fullTrees( n ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------- COLLECTION 14, GREEDY -----------------------------*/
/*--------------------------------------------------------------------------*/

namespace collezioni {

using Edge = WeightedEdge< long >;

/// the total weight of a set of edges
long weightOf( const std::vector< Edge > & T )
{
 long w = 0;
 for( const Edge & e : T ) w += e.w;
 return( w );
 }

// collezione 14-greedy, 1.4: Algoritmo alternativo per MST

/// the scheme proposed in the text, which is wrong: the minimum spanning
/// trees of the subgraphs induced by the two sides of the nodes, joined by
/// the lightest edge across; its weight, or LINF if no edge is across or
/// a side is not connected, where the scheme gives no spanning tree
long proposedMST( int n , const std::vector< Edge > & E ,
                  const std::vector< bool > & side )
{
 std::vector< Edge > A , B;
 long across = LINF;
 for( const Edge & e : E )
  if( side[ e.u ] != side[ e.v ] ) across = std::min( across , e.w );
  else ( side[ e.u ] ? A : B ).push_back( e );
 const auto TA = kruskal( n , A ) , TB = kruskal( n , B );
 if( ( across == LINF ) || ( int( TA.size() + TB.size() ) != n - 2 ) )
  return( LINF );
 return( weightOf( TA ) + weightOf( TB ) + across );
 }

// collezione 14-greedy, 1.5: Albero bianco e nero

/// minBlack(): the least number of black edges of a spanning tree: the
/// black edges weigh 1 and the white ones 0, and a minimum spanning tree
/// by Kruskal, its weights sorted by counting (k = 2), has the fewest;
/// O( n + m alpha( n ) )
/// \pre the ends of the edges in [ 0 , n )
long minBlack( int n , const std::vector< std::pair< int , int > > & white ,
               const std::vector< std::pair< int , int > > & black )
{
 std::vector< Edge > E;
 for( auto [ u , v ] : white ) E.push_back( Edge{ u , v , 0 } );
 for( auto [ u , v ] : black ) E.push_back( Edge{ u , v , 1 } );
 return( weightOf( kruskal( n , E , 2 ) ) );
 }

// collezione 14-greedy, 1.9: Batterie

/// minStops(): the fewest stops to go from 0 to L with a range R, the
/// stations at D: refuelingStops() of the library, which, when the next
/// station is beyond the range, stops at the last one before it; O( n )
/// \pre D strictly increasing, each gap (from 0, between the stations, to
///      L) at most R
int minStops( const Longs & D , long L , long R )
{
 const auto S = refuelingStops( D , L , R );
 ADS_PRE( S.has_value() );
 return( int( S->size() ) );
 }

// collezione 14-greedy, 1.10: Sciatori e sci

/// skis(): the least total difference of heights and lengths: both sorted
/// and matched by place, since two crossing pairs never cost less when
/// uncrossed; O( n log n )
/// \pre |H| = |L|
long skis( Ints H , Ints L )
{
 ADS_PRE( H.size() == L.size() );
 std::sort( H.begin() , H.end() );
 std::sort( L.begin() , L.end() );
 long c = 0;
 for( std::size_t i = 0 ; i < H.size() ; ++i )
  c += std::abs( long( H[ i ] ) - L[ i ] );
 return( c );
 }

/// the greedy of the text, which is wrong: the pair of least difference
/// first, then the least among the rest
long skisGreedy( const Ints & H , const Ints & L )
{
 const std::size_t n = H.size();
 std::vector< bool > h( n ) , l( n );
 long c = 0;
 for( std::size_t k = 0 ; k < n ; ++k ) {
  long best = LINF;
  std::size_t bi = 0 , bj = 0;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    if( ! h[ i ] && ! l[ j ] && ( std::abs( long( H[ i ] ) - L[ j ] ) <
                                  best ) ) {
     best = std::abs( long( H[ i ] ) - L[ j ] );
     bi = i;
     bj = j;
     }
  h[ bi ] = l[ bj ] = true;
  c += best;
  }
 return( c );
 }

}  // end( namespace collezioni )

namespace {

/// f( T ) on each spanning tree T of the nodes [ 0 , n ), the places of
/// its n - 1 edges in E, by trying all the sets of n - 1 edges
template< class F >
void spanningTrees( int n , const std::vector< collezioni::Edge > & E ,
                    F && f )
{
 const int m = int( E.size() );
 for( std::uint64_t mask = 0 ; mask < ( std::uint64_t( 1 ) << m ) ; ++mask ) {
  if( std::popcount( mask ) != n - 1 ) continue;
  Ints p( n );
  std::iota( p.begin() , p.end() , 0 );
  std::function< int( int ) > find = [ & ]( int x ) {
   return( p[ x ] == x ? x : p[ x ] = find( p[ x ] ) ); };
  bool ok = true;
  Ints T;
  for( int i = 0 ; i < m ; ++i )
   if( mask >> i & 1 ) {
    const int a = find( E[ i ].u ) , b = find( E[ i ].v );
    ok = ok && ( a != b );
    p[ a ] = b;
    T.push_back( i );
    }
  if( ok ) f( T );
  }
 }

/// a random connected unoriented graph: a random tree of n nodes and m more
/// edges, the weights in [ lo , hi ]
std::vector< collezioni::Edge > randomConnected( std::mt19937 & g , int n ,
                                                 int m , int lo , int hi )
{
 std::vector< collezioni::Edge > E;
 for( int v = 1 ; v < n ; ++v )
  E.push_back( collezioni::Edge{ draw( g , 0 , v - 1 ) , v ,
                                 draw( g , lo , hi ) } );
 for( int i = 0 ; ( i < m ) && ( n > 1 ) ; ++i ) {
  const int u = draw( g , 0 , n - 1 ) , v = draw( g , 0 , n - 1 );
  if( u != v ) E.push_back( collezioni::Edge{ u , v , draw( g , lo , hi ) } );
  }
 return( E );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

// collezione 14-greedy, 1.3: kruskal() of ADS with the weights in [ 1 , n ]
// sorted by counting, against the one sorted by comparisons and Prim
ADS_TEST( "collezioni-14-greedy-1-3" )
{
 CHECK( kruskal( 0 , std::vector< collezioni::Edge >() , 1 ).empty() );
 std::mt19937 g( 143 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 1 , 30 );
  const auto E = randomConnected( g , n , draw( g , 0 , 3 * n ) , 1 , n );
  const auto T = kruskal( n , E , n + 1 );
  CHECK( ( int( T.size() ) == n - 1 ) &&
         isMinimumSpanningForest( n , E , T ) );
  CHECK( collezioni::weightOf( T ) ==
         collezioni::weightOf( kruskal( n , E ) ) );
  }
 }

// collezione 14-greedy, 1.4: the counterexample of the text, 21 against
// 12, and on random graphs the scheme never beats Kruskal
ADS_TEST( "collezioni-14-greedy-1-4" )
{
 const std::vector< collezioni::Edge > C = { { 0 , 1 , 1 } , { 1 , 2 , 10 } ,
                                             { 2 , 3 , 1 } , { 3 , 0 , 10 } };
 const std::vector< bool > AD = { true , false , false , true };
 CHECK( collezioni::proposedMST( 4 , C , AD ) == 21 );
 CHECK( collezioni::weightOf( kruskal( 4 , C ) ) == 12 );
 std::mt19937 g( 144 );
 int worse = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 2 , 12 );
  const auto E = randomConnected( g , n , 2 * n , 1 , 20 );
  std::vector< bool > side( n );
  for( int v = 0 ; v < n ; ++v ) side[ v ] = draw( g , 0 , 1 );
  const long p = collezioni::proposedMST( n , E , side );
  if( p == LINF ) continue;
  const long best = collezioni::weightOf( kruskal( n , E ) );
  CHECK( p >= best );
  worse += ( p > best );
  }
 CHECK( worse > 0 );
 }

// collezione 14-greedy, 1.5: the fewest black edges against all the
// spanning trees of small graphs
ADS_TEST( "collezioni-14-greedy-1-5" )
{
 CHECK( collezioni::minBlack( 3 , { { 0 , 1 } } , { { 1 , 2 } , { 0 , 2 } } )
        == 1 );
 CHECK( collezioni::minBlack( 1 , {} , {} ) == 0 );
 std::mt19937 g( 145 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = draw( g , 1 , 6 );
  const auto E = randomConnected( g , n , draw( g , 0 , 6 ) , 0 , 1 );
  std::vector< std::pair< int , int > > W , B;
  for( const auto & e : E ) ( e.w ? B : W ).emplace_back( e.u , e.v );
  long best = LINF;
  spanningTrees( n , E , [ & ]( const Ints & T ) {
   long b = 0;
   for( int i : T ) b += E[ i ].w;
   best = std::min( best , b );
   } );
  CHECK( collezioni::minBlack( n , W , B ) == best );
  }
 }

// collezione 14-greedy, 1.6/3: the triangle of the text, whose minimum
// spanning tree has e1 and e2 but not the third lightest edge e3; on
// random graphs with distinct weights the two lightest edges are always in
ADS_TEST( "collezioni-14-greedy-1-6-3" )
{
 const std::vector< collezioni::Edge > T = { { 0 , 1 , 1 } , { 0 , 2 , 3 } ,
                                             { 1 , 2 , 5 } };
 const auto M = kruskal( 3 , T );
 CHECK( ( M.size() == 2 ) && ( collezioni::weightOf( M ) == 4 ) );
 CHECK( std::find( M.begin() , M.end() , T[ 2 ] ) == M.end() );
 std::mt19937 g( 146 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 3 , 10 );
  auto E = randomConnected( g , n , 2 * n , 0 , 0 );
  std::set< std::pair< int , int > > seen;   // no parallel edges
  std::erase_if( E , [ & ]( const collezioni::Edge & e ) {
   return( ! seen.insert( std::minmax( e.u , e.v ) ).second ); } );
  Ints w( E.size() );
  std::iota( w.begin() , w.end() , 1 );
  std::shuffle( w.begin() , w.end() , g );
  for( std::size_t i = 0 ; i < E.size() ; ++i ) E[ i ].w = w[ i ];
  const auto K = kruskal( n , E );
  for( long x : { 1L , 2L } ) {
   const auto e = std::find_if( E.begin() , E.end() , [ & ]( auto & y ) {
    return( y.w == x ); } );
   CHECK( std::find( K.begin() , K.end() , *e ) != K.end() );
   }
  }
 }

// collezione 14-greedy, 1.8: Kruskal on the weights themselves gives the
// least product, against the products of all the spanning trees
ADS_TEST( "collezioni-14-greedy-1-8" )
{
 std::mt19937 g( 148 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = draw( g , 1 , 6 );
  const auto E = randomConnected( g , n , draw( g , 0 , 6 ) , 1 , 9 );
  std::uint64_t best = std::numeric_limits< std::uint64_t >::max();
  spanningTrees( n , E , [ & ]( const Ints & T ) {
   std::uint64_t p = 1;
   for( int i : T ) p *= std::uint64_t( E[ i ].w );
   best = std::min( best , p );
   } );
  std::uint64_t p = 1;
  for( const auto & e : kruskal( n , E ) ) p *= std::uint64_t( e.w );
  CHECK( p == best );
  }
 }

// collezione 14-greedy, 1.9: the greedy against the fewest stops of a
// visit in breadth of the stations reachable from each other
ADS_TEST( "collezioni-14-greedy-1-9" )
{
 CHECK( collezioni::minStops( {} , 5 , 5 ) == 0 );
 CHECK( collezioni::minStops( { 4 } , 8 , 5 ) == 1 );
 CHECK( collezioni::minStops( { 5 , 10 } , 15 , 5 ) == 2 );   // = deadline
 std::mt19937 g( 149 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const long R = draw( g , 1 , 10 );
  Longs D;
  long x = 0;
  for( int k = draw( g , 0 , 15 ) ; k > 0 ; --k )
   D.push_back( x += draw( g , 1 , int( R ) ) );
  const long L = x + draw( g , 1 , int( R ) );
  Longs P = { 0 };                         // the start, the stations, L
  P.insert( P.end() , D.begin() , D.end() );
  P.push_back( L );
  Ints stops( P.size() , ADS::INF );
  stops[ 0 ] = 0;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   for( std::size_t j = i + 1 ; ( j < P.size() ) && ( P[ j ] - P[ i ] <= R ) ;
        ++j )
    stops[ j ] = std::min( stops[ j ] , stops[ i ] + ( j + 1 < P.size() ) );
  CHECK( collezioni::minStops( D , L , R ) == stops.back() );
  }
 }

// collezione 14-greedy, 1.10: the counterexample of the text, greedy 10
// and optimum 8, and the sorted matching against all the assignments
ADS_TEST( "collezioni-14-greedy-1-10" )
{
 CHECK( collezioni::skisGreedy( { 5 , 10 } , { 9 , 14 } ) == 10 );
 CHECK( collezioni::skis( { 5 , 10 } , { 9 , 14 } ) == 8 );
 CHECK( collezioni::skis( {} , {} ) == 0 );
 std::mt19937 g( 1410 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 1 , 7 );
  const Ints H = drawInts( g , n , 100 , 200 ) , L = drawInts( g , n , 100 ,
                                                                200 );
  Ints p( n );
  std::iota( p.begin() , p.end() , 0 );
  long best = LINF;
  do {
   long c = 0;
   for( int i = 0 ; i < n ; ++i ) c += std::abs( H[ i ] - L[ p[ i ] ] );
   best = std::min( best , c );
   } while( std::next_permutation( p.begin() , p.end() ) );
  CHECK( collezioni::skis( H , L ) == best );
  CHECK( collezioni::skisGreedy( H , L ) >= best );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- COLLECTION 15, NETWORKS OF FLOW --------------------*/
/*--------------------------------------------------------------------------*/

namespace collezioni {

using Arcs = std::vector< std::tuple< int , int , long > >;

// collezione 15-locale, 1.1: Formulazioni del flusso massimo

/// the maximum flow from the sources to the sinks: a super source joined
/// to each source and each sink to a super sink, by edges of capacity
/// larger than any cut
/// \pre sources and sinks disjoint and not empty
long multiFlow( int n , const Arcs & E , const Ints & sources ,
                const Ints & sinks )
{
 long big = 1;
 for( auto [ u , v , c ] : E ) big += c;
 FlowNetwork F( n + 2 );
 for( auto [ u , v , c ] : E ) F.addEdge( u , v , c );
 for( int s : sources ) F.addEdge( n , s , big );
 for( int t : sinks ) F.addEdge( t , n + 1 , big );
 return( F.maxFlow( n , n + 1 ) );
 }

/// the edges of the network where each node v but s and t is split into
/// v (entering) and n + v (leaving), joined by an edge of capacity cap[ v ]
Arcs splitNodes( int n , const Arcs & E , const Longs & cap , int s , int t )
{
 Arcs S;
 for( auto [ u , v , c ] : E )
  S.emplace_back( ( u == s ) || ( u == t ) ? u : n + u , v , c );
 for( int v = 0 ; v < n ; ++v )
  if( ( v != s ) && ( v != t ) ) S.emplace_back( v , n + v , cap[ v ] );
 return( S );
 }

/// the maximum flow with capacities on the nodes too, on the split nodes
long nodeCapacityFlow( int n , const Arcs & E , const Longs & cap , int s ,
                       int t )
{
 return( maxFlowOf( 2 * n , splitNodes( n , E , cap , s , t ) , s , t ) );
 }

/// the maximum flow of an unoriented network: each edge [ u , v ] of
/// capacity c becomes ( u , v ) and ( v , u ) of capacity c, the flows in
/// opposite directions cancelling
long unorientedFlow( int n , const Arcs & E , int s , int t )
{
 Arcs D;
 for( auto [ u , v , c ] : E ) {
  D.emplace_back( u , v , c );
  D.emplace_back( v , u , c );
  }
 return( maxFlowOf( n , D , s , t ) );
 }

// collezione 15-locale, 1.2: Aggiornamento del flusso massimo

/// increaseCapacity(): the maximum flow F after the capacity of ( u , v )
/// grows by 1, as an edge of capacity 1 parallel to it: the maximum grows
/// by at most 1, so one search of an augmenting path in the residual
/// network suffices; O( n + m ). The increase, 0 or 1
long increaseCapacity( FlowNetwork & F , int s , int t , int u , int v )
{
 F.addEdge( u , v , 1 );
 long paths = 0;
 const long d = F.maxFlow( s , t , Augment::FORD_FULKERSON , &paths );
 ADS_POST( ( paths <= 1 ) && ( d == paths ) );
 return( d );
 }

// collezione 15-locale, 1.3: Cammini indipendenti

// the most paths from s to t with no edge in common are those of
// edgeDisjointPaths() of the library, the maximum flow of capacity 1 on
// each edge (by integrality and the decomposition of a flow in paths);
// O( n ( n + m ) )

// collezione 15-locale, 1.4 and 1.5: Piano sanitario, Torri di controllo

/// assign(): each of the n items on the left to one on the right that it
/// can go to (A[ i ][ j ]), each j taking at most cap items: the source
/// joined to each item by capacity 1, i to j by 1 if A[ i ][ j ], j to the
/// sink by cap; an assignment exists iff the maximum flow is n, and the
/// edges i -> j of flow 1 give it; O( n^2 k ) by augmenting paths
std::optional< Ints > assign( const std::vector< std::vector< bool > > & A ,
                              int k , long cap )
{
 const int n = int( A.size() ) , s = n + k , t = n + k + 1;
 FlowNetwork F( n + k + 2 );
 std::vector< std::vector< int > > id( n , Ints( k , -1 ) );
 for( int i = 0 ; i < n ; ++i ) {
  F.addEdge( s , i , 1 );
  for( int j = 0 ; j < k ; ++j )
   if( A[ i ][ j ] ) id[ i ][ j ] = F.addEdge( i , n + j , 1 );
  }
 for( int j = 0 ; j < k ; ++j ) F.addEdge( n + j , t , cap );
 if( F.maxFlow( s , t ) < n ) return( std::nullopt );
 Ints to( n , -1 );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < k ; ++j )
   if( ( id[ i ][ j ] >= 0 ) && ( F.flow( id[ i ][ j ] ) == 1 ) ) to[ i ] = j;
 return( to );
 }

/// hospitals(): the patients assigned to reachable hospitals, at most
/// ceil( n / k ) each, or nothing
/// \pre k >= 1
std::optional< Ints > hospitals(
                        const std::vector< std::vector< bool > > & reachable ,
                        int k )
{
 ADS_PRE( k >= 1 );
 const long n = long( reachable.size() );
 return( assign( reachable , k , ( n + k - 1 ) / k ) );
 }

/// towers(): the planes assigned to towers within distance d, at most L
/// each, or nothing; Theta( n m ) to build the network
std::optional< Ints > towers( const std::vector< std::pair< int , int > > & A ,
                              const std::vector< std::pair< int , int > > & Q ,
                              long d , long L )
{
 std::vector< std::vector< bool > > M( A.size() ,
                                       std::vector< bool >( Q.size() ) );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < Q.size() ; ++j ) {
   const long dx = A[ i ].first - Q[ j ].first ,
              dy = A[ i ].second - Q[ j ].second;
   M[ i ][ j ] = ( dx * dx + dy * dy <= d * d );
   }
 return( assign( M , int( Q.size() ) , L ) );
 }

// collezione 15-locale, 1.6: McDonald's

/// mcdonalds(): the shifts of each employee (works[ j ][ i ]), shift i
/// on the day i / 3 needing p[ i ] employees, an employee working at
/// most W shifts in all and 2 a day, never when unavailable; or nothing.
/// The network: the source to each employee j by W, j to the node ( j ,
/// day ) by 2, that to each shift of the day j is free for by 1, each
/// shift to the sink by p[ i ]; feasible iff the flow is the sum of p
/// \pre |p| a multiple of 3, |unavailable[ j ]| = |p|
std::optional< std::vector< std::vector< bool > > > mcdonalds(
                    const Ints & p ,
                    const std::vector< std::vector< bool > > & unavailable ,
                    long W )
{
 const int S = int( p.size() ) , days = S / 3 ,
           E = int( unavailable.size() );
 ADS_PRE( S % 3 == 0 );
 const int s = E + E * days + S , t = s + 1;
 FlowNetwork F( t + 1 );
 std::vector< Ints > id( E , Ints( S , -1 ) );
 for( int j = 0 ; j < E ; ++j ) {
  F.addEdge( s , j , W );
  for( int d = 0 ; d < days ; ++d ) {
   const int wd = E + j * days + d;
   F.addEdge( j , wd , 2 );
   for( int i = 3 * d ; i < 3 * d + 3 ; ++i )
    if( ! unavailable[ j ][ i ] )
     id[ j ][ i ] = F.addEdge( wd , E + E * days + i , 1 );
   }
  }
 long P = 0;
 for( int i = 0 ; i < S ; ++i ) {
  F.addEdge( E + E * days + i , t , p[ i ] );
  P += p[ i ];
  }
 if( F.maxFlow( s , t ) < P ) return( std::nullopt );
 std::vector< std::vector< bool > > works( E , std::vector< bool >( S ) );
 for( int j = 0 ; j < E ; ++j )
  for( int i = 0 ; i < S ; ++i )
   works[ j ][ i ] = ( id[ j ][ i ] >= 0 ) && ( F.flow( id[ j ][ i ] ) == 1 );
 return( works );
 }

// collezione 15-locale, 1.7: Reti telematiche

/// films(): whether the server s can send the n films of b bit/s each to
/// their clients C[ i ] at the same time: a super sink joined to each
/// client by b times the films it gets, and the flow must be n b
bool films( int n , const Arcs & E , int s , const Ints & C , long b )
{
 Arcs N = E;
 for( int c : C )
  N.emplace_back( c , n , b );         // parallel edges add up
 return( maxFlowOf( n + 1 , N , s , n ) == long( C.size() ) * b );
 }

// collezione 15-locale, 1.8: Evacuazione

/// evacuation(): whether each village (village[ v ]) has a path to a safe
/// place, the paths with no edge in common: capacity 1 on the edges, a
/// super source joined to each village by 1, each safe place to a super
/// sink by the number of villages; feasible iff the flow is that number;
/// O( |S| ( n + m ) )
bool evacuation( const ListGraph<> & G , const std::vector< bool > & village )
{
 const int n = G.bound();
 Arcs E;
 long k = 0;
 for( int u : G.V() ) {
  for( int v : G.adj( u ) ) E.emplace_back( u , v , 1 );
  k += village[ u ];
  }
 for( int u : G.V() )
  E.emplace_back( village[ u ] ? n : u , village[ u ] ? u : n + 1 ,
                  village[ u ] ? 1 : k );
 return( k == 0 || maxFlowOf( n + 2 , E , n , n + 1 ) == k );
 }

// collezione 15-locale, 1.9: Riempimento di matrice

using Matrix = std::vector< Longs >;

/// fillMatrix(): a matrix of non-negative integers with row sums r and
/// column sums c, by the northwest corner rule (the least of what is left
/// of the row and of the column, then on to the next row or column), or
/// nothing if the sums of r and c differ; Theta( n^2 )
/// \pre |r| = |c|, r , c >= 0
std::optional< Matrix > fillMatrix( Longs r , Longs c )
{
 const std::size_t n = r.size();
 ADS_PRE( c.size() == n );
 if( std::accumulate( r.begin() , r.end() , 0L ) !=
     std::accumulate( c.begin() , c.end() , 0L ) )
  return( std::nullopt );
 Matrix M( n , Longs( n , 0 ) );
 for( std::size_t i = 0 , j = 0 ; ( i < n ) && ( j < n ) ; ) {
  M[ i ][ j ] = std::min( r[ i ] , c[ j ] );
  r[ i ] -= M[ i ][ j ];
  c[ j ] -= M[ i ][ j ];
  if( r[ i ] == 0 ) ++i;
  else ++j;
  }
 return( M );
 }

/// the same by a flow: the source to row i by r[ i ], each row to each
/// column by the sum, column j to the sink by c[ j ]; M[ i ][ j ] is the
/// flow of row i to column j
std::optional< Matrix > fillMatrixFlow( const Longs & r , const Longs & c )
{
 const int n = int( r.size() ) , s = 2 * n , t = 2 * n + 1;
 const long R = std::accumulate( r.begin() , r.end() , 0L );
 FlowNetwork F( 2 * n + 2 );
 std::vector< Ints > id( n , Ints( n ) );
 for( int i = 0 ; i < n ; ++i ) {
  F.addEdge( s , i , r[ i ] );
  F.addEdge( n + i , t , c[ i ] );
  for( int j = 0 ; j < n ; ++j ) id[ i ][ j ] = F.addEdge( i , n + j , R );
  }
 if( ( F.maxFlow( s , t ) != R ) ||
     ( std::accumulate( c.begin() , c.end() , 0L ) != R ) )
  return( std::nullopt );
 Matrix M( n , Longs( n ) );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j ) M[ i ][ j ] = F.flow( id[ i ][ j ] );
 return( M );
 }

// collezione 15-locale, 1.10: Somma di matrici

/// splitMatrix(): M = M1 + M2 of non-negative integers, the rows and the
/// columns of Mk summing to at most lk and all of Mk to at most tk, or
/// nothing. M1 decides M2, and is a circulation with bounds: the source to
/// row i in [ max( 0 , R_i - l2 ) , min( l1 , R_i ) ], row i to column j
/// in [ 0 , M[ i ][ j ] ], column j to the sink in [ max( 0 , C_j - l2 ) ,
/// min( l1 , C_j ) ], the sink to the source in [ max( 0 , W - t2 ) ,
/// min( t1 , W ) ], R, C and W the sums of M; O( m n ) edges
/// \pre M rectangular of non-negative integers, the limits >= 0
std::optional< std::pair< Matrix , Matrix > > splitMatrix(
               const Matrix & M , long l1 , long l2 , long t1 , long t2 )
{
 const int m = int( M.size() ) , n = m ? int( M[ 0 ].size() ) : 0;
 const int s = m + n , t = m + n + 1;
 std::vector< BoundedEdge > E;
 long W = 0;
 for( int i = 0 ; i < m ; ++i ) {
  const long R = std::accumulate( M[ i ].begin() , M[ i ].end() , 0L );
  E.push_back( BoundedEdge{ s , i , std::max( 0L , R - l2 ) ,
                            std::min( l1 , R ) } );
  W += R;
  }
 for( int j = 0 ; j < n ; ++j ) {
  long C = 0;
  for( int i = 0 ; i < m ; ++i ) C += M[ i ][ j ];
  E.push_back( BoundedEdge{ m + j , t , std::max( 0L , C - l2 ) ,
                            std::min( l1 , C ) } );
  }
 E.push_back( BoundedEdge{ t , s , std::max( 0L , W - t2 ) ,
                           std::min( t1 , W ) } );
 const std::size_t first = E.size();
 for( int i = 0 ; i < m ; ++i )
  for( int j = 0 ; j < n ; ++j )
   E.push_back( BoundedEdge{ i , m + j , 0 , M[ i ][ j ] } );
 for( const auto & e : E )
  if( e.low > e.high ) return( std::nullopt );
 std::vector< long > f;
 if( ! feasibleCirculation( m + n + 2 , E , &f ) ) return( std::nullopt );
 Matrix A( m , Longs( n ) ) , B = A;
 for( int i = 0 ; i < m ; ++i )
  for( int j = 0 ; j < n ; ++j ) {
   A[ i ][ j ] = f[ first + i * n + j ];
   B[ i ][ j ] = M[ i ][ j ] - A[ i ][ j ];
   }
 return( std::pair( A , B ) );
 }

}  // end( namespace collezioni )

namespace {

using collezioni::Arcs;
using collezioni::Matrix;

/// random edges among n nodes, each with probability p percent, of
/// capacity in [ 1 , hi ]
Arcs randomArcs( std::mt19937 & g , int n , int p , int hi )
{
 Arcs E;
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && ( draw( g , 1 , 100 ) <= p ) )
    E.emplace_back( u , v , draw( g , 1 , hi ) );
 return( E );
 }

/// whether the n items can go to the k places, each at most cap items,
/// by backtrack
bool assignBrute( const std::vector< std::vector< bool > > & A , int k ,
                  long cap )
{
 Longs load( k , 0 );
 std::function< bool( std::size_t ) > rec = [ & ]( std::size_t i ) {
  if( i == A.size() ) return( true );
  for( int j = 0 ; j < k ; ++j )
   if( A[ i ][ j ] && ( load[ j ] < cap ) ) {
    ++load[ j ];
    if( rec( i + 1 ) ) return( true );
    --load[ j ];
    }
  return( false );
  };
 return( rec( 0 ) );
 }

/// whether to is an assignment of the items, within A and cap
bool validAssignment( const std::vector< std::vector< bool > > & A , int k ,
                      long cap , const Ints & to )
{
 Longs load( k , 0 );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( ( to[ i ] < 0 ) || ! A[ i ][ to[ i ] ] || ( ++load[ to[ i ] ] > cap ) )
   return( false );
 return( true );
 }

/// a random boolean matrix of n x k, each entry true with probability p
std::vector< std::vector< bool > > randomBools( std::mt19937 & g , int n ,
                                                int k , int p )
{
 std::vector< std::vector< bool > > A( n , std::vector< bool >( k ) );
 for( auto & row : A )
  for( std::size_t j = 0 ; j < row.size() ; ++j )
   row[ j ] = draw( g , 1 , 100 ) <= p;
 return( A );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

// collezione 15-locale, 1.1: the three reductions against the minimum
// cuts by brute force: of the sources from the sinks, of the network with
// the nodes split, and of the unoriented one, whose cut counts the edges
// across in either direction
ADS_TEST( "collezioni-15-locale-1-1" )
{
 std::mt19937 g( 151 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = draw( g , 3 , 7 );
  const Arcs E = randomArcs( g , n , 40 , 9 );
  Ints S = drawInts( g , n , 0 , 2 ) , src , snk;
  S[ 0 ] = 1;                              // a source and a sink at least
  S[ 1 ] = 2;
  for( int v = 0 ; v < n ; ++v )
   if( S[ v ] ) ( S[ v ] == 1 ? src : snk ).push_back( v );
  CHECK( collezioni::multiFlow( n , E , src , snk ) ==
         minCutBrute( n , E , S ) );
  Longs cap( n );
  for( long & c : cap ) c = draw( g , 0 , 9 );
  CHECK( collezioni::nodeCapacityFlow( n , E , cap , 0 , n - 1 ) ==
         minCutBrute( 2 * n , collezioni::splitNodes( n , E , cap , 0 ,
                                                      n - 1 ) , 0 , n - 1 ) );
  Arcs D = E;
  for( auto [ u , v , c ] : E ) D.emplace_back( v , u , c );
  CHECK( collezioni::unorientedFlow( n , E , 0 , n - 1 ) ==
         minCutBrute( n , D , 0 , n - 1 ) );
  }
 // a node of capacity 1 in the middle of two paths lets one unit through
 const Arcs P = { { 0 , 1 , 5 } , { 1 , 2 , 5 } , { 0 , 2 , 0 } };
 CHECK( collezioni::nodeCapacityFlow( 3 , P , { 0 , 1 , 0 } , 0 , 2 ) == 1 );
 }

// collezione 15-locale, 1.2: after each increase of a capacity by one, a
// single augmenting path gives the maximum flow of the network rebuilt
ADS_TEST( "collezioni-15-locale-1-2" )
{
 std::mt19937 g( 152 );
 int grew = 0;
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = draw( g , 2 , 8 );
  Arcs E = randomArcs( g , n , 40 , 5 );
  if( E.empty() ) continue;
  FlowNetwork F( n );
  for( auto [ u , v , c ] : E ) F.addEdge( u , v , c );
  long f = F.maxFlow( 0 , n - 1 );
  for( int k = 0 ; k < 5 ; ++k ) {
   auto & [ u , v , c ] = E[ draw( g , 0 , int( E.size() ) - 1 ) ];
   ++c;
   const long d = collezioni::increaseCapacity( F , 0 , n - 1 , u , v );
   f += d;
   grew += d;
   CHECK( f == maxFlowOf( n , E , 0 , n - 1 ) );
   }
  }
 CHECK( grew > 50 );
 }

// collezione 15-locale, 1.3: the network of the text has 2 paths with no
// edge in common (the two edges b - c and d - e, whose direction the text
// does not give, taken from b to c and from e to d); on random graphs the
// flow is the least number of edges whose removal separates t from s
ADS_TEST( "collezioni-15-locale-1-3" )
{
 enum { s , b , c , d , e , t };
 ListGraph<> G( 6 );
 for( auto [ u , v ] : { std::pair( s , b ) , std::pair( s , c ) ,
                         std::pair( b , d ) , std::pair( c , e ) ,
                         std::pair( d , t ) , std::pair( e , t ) ,
                         std::pair( b , c ) , std::pair( e , d ) } )
  G.insertEdge( u , v );
 CHECK( edgeDisjointPaths( G , s , t ).size() == 2 );
 std::mt19937 g( 153 );
 for( int k = 0 ; k < 200 ; ++k ) {
  const int n = draw( g , 2 , 8 );
  ListGraph<> H( n );
  Arcs E;
  for( auto [ u , v , w ] : randomArcs( g , n , 35 , 1 ) ) {
   H.insertEdge( u , v );
   E.emplace_back( u , v , w );
   }
  CHECK( long( edgeDisjointPaths( H , 0 , n - 1 ).size() ) ==
         minCutBrute( n , E , 0 , n - 1 ) );
  }
 }

// collezione 15-locale, 1.4: the assignment found respects reachability
// and the capacities ceil( n / k ), and exists iff a backtrack finds one
ADS_TEST( "collezioni-15-locale-1-4" )
{
 CHECK( collezioni::hospitals( {} , 1 ) == Ints() );
 CHECK( collezioni::hospitals( { { true } , { true } } , 1 ) ==
        Ints( { 0 , 0 } ) );
 CHECK( ! collezioni::hospitals( { { true , false } , { true , false } } ,
                                 2 ) );   // ceil( 2 / 2 ) = 1 each
 std::mt19937 g( 154 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 1 , 8 ) , k = draw( g , 1 , 4 );
  const auto A = randomBools( g , n , k , 50 );
  const long cap = ( n + k - 1 ) / k;
  const auto R = collezioni::hospitals( A , k );
  CHECK( bool( R ) == assignBrute( A , k , cap ) );
  if( R ) {
   CHECK( validAssignment( A , k , cap , *R ) );
   ++yes;
   }
  }
 CHECK( ( yes > 30 ) && ( yes < 270 ) );
 }

// collezione 15-locale, 1.5: the planes and the towers at random integer
// points, the assignment within distance d and at most L planes a tower,
// against a backtrack
ADS_TEST( "collezioni-15-locale-1-5" )
{
 CHECK( collezioni::towers( { { 0 , 0 } , { 3 , 4 } } , { { 0 , 0 } } , 5 ,
                            2 ) == Ints( { 0 , 0 } ) );   // 5 is within 5
 CHECK( ! collezioni::towers( { { 0 , 0 } , { 3 , 4 } } , { { 0 , 0 } } , 5 ,
                              1 ) );
 std::mt19937 g( 155 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< std::pair< int , int > > A( draw( g , 1 , 8 ) ) ,
                                        Q( draw( g , 1 , 4 ) );
  for( auto & p : A ) p = std::pair( draw( g , 0 , 10 ) , draw( g , 0 , 10 ) );
  for( auto & p : Q ) p = std::pair( draw( g , 0 , 10 ) , draw( g , 0 , 10 ) );
  const long d = draw( g , 2 , 8 ) , L = draw( g , 1 , 3 );
  std::vector< std::vector< bool > > M( A.size() ,
                                        std::vector< bool >( Q.size() ) );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = 0 ; j < Q.size() ; ++j )
    M[ i ][ j ] = std::hypot( A[ i ].first - Q[ j ].first ,
                              A[ i ].second - Q[ j ].second ) <= d + 1e-9;
  const auto R = collezioni::towers( A , Q , d , L );
  CHECK( bool( R ) == assignBrute( M , int( Q.size() ) , L ) );
  if( R ) {
   CHECK( validAssignment( M , int( Q.size() ) , L , *R ) );
   ++yes;
   }
  }
 CHECK( ( yes > 30 ) && ( yes < 270 ) );
 }

// collezione 15-locale, 1.6: the schedule found meets the needs, the
// limits and the unavailabilities, and exists iff a backtrack over the
// shifts finds one; on two days with at most W = 3 shifts a week, then a
// whole week of 21 shifts with W = 5
ADS_TEST( "collezioni-15-locale-1-6" )
{
 const auto valid = []( const Ints & p ,
                        const std::vector< std::vector< bool > > & U ,
                        long W , const std::vector< std::vector< bool > > &
                        w ) {
  for( std::size_t j = 0 ; j < U.size() ; ++j ) {
   long all = 0;
   for( std::size_t d = 0 ; d < p.size() / 3 ; ++d ) {
    long day = 0;
    for( std::size_t i = 3 * d ; i < 3 * d + 3 ; ++i )
     if( w[ j ][ i ] ) {
      if( U[ j ][ i ] ) return( false );
      ++day;
      }
    if( day > 2 ) return( false );
    all += day;
    }
   if( all > W ) return( false );
   }
  for( std::size_t i = 0 ; i < p.size() ; ++i ) {
   long k = 0;
   for( std::size_t j = 0 ; j < U.size() ; ++j ) k += w[ j ][ i ];
   if( k != p[ i ] ) return( false );
   }
  return( true );
  };
 std::mt19937 g( 156 );
 int yes = 0;
 for( int t = 0 ; t < 200 ; ++t ) {
  const int E = draw( g , 1 , 4 ) , S = 6;
  const Ints p = drawInts( g , S , 0 , 2 );
  const auto U = randomBools( g , E , S , 30 );
  const long W = 3;
  std::vector< std::vector< bool > > w( E , std::vector< bool >( S ) );
  // the shift i needs need more employees, to choose from j on
  std::function< bool( int , int , int ) > rec = [ & ]( int i , int j ,
                                                        int need ) {
   if( need == 0 ) return( ( i + 1 == S ) || rec( i + 1 , 0 , p[ i + 1 ] ) );
   if( j == E ) return( false );
   long all = 0 , day = 0;
   for( int x = 0 ; x < S ; ++x ) {
    all += w[ j ][ x ];
    day += w[ j ][ x ] && ( x / 3 == i / 3 );
    }
   if( ! U[ j ][ i ] && ( all < W ) && ( day < 2 ) ) {
    w[ j ][ i ] = true;
    if( rec( i , j + 1 , need - 1 ) ) return( true );
    w[ j ][ i ] = false;
    }
   return( rec( i , j + 1 , need ) );
   };
  const bool brute = rec( 0 , 0 , p[ 0 ] );
  const auto R = collezioni::mcdonalds( p , U , W );
  CHECK( bool( R ) == brute );
  if( R ) {
   CHECK( valid( p , U , W , *R ) );
   ++yes;
   }
  }
 CHECK( ( yes > 20 ) && ( yes < 180 ) );
 const Ints week( 21 , 2 );                // 42 shifts by 9 employees
 const std::vector< std::vector< bool > > free( 9 ,
                                                std::vector< bool >( 21 ) );
 const auto R = collezioni::mcdonalds( week , free , 5 );
 CHECK( R && valid( week , free , 5 , *R ) );
 CHECK( ! collezioni::mcdonalds( week , std::vector< std::vector< bool > >(
                                         8 , std::vector< bool >( 21 ) ) ,
                                 5 ) );   // 8 x 5 < 42
 }

// collezione 15-locale, 1.7: the films fit iff the minimum cut between
// the server and the super sink, by brute force, is n b
ADS_TEST( "collezioni-15-locale-1-7" )
{
 CHECK( collezioni::films( 3 , { { 0 , 1 , 4 } , { 0 , 2 , 4 } } , 0 ,
                           { 1 , 2 } , 4 ) );
 CHECK( ! collezioni::films( 2 , { { 0 , 1 , 7 } } , 0 , { 1 , 1 } , 4 ) );
 std::mt19937 g( 157 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 2 , 7 );
  const Arcs E = randomArcs( g , n , 50 , 9 );
  Ints C( draw( g , 1 , 3 ) );
  for( int & c : C ) c = draw( g , 1 , n - 1 );
  const long b = draw( g , 1 , 4 );
  Arcs N = E;
  for( int c : C ) N.emplace_back( c , n , b );
  const bool f = collezioni::films( n , E , 0 , C , b );
  CHECK( f == ( minCutBrute( n + 1 , N , 0 , n ) == long( C.size() ) * b ) );
  yes += f;
  }
 CHECK( ( yes > 20 ) && ( yes < 280 ) );
 }

// collezione 15-locale, 1.8: the evacuation is possible iff a backtrack
// finds a path from each village to a safe place, no edge used twice
ADS_TEST( "collezioni-15-locale-1-8" )
{
 std::mt19937 g( 158 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = draw( g , 2 , 6 );
  ListGraph<> G( n );
  for( auto [ u , v , w ] : randomArcs( g , n , 35 , 1 ) )
   G.insertEdge( u , v );
  std::vector< bool > village( n );
  Ints V;
  for( int v = 0 ; v < n ; ++v )
   if( ( village[ v ] = draw( g , 0 , 1 ) ) ) V.push_back( v );
  std::set< std::pair< int , int > > used;
  std::function< bool( std::size_t ) > next;
  std::function< bool( std::size_t , int ) > walk = [ & ]( std::size_t i ,
                                                           int u ) {
   if( ! village[ u ] && next( i + 1 ) ) return( true );
   for( int v : G.adj( u ) )
    if( used.insert( std::pair( u , v ) ).second ) {
     if( walk( i , v ) ) return( true );
     used.erase( std::pair( u , v ) );
     }
   return( false );
   };
  next = [ & ]( std::size_t i ) {
   return( i == V.size() || walk( i , V[ i ] ) ); };
  const bool e = collezioni::evacuation( G , village );
  CHECK( e == next( 0 ) );
  yes += e;
  }
 CHECK( ( yes > 30 ) && ( yes < 270 ) );
 }

// collezione 15-locale, 1.9: both matrices have the sums asked for and no
// negative entry, and there is none iff the sums of r and c differ
ADS_TEST( "collezioni-15-locale-1-9" )
{
 const auto ok = []( const Matrix & M , const Longs & r , const Longs & c ) {
  for( std::size_t i = 0 ; i < r.size() ; ++i ) {
   long a = 0 , b = 0;
   for( std::size_t j = 0 ; j < r.size() ; ++j ) {
    if( M[ i ][ j ] < 0 ) return( false );
    a += M[ i ][ j ];
    b += M[ j ][ i ];
    }
   if( ( a != r[ i ] ) || ( b != c[ i ] ) ) return( false );
   }
  return( true );
  };
 CHECK( collezioni::fillMatrix( { 3 , 1 } , { 2 , 2 } ) ==
        Matrix( { { 2 , 1 } , { 0 , 1 } } ) );
 CHECK( collezioni::fillMatrix( {} , {} ) == Matrix() );
 CHECK( ! collezioni::fillMatrix( { 1 } , { 2 } ) );
 std::mt19937 g( 159 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = draw( g , 1 , 7 );
  Longs r( n ) , c( n );
  for( long & x : r ) x = draw( g , 0 , 9 );
  for( long & x : c ) x = draw( g , 0 , 9 );
  if( t % 2 ) {                            // equal sums
   c.assign( n , 0 );
   for( long x : r ) c[ draw( g , 0 , n - 1 ) ] += x;
   }
  const bool eq = std::accumulate( r.begin() , r.end() , 0L ) ==
                  std::accumulate( c.begin() , c.end() , 0L );
  const auto A = collezioni::fillMatrix( r , c ) ,
             B = collezioni::fillMatrixFlow( r , c );
  CHECK( ( bool( A ) == eq ) && ( bool( B ) == eq ) );
  if( eq ) CHECK( ok( *A , r , c ) && ok( *B , r , c ) );
  }
 }

// collezione 15-locale, 1.10: the two matrices found meet the limits and
// add up to M, and there are none iff no M1 of small matrices, tried one
// by one, meets them
ADS_TEST( "collezioni-15-locale-1-10" )
{
 const auto fits = []( const Matrix & A , long l , long T ) {
  long all = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i ) {
   long r = 0;
   for( long x : A[ i ] ) { r += x; if( x < 0 ) return( false ); }
   if( r > l ) return( false );
   all += r;
   }
  for( std::size_t j = 0 ; ! A.empty() && ( j < A[ 0 ].size() ) ; ++j ) {
   long c = 0;
   for( const auto & row : A ) c += row[ j ];
   if( c > l ) return( false );
   }
  return( all <= T );
  };
 CHECK( collezioni::splitMatrix( { { 2 } } , 1 , 1 , 1 , 1 ) );
 CHECK( ! collezioni::splitMatrix( { { 3 } } , 1 , 1 , 9 , 9 ) );
 std::mt19937 g( 1510 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int m = draw( g , 1 , 2 ) , n = draw( g , 1 , 3 );
  Matrix M( m , Longs( n ) );
  for( auto & row : M )
   for( long & x : row ) x = draw( g , 0 , 2 );
  const long l1 = draw( g , 0 , 4 ) , l2 = draw( g , 0 , 4 ) ,
             t1 = draw( g , 0 , 6 ) , t2 = draw( g , 0 , 6 );
  bool brute = false;
  Matrix A( m , Longs( n , 0 ) );
  std::function< void( int ) > rec = [ & ]( int k ) {
   if( brute ) return;
   if( k == m * n ) {
    Matrix B = M;
    for( int i = 0 ; i < m ; ++i )
     for( int j = 0 ; j < n ; ++j ) B[ i ][ j ] -= A[ i ][ j ];
    brute = fits( A , l1 , t1 ) && fits( B , l2 , t2 );
    return;
    }
   for( long x = 0 ; x <= M[ k / n ][ k % n ] ; ++x ) {
    A[ k / n ][ k % n ] = x;
    rec( k + 1 );
    }
   };
  rec( 0 );
  const auto R = collezioni::splitMatrix( M , l1 , l2 , t1 , t2 );
  CHECK( bool( R ) == brute );
  if( R ) {
   ++yes;
   CHECK( fits( R->first , l1 , t1 ) && fits( R->second , l2 , t2 ) );
   for( int i = 0 ; i < m ; ++i )
    for( int j = 0 ; j < n ; ++j )
     CHECK( R->first[ i ][ j ] + R->second[ i ][ j ] == M[ i ][ j ] );
   }
  }
 CHECK( ( yes > 30 ) && ( yes < 270 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ THE VARIOUS COLLECTION --------------------------*/
/*--------------------------------------------------------------------------*/

namespace collezioni {

// collezione varie, 1.2: Blocchi 0-1

/// longestBalancedBlock(): the longest run of k zeros followed by k ones,
/// k >= 1, 0 if none: during a run of ones, the zeros of the run before
/// it and the ones so far give 2 min( zeros , ones ); Theta( n ) time,
/// O( 1 ) space
/// \pre the items are 0 or 1
int longestBalancedBlock( const Ints & S )
{
 int zeros = 0 , ones = 0 , best = 0;
 for( int x : S ) {
  ADS_PRE( ( x == 0 ) || ( x == 1 ) );
  if( x == 0 ) {
   if( ones > 0 ) zeros = ones = 0;
   ++zeros;
   }
  else {
   ++ones;
   best = std::max( best , 2 * std::min( zeros , ones ) );
   }
  }
 return( best );
 }

// collezione varie, 1.3: Distribuzione

/// preprocess(): B[ j ], the number of items of A at most j, by the counts
/// of the counting sort and their prefix sums; O( n + k ) time, O( k )
/// space
/// \pre the items of A in [ 1 , k ]
Longs preprocess( const Ints & A , int k )
{
 Longs B( k + 1 , 0 );
 for( int x : A ) {
  ADS_PRE( ( 1 <= x ) && ( x <= k ) );
  ++B[ x ];
  }
 for( int j = 1 ; j <= k ; ++j ) B[ j ] += B[ j - 1 ];
 return( B );
 }

/// rangeCount(): the number of items in [ a , b ], O( 1 )
/// \pre 1 <= a <= b <= k
long rangeCount( const Longs & B , int a , int b )
{
 ADS_PRE( ( 1 <= a ) && ( a <= b ) && ( b < int( B.size() ) ) );
 return( B[ b ] - B[ a - 1 ] );
 }

// collezione varie, 1.4: Secondo terzile

/// middleThird(): the items of A, n = 3 m distinct, larger than at least m
/// and smaller than at least m others: those between the items of the
/// places m and 2 m - 1 of the sorted A, found by the selection in linear
/// time; O( n )
/// \pre |A| a positive multiple of 3, the items distinct
Ints middleThird( const Ints & A )
{
 ADS_PRE( ! A.empty() && ( A.size() % 3 == 0 ) );
 const std::size_t m = A.size() / 3;
 Ints B = A;
 const int lo = medianOfMedians( B , m + 1 );
 B = A;
 const int hi = medianOfMedians( B , 2 * m );
 Ints out;
 for( int x : A )
  if( ( lo <= x ) && ( x <= hi ) ) out.push_back( x );
 return( out );
 }

// collezione varie, 1.5: Un algoritmo alla moda

/// mode(): the most frequent value, the largest among equals: heapsort,
/// then the runs, a later run of the same length replacing the best;
/// O( n log n ) time, O( 1 ) space
/// \pre A is not empty
int mode( Ints A )
{
 ADS_PRE( ! A.empty() );
 heapsort( A );
 int best = A[ 0 ];
 std::size_t bestCount = 0 , count = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  count = ( i > 0 ) && ( A[ i ] == A[ i - 1 ] ) ? count + 1 : 1;
  if( count >= bestCount ) {
   bestCount = count;
   best = A[ i ];
   }
  }
 return( best );
 }

// collezione varie, 1.6: Cerca la somma

/// searchSum(): whether two items in distinct places sum to x: heapsort,
/// then pairWithSum() of the library, two indices from the ends, the left
/// one up when the sum is small, the right one down when it is large;
/// O( n log n ) time, O( 1 ) space
bool searchSum( std::vector< double > A , double x )
{
 heapsort( A );
 return( pairWithSum( A , x ).has_value() );
 }

// collezione varie, 1.7: Bounding box

/// a box [ x0 , x1 ] x [ y0 , y1 ]
struct Box {
 int x0 , x1 , y0 , y1;
 bool operator==( const Box & ) const = default;
 };

/// the smallest box with a and b
Box join( const Box & a , const Box & b )
{
 return( Box{ std::min( a.x0 , b.x0 ) , std::max( a.x1 , b.x1 ) ,
              std::min( a.y0 , b.y0 ) , std::max( a.y1 , b.y1 ) } );
 }

/// a set of points with isPresent(), insert(), the bounding box of all
/// and the one of the points with x in [ x0 , x1 ]: a balanced search tree
/// on ( x , y ), each node with the box of its subtree, kept along the
/// insertions and the rotations in O( 1 ) per node; a range is the union
/// of O( log n ) subtrees and of the nodes on two paths. Each operation
/// O( log n ), the box of all O( 1 ), space O( n ). The tree here is a
/// treap (balanced in expectation by random priorities), not the
/// Red-Black tree of the text, since only the augmentation matters
class BoxSet {
 public:
 [[nodiscard]] bool isPresent( int x , int y ) const {
  const Node * u = f_root.get();
  while( u && ( u->p != std::pair( x , y ) ) )
   u = ( std::pair( x , y ) < u->p ? u->l : u->r ).get();
  return( u != nullptr );
  }

 void insert( int x , int y ) {
  if( ! isPresent( x , y ) ) insert( f_root , std::pair( x , y ) );
  }

 /// the box of all the points, nothing if there is none
 [[nodiscard]] std::optional< Box > bbox( void ) const {
  return( f_root ? std::optional< Box >( f_root->box ) : std::nullopt );
  }

 /// the box of the points with x0 <= x <= x1, nothing if there is none
 [[nodiscard]] std::optional< Box > bbox( int x0 , int x1 ) const {
  std::optional< Box > b;
  range( f_root.get() , x0 , x1 , false , false , b );
  return( b );
  }

 private:
 struct Node {
  std::pair< int , int > p;
  unsigned prio;
  Box box;
  std::unique_ptr< Node > l , r;
  };

 static void update( Node * u ) {
  u->box = Box{ u->p.first , u->p.first , u->p.second , u->p.second };
  for( Node * c : { u->l.get() , u->r.get() } )
   if( c ) u->box = join( u->box , c->box );
  }

 static void add( std::optional< Box > & b , const Box & c ) {
  b = b ? join( *b , c ) : c;
  }

 /// the points of u with x in [ x0 , x1 ]; lo (hi) if all the subtree is
 /// known to be at least x0 (at most x1), so that its box is taken whole
 static void range( const Node * u , int x0 , int x1 , bool lo , bool hi ,
                    std::optional< Box > & b ) {
  if( ! u ) return;
  if( lo && hi ) {
   add( b , u->box );
   return;
   }
  const int x = u->p.first;
  if( x < x0 ) { range( u->r.get() , x0 , x1 , lo , hi , b ); return; }
  if( x > x1 ) { range( u->l.get() , x0 , x1 , lo , hi , b ); return; }
  add( b , Box{ x , x , u->p.second , u->p.second } );
  range( u->l.get() , x0 , x1 , lo , true , b );
  range( u->r.get() , x0 , x1 , true , hi , b );
  }

 static void rotate( std::unique_ptr< Node > & u , bool left ) {
  std::unique_ptr< Node > c = std::move( left ? u->r : u->l );
  ( left ? u->r : u->l ) = std::move( left ? c->l : c->r );
  update( u.get() );
  ( left ? c->l : c->r ) = std::move( u );
  u = std::move( c );
  update( u.get() );
  }

 void insert( std::unique_ptr< Node > & u , std::pair< int , int > p ) {
  if( ! u ) {
   u.reset( new Node{ p , unsigned( f_rng() ) , Box{} , nullptr ,
                      nullptr } );
   update( u.get() );
   return;
   }
  const bool right = u->p < p;
  insert( right ? u->r : u->l , p );
  update( u.get() );
  if( ( right ? u->r : u->l )->prio > u->prio ) rotate( u , right );
  }

 std::unique_ptr< Node > f_root;
 std::mt19937 f_rng{ 17 };
 };

}  // end( namespace collezioni )

/*--------------------------------------------------------------------------*/

// collezione varie, 1.2: the scan against all the starts and the k
ADS_TEST( "collezioni-varie-1-2" )
{
 CHECK( collezioni::longestBalancedBlock( Ints{ 1 , 1 , 0 } ) == 0 );
 CHECK( collezioni::longestBalancedBlock( Ints{ 0 } ) == 0 );
 CHECK( collezioni::longestBalancedBlock( Ints{ 0 , 0 , 0 , 1 , 1 , 0 , 1 } )
        == 4 );
 std::mt19937 g( 162 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints S = drawInts( g , draw( g , 1 , 25 ) , 0 , 1 );
  const int n = int( S.size() );
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int k = 1 ; i + 2 * k <= n ; ++k ) {
    bool ok = true;
    for( int h = 0 ; h < k ; ++h )
     ok = ok && ( S[ i + h ] == 0 ) && ( S[ i + k + h ] == 1 );
    if( ok ) best = std::max( best , 2 * k );
    }
  CHECK( collezioni::longestBalancedBlock( S ) == best );
  }
 }

// collezione varie, 1.3: the counts in each range against count_if, and
// B against the places of the array sorted by countingSort() of ADS
ADS_TEST( "collezioni-varie-1-3" )
{
 std::mt19937 g( 163 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int k = draw( g , 1 , 12 );
  const Ints A = drawInts( g , draw( g , 0 , 40 ) , 1 , k );
  const Longs B = collezioni::preprocess( A , k );
  Ints sorted = A;
  countingSort( sorted , std::size_t( k + 1 ) );
  for( int j = 0 ; j <= k ; ++j )
   CHECK( B[ j ] == std::upper_bound( sorted.begin() , sorted.end() , j ) -
                    sorted.begin() );
  for( int a = 1 ; a <= k ; ++a )
   for( int b = a ; b <= k ; ++b )
    CHECK( collezioni::rangeCount( B , a , b ) ==
           std::count_if( A.begin() , A.end() , [ & ]( int x ) {
            return( ( a <= x ) && ( x <= b ) ); } ) );
  }
 }

// collezione varie, 1.4: the middle third against the places m to 2 m - 1
// of the sorted array
ADS_TEST( "collezioni-varie-1-4" )
{
 CHECK( collezioni::middleThird( Ints{ 3 , 1 , 2 } ) == Ints{ 2 } );
 std::mt19937 g( 164 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int m = draw( g , 1 , 20 );
  Ints A = drawSortedDistinct( g , 3 * m , -1000 , 1000 );
  const Ints mid( A.begin() + m , A.begin() + 2 * m );
  std::shuffle( A.begin() , A.end() , g );
  Ints R = collezioni::middleThird( A );
  std::sort( R.begin() , R.end() );
  CHECK( R == mid );
  }
 }

// collezione varie, 1.5: the example of the text, 28, and a map of the
// frequencies, the largest value among the most frequent
ADS_TEST( "collezioni-varie-1-5" )
{
 CHECK( collezioni::mode( { 1 , 2 , 7 , 12 , 3 , 6 , 7 , 18 , 28 , 11 , 28 ,
                            11 , 12 , 12 , 14 , 3 , 5 , 8 , 9 , 2 , 38 ,
                            28 } ) == 28 );
 CHECK( collezioni::mode( { -4 } ) == -4 );
 CHECK( collezioni::mode( { 2 , 1 , 3 } ) == 3 );
 std::mt19937 g( 165 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints A = drawInts( g , draw( g , 1 , 30 ) , -5 , 5 );
  std::map< int , int > f;
  for( int x : A ) ++f[ x ];
  std::pair< int , int > best( 0 , 0 );
  for( auto [ x , c ] : f ) best = std::max( best , std::pair( c , x ) );
  CHECK( collezioni::mode( A ) == best.second );
  }
 }

// collezione varie, 1.6: the two indices against all the pairs, on
// integer values so that the sums are exact
ADS_TEST( "collezioni-varie-1-6" )
{
 CHECK( ! collezioni::searchSum( {} , 0 ) );
 CHECK( ! collezioni::searchSum( { 2 } , 4 ) );   // distinct places
 CHECK( collezioni::searchSum( { 2 , 2 } , 4 ) );
 std::mt19937 g( 166 );
 int yes = 0;
 for( int t = 0 ; t < 1000 ; ++t ) {
  const Ints I = drawInts( g , draw( g , 0 , 20 ) , -20 , 20 );
  const std::vector< double > A( I.begin() , I.end() );
  const double x = draw( g , -30 , 30 );
  bool any = false;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; j < A.size() ; ++j )
    any = any || ( A[ i ] + A[ j ] == x );
  CHECK( collezioni::searchSum( A , x ) == any );
  yes += any;
  }
 CHECK( ( yes > 100 ) && ( yes < 900 ) );
 }

// collezione varie, 1.7: random insertions and queries against a scan of
// a vector of the points
ADS_TEST( "collezioni-varie-1-7" )
{
 collezioni::BoxSet B;
 CHECK( ! B.bbox() && ! B.bbox( -9 , 9 ) && ! B.isPresent( 0 , 0 ) );
 std::mt19937 g( 167 );
 std::vector< std::pair< int , int > > P;
 for( int t = 0 ; t < 3000 ; ++t ) {
  const int x = draw( g , -50 , 50 ) , y = draw( g , -50 , 50 );
  const bool in = std::find( P.begin() , P.end() , std::pair( x , y ) ) !=
                  P.end();
  CHECK( B.isPresent( x , y ) == in );
  B.insert( x , y );
  if( ! in ) P.emplace_back( x , y );
  int x0 = draw( g , -60 , 60 ) , x1 = draw( g , -60 , 60 );
  if( x0 > x1 ) std::swap( x0 , x1 );
  std::optional< collezioni::Box > all , part;
  for( auto [ a , b ] : P ) {
   const collezioni::Box c{ a , a , b , b };
   all = all ? collezioni::join( *all , c ) : c;
   if( ( x0 <= a ) && ( a <= x1 ) )
    part = part ? collezioni::join( *part , c ) : c;
   }
  CHECK( ( B.bbox() == all ) && ( B.bbox( x0 , x1 ) == part ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File exam_collezionib.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
