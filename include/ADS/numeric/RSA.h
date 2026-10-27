/*--------------------------------------------------------------------------*/
/*------------------------------- File RSA.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The public key cryptosystem RSA of the reality check of section 17.3, on
 * integers of 64 bits, a toy that shows the method and is not secure: the
 * keys need some thousand bits. Three random primes \f$ s \f$, \f$ x \f$ and
 * \f$ y \f$, found by the test of Miller and Rabin, give \f$ N = x y \f$ and
 * the public exponent \f$ p \f$ with
 * \f$ p s \bmod ( x - 1 ) ( y - 1 ) = 1 \f$; the public key is
 * \f$ ( N , p ) \f$, the private one \f$ ( N , s ) \f$,
 * \f$ C = M^p \bmod N \f$ encrypts and \f$ M = C^s \bmod N \f$ decrypts,
 * since \f$ M^{p s} \bmod N = M \f$. Signing is decrypting with the private
 * key, checking is encrypting with the public one.
 *
 * The contracts [see `Contract.h`]: `randomPrime()` needs
 * \f$ 2 \le \mathit{bits} \le 63 \f$ and gives an odd number of exactly that
 * many bits; the keys of `rsaKeys()` undo each other, checked on the message
 * 2; `rsa()` needs a message \f$ M < N \f$. The functions are defined in
 * `RSA.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see R. L. Rivest, A. Shamir, L. Adleman, A method for obtaining digital
 *      signatures and public-key cryptosystems, Communications of the ACM
 *      21(2), 120-126, 1978, doi:10.1145/359340.359342
 * \see G. L. Miller, Riemann's hypothesis and tests for primality, Journal
 *      of Computer and System Sciences 13(3), 300-317, 1976,
 *      doi:10.1016/S0022-0000(76)80043-8
 * \see M. O. Rabin, Probabilistic algorithm for testing primality, Journal
 *      of Number Theory 12(1), 128-138, 1980,
 *      doi:10.1016/0022-314X(80)90084-0
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

#ifndef __ADS_RSA
 #define __ADS_RSA
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/numeric/NumberTheory.h"

#include <cstdint>
#include <random>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a key \f$ ( N , e ) \f$
struct RsaKey {
 U64 N ,   ///< the modulus, the product of two primes
     e;    ///< the exponent
 };

/// a random odd prime of exactly \f$ \mathit{bits} \f$ bits, by the test of
/// Miller and Rabin on 40 bases
///
/// In pseudocode (section 17.3), a wrong answer with probability at most
/// \f$ 4^{-40} \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{randomPrime}( \mathit{bits} ) \cr
/// 2 & \quad \textbf{repeat} \cr
/// 3 & \qquad c \gets \text{a random integer in } [ 2^{\mathit{bits} - 1} ,
///     2^{\mathit{bits}} ) \text{, made odd} \cr
/// 4 & \quad \textbf{until}\ \mathrm{millerRabin}( c , 40 ) \cr
/// 5 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ 2 \le \mathit{bits} \le 63 \f$
/// \post an odd number of exactly \f$ \mathit{bits} \f$ bits
template< class R >
U64 randomPrime( int bits , R & rng );

/// a pair (public, private) of keys, with \f$ N \f$ of about 62 bits
///
/// In pseudocode (section 17.3), the private exponent a prime \f$ s \f$ that
/// does not divide \f$ \varphi = ( x - 1 ) ( y - 1 ) \f$, hence prime to it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rsaKeys}() \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad x \gets \mathrm{randomPrime}( 31 );\ y \gets
///     \mathrm{randomPrime}( 31 ) \cr
/// 4 & \qquad s \gets \mathrm{randomPrime}( 24 );\ \varphi \gets ( x - 1 ) (
///     y - 1 ) \cr
/// 5 & \qquad \textbf{if}\ x = y\ \textbf{or}\ s \mid \varphi
///     \ \textbf{then}\ \textbf{continue} \cr
/// 6 & \qquad p \gets \mathrm{modInverse}( s , \varphi ) \cr
/// 7 & \qquad \textbf{return}\ ( ( x y , p ) , ( x y , s ) )
/// \end{array}}
/// \f]
/// \post each key undoes the other, checked on the message 2
template< class R >
std::pair< RsaKey , RsaKey > rsaKeys( R & rng );

/// the message \f$ M < N \f$ under the key, \f$ M^e \bmod N \f$: encrypting
/// with a public key, or decrypting and signing with a private one
///
/// In pseudocode (section 17.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rsa}( M , ( N , e ) ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{powMod}( M , e , N )
/// \end{array}}
/// \f]
/// \pre \f$ M < N \f$
inline U64 rsa( U64 M , const RsaKey & k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/RSA.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RSA.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File RSA.h -----------------------------*/
/*--------------------------------------------------------------------------*/
