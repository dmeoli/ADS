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

#if defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_IX86 ) )
 #include <intrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the hint to the processor that the thread spins on a lock
inline void spinPause( void )
{
#if defined( __x86_64__ ) || defined( __i386__ )
 __builtin_ia32_pause();
#elif defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_IX86 ) )
 _mm_pause();
#elif defined( __aarch64__ )
 asm volatile( "yield" );
#endif
 }

/// the line of cache of p fetched ahead, for reading
inline void prefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p );
#else
 (void) p;
#endif
 }

}  // end( namespace detail )

}  // end( namespace ADS )

#endif  /* Spin.tpp included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Spin.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
