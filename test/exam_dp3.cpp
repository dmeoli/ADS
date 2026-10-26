/*--------------------------------------------------------------------------*/
/*--------------------------- File exam_dp3.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the dynamic programming exercises 73 to 108 of the collection of
 * written exams of Montresor's Algoritmicamente: each official solution is
 * implemented in the namespace dp, at the level 3 of the contracts, and
 * checked on the examples of its text, against a brute force on random
 * small instances, and on its edge cases.
 *
 * The exercises 75 and 94 repeat the exercises 62 and 16, whose tests are
 * dp-62 and dp-16.
 *
 * \see A. Montresor, Algoritmicamente, Esercizi di programmazione dinamica,
 *      raccolte da compiti scritti
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/Contract.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/StockTrading.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/SubsequenceProblems.h"

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using I64 = std::int64_t;
using Ints = std::vector< int >;
using Longs = std::vector< I64 >;
using Grid = std::vector< Ints >;

constexpr int INF = std::numeric_limits< int >::max();
constexpr int NEGI = std::numeric_limits< int >::min();
constexpr I64 NEG = std::numeric_limits< I64 >::min();

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a random integer in [ lo , hi ]
int randInt( std::mt19937 & g , int lo , int hi )
{
 return( lo + int( g() % unsigned( hi - lo + 1 ) ) );
 }

/// n random integers in [ lo , hi ]
Ints randInts( std::mt19937 & g , int n , int lo , int hi )
{
 Ints A( n );
 for( auto & a : A ) a = randInt( g , lo , hi );
 return( A );
 }

/// n distinct random integers in [ lo , hi ]
Ints randDistinct( std::mt19937 & g , int n , int lo , int hi )
{
 Ints R;
 for( int v = lo ; v <= hi ; ++v ) R.push_back( v );
 std::shuffle( R.begin() , R.end() , g );
 R.resize( n );
 return( R );
 }

/// a random n x m matrix of integers in [ lo , hi ]
Grid randGrid( std::mt19937 & g , int n , int m , int lo , int hi )
{
 Grid A( n );
 for( auto & r : A ) r = randInts( g , m , lo , hi );
 return( A );
 }

/// a random string of n characters among the first s of A, B, C, ...
std::string randString( std::mt19937 & g , int n , int s )
{
 std::string w( std::size_t( n ) , 'A' );
 for( auto & c : w ) c = char( 'A' + randInt( g , 0 , s - 1 ) );
 return( w );
 }

/// the positions of the bits of s
Ints bitsOf( unsigned s )
{
 Ints P;
 for( int i = 0 ; s ; ++i , s >>= 1 )
  if( s & 1u ) P.push_back( i );
 return( P );
 }

/// all the binary tree shapes with n nodes: "." the empty tree, "(LR)" a
/// node with subtrees L and R
const std::vector< std::string > & shapes( int n )
{
 static std::vector< std::vector< std::string > > memo;
 static std::vector< char > done;
 if( int( memo.size() ) <= n ) {
  memo.resize( n + 1 );
  done.resize( n + 1 , 0 );
  }
 if( ! done[ n ] ) {
  std::vector< std::string > S;
  if( n == 0 )
   S.push_back( "." );
  for( int l = 0 ; l < n ; ++l )
   for( const auto & a : shapes( l ) )
    for( const auto & b : shapes( n - 1 - l ) )
     S.push_back( "(" + a + b + ")" );
  memo[ n ] = std::move( S );
  done[ n ] = 1;
  }
 return( memo[ n ] );
 }

/// a node of a parsed shape: its children, -1 if absent
using Kids = std::vector< std::array< int , 2 > >;

/// the nodes of the shape w, the root first
Kids parseShape( const std::string & w )
{
 Kids K;
 std::size_t p = 0;
 std::function< int( void ) > rec = [ & ]( void ) -> int {
  if( w[ p ] == '.' ) {
   ++p;
   return( -1 );
   }
  ++p;                                                  // '('
  const int v = int( K.size() );
  K.push_back( { -1 , -1 } );
  const int l = rec();
  const int r = rec();
  K[ v ] = { l , r };
  ++p;                                                  // ')'
  return( v );
  };
 rec();
 return( K );
 }

/// the size of the subtree of v in K
int subSize( const Kids & K , int v )
{
 return( v < 0 ? 0 :
         1 + subSize( K , K[ v ][ 0 ] ) + subSize( K , K[ v ][ 1 ] ) );
 }

/// the binomial coefficients up to 62 by Pascal's triangle
I64 binom( int n , int k )
{
 static std::vector< Longs > P;
 if( P.empty() ) {
  P.assign( 63 , Longs( 63 , 0 ) );
  for( int a = 0 ; a <= 62 ; ++a ) {
   P[ a ][ 0 ] = 1;
   for( int b = 1 ; b <= a ; ++b )
    P[ a ][ b ] = P[ a - 1 ][ b - 1 ] + P[ a - 1 ][ b ];
   }
  }
 return( ( k < 0 ) || ( k > n ) ? 0 : P[ n ][ k ] );
 }

/// the n-th Catalan number by its closed formula, n <= 30
I64 catalan( int n )
{
 return( binom( 2 * n , n ) / ( n + 1 ) );
 }

/// whether s is a subsequence of t
template< class S >
bool isSubsequence( const S & s , const S & t )
{
 std::size_t i = 0;
 for( std::size_t j = 0 ; ( i < s.size() ) && ( j < t.size() ) ; ++j )
  if( s[ i ] == t[ j ] ) ++i;
 return( i == s.size() );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-73, 2021-01-18, Stiamo compatti -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Stiamo compatti: the fewest items of A, distinct, to remove so that the
/// rest spans at most k. On A sorted, the interval DP of the text,
/// DP[ i ][ j ] = 0 if A[ j ] - A[ i ] <= k, else 1 + min( DP[ i + 1 ][ j ] ,
/// DP[ i ][ j - 1 ] ), filled by diagonals; Theta( n^2 ) time and space
/// \pre k > 0 and the items of A are distinct
int minRemoveDP( std::vector< int > A , int k )
{
 ADS_PRE( k > 0 );
 std::sort( A.begin() , A.end() );
 ADS_PRE( std::adjacent_find( A.begin() , A.end() ) == A.end() );
 const int n = int( A.size() );
 if( n == 0 )
  return( 0 );
 Grid DP( n , Ints( n , 0 ) );
 for( int h = 1 ; h < n ; ++h )
  for( int i = 0 ; i + h < n ; ++i ) {
   const int j = i + h;
   DP[ i ][ j ] = I64( A[ j ] ) - A[ i ] <= k ? 0 :
                  1 + std::min( DP[ i + 1 ][ j ] , DP[ i ][ j - 1 ] );
   }
 return( DP[ 0 ][ n - 1 ] );
 }

/// the same by a sliding window on A sorted: the window [ i , j ] grows
/// while it spans at most k, else it loses its first item; the answer is n
/// minus the widest window, O( n log n ) time
/// \pre k > 0 and the items of A are distinct
int minRemove( std::vector< int > A , int k )
{
 ADS_PRE( k > 0 );
 std::sort( A.begin() , A.end() );
 ADS_PRE( std::adjacent_find( A.begin() , A.end() ) == A.end() );
 const int n = int( A.size() );
 int best = 0;
 for( int i = 0 , j = 0 ; j < n ; )
  if( I64( A[ j ] ) - A[ i ] <= k ) {
   best = std::max( best , j - i + 1 );
   ++j;
   }
  else
   ++i;
 return( n - best );
 }

}  // end( namespace dp )

namespace {

int minRemoveBrute( const Ints & A , int k )
{
 const int n = int( A.size() );
 int best = 0;
 for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
  const Ints P = bitsOf( s );
  if( P.empty() ) continue;
  I64 lo = A[ P[ 0 ] ] , hi = lo;
  for( int p : P ) {
   lo = std::min< I64 >( lo , A[ p ] );
   hi = std::max< I64 >( hi , A[ p ] );
   }
  if( hi - lo <= k ) best = std::max( best , int( P.size() ) );
  }
 return( n - best );
 }

// the two examples, both versions against the subsets on random distinct
// items, the empty and the single item, the extremes of int, a span equal
// to k, every gap wider than k, sorted and reverse sorted A, the
// preconditions of both versions
ADS_TEST( "dp-73" )
{
 CHECK( dp::minRemove( { -3 , 3 , 4 , 7 , 9 , 12 , 15 } , 6 ) == 3 );
 CHECK( dp::minRemoveDP( { -3 , 3 , 4 , 7 , 9 , 12 , 15 } , 6 ) == 3 );
 CHECK( dp::minRemove( { -30 , 4 , 1 , -40 , 3 , 2 , -2 , -20 } , 4 ) == 4 );
 CHECK( dp::minRemoveDP( { -30 , 4 , 1 , -40 , 3 , 2 , -2 , -20 } , 4 ) ==
        4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints A = randDistinct( g , randInt( g , 0 , 12 ) , -30 , 30 );
  const int k = randInt( g , 1 , 20 );
  const int r = minRemoveBrute( A , k );
  CHECK( dp::minRemove( A , k ) == r && dp::minRemoveDP( A , k ) == r );
  }
 CHECK( dp::minRemove( {} , 1 ) == 0 && dp::minRemoveDP( {} , 1 ) == 0 );
 CHECK( dp::minRemove( { 5 } , 1 ) == 0 && dp::minRemoveDP( { 5 } , 1 ) == 0 );
 CHECK( dp::minRemove( { INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK( dp::minRemoveDP( { INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK( dp::minRemove( { INT_MIN , -1 , INT_MAX } , INT_MAX ) == 1 );
 CHECK( dp::minRemove( { 0 , INT_MAX } , INT_MAX ) == 0 );
 CHECK( dp::minRemove( { 0 , 5 } , 5 ) == 0 &&
        dp::minRemoveDP( { 0 , 5 } , 5 ) == 0 );
 CHECK( dp::minRemove( { 0 , 5 } , 4 ) == 1 &&
        dp::minRemoveDP( { 0 , 5 } , 4 ) == 1 );
 CHECK( dp::minRemove( { 30 , 1 , 20 , 10 } , 5 ) == 3 &&
        dp::minRemoveDP( { 30 , 1 , 20 , 10 } , 5 ) == 3 );
 CHECK( dp::minRemove( { 1 , 2 , 3 , 4 , 10 } , 3 ) == 1 &&
        dp::minRemoveDP( { 10 , 4 , 3 , 2 , 1 } , 3 ) == 1 );
 CHECK_THROWS( dp::minRemove( { 1 , 1 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::minRemove( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( dp::minRemoveDP( { 3 , 1 , 3 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::minRemoveDP( { 1 } , -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------- dp-74, 2021-02-08, Zaino leggero --------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of smallSum(): DP[ i ][ c ] < 0 marks the
/// subproblems not solved yet
int ssRec( const Ints & W , int i , int c , Grid & DP )
{
 if( c < 0 )
  return( INF );
 if( c == 0 )
  return( 0 );
 if( i == 0 )
  return( INF );
 if( DP[ i ][ c ] < 0 ) {
  const int a = ssRec( W , i - 1 , c , DP );
  const int b = ssRec( W , i - 1 , c - W[ i - 1 ] , DP );
  DP[ i ][ c ] = std::min( a , b == INF ? INF : b + 1 );
  }
 return( DP[ i ][ c ] );
 }

/// Zaino leggero: the size of a smallest subset of the weights W of sum
/// exactly C, INF if there is none. DP[ i ][ c ] = min( DP[ i - 1 ][ c ] ,
/// DP[ i - 1 ][ c - W[ i ] ] + 1 ), by memoization; O( n C ) time and space
/// \pre C >= 0 and the weights are positive
int smallSum( const Ints & W , int C )
{
 ADS_PRE( C >= 0 );
 ADS_PRE( std::all_of( W.begin() , W.end() , []( int w ) { return( w > 0 ); }
                     ) );
 const int n = int( W.size() );
 Grid DP( n + 1 , Ints( std::size_t( C ) + 1 , -1 ) );
 return( ssRec( W , n , C , DP ) );
 }

}  // end( namespace dp )

namespace {

int smallSumBrute( const Ints & W , int C )
{
 int best = INF;
 for( unsigned s = 0 ; s < ( 1u << W.size() ) ; ++s ) {
  I64 sum = 0;
  for( int p : bitsOf( s ) ) sum += W[ p ];
  if( sum == C ) best = std::min( best , std::popcount( s ) );
  }
 return( best );
 }

// the example, the subsets and the feasibility of subsetSumDP() on random
// weights, a sum of zero, all weights equal, a large capacity, every
// weight above C, the whole set needed, a negative weight
ADS_TEST( "dp-74" )
{
 CHECK( dp::smallSum( { 5 , 1 , 7 , 2 , 3 , 9 , 8 } , 17 ) == 2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints W = randInts( g , randInt( g , 0 , 12 ) , 1 , 20 );
  const int C = randInt( g , 0 , 60 );
  const int r = dp::smallSum( W , C );
  CHECK( r == smallSumBrute( W , C ) );
  CHECK( ( r < INF ) ==
         subsetSumDP( std::vector< long >( W.begin() , W.end() ) , C ) );
  }
 CHECK( dp::smallSum( {} , 0 ) == 0 && dp::smallSum( {} , 3 ) == INF );
 CHECK( dp::smallSum( { 4 } , 4 ) == 1 && dp::smallSum( { 4 } , 3 ) == INF );
 CHECK( dp::smallSum( Ints( 5 , 3 ) , 9 ) == 3 );
 CHECK( dp::smallSum( Ints( 5 , 3 ) , 10 ) == INF );
 CHECK( dp::smallSum( { 999999 , 1 , 1000000 } , 1000000 ) == 1 );
 CHECK( dp::smallSum( { 5 , 6 } , 0 ) == 0 );
 CHECK( dp::smallSum( { 10 , 20 } , 5 ) == INF );
 CHECK( dp::smallSum( { 1 , 2 , 4 , 8 } , 15 ) == 4 );
 CHECK( dp::smallSum( { 1 , 2 , 4 , 8 } , 16 ) == INF );
 CHECK_THROWS( dp::smallSum( { 0 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::smallSum( { 2 , -1 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::smallSum( { 1 } , -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- dp-76, 2021-06-16, Rimozioni ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Rimozioni: the most items of A, distinct and positive, that can be
/// removed so that the rest sums to exactly C, NEGI if none. DP[ i ][ c ]
/// = i if c = 0, else max( DP[ i - 1 ][ c ] + 1 , DP[ i - 1 ][ c - A[ i ] ] )
/// (remove the item or keep it); O( n C ) time and space
/// \pre C > 0 and the items of A are positive and distinct
int removeSum( const Ints & A , int C )
{
 ADS_PRE( C > 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() , []( int a ) { return( a > 0 ); }
                     ) );
 ADS_PRE( std::set< int >( A.begin() , A.end() ).size() == A.size() );
 const int n = int( A.size() );
 Grid DP( n + 1 , Ints( std::size_t( C ) + 1 , NEGI ) );
 for( int i = 0 ; i <= n ; ++i )
  DP[ i ][ 0 ] = i;
 for( int i = 1 ; i <= n ; ++i )
  for( int c = 1 ; c <= C ; ++c ) {
   DP[ i ][ c ] = DP[ i - 1 ][ c ] == NEGI ? NEGI : DP[ i - 1 ][ c ] + 1;
   if( c >= A[ i - 1 ] )
    DP[ i ][ c ] = std::max( DP[ i ][ c ] , DP[ i - 1 ][ c - A[ i - 1 ] ] );
   }
 return( DP[ n ][ C ] );
 }

}  // end( namespace dp )

namespace {

// the example, the duality with smallSum() and the subsets on random
// distinct items, the single item, all the items kept, C above the total,
// the preconditions
ADS_TEST( "dp-76" )
{
 CHECK( dp::removeSum( { 5 , 1 , 7 , 2 , 3 , 9 , 8 } , 17 ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints A = randDistinct( g , randInt( g , 0 , 12 ) , 1 , 25 );
  const int C = randInt( g , 1 , 70 );
  const int s = smallSumBrute( A , C );
  CHECK( dp::removeSum( A , C ) ==
         ( s == INF ? NEGI : int( A.size() ) - s ) );
  CHECK( dp::smallSum( A , C ) == s );
  }
 CHECK( dp::removeSum( {} , 1 ) == NEGI );
 CHECK( dp::removeSum( { 7 } , 7 ) == 0 &&
        dp::removeSum( { 7 } , 6 ) == NEGI );
 CHECK( dp::removeSum( { 1 , 2 , 3 , 4 } , 10 ) == 0 );
 CHECK( dp::removeSum( { 1 , 2 , 3 , 4 } , 4 ) == 3 );
 CHECK( dp::removeSum( { 1 , 2 , 3 , 4 } , 11 ) == NEGI );
 CHECK( dp::removeSum( { 9 , 8 } , 1 ) == NEGI );
 CHECK_THROWS( dp::removeSum( { 2 , 2 } , 2 ) , contract_violation );
 CHECK_THROWS( dp::removeSum( { 0 , 2 } , 2 ) , contract_violation );
 CHECK_THROWS( dp::removeSum( { 3 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::removeSum( { 2 } , 0 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------- dp-77, 2021-07-26, Sottosequenze k-contigue ---------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of maxSumK(): the value of DP[ i ][ j ] is
/// meaningful once solved[ i ][ j ] is set
I64 maxSumKRec( const Ints & V , int k , int i , int j ,
                std::vector< Longs > & DP ,
                std::vector< std::vector< char > > & solved )
{
 if( i <= 0 )
  return( 0 );
 if( ! solved[ i ][ j ] ) {
  const I64 take = maxSumKRec( V , k , i - 1 , k , DP , solved ) + V[ i - 1 ];
  DP[ i ][ j ] = j == 0 ? take :
                 std::max( maxSumKRec( V , k , i - 1 , j - 1 , DP , solved ) ,
                           take );
  solved[ i ][ j ] = 1;
  }
 return( DP[ i ][ j ] );
 }

/// Sottosequenze k-contigue: the largest sum of a subsequence of V that
/// never deletes more than k consecutive items (the leading and the
/// trailing runs included). DP[ i ][ j ] is the best of the first i items
/// when j more consecutive items may still be deleted: DP[ i - 1 ][ k ] +
/// V[ i ] if j = 0, else the max of that and DP[ i - 1 ][ j - 1 ]; the
/// answer is DP[ n ][ k ], O( n k ) time and space. The text marks the
/// unsolved entries by -1, wrong here since V may be negative: a flag
/// table marks them instead
/// \pre k >= 0
I64 maxSumK( const Ints & V , int k )
{
 ADS_PRE( k >= 0 );
 const int n = int( V.size() );
 std::vector< Longs > DP( n + 1 , Longs( k + 1 , 0 ) );
 std::vector< std::vector< char > > solved( n + 1 ,
                                            std::vector< char >( k + 1 , 0 ) );
 return( maxSumKRec( V , k , n , k , DP , solved ) );
 }

/// the same in O( n ) space: DP[ i ] is the best ending by taking V[ i ],
/// V[ i ] + max of DP[ j ] for i - k - 1 <= j < i (DP[ 0 ] = 0 the start),
/// the answer the max of DP[ n - k .. n ]; O( n k ) time. The code of the
/// text starts j at i - k, which deletes at most k - 1 items between two
/// taken ones, and its n - k may be negative
/// \pre k >= 0
I64 maxSumKLinear( const Ints & V , int k )
{
 ADS_PRE( k >= 0 );
 const int n = int( V.size() );
 Longs DP( n + 1 , 0 );
 for( int i = 1 ; i <= n ; ++i ) {
  DP[ i ] = NEG;
  for( int j = std::max( 0 , i - k - 1 ) ; j < i ; ++j )
   DP[ i ] = std::max( DP[ i ] , DP[ j ] + V[ i - 1 ] );
  }
 return( *std::max_element( DP.begin() + std::max( 0 , n - k ) , DP.end() ) );
 }

}  // end( namespace dp )

namespace {

I64 maxSumKBrute( const Ints & V , int k )
{
 const int n = int( V.size() );
 I64 best = NEG;
 for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
  int run = 0 , worst = 0;
  I64 sum = 0;
  for( int i = 0 ; i < n ; ++i )
   if( ( s >> i ) & 1u ) {
    sum += V[ i ];
    run = 0;
    }
   else
    worst = std::max( worst , ++run );
  if( worst <= k ) best = std::max( best , sum );
  }
 return( best );
 }

// the gaps of the examples, both recurrences against the subsequences on
// random items, the text's range of j failing, the empty vector, a k as
// long as V, all the items negative, a single item, k beyond n, the
// precondition of both versions
ADS_TEST( "dp-77" )
{
 const Ints V = { 1 , 2 , 3 , 4 , 5 , 6 };
 auto maxGap = [ & ]( const Ints & S ) {      // S, an increasing subsequence
  int prev = 0 , gap = 0;                     // of V, by its values
  for( int x : S ) {
   gap = std::max( gap , x - prev - 1 );
   prev = x;
   }
  return( std::max( gap , int( V.size() ) - prev ) );
  };
 CHECK( maxGap( { 1 , 3 , 5 } ) == 1 && maxGap( { 1 , 4 , 6 } ) == 2 &&
        maxGap( { 1 , 5 } ) == 3 );
 CHECK( dp::maxSumK( V , 0 ) == 21 && dp::maxSumKLinear( V , 0 ) == 21 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Ints A = randInts( g , randInt( g , 0 , 12 ) , -10 , 10 );
  const int k = randInt( g , 0 , 4 );
  const I64 r = maxSumKBrute( A , k );
  CHECK( dp::maxSumK( A , k ) == r && dp::maxSumKLinear( A , k ) == r );
  }
 auto textLinear = []( const Ints & A , int k ) {   // j from max( 0 , i - k )
  const int n = int( A.size() );
  Longs DP( n + 1 , 0 );
  for( int i = 1 ; i <= n ; ++i ) {
   DP[ i ] = NEG;
   for( int j = std::max( 0 , i - k ) ; j < i ; ++j )
    DP[ i ] = std::max( DP[ i ] , DP[ j ] + A[ i - 1 ] );
   }
  return( *std::max_element( DP.begin() + std::max( 0 , n - k ) ,
                             DP.end() ) );
  };
 CHECK( dp::maxSumK( { 5 , -1 , 5 } , 1 ) == 10 );
 CHECK( textLinear( { 5 , -1 , 5 } , 1 ) == 9 );
 CHECK( dp::maxSumK( {} , 3 ) == 0 && dp::maxSumKLinear( {} , 0 ) == 0 );
 CHECK( dp::maxSumK( { -4 , -2 , -7 } , 3 ) == 0 );
 CHECK( dp::maxSumK( { -4 , -2 , -7 } , 1 ) == -2 );
 CHECK( dp::maxSumKLinear( { -4 , -2 , -7 } , 1 ) == -2 );
 CHECK( dp::maxSumK( { -4 , -2 , -7 } , 0 ) == -13 );
 CHECK( dp::maxSumK( Ints( 1000 , INT_MAX ) , 2 ) == I64( 1000 ) * INT_MAX );
 CHECK( dp::maxSumK( { 5 } , 0 ) == 5 && dp::maxSumKLinear( { 5 } , 0 ) == 5 );
 CHECK( dp::maxSumK( { -5 } , 0 ) == -5 &&
        dp::maxSumKLinear( { -5 } , 0 ) == -5 );
 CHECK( dp::maxSumK( { -5 } , 1 ) == 0 &&
        dp::maxSumKLinear( { -5 } , 1 ) == 0 );
 CHECK( dp::maxSumK( { 3 , -9 , 4 } , 10 ) == 7 &&
        dp::maxSumKLinear( { 3 , -9 , 4 } , 10 ) == 7 );
 CHECK( dp::maxSumK( Ints( 1000 , INT_MIN ) , 0 ) == I64( 1000 ) * INT_MIN );
 CHECK( dp::maxSumKLinear( Ints( 1000 , INT_MIN ) , 1 ) ==
        I64( 500 ) * INT_MIN );
 CHECK_THROWS( dp::maxSumK( V , -1 ) , contract_violation );
 CHECK_THROWS( dp::maxSumKLinear( V , -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-78, 2021-08-31, Somma Palindroma -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Somma Palindroma: the number of palindromic sequences of even length of
/// positive integers summing to n. DP[ 0 ] = 1, DP[ i ] = sum of DP[ i - 2 j
/// ] for 1 <= j <= i / 2 (the outer pair j), over even i; O( n^2 ) time
/// \pre n > 0 even, n <= 126
I64 countSumEven( int n )
{
 ADS_PRE( ( n > 0 ) && ( n % 2 == 0 ) && ( n <= 126 ) );
 Longs DP( n + 1 , 0 );
 DP[ 0 ] = 1;
 for( int i = 2 ; i <= n ; i += 2 )
  for( int j = 1 ; j <= i / 2 ; ++j )
   DP[ i ] += DP[ i - 2 * j ];
 return( DP[ n ] );
 }

/// the closed form of the text, DP[ i ] = 2 DP[ i - 2 ]: 2^( n / 2 - 1 )
/// \pre n > 0 even, n <= 126
I64 countSumEvenFast( int n )
{
 ADS_PRE( ( n > 0 ) && ( n % 2 == 0 ) && ( n <= 126 ) );
 return( I64( 1 ) << ( n / 2 - 1 ) );
 }

}  // end( namespace dp )

namespace {

// the compositions of n, by their cut points, that are even palindromes
I64 countSumEvenBrute( int n )
{
 I64 c = 0;
 for( unsigned s = 0 ; s < ( 1u << ( n - 1 ) ) ; ++s ) {
  Ints parts;
  int last = 0;
  for( int i = 1 ; i < n ; ++i )
   if( ( s >> ( i - 1 ) ) & 1u ) {
    parts.push_back( i - last );
    last = i;
    }
  parts.push_back( n - last );
  if( ( parts.size() % 2 == 0 ) &&
      std::equal( parts.begin() , parts.end() , parts.rbegin() ) )
   ++c;
  }
 return( c );
 }

// the example, the compositions up to 20, the smallest and the largest n,
// the preconditions of both versions (odd, zero, negative, beyond 126)
ADS_TEST( "dp-78" )
{
 CHECK( dp::countSumEven( 6 ) == 4 && dp::countSumEvenFast( 6 ) == 4 );
 for( int n = 2 ; n <= 20 ; n += 2 ) {
  const I64 r = countSumEvenBrute( n );
  CHECK( dp::countSumEven( n ) == r && dp::countSumEvenFast( n ) == r );
  }
 CHECK( dp::countSumEven( 2 ) == 1 );
 for( int n = 22 ; n <= 126 ; n += 2 )
  CHECK( dp::countSumEven( n ) == dp::countSumEvenFast( n ) );
 CHECK( dp::countSumEven( 126 ) == ( I64( 1 ) << 62 ) );
 CHECK_THROWS( dp::countSumEven( 5 ) , contract_violation );
 CHECK_THROWS( dp::countSumEven( 0 ) , contract_violation );
 CHECK_THROWS( dp::countSumEven( -2 ) , contract_violation );
 CHECK_THROWS( dp::countSumEven( 128 ) , contract_violation );
 CHECK( dp::countSumEvenFast( 2 ) == 1 &&
        dp::countSumEvenFast( 126 ) == ( I64( 1 ) << 62 ) );
 CHECK_THROWS( dp::countSumEvenFast( 3 ) , contract_violation );
 CHECK_THROWS( dp::countSumEvenFast( 0 ) , contract_violation );
 CHECK_THROWS( dp::countSumEvenFast( 128 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-79, 2022-01-17, Ben bilanciato --------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of countWellBalanced()
I64 wbRec( int i , Longs & DP )
{
 if( i <= 1 )
  return( 1 );
 if( DP[ i ] < 0 )
  DP[ i ] = i % 2 ? wbRec( ( i - 1 ) / 2 , DP ) * wbRec( ( i - 1 ) / 2 , DP ) :
                    2 * wbRec( ( i - 1 ) / 2 , DP ) * wbRec( i / 2 , DP );
 return( DP[ i ] );
 }

/// Ben bilanciato: the binary trees of n nodes whose every node has
/// subtrees of sizes differing by at most 1. DP[ i ] = DP[ ( i - 1 ) / 2
/// ]^2 for i odd, 2 DP[ ( i - 1 ) / 2 ] DP[ i / 2 ] for i even, by
/// memoization; O( n ) space, O( log n ) entries touched
/// \pre 0 <= n <= 147 (beyond, the count exceeds 2^63)
I64 countWellBalanced( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 147 ) );
 Longs DP( n + 1 , -1 );
 return( wbRec( n , DP ) );
 }

}  // end( namespace dp )

namespace {

bool wellBalanced( const Kids & K , int v )
{
 if( v < 0 ) return( true );
 return( std::abs( subSize( K , K[ v ][ 0 ] ) - subSize( K , K[ v ][ 1 ] ) )
         <= 1 && wellBalanced( K , K[ v ][ 0 ] ) &&
         wellBalanced( K , K[ v ][ 1 ] ) );
 }

// the sequence of the text, all the tree shapes up to 11 nodes, the full
// trees of 2^h - 1 nodes, unique
ADS_TEST( "dp-79" )
{
 const Longs seq = { 1 , 1 , 2 , 1 , 4 , 4 , 4 , 1 , 8 , 16 };
 for( int n = 0 ; n < 10 ; ++n )
  CHECK( dp::countWellBalanced( n ) == seq[ n ] );
 for( int n = 0 ; n <= 11 ; ++n ) {
  I64 c = 0;
  for( const auto & w : shapes( n ) )
   if( wellBalanced( parseShape( w ) , n ? 0 : -1 ) ) ++c;
  CHECK( dp::countWellBalanced( n ) == c );
  }
 for( int h = 1 ; h <= 7 ; ++h )
  CHECK( dp::countWellBalanced( ( 1 << h ) - 1 ) == 1 );
 CHECK( dp::countWellBalanced( 147 ) == ( I64( 1 ) << 60 ) );
 CHECK_THROWS( dp::countWellBalanced( 148 ) , contract_violation );
 CHECK_THROWS( dp::countWellBalanced( -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------- dp-80, 2022-01-17, Longest repeating sequence --------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Longest repeating sequence: the longest subsequence occurring twice in T
/// with its i-th items never at the same place. The LCS of T with itself
/// forbidding the matches with i = j: DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] +
/// 1 if T[ i ] = T[ j ] and i != j, else max( DP[ i - 1 ][ j ] , DP[ i ][ j
/// - 1 ] ); Theta( n^2 ) time and space
int lrs( const std::string & T )
{
 const std::size_t n = T.size();
 Grid DP( n + 1 , Ints( n + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = ( T[ i - 1 ] == T[ j - 1 ] ) && ( i != j ) ?
                  DP[ i - 1 ][ j - 1 ] + 1 :
                  std::max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
 return( DP[ n ][ n ] );
 }

}  // end( namespace dp )

namespace {

int lrsBrute( const std::string & T )
{
 const int n = int( T.size() );
 int best = 0;
 for( unsigned a = 0 ; a < ( 1u << n ) ; ++a )
  for( unsigned b = 0 ; b < ( 1u << n ) ; ++b ) {
   if( std::popcount( a ) != std::popcount( b ) ||
       std::popcount( a ) <= best ) continue;
   const Ints I = bitsOf( a ) , J = bitsOf( b );
   bool ok = true;
   for( std::size_t t = 0 ; ok && ( t < I.size() ) ; ++t )
    ok = ( I[ t ] != J[ t ] ) && ( T[ I[ t ] ] == T[ J[ t ] ] );
   if( ok ) best = int( I.size() );
   }
 return( best );
 }

// the examples, the pairs of index sets on random strings, the empty and
// the single item, two items equal or distinct, all items equal, the whole
// alphabet once and twice
ADS_TEST( "dp-80" )
{
 CHECK( dp::lrs( "AABBCC" ) == 3 && dp::lrs( "ABCABC" ) == 3 );
 CHECK( dp::lrs( "AAAB" ) == 2 && dp::lrs( "AAB" ) == 1 );
 CHECK( dp::lrs( "ABCCBA" ) == 1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::string T = randString( g , randInt( g , 0 , 8 ) ,
                                    randInt( g , 1 , 3 ) );
  CHECK( dp::lrs( T ) == lrsBrute( T ) );
  }
 CHECK( dp::lrs( "" ) == 0 && dp::lrs( "A" ) == 0 );
 CHECK( dp::lrs( std::string( 500 , 'A' ) ) == 499 );
 CHECK( dp::lrs( "ABCDEFGH" ) == 0 );
 CHECK( dp::lrs( "AA" ) == 1 && dp::lrs( "AB" ) == 0 );
 std::string w;
 for( char c = 'A' ; c <= 'Z' ; ++c ) w += c;
 CHECK( dp::lrs( w ) == 0 && dp::lrs( w + w ) == 26 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------- dp-81, 2022-02-07, Discordville ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Discordville: the largest total donation D of the houses that take part,
/// house i refusing if any of the N[ i ] houses before it takes part.
/// DP[ 0 ] = 0, DP[ i ] = max( DP[ i - 1 ] , D[ i - 1 ] + DP[ i - 1 - N[ i -
/// 1 ] ] ); Theta( n ) time and space
/// \pre n > 0, D >= 0, 0 <= N[ i ] <= i
I64 discordville( const Ints & D , const Ints & N )
{
 const int n = int( D.size() );
 ADS_PRE( ( n > 0 ) && ( int( N.size() ) == n ) );
 for( int i = 0 ; i < n ; ++i )
  ADS_PRE( ( D[ i ] >= 0 ) && ( N[ i ] >= 0 ) && ( N[ i ] <= i ) );
 Longs DP( n + 1 , 0 );
 for( int i = 1 ; i <= n ; ++i )
  DP[ i ] = std::max( DP[ i - 1 ] , D[ i - 1 ] + DP[ i - 1 - N[ i - 1 ] ] );
 return( DP[ n ] );
 }

}  // end( namespace dp )

namespace {

I64 discordvilleBrute( const Ints & D , const Ints & N )
{
 const int n = int( D.size() );
 I64 best = 0;
 for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
  bool ok = true;
  I64 sum = 0;
  for( int i : bitsOf( s ) ) {
   sum += D[ i ];
   for( int j = i - N[ i ] ; j < i ; ++j )
    if( ( s >> j ) & 1u ) ok = false;
   }
  if( ok ) best = std::max( best , sum );
  }
 return( best );
 }

// the example, the subsets on random villages, one house, nobody refusing,
// everybody refusing everybody before, all donations zero, the
// preconditions (no house, sizes differing, D or N negative, N[ i ] > i)
ADS_TEST( "dp-81" )
{
 CHECK( dp::discordville( { 3 , 4 , 5 , 7 , 1 } , { 0 , 1 , 1 , 3 , 1 } ) ==
        9 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = randInt( g , 1 , 12 );
  const Ints D = randInts( g , n , 0 , 20 );
  Ints N( n );
  for( int i = 0 ; i < n ; ++i ) N[ i ] = randInt( g , 0 , i );
  CHECK( dp::discordville( D , N ) == discordvilleBrute( D , N ) );
  }
 CHECK( dp::discordville( { 6 } , { 0 } ) == 6 );
 CHECK( dp::discordville( { 1 , 2 , 3 } , { 0 , 0 , 0 } ) == 6 );
 CHECK( dp::discordville( { 1 , 5 , 3 } , { 0 , 1 , 2 } ) == 5 );
 CHECK( dp::discordville( Ints( 1000 , INT_MAX ) , Ints( 1000 , 0 ) ) ==
        I64( 1000 ) * INT_MAX );
 CHECK( dp::discordville( { 0 , 0 , 0 } , { 0 , 1 , 2 } ) == 0 );
 CHECK_THROWS( dp::discordville( { 1 , 1 } , { 1 , 0 } ) ,
               contract_violation );
 CHECK_THROWS( dp::discordville( {} , {} ) , contract_violation );
 CHECK_THROWS( dp::discordville( { 1 , 2 } , { 0 } ) , contract_violation );
 CHECK_THROWS( dp::discordville( { -1 } , { 0 } ) , contract_violation );
 CHECK_THROWS( dp::discordville( { 1 , 2 } , { 0 , -1 } ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- dp-82, 2022-02-07, Math ways ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Math ways: the sequences of the operations +1, *2 and squaring leading
/// from k to n. DP[ n ] = 1, DP[ i ] = DP[ i + 1 ] + DP[ 2 i ] + DP[ i^2 ],
/// the terms beyond n left out, bottom-up from n - 1 down to k; Theta( n -
/// k + 1 ) time; k >= 3 makes the three results distinct
/// \pre 3 <= k <= n <= 10000 (the count stays below 2^63)
I64 mathWays( int n , int k )
{
 ADS_PRE( ( 3 <= k ) && ( k <= n ) && ( n <= 10000 ) );
 Longs DP( std::size_t( n ) + 1 , 0 );
 DP[ n ] = 1;
 for( int i = n - 1 ; i >= k ; --i ) {
  DP[ i ] = DP[ i + 1 ];
  if( 2 * I64( i ) <= n ) DP[ i ] += DP[ 2 * i ];
  if( I64( i ) * i <= n ) DP[ i ] += DP[ i * i ];
  }
 return( DP[ k ] );
 }

}  // end( namespace dp )

namespace {

I64 mathWaysBrute( I64 i , I64 n )
{
 if( i == n ) return( 1 );
 if( i > n ) return( 0 );
 return( mathWaysBrute( i + 1 , n ) + mathWaysBrute( 2 * i , n ) +
         mathWaysBrute( i * i , n ) );
 }

// the example, the plain recursion for every n <= 50 and every k, k = n
// (also at the largest n), the preconditions
ADS_TEST( "dp-82" )
{
 CHECK( dp::mathWays( 10 , 3 ) == 5 );
 for( int n = 3 ; n <= 50 ; ++n )
  for( int k = 3 ; k <= n ; ++k )
   CHECK( dp::mathWays( n , k ) == mathWaysBrute( k , n ) );
 CHECK( dp::mathWays( 3 , 3 ) == 1 && dp::mathWays( 1000 , 999 ) == 1 );
 CHECK( dp::mathWays( 10000 , 3 ) == 81838953590412168 );
 CHECK( dp::mathWays( 10000 , 10000 ) == 1 &&
        dp::mathWays( 10000 , 9999 ) == 1 );
 CHECK_THROWS( dp::mathWays( 10001 , 3 ) , contract_violation );
 CHECK_THROWS( dp::mathWays( 10 , 2 ) , contract_violation );
 CHECK_THROWS( dp::mathWays( 5 , 6 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-83, 2022-06-21, Double sequence -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of countSequences()
I64 csRec( int i , int j , std::vector< Longs > & DP )
{
 if( j == 0 )
  return( 0 );
 if( i == 1 )
  return( j );
 if( DP[ i ][ j ] < 0 )
  DP[ i ][ j ] = csRec( i - 1 , j / 2 , DP ) + csRec( i , j - 1 , DP );
 return( DP[ i ][ j ] );
 }

/// Double sequence: the sequences of n positive integers at most m, each at
/// least twice the one before. DP[ i ][ j ] counts those of length i with
/// values at most j: j if i = 1, DP[ i - 1 ][ j / 2 ] + DP[ i ][ j - 1 ]
/// (the last one j or less than j), by memoization; O( n m )
/// \pre n > 0 and m > 0
I64 countSequences( int n , int m )
{
 ADS_PRE( ( n > 0 ) && ( m > 0 ) );
 std::vector< Longs > DP( n + 1 , Longs( std::size_t( m ) + 1 , -1 ) );
 return( csRec( n , m , DP ) );
 }

/// the alternative of the text: E[ i ][ j ] counts those ending in j, the
/// sum of E[ i - 1 ][ t ] for t <= j / 2; O( n m^2 )
/// \pre n > 0 and m > 0
I64 countSequencesEnding( int n , int m )
{
 ADS_PRE( ( n > 0 ) && ( m > 0 ) );
 std::vector< Longs > E( n + 1 , Longs( std::size_t( m ) + 1 , 0 ) );
 for( int j = 1 ; j <= m ; ++j ) E[ 1 ][ j ] = 1;
 for( int i = 2 ; i <= n ; ++i )
  for( int j = 1 ; j <= m ; ++j )
   for( int t = 1 ; t <= j / 2 ; ++t )
    E[ i ][ j ] += E[ i - 1 ][ t ];
 I64 c = 0;
 for( int j = 1 ; j <= m ; ++j ) c += E[ n ][ j ];
 return( c );
 }

}  // end( namespace dp )

namespace {

I64 countSequencesBrute( int left , I64 low , int m )
{
 if( left == 0 ) return( 1 );
 I64 c = 0;
 for( I64 v = std::max< I64 >( low , 1 ) ; v <= m ; ++v )
  c += countSequencesBrute( left - 1 , 2 * v , m );
 return( c );
 }

// the example, the enumeration for small n and m, length 1, lengths too
// long for m, a large m, m = 1, the preconditions of both versions
ADS_TEST( "dp-83" )
{
 CHECK( dp::countSequences( 4 , 10 ) == 4 );
 CHECK( dp::countSequencesEnding( 4 , 10 ) == 4 );
 for( int n = 1 ; n <= 6 ; ++n )
  for( int m = 1 ; m <= 40 ; ++m ) {
   const I64 r = countSequencesBrute( n , 1 , m );
   CHECK( dp::countSequences( n , m ) == r &&
          dp::countSequencesEnding( n , m ) == r );
   }
 CHECK( dp::countSequences( 1 , 77 ) == 77 );
 CHECK( dp::countSequences( 5 , 15 ) == 0 &&
        dp::countSequences( 5 , 16 ) == 1 );
 CHECK( dp::countSequences( 10 , 512 ) == 1 );
 CHECK( dp::countSequences( 3 , 300 ) == dp::countSequencesEnding( 3 , 300 ) );
 CHECK( dp::countSequences( 1 , 1 ) == 1 && dp::countSequences( 2 , 1 ) == 0 );
 CHECK( dp::countSequencesEnding( 2 , 1 ) == 0 );
 CHECK_THROWS( dp::countSequences( 0 , 3 ) , contract_violation );
 CHECK_THROWS( dp::countSequences( 3 , 0 ) , contract_violation );
 CHECK_THROWS( dp::countSequencesEnding( 0 , 3 ) , contract_violation );
 CHECK_THROWS( dp::countSequencesEnding( 3 , 0 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*----------------- dp-84, 2022-07-13, Sequenze crescenti ------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Sequenze crescenti: the strictly increasing subsequences of length k of
/// A. DP[ i ][ j ] counts those of length j ending in A[ i ]: 1 if j = 1,
/// else the sum of DP[ m ][ j - 1 ] for m < i with A[ m ] < A[ i ]; the
/// answer is the sum of DP[ i ][ k ]; O( k n^2 ) time, O( n k ) space
/// \pre k > 0
I64 countIncreasing( const Ints & A , int k )
{
 ADS_PRE( k > 0 );
 const int n = int( A.size() );
 if( k > n )
  return( 0 );
 std::vector< Longs > DP( n , Longs( k + 1 , 0 ) );
 I64 c = 0;
 for( int i = 0 ; i < n ; ++i ) {
  DP[ i ][ 1 ] = 1;
  for( int j = 2 ; j <= k ; ++j )
   for( int m = 0 ; m < i ; ++m )
    if( A[ m ] < A[ i ] ) DP[ i ][ j ] += DP[ m ][ j - 1 ];
  c += DP[ i ][ k ];
  }
 return( c );
 }

}  // end( namespace dp )

namespace {

I64 countIncreasingBrute( const Ints & A , int k )
{
 I64 c = 0;
 for( unsigned s = 0 ; s < ( 1u << A.size() ) ; ++s ) {
  if( std::popcount( s ) != k ) continue;
  const Ints P = bitsOf( s );
  bool ok = true;
  for( std::size_t t = 1 ; t < P.size() ; ++t )
   if( A[ P[ t - 1 ] ] >= A[ P[ t ] ] ) ok = false;
  if( ok ) ++c;
  }
 return( c );
 }

// the example, the k-subsets on random distinct items, k = 1, k > n, a
// sorted A, which gives the binomial coefficients, k = n, a reverse sorted
// A, equal items (never increasing) against the subsets
ADS_TEST( "dp-84" )
{
 CHECK( dp::countIncreasing( { -3 , 7 , 5 , 6 , 9 } , 3 ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Ints A = randDistinct( g , randInt( g , 0 , 12 ) , -20 , 20 );
  const int k = randInt( g , 1 , 6 );
  CHECK( dp::countIncreasing( A , k ) == countIncreasingBrute( A , k ) );
  }
 CHECK( dp::countIncreasing( {} , 1 ) == 0 );
 CHECK( dp::countIncreasing( { 3 , 1 , 2 } , 1 ) == 3 );
 CHECK( dp::countIncreasing( { 3 , 1 , 2 } , 4 ) == 0 );
 Ints S( 60 );
 for( int i = 0 ; i < 60 ; ++i ) S[ i ] = i;
 CHECK( dp::countIncreasing( S , 30 ) == binom( 60 , 30 ) );
 CHECK( dp::countIncreasing( S , 60 ) == 1 );
 CHECK( dp::countIncreasing( Ints( S.rbegin() , S.rend() ) , 2 ) == 0 );
 CHECK( dp::countIncreasing( Ints( 7 , 4 ) , 1 ) == 7 &&
        dp::countIncreasing( Ints( 7 , 4 ) , 2 ) == 0 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const Ints A = randInts( g , randInt( g , 0 , 11 ) , 1 , 3 );
  const int k = randInt( g , 1 , 4 );
  CHECK( dp::countIncreasing( A , k ) == countIncreasingBrute( A , k ) );
  }
 CHECK_THROWS( dp::countIncreasing( S , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- dp-85, 2022-08-24, Connessioni ---------------------*/
/*--------------------------------------------------------------------------*/

// the largest set of non-crossing connections between equal items is the
// LCS of X and Y, so lcsTable() of the library answers: the example, the
// subsequences of X on random vectors (also of different lengths), one
// side empty, all equal, nothing in common, Y the reverse of X

int maxConnectionsBrute( const Ints & X , const Ints & Y )
{
 int best = 0;
 for( unsigned s = 0 ; s < ( 1u << X.size() ) ; ++s ) {
  if( std::popcount( s ) <= best ) continue;
  Ints S;
  for( int p : bitsOf( s ) ) S.push_back( X[ p ] );
  if( isSubsequence( S , Y ) ) best = int( S.size() );
  }
 return( best );
 }

ADS_TEST( "dp-85" )
{
 const Ints X = { 1 , 3 , 6 , 7 , 5 , 9 } , Y = { 1 , 8 , 3 , 7 , 8 , 9 };
 CHECK( lcsTable( X , Y )[ 6 ][ 6 ] == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = randInt( g , 0 , 11 );
  const Ints A = randInts( g , n , 1 , 4 ) , B = randInts( g , n , 1 , 4 );
  CHECK( lcsTable( A , B )[ n ][ n ] == maxConnectionsBrute( A , B ) );
  }
 CHECK( lcsTable( Ints() , Ints() )[ 0 ][ 0 ] == 0 );
 CHECK( lcsTable( Ints( 300 , 2 ) , Ints( 300 , 2 ) )[ 300 ][ 300 ] == 300 );
 CHECK( lcsTable( Ints( 300 , 2 ) , Ints( 300 , 3 ) )[ 300 ][ 300 ] == 0 );
 CHECK( lcsTable( Ints() , Ints{ 1 , 2 } )[ 0 ][ 2 ] == 0 );
 CHECK( lcsTable( Ints{ 1 , 2 } , Ints() )[ 2 ][ 0 ] == 0 );
 CHECK( lcsTable( Ints{ 1 , 2 , 3 } , Ints{ 3 , 2 , 1 } )[ 3 ][ 3 ] == 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = randInt( g , 0 , 10 ) , m = randInt( g , 0 , 10 );
  const Ints A = randInts( g , n , 1 , 3 ) , B = randInts( g , m , 1 , 3 );
  CHECK( lcsTable( A , B )[ n ][ m ] == maxConnectionsBrute( A , B ) );
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------- dp-86, 2023-02-09, k-prodotto ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of productK()
I64 pkRec( const Ints & A , std::vector< Longs > & DP , int i , int k )
{
 if( i == 0 )
  return( 1 );
 if( DP[ i ][ k ] < 0 )
  DP[ i ][ k ] = A[ i - 1 ] > k ? pkRec( A , DP , i - 1 , k ) :
                 pkRec( A , DP , i - 1 , k ) +
                 pkRec( A , DP , i - 1 , k / A[ i - 1 ] );
 return( DP[ i ][ k ] );
 }

/// k-prodotto: the subsequences of A, positive (the empty one, of product
/// 1, included), whose product is at most m. DP[ i ][ k ] = DP[ i - 1 ][ k
/// ] + DP[ i - 1 ][ k / A[ i ] ] (floor; the second term only if A[ i ] <=
/// k), by memoization; O( n m ) time and space
/// \pre m > 0 and the items of A are positive
I64 productK( const Ints & A , int m )
{
 ADS_PRE( m > 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() , []( int a ) { return( a > 0 ); }
                     ) );
 const int n = int( A.size() );
 std::vector< Longs > DP( n + 1 , Longs( std::size_t( m ) + 1 , -1 ) );
 return( pkRec( A , DP , n , m ) );
 }

}  // end( namespace dp )

namespace {

I64 productKBrute( const Ints & A , int m )
{
 I64 c = 0;
 for( unsigned s = 0 ; s < ( 1u << A.size() ) ; ++s ) {
  I64 p = 1;
  for( int i : bitsOf( s ) )
   if( ( p *= A[ i ] ) > m ) break;
  if( p <= m ) ++c;
  }
 return( c );
 }

// the example, the subsets on random items, all ones, the empty A, a
// single item larger than m, m = 1, every subset fitting, the
// preconditions
ADS_TEST( "dp-86" )
{
 CHECK( dp::productK( { 1 , 3 , 2 , 4 } , 10 ) == 12 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints A = randInts( g , randInt( g , 0 , 12 ) , 1 , 6 );
  const int m = randInt( g , 1 , 80 );
  CHECK( dp::productK( A , m ) == productKBrute( A , m ) );
  }
 CHECK( dp::productK( {} , 5 ) == 1 && dp::productK( { 9 } , 5 ) == 1 );
 CHECK( dp::productK( Ints( 20 , 1 ) , 1 ) == ( I64( 1 ) << 20 ) );
 CHECK( dp::productK( { 2 , 3 } , 6 ) == 4 );
 CHECK( dp::productK( { 1000000 , 1000000 } , 1000000 ) == 3 );
 CHECK( dp::productK( { 2 , 3 , 5 } , 1 ) == 1 );
 CHECK( dp::productK( { 2 , 2 , 2 } , 8 ) == 8 &&
        dp::productK( { 2 , 2 , 2 } , 7 ) == 7 );
 CHECK( dp::productK( { INT_MAX } , 1 ) == 1 );
 CHECK_THROWS( dp::productK( { 0 } , 5 ) , contract_violation );
 CHECK_THROWS( dp::productK( { 2 , -3 } , 5 ) , contract_violation );
 CHECK_THROWS( dp::productK( { 2 } , 0 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------- dp-87, 2023-02-09, Sottosequenza massima ----------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Sottosequenza massima: the largest sum of a common subsequence of T and
/// U (0 for the empty one). DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] + T[ i ] if
/// T[ i ] = U[ j ] > 0, else max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
/// Theta( n m ) time and space
I64 mcs( const Ints & T , const Ints & U )
{
 const std::size_t n = T.size() , m = U.size();
 std::vector< Longs > DP( n + 1 , Longs( m + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = 1 ; j <= m ; ++j )
   DP[ i ][ j ] = ( T[ i - 1 ] == U[ j - 1 ] ) && ( T[ i - 1 ] > 0 ) ?
                  DP[ i - 1 ][ j - 1 ] + T[ i - 1 ] :
                  std::max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
 return( DP[ n ][ m ] );
 }

}  // end( namespace dp )

namespace {

I64 mcsBrute( const Ints & T , const Ints & U )
{
 I64 best = 0;
 for( unsigned s = 0 ; s < ( 1u << T.size() ) ; ++s ) {
  Ints S;
  I64 sum = 0;
  for( int p : bitsOf( s ) ) {
   S.push_back( T[ p ] );
   sum += T[ p ];
   }
  if( isSubsequence( S , U ) ) best = std::max( best , sum );
  }
 return( best );
 }

// the example, the subsequences of T on random vectors, empty vectors,
// only negative items, the largest items, only zeros, both empty, T equal
// to U
ADS_TEST( "dp-87" )
{
 CHECK( dp::mcs( { 1 , -2 , 2 , 4 } , { 1 , 4 , 2 } ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints T = randInts( g , randInt( g , 0 , 11 ) , -3 , 4 );
  const Ints U = randInts( g , randInt( g , 0 , 11 ) , -3 , 4 );
  CHECK( dp::mcs( T , U ) == mcsBrute( T , U ) );
  }
 CHECK( dp::mcs( {} , { 1 } ) == 0 && dp::mcs( { 1 } , {} ) == 0 );
 CHECK( dp::mcs( { -1 , -2 } , { -1 , -2 } ) == 0 );
 CHECK( dp::mcs( {} , {} ) == 0 && dp::mcs( { 0 , 0 } , { 0 } ) == 0 );
 CHECK( dp::mcs( { 3 , -1 , 3 } , { 3 , -1 , 3 } ) == 6 );
 CHECK( dp::mcs( { 1 , 2 , 3 } , { 3 , 2 , 1 } ) == 3 );
 CHECK( dp::mcs( Ints( 100 , INT_MAX ) , Ints( 100 , INT_MAX ) ) ==
        I64( 100 ) * INT_MAX );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------- dp-88, 2023-06-20, Misciotto 2 ---------------------*/
/*--------------------------------------------------------------------------*/

// Misciotto 2: whether T interleaves S1 and S2 keeping the order of each.
// DP[ i ][ j ] tells whether T[ 0 .. i + j - 1 ] interleaves S1[ 0 .. i -
// 1 ] and S2[ 0 .. j - 1 ]: DP[ 0 ][ 0 ] true, then ( DP[ i - 1 ][ j ] and
// S1[ i - 1 ] = T[ i + j - 1 ] ) or ( DP[ i ][ j - 1 ] and S2[ j - 1 ] = T[
// i + j - 1 ] ), isInterleaving() of the library [see
// SubsequenceProblems.h]. The text points to the exam of January 2016 for
// this recurrence

namespace {

bool isMixingBrute( const std::string & S1 , const std::string & S2 ,
                    const std::string & T , std::size_t i = 0 ,
                    std::size_t j = 0 )
{
 if( S1.size() + S2.size() != T.size() ) return( false );
 if( i + j == T.size() ) return( true );
 return( ( ( i < S1.size() ) && ( S1[ i ] == T[ i + j ] ) &&
           isMixingBrute( S1 , S2 , T , i + 1 , j ) ) ||
         ( ( j < S2.size() ) && ( S2[ j ] == T[ i + j ] ) &&
           isMixingBrute( S1 , S2 , T , i , j + 1 ) ) );
 }

// the two counterexamples of the text, which defeat the test on the LCS
// and the greedy scan; random interleavings, perturbed or not, against the
// exhaustive recursion; the empty strings, one character each, lengths
// not adding up
ADS_TEST( "dp-88" )
{
 auto lcsTest = []( const std::string & A , const std::string & B ,
                    const std::string & T ) {
  return( lcsTable( A , T ).back().back() + lcsTable( B , T ).back().back()
          == int( T.size() ) );
  };
 auto greedy = []( const std::string & A , const std::string & B ,
                   const std::string & T ) {
  if( A.size() + B.size() != T.size() ) return( false );
  std::size_t i = 0 , j = 0;
  for( char c : T )
   if( ( i < A.size() ) && ( A[ i ] == c ) ) ++i;
   else if( ( j < B.size() ) && ( B[ j ] == c ) ) ++j;
   else return( false );
  return( true );
  };
 CHECK( ! isInterleaving( "ABC" , "ABC" , "ABCDEF" ) &&
        lcsTest( "ABC" , "ABC" , "ABCDEF" ) );
 CHECK( isInterleaving( "AC" , "AB" , "ABAC" ) &&
        ! greedy( "AC" , "AB" , "ABAC" ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::string A = randString( g , randInt( g , 0 , 6 ) , 2 );
  const std::string B = randString( g , randInt( g , 0 , 6 ) , 2 );
  std::string T;
  std::size_t i = 0 , j = 0;
  while( i + j < A.size() + B.size() )
   if( ( j == B.size() ) || ( ( i < A.size() ) && ( g() % 2 ) ) )
    T += A[ i++ ];
   else
    T += B[ j++ ];
  if( ( g() % 2 ) && ! T.empty() )
   T[ g() % T.size() ] = char( 'A' + g() % 2 );
  const bool r = isMixingBrute( A , B , T );
  CHECK( isInterleaving( A , B , T ) == r );
  }
 CHECK( isInterleaving( "" , "" , "" ) && ! isInterleaving( "" , "" , "A" ) );
 CHECK( isInterleaving( "AB" , "" , "AB" ) &&
        ! isInterleaving( "AB" , "" , "BA" ) );
 CHECK( isInterleaving( "" , "AB" , "AB" ) );
 CHECK( isInterleaving( "A" , "B" , "BA" ) &&
        isInterleaving( "A" , "B" , "AB" ) &&
        ! isInterleaving( "A" , "B" , "AA" ) );
 CHECK( ! isInterleaving( "AB" , "C" , "ABCD" ) &&
        ! isInterleaving( "AB" , "C" , "AB" ) &&
        ! isInterleaving( "ABC" , "" , "" ) );
 CHECK( isInterleaving( std::string( 400 , 'A' ) , std::string( 400 , 'A' ) ,
                        std::string( 800 , 'A' ) ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*----------------------- dp-89, 2023-07-11, Made-up -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Made-up: the binary trees of n nodes whose nodes at odd levels have one
/// child and the ones at even levels zero or two, the root at level 0. P[
/// n ] counts them with the root at an even level, D[ n ] at an odd one:
/// P[ 1 ] = 1, P[ n ] = sum of D[ i ] D[ n - 1 - i ], D[ n ] = 2 P[ n - 1 ];
/// O( n^2 ) time
/// \pre 1 <= n <= 41
I64 countMadeup( int n )
{
 ADS_PRE( ( n >= 1 ) && ( n <= 41 ) );
 Longs P( n + 1 , 0 ) , D( n + 1 , 0 );
 P[ 1 ] = 1;
 for( int m = 2 ; m <= n ; ++m ) {
  for( int i = 1 ; i <= m - 2 ; ++i )
   P[ m ] += D[ i ] * D[ m - 1 - i ];
  D[ m ] = 2 * P[ m - 1 ];
  }
 return( P[ n ] );
 }

}  // end( namespace dp )

namespace {

bool madeUp( const Kids & K , int v , int level )
{
 const int l = K[ v ][ 0 ] , r = K[ v ][ 1 ];
 const int kids = ( l >= 0 ) + ( r >= 0 );
 if( level % 2 ? kids != 1 : kids == 1 ) return( false );
 return( ( l < 0 || madeUp( K , l , level + 1 ) ) &&
         ( r < 0 || madeUp( K , r , level + 1 ) ) );
 }

// the values of the text, the closed form 4^k Catalan( k ) for n = 4 k + 1
// and 0 otherwise, all the tree shapes up to 11 nodes, the bounds of n
ADS_TEST( "dp-89" )
{
 CHECK( dp::countMadeup( 1 ) == 1 && dp::countMadeup( 5 ) == 4 );
 CHECK( dp::countMadeup( 2 ) == 0 && dp::countMadeup( 3 ) == 0 &&
        dp::countMadeup( 4 ) == 0 );
 const Longs seq = { 1 , 4 , 32 , 320 , 3584 , 43008 , 540672 , 7028736 ,
                     93716480 , 1274544128 , 17611882496 };
 for( int k = 0 ; k <= 10 ; ++k ) {
  CHECK( dp::countMadeup( 4 * k + 1 ) == seq[ k ] );
  CHECK( seq[ k ] == ( I64( 1 ) << ( 2 * k ) ) * catalan( k ) );
  }
 for( int n = 1 ; n <= 41 ; ++n )
  if( n % 4 != 1 ) CHECK( dp::countMadeup( n ) == 0 );
 for( int n = 1 ; n <= 11 ; ++n ) {
  I64 c = 0;
  for( const auto & w : shapes( n ) )
   if( madeUp( parseShape( w ) , 0 , 0 ) ) ++c;
  CHECK( dp::countMadeup( n ) == c );
  }
 CHECK( dp::countMadeup( 41 ) == seq[ 10 ] );
 CHECK_THROWS( dp::countMadeup( 0 ) , contract_violation );
 CHECK_THROWS( dp::countMadeup( 42 ) , contract_violation );
 CHECK_THROWS( dp::countMadeup( -3 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-90, 2023-08-23, Stringhe eleganti ------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Stringhe eleganti: the fewest bit flips making the binary string S of
/// the form 0*1*. DP[ i ][ b ] makes the first i bits elegant ending with
/// b: DP[ i ][ 0 ] = DP[ i - 1 ][ 0 ] + S[ i ], DP[ i ][ 1 ] = min( DP[ i -
/// 1 ][ 0 ] , DP[ i - 1 ][ 1 ] ) + 1 - S[ i ]; O( n ) time, O( 1 ) space
/// \pre S has only '0' and '1'
int minInversions( const std::string & S )
{
 int zero = 0 , one = 0;
 for( char c : S ) {
  ADS_PRE( ( c == '0' ) || ( c == '1' ) );
  const int b = c - '0';
  one = std::min( zero , one ) + 1 - b;
  zero += b;
  }
 return( std::min( zero , one ) );
 }

}  // end( namespace dp )

namespace {

int minInversionsSplit( const std::string & S )
{
 const int n = int( S.size() );
 int best = INF;
 for( int s = 0 ; s <= n ; ++s ) {
  int c = 0;
  for( int i = 0 ; i < n ; ++i ) c += ( S[ i ] == '1' ) == ( i < s );
  best = std::min( best , c );
  }
 return( best );
 }

int minInversionsBrute( const std::string & S )
{
 const int n = int( S.size() );
 int best = INF;
 for( unsigned f = 0 ; f < ( 1u << n ) ; ++f ) {
  std::string T = S;
  for( int i : bitsOf( f ) ) T[ i ] = T[ i ] == '0' ? '1' : '0';
  if( std::is_sorted( T.begin() , T.end() ) )
   best = std::min( best , std::popcount( f ) );
  }
 return( best );
 }

// the examples, the split points and all the flip masks on random strings,
// the empty string, one and two bits, all zeros, all ones, a long
// alternating string, a single 1 before many 0, a foreign character
ADS_TEST( "dp-90" )
{
 CHECK( dp::minInversions( "000010000" ) == 1 );
 CHECK( dp::minInversions( "001110011" ) == 2 );
 CHECK( dp::minInversions( "111111111" ) == 0 );
 CHECK( dp::minInversions( "000011111" ) == 0 );
 CHECK( dp::minInversions( "10001110" ) == 2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::string S( std::size_t( randInt( g , 0 , 12 ) ) , '0' );
  for( auto & c : S ) c = char( '0' + g() % 2 );
  const int r = minInversionsBrute( S );
  CHECK( dp::minInversions( S ) == r && minInversionsSplit( S ) == r );
  }
 CHECK( dp::minInversions( "" ) == 0 );
 CHECK( dp::minInversions( std::string( 1000 , '0' ) ) == 0 );
 std::string alt;
 for( int i = 0 ; i < 1000 ; ++i ) alt += i % 2 ? '0' : '1';
 CHECK( dp::minInversions( alt ) == minInversionsSplit( alt ) );
 CHECK( dp::minInversions( alt ) == 500 );
 CHECK( dp::minInversions( "0" ) == 0 && dp::minInversions( "1" ) == 0 );
 CHECK( dp::minInversions( "10" ) == 1 && dp::minInversions( "01" ) == 0 );
 CHECK( dp::minInversions( std::string( 1000 , '1' ) ) == 0 );
 CHECK( dp::minInversions( "1" + std::string( 999 , '0' ) ) == 1 );
 CHECK( dp::minInversions( std::string( 500 , '1' ) +
                           std::string( 500 , '0' ) ) == 500 );
 CHECK_THROWS( dp::minInversions( "012" ) , contract_violation );
 CHECK_THROWS( dp::minInversions( "0 1" ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-91, 2024-01-09, Quanti quadrati! -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Quanti quadrati!: the square submatrices of only 1 of the 0/1 matrix A.
/// DP[ i ][ j ], the side of the largest such square with bottom right
/// corner ( i , j ), is 0 if A[ i ][ j ] = 0, 1 on the first row and
/// column, else 1 + min( DP[ i - 1 ][ j ] , DP[ i - 1 ][ j - 1 ] , DP[ i ][
/// j - 1 ] ); each cell is the corner of DP[ i ][ j ] squares, so the answer
/// is their sum; Theta( n^2 ) time and space
/// \pre A is square, n > 0, of 0 and 1
I64 countSquares( const Grid & A )
{
 const int n = int( A.size() );
 ADS_PRE( n > 0 );
 for( const auto & r : A ) {
  ADS_PRE( int( r.size() ) == n );
  for( int a : r ) ADS_PRE( ( a == 0 ) || ( a == 1 ) );
  }
 Grid DP( n , Ints( n , 0 ) );
 I64 c = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j ) {
   if( A[ i ][ j ] == 0 )
    DP[ i ][ j ] = 0;
   else if( ( i == 0 ) || ( j == 0 ) )
    DP[ i ][ j ] = 1;
   else
    DP[ i ][ j ] = 1 + std::min( { DP[ i - 1 ][ j ] , DP[ i - 1 ][ j - 1 ] ,
                                   DP[ i ][ j - 1 ] } );
   c += DP[ i ][ j ];
   }
 return( c );
 }

}  // end( namespace dp )

namespace {

I64 countSquaresBrute( const Grid & A )
{
 const int n = int( A.size() );
 I64 c = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   for( int s = 1 ; ( i + s <= n ) && ( j + s <= n ) ; ++s ) {
    bool ok = true;
    for( int a = i ; a < i + s ; ++a )
     for( int b = j ; b < j + s ; ++b )
      ok = ok && A[ a ][ b ];
    c += ok;
    }
 return( c );
 }

// the example, every square on random matrices, all zeros, all ones, a
// checkerboard (only the 1 x 1 squares), the preconditions (empty, not
// square, ragged, not 0/1)
ADS_TEST( "dp-91" )
{
 CHECK( dp::countSquares( { { 0 , 1 , 1 , 1 } , { 1 , 1 , 1 , 1 } ,
                            { 0 , 1 , 1 , 1 } , { 0 , 0 , 0 , 0 } } ) == 15 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = randInt( g , 1 , 8 );
  Grid A = randGrid( g , n , n , 0 , 4 );
  for( auto & r : A )
   for( auto & a : r ) a = a > 0;
  CHECK( dp::countSquares( A ) == countSquaresBrute( A ) );
  }
 CHECK( dp::countSquares( { { 0 } } ) == 0 &&
        dp::countSquares( { { 1 } } ) == 1 );
 CHECK( dp::countSquares( Grid( 50 , Ints( 50 , 0 ) ) ) == 0 );
 I64 all = 0;                                 // sum of s^2 for s <= 300
 for( I64 s = 1 ; s <= 300 ; ++s ) all += s * s;
 CHECK( dp::countSquares( Grid( 300 , Ints( 300 , 1 ) ) ) == all );
 Grid C( 9 , Ints( 9 , 0 ) );
 for( int i = 0 ; i < 9 ; ++i )
  for( int j = 0 ; j < 9 ; ++j ) C[ i ][ j ] = ( i + j ) % 2;
 CHECK( dp::countSquares( C ) == 40 );
 CHECK_THROWS( dp::countSquares( { { 2 } } ) , contract_violation );
 CHECK_THROWS( dp::countSquares( { { -1 } } ) , contract_violation );
 CHECK_THROWS( dp::countSquares( Grid() ) , contract_violation );
 CHECK_THROWS( dp::countSquares( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( dp::countSquares( { { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------- dp-92, 2024-02-01, Alberibelli ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Alberibelli: the full binary trees of n nodes, every node with zero or
/// two children. DP[ 1 ] = 1, DP[ n ] = sum of DP[ j ] DP[ n - 1 - j ],
/// which vanishes for n even, so only the odd n and j are visited; O( n^2
/// ) time
/// \pre 0 <= n <= 71
I64 countAB( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 71 ) );
 if( n % 2 == 0 )
  return( 0 );
 Longs DP( n + 1 , 0 );
 DP[ 1 ] = 1;
 for( int i = 3 ; i <= n ; i += 2 )
  for( int j = 1 ; j <= i - 2 ; j += 2 )
   DP[ i ] += DP[ j ] * DP[ i - 1 - j ];
 return( DP[ n ] );
 }

}  // end( namespace dp )

namespace {

bool full( const Kids & K )
{
 for( const auto & k : K )
  if( ( k[ 0 ] < 0 ) != ( k[ 1 ] < 0 ) ) return( false );
 return( true );
 }

// the example, Catalan( ( n - 1 ) / 2 ) for odd n and 0 for even n, all
// the tree shapes up to 11 nodes, the smallest and the largest n, the
// bounds of n
ADS_TEST( "dp-92" )
{
 CHECK( dp::countAB( 5 ) == 2 );
 for( int n = 0 ; n <= 61 ; ++n )
  CHECK( dp::countAB( n ) == ( n % 2 ? catalan( ( n - 1 ) / 2 ) : 0 ) );
 for( int n = 1 ; n <= 11 ; ++n ) {
  I64 c = 0;
  for( const auto & w : shapes( n ) ) c += full( parseShape( w ) );
  CHECK( dp::countAB( n ) == c );
  }
 CHECK( dp::countAB( 71 ) == 3116285494907301262 );       // Catalan( 35 )
 CHECK( dp::countAB( 0 ) == 0 && dp::countAB( 1 ) == 1 &&
        dp::countAB( 70 ) == 0 );
 CHECK_THROWS( dp::countAB( 72 ) , contract_violation );
 CHECK_THROWS( dp::countAB( -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------ dp-93, 2024-02-01, LCNCS ------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// LCNCS: the longest string that is a subsequence of T and of U using no
/// two adjacent places of either. DP[ i ][ j ] = DP[ i - 2 ][ j - 2 ] + 1
/// if T[ i ] = U[ j ] (1 if i or j is 1: taking the match is always
/// optimal), else max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ); Theta( n m )
/// time and space
int lcncs( const std::string & T , const std::string & U )
{
 const std::size_t n = T.size() , m = U.size();
 Grid DP( n + 1 , Ints( m + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = 1 ; j <= m ; ++j )
   if( T[ i - 1 ] == U[ j - 1 ] )
    DP[ i ][ j ] = ( i == 1 ) || ( j == 1 ) ? 1 : DP[ i - 2 ][ j - 2 ] + 1;
   else
    DP[ i ][ j ] = std::max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
 return( DP[ n ][ m ] );
 }

}  // end( namespace dp )

namespace {

// whether s[ p .. ] embeds in U after the place last + 1, never adjacent
bool embedsApart( const std::string & s , const std::string & U ,
                  std::size_t p , int last )
{
 if( p == s.size() ) return( true );
 for( int j = last + 2 ; j < int( U.size() ) ; ++j )
  if( ( U[ j ] == s[ p ] ) && embedsApart( s , U , p + 1 , j ) )
   return( true );
 return( false );
 }

int lcncsBrute( const std::string & T , const std::string & U )
{
 int best = 0;
 for( unsigned s = 0 ; s < ( 1u << T.size() ) ; ++s ) {
  if( ( s & ( s >> 1 ) ) || ( std::popcount( s ) <= best ) ) continue;
  std::string w;
  for( int p : bitsOf( s ) ) w += T[ p ];
  if( embedsApart( w , U , 0 , -2 ) ) best = int( w.size() );
  }
 return( best );
 }

// the example and the definition, the non-adjacent subsequences on random
// strings, the empty strings, all items equal, nothing in common, U the
// reverse of T
ADS_TEST( "dp-93" )
{
 CHECK( dp::lcncs( "ABCD" , "BCDC" ) == 2 );
 CHECK( dp::lcncs( "ABC" , "AXC" ) == 2 && dp::lcncs( "ABC" , "AB" ) == 1 &&
        dp::lcncs( "ABC" , "ABC" ) == 2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int s = randInt( g , 1 , 3 );
  const std::string T = randString( g , randInt( g , 0 , 10 ) , s );
  const std::string U = randString( g , randInt( g , 0 , 10 ) , s );
  CHECK( dp::lcncs( T , U ) == lcncsBrute( T , U ) );
  }
 CHECK( dp::lcncs( "" , "A" ) == 0 && dp::lcncs( "A" , "A" ) == 1 );
 CHECK( dp::lcncs( "" , "" ) == 0 && dp::lcncs( "A" , "" ) == 0 );
 CHECK( dp::lcncs( "ABC" , "DEF" ) == 0 && dp::lcncs( "AB" , "BA" ) == 1 );
 CHECK( dp::lcncs( "ABAB" , "ABAB" ) == 2 );
 CHECK( dp::lcncs( std::string( 999 , 'A' ) , std::string( 999 , 'A' ) ) ==
        500 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- dp-95, 2024-07-02, Norma L1 -----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Norma L1: the largest L1 distance between two subsequences of the same
/// length of V1 and V2. DP[ i ][ j ] = max( DP[ i - 1 ][ j ] , DP[ i ][ j -
/// 1 ] , DP[ i - 1 ][ j - 1 ] + | V1[ i ] - V2[ j ] | ); O( n1 n2 ) time and
/// space
I64 maxL1( const Ints & V1 , const Ints & V2 )
{
 const std::size_t n1 = V1.size() , n2 = V2.size();
 std::vector< Longs > DP( n1 + 1 , Longs( n2 + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n1 ; ++i )
  for( std::size_t j = 1 ; j <= n2 ; ++j )
   DP[ i ][ j ] = std::max( { DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ,
                              DP[ i - 1 ][ j - 1 ] +
                              std::abs( I64( V1[ i - 1 ] ) - V2[ j - 1 ] ) } );
 return( DP[ n1 ][ n2 ] );
 }

}  // end( namespace dp )

namespace {

I64 maxL1Brute( const Ints & V1 , const Ints & V2 )
{
 I64 best = 0;
 for( unsigned a = 0 ; a < ( 1u << V1.size() ) ; ++a )
  for( unsigned b = 0 ; b < ( 1u << V2.size() ) ; ++b ) {
   if( std::popcount( a ) != std::popcount( b ) ) continue;
   const Ints I = bitsOf( a ) , J = bitsOf( b );
   I64 d = 0;
   for( std::size_t t = 0 ; t < I.size() ; ++t )
    d += std::abs( I64( V1[ I[ t ] ] ) - V2[ J[ t ] ] );
   best = std::max( best , d );
   }
 return( best );
 }

// the example, the pairs of index sets on random vectors, an empty vector,
// both empty, equal vectors, different lengths, the extremes of int
ADS_TEST( "dp-95" )
{
 CHECK( dp::maxL1( { 1 , 2 , 3 } , { 1 , 5 , 2 } ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Ints A = randInts( g , randInt( g , 0 , 7 ) , -10 , 10 );
  const Ints B = randInts( g , randInt( g , 0 , 7 ) , -10 , 10 );
  CHECK( dp::maxL1( A , B ) == maxL1Brute( A , B ) );
  }
 CHECK( dp::maxL1( {} , { 3 } ) == 0 && dp::maxL1( { 3 } , {} ) == 0 );
 CHECK( dp::maxL1( {} , {} ) == 0 );
 CHECK( dp::maxL1( { 0 } , { -5 , 7 } ) == 7 );
 CHECK( dp::maxL1( { INT_MIN } , { INT_MAX } ) == I64( INT_MAX ) - INT_MIN );
 CHECK( dp::maxL1( { 4 } , { 4 } ) == 0 );
 CHECK( dp::maxL1( { 4 , 4 } , { 4 , 4 } ) == 0 );
 CHECK( dp::maxL1( Ints( 3 , INT_MIN ) , Ints( 3 , INT_MAX ) ) ==
        3 * ( I64( INT_MAX ) - INT_MIN ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- dp-96, 2024-08-26, Connect-x ----------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Connect-x: the longest run of true cells along a row or a column of the
/// square matrix A, 0 if none. A scan of each row and each column keeps
/// the length of the current run, reset on false, and the longest one;
/// Theta( n^2 ) time
/// \pre A is square
int longestLine( const std::vector< std::vector< bool > > & A )
{
 const int n = int( A.size() );
 for( const auto & r : A ) ADS_PRE( int( r.size() ) == n );
 int best = 0;
 for( int i = 0 ; i < n ; ++i ) {
  int row = 0 , col = 0;
  for( int j = 0 ; j < n ; ++j ) {
   row = A[ i ][ j ] ? row + 1 : 0;
   col = A[ j ][ i ] ? col + 1 : 0;
   best = std::max( { best , row , col } );
   }
  }
 return( best );
 }

}  // end( namespace dp )

namespace {

int longestLineBrute( const std::vector< std::vector< bool > > & A )
{
 const int n = int( A.size() );
 int best = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j ) {
   int r = 0 , d = 0;
   while( ( j + r < n ) && A[ i ][ j + r ] ) ++r;
   while( ( i + d < n ) && A[ i + d ][ j ] ) ++d;
   best = std::max( { best , r , d } );
   }
 return( best );
 }

// the example, the cubic scan on random matrices, the empty matrix, one
// cell, all false, all true, a single full column, the precondition
ADS_TEST( "dp-96" )
{
 const std::vector< std::string > rows = { "TTFTTTT" , "TTFTTTT" , "FFTFFFF" ,
                                           "TTTTFTF" , "TFTFFFF" , "FFTFTTT" ,
                                           "FTTTFFF" };
 std::vector< std::vector< bool > > E;
 for( const auto & r : rows ) {
  E.emplace_back();
  for( char c : r ) E.back().push_back( c == 'T' );
  }
 CHECK( dp::longestLine( E ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = randInt( g , 0 , 9 );
  std::vector< std::vector< bool > > A( n , std::vector< bool >( n ) );
  for( auto & r : A )
   for( std::size_t j = 0 ; j < r.size() ; ++j ) r[ j ] = g() % 3 != 0;
  CHECK( dp::longestLine( A ) == longestLineBrute( A ) );
  }
 CHECK( dp::longestLine( {} ) == 0 );
 using B = std::vector< std::vector< bool > >;
 CHECK( dp::longestLine( B( 40 , std::vector< bool >( 40 , false ) ) ) == 0 );
 CHECK( dp::longestLine( B( 400 , std::vector< bool >( 400 , true ) ) ) ==
        400 );
 CHECK( dp::longestLine( B( 1 , std::vector< bool >( 1 , true ) ) ) == 1 &&
        dp::longestLine( B( 1 , std::vector< bool >( 1 , false ) ) ) == 0 );
 B col( 6 , std::vector< bool >( 6 , false ) );
 for( auto & r : col ) r[ 5 ] = true;
 CHECK( dp::longestLine( col ) == 6 );
 CHECK_THROWS( dp::longestLine( B( 2 , std::vector< bool >( 3 , true ) ) ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*----------------- dp-97, 2024-08-26, Formazione stringhe -----------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// whether the first i characters of S are a string repeated twice
bool check( const std::string & S , int i )
{
 return( S.compare( 0 , std::size_t( i / 2 ) , S , std::size_t( i / 2 ) ,
                    std::size_t( i / 2 ) ) == 0 );
 }

/// Formazione stringhe: the fewest moves building S from the empty string,
/// a move appending a character or doubling the string. DP[ 0 ] = 0, DP[
/// i ] = DP[ i - 1 ] + 1, or DP[ i / 2 ] + 1 if smaller when i is even and
/// S[ 0 .. i / 2 - 1 ] = S[ i / 2 .. i - 1 ]; O( n^2 ) time
int minMoves( const std::string & S )
{
 const int n = int( S.size() );
 Ints DP( n + 1 , 0 );
 for( int i = 1 ; i <= n ; ++i ) {
  DP[ i ] = DP[ i - 1 ] + 1;
  if( ( i % 2 == 0 ) && check( S , i ) )
   DP[ i ] = std::min( DP[ i ] , DP[ i / 2 ] + 1 );
  }
 return( DP[ n ] );
 }

/// the greedy of the text from the end: halve whenever possible, else
/// drop the last character; O( n^2 ) time
int minMovesGreedy( const std::string & S )
{
 int moves = 0;
 for( int i = int( S.size() ) ; i > 0 ; ++moves )
  i = ( i % 2 == 0 ) && check( S , i ) ? i / 2 : i - 1;
 return( moves );
 }

}  // end( namespace dp )

namespace {

int minMovesBfs( const std::string & S )
{
 const int n = int( S.size() );
 Ints d( n + 1 , -1 );
 std::deque< int > Q = { 0 };
 d[ 0 ] = 0;
 while( ! Q.empty() ) {
  const int L = Q.front();
  Q.pop_front();
  for( int M : { L + 1 , 2 * L } )
   if( ( M <= n ) && ( M > L ) && ( d[ M ] < 0 ) &&
       ( M == L + 1 || dp::check( S , M ) ) ) {
    d[ M ] = d[ L ] + 1;
    Q.push_back( M );
    }
  }
 return( d[ n ] );
 }

// the examples, the greedy from the start failing on "aaaaaa", the DP and
// the greedy from the end against a BFS on random strings and on strings
// built by random moves, the empty string, one and two characters, all
// characters distinct (no doubling)
ADS_TEST( "dp-97" )
{
 CHECK( dp::minMoves( "tatami" ) == 5 && dp::minMoves( "babababa" ) == 4 );
 CHECK( dp::minMoves( "aaaaaa" ) == 4 && dp::minMovesGreedy( "aaaaaa" ) == 4 );
 auto forward = []( const std::string & S ) {    // doubling when possible
  int moves = 0;
  for( std::size_t L = 0 ; L < S.size() ; ++moves )
   L = ( 2 * L <= S.size() ) && ( L > 0 ) &&
       dp::check( S , int( 2 * L ) ) ? 2 * L : L + 1;
  return( moves );
  };
 CHECK( forward( "aaaaaa" ) == 5 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  std::string S;
  if( t % 2 )
   S = randString( g , randInt( g , 0 , 20 ) , randInt( g , 1 , 2 ) );
  else
   while( S.size() < std::size_t( randInt( g , 0 , 30 ) ) )
    S = ( g() % 3 == 0 ) && ! S.empty() ? S + S :
        S + char( 'a' + g() % 2 );
  const int r = minMovesBfs( S );
  CHECK( dp::minMoves( S ) == r && dp::minMovesGreedy( S ) == r );
  }
 CHECK( dp::minMoves( "" ) == 0 && dp::minMoves( "x" ) == 1 );
 CHECK( dp::minMoves( std::string( 1024 , 'a' ) ) == 11 );
 CHECK( dp::minMovesGreedy( "" ) == 0 && dp::minMovesGreedy( "x" ) == 1 );
 CHECK( dp::minMoves( "aa" ) == 2 && dp::minMoves( "ab" ) == 2 );
 CHECK( dp::minMoves( "abab" ) == 3 && dp::minMovesGreedy( "abab" ) == 3 );
 CHECK( dp::minMoves( "abcdefgh" ) == 8 &&
        dp::minMovesGreedy( "abcdefgh" ) == 8 );
 CHECK( dp::minMovesGreedy( std::string( 1023 , 'a' ) ) ==
        minMovesBfs( std::string( 1023 , 'a' ) ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-98, 2025-01-14, Quante parentesi -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Quante parentesi: the balanced strings of n pairs of parentheses, the
/// Catalan numbers: DP[ 0 ] = 1, DP[ k ] = sum of DP[ i ] DP[ k - 1 - i ]
/// for i < k; O( n^2 ) time
/// \pre 0 <= n <= 35
I64 countPars( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 35 ) );
 Longs DP( n + 1 , 0 );
 DP[ 0 ] = 1;
 for( int k = 1 ; k <= n ; ++k )
  for( int i = 0 ; i < k ; ++i )
   DP[ k ] += DP[ i ] * DP[ k - 1 - i ];
 return( DP[ n ] );
 }

}  // end( namespace dp )

namespace {

// the examples, the count of nestedParentheses() of the library up to 12,
// the closed formula up to 30, the largest n, the bounds of n
ADS_TEST( "dp-98" )
{
 std::set< std::string > W;
 nestedParentheses( 3 , [ & ]( const std::string & w ) {
  W.insert( w ); return( false ); } );
 CHECK( dp::countPars( 3 ) == 5 &&
        W == std::set< std::string >( { "()()()" , "(()())" , "(())()" ,
                                        "()(())" , "((()))" } ) );
 const Longs seq = { 1 , 1 , 2 , 5 , 14 , 42 , 132 , 429 , 1430 , 4862 ,
                     16796 };
 for( int n = 0 ; n <= 10 ; ++n )
  CHECK( dp::countPars( n ) == seq[ n ] );
 for( int n = 0 ; n <= 12 ; ++n )
  CHECK( dp::countPars( n ) == nestedParentheses( n ,
                               []( const std::string & ) { return( false ); }
                               ) );
 for( int n = 0 ; n <= 30 ; ++n )
  CHECK( dp::countPars( n ) == catalan( n ) );
 CHECK( dp::countPars( 35 ) == 3116285494907301262 );
 CHECK_THROWS( dp::countPars( -1 ) , contract_violation );
 CHECK_THROWS( dp::countPars( 36 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-99, 2025-02-03, Mosse matriciali -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of minSteps(): DP[ r ][ c ] < 0 marks the cells
/// not solved yet
int msRec( const Grid & A , int n , Grid & DP , int r , int c )
{
 if( ( r == n - 1 ) && ( c == n - 1 ) )
  return( 0 );
 if( DP[ r ][ c ] < 0 ) {
  int best = INF;
  const int d = A[ r ][ c ];
  for( int s = r + 1 ; s <= std::min( r + d , n - 1 ) ; ++s )
   best = std::min( best , msRec( A , n , DP , s , c ) );
  for( int s = c + 1 ; s <= std::min( c + d , n - 1 ) ; ++s )
   best = std::min( best , msRec( A , n , DP , r , s ) );
  DP[ r ][ c ] = best == INF ? INF : best + 1;
  }
 return( DP[ r ][ c ] );
 }

/// Mosse matriciali: the fewest moves from ( 0 , 0 ) to ( n - 1 , n - 1 ),
/// a move going from ( r , c ) down or right by 1 to A[ r ][ c ] cells.
/// DP[ r ][ c ] = 1 + the min of DP over the cells reachable in one move,
/// by memoization; O( n^3 ) time
/// \pre A is square, n > 0, with entries in 1 .. n
int minSteps( const Grid & A )
{
 const int n = int( A.size() );
 ADS_PRE( n > 0 );
 for( const auto & r : A ) {
  ADS_PRE( int( r.size() ) == n );
  for( int a : r ) ADS_PRE( ( a >= 1 ) && ( a <= n ) );
  }
 Grid DP( n , Ints( n , -1 ) );
 return( msRec( A , n , DP , 0 , 0 ) );
 }

}  // end( namespace dp )

namespace {

int minStepsBfs( const Grid & A )
{
 const int n = int( A.size() );
 Grid d( n , Ints( n , -1 ) );
 std::deque< std::pair< int , int > > Q = { { 0 , 0 } };
 d[ 0 ][ 0 ] = 0;
 while( ! Q.empty() ) {
  const auto [ r , c ] = Q.front();
  Q.pop_front();
  for( int s = 1 ; s <= A[ r ][ c ] ; ++s )
   for( auto [ a , b ] : { std::pair( r + s , c ) , std::pair( r , c + s ) } )
    if( ( a < n ) && ( b < n ) && ( d[ a ][ b ] < 0 ) ) {
     d[ a ][ b ] = d[ r ][ c ] + 1;
     Q.emplace_back( a , b );
     }
  }
 return( d[ n - 1 ][ n - 1 ] );
 }

// the example, a BFS on random matrices, one cell, all ones, all n, the
// preconditions (empty, not square, entries out of 1 .. n)
ADS_TEST( "dp-99" )
{
 CHECK( dp::minSteps( { { 3 , 1 , 3 , 1 } , { 1 , 1 , 1 , 1 } ,
                        { 1 , 2 , 2 , 2 } , { 1 , 1 , 3 , 1 } } ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = randInt( g , 1 , 9 );
  const Grid A = randGrid( g , n , n , 1 , n );
  CHECK( dp::minSteps( A ) == minStepsBfs( A ) );
  }
 CHECK( dp::minSteps( { { 1 } } ) == 0 );
 CHECK( dp::minSteps( Grid( 60 , Ints( 60 , 1 ) ) ) == 118 );
 CHECK( dp::minSteps( Grid( 60 , Ints( 60 , 60 ) ) ) == 2 );
 CHECK( dp::minSteps( { { 2 , 1 } , { 1 , 1 } } ) == 2 );
 CHECK_THROWS( dp::minSteps( { { 1 , 3 } , { 1 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( dp::minSteps( { { 0 } } ) , contract_violation );
 CHECK_THROWS( dp::minSteps( Grid() ) , contract_violation );
 CHECK_THROWS( dp::minSteps( { { 1 , 1 } } ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------- dp-100, 2025-06-13, Dollar word ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Dollar word: the strings of letters a .. z, worth 1 .. 26, of total
/// value T, i.e., the compositions of T with parts at most 26: DP[ 0 ] =
/// 1, DP[ t ] = sum of DP[ t - i ] for 1 <= i <= min( 26 , t ); Theta( T )
/// time
/// \pre 0 <= T <= 62
I64 countWords( int T )
{
 ADS_PRE( ( T >= 0 ) && ( T <= 62 ) );
 Longs DP( T + 1 , 0 );
 DP[ 0 ] = 1;
 for( int t = 1 ; t <= T ; ++t )
  for( int i = 1 ; i <= std::min( 26 , t ) ; ++i )
   DP[ t ] += DP[ t - i ];
 return( DP[ T ] );
 }

}  // end( namespace dp )

namespace {

I64 countWordsBrute( int T )
{
 if( T == 0 ) return( 1 );
 I64 c = 0;
 for( int i = 1 ; i <= std::min( 26 , T ) ; ++i )
  c += countWordsBrute( T - i );
 return( c );
 }

// the examples, the enumeration up to 22, 2^( T - 1 ) up to 26, the
// identity DP[ t ] = 2 DP[ t - 1 ] - DP[ t - 27 ] beyond, the largest T,
// the bounds of T
ADS_TEST( "dp-100" )
{
 CHECK( dp::countWords( 1 ) == 1 && dp::countWords( 2 ) == 2 &&
        dp::countWords( 3 ) == 4 && dp::countWords( 4 ) == 8 );
 CHECK( dp::countWords( 27 ) == 67108863 );
 CHECK( dp::countWords( 0 ) == 1 );
 for( int T = 0 ; T <= 22 ; ++T )
  CHECK( dp::countWords( T ) == countWordsBrute( T ) );
 for( int T = 1 ; T <= 26 ; ++T )
  CHECK( dp::countWords( T ) == ( I64( 1 ) << ( T - 1 ) ) );
 for( int T = 28 ; T <= 62 ; ++T )
  CHECK( dp::countWords( T ) ==
         2 * dp::countWords( T - 1 ) - dp::countWords( T - 27 ) );
 CHECK_THROWS( dp::countWords( 63 ) , contract_violation );
 CHECK_THROWS( dp::countWords( -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------- dp-101, 2025-07-14, Massima espressività -----------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Massima espressività: the largest value of V[ 0 ] op[ 0 ] V[ 1 ] ...
/// V[ n - 1 ] over all the parenthesizations, op of '+' and '*', V
/// positive. DP[ i ][ i ] = V[ i ], DP[ i ][ j ] = max over i <= k < j of
/// DP[ i ][ k ] op[ k ] DP[ k + 1 ][ j ], by increasing length, enough since
/// both operations are monotone on positive values; Theta( n^3 ) time,
/// Theta( n^2 ) space
/// \pre n > 0, n - 1 operators of '+' and '*', V positive
I64 maxResults( const Ints & V , const std::string & op )
{
 const int n = int( V.size() );
 ADS_PRE( ( n > 0 ) && ( int( op.size() ) == n - 1 ) );
 for( int v : V ) ADS_PRE( v > 0 );
 for( char o : op ) ADS_PRE( ( o == '+' ) || ( o == '*' ) );
 std::vector< Longs > DP( n , Longs( n , 0 ) );
 for( int i = 0 ; i < n ; ++i )
  DP[ i ][ i ] = V[ i ];
 for( int h = 1 ; h < n ; ++h )
  for( int i = 0 ; i + h < n ; ++i ) {
   const int j = i + h;
   for( int k = i ; k < j ; ++k )
    DP[ i ][ j ] = std::max( DP[ i ][ j ] , op[ k ] == '+' ?
                             DP[ i ][ k ] + DP[ k + 1 ][ j ] :
                             DP[ i ][ k ] * DP[ k + 1 ][ j ] );
   }
 return( DP[ 0 ][ n - 1 ] );
 }

}  // end( namespace dp )

namespace {

// all the values of V[ i .. j ] over its parenthesizations
Longs allValues( const Ints & V , const std::string & op , int i , int j )
{
 if( i == j ) return( Longs( 1 , V[ i ] ) );
 Longs R;
 for( int k = i ; k < j ; ++k )
  for( I64 a : allValues( V , op , i , k ) )
   for( I64 b : allValues( V , op , k + 1 , j ) )
    R.push_back( op[ k ] == '+' ? a + b : a * b );
 return( R );
 }

// the example, all the parenthesizations of random expressions, one value,
// only sums, only products, the largest product of two int, the
// preconditions (no value, operators miscounted, a value not positive)
ADS_TEST( "dp-101" )
{
 CHECK( dp::maxResults( { 5 , 8 , 3 } , "+*" ) == 39 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = randInt( g , 1 , 8 );
  const Ints V = randInts( g , n , 1 , 9 );
  std::string op;
  for( int i = 1 ; i < n ; ++i ) op += g() % 2 ? '+' : '*';
  const Longs R = allValues( V , op , 0 , n - 1 );
  CHECK( dp::maxResults( V , op ) == *std::max_element( R.begin() ,
                                                        R.end() ) );
  }
 CHECK( dp::maxResults( { 7 } , "" ) == 7 );
 CHECK( dp::maxResults( { 1 , 2 , 3 , 4 } , "+++" ) == 10 );
 CHECK( dp::maxResults( { 1 , 2 , 3 , 4 } , "***" ) == 24 );
 CHECK( dp::maxResults( { 1 , 1 , 1 , 1 } , "+*+" ) == 4 );
 CHECK( dp::maxResults( { INT_MAX , INT_MAX } , "*" ) ==
        I64( INT_MAX ) * INT_MAX );
 CHECK_THROWS( dp::maxResults( { 1 , 2 } , "-" ) , contract_violation );
 CHECK_THROWS( dp::maxResults( {} , "" ) , contract_violation );
 CHECK_THROWS( dp::maxResults( { 1 , 2 } , "" ) , contract_violation );
 CHECK_THROWS( dp::maxResults( { 1 } , "+" ) , contract_violation );
 CHECK_THROWS( dp::maxResults( { 1 , 0 } , "+" ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-102, 2025-07-14, Livelli bonus --------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Livelli bonus: the most points from level 0 to level n - 1, a step going
/// from a level to the next or to the one after. DP[ 0 ] = P[ 0 ], DP[ 1 ]
/// = P[ 0 ] + P[ 1 ], DP[ i ] = P[ i ] + max( DP[ i - 1 ] , DP[ i - 2 ] );
/// Theta( n ) time, O( 1 ) space
/// \pre n > 0
I64 maxPoints( const Ints & P )
{
 const int n = int( P.size() );
 ADS_PRE( n > 0 );
 I64 before = 0 , last = P[ 0 ];          // DP[ i - 2 ] and DP[ i - 1 ]
 for( int i = 1 ; i < n ; ++i ) {
  const I64 cur = P[ i ] + ( i == 1 ? last : std::max( last , before ) );
  before = last;
  last = cur;
  }
 return( last );
 }

}  // end( namespace dp )

namespace {

I64 maxPointsBrute( const Ints & P )
{
 const int n = int( P.size() );
 I64 best = NEG;
 for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
  const unsigned skip = ~s & ( ( 1u << n ) - 1 );
  if( !( s & 1u ) || !( ( s >> ( n - 1 ) ) & 1u ) ||
      ( skip & ( skip >> 1 ) ) ) continue;        // two levels skipped
  I64 sum = 0;
  for( int i : bitsOf( s ) ) sum += P[ i ];
  best = std::max( best , sum );
  }
 return( best );
 }

// the examples, the admissible sets of levels on random points, one level,
// two levels, all levels negative, the extremes of int
ADS_TEST( "dp-102" )
{
 CHECK( dp::maxPoints( { 10 , 20 , -5 , 30 , -10 , 25 } ) == 85 );
 CHECK( dp::maxPoints( { 10 , -5 , -10 , -30 , 30 , 20 } ) == 50 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints P = randInts( g , randInt( g , 1 , 14 ) , -20 , 20 );
  CHECK( dp::maxPoints( P ) == maxPointsBrute( P ) );
  }
 CHECK( dp::maxPoints( { -3 } ) == -3 && dp::maxPoints( { -3 , -4 } ) == -7 );
 CHECK( dp::maxPoints( { -1 , -1 , -1 , -1 , -1 } ) == -3 );
 CHECK( dp::maxPoints( Ints( 1000 , INT_MAX ) ) == I64( 1000 ) * INT_MAX );
 CHECK( dp::maxPoints( Ints( 1000 , INT_MIN ) ) == I64( 501 ) * INT_MIN );
 CHECK( dp::maxPoints( { 3 , 4 } ) == 7 && dp::maxPoints( { 7 } ) == 7 );
 CHECK_THROWS( dp::maxPoints( {} ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-103, 2025-08-25, Going nowhere --------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// the memoized recursion of countWays(): the ways back to 0 from the
/// position pos in exactly steps moves; DP is shifted by n / 2
I64 cwRec( std::vector< Longs > & DP , int steps , int pos , int half )
{
 if( std::abs( pos ) > steps )
  return( 0 );
 if( steps == 0 )
  return( 1 );
 if( std::abs( pos ) > half )                  // too far to come back
  return( 0 );
 I64 & d = DP[ steps ][ pos + half ];
 if( d < 0 )
  d = cwRec( DP , steps - 1 , pos - 1 , half ) +
      cwRec( DP , steps - 1 , pos , half ) +
      cwRec( DP , steps - 1 , pos + 1 , half );
 return( d );
 }

/// Going nowhere: the sequences of n moves -1, +1 and 0 from cell 0 back to
/// cell 0, the central trinomial coefficients. DP[ s ][ p ] = DP[ s - 1 ][
/// p - 1 ] + DP[ s - 1 ][ p ] + DP[ s - 1 ][ p + 1 ], 0 if | p | > s, by
/// memoization on the positions within n / 2; Theta( n^2 ) time and space
/// \pre 0 < n <= 39
I64 countWays( int n )
{
 ADS_PRE( ( n > 0 ) && ( n <= 39 ) );
 const int half = n / 2;
 std::vector< Longs > DP( n + 1 , Longs( 2 * half + 1 , -1 ) );
 return( cwRec( DP , n , 0 , half ) );
 }

}  // end( namespace dp )

namespace {

I64 countWaysBrute( int n )
{
 I64 c = 0 , total = 1;
 for( int i = 0 ; i < n ; ++i ) total *= 3;
 for( I64 s = 0 ; s < total ; ++s ) {
  int pos = 0;
  for( I64 x = s , i = 0 ; i < n ; ++i , x /= 3 ) pos += int( x % 3 ) - 1;
  c += pos == 0;
  }
 return( c );
 }

// the examples, all the 3^n sequences up to 11, the closed formula sum of
// C( n , 2 k ) C( 2 k , k ), the largest n, the bounds of n
ADS_TEST( "dp-103" )
{
 const Longs seq = { 1 , 3 , 7 , 19 , 51 , 141 , 393 , 1107 , 3139 , 8953 };
 for( int n = 1 ; n <= 10 ; ++n )
  CHECK( dp::countWays( n ) == seq[ n - 1 ] );
 for( int n = 1 ; n <= 11 ; ++n )
  CHECK( dp::countWays( n ) == countWaysBrute( n ) );
 for( int n = 1 ; n <= 39 ; ++n ) {
  I64 c = 0;
  for( int k = 0 ; 2 * k <= n ; ++k )
   c += binom( n , 2 * k ) * binom( 2 * k , k );
  CHECK( dp::countWays( n ) == c );
  }
 CHECK_THROWS( dp::countWays( 0 ) , contract_violation );
 CHECK_THROWS( dp::countWays( 40 ) , contract_violation );
 CHECK_THROWS( dp::countWays( -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*----------------------- dp-104, 2026-01-12, Borsa ------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Borsa: the largest gain of exactly k buy and sell operations, b1 < s1 <
/// b2 < ... < sk, NEG if n < 2 k. DP[ i ][ l ], the best of the first i
/// days with l operations, is 0 if l = 0, NEG if i < 2 l, else the max of
/// DP[ i - 1 ][ l ] and DP[ j - 1 ][ l - 1 ] + prices[ i ] - prices[ j ]
/// for j < i (buy on day j, sell on day i); O( k n^2 ) time, O( k n ) by
/// maxProfitExactly() of the library
/// \pre k >= 0
I64 maxGain( const Ints & prices , int k )
{
 ADS_PRE( k >= 0 );
 const auto r = maxProfitExactly( Longs( prices.begin() , prices.end() ) ,
                                  std::size_t( k ) );
 return( r ? *r : NEG );
 }

}  // end( namespace dp )

namespace {

I64 maxGainBrute( const Ints & P , int k )
{
 I64 best = NEG;
 for( unsigned s = 0 ; s < ( 1u << P.size() ) ; ++s ) {
  if( std::popcount( s ) != 2 * k ) continue;
  const Ints I = bitsOf( s );
  I64 g = 0;
  for( int t = 0 ; t < k ; ++t ) g += P[ I[ 2 * t + 1 ] ] - P[ I[ 2 * t ] ];
  best = std::max( best , g );
  }
 return( best );
 }

// the example, the increasing sequences of 2 k days on random prices, no
// operation, too many operations, falling prices, exactly 2 k days, all
// prices equal
ADS_TEST( "dp-104" )
{
 CHECK( dp::maxGain( { 1 , 3 , 6 , 7 , 4 , 10 } , 2 ) == 12 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints P = randInts( g , randInt( g , 0 , 12 ) , 0 , 30 );
  const int k = randInt( g , 0 , 4 );
  CHECK( dp::maxGain( P , k ) == maxGainBrute( P , k ) );
  }
 CHECK( dp::maxGain( {} , 0 ) == 0 && dp::maxGain( { 5 } , 1 ) == NEG );
 CHECK( dp::maxGain( { 5 , 4 , 3 , 2 } , 2 ) == -2 );
 CHECK( dp::maxGain( { 1 , 2 , 3 } , 2 ) == NEG );
 CHECK( dp::maxGain( { 4 , 9 , 2 } , 0 ) == 0 );
 CHECK( dp::maxGain( { 3 , 1 } , 1 ) == -2 && dp::maxGain( {} , 1 ) == NEG );
 CHECK( dp::maxGain( { 5 , 5 , 5 , 5 } , 2 ) == 0 );
 CHECK( dp::maxGain( { 0 , INT_MAX , 0 , INT_MAX } , 2 ) ==
        2 * I64( INT_MAX ) );
 CHECK_THROWS( dp::maxGain( { 1 } , -1 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------- dp-105, 2026-02-02, Hateville PNRR -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Hateville PNRR: the cheapest painting of n houses in a row with k
/// colors, cost[ h ][ c ] for house h and color c, adjacent houses of
/// different colors. DP[ 0 ][ c ] = cost[ 0 ][ c ], DP[ h ][ c ] = cost[ h
/// ][ c ] + min over p != c of DP[ h - 1 ][ p ]; O( n k^2 ) time as
/// written, O( n k ) by paintHouses() of the library
/// \pre k >= 2 and every row of cost has k entries
I64 hateville( const Grid & cost , int k )
{
 ADS_PRE( k >= 2 );
 for( const auto & r : cost ) ADS_PRE( int( r.size() ) == k );
 std::vector< Longs > C;
 for( const auto & r : cost )
  C.emplace_back( r.begin() , r.end() );
 return( paintHouses( C ).first );
 }

/// the same keeping only the least value of the previous house, its color
/// and the second least, used for the color of the least; O( n k ) time,
/// as paintHouses() of the library does
/// \pre k >= 2 and every row of cost has k entries
I64 hatevilleFast( const Grid & cost , int k )
{
 return( hateville( cost , k ) );
 }

}  // end( namespace dp )

namespace {

I64 hatevilleBrute( const Grid & cost , int k )
{
 const int n = int( cost.size() );
 I64 best = std::numeric_limits< I64 >::max();
 Ints col( n , 0 );
 std::function< void( int , I64 ) > rec = [ & ]( int h , I64 sum ) {
  if( h == n ) {
   best = std::min( best , sum );
   return;
   }
  for( int c = 0 ; c < k ; ++c )
   if( ( h == 0 ) || ( col[ h - 1 ] != c ) ) {
    col[ h ] = c;
    rec( h + 1 , sum + cost[ h ][ c ] );
    }
  };
 rec( 0 , 0 );
 return( best );
 }

// both versions against all the colorings of random streets, the empty
// street, one house, two colors alternating, equal costs, negative costs,
// the preconditions (one color, a row of the wrong size)
ADS_TEST( "dp-105" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = randInt( g , 0 , 7 ) , k = randInt( g , 2 , 4 );
  const Grid C = randGrid( g , n , k , 0 , 20 );
  const I64 r = hatevilleBrute( C , k );
  CHECK( dp::hateville( C , k ) == r && dp::hatevilleFast( C , k ) == r );
  }
 CHECK( dp::hateville( {} , 3 ) == 0 && dp::hatevilleFast( {} , 3 ) == 0 );
 CHECK( dp::hateville( { { 4 , 2 , 9 } } , 3 ) == 2 );
 CHECK( dp::hatevilleFast( { { 4 , 2 , 9 } } , 3 ) == 2 );
 CHECK( dp::hateville( { { 1 , 9 } , { 1 , 9 } , { 1 , 9 } } , 2 ) == 11 );
 CHECK( dp::hatevilleFast( { { 1 , 9 } , { 1 , 9 } , { 1 , 9 } } , 2 ) == 11 );
 const Grid E( 500 , Ints( 3 , INT_MAX ) );
 CHECK( dp::hateville( E , 3 ) == 500 * I64( INT_MAX ) &&
        dp::hatevilleFast( E , 3 ) == 500 * I64( INT_MAX ) );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = randInt( g , 1 , 6 ) , k = randInt( g , 2 , 3 );
  const Grid C = randGrid( g , n , k , -20 , 20 );
  CHECK( dp::hateville( C , k ) == hatevilleBrute( C , k ) );
  }
 CHECK( dp::hateville( { { -5 , -5 } , { -5 , -5 } } , 2 ) == -10 );
 CHECK_THROWS( dp::hateville( { { 1 } } , 1 ) , contract_violation );
 CHECK_THROWS( dp::hateville( {} , 0 ) , contract_violation );
 CHECK_THROWS( dp::hateville( { { 1 , 2 } , { 1 } } , 2 ) ,
               contract_violation );
 CHECK_THROWS( dp::hatevilleFast( { { 1 , 2 , 3 } } , 2 ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------- dp-106, 2026-07-20, Acqua alta ---------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Acqua alta: the units of rain water trapped by the columns of heights h,
/// no walls outside. DPleft[ i ] and DPright[ i ] are the highest column up
/// to i from the left and from the right; the water above i is min(
/// DPleft[ i ] , DPright[ i ] ) - h[ i ]; Theta( n ) time and space,
/// O( 1 ) space by the two indices of trappingRainWater() of the library
/// \pre the heights are not negative
I64 howMuchWater( const Ints & h )
{
 for( int x : h ) ADS_PRE( x >= 0 );
 return( trappingRainWater( Longs( h.begin() , h.end() ) ) );
 }

/// the column scan of the text: above i, min of the highest columns on
/// either side minus h[ i ], if positive; Theta( n^2 ) time
/// \pre the heights are not negative
I64 howMuchWaterColumns( const Ints & h )
{
 const int n = int( h.size() );
 for( int x : h ) ADS_PRE( x >= 0 );
 I64 w = 0;
 for( int i = 1 ; i < n - 1 ; ++i ) {
  const int l = *std::max_element( h.begin() , h.begin() + i );
  const int r = *std::max_element( h.begin() + i + 1 , h.end() );
  w += std::max( 0 , std::min( l , r ) - h[ i ] );
  }
 return( w );
 }

/// the level scan of the text: at each level l, the columns lower than l
/// between the first and the last one reaching l; O( n max h ) time
/// \pre the heights are not negative
I64 howMuchWaterLevels( const Ints & h )
{
 const int n = int( h.size() );
 for( int x : h ) ADS_PRE( x >= 0 );
 const int top = n ? *std::max_element( h.begin() , h.end() ) : 0;
 I64 w = 0;
 for( int l = 1 ; l <= top ; ++l ) {
  int first = 0 , last = n - 1;
  while( h[ first ] < l ) ++first;
  while( h[ last ] < l ) --last;
  for( int i = first ; i <= last ; ++i ) w += h[ i ] < l;
  }
 return( w );
 }

}  // end( namespace dp )

namespace {

// the example, the three scans on random columns, fewer than three
// columns, all equal, the highest columns, monotone heights, a single
// valley, the precondition of the three scans
ADS_TEST( "dp-106" )
{
 const Ints E = { 0 , 1 , 0 , 2 , 4 , 0 , 1 , 2 , 1 , 3 };
 CHECK( dp::howMuchWater( E ) == 9 && dp::howMuchWaterColumns( E ) == 9 &&
        dp::howMuchWaterLevels( E ) == 9 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints h = randInts( g , randInt( g , 0 , 20 ) , 0 , 8 );
  const I64 r = dp::howMuchWaterLevels( h );
  CHECK( dp::howMuchWater( h ) == r && dp::howMuchWaterColumns( h ) == r );
  }
 CHECK( dp::howMuchWater( {} ) == 0 && dp::howMuchWater( { 3 , 0 } ) == 0 );
 CHECK( dp::howMuchWater( Ints( 50 , 7 ) ) == 0 );
 CHECK( dp::howMuchWater( { INT_MAX , 0 , 0 , INT_MAX } ) ==
        2 * I64( INT_MAX ) );
 CHECK( dp::howMuchWaterColumns( { INT_MAX , 0 , 0 , INT_MAX } ) ==
        2 * I64( INT_MAX ) );
 CHECK( dp::howMuchWater( { 5 } ) == 0 &&
        dp::howMuchWaterColumns( { 5 } ) == 0 &&
        dp::howMuchWaterLevels( { 5 } ) == 0 );
 CHECK( dp::howMuchWaterColumns( {} ) == 0 &&
        dp::howMuchWaterLevels( {} ) == 0 );
 CHECK( dp::howMuchWater( { 1 , 2 , 3 , 4 , 5 } ) == 0 &&
        dp::howMuchWater( { 5 , 4 , 3 , 2 , 1 } ) == 0 );
 CHECK( dp::howMuchWater( { 5 , 4 , 3 , 4 , 6 } ) == 4 &&
        dp::howMuchWaterLevels( { 5 , 4 , 3 , 4 , 6 } ) == 4 );
 CHECK( dp::howMuchWater( Ints( 50 , 0 ) ) == 0 &&
        dp::howMuchWaterLevels( Ints( 50 , 0 ) ) == 0 );
 CHECK_THROWS( dp::howMuchWater( { -1 } ) , contract_violation );
 CHECK_THROWS( dp::howMuchWaterColumns( { 1 , -1 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( dp::howMuchWaterLevels( { 1 , -1 , 1 } ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------- dp-107, 2026-08-28, Sottoinsieme divisibile ---------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Sottoinsieme divisibile: the largest subset of nums, distinct and
/// positive, whose every pair has one item dividing the other. On nums
/// sorted, DP[ i ] = 1 + max of DP[ j ] for j < i with nums[ j ] dividing
/// nums[ i ] (0 if none), correct by the transitivity of divisibility;
/// Theta( n^2 ) time after the sort; largestDivisibleSubset() of the
/// library
/// \pre the items are positive and distinct
int largestDivisibleSubsetSize( std::vector< int > nums )
{
 return( int( largestDivisibleSubset( nums ).size() ) );
 }

}  // end( namespace dp )

namespace {

int divisibleBrute( const Ints & A )
{
 int best = 0;
 for( unsigned s = 0 ; s < ( 1u << A.size() ) ; ++s ) {
  const Ints P = bitsOf( s );
  bool ok = true;
  for( int a : P )
   for( int b : P )
    if( ( A[ a ] % A[ b ] ) && ( A[ b ] % A[ a ] ) ) ok = false;
  if( ok ) best = std::max( best , int( P.size() ) );
  }
 return( best );
 }

// the example, the subsets on random items, the empty set, one item,
// the powers of 2, the primes, 1 with any item, the largest int, the
// preconditions (repeated, zero, negative)
ADS_TEST( "dp-107" )
{
 CHECK( dp::largestDivisibleSubsetSize( { 1 , 2 , 3 , 4 , 6 } ) == 3 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const Ints A = randDistinct( g , randInt( g , 0 , 12 ) , 1 , 60 );
  CHECK( dp::largestDivisibleSubsetSize( A ) == divisibleBrute( A ) );
  }
 CHECK( dp::largestDivisibleSubsetSize( {} ) == 0 );
 CHECK( dp::largestDivisibleSubsetSize( { 9 } ) == 1 );
 Ints P2;
 for( int i = 0 ; i < 31 ; ++i ) P2.push_back( 1 << i );
 CHECK( dp::largestDivisibleSubsetSize( P2 ) == 31 );
 CHECK( dp::largestDivisibleSubsetSize( { 2 , 3 , 5 , 7 , 11 , 13 } ) == 1 );
 CHECK( dp::largestDivisibleSubsetSize( { 7 , 1 , 5 } ) == 2 );
 CHECK( dp::largestDivisibleSubsetSize( { INT_MAX , 1 } ) == 2 );
 CHECK_THROWS( dp::largestDivisibleSubsetSize( { 2 , 2 } ) ,
               contract_violation );
 CHECK_THROWS( dp::largestDivisibleSubsetSize( { 0 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( dp::largestDivisibleSubsetSize( { -2 , 4 } ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------ dp-108, 2026-08-28, Pari-bilanciato -------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

/// Pari-bilanciato: the longest subarray of A with as many even items as
/// odd ones, 0 if none. With +1 for an even item and -1 for an odd one, A[
/// i .. j - 1 ] is balanced iff the prefix sums at i and j are equal; a
/// hash table keeps the first prefix length of each sum; expected O( n )
/// time
int maxBalanced( const Ints & A )
{
 std::unordered_map< int , int > first;
 first.emplace( 0 , 0 );
 int balance = 0 , best = 0;
 for( int i = 0 ; i < int( A.size() ) ; ++i ) {
  balance += A[ i ] % 2 == 0 ? 1 : -1;
  const auto [ it , fresh ] = first.emplace( balance , i + 1 );
  if( ! fresh ) best = std::max( best , i + 1 - it->second );
  }
 return( best );
 }

/// the quadratic version of the text over the pairs of prefix sums
int maxBalancedQuadratic( const Ints & A )
{
 const int n = int( A.size() );
 Ints prefix( n + 1 , 0 );
 for( int i = 0 ; i < n ; ++i )
  prefix[ i + 1 ] = prefix[ i ] + ( A[ i ] % 2 == 0 ? 1 : -1 );
 int best = 0;
 for( int i = 0 ; i <= n ; ++i )
  for( int j = i + 1 ; j <= n ; ++j )
   if( prefix[ i ] == prefix[ j ] ) best = std::max( best , j - i );
 return( best );
 }

}  // end( namespace dp )

namespace {

int maxBalancedCubic( const Ints & A )
{
 const int n = int( A.size() );
 int best = 0;
 for( int i = 0 ; i < n ; ++i )
  for( int j = i + 1 ; j <= n ; ++j ) {
   int even = 0;
   for( int t = i ; t < j ; ++t ) even += A[ t ] % 2 == 0;
   if( 2 * even == j - i ) best = std::max( best , j - i );
   }
 return( best );
 }

// the example, the cubic count on random arrays with negative items, the
// empty array, all even, all odd (also negative), one pair, a long
// alternating array, the extremes of int
ADS_TEST( "dp-108" )
{
 CHECK( dp::maxBalanced( { 2 , 4 , 1 , 6 , 8 , 3 , 5 } ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Ints A = randInts( g , randInt( g , 0 , 25 ) , -9 , 9 );
  const int r = maxBalancedCubic( A );
  CHECK( dp::maxBalanced( A ) == r && dp::maxBalancedQuadratic( A ) == r );
  }
 CHECK( dp::maxBalanced( {} ) == 0 && dp::maxBalanced( { 3 } ) == 0 );
 CHECK( dp::maxBalanced( Ints( 100 , 4 ) ) == 0 );
 CHECK( dp::maxBalanced( Ints( 100 , -3 ) ) == 0 &&
        dp::maxBalancedQuadratic( Ints( 100 , -3 ) ) == 0 );
 CHECK( dp::maxBalanced( { -3 , -2 } ) == 2 &&
        dp::maxBalancedQuadratic( { -3 , -2 } ) == 2 );
 CHECK( dp::maxBalancedQuadratic( {} ) == 0 );
 Ints alt( 100000 );
 for( int i = 0 ; i < 100000 ; ++i ) alt[ i ] = i;
 CHECK( dp::maxBalanced( alt ) == 100000 );
 CHECK( dp::maxBalanced( { INT_MAX , INT_MIN } ) == 2 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File exam_dp3.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
