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
 * \f$ O( \log n ) \f$ for the forest), and by Lemma 6 of
 * \ref theory_graph_dynamic the events all harmless prove the solution
 * still optimal. The warm start of the maximum flow from the flow there is,
 * when an edge of the minimum cut grows, is one that the maximum flow of
 * OR-Tools leaves to do.
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
#include "ADS/graph/Graph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/ShortestPaths.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
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
/// In pseudocode (beyond the book; Lemma 6 of \ref theory_graph_dynamic
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
 /// In pseudocode (the algorithm of Ramalingam and Reps for the decreases,
 /// beyond the book), the tests of the events \f$ P \f$ coalesced, with
 /// \f$ T \f$ the tree and \f$ d \f$ the distances:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad S \gets \emptyset \cr
 /// 3 & \quad \textbf{foreach}\ ( u , v ) \text{ from } w \text{ to } w'
 ///     \text{ in } P\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ ( u , v ) \in T \textbf{ and } ( w' = \bot
 ///     \textbf{ or } w' > w )\ \textbf{then}\ \text{from scratch};
 ///     \ \textbf{return} \cr
 /// 5 & \qquad \textbf{if}\ ( u , v ) \in T \textbf{ or } d[ u ] + w'
 ///     < d[ v ]\ \textbf{then}\ S \gets S \cup \{ u \} \cr
 /// 6 & \quad \textbf{if}\ S = \emptyset\ \textbf{then}\ \textbf{return}
 ///     \quad \triangleright\ \text{certified} \cr
 /// 7 & \quad H \gets \text{a priority queue of } S \text{ by } d \cr
 /// 8 & \quad \textbf{while}\ H \ne \emptyset\ \textbf{do} \cr
 /// 9 & \qquad u \gets \mathrm{deleteMin}( H ) \cr
 /// 10 & \qquad \textbf{if}\ u \text{ was extracted more than } n
 ///     \text{ times}\ \textbf{then}\ \text{Bellman-Ford-Moore};
 ///     \ \textbf{return} \cr
 /// 11 & \qquad \textbf{foreach}\ ( u , v ) \in E\ \textbf{with}
 ///     \ d[ u ] + w( u , v ) < d[ v ]\ \textbf{do} \cr
 /// 12 & \qquad\quad d[ v ] \gets d[ u ] + w( u , v );\ T[ v ] \gets u;
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

 /// the prototype from the seeds; false if a node was extracted more than
 /// \f$ n \f$ times (a negative cycle)
 bool repair( const std::vector< int > & seeds );

 const G & f_g;                     ///< the graph
 int f_r;                           ///< the root
 Frontier f_how;                    ///< the algorithm from scratch
 bool f_ready = false;              ///< whether there is a tree
 ShortestPathTree< Weight > f_R;    ///< the tree

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
 /// In pseudocode (beyond the book), the tests of the events \f$ P \f$
 /// coalesced against (8) of \ref theory_graph_dynamic, with \f$ F \f$ the
 /// forest and \f$ \rho \f$ the replacements:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad D \gets \text{the edges out of } F \text{ lowered or}
 ///     \text{ inserted in } P \cr
 /// 3 & \quad U \gets \text{the edges of } F \text{ raised or deleted in }
 ///     P \cr
 /// 4 & \quad \textbf{if}\ D \ne \emptyset \textbf{ and } U \ne \emptyset
 ///     \ \textbf{then}\ \text{Kruskal from scratch};\ \textbf{return} \cr
 /// 5 & \quad \textbf{foreach}\ f = [ u , v ] \in D \text{ of weight } w'
 ///     \ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ u , v \text{ in two trees of } F
 ///     \textbf{ or } w' < \max_{F( u , v )} w\ \textbf{then}\ f
 ///     \text{ repairable} \cr
 /// 7 & \quad \textbf{foreach}\ e \in U \text{ of weight } w'
 ///     \ ( \infty \text{ if deleted} )\ \textbf{do} \cr
 /// 8 & \qquad \textbf{if}\ \rho( e ) \text{ exists}\textbf{ and }
 ///     w( \rho( e ) ) < w'\ \textbf{then}\ e \text{ repairable} \cr
 /// 9 & \quad \textbf{if}\ \text{an edge of } D \text{ is repairable}
 ///     \ \textbf{then}\ F \gets \mathrm{kruskal}( F \cup D ) \cr
 /// 10 & \quad \textbf{else if}\ U = \{ e \} \text{ repairable}
 ///     \ \textbf{then}\ F \gets F \setminus \{ e \} \cup \{ \rho( e ) \}
 ///     \cr
 /// 11 & \quad \textbf{else if}\ \text{an edge of } U \text{ is repairable}
 ///     \ \textbf{then}\ \text{Kruskal from scratch}
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the edges of the forest, up to date, in no given order
 [[nodiscard]] std::vector< WeightedEdge< Weight > > forest( void );

 /// the total weight of the forest
 [[nodiscard]] Weight weight( void );

 /// whether \f$ [ u , v ] \f$ is in the forest
 [[nodiscard]] bool inForest( int u , int v );

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 /// an edge of the copy: its weight and whether it is in the forest
 struct Rec {
  Weight w;                                      ///< the weight
  bool tree;                                     ///< whether in \f$ F \f$
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

 /// Kruskal on the edges of the graph, or on those of the copy
 void scratch( bool fromGraph );

 /// the forest as the edges given, the others out of it
 void setForest( const std::vector< WeightedEdge< Weight > > & F );

 /// the jumps of the binary lifting on the forest
 void buildLift( void );

 /// the replacements of the edges of the forest
 void buildRepl( void );

 /// the heaviest edge on the path of the forest between \f$ u \f$ and
 /// \f$ v \f$, nothing if they are in two trees
 [[nodiscard]] std::optional< Weight > maxPath( int u , int v ) const;

 /// the lowest common ancestor of \f$ u \f$ and \f$ v \f$, in the same
 /// tree
 [[nodiscard]] int lca( int u , int v ) const;

 /// the lightest edge out of \f$ F \f$ with one end in the subtree of
 /// \f$ x \f$, and its key, by a scan of the edges
 [[nodiscard]] std::optional< std::pair< Weight , std::uint64_t > >
  crossMin( int x ) const;

 const G & f_g;                                  ///< the graph
 bool f_ready = false;                           ///< whether there is
                                                 ///< \f$ F \f$
 int f_n = 0;                                    ///< the bound of the nodes
 std::unordered_map< std::uint64_t , Rec > f_E;  ///< the copy of the edges
 std::vector< std::uint64_t > f_F;               ///< the edges of \f$ F \f$

 bool f_shapeOk = false;                         ///< the parents, the
                                                 ///< depths and the
                                                 ///< weights to the
                                                 ///< parents are valid
 bool f_liftOk = false;                          ///< and the maxima of the
                                                 ///< jumps
 int f_log = 1;                                  ///< the levels of jumps
 std::vector< int > f_depth;                     ///< the depths
 std::vector< int > f_tree;                      ///< the tree of a node
 std::vector< int > f_tin;                       ///< the place in the
                                                 ///< preorder
 std::vector< int > f_size;                      ///< the size of the
                                                 ///< subtree
 std::vector< std::vector< int > > f_up;         ///< \f$ a_j( v ) \f$
 std::vector< std::vector< Weight > > f_mx;      ///< \f$ \mu_j( v ) \f$

 bool f_replOk = false;                          ///< the replacements are
                                                 ///< valid
 std::vector< std::optional< std::uint64_t > > f_repl;   ///< \f$ \rho \f$
                                                 ///< of the edge to the
                                                 ///< parent of each node
 std::vector< Weight > f_replW;                  ///< and its weight

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
 /// coalesced against the minimum cut \f$ S \f$ (by Theorem 4 of
 /// \ref theory_graph_flow):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad \textbf{foreach}\ ( u , v ) \text{ from } c \text{ to } c'
 ///     \text{ in } P\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ c' < f( u , v )\ \textbf{then}
 ///     \ \text{from scratch};\ \textbf{return} \cr
 /// 4 & \qquad \textbf{if}\ u \in S \textbf{ and } v \notin S
 ///     \textbf{ and } c' > c\ \textbf{then}\ \mathit{grow} \gets
 ///     \textbf{true} \cr
 /// 5 & \quad \text{set the capacities of } P \cr
 /// 6 & \quad \textbf{if}\ \mathit{grow}\ \textbf{then}\ | f | \gets | f |
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
