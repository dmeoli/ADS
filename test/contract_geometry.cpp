/*--------------------------------------------------------------------------*/
/*----------------------- File contract_geometry.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the convex hull: each operation
 * against its specification and the violations of its preconditions, which
 * must throw ADS::contract_violation. The tests are compiled at the level 3,
 * so that each operation also checks its postconditions and audits, on random
 * instances against a brute force or a second algorithm.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/geometry/ClosestPair.h"
#include "ADS/geometry/ConvexHull.h"

#include <algorithm>
#include <climits>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the check of the hulls refuses a polygon that is not convex or leaves a
// point out, and both hulls pass it on random points, collinear ones too;
// the coordinates just inside and outside [ -10^9 , 10^9 ], no point, one
// point twice; pseudoAngle() on the axes and at the extremes of long
ADS_TEST( "hull" )
{
 const std::vector< Point > P = { { 0 , 0 } , { 4 , 0 } , { 4 , 4 } ,
                                  { 0 , 4 } , { 2 , 2 } };
 CHECK( detail::isHullOf( { { 0 , 0 } , { 4 , 0 } , { 4 , 4 } , { 0 , 4 } } ,
                          P ) );
 CHECK( ! detail::isHullOf( { { 0 , 0 } , { 4 , 0 } , { 4 , 4 } } , P ) );
 CHECK( ! detail::isHullOf( { { 0 , 0 } , { 4 , 4 } , { 4 , 0 } } , P ) );
 CHECK( ! detail::isHullOf( { { 0 , 0 } , { 4 , 0 } , { 2 , 2 } ,
                              { 4 , 4 } , { 0 , 4 } } , P ) );
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< Point > Q( g() % 12 );
  const bool line = t % 5 == 0;
  for( auto & q : Q ) {
   q.x = long( g() % 10 );
   q.y = line ? 2 * q.x : long( g() % 10 );
   }
  const auto H = graham( Q ) , B = convexHullBrute( Q );
  CHECK( H.size() == B.size() );
  const long dx = long( g() % 21 ) - 10 , dy = long( g() % 21 ) - 10;
  const double a = pseudoAngle( dx , dy );
  CHECK( a >= 0 && a < 4 );
  }
 // the coordinates just inside and just outside [ -10^9 , 10^9 ]
 const long M = 1000000000L;
 CHECK( graham( { { M , M } , { -M , M } , { -M , -M } , { M , -M } } )
        .size() == 4 );
 CHECK( convexHullBrute( { { -M , -M } , { M , -M } , { 0 , M } } )
        .size() == 3 );
 CHECK_THROWS( graham( { { 0 , 0 } , { M + 1 , 0 } } ) , contract_violation );
 CHECK_THROWS( graham( { { 0 , -M - 1 } } ) , contract_violation );
 CHECK_THROWS( convexHullBrute( { { -M - 1 , 0 } } ) , contract_violation );
 CHECK_THROWS( convexHullBrute( { { 0 , M + 1 } } ) , contract_violation );
 CHECK( graham( {} ).empty() && convexHullBrute( {} ).empty() );
 CHECK( graham( { { 3 , 3 } , { 3 , 3 } } ).size() == 1 );
 // the extreme values of long: in [ 0 , 4 ), no overflow, the quadrants
 CHECK( pseudoAngle( 0 , 0 ) == 0 && pseudoAngle( LONG_MAX , 0 ) == 0 );
 CHECK( pseudoAngle( 0 , LONG_MAX ) == 1 && pseudoAngle( LONG_MIN , 0 ) == 2 );
 CHECK( pseudoAngle( 0 , LONG_MIN ) == 3 );
 const double far = pseudoAngle( LONG_MAX , -1 );     // 4 - 1e-19
 CHECK( far < 4 && far > 3.9 && far >= pseudoAngle( 1 , -1 ) );
 const double low = pseudoAngle( LONG_MIN , LONG_MIN );
 CHECK( low > 2.4 && low < 2.6 );
 }

// closest pair: n >= 2 and the coordinates in [ -10^9 , 10^9 ] just inside
// and just outside, the squared distance of the extreme corners (8 10^18,
// it fits an int64), coincident points at distance 0
ADS_TEST( "closest-pair" )
{
 const long M = 1000000000L;
 CHECK_THROWS( closestPair( {} ) , contract_violation );
 CHECK_THROWS( closestPair( { { 0 , 0 } } ) , contract_violation );
 CHECK_THROWS( closestPairBrute( {} ) , contract_violation );
 CHECK_THROWS( closestPairBrute( { { 1 , 1 } } ) , contract_violation );
 const std::vector< Point > two = { { -M , -M } , { M , M } };
 CHECK( closestPair( two ).second == 8000000000000000000LL );
 CHECK( closestPairBrute( two ) == closestPair( two ) );
 for( const Point & p : { Point{ M + 1 , 0 } , Point{ -M - 1 , 0 } ,
                          Point{ 0 , M + 1 } , Point{ 0 , -M - 1 } } ) {
  CHECK_THROWS( closestPair( { { 0 , 0 } , p } ) , contract_violation );
  CHECK_THROWS( closestPairBrute( { p , { 0 , 0 } } ) , contract_violation );
  }
 const std::vector< Point > same = { { 5 , 5 } , { 0 , 0 } , { 5 , 5 } };
 CHECK( closestPair( same ).second == 0 &&
        closestPair( same ).first.first == 0 &&
        closestPair( same ).first.second == 2 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File contract_geometry.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
