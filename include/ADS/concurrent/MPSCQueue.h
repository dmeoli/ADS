/*--------------------------------------------------------------------------*/
/*---------------------------- File MPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A queue that many producer threads fill and one consumer thread empties,
 * lock-free and unbounded, beyond the book: each producer has a lane of its
 * own, an unbounded queue of blocks of B places between it and the
 * consumer (the queue of Lamport [see SPSCQueue.h] on a list of blocks),
 * so that the producers never compete with each other, and an item costs
 * no allocation: a producer writes the item into the next place of its
 * last block and publishes, by one store with the release order, how many
 * places of the block are written; when the block is full it links a new
 * one, taken from the block that the consumer gave back last if there is
 * one. The consumer reads the lanes in turn, each from its first block,
 * with its own copy of the count of the places written, read again only
 * when it has taken them all, and gives a block back when it has taken
 * its B items and the producer has linked the next one.
 *
 * A producer is a handle that producer() returns, to be used by one thread
 * at a time; the items of a producer come out in the order in which it
 * pushed them, and the consumer takes the lanes in turn, draining the one
 * it is on before the next. An item costs O( 1 ) time, a block of B items
 * one allocation at most, and the memory is that of the items in the
 * queue plus at most 2 blocks per lane, \f$ ( B + 2 ) \f$ places a lane in
 * the steady state. The lanes live as long as the queue, also when their
 * producer is gone, its items still to be taken.
 *
 * The contracts [see Contract.h]: B >= 2; only one thread at a time calls
 * the consumer's operations, and only one at a time pushes through a
 * producer. The operations are defined in MPSCQueue.tpp.
 *
 * \see L. Lamport, Specifying concurrent program modules, ACM Transactions
 *      on Programming Languages and Systems 5(2), 190-222, 1983,
 *      doi:10.1145/69624.357207
 * \see P. P. C. Lee, T. Bu, G. Chandranmenon, A lock-free, cache-efficient
 *      multi-core synchronization mechanism for line-rate network traffic
 *      monitoring, IEEE International Symposium on Parallel and
 *      Distributed Processing (IPDPS), 1-12, 2010,
 *      doi:10.1109/IPDPS.2010.5470368 (the copy of the count)
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986 (the
 *      list of the lanes, which only grows)
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

#ifndef __ADS_MPSCQueue
 #define __ADS_MPSCQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <cstddef>
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
template< class T , std::size_t B = 512 >
class MPSCQueue {
 static_assert( B >= 2 , "a block has two places at least" );

 static constexpr std::size_t line = 64;   ///< a line of cache

 /// a block of B places, the count of those written and the next block,
 /// which the producer writes, on a line of their own
 struct Block {
  alignas( line ) std::atomic< std::size_t > written{ 0 };
  std::atomic< Block * > next{ nullptr };
  alignas( line ) alignas( T ) unsigned char place[ B * sizeof( T ) ];

  T * at( std::size_t i ) {
   return( std::launder( reinterpret_cast< T * >( place ) ) + i );
   }
  };

 /// the lane of a producer: what the producer writes, what the consumer
 /// writes, and the block given back, each on lines of its own
 struct Lane {
  alignas( line ) Block * tail;             ///< the producer's block
  std::size_t tailAt = 0;                   ///< its next place
  alignas( line ) Block * head;             ///< the consumer's block
  std::size_t headAt = 0;                   ///< its next place
  std::size_t seen = 0;                     ///< its count, as last read
  alignas( line ) std::atomic< Block * > spare{ nullptr };   ///< given back
  Lane * nextLane = nullptr;                ///< in the list of the lanes
  Lane( void ) : tail( new Block ) , head( tail ) {}
  };

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;   ///< the type of the items

 /// the handle of a producer, its lane: one thread at a time pushes
 class Producer {
  public:
  Producer( void ) = default;
  Producer( Producer && o ) noexcept
   : f_lane( std::exchange( o.f_lane , nullptr ) ) {}
  Producer & operator=( Producer && o ) noexcept {
   f_lane = std::exchange( o.f_lane , nullptr );
   return( *this );
   }

  /// enqueues the item built from args
  /// \pre the handle comes from producer()
  template< class ... Args >
  void emplace( Args && ... args );

  void push( const T & v ) { emplace( v ); }
  void push( T && v ) { emplace( std::move( v ) ); }

  private:
  friend class MPSCQueue;
  explicit Producer( Lane * l ) : f_lane( l ) {}
  Lane * f_lane = nullptr;
  };  // end( class Producer )

 MPSCQueue( void ) = default;
 MPSCQueue( const MPSCQueue & ) = delete;
 MPSCQueue & operator=( const MPSCQueue & ) = delete;

 /// destroys the items left, the blocks and the lanes
 ~MPSCQueue();

 /// a new producer, with a lane of its own; any thread may call it
 [[nodiscard]] Producer producer( void );

 /// dequeues an item into x, false if none was found in a turn of the
 /// lanes; only the consumer calls it
 bool tryPop( T & x );

 /// dequeues up to n items into out, those of a lane before the next
 /// one, and returns how many; only the consumer calls it
 std::size_t popInto( T * out , std::size_t n );

 /// true if no item was found in a turn of the lanes; only the consumer
 /// calls it
 [[nodiscard]] bool empty( void );

/*--------------------------------------------------------------------------*/
 private:
 /// the items of the lane l, up to n, into out; their number
 std::size_t drain( Lane * l , T * out , std::size_t n );

 std::atomic< Lane * > f_lanes{ nullptr };   ///< the list of the lanes
 Lane * f_at = nullptr;                    ///< the consumer's lane
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
