/*--------------------------------------------------------------------------*/
/*--------------------------- File ClosestPair.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The closest pair of a set \f$ P \f$ of \f$ n \ge 2 \f$ points of the
 * plane, the two points at the least Euclidean distance, by divide et impera
 * in \f$ O( n \log n ) \f$ time, against the \f$ \Theta( n^2 ) \f$ of the
 * comparison of all the pairs (`closestPairBrute()`).
 *
 * `closestPair()` sorts the points by \f$ x \f$ and splits them at the median
 * \f$ x \f$ into two halves of \f$ n / 2 \f$ points, solved recursively; if
 * \f$ \delta \f$ is the smaller of their two distances, a closer pair
 * crossing the split has both points in the strip of width
 * \f$ 2 \delta \f$ about it, and a point of the strip has at most 7 others
 * of the strip within \f$ \delta \f$ above it, since a
 * \f$ \delta \times 2 \delta \f$ rectangle holds at most 8 points at
 * mutual distances at least \f$ \delta \f$; the strip, sorted by
 * \f$ y \f$, is scanned comparing each point with the next ones until their
 * difference in \f$ y \f$ reaches \f$ \delta \f$, \f$ O( n ) \f$. The
 * sort by \f$ y \f$ comes from the merge of the halves, as in mergesort, so
 * that \f$ T( n ) = 2 T( n / 2 ) + O( n ) = O( n \log n ) \f$, which is
 * optimal for the algebraic decision trees, since the closest pair decides
 * whether \f$ n \f$ numbers are distinct, \f$ \Omega( n \log n ) \f$. The
 * squares of the distances are compared, exactly, with the integers of 64
 * bits. Rabin's randomized algorithm, and the grid of Khuller and Matias,
 * solve the problem in \f$ O( n ) \f$ expected time, with the floor function
 * out of the decision tree model.
 *
 * `closestPairSweep()`, beyond the book, is the sweep of the plane of
 * Hinrichs, Nievergelt and Schorn with no tree: the points, each made a key
 * of 64 bits \f$ ( x , y ) \f$, sorted by the radix sort of ADS [see
 * `Sort.h`] in \f$ O( n ) \f$, and each one compared with those before it
 * while their difference in \f$ x \f$ is below \f$ \delta \f$, the least
 * distance so far (all on squares, exact on 64 bits), a point of the same
 * \f$ x \f$ ending the scan of its column as soon as the difference in
 * \f$ y \f$ reaches \f$ \delta \f$, since the column is sorted by
 * \f$ y \f$. A point meets the points in the strip of width
 * \f$ \delta \f$ before it, \f$ O( 1 ) \f$ of them on points spread in
 * the plane, whence \f$ O( n ) \f$ after the sort; some inputs make the
 * strips full of points far in \f$ y \f$: after \f$ 16 n \f$ comparisons
 * the sweep gives up and starts again along \f$ y \f$, and if that fails too
 * the randomized grid of Golin, Raman, Schwarz and Smid solves the problem:
 * the points in a random order, the \f$ i \f$-th looked for in the 9 cells
 * around its own of a grid of side \f$ \lceil \delta_i \rceil \f$ hashed
 * by the cells, each cell with \f$ O( 1 ) \f$ points of the grid; the grid
 * is built again with the \f$ i \f$ points when \f$ \delta \f$ falls,
 * which happens with probability at most \f$ 2 / i \f$ over the random
 * order, whence \f$ \sum_i ( 1 + i \cdot 2 / i ) = O( n ) \f$ expected
 * time on any input. The time is thus \f$ O( n ) \f$ when a sweep succeeds
 * and \f$ O( n ) \f$ expected always, against the
 * \f$ \Omega( n \log n ) \f$ of the algebraic decision trees, which the
 * grid escapes by the floor function.
 *
 * The contracts [see `Contract.h`]: at least two points, coordinates of
 * absolute value at most \f$ 10^9 \f$, so that the square of a distance
 * fits in 63 bits; the distance found is that of its two points, a
 * postcondition. The functions are defined in `ClosestPair.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 12,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
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

/// the square \f$ ( x_p - x_q )^2 + ( y_p - y_q )^2 \f$ of the distance of
/// \f$ p \f$ and \f$ q \f$
inline std::int64_t squaredDistance( const Point & p , const Point & q );

/// the places \f$ i < j \f$ of the two closest points of \f$ P \f$ and the
/// square of their distance, by divide et impera; \f$ O( n \log n ) \f$
///
/// In pseudocode (Shamos and Hoey, beyond the book), \f$ I \f$ the
/// points sorted by \f$ x \f$ and \f$ \delta^2 \f$ the least square distance
/// found so far, a global:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{closestRec}( I , \mathit{lo} ,
///     \mathit{hi} )\quad \triangleright\ \text{sorts } I[ \mathit{lo} ..
///     \mathit{hi} - 1 ] \text{ by } y \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 3\ \textbf{then} \cr
/// 3 & \qquad \text{offer all the pairs};\ \text{sort the range by } y;
///     \ \textbf{return} \cr
/// 4 & \quad \mathit{mid} \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2
///     \rfloor;\ x_m \gets x( I[ \mathit{mid} ] ) \cr
/// 5 & \quad \mathrm{closestRec}( I , \mathit{lo} , \mathit{mid} );
///     \ \mathrm{closestRec}( I , \mathit{mid} , \mathit{hi} ) \cr
/// 6 & \quad \text{merge the two halves by } y \cr
/// 7 & \quad B \gets \text{the points } p \text{ of the range with }
///     ( x( p ) - x_m )^2 < \delta^2 \text{, in order of } y \cr
/// 8 & \quad \textbf{for}\ u \gets 1\ \textbf{to}\ |B|\ \textbf{do} \cr
/// 9 & \qquad \textbf{for}\ v \gets u + 1\ \textbf{to}\ |B|\ \textbf{do} \cr
/// 10 & \qquad\quad \textbf{if}\ ( y( B[ v ] ) - y( B[ u ] ) )^2 \ge
///     \delta^2\ \textbf{then}\ \textbf{break} \cr
/// 11 & \qquad\quad \mathrm{offer}( B[ u ] , B[ v ] ) \cr
/// 12 & \textbf{function}\ \mathrm{closestPair}( P ) \cr
/// 13 & \quad \delta^2 \gets \infty;\ I \gets \text{the places of } P
///     \text{ sorted by } x \cr
/// 14 & \quad \mathrm{closestRec}( I , 0 , n ) \cr
/// 15 & \quad \textbf{return}\ \text{the pair offered at } \delta^2
///     \text{, and } \delta^2
/// \end{array}}
/// \f]
/// where \f$ \mathrm{offer}( a , b ) \f$ keeps \f$ ( a , b ) \f$ if its square
/// distance is below \f$ \delta^2 \f$.
/// \pre \f$ n \ge 2 \f$, \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$
/// \post the square of the distance of \f$ P[ i ] \f$ and \f$ P[ j ] \f$
inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPair( const std::vector< Point > & P );

/// the pair of `closestPair()` by the sweep on the points sorted by
/// \f$ x \f$, and by the randomized grid when the sweep takes too long; a
/// pair of the least distance, not always the one of `closestPair()`
///
/// In pseudocode (the sweep of Hinrichs, Nievergelt and Schorn and the grid
/// of Golin, Raman, Schwarz and Smid, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sweep}( P , \mathit{axis} , \mathit{budget}
///     ) \cr
/// 2 & \quad K \gets \text{the points sorted by radix along } \mathit{axis}
///     \text{, then along the other axis} \cr
/// 3 & \quad \delta^2 \gets \infty \cr
/// 4 & \quad \textbf{for}\ u \gets 2\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad \textbf{for}\ v \gets u - 1\ \textbf{downto}\ 1\ \textbf{do}
///     \cr
/// 6 & \qquad\quad \textbf{if}\ \mathit{dx}^2 \ge \delta^2\ \textbf{then}
///     \ \textbf{break}\quad \triangleright\ \mathit{dx} , \mathit{dy}
///     \text{ the differences of } K[ u ] \text{ and } K[ v ] \cr
/// 7 & \qquad\quad \textbf{if}\ \mathit{dx}^2 + \mathit{dy}^2 < \delta^2
///     \ \textbf{then}\ \delta^2 \gets \mathit{dx}^2 + \mathit{dy}^2 \cr
/// 8 & \qquad\quad \textbf{else if}\ \mathit{dy}^2 \ge \delta^2 \text{ and }
///     \mathit{dx} = 0\ \textbf{then}\ v \gets \text{the first of the }
///     \text{column of } u \cr
/// 9 & \qquad\quad \mathit{budget} \gets \mathit{budget} - 1;
///     \ \textbf{if}\ \mathit{budget} = 0\ \textbf{then return nil} \cr
/// 10 & \quad \textbf{return}\ \delta^2 \text{ and its pair} \cr
/// 11 & \textbf{function}\ \mathrm{closestPairSweep}( P ) \cr
/// 12 & \quad r \gets \mathrm{sweep}( P , x , 16 n ) \cr
/// 13 & \quad \textbf{if}\ r = \textbf{nil}\ \textbf{then}\ r \gets
///     \mathrm{sweep}( P , y , 16 n ) \cr
/// 14 & \quad \textbf{if}\ r = \textbf{nil}\ \textbf{then return}
///     \ \mathrm{closestGrid}( P ) \cr
/// 15 & \quad \textbf{return}\ r \cr
/// 16 & \textbf{function}\ \mathrm{closestGrid}( P ) \cr
/// 17 & \quad \text{shuffle } P;\ \delta^2 \gets \text{the square }
///     \text{distance of } P[ 1 ] \text{ and } P[ 2 ] \cr
/// 18 & \quad G \gets \text{a grid of side } \lceil \delta \rceil
///     \text{ hashed by the cells, with } P[ 1 ] \text{ and } P[ 2 ] \cr
/// 19 & \quad \textbf{for}\ i \gets 3\ \textbf{to}\ n\ \textbf{do} \cr
/// 20 & \qquad \text{offer the pairs of } P[ i ] \text{ with the points of }
///     \text{the 9 cells around its own} \cr
/// 21 & \qquad \textbf{if}\ \delta^2 = 0\ \textbf{then}\ \textbf{break} \cr
/// 22 & \qquad \textbf{if}\ \delta \text{ fell}\ \textbf{then}\ G \gets
///     \text{a grid of side } \lceil \delta \rceil \text{ with } P[ 1 .. i
///     ] \cr
/// 23 & \qquad \textbf{else}\ \text{add } P[ i ] \text{ to } G \cr
/// 24 & \quad \textbf{return}\ \text{the pair offered at } \delta^2
///     \text{, and } \delta^2
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 2 \f$, \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$
/// \post the square of the distance of \f$ P[ i ] \f$ and \f$ P[ j ] \f$
inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPairSweep( const std::vector< Point > & P );

/// the pair of `closestPair()`, the first pair of the least distance, by
/// all the pairs; \f$ \Theta( n^2 ) \f$
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{closestPairBrute}( P ) \cr
/// 2 & \quad \delta^2 \gets \infty \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ j \gets i + 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad\quad e \gets \mathrm{squaredDistance}( P[ i ] , P[ j ] ) \cr
/// 6 & \qquad\quad \textbf{if}\ e < \delta^2\ \textbf{then}\ \delta^2 \gets
///     e;\ ( a , b ) \gets ( i , j ) \cr
/// 7 & \quad \textbf{return}\ ( ( a , b ) , \delta^2 )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 2 \f$, \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$
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
