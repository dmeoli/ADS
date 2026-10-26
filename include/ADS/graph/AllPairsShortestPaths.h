/*--------------------------------------------------------------------------*/
/*---------------------- File AllPairsShortestPaths.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths between all the pairs of nodes, section 13.5: with
 * the nodes v_0 , ... , v_{n-1}, a path is k-bound if it passes through no
 * node of { v_k , ... , v_{n-1} } but its ends, and the least cost of the
 * k-bound paths from u to v is
 * \f[
 *   d^k[ u ][ v ] = \min\{ d^{k-1}[ u ][ v ] , d^{k-1}[ u ][ k - 1 ] +
 *                   d^{k-1}[ k - 1 ][ v ] \} ,
 * \f]
 * from \f$ d^0 \f$ the costs of the edges, 0 on the diagonal and
 * \f$ +\infty \f$ elsewhere, with p[ u ][ v ] the predecessor of v on the
 * path from u. floydWarshall() overwrites d and p for k = 1 , ... , n,
 * Theta( n^3 ) time and Theta( n^2 ) space, with costs of any sign; there
 * is a negative cycle through u if and only if d[ u ][ u ] < 0, and then
 * negativeCycle is true. The row u of p is the tree of the shortest paths
 * from u, as a vector of parents. The matrices have bound() rows, and those
 * of a deleted node, its column and its place on the diagonal too, stay at
 * \f$ +\infty \f$ and NIL.
 *
 * addEdge() updates d and p after the insertion of an edge ( a , b ) of
 * cost w in O( n^2 ), since a path that improves goes through it once:
 * \f$ d[ u ][ v ] = \min\{ d[ u ][ v ] , d[ u ][ a ] + w + d[ b ][ v ] \}
 * \f$, which needs that the edge closes no negative cycle, i.e., that
 * w + d[ b ][ a ] >= 0 (exercise 13.5). A deletion has no such update: the
 * paths that used the edge may need any other one.
 *
 * The contracts [see Contract.h]: addEdge() needs two distinct nodes of
 * the matrix and an edge that closes no negative cycle; at the level of
 * the audits, the distances found are closed under the triangle
 * inequality, bounded by the costs of the edges, and their predecessors
 * realize them, unless there is a negative cycle. The functions are
 * defined in AllPairsShortestPaths.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see R. W. Floyd, Algorithm 97: Shortest path, Communications of the ACM
 *      5(6), 345, 1962, doi:10.1145/367766.368168
 * \see S. Warshall, A theorem on Boolean matrices, Journal of the ACM 9(1),
 *      11-12, 1962, doi:10.1145/321105.321107
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

#ifndef __ADS_AllPairsShortestPaths
 #define __ADS_AllPairsShortestPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the distances, the predecessors and whether there is a negative cycle
template< class D >
struct AllPairs {
 std::vector< std::vector< D > > d;
 std::vector< std::vector< int > > p;
 bool negativeCycle = false;
 };

/// floydWarshall(): the shortest paths between all the pairs of nodes
/// \post the distances are closed under the triangle inequality, bounded
///       by the edges and realized by p, unless negativeCycle (audit)
template< Graph G >
AllPairs< typename G::Weight > floydWarshall( const G & g );

/// the insertion of the edge ( a , b ) of cost w, O( n^2 )
/// \pre a and b are distinct places of R, and w + d[ b ][ a ] >= 0
/// \post d[ a ][ b ] <= w, and the distances are closed under the triangle
///       inequality (audit)
template< class D >
void addEdge( AllPairs< D > & R , int a , int b , const D & w );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/AllPairsShortestPaths.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AllPairsShortestPaths.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File AllPairsShortestPaths.h --------------------*/
/*--------------------------------------------------------------------------*/
