/*--------------------------------------------------------------------------*/
/*------------------------- File OpenMPExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the threads of OpenMP, beyond the book,
 * compiled only when the compiler has OpenMP on (`_OPENMP`, e.g.,
 * `-fopenmp`, or the CMake option `ADS_WITH_OPENMP`, OFF by default):
 * `bulk( n , f )` is, outside a parallel region, a loop
 * `#pragma omp parallel for` on \f$ n \f$ indices with the dynamic schedule
 * of chunks of 1 on \f$ \min\{ p , n \} \f$ threads, and `invoke( f , g )`
 * is `bulk()` on 2 indices, so that the parallel algorithms of ADS run on
 * the runtime of OpenMP of the compiler (the libgomp of GCC, the libomp of
 * LLVM) unchanged. The thread that calls is one of the \f$ p \f$, as in
 * every region of OpenMP. Inside a parallel region that is already open
 * (`omp_in_parallel()`: a region of the program around the call of an
 * algorithm of ADS, or a call of `bulk()` nested in another), `bulk()` is
 * instead a `#pragma omp taskloop` of chunks of 1, whose calls are tasks of
 * the team that is there, and which waits for them at its implicit
 * taskgroup, a point where the waiting thread runs tasks too; a nested
 * region would get one thread (the default of OpenMP), so that the
 * algorithm would run in order on the caller. In this mode the threads are
 * those of the team, and `concurrency()` of an executor built with
 * \f$ p = 0 \f$ is their number; the calls may be made by any thread of
 * the team that joins the wait, e.g., from inside `omp single` or `omp
 * masked`, or from each thread on data of its own. The task mode needs
 * `taskloop`, i.e., OpenMP 4.5 (not the OpenMP 2.0 of MSVC, which keeps the
 * nested regions). `run( p , f )` is a region
 * `omp parallel` of \f$ p \f$ threads, the thread \f$ i \f$ of the team
 * making the call \f$ f( i ) \f$ of a long-lived worker (the caller is the
 * thread 0, so that `isRunCaller()` holds in \f$ f( 0 ) \f$), and its lock is
 * the lock of OpenMP (`omp_lock_t`, `OpenMPLock`); a runtime that gives the
 * region fewer threads (a run nested in a region, or a limit of the
 * threads) makes no call and throws, since the workers could not all run at
 * the same time.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; an exception thrown by
 * \f$ f \f$ stops the claims and is thrown again by `bulk()` (an exception
 * may not leave a region of OpenMP).
 *
 * \see OpenMP Architecture Review Board, OpenMP Application Programming
 *      Interface, version 5.2, 2021, https://www.openmp.org/spec-html/5.2/
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

#ifndef __ADS_OpenMPExecutor
 #define __ADS_OpenMPExecutor
/* self-identification: #endif constant */

#if defined( _OPENMP )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <omp.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS OpenMPLock ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the lock of OpenMP, `omp_lock_t`, as a mutex of the standard
class OpenMPLock
{
 public:
 /// an unlocked lock; on the runtime of LLVM a test-and-set one, since its
 /// default lock queues the threads and hands over in their order, so that
 /// with more threads than cores the next one is often descheduled and the
 /// others wait behind it (libgomp ignores the hint)
 OpenMPLock( void ) {
#if defined( KMP_VERSION_MAJOR )
  omp_init_lock_with_hint( &f_lock , omp_sync_hint_uncontended );
#else
  omp_init_lock( &f_lock );
#endif
  }
 /// no copy, as a mutex
 OpenMPLock( const OpenMPLock & ) = delete;
 /// no copy, as a mutex
 OpenMPLock & operator=( const OpenMPLock & ) = delete;
 /// destroys the lock, which nobody holds
 ~OpenMPLock() { omp_destroy_lock( &f_lock ); }

 /// takes the lock, waiting for it
 void lock( void ) {
  omp_set_lock( &f_lock );
  ( void )f_turn.load( std::memory_order_acquire );
  }
 /// gives the lock back
 void unlock( void ) {
  f_turn.store( f_turn.load( std::memory_order_relaxed ) + 1 ,
		std::memory_order_release );
  omp_unset_lock( &f_lock );
  }
 /// takes the lock if it is free, and tells whether it did
 bool try_lock( void ) {
  if( omp_test_lock( &f_lock ) == 0 )
   return( false );
  ( void )f_turn.load( std::memory_order_acquire );
  return( true );
  }

 private:
 omp_lock_t f_lock;  ///< the lock of OpenMP
 /// the holders in turn: each releases it before giving the lock back and
 /// the next acquires it after taking the lock, an order that the lock of
 /// OpenMP already gives but that the sanitizer of threads, which does not
 /// see inside the runtime of OpenMP, sees only through these accesses
 /// (on x86-64 a plain store and a plain load)
 std::atomic< unsigned > f_turn{ 0 };
 };  // end( class OpenMPLock )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OpenMPExecutor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on the threads of OpenMP
class OpenMPExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the executor of \f$ p \f$ threads of OpenMP (`omp_get_max_threads()`
 /// if \f$ p = 0 \f$, or the team of the caller inside a region)
 /// \pre \f$ p \ge 1 \f$
 explicit OpenMPExecutor( std::size_t p = 0 ) : f_p( p ) {
  ADS_PRE( concurrency() >= 1 );
  }

 /// the number of threads: \f$ p \f$, or, if \f$ p = 0 \f$, the threads of
 /// the team of the caller inside a region and `omp_get_max_threads()`
 /// outside
 [[nodiscard]] std::size_t concurrency( void ) const {
  if( f_p )
   return( f_p );
  const int c =
    omp_in_parallel() ? omp_get_num_threads() : omp_get_max_threads();
  return( std::size_t( c > 0 ? c : 1 ) );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on the threads
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i ) \cr
 /// 3 & \quad \textbf{else if}\ \text{in a parallel region}\ \textbf{then}
 ///     \ \text{a } \mathrm{omp\ taskloop} \text{ on } [ 0 , n )
 ///     \text{ of chunks of 1, tasks of the team} \cr
 /// 4 & \quad \textbf{else}\ \text{a loop } \mathrm{omp\ parallel\ for}
 ///     \text{ on } [ 0 , n ) \text{ of dynamic chunks of 1, on }
 ///     \min\{ p , n \} \text{ threads}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( concurrency() == 1 ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  detail::FirstError error;
#if _OPENMP >= 201511
  if( omp_in_parallel() ) {
   #pragma omp taskloop grainsize( 1 ) shared( error, f )
   for( std::size_t i = 0 ; i < n ; ++i ) {
    if( error.failed() )
     continue;
    try {
     f( i );
     }
    catch( ... ) {
     error.keep();
     }
    }
   error.rethrow();
   return;
   }
#endif
  const long m = long( n );
  #pragma omp parallel for schedule( dynamic, 1 )                             \
    num_threads( int( std::min( concurrency(), n ) ) )
  for( long i = 0 ; i < m ; ++i ) {
   if( error.failed() )
    continue;
   try {
    f( std::size_t( i ) );
    }
   catch( ... ) {
    error.keep();
    }
   }
  error.rethrow();
  }

 /// the calls `f()` and `g()` in parallel, by `bulk()` on 2 indices
 template< class F , class G >
 void invoke( F && f , G && g ) {
  bulk( 2 , [ & ]( std::size_t i ) {
   if( i == 0 )
    f();
   else
    g();
   } );
  }

 /// the lock of the data shared by the workers of `run()`
 using lock_type = OpenMPLock;

 /// the calls \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ of long-lived workers,
 /// at the same time on the \f$ p \f$ threads of a region of OpenMP
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{run}( p , f ) \cr
 /// 2 & \quad \textbf{if}\ p \le 1\ \textbf{then}\ \textbf{for}\ i \gets 0
 ///     \ \textbf{to}\ p - 1\ \textbf{do}\ f( i );\ \textbf{return} \cr
 /// 3 & \quad \text{a region } \mathrm{omp\ parallel} \text{ of } p
 ///     \text{ threads, in which} \cr
 /// 4 & \qquad \textbf{if}\ \text{the team has } p \text{ threads}
 ///     \ \textbf{then}\ f( \text{the number of the thread} ) \cr
 /// 5 & \quad \text{throw if the team was smaller, else throw again the }
 ///     \text{first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \le \f$ `concurrency()`
 template< class F >
 void run( std::size_t p , F && f ) {
  ADS_PRE( p <= concurrency() );
  detail::RunCaller caller;
  if( p <= 1 ) {
   if( p == 1 )
    f( 0 );
   return;
   }
  detail::FirstError error;
  bool smaller = false;
  #pragma omp parallel num_threads( int( p ) )
  {
   if( omp_get_num_threads() != int( p ) ) {
    #pragma omp single
    smaller = true;
    }
   else
    try {
     f( std::size_t( omp_get_thread_num() ) );
     }
    catch( ... ) {
     error.keep();
     }
   }
  if( smaller )
   throw( std::runtime_error( "OpenMPExecutor::run: a team smaller than "
			      "the workers" ) );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 std::size_t f_p;  ///< the threads, 0 for those of OpenMP
 };  // end( class OpenMPExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* _OPENMP */

/*--------------------------------------------------------------------------*/

#endif /* OpenMPExecutor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File OpenMPExecutor.h ------------------------*/
/*--------------------------------------------------------------------------*/
