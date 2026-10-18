/*--------------------------------------------------------------------------*/
/*--------------------------- File LinkedQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A QUEUE [see ADT.h] realized as a singly linked list with a pointer to the
 * first cell, the front, and one to the last cell, the back (Esercizio 4.10
 * of the book): enqueue() appends a cell after the back and dequeue()
 * removes the front, both in O( 1 ) time, and the queue has no bound on its
 * size, unlike the circular vector of ArrayQueue.
 *
 * The contracts [see Contract.h]: top() and dequeue() need a queue that is
 * not empty; enqueue() ensures that the queue is not empty, and that top()
 * is v if the queue was empty. A queue is copied cell by cell and moved in
 * O( 1 ) time, the one moved from left empty. The operations are defined in
 * LinkedQueue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizio 4.10,
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

#ifndef __ADS_LinkedQueue
 #define __ADS_LinkedQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LinkedQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE realized as a singly linked list with a front and a back
template< class T >
class LinkedQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 LinkedQueue( void ) = default;

 LinkedQueue( const LinkedQueue & other );

 /// the cells of other, which is left empty
 LinkedQueue( LinkedQueue && other ) noexcept;

 LinkedQueue & operator=( LinkedQueue other ) noexcept;

 ~LinkedQueue();

 void swap( LinkedQueue & other ) noexcept;

 [[nodiscard]] bool isEmpty( void ) const;

 /// puts v at the back
 /// \post the queue is not empty, and v is at the front if it was empty
 void enqueue( const T & v );

 /// removes the item at the front and returns it
 /// \pre the queue is not empty
 T dequeue( void );

 /// the item at the front
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// the number of items, O( 1 )
 [[nodiscard]] std::size_t size( void ) const;

 /// the same items in the same order
 [[nodiscard]] bool operator==( const LinkedQueue & o ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a cell of the list
 struct Cell {
  T value;      ///< the item
  Cell * next;  ///< the next cell, toward the back
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the front to the back, forward

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using iterator = LinkedIterator< Cell , T , &Cell::next , &Cell::value >;
 using const_iterator =
  LinkedIterator< Cell , T , &Cell::next , &Cell::value >;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_front ) );
  }
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 Cell * f_front = nullptr;   ///< the first cell
 Cell * f_back = nullptr;    ///< the last cell
 std::size_t f_size = 0;     ///< the number of items

 };  // end( class LinkedQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/LinkedQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinkedQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File LinkedQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
