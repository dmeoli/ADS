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

#include "ADS/geometry/ConvexHull.h"

#include <algorithm>
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
// point out, and both hulls pass it on random points, collinear ones too
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
