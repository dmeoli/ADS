/*--------------------------------------------------------------------------*/
/*----------------------------- File reopt.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the reoptimization, as it is measured for the solvers
 * of an optimization framework: each algorithm that keeps its solution
 * through the changes of its input is timed while the input changes, a
 * solve after each batch of k changes, against the solve from scratch on
 * the input changed, as a function of the size k of the batches and of the
 * kind of the changes. The problems are
 *
 * - resolve sssp, the shortest paths from the node 0 of G( n , 4 n ),
 *   oriented, costs in [ 1 , 1000 ]: ShortestPathsObserver, against
 *   shortestPaths() on a binary heap and dijkstra_shortest_paths() of the
 *   Boost Graph Library ("Boost dijkstra"), the answer the distances of 8
 *   fixed nodes;
 * - resolve mst, the minimum spanning forest of the same graph unoriented:
 *   SpanningForestObserver, against kruskal() and
 *   kruskal_minimum_spanning_tree() of Boost ("Boost kruskal");
 * - resolve flow, the maximum flow from 0 to n - 1, capacities in
 *   [ 1 , 1000 ]: MaxFlowObserver on the augmentations of Dinic, and with
 *   the warm Preflow of abLEMON as its engine ("ADS reopt Preflow", with
 *   the rewritten headers of ADS::LEMON), against FlowNetwork built and
 *   solved from scratch by Dinic, the Preflow of abLEMON ("LEMON Preflow")
 *   and push_relabel_max_flow() of Boost ("Boost push-relabel");
 * - resolve assign, the assignment of the q = sqrt( n ) rows to the q
 *   columns of random costs in [ 1 , 10^6 ]: Assignment by setCost(),
 *   against a new Assignment and the MinCostAssignment of abLEMON ("LEMON
 *   MinCostAssignment");
 * - resolve mcf, with the rewritten headers of ADS::LEMON, the minimum cost
 *   flow of 1000 units from the node 0 to the node n - 1 of G( n , 4 n ),
 *   costs and capacities in [ 1 , 1000 ], with a ring of arcs of cost 10^4
 *   and unbounded capacity that keeps every change feasible:
 *   MinCostFlowObserver on the NetworkSimplex of abLEMON, told the changes
 *   by its ObservedMaps, against a NetworkSimplex run from scratch ("LEMON
 *   NetworkSimplex").
 *
 * The kinds of change are "up", a cost (a capacity, for the flows) raised
 * by a random amount, "down", lowered to a random value at least 1, and
 * "mixed", set to a random value in the range of the instance, each on a
 * random arc (a random pair of the assignment); on the graphs also
 * "insert", an arc that is not there inserted with a random weight, and
 * "erase", an arc that is there erased (its capacity set to 0 in the
 * networks of the other libraries); on mcf also "capacity", the upper bound
 * of a random arc set to a random value in [ 0 , 1000 ], and "supply", 1
 * to 10 units of supply moved from a random node to another. The sizes of
 * the batches are k = 1 , 4 , 16 , ... up to the arcs m (up to m / 16 for
 * "erase", which must leave half of the arcs), and the case of a cell is
 * "<kind> k=<k>". A cell is the time of one round, the k changes and the
 * solve after them, in nanoseconds: "ADS reopt", the algorithm that keeps
 * its solution, with the default recompute policy [see RatioPolicy of
 * Modification.h], which computes from scratch when the net events exceed
 * a quarter of the arcs; "ADS repair", the same with a policy that never
 * computes from scratch, so that the cost of a repair is seen past the
 * threshold too; "ADS scratch", the algorithm of ADS run from scratch on
 * the graph changed; and the other libraries from scratch. The warm rounds
 * are max( 8 , 1024 / k ), the rounds from scratch the first 8 of the same
 * stream of changes, generated from the seed and the same for all; the
 * building of the instance and the first solve are not timed. The answer
 * of each round is compared with that of the solve from scratch of ADS,
 * and a cell whose answers differ prints -1. For each cell of "ADS reopt"
 * the outcomes of its queries are rows of the benchmark "<bench>:count",
 * one per cause of the outcomes [see Reopt]: "warm", "certified", "scratch
 * by size", "scratch by abort" and "scratch by event" ("warm" and
 * "scratch" for mcf), with "queries", the rounds of the cell, so that the
 * size at which the policy stops repairing is seen next to the size at
 * which a repair stops paying (bench/summary.py --crossover). Each
 * structure runs only if ADS_BENCH_WHO names it, or with no such variable
 * [see Bench.h]; the first structure of each cell is run twice, the second
 * time as "A/A". The contracts are off.
 *
 *   reopt [ -r runs ] [ -s seed ] n1 n2 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/graph/Assignment.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if ( defined( ADS_HAS_LEMON ) || defined( ADS_WITH_LEMON ) ) &&              \
  __has_include( <lemon/preflow.h> )
 #include <lemon/preflow.h>
 #include <lemon/smart_graph.h>
 #define ADS_LEMON_FLOW 1
 #if __has_include( <lemon/bipartite_matching.h> )
  #include <lemon/bipartite_matching.h>
  #define ADS_LEMON_ASSIGN 1
 #endif
 #if defined( ADS_LEMON_SHIM )
  #include "ADS/interop/lemon/Maps.h"
  #include "ADS/interop/lemon/Warm.h"

  #include <lemon/network_simplex.h>
  #define ADS_LEMON_WARM 1
 #endif
#endif

#if __has_include( <boost/graph/dijkstra_shortest_paths.hpp> )
 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/dijkstra_shortest_paths.hpp>
 #include <boost/graph/kruskal_min_spanning_tree.hpp>
 #include <boost/graph/push_relabel_max_flow.hpp>
 #define ADS_BOOST_GRAPH 1
#endif

using namespace ADS;
using ADS::bench::Options;

namespace
{

using LG = ListGraph< long long >;
using Edges = std::vector< std::tuple< int , int , long long > >;
using Counts = std::vector< std::pair< const char * , long long > >;

/// the rounds of the solves from scratch, the first ones of the stream
constexpr std::size_t R0 = 8;

/// the changes of the warm rounds of a cell, at least
constexpr std::size_t SPAN = 1024;

/// a policy value that no count reaches
constexpr double NEVER = std::numeric_limits< double >::infinity();

/// the rounds of the batches of k changes: as many as make SPAN changes,
/// at least R0
std::size_t roundsFor( std::size_t k )
{
 return( std::max( R0 , ( SPAN + k - 1 ) / k ) );
 }

/// the sizes of the batches of a kind on m arcs: 1 , 4 , 16 , ... up to m,
/// up to the size whose rounds erase half of the arcs for "erase"
std::vector< std::size_t > batchSizes( std::size_t m ,
				       const std::string & kind )
{
 std::vector< std::size_t > K;
 for( std::size_t k = 1 ; k <= m ; k *= 4 )
  if( ( kind != "erase" ) || ( roundsFor( k ) * k <= m / 2 ) )
   K.push_back( k );
 return( K );
 }

/// whether one of the structures L is to be measured
bool wanted( const std::vector< std::string > & L )
{
 bool any = false;
 for( const auto & w : L )
  any = ! ADS::bench::skip( w ) || any;
 return( any );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE CHANGES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a change of an edge: its ends, the new weight (the one it had, if
/// erased), and 'w' a weight set, 'i' an insertion, 'e' an erasure
struct Change
{
 int u;
 int v;
 long long w;
 char op;
 };

/// the key of ( u , v ), of [ u , v ] if not directed
std::uint64_t keyOf( int u , int v , bool directed )
{
 if( ! directed && ( v < u ) )
  std::swap( u , v );
 return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) | std::uint32_t( v ) );
 }

/// the edges ( u , v , w ) of G( n , 4 n ), no loop, no repetition of a
/// pair in either order, weights in [ 1 , hi ]
Edges gnm( int n , unsigned seed , long long hi )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 Edges E;
 while( int( E.size() ) < 4 * n ) {
  const int u = int( g() % std::uint64_t( n ) ) ,
	    v = int( g() % std::uint64_t( n ) );
  if( ( u == v ) || ! seen.insert( keyOf( u , v , false ) ).second )
   continue;
  E.emplace_back( u , v ,
		  1 + static_cast< long long >( g() % std::uint64_t( hi ) ) );
  }
 return( E );
 }

/// total changes of the kind on the n nodes and the edges E, weights in
/// [ 1 , hi ], from the seed: each starts from the graph that the previous
/// ones leave, so that an insertion never finds its edge there and an
/// erasure always does
std::vector< Change > stream( int n , bool directed , const Edges & E ,
			      const std::string & kind , std::size_t total ,
			      long long hi , unsigned seed )
{
 std::mt19937_64 g( seed );
 // the edges there, and the place of each in P with its weight
 std::vector< std::uint64_t > P;
 std::unordered_map< std::uint64_t , std::pair< std::size_t , long long > > at;
 for( const auto & [ u , v , w ] : E ) {
  at[ keyOf( u , v , directed ) ] = { P.size() , w };
  P.push_back( keyOf( u , v , directed ) );
  }
 const auto H = std::uint64_t( hi );
 std::vector< Change > C;
 C.reserve( total );
 while( ( C.size() < total ) && ! P.empty() ) {
  if( kind == "insert" ) {
   const int u = int( g() % std::uint64_t( n ) ) ,
	     v = int( g() % std::uint64_t( n ) );
   const std::uint64_t x = keyOf( u , v , directed );
   if( ( u == v ) || at.count( x ) )
    continue;
   const long long w = 1 + static_cast< long long >( g() % H );
   at[ x ] = { P.size() , w };
   P.push_back( x );
   C.push_back( { u , v , w , 'i' } );
   continue;
   }
  const std::size_t i = std::size_t( g() % P.size() );
  const std::uint64_t x = P[ i ];
  const int u = int( x >> 32 ) , v = int( x & 0xffffffffu );
  long long & w = at[ x ].second;
  if( kind == "erase" ) {
   C.push_back( { u , v , w , 'e' } );
   at[ P.back() ].first = i;
   P[ i ] = P.back();
   P.pop_back();
   at.erase( x );
   continue;
   }
  if( kind == "up" )
   w += 1 + static_cast< long long >( g() % H );
  else if( kind == "down" )
   w =
     w > 1 ? 1 + static_cast< long long >( g() % std::uint64_t( w - 1 ) ) : 1;
  else
   w = 1 + static_cast< long long >( g() % H );
  C.push_back( { u , v , w , 'w' } );
  }
 return( C );
 }

/// the change c made to the graph g of ADS, or to its wrapper
template< class S >
void apply( S & g , const Change & c )
{
 if( c.op == 'e' )
  g.deleteEdge( c.u , c.v );
 else if( c.op == 'i' )
  g.insertEdge( c.u , c.v , c.w );
 else if constexpr( requires { g.setWeight( c.u , c.v , c.w ); } )
  ( void )g.setWeight( c.u , c.v , c.w );
 else
  g.insertEdge( c.u , c.v , c.w );  // the edge is there: only its weight
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE CELLS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the answers of the first rounds of k changes of C by the competitor of
/// prepare() and round(), the reference of the others
template< class T , class P , class R >
std::vector< std::uint64_t > reference( const std::vector< T > & C ,
					std::size_t k , std::size_t rounds ,
					P && prepare , R && round )
{
 auto s = prepare();
 std::vector< std::uint64_t > A;
 for( std::size_t i = 0 ; i < rounds ; ++i )
  A.push_back( round( *s , std::span< const T >( C.data() + i * k , k ) ) );
 return( A );
 }

/// a cell of one competitor: prepare() its state, not timed, round( s , D )
/// its answer after the changes D, counts( s ) the outcomes of its queries
/// so far; the time per round of the first `rounds` rounds of k changes of
/// C, -1 if an answer differs from ref, and the outcomes of the rounds of
/// the last run as rows of "<bench>:count"
template< class T , class P , class R , class N >
void cell( const char * bench , const std::string & c , std::size_t n ,
	   const std::string & who , int r , const std::vector< T > & C ,
	   std::size_t k , std::size_t rounds ,
	   const std::vector< std::uint64_t > & ref , P && prepare ,
	   R && round ,
	   N && counts )
{
 if( ADS::bench::skip( who ) )
  return;
 bool bad = false;
 Counts last;
 const double ns = ADS::bench::median(
   prepare ,
   [ & ]( auto & s ) {
    const Counts before = counts( *s );
    std::uint64_t h = 0;
    for( std::size_t i = 0 ; i < rounds ; ++i ) {
     const std::uint64_t a =
       round( *s , std::span< const T >( C.data() + i * k , k ) );
     if( ( i < ref.size() ) && ( a != ref[ i ] ) )
      bad = true;
     h += a;
     }
    ADS::bench::sink = h;
    last = counts( *s );
    for( std::size_t j = 0 ; j < last.size() ; ++j )
     last[ j ].second -= before[ j ].second;
    } ,
   r , rounds );
 if( ! last.empty() && ( who.find( "A/A" ) == std::string::npos ) ) {
  last.emplace_back( "queries" , static_cast< long long >( rounds ) );
  for( const auto & [ name , x ] : last )
   std::printf( "%s:count\t%s\t%zu\t%s %s\t%lld\n" , bench , c.c_str() , n ,
		who.c_str() , name , x );
  }
 ADS::bench::print( bench , c , n , who , bad ? -1.0 : ns );
 }

/// no counts
Counts noCounts( const auto & )
{
 return( Counts() );
 }

/// the counts of the outcomes of an observer, by their causes
template< class O >
Counts causes( const O & o )
{
 return(
   Counts{ { "warm" , o.byCause( Reopt::WARM ) } ,
	   { "certified" , o.byCause( Reopt::CERTIFIED ) } ,
	   { "scratch by size" , o.byCause( Reopt::SCRATCH_BY_SIZE ) } ,
	   { "scratch by abort" , o.byCause( Reopt::SCRATCH_BY_ABORT ) } ,
	   { "scratch by event" , o.byCause( Reopt::SCRATCH_BY_EVENT ) } } );
 }

/// an observer O of a graph and the wrapper that sends it the events
template< class O >
struct Watched
{
 LG g;
 std::unique_ptr< O > o;
 std::unique_ptr< Observable< LG > > og;
 };

/// the graph of the edges E, its observer made by make( g ) and solved
/// once by query( o ); with `repair`, a policy that never computes from
/// scratch
template< class Make , class Query >
auto watched( int n , bool directed , const Edges & E , bool repair ,
	      Make && make ,
	      Query && query )
{
 using O = typename std::decay_t< decltype( make(
   std::declval< LG & >() ) ) >::element_type;
 auto s = std::make_unique< Watched< O > >();
 s->g = LG( n , directed );
 for( const auto & [ u , v , w ] : E )
  s->g.insertEdge( u , v , w );
 s->o = make( s->g );
 if( repair ) {
  s->o->policy().rho = NEVER;
  s->o->policy().alpha = NEVER;
  }
 s->og = std::make_unique< Observable< LG > >( s->g );
 s->og->attach( *s->o );
 ADS::bench::sink = std::uint64_t( query( *s->o ) );
 return( s );
 }

/// the graph of the edges E
std::unique_ptr< LG > plainGraph( int n , bool directed , const Edges & E )
{
 auto g = std::make_unique< LG >( n , directed );
 for( const auto & [ u , v , w ] : E )
  g->insertEdge( u , v , w );
 return( g );
 }

/// the cells of a problem on a graph of n nodes and the edges E: make( g )
/// its observer and query( o ) the answer of the observer, fresh( g ) the
/// answer from scratch on the graph of ADS; more( c , C , k , ref ) the
/// cells of the other competitors, L all the structures
template< class Make , class Query , class Fresh , class More >
void graphProblem( const char * bench , int n , bool directed ,
		   const Edges & E ,
		   const std::vector< std::string > & L , int r ,
		   unsigned seed ,
		   Make && make , Query && query , Fresh && fresh ,
		   More && more )
{
 if( ! wanted( L ) )
  return;
 const std::size_t m = E.size();
 for( const std::string kind : { "up" , "down" , "mixed" , "insert" ,
				 "erase" } )
  for( const std::size_t k : batchSizes( m , kind ) ) {
   const std::size_t rounds = roundsFor( k );
   const auto C = stream( n , directed , E , kind , rounds * k , 1000 ,
			  seed + 17 );
   const std::string c = kind + " k=" + std::to_string( k );
   auto plain = [ & ]( void ) { return( plainGraph( n , directed , E ) ); };
   auto plainRound = [ & ]( LG & g , std::span< const Change > D ) {
    for( const auto & x : D )
     apply( g , x );
    return( std::uint64_t( fresh( g ) ) );
    };
   const auto ref = reference( C , k , R0 , plain , plainRound );
   for( const std::string who :
	{ "ADS reopt" , "ADS reopt A/A" , "ADS repair" } )
    cell(
      bench , c , std::size_t( n ) , who , r , C , k , rounds , ref ,
      [ & ]( void ) {
       return( watched( n , directed , E , who == "ADS repair" , make ,
			query ) );
       } ,
      [ & ]( auto & s , std::span< const Change > D ) {
       {
	auto b = s.og->batch();
	for( const auto & x : D )
	 apply( *s.og , x );
        }
       return( std::uint64_t( query( *s.o ) ) );
       } ,
      [ & ]( const auto & s ) { return( causes( *s.o ) ); } );
   cell( bench , c , std::size_t( n ) , "ADS scratch" , r , C , k , R0 , ref ,
	 plain ,
	 plainRound , []( const auto & s ) { return( noCounts( s ) ); } );
   more( c , C , k , ref );
   }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE COMPETITORS ----------------------------*/
/*--------------------------------------------------------------------------*/

#ifdef ADS_BOOST_GRAPH
/// a graph of Boost, its weights in the edges
template< class D >
using BoostGraph =
  boost::adjacency_list< boost::vecS , boost::vecS , D , boost::no_property ,
			 boost::property< boost::edge_weight_t , long long > >;

/// the graph of Boost of the edges E
template< class B >
std::unique_ptr< B > boostGraph( int n , const Edges & E )
{
 auto g = std::make_unique< B >( n );
 for( const auto & [ u , v , w ] : E )
  boost::add_edge( u , v , w , *g );
 return( g );
 }

/// the change c made to the graph of Boost g
template< class B >
void boostApply( B & g , const Change & c )
{
 if( c.op == 'e' )
  boost::remove_edge( c.u , c.v , g );
 else if( c.op == 'i' )
  boost::add_edge( c.u , c.v , c.w , g );
 else {
  const auto [ e , there ] = boost::edge( c.u , c.v , g );
  if( there )
   boost::put( boost::edge_weight , g , e , c.w );
  }
 }

/// the network of Boost, each arc with its reverse of capacity 0, the
/// lists of the arcs linked, so that an arc added keeps the others
struct BoostNet
{
 using Traits =
   boost::adjacency_list_traits< boost::listS , boost::vecS ,
				 boost::directedS >;
 using G = boost::adjacency_list<
   boost::listS , boost::vecS , boost::directedS , boost::no_property ,
   boost::property<
     boost::edge_capacity_t , long long ,
     boost::property<
       boost::edge_residual_capacity_t , long long ,
       boost::property< boost::edge_reverse_t ,
			Traits::edge_descriptor > > > >;
 G g;
 std::unordered_map< std::uint64_t , Traits::edge_descriptor > arc;
 int s , t;

 BoostNet( int n , const Edges & E , int s0 , int t0 )
     : g( n ) , s( s0 ) , t( t0 ) {
  for( const auto & [ u , v , c ] : E )
   add( u , v , c );
  }

 void add( int u , int v , long long c ) {
  auto cap = boost::get( boost::edge_capacity , g );
  auto rev = boost::get( boost::edge_reverse , g );
  const auto e = boost::add_edge( u , v , g ).first;
  const auto x = boost::add_edge( v , u , g ).first;
  cap[ e ] = c;
  cap[ x ] = 0;
  rev[ e ] = x;
  rev[ x ] = e;
  arc[ keyOf( u , v , true ) ] = e;
  }

 void apply( const Change & c ) {
  const auto it = arc.find( keyOf( c.u , c.v , true ) );
  if( it == arc.end() )
   add( c.u , c.v , c.w );
  else
   boost::get( boost::edge_capacity ,
	       g )[ it->second ] = c.op == 'e' ? 0 : c.w;
  }

 long long run( void ) { return( boost::push_relabel_max_flow( g , s , t ) ); }
 };
#endif

#ifdef ADS_LEMON_FLOW
/// the network of abLEMON, an erased arc kept with capacity 0
struct LemonFlow
{
 lemon::SmartDigraph g;
 lemon::SmartDigraph::ArcMap< long long > cap{ g };
 std::unordered_map< std::uint64_t , lemon::SmartDigraph::Arc > arc;
 lemon::SmartDigraph::Node s , t;

 LemonFlow( int n , const Edges & E , int s0 , int t0 ) {
  for( int i = 0 ; i < n ; ++i )
   g.addNode();
  for( const auto & [ u , v , c ] : E )
   add( u , v , c );
  s = g.nodeFromId( s0 );
  t = g.nodeFromId( t0 );
  }

 void add( int u , int v , long long c ) {
  const auto a = g.addArc( g.nodeFromId( u ) , g.nodeFromId( v ) );
  cap[ a ] = c;
  arc[ keyOf( u , v , true ) ] = a;
  }

 void apply( const Change & c ) {
  const auto it = arc.find( keyOf( c.u , c.v , true ) );
  if( it == arc.end() )
   add( c.u , c.v , c.w );
  else
   cap[ it->second ] = c.op == 'e' ? 0 : c.w;
  }

 long long run( void ) {
  lemon::Preflow< lemon::SmartDigraph ,
		  lemon::SmartDigraph::ArcMap< long long > >
    p( g , cap , s , t );
  p.run();
  return( p.flowValue() );
  }
 };
#endif

#ifdef ADS_LEMON_ASSIGN
/// the complete bipartite graph of the assignment in abLEMON, for its
/// MinCostAssignment
struct LemonBp
{
 lemon::SmartBpGraph g;
 lemon::SmartBpGraph::EdgeMap< long long > cost{ g };
 std::vector< lemon::SmartBpGraph::Edge > edge;

 LemonBp( int q , const std::vector< long long > & C ) {
  std::vector< lemon::SmartBpGraph::RedNode > R;
  std::vector< lemon::SmartBpGraph::BlueNode > B;
  for( int i = 0 ; i < q ; ++i ) {
   R.push_back( g.addRedNode() );
   B.push_back( g.addBlueNode() );
   }
  for( int i = 0 ; i < q ; ++i )
   for( int j = 0 ; j < q ; ++j ) {
    edge.push_back(
      g.addEdge( R[ std::size_t( i ) ] , B[ std::size_t( j ) ] ) );
    cost[ edge.back() ] =
      C[ std::size_t( i ) * std::size_t( q ) + std::size_t( j ) ];
    }
  }

 long long run( void ) {
  lemon::MinCostAssignment< lemon::SmartBpGraph ,
			    lemon::SmartBpGraph::EdgeMap< long long > >
    a( g , cost );
  a.run();
  return( a.totalCost() );
  }
 };
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- THE PROBLEMS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an answer of a distance: the distance, a mark if not reachable
std::uint64_t distanceOf( long long d )
{
 return( d == infinity< long long >() ? std::uint64_t( 0x9e3779b9u )
				       : std::uint64_t( d ) );
 }

/// the cells of the shortest paths
void sssp( int n , int r , unsigned seed )
{
 const auto E = gnm( n , seed , 1000 );
 std::vector< int > T;  // the nodes whose distances are the answer
 for( int j = 1 ; j <= 8 ; ++j )
  T.push_back( int( std::int64_t( j ) * n / 9 ) );
 std::vector< std::string > L = { "ADS reopt" , "ADS reopt A/A" ,
				  "ADS repair" ,
				  "ADS scratch" };
#ifdef ADS_BOOST_GRAPH
 L.emplace_back( "Boost dijkstra" );
#endif
 graphProblem(
   "resolve sssp" , n , true , E , L , r , seed ,
   []( LG & g ) {
    return( std::make_unique< ShortestPathsObserver< LG > >( g , 0 ) );
    } ,
   [ & ]( auto & o ) {
    std::uint64_t h = 0;
    for( int t : T )
     h += distanceOf( o.distance( t ) );
    return( h );
    } ,
   [ & ]( const LG & g ) {
    const auto R = shortestPaths( g , 0 , Frontier::HEAP );
    std::uint64_t h = 0;
    for( int t : T )
     h += distanceOf( R.d[ std::size_t( t ) ] );
    return( h );
    } ,
   [ & ]( const std::string & c , const std::vector< Change > & C ,
	  std::size_t k , const std::vector< std::uint64_t > & ref ) {
#ifdef ADS_BOOST_GRAPH
    using B = BoostGraph< boost::directedS >;
    cell(
      "resolve sssp" , c , std::size_t( n ) , "Boost dijkstra" , r , C , k ,
      R0 , ref ,
      [ & ]( void ) { return( boostGraph< B >( n , E ) ); } ,
      [ & ]( B & g , std::span< const Change > D ) {
       for( const auto & x : D )
	boostApply( g , x );
       std::vector< long long > d( boost::num_vertices( g ) );
       boost::dijkstra_shortest_paths(
	 g , boost::vertex( 0 , g ) ,
	 boost::distance_map( boost::make_iterator_property_map(
	   d.begin() , boost::get( boost::vertex_index , g ) ) ) );
       std::uint64_t h = 0;
       for( int t : T )
	h += distanceOf( d[ std::size_t( t ) ] );
       return( h );
       } ,
      []( const auto & s ) { return( noCounts( s ) ); } );
#else
    ( void )c;
    ( void )C;
    ( void )k;
    ( void )ref;
#endif
    } );
 }

/// the cells of the minimum spanning forest
void mst( int n , int r , unsigned seed )
{
 const auto E = gnm( n , seed , 1000 );
 std::vector< std::string > L = { "ADS reopt" , "ADS reopt A/A" ,
				  "ADS repair" ,
				  "ADS scratch" };
#ifdef ADS_BOOST_GRAPH
 L.emplace_back( "Boost kruskal" );
#endif
 graphProblem(
   "resolve mst" , n , false , E , L , r , seed ,
   []( LG & g ) {
    return( std::make_unique< SpanningForestObserver< LG > >( g ) );
    } ,
   []( auto & o ) { return( o.weight() ); } ,
   []( const LG & g ) {
    long long s = 0;
    for( const auto & e : kruskal( g ) )
     s += e.w;
    return( s );
    } ,
   [ & ]( const std::string & c , const std::vector< Change > & C ,
	  std::size_t k , const std::vector< std::uint64_t > & ref ) {
#ifdef ADS_BOOST_GRAPH
    using B = BoostGraph< boost::undirectedS >;
    cell(
      "resolve mst" , c , std::size_t( n ) , "Boost kruskal" , r , C , k , R0 ,
      ref ,
      [ & ]( void ) { return( boostGraph< B >( n , E ) ); } ,
      [ & ]( B & g , std::span< const Change > D ) {
       for( const auto & x : D )
	boostApply( g , x );
       std::vector< boost::graph_traits< B >::edge_descriptor > F;
       boost::kruskal_minimum_spanning_tree( g , std::back_inserter( F ) );
       long long s = 0;
       for( const auto & e : F )
	s += boost::get( boost::edge_weight , g , e );
       return( std::uint64_t( s ) );
       } ,
      []( const auto & s ) { return( noCounts( s ) ); } );
#else
    ( void )c;
    ( void )C;
    ( void )k;
    ( void )ref;
#endif
    } );
 }

/// the cells of the maximum flow
void flow( int n , int r , unsigned seed )
{
 const auto E = gnm( n , seed , 1000 );
 std::vector< std::string > L = { "ADS reopt" , "ADS reopt A/A" ,
				  "ADS repair" ,
				  "ADS scratch" };
#ifdef ADS_LEMON_WARM
 L.emplace_back( "ADS reopt Preflow" );
#endif
#ifdef ADS_LEMON_FLOW
 L.emplace_back( "LEMON Preflow" );
#endif
#ifdef ADS_BOOST_GRAPH
 L.emplace_back( "Boost push-relabel" );
#endif
 auto value = []( auto & o ) { return( o.value() ); };
 graphProblem(
   "resolve flow" , n , true , E , L , r , seed ,
   [ n ]( LG & g ) {
    return( std::make_unique< MaxFlowObserver< LG > >( g , 0 , n - 1 ,
							Augment::DINIC ) );
    } ,
   value ,
   [ n ]( const LG & g ) {
    FlowNetwork N( g.bound() );
    for( int u : g.V() )
     g.forEachEdge( u ,
		    [ & ]( int v , long long w ) { N.addEdge( u , v , w ); } );
    return( N.maxFlow( 0 , n - 1 , Augment::DINIC ) );
    } ,
   [ & ]( const std::string & c , const std::vector< Change > & C ,
	  std::size_t k , const std::vector< std::uint64_t > & ref ) {
    const char * b = "resolve flow";
    const std::size_t N = std::size_t( n );
    ( void )b;
    ( void )N;
#ifdef ADS_LEMON_WARM
    using PF =
      MaxFlowObserver< LG , RatioPolicy , lemon_interop::PreflowEngine >;
    cell(
      b , c , N , "ADS reopt Preflow" , r , C , k , roundsFor( k ) , ref ,
      [ & ]( void ) {
       return( watched(
	 n , true , E , false ,
	 [ n ]( LG & g ) { return( std::make_unique< PF >( g , 0 ,
							   n - 1 ) ); } ,
	 value ) );
       } ,
      [ & ]( auto & s , std::span< const Change > D ) {
       {
	auto bt = s.og->batch();
	for( const auto & x : D )
	 apply( *s.og , x );
        }
       return( std::uint64_t( s.o->value() ) );
       } ,
      [ & ]( const auto & s ) { return( causes( *s.o ) ); } );
#endif
#ifdef ADS_LEMON_FLOW
    cell(
      b , c , N , "LEMON Preflow" , r , C , k , R0 , ref ,
      [ & ]( void ) {
       return( std::make_unique< LemonFlow >( n , E , 0 , n - 1 ) );
       } ,
      []( LemonFlow & F , std::span< const Change > D ) {
       for( const auto & x : D )
	F.apply( x );
       return( std::uint64_t( F.run() ) );
       } ,
      []( const auto & s ) { return( noCounts( s ) ); } );
#endif
#ifdef ADS_BOOST_GRAPH
    cell(
      b , c , N , "Boost push-relabel" , r , C , k , R0 , ref ,
      [ & ]( void ) {
       return( std::make_unique< BoostNet >( n , E , 0 , n - 1 ) );
       } ,
      []( BoostNet & F , std::span< const Change > D ) {
       for( const auto & x : D )
	F.apply( x );
       return( std::uint64_t( F.run() ) );
       } ,
      []( const auto & s ) { return( noCounts( s ) ); } );
#endif
    ( void )c;
    ( void )C;
    ( void )k;
    ( void )ref;
    } );
 }

/// the cells of the assignment of q = sqrt( n ) rows
void assign( int n , int r , unsigned seed )
{
 std::vector< std::string > L = { "ADS reopt" , "ADS reopt A/A" ,
				  "ADS repair" ,
				  "ADS scratch" };
#ifdef ADS_LEMON_ASSIGN
 L.emplace_back( "LEMON MinCostAssignment" );
#endif
 if( ! wanted( L ) )
  return;
 const int q = std::max( 4 , int( std::sqrt( double( n ) ) ) );
 const std::size_t Q = std::size_t( q );
 std::mt19937_64 g( seed + 9 );
 std::vector< long long > C0( Q * Q );
 Edges E;  // the pairs ( i , j , c ), as the arcs of the changes
 for( std::size_t i = 0 ; i < Q ; ++i )
  for( std::size_t j = 0 ; j < Q ; ++j ) {
   C0[ i * Q + j ] = 1 + static_cast< long long >( g() % 1000000 );
   E.emplace_back( int( i ) , int( j ) , C0[ i * Q + j ] );
   }
 const char * b = "resolve assign";
 for( const std::string kind : { "up" , "down" , "mixed" } )
  for( const std::size_t k : batchSizes( E.size() , kind ) ) {
   const std::size_t rounds = roundsFor( k );
   const auto C = stream( q , true , E , kind , rounds * k , 1000000 ,
			  seed + 19 );
   const std::string c = kind + " k=" + std::to_string( k );
   auto plain = [ & ]( void ) {
    return( std::make_unique< std::vector< long long > >( C0 ) );
    };
   auto plainRound = [ & ]( std::vector< long long > & D ,
			    std::span< const Change > X ) {
    for( const auto & x : X )
     D[ std::size_t( x.u ) * Q + std::size_t( x.v ) ] = x.w;
    return( std::uint64_t( Assignment<>( q , D ).total() ) );
    };
   const auto ref = reference( C , k , R0 , plain , plainRound );
   for( const std::string who :
	{ "ADS reopt" , "ADS reopt A/A" , "ADS repair" } )
    cell(
      b , c , std::size_t( n ) , who , r , C , k , rounds , ref ,
      [ & ]( void ) {
       auto A = std::make_unique< Assignment<> >( q , C0 );
       if( who == "ADS repair" ) {
	A->policy().rho = NEVER;
	A->policy().alpha = NEVER;
        }
       ADS::bench::sink = std::uint64_t( A->total() );
       return( A );
       } ,
      []( Assignment<> & A , std::span< const Change > X ) {
       for( const auto & x : X )
	A.setCost( x.u , x.v , x.w );
       return( std::uint64_t( A.total() ) );
       } ,
      []( const Assignment<> & A ) { return( causes( A ) ); } );
   cell( b , c , std::size_t( n ) , "ADS scratch" , r , C , k , R0 , ref ,
	 plain ,
	 plainRound , []( const auto & s ) { return( noCounts( s ) ); } );
#ifdef ADS_LEMON_ASSIGN
   cell(
     b , c , std::size_t( n ) , "LEMON MinCostAssignment" , r , C , k , R0 ,
     ref ,
     [ & ]( void ) { return( std::make_unique< LemonBp >( q , C0 ) ); } ,
     [ & ]( LemonBp & A , std::span< const Change > X ) {
      for( const auto & x : X )
       A.cost[ A.edge[ std::size_t( x.u ) * Q + std::size_t( x.v ) ] ] = x.w;
      return( std::uint64_t( A.run() ) );
      } ,
     []( const auto & s ) { return( noCounts( s ) ); } );
#endif
   }
 }

#ifdef ADS_LEMON_WARM

/// the digraph of the minimum cost flows, its maps and its algorithm
using MG = lemon::SmartDigraph;
using MV = MG::ArcMap< long long >;
using MS = MG::NodeMap< long long >;
using NS = lemon::NetworkSimplex< MG , long long , long long >;

/// a change of a network: 'c' the cost of the arc x set to d, 'u' its upper
/// bound, 's' d units of supply moved from the node x to the node y
struct NetChange
{
 int x;
 int y;
 long long d;
 char op;
 };

/// the units of supply from the node 0 to the node n - 1
constexpr long long UNITS = 1000;

/// a network of the minimum cost flows: the arcs of E with their costs and
/// random upper bounds in [ 1 , 1000 ], then the ring of the arcs
/// ( i , i + 1 mod n ) of cost 10^4 and capacity 10^12
struct Net
{
 MG g;
 MV cost{ g };
 MV upper{ g };
 MS supply{ g };

 Net( int n , const Edges & E , unsigned seed ) {
  std::mt19937_64 rng( seed );
  for( int i = 0 ; i < n ; ++i )
   supply.set( g.addNode() , 0 );
  for( const auto & [ u , v , c ] : E ) {
   const auto a = g.addArc( g.nodeFromId( u ) , g.nodeFromId( v ) );
   cost.set( a , c );
   upper.set( a , 1 + static_cast< long long >( rng() % 1000 ) );
   }
  for( int i = 0 ; i < n ; ++i ) {
   const auto a = g.addArc( g.nodeFromId( i ) ,
			    g.nodeFromId( ( i + 1 ) % n ) );
   cost.set( a , 10000 );
   upper.set( a , 1000000000000LL );
   }
  supply.set( g.nodeFromId( 0 ) , UNITS );
  supply.set( g.nodeFromId( n - 1 ) , -UNITS );
  }

 /// the change x made to the maps
 void apply( const NetChange & x ) {
  if( x.op == 'c' )
   cost.set( g.arcFromId( x.x ) , x.d );
  else if( x.op == 'u' )
   upper.set( g.arcFromId( x.x ) , x.d );
  else {
   const auto a = g.nodeFromId( x.x ) , b = g.nodeFromId( x.y );
   supply.set( a , supply[ a ] + x.d );
   supply.set( b , supply[ b ] - x.d );
   }
  }
 };

/// total changes of the kind on the network of n nodes and of the arcs E
/// (the first arcs of the network), from the seed
std::vector< NetChange > netStream( int n , const Edges & E ,
				    const std::string & kind ,
				    std::size_t total , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< long long > c( E.size() );
 for( std::size_t a = 0 ; a < E.size() ; ++a )
  c[ a ] = std::get< 2 >( E[ a ] );
 std::vector< NetChange > C;
 C.reserve( total );
 while( C.size() < total ) {
  if( kind == "supply" ) {
   const int x = int( g() % std::uint64_t( n ) ) ,
	     y = int( g() % std::uint64_t( n ) );
   if( x != y )
    C.push_back( { x , y , 1 + static_cast< long long >( g() % 10 ) , 's' } );
   continue;
   }
  const std::size_t a = std::size_t( g() % E.size() );
  if( kind == "capacity" ) {
   C.push_back( { int( a ) , 0 , static_cast< long long >( g() % 1001 ) ,
		  'u' } );
   continue;
   }
  long long & w = c[ a ];
  if( kind == "up" )
   w += 1 + static_cast< long long >( g() % 1000 );
  else if( kind == "down" )
   w =
     w > 1 ? 1 + static_cast< long long >( g() % std::uint64_t( w - 1 ) ) : 1;
  else
   w = 1 + static_cast< long long >( g() % 1000 );
  C.push_back( { int( a ) , 0 , w , 'c' } );
  }
 return( C );
 }

/// the answer of a minimum cost flow: its cost, a mark if none
template< class A >
std::uint64_t costOf( const A & a , typename A::ProblemType p )
{
 return( p == A::OPTIMAL ? std::uint64_t( a.totalCost() )
			  : std::uint64_t( 0x9e3779b9u ) );
 }

/// the observer of a network and the maps that tell it the changes
struct NetWatched
{
 using O = lemon_interop::MinCostFlowObserver< MG , NS , MV , MV , MS >;
 Net net;
 O o;
 lemon_interop::ObservedMap< MG , MV , mod::Cost > oc;
 lemon_interop::ObservedMap< MG , MV , mod::Upper > ou;
 lemon_interop::ObservedMap< MG , MS , mod::Supply > os;

 NetWatched( int n , const Edges & E , unsigned seed )
     : net( n , E , seed ) , o( net.g , net.cost , net.upper , net.supply ) ,
       oc( net.g , net.cost ) , ou( net.g , net.upper ) ,
       os( net.g , net.supply ) {
  oc.attach( o );
  ou.attach( o );
  os.attach( o );
  ADS::bench::sink = costOf( o.algorithm() , o.solve() );
  }

 /// the change x told to the observer through the maps
 void apply( const NetChange & x ) {
  if( x.op == 'c' )
   oc.set( net.g.arcFromId( x.x ) , x.d );
  else if( x.op == 'u' )
   ou.set( net.g.arcFromId( x.x ) , x.d );
  else {
   const auto a = net.g.nodeFromId( x.x ) , b = net.g.nodeFromId( x.y );
   os.set( a , net.supply[ a ] + x.d );
   os.set( b , net.supply[ b ] - x.d );
   }
  }
 };

/// the cells of the minimum cost flows
void mcf( int n , int r , unsigned seed )
{
 const std::vector< std::string > L = { "ADS reopt" , "ADS reopt A/A" ,
					"LEMON NetworkSimplex" };
 if( ! wanted( L ) )
  return;
 const auto E = gnm( n , seed , 1000 );
 const char * b = "resolve mcf";
 for( const std::string kind :
      { "up" , "down" , "mixed" , "capacity" , "supply" } )
  for( const std::size_t k : batchSizes( E.size() , kind ) ) {
   const std::size_t rounds = roundsFor( k );
   const auto C = netStream( n , E , kind , rounds * k , seed + 23 );
   const std::string c = kind + " k=" + std::to_string( k );
   auto plain = [ & ]( void ) {
    return( std::make_unique< Net >( n , E , seed + 29 ) );
    };
   auto plainRound = [ & ]( Net & N , std::span< const NetChange > X ) {
    for( const auto & x : X )
     N.apply( x );
    NS a( N.g );
    a.costMap( N.cost ).upperMap( N.upper ).supplyMap( N.supply );
    const auto p = a.run();
    return( costOf( a , p ) );
    };
   const auto ref = reference( C , k , R0 , plain , plainRound );
   for( const std::string who : { "ADS reopt" , "ADS reopt A/A" } )
    cell(
      b , c , std::size_t( n ) , who , r , C , k , rounds , ref ,
      [ & ]( void ) {
       return( std::make_unique< NetWatched >( n , E , seed + 29 ) );
       } ,
      []( NetWatched & W , std::span< const NetChange > X ) {
       for( const auto & x : X )
	W.apply( x );
       const auto p = W.o.solve();
       return( costOf( W.o.algorithm() , p ) );
       } ,
      []( const NetWatched & W ) {
       return(
	 Counts{ { "warm" , W.o.warms } , { "scratch" , W.o.scratches } } );
       } );
   cell( b , c , std::size_t( n ) , "LEMON NetworkSimplex" , r , C , k , R0 ,
	 ref ,
	 plain , plainRound ,
	 []( const auto & s ) { return( noCounts( s ) ); } );
   }
 }

#endif

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 10000 , 100000 } );
 for( std::size_t n0 : o.sizes ) {
  const int n = int( n0 );
  sssp( n , o.runs , o.seed );
  mst( n , o.runs , o.seed );
  flow( n , o.runs , o.seed );
  assign( n , o.runs , o.seed );
#ifdef ADS_LEMON_WARM
  mcf( n , o.runs , o.seed );
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File reopt.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
