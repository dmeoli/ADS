/*--------------------------------------------------------------------------*/
/*------------------------- File OpenMPExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the threads of OpenMP, beyond the book,
 * compiled only when the compiler has OpenMP on (`_OPENMP`, e.g.,
 * `-fopenmp`, or the CMake option `ADS_WITH_OPENMP`, OFF by default):
 * `bulk( n , f )` is a loop `#pragma omp parallel for` on \f$ n \f$ indices
 * with the dynamic schedule of chunks of 1 on \f$ \min\{ p , n \} \f$
 * threads, and `invoke( f , g )` is `bulk()` on 2 indices, so that the
 * parallel algorithms of ADS run on the runtime of OpenMP of the compiler
 * (the libgomp of GCC, the libomp of LLVM) unchanged. The thread that calls
 * is one of the \f$ p \f$, as in every region of OpenMP. A call nested in
 * another runs on as many threads as the runtime gives a nested region (by
 * default 1, i.e., in order on that thread). `run( p , f )` is a region
 * `omp parallel` of \f$ p \f$ threads, the thread \f$ i \f$ of the team
 * making the call \f$ f( i ) \f$ of a long-lived worker, and its lock is
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
#include <cstddef>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS OpenMPLock ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the lock of OpenMP, `omp_lock_t`, as a mutex of the standard
class OpenMPLock {
 public:

 /// an unlocked lock
 OpenMPLock( void ) { omp_init_lock( &f_lock ); }
 /// no copy, as a mutex
 OpenMPLock( const OpenMPLock & ) = delete;
 /// no copy, as a mutex
 OpenMPLock & operator=( const OpenMPLock & ) = delete;
 /// destroys the lock, which nobody holds
 ~OpenMPLock() { omp_destroy_lock( &f_lock ); }

 /// takes the lock, waiting for it
 void lock( void ) { omp_set_lock( &f_lock ); }
 /// gives the lock back
 void unlock( void ) { omp_unset_lock( &f_lock ); }
 /// takes the lock if it is free, and tells whether it did
 bool try_lock( void ) { return( omp_test_lock( &f_lock ) != 0 ); }

 private:

 omp_lock_t f_lock;                        ///< the lock of OpenMP
 };  // end( class OpenMPLock )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OpenMPExecutor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on the threads of OpenMP
class OpenMPExecutor {

/*--------------------------------------------------------------------------*/
 public:

 /// the executor of \f$ p \f$ threads of OpenMP (`omp_get_max_threads()`
 /// if \f$ p = 0 \f$)
 /// \pre \f$ p \ge 1 \f$
 explicit OpenMPExecutor( std::size_t p = 0 )
  : f_p( p ? p : std::size_t( omp_get_max_threads() ) ) {
  ADS_PRE( f_p >= 1 );
  }

 /// the number of threads
 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on the threads
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i ) \cr
 /// 3 & \quad \textbf{else}\ \text{a loop } \mathrm{omp\ parallel\ for}
 ///     \text{ on } [ 0 , n ) \text{ of dynamic chunks of 1, on }
 ///     \min\{ p , n \} \text{ threads}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( f_p == 1 ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  detail::FirstError error;
  const long m = long( n );
  #pragma omp parallel for schedule( dynamic , 1 ) \
                           num_threads( int( std::min( f_p , n ) ) )
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
  bulk( 2 , [ & ]( std::size_t i ) { if( i == 0 ) f(); else g(); } );
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
  ADS_PRE( p <= f_p );
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

 std::size_t f_p;                          ///< the threads
 };  // end( class OpenMPExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* _OPENMP */

/*--------------------------------------------------------------------------*/

#endif  /* OpenMPExecutor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File OpenMPExecutor.h ------------------------*/
/*--------------------------------------------------------------------------*/
