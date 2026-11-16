/*--------------------------------------------------------------------------*/
/*------------------------------ File Wide.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic of two words that C++20 does not spell, in one form for
 * every compiler: the product of two words of 64 bits in two, the quotient
 * and the remainder of a number of two words by one, and the signed
 * integers of 128 bits. With `ADS_WITH_BOOST` the product is that of
 * Boost.Multiprecision (`boost::multiprecision::multiply()` into a
 * `uint128_t`), which Boost computes by the instruction of the processor
 * where there is one (one `mul` on x86-64, the same code as the integers of
 * 128 bits of GCC and Clang); without it, and in a constant expression, it
 * is the arithmetic on the halves of 32 bits (`wideProductHalves()` of
 * `Portable.h`, which is also the product of `BigInteger.h` when
 * `ADS_BIGINT_PORTABLE` is defined). The division is that of two digits
 * of 32 bits by one, twice, after the divisor is shifted to its top bit
 * (Hacker's Delight, figure 9-3); the signed integer of 128 bits is a
 * pair of words in two's complement. They are details of the
 * implementation, out of the API reference, and their arithmetic is on the
 * theory page of the area.
 *
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012,
 *      section 9-4 (the division of two digits by one)
 * \see D. E. Knuth, The Art of Computer Programming, Volume 2:
 *      Seminumerical Algorithms, 3rd ed., Addison-Wesley, 1997, section
 *      4.3.1, Algorithms M and D
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

#ifndef __ADS_Wide
 #define __ADS_Wide
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Portable.h"

#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <type_traits>

#if defined( ADS_WITH_BOOST )
 #include <boost/multiprecision/cpp_int.hpp>
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

/// the product of 128 bits of `a` and `b`, its high half in `hi`: that of
/// Boost.Multiprecision with `ADS_WITH_BOOST`, `wideProductHalves()`
/// otherwise; always inlined, since in a large function the compiler may
/// otherwise call it, at about 20 instructions more than the one product
[[gnu::always_inline]] constexpr std::uint64_t
wideProduct( std::uint64_t a , std::uint64_t b , std::uint64_t & hi )
{
#if defined( ADS_WITH_BOOST )
 if( ! std::is_constant_evaluated() ) {
  boost::multiprecision::uint128_t p;
  boost::multiprecision::multiply( p , a , b );
  hi = static_cast< std::uint64_t >( p >> 64 );
  return( static_cast< std::uint64_t >( p ) );
  }
#endif
 return( wideProductHalves( a , b , hi ) );
 }

/// the quotient of \f$ u_1 2^{64} + u_0 \f$ by \f$ v \f$, its remainder in
/// `r`, by two divisions of two digits of 32 bits by one after \f$ v \f$ is
/// shifted to its top bit (Hacker's Delight, figure 9-3)
/// \pre \f$ u_1 < v \f$, so that the quotient fits in a word
///
/// In pseudocode, \f$ s \f$ the leading zeros of \f$ v \f$, \f$ B = 2^{32}
/// \f$ and \f$ x_1 , x_0 \f$ the two digits of the word \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{wideDivide}( u_1 , u_0 , v ) \cr
/// 2 & \quad ( u_1 , u_0 , v ) \gets ( u_1 , u_0 , v ) \cdot 2^s \cr
/// 3 & \quad \textbf{for}\ ( j , d ) \in ( ( 1 , u_{0,1} ) , ( 0 ,
///     u_{0,0} ) )\ \textbf{do} \cr
/// 4 & \qquad \hat{q} \gets \lfloor u_1 / v_1 \rfloor;\ \hat{r} \gets u_1
///     - \hat{q} v_1 \cr
/// 5 & \qquad \textbf{while}\ \hat{q} \ge B\ \textbf{or}\ \hat{q} v_0 > B
///     \hat{r} + d\ \textbf{do}\ \hat{q} \gets \hat{q} - 1;\ \hat{r} \gets
///     \hat{r} + v_1\ \textbf{until}\ \hat{r} \ge B \cr
/// 6 & \qquad q_j \gets \hat{q};\ u_1 \gets B u_1 + d - \hat{q} v \cr
/// 7 & \quad \textbf{return}\ ( B q_1 + q_0 , u_1 / 2^s )
/// \end{array}}
/// \f]
constexpr std::uint64_t wideDivide( std::uint64_t u1 , std::uint64_t u0 ,
				    std::uint64_t v , std::uint64_t & r )
{
 constexpr std::uint64_t B = std::uint64_t( 1 ) << 32 , M = B - 1;
 const int s = std::countl_zero( v );
 if( s > 0 ) {
  v <<= s;
  u1 = ( u1 << s ) | ( u0 >> ( 64 - s ) );
  u0 <<= s;
  }
 const std::uint64_t v1 = v >> 32 , v0 = v & M;
 std::uint64_t q[ 2 ] = { 0 , 0 };
 for( int j = 1 ; j >= 0 ; --j ) {
  const std::uint64_t d = j ? ( u0 >> 32 ) : ( u0 & M );
  std::uint64_t qh = u1 / v1 , rh = u1 - qh * v1;
  while( ( qh >= B ) || ( qh * v0 > ( ( rh << 32 ) | d ) ) ) {
   --qh;
   rh += v1;
   if( rh >= B )
    break;
   }
  q[ j ] = qh;
  u1 = ( u1 << 32 ) + d - qh * v;  // modulo 2^64, below v
  }
 r = u1 >> s;
 return( ( q[ 1 ] << 32 ) | q[ 0 ] );
 }

/*--------------------------------------------------------------------------*/

/// a signed integer of 128 bits in two's complement, with the sums, the
/// differences, the products and the order of the integers modulo
/// \f$ 2^{128} \f$ (exact when the result fits)
struct Int128
{
 std::uint64_t lo = 0;  ///< the low word
 std::uint64_t hi = 0;  ///< the high word, with the sign

 constexpr Int128( void ) = default;

 template< std::integral I >
 constexpr Int128( I v )
     : lo( std::uint64_t( v ) ) ,
       hi( ( std::is_signed_v< I > && ( v < 0 ) ) ? ~std::uint64_t( 0 ) : 0 ) {
  }

 explicit operator long double( void ) const {
  return( ( long double )( std::int64_t( hi ) ) * 18446744073709551616.0L +
	   ( long double )( lo ) );
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

/*--------------------------------------------------------------------------*/

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Wide.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Wide.h -----------------------------*/
/*--------------------------------------------------------------------------*/
