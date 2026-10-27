/*--------------------------------------------------------------------------*/
/*---------------------------- File MPMCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A bounded queue of many producers and many consumers, beyond the book:
 * the circular vector of Vyukov, whose places carry a sequence number
 * besides the item. The place of the index \f$ i \f$, \f$ i \bmod c \f$ for
 * a capacity \f$ c \f$ that is a power of 2, holds the sequence \f$ i \f$
 * when it is free for the push of index \f$ i \f$, \f$ i + 1 \f$ when it
 * holds the item of index \f$ i \f$, and \f$ i + c \f$ when that item has
 * been taken and the place waits for the push of index \f$ i + c \f$. A
 * producer reads the tail \f$ p \f$ and the sequence \f$ s \f$ of its
 * place: if \f$ s = p \f$ it claims the index by a compare-and-swap of the
 * tail from \f$ p \f$ to \f$ p + 1 \f$, builds the item and publishes it by
 * the sequence \f$ p + 1 \f$ (a release); if \f$ s < p \f$ the queue is
 * full; if \f$ s > p \f$ another producer took \f$ p \f$, and it reads the
 * tail again. A consumer does the same on the head \f$ h \f$ with the
 * sequence \f$ h + 1 \f$, and frees the place by the sequence
 * \f$ h + c \f$. The tail and the head sit on lines of cache of their own,
 * so that producers and consumers meet only at the places they share, one
 * read-modify-write per operation; no item is ever read before its
 * producer published it, nor a place reused before its consumer freed it.
 * As in the queue of Vyukov, a thread that stops between its
 * compare-and-swap and its publication holds up the others when they come
 * round to that place: the queue is lock-free in the common case, not in
 * the strict sense.
 *
 * The contracts [see `Contract.h`]: the capacity is a power of 2, at least
 * 2. The operations are defined in `MPMCQueue.tpp`.
 *
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
  /// the sequence of the place
  std::atomic< std::size_t > seq;
  /// the room of the item
  alignas( T ) unsigned char raw[ sizeof( T ) ];
  /// the item in the room
  [[nodiscard]] T * item( void ) {
   return( std::launder( reinterpret_cast< T * >( raw ) ) );
   }
  };

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue of capacity \f$ c \f$
 /// \pre \f$ c \ge 2 \f$ is a power of 2
 explicit MPMCQueue( std::size_t capacity );
 /// no copy, since the threads share it by its address
 MPMCQueue( const MPMCQueue & ) = delete;
 /// no copy, since the threads share it by its address
 MPMCQueue & operator=( const MPMCQueue & ) = delete;

 /// destroys the items left
 ~MPMCQueue();

 /// enqueues the item built from `args`, false if the queue is full (and
 /// then nothing is built); any thread may call it
 ///
 /// In pseudocode (the queue of Vyukov, beyond the book), \f$ C[ i ] \f$
 /// the place \f$ i \bmod c \f$ with its sequence \f$ C[ i ].s \f$ and
 /// \f$ t \f$ the tail:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{emplace}( x ) \cr
 /// 2 & \quad p \gets t \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad s \gets C[ p ].s\quad \triangleright\ \text{acquire} \cr
 /// 5 & \qquad \textbf{if}\ s = p\ \textbf{then}\quad \triangleright
 ///     \ \text{the place is free for } p \cr
 /// 6 & \qquad\quad \textbf{if}\ \mathrm{CAS}( t , p , p + 1 )
 ///     \ \textbf{then} \cr
 /// 7 & \qquad\qquad C[ p ].x \gets x;\ C[ p ].s \gets p + 1\quad
 ///     \triangleright\ \text{release} \cr
 /// 8 & \qquad\qquad \textbf{return true} \cr
 /// 9 & \qquad\quad \textbf{else}\ p \gets t\quad \triangleright
 ///     \ \text{the CAS failed and read it} \cr
 /// 10 & \qquad \textbf{else if}\ s < p\ \textbf{then return false}\quad
 ///     \triangleright\ \text{full} \cr
 /// 11 & \qquad \textbf{else}\ p \gets t\quad \triangleright\ \text{another}
 ///     \text{ producer took } p
 /// \end{array}}
 /// \f]
 template< class ... Args >
 bool emplace( Args && ... args );

 /// enqueues a copy of \f$ v \f$, false if the queue is full; any thread
 /// may call it
 bool push( const T & v ) { return( emplace( v ) ); }

 /// enqueues \f$ v \f$, moved, false if the queue is full; any thread may
 /// call it
 bool push( T && v ) { return( emplace( std::move( v ) ) ); }

 /// dequeues an item into \f$ x \f$, false if the queue is empty; any
 /// thread may call it
 ///
 /// In pseudocode (the queue of Vyukov, beyond the book), \f$ h \f$ the
 /// head:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPop}( x ) \cr
 /// 2 & \quad q \gets h \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad s \gets C[ q ].s\quad \triangleright\ \text{acquire} \cr
 /// 5 & \qquad \textbf{if}\ s = q + 1\ \textbf{then}\quad \triangleright
 ///     \ \text{the item of } q \text{ is there} \cr
 /// 6 & \qquad\quad \textbf{if}\ \mathrm{CAS}( h , q , q + 1 )
 ///     \ \textbf{then} \cr
 /// 7 & \qquad\qquad x \gets C[ q ].x;\ C[ q ].s \gets q + c\quad
 ///     \triangleright\ \text{release} \cr
 /// 8 & \qquad\qquad \textbf{return true} \cr
 /// 9 & \qquad\quad \textbf{else}\ q \gets h \cr
 /// 10 & \qquad \textbf{else if}\ s < q + 1\ \textbf{then return false}\quad
 ///     \triangleright\ \text{empty} \cr
 /// 11 & \qquad \textbf{else}\ q \gets h
 /// \end{array}}
 /// \f]
 bool tryPop( T & x );

 /// the number of items at some moment of the call, at most \f$ c \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// true if the queue held no item at some moment of the call
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the capacity \f$ c \f$
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
