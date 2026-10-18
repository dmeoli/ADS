/*--------------------------------------------------------------------------*/
/*-------------------------- File exam_divide.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the divide-and-conquer problems of the collection "Esercizi
 * Divide-et-impera, Raccolte da compiti scritti" of the book: each problem
 * is solved by its official solution, in the namespace divide, and checked
 * on the examples of the text, against an independent oracle (a brute
 * force, the standard library or a function of the library) on random small
 * instances, and on the edge cases, at the level 3 of the contracts.
 *
 * The pseudocode of the text counts the places from 1, the functions here
 * from 0: the middle place floor( ( i + j ) / 2 ) is the same item either
 * way, and only the tests on the parity of a place change.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/Contract.h"
#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/algorithm/Subsequences.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/LocalOptima.h"
#include "ADS/search/MonotoneSearch.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
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

/*--------------------------------------------------------------------------*/
/*------------------------------- SOLUTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace divide {

using Ints = std::vector< int >;
using std::int64_t;
using std::size_t;

/*------------------------- 2011-05-27, Doublegap --------------------------*/

/// a double gap of V, a place i with V[ i + 1 ] - V[ i ] >= 2, when
/// V[ n - 1 ] - V[ 0 ] >= n; a binary search on the invariant V[ j ] - V[ i ]
/// >= j - i + 1, which keeps a double gap in V[ i .. j ] and holds in one of
/// the two halves, else their sum gives V[ j ] - V[ i ] <= j - i; O( log n )
size_t doublegapRec( const Ints & V , size_t i , size_t j )
{
 if( j == i + 1 )
  return( i );
 const size_t m = ( i + j ) / 2;
 if( int64_t( V[ m ] ) - V[ i ] >= int64_t( m - i + 1 ) )
  return( doublegapRec( V , i , m ) );
 return( doublegapRec( V , m , j ) );
 }

/// \pre n >= 2 , V[ n - 1 ] - V[ 0 ] >= n
/// \post V[ r + 1 ] - V[ r ] >= 2
size_t doublegap( const Ints & V )
{
 const size_t n = V.size();
 ADS_PRE( ( n >= 2 ) && ( int64_t( V[ n - 1 ] ) - V[ 0 ] >= int64_t( n ) ) );
 const size_t r = doublegapRec( V , 0 , n - 1 );
 ADS_POST( int64_t( V[ r + 1 ] ) - V[ r ] >= 2 );
 return( r );
 }

/*-------------------------- 2013-07-22, Maxprod ---------------------------*/

/// the largest product of a run of consecutive items of X, the empty run
/// giving 1; the scan of the largest sum of a run, with the product ending
/// at the current item or 1 in place of the sum; O( n ). A 0 restarts the
/// run by itself, since a product grown to +inf times 0 would give NaN.
/// \pre the items are >= 0 (not NaN, +inf allowed)
/// \post r >= 1
double maxprod( const std::vector< double > & X )
{
 ADS_PRE( std::all_of( X.begin() , X.end() ,
                       []( double x ) { return( x >= 0 ); } ) );
 double best = 1 , here = 1;
 for( double x : X ) {
  here = ( x == 0 ) ? 1.0 : std::max( here * x , 1.0 );
  best = std::max( here , best );
  }
 ADS_POST( best >= 1 );
 return( best );
 }

/*------------------------ 2014-04-24, Pythagorean -------------------------*/

/// whether three items of A in distinct places have x^2 + y^2 = z^2; for
/// each hypotenuse z, the search of a pair of the sum z^2 by two indices on
/// the sorted squares; Theta( n^2 ). The text sorts the items, which fails
/// on negative ones: here the squares are sorted.
bool pythagorean( const Ints & A )
{
 std::vector< std::uint64_t > S;
 for( int a : A )
  S.push_back( std::uint64_t( int64_t( a ) * a ) );
 std::sort( S.begin() , S.end() );
 for( size_t k = 2 ; k < S.size() ; ++k ) {
  size_t i = 0 , j = k - 1;
  while( i < j ) {
   const std::uint64_t s = S[ i ] + S[ j ];
   if( s == S[ k ] )
    return( true );
   if( s < S[ k ] )
    ++i;
   else
    --j;
   }
  }
 return( false );
 }

/*------------------------- 2014-10-31, Thickness --------------------------*/

/// the longest run of V of thickness ( max - min ) at most C, fixing the
/// first place and extending the last one with the running min and max;
/// Theta( n^2 )
/// \pre C > 0
size_t thicknessQuadratic( const Ints & V , int C )
{
 ADS_PRE( C > 0 );
 size_t best = 0;
 for( size_t i = 0 ; i < V.size() ; ++i ) {
  int lo = V[ i ] , hi = V[ i ];
  for( size_t j = i ; j < V.size() ; ++j ) {
   lo = std::min( lo , V[ j ] );
   hi = std::max( hi , V[ j ] );
   if( int64_t( hi ) - lo > C )
    break;
   best = std::max( best , j - i + 1 );
   }
  }
 return( best );
 }

/// the divide and conquer of thickness(): the best run of each half, and
/// the best run across the middle by two indices on the min and the max of
/// V[ k .. m ] and of V[ m + 1 .. k ]. The text moves start on a success
/// and last on a failure, which misses runs (asPrinted keeps that order):
/// here a success moves last, a failure start.
size_t thicknessRec( const Ints & V , size_t i , size_t j , int C ,
                     Ints & mins , Ints & maxs , bool asPrinted )
{
 if( i == j )
  return( 1 );
 const size_t m = ( i + j ) / 2;
 size_t maxlen = std::max( thicknessRec( V , i , m , C , mins , maxs ,
                                         asPrinted ) ,
                           thicknessRec( V , m + 1 , j , C , mins , maxs ,
                                         asPrinted ) );
 mins[ m ] = maxs[ m ] = V[ m ];
 for( size_t k = m ; k-- > i ; ) {
  mins[ k ] = std::min( mins[ k + 1 ] , V[ k ] );
  maxs[ k ] = std::max( maxs[ k + 1 ] , V[ k ] );
  }
 mins[ m + 1 ] = maxs[ m + 1 ] = V[ m + 1 ];
 for( size_t k = m + 2 ; k <= j ; ++k ) {
  mins[ k ] = std::min( mins[ k - 1 ] , V[ k ] );
  maxs[ k ] = std::max( maxs[ k - 1 ] , V[ k ] );
  }
 size_t start = i , last = m + 1;
 while( ( start <= m ) && ( last <= j ) ) {
  const bool ok = int64_t( std::max( maxs[ start ] , maxs[ last ] ) ) -
                  std::min( mins[ start ] , mins[ last ] ) <= C;
  if( ok )
   maxlen = std::max( maxlen , last - start + 1 );
  if( ok == asPrinted )
   ++start;
  else
   ++last;
  }
 return( maxlen );
 }

/// thickness() by divide and conquer, T( n ) = 2 T( n / 2 ) + n =
/// Theta( n log n ); 0 if V is empty
/// \pre C > 0
size_t thickness( const Ints & V , int C , bool asPrinted = false )
{
 ADS_PRE( C > 0 );
 if( V.empty() )
  return( 0 );
 Ints mins( V.size() ) , maxs( V.size() );
 return( thicknessRec( V , 0 , V.size() - 1 , C , mins , maxs ,
                       asPrinted ) );
 }

/*------------------------- 2015-09-01, Find peak --------------------------*/

/// a peak of A, an item larger than its neighbours: findPeak() of the
/// library, a binary search on the side of the larger neighbour, which
/// holds a peak; O( log n )
/// \pre n >= 1 , the items are distinct
/// \post A[ r ] is larger than its neighbours
size_t findPeak( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( n >= 1 );
 ADS_AUDIT( std::set< int >( A.begin() , A.end() ).size() == n );
 const size_t r = ADS::findPeak( A );
 ADS_POST( ( ( r == 0 ) || ( A[ r - 1 ] < A[ r ] ) ) &&
           ( ( r == n - 1 ) || ( A[ r ] > A[ r + 1 ] ) ) );
 return( r );
 }

/*------------------------ 2015-11-05, Compute sum -------------------------*/

/// the last place of a in A[ i .. j ], A[ i ] = a and A[ j ] = b: a binary
/// search keeping that invariant; T( n ) = T( n / 2 ) + 1 = Theta( log n )
size_t binarySearchAB( const Ints & A , size_t i , size_t j )
{
 if( j == i + 1 )
  return( i );
 const size_t m = ( i + j ) / 2;
 if( A[ m ] == A[ i ] )
  return( binarySearchAB( A , m , j ) );
 return( binarySearchAB( A , i , m ) );
 }

/// the number of a's, where A sorted holds a's and then b's, a < b
/// \pre n >= 2 , A[ 0 ] < A[ n - 1 ] , A is sorted with two values
size_t countA( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( ( n >= 2 ) && ( A[ 0 ] < A[ n - 1 ] ) );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) &&
            std::all_of( A.begin() , A.end() , [ & ]( int x ) {
             return( ( x == A[ 0 ] ) || ( x == A[ n - 1 ] ) ); } ) );
 return( binarySearchAB( A , 0 , n - 1 ) + 1 );
 }

/// the occurrences of the more frequent value, max( k , n - k ); O( log n )
size_t mostFrequent( const Ints & A )
{
 const size_t k = countA( A );
 return( std::max( k , A.size() - k ) );
 }

/// the sum of the items, k a + ( n - k ) b; O( log n )
int64_t computeSum( const Ints & A )
{
 const size_t k = countA( A );
 return( int64_t( k ) * A.front() + int64_t( A.size() - k ) * A.back() );
 }

/*------------------------ 2016-11-03, Punto fisso -------------------------*/

/// a zero of V[ i .. j ], V[ i ] and V[ j ] of opposite signs, which with
/// steps of at most 1 holds a zero; the half whose ends keep opposite
/// signs; O( log n ). The text states V[ 0 ] > 0 , V[ n - 1 ] < 0 and the
/// solution V[ 0 ] < 0 , V[ n - 1 ] > 0, using A for V: comparing with the
/// sign of V[ i ] serves both.
size_t zeroRec( const Ints & V , size_t i , size_t j )
{
 const size_t m = ( i + j ) / 2;
 if( V[ m ] == 0 )
  return( m );
 if( ( V[ m ] < 0 ) == ( V[ i ] < 0 ) )
  return( zeroRec( V , m , j ) );
 return( zeroRec( V , i , m ) );
 }

/// a place of a zero of V, which always exists (the -1 of the text is
/// never returned)
/// \pre n >= 3 , | V[ i - 1 ] - V[ i ] | <= 1 , V[ 0 ] and V[ n - 1 ] of
///      opposite signs
/// \post V[ r ] == 0
size_t zero( const Ints & V )
{
 const size_t n = V.size();
 ADS_PRE( ( n >= 3 ) && ( int64_t( V[ 0 ] ) * V[ n - 1 ] < 0 ) );
 ADS_AUDIT( [ & ]() {
  for( size_t i = 1 ; i < n ; ++i )
   if( std::abs( int64_t( V[ i - 1 ] ) - V[ i ] ) > 1 )
    return( false );
  return( true ); }() );
 const size_t r = zeroRec( V , 0 , n - 1 );
 ADS_POST( V[ r ] == 0 );
 return( r );
 }

/*------------------------- 2018-01-08, Max crime --------------------------*/

/// the most items of the sorted A in t consecutive minutes, the largest
/// j - i + 1 with A[ j ] - A[ i ] + 1 <= t: two indices, j moving on a
/// success and i on a failure; O( n )
/// \pre t >= 1 , A is sorted
size_t maxCrime( const Ints & A , int t )
{
 ADS_PRE( t >= 1 );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 size_t i = 0 , j = 0 , maxsofar = 0;
 while( j < A.size() )
  if( int64_t( A[ j ] ) - A[ i ] + 1 <= t ) {
   maxsofar = std::max( maxsofar , j - i + 1 );
   ++j;
   }
  else
   ++i;
 return( maxsofar );
 }

/*------------------------ 2018-01-31, Valore unico ------------------------*/

/// the item once in A[ i .. j ], where the other items are in pairs: the
/// pairs before it start at even places, those after it at odd ones;
/// O( log n ). The places from 0 swap the parities of the text.
int singleRec( const Ints & A , size_t i , size_t j )
{
 if( i == j )
  return( A[ i ] );
 const size_t m = ( i + j ) / 2;
 if( m % 2 == 0 ) {
  if( A[ m ] == A[ m + 1 ] )
   return( singleRec( A , m + 2 , j ) );
  return( singleRec( A , i , m ) );
  }
 if( A[ m - 1 ] == A[ m ] )
  return( singleRec( A , m + 1 , j ) );
 return( singleRec( A , i , m - 1 ) );
 }

/// the value once in the sorted A, all others twice
/// \pre n odd
int single( const Ints & A )
{
 ADS_PRE( A.size() % 2 == 1 );
 return( singleRec( A , 0 , A.size() - 1 ) );
 }

/*---------------------- 2018-08-21, Vettori traslati ----------------------*/

/// the shift k < n of a sorted vector of distinct items rotated right by
/// k places, the place of its minimum: rotatedMinimum() of the library, in
/// the right part of A[ i .. j ] if A[ m ] > A[ j ], else in the left one
/// with m; O( log n )
/// \pre n >= 1 , distinct items
size_t shift( const Ints & A )
{
 ADS_PRE( ! A.empty() );
 return( rotatedMinimum( A ) );
 }

/*------------------------- 2019-01-21, Successore -------------------------*/

/// the least item of the sorted A larger than v, -1 if none: the place of
/// upperBound() of the library; O( log n )
/// \pre A is sorted
int64_t successor( const Ints & A , int v )
{
 const size_t r = upperBound( A , v );
 return( r == A.size() ? -1 : int64_t( A[ r ] ) );
 }

/*-------------------------- 2020-01-14, Intruder --------------------------*/

/// the item of B[ i .. j ] missing in A: right of m if A[ m ] = B[ m ], else
/// left of m with m; O( log n )
int intruderRec( const Ints & A , const Ints & B , size_t i , size_t j )
{
 if( i == j )
  return( B[ i ] );
 const size_t m = ( i + j ) / 2;
 if( A[ m ] == B[ m ] )
  return( intruderRec( A , B , m + 1 , j ) );
 return( intruderRec( A , B , i , m ) );
 }

/// the item of B, sorted of n + 1 distinct items, not in A, sorted of the
/// other n
/// \pre | B | = | A | + 1
int intruder( const Ints & A , const Ints & B )
{
 ADS_PRE( B.size() == A.size() + 1 );
 return( intruderRec( A , B , 0 , A.size() ) );
 }

/*----------------------- 2020-06-12, Mistery array ------------------------*/

/// a sorted sequence with no size: a read past its end gives BOTTOM, here
/// std::nullopt; it counts its reads
class MisteryArray {
 public:

 explicit MisteryArray( const Ints & v ) : f_v( v ) {}

 std::optional< int > operator[]( size_t i ) const {
  ++f_reads;
  if( i < f_v.size() )
   return( f_v[ i ] );
  return( std::nullopt );
  }

 [[nodiscard]] long reads( void ) const { return( f_reads ); }

 private:

 const Ints & f_v;              ///< the items
 mutable long f_reads = 0;      ///< the reads so far

 };  // end( class MisteryArray )

/// the number of negative items of A, the first place of an item >= 0 or of
/// BOTTOM, which counts as +infinity: unboundedSearch() of the library,
/// the places 0 , 1 , 3 , 7 , ... until one of them, and a binary search;
/// O( log n )
/// \pre A is not empty
size_t countNegatives( const MisteryArray & A )
{
 ADS_PRE( A[ 0 ].has_value() );
 return( unboundedSearch( [ & ]( size_t i ) { return( A[ i ] ); } ,
                          []( int a ) { return( a >= 0 ); } ) );
 }

/*------------------------ 2020-07-03, Maggioranza -------------------------*/

/// the first place of v in the sorted A[ i .. j ], which holds it
size_t searchFirst( const Ints & A , size_t i , size_t j , int v )
{
 if( i == j )
  return( i );
 const size_t m = ( i + j ) / 2;
 if( v <= A[ m ] )
  return( searchFirst( A , i , m , v ) );
 return( searchFirst( A , m + 1 , j , v ) );
 }

/// whether a value is more than n / 2 items of the sorted A: only the
/// median can be, if its first place plus n / 2 holds it too; O( log n ).
/// The text reads A[ first + n / 2 ] with no check of the bound.
/// \pre n >= 1 , A is sorted
bool hasMajority( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( n >= 1 );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 const int candidate = A[ ( n + 1 ) / 2 - 1 ];
 const size_t first = searchFirst( A , 0 , n - 1 , candidate );
 return( ( first + n / 2 < n ) && ( A[ first + n / 2 ] == candidate ) );
 }

/*-------------------------- 2021-01-18, Singolo ---------------------------*/

/// the item once in A[ i .. j ], of odd length from an even place, where
/// the others are in consecutive pairs: m moved to an even place, the
/// item is right of the pair at m if it is one, else left; O( log n ).
/// The places from 0 swap the parities of the text.
int fsRec( const Ints & A , size_t i , size_t j )
{
 if( i == j )
  return( A[ i ] );
 size_t m = ( i + j ) / 2;
 if( m % 2 == 1 )
  --m;
 if( A[ m ] == A[ m + 1 ] )
  return( fsRec( A , m + 2 , j ) );
 return( fsRec( A , i , m ) );
 }

/// the value once in A, the others in consecutive pairs
/// \pre n odd
int findSingle( const Ints & A )
{
 ADS_PRE( A.size() % 2 == 1 );
 return( fsRec( A , 0 , A.size() - 1 ) );
 }

/*-------------------------- 2021-02-08, Closest ---------------------------*/

/// an item of the sorted A[ i .. j ] closest to x by the test of the text:
/// left with m only if | A[ m ] - x | < | A[ m + 1 ] - x |, which goes
/// right on A[ m ] = A[ m + 1 ] > x and misses a closer item on the left,
/// as 0 in [ 0 , 3 , 3 ] for x = 1; O( log n )
int closestAsPrinted( const Ints & A , size_t i , size_t j , int x )
{
 if( i == j )
  return( A[ i ] );
 const size_t m = ( i + j ) / 2;
 if( std::abs( int64_t( A[ m ] ) - x ) <
     std::abs( int64_t( A[ m + 1 ] ) - x ) )
  return( closestAsPrinted( A , i , m , x ) );
 return( closestAsPrinted( A , m + 1 , j , x ) );
 }

/// an item of the sorted A closest to x: closestValue() of the library,
/// the first item not smaller than x and the one before it; O( log n ).
/// asPrinted takes the test of the text instead.
/// \pre n >= 1 , A is sorted
int closest( const Ints & A , int x , bool asPrinted = false )
{
 ADS_PRE( ! A.empty() );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 if( asPrinted )
  return( closestAsPrinted( A , 0 , A.size() - 1 , x ) );
 return( A[ closestValue( A , x ) ] );
 }

/*-------------------------- 2021-02-19, Fast sum --------------------------*/

/// the last place of x in the sorted A[ i .. j ], whose first item is
/// <= x, with the middle rounded up; O( log n )
size_t lastPos( const Ints & A , size_t i , size_t j , int x )
{
 if( i == j )
  return( i );
 const size_t m = ( i + j + 1 ) / 2;
 if( A[ m ] > x )
  return( lastPos( A , i , m - 1 , x ) );
 return( lastPos( A , m , j , x ) );
 }

/// the sum of the sorted A of 1, 2 and 3, each present, 3 n - p1 - p2 with
/// p1 and p2 the numbers of items up to the last 1 and the last 2
/// \pre A is sorted, of 1 , 2 , 3 , each present
int64_t fastSum( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( ( n >= 3 ) && ( A.front() == 1 ) && ( A.back() == 3 ) );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) &&
            std::count( A.begin() , A.end() , 2 ) > 0 );
 const size_t pos1 = lastPos( A , 0 , n - 1 , 1 ) + 1;
 const size_t pos2 = lastPos( A , 0 , n - 1 , 2 ) + 1;
 return( 3 * int64_t( n ) - int64_t( pos1 ) - int64_t( pos2 ) );
 }

/*------------------------ 2021-08-31, ContainsZero ------------------------*/

/// whether a nonempty run of A has sum 0: two equal prefix sums among the
/// sorted S[ 0 .. n ]; Theta( n log n )
bool containsZeroSort( const Ints & A )
{
 std::vector< int64_t > S( 1 , 0 );
 for( int a : A )
  S.push_back( S.back() + a );
 std::sort( S.begin() , S.end() );
 return( std::adjacent_find( S.begin() , S.end() ) != S.end() );
 }

/// containsZero() on the sorted A: with tot the sum of A[ i .. j ], if
/// tot < 0 every run from i is negative and A[ i ] goes, if tot > 0 every
/// run to j is positive and A[ j ] goes; Theta( n ). The text tests A[ i ]
/// and A[ j ] before i <= j, reading past the ends: here i <= j comes first.
/// \pre A is sorted
bool containsZero( const Ints & A )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 std::ptrdiff_t i = 0 , j = std::ptrdiff_t( A.size() ) - 1;
 int64_t tot = std::accumulate( A.begin() , A.end() , int64_t( 0 ) );
 while( ( i <= j ) && ( tot != 0 ) && ( A[ i ] <= 0 ) && ( A[ j ] >= 0 ) )
  if( tot < 0 )
   tot -= A[ i++ ];
  else
   tot -= A[ j-- ];
 return( ( tot == 0 ) && ( i <= j ) );
 }

/*---------------------- 2021-08-31, Radice quadrata -----------------------*/

/// whether n = k^2 for an integer k: isPerfectSquare() of the library, a
/// binary search of the root testing m^2 against n by m against n / m,
/// with no overflow; O( log n ) with the uniform cost
/// \pre n >= 1
bool isSquare( std::uint64_t n )
{
 ADS_PRE( n >= 1 );
 return( isPerfectSquare( n ) );
 }

/*------------------ 2022-01-17, Serie aritmetica bucata -------------------*/

/// the missing term in A[ i .. j ], which holds the gap: in the right part
/// if A[ m ] = A[ i ] + d ( m - i ), else in the left one; O( log n )
int64_t missingRec( const Ints & A , size_t i , size_t j , int64_t d )
{
 if( j == i + 1 )
  return( A[ i ] + d );
 const size_t m = ( i + j ) / 2;
 if( A[ m ] == A[ i ] + d * int64_t( m - i ) )
  return( missingRec( A , m , j , d ) );
 return( missingRec( A , i , m , d ) );
 }

/// the term missing in A of x , x + d , ... , x + n d with an inner term
/// left out; d = ( A[ n - 1 ] - A[ 0 ] ) / n
/// \pre n >= 2
int64_t missing( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( n >= 2 );
 const int64_t d = ( int64_t( A[ n - 1 ] ) - A[ 0 ] ) / int64_t( n );
 return( missingRec( A , 0 , n - 1 , d ) );
 }

/*-------------------------- 2022-07-13, H-index ---------------------------*/

/// the h-index of A sorted in non-increasing order, the largest h with h
/// items >= h: a binary search of the last m with A[ m ] >= m + 1;
/// O( log n )
/// \pre A is non-increasing
size_t hIndex( const Ints & A )
{
 ADS_AUDIT( std::is_sorted( A.rbegin() , A.rend() ) );
 size_t h = 0;
 std::ptrdiff_t i = 0 , j = std::ptrdiff_t( A.size() ) - 1;
 while( i <= j ) {
  const std::ptrdiff_t m = ( i + j ) / 2;
  if( A[ m ] >= m + 1 ) {
   h = size_t( m + 1 );
   i = m + 1;
   }
  else
   j = m - 1;
  }
 return( h );
 }

/*----------------------- 2023-02-09, Minimo locale ------------------------*/

/// a local minimum of A, an inner item smaller than its neighbours:
/// localMinimum() of the library, which with A[ 0 ] > A[ 1 ] and
/// A[ n - 2 ] < A[ n - 1 ] cannot stop at an end; O( log n )
/// \pre n >= 3 , distinct items , A[ 0 ] > A[ 1 ] , A[ n - 2 ] < A[ n - 1 ]
/// \post 0 < r < n - 1 , A[ r - 1 ] > A[ r ] < A[ r + 1 ]
size_t findLocalMinimum( const Ints & A )
{
 const size_t n = A.size();
 ADS_PRE( ( n >= 3 ) && ( A[ 0 ] > A[ 1 ] ) && ( A[ n - 2 ] < A[ n - 1 ] ) );
 ADS_AUDIT( std::set< int >( A.begin() , A.end() ).size() == n );
 const size_t r = localMinimum( A );
 ADS_POST( ( 0 < r ) && ( r < n - 1 ) && ( A[ r - 1 ] > A[ r ] ) &&
           ( A[ r ] < A[ r + 1 ] ) );
 return( r );
 }

/*----------------------- 2024-01-09, k-Ripetizioni ------------------------*/

/// the longest run of S[ start .. end ] whose characters occur at least k
/// times in it: a character of S[ start .. end ] with fewer occurrences is
/// in no such run, which splits there; T( n ) = T( n - 1 ) + n = O( n^2 )
size_t krRec( const std::string & S , std::ptrdiff_t start ,
              std::ptrdiff_t end , int k )
{
 if( end < start )
  return( 0 );
 std::unordered_map< char , int > freq;
 for( std::ptrdiff_t i = start ; i <= end ; ++i )
  ++freq[ S[ i ] ];
 for( std::ptrdiff_t i = start ; i <= end ; ++i )
  if( freq[ S[ i ] ] < k )
   return( std::max( krRec( S , start , i - 1 , k ) ,
                     krRec( S , i + 1 , end , k ) ) );
 return( size_t( end - start + 1 ) );
 }

/// \pre k > 0
size_t krepeating( const std::string & S , int k )
{
 ADS_PRE( k > 0 );
 return( krRec( S , 0 , std::ptrdiff_t( S.size() ) - 1 , k ) );
 }

/*------------------------- 2024-02-01, Un singolo -------------------------*/

/// the item once in A[ start .. end ], of odd length, where the others are
/// in consecutive pairs, two pairs next to each other of different values:
/// by the parity of the length of A[ start .. mid ] and whether A[ mid ]
/// pairs with A[ mid + 1 ], the odd part that holds it; Theta( log n )
int osRec( const Ints & A , size_t start , size_t end )
{
 if( start == end )
  return( A[ start ] );
 const size_t mid = ( start + end ) / 2;
 if( ( mid - start + 1 ) % 2 == 1 ) {
  if( A[ mid ] != A[ mid + 1 ] )
   return( osRec( A , start , mid ) );
  return( osRec( A , mid + 2 , end ) );
  }
 if( A[ mid ] == A[ mid + 1 ] )
  return( osRec( A , start , mid - 1 ) );
 return( osRec( A , mid + 1 , end ) );
 }

/// \pre n odd
int oneSingle( const Ints & A )
{
 ADS_PRE( A.size() % 2 == 1 );
 return( osRec( A , 0 , A.size() - 1 ) );
 }

/*----------------------- 2024-06-14, Smaller Count ------------------------*/

/// count[ i ], the number of j > i with A[ j ] < A[ i ]: smallerToTheRight()
/// of the library, Merge Sort of the places counting, for each item of a
/// left half, the items of the right half that pass it; O( n log n ) time,
/// O( n ) space
/// \pre the items are distinct
std::vector< size_t > smallerCount( const Ints & A )
{
 ADS_AUDIT( std::set< int >( A.begin() , A.end() ).size() == A.size() );
 return( smallerToTheRight( A ) );
 }

/*--------------------- 2024-08-26, Where's negative? ----------------------*/

/// the least n >= 1 with f( n ) < 0 of a decreasing f with f( 1 ) > 0:
/// exponentialSearch() of the library, the places 1 , 2 , 4 , ... until
/// f < 0, and a binary search between the last two; O( log n ) calls
/// \pre f( 1 ) > 0 , f decreasing with a negative value
int64_t findNegative( const std::function< int64_t( int64_t ) > & f )
{
 ADS_PRE( f( 1 ) > 0 );
 return( exponentialSearch( [ & ]( int64_t k ) { return( f( k ) < 0 ); } ,
                            int64_t( 1 ) ) );
 }

/*------------------------ 2025-01-14, AlternaSort -------------------------*/

/// whether A[ 0 ] < A[ 1 ] > A[ 2 ] < A[ 3 ] > ...
bool alternating( const Ints & A )
{
 for( size_t i = 0 ; i + 1 < A.size() ; ++i )
  if( ( i % 2 == 0 ) ? !( A[ i ] < A[ i + 1 ] ) : !( A[ i ] > A[ i + 1 ] ) )
   return( false );
 return( true );
 }

/// A alternating: the sorted copy B, its smallest items at the even places
/// and its largest at the odd ones; O( n log n )
/// \pre the items are distinct
/// \post A alternates
void alternaSortSorted( Ints & A )
{
 Ints B( A );
 std::sort( B.begin() , B.end() );
 size_t i = 0 , j = A.size();
 for( size_t k = 0 ; k < A.size() ; ++k )
  A[ k ] = ( k % 2 == 0 ) ? B[ i++ ] : B[ --j ];
 ADS_POST( alternating( A ) );
 }

/// A alternating by local swaps: wiggleSort() of the library, after
/// A[ 0 .. i ] alternates a swap of A[ i ] and A[ i + 1 ] fixes their order
/// and keeps the one before by transitivity; O( n )
/// \pre the items are distinct
/// \post A alternates
void alternaSort( Ints & A )
{
 ADS_AUDIT( std::set< int >( A.begin() , A.end() ).size() == A.size() );
 wiggleSort( A );
 ADS_POST( alternating( A ) );
 }

/// alternaSort() by divide and conquer: both halves alternate by the
/// parity of their places, and a swap of A[ m ] and A[ m + 1 ] fixes the
/// border; T( n ) = 2 T( n / 2 ) + O( 1 ) = O( n )
void alternaSortRec( Ints & A , size_t i , size_t j )
{
 if( i >= j )
  return;
 const size_t m = ( i + j ) / 2;
 alternaSortRec( A , i , m );
 alternaSortRec( A , m + 1 , j );
 if( ( m % 2 == 0 ) ? ( A[ m ] > A[ m + 1 ] ) : ( A[ m ] < A[ m + 1 ] ) )
  std::swap( A[ m ] , A[ m + 1 ] );
 }

/// \pre the items are distinct
/// \post A alternates
void alternaSortDivide( Ints & A )
{
 if( ! A.empty() )
  alternaSortRec( A , 0 , A.size() - 1 );
 ADS_POST( alternating( A ) );
 }

/*-------------------- 2025-06-13, Stringhe pareggiate ---------------------*/

/// c with the other case
char swapcase( char c )
{
 if( ( c >= 'a' ) && ( c <= 'z' ) )
  return( char( c - 'a' + 'A' ) );
 if( ( c >= 'A' ) && ( c <= 'Z' ) )
  return( char( c - 'A' + 'a' ) );
 return( c );
 }

/// whether S[ start .. end ] holds swapcase( c ) for each of its c
bool isPaired( const std::string & S , size_t start , size_t end )
{
 const std::set< char > letters( S.begin() + std::ptrdiff_t( start ) ,
                                 S.begin() + std::ptrdiff_t( end ) + 1 );
 for( char c : letters )
  if( ! letters.count( swapcase( c ) ) )
   return( false );
 return( true );
 }

/// the longest paired run of S[ i .. j ]: a letter of it with no partner
/// in it is in no paired run, which splits there; Omega( n ) and
/// T( n ) = T( n - 1 ) + n = Theta( n^2 )
size_t lpRec( const std::string & S , std::ptrdiff_t i , std::ptrdiff_t j )
{
 if( i > j )
  return( 0 );
 std::map< char , std::ptrdiff_t > letters;
 for( std::ptrdiff_t k = i ; k <= j ; ++k )
  letters[ S[ k ] ] = k;
 for( const auto & [ c , m ] : letters )
  if( ! letters.count( swapcase( c ) ) )
   return( std::max( lpRec( S , i , m - 1 ) , lpRec( S , m + 1 , j ) ) );
 return( size_t( j - i + 1 ) );
 }

/// \pre S holds only letters
size_t largestPaired( const std::string & S )
{
 ADS_PRE( std::all_of( S.begin() , S.end() , []( char c ) {
  return( ( ( c >= 'a' ) && ( c <= 'z' ) ) ||
          ( ( c >= 'A' ) && ( c <= 'Z' ) ) ); } ) );
 return( lpRec( S , 0 , std::ptrdiff_t( S.size() ) - 1 ) );
 }

/*----------------------- 2026-02-02, Due caratteri ------------------------*/

/// the run from first leftwards with the characters chars, of size 1 or 2,
/// the first new character joining a set of size 1; its first place
std::ptrdiff_t goingLeft( const std::string & S , std::ptrdiff_t start ,
                          std::ptrdiff_t first , std::set< char > & chars )
{
 std::ptrdiff_t i = first;
 while( ( i >= start ) && ( ( chars.size() == 1 ) || chars.count( S[ i ] ) ) )
  chars.insert( S[ i-- ] );
 return( i + 1 );
 }

/// the same rightwards; its last place
std::ptrdiff_t goingRight( const std::string & S , std::ptrdiff_t first ,
                           std::ptrdiff_t end , std::set< char > & chars )
{
 std::ptrdiff_t j = first;
 while( ( j <= end ) && ( ( chars.size() == 1 ) || chars.count( S[ j ] ) ) )
  chars.insert( S[ j++ ] );
 return( j - 1 );
 }

/// the longest run of S[ start .. end ] with at most two characters: the
/// best of each half and of the runs across the middle, extended first
/// leftwards and then rightwards, and the other way round;
/// T( n ) = 2 T( n / 2 ) + O( n ) = O( n log n )
size_t maxTwoRec( const std::string & S , std::ptrdiff_t start ,
                  std::ptrdiff_t end )
{
 if( end - start + 1 <= 2 )
  return( size_t( end - start + 1 ) );
 const std::ptrdiff_t mid = ( start + end ) / 2;
 const size_t maxLeft = maxTwoRec( S , start , mid );
 const size_t maxRight = maxTwoRec( S , mid + 1 , end );
 std::set< char > chars{ S[ mid ] , S[ mid + 1 ] };
 std::ptrdiff_t i = goingLeft( S , start , mid - 1 , chars );
 std::ptrdiff_t j = goingRight( S , mid + 2 , end , chars );
 const size_t maxGoingLeftFirst = size_t( j - i + 1 );
 chars = { S[ mid ] , S[ mid + 1 ] };
 j = goingRight( S , mid + 2 , end , chars );
 i = goingLeft( S , start , mid - 1 , chars );
 const size_t maxGoingRightFirst = size_t( j - i + 1 );
 return( std::max( { maxLeft , maxRight , maxGoingLeftFirst ,
                     maxGoingRightFirst } ) );
 }

size_t maxTwoDivide( const std::string & S )
{
 if( S.empty() )
  return( 0 );
 return( maxTwoRec( S , 0 , std::ptrdiff_t( S.size() ) - 1 ) );
 }

/// the longest run of S with at most two characters: longestKDistinctRun()
/// of the library with k = 2, a sliding window with the counts of its
/// characters, start moving while it has three; O( n )
size_t maxTwo( const std::string & S )
{
 return( longestKDistinctRun( S , 2 ).second );
 }

/// the wrong solution of the text: it empties the set at a third character,
/// losing the second one, which may belong to the best run
size_t maxTwoWrong( const std::string & S )
{
 size_t maxSoFar = 0 , count = 0;
 std::set< char > letters;
 for( char c : S ) {
  if( ! letters.count( c ) && ( letters.size() == 2 ) ) {
   count = 0;
   letters.clear();
   }
  ++count;
  maxSoFar = std::max( maxSoFar , count );
  letters.insert( c );
  }
 return( maxSoFar );
 }

}  // end( namespace divide )

namespace {

using namespace divide;

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// n random items in [ lo , hi ]
Ints randomInts( size_t n , int lo , int hi , std::mt19937 & g )
{
 std::uniform_int_distribution< int > d( lo , hi );
 Ints A( n );
 for( int & a : A )
  a = d( g );
 return( A );
 }

/// n random sorted items in [ lo , hi ]
Ints sortedInts( size_t n , int lo , int hi , std::mt19937 & g )
{
 Ints A = randomInts( n , lo , hi , g );
 std::sort( A.begin() , A.end() );
 return( A );
 }

/// n distinct random items in [ lo , lo + span ), span >= n, shuffled
Ints distinctInts( size_t n , int lo , int span , std::mt19937 & g )
{
 Ints P( span );
 std::iota( P.begin() , P.end() , lo );
 std::shuffle( P.begin() , P.end() , g );
 P.resize( n );
 return( P );
 }

/// a random string of n characters of the alphabet a
std::string randomString( size_t n , const std::string & a ,
                          std::mt19937 & g )
{
 std::string s;
 for( size_t i = 0 ; i < n ; ++i )
  s += a[ g() % a.size() ];
 return( s );
 }

/// a BST with the keys of K inserted in their order, nil if K is empty;
/// the caller owns it
BinaryTree< int > * bst( const Ints & K )
{
 BinaryTree< int > * t = nullptr;
 for( int k : K ) {
  auto * u = new BinaryTree< int >( k );
  if( ! t ) {
   t = u;
   continue;
   }
  for( BinaryTree< int > * p = t ; ; )
   if( k < p->read() ) {
    if( ! p->left() ) {
     p->insertLeft( u );
     break;
     }
    p = p->left();
    }
   else {
    if( ! p->right() ) {
     p->insertRight( u );
     break;
     }
    p = p->right();
    }
  }
 return( t );
 }

/// whether a and b are equal up to a relative error of 1e-9
bool near( double a , double b )
{
 return( std::abs( a - b ) <= 1e-9 * std::max( 1.0 , std::abs( b ) ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/*------------------------- 2011-05-27, Doublegap --------------------------*/

// the double gap on random vectors against a linear scan, on the vectors
// whose only gap is at either end, and on the extreme values
ADS_TEST( "divide-1-b" )
{
 std::mt19937 g( 1 );
 auto gap = [ & ]( const Ints & V , size_t r ) {
  return( ( r + 1 < V.size() ) && ( int64_t( V[ r + 1 ] ) - V[ r ] >= 2 ) );
  };
 CHECK( doublegap( Ints{ 0 , 2 } ) == 0 );
 CHECK( doublegap( Ints{ 5 , 3 , 8 } ) == 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const size_t n = 2 + g() % 40;
  Ints V = randomInts( n , -60 , 60 , g );
  V[ n - 1 ] = V[ 0 ] + int( n ) + int( g() % 4 );
  bool some = false;
  for( size_t i = 0 ; i + 1 < n ; ++i )
   some = some || gap( V , i );
  CHECK( some && gap( V , doublegap( V ) ) );
  }
 for( size_t n = 2 ; n < 200 ; ++n ) {
  Ints V( n );
  std::iota( V.begin() , V.end() , 0 );
  V[ n - 1 ] = int( n );
  CHECK( doublegap( V ) == n - 2 );
  std::iota( V.begin() , V.end() , 1 );
  V[ 0 ] = 0;
  CHECK( doublegap( V ) == 0 );
  }
 CHECK( doublegap( Ints{ INT_MIN , INT_MAX } ) == 0 );
 CHECK( doublegap( Ints{ INT_MIN , 0 , 1 , INT_MAX } ) == 0 );
 CHECK_THROWS( doublegap( Ints{ 0 , 1 } ) , contract_violation );
 CHECK_THROWS( doublegap( Ints{ 3 } ) , contract_violation );
 }

/*-------------------------- 2013-07-22, Maxprod ---------------------------*/

// the largest product against all the runs, and against maxSubarraySum()
// of the library on the logarithms when the items are positive; a 0 after
// a product grown to +inf (once NaN), underflow, negative and NaN items
ADS_TEST( "divide-2" )
{
 std::mt19937 g( 1 );
 std::uniform_real_distribution< double > d( 0 , 3 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  std::vector< double > X( g() % 13 );
  for( double & x : X )
   x = ( g() % 5 == 0 ) ? 0 : d( g );
  double best = 1;
  for( size_t i = 0 ; i < X.size() ; ++i ) {
   double p = 1;
   for( size_t j = i ; j < X.size() ; ++j )
    best = std::max( best , p *= X[ j ] );
   }
  CHECK( near( maxprod( X ) , best ) );
  if( std::all_of( X.begin() , X.end() ,
                   []( double x ) { return( x > 0 ); } ) ) {
   std::vector< double > L;
   for( double x : X )
    L.push_back( std::log( x ) );
   CHECK( near( std::log( maxprod( X ) ) , maxSubarraySum( L ) ) );
   }
  }
 CHECK( maxprod( {} ) == 1 );
 CHECK( maxprod( { 0 , 0 , 0 } ) == 1 );
 CHECK( maxprod( { 0.5 } ) == 1 );
 CHECK( maxprod( { 5 } ) == 5 );
 CHECK( maxprod( { 2 , 2 , 2 , 2 } ) == 16 );
 CHECK( maxprod( { 2 , 0.25 , 3 , 0 , 5 } ) == 5 );
 CHECK( maxprod( { 4 , 0.5 , 4 } ) == 8 );
 CHECK( maxprod( { 1e300 , 1e300 } ) == HUGE_VAL );
 CHECK( maxprod( { 1e300 , 1e300 , 0 , 2 } ) == HUGE_VAL );
 CHECK( maxprod( { HUGE_VAL , 0 } ) == HUGE_VAL );
 CHECK( maxprod( { 0 , HUGE_VAL , 1e-300 } ) == HUGE_VAL );
 CHECK( maxprod( { 1e-300 , 1e-300 } ) == 1 );
 CHECK_THROWS( maxprod( { 1 , -1 } ) , contract_violation );
 CHECK_THROWS( maxprod( { -0.5 } ) , contract_violation );
 CHECK_THROWS( maxprod( { 2 , std::nan( "" ) } ) , contract_violation );
 }

/*------------------------ 2014-04-24, Pythagorean -------------------------*/

// the triples against the loop on three distinct places, with negative
// items, zeros and the largest values
ADS_TEST( "divide-3" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = randomInts( g() % 10 , -15 , 15 , g );
  bool found = false;
  for( size_t i = 0 ; i < A.size() ; ++i )
   for( size_t j = i + 1 ; j < A.size() ; ++j )
    for( size_t k = 0 ; k < A.size() ; ++k )
     if( ( k != i ) && ( k != j ) &&
         ( A[ i ] * A[ i ] + A[ j ] * A[ j ] == A[ k ] * A[ k ] ) )
      found = true;
  CHECK( pythagorean( A ) == found );
  }
 CHECK( pythagorean( { 3 , 4 , 5 } ) );
 CHECK( pythagorean( { 13 , 1 , 12 , 7 , 5 } ) );
 CHECK( pythagorean( { 3 , -4 , -5 } ) );
 CHECK( pythagorean( { 0 , 3 , 3 } ) );
 CHECK( pythagorean( { 0 , 0 , 0 } ) );
 CHECK( ! pythagorean( { 0 , 3 } ) );
 CHECK( ! pythagorean( { 0 } ) );
 CHECK( ! pythagorean( { 0 , 0 } ) );
 CHECK( ! pythagorean( { 1 , 1 , 1 } ) );
 CHECK( ! pythagorean( {} ) );
 CHECK( ! pythagorean( { INT_MAX , INT_MAX , INT_MAX } ) );
 CHECK( pythagorean( { INT_MIN , 0 , INT_MIN } ) );
 const int k = INT_MAX / 5;
 CHECK( pythagorean( { 5 * k , 4 * k , 3 * k } ) );
 CHECK( ! pythagorean( { 5 * k , 4 * k , 3 * k + 1 } ) );
 }

/*--------------------------- 2014-06-05, Equal ----------------------------*/

// equalTrees() of the library, the same root key and equal subtrees: the
// equality of BSTs against their parenthesized lists by to_list() of the
// library, on random insertions of the same and of different keys
ADS_TEST( "divide-4" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 2000 ; ++t ) {
  Ints K = distinctInts( g() % 9 , 0 , 12 , g ) , H = K;
  if( g() % 2 )
   std::shuffle( H.begin() , H.end() , g );
  else if( ! H.empty() && ( g() % 2 ) )
   H[ g() % H.size() ] = 100;
  BinaryTree< int > * a = bst( K ) , * b = bst( H );
  CHECK( equalTrees( a , b ) == ( to_list( a ) == to_list( b ) ) );
  CHECK( equalTrees( a , a ) && equalTrees( b , b ) );
  delete a;
  delete b;
  }
 BinaryTree< int > * a = bst( { 2 , 1 , 3 } ) , * b = bst( { 2 , 3 , 1 } );
 BinaryTree< int > * c = bst( { 1 , 2 , 3 } ) , * d = bst( { 2 , 1 , 4 } );
 BinaryTree< int > * e = bst( { 2 } ) , * const nil = nullptr;
 CHECK( equalTrees( a , b ) && ! equalTrees( a , c ) &&
        ! equalTrees( a , d ) );
 CHECK( ! equalTrees( a , e ) && ! equalTrees( e , a ) );
 CHECK( equalTrees( nil , nil ) );
 CHECK( ! equalTrees( e , nil ) && ! equalTrees( nil , e ) );
 BinaryTree< int > * x = bst( { INT_MAX , INT_MIN } );
 BinaryTree< int > * y = bst( { INT_MAX , INT_MIN } );
 CHECK( equalTrees( x , y ) );
 for( auto * t : { a , b , c , d , e , x , y } )
  delete t;
 }

/*------------------------- 2014-10-31, Thickness --------------------------*/

// the quadratic and the divide-and-conquer thickness against the loop on
// all the runs; the order of the moves printed in the text fails on four
// equal items
ADS_TEST( "divide-5" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints V = randomInts( g() % 15 , -5 , 5 , g );
  const int C = 1 + int( g() % 6 );
  size_t best = 0;
  for( size_t i = 0 ; i < V.size() ; ++i )
   for( size_t j = i ; j < V.size() ; ++j ) {
    const auto [ lo , hi ] = std::minmax_element( V.begin() + long( i ) ,
                                                  V.begin() + long( j ) + 1 );
    if( *hi - *lo <= C )
     best = std::max( best , j - i + 1 );
    }
  CHECK( thicknessQuadratic( V , C ) == best );
  CHECK( thickness( V , C ) == best );
  }
 CHECK( thickness( { 1 , 1 , 1 , 1 } , 1 ) == 4 );
 CHECK( thickness( { 1 , 1 , 1 , 1 } , 1 , true ) == 3 );
 CHECK( thickness( {} , 1 ) == 0 );
 CHECK( thickness( { 7 } , 1 ) == 1 );
 CHECK( thickness( { 1 , 5 , 9 , 13 } , 3 ) == 1 );
 CHECK( thickness( Ints( 1000 , 3 ) , 1 ) == 1000 );
 CHECK( thickness( { INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK( thickness( { 0 , INT_MAX , INT_MIN , -1 } , INT_MAX ) == 2 );
 CHECK( thicknessQuadratic( { INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK_THROWS( thickness( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( thickness( {} , -1 ) , contract_violation );
 CHECK_THROWS( thicknessQuadratic( { 1 } , 0 ) , contract_violation );
 }

/*------------------------- 2015-09-01, Find peak --------------------------*/

// the peak of the example of the text, of every permutation of up to 7
// items, of random vectors and of the monotone ones
ADS_TEST( "divide-6-b" )
{
 std::mt19937 g( 1 );
 auto peak = [ & ]( const Ints & A , size_t r ) {
  return( ( r < A.size() ) && ( ( r == 0 ) || ( A[ r - 1 ] < A[ r ] ) ) &&
          ( ( r + 1 == A.size() ) || ( A[ r ] > A[ r + 1 ] ) ) );
  };
 const size_t r = findPeak( { 5 , 7 , 9 , 6 , 12 , 8 , 10 } );
 CHECK( ( r == 2 ) || ( r == 4 ) || ( r == 6 ) );
 for( size_t n = 1 ; n <= 7 ; ++n ) {
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  do
   CHECK( peak( A , findPeak( A ) ) );
  while( std::next_permutation( A.begin() , A.end() ) );
  }
 for( int t = 0 ; t < 2000 ; ++t ) {
  const size_t n = 1 + g() % 60;
  const Ints A = distinctInts( n , -100 , 200 , g );
  CHECK( peak( A , findPeak( A ) ) );
  }
 Ints A( 1000 );
 std::iota( A.begin() , A.end() , 0 );
 CHECK( findPeak( A ) == 999 );
 std::reverse( A.begin() , A.end() );
 CHECK( findPeak( A ) == 0 );
 CHECK( findPeak( { INT_MIN , INT_MAX } ) == 1 );
 CHECK( findPeak( { INT_MAX , INT_MIN } ) == 0 );
 CHECK_THROWS( findPeak( {} ) , contract_violation );
 CHECK_THROWS( findPeak( { 1 , 1 } ) , contract_violation );
 }

/*------------------------ 2015-11-05, Compute sum -------------------------*/

// the frequency and the sum of the example and of random vectors of two
// values against std::count and upperBound() of the library
ADS_TEST( "divide-7" )
{
 std::mt19937 g( 1 );
 const Ints E{ 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 };
 CHECK( mostFrequent( E ) == 6 );
 CHECK( computeSum( E ) == 14 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const size_t n = 2 + g() % 50 , k = 1 + g() % ( n - 1 );
  const int a = int( g() % 100 ) - 50 , b = a + 1 + int( g() % 20 );
  Ints A( k , a );
  A.resize( n , b );
  const size_t na = size_t( std::count( A.begin() , A.end() , a ) );
  CHECK( countA( A ) == na );
  CHECK( countA( A ) == upperBound( A , a ) );
  CHECK( mostFrequent( A ) == std::max( na , n - na ) );
  CHECK( computeSum( A ) == std::accumulate( A.begin() , A.end() ,
                                             int64_t( 0 ) ) );
  }
 CHECK( mostFrequent( { 0 , 1 } ) == 1 );
 CHECK( computeSum( { INT_MIN , INT_MAX } ) == -1 );
 const Ints M{ INT_MAX - 1 , INT_MAX , INT_MAX , INT_MAX };
 CHECK( computeSum( M ) == 4 * int64_t( INT_MAX ) - 1 );
 CHECK( mostFrequent( M ) == 3 );
 CHECK_THROWS( countA( { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( countA( { 1 } ) , contract_violation );
 CHECK_THROWS( countA( { 2 , 1 } ) , contract_violation );
 CHECK_THROWS( countA( { 1 , 2 , 1 , 2 } ) , contract_violation );
 CHECK_THROWS( countA( { 1 , 2 , 3 } ) , contract_violation );
 }

/*------------------------ 2016-11-03, Punto fisso -------------------------*/

// the zero of random walks of steps -1, 0, 1 from a positive item to a
// negative one, and of their mirror images
ADS_TEST( "divide-8" )
{
 std::mt19937 g( 1 );
 CHECK( zero( { 1 , 0 , -1 } ) == 1 );
 CHECK( zero( { -1 , 0 , 1 } ) == 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints V{ 1 + int( g() % 3 ) };
  const size_t n = 2 + g() % 40;
  while( ( V.size() < n ) || ( V.back() >= 0 ) )
   V.push_back( V.back() + ( V.size() < n ? int( g() % 3 ) - 1 : -1 ) );
  CHECK( V[ zero( V ) ] == 0 );
  for( int & v : V )
   v = -v;
  CHECK( V[ zero( V ) ] == 0 );
  }
 Ints L( 100001 );
 std::iota( L.begin() , L.end() , -50000 );
 CHECK( zero( L ) == 50000 );
 Ints W( 9 , 0 );
 W.front() = 1;
 W.back() = -1;
 CHECK( W[ zero( W ) ] == 0 );
 CHECK_THROWS( zero( { 1 , -1 } ) , contract_violation );
 CHECK_THROWS( zero( { 1 , 0 , 1 } ) , contract_violation );
 CHECK_THROWS( zero( { 2 , 0 , -2 } ) , contract_violation );
 }

/*------------------------- 2018-01-08, Max crime --------------------------*/

// the window of the example and of random sorted vectors against
// std::upper_bound from each item
ADS_TEST( "divide-9" )
{
 std::mt19937 g( 1 );
 CHECK( maxCrime( { 10 , 17 , 19 , 23 , 24 , 26 } , 4 ) == 3 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = sortedInts( g() % 30 , 1 , 40 , g );
  const int w = 1 + int( g() % 12 );
  size_t best = 0;
  for( auto it = A.begin() ; it != A.end() ; ++it )
   best = std::max( best , size_t( std::upper_bound( it , A.end() ,
                                                    *it + w - 1 ) - it ) );
  CHECK( maxCrime( A , w ) == best );
  }
 CHECK( maxCrime( {} , 5 ) == 0 );
 CHECK( maxCrime( { 8 } , 1 ) == 1 );
 CHECK( maxCrime( Ints( 50 , 8 ) , 1 ) == 50 );
 CHECK( maxCrime( { 1 , 2 , 3 , 4 } , 1 ) == 1 );
 CHECK( maxCrime( { 1 , INT_MAX } , INT_MAX ) == 2 );
 CHECK( maxCrime( { 1 , INT_MAX } , INT_MAX - 1 ) == 1 );
 CHECK( maxCrime( { -5 , -4 , -4 , 0 } , 2 ) == 3 );
 CHECK( maxCrime( { INT_MIN , INT_MAX } , INT_MAX ) == 1 );
 CHECK_THROWS( maxCrime( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( maxCrime( {} , -1 ) , contract_violation );
 CHECK_THROWS( maxCrime( { 3 , 1 } , 5 ) , contract_violation );
 }

/*------------------------ 2018-01-31, Valore unico ------------------------*/

// the single value by the three solutions of 2018-01-31 (sorted pairs),
// 2021-01-18 (consecutive pairs of distinct values) and 2024-02-01 (pairs
// whose values may come back, not next to each other), on the examples of
// the three texts and on random vectors of the three kinds, each against the
// exclusive or of the items
ADS_TEST( "divide-10" )
{
 std::mt19937 g( 1 );
 CHECK( single( { 1 , 1 , 3 , 3 , 4 , 4 , 5 , 5 , 7 , 7 , 8 } ) == 8 );
 const Ints S{ 1 , 1 , 4 , 4 , 2 , 2 , 0 , 1000 , 1000 , -2 , -2 };
 CHECK( findSingle( S ) == 0 );
 CHECK( oneSingle( { 2 , 2 , 3 , 4 , 4 , 2 , 2 , -1 , -1 } ) == 3 );
 auto xorAll = []( const Ints & A ) {
  int x = 0;
  for( int a : A )
   x ^= a;
  return( x );
  };
 for( int t = 0 ; t < 3000 ; ++t ) {
  const size_t p = g() % 15 , at = g() % ( p + 1 );
  // pairs of distinct values, sorted for the first kind
  Ints D = distinctInts( p + 1 , -40 , 100 , g ) , A , B , C;
  const int x = D.back();
  D.pop_back();
  for( size_t i = 0 ; i <= p ; ++i ) {
   if( i == at )
    B.push_back( x );
   if( i < p )
    B.insert( B.end() , 2 , D[ i ] );
   }
  D.push_back( x );
  std::sort( D.begin() , D.end() );
  for( int d : D )
   A.insert( A.end() , size_t( d == x ? 1 : 2 ) , d );
  // pairs of values in 0 .. 3, two pairs next to each other differing
  int last = -1;
  for( size_t i = 0 ; i <= p ; ++i ) {
   if( i == at ) {
    C.push_back( 99 );
    last = -1;
    }
   if( i < p ) {
    int v;
    do
     v = int( g() % 4 );
    while( v == last );
    C.insert( C.end() , 2 , v );
    last = v;
    }
   }
  CHECK( single( A ) == x && findSingle( A ) == x && oneSingle( A ) == x );
  CHECK( xorAll( A ) == x && xorAll( B ) == x && xorAll( C ) == 99 );
  CHECK( single( B ) == x && findSingle( B ) == x && oneSingle( B ) == x );
  CHECK( single( C ) == 99 && findSingle( C ) == 99 &&
         oneSingle( C ) == 99 );
  }
 for( int x : { 0 , INT_MIN , INT_MAX } ) {
  CHECK( single( { x } ) == x && findSingle( { x } ) == x &&
         oneSingle( { x } ) == x );
  const Ints E{ INT_MIN , INT_MIN , x , INT_MAX , INT_MAX };
  if( ( x != INT_MIN ) && ( x != INT_MAX ) )
   CHECK( single( E ) == x && findSingle( E ) == x && oneSingle( E ) == x );
  }
 CHECK_THROWS( single( { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( findSingle( {} ) , contract_violation );
 CHECK_THROWS( oneSingle( { 1 , 1 } ) , contract_violation );
 }

/*---------------------- 2018-08-21, Vettori traslati ----------------------*/

// the shift of the examples and of every rotation of random sorted vectors
// against std::min_element
ADS_TEST( "divide-11" )
{
 std::mt19937 g( 1 );
 CHECK( shift( { 35 , 42 , 1 , 7 , 15 , 18 , 28 , 30 } ) == 2 );
 CHECK( shift( { 15 , 18 , 28 , 30 , 35 , 42 , 1 , 7 } ) == 6 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints A = distinctInts( 1 + g() % 40 , 1 , 100 , g );
  std::sort( A.begin() , A.end() );
  for( size_t k = 0 ; k < A.size() ; ++k ) {
   Ints R( A );
   std::rotate( R.begin() , R.end() - long( k ) , R.end() );
   CHECK( shift( R ) == k );
   CHECK( shift( R ) == size_t( std::min_element( R.begin() , R.end() ) -
                                R.begin() ) );
   }
  }
 CHECK( shift( { 5 } ) == 0 );
 CHECK( shift( { INT_MAX , INT_MIN } ) == 1 );
 CHECK( shift( { INT_MIN , 0 , INT_MAX } ) == 0 );
 CHECK_THROWS( shift( {} ) , contract_violation );
 }

/*------------------------- 2019-01-21, Successore -------------------------*/

// the successor by upperBound() of the library on the examples and on
// random sorted vectors against std::upper_bound
ADS_TEST( "divide-12" )
{
 std::mt19937 g( 1 );
 const Ints E{ 1 , 3 , 3 , 4 , 5 , 5 , 8 };
 CHECK( successor( E , 6 ) == 8 );
 CHECK( successor( E , 8 ) == -1 );
 CHECK( successor( E , 3 ) == 4 );
 CHECK( successor( E , 0 ) == 1 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = sortedInts( g() % 20 , 1 , 30 , g );
  const int v = int( g() % 34 ) - 2;
  const auto it = std::upper_bound( A.begin() , A.end() , v );
  CHECK( successor( A , v ) == ( it == A.end() ? -1 : int64_t( *it ) ) );
  }
 CHECK( successor( {} , 3 ) == -1 );
 CHECK( successor( { 5 } , 4 ) == 5 );
 CHECK( successor( { 5 } , 5 ) == -1 );
 CHECK( successor( Ints( 9 , 2 ) , 1 ) == 2 );
 CHECK( successor( { 1 , INT_MAX } , INT_MAX ) == -1 );
 CHECK( successor( { 1 , INT_MAX } , INT_MAX - 1 ) == INT_MAX );
 CHECK( successor( { 1 , INT_MAX } , INT_MIN ) == 1 );
 }

/*-------------------------- 2020-01-14, Intruder --------------------------*/

// the intruder of the example and of random sorted vectors, in every place,
// against the difference of the sums
ADS_TEST( "divide-13" )
{
 std::mt19937 g( 1 );
 CHECK( intruder( { 2 , 4 , 6 , 8 , 10 , 12 } ,
                  { 2 , 4 , 6 , 8 , 9 , 10 , 12 } ) == 9 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints B = distinctInts( 1 + g() % 40 , -50 , 120 , g );
  std::sort( B.begin() , B.end() );
  for( size_t k = 0 ; k < B.size() ; ++k ) {
   Ints A( B );
   A.erase( A.begin() + long( k ) );
   CHECK( intruder( A , B ) == B[ k ] );
   CHECK( intruder( A , B ) ==
          std::accumulate( B.begin() , B.end() , int64_t( 0 ) ) -
          std::accumulate( A.begin() , A.end() , int64_t( 0 ) ) );
   }
  }
 CHECK( intruder( {} , { 7 } ) == 7 );
 CHECK( intruder( { INT_MAX } , { INT_MIN , INT_MAX } ) == INT_MIN );
 CHECK( intruder( { INT_MIN } , { INT_MIN , INT_MAX } ) == INT_MAX );
 CHECK_THROWS( intruder( { 1 } , { 1 } ) , contract_violation );
 }

/*----------------------- 2020-06-12, Mistery array ------------------------*/

// the reads of the example, and the negatives of random sorted vectors
// against std::lower_bound, with at most 2 log2 n + 4 reads
ADS_TEST( "divide-14" )
{
 std::mt19937 g( 1 );
 const Ints E{ -5 , -3 , 11 , 13 , 11 };
 const MisteryArray M( E );
 CHECK( M[ 3 ] == 13 && ! M[ 999 ] && ! M[ 5 ] && M[ 4 ] == 11 );
 CHECK( M.reads() == 4 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const size_t n = 1 + g() % ( t < 2000 ? 40 : 5000 );
  const Ints A = sortedInts( n , -1000 , 1000 , g );
  const MisteryArray X( A );
  CHECK( countNegatives( X ) == size_t( std::lower_bound( A.begin() ,
                                        A.end() , 0 ) - A.begin() ) );
  CHECK( X.reads() <= 2 * long( std::bit_width( n ) ) + 4 );
  }
 for( size_t n : { size_t( 1 ) , size_t( 2 ) , size_t( 3 ) , size_t( 64 ) ,
                   size_t( 65 ) , size_t( 100000 ) } ) {
  const Ints N( n , INT_MIN ) , P( n , 0 ) , Q( n , INT_MAX );
  CHECK( countNegatives( MisteryArray( N ) ) == n );
  CHECK( countNegatives( MisteryArray( P ) ) == 0 );
  CHECK( countNegatives( MisteryArray( Q ) ) == 0 );
  }
 const Ints Z;
 CHECK_THROWS( countNegatives( MisteryArray( Z ) ) , contract_violation );
 }

/*------------------------ 2020-07-03, Maggioranza -------------------------*/

// the majority of random sorted vectors of few values against the count of
// the most frequent value, and the bounds n / 2 and n / 2 + 1
ADS_TEST( "divide-15" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 5000 ; ++t ) {
  const Ints A = sortedInts( 1 + g() % 30 , 0 , 1 + int( g() % 4 ) , g );
  std::map< int , size_t > c;
  size_t most = 0;
  for( int a : A )
   most = std::max( most , ++c[ a ] );
  CHECK( hasMajority( A ) == ( 2 * most > A.size() ) );
  }
 CHECK( hasMajority( { 4 } ) );
 CHECK( hasMajority( { 1 , 1 } ) && ! hasMajority( { 1 , 2 } ) );
 CHECK( ! hasMajority( { 1 , 1 , 2 , 2 } ) );
 CHECK( hasMajority( { 1 , 1 , 1 , 2 } ) && hasMajority( { 1 , 2 , 2 , 2 } ) );
 CHECK( ! hasMajority( { 1 , 2 , 3 } ) && hasMajority( { 1 , 3 , 3 } ) );
 CHECK( hasMajority( Ints( 1000 , INT_MAX ) ) );
 CHECK( hasMajority( { INT_MIN , INT_MIN , INT_MAX } ) );
 CHECK_THROWS( hasMajority( {} ) , contract_violation );
 CHECK_THROWS( hasMajority( { 2 , 1 , 2 } ) , contract_violation );
 }

/*-------------------------- 2021-02-08, Closest ---------------------------*/

// the closest items of the examples and of random sorted vectors against a
// linear scan, any item at the least distance being right; the test of the
// text fails on equal items
ADS_TEST( "divide-17" )
{
 std::mt19937 g( 1 );
 auto best = []( const Ints & A , int x ) {
  int64_t d = std::abs( int64_t( A[ 0 ] ) - x );
  for( int a : A )
   d = std::min( d , std::abs( int64_t( a ) - x ) );
  return( d );
  };
 const Ints E{ -3 , -1 , -1 , 2 , 4 , 7 };
 CHECK( closest( E , 0 ) == -1 );
 CHECK( closest( E , 1 ) == 2 && closest( E , 2 ) == 2 );
 CHECK( ( closest( E , 3 ) == 2 ) || ( closest( E , 3 ) == 4 ) );
 for( int t = 0 ; t < 5000 ; ++t ) {
  const Ints A = sortedInts( 1 + g() % 20 , -30 , 30 , g );
  const int x = int( g() % 80 ) - 40;
  const int r = closest( A , x );
  CHECK( std::binary_search( A.begin() , A.end() , r ) );
  CHECK( std::abs( int64_t( r ) - x ) == best( A , x ) );
  }
 CHECK( closest( { 0 , 3 , 3 } , 1 ) == 0 );
 CHECK( closest( { 0 , 3 , 3 } , 1 , true ) == 3 );
 CHECK( closest( { 5 } , INT_MIN ) == 5 );
 CHECK( closest( Ints( 7 , 3 ) , 0 ) == 3 );
 CHECK( closest( { INT_MIN , INT_MAX } , INT_MIN ) == INT_MIN );
 CHECK( closest( { INT_MIN , INT_MAX } , INT_MAX ) == INT_MAX );
 CHECK( closest( { INT_MIN , 0 , INT_MAX } , 1 ) == 0 );
 CHECK( closest( { 1 , 3 } , 2 ) == 1 || closest( { 1 , 3 } , 2 ) == 3 );
 CHECK( closest( { -7 , -2 } , INT_MAX ) == -2 );
 CHECK_THROWS( closest( {} , 0 ) , contract_violation );
 CHECK_THROWS( closest( { 3 , 1 } , 0 ) , contract_violation );
 }

/*-------------------------- 2021-02-19, Fast sum --------------------------*/

// the sum of the example and of random sorted vectors of 1, 2, 3 against
// std::accumulate
ADS_TEST( "divide-18" )
{
 std::mt19937 g( 1 );
 CHECK( fastSum( { 1 , 2 , 2 , 2 , 3 , 3 } ) == 13 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  Ints A( 1 + g() % 30 , 1 );
  A.resize( A.size() + 1 + g() % 30 , 2 );
  A.resize( A.size() + 1 + g() % ( t < 2900 ? 30 : 100000 ) , 3 );
  CHECK( fastSum( A ) == std::accumulate( A.begin() , A.end() ,
                                          int64_t( 0 ) ) );
  }
 CHECK( fastSum( { 1 , 2 , 3 } ) == 6 );
 CHECK_THROWS( fastSum( { 1 , 3 } ) , contract_violation );
 CHECK_THROWS( fastSum( { 1 , 1 , 3 } ) , contract_violation );
 CHECK_THROWS( fastSum( {} ) , contract_violation );
 CHECK_THROWS( fastSum( { 2 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( fastSum( { 1 , 2 , 2 } ) , contract_violation );
 CHECK_THROWS( fastSum( { 1 , 3 , 2 , 3 } ) , contract_violation );
 }

/*------------------------ 2021-08-31, ContainsZero ------------------------*/

// the runs of sum 0 of the examples and of random sorted vectors by the
// prefix sums and by the linear scan, against the double loop on the runs
ADS_TEST( "divide-19" )
{
 std::mt19937 g( 1 );
 for( const auto & [ A , z ] : std::vector< std::pair< Ints , bool > >{
       { { -5 , -4 , -1 , 2 , 3 , 4 , 7 } , true } ,
       { { -2 , 0 , 1 } , true } , { { -6 , -3 , 2 , 3 , 7 } , false } } ) {
  CHECK( containsZero( A ) == z );
  CHECK( containsZeroSort( A ) == z );
  }
 for( int t = 0 ; t < 5000 ; ++t ) {
  const Ints A = sortedInts( g() % 13 , -8 , 8 , g );
  bool z = false;
  for( size_t i = 0 ; i < A.size() ; ++i ) {
   int64_t tot = 0;
   for( size_t j = i ; j < A.size() ; ++j )
    z = z || ( ( tot += A[ j ] ) == 0 );
   }
  CHECK( containsZero( A ) == z );
  CHECK( containsZeroSort( A ) == z );
  }
 CHECK( ! containsZero( {} ) && ! containsZeroSort( {} ) );
 CHECK( containsZero( { 0 } ) && ! containsZero( { 5 } ) );
 CHECK( ! containsZero( { -3 } ) && ! containsZero( Ints( 9 , 4 ) ) );
 CHECK( containsZero( Ints( 9 , 0 ) ) );
 CHECK( containsZero( { -INT_MAX , INT_MAX } ) );
 CHECK( ! containsZero( { INT_MIN , INT_MAX } ) );
 CHECK( containsZero( { INT_MIN , 1 , INT_MAX } ) );
 CHECK( containsZeroSort( { INT_MIN , 1 , INT_MAX } ) );
 CHECK( ! containsZero( { INT_MIN , INT_MIN , INT_MAX , INT_MAX } ) );
 CHECK( containsZeroSort( { 3 , -3 } ) );
 CHECK_THROWS( containsZero( { 3 , -3 } ) , contract_violation );
 }

/*---------------------- 2021-08-31, Radice quadrata -----------------------*/

// the squares up to 10^6 against a running root, and the squares of random
// and of the largest roots of 64 bits and their neighbours
ADS_TEST( "divide-20" )
{
 std::mt19937_64 g( 1 );
 std::uint64_t k = 1;
 for( std::uint64_t n = 1 ; n <= 1000000 ; ++n ) {
  while( ( k + 1 ) * ( k + 1 ) <= n )
   ++k;
  CHECK( isSquare( n ) == ( k * k == n ) );
  }
 for( int t = 0 ; t < 3000 ; ++t ) {
  const std::uint64_t r = 2 + g() % 0xFFFFFFFFull;
  CHECK( isSquare( r * r ) );
  CHECK( ! isSquare( r * r - 1 ) && ! isSquare( r * r + 1 ) );
  }
 const std::uint64_t m = 0xFFFFFFFFull;
 CHECK( isSquare( m * m ) && ! isSquare( m * m + 1 ) );
 CHECK( ! isSquare( m * m - 1 ) );
 CHECK( ! isSquare( UINT64_MAX ) );
 CHECK( isSquare( 1 ) && ! isSquare( 2 ) && isSquare( 4 ) );
 CHECK_THROWS( isSquare( 0 ) , contract_violation );
 }

/*------------------ 2022-01-17, Serie aritmetica bucata -------------------*/

// the missing term of the example and of random progressions, each inner
// term left out, against the formula of the sum
ADS_TEST( "divide-21" )
{
 std::mt19937 g( 1 );
 CHECK( missing( { 3 , 5 , 7 , 9 , 11 , 15 , 17 } ) == 13 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const size_t n = 2 + g() % 40;
  const int x = int( g() % 101 ) - 50 , d = 1 + int( g() % 10 );
  for( size_t k = 1 ; k < n ; ++k ) {
   Ints A;
   for( size_t i = 0 ; i <= n ; ++i )
    if( i != k )
     A.push_back( x + int( i ) * d );
   const int64_t m = x + int64_t( k ) * d;
   CHECK( missing( A ) == m );
   CHECK( missing( A ) == int64_t( n + 1 ) * ( A.front() + A.back() ) / 2 -
                          std::accumulate( A.begin() , A.end() ,
                                           int64_t( 0 ) ) );
   }
  }
 for( size_t n = 2 ; n < 40 ; ++n )
  for( size_t k = 1 ; k < n ; ++k ) {
   Ints A;
   for( size_t i = 0 ; i <= n ; ++i )
    if( i != k )
     A.push_back( 100 - 3 * int( i ) );
   CHECK( missing( A ) == 100 - 3 * int64_t( k ) );
   }
 CHECK( missing( { 1 , 3 } ) == 2 && missing( { 3 , 1 } ) == 2 );
 CHECK( missing( { INT_MAX , INT_MIN + 1 } ) == 0 );
 CHECK( missing( { INT_MIN + 1 , INT_MAX } ) == 0 );
 const int d = INT_MAX / 2;
 CHECK( missing( { INT_MIN , INT_MIN + d , INT_MIN + d + d + d } ) ==
        int64_t( INT_MIN ) + 2 * d );
 CHECK_THROWS( missing( { 1 } ) , contract_violation );
 }

/*-------------------------- 2022-07-13, H-index ---------------------------*/

// the h-index of the examples and of random non-increasing vectors against
// its definition
ADS_TEST( "divide-22" )
{
 std::mt19937 g( 1 );
 CHECK( hIndex( { 16 , 12 , 7 , 5 , 4 , 3 , 2 , 1 } ) == 4 );
 CHECK( hIndex( { 54 , 17 , 14 , 10 , 9 , 6 , 6 , 5 , 4 , 3 } ) == 6 );
 for( int t = 0 ; t < 5000 ; ++t ) {
  Ints A = sortedInts( g() % 30 , 0 , int( g() % 40 ) , g );
  std::reverse( A.begin() , A.end() );
  size_t h = 0;
  for( size_t c = 0 ; c <= A.size() ; ++c )
   if( size_t( std::count_if( A.begin() , A.end() , [ & ]( int a ) {
        return( size_t( a ) >= c ); } ) ) >= c )
    h = c;
  CHECK( hIndex( A ) == h );
  }
 CHECK( hIndex( {} ) == 0 );
 CHECK( hIndex( Ints( 9 , 0 ) ) == 0 );
 CHECK( hIndex( { 1 } ) == 1 );
 CHECK( hIndex( Ints( 9 , 9 ) ) == 9 );
 CHECK( hIndex( Ints( 1000 , INT_MAX ) ) == 1000 );
 CHECK( hIndex( { 3 , -1 , -5 } ) == 1 && hIndex( { -1 , INT_MIN } ) == 0 );
 CHECK( hIndex( { 5 , 4 , 3 , 2 , 1 } ) == 3 );
 CHECK_THROWS( hIndex( { 1 , 2 } ) , contract_violation );
 }

/*----------------------- 2023-02-09, Minimo locale ------------------------*/

// the local minimum of the example and of random vectors with the two ends
// rising, checked by its neighbours
ADS_TEST( "divide-23" )
{
 std::mt19937 g( 1 );
 const size_t r = findLocalMinimum( { 17 , 15 , 13 , 14 , 12 , 18 } );
 CHECK( ( r == 2 ) || ( r == 4 ) );
 for( int t = 0 ; t < 5000 ; ++t ) {
  const size_t n = 3 + g() % 50;
  Ints A = distinctInts( n , -100 , 300 , g );
  if( A[ 0 ] < A[ 1 ] )
   std::swap( A[ 0 ] , A[ 1 ] );
  if( A[ n - 2 ] > A[ n - 1 ] )
   std::swap( A[ n - 2 ] , A[ n - 1 ] );
  const size_t m = findLocalMinimum( A );
  CHECK( ( 0 < m ) && ( m < n - 1 ) );
  CHECK( ( A[ m - 1 ] > A[ m ] ) && ( A[ m ] < A[ m + 1 ] ) );
  }
 CHECK( findLocalMinimum( { 3 , 1 , 2 } ) == 1 );
 CHECK( findLocalMinimum( { INT_MAX , INT_MIN , 0 } ) == 1 );
 Ints V( 1001 );
 for( size_t i = 0 ; i < V.size() ; ++i )
  V[ i ] = int( i > 700 ? 2 * i : 1400 - i );
 CHECK( findLocalMinimum( V ) == 700 );
 CHECK_THROWS( findLocalMinimum( { 1 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( findLocalMinimum( { 2 , 1 } ) , contract_violation );
 CHECK_THROWS( findLocalMinimum( { 3 , 1 , 1 , 2 } ) , contract_violation );
 }

/*----------------------- 2024-01-09, k-Ripetizioni ------------------------*/

// the longest run of characters repeated k times of the examples and of
// random strings against the extension of each run with the counts
ADS_TEST( "divide-24" )
{
 std::mt19937 g( 1 );
 CHECK( krepeating( "baaab" , 3 ) == 3 );
 CHECK( krepeating( "babbana" , 2 ) == 5 );
 CHECK( krepeating( "aabb" , 2 ) == 4 );
 CHECK( krepeating( "banana" , 1 ) == 6 );
 CHECK( krepeating( "banana" , 3 ) == 0 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const std::string S = randomString( g() % 15 , "abc" , g );
  const int k = 1 + int( g() % 4 );
  size_t best = 0;
  for( size_t i = 0 ; i < S.size() ; ++i ) {
   std::map< char , int > f;
   for( size_t j = i ; j < S.size() ; ++j ) {
    ++f[ S[ j ] ];
    if( std::all_of( f.begin() , f.end() , [ & ]( const auto & p ) {
         return( p.second >= k ); } ) )
     best = std::max( best , j - i + 1 );
    }
   }
  CHECK( krepeating( S , k ) == best );
  }
 CHECK( krepeating( "" , 1 ) == 0 );
 CHECK( krepeating( "x" , 1 ) == 1 && krepeating( "x" , 2 ) == 0 );
 CHECK( krepeating( "aaaa" , 4 ) == 4 && krepeating( "aaaa" , 5 ) == 0 );
 CHECK( krepeating( std::string( 2000 , 'z' ) , 2000 ) == 2000 );
 const std::string Z = "abcdefghijklmnopqrstuvwxyz";
 CHECK( krepeating( Z , 1 ) == 26 && krepeating( Z , 2 ) == 0 );
 CHECK( krepeating( Z + Z , 2 ) == 52 );
 CHECK( krepeating( "ab" , INT_MAX ) == 0 );
 CHECK_THROWS( krepeating( "a" , 0 ) , contract_violation );
 CHECK_THROWS( krepeating( "" , -1 ) , contract_violation );
 }

/*----------------------- 2024-06-14, Smaller Count ------------------------*/

// the smaller items on the right of the example and of random vectors of
// distinct items against the double loop, their sum against
// countInversions() of the library
ADS_TEST( "divide-26" )
{
 std::mt19937 g( 1 );
 CHECK( smallerCount( { 1 , 7 , 2 , 3 , -1 } ) ==
        ( std::vector< size_t >{ 1 , 3 , 1 , 1 , 0 } ) );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const Ints A = distinctInts( g() % 40 , -50 , 120 , g );
  std::vector< size_t > c( A.size() , 0 );
  for( size_t i = 0 ; i < A.size() ; ++i )
   for( size_t j = i + 1 ; j < A.size() ; ++j )
    c[ i ] += ( A[ j ] < A[ i ] );
  const auto r = smallerCount( A );
  CHECK( r == c );
  Ints B( A );
  CHECK( long( std::accumulate( r.begin() , r.end() , size_t( 0 ) ) ) ==
         countInversions( B ) );
  }
 CHECK( smallerCount( {} ).empty() );
 CHECK( smallerCount( { 5 } ) == std::vector< size_t >( 1 , 0 ) );
 Ints S( 500 );
 std::iota( S.begin() , S.end() , 0 );
 CHECK( smallerCount( S ) == std::vector< size_t >( 500 , 0 ) );
 std::reverse( S.begin() , S.end() );
 const auto R = smallerCount( S );
 for( size_t i = 0 ; i < S.size() ; ++i )
  CHECK( R[ i ] == S.size() - 1 - i );
 CHECK( smallerCount( { INT_MAX , 0 , INT_MIN } ) ==
        ( std::vector< size_t >{ 2 , 1 , 0 } ) );
 CHECK_THROWS( smallerCount( { 1 , 1 } ) , contract_violation );
 }

/*--------------------- 2024-08-26, Where's negative? ----------------------*/

// the first negative value of lines and of steps against a linear scan,
// with at most 2 log2 n + 3 calls of f
ADS_TEST( "divide-27" )
{
 for( int64_t c = 2 ; c < 3000 ; ++c )
  for( int64_t s = 1 ; s <= 3 ; ++s ) {
   long calls = 0;
   auto f = [ & ]( int64_t k ) { ++calls; return( c - k / s ); };
   int64_t n = 1;
   while( c - n / s >= 0 )
    ++n;
   calls = 0;
   CHECK( findNegative( f ) == n );
   CHECK( calls <= 2 * long( std::bit_width( std::uint64_t( n ) ) ) + 3 );
   }
 const int64_t big = int64_t( 1 ) << 40;
 CHECK( findNegative( [ & ]( int64_t k ) { return( big - k ); } ) == big + 1 );
 CHECK( findNegative( []( int64_t k ) { return( 3 - 2 * k ); } ) == 2 );
 CHECK( findNegative( []( int64_t k ) { return( 4 - 2 * k ); } ) == 3 );
 CHECK( findNegative( []( int64_t k ) { return( k == 1 ? 1 : -1 ); } ) == 2 );
 CHECK_THROWS( findNegative( []( int64_t k ) { return( -k ); } ) ,
               contract_violation );
 CHECK_THROWS( findNegative( []( int64_t k ) { return( 1 - k ); } ) ,
               contract_violation );
 }

/*------------------------ 2025-01-14, AlternaSort -------------------------*/

// the three alternating orders of the example and of random vectors of
// distinct items, checked on the pairs and as permutations
ADS_TEST( "divide-28" )
{
 std::mt19937 g( 1 );
 auto check = [ & ]( const Ints & A ) {
  for( auto sort : { alternaSort , alternaSortSorted , alternaSortDivide } ) {
   Ints B( A );
   sort( B );
   CHECK( alternating( B ) );
   CHECK( std::is_permutation( A.begin() , A.end() , B.begin() , B.end() ) );
   }
  };
 Ints E{ 1 , 3 , 4 , 6 , 8 , 7 };
 check( E );
 alternaSortSorted( E );
 CHECK( E == ( Ints{ 1 , 8 , 3 , 7 , 4 , 6 } ) );
 CHECK( alternating( { 1 , 8 , 3 , 6 , 4 , 7 } ) );
 CHECK( alternating( { 1 , 7 , 3 , 6 , 4 , 8 } ) );
 CHECK( ! alternating( { 1 , 3 , 4 , 6 , 8 , 7 } ) );
 for( int t = 0 ; t < 3000 ; ++t )
  check( distinctInts( g() % 40 , -50 , 120 , g ) );
 check( {} );
 check( { 4 } );
 check( { 2 , 1 } );
 check( { INT_MAX , 0 , INT_MIN } );
 Ints S( 1000 );
 std::iota( S.begin() , S.end() , 0 );
 check( S );
 std::reverse( S.begin() , S.end() );
 check( S );
 Ints D{ 1 , 1 } , F{ 1 , 1 };
 CHECK_THROWS( alternaSort( D ) , contract_violation );
 CHECK_THROWS( alternaSortSorted( F ) , contract_violation );
 }

/*-------------------- 2025-06-13, Stringhe pareggiate ---------------------*/

// the longest paired run of the examples and of random strings against the
// check of every run with a set
ADS_TEST( "divide-29" )
{
 std::mt19937 g( 1 );
 CHECK( largestPaired( "BaCcAno" ) == 4 );
 CHECK( largestPaired( "aAa" ) == 3 );
 CHECK( largestPaired( "torTA" ) == 0 );
 CHECK( isPaired( "aCcA" , 0 , 3 ) && isPaired( "caCA" , 0 , 3 ) );
 CHECK( largestPaired( "aCcA" ) == 4 && largestPaired( "caCA" ) == 4 );
 CHECK( ! isPaired( "ABa" , 0 , 2 ) && largestPaired( "ABa" ) == 0 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const std::string S = randomString( g() % 13 , "aAbBc" , g );
  size_t best = 0;
  for( size_t i = 0 ; i < S.size() ; ++i )
   for( size_t j = i ; j < S.size() ; ++j )
    if( isPaired( S , i , j ) )
     best = std::max( best , j - i + 1 );
  CHECK( largestPaired( S ) == best );
  }
 CHECK( largestPaired( "" ) == 0 && largestPaired( "z" ) == 0 );
 CHECK( largestPaired( "zZ" ) == 2 && largestPaired( "zzzz" ) == 0 );
 std::string L;
 for( char c = 'a' ; c <= 'z' ; ++c )
  L += std::string( 1 , c ) + swapcase( c );
 CHECK( largestPaired( L ) == 52 );
 CHECK( largestPaired( "abcdefghijklmnopqrstuvwxyz" ) == 0 );
 CHECK_THROWS( largestPaired( "a1" ) , contract_violation );
 }

/*----------------------- 2026-02-02, Due caratteri ------------------------*/

// the longest run of two characters of the examples and of random strings
// by divide and conquer and by the sliding window against the extension of
// each run; the wrong solution of the text fails on abccc
ADS_TEST( "divide-30" )
{
 std::mt19937 g( 1 );
 for( const auto & [ S , r ] : std::vector< std::pair< std::string ,
                                                      size_t > >{
       { "abacccbb" , 5 } , { "abccc" , 4 } } ) {
  CHECK( maxTwo( S ) == r );
  CHECK( maxTwoDivide( S ) == r );
  }
 CHECK( maxTwoWrong( "abccc" ) == 3 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const std::string S = randomString( g() % 17 , "abcd" , g );
  size_t best = 0;
  for( size_t i = 0 ; i < S.size() ; ++i ) {
   std::set< char > c;
   for( size_t j = i ; j < S.size() ; ++j ) {
    c.insert( S[ j ] );
    if( c.size() > 2 )
     break;
    best = std::max( best , j - i + 1 );
    }
   }
  CHECK( maxTwo( S ) == best );
  CHECK( maxTwoDivide( S ) == best );
  }
 for( const std::string & S : { std::string() , std::string( "q" ) ,
                                std::string( 1000 , 'q' ) ,
                                std::string( "ababababab" ) } ) {
  CHECK( maxTwo( S ) == S.size() );
  CHECK( maxTwoDivide( S ) == S.size() );
  }
 CHECK( maxTwo( "abcabcabc" ) == 2 && maxTwoDivide( "abcabcabc" ) == 2 );
 const std::string Z = "abcdefghijklmnopqrstuvwxyz";
 CHECK( maxTwo( Z ) == 2 && maxTwoDivide( Z ) == 2 );
 CHECK( maxTwo( "abbbbc" ) == 5 && maxTwoDivide( "abbbbc" ) == 5 );
 CHECK( maxTwo( "aaab" + std::string( 7 , 'c' ) ) == 8 );
 CHECK( maxTwoDivide( "aaab" + std::string( 7 , 'c' ) ) == 8 );
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
/*------------------------ End File exam_divide.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
