/*--------------------------------------------------------------------------*/
/*--------------------------- File TBBExecutor.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the threads of oneTBB, beyond the book,
 * compiled only with `ADS_WITH_TBB` (the CMake option of the same name, OFF
 * by default) and oneTBB on the path: the executor runs in a `task_arena`,
 * `bulk( n , f )` is a `parallel_for` of oneTBB on the \f$ n \f$ indices
 * with the simple partitioner (a task per index, since the patterns of ADS
 * already give it blocks), and `invoke( f , g )` is `parallel_invoke`, both
 * run in the arena, so that the parallel algorithms of ADS run on the
 * scheduler of oneTBB unchanged, and nest as its tasks do. The arena is one
 * of three: its own, of \f$ p \f$ threads (`TBBExecutor( p )`); one of the
 * program, which the executor refers to without owning it
 * (`TBBExecutor( arena )`), so that the constraints of that arena (its NUMA
 * node, its type of core, its priority, its number of threads) hold for the
 * calls of ADS too; or the arena of the thread that makes each call
 * (`TBBExecutor( TBBExecutor::current )`), so that an algorithm of ADS
 * called from inside a loop of oneTBB of the program runs in the arena of
 * that loop, whatever it is. Since the arenas of a process share the
 * workers of oneTBB, none of the three starts threads of its own; the last
 * two add nothing to the calls of oneTBB but `execute()`. The thread that
 * calls joins the arena and works. `run( p , f )` is the
 * same `parallel_for` on the \f$ p \f$ calls of long-lived workers, each
 * made inside `this_task_arena::isolate()`, so that a thread that waits
 * inside a call (for a job nested in it) takes only the tasks of that job,
 * never the call of another worker, which it would hold under its own; the
 * \f$ p \f$ calls then run at the same time as soon as the arena has
 * \f$ p \f$ threads free, i.e., when the threads of oneTBB are not fewer,
 * which \f$ p \f$ not larger than the hardware threads ensures for an arena
 * of its own (an arena of the program must have them free as well). The
 * caller joins the arena and may make any of the calls or none of them, so
 * that `isRunCaller()` holds in at most one call, whose index is not known
 * in advance. Its lock is the spin mutex of oneTBB (`tbb::spin_mutex`), for
 * the short sections of the workers.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; an exception thrown by
 * \f$ f \f$ is thrown again by `bulk()`, as oneTBB does, and by `run()`
 * once all the calls have returned.
 *
 * \see J. Reinders, Intel Threading Building Blocks: Outfitting C++ for
 *      Multi-core Processor Parallelism, O'Reilly, 2007
 * \see M. Voss, R. Asenjo, J. Reinders, Pro TBB: C++ Parallel Programming
 *      with Threading Building Blocks, Apress, 2019,
 *      doi:10.1007/978-1-4842-4398-5
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

#ifndef __ADS_TBBExecutor
 #define __ADS_TBBExecutor
/* self-identification: #endif constant */

#if defined( ADS_WITH_TBB )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/partitioner.h>
#include <tbb/spin_mutex.h>
#include <tbb/task_arena.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <thread>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS TBBExecutor ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on an arena of oneTBB
class TBBExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the tag of the executor on the arena of the calling thread
 struct Current
 { };

 /// the executor on the arena of the thread that makes each call [see
 /// `TBBExecutor( Current )`]
 static constexpr Current current{};

 /// the executor of an arena of its own, of \f$ p \f$ threads (the
 /// hardware threads if \f$ p = 0 \f$)
 /// \pre \f$ p \ge 1 \f$ once the hardware threads are counted
 explicit TBBExecutor( std::size_t p = 0 )
     : f_p( p ? p
	      : std::max< std::size_t >( std::thread::hardware_concurrency() ,
					 1 ) ) ,
       f_own( std::in_place , int( f_p ) ) {
  ADS_PRE( f_p >= 1 );
  }

 /// the executor on the arena of the program, which it does not own and
 /// which must outlive it; its threads are those of the arena
 explicit TBBExecutor( tbb::task_arena & arena )
     : f_p( 0 ) , f_user( &arena ) { }

 /// the executor on the arena of the thread that makes each call (the
 /// arena of a loop of oneTBB that calls an algorithm of ADS, the implicit
 /// arena of the process elsewhere); its threads are those of that arena
 explicit TBBExecutor( Current ) : f_p( 0 ) { }

 /// the number of threads of the arena: \f$ p \f$ for its own, the
 /// concurrency of the arena of the program, or of the current one
 [[nodiscard]] std::size_t concurrency( void ) const {
  if( f_p )
   return( f_p );
  const int c = f_user ? f_user->max_concurrency()
		       : tbb::this_task_arena::max_concurrency();
  return( std::size_t( c > 0 ? c : 1 ) );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ on the threads of the
 /// arena
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i ) \cr
 /// 3 & \quad \textbf{else}\ \text{in the arena, the } \mathrm{parallel\_for}
 ///     \text{ of oneTBB on } [ 0 , n ) \text{, a task per index}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( concurrency() == 1 ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  execute( [ & ] {
   tbb::parallel_for(
     tbb::blocked_range< std::size_t >( 0 , n , 1 ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & r ) {
      for( std::size_t i = r.begin() ; i < r.end() ; ++i )
       f( i );
      } ,
     tbb::simple_partitioner() );
   } );
  }

 /// the calls `f()` and `g()` in parallel, by the `parallel_invoke` of
 /// oneTBB in the arena
 template< class F , class G >
 void invoke( F && f , G && g ) {
  if( concurrency() == 1 ) {
   f();
   g();
   return;
   }
  execute( [ & ] { tbb::parallel_invoke( f , g ); } );
  }

 /// the lock of the data shared by the workers of `run()`
 using lock_type = tbb::spin_mutex;

 /// the calls \f$ f( 0 ) , \ldots , f( p - 1 ) \f$ of long-lived workers,
 /// at the same time on \f$ p \f$ threads of the arena
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{run}( p , f ) \cr
 /// 2 & \quad \textbf{if}\ p \le 1\ \textbf{then}\ \textbf{for}\ i \gets 0
 ///     \ \textbf{to}\ p - 1\ \textbf{do}\ f( i );\ \textbf{return} \cr
 /// 3 & \quad \text{in the arena, the } \mathrm{parallel\_for} \text{ of }
 ///     \text{oneTBB on } [ 0 , p ) \text{, a task per index } i
 ///     \text{ making } f( i ) \cr
 /// & \qquad \text{inside } \mathrm{isolate}() \cr
 /// 4 & \quad \text{throw again the first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \le \f$ `concurrency()`, and not more than the hardware
 ///       threads
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
  execute( [ & ] {
   tbb::parallel_for(
     tbb::blocked_range< std::size_t >( 0 , p , 1 ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & r ) {
      for( std::size_t i = r.begin() ; i < r.end() ; ++i )
       tbb::this_task_arena::isolate( [ & ] {
	try {
	 f( i );
	 }
	catch( ... ) {
	 error.keep();
	 }
        } );
      } ,
     tbb::simple_partitioner() );
   } );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the call `body()` in the arena of the executor
 template< class B >
 void execute( B && body ) {
  if( f_user )
   f_user->execute( body );
  else if( f_own )
   f_own->execute( body );
  else
   body();
  }

 std::size_t f_p;                         ///< the threads of its own arena
 std::optional< tbb::task_arena > f_own;  ///< its own arena, if any
 tbb::task_arena * f_user = nullptr;      ///< the arena of the program
 };  // end( class TBBExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_TBB */

/*--------------------------------------------------------------------------*/

#endif /* TBBExecutor.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File TBBExecutor.h -------------------------*/
/*--------------------------------------------------------------------------*/
