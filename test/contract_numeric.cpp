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

#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/BinaryInteger.h"
#include "ADS/numeric/Catalan.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/numeric/FFT.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/MatrixChain.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/Polynomial.h"
#include "ADS/numeric/RSA.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"
#include "ADS/numeric/Rational.h"
#include "ADS/numeric/SparseMatrix.h"

#include <cstddef>
#include <cstdint>
#include <limits>
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
 CHECK( randomPrime( 63 , g ) >= ( U64( 1 ) << 62 ) );
 }

// the bounds of the other headers just inside and just outside: the
// combinatorics at their largest n, Catalan up to the last one in 64
// bits, the radix of BigInteger at 2 , 36 , 1 , 37, a string with a sign
// only or a digit beyond the radix, the division and the fraction by 0,
// the FFT of 0 , 1 , 3 points, the generators and the statistical tests on
// their degenerate inputs, n = N and n = N + 1 samples, the sparse matrix
// at row m and column n, a pivot on a 0, the recursive product of two
// empty or unequal polynomials
ADS_TEST( "other-numeric" )
{
 // binomial() recurs in 2^n calls: n = 67 with k near 0 and n only
 CHECK( binomial( 67 , 1 ) == 67 && binomial( 67 , 66 ) == 67 &&
        binomial( 0 , 0 ) == 1 && binomial( 67 , 67 ) == 1 );
 CHECK( tartaglia( 67 )[ 67 ][ 33 ] == 14226520737620288370ULL );
 CHECK_THROWS( (void) binomial( 68 , 1 ) , contract_violation );
 CHECK( tartaglia( 67 ).size() == 68 && tartaglia( 0 ).size() == 1 );
 CHECK_THROWS( (void) tartaglia( 68 ) , contract_violation );
 const FibonacciCache F;
 CHECK( fibonacci( 92 ) == fibonacciTable( 92 ) &&
        F.value( 92 ) == fibonacci( 92 ) && F.value( 0 ) == fibonacci( 0 ) );
 CHECK_THROWS( (void) fibonacci( 93 ) , contract_violation );
 CHECK_THROWS( (void) fibonacciTable( 93 ) , contract_violation );
 CHECK_THROWS( (void) F.value( 93 ) , contract_violation );
 CHECK_THROWS( (void) F.value( -1 ) , contract_violation );
 CHECK( factorial( 20 ) == 2432902008176640000ULL &&
        factorialRec( 20 ) == factorial( 20 ) && factorial( 0 ) == 1 );
 CHECK_THROWS( (void) factorial( 21 ) , contract_violation );
 CHECK_THROWS( (void) factorialRec( -1 ) , contract_violation );
 CHECK( binomialByFactorials( 20 , 10 ) == 184756 &&
        binomialByFactorials( 0 , 0 ) == 1 );
 CHECK_THROWS( (void) binomialByFactorials( 21 , 1 ) , contract_violation );
 CHECK_THROWS( (void) binomialByFactorials( 5 , 6 ) , contract_violation );
 CHECK_THROWS( (void) binomialByFactorials( 5 , -1 ) , contract_violation );
 CHECK( catalan( 0 ) == 1 && catalan( 1 ) == 1 && catalan( 5 ) == 42 &&
        catalan( 36 ) == 11959798385860453492ULL );
 CHECK_THROWS( (void) catalan( -1 ) , contract_violation );

 CHECK( BigInteger( "zz" , 36 ).toString() == "1295" &&
        BigInteger( "-101" , 2 ).toString( 36 ) == "-5" &&
        BigInteger( "+0" ).sign() == 0 && BigInteger( "-0" ).sign() == 0 );
 CHECK_THROWS( BigInteger( "1" , 1 ) , contract_violation );
 CHECK_THROWS( BigInteger( "1" , 37 ) , contract_violation );
 CHECK_THROWS( BigInteger( "" ) , contract_violation );
 CHECK_THROWS( BigInteger( "-" ) , contract_violation );
 CHECK_THROWS( BigInteger( "12" , 2 ) , contract_violation );
 CHECK_THROWS( BigInteger( "1 2" ) , contract_violation );
 CHECK_THROWS( (void) BigInteger( 5 ).toString( 1 ) , contract_violation );
 CHECK_THROWS( (void) BigInteger( 5 ).toString( 37 ) , contract_violation );
 CHECK_THROWS( (void) BigInteger::divMod( 5 , 0 ) , contract_violation );
 const BigInteger m( std::numeric_limits< long long >::min() );
 CHECK( m.toString() == "-9223372036854775808" &&
        ( -m ).toString() == "9223372036854775808" );
 const auto [ qt , rm ] = BigInteger::divMod( -7 , 2 );
 CHECK( qt * 2 + rm == BigInteger( -7 ) );
 CHECK_THROWS( Rational( 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) ( Rational( 1 , 2 ) / Rational( 0 ) ) ,
               contract_violation );
 CHECK( Rational( 0 , -5 ) == Rational( 0 ) &&
        Rational( 2 , -4 ) == Rational( -1 , 2 ) );

 std::vector< Complex > a0 , a1 = { 2.0 } , a3( 3 ) , a4( 4 , 1.0 );
 fft( a0 , false );
 fft( a1 , false );
 fft( a4 , false );
 CHECK( a0.empty() && a1[ 0 ] == 2.0 && std::abs( a4[ 0 ] - 4.0 ) < 1e-12 );
 CHECK_THROWS( fft( a3 , false ) , contract_violation );
 CHECK( convolve( {} , { 1.0 } ).empty() || convolve( {} , { 1.0 } ).size()
        <= 1 );

 CHECK_THROWS( LinearCongruential( 5 , 1 , 5 , 0 ) , contract_violation );
 CHECK_THROWS( LinearCongruential( 2 , 5 , 5 , 0 ) , contract_violation );
 CHECK_THROWS( LinearCongruential( 2 , 1 , 5 , 5 ) , contract_violation );
 (void) LinearCongruential( 4 , 4 , 5 , 4 );          // m - 1 everywhere
 (void) LinearCongruential( 9 , 9 , 0 , 9 );          // m = 0: 2^64
 CHECK_THROWS( (void) chiSquare( { 1 } , {} ) , contract_violation );
 CHECK_THROWS( (void) chiSquare( { 1 , 1 } , { 1.0 , 0.0 } ) ,
               contract_violation );
 CHECK( chiSquare( { 3 } , { 1.0 } ) == 0 );
 CHECK_THROWS( (void) kolmogorovSmirnov( {} ) , contract_violation );
 (void) kolmogorovSmirnov( { 0.5 } );
 CHECK_THROWS( (void) serialTest( { 0.1 , 0.2 } , 1 ) , contract_violation );
 CHECK_THROWS( (void) serialTest( { 0.1 } , 2 ) , contract_violation );
 (void) serialTest( { 0.1 , 0.7 } , 2 );
 CHECK_THROWS( (void) spectralTest2( 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) spectralTest2( 0 , 7 ) , contract_violation );
 CHECK_THROWS( (void) spectralTest2( 7 , 7 ) , contract_violation );
 (void) spectralTest2( 1 , 2 );
 std::mt19937 g( 9 );
 CHECK( selectionSampling( 3 , 3 , g ).size() == 3 &&
        selectionSampling( 0 , 0 , g ).empty() &&
        sampleBySwaps( 4 , 4 , g ).size() == 4 &&
        sampleByDictionary( 0 , 5 , g ).empty() &&
        sampleBySorting( 5 , 5 , g ).size() == 5 &&
        sampleBySkips( 5 , 5 , g ).size() == 5 &&
        heapSampling( std::vector< int >{ 1 , 2 } , 2 , g ).size() == 2 );
 CHECK_THROWS( (void) selectionSampling( 4 , 3 , g ) , contract_violation );
 CHECK_THROWS( (void) sampleBySwaps( 1 , 0 , g ) , contract_violation );
 CHECK_THROWS( (void) sampleByDictionary( 6 , 5 , g ) , contract_violation );
 CHECK_THROWS( (void) sampleBySorting( 6 , 5 , g ) , contract_violation );
 CHECK_THROWS( (void) sampleBySkips( 6 , 5 , g ) , contract_violation );
 CHECK_THROWS( (void) heapSampling( std::vector< int >{ 1 } , 2 , g ) ,
               contract_violation );

 SparseMatrix M( 2 , 3 ) , Z( 0 , 0 );
 M.set( 1 , 2 , 5.0 );
 M.set( 0 , 0 , 1.0 );
 CHECK( M.get( 1 , 2 ) == 5.0 && M.row( 1 ).size() == 1 &&
        M.col( 2 ).size() == 1 && M.nonzeros() == 2 );
 M.set( 1 , 2 , 0.0 );                        // a 0 removes the entry
 CHECK( M.nonzeros() == 1 && M.get( 1 , 2 ) == 0 );
 CHECK_THROWS( (void) M.get( 2 , 0 ) , contract_violation );
 CHECK_THROWS( (void) M.get( 0 , 3 ) , contract_violation );
 CHECK_THROWS( M.set( 2 , 0 , 1.0 ) , contract_violation );
 CHECK_THROWS( M.set( 0 , 3 , 1.0 ) , contract_violation );
 CHECK_THROWS( (void) M.row( 2 ) , contract_violation );
 CHECK_THROWS( (void) M.col( 3 ) , contract_violation );
 CHECK_THROWS( (void) M.rowEntries( 2 ) , contract_violation );
 CHECK_THROWS( (void) M.colEntries( 3 ) , contract_violation );
 CHECK_THROWS( M.pivot( 1 , 1 ) , contract_violation );     // a 0
 CHECK_THROWS( M.pivot( 2 , 0 ) , contract_violation );
 CHECK_THROWS( M.pivot( 0 , 3 ) , contract_violation );
 M.pivot( 0 , 0 );
 CHECK( M.get( 0 , 0 ) == 1.0 );
 CHECK_THROWS( (void) Z.get( 0 , 0 ) , contract_violation );

 long mults = 0;
 CHECK_THROWS( (void) detail::multiplyRec( Longs{} , Longs{} , true ,
                                           &mults ) , contract_violation );
 CHECK_THROWS( (void) detail::multiplyRec( Longs{ 1 } , Longs{ 1 , 2 } ,
                                           false , &mults ) ,
               contract_violation );
 CHECK( detail::multiplyRec( Longs{ 3 } , Longs{ 4 } , true , &mults ) ==
        Longs( { 12 } ) );
 CHECK( multiplyKaratsuba( Longs{} , Longs{ 1 } ).empty() ||
        multiplyKaratsuba( Longs{} , Longs{ 1 } ) == Longs( { 0 } ) );
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
