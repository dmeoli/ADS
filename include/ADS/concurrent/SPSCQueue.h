/*--------------------------------------------------------------------------*/
/*---------------------------- File SPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A bounded queue between one producer thread and one consumer thread,
 * lock-free and wait-free, beyond the book: the circular vector of the
 * queue of section 4.3.3 [see ArrayQueue.h], whose head only the consumer
 * writes and whose tail only the producer writes, so that each index is an
 * atomic with the release and acquire orders and no lock is needed (the
 * queue of Lamport). Each thread keeps a copy of the index of the other
 * one, read again only when the queue looks full or empty, and the two
 * indices and the two copies sit on four lines of cache of their own,
 * grouped in pairs of lines so that a thread that spins on a full or empty
 * queue, rewriting its copy, does not invalidate the line that the other
 * one writes or reads (false sharing, also through the prefetch of the
 * adjacent line); a consumer that finds the queue empty asks for the line
 * of its next place before it reads the tail again, so that, when the item
 * has come, the two lines travel together and a round trip between the
 * threads costs one miss a side instead of two in a row; a consumer whose
 * last pop took an item and that finds fewer than min( 64 , capacity / 2 )
 * new ones waits 64 pauses of the processor for a batch before it takes
 * them (the backtracking of B-Queue), while after a pop that found nothing
 * it takes at once what comes, so that a single item after a silence
 * costs no wait;
 * the places are raw memory with a line of padding at each end, so that
 * the first and the last items share no line with other data. The
 * capacity is a power of 2 and the indices are reduced by a mask.
 *
 * The contracts [see Contract.h]: the capacity is a power of 2, at least 2;
 * only one thread calls push() and only one calls pop(). The operations are
 * defined in SPSCQueue.tpp.
 *
 * \see L. Lamport, Specifying concurrent program modules, ACM Transactions
 *      on Programming Languages and Systems 5(2), 190-222, 1983,
 *      doi:10.1145/69624.357207
 * \see P. P. C. Lee, T. Bu, G. Chandranmenon, A lock-free, cache-efficient
 *      multi-core synchronization mechanism for line-rate network traffic
 *      monitoring, IEEE International Symposium on Parallel and
 *      Distributed Processing (IPDPS), 1-12, 2010,
 *      doi:10.1109/IPDPS.2010.5470368 (the copies of the indices)
 * \see J. Wang, K. Zhang, X. Tang, B. Hua, B-Queue: efficient and
 *      practical queuing for fast core-to-core communication,
 *      International Journal of Parallel Programming 41(1), 137-159, 2013,
 *      doi:10.1007/s10766-012-0213-x (the wait for a batch)
 * \see Intel 64 and IA-32 Architectures Optimization Reference Manual,
 *      the spatial prefetcher that fetches the pairs of lines of 128
 *      bytes, https://www.intel.com/content/www/us/en/content-details/
 *      671488/intel-64-and-ia-32-architectures-optimization-reference-
 *      manual-volume-1.html
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
#include "ADS/concurrent/EventCount.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <utility>

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

 SPSCQueue( const SPSCQueue & ) = delete;
 SPSCQueue & operator=( const SPSCQueue & ) = delete;

 /// destroys the items left
 ~SPSCQueue();

 /// enqueues v, false if the queue is full; only the producer calls it
 bool push( const T & v ) { return( emplace( v ) ); }
 bool push( T && v ) { return( emplace( std::move( v ) ) ); }

 /// enqueues the item built from args, false if the queue is full (and
 /// then nothing is built); only the producer calls it
 template< class ... Args >
 bool emplace( Args && ... args );

 /// dequeues an item, nothing if the queue is empty; only the consumer
 /// calls it
 std::optional< T > pop( void );

 /// dequeues an item into x, false if the queue is empty; only the
 /// consumer calls it
 bool tryPop( T & x );

 /// wakes the consumer if it sleeps in waitPop(), after pushes: a fence
 /// and the read of a line when it does not; only the producer calls it
 void notify( void ) noexcept { f_wait.notify(); }

 /// dequeues an item into x, sleeping until one comes [see
 /// EventCount.h], the producer calling notify() after its pushes; only
 /// the consumer calls it
 void waitPop( T & x ) { f_wait.await( [ & ] { return( tryPop( x ) ); } ); }

 /// the number of items at some moment of the call
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] std::size_t capacity( void ) const { return( f_mask + 1 ); }

 private:
 static constexpr std::size_t line = 64;   ///< a line of cache
 static constexpr std::size_t pad = ( line + sizeof( T ) - 1 ) / sizeof( T );

 /// the place of the index i
 [[nodiscard]] T * at( std::size_t i ) const {
  return( f_A + pad + ( i & f_mask ) );
  }

 /// the tail into the consumer's copy, when it has taken all those of
 /// the copy (h is the head): if its last pop took an item and only a few
 /// came since, it first waits 64 pauses of the processor for a batch, so
 /// that a consumer right behind a streaming producer does not take from
 /// it the lines of the tail and of the items at each item
 void refresh( std::size_t h );

 /// the batch of items that a streaming consumer waits for
 [[nodiscard]] std::size_t batch( void ) const {
  return( std::min< std::size_t >( 64 , capacity() / 2 ) );
  }

 /// the wait for a batch, 64 pauses of the processor
 static void relax( void );

 /// a hint to load the line of p, so that a consumer that finds the queue
 /// empty fetches the place and the tail together, one miss and not two
 /// in a row when the item has come
 static void prefetch( const T * p ) {
#if defined( __GNUC__ ) || defined( __clang__ )
  __builtin_prefetch( p );
#else
  (void) p;
#endif
  }

 // the queue begins on a pair of lines, which the processor may fetch
 // together (the adjacent line prefetcher of x86), so that each pair
 // holds the line of the places and the tail, the copy of the head and
 // the head, the copy of the tail and nothing: the copy that a thread
 // rewrites while it spins shares no pair with the index of the other
 alignas( 2 * line ) T * f_A;              ///< the places, padded
 std::size_t f_mask;                       ///< capacity - 1
 alignas( line ) std::atomic< std::size_t > f_tail{ 0 };   ///< producer
 alignas( line ) std::size_t f_headSeen = 0;   ///< the head it saw
 alignas( line ) std::atomic< std::size_t > f_head{ 0 };   ///< consumer
 alignas( line ) std::size_t f_tailSeen = 0;   ///< the tail it saw
 bool f_streaming = false;                 ///< its last pop took one
 alignas( line ) char f_end = 0;           ///< no other data on that line
 EventCount f_wait;                        ///< the sleep of the consumer
 };  // end( class SPSCQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/SPSCQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SPSCQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SPSCQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
