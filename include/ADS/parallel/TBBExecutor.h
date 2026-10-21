/*--------------------------------------------------------------------------*/
/*--------------------------- File TBBExecutor.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see Executor.h] on the threads of oneTBB, beyond the book,
 * compiled only with ADS_WITH_TBB (the CMake option of the same name, OFF
 * by default) and oneTBB on the path: the executor holds a task_arena of p
 * threads, bulk( n , f ) is a parallel_for of oneTBB on the n indices with
 * the simple partitioner (a task per index, since the patterns of ADS
 * already give it blocks), and invoke( f , g ) is parallel_invoke, both
 * run in the arena, so that the parallel algorithms of ADS run on the
 * scheduler of oneTBB unchanged, and nest as its tasks do. The thread that
 * calls joins the arena and works.
 *
 * The contracts [see Contract.h]: p >= 1; an exception thrown by f is
 * thrown again by bulk(), as oneTBB does.
 * \see J. Reinders, Intel Threading Building Blocks, O'Reilly, 2007
 * \see M. Voss, R. Asenjo, J. Reinders, Pro TBB: C++ Parallel Programming
 *      with Threading Building Blocks, Apress, 2019,
 *      doi:10.1007/978-1-4842-4398-5
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

 /// the executor of an arena of p threads (the hardware threads if 0)
 explicit TBBExecutor( std::size_t p = 0 )
  : f_p( p ? p : std::max< std::size_t >(
                  std::thread::hardware_concurrency() , 1 ) ) ,
    f_arena( int( f_p ) ) {
  ADS_PRE( f_p >= 1 );
  }

 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// f( 0 ) , ... , f( n - 1 ) on the threads of the arena
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

 /// f() and g() in parallel
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
