/*--------------------------------------------------------------------------*/
/*------------------------ File WorkStealingDeque.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The deque of work stealing of Chase and Lev, beyond the book: its owner
 * thread pushes and takes at the bottom, as on a stack, and any other
 * thread steals from the top, lock-free, the items on a circular vector of
 * \f$ 2^k \f$ places that the owner doubles when it is full. The bottom b
 * and the top t are integers that only grow (b only by the owner, t by a
 * compare-and-swap), the items are those of \f$ [ t , b ) \f$, at the
 * places of their indices modulo the size; push() and take() cost O( 1 )
 * and no read-modify-write but when they reach the last item, steal()
 * one compare-and-swap, and a doubling O( b - t ) amortized over the pushes
 * that filled the vector.
 * The orders of memory are those of Lê, Pop, Cohen and Zappa Nardelli, who
 * proved the deque correct in the model of C11, with their two fences in
 * take() and steal() written as sequentially consistent operations on the
 * bottom and the top, which the sanitizer of threads models (it does not
 * model the fences): take() writes the bottom b - 1 and then reads the
 * top, steal() reads the top and then the bottom, all four in the single
 * total order of those operations, so that when the deque holds one item
 * the owner and a thief cannot both miss each other, and the one that loses
 * the compare-and-swap on the top gives the item up. The item is read
 * before the compare-and-swap that claims it, by a thief from the vector
 * it read after the bottom, which the owner publishes with a release; the
 * vectors left by a doubling stay until the deque is destroyed, since a
 * thief may still be reading one (they add up to less than the last one).
 * The items are kept in atomics of relaxed order, so T is trivially
 * copyable, e.g., an index or a pointer to a task.
 * The contracts [see Contract.h]: only the owner, one thread at a time,
 * calls push() and take(); the initial capacity is a power of 2, at least
 * 2.
 * \see D. Chase, Y. Lev, Dynamic circular work-stealing deque, ACM
 *      Symposium on Parallelism in Algorithms and Architectures (SPAA),
 *      21-28, 2005, doi:10.1145/1073970.1073974
 * \see N. M. Lê, A. Pop, A. Cohen, F. Zappa Nardelli, Correct and
 *      efficient work-stealing for weak memory models, ACM SIGPLAN
 *      Symposium on Principles and Practice of Parallel Programming
 *      (PPoPP), 69-80, 2013, doi:10.1145/2442516.2442524
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_WorkStealingDeque
 #define __ADS_WorkStealingDeque
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS WorkStealingDeque -------------------------*/
/*--------------------------------------------------------------------------*/

/// the deque of an owner that pushes and takes, and of thieves that steal
template< class T >
class WorkStealingDeque {
 static_assert( std::is_trivially_copyable_v< T > ,
                "the items are copied through atomics" );

 /// a circular vector of 2^k places
 struct Ring {
  std::int64_t mask;
  std::unique_ptr< std::atomic< T >[] > place;
  explicit Ring( std::int64_t c )
   : mask( c - 1 ) , place( new std::atomic< T >[ std::size_t( c ) ] ) {}
  [[nodiscard]] T get( std::int64_t i ) const {
   return( place[ std::size_t( i & mask ) ].load(
                                            std::memory_order_relaxed ) );
   }
  void put( std::int64_t i , T x ) {
   place[ std::size_t( i & mask ) ].store( x , std::memory_order_relaxed );
   }
  };

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// an empty deque with a vector of the given capacity
 /// \pre capacity >= 2 is a power of 2
 explicit WorkStealingDeque( std::size_t capacity = 1024 );
 WorkStealingDeque( const WorkStealingDeque & ) = delete;
 WorkStealingDeque & operator=( const WorkStealingDeque & ) = delete;

 /// pushes x at the bottom, doubling the vector if it is full; only the
 /// owner calls it
 void push( T x );

 /// takes the item at the bottom, the last pushed, nothing if the deque is
 /// empty or a thief took the last item first; only the owner calls it
 std::optional< T > take( void );

 /// steals the item at the top, the first pushed, into x; false if the
 /// deque is empty or another thread took that item first (and then a
 /// thief may try again); any thread may call it
 bool steal( T & x );

 /// the number of items at some moment of the call, for the owner exact
 [[nodiscard]] std::size_t size( void ) const;

 /// true if the deque held no item at some moment of the call
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the places of the current vector; only the owner calls it
 [[nodiscard]] std::size_t capacity( void ) const {
  return( std::size_t( f_ring.load( std::memory_order_relaxed )->mask +
                       1 ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t line = 64;   ///< a line of cache

 alignas( 2 * line ) std::atomic< std::int64_t > f_top{ 0 };   ///< thieves
 alignas( 2 * line ) std::atomic< std::int64_t > f_bottom{ 0 };   ///< owner
 std::atomic< Ring * > f_ring;             ///< the current vector
 std::vector< std::unique_ptr< Ring > > f_rings;   ///< all, for the owner
 };  // end( class WorkStealingDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/WorkStealingDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WorkStealingDeque.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File WorkStealingDeque.h ----------------------*/
/*--------------------------------------------------------------------------*/
