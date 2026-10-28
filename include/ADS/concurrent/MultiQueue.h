/*--------------------------------------------------------------------------*/
/*--------------------------- File MultiQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A relaxed priority queue shared by threads, beyond the book: the
 * MultiQueue of Rihani, Sanders and Dementiev, with the stickiness and the
 * buffers of Williams, Sanders and Dementiev. It is meant as the shared
 * OPEN of a parallel best-first search, i.e., of pairs \f$ ( k , x ) \f$ of
 * a key and an item, the integer or real key \f$ k \f$ deciding the order
 * by `less` (the least first, as \f$ f \f$ in a search).
 *
 * The \f$ n \f$ queues are sequential priority queues, each with a lock of
 * its own and with the key of its minimum kept apart in an atomic word,
 * which the other threads read without the lock. A push puts the pair in
 * a random queue whose lock it takes at the first try, and draws another
 * one when the lock is held. A pop reads the minima of two distinct random
 * queues, locks the queue with the smaller one, if free (draws two other
 * queues otherwise), and takes its minimum. The pop therefore returns an
 * item that is not always the least of all: its <em>rank error</em> is the
 * number of the pairs present with a smaller key. With \f$ n \f$ queues in
 * the sequential model (the pairs spread at random over the queues, then
 * removed by the better of two distinct queues drawn at random), the
 * expected rank error converges to
 * \f[ \sum_{i=1}^{n-1} \sigma_{\le i} \, \frac{n - i - 1}{i} ,
 *     \qquad \sigma_{\le i} = 1 - \frac{( n - i ) ( n - i - 1 )}
 *     {n ( n - 1 )} , \f]
 * a little less than the \f$ \frac{5}{6} n - 1 + \frac{1}{6 n} \f$ of two
 * draws with replacement, and its maximum over \f$ \mathrm{poly}( n ) \f$
 * pops is
 * \f$ O( n \log n ) \f$ with high probability (Walzer and Williams; the
 * bounds \f$ O( n ) \f$ and \f$ O( n \log n ) \f$ are those of Alistarh et
 * al.), while a pop from a single random queue makes it grow without bound
 * [see \ref theory_concurrent_mq]. A run of \f$ p \f$ threads with
 * \f$ n = 2 p \f$ queues keeps the lock of a queue rarely contended; with
 * \f$ n = 1 \f$ the queue is exact, a priority queue under one lock, for
 * testing and for comparison.
 *
 * Two devices of Williams, Sanders and Dementiev save most of the misses
 * of the cache. A thread keeps the two queues it drew for \f$ s \f$
 * operations (the <em>stickiness</em>), so that their lines stay in its
 * cache, and draws two others after \f$ s \f$ operations or when a lock is
 * held; a push goes to one of the two at random, a pop to the better one.
 * Each queue is an 8-ary heap [see `DaryHeap.h`] behind two buffers of 16
 * pairs: a deletion buffer \f$ D \f$, sorted, holding the least pairs of
 * the queue, from which a pop takes its pair (a ring of 16 places, so that
 * neither the pop of its least pair nor the eviction of its greatest
 * moves the others), and an insertion buffer
 * \f$ I \f$, unsorted, which a push fills and which goes into the heap
 * only when full or when \f$ D \f$ runs out; a pair with a key less than
 * the greatest of \f$ D \f$ goes into \f$ D \f$, whose greatest pair goes
 * into \f$ I \f$. The invariant of a queue is that \f$ D \f$ is empty only
 * if the queue is, and that no pair of \f$ I \f$ or of the heap is less than
 * one of \f$ D \f$: each queue stays exact, the least of its pairs being the
 * least of \f$ D \f$, and the heap is touched once per 16 operations or so.
 * The stickiness has no theory yet: the measures of Williams, Sanders and
 * Dementiev show rank errors that grow about linearly in \f$ s \f$, which
 * is why \f$ s \f$ is a parameter, 1 for the bounds above.
 *
 * A pop returns nothing only after a scan of all the queues, each locked in
 * turn, found them empty: when no push runs at the same time, an empty
 * answer means an empty structure, which a search uses for its
 * termination. The locks are test-and-test-and-set on a byte, taken by an
 * exchange after a plain read; a pop holds one lock, a push one, and the
 * scan one at a time, so that no deadlock is possible. Each operation of
 * a single queue is linearizable at its lock; the structure as a whole is
 * relaxed, i.e., it is not a linearizable priority queue but for
 * \f$ n = 1 \f$.
 *
 * The keys must be integers or reals that an atomic word holds without a
 * lock; the greatest key under `less` (the infinity of a real) stands for
 * an empty queue in the atomic minima, and a pair with that key is popped
 * only by the scan. The items must be copyable and default constructible.
 * The threads use the structure through a `Handle` each, which holds the
 * generator of their draws and their stickiness.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 1 \f$ and \f$ s \ge 1 \f$; the
 * invariant of a queue, checked under its lock after each operation, is
 * that of the buffers above, and `consistent()` checks it on all the
 * queues, together with their atomic minima. The operations are defined
 * in `MultiQueue.tpp`.
 *
 * \see H. Rihani, P. Sanders, R. Dementiev, MultiQueues: simple relaxed
 *      concurrent priority queues, ACM Symposium on Parallelism in
 *      Algorithms and Architectures (SPAA), 80-82, 2015,
 *      doi:10.1145/2755573.2755616
 * \see D. Alistarh, J. Kopinsky, J. Li, G. Nadiradze, The power of choice
 *      in priority scheduling, ACM Symposium on Principles of Distributed
 *      Computing (PODC), 283-292, 2017, doi:10.1145/3087801.3087810
 * \see M. Williams, P. Sanders, R. Dementiev, Engineering MultiQueues: fast
 *      relaxed concurrent priority queues, European Symposium on Algorithms
 *      (ESA), LIPIcs 204, 81:1-81:17, 2021,
 *      doi:10.4230/LIPIcs.ESA.2021.81
 * \see M. Williams, P. Sanders, The MultiQueue: a simple and fast relaxed
 *      concurrent priority queue, ACM Transactions on Parallel Computing
 *      12(4), 1-39, 2025, doi:10.1145/3771738
 * \see S. Walzer, M. Williams, A simple yet exact analysis of the
 *      MultiQueue, European Symposium on Algorithms (ESA), LIPIcs 351,
 *      85:1-85:14, 2025, doi:10.4230/LIPIcs.ESA.2025.85
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

#ifndef __ADS_MultiQueue
 #define __ADS_MultiQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/DaryHeap.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MultiQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a relaxed PRIORITY QUEUE shared by threads: \f$ n \f$ queues under locks,
/// the pop from the better of two
template< class K , class T , class Less = std::less< K > >
class MultiQueue {

 static_assert( std::numeric_limits< K >::is_specialized ,
                "the keys are integers or reals" );
 static_assert( std::atomic< K >::is_always_lock_free ,
                "the keys fit an atomic word without a lock" );

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Item = T;    ///< the type of the items

 /// a pair of the queue, its key and its item
 struct Entry {
  K key;    ///< the key, the least popped first
  T item;   ///< the item
  };

 class Handle;

 /// the structure of \f$ n \f$ empty queues, kept by a thread for
 /// \f$ s \f$ operations, with the draws from the seed `seed`;
 /// \f$ n = 2 p \f$ for \f$ p \f$ threads, \f$ n = 1 \f$ an exact queue
 /// \pre \f$ n \ge 1 \f$ and \f$ s \ge 1 \f$
 explicit MultiQueue( std::size_t n , unsigned s = 16 ,
                      std::uint64_t seed = 1 , Less less = Less() );

 /// no copy, since the threads share it by its address
 MultiQueue( const MultiQueue & ) = delete;
 /// no copy, since the threads share it by its address
 MultiQueue & operator=( const MultiQueue & ) = delete;

 /// the frees of the queues
 ~MultiQueue();

 /// a handle for one thread, with a generator of its own
 [[nodiscard]] Handle handle( void );

 /// the number of the queues
 [[nodiscard]] std::size_t queues( void ) const { return( f_n ); }

 /// the operations a thread keeps its two queues for
 [[nodiscard]] unsigned stickiness( void ) const { return( f_s ); }

 /// the number of the pairs, the sum of the counters of the queues, exact
 /// when no operation runs
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the buffers of each queue ordered and holding its least pairs, the
 /// atomic minima equal to them, and the counters to the pairs; under
 /// each lock in turn, with no operation running
 [[nodiscard]] bool consistent( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kBuf = 16;          ///< the pairs of a buffer

 /// a before b, by the keys
 struct ByKey {
  Less less;
  bool operator()( const Entry & a , const Entry & b ) const {
   return( less( a.key , b.key ) );
   }
  };

 /// a queue: its lock, its minimum, its counter, its two buffers and its
 /// heap, on lines of cache of its own
 struct alignas( 64 ) Queue {
  /// held while true
  std::atomic< bool > lock{ false };
  /// the first place of the deletion buffer in the ring D and its pairs,
  /// and the pairs of I
  std::uint8_t lo = 0 , nd = 0 , ni = 0;
  /// the key of the least pair, or the greatest key if empty
  std::atomic< K > top;
  /// the pairs, for size()
  std::atomic< std::size_t > count{ 0 };
  /// the rest of the pairs, its header on the line of the lock
  DaryHeap< Entry , ByKey , 8 > heap;
  /// the deletion buffer, a ring of D[ lo ] , D[ lo + 1 ] , ... modulo
  /// 16, sorted by decreasing keys: the greatest first, which leaves by
  /// lo, and the least last, which leaves by the end, so that neither
  /// moves the others
  alignas( 64 ) Entry D[ kBuf ];
  /// the insertion buffer, unsorted
  Entry I[ kBuf ];
  /// the empty queue, ordered by less
  explicit Queue( const ByKey & less , K none )
   : top( none ) , heap( less ) {}
  /// the place of the k-th pair of the deletion buffer, 0 the greatest
  [[nodiscard]] int at( int k ) const { return( ( lo + k ) & ( kBuf - 1 ) );
   }
  };

 /// the greatest key under less, the minimum of an empty queue
 [[nodiscard]] static K none( const Less & less );

 /// the lock of q if free at once
 [[nodiscard]] static bool tryLock( Queue & q );

 /// the lock of q, waiting for it
 static void lock( Queue & q );

 /// the lock of q released
 static void unlock( Queue & q );

 /// the pair e put into the locked queue q
 void put( Queue & q , Entry && e ) const;

 /// the least pair taken from the locked queue q, not empty
 Entry take( Queue & q ) const;

 /// the atomic minimum and the counter of the locked queue q set again
 void publish( Queue & q ) const;

 /// the buffers of the locked queue q as the invariant wants them
 [[nodiscard]] bool ordered( const Queue & q ) const;

 /// a pair taken from the first queue found not empty, from the queue r
 /// on, each locked in turn; nothing if all are empty
 std::optional< Entry > scan( std::size_t r );

 std::size_t f_n;                         ///< the number of the queues
 unsigned f_s;                            ///< the stickiness
 ByKey f_less;                            ///< the order of the pairs
 K f_none;                                ///< the minimum of an empty queue
 Queue * f_q;                             ///< the queues
 std::atomic< std::uint64_t > f_seed;     ///< the seeds of the handles

 };  // end( class MultiQueue )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MultiQueue::Handle ------------------------*/
/*--------------------------------------------------------------------------*/

/// the access of one thread to a `MultiQueue`: its draws and the two
/// queues it keeps
template< class K , class T , class Less >
class MultiQueue< K , T , Less >::Handle {

/*--------------------------------------------------------------------------*/
 public:

 /// puts the pair \f$ ( k , x ) \f$ into one of the two queues kept, or
 /// into the first free one of other draws
 ///
 /// In pseudocode (Rihani, Sanders and Dementiev, with the stickiness and
 /// the buffers of Williams, Sanders and Dementiev, beyond the book),
 /// \f$ D_j \f$ and \f$ I_j \f$ the buffers of the queue \f$ j \f$ and
 /// \f$ H_j \f$ its heap, \f$ b = 16 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( k , x ) \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad \textbf{if}\ \text{the two queues kept were used } s
 ///     \text{ times}\ \textbf{then}\ \mathrm{draw}() \cr
 /// 4 & \qquad j \gets \text{one of the two at random} \cr
 /// 5 & \qquad \textbf{if}\ \mathrm{tryLock}( j )\ \textbf{then break} \cr
 /// 6 & \qquad \mathrm{draw}()\quad \triangleright\ \text{two random}
 ///     \text{ queues, used } 0 \text{ times} \cr
 /// 7 & \quad \textbf{if}\ D_j \ne \emptyset\ \textbf{and}\ k < \max D_j
 ///     \ \textbf{then} \cr
 /// 8 & \qquad \textbf{if}\ |D_j| = b\ \textbf{then}\ \text{move } \max D_j
 ///     \text{ into } I_j \cr
 /// 9 & \qquad \text{insert } ( k , x ) \text{ into } D_j \text{, sorted} \cr
 /// 10 & \quad \textbf{else if}\ D_j = \emptyset \ \textbf{or}\ ( |D_j| < b
 ///     \ \textbf{and}\ I_j = \emptyset\ \textbf{and}\ H_j = \emptyset )
 ///     \ \textbf{then}\ \text{append } ( k , x ) \text{ to } D_j \cr
 /// 11 & \quad \textbf{else}\ \text{add } ( k , x ) \text{ to } I_j \cr
 /// 12 & \quad \textbf{if}\ |I_j| = b\ \textbf{then}\ \text{push the pairs}
 ///     \text{ of } I_j \text{ into } H_j;\ I_j \gets \emptyset \cr
 /// 13 & \quad \mathit{top}_j \gets \text{the key of } \min D_j;
 ///     \ \mathrm{unlock}( j )\quad \triangleright\ \text{release}
 /// \end{array}}
 /// \f]
 void push( const K & k , T x );

 /// the least pair of the better of the two queues kept, or of the first
 /// draw of two queues whose better one is free; nothing if a scan of all
 /// the queues found them empty
 ///
 /// In pseudocode (beyond the book), with the notation of `push()` above:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPop}() \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad \textbf{if}\ \text{the two queues kept were used } s
 ///     \text{ times}\ \textbf{then}\ \mathrm{draw}() \cr
 /// 4 & \qquad j \gets \text{the one of the two with the least }
 ///     \mathit{top}\quad \triangleright\ \text{read without locks} \cr
 /// 5 & \qquad \textbf{if}\ \mathit{top}_j = \infty\ \textbf{then return}
 ///     \ \mathrm{scan}() \cr
 /// 6 & \qquad \textbf{if}\ \mathrm{tryLock}( j )\ \textbf{then break} \cr
 /// 7 & \qquad \mathrm{draw}() \cr
 /// 8 & \quad \textbf{if}\ D_j = \emptyset\ \textbf{then}\ \mathrm{unlock}(
 ///     j );\ \textbf{return}\ \mathrm{scan}() \cr
 /// 9 & \quad e \gets \text{the least pair of } D_j \text{, removed} \cr
 /// 10 & \quad \textbf{if}\ D_j = \emptyset\ \textbf{then} \cr
 /// 11 & \qquad \text{push the pairs of } I_j \text{ into } H_j;
 ///     \ I_j \gets \emptyset \cr
 /// 12 & \qquad \text{move the } \min( b , |H_j| ) \text{ least pairs of }
 ///     H_j \text{ into } D_j \cr
 /// 13 & \quad \mathit{top}_j \gets \text{the key of } \min D_j \text{, or }
 ///     \infty;\ \mathrm{unlock}( j );\ \textbf{return}\ e \cr
 /// 14 & \textbf{function}\ \mathrm{scan}() \cr
 /// 15 & \quad \textbf{foreach}\ \text{queue } j \text{, from a random one}
 ///     \ \textbf{do} \cr
 /// 16 & \qquad \mathrm{lock}( j ) \cr
 /// 17 & \qquad \textbf{if}\ D_j \ne \emptyset\ \textbf{then}\ \text{take}
 ///     \text{ its least pair as above};\ \textbf{return}\ \text{it} \cr
 /// 18 & \qquad \mathrm{unlock}( j ) \cr
 /// 19 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 std::optional< Entry > tryPop( void );

/*--------------------------------------------------------------------------*/
 private:

 friend class MultiQueue;

 /// the handle of m, its draws from the seed
 Handle( MultiQueue & m , std::uint64_t seed )
  : f_m( &m ) , f_x( seed ) {}

 /// a random number of 64 bits (the splitmix64 of Steele et al.)
 std::uint64_t next( void );

 /// a random queue
 std::size_t any( void );

 /// two new queues to keep, used 0 times
 void draw( void );

 /// one more use of the queues kept, drawn again after f_s uses
 void use( void );

 MultiQueue * f_m;                        ///< the structure
 std::uint64_t f_x;                       ///< the state of the generator
 std::size_t f_a = 0 , f_b = 0;           ///< the two queues kept
 unsigned f_used = ~0u;                   ///< their uses so far

 };  // end( class MultiQueue::Handle )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/MultiQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultiQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MultiQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
