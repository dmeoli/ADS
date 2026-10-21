/*--------------------------------------------------------------------------*/
/*--------------------------- File BigInteger.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The integers of any length of Knuth (4.3.1), beyond the book: a sign and
 * the magnitude as a vector of digits (limbs) in base \f$ b = 2^{64} \f$,
 * the least significant first, with no leading zero, the numbers of one
 * or two digits kept in place with no allocation. The long operations
 * work on spans of digits with the scratch space allocated once per call,
 * and the digits are not zeroed before they are written. The product of
 * two digits in two and the sums with carry are one instruction where the
 * compiler offers them (unsigned __int128 in GCC and Clang, _umul128() and
 * _addcarry_u64() in MSVC on x64), else the four products of the halves
 * of 32 bits; ADS_BIGINT_PORTABLE forces the latter. Where GCC or Clang
 * compile for x86-64 with BMI2 and ADX, the loops of the sums and of the
 * products by one digit are in assembly, with MULX, which leaves the
 * carry alone; ADS_BIGINT_NOASM keeps them in C++.
 *
 * - The sum and the difference are Algorithms 4.3.1A and 4.3.1S, a carry
 *   or a borrow per digit, \f$ O( n ) \f$.
 * - The product is Algorithm 4.3.1M, \f$ O( m n ) \f$, for the short
 *   factors: in the assembly by passes of k = 6 or 4 rows over the longer
 *   factor, a digit \f$ a_i \f$ at a time, the window of the k + 1 places
 *   \f$ i , \ldots , i + k \f$ in registers, \f$ r_i \f$ and the high
 *   halves of the k products \f$ a_i b_c \f$ added on the chain of
 *   carries of ADOX and their low halves on that of ADCX: as
 *   \f$ W + a_i B + r_i \le ( b^k - 1 ) + ( b - 1 ) ( b^k - 1 ) + b - 1 =
 *   b^{k+1} - 1 \f$ for the window W and the k digits B of b, no carry
 *   leaves the window and both chains start again from 0 at the next
 *   digit. 31 instructions for 6 products, about 1.22 cycles per product
 *   on a Skylake, 1.4 on 32 digits; the rows left one at a time, and up to
 *   8 digits the passes with no loop. Elsewhere two rows per pass in C++.
 *   A square \f$ a^2 = 2 T + D \f$ takes the products \f$ a_i a_j \f$,
 *   \f$ i < j \f$, of digits of different tiles of 4 digits by the same
 *   passes in T, and the squares of the tiles, side by side, in D. From
 *   kKaratsuba digits it is the product of Karatsuba (4.3.3A) in the
 *   subtractive form,
 *   \f[ a b = z_2 b^{2l} + \bigl( z_0 + z_2 - ( a_0 - a_1 ) ( b_0 - b_1 )
 *       \bigr) b^l + z_0 , \qquad z_0 = a_0 b_0 , \; z_2 = a_1 b_1 , \f]
 *   the sum of the middle in two passes on the two chains of carries,
 *   \f$ M( n ) = 3 M( n / 2 ) + O( n ) = O( n^{\log_2 3} ) \f$; from
 *   kToom3 digits that of Toom-3, \f$ a b = r( x ) \f$, \f$ x = b^k \f$,
 *   from the five products \f$ v_t = a( t ) b( t ) \f$,
 *   \f$ t = 0 , 1 , -1 , 2 , \infty \f$, and the interpolation of Bodrato
 *   and Zanoni, \f$ r_3 = ( ( v_2 - v_{-1} ) / 3 - ( v_1 - v_0 ) ) / 2 -
 *   2 v_\infty \f$, \f$ r_1 = ( v_1 - v_{-1} ) / 2 - r_3 \f$,
 *   \f$ r_2 = v_1 - v_0 - r_1 - r_3 - v_\infty \f$, the divisions exact,
 *   by 3 as the product by \f$ -( b - 1 ) / 3 \f$ and a running
 *   difference, \f$ M( n ) = 5 M( n / 3 ) + O( n ) \f$; the longer
 *   factor cut in pieces as long as the shorter one when their lengths
 *   differ; from kFFT digits it is a convolution of the digits by the
 *   number theoretic transform (4.3.3, 4.6.4) and the Chinese remainder
 *   theorem (4.3.2), \f$ O( n \log n ) \f$, the top t digits of the
 *   longer factor left to rows of products by one digit when the
 *   transform of the rest is shorter by more than they cost, about
 *   \f$ 12 L \log_2 L \f$ cycles for L terms against \f$ 1.6 t n_b \f$.
 *   With AVX2 and the fused product and sum, the
 *   transform runs on lanes of doubles, one digit per term, modulo three
 *   primes \f$ p < 2^{50} \f$ with \f$ 3 \cdot 2^{30} \mid p - 1 \f$,
 *   on \f$ 2^k \f$ or \f$ 3 \cdot 2^k \f$ terms, whichever is shorter, the
 *   residues exact integers in \f$ [ -p , p ] \f$ and the product of two
 *   of them reduced by the quotient rounded from the product by
 *   \f$ 1 / p \f$, two rounds per pass over the terms: exact, since a
 *   coefficient, below \f$ n \, 2^{128} \f$, stays below the product of
 *   the primes, \f$ 2^{149} \f$, up to \f$ 2^{21} \f$ digits per
 *   factor. The coefficient i is \f$ y_1 + p_1 t_2 + p_1 p_2 t_3 \f$ by
 *   Garner, \f$ 0 \le y_1 , t_2 , t_3 < 2^{50} \f$: one digit each, so the
 *   product is \f$ Y_1 + p_1 T_2 + p_1 p_2 T_3 \f$ for the numbers of
 *   those digits, three passes of products by one digit. Elsewhere and
 *   past that the digits are cut in two pieces of
 *   32 bits each, modulo three primes \f$ p < 2^{31} \f$ with
 *   \f$ 2^{26} \mid p - 1 \f$, on 8 lanes of AVX2 or one at a time, the
 *   products by the roots of unity those of Shoup and the others those of
 *   Montgomery: exact up to transforms of \f$ 2^{26} \f$ pieces, i.e.,
 *   products of \f$ 2^{25} \f$ digits, past which Karatsuba splits the
 *   factors. multiplySchool(), multiplyKaratsuba() and multiplyFFT() are
 *   kept apart for the comparison.
 * - The quotient and the remainder are Algorithm 4.3.1D for the short
 *   quotients: the divisor is shifted until its leading digit has the top
 *   bit set, and each trial quotient \f$ \hat q \f$ is the quotient of the
 *   three leading digits of the remainder by the two of the divisor, by
 *   their reciprocal (Moeller and Granlund, Algorithms 5 and 6), which
 *   exceeds the true one by at most 1 and makes the correction after the
 *   multiplication and subtraction ("add back") happen with probability
 *   about 2 / b; below kDivShort digits on the stack. From kDivBlock
 *   digits of divisor (kDivBlock6 for K = 6) the quotient goes K = 4 or 6
 *   digits at a time: the K digits Q of the window, below \f$ v b^K \f$,
 *   from its top \f$ t + K \f$ digits by the top \f$ t = K + 2 \f$ of v,
 *   \f$ Q \le \hat Q \le Q + 1 \f$ as \f$ \hat Q - Q \le 1 + Q / v_t <
 *   2 \f$, then the window less \f$ \hat Q v \f$ by one pass of K rows on
 *   the complements, \f$ r - a b = ( r_{top} - s ) b^n +
 *   \overline{\overline{r} + a b} \f$ with s the top digits of
 *   \f$ \overline{r} + a b \f$, and v added back while it is negative. A
 *   division of two digits by one, in the short division,
 *   is two products by a reciprocal of the divisor computed once, from a
 *   table and three steps of Newton (Moeller and Granlund, Algorithms 2
 *   and 4). The remainder alone by a digit below \f$ 2^{61} \f$ from
 *   kModFold digits folds 4 digits per step into a remainder of two
 *   digits, by the residues of \f$ b^k \f$, with one division at the end;
 *   divExact() divides by one digit by its inverse modulo b (Hensel), from
 *   kDivExact2 digits both ways at once (Krandick and Jebelean): with
 *   \f$ q = q_h b^m + q_l \f$, \f$ q_l d = ( a \bmod b^m ) + c b^m \f$ for
 *   the last borrow \f$ c < d \f$ of Hensel's division of the low m
 *   digits, so that \f$ \lfloor a / b^m \rfloor = q_h d + c \f$ and its
 *   quotient by d is \f$ q_h \f$; \f$ m = n - n / 3 \f$, as the chain of
 *   the division from the top is about twice as long per digit.
 *   From kDivide digits of quotient the division is recursive (Burnikel
 *   and Ziegler; Brent and Zimmermann, Algorithm 1.8): the upper half of
 *   the quotient from the upper half of the divisor, corrected at most
 *   twice, then the lower half, \f$ O( M( n ) \log n ) \f$ for products in
 *   \f$ M( n ) \f$, a quotient longer than the divisor taken by blocks as
 *   long as the divisor. From kNewton digits of divisor, or kNewtonKept if
 *   its reciprocal is kept, it goes by blocks
 *   of about half the quotient, each block the product of the top digits
 *   of the remainder by a reciprocal of the top digits of the divisor
 *   (Newton's method, Brent and Zimmermann, Algorithm 3.5; Barrett), short
 *   by a few units at most, and the product of the block by the divisor
 *   taken modulo \f$ b^N - 1 \f$ by a cyclic convolution as long as the
 *   divisor, the digits that wrap known from the remainder and the lowest
 *   digit, \f$ O( M( n ) ) \f$. The reciprocal \f$ X = b^n + x \f$ of a
 *   of n digits, \f$ a X < b^{2n} \le a ( X + 2 ) \f$, is
 *   \f$ X = X_h b^l + \lfloor X_h t / b^{2h} \rfloor \f$ from that of the
 *   top \f$ h = n - l \f$ digits, \f$ t = b^{n+h} - a X_h < 2 a \f$: t
 *   from \f$ a X_h \f$ modulo \f$ b^N - 1 \f$, \f$ N \ge n \f$, as t, its
 *   sum with \f$ b^N - 1 \f$ and, \f$ X_h \f$ too large, its difference
 *   from it differ in the lowest digit, which is
 *   \f$ -a_0 x_0 \bmod b \f$; and of t only its h + 1 digits from the
 *   place l are not 0, about \f$ 1.8 \f$ to \f$ 2.1 M( n ) \f$ in all. The
 *   quotient alone, for operator/(), is first that of the top
 *   \f$ 2 m + 1 \f$ digits of the dividend by the top \f$ m + 2 \f$ of the
 *   divisor if it has m digits, \f$ m < n - 3 \f$: with
 *   \f$ v = v_t b^s + v_l \f$ and \f$ u = u_t b^s + u_l \f$, the quotient
 *   \f$ q_t \f$ of \f$ u_t \f$ by \f$ v_t \f$ is q or q + 1, as
 *   \f$ u_t / v_t - u_t / ( v_t + 1 ) < 2 / b \f$, and it is q unless
 *   \f$ r_t b^s + u_l < q_t v_l < b^{|q_t| + s} \f$, which a remainder
 *   \f$ r_t \f$ longer than \f$ q_t \f$ rules out with no product. Else
 *   it is that of the dividend with one digit more, so that the last
 *   digit tells when the quotient may be one off: below kDivCut digits of
 *   divisor by Algorithm D with the divisor cut to the j + 2 leading
 *   digits for the digit j, about half the work, and from kNewton digits
 *   with no remainder for the last block; the rare doubtful case is
 *   checked by a product. divModSchool() is Algorithm D alone. The
 *   quotient is truncated toward zero, as for the integers of C++.
 * - The conversion to and from the bases 2 to 36 (4.4) works with the
 *   largest power P of the base in one digit, \f$ 10^{19} \f$ for the base
 *   10. In the bases that are powers of 2 it is a slice of bits per
 *   figure, \f$ O( n ) \f$. In the others it is divide and conquer
 *   (Brent and Zimmermann, section 1.7), with the powers \f$ P^{2^i} \f$
 *   without their low digits 0 (about 30 per cent of the digits in the
 *   base 10) and their reciprocals kept per thread and base for the next
 *   conversions: the output splits the number by the power nearest the
 *   square root, the lower part padded with zeros, and the input joins two
 *   halves with one product per level, \f$ O( M( n ) \log n ) \f$; below
 *   kConvert digits they divide or multiply by P once per many figures,
 *   as toStringSchool() and fromStringSchool() do at every length. On the
 *   output, a remainder \f$ x < P^c \f$, \f$ c = 2^j \f$ chunks, below
 *   kConvertF digits goes by its fraction \f$ f = x / P^c \f$ (Bouvier and
 *   Zimmermann's scaled remainder tree), the product by the reciprocal of
 *   \f$ P^c \f$ kept, rounded up, to the q + 2 digits of \f$ P^c \f$ plus
 *   2: with \f$ f = ( X + \theta ) / P^c \f$, X the integer of the c
 *   chunks, the integer parts of \f$ f P^i \f$, \f$ i \le c \f$, are
 *   those of \f$ X / P^{c-i} \f$ while \f$ 0 \le \theta < 1 \f$. The low
 *   half of the chunks is the fraction of \f$ f P^{c/2} \f$, the same
 *   \f$ \theta \f$, by the band of the products that it needs (mulBand(),
 *   short by 2 units at most); the high half is the top digits of f, its
 *   \f$ \theta \f$ the fraction of the low half: f is rounded down if
 *   that fraction is at least 1/2, else up, and each half keeps its
 *   direction, so that \f$ \theta \f$ stays below 1/2 by less than
 *   \f$ 3 b^{-2} \f$ per level if up and above it if down, and no run of
 *   9 or of 0 carries wrong. From kFracLeaf chunks down the chunks are
 *   the integer parts of \f$ f P \f$, \f$ f P^2 \f$, ..., f cut to the
 *   digits that the chunks left need, and the 19 figures of a chunk come
 *   by pairs from a table.
 * - pow() is the binary method of 4.6.3, from left to right, squares and
 *   products by x. gcd() is the algorithm of Lehmer (4.5.2L): the leading
 *   62 bits of the two numbers run the algorithm of Euclid in single
 *   precision while the two bounds of each quotient agree and the
 *   cofactors stay below \f$ 2^{30} \f$, and then one pass of the two
 *   linear combinations replaces many long divisions, down to one digit,
 *   where the binary algorithm ends, with no branch but its loop; the
 *   number of one digit d = 2^t e, e odd, meets the long one by the
 *   folded remainder by e, or below kHensel digits by Hensel's division,
 *   whose \f$ a b^{-n} \f$ modulo e has the same gcd with e; gcdEuclid()
 *   is the algorithm of Euclid.
 * - With ADS_WITH_GMP defined (by hand, or by the option of CMake of the
 *   same name), the products, the quotients, the conversions in the bases
 *   that are not powers of 2 and the greatest common divisor go to the
 *   mpn and mpz layers of GMP (mpn_mul(), mpn_sqr(), mpn_tdiv_qr(),
 *   mpz_tdiv_q(), mpz_divexact(), mpz_get_str(), mpn_set_str(),
 *   mpz_gcd()) in the ranges of lengths where GMP was the faster on a Xeon
 *   Gold 5120 (the thresholds kGmp...), on the digits of BigInteger as they
 *   are, read in place: for factors of \f$ n_a \ge n_b \f$ digits, the
 *   product when \f$ k_{mul} \le n_b < k_{top} \f$, with
 *   \f$ k_{top} \f$ kGmpMulTop if \f$ n_a < 2 n_b \f$ and kGmpMulLongTop
 *   otherwise, the square of n digits when \f$ n < \f$ kGmpSqrTop; for a
 *   divisor of n digits and a quotient of m or m + 1, the division with
 *   the remainder when \f$ n < \f$ kGmpDivTop or \f$ m < \f$ kGmpShort,
 *   the quotient alone when \f$ m > n \f$ and \f$ n < \f$ kGmpDivTop, or
 *   when \f$ m \le n \f$, \f$ n \ge \f$ kGmpQuot and either
 *   \f$ n < \f$ kGmpQuotTop or \f$ 2 m \le n \f$; the exact quotient
 *   (by Jebelean's algorithm, from the low digits) when \f$ n < \f$
 *   kGmpExactTop; the conversion of n digits to a base not a power of 2
 *   when kConvert \f$ \le n < \f$ kGmpToStringTop, and from one always;
 *   the gcd (by the subquadratic algorithm of Moeller) when the shorter
 *   argument has kGmpGcd digits or more. Elsewhere, and in the methods
 *   asked by name (multiplySchool(), multiplyKaratsuba(), multiplyFFT(),
 *   divModSchool(), toStringSchool(), fromStringSchool(), gcdEuclid()),
 *   BigInteger keeps its own code, which is all of it without the macro,
 *   when <gmp.h> is not even included; the same where a limb of GMP is
 *   not a word of 64 bits. GMP is free software under the LGPL v3 (or the
 *   GPL v2), as ADS is under the LGPL v3; note that GMP aborts where it
 *   cannot allocate, while BigInteger throws std::bad_alloc.
 *
 * The contracts [see Contract.h]: the divisor is not 0, the base is in
 * [ 2 , 36 ] and the string a number in it; the invariant is the normal
 * form (no leading zero, 0 not negative); divMod() ensures a = q b + r
 * with |r| < |b| and r of the sign of a, and that it equals divModSchool(),
 * operator/() that its quotient does, divExact() that its result times b
 * is a; multiplyKaratsuba() and multiplyFFT() that they equal
 * multiplySchool(); the conversions that they equal toStringSchool() and
 * fromStringSchool(), gcd() that it equals gcdEuclid(), all audits. The
 * operations are defined in BigInteger.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.1, 4.3.2, 4.3.3, 4.4, 4.5.2 and
 *      4.6.3
 * \see A. Karatsuba, Yu. Ofman, Multiplication of multidigit numbers on
 *      automata, Soviet Physics Doklady 7, 595-596, 1963
 * \see A. L. Toom, The complexity of a scheme of functional elements
 *      realizing the multiplication of integers, Soviet Mathematics
 *      Doklady 3, 714-716, 1963
 * \see M. Bodrato, A. Zanoni, Integer and polynomial multiplication:
 *      towards optimal Toom-Cook matrices, Proceedings of ISSAC 2007,
 *      17-24, 2007, doi:10.1145/1277548.1277552
 * \see R. P. Brent, P. Zimmermann, Modern Computer Arithmetic, Cambridge
 *      University Press, 2010, sections 1.3, 1.4, 1.6, 1.7, 2.4 and 3.4
 * \see C. Burnikel, J. Ziegler, Fast recursive division, research report
 *      MPI-I-98-1-022, Max-Planck-Institut fuer Informatik, 1998
 * \see N. Moeller, T. Granlund, Improved division by invariant integers,
 *      IEEE Transactions on Computers 60(2), 165-175, 2011,
 *      doi:10.1109/TC.2010.143
 * \see P. L. Montgomery, Modular multiplication without trial division,
 *      Mathematics of Computation 44(170), 519-521, 1985,
 *      doi:10.1090/S0025-5718-1985-0777282-X
 * \see V. Shoup, NTL: a library for doing number theory, the products
 *      by a constant with its precomputed quotient
 * \see D. Harvey, Faster arithmetic for number-theoretic transforms,
 *      Journal of Symbolic Computation 60, 113-119, 2014,
 *      doi:10.1016/j.jsc.2013.09.002
 * \see T. Jebelean, An algorithm for exact division, Journal of Symbolic
 *      Computation 15(2), 169-180, 1993, doi:10.1006/jsco.1993.1012
 * \see W. Krandick, T. Jebelean, Bidirectional exact integer division,
 *      Journal of Symbolic Computation 21(4-6), 441-455, 1996,
 *      doi:10.1006/jsco.1996.0024
 * \see P. Barrett, Implementing the Rivest Shamir and Adleman public key
 *      encryption algorithm on a standard digital signal processor, Proc.
 *      CRYPTO 1986, Lecture Notes in Computer Science 263, 311-323, 1987,
 *      doi:10.1007/3-540-47721-7_24
 * \see C. Bouvier, P. Zimmermann, Division-free binary-to-decimal
 *      conversion, IEEE Transactions on Computers 63(8), 1895-1901, 2014,
 *      doi:10.1109/TC.2014.2315621
 * \see D. H. Lehmer, Euclid's algorithm for large numbers, The American
 *      Mathematical Monthly 45(4), 227-233, 1938,
 *      doi:10.1080/00029890.1938.11990797
 * \see N. Moeller, On Schoenhage's algorithm and subquadratic integer gcd
 *      computation, Mathematics of Computation 77(261), 589-607, 2008,
 *      doi:10.1090/S0025-5718-07-02017-0
 * \see T. Granlund and the GMP development team, GNU MP: The GNU Multiple
 *      Precision Arithmetic Library, version 6.3.0, 2023,
 *      https://gmplib.org/gmp-man-6.3.0.pdf, the layers mpn and mpz of the
 *      backend
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012,
 *      sections 9.2 and 9.4
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
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// the width of the product of two digits done by the compiler: 128 with
// unsigned __int128, 64 with the intrinsics of MSVC on x64, 0 for none
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

// the lanes of the NTT: 256 with AVX2, else 0, one at a time
#ifndef ADS_BIGINT_SIMD
 #if defined( __AVX2__ )
  #define ADS_BIGINT_SIMD 256
 #else
  #define ADS_BIGINT_SIMD 0
 #endif
#endif
#if ADS_BIGINT_SIMD > 0
 #include <immintrin.h>
#endif

// the NTT on doubles, with the fused product and sum, where the lanes of
// AVX2 and the products of 128 bits are there
#ifndef ADS_BIGINT_FNTT
 #if ( ADS_BIGINT_SIMD >= 256 ) && defined( __FMA__ ) && \
     ( ADS_BIGINT_WIDE == 128 )
  #define ADS_BIGINT_FNTT 1
 #else
  #define ADS_BIGINT_FNTT 0
 #endif
#endif
#if ADS_BIGINT_FNTT
 #include <cmath>
#endif

// the loops of the sums and of the products by one digit in the assembly
// of x86-64, with MULX, where GCC or Clang compile for a processor that has
// BMI2 and ADX; ADS_BIGINT_NOASM keeps them in C++
#ifndef ADS_BIGINT_ASM
 #if ( ADS_BIGINT_WIDE == 128 ) && defined( __x86_64__ ) && \
     defined( __GNUC__ ) && defined( __BMI2__ ) && defined( __ADX__ ) && \
     ! defined( ADS_BIGINT_NOASM )
  #define ADS_BIGINT_ASM 1
 #else
  #define ADS_BIGINT_ASM 0
 #endif
#endif

// the backend of GMP, with ADS_WITH_GMP: the products, the quotients, the
// conversions and the greatest common divisor go to the mpn and mpz layers
// of GMP in the ranges of lengths where those are faster, on the digits of
// BigInteger as they are; only where a limb of GMP has 64 bits, all of
// them numbers, else BigInteger stays on its own code
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

/// std::allocator, but the elements made with no value are left without
/// one: the digits are written before they are read, and the memset of
/// std::vector would cost a pass over them
template< class T >
struct NoInit : std::allocator< T > {
 template< class U > struct rebind { using other = NoInit< U >; };

 NoInit( void ) = default;
 template< class U > NoInit( const NoInit< U > & ) noexcept {}

 template< class U > void construct( U * p ) noexcept {
  ::new( static_cast< void * >( p ) ) U;
  }
 template< class U , class... A > void construct( U * p , A &&... a ) {
  ::new( static_cast< void * >( p ) ) U( std::forward< A >( a )... );
  }
 };

/// the blocks of digits freed, kept per thread for the next numbers: 8 at
/// most of each capacity \f$ 2^k \f$, \f$ 2 \le k < 12 \f$, and 2 up to
/// \f$ k = 16 \f$, about 2 MiB in all at most, so that the
/// temporaries of the operations cost no call to the allocator once the
/// thread has made a few; trivial, so that the thread reads it with no
/// guard, the blocks freed at the end of the thread by Reaper
struct DigitPool {
 static constexpr unsigned kBins = 17 , kKeep = 8;
 std::uint64_t * bin[ kBins ][ kKeep ];
 unsigned cnt[ kBins ];
 bool armed , dead;                         ///< the reaper made, and done
 };

inline constinit thread_local DigitPool digitPool{};

struct DigitReaper {
 ~DigitReaper( void ) {
  for( unsigned k = 0 ; k < DigitPool::kBins ; ++k )
   for( unsigned i = 0 ; i < digitPool.cnt[ k ] ; ++i )
    std::allocator< std::uint64_t >().deallocate( digitPool.bin[ k ][ i ] ,
                                                  std::size_t( 1 ) << k );
  for( unsigned & c : digitPool.cnt ) c = 0;
  digitPool.dead = true;                    // the blocks freed later go back
  }
 };

inline thread_local DigitReaper digitReaper;

/// the digits of a number, as a std::vector but with up to two of them
/// in place, so that the numbers of one or two digits are made with no
/// allocation; the digits made with no value are left without one; the
/// capacities from 4 to 2^16 are powers of 2, from DigitPool
class Digits {

 public:

 using T = std::uint64_t;

 Digits( void ) noexcept {}
 explicit Digits( std::size_t n ) { resize( n ); }
 Digits( std::size_t n , T v ) { resize( n , v ); }
 Digits( const T * b , const T * e ) {
  resize( std::size_t( e - b ) );
  std::copy( b , e , data() );
  }
 Digits( std::initializer_list< T > l ) {
  resize( l.size() );
  std::copy( l.begin() , l.end() , data() );
  }
 Digits( const Digits & o ) : Digits( o.data() , o.data() + o.f_n ) {}
 Digits( Digits && o ) noexcept { take( o ); }
 Digits & operator=( const Digits & o ) {
  if( this != &o ) {
   f_n = 0;
   resize( o.f_n );
   std::copy( o.data() , o.data() + o.f_n , data() );
   }
  return( *this );
  }
 Digits & operator=( Digits && o ) noexcept {
  if( this != &o ) {
   free();
   take( o );
   }
  return( *this );
  }
 ~Digits( void ) { free(); }

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }
 T * data( void ) { return( f_p ); }
 const T * data( void ) const { return( f_p ); }
 T * begin( void ) { return( data() ); }
 T * end( void ) { return( data() + f_n ); }
 const T * begin( void ) const { return( data() ); }
 const T * end( void ) const { return( data() + f_n ); }
 T & operator[]( std::size_t i ) { return( data()[ i ] ); }
 const T & operator[]( std::size_t i ) const { return( data()[ i ] ); }
 T & back( void ) { return( data()[ f_n - 1 ] ); }
 const T & back( void ) const { return( data()[ f_n - 1 ] ); }

 void reserve( std::size_t n ) { grow( n , n ); }
 void resize( std::size_t n ) {
  grow( n , n );
  f_n = n;
  }
 void resize( std::size_t n , T v ) {
  grow( n , n );
  if( n > f_n ) std::fill( data() + f_n , data() + n , v );
  f_n = n;
  }
 void assign( std::size_t n , T v ) {
  f_n = 0;
  resize( n , v );
  }
 void push_back( T v ) {
  grow( f_n + 1 , 2 * f_n );
  data()[ f_n++ ] = v;
  }
 void pop_back( void ) { --f_n; }

 friend bool operator==( const Digits & a , const Digits & b ) {
  return( ( a.f_n == b.f_n ) &&
          std::equal( a.data() , a.data() + a.f_n , b.data() ) );
  }

 private:

 static constexpr std::size_t kIn = 2;      ///< the digits in place

 /// room for n digits, c if it must move to the heap
 void grow( std::size_t n , std::size_t c ) {
  if( n <= f_cap ) return;
  c = std::max( n , c );
  T * q;
  if( c < ( std::size_t( 1 ) << ( DigitPool::kBins - 1 ) ) ) {
   const unsigned k = unsigned( std::bit_width( std::max< std::size_t >( c ,
                                                              4 ) - 1 ) );
   c = std::size_t( 1 ) << k;
   DigitPool & P = digitPool;
   q = P.cnt[ k ] ? P.bin[ k ][ --P.cnt[ k ] ]
                  : std::allocator< T >().allocate( c );
   }
  else q = std::allocator< T >().allocate( c );
  std::copy( data() , data() + f_n , q );
  free();
  f_p = q;
  f_cap = c;
  }
 void free( void ) {
  if( f_p != f_in ) {
   DigitPool & P = digitPool;
   const unsigned k = unsigned( std::countr_zero( f_cap ) );
   if( std::has_single_bit( f_cap ) && ( k < DigitPool::kBins ) &&
       ( P.cnt[ k ] < ( k < 12 ? DigitPool::kKeep : 2u ) ) && ! P.dead ) {
    if( ! P.armed ) {                      // the reaper, once per thread
     (void) &digitReaper;
     P.armed = true;
     }
    P.bin[ k ][ P.cnt[ k ]++ ] = f_p;
    }
   else std::allocator< T >().deallocate( f_p , f_cap );
   }
  f_p = f_in;
  f_cap = kIn;
  }
 void take( Digits & o ) {
  f_n = o.f_n;
  f_cap = o.f_cap;
  if( o.f_p != o.f_in ) f_p = o.f_p;
  else std::copy( o.f_in , o.f_in + f_n , f_in );
  o.f_p = o.f_in;
  o.f_n = 0;
  o.f_cap = kIn;
  }

 T * f_p = f_in;                            ///< f_in or on the heap
 std::size_t f_n = 0 , f_cap = kIn;
 T f_in[ kIn ] = {};

 };  // end( class Digits )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BigInteger -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer of any length, sign and magnitude in base 2^64
class BigInteger {

/*--------------------------------------------------------------------------*/
 public:

 using Limb = std::uint64_t;

 // the thresholds, in digits, from the timings of bench/bignum.cpp on a
 // Xeon Gold 5120, against GMP
 static constexpr std::size_t kKaratsuba = 40;   ///< limbs for Karatsuba
 static constexpr std::size_t kToom3 = 120;      ///< limbs for Toom-3
 static constexpr std::size_t kFFT = 448;        ///< limbs for the NTT
 static constexpr std::size_t kDivide = 96;      ///< quotient limbs, recursion
 static constexpr std::size_t kConvert = 24;     ///< limbs, divide and conquer
 static constexpr std::size_t kConvertF = 64;    ///< limbs, by fractions
 static constexpr std::size_t kFracLeaf = 32;    ///< chunks, by products
 static constexpr std::size_t kFracLevel = 4;    ///< 2^k chunks, fractions
 static constexpr std::size_t npos = std::size_t( -1 );
 static constexpr std::size_t kModFold = 16;     ///< limbs, folded remainder
 static constexpr std::size_t kHensel = 24;      ///< limbs, gcd by folding
 static constexpr std::size_t kNewton = 768;     ///< limbs, by the reciprocal
 static constexpr std::size_t kNewtonKept = 512; ///< limbs, the same if kept
 static constexpr std::size_t kRecip = 32;       ///< limbs, reciprocal by D
 static constexpr std::size_t kDivCut = 900;     ///< limbs, quotient alone
 static constexpr std::size_t kDivShort = 16;    ///< limbs, on the stack
 static constexpr std::size_t kDivBlock = 40;    ///< limbs, 4 digits a step
 static constexpr std::size_t kDivBlock6 = 64;   ///< limbs, 6 digits a step
 static constexpr std::size_t kDivExact2 = 32;   ///< limbs, exact both ways

 // with ADS_WITH_GMP, the lengths in digits that go to GMP, where it was
 // faster in the timings of the same benchmark: the products whose shorter
 // factor has [ kGmpMul , kGmpMulTop ) digits, [ kGmpMul , kGmpMulLongTop )
 // when the longer one is twice as long or more, the squares below
 // kGmpSqrTop; the quotients with the remainder whose divisor is below
 // kGmpDivTop or whose quotient is below kGmpShort, the quotients alone
 // longer than the divisor as those, the others from kGmpQuot digits of
 // divisor below kGmpQuotTop, or at most half as long as the divisor; the
 // exact quotients below kGmpExactTop, the conversions to a base not a
 // power of 2 from kConvert digits below kGmpToStringTop, from one at any
 // length; the greatest common divisors whose shorter argument has
 // kGmpGcd digits or more
 static constexpr std::size_t kGmpMul = 5;          ///< limbs, mpn_mul
 static constexpr std::size_t kGmpMulTop = 1536;
 static constexpr std::size_t kGmpMulLongTop = 704;
 static constexpr std::size_t kGmpSqrTop = 1920;    ///< limbs, mpn_sqr
 static constexpr std::size_t kGmpDivTop = 3584;    ///< mpn_tdiv_qr
 static constexpr std::size_t kGmpShort = 512;      ///< quotient limbs
 static constexpr std::size_t kGmpQuot = 8;         ///< mpz_tdiv_q
 static constexpr std::size_t kGmpQuotTop = 49152;
 static constexpr std::size_t kGmpExactTop = 262144;   ///< mpz_divexact
 static constexpr std::size_t kGmpToStringTop = 49152; ///< mpz_get_str
 static constexpr std::size_t kGmpGcd = 2;          ///< limbs, mpz_gcd

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

 /// the number of digits (limbs) in base 2^64
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

 /// a / b for b a divisor of a: by one digit by the inverse of b modulo
 /// 2^64 (Hensel's division, 4.3.1 exercise 16), with no division of two
 /// digits by one, in place of a, else as a / b
 /// \pre b != 0 and b divides a
 /// \post the result times b is a (an audit)
 static BigInteger divExact( BigInteger a , const BigInteger & b );

 /// r += a b, in place when a or b has one digit, r at least as long as
 /// the other and of the sign of the product
 static void addMul( BigInteger & r , const BigInteger & a ,
                     const BigInteger & b );

 /// x^n by the binary method
 static BigInteger pow( const BigInteger & x , unsigned long n );

 /// the greatest common divisor of |a| and |b|, by Lehmer
 /// \post it equals gcdEuclid( a , b ) (an audit)
 static BigInteger gcd( const BigInteger & a , const BigInteger & b );

 /// the greatest common divisor of |a| and |b|, by Euclid
 static BigInteger gcdEuclid( BigInteger a , BigInteger b );

/*--------------------------------------------------------------------------*/
 private:

 using Mag = detail::Digits;

 /// the longest product of the NTT, 2^25 digits, 2^26 pieces of 32 bits
 static constexpr std::size_t kNTTMax = std::size_t( 1 ) << 25;

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

 /// r += a ( m0 + m1 b ), n digits, r[ n ] and r[ n + 1 ] written
 static void addMul2( Limb * r , const Limb * a , std::size_t n , Limb m0 ,
                      Limb m1 );

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

 /// r = a^2, r of 2 n digits, n <= 4
 static void sqrTile( Limb * r , const Limb * a , std::size_t n );

 /// r = a b, n digits each, on the scratch w of mulScratch( n ) digits:
 /// Algorithm M, from kKaratsuba digits Karatsuba, from toom digits
 /// Toom-3, from fft digits the NTT
 static void mulN( Limb * r , const Limb * a , const Limb * b ,
                   std::size_t n , Limb * w , std::size_t fft ,
                   std::size_t toom );
 static std::size_t mulScratch( std::size_t n );

 /// one step of Karatsuba and of Toom-3 on n digits, the products of the
 /// parts by mulN()
 static void karatsuba( Limb * r , const Limb * a , const Limb * b ,
                        std::size_t n , Limb * w , std::size_t fft ,
                        std::size_t toom );
 static void toom3( Limb * r , const Limb * a , const Limb * b ,
                    std::size_t n , Limb * w , std::size_t fft ,
                    std::size_t toom );

 /// a /= 3 for a multiple of 3, by Hensel's division
 static void divExact3( Limb * a , std::size_t n );

 /// r = a + 2 b and r = a - 2 b, n > 0 digits, the carry or the borrow
 static Limb addLsh1( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );
 static Limb subLsh1( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );

 /// r = ( a + b ) / 2 and r = ( a - b ) / 2, n > 0 digits, the carry or
 /// the borrow the top bit
 static void rsh1Add( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );
 static void rsh1Sub( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );

 /// r = a b by the NTT, na + nb <= kNTTMax; by the transforms of n terms
 /// on the lanes of O
 static void mulNTT( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb );
 template< class O >
 static void mulNTT3( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t n );
 template< class O >
 static void mulFNTT( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t n ,
                      bool cyclic = false );

 /// r = a b modulo b^n - 1, n digits, by the cyclic convolution of n
 /// terms: n a power of 2 from 16 or 3 times one from 24, n <= 2^22,
 /// na , nb <= n and min( na , nb ) <= 2^21
 static void mulCyclic( Limb * r , const Limb * a , std::size_t na ,
                        const Limb * b , std::size_t nb , std::size_t n );

 /// r = a b, r of na + nb digits, by the method of the lengths; the NTT
 /// from fft digits
 static void mulP( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb ,
                   std::size_t fft = kFFT , std::size_t toom = kToom3 );
 static Mag mulMag( const Mag & a , const Mag & b , std::size_t fft = kFFT ,
                    std::size_t toom = kToom3 );

 /// mulP() for na >= nb >= kKaratsuba below the NTT, apart so that the
 /// short products do not set up its scratch on the stack
 static void mulRec( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb , std::size_t fft ,
                     std::size_t toom );

 /// a /= d, the remainder; the same for d = dn / 2^s, dn with the top bit
 /// set and its reciprocal v; the remainder of a by d
 static Limb divSmall( Mag & a , Limb d );
 static Limb divSmall( Mag & a , Limb dn , Limb v , int s );
 static Limb modSmall( const Limb * a , std::size_t n , Limb d );

 /// a /= d, d a divisor of a, by Hensel's division
 static void divExactSmall( Mag & a , Limb d );

 /// a = a m + c
 static void mulAddSmall( Mag & a , Limb m , Limb c );

 /// the quotient of a, n + m digits, by b, n >= 2 digits with the top bit
 /// set, a < 2 b 2^{64 m}: its m low digits in q and the top one, 0 or 1,
 /// returned, the remainder in a[ 0 , n ); by Algorithm D, and recursively
 /// for m <= n with the scratch t of n digits; with cut, the digit j of
 /// the quotient by the j + 2 leading digits of b alone, which leaves a
 /// quotient one off at most and no remainder [see divModMag()], or 2
 /// returned for a digit of b or more
 static Limb divBase( Limb * a , std::size_t m , const Limb * b ,
                      std::size_t n , Limb * q , bool cut = false );
 /// the digits of the quotient K at a time as divBase() takes them, while
 /// m >= K and the divisor, cut or not, has least digits or more; false,
 /// or true if a cut divisor gives a digit too large [see divBase()]
 template< std::size_t K >
 static bool divBlocks( Limb * a , std::size_t & m , const Limb * b ,
                        std::size_t n , Limb * q , bool cut , Limb d1 ,
                        Limb d0 , Limb v , std::size_t least );
 static Limb divRec( Limb * a , std::size_t m , const Limb * b ,
                     std::size_t n , Limb * q , Limb * t );

 /// x, n digits, such that X = b^n + x has a X < b^{2n} <= a ( X + 2 ),
 /// for a of n digits with the top bit set: by Newton's method (Brent
 /// and Zimmermann, Algorithm 3.5), from kRecip digits
 static void recip( Limb * x , const Limb * a , std::size_t n );

 /// the quotient of u by v, 2 <= v.size() < kDivShort and
 /// u.size() < 2 kDivShort, u >= v, and the remainder if r, by Algorithm D
 /// on the stack
 static void divShort( const Mag & u , const Mag & v , Mag & q , Mag * r );

 /// as divBase(), by blocks of quotient digits from the reciprocal of the
 /// top digits of b, the products by b modulo b^N - 1 where the NTT
 /// gives them (Barrett; Brent and Zimmermann, section 2.4.1); the
 /// reciprocal is kept in keep, if given, for the next divisions by b,
 /// whose top digits serve the shorter blocks; with quot, the last block
 /// skips the remainder when its quotient cannot reach its last digit,
 /// which leaves the digits above it exact
 static Limb divNewton( Limb * a , std::size_t m , const Limb * b ,
                        std::size_t n , Limb * q , Mag * keep = nullptr ,
                        bool quot = false );

 /// the quotient and the remainder of the magnitudes, recursive or by
 /// Algorithm D alone if school, the reciprocal kept in keep as by
 /// divNewton(), the remainder left empty unless rem
 static std::pair< Mag , Mag > divModMag( const Mag & u , const Mag & v ,
                                          bool school = false ,
                                          Mag * keep = nullptr ,
                                          bool rem = true );

 /// ( u , v ) = ( A u + B v , C u + D v ) in one pass, u >= v, both
 /// results nonnegative and at most u, |A| , |B| , |C| , |D| < 2^30, A
 /// and B of opposite signs, C and D too
 static void linComb( Mag & u , Mag & v , std::int64_t A , std::int64_t B ,
                      std::int64_t C , std::int64_t D );

 /// the greatest common divisor of the magnitudes, by Lehmer
 static Mag gcdMag( Mag u , Mag v );

 /// out += the figures of a in the base, at least pad of them, dividing
 /// by P, then those of b, if given, at least padb, divided with them
 static void figures( Mag a , std::size_t pad , int base ,
                      std::string & out , Mag * b = nullptr ,
                      std::size_t padb = 0 );

 /// out += the figures of x, at least pad, by halves from kConvert digits,
 /// pw[ i ] b^{zs[ i ]} = P^{2^i} with P^1 holding k figures, rc[ i ] the
 /// reciprocal of pw[ i ] kept by the divisions [see divNewton()]; a
 /// remainder x < P^{2^j}, j given, of up to kConvertF digits by fractions
 static void toFigures( const Mag & x , std::size_t pad , int base ,
                        const std::vector< Mag > & pw ,
                        const std::vector< std::size_t > & zs , int k ,
                        std::vector< Mag > & rc , std::vector< Mag > & rf ,
                        std::string & out , std::size_t j );

 /// out += the 2^j k figures of x < P^{2^j}, by the fraction x / P^{2^j},
 /// made by the reciprocal of P^{2^j} kept in rf[ j ] [see fracTree()]
 static void fracFigures( const Mag & x , std::size_t j , int base , int k ,
                          Limb P , const std::vector< Mag > & pw ,
                          const std::vector< std::size_t > & zs ,
                          std::vector< Mag > & rf , std::string & out );

 /// the C k figures of the fraction f, of C = 2^j chunks, to out: the
 /// high half from the top digits of f, the low half from the fraction of
 /// f P^{C/2}, both rounded up, by halves down to kFracLeaf chunks, which
 /// are the integer parts of f P, f P^2 and so on
 static void fracTree( Mag & f , std::size_t j , int base , int k , Limb P ,
                       const std::vector< Mag > & pw ,
                       const std::vector< std::size_t > & zs , bool up ,
                       char * out );
 static void fracLeaf( Mag & f , std::size_t c , Limb P , int k , int base ,
                       bool up , char * out );

 /// the digits [ s , e ) of a b, na > nb, short of the exact ones by 2 at
 /// most, from the products a_i b_j with s - 5 <= i + j < e + 3 only,
 /// in r from the place s on; false, r unchanged, if it does not apply
 static bool mulBand( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t s ,
                      std::size_t e );

 /// the sum of c[ j ] P^j for j < n, by halves from kConvert digits,
 /// pw[ i ] = P^{2^i}
 static Mag fromChunks( const Limb * c , std::size_t n ,
                        const std::vector< Mag > & pw , Limb P );

 /// the greatest common divisor of two numbers below 2^64, binary
 static std::uint64_t gcdWord( std::uint64_t a , std::uint64_t b );

 /// the greatest common divisor of a, n > 0 digits, not 0, and d != 0
 static Limb gcdLimb( const Limb * a , std::size_t n , Limb d );

#if ADS_BIGINT_GMP
 /// z as the read-only view of the n digits d, a magnitude
 static mpz_srcptr gmpView( mpz_t z , const Limb * d , std::size_t n );

 /// the digits of the magnitude of z
 static Mag gmpDigits( mpz_srcptr z );

 /// true if the quotient of u digits by v >= 2 digits goes to GMP, with
 /// its remainder if rem
 static bool gmpDivides( std::size_t u , std::size_t v , bool rem );
#endif

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
