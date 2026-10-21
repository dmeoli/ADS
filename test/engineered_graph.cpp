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
 * counts. On random graphs with fixed seeds and on the edge cases (no
 * node, one node, no edge, parallel edges, unreachable nodes, ties, a
 * negative cycle), at the level 3 of the contracts.
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

#include "ADS/graph/ArrayGraph.h"
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
#include <queue>
#include <random>
#include <set>
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

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_graph.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
