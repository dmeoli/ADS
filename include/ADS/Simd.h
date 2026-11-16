/*--------------------------------------------------------------------------*/
/*------------------------------ File Simd.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector kernels of ADS on Google Highway, with `ADS_WITH_HIGHWAY`: the
 * loops over arrays that a structure of ADS hands over whole (the 1 bits
 * of an array of words, the intersection of two sorted arrays, the hash of
 * a long key), written once on the portable operations of Highway and
 * compiled for every set of vector instructions that Highway knows on the
 * machine of the compiler (SSE2 to AVX-512 on x86-64, NEON and SVE on
 * ARM, and the others), of which Highway calls, when the program runs, the
 * best that the processor has. The code of ADS then tests no processor,
 * compiler or system, and a program built without any flag of the machine
 * runs the instructions of the processor it finds. A kernel is reached by
 * one call through a table of pointers, which costs as much as a call that
 * is not inlined; the kernels that run on a few bytes in the middle of a
 * search (a group of control bytes of a hash table, a node of a tree) are
 * instead inlined at the instructions given to the compiler [see
 * SimdInline.h], where a call would cost more than the kernel.
 *
 * Without `ADS_WITH_HIGHWAY` this header declares nothing, and every
 * structure runs its scalar code, which gives the same results.
 *
 * Highway dispatches by re-including, for every set of instructions, the
 * file of the kernels ADS/impl/Simd.tpp, once per translation unit
 * (hwy/foreach_target.h may run once per unit, and before any include of
 * hwy/highway.h). Every header of ADS that includes Highway includes this
 * one first, so that the dispatch runs at the first of them; a unit that
 * includes a header of ADS with Highway may therefore neither include
 * hwy/highway.h before it nor run a dispatch of its own, and a program
 * that has kernels of its own on Highway keeps them in units that do not
 * include the structures of ADS.
 *
 * \see J. Wassenberg, J. Alakuijala, Highway: performance-portable, length-
 *      agnostic SIMD with runtime dispatch,
 *      https://github.com/google/highway
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

#ifndef __ADS_Simd
 #define __ADS_Simd
/* self-identification: #endif constant */

#ifdef ADS_WITH_HIGHWAY

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

 #include <cstddef>
 #include <cstdint>
 #include <cstring>
 #include <type_traits>

// the prefetch of the kernels, defined in Prefetch.h, which may not be
// included before the dispatch, since nothing may include hwy/highway.h
// before hwy/foreach_target.h
namespace ADS
{
enum class Locality : int;
namespace detail
{
inline void prefetchLine( const void * p , Locality l ) noexcept;
}  // end( namespace detail )
}  // end( namespace ADS )

 #undef HWY_TARGET_INCLUDE
 #define HWY_TARGET_INCLUDE "ADS/impl/Simd.tpp"
 #include <hwy/foreach_target.h>  // IWYU pragma: keep

 #include <hwy/highway.h>

 #include "ADS/impl/Simd.tpp"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail::simd
{

HWY_EXPORT( popcountWords );
HWY_EXPORT( popcountAndWords );

/// the 1 bits of the words \f$ w[ 0 .. n - 1 ] \f$
inline std::uint64_t popcount( const std::uint64_t * w , std::size_t n )
{
 return( HWY_DYNAMIC_DISPATCH( popcountWords )( w , n ) );
 }

/// the 1 bits of the words \f$ a[ i ] \wedge b[ i ] \f$, \f$ 0 \le i < n \f$
inline std::uint64_t popcountAnd( const std::uint64_t * a ,
				  const std::uint64_t * b , std::size_t n )
{
 return( HWY_DYNAMIC_DISPATCH( popcountAndWords )( a , b , n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::detail::simd )

 #include "ADS/memory/Prefetch.h"  // the prefetch of the kernels

#endif

/*--------------------------------------------------------------------------*/

#endif /* Simd.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Simd.h -----------------------------*/
/*--------------------------------------------------------------------------*/
