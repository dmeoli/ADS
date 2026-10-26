/*--------------------------------------------------------------------------*/
/*-------------------------- File Approximation.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The approximation algorithms of section 19.2 and of the exercises 19.6
 * and 19.8. An absolute approximation gives a feasible x' with
 * \f$ | c( x' ) - c( x^* ) | / | c( x^* ) | \leq e \f$, e a constant of the
 * algorithm; an \f$ \epsilon \f$-approximation takes \f$ \epsilon \f$ in
 * input, in a time polynomial in the size and in general exponential in
 * \f$ 1 / \epsilon \f$.
 *
 * - firstFit(), ff and, on the volumes in decreasing order, ffd for BIN
 *   PACKING: no two boxes are filled at most half, so that N < 2 N*;
 * - deltaTsp() for the salesman with the triangle inequality: the nodes
 *   of a minimum spanning tree in preorder, the tree walked twice with the
 *   cities already visited skipped, at most twice the optimum (the bound is
 *   reached by the two concentric polygons of Figura 19.3);
 * - tspGap(): the distances of Teorema 19.2, 1 on the edges of G and s n
 *   elsewhere, so that an approximation within s would decide CIRCUITO
 *   HAMILTONIANO;
 * - apSubsetSum(), the \f$ \epsilon \f$-approximation of the optimization
 *   SOMMA DI SOTTOINSIEME with \f$ \epsilon = 1 / ( h + 1 ) \f$: each set of
 *   at most h elements, completed greedily on the elements in decreasing
 *   order, \f$ O( h n^{h+1} ) \f$;
 * - vertexCoverApprox(), the two ends of each edge not yet covered, a
 *   matching whose ends are at most twice a minimum cover (exercise 19.6);
 * - knapsackApprox(), an \f$ \epsilon \f$-approximation of ZAINO in time
 *   polynomial in n and \f$ 1 / \epsilon \f$ (exercise 19.8): the profits
 *   divided by \f$ K = \epsilon p_{max} / n \f$ and rounded down, and the
 *   least volume of each rounded profit by dynamic programming, at least
 *   \f$ ( 1 - \epsilon ) \f$ times the optimum in \f$ O( n^3 / \epsilon ) \f$.
 *
 * The contracts [see Contract.h]: the volumes, the profits and the sums are
 * not negative, the distances square and symmetric, and the triangle
 * inequality that the bound of deltaTsp() rests on is an audit, O( n^3 ); each
 * solution returned is checked as feasible. The operations are defined in
 * Approximation.tpp.
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

/// ff: the box of each object, the first one with room for it, the
/// objects in the order given or, if decreasing, in decreasing order of
/// volume (ffd); the boxes are 0 , 1 , ...
/// \pre each volume is in [ 0 , k ]
/// \post no box is filled beyond k
inline std::vector< int > firstFit( const std::vector< long > & A , long k ,
                                    bool decreasing = false );

/*--------------------------------------------------------------------------*/
/*----------------------------- DELTA TSP ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cost of the circuit S of the distances D
/// \pre D is square and S has distinct cities
template< class T >
T tourCost( const std::vector< std::vector< T > > & D , const Nodes & S );

/// the circuit of the nodes of a minimum spanning tree of the complete
/// graph of the symmetric distances D in preorder from 0, the children by
/// increasing name: at most twice the optimum if D is triangular
/// \pre D is square and symmetric, and triangular (an audit, O( n^3 ))
/// \post a permutation of the cities
template< class T >
Nodes deltaTsp( const std::vector< std::vector< T > > & D );

/// the distances of Teorema 19.2: 1 on the edges of G, s n elsewhere;
/// their optimum is n if G has a Hamiltonian circuit and more than s n
/// otherwise
/// \pre G is unoriented, i.e., square and symmetric, and s >= 1
/// \post a symmetric matrix of the size of G
inline std::vector< std::vector< long > > tspGap( const Adjacency & G ,
                                                  long s );

/*--------------------------------------------------------------------------*/
/*-------------------------- SUBSET SUM AND ZAINO --------------------------*/
/*--------------------------------------------------------------------------*/

/// apSubsetSum(): the sum, at most k, and the places of A of the best
/// set found from the sets of at most h elements, \f$ \epsilon =
/// 1 / ( h + 1 ) \f$; the sets of (1) in the order of the text, by size and
/// then lexicographically on A in decreasing order
/// \pre k, h >= 0 and A is not negative
/// \post a set of sum at most k, the sum returned
inline std::pair< long , Nodes > apSubsetSum( const std::vector< long > & A ,
                                              long k , int h );

/// the best profit and the objects of an \f$ \epsilon \f$-approximation of
/// ZAINO, profits p and volumes v within C, by the rounded profits
/// \pre p and v of the same size and not negative, 0 < eps <= 1, C >= 0
/// \post objects within C, their profit returned
inline std::pair< long , Nodes > knapsackApprox(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C , double eps );

/*--------------------------------------------------------------------------*/
/*---------------------------- VERTEX COVER --------------------------------*/
/*--------------------------------------------------------------------------*/

/// both ends of each edge [ u , v ], u < v, whose ends are both out of S,
/// at most twice a minimum vertex cover; O( n^2 ) on the adjacency matrix
/// \pre G is unoriented, i.e., square and symmetric
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
