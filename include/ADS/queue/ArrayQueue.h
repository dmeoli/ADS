/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The QUEUE of the book [see ADT.h] realized with a circular vector of m
 * places, the place 0 following the place m - 1: the queue is in size
 * consecutive places from the one of its front, head, so that its back is
 * in the place \f$ ( head + size - 1 ) \bmod m \f$ and every operation takes
 * O( 1 ) time.
 *
 * The contracts [see Contract.h] are the preconditions of the book: top()
 * and dequeue() need a queue that is not empty, enqueue() one that is not
 * full; enqueue() ensures one item more, dequeue() one less, and the
 * invariant is \f$ 0 \leq size \leq m \f$ with the front inside the
 * vector. The operations are defined in ArrayQueue.tpp.
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

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <memory>
#include <utility>

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
 explicit ArrayQueue( std::size_t dim );

 [[nodiscard]] bool isEmpty( void ) const;

 /// the item at the front
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the item at the front and returns it
 /// \pre the queue is not empty
 /// \post one item less
 T dequeue( void );

 /// puts v at the back
 /// \pre the queue is not full
 /// \post one item more
 void enqueue( const T & v );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// the maximum number of items
 [[nodiscard]] std::size_t capacity( void ) const;

 /// whether v is in the queue, O( n )
 [[nodiscard]] bool contains( const T & v ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the front to the back, random
 // access on the circular vector

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using iterator = CircularIterator< T >;
 using const_iterator = CircularIterator< T >;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_A.get() , f_head , f_capacity , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_A.get() , f_head , f_capacity ,
                          std::ptrdiff_t( f_size ) ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the items fit in the vector and the front is in it
 [[nodiscard]] bool fits( void ) const;

 std::unique_ptr< T[] > f_A;   ///< the circular vector
 std::size_t f_size;           ///< the number of items
 std::size_t f_head;           ///< the place of the front
 std::size_t f_capacity;       ///< the maximum number of items

 };  // end( class ArrayQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/ArrayQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArrayQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
