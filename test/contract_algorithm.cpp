/*--------------------------------------------------------------------------*/
/*---------------------- File contract_algorithm.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the binary counter, the towers of
 * Hanoi, the master theorem, the Monte Carlo algorithms and the subsequences:
 * the results as the specifications of the book state them, and the violations
 * of the preconditions, which must throw ADS::contract_violation. The tests
 * are compiled at the level 3, so that each call also checks its
 * postconditions and its audits, on random instances against a reference of
 * the standard library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 2.6 and
 *      4.4, https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.4,
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

#include "ADS/algorithm/AkraBazzi.h"
#include "ADS/algorithm/BinaryCounter.h"
#include "ADS/algorithm/Hanoi.h"
#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/algorithm/MaximumSegments.h"
#include "ADS/algorithm/MonteCarlo.h"
#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/algorithm/Subsequences.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <tuple>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the counter of k bits counts 0 , 1 , ... , 2^k - 1 and back to 0, at an
// amortized cost below 2; an item that is not a bit (2 or -1) violates the
// contract; no bit at all, and all ones wrapping around to 0
ADS_TEST( "binary-counter" )
{
 for( std::size_t k : { 1u , 4u , 10u } ) {
  Ints A( k , 0 );
  std::size_t cost = 0;
  for( std::size_t v = 1 ; v <= ( std::size_t( 1 ) << k ) ; ++v ) {
   cost += add( A );
   std::size_t x = 0;
   for( std::size_t i = k ; i-- > 0 ; ) x = 2 * x + std::size_t( A[ i ] );
   CHECK( x == v % ( std::size_t( 1 ) << k ) );
   }
  CHECK( cost < 2 * ( std::size_t( 1 ) << k ) + k );
  }
 Ints B = { 1 , 2 , 0 };
 CHECK_THROWS( (void) add( B ) , contract_violation );
 Ints N = { 0 , -1 };
 CHECK_THROWS( (void) add( N ) , contract_violation );
 Ints Z;                                  // no bit: nothing to change
 CHECK( add( Z ) == 0 && Z.empty() );
 Ints F( 5 , 1 );                         // all ones: back to 0, cost k
 CHECK( add( F ) == 5 && F == Ints( 5 , 0 ) );
 }

// the three towers of Hanoi make the same 2^n - 1 moves, each legal; no
// disk and two equal pegs violate the contracts: each condition of each
// precondition once, n = 1 just inside and n = 0 , -1 just outside
ADS_TEST( "hanoi" )
{
 for( int n = 1 ; n <= 10 ; ++n ) {
  std::vector< std::pair< int , int > > M[ 3 ];
  hanoi_recursive( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 0 ].emplace_back( a , b ); } );
  hanoi_iterative( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 1 ].emplace_back( a , b ); } );
  hanoi_iterative_labels( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 2 ].emplace_back( a , b ); } );
  CHECK( M[ 0 ].size() == ( std::size_t( 1 ) << n ) - 1 );
  CHECK( M[ 0 ] == M[ 1 ] && M[ 1 ] == M[ 2 ] );
  std::vector< Ints > peg( 3 );                  // the moves are legal
  for( int d = n ; d >= 1 ; --d ) peg[ 0 ].push_back( d );
  for( auto [ a , b ] : M[ 0 ] ) {
   CHECK( ! peg[ a ].empty() &&
          ( peg[ b ].empty() || peg[ b ].back() > peg[ a ].back() ) );
   peg[ b ].push_back( peg[ a ].back() );
   peg[ a ].pop_back();
   }
  CHECK( int( peg[ 2 ].size() ) == n );
  }
 auto none = []( int , int ) {};
 CHECK_THROWS( hanoi_recursive( 0 , 0 , 2 , 1 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative( 3 , 0 , 0 , 1 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative_labels( 3 , 0 , 2 , 2 , none ) ,
               contract_violation );
 // each condition of each precondition, n = 1 just inside, 0 and -1 out
 int moves = 0;
 auto count = [ & ]( int , int ) { ++moves; };
 hanoi_recursive( 1 , 0 , 2 , 1 , count );
 hanoi_iterative( 1 , 2 , 0 , 1 , count );
 hanoi_iterative_labels( 1 , 1 , 0 , 2 , count );
 CHECK( moves == 3 );
 for( auto [ n , s , d , m ] : { std::tuple( 0 , 0 , 2 , 1 ) ,
                                 std::tuple( -1 , 0 , 2 , 1 ) ,
                                 std::tuple( 3 , 1 , 1 , 2 ) ,
                                 std::tuple( 3 , 0 , 2 , 0 ) ,
                                 std::tuple( 3 , 0 , 1 , 1 ) } ) {
  CHECK_THROWS( hanoi_recursive( n , s , d , m , none ) ,
                contract_violation );
  CHECK_THROWS( hanoi_iterative( n , s , d , m , none ) ,
                contract_violation );
  CHECK_THROWS( hanoi_iterative_labels( n , s , d , m , none ) ,
                contract_violation );
  }
 }

// the three cases of the master theorem and the linear recurrences; a < 1
// and b <= 1 violate the contracts, a = 1 and b just above 1 do not, nor
// NaN; the exponent of Akra and Bazzi on one and two terms, p < 0, and
// no term, a <= 0, b = 0 , 1 , 1.5 and NaN violating its contract
ADS_TEST( "master" )
{
 CHECK( master( 2 , 2 , 1 ).q == 1 );            // Merge Sort: n log n
 CHECK( master( 8 , 2 , 2 ).p == 3 );            // the first case
 CHECK( master( 1 , 2 , 1 ).p == 1 );            // the third case
 CHECK( linear( 2 , 0 ).upper && linear( 1 , 1 ).p == 2 );
 CHECK_THROWS( (void) master( 0.5 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 2 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) linear( 0 , 1 ) , contract_violation );
 // the bounds a = 1 and b just above 1 inside, just below them outside
 const double below1 = std::nextafter( 1.0 , 0.0 ) ,
              above1 = std::nextafter( 1.0 , 2.0 );
 CHECK( master( 1 , 2 , 0 ).q == 1 );            // binary search: log n
 (void) master( 1 , above1 , 0 );
 (void) linear( 1 , 0 );
 CHECK_THROWS( (void) master( below1 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 2 , below1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 2 , 0.5 , 1 ) , contract_violation );
 CHECK_THROWS( (void) linear( below1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( std::nan( "" ) , 2 , 1 ) ,
               contract_violation );
 // Akra and Bazzi: one term at least, a > 0 and 0 < b < 1
 CHECK( std::abs( akraBazziExponent( { { 2 , 0.5 } } ) - 1 ) < 1e-9 );
 CHECK( std::abs( akraBazziExponent( { { 1 , 0.5 } } ) ) < 1e-9 );
 CHECK( std::abs( akraBazziExponent( { { 1 , 1.0 / 3 } ,
                                       { 1 , 2.0 / 3 } } ) - 1 ) < 1e-9 );
 (void) akraBazziExponent( { { 1e-6 , 0.5 } } );         // p < 0
 (void) akraBazziExponent( { { 2 , 1e-9 } , { 1 , below1 } } );
 (void) akraBazzi( { { 2 , 0.5 } } , 1 );
 CHECK_THROWS( (void) akraBazziExponent( {} ) , contract_violation );
 CHECK_THROWS( (void) akraBazzi( {} , 1 ) , contract_violation );
 CHECK_THROWS( (void) akraBazziExponent( { { 0 , 0.5 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) akraBazziExponent( { { 2 , 0.5 } , { -1 , 0.5 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) akraBazziExponent( { { 2 , 0 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) akraBazziExponent( { { 2 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) akraBazzi( { { 2 , 1.5 } } , 0 ) , contract_violation );
 CHECK_THROWS( (void) akraBazziExponent( { { std::nan( "" ) , 0.5 } } ) ,
               contract_violation );
 }

// Freivalds accepts the product and rejects a wrong one; the test of the
// null polynomial and the half-satisfiability; a negative number of
// repetitions and matrices that are not square violate the contracts
// (each of the three, a ragged row), K = 0 and empty matrices do not
ADS_TEST( "monte-carlo" )
{
 std::mt19937 g( 4 );
 using M = std::vector< std::vector< long > >;
 for( int t = 0 ; t < 50 ; ++t ) {
  const std::size_t n = 1 + g() % 6;
  M A( n , std::vector< long >( n ) ) , B = A , C = A;
  for( auto & r : A ) for( auto & x : r ) x = long( g() % 7 ) - 3;
  for( auto & r : B ) for( auto & x : r ) x = long( g() % 7 ) - 3;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    for( std::size_t k = 0 ; k < n ; ++k )
     C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
  CHECK( freivalds( A , B , C , 20 , g ) );
  C[ g() % n ][ g() % n ] += 1;
  CHECK( ! freivalds( A , B , C , 40 , g ) );
  }
 auto zero = []( const std::vector< long > & x ) {
  return( ( x[ 0 ] + x[ 1 ] ) * ( x[ 0 ] - x[ 1 ] ) - x[ 0 ] * x[ 0 ] +
          x[ 1 ] * x[ 1 ] ); };
 auto one = []( const std::vector< long > & x ) { return( x[ 0 ] - x[ 1 ] ); };
 CHECK( zeroPolynomial( zero , 2 , 2 , 10 , g ) );
 CHECK( ! zeroPolynomial( one , 2 , 1 , 30 , g ) );
 auto mostly = []( const std::vector< bool > & x ) {
  return( x[ 0 ] || x[ 1 ] ); };
 CHECK( halfSatisfiable( mostly , 2 , 400 , g ) );
 const M sq( 2 , std::vector< long >( 2 ) ) ,
         rect( 2 , std::vector< long >( 3 ) );
 CHECK_THROWS( (void) freivalds( sq , rect , sq , 1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) freivalds( sq , sq , sq , -1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) zeroPolynomial( zero , 2 , 2 , -1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) halfSatisfiable( mostly , 2 , -1 , g ) ,
               contract_violation );
 // K = 0 just inside: no evidence, the answer of no test; each matrix not
 // square once, a ragged row, the empty matrices
 CHECK( freivalds( sq , sq , sq , 0 , g ) );
 (void) zeroPolynomial( one , 2 , 1 , 0 , g );
 (void) halfSatisfiable( mostly , 2 , 0 , g );
 M ragged = sq;
 ragged[ 1 ].push_back( 0 );
 CHECK_THROWS( (void) freivalds( rect , sq , sq , 1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) freivalds( sq , sq , rect , 1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) freivalds( sq , ragged , sq , 1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) freivalds( sq , sq , M() , 1 , g ) ,
               contract_violation );
 CHECK( freivalds( M() , M() , M() , 3 , g ) );
 CHECK_THROWS( (void) zeroPolynomial( zero , 2 , 2 ,
                                      std::numeric_limits< int >::min() , g ) ,
               contract_violation );
 }

// the segments: each precondition just inside and just outside (an empty
// vector, L = 0, L > U, L > n, q = 0 and q < 0), one item, all items
// negative (the largest one alone), all equal, L = U = n; the gaps and the
// line assignment: an interval with a > b, a point interval, no interval,
// sizes that differ, the empty vectors
ADS_TEST( "segments" )
{
 const Ints E , O = { -7 } , Neg = { -3 , -1 , -2 } , D = { 2 , -5 , 3 , 1 };
 CHECK_THROWS( (void) maxSumSegmentCubic( E ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegmentQuadratic( E ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegment( E ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegmentPrefix( E ) , contract_violation );
 for( const Ints & A : { O , Neg , D , Ints( 4 , 2 ) } ) {
  const auto c = maxSumSegmentCubic( A ) , q = maxSumSegmentQuadratic( A ) ,
             s = maxSumSegment( A ) , p = maxSumSegmentPrefix( A );
  CHECK( c.sum == q.sum && q.sum == s.sum && s.sum == p.sum );
  CHECK( c.length >= 1 && s.length >= 1 && p.length >= 1 );
  }
 CHECK( maxSumSegment( O ).sum == -7 && maxSumSegment( Neg ).sum == -1 &&
        maxSumSegment( Neg ).first == 1 && maxSumSegment( D ).sum == 4 );
 // maxSumSegmentLength(): 1 <= L <= U, L <= n
 CHECK_THROWS( (void) maxSumSegmentLength( D , 0 , 2 ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegmentLength( D , 3 , 2 ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegmentLength( D , 5 , 9 ) , contract_violation );
 CHECK_THROWS( (void) maxSumSegmentLength( E , 1 , 1 ) , contract_violation );
 CHECK( maxSumSegmentLength( D , 4 , 4 ).sum == 1 );        // L = U = n
 CHECK( maxSumSegmentLength( D , 1 , 1 ).sum == 3 );
 CHECK( maxSumSegmentLength( D , 2 , 99 ).sum == 4 );       // U > n
 CHECK( maxSumSegmentLength( Neg , 1 , 3 ).sum == -1 );
 // longestSegmentAtLeast(): no precondition; nothing reaches t
 CHECK( longestSegmentAtLeast( E , 0 ).length == 0 );
 CHECK( longestSegmentAtLeast( Neg , 0 ).length == 0 );
 CHECK( longestSegmentAtLeast( Neg , -6 ).length == 3 );
 CHECK( longestSegmentAtLeast( O , -7 ).length == 1 );
 // longestSegmentAverageAtLeast(): q > 0
 CHECK_THROWS( (void) longestSegmentAverageAtLeast( D , 1 , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) longestSegmentAverageAtLeast( D , 1 , -1 ) ,
               contract_violation );
 CHECK( longestSegmentAverageAtLeast( D , 1 , 1 ).length == 2 );
 CHECK( longestSegmentAverageAtLeast( D , 1 , 4 ).length == 4 );
 CHECK( longestSegmentAverageAtLeast( E , 0 , 1 ).length == 0 );
 // maxDensitySegment(): 1 <= L <= n
 const std::vector< double > R = { 1 , 4 , 2 , 4 };
 CHECK_THROWS( (void) maxDensitySegment( R , 0 ) , contract_violation );
 CHECK_THROWS( (void) maxDensitySegment( R , 5 ) , contract_violation );
 CHECK_THROWS( (void) maxDensitySegment( std::vector< double >() , 1 ) ,
               contract_violation );
 CHECK( maxDensitySegment( R , 4 ).sum == 11 &&
        maxDensitySegment( R , 4 ).length == 4 );
 CHECK( maxDensitySegment( R , 1 ).sum == 4 &&
        maxDensitySegment( R , 1 ).length == 1 );
 // largestGap(): a <= b for each interval
 using I2 = std::vector< std::pair< int , int > >;
 CHECK_THROWS( (void) largestGap( I2{ { 3 , 2 } } ) , contract_violation );
 CHECK_THROWS( (void) largestGap( I2{ { 0 , 1 } , { 5 , 4 } } ) ,
               contract_violation );
 CHECK( ! largestGap( I2{} ) && ! largestGap( I2{ { 2 , 2 } } ) );
 CHECK( largestGap( I2{ { 1 , 1 } , { 3 , 3 } } ) == std::pair( 1 , 3 ) );
 CHECK( ! largestGap( I2{ { 0 , 2 } , { 2 , 4 } } ) );   // touching
 CHECK( largestGap( I2{ { 0 , 1 } , { 3 , 4 } , { 6 , 9 } } ) ==
        std::pair( 1 , 3 ) );                             // the first tie
 // lineAssignment(): the same size
 CHECK_THROWS( (void) lineAssignment( Ints{ 1 } , Ints{} ) ,
               contract_violation );
 CHECK_THROWS( (void) lineAssignment( Ints{} , Ints{ 2 } ) ,
               contract_violation );
 CHECK( lineAssignment( Ints{} , Ints{} ) == 0 );
 CHECK( lineAssignment( Ints{ 5 } , Ints{ 2 } ) == 3 );
 const int big = std::numeric_limits< int >::max();
 CHECK( ( lineAssignment< int , long long >( Ints{ -big , big } ,
                                             Ints{ big , -big } ) == 0 ) );
 CHECK( ( lineAssignment< int , long long >( Ints{ -big } , Ints{ big } ) ==
          2LL * big ) );
 }

// the longest increasing subsequence, checked by its audit against
// lisLength(), and the subvector of largest sum against the enumeration;
// the empty vector, one item, all negative, all equal, decreasing
ADS_TEST( "subsequences" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Ints A( g() % 40 );
  for( auto & a : A ) a = int( g() % 30 ) - 15;
  const Ints L = lis( A );
  CHECK( L.size() == lisLength( A ) );
  int best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i ; j < A.size() ; ++j ) {
    int s = 0;
    for( std::size_t h = i ; h <= j ; ++h ) s += A[ h ];
    best = std::max( best , s );
    }
  CHECK( maxSubarraySum( A ) == best );
  }
 // empty, one item, all negative (the empty subvector), all equal,
 // decreasing (an increasing subsequence of one item)
 CHECK( lis( Ints() ).empty() && lisLength( Ints() ) == 0 &&
        maxSubarraySum( Ints() ) == 0 );
 CHECK( lis( Ints( { 4 } ) ) == Ints( { 4 } ) &&
        maxSubarraySum( Ints( { -4 } ) ) == 0 );
 CHECK( lisLength( Ints( 6 , 3 ) ) == 1 && lis( Ints( 6 , 3 ) ).size() == 1 &&
        maxSubarraySum( Ints( 6 , 3 ) ) == 18 );
 CHECK( lisLength( Ints( { 5 , 4 , 3 , 2 } ) ) == 1 &&
        maxSubarraySum( Ints( { -5 , -4 } ) ) == 0 );
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
/*-------------------- End File contract_algorithm.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
