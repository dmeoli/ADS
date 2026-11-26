/*--------------------------------------------------------------------------*/
/*---------------------- File ParallelBiconnectivity.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The biconnected components and the 2-edge-connected components of an
 * unoriented graph on the threads of an executor [see `Executor.h`], beyond
 * the book, with the same results as the visit in depth of
 * `Biconnectivity.h` [see `Biconnectivity.h`], whose
 * `BiconnectedComponents` they give:
 *
 * - `biconnectedComponents( ex , g )` is FAST-BCC of Dong, Wang, Gu and
 *   Sun, which needs no visit in depth, whose depth grows with the
 *   diameter of the graph, and no auxiliary graph of \f$ O( m ) \f$ edges,
 *   as the algorithm of Tarjan and Vishkin does. A spanning forest comes
 *   from the connected components of Afforest [see `ParallelGraph.h`],
 *   whose links that join two sets are its edges; the forest is rooted at
 *   the least node of each tree by the tour of Euler of its edges, ranked
 *   by a sparse ruling set (Helman and JaJa), which gives each node its
 *   number in a preorder, the last number of its subtree and its parent.
 *   The tags \f$ \mathit{low}( v ) \f$ and \f$ \mathit{high}( v ) \f$ are
 *   the least and the largest preorder number of a neighbor of the subtree
 *   of \f$ v \f$ (its own nodes included), minima and maxima of the ranges
 *   of the preorder, which a sparse table on blocks of 64 numbers answers.
 *   An edge \f$ [ p( v ) , v ] \f$ of the forest is a fence if no edge
 *   leaves the subtree of \f$ v \f$ but into the subtree of its parent,
 *   i.e., \f$ \mathit{low}( v ) \ge \mathit{pre}( p( v ) ) \f$ and
 *   \f$ \mathit{high}( v ) \le \mathit{last}( p( v ) ) \f$; the connected
 *   components of the skeleton, the graph of the edges of the forest that
 *   are not fences and of the edges that join two nodes of which neither is
 *   an ancestor of the other, again by Afforest, are the blocks less their
 *   heads, the head of a component being the parent of its first node;
 * - `twoEdgeConnectedComponents( ex , g )`, the connected components of
 *   the graph less its bridges, the blocks of one edge, by Afforest.
 *
 * The work is \f$ O( n + m ) \f$ expected (the components and the ranking
 * are randomized), plus the sparse table of the \f$ n / 64 \f$ blocks, which
 * is built on one thread in \f$ O( ( n / 64 ) \log n ) \f$; the depth is
 * that of the components and of the ruling set (\f$ O( \log n ) \f$
 * expected rounds of hooks on random graphs, and walks of
 * \f$ O( \log n ) \f$ arcs with high probability), but for the loops
 * over the arcs of a node of large degree, which run on one thread. Besides
 * the result, the algorithm keeps \f$ O( n ) \f$ integers, at most about 30
 * per node (the arcs of the forest included), and nothing per edge of the
 * graph.
 *
 * The contracts [see `Contract.h`]: the graph is unoriented; at the level
 * of the audits each result is checked against the sequential one. The
 * functions are defined in `ParallelBiconnectivity.tpp`.
 *
 * \see X. Dong, L. Wang, Y. Gu, Y. Sun, Provably fast and space-efficient
 *      parallel biconnectivity, Proceedings of the 28th ACM SIGPLAN Annual
 *      Symposium on Principles and Practice of Parallel Programming
 *      (PPoPP), 52-65, 2023, doi:10.1145/3572848.3577483
 * \see R. E. Tarjan, U. Vishkin, An efficient parallel biconnectivity
 *      algorithm, SIAM Journal on Computing 14(4), 862-874, 1985,
 *      doi:10.1137/0214061
 * \see D. R. Helman, J. JaJa, Prefix computations on symmetric
 *      multiprocessors, Journal of Parallel and Distributed Computing
 *      61(2), 265-278, 2001, doi:10.1006/jpdc.2000.1678
 * \see M. Sutton, T. Ben-Nun, A. Barak, Optimizing parallel graph
 *      connectivity computation via subgraph sampling, IEEE International
 *      Parallel and Distributed Processing Symposium (IPDPS), 12-21, 2018,
 *      doi:10.1109/IPDPS.2018.00012
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

#ifndef __ADS_ParallelBiconnectivity
 #define __ADS_ParallelBiconnectivity
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/graph/Biconnectivity.h"
#include "ADS/graph/Graph.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelGraph.h"
#include "ADS/search/RangeMinimum.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the blocks, the cut nodes and the bridges of an unoriented graph, those
/// of `biconnectedComponents( g )`
///
/// In pseudocode (FAST-BCC of Dong, Wang, Gu and Sun, beyond the book), the
/// forest \f$ F \f$ rooted at the least node of each tree, \f$ p \f$ the
/// parent, \f$ \mathit{pre}( v ) \f$ the number of \f$ v \f$ in a preorder
/// of \f$ F \f$ and \f$ \mathit{last}( v ) \f$ the largest one in its
/// subtree \f$ T_v \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{biconnectedComponents}( \mathit{ex} , G )
///     \cr
/// 2 & \quad F \gets \text{the edges whose links join two sets in the }
///     \text{components of } G \text{ by Afforest} \cr
/// 3 & \quad ( \mathit{pre} , \mathit{last} , p ) \gets \mathrm{rootForest}(
///     F ) \cr
/// 4 & \quad \textbf{foreach}\ v \in V\ \textbf{do in parallel}\ \ell[
///     \mathit{pre}( v ) ] \gets \min \{ \mathit{pre}( w ) : w \in \{ v \}
///     \cup G.\mathrm{adj}( v ) \};\ h[ \mathit{pre}( v ) ] \gets \max \{
///     \ldots \} \cr
/// 5 & \quad \textbf{foreach}\ v \in V\ \textbf{with}\ p( v ) \ne
///     \textbf{nil}\ \textbf{do in parallel} \cr
/// 6 & \qquad \mathit{fence}( v ) \gets \min \ell[ \mathit{pre}( v ) ..
///     \mathit{last}( v ) ] \ge \mathit{pre}( p( v ) )\ \textbf{and}\ \max
///     h[ \mathit{pre}( v ) .. \mathit{last}( v ) ] \le \mathit{last}( p( v
///     ) ) \cr
/// 7 & \quad S \gets \text{the edges } [ p( v ) , v ] \text{ of } F
///     \text{ with } \textbf{not}\ \mathit{fence}( v ) , \text{and those }
///     [ u , w ] \text{ of } G \text{ with neither an ancestor of the other}
///     \cr
/// 8 & \quad C \gets \text{the least node of the component of each node in }
///     S \text{, by Afforest} \cr
/// 9 & \quad \textbf{foreach}\ v\ \textbf{with}\ p( v ) \ne \textbf{nil}
///     \ \textbf{do}\ B[ v ] \gets C[ v ]\quad \triangleright\ \text{the }
///     \text{block less its head} \cr
/// 10 & \qquad \textbf{if}\ C[ p( v ) ] \ne C[ v ]\ \textbf{then}
///     \ h[ C[ v ] ] \gets p( v )\quad \triangleright\ v \text{ is the }
///     \text{first node of its component} \cr
/// 11 & \quad \textbf{return}\ B \text{ with the blocks numbered by their }
///     \text{least node, and } \mathit{pre} \text{ as the rank}
/// \end{array}}
/// \f]
/// A block is a bridge if it is a node \f$ v \f$ with no edge in \f$ S \f$
/// and one edge to \f$ p( v ) \f$; a node is a cut node if it is the head of
/// a block that it is not in, a root if it heads two. The forest is rooted
/// by the tour of Euler of its arcs, \f$ \mathrm{next}( u , v ) \f$ the arc
/// out of \f$ v \f$ after \f$ ( v , u ) \f$ in the row of \f$ v \f$, ranked
/// by a sparse ruling set:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rootForest}( F ) \cr
/// 2 & \quad A \gets \text{the rows of the arcs } ( u , v ) \text{ and } ( v
///     , u ) \text{ of the edges of } F \text{, by counting} \cr
/// 3 & \quad \textbf{foreach}\ \text{root } r\ \textbf{do}\ \mathrm{next}(
///     \text{the arc into } r \text{ before its first} ) \gets
///     \textbf{nil}\quad \triangleright\ \text{a list per tree} \cr
/// 4 & \quad R \gets \text{the first arc of each list, and each arc with }
///     \text{probability } 1 / 64 \text{ by a hash} \cr
/// 5 & \quad \textbf{foreach}\ x \in R\ \textbf{do in parallel}\ \text{walk }
///     \mathrm{next} \text{ from } x \text{ to the next arc of } R
///     \text{, ranking the arcs met from } x \cr
/// 6 & \quad \textbf{foreach}\ \text{list}\ \textbf{do in parallel}
///     \ \text{add up the lengths of its walks in their order} \cr
/// 7 & \quad \mathit{rank}( x ) \gets \text{the place of } x \text{ in its }
///     \text{list, for each arc} \cr
/// 8 & \quad \mathit{pre}( r ) \gets \text{the nodes of the trees of the }
///     \text{roots before } r \text{, for each root} \cr
/// 9 & \quad \textbf{foreach}\ \text{arc } x = ( u , v )\ \textbf{with}
///     \ \mathit{rank}( x ) < \mathit{rank}( v , u )\ \textbf{do in parallel}
///     \cr
/// 10 & \qquad p( v ) \gets u;\ \mathit{pre}( v ) \gets \mathit{pre}( r )
///     + 1 + \text{the arcs going down before } x \text{ in its list} \cr
/// 11 & \qquad \mathit{last}( v ) \gets \mathit{pre}( v ) + ( \mathit{rank}(
///     v , u ) - \mathit{rank}( x ) - 1 ) / 2
/// \end{array}}
/// \f]
/// where the arcs going down are counted by a prefix sum over the arcs in
/// the order of the lists, each root before the arcs of its tree.
/// \pre \f$ g \f$ is unoriented
/// \post the result of `biconnectedComponents( g )` (audit)
template< Executor E , Graph G >
[[nodiscard]] BiconnectedComponents biconnectedComponents( E & ex ,
							   const G & g );

/// the least node of the 2-edge-connected component of each name,
/// \f$ -1 \f$ for a name that is not a node, those of
/// `twoEdgeConnectedComponents( g )`
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoEdgeConnectedComponents}( \mathit{ex}
///     , G ) \cr
/// 2 & \quad B \gets \mathrm{biconnectedComponents}( \mathit{ex} , G ) \cr
/// 3 & \quad \textbf{return}\ \text{the least node of the component of each }
///     \text{node, by Afforest on the edges of } G \text{ that are not } \cr
/// 4 & \quad\quad \text{loops nor in a block that is a bridge}
/// \end{array}}
/// \f]
/// \pre \f$ g \f$ is unoriented
/// \post the result of `twoEdgeConnectedComponents( g )` (audit)
template< Executor E , Graph G >
[[nodiscard]] std::vector< int > twoEdgeConnectedComponents( E & ex ,
							     const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelBiconnectivity.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ParallelBiconnectivity.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ParallelBiconnectivity.h -------------------*/
/*--------------------------------------------------------------------------*/
