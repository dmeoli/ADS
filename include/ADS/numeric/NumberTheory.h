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
 *   multiplications (exercise 17.1), each one of Montgomery when m is odd
 *   (see below), with no division but the one that sets up the modulus;
 * - binaryGcd(), beyond the book, the binary gcd of Stein (Algorithm 4.5.2B
 *   of Knuth) with the trailing zeros counted by one instruction: the
 *   common power of 2 set apart, then \f$ \gcd( a , b ) =
 *   \gcd( \min( a , b ) , | a - b | ) \f$ with the zeros of the difference
 *   shifted out, O( log a + log b ) subtractions and no division;
 * - Montgomery64, beyond the book, the arithmetic modulo an odd n of 64
 *   bits in the form \f$ \bar{x} = x R \bmod n \f$, \f$ R = 2^{64} \f$:
 *   the product \f$ \bar{a} \bar{b} R^{-1} \bmod n \f$ is the high half
 *   of \f$ \bar{a} \bar{b} \f$ minus that of \f$ q n \f$, q the low half
 *   times \f$ n^{-1} \bmod R \f$, i.e., two products of 128 bits and one
 *   of 64 instead of a division of 128 bits, exact for every odd n;
 * - isPrimeTrial(), the divisions up to \f$ \sqrt{n} \f$,
 *   \f$ O( 2^{d/2} ) \f$ for n of d bits;
 * - witness(), b is a witness that n is composite if
 *   \f$ \gcd( n , b ) \neq 1 \f$ or, with \f$ n - 1 = 2^v m \f$ and m odd,
 *   neither \f$ b^m \bmod n = 1 \f$ nor \f$ b^{2^i m} \bmod n = n - 1 \f$
 *   for some 0 <= i < v; millerRabin() tries K random bases in
 *   [ 2 , n - 2 ], at least 3/4 of which are witnesses for a composite n,
 *   and errs with probability at most \f$ ( 1 / 4 )^K \f$, only by saying
 *   prime; isPrime(), beyond the book, is exact on 64 bits: the divisions
 *   by the primes up to 53 by one product each (n is a multiple of the odd
 *   p iff \f$ n p^{-1} \bmod 2^{64} \le \lfloor ( 2^{64} - 1 ) / p
 *   \rfloor \f$), then the test of Miller and Rabin in Montgomery form on
 *   the fewest bases known to have no strong pseudoprime below n, from 1
 *   base below 341531 to the 7 of Sinclair for every n of 64 bits;
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
 * them (an audit). The functions are defined in NumberTheory.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.2 and
 *      exercises 17.1-17.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see G. L. Miller, Riemann's hypothesis and tests for primality, Journal
 *      of Computer and System Sciences 13(3), 300-317, 1976,
 *      doi:10.1016/S0022-0000(76)80043-8
 * \see M. O. Rabin, Probabilistic algorithm for testing primality, Journal
 *      of Number Theory 12(1), 128-138, 1980,
 *      doi:10.1016/0022-314X(80)90084-0
 * \see J. Sorenson, J. Webster, Strong pseudoprimes to twelve prime bases,
 *      Mathematics of Computation 86(304), 985-1003, 2017,
 *      doi:10.1090/mcom/3134
 * \see P. L. Montgomery, Modular multiplication without trial division,
 *      Mathematics of Computation 44(170), 519-521, 1985,
 *      doi:10.1090/S0025-5718-1985-0777282-X
 * \see J. Stein, Computational problems associated with Racah algebra,
 *      Journal of Computational Physics 1(3), 397-405, 1967,
 *      doi:10.1016/0021-9991(67)90047-2
 * \see T. Granlund, P. L. Montgomery, Division by invariant integers using
 *      multiplication, PLDI 1994, 61-72, doi:10.1145/178243.178249
 * \see G. Jaeschke, On strong pseudoprimes to several bases, Mathematics
 *      of Computation 61(204), 915-926, 1993, doi:10.2307/2153262, and the
 *      bases of J. Sinclair and others at https://miller-rabin.appspot.com
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
