/*--------------------------------------------------------------------------*/
/*-------------------------- File EulerianCycle.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Eulerian cycles and trails of a graph, oriented or not: a trail is a
 * path that uses each edge exactly once, a cycle a closed trail.
 *
 * - `eulerianCycle()`, the closed trail (exercise 15.3, for an oriented
 *   graph): by the theorem of Euler it exists if and only if each node has
 *   as many edges entering as leaving (an even degree, if the graph is not
 *   oriented) and all the edges are in one component, the orientation
 *   ignored. The book finds it by a local search on the partitions of the
 *   edges into closed paths, each edge choosing its successor and a swap of
 *   two successors merging two paths; here the algorithm of Hierholzer
 *   follows unused edges from a node until it comes back, and splices in
 *   the closed paths found from the nodes along the way, in
 *   \f$ O( n + m ) \f$ time;
 * - `eulerianTrail()`, the trail, closed if there is a cycle and open
 *   otherwise: an open trail exists if and only if the edges are in one
 *   component and exactly one node s has one edge leaving more than
 *   entering and one node t one edge entering more than leaving, the
 *   others balanced (exactly two nodes of odd degree, if the graph is not
 *   oriented), and it goes from s to t; the same walk of Hierholzer, from
 *   s, finds it. A node may also be given as the start;
 * - `isEulerianCycle()` and `isEulerianTrail()`, which check a sequence of
 *   nodes.
 *
 * The connection of the edges is not tested apart: under the conditions on
 * the degrees the walk of Hierholzer uses every edge reachable from its
 * start, which is every edge if and only if they are in one component, so
 * that a walk shorter than m edges tells that there is no trail. An
 * unoriented edge is used in one direction, the two arcs of the adjacency
 * paired in their order, so that parallel edges are distinct edges. The
 * theory is in \ref theory_graph_euler.
 *
 * The contracts [see `Contract.h`]: a start given is a node; an unoriented
 * graph has no loop; at the level of the audits, the trail found uses each
 * edge exactly once, which `isEulerianTrail()` checks. The functions are
 * defined in `EulerianCycle.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 15.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see C. Hierholzer, C. Wiener, Ueber die Moeglichkeit, einen
 *      Linienzug ohne Wiederholung und ohne Unterbrechung zu umfahren,
 *      Mathematische Annalen 6(1), 30-32, 1873, doi:10.1007/BF01442866
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

#ifndef __ADS_EulerianCycle
 #define __ADS_EulerianCycle
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the nodes of an Eulerian cycle of the graph, the first node repeated at
/// the end, or nothing if there is none; empty with no edge
///
/// In pseudocode (Hierholzer's algorithm, beyond the book): the walk from a
/// node with unused edges always ends where it started, since every node has
/// as many edges entering as leaving, and the walks found along the way are
/// spliced in.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{eulerianCycle}( G ) \cr
/// 2 & \quad \textbf{if}\  \text{a node has } \mathit{indeg} \ne
///     \mathit{outdeg} \text{ (an odd degree, if unoriented) }
///     \ \textbf{then return}\ \textbf{nil} \cr
/// 3 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ () \cr
/// 4 & \quad C \gets \mathrm{walk}( G , \text{the last node with an edge}
///     )\quad \triangleright\ \text{as } \mathrm{eulerianTrail} \cr
/// 5 & \quad \textbf{if}\ |C| = m + 1\ \textbf{then return}\ C
///     \ \textbf{else return}\ \textbf{nil}\quad \triangleright
///     \ \text{the edges are not connected}
/// \end{array}}
/// \f]
/// \pre no loop if the graph is unoriented
/// \post the cycle is Eulerian (audit)
template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g );

/// the nodes of an Eulerian trail of the graph, closed (the first node
/// repeated at the end) if there is an Eulerian cycle and open otherwise, or
/// nothing if there is none; empty with no edge
///
/// In pseudocode (Hierholzer's algorithm, beyond the book): the start
/// \f$ s \f$ is the node with one edge leaving more than entering (with an
/// odd degree, if unoriented, the first of the two), or the last node with
/// an edge if there is none; the walk from a node with unused edges can only
/// stop at the end of the trail, and each node is written when it has no
/// unused edge left, so that the trail comes out backwards.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{eulerianTrail}( G ) \cr
/// 2 & \quad \textbf{if}\  \text{the degrees allow no trail }
///     \ \textbf{then return}\ \textbf{nil}\quad \triangleright
///     \ \text{more than two unbalanced nodes} \cr
/// 3 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ () \cr
/// 4 & \quad S \gets ( s );\ C \gets () \cr
/// 5 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad u \gets \mathrm{top}( S ) \cr
/// 7 & \qquad \textbf{if}\ u \text{ has an unused edge } e = ( u , v )
///     \ \textbf{then} \cr
/// 8 & \qquad\quad \text{mark } e \text{ used (both its arcs, if }
///     \text{unoriented)};\ \mathrm{push}( S , v ) \cr
/// 9 & \qquad \textbf{else} \cr
/// 10 & \qquad\quad \mathrm{pop}( S );\ C \gets ( u ) \cdot C \cr
/// 11 & \quad \textbf{if}\ |C| = m + 1\ \textbf{then return}\ C
///     \ \textbf{else return}\ \textbf{nil}
/// \end{array}}
/// \f]
/// \pre no loop if the graph is unoriented
/// \post the trail is Eulerian (audit)
template< Graph G >
std::optional< std::vector< int > > eulerianTrail( const G & g );

/// the Eulerian trail of `eulerianTrail()` above from the node \f$ s \f$,
/// or nothing if no Eulerian trail starts from it; empty with no edge
///
/// In pseudocode: the walk of `eulerianTrail()` from \f$ s \f$, if
/// \f$ s \f$ has an edge and either no node is unbalanced or \f$ s \f$ is
/// the start of the open trail (either node of odd degree, if unoriented).
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{eulerianTrail}( G , s ) \cr
/// 2 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ () \cr
/// 3 & \quad \textbf{if}\ s \text{ can start no trail }\ \textbf{then}
///     \ \textbf{return}\ \textbf{nil} \cr
/// 4 & \quad \textbf{return}\ \text{the walk of lines 4 to 11 of }
///     \mathrm{eulerianTrail}( G ) \text{ from } s
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a node, and no loop if the graph is unoriented
/// \post the trail is Eulerian and starts from \f$ s \f$ (audit)
template< Graph G >
std::optional< std::vector< int > > eulerianTrail( const G & g , int s );

/// whether the nodes \f$ c \f$, the first repeated at the end, are a closed
/// path that uses each edge of \f$ g \f$ exactly once; empty if g has no edge
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isEulerianCycle}( G , c ) \cr
/// 2 & \quad \textbf{if}\ m > 0\ \textbf{and}\ c_1 \ne c_{|c|}
///     \ \textbf{then return false} \cr
/// 3 & \quad \textbf{return}\ \mathrm{isEulerianTrail}( G , c )
/// \end{array}}
/// \f]
template< Graph G >
bool isEulerianCycle( const G & g , const std::vector< int > & c );

/// whether the nodes \f$ c \f$ are a path, open or closed, that uses each
/// edge of \f$ g \f$ exactly once, an unoriented edge in either direction;
/// empty if g has no edge
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isEulerianTrail}( G , c ) \cr
/// 2 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ c = () \cr
/// 3 & \quad \textbf{if}\ |c| \ne m + 1\ \textbf{then return false} \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ |c| - 1\ \textbf{do}
///     \cr
/// 5 & \qquad \textbf{if}\ ( c_i , c_{i+1} ) \notin E\ \textbf{or}
///     \ \text{it was used before }\ \textbf{then return false} \cr
/// 6 & \qquad \text{mark } ( c_i , c_{i+1} ) \text{ used} \cr
/// 7 & \quad \textbf{return true}
/// \end{array}}
/// \f]
template< Graph G >
bool isEulerianTrail( const G & g , const std::vector< int > & c );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/EulerianCycle.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EulerianCycle.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EulerianCycle.h ------------------------*/
/*--------------------------------------------------------------------------*/
