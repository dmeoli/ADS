/*--------------------------------------------------------------------------*/
/*--------------------------- File exam_dp1.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems on dynamic programming of the collection of written
 * tests, from the longest palindromic substring to the sort of an array of
 * few distinct values: each problem is solved as its solution says, with
 * the errors of the printed pseudocode corrected and said in a comment, and
 * checked on the examples of the text, against a brute force or a closed
 * formula on random small instances with a fixed seed, and on the edge
 * cases. A problem that the library already solves is tested through the
 * function of the library.
 *
 * \see A. Montresor, Esercizi di programmazione dinamica, raccolte da
 *      compiti scritti, https://algoritmicamente.it
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

#include "ADS/algorithm/Subsequences.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/CoinGame.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/KnapsackVariants.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/OptimalSearchTree.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/stack/StackAlgorithms.h"
#include "ADS/string/Anagrams.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Palindromes.h"
#include "ADS/string/Segmentations.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SOLUTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace dp {

using Int = std::int64_t;
using Count = std::uint64_t;
using Table = std::vector< std::vector< Int > >;

constexpr Int INF = std::numeric_limits< Int >::max() / 4;

/*--- 1, 35, 2011-05-02, 2015-09-01, Sottostringa palindroma piu' lunga ----*/

// the table P[ i ][ j ] of the text, whether s[ i .. j ] is a palindrome,
// filled by increasing length, gives the same palindrome as
// longestPalindromicSubstring() of the library [see Palindromes.h]

/*-------------------- 2, 2011-05-27, Parentesizzazione --------------------*/

/// the least value of C[ 0 ] O[ 0 ] C[ 1 ] ... C[ n - 1 ] over all its
/// parenthesizations, with the best split S[ i ][ j ] of each C[ i .. j ]:
/// D[ i ][ j ] = min over i <= k < j of D[ i ][ k ] O[ k ] D[ k + 1 ][ j ],
/// right since + and * are monotone on positive integers; the scheme of the
/// chain of matrices, O( n^3 ) time and O( n^2 ) space
/// \pre n >= 1 positive operands, n - 1 operators in { +, * }
inline std::pair< Int , std::vector< std::vector< std::size_t > > >
 minParenthesization( const std::vector< Int > & C , const std::string & O )
{
 ADS_PRE( ( ! C.empty() ) && ( O.size() + 1 == C.size() ) );
 ADS_PRE( std::all_of( C.begin() , C.end() ,
                       []( Int c ) { return( c > 0 ); } ) );
 ADS_PRE( std::all_of( O.begin() , O.end() ,
                       []( char o ) { return( o == '+' || o == '*' ); } ) );
 const std::size_t n = C.size();
 Table D( n , std::vector< Int >( n , 0 ) );
 std::vector< std::vector< std::size_t > > S( n ,
                                     std::vector< std::size_t >( n , 0 ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  D[ i ][ i ] = C[ i ];
 for( std::size_t h = 2 ; h <= n ; ++h )
  for( std::size_t i = 0 ; i + h <= n ; ++i ) {
   const std::size_t j = i + h - 1;
   D[ i ][ j ] = INF;
   for( std::size_t k = i ; k < j ; ++k ) {
    const Int t = ( O[ k ] == '+' ) ? D[ i ][ k ] + D[ k + 1 ][ j ]
                                    : D[ i ][ k ] * D[ k + 1 ][ j ];
    if( t < D[ i ][ j ] ) {
     D[ i ][ j ] = t;
     S[ i ][ j ] = k;
     }
    }
   }
 return( std::pair( D[ 0 ][ n - 1 ] , S ) );
 }

/// stampaPar: the expression C[ i .. j ] fully parenthesized by the splits
/// S of minParenthesization(), "(a o b)" for each operator; O( n )
/// \pre i <= j < n
inline std::string printParenthesization(
                  const std::vector< Int > & C , const std::string & O ,
                  const std::vector< std::vector< std::size_t > > & S ,
                  std::size_t i , std::size_t j )
{
 ADS_PRE( ( i <= j ) && ( j < C.size() ) );
 if( i == j )
  return( std::to_string( C[ i ] ) );
 const std::size_t k = S[ i ][ j ];
 return( "(" + printParenthesization( C , O , S , i , k ) + " " + O[ k ] +
         " " + printParenthesization( C , O , S , k + 1 , j ) + ")" );
 }

/*------------------------- 3, 2011-06-06, Scatole -------------------------*/

/// a box, not to be rotated
struct Box {
 int x , y , z;
 };

/// whether a fits in b, smaller on every side
inline bool fitsIn( const Box & a , const Box & b )
{
 return( ( a.x < b.x ) && ( a.y < b.y ) && ( a.z < b.z ) );
 }

/// the places of a longest chain of boxes, each in the next and in the
/// given order: L[ j ] = 1 + max{ L[ i ] : i < j , S[ i ] fits in S[ j ] }
/// with the best i kept to read the chain back, the longest increasing
/// subsequence under the dominance on three sides; O( n^2 ) time, O( n )
/// space. The text sets N[ j ] = max( N[ j - 1 ] , N[ prev[ j ] ] ) with
/// prev[ j ] the largest i that fits: it misses the + 1, and the largest
/// such i is not the best one
/// (here longestChainSubsequence() of the library)
/// \post increasing places, each box in the next
inline std::vector< std::size_t > boxChain( const std::vector< Box > & S )
{
 return( longestChainSubsequence( S , fitsIn ) );
 }

/*---------------------- 4, 2011-07-18, Multiset sum -----------------------*/

/// multisetSum(): whether k is a sum of A[ 0 .. i - 1 ] with repetitions,
/// memoized in M (-1 unknown)
inline bool multisetSumRec( const std::vector< Int > & A , std::size_t i ,
                            Int k ,
                            std::vector< std::vector< signed char > > & M )
{
 if( k == 0 )
  return( true );
 if( ( k < 0 ) || ( i == 0 ) )
  return( false );
 auto & m = M[ i ][ static_cast< std::size_t >( k ) ];
 if( m < 0 )
  m = multisetSumRec( A , i - 1 , k , M ) ||
      multisetSumRec( A , i , k - A[ i - 1 ] , M );
 return( m > 0 );
 }

/// whether t is a sum of items of A, each taken any number of times:
/// S( i , k ) = S( i - 1 , k ) or S( i , k - A[ i ] ), S( i , 0 ) true,
/// S( 0 , k > 0 ) false, by memoization; O( n t ) time and space
/// \pre t >= 0 and A positive
inline bool multisetSum( const std::vector< Int > & A , Int t )
{
 ADS_PRE( t >= 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( Int a ) { return( a > 0 ); } ) );
 std::vector< std::vector< signed char > > M( A.size() + 1 ,
                std::vector< signed char >( static_cast< std::size_t >( t )
                                            + 1 , -1 ) );
 return( multisetSumRec( A , A.size() , t , M ) );
 }

/*------------------------ 5, 2011-09-07, Anagrams -------------------------*/

// the groups of anagrams, by the characters of each string sorted as its
// key in a hash table: anagramGroups() of the library [see Anagrams.h]

/*-------------------- 6, 2012-01-11, Chiavi frequenti ---------------------*/

/// an optimal binary search tree on the keys 0 .. n - 1: its cost, its
/// root and the children of each key (-1 for none)
using OptimalBst = OptimalSearchTree< Int >;

/// the binary search tree of least sum p[ i ] ( l[ i ] + 1 ), l[ i ] the
/// depth of key i: with W( i , j ) the sum of p[ i .. j - 1 ], C[ i ][ i ] = 0
/// and C[ i ][ j ] = W( i , j ) + min over i <= r < j of C[ i ][ r ] +
/// C[ r + 1 ][ j ], the best r kept as root; O( n^3 ) time (O( n^2 ) with
/// the monotone roots of Knuth, as optimalSearchTree() of the library
/// does), O( n^2 ) space. The text leaves it out
/// \pre p not negative
/// \post a tree whose in-order is 0 .. n - 1
inline OptimalBst optimalBst( const std::vector< Int > & p )
{
 return( optimalSearchTree( p ) );
 }

/*---------------- 7, 12, 2012-01-11, 2012-05-31, Mediana4 -----------------*/

/// the two middle values, of rank n and n + 1, of the 2 n items of the
/// sorted X and Y: on ranges of X and Y of equal length, mx the lower middle
/// of X and my the upper middle of Y; if X[ mx ] < Y[ my ] the items left of
/// mx and right of my go, if Y[ my ] < X[ mx ] those right of mx and left of
/// my, as many on each side, and if they are equal they are the two middle
/// values; with two items left on each side the middle two of the four;
/// O( log n ) time. The text computes my from bx and ex instead of by and
/// ey, and never stops for n = 1
/// \pre X and Y sorted, of the same length n >= 1
inline std::pair< Int , Int > medianPair( const std::vector< Int > & X ,
                                          const std::vector< Int > & Y )
{
 ADS_PRE( ( ! X.empty() ) && ( X.size() == Y.size() ) );
 ADS_PRE( std::is_sorted( X.begin() , X.end() ) &&
          std::is_sorted( Y.begin() , Y.end() ) );
 std::size_t bx = 0 , ex = X.size() - 1 , by = 0 , ey = Y.size() - 1;
 while( ex - bx >= 2 ) {
  const std::size_t mx = ( bx + ex ) / 2 , my = ( by + ey + 1 ) / 2;
  if( X[ mx ] < Y[ my ] ) {
   bx = mx;
   ey = my;
   }
  else if( Y[ my ] < X[ mx ] ) {
   ex = mx;
   by = my;
   }
  else
   return( std::pair( X[ mx ] , Y[ my ] ) );
  }
 std::vector< Int > v;                          // mediana4: 2 or 4 items
 for( std::size_t i = bx ; i <= ex ; ++i )
  v.push_back( X[ i ] );
 for( std::size_t i = by ; i <= ey ; ++i )
  v.push_back( Y[ i ] );
 std::sort( v.begin() , v.end() );
 const std::size_t h = v.size() / 2;
 return( std::pair( v[ h - 1 ] , v[ h ] ) );
 }

/*------------------------ 8, 2012-02-01, Best cut -------------------------*/

/// the largest gain of a board of length L sold in pieces, G[ t - 1 ] the
/// price of a piece of length t, and the lengths of the pieces:
/// M( l ) = max over 1 <= t <= l of G[ t ] + M( l - t ), M( 0 ) = 0,
/// keeping the first piece, then printCut follows the pieces; O( L^2 )
/// time, O( L ) space; rodCutting() of the library
/// \pre L <= |G| and G not negative
/// \post the pieces sum to L and their prices to the gain
inline std::pair< Int , std::vector< std::size_t > > bestCut(
                                const std::vector< Int > & G , std::size_t L )
{
 ADS_PRE( L <= G.size() );
 ADS_PRE( std::all_of( G.begin() , G.end() ,
                       []( Int x ) { return( x >= 0 ); } ) );
 return( rodCutting( G , L ) );
 }

/*-------------------- 9, 2012-02-01, Prima elementare ---------------------*/

/// printRGRec(): the strings that complete S[ 0 .. i - 1 ] with n 'R' and
/// m 'G'
inline void printRGRec( std::string & S , std::size_t i , int n , int m ,
                        std::vector< std::string > & out )
{
 if( ( n == 0 ) && ( m == 0 ) ) {
  out.push_back( S );
  return;
  }
 if( n > 0 ) {
  S[ i ] = 'R';
  printRGRec( S , i + 1 , n - 1 , m , out );
  }
 if( m > 0 ) {
  S[ i ] = 'G';
  printRGRec( S , i + 1 , n , m - 1 , out );
  }
 }

/// the strings of n 'R' and m 'G', by backtracking on the next character;
/// O( C( n + m , n ) ( n + m ) ) time
/// \pre n , m >= 0
inline std::vector< std::string > printRG( int n , int m )
{
 ADS_PRE( ( n >= 0 ) && ( m >= 0 ) );
 std::string S( static_cast< std::size_t >( n + m ) , ' ' );
 std::vector< std::string > out;
 printRGRec( S , 0 , n , m , out );
 return( out );
 }

/// the number of the strings of n 'R' and m 'G': D[ i ][ 0 ] = D[ 0 ][ j ]
/// = 1, D[ i ][ j ] = D[ i - 1 ][ j ] + D[ i ][ j - 1 ], that is
/// C( n + m , n ); O( n m ) time and space
/// \pre n , m >= 0 and n + m <= 67, not to overflow
inline Count countRG( int n , int m )
{
 ADS_PRE( ( n >= 0 ) && ( m >= 0 ) && ( n + m <= 67 ) );
 std::vector< std::vector< Count > > D( n + 1 ,
                                        std::vector< Count >( m + 1 , 1 ) );
 for( int i = 1 ; i <= n ; ++i )
  for( int j = 1 ; j <= m ; ++j )
   D[ i ][ j ] = D[ i - 1 ][ j ] + D[ i ][ j - 1 ];
 return( D[ n ][ m ] );
 }

/*----------------- 10, 2012-05-03, Il gioco delle coppie ------------------*/

/// whether the distinct items of A split into pairs of the same sum: after
/// sorting, the largest must go with the least, so the pairs are
/// A[ i ] , A[ n - 1 - i ]; O( n log n ) time
/// \pre |A| even, the sum of two items fits in Int
inline bool checkPairs( std::vector< Int > A )
{
 ADS_PRE( A.size() % 2 == 0 );
 if( A.empty() )
  return( true );
 std::sort( A.begin() , A.end() );
 const std::size_t n = A.size();
 const Int pairSum = A[ 0 ] + A[ n - 1 ];
 for( std::size_t i = 1 ; i < n / 2 ; ++i )
  if( A[ i ] + A[ n - 1 - i ] != pairSum )
   return( false );
 return( true );
 }

/// the same by hashing: the sum of each pair is S = sum( A ) / ( n / 2 ),
/// which must be an integer, and S - a must be in A for every a; O( n )
/// expected time
/// \pre |A| even and the items distinct, their partial sums fit in Int
inline bool checkPairsHash( const std::vector< Int > & A )
{
 ADS_PRE( A.size() % 2 == 0 );
 if( A.empty() )
  return( true );
 const Int half = static_cast< Int >( A.size() / 2 );
 const Int total = std::accumulate( A.begin() , A.end() , Int( 0 ) );
 if( total % half != 0 )
  return( false );
 const Int S = total / half;
 const std::unordered_set< Int > H( A.begin() , A.end() );
 ADS_PRE( H.size() == A.size() );
 return( std::all_of( A.begin() , A.end() , [ & ]( Int a ) {
          return( ( S - a != a ) && H.count( S - a ) ); } ) );
 }

/*------------------- 11, 2012-05-03, Ottimizza la somma -------------------*/

/// sum(): the largest sum X[ i ][ w ] <= w of multiples of V[ 0 .. i - 1 ],
/// memoized (-1 unknown), - INF for w < 0
inline Int maxSumRec( const std::vector< Int > & V , std::size_t i , Int w ,
                      Table & X )
{
 if( w < 0 )
  return( - INF );
 if( ( i == 0 ) || ( w == 0 ) )
  return( 0 );
 Int & x = X[ i ][ static_cast< std::size_t >( w ) ];
 if( x < 0 )
  x = std::max( maxSumRec( V , i - 1 , w , X ) ,
                maxSumRec( V , i , w - V[ i - 1 ] , X ) + V[ i - 1 ] );
 return( x );
 }

/// the largest X = sum x[ i ] V[ i ] <= W over the x[ i ] >= 0 and the
/// multiplicities x: X[ i ][ w ] = max( X[ i - 1 ][ w ] ,
/// X[ i ][ w - V[ i ] ] + V[ i ] ), 0 for i = 0 or w = 0, - INF for w < 0,
/// by memoization; then getsum walks back from ( n , W ), taking V[ i ]
/// while that is strictly better; O( n W ) time and space
/// \pre W >= 0 and V positive
/// \post the multiples sum to the largest value, at most W
inline std::pair< Int , std::vector< Int > > maxSum(
                                      const std::vector< Int > & V , Int W )
{
 ADS_PRE( W >= 0 );
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( Int v ) { return( v > 0 ); } ) );
 const std::size_t n = V.size();
 Table X( n + 1 , std::vector< Int >( static_cast< std::size_t >( W ) + 1 ,
                                      -1 ) );
 const Int best = maxSumRec( V , n , W , X );
 std::vector< Int > x( n , 0 );
 std::size_t i = n;
 Int w = W;
 while( ( i > 0 ) && ( w > 0 ) ) {
  const Int take = ( w >= V[ i - 1 ] )
                    ? maxSumRec( V , i , w - V[ i - 1 ] , X ) + V[ i - 1 ]
                    : - INF;
  if( take > maxSumRec( V , i - 1 , w , X ) ) {
   ++x[ i - 1 ];
   w -= V[ i - 1 ];
   }
  else
   --i;
  }
 ADS_POST( std::inner_product( x.begin() , x.end() , V.begin() , Int( 0 ) )
           == best );
 ADS_POST( best <= W );
 return( std::pair( best , x ) );
 }

/*------------------------- 13, 2012-05-31, Resto --------------------------*/

/// the number of ways of paying T with banknotes B in unlimited supply, the
/// order not counting: C[ i ][ t ] = C[ i - 1 ][ t ] + C[ i ][ t - B[ i ] ],
/// 1 for t = 0, 0 for t < 0 or i = 0; O( n T ) time, coinChangeWays() of
/// the library
/// \pre T >= 0 and B positive
inline Count changeWays( const std::vector< Int > & B , Int T )
{
 return( coinChangeWays< Count >( B , T ) );
 }

/*----------------------- 14, 2012-06-18, Scacchiera -----------------------*/

/// the largest sum of the cells of a n x 3 board of positive integers that
/// carry a token, no two tokens side by side: the tokens of a row are a
/// mask among 0, 1, 2, 4, 5, and M[ i ][ t ] = max over the masks t' with
/// t & t' = 0 of M[ i - 1 ][ t' ] plus the cells of t in row i; Theta( n )
/// time
/// \pre the cells positive
inline Int boardTokens( const std::vector< std::array< Int , 3 > > & A )
{
 constexpr std::array< int , 5 > masks = { 0 , 1 , 2 , 4 , 5 };
 std::array< Int , 8 > M{};
 for( const auto & row : A ) {
  ADS_PRE( ( row[ 0 ] > 0 ) && ( row[ 1 ] > 0 ) && ( row[ 2 ] > 0 ) );
  std::array< Int , 8 > N{};
  for( int t : masks ) {
   Int best = 0;
   for( int u : masks )
    if( ( t & u ) == 0 )
     best = std::max( best , M[ u ] );
   for( int x = 0 ; x < 3 ; ++x )
    if( ( t >> x ) & 1 )
     best += row[ x ];
   N[ t ] = best;
   }
  M = N;
  }
 Int best = 0;
 for( int t : masks )
  best = std::max( best , M[ t ] );
 return( best );
 }

/*------------------- 15, 2012-07-09, Stringhe primitive -------------------*/

// the ways of writing X as a sequence of the primitive strings S: D[ n ] =
// 1, D[ i ] = sum of D[ i + |s| ] over the s of S that occur in X at i,
// memoized by count() of the text: countSegmentations() of the library
// [see Segmentations.h]

/*--------------------- 16, 2013-01-07, Conta percorsi ---------------------*/

/// the number of paths from the top left to the bottom right corner of a
/// m x n board, right or down through the free cells (B 1): count[ i ][ j ]
/// = ( count[ i + 1 ][ j ] + count[ i ][ j + 1 ] ) B[ i ][ j ], 0 out of the
/// board, filled backwards; O( m n ) time, countLatticePaths() of the
/// library
/// \pre m , n >= 1 and B a rectangle of 0 and 1
inline Count countPaths( const std::vector< std::vector< int > > & B )
{
 ADS_PRE( ( ! B.empty() ) && ( ! B[ 0 ].empty() ) );
 return( countLatticePaths< Count >( B ) );
 }

/*-------------------- 18, 2013-05-03, Barche sul fiume --------------------*/

/// the least cost from port 0 to port n - 1 going downstream, C[ i ][ j ]
/// the price of a boat from i to j > i, and the ports where a boat is
/// taken: D[ n - 1 ] = 0, D[ i ] = min over j > i of C[ i ][ j ] + D[ j ]
/// with the best j kept; O( n^2 ) time, O( n ) space
/// \pre n >= 1, C at least n x n and not negative above the diagonal
inline std::pair< Int , std::vector< std::size_t > > boats(
                                                      const Table & C )
{
 const std::size_t n = C.size();
 ADS_PRE( n >= 1 );
 std::vector< Int > D( n , 0 );
 std::vector< std::size_t > next( n , n );
 for( std::size_t i = n - 1 ; i-- > 0 ; ) {
  ADS_PRE( C[ i ].size() >= n );
  D[ i ] = INF;
  for( std::size_t j = i + 1 ; j < n ; ++j ) {
   ADS_PRE( C[ i ][ j ] >= 0 );
   if( C[ i ][ j ] + D[ j ] < D[ i ] ) {
    D[ i ] = C[ i ][ j ] + D[ j ];
    next[ i ] = j;
    }
   }
  }
 std::vector< std::size_t > ports;
 for( std::size_t i = 0 ; i + 1 < n ; i = next[ i ] )
  ports.push_back( i );
 return( std::pair( D[ 0 ] , ports ) );
 }

/*----------------------- 19, 2013-05-31, Max resto ------------------------*/

/// maxResto(): the most coins among V[ 0 .. i - 1 ] summing to t, - INF if
/// none, memoized (INF unknown)
inline Int maxCoinsRec( const std::vector< Int > & V , std::size_t i , Int t ,
                        Table & D )
{
 if( t == 0 )
  return( 0 );
 if( ( t < 0 ) || ( i == 0 ) )
  return( - INF );
 Int & d = D[ i ][ static_cast< std::size_t >( t ) ];
 if( d == INF )
  d = std::max( maxCoinsRec( V , i - 1 , t , D ) ,
                maxCoinsRec( V , i - 1 , t - V[ i - 1 ] , D ) + 1 );
 return( d );
 }

/// the most coins of V, each used once, that pay exactly T, nothing if
/// none: D[ i ][ t ] = max( D[ i - 1 ][ t ] , D[ i - 1 ][ t - V[ i ] ] + 1 ),
/// 0 for t = 0, - INF for t < 0 or i = 0 < t, by memoization; O( n T ) time
/// and space
/// \pre T >= 0 and V positive
inline std::optional< Int > maxCoins( const std::vector< Int > & V , Int T )
{
 ADS_PRE( T >= 0 );
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( Int v ) { return( v > 0 ); } ) );
 Table D( V.size() + 1 ,
          std::vector< Int >( static_cast< std::size_t >( T ) + 1 , INF ) );
 const Int r = maxCoinsRec( V , V.size() , T , D );
 if( r < 0 )
  return( std::nullopt );
 return( r );
 }

/*---------------------- 20, 2013-06-17, Fundraising -----------------------*/

/// the largest sum of the donations D of houses no two of which are
/// neighbours: DP[ 1 ] = D[ 1 ], DP[ 2 ] = max( D[ 1 ] , D[ 2 ] ),
/// DP[ i ] = max( DP[ i - 1 ] , DP[ i - 2 ] + D[ i ] ); Theta( n ) time,
/// maxNonAdjacentSum() of the library. The text writes D for DP in the
/// last formula
/// \pre D not negative
inline Int fundraising( const std::vector< Int > & D )
{
 ADS_PRE( std::all_of( D.begin() , D.end() ,
                       []( Int d ) { return( d >= 0 ); } ) );
 return( maxNonAdjacentSum( D ) );
 }

/// the houses of an optimal fundraising(), walking the table back from n:
/// house i gives when DP[ i ] > DP[ i - 1 ]; Theta( n ) time,
/// nonAdjacentSelection() of the library
/// \pre D not negative
/// \post increasing places, no two consecutive, of sum fundraising( D )
inline std::vector< std::size_t > donors( const std::vector< Int > & D )
{
 ADS_PRE( std::all_of( D.begin() , D.end() ,
                       []( Int d ) { return( d >= 0 ); } ) );
 return( nonAdjacentSelection( D ).second );
 }

/*-------------------- 21, 2013-07-22, Gioco di monete ---------------------*/

/// the most the first player can be sure of in the game where two players
/// take in turn the first or the last coin of V: on the intervals of even
/// length, M[ i ][ j ] = max( V[ i ] + min( M[ i + 2 ][ j ] ,
/// M[ i + 1 ][ j - 1 ] ) , V[ j ] + min( M[ i + 1 ][ j - 1 ] ,
/// M[ i ][ j - 2 ] ) ), the opponent leaving the worse of the two;
/// O( n^2 ) time, coinsInALine() of the library
/// \pre |V| even and V not negative
inline Int coinGame( const std::vector< Int > & V )
{
 ADS_PRE( V.size() % 2 == 0 );
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( Int v ) { return( v >= 0 ); } ) );
 return( coinsInALine( V ) );
 }

/*------------------------ 22, 2013-09-02, Dice rec ------------------------*/

/// the table DP[ x ][ i ][ m ] of the dice, -1 unknown
using DiceTable = std::vector< std::vector< std::vector< long long > > >;

/// diceRec(): the ways of making x with the dice 1 .. i, nondecreasing
/// values from the last die, each at least m: the sum over m <= j <= F[ i ]
/// of DP[ x - j ][ i - 1 ][ j ]
inline long long diceRec( const std::vector< int > & F , std::size_t i ,
                          int x , int m , DiceTable & DP )
{
 if( ( x == 0 ) && ( i == 0 ) )
  return( 1 );
 if( ( x <= 0 ) || ( i == 0 ) )
  return( 0 );
 long long & d = DP[ x ][ i ][ m ];
 if( d < 0 ) {
  d = 0;
  for( int j = m ; j <= F[ i - 1 ] ; ++j )
   d += diceRec( F , i - 1 , x - j , j , DP );
  }
 return( d );
 }

/// diceFast(): the same by DP[ x ][ i ][ m ] = DP[ x - m ][ i - 1 ][ m ] +
/// DP[ x ][ i ][ m + 1 ], the second term only for m < F[ i ]
inline long long diceFastRec( const std::vector< int > & F , std::size_t i ,
                              int x , int m , DiceTable & DP )
{
 if( ( x == 0 ) && ( i == 0 ) )
  return( 1 );
 if( ( x <= 0 ) || ( i == 0 ) )
  return( 0 );
 long long & d = DP[ x ][ i ][ m ];
 if( d < 0 ) {
  d = diceFastRec( F , i - 1 , x - m , m , DP );
  if( m < F[ i - 1 ] )
   d += diceFastRec( F , i , x , m + 1 , DP );
  }
 return( d );
 }

/// the number of ways of making X with all the dice, die i of faces
/// 1 .. F[ i ], the permutations not counting (multisets of values): the
/// values are taken nondecreasing from the last die, which needs the dice
/// by decreasing number of faces (sorted here); fast false gives the sum
/// recurrence, O( n X M^2 ), true the one of two terms, O( n X M ), M the
/// most faces
/// \pre X >= 0 and F positive
inline long long dice( std::vector< int > F , int X , bool fast )
{
 ADS_PRE( X >= 0 );
 ADS_PRE( std::all_of( F.begin() , F.end() ,
                       []( int f ) { return( f > 0 ); } ) );
 std::sort( F.begin() , F.end() , std::greater< int >() );
 const int M = F.empty() ? 1 : F.front();
 DiceTable DP( X + 1 , std::vector< std::vector< long long > >(
                F.size() + 1 , std::vector< long long >( M + 2 , -1 ) ) );
 return( fast ? diceFastRec( F , F.size() , X , 1 , DP )
              : diceRec( F , F.size() , X , 1 , DP ) );
 }

/*------------------------ 23, 2014-01-07, Altezza -------------------------*/

/// the least total height of the shelves holding the books of heights y,
/// at most L on a shelf, a shelf as high as its highest book: the books by
/// decreasing height fill the shelves L at a time, so the total is the sum
/// of the heights of the books of places n - 1, n - 1 - L, ... in the
/// increasing order; O( n log n ) time
/// \pre L >= 1
inline Int shelvesHeight( std::vector< Int > y , std::size_t L )
{
 ADS_PRE( L >= 1 );
 std::sort( y.begin() , y.end() );
 Int total = 0;
 std::size_t left = 0;
 for( std::size_t i = y.size() ; i-- > 0 ; ) {
  if( left == 0 ) {                             // a new shelf
   total += y[ i ];
   left = L;
   }
  --left;
  }
 return( total );
 }

/*------------------------ 24, 2014-02-03, Max gain ------------------------*/

/// the largest gain of 4 rooks on a 4 x n board, no two on a row or a
/// column, S[ i ][ k ] the gain of row i and column k: DP[ k ][ m ] the best
/// on the first k columns with the rows of the mask m taken, 0 for k = 0 or
/// m = 15, max( DP[ k - 1 ][ m ] , DP[ k - 1 ][ m | 2^i ] + S[ i ][ k ] )
/// over the free rows i, the answer DP[ n ][ 0 ]; Theta( n ) time. The
/// text writes min for max and returns DP[ 0 ][ n ]
/// \pre n >= 4 and the gains positive
inline Int rooks( const std::array< std::vector< Int > , 4 > & S )
{
 const std::size_t n = S[ 0 ].size();
 ADS_PRE( n >= 4 );
 for( const auto & row : S ) {
  ADS_PRE( row.size() == n );
  ADS_PRE( std::all_of( row.begin() , row.end() ,
                        []( Int s ) { return( s > 0 ); } ) );
  }
 std::array< Int , 16 > D{};                    // DP[ k - 1 ][ . ]
 for( std::size_t k = 0 ; k < n ; ++k ) {
  std::array< Int , 16 > E{};
  for( int m = 0 ; m < 15 ; ++m ) {
   E[ m ] = D[ m ];
   for( int i = 0 ; i < 4 ; ++i )
    if( ! ( ( m >> i ) & 1 ) )
     E[ m ] = std::max( E[ m ] , D[ m | ( 1 << i ) ] + S[ i ][ k ] );
   }
  D = E;
  }
 return( D[ 0 ] );
 }

/*---------------------- 25, 2014-06-05, Partition k -----------------------*/

/// the least cost of a split of V in 2 nonempty contiguous parts, the cost
/// the largest sum of a part: one scan of the prefix sums against the
/// total; Theta( n ) time
/// \pre |V| >= 2
inline Int partition2( const std::vector< Int > & V )
{
 ADS_PRE( V.size() >= 2 );
 const Int tot = std::accumulate( V.begin() , V.end() , Int( 0 ) );
 Int sumSoFar = 0 , minSoFar = INF;
 for( std::size_t i = 0 ; i + 1 < V.size() ; ++i ) {
  sumSoFar += V[ i ];
  minSoFar = std::min( minSoFar , std::max( sumSoFar , tot - sumSoFar ) );
  }
 return( minSoFar );
 }

/// the same for 3 parts, over the pairs of cuts with the prefix sums;
/// Theta( n^2 ) time
/// \pre |V| >= 3
inline Int partition3( const std::vector< Int > & V )
{
 ADS_PRE( V.size() >= 3 );
 const std::size_t n = V.size();
 std::vector< Int > tot( n + 1 , 0 );
 for( std::size_t i = 0 ; i < n ; ++i )
  tot[ i + 1 ] = tot[ i ] + V[ i ];
 Int minSoFar = INF;
 for( std::size_t i = 1 ; i + 2 <= n ; ++i )
  for( std::size_t j = i + 1 ; j + 1 <= n ; ++j )
   minSoFar = std::min( minSoFar , std::max( { tot[ i ] , tot[ j ] - tot[ i ] ,
                                               tot[ n ] - tot[ j ] } ) );
 return( minSoFar );
 }

/// the least cost of a split of V, possibly negative, in k nonempty
/// contiguous parts, the cost the largest sum of a part: with the prefix
/// sums tot, DP[ i ][ 1 ] = tot[ i ], DP[ i ][ t ] = min over j < i of
/// max( DP[ j ][ t - 1 ] , tot[ i ] - tot[ j ] ); O( k n^2 ) time,
/// O( k n ) space, linearPartition() of the library
/// \pre 1 <= k <= |V|
inline Int partitionK( const std::vector< Int > & V , std::size_t k )
{
 return( linearPartition( V , k ).first );
 }

/*-------------------- 26, 2014-06-16, Input file file ---------------------*/

/// the expected access time sum P[ f ] T[ f ] of the files of lengths L
/// and probabilities P in the given order on the tape, T[ f ] the sum of
/// the lengths up to f included; O( n ) time
/// \pre order a permutation of the files
inline double tapeCost( const std::vector< double > & L ,
                        const std::vector< double > & P ,
                        const std::vector< std::size_t > & order )
{
 ADS_PRE( ( L.size() == P.size() ) && ( order.size() == L.size() ) );
 double T = 0 , cost = 0;
 for( std::size_t f : order ) {
  T += L[ f ];
  cost += P[ f ] * T;
  }
 return( cost );
 }

/// an optimal order of the files on the tape, by increasing L[ f ] / P[ f ]
/// (compared as L[ a ] P[ b ] < L[ b ] P[ a ]): swapping two neighbours out
/// of this order never lowers the cost; O( n log n ) time, Smith's rule,
/// smithOrder() of the library
/// \pre L not negative and P positive
inline std::vector< std::size_t > tapeOrder( const std::vector< double > & L ,
                                             const std::vector< double > & P )
{
 ADS_PRE( L.size() == P.size() );
 ADS_PRE( std::all_of( P.begin() , P.end() ,
                       []( double p ) { return( p > 0 ); } ) );
 return( smithOrder( L , P ) );
 }

/*------------------------ 27, 2014-06-16, Mintime -------------------------*/

/// the least time of recharge of a car of range A along the stations
/// 0 .. n - 1, d[ i ] the distance of station i from station i - 1 (d[ 0 ]
/// unused), t[ i ] the time of a km of recharge at i, leaving 0 empty:
/// T[ n - 1 ][ a ] = 0, T[ i ][ a ] = min over 0 <= g <= A - a with
/// a + g >= d[ i + 1 ] of T[ i + 1 ][ a + g - d[ i + 1 ] ] + g t[ i ],
/// nothing if a leg is longer than A; O( n A^2 ) time. The text writes 100
/// for A, goes to T[ i + 1 ][ a + g ] without the distance driven, and
/// tests d[ i ] for d[ i + 1 ]
/// \pre |t| = |d| = n >= 1, A >= 0, d and t not negative
inline std::optional< Int > mintime( const std::vector< Int > & t ,
                                     const std::vector< Int > & d , Int A )
{
 const std::size_t n = t.size();
 ADS_PRE( ( n >= 1 ) && ( d.size() == n ) && ( A >= 0 ) );
 ADS_PRE( std::all_of( d.begin() , d.end() ,
                       []( Int x ) { return( x >= 0 ); } ) );
 ADS_PRE( std::all_of( t.begin() , t.end() ,
                       []( Int x ) { return( x >= 0 ); } ) );
 const auto W = static_cast< std::size_t >( A ) + 1;
 Table T( n , std::vector< Int >( W , 0 ) );
 for( std::size_t i = n - 1 ; i-- > 0 ; )
  for( Int a = 0 ; a <= A ; ++a ) {
   Int & best = T[ i ][ static_cast< std::size_t >( a ) ];
   best = INF;
   for( Int g = 0 ; g <= A - a ; ++g )
    if( a + g >= d[ i + 1 ] ) {
     const Int next = T[ i + 1 ][ static_cast< std::size_t >(
                                                  a + g - d[ i + 1 ] ) ];
     if( next < INF )
      best = std::min( best , next + g * t[ i ] );
     }
   }
 if( T[ 0 ][ 0 ] >= INF )
  return( std::nullopt );
 return( T[ 0 ][ 0 ] );
 }

/*------------------- 28, 2014-07-21, Longest palindrome -------------------*/

// DP[ i ][ j ] = 0 for j < i, 1 for j = i, DP[ i + 1 ][ j - 1 ] + 2 if
// s[ i ] = s[ j ], else the larger of DP[ i + 1 ][ j ] and DP[ i ][ j - 1 ],
// printRec reading it back from both ends: the recursion of
// longestPalindromicSubsequence() of the library [see Palindromes.h]

/*-------------------- 30, 2014-12-19, Min allineamento --------------------*/

/// the fewest dashes that align P and S, every column two equal characters
/// or a character and a dash: DP[ i ][ 0 ] = i, DP[ 0 ][ j ] = j,
/// DP[ i ][ j ] = DP[ i - 1 ][ j - 1 ] if P[ i ] = S[ j ], else
/// min( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ) + 1; it is
/// n + m - 2 LCS( P , S ); O( n m ) time
inline std::size_t minAlignment( const std::string & P ,
                                 const std::string & S )
{
 const std::size_t n = P.size() , m = S.size();
 std::vector< std::vector< std::size_t > > DP( n + 1 ,
                                     std::vector< std::size_t >( m + 1 , 0 ) );
 for( std::size_t i = 0 ; i <= n ; ++i )
  DP[ i ][ 0 ] = i;
 for( std::size_t j = 0 ; j <= m ; ++j )
  DP[ 0 ][ j ] = j;
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = 1 ; j <= m ; ++j )
   DP[ i ][ j ] = ( P[ i - 1 ] == S[ j - 1 ] )
                   ? DP[ i - 1 ][ j - 1 ]
                   : std::min( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ) + 1;
 return( DP[ n ][ m ] );
 }

/*---------------------- 31, 2015-01-12, Count stairs ----------------------*/

/// the number of ordered ways of going down n stairs 1, 2, 3 or 4 at a
/// time: T( 0 ) = 1, T( n ) = sum over 1 <= k <= min( n , 4 ) of
/// T( n - k ), the Tetranacci numbers; O( n ) time
/// \pre 0 <= n <= 64, not to overflow
inline Count countStairs( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 64 ) );
 std::vector< Count > T( n + 1 , 0 );
 T[ 0 ] = 1;
 for( int i = 1 ; i <= n ; ++i )
  for( int k = 1 ; k <= std::min( i , 4 ) ; ++k )
   T[ i ] += T[ i - k ];
 return( T[ n ] );
 }

/*----------------------- 32, 2015-02-02, Piggy bank -----------------------*/

/// the least value of coins of weight exactly T, coin type i weighing p[ i ]
/// and worth v[ i ], in unlimited supply, nothing if none: DP[ t ][ i ] =
/// min( DP[ t - p[ i ] ][ i ] + v[ i ] , DP[ t ][ i - 1 ] ), 0 for t = 0,
/// INF for t < 0 or i = 0 < t; O( n T ) time, minValueExactWeight() of the
/// library
/// \pre T >= 0, p positive and v not negative
inline std::optional< Int > piggyBank( const std::vector< Int > & p ,
                                       const std::vector< Int > & v , Int T )
{
 ADS_PRE( std::all_of( v.begin() , v.end() ,
                       []( Int x ) { return( x >= 0 ); } ) );
 return( minValueExactWeight( p , v , T ) );
 }

/*------------------------- 34, 2015-07-14, Soccer -------------------------*/

/// soccerRec(): whether the first k players give a team of s players of
/// total m, memoized in D[ m ][ s ][ k ] (-1 unknown)
inline bool soccerRec( const std::vector< Int > & b , Int m , Int s ,
                       std::size_t k ,
                       std::vector< std::vector< std::vector< signed char > > >
                        & D )
{
 if( ( s == 0 ) && ( m == 0 ) )
  return( true );
 if( ( s < 0 ) || ( m < 0 ) || ( k == 0 ) )
  return( false );
 auto & d = D[ static_cast< std::size_t >( m ) ]
             [ static_cast< std::size_t >( s ) ][ k ];
 if( d < 0 )
  d = soccerRec( b , m - b[ k - 1 ] , s - 1 , k - 1 , D ) ||
      soccerRec( b , m , s , k - 1 , D );
 return( d > 0 );
 }

/// whether the 2 n players of skills b split into two teams of n players
/// and of equal total skill: DP[ m ][ s ][ k ] = DP[ m - b[ k ] ][ s - 1 ]
/// [ k - 1 ] or DP[ m ][ s ][ k - 1 ], the answer DP[ M / 2 ][ n ][ 2 n ],
/// false for an odd total M, by memoization; O( M n^2 ) time and space
/// \pre |b| even and b not negative
inline bool soccer( const std::vector< Int > & b )
{
 ADS_PRE( b.size() % 2 == 0 );
 ADS_PRE( std::all_of( b.begin() , b.end() ,
                       []( Int x ) { return( x >= 0 ); } ) );
 const Int M = std::accumulate( b.begin() , b.end() , Int( 0 ) );
 if( M % 2 != 0 )
  return( false );
 const std::size_t n = b.size() / 2;
 std::vector< std::vector< std::vector< signed char > > > D(
           static_cast< std::size_t >( M / 2 ) + 1 ,
           std::vector< std::vector< signed char > >( n + 1 ,
                         std::vector< signed char >( b.size() + 1 , -1 ) ) );
 return( soccerRec( b , M / 2 , static_cast< Int >( n ) , b.size() , D ) );
 }

/*---------------------- 36, 2015-09-01, Repeat sort -----------------------*/

/// sorts A of n items with m distinct values: a balanced search tree counts
/// the occurrences of each value, and its keys in order are written back
/// as many times as they occur; O( n log m ) time, O( m ) space
/// \post A sorted
inline void repeatSort( std::vector< Int > & A )
{
 std::map< Int , std::size_t > count;
 for( Int a : A )
  ++count[ a ];
 std::size_t i = 0;
 for( const auto & [ key , c ] : count )
  for( std::size_t r = 0 ; r < c ; ++r )
   A[ i++ ] = key;
 ADS_POST( std::is_sorted( A.begin() , A.end() ) );
 }

}  // end( namespace dp )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using dp::Int;
using dp::Count;

/// a uniform integer in [ a , b ]
Int rnd( std::mt19937 & g , Int a , Int b )
{
 return( std::uniform_int_distribution< Int >( a , b )( g ) );
 }

/// a vector of n uniform integers in [ a , b ]
std::vector< Int > rndVector( std::mt19937 & g , std::size_t n , Int a ,
                              Int b )
{
 std::vector< Int > v( n );
 for( auto & x : v )
  x = rnd( g , a , b );
 return( v );
 }

/// a string of n characters among the first k from 'a'
std::string rndString( std::mt19937 & g , std::size_t n , int k )
{
 std::string s( n , 'a' );
 for( auto & c : s )
  c = static_cast< char >( 'a' + rnd( g , 0 , k - 1 ) );
 return( s );
 }

/// C( n , k ) from the triangle of the library
Count choose( int n , int k )
{
 return( tartaglia( n )[ n ][ k ] );
 }

/// whether a is a subsequence of b
bool isSubsequence( const std::string & a , const std::string & b )
{
 std::size_t i = 0;
 for( char c : b )
  if( ( i < a.size() ) && ( a[ i ] == c ) )
   ++i;
 return( i == a.size() );
 }

/// whether s reads the same from both ends
bool palindrome( const std::string & s )
{
 return( std::equal( s.begin() , s.end() , s.rbegin() ) );
 }

}  // end( namespace )

// 2011-05-02 and 2015-09-01, Sottostringa palindroma piu' lunga: the
// examples of both texts, and every string of 1 to 9 characters over 3
// letters against the longest of all the substrings
ADS_TEST( "dp-1" )
{
 CHECK( longestPalindromicSubstring( "casacca" ) == "casac" );
 CHECK( longestPalindromicSubstring( "ovattavo con l'ovatta" ) ==
        "ovattavo" );
 CHECK( longestPalindromicSubstring( "x" ) == "x" );
 CHECK( longestPalindromicSubstring( "aaaa" ) == "aaaa" );
 CHECK( longestPalindromicSubstring( "abcd" ) == "a" );
 CHECK( longestPalindromicSubstring( std::string( 2000 , 'z' ) ).size()
        == 2000 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const std::string s = rndString( g , rnd( g , 1 , 9 ) , 3 );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < s.size() ; ++i )
   for( std::size_t j = i ; j < s.size() ; ++j )
    if( palindrome( s.substr( i , j - i + 1 ) ) )
     best = std::max( best , j - i + 1 );
  const auto p = longestPalindromicSubstring( s );
  CHECK( ( p.size() == best ) && palindrome( p ) &&
         ( s.find( p ) != std::string::npos ) );
  }
 CHECK( longestPalindromicSubstring( "" ).empty() );
 }

namespace {

/// the least value of C[ i .. j ] over all the trees, no memoization
Int minParenthesizationBrute( const std::vector< Int > & C ,
                              const std::string & O , std::size_t i ,
                              std::size_t j )
{
 if( i == j )
  return( C[ i ] );
 Int best = dp::INF;
 for( std::size_t k = i ; k < j ; ++k ) {
  const Int a = minParenthesizationBrute( C , O , i , k );
  const Int b = minParenthesizationBrute( C , O , k + 1 , j );
  best = std::min( best , ( O[ k ] == '+' ) ? a + b : a * b );
  }
 return( best );
 }

/// parses a fully parenthesized expression from s[ p ], appending its
/// operands to C and its operators to O, and gives its value
Int parseExpression( const std::string & s , std::size_t & p ,
                     std::vector< Int > & C , std::string & O )
{
 if( s[ p ] == '(' ) {
  ++p;
  const Int a = parseExpression( s , p , C , O );
  const char o = s[ p + 1 ];                    // " o "
  O.push_back( o );
  p += 3;
  const Int b = parseExpression( s , p , C , O );
  ++p;                                          // ")"
  return( ( o == '+' ) ? a + b : a * b );
  }
 Int v = 0;
 while( ( p < s.size() ) && ( s[ p ] >= '0' ) && ( s[ p ] <= '9' ) )
  v = 10 * v + ( s[ p++ ] - '0' );
 C.push_back( v );
 return( v );
 }

}  // end( namespace )

// 2011-05-27, Parentesizzazione: the example 5 + 13 * 2 and random
// expressions of 1 to 8 operands against all the trees
ADS_TEST( "dp-2-2" )
{
 CHECK( dp::minParenthesization( { 5 , 13 , 2 } , "+*" ).first == 31 );
 CHECK( dp::minParenthesization( { 7 } , "" ).first == 7 );
 CHECK( dp::minParenthesization( { 1 , 1 , 1 , 1 } , "***" ).first == 1 );
 CHECK( dp::minParenthesization( { 2 , 3 , 4 } , "++" ).first == 9 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 8 );
  const auto C = rndVector( g , n , 1 , 9 );
  std::string O;
  for( std::size_t i = 1 ; i < n ; ++i )
   O.push_back( rnd( g , 0 , 1 ) ? '+' : '*' );
  CHECK( dp::minParenthesization( C , O ).first ==
         minParenthesizationBrute( C , O , 0 , n - 1 ) );
  }
 CHECK_THROWS( dp::minParenthesization( { 0 , 1 } , "+" ) ,
               contract_violation );
 CHECK_THROWS( dp::minParenthesization( { 1 , 1 } , "-" ) ,
               contract_violation );
 CHECK_THROWS( dp::minParenthesization( {} , "" ) , contract_violation );
 CHECK_THROWS( dp::minParenthesization( { 1 , 2 } , "" ) ,
               contract_violation );
 CHECK_THROWS( dp::minParenthesization( { 1 } , "+" ) , contract_violation );
 CHECK_THROWS( dp::minParenthesization( { 2 , -1 } , "*" ) ,
               contract_violation );
 const Int big = Int( 1 ) << 40;
 CHECK( dp::minParenthesization( { big , big } , "+" ).first == 2 * big );
 CHECK( dp::minParenthesization( { 1 , 1 , 1 } , "+*" ).first == 2 );
 }

// 2011-05-27, Parentesizzazione: the printed expression of the example,
// and on random expressions one that keeps the operands and the operators
// in order and has the least value
ADS_TEST( "dp-2-3" )
{
 {
  const std::vector< Int > C = { 5 , 13 , 2 };
  const auto [ v , S ] = dp::minParenthesization( C , "+*" );
  CHECK( dp::printParenthesization( C , "+*" , S , 0 , 2 ) ==
         "(5 + (13 * 2))" );
  CHECK( dp::printParenthesization( C , "+*" , S , 1 , 1 ) == "13" );
  }
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 8 );
  const auto C = rndVector( g , n , 1 , 12 );
  std::string O;
  for( std::size_t i = 1 ; i < n ; ++i )
   O.push_back( rnd( g , 0 , 1 ) ? '+' : '*' );
  const auto [ v , S ] = dp::minParenthesization( C , O );
  const auto e = dp::printParenthesization( C , O , S , 0 , n - 1 );
  std::size_t p = 0;
  std::vector< Int > C2;
  std::string O2;
  CHECK( parseExpression( e , p , C2 , O2 ) == v );
  CHECK( ( p == e.size() ) && ( C2 == C ) && ( O2 == O ) );
  CHECK( v == minParenthesizationBrute( C , O , 0 , n - 1 ) );
  }
 const std::vector< Int > C = { 7 };
 const auto [ v , S ] = dp::minParenthesization( C , "" );
 CHECK( dp::printParenthesization( C , "" , S , 0 , 0 ) == "7" );
 CHECK_THROWS( dp::printParenthesization( C , "" , S , 0 , 1 ) ,
               contract_violation );
 const std::vector< Int > D = { 1 , 2 };
 const auto [ w , T ] = dp::minParenthesization( D , "+" );
 CHECK( dp::printParenthesization( D , "+" , T , 0 , 1 ) == "(1 + 2)" );
 CHECK_THROWS( dp::printParenthesization( D , "+" , T , 1 , 0 ) ,
               contract_violation );
 }

namespace {

/// the longest chain of boxes over all the subsets
std::size_t boxChainBrute( const std::vector< dp::Box > & S )
{
 const std::size_t n = S.size();
 std::size_t best = 0;
 for( std::uint32_t mask = 0 ; mask < ( 1u << n ) ; ++mask ) {
  std::vector< std::size_t > chain;
  for( std::size_t i = 0 ; i < n ; ++i )
   if( ( mask >> i ) & 1 )
    chain.push_back( i );
  bool ok = true;
  for( std::size_t k = 1 ; k < chain.size() ; ++k )
   ok = ok && dp::fitsIn( S[ chain[ k - 1 ] ] , S[ chain[ k ] ] );
  if( ok )
   best = std::max( best , chain.size() );
  }
 return( best );
 }

/// n random boxes of sides in [ 1 , k ]
std::vector< dp::Box > rndBoxes( std::mt19937 & g , std::size_t n , int k )
{
 std::vector< dp::Box > S( n );
 for( auto & b : S )
  b = dp::Box{ static_cast< int >( rnd( g , 1 , k ) ) ,
               static_cast< int >( rnd( g , 1 , k ) ) ,
               static_cast< int >( rnd( g , 1 , k ) ) };
 return( S );
 }

}  // end( namespace )

// 2011-06-06, Scatole: the length of the longest chain against all the
// subsets of up to 12 boxes, against lisLength() on cubes, and the edges
ADS_TEST( "dp-3-1" )
{
 CHECK( dp::boxChain( {} ).empty() );
 CHECK( dp::boxChain( { { 1 , 1 , 1 } } ).size() == 1 );
 CHECK( dp::boxChain( { { 2 , 2 , 2 } , { 2 , 2 , 2 } , { 2 , 2 , 2 } } )
        .size() == 1 );
 CHECK( dp::boxChain( { { 1 , 2 , 3 } , { 2 , 3 , 4 } , { 3 , 4 , 5 } } )
        .size() == 3 );
 CHECK( dp::boxChain( { { 3 , 4 , 5 } , { 2 , 3 , 4 } , { 1 , 2 , 3 } } )
        .size() == 1 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const auto S = rndBoxes( g , rnd( g , 0 , 12 ) , 5 );
  CHECK( dp::boxChain( S ).size() == boxChainBrute( S ) );
  }
 for( int r = 0 ; r < 200 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 1 , 40 ) , 0 , 30 );
  std::vector< dp::Box > S;
  for( Int a : A )
   S.push_back( dp::Box{ int( a ) , int( a ) , int( a ) } );
  CHECK( dp::boxChain( S ).size() == lisLength( A ) );
  }
 }

// 2011-06-06, Scatole: the chain read back is increasing, each box in the
// next, and as long as the longest
ADS_TEST( "dp-3-2" )
{
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const auto S = rndBoxes( g , rnd( g , 1 , 12 ) , 6 );
  const auto c = dp::boxChain( S );
  bool ok = ! c.empty();
  for( std::size_t k = 1 ; k < c.size() ; ++k )
   ok = ok && ( c[ k - 1 ] < c[ k ] ) && dp::fitsIn( S[ c[ k - 1 ] ] ,
                                                      S[ c[ k ] ] );
  CHECK( ok && ( c.size() == boxChainBrute( S ) ) );
  }
 }

// 2011-07-18, Multiset sum: the example, random instances against the sums
// reachable by a breadth-first search and against optimalChange()
ADS_TEST( "dp-4" )
{
 CHECK( dp::multisetSum( { 2 , 5 , 3 , 6 } , 11 ) );
 CHECK( dp::multisetSum( { 2 , 5 , 3 , 6 } , 1 ) == false );
 CHECK( dp::multisetSum( {} , 0 ) );
 CHECK( dp::multisetSum( {} , 5 ) == false );
 CHECK( dp::multisetSum( { 7 } , 7000 ) );
 CHECK( dp::multisetSum( { 4 , 6 } , 4001 ) == false );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 5 ) , 1 , 15 );
  const Int t = rnd( g , 0 , 60 );
  std::vector< char > reach( t + 1 , 0 );
  reach[ 0 ] = 1;
  for( Int w = 0 ; w <= t ; ++w )
   if( reach[ w ] )
    for( Int a : A )
     if( w + a <= t )
      reach[ w + a ] = 1;
  const bool s = dp::multisetSum( A , t );
  CHECK( s == bool( reach[ t ] ) );
  std::vector< long > coins( A.begin() , A.end() );
  CHECK( s == optimalChange( coins , t ).has_value() );
  }
 CHECK( dp::multisetSum( { 1 } , 0 ) && dp::multisetSum( { 3 , 3 } , 9 ) );
 CHECK( ! dp::multisetSum( { 5 } , 4 ) );
 CHECK_THROWS( dp::multisetSum( { 0 } , 3 ) , contract_violation );
 CHECK_THROWS( dp::multisetSum( { -2 , 3 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::multisetSum( { 1 } , -1 ) , contract_violation );
 }

namespace {

/// the groups as sorted lists of sorted strings, to compare partitions
std::vector< std::vector< std::string > > canonical(
                              std::vector< std::vector< std::string > > G )
{
 for( auto & group : G )
  std::sort( group.begin() , group.end() );
 std::sort( G.begin() , G.end() );
 return( G );
 }

}  // end( namespace )

// 2011-09-07, Anagrams: the example and random words over 3 letters
// against the quadratic grouping by std::is_permutation
ADS_TEST( "dp-5" )
{
 const auto G = anagramGroups( { "rosa" , "pippo" , "poppi" , "raso" ,
                                "orsa" , "giappone" } );
 CHECK( canonical( G ) == canonical( { { "rosa" , "raso" , "orsa" } ,
                                       { "pippo" , "poppi" } ,
                                       { "giappone" } } ) );
 CHECK( anagramGroups( {} ).empty() );
 CHECK( canonical( anagramGroups( { "ab" , "ab" , "ba" } ) ) ==
        canonical( { { "ab" , "ab" , "ba" } } ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  std::vector< std::string > S( rnd( g , 1 , 20 ) );
  for( auto & s : S )
   s = rndString( g , rnd( g , 0 , 4 ) , 3 );
  std::vector< std::vector< std::string > > B;
  std::vector< char > used( S.size() , 0 );
  for( std::size_t i = 0 ; i < S.size() ; ++i )
   if( ! used[ i ] ) {
    B.emplace_back();
    for( std::size_t j = i ; j < S.size() ; ++j )
     if( ( ! used[ j ] ) && ( S[ i ].size() == S[ j ].size() ) &&
         std::is_permutation( S[ i ].begin() , S[ i ].end() ,
                              S[ j ].begin() ) ) {
      used[ j ] = 1;
      B.back().push_back( S[ j ] );
      }
    }
  CHECK( canonical( anagramGroups( S ) ) == canonical( B ) );
  }
 }

namespace {

/// the cost of the best tree of the keys i .. j - 1 at depth d and below,
/// trying every root, no memoization
Int optimalBstBrute( const std::vector< Int > & p , int i , int j , Int d )
{
 if( i >= j )
  return( 0 );
 Int best = dp::INF;
 for( int r = i ; r < j ; ++r )
  best = std::min( best , p[ r ] * ( d + 1 ) +
                          optimalBstBrute( p , i , r , d + 1 ) +
                          optimalBstBrute( p , r + 1 , j , d + 1 ) );
 return( best );
 }

/// the cost of the tree T of root u at depth d
Int bstCost( const dp::OptimalBst & T , const std::vector< Int > & p , int u ,
             Int d )
{
 if( u < 0 )
  return( 0 );
 return( p[ u ] * ( d + 1 ) + bstCost( T , p , T.left[ u ] , d + 1 ) +
         bstCost( T , p , T.right[ u ] , d + 1 ) );
 }

}  // end( namespace )

// 2012-01-11, Chiavi frequenti: the cost of the optimal tree against all
// the trees of up to 9 keys, and the tree built has that cost
ADS_TEST( "dp-6" )
{
 const auto E = dp::optimalBst( {} );
 CHECK( ( E.cost == 0 ) && ( E.root == -1 ) );
 const auto one = dp::optimalBst( { 5 } );
 CHECK( ( one.cost == 5 ) && ( one.root == 0 ) );
 const auto three = dp::optimalBst( { 1 , 1 , 1 } );
 CHECK( ( three.cost == 5 ) && ( three.root == 1 ) );
 const auto skew = dp::optimalBst( { 1 , 1 , 100 } );
 CHECK( skew.root == 2 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const auto p = rndVector( g , rnd( g , 0 , 9 ) , 0 , 20 );
  const auto T = dp::optimalBst( p );
  CHECK( T.cost == optimalBstBrute( p , 0 , int( p.size() ) , 0 ) );
  CHECK( bstCost( T , p , T.root , 0 ) == T.cost );
  }
 const auto zero = dp::optimalBst( { 0 , 0 , 0 , 0 } );
 CHECK( ( zero.cost == 0 ) && ( bstCost( zero , { 0 , 0 , 0 , 0 } ,
                                         zero.root , 0 ) == 0 ) );
 const auto first = dp::optimalBst( { 100 , 1 , 1 } );
 CHECK( first.root == 0 );
 CHECK_THROWS( dp::optimalBst( { -1 } ) , contract_violation );
 CHECK_THROWS( dp::optimalBst( { 3 , -1 , 3 } ) , contract_violation );
 }

// 2012-01-11 and 2012-05-31, Mediana4: random sorted arrays of 1 to 40
// items, distinct and with repetitions, against std::merge
ADS_TEST( "dp-7" )
{
 CHECK( dp::medianPair( { 1 } , { 2 } ) == std::pair( Int( 1 ) , Int( 2 ) ) );
 CHECK( dp::medianPair( { 5 } , { 5 } ) == std::pair( Int( 5 ) , Int( 5 ) ) );
 CHECK( dp::medianPair( { 1 , 2 , 3 } , { 4 , 5 , 6 } ) ==
        std::pair( Int( 3 ) , Int( 4 ) ) );
 CHECK( dp::medianPair( { 4 , 5 , 6 } , { 1 , 2 , 3 } ) ==
        std::pair( Int( 3 ) , Int( 4 ) ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 40 );
  const Int top = ( r % 2 ) ? 1000000 : 6;
  auto X = rndVector( g , n , - top , top );
  auto Y = rndVector( g , n , - top , top );
  std::sort( X.begin() , X.end() );
  std::sort( Y.begin() , Y.end() );
  std::vector< Int > Z( 2 * n );
  std::merge( X.begin() , X.end() , Y.begin() , Y.end() , Z.begin() );
  CHECK( dp::medianPair( X , Y ) == std::pair( Z[ n - 1 ] , Z[ n ] ) );
  }
 CHECK_THROWS( dp::medianPair( { 2 , 1 } , { 1 , 2 } ) , contract_violation );
 CHECK_THROWS( dp::medianPair( {} , {} ) , contract_violation );
 CHECK_THROWS( dp::medianPair( { 1 } , { 1 , 2 } ) , contract_violation );
 CHECK_THROWS( dp::medianPair( { 1 , 2 } , { 2 , 1 } ) ,
               contract_violation );
 const Int lo = std::numeric_limits< Int >::min() ,
           hi = std::numeric_limits< Int >::max();
 CHECK( dp::medianPair( { lo , hi } , { lo , hi } ) == std::pair( lo , hi ) );
 CHECK( dp::medianPair( { lo , lo , lo } , { hi , hi , hi } ) ==
        std::pair( lo , hi ) );
 CHECK( dp::medianPair( std::vector< Int >( 9 , 4 ) ,
                        std::vector< Int >( 9 , 4 ) ) ==
        std::pair( Int( 4 ) , Int( 4 ) ) );
 }

// 2012-02-01, Best cut: random prices against all the partitions of the
// length from integerPartitions(), and the pieces sum to it
ADS_TEST( "dp-8" )
{
 CHECK( dp::bestCut( { 1 , 5 , 8 , 9 } , 4 ).first == 10 );
 CHECK( dp::bestCut( { 3 } , 0 ).second.empty() );
 CHECK( dp::bestCut( { 3 } , 1 ).first == 3 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const std::size_t L = rnd( g , 0 , 14 );
  const auto G = rndVector( g , std::max< std::size_t >( L , 1 ) , 0 , 30 );
  Int best = 0;
  integerPartitions( int( L ) , [ & ]( const Tuple & a ) {
   Int v = 0;
   for( int t : a )
    v += G[ t - 1 ];
   best = std::max( best , v );
   return( false );
   } );
  const auto [ gain , pieces ] = dp::bestCut( G , L );
  CHECK( gain == best );
  Int v = 0;
  std::size_t total = 0;
  for( std::size_t t : pieces ) {
   v += G[ t - 1 ];
   total += t;
   }
  CHECK( ( v == gain ) && ( total == L ) );
  }
 CHECK( dp::bestCut( { 0 , 0 } , 2 ).first == 0 );
 CHECK( dp::bestCut( { 1 , 100 } , 2 ).second ==
        std::vector< std::size_t >( 1 , 2 ) );
 CHECK( dp::bestCut( { 100 , 1 } , 2 ).second ==
        std::vector< std::size_t >( 2 , 1 ) );
 CHECK_THROWS( dp::bestCut( { 1 } , 2 ) , contract_violation );
 CHECK_THROWS( dp::bestCut( {} , 1 ) , contract_violation );
 CHECK_THROWS( dp::bestCut( { 1 , -1 } , 1 ) , contract_violation );
 }

// 2012-02-01, Prima elementare: the strings of n R and m G against
// std::next_permutation, C( n + m , n ) of them
ADS_TEST( "dp-9-1" )
{
 CHECK( dp::printRG( 0 , 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( dp::printRG( 2 , 0 ) == std::vector< std::string >( 1 , "RR" ) );
 CHECK( dp::printRG( 3 , 2 ).size() == 10 );
 for( int n = 0 ; n <= 6 ; ++n )
  for( int m = 0 ; m <= 6 ; ++m ) {
   auto out = dp::printRG( n , m );
   CHECK( out.size() == choose( n + m , n ) );
   std::sort( out.begin() , out.end() );
   std::string s = std::string( m , 'G' ) + std::string( n , 'R' );
   std::vector< std::string > all;
   do
    all.push_back( s );
   while( std::next_permutation( s.begin() , s.end() ) );
   CHECK( out == all );
   }
 CHECK( dp::printRG( 0 , 3 ) == std::vector< std::string >( 1 , "GGG" ) );
 CHECK( dp::printRG( 1 , 1 ) == ( std::vector< std::string >{ "RG" ,
                                                             "GR" } ) );
 CHECK_THROWS( dp::printRG( -1 , 2 ) , contract_violation );
 CHECK_THROWS( dp::printRG( 2 , -1 ) , contract_violation );
 }

// 2012-02-01, Prima elementare: the count of the strings against the
// triangle of Tartaglia for every n + m <= 67, the largest that fits
ADS_TEST( "dp-9-2" )
{
 const auto C = tartaglia( 67 );
 for( int n = 0 ; n <= 67 ; ++n )
  for( int m = 0 ; n + m <= 67 ; ++m )
   CHECK( dp::countRG( n , m ) == C[ n + m ][ n ] );
 CHECK( dp::countRG( 3 , 2 ) == 10 );
 CHECK_THROWS( dp::countRG( 34 , 34 ) , contract_violation );
 CHECK_THROWS( dp::countRG( 68 , 0 ) , contract_violation );
 CHECK_THROWS( dp::countRG( 0 , 68 ) , contract_violation );
 CHECK_THROWS( dp::countRG( -1 , 0 ) , contract_violation );
 CHECK_THROWS( dp::countRG( 0 , -1 ) , contract_violation );
 }

namespace {

/// whether the items of A split into pairs of the same sum, over all the
/// perfect matchings
bool pairsBrute( std::vector< Int > A , std::optional< Int > sum )
{
 if( A.empty() )
  return( true );
 const Int a = A.back();
 A.pop_back();
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  const Int s = a + A[ i ];
  if( sum && ( *sum != s ) )
   continue;
  auto B = A;
  B.erase( B.begin() + std::ptrdiff_t( i ) );
  if( pairsBrute( B , s ) )
   return( true );
  }
 return( false );
 }

}  // end( namespace )

// 2012-05-03, Il gioco delle coppie: the example, and random distinct
// items, often of a planted solution, against all the perfect matchings
ADS_TEST( "dp-10" )
{
 CHECK( dp::checkPairs( { 7 , 4 , 5 , 2 , 3 , 6 } ) );
 CHECK( dp::checkPairsHash( { 7 , 4 , 5 , 2 , 3 , 6 } ) );
 CHECK( dp::checkPairs( {} ) && dp::checkPairsHash( {} ) );
 CHECK( dp::checkPairs( { 1 , 9 } ) && dp::checkPairsHash( { 1 , 9 } ) );
 CHECK( ! dp::checkPairs( { 1 , 2 , 3 , 5 } ) );
 CHECK( ! dp::checkPairsHash( { 1 , 2 , 3 , 5 } ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 600 ; ++r ) {
  std::vector< Int > A;
  const std::size_t h = rnd( g , 1 , 5 );
  std::set< Int > used;
  if( r % 2 ) {                                 // planted: pairs of sum S
   const Int S = rnd( g , -20 , 20 );
   while( A.size() < 2 * h ) {
    const Int a = rnd( g , -30 , 30 );
    if( ( 2 * a != S ) && ! used.count( a ) && ! used.count( S - a ) ) {
     used.insert( a );
     used.insert( S - a );
     A.push_back( a );
     A.push_back( S - a );
     }
    }
   if( r % 4 == 1 )
    A.back() += 100;
   }
  else
   while( A.size() < 2 * h ) {
    const Int a = rnd( g , -8 , 8 );
    if( used.insert( a ).second )
     A.push_back( a );
    }
  std::shuffle( A.begin() , A.end() , g );
  const bool b = pairsBrute( A , std::nullopt );
  CHECK( dp::checkPairs( A ) == b );
  CHECK( dp::checkPairsHash( A ) == b );
  }
 CHECK_THROWS( dp::checkPairs( { 1 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( dp::checkPairsHash( { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( dp::checkPairsHash( { 1 } ) , contract_violation );
 const Int lo = std::numeric_limits< Int >::min() ,
           hi = std::numeric_limits< Int >::max();
 CHECK( dp::checkPairs( { lo , hi , lo + 1 , hi - 1 } ) );
 CHECK( dp::checkPairsHash( { lo , hi , lo + 1 , hi - 1 } ) );
 CHECK( ! dp::checkPairs( { lo , hi , 0 , 1 } ) );
 }

namespace {

/// the largest sum <= W of multiples of V, by the sums reachable
Int maxSumBrute( const std::vector< Int > & V , Int W )
{
 std::vector< char > reach( W + 1 , 0 );
 reach[ 0 ] = 1;
 Int best = 0;
 for( Int w = 0 ; w <= W ; ++w )
  if( reach[ w ] ) {
   best = w;
   for( Int v : V )
    if( w + v <= W )
     reach[ w + v ] = 1;
   }
 return( best );
 }

}  // end( namespace )

// 2012-05-03, Ottimizza la somma: the example with X = 15, and random
// instances against the sums reachable
ADS_TEST( "dp-11-1" )
{
 const std::vector< Int > V = { 18 , 3 , 21 , 9 , 12 , 24 };
 CHECK( dp::maxSum( V , 17 ).first == 15 );
 CHECK( dp::maxSum( V , 0 ).first == 0 );
 CHECK( dp::maxSum( V , 2 ).first == 0 );
 CHECK( dp::maxSum( {} , 10 ).first == 0 );
 CHECK( dp::maxSum( { 1 } , 3000 ).first == 3000 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 6 ) , 1 , 30 );
  const Int W = rnd( g , 0 , 80 );
  CHECK( dp::maxSum( A , W ).first == maxSumBrute( A , W ) );
  }
 CHECK( dp::maxSum( { 5 , 5 } , 4 ).first == 0 );
 CHECK( dp::maxSum( { 7 } , 7 ).first == 7 );
 CHECK_THROWS( dp::maxSum( { 0 } , 3 ) , contract_violation );
 CHECK_THROWS( dp::maxSum( { 2 , -1 } , 3 ) , contract_violation );
 CHECK_THROWS( dp::maxSum( { 1 } , -1 ) , contract_violation );
 }

// 2012-05-03, Ottimizza la somma: the multiplicities of the example and of
// random instances are not negative, and sum to the largest value <= W
ADS_TEST( "dp-11-2" )
{
 const std::vector< Int > V = { 18 , 3 , 21 , 9 , 12 , 24 };
 const std::vector< Int > text = { 0 , 2 , 0 , 1 , 0 , 0 };
 CHECK( std::inner_product( text.begin() , text.end() , V.begin() ,
                            Int( 0 ) ) == 15 );
 const auto [ X , x ] = dp::maxSum( V , 17 );
 CHECK( ( X == 15 ) && ( x.size() == V.size() ) );
 CHECK( std::inner_product( x.begin() , x.end() , V.begin() , Int( 0 ) )
        == 15 );
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 6 ) , 1 , 30 );
  const Int W = rnd( g , 0 , 80 );
  const auto [ best , m ] = dp::maxSum( A , W );
  CHECK( std::all_of( m.begin() , m.end() ,
                      []( Int c ) { return( c >= 0 ); } ) );
  CHECK( std::inner_product( m.begin() , m.end() , A.begin() , Int( 0 ) )
         == maxSumBrute( A , W ) );
  }
 }

namespace {

/// the multisets of B of sum T, over all the multiplicities
Count changeWaysBrute( const std::vector< Int > & B , std::size_t i , Int T )
{
 if( i == B.size() )
  return( T == 0 ? 1 : 0 );
 Count c = 0;
 for( Int t = T ; t >= 0 ; t -= B[ i ] )
  c += changeWaysBrute( B , i + 1 , t );
 return( c );
 }

}  // end( namespace )

// 2012-05-31, Resto: the two examples, and random banknotes against all
// the vectors of multiplicities
ADS_TEST( "dp-13" )
{
 CHECK( dp::changeWays( { 1 , 2 , 5 } , 4 ) == 3 );
 CHECK( dp::changeWays( { 5 } , 7 ) == 0 );
 CHECK( dp::changeWays( {} , 0 ) == 1 );
 CHECK( dp::changeWays( { 3 } , 0 ) == 1 );
 CHECK( dp::changeWays( { 1 , 2 } , 1000 ) == 501 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  std::set< Int > distinct;
  const std::size_t n = rnd( g , 0 , 4 );
  while( distinct.size() < n )
   distinct.insert( rnd( g , 1 , 9 ) );
  const std::vector< Int > B( distinct.begin() , distinct.end() );
  const Int T = rnd( g , 0 , 40 );
  CHECK( dp::changeWays( B , T ) == changeWaysBrute( B , 0 , T ) );
  }
 CHECK( dp::changeWays( {} , 5 ) == 0 );
 CHECK( dp::changeWays( { 1 } , 1000 ) == 1 );
 CHECK( dp::changeWays( { 7 } , 7 ) == 1 && dp::changeWays( { 7 } , 6 ) == 0 );
 CHECK_THROWS( dp::changeWays( { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::changeWays( { 0 } , 3 ) , contract_violation );
 }

// 2012-06-18, Scacchiera: random boards of up to 5 rows against all the
// 2^( 3 n ) placements
ADS_TEST( "dp-14" )
{
 CHECK( dp::boardTokens( {} ) == 0 );
 CHECK( dp::boardTokens( { { 1 , 1 , 1 } } ) == 2 );
 CHECK( dp::boardTokens( { { 1 , 10 , 1 } } ) == 10 );
 CHECK( dp::boardTokens( { { 1 , 1 , 1 } , { 1 , 1 , 1 } } ) == 3 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  std::vector< std::array< Int , 3 > > A( rnd( g , 1 , 5 ) );
  for( auto & row : A )
   for( auto & c : row )
    c = rnd( g , 1 , 20 );
  const std::size_t n = A.size();
  Int best = 0;
  for( std::uint32_t mask = 0 ; mask < ( 1u << ( 3 * n ) ) ; ++mask ) {
   auto on = [ & ]( std::size_t i , int x ) {
    return( ( mask >> ( 3 * i + x ) ) & 1 ); };
   bool ok = true;
   Int v = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    for( int x = 0 ; x < 3 ; ++x )
     if( on( i , x ) ) {
      v += A[ i ][ x ];
      ok = ok && ! ( ( x < 2 ) && on( i , x + 1 ) ) &&
           ! ( ( i + 1 < n ) && on( i + 1 , x ) );
      }
   if( ok )
    best = std::max( best , v );
   }
  CHECK( dp::boardTokens( A ) == best );
  }
 CHECK( dp::boardTokens( { { 5 , 1 , 5 } , { 1 , 9 , 1 } } ) == 19 );
 std::vector< std::array< Int , 3 > > tall( 1000 , { 1 , 1 , 1 } );
 CHECK( dp::boardTokens( tall ) == 1500 );
 CHECK_THROWS( dp::boardTokens( { { 0 , 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( dp::boardTokens( { { 1 , 1 , 1 } , { 1 , 1 , -1 } } ) ,
               contract_violation );
 }

namespace {

/// the segmentations of X[ i .. ] by plain recursion
Count segmentationsBrute( const std::string & X ,
                          const std::vector< std::string > & S ,
                          std::size_t i )
{
 if( i == X.size() )
  return( 1 );
 Count c = 0;
 for( const auto & s : S )
  if( X.substr( i , s.size() ) == s )
   c += segmentationsBrute( X , S , i + s.size() );
 return( c );
 }

}  // end( namespace )

// 2012-07-09, Stringhe primitive: the two examples, and random binary
// strings of up to 18 characters against the plain recursion
ADS_TEST( "dp-15" )
{
 const std::vector< std::string > S = { "01" , "10" , "011" , "101" };
 CHECK( countSegmentations( "0111010101" , S ) == 3 );
 CHECK( countSegmentations( "0110001" , S ) == 0 );
 CHECK( countSegmentations( "" , S ) == 1 );
 CHECK( countSegmentations( "01" , {} ) == 0 );
 CHECK( countSegmentations( std::string( 90 , 'a' ) , { "a" , "aa" } ) ==
        4660046610375530309ULL );               // Fibonacci( 91 )
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  std::vector< std::string > P( rnd( g , 1 , 5 ) );
  for( auto & p : P ) {
   p.assign( rnd( g , 1 , 3 ) , '0' );
   for( auto & c : p )
    c = static_cast< char >( '0' + rnd( g , 0 , 1 ) );
   }
  std::string X( rnd( g , 0 , 18 ) , '0' );
  for( auto & c : X )
   c = static_cast< char >( '0' + rnd( g , 0 , 1 ) );
  CHECK( countSegmentations( X , P ) == segmentationsBrute( X , P , 0 ) );
  }
 CHECK( countSegmentations( "0" , { "0" , "0" } ) == 2 );
 CHECK( countSegmentations( "01" , { "011" } ) == 0 );
 CHECK( countSegmentations( "011" , { "011" } ) == 1 );
 CHECK_THROWS( countSegmentations( "0" , { "" } ) , contract_violation );
 }

namespace {

/// the right and down paths from ( i , j ) to the corner, by search
Count countPathsBrute( const std::vector< std::vector< int > > & B ,
                       std::size_t i , std::size_t j )
{
 if( ( i >= B.size() ) || ( j >= B[ 0 ].size() ) || ! B[ i ][ j ] )
  return( 0 );
 if( ( i + 1 == B.size() ) && ( j + 1 == B[ 0 ].size() ) )
  return( 1 );
 return( countPathsBrute( B , i + 1 , j ) + countPathsBrute( B , i , j + 1 ) );
 }

}  // end( namespace )

// 2013-01-07, Conta percorsi: random boards with blocked cells against the
// search of all the paths, and C( m + n - 2 , m - 1 ) on the free boards
ADS_TEST( "dp-16" )
{
 CHECK( dp::countPaths( { { 1 } } ) == 1 );
 CHECK( dp::countPaths( { { 0 } } ) == 0 );
 CHECK( dp::countPaths( { { 1 , 1 } , { 1 , 0 } } ) == 0 );
 CHECK( dp::countPaths( { { 1 , 0 } , { 1 , 1 } } ) == 1 );
 for( int m = 1 ; m <= 30 ; ++m )
  for( int n = 1 ; n <= 30 ; ++n )
   CHECK( dp::countPaths( std::vector< std::vector< int > >(
           m , std::vector< int >( n , 1 ) ) ) == choose( m + n - 2 ,
                                                          m - 1 ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const std::size_t m = rnd( g , 1 , 7 );
  std::vector< std::vector< int > > B( m ,
                               std::vector< int >( rnd( g , 1 , 7 ) ) );
  for( auto & row : B )
   for( auto & c : row )
    c = rnd( g , 0 , 4 ) > 0;
  CHECK( dp::countPaths( B ) == countPathsBrute( B , 0 , 0 ) );
  }
 CHECK( dp::countPaths( { { 0 , 1 } , { 1 , 1 } } ) == 0 );
 CHECK( dp::countPaths( { { 1 , 1 , 1 , 0 , 1 } } ) == 0 );
 CHECK( dp::countPaths( { { 1 } , { 1 } , { 1 } } ) == 1 );
 CHECK_THROWS( dp::countPaths( {} ) , contract_violation );
 CHECK_THROWS( dp::countPaths( { {} } ) , contract_violation );
 }

// 2013-01-28, Intero positivo insieme: subsetSumDP() of the library
// against all the 2^k subsets, and the edges
ADS_TEST( "dp-17" )
{
 CHECK( subsetSumDP( {} , 0 ) );
 CHECK( ! subsetSumDP( {} , 1 ) );
 CHECK( subsetSumDP( { 3 , 34 , 4 , 12 , 5 , 2 } , 9 ) );
 CHECK( ! subsetSumDP( { 3 , 34 , 4 , 12 , 5 , 2 } , 30 ) );
 CHECK( ! subsetSumDP( { 5 , 5 , 5 } , 20 ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::size_t k = rnd( g , 0 , 10 );
  std::vector< long > A( k );
  for( auto & a : A )
   a = static_cast< long >( rnd( g , 1 , 25 ) );
  const long T = static_cast< long >( rnd( g , 0 , 80 ) );
  bool b = false;
  for( std::uint32_t mask = 0 ; mask < ( 1u << k ) ; ++mask ) {
   long s = 0;
   for( std::size_t i = 0 ; i < k ; ++i )
    if( ( mask >> i ) & 1 )
     s += A[ i ];
   b = b || ( s == T );
   }
  CHECK( subsetSumDP( A , T ) == b );
  }
 CHECK( subsetSumDP( { 0 , 0 } , 0 ) && ! subsetSumDP( { 0 } , 1 ) );
 CHECK( subsetSumDP( { 7 } , 7 ) && ! subsetSumDP( { 7 } , 14 ) );
 CHECK( subsetSumDP( { 3 , 34 , 4 , 12 , 5 , 2 } , 60 ) );
 CHECK( ! subsetSumDP( { 3 , 34 , 4 , 12 , 5 , 2 } , 61 ) );
 CHECK_THROWS( subsetSumDP( { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( subsetSumDP( { 2 , -1 } , 1 ) , contract_violation );
 }

// 2013-05-03, Barche sul fiume: random prices of up to 10 ports against
// all the sets of ports where the boat is changed; the ports read back
// cost the least
ADS_TEST( "dp-18" )
{
 CHECK( dp::boats( { { 0 } } ) ==
        std::pair( Int( 0 ) , std::vector< std::size_t >() ) );
 const dp::Table C3 = { { 0 , 2 , 10 } , { 0 , 0 , 3 } , { 0 , 0 , 0 } };
 CHECK( dp::boats( C3 ) ==
        std::pair( Int( 5 ) , std::vector< std::size_t >( { 0 , 1 } ) ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 10 );
  dp::Table C( n , std::vector< Int >( n , 0 ) );
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = i + 1 ; j < n ; ++j )
    C[ i ][ j ] = rnd( g , 0 , 50 );
  Int best = dp::INF;
  const std::size_t inner = ( n >= 2 ) ? n - 2 : 0;
  for( std::uint32_t mask = 0 ; mask < ( 1u << inner ) ; ++mask ) {
   std::size_t at = 0;
   Int cost = 0;
   for( std::size_t k = 1 ; k < n ; ++k )
    if( ( k == n - 1 ) || ( ( mask >> ( k - 1 ) ) & 1 ) ) {
     cost += C[ at ][ k ];
     at = k;
     }
   best = std::min( best , cost );
   }
  const auto [ cost , ports ] = dp::boats( C );
  CHECK( cost == ( ( n == 1 ) ? 0 : best ) );
  Int again = 0;
  for( std::size_t k = 0 ; k < ports.size() ; ++k )
   again += C[ ports[ k ] ][ ( k + 1 < ports.size() ) ? ports[ k + 1 ]
                                                      : n - 1 ];
  CHECK( ( ports.empty() == ( n == 1 ) ) && ( again == cost ) );
  CHECK( ports.empty() || ( ports.front() == 0 ) );
  }
 const dp::Table Z( 4 , std::vector< Int >( 4 , 0 ) );
 CHECK( dp::boats( Z ).first == 0 );
 const dp::Table W = { { 0 , 9 } , { 0 , 0 } };
 CHECK( dp::boats( W ) ==
        std::pair( Int( 9 ) , std::vector< std::size_t >( 1 , 0 ) ) );
 CHECK_THROWS( dp::boats( {} ) , contract_violation );
 CHECK_THROWS( dp::boats( { { 0 } , { 0 , 0 } } ) , contract_violation );
 CHECK_THROWS( dp::boats( { { 0 , -1 } , { 0 , 0 } } ) ,
               contract_violation );
 }

// 2013-05-31, Max resto: the examples, where the least coins first fails,
// and random coins against all the subsets
ADS_TEST( "dp-19" )
{
 CHECK( dp::maxCoins( { 2 , 1 , 5 , 2 , 2 , 5 } , 7 ) == 4 );
 CHECK( dp::maxCoins( { 1 , 2 , 2 , 2 } , 6 ) == 3 );
 CHECK( dp::maxCoins( { 3 } , 0 ) == 0 );
 CHECK( ! dp::maxCoins( {} , 1 ) );
 CHECK( ! dp::maxCoins( { 2 , 4 } , 5 ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto V = rndVector( g , rnd( g , 0 , 12 ) , 1 , 10 );
  const Int T = rnd( g , 0 , 40 );
  std::optional< Int > best;
  for( std::uint32_t mask = 0 ; mask < ( 1u << V.size() ) ; ++mask ) {
   Int s = 0 , c = 0;
   for( std::size_t i = 0 ; i < V.size() ; ++i )
    if( ( mask >> i ) & 1 ) {
     s += V[ i ];
     ++c;
     }
   if( ( s == T ) && ( ! best || ( c > *best ) ) )
    best = c;
   }
  CHECK( dp::maxCoins( V , T ) == best );
  }
 CHECK( dp::maxCoins( {} , 0 ) == 0 );
 CHECK( dp::maxCoins( std::vector< Int >( 12 , 1 ) , 12 ) == 12 );
 CHECK( ! dp::maxCoins( std::vector< Int >( 12 , 1 ) , 13 ) );
 CHECK_THROWS( dp::maxCoins( { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::maxCoins( { 0 } , 1 ) , contract_violation );
 }

namespace {

/// the largest sum of D with no two neighbours, over all the subsets
Int fundraisingBrute( const std::vector< Int > & D )
{
 Int best = 0;
 for( std::uint32_t mask = 0 ; mask < ( 1u << D.size() ) ; ++mask )
  if( ( mask & ( mask >> 1 ) ) == 0 ) {
   Int s = 0;
   for( std::size_t i = 0 ; i < D.size() ; ++i )
    if( ( mask >> i ) & 1 )
     s += D[ i ];
   best = std::max( best , s );
   }
 return( best );
 }

}  // end( namespace )

// 2013-06-17, Fundraising: random donations of up to 16 houses against all
// the subsets with no two neighbours
ADS_TEST( "dp-20-1" )
{
 CHECK( dp::fundraising( {} ) == 0 );
 CHECK( dp::fundraising( { 4 } ) == 4 );
 CHECK( dp::fundraising( { 4 , 9 } ) == 9 );
 CHECK( dp::fundraising( { 5 , 5 , 5 , 5 , 5 } ) == 15 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto D = rndVector( g , rnd( g , 0 , 16 ) , 0 , 30 );
  CHECK( dp::fundraising( D ) == fundraisingBrute( D ) );
  }
 CHECK_THROWS( dp::fundraising( { -1 } ) , contract_violation );
 }

// 2013-06-17, Fundraising: the houses read back have no two neighbours and
// give the largest sum
ADS_TEST( "dp-20-2" )
{
 CHECK( dp::donors( {} ).empty() );
 CHECK( dp::donors( { 4 } ) == std::vector< std::size_t >( 1 , 0 ) );
 CHECK( dp::donors( { 4 , 9 } ) == std::vector< std::size_t >( 1 , 1 ) );
 CHECK( dp::donors( { 5 , 1 , 1 , 5 } ) ==
        std::vector< std::size_t >( { 0 , 3 } ) );
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto D = rndVector( g , rnd( g , 0 , 16 ) , 0 , 30 );
  const auto H = dp::donors( D );
  Int s = 0;
  for( std::size_t i : H )
   s += D[ i ];
  CHECK( s == fundraisingBrute( D ) );
  }
 for( std::size_t i : dp::donors( std::vector< Int >( 9 , 0 ) ) )
  CHECK( i < 9 );
 const auto H = dp::donors( std::vector< Int >( 1001 , 1 ) );
 CHECK( H.size() == 501 );
 for( std::size_t k = 1 ; k < H.size() ; ++k )
  CHECK( H[ k ] >= H[ k - 1 ] + 2 );
 CHECK_THROWS( dp::donors( { 1 , -1 } ) , contract_violation );
 }

namespace {

/// the most the player to move gets from V[ i .. j - 1 ], by minimax with
/// the sums, no memoization
Int coinGameBrute( const std::vector< Int > & V , std::size_t i ,
                   std::size_t j )
{
 if( i >= j )
  return( 0 );
 const Int sum = std::accumulate( V.begin() + std::ptrdiff_t( i ) ,
                                  V.begin() + std::ptrdiff_t( j ) , Int( 0 ) );
 return( std::max( sum - coinGameBrute( V , i + 1 , j ) ,
                   sum - coinGameBrute( V , i , j - 1 ) ) );
 }

}  // end( namespace )

// 2013-07-22, Gioco di monete: random rows of up to 14 coins against the
// minimax by the sums
ADS_TEST( "dp-21" )
{
 CHECK( dp::coinGame( {} ) == 0 );
 CHECK( dp::coinGame( { 3 , 8 } ) == 8 );
 CHECK( dp::coinGame( { 5 , 3 , 7 , 10 } ) == 15 );
 CHECK( dp::coinGame( { 2 , 2 , 2 , 2 } ) == 4 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const auto V = rndVector( g , 2 * rnd( g , 0 , 7 ) , 0 , 30 );
  CHECK( dp::coinGame( V ) == coinGameBrute( V , 0 , V.size() ) );
  }
 CHECK( dp::coinGame( { 0 , 0 } ) == 0 );
 CHECK( dp::coinGame( { 1 , 100 , 2 , 3 } ) == 103 );
 CHECK_THROWS( dp::coinGame( { 1 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( dp::coinGame( { 1 } ) , contract_violation );
 CHECK_THROWS( dp::coinGame( { 1 , -2 } ) , contract_violation );
 }

namespace {

/// the multisets of values of sum X over all the outcomes of the dice
long long diceBrute( const std::vector< int > & F , int X )
{
 std::set< std::vector< int > > seen;
 std::vector< int > v( F.size() , 1 );
 for( ; ; ) {
  if( std::accumulate( v.begin() , v.end() , 0 ) == X ) {
   auto w = v;
   std::sort( w.begin() , w.end() );
   seen.insert( w );
   }
  std::size_t i = 0;
  while( ( i < v.size() ) && ( v[ i ] == F[ i ] ) )
   v[ i++ ] = 1;
  if( i == v.size() )
   break;
  ++v[ i ];
  }
 return( static_cast< long long >( seen.size() ) );
 }

}  // end( namespace )

// 2013-09-02, Dice rec: the two examples, and random dice of different
// faces, in any order, against the sorted outcomes; both recurrences
ADS_TEST( "dp-22" )
{
 for( bool fast : { false , true } ) {
  CHECK( dp::dice( { 4 , 4 } , 7 , fast ) == 1 );
  CHECK( dp::dice( { 4 , 4 , 4 } , 6 , fast ) == 3 );
  CHECK( dp::dice( {} , 0 , fast ) == 1 );
  CHECK( dp::dice( {} , 3 , fast ) == 0 );
  CHECK( dp::dice( { 6 } , 0 , fast ) == 0 );
  CHECK( dp::dice( { 6 , 6 } , 13 , fast ) == 0 );
  CHECK( dp::dice( { 1 , 1 , 1 } , 3 , fast ) == 1 );
  }
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  std::vector< int > F( rnd( g , 0 , 4 ) );
  for( auto & f : F )
   f = static_cast< int >( rnd( g , 1 , 6 ) );
  const int X = static_cast< int >( rnd( g , 0 , 20 ) );
  const long long b = diceBrute( F , X );
  CHECK( dp::dice( F , X , false ) == b );
  CHECK( dp::dice( F , X , true ) == b );
  }
 for( bool fast : { false , true } ) {
  CHECK( dp::dice( { 6 , 6 } , 12 , fast ) == 1 );
  CHECK( dp::dice( { 6 , 6 } , 2 , fast ) == 1 );
  CHECK( dp::dice( { 6 , 6 } , 1 , fast ) == 0 );
  CHECK( dp::dice( { 1 } , 1 , fast ) == 1 );
  }
 CHECK_THROWS( dp::dice( { 0 } , 1 , true ) , contract_violation );
 CHECK_THROWS( dp::dice( { 0 } , 1 , false ) , contract_violation );
 CHECK_THROWS( dp::dice( { 6 } , -1 , true ) , contract_violation );
 CHECK_THROWS( dp::dice( { 6 , -6 } , 3 , false ) , contract_violation );
 }

namespace {

/// the least total height over all the partitions of the books in shelves
/// of at most L, by the restricted growth strings
Int shelvesBrute( const std::vector< Int > & y , std::size_t L )
{
 const std::size_t n = y.size();
 if( n == 0 )
  return( 0 );
 Int best = dp::INF;
 std::vector< std::size_t > a( n , 0 );
 std::function< void( std::size_t , std::size_t ) > rec =
  [ & ]( std::size_t i , std::size_t blocks ) {
   if( i == n ) {
    std::vector< std::size_t > size( blocks , 0 );
    std::vector< Int > top( blocks , 0 );
    for( std::size_t k = 0 ; k < n ; ++k ) {
     ++size[ a[ k ] ];
     top[ a[ k ] ] = std::max( top[ a[ k ] ] , y[ k ] );
     }
    if( std::all_of( size.begin() , size.end() ,
                     [ L ]( std::size_t s ) { return( s <= L ); } ) )
     best = std::min( best , std::accumulate( top.begin() , top.end() ,
                                              Int( 0 ) ) );
    return;
    }
   for( std::size_t b = 0 ; b <= blocks ; ++b ) {
    a[ i ] = b;
    rec( i + 1 , std::max( blocks , b + 1 ) );
    }
   };
 rec( 0 , 0 );
 return( best );
 }

}  // end( namespace )

// 2014-01-07, Altezza: random heights of up to 8 books against all the
// partitions in shelves
ADS_TEST( "dp-23" )
{
 CHECK( dp::shelvesHeight( {} , 3 ) == 0 );
 CHECK( dp::shelvesHeight( { 7 } , 1 ) == 7 );
 CHECK( dp::shelvesHeight( { 4 , 4 , 4 , 4 } , 3 ) == 8 );
 CHECK( dp::shelvesHeight( { 1 , 2 , 3 , 4 , 5 } , 2 ) == 9 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const auto y = rndVector( g , rnd( g , 0 , 8 ) , 1 , 20 );
  const std::size_t L = rnd( g , 1 , 4 );
  CHECK( dp::shelvesHeight( y , L ) == shelvesBrute( y , L ) );
  }
 CHECK( dp::shelvesHeight( { 3 , 1 , 2 } , 1 ) == 6 );
 CHECK( dp::shelvesHeight( { 3 , 1 , 2 } , 3 ) == 3 );
 CHECK( dp::shelvesHeight( { 3 , 1 , 2 } , 1000 ) == 3 );
 CHECK_THROWS( dp::shelvesHeight( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( dp::shelvesHeight( {} , 0 ) , contract_violation );
 }

// 2014-02-03, Max gain: random boards of 4 rows and up to 9 columns
// against all the choices of 4 columns and of the rows on them
ADS_TEST( "dp-24" )
{
 {
  std::array< std::vector< Int > , 4 > S;
  for( int i = 0 ; i < 4 ; ++i )
   S[ i ].assign( 4 , 1 );
  CHECK( dp::rooks( S ) == 4 );
  S[ 2 ][ 3 ] = 50;
  CHECK( dp::rooks( S ) == 53 );
  }
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const std::size_t n = rnd( g , 4 , 9 );
  std::array< std::vector< Int > , 4 > S;
  for( auto & row : S )
   row = rndVector( g , n , 1 , 40 );
  Int best = 0;
  std::array< int , 4 > rows = { 0 , 1 , 2 , 3 };
  for( std::size_t a = 0 ; a < n ; ++a )
   for( std::size_t b = a + 1 ; b < n ; ++b )
    for( std::size_t c = b + 1 ; c < n ; ++c )
     for( std::size_t d = c + 1 ; d < n ; ++d ) {
      std::sort( rows.begin() , rows.end() );
      do
       best = std::max( best , S[ rows[ 0 ] ][ a ] + S[ rows[ 1 ] ][ b ] +
                               S[ rows[ 2 ] ][ c ] + S[ rows[ 3 ] ][ d ] );
      while( std::next_permutation( rows.begin() , rows.end() ) );
      }
  CHECK( dp::rooks( S ) == best );
  }
 std::array< std::vector< Int > , 4 > small;
 for( auto & row : small )
  row.assign( 3 , 1 );
 CHECK_THROWS( dp::rooks( small ) , contract_violation );
 std::array< std::vector< Int > , 4 > ragged;
 for( auto & row : ragged )
  row.assign( 5 , 1 );
 ragged[ 3 ].pop_back();
 CHECK_THROWS( dp::rooks( ragged ) , contract_violation );
 ragged[ 3 ].push_back( 0 );
 CHECK_THROWS( dp::rooks( ragged ) , contract_violation );
 }

namespace {

/// the least cost of V in k parts over all the sets of k - 1 cuts
Int partitionBrute( const std::vector< Int > & V , std::size_t k )
{
 const std::size_t n = V.size();
 Int best = dp::INF;
 for( std::uint32_t mask = 0 ; mask < ( 1u << ( n - 1 ) ) ; ++mask ) {
  if( std::popcount( mask ) != int( k - 1 ) )
   continue;
  Int part = 0 , worst = - dp::INF;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   part += V[ i ];
   if( ( i + 1 == n ) || ( ( mask >> i ) & 1 ) ) {
    worst = std::max( worst , part );
    part = 0;
    }
   }
  best = std::min( best , worst );
  }
 return( best );
 }

}  // end( namespace )

// 2014-06-05, Partition k, k = 2: the example array and random arrays
// with negative items against all the cuts
ADS_TEST( "dp-25-1" )
{
 const std::vector< Int > V = { 2 , 3 , 7 , -7 , 15 , 2 };
 CHECK( dp::partition2( V ) == partitionBrute( V , 2 ) );
 CHECK( dp::partition2( { 4 , 9 } ) == 9 );
 CHECK( dp::partition2( { -3 , -3 } ) == -3 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 2 , 12 ) , -20 , 20 );
  CHECK( dp::partition2( A ) == partitionBrute( A , 2 ) );
  }
 CHECK_THROWS( dp::partition2( { 1 } ) , contract_violation );
 }

// 2014-06-05, Partition k, k = 3: the example, cost 10, and random arrays
// against all the pairs of cuts
ADS_TEST( "dp-25-2" )
{
 CHECK( dp::partition3( { 2 , 3 , 7 , -7 , 15 , 2 } ) == 10 );
 CHECK( dp::partition3( { 1 , 1 , 1 } ) == 1 );
 std::mt19937 g( 2 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 3 , 12 ) , -20 , 20 );
  CHECK( dp::partition3( A ) == partitionBrute( A , 3 ) );
  }
 CHECK_THROWS( dp::partition3( { 1 , 2 } ) , contract_violation );
 }

// 2014-06-05, Partition k: the example with k = 3, cost 10, and random
// arrays and k against all the sets of cuts, and against partition2() and
// partition3()
ADS_TEST( "dp-25-3" )
{
 const std::vector< Int > V = { 2 , 3 , 7 , -7 , 15 , 2 };
 CHECK( dp::partitionK( V , 3 ) == 10 );
 CHECK( dp::partitionK( V , 1 ) == 22 );
 CHECK( dp::partitionK( V , 6 ) == 15 );
 CHECK( dp::partitionK( { 5 } , 1 ) == 5 );
 std::mt19937 g( 3 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 1 , 12 ) , -20 , 20 );
  const std::size_t k = rnd( g , 1 , Int( A.size() ) );
  CHECK( dp::partitionK( A , k ) == partitionBrute( A , k ) );
  if( A.size() >= 2 )
   CHECK( dp::partitionK( A , 2 ) == dp::partition2( A ) );
  if( A.size() >= 3 )
   CHECK( dp::partitionK( A , 3 ) == dp::partition3( A ) );
  }
 CHECK_THROWS( dp::partitionK( V , 7 ) , contract_violation );
 CHECK_THROWS( dp::partitionK( V , 0 ) , contract_violation );
 }

// 2014-06-16, Input file file: the counterexample to the order by
// decreasing probability, 100.49 against 52
ADS_TEST( "dp-26-1" )
{
 const std::vector< double > L = { 100 , 1 } , P = { 0.51 , 0.49 };
 const double greedy = dp::tapeCost( L , P , { 0 , 1 } );
 const double other = dp::tapeCost( L , P , { 1 , 0 } );
 CHECK( std::abs( greedy - 100.49 ) < 1e-9 );
 CHECK( std::abs( other - 52 ) < 1e-9 );
 CHECK( other < greedy );
 CHECK( dp::tapeOrder( L , P ) == std::vector< std::size_t >( { 1 , 0 } ) );
 CHECK( dp::tapeCost( {} , {} , {} ) == 0 );
 CHECK_THROWS( dp::tapeCost( L , P , { 0 } ) , contract_violation );
 CHECK_THROWS( dp::tapeCost( L , { 1 } , { 0 , 1 } ) , contract_violation );
 }

// 2014-06-16, Input file file: the counterexample to the order by
// increasing length, 125 against 112.5
ADS_TEST( "dp-26-2" )
{
 const std::vector< double > L = { 50 , 100 } , P = { 0.25 , 0.75 };
 const double greedy = dp::tapeCost( L , P , { 0 , 1 } );
 const double other = dp::tapeCost( L , P , { 1 , 0 } );
 CHECK( std::abs( greedy - 125 ) < 1e-9 );
 CHECK( std::abs( other - 112.5 ) < 1e-9 );
 CHECK( other < greedy );
 CHECK( dp::tapeOrder( L , P ) == std::vector< std::size_t >( { 1 , 0 } ) );
 }

// 2014-06-16, Input file file: the order by increasing L / P against all
// the orders of up to 7 files of integer lengths and weights
ADS_TEST( "dp-26-3" )
{
 CHECK( dp::tapeOrder( {} , {} ).empty() );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 7 );
  std::vector< double > L( n ) , P( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   L[ i ] = double( rnd( g , 0 , 30 ) );
   P[ i ] = double( rnd( g , 1 , 30 ) );
   }
  std::vector< std::size_t > order( n );
  std::iota( order.begin() , order.end() , std::size_t( 0 ) );
  double best = std::numeric_limits< double >::max();
  do
   best = std::min( best , dp::tapeCost( L , P , order ) );
  while( std::next_permutation( order.begin() , order.end() ) );
  CHECK( dp::tapeCost( L , P , dp::tapeOrder( L , P ) ) == best );
  }
 CHECK( dp::tapeOrder( { 5 } , { 1 } ) == std::vector< std::size_t >( 1 ,
                                                                    0 ) );
 const auto Z = dp::tapeOrder( { 0 , 0 , 0 } , { 1 , 2 , 3 } );
 CHECK( dp::tapeCost( { 0 , 0 , 0 } , { 1 , 2 , 3 } , Z ) == 0 );
 CHECK_THROWS( dp::tapeOrder( { 1 } , { 0 } ) , contract_violation );
 CHECK_THROWS( dp::tapeOrder( { 1 , 2 } , { 1 } ) , contract_violation );
 CHECK_THROWS( dp::tapeOrder( { -1 } , { 1 } ) , contract_violation );
 }

namespace {

/// the least recharge time over all the vectors of recharges
std::optional< Int > mintimeBrute( const std::vector< Int > & t ,
                                   const std::vector< Int > & d , Int A )
{
 const std::size_t n = t.size();
 std::optional< Int > best;
 std::function< void( std::size_t , Int , Int ) > rec =
  [ & ]( std::size_t i , Int a , Int time ) {
   if( i + 1 == n ) {
    if( ! best || ( time < *best ) )
     best = time;
    return;
    }
   for( Int g = 0 ; a + g <= A ; ++g )
    if( a + g >= d[ i + 1 ] )
     rec( i + 1 , a + g - d[ i + 1 ] , time + g * t[ i ] );
   };
 rec( 0 , 0 , 0 );
 return( best );
 }

}  // end( namespace )

// 2014-06-16, Mintime: random roads of up to 5 stations and ranges up to
// 6 against all the vectors of recharges, the impossible ones included
ADS_TEST( "dp-27" )
{
 CHECK( dp::mintime( { 5 } , { 0 } , 10 ) == 0 );
 CHECK( dp::mintime( { 1 , 9 , 9 } , { 0 , 3 , 3 } , 6 ) == 6 );
 CHECK( dp::mintime( { 9 , 1 , 9 } , { 0 , 3 , 3 } , 6 ) == 30 );
 CHECK( ! dp::mintime( { 1 , 1 } , { 0 , 7 } , 6 ) );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::size_t n = rnd( g , 1 , 5 );
  const Int A = rnd( g , 0 , 6 );
  const auto t = rndVector( g , n , 0 , 9 );
  auto d = rndVector( g , n , 0 , 7 );
  d[ 0 ] = 0;
  CHECK( dp::mintime( t , d , A ) == mintimeBrute( t , d , A ) );
  }
 CHECK( dp::mintime( { 4 , 4 } , { 0 , 0 } , 0 ) == 0 );
 CHECK( dp::mintime( { 4 , 4 } , { 0 , 6 } , 6 ) == 24 );
 CHECK( ! dp::mintime( { 4 , 4 } , { 0 , 1 } , 0 ) );
 CHECK_THROWS( dp::mintime( {} , {} , 3 ) , contract_violation );
 CHECK_THROWS( dp::mintime( { 1 , 1 } , { 0 } , 3 ) , contract_violation );
 CHECK_THROWS( dp::mintime( { 1 } , { 0 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::mintime( { 1 , 1 } , { 0 , -1 } , 3 ) ,
               contract_violation );
 CHECK_THROWS( dp::mintime( { -1 , 1 } , { 0 , 1 } , 3 ) ,
               contract_violation );
 }

// 2014-07-21, Longest palindrome: the example, length 7, and random
// strings against the LCS of the string and its reverse from the library
ADS_TEST( "dp-28" )
{
 const auto p = longestPalindromicSubsequence( "BBABCBCAB" );
 CHECK( ( p.size() == 7 ) && palindrome( p ) &&
        isSubsequence( p , "BBABCBCAB" ) );
 CHECK( longestPalindromicSubsequence( "" ).empty() );
 CHECK( longestPalindromicSubsequence( "q" ) == "q" );
 CHECK( longestPalindromicSubsequence( "abc" ).size() == 1 );
 CHECK( longestPalindromicSubsequence( std::string( 500 , 'k' ) )
        .size() == 500 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::string s = rndString( g , rnd( g , 0 , 30 ) , 4 );
  const std::string rev( s.rbegin() , s.rend() );
  const auto q = longestPalindromicSubsequence( s );
  CHECK( int( q.size() ) == lcsTable( s , rev )[ s.size() ][ s.size() ] );
  CHECK( palindrome( q ) && isSubsequence( q , s ) );
  }
 }

// 2014-12-19, Max ordinata distinta: the LCS with the alphabet, from the
// library, on the two examples and against lisLength() on random strings
ADS_TEST( "dp-29" )
{
 const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 CHECK( lcs( std::string( "RSTUARSTUBRSTUCRSTUDRSTUE" ) , alphabet ) ==
        "ABCDRSTU" );
 CHECK( lcs( std::string( "CBA" ) , alphabet ).size() == 1 );
 CHECK( lcs( std::string() , alphabet ).empty() );
 CHECK( lcs( std::string( 100 , 'Q' ) , alphabet ) == "Q" );
 CHECK( lcs( alphabet , alphabet ) == alphabet );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::string S( rnd( g , 0 , 40 ) , 'A' );
  for( auto & c : S )
   c = static_cast< char >( 'A' + rnd( g , 0 , 25 ) );
  const std::vector< int > codes( S.begin() , S.end() );
  const auto o = lcs( S , alphabet );
  CHECK( o.size() == lisLength( codes ) );
  CHECK( std::adjacent_find( o.begin() , o.end() ,
                             std::greater_equal< char >() ) == o.end() );
  CHECK( isSubsequence( o , S ) );
  }
 }

namespace {

/// the fewest dashes aligning P[ i .. ] and S[ j .. ], plain recursion
std::size_t minAlignmentBrute( const std::string & P , const std::string & S ,
                               std::size_t i , std::size_t j )
{
 if( i == P.size() )
  return( S.size() - j );
 if( j == S.size() )
  return( P.size() - i );
 std::size_t best = 1 + std::min( minAlignmentBrute( P , S , i + 1 , j ) ,
                                  minAlignmentBrute( P , S , i , j + 1 ) );
 if( P[ i ] == S[ j ] )
  best = std::min( best , minAlignmentBrute( P , S , i + 1 , j + 1 ) );
 return( best );
 }

}  // end( namespace )

// 2014-12-19, Min allineamento: the two examples, and random strings
// against n + m - 2 LCS from the library and the plain recursion
ADS_TEST( "dp-30" )
{
 CHECK( dp::minAlignment( "ABC" , "ACE" ) == 2 );
 CHECK( dp::minAlignment( "questoeunoscempio" , "unesempio" ) == 10 );
 CHECK( dp::minAlignment( "" , "" ) == 0 );
 CHECK( dp::minAlignment( "abc" , "" ) == 3 );
 CHECK( dp::minAlignment( "aaaa" , "aaaa" ) == 0 );
 CHECK( dp::minAlignment( "" , "abc" ) == 3 );
 CHECK( dp::minAlignment( "abc" , "xyz" ) == 6 );
 CHECK( dp::minAlignment( "abc" , "cba" ) == 4 );
 CHECK( dp::minAlignment( std::string( 300 , 'a' ) ,
                          std::string( 100 , 'a' ) ) == 200 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::string P = rndString( g , rnd( g , 0 , 9 ) , 3 );
  const std::string S = rndString( g , rnd( g , 0 , 9 ) , 3 );
  const std::size_t a = dp::minAlignment( P , S );
  CHECK( a == P.size() + S.size() -
              2 * std::size_t( lcsTable( P , S )[ P.size() ][ S.size() ] ) );
  CHECK( a == minAlignmentBrute( P , S , 0 , 0 ) );
  }
 }

namespace {

/// the compositions of n in parts 1 .. 4, each one as a sequence
void stairsBrute( int n , std::vector< int > & prefix ,
                  std::set< std::vector< int > > & out )
{
 if( n == 0 ) {
  out.insert( prefix );
  return;
  }
 for( int k = 1 ; k <= std::min( n , 4 ) ; ++k ) {
  prefix.push_back( k );
  stairsBrute( n - k , prefix , out );
  prefix.pop_back();
  }
 }

}  // end( namespace )

// 2015-01-12, Count stairs: the five ways of the text for n = 7 are among
// the 56 compositions, the counts against the enumeration up to 18, and
// the ratio of two counts tends to the root 1.92756... of x^4 = x^3 + x^2 +
// x + 1
ADS_TEST( "dp-31" )
{
 std::set< std::vector< int > > seven;
 std::vector< int > prefix;
 stairsBrute( 7 , prefix , seven );
 CHECK( ( seven.size() == 56 ) && ( dp::countStairs( 7 ) == 56 ) );
 const std::vector< std::vector< int > > text = {
  { 1 , 1 , 1 , 1 , 1 , 1 , 1 } , { 1 , 2 , 4 } , { 4 , 2 , 1 } ,
  { 2 , 2 , 2 , 1 } , { 1 , 2 , 2 , 1 , 1 } };
 for( const auto & w : text )
  CHECK( seven.count( w ) == 1 );
 CHECK( dp::countStairs( 0 ) == 1 );
 for( int n = 0 ; n <= 18 ; ++n ) {
  std::set< std::vector< int > > all;
  stairsBrute( n , prefix , all );
  CHECK( dp::countStairs( n ) == all.size() );
  }
 double x = 2;                                  // Newton on the quartic
 for( int k = 0 ; k < 50 ; ++k )
  x -= ( x * x * x * x - x * x * x - x * x - x - 1 ) /
       ( 4 * x * x * x - 3 * x * x - 2 * x - 1 );
 const double ratio = double( dp::countStairs( 64 ) ) /
                      double( dp::countStairs( 63 ) );
 CHECK( std::abs( ratio - x ) < 1e-9 );
 CHECK( std::abs( x - 1.92756 ) < 1e-5 );
 CHECK( ( dp::countStairs( 1 ) == 1 ) && ( dp::countStairs( 4 ) == 8 ) );
 CHECK( dp::countStairs( 64 ) > dp::countStairs( 63 ) );
 CHECK_THROWS( dp::countStairs( 65 ) , contract_violation );
 CHECK_THROWS( dp::countStairs( -1 ) , contract_violation );
 }

// 2015-02-02, Piggy bank: the example, 200, and random coins against the
// table on the weight alone
ADS_TEST( "dp-32" )
{
 CHECK( dp::piggyBank( { 50 , 10 } , { 200 , 50 } , 50 ) == 200 );
 CHECK( dp::piggyBank( { 3 } , { 7 } , 0 ) == 0 );
 CHECK( ! dp::piggyBank( { 3 } , { 7 } , 5 ) );
 CHECK( ! dp::piggyBank( {} , {} , 1 ) );
 CHECK( dp::piggyBank( { 1 } , { 2 } , 3000 ) == 6000 );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const std::size_t n = rnd( g , 0 , 5 );
  const auto p = rndVector( g , n , 1 , 12 );
  const auto v = rndVector( g , n , 0 , 40 );
  const Int T = rnd( g , 0 , 60 );
  std::vector< Int > best( T + 1 , dp::INF );
  best[ 0 ] = 0;
  for( Int w = 1 ; w <= T ; ++w )
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ( p[ i ] <= w ) && ( best[ w - p[ i ] ] < dp::INF ) )
     best[ w ] = std::min( best[ w ] , best[ w - p[ i ] ] + v[ i ] );
  const auto got = dp::piggyBank( p , v , T );
  CHECK( got ? ( *got == best[ T ] ) : ( best[ T ] == dp::INF ) );
  }
 CHECK( dp::piggyBank( { 2 , 2 } , { 9 , 4 } , 4 ) == 8 );
 CHECK( dp::piggyBank( { 5 } , { 0 } , 10 ) == 0 );
 CHECK_THROWS( dp::piggyBank( { 0 } , { 1 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::piggyBank( { 1 } , { -1 } , 1 ) , contract_violation );
 CHECK_THROWS( dp::piggyBank( { 1 } , { 1 } , -1 ) , contract_violation );
 CHECK_THROWS( dp::piggyBank( { 1 , 2 } , { 1 } , 1 ) , contract_violation );
 }

// 2015-06-08, Resto limitato: optimalChange() of the library at most k on
// the two examples of the text, the second one corrected, and against the
// sums reached with at most k coins
ADS_TEST( "dp-33" )
{
 auto limited = []( const std::vector< long > & v , long R , long k ) {
  const auto c = optimalChange( v , R );
  return( c.has_value() && ( *c <= k ) );
  };
 CHECK( limited( { 1 , 5 , 10 , 17 , 4 } , 17 , 4 ) );
 // the text says 18 cannot be paid with four coins, but 17 + 1 pays it
 // with two; it cannot with one
 CHECK( limited( { 1 , 5 , 10 , 17 , 4 } , 18 , 4 ) );
 CHECK( ! limited( { 1 , 5 , 10 , 17 , 4 } , 18 , 1 ) );
 CHECK( limited( { 3 } , 0 , 0 ) );
 CHECK( ! limited( { 3 } , 3 , 0 ) );
 CHECK( limited( { 3 } , 3 , 1 ) && ! limited( { 3 } , 4 , 100 ) );
 CHECK( limited( { 1 } , 50 , 50 ) && ! limited( { 1 } , 50 , 49 ) );
 CHECK_THROWS( optimalChange( std::vector< long >{ 1 } , -1 ) ,
               contract_violation );
 CHECK_THROWS( optimalChange( std::vector< long >{ 0 } , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::vector< long > v( rnd( g , 1 , 5 ) );
  for( auto & c : v )
   c = static_cast< long >( rnd( g , 1 , 15 ) );
  const long R = static_cast< long >( rnd( g , 0 , 50 ) );
  const long k = static_cast< long >( rnd( g , 0 , 6 ) );
  std::set< long > reach = { 0 };               // sums of at most j coins
  for( long j = 0 ; j < k ; ++j ) {
   auto next = reach;
   for( long s : reach )
    for( long c : v )
     if( s + c <= R )
      next.insert( s + c );
   reach = next;
   }
  CHECK( limited( v , R , k ) == bool( reach.count( R ) ) );
  }
 }

// 2015-07-14, Soccer: the example, and random skills of up to 14 players
// against all the teams of half of them
ADS_TEST( "dp-34" )
{
 CHECK( dp::soccer( { 3 , 5 , 4 , 2 } ) );
 CHECK( dp::soccer( {} ) );
 CHECK( dp::soccer( { 6 , 6 } ) );
 CHECK( ! dp::soccer( { 1 , 3 } ) );
 CHECK( ! dp::soccer( { 1 , 1 , 1 , 3 } ) );    // equal sums, not halves
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const auto b = rndVector( g , 2 * rnd( g , 0 , 7 ) , 0 , 12 );
  const std::size_t n = b.size() / 2;
  const Int M = std::accumulate( b.begin() , b.end() , Int( 0 ) );
  bool ok = false;
  for( std::uint32_t mask = 0 ; mask < ( 1u << b.size() ) ; ++mask )
   if( std::popcount( mask ) == int( n ) ) {
    Int s = 0;
    for( std::size_t i = 0 ; i < b.size() ; ++i )
     if( ( mask >> i ) & 1 )
      s += b[ i ];
    ok = ok || ( 2 * s == M );
    }
  CHECK( dp::soccer( b ) == ok );
  }
 CHECK( dp::soccer( { 0 , 0 } ) && dp::soccer( { 0 , 0 , 0 , 0 } ) );
 CHECK( ! dp::soccer( { 0 , 0 , 0 , 2 } ) );
 CHECK_THROWS( dp::soccer( { 1 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( dp::soccer( { 1 } ) , contract_violation );
 CHECK_THROWS( dp::soccer( { 1 , -1 } ) , contract_violation );
 }

// 2015-09-01, Repeat sort: the example of 20 items and 4 values, and random
// arrays of few values from a large range against std::sort
ADS_TEST( "dp-36" )
{
 std::vector< Int > A = { 100 , 12 , 100 , 1 , 1 , 12 , 100 , 1 , 12 , 100 ,
                          1 , 1 , 1 , 100999 , 12 , 1 , 100999 , 1 , 1 ,
                          100 };
 auto B = A;
 std::sort( B.begin() , B.end() );
 dp::repeatSort( A );
 CHECK( A == B );
 std::vector< Int > E;
 dp::repeatSort( E );
 CHECK( E.empty() );
 for( std::vector< Int > F : { std::vector< Int >{ 9 } ,
                               std::vector< Int >( 50 , -3 ) ,
                               std::vector< Int >{ 1 , 2 , 3 } ,
                               std::vector< Int >{ 3 , 2 , 1 , 3 , 2 , 1 } ,
                               std::vector< Int >{
                                std::numeric_limits< Int >::max() ,
                                std::numeric_limits< Int >::min() } } ) {
  auto G = F;
  std::sort( G.begin() , G.end() );
  dp::repeatSort( F );
  CHECK( F == G );
  }
 std::mt19937 g( 1 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const auto values = rndVector( g , rnd( g , 1 , 6 ) ,
                                 std::numeric_limits< Int >::min() ,
                                 std::numeric_limits< Int >::max() );
  std::vector< Int > C( rnd( g , 0 , 200 ) );
  for( auto & c : C )
   c = values[ rnd( g , 0 , Int( values.size() ) - 1 ) ];
  auto D = C;
  std::sort( D.begin() , D.end() );
  dp::repeatSort( C );
  CHECK( C == D );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File exam_dp1.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
