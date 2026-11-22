/*--------------------------------------------------------------------------*/
/*------------------------------ File Abi.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The name of the configuration of the layouts, an inline namespace: the
 * headers whose classes change their layout or their inline code with the
 * flags of the compiler declare them in `inline namespace ADS_ABI_NAME`,
 * opened by `ADS_ABI_BEGIN` and closed by `ADS_ABI_END`, whose name spells
 * the options that change them. These are the matching of the control bytes
 * of the hash tables (`ADS_FLAT_SIMD`: 0 broadword, 1 by the vectors of
 * Highway), the compare-and-swap of 16 bytes of `ConcurrentFlatTable`
 * (`ADS_FLAT_CAS16`, that of Boost.Atomic, which `-mcx16` and
 * `-march=x86-64-v2` or later make free of locks on x86-64), the pairs of
 * `boost::unordered_flat_map` in `FlatHashTable` (`ADS_FLATHASH_BOOST`) and
 * the set of vector instructions at which Highway compiles the kernels
 * inlined in the searches [see SimdInline.h], the namespace that Highway
 * gives it (`none` without Highway), e.g., `abi_s1c1b0_N_AVX2` for the
 * vectors of Highway, the CAS of 16 bytes, no Boost and AVX2. The kernels
 * on arrays [see Simd.h] are chosen by Highway when the program runs, and
 * change nothing here. The defaults of the first three are those of the
 * target, and are defined here, so that every header sees the same.
 *
 * The names stay those of `ADS`, since the members of an inline namespace
 * are found as members of the namespace around it, while the names that
 * the compiler gives the linker change with the configuration. Two
 * translation units of a program built with different flags, e.g., a hot
 * file with `-march=x86-64-v3` and the rest with the default, then get two
 * distinct classes in place of two definitions of one class, which the
 * linker would merge without a word (a violation of the one-definition
 * rule, whose outcome is the layout of a unit applied to the objects of the
 * other, or the instructions of AVX2 run on a processor without them); a
 * function of one unit that takes such a class from another fails to link
 * instead of running on the wrong layout. The other macros of the
 * configuration (the contracts, the backends) must still be the same in all
 * the units of a program.
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

#ifndef __ADS_Abi
 #define __ADS_Abi
/* self-identification: #endif constant */

// ADS_SIMD, 1 when the vector branches are compiled, and with them the
// dispatch of the kernels, before hwy/highway.h, which it includes
#include "ADS/Simd.h"

/// the matching of the 15 control bytes of a group of the hash tables: 1
/// by the vectors of Highway [see SimdInline.h], 0 by broadword
/// operations; by default ADS_SIMD [see Simd.h], i.e., 1 with
/// ADS_WITH_HIGHWAY and 0 without it or with ADS_SIMD_SCALAR
#ifndef ADS_FLAT_SIMD
 #define ADS_FLAT_SIMD ADS_SIMD
#endif
#if ADS_FLAT_SIMD && ! ADS_SIMD
 #error "ADS_FLAT_SIMD = 1 needs ADS_WITH_HIGHWAY and no ADS_SIMD_SCALAR"
#endif

// the compare-and-swap of 16 bytes of Boost.Atomic, with ADS_WITH_BOOST,
// where Boost finds it free of locks (e.g., cmpxchg16b on x86-64 with
// -mcx16, the pairs of AArch64), unless turned off
#ifndef ADS_FLAT_CAS16
 #if defined( ADS_WITH_BOOST )
  #include <boost/atomic/capabilities.hpp>
 #endif
 #if defined( BOOST_ATOMIC_INT128_LOCK_FREE ) &&                              \
   ( BOOST_ATOMIC_INT128_LOCK_FREE == 2 )
  #define ADS_FLAT_CAS16 1
 #else
  #define ADS_FLAT_CAS16 0
 #endif
#endif

// the pairs of boost::unordered_flat_map in FlatHashTable, with
// ADS_WITH_BOOST
#if defined( ADS_WITH_BOOST ) && ! defined( ADS_FLATHASH_BOOST )
 #define ADS_FLATHASH_BOOST 1
#endif
#ifndef ADS_FLATHASH_BOOST
 #define ADS_FLATHASH_BOOST 0
#endif

// the set of vector instructions of the kernels inlined by Highway, its
// namespace (e.g., N_SSE2, N_AVX2), which the flags of the compiler choose
// [see SimdInline.h], none without the vector branches
#if ADS_SIMD
 #define ADS_ABI_VECTOR HWY_NAMESPACE
#else
 #define ADS_ABI_VECTOR none
#endif

#define ADS_ABI_PASTE( S, C, B, V ) abi_s##S##c##C##b##B##_##V
#define ADS_ABI_SPELL( S, C, B, V ) ADS_ABI_PASTE( S, C, B, V )

/// the inline namespace of the configuration, e.g., `abi_s1c1b0_N_AVX2`
#define ADS_ABI_NAME                                                          \
 ADS_ABI_SPELL( ADS_FLAT_SIMD, ADS_FLAT_CAS16, ADS_FLATHASH_BOOST,            \
		ADS_ABI_VECTOR )

/// opens the inline namespace of the configuration
#define ADS_ABI_BEGIN                                                         \
 inline namespace ADS_ABI_NAME                                                \
 {

/// closes the inline namespace of the configuration
#define ADS_ABI_END }

/*--------------------------------------------------------------------------*/

#endif /* Abi.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Abi.h -------------------------------*/
/*--------------------------------------------------------------------------*/
