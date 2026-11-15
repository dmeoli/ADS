/*--------------------------------------------------------------------------*/
/*-------------------------- File split_visit.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of Visit.h: the header of the book, included alone, still gives
 * the strongly connected components by one visit of StrongComponents.h,
 * beside those of Kosaraju, at the level 3 of the contracts.
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

#include "ADS/graph/Visit.h"

#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

// the components by one visit, split off Visit.h, come with it
#if ! defined( __ADS_StrongComponents )
 #error "Visit.h does not include StrongComponents.h"
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

// the strongly connected components of the book and those by one visit
// with the header of the book alone: the cycle 0, 1, 2, the edge to the
// cycle 3, 4 and the node 5 alone give three components and one more,
// numbered alike by scc(), sccTarjan() and sccPearce(), in a topological
// order of their graph
ADS_TEST( "visit-old-include" )
{
 ListGraph<> g( 6 );
 for( const auto & [ u , v ] :
      std::vector< std::pair< int , int > >{ { 0 , 1 } ,
					    { 1 , 2 } ,
					    { 2 , 0 } ,
					    { 2 , 3 } ,
					    { 3 , 4 } ,
					    { 4 , 3 } ,
					    { 4 , 5 } } )
  g.insertEdge( u , v );
 const std::vector< int > C = scc( g );
 CHECK( ( C[ 0 ] == C[ 1 ] ) && ( C[ 1 ] == C[ 2 ] ) && ( C[ 3 ] == C[ 4 ] ) );
 CHECK( ( C[ 0 ] < C[ 3 ] ) && ( C[ 3 ] < C[ 5 ] ) );
 CHECK( sccTarjan( g ) == C );
 CHECK( sccPearce( g ) == C );
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
/*------------------------ End File split_visit.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
