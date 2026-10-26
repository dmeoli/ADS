/*--------------------------------------------------------------------------*/
/*--------------------- File contract_optimization.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the problems of optimization:
 * each operation against its specification and the violations of its
 * preconditions, which must throw ADS::contract_violation. The tests are
 * compiled at the level 3, so that each operation also checks its
 * postconditions and audits, on random instances against a brute force or a
 * second algorithm.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 13 and
 *      14, https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/IntervalCover.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/optimization/KnapsackVariants.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/OptimalSearchTree.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/optimization/WeightedIntervals.h"

#include <algorithm>
#include <bit>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Longs = std::vector< long >;
using Intervals = std::vector< std::pair< long , long > >;

/// random intervals [ a , b ) with a < b
Intervals random_intervals( std::mt19937 & g , int n )
{
 Intervals I( g() % ( n + 1 ) );
 for( auto & [ a , b ] : I ) {
  a = long( g() % 30 );
  b = a + 1 + long( g() % 8 );
  }
 return( I );
 }

// greedyChange() and optimalChange(): an amount or a coin not allowed,
// and the change of random coins summing to the amount
ADS_TEST( "change" )
{
 CHECK_THROWS( greedyChange( { 1 , 2 } , -1 ) , contract_violation );
 CHECK_THROWS( greedyChange( { 0 , 2 } , 4 ) , contract_violation );
 CHECK_THROWS( optimalChange( { 1 , -2 } , 4 ) , contract_violation );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Longs coins( 1 + g() % 4 );
  for( auto & d : coins ) d = 1 + long( g() % 9 );
  const long c = long( g() % 60 );
  const auto G = greedyChange( coins , c );
  const auto O = optimalChange( coins , c );
  CHECK( ! G || O );
  if( G ) CHECK( long( G->size() ) >= *O );
  }
 }

// knapsack() and zaino(): inputs not allowed, and the two equal on random
// instances, the objects of knapsack() feasible
ADS_TEST( "knapsack" )
{
 CHECK_THROWS( knapsack( { 1 , 2 } , { 1 } , 5 ) , contract_violation );
 CHECK_THROWS( knapsack( { 1 } , { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( zaino( { 1 } , { -1 } , 3 ) , contract_violation );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t n = g() % 8;
  Longs p( n ) , v( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   p[ i ] = long( g() % 20 );
   v[ i ] = long( g() % 10 );
   }
  const long C = long( g() % 25 );
  CHECK( knapsack( p , v , C ).first == zaino( p , v , C ) );
  }
 }

// fractionalKnapsack(): a volume 0 not allowed, and fractions that fill at
// most the capacity with a profit not below the 0-1 knapsack
ADS_TEST( "fractional-knapsack" )
{
 CHECK_THROWS( fractionalKnapsack( { 1 } , { 0 } , 1 ) , contract_violation );
 CHECK_THROWS( fractionalKnapsack( { 1 } , { 1 } , -1 ) ,
               contract_violation );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t n = 1 + g() % 7;
  Longs p( n ) , v( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   p[ i ] = long( g() % 20 );
   v[ i ] = 1 + long( g() % 10 );
   }
  const long C = long( g() % 25 );
  const std::vector< double > P( p.begin() , p.end() ) ,
                              V( v.begin() , v.end() );
  const auto x = fractionalKnapsack( P , V , double( C ) );
  CHECK( std::inner_product( x.begin() , x.end() , P.begin() , 0.0 ) >=
         double( knapsack( p , v , C ).first ) - 1e-9 );
  }
 }

// independentSet(), intervalColoring() and towers(): an interval with
// a >= b and d < 0 not allowed, and random instances against a brute force
ADS_TEST( "intervals" )
{
 CHECK_THROWS( independentSet( { { 3 , 3 } } ) , contract_violation );
 CHECK_THROWS( intervalColoring( { { 4 , 2 } } ) , contract_violation );
 CHECK_THROWS( towers( { 1.0 } , -1 ) , contract_violation );
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const auto I = random_intervals( g , 9 );
  std::size_t best = 0;                    // the largest disjoint subset
  for( unsigned S = 0 ; S < ( 1u << I.size() ) ; ++S ) {
   bool ok = true;
   for( std::size_t x = 0 ; x < I.size() && ok ; ++x )
    for( std::size_t y = x + 1 ; y < I.size() && ok ; ++y )
     if( ( S >> x & 1 ) && ( S >> y & 1 ) )
      ok = !( I[ x ].first < I[ y ].second && I[ y ].first < I[ x ].second );
   if( ok ) best = std::max( best , std::size_t( std::popcount( S ) ) );
   }
  CHECK( independentSet( I ).size() == best );
  intervalColoring( I );
  std::vector< double > k( g() % 10 );
  for( auto & x : k ) x = double( g() % 50 );
  towers( k , double( g() % 5 ) );
  }
 }

// shortestJobFirst(), totalCompletion() and maxinterval(): inputs not
// allowed, and random instances against all the orders and subsets
ADS_TEST( "sjf-weighted-intervals" )
{
 CHECK_THROWS( shortestJobFirst( Longs( { 2 , -1 } ) ) , contract_violation );
 CHECK_THROWS( totalCompletion( Longs( { 2 , 1 } ) , { 0 , 2 } ) ,
               contract_violation );
 using WI = WeightedInterval< long >;
 CHECK_THROWS( maxinterval( std::vector< WI >( { { 3 , 2 , 1 } } ) ) ,
               contract_violation );
 CHECK_THROWS( maxinterval( std::vector< WI >( { { 1 , 2 , -1 } } ) ) ,
               contract_violation );
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs tm( g() % 7 );
  for( auto & x : tm ) x = long( g() % 10 );
  std::vector< std::size_t > A( tm.size() );
  std::iota( A.begin() , A.end() , 0 );
  long best = totalCompletion( tm , A );
  do best = std::min( best , totalCompletion( tm , A ) );
  while( std::next_permutation( A.begin() , A.end() ) );
  CHECK( totalCompletion( tm , shortestJobFirst( tm ) ) == best );
  std::vector< WI > I( g() % 8 );
  for( auto & x : I ) {
   x.a = long( g() % 20 );
   x.b = x.a + 1 + long( g() % 6 );
   x.w = long( g() % 10 );
   }
  maxinterval( I );
  }
 }

// the inputs not allowed of the other headers, just inside and just
// outside each bound: ragged grids (also with an empty first row, a
// regression), houses with k = 1 and 0 colours, a cover of an empty or
// reversed range, a rod with no price, volumes and coins 0, a weight or an
// amount -1, k = 0 and k = n + 1 parts, a key probability negative, a
// divisible subset with 0 and with a repeated item, jobs of time and
// weight 0, a place out of the jobs; the empty inputs and the budgets 0
ADS_TEST( "other-optimization" )
{
 using G = std::vector< Longs >;
 CHECK_THROWS( (void) largestSquare( G{ { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) countLatticePaths( G{ { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) countLatticePaths( G{ {} , { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( (void) largestSquare( G{ {} , { 1 } } ) , contract_violation );
 CHECK( largestSquare( G{} ).side == 0 && largestSquare( G{ {} } ).side == 0 &&
        largestSquare( G{ { 0 , 0 } } ).side == 0 &&
        largestSquare( G{ { 5 } } ).side == 1 &&
        largestSquare( G( 3 , Longs( 3 , 1 ) ) ).side == 3 );
 CHECK( countLatticePaths( G{} ) == 0 && countLatticePaths( G{ {} } ) == 0 &&
        countLatticePaths( G{ { 1 } } ) == 1 &&
        countLatticePaths( G{ { 0 } } ) == 0 &&
        countLatticePaths( G( 2 , Longs( 2 , 1 ) ) ) == 2 &&
        countLatticePaths( G{ { 1 , 1 } , { 1 , 0 } } ) == 0 );

 CHECK( paintHouses( G{} ).first == 0 &&
        paintHouses( G{ { 4 } } ).first == 4 );
 CHECK_THROWS( (void) paintHouses( G{ { 4 } , { 5 } } ) , contract_violation );
 CHECK_THROWS( (void) paintHouses( G{ {} } ) , contract_violation );
 CHECK_THROWS( (void) paintHouses( G{ { 1 , 2 } , { 3 } } ) ,
               contract_violation );
 CHECK( paintHouses( G{ { 1 , 9 } , { 1 , 9 } } ).first == 10 );
 CHECK( maxNonAdjacentSum( Longs{} ) == 0 &&
        maxNonAdjacentSum( Longs{ -3 , -1 } ) == 0 &&
        nonAdjacentSelection( Longs{ 4 } ).first == 4 );

 const Intervals I05 = { { 0 , 5 } } , I50 = { { 5 , 0 } };
 CHECK_THROWS( (void) minimumIntervalCover( I05 , 3L , 3L ) ,
               contract_violation );
 CHECK_THROWS( (void) minimumIntervalCover( I05 , 4L , 3L ) ,
               contract_violation );
 CHECK_THROWS( (void) minimumIntervalCover( I50 , 0L , 3L ) ,
               contract_violation );
 CHECK( ! minimumIntervalCover( Intervals{} , 0L , 1L ) );
 CHECK( minimumIntervalCover( Intervals{ { 0 , 2 } , { 2 , 4 } } , 0L , 4L )
        ->size() == 2 );                       // touching at 2
 CHECK( ! minimumIntervalCover( Intervals{ { 0 , 1 } , { 2 , 4 } } , 0L ,
                                4L ) );
 CHECK( minimumIntervalCover( Intervals{ { 1 , 1 } , { -9 , 9 } } , 0L , 1L )
        ->size() == 1 );

 CHECK( rodCutting( Longs{} , 0 ).first == 0 );
 CHECK_THROWS( (void) rodCutting( Longs{} , 1 ) , contract_violation );
 CHECK( rodCutting( Longs{ 3 } , 4 ).first == 12 );
 CHECK_THROWS( (void) unboundedKnapsack( Longs{ 1 } , Longs{} , 3L ) ,
               contract_violation );
 CHECK_THROWS( (void) unboundedKnapsack( Longs{ 1 } , Longs{ 1 } , -1L ) ,
               contract_violation );
 CHECK_THROWS( (void) unboundedKnapsack( Longs{ 1 } , Longs{ 0 } , 3L ) ,
               contract_violation );
 CHECK( unboundedKnapsack( Longs{ 5 } , Longs{ 2 } , 0L ).first == 0 &&
        unboundedKnapsack( Longs{ 5 } , Longs{ 2 } , 5L ).first == 10 &&
        unboundedKnapsack( Longs{} , Longs{} , 5L ).first == 0 );
 CHECK_THROWS( (void) minValueExactWeight( Longs{ 1 } , Longs{} , 1L ) ,
               contract_violation );
 CHECK_THROWS( (void) minValueExactWeight( Longs{ 1 } , Longs{ 1 } , -1L ) ,
               contract_violation );
 CHECK_THROWS( (void) minValueExactWeight( Longs{ 0 } , Longs{ 1 } , 1L ) ,
               contract_violation );
 CHECK( minValueExactWeight( Longs{ 2 } , Longs{ 3 } , 0L ) == 0L &&
        ! minValueExactWeight( Longs{ 2 } , Longs{ 3 } , 3L ) &&
        minValueExactWeight( Longs{ 2 } , Longs{ 3 } , 4L ) == 6L );
 CHECK_THROWS( (void) coinChangeWays( Longs{ 1 } , -1L ) ,
               contract_violation );
 CHECK_THROWS( (void) coinChangeWays( Longs{ 0 } , 1L ) , contract_violation );
 CHECK( coinChangeWays( Longs{} , 0L ) == 1 &&
        coinChangeWays( Longs{} , 3L ) == 0 &&
        coinChangeWays( Longs{ 1 , 2 } , 4L ) == 3 );
 CHECK_THROWS( (void) subsetSumCount( Longs{ 1 } , -1L ) ,
               contract_violation );
 CHECK_THROWS( (void) subsetSumCount( Longs{ -1 } , 0L ) ,
               contract_violation );
 CHECK( subsetSumCount( Longs{} , 0L ) == 1 &&
        subsetSumCount( Longs{ 0 } , 0L ) == 2 &&
        subsetSumCount( Longs{ 2 , 2 } , 2L ) == 2 );
 CHECK_THROWS( (void) minimumSquares( -1 ) , contract_violation );
 CHECK( minimumSquares( 0 ) == 0 && minimumSquares( 1 ) == 1 &&
        minimumSquares( 12 ) == 3 && minimumSquares( 13 ) == 2 &&
        minimumSquares( 7 ) == 4 );

 const Longs A = { 4 , 1 , 3 };
 CHECK_THROWS( (void) linearPartition( A , 0 ) , contract_violation );
 CHECK_THROWS( (void) linearPartition( A , 4 ) , contract_violation );
 CHECK_THROWS( (void) linearPartition( Longs{} , 1 ) , contract_violation );
 CHECK( linearPartition( A , 1 ).first == 8 &&
        linearPartition( A , 3 ).first == 4 );
 CHECK_THROWS( (void) painterPartition( A , 0 ) , contract_violation );
 CHECK_THROWS( (void) painterPartition( Longs{ 1 , -1 } , 2 ) ,
               contract_violation );
 CHECK( painterPartition( A , 1 ) == 8 && painterPartition( A , 9 ) == 4 &&
        painterPartition( Longs{} , 2 ) == 0 );

 using D = std::vector< double >;
 CHECK_THROWS( (void) optimalSearchTree( D{ 1 } , D{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) optimalSearchTree( D{ -1 } , D{ 0 , 0 } ) ,
               contract_violation );
 CHECK_THROWS( (void) optimalSearchTree( D{ 1 } , D{ 0 , -1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) optimalSearchTree( D{ 1 , -2 } ) , contract_violation );
 CHECK( optimalSearchTree( D{} ).root == -1 &&
        optimalSearchTree( D{ 3 } ).root == 0 );

 CHECK_THROWS( (void) largestDivisibleSubset( Longs{ 0 , 2 } ) ,
               contract_violation );
 CHECK_THROWS( (void) largestDivisibleSubset( Longs{ -2 , 4 } ) ,
               contract_violation );
 CHECK_THROWS( (void) largestDivisibleSubset( Longs{ 2 , 2 } ) ,
               contract_violation );
 CHECK( largestDivisibleSubset( Longs{} ).empty() &&
        largestDivisibleSubset( Longs{ 1 } ) == Longs( { 1 } ) &&
        largestDivisibleSubset( Longs{ 3 , 1 , 9 } ).size() == 3 );
 CHECK( trappingRainWater( Longs{} ) == 0 &&
        trappingRainWater( Longs{ 2 , 0 , 2 } ) == 2 );

 CHECK_THROWS( (void) smithOrder( Longs{ 1 } , Longs{} ) ,
               contract_violation );
 CHECK_THROWS( (void) smithOrder( Longs{ -1 } , Longs{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) smithOrder( Longs{ 1 } , Longs{ -1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) smithOrder( Longs{ 0 } , Longs{ 0 } ) ,
               contract_violation );
 CHECK( smithOrder( Longs{ 0 , 2 } , Longs{ 1 , 0 } ).size() == 2 );
 CHECK_THROWS( (void) weightedCompletion( Longs{ 1 } , Longs{} ,
                                          { 0 } ) , contract_violation );
 CHECK_THROWS( (void) weightedCompletion( Longs{ 1 } , Longs{ 1 } ,
                                          { 1 } ) , contract_violation );
 CHECK( weightedCompletion( Longs{ 2 } , Longs{ 3 } , { 0 } ) == 6 &&
        totalCompletion( Longs{ 2 } , {} ) == 0 &&
        shortestJobFirst( Longs{} ).empty() );
 CHECK_THROWS( (void) totalCompletion( Longs{ 2 } , { 1 } ) ,
               contract_violation );
 CHECK_THROWS( (void) fractionalKnapsack( { -1 } , { 1 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( (void) fractionalKnapsack( { 1 } , { 1 , 2 } , 1 ) ,
               contract_violation );
 CHECK( fractionalKnapsack( { 1 } , { 1 } , 0 )[ 0 ] == 0 &&
        fractionalKnapsack( {} , {} , 5 ).empty() );
 CHECK_THROWS( (void) knapsack( { -1 } , { 1 } , 1 ) , contract_violation );
 CHECK_THROWS( (void) zaino( { 1 } , { 1 , 1 } , 1 ) , contract_violation );
 CHECK( knapsack( {} , {} , 0 ).first == 0 &&
        zaino( { 5 } , { 1 } , 0 ) == 0 &&
        knapsack( { 5 } , { 1 } , 1 ).first == 5 );
 CHECK( greedyChange( { 1 , 5 } , 0 )->empty() &&
        optimalChange( { 1 , 5 } , 0 ) == 0L &&
        ! greedyChange( {} , 3 ) && ! optimalChange( {} , 3 ) &&
        ! optimalChange( { 2 } , 3 ) );
 CHECK_THROWS( (void) optimalChange( { 1 } , -1 ) , contract_violation );
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
/*------------------- End File contract_optimization.cpp -------------------*/
/*--------------------------------------------------------------------------*/
