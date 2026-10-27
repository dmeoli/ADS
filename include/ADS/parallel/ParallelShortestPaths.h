/*--------------------------------------------------------------------------*/
/*---------------------- File ParallelShortestPaths.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel shortest paths from a node \f$ s \f$ of a GRAPH whose costs
 * are not negative, beyond the book, on the threads of an executor [see
 * `Executor.h`]: the stepping algorithms, which relax in parallel all the
 * active nodes whose distance is below a threshold, with the distances of
 * the algorithm of Dijkstra [see `ShortestPaths.h`], value for value.
 *
 * A node is active when its tentative distance \f$ d( v ) \f$ fell since
 * its arcs were last relaxed; a step takes a threshold \f$ \theta \f$,
 * extracts the active nodes with \f$ d( v ) \le \theta \f$ and relaxes
 * their arcs, each improvement of a head by an atomic minimum
 * (\f$ d( u ) \gets \min\{ d( u ) , d( v ) + w( v , u ) \} \f$ by a
 * compare-and-swap) and each head improved made active once, by a flag;
 * the steps end when no node is active. The rule of the threshold gives the
 * algorithm (Dong, Gu, Sun and Zhang):
 *
 * - `deltaStepping( ex , g , s , delta )`, the buckets of width
 *   \f$ \Delta \f$ of Meyer and Sanders: \f$ \theta \f$ closes the bucket
 *   \f$ \lfloor \mu / \Delta \rfloor \f$ of the least active distance
 *   \f$ \mu \f$, so that a bucket is relaxed again while its nodes improve
 *   each other and the next one opens when it is empty; \f$ \Delta \f$ at
 *   the least cost gives the algorithm of Dijkstra in rounds, \f$ \Delta =
 *   \infty \f$ the one of Bellman, Ford and Moore by rounds;
 * - `rhoStepping( ex , g , s , rho )`, the \f$ \rho \f$-stepping of Dong
 *   et al.: \f$ \theta \f$ is the \f$ \rho \f$-th least distance of the
 *   active nodes, estimated on a sample of them, so that each step relaxes
 *   about \f$ \rho \f$ nodes whatever the costs, with no parameter that
 *   depends on their scale.
 *
 * The arcs of the extracted nodes are cut into blocks of equal numbers of
 * arcs, by the prefix sums of their degrees, so that a node of large degree
 * is relaxed by several threads (a `StaticGraph`; on the other graphs each
 * node goes whole to one block), and the nodes that a block makes active
 * within the threshold of its step are relaxed by the same block at once,
 * up to 1024 of them (the fusion of the buckets of Zhang et al.), which
 * saves the steps of the chains of light arcs. The distances do not depend
 * on the executor nor on the order of the relaxations: they are the least
 * fixed point of \f$ d( u ) = \min_{( v , u )} d( v ) + w( v , u ) \f$
 * with \f$ d( s ) = 0 \f$, where each sum is the one of the computer (for the
 * floating points too, since a sum with a cost that is not negative does
 * not decrease, the property on which the algorithm of Dijkstra rests; see
 * Lemma 5 of \ref theory_parallel_sssp).
 *
 * Each step costs work proportional to the arcs of its nodes, plus
 * \f$ O( |A| ) \f$ for the active set \f$ A \f$, and depth
 * \f$ O( \log n ) \f$; a node is extracted once with Dijkstra, more often
 * here, as many times as its distance falls after it was extracted, which a
 * smaller \f$ \Delta \f$ or \f$ \rho \f$ makes rarer at the price of more
 * steps (the trade of the stepping algorithms; see \ref
 * theory_parallel_sssp).
 *
 * The contracts [see `Contract.h`]: \f$ s \f$ is a node, the costs are not
 * negative, \f$ \Delta > 0 \f$ and \f$ \rho \ge 1 \f$; the distances are
 * sums in the type of the costs, not checked for overflow, as in
 * `ShortestPaths.h`, and `infinity()` marks the nodes not reached. The
 * functions are defined in `ParallelShortestPaths.tpp`.
 *
 * \see U. Meyer, P. Sanders, \f$ \Delta \f$-stepping: a parallelizable
 *      shortest path algorithm, Journal of Algorithms 49(1), 114-152, 2003,
 *      doi:10.1016/S0196-6774(03)00076-2
 * \see X. Dong, Y. Gu, Y. Sun, Y. Zhang, Efficient stepping algorithms and
 *      implementations for parallel shortest paths, Proc. 33rd SPAA,
 *      184-197, 2021, doi:10.1145/3409964.3461782
 * \see Y. Zhang, A. Brahmakshatriya, X. Chen, L. Dhulipala, S. Kamil,
 *      S. Amarasinghe, J. Shun, Optimizing ordered graph algorithms with
 *      GraphIt, Proc. 18th CGO, 158-170, 2020, doi:10.1145/3368826.3377909
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

#ifndef __ADS_ParallelShortestPaths
 #define __ADS_ParallelShortestPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"
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

/// the distances from \f$ s \f$ by the \f$ \Delta \f$-stepping of Meyer and
/// Sanders, `infinity()` for the nodes not reached
///
/// In pseudocode (Meyer and Sanders, with the steps of Dong, Gu, Sun and
/// Zhang, beyond the book), \f$ A \f$ the active nodes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{deltaStepping}( \mathit{ex} , G , s ,
///     \Delta ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty \cr
/// 3 & \quad d[ s ] \gets 0;\ A \gets \{ s \} \cr
/// 4 & \quad \textbf{while}\ A \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad \mu \gets \min_{v \in A} d[ v ]\quad \triangleright\ \text{a}
///     \text{ parallel reduction} \cr
/// 6 & \qquad \mathrm{step}( \{ v \in A : \lfloor d[ v ] / \Delta \rfloor
///     \le \lfloor \mu / \Delta \rfloor \} ) \cr
/// 7 & \quad \textbf{return}\ d \cr
/// 8 & \textbf{procedure}\ \mathrm{step}( F ) \cr
/// 9 & \quad A \gets A \setminus F \cr
/// 10 & \quad \textbf{foreach}\ v \in F , ( v , u ) \in E\ \textbf{do in}
///     \ \textbf{parallel} \cr
/// 11 & \qquad \textbf{if}\ \mathrm{writeMin}( d[ u ] , d[ v ] + w( v , u ) )
///     \ \textbf{and}\ u \notin A\ \textbf{then}\ A \gets A \cup \{ u \}
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a node, the costs are not negative, \f$ \Delta > 0 \f$
template< Executor E , Graph G >
[[nodiscard]] std::vector< typename G::Weight > deltaStepping(
                              E & ex , const G & g , int s ,
                              typename G::Weight delta );

/// the distances from \f$ s \f$ by the \f$ \rho \f$-stepping of Dong, Gu,
/// Sun and Zhang, `infinity()` for the nodes not reached
///
/// In pseudocode (Dong, Gu, Sun and Zhang, beyond the book), with
/// \f$ \mathrm{step}() \f$ that of `deltaStepping()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rhoStepping}( \mathit{ex} , G , s , \rho )
///     \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty \cr
/// 3 & \quad d[ s ] \gets 0;\ A \gets \{ s \} \cr
/// 4 & \quad \textbf{while}\ A \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ |A| \le \rho\ \textbf{then}\ \theta \gets \infty
///     \cr
/// 6 & \qquad \textbf{else}\ \theta \gets \text{the } \lceil 1024 \rho / |A|
///     \rceil \text{-th least of the distances of } 1024 \text{ nodes of } A
///     \cr
/// 7 & \qquad \mathrm{step}( \{ v \in A : d[ v ] \le \theta \} ) \cr
/// 8 & \quad \textbf{return}\ d
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a node, the costs are not negative, \f$ \rho \ge 1 \f$
template< Executor E , Graph G >
[[nodiscard]] std::vector< typename G::Weight > rhoStepping(
                              E & ex , const G & g , int s ,
                              std::size_t rho = std::size_t( 1 ) << 20 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelShortestPaths.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelShortestPaths.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ParallelShortestPaths.h --------------------*/
/*--------------------------------------------------------------------------*/
