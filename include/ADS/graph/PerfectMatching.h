/*--------------------------------------------------------------------------*/
/*------------------------- File PerfectMatching.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The perfect matching of least cost of a general graph, beyond the book,
 * by the primal-dual algorithm of Edmonds in the form of Blossom V
 * (Kolmogorov), and the matching of largest weight reduced to it:
 *
 * - `minCostPerfectMatching()`, the perfect matching of least total cost
 *   of a general graph with integer costs of any sign, or the evidence that
 *   there is none, with the duals that certify its optimum: the duals of
 *   the nodes and of the odd sets (the blossoms, a laminar family) keep the
 *   slack of every edge nonnegative and that of the matched ones 0. Each
 *   free node is the root of an alternating tree, and, as in Blossom V,
 *   each tree has its own change \f$ \varepsilon_T \f$ of the duals, so
 *   that the duals of the nodes of a tree are kept relative to it and a
 *   change of the duals of a tree costs \f$ O( 1 ) \f$; the edges whose
 *   slack may bound a change are kept in pairing heaps, those from the
 *   outer nodes of a tree to the free nodes, those between two outer nodes
 *   of the tree, its inner blossoms by their duals, and, for each pair of
 *   trees joined by an edge, those between their outer nodes and those
 *   from the outer nodes of one to the inner nodes of the other, so that
 *   each step of a tree reads the least slacks of its own heaps only. A
 *   tree grows, shrinks and expands blossoms on its tight edges, and
 *   augments the matching as soon as a tight edge joins it to another
 *   tree, while the other trees go on; when no tree has a tight edge left,
 *   the change of the duals is the largest one of each connected component
 *   of the trees joined by the tight edges from an outer node of one to an
 *   inner node of another (the rule of Blossom IV and of Blossom V), the
 *   components taken in turn. A greedy matching on the duals raised node by
 *   node starts the trees;
 * - `maxWeightMatching( n , E , BlossomVOptions )`, the matching of largest
 *   weight by the same algorithm on the graph doubled: two copies of the
 *   edges of positive weight, of costs the weights negated, and an edge of
 *   cost 0 from each node to its copy, whose perfect matchings of least
 *   cost give twice the largest weight, in \f$ 2 n \f$ nodes and
 *   \f$ 2 m + n \f$ edges.
 *
 * The algorithm of Edmonds on the blossoms is also in `BlossomMatching.h`
 * (in the form of Galil, \f$ O( n^3 ) \f$, which recomputes the trees at
 * each augmentation), in LEMON (`MaxWeightedPerfectMatching`, the same form
 * with heaps of the slacks and one change of the duals for all the trees)
 * and in the Boost Graph Library (`maximum_weighted_matching`); the
 * engineering of Blossom V (the changes of the duals tree by tree, the
 * heaps per pair of trees and the trees kept across the augmentations),
 * which Kolmogorov reports as faster than the Blossom IV of Cook and Rohe on
 * most of the families of his experiments, the geometric instances of the
 * TSP among them, is the reason of this version, which keeps the trees of
 * Blossom V and its rule of the components, but neither its fractional
 * initialization nor its changes of the duals by a linear program.
 *
 * The contracts [see `Contract.h`]: the costs are integers, \f$ 4 n \f$
 * times the largest magnitude of a cost fits in a `long long`, an edge
 * joins two nodes of the graph (a loop is never matched), the graph of
 * the StaticGraph is unoriented; at the level of the audits the duals
 * certify the optimum, as a feasible dual of the same value. The
 * functions are defined in `PerfectMatching.tpp`.
 *
 * \see J. Edmonds, Maximum matching and a polyhedron with 0,1-vertices,
 *      Journal of Research of the National Bureau of Standards 69B,
 *      125-130, 1965, doi:10.6028/jres.069B.013
 * \see W. Cook, A. Rohe, Computing minimum-weight perfect matchings,
 *      INFORMS Journal on Computing 11(2), 138-148, 1999,
 *      doi:10.1287/ijoc.11.2.138
 * \see V. Kolmogorov, Blossom V: a new implementation of a minimum cost
 *      perfect matching algorithm, Mathematical Programming Computation
 *      1(1), 43-67, 2009, doi:10.1007/s12532-009-0002-8
 * \see M. L. Fredman, R. Sedgewick, D. D. Sleator, R. E. Tarjan, The
 *      pairing heap: a new form of self-adjusting heap, Algorithmica 1(1-4),
 *      111-129, 1986, doi:10.1007/BF01840439
 * \see W. T. Tutte, The factorization of linear graphs, Journal of the
 *      London Mathematical Society 22(2), 107-111, 1947,
 *      doi:10.1112/jlms/s1-22.2.107
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

#ifndef __ADS_PerfectMatching
 #define __ADS_PerfectMatching
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/priority/PairingHeap.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the options of `minCostPerfectMatching()` and of the matching of largest
/// weight on it
struct BlossomVOptions
{
 /// whether a greedy matching on the duals raised node by node starts the
 /// trees, or each node is a tree of its own
 bool greedy = true;
 };

/// a perfect matching of least cost with the duals that certify it
template< class W >
struct PerfectMatchingResult
{
 bool perfect = false;  ///< whether the graph has a perfect matching
 W cost = W( 0 );       ///< the least cost of a perfect matching
 /// the places of its edges, in increasing order
 std::vector< std::size_t > edges;
 /// the laminar family of the odd sets of the duals: the nodes \f$ 0 ,
 /// \ldots , n - 1 \f$ and then the sets of three nodes or more, the parent
 /// of each, the least set that holds it, `NIL` if none
 std::vector< int > parent;
 /// twice the dual of each node and of each set, in the order of `parent`
 std::vector< W > dual;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the perfect matching of least total cost of the graph of \f$ n \f$ nodes
/// and of the edges \f$ E = ( u , v , c ) \f$, with the duals that certify
/// it, or `perfect` false if the graph has none
///
/// In pseudocode (the primal-dual algorithm of Edmonds in the form of
/// Blossom V, beyond the book; see \ref theory_graph_flow): the costs are
/// doubled, so that the duals stay integers, \f$ \pi( e ) \f$ is the slack
/// of an edge, its cost minus the duals of the sets that it leaves, a tree
/// \f$ T \f$ has outer (+) and inner (-) nodes, the outermost blossoms, and
/// each node of \f$ T \f$ has the dual it had when it joined, plus
/// \f$ \varepsilon_T \f$ if outer and minus it if inner.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minCostPerfectMatching}( n , E ) \cr
/// 2 & \quad \textbf{if}\ n \text{ is odd or a node has no edge}
///     \ \textbf{then return}\ \text{none} \cr
/// 3 & \quad y_v \gets \min_{e \ni v} c( e ) \ \forall v;\ \textbf{foreach}
///     \ v \text{ free}\ \textbf{do}\ y_v \gets y_v + \min_{e \ni v}
///     \pi( e ) \text{, match a tight edge to a free node} \cr
/// 4 & \quad \text{each free node is the root of a tree, outer},
///     \ \varepsilon_T \gets 0 \cr
/// 5 & \quad \textbf{while}\ \text{there are trees}\ \textbf{do} \cr
/// 6 & \qquad \textbf{foreach}\ \text{tree } T\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{if}\ \text{a tight edge joins } T
///     \text{ to an outer node of } T'\ \textbf{then}\ \text{augment};
///     \ T , T' \text{ dissolve};\ \textbf{continue} \cr
/// 8 & \qquad\quad \textbf{while}\ T \text{ has a tight edge to a free}
///     \text{ node, between two of its outer nodes, or an inner blossom}
///     \text{ of dual 0}\ \textbf{do} \cr
/// 9 & \qquad\qquad \text{grow } T \text{, shrink the cycle into an outer}
///     \text{ blossom, or expand the blossom};\ \text{augment as in 7} \cr
/// 10 & \qquad \textbf{if not}\ \mathrm{dualUpdate}()\ \textbf{then}
///     \ \textbf{return}\ \text{none} \cr
/// 11 & \quad \textbf{return}\ \text{the matching, the } y_v \text{ and
///     the duals of the blossoms}
/// \end{array}}
/// \f]
/// The change of the duals takes the components of the trees joined by the
/// tight edges from an outer node of one tree to an inner node of another,
/// which must change together, in turn, each change applied before the
/// next component is taken, so that the slacks are always the current ones:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dualUpdate}() \cr
/// 2 & \quad \textbf{foreach}\ \text{component } C \text{ in turn}
///     \ \textbf{do} \cr
/// 3 & \qquad \delta \gets \min_{T \in C} \min\{ \pi( +_T , \text{free} ) ,
///     \pi( +_T , +_T ) / 2 , z_B ( B \text{ inner in } T ) \} \cr
/// 4 & \qquad \textbf{foreach}\ \text{edge } e \text{ from } +_T ,
///     T \in C \text{, to } +_{T'} \text{ or to } -_{T'} \text{, } T'
///     \notin C\ \textbf{do}\ \delta \gets \min\{ \delta , \pi( e ) \} \cr
/// 5 & \qquad \textbf{foreach}\ \text{edge } e \text{ between } +_T
///     \text{ and } +_{T'} \text{, } T \ne T' \text{ in } C\ \textbf{do}
///     \ \delta \gets \min\{ \delta , \pi( e ) / 2 \} \cr
/// 6 & \qquad \textbf{if}\ \delta = \infty\ \textbf{then return false}
///     \quad \triangleright\ \text{no perfect matching} \cr
/// 7 & \qquad \varepsilon_T \gets \varepsilon_T + \delta\ \forall T \in C
///     \cr
/// 8 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, each edge joins two nodes in \f$ [ 0 , n ) \f$,
///      \f$ 4 n \f$ times the largest magnitude of a cost in a `long long`
/// \post with `perfect`, each node is in one edge of `edges`, and the duals
///       are feasible, of the value of twice the cost (audit)
template< class W >
 requires std::is_integral_v< W > && std::is_signed_v< W >
PerfectMatchingResult< W >
minCostPerfectMatching( int n ,
			const std::vector< std::tuple< int , int , W > > & E ,
			const BlossomVOptions & o = BlossomVOptions() );

/// the perfect matching of least cost of the unoriented graph \f$ g \f$,
/// its edges the places of the arcs \f$ ( u , v ) \f$ with \f$ u < v \f$
/// \pre the graph is unoriented, as for the edges above
template< class W , ArcLayout L >
 requires std::is_integral_v< W > && std::is_signed_v< W >
PerfectMatchingResult< W >
minCostPerfectMatching( const StaticGraph< W , L > & g ,
			const BlossomVOptions & o = BlossomVOptions() );

/// the largest total weight of a matching of the graph of \f$ n \f$ nodes
/// and of the edges \f$ E = ( u , v , w ) \f$, and the places in \f$ E \f$
/// of its edges, in increasing order, by `minCostPerfectMatching()` on the
/// graph doubled
/// \pre as for `minCostPerfectMatching()` on \f$ 2 n \f$ nodes
/// \post the weight is half the least cost on the graph doubled, negated
template< class W >
 requires std::is_integral_v< W > && std::is_signed_v< W >
std::pair< W , std::vector< std::size_t > >
maxWeightMatching( int n ,
		   const std::vector< std::tuple< int , int , W > > & E ,
		   const BlossomVOptions & o );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/PerfectMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PerfectMatching.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PerfectMatching.h -----------------------*/
/*--------------------------------------------------------------------------*/
