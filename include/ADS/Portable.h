/*--------------------------------------------------------------------------*/
/*---------------------------- File Portable.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The primitives of the processor that the compilers spell each in their
 * own way, in one form for all: the product of two words of 64 bits in
 * two, the swap of the bytes of a word and the signed integers of 128 bits.
 * Each is the builtin of GCC and Clang where they have it, the intrinsic of
 * MSVC on x64 and ARM64 where it has one, and the arithmetic on halves of
 * 32 bits elsewhere; the signed integer of 128 bits is __int128 where the
 * compiler has it, else a pair of words in two's complement with the same
 * results wherever those of __int128 are defined. They are details of the
 * implementation, out of the documentation.
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

#if defined( _MSC_VER ) && !defined( __clang__ )
 #include <cstdlib>
 #include <intrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// \cond
namespace detail {

/*--------------------------------------------------------------------------*/

/// the product of 128 bits of a and b, its high half in hi
constexpr std::uint64_t wideProduct( std::uint64_t a , std::uint64_t b ,
                                     std::uint64_t & hi )
{
#if defined( __SIZEOF_INT128__ )
 __extension__ typedef unsigned __int128 U;
 const U p = U( a ) * b;
 hi = std::uint64_t( p >> 64 );
 return( std::uint64_t( p ) );
#else
 if( ! std::is_constant_evaluated() ) {
 #if defined( _MSC_VER ) && defined( _M_X64 ) && !defined( __clang__ )
  return( _umul128( a , b , &hi ) );
 #elif defined( _MSC_VER ) && defined( _M_ARM64 ) && !defined( __clang__ )
  hi = __umulh( a , b );
  return( a * b );
 #endif
  }
 constexpr std::uint64_t LOW = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & LOW , a1 = a >> 32 , b0 = b & LOW ,
                     b1 = b >> 32;
 const std::uint64_t p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
                     p11 = a1 * b1;
 const std::uint64_t mid = ( p00 >> 32 ) + ( p01 & LOW ) + ( p10 & LOW );
 hi = p11 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 );
 return( ( mid << 32 ) | ( p00 & LOW ) );
#endif
 }

/*--------------------------------------------------------------------------*/

/// the 8 bytes of x in the reverse order
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

#if defined( __SIZEOF_INT128__ )

__extension__ typedef __int128 Int128;

#else

/// a signed integer of 128 bits in two's complement, with the sums, the
/// differences, the products and the order of __int128 (those modulo
/// 2^128, exact when the result fits)
struct Int128 {
 std::uint64_t lo = 0;                ///< the low word
 std::uint64_t hi = 0;                ///< the high word, with the sign

 constexpr Int128( void ) = default;

 template< std::integral I >
 constexpr Int128( I v )
  : lo( std::uint64_t( v ) ) ,
    hi( ( std::is_signed_v< I > && ( v < 0 ) ) ? ~std::uint64_t( 0 ) : 0 )
 { }

 explicit operator long double( void ) const {
  return( (long double)( std::int64_t( hi ) ) * 18446744073709551616.0L +
          (long double)( lo ) );
  }

 friend constexpr Int128 operator+( Int128 a , Int128 b ) {
  Int128 r;
  r.lo = a.lo + b.lo;
  r.hi = a.hi + b.hi + ( r.lo < a.lo ? 1 : 0 );
  return( r );
  }

 friend constexpr Int128 operator-( Int128 a , Int128 b ) {
  Int128 r;
  r.lo = a.lo - b.lo;
  r.hi = a.hi - b.hi - ( a.lo < b.lo ? 1 : 0 );
  return( r );
  }

 friend constexpr Int128 operator*( Int128 a , Int128 b ) {
  Int128 r;
  r.lo = wideProduct( a.lo , b.lo , r.hi );
  r.hi += a.lo * b.hi + a.hi * b.lo;
  return( r );
  }

 friend constexpr bool operator==( Int128 a , Int128 b ) {
  return( ( a.lo == b.lo ) && ( a.hi == b.hi ) );
  }

 friend constexpr std::strong_ordering operator<=>( Int128 a , Int128 b ) {
  if( a.hi != b.hi )
   return( std::int64_t( a.hi ) <=> std::int64_t( b.hi ) );
  return( a.lo <=> b.lo );
  }
 };

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Portable.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Portable.h ---------------------------*/
/*--------------------------------------------------------------------------*/
