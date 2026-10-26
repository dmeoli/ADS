/*--------------------------------------------------------------------------*/
/*--------------------------- File ClosestPair.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The closest pair of a set of n >= 2 points of the plane, the two points
 * at the least Euclidean distance, by divide et impera in
 * \f$ O( n \log n ) \f$ time, against the \f$ \Theta( n^2 ) \f$ of the
 * comparison of all the pairs (closestPairBrute()).
 *
 * closestPair() sorts the points by x and splits them at the median x into
 * two halves of n / 2 points, solved recursively; if \f$ \delta \f$ is the
 * smaller of their two distances, a closer pair crossing the split has
 * both points in the strip of width \f$ 2 \delta \f$ about it, and a point
 * of the strip has at most 7 others of the strip within \f$ \delta \f$
 * above it, since a \f$ \delta \times 2 \delta \f$ rectangle holds at most
 * 8 points at mutual distances at least \f$ \delta \f$; the strip, sorted
 * by y, is scanned comparing each point with the next ones until their
 * difference in y reaches \f$ \delta \f$, \f$ O( n ) \f$. The sort by y
 * comes from the merge of the halves, as in mergesort, so that
 * \f$ T( n ) = 2 T( n / 2 ) + O( n ) = O( n \log n ) \f$, which is optimal
 * for the algebraic decision trees, since the closest pair decides whether
 * n numbers are distinct, \f$ \Omega( n \log n ) \f$. The squares of the
 * distances are compared, exactly, with the integers of 64 bits. Rabin's
 * randomized algorithm, and the grid of Khuller and Matias, solve the
 * problem in \f$ O( n ) \f$ expected time, with the floor function out of
 * the decision tree model.
 *
 * closestPairSweep(), beyond the book, is the sweep of the plane of
 * Hinrichs, Nievergelt and Schorn with no tree: the points, each made a
 * key of 64 bits ( x , y ), sorted by the radix sort of ADS [see Sort.h]
 * in O( n ), and each one compared with those before it while their
 * difference in x is below \f$ \delta \f$, the least distance so far (all
 * on squares, exact on 64 bits), a point of the same x ending the scan of
 * its column as soon as the difference in y reaches \f$ \delta \f$, since
 * the column is sorted by y. A point meets the points in the strip of
 * width \f$ \delta \f$ before it, O( 1 ) of them on points spread in the
 * plane, whence O( n ) after the sort; some inputs make the strips full
 * of points far in y: after 16 n comparisons the sweep gives up and
 * starts again along y, and if that fails too the randomized grid of
 * Golin, Raman, Schwarz and Smid solves the problem: the points in a
 * random order, the i-th looked for in the 9 cells around its own of a
 * grid of side \f$ \lceil \delta_i \rceil \f$ hashed by the cells, each
 * cell with O( 1 ) points of the grid; the grid is built again with
 * the i points when \f$ \delta \f$ falls, which happens with probability
 * at most 2 / i over the random order, whence \f$ \sum_i ( 1 + i \cdot 2 /
 * i ) = O( n ) \f$ expected time on any input. The time is thus O( n )
 * when a sweep succeeds and O( n ) expected always, against the
 * \f$ \Omega( n \log n ) \f$ of the algebraic decision trees, which the
 * grid escapes by the floor function.
 *
 * The contracts [see Contract.h]: at least two points, coordinates of
 * absolute value at most \f$ 10^9 \f$, so that the square of a distance
 * fits in 63 bits; the distance found is that of its two points, a
 * postcondition. The functions are defined in ClosestPair.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera
 * \see M. I. Shamos, D. Hoey, Closest-point problems, 16th Annual
 *      Symposium on Foundations of Computer Science, 151-162, 1975,
 *      doi:10.1109/SFCS.1975.8
 * \see K. Hinrichs, J. Nievergelt, P. Schorn, Plane-sweep solves the
 *      closest pair problem elegantly, Information Processing Letters
 *      26(5), 255-261, 1988, doi:10.1016/0020-0190(88)90150-0
 * \see M. Golin, R. Raman, C. Schwarz, M. Smid, Simple randomized
 *      algorithms for closest pair problems, Nordic Journal of Computing
 *      2(1), 3-27, 1995
 * \see S. Khuller, Y. Matias, A simple randomized sieve algorithm for the
 *      closest-pair problem, Information and Computation 118(1), 34-37,
 *      1995, doi:10.1006/inco.1995.1049
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

#ifndef __ADS_ClosestPair
 #define __ADS_ClosestPair
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/geometry/ConvexHull.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/sort/Sort.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the square of the distance of p and q
inline std::int64_t squaredDistance( const Point & p , const Point & q );

/// closestPair(): the places i < j of the two closest points of P and the
/// square of their distance, by divide et impera; O( n log n )
/// \pre n >= 2, | x | and | y | at most 10^9
/// \post the square of the distance of P[ i ] and P[ j ]
inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPair( const std::vector< Point > & P );

/// the same by the sweep on the points sorted by x, closestPair() when the
/// sweep takes too long; a pair of the least distance, not always the one
/// of closestPair()
/// \pre n >= 2, | x | and | y | at most 10^9
/// \post the square of the distance of P[ i ] and P[ j ]
inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPairSweep( const std::vector< Point > & P );

/// closestPairBrute(): the same, the first pair of the least distance by
/// all the pairs; Theta( n^2 )
/// \pre n >= 2, | x | and | y | at most 10^9
inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPairBrute( const std::vector< Point > & P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/geometry/impl/ClosestPair.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ClosestPair.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ClosestPair.h -------------------------*/
/*--------------------------------------------------------------------------*/
