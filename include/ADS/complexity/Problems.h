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

namespace ADS
{

using Adjacency = std::vector< std::vector< bool > >;  ///< an adjacency
using Clause = std::vector< int >;                     ///< some literals
using Cnf = std::vector< Clause >;                     ///< a formula
using Nodes = std::vector< int >;                      ///< a set, a path

/*--------------------------------------------------------------------------*/
/*------------------------------ GRAPHS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether \f$ S \f$, of distinct nodes, has \f$ k \f$ of them or more, all
/// adjacent (the certificatoCricca() of the book)
///
/// In pseudocode (chapter 18), \f$ O( |S|^2 ) \f$ time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cliqueCertificate}( G , S , k )\quad
///     \triangleright\ O( |S|^2 ) \cr
/// 2 & \quad \textbf{return}\ k \le |S| \text{ and every two places } a < b
///     \text{ of } S \text{ hold distinct nodes with } G[ S[ a ] ][ S[ b ] ]
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square
inline bool cliqueCertificate( const Adjacency & G , const Nodes & S , int k );

namespace detail
{

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
/// 2 & \quad \textbf{return}\ \emptyset \text{ if } k \le 0 \text{, else }
///     \mathrm{choose}( \emptyset , 0 ) \cr
/// 3 & \textbf{function}\ \mathrm{choose}( S , i )\quad \triangleright\ S
///     \text{ a clique of nodes before } i \cr
/// 4 & \quad \textbf{if}\ |S| = k\ \textbf{then return}\ S \cr
/// 5 & \quad \textbf{if}\ k - |S| > n - i\ \textbf{then return nil}\quad
///     \triangleright\ \text{not enough nodes left} \cr
/// 6 & \quad \textbf{foreach}\ v \in [ i , n )
///     \text{ adjacent to every node of } S \text{, in increasing order}
///     \ \textbf{do} \cr
/// 7 & \qquad R \gets \mathrm{choose}( S \cup \{ v \} , v + 1 );
///     \ \textbf{if}\ R \ne \textbf{nil}\ \textbf{then return}\ R \cr
/// 8 & \quad \textbf{return nil}
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
/// 2 & \quad \textbf{return}\ \text{as } \mathrm{clique}( G , k )
///     \text{, a node } v
///     \text{ being added when it is adjacent to no node of } S
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
/// 1 & \textbf{function}\ \mathrm{vertexCover}( G , k )\quad \triangleright
///     \ \text{the smallest first} \cr
/// 2 & \quad \textbf{foreach}\ S \subseteq V \text{, by size } 0 , 1 ,
///     \ldots , \min\{ k , n \} \text{ and then in lexicographic order}
///     \ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ \text{every edge has an end in } S
///     \ \textbf{then return}\ S \cr
/// 4 & \quad \textbf{return nil}
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
/// 1 & \textbf{function}\ \mathrm{color}( u )\quad \triangleright\ c[ 0 .. u
///     - 1 ] \text{ a proper coloring, from } u = 0 \cr
/// 2 & \quad \textbf{if}\ u = n\ \textbf{then return true} \cr
/// 3 & \quad \mathit{used} \gets \text{the number of the colors on } 0 ,
///     \ldots , u - 1 \text{, i.e., } 1 + \max_{v < u} c[ v ] \text{ or }
///     0\quad \triangleright\ \text{colors in order} \cr
/// 4 & \quad \textbf{foreach}\ x \in [ 0 , \min\{ k , \mathit{used} + 1 \} )
///     \text{ on no neighbor } v < u \text{ of } u\ \textbf{do}\quad
///     \triangleright\ \text{one new color at most} \cr
/// 5 & \qquad c[ u ] \gets x;\ \textbf{if}\ \mathrm{color}( u + 1 )
///     \ \textbf{then return true} \cr
/// 6 & \quad \textbf{return false}
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
/// 1 & \textbf{function}\ \mathrm{bisection}( G , k )\quad \triangleright\ 0
///     \in S \text{ breaks the symmetry of the two halves} \cr
/// 2 & \quad \textbf{if}\ n \text{ is odd}\ \textbf{then return nil} \cr
/// 3 & \quad \textbf{return}\ \text{the first } S \ni 0 \text{ of } n / 2
///     \text{ nodes crossed by at most } k \text{ edges, or } \textbf{nil}
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
/// 2 & \quad \textbf{return}\ P
///     \text{ is a permutation of the cities and the circuit } P[ 0 ] ,
///     \ldots , P[ n - 1 ] , P[ 0 ] \text{ costs } \le k
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
/// 1 & \textbf{function}\ \mathrm{extend}( S , \ell )\quad \triangleright\ S
///     \text{ a path from } 0 \text{ of length } \ell \text{, from } ( ( 0 )
///     , 0 ) \cr
/// 2 & \quad \textbf{if}\ k < \ell\ \textbf{then return false}\quad
///     \triangleright\ \text{the costs are not negative} \cr
/// 3 & \quad \textbf{if}\ |S| = n\ \textbf{then return}\ \ell + D[ S[ n - 1
///     ] ][ 0 ] \le k \cr
/// 4 & \quad \textbf{return}\ \text{whether } \mathrm{extend}( S \cdot ( c )
///     , \ell + D[ S[ |S| - 1 ] ][ c ] ) \text{ holds for a } c \notin S
///     \text{, tried by increasing } c
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square, with no negative distance, which would make the
///      pruning wrong (a tour may come back below \f$ k \f$); adding
///      \f$ c \f$ to all the distances and \f$ n c \f$ to \f$ k \f$ keeps the
///      answer
/// \post a tour returned passes `tspCertificate()`
template< class T >
std::optional< Nodes > tsp( const std::vector< std::vector< T > > & D , T k );

namespace detail
{

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
/// 1 & \textbf{function}\ \mathrm{hamilton}( P )\quad \triangleright\ P
///     \text{ a path of } G \cr
/// 2 & \quad \textbf{if}\ |P| = n\ \textbf{then return}\ ( P
///     \text{ ends at } \mathit{end} \text{, if given} ) \text{ and } (
///     \text{an edge closes it, if } \mathit{closed} ) \cr
/// 3 & \quad \textbf{return}\ \text{whether } \mathrm{hamilton}( P \cdot ( v
///     ) ) \text{ holds for a } v \notin P \text{ after the last node of } P
///     \text{, by increasing } v
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
/// 1 & \textbf{function}\ \mathrm{hamiltonianCycle}( G )\quad
///     \triangleright\ \text{a cycle passes by } 0 \cr
/// 2 & \quad \textbf{return}\ \textbf{nil} \text{ if } n < 2 \text{, else }
///     \mathrm{hamilton}( ( 0 ) ) \text{, closed and with no end}
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
/// 1 & \textbf{function}\ \mathrm{hamiltonianPath}( G , r , t )\quad
///     \triangleright\ r , t \text{ the ends, } -1 \text{ if free} \cr
/// 2 & \quad \textbf{foreach}\ \text{start } s \text{, only } r \text{ if }
///     r \ge 0\ \textbf{do}\ \textbf{if}\ \mathrm{hamilton}( ( s ) )
///     \text{, open and ending at } t \text{, finds } P
///     \ \textbf{then return}\ P \cr
/// 3 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square, \f$ r \f$ and \f$ t \f$ are nodes or \f$ -1 \f$
/// \post a path returned visits each node once along the edges, from
///       \f$ r \f$ and to \f$ t \f$ if given
inline std::optional< Nodes > hamiltonianPath( const Adjacency & G ,
					       int r = -1 ,
					       int t = -1 );

/// an unoriented chain through every node once
///
/// In pseudocode (exercise 18.14):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hamiltonianChain}( G )\quad
///     \triangleright\ G \text{ unoriented: a symmetric matrix} \cr
/// 2 & \quad \textbf{return}\ \mathrm{hamiltonianPath}( G , -1 , -1 )
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
/// 1 & \textbf{function}\ \mathrm{extend}( P )\quad \triangleright\ P
///     \text{ a simple path from } r \text{, from } P = ( r ) \cr
/// 2 & \quad \textbf{if}\ P \text{ ends at } t\ \textbf{then return}\ k \le
///     |P| - 1\quad \triangleright\ \text{its edges} \cr
/// 3 & \quad \textbf{return}\ \text{whether } \mathrm{extend}( P \cdot ( v )
///     ) \text{ holds for a } v \notin P \text{ after the last node of } P
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is square, \f$ r \f$ and \f$ t \f$ are nodes
/// \post a path returned is simple, from \f$ r \f$ to \f$ t \f$, of at least
///       \f$ k \f$ edges
inline std::optional< Nodes > longestPath( const Adjacency & G , int r ,
					   int t ,
					   int k );

/// the most edges of a path from \f$ r \f$ to \f$ t \f$ in an acyclic
/// oriented graph, \f$ -1 \f$ if \f$ t \f$ is not reachable;
/// \f$ O( n^2 ) \f$ by the topological order
///
/// In pseudocode (exercise 18.15), one scan in topological order, as
/// `dagLongestPaths()` of `Paths.h`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPathDag}( G , r , t )\quad
///     \triangleright\ L[ v ] \text{ the longest path from } r \text{, } -1
///     \text{ if none} \cr
/// 2 & \quad L[ v ] \gets -1 \text{ for every node } v;\ L[ r ] \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ u \text{ in a topological order with } L[ u ]
///     \ge 0\ \textbf{do} \cr
/// 4 & \qquad L[ v ] \gets \max\{ L[ v ] , L[ u ] + 1 \}
///     \text{ for each edge } ( u , v ) \cr
/// 5 & \quad \textbf{return}\ L[ t ]
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
///     \ \text{the edges } E[ i ] , \ldots , E[ m ] \text{ to decide, from }
///     ( \emptyset , 1 ) \cr
/// 2 & \quad \textbf{if}\ |T| = n - 1\ \textbf{then return true}\quad
///     \triangleright\ \text{a spanning tree of degree} \le k \cr
/// 3 & \quad \textbf{if}\ n - 1 - |T| > m - i + 1
///     \ \textbf{then return false}\quad \triangleright
///     \ \text{too few edges left, } i > m \text{ included} \cr
/// 4 & \quad [ u , v ] \gets E[ i ] \cr
/// 5 & \quad \textbf{if}\ u , v \text{ have degree } < k \text{ in } T
///     \text{, are in two trees of } T \text{, and } \mathrm{extend}( T \cup
///     \{ [ u , v ] \} , i + 1 )\ \textbf{then return true} \cr
/// 6 & \quad \textbf{return}\ \mathrm{extend}( T , i + 1 )
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
/// 2 & \quad \textbf{return}\ \text{whether every clause of } F
///     \text{ has a literal true under } x
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
/// 1 & \textbf{function}\ \mathrm{assign}( v )\quad \triangleright\ x[ 0 ..
///     v - 1 ] \text{ fixed, from } v = 0 \cr
/// 2 & \quad \textbf{if}\ \text{a clause whose last variable is } v
///     \text{ is false under } x\ \textbf{then return false}\quad
///     \triangleright\ \text{now fully fixed} \cr
/// 3 & \quad \textbf{if}\ v = n\ \textbf{then return true} \cr
/// 4 & \quad \textbf{foreach}\ b \in ( \textbf{false} , \textbf{true} )
///     \ \textbf{do}\ x[ v ] \gets b;\ \textbf{if}\ \mathrm{assign}( v + 1
///     )\ \textbf{then return true} \cr
/// 5 & \quad \textbf{return false}
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
/// 2 & \quad \textbf{return}\ \text{the first } x \in \{ 0 , 1 \}^n
///     \text{ in lexicographic order with } A x \le b \text{, or }
///     \textbf{nil}\quad \triangleright\ 2^n \text{ tries}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, \f$ A \f$ has \f$ n \f$ columns and as many rows as
///      \f$ b \f$
/// \post a vector returned is 0-1 and satisfies \f$ A x \le b \f$
inline std::optional< std::vector< int > >
lp01( const std::vector< std::vector< long > > & A ,
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
///     \ s \text{ the sum of } S \text{, from } ( \emptyset , 0 , 0 ) \cr
/// 2 & \quad \textbf{if}\ s = k\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ k < s\ \textbf{or}\ i = n
///     \ \textbf{then return false}\quad \triangleright
///     \ \text{the items are not negative} \cr
/// 4 & \quad \textbf{return}\ \mathrm{extend}( S \cup \{ i \} , i + 1 , s +
///     A[ i ] )\ \textbf{or}\ \mathrm{extend}( S , i + 1 , s )\quad
///     \triangleright\ \text{with } i \text{ first}
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
/// 1 & \textbf{function}\ \mathrm{partition}( A )\quad \triangleright
///     \ \text{a subset of half the sum} \cr
/// 2 & \quad s \gets \textstyle\sum_i A[ i ] \cr
/// 3 & \quad \textbf{return}\ \textbf{nil} \text{ if } s
///     \text{ is odd, else } \mathrm{subsetSumExists}( A , s / 2 )
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
///     )\quad \triangleright\ \text{the profit and the volume of } S
///     \text{, from } ( \emptyset , 0 , 0 , 0 ) \cr
/// 2 & \quad \textbf{if}\ C < \mathit{tv}\ \textbf{then return false}\quad
///     \triangleright\ \text{too heavy} \cr
/// 3 & \quad \textbf{if}\ k \le \mathit{tp}\ \textbf{then return true} \cr
/// 4 & \quad \textbf{if}\ i = n\ \textbf{then return false} \cr
/// 5 & \quad \textbf{return}\ \mathrm{extend}( S \cup \{ i \} , i + 1 ,
///     \mathit{tp} + p[ i ] , \mathit{tv} + v[ i ] )\ \textbf{or}
///     \ \mathrm{extend}( S , i + 1 , \mathit{tp} , \mathit{tv} )
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
///     \ \text{the jobs before } i \text{ placed, from } i = 0
///     \text{ and no load} \cr
/// 2 & \quad \textbf{if}\ i = n\ \textbf{then return true} \cr
/// 3 & \quad \textbf{foreach}\ \text{processor } q \text{, in order, with }
///     \mathit{load}[ q ] + t[ i ] \le d\ \textbf{do} \cr
/// 4 & \qquad ( \mathit{load}[ q ] , a[ i ] ) \gets ( \mathit{load}[ q ] +
///     t[ i ] , q );\ \textbf{if}\ \mathrm{assign}( i + 1 )
///     \ \textbf{then return true} \cr
/// 5 & \qquad \mathit{load}[ q ] \gets \mathit{load}[ q ] - t[ i ] \cr
/// 6 & \qquad \textbf{if}\ \mathit{load}[ q ] = 0
///     \ \textbf{then return false}\quad \triangleright
///     \ \text{the other empty processors are the same} \cr
/// 7 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$ and \f$ t \f$ is not negative
/// \post each processor returned is loaded at most \f$ d \f$
inline std::optional< std::vector< int > >
multiprocessorScheduling( const std::vector< long > & t , int m , long d );

/// sets of three of \f$ \{ 0 , \ldots , m - 1 \} \f$ that partition it, by
/// places, the ones that meet those chosen pruned, or nothing (the
/// enCopertura() of the book)
///
/// In pseudocode (exercise 18.7), each set \f$ Y[ j ] \f$ taken, if it meets
/// none of those chosen, and then left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extend}( S , j , \mathit{left} )\quad
///     \triangleright\ \mathit{left} \text{ the elements not covered, from }
///     ( \emptyset , 0 , m ) \cr
/// 2 & \quad \textbf{if}\ \mathit{left} = 0\ \textbf{then return true} \cr
/// 3 & \quad \textbf{if}\ j = |Y|\ \textbf{then return false} \cr
/// 4 & \quad \textbf{if}\ Y[ j ]
///     \text{ has three distinct elements, none covered by } S \text{, and}
///     \cr
///   & \qquad \mathrm{extend}( S \cup \{ j \} , j + 1 , \mathit{left} - 3 )
///     \ \textbf{then return true} \cr
/// 5 & \quad \textbf{return}\ \mathrm{extend}( S , j + 1 , \mathit{left} )
/// \end{array}}
/// \f]
/// \pre each set has three distinct elements of \f$ [ 0 , m ) \f$
/// \post the sets returned partition \f$ [ 0 , m ) \f$
inline std::optional< Nodes >
exactCover3( const std::vector< std::array< int , 3 > > & Y , int m );

/// a matching of \f$ q \f$ triples \f$ ( w , x , v ) \f$ of \f$ M \f$ with
/// distinct \f$ w \f$, \f$ x \f$ and \f$ v \f$, the three sets being
/// \f$ [ 0 , q ) \f$, or nothing
///
/// In pseudocode (chapter 18), by the reduction to EXACT COVER BY 3-SETS,
/// with the three coordinates made disjoint:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{threeDimensionalMatching}( M , q )\quad
///     \triangleright\ \text{the three sets one after the other} \cr
/// 2 & \quad \textbf{return}\ \mathrm{exactCover3}( ( \{ w , q + x , 2 q + v
///     \} : ( w , x , v ) \in M ) , 3 q )
/// \end{array}}
/// \f]
/// \pre the coordinates are in \f$ [ 0 , q ) \f$
/// \post a matching returned has \f$ q \f$ triples
inline std::optional< Nodes >
threeDimensionalMatching( const std::vector< std::array< int , 3 > > & M ,
			  int q );

/// the \f$ n \f$ bribes \f$ M \f$ to the \f$ n \f$ politicians,
/// \f$ \mathit{ps}[ i ] \f$ if the \f$ i \f$-th is of the party PS, so that
/// the PS gets twice the party PDF; the bribe of each politician, or nothing
/// (the mazzette() of the book)
///
/// In pseudocode (exercises 18.1 and 18.4), the permutations of the bribes in
/// lexicographic order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bribes}( M , \mathit{ps} )\quad
///     \triangleright\ \text{by enumeration of the permutations} \cr
/// 2 & \quad \textbf{return}\ \text{the first permutation } \pi \text{ of }
///     M \text{ in lexicographic order with} \cr
///   & \qquad \textstyle\sum_{i : \mathit{ps}[ i ]} \pi[ i ] = 2
///     \textstyle\sum_{i : \textbf{not}\ \mathit{ps}[ i ]} \pi[ i ]
///     \text{, or } \textbf{nil}
/// \end{array}}
/// \f]
/// \pre \f$ M \f$ and \f$ \mathit{ps} \f$ have the same size
/// \post the bribes returned are a permutation of \f$ M \f$
inline std::optional< std::vector< long > >
bribes( std::vector< long > M , const std::vector< bool > & ps );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/impl/Problems.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Problems.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Problems.h ---------------------------*/
/*--------------------------------------------------------------------------*/
