/*--------------------------------------------------------------------------*/
/*------------------------- File Reoptimization.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The solutions that follow the changes of a graph, beyond the book: the
 * observers of an Observable [see Observable.h] that keep the tree of the
 * shortest paths from a node, the minimum spanning forest and the maximum
 * flow between two nodes, each together with a certificate of its
 * optimality, so that after a change they can often prove at once that
 * the solution is still optimal, and otherwise repair it or compute it
 * again by the algorithms of the book [see ShortestPaths.h,
 * MinimumSpanningTree.h, MaxFlow.h].
 *
 * An observer R models the concept Reoptimizable: apply( e ) and
 * apply( s ) give it one event or a span of them (notify() is the same,
 * for the Observable), solve() brings the solution up to date, and
 * outcome() tells how the last solve() did it (a solve() with no event
 * queued does nothing and leaves it as it is), i.e.,
 * - Reopt::SCRATCH (0), from scratch, by the algorithm of the book;
 * - Reopt::WARM (1), by repairing the previous solution;
 * - Reopt::CERTIFIED (2), with no computation at all, the certificate of
 *   the previous solution proving it still optimal for the new data.
 * The events are queued and the work is done at the next query, i.e.,
 * solve() or any read of the solution (the lazy flush): the queue Q is
 * first coalesced into its net effect P [see coalesce()], and if
 * \f$ |P| > \rho \max( m , 1 ) \f$ (the ratio \f$ \rho \f$ of
 * setRecomputeRatio(), 1/4 by default) the solution is computed from
 * scratch, since a repair touching a large part of the graph costs as
 * much and is slower by a constant; otherwise each event of P is checked
 * against the certificate in \f$ O( 1 ) \f$ (or \f$ O( \log n ) \f$, see
 * below). The certificate is that of the previous solution, the events
 * all refer to the graph before P, and each test is a condition on a
 * single edge that stays true whatever the other events of P do; so the
 * tests of all the events of P together prove the solution optimal, and
 * then nothing is computed.
 * <pre>
 *   solve()
 *  1  if there is no solution yet then SCRATCH; return
 *  2  P ← coalesce( Q ); Q ← \f$ \emptyset \f$
 *  3  if \f$ |P| > \rho \max( m , 1 ) \f$ then SCRATCH; return
 *  4  for each e ∈ P: test e against the certificate: harmless,
 *     repairable or fatal
 *  5  if some e is fatal then SCRATCH
 *  6  else if some e is repairable then WARM (the repair of the problem)
 *  7  else CERTIFIED (only the bookkeeping of the events)
 * </pre>
 *
 * ShortestPathsObserver keeps the tree T of the shortest paths from r and
 * the distances d, whose certificate is the theorem of Bellman (Teorema
 * 11.1 of the book): T is optimal if and only if the reduced costs
 * \f[
 *   \bar{w}( u , v ) = d_u + w( u , v ) - d_v
 * \f]
 * are zero on the arcs of T and nonnegative on all the arcs. Hence a
 * change of the weight of an arc ( u , v ), to \f$ w' \f$, is
 * - harmless if ( u , v ) is not in T and \f$ d_u + w' \geq d_v \f$ (or
 *   u is not reached), as the deletion of an arc not in T: the
 *   conditions on all the other arcs are the same, and d does not change;
 * - repairable if \f$ d_u + w' < d_v \f$, or if ( u , v ) is in T and
 *   \f$ w' < w \f$: d is still an upper bound of the distances, given by
 *   real paths (those of T, which are still there and no longer), and the
 *   prototype of section 11.2 started with S = { u } on the seeds of all
 *   the repairable arcs, instead of S = { r }, makes it exact; a priority
 *   queue on ( d , node ), with the negative weights a node extracted
 *   more than once, and a negative cycle (closed by a decrease) found when
 *   a node is extracted more than n times, then solved from scratch by
 *   Bellman-Ford-Moore, which reports it (this is the algorithm of
 *   Ramalingam and Reps for the decreases);
 * - fatal if ( u , v ) is in T and \f$ w' > w \f$ or it is deleted: d
 *   grows on the subtree of v, which the certificate does not bound.
 * <pre>
 *   repair( seeds )
 *  1  H ← the priority queue of the seeds u by \f$ d_u \f$
 *  2  while H ≠ \f$ \emptyset \f$
 *  3   u ← H.deleteMin(); if u was extracted more than n times then
 *      SCRATCH by Bellman-Ford-Moore; return
 *  4   for each ( u , v ) ∈ E with \f$ d_u + w( u , v ) < d_v \f$
 *  5    \f$ d_v \gets d_u + w( u , v ) \f$; \f$ T_v \gets u \f$;
 *       H.insert( v , \f$ d_v \f$ )
 * </pre>
 * Its cost is \f$ O( ( n_\delta + m_\delta ) \log n_\delta ) \f$ with
 * nonnegative weights, \f$ n_\delta \f$ and \f$ m_\delta \f$ the nodes
 * whose distance falls and their arcs, against \f$ O( m \log n ) \f$ from
 * scratch.
 *
 * SpanningForestObserver keeps a minimum spanning forest F of an
 * unoriented graph, whose certificates are the two conditions of
 * optimality of section 14.3 (the Teorema 14.1 and its dual): F is
 * minimum if and only if
 * \f[
 *   w( f ) \geq \max_{e \in F( u , v )} w( e ) \quad
 *   \forall f = [ u , v ] \notin F , \qquad
 *   w( e ) \leq \min_{f \notin F ,\, e \in F( f )} w( f ) \quad
 *   \forall e \in F ,
 * \f]
 * F( u , v ) the path of F between u and v (the cycle condition) and
 * F( f ) that path for f (the cut condition: e is the lightest edge across
 * the cut that its removal leaves). The two sides are kept as data: the
 * maximum on the paths of F by the jumps of the binary lifting, i.e., for
 * each node v its ancestor \f$ a_j( v ) \f$ at \f$ 2^j \f$ steps and the
 * heaviest edge \f$ \mu_j( v ) \f$ on the way, so that
 * \f$ \max_{F( u , v )} \f$ takes \f$ O( \log n ) \f$; and the replacement
 * \f$ \rho( e ) \f$ of each edge e of F, the lightest f out of F whose
 * path holds e (the sensitivity analysis of Tarjan), found by scanning
 * the edges out of F by increasing weight and giving each its path's
 * edges of F still without one, which a union-find on the forest skips,
 * in \f$ O( m \log m ) \f$ in all. Hence a change of a weight is
 * - harmless if it raises an edge out of F or lowers one of F, or deletes
 *   an edge out of F: neither side gets worse;
 * - certified if it lowers (or inserts) an edge f out of F to
 *   \f$ w' \geq \max_{F( f )} \f$, or raises an edge e of F to
 *   \f$ w' \leq w( \rho( e ) ) \f$, or deletes an edge of F with no
 *   replacement (the forest splits there);
 * - repairable otherwise. The edges lowered or inserted, D, are a
 *   superset of the edges that enter the forest when F only gets lighter
 *   (an edge out of F and out of D is still the heaviest of its cycle), so
 *   that the new forest is the one of Kruskal on \f$ F \cup D \f$, in
 *   \f$ O( ( n + |D| ) \log( n + |D| ) ) \f$; an edge of F raised beyond
 *   its replacement, when it is the only edge of F raised, is swapped with
 *   it (the cut condition). Two or more edges of F raised and one of them
 *   beyond its replacement, or raises and decreases together (whose
 *   conditions interact), go from scratch, by Kruskal on the copy of the
 *   edges that the observer keeps.
 * The data of the certificate stay valid, or become conservative (they
 * may reject a change that would be harmless, never accept a wrong one),
 * as long as the forest keeps its edges: the maxima of the paths only
 * fall when edges of F get lighter, and the replacements only rise when
 * edges out of F get heavier or leave. When an edge of F gets heavier the
 * weight of the edge to its parent is updated and the maxima of the jumps
 * are dropped, so that a path is then scanned edge by edge, in
 * \f$ O( \mathit{length} ) \f$, until the next change of the shape of
 * F; when an edge out of F gets lighter the replacements are dropped, and
 * built again at their next use, in \f$ O( m \log m ) \f$, from the copy
 * of the edges.
 * <pre>
 *   certify( P )
 *  1  for each f = [ u , v ] ∉ F lowered or inserted to w' in P
 *  2   if u and v are in two trees of F or \f$ w' < \max_{F( u , v )} \f$
 *      then f is repairable
 *  3  for each e ∈ F raised to w' (deleted: w' = \f$ +\infty \f$) in P
 *  4   if \f$ \rho( e ) \f$ exists and \f$ w( \rho( e ) ) < w' \f$ then e
 *      is repairable
 * </pre>
 *
 * MaxFlowObserver keeps a maximum flow f from s to t, the capacities the
 * weights of the edges (integers, an unoriented edge two arcs), and the
 * side S of a minimum cut, the certificate of the Teorema 15.1: f is
 * maximum if and only if it is feasible and
 * \f[
 *   |f| = c( S , V - S ) = \sum_{u \in S ,\, v \notin S} c( u , v ) .
 * \f]
 * A change of the capacity of ( u , v ) from c to c' is
 * - harmless if \f$ f( u , v ) \leq c' \f$ and ( u , v ) does not go from
 *   S to V - S with \f$ c' > c \f$: f stays feasible, and the capacity of
 *   the cut cannot fall below |f| nor rise (an arc of the cut is
 *   saturated, so that lowering it is caught by the first test);
 * - repairable if ( u , v ) goes from S to V - S and \f$ c' > c \f$: f
 *   stays feasible, and the augmenting paths of the algorithm of Dinic
 *   start from it, not from zero (the warm start that the maximum flow of
 *   OR-Tools leaves as a TODO);
 * - fatal if \f$ c' < f( u , v ) \f$: f is no longer feasible.
 * After a certified change S is still a minimum cut, though perhaps not
 * the set of the nodes that the residual network reaches from s.
 *
 * The contracts [see Contract.h]: the root r, and s and t, stay nodes of
 * the graph; the graph of SpanningForestObserver is unoriented; the
 * capacities are nonnegative integers; the observer reads the graph it was
 * built on, which must be the graph of the Observable that sends it the
 * events (or one changed in the same way). At the level of the audits,
 * each solve() checks the result: the conditions of Bellman by
 * isShortestPathTree(), the forest by isMinimumSpanningForest() on the
 * graphs of at most 2000 edges, and |f| = c( S , V - S ) on the graph. The
 * operations are defined in Reoptimization.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 11, 14
 *      and 15, https://algoritmicamente.it/materiali/libro/capitoli/
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

/// how a solve() brought the solution up to date
struct Reopt {
 static constexpr int SCRATCH = 0;     ///< from scratch
 static constexpr int WARM = 1;        ///< by a repair
 static constexpr int CERTIFIED = 2;   ///< by the certificate, no work
 };

/// a solution that follows the events of weights W
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
template< class W >
class EventQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Event = GraphEvent< W >;   ///< the type of the events

 /// all the kinds of event
 [[nodiscard]] static constexpr unsigned mask( void ) {
  return( GraphEvents::ALL );
  }

 /// queues e
 void apply( const Event & e ) { f_q.push_back( e ); }

 /// queues the events of s
 void apply( std::span< const Event > s ) {
  f_q.insert( f_q.end() , s.begin() , s.end() );
  }

 void notify( const Event & e ) { apply( e ); }
 void notify( std::span< const Event > s ) { apply( s ); }

 /// the events waiting for the next query
 [[nodiscard]] std::size_t pending( void ) const { return( f_q.size() ); }

 /// how the last solve() with events to apply, or with no solution yet,
 /// did it [see Reopt]
 [[nodiscard]] int outcome( void ) const { return( f_outcome ); }

 /// the solves of each outcome so far
 [[nodiscard]] long long count( int outcome ) const {
  return( f_count[ outcome ] );
  }

 /// from scratch when the events coalesced exceed ratio times the edges
 /// \pre ratio >= 0
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

 /// whether k events are too many for m edges
 [[nodiscard]] bool tooMany( std::size_t k , long long m ) const {
  return( double( k ) > f_ratio * double( m > 1 ? m : 1 ) );
  }

 void done( int outcome ) {
  f_outcome = outcome;
  ++f_count[ outcome ];
  }

 std::vector< Event > f_q;              ///< the events queued
 double f_ratio = 0.25;                 ///< the ratio of setRecomputeRatio()
 int f_outcome = Reopt::SCRATCH;        ///< the last outcome
 long long f_count[ 3 ] = { 0 , 0 , 0 };   ///< the solves by outcome

 };  // end( class EventQueue )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ShortestPathsObserver -----------------------*/
/*--------------------------------------------------------------------------*/

/// the shortest paths from r in g, kept up to date
template< Graph G >
class ShortestPathsObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the shortest paths from r in g, computed by how at the first query
 /// \pre r is a node of g
 ShortestPathsObserver( const G & g , int r ,
                        Frontier how = Frontier::HEAP );

 /// brings the tree up to date
 void solve( void );

 /// the tree, up to date; its order is that of the last computation from
 /// scratch
 [[nodiscard]] const ShortestPathTree< Weight > & tree( void ) {
  solve();
  return( f_R );
  }

 /// the distance of v, infinity() if not reachable
 [[nodiscard]] Weight distance( int v ) {
  solve();
  return( f_R.d[ v ] );
  }

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 void scratch( Frontier how );

 /// the prototype from the seeds; false if a node was extracted more than
 /// n times (a negative cycle)
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

/// the minimum spanning forest of an unoriented g, kept up to date
template< Graph G >
class SpanningForestObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the forest of g, computed by Kruskal at the first query
 /// \pre g is unoriented
 explicit SpanningForestObserver( const G & g );

 /// brings the forest up to date
 void solve( void );

 /// the edges of the forest, up to date, in no given order
 [[nodiscard]] std::vector< WeightedEdge< Weight > > forest( void );

 /// the total weight of the forest
 [[nodiscard]] Weight weight( void );

 /// whether [ u , v ] is in the forest
 [[nodiscard]] bool inForest( int u , int v );

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 /// an edge of the copy: its weight and whether it is in the forest
 struct Rec {
  Weight w;
  bool tree;
  };

 static std::uint64_t key( int u , int v ) {
  if( v < u ) std::swap( u , v );
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
          std::uint32_t( v ) );
  }

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

 /// the heaviest edge on the path of the forest between u and v, nothing
 /// if they are in two trees
 [[nodiscard]] std::optional< Weight > maxPath( int u , int v ) const;

 /// the lowest common ancestor of u and v, in the same tree
 [[nodiscard]] int lca( int u , int v ) const;

 /// the lightest edge out of F with one end in the subtree of x, and its
 /// key, by a scan of the edges
 [[nodiscard]] std::optional< std::pair< Weight , std::uint64_t > >
  crossMin( int x ) const;

 const G & f_g;                                  ///< the graph
 bool f_ready = false;                           ///< whether there is F
 int f_n = 0;                                    ///< the bound of the nodes
 std::unordered_map< std::uint64_t , Rec > f_E;  ///< the copy of the edges
 std::vector< std::uint64_t > f_F;               ///< the edges of F

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

/// the maximum flow from s to t in g, the weights the capacities, kept up
/// to date
template< Graph G >
requires std::is_integral_v< typename G::Weight >
class MaxFlowObserver : public EventQueue< typename G::Weight > {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;   ///< the type of the weights

 /// the flow from s to t in g, computed by how at the first query
 /// \pre s and t are distinct nodes of g
 MaxFlowObserver( const G & g , int s , int t ,
                  Augment how = Augment::DINIC );

 /// brings the flow up to date
 void solve( void );

 /// the value of the maximum flow
 [[nodiscard]] long long value( void ) {
  solve();
  return( f_value );
  }

 /// the side of s of a minimum cut
 [[nodiscard]] const std::vector< bool > & minCut( void ) {
  solve();
  return( f_S );
  }

/*--------------------------------------------------------------------------*/
 private:

 using B = EventQueue< Weight >;

 static std::uint64_t key( int u , int v ) {
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
          std::uint32_t( v ) );
  }

 void scratch( void );

 /// the capacity of the cut f_S in the graph
 [[nodiscard]] long long cutCapacity( void ) const;

 const G & f_g;                                 ///< the graph
 int f_s , f_t;                                 ///< the source, the sink
 Augment f_how;                                 ///< the augmenting paths
 bool f_ready = false;                          ///< whether there is f
 FlowNetwork f_net{ 0 };                        ///< the network
 std::unordered_map< std::uint64_t , int > f_arc;   ///< the arc ( u , v )
 long long f_value = 0;                         ///< |f|
 std::vector< bool > f_S;                       ///< the side of s

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
