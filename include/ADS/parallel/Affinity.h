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
 * `pinThread( i )` from the body of the worker \f$ i \f$, never from the
 * thread that hands out the work, which would then stay pinned after the
 * parallel part (an executor that runs one of the calls on the calling
 * thread tells which one it is), and the function knows nothing of the
 * executors (see \ref theory_parallel_exec).
 *
 * The placement is that of hwloc, with `ADS_WITH_HWLOC` (the topology of
 * the machine loaded once, at the first use, the processors read by
 * `hwloc_get_cpubind()`, the thread bound by `hwloc_set_cpubind()` and its
 * processor by `hwloc_get_last_cpu_location()`), which keeps the calls of
 * each system; without hwloc, or where it cannot load the topology,
 * `affinityCpus()` is empty, `availableCpus()` is
 * `std::thread::hardware_concurrency()`, `pinThread()` does nothing and
 * returns false, and `currentCpu()` is \f$ -1 \f$.
 *
 * The contracts [see `Contract.h`]: `affinityCpus()` is called first
 * before any thread of the process is pinned, or it finds the mask of a
 * pinned thread. There is no other operation to define.
 *
 * \see U. Drepper, What every programmer should know about memory, Red
 *      Hat, Inc., 2007, section 6.4 (the placement of the threads and of
 *      the memory)
 * \see F. Broquedis, J. Clet-Ortega, S. Moreaud, N. Furmento, B. Goglin,
 *      G. Mercier, S. Thibault, R. Namyst, hwloc: a generic framework for
 *      managing hardware affinities in HPC applications, 18th Euromicro
 *      Conference on Parallel, Distributed and Network-based Processing
 *      (PDP), 180-186, 2010, doi:10.1109/PDP.2010.67
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

#if defined( ADS_WITH_HWLOC )
 #include <hwloc.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// \cond
namespace detail
{

#if defined( ADS_WITH_HWLOC )

/// the topology of the machine, loaded once by hwloc, nullptr if it cannot
/// be loaded
inline hwloc_topology_t hwlocTopology( void )
{
 struct Topology
 {
  hwloc_topology_t t = nullptr;
  Topology( void ) {
   if( hwloc_topology_init( &t ) != 0 )
    t = nullptr;
   else if( hwloc_topology_load( t ) != 0 ) {
    hwloc_topology_destroy( t );
    t = nullptr;
    }
   }
  ~Topology() {
   if( t )
    hwloc_topology_destroy( t );
   }
  };
 static const Topology T;
 return( T.t );
 }

/// a set of processors of hwloc, freed at the end of its scope
struct HwlocSet
{
 hwloc_bitmap_t s = hwloc_bitmap_alloc();
 HwlocSet( void ) = default;
 HwlocSet( const HwlocSet & ) = delete;
 HwlocSet & operator=( const HwlocSet & ) = delete;
 ~HwlocSet() { hwloc_bitmap_free( s ); }
 };

#endif

}  // end( namespace detail )
/// \endcond

/// the processors on which the process may run, in increasing order, as
/// the first call found them; empty where hwloc does not give them
inline const std::vector< int > & affinityCpus( void )
{
 static const std::vector< int > cpus = [] {
  std::vector< int > c;
#if defined( ADS_WITH_HWLOC )
  const hwloc_topology_t t = detail::hwlocTopology();
  detail::HwlocSet S;
  if( t && S.s && ( hwloc_get_cpubind( t , S.s ,
				       HWLOC_CPUBIND_THREAD ) == 0 ) )
   for( int i = hwloc_bitmap_first( S.s ) ; i >= 0 ;
	i = hwloc_bitmap_next( S.s , i ) )
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
#if defined( ADS_WITH_HWLOC )
 detail::HwlocSet S;
 if( ! S.s )
  return( false );
 hwloc_bitmap_only( S.s , unsigned( c[ i % c.size() ] ) );
 return( hwloc_set_cpubind( detail::hwlocTopology() , S.s ,
			     HWLOC_CPUBIND_THREAD ) == 0 );
#else
 ( void )i;
 return( false );
#endif
 }

/// the processor on which the calling thread runs now, \f$ -1 \f$ where it
/// is not known
inline int currentCpu( void )
{
#if defined( ADS_WITH_HWLOC )
 const hwloc_topology_t t = detail::hwlocTopology();
 detail::HwlocSet S;
 if( t && S.s &&
     ( hwloc_get_last_cpu_location( t , S.s , HWLOC_CPUBIND_THREAD ) == 0 ) )
  return( hwloc_bitmap_first( S.s ) );
#endif
 return( -1 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Affinity.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Affinity.h --------------------------*/
/*--------------------------------------------------------------------------*/
