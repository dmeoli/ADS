/*--------------------------------------------------------------------------*/
/*---------------------------- File geometry.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the geometry of the plane on n points of integer
 * coordinates in [ -10^9 , 10^9 ]:
 *
 * - "hull", the convex hull of points uniform in a square (case square),
 *   in a disk (case disk) and near a circle, most of them vertices (case
 *   circle): graham() and convexHull() of ADS against, when found,
 *   boost::geometry::convex_hull on a multi_point of points of 64-bit
 *   integers, and CGAL::convex_hull_2 (the algorithm of Akl and Toussaint)
 *   and CGAL::ch_graham_andrew on the kernel with exact predicates
 *   (ADS_HAS_CGAL);
 * - "closest", the closest pair of points uniform in a square, of points
 *   on 16 vertical lines (case lines, many equal x) and of points in a
 *   strip 2000 wide and 2 10^9 high (case strip, where the sweep meets
 *   too many points and falls back): closestPair() of the book, by divide
 *   et impera, and closestPairSweep().
 *
 * Each cell is the median of r runs, in nanoseconds per point; the
 * conversion to the types of the libraries is not timed, nor is any copy of
 * the input; the contracts are off (ADS_CONTRACTS=0).
 *
 *   geometry [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/geometry/ClosestPair.h"
#include "ADS/geometry/ConvexHull.h"

#if __has_include( <boost/geometry.hpp> )
 #include <boost/geometry.hpp>
 #include <boost/geometry/geometries/multi_point.hpp>
 #include <boost/geometry/geometries/point_xy.hpp>
 #include <boost/geometry/geometries/polygon.hpp>
 #define ADS_HAS_BOOST 1
#endif

#ifdef ADS_HAS_CGAL
 #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
 #include <CGAL/ch_graham_andrew.h>
 #include <CGAL/convex_hull_2.h>
#endif

#include <cmath>
#include <iterator>

using namespace ADS::bench;
using ADS::Point;

namespace {

/// n points of the case c
std::vector< Point > points( int c , std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 const double R = 1e9;
 std::uniform_real_distribution< double > U( -R , R );
 std::vector< Point > P( n );
 for( auto & p : P ) {
  double x , y;
  switch( c ) {
   case( 0 ): x = U( g ); y = U( g ); break;             // square
   case( 1 ):                                            // disk
    do { x = U( g ); y = U( g ); } while( x * x + y * y > R * R );
    break;
   case( 2 ): {                                          // circle
    const double a = U( g ) * 3.14159265358979323846 / R;
    x = R * std::cos( a );
    y = R * std::sin( a );
    break;
    }
   case( 3 ):                                            // 16 lines
    x = std::floor( ( U( g ) + R ) / ( 2 * R ) * 16 ) * ( R / 8 ) - R;
    y = U( g );
    break;
   default:                                              // a strip
    x = U( g ) / 1e6;
    y = U( g );
   }
  p = Point{ long( std::llround( x ) ) , long( std::llround( y ) ) };
  }
 return( P );
 }

/// a cell of the benchmark b
template< class F >
void cell( const char * b , const char * what , std::size_t n ,
           const std::string & who , int r , F && f )
{
 print( b , what , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { sink = sink + std::uint64_t( f() ); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  const char * hulls[] = { "square" , "disk" , "circle" };
  for( int c = 0 ; c < 3 ; ++c ) {
   const auto P = points( c , n , o.seed );
   const char * what = hulls[ c ];
#ifdef ADS_HAS_BOOST
   namespace bg = boost::geometry;
   using BP = bg::model::d2::point_xy< long long >;
   bg::model::multi_point< BP > M;
   for( const Point & p : P ) M.push_back( BP( p.x , p.y ) );
   for( const char * who : { "boost::geometry::convex_hull" ,
                             "boost::geometry::convex_hull A/A" } )
    cell( "hull" , what , n , who , r , [ & ]() {
     bg::model::polygon< BP > H;
     bg::convex_hull( M , H );
     return( H.outer().size() ); } );
#endif
#ifdef ADS_HAS_CGAL
   using K = CGAL::Exact_predicates_inexact_constructions_kernel;
   std::vector< K::Point_2 > C;
   for( const Point & p : P ) C.emplace_back( double( p.x ) , double( p.y ) );
   cell( "hull" , what , n , "CGAL::convex_hull_2" , r , [ & ]() {
    std::vector< K::Point_2 > H;
    CGAL::convex_hull_2( C.begin() , C.end() , std::back_inserter( H ) );
    return( H.size() ); } );
   cell( "hull" , what , n , "CGAL::ch_graham_andrew" , r , [ & ]() {
    std::vector< K::Point_2 > H;
    CGAL::ch_graham_andrew( C.begin() , C.end() , std::back_inserter( H ) );
    return( H.size() ); } );
#endif
   cell( "hull" , what , n , "ADS::graham" , r , [ & ]() {
    return( ADS::graham( P ).size() ); } );
#ifndef ADS_BENCH_OLD
   cell( "hull" , what , n , "ADS::convexHull" , r , [ & ]() {
    return( ADS::convexHull( P ).size() ); } );
#endif
   }
  for( int c : { 0 , 3 , 4 } ) {
   const auto P = points( c , n , o.seed );
   const char * what = c == 0 ? "square" : c == 3 ? "lines" : "strip";
   for( const char * who : { "ADS::closestPair" , "ADS::closestPair A/A" } )
    cell( "closest" , what , n , who , r , [ & ]() {
     return( ADS::closestPair( P ).second ); } );
#ifndef ADS_BENCH_OLD
   cell( "closest" , what , n , "ADS::closestPairSweep" , r , [ & ]() {
    return( ADS::closestPairSweep( P ).second ); } );
#endif
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File geometry.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
