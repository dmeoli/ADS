/*--------------------------------------------------------------------------*/
/*------------------------ File ListPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a list.
 *
 * ListPriorityQueue is the lists of section 10.1.1: sorted, min() and
 * deleteMin() in O( 1 ), insert() and decrease() in O( n ); unsorted, the
 * other way round. The handle is the position of the cell.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ListPriorityQueue
 #define __ADS_ListPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ListPriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a list, sorted by priority or not
template< class T , std::totally_ordered P = int , bool Sorted = true >
class ListPriorityQueue {

 using Pair = std::pair< P , T >;

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;                                ///< the type of the items
 using Priority = P;                            ///< the type of priorities
 using Handle = typename List< Pair >::Pos;     ///< the cell of an item

 [[nodiscard]] bool isEmpty( void ) const { return( f_L.isEmpty() ); }

 [[nodiscard]] int size( void ) const { return( f_size ); }

 /// O( 1 ) if sorted, O( n ) otherwise
 [[nodiscard]] const T & min( void ) const {
  return( f_L.read( first() ).second );
  }

 /// O( 1 ) if sorted, O( n ) otherwise
 T deleteMin( void ) {
  const auto p = first();
  T v = f_L.read( p ).second;
  f_L.remove( p );
  --f_size;
  return( v );
  }

 /// O( n ) if sorted, O( 1 ) otherwise
 Handle insert( const T & x , const P & p ) {
  ++f_size;
  if constexpr( Sorted )
   return( f_L.insert( after( p ) , { p , x } ) );
  else
   return( f_L.insert( f_L.head() , { p , x } ) );
  }

 /// O( n ) if sorted, O( 1 ) otherwise; the handle stays valid
 void decrease( Handle & y , const P & p ) {
  if( f_L.read( y ).first < p )
   throw( std::invalid_argument(
                      "ADS::ListPriorityQueue::decrease: larger priority" ) );
  if constexpr( Sorted ) {
   const T x = f_L.read( y ).second;
   f_L.remove( y );
   y = f_L.insert( after( p ) , { p , x } );
   }
  else
   f_L.write( y , { p , f_L.read( y ).second } );
  }

/*--------------------------------------------------------------------------*/
 private:

 Handle first( void ) const {
  if( f_L.isEmpty() )
   throw( std::logic_error( "ADS::ListPriorityQueue: empty" ) );
  auto m = f_L.head();
  if constexpr( ! Sorted )
   for( auto q = f_L.next( m ) ; ! f_L.finished( q ) ; q = f_L.next( q ) )
    if( f_L.read( q ).first < f_L.read( m ).first )
     m = q;
  return( m );
  }

 /// the position of the first item of priority larger than p
 Handle after( const P & p ) const {
  auto q = f_L.head();
  while( ! f_L.finished( q ) && ! ( p < f_L.read( q ).first ) )
   q = f_L.next( q );
  return( q );
  }

 List< Pair > f_L;   ///< the items
 int f_size = 0;     ///< their number

 };  // end( class ListPriorityQueue )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ListPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ListPriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
