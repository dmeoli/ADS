/*--------------------------------------------------------------------------*/
/*------------------------ File contract_graph.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the graphs: every operation
 * of the specification of the book, its result as the specification states
 * it, and the violations of its preconditions, which must throw
 * ADS::contract_violation. The tests are compiled at the level 3, so that
 * each operation also checks its postconditions, the invariant of the
 * structure and the audits (the conditions of Bellman, the minimum spanning
 * forests, the augmenting paths left), on random graphs against a
 * reference of the standard library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 9 and 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 13.5,
 *      14.3 and 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#include "ADSTest.h"

#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/EulerianCycle.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/PointerGraph.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// a random graph on n nodes, oriented or not, each edge with probability
/// p percent and a weight in [ lo , hi ]
template< class G >
G random_graph( int n , bool directed , int p , long lo , long hi ,
                std::mt19937 & g )
{
 G h( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && ( int( g() % 100 ) < p ) )
    h.insertEdge( u , v , lo + long( g() % ( hi - lo + 1 ) ) );
 return( h );
 }

/// GRAPH on G: random insertions and deletions of nodes and edges against
/// a set of pairs, the preconditions violated
template< class G >
void dynamic_graph( bool directed , std::mt19937 & g )
{
 G h( 4 , directed );
 CHECK( h.insertNode() == 4 && h.n() == 5 && h.bound() == 5 );
 CHECK_THROWS( h.insertEdge( 1 , 1 ) , contract_violation );   // a loop
 CHECK_THROWS( h.insertEdge( 0 , 9 ) , contract_violation );
 CHECK_THROWS( h.deleteEdge( -1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) h.adj( 7 ) , contract_violation );
 CHECK_THROWS( (void) h.label( 7 ) , contract_violation );
 h.deleteNode( 2 );
 CHECK_THROWS( h.deleteNode( 2 ) , contract_violation );
 CHECK_THROWS( h.insertEdge( 0 , 2 ) , contract_violation );
 CHECK_THROWS( G( -1 ) , contract_violation );
 CHECK( ! h.hasEdge( 0 , 2 ) && ! h.weight( 0 , 2 ) );   // queries only
 std::set< std::pair< int , int > > E;
 std::vector< bool > alive( h.bound() , true );
 alive[ 2 ] = false;
 auto key = [ & ]( int u , int v ) {
  return( directed ? std::pair( u , v )
                   : std::pair( std::min( u , v ) , std::max( u , v ) ) );
  };
 for( int op = 0 ; op < 600 ; ++op ) {
  const int k = int( g() % 10 );
  const int u = int( g() % h.bound() ) , v = int( g() % h.bound() );
  if( k == 0 ) {
   alive.push_back( true );
   CHECK( h.insertNode() == int( alive.size() ) - 1 );
   }
  else if( ( k == 1 ) && alive[ u ] && ( h.n() > 2 ) ) {
   h.deleteNode( u );
   alive[ u ] = false;
   for( auto it = E.begin() ; it != E.end() ; )
    it = ( it->first == u ) || ( it->second == u ) ? E.erase( it )
                                                   : std::next( it );
   }
  else if( ( u != v ) && alive[ u ] && alive[ v ] ) {
   if( k < 7 ) {
    h.insertEdge( u , v , int( g() % 10 ) );
    E.insert( key( u , v ) );
    }
   else {
    h.deleteEdge( u , v );
    E.erase( key( u , v ) );
    }
   }
  CHECK( h.m() == int( E.size() ) );
  }
 for( int u : h.V() )
  for( int v : h.V() )
   CHECK( h.hasEdge( u , v ) == ( ( u != v ) && E.count( key( u , v ) ) ) );
 }

/// the reachable nodes of an oriented graph with a negative cycle from 0
ListGraph< long > negative_cycle( void )
{
 ListGraph< long > h( 4 );
 h.insertEdge( 0 , 1 , 2 );
 h.insertEdge( 1 , 2 , -3 );
 h.insertEdge( 2 , 1 , 1 );
 h.insertEdge( 2 , 3 , 1 );
 return( h );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

// GRAPH on lists and on a matrix, oriented or not
ADS_TEST( "graphs" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 5 ; ++t )
  for( bool directed : { true , false } ) {
   dynamic_graph< ListGraph< int > >( directed , g );
   dynamic_graph< MatrixGraph< int > >( directed , g );
   }
 ListGraph< int , char > L( 2 );
 L.setLabel( 1 , 'b' );
 CHECK( L.label( 1 ) == 'b' );
 CHECK_THROWS( L.setLabel( 5 , 'c' ) , contract_violation );
 }

// the static graph: wrong edges refused, the same adjacency as its source
ADS_TEST( "array-graph" )
{
 using E = std::vector< std::tuple< int , int , int > >;
 CHECK_THROWS( ArrayGraph< int >( -1 , E() ) , contract_violation );
 CHECK_THROWS( ArrayGraph< int >( 3 , E{ { 0 , 3 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( ArrayGraph< int >( 3 , E{ { 1 , 1 , 1 } } ) ,
               contract_violation );
 std::mt19937 g( 2 );
 for( bool directed : { true , false } ) {
  const auto h = random_graph< ListGraph< int > >( 30 , directed , 20 , 1 ,
                                                    9 , g );
  const ArrayGraph< int > a( h );
  CHECK( a.m() == h.m() && a.n() == h.n() );
  for( int u : h.V() ) {
   auto x = a.adj( u ) , y = h.adj( u );
   std::sort( x.begin() , x.end() );
   std::sort( y.begin() , y.end() );
   CHECK( x == y );
   }
  CHECK_THROWS( (void) a.adj( 30 ) , contract_violation );
  }
 }

// the visits: from a node, dfs() from one not visited with a vector large
// enough, the breadth first trees and the topological orders audited on
// random graphs
ADS_TEST( "visits" )
{
 std::mt19937 g( 3 );
 const auto h = random_graph< ListGraph< int > >( 40 , true , 8 , 1 , 1 , g );
 auto none = []( auto... ) {};
 CHECK_THROWS( bfs( h , 40 , none , none ) , contract_violation );
 CHECK_THROWS( (void) distance( h , -1 ) , contract_violation );
 std::vector< bool > small( 3 , false ) , seen( 40 , false );
 CHECK_THROWS( dfs( h , 0 , small , none , none , none ) ,
               contract_violation );
 seen[ 5 ] = true;
 CHECK_THROWS( dfs( h , 5 , seen , none , none , none ) ,
               contract_violation );
 CHECK_THROWS( (void) path( 0 , 99 , std::vector< int >( 3 , NIL ) ) ,
               contract_violation );
 DfsSchema< ListGraph< int > > D( h );
 D.visit( 0 , none );
 CHECK_THROWS( D.visit( 0 , none ) , contract_violation );
 CHECK_THROWS( (void) D.dt( 41 ) , contract_violation );
 for( int t = 0 ; t < 30 ; ++t ) {
  // a random acyclic graph: the edges from smaller to larger names, the
  // names shuffled
  const int n = 2 + int( g() % 30 );
  std::vector< int > pi( n );
  std::iota( pi.begin() , pi.end() , 0 );
  std::shuffle( pi.begin() , pi.end() , g );
  ListGraph<> a( n );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 5 == 0 ) a.insertEdge( pi[ u ] , pi[ v ] );
  CHECK( ! hasCycle( a ) );
  CHECK( isTopologicalOrder( a , topsort( a ) ) );
  CHECK( isTopologicalOrder( a , topsortKahn( a ) ) );
  auto S = topsort( a );
  if( a.m() > 0 ) {                      // a wrong order
   std::reverse( S.begin() , S.end() );
   CHECK( ! isTopologicalOrder( a , S ) );
   }
  const auto [ d , p ] = distance( a , pi[ 0 ] );
  CHECK( d[ pi[ 0 ] ] == 0 );
  const auto c = scc( a );                // each node alone
  CHECK( std::set< int >( c.begin() , c.end() ).size() == std::size_t( n ) );
  }
 }

// the shortest paths: every frontier gives a tree that satisfies the
// conditions of Bellman; a negative cycle is found by the queue and
// refused by the others; dagShortestPaths() needs an acyclic graph
ADS_TEST( "shortest-paths" )
{
 std::mt19937 g( 4 );
 const Frontier all[] = { Frontier::LIST , Frontier::HEAP ,
                          Frontier::FIBONACCI , Frontier::QUEUE ,
                          Frontier::STACK , Frontier::DEQUE };
 for( int t = 0 ; t < 20 ; ++t ) {
  const auto h = random_graph< ListGraph< long > >( 25 , true , 15 , 0 , 20 ,
                                                     g );
  const auto ref = shortestPaths( h , 0 , Frontier::HEAP );
  for( Frontier f : all ) {
   const auto R = shortestPaths( h , 0 , f );
   CHECK( isShortestPathTree( h , 0 , R ) && ( R.d == ref.d ) );
   }
  auto bad = ref;                         // a tree that is not optimal
  for( int v : h.V() )
   if( ( v != 0 ) && ( bad.d[ v ] != infinity< long >() ) ) {
    bad.d[ v ] += 1;
    break;
    }
  CHECK( ( bad.d == ref.d ) || ! isShortestPathTree( h , 0 , bad ) );
  }
 const auto c = negative_cycle();
 CHECK( reachesNegativeCycle( c , 0 ) && ! reachesNegativeCycle( c , 3 ) );
 CHECK( shortestPaths( c , 0 , Frontier::QUEUE ).negativeCycle );
 for( Frontier f : { Frontier::LIST , Frontier::HEAP , Frontier::STACK ,
                     Frontier::DEQUE } )
  CHECK_THROWS( shortestPaths( c , 0 , f ) , contract_violation );
 CHECK_THROWS( shortestPaths( c , 4 , Frontier::QUEUE ) , contract_violation );
 CHECK_THROWS( dagShortestPaths( c , 0 ) , contract_violation );   // a cycle
 ListGraph< long > a( 4 );                 // negative costs, no cycle
 a.insertEdge( 0 , 1 , 4 );
 a.insertEdge( 0 , 2 , 1 );
 a.insertEdge( 2 , 1 , -3 );
 a.insertEdge( 1 , 3 , 2 );
 const auto R = dagShortestPaths( a , 0 );
 CHECK( R.d[ 1 ] == -2 && R.d[ 3 ] == 0 && isShortestPathTree( a , 0 , R ) );
 }

// all the pairs: the rows of Floyd-Warshall are the shortest paths from
// each node; addEdge() refuses a negative cycle and an edge out of the
// matrix, and keeps the distances closed
ADS_TEST( "all-pairs" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 10 ; ++t ) {
  auto h = random_graph< ListGraph< long > >( 12 , true , 25 , -2 , 10 , g );
  auto R = floydWarshall( h );
  if( R.negativeCycle )
   continue;
  for( int u : h.V() )
   CHECK( shortestPaths( h , u , Frontier::QUEUE ).d == R.d[ u ] );
  for( int k = 0 ; k < 10 ; ++k ) {
   const int a = int( g() % 12 ) , b = int( g() % 12 );
   long w = long( g() % 10 );
   if( ( a != b ) && ( R.d[ b ][ a ] != infinity< long >() ) )
    w = std::max( w , -R.d[ b ][ a ] );   // no negative cycle
   if( a == b ) {
    CHECK_THROWS( addEdge( R , a , b , w ) , contract_violation );
    continue;
    }
   addEdge( R , a , b , w );
   h.insertEdge( a , b , std::min( w , h.weight( a , b ).value_or( w ) ) );
   CHECK( floydWarshall( h ).d == R.d );
   }
  CHECK_THROWS( addEdge( R , 0 , 12 , 1L ) , contract_violation );
  }
 auto C = floydWarshall( negative_cycle() );
 CHECK( C.negativeCycle );
 ListGraph< long > two( 2 );
 two.insertEdge( 0 , 1 , 3 );
 auto T = floydWarshall( two );
 CHECK_THROWS( addEdge( T , 1 , 0 , -4L ) , contract_violation );
 addEdge( T , 1 , 0 , -3L );               // a cycle of cost 0
 CHECK( T.d[ 1 ][ 0 ] == -3 );
 }

// the minimum spanning trees: Kruskal and Prim with each priority queue
// audited by the property of the cycles; the preconditions violated
ADS_TEST( "spanning-tree" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 10 ; ++t ) {
  const auto h = random_graph< ListGraph< long > >( 30 , false , 20 , 0 , 50 ,
                                                     g );
  const auto T = kruskal( h );
  CHECK( isMinimumSpanningForest( h.bound() , edges( h ) , T ) );
  CHECK( kruskal( h , 51 ).size() == T.size() );      // by counting
  for( Frontier f : { Frontier::LIST , Frontier::HEAP , Frontier::FIBONACCI } )
   (void) prim( h , 0 , f );                           // audited
  auto E = edges( h );                    // T and one more edge: a cycle
  for( const auto & e : E )
   if( std::find( T.begin() , T.end() , e ) == T.end() ) {
    auto U = T;
    U.push_back( e );
    CHECK( ! isMinimumSpanningForest( h.bound() , E , U ) );
    break;
    }
  }
 const auto o = random_graph< ListGraph< long > >( 5 , true , 50 , 1 , 9 , g );
 CHECK_THROWS( (void) edges( o ) , contract_violation );
 CHECK_THROWS( (void) prim( o , 0 ) , contract_violation );
 ListGraph< long > u( 3 , false );
 u.insertEdge( 0 , 1 , 5 );
 CHECK_THROWS( (void) prim( u , 3 ) , contract_violation );
 CHECK_THROWS( (void) prim( u , 0 , Frontier::QUEUE ) , contract_violation );
 using WE = std::vector< WeightedEdge< long > >;
 CHECK_THROWS( (void) kruskal( 2 , WE{ { 0 , 2 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) kruskal( 2 , WE{ { 0 , 1 , 7 } } , 5 ) ,
               contract_violation );
 }

// the maximum flow: the three ways agree with the capacity of the minimum
// cut, the circulations audited; the preconditions violated
ADS_TEST( "max-flow" )
{
 CHECK_THROWS( FlowNetwork( -1 ) , contract_violation );
 FlowNetwork N( 3 );
 CHECK_THROWS( N.addEdge( 0 , 1 , -1 ) , contract_violation );
 CHECK_THROWS( N.addEdge( 0 , 3 , 1 ) , contract_violation );
 CHECK_THROWS( N.maxFlow( 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) N.flow( 0 ) , contract_violation );
 CHECK_THROWS( (void) N.minCut( 5 ) , contract_violation );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 2 + int( g() % 10 );
  std::vector< std::tuple< int , int , long > > E;
  for( int k = 0 ; k < 3 * n ; ++k ) {
   const int u = int( g() % n ) , v = int( g() % n );
   if( u != v ) E.emplace_back( u , v , long( g() % 10 ) );
   }
  long ref = -1;
  for( Augment a : { Augment::FORD_FULKERSON , Augment::EDMONDS_KARP ,
                     Augment::DINIC , Augment::PUSH_RELABEL } ) {
   FlowNetwork F( n );
   std::vector< int > id;
   for( auto [ u , v , c ] : E ) id.push_back( F.addEdge( u , v , c ) );
   const long f = F.maxFlow( 0 , n - 1 , a );
   const auto S = F.minCut( 0 );
   long cut = 0;
   for( std::size_t k = 0 ; k < E.size() ; ++k )
    if( S[ std::get< 0 >( E[ k ] ) ] && ! S[ std::get< 1 >( E[ k ] ) ] )
     cut += std::get< 2 >( E[ k ] );
   CHECK( f == cut && ( ref < 0 || f == ref ) );
   ref = f;
   }
  std::vector< BoundedEdge > B;
  for( auto [ u , v , c ] : E ) B.push_back( { u , v , c / 3 , c } );
  std::vector< long long > flows;
  (void) feasibleCirculation( n , B , &flows );        // audited
  }
 CHECK_THROWS( (void) feasibleCirculation( 2 , { { 0 , 2 , 0 , 1 } } ) ,
               contract_violation );
 }

// the Eulerian cycles of random unions of cycles, audited; an unoriented
// graph refused
ADS_TEST( "eulerian" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 3 + int( g() % 8 );
  ListGraph<> h( n );
  for( int c = 0 ; c < 3 ; ++c ) {        // a cycle on a random subset
   std::vector< int > S( n );
   std::iota( S.begin() , S.end() , 0 );
   std::shuffle( S.begin() , S.end() , g );
   S.resize( 2 + g() % ( n - 1 ) );
   bool fresh = true;
   for( std::size_t i = 0 ; i < S.size() ; ++i )
    fresh = fresh && ! h.hasEdge( S[ i ] , S[ ( i + 1 ) % S.size() ] );
   if( fresh )
    for( std::size_t i = 0 ; i < S.size() ; ++i )
     h.insertEdge( S[ i ] , S[ ( i + 1 ) % S.size() ] );
   }
  const auto c = eulerianCycle( h );
  if( c ) CHECK( isEulerianCycle( h , *c ) );
  }
 ListGraph<> u( 3 , false );
 CHECK_THROWS( (void) eulerianCycle( u ) , contract_violation );
 ListGraph<> d( 3 );
 d.insertEdge( 0 , 1 );
 d.insertEdge( 1 , 0 );
 CHECK( ! isEulerianCycle( d , { 0 , 1 , 0 , 1 , 0 } ) );
 CHECK( isEulerianCycle( d , { 1 , 0 , 1 } ) );
 }

// the graph with pointers: nil and the nodes of another graph refused,
// random insertions and deletions against a set of pairs
ADS_TEST( "pointer-graph" )
{
 PointerGraph< int > G , H;
 auto a = G.insertNode( 0 ) , b = G.insertNode( 1 );
 auto x = H.insertNode( 9 );
 CHECK_THROWS( G.insertEdge( a , nullptr ) , contract_violation );
 CHECK_THROWS( G.insertEdge( a , x ) , contract_violation );   // foreign
 CHECK_THROWS( G.dfs( nullptr , []( auto * ) {} ) , contract_violation );
 CHECK_THROWS( (void) G.deleteNode( x ) , contract_violation );
 G.insertEdge( a , b );
 CHECK( G.hasEdge( a , b ) && a->degree == 1 && b->degree == 1 );
 std::mt19937 g( 9 );
 std::vector< PointerGraph< int >::Node * > V = { a , b };
 for( int i = 2 ; i < 12 ; ++i ) V.push_back( G.insertNode( i ) );
 std::set< std::pair< int , int > > E = { { 0 , 1 } };
 for( int op = 0 ; op < 500 ; ++op ) {
  const int u = int( g() % V.size() ) , v = int( g() % V.size() );
  if( g() % 2 ) {
   G.insertEdge( V[ u ] , V[ v ] );
   E.insert( { u , v } );
   }
  else {
   G.deleteEdge( V[ u ] , V[ v ] );
   E.erase( { u , v } );
   }
  }
 for( std::size_t u = 0 ; u < V.size() ; ++u )
  for( std::size_t v = 0 ; v < V.size() ; ++v )
   CHECK( G.hasEdge( V[ u ] , V[ v ] ) ==
          bool( E.count( { int( u ) , int( v ) } ) ) );
 G.forceDeleteNode( V[ 3 ] );
 CHECK( G.n() == 11 );
 using N = PointerGraph< int >::Node;
 CHECK_THROWS( (void) G.hasEdge( nullptr , a ) , contract_violation );
 CHECK_THROWS( (void) G.hasEdge( a , ( N * ) nullptr ) , contract_violation );
 CHECK_THROWS( G.insertEdge( nullptr , a ) , contract_violation );
 CHECK_THROWS( G.deleteEdge( a , nullptr ) , contract_violation );
 CHECK_THROWS( G.deleteEdge( nullptr , a ) , contract_violation );
 CHECK_THROWS( (void) G.deleteNode( nullptr ) , contract_violation );
 CHECK_THROWS( G.forceDeleteNode( nullptr ) , contract_violation );
 CHECK_THROWS( (void) G.adjacent( nullptr ) , contract_violation );
 CHECK_THROWS( G.bfs( nullptr , []( auto * ) {} ) , contract_violation );
 PointerGraph< int > One;                     // one node, no edge
 auto o = One.insertNode( 7 );
 int seen = 0;
 One.dfs( o , [ & ]( auto * ) { ++seen; } );
 One.bfs( o , [ & ]( auto * ) { ++seen; } );
 CHECK( seen == 2 && One.adjacent( o ).empty() && One.deleteNode( o ) &&
        One.n() == 0 );
 }

// the other headers of graphs, each bound just inside and just outside:
// the nodes -1 and n of the array graph, the flow network, the paths, the
// grids, the matchings; the unoriented graphs refused where an oriented
// one is needed and vice versa; the trees with no node, a cycle and a
// disconnected graph; the decrease of a capacity on an edge of capacity
// 0, s = t, u = v; the bounds of the grid at -1 and at the size, a source
// on a wall; one node, no edge, the empty grid
ADS_TEST( "other-graphs" )
{
 using E = std::vector< std::tuple< int , int , int > >;
 const ArrayGraph< int > a( 3 , E{ { 0 , 2 , 5 } } ) , a0( 0 , E{} );
 const auto none1 = []( const auto & ) {};
 const auto none2 = []( const auto & ... ) {};
 a.forEachAdj( 2 , none1 );
 a.forEachEdge( 0 , none2 );
 CHECK( a.neighbors( 0 ).size() == 1 && a.neighbors( 2 ).empty() &&
        a0.n() == 0 );
 CHECK_THROWS( a.forEachAdj( 3 , none1 ) , contract_violation );
 CHECK_THROWS( a.forEachEdge( -1 , none2 ) , contract_violation );
 CHECK_THROWS( (void) a.neighbors( 3 ) , contract_violation );
 CHECK_THROWS( (void) a0.adj( 0 ) , contract_violation );
 CHECK_THROWS( ( ArrayGraph< int >( 2 , E{ { -1 , 0 , 1 } } ) ) ,
               contract_violation );

 ListGraph<> d( 3 ) , u( 3 , false ) , one( 1 ) , none( 0 );
 d.insertEdge( 0 , 1 );
 d.insertEdge( 1 , 2 );
 u.insertEdge( 0 , 1 );
 u.insertEdge( 1 , 2 );
 CHECK_THROWS( (void) adjFromIncidence( incidence( d ) , 3 , true ) ,
               contract_violation );
 CHECK_THROWS( (void) adjFromIncidence( incidence( d ) , -1 , true ) ,
               contract_violation );
 CHECK( adjFromIncidence( incidence( d ) , 2 , true ).empty() &&
        adjFromIncidence( incidence( u ) , 2 , false ).size() == 1 );
 CHECK_THROWS( (void) genericVisit( d , 3 , Choice::NEWEST ) ,
               contract_violation );
 (void) genericVisit( one , 0 , Choice::OLDEST );
 DfsSchema< ListGraph<> > D( d );
 D.visitAll( none2 );
 CHECK( D.ft( 2 ) > 0 && D.dt( 0 ) > 0 );
 CHECK_THROWS( (void) D.ft( 3 ) , contract_violation );
 CHECK_THROWS( (void) D.ft( -1 ) , contract_violation );
 CHECK_THROWS( D.visit( 1 , none2 ) , contract_violation );   // visited
 CHECK_THROWS( (void) path( -1 , 0 , std::vector< int >( 3 , NIL ) ) ,
               contract_violation );
 CHECK( path( 0 , 0 , std::vector< int >( 1 , NIL ) ) ==
        std::vector< int >( { 0 } ) );

 FlowNetwork F( 2 ) , F0( 0 );
 const int e = F.addEdge( 0 , 1 , 0 );        // capacity 0: just inside
 CHECK( F.maxFlow( 0 , 1 ) == 0 && F.capacity( e ) == 0 && F.flow( e ) == 0 );
 CHECK_THROWS( (void) F.capacity( e + 2 ) , contract_violation );
 CHECK_THROWS( (void) F.capacity( -1 ) , contract_violation );
 CHECK_THROWS( (void) F.flow( -1 ) , contract_violation );
 CHECK_THROWS( F.addEdge( -1 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( F.maxFlow( 0 , 2 ) , contract_violation );
 CHECK_THROWS( F.maxFlow( -1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) F.minCut( -1 ) , contract_violation );
 CHECK_THROWS( F0.addEdge( 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( (void) feasibleCirculation( -1 , {} ) , contract_violation );
 CHECK_THROWS( (void) feasibleCirculation( 2 , { { -1 , 0 , 0 , 1 } } ) ,
               contract_violation );
 CHECK( feasibleCirculation( 0 , {} ) &&
        feasibleCirculation( 2 , { { 0 , 1 , 0 , 1 } } ) &&
        ! feasibleCirculation( 2 , { { 0 , 1 , 1 , 1 } } ) );

 CHECK_THROWS( (void) edgeDisjointPaths( d , 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) edgeDisjointPaths( d , 0 , 3 ) , contract_violation );
 CHECK_THROWS( (void) edgeDisjointPaths( d , 0 , 2 , -1 ) ,
               contract_violation );
 CHECK( edgeDisjointPaths( d , 0 , 2 , 0 ).empty() &&
        edgeDisjointPaths( d , 0 , 2 ).size() == 1 &&
        edgeDisjointPaths( d , 2 , 0 ).empty() );
 using M = std::vector< std::vector< long > >;
 M c = { { 0 , 1 } , { 0 , 0 } } , f = { { 0 , 1 } , { -1 , 0 } };
 CHECK_THROWS( (void) decreaseCapacity( c , f , 0 , 0 , 0 , 1 ) ,
               contract_violation );
 CHECK_THROWS( (void) decreaseCapacity( c , f , 0 , 1 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( (void) decreaseCapacity( c , f , 0 , 1 , 1 , 0 ) ,
               contract_violation );                  // capacity 0
 CHECK_THROWS( (void) decreaseCapacity( c , f , 0 , 2 , 0 , 1 ) ,
               contract_violation );
 M ragged = { { 0 , 1 } };
 CHECK_THROWS( (void) decreaseCapacity( ragged , f , 0 , 1 , 0 , 1 ) ,
               contract_violation );
 M fbad = { { 0 , 0 } , { 0 , 0 } };          // not a maximum flow
 CHECK_THROWS( (void) decreaseCapacity( c , fbad , 0 , 1 , 0 , 1 ) ,
               contract_violation );
 CHECK( decreaseCapacity( c , f , 0 , 1 , 0 , 1 ) ==
        CapacityDecrease::PATH && c[ 0 ][ 1 ] == 0 && f[ 0 ][ 1 ] == 0 );

 const auto free = []( GridCell ) { return( true ); };
 const auto wall = []( GridCell x ) { return( x != GridCell( 0 , 0 ) ); };
 CHECK( gridDistances( 0 , 0 , {} , free ).empty() );
 CHECK( gridDistances( 2 , 2 , { { 1 , 1 } } , free )[ 0 ][ 0 ] == 2 );
 CHECK_THROWS( gridDistances( -1 , 2 , {} , free ) , contract_violation );
 CHECK_THROWS( gridDistances( 2 , 2 , { { 2 , 0 } } , free ) ,
               contract_violation );
 CHECK_THROWS( gridDistances( 2 , 2 , { { 0 , -1 } } , free ) ,
               contract_violation );
 CHECK_THROWS( gridDistances( 2 , 2 , { { 0 , 0 } } , wall ) ,
               contract_violation );
 CHECK( gridDistance( 2 , 2 , { 0 , 0 } , { 0 , 0 } , free ) == 0 &&
        gridDistance( 1 , 3 , { 0 , 0 } , { 0 , 2 } , free ) == 2 &&
        gridDistance( 2 , 2 , { 0 , 1 } , { 0 , 0 } , wall ) == INF );
 CHECK_THROWS( gridDistance( 2 , 2 , { 0 , 0 } , { 2 , 2 } , free ) ,
               contract_violation );
 CHECK_THROWS( gridDistance( 2 , 2 , { 0 , 0 } , { 1 , 1 } , wall ) ,
               contract_violation );
 CHECK_THROWS( gridDistance( 2 , -2 , { 0 , 0 } , { 0 , 0 } , free ) ,
               contract_violation );
 std::vector< std::vector< int > > P = { { 1 , 1 } , { 2 , 1 } } ,
                                   R = { { 1 , 1 } , { 1 } };
 CHECK( floodFill( P , 0 , 0 , 1 ) == 0 && floodFill( P , 1 , 1 , 3 ) == 3 &&
        P[ 1 ][ 0 ] == 2 );
 CHECK_THROWS( floodFill( P , 2 , 0 , 5 ) , contract_violation );
 CHECK_THROWS( floodFill( P , 0 , -1 , 5 ) , contract_violation );
 CHECK_THROWS( floodFill( R , 0 , 0 , 5 ) , contract_violation );
 CHECK( knightDistance( 1 , { 0 , 0 } , { 0 , 0 } ) == 0 &&
        knightDistance( 2 , { 0 , 0 } , { 1 , 1 } ) == INF &&
        knightDistance( 3 , { 0 , 0 } , { 2 , 1 } ) == 1 );
 CHECK_THROWS( knightDistance( 0 , { 0 , 0 } , { 0 , 0 } ) ,
               contract_violation );
 CHECK_THROWS( knightDistance( 3 , { 0 , 0 } , { 3 , 0 } ) ,
               contract_violation );

 using A = std::vector< std::vector< int > >;
 using L = std::vector< long long >;
 CHECK( bipartiteBMatching( A{} , 0 , L{} , L{} ).empty() );
 CHECK( bipartiteBMatching( A{ { 0 } } , 1 , L{ 0 } , L{ 1 } ).empty() &&
        bipartiteBMatching( A{ { 0 } } , 1 , L{ 2 } , L{ 2 } ).size() == 1 );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 0 } } , -1 , L{ 1 } , L{} ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 0 } } , 1 , L{} , L{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 0 } } , 1 , L{ 1 } , L{} ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 0 } } , 1 , L{ -1 } , L{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 1 } } , 1 , L{ 1 } , L{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { -1 } } , 1 , L{ 1 } , L{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) bipartiteBMatching( A{ { 0 } } , 1 , L{ 1 } , L{ -1 } ) ,
               contract_violation );
 using WE = std::vector< std::tuple< int , int , long > >;
 CHECK( maxWeightMatching( 0 , WE{} ).first == 0 &&
        maxWeightMatching( 2 , WE{ { 0 , 1 , 4 } } ).first == 4 );
 CHECK_THROWS( (void) maxWeightMatching( -1 , WE{} ) , contract_violation );
 CHECK_THROWS( (void) maxWeightMatching( 2 , WE{ { 0 , 2 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) maxWeightMatching( 2 , WE{ { -1 , 0 , 1 } } ) ,
               contract_violation );

 CHECK_THROWS( (void) countShortestPaths( d , 3 ) , contract_violation );
 CHECK( countShortestPaths( d , 0 )[ 2 ] == 1 &&
        countShortestPaths( d , 2 )[ 0 ] == 0 );
 CHECK_THROWS( (void) multiSourceDistance( d , { 0 , 3 } ) ,
               contract_violation );
 CHECK( multiSourceDistance( d , {} ).first[ 0 ] == INF );
 CHECK_THROWS( (void) setDistance( d , { true } , { false , false , true } ) ,
               contract_violation );
 CHECK( setDistance( d , { true , false , false } ,
                     { false , false , true } ) == 2 &&
        setDistance( d , { false , false , false } ,
                     { true , true , true } ) == INF );
 ListGraph<> cyc( 2 );
 cyc.insertEdge( 0 , 1 );
 cyc.insertEdge( 1 , 0 );
 CHECK_THROWS( (void) longestPathLayering( cyc ) , contract_violation );
 CHECK_THROWS( (void) dagLongestPaths( cyc ) , contract_violation );
 CHECK( longestPathLayering( none ).empty() &&
        dagLongestPaths( none ).empty() );
 CHECK_THROWS( simpleCycles( u , 0 , none1 ) , contract_violation );
 CHECK_THROWS( simpleCycles( d , 3 , none1 ) , contract_violation );
 int cycles = 0;
 simpleCycles( cyc , 0 , [ & ]( const auto & ) { ++cycles; } );
 CHECK( cycles == 1 );
 using V = std::vector< long >;
 CHECK( refuelingStops( V{} , 0L , 0L ) && ! refuelingStops( V{} , 1L , 0L ) &&
        refuelingStops( V{ 5 } , 10L , 5L )->size() == 1 );
 CHECK_THROWS( (void) refuelingStops( V{} , -1L , 0L ) , contract_violation );
 CHECK_THROWS( (void) refuelingStops( V{} , 1L , -1L ) , contract_violation );
 CHECK_THROWS( (void) refuelingStops( V{ 11 } , 10L , 5L ) ,
               contract_violation );
 CHECK_THROWS( (void) refuelingStops( V{ 3 , 2 } , 10L , 5L ) ,
               contract_violation );
 CHECK_THROWS( (void) refuelingStops( V{ -1 } , 10L , 5L ) ,
               contract_violation );

 CHECK_THROWS( (void) isTree( d ) , contract_violation );
 CHECK_THROWS( (void) isTree( none ) , contract_violation );
 ListGraph<> u1( 1 , false ) , u2( 2 , false ) , tri( 3 , false );
 tri.insertEdge( 0 , 1 );
 tri.insertEdge( 1 , 2 );
 tri.insertEdge( 2 , 0 );
 CHECK( isTree( u1 ) && ! isTree( u2 ) && isTree( u ) && ! isTree( tri ) );
 CHECK_THROWS( (void) diameter( none ) , contract_violation );
 CHECK_THROWS( (void) diameter( u2 ) , contract_violation );   // apart
 CHECK_THROWS( (void) diameter( d ) , contract_violation );    // not all
 CHECK( diameter( u1 ) == 0 && diameter( u ) == 2 );
 CHECK_THROWS( (void) treeDiameter( tri ) , contract_violation );
 CHECK( treeDiameter( u1 ) == 0 && treeDiameter( u ) == 2 );
 CHECK_THROWS( (void) treeVertexCover( d ) , contract_violation );
 CHECK_THROWS( (void) treeVertexCover( tri ) , contract_violation );
 CHECK( treeVertexCover( u1 ).empty() && treeVertexCover( u ).size() == 1 );
 CHECK_THROWS( (void) countTriangles( d ) , contract_violation );
 CHECK( countTriangles( tri ) == 1 && countTriangles( u1 ) == 0 );
 CHECK_THROWS( (void) twoColoring( d ) , contract_violation );
 CHECK( ! twoColoring( tri ) && twoColoring( u ) &&
        twoColoring( ListGraph<>( 0 , false ) ) );
 CHECK_THROWS( (void) universalSink( std::vector< std::vector< int > >{
                { 0 , 1 } , { 0 } } ) , contract_violation );
 CHECK( universalSink( std::vector< std::vector< int > >{ { 0 } } ) == 0 &&
        universalSink( std::vector< std::vector< int > >{ { 0 , 1 } ,
                                                          { 0 , 0 } } ) == 1 );
 CHECK_THROWS( (void) uniqueTopologicalOrder( u ) , contract_violation );
 CHECK( uniqueTopologicalOrder( d ) && ! uniqueTopologicalOrder( cyc ) &&
        uniqueTopologicalOrder( none ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File contract_graph.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
