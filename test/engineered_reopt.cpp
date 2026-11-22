/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_reopt.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the recompute policy of the observers of Reoptimization.h,
 * beyond the book: the causes of the outcomes of the four observers, the
 * `Rebuilt` that a batch sends instead of its events when the policy asks
 * it at the close (static and dynamic observers), policies of their own
 * (always from scratch, never, abandons at random, the reinsertions of
 * the shortest paths), the drift of the floating total of the forest
 * caught by the audit and cured by the periodic sum or by the compensated
 * one, the strong guarantee of a batch that exits by an exception (the
 * graph and every observer as before), and the speculative changes undone
 * by UndoLog. Every solution is checked against the algorithms of the
 * book run from scratch, at the level 3 of the contracts.
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

#include "ADS/Modification.h"
#include "ADS/graph/Assignment.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

using LG = ListGraph< long long >;
using Ev = GraphEvent< long long >;
using GM = GraphModifications< long long >;

static_assert( ReoptPolicy< RatioPolicy > );
static_assert( mod::with_policy< ShortestPathsObserver< LG > > );
static_assert( mod::with_policy< SpanningForestObserver< LG > > );
static_assert( mod::with_policy< MaxFlowObserver< LG > > );
static_assert( mod::with_policy< AssignmentObserver< LG > > );
static_assert( undoable< Observable< LG > > );
static_assert( ! undoable< ObservedGraph< LG > > );
static_assert( Reopt::outcomeOf( Reopt::SCRATCH_BY_ABORT ) == Reopt::SCRATCH );
static_assert( Reopt::outcomeOf( Reopt::PARTIAL ) == Reopt::WARM );

/// an observer that records what it gets, with a mask
struct Recorder
{
 unsigned m = GraphEvents::ALL;
 std::vector< Ev > one;
 std::vector< std::vector< Ev > > batches;
 [[nodiscard]] unsigned mask( void ) const { return( m ); }
 void notify( const Ev & e ) { one.push_back( e ); }
 void notify( std::span< const Ev > s ) {
  batches.emplace_back( s.begin() , s.end() );
  }
 };

/// a policy that always computes from scratch, and counts what it is told
struct Always
{
 int records = 0;
 int last = -1;
 [[nodiscard]] bool scratch( const ReoptStats & s ) const {
  return( ! s.closing );
  }
 [[nodiscard]] bool abandon( const ReoptStats & , double ) const {
  return( false );
  }
 void record( int c , double ) {
  ++records;
  last = c;
  }
 };

/// a policy that never computes from scratch nor abandons
struct Never
{
 [[nodiscard]] bool scratch( const ReoptStats & ) const { return( false ); }
 [[nodiscard]] bool abandon( const ReoptStats & , double ) const {
  return( false );
  }
 void record( int , double ) const { }
 };

/// a policy that abandons the repairs at random, and asks `Rebuilt` at
/// random at the close of the batches
struct Coin
{
 std::mt19937 rng{ 3 };
 [[nodiscard]] bool scratch( const ReoptStats & ) {
  return( rng() % 5 == 0 );
  }
 [[nodiscard]] bool abandon( const ReoptStats & , double ) {
  return( rng() % 3 == 0 );
  }
 void record( int , double ) const { }
 };

static_assert( ReoptPolicy< Always > && ReoptPolicy< Never > &&
	       ReoptPolicy< Coin > );

/// the total weight of a forest
template< class W >
W total( const std::vector< WeightedEdge< W > > & F )
{
 W s = W();
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
    }
   } );
 return( N.maxFlow( s , t ) );
 }

/// the edges of g with their weights, sorted
std::vector< std::tuple< int , int , long long > > edgesOf( const LG & g )
{
 std::vector< std::tuple< int , int , long long > > E;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , long long w ) {
   if( g.directed() || ( u < v ) )
    E.emplace_back( u , v , w );
   } );
 std::sort( E.begin() , E.end() );
 return( E );
 }

/// whether the counts of the outcomes of o are the sums of its causes
template< class O >
bool causesAddUp( const O & o )
{
 return(
   ( o.count( Reopt::SCRATCH ) == o.byCause( Reopt::SCRATCH_BY_SIZE ) +
				    o.byCause( Reopt::SCRATCH_BY_ABORT ) +
				    o.byCause( Reopt::SCRATCH_BY_EVENT ) ) &&
   ( o.count( Reopt::WARM ) ==
     o.byCause( Reopt::WARM ) + o.byCause( Reopt::PARTIAL ) ) &&
   ( o.count( Reopt::CERTIFIED ) == o.byCause( Reopt::CERTIFIED ) ) );
 }

/// a random unoriented graph of n nodes and about 3 n edges
LG randomGraph( int n , std::mt19937 & rng , bool directed = false )
{
 LG g( n , directed );
 for( int i = 0 ; i < 3 * n ; ++i ) {
  const int u = int( rng() % n ) , v = int( rng() % n );
  if( u != v )
   g.insertEdge( u , v , 1 + static_cast< long long >( rng() % 20 ) );
  }
 return( g );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------------ THE CAUSES --------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "reopt-causes" )
{
 // the shortest paths: each cause in turn
 {
  LG g( 5 );  // oriented
  for( int u = 0 ; u + 1 < 5 ; ++u )
   g.insertEdge( u , u + 1 , 1 );
  g.insertEdge( 0 , 4 , 9 );
  ShortestPathsObserver< LG > sp( g , 0 );
  sp.setRecomputeRatio( 10 );
  Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
  CHECK( ( sp.distance( 4 ) == 4 ) &&
	 ( sp.cause() == Reopt::SCRATCH_BY_EVENT ) );
  og.setWeight( 0 , 4 , 8 );  // off the tree, harmless
  CHECK( ( sp.distance( 4 ) == 4 ) && ( sp.cause() == Reopt::CERTIFIED ) );
  og.setWeight( 3 , 4 , 0 );  // a decrease
  CHECK( ( sp.distance( 4 ) == 3 ) && ( sp.cause() == Reopt::WARM ) );
  og.setWeight( 0 , 1 , 100 );  // 4 of the 5 nodes lost
  CHECK( ( sp.distance( 4 ) == 8 ) &&
	 ( sp.cause() == Reopt::SCRATCH_BY_ABORT ) &&
	 ( sp.outcome() == Reopt::SCRATCH ) );
  og.setWeight( 2 , 3 , 5 );  // 1 of 5 lost: a repair
  CHECK( ( sp.distance( 3 ) == 106 ) && ( sp.cause() == Reopt::WARM ) );
  sp.setRecomputeRatio( 0 );
  og.setWeight( 0 , 4 , 7 );
  CHECK( ( sp.distance( 4 ) == 7 ) &&
	 ( sp.cause() == Reopt::SCRATCH_BY_SIZE ) );
  sp.setRecomputeRatio( 10 );
  og.insertEdge( 2 , 1 , -200 );  // a negative cycle: fatal
  CHECK( sp.tree().negativeCycle &&
	 ( sp.cause() == Reopt::SCRATCH_BY_EVENT ) );
  og.setWeight( 2 , 1 , 5 );  // fatal while there is one
  CHECK( ! sp.tree().negativeCycle &&
	 ( sp.cause() == Reopt::SCRATCH_BY_EVENT ) );
  CHECK( causesAddUp( sp ) && ( sp.byCause( Reopt::PARTIAL ) == 0 ) );
  CHECK( sp.byCause( Reopt::SCRATCH_BY_EVENT ) == 3 );
  CHECK_THROWS( ( void )sp.byCause( 0 ) , contract_violation );
  CHECK_THROWS( ( void )sp.byCause( Reopt::CAUSES ) , contract_violation );
  }
 // the forest, the flow and the assignment
 {
  LG g( 4 , false );
  g.insertEdge( 0 , 1 , 1 );
  g.insertEdge( 1 , 2 , 2 );
  g.insertEdge( 2 , 3 , 3 );
  g.insertEdge( 0 , 3 , 4 );
  SpanningForestObserver< LG > sf( g );
  MaxFlowObserver< LG > mf( g , 0 , 3 );
  sf.setRecomputeRatio( 10 );
  mf.setRecomputeRatio( 10 );
  Observable< LG , SpanningForestObserver< LG > , MaxFlowObserver< LG > > og(
    g , sf , mf );
  CHECK( ( sf.weight() == 6 ) && ( sf.cause() == Reopt::SCRATCH_BY_EVENT ) );
  CHECK( ( mf.value() == 5 ) && ( mf.cause() == Reopt::SCRATCH_BY_EVENT ) );
  og.setWeight( 0 , 3 , 9 );  // off F; on the cut, up
  CHECK( ( sf.weight() == 6 ) && ( sf.cause() == Reopt::CERTIFIED ) );
  CHECK( ( mf.value() == 10 ) && ( mf.cause() == Reopt::WARM ) );
  og.setWeight( 1 , 2 , 20 );  // out of F, a swap; off the cut
  CHECK( ( sf.weight() == 13 ) && ( sf.cause() == Reopt::WARM ) );
  CHECK( ( mf.value() == 10 ) && ( mf.cause() == Reopt::CERTIFIED ) );
  sf.setRecomputeRatio( 0 );
  mf.setRecomputeRatio( 0 );
  og.setWeight( 2 , 3 , 1 );
  CHECK( ( sf.weight() == 11 ) && ( sf.cause() == Reopt::SCRATCH_BY_SIZE ) );
  CHECK( ( mf.value() == 10 ) && ( mf.cause() == Reopt::SCRATCH_BY_SIZE ) );
  CHECK( causesAddUp( sf ) && causesAddUp( mf ) );
  }
 {
  const int k = 4;
  LG g( 2 * k , false );
  for( int i = 0 ; i < k ; ++i )
   for( int j = 0 ; j < k ; ++j )
    g.insertEdge( i , k + j , i == j ? 1 : 10 );
  AssignmentObserver< LG > ao( g , k );
  Observable< LG , AssignmentObserver< LG > > og( g , ao );
  CHECK( ( ao.total() == 4 ) && ( ao.cause() == Reopt::SCRATCH_BY_EVENT ) );
  og.setWeight( 0 , k + 1 , 20 );  // off the pairs, up
  CHECK( ( ao.total() == 4 ) && ( ao.cause() == Reopt::CERTIFIED ) );
  og.setWeight( 0 , k , 30 );  // one row of the four
  CHECK( ( ao.total() == 22 ) && ( ao.cause() == Reopt::WARM ) );
  {
   auto b = og.batch();          // the pairs of three rows up:
   for( int i = 1 ; i < k ; ++i )  // more than half
    og.setWeight( i , ao.partner( i ) , 40 );
   }
  std::vector< long long > C( std::size_t( k * k ) );
  for( int i = 0 ; i < k ; ++i )
   for( int j = 0 ; j < k ; ++j )
    C[ std::size_t( i * k + j ) ] = *g.weight( i , k + j );
  Assignment<> fresh( k , C );
  CHECK( ( ao.total() == fresh.total() ) &&
	 ( ao.cause() == Reopt::SCRATCH_BY_ABORT ) );
  CHECK( ao.assignment().cause() == Reopt::SCRATCH_BY_ABORT );
  CHECK( causesAddUp( ao ) && causesAddUp( ao.assignment() ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- REBUILT AT THE CLOSE ------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "reopt-early" )
{
 std::mt19937 rng( 5 );
 LG g = randomGraph( 40 , rng );
 ShortestPathsObserver< LG > sp( g , 0 );
 SpanningForestObserver< LG > sf( g );
 Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
 const int id = og.attach( sf );
 ( void )sp.distance( 1 );
 ( void )sf.weight();
 // by default the policy does not act at the close: the events queued
 sp.policy().rho = 0.01;
 sf.policy().rho = 0.01;
 auto changes = [ & ]( int k ) {  // on k distinct edges
  const auto E = edgesOf( g );
  const std::size_t at = rng() % E.size();
  auto b = og.batch();
  for( int j = 0 ; j < k ; ++j ) {
   const auto & [ u , v , w ] = E[ ( at + std::size_t( j ) ) % E.size() ];
   og.setWeight( u , v , w + 1 + static_cast< long long >( rng() % 5 ) );
   }
  };
 changes( 5 );
 CHECK( ( sp.pending() == 5 ) && ( sf.pending() == 5 ) );
 CHECK( og.subject().staticCounts( 0 ).rebuilds == 0 );
 ( void )sp.distance( 1 );
 ( void )sf.weight();
 CHECK( ( sp.cause() == Reopt::SCRATCH_BY_SIZE ) &&
	( sf.cause() == Reopt::SCRATCH_BY_SIZE ) );
 // early: Rebuilt instead of the events, and nothing queued after it
 sp.policy().early = true;
 sf.policy().early = true;
 changes( 5 );
 CHECK( ( sp.pending() == 0 ) && ( sf.pending() == 0 ) );
 // the counts, kept at the level of the audits alone
 CHECK( og.subject().staticCounts( 0 ).rebuilds ==
	std::uint64_t( ADS::test::kAllContracts ) );
 CHECK( og.subject().slotCounts( id ).rebuilds ==
	std::uint64_t( ADS::test::kAllContracts ) );
 og.setWeight( 0 , 1 , 3 );  // ignored until the query
 og.insertEdge( 0 , 1 , 2 );
 CHECK( ( sp.pending() == 0 ) && ( sf.pending() == 0 ) );
 const auto R = shortestPaths( g , 0 , Frontier::HEAP );
 for( int x : g.V() )
  CHECK( sp.distance( x ) == R.d[ x ] );
 CHECK( sp.cause() == Reopt::SCRATCH_BY_EVENT );
 CHECK( ( sf.weight() == total( kruskal( g ) ) ) &&
	( sf.cause() == Reopt::SCRATCH_BY_EVENT ) );
 // a batch below the ratio: the events queued
 sp.policy().rho = 10;
 sf.policy().rho = 10;
 changes( 3 );
 CHECK( ( sp.pending() == 3 ) && ( sf.pending() == 3 ) );
 CHECK( og.subject().staticCounts( 0 ).rebuilds ==
	std::uint64_t( ADS::test::kAllContracts ) );
 CHECK( sp.distance( 2 ) == shortestPaths( g , 0 , Frontier::HEAP ).d[ 2 ] );
 CHECK( sf.weight() == total( kruskal( g ) ) );
 // the assignment: every row again from the graph
 {
  const int k = 5;
  LG a( 2 * k , false );
  for( int i = 0 ; i < k ; ++i )
   for( int j = 0 ; j < k ; ++j )
    a.insertEdge( i , k + j , static_cast< long long >( rng() % 50 ) );
  AssignmentObserver< LG > ao( a , k );
  Observable< LG , AssignmentObserver< LG > > oa( a , ao );
  ( void )ao.total();
  ao.policy().early = true;
  ao.policy().rho = 0.1;  // of the k^2 costs
  {
   auto b = oa.batch();
   for( int i = 0 ; i < k ; ++i )
    oa.setWeight( i , k + ( i + 1 ) % k , 0 );
   }
  CHECK( ao.pending() == 0 );
  CHECK( ( ao.total() == 0 ) && ( ao.cause() == Reopt::SCRATCH_BY_EVENT ) );
  CHECK( causesAddUp( ao ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ OTHER POLICIES ----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "reopt-custom-policy" )
{
 // always from scratch: every query with events, and record() told
 {
  std::mt19937 rng( 6 );
  LG g = randomGraph( 30 , rng , true );
  ShortestPathsObserver< LG , Always > sp( g , 0 );
  Observable< LG , ShortestPathsObserver< LG , Always > > og( g , sp );
  ( void )sp.distance( 0 );
  for( int s = 0 ; s < 50 ; ++s ) {
   const auto E = edgesOf( g );
   const auto & [ u , v , w ] = E[ rng() % E.size() ];
   og.setWeight( u , v , static_cast< long long >( rng() % 20 ) );
   const auto R = shortestPaths( g , 0 , Frontier::HEAP );
   CHECK( sp.distance( v ) == R.d[ v ] );
   }
  CHECK( sp.byCause( Reopt::SCRATCH_BY_SIZE ) == 50 );
  CHECK( ( sp.policy().records == 51 ) &&
	 ( sp.policy().last == Reopt::SCRATCH_BY_SIZE ) );
  }
 // never: the repairs alone, however large, and at random: the scratch
 // as a fallback, at any point; against the book from scratch
 for( int seed : { 1 , 2 } ) {
  std::mt19937 rng( seed );
  LG g = randomGraph( 40 , rng );
  ShortestPathsObserver< LG , Never > a( g , 0 );
  ShortestPathsObserver< LG , Coin > b( g , 0 );
  SpanningForestObserver< LG , Never > c( g );
  SpanningForestObserver< LG , Coin > d( g );
  MaxFlowObserver< LG , Coin > e( g , 0 , 1 );
  Observable< LG , ShortestPathsObserver< LG , Never > ,
	      ShortestPathsObserver< LG , Coin > ,
	      SpanningForestObserver< LG , Never > ,
	      SpanningForestObserver< LG , Coin > ,
	      MaxFlowObserver< LG , Coin > >
    og( g , a , b , c , d , e );
  for( int s = 0 ; s < ADS::test::scaled( 400 , 100 ) ; ++s ) {
   {
    auto bt = og.batch();
    for( int j = 0 ; j < 1 + int( rng() % 6 ) ; ++j ) {
     const int u = int( rng() % 40 ) , v = int( rng() % 40 );
     if( u == v )
      continue;
     if( rng() % 3 == 0 )
      og.deleteEdge( u , v );
     else
      og.insertEdge( u , v , 1 + static_cast< long long >( rng() % 20 ) );
     }
    }
   const auto R = shortestPaths( g , 0 , Frontier::HEAP );
   const int x = int( rng() % 40 );
   CHECK( ( a.distance( x ) == R.d[ x ] ) && ( b.distance( x ) == R.d[ x ] ) );
   const long long K = total( kruskal( g ) );
   CHECK( ( c.weight() == K ) && ( d.weight() == K ) );
   CHECK( e.value() == freshFlow( g , 0 , 1 ) );
   }
  CHECK( a.byCause( Reopt::SCRATCH_BY_SIZE ) +
	   a.byCause( Reopt::SCRATCH_BY_ABORT ) ==
	 0 );
  CHECK( ( b.byCause( Reopt::SCRATCH_BY_ABORT ) > 0 ) &&
	 ( b.byCause( Reopt::SCRATCH_BY_EVENT ) > 1 ) );
  CHECK( causesAddUp( a ) && causesAddUp( b ) && causesAddUp( c ) &&
	 causesAddUp( d ) && causesAddUp( e ) );
  }
 // the reinsertions of the shortest paths, on negative weights with no
 // negative cycle: beta = 1 abandons the repair that extracts t a third
 // time, from 0, then from a, then from b
 {
  LG g( 4 );  // 0 , t = 1 , a = 2 , b = 3
  g.insertEdge( 0 , 1 , 5 );
  g.insertEdge( 0 , 2 , 1 );
  g.insertEdge( 0 , 3 , 2 );
  ShortestPathsObserver< LG > sp( g , 0 );
  sp.policy().rho = 10;
  sp.policy().beta = 1;
  Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
  CHECK( sp.distance( 1 ) == 5 );
  {
   auto b = og.batch();
   og.setWeight( 0 , 1 , 0 );
   og.insertEdge( 2 , 1 , -2 );
   og.insertEdge( 3 , 1 , -4 );
   }
  CHECK( ( sp.distance( 1 ) == -2 ) &&
	 ( sp.cause() == Reopt::SCRATCH_BY_ABORT ) );
  {  // t once, through b: repaired
   auto b = og.batch();
   og.setWeight( 0 , 1 , 7 );
   og.setWeight( 0 , 1 , 0 );
   og.setWeight( 2 , 1 , -3 );
   og.setWeight( 3 , 1 , -5 );
   }
  CHECK( ( sp.distance( 1 ) == -3 ) && ( sp.cause() == Reopt::WARM ) );
  }
 // the same on random DAGs, against Bellman-Ford-Moore
 {
  std::mt19937 rng( 8 );
  const int n = 40;
  LG g( n );
  for( int i = 0 ; i < 4 * n ; ++i ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u < v )
    g.insertEdge( u , v , static_cast< long long >( rng() % 21 ) - 10 );
   }
  ShortestPathsObserver< LG > sp( g , 0 );
  sp.policy().rho = 10;
  sp.policy().alpha = 10;
  sp.policy().beta = 1;
  Observable< LG , ShortestPathsObserver< LG > > og( g , sp );
  for( int s = 0 ; s < ADS::test::scaled( 400 , 100 ) ; ++s ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u < v )
    og.insertEdge( u , v , static_cast< long long >( rng() % 21 ) - 15 );
   const auto R = shortestPaths( g , 0 , Frontier::QUEUE );
   for( int x : g.V() )
    CHECK( sp.distance( x ) == R.d[ x ] );
   }
  CHECK( causesAddUp( sp ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE DRIFT --------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "reopt-drift" )
{
 using DG = ListGraph< double >;
 // a path, whose edge 0-1 goes to 1e17 and back: the plain total loses
 // the other two edges, which the audit catches
 auto run = [ & ]( RatioPolicy p ) {
  DG g( 4 , false );
  g.insertEdge( 0 , 1 , 1 );
  g.insertEdge( 1 , 2 , 1 );
  g.insertEdge( 2 , 3 , 1 );
  SpanningForestObserver< DG > f( g );
  f.policy() = p;
  f.setRecomputeRatio( 10 );
  Observable< DG , SpanningForestObserver< DG > > og( g , f );
  CHECK( f.weight() == 3 );
  og.setWeight( 0 , 1 , 1e17 );
  ( void )f.weight();
  og.setWeight( 0 , 1 , 1 );
  return( f.weight() );
  };
 RatioPolicy plain;
 plain.period = 0;
 CHECK_THROWS( ( void )run( plain ) , contract_violation );
 RatioPolicy comp = plain;
 comp.compensated = true;
 CHECK( run( comp ) == 3 );
 RatioPolicy each = plain;
 each.period = 1e-9;  // summed again at each change
 CHECK( run( each ) == 3 );
 // the default on random weights, against Kruskal
 {
  std::mt19937 rng( 11 );
  std::uniform_real_distribution< double > U( 0 , 1 );
  const int n = 60;
  DG g( n , false );
  for( int i = 0 ; i < 3 * n ; ++i ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u != v )
    g.insertEdge( u , v , U( rng ) );
   }
  SpanningForestObserver< DG > f( g );
  Observable< DG , SpanningForestObserver< DG > > og( g , f );
  for( int s = 0 ; s < ADS::test::scaled( 3000 , 600 ) ; ++s ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u != v )
    og.insertEdge( u , v , U( rng ) * ( rng() % 50 == 0 ? 1e6 : 1 ) );
   const double K = total( kruskal( g ) );
   const double e = f.weight() - K;
   CHECK( ( e < 0 ? -e : e ) <= 1e-9 * ( K > 1 ? K : 1 ) );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ROLLBACK ------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// what an observer holds that a rollback must leave as it was
template< class O >
std::vector< long long > stateOf( const O & o )
{
 std::vector< long long > s{ ( long long )o.pending() , o.outcome() ,
			     o.cause() };
 for( int c = 0 ; c < 3 ; ++c )
  s.push_back( o.count( c ) );
 for( int c = 1 ; c < Reopt::CAUSES ; ++c )
  s.push_back( o.byCause( c ) );
 return( s );
 }

}  // end( namespace )

ADS_TEST( "reopt-undo-batch" )
{
 std::mt19937 rng( 13 );
 LG g = randomGraph( 30 , rng );
 ShortestPathsObserver< LG > sp( g , 0 );
 SpanningForestObserver< LG > sf( g );
 MaxFlowObserver< LG > mf( g , 0 , 1 );
 Observable< LG , ShortestPathsObserver< LG > , SpanningForestObserver< LG > ,
	     MaxFlowObserver< LG > >
   og( g , sp , sf , mf );
 auto query = [ & ]( void ) {
  std::vector< long long > a;
  for( int x : g.V() )
   a.push_back( sp.distance( x ) );
  a.push_back( sf.weight() );
  a.push_back( mf.value() );
  return( a );
  };
 auto states = [ & ]( void ) {
  return( std::make_tuple( stateOf( sp ) , stateOf( sf ) , stateOf( mf ) ,
			    og.subject().staticCounts( 0 ).groups ) );
  };
 const auto A0 = query();
 const auto G0 = edgesOf( g );
 const auto S0 = states();
 // an exception in the middle of a batch: the graph and every observer
 // as they were, nothing told
 for( int round = 0 ; round < 20 ; ++round ) {
  try {
   auto b = og.batch();
   for( int j = 0 ; j < 10 ; ++j ) {
    if( j == 7 )
     throw( std::runtime_error( "abort" ) );
    const int u = int( rng() % 30 ) , v = int( rng() % 30 );
    if( u == v )
     continue;
    if( j % 3 == 0 )
     og.deleteEdge( u , v );
    else if( j % 3 == 1 )
     og.insertEdge( u , v , static_cast< long long >( rng() % 20 ) );
    else
     og.setWeight( u , v , static_cast< long long >( rng() % 20 ) );
    }
   }
  catch( const std::runtime_error & ) {
   }
  CHECK( ( edgesOf( g ) == G0 ) && ( states() == S0 ) );
  CHECK( og.depth() == 0 );
  }
 CHECK( query() == A0 );
 // Batch::rollback(), also nested: the outer batch keeps its events
 {
  auto b = og.batch();
  og.setWeight( std::get< 0 >( G0[ 0 ] ) , std::get< 1 >( G0[ 0 ] ) , 99 );
  b.rollback();
  }
 CHECK( ( edgesOf( g ) == G0 ) && ( sp.pending() == 0 ) );
 {
  auto outer = og.batch();
  og.setWeight( std::get< 0 >( G0[ 0 ] ) , std::get< 1 >( G0[ 0 ] ) , 99 );
  {
   auto inner = og.batch();
   og.setWeight( std::get< 0 >( G0[ 1 ] ) , std::get< 1 >( G0[ 1 ] ) , 98 );
   og.insertEdge( 0 , 29 , 1 );
   inner.rollback();
   }
  }
 CHECK( ( sp.pending() == 1 ) && ( sf.pending() == 1 ) );
 CHECK( g.weight( std::get< 0 >( G0[ 1 ] ) , std::get< 1 >( G0[ 1 ] ) ) ==
	std::get< 2 >( G0[ 1 ] ) );
 // a node in the batch has no inverse: the events go to the observers,
 // as on a structure that cannot be undone
 const int n0 = g.n();
 try {
  auto b = og.batch();
  og.insertNode();
  og.insertEdge( 0 , n0 , 3 );
  throw( std::runtime_error( "abort" ) );
  }
 catch( const std::runtime_error & ) {
  }
 CHECK( ( g.n() == n0 + 1 ) && g.hasEdge( 0 , n0 ) );
 CHECK( sp.pending() == 3 );
 {
  auto b = og.batch();
  og.insertNode();
  CHECK_THROWS( b.rollback() , contract_violation );
  }
 const auto R = shortestPaths( g , 0 , Frontier::HEAP );
 for( int x : g.V() )
  CHECK( sp.distance( x ) == R.d[ x ] );
 CHECK( sf.weight() == total( kruskal( g ) ) );
 CHECK( mf.value() == freshFlow( g , 0 , 1 ) );
 // a kind that nobody listens to is not collected: no rollback
 {
  LG h( 3 , false );
  Recorder r;
  r.m = GraphEvents::WEIGHT;
  Observable< LG , Recorder > oh( h , r );
  try {
   auto b = oh.batch();
   oh.insertEdge( 0 , 1 , 1 );
   throw( std::runtime_error( "abort" ) );
   }
  catch( const std::runtime_error & ) {
   }
  CHECK( h.hasEdge( 0 , 1 ) && r.batches.empty() );
  }
 }

ADS_TEST( "reopt-undo-log" )
{
 std::mt19937 rng( 17 );
 LG g = randomGraph( 30 , rng );
 ShortestPathsObserver< LG > sp( g , 0 );
 SpanningForestObserver< LG > sf( g );
 MaxFlowObserver< LG > mf( g , 0 , 1 );
 Observable< LG , ShortestPathsObserver< LG > , SpanningForestObserver< LG > ,
	     MaxFlowObserver< LG > >
   og( g , sp , sf , mf );
 UndoLog< GM > log;
 og.attach( log );
 auto query = [ & ]( void ) {
  std::vector< long long > a;
  for( int x : g.V() )
   a.push_back( sp.distance( x ) );
  a.push_back( sf.weight() );
  a.push_back( mf.value() );
  return( a );
  };
 // speculative changes, queried along the way, then undone: the
 // observers get the net effect of the undo in one batch, and their
 // solutions are those of the start
 for( int round = 0 ; round < ADS::test::scaled( 30 , 10 ) ; ++round ) {
  const auto A0 = query();
  const auto G0 = edgesOf( g );
  const std::size_t t = log.checkpoint();
  for( int j = 0 ; j < 12 ; ++j ) {
   const int u = int( rng() % 30 ) , v = int( rng() % 30 );
   if( u == v )
    continue;
   if( j % 3 == 0 )
    og.deleteEdge( u , v );
   else
    og.insertEdge( u , v , static_cast< long long >( rng() % 20 ) );
   if( j % 4 == 0 ) {
    const auto R = shortestPaths( g , 0 , Frontier::HEAP );
    CHECK( sp.distance( u ) == R.d[ u ] );
    CHECK( sf.weight() == total( kruskal( g ) ) );
    }
   }
  if( round % 2 ) {  // in a batch, too
   auto b = og.batch();
   og.setWeight( std::get< 0 >( G0[ 0 ] ) , std::get< 1 >( G0[ 0 ] ) , 7 );
   }
  log.rollback( og , t );
  CHECK( ( edgesOf( g ) == G0 ) && ( query() == A0 ) );
  CHECK( log.checkpoint() == t );
  }
 CHECK( causesAddUp( sp ) && causesAddUp( sf ) && causesAddUp( mf ) );
 // a node has no inverse: a hole, no rollback before it
 const std::size_t t = log.checkpoint();
 og.setWeight( std::get< 0 >( edgesOf( g )[ 0 ] ) ,
	       std::get< 1 >( edgesOf( g )[ 0 ] ) , 5 );
 og.insertNode();
 CHECK( log.size() == 0 );
 CHECK_THROWS( log.rollback( og , t ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_reopt.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
