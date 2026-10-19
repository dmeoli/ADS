/*--------------------------------------------------------------------------*/
/*---------------------------- File MPSCQueue.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A queue that many producer threads fill and one consumer thread empties,
 * lock-free and unbounded, beyond the book: each producer has a lane of its
 * own, so that the producers never compete with each other, and a lane is
 * the queue of Lamport between its producer and the consumer [see
 * SPSCQueue.h] on a circular vector of places, each thread with a copy of
 * the index of the other, read again only when the vector looks full or
 * empty, the indices and the copies on lines of cache of their own. When
 * the vector is full the producer links a new one of twice the places and
 * goes on there, and the consumer, which finds the old one empty and its
 * successor linked, frees it and follows: a lane in a steady state runs
 * on one vector that stays in the caches, an item costs O( 1 ) time and
 * no allocation, and the vectors of a lane, \f$ C , 2 C , 4 C , \dots \f$
 * places from \f$ C = B \f$, hold at most twice the most items the lane
 * ever held at once, plus B.
 *
 * A producer is a handle that producer() returns, to be used by one thread
 * at a time; the items of a producer come out in the order in which it
 * pushed them, and the consumer takes the lanes in turn, draining the one
 * it is on before the next. The lanes live as long as the queue, also when
 * their producer is gone, its items still to be taken.
 *
 * The contracts [see Contract.h]: B >= 2 is a power of 2; only one thread
 * at a time calls the consumer's operations, and only one at a time pushes
 * through a producer. The operations are defined in MPSCQueue.tpp.
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
                "the first vector of a lane has 2^k places" );

 static constexpr std::size_t line = 64;   ///< a line of cache

 /// a circular vector of a lane, with the indices of the queue of
 /// Lamport and the link to the next vector, each on lines of its own
 struct Ring {
  T * place;                                ///< mask + 1 places
  std::size_t mask;
  alignas( 2 * line ) std::atomic< std::size_t > tail{ 0 };   ///< producer
  alignas( line ) std::size_t headSeen = 0; ///< the head it saw
  alignas( line ) std::atomic< std::size_t > head{ 0 };   ///< consumer
  alignas( line ) std::size_t tailSeen = 0; ///< the tail it saw
  alignas( line ) std::atomic< Ring * > next{ nullptr };   ///< when full
  explicit Ring( std::size_t c )
   : place( std::allocator< T >().allocate( c ) ) , mask( c - 1 ) {}
  ~Ring() { std::allocator< T >().deallocate( place , mask + 1 ); }
  };

 /// the lane of a producer: the vector it writes and the one the consumer
 /// reads, on lines of their own
 struct Lane {
  alignas( line ) Ring * in;                ///< the producer's vector
  alignas( line ) Ring * out;               ///< the consumer's vector
  Lane * nextLane = nullptr;                ///< in the list of the lanes
  Lane( void ) : in( new Ring( B ) ) , out( in ) {}
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
