/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_numeric.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered numerics and geometry, beyond the book: the
 * binary gcd, the Montgomery form and the exact primality on 64 bits, the
 * generators and the bounded integers, the batched shuffle and the
 * reservoir of Algorithm L, the product of dense matrices by blocks, the
 * sparse matrices by rows, the convex hull of the octagon and of the
 * monotone chain and the closest pair by the sweep, checked against the
 * book's functions and the standard library on random and extreme inputs,
 * at the level 3 of the contracts.
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

#include "ADS/geometry/ClosestPair.h"
#include "ADS/geometry/ConvexHull.h"

#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"
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
 // the moduli below 2^32: the bases above the modulus, the exponents 0,
 // the powers of 2 and their neighbours
 for( const U64 n : { U64( 3 ) , U64( 4294967291ULL ) ,
                      ( U64( 1 ) << 32 ) - 1 , U64( 65537 ) } )
  for( const U64 x : { U64( 0 ) , U64( 1 ) , n - 1 , n , ~U64( 0 ) } )
   for( int k = 0 ; k < 64 ; ++k )
    for( const U64 v : { U64( 1 ) << k , ( U64( 1 ) << k ) - 1 ,
                         ( U64( 1 ) << k ) + 1 } ) {
     U64 r = 1 % n;
     for( U64 y = x % n , e = v ; e ; e >>= 1 , y = mulMod( y , y , n ) )
      if( e & 1 ) r = mulMod( r , y , n );
     CHECK( powMod( x , v , n ) == r );
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
 // the one base of a bucket below 2^32: the edges at 223^2 and at 2^32,
 // the squares and the products of two primes above 211, and a sweep
 for( const U64 e : { U64( 223 * 223 ) , U64( 1 ) << 32 } )
  for( U64 n = e - 3000 ; n < e + 3000 ; ++n )
   CHECK( isPrime( n ) == isPrimeBook( n ) );
 for( U64 p = 223 ; p < 65536 ; p += 2 )
  if( isPrimeTrial( p ) )
   for( const U64 q : { p , p + 2 , p + 6 } )
    if( p * q < ( U64( 1 ) << 32 ) ) CHECK( ! isPrime( p * q ) );
 for( U64 n = 1 ; n < ( U64( 1 ) << 32 ) ; n += 2 * 1009 + ( n & 2 ) )
  CHECK( isPrime( n ) == isPrimeBook( n ) );
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

// factorize(): 1, 2, the primes, the powers of 2 and of a prime, squares
// of large primes, products of two primes of 32 bits and of three of 21,
// Carmichael numbers, 2^64 - 1, and random words against the divisions
// for those small enough, the product and the primality of the factors
// for the others
ADS_TEST( "factorize" )
{
 using V = std::vector< U64 >;
 CHECK( factorize( 1 ).empty() && factorize( 2 ) == V( { 2 } ) &&
        factorize( 4 ) == V( { 2 , 2 } ) &&
        factorize( 1ULL << 63 ) == V( 63 , 2 ) &&
        factorize( 18446744073709551557ULL ) ==
        V( { 18446744073709551557ULL } ) );
 CHECK( factorize( ~U64( 0 ) ) == V( { 3 , 5 , 17 , 257 , 641 , 65537 ,
                                       6700417 } ) );
 CHECK( factorize( 4294967291ULL * 4294967291ULL ) ==
        V( { 4294967291ULL , 4294967291ULL } ) );
 CHECK( factorize( 4294967279ULL * 4294967291ULL ) ==
        V( { 4294967279ULL , 4294967291ULL } ) );
 CHECK( factorize( 561 ) == V( { 3 , 11 , 17 } ) &&
        factorize( 3215031751ULL ) == V( { 151 , 751 , 28351 } ) );
 U64 p3 = 1;
 for( int i = 0 ; i < 3 ; ++i ) p3 *= 2097143;   // a prime of 21 bits
 CHECK( factorize( p3 ) == V( 3 , 2097143 ) );
 CHECK( factorize( 257ULL * 257 * 65537 ) == V( { 257 , 257 , 65537 } ) );
 std::mt19937_64 g( 43 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const U64 n = 1 + ( g() >> ( g() % 64 ) );
  const V F = factorize( n );
  U64 prod = 1;
  bool primes = std::is_sorted( F.begin() , F.end() );
  for( const U64 p : F ) {
   prod *= p;
   primes = primes && isPrime( p );
   }
  CHECK( prod == n && primes );
  if( n < ( U64( 1 ) << 36 ) ) {            // against the divisions
   V D;
   U64 m = n;
   for( U64 d = 2 ; d * d <= m ; ++d )
    for( ; m % d == 0 ; m /= d ) D.push_back( d );
   if( m > 1 ) D.push_back( m );
   CHECK( D == F );
   }
  }
 for( int t = 0 ; t < 200 ; ++t ) {         // semiprimes of 32-bit primes
  U64 p , q;
  do p = ( g() >> 32 ) | 1; while( ! isPrime( p ) );
  do q = ( g() >> 32 ) | 1; while( ! isPrime( q ) );
  CHECK( factorize( p * q ) == ( p < q ? V( { p , q } ) : V( { q , p } ) ) );
  }
 CHECK_THROWS( (void) factorize( 0 ) , contract_violation );
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

// xoshiro256** and xoshiro256++ against the reference algorithms of
// Blackman and Vigna on the same state, the words of SplitMix64 of the
// seed; the alias method: the probabilities of the table against the
// weights, the chi-square of the draws, weights zero, a single item, a
// mass on one item, and the contracts
ADS_TEST( "xoshiro-alias" )
{
 for( std::uint64_t seed : { 0ULL , 1ULL , 123456789ULL } ) {
  std::uint64_t st[ 4 ] , x = seed;
  for( auto & w : st ) {
   w = splitmix64( x );
   x += 0x9e3779b97f4a7c15ULL;
   }
  std::uint64_t a[ 4 ] = { st[ 0 ] , st[ 1 ] , st[ 2 ] , st[ 3 ] } ,
                b[ 4 ] = { st[ 0 ] , st[ 1 ] , st[ 2 ] , st[ 3 ] };
  auto step = []( std::uint64_t * q ) {
   const std::uint64_t t = q[ 1 ] << 17;
   q[ 2 ] ^= q[ 0 ]; q[ 3 ] ^= q[ 1 ]; q[ 1 ] ^= q[ 2 ]; q[ 0 ] ^= q[ 3 ];
   q[ 2 ] ^= t; q[ 3 ] = std::rotl( q[ 3 ] , 45 ); };
  Xoshiro256 P( seed );
  Xoshiro256StarStar S( seed );
  for( int i = 0 ; i < 1000 ; ++i ) {
   CHECK( P() == std::rotl( a[ 0 ] + a[ 3 ] , 23 ) + a[ 0 ] );
   step( a );
   CHECK( S() == std::rotl( b[ 1 ] * 5 , 7 ) * 9 );
   step( b );
   }
  }
 Xoshiro256 g( 41 );
 for( const auto & w : std::vector< std::vector< double > >{
       { 1 } , { 0 , 5 , 0 } , { 1 , 1 , 1 , 1 } , { 1 , 2 , 3 , 4 , 0 , 10 } ,
       { 1e-9 , 1 , 1e9 } } ) {
  const AliasTable T( w );
  double sum = 0 , tot = 0;
  for( double x : w ) sum += x;
  for( std::size_t i = 0 ; i < w.size() ; ++i ) {
   CHECK( std::abs( T.probability( i ) - w[ i ] / sum ) < 1e-12 );
   tot += T.probability( i );
   }
  CHECK( std::abs( tot - 1 ) < 1e-12 );
  std::vector< long > Y( w.size() , 0 );
  const int draws = 200000;
  for( int t = 0 ; t < draws ; ++t ) ++Y[ T( g ) ];
  for( std::size_t i = 0 ; i < w.size() ; ++i )
   if( w[ i ] == 0 ) CHECK( Y[ i ] == 0 );
  std::vector< long > Z;                    // the chi-square on the cells
  std::vector< double > q;                  // of a positive weight
  for( std::size_t i = 0 ; i < w.size() ; ++i )
   if( w[ i ] / sum > 1e-3 ) {
    Z.push_back( Y[ i ] );
    q.push_back( w[ i ] / sum );
    }
  double qs = 0;
  for( double v : q ) qs += v;
  for( double & v : q ) v /= qs;
  if( Z.size() > 1 ) CHECK( chiSquare( Z , q ) < 30 );
  }
 std::vector< double > R( 1000 );           // random weights
 for( auto & x : R ) x = double( g() % 1000 );
 const AliasTable T( R );
 double tot = 0;
 for( std::size_t i = 0 ; i < R.size() ; ++i ) tot += T.probability( i );
 CHECK( std::abs( tot - 1 ) < 1e-9 );
 CHECK_THROWS( AliasTable( std::vector< double >() ) , contract_violation );
 CHECK_THROWS( AliasTable( { 0.0 , 0.0 } ) , contract_violation );
 CHECK_THROWS( AliasTable( { 1.0 , -1.0 } ) , contract_violation );
 }

// Philox4x32-10 and Philox4x64-10: the known answers of Random123 for the
// counters and keys 0, all ones and the digits of pi, the 10000-th output
// of the default engine of C++26, discard() across the buffer and the
// carries of the counter, the order of a counter set near its top
ADS_TEST( "philox" )
{
 using B32 = Philox4x32::Block;
 using B64 = Philox4x64::Block;
 CHECK( Philox4x32::block( B32{ 0 , 0 , 0 , 0 } , { 0 , 0 } ) ==
        B32( { 0x6627e8d5u , 0xe169c58du , 0xbc57ac4cu , 0x9b00dbd8u } ) );
 const std::uint32_t o = ~0u;
 CHECK( Philox4x32::block( B32{ o , o , o , o } , { o , o } ) ==
        B32( { 0x408f276du , 0x41c83b0eu , 0xa20bc7c6u , 0x6d5451fdu } ) );
 CHECK( Philox4x32::block( B32{ 0x243f6a88u , 0x85a308d3u , 0x13198a2eu ,
                                0x03707344u } ,
                           { 0xa4093822u , 0x299f31d0u } ) ==
        B32( { 0xd16cfe09u , 0x94fdccebu , 0x5001e420u , 0x24126ea1u } ) );
 CHECK( Philox4x64::block( B64{ 0 , 0 , 0 , 0 } , { 0 , 0 } ) ==
        B64( { 0x16554d9eca36314cull , 0xdb20fe9d672d0fdcull ,
               0xd7e772cee186176bull , 0x7e68b68aec7ba23bull } ) );
 const std::uint64_t O = ~0ull;
 CHECK( Philox4x64::block( B64{ O , O , O , O } , { O , O } ) ==
        B64( { 0x87b092c3013fe90bull , 0x438c3c67be8d0224ull ,
               0x9cc7d7c69cd777b6ull , 0xa09caebf594f0ba0ull } ) );
 CHECK( Philox4x64::block( B64{ 0x243f6a8885a308d3ull ,
                                0x13198a2e03707344ull ,
                                0xa4093822299f31d0ull ,
                                0x082efa98ec4e6c89ull } ,
                           { 0x452821e638d01377ull ,
                             0xbe5466cf34e90c6cull } ) ==
        B64( { 0xa528f45403e61d95ull , 0x38c72dbd566e9788ull ,
               0xa5a1610e72fd18b5ull , 0x57bd43b5e52b7fe6ull } ) );
 {
  Philox4x32 a;
  Philox4x64 b;
  std::uint32_t x = 0;
  std::uint64_t y = 0;
  for( int i = 0 ; i < 10000 ; ++i ) {
   x = a();
   y = b();
   }
  CHECK( x == 1955073260u && y == 3409172418970261260ull );
  }
 for( const unsigned long long z : { 0ull , 1ull , 3ull , 4ull , 15ull ,
                                     16ull , 17ull , 63ull , 1000ull ,
                                     12345ull } )
  for( int pre = 0 ; pre < 20 ; pre += 7 ) {
   Philox4x32 a( 7 ) , b( 7 );
   for( int i = 0 ; i < pre ; ++i ) ( void ) a() , ( void ) b();
   a.discard( z );
   for( unsigned long long i = 0 ; i < z ; ++i ) ( void ) b();
   CHECK( a() == b() && a() == b() );
   }
 {
  Philox4x32 a( 3 );                        // the counter carries from
  a.setCounter( { o , o , 5 , 0 } );        // word to word
  for( int i = 0 ; i < 4 ; ++i ) ( void ) a();
  const auto next = Philox4x32::block( { 0 , 0 , 6 , 0 } , { 3 , 0 } );
  CHECK( a() == next[ 0 ] && a() == next[ 1 ] );
  Philox4x32 c( 3 );
  c.setCounter( { o - 1 , o , 5 , 0 } );
  c.discard( 8 );
  CHECK( c() == next[ 0 ] );
  Philox4x64 d( 9 );
  d.setCounter( { O , O , O , O } );
  for( int i = 0 ; i < 4 ; ++i ) ( void ) d();
  CHECK( d() == Philox4x64::block( { 0 , 0 , 0 , 0 } , { 9 , 0 } )[ 0 ] );
  }
 }

// normalZiggurat() and exponentialZiggurat(): 4 million draws each, the
// first four moments, the distance of Kolmogorov from the exact
// distribution, the frequencies of the tails beyond the base strip (3.654
// and 7.697) and of the layers near the top, and finiteness
ADS_TEST( "ziggurat" )
{
 const int n = 4000000;
 Xoshiro256 g( 11 );
 std::vector< double > N( n ) , E( n );
 for( auto & x : N ) x = normalZiggurat( g );
 for( auto & x : E ) x = exponentialZiggurat( g );
 auto moments = []( const std::vector< double > & v ) {
  double m[ 5 ] = {};
  for( const double x : v ) {
   double p = 1;
   for( int k = 1 ; k <= 4 ; ++k ) m[ k ] += ( p *= x );
   }
  for( int k = 1 ; k <= 4 ; ++k ) m[ k ] /= double( v.size() );
  return( std::vector< double >( m + 1 , m + 5 ) ); };
 const auto mn = moments( N ) , me = moments( E );
 CHECK( std::fabs( mn[ 0 ] ) < 0.003 && std::fabs( mn[ 1 ] - 1 ) < 0.004 &&
        std::fabs( mn[ 2 ] ) < 0.01 && std::fabs( mn[ 3 ] - 3 ) < 0.03 );
 CHECK( std::fabs( me[ 0 ] - 1 ) < 0.003 && std::fabs( me[ 1 ] - 2 ) < 0.01 &&
        std::fabs( me[ 2 ] - 6 ) < 0.06 && std::fabs( me[ 3 ] - 24 ) < 0.6 );
 auto ks = []( std::vector< double > v , auto cdf ) {
  std::sort( v.begin() , v.end() );
  double d = 0;
  for( std::size_t i = 0 ; i < v.size() ; ++i ) {
   const double c = cdf( v[ i ] );
   d = std::max( { d , c - double( i ) / double( v.size() ) ,
                   double( i + 1 ) / double( v.size() ) - c } );
   }
  return( d ); };
 // 1.63 / sqrt( n ) is the level 1% of the distance of Kolmogorov
 const double level = 1.63 / std::sqrt( double( n ) );
 CHECK( ks( N , []( double x ) {
         return( 0.5 * std::erfc( -x / std::sqrt( 2.0 ) ) ); } ) < level );
 CHECK( ks( E , []( double x ) { return( -std::expm1( -x ) ); } ) < level );
 auto count = []( const std::vector< double > & v , auto in ) {
  return( double( std::count_if( v.begin() , v.end() , in ) ) ); };
 const double rn = 3.6541528853610088 , re = 7.69711747013104972;
 const double tn = count( N , [ & ]( double x ) { return( x > rn ); } ) ,
              tnExp = n * 0.5 * std::erfc( rn / std::sqrt( 2.0 ) ) ,
              te = count( E , [ & ]( double x ) { return( x > re ); } ) ,
              teExp = n * std::exp( -re );
 CHECK( std::fabs( tn - tnExp ) < 5 * std::sqrt( tnExp ) );   // 5 sigma
 CHECK( std::fabs( te - teExp ) < 5 * std::sqrt( teExp ) );
 const double near = count( N , []( double x ) {
                             return( std::fabs( x ) < 0.05 ); } ) ,
              nearExp = n * std::erf( 0.05 / std::sqrt( 2.0 ) );
 CHECK( std::fabs( near - nearExp ) < 5 * std::sqrt( nearExp ) );
 CHECK( std::all_of( N.begin() , N.end() , []( double x ) {
         return( std::isfinite( x ) ); } ) &&
        std::all_of( E.begin() , E.end() , []( double x ) {
         return( std::isfinite( x ) && x >= 0 ); } ) );
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

// convexHull() against graham() of the book: no point, one, two, equal
// points, collinear ones, a square with points on its sides, the extreme
// coordinates, random sets with many collinear and equal points on a small
// grid, uniform sets in a square, in a disk and near a circle (where the
// octagon drops most points, or none)
ADS_TEST( "convex-hull" )
{
 using Points = std::vector< Point >;
 CHECK( convexHull( {} ).empty() );
 CHECK( convexHull( Points( 7 , { 3 , -2 } ) ) == Points( { { 3 , -2 } } ) );
 CHECK( convexHull( { { 1 , 1 } , { 0 , 0 } } ) == graham( { { 1 , 1 } ,
                                                           { 0 , 0 } } ) );
 CHECK( convexHull( { { 0 , 0 } , { 1 , 1 } , { 2 , 2 } , { 3 , 3 } } ) ==
        Points( { { 0 , 0 } , { 3 , 3 } } ) );
 const Points sq = { { 0 , 0 } , { 2 , 0 } , { 4 , 0 } , { 4 , 2 } ,
                     { 4 , 4 } , { 2 , 4 } , { 0 , 4 } , { 0 , 2 } ,
                     { 2 , 2 } };
 CHECK( convexHull( sq ) == Points( { { 4 , 0 } , { 4 , 4 } , { 0 , 4 } ,
                                      { 0 , 0 } } ) );
 const long G = 1000000000L;
 const Points far = { { -G , -G } , { G , -G } , { G , G } , { -G , G } ,
                      { 0 , 0 } , { G , 0 } , { -G , G - 1 } };
 CHECK( convexHull( far ) == graham( far ) );
 CHECK_THROWS( (void) convexHull( { { G + 1 , 0 } , { 0 , 0 } } ) ,
               contract_violation );
 std::mt19937_64 g( 23 );
 for( int t = 0 ; t < 3000 ; ++t ) {        // small grids: ties of all kinds
  const long w = 1 + long( g() % 12 );
  Points P( 1 + g() % 60 );
  for( auto & p : P )
   p = Point{ long( g() % std::uint64_t( w ) ) - w / 2 ,
              long( g() % std::uint64_t( w ) ) - w / 2 };
  CHECK( convexHull( P ) == graham( P ) );
  }
 for( int c = 0 ; c < 3 ; ++c )             // square, disk, circle
  for( int t = 0 ; t < 20 ; ++t ) {
   Points P( 2000 );
   const double R = 1e9;
   std::uniform_real_distribution< double > U( -R , R );
   for( auto & p : P ) {
    double x = U( g ) , y = U( g );
    if( c == 1 ) while( x * x + y * y > R * R ) { x = U( g ); y = U( g ); }
    if( c == 2 ) {
     const double a = x / R * 3.14159265358979;
     x = R * std::cos( a );
     y = R * std::sin( a );
     }
    p = Point{ long( std::llround( x ) ) , long( std::llround( y ) ) };
    }
   CHECK( convexHull( P ) == graham( P ) );
   }
 }

// closestPairSweep() against closestPairBrute(): two points, equal points
// (the distance 0), points in a column and in a row, random sets on small
// grids, the extreme coordinates, and strips along x and along y long
// enough that the sweeps give up for closestPair()
ADS_TEST( "closest-pair-sweep" )
{
 using Points = std::vector< Point >;
 auto check = []( const Points & P ) {
  const auto [ ij , d ] = closestPairSweep( P );
  const auto [ ij2 , d2 ] = closestPairBrute( P );
  return( ( d == d2 ) && ( ij.first < ij.second ) &&
          ( ij.second < P.size() ) &&
          ( squaredDistance( P[ ij.first ] , P[ ij.second ] ) == d ) );
  };
 CHECK( check( { { 0 , 0 } , { 3 , 4 } } ) &&
        closestPairSweep( { { 0 , 0 } , { 3 , 4 } } ).second == 25 );
 CHECK( closestPairSweep( { { 5 , 5 } , { 1 , 1 } , { 5 , 5 } } ) ==
        std::pair( std::pair( std::size_t( 0 ) , std::size_t( 2 ) ) ,
                   std::int64_t( 0 ) ) );
 Points col , row;
 for( long i = 0 ; i < 500 ; ++i ) {
  col.push_back( { 7 , i * i } );
  row.push_back( { i * i , -7 } );
  }
 CHECK( check( col ) && check( row ) );
 const long G = 1000000000L;
 CHECK( check( { { -G , -G } , { G , G } , { G , -G } } ) );
 CHECK( closestPairSweep( { { -G , -G } , { G , G } } ).second ==
        std::int64_t( 8 ) * G * G );
 CHECK_THROWS( (void) closestPairSweep( { { 0 , 0 } } ) ,
               contract_violation );
 std::mt19937_64 g( 29 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  const long w = 2 + long( g() % 1000 );
  Points P( 2 + g() % 200 );
  for( auto & p : P ) {
   p = Point{ long( g() % std::uint64_t( w ) ) ,
              long( g() % std::uint64_t( w ) ) };
   if( t % 3 == 0 ) p.x %= 3;               // a few columns
   }
  CHECK( check( P ) );
  }
 for( int byY = 0 ; byY < 2 ; ++byY ) {     // strips: the fallbacks
  Points P( 6000 );
  for( auto & p : P ) {
   const long a = long( g() % 2000 ) , b = long( g() % 2000000000 ) - G;
   p = byY ? Point{ b , a } : Point{ a , b };
   }
  CHECK( check( P ) );
  }
 for( int t = 0 ; t < 1000 ; ++t ) {        // the grid of the fallback
  const long w = 2 + long( g() % 100000 );
  Points P( 2 + g() % 300 );
  for( auto & p : P ) p = Point{ long( g() % std::uint64_t( w ) ) - w / 2 ,
                                 long( g() % std::uint64_t( w ) ) - w / 2 };
  const auto [ ij , d ] = detail::closestGrid( P );
  CHECK( ( d == closestPairBrute( P ).second ) && ( ij.first < ij.second ) &&
         ( squaredDistance( P[ ij.first ] , P[ ij.second ] ) == d ) );
  }
 CHECK( detail::closestGrid( { { -G , -G } , { G , G } } ).second ==
        std::int64_t( 8 ) * G * G );
 Points D( 6000 );                          // a diagonal strip: both
 for( auto & p : D ) {                      // sweeps give up
  const long a = long( g() % 2000000000 ) - G;
  p = Point{ a / 2 , a / 2 + long( g() % 2000 ) };
  }
 CHECK( check( D ) );
 }

// gemm() against the naive product on every shape from 0 to beyond the
// blocks, with leading dimensions larger than the columns, alpha and beta,
// beta = 0 on a C of NaN (which must not be read), k = 0 and alpha = 0;
// on doubles, floats and integers (the portable kernel); multiply() and
// transpose() of DenseMatrix against the book's Matrix
ADS_TEST( "dense-gemm" )
{
 std::mt19937_64 g( 31 );
 auto run = [ & ]< class T >( T , std::size_t m , std::size_t n ,
                              std::size_t k , T alpha , T beta ,
                              bool nanC ) {
  const std::size_t lda = k + g() % 3 , ldb = n + g() % 3 ,
                    ldc = n + g() % 3;
  std::vector< T > A( m * lda + 1 ) , B( k * ldb + 1 ) , C( m * ldc + 1 );
  for( auto & x : A ) x = T( long( g() % 7 ) - 3 );
  for( auto & x : B ) x = T( long( g() % 7 ) - 3 );
  for( auto & x : C ) x = nanC ? std::numeric_limits< T >::quiet_NaN()
                               : T( long( g() % 7 ) - 3 );
  std::vector< T > R = C;                   // the naive product
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j ) {
    T s = 0;
    for( std::size_t p = 0 ; p < k ; ++p )
     s += A[ i * lda + p ] * B[ p * ldb + j ];
    R[ i * ldc + j ] = beta == T( 0 ) ? alpha * s
                                      : alpha * s + beta * C[ i * ldc + j ];
    }
  gemm( m , n , k , alpha , A.data() , lda , B.data() , ldb , beta ,
        C.data() , ldc );
  for( std::size_t i = 0 ; i < m ; ++i )    // small integers: exact
   for( std::size_t j = 0 ; j < n ; ++j )
    if( !( C[ i * ldc + j ] == R[ i * ldc + j ] ) ) return( false );
  return( true );
  };
 const std::size_t S[] = { 0 , 1 , 2 , 7 , 12 , 13 , 16 , 17 , 33 , 65 };
 for( std::size_t m : S )
  for( std::size_t n : S )
   for( std::size_t k : { 0 , 1 , 5 , 16 , 40 } ) {
    CHECK( run( 1.0 , m , n , k , 1.0 , 0.0 , true ) );
    CHECK( run( 1.0 , m , n , k , -2.0 , 3.0 , false ) );
    CHECK( run( 1.0f , m , n , k , 2.0f , -1.0f , false ) );
    CHECK( run( 1.0f , m , n , k , 1.0f , 0.0f , true ) );
    CHECK( run( 1L , m , n , k , 3L , 2L , false ) );
    }
 // beyond the blocks KC , MC and the panels, and alpha = 0
 CHECK( run( 1.0 , 250 , 70 , 300 , 1.0 , 1.0 , false ) );
 CHECK( run( 1.0 , 260 , 33 , 520 , -1.0 , 0.0 , true ) );
 CHECK( run( 1.0f , 37 , 4100 , 3 , 1.0f , 0.0f , false ) );
 CHECK( run( 1.0 , 9 , 9 , 9 , 0.0 , 2.0 , false ) );
 CHECK_THROWS( gemm( 2 , 3 , 4 , 1.0 , ( const double * ) nullptr , 3 ,
                     ( const double * ) nullptr , 3 , 0.0 ,
                     ( double * ) nullptr , 3 ) , contract_violation );
 // the variant of Winograd against the product, exact on small integers,
 // with cutoffs that make many levels and every parity of the dimensions
 for( int t = 0 ; t < 60 ; ++t ) {
  const std::size_t m = 1 + g() % 40 , k = 1 + g() % 40 , n = 1 + g() % 40;
  DenseMatrix< double > X( m , k ) , Y( k , n );
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < k ; ++j ) X( i , j ) = double( g() % 7 ) - 3;
  for( std::size_t i = 0 ; i < k ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j ) Y( i , j ) = double( g() % 7 ) - 3;
  CHECK( multiplyStrassen( X , Y , 2 + g() % 6 ) == multiply( X , Y ) );
  }
 CHECK( multiplyStrassen( DenseMatrix< double >( 0 , 3 ) ,
                          DenseMatrix< double >( 3 , 2 ) ).rows() == 0 );
 CHECK_THROWS( (void) multiplyStrassen( DenseMatrix< double >( 2 , 3 ) ,
                                        DenseMatrix< double >( 2 , 3 ) ) ,
               contract_violation );
 Matrix< double > A( 30 , std::vector< double >( 20 ) ) ,
                  B( 20 , std::vector< double >( 25 ) );
 for( auto & r : A ) for( auto & x : r ) x = double( g() % 9 );
 for( auto & r : B ) for( auto & x : r ) x = double( g() % 9 );
 const DenseMatrix< double > DA( A ) , DB( B );
 CHECK( multiply( DA , DB ).toMatrix() == multiply( A , B ) );
 CHECK( transpose( DA ).toMatrix() == transpose( A ) );
 CHECK( transpose( transpose( DA ) ) == DA );
 CHECK( transpose( DenseMatrix< double >( 0 , 5 ) ).rows() == 5 );
 DenseMatrix< int > E( 2 , 3 , 7 );
 CHECK( E( 1 , 2 ) == 7 && E.rows() == 2 && E.cols() == 3 );
 CHECK_THROWS( (void) E( 2 , 0 ) , contract_violation );
 CHECK_THROWS( (void) multiply( DA , DA ) , contract_violation );
 CHECK_THROWS( DenseMatrix< double >( Matrix< double >( { { 1 , 2 } ,
                                                          { 3 } } ) ) ,
               contract_violation );
 }

// CsrMatrix against a dense reference: triplets in random order with
// equal places summed and empty rows, the zero matrix, a matrix of one
// row and one of one column, the product (rows long enough for the
// gathers and short ones), the transpose and its transpose, the
// conversion from the book's SparseMatrix
ADS_TEST( "csr-matrix" )
{
 std::mt19937_64 g( 37 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t m = g() % 40 , n = 1 + g() % 40;
  std::vector< std::vector< double > > D( m , std::vector< double >( n ) );
  std::vector< CsrMatrix::Triplet > T;
  const std::size_t z = g() % ( 1 + 3 * m * n );
  for( std::size_t e = 0 ; e < z && m ; ++e ) {
   const std::size_t i = g() % m , j = g() % n;
   const double v = double( long( g() % 9 ) - 4 );
   T.emplace_back( i , j , v );
   D[ i ][ j ] += v;
   }
  const CsrMatrix A( m , n , T );
  CHECK( A.rows() == m && A.cols() == n );
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    CHECK( A.get( i , j ) == D[ i ][ j ] );
  std::vector< double > x( n );
  for( auto & v : x ) v = double( long( g() % 9 ) - 4 );
  const auto y = A.multiply( x );
  for( std::size_t i = 0 ; i < m ; ++i ) {
   double s = 0;
   for( std::size_t j = 0 ; j < n ; ++j ) s += D[ i ][ j ] * x[ j ];
   CHECK( y[ i ] == s );                    // small integers: exact
   }
  const CsrMatrix Tr = A.transpose();
  CHECK( Tr.rows() == n && Tr.cols() == m && Tr.transpose() == A );
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    CHECK( Tr.get( j , i ) == D[ i ][ j ] );
  }
 const CsrMatrix Z( 3 , 4 );
 CHECK( Z.nonzeros() == 0 && Z.multiply( { 1 , 2 , 3 , 4 } ) ==
        std::vector< double >( 3 , 0.0 ) && Z.transpose().rows() == 4 );
 SparseMatrix S( 5 , 6 );
 S.set( 0 , 5 , 2 );
 S.set( 4 , 0 , -3 );
 S.set( 2 , 2 , 7 );
 const CsrMatrix C( S );
 CHECK( C.get( 0 , 5 ) == 2 && C.get( 4 , 0 ) == -3 && C.get( 2 , 2 ) == 7 &&
        C.nonzeros() == 3 && C.get( 1 , 1 ) == 0 );
 CHECK_THROWS( CsrMatrix( 2 , 2 , { { 2 , 0 , 1.0 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) C.multiply( { 1.0 } ) , contract_violation );
 CHECK_THROWS( (void) C.get( 5 , 0 ) , contract_violation );
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
