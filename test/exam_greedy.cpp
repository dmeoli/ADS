/*--------------------------------------------------------------------------*/
/*------------------------- File exam_greedy.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the solutions of the exercises of the collections "Esercizi
 * Greedy" and "Esercizi vari", one test per exercise that has something to
 * run: each solution is implemented here (or taken from the library, when
 * it has it) and checked on the examples of the text, against a brute
 * force or an independent algorithm on random small instances, and on the
 * edge cases, at the level 3 of the contracts. Where the solution of the
 * text has an error, the function implements the correct version and its
 * comment says what differs.
 *
 * \see A. Montresor, Esercizi Greedy, Raccolte da compiti scritti
 * \see A. Montresor, Esercizi vari, Raccolte da compiti scritti
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

#include "ADS/Contract.h"
#include "ADS/algorithm/Subsequences.h"
#include "ADS/complexity/EasyCases.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/SubsequenceProblems.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/// the integers of the exercises, 64 bits also where long has 32
using LL = long long;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------ HELPERS -----------------------------------*/
/*--------------------------------------------------------------------------*/

using Vec = std::vector< long long >;
using Matrix = std::vector< std::vector< int > >;

/// n random integers in [ lo , hi ]
Vec randomVec( std::mt19937 & g , std::size_t n , long long lo , long long hi )
{
 std::uniform_int_distribution< long long > U( lo , hi );
 Vec A( n );
 for( auto & x : A ) x = U( g );
 return( A );
 }

/// n distinct random integers in [ lo , hi ]
/// \pre hi - lo + 1 >= n
Vec distinctVec( std::mt19937 & g , std::size_t n , long long lo ,
                 long long hi )
{
 Vec all( std::size_t( hi - lo + 1 ) );
 std::iota( all.begin() , all.end() , lo );
 std::shuffle( all.begin() , all.end() , g );
 all.resize( n );
 return( all );
 }

/// whether P is a (not contiguous) subsequence of T
bool subsequenceOf( const std::string & P , const std::string & T )
{
 std::size_t pos = 0;
 for( char c : P ) {
  pos = T.find( c , pos );
  if( pos == std::string::npos ) return( false );
  ++pos;
  }
 return( true );
 }

/// an r x c random 0/1 matrix with a 1 of probability num / den
Matrix randomBits( std::mt19937 & g , int r , int c , unsigned num ,
                   unsigned den )
{
 Matrix A( std::size_t( r ) , std::vector< int >( std::size_t( c ) , 0 ) );
 for( auto & row : A )
  for( auto & x : row ) x = ( g() % den ) < num ? 1 : 0;
 return( A );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------------- GREEDY -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace greedy {

/*------------------- greedy 2, 2012-02-01, Check pairs --------------------*/

/// whether the n items of A, n even, split into pairs all of one sum;
/// sorted, the least must go with the largest (exchange), and so on
/// inwards, O( n log n )
/// \pre n even
bool checkPairs( Vec A )
{
 ADS_PRE( A.size() % 2 == 0 );
 std::sort( A.begin() , A.end() );
 const std::size_t n = A.size();
 for( std::size_t i = 1 ; i < n / 2 ; ++i )
  if( A[ i ] + A[ n - 1 - i ] != A[ 0 ] + A[ n - 1 ] )
   return( false );
 return( true );
 }

/*---------------------- greedy 3, 2012-05-31, Zigzag ----------------------*/

/// whether X is zig-zag: x_1 < x_2 > x_3 < x_4 ...
bool isZigzag( const Vec & X )
{
 for( std::size_t i = 0 ; i + 1 < X.size() ; ++i )
  if( i % 2 == 0 ? !( X[ i ] < X[ i + 1 ] ) : !( X[ i ] > X[ i + 1 ] ) )
   return( false );
 return( true );
 }

/// the length of a longest zig-zag subsequence of Y: a scan takes the
/// first item of each monotone run, flipping the direction expected at
/// each turn, so that the length is one plus the turns, O( n ) time and
/// O( 1 ) space; the text also adds one when the scan ends inside an
/// increasing run, which counts the last item twice (5 for 3, 8, 1, 5)
std::size_t zigzag( const Vec & Y )
{
 if( Y.empty() ) return( 0 );
 bool up = true;
 std::size_t tot = 1;
 for( std::size_t i = 0 ; i + 1 < Y.size() ; ++i )
  if( up ? Y[ i + 1 ] > Y[ i ] : Y[ i + 1 ] < Y[ i ] ) {
   up = ! up;
   ++tot;
   }
 return( tot );
 }

/*-------- greedy 4 and 5, 2013-05-31 and 2014-09-01, Max segments ---------*/

/// the most requests V[ i ] served exactly from a length L (the pieces S[ i
/// ] cut from a pipe L of exercise 5 are the same problem): the shortest
/// first (exchange with any longer one), O( n log n )
/// \pre the lengths are not negative
std::size_t maxSegments( Vec V , long long L )
{
 ADS_PRE( std::all_of( V.begin() , V.end() ,
                       []( long long x ) { return( x >= 0 ); } ) );
 std::sort( V.begin() , V.end() );
 std::size_t i = 0;
 while( ( i < V.size() ) && ( L >= V[ i ] ) )
  L -= V[ i++ ];
 return( i );
 }

/*------------------ greedy 6, 2015-06-08, Is subsequence ------------------*/

/// whether B^k, each character of B repeated k times, is a subsequence of
/// A: two indices, the j-th character of B^k being B[ j / k ], O( n )
bool isSubsequence( const std::string & A , const std::string & B ,
                    std::size_t k )
{
 const std::size_t n = A.size() , mk = B.size() * k;
 if( mk > n ) return( false );
 std::size_t j = 0;
 for( std::size_t i = 0 ; ( i < n ) && ( j < mk ) ; ++i )
  if( A[ i ] == B[ j / k ] ) ++j;
 return( j == mk );
 }

/// the largest k with B^k a subsequence of A, trying k = 1 , 2 , ...,
/// O( n^2 / m ); the text passes m and n swapped to isSubsequence()
/// \pre B is not empty
std::size_t maxSubsequence( const std::string & A , const std::string & B )
{
 ADS_PRE( ! B.empty() );
 std::size_t k = 1;
 while( isSubsequence( A , B , k ) ) ++k;
 return( k - 1 );
 }

/*--------------------- greedy 7, 2015-07-14, Univoco ----------------------*/

/// an edge [ u , v ] of weight w
struct Edge {
 int u , v;
 long long w;
 };

/// the largest weight of a set of edges no two with an end in common (a
/// matching) of a graph of n nodes: maxWeightMatching() of the library, a
/// backtracking that leaves the first free node alone or matches it to
/// each free neighbour, cutting the branches that cannot beat the best
/// matching found; the text recurs only on the edges of the last node
/// reached and repeats the recursive call outside the test of the free
/// ends, which takes edges on used nodes
/// \pre the ends are distinct nodes and the weights are positive
long long univoco( int n , const std::vector< Edge > & E )
{
 std::vector< std::tuple< int , int , long long > > T;
 for( const Edge & e : E ) {
  ADS_PRE( e.w > 0 );
  T.emplace_back( e.u , e.v , e.w );
  }
 return( maxWeightMatching( n , T ).first );
 }

/*---------------------- greedy 8, 2016-01-07, Cover -----------------------*/

/// a closed interval [ a , b ]
using Interval = std::pair< long long , long long >;

/// the union of the intervals as disjoint closed intervals, by increasing
/// ends, two touching intervals being one
std::vector< Interval > coverage( std::vector< Interval > X )
{
 std::sort( X.begin() , X.end() );
 std::vector< Interval > C;
 for( const auto & I : X )
  if( ! C.empty() && ( I.first <= C.back().second ) )
   C.back().second = std::max( C.back().second , I.second );
  else
   C.push_back( I );
 return( C );
 }

/// a smallest subset of the intervals with the coverage of all, by its
/// places: by increasing start and decreasing end, an interval not inside
/// the last one taken is taken, after dropping the last ones made useless
/// by the one before them and the new one (they touch), O( n log n ); the
/// first version of the text, without the dropping, takes all three of [
/// 2 , 4 ], [ 3 , 6 ], [ 4 , 7 ], as the text itself remarks
/// \pre a <= b for each interval
/// \post the coverage is that of all the intervals (audit)
std::vector< std::size_t > cover( const std::vector< Interval > & X )
{
 ADS_PRE( std::all_of( X.begin() , X.end() , []( const Interval & I ) {
  return( I.first <= I.second ); } ) );
 const std::size_t n = X.size();
 std::vector< std::size_t > by( n );
 std::iota( by.begin() , by.end() , 0 );
 std::sort( by.begin() , by.end() , [ & ]( std::size_t x , std::size_t y ) {
  return( X[ x ].first != X[ y ].first ? X[ x ].first < X[ y ].first
                                       : X[ x ].second > X[ y ].second ); } );
 std::vector< std::size_t > S;
 for( std::size_t i : by ) {
  if( ! S.empty() && ( X[ i ].second <= X[ S.back() ].second ) )
   continue;                                 // inside the last one taken
  while( ( S.size() > 1 ) &&
         ( X[ i ].first <= X[ S[ S.size() - 2 ] ].second ) )
   S.pop_back();
  S.push_back( i );
  }
 ADS_AUDIT( ( [ & ] {
  std::vector< Interval > Y;
  for( std::size_t i : S ) Y.push_back( X[ i ] );
  return( coverage( Y ) == coverage( X ) );
  } )() );
 return( S );
 }

/*----------- greedy 9 and 16, 2017-01-10 and 2023-01-17, Nastro -----------*/

/// the expected cost of reading the files of lengths l and probabilities
/// (or weights) p from a tape in the order A: sum of p times the lengths up
/// to the file
template< class T >
T tapeCost( const std::vector< T > & l , const std::vector< T > & p ,
            const std::vector< std::size_t > & A )
{
 T now = T( 0 ) , C = T( 0 );
 for( std::size_t i : A ) {
  now += l[ i ];
  C += p[ i ] * now;
  }
 return( C );
 }

/// the order of least expected cost: by increasing l_i / p_i, compared as
/// l_a p_b < l_b p_a (swapping two neighbours a, b changes the cost by p_a
/// l_b - p_b l_a), O( n log n ); neither the increasing lengths nor the
/// decreasing probabilities are optimal
/// \pre l > 0 and p >= 0
template< class T >
std::vector< std::size_t > tapeOrder( const std::vector< T > & l ,
                                      const std::vector< T > & p )
{
 ADS_PRE( ( l.size() == p.size() ) &&
          std::all_of( l.begin() , l.end() ,
                       []( const T & x ) { return( x > T( 0 ) ); } ) &&
          std::all_of( p.begin() , p.end() ,
                       []( const T & x ) { return( x >= T( 0 ) ); } ) );
 std::vector< std::size_t > A( l.size() );
 std::iota( A.begin() , A.end() , 0 );
 std::stable_sort( A.begin() , A.end() , [ & ]( std::size_t a ,
                                                std::size_t b ) {
  return( l[ a ] * p[ b ] < l[ b ] * p[ a ] ); } );
 return( A );
 }

/*--------------------- greedy 10, 2018-06-04, Negate ----------------------*/

/// the largest sum of X with exactly k items negated: the k least ones
/// (exchange), O( n log n )
/// \pre 0 <= k <= n
long long negate( Vec X , std::size_t k )
{
 ADS_PRE( k <= X.size() );
 std::sort( X.begin() , X.end() );
 long long tot = 0;
 for( std::size_t i = 0 ; i < X.size() ; ++i )
  tot += i < k ? -X[ i ] : X[ i ];
 return( tot );
 }

/*-------------------- greedy 11, 2019-01-21, Is doable --------------------*/

/// whether each participant j gets a shirt of size P[ j ] or P[ j ] + 1,
/// with T[ i - 1 ] shirts of size i: the requests counted by size, the
/// shirts of size i go to the participants of size i, and those missing
/// are taken from size i + 1, Theta( n ); the loop of the text never
/// increments i
/// \pre T has 6 counts, not negative, and the sizes in P are in [ 1 , 5 ]
bool isDoable( std::vector< long long > T , const std::vector< int > & P )
{
 ADS_PRE( ( T.size() == 6 ) &&
          std::all_of( T.begin() , T.end() ,
                       []( long long x ) { return( x >= 0 ); } ) &&
          std::all_of( P.begin() , P.end() ,
                       []( int s ) { return( ( s >= 1 ) && ( s <= 5 ) ); } ) );
 std::vector< long long > req( 6 , 0 );
 for( int s : P ) ++req[ std::size_t( s - 1 ) ];
 for( std::size_t i = 0 ; i < 5 ; ++i ) {
  T[ i ] -= req[ i ];
  if( T[ i ] < 0 ) {
   T[ i + 1 ] += T[ i ];
   if( T[ i + 1 ] < 0 ) return( false );
   }
  }
 return( true );
 }

/*-------------------- greedy 12, 2020-01-14, Min steps --------------------*/

/// the fewest steps from the first to the last place of A, a step going to
/// a neighbour or to any place of the same value: a BFS on the graph of
/// those moves, O( n^2 ) edges
/// \pre A is not empty
int minStepsQuadratic( const std::vector< int > & A )
{
 ADS_PRE( ! A.empty() );
 const int n = int( A.size() );
 ListGraph<> G( n , false );
 for( int i = 0 ; i + 1 < n ; ++i ) {
  G.insertEdge( i , i + 1 );
  for( int j = i + 2 ; j < n ; ++j )
   if( A[ i ] == A[ j ] ) G.insertEdge( i , j );
  }
 return( distance( G , 0 ).first[ n - 1 ] );
 }

/// the same in Theta( n ): the cells a_i, a node a'_i between a_i and
/// a_{i+1}, and one node per value joined to its cells, so that each move
/// is two edges, and the BFS distance is twice the steps; the text joins
/// the cell a_i to the node A[ i ], a cell, instead of the node of the
/// value A[ i ]
/// \pre A is not empty and its values are in [ 1 , 9 ]
int minStepsLinear( const std::vector< int > & A )
{
 ADS_PRE( ! A.empty() &&
          std::all_of( A.begin() , A.end() ,
                       []( int x ) { return( ( x >= 1 ) && ( x <= 9 ) ); } ) );
 const int n = int( A.size() );
 ListGraph<> G( 2 * n + 8 , false );   // a_i = 2 i, a'_i = 2 i + 1
 for( int i = 0 ; i < n ; ++i )
  G.insertEdge( 2 * i , 2 * n - 2 + A[ i ] );
 for( int i = 0 ; i + 1 < n ; ++i ) {
  G.insertEdge( 2 * i , 2 * i + 1 );
  G.insertEdge( 2 * i + 1 , 2 * i + 2 );
  }
 return( distance( G , 0 ).first[ 2 * n - 2 ] / 2 );
 }

/*--------------- greedy 13, 2020-07-03, Palindromo massimo ----------------*/

/// the digits of A rearranged into the largest palindrome: the digits
/// counted, then placed in pairs from the outside, the largest first, and
/// the one of odd count in the middle, O( n ); the text looks for the
/// middle digit in 1 , ... , 9 only, so a middle 0 is never written
/// \pre digits in [ 0 , 9 ], at most one of odd count
void lpp( std::vector< int > & A )
{
 std::vector< std::size_t > count( 10 , 0 );
 for( int x : A ) {
  ADS_PRE( ( x >= 0 ) && ( x <= 9 ) );
  ++count[ std::size_t( x ) ];
  }
 ADS_PRE( std::count_if( count.begin() , count.end() ,
                         []( std::size_t c ) { return( c % 2 ); } ) <= 1 );
 std::size_t start = 0 , end = A.size();
 for( int d = 9 ; d >= 0 ; --d )
  while( count[ std::size_t( d ) ] > 1 ) {
   A[ start++ ] = A[ --end ] = d;
   count[ std::size_t( d ) ] -= 2;
   }
 if( start < end )
  for( int d = 0 ; d <= 9 ; ++d )
   if( count[ std::size_t( d ) ] == 1 ) A[ start ] = d;
 }

/*----------------- greedy 14, 2021-02-08, Trova l'ordine ------------------*/

/// a permutation of the distinct items of A obeying the n - 1 signs of D
/// ('<' or '>') between neighbours: sorted, each '<' takes the least item
/// left and each '>' the largest (induction), O( n log n ); the text tests
/// D[ i ] instead of D[ k ] and writes [ n ] for S[ n ]
/// \pre D has n - 1 signs, each '<' or '>' (n = 0 allows an empty D)
Vec findOrder( Vec A , const std::string & D )
{
 ADS_PRE( ( D.size() + 1 == A.size() ) || ( A.empty() && D.empty() ) );
 ADS_PRE( std::all_of( D.begin() , D.end() ,
                       []( char c ) { return( ( c == '<' ) ||
                                              ( c == '>' ) ); } ) );
 if( A.empty() ) return( A );
 std::sort( A.begin() , A.end() );
 Vec S( A.size() );
 std::size_t i = 0 , j = A.size() - 1;
 for( std::size_t k = 0 ; k < D.size() ; ++k )
  S[ k ] = D[ k ] == '<' ? A[ i++ ] : A[ j-- ];
 S.back() = A[ i ];
 return( S );
 }

/*----------------- greedy 15, 2022-07-13, Prodotto minimo -----------------*/

/// the least product of a non empty subset of A: the product of all the
/// items not zero, divided by the largest negative one when it is positive;
/// with no negative item, the least item, O( n )
/// \pre A is not empty, and the products fit in 64 bits
std::int64_t minProduct( const std::vector< std::int64_t > & A )
{
 ADS_PRE( ! A.empty() );
 std::int64_t tot = 1;
 auto minNonNeg = std::numeric_limits< std::int64_t >::max();
 auto maxNeg = std::numeric_limits< std::int64_t >::min();
 for( auto x : A )
  if( x < 0 ) {
   maxNeg = std::max( maxNeg , x );
   tot *= x;
   }
  else {
   minNonNeg = std::min( minNonNeg , x );
   if( x > 0 ) tot *= x;
   }
 if( maxNeg == std::numeric_limits< std::int64_t >::min() )
  return( minNonNeg );
 return( tot > 0 ? tot / maxNeg : tot );
 }

/// the same by dynamic programming on the largest and the least product
/// of a subset of the first i items, O( n )
/// \pre A is not empty, and the products fit in 64 bits
std::int64_t minProductDP( const std::vector< std::int64_t > & A )
{
 ADS_PRE( ! A.empty() );
 std::int64_t mx = A[ 0 ] , mn = A[ 0 ];
 for( std::size_t i = 1 ; i < A.size() ; ++i ) {
  const std::int64_t t1 = A[ i ] * mx , t2 = A[ i ] * mn;
  mx = std::max( { mx , A[ i ] , t1 , t2 } );
  mn = std::min( { mn , A[ i ] , t1 , t2 } );
  }
 return( mn );
 }

/*------------------- greedy 17, 2024-06-14, Multi core --------------------*/

/// the makespan of the list scheduling of the tasks len in the order A on k
/// processors, each task to the processor least loaded, by a heap of the
/// loads, O( n log k ); no order (given, increasing or decreasing length)
/// is optimal
/// \pre k >= 1 and the lengths are not negative
long long listScheduling( const Vec & len , int k ,
                          const std::vector< std::size_t > & A )
{
 ADS_PRE( k >= 1 );
 using LL = long long;
 std::priority_queue< LL , std::vector< LL > , std::greater< LL > >
  H( std::greater< LL >() , std::vector< LL >( std::size_t( k ) , 0 ) );
 long long ms = 0;
 for( std::size_t i : A ) {
  ADS_PRE( len[ i ] >= 0 );
  const long long l = H.top() + len[ i ];
  H.pop();
  H.push( l );
  ms = std::max( ms , l );
  }
 return( ms );
 }

/// the places of len by increasing (or decreasing) length
std::vector< std::size_t > byLength( const Vec & len , bool increasing )
{
 std::vector< std::size_t > A( len.size() );
 std::iota( A.begin() , A.end() , 0 );
 std::stable_sort( A.begin() , A.end() , [ & ]( std::size_t x ,
                                                std::size_t y ) {
  return( increasing ? len[ x ] < len[ y ] : len[ x ] > len[ y ] ); } );
 return( A );
 }

/*------------------ greedy 18, 2025-02-03, Giardinaggio -------------------*/

/// the most plants P[ i ] potted in pots V[ j ] >= P[ i ], one per pot: both
/// sorted, the least plant in the least pot that holds it (exchange),
/// O( n log n + m log m )
std::size_t garden( Vec P , Vec V )
{
 std::sort( P.begin() , P.end() );
 std::sort( V.begin() , V.end() );
 std::size_t i = 0 , count = 0;
 for( std::size_t j = 0 ; ( i < P.size() ) && ( j < V.size() ) ; ++j )
  if( P[ i ] <= V[ j ] ) {
   ++count;
   ++i;
   }
 return( count );
 }

}  // end( namespace greedy )

/*--------------------------------------------------------------------------*/
/*-------------------------------- VARIE -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace varie {

/*------------------- varie 1, 2011-07-18, Close to zero -------------------*/

/// the places ( i , j ) of a non empty subvector of V of sum closest to 0,
/// the first one found: the sums of V[ i .. j ] by extending j, O( n^2 );
/// the text compares only the subvectors of two items or more, never the
/// single items M[ i ][ i ]
/// \pre V is not empty
template< class T >
std::pair< std::size_t , std::size_t > closeToZero(
                                                  const std::vector< T > & V )
{
 ADS_PRE( ! V.empty() );
 std::pair< std::size_t , std::size_t > best( 0 , 0 );
 T min = std::abs( V[ 0 ] );
 for( std::size_t i = 0 ; i < V.size() ; ++i ) {
  T sum = T( 0 );
  for( std::size_t j = i ; j < V.size() ; ++j ) {
   sum += V[ j ];
   if( std::abs( sum ) < min ) {
    min = std::abs( sum );
    best = std::pair( i , j );
    }
   }
  }
 return( best );
 }

/*------------------ varie 2, 2012-01-11, Nuts and bolts -------------------*/

/// the places of the least bolt and of the least nut, comparing only a bolt
/// with a nut by tryBN( b , d ) in { -1 , 0 , 1 }: the least bolt and nut
/// so far, each new nut compared with the bolt when the bolt is not larger
/// than the nut, each new bolt with the nut when the nut is not larger,
/// O( n ) comparisons; the loop of the text goes on while both indices are
/// in range ("and"), and so may stop before the least item of the other
/// kind is seen: it must go on while either is ("or")
/// \pre n >= 1 bolts and nuts
template< class Try >
std::pair< std::size_t , std::size_t > findMin( std::size_t n , Try && tryBN )
{
 ADS_PRE( n >= 1 );
 std::size_t mb = 0 , md = 0 , b = 1 , d = 1;
 while( ( d < n ) || ( b < n ) ) {
  if( tryBN( mb , md ) <= 0 ) {          // the bolt is not larger
   if( ( d < n ) && ( tryBN( mb , d ) >= 0 ) ) md = d;
   ++d;
   }
  if( tryBN( mb , md ) >= 0 ) {          // the nut is not larger
   if( ( b < n ) && ( tryBN( b , md ) <= 0 ) ) mb = b;
   ++b;
   }
  }
 return( std::pair( mb , md ) );
 }

/*------------------- varie 3, 2012-09-10, Close points --------------------*/

/// two places i != j of A with | A[ i ] - A[ j ] | <= ( max - min ) / ( n -
/// 1 ) (the sorted gaps sum to max - min): n - 1 buckets of that width d,
/// the first two items in one bucket, O( n ); the bucket of the largest
/// item is n - 1 and must be the last one, n - 2, and d = 0 (all equal)
/// makes the text divide by zero
/// \pre n >= 2
std::pair< std::size_t , std::size_t > closePoints(
                                              const std::vector< double > & A )
{
 ADS_PRE( A.size() >= 2 );
 const std::size_t n = A.size();
 const auto [ lo , hi ] = std::minmax_element( A.begin() , A.end() );
 const double min = *lo , d = ( *hi - min ) / double( n - 1 );
 if( d == 0 ) return( std::pair( std::size_t( 0 ) , std::size_t( 1 ) ) );
 constexpr std::size_t NONE = std::numeric_limits< std::size_t >::max();
 std::vector< std::size_t > B( n - 1 , NONE );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t j = std::min( std::size_t( ( A[ i ] - min ) / d ) ,
                                  n - 2 );
  if( B[ j ] != NONE ) return( std::pair( B[ j ] , i ) );
  B[ j ] = i;
  }
 ADS_POST( false );                      // n items in n - 1 buckets
 return( std::pair( NONE , NONE ) );
 }

/*-------------------- varie 4, 2012-09-10, Max square ---------------------*/

/// DP[ i ][ j ], the side of the largest square of 1s with its bottom right
/// corner at ( i , j ) of the r x c matrix A: 0 on a 0, else one more than
/// the least of the three neighbours above and to the left, Theta( r c );
/// the text returns the largest item of A instead of DP
Matrix maxSquareTable( const Matrix & A )
{
 Matrix DP( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  DP[ i ].assign( A[ i ].size() , 0 );
  for( std::size_t j = 0 ; j < A[ i ].size() ; ++j )
   if( A[ i ][ j ] )
    DP[ i ][ j ] = ( i == 0 ) || ( j == 0 ) ? 1 :
                   1 + std::min( { DP[ i - 1 ][ j ] , DP[ i - 1 ][ j - 1 ] ,
                                   DP[ i ][ j - 1 ] } );
  }
 return( DP );
 }

/// the side of the largest square of 1s of the 0/1 matrix A, Theta( r c ),
/// largestSquare() of the library
int maxSquare( const Matrix & A )
{
 return( int( largestSquare( A ).side ) );
 }

/*---------------------- varie 5, 2014-01-07, Mistery ----------------------*/

/// the procedure mistery() of the text: n times ceil( log2 n ) times
/// floor( log3 n ) + 1 increments, Theta( n log^2 n )
std::uint64_t mistery( std::uint64_t n )
{
 std::uint64_t s = 0;
 for( std::uint64_t i = 1 ; i <= n ; ++i )
  for( std::uint64_t j = 1 ; j < n ; j *= 2 )
   for( std::uint64_t k = 1 ; k <= n ; k *= 3 ) ++s;
 return( s );
 }

/*------------------- varie 6, 2014-06-05, Segmentcover --------------------*/

/// the places of disjoint segments [ a_i , b_i ) of largest total length,
/// and that length: the weighted intervals of weights b_i - a_i,
/// Theta( n log n )
/// \pre a_i < b_i, both fitting in a long (the ends of WeightedInterval)
std::pair< std::vector< std::size_t > , LL > segmentCover( const Vec & a ,
                                                          const Vec & b )
{
 ADS_PRE( a.size() == b.size() );
 std::vector< WeightedInterval< long long > > I;
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  I.push_back( { long( a[ i ] ) , long( b[ i ] ) , b[ i ] - a[ i ] } );
 return( maxinterval( I ) );
 }

/*--------------------- varie 7, 2014-09-01, Min moves ---------------------*/

/// the fewest moves from the first to the last place of V, a move from i
/// going forward by 1 to V[ i ] places: S[ i ] = 1 + min S[ i + k ] from
/// the end, O( n^2 ); the text does not return S[ 1 ]
/// \pre V is not empty and positive
int minMoves( const Vec & V )
{
 ADS_PRE( ! V.empty() &&
          std::all_of( V.begin() , V.end() ,
                       []( long long x ) { return( x > 0 ); } ) );
 const std::size_t n = V.size();
 std::vector< int > S( n , 0 );
 for( std::size_t i = n - 1 ; i-- > 0 ; ) {
  S[ i ] = std::numeric_limits< int >::max();
  for( std::size_t k = 1 ; ( k <= std::size_t( V[ i ] ) ) && ( i + k < n ) ;
       ++k )
   S[ i ] = std::min( S[ i ] , S[ i + k ] + 1 );
  }
 return( S[ 0 ] );
 }

/*-------------------- varie 8, 2017-01-10, Zeri e uni ---------------------*/

/// the length of the longest subvector of V made of k 0s and then k 1s: the
/// 0s and the 1s of the current run counted, Theta( n )
/// \pre V is made of 0 and 1
std::size_t count01( const std::vector< int > & V )
{
 std::size_t c0 = 0 , c1 = 0 , largest = 0;
 for( int x : V ) {
  ADS_PRE( ( x == 0 ) || ( x == 1 ) );
  if( x == 0 ) {
   if( c1 > 0 ) c0 = c1 = 0;
   ++c0;
   }
  else {
   ++c1;
   largest = std::max( largest , 2 * std::min( c0 , c1 ) );
   }
  }
 return( largest );
 }

/// the 0/1 vector of a string of '0' and '1'
std::vector< int > bitsOf( const std::string & s )
{
 std::vector< int > V;
 for( char c : s ) V.push_back( c == '1' ? 1 : 0 );
 return( V );
 }

/*--------------------- varie 9, 2019-01-21, Partition ---------------------*/

/// the least completion time of the houses H split among at most k
/// painters, each one a contiguous block: DP[ t ][ i ], the best for the
/// first i houses and t painters, is the least over j of the largest of
/// DP[ t - 1 ][ j ] and H[ j + 1 .. i ], O( k n^2 ) with the prefix sums;
/// O( n log sum H ) by the binary search of painterPartition() of the
/// library
/// \pre k >= 1 and the times are not negative
long long partition( const Vec & H , std::size_t k )
{
 return( painterPartition( H , k ) );
 }

/*---------------- varie 10, 2019-02-07, Senza consecutivi -----------------*/

/// the number of strings of n bits with no two 1s next to each other:
/// DP[ n ] = DP[ n - 1 ] + DP[ n - 2 ], DP[ 0 ] = 1, DP[ 1 ] = 2, O( n )
/// \pre 0 <= n <= 91 (64 bits)
std::uint64_t bits( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 91 ) );
 std::uint64_t a = 1 , b = 2;                 // DP[ i ], DP[ i + 1 ]
 for( int i = 0 ; i < n ; ++i ) {
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 return( a );
 }

/*----------------- varie 11, 2019-07-18, Somma di quattro -----------------*/

/// whether four items of A (repetitions allowed) sum to k: sorted, for
/// each pair i1 <= i2 two indices from the ends close in on the other two,
/// O( n^3 ) time and O( 1 ) more space
/// \pre | A[ i ] | < 2^60 and | k | < 2^62
bool fourCubic( std::vector< std::int64_t > A , std::int64_t k )
{
 std::sort( A.begin() , A.end() );
 const std::size_t n = A.size();
 for( std::size_t i1 = 0 ; i1 < n ; ++i1 )
  for( std::size_t i2 = i1 ; i2 < n ; ++i2 ) {
   std::size_t i3 = 0 , i4 = n - 1;
   while( i3 <= i4 ) {
    const std::int64_t s = A[ i1 ] + A[ i2 ] + A[ i3 ] + A[ i4 ];
    if( s == k ) return( true );
    if( s < k ) ++i3;
    else if( i4-- == 0 ) break;
    }
   }
 return( false );
 }

/// the same with the n ( n + 1 ) / 2 sums of two items sorted and a binary
/// search of k - B[ i ] for each one, O( n^2 log n ) time and O( n^2 )
/// space; the last version of the text never returns false
/// \pre | A[ i ] | < 2^60 and | k | < 2^62
bool fourPairs( const std::vector< std::int64_t > & A ,
                std::int64_t k )
{
 std::vector< std::int64_t > B;
 for( std::size_t i1 = 0 ; i1 < A.size() ; ++i1 )
  for( std::size_t i2 = i1 ; i2 < A.size() ; ++i2 )
   B.push_back( A[ i1 ] + A[ i2 ] );
 std::sort( B.begin() , B.end() );
 for( auto x : B )
  if( std::binary_search( B.begin() , B.end() , k - x ) ) return( true );
 return( false );
 }

/*------------------ varie 12, 2020-02-04, Bordi quadrati ------------------*/

/// the side of the largest square of the n x n 0/1 matrix A with only 1s on
/// its border: R[ i ][ j ] and C[ i ][ j ], the 1s ending at ( i , j ) in
/// its row and in its column, then each corner ( i , j ) tries the sides
/// larger than the best so far, O( n^3 ); the text never fills the first
/// row of R and the first column of C
/// \pre A is square
int largestSquare( const Matrix & A )
{
 const std::size_t n = A.size();
 ADS_PRE( std::all_of( A.begin() , A.end() , [ n ]( const auto & row ) {
  return( row.size() == n ); } ) );
 Matrix R( n , std::vector< int >( n , 0 ) ) , C( R );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j )
   if( A[ i ][ j ] ) {
    R[ i ][ j ] = j ? R[ i ][ j - 1 ] + 1 : 1;
    C[ i ][ j ] = i ? C[ i - 1 ][ j ] + 1 : 1;
    }
 int best = 0;
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j ) {
   const int m = std::min( R[ i ][ j ] , C[ i ][ j ] );
   for( int k = best + 1 ; k <= m ; ++k )
    if( ( R[ i - std::size_t( k ) + 1 ][ j ] >= k ) &&
        ( C[ i ][ j - std::size_t( k ) + 1 ] >= k ) )
     best = k;
   }
 return( best );
 }

/*---------------------- varie 13, 2020-02-04, Maxsum ----------------------*/

/// the largest number of 0s minus number of 1s of a subvector of A, the
/// empty one included: the largest sum of a subvector of 1 for 0 and -1
/// for 1, Theta( n )
/// \pre A is made of 0 and 1
long long zeroUnbalance( const std::vector< int > & A )
{
 Vec B;
 for( int x : A ) {
  ADS_PRE( ( x == 0 ) || ( x == 1 ) );
  B.push_back( x == 0 ? 1 : -1 );
  }
 return( maxSubarraySum( B ) );
 }

/*---------------------- varie 14, 2020-06-12, Paths -----------------------*/

/// the number of paths from the top left to the bottom right corner of an
/// n x n grid by steps right and down: DP[ i ][ j ] = DP[ i - 1 ][ j ] +
/// DP[ i ][ j - 1 ], 1 on the borders, Theta( n^2 ) sums
/// \pre 0 <= n <= 33 (64 bits)
std::uint64_t paths( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 33 ) );
 std::vector< std::uint64_t > DP( std::size_t( n ) + 1 , 1 );
 for( int i = 1 ; i <= n ; ++i )
  for( std::size_t j = 1 ; j <= std::size_t( n ) ; ++j )
   DP[ j ] += DP[ j - 1 ];
 return( DP[ std::size_t( n ) ] );
 }

/*-------------------- varie 15, 2020-06-12, Scacchiera --------------------*/

/// the largest sum of items of D no two next to each other (Hateville):
/// DP[ i ] = max( DP[ i - 1 ] , DP[ i - 2 ] + D[ i ] ), O( n ),
/// maxNonAdjacentSum() of the library
long long hateville( const Vec & D )
{
 return( maxNonAdjacentSum( D ) );
 }

/// the largest gain of cells of the 2 x n board A no two adjacent, also
/// diagonally: at most one cell per column, the larger, and no two
/// neighbouring columns, so Hateville on the largest of each column, O( n )
/// \pre A has two rows of n positive gains
long long maxGain( const std::vector< Vec > & A )
{
 ADS_PRE( ( A.size() == 2 ) && ( A[ 0 ].size() == A[ 1 ].size() ) );
 Vec D( A[ 0 ].size() );
 for( std::size_t i = 0 ; i < D.size() ; ++i )
  D[ i ] = std::max( A[ 0 ][ i ] , A[ 1 ][ i ] );
 return( hateville( D ) );
 }

/*--------------------- varie 16, 2020-07-24, Matrice ----------------------*/

// the length of a longest common subsequence of X, Y, Z: DP[ i ][ j ][ k ]
// one more than DP[ i - 1 ][ j - 1 ][ k - 1 ] when the three last
// characters are equal, else the largest of the three dropping one, the
// table of lcs3() of the library [see SubsequenceProblems.h]

/*----------------- varie 17, 2021-06-16, Trova la coppia ------------------*/

/// whether a1 + a2 = k for some a1 of A1 and a2 of A2, both sorted: one
/// index from the start of A1, one from the end of A2, O( n ); the text
/// tests sumSoFar, never assigned, for sum
/// \pre A1 and A2 sorted
bool findPair( const Vec & A1 , const Vec & A2 , long long k )
{
 ADS_PRE( std::is_sorted( A1.begin() , A1.end() ) &&
          std::is_sorted( A2.begin() , A2.end() ) );
 std::size_t i1 = 0 , i2 = A2.size();
 while( ( i1 < A1.size() ) && ( i2 > 0 ) ) {
  const long long sum = A1[ i1 ] + A2[ i2 - 1 ];
  if( sum == k ) return( true );
  if( sum > k ) --i2;
  else ++i1;
  }
 return( false );
 }

/*----------------- varie 18, 2021-07-26, Punto d'incontro -----------------*/

/// whether a node is reached from s and from t at one same least cost: the
/// shortest paths from s and from t, by Bellman-Ford-Moore since the costs
/// are real, and a scan of the nodes, O( n m )
/// \pre s and t are nodes, and no negative cycle is reached from them
template< class G >
bool meetingPoint( const G & g , int s , int t )
{
 const auto ds = bellmanFordMoore( g , s ) , dt = bellmanFordMoore( g , t );
 ADS_PRE( ! ds.negativeCycle && ! dt.negativeCycle );
 const auto INF = infinity< typename G::Weight >();
 for( int u : g.V() )
  if( ( ds.d[ u ] == dt.d[ u ] ) && ( ds.d[ u ] != INF ) ) return( true );
 return( false );
 }

/*---------------- varie 19, 2025-01-14, Serie aritmetiche -----------------*/

/// the number of the largest arithmetic subvectors (three items or more,
/// one difference) of A: a run of equal differences counts when it reaches
/// two, O( n ); two such subvectors may share an end
/// \pre n >= 3
std::size_t countArithmetic( const Vec & A )
{
 ADS_PRE( A.size() >= 3 );
 std::size_t series = 0 , count = 0;
 long long last = 0;
 for( std::size_t i = 1 ; i < A.size() ; ++i ) {
  const long long cur = A[ i ] - A[ i - 1 ];
  if( ( i == 1 ) || ( cur != last ) ) {
   last = cur;
   count = 1;
   }
  else if( ++count == 2 )
   ++series;
  }
 return( series );
 }

/*----------------- varie 20, 2025-02-03, Circular MaxSum ------------------*/

/// the largest sum of a subvector of the circular vector A, the empty one
/// included: the largest of the linear one (Kadane) and of the total less
/// the least sum of a subvector, Theta( n )
/// \post the result is not negative
long long circularMaxSum( const Vec & A )
{
 long long maxSoFar = 0 , maxHere = 0 , minSoFar = 0 , minHere = 0 , tot = 0;
 for( long long x : A ) {
  minHere = std::min( minHere + x , 0LL );
  minSoFar = std::min( minSoFar , minHere );
  maxHere = std::max( maxHere + x , 0LL );
  maxSoFar = std::max( maxSoFar , maxHere );
  tot += x;
  }
 const long long r = std::max( maxSoFar , tot - minSoFar );
 ADS_POST( r >= 0 );
 return( r );
 }

/*----------------- varie 21, 2026-07-20, Massima distanza -----------------*/

/// the largest | i - j | with A[ i ] = A[ j ], 0 if the items are distinct:
/// the first place of each value in a hash table, O( n ) expected time and
/// O( k ) space for k distinct values
std::size_t maxDistance( const Vec & A )
{
 std::unordered_map< long long , std::size_t > first;
 std::size_t best = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  const auto [ it , fresh ] = first.try_emplace( A[ i ] , i );
  if( ! fresh ) best = std::max( best , i - it->second );
  }
 return( best );
 }

}  // end( namespace varie )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/*--------------------- tests of the greedy exercises ----------------------*/

// the example of the text, the brute force on every pairing of up to 10
// distinct items (many of them pairable by construction), the edge cases
ADS_TEST( "greedy-2" )
{
 CHECK( greedy::checkPairs( { 7 , 4 , 5 , 2 , 3 , 6 } ) );
 // every pairing of the items of B in pairs of sum s
 std::function< bool( const Vec & , long long ) > pairs =
  [ & ]( const Vec & B , long long s ) -> bool {
  if( B.empty() ) return( true );
  for( std::size_t q = 1 ; q < B.size() ; ++q )
   if( B[ 0 ] + B[ q ] == s ) {
    Vec C;
    for( std::size_t i = 1 ; i < B.size() ; ++i )
     if( i != q ) C.push_back( B[ i ] );
    if( pairs( C , s ) ) return( true );
    }
  return( false );
  };
 auto brute = [ & ]( const Vec & A ) {
  for( std::size_t j = 1 ; j < A.size() ; ++j )
   if( pairs( A , A[ 0 ] + A[ j ] ) ) return( true );
  return( A.empty() );
  };
 std::mt19937 g( 1 );
 int yes = 0;
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::size_t n = 2 * ( 1 + g() % 5 );
  Vec A;
  if( t % 2 ) {                          // pairable: x and s - x
   const long long s = LL( g() % 21 );
   Vec half = distinctVec( g , n / 2 , -10 , -1 );
   for( long long x : half ) { A.push_back( x ); A.push_back( s - x ); }
   std::shuffle( A.begin() , A.end() , g );
   }
  else
   A = distinctVec( g , n , -8 , 8 );
  const bool r = greedy::checkPairs( A );
  CHECK( r == brute( A ) );
  yes += r;
  }
 CHECK( yes >= 300 );
 CHECK( greedy::checkPairs( Vec() ) );
 CHECK( greedy::checkPairs( { 5 , -5 } ) );
 const long long M = std::numeric_limits< long long >::max() / 2;
 CHECK( greedy::checkPairs( { M , -M , M - 1 , 1 - M } ) );
 CHECK( ! greedy::checkPairs( { 1 , 2 , 3 , 5 } ) );
 CHECK( greedy::checkPairs( Vec( 8 , -3 ) ) );           // all equal
 CHECK( greedy::checkPairs( { 2 , 2 , 0 , 4 } ) );       // duplicates
 CHECK( ! greedy::checkPairs( { 2 , 2 , 2 , 3 } ) );
 CHECK( greedy::checkPairs( { 0 , 0 } ) );
 CHECK_THROWS( greedy::checkPairs( { 1 , 2 , 3 } ) , contract_violation );
 CHECK_THROWS( greedy::checkPairs( { 1 } ) , contract_violation );
 }

// the examples of the text, the O( n^2 ) dynamic programming on 800 random
// vectors with repeated items, the edge cases and the case where the
// text's final increment counts the last item twice
ADS_TEST( "greedy-3" )
{
 CHECK( greedy::isZigzag( { 3 , 8 , 1 , 5 , 2 } ) );
 CHECK( ! greedy::isZigzag( { 3 , 8 , 10 , 5 , 2 } ) );
 CHECK( greedy::isZigzag( { 3 , 8 , 5 , 6 , 2 } ) );
 CHECK( greedy::isZigzag( { 4 , 8 , 5 , 6 , 2 } ) );
 CHECK( greedy::zigzag( { 3 , 4 , 8 , 5 , 6 , 2 } ) == 5 );
 auto dp = []( const Vec & Y ) {
  // L[ i ][ 0 ] (L[ i ][ 1 ]) the longest ending at i whose next step goes
  // up (down), 0 if none
  std::vector< std::array< std::size_t , 2 > > L( Y.size() , { 1 , 0 } );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < Y.size() ; ++i ) {
   for( std::size_t j = 0 ; j < i ; ++j ) {
    if( ( Y[ j ] < Y[ i ] ) && L[ j ][ 0 ] )
     L[ i ][ 1 ] = std::max( L[ i ][ 1 ] , L[ j ][ 0 ] + 1 );
    if( ( Y[ j ] > Y[ i ] ) && L[ j ][ 1 ] )
     L[ i ][ 0 ] = std::max( L[ i ][ 0 ] , L[ j ][ 1 ] + 1 );
    }
   best = std::max( { best , L[ i ][ 0 ] , L[ i ][ 1 ] } );
   }
  return( best );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 800 ; ++t ) {
  const Vec Y = randomVec( g , g() % 14 , 0 , 1 + LL( g() % 6 ) );
  CHECK( greedy::zigzag( Y ) == dp( Y ) );
  }
 CHECK( greedy::zigzag( Vec() ) == 0 );
 CHECK( greedy::zigzag( { 7 } ) == 1 );
 CHECK( greedy::zigzag( Vec( 9 , 4 ) ) == 1 );
 CHECK( greedy::zigzag( { 1 , 2 } ) == 2 );
 CHECK( greedy::zigzag( { 3 , 8 , 1 , 5 } ) == 4 );
 CHECK( greedy::zigzag( { 5 , 4 , 3 , 2 , 1 } ) == 1 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( greedy::zigzag( { -M , M , -M , M } ) == 4 );
 CHECK( greedy::isZigzag( Vec() ) && greedy::isZigzag( { 7 } ) );
 CHECK( greedy::isZigzag( { 1 , 2 } ) && ! greedy::isZigzag( { 2 , 1 } ) );
 CHECK( ! greedy::isZigzag( { 4 , 4 } ) );
 CHECK( greedy::zigzag( { 2 , 1 } ) == 1 );         // the first step goes up
 CHECK( greedy::zigzag( { 1 , 1 , 2 , 2 , 1 , 1 } ) == 3 );
 Vec Z( 10000 );                                     // 0 , 1 , 0 , 1 , ...
 for( std::size_t i = 0 ; i < Z.size() ; ++i ) Z[ i ] = LL( i % 2 );
 CHECK( greedy::zigzag( Z ) == Z.size() );
 CHECK( greedy::zigzag( Vec( Z.begin() + 1 , Z.end() ) ) == Z.size() - 2 );
 }

// the brute force on every subset of up to 12 requests (the pieces of
// exercise 5 alike), the cases where all or none fit, a budget of 0
ADS_TEST( "greedy-4" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 13;
  const Vec V = randomVec( g , n , 0 , 20 );
  const long long L = LL( g() % 60 );
  std::size_t best = 0;
  for( std::uint32_t s = 0 ; s < ( 1u << n ) ; ++s ) {
   long long sum = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( s >> i & 1 ) sum += V[ i ];
   if( sum <= L )
    best = std::max( best , std::size_t( std::popcount( s ) ) );
   }
  CHECK( greedy::maxSegments( V , L ) == best );
  }
 CHECK( greedy::maxSegments( Vec() , 10 ) == 0 );
 CHECK( greedy::maxSegments( { 11 , 12 } , 10 ) == 0 );
 CHECK( greedy::maxSegments( { 3 , 3 , 3 } , 9 ) == 3 );
 CHECK( greedy::maxSegments( { 3 , 3 , 3 } , 8 ) == 2 );
 CHECK( greedy::maxSegments( { 10 } , 10 ) == 1 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( greedy::maxSegments( { M , 1 } , M ) == 1 );
 CHECK( greedy::maxSegments( { 0 , 0 , 5 } , 0 ) == 2 );    // a budget of 0
 CHECK( greedy::maxSegments( { 4 } , 0 ) == 0 );
 CHECK( greedy::maxSegments( Vec( 1000 , 1 ) , 999 ) == 999 );
 CHECK_THROWS( greedy::maxSegments( { -1 } , 1 ) , contract_violation );
 CHECK_THROWS( greedy::maxSegments( { 3 , 1 , -2 } , 9 ) ,
               contract_violation );
 }

// the example of the text (B^4 would need 8 a's, there are 6), B^k built
// and tested by a search of each character on 600 random strings of two or
// three letters, the edge cases
ADS_TEST( "greedy-6" )
{
 const std::string A = "aabaabaabbbcb";
 CHECK( greedy::isSubsequence( A , "aab" , 3 ) );
 CHECK( subsequenceOf( "aaaaaabbb" , A ) );
 CHECK( ! greedy::isSubsequence( A , "aab" , 4 ) );
 CHECK( greedy::maxSubsequence( A , "aab" ) == 3 );
 auto power = []( const std::string & B , std::size_t k ) {
  std::string P;
  for( char c : B ) P += std::string( k , c );
  return( P );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const char top = char( 'a' + 1 + g() % 2 );
  std::string S , B;
  for( std::size_t i = g() % 25 ; i > 0 ; --i )
   S += char( 'a' + g() % unsigned( top - 'a' + 1 ) );
  for( std::size_t i = 1 + g() % 3 ; i > 0 ; --i )
   B += char( 'a' + g() % unsigned( top - 'a' + 1 ) );
  std::size_t best = 0;
  for( std::size_t k = 1 ; k * B.size() <= S.size() ; ++k ) {
   const bool in = subsequenceOf( power( B , k ) , S );
   CHECK( greedy::isSubsequence( S , B , k ) == in );
   if( in ) best = k;
   }
  CHECK( greedy::maxSubsequence( S , B ) == best );
  }
 CHECK( greedy::maxSubsequence( "" , "a" ) == 0 );
 CHECK( greedy::maxSubsequence( "b" , "a" ) == 0 );
 CHECK( greedy::maxSubsequence( "a" , "a" ) == 1 );
 CHECK( greedy::maxSubsequence( std::string( 50 , 'z' ) , "z" ) == 50 );
 CHECK( greedy::maxSubsequence( std::string( 50 , 'z' ) , "zz" ) == 25 );
 CHECK( greedy::isSubsequence( "abc" , "" , 7 ) );
 CHECK( greedy::isSubsequence( "abc" , "xyz" , 0 ) );    // B^0 is empty
 CHECK( greedy::isSubsequence( "" , "" , 1 ) );
 CHECK( ! greedy::isSubsequence( "ab" , "abc" , 1 ) );   // B longer than A
 CHECK( greedy::isSubsequence( "abc" , "abc" , 1 ) );    // B equal to A
 CHECK( greedy::maxSubsequence( "abc" , "abc" ) == 1 );
 CHECK( greedy::maxSubsequence( "ba" , "ab" ) == 0 );
 CHECK( greedy::maxSubsequence( "abcdefghijklmnopqrstuvwxyz" ,
                                "acegikmoqsuwy" ) == 1 );
 CHECK_THROWS( greedy::maxSubsequence( "a" , "" ) , contract_violation );
 }

// the path of the text (2 + 2 = 4 against 3 of the greedy by weight), a
// dynamic programming on the subsets of the nodes on 400 random graphs of
// up to 7 nodes, the edge cases
ADS_TEST( "greedy-7-b" )
{
 using greedy::Edge;
 CHECK( greedy::univoco( 4 , { { 0 , 1 , 2 } , { 1 , 2 , 3 } ,
                               { 2 , 3 , 2 } } ) == 4 );
 auto dp = []( int n , const std::vector< Edge > & E ) {
  std::vector< std::vector< long long > > W( std::size_t( n ) ,
   std::vector< long long >( std::size_t( n ) , 0 ) );
  for( const auto & e : E ) {
   W[ e.u ][ e.v ] = std::max( W[ e.u ][ e.v ] , e.w );
   W[ e.v ][ e.u ] = W[ e.u ][ e.v ];
   }
  // F[ S ], the best on the nodes of S: its least node left alone or
  // matched to another one of S
  std::vector< long long > F( std::size_t( 1 ) << n , 0 );
  for( std::size_t S = 1 ; S < F.size() ; ++S ) {
   const int u = std::countr_zero( S );
   const std::size_t R = S & ( S - 1 );
   F[ S ] = F[ R ];
   for( int v = u + 1 ; v < n ; ++v )
    if( ( R >> v & 1 ) && W[ u ][ v ] )
     F[ S ] = std::max( F[ S ] , W[ u ][ v ] + F[ R & ~( std::size_t( 1 )
                                                          << v ) ] );
   }
  return( F.back() );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = 1 + int( g() % 7 );
  std::vector< Edge > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 2 ) E.push_back( { u , v , 1 + LL( g() % 9 ) } );
  CHECK( greedy::univoco( n , E ) == dp( n , E ) );
  }
 CHECK( greedy::univoco( 0 , {} ) == 0 );
 CHECK( greedy::univoco( 2 , { { 0 , 1 , 5 } } ) == 5 );
 CHECK( greedy::univoco( 3 , { { 0 , 1 , 5 } , { 1 , 2 , 5 } ,
                               { 0 , 2 , 5 } } ) == 5 );
 CHECK( greedy::univoco( 1 , {} ) == 0 );
 CHECK( greedy::univoco( 5 , {} ) == 0 );                // no edges
 CHECK( greedy::univoco( 4 , { { 0 , 1 , 3 } , { 2 , 3 , 4 } } ) == 7 );
 CHECK( greedy::univoco( 4 , { { 0 , 1 , 1 } , { 0 , 2 , 1 } ,
                               { 0 , 3 , 9 } } ) == 9 ); // a star
 CHECK_THROWS( greedy::univoco( 2 , { { 0 , 0 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( greedy::univoco( 2 , { { 0 , 1 , 0 } } ) ,
               contract_violation );
 CHECK_THROWS( greedy::univoco( 2 , { { 0 , 1 , -4 } } ) ,
               contract_violation );
 }

// the examples of the text, the smallest subset with the same coverage by
// brute force on 500 random sets of up to 10 intervals, the edge cases
ADS_TEST( "greedy-8" )
{
 using greedy::Interval;
 auto take = []( const std::vector< Interval > & X ,
                 const std::vector< std::size_t > & S ) {
  std::vector< Interval > Y;
  for( std::size_t i : S ) Y.push_back( X[ i ] );
  std::sort( Y.begin() , Y.end() );
  return( Y );
  };
 const std::vector< Interval > X1 = { { 2 , 4 } , { 4 , 7 } , { 8 , 10 } ,
                                      { 3 , 6 } };
 CHECK( take( X1 , greedy::cover( X1 ) ) ==
        std::vector< Interval >( { { 2 , 4 } , { 4 , 7 } , { 8 , 10 } } ) );
 const std::vector< Interval > X2 = { { 2 , 4 } , { 3 , 6 } , { 4 , 7 } };
 CHECK( take( X2 , greedy::cover( X2 ) ) ==
        std::vector< Interval >( { { 2 , 4 } , { 4 , 7 } } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = 1 + g() % 10;
  std::vector< Interval > X;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const long long a = LL( g() % 20 );
   X.emplace_back( a , a + LL( g() % 6 ) );
   }
  const auto S = greedy::cover( X );
  const auto C = greedy::coverage( X );
  CHECK( greedy::coverage( take( X , S ) ) == C );
  std::size_t best = n;
  for( std::uint32_t s = 1 ; s < ( 1u << n ) ; ++s ) {
   if( std::size_t( std::popcount( s ) ) >= best ) continue;
   std::vector< Interval > Y;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( s >> i & 1 ) Y.push_back( X[ i ] );
   if( greedy::coverage( Y ) == C ) best = std::size_t( std::popcount( s ) );
   }
  CHECK( S.size() == best );
  }
 CHECK( greedy::cover( {} ).empty() );
 CHECK( greedy::cover( { { 1 , 1 } } ).size() == 1 );
 CHECK( greedy::cover( std::vector< Interval >( 5 , { 0 , 3 } ) ).size() ==
        1 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( greedy::cover( { { -M , 0 } , { 0 , M } , { -1 , 1 } } ).size() ==
        2 );
 CHECK( greedy::cover( { { 0 , 1 } , { 5 , 6 } , { 9 , 9 } } ).size() ==
        3 );                                             // disjoint
 CHECK( take( { { 0 , 9 } , { 2 , 3 } , { 4 , 9 } } ,
              greedy::cover( { { 0 , 9 } , { 2 , 3 } , { 4 , 9 } } ) ) ==
        std::vector< Interval >( { { 0 , 9 } } ) );      // nested
 CHECK( greedy::cover( { { 0 , 2 } , { 2 , 4 } , { 4 , 6 } } ).size() ==
        3 );                                             // touching chain
 CHECK( greedy::cover( { { 3 , 3 } , { 3 , 3 } , { 3 , 3 } } ).size() == 1 );
 CHECK_THROWS( greedy::cover( { { 2 , 1 } } ) , contract_violation );
 CHECK_THROWS( greedy::cover( { { 0 , 5 } , { 7 , 6 } } ) ,
               contract_violation );
 }

// the counterexamples of both exercises with their costs, the order by
// length over probability against every permutation of up to 7 files of
// integer weights, the order of Shortest Job First with equal
// probabilities, the edge cases
ADS_TEST( "greedy-9" )
{
 using D = std::vector< double >;
 auto near = []( double x , double y ) { return( std::abs( x - y ) < 1e-9 ); };
 const std::vector< std::size_t > I01 = { 0 , 1 } , I10 = { 1 , 0 };
 // exercise 9: increasing length, then decreasing probability
 CHECK( near( greedy::tapeCost( D{ 1 , 2 } , D{ 0.1 , 0.9 } , I01 ) , 2.8 ) );
 CHECK( near( greedy::tapeCost( D{ 1 , 2 } , D{ 0.1 , 0.9 } , I10 ) , 2.1 ) );
 CHECK( near( greedy::tapeCost( D{ 10 , 1 } , D{ 0.6 , 0.4 } , I01 ) ,
              10.4 ) );
 CHECK( near( greedy::tapeCost( D{ 10 , 1 } , D{ 0.6 , 0.4 } , I10 ) , 7.0 ) );
 CHECK( greedy::tapeOrder( D{ 1 , 2 } , D{ 0.1 , 0.9 } ) == I10 );
 CHECK( greedy::tapeOrder( D{ 10 , 1 } , D{ 0.6 , 0.4 } ) == I10 );
 // exercise 16: the figure, then decreasing probability and increasing
 // length
 CHECK( near( greedy::tapeCost( D( 4 , 100 ) , D( 4 , 0.25 ) ,
                                { 0 , 1 , 2 , 3 } ) , 250 ) );
 CHECK( near( greedy::tapeCost( D{ 200 , 100 } , D{ 0.6 , 0.4 } , I01 ) ,
              240 ) );
 CHECK( near( greedy::tapeCost( D{ 200 , 100 } , D{ 0.6 , 0.4 } , I10 ) ,
              220 ) );
 CHECK( near( greedy::tapeCost( D{ 100 , 200 } , D{ 0.1 , 0.9 } , I01 ) ,
              280 ) );
 CHECK( near( greedy::tapeCost( D{ 100 , 200 } , D{ 0.1 , 0.9 } , I10 ) ,
              210 ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::size_t n = g() % 8;
  const Vec l = randomVec( g , n , 1 , 9 ) , p = randomVec( g , n , 0 , 9 );
  std::vector< std::size_t > A( n );
  std::iota( A.begin() , A.end() , 0 );
  long long best = std::numeric_limits< long long >::max();
  do best = std::min( best , greedy::tapeCost( l , p , A ) );
  while( std::next_permutation( A.begin() , A.end() ) );
  CHECK( greedy::tapeCost( l , p , greedy::tapeOrder( l , p ) ) == best );
  const Vec q( n , 3 );
  CHECK( greedy::tapeCost( l , q , greedy::tapeOrder( l , q ) ) ==
         3 * totalCompletion( l , shortestJobFirst( l ) ) );
  }
 CHECK( greedy::tapeOrder( Vec() , Vec() ).empty() );
 CHECK( greedy::tapeOrder( Vec{ 5 } , Vec{ 0 } ).size() == 1 );
 CHECK( greedy::tapeCost( Vec{ 3 , 1 , 2 } , Vec{ 0 , 0 , 0 } ,
                          greedy::tapeOrder( Vec{ 3 , 1 , 2 } ,
                                             Vec{ 0 , 0 , 0 } ) ) == 0 );
 CHECK( greedy::tapeOrder( Vec{ 2 , 2 , 2 } , Vec{ 5 , 5 , 5 } ) ==
        std::vector< std::size_t >( { 0 , 1 , 2 } ) );  // ties stay
 CHECK( greedy::tapeOrder( Vec{ 1 , 1 } , Vec{ 0 , 7 } ) == I10 );
 CHECK_THROWS( greedy::tapeOrder( Vec{ 0 } , Vec{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::tapeOrder( Vec{ -1 } , Vec{ 1 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::tapeOrder( Vec{ 1 } , Vec{ -1 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::tapeOrder( Vec{ 1 , 2 } , Vec{ 1 } ) ,
               contract_violation );
 }

// the example of the text, every set of k indices on 500 random vectors of
// up to 10 distinct items, the edge cases
ADS_TEST( "greedy-10" )
{
 const Vec X = { 7 , 4 , -2 , 10 , 12 , 5 , 3 , 8 };
 CHECK( greedy::negate( X , 2 ) == 45 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 11 , k = n ? g() % ( n + 1 ) : 0;
  const Vec Y = distinctVec( g , n , -20 , 20 );
  long long best = std::numeric_limits< long long >::min();
  for( std::uint32_t s = 0 ; s < ( 1u << n ) ; ++s ) {
   if( std::size_t( std::popcount( s ) ) != k ) continue;
   long long sum = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) sum += s >> i & 1 ? -Y[ i ] : Y[ i ];
   best = std::max( best , sum );
   }
  CHECK( greedy::negate( Y , k ) == best );
  }
 CHECK( greedy::negate( Vec() , 0 ) == 0 );
 CHECK( greedy::negate( { -3 } , 1 ) == 3 );
 CHECK( greedy::negate( { 1 , 2 , 3 } , 3 ) == -6 );
 CHECK( greedy::negate( { 1 , 2 , 3 } , 0 ) == 6 );
 CHECK( greedy::negate( { -1 , -2 , -3 } , 3 ) == 6 );
 CHECK( greedy::negate( { -1 , -2 , -3 } , 0 ) == -6 );
 CHECK( greedy::negate( { 0 , 0 , 5 } , 2 ) == 5 );
 CHECK( greedy::negate( { -4 , -4 , 4 } , 1 ) == 4 );   // duplicates
 CHECK( greedy::negate( Vec() , 0 ) == 0 );
 CHECK_THROWS( greedy::negate( { 1 } , 2 ) , contract_violation );
 CHECK_THROWS( greedy::negate( Vec() , 1 ) , contract_violation );
 }

// a largest bipartite matching of the participants with the shirts on 600
// random instances, the edge cases (one size missing, all of one size)
ADS_TEST( "greedy-11" )
{
 std::mt19937 g( 1 );
 int yes = 0;
 for( int t = 0 ; t < 600 ; ++t ) {
  std::vector< long long > T( 6 );
  for( auto & x : T ) x = LL( g() % 3 );
  std::vector< int > P( g() % 9 );
  for( auto & s : P ) s = 1 + int( g() % 5 );
  std::vector< int > size;                     // the size of each shirt
  for( int i = 0 ; i < 6 ; ++i )
   for( long long c = 0 ; c < T[ std::size_t( i ) ] ; ++c )
    size.push_back( i + 1 );
  std::vector< std::pair< int , int > > E;
  for( std::size_t j = 0 ; j < P.size() ; ++j )
   for( std::size_t q = 0 ; q < size.size() ; ++q )
    if( ( size[ q ] == P[ j ] ) || ( size[ q ] == P[ j ] + 1 ) )
     E.emplace_back( int( j ) , int( q ) );
  const bool all = bipartiteMatching( int( P.size() ) , int( size.size() ) ,
                                      E ).size() == P.size();
  CHECK( greedy::isDoable( T , P ) == all );
  yes += all;
  }
 CHECK( ( yes > 100 ) && ( yes < 500 ) );
 CHECK( greedy::isDoable( { 0 , 0 , 0 , 0 , 0 , 0 } , {} ) );
 CHECK( greedy::isDoable( { 0 , 0 , 0 , 0 , 0 , 3 } , { 5 , 5 , 5 } ) );
 CHECK( ! greedy::isDoable( { 0 , 0 , 0 , 0 , 3 , 0 } , { 4 , 4 , 4 , 5 } ) );
 CHECK( ! greedy::isDoable( { 0 , 0 , 5 , 0 , 0 , 0 } , { 1 } ) );
 CHECK( greedy::isDoable( { 0 , 1 , 1 , 1 , 1 , 1 } ,
                          { 1 , 2 , 3 , 4 , 5 } ) );
 CHECK( greedy::isDoable( { 9 , 9 , 9 , 9 , 9 , 9 } , {} ) );
 CHECK( ! greedy::isDoable( { 0 , 0 , 0 , 0 , 0 , 0 } , { 3 } ) );
 CHECK( greedy::isDoable( { 0 , 1 , 0 , 0 , 0 , 0 } , { 1 } ) );
 CHECK( ! greedy::isDoable( { 1 , 0 , 0 , 0 , 0 , 0 } , { 2 } ) );
 CHECK( greedy::isDoable( { 2 , 0 , 0 , 0 , 0 , 0 } ,
                          std::vector< int >( 2 , 1 ) ) );
 CHECK_THROWS( greedy::isDoable( { 1 , 1 , 1 , 1 , 1 , 1 } , { 6 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::isDoable( { 1 , 1 , 1 , 1 , 1 , 1 } , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::isDoable( { 1 , 1 , 1 , 1 , 1 } , { 1 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::isDoable( { 1 , 1 , -1 , 1 , 1 , 1 } , { 1 } ) ,
               contract_violation );
 }

// the examples of the text, both graphs against a breadth-first search on
// the moves themselves on 500 random vectors, the edge cases
ADS_TEST( "greedy-12" )
{
 CHECK( greedy::minStepsQuadratic( { 1 , 9 , 3 , 9 } ) == 2 );
 CHECK( greedy::minStepsLinear( { 1 , 9 , 3 , 9 } ) == 2 );
 const std::vector< int > A = { 1 , 3 , 4 , 2 , 1 , 5 , 1 , 6 , 7 , 8 , 9 ,
                                2 };
 CHECK( greedy::minStepsQuadratic( A ) == 3 );
 CHECK( greedy::minStepsLinear( A ) == 3 );
 auto bfs = []( const std::vector< int > & V ) {
  const int n = int( V.size() );
  std::vector< int > d( std::size_t( n ) , -1 );
  std::queue< int > Q;
  d[ 0 ] = 0;
  Q.push( 0 );
  while( ! Q.empty() ) {
   const int u = Q.front();
   Q.pop();
   for( int v = 0 ; v < n ; ++v ) {
    const std::size_t x = std::size_t( u ) , y = std::size_t( v );
    if( ( d[ y ] < 0 ) && ( ( std::abs( u - v ) == 1 ) ||
                            ( ( v != u ) && ( V[ y ] == V[ x ] ) ) ) ) {
     d[ y ] = d[ x ] + 1;
     Q.push( v );
     }
    }
   }
  return( d.back() );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::vector< int > V( 1 + g() % 30 );
  const int top = 1 + int( g() % 9 );
  for( auto & x : V ) x = 1 + int( g() % unsigned( top ) );
  const int r = bfs( V );
  CHECK( greedy::minStepsQuadratic( V ) == r );
  CHECK( greedy::minStepsLinear( V ) == r );
  }
 CHECK( greedy::minStepsLinear( { 5 } ) == 0 );
 CHECK( greedy::minStepsLinear( std::vector< int >( 40 , 9 ) ) == 1 );
 std::vector< int > U( 9 );
 std::iota( U.begin() , U.end() , 1 );
 CHECK( greedy::minStepsLinear( U ) == 8 );
 CHECK( greedy::minStepsQuadratic( { 5 } ) == 0 );
 CHECK( ( greedy::minStepsQuadratic( { 3 , 4 } ) == 1 ) &&
        ( greedy::minStepsLinear( { 3 , 4 } ) == 1 ) );
 CHECK( greedy::minStepsLinear( { 2 , 7 , 7 , 7 , 2 } ) == 1 );
 CHECK( greedy::minStepsQuadratic( std::vector< int >( 300 , 1 ) ) == 1 );
 CHECK_THROWS( greedy::minStepsLinear( { 0 } ) , contract_violation );
 CHECK_THROWS( greedy::minStepsLinear( { 1 , 10 } ) , contract_violation );
 CHECK_THROWS( greedy::minStepsLinear( {} ) , contract_violation );
 CHECK_THROWS( greedy::minStepsQuadratic( {} ) , contract_violation );
 }

// the examples of the text, the largest palindromic permutation by
// enumeration on 500 random multisets of up to 9 digits, a middle 0
ADS_TEST( "greedy-13" )
{
 std::vector< int > A = { 1 , 1 , 3 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 1 , 3 , 1 } ) );
 A = { 2 , 5 , 1 , 5 , 1 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 5 , 1 , 2 , 1 , 5 } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  std::vector< int > B;
  for( std::size_t i = g() % 5 ; i > 0 ; --i ) {
   const int d = int( g() % 10 );
   B.push_back( d );
   B.push_back( d );
   }
  if( g() % 2 ) B.push_back( int( g() % 10 ) );
  std::shuffle( B.begin() , B.end() , g );
  std::vector< int > P( B ) , best;
  std::sort( P.begin() , P.end() );
  do
   if( std::equal( P.begin() , P.end() , P.rbegin() ) ) best = P;
  while( std::next_permutation( P.begin() , P.end() ) );
  greedy::lpp( B );
  CHECK( B == best );
  }
 A = { 1 , 0 , 1 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 1 , 0 , 1 } ) );
 A = { 0 , 1 , 1 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 1 , 0 , 1 } ) );
 A.clear();
 greedy::lpp( A );
 CHECK( A.empty() );
 A = { 9 , 9 , 9 , 9 , 9 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( 5 , 9 ) );
 A = { 0 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 0 } ) );
 A = { 0 , 0 , 0 , 0 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( 4 , 0 ) );
 A = { 0 , 9 , 0 , 9 , 5 };
 greedy::lpp( A );
 CHECK( A == std::vector< int >( { 9 , 0 , 5 , 0 , 9 } ) );
 A = { 1 , 2 };
 CHECK_THROWS( greedy::lpp( A ) , contract_violation );
 A = { 10 };
 CHECK_THROWS( greedy::lpp( A ) , contract_violation );
 A = { -1 , -1 };
 CHECK_THROWS( greedy::lpp( A ) , contract_violation );
 }

// the example of the text (any valid permutation is right), the signs and
// the items checked on 600 random instances, the edge cases
ADS_TEST( "greedy-14" )
{
 auto valid = []( Vec A , const std::string & D , const Vec & S ) {
  for( std::size_t k = 0 ; k < D.size() ; ++k )
   if( D[ k ] == '<' ? !( S[ k ] < S[ k + 1 ] ) : !( S[ k ] > S[ k + 1 ] ) )
    return( false );
  Vec T( S );
  std::sort( A.begin() , A.end() );
  std::sort( T.begin() , T.end() );
  return( A == T );
  };
 const Vec A = { 3 , 23 , 5 , 12 , 32 , 7 };
 CHECK( valid( A , "<><>>" , { 3 , 32 , 5 , 23 , 12 , 7 } ) );
 CHECK( valid( A , "<><>>" , greedy::findOrder( A , "<><>>" ) ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::size_t n = 1 + g() % 12;
  const Vec B = distinctVec( g , n , -50 , 50 );
  std::string D;
  for( std::size_t i = 1 ; i < n ; ++i ) D += g() % 2 ? '<' : '>';
  CHECK( valid( B , D , greedy::findOrder( B , D ) ) );
  }
 CHECK( greedy::findOrder( Vec() , "" ).empty() );
 CHECK( greedy::findOrder( { 4 } , "" ) == Vec( { 4 } ) );
 CHECK( greedy::findOrder( { 3 , 1 , 2 } , "<<" ) == Vec( { 1 , 2 , 3 } ) );
 CHECK( greedy::findOrder( { 3 , 1 , 2 } , ">>" ) == Vec( { 3 , 2 , 1 } ) );
 CHECK( greedy::findOrder( { 9 , -9 } , "<" ) == Vec( { -9 , 9 } ) );
 CHECK( greedy::findOrder( { 9 , -9 } , ">" ) == Vec( { 9 , -9 } ) );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( valid( { M , -M , 0 } , "><" ,
               greedy::findOrder( { M , -M , 0 } , "><" ) ) );
 CHECK_THROWS( greedy::findOrder( { 1 , 2 } , "=" ) , contract_violation );
 CHECK_THROWS( greedy::findOrder( { 1 , 2 } , "<<" ) , contract_violation );
 CHECK_THROWS( greedy::findOrder( { 1 , 2 } , "" ) , contract_violation );
 CHECK_THROWS( greedy::findOrder( Vec() , "<" ) , contract_violation );
 }

// the examples of the text, the wrong dynamic programming on [ -2 , -2 ,
// -2 ], every non empty subset of up to 12 items on 600 random vectors for
// both solutions, the edge cases
ADS_TEST( "greedy-15" )
{
 using V64 = std::vector< std::int64_t >;
 for( auto f : { greedy::minProduct , greedy::minProductDP } ) {
  CHECK( f( { -1 , -1 , 2 , -2 , 4 , 3 } ) == -48 );
  CHECK( f( { -1 , 0 } ) == -1 );
  CHECK( f( { 0 , 1 } ) == 0 );
  CHECK( f( { 1 , 2 , 4 } ) == 1 );
  CHECK( f( { -2 , -2 , -2 } ) == -8 );
  }
 auto wrong = []( const V64 & A ) {
  std::int64_t d = A[ 0 ];
  for( std::size_t i = 1 ; i < A.size() ; ++i )
   d = std::min( { d , d * A[ i ] , A[ i ] } );
  return( d );
  };
 CHECK( wrong( { -2 , -2 , -2 } ) == -2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::size_t n = 1 + g() % 12;
  V64 A( n );
  for( auto & x : A ) x = std::int64_t( g() % 13 ) - 6;
  std::int64_t best = std::numeric_limits< std::int64_t >::max();
  for( std::uint32_t s = 1 ; s < ( 1u << n ) ; ++s ) {
   std::int64_t p = 1;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( s >> i & 1 ) p *= A[ i ];
   best = std::min( best , p );
   }
  CHECK( greedy::minProduct( A ) == best );
  CHECK( greedy::minProductDP( A ) == best );
  }
 CHECK( greedy::minProduct( { 7 } ) == 7 );
 CHECK( greedy::minProduct( { -7 } ) == -7 );
 CHECK( greedy::minProduct( { 0 , 0 , 0 } ) == 0 );
 CHECK( greedy::minProduct( { 5 , 5 , 5 } ) == 5 );
 const std::int64_t M = std::int64_t( 1 ) << 31;
 CHECK( greedy::minProduct( { -M , M } ) == -M * M );
 for( auto f : { greedy::minProduct , greedy::minProductDP } ) {
  CHECK( f( { 7 } ) == 7 );
  CHECK( f( { -7 } ) == -7 );
  CHECK( f( { 0 } ) == 0 );
  CHECK( f( { -3 , -5 } ) == -5 );                   // one negative only
  CHECK( f( { -3 , 0 , -5 } ) == -5 );
  CHECK( f( { 0 , 0 , 0 } ) == 0 );
  }
 CHECK_THROWS( greedy::minProduct( {} ) , contract_violation );
 CHECK_THROWS( greedy::minProductDP( {} ) , contract_violation );
 }

// the counterexamples of the text, the optimum by enumeration of the k^n
// assignments and by the pseudo-polynomial decision of the library on 300
// random instances, the bounds of Graham for the three orders: list
// scheduling within 2 - 1 / k of the optimum, the decreasing order within
// 4 / 3 - 1 / ( 3 k )
ADS_TEST( "greedy-17" )
{
 const Vec L1 = { 1 , 1 , 2 } , L2 = { 3 , 3 , 2 , 2 , 2 };
 CHECK( greedy::listScheduling( L1 , 2 , greedy::byLength( L1 , true ) ) ==
        3 );
 CHECK( greedy::listScheduling( L2 , 2 , greedy::byLength( L2 , false ) ) ==
        7 );
 CHECK( greedy::listScheduling( L1 , 2 , { 0 , 1 , 2 } ) == 3 );
 CHECK( greedy::listScheduling( L2 , 2 , { 0 , 1 , 2 , 3 , 4 } ) == 7 );
 auto opt = []( const Vec & len , int k ) {
  const std::size_t n = len.size();
  long long best = std::numeric_limits< long long >::max();
  std::vector< int > a( n , 0 );
  for( ; ; ) {
   std::vector< long long > load( std::size_t( k ) , 0 );
   for( std::size_t i = 0 ; i < n ; ++i )
    load[ std::size_t( a[ i ] ) ] += len[ i ];
   best = std::min( best , *std::max_element( load.begin() , load.end() ) );
   std::size_t i = 0;
   while( ( i < n ) && ( ++a[ i ] == k ) ) a[ i++ ] = 0;
   if( i == n ) return( best );
   }
  };
 CHECK( opt( L1 , 2 ) == 2 );
 CHECK( opt( L2 , 2 ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = g() % 8;
  const int k = 1 + int( g() % 3 );
  const Vec len = randomVec( g , n , 0 , 9 );
  const long long o = opt( len , k );
  const std::vector< long > len32( len.begin() , len.end() ); // as taken
  long d = 0;
  while( ! schedulingDP( len32 , k , d ) ) ++d;
  CHECK( d == o );
  std::vector< std::size_t > A( n );
  std::iota( A.begin() , A.end() , 0 );
  for( const auto & order : { A , greedy::byLength( len , true ) } ) {
   const long long m = greedy::listScheduling( len , k , order );
   CHECK( ( m >= o ) && ( k * m <= ( 2 * k - 1 ) * o ) );
   }
  const long long m =
   greedy::listScheduling( len , k , greedy::byLength( len , false ) );
  CHECK( ( m >= o ) && ( 3 * k * m <= ( 4 * k - 1 ) * o ) );
  }
 CHECK( greedy::listScheduling( Vec() , 3 , {} ) == 0 );
 CHECK( greedy::listScheduling( { 4 , 4 , 4 } , 3 , { 0 , 1 , 2 } ) == 4 );
 CHECK( greedy::listScheduling( { 4 , 4 , 4 } , 1 , { 0 , 1 , 2 } ) == 12 );
 CHECK( greedy::listScheduling( { 5 , 2 } , 10 , { 0 , 1 } ) == 5 ); // k > n
 CHECK( greedy::listScheduling( { 0 , 0 , 0 } , 2 , { 0 , 1 , 2 } ) == 0 );
 CHECK( greedy::listScheduling( { 3 } , 1 , {} ) == 0 );   // no task given
 CHECK_THROWS( greedy::listScheduling( { 1 } , 0 , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::listScheduling( { 1 } , -1 , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( greedy::listScheduling( { -1 } , 2 , { 0 } ) ,
               contract_violation );
 }

// the example of the text, a largest bipartite matching of the plants with
// the pots on 600 random instances, the edge cases
ADS_TEST( "greedy-18" )
{
 CHECK( greedy::garden( { 3 , 4 , 2 } , { 3 , 3 , 1 } ) == 2 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec P = randomVec( g , g() % 9 , 1 , 10 ) ,
            V = randomVec( g , g() % 9 , 1 , 10 );
  std::vector< std::pair< int , int > > E;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   for( std::size_t j = 0 ; j < V.size() ; ++j )
    if( P[ i ] <= V[ j ] ) E.emplace_back( int( i ) , int( j ) );
  CHECK( greedy::garden( P , V ) ==
         bipartiteMatching( int( P.size() ) , int( V.size() ) , E ).size() );
  }
 CHECK( greedy::garden( {} , { 1 } ) == 0 );
 CHECK( greedy::garden( { 1 } , {} ) == 0 );
 CHECK( greedy::garden( { 5 , 5 , 5 } , { 5 , 5 } ) == 2 );
 CHECK( greedy::garden( { 6 } , { 5 , 5 } ) == 0 );
 CHECK( greedy::garden( {} , {} ) == 0 );
 CHECK( greedy::garden( { 1 , 2 , 3 } , { 3 , 2 , 1 } ) == 3 );  // exact fits
 CHECK( greedy::garden( { 1 } , { 9 , 9 , 9 } ) == 1 );
 CHECK( greedy::garden( Vec( 50 , 4 ) , Vec( 80 , 4 ) ) == 50 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( greedy::garden( { M , -M } , { M } ) == 1 );
 }

/*---------------------- tests of the varie exercises ----------------------*/

// the example of the text, the least | sum | of all the subvectors on 600
// random vectors, the edge cases
ADS_TEST( "varie-1" )
{
 const Vec V = { 3 , 2 , -3 , 4 };
 CHECK( varie::closeToZero( V ) == std::pair( std::size_t( 1 ) ,
                                              std::size_t( 2 ) ) );
 auto sum = []( const Vec & A , std::size_t i , std::size_t j ) {
  long long s = 0;
  for( std::size_t q = i ; q <= j ; ++q ) s += A[ q ];
  return( s );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec A = randomVec( g , 1 + g() % 12 , -20 , 20 );
  long long best = std::numeric_limits< long long >::max();
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i ; j < A.size() ; ++j )
    best = std::min( best , std::abs( sum( A , i , j ) ) );
  const auto [ i , j ] = varie::closeToZero( A );
  CHECK( ( i <= j ) && ( j < A.size() ) );
  CHECK( std::abs( sum( A , i , j ) ) == best );
  }
 CHECK( varie::closeToZero( Vec{ 5 } ) == std::pair( std::size_t( 0 ) ,
                                                     std::size_t( 0 ) ) );
 CHECK( varie::closeToZero( Vec{ 9 , 1 , 9 } ) ==
        std::pair( std::size_t( 1 ) , std::size_t( 1 ) ) );
 CHECK( varie::closeToZero( std::vector< double >{ 0.5 , -0.125 } ) ==
        std::pair( std::size_t( 1 ) , std::size_t( 1 ) ) );
 CHECK( varie::closeToZero( Vec{ 0 } ) == std::pair( std::size_t( 0 ) ,
                                                     std::size_t( 0 ) ) );
 CHECK( varie::closeToZero( Vec{ 4 , 4 , 4 } ) ==              // the first tie
        std::pair( std::size_t( 0 ) , std::size_t( 0 ) ) );
 CHECK( varie::closeToZero( Vec{ 1 , 2 , 0 , 3 } ) ==
        std::pair( std::size_t( 2 ) , std::size_t( 2 ) ) );
 CHECK( varie::closeToZero( Vec{ 5 , -5 } ) ==
        std::pair( std::size_t( 0 ) , std::size_t( 1 ) ) );
 CHECK( varie::closeToZero( Vec{ -3 , -1 , -2 } ) ==
        std::pair( std::size_t( 1 ) , std::size_t( 1 ) ) );
 CHECK_THROWS( varie::closeToZero( Vec() ) , contract_violation );
 }

// every pair of permutations of up to 6 sizes, then 300 random ones of up
// to 200: the least bolt and nut found with at most 8 n comparisons (at
// most 4 per round, and each round moves an index)
ADS_TEST( "varie-2" )
{
 auto run = []( const std::vector< int > & B , const std::vector< int > & D ) {
  long long calls = 0;
  const auto [ b , d ] = varie::findMin( B.size() , [ & ]( std::size_t x ,
                                                           std::size_t y ) {
   ++calls;
   return( B[ x ] < D[ y ] ? -1 : B[ x ] > D[ y ] ? 1 : 0 ); } );
  return( ( B[ b ] == 0 ) && ( D[ d ] == 0 ) &&
          ( calls <= 8 * LL( B.size() ) ) );
  };
 for( int n = 1 ; n <= 6 ; ++n ) {
  std::vector< int > B( static_cast< std::size_t >( n ) ) , D;
  std::iota( B.begin() , B.end() , 0 );
  do {
   D = B;
   do CHECK( run( B , D ) );
   while( std::next_permutation( D.begin() , D.end() ) );
   } while( std::next_permutation( B.begin() , B.end() ) );
  }
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  std::vector< int > B( 1 + g() % 200 );
  std::iota( B.begin() , B.end() , 0 );
  std::vector< int > D( B );
  std::shuffle( B.begin() , B.end() , g );
  std::shuffle( D.begin() , D.end() , g );
  CHECK( run( B , D ) );
  }
 CHECK_THROWS( varie::findMin( 0 , []( std::size_t , std::size_t ) {
  return( 0 ); } ) , contract_violation );
 }

// a pair within d on 800 random vectors with repetitions, against the
// least gap of the sorted vector, the edge cases (all equal, two items,
// the largest item alone in the last bucket)
ADS_TEST( "varie-3" )
{
 auto check = []( const std::vector< double > & A ) {
  const auto [ i , j ] = varie::closePoints( A );
  const auto [ lo , hi ] = std::minmax_element( A.begin() , A.end() );
  const double d = ( *hi - *lo ) / double( A.size() - 1 );
  std::vector< double > S( A );
  std::sort( S.begin() , S.end() );
  double gap = S[ 1 ] - S[ 0 ];
  for( std::size_t q = 2 ; q < S.size() ; ++q )
   gap = std::min( gap , S[ q ] - S[ q - 1 ] );
  return( ( i != j ) && ( i < A.size() ) && ( j < A.size() ) &&
          ( gap <= d ) && ( std::abs( A[ i ] - A[ j ] ) <= d * 1.000001 ) );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 800 ; ++t ) {
  std::vector< double > A( 2 + g() % 20 );
  const unsigned range = 1 + g() % 1000;
  for( auto & x : A ) x = double( g() % range ) / 7.0 - 50;
  CHECK( check( A ) );
  }
 CHECK( check( std::vector< double >( 5 , 3.5 ) ) );
 CHECK( check( { 1 , 2 } ) );
 CHECK( check( { 0 , 1 , 2 , 3 , 4 , 100 } ) );
 CHECK( check( { 0 , 25 , 50 , 75 , 100 } ) );
 CHECK( check( { -1e300 , 1e300 , 0 } ) );
 CHECK( check( { 7 , 7 } ) );                     // two equal items, d = 0
 CHECK( check( { 1 , 1 , 9 } ) );                 // a pair at the minimum
 CHECK( check( { 1 , 9 , 9 } ) );                 // a pair at the maximum
 CHECK( check( { 4 , 3 , 2 , 1 , 0 } ) );         // all gaps d, reversed
 CHECK( varie::closePoints( { 2 , 5 } ) ==
        std::pair( std::size_t( 0 ) , std::size_t( 1 ) ) );
 CHECK_THROWS( varie::closePoints( { 1 } ) , contract_violation );
 CHECK_THROWS( varie::closePoints( {} ) , contract_violation );
 }

// the example of the text with its matrix DP, the brute force on 400
// random matrices, the edge cases (also rectangular and ragged ones)
ADS_TEST( "varie-4" )
{
 const Matrix A = { { 1 , 0 , 1 , 0 , 1 , 0 , 0 } ,
                    { 1 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                    { 0 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 0 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 0 } };
 const Matrix DP = { { 1 , 0 , 1 , 0 , 1 , 0 , 0 } ,
                     { 1 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 1 , 1 , 2 , 2 , 2 , 0 } ,
                     { 0 , 0 , 1 , 2 , 3 , 3 , 0 } ,
                     { 1 , 1 , 1 , 2 , 3 , 4 , 0 } ,
                     { 1 , 2 , 2 , 2 , 3 , 4 , 0 } };
 CHECK( varie::maxSquareTable( A ) == DP );
 CHECK( varie::maxSquare( A ) == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  const Matrix B = randomBits( g , n , n , 1 + g() % 9 , 10 );
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int k = 1 ; ( i + k <= n ) && ( j + k <= n ) ; ++k ) {
     bool ok = true;
     for( int x = i ; x < i + k ; ++x )
      for( int y = j ; y < j + k ; ++y )
       ok = ok && B[ std::size_t( x ) ][ std::size_t( y ) ];
     if( ok ) best = std::max( best , k );
     }
  CHECK( varie::maxSquare( B ) == best );
  }
 CHECK( varie::maxSquare( {} ) == 0 );
 CHECK( varie::maxSquare( { { 0 } } ) == 0 );
 CHECK( varie::maxSquare( { { 1 } } ) == 1 );
 const Matrix ones( 30 , std::vector< int >( 30 , 1 ) ) ,
              zeros( 30 , std::vector< int >( 30 , 0 ) );
 CHECK( varie::maxSquare( ones ) == 30 );
 CHECK( varie::maxSquare( zeros ) == 0 );
 // rectangular matrices: a row, a column, a wide and a tall one
 CHECK( varie::maxSquare( { std::vector< int >( 9 , 1 ) } ) == 1 );
 CHECK( varie::maxSquare( Matrix( 9 , std::vector< int >( 1 , 1 ) ) ) == 1 );
 CHECK( varie::maxSquare( Matrix( 3 , std::vector< int >( 7 , 1 ) ) ) == 3 );
 CHECK( varie::maxSquare( Matrix( 7 , std::vector< int >( 2 , 1 ) ) ) == 2 );
 CHECK( varie::maxSquareTable( { { 1 , 1 , 1 } , { 1 , 1 , 1 } } ) ==
        Matrix( { { 1 , 1 , 1 } , { 1 , 2 , 2 } } ) );
 CHECK( varie::maxSquareTable( {} ).empty() );
 CHECK( varie::maxSquare( Matrix( 3 ) ) == 0 );          // rows of no cells
 CHECK_THROWS( varie::maxSquare( { { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 }

// the count of mistery() against n ceil( log2 n ) ( floor( log3 n ) + 1 )
// for n up to 300 and some powers, and its ratio to n log2 n log3 n
// tending to 1, so Theta( n log^2 n )
ADS_TEST( "varie-5" )
{
 auto formula = []( std::uint64_t n ) {
  const std::uint64_t c2 = n > 1 ? std::uint64_t( std::bit_width( n - 1 ) )
                                 : 0;
  std::uint64_t f3 = 0;
  for( std::uint64_t p = 3 ; p <= n ; p *= 3 ) ++f3;
  return( n * c2 * ( f3 + 1 ) );
  };
 for( std::uint64_t n = 0 ; n <= 300 ; ++n )
  CHECK( varie::mistery( n ) == formula( n ) );
 double last = 10;
 for( std::uint64_t n : { 9u , 81u , 729u , 6561u , 59049u } ) {
  const std::uint64_t s = varie::mistery( n );
  CHECK( s == formula( n ) );
  const double r = double( s ) / ( double( n ) * std::log2( double( n ) ) *
                                   std::log( double( n ) ) / std::log( 3.0 ) );
  CHECK( ( r >= 1 ) && ( r < last ) );
  last = r;
  }
 CHECK( last < 1.2 );
 }

// disjoint segments of largest length by brute force on every subset of up
// to 10 random segments, the edge cases
ADS_TEST( "varie-6" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 11;
  Vec a( n ) , b( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   a[ i ] = LL( g() % 30 );
   b[ i ] = a[ i ] + 1 + LL( g() % 8 );
   }
  long long best = 0;
  for( std::uint32_t s = 0 ; s < ( 1u << n ) ; ++s ) {
   bool ok = true;
   long long len = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( s >> i & 1 ) {
     len += b[ i ] - a[ i ];
     for( std::size_t j = i + 1 ; j < n ; ++j )
      if( ( s >> j & 1 ) && ( a[ i ] < b[ j ] ) && ( a[ j ] < b[ i ] ) )
       ok = false;
     }
   if( ok ) best = std::max( best , len );
   }
  const auto [ S , w ] = varie::segmentCover( a , b );
  long long len = 0;
  for( std::size_t i : S ) len += b[ i ] - a[ i ];
  CHECK( ( w == best ) && ( len == best ) );
  }
 CHECK( varie::segmentCover( {} , {} ).second == 0 );
 CHECK( varie::segmentCover( { 0 , 5 } , { 5 , 9 } ).second == 9 );
 CHECK( varie::segmentCover( { 0 , 0 , 0 } , { 4 , 4 , 4 } ).second == 4 );
 CHECK( varie::segmentCover( { 0 , 1 } , { 10 , 2 } ).second == 10 );
 CHECK( varie::segmentCover( { 0 , 4 , 6 } , { 5 , 7 , 11 } ).second == 10 );
 CHECK( varie::segmentCover( { -9 } , { -2 } ).second == 7 );
 CHECK_THROWS( varie::segmentCover( { 3 } , { 3 } ) , contract_violation );
 CHECK_THROWS( varie::segmentCover( { 4 } , { 3 } ) , contract_violation );
 CHECK_THROWS( varie::segmentCover( { 1 , 2 } , { 3 } ) , contract_violation );
 }

// a breadth-first search on the forward moves on 600 random vectors, the
// edge cases
ADS_TEST( "varie-7" )
{
 auto bfs = []( const Vec & V ) {
  const std::size_t n = V.size();
  std::vector< int > d( n , -1 );
  std::queue< std::size_t > Q;
  d[ 0 ] = 0;
  Q.push( 0 );
  while( ! Q.empty() ) {
   const std::size_t u = Q.front();
   Q.pop();
   for( std::size_t k = 1 ; ( k <= std::size_t( V[ u ] ) ) && ( u + k < n ) ;
        ++k )
    if( d[ u + k ] < 0 ) {
     d[ u + k ] = d[ u ] + 1;
     Q.push( u + k );
     }
   }
  return( d.back() );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec V = randomVec( g , 1 + g() % 25 , 1 , 1 + LL( g() % 6 ) );
  CHECK( varie::minMoves( V ) == bfs( V ) );
  }
 CHECK( varie::minMoves( { 7 } ) == 0 );
 CHECK( varie::minMoves( Vec( 20 , 1 ) ) == 19 );
 CHECK( varie::minMoves( Vec( 20 , 100 ) ) == 1 );
 CHECK( varie::minMoves( { 1 , 9 } ) == 1 );
 CHECK( varie::minMoves( { 2 , 1 , 1 } ) == 1 );         // exactly to the end
 CHECK( varie::minMoves( { 1 , 3 , 1 , 1 , 1 } ) == 2 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( varie::minMoves( { M , 1 , 1 } ) == 1 );
 CHECK_THROWS( varie::minMoves( { 0 , 1 } ) , contract_violation );
 CHECK_THROWS( varie::minMoves( { 1 , -1 } ) , contract_violation );
 CHECK_THROWS( varie::minMoves( {} ) , contract_violation );
 }

// the example of the text, the brute force on every subvector of 600
// random vectors, the edge cases
ADS_TEST( "varie-8" )
{
 CHECK( varie::count01( varie::bitsOf( "00111111100011110000" ) ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  std::vector< int > V( g() % 25 );
  for( auto & x : V ) x = int( g() % 2 );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < V.size() ; ++i )
   for( std::size_t k = 1 ; i + 2 * k <= V.size() ; ++k ) {
    bool ok = true;
    for( std::size_t q = 0 ; q < 2 * k ; ++q )
     ok = ok && ( V[ i + q ] == ( q < k ? 0 : 1 ) );
    if( ok ) best = std::max( best , 2 * k );
    }
  CHECK( varie::count01( V ) == best );
  }
 CHECK( varie::count01( {} ) == 0 );
 CHECK( varie::count01( { 1 , 0 } ) == 0 );
 CHECK( varie::count01( std::vector< int >( 9 , 0 ) ) == 0 );
 CHECK( varie::count01( varie::bitsOf( "0001" ) ) == 2 );
 CHECK( varie::count01( varie::bitsOf( "000111" ) ) == 6 );
 CHECK( varie::count01( varie::bitsOf( "01" ) ) == 2 );
 CHECK( varie::count01( varie::bitsOf( "0101" ) ) == 2 );
 CHECK( varie::count01( varie::bitsOf( "1100" ) ) == 0 );
 CHECK( varie::count01( std::vector< int >( 9 , 1 ) ) == 0 );
 CHECK( varie::count01( varie::bitsOf( "0011101" ) ) == 4 );
 CHECK_THROWS( varie::count01( { 2 } ) , contract_violation );
 CHECK_THROWS( varie::count01( { 0 , 1 , -1 } ) , contract_violation );
 }

// the examples of the text (the split { 2 , 5 } , { 1 , 3 } , { 2 , 1 } of
// time 7, the least 6), every split by brute force on 500 random
// instances, the edge cases
ADS_TEST( "varie-9" )
{
 const Vec H = { 2 , 5 , 1 , 3 , 2 , 1 };
 auto split = []( const Vec & A , const std::vector< std::size_t > & cuts ) {
  long long worst = 0 , cur = 0;
  std::size_t c = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i ) {
   if( ( c < cuts.size() ) && ( cuts[ c ] == i ) ) {
    worst = std::max( worst , cur );
    cur = 0;
    ++c;
    }
   cur += A[ i ];
   }
  return( std::max( worst , cur ) );
  };
 CHECK( split( H , { 2 , 4 } ) == 7 );
 CHECK( split( H , { 1 , 3 } ) == 6 );
 CHECK( varie::partition( H , 3 ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 10 , k = 1 + g() % 4;
  const Vec A = randomVec( g , n , 0 , 9 );
  long long best = std::numeric_limits< long long >::max();
  // the cuts as a subset of the n - 1 inner places, at most k - 1 of them
  const std::uint32_t top = n ? 1u << ( n - 1 ) : 1;
  for( std::uint32_t s = 0 ; s < top ; ++s ) {
   if( std::size_t( std::popcount( s ) ) + 1 > k ) continue;
   std::vector< std::size_t > cuts;
   for( std::size_t i = 1 ; i < n ; ++i )
    if( s >> ( i - 1 ) & 1 ) cuts.push_back( i );
   best = std::min( best , split( A , cuts ) );
   }
  CHECK( varie::partition( A , k ) == best );
  }
 CHECK( varie::partition( Vec() , 3 ) == 0 );
 CHECK( varie::partition( { 8 } , 3 ) == 8 );
 CHECK( varie::partition( Vec( 6 , 4 ) , 3 ) == 8 );
 CHECK( varie::partition( Vec( 6 , 4 ) , 10 ) == 4 );
 CHECK( varie::partition( { 1 , 2 , 3 } , 1 ) == 6 );
 CHECK( varie::partition( { 0 , 0 , 0 } , 2 ) == 0 );
 CHECK( varie::partition( { 1 , 9 , 1 } , 3 ) == 9 );    // k = n
 CHECK( varie::partition( { 9 , 1 , 1 , 1 } , 2 ) == 9 );
 CHECK( varie::partition( Vec() , 1 ) == 0 );
 CHECK_THROWS( varie::partition( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( varie::partition( { 1 , -1 } , 2 ) , contract_violation );
 }

// the example of the text, the strings counted on every n up to 20, the
// Fibonacci numbers of the library up to the largest n of 64 bits, the
// bounds of n
ADS_TEST( "varie-10" )
{
 CHECK( varie::bits( 5 ) == 13 );
 for( int n = 0 ; n <= 20 ; ++n ) {
  std::uint64_t c = 0;
  for( std::uint32_t m = 0 ; m < ( 1u << n ) ; ++m )
   c += ( m & ( m >> 1 ) ) == 0;
  CHECK( varie::bits( n ) == c );
  }
 for( int n = 0 ; n <= 91 ; ++n )
  CHECK( varie::bits( n ) == fibonacci( n + 1 ) );
 CHECK( varie::bits( 0 ) == 1 );
 CHECK( varie::bits( 91 ) == 12200160415121876738ull );
 CHECK( varie::bits( 1 ) == 2 );
 CHECK_THROWS( varie::bits( 92 ) , contract_violation );
 CHECK_THROWS( varie::bits( -1 ) , contract_violation );
 }

// the example of the text, the four nested loops on 500 random vectors and
// values of k, the edge cases with the largest values allowed
ADS_TEST( "varie-11" )
{
 using V64 = std::vector< std::int64_t >;
 const V64 A = { 7 , 1 , 13 , 12 , -3 , 7 };
 CHECK( varie::fourCubic( A , 35 ) && varie::fourPairs( A , 35 ) );
 std::mt19937 g( 1 );
 int yes = 0;
 for( int t = 0 ; t < 500 ; ++t ) {
  const Vec R = randomVec( g , g() % 9 , -30 , 30 );
  const V64 B( R.begin() , R.end() );
  const std::int64_t k = std::int64_t( g() % 241 ) - 120;
  bool r = false;
  const std::size_t n = B.size();
  for( std::size_t a = 0 ; a < n ; ++a )
   for( std::size_t b = a ; b < n ; ++b )
    for( std::size_t c = b ; c < n ; ++c )
     for( std::size_t d = c ; d < n ; ++d )
      r = r || ( B[ a ] + B[ b ] + B[ c ] + B[ d ] == k );
  CHECK( varie::fourCubic( B , k ) == r );
  CHECK( varie::fourPairs( B , k ) == r );
  yes += r;
  }
 CHECK( yes > 0 );                          // some yes: their number
                                            // depends on the library
 CHECK( ! varie::fourCubic( V64() , 0 ) && ! varie::fourPairs( V64() , 0 ) );
 CHECK( varie::fourCubic( { 5 } , 20 ) && varie::fourPairs( { 5 } , 20 ) );
 CHECK( ! varie::fourCubic( { 5 } , 15 ) && ! varie::fourPairs( { 5 } , 15 ) );
 const std::int64_t M = ( std::int64_t( 1 ) << 60 ) - 1 , K = 4 * M;
 CHECK( varie::fourCubic( { -M , M } , K ) && varie::fourPairs( { -M , M } ,
                                                                K ) );
 CHECK( varie::fourCubic( { -M , M } , -K ) && varie::fourPairs( { -M , M } ,
                                                                 -K ) );
 CHECK( ! varie::fourCubic( { -M , M } , 1 ) );
 CHECK( varie::fourCubic( { -M , M } , 0 ) && varie::fourPairs( { -M , M } ,
                                                                0 ) );
 CHECK( varie::fourCubic( { 0 } , 0 ) && varie::fourPairs( { 0 } , 0 ) );
 CHECK( ! varie::fourCubic( V64( 9 , 2 ) , 9 ) &&
        ! varie::fourPairs( V64( 9 , 2 ) , 9 ) );
 CHECK( varie::fourCubic( { 1 , 2 , 3 , 4 } , 10 ) &&
        varie::fourPairs( { 1 , 2 , 3 , 4 } , 10 ) );
 CHECK( varie::fourCubic( { 1 , 2 , 3 , 4 } , 4 ) &&
        varie::fourPairs( { 1 , 2 , 3 , 4 } , 16 ) );   // the least, the most
 CHECK( ! varie::fourCubic( { 1 , 2 , 3 , 4 } , 3 ) &&
        ! varie::fourPairs( { 1 , 2 , 3 , 4 } , 17 ) );
 }

// the example of the text, the four sides of every square checked on 400
// random matrices, the edge cases (a ring of 1s with a 0 inside)
ADS_TEST( "varie-12" )
{
 const Matrix A = { { 1 , 0 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } ,
                    { 0 , 1 , 1 , 0 , 0 , 1 , 1 , 1 } ,
                    { 1 , 1 , 1 , 0 , 0 , 1 , 1 , 1 } ,
                    { 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } ,
                    { 0 , 0 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 0 , 1 , 0 , 0 , 1 , 0 , 1 , 1 } };
 CHECK( varie::largestSquare( A ) == 5 );
 auto brute = []( const Matrix & B ) {
  const int n = int( B.size() );
  auto at = [ & ]( int i , int j ) {
   return( B[ std::size_t( i ) ][ std::size_t( j ) ] ); };
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int k = 1 ; ( i + k <= n ) && ( j + k <= n ) ; ++k ) {
     bool ok = true;
     for( int q = 0 ; q < k ; ++q )
      ok = ok && at( i , j + q ) && at( i + k - 1 , j + q ) &&
           at( i + q , j ) && at( i + q , j + k - 1 );
     if( ok ) best = std::max( best , k );
     }
  return( best );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  const Matrix B = randomBits( g , n , n , 5 + g() % 5 , 10 );
  CHECK( varie::largestSquare( B ) == brute( B ) );
  }
 CHECK( varie::largestSquare( {} ) == 0 );
 CHECK( varie::largestSquare( { { 1 } } ) == 1 );
 CHECK( varie::largestSquare( { { 0 } } ) == 0 );
 CHECK( varie::largestSquare( { { 1 , 1 , 1 } , { 1 , 0 , 1 } ,
                                { 1 , 1 , 1 } } ) == 3 );
 CHECK( varie::largestSquare( { { 0 , 1 , 1 } , { 0 , 1 , 1 } ,
                                { 0 , 0 , 0 } } ) == 2 );
 CHECK( varie::largestSquare( Matrix( 25 , std::vector< int >( 25 , 1 ) ) ) ==
        25 );
 CHECK( varie::largestSquare( { { 1 , 1 } , { 1 , 0 } } ) == 1 );
 CHECK( varie::largestSquare( { { 0 , 1 } , { 1 , 1 } } ) == 1 );
 CHECK( varie::largestSquare( { { 1 , 0 , 1 } , { 0 , 1 , 0 } ,
                                { 1 , 0 , 1 } } ) == 1 );
 CHECK_THROWS( varie::largestSquare( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( varie::largestSquare( { { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 }

// the examples of the text, the counts of every subvector on 600 random
// vectors, the edge cases
ADS_TEST( "varie-13" )
{
 CHECK( varie::zeroUnbalance( varie::bitsOf( "00001000" ) ) == 6 );
 CHECK( varie::zeroUnbalance( varie::bitsOf( "11000010001" ) ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  std::vector< int > A( g() % 30 );
  for( auto & x : A ) x = int( g() % 2 );
  long long best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i ) {
   long long z = 0;
   for( std::size_t j = i ; j < A.size() ; ++j ) {
    z += A[ j ] ? -1 : 1;
    best = std::max( best , z );
    }
   }
  CHECK( varie::zeroUnbalance( A ) == best );
  }
 CHECK( varie::zeroUnbalance( {} ) == 0 );
 CHECK( varie::zeroUnbalance( std::vector< int >( 9 , 1 ) ) == 0 );
 CHECK( varie::zeroUnbalance( std::vector< int >( 9 , 0 ) ) == 9 );
 CHECK( varie::zeroUnbalance( { 0 } ) == 1 );
 CHECK( varie::zeroUnbalance( { 1 } ) == 0 );
 CHECK( varie::zeroUnbalance( varie::bitsOf( "0101010" ) ) == 1 );
 CHECK( varie::zeroUnbalance( varie::bitsOf( "001000" ) ) == 4 );
 CHECK_THROWS( varie::zeroUnbalance( { 3 } ) , contract_violation );
 CHECK_THROWS( varie::zeroUnbalance( { 0 , -1 } ) , contract_violation );
 }

// the example of the text, the binomial coefficient C( 2 n , n ) of the
// triangle of Tartaglia of the library for every n up to 33 (and of the
// recursion and of the factorials for the small ones), the largest of 64
// bits
ADS_TEST( "varie-14" )
{
 CHECK( varie::paths( 2 ) == 6 );
 const auto C = tartaglia( 66 );
 for( int n = 0 ; n <= 33 ; ++n )
  CHECK( varie::paths( n ) == C[ std::size_t( 2 * n ) ][ std::size_t( n ) ] );
 for( int n = 0 ; n <= 10 ; ++n )
  CHECK( ( varie::paths( n ) == binomial( 2 * n , n ) ) &&
         ( varie::paths( n ) == binomialByFactorials( 2 * n , n ) ) );
 CHECK( varie::paths( 1 ) == 2 );
 CHECK( varie::paths( 33 ) == 7219428434016265740ull );
 CHECK( varie::paths( 0 ) == 1 );
 CHECK_THROWS( varie::paths( 34 ) , contract_violation );
 CHECK_THROWS( varie::paths( -1 ) , contract_violation );
 }

// the brute force on the 3^n choices per column on 500 random boards,
// Hateville on its own, the edge cases
ADS_TEST( "varie-15" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 9;
  const std::vector< Vec > A = { randomVec( g , n , 1 , 20 ) ,
                                 randomVec( g , n , 1 , 20 ) };
  long long best = 0;
  std::size_t total = 1;
  for( std::size_t i = 0 ; i < n ; ++i ) total *= 3;
  for( std::size_t code = 0 ; code < total ; ++code ) {
   std::vector< int > c( n );                // 0 none, 1 top, 2 bottom
   std::size_t x = code;
   for( std::size_t i = 0 ; i < n ; ++i , x /= 3 ) c[ i ] = int( x % 3 );
   bool ok = true;
   long long gain = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    if( c[ i ] ) gain += A[ std::size_t( c[ i ] - 1 ) ][ i ];
    if( ( i > 0 ) && c[ i ] && c[ i - 1 ] ) ok = false;
    }
   if( ok ) best = std::max( best , gain );
   }
  CHECK( varie::maxGain( A ) == best );
  }
 CHECK( varie::hateville( { 5 , 1 , 1 , 5 } ) == 10 );
 CHECK( varie::maxGain( { Vec() , Vec() } ) == 0 );
 CHECK( varie::maxGain( { { 3 } , { 4 } } ) == 4 );
 CHECK( varie::maxGain( { Vec( 7 , 2 ) , Vec( 7 , 2 ) } ) == 8 );
 CHECK( varie::maxGain( { { 1 , 9 } , { 9 , 1 } } ) == 9 );  // neighbours
 CHECK( varie::maxGain( { { 5 , 1 , 5 } , { 1 , 1 , 1 } } ) == 10 );
 CHECK( varie::hateville( {} ) == 0 );
 CHECK( varie::hateville( { 3 } ) == 3 );
 CHECK( varie::hateville( { 3 , 4 } ) == 4 );
 CHECK_THROWS( varie::maxGain( { { 1 } } ) , contract_violation );
 CHECK_THROWS( varie::maxGain( { { 1 , 2 } , { 3 } } ) , contract_violation );
 CHECK_THROWS( varie::maxGain( { { 1 } , { 2 } , { 3 } } ) ,
               contract_violation );
 }

// the examples of the text (and the pairwise solutions they refute, by the
// library), every subsequence of the first string on 300 random triples,
// the edge cases
ADS_TEST( "varie-16" )
{
 CHECK( lcs3( "ccabbacc" , "ccccabba" , "cccc" ) == "cccc" );
 CHECK( lcs( std::string( "ccabbacc" ) ,
             std::string( "ccccabba" ) ).size() == 6 );
 CHECK( lcs( std::string( "ccabba" ) , std::string( "cccc" ) ).size() == 2 );
 CHECK( lcs3( "ABC" , "BCD" , "CDE" ) == "C" );
 CHECK( lcs( std::string( "ABC" ) , std::string( "BCD" ) ).size() == 2 );
 CHECK( lcs( std::string( "BCD" ) , std::string( "CDE" ) ).size() == 2 );
 CHECK( lcs( std::string( "ABC" ) , std::string( "CDE" ) ).size() == 1 );
 std::mt19937 g( 1 );
 auto word = [ & ]( std::size_t n ) {
  std::string s;
  for( ; n > 0 ; --n ) s += char( 'a' + g() % 3 );
  return( s );
  };
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::string X = word( g() % 11 ) , Y = word( g() % 11 ) ,
                    Z = word( g() % 11 );
  std::size_t best = 0;
  for( std::uint32_t s = 0 ; s < ( 1u << X.size() ) ; ++s ) {
   std::string P;
   for( std::size_t i = 0 ; i < X.size() ; ++i )
    if( s >> i & 1 ) P += X[ i ];
   if( subsequenceOf( P , Y ) && subsequenceOf( P , Z ) )
    best = std::max( best , P.size() );
   }
  const std::size_t r = lcs3( X , Y , Z ).size();
  CHECK( r == best );
  CHECK( r <= lcs( X , Y ).size() );
  }
 CHECK( lcs3( "" , "abc" , "abc" ).empty() );
 CHECK( lcs3( "a" , "a" , "a" ) == "a" );
 CHECK( lcs3( "aaaa" , "aaaa" , "aaaa" ) == "aaaa" );
 CHECK( lcs3( "abc" , "def" , "abc" ).empty() );
 CHECK( lcs3( "" , "" , "" ).empty() );
 CHECK( lcs3( "abc" , "abc" , "" ).empty() );
 CHECK( lcs3( "abcd" , "abcd" , "abcd" ) == "abcd" );    // all equal
 CHECK( lcs3( "a" , "aaaa" , "aa" ) == "a" );            // one much shorter
 CHECK( lcs3( "abcdefghijklmnopqrstuvwxyz" , "zyxwvutsrqponmlkjihgfedcba" ,
              "abcdefghijklmnopqrstuvwxyz" ).size() == 1 );
 }

// the examples of the text, the two nested loops on 600 random sorted
// vectors, the edge cases
ADS_TEST( "varie-17" )
{
 const Vec A1 = { 1 , 5 , 8 , 10 } , A2 = { 31 , 37 , 43 , 52 };
 CHECK( varie::findPair( A1 , A2 , 51 ) );
 CHECK( ! varie::findPair( A1 , A2 , 6 ) );
 std::mt19937 g( 1 );
 int yes = 0;
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::size_t n = g() % 10;
  Vec B1 = randomVec( g , n , -20 , 20 ) , B2 = randomVec( g , n , -20 , 20 );
  std::sort( B1.begin() , B1.end() );
  std::sort( B2.begin() , B2.end() );
  const long long k = LL( g() % 61 ) - 30;
  bool r = false;
  for( long long x : B1 )
   for( long long y : B2 ) r = r || ( x + y == k );
  CHECK( varie::findPair( B1 , B2 , k ) == r );
  yes += r;
  }
 CHECK( yes > 0 );                          // some yes: their number
                                            // depends on the library
 CHECK( ! varie::findPair( Vec() , Vec() , 0 ) );
 CHECK( varie::findPair( { 3 } , { 4 } , 7 ) );
 CHECK( varie::findPair( Vec( 5 , 2 ) , Vec( 5 , 2 ) , 4 ) );
 CHECK( ! varie::findPair( { 1 , 2 } , Vec() , 3 ) );    // one side empty
 CHECK( ! varie::findPair( Vec() , { 1 , 2 } , 3 ) );
 CHECK( varie::findPair( { -5 , 0 , 5 } , { -5 , 0 , 5 } , -10 ) );
 CHECK( varie::findPair( { -5 , 0 , 5 } , { -5 , 0 , 5 } , 10 ) );
 CHECK( ! varie::findPair( { -5 , 0 , 5 } , { -5 , 0 , 5 } , 11 ) );
 CHECK( ! varie::findPair( { 3 } , { 4 } , 8 ) );
 CHECK_THROWS( varie::findPair( { 2 , 1 } , { 1 , 2 } , 3 ) ,
               contract_violation );
 CHECK_THROWS( varie::findPair( { 1 , 2 } , { 2 , 1 } , 3 ) ,
               contract_violation );
 }

// the distances of Floyd-Warshall and the same scan on 500 random graphs
// with negative costs and no negative cycle, the edge cases
ADS_TEST( "varie-18" )
{
 const double INF = std::numeric_limits< double >::infinity();
 std::mt19937 g( 1 );
 int done = 0 , yes = 0;
 while( done < 500 ) {
  const int n = 1 + int( g() % 7 );
  ListGraph< double > G( n , true );
  std::vector< std::vector< double > > F( std::size_t( n ) ,
                                          std::vector< double >(
                                           std::size_t( n ) , INF ) );
  for( int u = 0 ; u < n ; ++u ) {
   F[ std::size_t( u ) ][ std::size_t( u ) ] = 0;
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && ( g() % 3 == 0 ) ) {
     const double w = double( int( g() % 9 ) - 2 );
     G.insertEdge( u , v , w );
     F[ std::size_t( u ) ][ std::size_t( v ) ] = w;
     }
   }
  for( std::size_t k = 0 ; k < F.size() ; ++k )
   for( auto & row : F )
    for( std::size_t j = 0 ; j < F.size() ; ++j )
     if( ( row[ k ] < INF ) && ( F[ k ][ j ] < INF ) )
      row[ j ] = std::min( row[ j ] , row[ k ] + F[ k ][ j ] );
  bool negative = false;
  for( std::size_t u = 0 ; u < F.size() ; ++u ) negative |= F[ u ][ u ] < 0;
  if( negative ) continue;
  const int s = int( g() % unsigned( n ) ) , t = int( g() % unsigned( n ) );
  bool r = false;
  for( std::size_t u = 0 ; u < F.size() ; ++u )
   r = r || ( ( F[ std::size_t( s ) ][ u ] == F[ std::size_t( t ) ][ u ] ) &&
              ( F[ std::size_t( s ) ][ u ] < INF ) );
  CHECK( varie::meetingPoint( G , s , t ) == r );
  yes += r;
  ++done;
  }
 CHECK( ( yes > 100 ) && ( yes < 500 ) );
 ListGraph< double > G( 3 , true );
 CHECK( varie::meetingPoint( G , 1 , 1 ) );
 CHECK( ! varie::meetingPoint( G , 0 , 1 ) );
 G.insertEdge( 0 , 2 , 1.5 );
 G.insertEdge( 1 , 2 , 1.5 );
 CHECK( varie::meetingPoint( G , 0 , 1 ) );
 G.insertEdge( 1 , 2 , 2.5 );
 CHECK( ! varie::meetingPoint( G , 0 , 1 ) );
 ListGraph< double > H( 1 , true );
 CHECK( varie::meetingPoint( H , 0 , 0 ) );              // one node
 ListGraph< double > D( 2 , true );                      // 1 at 1 and 0,
 D.insertEdge( 0 , 1 , 1 );                              // 0 at 0 and inf
 CHECK( ! varie::meetingPoint( D , 0 , 1 ) );
 D.insertEdge( 1 , 0 , 0 );                              // 0 at 0 and 0
 CHECK( varie::meetingPoint( D , 0 , 1 ) );
 CHECK_THROWS( varie::meetingPoint( G , 0 , 3 ) , contract_violation );
 G.insertEdge( 2 , 0 , -2 );
 CHECK_THROWS( varie::meetingPoint( G , 0 , 1 ) , contract_violation );
 }

// the examples of the text, the largest arithmetic subvectors listed by
// brute force on 600 random vectors, the edge cases
ADS_TEST( "varie-19" )
{
 CHECK( varie::countArithmetic( { -1 , 1 , 3 , 5 , 8 , 11 } ) == 2 );
 CHECK( varie::countArithmetic( { 1 , 4 , 7 , 10 , 13 } ) == 1 );
 CHECK( varie::countArithmetic( { 6 , 6 , 6 , 6 } ) == 1 );
 CHECK( varie::countArithmetic( { 3 , -2 , -7 , -12 } ) == 1 );
 CHECK( varie::countArithmetic( { 3 , 4 , 2 , 1 } ) == 0 );
 auto brute = []( const Vec & A ) {
  const std::size_t n = A.size();
  auto arith = [ & ]( std::size_t i , std::size_t j ) {     // A[ i .. j ]
   if( ( j >= n ) || ( j < i + 2 ) ) return( false );
   for( std::size_t q = i + 2 ; q <= j ; ++q )
    if( A[ q ] - A[ q - 1 ] != A[ i + 1 ] - A[ i ] ) return( false );
   return( true );
   };
  std::size_t c = 0;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = i + 2 ; j < n ; ++j )
    c += arith( i , j ) && !( i > 0 && arith( i - 1 , j ) ) &&
         ! arith( i , j + 1 );
  return( c );
  };
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec A = randomVec( g , 3 + g() % 15 , 0 , 1 + LL( g() % 3 ) );
  CHECK( varie::countArithmetic( A ) == brute( A ) );
  }
 CHECK( varie::countArithmetic( { 1 , 2 , 4 } ) == 0 );
 CHECK( varie::countArithmetic( { 1 , 2 , 3 } ) == 1 );
 CHECK( varie::countArithmetic( { 1 , 2 , 3 , 2 , 1 } ) == 2 );
 CHECK( varie::countArithmetic( Vec( 100 , 7 ) ) == 1 );
 CHECK( varie::countArithmetic( { 5 , 5 , 5 } ) == 1 );
 CHECK( varie::countArithmetic( { 1 , 2 , 3 , 5 , 7 } ) == 2 );  // share 3
 CHECK( varie::countArithmetic( { 1 , 2 , 3 , 4 , 6 , 8 , 10 } ) == 2 );
 CHECK( varie::countArithmetic( { 0 , 1 , 0 , 1 , 0 } ) == 0 );
 CHECK_THROWS( varie::countArithmetic( { 1 , 2 } ) , contract_violation );
 CHECK_THROWS( varie::countArithmetic( {} ) , contract_violation );
 }

// the examples of the text (doubling the vector and Kadane gives 6 on [ 2 ,
// -1 , 2 ]), the n^2 subvectors on 600 random circular vectors, the edge
// cases
ADS_TEST( "varie-20" )
{
 CHECK( varie::circularMaxSum( { 1 , 3 , -3 , 1 , -2 , 4 , 5 } ) == 13 );
 CHECK( varie::circularMaxSum( { 2 , -1 , 2 } ) == 4 );
 CHECK( maxSubarraySum( Vec( { 2 , -1 , 2 , 2 , -1 , 2 } ) ) == 6 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec A = randomVec( g , g() % 15 , -10 , 10 );
  const std::size_t n = A.size();
  long long best = 0;
  for( std::size_t s = 0 ; s < n ; ++s ) {
   long long sum = 0;
   for( std::size_t j = 0 ; j < n ; ++j ) {
    sum += A[ ( s + j ) % n ];
    best = std::max( best , sum );
    }
   }
  CHECK( varie::circularMaxSum( A ) == best );
  CHECK( maxSubarraySum( A ) <= best );
  }
 CHECK( varie::circularMaxSum( Vec() ) == 0 );
 CHECK( varie::circularMaxSum( { -5 } ) == 0 );
 CHECK( varie::circularMaxSum( Vec( 6 , -1 ) ) == 0 );
 CHECK( varie::circularMaxSum( Vec( 6 , 3 ) ) == 18 );
 CHECK( varie::circularMaxSum( { 7 } ) == 7 );
 CHECK( varie::circularMaxSum( { 0 , 0 } ) == 0 );
 CHECK( varie::circularMaxSum( { 5 , -9 , 5 } ) == 10 );   // across the end
 CHECK( varie::circularMaxSum( { -1 , 5 , -1 } ) == 5 );
 CHECK( varie::circularMaxSum( { 4 , -1 , 4 , -1 } ) == 7 );
 }

// the example of the text, every pair on 600 random vectors, the edge
// cases
ADS_TEST( "varie-21" )
{
 CHECK( varie::maxDistance( { 1 , 2 , 3 , 2 , 1 , 4 } ) == 4 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const Vec A = randomVec( g , g() % 20 , 0 , 1 + LL( g() % 12 ) );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; j < A.size() ; ++j )
    if( A[ i ] == A[ j ] ) best = std::max( best , j - i );
  CHECK( varie::maxDistance( A ) == best );
  }
 CHECK( varie::maxDistance( Vec() ) == 0 );
 CHECK( varie::maxDistance( { 4 } ) == 0 );
 CHECK( varie::maxDistance( { 1 , 2 , 3 } ) == 0 );
 CHECK( varie::maxDistance( Vec( 50 , 9 ) ) == 49 );
 const long long M = std::numeric_limits< long long >::max();
 CHECK( varie::maxDistance( { M , -M , M } ) == 2 );
 CHECK( varie::maxDistance( { 3 , 3 } ) == 1 );
 CHECK( varie::maxDistance( { 1 , 2 , 1 , 2 } ) == 2 );    // two values
 CHECK( varie::maxDistance( { 5 , 1 , 2 , 3 , 4 , 5 } ) == 5 );
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
/*----------------------- End File exam_greedy.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
