/*--------------------------------------------------------------------------*/
/*-------------------------- File Approximation.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The approximation algorithms of section 19.2 and of the exercises 19.6
 * and 19.8. An absolute approximation gives a feasible \f$ x' \f$ with
 * \f$ | c( x' ) - c( x^* ) | / | c( x^* ) | \le e \f$, \f$ e \f$ a constant of
 * the algorithm; an \f$ \epsilon \f$-approximation takes \f$ \epsilon \f$ in
 * input, in a time polynomial in the size and in general exponential in
 * \f$ 1 / \epsilon \f$.
 *
 * - `firstFit()`, ff and, on the volumes in decreasing order, ffd for BIN
 *   PACKING: no two boxes are filled at most half, so that
 *   \f$ N < 2 N^* \f$;
 * - `deltaTsp()` for the salesman with the triangle inequality: the nodes
 *   of a minimum spanning tree in preorder, the tree walked twice with the
 *   cities already visited skipped, at most twice the optimum (the bound is
 *   reached by the two concentric polygons of figure 19.3);
 * - `tspGap()`: the distances of Theorem 19.2, 1 on the edges of \f$ G \f$
 *   and \f$ s n \f$ elsewhere, so that an approximation within \f$ s \f$
 *   would decide HAMILTONIAN CIRCUIT;
 * - `apSubsetSum()`, the \f$ \epsilon \f$-approximation of the optimization
 *   SUBSET SUM with \f$ \epsilon = 1 / ( h + 1 ) \f$: each set of at most
 *   \f$ h \f$ elements, completed greedily on the elements in decreasing
 *   order, \f$ O( h n^{h+1} ) \f$;
 * - `vertexCoverApprox()`, the two ends of each edge not yet covered, a
 *   matching whose ends are at most twice a minimum cover (exercise 19.6);
 * - `knapsackApprox()`, an \f$ \epsilon \f$-approximation of KNAPSACK in time
 *   polynomial in \f$ n \f$ and \f$ 1 / \epsilon \f$ (exercise 19.8): the
 *   profits divided by \f$ K = \epsilon p_{\max} / n \f$ and rounded down,
 *   and the least volume of each rounded profit by dynamic programming, at
 *   least \f$ ( 1 - \epsilon ) \f$ times the optimum in
 *   \f$ O( n^3 / \epsilon ) \f$.
 *
 * The contracts [see `Contract.h`]: the volumes, the profits and the sums are
 * not negative, the distances square and symmetric, and the triangle
 * inequality that the bound of `deltaTsp()` rests on is an audit,
 * \f$ O( n^3 ) \f$; each solution returned is checked as feasible. The
 * operations are defined in `Approximation.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.2 and
 *      exercises 19.6, 19.8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see D. S. Johnson, A. Demers, J. D. Ullman, M. R. Garey, R. L. Graham,
 *      Worst-case performance bounds for simple one-dimensional packing
 *      algorithms, SIAM Journal on Computing 3(4), 299-325, 1974,
 *      doi:10.1137/0203025
 * \see D. J. Rosenkrantz, R. E. Stearns, P. M. Lewis, An analysis of
 *      several heuristics for the traveling salesman problem, SIAM Journal
 *      on Computing 6(3), 563-581, 1977, doi:10.1137/0206041
 * \see O. H. Ibarra, C. E. Kim, Fast approximation algorithms for the
 *      knapsack and sum of subset problems, Journal of the ACM 22(4),
 *      463-468, 1975, doi:10.1145/321906.321909
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

#ifndef __ADS_Approximation
 #define __ADS_Approximation
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- BIN PACKING ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the box of each object, the first one with room for it (ff), the objects
/// in the order given or, if `decreasing`, in decreasing order of volume
/// (ffd); the boxes are \f$ 0 , 1 , \ldots \f$
///
/// In pseudocode (section 19.2.1), \f$ F \f$ the room left in each box open:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{firstFit}( A , k , \mathit{decreasing} ) \cr
/// 2 & \quad O \gets ( 0 , \ldots , n - 1 ) \text{, by decreasing } A
///     \text{ if }\ \mathit{decreasing} \cr
/// 3 & \quad F \gets \text{the empty sequence} \cr
/// 4 & \quad \textbf{foreach}\ i \in O\ \textbf{do} \cr
/// 5 & \qquad b \gets \text{the first box with } F[ b ] \ge A[ i ]
///     \text{; a}\ \text{new one with } F[ b ] = k \text{ if none} \cr
/// 6 & \qquad F[ b ] \gets F[ b ] - A[ i ];\ \mathit{box}[ i ] \gets b \cr
/// 7 & \quad \textbf{return}\ \mathit{box}
/// \end{array}}
/// \f]
/// \pre each volume is in \f$ [ 0 , k ] \f$
/// \post no box is filled beyond \f$ k \f$
inline std::vector< int > firstFit( const std::vector< long > & A , long k ,
                                    bool decreasing = false );

/*--------------------------------------------------------------------------*/
/*----------------------------- DELTA TSP ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cost \f$ \sum_i D[ S[ i ] ][ S[ ( i + 1 ) \bmod n ] ] \f$ of the
/// circuit \f$ S \f$ of the distances \f$ D \f$
/// \pre \f$ D \f$ is square and \f$ S \f$ has distinct cities
template< class T >
T tourCost( const std::vector< std::vector< T > > & D , const Nodes & S );

/// the circuit of the nodes of a minimum spanning tree of the complete
/// graph of the symmetric distances \f$ D \f$ in preorder from 0, the
/// children by increasing name: at most twice the optimum if \f$ D \f$ is
/// triangular
///
/// In pseudocode (section 19.2.1), \f$ T \f$ costs less than any circuit,
/// which is a spanning tree plus an edge, and the preorder skips cities of
/// the walk that goes twice along \f$ T \f$, which by the triangle inequality
/// costs no more:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{deltaTsp}( D ) \cr
/// 2 & \quad T \gets \mathrm{kruskal}( \text{the complete graph of the }
///     \text{weights } D ) \cr
/// 3 & \quad \textbf{return}\ \text{the nodes of } T \text{ in preorder from}
///     \ 0 \text{, the children by increasing name}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square and symmetric, and triangular (an audit,
///      \f$ O( n^3 ) \f$)
/// \post a permutation of the cities
template< class T >
Nodes deltaTsp( const std::vector< std::vector< T > > & D );

/// the distances of Theorem 19.2: 1 on the edges of \f$ G \f$, \f$ s n \f$
/// elsewhere; their optimum is \f$ n \f$ if \f$ G \f$ has a Hamiltonian
/// circuit and more than \f$ s n \f$ otherwise
///
/// In pseudocode (section 19.2.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tspGap}( G , s ) \cr
/// 2 & \quad \textbf{foreach}\ u \ne v\ \textbf{do}\ D[ u ][ v ] \gets ( 1
///     \text{ if } G[ u ][ v ] \text{, else } s n );\quad D[ u ][ u ] \gets
///     0 \cr
/// 3 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric, and
///      \f$ s \ge 1 \f$
/// \post a symmetric matrix of the size of \f$ G \f$
inline std::vector< std::vector< long > > tspGap( const Adjacency & G ,
                                                  long s );

/*--------------------------------------------------------------------------*/
/*------------------------ SUBSET SUM AND KNAPSACK -------------------------*/
/*--------------------------------------------------------------------------*/

/// the sum, at most \f$ k \f$, and the places of \f$ A \f$ of the best set
/// found from the sets of at most \f$ h \f$ elements,
/// \f$ \epsilon = 1 / ( h + 1 ) \f$; the sets of (1) in the order of the
/// text, by size and then lexicographically on \f$ A \f$ in decreasing order
///
/// In pseudocode (section 19.2.3), the elements renamed so that
/// \f$ A[ 1 ] \ge \cdots \ge A[ n ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{apSubsetSum}( A , k , h ) \cr
/// 2 & \quad \mathit{max} \gets 0;\ \mathit{maxSol} \gets \emptyset \cr
/// 3 & \quad \textbf{foreach}\ S \subseteq \{ 1 , \ldots , n \}
///     \text{ with } |S| \le h\ \textbf{do}\quad \triangleright\ (1) \cr
/// 4 & \qquad z \gets \sum_{j \in S} A[ j ] \cr
/// 5 & \qquad \textbf{if}\ z > k\ \textbf{then continue} \cr
/// 6 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do}\quad
///     \triangleright\ (2) \cr
/// 7 & \qquad\quad \textbf{if}\ j \notin S \text{ and } z + A[ j ] \le k
///     \ \textbf{then}\ S \gets S \cup \{ j \};\ z \gets z + A[ j ] \cr
/// 8 & \qquad \textbf{if}\ z \ge \mathit{max}\ \textbf{then}\ \mathit{max}
///     \gets z;\ \mathit{maxSol} \gets S \cr
/// 9 & \quad \textbf{return}\ ( \mathit{max} , \mathit{maxSol} )
/// \end{array}}
/// \f]
/// \pre \f$ k , h \ge 0 \f$ and \f$ A \f$ is not negative
/// \post a set of sum at most \f$ k \f$, the sum returned
inline std::pair< long , Nodes > apSubsetSum( const std::vector< long > & A ,
                                              long k , int h );

/// the best profit and the objects of an \f$ \epsilon \f$-approximation of
/// KNAPSACK, profits \f$ p \f$ and volumes \f$ v \f$ within \f$ C \f$, by
/// the rounded profits
///
/// In pseudocode (exercise 19.8), \f$ V[ i ][ s ] \f$ the least volume of the
/// first \f$ i \f$ objects that fit of rounded profit \f$ s \f$; the
/// rounding loses less than \f$ K \f$ per object, i.e., less than
/// \f$ \epsilon p_{\max} \le \epsilon\, \mathit{OPT} \f$ in all:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{knapsackApprox}( p , v , C , \epsilon ) \cr
/// 2 & \quad I \gets \text{the objects with } v_i \le C;\ \textbf{if}\ I =
///     \emptyset\ \textbf{then return}\ ( 0 , \emptyset ) \cr
/// 3 & \quad K \gets \max\{ 1 , \epsilon p_{\max} / |I| \};\ q_i \gets
///     \lfloor p_i / K \rfloor \text{ for } i \in I \cr
/// 4 & \quad V[ 0 ][ 0 ] \gets 0;\ V[ 0 ][ s ] \gets \infty \text{ for } s > 0
///     \cr
/// 5 & \quad \textbf{foreach}\ i \in I \text{, in order, and } s \in [ 0 ,
///     \sum q ]\ \textbf{do} \cr
/// 6 & \qquad V[ i + 1 ][ s ] \gets \min\{ V[ i ][ s ] , V[ i ][ s - q_i ] +
///     v_i \}\quad \triangleright\ \text{the second if } s \ge q_i \cr
/// 7 & \quad s \gets \text{the largest } s \text{ with } V[ |I| ][ s ] \le C
///     \cr
/// 8 & \quad \textbf{return}\ \text{the objects of } V[ |I| ][ s ]
///     \text{, read}\ \text{backwards, and their profit}
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ v \f$ of the same size and not negative,
///      \f$ 0 < \epsilon \le 1 \f$, \f$ C \ge 0 \f$
/// \post objects within \f$ C \f$, their profit returned
inline std::pair< long , Nodes > knapsackApprox(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C , double eps );

/*--------------------------------------------------------------------------*/
/*---------------------------- VERTEX COVER --------------------------------*/
/*--------------------------------------------------------------------------*/

/// both ends of each edge \f$ [ u , v ] \f$, \f$ u < v \f$, whose ends are
/// both out of \f$ S \f$, at most twice a minimum vertex cover;
/// \f$ O( n^2 ) \f$ on the adjacency matrix
///
/// In pseudocode (exercise 19.6), the edges taken form a matching, and a
/// cover has an end of each of them:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{vertexCoverApprox}( G ) \cr
/// 2 & \quad S \gets \emptyset \cr
/// 3 & \quad \textbf{foreach}\ [ u , v ] \in E \text{ with } u < v
///     \text{, in lexicographic order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ u \notin S \text{ and } v \notin S\ \textbf{then}
///     \ S \gets S \cup \{ u , v \} \cr
/// 5 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a cover of the edges
inline Nodes vertexCoverApprox( const Adjacency & G );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/Approximation.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Approximation.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Approximation.h ------------------------*/
/*--------------------------------------------------------------------------*/
