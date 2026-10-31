/*--------------------------------------------------------------------------*/
/*------------------------ File FastFlowExecutor.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the threads of FastFlow, beyond the
 * book, compiled only with `ADS_WITH_FASTFLOW` (the CMake option of the same
 * name, OFF by default) and the headers of FastFlow on the path:
 * `bulk( n , f )` is a `parallel_for` of a `ParallelFor` of FastFlow on
 * \f$ n \f$ indices in chunks of 1, scheduled dynamically by the workers
 * themselves (the scheduler thread is disabled, since the pool already has
 * as many workers as threads), so that the parallel algorithms of ADS run on
 * FastFlow unchanged. The `ParallelFor` is built at the first call of
 * `bulk()` that needs it and kept with the executor, and its \f$ p \f$
 * workers wait between the calls by spinning if `spin` is true (the default
 * of the executor, not of FastFlow), or sleeping otherwise; the thread that
 * calls `bulk()` waits for them and does not work. A call of `bulk()` from a
 * call of `bulk()` runs its calls in order on that thread, since a
 * `ParallelFor` does not nest. `run( p , f )` is a `parallel_for` of
 * another `ParallelFor`, also built at its first call, whose workers sleep
 * between the calls (those of `bulk()` are put to sleep before a run): the
 * workers of a run are long-lived, gain nothing by spinning between runs,
 * and threads that spin unpinned may share a core with the thread that a
 * run waits for, which then waits a quantum of the scheduler of the system
 * (milliseconds). On the \f$ p \f$ calls of long-lived workers FastFlow
 * gives each of its first \f$ p \f$ workers its own index before any
 * worker looks at the indices of the others, so that the calls run at the
 * same time, one per worker, and the caller only waits for them (no call
 * runs on it, `isRunCaller()` is false in each). Its lock is the spin lock
 * of FastFlow (`ff::lock_t`, `FastFlowLock`), the one its runtime takes,
 * whose `try_lock()` is one test-and-set of its flag; the farm with
 * feedback runs on a farm of FastFlow [see `Farm.h`].
 *
 * Unless `ADS_FASTFLOW_MAPPING` is defined, the header defines
 * `NO_DEFAULT_MAPPING` before it includes FastFlow: by default FastFlow pins
 * its \f$ i \f$-th thread to the \f$ i \f$-th core of the machine,
 * regardless of the cores the process may run on (e.g., under `taskset`),
 * and two programs pinned on the same cores slow each other down. Unless
 * they are defined, it also defines `NUM_CORES` and `NUM_REAL_CORES`, the
 * numbers of processors and of physical cores with which FastFlow skips
 * counting them by itself, as functions of ADS that count them once:
 * FastFlow counts the physical cores with a shell per processor (a
 * `popen()` of `cat` on its siblings in `/sys`) at the construction of
 * each `ParallelFor`, i.e., of each executor, which takes most of a second
 * on a machine of hundreds of processors, while the executor is otherwise
 * built in about a millisecond. These macros reach FastFlow only if the
 * header comes before every header of FastFlow, which reads them when its
 * `ff/config.hpp` is first included: a program that includes a header of
 * FastFlow first does not compile, and the error of the preprocessor
 * says why, unless it has defined `NUM_CORES`, `NUM_REAL_CORES` and
 * `NO_DEFAULT_MAPPING` (or `ADS_FASTFLOW_MAPPING`) itself before them.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; one thread at a time
 * calls the executor (the others wait on a mutex); an exception thrown by
 * \f$ f \f$ stops the claims and is thrown again by `bulk()`.
 *
 * \see M. Aldinucci, M. Danelutto, P. Kilpatrick, M. Torquati, FastFlow:
 *      high-level and efficient streaming on multicore, in S. Pllana,
 *      F. Xhafa (eds.), Programming Multi-core and Many-core Computing
 *      Systems, Wiley, 261-280, 2017, doi:10.1002/9781119332015.ch13
 * \see M. Danelutto, M. Torquati, Loop parallelism: a new skeleton
 *      perspective on data parallel patterns, Euromicro International
 *      Conference on Parallel, Distributed, and Network-Based Processing
 *      (PDP), 52-59, 2014, doi:10.1109/PDP.2014.13
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

#ifndef __ADS_FastFlowExecutor
 #define __ADS_FastFlowExecutor
                      /* self-identification: #endif constant */

#if defined( ADS_WITH_FASTFLOW )

// the macros below reach FastFlow only if none of its headers came first:
// ff/config.hpp reads NUM_CORES and NUM_REAL_CORES at its first inclusion,
// and the headers that read NO_DEFAULT_MAPPING include it before; with
// FF_CONFIG_HPP defined they are read already, and the executor would pin
// its threads and count the cores by a shell at each ParallelFor
#if defined( FF_CONFIG_HPP ) && ( ! defined( NUM_CORES ) || \
    ! defined( NUM_REAL_CORES ) || \
    ( ! defined( ADS_FASTFLOW_MAPPING ) && ! defined( NO_DEFAULT_MAPPING ) ) )
 #error "ADS/parallel/FastFlowExecutor.h included after a header of \
FastFlow: include it before any <ff/...> header (or define NUM_CORES, \
NUM_REAL_CORES and NO_DEFAULT_MAPPING before them), since FastFlow reads \
these macros when its ff/config.hpp is first included"
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! defined( ADS_FASTFLOW_MAPPING ) && ! defined( NO_DEFAULT_MAPPING )
 #define NO_DEFAULT_MAPPING
#endif

#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <thread>

#if defined( __linux__ ) && __has_include( <sched.h> )
 #include <sched.h>
 #define ADS_FASTFLOW_AFFINITY 1
#endif

namespace ADS::detail {

/// the logical processors of the machine, once, for `ff_numCores()` of
/// FastFlow, which otherwise runs a shell at each call
inline long fastFlowCores( void )
{
 static const long n = long( std::max( std::thread::hardware_concurrency() ,
                                       1u ) );
 return( n );
 }

/// the physical cores among the processors on which the process may run,
/// one per list of siblings of simultaneous multithreading read from
/// `/sys`, once, for `ff_realNumCores()` of FastFlow, which otherwise runs
/// a shell per processor at the construction of each `ParallelFor`
inline long fastFlowRealCores( void )
{
 static const long n = [] {
  long c = 0;
#if defined( ADS_FASTFLOW_AFFINITY )
  cpu_set_t s;
  CPU_ZERO( &s );
  if( sched_getaffinity( 0 , sizeof( s ) , &s ) == 0 ) {
   std::set< std::string > cores;
   for( int i = 0 ; i < CPU_SETSIZE ; ++i )
    if( CPU_ISSET( i , &s ) ) {
     std::ifstream f( "/sys/devices/system/cpu/cpu" + std::to_string( i ) +
                      "/topology/thread_siblings_list" );
     std::string l;
     if( ! ( f >> l ) )
      l = "#" + std::to_string( i );       // no topology: a core alone
     cores.insert( l );
     }
   c = long( cores.size() );
   }
#endif
  return( c > 0 ? c : fastFlowCores() );
  }();
 return( n );
 }

}  // end( namespace ADS::detail )

#if ! defined( NUM_CORES )
 #define NUM_CORES ::ADS::detail::fastFlowCores()
#endif
#if ! defined( NUM_REAL_CORES )
 #define NUM_REAL_CORES ::ADS::detail::fastFlowRealCores()
#endif

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
#include <ff/spin-lock.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FastFlowLock ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the spin lock of FastFlow, `ff::lock_t`, as a mutex of the standard
class FastFlowLock {
 public:

 /// an unlocked lock
 FastFlowLock( void ) { ff::init_unlocked( f_lock ); }
 /// no copy, as a mutex
 FastFlowLock( const FastFlowLock & ) = delete;
 /// no copy, as a mutex
 FastFlowLock & operator=( const FastFlowLock & ) = delete;

 /// takes the lock, spinning until it is free
 void lock( void ) { ff::spin_lock( f_lock ); }
 /// gives the lock back
 void unlock( void ) { ff::spin_unlock( f_lock ); }
 /// takes the lock if it is free, by one test-and-set of its flag (that of
 /// `spin_lock()`), and tells whether it did
 bool try_lock( void ) {
  return( ! f_lock->test_and_set( std::memory_order_acquire ) );
  }

 private:

 ff::lock_t f_lock;                        ///< the lock of FastFlow
 };  // end( class FastFlowLock )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS FastFlowExecutor -------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on a ParallelFor of FastFlow
class FastFlowExecutor {

/*--------------------------------------------------------------------------*/
 public:

 /// the executor of \f$ p \f$ workers of FastFlow (the hardware threads if
 /// \f$ p = 0 \f$), which spin between the calls if `spin`
 /// \pre \f$ p \ge 1 \f$ once the hardware threads are counted
 explicit FastFlowExecutor( std::size_t p = 0 , bool spin = true )
  : f_p( p ? p : std::max< std::size_t >(
                  std::thread::hardware_concurrency() , 1 ) ) ,
    f_spin( spin ) {
  ADS_PRE( f_p >= 1 );
  }

 /// the number of workers
 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on the workers, in
 /// order on the calling thread if it is inside a call
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{or}
 ///     \ \text{inside a call of } f\ \textbf{then} \cr
 /// 3 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ f( i ) \cr
 /// 4 & \quad \textbf{else}\ \text{the } \mathrm{parallel\_for} \text{ of }
 ///     \text{FastFlow on } [ 0 , n ) \text{ in chunks of 1, on }
 ///     \min\{ p , n \} \text{ workers}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( f_p == 1 ) || tl_inside ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  std::lock_guard< std::mutex > g( f_lock );
  detail::FirstError error;
  loop().parallel_for( 0 , long( n ) , 1 , 1 , [ & ]( const long i ) {
   if( error.failed() )
    return;
   tl_inside = true;
   try {
    f( std::size_t( i ) );
    }
   catch( ... ) {
    error.keep();
    }
   tl_inside = false;
   } , long( std::min( f_p , n ) ) );
  error.rethrow();
  }

 /// the lock of the data shared by the workers of `run()`
 using lock_type = FastFlowLock;

 /// the calls \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ of long-lived workers,
 /// at the same time on \f$ p \f$ workers of FastFlow
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{run}( p , f ) \cr
 /// 2 & \quad \textbf{if}\ p \le 1\ \textbf{or}\ \text{inside a call of }
 ///     f\ \textbf{then}\ \textbf{for}\ i \gets 0\ \textbf{to}\ p - 1
 ///     \ \textbf{do}\ f( i );\ \textbf{return} \cr
 /// 3 & \quad \text{the } \mathrm{parallel\_for} \text{ of FastFlow on }
 ///     [ 0 , p ) \text{ in chunks of 1, on } p \text{ workers, the worker }
 ///     i \text{ first given } i \cr
 /// 4 & \quad \text{throw again the first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \le \f$ `concurrency()`; \f$ p \le 1 \f$ inside a call
 template< class F >
 void run( std::size_t p , F && f ) {
  ADS_PRE( p <= f_p );
  ADS_PRE( ( p <= 1 ) || ! tl_inside );
  detail::RunCaller caller;
  if( p <= 1 ) {
   if( p == 1 )
    f( 0 );
   return;
   }
  std::lock_guard< std::mutex > g( f_lock );
  detail::FirstError error;
  workers().parallel_for( 0 , long( p ) , 1 , 1 , [ & ]( const long i ) {
   tl_inside = true;
   try {
    f( std::size_t( i ) );
    }
   catch( ... ) {
    error.keep();
    }
   tl_inside = false;
   } , long( p ) );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:

 static inline thread_local bool tl_inside = false;   ///< in a call of f

 /// the loop of `bulk()`, built at its first call, spinning if `f_spin`
 ff::ParallelFor & loop( void ) {
  if( ! f_for ) {
   f_for = std::make_unique< ff::ParallelFor >( long( f_p ) , f_spin ,
                                                f_spin );
   f_for->disableScheduler( true );
   }
  return( *f_for );
  }

 /// the loop of `run()`, built at its first call, whose threads sleep
 /// between the calls; the threads of the loop of `bulk()` put to sleep
 ff::ParallelFor & workers( void ) {
  if( ! f_run ) {
   f_run = std::make_unique< ff::ParallelFor >( long( f_p ) , false ,
                                                false );
   f_run->disableScheduler( true );
   }
  if( f_for && f_spin )
   f_for->threadPause();
  return( *f_run );
  }

 std::size_t f_p;                          ///< the workers
 bool f_spin;                              ///< whether bulk() spins
 std::unique_ptr< ff::ParallelFor > f_for;   ///< the loop of bulk()
 std::unique_ptr< ff::ParallelFor > f_run;   ///< the loop of run()
 std::mutex f_lock;                        ///< one caller at a time
 };  // end( class FastFlowExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* ADS_WITH_FASTFLOW */

/*--------------------------------------------------------------------------*/

#endif  /* FastFlowExecutor.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File FastFlowExecutor.h -----------------------*/
/*--------------------------------------------------------------------------*/
