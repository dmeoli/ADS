/*--------------------------------------------------------------------------*/
/*------------------------------ File Warm.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The warm starts of the algorithms of abLEMON and the solvers of ADS that
 * use them, beyond the book (see \ref theory_graph_lemon). abLEMON has no
 * warm start of its minimum cost flows and of its maximum flow: they are
 * written, with `ADS_WITH_LEMON`, into private copies of six of its
 * headers (`network_simplex.h`, `cost_scaling.h`, `cycle_canceling.h`,
 * `capacity_scaling.h`, `bits/mcf_feasible_flow.h`, `preflow.h`), rewritten
 * at configure time by the rules of `ADS/interop/lemon/shim/rewrites.txt`
 * and put on the include path before abLEMON, which defines `ADS_LEMON_SHIM`.
 * Each rule is `header|anchor|what`, the anchor a whole line of the header
 * of abLEMON that must be there exactly once, or the rewrite stops, and
 * `what` a block inserted before the line, a text appended to it or a line
 * in its place. They add:
 *
 * - `runWarm()` to `NetworkSimplex`, from the basis of the last run, whose
 *   potentials are computed again after a change of the costs and whose
 *   tree flow is computed again after a change of the upper bounds or of
 *   the supplies, a subtree whose flow no longer fits being hung from the
 *   root by its artificial arc; to `CostScaling` and `CycleCanceling`, from
 *   the flow of the last run put within the new bounds and made to meet the
 *   new supplies by `feasibleFlow()`, which starts from it, the scaling of
 *   the first starting from the largest violation of the optimality
 *   conditions under the potentials of the last run; to `CapacityScaling`,
 *   from the flow of the last run at the bound that its reduced cost under
 *   the last potentials says, the excess sent by the shortest augmenting
 *   paths. A change of the lower bounds, of the supply type, of the digraph
 *   or of all the parameters drops the warm start, and `runWarm()` then
 *   does what `run()` does;
 * - `warmCost( a , c )`, `warmUpper( a , u )` and `warmSupply( n , s )`, the
 *   changes of one arc or of one node in \f$ O( 1 ) \f$ time, which the map
 *   setters do in \f$ \Theta( m ) \f$ by a scan of the arcs, and
 *   `warmReady()`, whether `runWarm()` has a last run to start from;
 * - `initWarm( flow )` and `runWarm()` to `Preflow`: the flow given (the
 *   last one, for `runWarm()`) put within the capacities, and the deficit
 *   left at a node other than the source, after a capacity fell below the
 *   flow of its arc, moved forward along the arcs that carry flow out of
 *   it, until only the source has one; the excesses left at the tails are
 *   those that the two phases send on. `init( flow )` of abLEMON refuses such
 *   a flow.
 *
 * A program must see one rewritten copy of each of these headers: two
 * different rewrites of one header in one binary give two layouts of one
 * class, a violation of the one definition rule that compilers do not
 * diagnose. The same holds for a C++20 module of abLEMON, which must be built
 * from the rewritten headers.
 *
 * On these, the solvers of ADS that keep a solution of abLEMON warm:
 *
 * - `PreflowEngine`, the engine of `MaxFlowObserver` [see
 *   `Reoptimization.h`] on the `Preflow` of abLEMON: the network of ADS
 *   copied into a `SmartDigraph` once, its new edges appended, the
 *   capacities and the flow of the network read in \f$ \Theta( m ) \f$, a
 *   warm run, and the flow written back by `FlowNetwork::assignFlow()`. The
 *   push-relabel of ADS stays the default engine of the observer;
 * - `NetworkSimplexAssignment< W >`, an engine of `Assignment` [see
 *   `Assignment.h`] on the `NetworkSimplex` of abLEMON, the rows of supply 1,
 *   the columns of demand 1 and an arc for each finite cost: after changes
 *   of finite costs alone, one `warmCost()` per pair and `runWarm()`, and a
 *   new solve when a pair is forbidden or allowed again; the potentials of
 *   abLEMON, \f$ c( i , j ) + \pi( r_i ) - \pi( b_j ) \ge 0 \f$, give
 *   \f$ u_i = - \pi( r_i ) \f$ and \f$ v_j = \pi( b_j ) \f$;
 * - `MinCostFlowObserver< GR , A , CM , UM , SM >`, a minimum cost flow of a
 *   digraph of abLEMON by the algorithm `A` (`NetworkSimplex` by default, or
 *   `CostScaling`, `CycleCanceling`, `CapacityScaling`), kept up to date as
 *   an observer of the `ObservedMap`s of its costs, of its upper bounds and
 *   of its supplies (the attributes `mod::Cost`, `mod::Upper`,
 *   `mod::Supply`) [see `Maps.h`], each change given to the algorithm by
 *   the setter of its arc or of its node, and of a `LemonTap` of its arcs
 *   or of its nodes [see `Observers.h`], after whose events the algorithm
 *   is reset; `solve()` runs `runWarm()`, or `run()` after a reset, and
 *   counts both.
 *
 * The contracts [see `Contract.h`]: those of abLEMON; the digraph of a
 * `MinCostFlowObserver` changes only through the taps and the maps
 * observed. The classes are defined in this file.
 *
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
 * \see A. Frangioni, A. Manca, A computational study of cost
 *      reoptimization for min-cost flow problems, INFORMS Journal on
 *      Computing 18(1), 61-70, 2006, doi:10.1287/ijoc.1040.0081
 * \see A. V. Goldberg, R. E. Tarjan, A new approach to the maximum-flow
 *      problem, Journal of the ACM 35(4), 921-940, 1988,
 *      doi:10.1145/48014.61051
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_interop_lemon_Warm
 #define __ADS_interop_lemon_Warm
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! defined( ADS_LEMON_SHIM )
 #error "the warm start of LEMON needs its rewritten headers (ADS_LEMON_SHIM)"
#endif

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"

#include <lemon/core.h>
#include <lemon/network_simplex.h>
#include <lemon/preflow.h>
#include <lemon/smart_graph.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PreflowEngine ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the engine of `MaxFlowObserver` on the `Preflow` of abLEMON, with its warm
/// start from the flow of the network
class PreflowEngine
{

/*--------------------------------------------------------------------------*/
 public:
 /// the value of a maximum flow of `net` from the zero flow, by a new
 /// `Preflow` on a copy of `net`
 long long scratch( FlowNetwork & net , int s , int t , Augment ) {
  build( net , s , t );
  f_pf->run();
  ++scratches;
  return( giveBack( net ) );
  }

 /// the value of a maximum flow of `net` from its flow, by `runWarm()` of
 /// the `Preflow`, or from scratch if the nodes or the ends changed
 long long warm( FlowNetwork & net , int s , int t , Augment how ,
		 long long ) {
  if( ! f_pf || ( net.n() != f_n ) || ( s != f_s ) || ( t != f_t ) )
   return( scratch( net , s , t , how ) );
  for( int e = 2 * int( f_arc.size() ) ; e < net.places() ; e += 2 )
   f_arc.push_back( f_g.addArc( f_g.nodeFromId( net.tail( e ) ) ,
				f_g.nodeFromId( net.head( e ) ) ) );
  for( std::size_t k = 0 ; k < f_arc.size() ; ++k ) {
   f_cap.set( f_arc[ k ] , net.capacity( int( 2 * k ) ) );
   f_flow.set( f_arc[ k ] , net.flow( int( 2 * k ) ) );
   }
  f_pf->runWarm();
  ++warms;
  return( giveBack( net ) );
  }

 int scratches = 0;  ///< the solves from scratch
 int warms = 0;      ///< the warm ones

/*--------------------------------------------------------------------------*/
 private:
 using G = ::lemon::SmartDigraph;       ///< the copy of the network
 using CM = G::ArcMap< long long >;     ///< its capacities and flow
 using PF = ::lemon::Preflow< G , CM >;  ///< the algorithm

 /// the copy of `net`, and a new `Preflow` from s to t
 void build( const FlowNetwork & net , int s , int t ) {
  f_pf.reset();
  f_g.clear();
  f_arc.clear();
  f_n = net.n();
  f_s = s;
  f_t = t;
  f_g.reserveNode( f_n );
  for( int u = 0 ; u < f_n ; ++u )
   f_g.addNode();
  for( int e = 0 ; e < net.places() ; e += 2 ) {
   f_arc.push_back( f_g.addArc( f_g.nodeFromId( net.tail( e ) ) ,
				f_g.nodeFromId( net.head( e ) ) ) );
   f_cap.set( f_arc.back() , net.capacity( e ) );
   }
  f_pf = std::make_unique< PF >( f_g , f_cap , f_g.nodeFromId( s ) ,
				 f_g.nodeFromId( t ) );
  f_pf->flowMap( f_flow );
  }

 /// the flow of abLEMON written into `net`, and its value
 long long giveBack( FlowNetwork & net ) const {
  std::vector< long long > F( f_arc.size() );
  for( std::size_t k = 0 ; k < f_arc.size() ; ++k )
   F[ k ] = f_flow[ f_arc[ k ] ];
  net.assignFlow( F );
  return( f_pf->flowValue() );
  }

 G f_g;                        ///< the copy of the network
 CM f_cap{ f_g };              ///< its capacities
 CM f_flow{ f_g };             ///< its flow
 std::vector< G::Arc > f_arc;  ///< the arc of each edge of the network
 std::unique_ptr< PF > f_pf;   ///< the algorithm
 int f_n = -1;                 ///< the nodes of the copy
 int f_s = -1;                 ///< the source
 int f_t = -1;                 ///< the sink

 };  // end( class PreflowEngine )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS NetworkSimplexAssignment ---------------------*/
/*--------------------------------------------------------------------------*/

/// an engine of `Assignment` on the `NetworkSimplex` of abLEMON, warm after
/// changes of finite costs
template< class W = long long >
class NetworkSimplexAssignment
{

/*--------------------------------------------------------------------------*/
 public:
 /// an optimal assignment of the costs \f$ C \f$ and its potentials [see
 /// `AssignmentEngine`]
 int solve( int n , const std::vector< W > & C , bool all ,
	    const std::vector< std::pair< int , int > > & D ,
	    std::vector< int > & col , std::vector< W > & u ,
	    std::vector< W > & v ) {
  const W INF = infinity< W >();
  const auto at = [ n ]( int i , int j ) {
   return( std::size_t( i ) * std::size_t( n ) + std::size_t( j ) );
   };
  bool cold = all || ! f_ns || ( n != f_n ) || ! f_ns->warmReady();
  if( ! cold )  // a pair forbidden or allowed again changes the digraph
   for( const auto & [ i , j ] : D )
    if( ( C[ at( i , j ) ] != INF ) != ( f_arc[ at( i , j ) ] >= 0 ) ) {
     cold = true;
     break;
     }
  typename NS::ProblemType r;
  if( cold ) {
   build( n , C );
   r = f_ns->run();
   ++scratches;
   }
  else {
   for( const auto & [ i , j ] : D )
    if( C[ at( i , j ) ] != INF )
     f_ns->warmCost( f_g.arcFromId( f_arc[ at( i , j ) ] ) ,
		     C[ at( i , j ) ] );
   r = f_ns->runWarm();
   ++warms;
   }
  if( r != NS::OPTIMAL ) {
   f_ns.reset();  // a new solve next time
   return( -1 );
   }
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    const int a = f_arc[ at( i , j ) ];
    if( ( a >= 0 ) && ( f_ns->flow( f_g.arcFromId( a ) ) > 0 ) )
     col[ std::size_t( i ) ] = j;
    }
  for( int i = 0 ; i < n ; ++i ) {
   u[ std::size_t( i ) ] = -f_ns->potential( f_g.nodeFromId( i ) );
   v[ std::size_t( i ) ] = f_ns->potential( f_g.nodeFromId( n + i ) );
   }
  return( cold ? Reopt::SCRATCH_BY_EVENT : Reopt::WARM );
  }

 int scratches = 0;  ///< the solves from scratch
 int warms = 0;      ///< the warm ones

/*--------------------------------------------------------------------------*/
 private:
 using G = ::lemon::SmartDigraph;                  ///< the network
 using NS = ::lemon::NetworkSimplex< G , int , W >;  ///< the algorithm

 /// the network of the costs \f$ C \f$ and a new algorithm on it
 void build( int n , const std::vector< W > & C ) {
  const W INF = infinity< W >();
  f_ns.reset();
  f_g.clear();
  f_n = n;
  f_arc.assign( std::size_t( n ) * std::size_t( n ) , -1 );
  for( int k = 0 ; k < 2 * n ; ++k )
   f_g.addNode();
  G::ArcMap< W > cost( f_g );
  G::NodeMap< int > supply( f_g );
  for( int i = 0 ; i < n ; ++i ) {
   supply.set( f_g.nodeFromId( i ) , 1 );
   supply.set( f_g.nodeFromId( n + i ) , -1 );
   for( int j = 0 ; j < n ; ++j ) {
    const std::size_t k =
      std::size_t( i ) * std::size_t( n ) + std::size_t( j );
    if( C[ k ] == INF )
     continue;
    const G::Arc a =
      f_g.addArc( f_g.nodeFromId( i ) , f_g.nodeFromId( n + j ) );
    cost.set( a , C[ k ] );
    f_arc[ k ] = f_g.id( a );
    }
   }
  f_ns = std::make_unique< NS >( f_g );
  f_ns->costMap( cost ).supplyMap( supply );
  }

 G f_g;                       ///< the network
 std::vector< int > f_arc;    ///< the arc of each pair, -1 if forbidden
 std::unique_ptr< NS > f_ns;  ///< the algorithm
 int f_n = -1;                ///< the rows

 };  // end( class NetworkSimplexAssignment )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MinCostFlowObserver -----------------------*/
/*--------------------------------------------------------------------------*/

/// a minimum cost flow of a digraph of abLEMON by the algorithm `A`, kept up
/// to date as an observer of the maps of its costs, upper bounds and
/// supplies, and of the taps of its arcs and nodes
template< class GR , class A = ::lemon::NetworkSimplex< GR > ,
	  class CM = typename GR::template ArcMap< typename A::Cost > ,
	  class UM = typename GR::template ArcMap< typename A::Value > ,
	  class SM = typename GR::template NodeMap< typename A::Value > >
class MinCostFlowObserver
{

/*--------------------------------------------------------------------------*/
 public:
 using Cost = typename A::Cost;    ///< the type of the costs
 using Value = typename A::Value;  ///< the type of the flows

 /// the minimum cost flow of \f$ g \f$ with the costs \f$ c \f$, the upper
 /// bounds \f$ u \f$ and the supplies \f$ s \f$ (the maps below the
 /// `ObservedMap`s), computed at the first `solve()`; all must outlive it
 MinCostFlowObserver( const GR & g , const CM & c , const UM & u ,
		      const SM & s )
     : f_g( g ) , f_c( c ) , f_u( u ) , f_s( s ) , f_a( g ) {
  load();
  }

 /// a cost changed, given to the arc
 template< class V >
 void on( const mod::Changed< mod::Cost , int , V > & e ) {
  if( ! f_reset )
   f_a.warmCost( f_g.arcFromId( e.h ) , Cost( e.to ) );
  }

 /// an upper bound changed, given to the arc
 template< class V >
 void on( const mod::Changed< mod::Upper , int , V > & e ) {
  if( ! f_reset )
   f_a.warmUpper( f_g.arcFromId( e.h ) , Value( e.to ) );
  }

 /// a supply changed, given to the node
 template< class V >
 void on( const mod::Changed< mod::Supply , int , V > & e ) {
  if( ! f_reset )
   f_a.warmSupply( f_g.nodeFromId( e.h ) , Value( e.to ) );
  }

 /// an arc or a node inserted: the algorithm is reset at the next solve
 template< class K , class V >
 void on( const mod::Inserted< int , K , V > & ) {
  f_reset = true;
  }

 /// an arc or a node erased: the same
 template< class K , class V >
 void on( const mod::Erased< int , K , V > & ) noexcept {
  f_reset = true;
  }

 /// the digraph cleared: the same
 void on( const mod::Cleared & ) noexcept { f_reset = true; }

 /// the digraph rebuilt: the same
 void on( const mod::Rebuilt & ) noexcept { f_reset = true; }

 /// the flow brought up to date: `run()` after a reset or at the first
 /// call, `runWarm()` otherwise
 typename A::ProblemType solve( void ) {
  if( f_reset ) {
   f_a.reset();
   load();
   f_reset = false;
   }
  if( f_a.warmReady() ) {
   ++warms;
   return( f_a.runWarm() );
   }
  ++scratches;
  return( f_a.run() );
  }

 /// the algorithm, for its flow, its potentials and its total cost
 [[nodiscard]] const A & algorithm( void ) const { return( f_a ); }

 int scratches = 0;  ///< the solves from scratch
 int warms = 0;      ///< the warm ones

/*--------------------------------------------------------------------------*/
 private:
 /// the maps given to the algorithm
 void load( void ) { f_a.costMap( f_c ).upperMap( f_u ).supplyMap( f_s ); }

 const GR & f_g;        ///< the digraph
 const CM & f_c;        ///< the costs
 const UM & f_u;        ///< the upper bounds
 const SM & f_s;        ///< the supplies
 A f_a;                 ///< the algorithm
 bool f_reset = false;  ///< whether the digraph changed

 };  // end( class MinCostFlowObserver )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Warm.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Warm.h -----------------------------*/
/*--------------------------------------------------------------------------*/
