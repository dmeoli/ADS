/*--------------------------------------------------------------------------*/
/*---------------------------- File Affinity.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The placement of the threads on the processors, beyond the book: a
 * worker of a parallel algorithm pinned to a processor of its own keeps its
 * caches, its pages first touched near its memory on a machine of several
 * sockets (the policy of the first touch of the NUMA systems), and its
 * timings repeatable, while the scheduler of the system may move an
 * unpinned thread at every switch. `affinityCpus()` gives the processors on
 * which the process may run, as the call of the first use found them
 * (those given to `taskset` or to `numactl`, in increasing order), and
 * `pinThread( i )` binds the calling thread to the processor \f$ i \bmod c \f$
 * of them, \f$ c \f$ their number, so that the workers
 * \f$ 0 , 1 , \ldots \f$ of a pool, each pinning itself at its start, fill
 * the processors given to the process in their order. The executors do not
 * pin their threads [see `ThreadPool.h`]: whoever wants it calls
 * `pinThread()` from the work of each thread (see
 * \ref theory_parallel_exec).
 *
 * The placement exists on Linux (`sched_getaffinity()`,
 * `pthread_setaffinity_np()`), where the processors are the bits of the
 * mask of the process; elsewhere `affinityCpus()` is empty,
 * `availableCpus()` is `std::thread::hardware_concurrency()`, and
 * `pinThread()` does nothing and returns false.
 *
 * The contracts [see `Contract.h`]: `affinityCpus()` is called first
 * before any thread of the process is pinned, or it finds the mask of a
 * pinned thread. There is no other operation to define.
 *
 * \see U. Drepper, What every programmer should know about memory, Red
 *      Hat, Inc., 2007, section 6.4 (the placement of the threads and of
 *      the memory)
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

#ifndef __ADS_Affinity
 #define __ADS_Affinity
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <thread>
#include <vector>

#if defined( __linux__ ) && __has_include( <sched.h> ) && \
    __has_include( <pthread.h> )
 #include <pthread.h>
 #include <sched.h>
 #define ADS_AFFINITY_LINUX 1
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the processors on which the process may run, in increasing order, as
/// the first call found them; empty where the placement does not exist
inline const std::vector< int > & affinityCpus( void )
{
 static const std::vector< int > cpus = [] {
  std::vector< int > c;
#ifdef ADS_AFFINITY_LINUX
  cpu_set_t s;
  CPU_ZERO( &s );
  if( sched_getaffinity( 0 , sizeof( s ) , &s ) == 0 )
   for( int i = 0 ; i < CPU_SETSIZE ; ++i )
    if( CPU_ISSET( i , &s ) )
     c.push_back( i );
#endif
  return( c );
  }();
 return( cpus );
 }

/// the number of processors on which the process may run, that of
/// `affinityCpus()` where it is not empty and
/// `std::thread::hardware_concurrency()` otherwise (0 if not known)
inline unsigned availableCpus( void )
{
 const std::vector< int > & c = affinityCpus();
 return( c.empty() ? std::thread::hardware_concurrency()
                   : unsigned( c.size() ) );
 }

/// binds the calling thread to the processor \f$ i \bmod c \f$ of
/// `affinityCpus()`, \f$ c \f$ their number; true if the thread is bound,
/// false where the placement does not exist or the system refuses it
///
/// In pseudocode (beyond the book), \f$ C \f$ the processors of
/// `affinityCpus()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pinThread}( i ) \cr
/// 2 & \quad \textbf{if}\ C = \emptyset\ \textbf{then return}
///     \ \textbf{false} \cr
/// 3 & \quad \textbf{return}\ \text{whether the system binds the thread }
///     \text{to } C[ i \bmod |C| ]
/// \end{array}}
/// \f]
inline bool pinThread( unsigned i )
{
 const std::vector< int > & c = affinityCpus();
 if( c.empty() )
  return( false );
#ifdef ADS_AFFINITY_LINUX
 cpu_set_t s;
 CPU_ZERO( &s );
 CPU_SET( c[ i % c.size() ] , &s );
 return( pthread_setaffinity_np( pthread_self() , sizeof( s ) , &s ) ==
         0 );
#else
 (void) i;
 return( false );
#endif
 }

/// the processor on which the calling thread runs now, \f$ -1 \f$ where it
/// is not known
inline int currentCpu( void )
{
#ifdef ADS_AFFINITY_LINUX
 return( sched_getcpu() );
#else
 return( -1 );
#endif
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Affinity.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Affinity.h --------------------------*/
/*--------------------------------------------------------------------------*/
