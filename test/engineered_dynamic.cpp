/*--------------------------------------------------------------------------*/
/*----------------------- File engineered_dynamic.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the graphs that change, beyond the book: the events of
 * Observable on every model of graph, their batches and coalescing, the
 * masks, the static and the dynamic observers; the observers that keep
 * the shortest paths, the minimum spanning forest and the maximum flow,
 * on the boundaries of their certificates and on long random sequences of
 * events against the algorithms of the book run from scratch; the
 * union-find with rollback and the offline dynamic connectivity against a
 * union-find built again at each question. On the edge cases (no node,
 * one node, loops, parallel edges, disconnected graphs, zero and negative
 * weights, batches that cancel out, rollback to 0 and beyond, observers
 * attached midway, events with no one listening), at the level 3 of the
 * contracts.
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
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/mfset/RollbackUnionFind.h"
#include "ADS/mfset/UnionFind.h"

#include <cstdint>
#include <random>
#include <span>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using LG = ListGraph< long long >;
using Ev = GraphEvent< long long >;

/// an observer that records what it gets
struct Recorder {
 unsigned m = GraphEvents::ALL;
 std::vector< Ev > one;
 std::vector< std::vector< Ev > > batches;
 [[nodiscard]] unsigned mask( void ) const { return( m ); }
 void notify( const Ev & e ) { one.push_back( e ); }
 void notify( std::span< const Ev > s ) {
  batches.emplace_back( s.begin() , s.end() );
  }
 };

static_assert( GraphObserver< Recorder , long long > );
static_assert( Reoptimizable< ShortestPathsObserver< LG > , long long > );
static_assert( Reoptimizable< SpanningForestObserver< LG > , long long > );
static_assert( Reoptimizable< MaxFlowObserver< LG > , long long > );

/// the total weight of a forest
long long total( const std::vector< WeightedEdge< long long > > & F )
{
 long long s = 0;
 for( const auto & e : F )
  s += e.w;
 return( s );
 }

/// the maximum flow from s to t in g by a network built from scratch
long long freshFlow( const LG & g , int s , int t )
{
 FlowNetwork N( g.bound() );
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , long long w ) {
   if( g.directed() || ( u < v ) ) {
    N.addEdge( u , v , w );
    if( ! g.directed() )
     N.addEdge( v , u , w );
    } } );
 return( N.maxFlow( s , t ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EVENTS -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "observable-events" )
{
 // no one listening: no event is built, the graph changes all the same
 {
  LG g( 3 );
  Observable< LG > og( g );
  og.insertEdge( 0 , 1 , 5 );
  og.setWeight( 0 , 1 , 7 );
  og.deleteEdge( 0 , 1 );
  og.insertNode();
  og.deleteNode( 3 );
  CHECK( og.built() == 0 );
  CHECK( ! g.hasEdge( 0 , 1 ) && ( g.n() == 3 ) );
  CHECK( ! og.setWeight( 1 , 2 , 3 ) );   // no such edge
  }
 // an oriented graph, a static observer and a dynamic one with a mask
 {
  LG g( 3 );
  Recorder all , w;
  w.m = GraphEvents::WEIGHT;
  Observable< LG , Recorder > og( g , all );
  CHECK( og.listening() && ( og.mask() == GraphEvents::ALL ) );
  og.insertEdge( 0 , 1 , 5 );
  const int id = og.attach( w );          // attached midway
  og.insertEdge( 0 , 1 , 6 );             // only the weight changes
  og.insertEdge( 2 , 1 , 1 );
  og.setWeight( 2 , 1 , 1 );              // the same weight, still sent
  og.deleteEdge( 0 , 2 );                 // not there: nothing
  CHECK( all.one.size() == 4 );
  CHECK( all.one[ 0 ] == Ev( EdgeInserted< long long >{ 0 , 1 , 5 } ) );
  CHECK( all.one[ 1 ] == Ev( WeightChanged< long long >{ 0 , 1 , 5 , 6 } ) );
  CHECK( all.one[ 2 ] == Ev( EdgeInserted< long long >{ 2 , 1 , 1 } ) );
  CHECK( w.one.size() == 2 );
  CHECK( w.one[ 0 ] == Ev( WeightChanged< long long >{ 0 , 1 , 5 , 6 } ) );
  // deleteNode(): the edges out of and into the node first
  all.one.clear();
  og.deleteNode( 1 );
  CHECK( all.one.size() == 3 );
  CHECK( std::holds_alternative< EdgeDeleted< long long > >( all.one[ 0 ] ) );
  CHECK( std::holds_alternative< EdgeDeleted< long long > >( all.one[ 1 ] ) );
  CHECK( all.one[ 2 ] == Ev( NodeDeleted{ 1 } ) );
  og.detach( id );
  CHECK_THROWS( og.detach( id ) , contract_violation );
  const std::size_t before = w.one.size();
  og.insertEdge( 0 , 2 , 3 );
  og.setWeight( 0 , 2 , 4 );
  CHECK( w.one.size() == before );        // detached
  CHECK( og.insertNode() == 3 );
  CHECK( all.one.back() == Ev( NodeInserted{ 3 } ) );
  }
 // an unoriented graph: the ends in increasing order
 {
  LG g( 4 , false );
  Recorder r;
  Observable< LG , Recorder > og( g , r );
  og.insertEdge( 3 , 1 , 2 );
  og.setWeight( 1 , 3 , 0 );              // a zero weight
  og.deleteEdge( 3 , 1 );
  CHECK( r.one[ 0 ] == Ev( EdgeInserted< long long >{ 1 , 3 , 2 } ) );
  CHECK( r.one[ 1 ] == Ev( WeightChanged< long long >{ 1 , 3 , 2 , 0 } ) );
  CHECK( r.one[ 2 ] == Ev( EdgeDeleted< long long >{ 1 , 3 , 0 } ) );
  CHECK_THROWS( og.insertEdge( 2 , 2 , 1 ) , contract_violation );   // loop
  }
 // the models with fixed edges: only the weights change
 {
  ArrayGraph< long long > a( 3 , { { 0 , 1 , 4 } , { 1 , 2 , 5 } } , false );
  Recorder r;
  Observable< ArrayGraph< long long > , Recorder > oa( a , r );
  CHECK( oa.setWeight( 2 , 1 , -3 ) );    // a negative weight
  CHECK( ( a.weight( 1 , 2 ) == -3 ) && ( a.weight( 2 , 1 ) == -3 ) );
  CHECK( ! oa.setWeight( 0 , 2 , 1 ) && ( r.one.size() == 1 ) );
  CHECK( r.one[ 0 ] == Ev( WeightChanged< long long >{ 1 , 2 , 5 , -3 } ) );
  // parallel arcs in StaticGraph: the first one changes
  StaticGraph< long long > s( 3 , { { 0 , 1 , 4 } , { 0 , 1 , 9 } ,
                                    { 1 , 2 , 1 } } , true );
  Recorder q;
  Observable< StaticGraph< long long > , Recorder > os( s , q );
  CHECK( os.setWeight( 0 , 1 , 7 ) && ( s.weight( 0 , 1 ) == 7 ) );
  CHECK( ! os.setWeight( 1 , 0 , 7 ) && ( q.one.size() == 1 ) );
  CHECK( ( s.out( 0 )[ 0 ].w == 7 ) && ( s.out( 0 )[ 1 ].w == 9 ) );
  // MatrixGraph, dynamic
  MatrixGraph< long long > mg( 2 );
  Recorder t;
  Observable< MatrixGraph< long long > , Recorder > om( mg , t );
  om.insertEdge( 0 , 1 , 1 );
  CHECK( om.setWeight( 0 , 1 , 2 ) && ( t.one.size() == 2 ) );
  // the empty graph
  LG e( 0 );
  Observable< LG > oe( e );
  CHECK( ( oe.n() == 0 ) && ( oe.insertNode() == 0 ) );
  }
 }

ADS_TEST( "observable-batch" )
{
 LG g( 4 );
 g.insertEdge( 0 , 1 , 1 );
 g.insertEdge( 1 , 2 , 2 );
 Recorder all , ins;
 ins.m = GraphEvents::EDGE_INSERT | GraphEvents::NODE_INSERT;
 Observable< LG , Recorder > og( g , all );
 og.attach( ins );
 {
  auto b = og.batch();
  og.insertEdge( 2 , 3 , 5 );             // inserted and deleted: nothing
  og.deleteEdge( 2 , 3 );
  og.setWeight( 0 , 1 , 3 );              // three changes: the last one
  og.setWeight( 0 , 1 , 4 );
  og.setWeight( 0 , 1 , 1 );              // back to 1: nothing
  og.deleteEdge( 1 , 2 );                 // deleted and inserted again
  og.insertEdge( 1 , 2 , 8 );
  {
   auto inner = og.batch();               // nested: the outer one sends
   CHECK( og.depth() == 2 );
   og.insertEdge( 3 , 0 , 6 );
   }
  CHECK( all.batches.empty() && all.one.empty() );
  const int x = og.insertNode();          // a node that vanishes
  og.insertEdge( x , 0 , 1 );
  og.deleteNode( x );
  CHECK_THROWS( og.attach( all ) , contract_violation );   // channel open
  }
 CHECK( og.depth() == 0 );
 CHECK( all.batches.size() == 1 );
 const auto & B = all.batches[ 0 ];
 CHECK( B.size() == 2 );
 CHECK( B[ 0 ] == Ev( WeightChanged< long long >{ 1 , 2 , 2 , 8 } ) );
 CHECK( B[ 1 ] == Ev( EdgeInserted< long long >{ 3 , 0 , 6 } ) );
 CHECK( ins.batches.size() == 1 && ( ins.batches[ 0 ].size() == 1 ) );
 CHECK( ins.batches[ 0 ][ 0 ] == B[ 1 ] );
 // a batch that cancels out sends nothing, an empty one too
 {
  auto b = og.batch();
  og.setWeight( 0 , 1 , 9 );
  og.setWeight( 0 , 1 , 1 );
  b.close();
  b.close();                              // idempotent
  }
 { auto b = og.batch(); }
 CHECK( all.batches.size() == 1 );
 // a node deleted in a batch: its edges first, then the node
 {
  auto b = og.batch();
  og.deleteNode( 3 );
  }
 const auto & D = all.batches.back();
 CHECK( D.back() == Ev( NodeDeleted{ 3 } ) );
 CHECK( std::holds_alternative< EdgeDeleted< long long > >( D[ 0 ] ) );
 // coalesce() on its own: weights with no == never cancel
 struct NoEq { int x; };
 std::vector< GraphEvent< NoEq > > E = {
  WeightChanged< NoEq >{ 0 , 1 , { 1 } , { 2 } } ,
  WeightChanged< NoEq >{ 0 , 1 , { 2 } , { 1 } } };
 CHECK( coalesce< NoEq >( E ).size() == 1 );
 CHECK( coalesce< long long >( std::span< const Ev >() ).empty() );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OBSERVERS -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "reopt-shortest-paths" )
{
 // one node
 {
  LG g( 1 );
  ShortestPathsObserver< LG > sp( g , 0 );
  Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
  CHECK( ( sp.distance( 0 ) == 0 ) && ( sp.outcome() == Reopt::SCRATCH ) );
  sp.solve();
  CHECK( sp.outcome() == Reopt::SCRATCH );     // nothing to do
  }
 // the boundaries of the reduced costs: d = ( 0 , 2 , 5 )
 LG g( 3 );
 g.insertEdge( 0 , 1 , 2 );
 g.insertEdge( 1 , 2 , 3 );
 g.insertEdge( 0 , 2 , 9 );
 ShortestPathsObserver< LG > sp( g , 0 );
 sp.setRecomputeRatio( 10 );
 Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
 CHECK( sp.distance( 2 ) == 5 );
 og.setWeight( 0 , 2 , 5 );               // reduced cost 0: still optimal
 CHECK( ( sp.distance( 2 ) == 5 ) && ( sp.outcome() == Reopt::CERTIFIED ) );
 og.setWeight( 0 , 2 , 4 );               // one below: repaired
 CHECK( ( sp.distance( 2 ) == 4 ) && ( sp.outcome() == Reopt::WARM ) );
 og.setWeight( 1 , 2 , 1 );               // off the tree, lower: repaired
 CHECK( ( sp.distance( 2 ) == 3 ) && ( sp.outcome() == Reopt::WARM ) );
 og.setWeight( 0 , 1 , 0 );               // a tree edge lower, to zero
 CHECK( ( sp.distance( 2 ) == 1 ) && ( sp.outcome() == Reopt::WARM ) );
 og.setWeight( 0 , 2 , 7 );               // off the tree, higher
 sp.solve();
 CHECK( sp.outcome() == Reopt::CERTIFIED );
 og.setWeight( 1 , 2 , 6 );               // a tree edge higher: from scratch
 CHECK( ( sp.distance( 2 ) == 6 ) && ( sp.outcome() == Reopt::SCRATCH ) );
 og.deleteEdge( 0 , 1 );                  // a tree edge deleted
 CHECK( ( sp.distance( 1 ) == infinity< long long >() ) &&
        ( sp.outcome() == Reopt::SCRATCH ) );
 const int x = og.insertNode();           // a new node, unreachable
 CHECK( ( sp.distance( x ) == infinity< long long >() ) &&
        ( sp.outcome() == Reopt::CERTIFIED ) );
 og.insertEdge( x , 0 , 1 );              // from an unreached node
 sp.solve();
 CHECK( sp.outcome() == Reopt::CERTIFIED );
 og.deleteNode( x );
 sp.solve();
 CHECK( sp.outcome() == Reopt::CERTIFIED );
 // negative weights, no negative cycle, then a negative cycle closed
 {
  LG h( 3 );
  h.insertEdge( 0 , 1 , 4 );
  h.insertEdge( 1 , 2 , -2 );
  ShortestPathsObserver< LG > q( h , 0 , Frontier::QUEUE );
  q.setRecomputeRatio( 10 );
  Observable< LG > oh( h );
  oh.attach( q );
  CHECK( q.distance( 2 ) == 2 );
  oh.insertEdge( 2 , 1 , 1 );             // the cycle 1 2 1 weighs -1
  CHECK( q.tree().negativeCycle && ( q.outcome() == Reopt::SCRATCH ) );
  oh.deleteEdge( 2 , 1 );
  CHECK( ! q.tree().negativeCycle && ( q.distance( 2 ) == 2 ) );
  }
 // a random sequence of events against Johnson from scratch, oriented and
 // not, with batches
 for( bool directed : { true , false } ) {
  std::mt19937 rng( directed ? 7 : 8 );
  LG h( 30 , directed );
  ShortestPathsObserver< LG > q( h , 0 );
  q.setRecomputeRatio( 0.5 );
  Observable< LG , ShortestPathsObserver< LG > > oh( h , q );
  long long seen[ 3 ] = { 0 , 0 , 0 };
  for( int step = 0 ; step < 3000 ; ++step ) {
   const auto V = h.V();
   const int u = V[ rng() % V.size() ] , v = V[ rng() % V.size() ];
   const int k = int( rng() % 10 );
   auto b = oh.batch();
   if( ( k < 5 ) && ( u != v ) )
    oh.insertEdge( u , v , static_cast< long long >( rng() % 20 ) );
   else if( ( k < 8 ) && ( u != v ) )
    oh.deleteEdge( u , v );
   else if( k == 8 )
    oh.insertNode();
   else if( ( k == 9 ) && ( u != 0 ) && ( h.n() > 20 ) )
    oh.deleteNode( u );
   if( rng() % 3 == 0 ) {
    b.close();
    q.solve();
    ++seen[ q.outcome() ];
    const auto R = shortestPaths( h , 0 , Frontier::HEAP );
    for( int x : h.V() )
     CHECK( q.distance( x ) == R.d[ x ] );
    }
   }
  CHECK( ( seen[ 0 ] > 0 ) && ( seen[ 1 ] > 0 ) && ( seen[ 2 ] > 0 ) );
  }
 }

ADS_TEST( "reopt-spanning-forest" )
{
 CHECK_THROWS( SpanningForestObserver< LG >( LG( 2 , true ) ) ,
               contract_violation );
 // the empty graph and a single node
 {
  LG g( 0 , false );
  SpanningForestObserver< LG > f( g );
  CHECK( f.forest().empty() && ( f.weight() == 0 ) );
  Observable< LG , SpanningForestObserver< LG > > og( g , f );
  og.insertNode();
  CHECK( f.forest().empty() );
  }
 // a path 0 - 1 - 2 of weights 2 and 5, the edge [ 0 , 2 ] of weight 7,
 // and the node 3 apart
 LG g( 4 , false );
 g.insertEdge( 0 , 1 , 2 );
 g.insertEdge( 1 , 2 , 5 );
 g.insertEdge( 0 , 2 , 7 );
 SpanningForestObserver< LG > f( g );
 f.setRecomputeRatio( 10 );
 Observable< LG , SpanningForestObserver< LG > > og( g , f );
 CHECK( f.weight() == 7 );
 og.setWeight( 0 , 2 , 5 );               // as the maximum of its path
 CHECK( ( f.weight() == 7 ) && ( f.outcome() == Reopt::CERTIFIED ) );
 og.setWeight( 0 , 2 , 4 );               // below it: swapped in
 CHECK( ( f.weight() == 6 ) && ( f.outcome() == Reopt::WARM ) &&
        f.inForest( 2 , 0 ) && ! f.inForest( 1 , 2 ) );
 og.setWeight( 0 , 2 , 5 );               // a tree edge up to its
 CHECK( ( f.weight() == 7 ) &&            // replacement: kept
        ( f.outcome() == Reopt::CERTIFIED ) && f.inForest( 0 , 2 ) );
 og.setWeight( 0 , 2 , 6 );               // beyond it: swapped out
 CHECK( ( f.weight() == 7 ) && ( f.outcome() == Reopt::WARM ) &&
        f.inForest( 1 , 2 ) );
 og.setWeight( 1 , 2 , -1 );              // a tree edge lower, negative
 CHECK( ( f.weight() == 1 ) && ( f.outcome() == Reopt::CERTIFIED ) );
 og.insertEdge( 2 , 3 , 0 );              // joins the node apart
 CHECK( ( f.weight() == 1 ) && ( f.outcome() == Reopt::WARM ) );
 og.deleteEdge( 2 , 3 );                  // no replacement: it splits
 CHECK( ( f.weight() == 1 ) && ( f.outcome() == Reopt::CERTIFIED ) &&
        ( f.forest().size() == 2 ) );
 og.deleteEdge( 0 , 1 );                  // replaced by [ 0 , 2 ]
 CHECK( ( f.weight() == 5 ) && ( f.outcome() == Reopt::WARM ) );
 {
  auto b = og.batch();                    // two edges of F up: scratch
  og.setWeight( 0 , 2 , 50 );
  og.setWeight( 1 , 2 , 50 );
  og.insertEdge( 0 , 1 , 1 );
  }
 CHECK( ( f.weight() == 51 ) && ( f.outcome() == Reopt::SCRATCH ) );
 // random sequences against Kruskal, with ties, batches and nodes
 for( unsigned seed : { 1u , 2u , 3u } ) {
  std::mt19937 rng( seed );
  LG h( 25 , false );
  SpanningForestObserver< LG > q( h );
  q.setRecomputeRatio( seed == 3 ? 0 : 0.5 );   // 0: always scratch
  Observable< LG > oh( h );
  oh.attach( q );
  long long seen[ 3 ] = { 0 , 0 , 0 };
  for( int step = 0 ; step < 1500 ; ++step ) {
   const auto V = h.V();
   const int u = V[ rng() % V.size() ] , v = V[ rng() % V.size() ];
   const int k = int( rng() % 12 );
   auto b = oh.batch();
   const int many = rng() % 4 == 0 ? 3 : 1;
   for( int j = 0 ; j < many ; ++j ) {
    if( ( k < 6 ) && ( u != v ) )
     oh.insertEdge( u , v , static_cast< long long >( rng() % 10 ) - 2 );
    else if( ( k < 10 ) && ( u != v ) )
     oh.deleteEdge( u , v );
    else if( k == 10 )
     oh.insertNode();
    else if( ( k == 11 ) && ( h.n() > 15 ) ) {
     oh.deleteNode( u );
     break;
     }
    }
   b.close();
   if( rng() % 2 == 0 ) {
    q.solve();
    ++seen[ q.outcome() ];
    const auto K = kruskal( h );
    CHECK( ( q.weight() == total( K ) ) &&
           ( q.forest().size() == K.size() ) );
    }
   }
  if( seed == 3 )
   CHECK( ( seen[ 1 ] == 0 ) && ( seen[ 0 ] > 0 ) );
  else
   CHECK( ( seen[ 0 ] > 0 ) && ( seen[ 1 ] > 0 ) && ( seen[ 2 ] > 0 ) );
  }
 }

ADS_TEST( "reopt-max-flow" )
{
 // s = 0, t = 3; 0 -> 1 -> 3 of capacities 4 and 2, 0 -> 2 -> 3 of 1, 5
 LG g( 4 );
 g.insertEdge( 0 , 1 , 4 );
 g.insertEdge( 1 , 3 , 2 );
 g.insertEdge( 0 , 2 , 1 );
 g.insertEdge( 2 , 3 , 5 );
 MaxFlowObserver< LG > mf( g , 0 , 3 );
 mf.setRecomputeRatio( 10 );
 Observable< LG , MaxFlowObserver< LG > > og( g , mf );
 CHECK( mf.value() == 3 );
 og.setWeight( 0 , 1 , 2 );               // down to its flow: kept
 CHECK( ( mf.value() == 3 ) && ( mf.outcome() == Reopt::CERTIFIED ) );
 og.setWeight( 2 , 3 , 1 );               // down to its flow, 1
 CHECK( ( mf.value() == 3 ) && ( mf.outcome() == Reopt::CERTIFIED ) );
 og.setWeight( 1 , 3 , 1 );               // below its flow: scratch
 CHECK( ( mf.value() == 2 ) && ( mf.outcome() == Reopt::SCRATCH ) );
 const auto S = mf.minCut();
 og.setWeight( 1 , 3 , 9 );               // a cut arc up: augmented
 CHECK( ( mf.value() == 3 ) && ( mf.outcome() == Reopt::WARM ) );
 og.insertEdge( 3 , 0 , 7 );              // from the sink: kept
 CHECK( ( mf.value() == 3 ) && ( mf.outcome() == Reopt::CERTIFIED ) );
 og.setWeight( 3 , 0 , 0 );               // a zero capacity
 og.deleteEdge( 3 , 0 );
 CHECK( ( mf.value() == 3 ) && ( mf.outcome() == Reopt::CERTIFIED ) );
 const int x = og.insertNode();           // a new path through x
 og.insertEdge( 0 , x , 5 );
 og.insertEdge( x , 3 , 5 );
 CHECK( ( mf.value() == 8 ) && ( mf.outcome() == Reopt::WARM ) );
 ( void ) S;
 og.insertEdge( 0 , 1 , -1 );             // a negative capacity, seen
 CHECK_THROWS( mf.solve() , contract_violation );   // at the query
 // a graph with no path from s to t
 {
  LG h( 2 );
  MaxFlowObserver< LG > q( h , 0 , 1 );
  CHECK( ( q.value() == 0 ) && q.minCut()[ 0 ] && ! q.minCut()[ 1 ] );
  CHECK_THROWS( MaxFlowObserver< LG >( h , 0 , 0 ) , contract_violation );
  }
 // random sequences against a network from scratch, oriented and not
 for( bool directed : { true , false } ) {
  std::mt19937 rng( directed ? 11 : 12 );
  const int n = 12;
  LG h( n , directed );
  MaxFlowObserver< LG > q( h , 0 , n - 1 );
  q.setRecomputeRatio( 0.5 );
  Observable< LG , MaxFlowObserver< LG > > oh( h , q );
  long long seen[ 3 ] = { 0 , 0 , 0 };
  for( int step = 0 ; step < 3000 ; ++step ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u == v )
    continue;
   if( rng() % 3 )
    oh.insertEdge( u , v , static_cast< long long >( rng() % 10 ) );
   else
    oh.deleteEdge( u , v );
   if( rng() % 2 ) {
    q.solve();
    ++seen[ q.outcome() ];
    CHECK( q.value() == freshFlow( h , 0 , n - 1 ) );
    }
   }
  CHECK( ( seen[ 0 ] > 0 ) && ( seen[ 1 ] > 0 ) && ( seen[ 2 ] > 0 ) );
  }
 }

ADS_TEST( "reopt-lazy" )
{
 LG g( 5 );
 for( int u = 0 ; u + 1 < 5 ; ++u )
  g.insertEdge( u , u + 1 , 1 );
 Observable< LG > og( g );
 og.insertEdge( 0 , 4 , 9 );              // no one listens yet
 CHECK( og.built() == 0 );
 ShortestPathsObserver< LG > sp( g , 0 );
 og.attach( sp );                         // attached midway
 CHECK( ( sp.distance( 4 ) == 4 ) && ( sp.outcome() == Reopt::SCRATCH ) );
 // the queue waits for the next query, then coalesces
 og.setWeight( 0 , 4 , 1 );
 og.setWeight( 0 , 4 , 9 );
 CHECK( sp.pending() == 2 );
 CHECK( ( sp.distance( 4 ) == 4 ) && ( sp.outcome() == Reopt::CERTIFIED ) );
 CHECK( sp.pending() == 0 );
 // the threshold: 2 events on 5 edges, from scratch at a ratio below 2/5
 sp.setRecomputeRatio( 0.3 );
 og.setWeight( 0 , 4 , 2 );
 og.setWeight( 3 , 4 , 5 );
 CHECK( ( sp.distance( 4 ) == 2 ) && ( sp.outcome() == Reopt::SCRATCH ) );
 sp.setRecomputeRatio( 0.5 );
 og.setWeight( 0 , 4 , 1 );               // the tree edge lower
 og.setWeight( 3 , 4 , 7 );               // and one off the tree higher
 CHECK( ( sp.distance( 4 ) == 1 ) && ( sp.outcome() == Reopt::WARM ) );
 CHECK( sp.count( Reopt::SCRATCH ) == 2 );
 CHECK_THROWS( sp.setRecomputeRatio( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE ROLLBACK -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "rollback-union-find" )
{
 RollbackUnionFind U( 0 );
 CHECK( ( U.count() == 0 ) && ( U.checkpoint() == 0 ) );
 U.rollback( 0 );
 CHECK_THROWS( U.rollback( 1 ) , contract_violation );   // beyond
 CHECK_THROWS( U.undo() , contract_violation );
 U.reset( 1 );
 CHECK( ! U.merge( 0 , 0 ) && ( U.checkpoint() == 1 ) );   // a loop
 U.undo();
 U.reset( 6 );
 const int t0 = U.checkpoint();
 CHECK( U.merge( 0 , 1 ) && U.merge( 2 , 3 ) );
 const int t1 = U.checkpoint();
 CHECK( U.merge( 1 , 3 ) && ! U.merge( 0 , 2 ) );
 CHECK( ( U.count() == 3 ) && ( U.size( 2 ) == 4 ) && U.same( 0 , 3 ) );
 U.rollback( t1 );
 CHECK( ( U.count() == 4 ) && ! U.same( 0 , 3 ) && ( U.size( 0 ) == 2 ) );
 U.undo( 2 );
 CHECK( ( U.count() == 6 ) && ( U.checkpoint() == t0 ) );
 CHECK_THROWS( U.rollback( -1 ) , contract_violation );
 CHECK_THROWS( U.find( 6 ) , contract_violation );
 // random merges and rollbacks against a union-find built again
 std::mt19937 rng( 5 );
 const int n = 200;
 U.reset( n );
 std::vector< std::pair< int , int > > done;
 for( int step = 0 ; step < 5000 ; ++step ) {
  if( ( rng() % 4 == 0 ) && ! done.empty() ) {
   const int t = int( rng() % ( done.size() + 1 ) );
   U.rollback( t );
   done.resize( t );
   }
  else {
   const int x = int( rng() % n ) , y = int( rng() % n );
   U.merge( x , y );
   done.emplace_back( x , y );
   }
  if( step % 50 == 0 ) {
   UnionFind R( n );
   for( const auto & [ x , y ] : done )
    R.merge( x , y );
   CHECK( R.count() == U.count() );
   for( int x = 0 ; x < n ; ++x )
    CHECK( R.same( x , 0 ) == U.same( x , 0 ) );
   }
  // the rank bounds the height: a find in at most log2( n ) steps
  }
 }

ADS_TEST( "offline-connectivity" )
{
 using Op = ConnectivityOp;
 CHECK( offlineConnectivity( 0 , {} ).empty() );
 CHECK( offlineConnectivity( 3 , { { Op::INSERT , 0 , 1 } } ).empty() );
 CHECK( offlineConnectivity( 1 , { { Op::COMPONENTS } } ) ==
        std::vector< int >{ 1 } );
 // a loop, parallel edges, a deletion of one of them
 const std::vector< Op > ops = {
  { Op::INSERT , 0 , 0 } , { Op::COMPONENTS } ,
  { Op::INSERT , 0 , 1 } , { Op::INSERT , 1 , 0 } ,
  { Op::CONNECTED , 0 , 1 } , { Op::DELETE , 0 , 1 } ,
  { Op::CONNECTED , 1 , 0 } , { Op::DELETE , 1 , 0 } ,
  { Op::CONNECTED , 0 , 1 } , { Op::COMPONENTS } ,
  { Op::CONNECTED , 2 , 2 } };
 CHECK( offlineConnectivity( 3 , ops ) ==
        ( std::vector< int >{ 3 , 1 , 1 , 0 , 3 , 1 } ) );
 CHECK_THROWS( offlineConnectivity( 2 , { { Op::DELETE , 0 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( offlineConnectivity( 2 , { { Op::INSERT , 0 , 2 } } ) ,
               contract_violation );
 CHECK_THROWS( offlineConnectivity( -1 , {} ) , contract_violation );
 // random sequences against a union-find built at each question
 for( unsigned seed : { 1u , 2u } ) {
  std::mt19937 rng( seed );
  const int n = seed == 1 ? 30 : 8;
  std::vector< Op > R;
  std::vector< std::pair< int , int > > alive;
  for( int step = 0 ; step < 3000 ; ++step ) {
   const int k = int( rng() % 5 );
   if( k < 2 ) {
    const int u = int( rng() % n ) , v = int( rng() % n );
    R.push_back( { Op::INSERT , u , v } );
    alive.emplace_back( u , v );
    }
   else if( ( k == 2 ) && ! alive.empty() ) {
    const std::size_t i = rng() % alive.size();
    R.push_back( { Op::DELETE , alive[ i ].second , alive[ i ].first } );
    alive.erase( alive.begin() + std::ptrdiff_t( i ) );
    }
   else if( k == 3 )
    R.push_back( { Op::CONNECTED , int( rng() % n ) , int( rng() % n ) } );
   else
    R.push_back( { Op::COMPONENTS } );
   }
  const auto A = offlineConnectivity( n , R );
  std::vector< std::pair< int , int > > live;
  std::size_t q = 0;
  for( const auto & o : R ) {
   if( o.kind == Op::INSERT )
    live.emplace_back( o.u , o.v );
   else if( o.kind == Op::DELETE ) {
    for( std::size_t i = live.size() ; i-- > 0 ; )
     if( ( live[ i ] == std::pair< int , int >( o.u , o.v ) ) ||
         ( live[ i ] == std::pair< int , int >( o.v , o.u ) ) ) {
      live.erase( live.begin() + std::ptrdiff_t( i ) );
      break;
      }
    }
   else {
    UnionFind U( n );
    for( const auto & [ u , v ] : live )
     U.merge( u , v );
    CHECK( A[ q++ ] == ( o.kind == Op::COMPONENTS ? U.count() :
                         int( U.same( o.u , o.v ) ) ) );
    }
   }
  CHECK( q == A.size() );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File engineered_dynamic.cpp --------------------*/
/*--------------------------------------------------------------------------*/
