/*--------------------------------------------------------------------------*/
/*---------------------------- File Problems.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems in NP of chapter 18, each with its polynomial certificate,
 * which checks a proposed solution, and an enumeration by backtrack, which
 * decides the problem in exponential time and returns a solution if there
 * is one. The graphs are adjacency matrices, symmetric when unoriented,
 * whose diagonal (a loop of a node) every problem ignores: a node with a
 * loop may be in an independent set, need not be in a cover and takes a
 * color like any other; the formulae in conjunctive normal form are
 * clauses of literals, \f$ +( v + 1 ) \f$ for the variable \f$ v \f$ and
 * \f$ -( v + 1 ) \f$ for its negation; a system of 0-1 integer programming
 * is \f$ A x \le b \f$.
 *
 * - `clique()` (CLIQUE, the enCricca() of the book), `independentSet()`,
 *   `vertexCover()`, `coloring()` (exercise 18.6), `bisection()`;
 * - `tsp()` (TRAVELING SALESMAN, the enTsp() of the book, pruned when the
 *   partial length exceeds \f$ k \f$), `hamiltonianCircuit()` (unoriented),
 *   `hamiltonianCycle()` and `hamiltonianPath()` (oriented, exercises 18.12
 *   and 18.13), `hamiltonianChain()` (unoriented, exercise 18.14),
 *   `longestPath()` and `longestPathDag()`, polynomial on an acyclic graph
 *   (exercise 18.15), `boundedSpanningTree()` (exercise 18.16);
 * - `satisfiable()` (SATISFIABILITY), `lp01()` (0-1 INTEGER PROGRAMMING);
 * - `partition()`, `subsetSumExists()`, `knapsackDecision()` (KNAPSACK,
 *   exercise 18.5), `multiprocessorScheduling()`, `exactCover3()` (exercise
 *   18.7), `threeDimensionalMatching()`, `bribes()` (exercises 18.1 and
 *   18.4).
 *
 * The enumerations take, in the worst case, \f$ O( 2^n ) \f$ or
 * \f$ O( n! ) \f$ times the polynomial cost of a node of the tree of the
 * choices; their pruning changes the time in practice, not the bound.
 *
 * The contracts [see `Contract.h`]: the graphs are square adjacency
 * matrices, unoriented (symmetric) where the problem is, the literals name
 * the variables of the formula, the numbers of the numerical problems are not
 * negative; each certificate returned by an enumeration is checked as a
 * postcondition. The operations are defined in `Problems.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
 * \see R. M. Karp, Reducibility among combinatorial problems, Complexity of
 *      Computer Computations, Plenum Press, 85-103, 1972,
 *      doi:10.1007/978-1-4684-2001-2_9
 * \see M. R. Garey, D. S. Johnson, Computers and Intractability: A Guide to
 *      the Theory of NP-Completeness, W. H. Freeman, 1979
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

#ifndef __ADS_Problems
 #define __ADS_Problems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Adjacency = std::vector< std::vector< bool > >;   ///< an adjacency
using Clause = std::vector< int >;                      ///< some literals
using Cnf = std::vector< Clause >;                      ///< a formula
using Nodes = std::vector< int >;                       ///< a set, a path

/*--------------------------------------------------------------------------*/
/*------------------------------ GRAPHS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether \f$ S \f$, of distinct nodes, has \f$ k \f$ of them or more, all
/// adjacent (the certificatoCricca() of the book)
///
/// In pseudocode (chapter 18), \f$ O( |S|^2 ) \f$ time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cliqueCertificate}( G , S , k ) \cr
/// 2 & \quad \textbf{if}\ |S| < k\ \textbf{then return false} \cr
/// 3 & \quad \textbf{foreach}\ a < b \text{ places of } S\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ S[ a ] = S[ b ]\ \textbf{or not}\ G[ S[ a ] ][ S[
///     b ] ]\ \textbf{then return false} \cr
/// 5 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square
inline bool cliqueCertificate( const Adjacency & G , const Nodes & S , int k );

namespace detail {

/// the sets of k nodes of [ 0 , n ), extended only while ok( S , v )
template< class Ok , class Done >
bool chooseK( int n , int k , Ok && ok , Done && done );

}  // end( namespace detail )

/// \f$ k \f$ nodes all adjacent, or nothing (the enCricca() of the book)
///
/// In pseudocode (chapter 18), the sets of \f$ k \f$ nodes in increasing
/// order, a set extended only by a node adjacent to all of it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{clique}( G , k ) \cr
/// 2 & \quad \textbf{if}\ k \le 0\ \textbf{then return}\ \emptyset \cr
/// 3 & \quad \textbf{return}\ \mathrm{choose}( \emptyset , 0 ) \cr
/// 4 & \textbf{function}\ \mathrm{choose}( S , i ) \cr
/// 5 & \quad \textbf{if}\ |S| = k\ \textbf{then return}\ S \cr
/// 6 & \quad \textbf{if}\ |S| + n - i < k\ \textbf{then return nil}\quad
///     \triangleright\ \text{too few nodes left} \cr
/// 7 & \quad \textbf{for}\ v \gets i\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ G[ u ][ v ] \text{ for each } u \in S
///     \ \textbf{then} \cr
/// 9 & \qquad\quad R \gets \mathrm{choose}( S \cup \{ v \} , v + 1 ) \cr
/// 10 & \qquad\quad \textbf{if}\ R \ne \textbf{nil}\ \textbf{then return}\ R
///     \cr
/// 11 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a set returned is a clique of \f$ k \f$ nodes
inline std::optional< Nodes > clique( const Adjacency & G , int k );

/// \f$ k \f$ nodes no two adjacent, or nothing
///
/// In pseudocode (chapter 18), the enumeration of `clique()` with the test of
/// its line 8 negated:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{independentSet}( G , k ) \cr
/// 2 & \quad \textbf{if}\ k \le 0\ \textbf{then return}\ \emptyset \cr
/// 3 & \quad \textbf{return}\ \mathrm{choose}( \emptyset , 0 )\quad
///     \triangleright\ \text{that of } \mathrm{clique}() \text{, } v
///     \text{ added}\ \text{if no } u \in S \text{ has } G[ u ][ v ]
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a set returned has \f$ k \f$ nodes, no two adjacent
inline std::optional< Nodes > independentSet( const Adjacency & G , int k );

/// at most \f$ k \f$ nodes that touch every edge, or nothing
///
/// In pseudocode (chapter 18), the sets by increasing size:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{vertexCover}( G , k ) \cr
/// 2 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ \min\{ k , n \}
///     \ \textbf{do} \cr
/// 3 & \qquad \textbf{foreach}\ S \subseteq V \text{ with } |S| = s
///     \text{, in}\ \text{lexicographic order}\ \textbf{do} \cr
/// 4 & \qquad\quad \textbf{if}\ \text{each edge } [ u , v ] \text{ has } u
///     \in S \text{ or } v \in S\ \textbf{then return}\ S \cr
/// 5 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a set returned has at most \f$ k \f$ nodes and covers the edges
inline std::optional< Nodes > vertexCover( const Adjacency & G , int k );

/// a coloring of the nodes with at most \f$ k \f$ colors, adjacent nodes of
/// different colors, by backtrack, or nothing (exercise 18.6)
///
/// In pseudocode (exercise 18.6), the colors numbered in the order of their
/// first use, so that two colorings that differ by a renaming of the colors
/// are not both tried:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{color}( u )\quad \triangleright
///     \ \text{started with } u = 0 \cr
/// 2 & \quad \textbf{if}\ u = n\ \textbf{then return true} \cr
/// 3 & \quad \mathit{used} \gets 1 + \max_{v < u} c[ v ]\quad
///     \triangleright\ 0 \text{ if } u = 0 \cr
/// 4 & \quad \textbf{for}\ x \gets 0\ \textbf{to}\ \min\{ k , \mathit{used} +
///     1 \} - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ c[ v ] \ne x \text{ for each } v < u \text{ with }
///     G[ u ][ v ]\ \textbf{then} \cr
/// 6 & \qquad\quad c[ u ] \gets x \cr
/// 7 & \qquad\quad \textbf{if}\ \mathrm{color}( u + 1 )
///     \ \textbf{then return true} \cr
/// 8 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric, and
///      \f$ k \ge 0 \f$
/// \post a coloring returned is proper, with colors in \f$ [ 0 , k ) \f$
inline std::optional< std::vector< int > > coloring( const Adjacency & G ,
                                                     int k );

/// a partition of the nodes in two halves cut by at most \f$ k \f$ edges, or
/// nothing; the half of node 0
///
/// In pseudocode (chapter 18), the halves that hold the node 0, so that each
/// partition is tried once:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bisection}( G , k ) \cr
/// 2 & \quad \textbf{if}\ n \text{ is odd}\ \textbf{then return nil} \cr
/// 3 & \quad \textbf{foreach}\ S \subseteq V \text{ with } 0 \in S
///     \text{ and } |S| = n / 2\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ |\{ [ u , v ] \in E : u \in S , v \notin S \}|
///     \le k\ \textbf{then return}\ S \cr
/// 5 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a half returned has \f$ n / 2 \f$ nodes and cuts at most \f$ k \f$
///       edges
inline std::optional< Nodes > bisection( const Adjacency & G , int k );

/// whether the tour \f$ P \f$, a permutation, is at most \f$ k \f$ long; the
/// empty tour of no city is 0 long (the certificatoTsp() of the book)
///
/// In pseudocode (chapter 18):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tspCertificate}( D , k , P ) \cr
/// 2 & \quad \textbf{if}\ P \text{ is not a permutation of the }
///     \text{cities}\ \textbf{then return false} \cr
/// 3 & \quad \textbf{return}\ D[ P[ n - 1 ] ][ P[ 0 ] ] + \sum_{i=1}^{n-1}
///     D[ P[ i - 1 ] ][ P[ i ] ] \le k
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square
template< class T >
bool tspCertificate( const std::vector< std::vector< T > > & D , T k ,
                     const Nodes & P );

/// a tour of length at most \f$ k \f$ from city 0, the partial tours longer
/// than \f$ k \f$ pruned, or nothing (the enTsp() of the book)
///
/// In pseudocode (chapter 18), the permutations that start with the city 0:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S , \ell )\quad \triangleright
///     \ \text{started with } S = ( 0 ) ,\ \ell = 0 \cr
/// 2 & \quad \textbf{if}\ \ell > k\ \textbf{then return false}\quad
///     \triangleright\ \text{pruned} \cr
/// 3 & \quad \textbf{if}\ |S| = n\ \textbf{then return}\ \ell + D[ S[ n - 1 ]
///     ][ 0 ] \le k \cr
/// 4 & \quad \textbf{foreach}\ c \notin S \text{, in increasing order}
///     \ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{extend}( S \cdot ( c ) , \ell + D[ S[ |S|
///     - 1 ] ][ c ] )\ \textbf{then return true} \cr
/// 6 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square, with no negative distance, which would make the
///      pruning wrong (a tour may come back below \f$ k \f$); adding
///      \f$ c \f$ to all the distances and \f$ n c \f$ to \f$ k \f$ keeps the
///      answer
/// \post a tour returned passes `tspCertificate()`
template< class T >
std::optional< Nodes > tsp( const std::vector< std::vector< T > > & D , T k );

namespace detail {

/// a path through all the nodes, from the first one in starts, with
/// adjacent consecutive nodes, closed if cycle, ending in end if >= 0
inline std::optional< Nodes > hamilton( const Adjacency & G ,
                                        const Nodes & starts , bool cycle ,
                                        int end = -1 );

inline Nodes all( int n );

}  // end( namespace detail )

/// an unoriented circuit through every node once (\f$ n \ge 3 \f$), or
/// nothing
///
/// In pseudocode (chapter 18), the backtracking shared by the four
/// Hamiltonian problems, here from \f$ P = ( 0 ) \f$, closed and with no end
/// given:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamilton}( P ) \cr
/// 2 & \quad \textbf{if}\ |P| = n\ \textbf{then return}\ ( \textbf{not}
///     \ \mathit{closed}\ \textbf{or}\ G[ P[ n - 1 ] ][ P[ 0 ] ] )
///     \ \textbf{and}\ ( \mathit{end} = \textbf{nil}\ \textbf{or}\ P[ n - 1 ]
///     =
///     \mathit{end} ) \cr
/// 3 & \quad \textbf{foreach}\ v \notin P \text{ with } G[ P[ |P| - 1 ] ][ v
///     ] \text{, in increasing order}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{hamilton}( P \cdot ( v ) )
///     \ \textbf{then return true} \cr
/// 5 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a circuit returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianCircuit( const Adjacency & G );

/// an oriented cycle through every node once (\f$ n \ge 2 \f$), or nothing
///
/// In pseudocode (exercise 18.12), the backtracking of `hamiltonianCircuit()`
/// on the oriented edges:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamiltonianCycle}( G ) \cr
/// 2 & \quad \textbf{if}\ n < 2\ \textbf{then return nil} \cr
/// 3 & \quad \textbf{return}\ \mathrm{hamilton}( ( 0 ) )\quad
///     \triangleright\ \text{closed, with no end given}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square
/// \post a cycle returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianCycle( const Adjacency & G );

/// an oriented path through every node once, from \f$ r \f$ and to \f$ t \f$
/// if given
///
/// In pseudocode (exercise 18.13), the backtracking of `hamiltonianCircuit()`
/// from each possible start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamiltonianPath}( G , r , t ) \cr
/// 2 & \quad \textbf{foreach}\ s \in \{ r \} \text{ if } r \ge 0
///     \text{, else } s \in V\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ \mathrm{hamilton}( ( s ) )
///     \text{, open and with end } t \text{, succeeds}
///     \ \textbf{then return}\ P \cr
/// 4 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square, \f$ r \f$ and \f$ t \f$ are nodes or \f$ -1 \f$
/// \post a path returned visits each node once along the edges, from
///       \f$ r \f$ and to \f$ t \f$ if given
inline std::optional< Nodes > hamiltonianPath( const Adjacency & G ,
                                               int r = -1 , int t = -1 );

/// an unoriented chain through every node once
///
/// In pseudocode (exercise 18.14):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamiltonianChain}( G ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{hamiltonianPath}( G , -1 , -1 )\quad
///     \triangleright\ \text{on the symmetric matrix}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a chain returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianChain( const Adjacency & G );

/// a simple path from \f$ r \f$ to \f$ t \f$ of at least \f$ k \f$ edges, by
/// backtrack
///
/// In pseudocode (exercise 18.15), a path that reaches \f$ t \f$ too short is
/// not extended, since it could not come back to \f$ t \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( P )\quad \triangleright
///     \ \text{started with } P = ( r ) \cr
/// 2 & \quad \textbf{if}\ P[ |P| - 1 ] = t\ \textbf{then return}\ |P| - 1
///     \ge k \cr
/// 3 & \quad \textbf{foreach}\ v \notin P \text{ with } G[ P[ |P| - 1 ] ][ v
///     ]\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{extend}( P \cdot ( v ) )
///     \ \textbf{then return true} \cr
/// 5 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square, \f$ r \f$ and \f$ t \f$ are nodes
/// \post a path returned is simple, from \f$ r \f$ to \f$ t \f$, of at least
///       \f$ k \f$ edges
inline std::optional< Nodes > longestPath( const Adjacency & G , int r ,
                                           int t , int k );

/// the most edges of a path from \f$ r \f$ to \f$ t \f$ in an acyclic
/// oriented graph, \f$ -1 \f$ if \f$ t \f$ is not reachable;
/// \f$ O( n^2 ) \f$ by the topological order
///
/// In pseudocode (exercise 18.15), one scan in topological order, as
/// `dagLongestPaths()` of `Paths.h`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPathDag}( G , r , t ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ L[ v ] \gets -1 \cr
/// 3 & \quad L[ r ] \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ u \text{ in topological order, with } L[ u ]
///     \ge 0\ \textbf{do} \cr
/// 5 & \qquad \textbf{foreach}\ v \text{ with } G[ u ][ v ]\ \textbf{do}
///     \ L[ v ] \gets \max\{ L[ v ] , L[ u ] + 1 \} \cr
/// 6 & \quad \textbf{return}\ L[ t ]
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square and acyclic, \f$ r \f$ and \f$ t \f$ are nodes
inline int longestPathDag( const Adjacency & G , int r , int t );

/// a spanning tree of the unoriented graph with at most \f$ k \f$ edges at
/// each node, as \f$ n - 1 \f$ edges, by enumeration, or nothing
///
/// In pseudocode (exercise 18.16), each edge \f$ E[ i ] \f$ taken and then
/// left, in order, an edge that closes a cycle or exceeds a degree never
/// taken:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( T , i )\quad \triangleright
///     \ \text{started with } T = \emptyset ,\ i = 1 \cr
/// 2 & \quad \textbf{if}\ |T| = n - 1\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ i > m \text{ or } |T| + m - i + 1 < n - 1
///     \ \textbf{then return false} \cr
/// 4 & \quad [ u , v ] \gets E[ i ] \cr
/// 5 & \quad \textbf{if}\ d_T( u ) < k \text{, } d_T( v ) < k \text{ and } u
///     , v \text{ are apart in } T\ \textbf{then} \cr
/// 6 & \qquad \textbf{if}\ \mathrm{extend}( T \cup \{ [ u , v ] \} , i + 1 )
///     \ \textbf{then return true} \cr
/// 7 & \quad \textbf{return}\ \mathrm{extend}( T , i + 1 )
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post the edges returned form a spanning tree with at most \f$ k \f$ at
///       each node
inline std::optional< std::vector< std::pair< int , int > > >
boundedSpanningTree( const Adjacency & G , int k );

/*--------------------------------------------------------------------------*/
/*---------------------- LOGIC AND LINEAR PROGRAMMING ----------------------*/
/*--------------------------------------------------------------------------*/

/// whether \f$ x \f$ satisfies every clause of \f$ F \f$
///
/// In pseudocode (chapter 18), in time linear in the size of \f$ F \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{satCertificate}( F , x ) \cr
/// 2 & \quad \textbf{foreach}\ c \in F\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ \text{no literal of } c \text{ is true under }
///     x\ \textbf{then return false} \cr
/// 4 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre the literals of \f$ F \f$ are variables of \f$ x \f$
inline bool satCertificate( const Cnf & F , const std::vector< bool > & x );

/// an assignment of the \f$ n \f$ variables that satisfies \f$ F \f$, by
/// backtrack with a clause checked as soon as its variables are set, or
/// nothing
///
/// In pseudocode (chapter 18), \f$ \mathit{last}( c ) \f$ the largest
/// variable of the clause \f$ c \f$, counted from 1:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{assign}( v )\quad \triangleright
///     \ \text{started with } v = 0 \cr
/// 2 & \quad \textbf{foreach}\ c \in F \text{ with } \mathit{last}( c ) = v
///     \ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ c \text{ is false under } x
///     \ \textbf{then return false} \cr
/// 4 & \quad \textbf{if}\ v = n\ \textbf{then return true} \cr
/// 5 & \quad \textbf{foreach}\ b \in ( \textbf{false} , \textbf{true} )
///     \ \textbf{do} \cr
/// 6 & \qquad x[ v ] \gets b \cr
/// 7 & \qquad \textbf{if}\ \mathrm{assign}( v + 1 )
///     \ \textbf{then return true} \cr
/// 8 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$ and the literals of \f$ F \f$ are variables of
///      \f$ [ 0 , n ) \f$
/// \post an assignment returned passes `satCertificate()`
inline std::optional< std::vector< bool > > satisfiable( const Cnf & F ,
                                                         int n );

/// a 0-1 vector \f$ x \f$ with \f$ A x \le b \f$, by enumeration, or nothing
///
/// In pseudocode (chapter 18), the \f$ 2^n \f$ vectors in lexicographic
/// order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lp01}( A , b , n ) \cr
/// 2 & \quad \textbf{foreach}\ x \in \{ 0 , 1 \}^n
///     \text{, in lexicographic order}\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ A x \le b\ \textbf{then return}\ x \cr
/// 4 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, \f$ A \f$ has \f$ n \f$ columns and as many rows as
///      \f$ b \f$
/// \post a vector returned is 0-1 and satisfies \f$ A x \le b \f$
inline std::optional< std::vector< int > > lp01(
                              const std::vector< std::vector< long > > & A ,
                              const std::vector< long > & b , int n );

/*--------------------------------------------------------------------------*/
/*------------------------------- NUMBERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of the places of \f$ A \f$ of sum \f$ k \f$, by backtrack, or
/// nothing
///
/// In pseudocode (chapter 18), each element taken and then left, a sum above
/// \f$ k \f$ pruned since the numbers are not negative:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S , i , s )\quad \triangleright
///     \ \text{started with } S = \emptyset ,\ i = 0 ,\ s = 0 \cr
/// 2 & \quad \textbf{if}\ s = k\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ i = n \text{ or } s > k\ \textbf{then return false}
///     \cr
/// 4 & \quad \textbf{if}\ \mathrm{extend}( S \cup \{ i \} , i + 1 , s + A[ i ]
///     )\ \textbf{then return true} \cr
/// 5 & \quad \textbf{return}\ \mathrm{extend}( S , i + 1 , s )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post the places returned are distinct, of sum \f$ k \f$
inline std::optional< Nodes > subsetSumExists( const std::vector< long > & A ,
                                               long k );

/// the places of a half of \f$ A \f$ of half the sum, or nothing
///
/// In pseudocode (chapter 18):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{partition}( A ) \cr
/// 2 & \quad s \gets \sum_i A[ i ] \cr
/// 3 & \quad \textbf{if}\ s \text{ is odd}\ \textbf{then return nil} \cr
/// 4 & \quad \textbf{return}\ \mathrm{subsetSumExists}( A , s / 2 )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post the places returned are of half the sum
inline std::optional< Nodes > partition( const std::vector< long > & A );

/// objects of volume at most \f$ C \f$ and profit at least \f$ k \f$ (the
/// enZaino() of the book)
///
/// In pseudocode (exercise 18.5), each object taken and then left, a volume
/// above \f$ C \f$ pruned:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S , i , \mathit{tp} , \mathit{tv}
///     )\quad \triangleright\ \text{started with } S = \emptyset
///     \text{ and } 0 , 0 , 0 \cr
/// 2 & \quad \textbf{if}\ \mathit{tv} > C\ \textbf{then return false} \cr
/// 3 & \quad \textbf{if}\ \mathit{tp} \ge k\ \textbf{then return true} \cr
/// 4 & \quad \textbf{if}\ i = n\ \textbf{then return false} \cr
/// 5 & \quad \textbf{if}\ \mathrm{extend}( S \cup \{ i \} , i + 1 ,
///     \mathit{tp} + p[ i ] , \mathit{tv} + v[ i ] )
///     \ \textbf{then return true} \cr
/// 6 & \quad \textbf{return}\ \mathrm{extend}( S , i + 1 , \mathit{tp} ,
///     \mathit{tv} )
/// \end{array}}
/// \f]
/// \pre \f$ p \f$ and \f$ v \f$ have the same size, \f$ v \f$ not negative
/// \post the objects returned have volume at most \f$ C \f$ and profit at
///       least \f$ k \f$
inline std::optional< Nodes > knapsackDecision( const std::vector< long > & p ,
                                                const std::vector< long > & v ,
                                                long C , long k );

/// the processor of each program, so that \f$ m \f$ processors run them all
/// within time \f$ d \f$, or nothing (also if \f$ d < 0 \f$, even with no
/// programs, since an idle processor is loaded 0)
///
/// In pseudocode (chapter 18), the empty processors being all equal, a
/// program is tried on the first of them only:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{assign}( i )\quad \triangleright
///     \ \text{started with } i = 0 \text{, all the loads } 0 \cr
/// 2 & \quad \textbf{if}\ i = n\ \textbf{then return true} \cr
/// 3 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{load}[ q ] + t[ i ] > d
///     \ \textbf{then continue} \cr
/// 5 & \qquad \mathit{load}[ q ] \gets \mathit{load}[ q ] + t[ i ];\ a[ i ]
///     \gets q \cr
/// 6 & \qquad \textbf{if}\ \mathrm{assign}( i + 1 )
///     \ \textbf{then return true} \cr
/// 7 & \qquad \mathit{load}[ q ] \gets \mathit{load}[ q ] - t[ i ] \cr
/// 8 & \qquad \textbf{if}\ \mathit{load}[ q ] = 0
///     \ \textbf{then return false} \cr
/// 9 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$ and \f$ t \f$ is not negative
/// \post each processor returned is loaded at most \f$ d \f$
inline std::optional< std::vector< int > > multiprocessorScheduling(
                        const std::vector< long > & t , int m , long d );

/// sets of three of \f$ \{ 0 , \ldots , m - 1 \} \f$ that partition it, by
/// places, the ones that meet those chosen pruned, or nothing (the
/// enCopertura() of the book)
///
/// In pseudocode (exercise 18.7), each set \f$ Y[ j ] \f$ taken, if it meets
/// none of those chosen, and then left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S , j , \mathit{left} )\quad
///     \triangleright\ \text{started with } S = \emptyset ,\ j = 0 ,
///     \ \mathit{left} = m \cr
/// 2 & \quad \textbf{if}\ \mathit{left} = 0\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ j = |Y|\ \textbf{then return false} \cr
/// 4 & \quad \textbf{if}\ Y[ j ] \text{ has three distinct elements, none }
///     \text{covered}\ \textbf{then} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{extend}( S \cup \{ j \} , j + 1 ,
///     \mathit{left} - 3 )\ \textbf{then return true} \cr
/// 6 & \quad \textbf{return}\ \mathrm{extend}( S , j + 1 , \mathit{left} )
/// \end{array}}
/// \f]
/// \pre each set has three distinct elements of \f$ [ 0 , m ) \f$
/// \post the sets returned partition \f$ [ 0 , m ) \f$
inline std::optional< Nodes > exactCover3(
                      const std::vector< std::array< int , 3 > > & Y , int m );

/// a matching of \f$ q \f$ triples \f$ ( w , x , v ) \f$ of \f$ M \f$ with
/// distinct \f$ w \f$, \f$ x \f$ and \f$ v \f$, the three sets being
/// \f$ [ 0 , q ) \f$, or nothing
///
/// In pseudocode (chapter 18), by the reduction to EXACT COVER BY 3-SETS,
/// with the three coordinates made disjoint:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{threeDimensionalMatching}( M , q ) \cr
/// 2 & \quad Y \gets ( \{ w , q + x , 2 q + v \} : ( w , x , v ) \in M ) \cr
/// 3 & \quad \textbf{return}\ \mathrm{exactCover3}( Y , 3 q )
/// \end{array}}
/// \f]
/// \pre the coordinates are in \f$ [ 0 , q ) \f$
/// \post a matching returned has \f$ q \f$ triples
inline std::optional< Nodes > threeDimensionalMatching(
                     const std::vector< std::array< int , 3 > > & M , int q );

/// the \f$ n \f$ bribes \f$ M \f$ to the \f$ n \f$ politicians,
/// \f$ \mathit{ps}[ i ] \f$ if the \f$ i \f$-th is of the party PS, so that
/// the PS gets twice the party PDF; the bribe of each politician, or nothing
/// (the mazzette() of the book)
///
/// In pseudocode (exercises 18.1 and 18.4), the permutations of the bribes in
/// lexicographic order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bribes}( M , \mathit{ps} ) \cr
/// 2 & \quad \textbf{foreach}\ \text{permutation } \pi \text{ of } M
///     \text{, in}\ \text{lexicographic order}\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ \sum_{i : \mathit{ps}[ i ]} \pi[ i ] = 2
///     \sum_{i : \textbf{not}\ \mathit{ps}[ i ]} \pi[ i ]
///     \ \textbf{then return}\ \pi \cr
/// 4 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ M \f$ and \f$ \mathit{ps} \f$ have the same size
/// \post the bribes returned are a permutation of \f$ M \f$
inline std::optional< std::vector< long > > bribes(
                      std::vector< long > M , const std::vector< bool > & ps );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/Problems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Problems.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Problems.h ---------------------------*/
/*--------------------------------------------------------------------------*/
