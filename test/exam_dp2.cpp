/*--------------------------------------------------------------------------*/
/*-------------------------- File exam_dp2.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the solutions of the exercises of dynamic programming 37 to 72
 * of the collection of written tests of Montresor: each solution is a
 * function of the namespace dp, checked on the examples of the text, on
 * random small instances against an enumeration or an independent
 * algorithm, and on its edge cases, at the level 3 of the contracts. Where
 * the pseudocode of the text has a defect, the function implements the
 * correct version and its comment says what differs.
 *
 * \see A. Montresor, Esercizi di programmazione dinamica, raccolte da
 *      compiti scritti, https://algoritmicamente.it
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

#include "ADS/Contract.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/Catalan.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/IntervalCover.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/KnapsackVariants.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Palindromes.h"
#include "ADS/string/Parentheses.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/StringBreaking.h"
#include "ADS/string/SubsequenceProblems.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< long long >;
using Table = std::vector< std::vector< long long > >;
using UTable = std::vector< std::vector< std::uint64_t > >;

/// the mark of a subproblem not solved yet in a table of counts
constexpr std::uint64_t UNSOLVED = std::numeric_limits< std::uint64_t >::max();

/// n random integers in [ lo , hi ]
Ints random_ints( std::size_t n , long long lo , long long hi ,
                  std::mt19937 & g )
{
 std::uniform_int_distribution< long long > u( lo , hi );
 Ints v( n );
 for( auto & x : v ) x = u( g );
 return( v );
 }

/// a random string of n characters of an alphabet of k letters
std::string random_string( std::size_t n , int k , std::mt19937 & g )
{
 std::string s;
 for( std::size_t i = 0 ; i < n ; ++i )
  s += char( 'a' + g() % unsigned( k ) );
 return( s );
 }

/// the sum of the items of V chosen by the bits of mask
long long mask_sum( const Ints & V , unsigned mask )
{
 long long s = 0;
 for( std::size_t i = 0 ; i < V.size() ; ++i )
  if( mask >> i & 1 ) s += V[ i ];
 return( s );
 }

/// the items of V chosen by the bits of mask, in order
Ints mask_items( const Ints & V , unsigned mask )
{
 Ints s;
 for( std::size_t i = 0 ; i < V.size() ; ++i )
  if( mask >> i & 1 ) s.push_back( V[ i ] );
 return( s );
 }

/// the number of Catalan C( n ) = C( 2 n , n ) / ( n + 1 ), 0 <= n <= 33
std::uint64_t catalan( int n )
{
 static const auto T = tartaglia( 66 );
 return( T[ 2 * n ][ n ] / std::uint64_t( n + 1 ) );
 }

/// n! as a BigInteger
BigInteger factorial( int n )
{
 BigInteger f( 1 );
 for( int i = 2 ; i <= n ; ++i ) f *= BigInteger( i );
 return( f );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 37, 2015-12-17, K occurrence -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// kOccurrenceRec(): DP[ i ][ j ], -1 if not solved yet (every value is
/// >= 0, since skipping every item gives 0)
long long kOccurrenceRec( const Ints & V , int k , int i , int j ,
                          Table & DP )
{
 if( i <= 0 ) return( 0 );
 if( DP[ i ][ j ] < 0 ) {
  if( j == 0 )
   DP[ i ][ j ] = kOccurrenceRec( V , k , i - 1 , k , DP );
  else
   DP[ i ][ j ] = std::max( kOccurrenceRec( V , k , i - 1 , k , DP ) ,
                            kOccurrenceRec( V , k , i - 1 , j - 1 , DP ) +
                            V[ i - 1 ] );
  }
 return( DP[ i ][ j ] );
 }

/// the largest sum of a subsequence of V that takes no more than k items
/// of V in a row (0 for the empty one). DP[ i ][ j ] is the best sum of the
/// first i items when at most j more items in a row may be taken:
/// DP[ 0 ][ j ] = 0, DP[ i ][ 0 ] = DP[ i - 1 ][ k ] (a forced skip), and
/// otherwise the larger of the skip DP[ i - 1 ][ k ] and of the take
/// DP[ i - 1 ][ j - 1 ] + V_i; by memoization, O( n k ) time and space.
/// The table of the text has no column for j = 0, which the recursion uses.
/// \pre 0 <= k <= n
long long kOccurrence( const Ints & V , int k )
{
 const int n = int( V.size() );
 ADS_PRE( ( k >= 0 ) && ( k <= n ) );
 Table DP( n + 1 , Ints( k + 1 , -1 ) );
 return( kOccurrenceRec( V , k , n , k , DP ) );
 }

}  // end( namespace dp )

// the examples of the text; against all the 2^n choices, keeping those
// whose runs of consecutive bits are at most k long
ADS_TEST( "dp-37" )
{
 CHECK( dp::kOccurrence( { 9 , 1 , 9 } , 2 ) == 18 );
 const Ints V = { 1 , 7 , 8 , 9 , 1 , 10 , 6 , 8 , 8 };
 CHECK( dp::kOccurrence( V , 2 ) == 44 );
 CHECK( dp::kOccurrence( V , 3 ) == 50 );
 auto brute = []( const Ints & A , int k ) {
  long long best = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m ) {
   int run = 0 , longest = 0;
   for( std::size_t i = 0 ; i < A.size() ; ++i ) {
    run = ( m >> i & 1 ) ? run + 1 : 0;
    longest = std::max( longest , run );
    }
   if( longest <= k ) best = std::max( best , mask_sum( A , m ) );
   }
  return( best );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto A = random_ints( g() % 13 , -10 , 20 , g );
  const int k = int( g() % ( A.size() + 1 ) );
  CHECK( dp::kOccurrence( A , k ) == brute( A , k ) );
  }
 CHECK( dp::kOccurrence( {} , 0 ) == 0 );
 CHECK( dp::kOccurrence( { 5 } , 1 ) == 5 );
 CHECK( dp::kOccurrence( { -5 } , 1 ) == 0 );
 CHECK( dp::kOccurrence( Ints( 10 , 3 ) , 0 ) == 0 );
 CHECK( dp::kOccurrence( Ints( 10 , 3 ) , 10 ) == 30 );
 CHECK( dp::kOccurrence( Ints( 10 , 3 ) , 1 ) == 15 );   // every other one
 CHECK( dp::kOccurrence( Ints( 2000 , 1 ) , 2000 ) == 2000 );
 CHECK( dp::kOccurrence( { -1 , -2 , -3 } , 3 ) == 0 );
 CHECK( dp::kOccurrence( { 4 , -1 , 4 } , 1 ) == 8 );
 CHECK_THROWS( dp::kOccurrence( { 1 , 2 } , 3 ) , contract_violation );
 CHECK_THROWS( dp::kOccurrence( { 1 , 2 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::kOccurrence( {} , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 38, 2016-01-07, Cut string ------------------------*/
/*--------------------------------------------------------------------------*/

// the least cost of cutting a string of length L at the places V, where a
// cut costs the length of the piece cut: with the sentinels W_0 = 0 and
// W_{n+1} = L, DP[ i ][ j ] = min_{i<k<j} DP[ i ][ k ] + DP[ k ][ j ] plus
// W_j - W_i, the scheme of matrixChain() with another cost, which
// stringBreakingCost() of the library solves [see StringBreaking.h]

/// the cost of the cuts of a string of length L in the order given
long long cut_cost( const Ints & order , long long L )
{
 std::set< long long > done = { 0 , L };
 long long c = 0;
 for( long long p : order ) {
  const auto hi = done.upper_bound( p );
  c += *hi - *std::prev( hi );
  done.insert( p );
  }
 return( c );
 }

// the three orders of the text cost 46, 37 and 36, and the optimum is at
// most 36; against every order of the cuts
ADS_TEST( "dp-38" )
{
 CHECK( cut_cost( { 3 , 8 , 10 } , 19 ) == 46 );
 CHECK( cut_cost( { 10 , 8 , 3 } , 19 ) == 37 );
 CHECK( cut_cost( { 10 , 3 , 8 } , 19 ) == 36 );
 Ints V = { 3 , 8 , 10 };
 long long best = std::numeric_limits< long long >::max();
 do best = std::min( best , cut_cost( V , 19 ) );
 while( std::next_permutation( V.begin() , V.end() ) );
 CHECK( stringBreakingCost( { 3 , 8 , 10 } , 19 ) == best && best <= 36 );

 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const long long L = 2 + g() % 30;
  std::set< long long > s;
  const std::size_t n = g() % 7;
  while( ( s.size() < n ) && ( s.size() + 1 < std::size_t( L ) ) )
   s.insert( 1 + g() % ( L - 1 ) );
  Ints P( s.begin() , s.end() );
  long long b = std::numeric_limits< long long >::max();
  do b = std::min( b , cut_cost( P , L ) );
  while( std::next_permutation( P.begin() , P.end() ) );
  CHECK( stringBreakingCost( Ints( s.begin() , s.end() ) , L ) == b );
  }
 CHECK( stringBreakingCost( {} , 7 ) == 0 );
 CHECK( stringBreakingCost( { 4 } , 7 ) == 7 );
 Ints all( 99 );                                  // every place, halving
 std::iota( all.begin() , all.end() , 1 );
 CHECK( stringBreakingCost( all , 100 ) > 0 );
 CHECK( stringBreakingCost( { 1 , 2 , 3 } , 4 ) == 8 );
 CHECK_THROWS( stringBreakingCost( { 3 , 3 } , 7 ) , contract_violation );
 CHECK_THROWS( stringBreakingCost( { 7 } , 7 ) , contract_violation );
 CHECK_THROWS( stringBreakingCost( { 0 } , 7 ) , contract_violation );
 CHECK_THROWS( stringBreakingCost( { 5 , 3 } , 7 ) , contract_violation );
 CHECK( stringBreakingCost( {} , 1 ) == 0 );
 CHECK( stringBreakingCost( { 1 } , 2 ) == 2 );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 39, 2016-01-26, Somma di quadrati ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the fewest squares of positive integers that add up to n. Taking the
/// largest square first fails (18 = 9 + 9, but 16 + 1 + 1); DP[ 0 ] = 0 and
/// DP[ i ] = min_{1 <= t <= sqrt( i )} DP[ i - t^2 ] + 1, bottom up,
/// Theta( n sqrt( n ) ) time and O( n ) space; by Lagrange it is at most 4.
/// minimumSquares() of the library answers in O( sqrt( n ) ) by the
/// theorems of Lagrange and Legendre
/// \pre n >= 0
int squareSum( int n )
{
 return( minimumSquares( n ) );
 }

/// psRec(): the decompositions of n by bases <= limit after those in S
void psRec( int n , std::vector< int > & S , int limit ,
            std::vector< std::vector< int > > & out )
{
 if( n == 0 ) {
  out.push_back( S );
  return;
  }
 for( int t = 1 ; ( t * t <= n ) && ( t <= limit ) ; ++t ) {
  S.push_back( t );
  psRec( n - t * t , S , t , out );
  S.pop_back();
  }
 }

/// every way of writing n as a sum of squares, each multiset once, as the
/// bases in non-increasing order (the lines that the text prints): by
/// backtrack, each base at most the previous one (the limit), which avoids
/// the permutations; superpolynomial time
/// \pre n >= 0
std::vector< std::vector< int > > printSquares( int n )
{
 ADS_PRE( n >= 0 );
 std::vector< std::vector< int > > out;
 std::vector< int > S;
 int r = 0;
 while( ( r + 1 ) * ( r + 1 ) <= n ) ++r;
 psRec( n , S , r , out );
 return( out );
 }

}  // end( namespace dp )

// the examples 7, 13 and 18, where the greedy takes 3 squares; against
// the theorem of Legendre: 1 for a square, 2 for a sum of two squares, 4
// for 4^a ( 8 b + 7 ), 3 otherwise
ADS_TEST( "dp-39-a" )
{
 CHECK( dp::squareSum( 7 ) == 4 );
 CHECK( dp::squareSum( 13 ) == 2 );
 CHECK( dp::squareSum( 18 ) == 2 );
 auto greedy = []( int n ) {
  int c = 0;
  while( n > 0 ) {
   int t = 1;
   while( ( t + 1 ) * ( t + 1 ) <= n ) ++t;
   n -= t * t;
   ++c;
   }
  return( c );
  };
 CHECK( greedy( 18 ) == 3 );
 auto legendre = []( int n ) {
  auto square = []( int x ) {
   int t = 0;
   while( ( t + 1 ) * ( t + 1 ) <= x ) ++t;
   return( t * t == x );
   };
  if( n == 0 ) return( 0 );
  if( square( n ) ) return( 1 );
  for( int a = 1 ; a * a <= n ; ++a )
   if( square( n - a * a ) ) return( 2 );
  int m = n;
  while( m % 4 == 0 ) m /= 4;
  return( m % 8 == 7 ? 4 : 3 );
  };
 for( int n = 0 ; n <= 3000 ; ++n )
  CHECK( dp::squareSum( n ) == legendre( n ) );
 CHECK( dp::squareSum( 1 ) == 1 );
 CHECK( dp::squareSum( 100000 ) == legendre( 100000 ) );
 CHECK_THROWS( dp::squareSum( -1 ) , contract_violation );
 }

// the six ways of 13 of the text; against the count of the partitions of n
// into squares (the coin change with the coins 1, 4, 9, ...), each line
// adding up to n, in non-increasing order and printed once
ADS_TEST( "dp-39-b" )
{
 using Lines = std::vector< std::vector< int > >;
 auto ones = []( std::vector< int > s , int k ) {
  s.insert( s.end() , k , 1 );
  return( s );
  };
 Lines E = { ones( {} , 13 ) , ones( { 2 } , 9 ) , ones( { 2 , 2 } , 5 ) ,
             { 2 , 2 , 2 , 1 } , ones( { 3 } , 4 ) , { 3 , 2 } };
 auto P = dp::printSquares( 13 );
 std::sort( P.begin() , P.end() );
 std::sort( E.begin() , E.end() );
 CHECK( P == E );
 for( int n = 0 ; n <= 80 ; ++n ) {
  std::vector< std::uint64_t > ways( n + 1 , 0 );
  ways[ 0 ] = 1;
  for( int t = 1 ; t * t <= n ; ++t )
   for( int s = t * t ; s <= n ; ++s ) ways[ s ] += ways[ s - t * t ];
  const auto L = dp::printSquares( n );
  CHECK( L.size() == ways[ n ] );
  for( const auto & s : L ) {
   int sum = 0;
   for( int b : s ) sum += b * b;
   CHECK( sum == n );
   CHECK( std::is_sorted( s.rbegin() , s.rend() ) );
   }
  CHECK( std::set< std::vector< int > >( L.begin() , L.end() ).size() ==
         L.size() );
  }
 CHECK( dp::printSquares( 0 ) == Lines( 1 ) );
 CHECK( dp::printSquares( 1 ) == Lines( { { 1 } } ) );
 CHECK( dp::printSquares( 2 ) == Lines( { { 1 , 1 } } ) );
 CHECK( dp::printSquares( 4 ) == Lines( { { 1 , 1 , 1 , 1 } , { 2 } } ) ||
        dp::printSquares( 4 ) == Lines( { { 2 } , { 1 , 1 , 1 , 1 } } ) );
 CHECK_THROWS( dp::printSquares( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 40, 2016-06-06, Sequenze ordinate ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the vectors of length n of values in [ 1 , k ] in non-decreasing order,
/// first version: DP[ 0 ][ k ] = 1 and DP[ n ][ k ] =
/// sum_{i=1}^k DP[ n - 1 ][ i ] (the smallest value i leaves the values
/// i ... k to the others), bottom up, O( n k^2 ) time; the code of the
/// text subtracts in place of adding
/// \pre n >= 0, k >= 0, C( n + k - 1 , n ) < 2^64
std::uint64_t orderedSum( int n , int k )
{
 ADS_PRE( ( n >= 0 ) && ( k >= 0 ) );
 UTable DP( n + 1 , std::vector< std::uint64_t >( k + 1 , 0 ) );
 for( int j = 0 ; j <= k ; ++j ) DP[ 0 ][ j ] = 1;
 for( int i = 1 ; i <= n ; ++i )
  for( int j = 1 ; j <= k ; ++j )
   for( int v = 1 ; v <= j ; ++v ) DP[ i ][ j ] += DP[ i - 1 ][ v ];
 return( DP[ n ][ k ] );
 }

/// opRec(): DP[ n ][ k ] of the second version, UNSOLVED if not solved yet
std::uint64_t opRec( int n , int k , UTable & DP )
{
 if( n == 1 ) return( std::uint64_t( k ) );
 if( k == 0 ) return( 0 );
 if( DP[ n ][ k ] == UNSOLVED )
  DP[ n ][ k ] = opRec( n - 1 , k , DP ) + opRec( n , k - 1 , DP );
 return( DP[ n ][ k ] );
 }

/// the same count, second version: DP[ 1 ][ k ] = k, DP[ n ][ 0 ] = 0 and
/// DP[ n ][ k ] = DP[ n - 1 ][ k ] (the first value is k) + DP[ n ][ k - 1 ]
/// (it is less than k); by memoization, O( n k ) time and space. The code
/// of the text swaps the arguments of the second call, opRec( k , n - 1 );
/// the closed form is C( n + k - 1 , n )
/// \pre n >= 1, k >= 0, C( n + k - 1 , n ) < 2^64
std::uint64_t orderedPermutation( int n , int k )
{
 ADS_PRE( ( n >= 1 ) && ( k >= 0 ) );
 UTable DP( n + 1 , std::vector< std::uint64_t >( k + 1 , UNSOLVED ) );
 return( opRec( n , k , DP ) );
 }

}  // end( namespace dp )

// the example n = 4, k = 3 gives 15; both versions against the closed form
// C( n + k - 1 , n ) and against the enumeration of the k^n vectors
ADS_TEST( "dp-40" )
{
 CHECK( dp::orderedPermutation( 4 , 3 ) == 15 );
 CHECK( dp::orderedSum( 4 , 3 ) == 15 );
 const auto T = tartaglia( 67 );
 for( int n = 1 ; n <= 12 ; ++n )
  for( int k = 0 ; k <= 12 ; ++k ) {
   const std::uint64_t c = k ? T[ n + k - 1 ][ n ] : 0;
   CHECK( dp::orderedPermutation( n , k ) == c );
   CHECK( dp::orderedSum( n , k ) == c );
   }
 for( int n = 1 ; n <= 6 ; ++n )
  for( int k = 1 ; k <= 5 ; ++k ) {
   std::uint64_t count = 0;
   std::vector< int > v( n , 1 );
   for( ; ; ) {
    count += std::is_sorted( v.begin() , v.end() );
    int i = n - 1;
    while( ( i >= 0 ) && ( v[ i ] == k ) ) v[ i-- ] = 1;
    if( i < 0 ) break;
    ++v[ i ];
    }
   CHECK( dp::orderedPermutation( n , k ) == count );
   }
 CHECK( dp::orderedSum( 0 , 5 ) == 1 );
 CHECK( dp::orderedPermutation( 1 , 0 ) == 0 );
 CHECK( dp::orderedPermutation( 7 , 1 ) == 1 );
 CHECK( dp::orderedPermutation( 34 , 34 ) == T[ 67 ][ 34 ] );
 CHECK( dp::orderedSum( 34 , 34 ) == T[ 67 ][ 34 ] );
 CHECK( dp::orderedSum( 0 , 0 ) == 1 && dp::orderedSum( 3 , 0 ) == 0 );
 CHECK( dp::orderedSum( 1 , 9 ) == 9 && dp::orderedPermutation( 1 , 9 ) == 9 );
 CHECK_THROWS( dp::orderedPermutation( 0 , 3 ) , contract_violation );
 CHECK_THROWS( dp::orderedPermutation( 2 , -1 ) , contract_violation );
 CHECK_THROWS( dp::orderedSum( -1 , 3 ) , contract_violation );
 CHECK_THROWS( dp::orderedSum( 2 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 41, 2016-06-06, Silicon Valley 2 ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// a general tree by leftmost child and right sibling, -1 for nil
struct Tree {
 std::vector< int > child , sibling;
 int root = -1;
 };

int computeS( const Tree & T , int u , std::vector< int > & S ,
              std::vector< int > & L );

/// computeL(): the fewest nodes covering the subtree of u, u free
int computeL( const Tree & T , int u , std::vector< int > & S ,
              std::vector< int > & L )
{
 if( u < 0 ) return( 0 );
 if( L[ u ] < 0 ) {
  int tot = 0;
  for( int f = T.child[ u ] ; f >= 0 ; f = T.sibling[ f ] )
   tot += computeS( T , f , S , L );
  L[ u ] = std::min( computeS( T , u , S , L ) , tot );
  }
 return( L[ u ] );
 }

/// computeS(): the fewest nodes covering the subtree of u, u taken
int computeS( const Tree & T , int u , std::vector< int > & S ,
              std::vector< int > & L )
{
 if( u < 0 ) return( 0 );
 if( S[ u ] < 0 ) {
  int tot = 0;
  for( int f = T.child[ u ] ; f >= 0 ; f = T.sibling[ f ] )
   tot += computeL( T , f , S , L );
  S[ u ] = 1 + tot;
  }
 return( S[ u ] );
 }

/// the fewest nodes (spies) of a tree such that each edge has an end
/// among them, the vertex cover of a tree: S[ u ] = 1 + sum_f L[ f ] with u
/// taken, L[ u ] = min( S[ u ] , sum_f S[ f ] ) with u free, 0 for nil, by
/// memoization, or treeVertexCover() of the library, the parent of each
/// leaf taken, on the tree as an unoriented graph; O( n ) time and space.
/// The answer is L[ root ]: the text returns S[ root ], which forces the
/// root into the cover
/// \pre child and sibling have the same size, root a node or -1
int spies( const Tree & T )
{
 const int n = int( T.child.size() );
 ADS_PRE( int( T.sibling.size() ) == n );
 ADS_PRE( ( T.root >= -1 ) && ( T.root < n ) );
 ListGraph<> G( n , false );
 for( int u = 0 ; u < n ; ++u )
  for( int f = T.child[ u ] ; f >= 0 ; f = T.sibling[ f ] )
   G.insertEdge( u , f );
 return( int( treeVertexCover( G ).size() ) );
 }

/// the tree of the parents p, p[ 0 ] = -1 the root and p[ i ] < i
Tree from_parents( const std::vector< int > & p )
{
 const int n = int( p.size() );
 Tree T{ std::vector< int >( n , -1 ) , std::vector< int >( n , -1 ) ,
         n ? 0 : -1 };
 for( int i = n - 1 ; i >= 1 ; --i ) {
  T.sibling[ i ] = T.child[ p[ i ] ];
  T.child[ p[ i ] ] = i;
  }
 return( T );
 }

}  // end( namespace dp )

// against all the subsets of the nodes of random trees, and against the
// decision vertexCover() of the library on k = the answer and k - 1; the
// empty tree, a star, a path of 10^5 nodes and the contract violations
ADS_TEST( "dp-41" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 12 );
  std::vector< int > p( n , -1 );
  for( int i = 1 ; i < n ; ++i ) p[ i ] = int( g() % unsigned( i ) );
  int best = n;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   bool cover = true;
   for( int i = 1 ; i < n ; ++i )
    cover = cover && ( ( m >> i & 1 ) || ( m >> p[ i ] & 1 ) );
   if( cover ) best = std::min( best , std::popcount( m ) );
   }
  const int s = dp::spies( dp::from_parents( p ) );
  CHECK( s == best );
  if( t % 10 == 0 ) {
   Adjacency G( n , std::vector< bool >( n , false ) );
   for( int i = 1 ; i < n ; ++i ) G[ i ][ p[ i ] ] = G[ p[ i ] ][ i ] = true;
   CHECK( vertexCover( G , s ).has_value() );
   if( s > 0 ) CHECK( ! vertexCover( G , s - 1 ).has_value() );
   }
  }
 CHECK( dp::spies( dp::from_parents( {} ) ) == 0 );
 CHECK( dp::spies( dp::from_parents( { -1 } ) ) == 0 );
 CHECK( dp::spies( dp::from_parents( { -1 , 0 } ) ) == 1 );
 std::vector< int > star( 11 , 0 ) , path( 2001 );
 star[ 0 ] = -1;
 CHECK( dp::spies( dp::from_parents( star ) ) == 1 );
 for( int i = 0 ; i <= 2000 ; ++i ) path[ i ] = i - 1;
 CHECK( dp::spies( dp::from_parents( path ) ) == 1000 );
 // on a path of five nodes S[ root ], the answer of the text, is 3, while
 // the cover of the second and the fourth node has 2
 const auto T = dp::from_parents( { -1 , 0 , 1 , 2 , 3 } );
 std::vector< int > S( 5 , -1 ) , L( 5 , -1 );
 CHECK( dp::computeS( T , 0 , S , L ) == 3 );
 CHECK( dp::spies( T ) == 2 );
 // a path of 10^5 nodes, too deep for a recursion on the stack
 std::vector< int > deep( 100000 );
 for( int i = 0 ; i < 100000 ; ++i ) deep[ i ] = i - 1;
 CHECK( dp::spies( dp::from_parents( deep ) ) == 50000 );
 dp::Tree bad = dp::from_parents( { -1 , 0 } );
 bad.sibling.pop_back();
 CHECK_THROWS( dp::spies( bad ) , contract_violation );
 dp::Tree out = dp::from_parents( { -1 , 0 } );
 out.root = 2;
 CHECK_THROWS( dp::spies( out ) , contract_violation );
 out.root = -2;
 CHECK_THROWS( dp::spies( out ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 42, 2016-07-06, Serve the Donald ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the most voters e of a choice of the cities at the places m (in
/// increasing order) such that two chosen cities are at least D apart.
/// DP[ j ] is the best of the first j cities, DP[ 0 ] = 0 and DP[ j ] =
/// max( DP[ j - 1 ] , e_j + DP[ last ] ), where the cities 1 ... last are
/// those at least D before city j; since last never moves back, Theta( n )
/// time and O( n ) space. The two codes of the text differ: the quadratic
/// one never takes e_j alone when no city is D before j (m = { 0 , 1 },
/// e = { 1 , 10 } gives 1), and the linear one adds e_j only to the
/// cities that its pointer passes at step j, so it misses DP[ last ] + e_j
/// at the later steps (m = { 0 , 10 , 11 }, e = { 1 , 1 , 5 }, D = 5
/// gives 2 in place of 6)
/// \pre m increasing, e >= 0, D > 0, same sizes
long long serveTheDonald( const Ints & m , const Ints & e , long long D )
{
 const std::size_t n = m.size();
 ADS_PRE( ( e.size() == n ) && ( D > 0 ) );
 ADS_PRE( std::adjacent_find( m.begin() , m.end() ,
                              std::greater_equal<>() ) == m.end() );
 ADS_PRE( std::all_of( e.begin() , e.end() ,
                       []( long long x ) { return( x >= 0 ); } ) );
 Ints DP( n + 1 , 0 );
 std::size_t last = 0;
 for( std::size_t j = 1 ; j <= n ; ++j ) {
  while( ( last + 1 < j ) && ( m[ j - 1 ] - m[ last ] >= D ) ) ++last;
  DP[ j ] = std::max( DP[ j - 1 ] , e[ j - 1 ] + DP[ last ] );
  }
 return( DP[ n ] );
 }

}  // end( namespace dp )

// the two counterexamples to the codes of the text; against all the
// subsets of the cities and against maxinterval() of the library on the
// intervals [ m_i - D , m_i ) of weights e_i
ADS_TEST( "dp-42" )
{
 CHECK( dp::serveTheDonald( { 0 , 1 } , { 1 , 10 } , 5 ) == 10 );
 CHECK( dp::serveTheDonald( { 0 , 10 , 11 } , { 1 , 1 , 5 } , 5 ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 13;
  std::set< long long > s;
  while( s.size() < n ) s.insert( g() % 40 );
  const Ints m( s.begin() , s.end() ) , e = random_ints( n , 0 , 9 , g );
  const long long D = 1 + g() % 10;
  long long best = 0;
  for( unsigned k = 0 ; k < ( 1u << n ) ; ++k ) {
   const Ints c = mask_items( m , k );
   bool ok = true;
   for( std::size_t i = 1 ; i < c.size() ; ++i )
    ok = ok && ( c[ i ] - c[ i - 1 ] >= D );
   if( ok ) best = std::max( best , mask_sum( e , k ) );
   }
  std::vector< WeightedInterval< long long > > I;
  for( std::size_t i = 0 ; i < n ; ++i )
   I.push_back( { long( m[ i ] - D ) , long( m[ i ] ) , e[ i ] } );
  CHECK( dp::serveTheDonald( m , e , D ) == best );
  CHECK( maxinterval( I ).second == best );
  }
 CHECK( dp::serveTheDonald( {} , {} , 3 ) == 0 );
 CHECK( dp::serveTheDonald( { 5 } , { 7 } , 3 ) == 7 );
 Ints far( 1000 ) , ones( 1000 , 1 );
 std::iota( far.begin() , far.end() , 0 );
 CHECK( dp::serveTheDonald( far , ones , 1 ) == 1000 );
 CHECK( dp::serveTheDonald( far , ones , 1000 ) == 1 );
 CHECK( dp::serveTheDonald( far , ones , 10 ) == 100 );
 CHECK( dp::serveTheDonald( { 0 , 5 } , { 3 , 4 } , 5 ) == 7 );   // exactly D
 CHECK( dp::serveTheDonald( { 0 , 4 } , { 3 , 4 } , 5 ) == 4 );
 CHECK( dp::serveTheDonald( { -10 , 0 , 10 } , { 0 , 0 , 0 } , 1 ) == 0 );
 CHECK_THROWS( dp::serveTheDonald( { 1 , 1 } , { 1 , 1 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( dp::serveTheDonald( { 2 , 1 } , { 1 , 1 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( dp::serveTheDonald( { 1 } , { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( dp::serveTheDonald( { 1 } , { -1 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( dp::serveTheDonald( { 1 } , {} , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 43, 2016-08-29, Count ---------------------------*/
/*--------------------------------------------------------------------------*/

// the occurrences of P as a subsequence of T, i.e., the places i_1 < ...
// < i_m with T[ i_j ] = P[ j ]: DP[ p ][ t ], those of P[ 1 .. p ] in
// T[ 1 .. t ], is 0 if p > t, 1 if p = 0, DP[ p ][ t - 1 ] if P_p != T_t,
// and DP[ p - 1 ][ t - 1 ] + DP[ p ][ t - 1 ] otherwise, the recursion of
// subsequenceOccurrences() of the library [see SubsequenceProblems.h]

// the example abc in aabdcc, 4 times; against the enumeration of the
// C( n , m ) choices of places
ADS_TEST( "dp-43" )
{
 CHECK( subsequenceOccurrences( "abc" , "aabdcc" ) == 4 );
 std::function< std::uint64_t( const std::string & , const std::string & ,
                               std::size_t , std::size_t ) > brute =
  [ & ]( const std::string & P , const std::string & T , std::size_t p ,
         std::size_t t ) -> std::uint64_t {
   if( p == P.size() ) return( 1 );
   std::uint64_t c = 0;
   for( std::size_t i = t ; i < T.size() ; ++i )
    if( T[ i ] == P[ p ] ) c += brute( P , T , p + 1 , i + 1 );
   return( c );
   };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto T = random_string( g() % 13 , 2 + g() % 2 , g );
  const auto P = random_string( g() % 5 , 2 + g() % 2 , g );
  CHECK( subsequenceOccurrences( P , T ) == brute( P , T , 0 , 0 ) );
  }
 const auto C = tartaglia( 60 );
 CHECK( subsequenceOccurrences( "" , "" ) == 1 );
 CHECK( subsequenceOccurrences( "" , "abc" ) == 1 );
 CHECK( subsequenceOccurrences( "abcd" , "abc" ) == 0 );
 CHECK( subsequenceOccurrences( "a" , "a" ) == 1 );
 CHECK( subsequenceOccurrences( "a" , "" ) == 0 );
 CHECK( subsequenceOccurrences( "z" , "abc" ) == 0 );
 CHECK( subsequenceOccurrences( "abc" , "abc" ) == 1 );
 CHECK( subsequenceOccurrences( "cba" , "abc" ) == 0 );
 CHECK( subsequenceOccurrences( "aaa" , std::string( 50 , 'a' ) ) ==
        C[ 50 ][ 3 ] );
 CHECK( subsequenceOccurrences( std::string( 30 , 'a' ) ,
                                std::string( 60 , 'a' ) ) == C[ 60 ][ 30 ] );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 44, 2017-01-10, Supersequenza -----------------------*/
/*--------------------------------------------------------------------------*/

// the length of a shortest common supersequence of P and T: DP[ i ][ 0 ]
// = i, DP[ 0 ][ j ] = j, DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] + 1 if P_i =
// T_j, else min( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ) + 1, which equals
// n + m - | LCS( P , T ) |: scsLength() of the library [see
// SubsequenceProblems.h]

/// is a a subsequence of b?
bool is_subsequence( const std::string & a , const std::string & b )
{
 std::size_t i = 0;
 for( char c : b )
  if( ( i < a.size() ) && ( a[ i ] == c ) ) ++i;
 return( i == a.size() );
 }

// the examples AB, BC (ABC) and DAB, DCB (DACB); against n + m - | lcs |
// of the library and, on short strings, the shortest string over the
// alphabet that has both as subsequences
ADS_TEST( "dp-44" )
{
 CHECK( scsLength( "AB" , "BC" ) == 3 );
 CHECK( scsLength( "DAB" , "DCB" ) == 4 );
 CHECK( is_subsequence( "DAB" , "DACB" ) && is_subsequence( "DCB" , "DACB" ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto P = random_string( g() % 15 , 3 , g ) ,
             T = random_string( g() % 15 , 3 , g );
  CHECK( scsLength( P , T ) == P.size() + T.size() - lcs( P , T ).size() );
  }
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto P = random_string( g() % 4 , 2 , g ) ,
             T = random_string( g() % 4 , 2 , g );
  int best = -1;
  for( int len = 0 ; best < 0 ; ++len )
   for( unsigned w = 0 ; ( w < ( 1u << len ) ) && ( best < 0 ) ; ++w ) {
    std::string s;
    for( int i = 0 ; i < len ; ++i ) s += char( 'a' + ( w >> i & 1 ) );
    if( is_subsequence( P , s ) && is_subsequence( T , s ) ) best = len;
    }
  CHECK( int( scsLength( P , T ) ) == best );
  }
 CHECK( scsLength( "" , "" ) == 0 );
 CHECK( scsLength( "abc" , "" ) == 3 );
 CHECK( scsLength( "abc" , "abc" ) == 3 );
 CHECK( scsLength( "" , "abc" ) == 3 );
 CHECK( scsLength( "ac" , "abc" ) == 3 && scsLength( "abc" , "ac" ) == 3 );
 CHECK( scsLength( "abc" , "cba" ) == 5 );
 CHECK( scsLength( std::string( 500 , 'a' ) , std::string( 500 , 'b' ) ) ==
        1000 );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 45, 2017-01-24, Count palyndrome ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// cpRec(): the fewest palindromes of S[ i .. j ], INF if not solved yet
int cpRec( const std::string & S , int i , int j ,
           std::vector< std::vector< int > > & DP )
{
 constexpr int INF = std::numeric_limits< int >::max();
 if( i >= j ) return( 1 );
 if( ( cpRec( S , i + 1 , j - 1 , DP ) == 1 ) && ( S[ i ] == S[ j ] ) )
  return( 1 );
 if( DP[ i ][ j ] == INF )
  for( int k = i ; k < j ; ++k )
   DP[ i ][ j ] = std::min( DP[ i ][ j ] , cpRec( S , i , k , DP ) +
                                           cpRec( S , k + 1 , j , DP ) );
 return( DP[ i ][ j ] );
 }

/// the fewest palindromic substrings into which S can be cut, first
/// version: DP[ i ][ j ] = 1 if S[ i .. j ] is a palindrome, else
/// min_{i <= k < j} DP[ i ][ k ] + DP[ k + 1 ][ j ]; by memoization,
/// Theta( n^3 ) time; 0 for the empty string (cpRec() would give 1)
int countPalyndromeCubic( const std::string & S )
{
 const int n = int( S.size() );
 if( n == 0 ) return( 0 );
 std::vector< std::vector< int > > DP( n , std::vector< int >( n ,
                                     std::numeric_limits< int >::max() ) );
 return( cpRec( S , 0 , n - 1 , DP ) );
 }

/// the same, second version: DP[ 0 ] = 0 and DP[ j ] = min DP[ i - 1 ] + 1
/// over the palindromes S[ i .. j ], Theta( n^2 ) time, which
/// palindromicPartition() of the library computes [see Palindromes.h]. The
/// intermediate version of the text, with palyndrome() in O( n ), takes
/// the max in place of the min
int countPalyndrome( const std::string & S )
{
 return( int( palindromicPartition( S ).size() ) );
 }

}  // end( namespace dp )

// the examples BUB.BASEESAB.ANANA (3) and AFISICACISIFA (1); both versions
// against all the 2^( n - 1 ) ways of cutting the string
ADS_TEST( "dp-45" )
{
 for( auto f : { dp::countPalyndromeCubic , dp::countPalyndrome } ) {
  CHECK( f( "BUBBASEESABANANA" ) == 3 );
  CHECK( f( "AFISICACISIFA" ) == 1 );
  CHECK( f( "" ) == 0 );
  CHECK( f( "x" ) == 1 );
  CHECK( f( std::string( 60 , 'a' ) ) == 1 );
  CHECK( f( "abcdefghij" ) == 10 );
  }
 auto palindrome = []( const std::string & s ) {
  return( std::equal( s.begin() , s.end() , s.rbegin() ) );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const auto S = random_string( 1 + g() % 12 , 2 + g() % 2 , g );
  const std::size_t n = S.size();
  int best = int( n );
  for( unsigned m = 0 ; m < ( 1u << ( n - 1 ) ) ; ++m ) {
   int parts = 0;
   bool ok = true;
   std::size_t start = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ( i == n - 1 ) || ( m >> i & 1 ) ) {
     ok = ok && palindrome( S.substr( start , i + 1 - start ) );
     ++parts;
     start = i + 1;
     }
   if( ok ) best = std::min( best , parts );
   }
  CHECK( dp::countPalyndromeCubic( S ) == best );
  CHECK( dp::countPalyndrome( S ) == best );
  }
 const auto L = random_string( 80 , 2 , g );
 CHECK( dp::countPalyndrome( L ) == dp::countPalyndromeCubic( L ) );
 }

/*--------------------------------------------------------------------------*/
/*--- 46 and 49, 2017-02-07 and 2017-06-08, K limitato and Count tree ------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// ctRec(): DP[ n ][ h ], UNSOLVED if not solved yet
std::uint64_t ctRec( int n , int h , UTable & DP )
{
 if( h < 0 ) return( 0 );
 if( n <= 1 ) return( 1 );
 if( DP[ n ][ h ] == UNSOLVED ) {
  DP[ n ][ h ] = 0;
  for( int k = 0 ; k < n ; ++k )
   DP[ n ][ h ] += ctRec( k , h - 1 , DP ) * ctRec( n - 1 - k , h - 1 , DP );
  }
 return( DP[ n ][ h ] );
 }

/// the binary trees of n nodes and height at most h (k-limitato of the
/// first text, countTree of the second): DP[ n ][ h ] = 0 if h < 0, 1 if
/// n <= 1, and otherwise sum_{k=0}^{n-1} DP[ k ][ h - 1 ] DP[ n - 1 - k ][
/// h - 1 ], k nodes on the left; by memoization, O( n^2 h ) time (the
/// second text says O( n h )) and O( n h ) space, with h at most n - 1,
/// beyond which every tree counts (the Catalan number)
/// \pre 0 <= n <= 36 (the counts below 2^64), h >= 0
// candidate for the library
std::uint64_t countTree( int n , int h )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 36 ) && ( h >= 0 ) );
 h = std::min( h , std::max( n - 1 , 0 ) );
 UTable DP( n + 1 , std::vector< std::uint64_t >( h + 1 , UNSOLVED ) );
 return( ctRec( n , h , DP ) );
 }

}  // end( namespace dp )

// against the heights of all the shapes of n nodes, n <= 12; for h >= n - 1
// the Catalan numbers, also the largest one, C( 36 ), as a BigInteger
ADS_TEST( "dp-46" )
{
 std::vector< std::vector< int > > H = { { -1 } };    // the heights, by n
 for( int n = 1 ; n <= 12 ; ++n ) {
  std::vector< int > hs;
  for( int i = 0 ; i < n ; ++i )
   for( int a : H[ i ] )
    for( int b : H[ n - 1 - i ] ) hs.push_back( 1 + std::max( a , b ) );
  H.push_back( hs );
  for( int h = 0 ; h <= n ; ++h )
   CHECK( dp::countTree( n , h ) ==
          std::uint64_t( std::count_if( hs.begin() , hs.end() ,
                                [ h ]( int x ) { return( x <= h ); } ) ) );
  }
 CHECK( dp::countTree( 0 , 0 ) == 1 );
 CHECK( dp::countTree( 1 , 0 ) == 1 );
 CHECK( dp::countTree( 2 , 0 ) == 0 );
 CHECK( dp::countTree( 3 , 1 ) == 1 );
 CHECK( dp::countTree( 7 , 2 ) == 1 );                       // the full one
 for( int n = 0 ; n <= 33 ; ++n )
  CHECK( dp::countTree( n , 1000 ) == catalan( n ) );
 const BigInteger c36 = factorial( 72 ) /
                        ( factorial( 37 ) * factorial( 36 ) );
 CHECK( std::to_string( dp::countTree( 36 , 35 ) ) == c36.toString() );
 CHECK( dp::countTree( 0 , 1000 ) == 1 && dp::countTree( 36 , 0 ) == 0 );
 CHECK( dp::countTree( 15 , 3 ) == 1 );                    // full, height 3
 CHECK( dp::countTree( 16 , 3 ) == 0 );
 CHECK_THROWS( dp::countTree( 37 , 36 ) , contract_violation );
 CHECK_THROWS( dp::countTree( -1 , 0 ) , contract_violation );
 CHECK_THROWS( dp::countTree( 3 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 47, 2017-02-07, Longest balanced ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

// the length of a longest subsequence of S that is a balanced string of
// round parentheses: a scan matches each ')' with the nearest '(' still
// open, longestBalancedSubsequence() of the library [see Parentheses.h]

/// the same, by the interval recursion of the text: DP[ i ][ j ] = 0 if i
/// >= j, DP[ i + 1 ][ j ] if S_i != '(', DP[ i ][ j - 1 ] if S_j != ')',
/// else the larger of DP[ i + 1 ][ j - 1 ] + 2 and of the splits
/// DP[ i ][ k ] + DP[ k + 1 ][ j ], i <= k < j; bottom up, O( n^3 ) time
/// (the text lets k reach j, which is DP[ i ][ j ] itself)
int longestBalancedDP( const std::string & S )
{
 const int n = int( S.size() );
 if( n == 0 ) return( 0 );
 std::vector< std::vector< int > > DP( n + 1 , std::vector< int >( n , 0 ) );
 for( int len = 2 ; len <= n ; ++len )
  for( int i = 0 ; i + len <= n ; ++i ) {
   const int j = i + len - 1;
   if( S[ i ] != '(' ) DP[ i ][ j ] = DP[ i + 1 ][ j ];
   else if( S[ j ] != ')' ) DP[ i ][ j ] = DP[ i ][ j - 1 ];
   else {
    DP[ i ][ j ] = ( i + 1 <= j - 1 ? DP[ i + 1 ][ j - 1 ] : 0 ) + 2;
    for( int k = i ; k < j ; ++k )
     DP[ i ][ j ] = std::max( DP[ i ][ j ] , DP[ i ][ k ] + DP[ k + 1 ][ j ] );
    }
   }
 return( DP[ 0 ][ n - 1 ] );
 }

}  // end( namespace dp )

// the example of the text (10), and "()()", where the recursion without the
// splits gives 2; the scan and the recursion against all the subsequences
ADS_TEST( "dp-47" )
{
 const std::string E = "che)esercizio)(interess)()ante((())questo()qui";
 CHECK( longestBalancedSubsequence( E ) == 10 );
 CHECK( dp::longestBalancedDP( E ) == 10 );
 CHECK( longestBalancedSubsequence( "()()" ) == 4 );
 CHECK( dp::longestBalancedDP( "()()" ) == 4 );
 std::function< int( const std::string & , int , int ) > nosplit =
  [ & ]( const std::string & S , int i , int j ) -> int {
   if( i >= j ) return( 0 );
   if( S[ i ] != '(' ) return( nosplit( S , i + 1 , j ) );
   if( S[ j ] != ')' ) return( nosplit( S , i , j - 1 ) );
   return( nosplit( S , i + 1 , j - 1 ) + 2 );
   };
 CHECK( nosplit( "()()" , 0 , 3 ) == 2 );
 auto balanced = []( const std::string & s ) {
  int d = 0;
  for( char c : s ) {
   d += c == '(' ? 1 : -1;
   if( d < 0 ) return( false );
   }
  return( d == 0 );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::string S;
  for( std::size_t i = g() % 15 ; i > 0 ; --i )
   S += "()x"[ g() % 3 ];
  std::string p;
  for( char c : S ) if( c != 'x' ) p += c;
  int best = 0;
  for( unsigned m = 0 ; m < ( 1u << p.size() ) ; ++m ) {
   std::string s;
   for( std::size_t i = 0 ; i < p.size() ; ++i )
    if( m >> i & 1 ) s += p[ i ];
   if( balanced( s ) ) best = std::max( best , int( s.size() ) );
   }
  CHECK( int( longestBalancedSubsequence( S ) ) == best );
  CHECK( dp::longestBalancedDP( S ) == best );
  }
 CHECK( longestBalancedSubsequence( "" ) == 0 );
 CHECK( longestBalancedSubsequence( ")(" ) == 0 );
 CHECK( longestBalancedSubsequence( std::string( 1000 , '(' ) ) == 0 );
 CHECK( longestBalancedSubsequence( std::string( 500 , '(' ) +
                                    std::string( 500 , ')' ) ) == 1000 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 48, 2017-06-08, Count ---------------------------*/
/*--------------------------------------------------------------------------*/

// the ways of decoding the digits T, where A ... Z are 1 ... 26: DP[ i ]
// counts those of T[ i .. n ], DP[ n + 1 ] = 1, DP[ i ] = 0 if T_i = 0,
// DP[ i + 1 ] + DP[ i + 2 ] if T_i = 1, or T_i = 2 and T_{i+1} <= 6, and
// DP[ i + 1 ] otherwise: countDecodings() of the library [see
// Segmentations.h]. The code of the text lacks the else after the case
// T_i = 0, which the next test overwrites

/// the digits of a string
std::vector< int > digits( const std::string & s )
{
 std::vector< int > d;
 for( char c : s ) d.push_back( c - '0' );
 return( d );
 }

// the examples 13135 (ACME, MACE, MME, ACACE), 011 and 301; against the
// recursion on the pieces 1 ... 9 and 10 ... 26
ADS_TEST( "dp-48" )
{
 CHECK( countDecodings( digits( "13135" ) ) == 4 );
 CHECK( countDecodings( digits( "011" ) ) == 0 );
 CHECK( countDecodings( digits( "301" ) ) == 0 );
 std::function< std::uint64_t( const std::vector< int > & ,
                               std::size_t ) > brute =
  [ & ]( const std::vector< int > & T , std::size_t i ) -> std::uint64_t {
   if( i == T.size() ) return( 1 );
   std::uint64_t c = 0;
   if( T[ i ] >= 1 ) c += brute( T , i + 1 );
   if( ( i + 1 < T.size() ) && ( T[ i ] >= 1 ) &&
       ( 10 * T[ i ] + T[ i + 1 ] <= 26 ) )
    c += brute( T , i + 2 );
   return( c );
   };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::vector< int > T( g() % 16 );
  for( auto & d : T ) d = g() % 4 ? int( g() % 3 ) : int( g() % 10 );
  CHECK( countDecodings( T ) == brute( T , 0 ) );
  }
 CHECK( countDecodings( {} ) == 1 );
 CHECK( countDecodings( { 0 } ) == 0 );
 CHECK( countDecodings( { 7 } ) == 1 );
 CHECK( countDecodings( digits( "2020" ) ) == 1 );
 CHECK( countDecodings( digits( "27" ) ) == 1 );
 std::uint64_t a = 1 , b = 1;                     // F_91 for 90 ones
 for( int i = 3 ; i <= 91 ; ++i ) {
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 CHECK( countDecodings( std::vector< int >( 90 , 1 ) ) == b );
 CHECK( countDecodings( digits( "26" ) ) == 2 );
 CHECK( countDecodings( digits( "10" ) ) == 1 );
 CHECK( countDecodings( digits( "00" ) ) == 0 );
 CHECK( countDecodings( digits( "30" ) ) == 0 );
 CHECK( countDecodings( digits( "100" ) ) == 0 );
 CHECK_THROWS( countDecodings( { 1 , 10 } ) , contract_violation );
 CHECK_THROWS( countDecodings( { -1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 50, 2017-06-29, Https uva -------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the fewest sprinklers, at D_i with radius R_i, that water all of
/// [ 0 , L ], or -1: sorted by left end, each step takes, among those that
/// reach the first point last still dry, the one that reaches farthest;
/// O( n log n ) time for the sort. The code of the text goes on while
/// last <= L, so that when the water reaches L exactly and a sprinkler is
/// left it makes one more step, finds nothing and returns -1 (L = 10,
/// [ 0 , 10 ] and [ 20 , 30 ]); it also starts from the place 0 of
/// arrays from 1; minimumIntervalCover() of the library
/// \pre L > 0, R >= 0, same sizes
int sprinkles( const Ints & D , const Ints & R , long long L )
{
 const std::size_t n = D.size();
 ADS_PRE( ( R.size() == n ) && ( L > 0 ) );
 ADS_PRE( std::all_of( R.begin() , R.end() ,
                       []( long long r ) { return( r >= 0 ); } ) );
 std::vector< std::pair< long long , long long > > I( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  I[ i ] = { D[ i ] - R[ i ] , D[ i ] + R[ i ] };
 const auto cover = minimumIntervalCover( I , 0LL , L );
 return( cover ? int( cover->size() ) : -1 );
 }

}  // end( namespace dp )

// the case where the code of the text fails; against all the subsets of
// the sprinklers, whose union of closed intervals must contain [ 0 , L ]
ADS_TEST( "dp-50" )
{
 CHECK( dp::sprinkles( { 5 , 25 } , { 5 , 5 } , 10 ) == 1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 11;
  const long long L = 1 + g() % 12;
  const Ints D = random_ints( n , -2 , L + 2 , g ) ,
             R = random_ints( n , 0 , 4 , g );
  int best = -1;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   std::vector< std::pair< long long , long long > > I;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( m >> i & 1 ) I.emplace_back( D[ i ] - R[ i ] , D[ i ] + R[ i ] );
   std::sort( I.begin() , I.end() );
   long long reach = 0;
   for( const auto & [ a , b ] : I )
    if( a <= reach ) reach = std::max( reach , b );
   const int c = std::popcount( m );
   if( ( reach >= L ) && ( ( best < 0 ) || ( c < best ) ) ) best = c;
   }
  CHECK( dp::sprinkles( D , R , L ) == best );
  }
 CHECK( dp::sprinkles( {} , {} , 5 ) == -1 );
 CHECK( dp::sprinkles( { 2 } , { 3 } , 5 ) == 1 );
 CHECK( dp::sprinkles( { 4 } , { 3 } , 5 ) == -1 );           // 0 is dry
 CHECK( dp::sprinkles( { 1 , 3 } , { 1 , 1 } , 4 ) == 2 );    // touching
 CHECK( dp::sprinkles( { 1 , 1 , 1 } , { 1 , 1 , 1 } , 2 ) == 1 );
 Ints d( 1000 ) , r( 1000 , 1 );
 for( int i = 0 ; i < 1000 ; ++i ) d[ i ] = 2 * i + 1;
 CHECK( dp::sprinkles( d , r , 2000 ) == 1000 );
 CHECK( dp::sprinkles( { 0 } , { 0 } , 1 ) == -1 );            // a point
 CHECK( dp::sprinkles( { 0 , 5 } , { 5 , 5 } , 10 ) == 1 );
 CHECK( dp::sprinkles( { 0 , 6 } , { 3 , 4 } , 10 ) == 2 );
 CHECK( dp::sprinkles( { 0 , 8 } , { 3 , 3 } , 10 ) == -1 );      // a gap
 CHECK( dp::sprinkles( { -100 } , { 200 } , 5 ) == 1 );
 CHECK_THROWS( dp::sprinkles( { 1 } , { -1 } , 5 ) , contract_violation );
 CHECK_THROWS( dp::sprinkles( { 1 } , { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( dp::sprinkles( { 1 } , {} , 5 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 51, 2017-06-29, Snakes and ladders --------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the fewest throws of a die from the cell 0 to the last one, N = n^2
/// cells, where landing on i moves to V_i (a ladder if V_i > i, a snake if
/// V_i < i): the graph with the edges ( i , V_{i+k} ), 1 <= k <= 6, and
/// the distance() of the library, a BFS; O( n^2 ) time, since | E | <= 6
/// n^2; -1 if the last cell cannot be reached
/// \pre N >= 1, 0 <= V_i < N
int snakesAndLadders( const std::vector< int > & V )
{
 const int N = int( V.size() );
 ADS_PRE( N >= 1 );
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       [ N ]( int v ) {
                        return( ( v >= 0 ) && ( v < N ) ); } ) );
 ListGraph<> G( N , true );
 for( int i = 0 ; i < N ; ++i )
  for( int k = 1 ; ( k <= 6 ) && ( i + k < N ) ; ++k )
   if( V[ i + k ] != i ) G.insertEdge( i , V[ i + k ] );
 const int d = ADS::distance( G , 0 ).first[ N - 1 ];
 return( d == INF ? -1 : d );
 }

}  // end( namespace dp )

// a board without snakes nor ladders, one with a ladder, one where the
// only way passes through a snake, one where every way is closed; against
// the relaxation of the distances N times on random boards
ADS_TEST( "dp-51" )
{
 auto board = []( int N ) {
  std::vector< int > V( N );
  std::iota( V.begin() , V.end() , 0 );
  return( V );
  };
 auto V = board( 36 );
 CHECK( dp::snakesAndLadders( V ) == 6 );                     // ceil 35 / 6
 V[ 3 ] = 35;
 CHECK( dp::snakesAndLadders( V ) == 1 );
 V = board( 36 );
 for( int i = 7 ; i <= 12 ; ++i ) V[ i ] = 0;                 // a wall ...
 CHECK( dp::snakesAndLadders( V ) == -1 );
 V[ 2 ] = 30;                                                 // ... jumped
 CHECK( dp::snakesAndLadders( V ) == 2 );
 // 49 cells: a wall 7 ... 12 back to 0, a ladder 2 -> 40 before a second
 // wall 41 ... 46 of snakes to 20, and a ladder 25 -> 47; the only way is
 // 0 -> 2 -> 40, a snake 41 -> 20, 20 -> 25 -> 47 and 48
 V = board( 49 );
 for( int i = 7 ; i <= 12 ; ++i ) V[ i ] = 0;
 for( int i = 41 ; i <= 46 ; ++i ) V[ i ] = 20;
 V[ 2 ] = 40;
 V[ 25 ] = 47;
 CHECK( dp::snakesAndLadders( V ) == 4 );
 V[ 25 ] = 25;
 CHECK( dp::snakesAndLadders( V ) == -1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 6 ) , N = n * n;
  auto W = board( N );
  for( int i = 1 ; i < N - 1 ; ++i )
   if( g() % 4 == 0 ) W[ i ] = int( g() % unsigned( N ) );
  std::vector< int > d( N , INF );
  d[ 0 ] = 0;
  for( int r = 0 ; r < N ; ++r )
   for( int i = 0 ; i < N ; ++i )
    for( int k = 1 ; ( k <= 6 ) && ( i + k < N ) && ( d[ i ] < INF ) ; ++k )
     d[ W[ i + k ] ] = std::min( d[ W[ i + k ] ] , d[ i ] + 1 );
  CHECK( dp::snakesAndLadders( W ) == ( d[ N - 1 ] == INF ? -1 :
                                                            d[ N - 1 ] ) );
  }
 CHECK( dp::snakesAndLadders( { 0 } ) == 0 );
 CHECK( dp::snakesAndLadders( board( 10000 ) ) == 1667 );
 CHECK( dp::snakesAndLadders( { 0 , 1 } ) == 1 );
 auto S = board( 36 );
 S[ 35 ] = 0;                                     // a snake on the last cell
 CHECK( dp::snakesAndLadders( S ) == -1 );
 CHECK_THROWS( dp::snakesAndLadders( { 0 , 2 } ) , contract_violation );
 CHECK_THROWS( dp::snakesAndLadders( {} ) , contract_violation );
 CHECK_THROWS( dp::snakesAndLadders( { 0 , -1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 52, 2017-09-04, Double length -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// are there two distinct values that occur in A the same number of
/// times? Sort A, count the occurrences of each value into B, sort B and
/// look for two equal neighbours; O( n log n ) time
bool doubleLength( std::vector< long long > A )
{
 std::sort( A.begin() , A.end() );
 std::vector< std::size_t > B;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( ( i == 0 ) || ( A[ i ] != A[ i - 1 ] ) ) B.push_back( 1 );
  else ++B.back();
 std::sort( B.begin() , B.end() );
 return( std::adjacent_find( B.begin() , B.end() ) != B.end() );
 }

}  // end( namespace dp )

// the examples of the text; against std::map and std::set
ADS_TEST( "dp-52" )
{
 CHECK( dp::doubleLength( { 3 , 1 , 3 , 4 , 1 , 4 , 5 } ) );
 CHECK( ! dp::doubleLength( { 3 , 1 , 3 , 3 , 1 } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const auto A = random_ints( g() % 20 , 0 , 1 + g() % 8 , g );
  std::map< long long , int > c;
  for( long long x : A ) ++c[ x ];
  std::set< int > s;
  for( const auto & kv : c ) s.insert( kv.second );
  CHECK( dp::doubleLength( A ) == ( s.size() < c.size() ) );
  }
 CHECK( ! dp::doubleLength( {} ) );
 CHECK( ! dp::doubleLength( { 7 } ) );
 CHECK( ! dp::doubleLength( Ints( 100 , 7 ) ) );
 CHECK( dp::doubleLength( { 7 , 8 } ) );
 Ints big;
 for( int v = 1 ; v <= 300 ; ++v ) big.insert( big.end() , v , v );
 CHECK( ! dp::doubleLength( big ) );
 big.push_back( 1000 );
 CHECK( dp::doubleLength( big ) );
 const long long lo = std::numeric_limits< long long >::min() ,
                 hi = std::numeric_limits< long long >::max();
 CHECK( dp::doubleLength( { lo , hi } ) );
 CHECK( ! dp::doubleLength( { lo , hi , hi } ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 53, 2018-06-04, Kw constraint -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// kwConstraintRec(): DP[ i ][ s ][ v ], -1 if not solved yet
long long kwConstraintRec( const Ints & X , int i , int s , long long v ,
                           std::vector< Table > & DP )
{
 if( ( i == 0 ) || ( s == 0 ) || ( v == 0 ) ) return( 0 );
 auto & d = DP[ i ][ s ][ v ];
 if( d < 0 ) {
  d = kwConstraintRec( X , i - 1 , s , v , DP );
  if( X[ i - 1 ] <= v )
   d = std::max( d , kwConstraintRec( X , i - 1 , s - 1 , v - X[ i - 1 ] ,
                                      DP ) + X[ i - 1 ] );
  }
 return( d );
 }

/// the largest sum of at most k items of X that does not exceed w:
/// DP[ i ][ s ][ v ] is the best with the first i items, at most s of
/// them, up to v: 0 if i, s or v is 0, DP[ i - 1 ][ s ][ v ] if X_i > v,
/// else the larger of that and DP[ i - 1 ][ s - 1 ][ v - X_i ] + X_i; by
/// memoization, O( n k w ) time and space, pseudopolynomial
/// \pre X > 0, 0 <= k <= n, w >= 0
long long kwConstraint( const Ints & X , int k , long long w )
{
 const int n = int( X.size() );
 ADS_PRE( ( k >= 0 ) && ( k <= n ) && ( w >= 0 ) );
 ADS_PRE( std::all_of( X.begin() , X.end() ,
                       []( long long x ) { return( x > 0 ); } ) );
 std::vector< Table > DP( n + 1 , Table( k + 1 , Ints( w + 1 , -1 ) ) );
 return( kwConstraintRec( X , n , k , w , DP ) );
 }

}  // end( namespace dp )

// the example of the text, 25 (26 and 27 cannot be made); against all the
// subsets of at most k items
ADS_TEST( "dp-53" )
{
 const Ints X = { 12 , 13 , 11 , 6 , 23 , 19 };
 CHECK( dp::kwConstraint( X , 3 , 27 ) == 25 );
 CHECK( dp::kwConstraint( X , 3 , 26 ) == 25 );
 CHECK( dp::kwConstraint( X , 3 , 25 ) == 25 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::set< long long > s;
  const std::size_t n = g() % 12;
  while( s.size() < n ) s.insert( 1 + g() % 30 );
  const Ints A( s.begin() , s.end() );
  const int k = int( g() % ( n + 1 ) );
  const long long w = g() % 80;
  long long best = 0;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m )
   if( ( std::popcount( m ) <= k ) && ( mask_sum( A , m ) <= w ) )
    best = std::max( best , mask_sum( A , m ) );
  CHECK( dp::kwConstraint( A , k , w ) == best );
  }
 CHECK( dp::kwConstraint( {} , 0 , 10 ) == 0 );
 CHECK( dp::kwConstraint( X , 0 , 100 ) == 0 );
 CHECK( dp::kwConstraint( X , 6 , 0 ) == 0 );
 CHECK( dp::kwConstraint( X , 6 , 1000 ) == 84 );
 CHECK( dp::kwConstraint( X , 1 , 1000 ) == 23 );
 CHECK( dp::kwConstraint( { 5 } , 1 , 4 ) == 0 );
 CHECK( dp::kwConstraint( { 5 } , 1 , 5 ) == 5 );
 CHECK( dp::kwConstraint( Ints( 10 , 3 ) , 4 , 100 ) == 12 );
 CHECK_THROWS( dp::kwConstraint( X , 7 , 10 ) , contract_violation );
 CHECK_THROWS( dp::kwConstraint( X , -1 , 10 ) , contract_violation );
 CHECK_THROWS( dp::kwConstraint( X , 1 , -1 ) , contract_violation );
 CHECK_THROWS( dp::kwConstraint( { 0 } , 1 , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 54, 2018-06-28, Min palindrome ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// minPalRec(): DP[ i ][ j ], -1 if not solved yet
int minPalRec( const std::string & S , int i , int j ,
               std::vector< std::vector< int > > & DP )
{
 if( i >= j ) return( 0 );
 if( DP[ i ][ j ] < 0 )
  DP[ i ][ j ] = std::min( { minPalRec( S , i + 1 , j , DP ) + 1 ,
                             minPalRec( S , i , j - 1 , DP ) + 1 ,
                             minPalRec( S , i + 1 , j - 1 , DP ) +
                             ( S[ i ] == S[ j ] ? 0 : 1 ) } );
 return( DP[ i ][ j ] );
 }

/// the fewest removals and replacements of a character that make S a
/// palindrome: DP[ i ][ j ] = 0 if i >= j, else the least of DP[ i + 1 ][ j ]
/// + 1, DP[ i ][ j - 1 ] + 1 (a removal) and DP[ i + 1 ][ j - 1 ] plus 1 if
/// S_i != S_j (a replacement); by memoization, O( n^2 ) time and space
int minPalindrome( const std::string & S )
{
 const int n = int( S.size() );
 std::vector< std::vector< int > > DP( n , std::vector< int >( n , -1 ) );
 return( minPalRec( S , 0 , n - 1 , DP ) );
 }

}  // end( namespace dp )

// BAOBAB and ODORARONO take 1; against a BFS over the strings reached by
// removals and by replacements with the letters of S
ADS_TEST( "dp-54" )
{
 CHECK( dp::minPalindrome( "BAOBAB" ) == 1 );
 CHECK( dp::minPalindrome( "ODORARONO" ) == 1 );
 auto bfs = []( const std::string & S ) {
  const std::set< char > alpha( S.begin() , S.end() );
  std::map< std::string , int > d = { { S , 0 } };
  std::queue< std::string > Q;
  Q.push( S );
  while( ! Q.empty() ) {
   const auto s = Q.front();
   Q.pop();
   if( std::equal( s.begin() , s.end() , s.rbegin() ) ) return( d[ s ] );
   auto visit = [ & ]( const std::string & u ) {
    if( d.emplace( u , d[ s ] + 1 ).second ) Q.push( u );
    };
   for( std::size_t i = 0 ; i < s.size() ; ++i ) {
    visit( s.substr( 0 , i ) + s.substr( i + 1 ) );
    for( char c : alpha ) {
     auto u = s;
     u[ i ] = c;
     visit( u );
     }
    }
   }
  return( -1 );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto S = random_string( g() % 8 , 2 + g() % 2 , g );
  CHECK( dp::minPalindrome( S ) == bfs( S ) );
  }
 CHECK( dp::minPalindrome( "" ) == 0 );
 CHECK( dp::minPalindrome( "a" ) == 0 );
 CHECK( dp::minPalindrome( std::string( 1000 , 'z' ) ) == 0 );
 CHECK( dp::minPalindrome( "ab" ) == 1 );
 std::string L;
 for( int i = 0 ; i < 1000 ; ++i ) L += char( 'a' + i % 26 );
 CHECK( dp::minPalindrome( L ) <= 500 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 55, 2018-07-25, Profit --------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// profitRec(): DP[ i ][ k ], -1 if not solved yet
long long profitRec( const Ints & V , std::size_t i , std::size_t k ,
                     Table & DP )
{
 const long long sk = static_cast< long long >( k );
 if( i + 1 == V.size() ) return( V[ i ] * sk );
 if( DP[ i ][ k ] < 0 )
  DP[ i ][ k ] = std::max( { profitRec( V , i + 1 , k , DP ) ,
                             profitRec( V , i + 1 , k + 1 , DP ) - V[ i ] ,
                             profitRec( V , i + 1 , 0 , DP ) + sk * V[ i ] } );
 return( DP[ i ][ k ] );
 }

/// the largest gain from the prices V when each day one may buy a share,
/// sell all the shares held or do nothing: DP[ i ][ k ] is the gain from
/// day i on holding k shares, DP[ n ][ k ] = k V_n and otherwise the
/// largest of DP[ i + 1 ][ k ], DP[ i + 1 ][ k + 1 ] - V_i and
/// DP[ i + 1 ][ 0 ] + k V_i; by memoization, O( n^2 ) time and space; 0
/// with no day
/// \pre V > 0
long long profit( const Ints & V )
{
 const std::size_t n = V.size();
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( long long v ) { return( v > 0 ); } ) );
 if( n == 0 ) return( 0 );
 Table DP( n , Ints( n + 1 , -1 ) );
 return( profitRec( V , 0 , 0 , DP ) );
 }

}  // end( namespace dp )

// the examples of the text, 9 and 7; against the 3^n sequences of actions
// and against the scan that buys every day below the largest price to come
ADS_TEST( "dp-55" )
{
 CHECK( dp::profit( { 1 , 7 , 3 , 6 } ) == 9 );
 CHECK( dp::profit( { 7 , 1 , 2 , 5 , 3 , 1 } ) == 7 );
 auto scan = []( const Ints & V ) {
  long long best = 0 , gain = 0;
  for( std::size_t i = V.size() ; i-- > 0 ; ) {
   best = std::max( best , V[ i ] );
   gain += best - V[ i ];
   }
  return( gain );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto V = random_ints( g() % 9 , 1 , 10 , g );
  const std::size_t n = V.size();
  long long best = 0;
  std::size_t total = 1;
  for( std::size_t i = 0 ; i < n ; ++i ) total *= 3;
  for( std::size_t c = 0 ; c < total ; ++c ) {
   long long money = 0 , held = 0;
   std::size_t a = c;
   for( std::size_t i = 0 ; i < n ; ++i , a /= 3 )
    if( a % 3 == 1 ) {
     money -= V[ i ];
     ++held;
     }
    else if( a % 3 == 2 ) {
     money += held * V[ i ];
     held = 0;
     }
   best = std::max( best , money );
   }
  CHECK( dp::profit( V ) == best );
  CHECK( scan( V ) == best );
  }
 CHECK( dp::profit( {} ) == 0 );
 CHECK( dp::profit( { 5 } ) == 0 );
 CHECK( dp::profit( Ints( 50 , 5 ) ) == 0 );
 Ints up( 1000 );
 std::iota( up.begin() , up.end() , 1 );
 CHECK( dp::profit( up ) == scan( up ) && scan( up ) == 999LL * 1000 / 2 );
 Ints down( up.rbegin() , up.rend() );
 CHECK( dp::profit( down ) == 0 );
 CHECK( dp::profit( { 1 , 2 } ) == 1 && dp::profit( { 2 , 1 } ) == 0 );
 CHECK_THROWS( dp::profit( { 1 , 0 } ) , contract_violation );
 CHECK_THROWS( dp::profit( { -3 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 56, 2018-07-25, Value ---------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// value(): the number of the digits V[ s .. i ] (from 1), or cap + 1 if it
/// exceeds cap, so that long pieces cannot overflow
long long value( const std::string & V , int s , int i , long long cap )
{
 long long x = 0;
 for( int p = s ; p <= i ; ++p ) {
  x = 10 * x + ( V[ p - 1 ] - '0' );
  if( x > cap ) return( cap + 1 );
  }
 return( x );
 }

/// countSumRec(): DP[ i ][ r ], UNSOLVED if not solved yet
std::uint64_t countSumRec( const std::string & V , int i , long long r ,
                           UTable & DP )
{
 if( ( i == 0 ) && ( r == 0 ) ) return( 1 );
 if( ( r <= 0 ) || ( i == 0 ) ) return( 0 );
 if( DP[ i ][ r ] == UNSOLVED ) {
  DP[ i ][ r ] = 0;
  for( int s = 1 ; s <= i ; ++s )
   DP[ i ][ r ] += countSumRec( V , s - 1 , r - value( V , s , i , r ) ,
                                DP );
  }
 return( DP[ i ][ r ] );
 }

/// the ways of cutting the digits V into numbers that add up to k:
/// DP[ i ][ r ] counts those of the first i digits for r, 1 if i = r = 0,
/// 0 if r <= 0 < i or i = 0 < r, else sum_{s=1}^i DP[ s - 1 ][ r -
/// value( s , i ) ]; by memoization, O( n^2 k ) time with value() in O( n ).
/// The text returns 0 for i > 0 and r <= 0 and for i = 0 < r, but not for
/// i = 0 and r < 0, which a first piece larger than r reaches; the table
/// has n k places, k <= 100 n in the statement
/// \pre digits 1 ... 9, k >= 0, the count less than 2^64
std::uint64_t countSum( const std::string & V , long long k )
{
 ADS_PRE( k >= 0 );
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( char c ) { return( ( c >= '1' ) && ( c <= '9' ) );
                        } ) );
 const int n = int( V.size() );
 UTable DP( n + 1 , std::vector< std::uint64_t >( k + 1 , UNSOLVED ) );
 return( countSumRec( V , n , k , DP ) );
 }

}  // end( namespace dp )

// the example of the text, 29 ways for 141; against all the 2^( n - 1 )
// ways of cutting the digits
ADS_TEST( "dp-56" )
{
 CHECK( dp::countSum( "1151832175512" , 141 ) == 29 );
 CHECK( 11 + 5 + 1 + 83 + 21 + 7 + 5 + 5 + 1 + 2 == 141 );
 auto sums = []( const std::string & V ) {
  std::map< long long , std::uint64_t > c;
  const std::size_t n = V.size();
  for( unsigned m = 0 ; m < ( 1u << ( n - 1 ) ) ; ++m ) {
   long long s = 0 , cur = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    cur = 10 * cur + ( V[ i ] - '0' );
    if( ( i == n - 1 ) || ( m >> i & 1 ) ) {
     s += cur;
     cur = 0;
     }
    }
   ++c[ s ];
   }
  return( c );
  };
 CHECK( sums( "1151832175512" )[ 141 ] == 29 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  std::string V;
  for( std::size_t i = 1 + g() % 10 ; i > 0 ; --i )
   V += char( '1' + g() % 9 );
  auto c = sums( V );                // the sums within k <= 100 n
  const long long top = 100 * static_cast< long long >( V.size() );
  const std::size_t small = std::size_t( std::distance( c.begin() ,
                                                c.upper_bound( top ) ) );
  for( int q = 0 ; q < 5 ; ++q ) {
   long long k = std::next( c.begin() , g() % small )->first;
   if( q == 4 ) k = g() % 200;
   CHECK( dp::countSum( V , k ) == c[ k ] );
   }
  }
 CHECK( dp::countSum( "" , 0 ) == 1 );
 CHECK( dp::countSum( "" , 3 ) == 0 );
 CHECK( dp::countSum( "7" , 7 ) == 1 );
 CHECK( dp::countSum( "7" , 0 ) == 0 );
 CHECK( dp::countSum( "1111" , 4 ) == 1 );
 CHECK( dp::countSum( "1111" , 112 ) == 2 );
 const std::string many( 60 , '1' );                          // 2^59 cuts
 CHECK( dp::countSum( many , 60 ) == 1 );
 CHECK( dp::countSum( std::string( 30 , '9' ) , 270 ) > 0 );
 CHECK( dp::countSum( "12" , 12 ) == 1 && dp::countSum( "12" , 3 ) == 1 );
 CHECK( dp::countSum( "12" , 21 ) == 0 );
 CHECK_THROWS( dp::countSum( "105" , 6 ) , contract_violation );
 CHECK_THROWS( dp::countSum( "15" , -1 ) , contract_violation );
 CHECK_THROWS( dp::countSum( "1a" , 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 57, 2018-08-21, Più che doppio -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the longest subsequence of V in which each item is more than twice the
/// one before: the scheme of the longest increasing subsequence, DP[ i ] =
/// 1 + max{ DP[ j ] : j < i , V_i > 2 V_j } (1 if none), the answer the
/// largest DP[ i ]; Theta( n^2 ) time and O( n ) space. The test
/// V_i > 2 V_j is written V_i - V_j > V_j, which does not overflow
/// \pre V > 0
int moreThanDouble( const Ints & V )
{
 const std::size_t n = V.size();
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( long long v ) { return( v > 0 ); } ) );
 std::vector< int > DP( n , 1 );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < i ; ++j )
   if( V[ i ] - V[ j ] > V[ j ] ) DP[ i ] = std::max( DP[ i ] , DP[ j ] + 1 );
 return( n ? *std::max_element( DP.begin() , DP.end() ) : 0 );
 }

}  // end( namespace dp )

// the examples of the text, 6 and 3; against all the subsequences; the
// exact double, and items near the largest value, where 2 V overflows
ADS_TEST( "dp-57" )
{
 CHECK( dp::moreThanDouble( { 1 , 3 , 7 , 15 , 31 , 63 } ) == 6 );
 CHECK( dp::moreThanDouble( { 5 , 1 , 4 , 7 , 2 , 9 } ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto V = random_ints( g() % 14 , 1 , 60 , g );
  int best = 0;
  for( unsigned m = 0 ; m < ( 1u << V.size() ) ; ++m ) {
   const Ints s = mask_items( V , m );
   bool ok = true;
   for( std::size_t i = 1 ; i < s.size() ; ++i )
    ok = ok && ( s[ i ] > 2 * s[ i - 1 ] );
   if( ok ) best = std::max( best , int( s.size() ) );
   }
  CHECK( dp::moreThanDouble( V ) == best );
  }
 CHECK( dp::moreThanDouble( {} ) == 0 );
 CHECK( dp::moreThanDouble( { 4 } ) == 1 );
 CHECK( dp::moreThanDouble( Ints( 100 , 4 ) ) == 1 );
 CHECK( dp::moreThanDouble( { 1 , 2 , 4 , 8 } ) == 2 );     // 1 , 4
 Ints p;
 for( long long x = 1 ; x < ( 1LL << 61 ) ; x = 2 * x + 1 ) p.push_back( x );
 CHECK( dp::moreThanDouble( p ) == int( p.size() ) );
 const long long hi = std::numeric_limits< long long >::max();
 CHECK( dp::moreThanDouble( { ( hi >> 1 ) + 1 , hi } ) == 1 );  // 2 V > max
 CHECK( dp::moreThanDouble( { hi >> 1 , hi } ) == 2 );
 CHECK( dp::moreThanDouble( { hi , hi } ) == 1 );
 CHECK( dp::moreThanDouble( { 2 , 4 } ) == 1 && dp::moreThanDouble( { 2 , 5 } )
        == 2 );
 CHECK_THROWS( dp::moreThanDouble( { 0 } ) , contract_violation );
 CHECK_THROWS( dp::moreThanDouble( { 3 , -1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 58, 2019-02-07, Cross ---------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

using Matrix = std::vector< std::vector< int > >;

/// the number of 1 of the largest cross of 1 of the n x n binary matrix
/// M (a central 1 and four arms of equal length), 0 if there is no 1: the
/// matrices L, R, U, D of the lengths of the runs of 1 from each cell
/// (included) to the left, right, up and down, and the cross centred in
/// ( i , j ) has 4 min( L , R , U , D ) - 3 cells; Theta( n^2 ) time. The
/// code of the text sets only L[ i ][ 1 ], U[ 1 ][ j ], R[ i ][ n ],
/// D[ n ][ j ] and then fills the cells with i , j >= 2 (L and U) and
/// i , j <= n - 1 (R and D), leaving L of the first row, U of the first
/// column, R of the last row and D of the last column unset: a lone 1 at
/// the border, off the corners, gives 0 in place of 1
/// \pre M is square, of 0 and 1
int cross( const Matrix & M )
{
 const int n = int( M.size() );
 ADS_PRE( std::all_of( M.begin() , M.end() , [ n ]( const auto & r ) {
  return( int( r.size() ) == n && std::all_of( r.begin() , r.end() ,
                          []( int x ) { return( x == 0 || x == 1 ); } ) );
  } ) );
 Matrix L( n , std::vector< int >( n ) ) , R = L , U = L , D = L;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j ) {
   L[ i ][ j ] = M[ i ][ j ] ? ( j ? L[ i ][ j - 1 ] : 0 ) + 1 : 0;
   U[ i ][ j ] = M[ i ][ j ] ? ( i ? U[ i - 1 ][ j ] : 0 ) + 1 : 0;
   }
 for( int i = n ; i-- > 0 ; )
  for( int j = n ; j-- > 0 ; ) {
   R[ i ][ j ] = M[ i ][ j ] ? ( j < n - 1 ? R[ i ][ j + 1 ] : 0 ) + 1 : 0;
   D[ i ][ j ] = M[ i ][ j ] ? ( i < n - 1 ? D[ i + 1 ][ j ] : 0 ) + 1 : 0;
   }
 int maxsofar = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   maxsofar = std::max( maxsofar , 4 * std::min( { L[ i ][ j ] , R[ i ][ j ] ,
                                                   U[ i ][ j ] ,
                                                   D[ i ][ j ] } ) - 3 );
 return( maxsofar );
 }

/// the arm of the largest cross centred in ( i , j ), a 1
int largestCross( const Matrix & M , int i , int j )
{
 const int n = int( M.size() );
 int k = 1;
 while( ( i + k < n ) && ( j + k < n ) && ( i - k >= 0 ) && ( j - k >= 0 ) &&
        ( M[ i ][ j - k ] + M[ i ][ j + k ] + M[ i - k ][ j ] +
          M[ i + k ][ j ] == 4 ) )
  ++k;
 return( k - 1 );
 }

/// the same, by growing the arms from each 1, Theta( n^3 ) time; the text
/// keeps the largest arm k - 1, not the 4 ( k - 1 ) + 1 cells of its cross
int crossCubic( const Matrix & M )
{
 const int n = int( M.size() );
 int maxsofar = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( M[ i ][ j ] == 1 )
    maxsofar = std::max( maxsofar , 4 * largestCross( M , i , j ) + 1 );
 return( maxsofar );
 }

}  // end( namespace dp )

// a cross of 17 as in the figure, among other 1; the lone 1 at the border
// where the code of the text gives 0; both versions against a check of
// every centre and every arm on random matrices
ADS_TEST( "dp-58" )
{
 dp::Matrix F( 9 , std::vector< int >( 9 , 0 ) );
 for( int k = 0 ; k < 9 ; ++k ) F[ 4 ][ k ] = F[ k ][ 3 ] = 1;
 F[ 4 ][ 8 ] = 0;
 F[ 0 ][ 0 ] = F[ 1 ][ 0 ] = F[ 1 ][ 1 ] = F[ 7 ][ 6 ] = F[ 6 ][ 6 ] = 1;
 F[ 8 ][ 3 ] = 1;
 CHECK( dp::cross( F ) == 13 );                  // arm 3: F[ 4 ][ 8 ] is 0
 CHECK( dp::crossCubic( F ) == 13 );
 dp::Matrix G( 9 , std::vector< int >( 9 , 0 ) );
 for( int k = 0 ; k < 9 ; ++k ) G[ 4 ][ k ] = G[ k ][ 4 ] = 1;
 G[ 0 ][ 0 ] = G[ 8 ][ 7 ] = G[ 2 ][ 6 ] = 1;
 CHECK( dp::cross( G ) == 17 );
 CHECK( dp::crossCubic( G ) == 17 );
 dp::Matrix B( 3 , std::vector< int >( 3 , 0 ) );
 B[ 0 ][ 1 ] = 1;
 CHECK( dp::cross( B ) == 1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = int( g() % 9 );
  dp::Matrix M( n , std::vector< int >( n ) );
  for( auto & r : M )
   for( auto & x : r ) x = g() % 5 != 0;
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int a = 0 ; a < n ; ++a ) {
     bool ok = ( i - a >= 0 ) && ( i + a < n ) && ( j - a >= 0 ) &&
               ( j + a < n );
     for( int s = -a ; ok && ( s <= a ) ; ++s )
      ok = M[ i + s ][ j ] && M[ i ][ j + s ];
     if( ok ) best = std::max( best , 4 * a + 1 );
     }
  CHECK( dp::cross( M ) == best );
  CHECK( dp::crossCubic( M ) == best );
  }
 CHECK( dp::cross( {} ) == 0 );
 CHECK( dp::cross( { { 0 } } ) == 0 );
 CHECK( dp::cross( { { 1 } } ) == 1 );
 CHECK( dp::cross( dp::Matrix( 5 , std::vector< int >( 5 , 0 ) ) ) == 0 );
 CHECK( dp::cross( dp::Matrix( 301 , std::vector< int >( 301 , 1 ) ) ) ==
        601 );
 CHECK( dp::cross( dp::Matrix( 300 , std::vector< int >( 300 , 1 ) ) ) ==
        597 );
 CHECK( dp::cross( { { 1 , 1 } , { 1 , 1 } } ) == 1 );
 CHECK( dp::crossCubic( { { 1 , 1 } , { 1 , 1 } } ) == 1 );
 CHECK( dp::crossCubic( {} ) == 0 && dp::crossCubic( { { 0 } } ) == 0 );
 CHECK_THROWS( dp::cross( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( dp::cross( { { 2 } } ) , contract_violation );
 CHECK_THROWS( dp::cross( { { 1 , 0 } , { 0 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 59, 2019-02-07, Election -------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

enum Status { PE , PP , I };

/// the distances of the nodes from those of the party, by a BFS that
/// starts from all of them at distance 0
std::vector< int > dist( const ListGraph<> & G ,
                         const std::vector< Status > & status , Status party )
{
 std::vector< int > d( G.bound() , INF );
 std::queue< int > Q;
 for( int u : G.V() )
  if( status[ u ] == party ) {
   Q.push( u );
   d[ u ] = 0;
   }
 while( ! Q.empty() ) {
  const int u = Q.front();
  Q.pop();
  G.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    Q.push( v );
    }
   } );
  }
 return( d );
 }

/// the undecided nodes that stay so: the message of a party reaches a
/// node on the day of its distance from the party, so they are the nodes
/// at the same distance from both, found by two BFS from many sources;
/// O( n + m ) time
/// \pre G unoriented, a status per node
int election( const ListGraph<> & G , const std::vector< Status > & status )
{
 ADS_PRE( ! G.directed() );
 ADS_PRE( int( status.size() ) == G.bound() );
 const auto dPE = dist( G , status , PE ) , dPP = dist( G , status , PP );
 int tot = 0;
 for( int u : G.V() ) tot += dPE[ u ] == dPP[ u ];
 return( tot );
 }

}  // end( namespace dp )

// a graph with the days of the table of the text, where C, G and H stay
// undecided; against the simulation day by day on random connected graphs
ADS_TEST( "dp-59" )
{
 using dp::PE , dp::PP , dp::I;
 enum { A , B , C , D , E , F , G , H , II , J };
 ListGraph<> X( 10 , false );
 for( auto [ u , v ] : std::vector< std::pair< int , int > >( {
       { A , B } , { A , F } , { B , C } , { C , D } , { C , G } , { C , H } ,
       { D , E } , { D , II } , { E , J } } ) )
  X.insertEdge( u , v );
 const std::vector< dp::Status > st = { PE , PE , I , PP , PP ,
                                        I , I , I , I , I };
 CHECK( dp::dist( X , st , PE ) ==
        std::vector< int >( { 0 , 0 , 1 , 2 , 3 , 1 , 2 , 2 , 3 , 4 } ) );
 CHECK( dp::dist( X , st , PP ) ==
        std::vector< int >( { 3 , 2 , 1 , 0 , 0 , 4 , 2 , 2 , 1 , 1 } ) );
 CHECK( dp::election( X , st ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 12 );
  ListGraph<> Y( n , false );
  for( int u = 1 ; u < n ; ++u )
   Y.insertEdge( u , int( g() % unsigned( u ) ) );
  for( int e = int( g() % unsigned( n ) ) ; e > 0 ; --e ) {
   const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
   if( u != v ) Y.insertEdge( u , v );
   }
  std::vector< dp::Status > s( n );
  for( auto & x : s ) x = dp::Status( g() % 3 );
  // the day each node first receives each message, by simulation
  auto spread = [ & ]( dp::Status party ) {
   std::vector< int > day( n , INF );
   for( int u = 0 ; u < n ; ++u ) if( s[ u ] == party ) day[ u ] = 0;
   for( int d = 0 ; d < n ; ++d )
    for( int u = 0 ; u < n ; ++u )
     if( day[ u ] == d )
      for( int v : Y.adj( u ) ) if( day[ v ] == INF ) day[ v ] = d + 1;
   return( day );
   };
  const auto te = spread( PE ) , tp = spread( PP );
  int undecided = 0;
  for( int u = 0 ; u < n ; ++u )
   undecided += ( s[ u ] == I ) && ( te[ u ] == tp[ u ] );
  CHECK( dp::election( Y , s ) == undecided );
  }
 ListGraph<> one( 1 , false );
 CHECK( dp::election( one , { I } ) == 1 );
 CHECK( dp::election( one , { PE } ) == 0 );
 ListGraph<> path( 1001 , false );
 for( int u = 1 ; u <= 1000 ; ++u ) path.insertEdge( u - 1 , u );
 std::vector< dp::Status > s( 1001 , I );
 s[ 0 ] = PE;
 s[ 1000 ] = PP;
 CHECK( dp::election( path , s ) == 1 );                      // the middle
 s[ 1000 ] = I;
 CHECK( dp::election( path , s ) == 0 );
 ListGraph<> none( 3 , false );                // no edges: no message moves
 CHECK( dp::election( none , { PE , I , I } ) == 2 );
 CHECK( dp::election( none , { PE , PP , PE } ) == 0 );
 CHECK_THROWS( dp::election( ListGraph<>( 2 , true ) , { I , I } ) ,
               contract_violation );
 CHECK_THROWS( dp::election( none , { I , I } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 60, 2019-06-06, Count paths ------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// countPathsRec(): DP[ r ][ c ][ b ], UNSOLVED if not solved yet
std::uint64_t countPathsRec( const Matrix & P , std::vector< UTable > & DP ,
                             int r , int c , int b )
{
 if( ( b < 0 ) || ( r == 0 ) || ( c == 0 ) ) return( 0 );
 if( ( r == 1 ) && ( c == 1 ) ) return( 1 );
 auto & d = DP[ r ][ c ][ b ];
 if( d == UNSOLVED ) {
  const int p = P[ r - 1 ][ c - 1 ];
  d = countPathsRec( P , DP , r - 1 , c , b - p ) +
      countPathsRec( P , DP , r , c - 1 , b - p );
  }
 return( d );
 }

/// the paths down and right from ( 1 , 1 ) to ( n , n ) whose cells cost
/// at most B in all: DP[ r ][ c ][ b ] counts those to ( r , c ) within b,
/// 0 if b < 0 or out of the board, 1 at ( 1 , 1 ), else the sum of those
/// to ( r - 1 , c ) and ( r , c - 1 ) within b - P[ r ][ c ]; the budget
/// is cut at 10 ( 2 n - 2 ), beyond which every path counts, so that the
/// memoization takes O( n^3 ) time and space, polynomial
/// \pre P n x n of costs in [ 0 , 10 ], P[ 1 ][ 1 ] = 0, n >= 1
std::uint64_t countPaths( const Matrix & P , long long B )
{
 const int n = int( P.size() );
 ADS_PRE( ( n >= 1 ) && ( P[ 0 ][ 0 ] == 0 ) );
 ADS_PRE( std::all_of( P.begin() , P.end() , [ n ]( const auto & r ) {
  return( int( r.size() ) == n && std::all_of( r.begin() , r.end() ,
                          []( int x ) { return( x >= 0 && x <= 10 ); } ) );
  } ) );
 if( B < 0 ) return( 0 );
 const int size = int( std::min< long long >( 10 * ( 2 * n - 2 ) , B ) );
 std::vector< UTable > DP( n + 1 , UTable( n + 1 ,
                           std::vector< std::uint64_t >( size + 1 ,
                                                         UNSOLVED ) ) );
 return( countPathsRec( P , DP , n , n , size ) );
 }

}  // end( namespace dp )

// the example of the text, 4 paths within 4; against all the C( 2 n - 2 ,
// n - 1 ) paths of random boards
ADS_TEST( "dp-60" )
{
 const dp::Matrix P = { { 0 , 1 , 2 } , { 1 , 1 , 1 } , { 2 , 1 , 1 } };
 CHECK( dp::countPaths( P , 4 ) == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 6 );
  dp::Matrix Q( n , std::vector< int >( n ) );
  for( auto & r : Q )
   for( auto & x : r ) x = int( g() % 11 );
  Q[ 0 ][ 0 ] = 0;
  std::vector< long long > costs;
  std::function< void( int , int , long long ) > walk =
   [ & ]( int r , int c , long long s ) {
    s += Q[ r ][ c ];
    if( ( r == n - 1 ) && ( c == n - 1 ) ) costs.push_back( s );
    if( r < n - 1 ) walk( r + 1 , c , s );
    if( c < n - 1 ) walk( r , c + 1 , s );
    };
  walk( 0 , 0 , 0 );
  for( int q = 0 ; q < 4 ; ++q ) {
   const long long B = q == 3 ? 1000 :
                       static_cast< long long >( g() % ( 12 * n ) ) - 2;
   CHECK( dp::countPaths( Q , B ) == std::uint64_t(
          std::count_if( costs.begin() , costs.end() ,
                         [ B ]( long long x ) { return( x <= B ); } ) ) );
   }
  }
 CHECK( dp::countPaths( { { 0 } } , 0 ) == 1 );
 CHECK( dp::countPaths( { { 0 } } , -1 ) == 0 );
 const dp::Matrix Z( 30 , std::vector< int >( 30 , 0 ) ) ,
                  T( 30 , std::vector< int >( 30 , 10 ) );
 CHECK( dp::countPaths( Z , 0 ) == tartaglia( 58 )[ 58 ][ 29 ] );
 auto T0 = T;
 T0[ 0 ][ 0 ] = 0;
 CHECK( dp::countPaths( T0 , 580 ) == tartaglia( 58 )[ 58 ][ 29 ] );
 CHECK( dp::countPaths( T0 , 579 ) == 0 );
 CHECK( dp::countPaths( T0 , 1000000 ) == tartaglia( 58 )[ 58 ][ 29 ] );
 CHECK( dp::countPaths( { { 0 , 10 } , { 10 , 0 } } , 9 ) == 0 );
 CHECK( dp::countPaths( { { 0 , 10 } , { 10 , 0 } } , 10 ) == 2 );
 CHECK( dp::countPaths( { { 0 } } , std::numeric_limits< long long >::max() )
        == 1 );
 CHECK_THROWS( dp::countPaths( T , 5 ) , contract_violation );
 CHECK_THROWS( dp::countPaths( {} , 5 ) , contract_violation );
 CHECK_THROWS( dp::countPaths( { { 0 , 1 } } , 5 ) , contract_violation );
 CHECK_THROWS( dp::countPaths( { { 0 , 11 } , { 1 , 1 } } , 5 ) ,
               contract_violation );
 CHECK_THROWS( dp::countPaths( { { 0 , -1 } , { 1 , 1 } } , 5 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 61, 2019-06-17, K palindrome -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// kpalRec(): the fewest removals that make S[ i .. j ] a palindrome, -1
/// if not solved yet
int kpalRec( const std::string & S , int i , int j ,
             std::vector< std::vector< int > > & DP )
{
 if( j <= i ) return( 0 );
 if( DP[ i ][ j ] < 0 )
  DP[ i ][ j ] = S[ i ] == S[ j ] ? kpalRec( S , i + 1 , j - 1 , DP ) :
                 std::min( kpalRec( S , i + 1 , j , DP ) ,
                           kpalRec( S , i , j - 1 , DP ) ) + 1;
 return( DP[ i ][ j ] );
 }

/// can S become a palindrome by removing at most k characters? DP[ i ][ j ]
/// is the fewest removals for S[ i .. j ]: 0 if j <= i, DP[ i + 1 ][ j - 1 ]
/// if S_i = S_j, else min( DP[ i + 1 ][ j ] , DP[ i ][ j - 1 ] ) + 1; by
/// memoization, Theta( n^2 ) time and space; it is n - | LPS( S ) |
/// \pre k >= 0
bool kPalindrome( const std::string & S , int k )
{
 ADS_PRE( k >= 0 );
 const int n = int( S.size() );
 std::vector< std::vector< int > > DP( n , std::vector< int >( n , -1 ) );
 return( kpalRec( S , 0 , n - 1 , DP ) <= k );
 }

/// kpalRec3(): is S[ i .. j ] l-palindrome? -1 if not solved yet
bool kpalRec3( const std::string & S , int i , int j , int l ,
               std::vector< Table > & DP )
{
 if( l < 0 ) return( false );
 if( j <= i ) return( true );
 auto & d = DP[ i ][ j ][ l ];
 if( d < 0 )
  d = ( ( S[ i ] == S[ j ] ) && kpalRec3( S , i + 1 , j - 1 , l , DP ) ) ||
      kpalRec3( S , i + 1 , j , l - 1 , DP ) ||
      kpalRec3( S , i , j - 1 , l - 1 , DP );
 return( d == 1 );
 }

/// the same, by the table DP[ i ][ j ][ l ] of the booleans "S[ i .. j ] is
/// l-palindrome", with k cut at n; O( k n^2 ) time. The recursion of the
/// text writes DP[ i ][ j + 1 ][ k - 1 ] for DP[ i ][ j - 1 ][ k - 1 ] (the
/// code is right), and its prose swaps the codes 0 and 1 of the booleans
/// \pre k >= 0
bool kPalindromeCubic( const std::string & S , int k )
{
 ADS_PRE( k >= 0 );
 const int n = int( S.size() );
 k = std::min( n , k );
 std::vector< Table > DP( n , Table( n , Ints( k + 1 , -1 ) ) );
 return( kpalRec3( S , 0 , n - 1 , k , DP ) );
 }

}  // end( namespace dp )

// the example of the text, 4-palindrome and not 3-palindrome; both versions
// against n - | lcs( S , reverse( S ) ) | of the library and, on short
// strings, against all the subsets of the characters removed
ADS_TEST( "dp-61" )
{
 const std::string E = "DISillusaSfingosognifasulli";
 CHECK( dp::kPalindrome( E , 4 ) && ! dp::kPalindrome( E , 3 ) );
 CHECK( dp::kPalindromeCubic( E , 4 ) && ! dp::kPalindromeCubic( E , 3 ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const auto S = random_string( g() % 30 , 2 + g() % 3 , g );
  const std::string R( S.rbegin() , S.rend() );
  const int need = int( S.size() - lcs( S , R ).size() );
  for( int k = 0 ; k <= int( S.size() ) + 1 ; ++k ) {
   CHECK( dp::kPalindrome( S , k ) == ( need <= k ) );
   if( S.size() <= 12 )
    CHECK( dp::kPalindromeCubic( S , k ) == ( need <= k ) );
   }
  }
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto S = random_string( g() % 11 , 3 , g );
  int need = int( S.size() );
  for( unsigned m = 0 ; m < ( 1u << S.size() ) ; ++m ) {
   std::string s;
   for( std::size_t i = 0 ; i < S.size() ; ++i ) if( m >> i & 1 ) s += S[ i ];
   if( std::equal( s.begin() , s.end() , s.rbegin() ) )
    need = std::min( need , int( S.size() - s.size() ) );
   }
  CHECK( dp::kPalindrome( S , need ) );
  CHECK( need == 0 || ! dp::kPalindrome( S , need - 1 ) );
  }
 CHECK( dp::kPalindrome( "" , 0 ) && dp::kPalindromeCubic( "" , 0 ) );
 CHECK( dp::kPalindrome( "a" , 0 ) );
 CHECK( dp::kPalindrome( std::string( 2000 , 'q' ) , 0 ) );
 CHECK( dp::kPalindrome( "abcdef" , 5 ) && ! dp::kPalindrome( "abcdef" , 4 ) );
 CHECK( dp::kPalindromeCubic( "abcdef" , 1000 ) );
 CHECK( dp::kPalindromeCubic( "ab" , 1 ) );
 CHECK( ! dp::kPalindromeCubic( "ab" , 0 ) );
 CHECK( dp::kPalindrome( "ab" , 1000 ) );
 CHECK_THROWS( dp::kPalindrome( "ab" , -1 ) , contract_violation );
 CHECK_THROWS( dp::kPalindromeCubic( "ab" , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 62, 2019-07-18, Max Increasing ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the largest sum of a strictly increasing subsequence of A: DP[ i ] =
/// A_i + max{ DP[ j ] : j < i , A_j < A_i } (A_i if none), the answer the
/// largest DP[ i ]; Theta( n^2 ) time and O( n ) space, O( n log n ) by
/// maxSumIncreasingSubsequence() of the library
/// \pre A > 0
long long maxIncreasing( const Ints & A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( long long a ) { return( a > 0 ); } ) );
 return( maxSumIncreasingSubsequence( A ) );
 }

}  // end( namespace dp )

// the example of the text, 35 by 1, 8, 12, 14; against all the
// subsequences
ADS_TEST( "dp-62" )
{
 const Ints A = { 1 , 8 , 4 , 12 , 2 , 10 , 6 , 14 , 1 , 9 , 5 , 13 , 3 ,
                  11 };
 CHECK( dp::maxIncreasing( A ) == 35 );
 CHECK( 1 + 4 + 6 + 9 + 13 == 33 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto V = random_ints( g() % 14 , 1 , 20 , g );
  long long best = 0;
  for( unsigned m = 0 ; m < ( 1u << V.size() ) ; ++m ) {
   const Ints s = mask_items( V , m );
   if( std::adjacent_find( s.begin() , s.end() ,
                           std::greater_equal<>() ) == s.end() )
    best = std::max( best , mask_sum( V , m ) );
   }
  CHECK( dp::maxIncreasing( V ) == best );
  }
 CHECK( dp::maxIncreasing( {} ) == 0 );
 CHECK( dp::maxIncreasing( { 9 } ) == 9 );
 CHECK( dp::maxIncreasing( Ints( 50 , 9 ) ) == 9 );           // not [ 9 , 9 ]
 Ints up( 2000 );
 std::iota( up.begin() , up.end() , 1 );
 CHECK( dp::maxIncreasing( up ) == 2000LL * 2001 / 2 );
 Ints down( up.rbegin() , up.rend() );
 CHECK( dp::maxIncreasing( down ) == 2000 );
 CHECK( dp::maxIncreasing( { 3 , 1 , 2 } ) == 3 );
 CHECK_THROWS( dp::maxIncreasing( { 1 , -1 } ) , contract_violation );
 CHECK_THROWS( dp::maxIncreasing( { 0 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 63, 2019-08-22, Balanced -------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the 1-balanced binary trees of height exactly h (the shapes of the AVL
/// trees): DP[ 0 ] = 1, DP[ 1 ] = 3 and DP[ h ] = 2 DP[ h - 1 ] DP[ h - 2 ] +
/// DP[ h - 1 ]^2, a subtree of height h - 1 and the other of height h - 1
/// or h - 2; Theta( h ) operations on numbers that grow doubly
/// exponentially, hence BigInteger. The loop of the text writes DP[ h ] in
/// place of DP[ i ]
/// \pre h >= 0
BigInteger balanced( int h )
{
 ADS_PRE( h >= 0 );
 std::vector< BigInteger > DP( std::max( h + 1 , 2 ) );
 DP[ 0 ] = 1;
 DP[ 1 ] = 3;
 for( int i = 2 ; i <= h ; ++i )
  DP[ i ] = BigInteger( 2 ) * DP[ i - 1 ] * DP[ i - 2 ] +
            DP[ i - 1 ] * DP[ i - 1 ];
 return( DP[ h ] );
 }

}  // end( namespace dp )

/// the height of the shape written as "." (empty) or "(" left right ")"
/// from s[ p ], -1 if it is not 1-balanced; p moves past it
int balanced_height( const std::string & s , std::size_t & p )
{
 if( s[ p++ ] == '.' ) return( -1 );
 const int l = balanced_height( s , p ) , r = balanced_height( s , p );
 ++p;
 if( ( l < -1 ) || ( r < -1 ) || ( std::abs( l - r ) > 1 ) ) return( -2 );
 return( 1 + std::max( l , r ) );
 }

// against all the shapes of height at most 4, 458330 strings, those
// 1-balanced of height h being 1, 3, 15, 315, 108675; the next values
ADS_TEST( "dp-63" )
{
 std::vector< std::string > S = { "." };            // shapes of height <= h
 for( int h = 0 ; h <= 4 ; ++h ) {
  std::vector< std::string > N = { "." };
  for( const auto & l : S )
   for( const auto & r : S ) N.push_back( "(" + l + r + ")" );
  S = std::move( N );
  long long c = 0;
  for( const auto & s : S ) {
   std::size_t p = 0;
   c += balanced_height( s , p ) == h;
   }
  CHECK( dp::balanced( h ) == BigInteger( c ) );
  }
 CHECK( S.size() == 458330 );
 CHECK( dp::balanced( 3 ) == BigInteger( 315 ) );
 CHECK( dp::balanced( 5 ) == BigInteger( 11878720875LL ) );
 CHECK( dp::balanced( 6 ) == BigInteger( "141106591466142946875" ) );
 CHECK( dp::balanced( 7 ) == BigInteger(
                "19911070158545297149037891328865229296875" ) );
 CHECK( dp::balanced( 20 ).toString().size() > 10000 );
 CHECK_THROWS( dp::balanced( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 64, 2019-08-22, Skipass --------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the fewest ski-passes for the skiers of the times [ start_i , end_i ),
/// one pass used by one skier at a time: the colors of intervalColoring()
/// of the library (exercise 14.6, the minSkipass() of the text), the
/// largest number of skiers at the same time; O( n log n ) time
/// \pre start_i < end_i
int minSkipass( const std::vector< std::pair< long , long > > & I )
{
 const auto c = intervalColoring( I );
 return( c.empty() ? 0 : 1 + *std::max_element( c.begin() , c.end() ) );
 }

}  // end( namespace dp )

// the example of the text, two passes; against the largest number of
// intervals over a time, at each start; touching and extreme endpoints
ADS_TEST( "dp-64" )
{
 CHECK( dp::minSkipass( { { 0 , 60 } , { 60 , 120 } , { 0 , 120 } } ) == 2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::vector< std::pair< long , long > > I( g() % 15 );
  for( auto & [ a , b ] : I ) {
   a = long( g() % 30 );
   b = a + 1 + long( g() % 10 );
   }
  int depth = 0;
  for( const auto & [ s , e ] : I ) {
   int d = 0;
   for( const auto & [ a , b ] : I ) d += ( a <= s ) && ( s < b );
   depth = std::max( depth , d );
   }
  CHECK( dp::minSkipass( I ) == depth );
  }
 CHECK( dp::minSkipass( {} ) == 0 );
 CHECK( dp::minSkipass( { { 3 , 4 } } ) == 1 );
 CHECK( dp::minSkipass( std::vector< std::pair< long , long > >(
                                              1000 , { 0 , 600 } ) ) == 1000 );
 std::vector< std::pair< long , long > > chain;
 for( long i = 0 ; i < 1000 ; ++i ) chain.emplace_back( i , i + 1 );
 CHECK( dp::minSkipass( chain ) == 1 );
 const long lo = std::numeric_limits< long >::min() ,
            hi = std::numeric_limits< long >::max();
 CHECK( dp::minSkipass( { { lo , hi } , { 0 , 1 } , { lo , 0 } } ) == 2 );
 CHECK( dp::minSkipass( { { 0 , 2 } , { 1 , 3 } , { 2 , 4 } } ) == 2 );
 CHECK_THROWS( dp::minSkipass( { { 4 , 4 } } ) , contract_violation );
 CHECK_THROWS( dp::minSkipass( { { 0 , 1 } , { 5 , 4 } } ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 65, 2020-01-14, Max sum even -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the largest sum of a contiguous subarray of A of even length (0 for the
/// empty one): DP[ i ] is the best of those ending at i, DP[ 0 ] = DP[ 1 ] =
/// 0 and DP[ i ] = max( DP[ i - 2 ] + A_{i-1} + A_i , 0 ); the answer the
/// largest DP[ i ]; Theta( n ) time, the scheme of Kadane by pairs
long long maxSumEven( const Ints & A )
{
 const std::size_t n = A.size();
 Ints DP( n + 1 , 0 );
 long long best = 0;
 for( std::size_t i = 2 ; i <= n ; ++i ) {
  DP[ i ] = std::max( DP[ i - 2 ] + A[ i - 2 ] + A[ i - 1 ] , 0LL );
  best = std::max( best , DP[ i ] );
  }
 return( best );
 }

}  // end( namespace dp )

// the examples of the text, 1, 20 and 0; against all the subarrays
ADS_TEST( "dp-65" )
{
 CHECK( dp::maxSumEven( { 1 , -2 , 3 } ) == 1 );
 CHECK( dp::maxSumEven( { 8 , 9 , -8 , 9 , 10 } ) == 20 );
 CHECK( dp::maxSumEven( { -2 , 1 , -2 , 1 , -2 } ) == 0 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const auto A = random_ints( g() % 20 , -10 , 10 , g );
  long long best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i ) {
   long long s = 0;
   for( std::size_t j = i ; j < A.size() ; ++j ) {
    s += A[ j ];
    if( ( j - i ) % 2 == 1 ) best = std::max( best , s );
    }
   }
  CHECK( dp::maxSumEven( A ) == best );
  }
 CHECK( dp::maxSumEven( {} ) == 0 );
 CHECK( dp::maxSumEven( { 100 } ) == 0 );
 CHECK( dp::maxSumEven( Ints( 101 , 5 ) ) == 500 );
 CHECK( dp::maxSumEven( Ints( 100 , -5 ) ) == 0 );
 CHECK( dp::maxSumEven( { 3 , -1 } ) == 2 );
 CHECK( dp::maxSumEven( { -3 , 1 } ) == 0 );
 CHECK( dp::maxSumEven( { 5 , -100 , 5 } ) == 0 );          // no odd runs
 CHECK( dp::maxSumEven( { -1 , 10 , 10 , -1 } ) == 20 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 66, 2020-02-04, Count paths ------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// countJumpsRec(): the simple paths from ( i , j ) to the last cell
long long countJumpsRec( const Matrix & A , Matrix & visited , int i ,
                         int j )
{
 const int n = int( A.size() );
 if( ( i == n - 1 ) && ( j == n - 1 ) ) return( 1 );
 if( ( i < 0 ) || ( i >= n ) || ( j < 0 ) || ( j >= n ) || visited[ i ][ j ] )
  return( 0 );
 visited[ i ][ j ] = 1;
 const int a = A[ i ][ j ];
 const long long ret = countJumpsRec( A , visited , i + a , j ) +
                       countJumpsRec( A , visited , i , j + a ) +
                       countJumpsRec( A , visited , i - a , j ) +
                       countJumpsRec( A , visited , i , j - a );
 visited[ i ][ j ] = 0;
 return( ret );
 }

/// the simple paths from ( 0 , 0 ) to ( n - 1 , n - 1 ) that jump from
/// ( i , j ) by A[ i ][ j ] up, down, left or right (the countPaths() of
/// the text): by backtrack with a matrix of the visited cells, since the
/// cells visited break the optimal substructure; O( 4^{n^2} ) time
/// \pre A n x n of positive integers
long long countJumps( const Matrix & A )
{
 const int n = int( A.size() );
 ADS_PRE( std::all_of( A.begin() , A.end() , [ n ]( const auto & r ) {
  return( int( r.size() ) == n && std::all_of( r.begin() , r.end() ,
                                       []( int x ) { return( x > 0 ); } ) );
  } ) );
 if( n == 0 ) return( 0 );
 Matrix visited( n , std::vector< int >( n , 0 ) );
 return( countJumpsRec( A , visited , 0 , 0 ) );
 }

}  // end( namespace dp )

// the example of the text, 5 paths; against a DFS that collects the paths
// as sequences of cells and counts the distinct ones
ADS_TEST( "dp-66" )
{
 const dp::Matrix A = { { 1 , 2 , 4 , 3 } , { 1 , 2 , 3 , 2 } ,
                        { 1 , 2 , 3 , 2 } , { 1 , 4 , 4 , 5 } };
 CHECK( dp::countJumps( A ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 4 );
  dp::Matrix M( n , std::vector< int >( n ) );
  for( auto & r : M )
   for( auto & x : r ) x = 1 + int( g() % 3 );
  std::set< std::vector< int > > paths;
  std::vector< int > cur;
  std::function< void( int , int ) > dfs = [ & ]( int i , int j ) {
   if( ( i < 0 ) || ( j < 0 ) || ( i >= n ) || ( j >= n ) ) return;
   const int c = i * n + j;
   if( std::find( cur.begin() , cur.end() , c ) != cur.end() ) return;
   cur.push_back( c );
   if( c == n * n - 1 ) paths.insert( cur );
   else {
    const int a = M[ i ][ j ];
    dfs( i + a , j );
    dfs( i - a , j );
    dfs( i , j + a );
    dfs( i , j - a );
    }
   cur.pop_back();
   };
  dfs( 0 , 0 );
  CHECK( dp::countJumps( M ) == static_cast< long long >( paths.size() ) );
  }
 CHECK( dp::countJumps( {} ) == 0 );
 CHECK( dp::countJumps( { { 7 } } ) == 1 );
 CHECK( dp::countJumps( { { 5 , 5 } , { 5 , 5 } } ) == 0 );
 CHECK( dp::countJumps( { { 1 , 1 } , { 1 , 1 } } ) == 2 );
 CHECK( dp::countJumps( dp::Matrix( 5 , std::vector< int >( 5 , 1 ) ) ) ==
        8512 );         // self-avoiding walks across a 5 x 5 grid (A007764)
 CHECK( dp::countJumps( { { 1 , 9 } , { 9 , 9 } } ) == 0 );    // jumps out
 CHECK( dp::countJumps( { { 2 , 1 , 1 } , { 1 , 1 , 1 } , { 1 , 1 , 1 } } )
        > 0 );
 CHECK_THROWS( dp::countJumps( { { 0 } } ) , contract_violation );
 CHECK_THROWS( dp::countJumps( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( dp::countJumps( { { 1 , 1 } , { 1 , -1 } } ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 67, 2020-07-03, Ksequence -------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the largest sum of a subsequence of A whose neighbours differ by at
/// most k: DP[ i ] = A_i + max{ DP[ j ] : j < i , | A_i - A_j | <= k } (A_i
/// if none), the answer the largest DP[ i ]; O( n^2 ) time
/// \pre A > 0, k >= 0
long long ksequence( const Ints & A , long long k )
{
 const std::size_t n = A.size();
 ADS_PRE( k >= 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( long long a ) { return( a > 0 ); } ) );
 Ints DP( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  DP[ i ] = A[ i ];
  for( std::size_t j = 0 ; j < i ; ++j )
   if( std::abs( A[ j ] - A[ i ] ) <= k )
    DP[ i ] = std::max( DP[ i ] , DP[ j ] + A[ i ] );
  }
 return( n ? *std::max_element( DP.begin() , DP.end() ) : 0 );
 }

}  // end( namespace dp )

// the examples of the text, 11, 14 and 18; against all the subsequences
ADS_TEST( "dp-67" )
{
 const Ints A = { 4 , 6 , 3 , 1 , 4 };
 CHECK( dp::ksequence( A , 1 ) == 11 );
 CHECK( dp::ksequence( A , 2 ) == 14 );
 CHECK( dp::ksequence( A , 3 ) == 18 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto V = random_ints( g() % 14 , 1 , 12 , g );
  const long long k = g() % 6;
  long long best = 0;
  for( unsigned m = 0 ; m < ( 1u << V.size() ) ; ++m ) {
   const Ints s = mask_items( V , m );
   bool ok = true;
   for( std::size_t i = 1 ; i < s.size() ; ++i )
    ok = ok && ( std::abs( s[ i ] - s[ i - 1 ] ) <= k );
   if( ok ) best = std::max( best , mask_sum( V , m ) );
   }
  CHECK( dp::ksequence( V , k ) == best );
  }
 CHECK( dp::ksequence( {} , 3 ) == 0 );
 CHECK( dp::ksequence( { 8 } , 0 ) == 8 );
 CHECK( dp::ksequence( Ints( 2000 , 8 ) , 0 ) == 16000 );
 CHECK( dp::ksequence( { 1 , 100 , 2 } , 0 ) == 100 );
 CHECK( dp::ksequence( { 1 , 100 , 2 } ,
                       std::numeric_limits< long long >::max() ) == 103 );
 CHECK( dp::ksequence( { 5 , 3 , 5 } , 1 ) == 10 );
 CHECK_THROWS( dp::ksequence( { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::ksequence( { 1 , 0 } , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 68, 2020-07-24, Close duplicates ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// are there i < j with j - i <= k and A_i = A_j? A window of the last k
/// values in a hash set: each value is looked for there, then enters it,
/// and the one k + 1 places back leaves it; Theta( n ) expected time
/// \pre k >= 0
bool closeDuplicates( const Ints & A , std::size_t k )
{
 std::unordered_set< long long > S;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  if( i > k ) S.erase( A[ i - k - 1 ] );
  if( ! S.insert( A[ i ] ).second ) return( true );
  }
 return( false );
 }

}  // end( namespace dp )

// the examples of the text; against the two loops in O( n k )
ADS_TEST( "dp-68" )
{
 CHECK( dp::closeDuplicates( { 1 , 2 , 3 , 5 , 3 , 7 , 1 } , 2 ) );
 CHECK( ! dp::closeDuplicates( { 1 , 2 , 3 , 5 , 7 , 3 , 1 } , 2 ) );
 for( std::size_t k = 0 ; k < 6 ; ++k )
  CHECK( ! dp::closeDuplicates( { 1 , 2 , 3 , 4 , 5 , 6 } , k ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const auto A = random_ints( g() % 20 , 0 , 1 + g() % 15 , g );
  const std::size_t k = g() % 8;
  bool dup = false;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; ( j < A.size() ) && ( j - i <= k ) ; ++j )
    dup = dup || ( A[ i ] == A[ j ] );
  CHECK( dp::closeDuplicates( A , k ) == dup );
  }
 CHECK( ! dp::closeDuplicates( {} , 3 ) );
 CHECK( ! dp::closeDuplicates( { 4 } , 0 ) );
 CHECK( ! dp::closeDuplicates( Ints( 100 , 4 ) , 0 ) );
 CHECK( dp::closeDuplicates( Ints( 100 , 4 ) , 1 ) );
 Ints big( 100000 );
 std::iota( big.begin() , big.end() , 0 );
 big.push_back( 0 );
 CHECK( dp::closeDuplicates( big , 100000 ) );
 CHECK( ! dp::closeDuplicates( big , 99999 ) );
 const std::size_t inf = std::numeric_limits< std::size_t >::max();
 CHECK( dp::closeDuplicates( big , inf ) );
 CHECK( ! dp::closeDuplicates( { 1 , 2 , 3 } , inf ) );
 const long long lo = std::numeric_limits< long long >::min();
 CHECK( dp::closeDuplicates( { lo , 0 , lo } , 2 ) );
 CHECK( ! dp::closeDuplicates( { lo , 0 , lo } , 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 69, 2020-09-04, Count menu ------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the subsets of the dishes A (equal costs being distinct dishes) that
/// cost k in all: DP[ i ][ r ] counts those of the first i dishes for r, 1
/// if r = 0, 0 if r > 0 = i, else DP[ i - 1 ][ r ] plus DP[ i - 1 ][ r -
/// A_i ] if A_i <= r; O( n k ) time, pseudopolynomial, subsetSumCount() of
/// the library
/// \pre A > 0, k >= 0, the count less than 2^64
std::uint64_t countMenu( const Ints & A , long long k )
{
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( long long a ) { return( a > 0 ); } ) );
 return( subsetSumCount( A , k ) );
 }

}  // end( namespace dp )

// the example of the text, 4 menus of 8; against all the subsets
ADS_TEST( "dp-69" )
{
 CHECK( dp::countMenu( { 1 , 3 , 5 , 7 , 8 , 4 } , 8 ) == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto A = random_ints( g() % 14 , 1 , 10 , g );
  const long long k = g() % 40;
  std::uint64_t c = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m )
   c += mask_sum( A , m ) == k;
  CHECK( dp::countMenu( A , k ) == c );
  }
 const auto C = tartaglia( 60 );
 CHECK( dp::countMenu( {} , 0 ) == 1 );
 CHECK( dp::countMenu( {} , 5 ) == 0 );
 CHECK( dp::countMenu( { 5 } , 5 ) == 1 );
 CHECK( dp::countMenu( Ints( 60 , 1 ) , 30 ) == C[ 60 ][ 30 ] );
 CHECK( dp::countMenu( Ints( 60 , 2 ) , 31 ) == 0 );
 CHECK( dp::countMenu( { 5 , 5 } , 5 ) == 2 );       // equal costs, two dishes
 CHECK( dp::countMenu( { 5 } , 0 ) == 1 && dp::countMenu( { 5 } , 4 ) == 0 );
 CHECK_THROWS( dp::countMenu( { 0 } , 5 ) , contract_violation );
 CHECK_THROWS( dp::countMenu( { 1 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 70, 2020-09-04, Longest single ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the longest contiguous subarray of A without repeated values, A_i in
/// [ 1 , n ]: a window [ start , end ] with a vector of booleans for its
/// values; a repeated value moves start past its first occurrence, so that
/// each value enters and leaves once, Theta( n ) time;
/// longestDistinctSegment() of the library, for any values by a hash table
/// \pre 1 <= A_i <= n
int longestSingle( const std::vector< int > & A )
{
 const int n = int( A.size() );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       [ n ]( int a ) {
                        return( ( a >= 1 ) && ( a <= n ) ); } ) );
 return( int( longestDistinctSegment( A ).second ) );
 }

}  // end( namespace dp )

// the example of the text, 4; against every subarray checked by std::set
ADS_TEST( "dp-70" )
{
 CHECK( dp::longestSingle( { 1 , 2 , 3 , 2 , 4 , 1 , 3 } ) == 4 );
 CHECK( dp::longestSingle( { 1 , 2 , 3 , 3 , 1 , 2 , 3 , 4 } ) == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int n = int( g() % 20 );
  std::vector< int > A( n );
  const int k = 1 + int( g() % unsigned( std::max( n , 1 ) ) );
  for( auto & a : A ) a = 1 + int( g() % unsigned( k ) );
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = i ; j < n ; ++j )
    if( std::set< int >( A.begin() + i , A.begin() + j + 1 ).size() ==
        std::size_t( j - i + 1 ) )
     best = std::max( best , j - i + 1 );
  CHECK( dp::longestSingle( A ) == best );
  }
 CHECK( dp::longestSingle( {} ) == 0 );
 CHECK( dp::longestSingle( { 1 } ) == 1 );
 CHECK( dp::longestSingle( std::vector< int >( 1000 , 7 ) ) == 1 );
 std::vector< int > p( 100000 );
 std::iota( p.begin() , p.end() , 1 );
 CHECK( dp::longestSingle( p ) == 100000 );
 CHECK( dp::longestSingle( { 1 , 2 , 1 } ) == 2 );
 CHECK( dp::longestSingle( { 2 , 1 , 2 , 1 } ) == 2 );
 CHECK_THROWS( dp::longestSingle( { 2 } ) , contract_violation );
 CHECK_THROWS( dp::longestSingle( { 0 , 1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------- 71, 2020-09-04, Sottostringa comune --------------------*/
/*--------------------------------------------------------------------------*/

// longestCommonSubstring() of the library, the table of the longest common
// suffixes of the text, on the example abcdef, abxcdexf (cde, 3), against
// every substring of T looked for in U, and on the edge cases
ADS_TEST( "dp-71" )
{
 auto lcsub = []( const std::string & T , const std::string & U ) {
  return( std::get< 0 >( longestCommonSubstring( T , U ) ) );
  };
 CHECK( lcsub( "abcdef" , "abxcdexf" ) == 3 );
 const auto [ len , ei , ej ] = longestCommonSubstring(
                                       std::string( "abcdef" ) ,
                                       std::string( "abxcdexf" ) );
 CHECK( std::string( "abcdef" ).substr( ei - len , len ) == "cde" );
 CHECK( std::string( "abxcdexf" ).substr( ej - len , len ) == "cde" );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto T = random_string( g() % 15 , 2 + g() % 2 , g ) ,
             U = random_string( g() % 15 , 2 + g() % 2 , g );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < T.size() ; ++i )
   for( std::size_t l = 1 ; i + l <= T.size() ; ++l )
    if( U.find( T.substr( i , l ) ) != std::string::npos )
     best = std::max( best , l );
  CHECK( lcsub( T , U ) == best );
  }
 CHECK( lcsub( "" , "abc" ) == 0 );
 CHECK( lcsub( "abc" , "xyz" ) == 0 );
 CHECK( lcsub( "a" , "a" ) == 1 );
 CHECK( lcsub( "" , "" ) == 0 && lcsub( "abc" , "" ) == 0 );
 CHECK( lcsub( "abc" , "abc" ) == 3 && lcsub( "b" , "abc" ) == 1 );
 CHECK( lcsub( "abc" , "zabcz" ) == 3 && lcsub( "abc" , "cba" ) == 1 );
 CHECK( lcsub( std::string( 1000 , 'a' ) , std::string( 700 , 'a' ) ) ==
        700 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- 72, 2021-01-18, Stessa visita -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the binary trees of n nodes labelled 1 ... n whose symmetric visit is
/// 1 , 2 , ... , n: each shape has one such labelling, so they are the
/// shapes, the Catalan number: DP[ 0 ] = DP[ 1 ] = 1, DP[ i ] = sum_{k=0}^{
/// i-1} DP[ k ] DP[ i - 1 - k ]; Theta( n^2 ) operations, on numbers of
/// Theta( n ) bits (T a BigInteger for large n), catalan() of the library.
/// The code of the text writes DP[ n - k - 1 ] for DP[ i - k - 1 ]
/// \pre n >= 0, C( n ) representable in T
template< class T = std::uint64_t >
T sameVisit( int n )
{
 return( ADS::catalan< T >( n ) );
 }

}  // end( namespace dp )

// the example n = 3, 5 trees; against C( 2 n , n ) / ( n + 1 ), against the
// strings of balanced parentheses of nestedParentheses() of the library,
// and against ( 2 n )! / ( ( n + 1 )! n! ) in BigInteger for n = 200
ADS_TEST( "dp-72" )
{
 CHECK( dp::sameVisit( 3 ) == 5 );
 for( int n = 0 ; n <= 33 ; ++n ) CHECK( dp::sameVisit( n ) == catalan( n ) );
 for( int n = 0 ; n <= 10 ; ++n )
  CHECK( dp::sameVisit( n ) == std::uint64_t(
         nestedParentheses( n , []( const std::string & ) {
                                  return( false ); } ) ) );
 CHECK( std::to_string( dp::sameVisit( 36 ) ) ==
        ( factorial( 72 ) / ( factorial( 37 ) * factorial( 36 ) ) )
        .toString() );
 CHECK( dp::sameVisit< BigInteger >( 200 ) ==
        factorial( 400 ) / ( factorial( 201 ) * factorial( 200 ) ) );
 CHECK( dp::sameVisit( 0 ) == 1 && dp::sameVisit( 1 ) == 1 );
 CHECK_THROWS( dp::sameVisit( -1 ) , contract_violation );
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
/*------------------------ End File exam_dp2.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
