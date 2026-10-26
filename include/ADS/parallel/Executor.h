/*--------------------------------------------------------------------------*/
/*---------------------------- File Executor.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The layer of execution of the parallel algorithms of ADS, beyond the
 * book: an executor is anything that runs the calls f( 0 ) , ... ,
 * f( n - 1 ) of a function f on its threads and returns when all have
 * returned (bulk()), and that tells how many threads it has
 * (concurrency()); the parallel algorithms of ADS take one as their first
 * argument, and the sequential ones, which take none, stay what they are.
 * The executors are Sequential, which runs the calls in order on the
 * calling thread, ThreadPool [see ThreadPool.h], the pool of ADS on
 * std::thread with a deque of work stealing per thread, and, when their
 * libraries are there, FastFlowExecutor, OpenMPExecutor and TBBExecutor
 * [see FastFlowExecutor.h, OpenMPExecutor.h and TBBExecutor.h], the same
 * calls on the threads of FastFlow, of OpenMP and of oneTBB. An executor
 * may also have invoke( f , g ), which runs f and g in parallel; the
 * others get it from bulk().
 *
 * On these the header builds the patterns that the algorithms share, each
 * a function of the executor:
 * - parallelInvoke( ex , f , g ), the fork and join of two calls;
 * - parallelFor( ex , first , last , f ) and parallelForBlocks( ex , first ,
 *   last , f ), the loop over \f$ [ first , last ) \f$ cut into blocks of
 *   at least grain indices, which the threads claim one at a time from a
 *   counter, so that a faster thread takes more blocks (the dynamic
 *   schedule); the second gives f a whole block, f( lo , hi ), which lets
 *   the compiler vectorize its loop;
 * - parallelReduce( ex , first , last , id , f , op ), the reduction of the
 *   blocks by f( lo , hi ) into partial results, combined by op in the
 *   order of the blocks;
 * - parallelInclusiveScan() and parallelExclusiveScan(), the prefix sums
 *   by an associative op, in one pass over the memory.
 * The blocks depend only on the number of indices and on the grain, never
 * on the number of threads: at most kBlocks of them, each of at least
 * grain indices. Hence a reduction gives the same result on every
 * executor, Sequential included, also with an operation that is
 * associative only up to rounding, e.g., the sum of doubles; that result is
 * the sum by blocks, not the one of std::accumulate.
 *
 * The prefix sums are those of Merrill and Garland, written for the
 * processors instead of the graphics cards: the input is cut into tiles
 * of about 64 KB, which the threads claim in increasing order from a
 * counter. A thread reduces its tile c into its aggregate \f$ a_c \f$ (the
 * tile then stays in its cache), publishes it, and looks back at the
 * tiles before it: from c - 1 down, it adds on the left the aggregate of
 * each tile that has published only that, and stops at the first one that
 * has published its inclusive prefix \f$ P_{c'} = x_0 \oplus \cdots
 * \oplus x_{end( c' )} \f$, so that
 * \f[
 *   P_{c - 1} = P_{c'} \oplus a_{c' + 1} \oplus \cdots \oplus a_{c - 1} ;
 * \f]
 * then it publishes \f$ P_c = P_{c - 1} \oplus a_c \f$ and writes the
 * prefixes of its tile, reading it again from the cache. The input is read
 * from the memory once and the output written once, 2 n moves, where the
 * scan in two passes of Blelloch (the reduction of the blocks, then their
 * scan) reads the input twice, 3 n. A tile waits only for tiles claimed
 * before it, which threads that are running hold; the first tile not done
 * has no tile to wait for, so that the scan always ends, on any executor.
 * The look-back reads O( 1 ) tiles on average when the threads go at the
 * same pace, and at most c.
 *
 * The algorithms take \f$ O( W / p + D ) \f$ time on p threads for work W
 * and depth D, with the dynamic schedule within a factor of 2 of the best
 * when the blocks are many more than the threads (the greedy schedule of
 * Graham); the reduction and the scans have \f$ W = O( n ) \f$ and
 * \f$ D = O( n / B + B ) \f$ for B blocks.
 *
 * The contracts [see Contract.h]: f may run on any thread, and its calls
 * on distinct indices must not race; an exception thrown by a call stops
 * the claim of new blocks and is thrown again by the algorithm, after the
 * calls already started have returned; op is associative.
 * \see R. L. Graham, Bounds on multiprocessing timing anomalies, SIAM
 *      Journal on Applied Mathematics 17(2), 416-429, 1969,
 *      doi:10.1137/0117039
 * \see G. E. Blelloch, Prefix sums and their applications, Technical
 *      Report CMU-CS-90-190, Carnegie Mellon University, 1990
 * \see D. Merrill, M. Garland, Single-pass parallel prefix scan with
 *      decoupled look-back, Technical Report NVR-2016-002, NVIDIA, 2016
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Executor
 #define __ADS_Executor
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the blocks into which the loops and the reductions cut their indices,
/// at most
inline constexpr std::size_t kBlocks = 256;

/// the indices of a block of the loops, at least, by default
inline constexpr std::size_t kGrain = 1024;

/// an executor: bulk( n , f ) runs f( 0 ) , ... , f( n - 1 ) on its
/// threads and returns when they have returned, concurrency() is the
/// number of its threads
template< class E >
concept Executor = requires( E & e , std::size_t n ,
                             void ( & f )( std::size_t ) ) {
 { e.concurrency() } -> std::convertible_to< std::size_t >;
 e.bulk( n , f );
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Sequential ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor of the calling thread alone: the calls in their order
class Sequential {
 public:

 [[nodiscard]] static constexpr std::size_t concurrency( void ) {
  return( 1 );
  }

 template< class F >
 static void bulk( std::size_t n , F && f ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   f( i );
  }

 template< class F , class G >
 static void invoke( F && f , G && g ) {
  f();
  g();
  }
 };  // end( class Sequential )

/*--------------------------------------------------------------------------*/
/*------------------------------- PATTERNS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// runs f() and g() in parallel and returns when both have returned
template< Executor E , class F , class G >
void parallelInvoke( E & ex , F && f , G && g );

/// the blocks of [ first , last ) with at least grain indices each, at most
/// kBlocks of them: their number, and the size of all but the last
[[nodiscard]] inline std::pair< std::size_t , std::size_t > blocksOf(
                                std::size_t first , std::size_t last ,
                                std::size_t grain = kGrain );

/// f( lo , hi ) on the blocks [ lo , hi ) of [ first , last ), in parallel
template< Executor E , class F >
void parallelForBlocks( E & ex , std::size_t first , std::size_t last ,
                        F && f , std::size_t grain = kGrain );

/// f( i ) for each i in [ first , last ), in parallel
template< Executor E , class F >
void parallelFor( E & ex , std::size_t first , std::size_t last , F && f ,
                  std::size_t grain = kGrain );

/// the reduction of [ first , last ): f( lo , hi ) of each block, combined
/// by op in the order of the blocks, starting from id
template< Executor E , class T , class F , class Op >
[[nodiscard]] T parallelReduce( E & ex , std::size_t first ,
                                std::size_t last , T id , F && f , Op op ,
                                std::size_t grain = kGrain );

/// out[ i ] = init op in[ 0 ] op ... op in[ i ], for the n items from in;
/// in and out may be the same; the last prefix (init if n = 0)
template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class T ,
          class Op = std::plus<> >
T parallelInclusiveScan( E & ex , I in , std::size_t n , O out , T init ,
                         Op op = Op() );

/// out[ i ] = init op in[ 0 ] op ... op in[ i - 1 ], for the n items from
/// in; in and out may be the same; the sum of all of them with init
template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class T ,
          class Op = std::plus<> >
T parallelExclusiveScan( E & ex , I in , std::size_t n , O out , T init ,
                         Op op = Op() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/Executor.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Executor.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Executor.h ----------------------------*/
/*--------------------------------------------------------------------------*/
