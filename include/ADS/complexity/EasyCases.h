/*--------------------------------------------------------------------------*/
/*---------------------------- File EasyCases.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial halves of the pairs of Tabella 19.1 of the Conclusione,
 * problems close to NP-hard ones but solvable in polynomial time, and the
 * special cases that the Conclusione names, the interval graphs:
 *
 * - twoSat(), SODDISFATTIBILITA' with at most two literals per clause, by
 *   the graph of the implications \f$ \neg a \Rightarrow b \f$ and
 *   \f$ \neg b \Rightarrow a \f$ of each clause \f$ a \vee b \f$: the formula
 *   is satisfiable if and only if no variable is in the strongly connected
 *   component of its negation, and a variable is true if its component
 *   comes after that of its negation in topological order, O( n + m );
 * - twoColoring(), the 2-COLORAZIONE, by a breadth first visit of each
 *   component, O( n^2 ) on the adjacency matrix;
 * - bipartiteMatching(), the ABBINAMENTO 2-DIMENSIONALE, by a maximum flow
 *   from a source to the left nodes, along the edges, to a sink;
 * - intervalGraph(), the graph whose nodes are intervals [ a , b ) and
 *   whose edges join those that intersect, and on it intervalClique(),
 *   the largest number of intervals through a point, by a sweep in
 *   \f$ O( n \log n ) \f$; the independent set and the coloring of intervals
 *   are in Intervals.h. The other easy halves are linearTiling() in
 *   Domino.h (tested in chapter 18), eulerianCycle() in EulerianCycle.h,
 *   fractionalKnapsack() in FractionalKnapsack.h, kruskal() in
 *   MinimumSpanningTree.h and shortestPaths() in ShortestPaths.h.
 *
 * The contracts [see Contract.h]: the inputs are instances of the polynomial
 * problems (clauses of at most two literals, unoriented graphs, intervals with
 * a <= b) and each solution returned is checked as a postcondition. The
 * operations are defined in EasyCases.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Conclusione,
 *      https://algoritmicamente.it
 * \see B. Aspvall, M. F. Plass, R. E. Tarjan, A linear-time algorithm for
 *      testing the truth of certain quantified boolean formulas,
 *      Information Processing Letters 8(3), 121-123, 1979,
 *      doi:10.1016/0020-0190(79)90002-4
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

#ifndef __ADS_EasyCases
 #define __ADS_EasyCases
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Visit.h"
#include "ADS/queue/ArrayQueue.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the node of the literal +-( v + 1 ) in the graph of the implications
/// \pre l is not 0
inline int literalNode( int l );

/// an assignment of the n variables that satisfies F, each clause of one
/// or two literals, or nothing
/// \pre n >= 0, each clause has one or two literals of [ 0 , n )
/// \post an assignment returned passes satCertificate()
inline std::optional< std::vector< bool > > twoSat( const Cnf & F , int n );

/// the side, 0 or 1, of each node of G, the ends of each edge apart, or
/// nothing if G has an odd cycle; the diagonal of G is ignored, as by
/// coloring() [see Problems.h]
/// \pre G is unoriented, i.e., square and symmetric
/// \post a coloring returned is proper with two colors
inline std::optional< std::vector< int > > twoColoring( const Adjacency & G );

/// the pairs ( i , j ) of a largest matching of the bipartite graph with
/// p left and q right nodes and the edges E ( i , j ), by a maximum flow
/// \pre p, q >= 0 and the edges join [ 0 , p ) to [ 0 , q )
/// \post no two pairs returned share an end
inline std::vector< std::pair< int , int > > bipartiteMatching(
                        int p , int q ,
                        const std::vector< std::pair< int , int > > & E );

/// the graph of the intervals [ a , b ), an edge if two share a point: an
/// empty interval [ a , a ) meets none, as in intervalClique()
/// \pre each interval [ a , b ) has a <= b
/// \post an unoriented graph, one node per interval
inline Adjacency intervalGraph(
                        const std::vector< std::pair< long , long > > & I );

/// the largest clique of the graph of the intervals [ a , b ): the most
/// intervals through one point, the right ends before the left ones
/// \pre each interval [ a , b ) has a <= b
/// \post a number in [ 0 , n ]
inline int intervalClique( const std::vector< std::pair< long , long > > & I );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/EasyCases.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EasyCases.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File EasyCases.h --------------------------*/
/*--------------------------------------------------------------------------*/
