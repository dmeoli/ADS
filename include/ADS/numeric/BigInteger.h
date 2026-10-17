/*--------------------------------------------------------------------------*/
/*--------------------------- File BigInteger.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The integers of any length of Knuth (4.3.1), beyond the book: a sign and
 * the magnitude as a vector of digits in base \f$ b = 2^{32} \f$, the
 * least significant first, with no leading zero.
 *
 * - The sum and the difference are Algorithms 4.3.1A and 4.3.1S, a carry
 *   or a borrow per digit, \f$ O( n ) \f$.
 * - The product is Algorithm 4.3.1M, a row per digit, \f$ O( m n ) \f$;
 *   above a few hundred digits it becomes a convolution of digits of 16
 *   bits by the FFT [see FFT.h], \f$ O( n \log n ) \f$ (4.3.3), exact as
 *   long as the coefficients of the convolution, below
 *   \f$ n \, 2^{32} \f$, stay well within the 53 bits of a double;
 *   multiplySchool() and multiplyFFT() are kept apart for the comparison.
 * - The quotient and the remainder are Algorithm 4.3.1D: the divisor is
 *   shifted until its leading digit has the top bit set, which makes the
 *   trial quotient \f$ \hat q \f$, taken from the two leading digits of
 *   the remainder, exceed the true one by at most 2 (Theorem 4.3.1B), and
 *   the test on the third digit makes the rare correction after the
 *   multiplication and subtraction ("add back") happen with probability
 *   about 2 / b. The quotient is truncated toward zero, as for the
 *   integers of C++.
 * - The conversion to and from the bases 2 to 36 (4.4) divides by, or
 *   multiplies by, the largest power of the base in one digit, so that the
 *   long operation runs once per many digits of the output.
 * - pow() is the binary method of 4.6.3, from right to left, and gcd() the
 *   algorithm of Euclid (4.5.2).
 *
 * The contracts [see Contract.h]: the divisor is not 0, the base is in
 * [ 2 , 36 ] and the string a number in it; the invariant is the normal
 * form (no leading zero, 0 not negative); divMod() ensures a = q b + r
 * with |r| < |b| and r of the sign of a, multiplyFFT() that it equals
 * multiplySchool(), both audits. The operations are defined in
 * BigInteger.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.1, 4.3.3, 4.4 and 4.6.3
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012,
 *      section 9.2
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_BigInteger
 #define __ADS_BigInteger
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/FFT.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BigInteger -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer of any length, sign and magnitude in base 2^32
class BigInteger {

/*--------------------------------------------------------------------------*/
 public:

 using Limb = std::uint32_t;

 BigInteger( long long v = 0 );

 /// the number written in s in the base, with an optional sign
 /// \pre 2 <= base <= 36 and s a number in the base
 explicit BigInteger( const std::string & s , int base = 10 );

 /// \pre 2 <= base <= 36
 [[nodiscard]] std::string toString( int base = 10 ) const;

 /// -1, 0 or 1
 [[nodiscard]] int sign( void ) const;

 [[nodiscard]] BigInteger abs( void ) const;

 /// the number of digits in base 2^32
 [[nodiscard]] std::size_t limbs( void ) const;

 BigInteger operator-( void ) const;

 friend BigInteger operator+( const BigInteger & a , const BigInteger & b );
 friend BigInteger operator-( const BigInteger & a , const BigInteger & b );
 friend BigInteger operator*( const BigInteger & a , const BigInteger & b );

 /// \pre b != 0
 friend BigInteger operator/( const BigInteger & a , const BigInteger & b );
 friend BigInteger operator%( const BigInteger & a , const BigInteger & b );

 BigInteger & operator+=( const BigInteger & b );
 BigInteger & operator-=( const BigInteger & b );
 BigInteger & operator*=( const BigInteger & b );

 friend bool operator==( const BigInteger & a , const BigInteger & b ) =
                                                                     default;
 friend std::strong_ordering operator<=>( const BigInteger & a ,
                                          const BigInteger & b );

 /// the quotient truncated toward zero and the remainder
 /// \pre b != 0
 /// \post a = q b + r, |r| < |b|, r = 0 or of the sign of a (an audit)
 static std::pair< BigInteger , BigInteger > divMod( const BigInteger & a ,
                                                     const BigInteger & b );

 /// the product by Algorithm M, and by the FFT
 /// \post multiplyFFT( a , b ) == multiplySchool( a , b ) (an audit)
 static BigInteger multiplySchool( const BigInteger & a ,
                                   const BigInteger & b );
 static BigInteger multiplyFFT( const BigInteger & a , const BigInteger & b );

 /// x^n by the binary method
 static BigInteger pow( BigInteger x , unsigned long n );

 /// the greatest common divisor of |a| and |b|, by Euclid
 static BigInteger gcd( BigInteger a , BigInteger b );

/*--------------------------------------------------------------------------*/
 private:

 using Mag = std::vector< Limb >;

 static constexpr std::size_t kFFT = 256;  ///< limbs for the FFT product

 BigInteger( bool neg , Mag d );

 static void trim( Mag & d );
 static int compareMag( const Mag & a , const Mag & b );
 static Mag addMag( const Mag & a , const Mag & b );
 static Mag subMag( const Mag & a , const Mag & b );   ///< a >= b
 static Mag mulSchool( const Mag & a , const Mag & b );
 static Mag mulFFT( const Mag & a , const Mag & b );

 /// a /= d, the remainder
 static Limb divSmall( Mag & a , Limb d );

 /// a = a m + c
 static void mulAddSmall( Mag & a , Limb m , Limb c );

 /// the quotient and the remainder of the magnitudes, Algorithm D
 static std::pair< Mag , Mag > divModMag( const Mag & u , const Mag & v );

 [[nodiscard]] bool normal( void ) const;

 bool f_neg = false;
 Mag f_d;                   ///< the digits, the least significant first

 };  // end( class BigInteger )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/BigInteger.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BigInteger.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BigInteger.h --------------------------*/
/*--------------------------------------------------------------------------*/
