/*--------------------------------------------------------------------------*/
/*--------------------------- File BigInteger.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The integers of any length of Knuth (4.3.1), beyond the book: a sign and
 * the magnitude as a vector of digits in base \f$ b = 2^{32} \f$, the
 * least significant first, with no leading zero. The long operations work
 * on spans of digits with the scratch space allocated once per call.
 *
 * - The sum and the difference are Algorithms 4.3.1A and 4.3.1S, a carry
 *   or a borrow per digit, \f$ O( n ) \f$.
 * - The product is Algorithm 4.3.1M, a row per digit, \f$ O( m n ) \f$,
 *   for the short factors, with half the rows for a square; from
 *   kKaratsuba digits it is the product of Karatsuba (4.3.3A) in the
 *   subtractive form, three products of halves instead of four on one
 *   scratch buffer, \f$ O( n^{\log_2 3} ) \f$, the longer factor cut in
 *   pieces as long as the shorter one when their lengths differ; from kFFT
 *   digits it is a convolution of the digits by the number theoretic
 *   transform (4.3.3, 4.6.4) modulo three primes \f$ p < 2^{31} \f$ with
 *   \f$ 2^{26} \mid p - 1 \f$, in the arithmetic of Montgomery, and the
 *   Chinese remainder theorem (4.3.2), \f$ O( n \log n ) \f$: exact, since
 *   a coefficient, below \f$ n \, 2^{64} \f$, stays below the product of
 *   the primes, \f$ 2^{90} \f$, up to transforms of \f$ 2^{26} \f$ digits,
 *   past which Karatsuba splits the factors. multiplySchool(),
 *   multiplyKaratsuba() and multiplyFFT() are kept apart for the
 *   comparison.
 * - The quotient and the remainder are Algorithm 4.3.1D for the short
 *   quotients: the divisor is shifted until its leading digit has the top
 *   bit set, which makes the trial quotient \f$ \hat q \f$, taken from the
 *   two leading digits of the remainder, exceed the true one by at most 2
 *   (Theorem 4.3.1B), and the test on the third digit makes the rare
 *   correction after the multiplication and subtraction ("add back")
 *   happen with probability about 2 / b. A division of two digits by one,
 *   here and in the short division, is two products by a reciprocal of
 *   the divisor computed once (Moeller and Granlund). From kDivide digits
 *   of quotient the division is recursive (Burnikel and Ziegler; Brent and
 *   Zimmermann, Algorithm 1.8): the upper half of the quotient from the
 *   upper half of the divisor, corrected at most twice, then the lower
 *   half, \f$ O( M( n ) \log n ) \f$ for products in \f$ M( n ) \f$, a
 *   quotient longer than the divisor taken by blocks as long as the
 *   divisor. divModSchool() is Algorithm D alone. The quotient is
 *   truncated toward zero, as for the integers of C++.
 * - The conversion to and from the bases 2 to 36 (4.4) works with the
 *   largest power P of the base in one digit. In the bases that are powers
 *   of 2 it is a slice of bits per figure, \f$ O( n ) \f$. In the others it
 *   is divide and conquer (Brent and Zimmermann, section 1.7), with the
 *   powers \f$ P^{2^i} \f$ computed once per call: the output splits the
 *   number by the power nearest the square root, the lower part padded
 *   with zeros, and the input joins two halves with one product per level,
 *   \f$ O( M( n ) \log n ) \f$; below kConvert digits they divide or
 *   multiply by P once per many figures, as toStringSchool() and
 *   fromStringSchool() do at every length.
 * - pow() is the binary method of 4.6.3, from left to right, squares and
 *   products by x. gcd() is the algorithm of Lehmer (4.5.2L): the leading
 *   62 bits of the two numbers run the algorithm of Euclid in single
 *   precision while the two bounds of each quotient agree and the
 *   cofactors stay below \f$ 2^{30} \f$, and then one pass of two linear
 *   combinations replaces many long divisions; gcdEuclid() is the
 *   algorithm of Euclid.
 *
 * The contracts [see Contract.h]: the divisor is not 0, the base is in
 * [ 2 , 36 ] and the string a number in it; the invariant is the normal
 * form (no leading zero, 0 not negative); divMod() ensures a = q b + r
 * with |r| < |b| and r of the sign of a, and that it equals divModSchool();
 * multiplyKaratsuba() and multiplyFFT() that they equal multiplySchool();
 * the conversions that they equal toStringSchool() and fromStringSchool(),
 * gcd() that it equals gcdEuclid(), all audits. The operations are defined
 * in BigInteger.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.1, 4.3.2, 4.3.3, 4.4, 4.5.2 and
 *      4.6.3
 * \see R. P. Brent, P. Zimmermann, Modern Computer Arithmetic, Cambridge
 *      University Press, 2010, sections 1.3, 1.4, 1.6 and 1.7
 * \see C. Burnikel, J. Ziegler, Fast recursive division, research report
 *      MPI-I-98-1-022, Max-Planck-Institut fuer Informatik, 1998
 * \see N. Moeller, T. Granlund, Improved division by invariant integers,
 *      IEEE Transactions on Computers 60(2), 165-175, 2011,
 *      doi:10.1109/TC.2010.143
 * \see P. L. Montgomery, Modular multiplication without trial division,
 *      Mathematics of Computation 44(170), 519-521, 1985,
 *      doi:10.1090/S0025-5718-1985-0777282-X
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

#include <algorithm>
#include <bit>
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

 // the thresholds, in digits, from the instruction counts of the
 // benchmark bench/bignum.cpp; provisional, as those of ADS::sort()
 static constexpr std::size_t kKaratsuba = 16;   ///< limbs for Karatsuba
 static constexpr std::size_t kFFT = 1536;       ///< limbs for the NTT
 static constexpr std::size_t kDivide = 32;      ///< quotient limbs, recursion
 static constexpr std::size_t kConvert = 32;     ///< limbs, divide and conquer

 BigInteger( long long v = 0 );

 /// the number written in s in the base, with an optional sign
 /// \pre 2 <= base <= 36 and s a number in the base
 /// \post *this == fromStringSchool( s , base ) (an audit)
 explicit BigInteger( const std::string & s , int base = 10 );

 /// \pre 2 <= base <= 36
 /// \post the result equals toStringSchool( base ) (an audit)
 [[nodiscard]] std::string toString( int base = 10 ) const;

 /// the conversions dividing or multiplying by P only, O( n^2 )
 /// \pre as for toString() and BigInteger( s , base )
 [[nodiscard]] std::string toStringSchool( int base = 10 ) const;
 static BigInteger fromStringSchool( const std::string & s , int base = 10 );

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
 /// \post a = q b + r, |r| < |b|, r = 0 or of the sign of a, and the pair
 ///       equals divModSchool( a , b ) (audits)
 static std::pair< BigInteger , BigInteger > divMod( const BigInteger & a ,
                                                     const BigInteger & b );

 /// the quotient and the remainder by Algorithm D alone, O( m n )
 /// \pre b != 0
 static std::pair< BigInteger , BigInteger > divModSchool(
                                const BigInteger & a , const BigInteger & b );

 /// the product by Algorithm M, by Karatsuba and by the NTT
 /// \post multiplyKaratsuba( a , b ) and multiplyFFT( a , b ) equal
 ///       multiplySchool( a , b ) (an audit)
 static BigInteger multiplySchool( const BigInteger & a ,
                                   const BigInteger & b );
 static BigInteger multiplyKaratsuba( const BigInteger & a ,
                                      const BigInteger & b );
 static BigInteger multiplyFFT( const BigInteger & a , const BigInteger & b );

 /// x^n by the binary method
 static BigInteger pow( const BigInteger & x , unsigned long n );

 /// the greatest common divisor of |a| and |b|, by Lehmer
 /// \post it equals gcdEuclid( a , b ) (an audit)
 static BigInteger gcd( const BigInteger & a , const BigInteger & b );

 /// the greatest common divisor of |a| and |b|, by Euclid
 static BigInteger gcdEuclid( BigInteger a , BigInteger b );

/*--------------------------------------------------------------------------*/
 private:

 using Mag = std::vector< Limb >;

 /// the longest transform of the NTT, 2^26 digits
 static constexpr std::size_t kNTTMax = std::size_t( 1 ) << 26;

 BigInteger( bool neg , Mag d );

 static void trim( Mag & d );
 static int compareMag( const Mag & a , const Mag & b );

 /// the comparison of a and b, n digits each
 static int cmpN( const Limb * a , const Limb * b , std::size_t n );

 /// r = a + b, na >= nb, r of na digits; the carry
 static Limb addN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );

 /// r = a - b, na >= nb, r of na digits; the borrow
 static Limb subN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );

 /// r += x and r -= x, nr >= nx; the carry and the borrow
 static Limb addTo( Limb * r , std::size_t nr , const Limb * x ,
                    std::size_t nx );
 static Limb subFrom( Limb * r , std::size_t nr , const Limb * x ,
                      std::size_t nx );

 /// r = a m, r += a m and r -= a m, n digits; the carry or the borrow
 static Limb mul1( Limb * r , const Limb * a , std::size_t n , Limb m );
 static Limb addMul1( Limb * r , const Limb * a , std::size_t n , Limb m );
 static Limb subMul1( Limb * r , const Limb * a , std::size_t n , Limb m );

 /// r = |a - b|, na >= nb, r of na digits; true if a < b
 static bool absDiff( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb );

 static Mag addMag( const Mag & a , const Mag & b );
 static Mag subMag( const Mag & a , const Mag & b );   ///< a >= b

 /// a + b, or a - b if bneg differs from the sign of b
 static BigInteger addSigned( const BigInteger & a , const BigInteger & b ,
                              bool bneg );

 /// r = a b, r of na + nb digits: Algorithm M, a square if a == b
 static void mulSchool( Limb * r , const Limb * a , std::size_t na ,
                        const Limb * b , std::size_t nb );
 static void sqrSchool( Limb * r , const Limb * a , std::size_t n );

 /// r = a b, n digits each, by Karatsuba on the scratch w of
 /// karatsubaScratch( n ) digits; the NTT from fft digits
 static void karatsuba( Limb * r , const Limb * a , const Limb * b ,
                        std::size_t n , Limb * w , std::size_t fft );
 static std::size_t karatsubaScratch( std::size_t n );

 /// r = a b by the NTT, na + nb <= kNTTMax
 static void mulNTT( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb );

 /// r = a b, r of na + nb digits, by the method of the lengths; the NTT
 /// from fft digits
 static void mulP( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb ,
                   std::size_t fft = kFFT );
 static Mag mulMag( const Mag & a , const Mag & b , std::size_t fft = kFFT );

 /// a /= d, the remainder; the remainder of a by d
 static Limb divSmall( Mag & a , Limb d );
 static Limb modSmall( const Limb * a , std::size_t n , Limb d );

 /// a = a m + c
 static void mulAddSmall( Mag & a , Limb m , Limb c );

 /// the quotient of a, n + m digits, by b, n >= 2 digits with the top bit
 /// set, a < 2 b 2^{32 m}: its m low digits in q and the top one, 0 or 1,
 /// returned, the remainder in a[ 0 , n ); by Algorithm D, and recursively
 /// for m <= n with the scratch t of n digits
 static Limb divBase( Limb * a , std::size_t m , const Limb * b ,
                      std::size_t n , Limb * q );
 static Limb divRec( Limb * a , std::size_t m , const Limb * b ,
                     std::size_t n , Limb * q , Limb * t );

 /// the quotient and the remainder of the magnitudes, recursive or by
 /// Algorithm D alone if school
 static std::pair< Mag , Mag > divModMag( const Mag & u , const Mag & v ,
                                          bool school = false );

 /// a u + b v, nonnegative, |a| , |b| < 2^30 of opposite signs
 static Mag linComb( const Mag & u , const Mag & v , std::int64_t a ,
                     std::int64_t b );

 /// the greatest common divisor of the magnitudes, by Lehmer
 static Mag gcdMag( Mag u , Mag v );

 /// out += the figures of a in the base, at least pad of them, dividing
 /// by P
 static void figures( Mag a , std::size_t pad , int base ,
                      std::string & out );

 /// out += the figures of x, at least pad, by halves from kConvert digits,
 /// pw[ i ] = P^{2^i} with P^1 holding k figures
 static void toFigures( const Mag & x , std::size_t pad , int base ,
                        const std::vector< Mag > & pw , int k ,
                        std::string & out );

 /// the sum of c[ j ] P^j for j < n, by halves from kConvert digits,
 /// pw[ i ] = P^{2^i}
 static Mag fromChunks( const Limb * c , std::size_t n ,
                        const std::vector< Mag > & pw , Limb P );

 /// the greatest common divisor of two numbers below 2^64, binary
 static std::uint64_t gcdWord( std::uint64_t a , std::uint64_t b );

 [[nodiscard]] bool normal( void ) const;

 bool f_neg = false;
 Mag f_d;                   ///< the digits, the least significant first

 };  // end( class BigInteger )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/BigInteger.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BigInteger.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BigInteger.h --------------------------*/
/*--------------------------------------------------------------------------*/
