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
 * Both relax the arcs of nodes in parallel, each improvement of a head by
 * an atomic minimum (\f$ d( u ) \gets \min\{ d( u ) , d( v ) + w( v , u )
 * \} \f$ by a compare-and-swap), and differ in the nodes that a step
 * relaxes:
 *
 * - `deltaStepping( ex , g , s , delta )`, the buckets of width
 *   \f$ \Delta \f$ of Meyer and Sanders, a step relaxing the bucket of the
 *   least distance not yet relaxed. It runs as the Delta-stepping of the GAP
 *   benchmark suite (Beamer and Zhang): each thread keeps the nodes that it
 *   improves in buckets of its own, a step relaxes the shared frontier of
 *   the bucket \f$ c \f$ (the threads claim 64 nodes at a time) and then
 *   each thread relaxes at once its own bucket \f$ c \f$ while it holds
 *   fewer than 1000 nodes (the fusion of the buckets of Zhang et al.); the
 *   threads vote the next bucket, copy their part of it into the frontier,
 *   each at a place of its own, and two barriers end the step. The buckets
 *   of a thread are a window of 1024 buckets and an overflow past it,
 *   spread over the next window when it opens (the bucketing of Julienne,
 *   of Dhulipala, Blelloch and Shun), so that a small \f$ \Delta \f$ takes
 *   no more memory. A node is in a bucket once per improvement, and
 *   skipped when its distance has fallen below the bucket; from the step
 *   where the entries put back into the bucket of the step are more than a
 *   quarter of those relaxed (a bucket wider than the least costs, where a
 *   node improves many times), the distance at which each node was last
 *   relaxed is kept, and an entry met again at the same distance relaxes
 *   nothing.
 *   \f$ \Delta \f$ at the least cost gives the algorithm of Dijkstra by
 *   buckets, the least work and the most steps;
 * - `rhoStepping( ex , g , s , rho )`, the \f$ \rho \f$-stepping of Dong
 *   et al.: a node is active when its distance fell since its arcs were
 *   last relaxed, made active once by a flag, and a step relaxes the
 *   active nodes up to the \f$ \rho \f$-th least distance among them,
 *   estimated on a sample, so that each step relaxes about \f$ \rho \f$
 *   nodes whatever the costs, with no parameter that depends on their
 *   scale; the arcs of a step are cut into blocks of equal numbers of arcs
 *   by the prefix sums of the degrees, and the nodes that a block makes
 *   active within the threshold are relaxed by the same block, up to 1024
 *   of them.
 *
 * The threads of `deltaStepping()` meet at barriers for the whole run, which
 * a thread joins when the executor starts its call: the barrier counts it
 * at its next release and never waits for a call not started, so that the
 * run ends on any executor, one that runs the calls one after the other
 * included. The distances do not depend on the executor nor on the order
 * of the relaxations: they are the least fixed point of
 * \f$ d( u ) = \min_{( v , u )} d( v ) + w( v , u ) \f$ with
 * \f$ d( s ) = 0 \f$, where each sum is the one of the computer (for the
 * floating points too, since a sum with a cost that is not negative does
 * not decrease, the property on which the algorithm of Dijkstra rests; see
 * Lemma 5 of \ref theory_parallel_sssp).
 *
 * Each step costs work proportional to the arcs of its nodes and depth
 * proportional to the arcs of one node, a chunk of the frontier and the
 * fusion of one thread; a node is relaxed once with Dijkstra, more often
 * here, as many times as its distance falls after it was relaxed, which a
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
 * \see S. Beamer, K. Asanović, D. Patterson, The GAP benchmark suite,
 *      arXiv:1508.03619, 2015
 * \see L. Dhulipala, G. E. Blelloch, J. Shun, Julienne: a framework for
 *      parallel graph algorithms using work-efficient bucketing, Proc. 29th
 *      SPAA, 293-304, 2017, doi:10.1145/3087556.3087580
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
/// In pseudocode (Meyer and Sanders, on the buckets of each thread of the
/// GAP benchmark suite, beyond the book), \f$ B_k[ b ] \f$ the bucket
/// \f$ b \f$ of the thread \f$ k \f$ and \f$ \beta( x ) = \lfloor x /
/// \Delta \rfloor \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{deltaStepping}( \mathit{ex} , G , s ,
///     \Delta ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ d[ v ] \gets \infty \cr
/// 3 & \quad d[ s ] \gets 0;\ F \gets ( s );\ c \gets 0 \cr
/// 4 & \quad \text{on each thread } k \text{ of } \mathit{ex} \text{, from}
///     \text{ the barrier where it joins,}\ \textbf{loop} \cr
/// 5 & \qquad \textbf{foreach}\ v \in F\ \textbf{with}\ \beta( d[ v ] )
///     \ge c\ \textbf{do in parallel}\ \mathrm{relax}( k , v )\quad
///     \triangleright\ \text{64 at a time} \cr
/// 6 & \qquad \textbf{while}\ 0 < | B_k[ c ] | < 1000\ \textbf{do}
///     \ \textbf{foreach}\ v\ \text{taken from } B_k[ c ]\ \textbf{do}
///     \ \mathrm{relax}( k , v ) \cr
/// 7 & \qquad \text{barrier: } c \gets \min_k \min\{ b \ge c : B_k[ b ]
///     \ne \emptyset \};\ \textbf{if}\ \text{there is none}\ \textbf{then}
///     \ \textbf{return}\ d \cr
/// 8 & \qquad F \gets \text{the } B_k[ c ] \text{ of all the threads, each}
///     \text{ at a place of its own};\ B_k[ c ] \gets \emptyset \cr
/// 9 & \qquad \text{barrier} \cr
/// 10 & \textbf{procedure}\ \mathrm{relax}( k , v ) \cr
/// 11 & \quad \textbf{if}\ \text{the distances are kept}\ \textbf{then} \cr
/// 12 & \qquad \textbf{if}\ \mathit{last}[ v ] = d[ v ]\ \textbf{then}
///     \ \textbf{return};\ \mathit{last}[ v ] \gets d[ v ] \cr
/// 13 & \quad \textbf{foreach}\ ( v , u ) \in E\ \textbf{do} \cr
/// 14 & \qquad \textbf{if}\ \mathrm{writeMin}( d[ u ] , d[ v ] + w( v , u )
///     )\ \textbf{then}\ \text{append } u \text{ to } B_k[ \beta( d[ v ] + w(
///     v , u ) ) ]
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
/// In pseudocode (Dong, Gu, Sun and Zhang, beyond the book), \f$ A \f$
/// the active nodes:
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
/// 7 & \qquad F \gets \{ v \in A : d[ v ] \le \theta \};\ A \gets A
///     \setminus F \cr
/// 8 & \qquad \textbf{foreach}\ v \in F , ( v , u ) \in E\ \textbf{do in}
///     \ \textbf{parallel} \cr
/// 9 & \qquad\quad \textbf{if}\ \mathrm{writeMin}( d[ u ] , d[ v ] + w( v ,
///     u ) )\ \textbf{and}\ u \notin A\ \textbf{then}\ A \gets A \cup \{
///     u \} \cr
/// 10 & \quad \textbf{return}\ d
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
