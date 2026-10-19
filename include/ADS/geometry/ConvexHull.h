/*--------------------------------------------------------------------------*/
/*--------------------------- File ConvexHull.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The convex hull of a set of points of the plane (section 16.1), the
 * smallest convex polygon that contains them, as its vertices in counter
 * clockwise order. The side of a point q with respect to the line through
 * p_1 and p_2 is the sign of the cross product
 * \f$ ( x_2 - x_1 ) ( y_q - y_1 ) - ( y_2 - y_1 ) ( x_q - x_1 ) \f$, and
 * sameSide() of the book is the product of two such signs not being
 * negative; with integer coordinates it is exact.
 *
 * - convexHullBrute(): a segment S_ij is an edge of the hull if all the
 *   other points are on one side of its line, O( n^3 );
 * - graham(): the lowest point (the rightmost of the lowest ones) is a
 *   vertex; the others by increasing angle around it, of the collinear
 *   ones only the farthest kept, and each one pushed on a stack after
 *   popping the vertices that it makes not convex, a backtrack; each point
 *   is pushed and popped at most once, O( n ) after the sort,
 *   O( n log n ) in all. The angle is compared by the cross product, or by
 *   pseudoAngle(), a quantity of the same order as the angle that needs no
 *   arctangent (exercise 16.1).
 *
 * - convexHull(), beyond the book, the engineered hull: the extreme
 *   points in the eight directions of the axes and of the diagonals (the
 *   minimum and the maximum of x, y, x + y and x - y) are vertices of the
 *   hull, in counter clockwise order, and a point strictly inside their
 *   octagon (on the left of each of its edges, 8 cross products at most)
 *   is no vertex: one pass drops it (the heuristic of Akl and Toussaint).
 *   The k points left, each made a key of 64 bits
 *   \f$ ( x + 2^{31} ) 2^{32} + ( y + 2^{31} ) \f$, whose order is the one
 *   by x and then y, are sorted by the radix sort of ADS [see Sort.h] in
 *   O( k ) (at most 8 passes of bytes), and the lower and the upper chain
 *   of the monotone chain of Andrew pop the points that make a turn not
 *   to the left, each point pushed and popped at most once, O( k ): in
 *   all \f$ O( n + k ) \f$, \f$ k \le n \f$; for points uniform in a
 *   square or in a disk the octagon leaves a vanishing fraction of them,
 *   and the expected time is O( n ) (Devroye and Toussaint). The vertices
 *   come in the order and from the point of graham().
 *
 * The contracts [see Contract.h]: the pseudo-angle is in [ 0 , 4 ), and the
 * hulls are convex, counter clockwise, made of input points and contain all
 * of them (checked at level 3). The operations are defined in
 * ConvexHull.tpp.
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
 *         Universita' di Pisa \n
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

#include <algorithm>
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
 long x , y;
 bool operator==( const Point & ) const = default;
 };

/// the cross product ( b - a ) x ( c - a ): positive if c is to the left of
/// the line from a to b, 0 if on it; in long long, as long may have 32 bits
inline long long cross( const Point & a , const Point & b ,
                        const Point & c );

/// stessaparte(): p and q on the same side of the line through p1 and p2,
/// or on it
inline bool sameSide( const Point & p1 , const Point & p2 , const Point & p ,
                      const Point & q );

/// an increasing function of the angle of ( dx , dy ) in [ 0 , 4 ), with no
/// arctangent: dy / ( |dx| + |dy| ) in the first quadrant, then 2 minus it,
/// then 2 minus it, then 4 plus it
/// \post a value in [ 0 , 4 ), also for the extreme values of long (a
///       direction just below the positive x axis may round to the largest
///       double below 4, tying with the ones next to it)
inline double pseudoAngle( long dx , long dy );

/// the hull by checking every segment, O( n^3 ); the vertices in counter
/// clockwise order from the lowest one
/// \pre | x | and | y | at most 10^9, so that the cross products fit a long
///      long
/// \post the hull of P
inline std::vector< Point > convexHullBrute( std::vector< Point > P );

/// graham(): the vertices of the hull in counter clockwise order from the
/// lowest point, the rightmost of the lowest ones
/// \pre | x | and | y | at most 10^9, so that the cross products fit a long
///      long
/// \post the hull of P
inline std::vector< Point > graham( std::vector< Point > P );

/// the hull by the octagon of Akl and Toussaint, the radix sort and the
/// monotone chain of Andrew; the vertices as graham() gives them
/// \pre | x | and | y | at most 10^9
/// \post the hull of P
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
