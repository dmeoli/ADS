/*--------------------------------------------------------------------------*/
/*-------------------------- File NumberTheory.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic of section 17.2 and of exercises 17.1-17.3, on unsigned
 * integers of 64 bits:
 *
 * - `gcd()`, of Euclid, \f$ \gcd( s , m ) = \gcd( m , s \bmod m ) \f$,
 *   \f$ O( \log m ) \f$ steps; `extendedEuclid()` also gives \f$ p \f$ and
 *   \f$ q \f$ with \f$ s p + m q = \gcd( s , m ) \f$: from
 *   \f$ ( d , p' , q' ) \f$ of \f$ ( m , s \bmod m ) \f$,
 *   \f$ ( d , q' , p' - \lfloor s / m \rfloor q' ) \f$ (the solution of
 *   exercise 17.2 in the text has the two swapped);
 * - `modInverse()`, the \f$ p \f$ with \f$ p s \bmod m = 1 \f$ when
 *   \f$ \gcd( s , m ) = 1 \f$ (exercise 17.3);
 * - `mulMod()`, \f$ a b \bmod m \f$ exact for every modulus of 64 bits, the
 *   modular product of all of ADS: the product of 64 bits for
 *   \f$ m \le 2^{32} \f$; `mulMod61()` for the Mersenne prime
 *   \f$ p = 2^{61} - 1 \f$ of the universal hashing [see
 *   `UniversalHashing.h`], four products of 32 bits folded by
 *   \f$ 2^{61} \equiv 1 \pmod{p} \f$, faster than a division of 128 bits;
 *   the product of 128 bits where the compiler has one, and the doubling of
 *   Ahmes, \f$ O( \log b ) \f$ additions, where it has not (MSVC);
 * - `powMod()`, \f$ x^v \bmod m \f$ by repeated squaring,
 *   \f$ O( \log v ) \f$ multiplications (exercise 17.1),
 *   \f$ \lfloor \log_2 v \rfloor + 1 \f$ squarings and as many products, each
 *   one of Montgomery when \f$ m \f$ is odd (see `Montgomery64` below): the
 *   products and the squarings are two chains that do not depend on each
 *   other (from the lowest bit up), and the bit selects the product with no
 *   branch. Below \f$ 2^{62} \f$ the forms are kept in \f$ [ 0 , 2 m ) \f$
 *   (the lazy reduction): for \f$ a , b < 2 m \f$,
 *   \f$ T = a b < 4 m^2 < m R \f$, so that
 *   \f$ ( T - q m ) / R + m \in ( 0 , 2 m ) \f$ and the comparison that ends
 *   each product is spared; below \f$ 2^{32} \f$ the forms are of
 *   \f$ R = 2^{32} \f$, the products of 64 bits, the form of \f$ x \f$ by one
 *   division of 64 by 32 bits, the trailing zeros of \f$ v \f$ by squarings
 *   and the power from the lowest bit 1 of the odd part, so that the form of
 *   1 (a division more) is never needed;
 * - `binaryGcd()`, beyond the book, the binary gcd of Stein (Algorithm
 *   4.5.2B of Knuth): with \f$ k \f$ the trailing zeros of \f$ a \lor b \f$,
 *   \f$ \gcd( a , b ) = 2^k \gcd( a' , b' ) \f$ for the odd parts, and
 *   \f$ \gcd( a , b ) = \gcd( \min( a , b ) , | a - b | ) \f$ with the
 *   trailing zeros of the even difference shifted out, by one instruction;
 *   each step halves at least \f$ \max( a , b ) \f$, hence at most
 *   \f$ \log_2 a + \log_2 b + 2 \f$ steps and no division, the minimum and
 *   the difference by conditional moves, with no branch to mispredict; on
 *   x86-64 the trailing zeros are those of \f$ b - a \f$ (equal to those of
 *   \f$ | a - b | \f$), so that the count starts with the difference and the
 *   step is a subtraction, a count and a shift, the two conditional moves
 *   beside them;
 * - `Montgomery64`, beyond the book, the arithmetic modulo an odd \f$ n \f$
 *   of 64 bits in the form \f$ \bar{x} = x R \bmod n \f$,
 *   \f$ R = 2^{64} \f$: the product is
 *   \f$ \mathrm{REDC}( \bar{a} \bar{b} ) = \bar{a} \bar{b} R^{-1} \bmod n \f$,
 *   where for \f$ T = h R + l < n R \f$ and
 *   \f$ q = l n^{-1} \bmod R \f$ the low half of \f$ q n \f$ equals
 *   \f$ l \f$, so that
 *   \f$ ( T - q n ) / R = h - \lfloor q n / R \rfloor \in ( - n , n ) \f$
 *   and one addition of \f$ n \f$ when negative ends it: two products of 128
 *   bits and one of 64 instead of a division of 128 bits, exact for every
 *   odd \f$ n \f$; \f$ n^{-1} \bmod 2^{64} \f$ comes from
 *   \f$ x_0 = 3 n \oplus 2 \f$, right on 5 bits, by 4 steps of Newton
 *   \f$ x_{i+1} = x_i ( 2 - n x_i ) \f$, each doubling the bits that are
 *   right, and \f$ R \bmod n \f$ and \f$ R^2 \bmod n \f$ by one division of
 *   128 by 64 bits each;
 * - `isPrimeTrial()`, the divisions up to \f$ \sqrt{n} \f$,
 *   \f$ O( 2^{d/2} ) \f$ for \f$ n \f$ of \f$ d \f$ bits;
 * - `witness()`, \f$ b \f$ is a witness that \f$ n \f$ is composite if
 *   \f$ \gcd( n , b ) \neq 1 \f$ or, with \f$ n - 1 = 2^v m \f$ and
 *   \f$ m \f$ odd, neither \f$ b^m \bmod n = 1 \f$ nor
 *   \f$ b^{2^i m} \bmod n = n - 1 \f$ for some \f$ 0 \le i < v \f$;
 *   `millerRabin()` tries \f$ K \f$ random bases in \f$ [ 2 , n - 2 ] \f$,
 *   at least \f$ 3 / 4 \f$ of which are witnesses for a composite \f$ n \f$,
 *   and errs with probability at most \f$ ( 1 / 4 )^K \f$, only by saying
 *   prime;
 * - `isPrime()`, beyond the book, exact on 64 bits: first the divisions by
 *   the 46 odd primes up to 211, by one product each with no branch, all at
 *   once on vectors (for the odd \f$ p \f$,
 *   \f$ p \mid n \f$ if and only if
 *   \f$ n p^{-1} \bmod 2^{64} \le \lfloor ( 2^{64} - 1 ) / p \rfloor \f$,
 *   since
 *   \f$ x \mapsto x p^{-1} \f$ is a bijection that sends the multiples of
 *   \f$ p \f$ onto \f$ [ 0 , \lfloor ( 2^{64} - 1 ) / p \rfloor ] \f$), which
 *   leave about \f$ \prod_{3 \le p \le 211} ( 1 - 1 / p ) \approx 0.21 \f$ of
 *   the odd numbers; then the test of Miller and Rabin on the fewest bases
 *   known to have no strong pseudoprime below \f$ n \f$: below
 *   \f$ 2^{32} \f$ one base read in a table of 1024 at the hash
 *   \f$ \lfloor ( n \cdot 2654435761 \bmod 2^{32} ) / 2^{22} \rfloor \f$ (the
 *   hashed bases of Forisek and Jancina), the least base, at most 67, of
 *   which no odd composite of the bucket with no prime factor up to 211 is a
 *   strong pseudoprime (computed for ADS by the exhaustive search over the
 *   \f$ 2^{31} \f$ odd numbers), in an arithmetic of Montgomery of 32 bits
 *   that needs only \f$ n^{-1} \bmod 2^{32} \f$ (the form of \f$ b \f$ by
 *   one division, the power from the lowest bit of \f$ d \f$, the
 *   comparisons after one reduction), up to the 7 bases of Sinclair for
 *   every \f$ n \f$ of 64 bits. The \f$ k \f$ bases run in lockstep,
 *   \f$ k \f$ chains of products that the processor overlaps, so that 2 cost
 *   about as much as 1; with 3 or more, the exponent goes by windows of 4
 *   bits with a table of \f$ b^0 , \dots , b^{15} \f$,
 *   \f$ \lceil \log_2 d \rceil + \lceil \log_2 d / 4 \rceil + 14 \f$
 *   products per base instead of up to \f$ 2 \lceil \log_2 d \rceil \f$. The
 *   arithmetic is that of Montgomery of 32 bits below \f$ 2^{32} \f$
 *   (\f$ R = 2^{32} \f$, its constants by divisions of 32 bits), the lazy
 *   one below \f$ 2^{62} \f$, and `Montgomery64` above;
 * - `factorize()`, beyond the book, the prime factors of a word: the
 *   divisions by the primes below 256, then, on a composite cofactor
 *   \f$ m \f$ (`isPrime()` decides), the rho of Pollard with the cycle
 *   detection of Brent, in the Montgomery arithmetic of \f$ m \f$: the
 *   sequence \f$ x_{i+1} = x_i^2 + c \bmod m \f$ falls into a cycle modulo
 *   the least prime \f$ p \f$ of \f$ m \f$ after
 *   \f$ O( \sqrt{p} ) = O( m^{1/4} ) \f$ steps in expectation (the birthday
 *   paradox, the map taken as random), which
 *   \f$ \gcd( | x_i - x_j | , m ) \f$ finds; the differences are multiplied
 *   together, 128 at a time, before each gcd, and the last batch is gone
 *   through again one by one if the gcd is \f$ m \f$; with another \f$ c \f$
 *   if the factor is \f$ m \f$ itself. \f$ O( n^{1/4} ) \f$ products expected
 *   for \f$ n \f$ of 64 bits;
 * - `sieve()`, the sieve of Eratosthenes on a SET of the integers from 2 to
 *   \f$ n \f$ [see `BoolSet.h`], beyond the book: the least item \f$ p \f$
 *   left in the set is a prime, and its multiples are removed; since the
 *   multiples of the primes below \f$ p \f$ are already gone, the removal can
 *   start at \f$ p^2 \f$, and the sieve stops at \f$ p > \sqrt{n} \f$, in
 *   \f$ O( n \log \log n ) \f$.
 *
 * The contracts [see `Contract.h`]: `extendedEuclid()` needs
 * \f$ s , m \ge 0 \f$ and gives \f$ d = p s + q m \f$; `modInverse()` needs
 * \f$ m \ge 1 \f$ and \f$ \gcd( s , m ) = 1 \f$, and its result \f$ r \f$
 * satisfies \f$ s r \equiv 1 \pmod{m} \f$; the products and the powers
 * modulo \f$ m \f$ need \f$ m \ge 1 \f$, and `mulMod61()` factors below
 * \f$ 2^{61} - 1 \f$; `witness()` needs an odd \f$ n \ge 3 \f$ and a base
 * \f$ 1 \le b < n \f$, and `millerRabin()` \f$ K \ge 0 \f$; `sieve()` gives
 * the primes up to \f$ n \f$, increasing, as `isPrimeTrial()` finds them (an
 * audit); `factorize()` needs \f$ n \ge 1 \f$ and gives primes whose product
 * is \f$ n \f$ (an audit). The functions are defined in `NumberTheory.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.2 and
 *      exercises 17.1-17.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see P. L. Montgomery, Modular multiplication without trial division,
 *      Mathematics of Computation 44(170), 519-521, 1985,
 *      doi:10.1090/S0025-5718-1985-0777282-X
 * \see J. Stein, Computational problems associated with Racah algebra,
 *      Journal of Computational Physics 1(3), 397-405, 1967,
 *      doi:10.1016/0021-9991(67)90047-2
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, section 4.5.2 (Algorithm B)
 * \see G. L. Miller, Riemann's hypothesis and tests for primality, Journal
 *      of Computer and System Sciences 13(3), 300-317, 1976,
 *      doi:10.1016/S0022-0000(76)80043-8
 * \see M. O. Rabin, Probabilistic algorithm for testing primality, Journal
 *      of Number Theory 12(1), 128-138, 1980,
 *      doi:10.1016/0022-314X(80)90084-0
 * \see T. Granlund, P. L. Montgomery, Division by invariant integers using
 *      multiplication, Proceedings of the ACM SIGPLAN 1994 Conference on
 *      Programming Language Design and Implementation (PLDI), 61-72, 1994,
 *      doi:10.1145/178243.178249
 * \see G. Jaeschke, On strong pseudoprimes to several bases, Mathematics
 *      of Computation 61(204), 915-926, 1993,
 *      doi:10.1090/S0025-5718-1993-1192971-8
 * \see M. Forisek, J. Jancina, Fast primality testing for integers that fit
 *      into a machine word, SOFSEM 2015 Student Research Forum, CEUR-WS
 *      1326, 20-30, 2015, https://ceur-ws.org/Vol-1326/020-Forisek.pdf
 * \see J. Sinclair and others, the deterministic sets of bases for the
 *      test of Miller and Rabin, https://miller-rabin.appspot.com
 * \see J. M. Pollard, A Monte Carlo method for factorization, BIT 15(3),
 *      331-334, 1975, doi:10.1007/BF01933667
 * \see R. P. Brent, An improved Monte Carlo factorization algorithm, BIT
 *      Numerical Mathematics 20(2), 176-184, 1980, doi:10.1007/BF01933190
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

#ifndef __ADS_NumberTheory
 #define __ADS_NumberTheory
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/set/BoolSet.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using U64 = std::uint64_t;   ///< the unsigned integers of 64 bits

/// the greatest common divisor \f$ \gcd( s , m ) \f$ by the algorithm of
/// Euclid, \f$ O( \log m ) \f$ steps
///
/// In pseudocode (the solution of exercise 17.2), the invariant
/// \f$ \gcd( s , m ) = \gcd( m , s \bmod m ) \f$ kept at each step:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gcd}( s , m ) \cr
/// 2 & \quad \textbf{while}\ m \ne 0\ \textbf{do}\ ( s , m ) \gets ( m , s
///     \bmod m ) \cr
/// 3 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
inline U64 gcd( U64 s , U64 m );

/// the greatest common divisor of \f$ a \f$ and \f$ b \f$ by the binary
/// algorithm, with no division; \f$ \gcd( a , 0 ) = a \f$
///
/// In pseudocode (the binary gcd of Stein, Algorithm 4.5.2B of Knuth, beyond
/// the book), \f$ \mathrm{tz}( x ) \f$ the trailing zeros of \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binaryGcd}( a , b ) \cr
/// 2 & \quad \textbf{if}\ a = 0\ \textbf{then return}\ b;\ \textbf{if}\ b =
///     0\ \textbf{then return}\ a \cr
/// 3 & \quad k \gets \mathrm{tz}( a \lor b );\ a \gets a / 2^{\mathrm{tz}( a
///     )} \cr
/// 4 & \quad \textbf{repeat}\quad \triangleright\ a \text{ odd} \cr
/// 5 & \qquad b \gets b / 2^{\mathrm{tz}( b )} \cr
/// 6 & \qquad ( a , b ) \gets ( \min( a , b ) , | a - b | ) \cr
/// 7 & \quad \textbf{until}\ b = 0 \cr
/// 8 & \quad \textbf{return}\ 2^k a
/// \end{array}}
/// \f]
[[nodiscard]] constexpr U64 binaryGcd( U64 a , U64 b );

/// the triple \f$ ( d , p , q ) \f$ with
/// \f$ s p + m q = d = \gcd( s , m ) \f$ (the euclideEsteso() of the book)
///
/// In pseudocode (exercise 17.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{extendedEuclid}( s , m ) \cr
/// 2 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ ( s , 1 , 0 ) \cr
/// 3 & \quad ( d , p' , q' ) \gets \mathrm{extendedEuclid}( m , s \bmod m )
///     \cr
/// 4 & \quad \textbf{return}\ ( d , q' , p' - \lfloor s / m \rfloor q' )
/// \end{array}}
/// \f]
/// \pre \f$ s \ge 0 \f$ and \f$ m \ge 0 \f$
/// \post \f$ d = p s + q m \f$, checked modulo \f$ 2^{64} \f$
inline std::tuple< long long , long long , long long > extendedEuclid(
                                                   long long s , long long m );

/// the inverse \f$ p \in [ 0 , m ) \f$ of \f$ s \f$, with
/// \f$ p s \bmod m = 1 \f$; a violation of the precondition if
/// \f$ \gcd( s , m ) \ne 1 \f$
///
/// In pseudocode (exercise 17.3), from \f$ s p + m q = 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{modInverse}( s , m ) \cr
/// 2 & \quad ( d , p , q ) \gets \mathrm{extendedEuclid}( s , m ) \cr
/// 3 & \quad \textbf{return}\ p \bmod m \text{, in } [ 0 , m )
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$, \f$ s \ge 0 \f$ and \f$ \gcd( s , m ) = 1 \f$
/// \post \f$ s r \equiv 1 \pmod{m} \f$
inline long long modInverse( long long s , long long m );

/// the Mersenne prime \f$ 2^{61} - 1 \f$
inline constexpr U64 kMersenne61 = ( U64( 1 ) << 61 ) - 1;

/// the product \f$ a b \bmod p \f$, \f$ p = 2^{61} - 1 \f$, by four products
/// of 32 bits and the folding \f$ 2^{61} \equiv 1 \pmod{p} \f$
///
/// In pseudocode (beyond the book), \f$ a = a_1 2^{32} + a_0 \f$ and
/// \f$ b = b_1 2^{32} + b_0 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mulMod61}( a , b ) \cr
/// 2 & \quad h \gets a_1 b_1;\ \mathit{mid} \gets a_1 b_0 + a_0 b_1;\ l \gets
///     a_0 b_0 \cr
/// 3 & \quad s \gets 8 h + \lfloor \mathit{mid} / 2^{29} \rfloor + (
///     \mathit{mid} \bmod 2^{29} ) 2^{32} + ( l \bmod 2^{61} ) + \lfloor l /
///     2^{61} \rfloor \cr
/// 4 & \quad s \gets ( s \bmod 2^{61} ) + \lfloor s / 2^{61} \rfloor \cr
/// 5 & \quad \textbf{if}\ s \ge p\ \textbf{then}\ s \gets s - p \cr
/// 6 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// where \f$ 2^{64} \equiv 8 \f$ and \f$ 2^{61} \equiv 1 \f$ modulo
/// \f$ p \f$.
/// \pre \f$ a < 2^{61} - 1 \f$ and \f$ b < 2^{61} - 1 \f$
/// \post the result is below \f$ 2^{61} - 1 \f$
[[nodiscard]] constexpr U64 mulMod61( U64 a , U64 b );

/// the product \f$ a b \bmod m \f$, exact for any \f$ a \f$, \f$ b \f$ and
/// \f$ m \f$: by the product of 64 bits for \f$ m \le 2^{32} \f$, by
/// `mulMod61()` for \f$ m = 2^{61} - 1 \f$, by the product of 128 bits or
/// else by doubling and adding
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mulMod}( a , b , m ) \cr
/// 2 & \quad a \gets a \bmod m;\ b \gets b \bmod m \cr
/// 3 & \quad \textbf{if}\ m \le 2^{32}\ \textbf{then return}\ a b \bmod m \cr
/// 4 & \quad \textbf{if}\ m = 2^{61} - 1\ \textbf{then return}
///     \ \mathrm{mulMod61}( a , b ) \cr
/// 5 & \quad \textbf{if}\ \text{there is a product of 128 bits}
///     \ \textbf{then return}\ a b \bmod m \cr
/// 6 & \quad r \gets 0 \quad \triangleright\ \text{the doubling of Ahmes} \cr
/// 7 & \quad \textbf{while}\ b > 0\ \textbf{do} \cr
/// 8 & \qquad \textbf{if}\ b \text{ is odd}\ \textbf{then}\ r \gets ( r + a )
///     \bmod m \cr
/// 9 & \qquad a \gets 2 a \bmod m;\ b \gets \lfloor b / 2 \rfloor \cr
/// 10 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$
/// \post the result is below \f$ m \f$
[[nodiscard]] constexpr U64 mulMod( U64 a , U64 b , U64 m );

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS Montgomery64 ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the arithmetic modulo an odd \f$ n \f$ of 64 bits in the form of
/// Montgomery, \f$ x R \bmod n \f$ with \f$ R = 2^{64} \f$; the numbers in
/// the form are below \f$ n \f$
class Montgomery64 {
 public:
 /// the arithmetic modulo \f$ n \f$, with one division of 128 bits for
 /// \f$ R^2 \bmod n \f$
 ///
 /// In pseudocode (Montgomery, beyond the book), the inverse by Newton:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{Montgomery64}( n ) \cr
 /// 2 & \quad x \gets 3 n \oplus 2\quad \triangleright\ n^{-1} \bmod 2^5 \cr
 /// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ 4\ \textbf{do}\ x \gets
 ///     x ( 2 - n x ) \bmod 2^{64} \cr
 /// 4 & \quad \mathit{inv} \gets x;\ \mathit{one} \gets R \bmod n;
 ///     \ \mathit{r2} \gets R^2 \bmod n
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \f$ is odd
 /// \post \f$ n \cdot \mathit{inv} \equiv 1 \pmod{2^{64}} \f$
 explicit constexpr Montgomery64( U64 n );

 /// the modulus \f$ n \f$
 [[nodiscard]] constexpr U64 modulus( void ) const { return( f_n ); }

 /// the form \f$ x R \bmod n \f$ of \f$ x \f$, as
 /// \f$ \mathrm{mul}( x \bmod n , R^2 \bmod n ) \f$
 [[nodiscard]] constexpr U64 to( U64 x ) const;

 /// the number \f$ x R^{-1} \bmod n \f$ whose form is \f$ x \f$, as
 /// \f$ \mathrm{REDC}( x ) \f$
 /// \pre \f$ x < n \f$
 [[nodiscard]] constexpr U64 from( U64 x ) const { return( reduce( 0 , x ) ); }

 /// the form \f$ R \bmod n \f$ of 1
 [[nodiscard]] constexpr U64 one( void ) const { return( f_one ); }

 /// the form \f$ a b R^{-1} \bmod n \f$ of the product of the numbers of the
 /// forms \f$ a \f$ and \f$ b \f$
 ///
 /// In pseudocode (Montgomery, beyond the book), \f$ \mathrm{REDC} \f$ of the
 /// product of 128 bits:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{mul}( a , b ) \cr
 /// 2 & \quad ( h , l ) \gets \text{the high and the low half of } a b \cr
 /// 3 & \quad \textbf{return}\ \mathrm{reduce}( h , l )
 /// \end{array}}
 /// \f]
 /// \pre \f$ a < n \f$ and \f$ b < n \f$
 [[nodiscard]] constexpr U64 mul( U64 a , U64 b ) const;

 /// the form of \f$ x^v \f$, \f$ x \f$ a form
 ///
 /// In pseudocode (exercise 17.1 in the forms), from the lowest bit of
 /// \f$ v \f$, the bit selecting the product with no branch:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pow}( x , v ) \cr
 /// 2 & \quad r \gets \mathrm{one}() \cr
 /// 3 & \quad \textbf{while}\ v > 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ v \text{ is odd}\ \textbf{then}\ r \gets
 ///     \mathrm{mul}( r , x ) \cr
 /// 5 & \qquad x \gets \mathrm{mul}( x , x );\ v \gets \lfloor v / 2 \rfloor
 ///     \cr
 /// 6 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < n \f$
 [[nodiscard]] constexpr U64 pow( U64 x , U64 v ) const;

 private:
 /// the reduction \f$ \mathrm{REDC}( T ) = T R^{-1} \bmod n \f$ of
 /// \f$ T = \mathit{hi} \cdot 2^{64} + \mathit{lo} \f$
 ///
 /// In pseudocode (Montgomery, beyond the book), \f$ T - q n \f$ a multiple
 /// of \f$ R \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{reduce}( \mathit{hi} , \mathit{lo} ) \cr
 /// 2 & \quad q \gets \mathit{lo} \cdot \mathit{inv} \bmod 2^{64} \cr
 /// 3 & \quad t \gets \mathit{hi} - \lfloor q n / 2^{64} \rfloor \quad
 ///     \triangleright\ \text{in } ( -n , n ) \cr
 /// 4 & \quad \textbf{if}\ t < 0\ \textbf{then return}\ t + n \cr
 /// 5 & \quad \textbf{return}\ t
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{hi} < n \f$
 [[nodiscard]] constexpr U64 reduce( U64 hi , U64 lo ) const;

 U64 f_n;       ///< the modulus
 U64 f_inv;     ///< \f$ n^{-1} \bmod 2^{64} \f$
 U64 f_one;     ///< \f$ R \bmod n \f$
 U64 f_r2;      ///< \f$ R^2 \bmod n \f$
 };  // end( class Montgomery64 )

/// the power \f$ x^v \bmod m \f$ by repeated squaring, the products of
/// Montgomery when \f$ m \f$ is odd
///
/// In pseudocode (exercise 17.1), from the lowest bit of \f$ v \f$, each
/// product of Montgomery when \f$ m \f$ is odd [see `Montgomery64`]:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{powMod}( x , v , m ) \cr
/// 2 & \quad r \gets 1 \bmod m;\ x \gets x \bmod m \cr
/// 3 & \quad \textbf{while}\ v > 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ v \text{ is odd}\ \textbf{then}\ r \gets
///     \mathrm{mulMod}( r , x , m ) \cr
/// 5 & \qquad x \gets \mathrm{mulMod}( x , x , m );\ v \gets \lfloor v / 2
///     \rfloor \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$
inline U64 powMod( U64 x , U64 v , U64 m );

/// the primality of \f$ n \f$ by the divisions by
/// \f$ 2 , \ldots , \lfloor \sqrt{n} \rfloor \f$ (the primo() of the book)
///
/// In pseudocode (section 17.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isPrimeTrial}( n ) \cr
/// 2 & \quad \textbf{if}\ n < 2\ \textbf{then return false} \cr
/// 3 & \quad \textbf{for}\ j \gets 2\ \textbf{to}\ \lfloor \sqrt{n} \rfloor
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ n \bmod j = 0\ \textbf{then return false} \cr
/// 5 & \quad \textbf{return true}
/// \end{array}}
/// \f]
inline bool isPrimeTrial( U64 n );

/// true if \f$ b \f$ is a witness that the odd \f$ n > 2 \f$ is composite
///
/// In pseudocode (section 17.2), \f$ n - 1 = 2^v m \f$ with \f$ m \f$ odd:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{witness}( n , b ) \cr
/// 2 & \quad \textbf{if}\ \mathrm{gcd}( n , b ) \ne 1\ \textbf{then return
///     true} \cr
/// 3 & \quad x \gets \mathrm{powMod}( b , m , n ) \cr
/// 4 & \quad \textbf{if}\ x = 1\ \textbf{or}\ x = n - 1\ \textbf{then return
///     false} \cr
/// 5 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ v - 1\ \textbf{do} \cr
/// 6 & \qquad x \gets \mathrm{mulMod}( x , x , n ) \cr
/// 7 & \qquad \textbf{if}\ x = n - 1\ \textbf{then return false} \cr
/// 8 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 3 \f$ odd and \f$ 1 \le b < n \f$
inline bool witness( U64 n , U64 b );

/// the primality of \f$ n \f$ by the test of Miller and Rabin on \f$ K \f$
/// random bases, wrong with probability at most \f$ ( 1 / 4 )^K \f$, and
/// only on a composite \f$ n \f$ (the primo() of the book)
///
/// In pseudocode (section 17.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{millerRabin}( n , K ) \cr
/// 2 & \quad \textbf{if}\ n < 4\ \textbf{then return}\ n = 2\ \textbf{or}\ n
///     = 3 \cr
/// 3 & \quad \textbf{if}\ n \text{ is even}\ \textbf{then return false} \cr
/// 4 & \quad \textbf{for}\ j \gets 1\ \textbf{to}\ K\ \textbf{do} \cr
/// 5 & \qquad b \gets \text{a random integer in } [ 2 , n - 2 ] \cr
/// 6 & \qquad \textbf{if}\ \mathrm{witness}( n , b )\ \textbf{then return
///     false} \cr
/// 7 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ K \ge 0 \f$
template< class R >
bool millerRabin( U64 n , int K , R & rng );

/// the primality of \f$ n \f$, exact for every \f$ n \f$ of 64 bits
///
/// In pseudocode (the deterministic bases of Jaeschke, Forisek and Jancina,
/// and Sinclair, beyond the book), \f$ n - 1 = 2^s d \f$ with \f$ d \f$ odd:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isPrime}( n ) \cr
/// 2 & \quad \textbf{if}\ n < 256\ \textbf{then return}\ \text{the bit of }
///     n \text{ in a table} \cr
/// 3 & \quad \textbf{if}\ n \text{ is even or a multiple of an odd prime up }
///     \text{to } 211\ \textbf{then return false} \cr
/// 4 & \quad \textbf{if}\ n < 223^2\ \textbf{then return true} \cr
/// 5 & \quad \textbf{if}\ n < 2^{32}\ \textbf{then} \cr
/// 6 & \qquad b \gets \text{the base of the bucket of } n \text{ in the }
///     \text{hashed table} \cr
/// 7 & \qquad \textbf{return}\ n \text{ is a strong probable prime to }
///     \text{the base } b \cr
/// 8 & \quad B \gets \text{the fewest bases with no strong pseudoprime }
///     \text{below } n \cr
/// 9 & \quad \textbf{return}\ n \text{ is a strong probable prime to each }
///     \text{base of } B
/// \end{array}}
/// \f]
inline bool isPrime( U64 n );

/// the primes up to \f$ n \f$, increasing, by the sieve of Eratosthenes
///
/// In pseudocode (beyond the book), on a SET \f$ S \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sieve}( n ) \cr
/// 2 & \quad S \gets \{ 2 , \ldots , n \};\ P \gets \text{the empty list} \cr
/// 3 & \quad \textbf{for}\ p \gets 2\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ p \in S\ \textbf{then} \cr
/// 5 & \qquad\quad \text{append } p \text{ to } P \cr
/// 6 & \qquad\quad \textbf{for}\ m \gets p^2 , p^2 + p , \ldots \le n
///     \ \textbf{do}\ S \gets S \setminus \{ m \} \cr
/// 7 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \post the primes up to \f$ n \f$, as `isPrimeTrial()` finds them (audit)
inline std::vector< U64 > sieve( U64 n );

/// the prime factors of \f$ n \f$, with their multiplicity, increasing, by
/// the divisions by the primes below 256 and the rho of Pollard and Brent
///
/// In pseudocode (Pollard and Brent, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{factorize}( n ) \cr
/// 2 & \quad F \gets \text{the factors of } n \text{ below } 256 \text{ by }
///     \text{division};\ n \gets \text{the cofactor} \cr
/// 3 & \quad T \gets ( n ) \text{ if } n > 1 \cr
/// 4 & \quad \textbf{while}\ T \ne \emptyset\ \textbf{do} \cr
/// 5 & \qquad m \gets \mathrm{pop}( T ) \cr
/// 6 & \qquad \textbf{if}\ m < 256^2\ \textbf{or}\ \mathrm{isPrime}( m )
///     \ \textbf{then}\ \text{append } m \text{ to } F \cr
/// 7 & \qquad \textbf{else}\ d \gets \mathrm{rho}( m );\ \mathrm{push}( T ,
///     d );\ \mathrm{push}( T , m / d ) \cr
/// 8 & \quad \textbf{return}\ F \text{ sorted} \cr
/// 9 & \textbf{function}\ \mathrm{rho}( m )\quad \triangleright\ c \gets 1 ,
///     2 , \ldots \text{ until } 1 < g < m \cr
/// 10 & \quad y \gets 2;\ q \gets 1;\ g \gets 1;\ r \gets 1 \cr
/// 11 & \quad \textbf{while}\ g = 1\ \textbf{do} \cr
/// 12 & \qquad x \gets y;\ \text{apply } y \gets y^2 + c \bmod m \text{ }
///     r \text{ times} \cr
/// 13 & \qquad \textbf{for}\ k \gets 0 , 128 , \ldots < r\ \textbf{while}\ g
///     = 1\ \textbf{do} \cr
/// 14 & \qquad\quad \text{at most 128 times: } y \gets y^2 + c;\ q \gets q
///     | x - y | \bmod m \cr
/// 15 & \qquad\quad g \gets \mathrm{gcd}( q , m ) \cr
/// 16 & \qquad r \gets 2 r \cr
/// 17 & \quad \textbf{if}\ g = m\ \textbf{then}\ g \gets \text{the first }
///     \mathrm{gcd}( | x - y | , m ) \ne 1 \text{ of the last batch} \cr
/// 18 & \quad \textbf{return}\ g
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$
/// \post their product is \f$ n \f$ and each one is prime (audit)
inline std::vector< U64 > factorize( U64 n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/NumberTheory.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* NumberTheory.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File NumberTheory.h -------------------------*/
/*--------------------------------------------------------------------------*/
