/*--------------------------------------------------------------------------*/
/*------------------- File split_queuecustomization.cpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The split of PriorityQueue.h: QueueCustomization.h, included alone, gives
 * the customization points of the queues without the concepts of the book,
 * at the level 3 of the contracts.
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

#include "ADS/priority/QueueCustomization.h"

#include <cstddef>
#include <functional>
#include <queue>
#include <type_traits>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

// the customization points stand alone, without the concepts of the book
#if defined( __ADS_PriorityQueue )
 #error "QueueCustomization.h includes PriorityQueue.h"
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

// the customization points with QueueCustomization.h alone, on a queue of
// the book with handles and on a std::priority_queue
ADS_TEST( "queuecustomization-alone" )
{
 using Std =
   std::priority_queue< int , std::vector< int > , std::greater< int > >;
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
/*----------------- End File split_queuecustomization.cpp ------------------*/
/*--------------------------------------------------------------------------*/
