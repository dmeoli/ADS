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
 *   every n < 3.3 10^24, and so it is exact on 64 bits.
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

#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using U64 = std::uint64_t;

inline U64 gcd( U64 s , U64 m )
{
 while( m != 0 ) {
  const U64 r = s % m;
  s = m;
  m = r;
  }
 return( s );
 }

/// euclideEsteso(): ( d , p , q ) with s p + m q = d = gcd( s , m )
inline std::tuple< long long , long long , long long > extendedEuclid(
                                                   long long s , long long m )
{
 if( m == 0 )
  return( std::tuple( s , 1LL , 0LL ) );
 const auto [ d , p , q ] = extendedEuclid( m , s % m );
 return( std::tuple( d , q , p - ( s / m ) * q ) );
 }

/// the p in [ 0 , m ) with p s mod m = 1; throws if gcd( s , m ) != 1
inline long long modInverse( long long s , long long m )
{
 const auto [ d , p , q ] = extendedEuclid( s , m );
 if( d != 1 )
  throw( std::invalid_argument( "ADS::modInverse: gcd( s , m ) != 1" ) );
 return( ( ( p % m ) + m ) % m );
 }

/// a b mod m, exact
inline U64 mulMod( U64 a , U64 b , U64 m )
{
#ifdef __SIZEOF_INT128__
 return( U64( ( unsigned __int128 )( a ) * b % m ) );
#else
 U64 r = 0;
 for( a %= m ; b ; b >>= 1 ) {               // the doubling of Ahmes
  if( b & 1 ) r = ( r >= m - a ) ? r - ( m - a ) : r + a;
  a = ( a >= m - a ) ? a - ( m - a ) : a + a;
  }
 return( r );
#endif
 }

/// x^v mod m by repeated squaring
inline U64 powMod( U64 x , U64 v , U64 m )
{
 U64 r = 1 % m;
 for( x %= m ; v ; v >>= 1 , x = mulMod( x , x , m ) )
  if( v & 1 )
   r = mulMod( r , x , m );
 return( r );
 }

/// primo(): the divisions by 2 , ... , floor( sqrt( n ) )
inline bool isPrimeTrial( U64 n )
{
 if( n < 2 )
  return( false );
 for( U64 j = 2 ; j <= n / j ; ++j )
  if( n % j == 0 )
   return( false );
 return( true );
 }

/// true if b is a witness that the odd n > 2 is composite
inline bool witness( U64 n , U64 b )
{
 if( gcd( n , b ) != 1 )
  return( true );
 U64 m = n - 1;
 int v = 0;
 for( ; ( m & 1 ) == 0 ; m >>= 1 ) ++v;
 U64 x = powMod( b , m , n );
 if( ( x == 1 ) || ( x == n - 1 ) )
  return( false );
 for( int i = 1 ; i < v ; ++i ) {
  x = mulMod( x , x , n );
  if( x == n - 1 )
   return( false );
  }
 return( true );
 }

/// primo() of Miller-Rabin: K random bases, wrong with probability at most
/// ( 1 / 4 )^K, and only on a composite n
template< class R >
bool millerRabin( U64 n , int K , R & rng )
{
 if( n < 4 )
  return( n == 2 || n == 3 );
 if( n % 2 == 0 )
  return( false );
 std::uniform_int_distribution< U64 > base( 2 , n - 2 );
 for( int j = 0 ; j < K ; ++j )
  if( witness( n , base( rng ) ) )
   return( false );
 return( true );
 }

/// the primality of n, exact for every n of 64 bits
inline bool isPrime( U64 n )
{
 if( n < 2 )
  return( false );
 for( U64 p : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } ) {
  if( n == p )
   return( true );
  if( n % p == 0 )
   return( false );
  }
 for( U64 b : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } )
  if( witness( n , b ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* NumberTheory.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File NumberTheory.h -------------------------*/
/*--------------------------------------------------------------------------*/
