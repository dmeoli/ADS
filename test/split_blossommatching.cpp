/*--------------------------------------------------------------------------*/
/*--------------------- File split_blossommatching.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of Matching.h: BlossomMatching.h, included alone, gives the
 * matchings of the blossoms and those of the book, at the level 3 of the
 * contracts.
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

#include "ADS/graph/BlossomMatching.h"

#include "ADS/graph/ListGraph.h"

#include <cstddef>
#include <tuple>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

// the matchings of the book come with the blossoms
#if ! defined( __ADS_Matching )
 #error "BlossomMatching.h does not include Matching.h"
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

// the matchings of the blossoms and those of the book with
// BlossomMatching.h alone: the cycle of 5 nodes, whose largest matching
// has 2 edges, the weights of the example of section 14.11.2, on which the
// blossoms and the branch and bound agree, and a b-matching
ADS_TEST( "blossommatching-alone" )
{
 ListGraph<> g( 5 , false );
 for( int u = 0 ; u < 5 ; ++u )
  g.insertEdge( u , ( u + 1 ) % 5 );
 const std::vector< int > mate = maxCardinalityMatching( g );
 int matched = 0;
 for( int u = 0 ; u < 5 ; ++u )
  if( mate[ u ] != NIL ) {
   CHECK( mate[ mate[ u ] ] == u );
   ++matched;
   }
 CHECK( matched == 4 );
 const std::vector< std::tuple< int , int , int > > E = {
  { 0 , 1 , 4 } , { 1 , 2 , 5 } , { 2 , 3 , 4 } , { 0 , 3 , 1 }
  };
 const auto [ w , K ] = maxWeightMatching( 4 , E );
 CHECK( ( w == 8 ) && ( K == std::vector< std::size_t >{ 0 , 2 } ) );
 CHECK( branchAndBoundMatching( 4 , E ).first == w );
 const auto M =
   bipartiteBMatching( { { 0 , 1 } , { 0 } } , 2 , { 2 , 1 } , { 1 , 1 } );
 CHECK( M.size() == 2 );
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
/*------------------- End File split_blossommatching.cpp -------------------*/
/*--------------------------------------------------------------------------*/
