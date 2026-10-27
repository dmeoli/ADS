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
 * FastFlow unchanged. The `ParallelFor` is built once with the executor, and
 * its \f$ p \f$ workers wait between the calls by spinning if `spin` is true
 * (the default of the executor, not of FastFlow), or sleeping otherwise; the
 * thread that calls `bulk()` waits for them and does not work. A call of
 * `bulk()` from a call of `bulk()` runs its calls in order on that thread,
 * since a `ParallelFor` does not nest.
 *
 * Unless `ADS_FASTFLOW_MAPPING` is defined, the header defines
 * `NO_DEFAULT_MAPPING` before it includes FastFlow: by default FastFlow pins
 * its \f$ i \f$-th thread to the \f$ i \f$-th core of the machine,
 * regardless of the cores the process may run on (e.g., under `taskset`),
 * and two programs pinned on the same cores slow each other down.
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

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! defined( ADS_FASTFLOW_MAPPING ) && ! defined( NO_DEFAULT_MAPPING )
 #define NO_DEFAULT_MAPPING
#endif

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <mutex>
#include <thread>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

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
    f_for( long( f_p ) , spin , spin ) {
  ADS_PRE( f_p >= 1 );
  f_for.disableScheduler( true );
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
  f_for.parallel_for( 0 , long( n ) , 1 , 1 , [ & ]( const long i ) {
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

/*--------------------------------------------------------------------------*/
 private:

 static inline thread_local bool tl_inside = false;   ///< in a call of f

 std::size_t f_p;                          ///< the workers
 ff::ParallelFor f_for;                    ///< their loop
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
