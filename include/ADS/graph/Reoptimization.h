/*--------------------------------------------------------------------------*/
/*------------------------- File Reoptimization.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The solutions that follow the changes of a graph, beyond the book: the
 * observers of an `Observable` [see `Observable.h`] that keep the tree of
 * the shortest paths from a node, the minimum spanning forest and the
 * maximum flow between two nodes, each together with a certificate of its
 * optimality, so that after a change they can often prove at once that the
 * solution is still optimal, and otherwise repair it or compute it again by
 * the algorithms of the book [see `ShortestPaths.h`,
 * `MinimumSpanningTree.h`, `MaxFlow.h`]. The certificates, the tests of
 * the events against them and the reasons of the costs are in
 * \ref theory_graph_dynamic.
 *
 * An observer models the concept `Reoptimizable`: `apply( e )` and
 * `apply( s )` give it one event or a span of them (`notify()` is the same,
 * for the `Observable`), `solve()` brings the solution up to date, and
 * `outcome()` tells how the last `solve()` with events to apply did it,
 * i.e., `Reopt::SCRATCH` (0) by the algorithm of the book,
 * `Reopt::WARM` (1) by repairing the previous solution, and
 * `Reopt::CERTIFIED` (2) with no computation at all. The events are queued
 * and the work is done at the next query, i.e., `solve()` or any read of
 * the solution: the queue is coalesced, and if its net events exceed
 * \f$ \rho \max( m , 1 ) \f$ (\f$ \rho \f$ of `setRecomputeRatio()`,
 * \f$ 1 / 4 \f$ by default) the solution is computed from scratch, else
 * each event is tested against the certificate in \f$ O( 1 ) \f$ (in
 * \f$ O( \log n ) \f$ for the forest), and by Lemma 7 of
 * \ref theory_graph_dynamic the events all harmless prove the solution
 * still optimal. The shortest paths are repaired after the raises and the
 * deletions of the edges of the tree as well as after the decreases, on
 * the subtrees whose distances are lost (Lemma 8 of
 * \ref theory_graph_dynamic). The maximum flow starts again from the flow
 * there is when an edge of the minimum cut grows, and when a capacity falls
 * below the flow of its edge, after its excess and its deficit are routed
 * [see `FlowNetwork::setCapacity()`], by the augmenting paths or by the
 * push-relabel: a warm start that the maximum flow of OR-Tools leaves to
 * do.
 *
 * The contracts [see `Contract.h`]: the root, and the source and the sink,
 * stay nodes of the graph; the graph of `SpanningForestObserver` is
 * unoriented; the capacities are nonnegative integers; the observer reads
 * the graph it was built on, which must be the graph of the `Observable`
 * that sends it the events (or one changed in the same way). At the level
 * of the audits each `solve()` checks its result: the conditions of
 * Bellman by `isShortestPathTree()`, the forest by
 * `isMinimumSpanningForest()` on the graphs of at most 2000 edges, and
 * \f$ | f | = c( S , V \setminus S ) \f$ on the graph. The operations are
 * defined in `Reoptimization.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 11, 14
 *      and 15, https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf
 * \see G. Ramalingam, T. Reps, An incremental algorithm for a
 *      generalization of the shortest-path problem, Journal of
 *      Algorithms 21(2), 267-305, 1996, doi:10.1006/jagm.1996.0046
 * \see R. Bauer, D. Wagner, Batch dynamic single-source shortest-path
 *      algorithms: an experimental study, Proceedings of SEA, LNCS 5526,
 *      51-62, 2009, doi:10.1007/978-3-642-02011-7_7
 * \see D. Frigioni, A. Marchetti-Spaccamela, U. Nanni, Fully dynamic
 *      algorithms for maintaining shortest paths trees, Journal of
 *      Algorithms 34(2), 251-281, 2000, doi:10.1006/jagm.1999.1048
 * \see P. M. Spira, A. Pan, On finding and updating spanning trees and
 *      shortest paths, SIAM Journal on Computing 4(3), 375-380, 1975,
 *      doi:10.1137/0204032
 * \see F. Chin, D. Houck, Algorithms for updating minimal spanning trees,
 *      Journal of Computer and System Sciences 16(3), 333-344, 1978,
 *      doi:10.1016/0022-0000(78)90022-3
 * \see R. E. Tarjan, Sensitivity analysis of minimum spanning trees and
 *      shortest path trees, Information Processing Letters 14(1), 30-33,
 *      1982, doi:10.1016/0020-0190(82)90137-5, and its corrigendum, 23(4),
 *      219, 1986, doi:10.1016/0020-0190(86)90139-0
 * \see D. D. Sleator, R. E. Tarjan, A data structure for dynamic trees,
 *      Journal of Computer and System Sciences 26(3), 362-391, 1983,
 *      doi:10.1016/0022-0000(83)90006-5
 * \see S. Kumar, P. Gupta, An incremental algorithm for the maximum flow
 *      problem, Journal of Mathematical Modelling and Algorithms 2(1),
 *      1-16, 2003, doi:10.1023/A:1023607406540
 * \see P. Kohli, P. H. S. Torr, Dynamic graph cuts for efficient inference
 *      in Markov random fields, IEEE Transactions on Pattern Analysis and
 *      Machine Intelligence 29(12), 2079-2088, 2007,
 *      doi:10.1109/TPAMI.2007.1128
 * \see A. V. Goldberg, S. Hed, H. Kaplan, P. Kohli, R. E. Tarjan,
 *      R. F. Werneck, Faster and more dynamic maximum flow by incremental
 *      breadth-first search, Proceedings of ESA, LNCS 9294, 619-630, 2015,
 *      doi:10.1007/978-3-662-48350-3_52
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

#ifndef __ADS_Reoptimization
 #define __ADS_Reoptimization
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Assignment.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/tree/LinkCutTree.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// how a `solve()` brought the solution up to date
struct Reopt {
 static constexpr int SCRATCH = 0;     ///< from scratch
 static constexpr int WARM = 1;        ///< by a repair
 static constexpr int CERTIFIED = 2;   ///< by the certificate, no work
 };

/// a solution that follows the events of weights `W`
template< class R , class W >
concept Reoptimizable = GraphObserver< R , W > &&
 requires( R & r , const GraphEvent< W > & e ,
           std::span< const GraphEvent< W > > s ) {
 r.apply( e );
 r.apply( s );
 r.solve();
 { r.outcome() } -> std::convertible_to< int >;
 };

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS EventQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the queue of the events and the counts that the observers share
///
/// In pseudocode (beyond the book; Lemma 7 of \ref theory_graph_dynamic
/// proves it correct), the query of every observer, with \f$ Q \f$ the
/// queue and \f$ \rho \f$ the ratio of `setRecomputeRatio()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
/// 2 & \quad \textbf{if}\ \text{there is no solution yet}\ \textbf{then}
///     \ \text{compute it from scratch};\ \textbf{return} \cr
/// 3 & \quad P \gets \mathrm{coalesce}( Q );\ Q \gets \emptyset \cr
/// 4 & \quad \textbf{if}\ |P| > \rho \max( m , 1 )\ \textbf{then}
///     \ \text{compute from scratch};\ \textbf{return} \cr
/// 5 & \quad \textbf{foreach}\ e \in P\ \textbf{do}\ \text{test } e
///     \text{ against the certificate} \cr
/// 6 & \quad \textbf{if}\ \text{an event is fatal}\ \textbf{then}
///     \ \text{compute from scratch} \cr
/// 7 & \quad \textbf{else if}\ \text{an event is repairable}
///     \ \textbf{then}\ \text{repair the solution} \cr
/// 8 & \quad \textbf{else}\ \text{keep it, certified}
/// \end{array}}
/// \f]
template< class W >
class EventQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Event = GraphEvent< W >;   ///< the type of the events

 /// all the kinds of event
 [[nodiscard]] static constexpr unsigned mask( void ) {
  return( GraphEvents::ALL );
  }

 /// queues \f$ e \f$
 void apply( const Event & e ) { f_q.push_back( e ); }

 /// queues the events of \f$ s \f$
 void apply( std::span< const Event > s ) {
  f_q.insert( f_q.end() , s.begin() , s.end() );
  }

 /// `apply( e )`, for the `Observable`
 void notify( const Event & e ) { apply( e ); }
 /// `apply( s )`, for the `Observable`
 void notify( std::span< const Event > s ) { apply( s ); }

 /// the events waiting for the next query
 [[nodiscard]] std::size_t pending( void ) const { return( f_q.size() ); }

 /// how the last `solve()` with events to apply, or with no solution yet,
 /// did it [see `Reopt`]
 [[nodiscard]] int outcome( void ) const { return( f_outcome ); }

 /// the solves of each outcome so far
 [[nodiscard]] long long count( int outcome ) const {
  return( f_count[ outcome ] );
  }

 /// from scratch when the events coalesced exceed \f$ \mathit{ratio} \f$
 /// times the edges
 /// \pre \f$ \mathit{ratio} \ge 0 \f$
 void setRecomputeRatio( double ratio ) {
  ADS_PRE( ratio >= 0 );
  f_ratio = ratio;
  }

/*--------------------------------------------------------------------------*/
 protected:

 /// the queue coalesced, emptied
 std::vector< Event > take( void ) {
  std::vector< Event > P = coalesce< W >( std::span< const Event >( f_q ) );
  f_q.clear();
  return( P );
  }

 /// whether \f$ k \f$ events are too many for \f$ m \f$ edges
 [[nodiscard]] bool tooMany( std::size_t k , long long m ) const {
  return( double( k ) > f_ratio * double( m > 1 ? m : 1 ) );
  }

 /// the outcome of a `solve()` recorded
 void done( int outcome ) {
  f_outcome = outcome;
  ++f_count[ outcome ];
  }

 std::vector< Event > f_q;              ///< the events queued
 double f_ratio = 0.25;                 ///< the ratio of `setRecomputeRatio()`
 int f_outcome = Reopt::SCRATCH;        ///< the last outcome
 long long f_count[ 3 ] = { 0 , 0 , 0 };   ///< the solves by outcome

 };  // end( class EventQueue )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ShortestPathsObserver -----------------------*/
/*--------------------------------------------------------------------------*/

/// the shortest paths from \f$ r \f$ in \f$ g \f$, kept up to date
template< Graph G >
class ShortestPathsObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the shortest paths from \f$ r \f$ in \f$ g \f$, computed by `how` at
 /// the first query
 /// \pre \f$ r \f$ is a node of \f$ g \f$
 ShortestPathsObserver( const G & g , int r ,
                        Frontier how = Frontier::HEAP );

 /// brings the tree up to date
 ///
 /// In pseudocode (the algorithms of Ramalingam and Reps for the decreases
 /// and the increases, in the batch form of Bauer and Wagner, beyond the
 /// book; it is correct by Lemma 8 of \ref theory_graph_dynamic), the
 /// events \f$ P \f$ coalesced, \f$ T \f$ the tree and \f$ d \f$ the
 /// distances before them:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad R \gets \text{the heads } v \text{ of the arcs } ( u , v )
 ///     \in T \text{ raised or deleted in } P \cr
 /// 3 & \quad L \gets \text{the arcs lowered or inserted in } P \cr
 /// 4 & \quad A \gets \text{the nodes of the subtrees of } T \text{ under }
 ///     R \cr
 /// 5 & \quad \textbf{if}\ 2 |A| > n\ \textbf{then}\ \text{from scratch};
 ///     \ \textbf{return} \cr
 /// 6 & \quad \textbf{foreach}\ x \in A\ \textbf{do}\ d[ x ] \gets \infty;
 ///     \ T[ x ] \gets \textbf{nil} \cr
 /// 7 & \quad S \gets \emptyset \cr
 /// 8 & \quad \textbf{foreach}\ x \in A\ \textbf{do} \cr
 /// 9 & \qquad \textbf{foreach}\ ( u , x ) \in E\ \textbf{with}
 ///     \ d[ u ] + w( u , x ) < d[ x ]\ \textbf{do}\ d[ x ] \gets d[ u ]
 ///     + w( u , x );\ T[ x ] \gets u \cr
 /// 10 & \qquad \textbf{if}\ d[ x ] < \infty\ \textbf{then}\ S \gets S
 ///     \cup \{ x \} \cr
 /// 11 & \quad \textbf{foreach}\ ( u , v ) \in L\ \textbf{with}\ d[ u ]
 ///     + w( u , v ) < d[ v ]\ \textbf{do}\ S \gets S \cup \{ u \} \cr
 /// 12 & \quad \textbf{if}\ S = \emptyset\ \textbf{then}\ \textbf{return}
 ///     \quad \triangleright\ \text{certified} \cr
 /// 13 & \quad H \gets \text{a priority queue of } S \text{ by } d \cr
 /// 14 & \quad \textbf{while}\ H \ne \emptyset\ \textbf{do} \cr
 /// 15 & \qquad u \gets \mathrm{deleteMin}( H ) \cr
 /// 16 & \qquad \textbf{if}\ u \text{ was extracted more than } n
 ///     \text{ times}\ \textbf{then}\ \text{Bellman-Ford-Moore};
 ///     \ \textbf{return} \cr
 /// 17 & \qquad \textbf{foreach}\ ( u , v ) \in E\ \textbf{with}
 ///     \ d[ u ] + w( u , v ) < d[ v ]\ \textbf{do} \cr
 /// 18 & \qquad\quad d[ v ] \gets d[ u ] + w( u , v );\ T[ v ] \gets u;
 ///     \ \mathrm{insert}( H , v , d[ v ] )
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the tree, up to date; its order is that of the last computation from
 /// scratch
 [[nodiscard]] const ShortestPathTree< Weight > & tree( void ) {
  solve();
  return( f_R );
  }

 /// the distance of \f$ v \f$, `infinity()` if not reachable
 [[nodiscard]] Weight distance( int v ) {
  solve();
  return( f_R.d[ v ] );
  }

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 /// the tree from scratch, by `how`
 void scratch( Frontier how );

 using Event = GraphEvent< Weight >;   ///< the type of the events

 /// the prototype from the seeds \f$ S \f$, which it empties; false if a
 /// node was extracted more than \f$ n \f$ times (a negative cycle)
 bool repair( std::vector< int > & S );

 /// the lists of the children of \f$ T \f$, from the parents
 void buildTree( void );

 /// the vectors of the nodes up to `bound()` of the graph
 void grow( void );

 /// \f$ x \f$ the first child of \f$ p \f$
 void link( int x , int p );

 /// \f$ x \f$ out of the children of its parent
 void unlink( int x );

 /// \f$ u \f$ the parent of \f$ v \f$, `NIL` for none
 void setParent( int v , int u );

 /// the arcs that enter each node, from the graph
 void buildIn( void );

 /// the events \f$ P \f$ on the arcs that enter each node
 void applyIn( std::span< const Event > P );

 /// `f( u , w )` for each arc \f$ ( u , x ) \f$ of weight \f$ w \f$
 template< class F >
 void forEachIn( int x , F && f ) const;

 const G & f_g;                     ///< the graph
 int f_r;                           ///< the root
 Frontier f_how;                    ///< the algorithm from scratch
 bool f_ready = false;              ///< whether there is a tree
 ShortestPathTree< Weight > f_R;    ///< the tree
 std::vector< int > f_child;        ///< the first child in \f$ T \f$
 std::vector< int > f_next;         ///< the next sibling in \f$ T \f$
 std::vector< int > f_prev;         ///< the previous sibling in \f$ T \f$
 std::vector< int > f_pops;         ///< the extractions of a repair
 std::vector< char > f_inA;         ///< the nodes whose distance is lost
 bool f_inOk = false;               ///< whether `f_in` is up to date
 /// the arcs \f$ ( u , x ) \f$ that enter each node \f$ x \f$ with their
 /// weights, kept for an oriented graph from the first increase on
 std::vector< std::vector< std::pair< int , Weight > > > f_in;

 };  // end( class ShortestPathsObserver )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SpanningForestObserver ----------------------*/
/*--------------------------------------------------------------------------*/

/// the minimum spanning forest of an unoriented \f$ g \f$, kept up to date
template< Graph G >
class SpanningForestObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the forest of \f$ g \f$, computed by Kruskal at the first query
 /// \pre \f$ g \f$ is unoriented
 explicit SpanningForestObserver( const G & g );

 /// brings the forest up to date
 ///
 /// In pseudocode (Sleator and Tarjan for the cycles, beyond the book),
 /// the events \f$ P \f$ coalesced applied one at a time to the forest
 /// \f$ F \f$ kept by a link-cut tree [see `LinkCutTree.h`], each by (10)
 /// of \ref theory_graph_dynamic, as follows:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad \textbf{foreach}\ e = [ u , v ] \text{ from } w \text{ to }
 ///     w' \text{ in } P\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ e \in F \textbf{ and } w' \le w\ \textbf{then}
 ///     \ \text{set its weight in } F \quad \triangleright
 ///     \ \text{harmless} \cr
 /// 4 & \qquad \textbf{else if}\ e \in F\ \textbf{then} \cr
 /// 5 & \qquad\quad \mathrm{cut}( u , v );\ X \gets \text{the smaller of}
 ///     \text{ the two trees, by two visits of } F \text{ in turn} \cr
 /// 6 & \qquad\quad f \gets \text{the lightest edge out of } X \text{ ($e$
 ///     with } w' \text{ among them, unless deleted)} \cr
 /// 7 & \qquad\quad \textbf{if}\ f \ne \textbf{nil}\ \textbf{then}
 ///     \ \mathrm{link}( f ) \cr
 /// 8 & \qquad \textbf{else if}\ w' \ne \bot \textbf{ and } ( w' < w
 ///     \textbf{ or } w = \bot )\ \textbf{then} \cr
 /// 9 & \qquad\quad \textbf{if}\ \textbf{not}\ \mathrm{connected}( u , v )
 ///     \ \textbf{then}\ \mathrm{link}( e ) \cr
 /// 10 & \qquad\quad \textbf{else}\ f \gets \text{the heaviest edge of }
 ///     F( u , v ) \text{ by } \mathrm{aggregate}( u , v ) \cr
 /// 11 & \qquad\qquad \textbf{if}\ w' < w( f )\ \textbf{then}
 ///     \ \mathrm{cut}( f );\ \mathrm{link}( e )
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the edges of the forest, up to date, in no given order
 [[nodiscard]] std::vector< WeightedEdge< Weight > > forest( void );

 /// the total weight of the forest, kept as it changes when the weights
 /// subtract (up to the rounding of floating weights), \f$ O( 1 ) \f$
 /// after the query, and summed otherwise
 [[nodiscard]] Weight weight( void );

 /// whether \f$ [ u , v ] \f$ is in the forest
 [[nodiscard]] bool inForest( int u , int v );

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;
 using Event = GraphEvent< Weight >;   ///< the type of the events

 /// an edge of the copy: its weight, its node in the link-cut tree if it
 /// is in the forest (`NIL` if not) and its place in `f_F`
 struct Rec {
  Weight w;                                      ///< the weight
  int node = NIL;                                ///< its node in `f_lct`
  int at = -1;                                   ///< its place in `f_F`
  };

 /// the value of a node of the link-cut tree: the weight of its edge and
 /// the node, none for the nodes of the graph
 struct Heavy {
  bool has = false;                              ///< whether an edge
  Weight w = Weight();                           ///< its weight
  int node = NIL;                                ///< its node
  /// whether the two values are the same
  bool operator==( const Heavy & h ) const {
   return( ( has == h.has ) && ( node == h.node ) );
   }
  };

 /// the heavier of two values, the maximum of the paths
 struct Heavier {
  /// the heavier of \f$ a \f$ and \f$ b \f$, by the weight and then by
  /// the node
  Heavy operator()( const Heavy & a , const Heavy & b ) const {
   if( ! a.has ) return( b );
   if( ! b.has ) return( a );
   if( a.w < b.w ) return( b );
   if( b.w < a.w ) return( a );
   return( a.node < b.node ? b : a );
   }
  };

 /// the key of \f$ [ u , v ] \f$
 static std::uint64_t key( int u , int v ) {
  if( v < u ) std::swap( u , v );
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
          std::uint32_t( v ) );
  }

 /// the ends of the key \f$ k \f$
 static std::pair< int , int > ends( std::uint64_t k ) {
  return( std::pair< int , int >( int( k >> 32 ) ,
                                  int( k & 0xffffffffu ) ) );
  }

 /// the copy of the edges from the graph, and Kruskal on it
 void scratch( bool fromGraph );

 /// the nodes up to \f$ n \f$
 void grow( int n );

 /// the edge \f$ k \f$ into the forest
 void addTree( std::uint64_t k );

 /// the edge \f$ k \f$ out of the forest
 void removeTree( std::uint64_t k );

 /// the lightest edge between the two trees of \f$ u \f$ and \f$ v \f$,
 /// just split, into `best`; false if none
 ///
 /// In pseudocode (beyond the book), \f$ F \f$ the forest, \f$ N( x ) \f$
 /// the edges of the copy at \f$ x \f$ and \f$ L \f$ the edges out of
 /// \f$ F \f$ by increasing weight; the two searches run in turn, a few
 /// steps each, and the first one that ends answers:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{replacement}( u , v ) \cr
 /// 2 & \quad \text{search 1: visit } F \text{ from } u \text{ and from } v
 ///     \text{, one node of each in turn, until one visit ends} \cr
 /// 3 & \qquad X \gets \text{the nodes of the visit that ended} \cr
 /// 4 & \qquad \textbf{return}\ \text{the lightest } [ x , y ] \in N( x )
 ///     \text{ with } x \in X ,\ y \notin X \cr
 /// 5 & \quad \text{search 2: } \textbf{foreach}\ [ a , b ] \in L
 ///     \text{ in order}\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ \mathrm{findRoot}( a ) = \mathrm{findRoot}( u )
 ///     \text{ for one end only}\ \textbf{then}\ \textbf{return}\ [ a , b ]
 ///     \cr
 /// 7 & \quad \textbf{return}\ \textbf{nil} \quad \triangleright
 ///     \ \text{search 2 found no edge across}
 /// \end{array}}
 /// \f]
 bool replacement( int u , int v , std::uint64_t & best );

 const G & f_g;                                  ///< the graph
 bool f_ready = false;                           ///< whether there is
                                                 ///< \f$ F \f$
 int f_n = 0;                                    ///< the bound of the nodes
 std::unordered_map< std::uint64_t , Rec > f_E;  ///< the copy of the edges
 std::vector< std::vector< int > > f_inc;        ///< the ends of the edges
                                                 ///< of the copy at a node
 std::vector< std::vector< int > > f_adjF;       ///< those of the forest
 std::vector< std::uint64_t > f_F;               ///< the edges of \f$ F \f$
 LinkCutTree< Heavy , Heavier > f_lct;           ///< \f$ F \f$, a node per
                                                 ///< node and per edge
 std::vector< int > f_vid;                       ///< the node of `f_lct`
                                                 ///< of each node
 std::vector< std::uint64_t > f_edgeOf;          ///< the edge of a node
                                                 ///< of `f_lct`
 std::vector< int > f_free;                      ///< the nodes of `f_lct`
                                                 ///< free for the edges
 std::vector< int > f_mark;                      ///< the visits of
                                                 ///< `replacement()`
 /// the edges out of \f$ F \f$ by increasing weight
 std::set< std::pair< Weight , std::uint64_t > > f_off;
 /// whether the weights subtract, so that the total is kept as they change
 static constexpr bool SUBTRACT = requires( Weight a , Weight b ) {
  { a - b } -> std::convertible_to< Weight >;
  };
 Weight f_total = Weight();                      ///< the total weight of
                                                 ///< \f$ F \f$, if they do
 int f_stamp = 0;                                ///< the visit of now

 };  // end( class SpanningForestObserver )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS MaxFlowObserver --------------------------*/
/*--------------------------------------------------------------------------*/

/// the maximum flow from \f$ s \f$ to \f$ t \f$ in \f$ g \f$, the weights
/// the capacities, kept up to date
template< Graph G >
requires std::is_integral_v< typename G::Weight >
class MaxFlowObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the flow from \f$ s \f$ to \f$ t \f$ in \f$ g \f$, computed by `how`
 /// at the first query
 /// \pre \f$ s \f$ and \f$ t \f$ are distinct nodes of \f$ g \f$
 MaxFlowObserver( const G & g , int s , int t ,
                  Augment how = Augment::DINIC );

 /// brings the flow up to date
 ///
 /// In pseudocode (beyond the book), the tests of the events \f$ P \f$
 /// coalesced against the minimum cut \f$ S \f$ (by Theorem 5 of
 /// \ref theory_graph_flow); a capacity cut below the flow of its edge
 /// leaves an excess and a deficit that `maxFlow()` routes first [see
 /// `FlowNetwork::setCapacity()`]:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad \mathit{warm} \gets \textbf{false} \cr
 /// 3 & \quad \textbf{foreach}\ ( u , v ) \text{ from } c \text{ to } c'
 ///     \text{ in } P\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ c' < f( u , v )\ \textbf{then}\ \mathit{warm}
 ///     \gets \textbf{true} \cr
 /// 5 & \qquad \textbf{if}\ u \in S \textbf{ and } v \notin S
 ///     \textbf{ and } c' > c\ \textbf{then}\ \mathit{warm} \gets
 ///     \textbf{true} \cr
 /// 6 & \quad \text{set the capacities of } P \cr
 /// 7 & \quad \textbf{if}\ \mathit{warm}\ \textbf{then}\ | f | \gets | f |
 ///     + \mathrm{maxFlow}( s , t ) \text{ from } f;
 ///     \ S \gets \mathrm{minCut}( s )
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the value of the maximum flow
 [[nodiscard]] long long value( void ) {
  solve();
  return( f_value );
  }

 /// the side of \f$ s \f$ of a minimum cut
 [[nodiscard]] const std::vector< bool > & minCut( void ) {
  solve();
  return( f_S );
  }

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 /// the key of \f$ ( u , v ) \f$
 static std::uint64_t key( int u , int v ) {
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
          std::uint32_t( v ) );
  }

 /// the flow from scratch
 void scratch( void );

 /// the capacity of the cut `f_S` in the graph
 [[nodiscard]] long long cutCapacity( void ) const;

 const G & f_g;                                 ///< the graph
 int f_s;                                       ///< the source
 int f_t;                                       ///< the sink
 Augment f_how;                                 ///< the augmenting paths
 bool f_ready = false;                          ///< whether there is f
 FlowNetwork f_net{ 0 };                        ///< the network
 /// the number of the arc \f$ ( u , v ) \f$ in the network
 std::unordered_map< std::uint64_t , int > f_arc;
 long long f_value = 0;                         ///< \f$ | f | \f$
 std::vector< bool > f_S;                       ///< the side of \f$ s \f$

 };  // end( class MaxFlowObserver )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS AssignmentObserver ------------------------*/
/*--------------------------------------------------------------------------*/

/// the assignment of least cost of the \f$ k \f$ rows \f$ 0 , \ldots ,
/// k - 1 \f$ of \f$ g \f$ to its \f$ k \f$ columns \f$ k , \ldots ,
/// 2 k - 1 \f$, the weights of the edges between them the costs, kept up
/// to date [see `Assignment.h`]
template< Graph G >
requires std::is_arithmetic_v< typename G::Weight >
class AssignmentObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the assignment of the \f$ k \f$ rows of \f$ g \f$, computed at the
 /// first query; a pair with no edge is forbidden
 /// \pre \f$ k \ge 0 \f$, the nodes of \f$ g \f$ are
 ///      \f$ 0 , \ldots , 2 k - 1 \f$, each edge between a row and a
 ///      column
 AssignmentObserver( const G & g , int k );

 /// brings the assignment up to date
 ///
 /// In pseudocode (beyond the book), the events \f$ P \f$ coalesced given
 /// to the `Assignment` [see `Assignment::solve()`]:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad \textbf{foreach}\ ( i , k + j ) \text{ from } w \text{ to }
 ///     w' \text{ in } P\ \textbf{do}\ c( i , j ) \gets w' \text{, or }
 ///     \infty \text{ if deleted} \cr
 /// 3 & \quad A.\mathrm{solve}()
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the assignment, up to date
 [[nodiscard]] Assignment< Weight > & assignment( void ) {
  solve();
  return( f_A );
  }

 /// the column, in \f$ [ k , 2 k ) \f$, of the row \f$ i \f$, \f$ -1 \f$ if
 /// none
 /// \pre \f$ i \in [ 0 , k ) \f$
 [[nodiscard]] int partner( int i ) {
  solve();
  const int j = f_A.column( i );
  return( j < 0 ? -1 : f_k + j );
  }

 /// the total cost of the assignment
 [[nodiscard]] Weight total( void ) {
  solve();
  return( f_A.total() );
  }

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 const G & f_g;                     ///< the graph
 int f_k;                           ///< the rows
 Assignment< Weight > f_A;          ///< the assignment

 };  // end( class AssignmentObserver )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Reoptimization.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Reoptimization.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Reoptimization.h ------------------------*/
/*--------------------------------------------------------------------------*/
