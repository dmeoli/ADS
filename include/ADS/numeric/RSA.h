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
template< class R >
U64 randomPrime( int bits , R & rng )
{
 std::uniform_int_distribution< U64 > d( U64( 1 ) << ( bits - 1 ) ,
                                         ( U64( 1 ) << bits ) - 1 );
 for( ; ; )
  if( const U64 c = d( rng ) | 1 ; millerRabin( c , 40 , rng ) )
   return( c );
 }

/// a pair ( public , private ) of keys, with N of about 62 bits
template< class R >
std::pair< RsaKey , RsaKey > rsaKeys( R & rng )
{
 for( ; ; ) {
  const U64 x = randomPrime( 31 , rng ) , y = randomPrime( 31 , rng );
  if( x == y ) continue;
  const U64 phi = ( x - 1 ) * ( y - 1 );
  const U64 s = randomPrime( 24 , rng );           // private, prime
  if( phi % s == 0 ) continue;
  const U64 p = U64( modInverse( static_cast< long long >( s ) ,
                                  static_cast< long long >( phi ) ) );
  return( std::pair( RsaKey{ x * y , p } , RsaKey{ x * y , s } ) );
  }
 }

/// the message M < N under the key: encrypting with a public key, or
/// decrypting and signing with a private one
inline U64 rsa( U64 M , const RsaKey & k )
{
 return( powMod( M , k.e , k.N ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* RSA.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File RSA.h -----------------------------*/
/*--------------------------------------------------------------------------*/
