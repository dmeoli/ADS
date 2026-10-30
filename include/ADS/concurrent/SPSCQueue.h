/*--------------------------------------------------------------------------*/
/*---------------------------- File SPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A bounded queue between one producer thread and one consumer thread,
 * lock-free and wait-free, beyond the book: the circular vector of the
 * queue of section 4.3.3 [see `ArrayQueue.h`], whose head only the consumer
 * writes and whose tail only the producer writes, so that each index is an
 * atomic with the release and acquire orders and no lock is needed (the
 * queue of Lamport). Each thread keeps a copy of the index of the other
 * one, read again only when the queue looks full or empty, and the two
 * indices and the two copies sit on four lines of cache of their own,
 * grouped in pairs of lines so that a thread that spins on a full or empty
 * queue, rewriting its copy, does not invalidate the line that the other
 * one writes or reads (false sharing, also through the prefetch of the
 * adjacent line). A consumer that finds the queue empty asks for the line
 * of its next place before it reads the tail again, so that, when the item
 * has come, the two lines travel together and a round trip between the
 * threads costs one miss a side instead of two in a row; a consumer whose
 * last pop took an item and that finds fewer than
 * \f$ \min( 64 , c / 2 ) \f$ new ones, \f$ c \f$ the capacity, waits 64
 * pauses of the processor for a batch before it takes them (the
 * backtracking of B-Queue), while after a pop that found nothing it takes
 * at once what comes, so that a single item after a silence costs no wait.
 * The places are raw memory with a line of padding at each end, so that
 * the first and the last items share no line with other data. The
 * capacity is a power of 2 and the indices, which only grow, are reduced
 * by a mask.
 *
 * The contracts [see `Contract.h`]: the capacity is a power of 2, at least
 * 2; only one thread calls `push()` and only one calls `pop()`. The
 * operations are defined in `SPSCQueue.tpp`.
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
 *         Università di Pisa \n
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

 /// the empty queue of capacity \f$ c \f$
 /// \pre \f$ c \ge 2 \f$ is a power of 2
 explicit SPSCQueue( std::size_t capacity );

 /// no copy, since the two threads share it by its address
 SPSCQueue( const SPSCQueue & ) = delete;
 /// no copy, since the two threads share it by its address
 SPSCQueue & operator=( const SPSCQueue & ) = delete;

 /// destroys the items left
 ~SPSCQueue();

 /// enqueues a copy of \f$ v \f$, false if the queue is full; only the
 /// producer calls it
 bool push( const T & v ) { return( emplace( v ) ); }

 /// enqueues \f$ v \f$, moved, false if the queue is full; only the producer
 /// calls it
 bool push( T && v ) { return( emplace( std::move( v ) ) ); }

 /// enqueues the item built from `args`, false if the queue is full (and
 /// then nothing is built); only the producer calls it
 ///
 /// In pseudocode (the queue of Lamport, beyond the book), \f$ t \f$ and
 /// \f$ h \f$ the tail and the head, which only grow, \f$ c \f$ the
 /// capacity and \f$ \mathit{seen} \f$ the copy of the head that the
 /// producer keeps:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{emplace}( x ) \cr
 /// 2 & \quad t' \gets t\quad \triangleright\ \text{relaxed, only this}
 ///     \text{ thread writes it} \cr
 /// 3 & \quad \textbf{if}\ t' - \mathit{seen} = c\ \textbf{then} \cr
 /// 4 & \qquad \mathit{seen} \gets h\quad \triangleright\ \text{acquire:}
 ///     \text{ the places freed are seen free} \cr
 /// 5 & \qquad \textbf{if}\ t' - \mathit{seen} = c\ \textbf{then}
 ///     \ \textbf{return false} \cr
 /// 6 & \quad A[ t' \bmod c ] \gets x \cr
 /// 7 & \quad t \gets t' + 1\quad \triangleright\ \text{release: the item}
 ///     \text{ is written before the tail} \cr
 /// 8 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 template< class ... Args >
 bool emplace( Args && ... args );

 /// dequeues an item, nothing if the queue is empty; only the consumer
 /// calls it
 ///
 /// It is `tryPop()` below, with the item returned.
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \textbf{if}\ \mathrm{tryPop}( x )\ \textbf{then return}\ x
 ///     \ \textbf{else return nil}
 /// \end{array}}
 /// \f]
 std::optional< T > pop( void );

 /// dequeues an item into \f$ x \f$, false if the queue is empty; only the
 /// consumer calls it
 ///
 /// In pseudocode (the queue of Lamport with the copies of Lee et al. and
 /// the wait for a batch of B-Queue, beyond the book), \f$ \mathit{seen} \f$
 /// the copy of the tail that the consumer keeps and
 /// \f$ \mathit{streaming} \f$ true if its last pop took an item:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPop}( x ) \cr
 /// 2 & \quad h' \gets h\quad \triangleright\ \text{relaxed, only this}
 ///     \text{ thread writes it} \cr
 /// 3 & \quad \textbf{if}\ h' = \mathit{seen}\ \textbf{then} \cr
 /// 4 & \qquad \mathrm{prefetch}( A[ h' \bmod c ] );\ \mathit{seen} \gets
 ///     t\quad \triangleright\ \text{acquire} \cr
 /// 5 & \qquad \textbf{if}\ \mathit{streaming}\ \textbf{and}\ 0 <
 ///     \mathit{seen} - h' < \min( 64 , c / 2 )\ \textbf{then} \cr
 /// 6 & \qquad\quad \text{wait 64 pauses};\ \mathit{seen} \gets t \cr
 /// 7 & \qquad \textbf{if}\ h' = \mathit{seen}\ \textbf{then}
 ///     \ \mathit{streaming} \gets \textbf{false};\ \textbf{return false} \cr
 /// 8 & \quad x \gets A[ h' \bmod c ] \cr
 /// 9 & \quad h \gets h' + 1\quad \triangleright\ \text{release: the place}
 ///     \text{ is read before it is freed} \cr
 /// 10 & \quad \mathit{streaming} \gets \textbf{true};
 ///     \ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool tryPop( T & x );

 /// enqueues the first of the \f$ n \f$ items from `first` that fit, and
 /// returns how many, publishing them all by one store of the tail; only
 /// the producer calls it
 ///
 /// In pseudocode (the queue of Lamport for a batch, beyond the book), as
 /// `emplace()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pushMany}( x_0 , \ldots , x_{n - 1} )
 ///     \cr
 /// 2 & \quad t' \gets t;\ \textbf{if}\ t' - \mathit{seen} < n
 ///     \ \textbf{then}\ \mathit{seen} \gets h\quad \triangleright
 ///     \ \text{acquire} \cr
 /// 3 & \quad k \gets \min\{ n , c - ( t' - \mathit{seen} ) \} \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
 ///     \ A[ ( t' + j ) \bmod c ] \gets x_j \cr
 /// 5 & \quad t \gets t' + k\quad \triangleright\ \text{release};
 ///     \ \textbf{return}\ k
 /// \end{array}}
 /// \f]
 template< class It >
 std::size_t pushMany( It first , std::size_t n );

 /// the calls `f( x )` on the items \f$ x \f$ in the queue, at most
 /// \f$ n \f$, each dequeued after its call, which may move it, with no
 /// wait for a batch; how many, all freed by one store of the head; only
 /// the consumer calls it
 ///
 /// In pseudocode (the queue of Lamport for a batch, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{consume}( f , n ) \cr
 /// 2 & \quad h' \gets h;\ \textbf{if}\ h' = \mathit{seen}\ \textbf{then}
 ///     \ \mathit{seen} \gets t\quad \triangleright\ \text{acquire} \cr
 /// 3 & \quad k \gets \min\{ n , \mathit{seen} - h' \} \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
 ///     \ f( A[ ( h' + j ) \bmod c ] ) \cr
 /// 5 & \quad h \gets h' + k\quad \triangleright\ \text{release};
 ///     \ \textbf{return}\ k
 /// \end{array}}
 /// \f]
 /// If a call throws, the items before it and its own are dequeued.
 template< class F >
 std::size_t consume( F && f ,
                      std::size_t n = std::size_t( -1 ) );

 /// wakes the consumer if it sleeps in `waitPop()`, after pushes: a fence
 /// and the read of a line when it does not; only the producer calls it
 void notify( void ) noexcept { f_wait.notify(); }

 /// dequeues an item into \f$ x \f$, sleeping until one comes [see
 /// `EventCount.h`], the producer calling `notify()` after its pushes; only
 /// the consumer calls it
 ///
 /// In pseudocode (beyond the book), on the wait of `EventCount`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{waitPop}( x ) \cr
 /// 2 & \quad \mathrm{await}( \mathrm{tryPop}( x ) )
 /// \end{array}}
 /// \f]
 void waitPop( T & x ) { f_wait.await( [ & ] { return( tryPop( x ) ); } ); }

 /// the number of items at some moment of the call, \f$ t - h \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// the capacity \f$ c \f$
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
