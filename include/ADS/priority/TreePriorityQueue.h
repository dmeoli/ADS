/*--------------------------------------------------------------------------*/
/*------------------------ File TreePriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a tree.
 *
 * TreePriorityQueue is a Red-Black tree of the pairs ( priority , number
 * of insertion ) (section 10.1.2): every operation in O( log n ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.2,
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

#ifndef __ADS_TreePriorityQueue
 #define __ADS_TreePriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"
#include "ADS/tree/SearchTree.h"

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
/*------------------------ CLASS TreePriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a Red-Black tree
template< class T , std::totally_ordered P = int >
class TreePriorityQueue {

 using Key = std::pair< P , long >;   ///< priority, number of insertion

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;        ///< the type of the items
 using Priority = P;    ///< the type of the priorities
 using Handle = long;   ///< the number of insertion of an item

 [[nodiscard]] bool isEmpty( void ) const { return( f_t.isEmpty() ); }

 [[nodiscard]] int size( void ) const { return( int( f_t.size() ) ); }

 [[nodiscard]] const T & min( void ) const {
  if( f_t.isEmpty() )
   throw( std::logic_error( "ADS::TreePriorityQueue::min: empty" ) );
  return( RedBlackTree< Key , T >::value( f_t.min() ) );
  }

 T deleteMin( void ) {
  const T v = min();
  f_t.remove( RedBlackTree< Key , T >::key( f_t.min() ) );
  return( v );
  }

 Handle insert( const T & x , const P & p ) {
  f_t.insert( { p , long( f_p.size() ) } , x );
  f_p.push_back( p );
  return( long( f_p.size() ) - 1 );
  }

 void decrease( Handle y , const P & p ) {
  if( f_p[ y ] < p )
   throw( std::invalid_argument(
                      "ADS::TreePriorityQueue::decrease: larger priority" ) );
  const auto v = f_t.lookup( { f_p[ y ] , y } );
  if( ! v )
   throw( std::logic_error( "ADS::TreePriorityQueue::decrease: no item" ) );
  f_t.remove( { f_p[ y ] , y } );
  f_t.insert( { p , y } , *v );
  f_p[ y ] = p;
  }

/*--------------------------------------------------------------------------*/
 private:

 RedBlackTree< Key , T > f_t;   ///< the items by priority
 std::vector< P > f_p;          ///< the priority of each insertion

 };  // end( class TreePriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* TreePriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File TreePriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
