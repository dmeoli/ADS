/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel visits of a StaticGraph [see StaticGraph.h], beyond the
 * book, on the threads of an executor [see Executor.h]: the distances of
 * a visit in breadth and the connected components, with the same results
 * as the sequential visits.
 * - bfsDistances( ex , g , s ) is the visit in breadth from s that
 *   optimizes its direction, as Beamer, Asanovic and Patterson: a level
 *   goes from the top down (each node of the frontier claims, by an
 *   atomic or on a bitmap of the nodes reached, its neighbours not yet
 *   reached, and
 *   the nodes claimed by each block of the frontier are gathered into the
 *   next one) while the frontier is small, and from the bottom up (each
 *   node not yet reached looks for a neighbour in the frontier, kept as a
 *   bitmap, and stops at the first it finds) when the arcs out of the
 *   frontier \f$ m_f \f$ exceed the arcs still to check \f$ m_u \f$
 *   divided by \f$ \alpha = 15 \f$; it goes back from the top down when
 *   the frontier shrinks below \f$ n / \beta \f$, \f$ \beta = 18 \f$. The
 *   steps from the bottom up skip most of the arcs of the large levels of
 *   the graphs of small diameter, e.g., the social networks and the
 *   graphs of Kronecker of the Graph 500, where a level may hold most of
 *   the nodes. Both directions test whether a node was reached on the
 *   bitmap, a bit per node, instead of on its distance (the visit of
 *   GAPBS tests the parents, 4 bytes per node): the bitmap of \f$ 2^{19}
 *   \f$ nodes takes 64 KB, in the cache of second level of each core.
 *   The steps from the bottom up read the arcs into each node, i.e.,
 *   those of g if it is unoriented, of its transpose (given as gt)
 *   if it is oriented; with no transpose an oriented graph is visited from
 *   the top down only. The distance of a node is its level, the same
 *   whoever claims it: -1 for the nodes not reached.
 * - connectedComponents( ex , g ) labels each node with the least node of
 *   its component, by the sampling of Afforest (Sutton, Ben-Nun and
 *   Barak) on the concurrent union-find of ADS [see
 *   ConcurrentUnionFind.h], whose roots are the least elements of their
 *   sets: a link hooks the larger of two roots under the smaller by a
 *   compare-and-swap, trying again from the new roots if another thread
 *   moved first (the hooks of Shiloach and Vishkin, which never form a
 *   cycle), and the paths are split as they are walked. Each node first
 *   links its first 2 neighbours, all the trees are compressed, a sample
 *   of 1024 nodes finds the largest component c, and then only the nodes
 *   outside c link the rest of their neighbours: in an unoriented graph
 *   every arc between c and another component is also seen from the other
 *   side, so that the arcs inside the giant component, usually most of
 *   them, are skipped. An oriented graph is taken as unoriented, its
 *   arcs read from their tails only, and no node is skipped.
 * Both take \f$ O( n + m ) \f$ work; the visit has depth \f$ O( D \log n
 * ) \f$ for diameter D (the gathering of the frontiers), the components
 * \f$ O( \log n ) \f$ expected rounds of hooks on random graphs.
 *
 * The contracts [see Contract.h]: s is a node; gt, if given, is the
 * transpose of g.
 * \see S. Beamer, K. Asanovic, D. Patterson, Direction-optimizing
 *      breadth-first search, International Conference for High
 *      Performance Computing, Networking, Storage and Analysis (SC), 1-10,
 *      2012, doi:10.1109/SC.2012.50
 * \see M. Sutton, T. Ben-Nun, A. Barak, Optimizing parallel graph
 *      connectivity computation via subgraph sampling, IEEE International
 *      Parallel and Distributed Processing Symposium (IPDPS), 12-21, 2018,
 *      doi:10.1109/IPDPS.2018.00012
 * \see Y. Shiloach, U. Vishkin, An O( log n ) parallel connectivity
 *      algorithm, Journal of Algorithms 3(1), 57-67, 1982,
 *      doi:10.1016/0196-6774(82)90008-6
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// the distances from s by a visit in breadth that optimizes its
/// direction, -1 for the nodes not reached; gt the transpose of g if g is
/// oriented (nullptr: the visit goes from the top down only)
/// \pre s is a node of g
template< Executor E , class W >
[[nodiscard]] std::vector< int > bfsDistances( E & ex ,
                                               const StaticGraph< W > & g ,
                                               int s ,
                                               const StaticGraph< W > * gt =
                                                nullptr );

/// the least node of the component of each node, an oriented graph taken
/// as unoriented
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
