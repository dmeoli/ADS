/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_lemon.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the interoperation with LEMON, beyond the book: the heaps of
 * ADS in the algorithms of LEMON, the maps of LEMON on the structures of
 * ADS, the graphs of either library seen by the other, the notifier of
 * LEMON told to the observers of ADS, a graph of LEMON kept in step with
 * one of ADS, the runs of LEMON on the threads of ADS, and the solves
 * from scratch of the matchings and of the assignment that ADS delegates
 * to LEMON. Each runs an algorithm of LEMON on the structure of ADS and
 * compares its result with that of LEMON on its own structures, or with
 * that of ADS. Built with and without `ADS_WITH_LEMON`: without it the
 * tests of the adapters check that the code of ADS is the one in use, and
 * those of the matchings, of the assignment and of the engine of the flows
 * check the code of ADS against the same references; at the level 3 of the
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

#include "ADS/Modification.h"
#include "ADS/graph/Assignment.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#if defined( ADS_WITH_LEMON )
 #include "ADS/interop/lemon/Digraphs.h"
 #include "ADS/interop/lemon/Heaps.h"
 #include "ADS/interop/lemon/Maps.h"
 #include "ADS/interop/lemon/Observers.h"
 #include "ADS/interop/lemon/Parallel.h"
 #if defined( ADS_LEMON_SHIM )
  #include "ADS/interop/lemon/Warm.h"

  #include <lemon/capacity_scaling.h>
  #include <lemon/cost_scaling.h>
  #include <lemon/cycle_canceling.h>
 #endif
 #include "ADS/parallel/ParallelShortestPaths.h"
 #include "ADS/parallel/ThreadPool.h"
 #include "ADS/priority/DialQueue.h"
 #include "ADS/priority/RadixHeap.h"
 #include "ADS/sequence/Fenwick.h"
 #include "ADS/sequence/ObservableArray.h"

 #include <lemon/bfs.h>
 #include <lemon/bin_heap.h>
 #include <lemon/concept_check.h>
 #include <lemon/concepts/digraph.h>
 #include <lemon/concepts/heap.h>
 #include <lemon/concepts/maps.h>
 #include <lemon/dijkstra.h>
 #include <lemon/kruskal.h>
 #include <lemon/list_graph.h>
 #include <lemon/maps.h>
 #include <lemon/network_simplex.h>
 #include <lemon/preflow.h>
 #include <lemon/smart_graph.h>
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether the interoperation with LEMON is on, as `ADS_WITH_LEMON` asks
#if defined( ADS_WITH_LEMON )
constexpr bool withLemon = true;
#else
constexpr bool withLemon = false;
#endif

/// whether the matchings are solved by LEMON
#if defined( ADS_MATCHING_LEMON ) && ADS_MATCHING_LEMON
constexpr bool matchingLemon = true;
#else
constexpr bool matchingLemon = false;
#endif

/// the delegations follow `ADS_WITH_LEMON`
void checkBackends( void )
{
 CHECK( matchingLemon == withLemon );
#if defined( ADS_ASSIGNMENT_LEMON ) && ADS_ASSIGNMENT_LEMON
 CHECK( withLemon );
#endif
 }

/// an engine of MaxFlowObserver that counts its solves
struct Counting
{
 static inline int scratches = 0;  ///< the solves from scratch
 static inline int warms = 0;      ///< the warm ones
 /// the solve from scratch of ADS
 static long long scratch( FlowNetwork & net , int s , int t , Augment h ) {
  ++scratches;
  return( net.maxFlow( s , t , h ) );
  }
 /// the warm solve of ADS
 static long long warm( FlowNetwork & net , int s , int t , Augment h ,
			long long v ) {
  ++warms;
  return( v + net.maxFlow( s , t , h ) );
  }
 };

#if defined( ADS_WITH_LEMON )

namespace LI = ADS::lemon_interop;
using GR = lemon::ListDigraph;
using LM = GR::ArcMap< int >;
using NM = GR::NodeMap< int >;

/// a random digraph of \f$ n \f$ nodes, \f$ m \f$ tries of an arc of length
/// in \f$ [ 0 , 99 ] \f$ (parallel arcs among them), and the node 7 erased,
/// so that the ids have a hole
void randomDigraph( GR & g , LM & l , int n , int m , unsigned seed )
{
 std::mt19937 rng( seed );
 std::vector< GR::Node > V;
 for( int i = 0 ; i < n ; ++i )
  V.push_back( g.addNode() );
 for( int k = 0 ; k < m ; ++k ) {
  const int u = int( rng() % unsigned( n ) );
  const int v = int( rng() % unsigned( n ) );
  if( u != v )
   l.set( g.addArc( V[ u ] , V[ v ] ) , int( rng() % 100 ) );
  }
 // two parallel arcs
 l.set( g.addArc( V[ 0 ] , V[ 1 ] ) , 5 );
 l.set( g.addArc( V[ 0 ] , V[ 1 ] ) , 3 );
 g.erase( V[ 7 ] );
 }

/// the distances of a run of `D` from \f$ s \f$, by the ids, -1 if not
/// reached
template< class D >
std::vector< int > distances( const GR & g , D & d , GR::Node s )
{
 d.run( s );
 std::vector< int > r( std::size_t( g.maxNodeId() + 1 ) , -1 );
 for( GR::NodeIt v( g ) ; v != lemon::INVALID ; ++v )
  if( d.reached( v ) )
   r[ std::size_t( g.id( v ) ) ] = d.dist( v );
 return( r );
 }

/// the sequence of LEMON's heap_test.cc, sorted by a heap of type `H`
template< class H >
void heapSort( void )
{
 const std::vector< int > seq = { 2 ,  28 , 19 , 27 , 33 , 25 , 13 ,
				  41 , 10 , 26 , 1 ,  9 ,  4 ,  34 };
 const std::vector< int > inc = { 20 , 28 , 34 , 16 , 0 , 46 , 44 ,
				  0 ,  42 , 32 , 14 , 8 , 6 ,  37 };
 const int n = int( seq.size() );
 {
  lemon::RangeMap< int > map( n , -1 );
  H h( map );
  for( int i = 0 ; i < n ; ++i )
   h.push( i , seq[ std::size_t( i ) ] );
  std::vector< int > v = seq;
  std::sort( v.begin() , v.end() );
  for( int x : v ) {
   CHECK( h.prio() == x );
   h.pop();
   }
  CHECK( h.empty() );
  }
 {  // the raises of heapIncreaseTest()
  lemon::RangeMap< int > map( n , -1 );
  H h( map );
  std::vector< int > v = seq;
  for( int i = 0 ; i < n ; ++i )
   h.push( i , v[ std::size_t( i ) ] );
  for( int i = 0 ; i < n ; ++i ) {
   v[ std::size_t( i ) ] += inc[ std::size_t( i ) ];
   h.increase( i , v[ std::size_t( i ) ] );
   }
  std::sort( v.begin() , v.end() );
  for( int x : v ) {
   CHECK( h.prio() == x );
   h.pop();
   }
  }
 {  // two equal priorities, heapEqualPairTest()
  lemon::RangeMap< int > map( 2 , -1 );
  H h( map );
  h.push( 0 , 0 );
  h.push( 1 , 0 );
  h.pop();
  h.pop();
  CHECK( h.empty() && ( h.state( 0 ) == H::POST_HEAP ) );
  }
 }

/// the shortest path tree of LEMON's Dijkstra on the heap `H` checked arc
/// by arc, as dijkstraHeapTest() of heap_test.cc, and its distances against
/// those of `BinHeap`
template< class H >
void heapDijkstra( const GR & g , const LM & l )
{
 typename lemon::Dijkstra< GR , LM >::template SetStandardHeap< H >::Create d(
   g , l );
 lemon::Dijkstra< GR , LM > b( g , l );
 const GR::Node s = GR::NodeIt( g );
 CHECK( distances( g , d , s ) == distances( g , b , s ) );
 for( GR::ArcIt a( g ) ; a != lemon::INVALID ; ++a )
  if( d.reached( g.source( a ) ) )
   CHECK( d.dist( g.target( a ) ) - d.dist( g.source( a ) ) <= l[ a ] );
 for( GR::NodeIt v( g ) ; v != lemon::INVALID ; ++v )
  if( d.reached( v ) && ( d.predArc( v ) != lemon::INVALID ) )
   CHECK( d.dist( v ) - d.dist( g.source( d.predArc( v ) ) ) ==
	  l[ d.predArc( v ) ] );
 // a second run on the same object: init() resets the cross references
 const GR::Node t = g.nodeFromId( 3 );
 lemon::Dijkstra< GR , LM > b2( g , l );
 CHECK( distances( g , d , t ) == distances( g , b2 , t ) );
 }

/// an observer that counts the events of a heap
struct HeapCounter
{
 int ins = 0;  ///< insertions
 int era = 0;  ///< erasures
 int chg = 0;  ///< changes of keys
 template< class H , class K >
 void on( const mod::Inserted< H , K > & ) {
  ++ins;
  }
 template< class H , class K >
 void on( const mod::Erased< H , K > & ) noexcept {
  ++era;
  }
 template< class H , class K >
 void on( const mod::Changed< mod::Key , H , K > & ) {
  ++chg;
  }
 };

#endif

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// DaryHeap: the concept of LEMON, the sequences of its heap_test.cc, the
// heap of maxima, LEMON's Dijkstra against BinHeap on a digraph with a hole
// in the ids and parallel arcs, and the events of the frontier
ADS_TEST( "lemon-dary-heap" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 using DH = LI::DaryHeap< int , lemon::RangeMap< int > >;
 lemon::checkConcept< lemon::concepts::Heap< int , lemon::RangeMap< int > > ,
		      DH >();
 lemon::checkConcept< lemon::concepts::Heap< int , NM > ,
		      LI::DaryHeap< int , NM > >();
 heapSort< DH >();
 heapSort<
   LI::DaryHeap< int , lemon::RangeMap< int > , std::less< int > , 2 > >();
 {  // the heap of maxima and the erasures of heapEraseTest()
  using H =
    LI::DaryHeap< double , lemon::RangeMap< int > , std::greater< double > >;
  lemon::RangeMap< int > map( 10 , -1 );
  H h( map );
  for( int i = 0 ; i < 10 ; ++i )
   h.push( i , i );
  h.erase( 1 );
  h.erase( 9 );
  CHECK( ( h.state( 1 ) == H::POST_HEAP ) &&
	 ( h.state( 9 ) == H::POST_HEAP ) );
  for( int i = 8 ; i >= 0 ; --i )
   if( i != 1 ) {
    CHECK( h.top() == i );
    h.pop();
    }
  CHECK( h.empty() );
  h.push( 4 , 1.5 );  // a slot freed, used again
  h.set( 4 , 2.5 );
  h.set( 5 , 0.5 );
  CHECK( ( h.top() == 4 ) && ( h[ 5 ] == 0.5 ) && ( h.size() == 2 ) );
  h.state( 4 , H::PRE_HEAP );
  CHECK( ( h.size() == 1 ) && ( h.state( 4 ) == H::PRE_HEAP ) );
  }
 GR g;
 LM l( g );
 randomDigraph( g , l , 400 , 3000 , 1 );
 heapDijkstra< LI::DaryHeap< int , NM > >( g , l );
 heapDijkstra< LI::DaryHeap< int , NM , std::less< int > , 8 > >( g , l );
 {  // the events of the frontier of LEMON's Dijkstra, told by ADS
  using H = LI::DaryHeap< int , NM , std::less< int > , 4 , Dynamic<> >;
  NM cr( g , -1 );
  H h( cr );
  HeapCounter c;
  h.subject().attach( c );
  typename lemon::Dijkstra< GR , LM >::template SetHeap< H ,
							 NM >::Create d( g ,
									   l );
  d.heap( h , cr );
  d.run( GR::NodeIt( g ) );
  int reached = 0;
  for( GR::NodeIt v( g ) ; v != lemon::INVALID ; ++v )
   reached += d.reached( v ) ? 1 : 0;
  CHECK( ( c.ins == reached ) && ( c.era == reached ) && ( c.chg > 0 ) );
  }
#endif
 }

// MonotoneHeap on DialQueue and on RadixHeap: the sequences of heap_test.cc
// but the raises, a randomized monotone sequence against std::set, and
// LEMON's Dijkstra against BinHeap
ADS_TEST( "lemon-monotone-heap" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 using MD = LI::MonotoneHeap< DialQueue< int > , int ,
			      lemon::RangeMap< int > >;
 using MR = LI::MonotoneHeap< RadixHeap< int > , int ,
			      lemon::RangeMap< int > >;
 lemon::checkConcept< lemon::concepts::Heap< int , lemon::RangeMap< int > > ,
		      MD >();
 lemon::checkConcept< lemon::concepts::Heap< int , lemon::RangeMap< int > > ,
		      MR >();
 auto sorted = [ & ]< class H >( void ) {
  const std::vector< int > seq = { 2 ,  28 , 19 , 27 , 33 , 25 , 13 ,
				   41 , 10 , 26 , 1 ,  9 ,  4 ,  34 };
  lemon::RangeMap< int > map( int( seq.size() ) , -1 );
  H h( map );
  for( std::size_t i = 0 ; i < seq.size() ; ++i )
   h.push( int( i ) , seq[ i ] );
  std::vector< int > v = seq;
  std::sort( v.begin() , v.end() );
  for( int x : v ) {
   CHECK( h.prio() == x );
   h.pop();
   }
  CHECK( h.empty() );
  };
 sorted.template operator()< MD >();
 sorted.template operator()< MR >();
 auto random = [ & ]< class H >( void ) {  // monotone: keys >= the last one
  const int n = 60;
  lemon::RangeMap< int > map( n , -1 );
  H h( map );
  std::set< std::pair< int , int > > ref;
  std::vector< int > pr( n , -1 );
  std::vector< bool > done( n , false );
  int low = 0;
  std::mt19937 rng( 7 );
  for( int step = 0 ; step < 6000 ; ++step ) {
   const int i = int( rng() % n );
   const int op = int( rng() % 5 );
   const int d = int( rng() % 50 );
   if( done[ i ] )
    continue;
   if( pr[ i ] < 0 ) {
    pr[ i ] = low + d;
    h.push( i , pr[ i ] );
    ref.emplace( pr[ i ] , i );
    continue;
    }
   if( op == 0 ) {  // a decrease, not below the last one popped
    ref.erase( { pr[ i ] , i } );
    pr[ i ] = low + d % ( pr[ i ] - low + 1 );
    h.decrease( i , pr[ i ] );
    ref.emplace( pr[ i ] , i );
    }
   else if( op == 1 ) {
    ref.erase( { pr[ i ] , i } );
    h.erase( i );
    pr[ i ] = -1;
    done[ i ] = true;  // a slot left is not pushed again
    }
   else if( ! ref.empty() ) {
    CHECK( h.prio() == ref.begin()->first );
    const int t = h.top();
    CHECK( pr[ t ] == ref.begin()->first );
    low = pr[ t ];
    ref.erase( { pr[ t ] , t } );  // of a tie, the item on top
    h.pop();
    pr[ t ] = -1;
    done[ t ] = true;
    }
   CHECK( h.size() == int( ref.size() ) );
   }
  };
 random.template operator()< MD >();
 random.template operator()< MR >();
 GR g;
 LM l( g );
 randomDigraph( g , l , 400 , 3000 , 2 );
 heapDijkstra< LI::MonotoneHeap< DialQueue< int > , int , NM > >( g , l );
 heapDijkstra< LI::MonotoneHeap< RadixHeap< int > , int , NM > >( g , l );
#endif
 }

// DenseMap as the maps of LEMON's Dijkstra, IndexMap over an
// ObservableArray as its length map (its writes told) and over a Fenwick
// (its prefix sums), ObservedMap with the old and the new value
ADS_TEST( "lemon-maps" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 lemon::checkConcept<
   lemon::concepts::ReferenceMap< GR::Node , bool , bool & , const bool & > ,
   LI::DenseMap< GR , bool > >();
 lemon::checkConcept< lemon::concepts::ReadWriteMap< GR::Arc , int > ,
		      LI::IndexMap< GR , ObservableArray< int > ,
				    GR::Arc > >();
 lemon::checkConcept< lemon::concepts::ReadWriteMap< GR::Arc , int > ,
		      LI::ObservedMap< GR , LM > >();
 GR g;
 LM l( g );
 randomDigraph( g , l , 300 , 2000 , 3 );
 {  // the lengths in an observable array of ADS
  using A = ObservableArray< int , Dynamic<> >;
  A arr( std::size_t( g.maxArcId() + 1 ) );
  LI::IndexMap< GR , A , GR::Arc > lm( g , arr );
  struct Writes
  {
   int n = 0;
   void on( const mod::Assigned< std::size_t , int > & ) { ++n; }
   } w;
  arr.attach( w );
  int arcs = 0;
  for( GR::ArcIt a( g ) ; a != lemon::INVALID ; ++a , ++arcs )
   lm.set( a , l[ a ] );
  CHECK( w.n == arcs );
  using DM = LI::DenseMap< GR , int >;
  using PM = LI::DenseMap< GR , GR::Arc >;
  using D = lemon::Dijkstra< GR ,
			     LI::IndexMap< GR , A , GR::Arc > >::SetDistMap<
    DM >::Create::SetPredMap< PM >::Create;
  D d( g , lm );
  DM dm( g );
  PM pm( g , lemon::INVALID );
  d.distMap( dm ).predMap( pm );
  lemon::Dijkstra< GR , LM > b( g , l );
  const GR::Node s = GR::NodeIt( g );
  CHECK( distances( g , d , s ) == distances( g , b , s ) );
  }
 {  // the prefix sums of the out-degrees by the ids of the nodes
  using F = Fenwick< SumOf< long long > >;
  F f( std::size_t( g.maxNodeId() + 1 ) );
  LI::IndexMap< GR , F > fm( g , f );
  for( GR::NodeIt v( g ) ; v != lemon::INVALID ; ++v )
   fm.set( v , lemon::countOutArcs( g , v ) );
  long long sum = 0;
  for( int i = 0 ; i <= g.maxNodeId() ; ++i ) {
   const GR::Node v = g.nodeFromId( i );
   if( g.valid( v ) ) {
    sum += lemon::countOutArcs( g , v );
    CHECK( ( fm[ v ] == lemon::countOutArcs( g , v ) ) &&
	   ( fm.prefix( v ) == sum ) );
    }
   }
  }
 {  // the writes of a map of LEMON, observed
  struct Changes
  {
   std::vector< std::tuple< int , int , int > > log;
   void on( const mod::Changed< mod::Cost , int , int > & e ) {
    log.emplace_back( e.h , e.from , e.to );
    }
   } c;
  LI::ObservedMap< GR , LM > om( g , l );
  const GR::Arc a = GR::ArcIt( g );
  const int old = l[ a ];
  om.set( a , old + 1 );  // nobody listens yet
  CHECK( ( l[ a ] == old + 1 ) && c.log.empty() );
  om.attach( c );
  om.set( a , 1000 );
  CHECK( ( om[ a ] == 1000 ) && ( c.log.size() == 1 ) &&
	 ( c.log[ 0 ] == std::tuple( g.id( a ) , old + 1 , 1000 ) ) );
  }
#endif
 }

// StaticDigraphView: the concept of LEMON's digraphs, and Dijkstra, Bfs,
// Preflow and NetworkSimplex of LEMON on a StaticGraph of ADS against ADS's
// own algorithms and against LEMON on a SmartDigraph of the same arcs
ADS_TEST( "lemon-static-digraph" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 using V = LI::StaticDigraphView< long long >;
 lemon::checkConcept< lemon::concepts::Digraph , V >();
 std::mt19937 rng( 4 );
 const int n = 600;
 std::vector< std::tuple< int , int , long long > > E;
 for( int k = 0 ; k < 4000 ; ++k ) {
  const int u = int( rng() % n );
  const int v = int( rng() % n );
  if( u != v )
   E.emplace_back( u , v , 1 + rng() % 100 );
  }
 StaticGraph< long long > g( n , E , true );
 V lg( g );
 CHECK( ( lemon::countNodes( lg ) == n ) &&
	( lemon::countArcs( lg ) == int( E.size() ) ) );
 int in = 0;
 for( V::NodeIt v( lg ) ; v != lemon::INVALID ; ++v )
  in += lemon::countInArcs( lg , v );
 CHECK( in == int( E.size() ) );
 const auto w = lg.weights();
 lemon::SmartDigraph sg;
 lemon::SmartDigraph::ArcMap< long long > sw( sg );
 for( int i = 0 ; i < n ; ++i )
  sg.addNode();
 for( const auto & [ u , v , c ] : E )
  sw.set( sg.addArc( sg.nodeFromId( u ) , sg.nodeFromId( v ) ) , c );
 // Dijkstra
 lemon::Dijkstra< V , V::WeightMap > d( lg , w );
 d.run( lg.nodeFromId( 0 ) );
 lemon::Dijkstra< lemon::SmartDigraph ,
		  lemon::SmartDigraph::ArcMap< long long > >
   sd( sg , sw );
 sd.run( sg.nodeFromId( 0 ) );
 const auto T = ADS::dijkstra( g , 0 );
 for( int v = 0 ; v < n ; ++v ) {
  CHECK( d.reached( lg.nodeFromId( v ) ) == sd.reached( sg.nodeFromId( v ) ) );
  if( d.reached( lg.nodeFromId( v ) ) )
   CHECK( ( d.dist( lg.nodeFromId( v ) ) == T.d[ std::size_t( v ) ] ) &&
	  ( d.dist( lg.nodeFromId( v ) ) == sd.dist( sg.nodeFromId( v ) ) ) );
  else
   CHECK( T.d[ std::size_t( v ) ] == infinity< long long >() );
  }
 // Bfs with a map of LEMON on the view
 V::NodeMap< int > dist( lg );
 lemon::Bfs< V > b( lg );
 b.distMap( dist );
 b.run( lg.nodeFromId( 0 ) );
 lemon::Bfs< lemon::SmartDigraph > sb( sg );
 sb.run( sg.nodeFromId( 0 ) );
 for( int v = 0 ; v < n ; ++v )
  if( sb.reached( sg.nodeFromId( v ) ) )
   CHECK( dist[ lg.nodeFromId( v ) ] == sb.dist( sg.nodeFromId( v ) ) );
 // Preflow
 lemon::Preflow< V , V::WeightMap > pf( lg , w , lg.nodeFromId( 0 ) ,
				       lg.nodeFromId( n - 1 ) );
 pf.run();
 FlowNetwork F( n );
 for( const auto & [ u , v , c ] : E )
  F.addEdge( u , v , c );
 const long long fa = F.maxFlow( 0 , n - 1 );
 CHECK( pf.flowValue() == fa );
 // NetworkSimplex: the flow of value fa of least cost, as on SmartDigraph
 lemon::NetworkSimplex< V , long long , long long > ns( lg );
 ns.upperMap( w ).costMap( w ).stSupply( lg.nodeFromId( 0 ) ,
					 lg.nodeFromId( n - 1 ) , fa );
 lemon::NetworkSimplex< lemon::SmartDigraph , long long ,
			long long > sns( sg );
 sns.upperMap( sw ).costMap( sw ).stSupply( sg.nodeFromId( 0 ) ,
					    sg.nodeFromId( n - 1 ) , fa );
 CHECK( ns.run() == decltype( ns )::OPTIMAL );
 CHECK( sns.run() == decltype( sns )::OPTIMAL );
 CHECK( ns.totalCost() == sns.totalCost() );
 // Dijkstra on the view of the interleaved layout
 using VI = LI::StaticDigraphView< long long , ArcLayout::Interleaved >;
 lemon::checkConcept< lemon::concepts::Digraph , VI >();
 const StaticGraph< long long , ArcLayout::Interleaved > gi( n , E , true );
 VI li( gi );
 const auto wi = li.weights();  // the map, which Dijkstra holds by reference
 lemon::Dijkstra< VI , VI::WeightMap > di( li , wi );
 di.run( li.nodeFromId( 0 ) );
 for( int v = 0 ; v < n ; ++v )
  if( di.reached( li.nodeFromId( v ) ) )
   CHECK( di.dist( li.nodeFromId( v ) ) == T.d[ std::size_t( v ) ] );
  else
   CHECK( T.d[ std::size_t( v ) ] == infinity< long long >() );
#endif
 }

// GraphView: the shortest paths of ADS on a ListDigraph with a hole in the
// ids and parallel arcs, against LEMON; toStatic() and deltaStepping() on
// four threads against the same
ADS_TEST( "lemon-graph-view" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 GR g;
 LM l( g );
 randomDigraph( g , l , 2000 , 10000 , 5 );
 LI::GraphView< GR , LM > view( g , l );
 static_assert( Graph< decltype( view ) > );
 CHECK( ( view.n() == 1999 ) && ( view.bound() == 2000 ) &&
	! view.hasNode( 7 ) && view.hasNode( 8 ) );
 lemon::Dijkstra< GR , LM > d( g , l );
 const GR::Node s = g.nodeFromId( 0 );
 const auto r = distances( g , d , s );
 const auto T = ADS::dijkstra( view , 0 );
 const auto S = LI::toStatic( g , l );
 CHECK( S.n() == view.bound() );
 ThreadPool pool( 4 );
 const auto dd = deltaStepping( pool , S , 0 , 16 );
 for( std::size_t i = 0 ; i < r.size() ; ++i )
  if( r[ i ] >= 0 )
   CHECK( ( T.d[ i ] == r[ i ] ) && ( dd[ i ] == r[ i ] ) );
  else if( i != 7 )
   CHECK( ( T.d[ i ] == infinity< int >() ) &&
	  ( dd[ i ] == infinity< int >() ) );
 // deltaStepping() on the view itself, whose bound exceeds n
 Sequential ex;
 const auto dv = deltaStepping( ex , view , 0 , 16 );
 for( std::size_t i = 0 ; i < r.size() ; ++i )
  if( r[ i ] >= 0 )
   CHECK( dv[ i ] == r[ i ] );
#endif
 }

// LemonTap: a random sequence of insertions and erasures of arcs and nodes
// of a ListDigraph, each told to an observer of ADS that keeps the arcs by
// their ids, compared with the graph after each step; the costs of the
// erasures; a batch for the arcs of an erased node; and an observer that
// refuses an insertion, after which the graph and the other observer are
// as before
ADS_TEST( "lemon-tap" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 GR g;
 LM l( g );
 using Tap = LI::LemonTap< GR , GR::Arc , LM >;
 using NTap = LI::LemonTap< GR , GR::Node >;
 Tap tap( g , l );  // after the map it reads
 NTap ntap( g );
 struct Shadow
 {
  std::vector< std::tuple< int , int , int , bool > > arc;  ///< u v cost in
  int refuse = 0;  ///< whether the insertions are refused
  void on( const Tap::Insert & e ) {
   if( refuse )
    throw( std::runtime_error( "refused" ) );
   if( arc.size() <= std::size_t( e.h ) )
    arc.resize( std::size_t( e.h ) + 1 , { -1 , -1 , 0 , false } );
   arc[ std::size_t( e.h ) ] = { e.key.u , e.key.v , e.value , true };
   }
  void on( const Tap::Erase & e ) noexcept {
   std::get< 2 >( arc[ std::size_t( e.h ) ] ) = e.value;
   std::get< 3 >( arc[ std::size_t( e.h ) ] ) = false;
   }
  void on( const mod::Cleared & ) noexcept {
   for( auto & t : arc )
    std::get< 3 >( t ) = false;
   }
  } sh , other;
 struct Nodes
 {
  int live = 0;
  void on( const NTap::Insert & ) { ++live; }
  void on( const NTap::Erase & ) noexcept { --live; }
  } nodes;
 tap.attach( other );
 tap.attach( sh );
 ntap.attach( nodes );
 std::mt19937 rng( 6 );
 std::vector< GR::Node > V;
 for( int i = 0 ; i < 50 ; ++i )
  V.push_back( g.addNode() );
 std::vector< int > lastCost( 1 << 16 , 0 );
 auto same = [ & ]( const Shadow & x ) {
  int live = 0;
  for( const auto & t : x.arc )
   live += std::get< 3 >( t ) ? 1 : 0;
  if( live != lemon::countArcs( g ) )
   return( false );
  for( GR::ArcIt a( g ) ; a != lemon::INVALID ; ++a ) {
   const auto & t = x.arc[ std::size_t( g.id( a ) ) ];
   if( ! std::get< 3 >( t ) ||
       ( std::get< 0 >( t ) != g.id( g.source( a ) ) ) ||
       ( std::get< 1 >( t ) != g.id( g.target( a ) ) ) )
    return( false );
   }
  return( true );
  };
 for( int step = 0 ; step < 3000 ; ++step ) {
  if( ( rng() % 3 ) || ( lemon::countArcs( g ) == 0 ) ) {
   const GR::Node u = V[ rng() % V.size() ] , v = V[ rng() % V.size() ];
   const GR::Arc a = g.addArc( u , v );
   CHECK( std::get< 2 >( sh.arc[ std::size_t( g.id( a ) ) ] ) == 0 );
   l.set( a , 1 + int( rng() % 100 ) );  // after add(): the default was told
   lastCost[ std::size_t( g.id( a ) ) ] = l[ a ];
   }
  else {
   GR::ArcIt a( g );
   for( unsigned k = rng() % 8 ; k > 0 && ( a != lemon::INVALID ) ; --k )
    ++a;
   if( a == lemon::INVALID )
    a = GR::ArcIt( g );
   const int id = g.id( a );
   g.erase( GR::Arc( a ) );
   CHECK( std::get< 2 >( sh.arc[ std::size_t( id ) ] ) ==
	  lastCost[ std::size_t( id ) ] );  // read before the removal
   }
  CHECK( same( sh ) && same( other ) );
  }
 // a node erased: the erasures of its arcs first
 const int before = lemon::countArcs( g );
 const int deg =
   lemon::countOutArcs( g , V[ 3 ] ) + lemon::countInArcs( g , V[ 3 ] );
 g.erase( V[ 3 ] );
 V.erase( V.begin() + 3 );
 CHECK( ( lemon::countArcs( g ) <= before - deg / 2 ) && same( sh ) &&
	( nodes.live == 49 ) );
 // an insertion refused by an observer of ADS: the other one, told
 // before, receives the erasure
 const int arcs = lemon::countArcs( g );
 sh.refuse = 1;
 bool thrown = false;
 try {
  g.addArc( V[ 0 ] , V[ 1 ] );
  }
 catch( const std::runtime_error & ) {
  thrown = true;
  }
 CHECK( thrown && ! tap.spoiled() );
 // the arc refused: abLEMON (from 1.9.0-37) removes it, and the other
 // observer then holds the graph; LEMON 1.3 keeps it
 if( lemon::countArcs( g ) == arcs )
  CHECK( same( other ) );
 else
  CHECK( lemon::countArcs( g ) == arcs + 1 );
 sh.refuse = 0;
 g.clear();
 CHECK( same( sh ) );
#endif
 }

// LemonMirror: a ListDigraph kept in step with a ListGraph of ADS, oriented
// and not, through random insertions, deletions and changes of weights,
// the maximum flow of LEMON's Preflow on the mirror against that of ADS
ADS_TEST( "lemon-mirror" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 for( const bool directed : { true , false } ) {
  using G = ListGraph< long long >;
  using M = LI::LemonMirror< G >;
  std::mt19937 rng( directed ? 8 : 9 );
  const int n = 80;
  G g( n , directed );
  for( int k = 0 ; k < 400 ; ++k ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u != v )
    g.insertEdge( u , v , 1 + rng() % 100 );
   }
  M m( g );
  ObservedGraph< G , Static< M > > og( g , m );
  CHECK( ! m.stale() );
  for( int step = 0 ; step < 300 ; ++step ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u == v )
    continue;
   if( g.weight( u , v ) ) {
    if( rng() % 2 )
     og.setWeight( u , v , static_cast< long long >( rng() % 100 ) );
    else
     og.deleteEdge( u , v );
    }
   else
    og.insertEdge( u , v , 1 + rng() % 100 );
   CHECK( m.stale() );
   long long sum = 0 , lsum = 0;
   int edges = 0;
   for( int x : g.V() )
    g.forEachEdge( x , [ & ]( int y , long long w ) {
     ++edges;
     sum += w;
     CHECK( m.arc( x , y ) != lemon::INVALID );
     } );
   for( M::LG::ArcIt a( m.digraph() ) ; a != lemon::INVALID ; ++a )
    lsum += m.weights()[ a ];
   CHECK( ( lemon::countArcs( m.digraph() ) == edges ) && ( sum == lsum ) );
   if( step % 50 == 0 ) {
    lemon::Preflow< M::LG , M::WeightMap > pf( m.digraph() , m.weights() ,
					      m.node( 0 ) , m.node( n - 1 ) );
    pf.run();
    MaxFlowObserver< G > fresh( g , 0 , n - 1 );
    CHECK( pf.flowValue() == fresh.value() );
    m.clearKinds();
    }
   }
  }
#endif
 }

// k Dijkstras of LEMON at once on one ListDigraph, on the threads of ADS,
// with the maps that do not attach to the notifier, against k runs one
// after the other; LEMON's kruskal() on the edges sorted by ADS
ADS_TEST( "lemon-parallel" )
{
 checkBackends();
#if defined( ADS_WITH_LEMON )
 GR g;
 LM l( g );
 randomDigraph( g , l , 1500 , 9000 , 10 );
 std::vector< GR::Node > S;
 for( int i = 0 ; i < 16 ; ++i )
  S.push_back( g.nodeFromId( i == 7 ? 100 : i ) );
 ThreadPool pool( 4 );
 const auto D = LI::dijkstraFrom( pool , g , l , S );
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  lemon::Dijkstra< GR , LM > d( g , l );
  const auto r = distances( g , d , S[ i ] );
  for( std::size_t v = 0 ; v < r.size() ; ++v )
   if( r[ v ] >= 0 )
    CHECK( D[ i ][ v ] == r[ v ] );
   else if( v != 7 )
    CHECK( D[ i ][ v ] == infinity< int >() );
  }
 using UG = lemon::ListGraph;
 UG u;
 UG::EdgeMap< int > c( u );
 std::mt19937 rng( 11 );
 std::vector< UG::Node > V;
 for( int i = 0 ; i < 3000 ; ++i )
  V.push_back( u.addNode() );
 for( int k = 0 ; k < 20000 ; ++k ) {
  const unsigned a = rng() % 3000 , b = rng() % 3000;
  if( a != b )
   c.set( u.addEdge( V[ a ] , V[ b ] ) , int( rng() % 1000 ) );
  }
 UG::EdgeMap< bool > t0( u ) , t1( u );
 const long long w0 = lemon::kruskal( u , c , t0 );
 const auto E = LI::sortedArcs( pool , u , c );
 CHECK(
   std::is_sorted( E.begin() , E.end() ,
		   []( const auto & a , const auto & b ) {
    return( a.second < b.second );
    } ) );
 const long long w1 = lemon::kruskal( u , E , t1 );
 CHECK( w0 == w1 );
#endif
 }

// the matchings, by LEMON with ADS_WITH_LEMON and by ADS without, against
// the branch and bound and against each other's sizes, on random graphs
ADS_TEST( "lemon-matching" )
{
 checkBackends();
 std::mt19937 rng( 12 );
 for( int rep = 0 ; rep < 40 ; ++rep ) {
  const int n = 4 + int( rng() % 11 );
  std::vector< std::tuple< int , int , long long > > E;
  ListGraph< long long > g( n , false );
  for( int k = 0 ; k < 3 * n ; ++k ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( ( u != v ) && ! g.weight( u , v ) ) {
    const long long w = -20 + static_cast< long long >( rng() % 100 );
    E.emplace_back( u , v , w );
    g.insertEdge( u , v , w );
    }
   }
  const auto [ bw , bm ] = branchAndBoundMatching( n , E );
  const auto [ mw , mm ] = maxWeightMatching( n , E );
  CHECK( mw == bw );
  long long s = 0;
  std::vector< bool > end( n , false );
  for( std::size_t e : mm ) {
   const auto [ u , v , w ] = E[ e ];
   CHECK( ! end[ u ] && ! end[ v ] );
   end[ u ] = end[ v ] = true;
   s += w;
   }
  CHECK( s == mw );
  // the size: that of the largest matching of the edges, all weight 1
  std::vector< std::tuple< int , int , long long > > U;
  for( const auto & [ u , v , w ] : E )
   U.emplace_back( u , v , 1 );
  const auto mate = maxCardinalityMatching( g );
  int size = 0;
  for( int u = 0 ; u < n ; ++u )
   if( mate[ std::size_t( u ) ] != NIL ) {
    CHECK( mate[ std::size_t( mate[ std::size_t( u ) ] ) ] == u );
    CHECK( g.weight( u , mate[ std::size_t( u ) ] ).has_value() );
    ++size;
    }
  CHECK( size / 2 == branchAndBoundMatching( n , U ).first );
  // of largest size, the weight by ADS in both builds
  const auto [ cw , cm ] = maxWeightMatching( n , E , true );
  CHECK( 2 * static_cast< long long >( cm.size() ) == size );
  ( void )cw;
  }
 }

// the assignment: the solves from scratch, by LEMON's MinCostAssignment
// with ADS_WITH_LEMON and abLEMON, by ADS otherwise, against the
// enumeration of the permutations, then the warm repairs of ADS after
// changes of rows, columns and single costs, and forbidden pairs
ADS_TEST( "lemon-assignment" )
{
 checkBackends();
 std::mt19937 rng( 13 );
 auto best = []( int n , const std::vector< long long > & C , bool & any ) {
  std::vector< int > p( static_cast< std::size_t >( n ) );
  std::iota( p.begin() , p.end() , 0 );
  long long b = infinity< long long >();
  any = false;
  do {
   long long s = 0;
   bool ok = true;
   for( int i = 0 ; i < n ; ++i ) {
    const long long c =
      C[ std::size_t( i ) * std::size_t( n ) + std::size_t( p[ i ] ) ];
    if( c == infinity< long long >() ) {
     ok = false;
     break;
     }
    s += c;
    }
   if( ok ) {
    any = true;
    b = std::min( b , s );
    }
   } while( std::next_permutation( p.begin() , p.end() ) );
  return( b );
  };
 for( int rep = 0 ; rep < 30 ; ++rep ) {
  const int n = 2 + int( rng() % 6 );
  std::vector< long long > C( std::size_t( n * n ) );
  for( auto & c : C )
   c = ( rng() % 9 == 0 ) ? infinity< long long >()
			  : -30 + static_cast< long long >( rng() % 100 );
  Assignment< long long > A( n , C );
  bool any;
  const long long b = best( n , C , any );
  CHECK( A.perfect() == any );
  if( any )
   CHECK( A.total() == b );
  for( int step = 0 ; step < 6 ; ++step ) {
   const int i = int( rng() % n ) , j = int( rng() % n );
   const long long c = -30 + static_cast< long long >( rng() % 100 );
   C[ std::size_t( i * n + j ) ] = c;
   A.setCost( i , j , c );
   if( step % 3 == 2 ) {
    std::vector< long long > R( static_cast< std::size_t >( n ) );
    for( int k = 0 ; k < n ; ++k )
     R[ std::size_t( k ) ] = C[ std::size_t( i * n + k ) ] =
       -30 + static_cast< long long >( rng() % 100 );
    A.setRow( i , R );
    }
   const long long bb = best( n , C , any );
   CHECK( A.perfect() == any );
   if( any )
    CHECK( A.total() == bb );
   }
  }
 }

// the engine of MaxFlowObserver: the solves of a counting engine give the
// values of the default, on random changes of the capacities
ADS_TEST( "lemon-flow-engine" )
{
 checkBackends();
 static_assert( FlowEngine< Counting > && FlowEngine< AugmentEngine > );
 using G = ListGraph< long long >;
 std::mt19937 rng( 14 );
 const int n = 60;
 G g( n , true );
 for( int k = 0 ; k < 400 ; ++k ) {
  const int u = int( rng() % n ) , v = int( rng() % n );
  if( u != v )
   g.insertEdge( u , v , 1 + rng() % 50 );
  }
 MaxFlowObserver< G > a( g , 0 , n - 1 );
 MaxFlowObserver< G , RatioPolicy , Counting > b( g , 0 , n - 1 ,
						Augment::PUSH_RELABEL );
 Observable< G , MaxFlowObserver< G > ,
	     MaxFlowObserver< G , RatioPolicy , Counting > >
   og( g , a , b );
 CHECK( a.value() == b.value() );
 for( int step = 0 ; step < 100 ; ++step ) {
  const int u = int( rng() % n ) , v = int( rng() % n );
  if( u == v )
   continue;
  if( g.weight( u , v ) )
   og.setWeight( u , v , static_cast< long long >( rng() % 60 ) );
  else
   og.insertEdge( u , v , 1 + rng() % 50 );
  CHECK( a.value() == b.value() );
  }
 CHECK( ( Counting::scratches >= 1 ) &&
	( Counting::scratches + Counting::warms >= 2 ) );
 }

// the warm starts of the four minimum cost flows of LEMON written by the
// rules of the shim: after each random change of a cost, of an upper bound
// or of a pair of supplies, runWarm() against a new run() from scratch
// (the same outcome and the same total cost), with the warm path entered;
// and the setters of one arc or node against the map setters
ADS_TEST( "lemon-warm-mcf" )
{
 checkBackends();
#if defined( ADS_LEMON_SHIM )
 using G = lemon::SmartDigraph;
 using VM = G::ArcMap< long long >;
 using SM = G::NodeMap< long long >;
 auto check = [ & ]< class A >( unsigned seed ) {
  std::mt19937 rng( seed );
  G g;
  const int n = 50;
  for( int i = 0 ; i < n ; ++i )
   g.addNode();
  VM c( g ) , u( g );
  SM s( g , 0 );
  std::vector< G::Arc > arcs;
  for( int k = 0 ; k < 350 ; ++k ) {
   const int a = int( rng() % n ) , b = int( rng() % n );
   if( a == b )
    continue;
   arcs.push_back( g.addArc( g.nodeFromId( a ) , g.nodeFromId( b ) ) );
   c.set( arcs.back() , 1 + static_cast< long long >( rng() % 100 ) );
   u.set( arcs.back() , 1 + rng() % 30 );
   }
  s.set( g.nodeFromId( 0 ) , 25 );
  s.set( g.nodeFromId( n - 1 ) , -25 );
  A w( g ) , m( g );  // by the setters of one item, by the maps
  w.costMap( c ).upperMap( u ).supplyMap( s );
  m.costMap( c ).upperMap( u ).supplyMap( s );
  CHECK( w.run() == m.run() );
  int warm = 0;
  for( int step = 0 ; step < 120 ; ++step ) {
   const G::Arc a = arcs[ rng() % arcs.size() ];
   const int kind = int( rng() % 3 );
   if( kind == 0 ) {
    c.set( a , rng() % 100 );
    w.warmCost( a , c[ a ] );
    m.costMap( c );
    }
   else if( kind == 1 ) {
    u.set( a , rng() % 30 );
    w.warmUpper( a , u[ a ] );
    m.upperMap( u );
    }
   else {  // a unit of supply moved, the sum kept 0
    const G::Node x = g.nodeFromId( int( rng() % n ) ) ,
		  y = g.nodeFromId( int( rng() % n ) );
    s.set( x , s[ x ] + 1 );
    s.set( y , s[ y ] - 1 );
    w.warmSupply( x , s[ x ] ).warmSupply( y , s[ y ] );
    m.supplyMap( s );
    }
   warm += w.warmReady() ? 1 : 0;
   const auto rw = w.runWarm() , rm = m.runWarm();
   A cold( g );
   cold.costMap( c ).upperMap( u ).supplyMap( s );
   const auto rc = cold.run();
   CHECK( ( rw == rc ) && ( rm == rc ) );
   if( rc == A::OPTIMAL )
    CHECK( ( w.totalCost() == cold.totalCost() ) &&
	   ( m.totalCost() == cold.totalCost() ) );
   }
  CHECK( warm > 60 );  // the warm path taken after most changes
  };
 check.template operator()< lemon::NetworkSimplex< G , long long ,
						   long long > >(
   21 );
 check.template operator()< lemon::CostScaling< G , long long , long long > >(
   22 );
 check.template operator()< lemon::CycleCanceling< G , long long ,
						   long long > >(
   23 );
 check.template
 operator()< lemon::CapacityScaling< G , long long , long long > >( 24 );
#endif
 }

// Preflow from a flow with deficits: initWarm() of a flow whose arcs were
// cut below their flow against Preflow from scratch, and runWarm() after
// random changes of the capacities, raises and cuts
ADS_TEST( "lemon-warm-preflow" )
{
 checkBackends();
#if defined( ADS_LEMON_SHIM )
 using G = lemon::SmartDigraph;
 using VM = G::ArcMap< long long >;
 std::mt19937 rng( 25 );
 G g;
 const int n = 80;
 for( int i = 0 ; i < n ; ++i )
  g.addNode();
 VM cap( g );
 for( int k = 0 ; k < 600 ; ++k ) {
  const int a = int( rng() % n ) , b = int( rng() % n );
  if( a != b )
   cap.set( g.addArc( g.nodeFromId( a ) , g.nodeFromId( b ) ) ,
	    1 + rng() % 40 );
  }
 const G::Node s = g.nodeFromId( 0 ) , t = g.nodeFromId( n - 1 );
 using PF = lemon::Preflow< G , VM >;
 auto cold = [ & ]( void ) {
  PF p( g , cap , s , t );
  p.run();
  return( p.flowValue() );
  };
 PF w( g , cap , s , t );
 CHECK( ! w.warmReady() );
 w.run();
 CHECK( w.warmReady() && ( w.flowValue() == cold() ) );
 int cuts = 0;
 for( int step = 0 ; step < 200 ; ++step ) {
  const bool raise = rng() % 2;
  G::Arc a = g.arcFromId( int( rng() % unsigned( g.arcNum() ) ) );
  for( int k = 0 ; ! raise && ( k < g.arcNum() ) && ( w.flow( a ) == 0 ) ;
       ++k )  // a cut of an arc with flow
   a = g.arcFromId( ( g.id( a ) + 1 ) % g.arcNum() );
  const long long f = w.flow( a );
  const long long c = raise
			? cap[ a ] + 1 + static_cast< long long >( rng() % 20 )
			: static_cast< long long >( rng() % ( f + 1 ) );
  cuts += c < f ? 1 : 0;  // a deficit at the head
  cap.set( a , c );
  w.runWarm();
  CHECK( w.flowValue() == cold() );
  }
 CHECK( cuts > 10 );
 {  // a flow that init( flow ) refuses, which initWarm() repairs
  VM f( g );
  for( G::ArcIt a( g ) ; a != lemon::INVALID ; ++a )
   f.set( a , w.flow( a ) );
  for( G::ArcIt a( g ) ; a != lemon::INVALID ; ++a )
   if( f[ a ] > 1 ) {
    cap.set( a , f[ a ] / 2 );
    break;
    }
  PF p( g , cap , s , t );
  p.initWarm( f );
  p.startFirstPhase();
  p.startSecondPhase();
  CHECK( p.flowValue() == cold() );
  }
#endif
 }

// the solvers of ADS on the warm starts of LEMON: MaxFlowObserver on
// PreflowEngine against the push-relabel of ADS, Assignment on
// NetworkSimplexAssignment against the code of ADS, and
// MinCostFlowObserver, through the maps and the tap observed, against a
// NetworkSimplex from scratch; each with its warm solves counted
ADS_TEST( "lemon-warm-observers" )
{
 checkBackends();
#if defined( ADS_LEMON_SHIM )
 {
  using G = ListGraph< long long >;
  std::mt19937 rng( 26 );
  const int n = 60;
  G g( n , true );
  for( int k = 0 ; k < 400 ; ++k ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u != v )
    g.insertEdge( u , v , 1 + rng() % 50 );
   }
  using L = MaxFlowObserver< G , RatioPolicy , LI::PreflowEngine >;
  MaxFlowObserver< G > a( g , 0 , n - 1 , Augment::PUSH_RELABEL );
  L b( g , 0 , n - 1 );
  Observable< G , MaxFlowObserver< G > , L > og( g , a , b );
  CHECK( a.value() == b.value() );
  for( int step = 0 ; step < 150 ; ++step ) {
   const int u = int( rng() % n ) , v = int( rng() % n );
   if( u == v )
    continue;
   if( g.weight( u , v ) )
    og.setWeight( u , v , static_cast< long long >( rng() % 60 ) );
   else
    og.insertEdge( u , v , 1 + rng() % 50 );
   CHECK( a.value() == b.value() );
   }
  }
 {
  std::mt19937 rng( 27 );
  const int n = 30;
  std::vector< long long > C( std::size_t( n * n ) );
  for( auto & c : C )
   c = static_cast< long long >( rng() % 1000 );
  Assignment< long long > a( n , C );
  Assignment< long long , RatioPolicy , LI::NetworkSimplexAssignment<> > b( n ,
									  C );
  CHECK( a.total() == b.total() );
  for( int step = 0 ; step < 60 ; ++step ) {
   const int i = int( rng() % n ) , j = int( rng() % n );
   const long long c = ( step % 10 == 9 )
			 ? infinity< long long >()
			 : static_cast< long long >( rng() % 1000 );
   a.setCost( i , j , c );
   b.setCost( i , j , c );
   if( step % 7 == 3 ) {
    std::vector< long long > R( static_cast< std::size_t >( n ) );
    for( auto & x : R )
     x = static_cast< long long >( rng() % 1000 );
    a.setRow( i , R );
    b.setRow( i , R );
    }
   CHECK( a.perfect() == b.perfect() );
   if( a.perfect() )
    CHECK( a.total() == b.total() );
   }
  CHECK( b.count( Reopt::WARM ) > 30 );
  }
 {
  using G = lemon::ListDigraph;
  using VM = G::ArcMap< long long >;
  using SM = G::NodeMap< long long >;
  using A = lemon::NetworkSimplex< G , long long , long long >;
  using O = LI::MinCostFlowObserver< G , A , VM , VM , SM >;
  std::mt19937 rng( 28 );
  G g;
  VM c( g ) , u( g );
  SM s( g , 0 );
  const int n = 40;
  std::vector< G::Node > V;
  for( int i = 0 ; i < n ; ++i )
   V.push_back( g.addNode() );
  for( int k = 0 ; k < 250 ; ++k ) {
   const int a = int( rng() % n ) , b = int( rng() % n );
   if( a != b ) {
    const G::Arc x = g.addArc( V[ a ] , V[ b ] );
    c.set( x , rng() % 100 );
    u.set( x , 1 + rng() % 20 );
    }
   }
  s.set( V[ 0 ] , 15 );
  s.set( V[ n - 1 ] , -15 );
  O o( g , c , u , s );
  LI::ObservedMap< G , VM , mod::Cost > oc( g , c );
  LI::ObservedMap< G , VM , mod::Upper > ou( g , u );
  LI::ObservedMap< G , SM , mod::Supply > os( g , s );
  LI::LemonTap< G > tap( g );
  oc.attach( o );
  ou.attach( o );
  os.attach( o );
  tap.attach( o );
  auto cold = [ & ]( void ) {
   A x( g );
   x.costMap( c ).upperMap( u ).supplyMap( s );
   const auto r = x.run();
   return( std::pair( r , r == A::OPTIMAL ? x.totalCost() : 0LL ) );
   };
  for( int step = 0 ; step < 120 ; ++step ) {
   G::ArcIt a( g );
   for( unsigned k = rng() % 50 ; k > 0 && ( a != lemon::INVALID ) ; --k )
    ++a;
   if( a == lemon::INVALID )
    a = G::ArcIt( g );
   const int kind = int( rng() % 4 );
   // every other step the changes of the maps go through a channel, which
   // sends their net effect at its end
   std::optional< Channel< decltype( oc ) , decltype( ou ) , decltype( os ) > >
     ch;
   if( step % 2 )
    ch.emplace( oc , ou , os );
   if( kind == 0 )
    oc.set( a , rng() % 100 );
   else if( kind == 1 )
    ou.set( a , rng() % 20 );
   else if( kind == 2 ) {
    const G::Node x = V[ rng() % V.size() ] , y = V[ rng() % V.size() ];
    os.set( x , s[ x ] + 1 );
    os.set( y , s[ y ] - 1 );
    }
   else {  // a new arc: the algorithm reset
    const G::Arc x = g.addArc( V[ rng() % V.size() ] , V[ rng() % V.size() ] );
    oc.set( x , rng() % 100 );
    ou.set( x , 1 + rng() % 20 );
    }
   ch.reset();
   const auto r = o.solve();
   const auto [ rc , tc ] = cold();
   CHECK( r == rc );
   if( r == A::OPTIMAL )
    CHECK( o.algorithm().totalCost() == tc );
   }
  CHECK( ( o.warms > 60 ) && ( o.scratches >= 1 ) );
  }
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_lemon.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
