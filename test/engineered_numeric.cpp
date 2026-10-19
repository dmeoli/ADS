/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_numeric.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered numerics and geometry, beyond the book: the
 * binary gcd, the Montgomery form and the exact primality on 64 bits, the
 * generators and the bounded integers, the batched shuffle and the
 * reservoir of Algorithm L, checked against the book's functions and the
 * standard library on random and extreme inputs, at the level 3 of the
 * contracts.
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

#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// the primality by the 12 prime bases up to 37, the test of the book
bool isPrimeBook( U64 n )
{
 if( n < 2 ) return( false );
 for( U64 p : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } ) {
  if( n == p ) return( true );
  if( n % p == 0 ) return( false );
  }
 for( U64 b : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } )
  if( witness( n , b ) ) return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the binary gcd against std::gcd: zeros, equal, powers of 2, the largest
// words, consecutive numbers of Fibonacci and random pairs with a common
// factor
ADS_TEST( "binary-gcd" )
{
 const U64 M = ~U64( 0 );
 CHECK( binaryGcd( 0 , 0 ) == 0 && binaryGcd( 0 , 7 ) == 7 &&
        binaryGcd( 7 , 0 ) == 7 && binaryGcd( M , M ) == M &&
        binaryGcd( M , 0 ) == M && binaryGcd( 1 , M ) == 1 );
 CHECK( binaryGcd( U64( 1 ) << 63 , U64( 1 ) << 40 ) == U64( 1 ) << 40 );
 CHECK( binaryGcd( M , M - 1 ) == 1 && binaryGcd( M - 1 , M - 3 ) == 2 );
 static_assert( binaryGcd( 12 , 18 ) == 6 );
 U64 f0 = 1 , f1 = 2;
 while( f1 < ( U64( 1 ) << 62 ) ) {
  CHECK( binaryGcd( f1 , f0 ) == 1 );
  f1 += f0;
  f0 = f1 - f0;
  }
 std::mt19937_64 g( 1 );
 for( int t = 0 ; t < 200000 ; ++t ) {
  U64 a = g() >> ( g() % 64 ) , b = g() >> ( g() % 64 );
  if( t % 3 == 0 ) {                        // a common factor
   const U64 c = 1 + ( g() >> 44 );
   a = ( a >> 21 ) * c;
   b = ( b >> 21 ) * c;
   }
  if( t % 5 == 0 ) a <<= g() % 8;
  CHECK( binaryGcd( a , b ) == std::gcd( a , b ) &&
         binaryGcd( a , b ) == gcd( a , b ) );
  }
 }

// the Montgomery form against mulMod() and powMod() of the book: moduli 1,
// 3, the largest odd words, 2^61 - 1, random odd moduli of every size, the
// operands 0, 1 and n - 1; the contract of an even modulus
ADS_TEST( "montgomery" )
{
 std::mt19937_64 g( 2 );
 std::vector< U64 > N = { 1 , 3 , 5 , 7 , kMersenne61 , ~U64( 0 ) ,
                          ~U64( 0 ) - 2 , ( U64( 1 ) << 63 ) + 1 ,
                          ( U64( 1 ) << 32 ) + 1 , ( U64( 1 ) << 32 ) - 1 };
 for( int t = 0 ; t < 2000 ; ++t ) N.push_back( ( g() >> ( g() % 63 ) ) | 1 );
 for( const U64 n : N ) {
  const Montgomery64 M( n );
  CHECK( M.modulus() == n && M.from( M.one() ) == 1 % n );
  std::vector< U64 > X = { 0 , 1 % n , n - 1 , n / 2 };
  for( int i = 0 ; i < 20 ; ++i ) X.push_back( g() % n );
  for( const U64 a : X ) {
   CHECK( M.from( M.to( a ) ) == a && M.to( a ) < n );
   for( const U64 b : X ) {
    CHECK( M.from( M.mul( M.to( a ) , M.to( b ) ) ) == mulMod( a , b , n ) );
    }
   const U64 v = g() >> ( g() % 64 );
   U64 r = 1 % n;                           // by the doubling of Ahmes
   for( U64 x = a , e = v ; e ; e >>= 1 , x = mulMod( x , x , n ) )
    if( e & 1 ) r = mulMod( r , x , n );
   CHECK( M.from( M.pow( M.to( a ) , v ) ) == r && powMod( a , v , n ) == r );
   }
  CHECK( M.to( ~U64( 0 ) ) == M.to( ~U64( 0 ) % n ) );
  }
 CHECK( powMod( 0 , 0 , 7 ) == 1 && powMod( 3 , 5 , 1 ) == 0 &&
        powMod( 2 , 64 , 18446744073709551557ull ) == 59 &&
        powMod( 3 , 4 , 10 ) == 1 );        // an even modulus
 CHECK_THROWS( Montgomery64( 10 ) , contract_violation );
 CHECK_THROWS( Montgomery64( 0 ) , contract_violation );
 const Montgomery64 M( 11 );
 CHECK_THROWS( (void) M.mul( 11 , 1 ) , contract_violation );
 CHECK_THROWS( (void) M.pow( 12 , 1 ) , contract_violation );
 }

// isPrime() against the divisions up to 10^6 and on the edges of each set
// of bases, against the 12 bases of the book on random words of every
// size, on products of two primes near 2^32 and on the strong
// pseudoprimes to the most bases known
ADS_TEST( "is-prime" )
{
 for( U64 n = 0 ; n < 1000000 ; ++n )
  CHECK( isPrime( n ) == isPrimeTrial( n ) );
 std::mt19937_64 g( 3 );
 const U64 edges[] = { 341531ULL , 1050535501ULL , 350269456337ULL ,
                       55245642489451ULL , 7999252175582851ULL ,
                       585226005592931977ULL };
 for( const U64 e : edges )
  for( U64 n = e - 3000 ; n < e + 3000 ; ++n )
   CHECK( isPrime( n ) == isPrimeBook( n ) );
 for( int t = 0 ; t < 300000 ; ++t ) {
  const U64 n = g() >> ( g() % 64 );
  CHECK( isPrime( n ) == isPrimeBook( n ) );
  }
 for( U64 n = ~U64( 0 ) ; n > ~U64( 0 ) - 3000 ; --n )
  CHECK( isPrime( n ) == isPrimeBook( n ) );
 CHECK( isPrime( 18446744073709551557ULL ) && ! isPrime( ~U64( 0 ) ) );
 const U64 P[] = { 4294967291ULL , 4294967279ULL , 4294967231ULL ,
                   4294967197ULL , 65521 , 65519 , 1000003 , 999983 };
 for( const U64 p : P )
  for( const U64 q : P ) CHECK( ! isPrime( p * q ) && isPrime( p ) );
 // strong pseudoprimes to 2 , to 2 and 3 , ... , to the primes up to
 // 23 (Jaeschke), and Carmichael numbers, all composite
 const U64 S[] = { 2047 , 1373653 , 25326001 , 3215031751ULL ,
                   2152302898747ULL , 3474749660383ULL ,
                   341550071728321ULL , 3825123056546413051ULL , 561 ,
                   1105 , 1729 , 41041 , 825265 , 321197185 };
 for( const U64 n : S ) CHECK( ! isPrime( n ) );
 for( const U64 n : S ) CHECK( isPrime( n ) == isPrimeBook( n ) );
 }

// the generators: the first words of the reference implementations, the
// jump, equality and the use by the distributions of std
ADS_TEST( "generators" )
{
 static_assert( FullWordGenerator< Xoshiro256 > &&
                FullWordGenerator< WyRand > &&
                FullWordGenerator< std::mt19937_64 > &&
                ! FullWordGenerator< std::mt19937 > &&
                ! FullWordGenerator< std::minstd_rand > );
 Xoshiro256 x( 0 ) , y( 0 ) , z( 1 );
 CHECK( x == y && !( x == z ) );
 const U64 a = x();
 CHECK( a == y() && !( a == z() ) );
 Xoshiro256 j = x , k = x;
 j.jump();
 k.jump();
 CHECK( j == k && !( j == x ) && j() == k() );
 WyRand w( 0 ) , v( 0 );
 CHECK( w() == v() && w == v );
 // the bits are balanced and the words do not repeat soon
 for( int gen = 0 ; gen < 2 ; ++gen ) {
  Xoshiro256 X( 7 );
  WyRand W( 7 );
  std::set< U64 > seen;
  long ones = 0;
  for( int i = 0 ; i < 100000 ; ++i ) {
   const U64 r = gen ? W() : X();
   seen.insert( r );
   ones += std::popcount( r );
   }
  CHECK( seen.size() == 100000 &&
         std::abs( double( ones ) - 3.2e6 ) < 6 * std::sqrt( 1.6e6 ) );
  }
 std::uniform_real_distribution< double > U( 0 , 1 );
 Xoshiro256 X( 9 );
 double s = 0;
 for( int i = 0 ; i < 100000 ; ++i ) s += U( X );
 CHECK( std::abs( s / 100000 - 0.5 ) < 0.01 );
 }

// uniformBelow() and uniformReal(): the bounds 1, 2, 3, 2^63 + 1 and
// 2^64 - 1 (rejections half of the times), the chi-square of small
// bounds, the contract of the bound 0
ADS_TEST( "bounded-integers" )
{
 Xoshiro256 g( 11 );
 for( int i = 0 ; i < 1000 ; ++i ) {
  CHECK( uniformBelow( g , 1 ) == 0 && uniformBelow( g , 2 ) < 2 );
  CHECK( uniformBelow( g , ( U64( 1 ) << 63 ) + 1 ) <=
         ( U64( 1 ) << 63 ) );
  CHECK( uniformBelow( g , ~U64( 0 ) ) < ~U64( 0 ) );
  const double u = uniformReal( g );
  CHECK( u >= 0 && u < 1 );
  }
 for( const U64 s : { 3ULL , 7ULL , 10ULL , 64ULL } ) {
  std::vector< long > Y( s , 0 );
  for( int i = 0 ; i < 70000 ; ++i ) ++Y[ uniformBelow( g , s ) ];
  // V below the 99.9% quantile of chi-square with s - 1 <= 63 degrees
  CHECK( chiSquare( Y , std::vector< double >( s , 1.0 / double( s ) ) ) <
         110 );
  }
 // the bound 2^63 + 1 rejects about half of the words: the upper half of
 // the range must be as likely as the lower one
 long upper = 0;
 for( int i = 0 ; i < 20000 ; ++i )
  upper += uniformBelow( g , ( U64( 1 ) << 63 ) + 1 ) >= ( U64( 1 ) << 62 );
 CHECK( std::abs( upper - 10000 ) < 400 );
 CHECK_THROWS( (void) uniformBelow( g , 0 ) , contract_violation );
 }

// shuffleFast(): every permutation of 3 and of 4 items about equally
// likely, the items kept on sizes across the boundaries of the batches,
// the empty range and a single item, and a shuffle of a std::deque
ADS_TEST( "shuffle-fast" )
{
 Xoshiro256 g( 13 );
 for( int k = 3 ; k <= 4 ; ++k ) {
  std::map< std::vector< int > , long > C;
  const int T = 240000;
  for( int t = 0 ; t < T ; ++t ) {
   std::vector< int > A( k );
   std::iota( A.begin() , A.end() , 0 );
   shuffleFast( A.begin() , A.end() , g );
   ++C[ A ];
   }
  const long perms = k == 3 ? 6 : 24;
  CHECK( long( C.size() ) == perms );
  std::vector< long > Y;
  for( const auto & c : C ) Y.push_back( c.second );
  CHECK( chiSquare( Y , std::vector< double >( perms , 1.0 / perms ) ) <
         ( k == 3 ? 21 : 50 ) );
  }
 for( std::size_t n : { 0 , 1 , 2 , 6 , 7 , 13 , 1023 , 1024 , 1025 , 1030 ,
                        5000 } ) {
  std::vector< std::size_t > A( n );
  std::iota( A.begin() , A.end() , std::size_t( 0 ) );
  shuffleFast( A.begin() , A.end() , g );
  std::vector< std::size_t > B = A;
  std::sort( B.begin() , B.end() );
  for( std::size_t i = 0 ; i < n ; ++i ) CHECK( B[ i ] == i );
  if( n >= 1000 ) {                         // the last item moved anywhere
   std::size_t fixed = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) fixed += A[ i ] == i;
   CHECK( fixed < 10 );
   }
  }
 // the position of the item 0 after a shuffle of 1100 items, across the
 // batches of 2 and of 6, uniform
 std::vector< long > Y( 11 , 0 );
 for( int t = 0 ; t < 22000 ; ++t ) {
  std::vector< int > A( 1100 );
  std::iota( A.begin() , A.end() , 0 );
  shuffleFast( A.begin() , A.end() , g );
  ++Y[ std::size_t( std::find( A.begin() , A.end() , 0 ) - A.begin() ) /
       100 ];
  }
 CHECK( chiSquare( Y , std::vector< double >( 11 , 1.0 / 11 ) ) < 30 );
 std::vector< int > D = { 5 };
 shuffleFast( D.begin() , D.end() , g );
 CHECK( D[ 0 ] == 5 );
 }

// reservoirSampleL(): a short stream, the sample of 0, a stream as long as
// the sample, the inclusion of each item with probability n / N, on a
// random access range and on a std::list (no jump)
ADS_TEST( "reservoir-l" )
{
 Xoshiro256 g( 17 );
 std::vector< int > S( 10 );
 std::iota( S.begin() , S.end() , 0 );
 CHECK( reservoirSampleL( S.begin() , S.end() , 0 , g ).empty() );
 CHECK( reservoirSampleL( S.begin() , S.end() , 10 , g ) == S );
 CHECK( reservoirSampleL( S.begin() , S.end() , 50 , g ) == S );
 CHECK( reservoirSampleL( S.begin() , S.begin() , 3 , g ).empty() );
 const std::size_t N = 200 , n = 10;
 std::vector< int > V( N );
 std::iota( V.begin() , V.end() , 0 );
 const std::list< int > L( V.begin() , V.end() );
 for( int kind = 0 ; kind < 2 ; ++kind ) {
  std::vector< long > Y( N , 0 );
  const int T = 40000;
  for( int t = 0 ; t < T ; ++t ) {
   const auto R = kind ? reservoirSampleL( L.begin() , L.end() , n , g )
                       : reservoirSampleL( V.begin() , V.end() , n , g );
   CHECK( R.size() == n );
   std::set< int > D( R.begin() , R.end() );
   CHECK( D.size() == n && *D.rbegin() < int( N ) );
   for( int x : R ) ++Y[ std::size_t( x ) ];
   }
  // each count binomial( T , n / N ): the chi-square of the N counts
  CHECK( chiSquare( Y , std::vector< double >( N , 1.0 / N ) ) < 290 );
  }
 // a long stream: the largest item seen often, the sample spread
 std::vector< int > Big( 1000000 );
 std::iota( Big.begin() , Big.end() , 0 );
 long last = 0;
 for( int t = 0 ; t < 2000 ; ++t ) {
  const auto R = reservoirSampleL( Big.begin() , Big.end() , 100 , g );
  last += std::count_if( R.begin() , R.end() , []( int x ) {
   return( x >= 900000 ); } );
  }
 CHECK( std::abs( last - 20000 ) < 800 );   // 10% of the 100 items
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
/*--------------------- End File engineered_numeric.cpp --------------------*/
/*--------------------------------------------------------------------------*/
