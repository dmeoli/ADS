/*--------------------------------------------------------------------------*/
/*----------------------- File contract_numeric.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the numeric algorithms: the
 * results as the specifications of the book state them, and the violations of
 * the preconditions, which must throw ADS::contract_violation. The tests are
 * compiled at the level 3, so that each call also checks its postconditions
 * and its audits, on random instances against a reference of the standard
 * library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.4 and
 *      12.3-12.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 13.1 and
 *      13.4, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 17.2 and
 *      17.3, https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
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

#include "ADS/numeric/BinaryInteger.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/MatrixChain.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/Polynomial.h"
#include "ADS/numeric/RSA.h"

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Longs = std::vector< long >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the bits of an integer and back, the sum and the three products against
// the arithmetic of 64 bits; more than 64 bits, or an item that is not a
// bit, violate the contracts
ADS_TEST( "binary-integer" )
{
 std::mt19937_64 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::uint64_t x = g() >> 33 , y = g() >> 33;
  const Bits X = toBits( x ) , Y = toBits( y , 40 );
  CHECK( fromBits( X ) == x && Y.size() >= 40 && fromBits( Y ) == y );
  CHECK( fromBits( binaryAdd( X , Y ) ) == x + y );
  CHECK( fromBits( multiplyBinarySchool( X , Y ) ) == x * y );
  CHECK( fromBits( multiplyBinaryFour( X , Y ) ) == x * y );
  CHECK( fromBits( multiplyBinaryKaratsuba( X , Y ) ) == x * y );
  CHECK( ahmes( x , y ) == x * y );
  }
 CHECK_THROWS( (void) fromBits( Bits( 65 , 0 ) ) , contract_violation );
 CHECK_THROWS( (void) fromBits( Bits( { 1 , 2 } ) ) , contract_violation );
 CHECK_THROWS( (void) binaryAdd( Bits( { 1 } ) , Bits( { -1 } ) ) ,
               contract_violation );
 CHECK_THROWS( (void) multiplyBinaryKaratsuba( Bits( { 3 } ) ,
                                               Bits( { 1 } ) ) ,
               contract_violation );
 }

// the binomial coefficients against the triangle, the two Fibonacci; the
// indices out of their domain violate the contracts
ADS_TEST( "combinatorics" )
{
 const auto C = tartaglia( 20 );
 for( int n = 0 ; n <= 20 ; ++n )
  for( int k = 0 ; k <= n ; ++k ) CHECK( binomial( n , k ) == C[ n ][ k ] );
 for( int n = 0 ; n <= 80 ; ++n )
  CHECK( fibonacci( n ) == fibonacciTable( n ) );
 CHECK_THROWS( (void) binomial( 3 , 4 ) , contract_violation );
 CHECK_THROWS( (void) binomial( 3 , -1 ) , contract_violation );
 CHECK_THROWS( (void) tartaglia( -1 ) , contract_violation );
 CHECK_THROWS( (void) fibonacci( -1 ) , contract_violation );
 CHECK_THROWS( (void) fibonacciTable( -2 ) , contract_violation );
 }

// the chain of matrices against the enumeration of the
// parenthesizations; no size, a size that is not positive, and a
// parenthesization out of the chain violate the contracts
ADS_TEST( "matrix-chain" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Longs c( 2 + g() % 6 );
  for( auto & x : c ) x = 1 + long( g() % 9 );
  const std::size_t n = c.size() - 1;
  std::vector< std::vector< long > > M( n , std::vector< long >( n , 0 ) );
  for( std::size_t h = 2 ; h <= n ; ++h )
   for( std::size_t i = 0 ; i + h <= n ; ++i ) {
    const std::size_t j = i + h - 1;
    M[ i ][ j ] = -1;
    for( std::size_t k = i ; k < j ; ++k ) {
     const long v = M[ i ][ k ] + M[ k + 1 ][ j ] +
                    c[ i ] * c[ k + 1 ] * c[ j + 1 ];
     if( ( M[ i ][ j ] < 0 ) || ( v < M[ i ][ j ] ) ) M[ i ][ j ] = v;
     }
    }
  const auto [ best , S ] = matrixChain( c );
  CHECK( best == M[ 0 ][ n - 1 ] &&
         ! parenthesization( S , 0 , n - 1 ).empty() );
  }
 CHECK_THROWS( (void) matrixChain( Longs() ) , contract_violation );
 CHECK_THROWS( (void) matrixChain( Longs( { 2 , 0 , 3 } ) ) ,
               contract_violation );
 const auto S = matrixChain( Longs( { 2 , 3 , 4 } ) ).second;
 CHECK_THROWS( (void) parenthesization( S , 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) parenthesization( S , 0 , 2 ) , contract_violation );
 }

// Strassen against the naive product on random sizes, and the rectangular
// products; matrices of incompatible or ragged shapes violate the
// contracts
ADS_TEST( "matrix" )
{
 std::mt19937 g( 4 );
 for( std::size_t n = 0 ; n <= 9 ; ++n ) {
  Matrix< long > A( n , Longs( n ) ) , B( n , Longs( n ) );
  for( auto & r : A ) for( auto & x : r ) x = long( g() % 11 ) - 5;
  for( auto & r : B ) for( auto & x : r ) x = long( g() % 11 ) - 5;
  CHECK( strassen( A , B , 1 ) == multiplyNaive( A , B ) );
  CHECK( multiply( A , B ) == multiplyNaive( A , B ) );
  }
 const Matrix< long > R = { { 1 , 2 , 3 } , { 4 , 5 , 6 } };
 CHECK( multiply( R , transpose( R ) ) ==
        Matrix< long >( { { 14 , 32 } , { 32 , 77 } } ) );
 const Matrix< long > ragged = { { 1 , 2 } , { 3 } };
 CHECK_THROWS( (void) multiply( R , R ) , contract_violation );
 CHECK_THROWS( (void) transpose( ragged ) , contract_violation );
 CHECK_THROWS( (void) multiplyNaive( R , R ) , contract_violation );
 CHECK_THROWS( (void) strassen( R , R ) , contract_violation );
 CHECK_THROWS( (void) strassen( Matrix< long >( 2 , Longs( 2 ) ) ,
                                Matrix< long >( 3 , Longs( 3 ) ) ) ,
               contract_violation );
 }

// Euclid, the inverse modulo m, the products and the powers modulo m, the
// primality of Miller-Rabin and of the bases up to 37 against the trial
// divisions; arguments out of their domain violate the contracts
ADS_TEST( "number-theory" )
{
 std::mt19937_64 g( 5 );
 for( int t = 0 ; t < 300 ; ++t ) {
  using LL = long long;
  const LL s = LL( g() % 100000 ) , m = 1 + LL( g() % 100000 );
  const auto [ d , p , q ] = extendedEuclid( s , m );
  CHECK( U64( d ) == gcd( U64( s ) , U64( m ) ) && d == p * s + q * m );
  if( d == 1 ) {
   const LL r = modInverse( s , m );
   CHECK( r >= 0 && r < m && ( s * r ) % m == 1 % m );
   }
  const U64 us = U64( s ) , um = U64( m );
  CHECK( powMod( us , 3 , um ) == us % um * us % um * us % um );
  }
 for( U64 n = 0 ; n < 3000 ; ++n ) {
  CHECK( isPrime( n ) == isPrimeTrial( n ) );
  CHECK( millerRabin( n , 20 , g ) == isPrimeTrial( n ) );
  }
 CHECK_THROWS( (void) extendedEuclid( -1 , 5 ) , contract_violation );
 CHECK_THROWS( (void) modInverse( 6 , 9 ) , contract_violation );
 CHECK_THROWS( (void) modInverse( 3 , 0 ) , contract_violation );
 CHECK_THROWS( (void) mulMod( 3 , 4 , 0 ) , contract_violation );
 CHECK_THROWS( (void) powMod( 3 , 4 , 0 ) , contract_violation );
 CHECK_THROWS( (void) witness( 10 , 3 ) , contract_violation );
 CHECK_THROWS( (void) witness( 11 , 0 ) , contract_violation );
 CHECK_THROWS( (void) witness( 11 , 11 ) , contract_violation );
 CHECK_THROWS( (void) millerRabin( 11 , -1 , g ) , contract_violation );
 }

// the products of the four and of the three halves against the naive one
// on random degrees, each also checked by its audit
ADS_TEST( "polynomial" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Longs p( g() % 20 ) , q( g() % 20 );
  for( auto & x : p ) x = long( g() % 7 ) - 3;
  for( auto & x : q ) x = long( g() % 7 ) - 3;
  const auto r = multiplyNaive( p , q );
  CHECK( multiplyFour( p , q ) == r && multiplyKaratsuba( p , q ) == r );
  }
 }

// the keys of RSA undo each other on random messages; a number of bits out
// of [ 2 , 63 ] and a message not below N violate the contracts
ADS_TEST( "rsa" )
{
 std::mt19937_64 g( 7 );
 for( int t = 0 ; t < 5 ; ++t ) {
  const auto [ pub , priv ] = rsaKeys( g );
  for( int m = 0 ; m < 20 ; ++m ) {
   const U64 M = g() % pub.N;
   CHECK( rsa( rsa( M , pub ) , priv ) == M );
   }
  CHECK_THROWS( (void) rsa( pub.N , pub ) , contract_violation );
  }
 CHECK( randomPrime( 2 , g ) == 3 );
 CHECK_THROWS( (void) randomPrime( 1 , g ) , contract_violation );
 CHECK_THROWS( (void) randomPrime( 64 , g ) , contract_violation );
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
/*--------------------- End File contract_numeric.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
