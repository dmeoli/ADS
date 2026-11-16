/*--------------------------------------------------------------------------*/
/*----------------------------- File Spin.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * What the structures of the area shared by threads use while they wait
 * or before they read: the pause of the processor in a loop that spins,
 * and the line of cache of an address fetched ahead.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Spin
 #define __ADS_Spin
/* self-identification: #endif constant */

#include "ADS/memory/Prefetch.h"

#include <atomic>

// the pause of Boost.Core, which knows the instruction of each processor
#if __has_include( <boost/core/yield_primitives.hpp> )
 #include <boost/core/yield_primitives.hpp>
 #define ADS_SPIN_BOOST 1
#else
 #define ADS_SPIN_BOOST 0
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the hint to the processor that the thread spins on a lock: the pause
/// of Boost.Core (`pause` on x86, `yield` on ARM), or with no Boost only a
/// barrier of the compiler, so that the loop still reads memory again
inline void spinPause( void )
{
#if ADS_SPIN_BOOST
 boost::core::sp_thread_pause();
#else
 std::atomic_signal_fence( std::memory_order_seq_cst );
#endif
 }

/// the line of cache of p fetched ahead, for reading
inline void prefetch( const void * p )
{
 ADS::detail::prefetchLine( p );
 }

}  // end( namespace detail )

}  // end( namespace ADS )

#endif /* Spin.tpp included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Spin.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
