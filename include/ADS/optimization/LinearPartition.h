/*--------------------------------------------------------------------------*/
/*------------------------- File LinearPartition.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The linear partition problem (also the painter's partition, or the
 * bottleneck partition of a chain): the items A[ 0 .. n - 1 ] split into k
 * contiguous parts so that the largest sum of a part is the least, e.g.
 * n boards of lengths A_i to be painted by k painters, each one a block of
 * consecutive boards, in the least time.
 *
 * - linearPartition() solves it for items of any sign and exactly k
 *   nonempty parts by the dynamic programming on the prefix sums S,
 *   \f$ D[ t ][ i ] = \min_{t - 1 \leq j < i} \max( D[ t - 1 ][ j ] ,
 *   S[ i ] - S[ j ] ) \f$, \f$ D[ 1 ][ i ] = S[ i ] \f$, with the best cut
 *   kept for each entry; \f$ O( k n^2 ) \f$ time and \f$ O( k n ) \f$ space.
 * - painterPartition() solves it for integer items not negative and at most k
 *   parts by a binary search on the answer x: the greedy that closes a part
 *   only when the next item does not fit in x uses the fewest parts, so x is
 *   feasible iff the greedy needs at most k of them;
 *   \f$ O( n \log \sum_i A_i ) \f$ time and \f$ O( 1 ) \f$ space. With items
 *   not negative and k <= n, at most k parts and exactly k nonempty ones give
 *   the same value.
 *
 * In the literature, the problem on a chain of n items is solved in
 * \f$ O( n ) \f$ time for k fixed, and in \f$ O( n + k^2 \log^2 n ) \f$ in
 * general, by parametric search on the prefix sums (Frederickson); the dynamic
 * programming above has the quadrangle inequality, so that the monotone cuts
 * of Knuth and Yao bring it to \f$ O( k n ) \f$ for items not negative.
 *
 * The contracts [see Contract.h]: 1 <= k <= n for linearPartition(), and
 * its cuts give the cost found (checked at level 3); k >= 1 and the items
 * not negative for painterPartition(). The functions are defined in
 * LinearPartition.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see S. S. Skiena, The Algorithm Design Manual, 3rd ed., Springer, 2020,
 *      section 10.7
 * \see G. N. Frederickson, Optimal algorithms for tree partitioning, Proc.
 *      2nd ACM-SIAM Symposium on Discrete Algorithms, 168-177, 1991
 * \see F. F. Yao, Efficient dynamic programming using quadrangle
 *      inequalities, Proc. 12th ACM Symposium on Theory of Computing,
 *      429-435, 1980, doi:10.1145/800141.804691
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

#ifndef __ADS_LinearPartition
 #define __ADS_LinearPartition
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the least largest sum of a part over the splits of A in k nonempty
/// contiguous parts, and the starts of the parts after the first one
/// \pre 1 <= k <= |A|
/// \post k - 1 increasing starts in [ 1 , |A| ) of the cost found
template< class T >
std::pair< T , std::vector< std::size_t > > linearPartition(
                                  const std::vector< T > & A , std::size_t k );

/// the least largest sum of a part over the splits of A in at most k
/// contiguous parts; 0 for A empty
/// \pre k >= 1 and A not negative
template< std::integral T >
T painterPartition( const std::vector< T > & A , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/LinearPartition.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinearPartition.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File LinearPartition.h -----------------------*/
/*--------------------------------------------------------------------------*/
