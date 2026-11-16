/*--------------------------------------------------------------------------*/
/*---------------------------- File Portable.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The primitives of the processor that the compilers spell each in their
 * own way, in one form for all: the product of two words of 64 bits by the
 * products of their halves of 32 bits (`wideProductHalves()`, the portable
 * form of the product of `Wide.h`), and the swap of the bytes of a word,
 * which is the builtin of GCC and Clang, else the `std::byteswap()` of
 * C++23, else the intrinsic of MSVC, else three exchanges by masks, of the
 * bytes, of the pairs of bytes and of the halves. The rest of the
 * arithmetic of two words (the product by the instruction of the
 * processor, the division of two words by one and the signed integers of
 * 128 bits) is in `Wide.h`. They are details of the implementation, out of
 * the API reference, and their arithmetic is on the theory page of the
 * area.
 *
 * Two attributes have the same fate: `ADS_NO_UNIQUE_ADDRESS`, the
 * `[[no_unique_address]]` of C++20 under the name that MSVC honours
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
 * a stale pointer at its first access; elsewhere they are no code.
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

#if defined( _MSC_VER ) && ! defined( __clang__ )
 #include <cstdlib>
 #include <intrin.h>
#endif

/// an empty member that takes no storage, with every compiler
#if defined( _MSC_VER ) && ! defined( __clang__ )
 #define ADS_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
 #define ADS_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

/// a function of a rare path: never inlined, laid out away from the hot
/// code
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_COLD_PATH [[gnu::cold, gnu::noinline]]
#elif defined( _MSC_VER )
 #define ADS_COLD_PATH __declspec( noinline )
#else
 #define ADS_COLD_PATH
#endif

/// the sanitizer of addresses, by the macro of GCC and MSVC or the feature
/// of Clang
#if defined( __SANITIZE_ADDRESS__ )
 #define ADS_ASAN 1
#elif defined( __has_feature )
 #if __has_feature( address_sanitizer )
  #define ADS_ASAN 1
 #endif
#endif
#ifndef ADS_ASAN
 #define ADS_ASAN 0
#endif

/// the \f$ n \f$ bytes from \f$ p \f$ forbidden, and allowed again, under
/// the sanitizer of addresses; no code elsewhere
#if ADS_ASAN
 #include <sanitizer/asan_interface.h>
 #define ADS_POISON( p, n ) ASAN_POISON_MEMORY_REGION( ( p ), ( n ) )
 #define ADS_UNPOISON( p, n ) ASAN_UNPOISON_MEMORY_REGION( ( p ), ( n ) )
#else
 #define ADS_POISON( p, n ) ( ( void )0 )
 #define ADS_UNPOISON( p, n ) ( ( void )0 )
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

/// the product of 128 bits of `a` and `b`, its high half in `hi`, by the
/// four products of their halves of 32 bits (Algorithm 4.3.1M of Knuth),
/// for the product of two words of `Wide.h` without Boost and in the
/// constant expressions, and for the tests
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
/// 5 & \quad \textbf{return}\ ( \mathit{mid} \bmod 2^{32} ) \, 2^{32} +
///     ( p_{0 0} \bmod 2^{32} )
/// \end{array}}
/// \f]
constexpr std::uint64_t wideProductHalves( std::uint64_t a , std::uint64_t b ,
					   std::uint64_t & hi )
{
 constexpr std::uint64_t LOW = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & LOW , a1 = a >> 32 , b0 = b & LOW , b1 = b >> 32;
 const std::uint64_t p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
		     p11 = a1 * b1;
 const std::uint64_t mid = ( p00 >> 32 ) + ( p01 & LOW ) + ( p10 & LOW );
 hi = p11 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 );
 return( ( mid << 32 ) | ( p00 & LOW ) );
 }

/*--------------------------------------------------------------------------*/

/// the 8 bytes of `x` in the reverse order
constexpr std::uint64_t byteSwap( std::uint64_t x )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 return( __builtin_bswap64( x ) );
#elif defined( __cpp_lib_byteswap )
 return( std::byteswap( x ) );
#else
 #if defined( _MSC_VER )
 if( ! std::is_constant_evaluated() )
  return( _byteswap_uint64( x ) );
 #endif
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
