/*--------------------------------------------------------------------------*/
/*---------------------------- File MPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A queue that many producer threads fill and one consumer thread empties,
 * lock-free and unbounded, beyond the book: each producer has a lane of its
 * own, so that the producers never compete with each other, and a lane is a
 * list of blocks of \f$ B \f$ places between its producer and the consumer.
 * The producer writes the places of its last block in order and publishes,
 * after each item, their count in the block (a store, no read-modify-write
 * operation); when the block is full it links a new one after it. The
 * consumer reads the count of its first block once for all the items it
 * shows, takes them, and when it has taken the \f$ B \f$ items of a block
 * and finds the next one linked it gives the block back to its lane, on a
 * stack that the producer empties at once (an exchange) when it needs a
 * block, before it allocates one. A lane in a steady state therefore cycles
 * over the same blocks, which stay in the caches: an item costs
 * \f$ O( 1 ) \f$ time, a store of the count and no allocation, the count
 * and the link of a block share a line that only the producer writes, and
 * the consumer touches the lines of the producer once per batch; a lane
 * keeps \f$ \lceil m / B \rceil + 1 \f$ blocks for the most items \f$ m \f$
 * it ever held at once.
 *
 * A consumer that is right behind its producer and looks again at once
 * would take from it, at each item, the lines of the count and of the
 * items it is writing, and slow it down several times over: when a call
 * took some items and the next one finds fewer than \f$ \min( B , 64 ) \f$
 * in the block, the consumer first waits 64 pauses of the processor for a
 * batch (the backtracking of B-Queue), while after a call that took none
 * it takes at once what it finds, so that a single item after a silence
 * costs no wait. A producer is a handle that `producer()` returns, to be
 * used by one thread at a time; the items of a producer come out in the
 * order in which it pushed them, and the consumer takes the lanes in turn,
 * draining the one it is on before the next. The lanes live as long as the
 * queue, also when their producer is gone, its items still to be taken.
 *
 * The contracts [see `Contract.h`]: \f$ B \ge 2 \f$ is a power of 2; only
 * one thread at a time calls the operations of the consumer, and only one
 * at a time pushes through a producer. The operations are defined in
 * `MPSCQueue.tpp`.
 *
 * \see L. Lamport, Specifying concurrent program modules, ACM Transactions
 *      on Programming Languages and Systems 5(2), 190-222, 1983,
 *      doi:10.1145/69624.357207
 * \see M. Aldinucci, M. Danelutto, P. Kilpatrick, M. Meneghin, M.
 *      Torquati, An efficient unbounded lock-free queue for multi-core
 *      systems, Euro-Par 2012, LNCS 7484, 662-673, 2012,
 *      doi:10.1007/978-3-642-32820-6_65 (the list of bounded buffers and
 *      the pool that recycles them)
 * \see J. Wang, K. Zhang, X. Tang, B. Hua, B-Queue: efficient and
 *      practical queuing for fast core-to-core communication,
 *      International Journal of Parallel Programming 41(1), 137-159, 2013,
 *      doi:10.1007/s10766-012-0213-x (the wait for a batch)
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986 (the
 *      stacks of the lanes and of the given back blocks)
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_MPSCQueue
 #define __ADS_MPSCQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/EventCount.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPSCQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an unbounded queue from many producers to one consumer, a lane each
template< class T , std::size_t B = 1024 >
class MPSCQueue {
 static_assert( ( B >= 2 ) && ( ( B & ( B - 1 ) ) == 0 ) ,
                "a block has 2^k places" );

 static constexpr std::size_t line = 64;   ///< a line of cache
 /// the batch below which a consumer that is streaming waits, and the
 /// pauses of the processor of the wait
 static constexpr std::size_t want = B < 64 ? B : 64;
 static constexpr int pauses = 64;

 /// a block of B places, with the count of the items written and the
 /// link to the next block on a line that only the producer writes
 struct Block {
  /// the number of the items written, published with a release
  alignas( 2 * line ) std::atomic< std::size_t > filled{ 0 };
  /// the next block of the lane
  std::atomic< Block * > next{ nullptr };
  Block * spare = nullptr;                  ///< in the stacks of the lane
  /// the room of the items
  alignas( 2 * line ) unsigned char raw[ B * sizeof( T ) ];
  /// the item \f$ i \f$
  [[nodiscard]] T * at( std::size_t i ) {
   return( std::launder( reinterpret_cast< T * >( raw ) ) + i );
   }
  /// the room of the item \f$ i \f$
  [[nodiscard]] void * slot( std::size_t i ) {
   return( raw + i * sizeof( T ) );
   }
  };

 /// the lane of a producer: its block and place, the consumer's block
 /// and place, the blocks the consumer gave back, each on lines of its own
 struct Lane {
  alignas( 2 * line ) Block * in;           ///< the producer's block
  std::size_t pos = 0;                      ///< its next place
  Block * spare = nullptr;                  ///< its blocks to reuse
  alignas( 2 * line ) Block * out;          ///< the consumer's block
  std::size_t at = 0;                       ///< its next place
  bool streaming = false;                   ///< its last call took some
  /// the blocks that the consumer gave back
  alignas( 2 * line ) std::atomic< Block * > given{ nullptr };
  Lane * nextLane = nullptr;                ///< in the list of the lanes
  EventCount * wait;                        ///< the sleep of the consumer
  /// the lane of one empty block, the sleep of its consumer in \f$ w \f$
  explicit Lane( EventCount * w ) : in( new Block ) , out( in ) ,
                                    wait( w ) {}
  };

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;   ///< the type of the items

 /// the handle of a producer, its lane: one thread at a time pushes
 class Producer {
  public:
  /// a handle of no lane, to be assigned one from `producer()`
  Producer( void ) = default;

  /// the lane of \f$ o \f$, which is left with none
  Producer( Producer && o ) noexcept
   : f_lane( std::exchange( o.f_lane , nullptr ) ) {}

  /// the lane of \f$ o \f$, which is left with none
  Producer & operator=( Producer && o ) noexcept {
   f_lane = std::exchange( o.f_lane , nullptr );
   return( *this );
   }

  /// enqueues the item built from `args` in the lane of the handle
  ///
  /// In pseudocode (the list of bounded buffers of Aldinucci et al., beyond
  /// the book), \f$ b \f$ the last block of the lane, \f$ b.f \f$ the count
  /// of its items:
  /// \f[
  /// \class{pseudocode}{\begin{array}{r@{\quad}l}
  /// 1 & \textbf{procedure}\ \mathrm{emplace}( x ) \cr
  /// 2 & \quad p \gets b.f\quad \triangleright\ \text{relaxed, only this}
  ///     \text{ thread writes it} \cr
  /// 3 & \quad \textbf{if}\ p = B\ \textbf{then} \cr
  /// 4 & \qquad n \gets \text{a block of the spare stack, else of the}
  ///     \text{ stack given back, taken whole by an exchange, else a new}
  ///     \text{ block} \cr
  /// 5 & \qquad n.f \gets 0;\ b.\mathit{next} \gets n\quad \triangleright
  ///     \ \text{release};\ b \gets n;\ p \gets 0 \cr
  /// 6 & \quad b[ p ] \gets x \cr
  /// 7 & \quad b.f \gets p + 1\quad \triangleright\ \text{release: the item}
  ///     \text{ before its count}
  /// \end{array}}
  /// \f]
  /// \pre the handle comes from `producer()`
  template< class ... Args >
  void emplace( Args && ... args );

  /// enqueues a copy of \f$ v \f$, as `emplace()`
  void push( const T & v ) { emplace( v ); }

  /// enqueues \f$ v \f$, moved, as `emplace()`
  void push( T && v ) { emplace( std::move( v ) ); }

  /// wakes the consumer if it sleeps in `waitPop()` or `waitPopInto()`,
  /// after pushes: a fence and the read of a line when it does not
  /// \pre the handle comes from `producer()`
  void notify( void ) {
   ADS_PRE( f_lane != nullptr );
   f_lane->wait->notify();
   }

  private:
  friend class MPSCQueue;
  explicit Producer( Lane * l ) : f_lane( l ) {}
  Lane * f_lane = nullptr;
  };  // end( class Producer )

 /// the queue with no lane
 MPSCQueue( void ) = default;
 /// no copy, since the producers hold its lanes
 MPSCQueue( const MPSCQueue & ) = delete;
 /// no copy, since the producers hold its lanes
 MPSCQueue & operator=( const MPSCQueue & ) = delete;

 /// destroys the items left, the blocks and the lanes
 ~MPSCQueue();

 /// a new producer, with a lane of its own; any thread may call it
 ///
 /// In pseudocode (the stack of Treiber, beyond the book), \f$ L \f$ the
 /// head of the list of the lanes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{producer}() \cr
 /// 2 & \quad \ell \gets \text{a new lane with one empty block} \cr
 /// 3 & \quad \textbf{repeat}\ \ell.\mathit{next} \gets L\ \textbf{until}
 ///     \ \mathrm{CAS}( L , \ell.\mathit{next} , \ell )\quad \triangleright
 ///     \ \text{release} \cr
 /// 4 & \quad \textbf{return}\ \ell
 /// \end{array}}
 /// \f]
 [[nodiscard]] Producer producer( void );

 /// dequeues an item into \f$ x \f$, false if none was found in a turn of
 /// the lanes; only the consumer calls it
 ///
 /// It is `popInto()` below, for one item.
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPop}( x ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{popInto}( x , 1 ) = 1
 /// \end{array}}
 /// \f]
 bool tryPop( T & x );

 /// dequeues up to \f$ n \f$ items into `out`, those of a lane before the
 /// next one, and returns how many; only the consumer calls it
 ///
 /// In pseudocode (the list of bounded buffers of Aldinucci et al. and the
 /// wait for a batch of B-Queue, beyond the book), the lanes taken in turn
 /// from the one of the last call, \f$ b \f$ and \f$ h \f$ the block and the
 /// place of the consumer in the lane \f$ \ell \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popInto}( \mathit{out} , n ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{lane } \ell \text{ in turn, from the}
 ///     \text{ last one drained}\ \textbf{do} \cr
 /// 3 & \qquad k \gets 0 \cr
 /// 4 & \qquad \textbf{while}\ k < n\ \textbf{do} \cr
 /// 5 & \qquad\quad \textbf{if}\ h = B\ \textbf{then} \cr
 /// 6 & \qquad\qquad \textbf{if}\ b.\mathit{next} = \textbf{nil}
 ///     \ \textbf{then break}\quad \triangleright\ \text{acquire} \cr
 /// 7 & \qquad\qquad \text{push } b \text{ on the stack given back of }
 ///     \ell \text{ by a CAS};\ b \gets b.\mathit{next};\ h \gets 0 \cr
 /// 8 & \qquad\quad f \gets b.f\quad \triangleright\ \text{acquire} \cr
 /// 9 & \qquad\quad \textbf{if}\ f - h < \min( B , 64 )\ \textbf{and}\ f <
 ///     B\ \textbf{and}\ \ell.\mathit{streaming}\ \textbf{then} \cr
 /// 10 & \qquad\qquad \text{wait 64 pauses};\ f \gets b.f \cr
 /// 11 & \qquad\quad \textbf{if}\ h = \min( f , h + n - k )
 ///     \ \textbf{then break} \cr
 /// 12 & \qquad\quad \textbf{while}\ h < \min( f , h + n - k )
 ///     \ \textbf{do} \cr
 /// 13 & \qquad\qquad \mathit{out}[ k ] \gets b[ h ];\ h \gets h + 1;\ k
 ///     \gets k + 1 \cr
 /// 14 & \qquad \ell.\mathit{streaming} \gets k > 0 \cr
 /// 15 & \qquad \textbf{if}\ k > 0\ \textbf{then return}\ k \cr
 /// 16 & \quad \textbf{return}\ 0
 /// \end{array}}
 /// \f]
 std::size_t popInto( T * out , std::size_t n );

 /// dequeues an item into \f$ x \f$, sleeping until one comes [see
 /// `EventCount.h`], the producers calling `notify()` after their pushes;
 /// only the consumer calls it
 ///
 /// In pseudocode (beyond the book), on the wait of `EventCount`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{waitPop}( x ) \cr
 /// 2 & \quad \mathrm{await}( \mathrm{tryPop}( x ) )
 /// \end{array}}
 /// \f]
 void waitPop( T & x ) { f_wait.await( [ & ] { return( tryPop( x ) ); } ); }

 /// the items of `popInto()`, sleeping until at least one comes
 /// \pre \f$ n \ge 1 \f$
 std::size_t waitPopInto( T * out , std::size_t n ) {
  ADS_PRE( n >= 1 );
  std::size_t k = 0;
  f_wait.await( [ & ] { return( ( k = popInto( out , n ) ) > 0 ); } );
  return( k );
  }

 /// true if no item was found in a turn of the lanes; only the consumer
 /// calls it
 [[nodiscard]] bool empty( void );

/*--------------------------------------------------------------------------*/
 private:
 /// the items of the lane l, up to n, into out; their number
 std::size_t drain( Lane * l , T * out , std::size_t n );

 /// gives the block b, all taken, back to the lane l
 static void giveBack( Lane * l , Block * b );

 /// the wait of a consumer for a batch
 static void relax( void );

 std::atomic< Lane * > f_lanes{ nullptr };   ///< the list of the lanes
 Lane * f_at = nullptr;                    ///< the consumer's lane
 EventCount f_wait;                        ///< the sleep of the consumer
 };  // end( class MPSCQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/MPSCQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MPSCQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MPSCQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
