/*--------------------------------------------------------------------------*/
/*--------------------------- File BigInteger.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The integers of any length of Knuth (4.3.1): a sign and the magnitude as
 * a vector of digits (limbs) in base \f$ b = 2^{64} \f$, the least
 * significant first, with no leading zero, the first two in place in the
 * number, so that the numbers of one or two digits are made with no
 * allocation, and the longer ones in a block of the heap that grows by
 * doubling (`detail::Limbs`).
 *
 * With `ADS_WITH_GMP` defined (by the option of CMake of the same name,
 * which defines it whenever GMP is found, by the makefile, which does the
 * same, or by hand), every operation goes to GMP, at every length: the
 * digits of `BigInteger` are those of the mpn layer, read in place, and the
 * sum, the difference, the order, the product, the quotient with the
 * remainder, the square root and the conversion from a string write their
 * result in place too (`mpn_add()`, `mpn_sub()`, `mpn_cmp()`, `mpn_mul()`,
 * `mpn_sqr()`, `mpn_tdiv_qr()`, `mpn_sqrtrem()`, `mpn_set_str()`), while the
 * quotient alone, the exact quotient, the greatest common divisor, the
 * power and the conversion to a string go to the mpz layer
 * (`mpz_tdiv_q()`, `mpz_divexact()`, `mpz_gcd()`, `mpz_pow_ui()`,
 * `mpz_get_str()`) on views of the digits in place (`mpz_roinit_n()`), the
 * digits of the result copied from it in \f$ O( n ) \f$, against the
 * superlinear cost of the operation. Where the result fits the digits in
 * place, the number of GMP is spared: the quotient by a divisor of one or
 * two digits goes to `mpn_tdiv_qr()`, its remainder kept in place, and the
 * gcd with a number of one digit to `mpn_gcd_1()`. GMP chooses its own
 * algorithms and thresholds for the processor it was built for. All this
 * holds where a limb of GMP is a word of 64 bits, all of it number;
 * elsewhere, and with `ADS_BIGINT_GMP` defined to 0, `BigInteger` keeps the
 * code of the book below. GMP is free software under the LGPL v3 (or the
 * GPL v2), as ADS is under the LGPL v3; note that GMP aborts where it cannot
 * allocate, while the code of the book throws `std::bad_alloc` [see
 * \ref theory_numeric_backends].
 *
 * Without the backend, and in the methods asked by name
 * (`multiplySchool()`, `multiplyKaratsuba()`, `multiplyFFT()`,
 * `divModSchool()`, `toStringSchool()`, `fromStringSchool()`,
 * `sqrtNewton()`, `gcdEuclid()`), which the audits compare with the
 * operations, the algorithms are those of the book, written for clarity
 * and portability, with no assembly, no vector instructions and no
 * threshold measured on a processor:
 *
 * - the sum and the difference are Algorithms 4.3.1A and 4.3.1S, a carry
 *   or a borrow per digit, \f$ O( n ) \f$;
 * - the product is Algorithm 4.3.1M, \f$ O( m n ) \f$, below `kKaratsuba`
 *   digits of the shorter factor; from there the method of Karatsuba
 *   (4.3.3), with \f$ a = a_1 \beta + a_0 \f$ and
 *   \f$ b = b_1 \beta + b_0 \f$,
 *   \f[ a b = z_2 \beta^2 + \bigl( ( a_0 + a_1 ) ( b_0 + b_1 ) - z_0 - z_2
 *       \bigr) \beta + z_0 , \qquad z_0 = a_0 b_0 , \; z_2 = a_1 b_1 , \f]
 *   \f$ M( n ) = 3 M( n / 2 ) + O( n ) = O( n^{\log_2 3} ) \f$; from `kFFT`
 *   digits the convolution of the digits by the number theoretic
 *   transform (4.3.3 and 4.6.4): the digits cut in pieces of 32 bits, the
 *   cyclic convolution of the pieces modulo three primes \f$ p < 2^{31} \f$
 *   with \f$ 2^{26} \mid p - 1 \f$ by the transform of Cooley and Tukey,
 *   and each coefficient, below \f$ 2^{89} \f$ and so below the product of
 *   the primes, from its three residues by the Chinese remainder theorem in
 *   the form of Garner (4.3.2), \f$ O( n \log n ) \f$ operations on words
 *   up to \f$ 2^{25} \f$ digits in all, past which Karatsuba splits the
 *   factors;
 * - the quotient and the remainder are Algorithm 4.3.1D, \f$ O( m n ) \f$
 *   for a quotient of \f$ m \f$ digits and a divisor of \f$ n \f$: the
 *   divisor shifted until its leading digit has the top bit set, each
 *   trial quotient \f$ \hat q \f$ from the two leading digits of the
 *   remainder and corrected by the test of step D3, which leaves it at most
 *   one above the true one, the rare excess undone by the add back of step
 *   D6; by a divisor of one digit the short division of exercise 16 of
 *   section 4.3.1;
 * - the conversion to and from the bases 2 to 36 is that of section 4.4:
 *   in the bases that are powers of 2 a slice of bits per figure,
 *   \f$ O( n ) \f$, in the others by the largest power \f$ P \f$ of the base
 *   in one digit (\f$ 10^{19} \f$ for the base 10), the remainders of the
 *   short divisions by \f$ P \f$ on the way out and the products
 *   \f$ x \gets x P + c \f$ on the way in, \f$ O( n^2 ) \f$;
 * - `pow()` is the binary method of 4.6.3, from left to right;
 * - `gcd()` is the algorithm of Lehmer (4.5.2L): the leading 60 bits
 *   \f$ \hat u \f$, \f$ \hat v \f$ of the two numbers run the algorithm of
 *   Euclid on \f$ ( \hat u + A , \hat v + C ) \f$ and on
 *   \f$ ( \hat u + B , \hat v + D ) \f$ while the two quotients agree, so
 *   that they are those of the numbers, and one pass makes
 *   \f$ ( u , v ) \gets ( A u + B v , C u + D v ) \f$, about 30 bits off
 *   both; a pass that the leading bits cannot make is a division
 *   (4.5.2A), and the last digit goes by the binary algorithm on words
 *   (4.5.2B), \f$ O( n^2 ) \f$ in all;
 * - `sqrtRem()` and `sqrt()` take the root of `sqrtNewton()`, Newton's
 *   method from above, \f$ x \gets \lfloor ( x + \lfloor a / x \rfloor ) /
 *   2 \rfloor \f$ from a power of 2 at least the root, and the remainder
 *   \f$ a - s^2 \f$;
 * - `divExact()` is the quotient of Algorithm D.
 *
 * The contracts [see `Contract.h`]: the divisor is not 0, the base is in
 * \f$ [ 2 , 36 ] \f$ and the string a number in it; the invariant is the
 * normal form (no leading zero, 0 not negative); `divMod()` ensures
 * \f$ a = q b + r \f$ with \f$ |r| < |b| \f$ and \f$ r \f$ of the sign of
 * \f$ a \f$, and that it equals `divModSchool()`, `operator/()` that its
 * quotient does, `divExact()` that its result times \f$ b \f$ is \f$ a \f$;
 * `multiplyKaratsuba()` and `multiplyFFT()` that they equal
 * `multiplySchool()`; the conversions that they equal `toStringSchool()` and
 * `fromStringSchool()`, `gcd()` that it equals `gcdEuclid()`, `sqrtRem()`
 * that \f$ s^2 + r = a \f$ and that \f$ s \f$ equals `sqrtNewton()`, all
 * audits. The operations are defined in `BigInteger.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.1, 4.3.2, 4.3.3, 4.4, 4.5.2,
 *      4.6.3 and 4.6.4
 * \see A. Karatsuba, Yu. Ofman, Multiplication of multidigit numbers on
 *      automata, Soviet Physics Doklady 7, 595-596, 1963
 * \see J. M. Pollard, The fast Fourier transform in a finite field,
 *      Mathematics of Computation 25(114), 365-374, 1971,
 *      doi:10.1090/S0025-5718-1971-0301966-0
 * \see J. W. Cooley, J. W. Tukey, An algorithm for the machine calculation
 *      of complex Fourier series, Mathematics of Computation 19(90),
 *      297-301, 1965, doi:10.1090/S0025-5718-1965-0178586-1
 * \see D. H. Lehmer, Euclid's algorithm for large numbers, The American
 *      Mathematical Monthly 45(4), 227-233, 1938,
 *      doi:10.1080/00029890.1938.11990797
 * \see T. Granlund and the GMP development team, GNU MP: The GNU Multiple
 *      Precision Arithmetic Library, version 6.3.0, 2023,
 *      https://gmplib.org/gmp-man-6.3.0.pdf, the layers mpn and mpz of the
 *      backend
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
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// the width of the product of two digits done by the compiler: 128 with
// unsigned __int128, 64 with the intrinsics of MSVC on x64, 0 for none,
// the four products of the halves of 32 bits
#ifndef ADS_BIGINT_WIDE
 #if defined( ADS_BIGINT_PORTABLE )
  #define ADS_BIGINT_WIDE 0
 #elif defined( __SIZEOF_INT128__ )
  #define ADS_BIGINT_WIDE 128
 #elif defined( _MSC_VER ) && defined( _M_X64 )
  #define ADS_BIGINT_WIDE 64
 #else
  #define ADS_BIGINT_WIDE 0
 #endif
#endif
#if ADS_BIGINT_WIDE == 64
 #include <intrin.h>
#endif

// the backend of GMP, with ADS_WITH_GMP: every operation goes to the mpn
// and mpz layers of GMP, on the digits of BigInteger as they are; only
// where a limb of GMP has 64 bits, all of them number, else BigInteger
// stays on the code of the book
#if defined( ADS_WITH_GMP )
 #include <cstring>
 #include <gmp.h>
 #if ! defined( ADS_BIGINT_GMP ) && ( GMP_LIMB_BITS == 64 ) && \
     ( GMP_NAIL_BITS == 0 )
  #define ADS_BIGINT_GMP 1
 #endif
#endif
#ifndef ADS_BIGINT_GMP
 #define ADS_BIGINT_GMP 0
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Limbs --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the digits of a number, as a vector of words of 64 bits whose first two
/// are kept in place: the numbers of one or two digits, and the results
/// that fit there, are made with no allocation, the longer ones in a block
/// of the heap whose capacity at least doubles when it grows, freed when the
/// object is; the digits added by `resize()` are set to the value given, 0
/// by default
class Limbs {

 public:

 using T = std::uint64_t;   ///< a digit

 /// no digit
 Limbs( void ) noexcept {}
 /// \f$ n \f$ digits 0
 explicit Limbs( std::size_t n ) { resize( n ); }
 /// \f$ n \f$ digits \f$ v \f$
 Limbs( std::size_t n , T v ) { resize( n , v ); }
 /// the digits of \f$ [ b , e ) \f$
 Limbs( const T * b , const T * e ) {
  reserve( std::size_t( e - b ) );
  std::copy( b , e , f_p );
  f_n = std::size_t( e - b );
  }
 /// the digits of the list
 Limbs( std::initializer_list< T > l ) : Limbs( l.begin() , l.end() ) {}
 /// a copy of the digits of \f$ o \f$
 Limbs( const Limbs & o ) : Limbs( o.f_p , o.f_p + o.f_n ) {}
 /// the digits of \f$ o \f$, taken from it, which is left empty
 Limbs( Limbs && o ) noexcept { take( o ); }
 /// a copy of the digits of \f$ o \f$; unchanged if the allocation throws
 Limbs & operator=( const Limbs & o ) {
  if( this != &o ) {
   if( o.f_n > f_cap ) {                    // the new block first
    T * q = std::allocator< T >().allocate( o.f_n );
    release();
    f_p = q;
    f_cap = o.f_n;
    }
   std::copy( o.f_p , o.f_p + o.f_n , f_p );
   f_n = o.f_n;
   }
  return( *this );
  }
 /// the digits of \f$ o \f$, taken from it, which is left empty
 Limbs & operator=( Limbs && o ) noexcept {
  if( this != &o ) {
   release();
   take( o );
   }
  return( *this );
  }
 ~Limbs( void ) { release(); }

 /// the number of digits
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }
 /// true if there is no digit
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }
 /// the digits
 T * data( void ) { return( f_p ); }
 /// the digits of a constant object
 const T * data( void ) const { return( f_p ); }
 /// the first digit
 T * begin( void ) { return( f_p ); }
 /// past the last digit
 T * end( void ) { return( f_p + f_n ); }
 /// the first digit of a constant object
 const T * begin( void ) const { return( f_p ); }
 /// past the last digit of a constant object
 const T * end( void ) const { return( f_p + f_n ); }
 /// the digit \f$ i \f$
 T & operator[]( std::size_t i ) { return( f_p[ i ] ); }
 /// the digit \f$ i \f$ of a constant object
 const T & operator[]( std::size_t i ) const { return( f_p[ i ] ); }
 /// the last digit
 T & back( void ) { return( f_p[ f_n - 1 ] ); }
 /// the last digit of a constant object
 const T & back( void ) const { return( f_p[ f_n - 1 ] ); }

 /// room for \f$ n \f$ digits; unchanged if the allocation throws
 void reserve( std::size_t n ) {
  if( n > f_cap ) regrow( std::max( n , 2 * f_cap ) );
  }
 /// \f$ n \f$ digits, those added set to \f$ v \f$
 void resize( std::size_t n , T v = 0 ) {
  reserve( n );
  if( n > f_n ) std::fill( f_p + f_n , f_p + n , v );
  f_n = n;
  }
 /// \f$ n \f$ digits \f$ v \f$
 void assign( std::size_t n , T v ) {
  f_n = 0;
  resize( n , v );
  }
 /// the digit \f$ v \f$ appended
 void push_back( T v ) {
  reserve( f_n + 1 );
  f_p[ f_n++ ] = v;
  }
 /// the last digit removed
 void pop_back( void ) { --f_n; }

 /// true if the two have the same digits
 friend bool operator==( const Limbs & a , const Limbs & b ) {
  return( ( a.f_n == b.f_n ) && std::equal( a.f_p , a.f_p + a.f_n , b.f_p ) );
  }

 private:

 static constexpr std::size_t kIn = 2;      ///< the digits in place

 /// the digits moved to a block of \f$ c \f$ digits of the heap
 void regrow( std::size_t c ) {
  T * q = std::allocator< T >().allocate( c );
  std::copy( f_p , f_p + f_n , q );
  release();
  f_p = q;
  f_cap = c;
  }
 /// the block of the heap freed, if any, the digits back in place
 void release( void ) noexcept {
  if( f_p != f_in ) {
   std::allocator< T >().deallocate( f_p , f_cap );
   f_p = f_in;
   f_cap = kIn;
   }
  }
 /// the digits of \f$ o \f$ taken, its block if on the heap, and \f$ o \f$
 /// left empty, in place
 void take( Limbs & o ) noexcept {
  f_n = o.f_n;
  f_cap = o.f_cap;
  if( o.f_p != o.f_in ) f_p = o.f_p;
  else {
   std::copy( o.f_in , o.f_in + o.f_n , f_in );
   f_p = f_in;
   }
  o.f_p = o.f_in;
  o.f_n = 0;
  o.f_cap = kIn;
  }

 T * f_p = f_in;                            ///< `f_in` or on the heap
 std::size_t f_n = 0 ,                      ///< the digits
             f_cap = kIn;                   ///< the room for them
 T f_in[ kIn ] = {};                        ///< the digits in place

 };  // end( class Limbs )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BigInteger -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer of any length, sign and magnitude in base \f$ 2^{64} \f$
class BigInteger {

/*--------------------------------------------------------------------------*/
 public:

 using Limb = std::uint64_t;   ///< a digit, in base \f$ 2^{64} \f$

 // the lengths, in digits of the shorter factor, from which the code of
 // the book multiplies by Karatsuba and by the NTT: round numbers near
 // where each method overtakes the one before, not tuned to a processor,
 // the backend of GMP being the fast route
 static constexpr std::size_t kKaratsuba = 32;   ///< limbs for Karatsuba
 static constexpr std::size_t kFFT = 4096;       ///< limbs for the NTT

 /// the number \f$ v \f$
 BigInteger( long long v = 0 );

 /// the number written in \f$ s \f$ in the base, with an optional sign
 ///
 /// In pseudocode (section 4.4), \f$ P = \beta^c \f$ the largest power of
 /// the base in a digit:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{fromString}( s , \beta ) \cr
 /// 2 & \quad \textbf{if}\ \beta \text{ is a power of 2}\ \textbf{then}
 ///     \ \textbf{return}\ \text{the bits of the figures, side by side} \cr
 /// 3 & \quad x \gets 0 \cr
 /// 4 & \quad \textbf{foreach}\ \text{chunk } t \text{ of up to } c
 ///     \text{ figures, from the left}\ \textbf{do}\ x \gets x
 ///     \beta^{|t|} + t \cr
 /// 5 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$ and \f$ s \f$ a number in the
 ///      base
 /// \post `*this == fromStringSchool( s , base )` (audit)
 explicit BigInteger( const std::string & s , int base = 10 );

 /// the figures of the number in the base, with a sign if negative
 ///
 /// In pseudocode (section 4.4), \f$ P = \beta^c \f$ the largest power of
 /// the base in a digit:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toString}( x , \beta ) \cr
 /// 2 & \quad \textbf{if}\ \beta \text{ is a power of 2}\ \textbf{then}
 ///     \ \textbf{return}\ \text{the slices of bits of } x \cr
 /// 3 & \quad \textbf{while}\ x \ne 0\ \textbf{do} \cr
 /// 4 & \qquad ( x , r ) \gets \mathrm{divMod}( x , P ) \text{, a division }
 ///     \text{by one digit} \cr
 /// 5 & \qquad \text{prepend the } c \text{ figures of } r
 ///     \text{ in the base } \beta \cr
 /// 6 & \quad \textbf{return}\ \text{the figures, the zeros on the left }
 ///     \text{removed}
 /// \end{array}}
 /// \f]
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$
 /// \post the result equals `toStringSchool( base )` (audit)
 [[nodiscard]] std::string toString( int base = 10 ) const;

 /// the figures of `toString()` by the code of the book, with no backend,
 /// \f$ O( n^2 ) \f$
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$
 [[nodiscard]] std::string toStringSchool( int base = 10 ) const;
 /// the number of the constructor from a string by the code of the book,
 /// with no backend, \f$ O( n^2 ) \f$: each chunk of \f$ c \f$ figures is
 /// \f$ x \gets x P + \mathit{chunk} \f$ (section 4.4)
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$ and \f$ s \f$ a number in the
 ///      base
 static BigInteger fromStringSchool( const std::string & s , int base = 10 );

 /// the sign, \f$ -1 \f$, 0 or 1
 [[nodiscard]] int sign( void ) const;
 /// the absolute value \f$ |x| \f$
 [[nodiscard]] BigInteger abs( void ) const;

 /// the number of digits (limbs) in base \f$ 2^{64} \f$
 [[nodiscard]] std::size_t limbs( void ) const;

 /// the opposite \f$ -x \f$
 BigInteger operator-( void ) const;
 /// the sum \f$ a + b \f$, by Algorithm 4.3.1A on the magnitudes of the same
 /// sign and 4.3.1S on those of opposite signs, \f$ O( n ) \f$
 friend BigInteger operator+( const BigInteger & a , const BigInteger & b );
 /// the difference \f$ a - b \f$, as the sum with the sign of \f$ b \f$
 /// changed
 friend BigInteger operator-( const BigInteger & a , const BigInteger & b );
 /// the product \f$ a b \f$, by the method of the length of the factors
 ///
 /// In pseudocode (sections 4.3.1 and 4.3.3), \f$ n_a \ge n_b \f$ the
 /// digits of the factors:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiply}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ n_b < \mathtt{kKaratsuba}\ \textbf{then
 ///     return}\ \mathrm{multiplySchool}( a , b ) \cr
 /// 3 & \quad \textbf{if}\ n_b \ge \mathtt{kFFT}\ \textbf{then return}
 ///     \ \text{the product by the NTT} \cr
 /// 4 & \quad \textbf{return}\ \text{the product by Karatsuba, each }
 ///     \text{part multiplied again}
 /// \end{array}}
 /// \f]
 friend BigInteger operator*( const BigInteger & a , const BigInteger & b );
 /// the quotient \f$ a / b \f$ truncated toward zero
 /// \pre \f$ b \ne 0 \f$
 friend BigInteger operator/( const BigInteger & a , const BigInteger & b );
 /// the remainder of `divMod()`, of the sign of \f$ a \f$
 /// \pre \f$ b \ne 0 \f$
 friend BigInteger operator%( const BigInteger & a , const BigInteger & b );
 /// the sum \f$ x + b \f$ into \f$ x \f$
 BigInteger & operator+=( const BigInteger & b );
 /// the difference \f$ x - b \f$ into \f$ x \f$
 BigInteger & operator-=( const BigInteger & b );
 /// the product \f$ x b \f$ into \f$ x \f$
 BigInteger & operator*=( const BigInteger & b );
 /// true if the two numbers are equal
 friend bool operator==( const BigInteger & a , const BigInteger & b ) =
                                                                     default;
 /// the order of \f$ a \f$ and \f$ b \f$, by the signs, the lengths and the
 /// digits from the most significant
 friend std::strong_ordering operator<=>( const BigInteger & a ,
                                          const BigInteger & b );

 /// the quotient truncated toward zero and the remainder, by Algorithm D
 /// [see `divModSchool()`] or by the backend
 /// \pre \f$ b \ne 0 \f$
 /// \post \f$ a = q b + r \f$, \f$ |r| < |b| \f$, \f$ r = 0 \f$ or of the
 ///       sign of \f$ a \f$, and the pair equals `divModSchool( a , b )`
 ///       (audits)
 static std::pair< BigInteger , BigInteger > divMod( const BigInteger & a ,
                                                     const BigInteger & b );

 /// the quotient and the remainder by Algorithm D, \f$ O( m n ) \f$
 ///
 /// In pseudocode (Algorithm 4.3.1D), in base \f$ \beta = 2^{64} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{divModSchool}( u , v ) \cr
 /// 2 & \quad \text{shift } u \text{ and } v \text{ left until the top bit }
 ///     \text{of } v_{n-1} \text{ is 1} \cr
 /// 3 & \quad \textbf{for}\ j \gets m\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \hat q \gets \min\{ \lfloor ( u_{j+n} \beta + u_{j+n-1} ) /
 ///     v_{n-1} \rfloor , \beta - 1 \};\ \hat r \gets u_{j+n} \beta +
 ///     u_{j+n-1} - \hat q v_{n-1} \cr
 /// 5 & \qquad \textbf{while}\ \hat r < \beta\ \textbf{and}\ \hat q v_{n-2}
 ///     > \hat r \beta + u_{j+n-2}\ \textbf{do}\ \hat q \gets \hat q - 1;
 ///     \ \hat r \gets \hat r + v_{n-1} \cr
 /// 6 & \qquad u_{j .. j+n} \gets u_{j .. j+n} - \hat q v \cr
 /// 7 & \qquad \textbf{if}\ \text{it is negative}\ \textbf{then}\ \hat q
 ///     \gets \hat q - 1;\ u_{j .. j+n} \gets u_{j .. j+n} + v \cr
 /// 8 & \qquad q_j \gets \hat q \cr
 /// 9 & \quad \textbf{return}\ ( q , u \text{ shifted back right} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ b \ne 0 \f$
 static std::pair< BigInteger , BigInteger > divModSchool(
                                const BigInteger & a , const BigInteger & b );

 /// the product by Algorithm M, \f$ O( n_a n_b ) \f$
 ///
 /// In pseudocode (Algorithm 4.3.1M), in base \f$ \beta = 2^{64} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiplySchool}( a , b ) \cr
 /// 2 & \quad w \gets n_a + n_b \text{ zero digits} \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n_b - 1\ \textbf{do} \cr
 /// 4 & \qquad c \gets 0 \cr
 /// 5 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n_a - 1\ \textbf{do}
 ///     \ ( c , w_{i+j} ) \gets a_i b_j + w_{i+j} + c \cr
 /// 6 & \qquad w_{j+n_a} \gets c \cr
 /// 7 & \quad \textbf{return}\ w \text{ with the sign of } a b
 /// \end{array}}
 /// \f]
 /// \post `multiplyKaratsuba( a , b )` and `multiplyFFT( a , b )` equal
 ///       `multiplySchool( a , b )` (audit)
 static BigInteger multiplySchool( const BigInteger & a ,
                                   const BigInteger & b );
 /// the product by Karatsuba at every length down to factors of fewer than
 /// 4 digits, which Algorithm M multiplies, never by the NTT
 ///
 /// In pseudocode (section 4.3.3), \f$ n_a \ge n_b \f$,
 /// \f$ l = \lceil n_a / 2 \rceil \f$, \f$ a = a_1 \beta^l + a_0 \f$ and
 /// \f$ b = b_1 \beta^l + b_0 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{karatsuba}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ n_b < 4\ \textbf{then return}
///     \ \mathrm{multiplySchool}( a , b ) \cr
 /// 3 & \quad \textbf{if}\ n_b \le l\ \textbf{then return}
 ///     \ \mathrm{karatsuba}( a_1 , b ) \beta^l + \mathrm{karatsuba}( a_0 ,
 ///     b ) \cr
 /// 4 & \quad z_0 \gets a_0 b_0;\ z_2 \gets a_1 b_1;\ z_1 \gets ( a_0 + a_1
 ///     ) ( b_0 + b_1 ) - z_0 - z_2 \cr
 /// 5 & \quad \textbf{return}\ z_2 \beta^{2l} + z_1 \beta^l + z_0
 /// \end{array}}
 /// \f]
 /// \post the result equals `multiplySchool( a , b )` (audit)
 static BigInteger multiplyKaratsuba( const BigInteger & a ,
                                      const BigInteger & b );
 /// the product by the number theoretic transform at every length
 ///
 /// In pseudocode (sections 4.3.3 and 4.6.4), \f$ p_1 , p_2 , p_3 \f$
 /// primes with roots of unity of the length:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiplyFFT}( a , b ) \cr
 /// 2 & \quad \textbf{foreach}\ p \in \{ p_1 , p_2 , p_3 \}\ \textbf{do} \cr
 /// 3 & \qquad c^{( p )} \gets \text{the cyclic convolution of the pieces }
 ///     \text{of 32 bits modulo } p \cr
 /// 4 & \quad c \gets \text{the coefficients from the } c^{( p )} \text{ by }
 ///     \text{the Chinese remainder theorem} \cr
 /// 5 & \quad \textbf{return}\ \sum_i c_i 2^{32 i} \text{, by the carries}
 /// \end{array}}
 /// \f]
 /// \post the result equals `multiplySchool( a , b )` (audit)
 static BigInteger multiplyFFT( const BigInteger & a , const BigInteger & b );

 /// the quotient \f$ a / b \f$ for \f$ b \f$ a divisor of \f$ a \f$, by
 /// `mpz_divexact()` with the backend, else as \f$ a / b \f$
 /// \pre \f$ b \ne 0 \f$ and \f$ b \f$ divides \f$ a \f$
 /// \post the result times \f$ b \f$ is \f$ a \f$ (audit)
 static BigInteger divExact( BigInteger a , const BigInteger & b );

 /// \f$ r \gets r + a b \f$
 static void addMul( BigInteger & r , const BigInteger & a ,
                     const BigInteger & b );

 /// the power \f$ x^n \f$, by `mpz_pow_ui()` with the backend, else by the
 /// binary method
 ///
 /// In pseudocode (section 4.6.3), from the leftmost bit of \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pow}( x , n ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ 1 \cr
 /// 3 & \quad y \gets x \cr
 /// 4 & \quad \textbf{foreach}\ \text{bit } \nu \text{ of } n \text{ after }
 ///     \text{its leftmost 1, from the left}\ \textbf{do} \cr
 /// 5 & \qquad y \gets y^2;\ \textbf{if}\ \nu = 1\ \textbf{then}\ y \gets y x
 ///     \cr
 /// 6 & \quad \textbf{return}\ y
 /// \end{array}}
 /// \f]
 static BigInteger pow( const BigInteger & x , unsigned long n );

 /// the integer square root \f$ s = \lfloor \sqrt{a} \rfloor \f$ and
 /// \f$ r = a - s^2 \f$: \f$ s \f$ by `mpn_sqrtrem()` with the backend,
 /// else by `sqrtNewton()`
 /// \pre \f$ a \ge 0 \f$
 /// \post \f$ s^2 + r = a \f$, \f$ 0 \le r \le 2 s \f$, \f$ s \f$ equal to
 ///       `sqrtNewton( a )` (audits)
 static std::pair< BigInteger , BigInteger > sqrtRem( const BigInteger & a );
 /// the root \f$ \lfloor \sqrt{a} \rfloor \f$ of `sqrtRem()`, with no
 /// remainder
 /// \pre \f$ a \ge 0 \f$
 /// \post the result equals `sqrtNewton( a )` (audit)
 static BigInteger sqrt( const BigInteger & a );

 /// the integer square root by Newton's method from above
 ///
 /// In pseudocode (Newton's method on the integers), the iterates
 /// decreasing to \f$ \lfloor \sqrt{a} \rfloor \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{sqrtNewton}( a ) \cr
 /// 2 & \quad \textbf{if}\ a = 0\ \textbf{then return}\ 0 \cr
 /// 3 & \quad x \gets 2^{\lceil \ell / 2 \rceil}\quad \triangleright\ \ell
 ///     \text{ the bits of } a \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad y \gets \lfloor ( x + \lfloor a / x \rfloor ) / 2 \rfloor \cr
 /// 6 & \qquad \textbf{if}\ y \ge x\ \textbf{then return}\ x \cr
 /// 7 & \qquad x \gets y
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \ge 0 \f$
 static BigInteger sqrtNewton( const BigInteger & a );

 /// the greatest common divisor of \f$ |a| \f$ and \f$ |b| \f$, by
 /// `mpz_gcd()` with the backend, else by Lehmer
 ///
 /// In pseudocode (Algorithm 4.5.2L), \f$ \hat u , \hat v \f$ the leading
 /// 60 bits of \f$ u \ge v \f$ at the same place:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{gcd}( u , v ) \cr
 /// 2 & \quad \textbf{while}\ v \text{ has two digits or more}\ \textbf{do}
 ///     \cr
 /// 3 & \qquad ( A , B , C , D ) \gets ( 1 , 0 , 0 , 1 ) \cr
 /// 4 & \qquad \textbf{while}\ \hat v + C \ne 0\ \textbf{and}\ \hat v + D
 ///     \ne 0\ \textbf{do} \cr
 /// 5 & \qquad\quad q \gets \lfloor ( \hat u + A ) / ( \hat v + C ) \rfloor;
 ///     \ \textbf{if}\ q \ne \lfloor ( \hat u + B ) / ( \hat v + D )
 ///     \rfloor\ \textbf{then break} \cr
 /// 6 & \qquad\quad ( A , B , C , D , \hat u , \hat v ) \gets ( C , D , A -
 ///     q C , B - q D , \hat v , \hat u - q \hat v ) \cr
 /// 7 & \qquad \textbf{if}\ B = 0\ \textbf{then}\ ( u , v ) \gets ( v , u
 ///     \bmod v ) \cr
 /// 8 & \qquad \textbf{else}\ ( u , v ) \gets ( A u + B v , C u + D v ) \cr
 /// 9 & \quad \textbf{return}\ \text{the binary gcd of the last digits}
 /// \end{array}}
 /// \f]
 /// \post it equals `gcdEuclid( a , b )` (audit)
 static BigInteger gcd( const BigInteger & a , const BigInteger & b );

 /// the greatest common divisor of \f$ |a| \f$ and \f$ |b| \f$, by Euclid
 ///
 /// In pseudocode (Algorithm 4.5.2A):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{gcdEuclid}( a , b ) \cr
 /// 2 & \quad a \gets |a|;\ b \gets |b| \cr
 /// 3 & \quad \textbf{while}\ b \ne 0\ \textbf{do}\ ( a , b ) \gets ( b , a
 ///     \bmod b ) \cr
 /// 4 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 static BigInteger gcdEuclid( BigInteger a , BigInteger b );

/*--------------------------------------------------------------------------*/
 private:

 using Mag = detail::Limbs;   ///< a magnitude, no leading zero

 /// the digits of the two factors of the longest convolution of the NTT,
 /// \f$ 2^{26} \f$ pieces of 32 bits, which stays exact
 static constexpr std::size_t kNTTMax = std::size_t( 1 ) << 25;

 /// how a product is made: by the lengths, by Algorithm M, by Karatsuba
 /// down to fewer than 4 digits, or by the NTT
 enum class Method {
  AUTO ,       ///< Algorithm M, Karatsuba or the NTT, by the lengths
  SCHOOL ,     ///< Algorithm M
  KARATSUBA ,  ///< Karatsuba down to fewer than 4 digits
  FFT          ///< the NTT
  };

 /// the number of the sign and of the digits, the leading zeros removed
 BigInteger( bool neg , Mag d );

 /// the leading zeros of \f$ d \f$ removed
 static void trim( Mag & d );
 /// the order of two magnitudes, \f$ -1 \f$, 0 or 1
 static int compareMag( const Mag & a , const Mag & b );

 /// \f$ r \gets a + b \f$, \f$ n_a \ge n_b \f$, the carry returned
 /// (Algorithm 4.3.1A)
 static Limb addN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );
 /// \f$ r \gets a - b \f$, \f$ n_a \ge n_b \f$, the borrow returned
 /// (Algorithm 4.3.1S)
 static Limb subN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );
 /// \f$ r \gets r + a m \f$ on \f$ n \f$ digits, the carry returned
 static Limb addMul1( Limb * r , const Limb * a , std::size_t n , Limb m );
 /// \f$ r \gets r - a m \f$ on \f$ n \f$ digits, the borrow returned
 static Limb subMul1( Limb * r , const Limb * a , std::size_t n , Limb m );

 /// \f$ a + b \f$ of two magnitudes
 static Mag addMag( const Mag & a , const Mag & b );
 /// \f$ a - b \f$ of two magnitudes, \f$ a \ge b \f$
 static Mag subMag( const Mag & a , const Mag & b );
 /// the sum of \f$ a \f$ and of \f$ b \f$ with the sign `bneg`
 static BigInteger addSigned( const BigInteger & a , const BigInteger & b ,
                              bool bneg );

 /// \f$ r \gets a b \f$ of two magnitudes by the method asked, by the
 /// backend if `AUTO`, \f$ r \f$ distinct from \f$ a \f$ and \f$ b \f$
 static void mulMag( Mag & r , const Mag & a , const Mag & b ,
                     Method how = Method::AUTO );
 /// the product \f$ a b \f$ by the method asked, its digits written in
 /// place by `mulMag()`
 static BigInteger product( const BigInteger & a , const BigInteger & b ,
                            Method how );
 /// the product of Algorithm M, \f$ r \f$ of \f$ n_a + n_b \f$ digits
 static void mulSchool( Limb * r , const Limb * a , std::size_t na ,
                        const Limb * b , std::size_t nb );
 /// the digits of scratch space of `mulRec()` for factors of up to
 /// \f$ n \f$ digits
 static std::size_t mulScratch( std::size_t n );
 /// \f$ r \gets a b \f$ on \f$ n_a + n_b \f$ digits, \f$ n_a , n_b \ge 1
 /// \f$, by the method asked, Karatsuba splitting the factors when it
 /// must, its parts in the scratch space \f$ w \f$ of `mulScratch()`
 /// digits
 static void mulRec( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb , Method how ,
                     Limb * w );
 /// the product by the NTT, \f$ n_a + n_b \le \f$ `kNTTMax`
 static Mag mulNTT( const Limb * a , std::size_t na , const Limb * b ,
                    std::size_t nb );

 /// \f$ a \gets \lfloor a / d \rfloor \f$ for one digit \f$ d > 0 \f$, the
 /// remainder returned (exercise 16 of section 4.3.1)
 static Limb divSmall( Mag & a , Limb d );
 /// \f$ a \gets a m + c \f$ for the digits \f$ m \f$ and \f$ c \f$
 static void mulAddSmall( Mag & a , Limb m , Limb c );
 /// the quotient and the remainder of two magnitudes by Algorithm D, or by
 /// the backend unless `school`
 static std::pair< Mag , Mag > divModMag( const Mag & u , const Mag & v ,
                                          bool school );

 /// the figures of the magnitude \f$ a \f$ in the base by the code of the
 /// book, appended to `out`
 static void figures( Mag a , int base , std::string & out );

 /// the gcd of two words, by Euclid
 static std::uint64_t gcdWord( std::uint64_t a , std::uint64_t b );
 /// the gcd of two magnitudes by Algorithm L
 static Mag gcdLehmer( Mag u , Mag v );

#if ADS_BIGINT_GMP
 /// \f$ z \f$ as the read-only view of the \f$ n \f$ digits at \f$ d \f$
 static mpz_srcptr gmpView( mpz_t z , const Limb * d , std::size_t n );
 /// \f$ z \f$ as the read-only view of \f$ x \f$, its sign included
 static mpz_srcptr gmpView( mpz_t z , const BigInteger & x );
 /// the number of GMP \f$ z \f$, its digits copied
 static BigInteger gmpNumber( mpz_srcptr z );
 /// the number of GMP \f$ z \f$, its digits copied, \f$ z \f$ cleared
 static BigInteger gmpTake( mpz_ptr z );
#endif

 friend class Rational;

 /// the invariant: no leading zero, and 0 not negative
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
