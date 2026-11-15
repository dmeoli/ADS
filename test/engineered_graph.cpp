/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_graph.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered structures of the graphs, beyond the book, and
 * of the algorithms of the book on them: the graph in compressed rows, the
 * indexed d-ary heap and the union-find by size or rank with the paths
 * halved or compressed, each against the structure of the book or a set of
 * the standard library; the shortest paths, the spanning trees and the
 * visit in breadth of the book on every priority queue, queue, stack,
 * deque and union-find, which must give the same results and the same
 * counts; the implicit graph of a grid map against the explicit graphs of
 * the same grid, and the builder of the graphs in compressed rows against
 * their constructors. On random graphs with fixed seeds and on the edge
 * cases (no node, one node, no edge, parallel edges, unreachable nodes,
 * ties, a negative cycle, grids of one row, of no free cell and of
 * 2^31 - 1 cells), at the level 3 of the contracts.
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
#include "ADS/graph/Assignment.h"
#include "ADS/graph/EulerianCycle.h"
#include "ADS/graph/GraphBuilder.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/GridGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/complexity/EasyCases.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- INPUTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Edges = std::vector< std::tuple< int , int , int > >;

/// m random edges on n >= 2 nodes, no loop, weights in [ lo , hi ]
Edges randomEdges( int n , int m , int lo , int hi , std::mt19937 & g )
{
 Edges E;
 while( int( E.size() ) < m ) {
  const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( u != v )
   E.emplace_back( u , v , lo + int( g() % unsigned( hi - lo + 1 ) ) );
  }
 return( E );
 }


/// a priority queue of the nodes on a std::set of their priorities, the
/// decrease an erase and an insert, the node its own handle
struct SetQueue
{
 using Item = int;
 using Priority = std::pair< int , int >;
 using Handle = int;
 std::set< Priority > s;
 std::vector< Priority > key;
 explicit SetQueue( int n ) : key( std::size_t( n ) ) { }
 bool empty( void ) const { return( s.empty() ); }
 int min( void ) const { return( s.begin()->second ); }
 int deleteMin( void ) {
  const int v = s.begin()->second;
  s.erase( s.begin() );
  return( v );
  }
 Handle insert( int v , const Priority & p ) {
  key[ v ] = p;
  s.insert( p );
  return( v );
  }
 void decrease( Handle v , const Priority & p ) {
  s.erase( key[ v ] );
  key[ v ] = p;
  s.insert( p );
  }
 };

static_assert( PriorityQueue< SetQueue > );
static_assert( PriorityQueue< IndexedDaryHeap< std::pair< int , int > > > );
static_assert( AddressablePriorityQueue< SetQueue > );
static_assert(
  AddressablePriorityQueue< IndexedDaryHeap< std::pair< int , int > > > );

/// std::queue with the names of the book
struct StdQueue
{
 std::queue< int > q;
 bool empty( void ) const { return( q.empty() ); }
 void enqueue( int v ) { q.push( v ); }
 int dequeue( void ) {
  const int v = q.front();
  q.pop();
  return( v );
  }
 };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// StaticGraph.h defines no macro of its own: the hint of prefetch that it
// defined, used nowhere and not guarded, is gone
ADS_TEST( "static-graph-macros" )
{
#if defined( ADS_GRAPH_PREFETCH )
 CHECK( false );
#else
 CHECK( true );
#endif
 }

// the graph in compressed rows against ArrayGraph: the rows sorted by
// head, parallel edges kept, an unoriented edge both ways, the transpose,
// a relabelling by bfsOrder() that keeps the degrees, the empty graph, an
// isolated node, loops and nodes out of range violating the contracts
ADS_TEST( "static-graph" )
{
 std::mt19937 g( 1 );
 for( bool directed : { true , false } ) {
  Edges E = randomEdges( 30 , 120 , 1 , 9 , g );
  E.push_back( E.front() );                       // a parallel edge
  const StaticGraph< int > S( 31 , E , directed );  // 30 isolated
  const ArrayGraph< int > A( 31 , E , directed );
  CHECK( S.n() == 31 && S.m() == int( E.size() ) && S.degree( 30 ) == 0 );
  CHECK( S.arcs() == ( directed ? 1 : 2 ) * int( E.size() ) );
  for( int u = 0 ; u < S.n() ; ++u ) {
   auto a = A.adj( u );
   std::sort( a.begin() , a.end() );
   CHECK( S.adj( u ) == a );
   std::multiset< std::pair< int , int > > x , y;
   S.forEachEdge( u , [ & ]( int v , int w ) { x.emplace( v , w ); } );
   A.forEachEdge( u , [ & ]( int v , int w ) { y.emplace( v , w ); } );
   CHECK( x == y );
   }
  const auto T = S.transpose();
  std::multiset< std::pair< int , int > > fw , bw;
  for( int u = 0 ; u < S.n() ; ++u ) {
   for( int v : S.adj( u ) )
    fw.emplace( u , v );
   for( int v : T.adj( u ) )
    bw.emplace( v , u );
   }
  CHECK( fw == bw );
  const auto p = bfsOrder( S , 3 );
  CHECK( p[ 3 ] == 0 );
  const auto R = S.relabel( p );
  for( int u = 0 ; u < S.n() ; ++u )
   CHECK( R.degree( p[ u ] ) == S.degree( u ) );
  const StaticGraph< int > C( A );  // from a graph
  for( int u = 0 ; u < S.n() ; ++u )
   CHECK( C.adj( u ) == S.adj( u ) );
  }
 const StaticGraph<> Z;
 CHECK( Z.n() == 0 && Z.m() == 0 && Z.V().empty() );
 CHECK( bfsOrder( Z ).empty() );
 const StaticGraph<> One( 1 , std::vector< std::pair< int , int > >{} );
 CHECK( One.n() == 1 && One.adj( 0 ).empty() );
 CHECK_THROWS(
   StaticGraph<>( 2 , std::vector< std::pair< int , int > >{ { 1 , 1 } } ) ,
   contract_violation );
 CHECK_THROWS(
   StaticGraph<>( 2 , std::vector< std::pair< int , int > >{ { 0 , 2 } } ) ,
   contract_violation );
 CHECK_THROWS( StaticGraph<>( -1 , std::vector< std::pair< int , int > >{} ) ,
	       contract_violation );
 CHECK_THROWS( ( void )One.adj( 1 ) , contract_violation );
 CHECK_THROWS( ( void )One.relabel( { 1 } ) , contract_violation );
 }

// the indexed heap for D = 2 , 4 , 8 against a set of ( key , id ): random
// pushes, decreases and pops, the ties, pushOrDecrease() with a larger
// key, clear(), and the contracts on an id twice, a key raised and an
// empty heap
ADS_TEST( "indexed-dary-heap" )
{
 auto run = [ & ]( auto H ) {
  std::mt19937 g( 2 );
  const int n = 300;
  std::set< std::pair< int , int > > R;
  std::vector< int > key( n , -1 );
  for( int t = 0 ; t < 4000 ; ++t ) {
   const int id = int( g() % n );
   const int k = int( g() % 50 );
   if( g() % 3 ) {
    const bool in = key[ id ] >= 0;
    const bool changed = H.pushOrDecrease( id , k );
    CHECK( changed == ( ! in || ( k < key[ id ] ) ) );
    if( changed ) {
     if( in )
      R.erase( { key[ id ] , id } );
     key[ id ] = k;
     R.emplace( k , id );
     }
    }
   else if( ! R.empty() ) {
    const auto e = H.pop();
    CHECK( e.key == R.begin()->first );  // a least key, ties any id
    CHECK( R.erase( { e.key , e.id } ) == 1 );
    key[ e.id ] = -1;
    }
   CHECK( H.size() == R.size() );
   }
  H.clear();
  CHECK( H.empty() && ! H.contains( 0 ) );
  H.push( 0 , 5 );
  CHECK( H.key( 0 ) == 5 );
  CHECK_THROWS( H.push( 0 , 1 ) , contract_violation );
  CHECK_THROWS( H.decrease( 0 , 9 ) , contract_violation );
  CHECK_THROWS( H.decrease( 1 , 1 ) , contract_violation );
  CHECK( H.pop().id == 0 );
  CHECK_THROWS( H.pop() , contract_violation );
  CHECK_THROWS( ( void )H.top() , contract_violation );
  CHECK_THROWS( ( void )H.contains( n ) , contract_violation );
  H.push( 1 , INT_MIN );  // the extremes of int
  H.push( 2 , INT_MAX );
  CHECK( H.pop().key == INT_MIN && H.pop().key == INT_MAX );
  };
 run( IndexedDaryHeap< int , 2 >( 300 ) );
 run( IndexedDaryHeap< int , 4 >( 300 ) );
 run( IndexedDaryHeap< int , 8 >( 300 ) );
 IndexedDaryHeap< double > Z( 0 );
 CHECK( Z.empty() && Z.ids() == 0 );
 }

// the union-find against MFSet of the book on random merges: the same
// answers to same(), the counts and the sizes; n = 0 and n = 1, a merge of
// one component with itself, elements out of range
ADS_TEST( "union-find" )
{
 std::mt19937 g( 4 );
 const int n = 500;
 UnionFind U( n );
 MFSet M( n , MFSet::Union::SIZE );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const int x = int( g() % n ) , y = int( g() % n );
  if( g() % 2 )
   CHECK( U.merge( x , y ) == M.merge( x , y ) );
  else
   CHECK( U.same( x , y ) == M.same( x , y ) );
  CHECK( U.count() == M.count() );
  }
 for( int x = 0 ; x < n ; ++x )
  CHECK( U.size( x ) == M.size( M.find( x ) ) );
 CHECK( ! U.merge( 3 , 3 ) );
 UnionFind Z( 0 );
 CHECK( Z.count() == 0 && Z.n() == 0 );
 UnionFind One( 1 );
 CHECK( One.find( 0 ) == 0 && One.size( 0 ) == 1 && ! One.merge( 0 , 0 ) );
 CHECK_THROWS( One.find( 1 ) , contract_violation );
 CHECK_THROWS( One.merge( -1 , 0 ) , contract_violation );
 CHECK_THROWS( UnionFind( -1 ) , contract_violation );
 One.reset( 4 );
 CHECK( One.count() == 4 && One.merge( 0 , 3 ) && One.size( 3 ) == 2 );
 // the other rules against the same MFSet: the union by rank and the full
 // compression; a tie puts the root of x under that of y
 auto other = [ & ]( auto V ) {
  std::mt19937 h( 6 );
  MFSet R( n , MFSet::Union::SIZE );
  for( int t = 0 ; t < 3000 ; ++t ) {
   const int x = int( h() % n ) , y = int( h() % n );
   if( h() % 2 )
    CHECK( V.merge( x , y ) == R.merge( x , y ) );
   else
    CHECK( V.same( x , y ) == R.same( x , y ) );
   CHECK( V.count() == R.count() );
   }
  V.reset( 3 );
  CHECK( V.merge( 0 , 1 ) && V.find( 0 ) == 1 );  // the tie
  CHECK( V.merge( 2 , 0 ) && V.find( 2 ) == 1 );  // 2 under the larger
  CHECK( ! V.merge( 2 , 1 ) && V.count() == 1 );
  CHECK_THROWS( V.find( 3 ) , contract_violation );
  };
 other( DisjointSets< Union::SIZE , Compression::FULL >( n ) );
 other( DisjointSets< Union::RANK , Compression::HALVING >( n ) );
 other( DisjointSets< Union::RANK , Compression::FULL >( n ) );
 other( UnionFind( n ) );
 DisjointSets< Union::SIZE , Compression::FULL > F( 5 );
 for( int x = 1 ; x < 5 ; ++x )
  F.merge( x - 1 , x );
 CHECK( F.size( 0 ) == 5 );
 }

// the algorithms of the book on structures given as types: shortestPaths()
// on every priority queue of the library with the priorities ( distance ,
// node ), and on a priority queue made of a std::set, gives the same
// distances, parents, order of extraction and counts, and the distances of
// johnson(); on a queue (ArrayQueue, LinkedQueue, std::queue) those of
// bellmanFordMoore(), a negative cycle detected; on a stack and a deque
// those of the frontiers of the enumeration; prim() and kruskal() the same
// trees and counts on every queue and union-find; distance() the same
// visit on every queue, all on the same graph (the order of its rows
// decides the order of the relaxations), and the distances of the book on
// its ArrayGraph; a frontier that is not empty violates the contracts
ADS_TEST( "book-on-structures" )
{
 using P = std::pair< int , int >;
 std::mt19937 g( 21 );
 for( int t = 0 ; t < 12 ; ++t ) {
  const int n = 2 + int( g() % 300 );
  const Edges E = randomEdges( n , 1 + int( g() % unsigned( 5 * n ) ) ,
			       t % 3 ? 1 : 0 , t % 2 ? 1000 : 3 , g );
  const StaticGraph< int > S( n + 1 , E , true );  // n unreachable
  const ArrayGraph< int > A( n + 1 , E , true );
  // the shortest paths on the priority queues
  ShortestPathTree< int > ref;
  FrontierCounts kref;
  bool first = true;
  auto same = [ & ]( auto & q , const auto & G ) {
   PriorityFrontier F( q , G.bound() );
   FrontierCounts k;
   const auto R = shortestPaths( G , 0 , F , &k );
   if( first ) {
    ref = R;
    kref = k;
    first = false;
    CHECK( R.d == johnson( A , 0 ).d );
    return;
    }
   CHECK( R.d == ref.d && R.T == ref.T && R.order == ref.order );
   CHECK( k.adds == kref.adds && k.extracts == kref.extracts &&
	  k.decreases == kref.decreases && k.edges == kref.edges );
   };
  {
   HeapPriorityQueue< int , P , 2 > q( n + 1 );
   same( q , S );
   }
  {
   HeapPriorityQueue< int , P , 4 > q( n + 1 );
   same( q , S );
   }
  {
   FibonacciHeapPriorityQueue< int , P > q;
   same( q , S );
   }
  {
   PairingHeap< int , P > q;
   same( q , S );
   }
  {
   ListPriorityQueue< int , P , false > q;
   same( q , S );
   }
  {
   IndexedDaryHeap< P , 2 > q( n + 1 );
   same( q , S );
   }
  {
   IndexedDaryHeap< P , 4 > q( n + 1 );
   same( q , S );
   }
  {
   SetQueue q( n + 1 );
   same( q , S );
   }
  CHECK( kref.extracts == kref.adds && kref.edges <= S.arcs() );
  // on the queues, the stack and the deque
  ArrayQueue< int > aq( n + 1 );
  LinkedQueue< int > lq;
  StdQueue sq;
  QueueFrontier FA( aq );
  QueueFrontier FL( lq );
  QueueFrontier FS( sq );
  FrontierCounts ka , kl , ks;
  const auto BA = shortestPaths( S , 0 , FA , &ka );
  CHECK( BA.d == bellmanFordMoore( A , 0 ).d && BA.d == ref.d );
  const auto BL = shortestPaths( S , 0 , FL , &kl );
  const auto BS = shortestPaths( S , 0 , FS , &ks );
  CHECK( BL.order == BA.order && BS.order == BA.order );
  CHECK( ka.adds == kl.adds && ka.adds == ks.adds &&
	 ka.extracts == ks.extracts && ka.decreases == ks.decreases );
  ArrayStack< int > st( n + 1 );
  StackFrontier FT( st );
  CHECK( shortestPaths( S , 0 , FT ).d ==
	 shortestPaths( A , 0 , Frontier::STACK ).d );
  Dequeue< int > dq;
  DequeFrontier FD( dq );
  CHECK( shortestPaths( S , 0 , FD ).d == papeDEsopo( A , 0 ).d );
  // the visit in breadth on the queues
  ArrayQueue< int > vq( n + 1 );
  LinkedQueue< int > wq;
  VisitCounts va , vb;
  const auto DA = distance( S , 0 , vq , &va );
  const auto DB = distance( S , 0 , wq , &vb );
  CHECK( DA == DB && DA.first == distance( A , 0 ).first );
  CHECK( va.nodes == vb.nodes && va.edges == vb.edges );
  // the spanning trees, on an unoriented graph without parallel edges
  // (the audit of prim() reads the first edge to a parent)
  std::set< std::pair< int , int > > seen;
  Edges SE;
  for( const auto & [ u , v , w ] : E )
   if( seen.emplace( std::min( u , v ) , std::max( u , v ) ).second )
    SE.emplace_back( u , v , w );
  const StaticGraph< int > U( n , SE , false );
  std::vector< int > pref;
  FrontierCounts cref;
  bool once = true;
  auto tree = [ & ]( auto & q , const auto & G ) {
   FrontierCounts k;
   const auto p = prim( G , 0 , q , &k );
   if( once ) {
    pref = p;
    cref = k;
    once = false;
    return;
    }
   CHECK( p == pref && k.decreases == cref.decreases &&
	  k.extracts == cref.extracts && k.edges == cref.edges );
   };
  {
   HeapPriorityQueue< int , P , 4 > q( n );
   tree( q , U );
   }
  {
   FibonacciHeapPriorityQueue< int , P > q;
   tree( q , U );
   }
  {
   PairingHeap< int , P > q;
   tree( q , U );
   }
  {
   IndexedDaryHeap< P , 2 > q( n );
   tree( q , U );
   }
  {
   SetQueue q( n );
   tree( q , U );
   }
  long long wp = 0 , wk = 0;
  for( int v = 0 ; v < n ; ++v )
   if( pref[ v ] != NIL ) {
    int w = INT_MAX;
    U.forEachEdge( v , [ & ]( int x , int c ) {
     if( x == pref[ v ] )
      w = std::min( w , c );
     } );
    wp += w;
    }
  std::vector< WeightedEdge< int > > W;
  for( const auto & [ u , v , w ] : SE )
   W.push_back( { u , v , w } );
  for( const auto & e : kruskal( n , W ) )
   wk += e.w;
  bool connected = true;  // then Prim spans it all
  for( int d : distance( U , 0 ).first )
   connected = connected && d != INF;
  if( connected )
   CHECK( wp == wk );
  MergeCounts m1 , m2 , m3;
  MFSet M( n );
  UnionFind F( n );
  DisjointSets< Union::RANK , Compression::FULL > D( n );
  const auto K1 = kruskal( n , W , M , 0 , &m1 );
  const auto K2 = kruskal( n , W , F , 0 , &m2 );
  const auto K3 = kruskal( n , W , D , 1001 , &m3 );  // counting sort
  CHECK( K1 == K2 && K1.size() == K3.size() );
  CHECK( m1.merges == m2.merges && m1.unions == m2.unions &&
	 m1.unions == m3.unions );
  }
 // a negative cycle on a queue, and frontiers that are not empty
 const StaticGraph< int > C(
   3 , Edges{ { 0 , 1 , 1 } , { 1 , 2 , -3 } , { 2 , 1 , 1 } } , true );
 ArrayQueue< int > q( 3 );
 QueueFrontier F( q );
 CHECK( shortestPaths( C , 0 , F ).negativeCycle );
 ArrayQueue< int > full( 3 );
 full.enqueue( 1 );
 QueueFrontier FF( full );
 CHECK_THROWS( shortestPaths( C , 0 , FF ) , contract_violation );
 CHECK_THROWS( distance( C , 0 , full ) , contract_violation );
 HeapPriorityQueue< int , P > h( 3 );
 h.insert( 2 , P( 0 , 2 ) );
 CHECK_THROWS(
   prim( StaticGraph< int >( 3 , Edges{ { 0 , 1 , 1 } } , false ) , 0 , h ) ,
   contract_violation );
 const StaticGraph< int > One( 1 , Edges{} , false );
 HeapPriorityQueue< int , P > h1( 1 );
 CHECK( prim( One , 0 , h1 ) == std::vector< int >{ NIL } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE GRIDS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a grid of rows x cols cells, a quarter of them blocked, the others of
/// cost in [ 1 , 9 ]
std::vector< unsigned char > randomCosts( int rows , int cols ,
					  std::mt19937 & g )
{
 std::vector< unsigned char > cost( std::size_t( rows ) * cols );
 for( auto & c : cost )
  c = ( g() % 4 == 0 ) ? 0 : ( unsigned char )( 1 + g() % 9 );
 return( cost );
 }

/// the sorted neighbours of u in g, by forEachAdj()
template< class G >
std::vector< int > around( const G & g , int u )
{
 std::vector< int > out;
 g.forEachAdj( u , [ & ]( int v ) { out.push_back( v ); } );
 return( out );
 }

// the grid on the edge cases: no cell, 1 x 1, 1 x N and N x 1, all the
// cells blocked, the four and the eight moves of a corner, a side and the
// middle, the three rules on the corners, the weights of the moves on
// weighted and on uniform costs, the symmetry of the adjacency, and the
// contracts on a blocked start, a missing diagonal cost and a vector of
// the wrong size
ADS_TEST( "grid-graph" )
{
 using V = std::vector< int >;
 for( auto [ r , c ] :
      { std::pair( 0 , 0 ) , std::pair( 0 , 5 ) , std::pair( 5 , 0 ) } ) {
  const GridGraph<> Z( r , c );
  CHECK( Z.n() == 0 && Z.bound() == 0 && Z.V().empty() && ! Z.hasNode( 0 ) );
  }
 for( GridMoves mv : { GridMoves::FOUR , GridMoves::EIGHT } ) {
  const GridGraph< double > One( 1 , 1 , mv );
  CHECK( One.n() == 1 && One.adj( 0 ).empty() );
  CHECK( distance( One , 0 ).first == V{ 0 } );
  CHECK( johnson( One , 0 ).d == std::vector< double >{ 0 } );
  const GridGraph< double > Row( 1 , 6 , mv ) , Col( 6 , 1 , mv );
  CHECK( Row.adj( 0 ) == V{ 1 } && Row.adj( 5 ) == V{ 4 } &&
	 Row.adj( 3 ) == ( V{ 2 , 4 } ) );
  CHECK( Col.adj( 0 ) == V{ 1 } && Col.adj( 5 ) == V{ 4 } &&
	 Col.adj( 3 ) == ( V{ 2 , 4 } ) );
  CHECK( distance( Row , 0 ).first == ( V{ 0 , 1 , 2 , 3 , 4 , 5 } ) );
  CHECK( ! Row.directed() );
  }
 // all blocked: no node, no visit
 const GridGraph< int , unsigned char > None(
   3 , 4 , std::vector< unsigned char >( 12 , 0 ) );
 CHECK( None.n() == 0 && None.bound() == 12 && None.V().empty() );
 for( int u = 0 ; u < 12 ; ++u )
  CHECK( ! None.hasNode( u ) );
 CHECK( gridDistances( None , {} ) == V( 12 , INF ) );
 CHECK_THROWS( distance( None , 0 ) , contract_violation );
 // the moves of a corner, of a side and of the middle of a 3 x 3 grid
 const GridGraph< int > F4( 3 , 3 );
 const GridGraph< int > F8( 3 , 3 , GridMoves::EIGHT , Corners::NO_CUT , 2 ,
			    3 );
 CHECK( F4.adj( 0 ) == ( V{ 1 , 3 } ) && F4.adj( 4 ) == ( V{ 1 , 3 , 5 ,
							     7 } ) &&
	F4.adj( 5 ) == ( V{ 2 , 4 , 8 } ) );
 CHECK( F8.adj( 0 ) == ( V{ 1 , 3 , 4 } ) &&
	F8.adj( 4 ) == ( V{ 0 , 1 , 2 , 3 , 5 , 6 , 7 , 8 } ) &&
	F8.adj( 8 ) == ( V{ 4 , 5 , 7 } ) );
 std::vector< std::pair< int , int > > e;
 F8.forEachEdge( 4 , [ & ]( int v , int w ) { e.emplace_back( v , w ); } );
 CHECK( e == ( std::vector< std::pair< int , int > >{ { 0 , 3 } ,
						     { 1 , 2 } ,
						     { 2 , 3 } ,
						     { 3 , 2 } ,
						     { 5 , 2 } ,
						     { 6 , 3 } ,
						     { 7 , 2 } ,
						     { 8 , 3 } } ) );
 CHECK( F8.node( 2 , 1 ) == 7 && F8.cell( 7 ) == GridCell( 2 , 1 ) );
 // the corners of a 2 x 2 grid with the cells 1 and 2 blocked, then with
 // the cell 1 alone
 auto two = [ & ]( std::vector< int > cost , Corners k ) {
  return(
    GridGraph< int >( 2 , 2 , std::move( cost ) , GridMoves::EIGHT , k , 1 ,
		      1 ) );
  };
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::CUT ).adj( 0 ) == V{ 3 } );
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::NO_SQUEEZE ).adj( 0 ).empty() );
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::NO_CUT ).adj( 0 ).empty() );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::CUT ).adj( 0 ) == ( V{ 2 , 3 } ) );
 CHECK( two( { 1 , 0 , 1 , 1 } ,
	     Corners::NO_SQUEEZE ).adj( 0 ) == ( V{ 2 , 3 } ) );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::NO_CUT ).adj( 0 ) == V{ 2 } );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::NO_CUT ).adj( 3 ) == V{ 2 } );
 // the weights: the cost of the cell entered times that of the move
 const GridGraph< int > Wt( 1 , 3 , std::vector< int >{ 1 , 5 , 2 } ,
			    GridMoves::FOUR , Corners::NO_CUT , 10 );
 CHECK( Wt.directed() && Wt.minCost() == 1 );
 e.clear();
 Wt.forEachEdge( 1 , [ & ]( int v , int w ) { e.emplace_back( v , w ); } );
 CHECK( e ==
	( std::vector< std::pair< int , int > >{ { 0 , 10 } , { 2 , 20 } } ) );
 CHECK( johnson( Wt , 0 ).d == ( V{ 0 , 50 , 70 } ) );
 const GridGraph< int > Un( 2 , 2 , std::vector< int >{ 3 , 3 , 0 , 3 } );
 CHECK( ! Un.directed() && Un.n() == 3 && Un.minCost() == 3 );
 e.clear();
 Un.forEachEdge( 1 , [ & ]( int v , int w ) { e.emplace_back( v , w ); } );
 CHECK( e == ( std::vector< std::pair< int , int > >{ { 0 , 3 } ,
						      { 3 , 3 } } ) );
 // the nodes, V() and nodes() alike, and the adjacency symmetric
 std::mt19937 g( 7 );
 for( Corners k : { Corners::CUT , Corners::NO_SQUEEZE , Corners::NO_CUT } )
  for( GridMoves mv : { GridMoves::FOUR , GridMoves::EIGHT } ) {
   const GridGraph< int , unsigned char > G( 9 , 11 ,
					     randomCosts( 9 , 11 , g ) , mv ,
					    k , 2 , 3 );
   V nodes;
   for( int u : G.nodes() )
    nodes.push_back( u );
   CHECK( nodes == G.V() && int( nodes.size() ) == G.n() );
   for( int u : G.V() ) {
    const V a = G.adj( u );
    CHECK( a == around( G , u ) && std::is_sorted( a.begin() , a.end() ) );
    for( int v : a ) {
     const V b = G.adj( v );
     CHECK( std::find( b.begin() , b.end() , u ) != b.end() );
     }
    }
   }
 // the contracts
 const GridGraph< int > Blocked( 1 , 2 , std::vector< int >{ 0 , 1 } );
 CHECK_THROWS( distance( Blocked , 0 ) , contract_violation );
 CHECK_THROWS( johnson( Blocked , 0 ) , contract_violation );
 CHECK_THROWS( ( void )Blocked.adj( 0 ) , contract_violation );
 CHECK_THROWS( gridDistances( Blocked , { 0 } ) , contract_violation );
 CHECK_THROWS( GridGraph< int >( 2 , 2 , GridMoves::EIGHT ) ,
	       contract_violation );
 CHECK_THROWS( GridGraph< int >( 2 , 2 , std::vector< int >( 3 , 1 ) ) ,
	       contract_violation );
 CHECK_THROWS( GridGraph< int >( -1 , 2 ) , contract_violation );
 CHECK_THROWS(
   GridGraph< double >( 2 , 2 , GridMoves::FOUR , Corners::NO_CUT , 0.0 ) ,
   contract_violation );
 CHECK_THROWS( ( void )F4.node( 3 , 0 ) , contract_violation );
 CHECK_THROWS( ( void )F4.cell( 9 ) , contract_violation );
 }

// the visit in breadth and the shortest paths on random grids against the
// same on the explicit graphs of the grid (ArrayGraph, StaticGraph), on the
// four and the eight moves and the three rules on the corners; the visit
// of the parents too on StaticGraph, whose rows are in the same order;
// gridDistances() against that of Grid.h on the matrix and floodFill() on
// the free cells; the heuristics admissible and consistent, exact on a free
// grid
ADS_TEST( "grid-graph-paths" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 6 ; ++t ) {
  const int R = 1 + int( g() % 23 ) , Cc = 1 + int( g() % 29 );
  const auto cost = randomCosts( R , Cc , g );
  for( Corners k : { Corners::CUT , Corners::NO_SQUEEZE , Corners::NO_CUT } )
   for( GridMoves mv : { GridMoves::FOUR , GridMoves::EIGHT } ) {
    const GridGraph< int , unsigned char > G( R , Cc , cost , mv , k , 5 , 7 );
    const ArrayGraph< int > A( G );
    const StaticGraph< int > S( G );
    CHECK( A.n() == G.bound() && S.n() == G.bound() );
    CHECK( S.directed() == G.directed() );
    // a few sources, each against the explicit graphs: the audits of
    // johnson() run a Bellman-Ford of O( n m ) each
    const std::vector< int > all = G.V();
    for( std::size_t i = 0 ; i < all.size() ; i += all.size() / 5 + 1 ) {
     const int s = all[ i ];
     const auto dg = distance( G , s );
     const auto ds = distance( S , s );
     CHECK( dg == ds );
     CHECK( dg.first == distance( A , s ).first );
     CHECK( dg.first == gridDistances( G , { s } ) );
     const auto jg = johnson( G , s );
     CHECK( jg.d == johnson( A , s ).d && jg.d == johnson( S , s ).d );
     CHECK( isShortestPathTree( G , s , jg ) );
     // the heuristic towards s: no more than the distance, consistent
     for( int u : G.V() ) {
      const int h = gridHeuristic( G , u , s );
      CHECK( ( jg.d[ u ] == infinity< int >() ) || ( h <= jg.d[ u ] ) );
      G.forEachEdge( u , [ & ]( int v , int w ) {
       CHECK( h <= w + gridHeuristic( G , v , s ) );
       } );
      }
     }
    if( G.n() == 0 )
     continue;
    // several sources at once, against Grid.h on the matrix
    const std::vector< int > Src = { G.V().front() , G.V().back() };
    const auto D = gridDistances( G , Src );
    if( ( mv == GridMoves::FOUR ) || ( k == Corners::CUT ) ) {
     auto free = [ & ]( GridCell x ) {
      return( cost[ std::size_t( x.first ) * Cc + x.second ] > 0 );
      };
     std::vector< GridCell > X;
     for( int s : Src )
      X.push_back( G.cell( s ) );
     const auto M = mv == GridMoves::FOUR
		      ? ADS::gridDistances( R , Cc , X , free )
		      : ADS::gridDistances( R , Cc , X , free , EIGHT_MOVES );
     for( int u = 0 ; u < G.bound() ; ++u )
      CHECK( M[ u / Cc ][ u % Cc ] == D[ u ] );
     }
    // the paint bucket on the free cells reaches what the visit reaches
    if( mv == GridMoves::FOUR ) {
     std::vector< std::vector< int > > P( R , std::vector< int >( Cc ) );
     for( int u = 0 ; u < G.bound() ; ++u )
      P[ u / Cc ][ u % Cc ] = G.hasNode( u ) ? 1 : 0;
     const GridCell x = G.cell( Src[ 0 ] );
     const auto d = distance( G , Src[ 0 ] ).first;
     const std::size_t k1 = floodFill( P , x.first , x.second , 2 );
     CHECK( k1 == std::size_t( std::count_if( d.begin() , d.end() ,
					      []( int y ) {
	     return( y != INF );
	     } ) ) );
     for( int u = 0 ; u < G.bound() ; ++u )
      CHECK( ( P[ u / Cc ][ u % Cc ] == 2 ) == ( d[ u ] != INF ) );
     }
    }
  }
 // on a free grid the octile distance is the distance, with the octile
 // costs in double and no cut corners
 const GridGraph< double > F( 13 , 17 , GridMoves::EIGHT );
 const auto d = johnson( F , F.node( 6 , 8 ) ).d;
 for( int u = 0 ; u < F.bound() ; ++u )
  CHECK( std::fabs( d[ u ] - gridHeuristic( F , u ,
					    F.node( 6 , 8 ) ) ) < 1e-9 );
 CHECK( manhattan( GridCell( 0 , 0 ) , GridCell( 2 , -3 ) ) == 5 );
 CHECK( chebyshev( GridCell( 0 , 0 ) , GridCell( 2 , -3 ) , 4 ) == 12 );
 CHECK( octile( GridCell( 0 , 0 ) , GridCell( 2 , -3 ) , 5 , 7 ) == 19 );
 CHECK_THROWS( octile( GridCell( 0 , 0 ) , GridCell( 1 , 1 ) , 5 , 11 ) ,
	       contract_violation );
 }

// the grids of 2^31 - 1 cells and nearly so, with no vector of the costs:
// the names of the last cells and their moves, none of which overflows an
// int (the sanitizers would tell), the four corners of the largest square,
// and the contracts on a grid of 2^31 cells
ADS_TEST( "grid-graph-huge" )
{
 using V = std::vector< int >;
 const int k = 46340;  // k ( k + 1 ) < 2^31 - 1
 const GridGraph< long long > Q( k , k + 1 , GridMoves::EIGHT ,
				 Corners::NO_CUT ,
				 1 , 1 );
 const int last = Q.bound() - 1;
 CHECK( Q.bound() == k * ( k + 1 ) && Q.n() == Q.bound() );
 CHECK( Q.node( k - 1 , k ) == last );
 CHECK( Q.cell( last ) == GridCell( k - 1 , k ) );
 CHECK( Q.adj( last ) == ( V{ last - k - 2 , last - k - 1 , last - 1 } ) );
 CHECK( Q.adj( 0 ) == ( V{ 1 , k + 1 , k + 2 } ) );
 CHECK( Q.adj( k ) == ( V{ k - 1 , 2 * k , 2 * k + 1 } ) );
 CHECK( Q.adj( last - k ) ==
	( V{ last - 2 * k - 1 , last - 2 * k , last - k + 1 } ) );
 CHECK( gridHeuristic( Q , 0 , last ) == k );
 const GridGraph<> Row( 1 , INT_MAX , GridMoves::EIGHT , Corners::CUT , 1 ,
			1 );
 const GridGraph<> Col( INT_MAX , 1 , GridMoves::EIGHT , Corners::CUT , 1 ,
			1 );
 for( const auto * G : { &Row , &Col } ) {
  CHECK( G->bound() == INT_MAX && G->hasNode( INT_MAX - 1 ) );
  CHECK( ! G->hasNode( INT_MAX ) && ! G->hasNode( -1 ) );
  CHECK( G->adj( INT_MAX - 1 ) == V{ INT_MAX - 2 } );
  CHECK( G->adj( 0 ) == V{ 1 } );
  }
 CHECK( manhattan< long long >(
	  GridCell( 0 , 0 ) ,
	  GridCell( INT_MAX , INT_MAX ) ) == 2LL * INT_MAX );
 CHECK(
   manhattan< long long >( GridCell( INT_MIN , 0 ) ,
			   GridCell( INT_MAX , 0 ) ) ==
   1LL * INT_MAX - INT_MIN );
 CHECK_THROWS( GridGraph<>( 65536 , 32768 ) , contract_violation );
 CHECK_THROWS( GridGraph<>( k + 1 , k + 1 ) , contract_violation );
 CHECK_THROWS( GridGraph<>( INT_MAX , 2 ) , contract_violation );
 }

// the two models of the cost of a move: the cell entered and the cell
// left; a 2 x 2 map where their shortest paths differ, random maps of four
// and eight moves under the three rules on the corners against an explicit
// graph whose weights are computed from the geometry of each move, the
// flag of a corner of forEachMove(), the shift by a ( cost( s ) - cost( t ) )
// of the four moves, the distances from s of one model as the distances to
// s of the other, the heuristic admissible and consistent under both, the
// orientation the same, and a map with no vector where the two coincide
ADS_TEST( "grid-graph-cost-models" )
{
 using G8 = GridGraph< int , unsigned char >;
 // 2 x 2, the cells 0 1 / 2 3, costs 1 1 / 1 9, a = 2 and b = 3: entering
 // 3 costs 27 on the diagonal and 2 + 18 by a side; leaving 0 costs 3 on
 // the diagonal and 2 + 2 by a side
 const std::vector< unsigned char > c4 = { 1 , 1 , 1 , 9 };
 const G8 E( 2 , 2 , c4 , GridMoves::EIGHT , Corners::NO_CUT , 2 , 3 );
 const G8 L( 2 , 2 , c4 , GridMoves::EIGHT , Corners::NO_CUT , 2 , 3 ,
	     MoveCost::LEFT );
 CHECK( E.moveCost() == MoveCost::ENTERED && L.moveCost() == MoveCost::LEFT );
 const auto pe = johnson( E , 0 ) , pl = johnson( L , 0 );
 CHECK( pe.d[ 3 ] == 20 && pe.T[ 3 ] != 0 );
 CHECK( pl.d[ 3 ] == 3 && pl.T[ 3 ] == 0 );
 CHECK( E.directed() && L.directed() );
 std::vector< std::tuple< int , int , bool > > mv;
 E.forEachMove( 0 ,
		[ & ]( int v , bool d ) { mv.emplace_back( 0 , v , d ); } );
 CHECK( mv == ( std::vector< std::tuple< int , int , bool > >{
		{ 0 , 1 , false } , { 0 , 2 , false } , { 0 , 3 , true } } ) );
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 5 ; ++t ) {
  const int R = 1 + int( g() % 13 ) , Cc = 1 + int( g() % 17 );
  const auto cost = randomCosts( R , Cc , g );
  for( Corners k : { Corners::CUT , Corners::NO_SQUEEZE , Corners::NO_CUT } )
   for( GridMoves m : { GridMoves::FOUR , GridMoves::EIGHT } ) {
    std::vector< std::vector< int > > dist[ 2 ] , to;
    const std::vector< int > all = G8( R , Cc , cost ).V();
    for( MoveCost model : { MoveCost::ENTERED , MoveCost::LEFT } ) {
     const G8 G( R , Cc , cost , m , k , 5 , 7 , model );
     // the explicit graph: the weight of each move from its geometry and
     // from the cost of the cell that pays
     std::vector< std::tuple< int , int , int > > X;
     for( int u : G.V() )
      G.forEachMove( u , [ & ]( int v , bool d ) {
       const GridCell a = G.cell( u ) , b = G.cell( v );
       const bool diag = ( a.first != b.first ) && ( a.second != b.second );
       CHECK( d == diag );
       const int pay = model == MoveCost::LEFT ? cost[ u ] : cost[ v ];
       X.emplace_back( u , v , pay * ( diag ? 7 : 5 ) );
       } );
     const ArrayGraph< int > A( G.bound() , X , true );
     for( int u : G.V() ) {
      std::vector< std::pair< int , int > > x , y;
      G.forEachEdge( u , [ & ]( int v , int w ) { x.emplace_back( v , w ); } );
      A.forEachEdge( u , [ & ]( int v , int w ) { y.emplace_back( v , w ); } );
      CHECK( x == y );
      }
     CHECK( G.directed() == G8( R , Cc , cost , m , k , 5 , 7 ).directed() );
     auto & D = dist[ model == MoveCost::LEFT ];
     for( std::size_t i = 0 ; i < all.size() ; i += all.size() / 4 + 1 ) {
      const int s = all[ i ];
      const auto J = johnson( G , s );
      CHECK( J.d == johnson( A , s ).d );
      D.push_back( J.d );
      for( int u : G.V() ) {
       const int h = gridHeuristic( G , u , s );
       G.forEachEdge( u , [ & ]( int v , int w ) {
	CHECK( h <= w + gridHeuristic( G , v , s ) );
        } );
       }
      // the costs to s, on the transposed graph: the heuristic towards s
      // is a lower bound on them
      const auto T = johnson( StaticGraph< int >( A ).transpose() , s ).d;
      for( int u : G.V() )
       CHECK( ( T[ u ] == infinity< int >() ) ||
	      ( gridHeuristic( G , u , s ) <= T[ u ] ) );
      if( model == MoveCost::ENTERED )
       to.push_back( T );
      }
     }
    // the distances from s leaving are those to s entering, and with four
    // moves they differ by 5 ( cost( s ) - cost( t ) )
    std::size_t q = 0;
    for( std::size_t i = 0 ; i < all.size() ; i += all.size() / 4 + 1 , ++q ) {
     const int s = all[ i ];
     for( int t : all ) {
      const int de = dist[ 0 ][ q ][ t ] , dl = dist[ 1 ][ q ][ t ];
      CHECK( ( de == infinity< int >() ) == ( dl == infinity< int >() ) );
      if( de == infinity< int >() )
       continue;
      if( m == GridMoves::FOUR )
       CHECK( dl - de == 5 * ( int( cost[ s ] ) - int( cost[ t ] ) ) );
      CHECK( dl == to[ q ][ t ] );
      }
     }
    }
  }
 // with no vector the two models give the same weights
 const GridGraph< double > F( 4 , 5 , GridMoves::EIGHT , Corners::CUT , 1.0 ,
			      std::nullopt , MoveCost::LEFT );
 const GridGraph< double > H( 4 , 5 , GridMoves::EIGHT , Corners::CUT );
 for( int u : F.V() ) {
  std::vector< std::pair< int , double > > x , y;
  F.forEachEdge( u , [ & ]( int v , double w ) { x.emplace_back( v , w ); } );
  H.forEachEdge( u , [ & ]( int v , double w ) { y.emplace_back( v , w ); } );
  CHECK( x == y );
  }
 const G8 B( 1 , 2 , std::vector< unsigned char >{ 0 , 1 } );
 CHECK_THROWS( B.forEachMove( 0 , []( int , bool ) { } ) ,
	       contract_violation );
 CHECK_THROWS( B.forEachMove( 2 , []( int , bool ) { } ) ,
	       contract_violation );
 }

// a map in the format of the benchmarks of Sturtevant: its free and its
// blocked characters, the octile distances with no cut corners against
// the explicit graph, and the contracts on a header and on rows that are
// not those of the format
ADS_TEST( "grid-map" )
{
 std::istringstream in( "type octile\nheight 4\nwidth 5\nmap\n"
			"..@..\n.T.G.\n.S..W\n@@O..\n" );
 const auto M = readGridMap( in );
 CHECK( M.rows() == 4 && M.cols() == 5 && M.n() == 14 );
 CHECK( ! M.hasNode( 2 ) && ! M.hasNode( 6 ) && M.hasNode( 8 ) &&
	M.hasNode( 11 ) && ! M.hasNode( 14 ) && ! M.hasNode( 17 ) );
 const auto d = johnson( M , 0 ).d;
 const ArrayGraph< double > A( M );
 const auto e = johnson( A , 0 ).d;
 for( int u = 0 ; u < M.bound() ; ++u )
  CHECK( ( d[ u ] == e[ u ] ) || ( std::fabs( d[ u ] - e[ u ] ) < 1e-9 ) );
 // the T at 6 forbids the corners around it: 0 5 10 11 12 in four
 // moves, then 12 8 across a free corner and 8 9
 CHECK( std::fabs( d[ 12 ] - 4.0 ) < 1e-9 );
 CHECK( std::fabs( d[ 9 ] - ( 5.0 + std::sqrt( 2.0 ) ) ) < 1e-9 );
 std::istringstream empty( "type octile\nheight 0\nwidth 0\nmap\n" );
 CHECK( readGridMap( empty ).n() == 0 );
 std::istringstream bad( "type octile\nheight 2\nwidth 3\nmap\n...\n..\n" );
 CHECK_THROWS( readGridMap( bad ) , contract_violation );
 std::istringstream nohead( "height 2\nwidth 3\nmap\n...\n...\n" );
 CHECK_THROWS( readGridMap( nohead ) , contract_violation );
 }

// the builder: no node, isolated nodes added one at a time, parallel edges
// kept, the order of insertion kept by ArrayGraph and the rows sorted by
// StaticGraph, an unoriented edge in both rows at its place, the weights,
// random graphs against the constructors from the edges, and the contracts
// on a loop and on a node not there
ADS_TEST( "graph-builder" )
{
 using V = std::vector< int >;
 const GraphBuilder< int > Z;
 CHECK( Z.n() == 0 && Z.m() == 0 );
 CHECK( Z.toStaticGraph().n() == 0 && Z.toArrayGraph().n() == 0 );
 GraphBuilder<> I;
 for( int i = 0 ; i < 4 ; ++i )
  CHECK( I.addNode() == i );
 I.addEdge( 2 , 0 );
 const auto IS = I.toStaticGraph();
 CHECK( IS.n() == 4 && IS.m() == 1 && IS.degree( 1 ) == 0 &&
	IS.degree( 3 ) == 0 && IS.adj( 2 ) == V{ 0 } );
 GraphBuilder< int > B( 3 );
 B.reserve( 4 );
 B.addEdge( 0 , 2 , 5 );
 B.addEdge( 0 , 1 , 1 );
 B.addEdge( 0 , 2 , 3 );  // parallel
 B.addEdge( 1 , 0 , 4 );
 CHECK( B.m() == 4 && B.edges()[ 2 ] == std::make_tuple( 0 , 2 , 3 ) );
 const auto BA = B.toArrayGraph();
 const auto BS = B.toStaticGraph();
 CHECK( BA.adj( 0 ) == ( V{ 2 , 1 , 2 } ) && BS.adj( 0 ) == ( V{ 1 , 2 ,
								 2 } ) );
 std::vector< std::pair< int , int > > w;
 BA.forEachEdge( 0 , [ & ]( int v , int x ) { w.emplace_back( v , x ); } );
 CHECK( w == ( std::vector< std::pair< int , int > >{
	       { 2 , 5 } , { 1 , 1 } , { 2 , 3 } } ) );
 CHECK( johnson( BS , 0 ).d == ( V{ 0 , 1 , 3 } ) );
 GraphBuilder< int > U( 4 , false );
 U.addEdge( 3 , 1 , 1 );
 U.addEdge( 0 , 3 , 1 );
 U.addEdge( 3 , 2 , 1 );
 CHECK( U.toArrayGraph().adj( 3 ) == ( V{ 1 , 0 , 2 } ) );
 CHECK( U.toStaticGraph().adj( 3 ) == ( V{ 0 , 1 , 2 } ) );
 CHECK( U.toStaticGraph().m() == 3 && U.toArrayGraph().m() == 3 );
 std::mt19937 g( 5 );
 for( bool directed : { true , false } ) {
  const Edges E = randomEdges( 40 , 200 , 1 , 9 , g );
  GraphBuilder< int > R( 40 , directed );
  for( const auto & [ u , v , x ] : E )
   R.addEdge( u , v , x );
  const StaticGraph< int > S( 40 , E , directed );
  const ArrayGraph< int > A( 40 , E , directed );
  const auto RS = R.toStaticGraph();
  const auto RA = R.toArrayGraph();
  for( int u = 0 ; u < 40 ; ++u ) {
   CHECK( RS.adj( u ) == S.adj( u ) && RA.adj( u ) == A.adj( u ) );
   std::vector< std::pair< int , int > > x , y;
   RA.forEachEdge( u , [ & ]( int v , int c ) { x.emplace_back( v , c ); } );
   A.forEachEdge( u , [ & ]( int v , int c ) { y.emplace_back( v , c ); } );
   CHECK( x == y );
   }
  }
 CHECK_THROWS( B.addEdge( 1 , 1 , 0 ) , contract_violation );
 CHECK_THROWS( B.addEdge( 0 , 3 , 0 ) , contract_violation );
 CHECK_THROWS( B.addEdge( -1 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( GraphBuilder<>( -1 ) , contract_violation );
 }

/// whether the labels a and b of the nodes make the same partition, i.e.,
/// a[ u ] = a[ v ] if and only if b[ u ] = b[ v ]
bool samePartition( const std::vector< int > & a ,
		    const std::vector< int > & b )
{
 if( a.size() != b.size() )
  return( false );
 std::vector< int > ab( a.size() + 1 , -1 ) , ba( a.size() + 1 , -1 );
 for( std::size_t u = 0 ; u < a.size() ; ++u ) {
  if( ( a[ u ] < 0 ) || ( b[ u ] < 0 ) || ( a[ u ] > int( a.size() ) ) ||
      ( b[ u ] > int( a.size() ) ) )
   return( false );
  if( ab[ a[ u ] ] == -1 )
   ab[ a[ u ] ] = b[ u ];
  if( ba[ b[ u ] ] == -1 )
   ba[ b[ u ] ] = a[ u ];
  if( ( ab[ a[ u ] ] != b[ u ] ) || ( ba[ b[ u ] ] != a[ u ] ) )
   return( false );
  }
 return( true );
 }

// the strongly connected components in one pass, Tarjan's and Pearce's,
// against the closure on every oriented graph of up to 4 nodes and on
// random ones of 5 to 7, and against scc() (Kosaraju) on large ones: the
// same partition, numbered from 1 in a topological order of the graph of
// the components (which the audits check); unoriented graphs, where they
// are the connected components; a deleted node, numbered 0; parallel
// arcs; paths and circuits of 10^6 nodes, which a recursion would not
// survive
ADS_TEST( "scc-one-pass" )
{
 auto check = [ & ]( const auto & h , int n ) {
  std::vector< std::vector< bool > > R( n , std::vector< bool >( n ) );
  for( int u : h.V() ) {
   R[ u ][ u ] = true;
   h.forEachAdj( u , [ & ]( int v ) { R[ u ][ v ] = true; } );
   }
  for( int k = 0 ; k < n ; ++k )
   for( int u = 0 ; u < n ; ++u )
    for( int v = 0 ; v < n ; ++v )
     if( R[ u ][ k ] && R[ k ][ v ] )
      R[ u ][ v ] = true;
  const auto K = scc( h ) , T = sccTarjan( h ) , P = sccPearce( h );
  for( int u : h.V() )
   for( int v : h.V() ) {
    const bool same = R[ u ][ v ] && R[ v ][ u ];
    CHECK( ( T[ u ] == T[ v ] ) == same && ( P[ u ] == P[ v ] ) == same );
    }
  CHECK( samePartition( K , T ) && samePartition( K , P ) );
  CHECK( *std::max_element( K.begin() , K.end() ) ==
	 *std::max_element( T.begin() , T.end() ) );
  };
 for( int n = 1 ; n <= 4 ; ++n ) {
  std::vector< std::pair< int , int > > A;
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v )
     A.emplace_back( u , v );
  for( unsigned mask = 0 ; mask < ( 1u << A.size() ) ; ++mask ) {
   ListGraph<> h( n , true );
   for( std::size_t i = 0 ; i < A.size() ; ++i )
    if( mask & ( 1u << i ) )
     h.insertEdge( A[ i ].first , A[ i ].second );
   check( h , n );
   }
  }
 std::mt19937 g( 11 );
 for( int it = 0 ; it < 3000 ; ++it ) {
  const int n = 5 + it % 3;
  ListGraph<> h( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && ( g() % 3 == 0 ) )
     h.insertEdge( u , v );
  if( it % 5 == 0 )
   h.deleteNode( int( g() % unsigned( n ) ) );
  check( h , n );
  const auto T = sccTarjan( h ) , P = sccPearce( h );
  for( int u = 0 ; u < n ; ++u )
   CHECK( ( T[ u ] == 0 ) == ! h.hasNode( u ) &&
	  ( P[ u ] == 0 ) == ! h.hasNode( u ) );
  }

 // the edges: no node, no edge, an unoriented graph (its connected
 // components), parallel arcs, the chain of 10^6 nodes (numbered along
 // it) and the same chain closed
 CHECK( sccTarjan( ListGraph<>( 0 ) ).empty() &&
	sccPearce( ListGraph<>( 0 ) ).empty() );
 auto e = sccPearce( ListGraph<>( 4 ) );
 std::sort( e.begin() , e.end() );
 CHECK( e == std::vector< int >( { 1 , 2 , 3 , 4 } ) );
 ListGraph<> u( 6 , false );
 u.insertEdge( 0 , 3 );
 u.insertEdge( 3 , 5 );
 u.insertEdge( 1 , 2 );
 CHECK( samePartition( sccTarjan( u ) , cc( u ) ) &&
	samePartition( sccPearce( u ) , cc( u ) ) );
 const StaticGraph<> par(
   3 ,
   std::vector< std::pair< int , int > >(
     { { 0 , 1 } , { 0 , 1 } , { 1 , 0 } , { 1 , 2 } , { 1 , 2 } } ) ,
   true );
 CHECK( sccTarjan( par ) == std::vector< int >( { 1 , 1 , 2 } ) &&
	sccPearce( par ) == std::vector< int >( { 1 , 1 , 2 } ) );
 const int N = 1000000;
 std::vector< std::pair< int , int > > C;
 for( int v = 0 ; v + 1 < N ; ++v )
  C.emplace_back( v , v + 1 );
 const StaticGraph<> chain( N , C , true );
 const auto TC = sccTarjan( chain ) , PC = sccPearce( chain );
 bool along = true;
 for( int v = 0 ; v < N ; ++v )
  along = along && ( TC[ v ] == v + 1 ) && ( PC[ v ] == v + 1 );
 CHECK( along );
 C.emplace_back( N - 1 , 0 );
 const StaticGraph<> ring( N , C , true );
 const auto TR = sccTarjan( ring ) , PR = sccPearce( ring );
 CHECK(
   std::all_of( TR.begin() , TR.end() , []( int x ) { return( x == 1 ); } ) &&
   TR == PR );

 // random large graphs against Kosaraju: sparse ones, with many small
 // components and a giant one, and denser ones
 for( int it = 0 ; it < 4 ; ++it ) {
  const int n = 100000 , m = ( it + 1 ) * n / 2 + n / 3;
  std::vector< std::pair< int , int > > E;
  while( int( E.size() ) < m ) {
   const int a = int( g() % unsigned( n ) ) , b = int( g() % unsigned( n ) );
   if( a != b )
    E.emplace_back( a , b );
   }
  const StaticGraph<> h( n , E , true );
  const auto K = scc( h ) , T = sccTarjan( h ) , P = sccPearce( h );
  CHECK( samePartition( K , T ) && samePartition( K , P ) );
  }
 }

/// whether the m edges E, oriented or not, have a trail that uses each of
/// them once from s (any start if s = -1), closed if asked, by backtracking
bool trailExists( int n , const std::vector< std::pair< int , int > > & E ,
		  bool directed , int s , bool closed )
{
 if( E.empty() )
  return( true );
 std::vector< bool > used( E.size() , false );
 std::function< bool( int , int , std::size_t ) > go = [ & ]( int start ,
							      int u ,
							    std::size_t k ) {
  if( k == E.size() )
   return( ! closed || ( u == start ) );
  for( std::size_t e = 0 ; e < E.size() ; ++e ) {
   if( used[ e ] )
    continue;
   int v = -1;
   if( E[ e ].first == u )
    v = E[ e ].second;
   else if( ! directed && ( E[ e ].second == u ) )
    v = E[ e ].first;
   if( v < 0 )
    continue;
   used[ e ] = true;
   const bool ok = go( start , v , k + 1 );
   used[ e ] = false;
   if( ok )
    return( true );
   }
  return( false );
  };
 for( int a = 0 ; a < n ; ++a )
  if( ( ( s == -1 ) || ( s == a ) ) && go( a , a , 0 ) )
   return( true );
 return( false );
 }

// the Eulerian trails, open and closed, of oriented and unoriented graphs
// against a backtracking on every graph of up to 4 nodes (oriented) and 5
// (unoriented) and on random multigraphs of parallel edges: eulerianTrail()
// with and without a start, eulerianCycle(), isEulerianTrail() on what
// they give; the walks on random Eulerian graphs of 10^5 nodes and 10^6
// edges, and on paths of 10^6 nodes, open and closed, oriented or not
ADS_TEST( "euler-trails" )
{
 using EL = std::vector< std::pair< int , int > >;
 auto check = [ & ]( const auto & h , int n , const EL & E , bool d ) {
  const auto T = eulerianTrail( h );
  CHECK( T.has_value() == trailExists( n , E , d , -1 , false ) );
  if( T )
   CHECK( isEulerianTrail( h , *T ) );
  const auto C = eulerianCycle( h );
  CHECK( C.has_value() == trailExists( n , E , d , -1 , true ) );
  if( C )
   CHECK( isEulerianCycle( h , *C ) && isEulerianTrail( h , *C ) );
  if( C && ! E.empty() )
   CHECK( T && T->front() == T->back() );
  for( int s = 0 ; s < n ; ++s ) {
   const auto S = eulerianTrail( h , s );
   CHECK( S.has_value() ==
	  ( trailExists( n , E , d , s , false ) &&
	    ( E.empty() || std::any_of( E.begin() , E.end() , [ & ]( auto e ) {
	       return( e.first == s || ( ! d && e.second == s ) );
	       } ) ) ) );
   if( S )
    CHECK( isEulerianTrail( h , *S ) && ( S->empty() || S->front() == s ) );
   }
  };
 for( bool d : { true , false } )
  for( int n = 1 ; n <= ( d ? 4 : 5 ) ; ++n ) {
   EL A;
   for( int u = 0 ; u < n ; ++u )
    for( int v = d ? 0 : u + 1 ; v < n ; ++v )
     if( u != v )
      A.emplace_back( u , v );
   for( unsigned mask = 0 ; mask < ( 1u << A.size() ) ; ++mask ) {
    ListGraph<> h( n , d );
    EL E;
    for( std::size_t i = 0 ; i < A.size() ; ++i )
     if( mask & ( 1u << i ) ) {
      h.insertEdge( A[ i ].first , A[ i ].second );
      E.push_back( A[ i ] );
      }
    check( h , n , E , d );
    }
   }
 std::mt19937 g( 17 );
 for( int it = 0 ; it < 3000 ; ++it ) {
  const bool d = it % 2 == 0;
  const int n = 2 + it % 3 , m = int( g() % 7 );
  EL E;
  while( int( E.size() ) < m ) {
   const int a = int( g() % unsigned( n ) ) , b = int( g() % unsigned( n ) );
   if( a != b )
    E.emplace_back( a , b );
   }
  const StaticGraph<> h( n , E , d );
  check( h , n , E , d );
  }

 // the edges: an isolated node beside a cycle, a start with no edge, the
 // unoriented triangle with a tail (the trail from an odd node), the
 // checkers on a wrong trail, a repeated edge and a reversed arc
 ListGraph<> u( 5 , false );
 u.insertEdge( 0 , 1 );
 u.insertEdge( 1 , 2 );
 u.insertEdge( 2 , 0 );
 CHECK( eulerianCycle( u ) && eulerianCycle( u )->size() == 4 );
 CHECK( ! eulerianTrail( u , 4 ) && eulerianTrail( u , 1 ) );
 u.insertEdge( 2 , 3 );
 CHECK( ! eulerianCycle( u ) && ! eulerianTrail( u , 0 ) );
 const auto ut = eulerianTrail( u );
 CHECK( ut && ut->front() == 2 && ut->back() == 3 );
 CHECK( eulerianTrail( u , 3 ) && eulerianTrail( u , 3 )->back() == 2 );
 CHECK( isEulerianTrail( u , { 3 , 2 , 1 , 0 , 2 } ) &&
	! isEulerianTrail( u , { 3 , 2 , 1 , 0 } ) &&
	! isEulerianTrail( u , { 3 , 2 , 1 , 2 , 0 } ) &&
	! isEulerianCycle( u , { 3 , 2 , 1 , 0 , 2 } ) );
 ListGraph<> a( 3 , true );
 a.insertEdge( 0 , 1 );
 a.insertEdge( 1 , 2 );
 CHECK( eulerianTrail( a ) == std::vector< int >( { 0 , 1 , 2 } ) );
 CHECK( ! eulerianTrail( a , 1 ) && ! isEulerianTrail( a , { 2 , 1 , 0 } ) );

 // large: random closed walks glued into an Eulerian graph, then one edge
 // removed (an open trail between its ends); paths of 10^6 nodes
 for( bool d : { true , false } ) {
  const int n = 100000 , m = 1000000;
  EL E;
  int at = 0;
  while( int( E.size() ) < m - 1 ) {
   const int v = int( g() % unsigned( n ) );
   if( v != at ) {
    E.emplace_back( at , v );
    at = v;
    }
   }
  if( at != 0 )
   E.emplace_back( at , 0 );
  const StaticGraph<> h( n , E , d );
  const auto C = eulerianCycle( h );
  CHECK( C && C->size() == E.size() + 1 && isEulerianCycle( h , *C ) );
  const auto [ x , y ] = E[ E.size() / 2 ];
  E.erase( E.begin() + E.size() / 2 );
  const StaticGraph<> o( n , E , d );
  const auto T = eulerianTrail( o );
  CHECK( T && T->size() == E.size() + 1 && isEulerianTrail( o , *T ) &&
	 ! eulerianCycle( o ) );
  CHECK( ( T->front() == y && T->back() == x ) ||
	 ( ! d && T->front() == x && T->back() == y ) );
  EL P;
  const int N = 1000000;
  for( int v = 0 ; v + 1 < N ; ++v )
   P.emplace_back( v , v + 1 );
  const StaticGraph<> path( N , P , d );
  const auto pt = eulerianTrail( path , 0 );
  CHECK( pt && int( pt->size() ) == N && pt->back() == N - 1 );
  P.emplace_back( N - 1 , 0 );
  const StaticGraph<> ring( N , P , d );
  CHECK( eulerianCycle( ring ) &&
	 int( eulerianCycle( ring )->size() ) == N + 1 );
  }
 }

/// the size of a largest matching of the graph of n <= 20 nodes and the
/// adjacency masks N, by the dynamic programming on the subsets: the least
/// node of a set is left out or matched to a neighbour in the set
int matchingSize( int n , const std::vector< unsigned > & N )
{
 std::vector< int > f( std::size_t( 1 ) << n , 0 );
 for( unsigned S = 1 ; S < ( 1u << n ) ; ++S ) {
  const int u = std::countr_zero( S );
  const unsigned R = S & ~( 1u << u );
  int best = f[ R ];
  for( unsigned T = R & N[ u ] ; T ; T &= T - 1 )
   best = std::max( best , 1 + f[ R & ~( 1u << std::countr_zero( T ) ) ] );
  f[ S ] = best;
  }
 return( f[ ( 1u << n ) - 1 ] );
 }

/// the size of the matching of mates M on g, checked: symmetric, on edges,
/// NIL on the names that are not nodes
template< class G >
int checkedSize( const G & g , const std::vector< int > & M )
{
 CHECK( int( M.size() ) == g.bound() );
 int k = 0;
 for( int u = 0 ; u < g.bound() ; ++u ) {
  if( M[ u ] == NIL )
   continue;
  CHECK( g.hasNode( u ) && M[ M[ u ] ] == u );
  const auto A = g.adj( u );
  CHECK( std::find( A.begin() , A.end() , M[ u ] ) != A.end() );
  ++k;
  }
 return( k / 2 );
 }

// the maximum matching of a general graph by the blossoms of Edmonds
// against the dynamic programming on the subsets on every graph of up to 6
// nodes and on random ones of up to 16, against the branch and bound of
// branchAndBoundMatching() with unit weights, against the maximum flow on
// bipartite graphs of thousands of nodes; the graphs that make blossoms
// within blossoms (the complete graphs, the Petersen graph, chains of
// triangles and of pentagons), a deleted node, a path of 10^6 nodes,
// parallel edges, and an oriented graph refused
ADS_TEST( "blossom-matching" )
{
 auto check = [ & ]( const auto & h , int n ) {
  std::vector< unsigned > N( n , 0 );
  for( int u : h.V() )
   h.forEachAdj( u , [ & ]( int v ) { N[ u ] |= 1u << v; } );
  for( int u = 0 ; u < n ; ++u )
   if( ! h.hasNode( u ) )
    for( auto & x : N )
     x &= ~( 1u << u );
  CHECK( checkedSize( h , maxCardinalityMatching( h ) ) ==
	 matchingSize( n , N ) );
  };
 for( int n = 1 ; n <= 6 ; ++n ) {
  std::vector< std::pair< int , int > > P;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    P.emplace_back( u , v );
  for( unsigned mask = 0 ; mask < ( 1u << P.size() ) ; ++mask ) {
   ListGraph<> h( n , false );
   for( std::size_t i = 0 ; i < P.size() ; ++i )
    if( mask & ( 1u << i ) )
     h.insertEdge( P[ i ].first , P[ i ].second );
   check( h , n );
   }
  }
 std::mt19937 g( 23 );
 for( int it = 0 ; it < 4000 ; ++it ) {
  const int n = 7 + it % 10;
  const unsigned p = 1 + g() % 6;
  ListGraph<> h( n , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 10 < p )
     h.insertEdge( u , v );
  if( it % 7 == 0 )
   h.deleteNode( int( g() % unsigned( n ) ) );
  check( h , n );
  }
 for( int it = 0 ; it < 200 ; ++it ) {  // the branch and bound
  const int n = 20 + it % 11;
  std::vector< std::tuple< int , int , long long > > E;
  ListGraph<> h( n , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 8 == 0 ) {
     h.insertEdge( u , v );
     E.emplace_back( u , v , 1 );
     }
  CHECK( checkedSize( h , maxCardinalityMatching( h ) ) ==
	 branchAndBoundMatching( n , E ).first );
  }
 for( int it = 0 ; it < 6 ; ++it ) {          // bipartiteMatching()
  const int p = 2000 , q = 1500 + 300 * it;  // (an audit per edge added
					    // to the network of flows)
  std::vector< std::pair< int , int > > B , E;
  for( int k = 0 ; k < 2 * p + it * p / 2 ; ++k ) {
   const int i = int( g() % p ) , j = int( g() % unsigned( q ) );
   B.emplace_back( i , j );
   E.emplace_back( i , p + j );
   }
  const StaticGraph<> h( p + q , E , false );
  CHECK( checkedSize( h , maxCardinalityMatching( h ) ) ==
	 int( bipartiteMatching( p , q , B ).size() ) );
  }

 // blossoms within blossoms: the complete graphs, the Petersen graph,
 // chains of triangles and pentagons joined by an edge (a perfect matching
 // only through the blossoms), odd cycles sharing a node
 for( int n : { 2 , 3 , 9 , 64 , 101 } ) {
  ListGraph<> k( n , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    k.insertEdge( u , v );
  CHECK( checkedSize( k , maxCardinalityMatching( k ) ) == n / 2 );
  }
 ListGraph<> pet( 10 , false );
 for( int i = 0 ; i < 5 ; ++i ) {
  pet.insertEdge( i , ( i + 1 ) % 5 );
  pet.insertEdge( 5 + i , 5 + ( i + 2 ) % 5 );
  pet.insertEdge( i , 5 + i );
  }
 CHECK( checkedSize( pet , maxCardinalityMatching( pet ) ) == 5 );
 for( int c : { 3 , 5 } ) {
  const int K = 2000;
  std::vector< std::pair< int , int > > E;
  for( int b = 0 ; b < K ; ++b ) {
   for( int i = 0 ; i < c ; ++i )
    E.emplace_back( b * c + i , b * c + ( i + 1 ) % c );
   if( b % 2 == 1 )
    E.emplace_back( ( b - 1 ) * c + 1 , b * c + 1 );
   }
  const StaticGraph<> h( K * c , E , false );
  CHECK( checkedSize( h , maxCardinalityMatching( h ) ) == K * c / 2 );
  }
 ListGraph<> star( 9 , false );  // four triangles on node 0
 for( int t = 0 ; t < 4 ; ++t ) {
  star.insertEdge( 0 , 1 + 2 * t );
  star.insertEdge( 0 , 2 + 2 * t );
  star.insertEdge( 1 + 2 * t , 2 + 2 * t );
  }
 CHECK( checkedSize( star , maxCardinalityMatching( star ) ) == 4 );
 const int N = 1000000;
 std::vector< std::pair< int , int > > P;
 for( int v = 0 ; v + 1 < N ; ++v )
  P.emplace_back( v , v + 1 );
 P.emplace_back( 0 , 1 );  // parallel
 const StaticGraph<> path( N , P , false );
 CHECK( checkedSize( path , maxCardinalityMatching( path ) ) == N / 2 );
 CHECK( maxCardinalityMatching( ListGraph<>( 0 , false ) ).empty() );
 CHECK( maxCardinalityMatching( ListGraph<>( 3 , false ) ) ==
	std::vector< int >( 3 , NIL ) );
 CHECK_THROWS( maxCardinalityMatching( ListGraph<>( 3 , true ) ) ,
	       contract_violation );
 }

/// the largest ( size , weight ) of a matching of the edges E of n <= 16
/// nodes, by size first if card and by weight alone (edges of weight > 0)
/// otherwise, by the dynamic programming on the subsets
template< class W >
std::pair< int , W >
bestMatching( int n , const std::vector< std::tuple< int , int , W > > & E ,
	      bool card )
{
 std::vector< std::vector< std::pair< int , W > > > A( n );
 for( const auto & [ u , v , w ] : E )
  if( card || ( W( 0 ) < w ) ) {
   A[ std::min( u , v ) ].emplace_back( std::max( u , v ) , w );
   }
 auto better = [ & ]( const std::pair< int , W > & a ,
		      const std::pair< int , W > & b ) {
  return( card ? a > b : a.second > b.second );
  };
 std::vector< std::pair< int , W > > f( std::size_t( 1 ) << n ,
					{ 0 , W( 0 ) } );
 for( unsigned S = 1 ; S < ( 1u << n ) ; ++S ) {
  const int u = std::countr_zero( S );
  const unsigned R = S & ~( 1u << u );
  auto best = f[ R ];
  for( const auto & [ v , w ] : A[ u ] )
   if( R & ( 1u << v ) ) {
    const auto & g = f[ R & ~( 1u << v ) ];
    const std::pair< int , W > c( g.first + 1 , g.second + w );
    if( better( c , best ) )
     best = c;
    }
  f[ S ] = best;
  }
 return( f[ ( 1u << n ) - 1 ] );
 }

/// the ( size , weight ) of the matching M of the edges E, checked: no two
/// edges with an end in common, the places increasing
template< class W >
std::pair< int , W >
checkedMatching( int n , const std::vector< std::tuple< int , int , W > > & E ,
		 const std::pair< W , std::vector< std::size_t > > & M )
{
 std::vector< bool > end( n , false );
 W sum( 0 );
 CHECK( std::is_sorted( M.second.begin() , M.second.end() ) );
 for( std::size_t e : M.second ) {
  CHECK( e < E.size() );
  const auto & [ u , v , w ] = E[ e ];
  CHECK( ! end[ u ] && ! end[ v ] );
  end[ u ] = end[ v ] = true;
  sum += w;
  }
 CHECK( sum == M.first );
 return( std::pair( int( M.second.size() ) , sum ) );
 }

// the matching of largest weight, and of largest weight among those of
// largest size, of a general graph by the primal-dual blossoms of Edmonds,
// whose duals the audits check, against the dynamic programming on the
// subsets on every graph of up to 5 nodes with random weights (negative
// ones too) and on random graphs of up to 14 nodes with weights from
// narrow (many ties) to wide ranges, against the branch and bound, against
// the assignment on complete bipartite graphs, against the blossoms of
// maxCardinalityMatching() with unit weights on large graphs, on weights
// of type double, on parallel edges and on the edges of the contracts
ADS_TEST( "weighted-blossom" )
{
 using WE = std::vector< std::tuple< int , int , long long > >;
 std::mt19937 g( 29 );
 auto check = [ & ]( int n , const WE & E ) {
  const auto M = maxWeightMatching( n , E );
  CHECK( checkedMatching( n , E , M ).second ==
	 bestMatching( n , E , false ).second );
  const auto C = maxWeightMatching( n , E , true );
  CHECK( checkedMatching( n , E , C ) == bestMatching( n , E , true ) );
  };
 for( int n = 1 ; n <= 5 ; ++n ) {
  std::vector< std::pair< int , int > > P;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    P.emplace_back( u , v );
  for( unsigned mask = 0 ; mask < ( 1u << P.size() ) ; ++mask )
   for( int draw = 0 ; draw < 4 ; ++draw ) {
    WE E;
    for( std::size_t i = 0 ; i < P.size() ; ++i )
     if( mask & ( 1u << i ) )
      E.emplace_back( P[ i ].first , P[ i ].second ,
		      ( long long )( g() % 13 ) - 3 );
    check( n , E );
    }
  }
 for( int it = 0 ; it < 3000 ; ++it ) {
  const int n = 6 + it % 9;
  const long long range[] = { 2 , 5 , 100 , 1000000 };
  const long long R = range[ it % 4 ];
  WE E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 3 == 0 )
     E.emplace_back(
       u , v , 1 + ( long long )( g() % R ) - ( it % 8 == 0 ? R / 2 : 0 ) );
  if( it % 5 == 0 && ! E.empty() )  // parallel
   E.push_back( E[ g() % E.size() ] );
  check( n , E );
  if( n <= 10 )
   CHECK( maxWeightMatching( n , E ).first ==
	  branchAndBoundMatching( n , E ).first );
  }

 for( int it = 0 ; it < 300 ; ++it ) {  // the branch and bound
  const int n = 16 + it % 9;
  WE E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 6 == 0 )
     E.emplace_back( u , v , 1 + ( long long )( g() % ( it % 2 ? 9 : 999 ) ) );
  CHECK( checkedMatching( n , E , maxWeightMatching( n , E ) ).second ==
	 branchAndBoundMatching( n , E ).first );
  }

 // weights of type double and of type int, against the same programming
 for( int it = 0 ; it < 300 ; ++it ) {
  const int n = 4 + it % 9;
  std::vector< std::tuple< int , int , double > > D;
  std::vector< std::tuple< int , int , int > > I;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 2 == 0 ) {
     const int w = int( g() % 64 );
     D.emplace_back( u , v , w * 0.25 );  // exact in binary
     I.emplace_back( u , v , w );
     }
  CHECK( maxWeightMatching( n , D ).first ==
	 bestMatching( n , D , false ).second );
  CHECK( maxWeightMatching( n , I , true ).first ==
	 bestMatching( n , I , true ).second );
  }

 // complete bipartite graphs: the perfect matching of largest weight is
 // the assignment of least cost with the weights negated
 for( int k : { 1 , 2 , 7 , 40 , 150 } ) {
  WE E;
  std::vector< long long > C( std::size_t( k ) * std::size_t( k ) );
  for( int i = 0 ; i < k ; ++i )
   for( int j = 0 ; j < k ; ++j ) {
    const long long w = 1 + ( long long )( g() % 1000000 );
    E.emplace_back( i , k + j , w );
    C[ std::size_t( i ) * std::size_t( k ) + std::size_t( j ) ] = -w;
    }
  Assignment< long long > A( k , C );
  const auto M = maxWeightMatching( 2 * k , E , true );
  CHECK( int( M.second.size() ) == k && M.first == -A.total() );
  CHECK( maxWeightMatching( 2 * k , E ).first == M.first );
  }

 // large sparse graphs of unit weights: the size of the blossoms of
 // maxCardinalityMatching(), for both kinds of the weighted one
 for( int it = 0 ; it < 3 ; ++it ) {
  const int n = 1500 + 500 * it , m = 3 * n;
  WE E;
  std::vector< std::pair< int , int > > P;
  while( int( E.size() ) < m ) {
   const int a = int( g() % unsigned( n ) ) , b = int( g() % unsigned( n ) );
   if( a != b ) {
    E.emplace_back( a , b , 1 );
    P.emplace_back( a , b );
    }
   }
  const StaticGraph<> h( n , P , false );
  const int c = checkedSize( h , maxCardinalityMatching( h ) );
  CHECK( maxWeightMatching( n , E ).first == c &&
	 maxWeightMatching( n , E , true ).first == c );
  }

 // the edges: no node, no edge, light edges only (none taken, unless the
 // size comes first), a triangle, a path where the heaviest edge must be
 // left, the contracts
 CHECK( maxWeightMatching( 0 , WE{} ).first == 0 &&
	maxWeightMatching( 0 , WE{} , true ).second.empty() );
 CHECK( maxWeightMatching( 3 , WE{} ).second.empty() );
 const WE light = { { 0 , 1 , 0 } , { 2 , 3 , -4 } };
 CHECK( maxWeightMatching( 4 , light ).second.empty() );
 CHECK( maxWeightMatching( 4 , light , true ) ==
	std::pair( -4LL , std::vector< std::size_t >( { 0 , 1 } ) ) );
 const WE path = { { 0 , 1 , 5 } , { 1 , 2 , 8 } , { 2 , 3 , 5 } };
 CHECK( maxWeightMatching( 4 , path ) ==
	std::pair( 10LL , std::vector< std::size_t >( { 0 , 2 } ) ) );
 const WE tri = { { 0 , 1 , 6 } , { 1 , 2 , 7 } , { 0 , 2 , 5 } ,
		  { 2 , 3 , 1 } };
 CHECK( maxWeightMatching( 4 , tri ).first == 7 &&
	maxWeightMatching( 4 , tri , true ).first == 7 );
 CHECK_THROWS( maxWeightMatching( -1 , WE{} ) , contract_violation );
 CHECK_THROWS( maxWeightMatching( 2 , WE{ { 0 , 2 , 1 } } ) ,
	       contract_violation );
 CHECK_THROWS( maxWeightMatching( 2 , WE{ { 1 , 1 , 1 } } ) ,
	       contract_violation );
 }

/// the moves of g as ( tail , head , weight ), sorted
template< class G >
std::vector< std::tuple< int , int , long long > > arcsOf( const G & g )
{
 std::vector< std::tuple< int , int , long long > > X;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , auto w ) {
   X.emplace_back( u , v , ( long long )( w ) );
   } );
 std::sort( X.begin() , X.end() );
 return( X );
 }

/// the heads of the arcs in one of A and B and not in the other, or with
/// another weight
std::set< int >
changedHeads( const std::vector< std::tuple< int , int , long long > > & A ,
	      const std::vector< std::tuple< int , int , long long > > & B )
{
 std::vector< std::tuple< int , int , long long > > D;
 std::set_symmetric_difference( A.begin() , A.end() , B.begin() , B.end() ,
				std::back_inserter( D ) );
 std::set< int > H;
 for( const auto & [ u , v , w ] : D )
  H.insert( v );
 return( H );
 }

/// an observer of the changes of the costs of a grid of int costs
struct CostLog
{
 std::vector< mod::Changed< mod::Value , int , int > > E;
 int batches = 0;
 void on( const mod::Changed< mod::Value , int , int > & e ) {
  E.push_back( e );
  }
 void group_begin( void ) { ++batches; }
 void group_end( void ) { }
 };

// GridGraph::setCost(): the counts of the free cells and of the least cost
// and the orientation against a grid built from the same costs, after
// each change of random grids of both models of the costs, with and
// without a vector at the start, and the moves of every node; the events
// of a static and of a dynamic observer, one per change that changes the
// cost, coalesced per cell in a batch; no event and no member with
// NoObservers
ADS_TEST( "grid-graph-set-cost" )
{
 static_assert( std::is_empty_v< GridGraph< int , int >::Subject > );
 static_assert( std::same_as< GridGraph< int , int >::CellChanged ,
			      mod::Changed< mod::Value , int , int > > );
 std::mt19937 g( 23 );
 for( int t = 0 ; t < 40 ; ++t ) {
  const int R = 1 + int( g() % 7 ) , Cc = 1 + int( g() % 8 );
  const GridMoves m = t % 2 ? GridMoves::EIGHT : GridMoves::FOUR;
  const Corners k = Corners( t % 3 );
  const MoveCost model = ( t / 2 ) % 2 ? MoveCost::LEFT : MoveCost::ENTERED;
  const bool vec = t % 5 != 0;
  std::vector< int > cost( std::size_t( R ) * Cc , 1 );
  if( vec )
   for( auto & c : cost )
    c = int( g() % 4 );
  CostLog st;
  GridGraph< int , int , Static< CostLog > > G(
    R , Cc , vec ? cost : std::vector< int >() , m , k , 2 , 3 , model , st );
  std::size_t told = 0;
  for( int s = 0 ; s < 30 ; ++s ) {
   const int u = int( g() % std::size_t( R * Cc ) );
   const int c = int( g() % 4 );
   const int a = cost[ u ];
   G.setCost( u , c );
   cost[ u ] = c;
   if( a != c ) {
    ++told;
    CHECK( ( st.E.size() == told ) && ( st.E.back().h == u ) &&
	   ( st.E.back().from == a ) && ( st.E.back().to == c ) );
    }
   CHECK( st.E.size() == told );
   const GridGraph< int , int > F( R , Cc , cost , m , k , 2 , 3 , model );
   CHECK( ( G.n() == F.n() ) && ( G.minCost() == F.minCost() ) &&
	  ( G.directed() == F.directed() ) && ( G.cost( u ) == c ) );
   CHECK( arcsOf( G ) == arcsOf( F ) );
   }
  }
 // a grid with no vector takes one only for a cost other than 1
 GridGraph< int , int > Z( 3 , 3 );
 Z.setCost( 4 , 1 );
 CHECK( Z.n() == 9 && ! Z.directed() );
 Z.setCost( 4 , 0 );
 CHECK( Z.n() == 8 && Z.minCost() == 1 && ! Z.directed() );
 for( int u = 0 ; u < 9 ; ++u )
  Z.setCost( u , 0 );
 CHECK( Z.n() == 0 && Z.minCost() == 1 && ! Z.directed() );
 Z.setCost( 8 , 5 );
 CHECK( Z.n() == 1 && Z.minCost() == 5 && ! Z.directed() );
 // the last cell of the least cost raised: the least cost counted again
 GridGraph< int , int > Y( 1 , 3 , std::vector< int >{ 1 , 4 , 4 } );
 Y.setCost( 0 , 6 );
 CHECK( Y.minCost() == 4 && Y.directed() );
 Y.setCost( 0 , 4 );
 CHECK( Y.minCost() == 4 && ! Y.directed() );
 // a dynamic observer, and a batch: one event per cell, from the first
 // cost to the last one
 GridGraph< int , int , Dynamic<> > D( 2 , 3 , std::vector< int >( 6 , 1 ) );
 CostLog dy;
 const int id = D.attach( dy );
 D.setCost( 1 , 2 );
 CHECK( dy.E.size() == 1 && dy.E[ 0 ].direction() == mod::effect::up );
 {
  auto b = D.batch();
  D.setCost( 0 , 3 );
  D.setCost( 0 , 0 );
  D.setCost( 5 , 7 );
  D.setCost( 1 , 1 );
  CHECK( dy.E.size() == 1 && D.subject().depth() == 1 );
  }
 CHECK( dy.batches == 1 && dy.E.size() == 4 );
 std::map< int , std::pair< int , int > > net;
 for( std::size_t i = 1 ; i < dy.E.size() ; ++i )
  net[ dy.E[ i ].h ] = { dy.E[ i ].from , dy.E[ i ].to };
 CHECK( ( net[ 0 ] == std::pair< int , int >( 1 , 0 ) ) &&
	( net[ 5 ] == std::pair< int , int >( 1 , 7 ) ) &&
	( net[ 1 ] == std::pair< int , int >( 2 , 1 ) ) );
 D.detach( id );
 D.setCost( 2 , 9 );
 CHECK( dy.E.size() == 4 );
 CHECK_THROWS( D.setCost( 6 , 1 ) , contract_violation );
 CHECK_THROWS( D.setCost( -1 , 1 ) , contract_violation );
 }

// GridGraph::forEachAffected(): the heads that it gives, in increasing
// order, hold every head of an arc that a change created, removed or
// weighed differently, for one change and for a batch of changes read
// after it, on random grids of every rule of the corners, of both models of
// the costs and of four and eight moves; the cell alone under ENTERED and
// no head for a cell blocked before and after
ADS_TEST( "grid-graph-affected" )
{
 std::mt19937 g( 29 );
 std::size_t seen = 0;  // the heads changed, in all
 for( int t = 0 ; t < 120 ; ++t ) {
  const int R = 1 + int( g() % 6 ) , Cc = 1 + int( g() % 6 );
  const GridMoves m = t % 2 ? GridMoves::EIGHT : GridMoves::FOUR;
  const Corners k = Corners( ( t / 2 ) % 3 );
  const MoveCost model = ( t / 6 ) % 2 ? MoveCost::LEFT : MoveCost::ENTERED;
  std::vector< int > cost( std::size_t( R ) * Cc );
  for( auto & c : cost )
   c = int( g() % 4 );
  CostLog log;
  GridGraph< int , int , Static< CostLog > > G( R , Cc , cost , m , k , 2 , 3 ,
						model ,
					      log );
  for( int s = 0 ; s < 20 ; ++s ) {
   const auto before = arcsOf( G );
   log.E.clear();
   const bool many = s % 3 == 0;
   {
    auto b = G.batch();
    for( int j = 0 ; j < ( many ? 4 : 1 ) ; ++j )
     G.setCost( int( g() % std::size_t( R * Cc ) ) , int( g() % 4 ) );
    }
   std::set< int > H;
   for( const auto & e : log.E ) {
    std::vector< int > hs;
    G.forEachAffected( e.h , e.from , e.to ,
		       [ & ]( int v ) { hs.push_back( v ); } );
    CHECK( std::is_sorted( hs.begin() , hs.end() ) &&
	   std::adjacent_find( hs.begin() , hs.end() ) == hs.end() &&
	   hs.size() <= 9 );
    if( ( e.from <= 0 ) && ( e.to <= 0 ) )
     CHECK( hs.empty() );
    if( ( e.from > 0 ) && ( e.to > 0 ) && ( model == MoveCost::ENTERED ) )
     CHECK( hs == std::vector< int >{ e.h } );
    H.insert( hs.begin() , hs.end() );
    }
   const auto ch = changedHeads( before , arcsOf( G ) );
   CHECK( std::includes( H.begin() , H.end() , ch.begin() , ch.end() ) );
   seen += ch.size();
   }
  }
 CHECK( seen > 1000 );
 GridGraph< int , int > F( 2 , 2 );
 CHECK_THROWS( F.forEachAffected( 4 , 1 , 0 , []( int ) { } ) ,
	       contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_graph.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
