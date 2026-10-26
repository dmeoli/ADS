/*--------------------------------------------------------------------------*/
/*----------------------- File lib_optimization.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems of dynamic programming and of optimization of the
 * library that come from the classics of the exercises: the knapsack with
 * repetitions and the counting knapsacks, the linear partition, the
 * trading of a stock, the rows of houses, the optimal binary search tree,
 * the game of the coins, the matrices of 0 and 1, the problems on
 * sequences, the cover by intervals, Smith's rule, the recurrences of Akra
 * and Bazzi and the numbers of Catalan. Each one against a brute force on
 * random small instances with a fixed seed, at the level 3 of the
 * contracts, on the examples of the literature and on the edge cases.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 13 and 14,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/Catalan.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/CoinGame.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/IntervalCover.h"
#include "ADS/optimization/KnapsackVariants.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/OptimalSearchTree.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/optimization/StockTrading.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
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

using I64 = std::int64_t;
using U64 = std::uint64_t;
using Longs = std::vector< I64 >;
using Grid = std::vector< std::vector< int > >;

constexpr I64 NONE = std::numeric_limits< I64 >::min();

/// a random integer in [ lo , hi ]
I64 rnd( std::mt19937 & g , I64 lo , I64 hi )
{
 return( lo + I64( g() % std::uint32_t( hi - lo + 1 ) ) );
 }

/// n random integers in [ lo , hi ]
Longs rndVector( std::mt19937 & g , std::size_t n , I64 lo , I64 hi )
{
 Longs v( n );
 for( auto & x : v )
  x = rnd( g , lo , hi );
 return( v );
 }

/// a random r x c matrix of 0 and 1, a 1 with probability ones / 10
Grid rndGrid( std::mt19937 & g , std::size_t r , std::size_t c , int ones )
{
 Grid A( r , std::vector< int >( c ) );
 for( auto & row : A )
  for( auto & x : row )
   x = int( g() % 10 ) < ones ? 1 : 0;
 return( A );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the best gain of a rod of length l, by all the first pieces
I64 rodBrute( const Longs & price , std::size_t l )
{
 if( l == 0 )
  return( 0 );
 I64 best = NONE;
 for( std::size_t t = 1 ; t <= std::min( l , price.size() ) ; ++t )
  best = std::max( best , price[ t - 1 ] + rodBrute( price , l - t ) );
 return( best );
 }

// the prices of Cormen et al. (10 for 4, 22 for 8), against all the cuts
// on random prices, also negative and shorter than the rod
ADS_TEST( "rod-cutting" )
{
 const Longs P = { 1 , 5 , 8 , 9 , 10 , 17 , 17 , 20 };
 CHECK( rodCutting( P , 4 ).first == 10 );
 CHECK( rodCutting( P , 8 ).first == 22 );
 CHECK( rodCutting( P , 0 ).second.empty() );
 CHECK( rodCutting( Longs() , 0 ).first == 0 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs price = rndVector( g , std::size_t( rnd( g , 1 , 8 ) ) ,
                                 -3 , 20 );
  const auto L = std::size_t( rnd( g , 0 , 12 ) );
  const auto [ gain , pieces ] = rodCutting( price , L );
  CHECK( gain == rodBrute( price , L ) );
  CHECK( std::accumulate( pieces.begin() , pieces.end() ,
                          std::size_t( 0 ) ) == L );
  }
 CHECK( rodCutting( Longs( 1 , 3 ) , 1000 ).first == 3000 );
 CHECK_THROWS( rodCutting( Longs() , 1 ) , contract_violation );
 }

/// the largest profit within capacity c, by the first object taken or none
I64 unboundedBrute( const Longs & p , const Longs & v , I64 c )
{
 I64 best = 0;
 for( std::size_t i = 0 ; i < p.size() ; ++i )
  if( v[ i ] <= c )
   best = std::max( best , p[ i ] + unboundedBrute( p , v , c - v[ i ] ) );
 return( best );
 }

/// the least value of weight exactly c, by the first coin taken
std::optional< I64 > exactBrute( const Longs & w , const Longs & v , I64 c )
{
 if( c == 0 )
  return( I64( 0 ) );
 std::optional< I64 > best;
 for( std::size_t i = 0 ; i < w.size() ; ++i )
  if( w[ i ] <= c )
   if( const auto r = exactBrute( w , v , c - w[ i ] ) )
    if( ( ! best ) || ( *r + v[ i ] < *best ) )
     best = *r + v[ i ];
 return( best );
 }

// the knapsack with repetitions and the coins of exact weight against
// their recursions, and with values 1 against optimalChange()
ADS_TEST( "unbounded-knapsack" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto n = std::size_t( rnd( g , 0 , 4 ) );
  const Longs p = rndVector( g , n , 0 , 20 ) , v = rndVector( g , n , 1 , 6 );
  const I64 C = rnd( g , 0 , 14 );
  const auto [ best , copies ] = unboundedKnapsack( p , v , C );
  CHECK( best == unboundedBrute( p , v , C ) );
  CHECK( copies.size() == n );
  const Longs w = rndVector( g , n , 1 , 6 ) ,
              val = rndVector( g , n , 0 , 9 );
  CHECK( minValueExactWeight( w , val , C ) == exactBrute( w , val , C ) );
  const std::vector< long > coins( w.begin() , w.end() );
  const auto one = minValueExactWeight( w , Longs( n , 1 ) , C );
  const auto opt = optimalChange( coins , long( C ) );
  CHECK( one.has_value() == opt.has_value() );
  if( one && opt )
   CHECK( *one == *opt );
  }
 CHECK( minValueExactWeight( Longs( { 50 , 10 } ) , Longs( { 200 , 50 } ) ,
                             I64( 50 ) ) == 200 );
 CHECK( ! minValueExactWeight( Longs( { 3 } ) , Longs( { 7 } ) , I64( 5 ) ) );
 CHECK( minValueExactWeight( Longs() , Longs() , I64( 0 ) ) == 0 );
 CHECK( unboundedKnapsack( Longs( { 10 , 40 , 50 , 70 } ) ,
                           Longs( { 1 , 3 , 4 , 5 } ) , I64( 8 ) ).first
        == 110 );
 CHECK_THROWS( unboundedKnapsack( Longs( { 1 } ) , Longs( { 0 } ) ,
                                  I64( 3 ) ) ,
               contract_violation );
 CHECK_THROWS( minValueExactWeight( Longs( { 1 } ) , Longs( { 1 } ) ,
                                    I64( -1 ) ) , contract_violation );
 }

/// the multisets of coins[ i .. ] of sum t
U64 waysBrute( const Longs & coins , std::size_t i , I64 t )
{
 if( t == 0 )
  return( 1 );
 if( i == coins.size() )
  return( 0 );
 U64 c = 0;
 for( I64 s = t ; s >= 0 ; s -= coins[ i ] )
  c += waysBrute( coins , i + 1 , s );
 return( c );
 }

// the ways of the change and the subsets of a given sum against their
// enumerations; ones against the binomials, the empty sets
ADS_TEST( "coin-change-ways" )
{
 CHECK( coinChangeWays( Longs( { 1 , 2 , 5 } ) , I64( 5 ) ) == 4 );
 CHECK( coinChangeWays( Longs( { 2 } ) , I64( 3 ) ) == 0 );
 CHECK( coinChangeWays( Longs() , I64( 0 ) ) == 1 );
 CHECK( coinChangeWays( Longs( { 1 , 2 } ) , I64( 1000 ) ) == 501 );
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs coins = rndVector( g , std::size_t( rnd( g , 0 , 4 ) ) , 1 , 7 );
  const I64 T = rnd( g , 0 , 25 );
  CHECK( coinChangeWays( coins , T ) == waysBrute( coins , 0 , T ) );
  const Longs A = rndVector( g , std::size_t( rnd( g , 0 , 12 ) ) , 0 , 9 );
  const I64 k = rnd( g , 0 , 30 );
  U64 c = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m ) {
   I64 s = 0;
   for( std::size_t i = 0 ; i < A.size() ; ++i )
    if( ( m >> i ) & 1 )
     s += A[ i ];
   c += ( s == k );
   }
  CHECK( subsetSumCount( A , k ) == c );
  }
 const auto C = tartaglia( 60 );
 CHECK( subsetSumCount( Longs( 60 , 1 ) , I64( 30 ) ) == C[ 60 ][ 30 ] );
 CHECK( subsetSumCount( Longs( { 0 , 0 } ) , I64( 0 ) ) == 4 );
 CHECK( subsetSumCount( Longs() , I64( 3 ) ) == 0 );
 CHECK_THROWS( coinChangeWays( Longs( { 0 } ) , I64( 1 ) ) ,
               contract_violation );
 CHECK_THROWS( subsetSumCount( Longs( { -1 } ) , I64( 1 ) ) ,
               contract_violation );
 }

// the fewest squares against the dynamic programming up to 3000, 18 = 9 +
// 9, the numbers 4^a ( 8 b + 7 ), large squares and sums of two squares
ADS_TEST( "minimum-squares" )
{
 const int N = 3000;
 std::vector< int > S( N + 1 , 0 );
 for( int i = 1 ; i <= N ; ++i ) {
  S[ i ] = i;
  for( int t = 1 ; t * t <= i ; ++t )
   S[ i ] = std::min( S[ i ] , S[ i - t * t ] + 1 );
  }
 for( int i = 0 ; i <= N ; ++i )
  CHECK( minimumSquares( i ) == S[ i ] );
 CHECK( minimumSquares( 18 ) == 2 && minimumSquares( 7 ) == 4 );
 CHECK( minimumSquares( I64( 1000000007 ) * 7 ) <= 4 );
 CHECK( minimumSquares( I64( 3037000499 ) * 3037000499 ) == 1 );
 CHECK( minimumSquares( I64( 1 ) << 62 ) == 1 );
 CHECK( minimumSquares( ( I64( 1 ) << 60 ) * 7 / 4 ) == 4 );
 CHECK( minimumSquares( I64( 1000000 ) * 1000000 + 1 ) == 2 );
 CHECK_THROWS( minimumSquares( -1 ) , contract_violation );
 }

/// the cost of the cuts starts of A
I64 partitionCost( const Longs & A , const std::vector< std::size_t > & c )
{
 I64 worst = NONE , s = 0;
 std::size_t next = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  if( ( next < c.size() ) && ( c[ next ] == i ) ) {
   worst = std::max( worst , s );
   s = 0;
   ++next;
   }
  s += A[ i ];
  }
 return( std::max( worst , s ) );
 }

/// the least cost over all the sets of k - 1 cuts of A
I64 partitionBrute( const Longs & A , std::size_t k )
{
 I64 best = std::numeric_limits< I64 >::max();
 const std::size_t n = A.size();
 for( unsigned m = 0 ; m < ( 1u << ( n - 1 ) ) ; ++m )
  if( std::size_t( std::popcount( m ) ) == k - 1 ) {
   std::vector< std::size_t > c;
   for( std::size_t i = 0 ; i + 1 < n ; ++i )
    if( ( m >> i ) & 1 )
     c.push_back( i + 1 );
   best = std::min( best , partitionCost( A , c ) );
   }
 return( best );
 }

// the linear partition against all the cuts, also with negative items, and
// the painter's partition against it on items not negative, more painters
// than boards, no board
ADS_TEST( "linear-partition" )
{
 const Longs V = { 2 , 3 , 7 , -7 , 15 , 2 };
 CHECK( linearPartition( V , 3 ).first == 10 );
 CHECK( linearPartition( V , 1 ).first == 22 );
 CHECK( linearPartition( V , 6 ).first == 15 );
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs A = rndVector( g , std::size_t( rnd( g , 1 , 11 ) ) , -9 , 20 );
  const auto k = std::size_t( rnd( g , 1 , I64( A.size() ) ) );
  const auto [ cost , starts ] = linearPartition( A , k );
  CHECK( cost == partitionBrute( A , k ) );
  CHECK( ( starts.size() == k - 1 ) &&
         ( partitionCost( A , starts ) == cost ) );
  const Longs B = rndVector( g , std::size_t( rnd( g , 1 , 11 ) ) , 0 , 20 );
  const auto h = std::size_t( rnd( g , 1 , 14 ) );
  CHECK( painterPartition( B , h ) ==
         linearPartition( B , std::min( h , B.size() ) ).first );
  }
 CHECK( painterPartition( Longs() , 3 ) == 0 );
 CHECK( painterPartition( Longs( 6 , 4 ) , 10 ) == 4 );
 CHECK( painterPartition( Longs( { 0 , 0 , 0 } ) , 2 ) == 0 );
 CHECK_THROWS( linearPartition( V , 7 ) , contract_violation );
 CHECK_THROWS( linearPartition( V , 0 ) , contract_violation );
 CHECK_THROWS( painterPartition( Longs( { -1 } ) , 1 ) , contract_violation );
 }

/// the best gain of exactly k transactions over the 2 k days of each mask
std::optional< I64 > tradingBrute( const Longs & P , std::size_t k )
{
 std::optional< I64 > best;
 for( unsigned m = 0 ; m < ( 1u << P.size() ) ; ++m ) {
  if( std::size_t( std::popcount( m ) ) != 2 * k )
   continue;
  std::vector< std::size_t > d;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   if( ( m >> i ) & 1 )
    d.push_back( i );
  I64 s = 0;
  for( std::size_t t = 0 ; t < k ; ++t )
   s += P[ d[ 2 * t + 1 ] ] - P[ d[ 2 * t ] ];
  if( ( ! best ) || ( *best < s ) )
   best = s;
  }
 return( best );
 }

// at most and exactly k transactions against all the choices of days; the
// example of the exercise (12), falling prices, too few days, k = 0, one
// day, all prices equal, k far above the days
ADS_TEST( "stock-trading" )
{
 CHECK( maxProfitExactly( Longs( { 1 , 3 , 6 , 7 , 4 , 10 } ) , 2 ) == 12 );
 CHECK( maxProfit( Longs( { 3 , 2 , 6 , 5 , 0 , 3 } ) , 2 ) == 7 );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs P = rndVector( g , std::size_t( rnd( g , 0 , 12 ) ) , 0 , 30 );
  const auto k = std::size_t( rnd( g , 0 , 7 ) );
  CHECK( maxProfitExactly( P , k ) == tradingBrute( P , k ) );
  I64 best = 0;
  for( std::size_t l = 0 ; l <= k ; ++l )
   if( const auto r = tradingBrute( P , l ) )
    best = std::max( best , *r );
  CHECK( maxProfit( P , k ) == best );
  }
 CHECK( maxProfitExactly( Longs( { 5 , 4 , 3 , 2 } ) , 2 ) == -2 );
 CHECK( maxProfit( Longs( { 5 , 4 , 3 , 2 } ) , 2 ) == 0 );
 CHECK( ! maxProfitExactly( Longs( { 1 , 2 , 3 } ) , 2 ) );
 CHECK( maxProfitExactly( Longs() , 0 ) == 0 );
 CHECK( maxProfit( Longs() , 3 ) == 0 );
 // k = 0, one day, all prices equal, k far above the days
 CHECK( maxProfit( Longs( { 1 , 9 } ) , 0 ) == 0 &&
        maxProfitExactly( Longs( { 1 , 9 } ) , 0 ) == 0 );
 CHECK( maxProfit( Longs( { 4 } ) , 1 ) == 0 &&
        ! maxProfitExactly( Longs( { 4 } ) , 1 ) );
 CHECK( maxProfit( Longs( 6 , 3 ) , 2 ) == 0 &&
        maxProfitExactly( Longs( 6 , 3 ) , 3 ) == 0 );
 CHECK( maxProfit( Longs( { 1 , 5 , 2 , 8 } ) , 100 ) == 10 );
 }

// the largest sum of items no two adjacent against all the masks, also
// with negative items; the houses read back; the cheapest painting against
// all the colorings, the colors read back
ADS_TEST( "house-problems" )
{
 CHECK( maxNonAdjacentSum( Longs( { 5 , 1 , 1 , 5 } ) ) == 10 );
 CHECK( nonAdjacentSelection( Longs( { 5 , 1 , 1 , 5 } ) ).second ==
        std::vector< std::size_t >( { 0 , 3 } ) );
 CHECK( maxNonAdjacentSum( Longs() ) == 0 );
 CHECK( maxNonAdjacentSum( Longs( { -3 , -1 } ) ) == 0 );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs A = rndVector( g , std::size_t( rnd( g , 0 , 14 ) ) , -5 , 20 );
  I64 best = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m )
   if( ( m & ( m >> 1 ) ) == 0 ) {
    I64 s = 0;
    for( std::size_t i = 0 ; i < A.size() ; ++i )
     if( ( m >> i ) & 1 )
      s += A[ i ];
    best = std::max( best , s );
    }
  CHECK( maxNonAdjacentSum( A ) == best );
  const auto [ s , P ] = nonAdjacentSelection( A );
  I64 sp = 0;
  for( std::size_t i = 0 ; i < P.size() ; ++i ) {
   CHECK( ( i == 0 ) || ( P[ i - 1 ] + 1 < P[ i ] ) );
   sp += A[ P[ i ] ];
   }
  CHECK( ( s == best ) && ( sp == best ) );
  const auto n = std::size_t( rnd( g , 0 , 6 ) ) ,
             k = std::size_t( rnd( g , 2 , 4 ) );
  std::vector< Longs > cost( n );
  for( auto & r : cost )
   r = rndVector( g , k , 0 , 20 );
  I64 cheapest = n ? std::numeric_limits< I64 >::max() : 0;
  std::size_t total = 1;
  for( std::size_t i = 0 ; i < n ; ++i )
   total *= k;
  for( std::size_t code = 0 ; ( n > 0 ) && ( code < total ) ; ++code ) {
   std::size_t x = code , prev = k;
   I64 c = 0;
   bool ok = true;
   for( std::size_t h = 0 ; h < n ; ++h , x /= k ) {
    ok = ok && ( x % k != prev );
    prev = x % k;
    c += cost[ h ][ prev ];
    }
   if( ok )
    cheapest = std::min( cheapest , c );
   }
  const auto [ c , colors ] = paintHouses( cost );
  CHECK( ( c == cheapest ) && ( colors.size() == n ) );
  }
 CHECK( paintHouses( std::vector< Longs >( { { 4 } } ) ).first == 4 );
 CHECK( paintHouses( std::vector< Longs >( 3 , Longs( { 1 , 9 } ) ) ).first
        == 11 );
 CHECK_THROWS( paintHouses( std::vector< Longs >( 2 , Longs( { 1 } ) ) ) ,
               contract_violation );
 CHECK_THROWS( paintHouses( std::vector< Longs >( { { 1 , 2 } , { 1 } } ) ) ,
               contract_violation );
 }

/// the least weighted path length of the keys i .. j - 1 at depth d
I64 bstBrute( const Longs & p , const Longs & q , int i , int j , I64 d )
{
 if( i >= j )
  return( q[ i ] * d );
 I64 best = std::numeric_limits< I64 >::max();
 for( int r = i ; r < j ; ++r )
  best = std::min( best , p[ r ] * ( d + 1 ) +
                          bstBrute( p , q , i , r , d + 1 ) +
                          bstBrute( p , q , r + 1 , j , d + 1 ) );
 return( best );
 }

// Knuth's algorithm against all the trees, with and without the weights
// of the gaps, many of them 0; the example of Cormen et al. (2.75 counting
// the external nodes one level deeper, 2.35 here, the roots 1 and 3 tied
// and the largest kept)
ADS_TEST( "optimal-search-tree" )
{
 const auto E = optimalSearchTree( Longs() );
 CHECK( ( E.cost == 0 ) && ( E.root == -1 ) );
 const auto C = optimalSearchTree( Longs( { 15 , 10 , 5 , 10 , 20 } ) ,
                                   Longs( { 5 , 10 , 5 , 5 , 5 , 10 } ) );
 CHECK( ( C.cost == 235 ) && ( C.root == 3 ) );     // 1 and 3 tie
 CHECK( optimalSearchTree( Longs( { 1 , 1 , 1 } ) ).cost == 5 );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto n = std::size_t( rnd( g , 0 , 8 ) );
  const Longs p = rndVector( g , n , 0 , 6 ) ,
              q = rndVector( g , n + 1 , 0 , 4 );
  CHECK( optimalSearchTree( p , q ).cost == bstBrute( p , q , 0 , int( n ) ,
                                                      0 ) );
  CHECK( optimalSearchTree( p ).cost ==
         bstBrute( p , Longs( n + 1 , 0 ) , 0 , int( n ) , 0 ) );
  }
 CHECK( optimalSearchTree( std::vector< double >( { 0.6 , 0.2 , 0.2 } ) )
        .root == 0 );
 CHECK_THROWS( optimalSearchTree( Longs( { -1 } ) ) , contract_violation );
 CHECK_THROWS( optimalSearchTree( Longs( { 1 } ) , Longs( { 1 } ) ) ,
               contract_violation );
 }

/// the most the player to move gets from V[ i .. j - 1 ]
I64 coinsBrute( const Longs & V , std::size_t i , std::size_t j )
{
 if( i >= j )
  return( 0 );
 const I64 sum = std::accumulate( V.begin() + I64( i ) , V.begin() + I64( j ) ,
                                  I64( 0 ) );
 return( std::max( sum - coinsBrute( V , i + 1 , j ) ,
                   sum - coinsBrute( V , i , j - 1 ) ) );
 }

// the game of the coins against the minimax on all the plays, odd and
// even lengths, negative values
ADS_TEST( "coins-in-a-line" )
{
 CHECK( coinsInALine( Longs( { 5 , 3 , 7 , 10 } ) ) == 15 );
 CHECK( coinsInALine( Longs() ) == 0 && coinsInALine( Longs( { 4 } ) ) == 4 );
 CHECK( coinsInALine( Longs( { 2 , 2 , 2 , 2 } ) ) == 4 );
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs V = rndVector( g , std::size_t( rnd( g , 0 , 12 ) ) , -5 , 20 );
  CHECK( coinsInALine( V ) == coinsBrute( V , 0 , V.size() ) );
  }
 CHECK( coinsInALine( std::vector< double >( { 1.5 , 2.5 } ) ) == 2.5 );
 }

// the largest square of ones against all the corners and sides; the paths
// against their recursion and, without obstacles, against the binomials
ADS_TEST( "grid-paths" )
{
 const Grid A = { { 1 , 0 , 1 , 0 , 1 , 0 , 0 } ,
                  { 1 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                  { 0 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                  { 0 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                  { 1 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                  { 1 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                  { 1 , 0 , 1 , 0 , 1 , 0 , 1 } };
 const auto S = largestSquare( A );
 CHECK( ( S.side == 4 ) && ( S.row == 1 ) && ( S.col == 2 ) );
 CHECK( largestSquare( Grid() ).side == 0 );
 CHECK( largestSquare( Grid( 30 , std::vector< int >( 40 , 1 ) ) ).side
        == 30 );
 std::mt19937 g( 9 );
 std::function< U64( const Grid & , std::size_t , std::size_t ) > paths =
  [ & ]( const Grid & B , std::size_t i , std::size_t j ) -> U64 {
   if( ( i >= B.size() ) || ( j >= B[ 0 ].size() ) || ( B[ i ][ j ] == 0 ) )
    return( 0 );
   if( ( i + 1 == B.size() ) && ( j + 1 == B[ 0 ].size() ) )
    return( 1 );
   return( paths( B , i + 1 , j ) + paths( B , i , j + 1 ) );
   };
 for( int t = 0 ; t < 400 ; ++t ) {
  const auto r = std::size_t( rnd( g , 1 , 7 ) ) ,
             c = std::size_t( rnd( g , 1 , 7 ) );
  const Grid B = rndGrid( g , r , c , int( rnd( g , 5 , 10 ) ) );
  std::size_t side = 0;
  for( std::size_t i = 0 ; i < r ; ++i )
   for( std::size_t j = 0 ; j < c ; ++j )
    for( std::size_t s = 1 ; ( i + s <= r ) && ( j + s <= c ) ; ++s ) {
     bool ok = true;
     for( std::size_t a = 0 ; a < s ; ++a )
      for( std::size_t b = 0 ; b < s ; ++b )
       ok = ok && B[ i + a ][ j + b ];
     if( ok )
      side = std::max( side , s );
     }
  CHECK( largestSquare( B ).side == side );
  CHECK( countLatticePaths( B ) == paths( B , 0 , 0 ) );
  }
 const auto T = tartaglia( 60 );
 CHECK( countLatticePaths( Grid( 31 , std::vector< int >( 31 , 1 ) ) ) ==
        T[ 60 ][ 30 ] );
 CHECK( countLatticePaths( Grid( { { 0 } } ) ) == 0 );
 CHECK( countLatticePaths( Grid() ) == 0 );
 CHECK_THROWS( largestSquare( Grid( { { 1 , 1 } , { 1 } } ) ) ,
               contract_violation );
 CHECK_THROWS( countLatticePaths( Grid( { { 1 } , { 1 , 1 } } ) ) ,
               contract_violation );
 }

// the water against the quadratic formula of the maxima on either side,
// the classical example (6), fewer than three columns, all equal
ADS_TEST( "trapping-rain-water" )
{
 CHECK( trappingRainWater( Longs( { 0 , 1 , 0 , 2 , 1 , 0 , 1 , 3 , 2 , 1 ,
                                    2 , 1 } ) ) == 6 );
 CHECK( trappingRainWater( Longs() ) == 0 &&
        trappingRainWater( Longs( { 3 , 0 } ) ) == 0 );
 CHECK( trappingRainWater( Longs( 50 , 7 ) ) == 0 );
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Longs h = rndVector( g , std::size_t( rnd( g , 0 , 20 ) ) , 0 , 9 );
  I64 w = 0;
  for( std::size_t i = 0 ; i < h.size() ; ++i ) {
   const I64 l = *std::max_element( h.begin() , h.begin() + I64( i ) + 1 ) ,
             r = *std::max_element( h.begin() + I64( i ) , h.end() );
   w += std::min( l , r ) - h[ i ];
   }
  CHECK( trappingRainWater( h ) == w );
  }
 }

// the divisible subsets, the heaviest increasing subsequences and the
// chains of boxes against all the subsets; the distinct segments against
// every segment
ADS_TEST( "sequence-problems" )
{
 CHECK( largestDivisibleSubset( std::vector< int >( { 1 , 2 , 3 , 4 , 6 } ) )
        .size() == 3 );
 CHECK( largestDivisibleSubset( std::vector< int >() ).empty() );
 CHECK( maxSumIncreasingSubsequence( Longs( { 1 , 101 , 2 , 3 , 100 , 4 ,
                                              5 } ) ) == 106 );
 CHECK( longestDistinctSegment( Longs( { 1 , 2 , 3 , 1 , 2 , 3 , 2 , 2 } ) )
        == std::pair( std::size_t( 0 ) , std::size_t( 3 ) ) );
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::set< int > pool;
  const auto n = std::size_t( rnd( g , 0 , 11 ) );
  while( pool.size() < n )
   pool.insert( int( rnd( g , 1 , 60 ) ) );
  const std::vector< int > A( pool.begin() , pool.end() );
  std::size_t best = 0;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   bool ok = true;
   for( std::size_t i = 0 ; i < n ; ++i )
    for( std::size_t j = 0 ; j < n ; ++j )
     if( ( ( m >> i ) & ( m >> j ) & 1 ) && ( A[ j ] % A[ i ] ) &&
         ( A[ i ] % A[ j ] ) )
      ok = false;
   if( ok )
    best = std::max( best , std::size_t( std::popcount( m ) ) );
   }
  CHECK( largestDivisibleSubset( A ).size() == best );
  const Longs V = rndVector( g , std::size_t( rnd( g , 0 , 12 ) ) , -6 , 15 );
  I64 heaviest = 0;
  for( unsigned m = 0 ; m < ( 1u << V.size() ) ; ++m ) {
   I64 s = 0 , last = NONE;
   bool ok = true;
   for( std::size_t i = 0 ; i < V.size() ; ++i )
    if( ( m >> i ) & 1 ) {
     ok = ok && ( last < V[ i ] );
     last = V[ i ];
     s += V[ i ];
     }
   if( ok )
    heaviest = std::max( heaviest , s );
   }
  CHECK( maxSumIncreasingSubsequence( V ) == heaviest );
  std::vector< std::array< int , 3 > > B( std::size_t( rnd( g , 0 , 11 ) ) );
  for( auto & b : B )
   b = { int( rnd( g , 1 , 6 ) ) , int( rnd( g , 1 , 6 ) ) ,
         int( rnd( g , 1 , 6 ) ) };
  auto fits = []( const std::array< int , 3 > & a ,
                  const std::array< int , 3 > & b ) {
   return( ( a[ 0 ] < b[ 0 ] ) && ( a[ 1 ] < b[ 1 ] ) && ( a[ 2 ] < b[ 2 ] ) );
   };
  std::size_t longest = 0;
  for( unsigned m = 0 ; m < ( 1u << B.size() ) ; ++m ) {
   std::size_t prev = B.size();
   bool ok = true;
   for( std::size_t i = 0 ; i < B.size() ; ++i )
    if( ( m >> i ) & 1 ) {
     ok = ok && ( ( prev == B.size() ) || fits( B[ prev ] , B[ i ] ) );
     prev = i;
     }
   if( ok )
    longest = std::max( longest , std::size_t( std::popcount( m ) ) );
   }
  CHECK( longestChainSubsequence( B , fits ).size() == longest );
  const Longs D = rndVector( g , std::size_t( rnd( g , 0 , 16 ) ) , 1 , 5 );
  std::size_t widest = 0;
  for( std::size_t i = 0 ; i < D.size() ; ++i )
   for( std::size_t j = i ; j < D.size() ; ++j )
    if( std::set< I64 >( D.begin() + I64( i ) , D.begin() + I64( j ) + 1 )
        .size() == j - i + 1 )
     widest = std::max( widest , j - i + 1 );
  CHECK( longestDistinctSegment( D ).second == widest );
  }
 std::vector< int > P2;
 for( int i = 0 ; i < 31 ; ++i )
  P2.push_back( 1 << i );
 CHECK( largestDivisibleSubset( P2 ) == P2 );
 CHECK( longestDistinctSegment( Longs() ).second == 0 );
 CHECK( longestChainSubsequence( Longs( { 3 , 1 , 2 , 5 , 4 } ) ,
                                 std::less<>() ) ==
        std::vector< std::size_t >( { 1 , 2 , 3 } ) );
 CHECK_THROWS( largestDivisibleSubset( std::vector< int >( { 2 , 2 } ) ) ,
               contract_violation );
 CHECK_THROWS( largestDivisibleSubset( std::vector< int >( { 0 } ) ) ,
               contract_violation );
 }

// the cover by the fewest intervals against all the subsets of at most 11
// intervals; touching intervals, an uncovered start, no interval
ADS_TEST( "interval-cover" )
{
 using Is = std::vector< std::pair< I64 , I64 > >;
 CHECK( minimumIntervalCover( Is( { { 0 , 10 } , { 20 , 30 } } ) , I64( 0 ) ,
                              I64( 10 ) )->size() == 1 );
 CHECK( ! minimumIntervalCover( Is() , I64( 0 ) , I64( 5 ) ) );
 CHECK( ! minimumIntervalCover( Is( { { 1 , 7 } } ) , I64( 0 ) , I64( 5 ) ) );
 CHECK( minimumIntervalCover( Is( { { 0 , 2 } , { 2 , 4 } } ) , I64( 0 ) ,
                              I64( 4 ) )->size() == 2 );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const auto n = std::size_t( rnd( g , 0 , 11 ) );
  const I64 L = rnd( g , 1 , 12 );
  Is I( n );
  for( auto & [ a , b ] : I ) {
   const I64 d = rnd( g , -2 , L + 2 ) , r = rnd( g , 0 , 4 );
   a = d - r;
   b = d + r;
   }
  std::optional< std::size_t > best;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   Is J;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ( m >> i ) & 1 )
     J.push_back( I[ i ] );
   std::sort( J.begin() , J.end() );
   I64 reach = 0;
   for( const auto & [ a , b ] : J )
    if( a <= reach )
     reach = std::max( reach , b );
   const auto c = std::size_t( std::popcount( m ) );
   if( ( reach >= L ) && ( ( ! best ) || ( c < *best ) ) )
    best = c;
   }
  const auto got = minimumIntervalCover( I , I64( 0 ) , L );
  CHECK( got.has_value() == best.has_value() );
  if( got && best )
   CHECK( got->size() == *best );
  }
 CHECK_THROWS( minimumIntervalCover( Is() , I64( 3 ) , I64( 3 ) ) ,
               contract_violation );
 CHECK_THROWS( minimumIntervalCover( Is( { { 2 , 1 } } ) , I64( 0 ) ,
                                     I64( 3 ) ) , contract_violation );
 }

// Smith's rule against all the orders, weights 0 and times 0 included;
// with the weights 1 it is shortestJobFirst()
ADS_TEST( "smith-rule" )
{
 const std::vector< double > l = { 1 , 2 } , f = { 0.3 , 0.7 };
 CHECK( smithOrder( l , f ) == std::vector< std::size_t >( { 1 , 0 } ) );
 CHECK( smithOrder( std::vector< double >() , std::vector< double >() )
        .empty() );
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto n = std::size_t( rnd( g , 0 , 6 ) );
  Longs T = rndVector( g , n , 0 , 9 ) , W = rndVector( g , n , 0 , 9 );
  for( std::size_t i = 0 ; i < n ; ++i )
   if( T[ i ] + W[ i ] == 0 )
    T[ i ] = 1;
  std::vector< std::size_t > order( n );
  std::iota( order.begin() , order.end() , std::size_t( 0 ) );
  I64 best = std::numeric_limits< I64 >::max();
  do best = std::min( best , I64( weightedCompletion( T , W , order ) ) );
  while( std::next_permutation( order.begin() , order.end() ) );
  CHECK( weightedCompletion( T , W , smithOrder( T , W ) ) == best );
  CHECK( weightedCompletion( T , Longs( n , 1 ) , smithOrder( T ,
                                                    Longs( n , 1 ) ) ) ==
         totalCompletion( T , shortestJobFirst( T ) ) );
  }
 CHECK_THROWS( smithOrder( Longs( { 0 } ) , Longs( { 0 } ) ) ,
               contract_violation );
 CHECK_THROWS( smithOrder( Longs( { 1 } ) , Longs( { -1 } ) ) ,
               contract_violation );
 }

// the exponent against the root of the equation and against the master
// theorem for one term; the selection in linear time, 2 T( n / 3 ) +
// T( 2 n / 3 ), a negative exponent, the three cases of the order
ADS_TEST( "akra-bazzi" )
{
 auto near = []( double a , double b ) { return( std::abs( a - b ) < 1e-9 ); };
 const double p = akraBazziExponent( { { 1 , 0.2 } , { 1 , 0.7 } } );
 CHECK( near( std::pow( 0.2 , p ) + std::pow( 0.7 , p ) , 1 ) && ( p < 1 ) );
 CHECK( akraBazzi( { { 1 , 0.2 } , { 1 , 0.7 } } , 1 ) ==
        ( Growth{ 1 , 1 , 0 , 0 } ) );
 CHECK( std::abs( akraBazziExponent( { { 2 , 1.0 / 3 } , { 1 , 2.0 / 3 } } )
                  - 1.394954 ) < 1e-6 );
 CHECK( near( akraBazziExponent( { { 0.25 , 0.5 } } ) , -2 ) );
 CHECK( near( akraBazziExponent( { { 1 , 0.5 } , { 1 , 0.5 } } ) , 1 ) );
 for( double a : { 1.0 , 2.0 , 3.0 , 4.0 , 7.0 , 8.0 } )
  for( double b : { 2.0 , 3.0 , 4.0 } )
   for( double beta : { 0.0 , 0.5 , 1.0 , 2.0 , 3.0 } ) {
    const Growth x = akraBazzi( { { a , 1 / b } } , beta , 0 ) ,
                 y = master( a , b , beta , 0 );
    CHECK( ( std::abs( x.p - y.p ) < 1e-6 ) && ( x.q == y.q ) &&
           ( x.r == y.r ) );
    }
 CHECK( akraBazzi( { { 1 , 0.5 } , { 1 , 0.5 } } , 1 , -1 ) ==
        ( Growth{ 1 , 1 , 0 , 1 } ) );
 CHECK( akraBazzi( { { 1 , 0.5 } , { 1 , 0.5 } } , 1 , -2 ) ==
        ( Growth{ 1 , 1 , 0 , 0 } ) );
 CHECK( akraBazzi( { { 1 , 0.5 } } , 2 , 1 ) == ( Growth{ 1 , 2 , 1 , 0 } ) );
 CHECK_THROWS( akraBazziExponent( {} ) , contract_violation );
 CHECK_THROWS( akraBazziExponent( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( akraBazziExponent( { { 0 , 0.5 } } ) , contract_violation );
 }

// the numbers of Catalan against binom( 2 n , n ) / ( n + 1 ) up to 33,
// the largest in 64 bits, and in BigInteger for n = 200
ADS_TEST( "catalan" )
{
 const auto T = tartaglia( 67 );
 for( int n = 0 ; n <= 33 ; ++n )
  CHECK( catalan( n ) == T[ 2 * n ][ n ] / U64( n + 1 ) );
 CHECK( catalan( 3 ) == 5 && catalan( 0 ) == 1 && catalan( 1 ) == 1 );
 CHECK( catalan( 36 ) == 11959798385860453492ull );
 BigInteger c( 1 );                             // C_{n+1} from C_n
 for( long long n = 0 ; n < 200 ; ++n )
  c = c * BigInteger( 2 * ( 2 * n + 1 ) ) / BigInteger( n + 2 );
 CHECK( catalan< BigInteger >( 200 ) == c );
 CHECK_THROWS( catalan( -1 ) , contract_violation );
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
/*--------------------- End File lib_optimization.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
