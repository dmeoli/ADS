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
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see R. L. Graham, An efficient algorithm for determining the convex
 *      hull of a finite planar set, Information Processing Letters 1(4),
 *      132-133, 1972, doi:10.1016/0020-0190(72)90045-2
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
#include <cstddef>
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
/// the line from a to b, 0 if on it
inline long cross( const Point & a , const Point & b , const Point & c )
{
 return( ( b.x - a.x ) * ( c.y - a.y ) - ( b.y - a.y ) * ( c.x - a.x ) );
 }

/// stessaparte(): p and q on the same side of the line through p1 and p2,
/// or on it
inline bool sameSide( const Point & p1 , const Point & p2 , const Point & p ,
                      const Point & q )
{
 const long a = cross( p1 , p2 , p ) , b = cross( p1 , p2 , q );
 return( ( a >= 0 && b >= 0 ) || ( a <= 0 && b <= 0 ) );
 }

/// an increasing function of the angle of ( dx , dy ) in [ 0 , 4 ), with no
/// arctangent: dy / ( |dx| + |dy| ) in the first quadrant, then 2 minus it,
/// then 2 minus it, then 4 plus it
inline double pseudoAngle( long dx , long dy )
{
 if( ( dx == 0 ) && ( dy == 0 ) )
  return( 0 );
 const double t = double( dy ) / double( std::labs( dx ) + std::labs( dy ) );
 if( dx < 0 )
  return( 2 - t );
 if( dy < 0 )
  return( 4 + t );
 return( t );
 }

/// the hull by checking every segment, O( n^3 ); the vertices in counter
/// clockwise order from the lowest one
inline std::vector< Point > convexHullBrute( std::vector< Point > P )
{
 std::sort( P.begin() , P.end() , []( const Point & a , const Point & b ) {
  return( a.x < b.x || ( a.x == b.x && a.y < b.y ) ); } );
 P.erase( std::unique( P.begin() , P.end() ) , P.end() );
 const std::size_t n = P.size();
 if( n < 3 )
  return( P );
 std::vector< bool > vertex( n , false );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j ) {
   if( i == j ) continue;
   bool edge = true;                       // all the others to the left
   for( std::size_t k = 0 ; k < n && edge ; ++k ) {
    const long c = cross( P[ i ] , P[ j ] , P[ k ] );
    if( c < 0 ) edge = false;
    // a point on the segment, strictly inside, is not a vertex
    if( ( c == 0 ) && ( k != i ) && ( k != j ) &&
        ( ( P[ k ].x - P[ i ].x ) * ( P[ k ].x - P[ j ].x ) +
          ( P[ k ].y - P[ i ].y ) * ( P[ k ].y - P[ j ].y ) > 0 ) )
     edge = false;
    }
   if( edge ) vertex[ i ] = vertex[ j ] = true;
   }
 std::vector< Point > H;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( vertex[ i ] ) H.push_back( P[ i ] );
 // counter clockwise around the lowest one
 auto low = std::min_element( H.begin() , H.end() , []( auto & a , auto & b ) {
  return( a.y < b.y || ( a.y == b.y && a.x > b.x ) ); } );
 std::iter_swap( H.begin() , low );
 const Point o = H[ 0 ];
 std::sort( H.begin() + 1 , H.end() , [ & ]( const Point & a ,
                                            const Point & b ) {
  return( cross( o , a , b ) > 0 ); } );
 return( H );
 }

/// graham(): the vertices of the hull in counter clockwise order from the
/// lowest point, the rightmost of the lowest ones
inline std::vector< Point > graham( std::vector< Point > P )
{
 std::sort( P.begin() , P.end() , []( const Point & a , const Point & b ) {
  return( a.x < b.x || ( a.x == b.x && a.y < b.y ) ); } );
 P.erase( std::unique( P.begin() , P.end() ) , P.end() );
 if( P.size() < 3 )
  return( P );
 const std::size_t min = std::min_element( P.begin() , P.end() ,
                          []( const Point & a , const Point & b ) {
  return( a.y < b.y || ( a.y == b.y && a.x > b.x ) ); } ) - P.begin();
 std::swap( P[ 0 ] , P[ min ] );
 const Point o = P[ 0 ];
 auto dist = [ & ]( const Point & a ) {
  return( ( a.x - o.x ) * ( a.x - o.x ) + ( a.y - o.y ) * ( a.y - o.y ) ); };
 std::sort( P.begin() + 1 , P.end() , [ & ]( const Point & a ,
                                            const Point & b ) {
  const long c = cross( o , a , b );
  return( c > 0 || ( c == 0 && dist( a ) < dist( b ) ) ); } );
 // of the collinear ones with o only the farthest
 std::vector< Point > Q = { o };
 for( std::size_t i = 1 ; i < P.size() ; ++i ) {
  if( ( i + 1 < P.size() ) && ( cross( o , P[ i ] , P[ i + 1 ] ) == 0 ) )
   continue;
  Q.push_back( P[ i ] );
  }
 std::vector< Point > S;                     // the stack of the hull
 for( const Point & p : Q ) {
  while( ( S.size() >= 2 ) &&
         ( cross( S[ S.size() - 2 ] , S.back() , p ) <= 0 ) )
   S.pop_back();                             // the backtrack
  S.push_back( p );
  }
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ConvexHull.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ConvexHull.h --------------------------*/
/*--------------------------------------------------------------------------*/
