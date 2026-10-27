/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel visits of a `StaticGraph` [see `StaticGraph.h`], beyond the
 * book, on the threads of an executor [see `Executor.h`]: the distances of a
 * visit in breadth and the connected components, with the same results as
 * the sequential visits.
 *
 * - `bfsDistances( ex , g , s )` is the visit in breadth from \f$ s \f$ that
 *   optimizes its direction, as Beamer, Asanovic and Patterson: a level goes
 *   from the top down (each node of the frontier claims, by a
 *   compare-and-swap on its distance, its neighbors not yet reached, and
 *   the nodes claimed by each block of the frontier are gathered into the
 *   next one) while the frontier is small, and from the bottom up (each node
 *   not yet reached looks for a neighbor in the frontier, kept as a bitmap,
 *   and stops at the first it finds) when the arcs out of the frontier
 *   \f$ m_f \f$ exceed the arcs still to check \f$ m_u \f$ divided by
 *   \f$ \alpha = 15 \f$; it goes back from the top down when the frontier
 *   shrinks below \f$ n / \beta \f$, \f$ \beta = 18 \f$. The steps from the
 *   bottom up skip most of the arcs of the large levels of the graphs of
 *   small diameter, e.g., the social networks and the graphs of Kronecker of
 *   the Graph 500, where a level may hold most of the nodes. The steps from
 *   the bottom up read the arcs into each node, i.e., those of \f$ g \f$ if
 *   it is unoriented, of its transpose (given as `gt`) if it is oriented;
 *   with no transpose an oriented graph is visited from the top down only.
 *   The distance of a node is its level, the same whoever claims it: \f$ -1
 *   \f$ for the nodes not reached.
 * - `connectedComponents( ex , g )` labels each node with the least node of
 *   its component, by the sampling of Afforest (Sutton, Ben-Nun and Barak)
 *   on the concurrent union-find of ADS [see `ConcurrentUnionFind.h`], whose
 *   roots are the least elements of their sets: a link hooks the larger of
 *   two roots under the smaller by a compare-and-swap, trying again from the
 *   new roots if another thread moved first (the hooks of Shiloach and
 *   Vishkin, which never form a cycle), and the paths are split as they are
 *   walked. Each node first links its first 2 neighbors, all the trees are
 *   compressed, a sample of 1024 nodes finds the largest component
 *   \f$ c \f$, and then only the nodes outside \f$ c \f$ link the rest of
 *   their neighbors: in an unoriented graph every arc between \f$ c \f$ and
 *   another component is also seen from the other side, so that the arcs
 *   inside the giant component, usually most of them, are skipped. An
 *   oriented graph is taken as unoriented, its arcs read from their tails
 *   only, and no node is skipped.
 *
 * Both take \f$ O( n + m ) \f$ work; the visit has depth
 * \f$ O( D \log n ) \f$ for diameter \f$ D \f$ (the gathering of the
 * frontiers), the components \f$ O( \log n ) \f$ expected rounds of hooks
 * on random graphs.
 *
 * The contracts [see `Contract.h`]: \f$ s \f$ is a node; `gt`, if given, is
 * the transpose of \f$ g \f$. The functions are defined in
 * `ParallelGraph.tpp`.
 *
 * \see S. Beamer, K. Asanovic, D. Patterson, Direction-optimizing
 *      breadth-first search, International Conference for High
 *      Performance Computing, Networking, Storage and Analysis (SC), 1-10,
 *      2012, doi:10.1109/SC.2012.50
 * \see M. Sutton, T. Ben-Nun, A. Barak, Optimizing parallel graph
 *      connectivity computation via subgraph sampling, IEEE International
 *      Parallel and Distributed Processing Symposium (IPDPS), 12-21, 2018,
 *      doi:10.1109/IPDPS.2018.00012
 * \see Y. Shiloach, U. Vishkin, An \f$ O( \log n ) \f$ parallel connectivity
 *      algorithm, Journal of Algorithms 3(1), 57-67, 1982,
 *      doi:10.1016/0196-6774(82)90008-6
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

#ifndef __ADS_ParallelGraph
 #define __ADS_ParallelGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the distances from \f$ s \f$ by a visit in breadth that optimizes its
/// direction, \f$ -1 \f$ for the nodes not reached; `gt` the transpose of
/// \f$ g \f$ if \f$ g \f$ is oriented (`nullptr`: the visit goes from the
/// top down only)
///
/// In pseudocode (the visit of Beamer, Asanovic and Patterson, beyond the
/// book), \f$ \mathrm{in}( v ) \f$ the arcs into \f$ v \f$, of \f$ g \f$ if
/// unoriented and of `gt` if oriented:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bfsDistances}( \mathit{ex} , G , s ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets -1 \cr
/// 3 & \quad d[ s ] \gets 0;\ F \gets ( s );\ \ell \gets 0;\ m_u \gets m;
///     \ m_f \gets | \mathrm{adj}( s ) | \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{in} \text{ is known}\ \textbf{and}\ m_f >
///     m_u / 15\ \textbf{then}\quad \triangleright\ \text{from the bottom }
///     \text{up} \cr
/// 6 & \qquad\quad \textbf{repeat} \cr
/// 7 & \qquad\qquad F' \gets \emptyset \cr
/// 8 & \qquad\qquad \textbf{foreach}\ v\ \textbf{with}\ d[ v ] = -1
///     \ \textbf{do in parallel} \cr
/// 9 & \qquad\qquad\quad \textbf{if}\ \text{some } u \in \mathrm{in}( v )
///     \text{ is in } F \text{ (the first found)}\ \textbf{then}\ d[ v ]
///     \gets \ell + 1;\ F' \gets F' \cup \{ v \} \cr
/// 10 & \qquad\qquad \ell \gets \ell + 1;\ F \gets F' \cr
/// 11 & \qquad\quad \textbf{until}\ F = \emptyset\ \textbf{or}\ ( |F|
///     \text{ shrank}\ \textbf{and}\ |F| \le n / 18 ) \cr
/// 12 & \qquad\quad \textbf{if}\ F = \emptyset\ \textbf{then return}\ d \cr
/// 13 & \qquad\quad m_f \gets 1;\ \textbf{continue} \cr
/// 14 & \qquad m_u \gets m_u - m_f;\ F' \gets ();\ m_f \gets 0\quad
///     \triangleright\ \text{from the top down} \cr
/// 15 & \qquad \textbf{foreach}\ u \in F , v \in G.\mathrm{adj}( u )
///     \ \textbf{do in parallel} \cr
/// 16 & \qquad\quad \textbf{if}\ \mathrm{compareAndSwap}( d[ v ] , -1 , \ell
///     + 1 )\ \textbf{then}\ \text{append } v \text{ to } F';\ m_f \gets m_f
///     + | \mathrm{adj}( v ) | \cr
/// 17 & \qquad F \gets F';\ \ell \gets \ell + 1 \cr
/// 18 & \qquad \textbf{if}\ F = \emptyset\ \textbf{then return}\ d
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a node of \f$ g \f$, `gt` has as many nodes if given
template< Executor E , class W >
[[nodiscard]] std::vector< int > bfsDistances( E & ex ,
                                               const StaticGraph< W > & g ,
                                               int s ,
                                               const StaticGraph< W > * gt =
                                                nullptr );

/// the least node of the component of each node, an oriented graph taken
/// as unoriented
///
/// In pseudocode (the sampling of Afforest of Sutton, Ben-Nun and Barak,
/// beyond the book), on a concurrent MFSET whose roots are the least nodes
/// of their sets:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{connectedComponents}( \mathit{ex} , G ) \cr
/// 2 & \quad M \gets \text{an MFSET of the nodes, each alone} \cr
/// 3 & \quad \textbf{for}\ r \gets 1\ \textbf{to}\ 2\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ v \in V\ \textbf{do in parallel}
///     \ \textbf{if}\ v \text{ has an } r \text{-th arc } ( v , u )
///     \ \textbf{then}\ \mathrm{merge}( M , v , u ) \cr
/// 5 & \qquad \textbf{foreach}\ v \in V\ \textbf{do in parallel}\ \text{make }
///     \text{the root of } v \text{ its parent} \cr
/// 6 & \quad c \gets \textbf{nil} \cr
/// 7 & \quad \textbf{if}\ G \text{ is unoriented}\ \textbf{then}\ c \gets
///     \text{the root most frequent among 1024 random nodes} \cr
/// 8 & \quad \textbf{foreach}\ v \in V\ \textbf{with}\ \text{parent}
///     \text{ of } v \ne c\ \textbf{do in parallel} \cr
/// 9 & \qquad \textbf{foreach}\ \text{arc } ( v , u ) \text{ after the }
///     \text{first two}\ \textbf{do}\ \mathrm{merge}( M , v , u ) \cr
/// 10 & \quad \textbf{return}\ \text{the root } \mathrm{find}( M , v )
///     \text{ of each node } v
/// \end{array}}
/// \f]
template< Executor E , class W >
[[nodiscard]] std::vector< int > connectedComponents(
                                 E & ex , const StaticGraph< W > & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelGraph.h ------------------------*/
/*--------------------------------------------------------------------------*/
