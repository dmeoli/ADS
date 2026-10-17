/*--------------------------------------------------------------------------*/
/*---------------------------- File Problems.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems in NP of chapter 18, each with its polynomial certificate,
 * which checks a proposed solution, and an enumeration by backtrack, which
 * decides the problem in exponential time and returns a solution if there
 * is one. The graphs are adjacency matrices, symmetric when unoriented;
 * the formulas in conjunctive normal form are clauses of literals, +( v +
 * 1 ) for the variable v and -( v + 1 ) for its negation; a system of 0/1
 * linear programming is A x <= b.
 *
 * - clique() (CRICCA, enCricca()), independentSet(), vertexCover(),
 *   coloring() (exercise 18.6), bisection();
 * - tsp() (COMMESSO VIAGGIATORE, enTsp(), pruned when the partial length
 *   exceeds k), hamiltonianCircuit() (unoriented), hamiltonianCycle() and
 *   hamiltonianPath() (oriented, exercises 18.12 and 18.13),
 *   hamiltonianChain() (unoriented, exercise 18.14), longestPath() and
 *   longestPathDag(), polynomial on an acyclic graph (exercise 18.15),
 *   boundedSpanningTree() (exercise 18.16);
 * - satisfiable() (SODDISFATTIBILITA'), lp01() (PROGRAMMAZIONE LINEARE
 *   0/1);
 * - partition(), subsetSumExists(), knapsackDecision() (ZAINO, exercise
 *   18.5), multiprocessorScheduling(), exactCover3() (exercise 18.7),
 *   threeDimensionalMatching(), bribes() (exercises 18.1 and 18.4).
 *
 * The contracts [see Contract.h]: the graphs are square adjacency matrices,
 * unoriented (symmetric) where the problem is, the literals name the variables
 * of the formula, the numbers of the numerical problems are not negative; each
 * certificate returned by an enumeration is checked as a postcondition. The
 * operations are defined in Problems.tpp.
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
 *         Universita' di Pisa \n
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
using Clause = std::vector< int >;                      ///< +-( v + 1 )
using Cnf = std::vector< Clause >;                      ///< a formula
using Nodes = std::vector< int >;                       ///< a set, a path

/*--------------------------------------------------------------------------*/
/*------------------------------ GRAPHS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// certificatoCricca(): S, of distinct nodes, has k of them or more, all
/// adjacent
/// \pre G is square
inline bool cliqueCertificate( const Adjacency & G , const Nodes & S , int k );

namespace detail {

/// the sets of k nodes of [ 0 , n ), extended only while ok( S , v )
template< class Ok , class Done >
bool chooseK( int n , int k , Ok && ok , Done && done );

}  // end( namespace detail )

/// enCricca(): k nodes all adjacent, or nothing
/// \pre G is unoriented, i.e., square and symmetric
/// \post a set returned is a clique of k nodes
inline std::optional< Nodes > clique( const Adjacency & G , int k );

/// k nodes no two adjacent, or nothing
/// \pre G is unoriented, i.e., square and symmetric
/// \post a set returned has k nodes, no two adjacent
inline std::optional< Nodes > independentSet( const Adjacency & G , int k );

/// at most k nodes that touch every edge, or nothing
/// \pre G is unoriented, i.e., square and symmetric
/// \post a set returned has at most k nodes and covers the edges
inline std::optional< Nodes > vertexCover( const Adjacency & G , int k );

/// a coloring of the nodes with at most k colors, adjacent nodes of
/// different colors, by backtrack, or nothing (exercise 18.6)
/// \pre G is unoriented, i.e., square and symmetric, and k >= 0
/// \post a coloring returned is proper, with colors in [ 0 , k )
inline std::optional< std::vector< int > > coloring( const Adjacency & G ,
                                                     int k );

/// a partition of the nodes in two halves cut by at most k edges, or
/// nothing; the half of node 0
/// \pre G is unoriented, i.e., square and symmetric
/// \post a half returned has n / 2 nodes and cuts at most k edges
inline std::optional< Nodes > bisection( const Adjacency & G , int k );

/// certificatoTsp(): the tour P, a permutation, is at most k long
/// \pre D is square
template< class T >
bool tspCertificate( const std::vector< std::vector< T > > & D , T k ,
                     const Nodes & P );

/// enTsp(): a tour of length at most k from city 0, the partial tours
/// longer than k pruned, or nothing
/// \pre D is square
/// \post a tour returned passes tspCertificate()
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

/// an unoriented circuit through every node once (n >= 3), or nothing
/// \pre G is unoriented, i.e., square and symmetric
/// \post a circuit returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianCircuit( const Adjacency & G );

/// an oriented cycle through every node once (n >= 2), or nothing
/// \pre G is square
/// \post a cycle returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianCycle( const Adjacency & G );

/// an oriented path through every node once, from r and to t if given
/// \pre G is square, r and t are nodes or -1
/// \post a path returned visits each node once along the edges, from r and to
///       t if given
inline std::optional< Nodes > hamiltonianPath( const Adjacency & G ,
                                               int r = -1 , int t = -1 );

/// an unoriented chain through every node once
/// \pre G is unoriented, i.e., square and symmetric
/// \post a chain returned visits each node once along the edges
inline std::optional< Nodes > hamiltonianChain( const Adjacency & G );

/// a simple path from r to t of at least k edges, by backtrack
/// \pre G is square, r and t are nodes
/// \post a path returned is simple, from r to t, of at least k edges
inline std::optional< Nodes > longestPath( const Adjacency & G , int r ,
                                           int t , int k );

/// the most edges of a path from r to t in an acyclic oriented graph, -1
/// if t is not reachable; O( n^2 ) by the topological order
/// \pre G is square and acyclic, r and t are nodes
inline int longestPathDag( const Adjacency & G , int r , int t );

/// a spanning tree of the unoriented graph with at most k edges at each
/// node, as n - 1 edges, by enumeration, or nothing
/// \pre G is unoriented, i.e., square and symmetric
/// \post the edges returned form a spanning tree with at most k at each node
inline std::optional< std::vector< std::pair< int , int > > >
boundedSpanningTree( const Adjacency & G , int k );

/*--------------------------------------------------------------------------*/
/*---------------------- LOGIC AND LINEAR PROGRAMMING ----------------------*/
/*--------------------------------------------------------------------------*/

/// the certificate: x satisfies every clause
/// \pre the literals of F are variables of x
inline bool satCertificate( const Cnf & F , const std::vector< bool > & x );

/// an assignment of the n variables that satisfies F, by backtrack with
/// a clause checked as soon as its variables are set, or nothing
/// \pre n >= 0 and the literals of F are variables of [ 0 , n )
/// \post an assignment returned passes satCertificate()
inline std::optional< std::vector< bool > > satisfiable( const Cnf & F ,
                                                         int n );

/// a 0/1 vector x with A x <= b, by enumeration, or nothing
/// \pre n >= 0, A has n columns and as many rows as b
/// \post a vector returned is 0/1 and satisfies A x <= b
inline std::optional< std::vector< int > > lp01(
                              const std::vector< std::vector< long > > & A ,
                              const std::vector< long > & b , int n );

/*--------------------------------------------------------------------------*/
/*------------------------------- NUMBERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of the places of A of sum k, by backtrack, or nothing
/// \pre A is not negative
/// \post the places returned are distinct, of sum k
inline std::optional< Nodes > subsetSumExists( const std::vector< long > & A ,
                                               long k );

/// the places of a half of A of half the sum, or nothing
/// \pre A is not negative
/// \post the places returned are of half the sum
inline std::optional< Nodes > partition( const std::vector< long > & A );

/// enZaino(): objects of volume at most C and profit at least k
/// \pre p and v have the same size, v not negative
/// \post the objects returned have volume at most C and profit at least k
inline std::optional< Nodes > knapsackDecision( const std::vector< long > & p ,
                                                const std::vector< long > & v ,
                                                long C , long k );

/// the processor of each program, so that m processors run them all within
/// time d, or nothing
/// \pre m >= 1 and t is not negative
/// \post each processor returned is loaded at most d
inline std::optional< std::vector< int > > multiprocessorScheduling(
                        const std::vector< long > & t , int m , long d );

/// enCopertura(): sets of three of { 0 , ... , m - 1 } that partition it,
/// by places, the ones that meet those chosen pruned, or nothing
/// \pre each set has three distinct elements of [ 0 , m )
/// \post the sets returned partition [ 0 , m )
inline std::optional< Nodes > exactCover3(
                      const std::vector< std::array< int , 3 > > & Y , int m );

/// a matching of q triples ( w , x , v ) of M with distinct w, x and v, the
/// three sets being [ 0 , q ), or nothing
/// \pre the coordinates are in [ 0 , q )
/// \post a matching returned has q triples
inline std::optional< Nodes > threeDimensionalMatching(
                     const std::vector< std::array< int , 3 > > & M , int q );

/// mazzette(): the n bribes M to the n politicians, ps[ i ] if the i-th
/// is of the PS, so that the PS gets twice the PDF; the bribe of each
/// politician, or nothing
/// \pre M and ps have the same size
/// \post the bribes returned are a permutation of M
inline std::optional< std::vector< long > > bribes(
                      std::vector< long > M , const std::vector< bool > & ps );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Problems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Problems.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Problems.h ---------------------------*/
/*--------------------------------------------------------------------------*/
