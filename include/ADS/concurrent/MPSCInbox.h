/*--------------------------------------------------------------------------*/
/*---------------------------- File MPSCInbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An inbox that many producer threads fill and one consumer thread empties,
 * lock-free, beyond the book: the messages to a worker of a parallel
 * search, e.g., the nodes that the other workers send to their owner. A
 * producer pushes a cell on a stack with a compare-and-swap on its top (the
 * stack of Treiber); the consumer takes the whole stack at once by
 * exchanging its top with nil, and reverses it, so that the messages of
 * each producer come out in the order in which it sent them. take() thus
 * costs one atomic operation however many messages there are, and the
 * consumer never competes with the producers on single items; since the
 * consumer only takes the whole stack, the ABA problem of the stack of
 * Treiber cannot occur.
 *
 * The contracts [see Contract.h]: only one thread calls take(). The
 * operations are defined in MPSCInbox.tpp.
 *
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986
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

#ifndef __ADS_MPSCInbox
 #define __ADS_MPSCInbox
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPSCInbox -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an inbox filled by many threads and emptied by one, lock-free
template< class T >
class MPSCInbox {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the messages

 MPSCInbox( void ) = default;
 MPSCInbox( const MPSCInbox & ) = delete;
 MPSCInbox & operator=( const MPSCInbox & ) = delete;

 /// frees the messages not taken
 ~MPSCInbox();

 /// sends v; any thread may call it
 void push( T v );

 /// all the messages sent so far and not yet taken, those of each sender
 /// in the order of sending; only the consumer calls it
 [[nodiscard]] std::vector< T > take( void );

 /// true if no message was waiting at some moment of the call
 [[nodiscard]] bool isEmpty( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a cell of the stack
 struct Cell {
  T value;
  Cell * next;
  };

 std::atomic< Cell * > f_top{ nullptr };   ///< the top of the stack

 };  // end( class MPSCInbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/MPSCInbox.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MPSCInbox.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MPSCInbox.h --------------------------*/
/*--------------------------------------------------------------------------*/
