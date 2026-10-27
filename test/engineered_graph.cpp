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
#include "ADS/graph/GraphBuilder.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/GridGraph.h"
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

#include <algorithm>
#include <climits>
#include <cmath>
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

namespace {

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
struct SetQueue {
 using Item = int;
 using Priority = std::pair< int , int >;
 using Handle = int;
 std::set< Priority > s;
 std::vector< Priority > key;
 explicit SetQueue( int n ) : key( std::size_t( n ) ) {}
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

/// std::queue with the names of the book
struct StdQueue {
 std::queue< int > q;
 bool empty( void ) const { return( q.empty() ); }
 void enqueue( int v ) { q.push( v ); }
 int dequeue( void ) { const int v = q.front(); q.pop(); return( v ); }
 };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

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
   for( int v : S.adj( u ) ) fw.emplace( u , v );
   for( int v : T.adj( u ) ) bw.emplace( v , u );
   }
  CHECK( fw == bw );
  const auto p = bfsOrder( S , 3 );
  CHECK( p[ 3 ] == 0 );
  const auto R = S.relabel( p );
  for( int u = 0 ; u < S.n() ; ++u )
   CHECK( R.degree( p[ u ] ) == S.degree( u ) );
  const StaticGraph< int > C( A );                  // from a graph
  for( int u = 0 ; u < S.n() ; ++u )
   CHECK( C.adj( u ) == S.adj( u ) );
  }
 const StaticGraph<> Z;
 CHECK( Z.n() == 0 && Z.m() == 0 && Z.V().empty() );
 CHECK( bfsOrder( Z ).empty() );
 const StaticGraph<> One( 1 , std::vector< std::pair< int , int > >{} );
 CHECK( One.n() == 1 && One.adj( 0 ).empty() );
 CHECK_THROWS( StaticGraph<>( 2 , std::vector< std::pair< int , int > >{
               { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( StaticGraph<>( 2 , std::vector< std::pair< int , int > >{
               { 0 , 2 } } ) , contract_violation );
 CHECK_THROWS( StaticGraph<>( -1 , std::vector< std::pair< int , int > >{} ) ,
               contract_violation );
 CHECK_THROWS( (void) One.adj( 1 ) , contract_violation );
 CHECK_THROWS( (void) One.relabel( { 1 } ) , contract_violation );
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
     if( in ) R.erase( { key[ id ] , id } );
     key[ id ] = k;
     R.emplace( k , id );
     }
    }
   else if( ! R.empty() ) {
    const auto e = H.pop();
    CHECK( e.key == R.begin()->first );     // a least key, ties any id
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
  CHECK_THROWS( (void) H.top() , contract_violation );
  CHECK_THROWS( (void) H.contains( n ) , contract_violation );
  H.push( 1 , INT_MIN );                       // the extremes of int
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
  const StaticGraph< int > S( n + 1 , E , true );   // n unreachable
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
  { HeapPriorityQueue< int , P , 2 > q( n + 1 ); same( q , S ); }
  { HeapPriorityQueue< int , P , 4 > q( n + 1 ); same( q , S ); }
  { FibonacciHeapPriorityQueue< int , P > q; same( q , S ); }
  { PairingHeap< int , P > q; same( q , S ); }
  { ListPriorityQueue< int , P , false > q; same( q , S ); }
  { IndexedDaryHeap< P , 2 > q( n + 1 ); same( q , S ); }
  { IndexedDaryHeap< P , 4 > q( n + 1 ); same( q , S ); }
  { SetQueue q( n + 1 ); same( q , S ); }
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
  { HeapPriorityQueue< int , P , 4 > q( n ); tree( q , U ); }
  { FibonacciHeapPriorityQueue< int , P > q; tree( q , U ); }
  { PairingHeap< int , P > q; tree( q , U ); }
  { IndexedDaryHeap< P , 2 > q( n ); tree( q , U ); }
  { SetQueue q( n ); tree( q , U ); }
  long long wp = 0 , wk = 0;
  for( int v = 0 ; v < n ; ++v )
   if( pref[ v ] != NIL ) {
    int w = INT_MAX;
    U.forEachEdge( v , [ & ]( int x , int c ) {
     if( x == pref[ v ] ) w = std::min( w , c ); } );
    wp += w;
    }
  std::vector< WeightedEdge< int > > W;
  for( const auto & [ u , v , w ] : SE ) W.push_back( { u , v , w } );
  for( const auto & e : kruskal( n , W ) ) wk += e.w;
  bool connected = true;                    // then Prim spans it all
  for( int d : distance( U , 0 ).first ) connected = connected && d != INF;
  if( connected )
   CHECK( wp == wk );
  MergeCounts m1 , m2 , m3;
  MFSet M( n );
  UnionFind F( n );
  DisjointSets< Union::RANK , Compression::FULL > D( n );
  const auto K1 = kruskal( n , W , M , 0 , &m1 );
  const auto K2 = kruskal( n , W , F , 0 , &m2 );
  const auto K3 = kruskal( n , W , D , 1001 , &m3 );   // counting sort
  CHECK( K1 == K2 && K1.size() == K3.size() );
  CHECK( m1.merges == m2.merges && m1.unions == m2.unions &&
         m1.unions == m3.unions );
  }
 // a negative cycle on a queue, and frontiers that are not empty
 const StaticGraph< int > C( 3 , Edges{ { 0 , 1 , 1 } , { 1 , 2 , -3 } ,
                                         { 2 , 1 , 1 } } , true );
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
 CHECK_THROWS( prim( StaticGraph< int >( 3 , Edges{ { 0 , 1 , 1 } } ,
                                         false ) , 0 , h ) ,
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
  c = ( g() % 4 == 0 ) ? 0 : (unsigned char)( 1 + g() % 9 );
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
 for( auto [ r , c ] : { std::pair( 0 , 0 ) , std::pair( 0 , 5 ) ,
                         std::pair( 5 , 0 ) } ) {
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
 const GridGraph< int , unsigned char > None( 3 , 4 ,
                              std::vector< unsigned char >( 12 , 0 ) );
 CHECK( None.n() == 0 && None.bound() == 12 && None.V().empty() );
 for( int u = 0 ; u < 12 ; ++u )
  CHECK( ! None.hasNode( u ) );
 CHECK( gridDistances( None , {} ) == V( 12 , INF ) );
 CHECK_THROWS( distance( None , 0 ) , contract_violation );
 // the moves of a corner, of a side and of the middle of a 3 x 3 grid
 const GridGraph< int > F4( 3 , 3 );
 const GridGraph< int > F8( 3 , 3 , GridMoves::EIGHT , Corners::NO_CUT , 2 ,
                            3 );
 CHECK( F4.adj( 0 ) == ( V{ 1 , 3 } ) && F4.adj( 4 ) == ( V{ 1 , 3 , 5 , 7 } )
        && F4.adj( 5 ) == ( V{ 2 , 4 , 8 } ) );
 CHECK( F8.adj( 0 ) == ( V{ 1 , 3 , 4 } ) &&
        F8.adj( 4 ) == ( V{ 0 , 1 , 2 , 3 , 5 , 6 , 7 , 8 } ) &&
        F8.adj( 8 ) == ( V{ 4 , 5 , 7 } ) );
 std::vector< std::pair< int , int > > e;
 F8.forEachEdge( 4 , [ & ]( int v , int w ) { e.emplace_back( v , w ); } );
 CHECK( e == ( std::vector< std::pair< int , int > >{ { 0 , 3 } , { 1 , 2 } ,
               { 2 , 3 } , { 3 , 2 } , { 5 , 2 } , { 6 , 3 } , { 7 , 2 } ,
               { 8 , 3 } } ) );
 CHECK( F8.node( 2 , 1 ) == 7 && F8.cell( 7 ) == GridCell( 2 , 1 ) );
 // the corners of a 2 x 2 grid with the cells 1 and 2 blocked, then with
 // the cell 1 alone
 auto two = [ & ]( std::vector< int > cost , Corners k ) {
  return( GridGraph< int >( 2 , 2 , std::move( cost ) , GridMoves::EIGHT ,
                            k , 1 , 1 ) );
  };
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::CUT ).adj( 0 ) == V{ 3 } );
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::NO_SQUEEZE ).adj( 0 ).empty() );
 CHECK( two( { 1 , 0 , 0 , 1 } , Corners::NO_CUT ).adj( 0 ).empty() );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::CUT ).adj( 0 ) == ( V{ 2 , 3 } ) );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::NO_SQUEEZE ).adj( 0 ) ==
        ( V{ 2 , 3 } ) );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::NO_CUT ).adj( 0 ) == V{ 2 } );
 CHECK( two( { 1 , 0 , 1 , 1 } , Corners::NO_CUT ).adj( 3 ) == V{ 2 } );
 // the weights: the cost of the cell entered times that of the move
 const GridGraph< int > Wt( 1 , 3 , std::vector< int >{ 1 , 5 , 2 } ,
                            GridMoves::FOUR , Corners::NO_CUT , 10 );
 CHECK( Wt.directed() && Wt.minCost() == 1 );
 e.clear();
 Wt.forEachEdge( 1 , [ & ]( int v , int w ) { e.emplace_back( v , w ); } );
 CHECK( e == ( std::vector< std::pair< int , int > >{ { 0 , 10 } ,
                                                      { 2 , 20 } } ) );
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
   const GridGraph< int , unsigned char > G( 9 , 11 , randomCosts( 9 , 11 ,
                                             g ) , mv , k , 2 , 3 );
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
 CHECK_THROWS( (void) Blocked.adj( 0 ) , contract_violation );
 CHECK_THROWS( gridDistances( Blocked , { 0 } ) , contract_violation );
 CHECK_THROWS( GridGraph< int >( 2 , 2 , GridMoves::EIGHT ) ,
               contract_violation );
 CHECK_THROWS( GridGraph< int >( 2 , 2 , std::vector< int >( 3 , 1 ) ) ,
               contract_violation );
 CHECK_THROWS( GridGraph< int >( -1 , 2 ) , contract_violation );
 CHECK_THROWS( GridGraph< double >( 2 , 2 , GridMoves::FOUR ,
                                    Corners::NO_CUT , 0.0 ) ,
               contract_violation );
 CHECK_THROWS( (void) F4.node( 3 , 0 ) , contract_violation );
 CHECK_THROWS( (void) F4.cell( 9 ) , contract_violation );
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
    const GridGraph< int , unsigned char > G( R , Cc , cost , mv , k , 5 ,
                                              7 );
    const ArrayGraph< int > A( G );
    const StaticGraph< int > S( G );
    CHECK( A.n() == G.bound() && S.n() == G.bound() );
    CHECK( S.directed() == G.directed() );
    // a few sources, each against the explicit graphs: the audits of
    // johnson() run a Bellman-Ford of O( n m ) each
    const std::vector< int > all = G.V();
    for( std::size_t i = 0 ; i < all.size() ;
         i += all.size() / 5 + 1 ) {
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
       CHECK( h <= w + gridHeuristic( G , v , s ) ); } );
      }
     }
    if( G.n() == 0 )
     continue;
    // several sources at once, against Grid.h on the matrix
    const std::vector< int > Src = { G.V().front() , G.V().back() };
    const auto D = gridDistances( G , Src );
    if( ( mv == GridMoves::FOUR ) || ( k == Corners::CUT ) ) {
     auto free = [ & ]( GridCell x ) {
      return( cost[ std::size_t( x.first ) * Cc + x.second ] > 0 ); };
     std::vector< GridCell > X;
     for( int s : Src )
      X.push_back( G.cell( s ) );
     const auto M = mv == GridMoves::FOUR ?
                    ADS::gridDistances( R , Cc , X , free ) :
                    ADS::gridDistances( R , Cc , X , free , EIGHT_MOVES );
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
                               []( int y ) { return( y != INF ); } ) ) );
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
  CHECK( std::fabs( d[ u ] - gridHeuristic( F , u , F.node( 6 , 8 ) ) )
         < 1e-9 );
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
 const int k = 46340;                       // k ( k + 1 ) < 2^31 - 1
 const GridGraph< long long > Q( k , k + 1 , GridMoves::EIGHT ,
                                 Corners::NO_CUT , 1 , 1 );
 const int last = Q.bound() - 1;
 CHECK( Q.bound() == k * ( k + 1 ) && Q.n() == Q.bound() );
 CHECK( Q.node( k - 1 , k ) == last );
 CHECK( Q.cell( last ) == GridCell( k - 1 , k ) );
 CHECK( Q.adj( last ) == ( V{ last - k - 2 , last - k - 1 , last - 1 } ) );
 CHECK( Q.adj( 0 ) == ( V{ 1 , k + 1 , k + 2 } ) );
 CHECK( Q.adj( k ) == ( V{ k - 1 , 2 * k , 2 * k + 1 } ) );
 CHECK( Q.adj( last - k ) == ( V{ last - 2 * k - 1 , last - 2 * k ,
                                  last - k + 1 } ) );
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
 CHECK( manhattan< long long >( GridCell( 0 , 0 ) ,
                                GridCell( INT_MAX , INT_MAX ) ) ==
        2LL * INT_MAX );
 CHECK( manhattan< long long >( GridCell( INT_MIN , 0 ) ,
                                GridCell( INT_MAX , 0 ) ) ==
        1LL * INT_MAX - INT_MIN );
 CHECK_THROWS( GridGraph<>( 65536 , 32768 ) , contract_violation );
 CHECK_THROWS( GridGraph<>( k + 1 , k + 1 ) , contract_violation );
 CHECK_THROWS( GridGraph<>( INT_MAX , 2 ) , contract_violation );
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
 B.addEdge( 0 , 2 , 3 );                         // parallel
 B.addEdge( 1 , 0 , 4 );
 CHECK( B.m() == 4 && B.edges()[ 2 ] == std::make_tuple( 0 , 2 , 3 ) );
 const auto BA = B.toArrayGraph();
 const auto BS = B.toStaticGraph();
 CHECK( BA.adj( 0 ) == ( V{ 2 , 1 , 2 } ) &&
        BS.adj( 0 ) == ( V{ 1 , 2 , 2 } ) );
 std::vector< std::pair< int , int > > w;
 BA.forEachEdge( 0 , [ & ]( int v , int x ) { w.emplace_back( v , x ); } );
 CHECK( w == ( std::vector< std::pair< int , int > >{ { 2 , 5 } , { 1 , 1 } ,
               { 2 , 3 } } ) );
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

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_graph.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
