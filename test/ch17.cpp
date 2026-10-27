/*--------------------------------------------------------------------------*/
/*----------------------------- File ch17.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 17, Algoritmi probabilistici: the coin, the null
 * polynomial expression, the selection of Hoare with random pivots and the
 * one of linear worst case, the test of Miller-Rabin against the trial
 * divisions and on the numbers that fool weaker tests, RSA, and the
 * exercises 17.1 to 17.5, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 17,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/algorithm/MonteCarlo.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/RSA.h"
#include "ADS/search/Selection.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// heads or tails: random( 0 , 1 ) gives each with probability 1/2; the null
// polynomial expressions, and the nonzero ones, by Schwartz-Zippel; K = 0,
// constants, degree 0, and the precondition
ADS_TEST( "sec17.monte-carlo" )
{
 std::mt19937 g( 1 );
 std::uniform_int_distribution< int > coin( 0 , 1 );
 int heads = 0;
 for( int t = 0 ; t < 100000 ; ++t ) heads += coin( g );
 CHECK( std::abs( heads - 50000 ) < 1500 );
 auto square = []( const std::vector< long > & x ) {
  const long a = x[ 0 ] , b = x[ 1 ];
  return( ( a + b ) * ( a + b ) - a * a - 2 * a * b - b * b ); };
 auto almost = []( const std::vector< long > & x ) {     // x0 x1 - x1 x0 + 1
  return( x[ 0 ] * x[ 1 ] - x[ 1 ] * x[ 0 ] + 1 ); };
 auto roots = []( const std::vector< long > & x ) {   // ( x - 1 ) .. ( x - 6 )
  long p = 1;
  for( long r = 1 ; r <= 6 ; ++r ) p *= x[ 0 ] - r;
  return( p ); };
 CHECK( zeroPolynomial( square , 2 , 2 , 20 , g ) );
 CHECK( ! zeroPolynomial( almost , 2 , 2 , 20 , g ) );
 CHECK( ! zeroPolynomial( roots , 1 , 6 , 40 , g ) );
 int fooled = 0;                          // one trial: at most 1/2 wrong
 for( int t = 0 ; t < 10000 ; ++t )
  fooled += zeroPolynomial( roots , 1 , 6 , 1 , g );
 CHECK( fooled < 5300 );
 // the edges: K = 0 trials say zero; no variable, a constant; degree 0;
 // the polynomial 0 of any degree; K negative
 auto one = []( const std::vector< long > & ) { return( 1L ); };
 auto nought = []( const std::vector< long > & ) { return( 0L ); };
 CHECK( zeroPolynomial( one , 0 , 0 , 0 , g ) );
 CHECK( ! zeroPolynomial( one , 0 , 0 , 1 , g ) );
 CHECK( zeroPolynomial( nought , 0 , 0 , 5 , g ) &&
        zeroPolynomial( nought , 3 , 1000 , 5 , g ) );
 CHECK( ! zeroPolynomial( one , 4 , -1 , 3 , g ) );
 CHECK_THROWS( zeroPolynomial( one , 1 , 1 , -1 , g ) , contract_violation );
 }

// the selection of Hoare, with and without random pivots, and the median
// of medians, against std::nth_element for every rank; k = 1 and k = n,
// equal items, two values, the extremes of int, and the preconditions
ADS_TEST( "sec17.1.selection" )
{
 std::mt19937 g( 2 );
 for( int n : { 1 , 2 , 3 , 10 , 100 , 1000 } )
  for( int t = 0 ; t < 5 ; ++t ) {
   Ints A( n );
   for( auto & a : A ) a = int( g() % ( t % 2 ? 10 : 1000 ) );
   Ints S = A;
   std::sort( S.begin() , S.end() );
   for( int k = 1 ; k <= n ; k += std::max( 1 , n / 37 ) ) {
    Ints B = A , C = A , D = A;
    CHECK( selection( B , k , &g ) == S[ k - 1 ] );
    CHECK( selection< int >( C , k ) == S[ k - 1 ] );
    CHECK( medianOfMedians( D , k ) == S[ k - 1 ] );
    }
   }
 Ints sorted( 200000 );                  // the bad case of the first pivot
 std::iota( sorted.begin() , sorted.end() , 0 );
 Ints E = sorted;
 CHECK( selection( E , 100000 , &g ) == 99999 );
 Ints F = sorted;
 CHECK( medianOfMedians( F , 100000 ) == 99999 );
 CHECK_THROWS( selection< int >( E , 0 ) , contract_violation );
 // the edges: k = 1 and k = n, the least and the largest; all items
 // equal; two values only; the reversed sequence; the extremes of int;
 // k > n and the empty vector
 for( int n : { 1 , 2 , 7 , 1000 } ) {
  Ints R( n );
  std::iota( R.rbegin() , R.rend() , -n / 2 );
  for( std::size_t k : { std::size_t( 1 ) , std::size_t( n ) } ) {
   Ints B = R , C = R , D = R;
   const int want = k == 1 ? -n / 2 : -n / 2 + n - 1;
   CHECK( selection( B , k , &g ) == want && selection< int >( C , k ) ==
          want && medianOfMedians( D , k ) == want );
   }
  }
 Ints same( 3000 , 42 );
 for( std::size_t k : { std::size_t( 1 ) , std::size_t( 1500 ) ,
                        std::size_t( 3000 ) } ) {
  Ints B = same , C = same , D = same;
  CHECK( selection( B , k , &g ) == 42 && selection< int >( C , k ) == 42 &&
         medianOfMedians( D , k ) == 42 );
  }
 Ints two( 999 );
 for( int i = 0 ; i < 999 ; ++i ) two[ i ] = ( i * 7 ) % 3 == 0 ? 1 : 0;
 const int zeros = int( std::count( two.begin() , two.end() , 0 ) );
 for( std::size_t k : { std::size_t( zeros ) , std::size_t( zeros + 1 ) } ) {
  Ints B = two , C = two , D = two;
  const int want = int( k ) <= zeros ? 0 : 1;
  CHECK( selection( B , k , &g ) == want && selection< int >( C , k ) ==
         want && medianOfMedians( D , k ) == want );
  }
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 Ints X = { hi , lo , 0 , hi , lo };
 CHECK( medianOfMedians( X , 1 ) == lo && medianOfMedians( X , 5 ) == hi &&
        selection( X , 3 , &g ) == 0 );
 Ints none;
 CHECK_THROWS( selection< int >( X , 6 ) , contract_violation );
 CHECK_THROWS( selection< int >( none , 1 ) , contract_violation );
 CHECK_THROWS( medianOfMedians( X , 0 ) , contract_violation );
 CHECK_THROWS( medianOfMedians( none , 1 ) , contract_violation );
 }

// Miller-Rabin against the trial divisions; the Carmichael numbers and the
// strong pseudoprimes to small bases are found composite; for every odd
// composite n < 3000 at least 3/4 of the bases are witnesses; n below 5,
// 2^64 - 1, squares of primes, the bases 1 and n - 1, K = 0, the sieve,
// and the preconditions
ADS_TEST( "sec17.2.primality" )
{
 std::mt19937_64 g( 3 );
 for( U64 n = 0 ; n < 100000 ; ++n )
  CHECK( isPrime( n ) == isPrimeTrial( n ) );
 for( U64 n = 0 ; n < 20000 ; ++n )
  CHECK( millerRabin( n , 20 , g ) == isPrimeTrial( n ) );
 for( U64 c : { 561ull , 1105ull , 1729ull , 2465ull , 2821ull , 6601ull ,
                3215031751ull , 3825123056546413051ull } )
  CHECK( ! isPrime( c ) && ! millerRabin( c , 30 , g ) );
 for( U64 p : { 2305843009213693951ull ,                    // 2^61 - 1
                18446744073709551557ull ,                   // 2^64 - 59
                1000000007ull , 998244353ull } )
  CHECK( isPrime( p ) && millerRabin( p , 30 , g ) );
 for( U64 n = 9 ; n < 3000 ; n += 2 ) {
  if( isPrimeTrial( n ) ) continue;
  U64 w = 0;
  for( U64 b = 2 ; b <= n - 2 ; ++b ) w += witness( n , b );
  CHECK( 4 * w >= 3 * ( n - 3 ) );
  }
 // the edges: 0 , 1 not prime, 2 , 3 prime, 4 the least even composite;
 // the largest 64 bit number, composite, and the squares of primes near
 // 2^32; the base 1 and the base n - 1 are never witnesses; K = 0 trials
 // say prime; the sieve up to 0 , 1 , 2 and against isPrime(); n even or
 // below 3, a base out of [ 1 , n ), K negative
 for( U64 n : { 0ull , 1ull , 4ull } )
  CHECK( ! isPrime( n ) && ! millerRabin( n , 5 , g ) );
 for( U64 n : { 2ull , 3ull } )
  CHECK( isPrime( n ) && millerRabin( n , 5 , g ) );
 CHECK( ! isPrime( ~U64( 0 ) ) && ! millerRabin( ~U64( 0 ) , 10 , g ) );
 CHECK( ! isPrime( 4294967291ull * 4294967291ull ) );        // p^2, p < 2^32
 CHECK( ! isPrime( 4294967291ull * 4294967279ull ) );
 for( U64 n : { U64( 9 ) , U64( 561 ) , U64( 1000000007 ) , ~U64( 0 ) } )
  CHECK( ! witness( n , 1 ) && ! witness( n , n - 1 ) );
 CHECK( millerRabin( 561 , 0 , g ) );                       // no trial
 CHECK( sieve( 0 ).empty() && sieve( 1 ).empty() &&
        sieve( 2 ) == std::vector< U64 >( { 2 } ) );
 const auto P = sieve( 10000 );
 std::size_t at = 0;
 for( U64 n = 0 ; n <= 10000 ; ++n )
  if( isPrime( n ) ) CHECK( at < P.size() && P[ at++ ] == n );
 CHECK( at == P.size() && P.size() == 1229 );
 CHECK_THROWS( witness( 10 , 3 ) , contract_violation );
 CHECK_THROWS( witness( 1 , 1 ) , contract_violation );
 CHECK_THROWS( witness( 9 , 0 ) , contract_violation );
 CHECK_THROWS( witness( 9 , 9 ) , contract_violation );
 CHECK_THROWS( millerRabin( 7 , -1 , g ) , contract_violation );
 }

// RSA: M^( p s ) mod N = M, so decrypting undoes encrypting, and checking
// with the public key undoes signing with the private one; the messages
// 0 , 1 , N - 1, the primes of 2 and 63 bits, and the preconditions
ADS_TEST( "sec17.3.rsa" )
{
 std::mt19937_64 g( 4 );
 for( int t = 0 ; t < 5 ; ++t ) {
  const auto [ pub , priv ] = rsaKeys( g );
  CHECK( pub.N == priv.N && pub.N > ( U64( 1 ) << 59 ) );
  std::uniform_int_distribution< U64 > msg( 0 , pub.N - 1 );
  for( int k = 0 ; k < 100 ; ++k ) {
   const U64 M = msg( g );
   CHECK( rsa( rsa( M , pub ) , priv ) == M );
   CHECK( rsa( rsa( M , priv ) , pub ) == M );        // a signature
   }
  // the edges: the messages 0 , 1 and N - 1 are fixed or undone too; a
  // message N is out of range
  for( U64 M : { U64( 0 ) , U64( 1 ) , pub.N - 1 } )
   CHECK( rsa( rsa( M , pub ) , priv ) == M &&
          rsa( rsa( M , priv ) , pub ) == M );
  CHECK( rsa( 0 , pub ) == 0 && rsa( 1 , priv ) == 1 );
  CHECK_THROWS( rsa( pub.N , pub ) , contract_violation );
  }
 // the primes of 2 and of 63 bits, the extremes of randomPrime(); 1 and
 // 64 bits out of range
 CHECK( randomPrime( 2 , g ) == 3 );
 const U64 p63 = randomPrime( 63 , g );
 CHECK( ( p63 >> 62 ) == 1 && isPrime( p63 ) );
 CHECK_THROWS( randomPrime( 1 , g ) , contract_violation );
 CHECK_THROWS( randomPrime( 64 , g ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// x^v mod m by repeated squaring, against the product of v factors; v = 0,
// m = 1, x = 0, x >= m, the largest modulus, and the preconditions
ADS_TEST( "ex17.1" )
{
 std::mt19937_64 g( 5 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const U64 x = g() % 1000 , v = g() % 300 , m = 1 + g() % 100000;
  U64 r = 1 % m;
  for( U64 i = 0 ; i < v ; ++i ) r = r * x % m;
  CHECK( powMod( x , v , m ) == r );
  }
 CHECK( powMod( 2 , 64 , 18446744073709551557ull ) == 59 );   // 2^64 - p
 // the edges: v = 0 gives 1 mod m, so 0^0 = 1 and anything mod 1 is 0;
 // x = 0; x >= m; the largest modulus, where the products need 128 bits,
 // and Fermat on the prime 2^64 - 59; mulMod() on the largest operands;
 // m = 0
 const U64 M = ~U64( 0 ) , p = 18446744073709551557ull;
 CHECK( powMod( 0 , 0 , 7 ) == 1 && powMod( 5 , 0 , 1 ) == 0 &&
        powMod( 0 , 0 , 1 ) == 0 && powMod( 0 , 9 , 7 ) == 0 );
 CHECK( powMod( 23 , 3 , 10 ) == 7 && powMod( M , 1 , M ) == 0 );
 CHECK( powMod( M - 1 , 2 , M ) == 1 );                 // ( -1 )^2
 CHECK( powMod( 3 , p - 1 , p ) == 1 && powMod( p - 1 , 3 , p ) == p - 1 );
 CHECK( mulMod( M , M , M ) == 0 && mulMod( M - 1 , M - 1 , M ) == 1 );
 CHECK( mulMod( M , M , 1 ) == 0 && mulMod( M , 2 , 10 ) == 0 );
 CHECK_THROWS( powMod( 2 , 3 , 0 ) , contract_violation );
 CHECK_THROWS( mulMod( 2 , 3 , 0 ) , contract_violation );
 }

// the extended algorithm of Euclid: s p + m q = gcd( s , m ); zeros,
// s = m, Fibonacci numbers, the largest long long, and the preconditions
ADS_TEST( "ex17.2" )
{
 std::mt19937_64 g( 6 );
 for( int t = 0 ; t < 10000 ; ++t ) {
  using LL = long long;
  const LL s = LL( g() % 1000000 ) , m = LL( g() % 1000000 );
  const auto [ d , p , q ] = extendedEuclid( s , m );
  CHECK( d == std::gcd( s , m ) && s * p + m * q == d );
  CHECK( U64( d ) == gcd( U64( s ) , U64( m ) ) );
  }
 // the edges: gcd( 0 , 0 ) = 0 with p = 1 , q = 0; gcd( s , 0 ) = s and
 // gcd( 0 , m ) = m; s = m; coprime consecutive Fibonacci numbers, the
 // longest chain of divisions; the largest long long, where p s + q m is
 // checked modulo 2^64; s or m negative
 using LL = long long;
 using T3 = std::tuple< LL , LL , LL >;
 CHECK( extendedEuclid( 0 , 0 ) == T3( 0 , 1 , 0 ) && gcd( 0 , 0 ) == 0 );
 CHECK( extendedEuclid( 12 , 0 ) == T3( 12 , 1 , 0 ) && gcd( 12 , 0 ) == 12 );
 CHECK( std::get< 0 >( extendedEuclid( 0 , 12 ) ) == 12 && gcd( 0 , 12 ) ==
        12 );
 const auto [ e0 , e1 , e2 ] = extendedEuclid( 0 , 12 );
 CHECK( e1 * 0 + e2 * 12 == e0 );
 const auto [ f0 , f1 , f2 ] = extendedEuclid( 9 , 9 );
 CHECK( f0 == 9 && f1 * 9 + f2 * 9 == 9 );
 const LL F91 = 4660046610375530309LL , F92 = 7540113804746346429LL;
 const auto [ d , p , q ] = extendedEuclid( F92 , F91 );
 CHECK( d == 1 && U64( p ) * U64( F92 ) + U64( q ) * U64( F91 ) == 1 );
 CHECK( std::abs( p ) <= F91 && std::abs( q ) <= F92 );
 const LL top = std::numeric_limits< LL >::max();
 CHECK( std::get< 0 >( extendedEuclid( top , top - 1 ) ) == 1 &&
        std::get< 0 >( extendedEuclid( top , top ) ) == top );
 CHECK( gcd( ~U64( 0 ) , ~U64( 0 ) - 2 ) == 1 &&
        gcd( U64( 1 ) << 63 , U64( 1 ) << 40 ) == ( U64( 1 ) << 40 ) );
 CHECK_THROWS( extendedEuclid( -1 , 5 ) , contract_violation );
 CHECK_THROWS( extendedEuclid( 5 , -1 ) , contract_violation );
 }

// the multiplicative inverse, p s mod ( x - 1 ) ( y - 1 ) = 1; modulo 1,
// 1 and m - 1, s > m, a prime near 2^63, and the preconditions
ADS_TEST( "ex17.3" )
{
 const long long x = 61 , y = 53 , phi = ( x - 1 ) * ( y - 1 );
 // phi = 3120 = 2^4 3 5 13
 for( long long s : { 7LL , 11LL , 17LL , 19LL , 23LL } ) {
  const long long p = modInverse( s , phi );
  CHECK( p * s % phi == 1 && p >= 0 && p < phi );
  }
 CHECK_THROWS( modInverse( 13 , phi ) , contract_violation );
 // the edges: modulo 1 the inverse is 0; 1 is its own inverse, and so is
 // m - 1; s larger than m; a prime modulus near the largest long long;
 // s = 0, m < 1 and s negative
 CHECK( modInverse( 5 , 1 ) == 0 && modInverse( 0 , 1 ) == 0 );
 CHECK( modInverse( 1 , 97 ) == 1 && modInverse( 96 , 97 ) == 96 );
 CHECK( modInverse( 97 + 3 , 97 ) == modInverse( 3 , 97 ) );
 const long long P = 9223372036854775783LL;            // 2^63 - 25, prime
 const long long r = modInverse( 2 , P );
 CHECK( r == ( P + 1 ) / 2 && mulMod( 2 , U64( r ) , U64( P ) ) == 1 );
 CHECK_THROWS( modInverse( 0 , 7 ) , contract_violation );
 CHECK_THROWS( modInverse( 3 , 0 ) , contract_violation );
 CHECK_THROWS( modInverse( -3 , 7 ) , contract_violation );
 }

// the check of a product by Freivalds, O( n^2 ) per run; 0 x 0 and 1 x 1,
// K = 0, the zero matrix, and the preconditions
ADS_TEST( "ex17.4" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const std::size_t n = 1 + g() % 30;
  Matrix< long > A( n , Longs( n ) ) , B = A;
  for( auto & r : A ) for( auto & x : r ) x = long( g() % 21 ) - 10;
  for( auto & r : B ) for( auto & x : r ) x = long( g() % 21 ) - 10;
  auto C = multiplyNaive( A , B );
  CHECK( freivalds( A , B , C , 20 , g ) );
  C[ g() % n ][ g() % n ] += 1;            // one entry off
  CHECK( ! freivalds( A , B , C , 20 , g ) );
  }
 // the edges: the matrices 0 x 0 and 1 x 1; K = 0 runs accept anything;
 // the zero matrix; the matrices not square or of different sizes, and K
 // negative
 using M = Matrix< long >;
 CHECK( freivalds( M() , M() , M() , 5 , g ) );
 CHECK( freivalds( M( { { 3 } } ) , M( { { -4 } } ) , M( { { -12 } } ) , 5 ,
                   g ) );
 CHECK( ! freivalds( M( { { 3 } } ) , M( { { -4 } } ) , M( { { 12 } } ) , 5 ,
                     g ) );
 CHECK( freivalds( M( { { 3 } } ) , M( { { 4 } } ) , M( { { 0 } } ) , 0 ,
                   g ) );
 const M Z( 4 , Longs( 4 , 0 ) );
 CHECK( freivalds( Z , Z , Z , 5 , g ) );
 const M I2 = { { 1 , 0 } , { 0 , 1 } };
 CHECK_THROWS( freivalds( I2 , I2 , M( { { 1 , 0 } } ) , 5 , g ) ,
               contract_violation );
 CHECK_THROWS( freivalds( I2 , Z , Z , 5 , g ) , contract_violation );
 CHECK_THROWS( freivalds( M( { { 1 , 2 } , { 3 } } ) , I2 , I2 , 5 , g ) ,
               contract_violation );
 CHECK_THROWS( freivalds( I2 , I2 , I2 , -1 , g ) , contract_violation );
 }

// more than half of the assignments: x0 or x1 holds on 3/4, x0 and x1 on
// 1/4, the majority of three variables on exactly 1/2 of 8 assignments;
// K = 0, no variable, exactly half, and the precondition
ADS_TEST( "ex17.5" )
{
 std::mt19937 g( 8 );
 auto orf = []( const std::vector< bool > & x ) {
  return( x[ 0 ] || x[ 1 ] ); };
 auto andf = []( const std::vector< bool > & x ) {
  return( x[ 0 ] && x[ 1 ] ); };
 CHECK( halfSatisfiable( orf , 2 , 2000 , g ) );
 CHECK( ! halfSatisfiable( andf , 2 , 2000 , g ) );
 // a formula of 20 variables true on 1 - 2^-3 of them: not x0 or not x1
 // or not x2
 auto many = []( const std::vector< bool > & x ) {
  return( ! x[ 0 ] || ! x[ 1 ] || ! x[ 2 ] ); };
 CHECK( halfSatisfiable( many , 20 , 2000 , g ) );
 // the edges: K = 0 assignments, no majority; no variable, the constant
 // formulas; a formula true on exactly half is not more than half when
 // the count is exact, as with K = 2 and alternating answers; K negative
 auto yes = []( const std::vector< bool > & ) { return( true ); };
 auto no = []( const std::vector< bool > & ) { return( false ); };
 CHECK( ! halfSatisfiable( yes , 3 , 0 , g ) );
 CHECK( halfSatisfiable( yes , 0 , 7 , g ) && ! halfSatisfiable( no , 0 , 7 ,
                                                                 g ) );
 bool flip = false;
 auto alternate = [ & ]( const std::vector< bool > & ) {
  return( flip = ! flip ); };
 CHECK( ! halfSatisfiable( alternate , 1 , 2 , g ) );
 CHECK_THROWS( halfSatisfiable( yes , 1 , -1 , g ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch17.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
