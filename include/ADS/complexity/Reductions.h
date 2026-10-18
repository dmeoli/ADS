/*--------------------------------------------------------------------------*/
/*--------------------------- File Reductions.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial reductions of chapter 18: a function f from the inputs
 * of A to the inputs of B, computable in polynomial time, such that x is a
 * YES of A if and only if f( x ) is a YES of B; then A is no harder than B
 * (A \f$ \propto \f$ B).
 *
 * - DOMINO LIMITATO to CRICCA (Teorema 18.2): a node u_hij for the tile h
 *   in the square ( i , j ), only u_000 in ( 0 , 0 ), joined to the nodes
 *   of the squares not adjacent and to those of the adjacent squares whose
 *   tile fits, and k = n^2;
 * - DOMINO LIMITATO to SODDISFATTIBILITA' (Teorema 18.3): u_000, a tile at
 *   least and one at most in each square, and no two tiles that do not fit
 *   side by side;
 * - DOMINO LIMITATO to PROGRAMMAZIONE LINEARE 0/1 (Teorema 18.4): x_000 =
 *   1, one tile per square, and x_hij plus the tiles that do not fit next
 *   to it at most 1 (the example of the text writes one inequality per
 *   pair, which is the same since a square holds one tile);
 * - CRICCA to INSIEME INDIPENDENTE (Teorema 18.5), the complement graph;
 *   INSIEME INDIPENDENTE to COPERTURA CON NODI (exercise 18.11), the
 *   complement of a set is a cover, k' = n - k;
 * - CIRCUITO HAMILTONIANO to COMMESSO VIAGGIATORE (Teorema 18.6): the
 *   distance 1 on the edges and 2 elsewhere, k = n;
 * - PARTIZIONE to SCHEDULING MULTIPROCESSORE, SOMMA DI SOTTOINSIEME and
 *   ZAINO (Teoremi 18.7-18.9), half of the sum as the bound;
 * - CIRCUITO HAMILTONIANO to CICLO HAMILTONIANO (exercise 18.12), both the
 *   orientations of each edge; CICLO HAMILTONIANO to CAMMINO HAMILTONIANO
 *   (exercise 18.13), the node 0 split into a start with its edges out and
 *   an end with its edges in; CICLO HAMILTONIANO to CATENA HAMILTONIANA
 *   (exercise 18.14), that path with each node made a chain of three and a
 *   pendant node at each end;
 * - CAMMINO HAMILTONIANO from r to t to CAMMINO MASSIMO, k = n - 1, and
 *   CATENA HAMILTONIANA to ALBERO DI COPERTURA LIMITATO, k = 2 (exercises
 *   18.15 and 18.16);
 * - ORDINAMENTO to INVILUPPO CONVESSO: the points ( a_i , a_i^2 ) on the
 *   parabola are all vertices of the hull, in the order of the a_i, so
 *   the hull costs \f$ \Omega( n \log n ) \f$.
 *
 * The contracts [see Contract.h]: each reduction checks that its input is an
 * instance of the problem reduced (a square or unoriented graph, a nonempty
 * set of tiles, numbers not negative) and that its output has the shape of an
 * instance of the target problem. The operations are defined in
 * Reductions.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 18.5 and 18.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

/// the place of the variable or node of the tile h in the square ( i , j )
/// \pre n >= 1, h >= 0, and ( i , j ) a square of the n x n board
inline int tileVar( int h , int i , int j , int n );

/// DOMINO LIMITATO to CRICCA: the graph on the m n^2 places of tileVar(),
/// the ones of ( 0 , 0 ) but u_000 isolated, and k
/// \pre n >= 1 and D is not empty
/// \post an unoriented graph of m n^2 nodes, and k = n^2
inline std::pair< Adjacency , int > dominoToClique(
                                   const std::vector< Tile > & D , int n );

/// DOMINO LIMITATO to SODDISFATTIBILITA', on the m n^2 variables of
/// tileVar()
/// \pre n >= 1 and D is not empty
/// \post the literals are variables of [ 0 , m n^2 )
inline Cnf dominoToSat( const std::vector< Tile > & D , int n );

/// DOMINO LIMITATO to PROGRAMMAZIONE LINEARE 0/1: A x <= b on the m n^2
/// variables of tileVar(), each equation as two inequalities
/// \pre n >= 1 and D is not empty
/// \post A has m n^2 columns and as many rows as b
inline std::pair< std::vector< std::vector< long > > , std::vector< long > >
dominoToLp01( const std::vector< Tile > & D , int n );

/// the complement graph: CRICCA of size k to INSIEME INDIPENDENTE of size k
/// \pre G is square
/// \post the edges between distinct nodes are those that G lacks
inline Adjacency complement( const Adjacency & G );

/// CIRCUITO HAMILTONIANO to COMMESSO VIAGGIATORE: the distances, k = n
/// \pre G is unoriented, i.e., square and symmetric
/// \post a square matrix of the size of G
inline std::vector< std::vector< long > > hamiltonToTsp( const Adjacency & G );

/// half the sum, or -1 if the sum is odd (then no partition: the bound
/// -1 has no YES in any of the three problems)
/// \pre A is not negative
/// \post -1 or half the sum
inline long halfSum( const std::vector< long > & A );

/// CICLO HAMILTONIANO to CAMMINO HAMILTONIANO from n to 0: the node 0 split,
/// a new node n with the edges into 0
/// \pre G is square
/// \post a square graph of n + 1 nodes
inline Adjacency cycleToPath( const Adjacency & G );

/// CICLO HAMILTONIANO to CATENA HAMILTONIANA: the path of cycleToPath(), each
/// node v a chain v_in - v_mid - v_out, each edge ( u , v ) an edge
/// u_out - v_in, and pendant nodes at the start and at the end
/// \pre G is square
/// \post an unoriented graph of 3 ( n + 1 ) + 2 nodes
inline Adjacency cycleToChain( const Adjacency & G );

/// ORDINAMENTO to INVILUPPO CONVESSO: the points ( a , a^2 )
/// \pre the squares of a fit in a long
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
