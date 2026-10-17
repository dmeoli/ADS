/*--------------------------------------------------------------------------*/
/*---------------------------- File SPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A bounded queue between one producer thread and one consumer thread,
 * lock-free and wait-free, beyond the book: the circular vector of the
 * queue of section 4.3.3 [see ArrayQueue.h], whose head only the consumer
 * writes and whose tail only the producer writes, so that each index is an
 * atomic with the release and acquire orders and no lock is needed (the
 * queue of Lamport). The two indices sit on lines of cache of their own, so
 * that the two threads do not invalidate each other's line at every
 * operation (false sharing), and each thread keeps a copy of the index of
 * the other one, read again only when the queue looks full or empty. The
 * capacity is a power of 2 and the indices are reduced by a mask.
 *
 * The contracts [see Contract.h]: the capacity is a power of 2, at least 2;
 * only one thread calls push() and only one calls pop(). The operations are
 * defined in SPSCQueue.tpp.
 *
 * \see L. Lamport, Specifying concurrent program modules, ACM Transactions
 *      on Programming Languages and Systems 5(2), 190-222, 1983,
 *      doi:10.1145/69624.357207
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

#ifndef __ADS_SPSCQueue
 #define __ADS_SPSCQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SPSCQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bounded queue from one producer to one consumer, lock-free
template< class T >
class SPSCQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// a queue of the given capacity
 /// \pre capacity >= 2 is a power of 2
 explicit SPSCQueue( std::size_t capacity );

 /// enqueues v, false if the queue is full; only the producer calls it
 bool push( const T & v );

 /// dequeues an item, nothing if the queue is empty; only the consumer
 /// calls it
 std::optional< T > pop( void );

 /// the number of items at some moment of the call
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] std::size_t capacity( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t line = 64;   ///< a line of cache

 std::vector< T > f_A;                     ///< the circular vector
 std::size_t f_mask;                       ///< capacity - 1
 alignas( line ) std::atomic< std::size_t > f_head{ 0 };   ///< consumer
 std::size_t f_tailSeen = 0;               ///< the tail the consumer saw
 alignas( line ) std::atomic< std::size_t > f_tail{ 0 };   ///< producer
 std::size_t f_headSeen = 0;               ///< the head the producer saw

 };  // end( class SPSCQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/SPSCQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SPSCQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SPSCQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
