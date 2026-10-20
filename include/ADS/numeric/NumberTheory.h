/*--------------------------------------------------------------------------*/
/*-------------------------- File NumberTheory.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic of section 17.2 and of exercises 17.1-17.3, on unsigned
 * integers of 64 bits:
 *
 * - gcd(), of Euclid, \f$ \gcd( s , m ) = \gcd( m , s \bmod m ) \f$,
 *   O( log m ) steps; extendedEuclid() also gives p and q with
 *   \f$ s p + m q = \gcd( s , m ) \f$: from ( d , p' , q' ) of
 *   ( m , s mod m ), ( d , q' , p' - \lfloor s / m \rfloor q' ) (the
 *   solution of exercise 17.2 in the text has the two swapped);
 * - modInverse(), the p with \f$ p s \bmod m = 1 \f$ when
 *   \f$ \gcd( s , m ) = 1 \f$ (exercise 17.3);
 * - mulMod(), \f$ a b \bmod m \f$ exact for every modulus of 64 bits, the
 *   modular product of all of ADS: the product of 64 bits for
 *   \f$ m \le 2^{32} \f$; mulMod61() for the Mersenne prime
 *   \f$ p = 2^{61} - 1 \f$ of the universal hashing [see
 *   UniversalHashing.h], four products of 32 bits folded by
 *   \f$ 2^{61} \equiv 1 \pmod{p} \f$, faster than a division of 128 bits;
 *   the product of 128 bits where the compiler has one, and the doubling
 *   of Ahmes, O( log b ) additions, where it has not (MSVC);
 * - powMod(), \f$ x^v \bmod m \f$ by repeated squaring, O( log v )
 *   multiplications (exercise 17.1), \f$ \lfloor \log_2 v \rfloor + 1 \f$
 *   squarings and as many products, each one of Montgomery when m is odd
 *   (see Montgomery64 below): the products and the squarings are two
 *   chains that do not depend on each other (from the lowest bit up), and
 *   the bit selects the product with no branch. Below \f$ 2^{62} \f$ the
 *   forms are kept in \f$ [ 0 , 2 m ) \f$ (the lazy reduction): for
 *   \f$ a , b < 2 m \f$, \f$ T = a b < 4 m^2 < m R \f$, so that
 *   \f$ ( T - q m ) / R + m \in ( 0 , 2 m ) \f$ and the comparison that
 *   ends each product is spared; below \f$ 2^{32} \f$ the forms are of
 *   \f$ R = 2^{32} \f$, the products of 64 bits, the form of x by one
 *   division of 64 by 32 bits, the trailing zeros of v by squarings and
 *   the power from the lowest bit 1 of the odd part, so that the form of
 *   1 (a division more) is never needed;
 * - binaryGcd(), beyond the book, the binary gcd of Stein (Algorithm 4.5.2B
 *   of Knuth): with \f$ k \f$ the trailing zeros of \f$ a \lor b \f$,
 *   \f$ \gcd( a , b ) = 2^k \gcd( a' , b' ) \f$ for the odd parts, and
 *   \f$ \gcd( a , b ) = \gcd( \min( a , b ) , | a - b | ) \f$ with the
 *   trailing zeros of the even difference shifted out, by one instruction;
 *   each step halves at least \f$ \max( a , b ) \f$, hence at most
 *   \f$ \log_2 a + \log_2 b + 2 \f$ steps and no division, the minimum and
 *   the difference by conditional moves, with no branch to mispredict;
 * - Montgomery64, beyond the book, the arithmetic modulo an odd n of 64
 *   bits in the form \f$ \bar{x} = x R \bmod n \f$, \f$ R = 2^{64} \f$:
 *   the product is \f$ \mathrm{REDC}( \bar{a} \bar{b} ) = \bar{a} \bar{b}
 *   R^{-1} \bmod n \f$, where for \f$ T = h R + l < n R \f$ and \f$ q = l
 *   n^{-1} \bmod R \f$ the low half of \f$ q n \f$ equals l, so that
 *   \f$ ( T - q n ) / R = h - \lfloor q n / R \rfloor \in ( - n , n ) \f$
 *   and one addition of n when negative ends it: two products of 128 bits
 *   and one of 64 instead of a division of 128 bits, exact for every odd
 *   n; \f$ n^{-1} \bmod 2^{64} \f$ comes from \f$ x_0 = 3 n \oplus 2 \f$,
 *   right on 5 bits, by 4 steps of Newton \f$ x_{i+1} = x_i ( 2 - n x_i )
 *   \f$, each doubling the bits that are right, and \f$ R \bmod n \f$ and
 *   \f$ R^2 \bmod n \f$ by one division of 128 by 64 bits each;
 * - isPrimeTrial(), the divisions up to \f$ \sqrt{n} \f$,
 *   \f$ O( 2^{d/2} ) \f$ for n of d bits;
 * - witness(), b is a witness that n is composite if
 *   \f$ \gcd( n , b ) \neq 1 \f$ or, with \f$ n - 1 = 2^v m \f$ and m odd,
 *   neither \f$ b^m \bmod n = 1 \f$ nor \f$ b^{2^i m} \bmod n = n - 1 \f$
 *   for some 0 <= i < v; millerRabin() tries K random bases in
 *   [ 2 , n - 2 ], at least 3/4 of which are witnesses for a composite n,
 *   and errs with probability at most \f$ ( 1 / 4 )^K \f$, only by saying
 *   prime;
 * - isPrime(), beyond the book, exact on 64 bits: first the divisions by
 *   the 46 odd primes up to 211, by one product each with no branch, all
 *   at once on vectors (for the odd p, \f$ p \mid n \iff n p^{-1} \bmod
 *   2^{64} \le \lfloor ( 2^{64} - 1 ) / p \rfloor \f$, since \f$ x
 *   \mapsto x p^{-1} \f$ is a bijection that sends the multiples of p onto
 *   \f$ [ 0 , \lfloor ( 2^{64} - 1 ) / p \rfloor ] \f$), which leave about
 *   \f$ \prod_{3 \le p \le 211} ( 1 - 1 / p ) \approx 0.21 \f$ of the odd
 *   numbers; then the test of Miller and Rabin on the fewest bases known
 *   to have no strong pseudoprime below n: below \f$ 2^{32} \f$ one base
 *   read in a table of 1024 at the hash \f$ \lfloor ( n \cdot 2654435761
 *   \bmod 2^{32} ) / 2^{22} \rfloor \f$ (the hashed bases of Forisek and
 *   Jancina), the least base, at most 67, of which no odd composite of the
 *   bucket with no prime factor up to 211 is a strong pseudoprime
 *   (computed for ADS by the exhaustive search over the \f$ 2^{31} \f$
 *   odd numbers), in an arithmetic of Montgomery of 32 bits that needs
 *   only \f$ n^{-1} \bmod 2^{32} \f$ (the form of b by one division, the
 *   power from the lowest bit of d, the comparisons after one
 *   reduction), up to the 7 bases of
 *   Sinclair for every n of 64 bits. The k bases run in lockstep, k chains
 *   of products that the processor overlaps, so that 2 cost about as much
 *   as 1; with 3 or more, the exponent goes by windows of 4 bits with a
 *   table of \f$ b^0 , \dots , b^{15} \f$, \f$ \lceil \log_2 d \rceil + \lceil
 *   \log_2 d / 4 \rceil + 14 \f$ products per base instead of up to
 *   \f$ 2 \lceil \log_2 d \rceil \f$. The arithmetic is that of Montgomery
 *   of 32 bits below \f$ 2^{32} \f$ (\f$ R = 2^{32} \f$, its constants by
 *   divisions of 32 bits), the lazy one below \f$ 2^{62} \f$, and
 *   Montgomery64 above;
 * - factorize(), beyond the book, the prime factors of a word: the
 *   divisions by the primes below 256, then, on a composite cofactor m
 *   (isPrime() decides), the rho of Pollard with the cycle detection of
 *   Brent, in the Montgomery arithmetic of m: the sequence \f$ x_{i+1} =
 *   x_i^2 + c \bmod m \f$ falls into a cycle modulo the least prime p of m
 *   after \f$ O( \sqrt{p} ) = O( m^{1/4} ) \f$ steps in expectation (the
 *   birthday paradox, the map taken as random), which
 *   \f$ \gcd( | x_i - x_j | , m ) \f$ finds; the differences are
 *   multiplied together, 128 at a time, before each gcd, and the last
 *   batch is gone through again one by one if the gcd is m; with another
 *   c if the factor is m itself. O( n^{1/4} ) products expected for n of
 *   64 bits;
 * - sieve(), the sieve of Eratosthenes on a SET of the integers from 2 to n
 *   [see BoolSet.h], beyond the book: the least item p left in the set is a
 *   prime, and its multiples are removed; since the multiples of the primes
 *   below p are already gone, the removal can start at \f$ p^2 \f$, and the
 *   sieve stops at \f$ p > \sqrt{n} \f$, in \f$ O( n \log \log n ) \f$.
 *
 * The contracts [see Contract.h]: extendedEuclid() needs s , m >= 0 and gives
 * d = p s + q m; modInverse() needs m >= 1 and gcd( s , m ) = 1, and its
 * result r satisfies s r = 1 modulo m; the products and the powers modulo m
 * need m >= 1, and mulMod61() factors below \f$ 2^{61} - 1 \f$; witness()
 * needs an odd n >= 3 and a base 1 <= b < n, and millerRabin() K >= 0;
 * sieve() gives the primes up to n, increasing, as isPrimeTrial() finds
 * them (an audit); factorize() needs n >= 1 and gives primes whose product
 * is n (an audit). The functions are defined in NumberTheory.tpp.
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
 *         Universita' di Pisa \n
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

using U64 = std::uint64_t;

inline U64 gcd( U64 s , U64 m );

/// the gcd of a and b by the binary algorithm, with no division;
/// gcd( a , 0 ) = a
[[nodiscard]] constexpr U64 binaryGcd( U64 a , U64 b );

/// euclideEsteso(): ( d , p , q ) with s p + m q = d = gcd( s , m )
/// \pre s >= 0 and m >= 0
/// \post d = p s + q m
inline std::tuple< long long , long long , long long > extendedEuclid(
                                                   long long s , long long m );

/// the p in [ 0 , m ) with p s mod m = 1; throws if gcd( s , m ) != 1
/// \pre m >= 1 and gcd( s , m ) = 1
/// \post s r = 1 modulo m
inline long long modInverse( long long s , long long m );

/// the Mersenne prime 2^61 - 1
inline constexpr U64 kMersenne61 = ( U64( 1 ) << 61 ) - 1;

/// a b mod 2^61 - 1, by four products of 32 bits and the folding
/// 2^61 = 1 mod 2^61 - 1
/// \pre a < 2^61 - 1 and b < 2^61 - 1
/// \post the result is below 2^61 - 1
[[nodiscard]] constexpr U64 mulMod61( U64 a , U64 b );

/// a b mod m, exact for any a, b and m: by the product of 64 bits for
/// m <= 2^32, by mulMod61() for m = 2^61 - 1, by the product of 128 bits
/// or else by doubling and adding
/// \pre m >= 1
/// \post the result is below m
[[nodiscard]] constexpr U64 mulMod( U64 a , U64 b , U64 m );

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS Montgomery64 ----------------------------*/
/*--------------------------------------------------------------------------*/
/// the arithmetic modulo an odd n of 64 bits in Montgomery form, x R mod n
/// with R = 2^64; the numbers in the form are below n
class Montgomery64 {
 public:
 /// the arithmetic modulo n, with one division of 128 bits for R^2 mod n
 /// \pre n is odd
 explicit constexpr Montgomery64( U64 n );

 [[nodiscard]] constexpr U64 modulus( void ) const { return( f_n ); }

 /// x R mod n
 [[nodiscard]] constexpr U64 to( U64 x ) const;

 /// x R^-1 mod n, the number whose form is x
 /// \pre x < n
 [[nodiscard]] constexpr U64 from( U64 x ) const { return( reduce( 0 , x ) ); }

 /// R mod n, the form of 1
 [[nodiscard]] constexpr U64 one( void ) const { return( f_one ); }

 /// a b R^-1 mod n, the form of the product of the numbers of the forms
 /// a and b
 /// \pre a < n and b < n
 [[nodiscard]] constexpr U64 mul( U64 a , U64 b ) const;

 /// the form of x^v, x a form
 /// \pre x < n
 [[nodiscard]] constexpr U64 pow( U64 x , U64 v ) const;

 private:
 /// ( hi 2^64 + lo ) R^-1 mod n
 /// \pre hi < n
 [[nodiscard]] constexpr U64 reduce( U64 hi , U64 lo ) const;

 U64 f_n;       ///< the modulus
 U64 f_inv;     ///< n^-1 mod 2^64
 U64 f_one;     ///< R mod n
 U64 f_r2;      ///< R^2 mod n
 };  // end( class Montgomery64 )

/// x^v mod m by repeated squaring, the products of Montgomery when m is odd
/// \pre m >= 1
inline U64 powMod( U64 x , U64 v , U64 m );

/// primo(): the divisions by 2 , ... , floor( sqrt( n ) )
inline bool isPrimeTrial( U64 n );

/// true if b is a witness that the odd n > 2 is composite
/// \pre n >= 3 odd and 1 <= b < n
inline bool witness( U64 n , U64 b );

/// primo() of Miller-Rabin: K random bases, wrong with probability at most
/// ( 1 / 4 )^K, and only on a composite n
/// \pre K >= 0
template< class R >
bool millerRabin( U64 n , int K , R & rng );

/// the primality of n, exact for every n of 64 bits
inline bool isPrime( U64 n );

/// the primes up to n, increasing, by the sieve of Eratosthenes
/// \post the primes up to n, as isPrimeTrial() finds them (an audit)
inline std::vector< U64 > sieve( U64 n );

/// the prime factors of n, with their multiplicity, increasing, by the
/// divisions by the primes below 256 and the rho of Pollard and Brent
/// \pre n >= 1
/// \post their product is n and each one is prime (an audit)
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
