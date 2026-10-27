/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The QUEUE of the book [see `ADT.h`] realized with a circular vector of
 * \f$ m \f$ places, the place 0 following the place \f$ m - 1 \f$: the queue
 * is in \f$ n \f$ consecutive places from the one of its front, \f$ h \f$,
 * so that its back is in the place \f$ ( h + n - 1 ) \bmod m \f$ and every
 * operation takes \f$ O( 1 ) \f$ time; the modulo is a comparison and a
 * subtraction, since the sum is below \f$ 2 m \f$, instead of a division.
 *
 * The contracts [see `Contract.h`] are the preconditions of the book:
 * `top()` and `dequeue()` need a queue that is not empty, `enqueue()` one
 * that is not full; `enqueue()` ensures one item more, `dequeue()` one less,
 * and the invariant is \f$ 0 \leq n \leq m \f$ with the front inside the
 * vector. The operations are defined in `ArrayQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.3,
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

 /// the empty queue of at most \f$ \mathit{dim} \f$ items
 explicit ArrayQueue( std::size_t dim );

 /// true if there is no item, i.e., \f$ n = 0 \f$
 [[nodiscard]] bool empty( void ) const;

 /// the item at the front, \f$ A[ h ] \f$
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the item at the front and returns it
 ///
 /// In pseudocode (section 4.3.3), \f$ A[ 0 .. m - 1 ] \f$ the circular
 /// vector:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{dequeue}() \cr
 /// 2 & \quad t \gets A[ h ] \cr
 /// 3 & \quad h \gets ( h + 1 ) \bmod m;\ n \gets n - 1 \cr
 /// 4 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less
 T dequeue( void );

 /// puts \f$ v \f$ at the back
 ///
 /// In pseudocode (section 4.3.3):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enqueue}( v ) \cr
 /// 2 & \quad A[ ( h + n ) \bmod m ] \gets v;\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre the queue is not full
 /// \post one item more
 void enqueue( const T & v );

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// the maximum number of items, \f$ m \f$
 [[nodiscard]] std::size_t capacity( void ) const;

 /// whether \f$ v \f$ is in the queue, \f$ O( n ) \f$
 [[nodiscard]] bool contains( const T & v ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the front to the back, random
 // access on the circular vector

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const T &;              ///< a constant reference
 using const_reference = const T &;        ///< a constant reference
 using iterator = CircularIterator< T >;   ///< a constant iterator
 using const_iterator = CircularIterator< T >;   ///< a constant iterator

 /// the iterator on the item at the front
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_A.get() , f_head , f_capacity , 0 ) );
  }
 /// the iterator past the item at the back
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_A.get() , f_head , f_capacity ,
                          std::ptrdiff_t( f_size ) ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
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
