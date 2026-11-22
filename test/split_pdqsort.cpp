/*--------------------------------------------------------------------------*/
/*------------------------- File split_pdqsort.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of EngineeredQuickSorts.h: PdqSort.h, included alone, gives the
 * pattern-defeating quicksort and the quicksorts of Ferragina's book, at the
 * level 3 of the contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/sort/PdqSort.h"

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

// the quicksorts of the book come with the quicksort of Peters
#if ! defined( __ADS_EngineeredQuickSorts )
 #error "PdqSort.h does not include EngineeredQuickSorts.h"
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the three quicksorts with PdqSort.h alone, on a vector of
// random keys with repetitions, in increasing and in decreasing order,
// against std::sort
ADS_TEST( "pdqsort-alone" )
{
 std::mt19937 gen( 7 );
 std::vector< int > V( 3000 );
 for( int & x : V )
  x = int( gen() % 500 );
 for( int k = 0 ; k < 2 ; ++k ) {
  std::vector< int > R = V , A = V , B = V , C = V;
  if( k == 0 ) {
   std::sort( R.begin() , R.end() );
   pdqSort( A );
   dualPivotQuickSort( B );
   threeWayQuickSort( C );
   }
  else {
   std::sort( R.begin() , R.end() , std::greater<>() );
   pdqSort( A , std::greater<>() );
   dualPivotQuickSort( B , std::greater<>() );
   threeWayQuickSort( C , std::greater<>() );
   }
  CHECK( ( A == R ) && ( B == R ) && ( C == R ) );
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
/*----------------------- End File split_pdqsort.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
