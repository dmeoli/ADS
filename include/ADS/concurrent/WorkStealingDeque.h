/*--------------------------------------------------------------------------*/
/*------------------------ File WorkStealingDeque.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The deque of work stealing of Chase and Lev, beyond the book: its owner
 * thread pushes and takes at the bottom, as on a stack, and any other thread
 * steals from the top, lock-free, the items on a circular vector of
 * \f$ 2^k \f$ places that the owner doubles when it is full. The bottom
 * \f$ b \f$ and the top \f$ t \f$ are integers that only grow (\f$ b \f$
 * only by the owner, \f$ t \f$ by a compare-and-swap), the items are those
 * of \f$ [ t , b ) \f$, at the places of their indices modulo the size;
 * `push()` and `take()` cost \f$ O( 1 ) \f$ and no read-modify-write but
 * when they reach the last item, `steal()` one compare-and-swap, and a
 * doubling \f$ O( b - t ) \f$, amortized over the pushes that filled the
 * vector.
 *
 * The orders of memory are those of Lê, Pop, Cohen and Zappa Nardelli, who
 * proved the deque correct in the model of C11, with their two fences in
 * `take()` and `steal()` written as sequentially consistent operations on
 * the bottom and the top, which the sanitizer of threads models (it does
 * not model the fences): `take()` writes the bottom \f$ b - 1 \f$ and then
 * reads the top, `steal()` reads the top and then the bottom, all four in
 * the single total order of those operations, so that when the deque holds
 * one item the owner and a thief cannot both miss each other, and the one
 * that loses the compare-and-swap on the top gives the item up. The item is
 * read before the compare-and-swap that claims it, by a thief from the
 * vector it read after the bottom, which the owner publishes with a
 * release; the vectors left by a doubling stay until the deque is
 * destroyed, since a thief may still be reading one (they add up to less
 * than the last one). The items are kept in atomics of relaxed order, so
 * `T` is trivially copyable, e.g., an index or a pointer to a task.
 *
 * The contracts [see `Contract.h`]: only the owner, one thread at a time,
 * calls `push()` and `take()`; the initial capacity is a power of 2, at
 * least 2. The operations are defined in `WorkStealingDeque.tpp`.
 *
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
  /// the places less 1
  std::int64_t mask;
  /// the places
  std::unique_ptr< std::atomic< T >[] > place;
  /// the vector of \f$ c \f$ places
  explicit Ring( std::int64_t c )
   : mask( c - 1 ) , place( new std::atomic< T >[ std::size_t( c ) ] ) {}
  /// the item of the index \f$ i \f$
  [[nodiscard]] T get( std::int64_t i ) const {
   return( place[ std::size_t( i & mask ) ].load(
                                            std::memory_order_relaxed ) );
   }
  /// the item of the index \f$ i \f$ set to \f$ x \f$
  void put( std::int64_t i , T x ) {
   place[ std::size_t( i & mask ) ].store( x , std::memory_order_relaxed );
   }
  };

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty deque on a vector of capacity \f$ c \f$
 /// \pre \f$ c \ge 2 \f$ is a power of 2
 explicit WorkStealingDeque( std::size_t capacity = 1024 );
 /// no copy, since the thieves share it by its address
 WorkStealingDeque( const WorkStealingDeque & ) = delete;
 /// no copy, since the thieves share it by its address
 WorkStealingDeque & operator=( const WorkStealingDeque & ) = delete;

 /// pushes \f$ x \f$ at the bottom, doubling the vector if it is full; only
 /// the owner calls it
 ///
 /// In pseudocode (Chase and Lev, beyond the book), \f$ A \f$ the vector of
 /// \f$ |A| \f$ places, the index \f$ i \f$ at its place
 /// \f$ i \bmod |A| \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad b' \gets b;\ t' \gets t\quad \triangleright\ \text{relaxed}
 ///     \text{ and acquire} \cr
 /// 3 & \quad \textbf{if}\ b' - t' = |A|\ \textbf{then} \cr
 /// 4 & \qquad A' \gets \text{a vector of } 2 |A| \text{ places};
 ///     \ \textbf{for}\ i \gets t'\ \textbf{to}\ b' - 1\ \textbf{do}\ A'[ i ]
 ///     \gets A[ i ] \cr
 /// 5 & \qquad A \gets A'\quad \triangleright\ \text{release, the old}
 ///     \text{ vector kept for the thieves} \cr
 /// 6 & \quad A[ b' ] \gets x \cr
 /// 7 & \quad b \gets b' + 1\quad \triangleright\ \text{release}
 /// \end{array}}
 /// \f]
 void push( T x );

 /// takes the item at the bottom, the last pushed, nothing if the deque is
 /// empty or a thief took the last item first; only the owner calls it
 ///
 /// In pseudocode (Chase and Lev with the orders of Lê et al., beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{take}() \cr
 /// 2 & \quad b' \gets b - 1;\ b \gets b'\quad \triangleright
 ///     \ \text{sequentially consistent, before the read of } t \cr
 /// 3 & \quad t' \gets t\quad \triangleright\ \text{sequentially}
 ///     \text{ consistent} \cr
 /// 4 & \quad \textbf{if}\ t' > b'\ \textbf{then}\ b \gets b' + 1;
 ///     \ \textbf{return nil}\quad \triangleright\ \text{empty} \cr
 /// 5 & \quad x \gets A[ b' ] \cr
 /// 6 & \quad \textbf{if}\ t' < b'\ \textbf{then return}\ x\quad
 ///     \triangleright\ \text{no thief reaches } b' \cr
 /// 7 & \quad \mathit{won} \gets \mathrm{CAS}( t , t' , t' + 1 )\quad
 ///     \triangleright\ \text{the last item, raced for} \cr
 /// 8 & \quad b \gets b' + 1 \cr
 /// 9 & \quad \textbf{if}\ \mathit{won}\ \textbf{then return}\ x
 ///     \ \textbf{else return nil}
 /// \end{array}}
 /// \f]
 std::optional< T > take( void );

 /// steals the item at the top, the first pushed, into \f$ x \f$; false if
 /// the deque is empty or another thread took that item first (and then a
 /// thief may try again); any thread may call it
 ///
 /// In pseudocode (Chase and Lev with the orders of Lê et al., beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{steal}( x ) \cr
 /// 2 & \quad t' \gets t;\ b' \gets b\quad \triangleright\ \text{both}
 ///     \text{ sequentially consistent, in this order} \cr
 /// 3 & \quad \textbf{if}\ t' \ge b'\ \textbf{then return false} \cr
 /// 4 & \quad y \gets A[ t' ]\quad \triangleright\ A \text{ read with}
 ///     \text{ acquire} \cr
 /// 5 & \quad \textbf{if not}\ \mathrm{CAS}( t , t' , t' + 1 )
 ///     \ \textbf{then return false} \cr
 /// 6 & \quad x \gets y;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool steal( T & x );

 /// the number of items at some moment of the call, \f$ b - t \f$, exact
 /// for the owner
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
