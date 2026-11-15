/*--------------------------------------------------------------------------*/
/*------------------------------ File Abi.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The name of the configuration of the layouts, an inline namespace: the
 * headers whose classes change their layout or their inline code with the
 * flags of the compiler declare them in `inline namespace ADS_ABI_NAME`,
 * opened by `ADS_ABI_BEGIN` and closed by `ADS_ABI_END`, whose name spells
 * the options that change them. These are the matching of the control bytes
 * of the hash tables (`ADS_FLAT_SIMD`: 0 broadword, 1 SSE2, 2 NEON), the
 * compare-and-swap of 16 bytes of `ConcurrentFlatTable` (`ADS_FLAT_CAS16`,
 * which `-mcx16` and `-march=x86-64-v2` or later turn on), the pairs of
 * `boost::unordered_flat_map` in `FlatHashTable` (`ADS_FLATHASH_BOOST`) and
 * the level of the vector instructions of x86-64 (1 for SSE2, 3 for AVX2, 4
 * for AVX-512, 0 elsewhere), e.g., `abi_s1c1b0v3` for SSE2, the CAS of 16
 * bytes, no Boost and AVX2. The defaults of the first three are those of
 * the target, and are defined here, so that every header sees the same.
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

/// the matching of the 15 control bytes of a group of the hash tables: 1 by
/// SSE2, 2 by NEON, 0 by broadword operations, on any machine; by default
/// the first that the target has
#ifndef ADS_FLAT_SIMD
 #if defined( __SSE2__ ) || defined( _M_X64 ) ||                              \
   ( defined( _M_IX86_FP ) && ( _M_IX86_FP >= 2 ) )
  #define ADS_FLAT_SIMD 1
 #elif ( defined( __ARM_NEON ) && defined( __AARCH64EL__ ) ) ||               \
   defined( _M_ARM64 )
  #define ADS_FLAT_SIMD 2
 #else
  #define ADS_FLAT_SIMD 0
 #endif
#endif

// the compare-and-swap of 16 bytes without locks, on little-endian x86-64
// and AArch64, unless turned off; not with the GCC of macOS on AArch64 (the
// port of Homebrew), which gives its caspal a pair of registers that does
// not start at an even one, and the assembler refuses it
#ifndef ADS_FLAT_CAS16
 #if defined( __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16 ) &&                        \
   ( defined( __x86_64__ ) || defined( __aarch64__ ) ) &&                     \
   defined( __BYTE_ORDER__ ) &&                                               \
   ( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ) &&                           \
   ! ( defined( __APPLE__ ) && defined( __aarch64__ ) &&                      \
       ! defined( __clang__ ) )
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

// the level of the vector instructions of x86-64 that the inline code may
// use, 0 on the other targets
#if defined( __AVX512F__ )
 #define ADS_ABI_VECTOR 4
#elif defined( __AVX2__ )
 #define ADS_ABI_VECTOR 3
#elif defined( __SSE2__ ) || defined( _M_X64 )
 #define ADS_ABI_VECTOR 1
#else
 #define ADS_ABI_VECTOR 0
#endif

#define ADS_ABI_PASTE( S, C, B, V ) abi_s##S##c##C##b##B##v##V
#define ADS_ABI_SPELL( S, C, B, V ) ADS_ABI_PASTE( S, C, B, V )

/// the inline namespace of the configuration, e.g., `abi_s1c1b0v3`
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
