/*--------------------------------------------------------------------------*/
/*------------------------ File InteropExecutors.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executors on the runtimes of a program, for the tests of the parallel
 * algorithms: `onEachInterop( f )` makes the call `f( ex )` on each of those
 * compiled in, i.e., `TBBExecutor` on an arena of the program and on the
 * current arena (inside the `execute()` of another arena, so that the
 * current arena is not the implicit one), `OpenMPExecutor` called inside a
 * region `omp parallel` + `omp single` (its taskloop), and the executors of
 * `ADS/interop/parallel/` whose runtime the test is built with
 * (`ADS_WITH_STDPAR`, `ADS_WITH_STDEXEC`, `ADS_WITH_TASKFLOW`,
 * `ADS_WITH_KOKKOS`, defined by the CMake targets `ADS::StdPar`,
 * `ADS::stdexec`, `ADS::Taskflow` and `ADS::Kokkos`). A failure inside a
 * region of OpenMP is carried out of it and thrown again, since an
 * exception may not leave a region.
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

#ifndef __ADS_InteropExecutors
 #define __ADS_InteropExecutors
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/TBBExecutor.h"

#if defined( ADS_WITH_STDPAR )
 #include "ADS/interop/parallel/StdParExecutor.h"
#endif
#if defined( ADS_WITH_STDEXEC )
 #include "ADS/interop/parallel/SenderExecutor.h"

 #include <exec/static_thread_pool.hpp>
#endif
#if defined( ADS_WITH_TASKFLOW )
 #include "ADS/interop/parallel/TaskflowExecutor.h"
#endif
#if defined( ADS_WITH_KOKKOS )
 #include "ADS/interop/parallel/KokkosExecutor.h"
#endif

#include <exception>

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::test
{

#if defined( ADS_WITH_KOKKOS )
/// Kokkos, initialized once for the whole program
inline void kokkosOnce( void )
{
 static Kokkos::ScopeGuard guard;
 }
#endif

/// f( ex ) on each executor of a runtime of the program compiled in
template< class F >
void onEachInterop( [[maybe_unused]] F && f )
{
#if defined( ADS_WITH_TBB )
 {
  tbb::task_arena arena( 3 ) , other( 2 );
  TBBExecutor user( arena );
  f( user );
  other.execute( [ & ] {
   TBBExecutor current( TBBExecutor::current );
   f( current );
   } );
  }
#endif
#if defined( _OPENMP ) && ( _OPENMP >= 201511 )
 {
  std::exception_ptr error;
  #pragma omp parallel num_threads( 3 )
  #pragma omp single
  {
   try {
    OpenMPExecutor team;
    f( team );
    }
   catch( ... ) {
    error = std::current_exception();
    }
   }
  if( error )
   std::rethrow_exception( error );
  }
#endif
#if defined( ADS_WITH_STDPAR )
 {
  StdParExecutor<> par( 4 );
  f( par );
  }
#endif
#if defined( ADS_WITH_STDEXEC )
 {
  exec::static_thread_pool pool( 3 );
  SenderExecutor sender( pool.get_scheduler() , 3 );
  f( sender );
  }
#endif
#if defined( ADS_WITH_TASKFLOW )
 {
  tf::Executor workers( 3 );
  TaskflowExecutor flow( workers );
  f( flow );
  }
#endif
#if defined( ADS_WITH_KOKKOS )
 {
  kokkosOnce();
  KokkosExecutor<> kokkos;
  f( kokkos );
  }
#endif
 }

}  // end( namespace ADS::test )

/*--------------------------------------------------------------------------*/

#endif /* InteropExecutors.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File InteropExecutors.h ----------------------*/
/*--------------------------------------------------------------------------*/
