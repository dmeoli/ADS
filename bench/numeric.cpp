/*--------------------------------------------------------------------------*/
/*---------------------------- File numeric.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the arithmetic on machine words and of the random
 * numbers:
 *
 * - "gcd", n pairs of random words of 64 bits (case random) and of
 *   consecutive numbers of Fibonacci times a random factor, the worst case
 *   of Euclid (case fibonacci): std::gcd, boost::integer::gcd, ADS::gcd and
 *   ADS::binaryGcd;
 * - "powmod", n powers \f$ x^v \bmod m \f$ of random words, the modulus odd
 *   of 63 bits, odd of 32 bits, or the prime \f$ 2^{61} - 1 \f$:
 *   boost::multiprecision::powm on words, mpz_powm of GMP on numbers of
 *   one limb (ADS_HAS_GMP), the Montgomery form of hurchalla
 *   (ADS_HAS_HURCHALLA) and ADS::powMod;
 * - "isprime", n tests of primality of random odd words of 64 bits, of
 *   random primes of 64 bits and of random odd words of 32 bits:
 *   boost::multiprecision::miller_rabin_test with 25 bases, GMP's
 *   mpz_probab_prime_p with 24 (the test of Baillie and PSW alone, exact
 *   on 64 bits), hurchalla::is_prime and ADS::isPrime;
 * - "factorize", n / 100 factorizations of random odd words and of
 *   products of two random primes of 32 bits (the hardest words for the
 *   rho of Pollard): hurchalla::factorize (ADS_HAS_HURCHALLA) and
 *   ADS::factorize;
 * - "generators", the sum of n words: std::mt19937_64, std::minstd_rand,
 *   boost::random::mt19937_64 and taus88, pcg64 and pcg32 of O'Neill
 *   (ADS_HAS_PCG), Philox4x32-10 and Philox4x64-10 of Random123
 *   (ADS_HAS_RANDOM123, r123::Engine), and the generators of ADS
 *   (xoshiro256++, xoshiro256**, wyrand, Philox4x32 and Philox4x64), the
 *   words of 32 bits counted as words;
 * - "normal" and "exponential", the sum of n deviates of N( 0 , 1 ) and of
 *   mean 1 on the same generator (xoshiro256++): std::normal_distribution
 *   (the polar method of Marsaglia) and std::exponential_distribution (a
 *   logarithm), the ziggurats of boost::random, ADS::normalZiggurat and
 *   ADS::exponentialZiggurat;
 * - "bounded", n integers uniform in [ 0 , s ), s random below 2^32:
 *   std::uniform_int_distribution and boost::random's on the same
 *   generator (xoshiro256++), and ADS::uniformBelow;
 * - "shuffle", a shuffle of n items: std::shuffle and ADS::shuffleFast on
 *   the same generator, ADS::shuffle of Knuth, std::shuffle on mt19937_64;
 * - "discrete", n draws from 100 and from 10^5 categories of random
 *   weights: std::discrete_distribution (the search in the cumulative
 *   sums), boost::random::discrete_distribution (an alias table) and
 *   ADS::AliasTable, on the same generator;
 * - "sampling", a sample of 1000 items of a stream of n: std::sample on a
 *   forward range (the selection of Knuth) and on an input range (the
 *   reservoir), ADS::reservoirSampling, ADS::reservoirSampleL and
 *   ADS::sampleBySkips (the positions, N known).
 *
 * Each cell is the median of r runs, in nanoseconds per operation (an item
 * of the stream for the sampling); the contracts are off (ADS_CONTRACTS=0).
 * ADS_BENCH_OLD leaves out what the release before this benchmark lacks,
 * so that the same file measures it.
 *
 *   numeric [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"
#ifndef ADS_BENCH_OLD
 #include "ADS/numeric/RandomGenerators.h"
#endif

#if __has_include( <boost/integer/common_factor_rt.hpp> )
 #include <boost/integer/common_factor_rt.hpp>
 #include <boost/multiprecision/integer.hpp>
 #include <boost/multiprecision/miller_rabin.hpp>
 #include <boost/random/mersenne_twister.hpp>
 #include <boost/random/taus88.hpp>
 #include <boost/random/discrete_distribution.hpp>
 #include <boost/random/uniform_int_distribution.hpp>
 #include <boost/random/normal_distribution.hpp>
 #include <boost/random/exponential_distribution.hpp>
 #define ADS_HAS_BOOST 1
#endif

#ifdef ADS_HAS_GMP
 #include <gmp.h>
#endif

#ifdef ADS_HAS_HURCHALLA
 #include "hurchalla/factoring/factorize.h"
 #include "hurchalla/factoring/is_prime.h"
 #include "hurchalla/modular_arithmetic/modular_pow.h"
 #include "hurchalla/montgomery_arithmetic/MontgomeryForm.h"
#endif

#ifdef ADS_HAS_RANDOM123
 #include <Random123/philox.h>
 #include <Random123/conventional/Engine.hpp>
#endif
#ifdef ADS_HAS_PCG
 #include "pcg_random.hpp"
#endif

#include <iterator>
#include <numeric>

using namespace ADS::bench;
using U64 = std::uint64_t;

namespace {

#ifdef ADS_BENCH_OLD
using Gen = std::mt19937_64;                // xoshiro is not there
#else
using Gen = ADS::Xoshiro256;
#endif

/// an input iterator over a vector, which std::sample cannot jump
struct InputIt {
 using iterator_category = std::input_iterator_tag;
 using value_type = U64;
 using difference_type = std::ptrdiff_t;
 using pointer = const U64 *;
 using reference = const U64 &;
 const U64 * p;
 reference operator*( void ) const { return( *p ); }
 InputIt & operator++( void ) { ++p; return( *this ); }
 InputIt operator++( int ) { InputIt t = *this; ++p; return( t ); }
 bool operator==( const InputIt & o ) const { return( p == o.p ); }
 };

/// a cell of n operations op( i ), i = 0 , ... , n - 1
template< class F >
void cell( const char * bench , const char * what , std::size_t n ,
           const std::string & who , int r , F && op )
{
 print( bench , what , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          U64 s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s += U64( op( i ) );
          sink = sink + s; } , r , n ) );
 }

void gcds( std::size_t n , unsigned seed , int r )
{
 std::mt19937_64 g( seed );
 std::vector< U64 > A( n ) , B( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  A[ i ] = g();
  B[ i ] = g();
  }
 std::vector< U64 > F = { 1 , 2 };          // the pairs of Fibonacci, the
 while( F.back() < ( U64( 1 ) << 62 ) )     // largest first
  F.push_back( F[ F.size() - 1 ] + F[ F.size() - 2 ] );
 std::vector< U64 > FA( n ) , FB( n );      // times a random factor, so
 for( std::size_t i = 0 ; i < n ; ++i ) {   // that no two pairs repeat
  const std::size_t k = 40 + g() % ( F.size() - 41 );
  const U64 m = 1 + g() % ( ( U64( 1 ) << 62 ) / F[ k ] );
  FA[ i ] = F[ k ] * m;
  FB[ i ] = F[ k - 1 ] * m;
  }
 for( int c = 0 ; c < 2 ; ++c ) {
  const char * what = c ? "fibonacci" : "random";
  const auto & X = c ? FA : A;
  const auto & Y = c ? FB : B;
  for( const char * who : { "std::gcd" , "std::gcd A/A" } )
   cell( "gcd" , what , n , who , r , [ & ]( std::size_t i ) {
    return( std::gcd( X[ i ] , Y[ i ] ) ); } );
#ifdef ADS_HAS_BOOST
  cell( "gcd" , what , n , "boost::integer::gcd" , r , [ & ]( std::size_t i ) {
   return( boost::integer::gcd( X[ i ] , Y[ i ] ) ); } );
#endif
  cell( "gcd" , what , n , "ADS::gcd" , r , [ & ]( std::size_t i ) {
   return( ADS::gcd( X[ i ] , Y[ i ] ) ); } );
#ifndef ADS_BENCH_OLD
  cell( "gcd" , what , n , "ADS::binaryGcd" , r , [ & ]( std::size_t i ) {
   return( ADS::binaryGcd( X[ i ] , Y[ i ] ) ); } );
#endif
  }
 }

void powers( std::size_t n , unsigned seed , int r )
{
 std::mt19937_64 g( seed );
 for( int c = 0 ; c < 3 ; ++c ) {
  const char * what = c == 0 ? "odd63" : c == 1 ? "odd32" : "mersenne61";
  std::vector< U64 > X( n ) , V( n ) , M( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   M[ i ] = c == 0 ? ( g() >> 1 ) | 1 : c == 1 ? ( g() >> 32 ) | 1
                                               : ADS::kMersenne61;
   X[ i ] = g() % M[ i ];
   V[ i ] = g();
   }
#ifdef ADS_HAS_BOOST
  for( const char * who : { "boost::multiprecision::powm" ,
                            "boost::multiprecision::powm A/A" } )
   cell( "powmod" , what , n , who , r , [ & ]( std::size_t i ) {
    return( boost::multiprecision::powm( X[ i ] , V[ i ] , M[ i ] ) ); } );
#endif
#ifdef ADS_HAS_GMP
  {
   mpz_t x , v , m , y;
   mpz_inits( x , v , m , y , nullptr );
   cell( "powmod" , what , n , "GMP mpz_powm" , r , [ & ]( std::size_t i ) {
    mpz_set_ui( x , X[ i ] );
    mpz_set_ui( v , V[ i ] );
    mpz_set_ui( m , M[ i ] );
    mpz_powm( y , x , v , m );
    return( mpz_get_ui( y ) ); } );
   mpz_clears( x , v , m , y , nullptr );
   }
#endif
#ifdef ADS_HAS_HURCHALLA
  cell( "powmod" , what , n , "hurchalla MontgomeryForm" , r ,
        [ & ]( std::size_t i ) {
   const hurchalla::MontgomeryForm< U64 > mf( M[ i ] );
   return( mf.convertOut( mf.pow( mf.convertIn( X[ i ] ) , V[ i ] ) ) ); } );
#endif
  cell( "powmod" , what , n , "ADS::powMod" , r , [ & ]( std::size_t i ) {
   return( ADS::powMod( X[ i ] , V[ i ] , M[ i ] ) ); } );
  }
 }

void primality( std::size_t n , unsigned seed , int r )
{
 std::mt19937_64 g( seed );
 for( int c = 0 ; c < 3 ; ++c ) {
  const char * what = c == 0 ? "random64" : c == 1 ? "primes64"
                                                   : "random32";
  std::vector< U64 > N( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   if( c == 1 )                             // a cycle of 64 primes
    N[ i ] = i < 64 ? [ & ]() {
     for( ; ; )
      if( const U64 p = g() | 1 ; ADS::isPrime( p ) ) return( p ); }()
                    : N[ i - 64 ];
   else
    N[ i ] = c == 0 ? g() | 1 : ( g() >> 32 ) | 1;
#ifdef ADS_HAS_BOOST
  for( const char * who : { "boost::multiprecision::miller_rabin_test" ,
                            "boost::multiprecision::miller_rabin_test A/A" } )
   cell( "isprime" , what , n , who , r , [ & ]( std::size_t i ) {
    return( boost::multiprecision::miller_rabin_test( N[ i ] , 25 ) ); } );
#endif
#ifdef ADS_HAS_GMP
  {
   mpz_t x;
   mpz_init( x );
   cell( "isprime" , what , n , "GMP mpz_probab_prime_p" , r ,
         [ & ]( std::size_t i ) {
    mpz_set_ui( x , N[ i ] );
    return( mpz_probab_prime_p( x , 24 ) ); } );
   mpz_clear( x );
   }
#endif
#ifdef ADS_HAS_HURCHALLA
  cell( "isprime" , what , n , "hurchalla::is_prime" , r ,
        [ & ]( std::size_t i ) { return( hurchalla::is_prime( N[ i ] ) ); } );
#endif
  cell( "isprime" , what , n , "ADS::isPrime" , r , [ & ]( std::size_t i ) {
   return( ADS::isPrime( N[ i ] ) ); } );
  }
 }

void factorizations( std::size_t n , unsigned seed , int r )
{
 std::mt19937_64 g( seed );
 for( int c = 0 ; c < 2 ; ++c ) {
  const char * what = c ? "semiprime64" : "random64";
  std::vector< U64 > N( n );
  for( auto & x : N )
   if( c ) {                                // two primes of 32 bits
    U64 p , q;
    do p = ( g() >> 32 ) | 1; while( ! ADS::isPrime( p ) );
    do q = ( g() >> 32 ) | 1; while( ! ADS::isPrime( q ) );
    x = p * q;
    }
   else
    x = g() | 1;
#ifdef ADS_HAS_HURCHALLA
  for( const char * who : { "hurchalla::factorize" ,
                            "hurchalla::factorize A/A" } )
   cell( "factorize" , what , n , who , r , [ & ]( std::size_t i ) {
    std::vector< U64 > F;
    hurchalla::factorize( N[ i ] , F );
    return( F.back() ); } );
#endif
#ifndef ADS_BENCH_OLD
  cell( "factorize" , what , n , "ADS::factorize" , r ,
        [ & ]( std::size_t i ) {
         return( ADS::factorize( N[ i ] ).back() ); } );
#endif
  }
 }

/// the sum of n words of the generator that make() builds
template< class M >
void words( const std::string & who , std::size_t n , int r , M && make )
{
 print( "generators" , "words" , n , who , median( make ,
         [ & ]( auto & G ) {
          U64 s = 0;
          for( std::size_t i = 0 ; i < n ; ++i ) s += U64( G() );
          sink = sink + s; } , r , n ) );
 }

void generators( std::size_t n , unsigned seed , int r )
{
 for( const char * who : { "std::mt19937_64" , "std::mt19937_64 A/A" } )
  words( who , n , r , [ & ](){ return( std::mt19937_64( seed ) ); } );
 words( "std::minstd_rand" , n , r , [ & ](){
        return( std::minstd_rand( seed ) ); } );
#ifdef ADS_HAS_BOOST
 words( "boost::random::mt19937_64" , n , r , [ & ](){
        return( boost::random::mt19937_64( seed ) ); } );
 words( "boost::random::taus88" , n , r , [ & ](){
        return( boost::random::taus88( seed ) ); } );
#endif
#ifdef ADS_HAS_PCG
 words( "pcg64" , n , r , [ & ](){ return( pcg64( seed ) ); } );
 words( "pcg32" , n , r , [ & ](){ return( pcg32( seed ) ); } );
#endif
 words( "ADS::LinearCongruential mmix" , n , r , [ & ](){
        return( ADS::LinearCongruential::mmix( seed ) ); } );
#ifndef ADS_BENCH_OLD
 words( "ADS::Xoshiro256" , n , r , [ & ](){
        return( ADS::Xoshiro256( seed ) ); } );
 words( "ADS::Xoshiro256StarStar" , n , r , [ & ](){
        return( ADS::Xoshiro256StarStar( seed ) ); } );
 words( "ADS::WyRand" , n , r , [ & ](){ return( ADS::WyRand( seed ) ); } );
#endif
#ifdef ADS_HAS_RANDOM123
 words( "r123 Philox4x32" , n , r , [ & ](){
        return( r123::Engine< r123::Philox4x32 >( seed ) ); } );
 words( "r123 Philox4x64" , n , r , [ & ](){
        return( r123::Engine< r123::Philox4x64 >( seed ) ); } );
#endif
#ifndef ADS_BENCH_OLD
 words( "ADS::Philox4x32" , n , r , [ & ](){
        return( ADS::Philox4x32( seed ) ); } );
 words( "ADS::Philox4x64" , n , r , [ & ](){
        return( ADS::Philox4x64( seed ) ); } );
#endif

 std::mt19937_64 h( seed );                 // the bounds below 2^32
 std::vector< U64 > S( 1024 );
 for( auto & s : S ) s = ( h() >> 32 ) | 1;
 auto bounded = [ & ]( const std::string & who , auto && draw ) {
  print( "bounded" , "below2^32" , n , who , median(
          [ & ](){ return( Gen( seed ) ); } , [ & ]( Gen & G ) {
           U64 s = 0;
           for( std::size_t i = 0 ; i < n ; ++i )
            s += draw( G , S[ i & 1023 ] );
           sink = sink + s; } , r , n ) );
  };
 for( const char * who : { "std::uniform_int_distribution" ,
                           "std::uniform_int_distribution A/A" } )
  bounded( who , []( Gen & G , U64 s ) {
   return( std::uniform_int_distribution< U64 >( 0 , s - 1 )( G ) ); } );
#ifdef ADS_HAS_BOOST
 bounded( "boost::random::uniform_int_distribution" , []( Gen & G , U64 s ) {
  return( boost::random::uniform_int_distribution< U64 >( 0 , s - 1 )( G ) );
  } );
#endif
#ifndef ADS_BENCH_OLD
 bounded( "ADS::uniformBelow" , []( Gen & G , U64 s ) {
  return( ADS::uniformBelow( G , s ) ); } );
#endif

 std::vector< std::uint32_t > A( n );
 std::iota( A.begin() , A.end() , 0u );
 auto shuffle = [ & ]( const std::string & who , auto && sh ) {
  print( "shuffle" , "uint32" , n , who , median( [ & ](){
          return( std::pair( A , Gen( seed ) ) ); } , [ & ]( auto & P ) {
           sh( P.first , P.second );
           sink = sink + P.first[ 0 ]; } , r , n ) );
  };
 for( const char * who : { "std::shuffle" , "std::shuffle A/A" } )
  shuffle( who , []( auto & B , Gen & G ) {
   std::shuffle( B.begin() , B.end() , G ); } );
 shuffle( "std::shuffle mt19937_64" , [ & ]( auto & B , Gen & ) {
  std::mt19937_64 m( seed );
  std::shuffle( B.begin() , B.end() , m ); } );
 shuffle( "ADS::shuffle" , []( auto & B , Gen & G ) {
  ADS::shuffle( B , G ); } );
#ifndef ADS_BENCH_OLD
 shuffle( "ADS::shuffleFast" , []( auto & B , Gen & G ) {
  ADS::shuffleFast( B.begin() , B.end() , G ); } );
#endif
 }

/// the sums of n normal and n exponential deviates
void deviates( std::size_t n , unsigned seed , int r )
{
 auto cellOf = [ & ]( const char * what , const std::string & who ,
                      auto && draw ) {
  print( what , "sum" , n , who , median( [ & ](){
          return( Gen( seed ) ); } , [ & ]( Gen & G ) {
           double s = 0;
           for( std::size_t i = 0 ; i < n ; ++i ) s += draw( G );
           sink = sink + U64( s ); } , r , n ) );
  };
 for( const char * who : { "std::normal_distribution" ,
                           "std::normal_distribution A/A" } )
  cellOf( "normal" , who , [ D = std::normal_distribution< double >() ](
                            Gen & G ) mutable { return( D( G ) ); } );
#ifdef ADS_HAS_BOOST
 cellOf( "normal" , "boost::random::normal_distribution" ,
         [ D = boost::random::normal_distribution< double >() ](
          Gen & G ) mutable { return( D( G ) ); } );
#endif
#ifndef ADS_BENCH_OLD
 cellOf( "normal" , "ADS::normalZiggurat" , []( Gen & G ) {
  return( ADS::normalZiggurat( G ) ); } );
#endif
 for( const char * who : { "std::exponential_distribution" ,
                           "std::exponential_distribution A/A" } )
  cellOf( "exponential" , who ,
          [ D = std::exponential_distribution< double >() ](
           Gen & G ) mutable { return( D( G ) ); } );
#ifdef ADS_HAS_BOOST
 cellOf( "exponential" , "boost::random::exponential_distribution" ,
         [ D = boost::random::exponential_distribution< double >() ](
          Gen & G ) mutable { return( D( G ) ); } );
#endif
#ifndef ADS_BENCH_OLD
 cellOf( "exponential" , "ADS::exponentialZiggurat" , []( Gen & G ) {
  return( ADS::exponentialZiggurat( G ) ); } );
#endif
 }

/// n draws from k categories of random weights
void discrete( std::size_t n , unsigned seed , int r )
{
 for( const std::size_t k : { std::size_t( 100 ) , std::size_t( 100000 ) } ) {
  const std::string what = "k=" + std::to_string( k );
  std::mt19937_64 h( seed );
  std::vector< double > w( k );
  for( auto & x : w ) x = double( h() % 1000 + 1 );
  auto cellOf = [ & ]( const std::string & who , auto && draw ) {
   print( "discrete" , what.c_str() , n , who , median( [ & ](){
           return( Gen( seed ) ); } , [ & ]( Gen & G ) {
            U64 s = 0;
            for( std::size_t i = 0 ; i < n ; ++i ) s += U64( draw( G ) );
            sink = sink + s; } , r , n ) );
   };
  std::discrete_distribution< std::size_t > D( w.begin() , w.end() );
  for( const char * who : { "std::discrete_distribution" ,
                            "std::discrete_distribution A/A" } )
   cellOf( who , [ & ]( Gen & G ) { return( D( G ) ); } );
#ifdef ADS_HAS_BOOST
  boost::random::discrete_distribution< std::size_t > E( w.begin() ,
                                                         w.end() );
  cellOf( "boost::random::discrete_distribution" , [ & ]( Gen & G ) {
   return( E( G ) ); } );
#endif
#ifndef ADS_BENCH_OLD
  const ADS::AliasTable T( w );
  cellOf( "ADS::AliasTable" , [ & ]( Gen & G ) { return( T( G ) ); } );
#endif
  }
 }

void sampling( std::size_t N , unsigned seed , int r )
{
 const std::size_t n = 1000;
 std::vector< U64 > S( N );
 std::iota( S.begin() , S.end() , U64( 0 ) );
 auto sample = [ & ]( const std::string & who , auto && smp ) {
  print( "sampling" , "1000" , N , who , median( [ & ](){
          return( Gen( seed ) ); } , [ & ]( Gen & G ) {
           sink = sink + smp( G ); } , r , N ) );
  };
 for( const char * who : { "std::sample forward" ,
                           "std::sample forward A/A" } )
  sample( who , [ & ]( Gen & G ) {
   std::vector< U64 > out( n );
   std::sample( S.begin() , S.end() , out.begin() , n , G );
   return( out[ 0 ] ); } );
 sample( "std::sample input" , [ & ]( Gen & G ) {
  std::vector< U64 > out( n );
  std::sample( InputIt{ S.data() } , InputIt{ S.data() + N } ,
               out.begin() , n , G );
  return( out[ 0 ] ); } );
 sample( "ADS::reservoirSampling" , [ & ]( Gen & G ) {
  return( ADS::reservoirSampling( S , n , G )[ 0 ] ); } );
 sample( "ADS::sampleBySkips" , [ & ]( Gen & G ) {
  return( ADS::sampleBySkips( n , N , G )[ 0 ] ); } );
#ifndef ADS_BENCH_OLD
 sample( "ADS::reservoirSampleL" , [ & ]( Gen & G ) {
  return( ADS::reservoirSampleL( S.begin() , S.end() , n , G )[ 0 ] ); } );
 sample( "ADS::reservoirSampleL input" , [ & ]( Gen & G ) {
  return( ADS::reservoirSampleL( InputIt{ S.data() } ,
                                 InputIt{ S.data() + N } , n , G )[ 0 ] ); } );
#endif
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  gcds( n , o.seed , o.runs );
  powers( n / 10 , o.seed , o.runs );
  primality( n / 10 , o.seed , o.runs );
  factorizations( n / 100 , o.seed , o.runs );
  generators( n , o.seed , o.runs );
  deviates( n , o.seed , o.runs );
  discrete( n , o.seed , o.runs );
  sampling( 10 * n , o.seed , o.runs );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File numeric.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
