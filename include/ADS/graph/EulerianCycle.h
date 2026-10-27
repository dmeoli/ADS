/*--------------------------------------------------------------------------*/
/*-------------------------- File EulerianCycle.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Eulerian cycle of an oriented graph (exercise 15.3), a closed path
 * that uses each edge exactly once: by the theorem of Euler it exists if
 * and only if each node has as many edges entering as leaving and all the
 * edges are in one component, the orientation ignored. The book finds it
 * by a local search on the partitions of the edges into closed paths, each
 * edge choosing its successor and a swap of two successors merging two
 * paths; here the algorithm of Hierholzer follows unused edges from a node
 * until it comes back, and splices in the closed paths found from the
 * nodes along the way, in \f$ O( n + m ) \f$ time.
 *
 * The contracts [see `Contract.h`]: the graph is oriented; at the level of
 * the audits, the cycle found is closed and uses each edge exactly once,
 * which `isEulerianCycle()` checks. The functions are defined in
 * `EulerianCycle.tpp`.
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
#include "ADS/mfset/MFSet.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the nodes of an Eulerian cycle of the oriented graph, the first node
/// repeated at the end, or nothing if there is none; empty with no edge
///
/// In pseudocode (Hierholzer's algorithm, beyond the book): the walk from a
/// node with unused edges always ends where it started, since every node has
/// as many edges entering as leaving, and the walks found along the way are
/// spliced in.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{eulerianCycle}( G ) \cr
/// 2 & \quad \textbf{if}\  \text{a node has } \mathit{indeg} \ne
///     \mathit{outdeg}\ \textbf{or}\  \text{the edges are not connected }
///     \ \textbf{then return}\ \textbf{nil} \cr
/// 3 & \quad S \gets ( \text{a node with an edge} );\ C \gets () \cr
/// 4 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad u \gets \mathrm{top}( S ) \cr
/// 6 & \qquad \textbf{if}\ u \text{ has an unused edge } ( u , v )
///     \ \textbf{then} \cr
/// 7 & \qquad\quad \text{mark } ( u , v ) \text{ used};\ \mathrm{push}( S , v
///     ) \cr
/// 8 & \qquad \textbf{else} \cr
/// 9 & \qquad\quad \mathrm{pop}( S );\ C \gets ( u ) \cdot C \cr
/// 10 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre the graph is oriented
/// \post the cycle is Eulerian (audit)
template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g );

/// whether the nodes \f$ c \f$, the first repeated at the end, are a closed
/// path that uses each edge of \f$ g \f$ exactly once; empty if g has no edge
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isEulerianCycle}( G , c ) \cr
/// 2 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ c = () \cr
/// 3 & \quad \textbf{if}\ c_1 \ne c_{|c|}\ \textbf{then return false} \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ |c| - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ ( c_i , c_{i+1} ) \notin E\ \textbf{or}
///     \text{it was used before }\ \textbf{then return false} \cr
/// 6 & \qquad \text{mark } ( c_i , c_{i+1} ) \text{ used} \cr
/// 7 & \quad \textbf{return}\ |c| - 1 = m
/// \end{array}}
/// \f]
template< Graph G >
bool isEulerianCycle( const G & g , const std::vector< int > & c );

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
