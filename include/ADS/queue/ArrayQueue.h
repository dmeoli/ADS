/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The QUEUE of the book [see ADT.h] realized with a circular vector of m
 * places, the place 0 following the place m - 1: the queue is in size
 * consecutive places from the one of its front, head, so that its back is
 * in the place \f$ ( head + size - 1 ) \bmod m \f$ and every operation takes
 * O( 1 ) time. As for ArrayStack, the preconditions of the book (no
 * dequeue() from an empty queue, no enqueue() in a full one) are checked,
 * and std::logic_error is thrown.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_ArrayQueue
 #define __ADS_ArrayQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <memory>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ArrayQueue ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE realized with a circular vector of a given capacity
template< class T >
class ArrayQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue of at most dim items
 explicit ArrayQueue( std::size_t dim )
  : f_A( new T[ dim ] ) , f_size( 0 ) , f_head( 0 ) , f_capacity( dim ) {}

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// the item at the front
 [[nodiscard]] const T & top( void ) const {
  if( f_size == 0 )
   throw( std::logic_error( "ADS::ArrayQueue::top: empty queue" ) );
  return( f_A[ f_head ] );
  }

 /// removes the item at the front and returns it
 T dequeue( void ) {
  if( f_size == 0 )
   throw( std::logic_error( "ADS::ArrayQueue::dequeue: empty queue" ) );
  T t = std::move( f_A[ f_head ] );
  f_head = ( f_head + 1 ) % f_capacity;
  --f_size;
  return( t );
  }

 /// puts v at the back
 void enqueue( const T & v ) {
  if( f_size == f_capacity )
   throw( std::logic_error( "ADS::ArrayQueue::enqueue: full queue" ) );
  f_A[ ( f_head + f_size ) % f_capacity ] = v;
  ++f_size;
  }

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// whether v is in the queue, O( n )
 [[nodiscard]] bool contains( const T & v ) const {
  for( std::size_t i = 0 ; i < f_size ; ++i )
   if( f_A[ ( f_head + i ) % f_capacity ] == v ) return( true );
  return( false );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::unique_ptr< T[] > f_A;   ///< the circular vector
 std::size_t f_size;           ///< the number of items
 std::size_t f_head;           ///< the place of the front
 std::size_t f_capacity;       ///< the maximum number of items

 };  // end( class ArrayQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ArrayQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
