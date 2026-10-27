/*--------------------------------------------------------------------------*/
/*--------------------------- File Reductions.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial reductions of chapter 18: a function \f$ f \f$ from the
 * inputs of \f$ A \f$ to the inputs of \f$ B \f$, computable in polynomial
 * time, such that \f$ x \f$ is a YES of \f$ A \f$ if and only if
 * \f$ f( x ) \f$ is a YES of \f$ B \f$; then \f$ A \f$ is no harder than
 * \f$ B \f$ (\f$ A \propto B \f$).
 *
 * - BOUNDED TILING to CLIQUE (Theorem 18.2): a node \f$ u_{hij} \f$ for the
 *   tile \f$ h \f$ in the square \f$ ( i , j ) \f$, only \f$ u_{000} \f$ in
 *   \f$ ( 0 , 0 ) \f$, joined to the nodes of the squares not adjacent and
 *   to those of the adjacent squares whose tile fits, and \f$ k = n^2 \f$;
 * - BOUNDED TILING to SATISFIABILITY (Theorem 18.3): \f$ u_{000} \f$, a tile
 *   at least and one at most in each square, and no two tiles that do not
 *   fit side by side;
 * - BOUNDED TILING to 0-1 INTEGER PROGRAMMING (Theorem 18.4):
 *   \f$ x_{000} = 1 \f$, one tile per square, and \f$ x_{hij} \f$ plus the
 *   tiles that do not fit next to it at most 1 (the example of the text
 *   writes one inequality per pair, which is the same since a square holds
 *   one tile);
 * - CLIQUE to INDEPENDENT SET (Theorem 18.5), the complement graph;
 *   INDEPENDENT SET to VERTEX COVER (exercise 18.11), the complement of a
 *   set is a cover, \f$ k' = n - k \f$;
 * - HAMILTONIAN CIRCUIT to TRAVELING SALESMAN (Theorem 18.6): the distance 1
 *   on the edges and 2 elsewhere, \f$ k = n \f$;
 * - PARTITION to MULTIPROCESSOR SCHEDULING, SUBSET SUM and KNAPSACK
 *   (Theorems 18.7 to 18.9), half of the sum as the bound;
 * - HAMILTONIAN CIRCUIT to DIRECTED HAMILTONIAN CIRCUIT (exercise 18.12),
 *   both the orientations of each edge; DIRECTED HAMILTONIAN CIRCUIT to
 *   DIRECTED HAMILTONIAN PATH (exercise 18.13), the node 0 split into a
 *   start with its edges out and an end with its edges in; DIRECTED
 *   HAMILTONIAN CIRCUIT to HAMILTONIAN PATH (exercise 18.14), that path with
 *   each node made a chain of three and a pendant node at each end;
 * - DIRECTED HAMILTONIAN PATH from \f$ r \f$ to \f$ t \f$ to LONGEST PATH,
 *   \f$ k = n - 1 \f$, and HAMILTONIAN PATH to DEGREE-CONSTRAINED SPANNING
 *   TREE, \f$ k = 2 \f$ (exercises 18.15 and 18.16);
 * - SORTING to CONVEX HULL: the points \f$ ( a_i , a_i^2 ) \f$ on the
 *   parabola are all vertices of the hull, in the order of the \f$ a_i \f$,
 *   so the hull costs \f$ \Omega( n \log n ) \f$.
 *
 * The contracts [see `Contract.h`]: each reduction checks that its input is
 * an instance of the problem reduced (a square or unoriented graph, a
 * nonempty set of tiles, numbers not negative) and that its output has the
 * shape of an instance of the target problem. The operations are defined in
 * `Reductions.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 18.5 and 18.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

#ifndef __ADS_Reductions
 #define __ADS_Reductions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Domino.h"
#include "ADS/complexity/Problems.h"
#include "ADS/geometry/ConvexHull.h"

#include <array>
#include <cstddef>
#include <limits>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place \f$ ( h n + i ) n + j \f$ of the variable or node of the tile
/// \f$ h \f$ in the square \f$ ( i , j ) \f$
/// \pre \f$ n \ge 1 \f$, \f$ h \ge 0 \f$, and \f$ ( i , j ) \f$ a square of
///      the \f$ n \times n \f$ board
inline int tileVar( int h , int i , int j , int n );

/// BOUNDED TILING to CLIQUE: the graph on the \f$ m n^2 \f$ places of
/// `tileVar()`, the ones of \f$ ( 0 , 0 ) \f$ but \f$ u_{000} \f$ isolated,
/// and \f$ k \f$
///
/// In pseudocode (Theorem 18.2), a clique of \f$ n^2 \f$ nodes takes one tile
/// per square, since two nodes of a square are never joined, and those tiles
/// fit wherever two squares touch:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dominoToClique}( D , n ) \cr
/// 2 & \quad G \gets \text{the nodes } u_{hij} \text{ and no edge} \cr
/// 3 & \quad \textbf{foreach}\ u_{hij} , u_{gab} \text{ of distinct squares,}
///     \ \text{neither of } ( 0 , 0 ) \text{ but } u_{000}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ ( i , j ) \text{ and } ( a , b ) \text{ are not}
///     \ \text{adjacent}\ \textbf{then}\ \text{join them} \cr
/// 5 & \qquad \textbf{else if}\ \text{the tiles } h \text{ and } g
///     \ \text{fit on their common side}\ \textbf{then}\ \text{join them} \cr
/// 6 & \quad \textbf{return}\ ( G , n^2 )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$ and \f$ D \f$ is not empty
/// \post an unoriented graph of \f$ m n^2 \f$ nodes, and \f$ k = n^2 \f$
inline std::pair< Adjacency , int > dominoToClique(
                                   const std::vector< Tile > & D , int n );

/// BOUNDED TILING to SATISFIABILITY, on the \f$ m n^2 \f$ variables of
/// `tileVar()`
///
/// In pseudocode (Theorem 18.3), \f$ u_{hij} \f$ true if the tile \f$ h \f$
/// is in \f$ ( i , j ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dominoToSat}( D , n ) \cr
/// 2 & \quad F \gets ( u_{000} ) \cr
/// 3 & \quad \textbf{foreach}\ ( i , j )\ \textbf{do}\ \text{add }
///     u_{0ij} \vee \cdots \vee u_{m-1,ij}\quad \triangleright
///     \ \text{a tile at least} \cr
/// 4 & \quad \textbf{foreach}\ ( i , j ) \text{ and } h < g\ \textbf{do}
///     \ \text{add } \neg u_{hij} \vee \neg u_{gij}\quad \triangleright
///     \ \text{one at most} \cr
/// 5 & \quad \textbf{foreach}\ h , g \text{ with } D[ h ].\mathit{right} \ne
///     D[ g ].\mathit{left}\ \textbf{do}\ \text{add } \neg u_{hij} \vee \neg
///     u_{g,i+1,j} \cr
/// 6 & \quad \textbf{foreach}\ h , g \text{ with } D[ h ].\mathit{up} \ne D[
///     g ].\mathit{down}\ \textbf{do}\ \text{add } \neg u_{hij} \vee \neg
///     u_{g,i,j+1} \cr
/// 7 & \quad \textbf{return}\ F
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$ and \f$ D \f$ is not empty
/// \post the literals are variables of \f$ [ 0 , m n^2 ) \f$
inline Cnf dominoToSat( const std::vector< Tile > & D , int n );

/// BOUNDED TILING to 0-1 INTEGER PROGRAMMING: \f$ A x \le b \f$ on the
/// \f$ m n^2 \f$ variables of `tileVar()`, each equation as two inequalities
///
/// In pseudocode (Theorem 18.4), \f$ x_{hij} = 1 \f$ if the tile \f$ h \f$ is
/// in \f$ ( i , j ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dominoToLp01}( D , n ) \cr
/// 2 & \quad \text{add } x_{000} = 1 \cr
/// 3 & \quad \textbf{foreach}\ ( i , j )\ \textbf{do}\ \text{add }
///     \sum_h x_{hij} = 1 \cr
/// 4 & \quad \textbf{foreach}\ h , ( i , j )\ \textbf{do} \cr
/// 5 & \qquad \text{add } x_{hij} + \sum x_{g,i+1,j} \le 1
///     \text{, over the tiles } g\ \text{that do not fit right of } h \cr
/// 6 & \qquad \text{add } x_{hij} + \sum x_{g,i,j+1} \le 1
///     \text{, over the tiles } g\ \text{that do not fit above } h \cr
/// 7 & \quad \textbf{return}\ ( A , b )\quad \triangleright
///     \ \text{an equation } a x = c \text{ as } a x \le c \text{ and }
///     -a x \le -c
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$ and \f$ D \f$ is not empty
/// \post \f$ A \f$ has \f$ m n^2 \f$ columns and as many rows as \f$ b \f$
inline std::pair< std::vector< std::vector< long > > , std::vector< long > >
dominoToLp01( const std::vector< Tile > & D , int n );

/// the complement graph: CLIQUE of size \f$ k \f$ to INDEPENDENT SET of size
/// \f$ k \f$
///
/// In pseudocode (Theorem 18.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{complement}( G ) \cr
/// 2 & \quad \textbf{foreach}\ u , v\ \textbf{do}\ C[ u ][ v ] \gets u \ne v
///     \ \textbf{and not}\ G[ u ][ v ] \cr
/// 3 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square
/// \post the edges between distinct nodes are those that \f$ G \f$ lacks
inline Adjacency complement( const Adjacency & G );

/// HAMILTONIAN CIRCUIT to TRAVELING SALESMAN: the distances, \f$ k = n \f$
///
/// In pseudocode (Theorem 18.6), a tour of length \f$ n \f$ uses only edges
/// of \f$ G \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamiltonToTsp}( G ) \cr
/// 2 & \quad \textbf{foreach}\ i \ne j\ \textbf{do}\ D[ i ][ j ] \gets ( 1
///     \text{ if } G[ i ][ j ] \text{, else } 2 );\quad D[ i ][ i ] \gets 0
///     \cr
/// 3 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a square matrix of the size of \f$ G \f$
inline std::vector< std::vector< long > > hamiltonToTsp( const Adjacency & G );

/// half the sum, or \f$ -1 \f$ if the sum is odd (then no partition: the
/// bound \f$ -1 \f$ has no YES in any of the three problems)
///
/// In pseudocode (Theorems 18.7 to 18.9), the bound of MULTIPROCESSOR
/// SCHEDULING on two processors, of SUBSET SUM, and of KNAPSACK with the
/// profits and the volumes equal to the numbers:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{halfSum}( A ) \cr
/// 2 & \quad s \gets \sum_i A[ i ] \cr
/// 3 & \quad \textbf{return}\ ( -1 \text{ if } s \text{ is odd, else } s / 2
///     )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post \f$ -1 \f$ or half the sum
inline long halfSum( const std::vector< long > & A );

/// DIRECTED HAMILTONIAN CIRCUIT to DIRECTED HAMILTONIAN PATH from \f$ n \f$
/// to 0: the node 0 split, a new node \f$ n \f$ with the edges out of 0; the
/// loops of \f$ G \f$ dropped, as the problems of `Problems.h` ignore them
///
/// In pseudocode (exercise 18.13), a cycle through 0 becomes a path from the
/// copy \f$ n \f$ of 0 to 0, and conversely:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cycleToPath}( G ) \cr
/// 2 & \quad H \gets \text{a graph of the nodes } 0 , \ldots , n
///     \text{ and no edge} \cr
/// 3 & \quad \textbf{foreach}\ ( u , v ) \in E \text{ with } u \ne v
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ u = 0\ \textbf{then}\ \text{insert } ( n , v )
///     \ \textbf{else}\ \text{insert } ( u , v ) \cr
/// 5 & \quad \textbf{return}\ H
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square and not empty (no node 0 to split otherwise)
/// \post a square graph of \f$ n + 1 \f$ nodes
inline Adjacency cycleToPath( const Adjacency & G );

/// DIRECTED HAMILTONIAN CIRCUIT to HAMILTONIAN PATH: the path of
/// `cycleToPath()`, each node \f$ v \f$ a chain
/// \f$ v_{in} - v_{mid} - v_{out} \f$, each edge \f$ ( u , v ) \f$ an edge
/// \f$ [ u_{out} , v_{in} ] \f$, and pendant nodes at the start and at the
/// end
///
/// In pseudocode (exercise 18.14), the chains keep the orientation, since a
/// Hamiltonian path must cross each chain from \f$ v_{in} \f$ to
/// \f$ v_{out} \f$, and the pendant nodes fix its two ends:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cycleToChain}( G ) \cr
/// 2 & \quad P \gets \mathrm{cycleToPath}( G );\ N \gets n + 1 \cr
/// 3 & \quad \textbf{foreach}\ v \in [ 0 , N )\ \textbf{do}\ \text{join }
///     v_{in} , v_{mid} \text{ and } v_{mid} , v_{out} \cr
/// 4 & \quad \textbf{foreach}\ ( u , v ) \text{ of } P\ \textbf{do}
///     \ \text{join } u_{out} , v_{in} \cr
/// 5 & \quad \text{join a new node to } n_{in} \text{ and another to }
///     0_{out} \cr
/// 6 & \quad \textbf{return}\ \text{the graph of the } 3 N + 2
///     \text{ nodes}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square and not empty
/// \post an unoriented graph of \f$ 3 ( n + 1 ) + 2 \f$ nodes
inline Adjacency cycleToChain( const Adjacency & G );

/// SORTING to CONVEX HULL: the points \f$ ( a , a^2 ) \f$
///
/// In pseudocode (section 18.5), the hull of the points lists them by
/// increasing \f$ a \f$, from the leftmost one, and so sorts the numbers:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sortToHull}( a ) \cr
/// 2 & \quad \textbf{return}\ ( ( a_i , a_i^2 ) : i = 1 , \ldots , n )
/// \end{array}}
/// \f]
/// \pre the squares of \f$ a \f$ fit in a `long`
/// \post one point per number
inline std::vector< Point > sortToHull( const std::vector< long > & a );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/Reductions.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Reductions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Reductions.h --------------------------*/
/*--------------------------------------------------------------------------*/
