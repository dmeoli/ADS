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
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"
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
