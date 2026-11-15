/*--------------------------------------------------------------------------*/
/*---------------------- File split_priorityqueue.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of PriorityQueue.h: the header of the concepts of the book,
 * included alone, still gives the customization points of
 * QueueCustomization.h, through which the concepts read a queue, at the
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

#include "ADS/priority/PriorityQueue.h"

#include <cstddef>
#include <functional>
#include <queue>
#include <type_traits>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

// the customization points, split off PriorityQueue.h, come with it
#if ! defined( __ADS_QueueCustomization )
 #error "PriorityQueue.h does not include QueueCustomization.h"
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a queue of the book with handles, its items kept in a vector
struct BookQueue
{
 using Item = int;
 using Priority = int;
 using Handle = int;
 std::vector< int > x , p;
 [[nodiscard]] bool empty( void ) const { return( x.empty() ); }
 [[nodiscard]] int min( void ) const { return( x[ least() ] ); }
 int deleteMin( void ) {
  const std::size_t i = least();
  const int y = x[ i ];
  x.erase( x.begin() + long( i ) );
  p.erase( p.begin() + long( i ) );
  return( y );
  }
 Handle insert( const int & y , const int & q ) {
  x.push_back( y );
  p.push_back( q );
  return( int( x.size() ) - 1 );
  }
 void decrease( Handle h , const int & q ) { p[ std::size_t( h ) ] = q; }
 [[nodiscard]] std::size_t least( void ) const {
  std::size_t i = 0;
  for( std::size_t j = 1 ; j < p.size() ; ++j )
   if( p[ j ] < p[ i ] )
    i = j;
  return( i );
  }
 };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the concepts of the book and the customization points with the header
// of the book alone: a queue of the book with handles is addressable, a
// std::priority_queue only a PriorityQueue, and the points read both
ADS_TEST( "priorityqueue-old-include" )
{
 static_assert( AddressablePriorityQueue< BookQueue > );
 using Std =
   std::priority_queue< int , std::vector< int > , std::greater< int > >;
 static_assert( PriorityQueue< Std > && ! AddressablePriorityQueue< Std > );
 static_assert( std::is_same_v< queue_item_t< Std > , int > );
 static_assert( std::is_same_v< queue_handle_t< BookQueue > , int > );
 BookQueue B;
 Std S;
 for( int y : { 5 , 3 , 8 } ) {
  const int h = queue_insert( B , y , y );
  if( y == 8 )
   queue_decrease( B , h , 1 );
  queue_insert( S , y );
  }
 CHECK( ( queue_min( B ) == 8 ) && ( queue_deleteMin( B ) == 8 ) );
 CHECK( ( queue_min( S ) == 3 ) && ( queue_deleteMin( S ) == 3 ) );
 CHECK( ( queue_deleteMin( B ) == 3 ) && ( queue_deleteMin( S ) == 5 ) );
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
/*-------------------- End File split_priorityqueue.cpp --------------------*/
/*--------------------------------------------------------------------------*/
