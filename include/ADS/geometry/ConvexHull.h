/*--------------------------------------------------------------------------*/
/*--------------------------- File ConvexHull.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The convex hull of a set \f$ P \f$ of \f$ n \f$ points of the plane
 * (section 16.1), the smallest convex polygon that contains them, as its
 * vertices in counterclockwise order. The side of a point \f$ q \f$ with
 * respect to the line through \f$ p_1 \f$ and \f$ p_2 \f$ is the sign of the
 * cross product
 * \f$ ( x_2 - x_1 ) ( y_q - y_1 ) - ( y_2 - y_1 ) ( x_q - x_1 ) \f$, and
 * `sameSide()`, the stessaparte() of the book, is the product of two such
 * signs not being negative; with integer coordinates it is exact.
 *
 * - `convexHullBrute()`: a segment \f$ p_i p_j \f$ is an edge of the hull if
 *   all the other points are on one side of its line, \f$ O( n^3 ) \f$;
 * - `graham()`: the lowest point (the rightmost of the lowest ones) is a
 *   vertex; the others by increasing angle around it, of the collinear ones
 *   only the farthest kept, and each one pushed on a stack after popping the
 *   vertices that it makes not convex, a backtrack; each point is pushed and
 *   popped at most once, \f$ O( n ) \f$ after the sort,
 *   \f$ O( n \log n ) \f$ in all. The angle is compared by the cross
 *   product, or by `pseudoAngle()`, a quantity of the same order as the
 *   angle that needs no arctangent (exercise 16.1);
 * - `convexHull()`, beyond the book, the engineered hull: the extreme points
 *   in the eight directions of the axes and of the diagonals (the minimum and
 *   the maximum of \f$ x \f$, \f$ y \f$, \f$ x + y \f$ and \f$ x - y \f$) are
 *   vertices of the hull, in counterclockwise order, and a point strictly
 *   inside their octagon (on the left of each of its edges, 8 cross products
 *   at most) is no vertex: one pass drops it (the heuristic of Akl and
 *   Toussaint). The \f$ k \f$ points left, each made a key of 64 bits
 *   \f$ ( x + 2^{31} ) 2^{32} + ( y + 2^{31} ) \f$, whose order is the one
 *   by \f$ x \f$ and then \f$ y \f$, are sorted by the radix sort of ADS [see
 *   `Sort.h`] in \f$ O( k ) \f$ (at most 8 passes of bytes), and the lower
 *   and the upper chain of the monotone chain of Andrew pop the points that
 *   make a turn not to the left, each point pushed and popped at most once,
 *   \f$ O( k ) \f$: in all \f$ O( n + k ) \f$, \f$ k \le n \f$; for points
 *   uniform in a square or in a disk the octagon leaves a vanishing fraction
 *   of them, and the expected time is \f$ O( n ) \f$ (Devroye and
 *   Toussaint). The vertices come in the order and from the point of
 *   `graham()`.
 *
 * The contracts [see `Contract.h`]: the pseudo-angle is in \f$ [ 0 , 4 ) \f$,
 * and the hulls are convex, counterclockwise, made of input points and
 * contain all of them (checked at level 3). The operations are defined in
 * `ConvexHull.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see R. L. Graham, An efficient algorithm for determining the convex
 *      hull of a finite planar set, Information Processing Letters 1(4),
 *      132-133, 1972, doi:10.1016/0020-0190(72)90045-2
 * \see S. G. Akl, G. T. Toussaint, A fast convex hull algorithm,
 *      Information Processing Letters 7(5), 219-222, 1978,
 *      doi:10.1016/0020-0190(78)90003-0
 * \see A. M. Andrew, Another efficient algorithm for convex hulls in two
 *      dimensions, Information Processing Letters 9(5), 216-219, 1979,
 *      doi:10.1016/0020-0190(79)90072-3
 * \see L. Devroye, G. T. Toussaint, A note on linear expected time
 *      algorithms for finding convex hulls, Computing 26(4), 361-366,
 *      1981, doi:10.1007/BF02237955
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

#ifndef __ADS_ConvexHull
 #define __ADS_ConvexHull
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sort.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a point of the plane, of integer coordinates
struct Point {
 long x ,   ///< the abscissa
      y;    ///< the ordinate
 /// true if the two points have the same coordinates
 bool operator==( const Point & ) const = default;
 };

/// the cross product \f$ ( b - a ) \times ( c - a ) \f$: positive if \f$ c \f$
/// is to the left of the line from \f$ a \f$ to \f$ b \f$, 0 if on it; in
/// `long long`, as `long` may have 32 bits
///
/// It is the determinant
/// \f[
///   ( x_b - x_a ) ( y_c - y_a ) - ( y_b - y_a ) ( x_c - x_a ) ,
/// \f]
/// twice the signed area of the triangle \f$ a b c \f$.
inline long long cross( const Point & a , const Point & b ,
                        const Point & c );

/// true if \f$ p \f$ and \f$ q \f$ are on the same side of the line through
/// \f$ p_1 \f$ and \f$ p_2 \f$, or on it (the stessaparte() of the book)
///
/// In pseudocode (section 16.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sameSide}( p_1 , p_2 , p , q ) \cr
/// 2 & \quad a \gets \mathrm{cross}( p_1 , p_2 , p );\ b \gets
///     \mathrm{cross}( p_1 , p_2 , q ) \cr
/// 3 & \quad \textbf{return}\ ( a \ge 0\ \textbf{and}\ b \ge 0 )
///     \ \textbf{or}\ ( a \le 0\ \textbf{and}\ b \le 0 )
/// \end{array}}
/// \f]
inline bool sameSide( const Point & p1 , const Point & p2 , const Point & p ,
                      const Point & q );

/// an increasing function of the angle of the vector
/// \f$ ( \mathit{dx} , \mathit{dy} ) \f$, in \f$ [ 0 , 4 ) \f$ and with no
/// arctangent (exercise 16.1)
///
/// With \f$ t = \mathit{dy} / ( |\mathit{dx}| + |\mathit{dy}| ) \f$, which
/// grows with the angle in the first and in the fourth quadrant, it is
/// \f[
///   \begin{array}{ll}
///   0 & \mathit{dx} = \mathit{dy} = 0 , \cr
///   t & \mathit{dx} \ge 0 , \mathit{dy} \ge 0 , \cr
///   2 - t & \mathit{dx} < 0 , \cr
///   4 + t & \mathit{dx} \ge 0 , \mathit{dy} < 0 ,
///   \end{array}
/// \f]
/// i.e., 1, 2 and 3 on the three axes after the positive one.
/// \post a value in \f$ [ 0 , 4 ) \f$, also for the extreme values of `long`
///       (a direction just below the positive \f$ x \f$ axis may round to
///       the largest `double` below 4, tying with the ones next to it)
inline double pseudoAngle( long dx , long dy );

/// the hull by checking every segment, \f$ O( n^3 ) \f$; the vertices in
/// counterclockwise order from the lowest one
///
/// In pseudocode (section 16.1), a point strictly inside a segment of the
/// hull being no vertex:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{convexHullBrute}( P ) \cr
/// 2 & \quad \text{remove the repeated points of } P \cr
/// 3 & \quad \textbf{if}\ |P| < 3\ \textbf{then return}\ P \cr
/// 4 & \quad H \gets \emptyset \cr
/// 5 & \quad \textbf{foreach}\ p_i , p_j \in P \text{ with } i \ne j
///     \ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ \mathrm{cross}( p_i , p_j , p_k ) \ge 0
///     \text{ for each } k\ \textbf{and}\ \text{no } p_k \text{ lies }
///     \text{strictly inside } p_i p_j\ \textbf{then} \cr
/// 7 & \qquad\quad H \gets H \cup \{ p_i , p_j \} \cr
/// 8 & \quad \textbf{return}\ H \text{ sorted by angle around its lowest }
///     \text{point}
/// \end{array}}
/// \f]
/// \pre \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$, so that the cross
///      products fit a `long long`
/// \post the hull of \f$ P \f$ (audit)
inline std::vector< Point > convexHullBrute( std::vector< Point > P );

/// the vertices of the hull in counterclockwise order from the lowest point,
/// the rightmost of the lowest ones, by the scan of Graham
///
/// In pseudocode (section 16.1), the stack \f$ S \f$ holding at all times the
/// hull of the points scanned so far:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{graham}( P ) \cr
/// 2 & \quad \text{remove the repeated points of } P;\ \textbf{if}\ |P| < 3
///     \ \textbf{then return}\ P \cr
/// 3 & \quad o \gets \text{the lowest point of } P \text{, the rightmost of }
///     \text{the lowest ones} \cr
/// 4 & \quad \text{sort the other points by increasing angle around } o
///     \text{, the nearest first on a tie} \cr
/// 5 & \quad Q \gets ( o ) \cdot \text{those points, of the ones collinear }
///     \text{with } o \text{ only the farthest} \cr
/// 6 & \quad S \gets \text{an empty stack} \cr
/// 7 & \quad \textbf{foreach}\ p \in Q\ \textbf{do} \cr
/// 8 & \qquad \textbf{while}\ |S| \ge 2\ \textbf{and}\ \mathrm{cross}(
///     \text{the second of } S , \mathrm{top}( S ) , p ) \le 0\ \textbf{do}
///     \cr
/// 9 & \qquad\quad \mathrm{pop}( S )\quad \triangleright\ \text{the }
///     \text{backtrack} \cr
/// 10 & \qquad \mathrm{push}( S , p ) \cr
/// 11 & \quad \textbf{return}\ S \text{ from its bottom}
/// \end{array}}
/// \f]
/// \pre \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$, so that the cross
///      products fit a `long long`
/// \post the hull of \f$ P \f$ (audit)
inline std::vector< Point > graham( std::vector< Point > P );

/// the hull by the octagon of Akl and Toussaint, the radix sort and the
/// monotone chain of Andrew; the vertices as `graham()` gives them
///
/// In pseudocode (the heuristic of Akl and Toussaint and the monotone chain
/// of Andrew, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{convexHull}( P ) \cr
/// 2 & \quad O \gets \text{the distinct points of } P \text{ of largest }
///     ( -y , x - y , x , x + y , y , y - x , -x , -x - y ) \cr
/// 3 & \quad K \gets \text{the keys } ( x , y ) \text{ of the points of } P
///     \text{ not strictly inside } O \cr
/// 4 & \quad \text{sort } K \text{ by radix and remove its repetitions} \cr
/// 5 & \quad \textbf{if}\ |K| < 3\ \textbf{then return}\ K \cr
/// 6 & \quad H \gets \text{an empty stack} \cr
/// 7 & \quad \textbf{foreach}\ p \in K \text{ forward (the lower chain), }
///     \text{then backward (the upper one)}\ \textbf{do} \cr
/// 8 & \qquad \textbf{while}\ H \text{ has two points of this chain}
///     \ \textbf{and}\ \mathrm{cross}( \text{the second of } H , \mathrm{top}(
///     H ) , p ) \le 0\ \textbf{do} \cr
/// 9 & \qquad\quad \mathrm{pop}( H ) \cr
/// 10 & \qquad \mathrm{push}( H , p ) \cr
/// 11 & \quad \text{drop the last point of } H \text{, the first one come }
///     \text{back} \cr
/// 12 & \quad \textbf{return}\ H \text{ rotated to start from its lowest }
///     \text{point, the rightmost of the lowest ones}
/// \end{array}}
/// \f]
/// \pre \f$ |x| \le 10^9 \f$ and \f$ |y| \le 10^9 \f$
/// \post the hull of \f$ P \f$ (audit)
inline std::vector< Point > convexHull( const std::vector< Point > & P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/geometry/impl/ConvexHull.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ConvexHull.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ConvexHull.h --------------------------*/
/*--------------------------------------------------------------------------*/
