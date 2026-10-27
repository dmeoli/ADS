/*--------------------------------------------------------------------------*/
/*------------------------- File BranchAndBound.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The branch-&-bound algorithms of section 19.3 and of the exercises 19.9
 * to 19.12: the enumeration of chapter 16 that skips the subtree of a
 * sequence of choices \f$ S \f$ whose lower bound \f$ \mathit{lb}( S ) \f$ is
 * not below the cost \f$ \mathit{minCost} \f$ of the best solution found so
 * far. The worst case stays superpolynomial; the time in practice depends
 * on the starting solution, on the rule of branch, on the bound and on the
 * order of visit.
 *
 * `bbTsp()` chooses the cities of the circuit one after the other from
 * \f$ S[ 0 ] = 0 \f$, in increasing order, depth first. The bound of the
 * text is
 * \f[
 *   \mathit{lb}( S ) = C_i + \left\lceil \Big( A + B + \sum_{h \notin S} D_h
 *   \Big) / 2 \right\rceil ,
 * \f]
 * \f$ C_i \f$ the length of the path \f$ S[ 0 .. i ] \f$, \f$ A \f$ and
 * \f$ B \f$ the least distances to \f$ S[ 0 ] \f$ and from \f$ S[ i ] \f$ of
 * a city out of \f$ S \f$ (the text has from \f$ S[ 0 ] \f$, the same for
 * symmetric distances), \f$ D_h \f$ the least distance into and out of
 * \f$ h \f$ over all the cities, as in the example of the text
 * (\f$ \mathit{lb} = 18 \f$ for \f$ S = 1 , 3 \f$ of figure 19.5). Two
 * values of figure 19.6 do not follow from this formula, 21 for 1 2 3 and
 * for 1 5 (it gives 19 and 20), and no rule on \f$ p \f$ and \f$ q \f$ found
 * gives all the values of the figure; with the formula the tree has 25 nodes
 * instead of 23 and ends with the same circuit of cost 19. The bound of the
 * 1-tree (exercise 19.10) replaces the sum by the weight of a minimum
 * spanning tree of the cities out of \f$ S \f$, for the symmetric distances.
 * `littleTsp()` (exercise 19.11, the algorithm of Little, Murty, Sweeney and
 * Karel) branches on including or excluding an edge, the one of reduced cost
 * 0 whose exclusion raises the bound the most, visits by least bound, and
 * bounds by reducing the rows and the columns of the matrix of the distances
 * to a least entry 0. `maxWeightClique()` (exercise 19.9) bounds by the
 * weight of the candidates, `knapsackBB()` (exercise 19.12) by the
 * fractional knapsack of chapter 14.
 *
 * The contracts [see `Contract.h`]: the distances are square, symmetric for
 * the bound of the 1-tree, the weights and the profits not negative and the
 * volumes positive; each optimum returned is checked as feasible, with its
 * cost. The operations are defined in `BranchAndBound.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.3 and
 *      exercises 19.9-19.12,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see J. D. C. Little, K. G. Murty, D. W. Sweeney, C. Karel, An algorithm
 *      for the traveling salesman problem, Operations Research 11(6),
 *      972-989, 1963, doi:10.1287/opre.11.6.972
 * \see M. Held, R. M. Karp, The traveling-salesman problem and minimum
 *      spanning trees: part II, Mathematical Programming 1, 6-25, 1971,
 *      doi:10.1007/BF01584070
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

#ifndef __ADS_BranchAndBound
 #define __ADS_BranchAndBound
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"

#include <algorithm>
#include <concepts>
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
/*------------------------------ BOUNDS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the lower bound of a circuit that starts with \f$ S \f$
enum class TspBound {
 BOOK ,       ///< the bound of the text, by the least edges of each city
 ONE_TREE     ///< the bound of the 1-tree (exercise 19.10)
 };

/// the weight of a minimum spanning tree of the nodes in \f$ V \f$, by the
/// symmetric distances \f$ D \f$
///
/// In pseudocode (exercise 19.10):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{spanningTreeWeight}( D , V ) \cr
/// 2 & \quad T \gets \mathrm{kruskal}( \text{the complete graph on } V
///     \text{ of the weights } D ) \cr
/// 3 & \quad \textbf{return}\ \sum_{e \in T} w( e )
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square and symmetric, and \f$ V \f$ has distinct cities
template< class T >
T spanningTreeWeight( const std::vector< std::vector< T > > & D ,
                      const Nodes & V );

/// the weight of a minimum 1-tree of \f$ r \f$: a minimum spanning tree of
/// the other nodes and the two least edges of \f$ r \f$; each circuit is a
/// 1-tree, so that the weight bounds the optimum from below
///
/// In pseudocode (exercise 19.10, after Held and Karp):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{oneTree}( D , r ) \cr
/// 2 & \quad w \gets \mathrm{spanningTreeWeight}( D , V \setminus \{ r \} )
///     \cr
/// 3 & \quad \textbf{return}\ w + \text{the two least of } D[ r ][ h ]
///     \text{, } h \ne r
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square and symmetric, and \f$ r \f$ is a city
template< class T >
T oneTree( const std::vector< std::vector< T > > & D , int r );

/// a lower bound \f$ \mathit{lb}( S ) \f$ of the circuits that start with the
/// path \f$ S \f$, the cost of the circuit if \f$ S \f$ has all the cities or
/// if \f$ n = 2 \f$, where the circuit through both cities is the only one
///
/// In pseudocode (section 19.3.1), \f$ R \f$ the cities out of \f$ S \f$ and
/// \f$ i = |S| - 1 \f$; each city of a circuit is entered once and left once,
/// so that half the least cost of entering and leaving each city of
/// \f$ R \f$ bounds what is left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tspLowerBound}( D , S , \mathit{how} ) \cr
/// 2 & \quad C \gets \sum_{k < i} D[ S[ k ] ][ S[ k + 1 ] ] \cr
/// 3 & \quad \textbf{if}\ R = \emptyset\ \textbf{then return}\ C + D[ S[ i ]
///     ][ S[ 0 ] ] \cr
/// 4 & \quad A \gets \min_{h \in R} D[ h ][ S[ 0 ] ];\ B \gets \min_{h \in R}
///     D[ S[ i ] ][ h ] \cr
/// 5 & \quad \textbf{if}\ \mathit{how} = \text{ONE_TREE}
///     \ \textbf{then return}\ C + A + B + \mathrm{spanningTreeWeight}( D , R
///     ) \cr
/// 6 & \quad \textbf{foreach}\ h \in R\ \textbf{do}\ D_h \gets \min_{p \ne q ,
///     \, p , q \ne h} ( D[ p ][ h ] + D[ h ][ q ] ) \cr
/// 7 & \quad \textbf{return}\ C + \lceil ( A + B + \sum_{h \in R} D_h ) / 2
///     \rceil
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square, \f$ S \f$ has distinct cities and is not empty,
///      \f$ D \f$ symmetric for the 1-tree
template< class T >
T tspLowerBound( const std::vector< std::vector< T > > & D , const Nodes & S ,
                 TspBound how = TspBound::BOOK );

/*--------------------------------------------------------------------------*/
/*------------------------------- BBTSP ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cost and a minimum circuit from 0, starting from the circuit
/// `start` if not empty; the number of nodes of the tree of the choices
/// generated, the root included, goes to `*generated`, if given
///
/// In pseudocode (section 19.3.1), depth first, a child generated for each
/// city not in \f$ S \f$ and visited only if its bound is below the best:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bb}( S )\quad \triangleright
///     \ \text{started with } S = ( 0 ) \cr
/// 2 & \quad \textbf{foreach}\ c \notin S \text{, in increasing order}
///     \ \textbf{do} \cr
/// 3 & \qquad \mathit{lb} \gets \mathrm{tspLowerBound}( D , S \cdot ( c ) ,
///     \mathit{how} ) \cr
/// 4 & \qquad \textbf{if}\ \mathit{lb} < \mathit{minCost}\ \textbf{then} \cr
/// 5 & \qquad\quad \textbf{if}\ |S| + 1 < n\ \textbf{then}\ \mathrm{bb}( S
///     \cdot ( c ) ) \cr
/// 6 & \qquad\quad \textbf{else}\ \mathit{minSol} \gets S \cdot ( c );
///     \ \mathit{minCost} \gets \mathit{lb}\quad \triangleright
///     \ \text{the cost} \cr
/// 7 & \textbf{function}\ \mathrm{bbTsp}( D , \mathit{start} ) \cr
/// 8 & \quad \mathit{minSol} \gets \mathit{start};\ \mathit{minCost} \gets
///     \text{its cost, } \infty \text{ if empty} \cr
/// 9 & \quad \mathrm{bb}( ( 0 ) ) \cr
/// 10 & \quad \textbf{return}\ ( \mathit{minCost} , \mathit{minSol} )
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square, `start` empty or a permutation, \f$ D \f$
///      symmetric for the 1-tree
/// \post a permutation and its cost, if \f$ n \ge 2 \f$
template< class T >
std::pair< T , Nodes > bbTsp( const std::vector< std::vector< T > > & D ,
                              const Nodes & start = {} ,
                              TspBound how = TspBound::BOOK ,
                              long * generated = nullptr );

/*--------------------------------------------------------------------------*/
/*------------------------------ LITTLE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cost and a minimum circuit from 0, by the reduction of the matrix;
/// the number of nodes of the tree generated goes to `*generated`, if given
///
/// In pseudocode (exercise 19.11, the algorithm of Little, Murty, Sweeney and
/// Karel), a node \f$ x \f$ with its reduced matrix \f$ M_x \f$, its edges
/// included and its bound \f$ \mathit{lb}_x \f$, the sum of the minima
/// subtracted, which no circuit of the subtree can avoid:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{littleTsp}( D ) \cr
/// 2 & \quad x \gets \text{the root, } M_x = D \text{ with } \infty
///     \text{ on the diagonal};\ \mathrm{reduce}( x ) \cr
/// 3 & \quad Q \gets \text{a priority queue};\ \mathrm{insert}( Q , x ,
///     \mathit{lb}_x ) \cr
/// 4 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad x \gets \mathrm{deleteMin}( Q ) \cr
/// 6 & \qquad \textbf{if}\ x \text{ has } n \text{ edges}
///     \ \textbf{then return}\ \text{its circuit and its cost} \cr
/// 7 & \qquad ( i , j ) \gets \text{the entry } 0 \text{ of } M_x
///     \text{ with the largest}\ \min_{h \ne j} M_x[ i ][ h ] + \min_{h \ne
///     i} M_x[ h ][ j ] \cr
/// 8 & \qquad y \gets x \text{ with } ( i , j ) \text{, its row and column}
///     \ \text{done, the edge that would close a subcircuit set to } \infty
///     \cr
/// 9 & \qquad z \gets x \text{ with } M_z[ i ][ j ] = \infty \cr
/// 10 & \qquad \textbf{foreach}\ c \in \{ y , z \}\ \textbf{do} \cr
/// 11 & \qquad\quad \textbf{if}\ c \text{ has } n \text{ edges or }
///     \mathrm{reduce}( c )\ \textbf{then}\ \mathrm{insert}( Q , c ,
///     \mathit{lb}_c ) \cr
/// 12 & \textbf{function}\ \mathrm{reduce}( x )\quad \triangleright
///     \ \text{false if a row or column has no finite entry} \cr
/// 13 & \quad \textbf{foreach}\ \text{row, then column, not done}
///     \ \textbf{do}\ \text{subtract its minimum } \mu \text{, and add } \mu
///     \text{ to } \mathit{lb}_x
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square
/// \post a permutation and its cost, if \f$ n \ge 2 \f$
template< class T >
std::pair< T , Nodes > littleTsp( const std::vector< std::vector< T > > & D ,
                                  long * generated = nullptr );

/*--------------------------------------------------------------------------*/
/*----------------------- CLIQUE AND KNAPSACK ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the weight and the nodes of a clique of \f$ G \f$ of largest weight
/// (MAXIMUM WEIGHT CLIQUE), the weights \f$ w \f$ not negative; a set is
/// extended by the candidates adjacent to all of it, and pruned when its
/// weight and that of all the candidates is not above the best; the nodes
/// generated go to `*generated`, if given
///
/// In pseudocode (exercise 19.9), \f$ K \f$ the candidates, started with
/// \f$ S = \emptyset \f$ and \f$ K = V \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{extend}( S , K , \mathit{weight} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{weight} > \mathit{best}\ \textbf{then}
///     \ \mathit{best} \gets \mathit{weight};\ \mathit{bestSet} \gets S \cr
/// 3 & \quad \mathit{rest} \gets \sum_{v \in K} w( v ) \cr
/// 4 & \quad \textbf{for}\ a \gets 1\ \textbf{to}\ |K|\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathit{weight} + \mathit{rest} \le \mathit{best}
///     \ \textbf{then return}\quad \triangleright\ \text{the bound} \cr
/// 6 & \qquad v \gets K[ a ];\ \mathit{rest} \gets \mathit{rest} - w( v ) \cr
/// 7 & \qquad \mathrm{extend}( S \cup \{ v \} , \{ K[ b ] : b > a , G[ v ][
///     K[ b ] ] \} , \mathit{weight} + w( v ) )
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric, \f$ w \f$ of
///      size \f$ n \f$ and not negative
/// \post a clique and its weight
inline std::pair< long , Nodes > maxWeightClique(
                                            const Adjacency & G ,
                                            const std::vector< long > & w ,
                                            long * generated = nullptr );

/// the best profit and the objects of KNAPSACK by branch-&-bound, by places;
/// the objects by decreasing profit per volume, each in and then out, pruned
/// when the fractional knapsack of the rest does not beat the best; the
/// nodes generated go to `*generated`, if given
///
/// In pseudocode (exercise 19.12), the objects renamed by decreasing
/// \f$ p_i / v_i \f$, \f$ \mathit{frac}( i , c ) \f$ the greedy of the
/// fractional knapsack of chapter 14 on the objects from \f$ i \f$ within
/// \f$ c \f$, an upper bound of their best profit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bb}( S , i , c , \mathit{profit} )\quad
///     \triangleright\ \text{started with } S = \emptyset ,\ 0 ,\ C ,\ 0 \cr
/// 2 & \quad \textbf{if}\ \mathit{profit} > \mathit{best}\ \textbf{then}
///     \ \mathit{best} \gets \mathit{profit};\ \mathit{bestSet} \gets S \cr
/// 3 & \quad \textbf{if}\ i = n \text{ or } \mathit{profit} +
///     \mathit{frac}( i , c ) \le \mathit{best}\ \textbf{then return} \cr
/// 4 & \quad \textbf{if}\ v_i \le c\ \textbf{then}\ \mathrm{bb}( S \cup \{ i
///     \} , i + 1 , c - v_i , \mathit{profit} + p_i ) \cr
/// 5 & \quad \mathrm{bb}( S , i + 1 , c , \mathit{profit} )
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ v \f$ of the same size, \f$ p \f$ not negative,
///      \f$ v \f$ positive, \f$ C \ge 0 \f$
/// \post objects within \f$ C \f$ and their profit
inline std::pair< long , Nodes > knapsackBB( const std::vector< long > & p ,
                                             const std::vector< long > & v ,
                                             long C ,
                                             long * generated = nullptr );

/// KNAPSACK by backtracking with the simple bound: the objects in their
/// order, each in and then out, an object that does not fit skipped, and a
/// node pruned when its profit plus the profits of all the objects still
/// to decide does not beat the best; the nodes generated go to
/// `*generated`
///
/// In pseudocode (exercise 19.12), the procedure of `knapsackBB()` with the
/// objects in their order and \f$ \sum_{j \ge i} p_j \f$ in place of
/// \f$ \mathit{frac}( i , c ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bt}( S , i , c , \mathit{profit} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{profit} > \mathit{best}\ \textbf{then}
///     \ \mathit{best} \gets \mathit{profit};\ \mathit{bestSet} \gets S \cr
/// 3 & \quad \textbf{if}\ i = n \text{ or } \mathit{profit} + \sum_{j \ge i}
///     p_j \le \mathit{best}\ \textbf{then return} \cr
/// 4 & \quad \textbf{if}\ v_i \le c\ \textbf{then}\ \mathrm{bt}( S \cup \{ i
///     \} , i + 1 , c - v_i , \mathit{profit} + p_i ) \cr
/// 5 & \quad \mathrm{bt}( S , i + 1 , c , \mathit{profit} )
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ v \f$ of the same size, \f$ p \f$ not negative,
///      \f$ v \f$ positive, \f$ C \ge 0 \f$
/// \post objects within \f$ C \f$ and their profit
inline std::pair< long , Nodes > knapsackBacktrack(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C ,
                                  long * generated = nullptr );

/// the greedy heuristic of KNAPSACK: the objects by decreasing profit per
/// volume, each taken if it still fits; not optimal in general
///
/// In pseudocode (exercise 19.12):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{knapsackGreedy}( p , v , C ) \cr
/// 2 & \quad S \gets \emptyset \cr
/// 3 & \quad \textbf{foreach}\ i \text{ by decreasing } p_i / v_i
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ v_i \le C\ \textbf{then}\ C \gets C - v_i;\ S
///     \gets S \cup \{ i \} \cr
/// 5 & \quad \textbf{return}\ ( \sum_{i \in S} p_i , S )
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ v \f$ of the same size, \f$ p \f$ not negative,
///      \f$ v \f$ positive, \f$ C \ge 0 \f$
/// \post objects within \f$ C \f$ and their profit
inline std::pair< long , Nodes > knapsackGreedy(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/BranchAndBound.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BranchAndBound.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BranchAndBound.h ------------------------*/
/*--------------------------------------------------------------------------*/
