/*--------------------------------------------------------------------------*/
/*--------------------------- File UniqueQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A QUEUE [see ADT.h] with no duplicates, beyond the book, with one of two
 * policies for an item that is already in the queue: IGNORE_NEW does not
 * enqueue it again, FORGET_OLD removes the old copy and enqueues the item
 * at the back. The items are kept in a List [see List.h], so that
 * enqueue() takes O( n ) time for the search of the copy and the other
 * operations O( 1 ).
 *
 * The contracts [see Contract.h]: top() and dequeue() need a queue that is
 * not empty; enqueue() ensures that the item is in the queue, at the back
 * with FORGET_OLD; the invariant is that no item occurs twice. The
 * operations are defined in UniqueQueue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3,
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

#ifndef __ADS_UniqueQueue
 #define __ADS_UniqueQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS UniqueQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// what enqueue() does with an item already in the queue
enum class Duplicate {
 IGNORE_NEW ,   ///< the queue is left as it is
 FORGET_OLD     ///< the old copy is removed, the item goes to the back
 };

/// a QUEUE with no duplicates
template< std::equality_comparable T , Duplicate D = Duplicate::IGNORE_NEW >
class UniqueQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 [[nodiscard]] bool isEmpty( void ) const;

 /// puts v at the back, unless the policy keeps an old copy of it
 /// \post v is in the queue, at the back with FORGET_OLD
 void enqueue( const T & v );

 /// removes the item at the front and returns it
 /// \pre the queue is not empty
 T dequeue( void );

 /// the item at the front
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// whether v is in the queue, O( n )
 [[nodiscard]] bool contains( const T & v ) const;

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the front to the back, bidirectional;
 // a change through it could break the uniqueness

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const T &;
 using const_reference = const T &;
 using iterator = typename List< T >::const_iterator;
 using const_iterator = typename List< T >::const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// no item occurs twice, \f$ O( n^2 ) \f$
 [[nodiscard]] bool unique( void ) const;

 /// the position of v in the list, or pos_n
 [[nodiscard]] typename List< T >::Pos find( const T & v ) const;

 List< T > f_L;           ///< the items, the front first
 std::size_t f_size = 0;  ///< the number of items

 };  // end( class UniqueQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/UniqueQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* UniqueQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File UniqueQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
