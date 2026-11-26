/*--------------------------------------------------------------------------*/
/*-------------------- File engineered_biconnectivity.cpp ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the blocks, the cut nodes, the bridges, the 2-edge-connected
 * components and the block-cut tree, beyond the book: the visit in depth
 * against the definitions by brute force on all the small graphs of a
 * random family (parallel edges, loops, isolated nodes and names that are
 * not nodes included), FAST-BCC against the visit on each executor, on
 * random graphs, grids, grids with holes and dangling paths (a proxy of a
 * road network), graphs of a power law, forests and graphs of many small
 * components, at sizes around the powers of 2, and both against
 * Boost.Graph where it is found; and the edge cases (no node, one node,
 * one edge, a path of 10^6 nodes, a star, a cycle, complete graphs), at
 * the level 3 of the contracts.
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
#include "ADS/graph/Biconnectivity.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelBiconnectivity.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <tuple>
#include <random>
#include <utility>
#include <vector>

#if __has_include( <boost/graph/biconnected_components.hpp> )
 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/biconnected_components.hpp>
 #define ADS_TEST_BOOST_GRAPH 1
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

using Edges = std::vector< std::pair< int , int > >;

/// an unoriented graph of rows of adjacency that may have loops, parallel
/// edges and names that are not nodes, a model of `Graph` that the
/// compressed rows copy
struct LoopGraph
{
 int nn = 0;
 std::vector< std::vector< int > > a;
 std::vector< bool > alive;

 LoopGraph( int n , const Edges & E , const std::vector< int > & dead = {} )
     : nn( n ) , a( std::size_t( n ) ) , alive( std::size_t( n ) , true ) {
  for( int d : dead )
   alive[ d ] = false;
  for( const auto & [ u , v ] : E ) {
   a[ u ].push_back( v );
   if( u != v )
    a[ v ].push_back( u );
   }
  }
 [[nodiscard]] int n( void ) const {
  return( int( std::count( alive.begin() , alive.end() , true ) ) );
  }
 [[nodiscard]] int bound( void ) const { return( nn ); }
 [[nodiscard]] bool directed( void ) const { return( false ); }
 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < nn ) && alive[ u ] );
  }
 [[nodiscard]] std::vector< int > V( void ) const {
  std::vector< int > v;
  for( int u = 0 ; u < nn ; ++u )
   if( alive[ u ] )
    v.push_back( u );
  return( v );
  }
 [[nodiscard]] std::vector< int > adj( int u ) const { return( a[ u ] ); }
 template< class F >
 void forEachAdj( int u , F && f ) const {
  for( int v : a[ u ] )
   f( v );
  }
 };

static_assert( Graph< LoopGraph > );

/// the executors of the tests: one thread, and pools of 1, 2, 3 and 8
template< class F >
void onEach( F && f )
{
 Sequential s;
 f( s );
 for( std::size_t p : { 1 , 2 , 3 , 8 } ) {
  ThreadPool pool( p );
  f( pool );
  }
 }

/// the number of connected components of the nodes of g not in X, the
/// edges with an end in X or in the set D of the places of E left out
int components( int n , const Edges & E , const std::vector< bool > & dead ,
		const std::vector< bool > & X , std::size_t skip = SIZE_MAX )
{
 DisjointSets<> D( n );
 int c = 0;
 for( int u = 0 ; u < n ; ++u )
  c += ! dead[ u ] && ! X[ u ];
 for( std::size_t i = 0 ; i < E.size() ; ++i ) {
  const auto [ u , v ] = E[ i ];
  if( ( i != skip ) && ! X[ u ] && ! X[ v ] && D.merge( u , v ) )
   --c;
  }
 return( c );
 }

/// whether u and v are joined in g less the node x (-1 for none)
bool joined( int n , const Edges & E , int x , int u , int v )
{
 DisjointSets<> D( n );
 for( const auto & [ a , b ] : E )
  if( ( a != x ) && ( b != x ) )
   D.merge( a , b );
 return( D.same( u , v ) );
 }

/// the blocks of the edges E (loops aside) by the definition: two edges
/// are in one block if and only if no node, nor the removal of nothing,
/// separates their ends left
std::vector< int > blocksByDefinition( int n , const Edges & E )
{
 const std::size_t m = E.size();
 std::vector< int > b( m , -1 );
 int k = 0;
 for( std::size_t i = 0 ; i < m ; ++i ) {
  if( ( E[ i ].first == E[ i ].second ) || ( b[ i ] >= 0 ) )
   continue;
  b[ i ] = k;
  for( std::size_t j = i + 1 ; j < m ; ++j ) {
   if( E[ j ].first == E[ j ].second )
    continue;
   bool same = true;
   for( int x = -1 ; same && ( x < n ) ; ++x ) {
    const auto [ a , c ] = E[ i ];
    const auto [ d , e ] = E[ j ];
    const int p = a != x ? a : c , q = d != x ? d : e;
    same = joined( n , E , x , p , q );
    }
   if( same )
    b[ j ] = k;
   }
  ++k;
  }
 return( b );
 }

/// the partition of the places by their labels, each label replaced by the
/// order of its first place
std::vector< int > canonical( const std::vector< int > & L )
{
 std::map< int , int > first;
 std::vector< int > c( L.size() );
 for( std::size_t i = 0 ; i < L.size() ; ++i )
  c[ i ] = L[ i ] < 0
	     ? -1
	     : first.emplace( L[ i ] , int( first.size() ) ).first->second;
 return( c );
 }

/// the blocks of the edges E by a decomposition
std::vector< int > blocksOf( const BiconnectedComponents & B ,
			     const Edges & E )
{
 std::vector< int > b;
 for( const auto & [ u , v ] : E )
  b.push_back( B.block( u , v ) );
 return( b );
 }

/// a decomposition checked against the definitions on the graph of n names
/// and edges E: the blocks, the cut nodes, the bridges, the
/// 2-edge-connected components and the block-cut tree
void checkDefinitions( int n , const Edges & E ,
		       const std::vector< int > & deadList ,
		       const BiconnectedComponents & B ,
		       const std::vector< int > & twoEdge )
{
 std::vector< bool > dead( std::size_t( n ) , false ) ,
   none( std::size_t( n ) , false );
 for( int d : deadList )
  dead[ d ] = true;
 CHECK( canonical( blocksOf( B , E ) ) ==
	canonical( blocksByDefinition( n , E ) ) );
 const int c0 = components( n , E , dead , none );
 for( int x = 0 ; x < n ; ++x ) {
  if( dead[ x ] ) {
   CHECK( ! B.isCut( x ) && ( B.blockOf( x ) == -1 ) );
   continue;
   }
  std::vector< bool > X( std::size_t( n ) , false );
  X[ x ] = true;
  bool isolated = true;
  for( const auto & [ u , v ] : E )
   isolated = isolated && ( ( u != x || v == x ) && ( v != x || u == x ) );
  CHECK( B.isCut( x ) ==
	 ( components( n , E , dead , X ) > c0 - ( isolated ? 1 : 0 ) ) );
  }
 // a block is a bridge if and only if its edge alone splits a component
 std::vector< int > edges( std::size_t( B.blocks() ) , 0 );
 Edges bridges;
 for( std::size_t i = 0 ; i < E.size() ; ++i ) {
  const int k = B.block( E[ i ].first , E[ i ].second );
  if( k < 0 )
   continue;
  ++edges[ k ];
  if( components( n , E , dead , none , i ) > c0 )
   bridges.emplace_back( std::min( E[ i ].first , E[ i ].second ) ,
			 std::max( E[ i ].first , E[ i ].second ) );
  }
 for( int k = 0 ; k < B.blocks() ; ++k )
  CHECK( B.isBridge( k ) == ( edges[ k ] == 1 ) );
 std::sort( bridges.begin() , bridges.end() );
 Edges got = B.bridges();
 std::sort( got.begin() , got.end() );
 CHECK( got == bridges );
 // the 2-edge-connected components: the components less the bridges
 DisjointSets<> D( n );
 for( std::size_t i = 0 ; i < E.size() ; ++i ) {
  const auto [ u , v ] = E[ i ];
  if( ( u != v ) && ! B.isBridge( B.block( u , v ) ) )
   D.merge( u , v );
  }
 for( int u = 0 ; u < n ; ++u ) {
  int least = -1;
  for( int v = 0 ; ( v < n ) && ( least < 0 ) ; ++v )
   if( ! dead[ v ] && D.same( u , v ) )
    least = v;
  CHECK( twoEdge[ u ] == ( dead[ u ] ? -1 : least ) );
  }
 // the block-cut tree: a forest with an edge for each cut node in a block
 const BlockCutTree T = blockCutTree( B );
 CHECK( T.cuts == B.cutNodes() );
 int incidences = 0;
 for( int x : T.cuts ) {
  std::vector< int > in;
  for( const auto & [ u , v ] : E )
   if( ( u != v ) && ( ( u == x ) || ( v == x ) ) )
    in.push_back( B.block( u , v ) );
  std::sort( in.begin() , in.end() );
  in.erase( std::unique( in.begin() , in.end() ) , in.end() );
  CHECK( in.size() >= 2 );
  incidences += int( in.size() );
  }
 CHECK( T.tree.m() == incidences );
 CHECK( T.tree.n() == B.blocks() + int( T.cuts.size() ) );
 }

/// the edges of a random graph of n nodes and m edges, with parallel
/// edges if multi and loops if loops
Edges randomEdges( std::mt19937_64 & g , int n , int m , bool multi ,
		   bool loops )
{
 Edges E;
 std::map< std::pair< int , int > , int > seen;
 for( int i = 0 ; ( i < m ) && ( n > 0 ) ; ++i ) {
  const int u = int( g() % std::uint64_t( n ) ) ,
	    v = int( g() % std::uint64_t( n ) );
  if( ( u == v ) && ! loops )
   continue;
  if( ! multi && seen[ { std::min( u , v ) , std::max( u , v ) } ]++ )
   continue;
  E.emplace_back( u , v );
  }
 return( E );
 }

/// the edges of a grid of r rows and c columns, each kept with probability
/// keep, with a dangling path of length tail from each corner
Edges gridEdges( std::mt19937_64 & g , int r , int c , double keep , int tail ,
		 int & n )
{
 Edges E;
 std::bernoulli_distribution k( keep );
 auto id = [ & ]( int i , int j ) { return( i * c + j ); };
 for( int i = 0 ; i < r ; ++i )
  for( int j = 0 ; j < c ; ++j ) {
   if( ( j + 1 < c ) && k( g ) )
    E.emplace_back( id( i , j ) , id( i , j + 1 ) );
   if( ( i + 1 < r ) && k( g ) )
    E.emplace_back( id( i , j ) , id( i + 1 , j ) );
   }
 n = r * c;
 for( int corner : { 0 , c - 1 , ( r - 1 ) * c , r * c - 1 } ) {
  int at = corner;
  for( int t = 0 ; t < tail ; ++t ) {
   E.emplace_back( at , n );
   at = n++;
   }
  }
 return( E );
 }

/// the edges of a graph of n nodes and about m edges whose degrees follow
/// a power law of exponent 2.5 (the model of Chung and Lu)
Edges powerLawEdges( std::mt19937_64 & g , int n , int m )
{
 std::vector< double > w( std::size_t( n ) , 0.0 );
 for( int i = 0 ; i < n ; ++i )
  w[ i ] = std::pow( double( i + 1 ) , -1.0 / 1.5 );
 std::discrete_distribution< int > pick( w.begin() , w.end() );
 Edges E;
 for( int i = 0 ; i < m ; ++i ) {
  const int u = pick( g ) , v = pick( g );
  if( u != v )
   E.emplace_back( u , v );
  }
 return( E );
 }

/// the edges of a random forest of n nodes in t trees
Edges forestEdges( std::mt19937_64 & g , int n , int t )
{
 Edges E;
 for( int v = t ; v < n ; ++v )
  E.emplace_back( int( g() % std::uint64_t( v ) ) , v );
 return( E );
 }

/// the edges without the loops
Edges noLoops( const Edges & E )
{
 Edges F;
 for( const auto & e : E )
  if( e.first != e.second )
   F.push_back( e );
 return( F );
 }

/// FAST-BCC and the sequential visit on a graph, on each executor, the
/// first on the graph G and the second also on its StaticGraph if it has
/// no loop
template< class G >
void checkParallel( const G & g , const Edges & E )
{
 const BiconnectedComponents ref = biconnectedComponents( g );
 const std::vector< int > two = twoEdgeConnectedComponents( g );
 CHECK( ref.bound() == g.bound() );
 for( const auto & [ u , v ] : E )
  CHECK( ( u == v ) == ( ref.block( u , v ) < 0 ) );
 onEach( [ & ]( auto & ex ) {
  const BiconnectedComponents B = biconnectedComponents( ex , g );
  CHECK( B.sameBlocks( ref ) );
  CHECK( blocksOf( B , E ) == blocksOf( ref , E ) );
  CHECK( twoEdgeConnectedComponents( ex , g ) == two );
  } );
 }

// the visit in depth against the definitions on 3000 random graphs of at
// most 9 names, with parallel edges, loops, isolated nodes and names that
// are not nodes, and FAST-BCC against the visit on each of them
ADS_TEST( "biconnected-definitions" )
{
 std::mt19937_64 g( 41 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const int n = int( g() % 10 );
  const int m = n == 0 ? 0 : int( g() % std::uint64_t( 2 * n + 2 ) );
  Edges E = randomEdges( g , n , m , t % 3 != 0 , t % 4 == 0 );
  std::vector< int > dead;
  if( ( t % 5 == 0 ) && ( n > 0 ) ) {  // a name that is not a node
   const int d = int( g() % std::uint64_t( n ) );
   dead.push_back( d );
   Edges F;
   for( const auto & e : E )
    if( ( e.first != d ) && ( e.second != d ) )
     F.push_back( e );
   E = F;
   }
  const LoopGraph L( n , E , dead );
  const BiconnectedComponents B = biconnectedComponents( L );
  checkDefinitions( n , E , dead , B , twoEdgeConnectedComponents( L ) );
  Sequential seq;
  const BiconnectedComponents P = biconnectedComponents( seq , L );
  CHECK( P.sameBlocks( B ) );
  if( dead.empty() ) {
   const Edges F = noLoops( E );
   const StaticGraph<> S( n , F , false );
   CHECK( biconnectedComponents( S ).sameBlocks( B ) );
   ThreadPool pool( 3 );
   CHECK( biconnectedComponents( pool , S ).sameBlocks( B ) );
   CHECK( twoEdgeConnectedComponents( pool , S ) ==
	  twoEdgeConnectedComponents( L ) );
   std::vector< std::tuple< int , int , int > > W;
   for( const auto & [ u , v ] : F )
    W.emplace_back( u , v , 0 );
   const ArrayGraph< int > AG( n , W , false );
   CHECK( biconnectedComponents( AG ).sameBlocks( B ) );
   }
  }
 }

// the edge cases: no node, one node, one edge, two parallel edges, a
// triangle, a bowtie, a star, a cycle, a complete graph, and a path of 10^6
// nodes, whose visit would overflow a recursion
ADS_TEST( "biconnected-shapes" )
{
 const StaticGraph<> empty( 0 , Edges{} , false );
 const BiconnectedComponents B0 = biconnectedComponents( empty );
 CHECK( ( B0.blocks() == 0 ) && ( B0.bound() == 0 ) );
 CHECK( blockCutTree( B0 ).tree.n() == 0 );
 ThreadPool pool( 4 );
 CHECK( biconnectedComponents( pool , empty ).blocks() == 0 );
 const StaticGraph<> one( 1 , Edges{} , false );
 CHECK( biconnectedComponents( one ).blocks() == 0 );
 CHECK( biconnectedComponents( pool , one ).blocks() == 0 );
 CHECK( twoEdgeConnectedComponents( pool , one ) ==
	std::vector< int >( { 0 } ) );
 {
  const StaticGraph<> e( 2 , Edges{ { 1 , 0 } } , false );
  const BiconnectedComponents B = biconnectedComponents( e );
  CHECK( ( B.blocks() == 1 ) && B.isBridge( 0 ) && ! B.isCut( 0 ) );
  CHECK( ( B.head( 0 ) == 0 ) && ( B.least( 0 ) == 1 ) );
  CHECK( B.bridges() == Edges( { { 0 , 1 } } ) );
  CHECK( twoEdgeConnectedComponents( e ) == std::vector< int >( { 0 , 1 } ) );
  CHECK( B.arcBlocks( e ) == std::vector< int >( { 0 , 0 } ) );
  }
 {
  const StaticGraph<> e( 2 , Edges{ { 0 , 1 } , { 1 , 0 } } , false );
  const BiconnectedComponents B = biconnectedComponents( e );
  CHECK( ( B.blocks() == 1 ) && ! B.isBridge( 0 ) );
  CHECK( twoEdgeConnectedComponents( pool , e ) ==
	 std::vector< int >( { 0 , 0 } ) );
  }
 {
  // two triangles that share the node 2, and a pendant edge at 4
  const Edges E{ { 0 , 1 } , { 1 , 2 } , { 2 , 0 } , { 2 , 3 } ,
		 { 3 , 4 } , { 4 , 2 } , { 4 , 5 } };
  const StaticGraph<> s( 6 , E , false );
  const BiconnectedComponents B = biconnectedComponents( s );
  CHECK( B.blocks() == 3 );
  CHECK( B.cutNodes() == std::vector< int >( { 2 , 4 } ) );
  CHECK( B.bridges() == Edges( { { 4 , 5 } } ) );
  CHECK( B.block( 0 , 1 ) == B.block( 2 , 0 ) );
  CHECK( B.block( 3 , 4 ) == B.block( 2 , 4 ) );
  CHECK( B.block( 0 , 1 ) != B.block( 3 , 4 ) );
  const BlockCutTree T = blockCutTree( B );
  CHECK( ( T.tree.n() == 5 ) && ( T.tree.m() == 4 ) );
  CHECK( biconnectedComponents( pool , s ).sameBlocks( B ) );
  CHECK( twoEdgeConnectedComponents( s ) ==
	 std::vector< int >( { 0 , 0 , 0 , 0 , 0 , 5 } ) );
  }
 for( int n : { 2 , 3 , 63 , 64 , 65 , 1000 } ) {
  Edges star , cycle , complete;
  for( int v = 1 ; v < n ; ++v )
   star.emplace_back( 0 , v );
  for( int v = 0 ; v < n ; ++v )
   cycle.emplace_back( v , ( v + 1 ) % n );
  if( n <= 65 )
   for( int u = 0 ; u < n ; ++u )
    for( int v = u + 1 ; v < n ; ++v )
     complete.emplace_back( u , v );
  const StaticGraph<> S( n , star , false );
  const BiconnectedComponents B = biconnectedComponents( S );
  CHECK( B.blocks() == n - 1 );
  CHECK( B.isCut( 0 ) == ( n > 2 ) );
  CHECK( int( B.bridges().size() ) == n - 1 );
  CHECK( biconnectedComponents( pool , S ).sameBlocks( B ) );
  if( n > 2 ) {
   const StaticGraph<> C( n , cycle , false );
   const BiconnectedComponents BC = biconnectedComponents( C );
   CHECK( ( BC.blocks() == 1 ) && BC.cutNodes().empty() &&
	  BC.bridges().empty() );
   CHECK( biconnectedComponents( pool , C ).sameBlocks( BC ) );
   }
  if( ! complete.empty() && ( n > 2 ) ) {
   const StaticGraph<> K( n , complete , false );
   CHECK( biconnectedComponents( K ).blocks() == 1 );
   CHECK( biconnectedComponents( pool , K ).blocks() == 1 );
   }
  }
 {
  const int n = test::scaled( 1000000 , 10000 );
  Edges path;
  for( int v = 0 ; v + 1 < n ; ++v )
   path.emplace_back( v , v + 1 );
  const StaticGraph<> P( n , path , false );
  const BiconnectedComponents B = biconnectedComponents( P );
  CHECK( ( B.blocks() == n - 1 ) && ( int( B.bridges().size() ) == n - 1 ) );
  CHECK( int( B.cutNodes().size() ) == n - 2 );
  const std::vector< int > two = twoEdgeConnectedComponents( P );
  for( int v = 0 ; v < n ; ++v )
   CHECK( two[ v ] == v );
  // the parallel algorithm on a shorter path, its audits quadratic in
  // the depth of the forest
  const int h = test::scaled( 100000 , 5000 );
  const StaticGraph<> Q( h , Edges( path.begin() , path.begin() + h - 1 ) ,
			 false );
  CHECK( biconnectedComponents( pool , Q ).sameBlocks(
    biconnectedComponents( Q ) ) );
  }
 CHECK_THROWS(
   biconnectedComponents( StaticGraph<>( 2 , Edges{ { 0 , 1 } } , true ) ) ,
   contract_violation );
 }

// FAST-BCC against the visit on each executor, on random graphs, grids,
// grids with holes and dangling paths, graphs of a power law, forests and
// graphs of many small components with isolated nodes, at sizes around
// the powers of 2
ADS_TEST( "biconnected-parallel" )
{
 std::mt19937_64 g( 43 );
 for( int n : { 63 , 64 , 65 , 4095 , 4096 , 4097 } ) {
  for( int d : { 1 , 2 , 4 } ) {
   const Edges E = randomEdges( g , n , d * n , true , false );
   checkParallel( StaticGraph<>( n , E , false ) , E );
   }
  const Edges P = powerLawEdges( g , n , 3 * n );
  checkParallel( StaticGraph<>( n , P , false ) , P );
  const Edges T = forestEdges( g , n , 5 );
  checkParallel( StaticGraph<>( n , T , false ) , T );
  }
 for( double keep : { 1.0 , 0.75 , 0.55 } ) {
  int n = 0;
  const Edges E = gridEdges( g , 64 , test::scaled( 300 , 40 ) , keep , 20 ,
			     n );
  checkParallel( StaticGraph<>( n , E , false ) , E );
  }
 {
  // many small components, isolated nodes among them, loops and parallel
  // edges, on a graph that the compressed rows copy
  const int n = 20000;
  Edges E;
  for( int c = 0 ; c < n / 10 - 200 ; ++c )
   for( const auto & [ u , v ] : randomEdges( g , 10 , 14 , true , true ) )
    E.emplace_back( 10 * c + u , 10 * c + v );
  checkParallel( LoopGraph( n , E ) , E );
  }
 {
  const int n = test::scaled( 1 << 17 , 1 << 13 );
  const Edges E = powerLawEdges( g , n , 8 * n );
  checkParallel( StaticGraph<>( n , E , false ) , E );
  }
 }

// the visit and FAST-BCC against biconnected_components() and
// articulation_points() of Boost.Graph, on the same families, loops and
// parallel edges included; nothing to compare without Boost
ADS_TEST( "biconnected-boost" )
{
#if defined( ADS_TEST_BOOST_GRAPH )
 using BG = boost::adjacency_list<
   boost::vecS , boost::vecS , boost::undirectedS , boost::no_property ,
   boost::property< boost::edge_index_t , std::size_t > >;
 std::mt19937_64 g( 47 );
 ThreadPool pool( 4 );
 auto compare = [ & ]( int n , const Edges & E ) {
  BG bg{ std::size_t( n ) };
  for( std::size_t i = 0 ; i < E.size() ; ++i )
   boost::add_edge( std::size_t( E[ i ].first ) ,
		    std::size_t( E[ i ].second ) ,
		    i , bg );
  std::vector< std::size_t > comp( E.size() , SIZE_MAX );
  boost::biconnected_components(
    bg , boost::make_iterator_property_map(
	  comp.begin() , boost::get( boost::edge_index , bg ) ) );
  std::vector< std::size_t > art;
  boost::articulation_points( bg , std::back_inserter( art ) );
  std::sort( art.begin() , art.end() );
  // Boost skips each edge to the parent of a node, and so gives no
  // component to the parallel copies of the edges of its tree, which the
  // comparison leaves out, with the loops
  const LoopGraph L( n , E );
  const BiconnectedComponents B = biconnectedComponents( L );
  std::vector< int > theirs , ours = blocksOf( B , E );
  for( std::size_t i = 0 ; i < E.size() ; ++i ) {
   const bool out =
     ( E[ i ].first == E[ i ].second ) || ( comp[ i ] == SIZE_MAX );
   theirs.push_back( out ? -1 : int( comp[ i ] ) );
   if( out )
    ours[ i ] = -1;
   }
  CHECK( canonical( ours ) == canonical( theirs ) );
  const std::vector< int > cuts = B.cutNodes();
  CHECK( std::equal( cuts.begin() , cuts.end() , art.begin() , art.end() ) );
  CHECK( biconnectedComponents( pool , L ).sameBlocks( B ) );
  };
 for( int n : { 1 , 2 , 100 , 1023 , 1024 , 1025 } )
  for( int d : { 1 , 3 } )
   compare( n , randomEdges( g , n , d * n , true , true ) );
 int n = 0;
 const Edges G = gridEdges( g , 50 , 50 , 0.6 , 10 , n );
 compare( n , G );
 compare( 5000 , powerLawEdges( g , 5000 , 15000 ) );
 compare( 5000 , forestEdges( g , 5000 , 3 ) );
#endif
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------ End File engineered_biconnectivity.cpp ----------------*/
/*--------------------------------------------------------------------------*/
