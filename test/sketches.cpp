/*--------------------------------------------------------------------------*/
/*---------------------------- File sketches.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the sketches of streams, beyond the book: the Count-Min sketch
 * with the plain and the conservative update, the Count sketch, the
 * minwise hashing by one permutation with the optimal densification and
 * their parallel construction. The estimators are checked against their
 * analysis by statistical tests over many seeds, each with a fixed seed
 * and the tolerance that its comment states, so that a test passes or
 * fails always in the same way; the merges are checked against the sketch
 * of the union, and the edge cases (no item, one item, duplicates, the
 * extreme parameters, items of 32 bits) one by one, at the level 3 of the
 * contracts.
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
#include "ADS/set/MinHashSketch.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
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
// alone: the plain Count-Min, the Count sketch and the minwise hashing
// equal to the sequential sketches; an empty range, the empty sketch
ADS_TEST( "parallel-sketch" )
{
 const std::size_t n = 200000;
 const auto W = words( n , 21 );
 Sequential seq;
 ThreadPool one( 1 ) , four( 4 );
 CountMinSketch< std::uint64_t > C( 1000 , 4 );
 CountSketch< std::uint64_t > S( 1000 , 5 );
 MinHashSketch< std::uint64_t > M( 256 , 7 );
 for( const auto w : W ) {
  C.insert( w );
  S.insert( w );
  M.insert( w );
  }
 const auto all = [ & ]( auto & ex ) {
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 CountMinSketch< std::uint64_t >( 1000 , 4 ) ) == C );
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 CountSketch< std::uint64_t >( 1000 , 5 ) ) == S );
  CHECK( parallelSketch( ex , W.begin() , W.end() ,
			 MinHashSketch< std::uint64_t >( 256 , 7 ) ) == M );
  CHECK( parallelSketch( ex , W.begin() , W.begin() ,
			 CountMinSketch< std::uint64_t >( 1000 , 4 ) )
	   .empty() );
  };
 all( seq );
 all( one );
 all( four );
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
