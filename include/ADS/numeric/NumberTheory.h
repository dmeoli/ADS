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
 * - powMod(), \f$ x^v \bmod m \f$ by repeated squaring, O( log v )
 *   multiplications (exercise 17.1), each mulMod() exact on 128 bits;
 * - isPrimeTrial(), the divisions up to \f$ \sqrt{n} \f$,
 *   \f$ O( 2^{d/2} ) \f$ for n of d bits;
 * - witness(), b is a witness that n is composite if
 *   \f$ \gcd( n , b ) \neq 1 \f$ or, with \f$ n - 1 = 2^v m \f$ and m odd,
 *   neither \f$ b^m \bmod n = 1 \f$ nor \f$ b^{2^i m} \bmod n = n - 1 \f$
 *   for some 0 <= i < v; millerRabin() tries K random bases in
 *   [ 2 , n - 2 ], at least 3/4 of which are witnesses for a composite n,
 *   and errs with probability at most \f$ ( 1 / 4 )^K \f$, only by saying
 *   prime; isPrime() tries the 12 primes up to 37, which are enough for
 *   every n < 3.3 10^24, and so it is exact on 64 bits;
 * - sieve(), the sieve of Eratosthenes on a SET of the integers from 2 to n
 *   [see BoolSet.h], beyond the book: the least item p left in the set is a
 *   prime, and its multiples are removed; since the multiples of the primes
 *   below p are already gone, the removal can start at \f$ p^2 \f$, and the
 *   sieve stops at \f$ p > \sqrt{n} \f$, in \f$ O( n \log \log n ) \f$.
 *
 * The contracts [see Contract.h]: extendedEuclid() needs s , m >= 0 and gives
 * d = p s + q m; modInverse() needs m >= 1 and gcd( s , m ) = 1, and its
 * result r satisfies s r = 1 modulo m; the products and the powers modulo m
 * need m >= 1; witness() needs an odd n >= 3 and a base 1 <= b < n, and
 * millerRabin() K >= 0; sieve() gives the primes up to n, increasing, as
 * isPrimeTrial() finds them (an audit). The functions are defined in
 * NumberTheory.tpp.
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

#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using U64 = std::uint64_t;

inline U64 gcd( U64 s , U64 m );

/// euclideEsteso(): ( d , p , q ) with s p + m q = d = gcd( s , m )
/// \pre s >= 0 and m >= 0
/// \post d = p s + q m
inline std::tuple< long long , long long , long long > extendedEuclid(
                                                   long long s , long long m );

/// the p in [ 0 , m ) with p s mod m = 1; throws if gcd( s , m ) != 1
/// \pre m >= 1 and gcd( s , m ) = 1
/// \post s r = 1 modulo m
inline long long modInverse( long long s , long long m );

/// a b mod m, exact
/// \pre m >= 1
inline U64 mulMod( U64 a , U64 b , U64 m );

/// x^v mod m by repeated squaring
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
