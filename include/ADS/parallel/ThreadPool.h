/*--------------------------------------------------------------------------*/
/*---------------------------- File ThreadPool.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pool of threads of ADS, beyond the book: the executor [see
 * `Executor.h`] of the parallel algorithms on `std::thread`, built on the
 * concurrent structures of ADS. A pool of \f$ p \f$ threads starts
 * \f$ p - 1 \f$ of them, the workers; the thread that calls it is the
 * \f$ p \f$-th, which works as they do until its call returns. Each of the
 * \f$ p \f$ has a deque of work stealing of Chase and Lev [see
 * `WorkStealingDeque.h`], on which it pushes the tasks that the others may
 * take: it takes them back from the bottom, the others steal them from the
 * top (the work-first principle of Cilk, which leaves the oldest, and
 * usually largest, tasks to the thieves). A worker that finds no task in its
 * deque tries to steal one from the others, starting at a random one; if all
 * are empty it looks again for 2 ms (a parameter of the pool) and then
 * sleeps on an event count [see `EventCount.h`], which wakes it when a task
 * is pushed and costs the pusher a light fence and the read of a line when
 * nobody sleeps. The 2 ms are those of the runtimes of OpenMP, whose threads
 * also look for work for a while before they sleep: the parallel algorithms
 * run phases of tens of microseconds separated by short sequential steps,
 * and a worker woken from its sleep for each phase arrives 10 to 50
 * microseconds late (on the benchmark, a reduction of \f$ 10^6 \f$ integers
 * on 8 threads takes 115 microseconds with 20 microseconds of looking and 84
 * with 2 ms, as long as with OpenMP).
 *
 * `bulk( n , f )` puts a job on the stack of the caller: a counter of the
 * calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ not yet claimed, and
 * \f$ h = \min\{ p - 1 , n - 1 \} \f$ tokens of the job pushed on the deque
 * of the caller. Every thread that runs a token (a thief, or the caller
 * itself) claims calls from the counter, by a fetch-and-add, until none is
 * left, and then gives the token back by decrementing a count of the tokens
 * out; the caller claims calls as well, then takes back from the bottom of
 * its deque the tokens nobody stole and waits for the count to reach 0,
 * running in the meantime the tasks it steals, so that a call nested in
 * \f$ f( i ) \f$ on another thread is helped. Since a thread pushes and
 * takes back its tokens in the same call, the deques hold only the tokens of
 * jobs still running, and a job never outlives the frame that holds it: no
 * task is allocated. `invoke( f , g )` pushes \f$ g \f$ as a task on the
 * deque of the caller and runs \f$ f \f$; then it takes \f$ g \f$ back and
 * runs it, unless a thief stole it, in which case it waits for \f$ g \f$
 * while stealing other tasks (the leapfrogging of Wagner and Calder). Calls
 * of `bulk()` and `invoke()` nest to any depth, on any thread of the pool;
 * a thread outside the pool takes the place of the \f$ p \f$-th, one at a
 * time (the others wait for it on a mutex).
 *
 * `run( p , f )` runs the \f$ p \f$ calls of long-lived workers at the
 * same time on \f$ p \f$ distinct threads (see
 * \ref theory_parallel_workers): the caller runs \f$ f( 0 ) \f$ (the call
 * in which `isRunCaller()` holds), and the other calls are claimed one
 * each, under a mutex, by the workers that are idle in their loop, never
 * by a thread that waits inside a call and steals
 * (a worker that waited for a nested job and took a call of a worker would
 * hold it under its own, which may wait for it forever). A run is the only
 * one of the pool for its time; while it lasts the workers that are not
 * among its \f$ p - 1 \f$ keep running the tasks, the nested jobs of the
 * calls among them.
 *
 * With work \f$ W \f$ and depth \f$ D \f$, the work stealing runs in
 * expected time \f$ W / p + O( D ) \f$ (Blumofe and Leiserson), and a job of
 * `bulk()` of \f$ n \f$ calls of equal cost \f$ c \f$ in
 * \f$ \lceil n / p \rceil c + O( \log p ) \f$ after the wake-up, since its
 * \f$ h \f$ tokens are stolen one at a time; a steal costs one
 * compare-and-swap, a claim one fetch-and-add on the line of the job.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; the pool outlives the
 * calls; an exception thrown by \f$ f \f$ in `bulk()` stops the claims and
 * is thrown again by `bulk()` once the calls already started have returned,
 * the same for \f$ f \f$ and \f$ g \f$ in `invoke()`; `run( p , f )` needs
 * \f$ p - 1 \f$ workers idle besides the caller (all of them if it is called
 * from outside the pool with \f$ p \f$ its threads), and throws again the
 * first exception of its calls once all have returned. The operations are
 * defined in `ThreadPool.tpp`.
 *
 * \see D. Chase, Y. Lev, Dynamic circular work-stealing deque, ACM
 *      Symposium on Parallelism in Algorithms and Architectures (SPAA),
 *      21-28, 2005, doi:10.1145/1073970.1073974
 * \see R. D. Blumofe, C. E. Leiserson, Scheduling multithreaded
 *      computations by work stealing, Journal of the ACM 46(5), 720-748,
 *      1999, doi:10.1145/324133.324234
 * \see M. Frigo, C. E. Leiserson, K. H. Randall, The implementation of the
 *      Cilk-5 multithreaded language, ACM SIGPLAN Conference on
 *      Programming Language Design and Implementation (PLDI), 212-223,
 *      1998, doi:10.1145/277650.277725
 * \see D. B. Wagner, B. G. Calder, Leapfrogging: a portable technique for
 *      implementing efficient futures, ACM SIGPLAN Symposium on Principles
 *      and Practice of Parallel Programming (PPoPP), 208-217, 1993,
 *      doi:10.1145/155332.155354
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

#ifndef __ADS_ThreadPool
 #define __ADS_ThreadPool
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/EventCount.h"
#include "ADS/concurrent/WorkStealingDeque.h"
#include "ADS/parallel/Executor.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS ThreadPool ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a pool of threads with work stealing, an executor
class ThreadPool {

/*--------------------------------------------------------------------------*/
 public:

 /// a pool of \f$ p \f$ threads, the caller included (the number of the
 /// hardware threads if \f$ p = 0 \f$), whose idle workers look for tasks
 /// for `spin` ns before they sleep
 /// \pre \f$ p \ge 1 \f$ once the hardware threads are counted
 explicit ThreadPool( std::size_t p = 0 , std::int64_t spin = 2000000 );
 /// no copy, since the workers hold the address of the pool
 ThreadPool( const ThreadPool & ) = delete;
 /// no copy, since the workers hold the address of the pool
 ThreadPool & operator=( const ThreadPool & ) = delete;

 /// stops the workers, which first finish their tasks
 ~ThreadPool();

 /// the number of threads, the caller included
 [[nodiscard]] std::size_t concurrency( void ) const {
  return( f_slot.size() );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on the threads of the
 /// pool, returning when all have returned
 ///
 /// In pseudocode (a job of tokens on the deques of Chase and Lev, beyond
 /// the book), \f$ D_s \f$ the deque of the slot \f$ s \f$ of the caller:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ p = 1\ \textbf{or}\ n = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i );
 ///     \ \textbf{return} \cr
 /// 3 & \quad J \gets \text{a job on the stack};\ J.\mathit{next} \gets 0;
 ///     \ h \gets \min\{ p - 1 , n - 1 \};\ J.\mathit{out} \gets h \cr
 /// 4 & \quad \text{push } h \text{ tokens of } J \text{ on } D_s;
 ///     \ \text{wake the threads that sleep} \cr
 /// 5 & \quad \mathrm{work}( J ) \cr
 /// 6 & \quad \textbf{while}\ \text{the bottom of } D_s \text{ is a token }
 ///     \text{of } J\ \textbf{do} \cr
 /// 7 & \qquad \mathrm{take}( D_s );\ J.\mathit{out} \gets J.\mathit{out} - 1
 ///     \cr
 /// 8 & \quad \textbf{while}\ J.\mathit{out} \ne 0\ \textbf{do}\ \text{steal }
 ///     \text{a task and run it, or pause} \cr
 /// 9 & \quad \text{throw again the first exception of the calls, if any} \cr
 /// 10 & \textbf{procedure}\ \mathrm{work}( J )\quad \triangleright
 ///     \ \text{also run by a thief of a token, which then decrements }
 ///     J.\mathit{out} \cr
 /// 11 & \quad \textbf{loop} \cr
 /// 12 & \qquad i \gets \mathrm{fetchAndAdd}( J.\mathit{next} , 1 );
 ///     \ \textbf{if}\ i \ge n\ \textbf{then return} \cr
 /// 13 & \qquad f( i )\quad \triangleright\ \text{if it throws, keep the }
 ///     \text{exception and set } J.\mathit{next} \gets n
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f );

 /// the calls `f()` and `g()` in parallel, returning when both have returned
 ///
 /// In pseudocode (the leapfrogging of Wagner and Calder, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{invoke}( f , g ) \cr
 /// 2 & \quad \textbf{if}\ p = 1\ \textbf{then}\ f();\ g();\ \textbf{return}
 ///     \cr
 /// 3 & \quad \text{push the task } g \text{ on } D_s;\ \text{wake the }
 ///     \text{threads that sleep} \cr
 /// 4 & \quad f() \cr
 /// 5 & \quad t \gets \mathrm{take}( D_s ) \cr
 /// 6 & \quad \textbf{if}\ t = g\ \textbf{then}\ g()\quad \triangleright
 ///     \ \text{not stolen: here} \cr
 /// 7 & \quad \textbf{else} \cr
 /// 8 & \qquad \textbf{if}\ t \ne \textbf{nil}\ \textbf{then}\ \mathrm{push}(
 ///     D_s , t )\quad \triangleright\ \text{a task of an enclosing call} \cr
 /// 9 & \qquad \textbf{while}\ g \text{ has not returned}\ \textbf{do}
 ///     \ \text{steal a task and run it, or pause} \cr
 /// 10 & \quad \text{throw again the exception of } f \text{, else that of }
 ///     g \text{, if any}
 /// \end{array}}
 /// \f]
 template< class F , class G >
 void invoke( F && f , G && g );

 /// the lock of the data shared by the workers of `run()`
 using lock_type = std::mutex;

 /// the calls \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ of long-lived workers,
 /// at the same time on \f$ p \f$ distinct threads of the pool, returning
 /// when all have returned
 ///
 /// In pseudocode (beyond the book), \f$ c \f$ the next call to claim and
 /// \f$ d \f$ the calls of the workers returned:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{run}( p , f ) \cr
 /// 2 & \quad \textbf{if}\ p \le 1\ \textbf{then}\ \textbf{for}\ i \gets 0
 ///     \ \textbf{to}\ p - 1\ \textbf{do}\ f( i );\ \textbf{return} \cr
 /// 3 & \quad \text{under the mutex of the runs: } c \gets 1;\ d \gets 0;
 ///     \ \text{open the run};\ \text{wake the threads that sleep} \cr
 /// 4 & \quad f( 0 ) \cr
 /// 5 & \quad \textbf{while}\ d \ne p - 1\ \textbf{do}\ \text{steal a task}
 ///     \text{ and run it, or pause} \cr
 /// 6 & \quad \text{throw again the first exception of the calls, if any}
 ///     \cr
 /// 7 & \textbf{procedure}\ \mathrm{claim}()\quad \triangleright
 ///     \ \text{by an idle worker, in its loop only} \cr
 /// 8 & \quad \text{under the mutex: } \textbf{if}\ \text{the run is }
 ///     \text{open}\ \textbf{then}\ i \gets c;\ c \gets c + 1;
 ///     \ \text{close it if } c = p \cr
 /// 9 & \quad f( i );\ \mathrm{fetchAndAdd}( d , 1 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \le \f$ `concurrency()`, with \f$ p - 1 \f$ workers idle
 template< class F >
 void run( std::size_t p , F && f );

/*--------------------------------------------------------------------------*/
 private:

 /// a task: the function that runs it
 struct Task {
  void ( * run )( Task * ) noexcept;   ///< runs the task given
  };

 /// the deque of a thread, on lines of its own
 struct alignas( 128 ) Slot {
  WorkStealingDeque< Task * > deque{ 256 };   ///< the tasks of the thread
  std::uint64_t seed = 0;   ///< the state of its choice of the victims
  };

 /// the pool and the slot of the running thread
 struct Self {
  ThreadPool * pool;   ///< the pool, `nullptr` outside any
  std::size_t slot;    ///< the slot in the pool
  };

 /// the place of the calling thread in the pool for the time of a call:
 /// its own if it is a worker, the p-th otherwise
 class Enter;

 template< class F > struct BulkJob;
 template< class G > struct InvokeTask;

 /// the loop of the worker s
 void loop( std::size_t s );

 /// a task for the thread s: from its deque, or stolen from another
 Task * find( std::size_t s );

 /// steals a task for the thread s and runs it; false if none was found
 bool help( std::size_t s );

 /// claims a call of the open run and makes it; false if none was left
 bool claim( void );

 /// the pool and the slot of the running thread
 static inline thread_local Self tl_self{ nullptr , 0 };

 std::vector< std::unique_ptr< Slot > > f_slot;   ///< one per thread
 std::vector< std::thread > f_thread;   ///< the workers
 EventCount f_event;                       ///< where the idle ones sleep
 std::atomic< bool > f_stop{ false };      ///< set by the destructor
 std::mutex f_master;                      ///< the slot of the caller
 std::int64_t f_spin;                      ///< ns of looking before sleep
 std::mutex f_runOne;                      ///< one run at a time
 std::mutex f_runLock;                     ///< the claims of the calls
 std::atomic< bool > f_runOpen{ false };   ///< calls left to claim
 std::size_t f_runNext = 0;                ///< the next call to claim
 std::size_t f_runN = 0;                   ///< the calls of the run
 void ( * f_runCall )( void * , std::size_t ) = nullptr;   ///< makes one
 void * f_runCtx = nullptr;                ///< its function
 std::atomic< std::size_t > f_runDone{ 0 };   ///< the calls returned
 };  // end( class ThreadPool )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ThreadPool.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ThreadPool.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ThreadPool.h ---------------------------*/
/*--------------------------------------------------------------------------*/
