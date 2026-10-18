/*--------------------------------------------------------------------------*/
/*-------------------------- File pearls_io.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the algorithms of chapters 2 to 6 of Ferragina's notes: the
 * segments of largest sum and their variants, the random samplings, the
 * ranking of a list, the sorts in the two-level memory with their lower
 * bounds, and the intersections of sorted sets. Each one against a brute
 * force or the standard library on random instances with a fixed seed and
 * on the examples of the notes, at the level 3 of the contracts; the
 * algorithms of the external memory also against the I/Os and the memory
 * of their analysis, counted by the model of TwoLevelMemory.h.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapters 2-6
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

#include "ADS/algorithm/MaximumSegments.h"
#include "ADS/algorithm/Subsequences.h"
#include "ADS/external/ExternalSorting.h"
#include "ADS/external/ListRanking.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/set/SortedIntersection.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// n integers uniform in [ lo , hi ]
std::vector< long > randomVector( std::mt19937 & g , std::size_t n ,
                                  long lo , long hi )
{
 std::uniform_int_distribution< long > d( lo , hi );
 std::vector< long > D( n );
 for( auto & x : D ) x = d( g );
 return( D );
 }

/// the sum of D[ b .. e )
long sumOf( const std::vector< long > & D , std::size_t b , std::size_t e )
{
 return( std::accumulate( D.begin() + std::ptrdiff_t( b ) ,
                          D.begin() + std::ptrdiff_t( e ) , 0L ) );
 }

/// a random list of n items: the successors of a random order
std::vector< std::size_t > randomList( std::mt19937 & g , std::size_t n )
{
 std::vector< std::size_t > order( n ) , succ( n );
 std::iota( order.begin() , order.end() , std::size_t( 0 ) );
 std::shuffle( order.begin() , order.end() , g );
 for( std::size_t k = 0 ; k < n ; ++k )
  succ[ order[ k ] ] = k + 1 < n ? order[ k + 1 ] : order[ k ];
 return( succ );
 }

/// ceil( log2( x ) ) for x >= 1
std::size_t ceilLog2( std::size_t x )
{
 std::size_t r = 0;
 while( ( std::size_t( 1 ) << r ) < x ) ++r;
 return( r );
 }

/// a strictly increasing random set of n values below U
std::vector< long > randomSet( std::mt19937 & g , std::size_t n , long U )
{
 std::set< long > S;
 std::uniform_int_distribution< long > d( 0 , U - 1 );
 while( S.size() < n ) S.insert( d( g ) );
 return( std::vector< long >( S.begin() , S.end() ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 2 ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the example of the notes, D[ 3 , 7 ] of sum 8, by the four algorithms;
// an array of negative numbers gives its largest item, where
// maxSubarraySum() gives the empty segment
ADS_TEST( "ch2.max-sum-example" )
{
 const std::vector< long > D = { 4 , -6 , 3 , 1 , 3 , -2 , 3 , -4 , 1 , -9 ,
                                 6 };
 for( const auto & S : { maxSumSegmentCubic( D ) ,
                         maxSumSegmentQuadratic( D ) , maxSumSegment( D ) ,
                         maxSumSegmentPrefix( D ) } )
  CHECK( S.sum == 8 && S.first == 2 && S.length == 5 );
 CHECK( maxSubarraySum( D ) == 8 );
 const std::vector< long > N = { -5 , -2 , -7 };
 const auto S = maxSumSegment( N );
 CHECK( S.sum == -2 && S.first == 1 && S.length == 1 );
 CHECK( maxSumSegmentPrefix( N ).sum == -2 && maxSubarraySum( N ) == 0 );
 CHECK( maxSumSegment( std::vector< long >{ 3 } ).sum == 3 );
 CHECK_THROWS( maxSumSegment( std::vector< long >() ) ,
               contract_violation );
 }

// the linear scans against the cubic enumeration on random arrays
ADS_TEST( "ch2.max-sum-random" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const auto D = randomVector( g , 1 + std::size_t( t % 40 ) , -10 , 10 );
  const long best = maxSumSegmentCubic( D ).sum;
  CHECK( maxSumSegmentQuadratic( D ).sum == best );
  for( const auto & S : { maxSumSegment( D ) , maxSumSegmentPrefix( D ) } ) {
   CHECK( S.sum == best && S.length >= 1 );
   CHECK( sumOf( D , S.first , S.first + S.length ) == best );
   }
  CHECK( maxSubarraySum( D ) == std::max( best , 0L ) );
  }
 }

// the sum with the length in [ L , U ] against all the segments
ADS_TEST( "ch2.length-constrained" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = 1 + std::size_t( t % 30 );
  const auto D = randomVector( g , n , -10 , 10 );
  const std::size_t L = 1 + g() % n , U = L + g() % ( n + 2 );
  long best = 0;
  bool any = false;
  for( std::size_t b = 0 ; b < n ; ++b )
   for( std::size_t e = b + L ; ( e <= n ) && ( e - b <= U ) ; ++e )
    if( ! any || ( sumOf( D , b , e ) > best ) ) {
     best = sumOf( D , b , e );
     any = true;
     }
  const auto S = maxSumSegmentLength( D , L , U );
  CHECK( S.sum == best && S.length >= L && S.length <= U );
  CHECK( sumOf( D , S.first , S.first + S.length ) == best );
  }
 const std::vector< long > D = { 4 , -6 , 3 , 1 , 3 , -2 , 3 , -4 , 1 , -9 ,
                                 6 };
 CHECK( maxSumSegmentLength( D , 1 , 11 ).sum == 8 );
 CHECK( maxSumSegmentLength( D , 1 , 1 ).sum == 6 );
 CHECK( maxSumSegmentLength( D , 11 , 11 ).sum == 0 );
 CHECK_THROWS( maxSumSegmentLength( D , 3 , 2 ) , contract_violation );
 CHECK_THROWS( maxSumSegmentLength( D , 12 , 12 ) , contract_violation );
 }

// the longest segment of sum at least t, and of average at least p / q,
// against all the segments
ADS_TEST( "ch2.longest-at-least" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = std::size_t( t % 35 );
  const auto D = randomVector( g , n , -6 , 6 );
  const long thr = long( g() % 21 ) - 10;
  std::size_t best = 0;
  for( std::size_t b = 0 ; b < n ; ++b )
   for( std::size_t e = b + 1 ; e <= n ; ++e )
    if( sumOf( D , b , e ) >= thr ) best = std::max( best , e - b );
  const auto S = longestSegmentAtLeast( D , thr );
  CHECK( S.length == best );
  if( best ) CHECK( S.sum >= thr &&
                    S.sum == sumOf( D , S.first , S.first + S.length ) );
  const long p = long( g() % 7 ) - 3 , q = 1 + long( g() % 3 );
  std::size_t bestAvg = 0;                  // sum / length >= p / q
  for( std::size_t b = 0 ; b < n ; ++b )
   for( std::size_t e = b + 1 ; e <= n ; ++e )
    if( q * sumOf( D , b , e ) >= p * long( e - b ) )
     bestAvg = std::max( bestAvg , e - b );
  CHECK( longestSegmentAverageAtLeast( D , p , q ).length == bestAvg );
  }
 }

// the segment of largest density with length at least L against all the
// segments, compared as fractions; the measures of GC-richness of a DNA
// sequence
ADS_TEST( "ch2.density" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = 1 + std::size_t( t % 40 );
  const auto D = randomVector( g , n , -8 , 8 );
  const std::size_t L = 1 + g() % n;
  long bs = 0;
  std::size_t bl = 0;
  for( std::size_t b = 0 ; b < n ; ++b )
   for( std::size_t e = b + L ; e <= n ; ++e )
    if( ( bl == 0 ) || ( sumOf( D , b , e ) * long( bl ) >
                         bs * long( e - b ) ) ) {
     bs = sumOf( D , b , e );
     bl = e - b;
     }
  const auto S = maxDensitySegment( D , L );
  CHECK( S.length >= L && S.sum * long( bl ) == bs * long( S.length ) );
  CHECK( S.sum == sumOf( D , S.first , S.first + S.length ) );
  }
 const std::string dna = "ATGCGCGATATTTACGGCGCCGTAAATCGATTAGCGCGCGTA";
 const auto I = gcIndicator( dna );
 std::vector< long > X( I.begin() , I.end() );
 CHECK( std::accumulate( X.begin() , X.end() , 0L ) == 22 );
 std::vector< long > E( X.size() );         // x - l / 2 as 2 x - l
 for( std::size_t k = 0 ; k < X.size() ; ++k ) E[ k ] = 2 * X[ k ] - 1;
 const auto R = maxSumSegment( E );
 CHECK( R.sum == maxSumSegmentCubic( E ).sum && R.sum == 8 );
 const auto Dn = maxDensitySegment( X , 8 );   // GCGCGCGC
 CHECK( Dn.sum == long( Dn.length ) && Dn.length >= 8 );
 const auto Lg = longestSegmentAverageAtLeast( X , 2L , 3L );
 CHECK( 3 * Lg.sum >= 2 * long( Lg.length ) && Lg.length == 15 );
 CHECK( maxDensitySegment( X , 1 ).sum == 1 );  // a single C or G
 CHECK( gcIndicator( "acgtN" ) == std::vector< int >( { 0 , 1 , 1 , 0 ,
                                                        0 } ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 3 ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the chi-square of the frequencies of the subsets drawn by f, all the
/// subsets of n of N items equally likely
template< class F >
double subsetChiSquare( F f , std::size_t n , std::size_t N , long trials ,
                        std::size_t subsets )
{
 std::map< std::vector< std::size_t > , long > freq;
 for( long t = 0 ; t < trials ; ++t ) {
  const auto s = f( n , N );
  if( ( s.size() != n ) || ! std::is_sorted( s.begin() , s.end() ) )
   return( 1e18 );
  ++freq[ s ];
  }
 std::vector< long > Y;
 for( const auto & [ s , c ] : freq ) Y.push_back( c );
 Y.resize( subsets , 0 );                   // the subsets never drawn
 return( chiSquare( Y , std::vector< double >( subsets ,
                                               1.0 / double( subsets ) ) ) );
 }

// the four samplings of positions: every subset of 3 of 6 positions equally
// likely (20 subsets, the 99.9% quantile of chi-square with 19 degrees of
// freedom is 43.8), and of 2 of 40 for the skips, which then run
// Algorithm D (780 subsets, the quantile with 779 degrees is about 910)
ADS_TEST( "ch3.samplers-uniform" )
{
 std::mt19937 g( 1 );
 auto swaps = [ & ]( std::size_t n , std::size_t N ) {
  return( sampleBySwaps( n , N , g ) ); };
 auto dict = [ & ]( std::size_t n , std::size_t N ) {
  return( sampleByDictionary( n , N , g ) ); };
 auto sorting = [ & ]( std::size_t n , std::size_t N ) {
  return( sampleBySorting( n , N , g ) ); };
 auto skips = [ & ]( std::size_t n , std::size_t N ) {
  return( sampleBySkips( n , N , g ) ); };
 CHECK( subsetChiSquare( swaps , 3 , 6 , 20000 , 20 ) < 43.8 );
 CHECK( subsetChiSquare( dict , 3 , 6 , 20000 , 20 ) < 43.8 );
 CHECK( subsetChiSquare( dict , 4 , 6 , 15000 , 15 ) < 36.2 );
 CHECK( subsetChiSquare( sorting , 3 , 6 , 20000 , 20 ) < 43.8 );
 CHECK( subsetChiSquare( sorting , 5 , 6 , 6000 , 6 ) < 20.5 );
 CHECK( subsetChiSquare( skips , 3 , 6 , 20000 , 20 ) < 43.8 );
 CHECK( subsetChiSquare( skips , 2 , 40 , 78000 , 780 ) < 920 );
 std::vector< long > Y( 1000 , 0 );         // positions of 5 of 1000
 for( int t = 0 ; t < 20000 ; ++t )
  for( auto p : sampleBySkips( 5 , 1000 , g ) ) ++Y[ p ];
 CHECK( chiSquare( Y , std::vector< double >( 1000 , 0.001 ) ) < 1150 );
 for( std::size_t N : { 0 , 1 , 7 , 100 } )
  for( std::size_t n : { std::size_t( 0 ) , N / 3 , N } ) {
   std::vector< std::size_t > all( n );
   std::iota( all.begin() , all.end() , std::size_t( 0 ) );
   for( const auto & s : { sampleBySwaps( n , N , g ) ,
                           sampleByDictionary( n , N , g ) ,
                           sampleBySorting( n , N , g ) ,
                           sampleBySkips( n , N , g ) } ) {
    CHECK( s.size() == n && std::is_sorted( s.begin() , s.end() ) );
    if( n == N ) CHECK( s == all );
    }
   }
 CHECK_THROWS( sampleBySkips( 4 , 3 , g ) , contract_violation );
 CHECK_THROWS( sampleBySwaps( 4 , 3 , g ) , contract_violation );
 }

// the random numbers of the skips against the N of the scan, and the
// rounds of the sorting, few for n much smaller than sqrt( N )
ADS_TEST( "ch3.sampler-costs" )
{
 std::mt19937 g( 1 );
 std::size_t randoms = 0 , rounds = 0;
 const auto s = sampleBySkips( 100 , 1000000 , g , &randoms );
 CHECK( s.size() == 100 && randoms <= 4 * 100 + 2 );
 sampleBySkips( 300 , 1000 , g , &randoms ); // Algorithm A
 CHECK( randoms == 300 );
 sampleBySorting( 10 , 1000000 , g , &rounds );
 CHECK( rounds == 1 );
 sampleBySorting( 5000 , 10000 , g , &rounds );
 CHECK( rounds >= 1 && rounds <= 40 );
 }

// the sampling from a stream of unknown length: each item with
// probability n / N (the 99.9% quantile with 9 degrees is 27.9), the
// sample in the order of the stream, like the reservoir
ADS_TEST( "ch3.heap-sampling" )
{
 std::mt19937 g( 1 );
 std::vector< int > stream( 10 );
 std::iota( stream.begin() , stream.end() , 0 );
 std::vector< long > H( 10 , 0 ) , R( 10 , 0 );
 for( int t = 0 ; t < 10000 ; ++t ) {
  const auto h = heapSampling( stream , 3 , g );
  CHECK( h.size() == 3 && std::is_sorted( h.begin() , h.end() ) );
  for( int i : h ) ++H[ i ];
  for( int i : reservoirSampling( stream , 3 , g ) ) ++R[ i ];
  }
 CHECK( chiSquare( H , std::vector< double >( 10 , 0.1 ) ) < 27.9 );
 CHECK( chiSquare( R , std::vector< double >( 10 , 0.1 ) ) < 27.9 );
 CHECK( heapSampling( stream , 10 , g ) == stream );
 CHECK( heapSampling( stream , 0 , g ).empty() );
 CHECK_THROWS( heapSampling( stream , 11 , g ) , contract_violation );
 }

// the sampled items read from the disk in the order of their positions:
// at most min{ m , ceil( N / B ) } reads (Facts 3.1-3.3)
ADS_TEST( "ch3.sample-ios" )
{
 std::mt19937 g( 1 );
 TwoLevelMemory mem( 16 , 64 );
 std::vector< long > items( 10000 );
 std::iota( items.begin() , items.end() , 0L );
 const DiskFile< long > S( mem , items );
 for( std::size_t m : { 1 , 10 , 200 , 5000 , 10000 } ) {
  const auto pos = sampleBySorting( m , items.size() , g );
  mem.resetCounters();
  DiskReader< long > rd( S );
  for( auto p : pos ) CHECK( rd.at( p ) == long( p ) );
  CHECK( mem.reads() <= std::min< std::uint64_t >( m , 625 ) );
  CHECK( mem.writes() == 0 );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 4 ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the list of Figure 4.1, 2 5 4 6 1 3, the ranks 1 5 0 3 4 2 (the items
// numbered from 0 here), by all the algorithms, the pointer jumping in the
// 3 rounds of Figure 4.2
ADS_TEST( "ch4.list-ranking-example" )
{
 const std::vector< std::size_t > succ = { 2 , 4 , 2 , 5 , 3 , 0 } ,
                                  rank = { 1 , 5 , 0 , 3 , 4 , 2 };
 std::mt19937 g( 1 );
 std::size_t rounds = 0;
 CHECK( listRank( succ ) == rank );
 CHECK( listRankPointerJumping( succ , &rounds ) == rank && rounds == 3 );
 CHECK( listRankIndependentSet( succ , g ) == rank );
 CHECK( listRankIndependentSet( succ , g , true ) == rank );
 TwoLevelMemory mem( 2 , 16 );
 const DiskFile< std::size_t > S( mem , succ );
 DiskFile< std::size_t > R1( mem ) , R2( mem );
 externalPointerJumping( S , R1 , &rounds );
 externalListRank( S , R2 , g );
 CHECK( R1.contents() == rank && rounds == 3 && R2.contents() == rank );
 CHECK( listRank( std::vector< std::size_t >{ 0 } ) ==
        std::vector< std::size_t >{ 0 } );
 CHECK( listRank( std::vector< std::size_t >() ).empty() );
 CHECK( ! isList( { 1 , 0 } ) && ! isList( { 0 , 1 } ) &&
        ! isList( { 1 , 2 , 1 } ) && isList( { 1 , 2 , 2 } ) );
 CHECK_THROWS( listRank( { 1 , 0 , 2 } ) , contract_violation );
 }

// the parallel algorithms against the sequential one on random lists,
// ceil( log2( n - 1 ) ) rounds of pointer jumping
ADS_TEST( "ch4.ranking-random" )
{
 std::mt19937 g( 1 );
 for( std::size_t n = 1 ; n <= 300 ; n += 1 + n / 10 ) {
  const auto succ = randomList( g , n );
  const auto rank = listRank( succ );
  std::size_t rounds = 0 , levels = 0;
  CHECK( listRankPointerJumping( succ , &rounds ) == rank );
  CHECK( rounds == ( n >= 2 ? ceilLog2( n - 1 ) : 0 ) );
  CHECK( listRankIndependentSet( succ , g , false , &levels ) == rank );
  CHECK( levels <= 8 * ceilLog2( n + 1 ) + 1 );
  CHECK( listRankIndependentSet( succ , g , true , &levels ) == rank );
  CHECK( levels <= 8 * ceilLog2( n + 1 ) + 1 );
  }
 }

// the independent sets: random ones of about n / 4 items, the local minima
// of three colors, made in log* n rounds, of at least about n / 4
ADS_TEST( "ch4.independent-sets" )
{
 std::mt19937 g( 1 );
 for( std::size_t n : { 1 , 2 , 3 , 10 , 1000 , 100000 } ) {
  const auto succ = randomList( g , n );
  std::size_t rounds = 0;
  const auto color = deterministicCoinTossing( succ , &rounds );
  CHECK( rounds <= 5 );
  const auto I = independentSetOfColors( succ , color );
  const auto k = std::size_t( std::count( I.begin() , I.end() , true ) );
  CHECK( 4 * k + 4 >= n );
  CHECK( ( n < 2 ) || ( k >= 1 ) );
  const auto Ir = randomIndependentSet( succ , g );
  const auto kr = std::size_t( std::count( Ir.begin() , Ir.end() , true ) );
  if( n >= 1000 ) CHECK( 8 * kr >= n && 8 * kr <= 3 * n );
  }
 const std::vector< std::size_t > path = { 1 , 2 , 3 , 3 };
 const auto c = deterministicCoinTossing( path );
 CHECK( c[ 0 ] != c[ 1 ] && c[ 1 ] != c[ 2 ] && c[ 2 ] != c[ 3 ] );
 CHECK_THROWS( independentSetOfColors( path , { 0 , 0 , 1 , 2 } ) ,
               contract_violation );
 }

// a parallel step on the disk against the same step in memory, two sorts
// and three scans
ADS_TEST( "ch4.parallel-step" )
{
 using Pair = std::pair< std::size_t , std::size_t >;
 std::mt19937 g( 1 );
 for( std::size_t n : { 0 , 1 , 7 , 100 , 3000 } ) {
  TwoLevelMemory mem( 4 , 32 );
  const auto A = randomVector( g , n , -100 , 100 );
  std::vector< std::size_t > dest( n );
  std::iota( dest.begin() , dest.end() , std::size_t( 0 ) );
  std::shuffle( dest.begin() , dest.end() , g );
  std::vector< Pair > ops;
  std::vector< long > ref( A );
  for( std::size_t k = 0 ; k < n / 2 ; ++k ) {
   const std::size_t b = g() % n;
   ops.push_back( Pair( dest[ k ] , b ) );
   ref[ dest[ k ] ] = A[ dest[ k ] ] + 2 * A[ b ];
   }
  const DiskFile< long > FA( mem , A );
  const DiskFile< Pair > FO( mem , ops );
  DiskFile< long > out( mem );
  mem.resetCounters();
  parallelStep( FA , FO , []( long x , long y ) { return( x + 2 * y ); } ,
                out );
  CHECK( out.contents() == ref );
  const std::size_t m = ops.size();
  CHECK( mem.ios() <= 2 * mergeSortIOs( m , 32 , 4 , 7 ) +
                      5 * mem.blocks( m ) + 3 * mem.blocks( n ) );
  CHECK( mem.peak() <= 32 );
  }
 }

// the rankings on the disk against the sequential ranking on random lists,
// with their I/Os: log n rounds of two sorts for the pointer jumping, a
// geometric sum of a few sorts for the recursion
ADS_TEST( "ch4.external-ranking" )
{
 std::mt19937 g( 1 );
 for( std::size_t n : { 2 , 3 , 50 , 1000 , 4000 } ) {
  TwoLevelMemory mem( 8 , 64 );
  const auto succ = randomList( g , n );
  const auto rank = listRank( succ );
  const DiskFile< std::size_t > S( mem , succ );
  DiskFile< std::size_t > R1( mem ) , R2( mem );
  std::size_t rounds = 0 , levels = 0;
  mem.resetCounters();
  externalPointerJumping( S , R1 , &rounds );
  const std::uint64_t jump = mem.ios();
  CHECK( R1.contents() == rank && rounds == ceilLog2( n - 1 ) );
  const std::uint64_t sortN = mergeSortIOs( n , 64 , 8 , 7 ) ,
                      scanN = mem.blocks( n );
  CHECK( jump <= rounds * ( 2 * sortN + 12 * scanN ) + 5 * scanN );
  CHECK( mem.peak() <= 64 );
  mem.resetCounters();
  externalListRank( S , R2 , g , &levels );
  const std::uint64_t dc = mem.ios();
  CHECK( R2.contents() == rank );
  CHECK( dc <= 8 * ( 6 * sortN + 30 * scanN ) );
  CHECK( mem.peak() <= 64 );
  if( n >= 1000 ) CHECK( dc < jump );      // log n rounds lose
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 5 ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the model: a scan costs ceil( n / B ) reads, a reader rereads a block
// only when it leaves it, a writer writes the partial block at the end,
// and the memory cannot be exceeded
ADS_TEST( "ch5.two-level-memory" )
{
 TwoLevelMemory mem( 4 , 12 );
 CHECK( mem.blockSize() == 4 && mem.memorySize() == 12 &&
        mem.blocks( 0 ) == 0 && mem.blocks( 9 ) == 3 );
 std::vector< int > v( 10 );
 std::iota( v.begin() , v.end() , 0 );
 const DiskFile< int > F( mem , v );
 CHECK( F.blocks() == 3 && mem.ios() == 0 );
 DiskFile< int > G( mem );
 {
  DiskReader< int > rd( F );
  DiskWriter< int > w( G );
  CHECK( mem.used() == 8 );
  while( rd.hasNext() ) w.push( rd.next() );
  CHECK( mem.reads() == 3 && mem.writes() == 2 );
  }
 CHECK( mem.writes() == 3 && G.contents() == v && mem.used() == 0 );
 mem.resetCounters();
 {
  DiskReader< int > rd( F );
  CHECK( rd.at( 1 ) == 1 && rd.at( 3 ) == 3 && rd.at( 0 ) == 0 );
  CHECK( mem.reads() == 1 );
  CHECK( rd.at( 9 ) == 9 && rd.at( 2 ) == 2 && mem.reads() == 3 );
  CHECK( rd.next() == 3 && mem.reads() == 3 );
  }
 {
  MemoryLease a( mem , 8 );
  CHECK_THROWS( MemoryLease( mem , 5 ) , contract_violation );
  CHECK( mem.peak() == 8 );
  }
 int buf[ 4 ] = { 1 , 2 , 3 , 4 };
 DiskFile< int > H( mem );
 H.appendBlock( buf , 2 );
 CHECK_THROWS( H.appendBlock( buf , 2 ) , contract_violation );
 CHECK_THROWS( H.readBlock( 1 , buf ) , contract_violation );
 CHECK_THROWS( TwoLevelMemory( 4 , 7 ) , contract_violation );
 }

// the merge of k sorted runs through the tree of losers against
// std::stable_sort, ceil( log2 k ) comparisons per item, stable
ADS_TEST( "ch5.loser-tree" )
{
 std::mt19937 g( 1 );
 for( std::size_t k = 1 ; k <= 33 ; k += 2 ) {
  std::vector< std::vector< std::pair< int , std::size_t > > > R( k );
  std::vector< std::pair< int , std::size_t > > all;
  std::size_t n = 0;
  for( std::size_t i = 0 ; i < k ; ++i ) {
   const std::size_t len = g() % 40;        // some runs empty
   for( std::size_t j = 0 ; j < len ; ++j )
    R[ i ].push_back( std::pair( int( g() % 20 ) , i ) );
   std::sort( R[ i ].begin() , R[ i ].end() );
   all.insert( all.end() , R[ i ].begin() , R[ i ].end() );
   n += len;
   }
  long cmp = 0;
  auto byKey = [ & ]( const auto & a , const auto & b ) {
   if( ! checkingContract() ) ++cmp;
   return( a.first < b.first ); };
  const auto out = kWayMerge( R , byKey );
  std::stable_sort( all.begin() , all.end() , []( const auto & a ,
                                                  const auto & b ) {
   return( a.first < b.first ); } );
  CHECK( out == all );                      // stable: by run on ties
  CHECK( cmp <= long( n * ceilLog2( k ) + k ) );
  }
 LoserTree< int > L( { 5 , std::nullopt , 2 , 9 } );
 CHECK( L.players() == 4 && L.winner() == 2 && L.top() == 2 );
 L.replace( 7 );
 CHECK( L.winner() == 0 && L.top() == 5 );
 L.replace( std::nullopt );
 L.replace( std::nullopt );
 L.replace( std::nullopt );
 CHECK( L.isEmpty() );
 CHECK_THROWS( L.top() , contract_violation );
 CHECK_THROWS( LoserTree< int >( {} ) , contract_violation );
 }

// the binary and the multiway mergesorts against std::sort: the I/Os of
// the analysis (exactly when no run is left alone in a pass), never below
// the lower bound, and the memory within M; the snow plow, whose runs are
// at least M - 2 B long and not aligned to the blocks, makes a single run
// of a sorted input
ADS_TEST( "ch5.merge-sorts" )
{
 std::mt19937 g( 1 );
 const std::size_t B = 4 , M = 32;
 for( std::size_t n : { 0 , 1 , 4 , 32 , 33 , 224 , 256 , 1000 , 5000 } ) {
  const auto v = randomVector( g , n , 0 , 1000 );
  auto sorted = v;
  std::sort( sorted.begin() , sorted.end() );
  TwoLevelMemory mem( B , M );
  const DiskFile< long > in( mem , v );
  for( int alg = 0 ; alg < 3 ; ++alg ) {
   DiskFile< long > out( mem );
   mem.resetCounters();
   if( alg == 0 ) binaryMergeSort( in , out );
   else multiwayMergeSort( in , out , std::less<>() , alg == 2 );
   CHECK( out.contents() == sorted && mem.peak() <= M );
   CHECK( mem.ios() >= sortingLowerBound( n , M , B ) );
   if( alg < 2 )
    CHECK( mem.ios() <= mergeSortIOs( n , M , B , alg == 0 ? 2 : 7 ) );
   else {                                   // runs of M - 2 B at least
    const std::size_t r = ( n + M - 2 * B - 1 ) / ( M - 2 * B );
    const std::size_t passes = r > 1 ? std::size_t( std::ceil(
                               std::log( double( r ) ) / std::log( 7.0 ) -
                               1e-9 ) ) : 0;
    CHECK( mem.ios() <= 2 * ( mem.blocks( n ) + r ) * ( 1 + passes ) );
    }
   }
  }
 TwoLevelMemory mem( B , M );
 std::vector< long > v( 256 );
 std::iota( v.begin() , v.end() , 0L );
 const DiskFile< long > in( mem , v );
 DiskFile< long > o1( mem ) , o2( mem ) , o3( mem );
 binaryMergeSort( in , o1 );
 CHECK( mem.ios() == 512 );                 // 8 runs, 3 passes
 mem.resetCounters();
 multiwayMergeSort( in , o2 , std::less<>() , true );
 CHECK( mem.ios() == 128 && o2.contents() == v );   // a single run
 const DiskFile< long > in7( mem , std::vector< long >( 224 , 3 ) );
 mem.resetCounters();
 multiwayMergeSort( in7 , o3 );
 CHECK( mem.ios() == 224 );                 // 7 runs, 1 pass
 DiskFile< long > bad( mem , std::vector< long >{ 1 } );
 CHECK_THROWS( binaryMergeSort( in , bad ) , contract_violation );
 TwoLevelMemory small( 4 , 8 );
 const DiskFile< long > tiny( small , v );
 DiskFile< long > to( small );
 CHECK_THROWS( multiwayMergeSort( tiny , to ) , contract_violation );
 }

// the lower bounds: at least the scans, growing as the logarithm of n /
// M in base M / B, and below the I/Os of every algorithm that sorts or
// permutes
ADS_TEST( "ch5.lower-bounds" )
{
 CHECK( sortingLowerBound( 0 , 32 , 4 ) == 0 );
 CHECK( sortingLowerBound( 32 , 32 , 4 ) == 16 );
 CHECK( permutingLowerBound( 32 , 32 , 4 ) == 16 );
 std::uint64_t prev = 0;
 for( std::size_t n = 1000 ; n <= 64000 ; n *= 2 ) {
  const auto s = sortingLowerBound( n , 64 , 8 );
  CHECK( s >= 2 * ( n / 8 ) && s >= prev );
  CHECK( s <= mergeSortIOs( n , 64 , 8 , 7 ) );
  CHECK( permutingLowerBound( n , 64 , 8 ) <= s + n / 8 );
  prev = s;
  }
 // log2 n! <= ceil( n / B ) log2 B! + t log2( M choose B ): for n = 4096,
 // M = 64, B = 8 the reads must be more than 2 n / B
 CHECK( sortingLowerBound( 4096 , 64 , 8 ) > 3 * 512 );
 CHECK_THROWS( sortingLowerBound( 10 , 7 , 4 ) , contract_violation );
 }

// the permutations by two sorts and by one read per item, against the
// definition and the lower bound
ADS_TEST( "ch5.permute" )
{
 std::mt19937 g( 1 );
 for( std::size_t n : { 0 , 1 , 50 , 2000 } ) {
  TwoLevelMemory mem( 8 , 64 );
  const auto v = randomVector( g , n , -50 , 50 );
  std::vector< std::size_t > pi( n );
  std::iota( pi.begin() , pi.end() , std::size_t( 0 ) );
  std::shuffle( pi.begin() , pi.end() , g );
  std::vector< long > ref( n );
  for( std::size_t i = 0 ; i < n ; ++i ) ref[ i ] = v[ pi[ i ] ];
  const DiskFile< long > in( mem , v );
  const DiskFile< std::size_t > P( mem , pi );
  DiskFile< long > o1( mem ) , o2( mem );
  mem.resetCounters();
  permuteBySorting( in , P , o1 );
  CHECK( o1.contents() == ref && mem.peak() <= 64 );
  CHECK( mem.ios() >= permutingLowerBound( n , 64 , 8 ) );
  CHECK( mem.ios() <= 2 * mergeSortIOs( n , 64 , 8 , 7 ) +
                      7 * mem.blocks( n ) );
  mem.resetCounters();
  permuteDirect( in , P , o2 );
  CHECK( o2.contents() == ref );
  CHECK( mem.ios() >= permutingLowerBound( n , 64 , 8 ) );
  CHECK( mem.reads() <= n + mem.blocks( n ) &&
         mem.writes() == mem.blocks( n ) );
  }
 TwoLevelMemory mem( 8 , 64 );
 const DiskFile< long > in( mem , { 1 , 2 } );
 const DiskFile< std::size_t > P( mem , { 1 , 1 } );
 DiskFile< long > o( mem );
 CHECK_THROWS( permuteDirect( in , P , o ) , contract_violation );
 }

// the distribution sort against std::sort, also with many equal keys, with
// the I/Os of a sort and the memory within M
ADS_TEST( "ch5.distribution-sort" )
{
 std::mt19937_64 g( 1 );
 std::mt19937 h( 1 );
 const std::size_t B = 4 , M = 64;
 for( std::size_t n : { 0 , 1 , 60 , 61 , 500 , 5000 , 20000 } )
  for( long range : { 1L , 4L , 1000000L } ) {
   auto v = randomVector( h , n , 0 , range - 1 );
   if( range == 4 && n == 5000 ) std::sort( v.begin() , v.end() );
   auto sorted = v;
   std::sort( sorted.begin() , sorted.end() );
   TwoLevelMemory mem( B , M );
   const DiskFile< long > in( mem , v );
   DiskFile< long > out( mem );
   mem.resetCounters();
   distributionSort( in , out , g );
   CHECK( out.contents() == sorted && mem.peak() <= M );
   CHECK( mem.ios() >= sortingLowerBound( n , M , B ) || range < 1000 );
   CHECK( mem.ios() <= 3 * mergeSortIOs( n , M , B , 7 ) + 2 * n / B );
   }
 TwoLevelMemory mem( 4 , 21 );
 const DiskFile< long > in( mem , { 3 , 1 } );
 DiskFile< long > out( mem );
 CHECK_THROWS( distributionSort( in , out , g ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CHAPTER 6 ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the examples of the notes: Figure 6.3, the query abaco mathematics of
// Figure 6.2, the lists of Figure 6.5
ADS_TEST( "ch6.intersection-example" )
{
 using Inter = std::vector< long > ( * )( const std::vector< long > & ,
                                          const std::vector< long > & ,
                                          std::less<> );
 const std::vector< Inter > alg = { &intersectMerge< long , std::less<> > ,
                         &intersectBinarySearch< long , std::less<> > ,
                         &intersectMutualPartition< long , std::less<> > ,
                         &intersectDoubling< long , std::less<> > };
 const std::vector< long > A = { 1 , 3 , 7 , 12 , 16 , 19 , 20 , 25 , 27 ,
                                 40 , 50 , 100 } ,
                           B = { 2 , 5 , 6 , 10 , 12 , 16 , 20 , 29 , 32 } ,
                     abaco = { 10 , 23 , 50 } ,
                     math = { 1 , 3 , 7 , 10 , 15 , 18 , 23 , 30 , 40 , 70 };
 for( const auto f : alg ) {
  CHECK( f( A , B , std::less<>() ) == std::vector< long >( { 12 , 16 ,
                                                              20 } ) );
  CHECK( f( abaco , math , std::less<>() ) ==
         std::vector< long >( { 10 , 23 } ) );
  CHECK( f( math , abaco , std::less<>() ) ==
         std::vector< long >( { 10 , 23 } ) );
  CHECK( f( A , std::vector< long >() , std::less<>() ).empty() );
  }
 for( std::size_t L : { 1 , 2 , 3 , 32 } )
  CHECK( intersectTwoLevel( A , B , L ) ==
         std::vector< long >( { 12 , 16 , 20 } ) );
 const PermutedBuckets PA( { 1 , 2 , 3 , 4 , 7 , 8 , 11 , 12 } , 4 , 2 ,
                           5 , 1 ) ,
                       PB( { 2 , 4 , 6 , 12 } , 4 , 2 , 5 , 1 );
 CHECK( PA.intersect( PB ) == std::vector< std::uint64_t >( { 2 , 4 ,
                                                              12 } ) );
 CHECK( PA.buckets() == 4 && PB.buckets() == 2 );
 CHECK( PA.values() == std::vector< std::uint64_t >( { 1 , 2 , 3 , 4 , 7 ,
                                                       8 , 11 , 12 } ) );
 CHECK_THROWS( intersectMerge( std::vector< long >( { 2 , 1 } ) , B ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( { 16 } , 4 , 2 , 5 , 1 ) ,
               contract_violation );
 CHECK_THROWS( PermutedBuckets( { 1 } , 4 , 2 , 4 , 1 ) ,
               contract_violation );
 CHECK_THROWS( PA.intersect( PermutedBuckets( { 2 } , 4 , 2 , 7 , 1 ) ) ,
               contract_violation );
 }

// all the intersections against std::set_intersection on random sets of
// unequal lengths, with their comparisons: n + m by merging, O( m log n )
// by binary search, O( m ( 1 + log( n / m ) ) ) by doubling
ADS_TEST( "ch6.intersection-random" )
{
 std::mt19937 g( 1 );
 for( std::size_t m : { 0 , 1 , 10 , 100 , 1000 } )
  for( std::size_t n : { 0 , 1 , 1000 , 100000 } ) {
   const long U = 4 * long( std::max( n , m ) ) + 10;
   const auto A = randomSet( g , n , U );
   const auto B = randomSet( g , m , U );
   std::vector< long > ref;
   std::set_intersection( A.begin() , A.end() , B.begin() , B.end() ,
                          std::back_inserter( ref ) );
   long cmp = 0;
   auto less = [ & ]( long a , long b ) {
    if( ! checkingContract() ) ++cmp;
    return( a < b ); };
   const double lo = double( std::min( n , m ) ) ,
                hi = double( std::max( n , m ) );
   cmp = 0;
   CHECK( intersectMerge( A , B , less ) == ref );
   CHECK( cmp <= 2 * long( n + m ) );
   cmp = 0;
   CHECK( intersectBinarySearch( A , B , less ) == ref );
   CHECK( double( cmp ) <= 2 * lo * ( std::log2( hi + 1 ) + 2 ) );
   cmp = 0;
   CHECK( intersectDoubling( A , B , less ) == ref );
   CHECK( double( cmp ) <=
          lo * ( 4 * std::log2( hi / std::max( lo , 1.0 ) + 1 ) + 8 ) );
   cmp = 0;
   CHECK( intersectMutualPartition( A , B , less ) == ref );
   CHECK( double( cmp ) <=
          lo * ( 4 * std::log2( hi / std::max( lo , 1.0 ) + 1 ) + 8 ) );
   for( std::size_t L : { 1 , 8 , 64 } ) {
    cmp = 0;
    CHECK( intersectTwoLevel( A , B , L , less ) == ref );
    CHECK( cmp <= long( 4 * m + n / L + std::min( n , m * L ) + 2 ) );
    }
   }
 }

// the random permutation and splitting: the intersection, and few items
// of the longer list looked at when the other one is short
ADS_TEST( "ch6.permuted-buckets" )
{
 std::mt19937 g( 1 );
 for( std::size_t m : { 0 , 1 , 16 , 1000 } )
  for( std::size_t n : { 0 , 5 , 100000 } ) {
   const auto A = randomSet( g , n , 1L << 30 );
   auto B = randomSet( g , m , 1L << 30 );
   for( std::size_t k = 0 ; k < std::min( n , m ) / 2 ; ++k )
    B[ k ] = A[ g() % n ];                  // some common values
   std::sort( B.begin() , B.end() );
   B.erase( std::unique( B.begin() , B.end() ) , B.end() );
   std::vector< long > ref;
   std::set_intersection( A.begin() , A.end() , B.begin() , B.end() ,
                          std::back_inserter( ref ) );
   std::vector< std::uint64_t > UA( A.begin() , A.end() ) ,
                                UB( B.begin() , B.end() ) ,
                                R( ref.begin() , ref.end() );
   std::shuffle( UA.begin() , UA.end() , g );   // no order needed
   for( std::size_t L : { 1 , 8 } ) {
    const std::uint64_t a = ( g() | 1 ) , c = g();
    const PermutedBuckets PA( UA , 30 , L , a , c ) ,
                          PB( UB , 30 , L , a , c );
    std::size_t scanned = 0;
    CHECK( PA.intersect( PB , &scanned ) == R );
    CHECK( PB.intersect( PA ) == R );
    CHECK( scanned <= n + B.size() );
    if( n == 100000 && m == 16 )
     CHECK( scanned <= B.size() * ( 8 * L + 1 ) );
    }
   }
 }

// the intersections on the disk: the merge reads both lists once, the two
// levels read the headers, B, and at most ceil( L / B ) + 1 blocks of A
// for each item of B (Fact 6.5)
ADS_TEST( "ch6.intersection-ios" )
{
 std::mt19937 g( 1 );
 const std::size_t Bk = 8;
 for( std::size_t m : { 0 , 1 , 20 , 2000 } )
  for( std::size_t L : { 1 , 16 , 64 } ) {
   const std::size_t n = 50000;
   TwoLevelMemory mem( Bk , 64 );
   const auto A = randomSet( g , n , 200000 );
   const auto Bv = randomSet( g , m , 200000 );
   std::vector< long > ref;
   std::set_intersection( A.begin() , A.end() , Bv.begin() , Bv.end() ,
                          std::back_inserter( ref ) );
   const DiskFile< long > FA( mem , A ) , FB( mem , Bv );
   DiskFile< long > H( mem ) , o1( mem ) , o2( mem );
   twoLevelHeaders( FA , L , H );
   CHECK( H.size() == ( n + L - 1 ) / L );
   mem.resetCounters();
   intersectMergeExternal( FA , FB , o1 );
   CHECK( o1.contents() == ref );
   CHECK( mem.reads() <= mem.blocks( n ) + mem.blocks( m ) );
   CHECK( mem.writes() == mem.blocks( ref.size() ) );
   mem.resetCounters();
   intersectTwoLevelExternal( FA , H , L , FB , o2 );
   CHECK( o2.contents() == ref && mem.peak() <= 4 * Bk );
   CHECK( mem.reads() <= mem.blocks( H.size() ) + mem.blocks( m ) +
                         m * ( ( L + Bk - 1 ) / Bk + 1 ) );
   if( m == 20 && L == 16 ) CHECK( mem.reads() < mem.blocks( n ) / 4 );
   }
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
/*------------------------- End File pearls_io.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
