/*--------------------------------------------------------------------------*/
/*----------------------------- File ch09.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 9, Grafi: the three realizations of the GRAPH against a
 * set of edges on random insertions and deletions, the graphs of the
 * figures and their matrices, the orders of the visits bfs and dfs of the
 * example of figure 9.9, the distances and the paths, the components, the
 * kinds of the edges of figure 9.10, the acyclic graphs, the topological
 * order of the example of tsort and the strongly connected components
 * against the transitive closure, the case of the duke McPollock, and the
 * exercises 9.1 to 9.11, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Edges = std::vector< std::pair< int , int > >;

static_assert( DynamicGraph< ListGraph<> > );
static_assert( DynamicGraph< MatrixGraph<> > );
static_assert( Graph< ArrayGraph<> > );
static_assert( ! DynamicGraph< ArrayGraph<> > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph of n nodes and of the edges E
template< class G = ListGraph<> >
G make( int n , const Edges & E , bool directed )
{
 G g( n , directed );
 for( auto [ u , v ] : E )
  g.insertEdge( u , v );
 return( g );
 }

/// Figure 9.9: the edges in increasing order, so that the lists of
/// adjacency are sorted
const Edges fig99 = { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 0 , 5 } ,
                      { 0 , 7 } , { 1 , 3 } , { 1 , 4 } , { 1 , 6 } ,
                      { 1 , 8 } , { 2 , 5 } , { 3 , 4 } , { 6 , 7 } ,
                      { 6 , 8 } };

/// Figure 9.1, oriented, and figure 9.2, unoriented
const Edges fig91 = { { 0 , 1 } , { 0 , 2 } , { 1 , 3 } , { 2 , 1 } ,
                      { 3 , 1 } , { 3 , 2 } };
const Edges fig92 = { { 0 , 1 } , { 0 , 2 } , { 1 , 3 } , { 2 , 1 } ,
                      { 3 , 2 } };

/// the transitive closure: R[ u ][ v ] if there is a path of at least one
/// edge from u to v
template< Graph G >
std::vector< std::vector< bool > > closure( const G & g )
{
 const int n = g.bound();
 std::vector< std::vector< bool > > R( n , std::vector< bool >( n ) );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { R[ u ][ v ] = true; } );
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   if( R[ i ][ k ] )
    for( int j = 0 ; j < n ; ++j )
     if( R[ k ][ j ] )
      R[ i ][ j ] = true;
 return( R );
 }

/// a random graph of n nodes, each edge with probability p
ListGraph<> random_graph( int n , double p , bool directed , unsigned seed )
{
 std::mt19937 g( seed );
 std::bernoulli_distribution b( p );
 ListGraph<> G( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && b( g ) )
    G.insertEdge( u , v );
 return( G );
 }

/// the path 0 - 1 - ... - n - 1, a degenerate graph for the depth of the
/// visits (10^5 nodes overflowed the stack of a recursive dfs)
ListGraph<> chain( int n , bool directed )
{
 ListGraph<> G( n , directed );
 for( int u = 0 ; u + 1 < n ; ++u )
  G.insertEdge( u , u + 1 );
 return( G );
 }

/// a random acyclic graph: the edges go from a smaller to a larger place of
/// a random permutation
ListGraph<> random_dag( int n , double p , unsigned seed )
{
 std::mt19937 g( seed );
 Ints pi( n );
 for( int i = 0 ; i < n ; ++i )
  pi[ i ] = i;
 std::shuffle( pi.begin() , pi.end() , g );
 std::bernoulli_distribution b( p );
 ListGraph<> G( n , true );
 for( int i = 0 ; i < n ; ++i )
  for( int j = i + 1 ; j < n ; ++j )
   if( b( g ) )
    G.insertEdge( pi[ i ] , pi[ j ] );
 return( G );
 }

/// true if order is a topological order of g
template< Graph G >
bool topological( const G & g , const Ints & order )
{
 if( int( order.size() ) != g.n() )
  return( false );
 std::vector< int > place( g.bound() , -1 );
 for( std::size_t i = 0 ; i < order.size() ; ++i )
  place[ order[ i ] ] = int( i );
 bool ok = true;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ok = ok && place[ u ] < place[ v ]; } );
 return( ok );
 }

/// the distances by Floyd-Warshall with unit weights
template< Graph G >
std::vector< std::vector< int > > all_distances( const G & g )
{
 const int n = g.bound();
 std::vector< std::vector< int > > d( n , std::vector< int >( n , INF ) );
 for( int u = 0 ; u < n ; ++u ) {
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

/// the trace of a visit: a node as "u", an edge as "(u,v)"
using Trace = std::vector< std::string >;

std::string E( int u , int v ) {
 return( "(" + std::to_string( u ) + "," + std::to_string( v ) + ")" );
 }

Trace trace( const char * s )
{
 Trace t;
 std::string w;
 for( const char * c = s ; ; ++c )
  if( ( *c == ' ' ) || ! *c ) {
   if( ! w.empty() )
    t.push_back( w );
   w.clear();
   if( ! *c )
    return( t );
   }
  else
   w += *c;
 }

const Trace bfs99 = trace(
 "0 (0,1) (0,2) (0,3) (0,5) (0,7) 1 (1,0) (1,3) (1,4) (1,6) (1,8) 2 (2,0) "
 "(2,5) 3 (3,0) (3,1) (3,4) 5 (5,0) (5,2) 7 (7,0) (7,6) 4 (4,1) (4,3) 6 "
 "(6,1) (6,7) (6,8) 8 (8,1) (8,6)" );

const Trace dfs99 = trace(
 "0 (0,1) 1 (1,0) (1,3) 3 (3,0) (3,1) (3,4) 4 (4,1) (4,3) (1,4) (1,6) 6 "
 "(6,1) (6,7) 7 (7,0) (7,6) (6,8) 8 (8,1) (8,6) (1,8) (0,2) 2 (2,0) (2,5) "
 "5 (5,0) (5,2) (0,3) (0,5) (0,7)" );

/// the trace of the recursive dfs from r
template< Graph G >
Trace dfs_trace( const G & g , int r )
{
 Trace t;
 std::vector< bool > visited( g.bound() , false );
 dfs( g , r , visited ,
      [ & ]( int u ) { t.push_back( std::to_string( u ) ); } ,
      [ & ]( int u , int v ) { t.push_back( E( u , v ) ); } , []( int ) {} );
 return( t );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// a realization against a set of edges, on random insertions and deletions
// of nodes and edges, with labels and weights; the graph with no node, the
// edge inserted twice or deleted when absent, the deleted names, the
// contracts
template< class G >
void spec( bool directed , unsigned seed )
{
 std::mt19937 r( seed );
 G g( 10 , directed );
 std::set< int > nodes;
 for( int u = 0 ; u < 10 ; ++u )
  nodes.insert( u );
 std::map< std::pair< int , int > , double > edges;
 auto key = [ & ]( int u , int v ) {
  return( directed ? std::pair( u , v )
                   : std::pair( std::min( u , v ) , std::max( u , v ) ) );
  };
 for( int o = 0 ; o < 3000 ; ++o ) {
  const Ints V( nodes.begin() , nodes.end() );
  const int u = V[ r() % V.size() ] , v = V[ r() % V.size() ];
  const int what = r() % 20;
  if( ( what == 0 ) && ( nodes.size() > 2 ) ) {
   g.deleteNode( u );
   nodes.erase( u );
   std::erase_if( edges , [ & ]( const auto & e ) {
    return( ( e.first.first == u ) || ( e.first.second == u ) ); } );
   }
  else if( what == 1 ) {
   const int w = g.insertNode( "n" + std::to_string( o ) );
   CHECK( g.label( w ) == "n" + std::to_string( o ) );
   nodes.insert( w );
   }
  else if( u == v )
   CHECK_THROWS( g.insertEdge( u , v ) , contract_violation );
  else if( what < 12 ) {
   g.insertEdge( u , v , o );
   edges[ key( u , v ) ] = o;
   }
  else {
   g.deleteEdge( u , v );
   edges.erase( key( u , v ) );
   }
  CHECK( g.n() == int( nodes.size() ) && g.m() == int( edges.size() ) );
  }
 CHECK( g.V() == Ints( nodes.begin() , nodes.end() ) );
 for( int u : nodes ) {
  std::set< int > adj;
  for( const auto & [ e , w ] : edges ) {
   if( e.first == u ) adj.insert( e.second );
   if( ! directed && ( e.second == u ) ) adj.insert( e.first );
   }
  auto a = g.adj( u );
  std::sort( a.begin() , a.end() );
  CHECK( a == Ints( adj.begin() , adj.end() ) );
  for( int v : adj ) {
   CHECK( g.hasEdge( u , v ) );
   CHECK( g.weight( u , v ) == edges.at( key( u , v ) ) );
   }
  }
 // the copy into vectors of adjacency
 const ArrayGraph< double > a( g );
 for( int u : nodes ) {
  auto x = g.adj( u ) , y = a.adj( u );
  std::sort( x.begin() , x.end() );
  std::sort( y.begin() , y.end() );
  CHECK( x == y );
  }
 CHECK_THROWS( (void) g.adj( g.bound() ) , contract_violation );

 // the degenerate cases: no node, an edge inserted twice (its weight
 // replaced, counted once), an absent one deleted (nothing), the queries
 // on names that are not nodes, the names of the deleted nodes not reused
 // and isolated in the copy, the contracts of each operation
 G e( 0 , directed );
 CHECK( e.n() == 0 && e.m() == 0 && e.bound() == 0 && e.V().empty() );
 CHECK( e.insertNode() == 0 && e.insertNode() == 1 && e.n() == 2 );
 e.insertEdge( 0 , 1 , 1 );
 e.insertEdge( 0 , 1 , 2 );
 CHECK( e.m() == 1 && e.weight( 0 , 1 ) == 2.0 );
 CHECK( e.hasEdge( 1 , 0 ) == ! directed );
 if( ! directed )
  CHECK( e.weight( 1 , 0 ) == 2.0 );
 e.deleteEdge( 1 , 0 );
 CHECK( e.m() == ( directed ? 1 : 0 ) );
 CHECK( ! e.hasEdge( 0 , 5 ) && ! e.hasEdge( -1 , 0 ) && ! e.weight( 0 , 5 ) );
 e.deleteNode( 0 );
 CHECK( e.n() == 1 && e.m() == 0 && e.bound() == 2 && ! e.hasNode( 0 ) );
 CHECK( e.insertNode() == 2 && e.V() == Ints( { 1 , 2 } ) );
 const ArrayGraph< double > h( e );
 CHECK( h.n() == 3 && h.m() == 0 && h.adj( 0 ).empty() );
 CHECK_THROWS( e.deleteNode( 0 ) , contract_violation );
 CHECK_THROWS( (void) e.label( 0 ) , contract_violation );
 CHECK_THROWS( e.setLabel( 0 , "x" ) , contract_violation );
 CHECK_THROWS( e.insertEdge( 0 , 1 ) , contract_violation );
 CHECK_THROWS( e.insertEdge( 1 , 7 ) , contract_violation );
 CHECK_THROWS( e.deleteEdge( 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) e.adj( 0 ) , contract_violation );
 CHECK_THROWS( G( -1 , directed ) , contract_violation );
 }

ADS_TEST( "spec.ListGraph" )
{
 spec< ListGraph< double , std::string > >( true , 1 );
 spec< ListGraph< double , std::string > >( false , 2 );
 }

ADS_TEST( "spec.MatrixGraph" )
{
 spec< MatrixGraph< double , std::string > >( true , 3 );
 spec< MatrixGraph< double , std::string > >( false , 4 );
 }

/*--------------------------------------------------------------------------*/

// the graphs of Figure 9.1 and 9.2: degrees, paths, circuits, components
ADS_TEST( "sec9.1.definitions" )
{
 const auto G = make( 4 , fig91 , true );
 CHECK( G.n() == 4 && G.m() == 6 );
 auto is_path = [ & ]( const Ints & p ) {
  for( std::size_t i = 0 ; i + 1 < p.size() ; ++i )
   if( ! G.hasEdge( p[ i ] , p[ i + 1 ] ) )
    return( false );
  return( true );
  };
 CHECK( is_path( { 0 , 1 , 3 , 2 , 1 } ) && is_path( { 1 , 3 , 2 , 1 } ) );
 CHECK( ! is_path( { 3 , 0 } ) );
 const auto R = closure( G );
 CHECK( R[ 0 ][ 3 ] && ! R[ 3 ][ 0 ] );             // not strongly connected
 const auto id = scc( G );
 CHECK( id[ 1 ] == id[ 2 ] && id[ 2 ] == id[ 3 ] && id[ 0 ] != id[ 1 ] );

 const auto U = make( 4 , fig92 , false );
 CHECK( U.n() == 4 && U.m() == 5 );
 CHECK( U.hasEdge( 0 , 2 ) && U.hasEdge( 2 , 0 ) && ! U.hasEdge( 0 , 3 ) );
 CHECK( cc( U ) == Ints( { 1 , 1 , 1 , 1 } ) );       // connected
 CHECK( U.m() > U.n() - 1 );                          // not a free tree
 }

// the case of the duke McPollock: an interval graph is chordal (each
// circuit of length at least 4 has a chord), the graph of the meetings is
// not, and Ann is the only woman whose removal makes it chordal; chordal
// by the maximum cardinality search of Tarjan and Yannakakis (SIAM J.
// Comput. 13(3), 566-579, 1984); the empty graph, one node, a circuit of 4
// with and without chords
bool chordal( const MatrixGraph<> & g )
{
 const Ints V = g.V();
 std::vector< int > weight( g.bound() , 0 ) , place( g.bound() , -1 );
 Ints order;                                       // reversed elimination
 for( std::size_t i = 0 ; i < V.size() ; ++i ) {
  int best = -1;
  for( int u : V )
   if( ( place[ u ] < 0 ) &&
       ( ( best < 0 ) || ( weight[ u ] > weight[ best ] ) ) )
    best = u;
  place[ best ] = int( i );
  order.push_back( best );
  g.forEachAdj( best , [ & ]( int v ) {
   if( place[ v ] < 0 )
    ++weight[ v ];
   } );
  }
 // the earlier neighbours of each node must be a clique: enough to check
 // that they are adjacent to the latest of them
 for( int u : order ) {
  Ints before;
  g.forEachAdj( u , [ & ]( int v ) {
   if( place[ v ] < place[ u ] )
    before.push_back( v );
   } );
  if( before.empty() )
   continue;
  const int w = *std::max_element( before.begin() , before.end() ,
   [ & ]( int a , int b ) { return( place[ a ] < place[ b ] ); } );
  for( int v : before )
   if( ( v != w ) && ! g.hasEdge( v , w ) )
    return( false );
  }
 return( true );
 }

ADS_TEST( "sec9.1.mcpollock" )
{
 const std::string who = "ABCEFGH";
 auto I = [ & ]( char c ) { return( int( who.find( c ) ) ); };
 MatrixGraph<> g( 7 , false );
 for( const char * e : { "AB" , "AC" , "AF" , "AG" , "BC" , "BE" , "BF" ,
                         "BH" , "CE" , "EF" , "FH" , "GH" } )
  g.insertEdge( I( e[ 0 ] ) , I( e[ 1 ] ) );
 // the testimonies are symmetric: Betty met Ann, Charlotte, Edith, ...
 CHECK( g.adj( I( 'B' ) ) == Ints( { 0 , 2 , 3 , 4 , 6 } ) );
 CHECK( ! chordal( g ) );
 for( char c : who ) {
  auto h = g;
  h.deleteNode( I( c ) );
  CHECK( chordal( h ) == ( c == 'A' ) );
  }
 // the degenerate graphs: no node, one, a triangle and a complete graph
 // are chordal, a circuit of 4 is not until a chord is added
 CHECK( chordal( MatrixGraph<>( 0 , false ) ) );
 CHECK( chordal( MatrixGraph<>( 1 , false ) ) );
 auto k = make< MatrixGraph<> >( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                                       { 3 , 0 } } , false );
 CHECK( ! chordal( k ) );
 k.insertEdge( 0 , 2 );
 CHECK( chordal( k ) );
 k.insertEdge( 1 , 3 );
 CHECK( chordal( k ) );
 }

/*--------------------------------------------------------------------------*/

// the matrices of Figure 9.6 and 9.7 and the vectors of figure 9.8; the
// lists read back from the incidences, the graphs with no edge or no node
ADS_TEST( "sec9.3.matrices" )
{
 const auto U = make< MatrixGraph<> >( 4 , fig92 , false );
 const std::vector< Ints > M = { { 0 , 1 , 1 , 0 } , { 1 , 0 , 1 , 1 } ,
                                 { 1 , 1 , 0 , 1 } , { 0 , 1 , 1 , 0 } };
 for( int u = 0 ; u < 4 ; ++u )
  for( int v = 0 ; v < 4 ; ++v )
   CHECK( U.hasEdge( u , v ) == bool( M[ u ][ v ] ) );

 // each column has a -1 and a +1, oriented, or two 1, unoriented
 for( bool d : { true , false } ) {
  const auto B = incidence( make( 4 , d ? fig91 : fig92 , d ) );
  CHECK( B.size() == 4 && B[ 0 ].size() == ( d ? 6u : 5u ) );
  for( std::size_t k = 0 ; k < B[ 0 ].size() ; ++k ) {
   Ints c;
   for( int u = 0 ; u < 4 ; ++u )
    if( B[ u ][ k ] )
     c.push_back( B[ u ][ k ] );
   std::sort( c.begin() , c.end() );
   CHECK( c == ( d ? Ints( { -1 , 1 } ) : Ints( { 1 , 1 } ) ) );
   }
  }

 const ArrayGraph<> A( U );
 CHECK( A.adj( 0 ) == Ints( { 1 , 2 } ) );
 CHECK( A.adj( 1 ) == Ints( { 0 , 2 , 3 } ) );
 CHECK( A.adj( 2 ) == Ints( { 0 , 1 , 3 } ) );
 CHECK( A.adj( 3 ) == Ints( { 1 , 2 } ) );
 CHECK( A.m() == 5 );

 // the rows of incidence give the lists back; a graph with no edge has a
 // matrix with no column, one with no node an empty matrix and an empty
 // array of vectors
 for( bool d : { true , false } ) {
  const auto g = make( 4 , d ? fig91 : fig92 , d );
  const auto B = incidence( g );
  for( int u = 0 ; u < 4 ; ++u ) {
   auto a = adjFromIncidence( B , u , d ) , b = g.adj( u );
   std::sort( a.begin() , a.end() );
   std::sort( b.begin() , b.end() );
   CHECK( a == b );
   }
  CHECK_THROWS( (void) adjFromIncidence( B , 4 , d ) , contract_violation );
  CHECK_THROWS( (void) adjFromIncidence( B , -1 , d ) , contract_violation );
  const auto Z = incidence( ListGraph<>( 3 , d ) );
  CHECK( Z.size() == 3 && Z[ 0 ].empty() );
  CHECK( adjFromIncidence( Z , 2 , d ).empty() );
  CHECK( incidence( ListGraph<>( 0 , d ) ).empty() );
  }
 const ArrayGraph<> N( 0 , {} );
 CHECK( N.n() == 0 && N.m() == 0 && N.V().empty() );
 CHECK_THROWS( (void) N.adj( 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

// the orders of the example bfs, and the Erdos numbers with their paths;
// an isolated root, a root that is not an ancestor, parents with a cycle,
// a path of 10^5 nodes, the contracts
ADS_TEST( "sec9.5.1.bfs" )
{
 auto g = make( 9 , fig99 , false );
 Trace t;
 bfs( g , 0 , [ & ]( int u ) { t.push_back( std::to_string( u ) ); } ,
      [ & ]( int u , int v ) { t.push_back( E( u , v ) ); } );
 CHECK( t == bfs99 );

 g.insertNode();                                    // an author alone
 const auto [ d , p ] = distance( g , 0 );
 CHECK( d == Ints( { 0 , 1 , 1 , 1 , 2 , 1 , 2 , 1 , 2 , INF } ) );
 CHECK( path( 0 , 8 , p ) == Ints( { 0 , 1 , 8 } ) );
 CHECK( path( 0 , 0 , p ) == Ints( { 0 } ) );
 CHECK( path( 0 , 9 , p ).empty() );
 for( unsigned s = 0 ; s < 20 ; ++s ) {             // against Floyd-Warshall
  const auto h = random_graph( 25 , 0.08 , s % 2 , s );
  const auto D = all_distances( h );
  for( int r = 0 ; r < 25 ; r += 6 ) {
   const auto [ dd , pp ] = distance( h , r );
   for( int v = 0 ; v < 25 ; ++v ) {
    CHECK( dd[ v ] == D[ r ][ v ] );
    if( dd[ v ] < INF )
     CHECK( int( path( r , v , pp ).size() ) == dd[ v ] + 1 );
    }
   }
  }

 // the degenerate cases: a visit from an isolated node sees only it; a
 // root r that is not an ancestor of s gives no path; parents with a
 // cycle give none instead of looping; a path of 10^5 nodes (the
 // recursive path() overflowed the stack); the contracts
 Trace one;
 bfs( g , 9 , [ & ]( int u ) { one.push_back( std::to_string( u ) ); } ,
      [ & ]( int u , int v ) { one.push_back( E( u , v ) ); } );
 CHECK( one == Trace( { "9" } ) );
 const auto [ d9 , p9 ] = distance( g , 9 );
 CHECK( d9[ 9 ] == 0 && d9[ 0 ] == INF && p9 == Ints( 10 , NIL ) );
 CHECK( path( 3 , 8 , p ).empty() && path( 1 , 8 , p ) == Ints( { 1 , 8 } ) );
 CHECK( path( 0 , 2 , Ints( { NIL , 2 , 1 } ) ).empty() );
 const int n = 100000;
 const auto c = chain( n , true );
 const auto [ dc , pc ] = distance( c , 0 );
 CHECK( dc[ n - 1 ] == n - 1 &&
        path( 0 , n - 1 , pc ).size() == std::size_t( n ) );
 CHECK( path( 1 , 0 , pc ).empty() );
 CHECK( distance( c , n - 1 ).first[ 0 ] == INF );
 g.deleteNode( 4 );
 auto none = []( auto... ) {};
 CHECK_THROWS( bfs( g , 4 , none , none ) , contract_violation );
 CHECK_THROWS( bfs( g , 10 , none , none ) , contract_violation );
 CHECK_THROWS( (void) distance( g , -1 ) , contract_violation );
 CHECK_THROWS( (void) path( 0 , 10 , p ) , contract_violation );
 CHECK_THROWS( (void) path( -1 , 0 , p ) , contract_violation );
 CHECK_THROWS( (void) path( 0 , 1 , Ints( { NIL , 7 } ) ) ,
               contract_violation );
 }

// the order of the example dfs; a single node, the previsits and the
// postvisits on a path of 10^5 nodes (the recursive dfs overflowed the
// stack), its examinations of each unoriented edge from both ends, the
// contracts
ADS_TEST( "sec9.5.4.dfs" )
{
 CHECK( dfs_trace( make( 9 , fig99 , false ) , 0 ) == dfs99 );
 CHECK( dfs_trace( ListGraph<>( 1 ) , 0 ) == Trace( { "0" } ) );
 for( bool d : { true , false } ) {
  const int n = 100000;
  const auto c = chain( n , d );
  std::vector< bool > visited( n , false );
  Ints pre , post;
  long edges = 0;
  dfs( c , 0 , visited , [ & ]( int u ) { pre.push_back( u ); } ,
       [ & ]( int , int ) { ++edges; } ,
       [ & ]( int u ) { post.push_back( u ); } );
  CHECK( int( pre.size() ) == n && int( post.size() ) == n );
  CHECK( pre.front() == 0 && pre.back() == n - 1 );
  CHECK( post.front() == n - 1 && post.back() == 0 );
  CHECK( std::is_sorted( pre.begin() , pre.end() ) );
  CHECK( edges == ( d ? 1L : 2L ) * ( n - 1 ) );
  }
 const auto g = make( 3 , { { 0 , 1 } } , true );
 std::vector< bool > visited( 3 , false );
 auto none = []( auto... ) {};
 dfs( g , 0 , visited , none , none , none );
 CHECK( visited == std::vector< bool >( { true , true , false } ) );
 CHECK_THROWS( dfs( g , 1 , visited , none , none , none ) ,
               contract_violation );
 std::vector< bool > shorter( 2 , false );
 CHECK_THROWS( dfs( g , 2 , shorter , none , none , none ) ,
               contract_violation );
 CHECK_THROWS( dfs( g , 3 , visited , none , none , none ) ,
               contract_violation );
 }

// the components of a graph like the one of figure 9.3 (its edges are not
// in the text: these give the order of visit 0, 1, 4, 5, 2, 6, 7, 3 of the
// example), and the dependence of cc() on the order of the stack in an
// oriented graph; no node, no edge, a deleted node, an empty stack and one
// with repetitions, a path of 10^5 nodes
ADS_TEST( "sec9.5.5.cc" )
{
 const auto g = make( 8 , { { 0 , 1 } , { 1 , 4 } , { 1 , 5 } , { 4 , 5 } ,
                            { 2 , 6 } , { 6 , 7 } } , false );
 Ints order;
 std::vector< bool > visited( 8 , false );
 for( int u = 0 ; u < 8 ; ++u )
  if( ! visited[ u ] )
   dfs( g , u , visited , [ & ]( int v ) { order.push_back( v ); } ,
        []( int , int ) {} , []( int ) {} );
 CHECK( order == Ints( { 0 , 1 , 4 , 5 , 2 , 6 , 7 , 3 } ) );
 CHECK( cc( g ) == Ints( { 1 , 1 , 2 , 3 , 1 , 1 , 2 , 2 } ) );

 const auto G = make( 4 , fig91 , true );
 CHECK( cc( G , { 3 , 2 , 0 , 1 } ) == Ints( { 2 , 1 , 1 , 1 } ) );
 CHECK( cc( G , { 3 , 2 , 1 , 0 } ) == Ints( { 1 , 1 , 1 , 1 } ) );

 for( unsigned s = 0 ; s < 20 ; ++s ) {             // against the closure
  const auto h = random_graph( 30 , 0.05 , false , s );
  const auto R = closure( h );
  const auto id = cc( h );
  for( int u = 0 ; u < 30 ; ++u )
   for( int v = 0 ; v < 30 ; ++v )
    if( u != v )
     CHECK( ( id[ u ] == id[ v ] ) == bool( R[ u ][ v ] ) );
  }

 // the degenerate graphs: no node, no edge (a component per node), a
 // deleted node (component 0), an empty stack (no visit) and one with
 // repetitions, a path of 10^5 nodes (one component, no overflow)
 CHECK( cc( ListGraph<>( 0 , false ) ).empty() );
 CHECK( cc( ListGraph<>( 4 , false ) ) == Ints( { 1 , 2 , 3 , 4 } ) );
 auto h = g;
 h.deleteNode( 1 );
 CHECK( cc( h ) == Ints( { 1 , 0 , 2 , 3 , 4 , 4 , 2 , 2 } ) );
 CHECK( cc( g , {} ) == Ints( 8 , 0 ) );
 CHECK( cc( g , { 3 , 2 , 3 , 0 , 0 } ) ==
        Ints( { 1 , 1 , 3 , 2 , 1 , 1 , 3 , 3 } ) );
 const int n = 100000;
 const auto c = cc( chain( n , false ) );
 CHECK( std::all_of( c.begin() , c.end() ,
                     []( int x ) { return( x == 1 ); } ) );
 }

// the kinds of the edges of figure 9.10; in an unoriented graph the first
// examination of each edge is in T or back; the intervals [ dt , ft ] are
// nested or disjoint; a single node, a circuit of 2, a path of 10^5 nodes,
// the contracts
ADS_TEST( "sec9.5.6.schema" )
{
 const auto G = make( 4 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 1 , 2 } ,
                            { 2 , 0 } , { 3 , 1 } , { 3 , 2 } } , true );
 DfsSchema S( G );
 std::map< std::pair< int , int > , EdgeKind > kind;
 Trace t;
 S.visit( 0 , [ & ]( int u , int v , EdgeKind k ) {
  kind[ { u , v } ] = k;
  t.push_back( E( u , v ) );
  } );
 CHECK( t == trace( "(0,1) (1,2) (2,0) (0,2) (0,3) (3,1) (3,2)" ) );
 using enum EdgeKind;
 auto K = [ & ]( int u , int v ) { return( kind[ std::pair( u , v ) ] ); };
 CHECK( K( 0 , 1 ) == TREE && K( 1 , 2 ) == TREE && K( 0 , 3 ) == TREE );
 CHECK( K( 0 , 2 ) == FORWARD && K( 2 , 0 ) == BACK );
 CHECK( K( 3 , 1 ) == CROSS && K( 3 , 2 ) == CROSS );

 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const bool d = s % 2;
  const auto h = random_graph( 30 , 0.1 , d , s );
  DfsSchema T( h );
  std::set< std::pair< int , int > > seen;
  T.visitAll( [ & ]( int u , int v , EdgeKind k ) {
   if( ! d && seen.insert( { std::min( u , v ) , std::max( u , v ) } ).second )
    CHECK( k == TREE || k == BACK );
   } );
  for( int u = 0 ; u < 30 ; ++u )
   for( int v = 0 ; v < 30 ; ++v ) {
    const bool disjoint = T.ft( u ) < T.dt( v ) || T.ft( v ) < T.dt( u );
    const bool nested = ( T.dt( u ) <= T.dt( v ) && T.ft( v ) <= T.ft( u ) ) ||
                        ( T.dt( v ) <= T.dt( u ) && T.ft( u ) <= T.ft( v ) );
    CHECK( disjoint || nested );
    }
  }

 // the degenerate cases: a single node; a circuit of 2, whose second edge
 // is back, as is the return of an unoriented edge; a path of 10^5 nodes
 // (no overflow), with dt( u ) = u + 1 and ft( u ) = 2 n - u; the
 // contracts
 const ListGraph<> o( 1 );
 DfsSchema O( o );
 O.visitAll( []( int , int , EdgeKind ) {} );
 CHECK( O.dt( 0 ) == 1 && O.ft( 0 ) == 2 );
 for( bool d : { true , false } ) {
  const auto c2 = d ? make( 2 , { { 0 , 1 } , { 1 , 0 } } , true )
                    : make( 2 , { { 0 , 1 } } , false );
  DfsSchema C( c2 );
  std::vector< EdgeKind > k;
  C.visit( 0 , [ & ]( int , int , EdgeKind e ) { k.push_back( e ); } );
  CHECK( k == std::vector< EdgeKind >( { TREE , BACK } ) );
  CHECK_THROWS( C.visit( 1 , []( int , int , EdgeKind ) {} ) ,
                contract_violation );
  CHECK_THROWS( (void) C.dt( 2 ) , contract_violation );
  CHECK_THROWS( (void) C.ft( -1 ) , contract_violation );
  }
 const int n = 100000;
 const auto c = chain( n , true );
 DfsSchema P( c );
 long tree = 0;
 P.visitAll( [ & ]( int , int , EdgeKind e ) { tree += e == TREE; } );
 CHECK( tree == n - 1 && P.dt( 0 ) == 1 && P.ft( 0 ) == 2 * n );
 CHECK( P.dt( n - 1 ) == n && P.ft( n - 1 ) == n + 1 );
 }

// a graph has a cycle if and only if a node reaches itself; no node, a
// circuit of 2, an unoriented edge (a cycle, as documented), the cycle in
// the last component, a path of 10^5 nodes without and with its closing
// edge
ADS_TEST( "sec9.5.7.cycle" )
{
 CHECK( hasCycle( make( 4 , fig91 , true ) ) );
 CHECK( ! hasCycle( make( 3 , { { 0 , 1 } , { 1 , 2 } , { 0 , 2 } } ,
                          true ) ) );
 CHECK( ! hasCycle( ListGraph<>( 5 ) ) && ! hasCycle( ListGraph<>( 0 ) ) );
 CHECK( hasCycle( make( 2 , { { 0 , 1 } , { 1 , 0 } } , true ) ) );
 CHECK( hasCycle( make( 2 , { { 0 , 1 } } , false ) ) );    // documented
 CHECK( ! hasCycle( ListGraph<>( 3 , false ) ) );
 // the cycle in the last component, and a path of 10^5 nodes without and
 // with the edge that closes it (the recursive visit overflowed)
 CHECK( hasCycle( make( 5 , { { 0 , 1 } , { 2 , 3 } , { 3 , 4 } ,
                              { 4 , 2 } } , true ) ) );
 auto c = chain( 100000 , true );
 CHECK( ! hasCycle( c ) );
 c.insertEdge( 99999 , 0 );
 CHECK( hasCycle( c ) );
 for( unsigned s = 0 ; s < 40 ; ++s ) {
  const auto h = random_graph( 20 , 0.04 , true , s );
  const auto R = closure( h );
  bool cyc = false;
  for( int u = 0 ; u < 20 ; ++u )
   cyc = cyc || R[ u ][ u ];
  CHECK( hasCycle( h ) == cyc );
  CHECK( ! hasCycle( random_dag( 20 , 0.3 , s ) ) );
  }
 }

// the example of tsort: the order of the book is topological, and so are
// the ones of topsort() and of topsortKahn(); with a cycle Kahn stops short;
// no node, no edge, a circuit through all the nodes, a path of 10^5 nodes,
// the orders that are not topological
ADS_TEST( "sec9.5.8.topsort" )
{
 ListGraph<> g( 12 , true );
 for( int u : { 0 , 1 , 4 , 6 } )
  g.deleteNode( u );
 for( auto [ u , v ] : Edges( { { 3 , 8 } , { 3 , 10 } , { 5 , 11 } ,
                                { 7 , 8 } , { 7 , 11 } , { 8 , 9 } ,
                                { 11 , 2 } , { 11 , 9 } , { 11 , 10 } } ) )
  g.insertEdge( u , v );
 CHECK( topological( g , { 3 , 5 , 7 , 11 , 8 , 10 , 2 , 9 } ) );
 CHECK( topological( g , topsort( g ) ) );
 CHECK( topological( g , topsortKahn( g ) ) );
 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const auto h = random_dag( 40 , 0.1 , s );
  CHECK( topological( h , topsort( h ) ) );
  CHECK( topological( h , topsortKahn( h ) ) );
  }
 CHECK( topsortKahn( make( 4 , fig91 , true ) ).size() == 1 );

 // the degenerate graphs: no node, no edge (every order is topological),
 // a circuit through all the nodes (Kahn finds no source), a path of 10^5
 // nodes, whose only order is 0 , ... , n - 1; the orders that are not
 // topological: a node twice, a node missing, a name that is not a node
 CHECK( topsort( ListGraph<>( 0 ) ).empty() );
 CHECK( topsortKahn( ListGraph<>( 0 ) ).empty() );
 CHECK( topsort( ListGraph<>( 3 ) ).size() == 3 );
 CHECK( topsortKahn( ListGraph<>( 3 ) ) == Ints( { 0 , 1 , 2 } ) );
 const auto c3 = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , true );
 CHECK( topsortKahn( c3 ).empty() && topsort( c3 ).size() == 3 );
 CHECK( ! isTopologicalOrder( c3 , topsort( c3 ) ) );
 const int n = 100000;
 const auto c = chain( n , true );
 Ints iota( n );
 for( int i = 0 ; i < n ; ++i )
  iota[ i ] = i;
 CHECK( topsort( c ) == iota && topsortKahn( c ) == iota );
 CHECK( isTopologicalOrder( g , { 3 , 5 , 7 , 11 , 8 , 10 , 2 , 9 } ) );
 CHECK( ! isTopologicalOrder( g , { 3 , 5 , 7 , 11 , 8 , 10 , 2 , 2 } ) );
 CHECK( ! isTopologicalOrder( g , { 3 , 5 , 7 , 11 , 8 , 10 , 2 } ) );
 CHECK( ! isTopologicalOrder( g , { 3 , 5 , 7 , 11 , 8 , 10 , 2 , 0 } ) );
 CHECK( isTopologicalOrder( g , { 5 , 7 , 11 , 2 , 3 , 8 , 9 , 10 } ) );
 CHECK( ! isTopologicalOrder( g , { 3 , 5 , 7 , 11 , 2 , 9 , 8 , 10 } ) );
 }

// the strongly connected components against the closure: u and v are in
// the same component if and only if each reaches the other; no node, no
// edge, a deleted node, a circuit of 2, a path of 10^5 nodes open and
// closed
ADS_TEST( "sec9.5.9.scc" )
{
 for( unsigned s = 0 ; s < 30 ; ++s ) {
  const auto h = random_graph( 30 , 0.04 + 0.002 * s , true , s );
  const auto R = closure( h );
  const auto id = scc( h );
  for( int u = 0 ; u < 30 ; ++u )
   for( int v = 0 ; v < 30 ; ++v )
    if( u != v )
     CHECK( ( id[ u ] == id[ v ] ) == ( R[ u ][ v ] && R[ v ][ u ] ) );
  }

 // the degenerate graphs: no node, no edge (a component per node), a
 // deleted node (component 0), a circuit of 2; a path of 10^5 nodes (a
 // component per node, numbered along the path) and the same path closed
 // into a circuit (one component)
 CHECK( scc( ListGraph<>( 0 ) ).empty() );
 auto e = scc( ListGraph<>( 4 ) );
 std::sort( e.begin() , e.end() );
 CHECK( e == Ints( { 1 , 2 , 3 , 4 } ) );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } } , true );
 CHECK( scc( h ) == Ints( { 1 , 1 , 2 } ) );
 h.deleteNode( 0 );
 CHECK( scc( h ) == Ints( { 0 , 1 , 2 } ) );
 const int n = 100000;
 auto c = chain( n , true );
 const auto id = scc( c );
 bool along = true;
 for( int u = 0 ; u < n ; ++u )
  along = along && ( id[ u ] == u + 1 );
 CHECK( along );
 c.insertEdge( n - 1 , 0 );
 const auto one = scc( c );
 CHECK( std::all_of( one.begin() , one.end() ,
                     []( int x ) { return( x == 1 ); } ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// with d bits per pointer and integer, the matrix takes n^2 bits and the
// lists d n + 2 d m, less when m < ( n^2 - d n ) / ( 2 d ); the ends m = 0
// and m = n ( n - 1 ), and n <= d
ADS_TEST( "ex9.1" )
{
 for( long n : { 10 , 100 , 1000 } )
  for( long d : { 32 , 64 } )
   for( long m = 0 ; m <= n * ( n - 1 ) ; m += std::max( 1L , n / 7 ) ) {
    const bool lists = d * n + 2 * d * m < n * n;
    CHECK( lists == ( 2 * d * m < n * n - d * n ) );
    }
 // the ends: with no edge the lists are smaller exactly when n > d, with
 // all the n ( n - 1 ) edges never (for d >= 1); with n <= d never
 for( long n : { 1 , 32 , 33 , 64 , 65 , 1000 } )
  for( long d : { 32 , 64 } ) {
   CHECK( ( d * n < n * n ) == ( n > d ) );
   CHECK( ! ( d * n + 2 * d * n * ( n - 1 ) < n * n ) );
   }
 }

// the iterative dfs, with a stack of nodes and of places in their lists,
// in the order of the recursive one
template< Graph G >
Trace dfs_iterative( const G & g , int r )
{
 Trace t;
 std::vector< bool > visited( g.bound() , false );
 std::vector< std::pair< int , std::size_t > > S;   // node, next place
 std::vector< Ints > adj( g.bound() );
 auto discover = [ & ]( int u ) {
  visited[ u ] = true;
  t.push_back( std::to_string( u ) );
  adj[ u ] = g.adj( u );
  S.emplace_back( u , 0 );
  };
 discover( r );
 while( ! S.empty() ) {
  auto & [ u , i ] = S.back();
  if( i == adj[ u ].size() ) {
   S.pop_back();
   continue;
   }
  const int v = adj[ u ][ i++ ];
  t.push_back( E( u , v ) );
  if( ! visited[ v ] )
   discover( v );
  }
 return( t );
 }

ADS_TEST( "ex9.2" )
{
 CHECK( dfs_iterative( make( 9 , fig99 , false ) , 0 ) == dfs99 );
 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const auto h = random_graph( 40 , 0.08 , s % 2 , s );
  CHECK( dfs_iterative( h , 0 ) == dfs_trace( h , 0 ) );
  }
 // a single node, a root with no edge, a deleted node, a path of 10^5
 // nodes from either end
 CHECK( dfs_iterative( ListGraph<>( 1 ) , 0 ) == Trace( { "0" } ) );
 CHECK( dfs_iterative( ListGraph<>( 3 ) , 2 ) == Trace( { "2" } ) );
 auto f = make( 9 , fig99 , false );
 f.deleteNode( 1 );
 CHECK( dfs_iterative( f , 0 ) == dfs_trace( f , 0 ) );
 const auto c = chain( 100000 , false );
 CHECK( dfs_iterative( c , 0 ) == dfs_trace( c , 0 ) );
 CHECK( dfs_iterative( c , 99999 ) == dfs_trace( c , 99999 ) );
 }

// bipartire() of the solution, on every component; the book's test
// color[ v ] != color[ u ] must be ==, two adjacent nodes of the same color
template< Graph G >
bool bipartite( const G & g )
{
 std::vector< int > color( g.bound() , -1 );
 for( int r : g.V() ) {
  if( color[ r ] >= 0 )
   continue;
  ArrayQueue< int > S( g.n() );
  S.enqueue( r );
  color[ r ] = 0;
  while( ! S.empty() ) {
   const int u = S.dequeue();
   bool ok = true;
   g.forEachAdj( u , [ & ]( int v ) {
    if( color[ v ] < 0 ) {
     color[ v ] = 1 - color[ u ];
     S.enqueue( v );
     }
    else if( color[ v ] == color[ u ] )
     ok = false;
    } );
   if( ! ok )
    return( false );
   }
  }
 return( true );
 }

// bipartite iff 2-colorable iff no odd circuit, on small random graphs: the
// colorings by enumeration, the odd circuits as the closed walks of odd
// length (an odd closed walk contains an odd circuit), found as the paths
// from ( u , 0 ) to ( u , 1 ) in the graph of the pairs ( node , parity )
ADS_TEST( "ex9.3" )
{
 for( unsigned s = 0 ; s < 200 ; ++s ) {
  const int n = 8;
  const auto g = random_graph( n , 0.25 , false , s );
  bool colorable = false;
  for( int c = 0 ; c < ( 1 << n ) && ! colorable ; ++c ) {
   bool ok = true;
   for( int u = 0 ; u < n ; ++u )
    g.forEachAdj( u , [ & ]( int v ) {
     ok = ok && ( ( ( c >> u ) & 1 ) != ( ( c >> v ) & 1 ) );
     } );
   colorable = ok;
   }
  ListGraph<> P( 2 * n , true );
  for( int u = 0 ; u < n ; ++u )
   g.forEachAdj( u , [ & ]( int v ) {
    P.insertEdge( 2 * u , 2 * v + 1 );
    P.insertEdge( 2 * u + 1 , 2 * v );
    } );
  bool odd = false;
  for( int u = 0 ; u < n ; ++u )
   odd = odd || ( distance( P , 2 * u ).first[ 2 * u + 1 ] < INF );
  CHECK( colorable == ! odd );
  CHECK( bipartite( g ) == colorable );
  }
 }

ADS_TEST( "ex9.4" )
{
 CHECK( bipartite( make( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                               { 3 , 0 } } , false ) ) );
 CHECK( ! bipartite( make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } ,
                           false ) ) );
 // the odd circuit in the second component: a bfs from 0 alone misses it
 CHECK( ! bipartite( make( 5 , { { 0 , 1 } , { 2 , 3 } , { 3 , 4 } ,
                                 { 4 , 2 } } , false ) ) );
 CHECK( bipartite( make( 9 , fig99 , false ) ) == false );   // 0-1-3
 CHECK( bipartite( ListGraph<>( 3 , false ) ) );
 // no node, one edge, a deleted node breaking the odd circuit, circuits of
 // 1000 and 1001 nodes
 CHECK( bipartite( ListGraph<>( 0 , false ) ) );
 CHECK( bipartite( make( 2 , { { 0 , 1 } } , false ) ) );
 auto t = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , false );
 t.deleteNode( 1 );
 CHECK( bipartite( t ) );
 for( int n : { 1000 , 1001 } ) {
  auto c = chain( n , false );
  c.insertEdge( n - 1 , 0 );
  CHECK( bipartite( c ) == ( n % 2 == 0 ) );
  }
 }

// the width of a rooted tree, the largest number of nodes on a level, by a
// bfs that counts the nodes at each distance from the root; a single node,
// a chain, a star, a root inside the tree
ADS_TEST( "ex9.5" )
{
 auto width = []( const ListGraph<> & t , int r ) {
  const auto d = distance( t , r ).first;
  std::map< int , int > level;
  for( int x : d )
   if( x < INF )
    ++level[ x ];
  int w = 0;
  for( auto [ l , c ] : level )
   w = std::max( w , c );
  return( w );
  };
 // a root, 3 children, 5 grandchildren, 1 great-grandchild
 const auto t = make( 10 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 1 , 4 } ,
                             { 1 , 5 } , { 2 , 6 } , { 3 , 7 } , { 3 , 8 } ,
                             { 8 , 9 } } , true );
 CHECK( width( t , 0 ) == 5 );
 CHECK( width( ListGraph<>( 1 ) , 0 ) == 1 );
 const auto chain = make( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } } , true );
 CHECK( width( chain , 0 ) == 1 );
 // a star, all the nodes but the root on one level; a root in the middle
 // of the tree sees only its subtree
 ListGraph<> star( 1001 , true );
 for( int u = 1 ; u <= 1000 ; ++u )
  star.insertEdge( 0 , u );
 CHECK( width( star , 0 ) == 1000 && width( star , 7 ) == 1 );
 CHECK( width( t , 1 ) == 2 && width( t , 9 ) == 1 );
 }

// a free tree: connected, and with n - 1 edges
template< Graph G >
bool free_tree( const G & g )
{
 if( g.n() == 0 )
  return( false );
 const auto id = cc( g );
 for( int u : g.V() )
  if( id[ u ] != 1 )
   return( false );
 return( g.m() == g.n() - 1 );
 }

ADS_TEST( "ex9.6" )
{
 CHECK( free_tree( make( 5 , { { 0 , 1 } , { 1 , 2 } , { 1 , 3 } ,
                               { 3 , 4 } } , false ) ) );
 CHECK( ! free_tree( make( 4 , fig92 , false ) ) );              // a circuit
 CHECK( ! free_tree( make( 5 , { { 0 , 1 } , { 1 , 2 } , { 3 , 4 } } ,
                           false ) ) );                          // a forest
 CHECK( free_tree( ListGraph<>( 1 , false ) ) );
 // no node (not a tree), two nodes with and without their edge, a path
 // of 10^5 nodes, a tree with a deleted leaf
 CHECK( ! free_tree( ListGraph<>( 0 , false ) ) );
 CHECK( free_tree( make( 2 , { { 0 , 1 } } , false ) ) );
 CHECK( ! free_tree( ListGraph<>( 2 , false ) ) );
 CHECK( free_tree( chain( 100000 , false ) ) );
 auto t = make( 4 , { { 0 , 1 } , { 1 , 2 } , { 1 , 3 } } , false );
 t.deleteNode( 2 );
 CHECK( free_tree( t ) );
 // against the definition: exactly one simple chain between two nodes,
 // i.e., connected and without circuits (a bfs finds no non-tree edge)
 for( unsigned s = 0 ; s < 100 ; ++s ) {
  const auto g = random_graph( 9 , 0.25 , false , s );
  const auto id = cc( g );
  bool connected = std::all_of( id.begin() , id.end() ,
                                []( int x ) { return( x == 1 ); } );
  CHECK( free_tree( g ) == ( connected && g.m() == 8 ) );
  }
 }

// the scheduling: each program starts when the last of its predecessors
// ends, start[ j ] = max( start[ i ] + t[ i ] : ( i , j ) in E ), scanning
// the nodes in topological order; the solution of the book adds to
// start[ j ] its own length, which is not this; no edge, one program,
// lengths 0, a chain
template< Graph G >
Ints scheduling( const G & g , const Ints & length )
{
 Ints start( g.bound() , 0 );
 for( int i : topsort( g ) )
  g.forEachAdj( i , [ & ]( int j ) {
   start[ j ] = std::max( start[ j ] , start[ i ] + length[ i ] );
   } );
 return( start );
 }

ADS_TEST( "ex9.7" )
{
 // 0 and 1 first, 2 after both, 3 after 1, 4 after 2 and 3
 const auto g = make( 5 , { { 0 , 2 } , { 1 , 2 } , { 1 , 3 } , { 2 , 4 } ,
                            { 3 , 4 } } , true );
 const Ints t = { 3 , 5 , 2 , 9 , 1 };
 CHECK( scheduling( g , t ) == Ints( { 0 , 0 , 5 , 5 , 14 } ) );
 for( unsigned s = 0 ; s < 20 ; ++s ) {      // against n rounds of relaxing
  const auto h = random_dag( 25 , 0.15 , s );
  std::mt19937 r( s );
  Ints len( 25 );
  for( auto & l : len )
   l = 1 + r() % 10;
  Ints ref( 25 , 0 );
  for( int k = 0 ; k < 25 ; ++k )
   for( int i = 0 ; i < 25 ; ++i )
    h.forEachAdj( i , [ & ]( int j ) {
     ref[ j ] = std::max( ref[ j ] , ref[ i ] + len[ i ] );
     } );
  CHECK( scheduling( h , len ) == ref );
  }
 // no edge (all start at 0), a single program, programs of length 0, a
 // chain (the prefix sums of the lengths)
 CHECK( scheduling( ListGraph<>( 3 ) , { 4 , 5 , 6 } ) == Ints( 3 , 0 ) );
 CHECK( scheduling( ListGraph<>( 1 ) , { 7 } ) == Ints( { 0 } ) );
 CHECK( scheduling( g , Ints( 5 , 0 ) ) == Ints( 5 , 0 ) );
 const auto c = chain( 5 , true );
 CHECK( scheduling( c , { 1 , 2 , 3 , 4 , 5 } ) ==
        Ints( { 0 , 1 , 3 , 6 , 10 } ) );
 }

// the largest set of intervals that meet, by sorting the 2 n ends: + 1 at
// a left end, - 1 at a right one, the prefix sums count the intervals over
// each point; with closed intervals a left end goes before a right one
// equal to it, so that touching intervals meet; no interval, a point,
// equal, touching, disjoint ones, negative ends
std::pair< int , Ints > overlap(
                           const std::vector< std::pair< int , int > > & I )
{
 std::vector< std::pair< int , int > > S;           // ( point , -1 or +1 )
 for( auto [ a , b ] : I ) {
  S.emplace_back( a , -1 );
  S.emplace_back( b , +1 );
  }
 std::sort( S.begin() , S.end() );                  // left ends first
 int c = 0 , best = 0 , x = 0;
 for( auto [ p , e ] : S ) {
  c -= e;
  if( c > best ) {
   best = c;
   x = p;
   }
  }
 Ints out;
 for( std::size_t i = 0 ; i < I.size() ; ++i )
  if( ( I[ i ].first <= x ) && ( x <= I[ i ].second ) )
   out.push_back( int( i ) );
 return( std::pair( best , out ) );
 }

ADS_TEST( "ex9.8" )
{
 const auto [ c , s ] = overlap( { { 1 , 4 } , { 2 , 6 } , { 5 , 8 } ,
                                   { 3 , 5 } , { 7 , 9 } } );
 CHECK( c == 3 && s == Ints( { 0 , 1 , 3 } ) );     // over the point 3
 for( unsigned t = 0 ; t < 100 ; ++t ) {     // against every end as a point
  std::mt19937 r( t );
  std::vector< std::pair< int , int > > I( 1 + r() % 15 );
  for( auto & [ a , b ] : I ) {
   a = r() % 30;
   b = a + r() % 10;
   }
  int best = 0;
  for( auto [ a , b ] : I )
   for( int x : { a , b } ) {
    int k = 0;
    for( auto [ p , q ] : I )
     k += ( p <= x ) && ( x <= q );
    best = std::max( best , k );
    }
  const auto [ cc , ss ] = overlap( I );
  CHECK( cc == best && int( ss.size() ) == best );
  }
 // no interval, one, one of a single point, n equal ones, two touching
 // (closed, so they meet), two disjoint, negative ends
 CHECK( overlap( {} ) == std::pair( 0 , Ints() ) );
 CHECK( overlap( { { 2 , 5 } } ) == std::pair( 1 , Ints( { 0 } ) ) );
 CHECK( overlap( { { 4 , 4 } } ) == std::pair( 1 , Ints( { 0 } ) ) );
 CHECK( overlap( { { 1 , 3 } , { 1 , 3 } , { 1 , 3 } } ) ==
        std::pair( 3 , Ints( { 0 , 1 , 2 } ) ) );
 CHECK( overlap( { { 1 , 2 } , { 2 , 3 } } ).first == 2 );
 CHECK( overlap( { { 1 , 2 } , { 3 , 4 } } ).first == 1 );
 CHECK( overlap( { { -5 , -1 } , { -3 , 0 } } ) ==
        std::pair( 2 , Ints( { 0 , 1 } ) ) );
 }

// the number of nodes at the largest distance from r; in the solution of
// the book S.enqueue( v ) must be inside the if, or the visit never ends;
// a single node, a chain from either end, a star
template< Graph G >
int farthest( const G & g , int r )
{
 const auto d = distance( g , r ).first;
 int max = 0 , count = 0;
 for( int u : g.V() )
  if( d[ u ] == INF )
   continue;
  else if( d[ u ] > max ) {
   max = d[ u ];
   count = 1;
   }
  else if( d[ u ] == max )
   ++count;
 return( max == 0 ? 0 : count );
 }

ADS_TEST( "ex9.9" )
{
 CHECK( farthest( make( 9 , fig99 , false ) , 0 ) == 3 );     // 4, 6, 8
 CHECK( farthest( make( 4 , fig91 , true ) , 0 ) == 1 );      // 3
 CHECK( farthest( make( 4 , fig91 , true ) , 3 ) == 2 );      // 1 and 2
 CHECK( farthest( ListGraph<>( 3 ) , 0 ) == 0 );
 // a single node, the end of a chain (nothing reached), its start (one
 // node, at distance n - 1, also for 10^5 nodes), a star
 CHECK( farthest( ListGraph<>( 1 ) , 0 ) == 0 );
 const auto c = chain( 100000 , true );
 CHECK( farthest( c , 99999 ) == 0 && farthest( c , 0 ) == 1 );
 ListGraph<> star( 6 , false );
 for( int u = 1 ; u < 6 ; ++u )
  star.insertEdge( 0 , u );
 CHECK( farthest( star , 0 ) == 5 && farthest( star , 3 ) == 4 );
 }

// the square of a graph on its adjacency matrix: ( u , w ) when there is a
// node v with ( u , v ) and ( v , w ), Theta( n^3 ) time; a pair ( u , u ),
// a loop, is left out (the text writes the condition with u in place of
// v); no node, no edge, one edge, a circuit of 2, a path of 3 nodes
MatrixGraph<> square( const MatrixGraph<> & g )
{
 MatrixGraph<> s( g.bound() , true );
 for( int u = 0 ; u < g.bound() ; ++u )
  for( int w = 0 ; w < g.bound() ; ++w )
   for( int v = 0 ; ( v < g.bound() ) && ( u != w ) ; ++v )
    if( g.hasEdge( u , v ) && g.hasEdge( v , w ) ) {
     s.insertEdge( u , w );
     break;
     }
 return( s );
 }

ADS_TEST( "ex9.10" )
{
 const auto g = make< MatrixGraph<> >( 4 , fig91 , true );
 const auto s = square( g );
 // 0 -> 1 -> 3, 0 -> 2 -> 1, 1 -> 3 -> 1 (a loop, left out), 1 -> 3 -> 2,
 // 2 -> 1 -> 3, 3 -> 1 -> 3 (loop), 3 -> 2 -> 1
 Edges e;
 for( int u = 0 ; u < 4 ; ++u )
  for( int w : s.adj( u ) )
   e.emplace_back( u , w );
 CHECK( e == Edges( { { 0 , 1 } , { 0 , 3 } , { 1 , 2 } , { 2 , 3 } ,
                      { 3 , 1 } } ) );
 for( unsigned t = 0 ; t < 10 ; ++t ) {             // against the lists
  const auto h = random_graph( 15 , 0.15 , true , t );
  MatrixGraph<> m( 15 , true );
  for( int u = 0 ; u < 15 ; ++u )
   h.forEachAdj( u , [ & ]( int v ) { m.insertEdge( u , v ); } );
  const auto q = square( m );
  for( int u = 0 ; u < 15 ; ++u ) {
   std::set< int > two;
   h.forEachAdj( u , [ & ]( int v ) {
    h.forEachAdj( v , [ & ]( int w ) { if( w != u ) two.insert( w ); } );
    } );
   CHECK( q.adj( u ) == Ints( two.begin() , two.end() ) );
   }
  }
 // no node, no edge, one edge (no path of 2), a circuit of 2 (only loops,
 // left out), a path of 3 nodes
 CHECK( square( MatrixGraph<>( 0 , true ) ).n() == 0 );
 CHECK( square( MatrixGraph<>( 3 , true ) ).m() == 0 );
 CHECK( square( make< MatrixGraph<> >( 2 , { { 0 , 1 } } , true ) ).m() == 0 );
 CHECK( square( make< MatrixGraph<> >( 2 , { { 0 , 1 } , { 1 , 0 } } ,
                                       true ) ).m() == 0 );
 const auto p = square( make< MatrixGraph<> >( 3 , { { 0 , 1 } , { 1 , 2 } } ,
                                               true ) );
 CHECK( p.m() == 1 && p.hasEdge( 0 , 2 ) );
 }

// the diameter by a bfs from each node, O( n ( n + m ) ) time; nothing if
// some node does not reach another; no node, one, one edge, two isolated
// nodes, a path
template< Graph G >
std::optional< int > diameter( const G & g )
{
 int D = 0;
 for( int u : g.V() ) {
  const auto d = distance( g , u ).first;
  for( int v : g.V() )
   if( d[ v ] == INF )
    return( std::nullopt );
   else
    D = std::max( D , d[ v ] );
  }
 return( D );
 }

ADS_TEST( "ex9.11" )
{
 CHECK( diameter( make( 9 , fig99 , false ) ) == 3 );   // 4, 8 via 1, ...
 CHECK( ! diameter( make( 4 , fig91 , true ) ) );       // 0 not reached
 CHECK( diameter( make( 4 , fig92 , false ) ) == 2 );
 for( unsigned s = 0 ; s < 20 ; ++s ) {
  const auto h = random_graph( 20 , 0.2 , s % 2 , s );
  const auto D = all_distances( h );
  int ref = 0;
  bool inf = false;
  for( const auto & r : D )
   for( int x : r )
    if( x == INF ) inf = true; else ref = std::max( ref , x );
  CHECK( diameter( h ) == ( inf ? std::nullopt : std::optional( ref ) ) );
  }
 // no node and one node (0), one edge (1 unoriented, nothing oriented),
 // two isolated nodes, a path of 50 nodes
 CHECK( diameter( ListGraph<>( 0 ) ) == 0 );
 CHECK( diameter( ListGraph<>( 1 ) ) == 0 );
 CHECK( diameter( make( 2 , { { 0 , 1 } } , false ) ) == 1 );
 CHECK( ! diameter( make( 2 , { { 0 , 1 } } , true ) ) );
 CHECK( ! diameter( ListGraph<>( 2 , false ) ) );
 CHECK( diameter( chain( 50 , false ) ) == 49 );
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
/*---------------------------- End File ch09.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
