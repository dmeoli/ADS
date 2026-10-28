/*--------------------------------------------------------------------------*/
/*---------------------------- File Rational.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exact rational numbers of Knuth (section 4.5.1 of The Art of Computer
 * Programming), beyond the book: a fraction \f$ u / u' \f$ of integers of
 * any length [see `BigInteger.h`] kept in lowest terms, with \f$ u' > 0 \f$.
 * The sum and the product reduce by the greatest common divisors of the
 * smaller numbers, as Henrici suggested, instead of by that of the result:
 * for \f$ u / u' + v / v' \f$, with \f$ d_1 = \gcd( u' , v' ) \f$, the
 * numerator is \f$ t = u ( v' / d_1 ) + v ( u' / d_1 ) \f$ and, with
 * \f$ d_2 = \gcd( t , d_1 ) \f$, the result is
 * \f$ ( t / d_2 ) / ( ( u' / d_1 ) ( v' / d_2 ) ) \f$, already in lowest
 * terms; for the product, \f$ d_1 = \gcd( u , v' ) \f$ and
 * \f$ d_2 = \gcd( u' , v ) \f$ cancel crosswise. Since \f$ d_1 = 1 \f$ with
 * probability about \f$ 6 / \pi^2 \f$, the second gcd is most often
 * skipped; a division by a gcd of 1 is skipped too, each quotient is
 * computed once, by the exact division of Hensel when the gcd has one digit
 * [see `BigInteger::divExact()`], the sum of the two products is made in
 * place, and the gcds are those of Lehmer [see `BigInteger.h`], one folded
 * remainder when a denominator has one digit.
 *
 * With `ADS_WITH_GMP` [see `BigInteger.h`], the sums, the differences, the
 * products, the quotients and the comparisons of two fractions whose longest
 * term has \f$ n \f$ digits go to `mpq_add()`, `mpq_sub()`, `mpq_mul()`,
 * `mpq_div()` and `mpq_cmp()` of GMP, which follow the same method of Henrici,
 * when `kGmp` \f$ \le n < \f$ `kGmpTop`, the range where GMP was the faster;
 * on the longer terms the gcds of `BigInteger` are faster than those of GMP,
 * and on the terms of one digit the code of `Rational` needs no allocation,
 * and `Rational` keeps its own code there. GMP reads the terms as they are, in
 * place, the result made in a fraction of GMP kept by the thread (whose digits
 * are allocated once, for terms that short) and copied into the terms of the
 * result; the reductions of the constructor go to GMP through the gcd and the
 * exact quotient of `BigInteger`. Elsewhere, and without the macro, `Rational`
 * keeps its own code, as with `ADS_RATIONAL_GMP` defined to 0
 * [see \ref theory_numeric_backends].
 *
 * The contracts [see `Contract.h`]: the denominator and a divisor are not
 * 0; the invariant is \f$ u' > 0 \f$ and \f$ \gcd( u , u' ) = 1 \f$, an
 * audit. The operations are defined in `Rational.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, section 4.5.1
 * \see T. Granlund and the GMP development team, GNU MP: The GNU Multiple
 *      Precision Arithmetic Library, version 6.3.0, 2023,
 *      https://gmplib.org/gmp-man-6.3.0.pdf, the layer mpq of the backend
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

#ifndef __ADS_Rational
 #define __ADS_Rational
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/BigInteger.h"

#include <compare>
#include <cstddef>
#include <string>
#include <utility>

// the backend of GMP, with ADS_WITH_GMP: the operations on the long terms
// go to the mpq layer of GMP, wherever BigInteger is on GMP
#ifndef ADS_RATIONAL_GMP
 #define ADS_RATIONAL_GMP ADS_BIGINT_GMP
#endif

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

 // with ADS_WITH_GMP, the digits of the longest term of the two fractions
 // with which an operation goes to GMP, [ kGmp , kGmpTop ), from the
 // timings of bench/bignum.cpp on a Xeon Gold 5120
 static constexpr std::size_t kGmp = 2;            ///< limbs, mpq
 static constexpr std::size_t kGmpTop = 4;         ///< the same, top

 /// the fraction \f$ \mathit{num} / \mathit{den} \f$, brought to lowest
 /// terms with a positive denominator by one gcd
 /// \pre \f$ \mathit{den} \ne 0 \f$
 Rational( BigInteger num = 0 , BigInteger den = 1 );

 /// the numerator \f$ u \f$
 [[nodiscard]] const BigInteger & num( void ) const { return( f_num ); }
 /// the denominator \f$ u' > 0 \f$
 [[nodiscard]] const BigInteger & den( void ) const { return( f_den ); }

 /// the text `u/u'`, or `u` when \f$ u' = 1 \f$
 [[nodiscard]] std::string toString( void ) const;

 /// the sum \f$ a + b \f$, by the gcds of the smaller numbers
 ///
 /// In pseudocode (the method of Henrici, section 4.5.1 of Knuth, beyond the
 /// book), \f$ a = u / u' \f$ and \f$ b = v / v' \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ a + b \cr
 /// 2 & \quad d_1 \gets \mathrm{gcd}( u' , v' ) \cr
 /// 3 & \quad \textbf{if}\ d_1 = 1\ \textbf{then return}\ ( u v' + v u' ) /
 ///     ( u' v' ) \cr
 /// 4 & \quad t \gets u ( v' / d_1 ) + v ( u' / d_1 ) \cr
 /// 5 & \quad \textbf{if}\ t = 0\ \textbf{then return}\ 0 \cr
 /// 6 & \quad d_2 \gets \mathrm{gcd}( t , d_1 ) \cr
 /// 7 & \quad \textbf{return}\ ( t / d_2 ) / ( ( u' / d_1 ) ( v' / d_2 ) )
 /// \end{array}}
 /// \f]
 friend Rational operator+( const Rational & a , const Rational & b );
 /// the difference \f$ a - b \f$, as \f$ a + ( - b ) \f$
 friend Rational operator-( const Rational & a , const Rational & b );
 /// the product \f$ a b \f$, by the gcds that cancel crosswise
 ///
 /// In pseudocode (section 4.5.1 of Knuth, beyond the book),
 /// \f$ a = u / u' \f$ and \f$ b = v / v' \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ a \cdot b \cr
 /// 2 & \quad \textbf{if}\ u = 0\ \textbf{or}\ v = 0\ \textbf{then return}
 ///     \ 0 \cr
 /// 3 & \quad d_1 \gets \mathrm{gcd}( u , v' );\ d_2 \gets \mathrm{gcd}( u'
 ///     , v ) \cr
 /// 4 & \quad \textbf{return}\ ( ( u / d_1 ) ( v / d_2 ) ) / ( ( u' / d_2 )
 ///     ( v' / d_1 ) )
 /// \end{array}}
 /// \f]
 friend Rational operator*( const Rational & a , const Rational & b );

 /// the quotient \f$ a / b \f$, as \f$ a \f$ times the inverse of \f$ b \f$
 /// \pre \f$ b \ne 0 \f$
 friend Rational operator/( const Rational & a , const Rational & b );

 /// the opposite \f$ - u / u' \f$
 Rational operator-( void ) const;

 /// true if the two fractions are equal, i.e., their terms are
 friend bool operator==( const Rational & a , const Rational & b ) = default;
 /// the order of \f$ a \f$ and \f$ b \f$, by the signs and then by
 /// \f$ u v' \f$ against \f$ v u' \f$
 friend std::strong_ordering operator<=>( const Rational & a ,
                                          const Rational & b );

/*--------------------------------------------------------------------------*/
 private:

 struct Reduced {};         ///< the fraction is already in lowest terms

 /// the fraction \f$ \mathit{num} / \mathit{den} \f$, already in lowest
 /// terms
 Rational( BigInteger num , BigInteger den , Reduced );

 /// the invariant: \f$ u' > 0 \f$ and \f$ \gcd( u , u' ) = 1 \f$
 [[nodiscard]] bool lowest( void ) const;

#if ADS_RATIONAL_GMP
 /// true if the operation on \f$ a \f$ and \f$ b \f$ goes to GMP
 static bool gmp( const Rational & a , const Rational & b );

 /// \f$ q \f$ as the read-only view of \f$ a \f$, its terms in place
 static mpq_srcptr gmpView( mpq_t q , const Rational & a );

 /// \f$ \mathit{op}( a , b ) \f$, a function of the mpq layer of GMP,
 /// on the terms in place
 static Rational gmpApply( const Rational & a , const Rational & b ,
                           void ( *op )( mpq_ptr , mpq_srcptr ,
                                         mpq_srcptr ) );

 /// the comparison of \f$ a \f$ and \f$ b \f$ by `mpq_cmp()`, in place
 static int gmpCompare( const Rational & a , const Rational & b );
#endif

 BigInteger f_num ,   ///< the numerator
            f_den;    ///< the denominator

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
