/*--------------------------------------------------------------------------*/
/*-------------------- File split_strongcomponents.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of Visit.h: StrongComponents.h, included alone, gives the
 * strongly connected components by one visit and the visits of the book, at
 * the level 3 of the contracts.
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

#include "ADS/graph/StrongComponents.h"

#include <utility>
#include <vector>

// the visits of the book come with the components by one visit
#if ! defined( __ADS_Visit )
 #error "StrongComponents.h does not include Visit.h"
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

// the strongly connected components by one visit and those of the book
// with StrongComponents.h alone: the cycle 0, 1, 2, the edge to the
// cycle 3, 4 and the node 5 alone give three components and one more,
// numbered alike by scc(), sccTarjan() and sccPearce(), in a topological
// order of their graph
ADS_TEST( "strongcomponents-alone" )
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
/*------------------ End File split_strongcomponents.cpp -------------------*/
/*--------------------------------------------------------------------------*/
