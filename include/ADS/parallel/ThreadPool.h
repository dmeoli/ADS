/*--------------------------------------------------------------------------*/
/*---------------------------- File ThreadPool.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pool of threads of ADS, beyond the book: the executor [see
 * Executor.h] of the parallel algorithms on std::thread, built on the
 * concurrent structures of ADS. A pool of p threads starts p - 1 of them,
 * the workers; the thread that calls it is the p-th, which works as they
 * do until its call returns. Each of the p has a deque of work stealing of
 * Chase and Lev [see WorkStealingDeque.h], on which it pushes the tasks
 * that the others may take: it takes them back from the bottom, the
 * others steal them from the top (the work-first principle of Cilk, which
 * leaves the oldest, and usually largest, tasks to the thieves). A worker
 * that finds no task in its deque tries to steal one from the others,
 * starting at a random one; if all are empty it looks again for 2 ms (a
 * parameter of the pool) and then sleeps on an event count [see
 * EventCount.h], which wakes it when a task is pushed and costs the pusher
 * a light fence and the read of a line when nobody sleeps. The 2 ms are
 * those of the runtimes of OpenMP, whose threads also look for work for a
 * while before they sleep: the parallel algorithms run phases of tens of
 * microseconds separated by short sequential steps, and a worker woken
 * from its sleep for each phase arrives 10 to 50 us late (on the
 * benchmark, a reduction of 10^6 integers on 8 threads takes 115 us with
 * 20 us of looking and 84 us with 2 ms, as long as with OpenMP).
 *
 * bulk( n , f ) puts a job on the stack of the caller: a counter of the
 * calls f( 0 ) , ... , f( n - 1 ) not yet claimed, and h = min( p - 1 ,
 * n - 1 ) tokens of the job pushed on the deque of the caller. Every
 * thread that runs a token (a thief, or the caller itself) claims calls
 * from the counter, by a fetch-and-add, until none is left, and then gives
 * the token back by decrementing a count of the tokens out; the caller
 * claims calls as well, then takes back from the bottom of its deque the
 * tokens nobody stole and waits for the count to reach 0, running in the
 * meantime the tasks it steals, so that a call nested in f( i ) on another
 * thread is helped. Since a thread pushes and takes back its tokens in
 * the same call, the deques hold only the tokens of jobs still running,
 * and a job never outlives the frame that holds it: no task is allocated.
 * invoke( f , g ) pushes g as a task on the deque of the caller and runs
 * f; then it takes g back and runs it, unless a thief stole it, in which
 * case it waits for g while stealing other tasks (the leapfrogging of
 * Wagner and Calder). Calls of bulk() and invoke() nest to any depth, on
 * any thread of the pool; a thread outside the pool takes the place of the
 * p-th, one at a time (the others wait for it on a mutex).
 *
 * With work W and depth D, the work stealing runs in expected time
 * \f$ W / p + O( D ) \f$ (Blumofe and Leiserson), and a job of bulk() of
 * n calls of equal cost c in \f$ \lceil n / p \rceil c + O( \log p ) \f$
 * after the wake-up, since its h tokens are stolen one at a time; a steal
 * costs one compare-and-swap, a claim one fetch-and-add on the line of the
 * job.
 *
 * The contracts [see Contract.h]: p >= 1; the pool outlives the calls; an
 * exception thrown by f in bulk() stops the claims and is thrown again by
 * bulk() once the calls already started have returned, the same for f and
 * g in invoke().
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
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

 /// a pool of p threads, the caller included (the number of the hardware
 /// threads if 0), whose idle workers look for tasks for spin ns before
 /// they sleep
 explicit ThreadPool( std::size_t p = 0 , std::int64_t spin = 2000000 );
 ThreadPool( const ThreadPool & ) = delete;
 ThreadPool & operator=( const ThreadPool & ) = delete;

 /// stops the workers, which first finish their tasks
 ~ThreadPool();

 /// the number of threads, the caller included
 [[nodiscard]] std::size_t concurrency( void ) const {
  return( f_slot.size() );
  }

 /// f( 0 ) , ... , f( n - 1 ) on the threads of the pool
 template< class F >
 void bulk( std::size_t n , F && f );

 /// f() and g() in parallel
 template< class F , class G >
 void invoke( F && f , G && g );

/*--------------------------------------------------------------------------*/
 private:

 /// a task: the function that runs it
 struct Task {
  void ( * run )( Task * ) noexcept;
  };

 /// the deque of a thread, on lines of its own
 struct alignas( 128 ) Slot {
  WorkStealingDeque< Task * > deque{ 256 };
  std::uint64_t seed = 0;
  };

 /// the pool and the slot of the running thread
 struct Self {
  ThreadPool * pool;
  std::size_t slot;
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

 static inline thread_local Self tl_self{ nullptr , 0 };

 std::vector< std::unique_ptr< Slot > > f_slot;   ///< one per thread
 std::vector< std::thread > f_thread;   ///< the workers
 EventCount f_event;                       ///< where the idle ones sleep
 std::atomic< bool > f_stop{ false };      ///< set by the destructor
 std::mutex f_master;                      ///< the slot of the caller
 std::int64_t f_spin;                      ///< ns of looking before sleep
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
