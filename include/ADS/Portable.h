/*--------------------------------------------------------------------------*/
/*---------------------------- File Portable.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The primitives of the processor that the compilers spell each in their
 * own way, in one form for all: the product of two words of 64 bits by the
 * products of their halves of 32 bits (`wideProductHalves()`, the portable
 * form of the product of `Wide.h`), and the swap of the bytes of a word,
 * which is the `std::byteswap()` of C++23 where the library has it, else
 * the `boost::core::byteswap()` of Boost.Core where Boost is found, else
 * three exchanges by masks, of the bytes, of the pairs of bytes and of the
 * halves. The rest of the arithmetic of two words (the product by the
 * instruction of the processor, the division of two words by one and the
 * signed integers of 128 bits) is in `Wide.h`. They are details of the
 * implementation, out of the API reference, and their arithmetic is on
 * the theory page of the area.
 *
 * Two attributes have the same fate: `ADS_NO_UNIQUE_ADDRESS`, the
 * `[[no_unique_address]]` of C++20 under the name that MSVC honors
 * (`[[msvc::no_unique_address]]`, since MSVC accepts the standard spelling
 * and ignores it), so that an empty member takes no storage with every
 * compiler; and `ADS_COLD_PATH`, a function that is never inlined and is
 * laid out away from the hot code (`[[gnu::cold, gnu::noinline]]`, the
 * `noinline` of MSVC), which keeps the rare path of an operation, e.g.,
 * the notification of its observers, out of the code of the common one.
 * Under the sanitizer of addresses, `ADS_POISON( p , n )` and
 * `ADS_UNPOISON( p , n )` mark the \f$ n \f$ bytes from \f$ p \f$ as
 * forbidden and allowed again, so that a pool which drops its objects at
 * once (the `clear()` of `Arena` and `ObjectPool`) makes the sanitizer stop
 * a stale pointer at its first access; elsewhere they are no code, which
 * the interface of the sanitizer decides by itself (its macros are empty
 * when it is off), so that ADS needs not know whether it is on.
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

#ifndef __ADS_Portable
 #define __ADS_Portable
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <type_traits>

// the swap of bytes of Boost.Core, which knows the one of each compiler
// (boost::core::byteswap, taken from Boost 1.83 on)
#if __has_include( <boost/version.hpp> )
 #include <boost/version.hpp>
#endif
#if defined( BOOST_VERSION ) && ( BOOST_VERSION >= 108300 )
 #include <boost/core/bit.hpp>
 #define ADS_BYTESWAP_BOOST 1
#else
 #define ADS_BYTESWAP_BOOST 0
#endif

// The attributes below are chosen by the feature test of the standard
// (__has_cpp_attribute), not by the name of the compiler: an attribute of a
// namespace the compiler does not know is ignored, but GCC and Clang warn
// about it, so that writing both spellings unconditionally would warn in
// every translation unit that includes ADS.

/// an empty member that takes no storage, by the attribute that does so
/// (the standard one is accepted but has no effect under the ABI of MSVC)
#if __has_cpp_attribute( msvc::no_unique_address )
 #define ADS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
 #define ADS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

/// a function inlined even where the compiler would not
#if __has_cpp_attribute( gnu::always_inline )
 #define ADS_ALWAYS_INLINE [[gnu::always_inline]] inline
#elif __has_cpp_attribute( msvc::forceinline )
 #define ADS_ALWAYS_INLINE [[msvc::forceinline]] inline
#else
 #define ADS_ALWAYS_INLINE inline
#endif

/// a function never inlined
#if __has_cpp_attribute( gnu::noinline )
 #define ADS_NOINLINE [[gnu::noinline]]
#elif __has_cpp_attribute( msvc::noinline )
 #define ADS_NOINLINE [[msvc::noinline]]
#else
 #define ADS_NOINLINE
#endif

/// a function of a rare path: never inlined, laid out away from the hot
/// code
#if __has_cpp_attribute( gnu::cold )
 #define ADS_COLD_PATH [[gnu::cold]] ADS_NOINLINE
#else
 #define ADS_COLD_PATH ADS_NOINLINE
#endif

/// the \f$ n \f$ bytes from \f$ p \f$ forbidden, and allowed again, under
/// the sanitizer of addresses, by the macros of its interface, which the
/// interface itself turns into no code when the sanitizer is off; no code
/// where the toolchain has no such interface
#if __has_include( <sanitizer/asan_interface.h> )
 #include <sanitizer/asan_interface.h>
 #define ADS_POISON( p, n ) ASAN_POISON_MEMORY_REGION( ( p ), ( n ) )
 #define ADS_UNPOISON( p, n ) ASAN_UNPOISON_MEMORY_REGION( ( p ), ( n ) )
#else
 #define ADS_POISON( p, n ) ( ( void )( p ), ( void )( n ) )
 #define ADS_UNPOISON( p, n ) ( ( void )( p ), ( void )( n ) )
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// \cond
namespace detail
{

/*--------------------------------------------------------------------------*/

/// a number of two words, \f$ \mathit{hi} \, 2^{64} + \mathit{lo} \f$
struct WordPair
{
 std::uint64_t lo ,  ///< the low word
   hi;              ///< the high word
 };

/// the product of 128 bits of `a` and `b` by the four products of their
/// halves of 32 bits (Algorithm 4.3.1M of Knuth), for the product of two
/// words of `Wide.h` without Boost and in the constant expressions, and for
/// the tests; always inlined, its two words returned in registers
///
/// In pseudocode, \f$ x_0 \f$ and \f$ x_1 \f$ the low and the high half of
/// 32 bits of the word \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{wideProductHalves}( a , b ) \cr
/// 2 & \quad p_{i j} \gets a_i b_j \text{ for } i , j \in \{ 0 , 1 \}
///     \quad \triangleright\ \text{four products of 32 bits} \cr
/// 3 & \quad \mathit{mid} \gets \lfloor p_{0 0} / 2^{32} \rfloor +
///     ( p_{0 1} \bmod 2^{32} ) + ( p_{1 0} \bmod 2^{32} ) \cr
/// 4 & \quad \mathit{hi} \gets p_{1 1} + \lfloor p_{0 1} / 2^{32}
///     \rfloor + \lfloor p_{1 0} / 2^{32} \rfloor + \lfloor \mathit{mid}
///     / 2^{32} \rfloor \cr
/// 5 & \quad \textbf{return}\ ( ( \mathit{mid} \bmod 2^{32} ) \, 2^{32} +
///     ( p_{0 0} \bmod 2^{32} ) , \mathit{hi} )
/// \end{array}}
/// \f]
[[nodiscard]] ADS_ALWAYS_INLINE constexpr WordPair
wideProductHalves( std::uint64_t a , std::uint64_t b )
{
 constexpr std::uint64_t LOW = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & LOW , a1 = a >> 32 , b0 = b & LOW , b1 = b >> 32;
 const std::uint64_t p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
		     p11 = a1 * b1;
 const std::uint64_t mid = ( p00 >> 32 ) + ( p01 & LOW ) + ( p10 & LOW );
 return( WordPair{ ( mid << 32 ) | ( p00 & LOW ) ,
		    p11 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 ) } );
 }

/*--------------------------------------------------------------------------*/

/// the 8 bytes of `x` in the reverse order, by std::byteswap where the
/// library has it (C++23), by boost::core::byteswap where Boost is found
/// (both the one swap of the processor), and otherwise by three rounds of
/// masks, which GCC does not always turn into that swap once inlined
constexpr std::uint64_t byteSwap( std::uint64_t x )
{
#if defined( __cpp_lib_byteswap )
 return( std::byteswap( x ) );
#elif ADS_BYTESWAP_BOOST
 return( boost::core::byteswap( x ) );
#else
 x = ( ( x >> 8 ) & 0x00FF00FF00FF00FFULL ) |
     ( ( x & 0x00FF00FF00FF00FFULL ) << 8 );
 x = ( ( x >> 16 ) & 0x0000FFFF0000FFFFULL ) |
     ( ( x & 0x0000FFFF0000FFFFULL ) << 16 );
 return( ( x >> 32 ) | ( x << 32 ) );
#endif
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Portable.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Portable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
