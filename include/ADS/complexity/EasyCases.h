/*--------------------------------------------------------------------------*/
/*---------------------------- File EasyCases.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial halves of the pairs of table 19.1 of the conclusion,
 * problems close to NP-hard ones but solvable in polynomial time, and the
 * special cases that the conclusion names, the interval graphs:
 *
 * - `twoSat()`, SATISFIABILITY with at most two literals per clause
 *   (2-SATISFIABILITY), by the graph of the implications
 *   \f$ \neg a \Rightarrow b \f$ and \f$ \neg b \Rightarrow a \f$ of each
 *   clause \f$ a \vee b \f$: the formula is satisfiable if and only if no
 *   variable is in the strongly connected component of its negation, and a
 *   variable is true if its component comes after that of its negation in
 *   topological order, \f$ O( n + m ) \f$;
 * - `twoColoring()`, the 2-COLORABILITY, by a visit in breadth of each
 *   component, \f$ O( n^2 ) \f$ on the adjacency matrix;
 * - `bipartiteMatching()`, the 2-DIMENSIONAL MATCHING, by a maximum flow
 *   from a source to the left nodes, along the edges, to a sink;
 * - `intervalGraph()`, the graph whose nodes are intervals
 *   \f$ [ a , b ) \f$ and whose edges join those that intersect, and on it
 *   `intervalClique()`, the largest number of intervals through a point, by a
 *   sweep in \f$ O( n \log n ) \f$; the independent set and the coloring of
 *   intervals are in `Intervals.h`. The other easy halves are
 *   `linearTiling()` in `Domino.h` (tested in chapter 18), `eulerianCycle()`
 *   in `EulerianCycle.h`, `fractionalKnapsack()` in `FractionalKnapsack.h`,
 *   `kruskal()` in `MinimumSpanningTree.h` and `shortestPaths()` in
 *   `ShortestPaths.h`.
 *
 * The contracts [see `Contract.h`]: the inputs are instances of the
 * polynomial problems (clauses of at most two literals, unoriented graphs,
 * intervals with \f$ a \le b \f$) and each solution returned is checked as a
 * postcondition. The operations are defined in `EasyCases.tpp`.
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
 *         Università di Pisa \n
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

/// the node of the literal \f$ \pm( v + 1 ) \f$ in the graph of the
/// implications, \f$ 2 v \f$ for \f$ v \f$ and \f$ 2 v + 1 \f$ for its
/// negation
/// \pre \f$ l \ne 0 \f$
inline int literalNode( int l );

/// an assignment of the \f$ n \f$ variables that satisfies \f$ F \f$, each
/// clause of one or two literals, or nothing
///
/// In pseudocode (the algorithm of Aspvall, Plass and Tarjan, beyond the
/// book), a clause of one literal \f$ a \f$ taken as \f$ a \vee a \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoSat}( F , n ) \cr
/// 2 & \quad G \gets \text{a graph of the } 2 n \text{ literals and no edge}
///     \cr
/// 3 & \quad \textbf{foreach}\ a \vee b \in F\ \textbf{do}\ \text{insert } (
///     \neg a , b ) \text{ and } ( \neg b , a ) \text{ in } G \cr
/// 4 & \quad C \gets \mathrm{scc}( G );\ r \gets
///     \text{the rank of each component}\ \text{in the order of }
///     \mathrm{topsort}( G ) \cr
/// 5 & \quad \textbf{for}\ v \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ C[ v ] = C[ \neg v ]\ \textbf{then return nil} \cr
/// 7 & \qquad x[ v ] \gets r[ C[ v ] ] > r[ C[ \neg v ] ] \cr
/// 8 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, each clause has one or two literals of
///      \f$ [ 0 , n ) \f$
/// \post an assignment returned passes `satCertificate()`
inline std::optional< std::vector< bool > > twoSat( const Cnf & F , int n );

/// the side, 0 or 1, of each node of \f$ G \f$, the ends of each edge apart,
/// or nothing if \f$ G \f$ has an odd cycle; the diagonal of \f$ G \f$ is
/// ignored, as by `coloring()` [see `Problems.h`]
///
/// In pseudocode (the conclusion), a visit in breadth from each node not
/// reached, which gives each node the side opposite to that of its parent:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoColoring}( G ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ c[ v ] \gets
///     \textbf{nil} \cr
/// 3 & \quad \textbf{foreach}\ r \in V \text{ with } c[ r ] = \textbf{nil}
///     \ \textbf{do} \cr
/// 4 & \qquad c[ r ] \gets 0;\ Q \gets ( r ) \cr
/// 5 & \qquad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad\quad u \gets \mathrm{dequeue}( Q ) \cr
/// 7 & \qquad\quad \textbf{foreach}\ v \ne u \text{ with } G[ u ][ v ]
///     \ \textbf{do} \cr
/// 8 & \qquad\qquad \textbf{if}\ c[ v ] = c[ u ]\ \textbf{then return nil} \cr
/// 9 & \qquad\qquad \textbf{if}\ c[ v ] = \textbf{nil}\ \textbf{then}\ c[ v ]
///     \gets 1 - c[ u ];\ \mathrm{enqueue}( Q , v ) \cr
/// 10 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a coloring returned is proper with two colors
inline std::optional< std::vector< int > > twoColoring( const Adjacency & G );

/// the pairs \f$ ( i , j ) \f$ of a largest matching of the bipartite graph
/// with \f$ p \f$ left and \f$ q \f$ right nodes and the edges
/// \f$ ( i , j ) \f$ of \f$ E \f$, by a maximum flow
///
/// In pseudocode (the conclusion), the reduction of section 15.2:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bipartiteMatching}( p , q , E ) \cr
/// 2 & \quad N \gets \text{a network of the nodes } 0 , \ldots , p + q + 1;
///     \ s \gets p + q;\ t \gets p + q + 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ p - 1\ \textbf{do}
///     \ \text{add } ( s , i ) \text{ of capacity } 1 \cr
/// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ q - 1\ \textbf{do}
///     \ \text{add } ( p + j , t ) \text{ of capacity } 1 \cr
/// 5 & \quad \textbf{foreach}\ ( i , j ) \in E\ \textbf{do}\ \text{add } ( i ,
///     p + j ) \text{ of capacity } 1 \cr
/// 6 & \quad N.\mathrm{maxFlow}( s , t ) \cr
/// 7 & \quad \textbf{return}\ \text{the } ( i , j ) \in E
///     \text{ whose edge carries flow}
/// \end{array}}
/// \f]
/// \pre \f$ p , q \ge 0 \f$ and the edges join \f$ [ 0 , p ) \f$ to
///      \f$ [ 0 , q ) \f$
/// \post no two pairs returned share an end
inline std::vector< std::pair< int , int > > bipartiteMatching(
                        int p , int q ,
                        const std::vector< std::pair< int , int > > & E );

/// the graph of the intervals \f$ [ a , b ) \f$, an edge if two share a
/// point: an empty interval \f$ [ a , a ) \f$ meets none, as in
/// `intervalClique()`
///
/// In pseudocode (the conclusion), \f$ O( n^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intervalGraph}( I ) \cr
/// 2 & \quad \textbf{foreach}\ u < v\ \textbf{do} \cr
/// 3 & \qquad G[ u ][ v ] \gets G[ v ][ u ] \gets \max\{ a_u , a_v \} <
///     \min\{ b_u , b_v \} \cr
/// 4 & \quad \textbf{return}\ G
/// \end{array}}
/// \f]
/// \pre each interval \f$ [ a , b ) \f$ has \f$ a \le b \f$
/// \post an unoriented graph, one node per interval
inline Adjacency intervalGraph(
                        const std::vector< std::pair< long , long > > & I );

/// the largest clique of the graph of the intervals \f$ [ a , b ) \f$: the
/// most intervals through one point, the right ends before the left ones
///
/// In pseudocode (the conclusion), a sweep of the ends: a set of intervals
/// that meet pairwise has a common point (the property of Helly on the line),
/// and so a clique is a set of intervals through one point:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intervalClique}( I ) \cr
/// 2 & \quad P \gets \text{the pairs } ( a , +1 ) \text{ and } ( b , -1 )
///     \text{ of each } [ a , b ) \in I \cr
/// 3 & \quad \text{sort } P \text{, at equal abscissa } -1 \text{ first} \cr
/// 4 & \quad c \gets 0;\ \mathit{best} \gets 0 \cr
/// 5 & \quad \textbf{foreach}\ ( x , \delta ) \in P\ \textbf{do}\ c \gets c +
///     \delta;\ \mathit{best} \gets \max\{ \mathit{best} , c \} \cr
/// 6 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \pre each interval \f$ [ a , b ) \f$ has \f$ a \le b \f$
/// \post a number in \f$ [ 0 , n ] \f$
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
