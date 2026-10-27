/*--------------------------------------------------------------------------*/
/*--------------------------- File TBBExecutor.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the threads of oneTBB, beyond the book,
 * compiled only with `ADS_WITH_TBB` (the CMake option of the same name, OFF
 * by default) and oneTBB on the path: the executor holds a `task_arena` of
 * \f$ p \f$ threads, `bulk( n , f )` is a `parallel_for` of oneTBB on the
 * \f$ n \f$ indices with the simple partitioner (a task per index, since the
 * patterns of ADS already give it blocks), and `invoke( f , g )` is
 * `parallel_invoke`, both run in the arena, so that the parallel algorithms
 * of ADS run on the scheduler of oneTBB unchanged, and nest as its tasks do.
 * The thread that calls joins the arena and works.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; an exception thrown by
 * \f$ f \f$ is thrown again by `bulk()`, as oneTBB does.
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
#include <tbb/task_arena.h>

#include <algorithm>
#include <cstddef>
#include <thread>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS TBBExecutor ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on an arena of oneTBB
class TBBExecutor {

/*--------------------------------------------------------------------------*/
 public:

 /// the executor of an arena of \f$ p \f$ threads (the hardware threads if
 /// \f$ p = 0 \f$)
 /// \pre \f$ p \ge 1 \f$ once the hardware threads are counted
 explicit TBBExecutor( std::size_t p = 0 )
  : f_p( p ? p : std::max< std::size_t >(
                  std::thread::hardware_concurrency() , 1 ) ) ,
    f_arena( int( f_p ) ) {
  ADS_PRE( f_p >= 1 );
  }

 /// the number of threads of the arena
 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

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
  if( ( n <= 1 ) || ( f_p == 1 ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  f_arena.execute( [ & ] {
   tbb::parallel_for( tbb::blocked_range< std::size_t >( 0 , n , 1 ) ,
                      [ & ]( const tbb::blocked_range< std::size_t > & r ) {
                       for( std::size_t i = r.begin() ; i < r.end() ; ++i )
                        f( i );
                       } , tbb::simple_partitioner() );
   } );
  }

 /// the calls `f()` and `g()` in parallel, by the `parallel_invoke` of
 /// oneTBB in the arena
 template< class F , class G >
 void invoke( F && f , G && g ) {
  if( f_p == 1 ) {
   f();
   g();
   return;
   }
  f_arena.execute( [ & ] { tbb::parallel_invoke( f , g ); } );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_p;                          ///< the threads
 tbb::task_arena f_arena;                  ///< where they run
 };  // end( class TBBExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* ADS_WITH_TBB */

/*--------------------------------------------------------------------------*/

#endif  /* TBBExecutor.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File TBBExecutor.h -------------------------*/
/*--------------------------------------------------------------------------*/
