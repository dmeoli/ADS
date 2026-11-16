/*--------------------------------------------------------------------------*/
/*---------------------------- File Prefetch.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The prefetch of a line of the cache, beyond the book: `ADS::prefetch( p )`
 * asks the processor to bring the line that holds the byte at \f$ p \f$
 * into its caches, and returns at once, so that a load issued later finds
 * the line there instead of waiting for the memory. It is a hint: it never
 * faults, whatever \f$ p \f$ points to (a null pointer, a place past an
 * array, memory freed), it changes nothing that the program can read, and
 * the processor may ignore it. The structures of ADS issue it where they
 * know the next address before they need it (the slots of a group of a
 * hash table while its control bytes are compared, the next node of a
 * search, the item a few steps ahead of a scan), and this is the one place
 * of ADS that issues it. `ADS::prefetch` is the customization point of
 * Capabilities.h, whose form `prefetch( s , k )` asks a structure \f$ s \f$
 * for the places of a key \f$ k \f$, and whose forms on a pointer, which
 * this header adds, ask for a line of memory; both are noexcept.
 *
 * With `ADS_WITH_HIGHWAY` the prefetch is `hwy::Prefetch()` of Google
 * Highway, which keeps the line in every level of the cache; without it,
 * `prefetch()` does nothing, which changes the time of a search and never
 * its result. The locality of the second form tells how soon and how often
 * the line will be read again, as the levels of the prefetch of the
 * processors do; Highway gives a single level, so that every locality
 * issues the same prefetch for now, and a structure that knows it reads a
 * line only once still says so, for the backends that tell them apart.
 *
 * \see T. C. Mowry, M. S. Lam, A. Gupta, Design and evaluation of a
 *      compiler algorithm for prefetching, Proceedings of the 5th
 *      International Conference on Architectural Support for Programming
 *      Languages and Operating Systems (ASPLOS V), 62-73, 1992,
 *      doi:10.1145/143365.143488
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

#ifndef __ADS_Prefetch
 #define __ADS_Prefetch
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#ifdef ADS_WITH_HIGHWAY
 #include <hwy/cache_control.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// how soon and how often a prefetched line is read again: `None` once
/// (keep it out of the caches it would pollute), `Low` once more soon,
/// `Moderate` a few times, `High` (the default) often, in every level
enum class Locality : int { None = 0 , Low = 1 , Moderate = 2 , High = 3 };

namespace detail
{

/// the prefetch of the line of \f$ p \f$, with the locality \f$ l \f$,
/// which the backend may not tell apart [see the file]
//! [prefetch]
inline void prefetchLine( const void * p ,
			  Locality l = Locality::High ) noexcept
{
 ( void )l;
#ifdef ADS_WITH_HIGHWAY
 hwy::Prefetch( static_cast< const unsigned char * >( p ) );
#else
 ( void )p;
#endif
 }
//! [prefetch]

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

// the customization point ADS::prefetch, whose forms prefetch( p ) and
// prefetch( p , l ) call prefetchLine()
#include "ADS/Capabilities.h"

/*--------------------------------------------------------------------------*/

#endif /* Prefetch.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Prefetch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
