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
 *         Universita' di Pisa \n
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
// polynomial expressions, and the nonzero ones, by Schwartz-Zippel
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
 }

// the selection of Hoare, with and without random pivots, and the median
// of medians, against std::nth_element for every rank
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
 CHECK_THROWS( selection< int >( E , 0 ) , std::invalid_argument );
 }

// Miller-Rabin against the trial divisions; the Carmichael numbers and the
// strong pseudoprimes to small bases are found composite; for every odd
// composite n < 3000 at least 3/4 of the bases are witnesses
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
 }

// RSA: M^( p s ) mod N = M, so decrypting undoes encrypting, and checking
// with the public key undoes signing with the private one
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
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// x^v mod m by repeated squaring, against the product of v factors
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
 }

// the extended algorithm of Euclid: s p + m q = gcd( s , m )
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
 }

// the multiplicative inverse, p s mod ( x - 1 ) ( y - 1 ) = 1
ADS_TEST( "ex17.3" )
{
 const long long x = 61 , y = 53 , phi = ( x - 1 ) * ( y - 1 );
 // phi = 3120 = 2^4 3 5 13
 for( long long s : { 7LL , 11LL , 17LL , 19LL , 23LL } ) {
  const long long p = modInverse( s , phi );
  CHECK( p * s % phi == 1 && p >= 0 && p < phi );
  }
 CHECK_THROWS( modInverse( 13 , phi ) , std::invalid_argument );
 }

// the check of a product by Freivalds, O( n^2 ) per run
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
 }

// more than half of the assignments: x0 or x1 holds on 3/4, x0 and x1 on
// 1/4, the majority of three variables on exactly 1/2 of 8 assignments
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
