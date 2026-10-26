/*--------------------------------------------------------------------------*/
/*---------------------------- File MPMCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A bounded queue of many producers and many consumers, beyond the book:
 * the circular vector of Vyukov, whose places carry a sequence number
 * besides the item. The place of the index i, i mod C for a capacity C
 * that is a power of 2, holds the sequence i when it is free for the push
 * of index i, i + 1 when it holds the item of index i, and i + C when that
 * item has been taken and the place waits for the push of index i + C. A
 * producer reads the tail p and the sequence s of its place: if s = p it
 * claims the index by a compare-and-swap of the tail from p to p + 1,
 * builds the item and publishes it by the sequence p + 1 (a release); if
 * s < p the queue is full; if s > p another producer took p, and it reads
 * the tail again. A consumer does the same on the head h with the
 * sequence h + 1, and frees the place by the sequence h + C. The tail and
 * the head sit on lines of cache of their own, so that producers and
 * consumers meet only at the places they share, one read-modify-write per
 * operation; no item is ever read before its producer published it, nor
 * a place reused before its consumer freed it. As in the queue of Vyukov,
 * a thread that stops between its compare-and-swap and its publication
 * holds up the others when they come round to that place.
 * The contracts [see Contract.h]: the capacity is a power of 2, at least 2.
 * \see D. Vyukov, Bounded MPMC queue, 2010,
 *      https://www.1024cores.net/home/lock-free-algorithms/queues/
 *      bounded-mpmc-queue
 * \see R. Nikolaev, A scalable, portable, and memory-efficient lock-free
 *      FIFO queue, International Symposium on Distributed Computing
 *      (DISC), LIPIcs 146, 28:1-28:16, 2019,
 *      doi:10.4230/LIPIcs.DISC.2019.28 (the rings with a cycle per place)
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_MPMCQueue
 #define __ADS_MPMCQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPMCQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bounded queue of many producers and many consumers
template< class T >
class MPMCQueue {

 /// a place: its sequence and the room of an item
 struct Cell {
  std::atomic< std::size_t > seq;
  alignas( T ) unsigned char raw[ sizeof( T ) ];
  [[nodiscard]] T * item( void ) {
   return( std::launder( reinterpret_cast< T * >( raw ) ) );
   }
  };

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// an empty queue of the given capacity
 /// \pre capacity >= 2 is a power of 2
 explicit MPMCQueue( std::size_t capacity );
 MPMCQueue( const MPMCQueue & ) = delete;
 MPMCQueue & operator=( const MPMCQueue & ) = delete;

 /// destroys the items left
 ~MPMCQueue();

 /// enqueues the item built from args, false if the queue is full (and
 /// then nothing is built); any thread may call it
 template< class ... Args >
 bool emplace( Args && ... args );

 bool push( const T & v ) { return( emplace( v ) ); }
 bool push( T && v ) { return( emplace( std::move( v ) ) ); }

 /// dequeues an item into x, false if the queue is empty; any thread may
 /// call it
 bool tryPop( T & x );

 /// the number of items at some moment of the call, at most the capacity
 [[nodiscard]] std::size_t size( void ) const;

 /// true if the queue held no item at some moment of the call
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 [[nodiscard]] std::size_t capacity( void ) const { return( f_mask + 1 ); }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t line = 64;   ///< a line of cache

 alignas( 2 * line ) Cell * f_cell;        ///< the places
 std::size_t f_mask;                       ///< capacity - 1
 alignas( 2 * line ) std::atomic< std::size_t > f_tail{ 0 };   ///< pushes
 alignas( 2 * line ) std::atomic< std::size_t > f_head{ 0 };   ///< pops
 alignas( 2 * line ) char f_end = 0;       ///< no other data on that line
 };  // end( class MPMCQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/MPMCQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MPMCQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MPMCQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
