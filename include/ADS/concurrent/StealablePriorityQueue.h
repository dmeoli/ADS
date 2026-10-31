/*--------------------------------------------------------------------------*/
/*---------------------- File StealablePriorityQueue.h ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The priority queue of one worker that the other workers steal from,
 * beyond the book, and the frontier of \f$ p \f$ such queues of a parallel
 * best-first search or branch-and-bound with work stealing. Each queue
 * holds pairs \f$ ( k , x ) \f$ of a key and an item, the least key first by
 * `less`, and its owner pops them in the exact order of the keys of the
 * pairs it holds, which a search by bounds needs for its limits on the
 * nodes and for a run that the same steals make the same; a thread that
 * has no work takes about half of the pairs of another queue (see
 * \ref theory_concurrent_spq). It is neither a relaxed queue shared by all
 * [see `MultiQueue.h`] nor a deque of tasks in the order of their pushes
 * [see `WorkStealingDeque.h`].
 *
 * The pairs of a queue are kept in two 4-ary heaps of the owner [see
 * `DaryHeap.h`], \f$ H_0 \f$ and \f$ H_1 \f$, which no other thread reads: a
 * push goes into the smaller of the two (into each in turn when they are as
 * large), and a pop takes the better of their two least pairs. The pairs
 * pushed one after the other, e.g., the children of a node, are so dealt
 * to the two heaps in turn, and each heap holds about half of the pairs of
 * every range of keys, an interleaved half. A thief that finds no pair to
 * claim raises a flag of the queue (`hungry`); the owner reads it, a load
 * without order, at its next operation, and then moves the heap whose least
 * pair is not the least of the queue, in \f$ O( 1 ) \f$, into the
 * <em>offer</em> of the queue, an atomic pointer to it, which a thief claims
 * by an exchange with null and makes its own. The owner keeps its least
 * pair, and so its next pop; when the two heaps differ in size by more than
 * a factor of 4 it first deals all the pairs again to two heaps of equal
 * sizes, in \f$ O( m ) \f$ for \f$ m \f$ pairs. Until a thief claims the
 * offer, its pairs still belong to the owner: a pop whose least local key is
 * not less than the least key of the offer takes the offer back by the same
 * exchange, so that whoever of the two exchanges first gets it all, and
 * the owner never pops a pair while a better one it owns waits in the
 * offer. The common case of the owner, a push or a pop with no thief
 * hungry, costs therefore no read-modify-write and no fence, since no
 * other thread can take a pair from its heaps (an owner that raced with
 * the thieves for each pair, as in a deque of work stealing, would need
 * one of the two at each operation, by the laws of order of Attiya et
 * al.); a steal costs the thief two exchanges at most, and the owner a
 * store of a pointer, or \f$ O( m ) \f$ when it deals the pairs again. The
 * price is that a thief waits for the next operation of the owner, which
 * `serve()` brings forward in a long computation of the owner.
 *
 * `StealingFrontier` holds the queues of \f$ p \f$ workers, one per call
 * \f$ f( i ) \f$ of the `run( p , f )` of an executor [see `Executor.h`],
 * and their termination: `pop( i )` returns the least pair of the queue of
 * the worker \f$ i \f$ or, when it is empty, the least pair of what it
 * steals from victims drawn at random, and nothing only when no worker holds
 * a pair, or after `stop()`. A counter \f$ C \f$ shared by the workers holds
 * the number of the workers that hold pairs or work on one, plus the
 * thieves that are trying a steal: a worker leaves it when its `pop()`
 * finds its queue and its offer empty, a thief enters it before the
 * exchange that claims an offer and leaves it if the exchange fails, so
 * that \f$ C = 0 \f$ only when no pair is left (see
 * \ref theory_concurrent_spq).
 *
 * The keys and the items must be copyable; a pair is moved or copied only
 * by its owner of the moment. The contracts [see `Contract.h`]: one thread
 * at a time, the owner, calls the operations of a queue but the `steal()`
 * that names it as the victim; a thief steals into its own queue, empty;
 * the worker \f$ i \f$ alone calls `push( i , ... )`, `pop( i )` and
 * `serve( i )` of a frontier, and pushes only between a `pop( i )` that
 * returned a pair and the next one (or before the run); the invariant of a
 * queue is that of its two heaps, checked by `consistent()`. The
 * operations are defined in `StealablePriorityQueue.tpp`.
 *
 * \see D. Hendler, N. Shavit, Non-blocking steal-half work queues, ACM
 *      Symposium on Principles of Distributed Computing (PODC), 280-289,
 *      2002, doi:10.1145/571825.571876
 * \see U. A. Acar, A. Charguéraud, M. Rainey, Scheduling parallel programs
 *      by work stealing with private deques, ACM SIGPLAN Symposium on
 *      Principles and Practice of Parallel Programming (PPoPP), 219-228,
 *      2013, doi:10.1145/2442516.2442538
 * \see H. Attiya, R. Guerraoui, D. Hendler, P. Kuznetsov, M. M. Michael,
 *      M. Vechev, Laws of order: expensive synchronization in concurrent
 *      algorithms cannot be eliminated, ACM SIGPLAN-SIGACT Symposium on
 *      Principles of Programming Languages (POPL), 487-498, 2011,
 *      doi:10.1145/1926385.1926442
 * \see M. Wimmer, F. Versaci, J. L. Träff, D. Cederman, P. Tsigas, Data
 *      structures for task-based priority scheduling, ACM SIGPLAN Symposium
 *      on Principles and Practice of Parallel Programming (PPoPP),
 *      379-380, 2014, doi:10.1145/2555243.2555278
 * \see A. Postnikova, N. Koval, G. Nadiradze, D. Alistarh, Multi-queues can
 *      be state-of-the-art priority schedulers, ACM SIGPLAN Symposium on
 *      Principles and Practice of Parallel Programming (PPoPP), 353-367,
 *      2022, doi:10.1145/3503221.3508432
 * \see S. Tschöke, R. Lüling, B. Monien, Solving the traveling salesman
 *      problem with a distributed branch-and-bound algorithm on a 1024
 *      processor network, International Parallel Processing Symposium
 *      (IPPS), 182-189, 1995, doi:10.1109/IPPS.1995.395930
 * \see R. M. Karp, Y. Zhang, Randomized parallel algorithms for backtrack
 *      search and branch-and-bound computation, Journal of the ACM 40(3),
 *      765-789, 1993, doi:10.1145/174130.174145
 * \see R. D. Blumofe, C. E. Leiserson, Scheduling multithreaded
 *      computations by work stealing, Journal of the ACM 46(5), 720-748,
 *      1999, doi:10.1145/324133.324234
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

#ifndef __ADS_StealablePriorityQueue
 #define __ADS_StealablePriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/Backoff.h"
#include "ADS/priority/DaryHeap.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

/// a function of the common path of the owner, inlined even where the
/// compiler would not
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_SPQ_INLINE inline __attribute__(( always_inline ))
#elif defined( _MSC_VER )
 #define ADS_SPQ_INLINE __forceinline
#else
 #define ADS_SPQ_INLINE inline
#endif

/// a function never inlined, the rare path out of the common one
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_SPQ_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_SPQ_NOINLINE __declspec( noinline )
#else
 #define ADS_SPQ_NOINLINE
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS StealablePriorityQueue -----------------------*/
/*--------------------------------------------------------------------------*/

/// the exact PRIORITY QUEUE of one owner, of which a thief takes about half
/// of the pairs, offered by the owner at its next operation
template< class K , class T , class Less = std::less< K > >
class StealablePriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Item = T;    ///< the type of the items

 /// a pair of the queue, its key and its item
 struct Entry {
  K key;    ///< the key, the least popped first
  T item;   ///< the item
  };

 /// the empty queue of the order `less`
 explicit StealablePriorityQueue( Less less = Less() );

 /// no copy, since the thieves share it by its address
 StealablePriorityQueue( const StealablePriorityQueue & ) = delete;
 /// no copy, since the thieves share it by its address
 StealablePriorityQueue & operator=( const StealablePriorityQueue & ) =
                                                                 delete;

 /// the free of the offer not claimed
 ~StealablePriorityQueue();

 /// pushes \f$ ( k , x ) \f$ into the smaller heap, \f$ O( \log m ) \f$,
 /// then answers a thief that asked
 ///
 /// In pseudocode (beyond the book), \f$ H_0 \f$ and \f$ H_1 \f$ the two
 /// heaps and \f$ \tau \f$ the turn of the pushes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( k , x ) \cr
 /// 2 & \quad \textbf{if}\ |H_0| \ne |H_1|\ \textbf{then}\ j \gets
 ///     \text{the smaller}\ \textbf{else}\ \tau \gets 1 - \tau;\ j \gets
 ///     \tau \cr
 /// 3 & \quad \mathrm{push}( H_j , ( k , x ) );\ \mathrm{serve}()
 /// \end{array}}
 /// \f]
 void push( K k , T x );

 /// the least pair of the queue, removed, nothing if it is empty, after
 /// answering a thief that asked; \f$ O( \log m ) \f$ but for the offer
 /// taken back
 ///
 /// In pseudocode (beyond the book), \f$ O \f$ the offer, \f$ k_O \f$ its
 /// least key and \f$ \mathit{offered} \f$ the knowledge of the owner that
 /// it made it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \mathrm{serve}();\ \mathrm{settle}() \cr
 /// 3 & \quad \textbf{if}\ H_0 = H_1 = \emptyset\ \textbf{then return nil}
 ///     \cr
 /// 4 & \quad \textbf{return}\ \mathrm{pop}( H_j ) \text{ for the } j
 ///     \text{ of the least top, } 0 \text{ on a tie} \cr
 /// 5 & \textbf{procedure}\ \mathrm{settle}() \cr
 /// 6 & \quad \textbf{if}\ \mathit{offered}\ \textbf{and}\ ( H_0 = H_1 =
 ///     \emptyset\ \textbf{or}\ k_O < \text{the least local key} )
 ///     \ \textbf{then} \cr
 /// 7 & \qquad G \gets \mathrm{exchange}( O , \textbf{nil} );
 ///     \ \mathit{offered} \gets \textbf{false}\quad \triangleright
 ///     \ \text{acquire and release} \cr
 /// 8 & \qquad \textbf{if}\ G \ne \textbf{nil}\ \textbf{then}\ \text{the}
 ///     \text{ pairs of } G \text{ and of the smaller heap into one heap}
 /// \end{array}}
 /// \f]
 std::optional< Entry > pop( void );

 /// the least pair of the queue, the offer taken back if it holds it
 /// \pre the queue is not empty
 const Entry & top( void );

 /// answers a thief that asked, offering about half of the pairs if there
 /// are at least two and no offer waits; \f$ O( 1 ) \f$, or \f$ O( m ) \f$
 /// when the pairs are dealt again, and a load without order when no thief
 /// asked
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{serve}() \cr
 /// 2 & \quad \textbf{if not}\ \mathit{hungry}\ \textbf{then return}
 ///     \quad \triangleright\ \text{relaxed} \cr
 /// 3 & \quad \textbf{if}\ \mathit{offered}\ \textbf{and}\ O \ne
 ///     \textbf{nil}\ \textbf{then return}\quad \triangleright\ \text{one}
 ///     \text{ offer at a time} \cr
 /// 4 & \quad \mathit{offered} \gets \textbf{false};\ \mathit{hungry}
 ///     \gets \textbf{false} \cr
 /// 5 & \quad \textbf{if}\ |H_0| + |H_1| < 2\ \textbf{then return} \cr
 /// 6 & \quad j \gets \text{the heap whose top is not the least};
 ///     \ m \gets |H_0| + |H_1| \cr
 /// 7 & \quad \textbf{if}\ 5 |H_j| < m\ \textbf{or}\ 5 |H_j| > 4 m
 ///     \ \textbf{then} \cr
 /// 8 & \qquad \text{deal the pairs of } H_0 \text{ and } H_1
 ///     \text{ in turn to two new heaps};\ j \gets \text{as at line } 6
 ///     \cr
 /// 9 & \quad k_O \gets \text{the key of } \mathrm{top}( H_j );\ O \gets
 ///     H_j;\ H_j \gets \emptyset;\ \mathit{offered} \gets \textbf{true}
 ///     \quad \triangleright\ \text{release}
 /// \end{array}}
 /// \f]
 void serve( void );

 /// steals into this queue, empty, the offer of the queue \f$ v \f$, or
 /// asks \f$ v \f$ for one and waits a little for it (as long as a
 /// `Backoff` spins); the number of the pairs stolen, 0 if none; called by
 /// the owner of this queue, a thief of \f$ v \f$
 ///
 /// In pseudocode (beyond the book), \f$ O_v \f$ the offer of \f$ v \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{steal}( v ) \cr
 /// 2 & \quad \textbf{if}\ O_v = \textbf{nil}\ \textbf{then} \cr
 /// 3 & \qquad \textbf{if}\ v \text{ holds less than two pairs}
 ///     \ \textbf{then return}\ 0\quad \triangleright\ \text{a hint of }
 ///     v \cr
 /// 4 & \qquad \mathit{hungry}_v \gets \textbf{true};\ \text{wait a little}
 ///     \text{ while } O_v = \textbf{nil} \cr
 /// 5 & \qquad \textbf{if}\ O_v = \textbf{nil}\ \textbf{then return}\ 0 \cr
 /// 6 & \quad G \gets \mathrm{exchange}( O_v , \textbf{nil} )\quad
 ///     \triangleright\ \text{acquire and release} \cr
 /// 7 & \quad \textbf{if}\ G = \textbf{nil}\ \textbf{then return}\ 0
 ///     \quad \triangleright\ \text{another thread was first} \cr
 /// 8 & \quad \text{deal the pairs of } G \text{ in turn to } H_0 \text{ and }
 ///     H_1;\ \textbf{return}\ |G|
 /// \end{array}}
 /// \f]
 /// \pre \f$ v \f$ is not this queue, and this queue is empty
 std::size_t steal( StealablePriorityQueue & v );

 /// the number of the pairs of the queue, those of the offer included
 /// while no thief has claimed it; for the owner
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// whether the queue offers pairs or holds at least two, a hint read
 /// with no order; any thread may call it, e.g., a thief that chooses its
 /// victim
 [[nodiscard]] bool stealable( void ) const {
  return( ( f_offer.load( std::memory_order_relaxed ) != nullptr ) ||
          f_canGive.load( std::memory_order_relaxed ) );
  }

 /// whether an offer of the owner waits for a thief, for the owner
 [[nodiscard]] bool offering( void ) const;

 /// the pairs that the two heaps hold, \f$ |H_0| \f$ and \f$ |H_1| \f$
 [[nodiscard]] std::pair< std::size_t , std::size_t > halves( void ) const {
  return( std::make_pair( f_h[ 0 ].size() , f_h[ 1 ].size() ) );
  }

 /// the two heaps ordered, and the hint of the thieves equal to their
 /// size; for the owner
 [[nodiscard]] bool consistent( void ) const;

 /// the calls `f( e )` on the pairs of the queue, removed, in increasing
 /// order of the keys, the offer taken back first; their number
 template< class F >
 std::size_t drain( F && f );

/*--------------------------------------------------------------------------*/
 private:

 /// a before b, by the keys
 struct ByKey {
  Less less;    ///< the order of the keys
  /// whether the key of a is less than that of b
  bool operator()( const Entry & a , const Entry & b ) const {
   return( less( a.key , b.key ) );
   }
  };

 /// a heap of the owner, the least pair on top
 using Heap = DaryHeap< Entry , ByKey , 4 >;

 /// the heap with the least top, 0 on a tie or if \f$ H_1 \f$ is empty
 [[nodiscard]] int best( void ) const;

 /// the offer taken back if it may hold the least pair
 void settle( void );

 /// the pairs of g joined to those of the smaller heap, the result in its
 /// place, with no change if a copy throws
 void merge( Heap & g );

 /// the pairs of the two heaps dealt in turn to two new ones
 void deal( void );

 /// the hint of the thieves set again, if changed
 void hint( void );

 /// the sizes of the two heaps counted again, and the hint
 void recount( void );

 /// the answer to a thief that asked, the body of `serve()`
 void answer( void );

 /// the hint of the thieves equal to the size of the heaps, the invariant
 /// checked after each operation (the order of the heaps is checked by
 /// their own operations)
 [[nodiscard]] bool hinted( void ) const;

 static constexpr std::size_t line = 64;   ///< a line of cache

 // of the owner alone
 Heap f_h[ 2 ];                            ///< the two heaps
 ByKey f_less;                             ///< the order of the pairs
 std::optional< K > f_offerKey;            ///< the least key of the offer
 std::size_t f_offerSize = 0;              ///< the pairs of the offer
 std::size_t f_n[ 2 ] = { 0 , 0 };        ///< the sizes of the two heaps
 bool f_offered = false;                   ///< an offer was made
 bool f_can = false;                       ///< the last hint
 unsigned char f_turn = 1;                 ///< the turn of the pushes

 // shared with the thieves, on lines of their own (the alignment of the
 // first rounds the size of the queue up to a multiple of two lines)
 alignas( 2 * line ) std::atomic< Heap * > f_offer{ nullptr }; ///< offer
 std::atomic< bool > f_hungry{ false };    ///< a thief asked
 std::atomic< bool > f_canGive{ false };   ///< at least two pairs, a hint

 };  // end( class StealablePriorityQueue )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS StealingFrontier --------------------------*/
/*--------------------------------------------------------------------------*/

/// the frontier of a best-first search of \f$ p \f$ workers, a
/// `StealablePriorityQueue` each, with the steals from random victims and
/// the detection of the termination
template< class K , class T , class Less = std::less< K > >
class StealingFrontier {

/*--------------------------------------------------------------------------*/
 public:

 /// the queue of a worker
 using Queue = StealablePriorityQueue< K , T , Less >;
 /// a pair of a queue
 using Entry = typename Queue::Entry;

 /// the frontier of \f$ p \f$ empty queues, the victims drawn from the
 /// seed `seed`
 /// \pre \f$ p \ge 1 \f$
 explicit StealingFrontier( std::size_t p , std::uint64_t seed = 1 ,
                            Less less = Less() );

 /// no copy, since the workers share it by its address
 StealingFrontier( const StealingFrontier & ) = delete;
 /// no copy, since the workers share it by its address
 StealingFrontier & operator=( const StealingFrontier & ) = delete;

 /// the frees of the queues and of their offers
 ~StealingFrontier();

 /// the number of the workers, \f$ p \f$
 [[nodiscard]] std::size_t workers( void ) const { return( f_p ); }

 /// the queue of the worker \f$ i \f$
 /// \pre \f$ i < p \f$
 [[nodiscard]] Queue & queue( std::size_t i ) {
  ADS_PRE( i < workers() );
  return( f_w[ i ].q );
  }

 /// pushes \f$ ( k , x ) \f$ into the queue of the worker \f$ i \f$
 /// \pre \f$ i < p \f$
 void push( std::size_t i , K k , T x ) {
  ADS_PRE( i < workers() );
  f_w[ i ].q.push( std::move( k ) , std::move( x ) );
  }

 /// lets the worker \f$ i \f$ answer a thief in a long computation
 /// \pre \f$ i < p \f$
 void serve( std::size_t i ) {
  ADS_PRE( i < workers() );
  f_w[ i ].q.serve();
  }

 /// the least pair of the queue of the worker \f$ i \f$, or of what it
 /// steals when its queue is empty; nothing when no worker holds a pair, or
 /// after `stop()`; the call ends the work of \f$ i \f$ on the pair that
 /// its previous call returned
 ///
 /// In pseudocode (beyond the book), \f$ C \f$ the counter of the workers
 /// at work and of the thieves trying, \f$ p \f$ at the start:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}( i ) \cr
 /// 2 & \quad e \gets \mathrm{pop}( Q_i );\ \textbf{if}\ e \ne
 ///     \textbf{nil}\ \textbf{then return}\ e \cr
 /// 3 & \quad C \gets C - 1\quad \triangleright\ i \text{ holds nothing}
 ///     \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad \textbf{if}\ \text{stopped}\ \textbf{or}\ C = 0
 ///     \ \textbf{then return nil} \cr
 /// 6 & \qquad v \gets \text{a worker} \ne i \text{ at random} \cr
 /// 7 & \qquad \textbf{if}\ v \text{ offers, or holds two pairs at least}
 ///     \ \textbf{then} \cr
 /// 8 & \qquad\quad C \gets C + 1 \cr
 /// 9 & \qquad\quad \textbf{if}\ \mathrm{steal}( Q_i , Q_v ) > 0
 ///     \ \textbf{then return}\ \mathrm{pop}( Q_i ) \cr
 /// 10 & \qquad\quad C \gets C - 1 \cr
 /// 11 & \qquad \text{back off}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < p \f$
 std::optional< Entry > pop( std::size_t i );

 /// ends the computation: from now on `pop()` returns nothing to every
 /// worker; any thread may call it
 void stop( void ) { f_stop.store( true , std::memory_order_release ); }

 /// whether `stop()` was called
 [[nodiscard]] bool stopped( void ) const {
  return( f_stop.load( std::memory_order_acquire ) );
  }

 /// whether no worker holds a pair, i.e., \f$ C = 0 \f$
 [[nodiscard]] bool terminated( void ) const {
  return( f_active.load( std::memory_order_acquire ) == 0 );
  }

 /// the steals that succeeded, summed over the workers, exact when no
 /// worker runs
 [[nodiscard]] std::size_t steals( void ) const;

 /// the calls \f$ f( i ) \f$ of the \f$ p \f$ workers by `ex.run( p , f )`;
 /// the first exception of a call stops the others, through `stop()`, and
 /// is thrown again once all have returned
 /// \pre the executor runs \f$ p \f$ workers at the same time
 template< class E , class F >
 void run( E & ex , F && f );

 /// the calls `f( e )` on the pairs left in the queues, each taken back
 /// from its offer first, in increasing order of the keys in each queue;
 /// their number
 /// \pre no worker calls the frontier
 template< class F >
 std::size_t leftovers( F && f );

/*--------------------------------------------------------------------------*/
 private:

 /// a worker: its queue, its generator and its steals, on lines of their
 /// own
 struct alignas( 128 ) Worker {
  Queue q;                     ///< the queue
  std::uint64_t x;             ///< the state of the generator of victims
  std::atomic< std::size_t > steals{ 0 };   ///< the steals that succeeded
  /// the worker of the order less, its draws from the seed
  Worker( const Less & less , std::uint64_t seed ) : q( less ) , x( seed ) {}
  };

 /// a random number of 64 bits for the worker w (the splitmix64 of Steele
 /// et al.)
 static std::uint64_t next( Worker & w );

 /// the rest of `pop( i )` when the queue of \f$ i \f$ is empty, its offer
 /// taken back or claimed: the steals until the end
 std::optional< Entry > idle( std::size_t i );

 std::size_t f_p;                          ///< the number of the workers
 Worker * f_w;                             ///< the workers, in a row
 alignas( 128 ) std::atomic< std::int64_t > f_active;  ///< the counter C
 alignas( 128 ) std::atomic< bool > f_stop{ false };   ///< the stop

 };  // end( class StealingFrontier )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/StealablePriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StealablePriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------- End File StealablePriorityQueue.h --------------------*/
/*--------------------------------------------------------------------------*/
