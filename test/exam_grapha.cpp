/*--------------------------------------------------------------------------*/
/*-------------------------- File exam_grapha.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems 1 to 24 of the collection Esercizi Grafi: each
 * problem of code or of computation is solved as its official solution
 * says, by a function of the namespace graph or by the function of ADS that
 * already solves it, and checked on the examples of the statement, against
 * an independent oracle on small random instances (a brute force, an
 * enumeration of the paths, a transitive closure, Floyd-Warshall, a
 * merge-find set, a simulation) and on the edge cases. The tests are named
 * after the problems, graph-N, or graph-N-K for the part K of the problem
 * N; where the official solution has an error, the function is right and
 * its comment says how it differs from the text.
 *
 * \see A. Montresor, Esercizi Grafi, Soluzioni, Algoritmicamente, 2026
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 9 and 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
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

#include "ADSTest.h"

#include "ADS/complexity/EasyCases.h"
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*--------------------------------- COMMON ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace graph {

using Ints = std::vector< int >;
using Matrix = std::vector< std::vector< int > >;
using BoolMatrix = std::vector< std::vector< bool > >;

/// the four moves of a cell, as differences of the row and of the column
inline constexpr int DR[ 4 ] = { -1 , 0 , 1 , 0 };
inline constexpr int DC[ 4 ] = { 0 , -1 , 0 , 1 };

/// whether ( r , c ) is a cell of an n x n matrix
inline bool inside( int n , int r , int c )
{
 return( ( 0 <= r ) && ( r < n ) && ( 0 <= c ) && ( c < n ) );
 }

/// whether M is square
template< class T >
bool square( const std::vector< std::vector< T > > & M )
{
 for( const auto & row : M )
  if( row.size() != M.size() )
   return( false );
 return( true );
 }

/// whether a visit of g from v reaches every node: the ccdfs() of the book
/// followed by the check that no node is left unvisited
/// \pre v is a node
template< Graph G >
bool allReached( const G & g , int v )
{
 ADS_PRE( g.hasNode( v ) );
 std::vector< bool > visited( g.bound() , false );
 auto none = []( auto... ) {};
 dfs( g , v , visited , none , none , none );
 for( int u : g.V() )
  if( ! visited[ u ] )
   return( false );
 return( true );
 }

}  // end( namespace graph )

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using graph::BoolMatrix;
using graph::Ints;
using graph::Matrix;
using Edges = std::vector< std::pair< int , int > >;

/// an integer in [ lo , hi ]
int uniform( std::mt19937 & r , int lo , int hi )
{
 return( std::uniform_int_distribution< int >( lo , hi )( r ) );
 }

/// a real in [ 0 , 1 )
double real( std::mt19937 & r )
{
 return( std::uniform_real_distribution< double >( 0 , 1 )( r ) );
 }

/// the graph of n nodes and of the edges E
template< class G = ListGraph<> >
G make( int n , const Edges & E , bool directed )
{
 G g( n , directed );
 for( auto [ u , v ] : E )
  g.insertEdge( u , v );
 return( g );
 }

/// a random graph of n nodes, each edge with probability p
ListGraph<> randomGraph( std::mt19937 & r , int n , double p , bool directed )
{
 ListGraph<> g( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && ( real( r ) < p ) )
    g.insertEdge( u , v );
 return( g );
 }

/// a random acyclic graph, its edges from a smaller to a larger place of a
/// random permutation
ListGraph<> randomDag( std::mt19937 & r , int n , double p )
{
 Ints pi( n );
 for( int i = 0 ; i < n ; ++i )
  pi[ i ] = i;
 std::shuffle( pi.begin() , pi.end() , r );
 ListGraph<> g( n , true );
 for( int i = 0 ; i < n ; ++i )
  for( int j = i + 1 ; j < n ; ++j )
   if( real( r ) < p )
    g.insertEdge( pi[ i ] , pi[ j ] );
 return( g );
 }

/// a random graph of n nodes, each edge with probability p and a weight in
/// [ lo , hi ]
ListGraph< int > randomWeighted( std::mt19937 & r , int n , double p ,
                                 bool directed , int lo , int hi )
{
 ListGraph< int > g( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && ( real( r ) < p ) )
    g.insertEdge( u , v , uniform( r , lo , hi ) );
 return( g );
 }

/// a random unoriented tree of n nodes, each node joined to an earlier one
/// with fewer than k adjacent nodes
/// \pre k >= 2
ListGraph<> randomTree( std::mt19937 & r , int n , int k )
{
 ListGraph<> g( n , false );
 for( int v = 1 ; v < n ; ++v )
  for( ; ; ) {
   const int u = uniform( r , 0 , v - 1 );
   if( int( g.adj( u ).size() ) < k ) {
    g.insertEdge( u , v );
    break;
    }
   }
 return( g );
 }

/// a random connected unoriented graph: a random tree and more edges, each
/// with probability p
ListGraph<> randomConnected( std::mt19937 & r , int n , double p )
{
 auto g = randomTree( r , n , std::max( n , 2 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( ! g.hasEdge( u , v ) && ( real( r ) < p ) )
    g.insertEdge( u , v );
 return( g );
 }

/// a random tournament: one edge between each two teams, from the winner
ListGraph<> randomTournament( std::mt19937 & r , int n )
{
 ListGraph<> g( n , true );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( real( r ) < 0.5 )
    g.insertEdge( u , v );
   else
    g.insertEdge( v , u );
 return( g );
 }

/// R[ u ][ v ]: a path from u to v of at least one edge, or of any length
/// if reflexive
template< Graph G >
BoolMatrix closure( const G & g , bool reflexive )
{
 const int n = g.bound();
 BoolMatrix R( n , std::vector< bool >( n , false ) );
 for( int u : g.V() ) {
  R[ u ][ u ] = reflexive;
  g.forEachAdj( u , [ & ]( int v ) { R[ u ][ v ] = true; } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   if( R[ i ][ k ] )
    for( int j = 0 ; j < n ; ++j )
     if( R[ k ][ j ] )
      R[ i ][ j ] = true;
 return( R );
 }

/// the distances with unit weights, by Floyd-Warshall
template< Graph G >
Matrix unitDistances( const G & g )
{
 const int n = g.bound();
 Matrix d( n , Ints( n , INF ) );
 for( int u : g.V() ) {
  d[ u ][ u ] = 0;
  g.forEachAdj( u , [ & ]( int v ) { d[ u ][ v ] = 1; } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ( d[ i ][ k ] < INF ) && ( d[ k ][ j ] < INF ) )
     d[ i ][ j ] = std::min( d[ i ][ j ] , d[ i ][ k ] + d[ k ][ j ] );
 return( d );
 }

/// f( P ) on every simple path P from s, the path of s alone included
template< Graph G , class F >
void simplePaths( const G & g , int s , F && f )
{
 Ints P = { s };
 std::vector< bool > on( g.bound() , false );
 on[ s ] = true;
 auto rec = [ & ]( auto & self , int u ) -> void {
  f( std::as_const( P ) );
  for( int v : g.adj( u ) )
   if( ! on[ v ] ) {
    on[ v ] = true;
    P.push_back( v );
    self( self , v );
    P.pop_back();
    on[ v ] = false;
    }
  };
 rec( rec , s );
 }

/// whether the nodes of g are all in one component (g unoriented)
template< Graph G >
bool connected( const G & g )
{
 const auto id = cc( g );
 for( int u : g.V() )
  if( id[ u ] != 1 )
   return( false );
 return( true );
 }

/// the unoriented edges of g, as ( smaller , larger ) pairs
template< Graph G >
std::set< std::pair< int , int > > edgeSet( const G & g )
{
 std::set< std::pair< int , int > > E;
 for( int u : g.V() )
  for( int v : g.adj( u ) )
   E.insert( { std::min( u , v ) , std::max( u , v ) } );
 return( E );
 }

/// a random n x n matrix of values in [ lo , hi ]
Matrix randomMatrix( std::mt19937 & r , int n , int lo , int hi )
{
 Matrix M( n , Ints( n ) );
 for( auto & row : M )
  for( int & x : row )
   x = uniform( r , lo , hi );
 return( M );
 }

/// a random n x n matrix, each item true with probability p
BoolMatrix randomBools( std::mt19937 & r , int n , double p )
{
 BoolMatrix M( n , std::vector< bool >( n ) );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   M[ i ][ j ] = real( r ) < p;
 return( M );
 }

/// whether s can be the last team standing of the tournament g, by trying
/// every match between two teams still in, on the sets of teams
bool canWinBrute( const ListGraph<> & g , int s )
{
 const int n = g.bound();
 std::vector< signed char > T( std::size_t( 1 ) << n , -1 );
 auto rec = [ & ]( auto & self , unsigned A ) -> bool {
  if( A == ( 1u << s ) )
   return( true );
  if( T[ A ] < 0 ) {
   bool ok = false;
   for( int u = 0 ; ( u < n ) && ! ok ; ++u )
    if( ( A >> u ) & 1u )
     for( int v : g.adj( u ) )
      if( ( v != s ) && ( ( A >> v ) & 1u ) &&
          self( self , A & ~( 1u << v ) ) ) {
       ok = true;
       break;
       }
   T[ A ] = ok ? 1 : 0;
   }
  return( T[ A ] == 1 );
  };
 return( rec( rec , ( 1u << n ) - 1 ) );
 }

/// the matrix of adjacency of g, for twoColoring()
template< Graph G >
Adjacency adjacency( const G & g )
{
 Adjacency A( g.bound() , std::vector< bool >( g.bound() , false ) );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { A[ u ][ v ] = true; } );
 return( A );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- PROBLEMS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*-------------- 1. 2011-05-02, Conteggio dei cammini minimi ---------------*/

namespace graph {

/// countShortestPaths(): the number of distinct shortest paths, in edges,
/// from s to each node, 0 if it is unreachable and 1 for s (the empty
/// path): countShortestPaths() of the library, a breadth first visit in
/// which a node inherits the counts of the nodes one level before it with
/// an edge to it; Theta( n + m ) time, Theta( n ) space
using ADS::countShortestPaths;

}  // end( namespace graph )

ADS_TEST( "graph-1" )
{
 // the figure: A -> B , C , D and B , C , D -> E
 const auto f = make( 5 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 1 , 4 } ,
                            { 2 , 4 } , { 3 , 4 } } , true );
 CHECK( graph::countShortestPaths( f , 0 ) ==
        std::vector< std::uint64_t >( { 1 , 1 , 1 , 1 , 3 } ) );
 // the simple paths as long as the distance, enumerated
 std::mt19937 r( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomGraph( r , n , real( r ) , t % 2 == 0 );
  const int s = uniform( r , 0 , n - 1 );
  const auto d = distance( g , s ).first;
  std::vector< std::uint64_t > ref( n , 0 );
  simplePaths( g , s , [ & ]( const Ints & P ) {
   if( int( P.size() ) - 1 == d[ P.back() ] )
    ++ref[ P.back() ];
   } );
  CHECK( graph::countShortestPaths( g , s ) == ref );
  }
 // one node, and a chain of 40 diamonds, with 2^40 shortest paths
 CHECK( graph::countShortestPaths( ListGraph<>( 1 ) , 0 ) ==
        std::vector< std::uint64_t >( { 1 } ) );
 ListGraph<> h( 1 + 3 * 40 , true );
 for( int i = 0 ; i < 40 ; ++i ) {
  h.insertEdge( 3 * i , 3 * i + 1 );
  h.insertEdge( 3 * i , 3 * i + 2 );
  h.insertEdge( 3 * i + 1 , 3 * i + 3 );
  h.insertEdge( 3 * i + 2 , 3 * i + 3 );
  }
 CHECK( graph::countShortestPaths( h , 0 )[ 3 * 40 ] ==
        ( std::uint64_t( 1 ) << 40 ) );
 }

/*------------------------- 2. 2011-06-06, Mindist -------------------------*/

// mindist(), the distance in edges from the set V1 to the set V2, 0 if they
// meet and INF if no node of V2 is reachable from V1: setDistance() of the
// library, a breadth first visit started from all the nodes of V1 at once,
// stopped at the first node of V2 that it discovers, O( n + m )
ADS_TEST( "graph-2" )
{
 // one breadth first visit from each node of V1
 std::mt19937 r( 2 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , 0.4 * real( r ) , t % 2 == 0 );
  std::vector< bool > in1( n ) , in2( n );
  for( int u = 0 ; u < n ; ++u ) {
   in1[ u ] = real( r ) < 0.3;
   in2[ u ] = real( r ) < 0.3;
   }
  int ref = INF;
  for( int u = 0 ; u < n ; ++u )
   if( in1[ u ] ) {
    const auto d = distance( g , u ).first;
    for( int v = 0 ; v < n ; ++v )
     if( in2[ v ] )
      ref = std::min( ref , d[ v ] );
    }
  CHECK( setDistance( g , in1 , in2 ) == ref );
  }
 // V1 empty, a long path both ways, a common node
 ListGraph<> p( 1000 , true );
 for( int u = 0 ; u + 1 < 1000 ; ++u )
  p.insertEdge( u , u + 1 );
 std::vector< bool > a( 1000 , false ) , b( 1000 , false );
 b[ 999 ] = true;
 CHECK( setDistance( p , a , b ) == INF );
 a[ 0 ] = true;
 CHECK( setDistance( p , a , b ) == 999 );
 CHECK( setDistance( p , b , a ) == INF );
 b[ 0 ] = true;
 CHECK( setDistance( p , a , b ) == 0 );
 }

/*-------------------------- 3. 2012-05-03, Roma ---------------------------*/

namespace graph {

/// isRoma(): whether every node reaches v, i.e., whether a visit of the
/// transposed graph from v reaches every node, O( n + m )
/// \pre v is a node
template< Graph G >
bool isRoma( const G & g , int v )
{
 ADS_PRE( g.hasNode( v ) );
 return( allReached( transpose( g ) , v ) );
 }

/// roma(): whether some node is reached from every node; if any node is,
/// so is the last one closed by a depth first visit of the whole transposed
/// graph, the first of its topsort(), O( n + m )
/// \pre g has a node
template< Graph G >
bool roma( const G & g )
{
 ADS_PRE( g.n() > 0 );
 const auto T = transpose( g );
 return( allReached( T , topsort( T ).front() ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-3-1" )
{
 // a visit from every node
 std::mt19937 r( 3 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , real( r ) , true );
  for( int v = 0 ; v < n ; ++v ) {
   bool ref = true;
   for( int w = 0 ; w < n ; ++w )
    ref = ref && ( distance( g , w ).first[ v ] != INF );
   CHECK( graph::isRoma( g , v ) == ref );
   }
  }
 CHECK( graph::isRoma( ListGraph<>( 1 ) , 0 ) );
 CHECK( ! graph::isRoma( ListGraph<>( 2 ) , 0 ) );
 const auto s = make( 4 , { { 1 , 0 } , { 2 , 0 } , { 3 , 0 } } , true );
 CHECK( graph::isRoma( s , 0 ) && ! graph::isRoma( s , 1 ) );
 }

ADS_TEST( "graph-3-2" )
{
 // the transitive closure, and the condensation with one sink
 std::mt19937 r( 4 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , 0.5 * real( r ) , true );
  const auto R = closure( g , true );
  bool ref = false;
  for( int v = 0 ; v < n ; ++v ) {
   bool all = true;
   for( int w = 0 ; w < n ; ++w )
    all = all && R[ w ][ v ];
   ref = ref || all;
   }
  const auto id = scc( g );
  std::set< int > comps , left;
  for( int u = 0 ; u < n ; ++u ) {
   comps.insert( id[ u ] );
   for( int v : g.adj( u ) )
    if( id[ v ] != id[ u ] )
     left.insert( id[ u ] );
   }
  CHECK( ref == ( comps.size() - left.size() == 1 ) );
  CHECK( graph::roma( g ) == ref );
  }
 CHECK( graph::roma( ListGraph<>( 1 ) ) );
 CHECK( ! graph::roma( ListGraph<>( 2 ) ) );
 CHECK( graph::roma( make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } ,
                           true ) ) );
 }

/*------------------------ 4. 2012-06-18, Qcammini -------------------------*/

namespace graph {

/// qcammini(): D[ u ][ v ], the least weight of a path from u to v through
/// q in an unoriented graph with positive weights, which is d[ u ] + d[ v ]
/// for the distances d from q given by one run of Dijkstra, the graph
/// being unoriented; O( n^2 ) time, infinity() if u or v cannot reach q
/// \pre q is a node and g is unoriented
template< Graph G >
auto qPaths( const G & g , int q )
{
 using W = typename G::Weight;
 ADS_PRE( g.hasNode( q ) && ! g.directed() );
 const auto d = dijkstra( g , q ).d;
 const W inf = infinity< W >();
 std::vector< std::vector< W > > D( g.bound() ,
                                    std::vector< W >( g.bound() , inf ) );
 for( int u : g.V() )
  for( int v : g.V() )
   if( ( d[ u ] != inf ) && ( d[ v ] != inf ) )
    D[ u ][ v ] = d[ u ] + d[ v ];
 return( D );
 }

}  // end( namespace graph )

ADS_TEST( "graph-4" )
{
 // Floyd-Warshall, d( u , q ) + d( q , v )
 std::mt19937 r( 5 );
 const int inf = infinity< int >();
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomWeighted( r , n , real( r ) , false , 1 , 20 );
  const int q = uniform( r , 0 , n - 1 );
  const auto F = floydWarshall( g ).d;
  const auto D = graph::qPaths( g , q );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    CHECK( D[ u ][ v ] == ( ( F[ u ][ q ] == inf ) || ( F[ q ][ v ] == inf )
                            ? inf : F[ u ][ q ] + F[ q ][ v ] ) );
  }
 // one node, and large weights
 CHECK( graph::qPaths( ListGraph< int >( 1 , false ) , 0 )[ 0 ][ 0 ] == 0 );
 ListGraph< int > h( 3 , false );
 h.insertEdge( 0 , 1 , 100000000 );
 h.insertEdge( 1 , 2 , 100000000 );
 CHECK( graph::qPaths( h , 1 )[ 0 ][ 2 ] == 200000000 );
 CHECK( graph::qPaths( h , 2 )[ 0 ][ 0 ] == 400000000 );
 }

/*------------------------ 5. 2013-01-28, Monotono -------------------------*/

namespace graph {

/// monotono(): a path from s to d, in order, along which the weights w of
/// the nodes strictly increase, or nothing; the depth first visit of the
/// text, which follows an edge [ u , v ] only if w( u ) < w( v ) and builds
/// the path backwards as the recursion returns (the text prints it from d
/// to s), O( n + m )
/// \pre s and d are nodes, w has a place per name
template< Graph G >
std::optional< Ints > monotone( const G & g , const std::vector< double > & w ,
                                int s , int d )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( d ) &&
          ( int( w.size() ) == g.bound() ) );
 std::vector< bool > mark( g.bound() , false );
 Ints P;
 auto rec = [ & ]( auto & self , int u ) -> bool {
  mark[ u ] = true;
  if( u == d ) {
   P.push_back( d );
   return( true );
   }
  for( int v : g.adj( u ) )
   if( ! mark[ v ] && ( w[ u ] < w[ v ] ) && self( self , v ) ) {
    P.push_back( u );
    return( true );
    }
  return( false );
  };
 if( ! rec( rec , s ) )
  return( std::nullopt );
 std::reverse( P.begin() , P.end() );
 return( P );
 }

}  // end( namespace graph )

ADS_TEST( "graph-5-2" )
{
 // reachability on the graph of the edges oriented upwards, and the path
 std::mt19937 r( 6 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , real( r ) , false );
  std::vector< double > w( n );
  for( double & x : w )
   x = uniform( r , -3 , 3 ) / 2.0;
  const int s = uniform( r , 0 , n - 1 ) , d = uniform( r , 0 , n - 1 );
  ListGraph<> up( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v : g.adj( u ) )
    if( w[ u ] < w[ v ] )
     up.insertEdge( u , v );
  const bool ref = distance( up , s ).first[ d ] != INF;
  const auto P = graph::monotone( g , w , s , d );
  CHECK( P.has_value() == ref );
  if( P ) {
   CHECK( ( P->front() == s ) && ( P->back() == d ) );
   for( std::size_t i = 1 ; i < P->size() ; ++i )
    CHECK( g.hasEdge( ( *P )[ i - 1 ] , ( *P )[ i ] ) &&
           ( w[ ( *P )[ i - 1 ] ] < w[ ( *P )[ i ] ] ) );
   }
  }
 // s = d, and all the weights equal
 const auto c = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 CHECK( graph::monotone( c , { 1 , 1 , 1 } , 1 , 1 ) == Ints( { 1 } ) );
 CHECK( ! graph::monotone( c , { 1 , 1 , 1 } , 0 , 2 ) );
 CHECK( graph::monotone( c , { -1 , 0 , 1 } , 0 , 2 ) ==
        Ints( { 0 , 1 , 2 } ) );
 }

/*---------------------- 6. 2013-05-03, Grid distance ----------------------*/

namespace graph {

/// whether the border of the square matrix G is all obstacles (value 1)
inline bool walled( const Matrix & G )
{
 const int n = int( G.size() );
 for( int i = 0 ; i < n ; ++i )
  if( ( G[ 0 ][ i ] != 1 ) || ( G[ n - 1 ][ i ] != 1 ) ||
      ( G[ i ][ 0 ] != 1 ) || ( G[ i ][ n - 1 ] != 1 ) )
   return( false );
 return( true );
 }

/// gridDistance(): the least number of moves from ( 1 , 1 ) to ( n , n ) in
/// the ( n + 2 ) x ( n + 2 ) grid G, 0 free and 1 obstacle, INF if there is
/// no way: gridDistance() of the library, a breadth first visit of the
/// implicit graph of the free cells that stops when it takes ( n , n ) out
/// of the queue, Theta( n^2 )
/// \pre n > 1, G is ( n + 2 ) x ( n + 2 ) with obstacles on the border and
///      ( 1 , 1 ), ( n , n ) free
inline int gridDistance( const Matrix & G , int n )
{
 ADS_PRE( ( n > 1 ) && ( int( G.size() ) == n + 2 ) && square( G ) &&
          walled( G ) && ( G[ 1 ][ 1 ] == 0 ) && ( G[ n ][ n ] == 0 ) );
 return( ADS::gridDistance( n + 2 , n + 2 , GridCell( 1 , 1 ) ,
                            GridCell( n , n ) , [ & ]( GridCell x ) {
                             return( G[ x.first ][ x.second ] == 0 ); } ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-6" )
{
 // breadth first visit of the explicit graph of the free cells
 std::mt19937 r( 7 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 2 , 7 );
  Matrix G( n + 2 , Ints( n + 2 , 1 ) );
  for( int i = 1 ; i <= n ; ++i )
   for( int j = 1 ; j <= n ; ++j )
    G[ i ][ j ] = real( r ) < 0.3 ? 1 : 0;
  G[ 1 ][ 1 ] = G[ n ][ n ] = 0;
  ListGraph<> h( n * n , false );
  for( int i = 1 ; i <= n ; ++i )
   for( int j = 1 ; j <= n ; ++j )
    if( G[ i ][ j ] == 0 ) {
     const int u = ( i - 1 ) * n + ( j - 1 );
     if( G[ i ][ j + 1 ] == 0 )
      h.insertEdge( u , u + 1 );
     if( G[ i + 1 ][ j ] == 0 )
      h.insertEdge( u , u + n );
     }
  CHECK( graph::gridDistance( G , n ) ==
         distance( h , 0 ).first[ n * n - 1 ] );
  }
 Matrix G( 4 , Ints( 4 , 1 ) );
 G[ 1 ][ 1 ] = G[ 1 ][ 2 ] = G[ 2 ][ 1 ] = G[ 2 ][ 2 ] = 0;
 CHECK( graph::gridDistance( G , 2 ) == 2 );
 G[ 1 ][ 2 ] = G[ 2 ][ 1 ] = 1;
 CHECK( graph::gridDistance( G , 2 ) == INF );
 Matrix L( 42 , Ints( 42 , 1 ) );
 for( int i = 1 ; i <= 40 ; ++i )
  for( int j = 1 ; j <= 40 ; ++j )
   L[ i ][ j ] = 0;
 CHECK( graph::gridDistance( L , 40 ) == 78 );
 }

/*----------------------- 7. 2013-05-31, Connected k -----------------------*/

namespace graph {

/// connectedK(): whether the component of s has at most k nodes; a depth
/// first visit from s that counts the nodes it reaches and stops when they
/// are more than k, O( n + min( m , k^2 ) ). The text passes k - 1 to each
/// recursive call, a bound on the depth and not on the nodes (a star of
/// three leaves passes with k = 2): here the counter is shared by the visit
/// \pre s is a node, k > 0
template< Graph G >
bool connectedK( const G & g , int s , int k )
{
 ADS_PRE( g.hasNode( s ) && ( k > 0 ) );
 std::vector< bool > visited( g.bound() , false );
 int count = 0;
 auto rec = [ & ]( auto & self , int u ) -> bool {
  visited[ u ] = true;
  if( ++count > k )
   return( false );
  for( int v : g.adj( u ) )
   if( ! visited[ v ] && ! self( self , v ) )
    return( false );
  return( true );
  };
 return( rec( rec , s ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-7" )
{
 // a star of three leaves has 4 nodes, more than 2
 const auto star = make( 4 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } } , false );
 CHECK( ! graph::connectedK( star , 0 , 2 ) );
 CHECK( ! graph::connectedK( star , 1 , 3 ) );
 CHECK( graph::connectedK( star , 2 , 4 ) );
 // the size of the component from cc()
 std::mt19937 r( 8 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 9 );
  const auto g = randomGraph( r , n , 0.4 * real( r ) , false );
  const auto id = cc( g );
  const int s = uniform( r , 0 , n - 1 );
  const int size = int( std::count( id.begin() , id.end() , id[ s ] ) );
  for( int k = 1 ; k <= n + 1 ; ++k )
   CHECK( graph::connectedK( g , s , k ) == ( size <= k ) );
  }
 CHECK( graph::connectedK( ListGraph<>( 1 , false ) , 0 , 1 ) );
 }

/*-------------------------- 8. 2013-09-02, Count --------------------------*/

namespace graph {

/// count(): the number of nodes at distance at most d from r, r included,
/// from the distances of a breadth first visit, O( n + m )
/// \pre r is a node, d > 0
template< Graph G >
int countWithin( const G & g , int r , int d )
{
 ADS_PRE( g.hasNode( r ) && ( d > 0 ) );
 const auto dist = distance( g , r ).first;
 int count = 0;
 for( int u : g.V() )
  if( dist[ u ] <= d )
   ++count;
 return( count );
 }

}  // end( namespace graph )

ADS_TEST( "graph-8" )
{
 // the row of r in Floyd-Warshall with unit weights
 std::mt19937 r( 9 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , real( r ) , false );
  const auto D = unitDistances( g );
  const int s = uniform( r , 0 , n - 1 );
  for( int d = 1 ; d <= n ; ++d ) {
   int ref = 0;
   for( int v = 0 ; v < n ; ++v )
    if( D[ s ][ v ] <= d )
     ++ref;
   CHECK( graph::countWithin( g , s , d ) == ref );
   }
  }
 CHECK( graph::countWithin( ListGraph<>( 1 , false ) , 0 , 5 ) == 1 );
 ListGraph<> p( 100 , false );
 for( int u = 0 ; u + 1 < 100 ; ++u )
  p.insertEdge( u , u + 1 );
 CHECK( graph::countWithin( p , 0 , 10 ) == 11 );
 CHECK( graph::countWithin( p , 50 , 10 ) == 21 );
 CHECK( graph::countWithin( p , 0 , 1000 ) == 100 );
 }

/*------------------- 9. 2014-01-07, Mostri nelle stanze -------------------*/

namespace graph {

/// the length of a shortest path from s to d through rooms with no monster
/// (M[ u ] true if the room u has one), INF if there is none; a breadth
/// first visit that never enters a room with a monster, O( n + m ). The
/// text gives a distance to such rooms without leaving them, and so a
/// finite one when d has a monster: here a monster at an end means INF
/// \pre s and d are nodes, M has a place per name
template< Graph G >
int noMonsters( const G & g , const std::vector< bool > & M , int s , int d )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( d ) &&
          ( int( M.size() ) == g.bound() ) );
 if( M[ s ] || M[ d ] )
  return( INF );
 Ints dist( g.bound() , INF );
 std::deque< int > Q = { s };
 dist[ s ] = 0;
 while( ! Q.empty() ) {
  const int u = Q.front();
  Q.pop_front();
  for( int v : g.adj( u ) )
   if( ( dist[ v ] == INF ) && ! M[ v ] ) {
    dist[ v ] = dist[ u ] + 1;
    Q.push_back( v );
    }
  }
 return( dist[ d ] );
 }

/// the least number of rooms with a monster on a path from s to d, its ends
/// included, INF if d is unreachable; Dijkstra on the edges ( u , v )
/// weighted 1 if v has a monster and 0 otherwise, plus 1 if s has one,
/// O( n^2 )
/// \pre s and d are nodes, M has a place per name
template< Graph G >
int fewestMonsters( const G & g , const std::vector< bool > & M , int s ,
                    int d )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( d ) &&
          ( int( M.size() ) == g.bound() ) );
 ListGraph< int > H( g.bound() , true );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   H.insertEdge( u , v , M[ v ] ? 1 : 0 );
   } );
 const int x = dijkstra( H , s ).d[ d ];
 return( x == INF ? INF : x + ( M[ s ] ? 1 : 0 ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-9-1" )
{
 // the graph without the rooms with a monster
 std::mt19937 r( 10 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , real( r ) , false );
  std::vector< bool > M( n );
  for( int u = 0 ; u < n ; ++u )
   M[ u ] = real( r ) < 0.3;
  const int s = uniform( r , 0 , n - 1 ) , d = uniform( r , 0 , n - 1 );
  auto h = g;
  for( int u = 0 ; u < n ; ++u )
   if( M[ u ] )
    h.deleteNode( u );
  const int ref = ( M[ s ] || M[ d ] ) ? INF : distance( h , s ).first[ d ];
  CHECK( graph::noMonsters( g , M , s , d ) == ref );
  }
 const auto c = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 CHECK( graph::noMonsters( c , { false , false , false } , 1 , 1 ) == 0 );
 CHECK( graph::noMonsters( c , { false , true , false } , 1 , 1 ) == INF );
 CHECK( graph::noMonsters( c , { false , true , false } , 0 , 2 ) == INF );
 }

ADS_TEST( "graph-9-2" )
{
 // the simple paths from s to d, enumerated
 std::mt19937 r( 11 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomGraph( r , n , real( r ) , false );
  std::vector< bool > M( n );
  for( int u = 0 ; u < n ; ++u )
   M[ u ] = real( r ) < 0.4;
  const int s = uniform( r , 0 , n - 1 ) , d = uniform( r , 0 , n - 1 );
  int ref = INF;
  simplePaths( g , s , [ & ]( const Ints & P ) {
   if( P.back() == d ) {
    int k = 0;
    for( int x : P )
     k += M[ x ] ? 1 : 0;
    ref = std::min( ref , k );
    }
   } );
  CHECK( graph::fewestMonsters( g , M , s , d ) == ref );
  }
 // a detour with no monster, and s = d
 const auto c = make( 5 , { { 0 , 1 } , { 1 , 2 } , { 0 , 3 } , { 3 , 4 } ,
                            { 4 , 2 } } , false );
 std::vector< bool > M( 5 , false );
 M[ 1 ] = true;
 CHECK( graph::fewestMonsters( c , M , 0 , 2 ) == 0 );
 M[ 4 ] = true;
 CHECK( graph::fewestMonsters( c , M , 0 , 2 ) == 1 );
 CHECK( graph::fewestMonsters( c , M , 1 , 1 ) == 1 );
 CHECK( graph::fewestMonsters( ListGraph<>( 2 , false ) , { true , true } ,
                               0 , 1 ) == INF );
 }

/*------------------------ 10. 2014-02-03, Connesso ------------------------*/

namespace graph {

/// connesso(): whether g stays connected without the node x, by a visit
/// from another node that never enters x
/// \pre x is a node
template< Graph G >
bool connectedWithout( const G & g , int x )
{
 ADS_PRE( g.hasNode( x ) );
 std::vector< bool > visited( g.bound() , false );
 visited[ x ] = true;                       // the visit never enters x
 int start = NIL;
 for( int u : g.V() )
  if( u != x ) {
   start = u;
   break;
   }
 if( start == NIL )
  return( true );
 auto none = []( auto... ) {};
 dfs( g , start , visited , none , none , none );
 for( int u : g.V() )
  if( ! visited[ u ] )
   return( false );
 return( true );
 }

/// cercaNodo(): a node whose removal leaves the connected unoriented graph
/// connected, the first one for which connesso() holds, O( n m )
/// \pre g is unoriented, connected, with a node
template< Graph G >
int removableNode( const G & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) );
 for( int x : g.V() )
  if( connectedWithout( g , x ) )
   return( x );
 return( NIL );
 }

}  // end( namespace graph )

ADS_TEST( "graph-10" )
{
 // the node returned leaves the graph connected, the earlier ones do not
 std::mt19937 r( 12 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomConnected( r , n , 0.3 * real( r ) );
  const int x = graph::removableNode( g );
  CHECK( x != NIL );
  for( int y = 0 ; y <= x ; ++y ) {
   auto h = g;
   h.deleteNode( y );
   CHECK( connected( h ) == ( y == x ) );
   }
  }
 CHECK( graph::removableNode( ListGraph<>( 1 , false ) ) == 0 );
 const auto star = make( 4 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } } , false );
 CHECK( graph::removableNode( star ) == 1 );
 }

/*--------------------- 11. 2014-04-24, Ciclo con arco ---------------------*/

namespace graph {

/// whether the edge [ x , y ] of a connected unoriented graph lies on a
/// cycle, i.e., whether the graph stays connected without it: a depth first
/// visit from x that never crosses [ x , y ] and reaches every node,
/// O( n + m )
/// \pre g is unoriented and [ x , y ] is an edge
template< Graph G >
bool edgeOnCycle( const G & g , int x , int y )
{
 ADS_PRE( ! g.directed() && g.hasNode( x ) && g.hasNode( y ) );
 std::vector< bool > visited( g.bound() , false );
 auto rec = [ & ]( auto & self , int u ) -> void {
  visited[ u ] = true;
  for( int v : g.adj( u ) )
   if( ! ( ( u == x ) && ( v == y ) ) && ! ( ( u == y ) && ( v == x ) ) &&
       ! visited[ v ] )
    self( self , v );
  };
 rec( rec , x );
 for( int u : g.V() )
  if( ! visited[ u ] )
   return( false );
 return( true );
 }

}  // end( namespace graph )

ADS_TEST( "graph-11" )
{
 // the graph without the edge, from x to y
 std::mt19937 r( 13 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 2 , 8 );
  const auto g = randomConnected( r , n , 0.4 * real( r ) );
  for( auto [ x , y ] : edgeSet( g ) ) {
   auto h = g;
   h.deleteEdge( x , y );
   const bool ref = distance( h , x ).first[ y ] != INF;
   CHECK( graph::edgeOnCycle( g , x , y ) == ref );
   CHECK( graph::edgeOnCycle( g , y , x ) == ref );
   }
  }
 // a tree, a cycle, a triangle with a pendant edge
 const auto tree = randomTree( r , 10 , 3 );
 for( auto [ x , y ] : edgeSet( tree ) )
  CHECK( ! graph::edgeOnCycle( tree , x , y ) );
 const auto c = make( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } , { 2 , 3 } } ,
                      false );
 CHECK( graph::edgeOnCycle( c , 0 , 1 ) && graph::edgeOnCycle( c , 2 , 0 ) );
 CHECK( ! graph::edgeOnCycle( c , 2 , 3 ) );
 }

/*---------------------- 12. 2014-07-21, Is principal ----------------------*/

namespace graph {

/// isPrincipal(): whether v reaches every node, by a visit from v,
/// O( n + m ); it also says whether the team v can win the tournament g
/// (the canItalyWin() of 2018-06-28, the same problem)
/// \pre v is a node
template< Graph G >
bool isPrincipal( const G & g , int v )
{
 ADS_PRE( g.hasNode( v ) );
 return( allReached( g , v ) );
 }

/// principal(): whether some node reaches every node; if any node does, so
/// does the last one closed by a depth first visit of the whole graph, the
/// first of topsort(), O( n + m ) (the text speaks of the transposed graph
/// but its code, as here, visits G)
/// \pre g has a node
template< Graph G >
bool principal( const G & g )
{
 ADS_PRE( g.n() > 0 );
 return( allReached( g , topsort( g ).front() ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-12-1" )
{
 // the transitive closure
 std::mt19937 r( 14 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , real( r ) , true );
  const auto R = closure( g , true );
  for( int v = 0 ; v < n ; ++v ) {
   bool ref = true;
   for( int w = 0 ; w < n ; ++w )
    ref = ref && R[ v ][ w ];
   CHECK( graph::isPrincipal( g , v ) == ref );
   }
  }
 // canItalyWin() of 2018-06-28: every order of the matches, tried
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomTournament( r , n );
  for( int s = 0 ; s < n ; ++s )
   CHECK( graph::isPrincipal( g , s ) == canWinBrute( g , s ) );
  }
 // the example of the text, the teams 1 , 2 , 3 as 0 , 1 , 2: 2 beats 1
 // and 3, 1 beats 3; 1 cannot win, 2 can; on a cycle anyone can
 const auto e = make( 3 , { { 1 , 0 } , { 0 , 2 } , { 1 , 2 } } , true );
 CHECK( ! graph::isPrincipal( e , 0 ) && graph::isPrincipal( e , 1 ) );
 const auto c = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , true );
 for( int s = 0 ; s < 3 ; ++s )
  CHECK( graph::isPrincipal( c , s ) && canWinBrute( c , s ) );
 CHECK( graph::isPrincipal( ListGraph<>( 1 ) , 0 ) );
 }

ADS_TEST( "graph-12-2" )
{
 // the transitive closure
 std::mt19937 r( 15 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , 0.5 * real( r ) , true );
  const auto R = closure( g , true );
  bool ref = false;
  for( int v = 0 ; v < n ; ++v ) {
   bool all = true;
   for( int w = 0 ; w < n ; ++w )
    all = all && R[ v ][ w ];
   ref = ref || all;
   }
  CHECK( graph::principal( g ) == ref );
  }
 CHECK( graph::principal( ListGraph<>( 1 ) ) );
 CHECK( ! graph::principal( ListGraph<>( 2 ) ) );
 CHECK( graph::principal( make( 3 , { { 2 , 1 } , { 1 , 0 } } , true ) ) );
 }

/*---------------------- 13. 2015-01-12, 4cycles dfs -----------------------*/

namespace graph {

/// 4cycles(): whether the oriented graph has a closed walk of four pairwise
/// distinct edges; M[ u ][ v ] is true if some w not in { u , v } has
/// ( u , w ) and ( w , v ), and there is such a walk if and only if
/// M[ u ][ v ] and M[ v ][ u ] for two nodes u != v, O( n^3 ). The text
/// also takes u = v, which counts ( a , b ) , ( b , a ) , ( a , b ) ,
/// ( b , a ): here u != v
template< Graph G >
bool hasFourCycle( const G & g )
{
 const int n = g.bound();
 BoolMatrix A( n , std::vector< bool >( n , false ) ) , M = A;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { A[ u ][ v ] = true; } );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   for( int w = 0 ; ( w < n ) && ! M[ u ][ v ] ; ++w )
    M[ u ][ v ] = ( w != u ) && ( w != v ) && A[ u ][ w ] && A[ w ][ v ];
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && M[ u ][ v ] && M[ v ][ u ] )
    return( true );
 return( false );
 }

}  // end( namespace graph )

ADS_TEST( "graph-13" )
{
 // the examples, a , b , c as 0 , 1 , 2
 CHECK( graph::hasFourCycle( make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 1 } ,
                                         { 1 , 0 } } , true ) ) );
 CHECK( ! graph::hasFourCycle( make( 2 , { { 0 , 1 } , { 1 , 0 } } ,
                                     true ) ) );
 // the closed walks of four distinct edges, enumerated
 std::mt19937 r( 16 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  const auto g = randomGraph( r , n , 0.6 * real( r ) , true );
  bool ref = false;
  for( int u = 0 ; u < n ; ++u )
   for( int v : g.adj( u ) )
    for( int w : g.adj( v ) )
     for( int x : g.adj( w ) )
      if( g.hasEdge( x , u ) ) {
       const std::set< std::pair< int , int > > E = {
        { u , v } , { v , w } , { w , x } , { x , u } };
       ref = ref || ( E.size() == 4 );
       }
  CHECK( graph::hasFourCycle( g ) == ref );
  }
 CHECK( ! graph::hasFourCycle( ListGraph<>( 0 ) ) );
 CHECK( graph::hasFourCycle( make( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                                         { 3 , 0 } } , true ) ) );
 CHECK( ! graph::hasFourCycle( make( 3 , { { 0 , 1 } , { 1 , 2 } ,
                                           { 2 , 0 } } , true ) ) );
 }

/*------------------------- 14. 2015-02-02, Count --------------------------*/

namespace graph {

/// count(): the number of nodes that reach every node; only the strongly
/// connected component numbered 1 by scc() (the first one found on the
/// transposed graph) can hold them, and it holds them all if one of its
/// nodes reaches every node, O( n + m )
/// \pre g has a node
template< Graph G >
int countPrincipal( const G & g )
{
 ADS_PRE( g.n() > 0 );
 const auto id = scc( g );
 int count = 0 , u = NIL;
 for( int v : g.V() )
  if( id[ v ] == 1 ) {
   ++count;
   u = v;
   }
 return( allReached( g , u ) ? count : 0 );
 }

}  // end( namespace graph )

ADS_TEST( "graph-14" )
{
 // the transitive closure
 std::mt19937 r( 17 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , 0.5 * real( r ) , true );
  const auto R = closure( g , true );
  int ref = 0;
  for( int v = 0 ; v < n ; ++v ) {
   bool all = true;
   for( int w = 0 ; w < n ; ++w )
    all = all && R[ v ][ w ];
   ref += all ? 1 : 0;
   }
  CHECK( graph::countPrincipal( g ) == ref );
  }
 CHECK( graph::countPrincipal( ListGraph<>( 1 ) ) == 1 );
 ListGraph<> c( 50 , true );
 for( int u = 0 ; u < 50 ; ++u )
  c.insertEdge( u , ( u + 1 ) % 50 );
 CHECK( graph::countPrincipal( c ) == 50 );
 CHECK( graph::countPrincipal( make( 3 , { { 0 , 2 } , { 1 , 2 } } ,
                                     true ) ) == 0 );
 }

/*--------------------- 15. 2015-07-14, Cammini minimi ---------------------*/

namespace graph {

/// secretPlace(): the least d( a , z ) + d( b , z ) over the nodes z, the
/// paths not crossing c, infinity() if no z is reachable from both; the
/// edges entering c are dropped (the text gives them an infinite weight)
/// and the distances from a and from b come from Johnson, O( m log n )
/// \pre a, b and c are distinct nodes, the weights are not negative
template< Graph G >
auto secretPlace( const G & g , int a , int b , int c )
{
 using W = typename G::Weight;
 ADS_PRE( g.hasNode( a ) && g.hasNode( b ) && g.hasNode( c ) &&
          ( a != b ) && ( a != c ) && ( b != c ) );
 ListGraph< W > H( g.bound() , true );
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const W & w ) {
   if( v != c )
    H.insertEdge( u , v , w );
   } );
 const auto da = johnson( H , a ).d , db = johnson( H , b ).d;
 const W inf = infinity< W >();
 W best = inf;
 for( int u : g.V() )
  if( ( da[ u ] != inf ) && ( db[ u ] != inf ) )
   best = std::min( best , W( da[ u ] + db[ u ] ) );
 return( best );
 }

}  // end( namespace graph )

ADS_TEST( "graph-15" )
{
 // Floyd-Warshall on the graph without c
 std::mt19937 r( 18 );
 const int inf = infinity< int >();
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 3 , 7 );
  const auto g = randomWeighted( r , n , real( r ) , true , 0 , 9 );
  Ints abc = { 0 , 1 , 2 };
  for( int & x : abc )
   x = uniform( r , 0 , n - 1 );
  if( ( abc[ 0 ] == abc[ 1 ] ) || ( abc[ 0 ] == abc[ 2 ] ) ||
      ( abc[ 1 ] == abc[ 2 ] ) )
   continue;
  const int a = abc[ 0 ] , b = abc[ 1 ] , c = abc[ 2 ];
  ListGraph< int > h( n , true );
  for( int u = 0 ; u < n ; ++u )
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( ( u != c ) && ( v != c ) )
     h.insertEdge( u , v , w );
    } );
  const auto F = floydWarshall( h ).d;
  int ref = inf;
  for( int z = 0 ; z < n ; ++z )
   if( ( z != c ) && ( F[ a ][ z ] != inf ) && ( F[ b ][ z ] != inf ) )
    ref = std::min( ref , F[ a ][ z ] + F[ b ][ z ] );
  CHECK( graph::secretPlace( g , a , b , c ) == ref );
  }
 // a meeting at Bob, and the only way through Carl
 ListGraph< int > e( 3 , true );
 e.insertEdge( 0 , 1 , 5 );
 CHECK( graph::secretPlace( e , 0 , 1 , 2 ) == 5 );
 ListGraph< int > f( 3 , true );
 f.insertEdge( 0 , 2 , 1 );
 f.insertEdge( 2 , 1 , 1 );
 CHECK( graph::secretPlace( f , 0 , 1 , 2 ) == inf );
 }

/*---------------- 16. 2015-11-05, Check all, Check exists -----------------*/

namespace graph {

/// checkAll(): whether every path from s to t passes through u, true also
/// when there is no path; a depth first visit from s that never enters u
/// does not reach t, O( n + m )
/// \pre s, t and u are distinct nodes
template< Graph G >
bool checkAll( const G & g , int s , int t , int u )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) && g.hasNode( u ) &&
          ( s != t ) && ( s != u ) && ( t != u ) );
 std::vector< bool > visited( g.bound() , false );
 visited[ u ] = true;                       // the visit never enters u
 auto none = []( auto... ) {};
 dfs( g , s , visited , none , none , none );
 return( ! visited[ t ] );
 }

/// checkExists(): whether some path from s to t passes through u: u is
/// reachable from s and t from u, by two breadth first visits, O( n + m )
/// \pre s, t and u are distinct nodes
template< Graph G >
bool checkExists( const G & g , int s , int t , int u )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) && g.hasNode( u ) &&
          ( s != t ) && ( s != u ) && ( t != u ) );
 return( ( distance( g , s ).first[ u ] != INF ) &&
         ( distance( g , u ).first[ t ] != INF ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-16-1" )
{
 // the graph without u, from s to t
 std::mt19937 r( 19 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 3 , 8 );
  const auto g = randomGraph( r , n , real( r ) , true );
  const int s = uniform( r , 0 , n - 1 ) , e = uniform( r , 0 , n - 1 ) ,
            u = uniform( r , 0 , n - 1 );
  if( ( s == e ) || ( s == u ) || ( e == u ) )
   continue;
  auto h = g;
  h.deleteNode( u );
  CHECK( graph::checkAll( g , s , e , u ) ==
         ( distance( h , s ).first[ e ] == INF ) );
  }
 CHECK( graph::checkAll( ListGraph<>( 3 ) , 0 , 1 , 2 ) );
 CHECK( ! graph::checkAll( make( 3 , { { 0 , 1 } } , true ) , 0 , 1 , 2 ) );
 CHECK( graph::checkAll( make( 3 , { { 0 , 2 } , { 2 , 1 } } , true ) ,
                         0 , 1 , 2 ) );
 }

ADS_TEST( "graph-16-2" )
{
 // the reflexive transitive closure
 std::mt19937 r( 20 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 3 , 8 );
  const auto g = randomGraph( r , n , 0.5 * real( r ) , true );
  const auto R = closure( g , true );
  for( int s = 0 ; s < n ; ++s )
   for( int e = 0 ; e < n ; ++e )
    for( int u = 0 ; u < n ; ++u )
     if( ( s != e ) && ( s != u ) && ( e != u ) )
      CHECK( graph::checkExists( g , s , e , u ) ==
             ( R[ s ][ u ] && R[ u ][ e ] ) );
  }
 CHECK( ! graph::checkExists( make( 3 , { { 0 , 1 } } , true ) , 0 , 1 ,
                              2 ) );
 CHECK( graph::checkExists( make( 3 , { { 0 , 2 } , { 2 , 1 } } , true ) ,
                            0 , 1 , 2 ) );
 CHECK( ! graph::checkExists( make( 3 , { { 0 , 2 } , { 1 , 2 } } , true ) ,
                              0 , 1 , 2 ) );
 }

/*---------------------- 18. 2016-07-06, Compute prob ----------------------*/

namespace graph {

/// computeProb(): the expected number of prizes found, the prize u found
/// by itself with probability p[ u ] and then with all the prizes it
/// reaches; u is missed with probability x[ u ], the product of 1 - p[ v ]
/// over u and the nodes v that reach it, which a visit from each node v
/// accumulates on the nodes it reaches; the answer is the sum of the
/// 1 - x[ u ], O( n ( n + m ) )
/// \pre g is acyclic (audit), p has a place per name, in [ 0 , 1 ]
template< Graph G >
double expectedPrizes( const G & g , const std::vector< double > & p )
{
 ADS_PRE( int( p.size() ) == g.bound() );
 ADS_AUDIT( ! hasCycle( g ) );
 std::vector< double > x( g.bound() , 1 );
 auto none = []( auto... ) {};
 for( int u : g.V() ) {
  ADS_PRE( ( 0 <= p[ u ] ) && ( p[ u ] <= 1 ) );
  std::vector< bool > visited( g.bound() , false );
  dfs( g , u , visited , [ & ]( int v ) { x[ v ] *= 1 - p[ u ]; } , none ,
       none );
  }
 double e = 0;
 for( int u : g.V() )
  e += 1 - x[ u ];
 return( e );
 }

}  // end( namespace graph )

ADS_TEST( "graph-18" )
{
 // the 2^n sets of prizes found by themselves, closed under reachability
 std::mt19937 r( 21 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 1 , 10 );
  const auto g = randomDag( r , n , 0.5 * real( r ) );
  std::vector< double > p( n );
  for( double & x : p )
   x = ( real( r ) < 0.1 ) ? uniform( r , 0 , 1 ) : real( r );
  const auto R = closure( g , true );
  double ref = 0;
  for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
   double prob = 1;
   for( int u = 0 ; u < n ; ++u )
    prob *= ( ( S >> u ) & 1u ) ? p[ u ] : 1 - p[ u ];
   int found = 0;
   for( int v = 0 ; v < n ; ++v )
    for( int u = 0 ; u < n ; ++u )
     if( ( ( S >> u ) & 1u ) && R[ u ][ v ] ) {
      ++found;
      break;
      }
   ref += prob * found;
   }
  CHECK( std::abs( graph::expectedPrizes( g , p ) - ref ) < 1e-9 );
  }
 // no edges, all certain, none possible, a chain from a certain prize
 const std::vector< double > p = { 0.5 , 0.25 , 0.125 , 1 };
 CHECK( std::abs( graph::expectedPrizes( ListGraph<>( 4 ) , p ) - 1.875 ) <
        1e-12 );
 const auto chain = make( 4 , { { 3 , 2 } , { 2 , 1 } , { 1 , 0 } } , true );
 CHECK( std::abs( graph::expectedPrizes( chain , p ) - 4 ) < 1e-12 );
 CHECK( graph::expectedPrizes( chain , { 0 , 0 , 0 , 0 } ) == 0 );
 CHECK( graph::expectedPrizes( chain , { 1 , 1 , 1 , 1 } ) == 4 );
 }

/*----------------------- 19. 2016-08-29, Check path -----------------------*/

namespace graph {

/// checkPath(): whether some path of the acyclic graph has the label A,
/// the label of the node u being l[ u ]; the text tries each start node and
/// follows the edges while the labels match, superpolynomial in the worst
/// case
/// \pre A is not empty, l has a place per name
template< Graph G >
bool checkPath( const G & g , const std::string & l , const std::string & A )
{
 ADS_PRE( ! A.empty() && ( int( l.size() ) == g.bound() ) );
 auto rec = [ & ]( auto & self , std::size_t i , int u ) -> bool {
  if( l[ u ] != A[ i ] )
   return( false );
  if( i + 1 == A.size() )
   return( true );
  for( int v : g.adj( u ) )
   if( self( self , i + 1 , v ) )
    return( true );
  return( false );
  };
 for( int u : g.V() )
  if( rec( rec , 0 , u ) )
   return( true );
 return( false );
 }

/// the answer of checkPath() by memoization of the pairs ( node , place of
/// A ), O( |A| ( n + m ) )
/// \pre A is not empty, l has a place per name
template< Graph G >
bool checkPathMemo( const G & g , const std::string & l ,
                    const std::string & A )
{
 ADS_PRE( ! A.empty() && ( int( l.size() ) == g.bound() ) );
 std::vector< std::vector< signed char > > T( g.bound() ,
                               std::vector< signed char >( A.size() , -1 ) );
 auto rec = [ & ]( auto & self , std::size_t i , int u ) -> bool {
  if( T[ u ][ i ] < 0 ) {
   bool ok = ( l[ u ] == A[ i ] );
   if( ok && ( i + 1 < A.size() ) ) {
    ok = false;
    for( int v : g.adj( u ) )
     if( self( self , i + 1 , v ) ) {
      ok = true;
      break;
      }
    }
   T[ u ][ i ] = ok ? 1 : 0;
   }
  return( T[ u ][ i ] == 1 );
  };
 for( int u : g.V() )
  if( rec( rec , 0 , u ) )
   return( true );
 return( false );
 }

}  // end( namespace graph )

ADS_TEST( "graph-19" )
{
 // the labels of the figure, F L D O A E R F A, with the path L O D E
 const std::string l = "FLDOAERFA";
 const auto f = make( 9 , { { 4 , 1 } , { 1 , 3 } , { 3 , 2 } , { 2 , 5 } ,
                            { 0 , 2 } , { 4 , 6 } , { 6 , 7 } , { 7 , 8 } ,
                            { 3 , 7 } } , true );
 CHECK( ! hasCycle( f ) );
 for( const char * A : { "LODE" , "ALODE" , "LOFA" , "ARFA" , "F" } )
  CHECK( graph::checkPath( f , l , A ) && graph::checkPathMemo( f , l , A ) );
 for( const char * A : { "LODA" , "EL" , "ALODEX" , "Z" } )
  CHECK( ! graph::checkPath( f , l , A ) &&
         ! graph::checkPathMemo( f , l , A ) );
 // the labels of all the paths, enumerated
 std::mt19937 r( 22 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomDag( r , n , real( r ) );
  std::string lab( n , 'a' ) , A( uniform( r , 1 , 4 ) , 'a' );
  for( char & c : lab )
   c = char( 'a' + uniform( r , 0 , 1 ) );
  for( char & c : A )
   c = char( 'a' + uniform( r , 0 , 1 ) );
  bool ref = false;
  for( int s = 0 ; s < n ; ++s )
   simplePaths( g , s , [ & ]( const Ints & P ) {
    std::string w;
    for( int x : P )
     w += lab[ x ];
    ref = ref || ( w == A );
    } );
  CHECK( graph::checkPath( g , lab , A ) == ref );
  CHECK( graph::checkPathMemo( g , lab , A ) == ref );
  }
 }

/*--------------------- 20. 2017-01-24, Good bad guys ----------------------*/

// the wrestlers split into two groups with no rivalry inside a group if
// and only if the graph is bipartite, which twoColoring() of ADS decides
// [see EasyCases.h], as the text does, by a visit that gives each node the
// side opposite to that of the node it comes from

ADS_TEST( "graph-20" )
{
 // twoColoring() of ADS against the 2^n splits
 std::mt19937 r( 23 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomGraph( r , n , 0.5 * real( r ) , false );
  bool ref = false;
  for( unsigned S = 0 ; ( S < ( 1u << n ) ) && ! ref ; ++S ) {
   bool ok = true;
   for( int u = 0 ; u < n ; ++u )
    for( int v : g.adj( u ) )
     ok = ok && ( ( ( S >> u ) & 1u ) != ( ( S >> v ) & 1u ) );
   ref = ok;
   }
  const auto C = twoColoring( adjacency( g ) );
  CHECK( C.has_value() == ref );
  if( C )
   for( int u = 0 ; u < n ; ++u )
    for( int v : g.adj( u ) )
     CHECK( ( *C )[ u ] != ( *C )[ v ] );
  }
 // an odd cycle, an even one, no rivalry
 ListGraph<> c5( 5 , false ) , c6( 6 , false );
 for( int u = 0 ; u < 5 ; ++u )
  c5.insertEdge( u , ( u + 1 ) % 5 );
 for( int u = 0 ; u < 6 ; ++u )
  c6.insertEdge( u , ( u + 1 ) % 6 );
 CHECK( ! twoColoring( adjacency( c5 ) ) );
 CHECK( twoColoring( adjacency( c6 ) ).has_value() );
 CHECK( twoColoring( adjacency( ListGraph<>( 3 , false ) ) ).has_value() );
 }

/*---------------- 21. 2017-09-04, Longest descending path -----------------*/

namespace graph {

/// longestDescendingPath(): the largest total length of a path along which
/// the altitudes z strictly decrease, in an unoriented graph whose weights
/// are the lengths of the edges; D[ u ] = 0 if no neighbour v has z[ v ] <
/// z[ u ], else the largest d( u , v ) + D[ v ] over them: dagLongestPaths()
/// of the library on the acyclic graph of the downhill edges, a memoized
/// depth first visit, O( n + m )
/// \pre g has a node, z has a place per name, the lengths are not negative
template< Graph G >
auto longestDescending( const G & g , const Ints & z )
{
 ADS_PRE( ( g.n() > 0 ) && ( int( z.size() ) == g.bound() ) );
 const auto D = dagLongestPaths( g , [ & ]( int u , int v ) {
  return( z[ v ] < z[ u ] ); } );
 return( *std::max_element( D.begin() , D.end() ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-21" )
{
 // the descending paths, enumerated, and the longest paths of the acyclic
 // graph of the downhill edges by dagShortestPaths() with negated weights
 std::mt19937 r( 24 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto g = randomWeighted( r , n , real( r ) , false , 1 , 9 );
  Ints z( n );
  for( int & x : z )
   x = uniform( r , 0 , 3 );
  int ref = 0;
  auto rec = [ & ]( auto & self , int u , int len ) -> void {
   ref = std::max( ref , len );
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( z[ v ] < z[ u ] )
     self( self , v , len + w );
    } );
   };
  for( int u = 0 ; u < n ; ++u )
   rec( rec , u , 0 );
  ListGraph< int > down( n , true );
  for( int u = 0 ; u < n ; ++u )
   g.forEachEdge( u , [ & ]( int v , int w ) {
    if( z[ v ] < z[ u ] )
     down.insertEdge( u , v , -w );
    } );
  int ref2 = 0;
  for( int s = 0 ; s < n ; ++s ) {
   const auto R = dagShortestPaths( down , s );
   for( int v = 0 ; v < n ; ++v )
    if( R.d[ v ] != infinity< int >() )
     ref2 = std::max( ref2 , -R.d[ v ] );
   }
  CHECK( ref == ref2 );
  CHECK( graph::longestDescending( g , z ) == ref );
  }
 // one point, flat ground, and a slope
 CHECK( graph::longestDescending( ListGraph< int >( 1 , false ) , { 7 } ) ==
        0 );
 ListGraph< int > p( 4 , false );
 p.insertEdge( 0 , 1 , 3 );
 p.insertEdge( 1 , 2 , 4 );
 p.insertEdge( 2 , 3 , 5 );
 CHECK( graph::longestDescending( p , { 1 , 1 , 1 , 1 } ) == 0 );
 CHECK( graph::longestDescending( p , { 4 , 3 , 2 , 1 } ) == 12 );
 CHECK( graph::longestDescending( p , { 1 , 3 , 2 , 4 } ) == 5 );
 }

/*-------------------- 22. 2018-01-08, Grafo trasposto ---------------------*/

namespace graph {

/// transpose(): the transposed graph in place on the adjacency matrix M,
/// swapping M[ i ][ j ] and M[ j ][ i ] for j > i only (from j = 1 every
/// pair would be swapped twice), Theta( n^2 )
/// \pre M is square
inline void transposeInPlace( BoolMatrix & M )
{
 ADS_PRE( square( M ) );
 const int n = int( M.size() );
 for( int i = 0 ; i < n - 1 ; ++i )
  for( int j = i + 1 ; j < n ; ++j ) {
   const bool t = M[ i ][ j ];
   M[ i ][ j ] = M[ j ][ i ];
   M[ j ][ i ] = t;
   }
 }

}  // end( namespace graph )

ADS_TEST( "graph-22" )
{
 // the copy transposed out of place, and twice the same matrix
 std::mt19937 r( 25 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = uniform( r , 0 , 8 );
  const auto O = randomBools( r , n , 0.5 );
  auto M = O;
  graph::transposeInPlace( M );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    CHECK( M[ i ][ j ] == O[ j ][ i ] );
  graph::transposeInPlace( M );
  CHECK( M == O );
  }
 BoolMatrix one = { { true } };
 graph::transposeInPlace( one );
 CHECK( one == BoolMatrix( { { true } } ) );
 }

/*--------------- 23. 2018-06-28, Can italy win (printOrder) ---------------*/

namespace graph {

/// printOrder(): an order of the n - 1 matches of the tournament g, as
/// pairs ( winner , loser ), that leaves s alone; a depth first visit from
/// s that, on discovering v from u, first visits v and then lets u beat v,
/// so that each team beats its children before its parent beats it,
/// O( n + m )
/// \pre every node is reachable from s
template< Graph G >
std::vector< std::pair< int , int > > matchOrder( const G & g , int s )
{
 ADS_PRE( g.hasNode( s ) && allReached( g , s ) );
 std::vector< bool > visited( g.bound() , false );
 std::vector< std::pair< int , int > > L;
 auto rec = [ & ]( auto & self , int u ) -> void {
  visited[ u ] = true;
  for( int v : g.adj( u ) )
   if( ! visited[ v ] ) {
    self( self , v );
    L.emplace_back( u , v );
    }
  };
 rec( rec , s );
 return( L );
 }

}  // end( namespace graph )

ADS_TEST( "graph-23-2" )
{
 // the matches played in order: two teams still in, the winner by the edge,
 // and s alone at the end
 std::mt19937 r( 26 );
 int played = 0;
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomTournament( r , n );
  for( int s = 0 ; s < n ; ++s )
   if( graph::isPrincipal( g , s ) ) {
    const auto L = graph::matchOrder( g , s );
    CHECK( int( L.size() ) == n - 1 );
    std::vector< bool > in( n , true );
    for( auto [ w , x ] : L ) {
     CHECK( in[ w ] && in[ x ] && g.hasEdge( w , x ) );
     in[ x ] = false;
     }
    CHECK( int( std::count( in.begin() , in.end() , true ) ) == 1 );
    CHECK( in[ s ] );
    ++played;
    }
  }
 CHECK( played > 100 );
 CHECK( graph::matchOrder( ListGraph<>( 1 ) , 0 ).empty() );
 }

/*--------- 24. 2018-07-25, Erdos; 37. 2023-07-11, Island Hopping ----------*/

namespace graph {

/// grid(): the least number of moves between a cell of value a and one of
/// value b of a square matrix whose cells are all passable, INF if there
/// is no cell of value b; a breadth first visit from all the cells of value
/// a at once, which stops at the first cell of value b that it discovers,
/// O( n^2 ). It is the grid() of 2018-07-25 for a = 1 and b = 3, and the
/// distance() of 2023-07-11 for a = 1 and b = 2
/// \pre M is square, a != b
inline int cellDistance( const Matrix & M , int a , int b )
{
 ADS_PRE( square( M ) && ( a != b ) );
 const int n = int( M.size() );
 Matrix d( n , Ints( n , -1 ) );
 std::deque< std::pair< int , int > > Q;
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c )
   if( M[ r ][ c ] == a ) {
    d[ r ][ c ] = 0;
    Q.push_back( { r , c } );
    }
 while( ! Q.empty() ) {
  const auto [ r , c ] = Q.front();
  Q.pop_front();
  for( int k = 0 ; k < 4 ; ++k ) {
   const int x = r + DR[ k ] , y = c + DC[ k ];
   if( inside( n , x , y ) && ( d[ x ][ y ] < 0 ) ) {
    d[ x ][ y ] = d[ r ][ c ] + 1;
    if( M[ x ][ y ] == b )
     return( d[ x ][ y ] );
    Q.push_back( { x , y } );
    }
   }
  }
 return( INF );
 }

}  // end( namespace graph )

ADS_TEST( "graph-24" )
{
 // the examples of 2018-07-25 and of 2023-07-11 (the same problem)
 CHECK( graph::cellDistance( { { 1 , 2 , 2 , 3 } , { 2 , 1 , 2 , 3 } ,
                               { 2 , 2 , 2 , 3 } , { 3 , 2 , 1 , 2 } } ,
                             1 , 3 ) == 2 );
 Matrix hop( 8 , Ints( 8 , 0 ) );
 hop[ 1 ][ 1 ] = hop[ 2 ][ 1 ] = hop[ 2 ][ 2 ] = hop[ 3 ][ 1 ] = 1;
 hop[ 3 ][ 2 ] = 1;
 hop[ 4 ][ 5 ] = hop[ 4 ][ 6 ] = hop[ 5 ][ 5 ] = hop[ 5 ][ 6 ] = 2;
 hop[ 5 ][ 7 ] = hop[ 6 ][ 5 ] = hop[ 6 ][ 6 ] = 2;
 CHECK( graph::cellDistance( hop , 1 , 2 ) == 4 );
 // the least Manhattan distance between the two kinds of cells
 std::mt19937 r( 27 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const int lo = t % 2 , b = 3 - lo;
  const auto M = randomMatrix( r , n , lo , 3 - lo );
  int ref = INF;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int x = 0 ; x < n ; ++x )
     for( int y = 0 ; y < n ; ++y )
      if( ( M[ i ][ j ] == 1 ) && ( M[ x ][ y ] == b ) )
       ref = std::min( ref , std::abs( i - x ) + std::abs( j - y ) );
  CHECK( graph::cellDistance( M , 1 , b ) == ref );
  }
 Matrix big( 50 , Ints( 50 , 2 ) );
 big[ 0 ][ 0 ] = 1;
 big[ 49 ][ 49 ] = 3;
 CHECK( graph::cellDistance( big , 1 , 3 ) == 98 );
 CHECK( graph::cellDistance( { { 1 , 3 } , { 2 , 2 } } , 1 , 3 ) == 1 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File exam_grapha.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
