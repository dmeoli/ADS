/*--------------------------------------------------------------------------*/
/*------------------------- File OpenMPExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see Executor.h] on the threads of OpenMP, beyond the book,
 * compiled only when the compiler has OpenMP on (_OPENMP, e.g., -fopenmp,
 * or the CMake option ADS_WITH_OPENMP, OFF by default): bulk( n , f ) is
 * a loop `#pragma omp parallel for` on n indices with the dynamic schedule
 * of chunks of 1 on min( p , n ) threads, invoke( f , g ) two sections,
 * so that the parallel algorithms of ADS run on the runtime of OpenMP of
 * the compiler (the libgomp of GCC, the libomp of LLVM) unchanged. The
 * thread that calls is one of the p, as in every region of OpenMP. A call
 * nested in another runs on as many threads as the runtime gives a nested
 * region (by default 1, i.e., in order on that thread).
 *
 * The contracts [see Contract.h]: p >= 1; an exception thrown by f stops
 * the claims and is thrown again by bulk() (an exception may not leave a
 * region of OpenMP).
 * \see OpenMP Architecture Review Board, OpenMP Application Programming
 *      Interface, version 5.2, 2021, https://www.openmp.org/spec-html/5.2/
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS OpenMPExecutor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on the threads of OpenMP
class OpenMPExecutor {

/*--------------------------------------------------------------------------*/
 public:

 /// the executor of p threads of OpenMP (omp_get_max_threads() if 0)
 explicit OpenMPExecutor( std::size_t p = 0 )
  : f_p( p ? p : std::size_t( omp_get_max_threads() ) ) {
  ADS_PRE( f_p >= 1 );
  }

 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// f( 0 ) , ... , f( n - 1 ) on the threads
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

 /// f() and g() in parallel
 template< class F , class G >
 void invoke( F && f , G && g ) {
  bulk( 2 , [ & ]( std::size_t i ) { if( i == 0 ) f(); else g(); } );
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
