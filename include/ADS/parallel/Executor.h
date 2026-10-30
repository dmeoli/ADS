/*--------------------------------------------------------------------------*/
/*---------------------------- File Executor.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The layer of execution of the parallel algorithms of ADS, beyond the
 * book: an executor runs the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ of a
 * function \f$ f \f$ on its threads and returns when all have returned
 * (`bulk( n , f )`), and tells how many threads it has (`concurrency()`).
 * The parallel algorithms of ADS take one as their first argument, while the
 * sequential ones, which take none, stay what they are. The executors are
 * `Sequential`, which runs the calls in order on the calling thread,
 * `ThreadPool` [see `ThreadPool.h`], the pool of ADS on `std::thread` with a
 * deque of work stealing per thread, and, when their libraries are there,
 * `FastFlowExecutor`, `OpenMPExecutor` and `TBBExecutor` [see
 * `FastFlowExecutor.h`, `OpenMPExecutor.h` and `TBBExecutor.h`], the same
 * calls on the threads of FastFlow, of OpenMP and of oneTBB. An executor may
 * also have `invoke( f , g )`, which runs \f$ f \f$ and \f$ g \f$ in
 * parallel; the others get it from `bulk()`.
 *
 * The calls of `bulk()` are tasks: a thread may run several of them, one
 * after the other, so that a call that waits for another one may never see
 * it start. A program of long-lived workers that talk to each other (the
 * workers of a parallel search, each with its own queue and its inbox)
 * needs instead `run( p , f )`, whose \f$ p \f$ calls
 * \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ run at the same time on
 * \f$ p \f$ distinct threads of the executor, and a lock for the data that
 * they share, `lock_type` (the concept `WorkerExecutor`): `Sequential` runs
 * one worker, the others as many as they have threads (see
 * \ref theory_parallel_workers). The channels between the workers, the farm
 * with feedback and the mailbox with the detection of the termination are
 * built on these [see `Channels.h`, `Farm.h` and `Mailbox.h`].
 *
 * On these the header builds the patterns that the algorithms share, each a
 * function of the executor:
 *
 * - `parallelInvoke( ex , f , g )`, the fork and join of two calls;
 * - `parallelFor( ex , first , last , f )` and
 *   `parallelForBlocks( ex , first , last , f )`, the loop over
 *   \f$ [ \mathit{first} , \mathit{last} ) \f$ cut into blocks of at least
 *   \f$ g \f$ indices (the grain), which the threads claim one at a time
 *   from a counter, so that a faster thread takes more blocks (the dynamic
 *   schedule); the second gives \f$ f \f$ a whole block, `f( lo , hi )`,
 *   which lets the compiler vectorize its loop;
 * - `parallelReduce( ex , first , last , id , f , op )`, the reduction of
 *   the blocks by `f( lo , hi )` into partial results, combined by
 *   \f$ \oplus \f$ (`op`) in the order of the blocks;
 * - `parallelInclusiveScan()` and `parallelExclusiveScan()`, the prefix
 *   sums by an associative \f$ \oplus \f$, in one pass over the memory.
 *
 * The blocks depend only on the number of indices and on the grain, never on
 * the number of threads: at most `kBlocks` of them, each of at least
 * \f$ g \f$ indices. Hence a reduction gives the same result on every
 * executor, `Sequential` included, also with an operation that is
 * associative only up to rounding, e.g., the sum of doubles; that result is
 * the sum by blocks, not the one of `std::accumulate()`.
 *
 * The prefix sums are those of Merrill and Garland, written for the
 * processors instead of the graphics cards: the input is cut into tiles of
 * about 64 KB, which the threads claim in increasing order from a counter.
 * A thread reduces its tile \f$ c \f$ into its aggregate \f$ a_c \f$ (the
 * tile then stays in its cache), publishes it, and looks back at the tiles
 * before it: from \f$ c - 1 \f$ down, it adds on the left the aggregate of
 * each tile that has published only that, and stops at the first one that
 * has published its inclusive prefix
 * \f$ P_{c'} = x_0 \oplus \cdots \oplus x_{\mathrm{end}( c' )} \f$, so that
 * \f[
 *   P_{c - 1} = P_{c'} \oplus a_{c' + 1} \oplus \cdots \oplus a_{c - 1} ;
 * \f]
 * then it publishes \f$ P_c = P_{c - 1} \oplus a_c \f$ and writes the
 * prefixes of its tile, reading it again from the cache. The input is read
 * from the memory once and the output written once, \f$ 2 n \f$ moves, where
 * the scan in two passes of Blelloch (the reduction of the blocks, then
 * their scan) reads the input twice, \f$ 3 n \f$. A tile waits only for
 * tiles claimed before it, which threads that are running hold; the first
 * tile not done has no tile to wait for, so that the scan always ends, on
 * any executor. The look-back reads \f$ O( 1 ) \f$ tiles on average when the
 * threads go at the same pace, and at most \f$ c \f$.
 *
 * The algorithms take \f$ O( W / p + D ) \f$ time on \f$ p \f$ threads for
 * work \f$ W \f$ and depth \f$ D \f$, with the dynamic schedule within a
 * factor of 2 of the best when the blocks are many more than the threads
 * (the greedy schedule of Graham); the reduction and the scans have
 * \f$ W = O( n ) \f$ and \f$ D = O( n / B + B ) \f$ for \f$ B \f$ blocks.
 *
 * The contracts [see `Contract.h`]: \f$ f \f$ may run on any thread, and its
 * calls on distinct indices must not race; an exception thrown by a call
 * stops the claim of new blocks and is thrown again by the algorithm, after
 * the calls already started have returned; \f$ \oplus \f$ is associative.
 * The patterns are defined in `Executor.tpp`.
 *
 * \see R. L. Graham, Bounds on multiprocessing timing anomalies, SIAM
 *      Journal on Applied Mathematics 17(2), 416-429, 1969,
 *      doi:10.1137/0117039
 * \see R. P. Brent, The parallel evaluation of general arithmetic
 *      expressions, Journal of the ACM 21(2), 201-206, 1974,
 *      doi:10.1145/321812.321815
 * \see G. E. Blelloch, Prefix sums and their applications, Technical
 *      Report CMU-CS-90-190, Carnegie Mellon University, 1990
 * \see D. Merrill, M. Garland, Single-pass parallel prefix scan with
 *      decoupled look-back, Technical Report NVR-2016-002, NVIDIA, 2016
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

/// the largest number of blocks into which the loops and the reductions cut
/// their indices
inline constexpr std::size_t kBlocks = 256;

/// the least number of indices of a block of the loops, by default
inline constexpr std::size_t kGrain = 1024;

/// the executors, whose `bulk( n , f )` runs
/// \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on their threads and returns when
/// they have returned, and whose `concurrency()` is the number of their
/// threads
template< class E >
concept Executor = requires( E & e , std::size_t n ,
                             void ( & f )( std::size_t ) ) {
 { e.concurrency() } -> std::convertible_to< std::size_t >;
 e.bulk( n , f );
 };

/// the executors that also run long-lived workers: `run( p , f )` runs
/// \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ at the same time on \f$ p \f$
/// distinct threads and returns when they have returned, and `lock_type`
/// is a mutex of the executor (`lock()` and `unlock()`)
template< class E >
concept WorkerExecutor = Executor< E > &&
 requires( E & e , std::size_t p , void ( & f )( std::size_t ) ,
           typename E::lock_type & l ) {
  e.run( p , f );
  l.lock();
  l.unlock();
  };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Sequential ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor of the calling thread alone, which makes the calls in their
/// order
class Sequential {
 public:

 /// the number of threads, 1
 [[nodiscard]] static constexpr std::size_t concurrency( void ) {
  return( 1 );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$, in this order, on the
 /// calling thread
 template< class F >
 static void bulk( std::size_t n , F && f ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   f( i );
  }

 /// the call `f()` and then the call `g()`, on the calling thread
 template< class F , class G >
 static void invoke( F && f , G && g ) {
  f();
  g();
  }

 /// the lock of the data shared by the workers
 using lock_type = std::mutex;

 /// the call \f$ f( 0 ) \f$ on the calling thread, the only worker, if
 /// \f$ p = 1 \f$ (none if \f$ p = 0 \f$)
 /// \pre \f$ p \le 1 \f$
 template< class F >
 static void run( std::size_t p , F && f ) {
  ADS_PRE( p <= 1 );
  if( p == 1 )
   f( 0 );
  }
 };  // end( class Sequential )

/*--------------------------------------------------------------------------*/
/*------------------------------- PATTERNS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the calls `f()` and `g()` in parallel, returning when both have returned
///
/// In pseudocode (the fork and join, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{parallelInvoke}( \mathit{ex} , f , g ) \cr
/// 2 & \quad \textbf{if}\ \mathit{ex} \text{ has } \mathrm{invoke}
///     \ \textbf{then}\ \mathit{ex}.\mathrm{invoke}( f , g ) \cr
/// 3 & \quad \textbf{else}\ \mathit{ex}.\mathrm{bulk}( 2 , i \mapsto
///     \textbf{if}\ i = 0\ \textbf{then}\ f()\ \textbf{else}\ g() ) \cr
/// 4 & \quad \text{throw again the first exception of the calls, if any}
/// \end{array}}
/// \f]
template< Executor E , class F , class G >
void parallelInvoke( E & ex , F && f , G && g );

/// the blocks of \f$ [ \mathit{first} , \mathit{last} ) \f$, with at least
/// \f$ g \f$ indices each and at most `kBlocks` of them: their number
/// \f$ b \f$, and the size \f$ s \f$ of all but the last
///
/// In pseudocode (beyond the book), the block \f$ i \f$ being
/// \f$ [ \mathit{first} + i s , \min\{ \mathit{last} , \mathit{first} +
/// ( i + 1 ) s \} ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{blocksOf}( \mathit{first} , \mathit{last}
///     , g ) \cr
/// 2 & \quad n \gets \max\{ \mathit{last} - \mathit{first} , 0 \} \cr
/// 3 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ ( 0 , 0 ) \cr
/// 4 & \quad s \gets \max\{ g , 1 , \lceil n / \mathit{kBlocks} \rceil \} \cr
/// 5 & \quad \textbf{return}\ ( \lceil n / s \rceil , s )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::pair< std::size_t , std::size_t > blocksOf(
                                std::size_t first , std::size_t last ,
                                std::size_t grain = kGrain );

/// the calls `f( lo , hi )` on the blocks \f$ [ \mathit{lo} , \mathit{hi} )
/// \f$ of \f$ [ \mathit{first} , \mathit{last} ) \f$, in parallel
///
/// In pseudocode (the dynamic schedule, beyond the book), with
/// \f$ [ \mathit{lo}_i , \mathit{hi}_i ) \f$ the block \f$ i \f$ of
/// `blocksOf()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{parallelForBlocks}( \mathit{ex} ,
///     \mathit{first} , \mathit{last} , f , g ) \cr
/// 2 & \quad ( b , s ) \gets \mathrm{blocksOf}( \mathit{first} ,
///     \mathit{last} , g ) \cr
/// 3 & \quad \textbf{if}\ b = 1\ \textbf{or}
///     \ \mathit{ex}.\mathrm{concurrency}() = 1\ \textbf{then} \cr
/// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ b - 1\ \textbf{do}\ f(
///     \mathit{lo}_i , \mathit{hi}_i ) \cr
/// 5 & \quad \textbf{else}\ \mathit{ex}.\mathrm{bulk}( b , i \mapsto
///     \textbf{if}\ \text{no call has thrown }\ \textbf{then}\ f(
///     \mathit{lo}_i , \mathit{hi}_i ) ) \cr
/// 6 & \quad \text{throw again the first exception of the calls, if any}
/// \end{array}}
/// \f]
template< Executor E , class F >
void parallelForBlocks( E & ex , std::size_t first , std::size_t last ,
                        F && f , std::size_t grain = kGrain );

/// the calls `f( i )` for each \f$ i \in [ \mathit{first} , \mathit{last} )
/// \f$, in parallel
///
/// In pseudocode (beyond the book), a loop on each block of
/// `parallelForBlocks()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{parallelFor}( \mathit{ex} , \mathit{first}
///     , \mathit{last} , f , g ) \cr
/// 2 & \quad \mathrm{parallelForBlocks}( \mathit{ex} , \mathit{first} ,
///     \mathit{last} , ( \mathit{lo} , \mathit{hi} ) \mapsto \textbf{for}\ i
///     \gets \mathit{lo}\ \textbf{to}\ \mathit{hi} - 1\ \textbf{do}\ f( i ) ,
///     g )
/// \end{array}}
/// \f]
template< Executor E , class F >
void parallelFor( E & ex , std::size_t first , std::size_t last , F && f ,
                  std::size_t grain = kGrain );

/// the reduction of \f$ [ \mathit{first} , \mathit{last} ) \f$: the
/// results of `f( lo , hi )` on the blocks, combined by \f$ \oplus \f$
/// (`op`) in the order of the blocks, starting from `id`
///
/// In pseudocode (beyond the book), the same blocks on every executor:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{parallelReduce}( \mathit{ex} ,
///     \mathit{first} , \mathit{last} , \mathit{id} , f , \oplus , g ) \cr
/// 2 & \quad ( b , s ) \gets \mathrm{blocksOf}( \mathit{first} ,
///     \mathit{last} , g ) \cr
/// 3 & \quad \mathrm{parallelFor}( \mathit{ex} , 0 , b , i \mapsto
///     \mathit{part}[ i ] \gets f( \mathit{lo}_i , \mathit{hi}_i ) , 1 ) \cr
/// 4 & \quad r \gets \mathit{id} \cr
/// 5 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ b - 1\ \textbf{do}\ r
///     \gets r \oplus \mathit{part}[ i ] \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ \oplus \f$ is associative
template< Executor E , class T , class F , class Op >
[[nodiscard]] T parallelReduce( E & ex , std::size_t first ,
                                std::size_t last , T id , F && f , Op op ,
                                std::size_t grain = kGrain );

/// the inclusive prefixes
/// \f$ y[ i ] = \mathit{init} \oplus x[ 0 ] \oplus \cdots \oplus x[ i ] \f$
/// of the \f$ n \f$ items \f$ x \f$ from `in`, written from `out`, which may
/// be `in`; the last prefix (`init` if \f$ n = 0 \f$)
///
/// In pseudocode (the decoupled look-back of Merrill and Garland, beyond the
/// book), with tiles of \f$ t \f$ items, the tile \f$ c \f$ being
/// \f$ [ \mathit{lo}_c , \mathit{hi}_c ) \f$, and the flag of a tile 0, 1
/// when its aggregate \f$ A[ c ] \f$ is published, 2 when its prefix
/// \f$ P[ c ] \f$ is:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{parallelInclusiveScan}( \mathit{ex} , x ,
///     n , y , \mathit{init} , \oplus ) \cr
/// 2 & \quad t \gets \max\{ 256 , \lfloor 2^{16} / \text{the bytes of an }
///     \text{item} \rfloor \};\ T \gets \lceil n / t \rceil \cr
/// 3 & \quad \textbf{if}\ p = 1\ \textbf{or}\ T \le 1
///     \ \textbf{then return}\ \text{the scan of } x \text{ on one thread} \cr
/// 4 & \quad \mathit{next} \gets 0;\ \mathit{flag}[ 0 .. T - 1 ] \gets 0 \cr
/// 5 & \quad \text{on each of } \min\{ p , T \} \text{ threads}
///     \ \textbf{loop} \cr
/// 6 & \qquad c \gets \mathrm{fetchAndAdd}( \mathit{next} , 1 );
///     \ \textbf{if}\ c \ge T\ \textbf{then return} \cr
/// 7 & \qquad a \gets x[ \mathit{lo}_c ] \oplus \cdots \oplus x[
///     \mathit{hi}_c - 1 ];\ e \gets \mathit{init} \cr
/// 8 & \qquad \textbf{if}\ c > 0\ \textbf{then} \cr
/// 9 & \qquad\quad A[ c ] \gets a;\ \mathit{flag}[ c ] \gets 1;\ e \gets
///     \textbf{nil};\ j \gets c \cr
/// 10 & \qquad\quad \textbf{repeat}\quad \triangleright\ \text{the }
///     \text{look-back} \cr
/// 11 & \qquad\qquad j \gets j - 1;\ \text{wait while }
///     \mathit{flag}[ j ] = 0 \cr
/// 12 & \qquad\qquad v \gets P[ j ]\ \textbf{if}\ \mathit{flag}[ j ] = 2
///     \ \textbf{else}\ A[ j ] \cr
/// 13 & \qquad\qquad e \gets v \oplus e\quad \triangleright\ v \text{ if }
///     e = \textbf{nil} \cr
/// 14 & \qquad\quad \textbf{until}\ \mathit{flag}[ j ] = 2 \cr
/// 15 & \qquad P[ c ] \gets e \oplus a;\ \mathit{flag}[ c ] \gets 2 \cr
/// 16 & \qquad \textbf{for}\ i \gets \mathit{lo}_c\ \textbf{to}
///     \ \mathit{hi}_c - 1\ \textbf{do}\ e \gets e \oplus x[ i ];\ y[ i ]
///     \gets e \cr
/// 17 & \quad \textbf{return}\ P[ T - 1 ]
/// \end{array}}
/// \f]
/// \pre \f$ \oplus \f$ is associative
template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class T ,
          class Op = std::plus<> >
T parallelInclusiveScan( E & ex , I in , std::size_t n , O out , T init ,
                         Op op = Op() );

/// the exclusive prefixes
/// \f$ y[ i ] = \mathit{init} \oplus x[ 0 ] \oplus \cdots \oplus x[ i - 1 ]
/// \f$ of the \f$ n \f$ items \f$ x \f$ from `in`, written from `out`, which
/// may be `in`; the sum of all of them with `init`
///
/// In pseudocode (beyond the book), that of `parallelInclusiveScan()` with
/// its line 16 writing each prefix before it adds the item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 16 & \qquad \textbf{for}\ i \gets \mathit{lo}_c\ \textbf{to}
///     \ \mathit{hi}_c - 1\ \textbf{do}\ y[ i ] \gets e;\ e \gets e \oplus x[
///     i ]
/// \end{array}}
/// \f]
/// \pre \f$ \oplus \f$ is associative
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
