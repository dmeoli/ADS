/*--------------------------------------------------------------------------*/
/*---------------------------- File Rational.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact rational numbers of Knuth (4.5.1), beyond the book: a fraction
 * u / u' of integers of any length [see BigInteger.h] kept in lowest
 * terms, with u' > 0. The sum and the product reduce by the greatest
 * common divisors of the smaller numbers, as Henrici suggested, instead of
 * by that of the result: for u / u' + v / v', with
 * \f$ d_1 = \gcd( u' , v' ) \f$, the numerator is
 * \f$ t = u ( v' / d_1 ) + v ( u' / d_1 ) \f$ and, with
 * \f$ d_2 = \gcd( t , d_1 ) \f$, the result is
 * \f$ ( t / d_2 ) / ( ( u' / d_1 ) ( v' / d_2 ) ) \f$, already in lowest
 * terms; for the product, \f$ d_1 = \gcd( u , v' ) \f$ and
 * \f$ d_2 = \gcd( u' , v ) \f$ cancel crosswise. Since \f$ d_1 = 1 \f$
 * with probability about \f$ 6 / \pi^2 \f$, the second gcd is most often
 * skipped; a division by a gcd of 1 is skipped too, each quotient is
 * computed once, and the gcds are those of Lehmer [see BigInteger.h], one
 * short division when a denominator has one digit.
 *
 * The contracts [see Contract.h]: the denominator and a divisor are not 0;
 * the invariant is u' > 0 and \f$ \gcd( u , u' ) = 1 \f$, an audit.
 * The operations are defined in Rational.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, section 4.5.1
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

#ifndef __ADS_Rational
 #define __ADS_Rational
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/BigInteger.h"

#include <compare>
#include <string>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Rational ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a fraction in lowest terms, the denominator positive
class Rational {

/*--------------------------------------------------------------------------*/
 public:

 /// \pre den != 0
 Rational( BigInteger num = 0 , BigInteger den = 1 );

 [[nodiscard]] const BigInteger & num( void ) const { return( f_num ); }
 [[nodiscard]] const BigInteger & den( void ) const { return( f_den ); }

 /// "u/u'", or "u" when u' = 1
 [[nodiscard]] std::string toString( void ) const;

 friend Rational operator+( const Rational & a , const Rational & b );
 friend Rational operator-( const Rational & a , const Rational & b );
 friend Rational operator*( const Rational & a , const Rational & b );

 /// \pre b != 0
 friend Rational operator/( const Rational & a , const Rational & b );

 Rational operator-( void ) const;

 friend bool operator==( const Rational & a , const Rational & b ) = default;
 friend std::strong_ordering operator<=>( const Rational & a ,
                                          const Rational & b );

/*--------------------------------------------------------------------------*/
 private:

 struct Reduced {};         ///< the fraction is already in lowest terms

 Rational( BigInteger num , BigInteger den , Reduced );

 [[nodiscard]] bool lowest( void ) const;

 BigInteger f_num , f_den;

 };  // end( class Rational )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Rational.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Rational.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Rational.h ---------------------------*/
/*--------------------------------------------------------------------------*/
