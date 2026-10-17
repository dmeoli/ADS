/*--------------------------------------------------------------------------*/
/*------------------------------- File RSA.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The public key cryptosystem RSA of the reality check of section 17.3, on
 * integers of 64 bits, a toy that shows the method and is not secure: the
 * keys need some thousand bits. Three random primes s, x and y, found by
 * Miller-Rabin, give N = x y and the public exponent p with
 * \f$ p s \bmod ( x - 1 ) ( y - 1 ) = 1 \f$; the public key is ( N , p ),
 * the private one ( N , s ), \f$ C = M^p \bmod N \f$ encrypts and
 * \f$ M = C^s \bmod N \f$ decrypts, since \f$ M^{p s} \bmod N = M \f$.
 * Signing is decrypting with the private key, checking is encrypting with
 * the public one.
 *
 * The contracts [see Contract.h]: randomPrime() needs 2 <= bits <= 63 and
 * gives an odd number of exactly that many bits; the keys of rsaKeys() undo
 * each other, checked on the message 2; rsa() needs a message M < N. The
 * functions are defined in RSA.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see R. L. Rivest, A. Shamir, L. Adleman, A method for obtaining digital
 *      signatures and public-key cryptosystems, Communications of the ACM
 *      21(2), 120-126, 1978, doi:10.1145/359340.359342
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

/// a key ( N , e )
struct RsaKey {
 U64 N , e;
 };

/// a random prime of at most the given bits, by Miller-Rabin
/// \pre 2 <= bits <= 63
/// \post an odd prime of exactly bits bits
template< class R >
U64 randomPrime( int bits , R & rng );

/// a pair ( public , private ) of keys, with N of about 62 bits
/// \post each key undoes the other
template< class R >
std::pair< RsaKey , RsaKey > rsaKeys( R & rng );

/// the message M < N under the key: encrypting with a public key, or
/// decrypting and signing with a private one
/// \pre M < N
inline U64 rsa( U64 M , const RsaKey & k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/RSA.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RSA.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File RSA.h -----------------------------*/
/*--------------------------------------------------------------------------*/
