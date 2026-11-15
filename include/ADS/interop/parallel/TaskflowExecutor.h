/*--------------------------------------------------------------------------*/
/*------------------------ File TaskflowExecutor.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on a `tf::Executor` of Taskflow, beyond
 * the book: `TaskflowExecutor` refers to the executor of Taskflow of the
 * program, which it does not own, and runs `bulk( n , f )` as a graph of
 * Taskflow made of one `for_each_index` on the \f$ n \f$ indices with the
 * dynamic partitioner of chunks of 1 (the patterns of ADS already give it
 * blocks). From a thread outside the executor the graph is run and waited
 * for (`run( tf ).wait()`); from one of its workers, i.e., in a call nested
 * in another, it is co-run (`corun( tf )`), so that the worker keeps running
 * the tasks of Taskflow while it waits instead of holding its thread, and
 * the calls nest. `concurrency()` is the number of the workers of Taskflow.
 *
 * The cost is the graph, built at each call of `bulk()` with its nodes on
 * the heap of Taskflow, which the executors of ADS, oneTBB and OpenMP do not
 * pay; the patterns of ADS keep the calls of `bulk()` to one per loop, of at
 * most `kBlocks` indices. There is no `run()`, since Taskflow does not
 * promise that the tasks of a graph run at the same time.
 *
 * The header is compiled where the headers of Taskflow 3.7 or later are
 * found (`<taskflow/taskflow.hpp>`, or `ADS_WITH_TASKFLOW`, which the CMake
 * target `ADS::Taskflow` defines).
 *
 * The contracts [see `Contract.h`]: an exception thrown by \f$ f \f$ stops
 * the claims and is thrown again by `bulk()`.
 *
 * \see T.-W. Huang, D.-L. Lin, C.-X. Lin, Y. Lin, Taskflow: a lightweight
 *      parallel and heterogeneous task graph computing system, IEEE
 *      Transactions on Parallel and Distributed Systems 33(6), 1303-1320,
 *      2022, doi:10.1109/TPDS.2021.3104255
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

#ifndef __ADS_TaskflowExecutor
 #define __ADS_TaskflowExecutor
/* self-identification: #endif constant */

#if defined( ADS_WITH_TASKFLOW ) || __has_include( <taskflow/taskflow.hpp> )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/taskflow.hpp>

#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS TaskflowExecutor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on an executor of Taskflow
class TaskflowExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the executor on the executor of Taskflow \f$ e \f$, which it does not
 /// own and which must outlive it
 explicit TaskflowExecutor( tf::Executor & e ) : f_e( &e ) { }

 /// the number of the workers of Taskflow
 [[nodiscard]] std::size_t concurrency( void ) const {
  const std::size_t w = f_e->num_workers();
  return( w ? w : 1 );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ by a graph of Taskflow,
 /// co-run from a worker of Taskflow
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i ) \cr
 /// 3 & \quad \mathit{tf} \gets \text{a graph of one } \mathrm{forEachIndex}
 ///     \text{ on } [ 0 , n ) \text{, dynamic chunks of 1} \cr
 /// 4 & \quad \textbf{if}\ \text{the caller is a worker of Taskflow}
 ///     \ \textbf{then}\ \mathrm{corun}( \mathit{tf} )
 ///     \ \textbf{else}\ \mathrm{run}( \mathit{tf} ).\mathrm{wait}() \cr
 /// 5 & \quad \text{throw again the first exception of the calls, if any}
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
  tf::Taskflow tf;
  tf.for_each_index(
    std::size_t( 0 ) , n , std::size_t( 1 ) ,
    [ & ]( std::size_t i ) {
     if( error.failed() )
      return;
     try {
      f( i );
      }
     catch( ... ) {
      error.keep();
      }
     } ,
    tf::DynamicPartitioner( 1 ) );
  if( f_e->this_worker_id() >= 0 )
   f_e->corun( tf );
  else
   f_e->run( tf ).wait();
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 tf::Executor * f_e;  ///< the executor of Taskflow
 };  // end( class TaskflowExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_TASKFLOW */

/*--------------------------------------------------------------------------*/

#endif /* TaskflowExecutor.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File TaskflowExecutor.h -----------------------*/
/*--------------------------------------------------------------------------*/
