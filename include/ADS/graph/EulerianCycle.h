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
 * nodes along the way, in O( n + m ) time.
 *
 * The contracts [see Contract.h]: the graph is oriented; at the level of
 * the audits, the cycle found is closed and uses each edge exactly once,
 * which isEulerianCycle() checks. The functions are defined in
 * EulerianCycle.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 15.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see C. Hierholzer, C. Wiener, Ueber die Moeglichkeit, einen
 *      Linienzug ohne Wiederholung und ohne Unterbrechung zu umfahren,
 *      Mathematische Annalen 6(1), 30-32, 1873, doi:10.1007/BF01442866
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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
/// \pre the graph is oriented
/// \post the cycle is Eulerian (audit)
template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g );

/// whether the nodes c, the first repeated at the end, are a closed path
/// that uses each edge of g exactly once; empty if g has no edge
template< Graph G >
bool isEulerianCycle( const G & g , const std::vector< int > & c );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/EulerianCycle.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EulerianCycle.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EulerianCycle.h ------------------------*/
/*--------------------------------------------------------------------------*/
