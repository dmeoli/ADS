/*--------------------------------------------------------------------------*/
/*---------------------------- File sketches.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the sketches of streams, beyond the book: UltraLogLog with its
 * two estimators, the Count-Min sketch with the plain and the conservative
 * update, the Count sketch, the KLL sketch of the quantiles, the minwise
 * hashing by one permutation with the optimal densification and their
 * parallel construction. The estimators are checked against their
 * analysis by statistical tests over many seeds, each with a fixed seed
 * and the tolerance that its comment states, so that a test passes or
 * fails always in the same way; the merges are checked against the sketch
 * of the union, and the edge cases (no item, one item, duplicates, the
 * extreme parameters, items of 32 bits, sizes around the powers of 2) one
 * by one, at the level 3 of the contracts.
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

#include "ADS/parallel/ParallelSketch.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/set/CountMinSketch.h"
#include "ADS/set/KllSketch.h"
#include "ADS/set/MinHashSketch.h"
#include "ADS/set/UltraLogLog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// n random words from the seed, distinct with probability
/// \f$ 1 - n^2 / 2^{65} \f$
std::vector< std::uint64_t > words( std::size_t n , std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::uint64_t > K( n );
 for( auto & k : K )
  k = g();
 return( K );
 }

/// the mean and the root of the mean square of the values
struct Moments
{
 double mean = 0 , rms = 0;
 };

Moments moments( const std::vector< double > & e )
{
 Moments m;
 for( const double x : e ) {
  m.mean += x;
  m.rms += x * x;
  }
 m.mean /= double( e.size() );
 m.rms = std::sqrt( m.rms / double( e.size() ) );
 return( m );
 }

/// a stream of N items of a Zipf law of exponent 1 over u items, with the
/// exact frequencies
std::vector< std::uint64_t >
zipf( std::size_t N , std::size_t u , std::uint64_t seed ,
      std::unordered_map< std::uint64_t , std::uint64_t > & f )
{
 std::vector< double > c( u );
 double s = 0;
 for( std::size_t i = 0 ; i < u ; ++i )
  c[ i ] = ( s += 1.0 / double( i + 1 ) );
 std::mt19937_64 g( seed );
 std::uniform_real_distribution< double > U( 0 , s );
 std::vector< std::uint64_t > X( N );
 for( auto & x : X ) {
  x = std::uint64_t( std::lower_bound( c.begin() , c.end() , U( g ) ) -
		     c.begin() ) *
      0x9E3779B97F4A7C15ULL;
  ++f[ x ];
  }
 return( X );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ ULTRALOGLOG -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the relative errors of the two estimators over R streams of n random
/// words, at the precision p
void ullErrors( unsigned p , std::size_t n , int R ,
		std::vector< double > & F ,
		std::vector< double > & M )
{
 F.clear();
 M.clear();
 for( int r = 0 ; r < R ; ++r ) {
  UltraLogLog< std::uint64_t > U( p );
  for( const auto k : words( n , 1000003 * std::uint64_t( r ) + n + p ) )
   U.insert( k );
  F.push_back( U.estimate() / double( n ) - 1 );
  M.push_back( U.estimateML() / double( n ) - 1 );
  }
 }

// the two estimators over 200 streams of each size from 1 to 10^5 at the
// precisions 6 and 10 (100 streams at 12): the mean of the relative errors
// within 4 standard errors of the mean of the asymptotic law,
// sigma / 200^{1/2} with sigma = relativeStandardError(), plus 1 / m for
// the bias of order 1 / m, and from 30 items on, where a collision of two
// items in a register is no longer an error of 1 / n alone, their root
// mean square within 1.25 sigma, whose own relative error over 200 streams
// is about 5 per cent; the asymptotic errors of the two estimators,
// sqrt( 0.6119 / m ) and sqrt( 0.5789 / m )
ADS_TEST( "ultraloglog-estimates" )
{
 std::vector< double > F , M;
 for( const unsigned p : { 6U , 10U , 12U } ) {
  const int R = p == 12 ? 100 : 200;
  const double m = double( 1U << p );
  const UltraLogLog< std::uint64_t > E( p );
  const double sF = E.relativeStandardError() ,
	       sM = E.relativeStandardError( true );
  CHECK( std::abs( sF * sF * m - 0.6119 ) < 1e-3 &&
	 std::abs( sM * sM * m - 0.5789 ) < 1e-3 );
  for( const std::size_t n :
       { 1 , 2 , 5 , 10 , 30 , 100 , 300 , 1000 , 3000 , 10000 , 100000 } ) {
   ullErrors( p , n , R , F , M );
   const Moments f = moments( F ) , g = moments( M );
   const double tF = 4 * sF / std::sqrt( double( R ) ) + 1 / m ,
		tM = 4 * sM / std::sqrt( double( R ) ) + 1 / m;
   CHECK( std::abs( f.mean ) < tF && std::abs( g.mean ) < tM );
   CHECK( n < 30 || ( f.rms < 1.25 * sF && g.rms < 1.25 * sM ) );
   }
  }
 }

// the empty sketch, of estimate 0, and one item, estimated within 1 per
// cent at the precisions 12 and 20 and within 15 per cent at 3, whose 8
// registers give the estimators a bias of order 1 / 8; the same item many
// times, the same
// registers as once; a stream of items of 32 bits and one of strings,
// within 4 sigma; the sketch of the highest precision, 26; the
// precisions out of 3 to 26
ADS_TEST( "ultraloglog-edges" )
{
 for( const unsigned p : { 3U , 12U , 20U } ) {
  UltraLogLog< std::uint64_t > U( p );
  CHECK( U.empty() && U.estimate() == 0 && U.estimateML() == 0 );
  CHECK( U.precision() == p && U.state().size() == ( std::size_t( 1 ) << p ) );
  U.insert( 42 );
  CHECK( ! U.empty() );
  const double t = p == 3 ? 0.15 : 0.01;
  CHECK( std::abs( U.estimate() - 1 ) < t &&
	 std::abs( U.estimateML() - 1 ) < t );
  const auto once = U;
  for( int i = 0 ; i < 1000 ; ++i )
   U.insert( 42 );
  CHECK( U == once );
  U.clear();
  CHECK( U.empty() );
  }
 {  // 32 bits and strings
  UltraLogLog< std::uint32_t > A( 12 );
  UltraLogLog< std::string > S( 12 );
  for( std::uint32_t i = 0 ; i < 50000 ; ++i ) {
   A.insert( i );
   A.insert( i );  // a duplicate
   S.insert( std::to_string( i ) );
   }
  const double s = A.relativeStandardError();
  CHECK( std::abs( A.estimate() / 50000 - 1 ) < 4 * s );
  CHECK( std::abs( S.estimate() / 50000 - 1 ) < 4 * s );
  CHECK( std::abs( S.estimateML() / 50000 - 1 ) < 4 * s );
  }
 {  // the highest precision
  UltraLogLog< std::uint64_t > U( 26 );
  for( const auto k : words( 1000 , 5 ) )
   U.insert( k );
  CHECK( std::abs( U.estimate() / 1000 - 1 ) < 0.01 );
  }
 CHECK_THROWS( UltraLogLog< int >( 2 ) , contract_violation );
 CHECK_THROWS( UltraLogLog< int >( 27 ) , contract_violation );
 }

// the merge of two sketches of the halves of a stream, equal to the sketch
// of the stream at the precisions 3 to 14; the merge of a sketch of a
// higher precision, equal to the merge of its downsizing; the downsizing
// from 14 to each lower precision, equal to the sketch of the same stream
// at that precision, register by register; a merge into the empty sketch,
// and of an empty one; the merge of a lower precision and a downsizing
// beyond the precision, rejected
ADS_TEST( "ultraloglog-merge" )
{
 const auto K = words( 30000 , 11 );
 for( unsigned p = 3 ; p <= 14 ; ++p ) {
  UltraLogLog< std::uint64_t > A( p ) , B( p ) , U( p );
  for( std::size_t i = 0 ; i < K.size() ; ++i ) {
   ( i % 3 ? A : B ).insert( K[ i ] );
   U.insert( K[ i ] );
   }
  auto C = A;
  C.merge( B );
  CHECK( C == U );
  UltraLogLog< std::uint64_t > E( p );
  E.merge( U );
  CHECK( E == U );
  U.merge( UltraLogLog< std::uint64_t >( p ) );
  CHECK( E == U );
  }
 UltraLogLog< std::uint64_t > H( 14 );
 for( const auto k : K )
  H.insert( k );
 for( unsigned q = 3 ; q <= 14 ; ++q ) {
  UltraLogLog< std::uint64_t > D( q ) , L( q );
  for( const auto k : K )
   D.insert( k );
  CHECK( H.downsize( q ) == D );
  for( std::size_t i = 0 ; i < 1000 ; ++i )
   L.insert( K[ i ] );
  auto M = L;
  M.merge( H );
  L.merge( H.downsize( q ) );
  CHECK( M == L && M == D );
  }
 UltraLogLog< std::uint64_t > X( 10 );
 CHECK_THROWS( H.downsize( 15 ) , contract_violation );
 CHECK_THROWS( H.downsize( 2 ) , contract_violation );
 CHECK_THROWS( H.merge( X ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ COUNT-MIN ---------------------------------*/
/*--------------------------------------------------------------------------*/

// a stream of 2 10^5 items of a Zipf law over 10^4 items into sketches of
// epsilon = 10^-3 and delta = e^-5: no estimate below its frequency, the
// fraction of the items whose plain estimate exceeds it by more than
// epsilon N at most delta, the conservative estimates between the
// frequencies and the plain ones, and above the frequency by more than
// epsilon N on fewer items than the plain ones; the merges of the sketches
// of three parts, equal to the sketch of the stream for the plain update
// and between the frequencies and the plain sketch for the conservative
// one; an empty sketch and one of one counter; the shapes rejected
ADS_TEST( "count-min" )
{
 std::unordered_map< std::uint64_t , std::uint64_t > f;
 const auto X = zipf( 200000 , 10000 , 3 , f );
 auto P = CountMinSketch< std::uint64_t >::forError( 1e-3 , std::exp( -5.0 ) );
 auto C = CountMinSketch< std::uint64_t >::forError(
   1e-3 , std::exp( -5.0 ) , CountMinUpdate::conservative );
 CHECK( P.width() == 2719 && P.depth() == 5 && P.empty() );
 CHECK( C.update() == CountMinUpdate::conservative );
 std::vector< CountMinSketch< std::uint64_t > > Pp( 3 , P ) , Cp( 3 , C );
 for( std::size_t i = 0 ; i < X.size() ; ++i ) {
  P.insert( X[ i ] );
  C.insert( X[ i ] );
  Pp[ i % 3 ].insert( X[ i ] );
  Cp[ i % 3 ].insert( X[ i ] );
  }
 CHECK( P.total() == X.size() && C.total() == X.size() );
 const double eN = P.errorBound();
 CHECK( std::abs( eN - std::exp( 1.0 ) / 2719 * 200000 ) < 1e-9 );
 std::size_t overP = 0 , overC = 0;
 for( const auto & [ x , c ] : f ) {
  const auto p = P.estimate( x ) , q = C.estimate( x );
  CHECK( c <= q && q <= p );
  overP += double( p - c ) > 1e-3 * double( X.size() );
  overC += double( q - c ) > 1e-3 * double( X.size() );
  }
 CHECK( double( overP ) <= std::exp( -5.0 ) * double( f.size() ) );
 CHECK( overC <= overP );
 Pp[ 0 ].merge( Pp[ 1 ] );
 Pp[ 0 ].merge( Pp[ 2 ] );
 CHECK( Pp[ 0 ] == P );
 Cp[ 0 ].merge( Cp[ 1 ] );
 Cp[ 0 ].merge( Cp[ 2 ] );
 for( const auto & [ x , c ] : f )
  CHECK( c <= Cp[ 0 ].estimate( x ) &&
	 Cp[ 0 ].estimate( x ) <= P.estimate( x ) );
 P.clear();
 CHECK( P.empty() && P.estimate( X[ 0 ] ) == 0 );
 CountMinSketch< std::uint32_t , std::uint32_t > One( 1 , 1 );
 One.insert( 7 , 5 );
 One.insert( 8 , 2 );
 CHECK( One.estimate( 9 ) == 7 && One.total() == 7 );
 CHECK_THROWS( ( CountMinSketch< int >( 0 , 3 ) ) , contract_violation );
 CHECK_THROWS( ( CountMinSketch< int >( 3 , 0 ) ) , contract_violation );
 CHECK_THROWS( CountMinSketch< int >::forError( 0 , 0.1 ) ,
	       contract_violation );
 CHECK_THROWS( CountMinSketch< int >::forError( 0.1 , 1 ) ,
	       contract_violation );
 CountMinSketch< int > A( 10 , 3 ) , B( 11 , 3 ) ,
   Cc( 10 , 3 , CountMinUpdate::conservative );
 CHECK_THROWS( A.merge( B ) , contract_violation );
 CHECK_THROWS( A.merge( Cc ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- COUNT SKETCH -------------------------------*/
/*--------------------------------------------------------------------------*/

// the same stream into a Count sketch of 5 rows of 1000 counters: the
// fraction of the items whose error exceeds 3 ( F_2 / w )^{1/2} at most 2
// per cent (a row exceeds it with probability at most 1/9 by the inequality
// of Chebyshev, the median of 5 with probability below 1.5 per cent), the
// mean of the signed errors within 0.2 ( F_2 / w )^{1/2}, the second
// moment within 15 per cent (3 times the standard error ( 2 / w )^{1/2} of
// a row); the merge of three parts, equal to the sketch of the stream; the
// removal of every item, back to the empty sketch; an even number of rows
ADS_TEST( "count-sketch" )
{
 std::unordered_map< std::uint64_t , std::uint64_t > f;
 const auto X = zipf( 200000 , 10000 , 4 , f );
 CountSketch< std::uint64_t > S( 1000 , 5 ) , E( 1000 , 5 );
 std::vector< CountSketch< std::uint64_t > > Q( 3 , S );
 for( std::size_t i = 0 ; i < X.size() ; ++i ) {
  S.insert( X[ i ] );
  Q[ i % 3 ].insert( X[ i ] );
  }
 CHECK( S.width() == 1000 && S.depth() == 5 );
 double F2 = 0;
 for( const auto & [ x , c ] : f )
  F2 += double( c ) * double( c );
 const double b = std::sqrt( F2 / 1000 );
 std::size_t over = 0;
 double mean = 0;
 for( const auto & [ x , c ] : f ) {
  const double e = S.estimate( x ) - double( c );
  over += std::abs( e ) > 3 * b;
  mean += e;
  }
 CHECK( double( over ) <= 0.02 * double( f.size() ) );
 CHECK( std::abs( mean / double( f.size() ) ) < 0.2 * b );
 CHECK( std::abs( S.secondMoment() / F2 - 1 ) < 0.15 );
 Q[ 0 ].merge( Q[ 1 ] );
 Q[ 0 ].merge( Q[ 2 ] );
 CHECK( Q[ 0 ] == S );
 for( const auto & [ x , c ] : f )
  S.insert( x , -std::int64_t( c ) );
 CHECK( S == E && S.secondMoment() == 0 );
 CountSketch< std::uint32_t > T( 64 , 4 );
 T.insert( 5 , 10 );
 CHECK( T.estimate( 5 ) == 10 );
 T.clear();
 CHECK( T.estimate( 5 ) == 0 );
 CHECK_THROWS( ( CountSketch< int >( 0 , 1 ) ) , contract_violation );
 CountSketch< int > A( 10 , 3 ) , B( 10 , 4 );
 CHECK_THROWS( A.merge( B ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- KLL -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the largest error of the ranks of the items 0 , ... , n - 1 of the
/// sketch, every step-th of them, and the fraction above eps
template< class S >
std::pair< double , double > rankErrors( const S & K , std::size_t n ,
					std::size_t step , double eps )
{
 const auto V = K.view();
 double worst = 0;
 std::size_t over = 0 , q = 0;
 for( std::size_t v = 0 ; v < n ; v += step , ++q ) {
  const double e =
    std::abs( V.rank( double( v ) ) - double( v + 1 ) / double( n ) );
  worst = std::max( worst , e );
  over += e > eps;
  }
 return( std::pair< double , double >( worst ,
				       double( over ) / double( q ) ) );
 }

// the ranks of 100 queries in random permutations of 0 , ... , n - 1 for n
// from 2^12 - 1 to 10^5 and k = 200 and 50, over 30 seeds each, and in the
// sorted and the reversed orders: at most 2 per cent of the errors above
// normalizedRankError( k ), the fit of DataSketches at 99 per cent, and
// none above twice it; the items retained within the capacity, at most
// 3 k plus 8.5 per level
ADS_TEST( "kll-ranks" )
{
 for( const std::uint16_t k : { std::uint16_t( 50 ) ,
				std::uint16_t( 200 ) } ) {
  const double eps = KllSketch< double >::normalizedRankError( k );
  for( const std::size_t n : { 4095 , 4096 , 4097 , 100000 } ) {
   std::vector< double > X( n );
   std::iota( X.begin() , X.end() , 0.0 );
   double worst = 0 , over = 0;
   int runs = 0;
   for( int s = 0 ; s < 32 ; ++s , ++runs ) {
    std::vector< double > Y = X;
    if( s == 30 )
     std::reverse( Y.begin() , Y.end() );
    else if( s < 30 )
     std::shuffle( Y.begin() , Y.end() , std::mt19937_64( s + n ) );
    KllSketch< double > K( k , std::uint64_t( s ) );
    for( const double y : Y )
     K.insert( y );
    CHECK( K.size() == n && K.min() == 0 && K.max() == double( n - 1 ) );
    CHECK( 2 * K.retained() <= 6 * std::size_t( k ) + 17 * K.levels() );
    const auto [ w , o ] = rankErrors( K , n , n / 100 , eps );
    worst = std::max( worst , w );
    over += o;
    }
   CHECK( over / runs <= 0.02 && worst <= 2 * eps );
   }
  }
 }

// no item, one item, n <= k items (the ranks exact, the quantiles the items
// themselves) at n = 1, 2, 7, 8, 9, 199, 200, 201 for k = 8 and 200; 10^5
// copies of one item and of two; the sizes around the powers of 2 up to
// 2^16 + 1 at k = 8, the least, whose capacities are all 8 but the top;
// items of 32 bits and strings; k below 8 and the quantiles of an empty
// sketch, rejected
ADS_TEST( "kll-edges" )
{
 KllSketch< double > Z;
 CHECK( Z.empty() && Z.size() == 0 && Z.retained() == 0 && Z.levels() == 1 );
 CHECK( Z.rank( 3 ) == 0 && Z.k() == 200 );
 CHECK_THROWS( ( void )Z.quantile( 0.5 ) , contract_violation );
 CHECK_THROWS( ( void )Z.min() , contract_violation );
 for( const std::uint16_t k : { std::uint16_t( 8 ) , std::uint16_t( 200 ) } )
  for( const std::size_t n : { 1 , 2 , 7 , 8 , 9 , 199 , 200 , 201 } ) {
   if( n > k )
    continue;
   KllSketch< int > K( k );
   for( std::size_t i = n ; i-- > 0 ; )
    K.insert( int( i ) );
   CHECK( K.size() == n && K.retained() == n );
   for( std::size_t i = 0 ; i < n ; ++i ) {
    CHECK( K.rank( int( i ) ) == double( i + 1 ) / double( n ) );
    CHECK( K.quantile( double( i + 1 ) / double( n ) ) == int( i ) );
    }
   CHECK( K.quantile( 0 ) == 0 && K.quantile( 1 ) == int( n - 1 ) );
   }
 {  // duplicates
  KllSketch< int > D , E;
  for( int i = 0 ; i < 100000 ; ++i ) {
   D.insert( 7 );
   E.insert( i % 2 ? 3 : 9 );
   }
  for( const double q : { 0.0 , 0.1 , 0.5 , 0.99 , 1.0 } )
   CHECK( D.quantile( q ) == 7 );
  CHECK( D.rank( 7 ) == 1 && D.rank( 6 ) == 0 && D.min() == 7 );
  CHECK( std::abs( E.rank( 3 ) - 0.5 ) < 0.02 && E.rank( 9 ) == 1 );
  CHECK( E.quantile( 0.25 ) == 3 && E.quantile( 0.75 ) == 9 );
  }
 for( std::size_t n = 2 ; n <= 65537 ; n *= 2 )
  for( const std::size_t m : { n - 1 , n , n + 1 } ) {
   KllSketch< std::uint32_t > K( 8 , m );
   for( std::uint32_t i = 0 ; i < m ; ++i )
    K.insert( i );
   CHECK( K.size() == m && K.min() == 0 && K.max() == m - 1 );
   CHECK( K.retained() <= 8 * std::size_t( K.levels() ) );
   }
 {
  KllSketch< std::string > S( 64 );
  for( int i = 0 ; i < 10000 ; ++i )
   S.insert( std::to_string( 100000 + i ) );
  const std::string m = S.quantile( 0.5 );
  CHECK( std::abs( std::stod( m ) - 105000 ) <
	 10000 * 2 * KllSketch< std::string >::normalizedRankError( 64 ) );
  }
 CHECK_THROWS( KllSketch< int >( 7 ) , contract_violation );
 KllSketch< int > K;
 K.insert( 1 );
 CHECK_THROWS( ( void )K.quantile( 1.5 ) , contract_violation );
 K.clear();
 CHECK( K.empty() && K.retained() == 0 );
 }

// the sketch of 10^5 items merged from 2, 16 and 100 parts of a random
// permutation (and the self merge of one), with the bound of kll-ranks on
// the whole stream, its size and its extremes; a merge of sketches of
// different k; the merge of an empty sketch and into one
ADS_TEST( "kll-merge" )
{
 const std::size_t n = 100000;
 std::vector< double > X( n );
 std::iota( X.begin() , X.end() , 0.0 );
 std::shuffle( X.begin() , X.end() , std::mt19937_64( 9 ) );
 const double eps = KllSketch< double >::normalizedRankError( 200 );
 for( const std::size_t parts : { 2 , 16 , 100 } ) {
  double over = 0 , worst = 0;
  for( int s = 0 ; s < 10 ; ++s ) {
   std::vector< KllSketch< double > > P;
   for( std::size_t p = 0 ; p < parts ; ++p )
    P.emplace_back( 200 , std::uint64_t( 1000 * s + p ) );
   for( std::size_t i = 0 ; i < n ; ++i )
    P[ i * parts / n ].insert( X[ i ] );
   KllSketch< double > M( 200 , std::uint64_t( s ) );
   for( const auto & p : P )
    M.merge( p );
   CHECK( M.size() == n && M.min() == 0 && M.max() == double( n - 1 ) );
   const auto [ w , o ] = rankErrors( M , n , n / 100 , eps );
   worst = std::max( worst , w );
   over += o;
   }
  CHECK( over / 10 <= 0.02 && worst <= 2 * eps );
  }
 KllSketch< double > A( 200 , 1 ) , B( 50 , 2 ) , E;
 for( std::size_t i = 0 ; i < n ; ++i )
  ( i % 2 ? A : B ).insert( X[ i ] );
 A.merge( B );
 CHECK( A.size() == n &&
	rankErrors( A , n , n / 100 ,
		    2 * KllSketch< double >::normalizedRankError( 50 ) )
	    .second == 0 );
 const auto before = A.quantile( 0.5 );
 A.merge( E );
 CHECK( A.size() == n && A.quantile( 0.5 ) == before );
 E.merge( A );
 CHECK( E.size() == n && E.min() == 0 && E.max() == double( n - 1 ) );
 KllSketch< double > S( 200 , 5 );
 for( std::size_t i = 0 ; i < 5000 ; ++i )
  S.insert( X[ i ] );
 S.merge( S );
 CHECK( S.size() == 10000 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MINHASH ---------------------------------*/
/*--------------------------------------------------------------------------*/

// two sets of u items in all, of similarity J = 0, 0.1, 0.5, 0.9 and 1, at
// u = 20 (most of the 256 bins empty, the densification at work), 300 and
// 10^4, over 300 pairs of random words each: the mean of the estimates
// within 4 standard errors of the mean of J (from the spread of the
// estimates, plus 0.002), and at u = 10^4 the root mean square of the
// errors within 1.25 times ( J ( 1 - J ) / K )^{1/2}, that of K
// independent minima
ADS_TEST( "minhash-jaccard" )
{
 const std::size_t K = 256;
 for( const std::size_t u : { 20 , 300 , 10000 } )
  for( const double J : { 0.0 , 0.1 , 0.5 , 0.9 , 1.0 } ) {
   const std::size_t common = std::size_t( std::lround( J * double( u ) ) ) ,
		     only = ( u - common ) / 2;
   const double j = double( common ) / double( common + 2 * only );
   std::vector< double > E;
   for( int r = 0 ; r < 300 ; ++r ) {
    const auto W = words( common + 2 * only , 7919 * std::uint64_t( r ) + u );
    MinHashSketch< std::uint64_t > A( K ) , B( K );
    for( std::size_t i = 0 ; i < common ; ++i ) {
     A.insert( W[ i ] );
     B.insert( W[ i ] );
     }
    for( std::size_t i = 0 ; i < only ; ++i ) {
     A.insert( W[ common + i ] );
     B.insert( W[ common + only + i ] );
     }
    E.push_back( A.jaccard( B ) - j );
    }
   const Moments m = moments( E );
   const double sd =
     std::sqrt( std::max( 0.0 , m.rms * m.rms - m.mean * m.mean ) );
   CHECK( std::abs( m.mean ) <= 4 * sd / std::sqrt( 300.0 ) + 0.002 );
   if( u == 10000 )
    CHECK( m.rms <= 1.25 * std::sqrt( j * ( 1 - j ) / double( K ) ) + 1e-12 );
   }
 }

// the merge of the sketches of two parts, equal to the sketch of the whole
// set; equal sets, the same signature and the similarity 1; duplicates,
// the same bins as once; one item, all the bins of the signature its
// value; one bin; the empty sets (1 between two, 0 with another); the
// signatures of an empty set, all empty; sketches of other bins or seeds,
// rejected
ADS_TEST( "minhash-edges" )
{
 const auto W = words( 5000 , 13 );
 MinHashSketch< std::uint64_t > A( 128 , 3 ) , B( 128 , 3 ) , U( 128 , 3 ) ,
   E( 128 , 3 );
 for( std::size_t i = 0 ; i < W.size() ; ++i ) {
  ( i % 2 ? A : B ).insert( W[ i ] );
  U.insert( W[ i ] );
  U.insert( W[ i ] );
  }
 A.merge( B );
 CHECK( A == U && A.signature() == U.signature() && A.jaccard( U ) == 1 );
 CHECK( A.filled() == 128 && ! A.empty() && A.bins() == 128 );
 CHECK( E.empty() && E.filled() == 0 && E.jaccard( E ) == 1 &&
	E.jaccard( A ) == 0 && A.jaccard( E ) == 0 );
 for( const auto s : E.signature() )
  CHECK( s == MinHashSketch< std::uint64_t >::kEmpty );
 MinHashSketch< std::uint32_t > O( 64 );
 O.insert( 77 );
 CHECK( O.filled() == 1 );
 const auto S = O.signature();
 CHECK( std::all_of( S.begin() , S.end() ,
		     [ & ]( std::uint64_t s ) { return( s == S[ 0 ] ); } ) );
 CHECK( S[ 0 ] != MinHashSketch< std::uint32_t >::kEmpty );
 MinHashSketch< int > One( 1 ) , Two( 1 );
 One.insert( 1 );
 Two.insert( 2 );
 CHECK( One.jaccard( Two ) == 0 && One.jaccard( One ) == 1 );
 One.clear();
 CHECK( One.empty() );
 MinHashSketch< std::uint64_t > C( 64 , 3 ) , D( 128 , 4 );
 CHECK_THROWS( A.merge( C ) , contract_violation );
 CHECK_THROWS( ( void )A.jaccard( D ) , contract_violation );
 CHECK_THROWS( MinHashSketch< int >( 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- PARALLEL ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the parallel construction on 1 and 4 threads and on the calling thread
// alone: UltraLogLog, the plain Count-Min, the Count sketch and the minwise
// hashing equal to the sequential sketches; the KLL sketch the same on the
// three executors, with the bound of kll-ranks; an empty range, the empty
// sketch
ADS_TEST( "parallel-sketch" )
{
 const std::size_t n = 200000;
 const auto W = words( n , 21 );
 Sequential seq;
 ThreadPool one( 1 ) , four( 4 );
 UltraLogLog< std::uint64_t > U( 12 );
 CountMinSketch< std::uint64_t > C( 1000 , 4 );
 CountSketch< std::uint64_t > S( 1000 , 5 );
 MinHashSketch< std::uint64_t > M( 256 , 7 );
 for( const auto w : W ) {
  U.insert( w );
  C.insert( w );
  S.insert( w );
  M.insert( w );
  }
 const auto all = [ & ]( auto & ex ) {
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 UltraLogLog< std::uint64_t >( 12 ) ) == U );
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 CountMinSketch< std::uint64_t >( 1000 , 4 ) ) == C );
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 CountSketch< std::uint64_t >( 1000 , 5 ) ) == S );
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 MinHashSketch< std::uint64_t >( 256 , 7 ) ) == M );
  CHECK( parallelSketch( ex , W.begin() , W.begin() ,
			 UltraLogLog< std::uint64_t >( 12 ) )
	   .empty() );
  };
 all( seq );
 all( one );
 all( four );
 std::vector< double > X( n );
 std::iota( X.begin() , X.end() , 0.0 );
 std::shuffle( X.begin() , X.end() , std::mt19937_64( 3 ) );
 const KllSketch< double > K0( 200 , 1 );
 const auto a = parallelSketch( seq , X.begin() , X.end() , K0 ) ,
	    b = parallelSketch( four , X.begin() , X.end() , K0 );
 CHECK( a.size() == n && b.size() == n && a.retained() == b.retained() );
 for( const double q : { 0.01 , 0.1 , 0.5 , 0.9 , 0.99 } )
  CHECK( a.quantile( q ) == b.quantile( q ) );
 CHECK( rankErrors( a , n , n / 100 ,
		    2 * KllSketch< double >::normalizedRankError( 200 ) )
	  .second == 0 );
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
/*------------------------- End File sketches.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
