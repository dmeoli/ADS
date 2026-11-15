/*--------------------------------------------------------------------------*/
/*------------------------- File split_maxflow.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of MaxFlow.h: the header of the book, included alone, still
 * gives the engineered push-relabel of PushRelabel.h, by every way of
 * FlowNetwork::maxFlow(), at the level 3 of the contracts.
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

#include "ADS/graph/MaxFlow.h"

#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

// the push-relabel, split off MaxFlow.h, comes with it
#if ! defined( __ADS_PushRelabel )
 #error "MaxFlow.h does not include PushRelabel.h"
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

// the network of Figure 14.12 by every way of maxFlow(), the push-relabel
// of PushRelabel.h among them, with the header of the book alone: the
// value 5, the same minimum cut, and the same repair after a capacity cut
// below its flow
ADS_TEST( "maxflow-old-include" )
{
 for( Augment how : { Augment::FORD_FULKERSON , Augment::EDMONDS_KARP ,
		      Augment::DINIC , Augment::PUSH_RELABEL } ) {
  FlowNetwork N( 4 );
  N.addEdge( 0 , 1 , 4 );
  N.addEdge( 0 , 2 , 2 );
  const int e = N.addEdge( 1 , 2 , 3 );
  N.addEdge( 1 , 3 , 1 );
  N.addEdge( 2 , 3 , 4 );
  CHECK( N.maxFlow( 0 , 3 , how ) == 5 );
  const std::vector< bool > S = N.minCut( 0 );
  CHECK( S[ 0 ] && S[ 1 ] && S[ 2 ] && ! S[ 3 ] );
  N.setCapacity( e , 1 );
  CHECK( N.maxFlow( 0 , 3 , how ) == -1 );
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
/*----------------------- End File split_maxflow.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
