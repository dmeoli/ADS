/*--------------------------------------------------------------------------*/
/*------------------------ File StrongComponents.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The strongly connected components of an oriented graph by one visit in
 * depth, beyond the book, whose visits and components by the algorithm of
 * Kosaraju are in `Visit.h` [see `Visit.h`, which includes this header]:
 *
 * - `sccTarjan()` and `sccPearce()`, the components of `scc()` by one visit in
 *   depth and no transposed graph (beyond the book): the algorithm of
 *   Tarjan keeps the order of discovery and the lowest one reachable
 *   through the tree and one more edge of each node, and pops a component
 *   off a stack when its first node finishes with the two equal; the
 *   variant of Pearce keeps one integer per node instead of three, the two
 *   numbers sharing a place and the closed components taking numbers that
 *   count down from n - 1 above those in use, and one bit per node. Both
 *   number the components as `scc()` does, in a topological order of the
 *   graph of the components.
 *
 * The visits in depth keep their recursion on an explicit stack, as those
 * of `Visit.h`. The contracts [see `Contract.h`]: at the level of the
 * audits each node has a component and the numbers of the components are
 * a topological order of their graph. The functions are defined in
 * `StrongComponents.tpp`.
 *
 * \see R. E. Tarjan, Depth-first search and linear graph algorithms, SIAM
 *      Journal on Computing 1(2), 146-160, 1972, doi:10.1137/0201010
 * \see D. J. Pearce, A space-efficient algorithm for finding strongly
 *      connected components, Information Processing Letters 116(1), 47-52,
 *      2016, doi:10.1016/j.ipl.2015.08.010
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

#ifndef __ADS_StrongComponents
 #define __ADS_StrongComponents
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------- THE COMPONENTS BY ONE VISIT -----------------------*/
/*--------------------------------------------------------------------------*/

/// the strongly connected components of `scc()` by one visit in depth, with
/// no transposed graph, numbered as there
///
/// In pseudocode (Tarjan's algorithm, beyond the book):
/// \f$ \mathit{low}[ u ] \f$ is the least order of discovery of a node of
/// the stack \f$ S \f$ that the subtree of \f$ u \f$ reaches by one more
/// edge, and \f$ u \f$ is the first node of its component if and only if
/// \f$ \mathit{low}[ u ] = \mathit{dt}[ u ] \f$ when \f$ u \f$ finishes
/// (see \ref theory_graph_visits); the recursion is kept on an explicit
/// stack.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sccTarjan}( G ) \cr
/// 2 & \quad t \gets 0;\ k \gets 0;\ S \gets \text{an empty stack};
///     \ \mathit{dt}[ u ] \gets 0 , C[ u ] \gets 0\ \forall u \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{dt}[ r ]
///     = 0\ \textbf{do}\ \mathrm{visit}( r ) \cr
/// 4 & \quad \textbf{return}\ C\ \text{with each } c \text{ replaced by }
///     k + 1 - c \cr
/// 5 & \textbf{procedure}\ \mathrm{visit}( u ) \cr
/// 6 & \quad t \gets t + 1;\ \mathit{dt}[ u ] \gets t;\ \mathit{low}[ u ]
///     \gets t;\ \mathrm{push}( S , u ) \cr
/// 7 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ \mathit{dt}[ v ] = 0\ \textbf{then}
///     \ \mathrm{visit}( v );\ \mathit{low}[ u ] \gets \min( \mathit{low}[ u ]
///     , \mathit{low}[ v ] ) \cr
/// 9 & \qquad \textbf{else if}\ C[ v ] = 0\ \textbf{then}\ \mathit{low}[ u ]
///     \gets \min( \mathit{low}[ u ] , \mathit{dt}[ v ] )\quad
///     \triangleright\ v \in S \cr
/// 10 & \quad \textbf{if}\ \mathit{low}[ u ] = \mathit{dt}[ u ]
///     \ \textbf{then} \cr
/// 11 & \qquad k \gets k + 1 \cr
/// 12 & \qquad \textbf{repeat}\ v \gets \mathrm{pop}( S );\ C[ v ] \gets k
///     \ \textbf{until}\ v = u
/// \end{array}}
/// \f]
/// \post each node has a component, and an edge from the component
///       \f$ C \f$ to \f$ C' \ne C \f$ gives \f$ C < C' \f$ (audit)
template< Graph G >
std::vector< int > sccTarjan( const G & g );

/// the strongly connected components of `sccTarjan()` with one integer and
/// one bit per node in place of three integers, numbered as `scc()` does
///
/// In pseudocode (Pearce's algorithm, beyond the book):
/// \f$ \mathit{rindex}[ u ] \f$ is the order of discovery of \f$ u \f$
/// until a node of a lower one is reached, which it then takes; a closed
/// component takes the number \f$ c \f$, which counts down from
/// \f$ N - 1 \f$ (\f$ N \f$ the `bound()`) and is never below a number
/// in use, since \f$ i \f$ gives back the numbers of the nodes closed. A
/// test of \f$ \mathit{rindex} \f$ alone thus tells a node of the stack
/// from a closed one (see \ref theory_graph_visits).
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sccPearce}( G ) \cr
/// 2 & \quad i \gets 1;\ c \gets N - 1;\ S \gets \text{an empty stack};
///     \ \mathit{rindex}[ u ] \gets 0\ \forall u \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ \mathit{rindex}[ r ]
///     = 0\ \textbf{do}\ \mathrm{visit}( r ) \cr
/// 4 & \quad \textbf{return}\ \mathit{rindex}[ u ] - c\ \forall u \cr
/// 5 & \textbf{procedure}\ \mathrm{visit}( u ) \cr
/// 6 & \quad \mathit{root}[ u ] \gets \textbf{true};\ \mathit{rindex}[ u ]
///     \gets i;\ i \gets i + 1 \cr
/// 7 & \quad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ \mathit{rindex}[ v ] = 0\ \textbf{then}
///     \ \mathrm{visit}( v ) \cr
/// 9 & \qquad \textbf{if}\ \mathit{rindex}[ v ] < \mathit{rindex}[ u ]
///     \ \textbf{then}\ \mathit{rindex}[ u ] \gets \mathit{rindex}[ v ];
///     \ \mathit{root}[ u ] \gets \textbf{false} \cr
/// 10 & \quad \textbf{if not}\ \mathit{root}[ u ]\ \textbf{then}
///     \ \mathrm{push}( S , u );\ \textbf{return} \cr
/// 11 & \quad i \gets i - 1 \cr
/// 12 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{and}
///     \ \mathit{rindex}[ u ] \le \mathit{rindex}[ \mathrm{top}( S ) ]
///     \ \textbf{do} \cr
/// 13 & \qquad \mathit{rindex}[ \mathrm{pop}( S ) ] \gets c;\ i \gets i - 1
///     \cr
/// 14 & \quad \mathit{rindex}[ u ] \gets c;\ c \gets c - 1
/// \end{array}}
/// \f]
/// \post each node has a component, and an edge from the component
///       \f$ C \f$ to \f$ C' \ne C \f$ gives \f$ C < C' \f$ (audit)
template< Graph G >
std::vector< int > sccPearce( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/StrongComponents.tpp"

/*--------------------------------------------------------------------------*/

#endif /* StrongComponents.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File StrongComponents.h -----------------------*/
/*--------------------------------------------------------------------------*/
