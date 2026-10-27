/*--------------------------------------------------------------------------*/
/*--------------------------- File LinkedQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A QUEUE [see `ADT.h`] realized as a singly linked list with a pointer to
 * the first cell, the front, and one to the last cell, the back (exercise
 * 4.10 of the book): `enqueue()` appends a cell after the back and
 * `dequeue()` removes the front, both in \f$ O( 1 ) \f$ time, and the queue
 * has no bound on its size, unlike the circular vector of `ArrayQueue`. The
 * cells come from an `ObjectPool` [see `ObjectPool.h`], the one of a
 * `dequeue()` serving the next `enqueue()`, so that a queue in a steady
 * state calls no allocator and its cells stay in the cache.
 *
 * The contracts [see `Contract.h`]: `top()` and `dequeue()` need a queue
 * that is not empty; `enqueue()` ensures that the queue is not empty, and
 * that `top()` is \f$ v \f$ if the queue was empty. A queue is copied cell by
 * cell and moved in \f$ O( 1 ) \f$ time, the one moved from left empty. The
 * operations are defined in `LinkedQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.10,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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
#include "ADS/memory/ObjectPool.h"

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

 /// the empty queue
 LinkedQueue( void ) = default;

 /// a copy of `other`, cell by cell
 LinkedQueue( const LinkedQueue & other );

 /// the cells of `other`, which is left empty
 LinkedQueue( LinkedQueue && other ) noexcept;

 /// the items of `other`, by copy and swap
 LinkedQueue & operator=( LinkedQueue other ) noexcept;

 /// frees the cells
 ~LinkedQueue();

 /// exchanges the cells with `other`
 void swap( LinkedQueue & other ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const;

 /// puts \f$ v \f$ at the back
 ///
 /// In pseudocode (exercise 4.10), a new cell after the last one:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enqueue}( v ) \cr
 /// 2 & \quad t \gets \text{a new cell};\ t.\mathit{value} \gets v;
 ///     \ t.\mathit{next} \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{if}\ \mathit{back} = \textbf{nil}\ \textbf{then}
 ///     \ \mathit{front} \gets t\ \textbf{else}\ \mathit{back}.\mathit{next}
 ///     \gets t \cr
 /// 4 & \quad \mathit{back} \gets t
 /// \end{array}}
 /// \f]
 /// \post the queue is not empty, and \f$ v \f$ is at the front if it was
 ///       empty
 void enqueue( const T & v );

 /// removes the item at the front and returns it
 ///
 /// In pseudocode (exercise 4.10), the first cell unlinked:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{dequeue}() \cr
 /// 2 & \quad t \gets \mathit{front};\ \mathit{front} \gets
 ///     t.\mathit{next} \cr
 /// 3 & \quad \textbf{if}\ \mathit{front} = \textbf{nil}\ \textbf{then}
 ///     \ \mathit{back} \gets \textbf{nil} \cr
 /// 4 & \quad v \gets t.\mathit{value};\ \text{free the cell } t;
 ///     \ \textbf{return}\ v
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 T dequeue( void );

 /// the item at the front
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// the number of items, \f$ O( 1 ) \f$
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

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const T &;              ///< a constant reference
 using const_reference = const T &;        ///< a constant reference
 /// a constant iterator along the cells
 using iterator = LinkedIterator< Cell , T , &Cell::next , &Cell::value >;
 /// a constant iterator along the cells
 using const_iterator =
  LinkedIterator< Cell , T , &Cell::next , &Cell::value >;

 /// the iterator on the item at the front
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_front ) );
  }

 /// the iterator past the item at the back, nil
 [[nodiscard]] const_iterator end( void ) const { return( const_iterator() ); }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 Cell * f_front = nullptr;   ///< the first cell
 Cell * f_back = nullptr;    ///< the last cell
 std::size_t f_size = 0;     ///< the number of items
 ObjectPool< Cell > f_pool;  ///< the cells

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
