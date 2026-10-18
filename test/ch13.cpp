/*--------------------------------------------------------------------------*/
/*----------------------------- File ch13.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 13, Programmazione dinamica: the binomial coefficients,
 * the approximate string matching on Figura 13.2 and the edit distance,
 * the weighted intervals, the chain of matrices, Fibonacci in constant
 * space, Floyd-Warshall on Figura 13.3, the knapsack by tables and by
 * memoization, the longest common subsequence and diff on Figura 13.4,
 * each against an enumeration, and the exercises 13.1 to 13.13, each a
 * test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/numeric/MatrixChain.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/string/Alignment.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/WordBreak.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;
using Lines = std::vector< std::string >;

/// a random string of n characters of an alphabet of k letters
std::string random_string( std::size_t n , int k , std::mt19937 & g )
{
 std::string s;
 for( std::size_t i = 0 ; i < n ; ++i )
  s += char( 'a' + g() % k );
 return( s );
 }

/// the weighted oriented graph of n nodes and of the edges ( u , v , w )
ListGraph< long > make(
           int n , const std::vector< std::tuple< int , int , long > > & E )
{
 ListGraph< long > g( n , true );
 for( auto [ u , v , w ] : E )
  g.insertEdge( u , v , w );
 return( g );
 }

constexpr long INFL = std::numeric_limits< long >::max();

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the binomial coefficients: the recursion makes 2 C( n , k ) - 1 calls,
// the triangle of Tartaglia fills Theta( n^2 ) places; n = 0, the largest
// n = 67 whose C( 67 , 33 ) fits in 64 bits, and the contracts past them
ADS_TEST( "sec13.binomial" )
{
 const auto C = tartaglia( 30 );
 for( int n = 0 ; n <= 20 ; ++n )
  for( int k = 0 ; k <= n ; ++k ) {
   long calls = 0;
   CHECK( binomial( n , k , &calls ) == C[ n ][ k ] );
   CHECK( calls == 2 * long( C[ n ][ k ] ) - 1 );
   }
 CHECK( C[ 30 ][ 15 ] == 155117520ull );
 // the bounds: the empty triangle of n = 0, the largest one, n = 67, its
 // largest entry near 2^64 and symmetric, and the contracts past them
 CHECK( tartaglia( 0 ) == std::vector< std::vector< std::uint64_t > >(
                                                              { { 1 } } ) );
 const auto B = tartaglia( 67 );
 CHECK( B[ 67 ][ 33 ] == 14226520737620288370ull && B[ 67 ][ 34 ] ==
        B[ 67 ][ 33 ] && B[ 67 ][ 0 ] == 1 && B[ 67 ][ 67 ] == 1 );
 CHECK( binomial( 67 , 0 ) == 1 && binomial( 67 , 67 ) == 1 &&
        binomial( 67 , 1 ) == 67 && binomial( 67 , 66 ) == 67 );
 CHECK_THROWS( binomial( 3 , 4 ) , std::logic_error );
 CHECK_THROWS( binomial( 3 , -1 ) , std::logic_error );
 CHECK_THROWS( binomial( 68 , 0 ) , std::logic_error );
 CHECK_THROWS( tartaglia( -1 ) , std::logic_error );
 CHECK_THROWS( tartaglia( 68 ) , std::logic_error );
 }

// Figura 13.2: T = ababa, P = bab, an occurrence without errors ends in the
// place 3 of T (4 counting from 1, as the text does); the example of the
// text, an occurrence of "unesempio" with 2 errors from the place 7 of
// "questoèunoscempio"; the edit distance of google and yahoo is 6; the
// empty pattern and text, the pattern longer than the text or equal to
// it, a letter not in it, several exact occurrences, occurrence() at
// j = 0 and its contract
ADS_TEST( "sec13.1.matching" )
{
 const std::string P = "bab" , T = "ababa";
 const Table fig = { { 0 , 0 , 0 , 0 , 0 , 0 } , { 1 , 1 , 0 , 1 , 0 , 1 } ,
                     { 2 , 1 , 1 , 0 , 1 , 0 } , { 3 , 2 , 1 , 1 , 0 , 1 } };
 CHECK( matchingTable( P , T ) == fig );
 CHECK( stringMatching( P , T ) == std::pair( 3L , 0 ) );
 const std::u32string Q = U"unesempio" , U = U"questoèunoscempio";
 const auto [ end , k ] = stringMatching( Q , U );
 CHECK( k == 2 );
 CHECK( occurrence( matchingTable( Q , U ) , Q , U , end + 1 ) == 7 );
 const std::string google = "google" , yahoo = "yahoo";
 CHECK( editDistance( google , yahoo ) == 6 );
 // the empty strings: no text, ( -1 , m ); the empty pattern occurs with
 // no error at the first place; the distance from "" is the length
 const std::string E , abc = "abc";
 CHECK( stringMatching( abc , E ) == std::pair( -1L , 3 ) );
 CHECK( stringMatching( E , E ) == std::pair( -1L , 0 ) );
 CHECK( stringMatching( E , abc ) == std::pair( 0L , 0 ) );
 CHECK( stringMatchingSingleRow( abc , E ) == 3 );
 CHECK( stringMatchingSingleRow( E , abc ) == 0 );
 CHECK( editDistance( E , E ) == 0 && editDistance( E , abc ) == 3 &&
        editDistance( abc , E ) == 3 );
 // the pattern longer than the text, equal to it, its reverse, a letter
 // never in it, the first of several exact occurrences
 CHECK( stringMatching( std::string( "abcab" ) , std::string( "ab" ) ) ==
        std::pair( 1L , 3 ) );
 CHECK( stringMatching( abc , abc ) == std::pair( 2L , 0 ) );
 CHECK( editDistance( abc , std::string( "cba" ) ) == 2 );
 CHECK( stringMatching( std::string( "z" ) , abc ).second == 1 );
 CHECK( stringMatching( std::string( "ab" ) , std::string( "xabyab" ) ) ==
        std::pair( 2L , 0 ) );
 // occurrence() at j = 0, and its contract on the table and on j
 const auto DA = matchingTable( abc , std::string( "xx" ) );
 CHECK( occurrence( DA , abc , std::string( "xx" ) , 0 ) == 0 );
 CHECK_THROWS( occurrence( DA , abc , std::string( "xx" ) , 3 ) ,
               std::logic_error );
 CHECK_THROWS( occurrence( DA , abc , std::string( "xxx" ) , 1 ) ,
               std::logic_error );
 CHECK_THROWS( occurrence( DA , std::string( "ab" ) , std::string( "xx" ) ,
                           1 ) , std::logic_error );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const auto p = random_string( g() % 8 , 3 , g ) ,
             s = random_string( g() % 20 , 3 , g );
  const auto D = matchingTable( p , s );
  int best = int( p.size() );
  for( std::size_t j = 1 ; j <= s.size() ; ++j )
   best = std::min( best , D[ p.size() ][ j ] );
  CHECK( stringMatchingSingleRow( p , s ) == best );
  const auto E = matchingTable( p , s , true );
  CHECK( editDistance( p , s ) == E[ p.size() ][ s.size() ] );
  CHECK( editDistance( p , s ) == editDistance( s , p ) );
  }
 }

// the weighted intervals against all the subsets; none, one, touching ends,
// all equal, weights 0, nested, the extremes of long, and the contract
ADS_TEST( "sec13.2.intervals" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = int( g() % 11 );
  std::vector< WeightedInterval< long > > I( n );
  for( auto & x : I ) {
   x.a = long( g() % 20 );
   x.b = x.a + 1 + long( g() % 6 );
   x.w = long( g() % 10 );
   }
  const auto [ S , w ] = maxinterval( I );
  long best = 0;
  for( int m = 0 ; m < ( 1 << n ) ; ++m ) {
   long s = 0;
   bool ok = true;
   for( int i = 0 ; i < n && ok ; ++i )
    if( m >> i & 1 ) {
     s += I[ i ].w;
     for( int j = i + 1 ; j < n ; ++j )
      if( ( m >> j & 1 ) && I[ i ].a < I[ j ].b && I[ j ].a < I[ i ].b )
       ok = false;
     }
   if( ok ) best = std::max( best , s );
   }
  long sw = 0;
  for( auto i : S ) sw += I[ i ].w;
  CHECK( w == best && sw == best );
  for( std::size_t x = 0 ; x < S.size() ; ++x )            // disjoint
   for( std::size_t y = x + 1 ; y < S.size() ; ++y )
    CHECK( I[ S[ x ] ].b <= I[ S[ y ] ].a || I[ S[ y ] ].b <= I[ S[ x ] ].a );
  }
 using WI = std::vector< WeightedInterval< long > >;
 using Places = std::vector< std::size_t >;
 // none; one; touching ends, compatible since open on the right; all
 // equal, one taken; weights 0, none taken; nested, the inner ones
 // heavier together; the extremes of long
 CHECK( maxinterval( WI() ) == std::pair( Places() , 0L ) );
 CHECK( maxinterval( WI( { { 3 , 4 , 7 } } ) ) == std::pair( Places( { 0 } ) ,
                                                              7L ) );
 CHECK( maxinterval( WI( { { 0 , 1 , 1 } , { 1 , 2 , 1 } , { 2 , 3 , 1 } } ) )
        == std::pair( Places( { 0 , 1 , 2 } ) , 3L ) );
 const auto [ Se , we ] = maxinterval( WI( 5 , { 2 , 6 , 4 } ) );
 CHECK( Se.size() == 1 && we == 4 );
 CHECK( maxinterval( WI( { { 0 , 5 , 0 } , { 6 , 9 , 0 } } ) ) ==
        std::pair( Places() , 0L ) );
 CHECK( maxinterval( WI( { { 0 , 10 , 5 } , { 1 , 3 , 3 } , { 4 , 6 , 3 } } ) )
        == std::pair( Places( { 1 , 2 } ) , 6L ) );
 const long LO = std::numeric_limits< long >::min() , HI = INFL;
 CHECK( maxinterval( WI( { { LO , 0 , 2 } , { 0 , HI , 3 } ,
                           { LO , HI , 4 } } ) ) ==
        std::pair( Places( { 0 , 1 } ) , 5L ) );
 // an empty or reversed interval, and a negative weight, break the contract
 CHECK_THROWS( maxinterval( WI( { { 2 , 2 , 1 } } ) ) , std::logic_error );
 CHECK_THROWS( maxinterval( WI( { { 3 , 2 , 1 } } ) ) , std::logic_error );
 CHECK_THROWS( maxinterval( WI( { { 0 , 1 , -1 } } ) ) , std::logic_error );
 }

// the example of the text, 30 x 5, 5 x 25, 25 x 7: 9000 multiplications
// with ( A0 A1 ) A2, 1925 with A0 ( A1 A2 ); the six matrices of the
// classic example of Cormen et al. cost 15125; against the recursion on
// all the parenthesizations; no matrix, one, two, all 1 x 1, and the
// contracts on the sizes and on the range
ADS_TEST( "sec13.3.matrix-chain" )
{
 const auto [ c3 , S3 ] = matrixChain( { 30 , 5 , 25 , 7 } );
 CHECK( c3 == 1925 && parenthesization( S3 , 0 , 2 ) == "(A0·(A1·A2))" );
 const auto [ c6 , S6 ] = matrixChain( { 30 , 35 , 15 , 5 , 10 , 20 , 25 } );
 CHECK( c6 == 15125 );
 CHECK( parenthesization( S6 , 0 , 5 ) == "((A0·(A1·A2))·((A3·A4)·A5))" );
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Longs c( 2 + g() % 7 );
  for( auto & x : c ) x = 1 + long( g() % 20 );
  std::function< long( std::size_t , std::size_t ) > rec =
   [ & ]( std::size_t i , std::size_t j ) {
    if( i == j ) return( 0L );
    long b = INFL;
    for( std::size_t k = i ; k < j ; ++k )
     b = std::min( b , rec( i , k ) + rec( k + 1 , j ) +
                       c[ i ] * c[ k + 1 ] * c[ j + 1 ] );
    return( b );
    };
  CHECK( matrixChain( c ).first == rec( 0 , c.size() - 2 ) );
  }
 // no matrix (cost 0, no split), one (cost 0, "A0"), two (no choice),
 // all 1 x 1 (cost n - 1), and the ties between the splits
 const auto [ c0 , S0 ] = matrixChain( { 7 } );
 CHECK( c0 == 0 && S0.empty() );
 const auto [ c1 , S1 ] = matrixChain( { 7 , 9 } );
 CHECK( c1 == 0 && parenthesization( S1 , 0 , 0 ) == "A0" );
 CHECK( matrixChain( { 2 , 3 , 4 } ).first == 24 );
 const auto [ cu , Su ] = matrixChain( Longs( 6 , 1 ) );
 CHECK( cu == 4 && parenthesization( Su , 0 , 4 ).size() > 0 );
 // the contracts: no size, a size 0 or negative, a range outside S
 CHECK_THROWS( matrixChain( Longs() ) , std::logic_error );
 CHECK_THROWS( matrixChain( { 3 , 0 , 2 } ) , std::logic_error );
 CHECK_THROWS( matrixChain( { 3 , -1 , 2 } ) , std::logic_error );
 CHECK_THROWS( parenthesization( S3 , 2 , 1 ) , std::logic_error );
 CHECK_THROWS( parenthesization( S3 , 0 , 3 ) , std::logic_error );
 }

// Fibonacci, F_0 = F_1 = 1, with a table and in constant space; F_92, the
// last in 64 bits, and the contracts on 93 and -1
ADS_TEST( "sec13.4.fibonacci" )
{
 const std::vector< std::uint64_t > F = { 1 , 1 , 2 , 3 , 5 , 8 , 13 , 21 ,
                                          34 , 55 , 89 };
 for( int n = 0 ; n <= 10 ; ++n )
  CHECK( fibonacci( n ) == F[ n ] && fibonacciTable( n ) == F[ n ] );
 CHECK( fibonacci( 90 ) == fibonacciTable( 90 ) );
 // the last that fits in 64 bits, F_92, and the contracts around
 CHECK( fibonacci( 92 ) == 12200160415121876738ull &&
        fibonacciTable( 92 ) == 12200160415121876738ull );
 CHECK_THROWS( fibonacci( 93 ) , std::logic_error );
 CHECK_THROWS( fibonacciTable( 93 ) , std::logic_error );
 CHECK_THROWS( fibonacci( -1 ) , std::logic_error );
 CHECK_THROWS( fibonacciTable( -1 ) , std::logic_error );
 }

// Figura 13.3: the final matrices d and p; in the figure p[ 1 ][ 4 ] is 2
// (counting from 1), but d[ 1 ][ 4 ] = 7 comes from the path 1 , 3 , 4,
// and so p[ 1 ][ 4 ] = 3, as it is after k = 3; against Johnson from each
// node, and the negative cycles against Bellman-Ford-Moore; no node, one,
// no edge, a cycle of cost 0 and of cost -1, an unoriented graph, and a
// deleted node, whose row and column stay infinite
ADS_TEST( "sec13.5.floyd-warshall" )
{
 const auto g = make( 4 , { { 0 , 1 , 5 } , { 0 , 2 , 2 } , { 1 , 2 , 3 } ,
                            { 1 , 3 , 4 } , { 2 , 3 , 5 } , { 3 , 1 , -3 } } );
 const auto R = floydWarshall( g );
 const std::vector< Longs > d = { { 0 , 4 , 2 , 7 } , { INFL , 0 , 3 , 4 } ,
                                  { INFL , 2 , 0 , 5 } ,
                                  { INFL , -3 , 0 , 0 } };
 const std::vector< Ints > p = { { NIL , 3 , 0 , 2 } , { NIL , NIL , 1 , 1 } ,
                                 { NIL , 3 , NIL , 2 } ,
                                 { NIL , 3 , 1 , NIL } };
 CHECK( R.d == d && R.p == p && ! R.negativeCycle );
 std::mt19937 gen( 4 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 15;
  ListGraph< long > h( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v && gen() % 5 == 0 )
     h.insertEdge( u , v , long( gen() % 30 ) - ( t % 2 ? 5 : 0 ) );
  const auto F = floydWarshall( h );
  bool neg = false;
  for( int r = 0 ; r < n ; ++r )
   neg = neg || bellmanFordMoore( h , r ).negativeCycle;
  CHECK( F.negativeCycle == neg );
  if( ! neg )
   for( int r = 0 ; r < n ; ++r )
    CHECK( F.d[ r ] == bellmanFordMoore( h , r ).d );
  }
 // no node, one node, no edge (all infinite off the diagonal), a cycle of
 // two of cost 0 (no negative cycle) and of cost -1 (one), unoriented
 const auto Z = floydWarshall( ListGraph< long >( 0 , true ) );
 CHECK( Z.d.empty() && Z.p.empty() && ! Z.negativeCycle );
 const auto O = floydWarshall( ListGraph< long >( 1 , true ) );
 CHECK( O.d == std::vector< Longs >( { { 0 } } ) && O.p[ 0 ][ 0 ] == NIL );
 const auto N = floydWarshall( ListGraph< long >( 3 , true ) );
 for( int u = 0 ; u < 3 ; ++u )
  for( int v = 0 ; v < 3 ; ++v )
   CHECK( N.d[ u ][ v ] == ( u == v ? 0 : INFL ) && N.p[ u ][ v ] == NIL );
 const auto C0 = floydWarshall( make( 2 , { { 0 , 1 , 4 } ,
                                            { 1 , 0 , -4 } } ) );
 CHECK( ! C0.negativeCycle && C0.d[ 1 ][ 0 ] == -4 && C0.d[ 0 ][ 0 ] == 0 );
 CHECK( floydWarshall( make( 2 , { { 0 , 1 , 4 } , { 1 , 0 , -5 } } ) )
        .negativeCycle );
 ListGraph< long > un( 3 , false );
 un.insertEdge( 0 , 1 , 2 ); un.insertEdge( 1 , 2 , 3 );
 const auto U = floydWarshall( un );
 CHECK( U.d[ 2 ][ 0 ] == 5 && U.d[ 0 ][ 2 ] == 5 && U.p[ 2 ][ 0 ] == 1 );
 // a deleted node keeps its row and column at infinity, NIL
 auto hole = make( 3 , { { 0 , 1 , 1 } , { 1 , 2 , 1 } , { 0 , 2 , 5 } } );
 hole.deleteNode( 1 );
 const auto Hd = floydWarshall( hole );
 CHECK( Hd.d[ 0 ][ 2 ] == 5 && Hd.d[ 0 ][ 1 ] == INFL &&
        Hd.d[ 1 ][ 1 ] == INFL && Hd.p[ 0 ][ 1 ] == NIL );
 }

// the knapsack: the table and the memoization against all the subsets;
// with large volumes the memoization solves far fewer subproblems than n C;
// no object, capacity 0, none or all fitting, the largest capacity for
// zaino(), and the contracts
ADS_TEST( "sec13.6.knapsack" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = int( g() % 12 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + g() % 20;
   v[ i ] = g() % 15;                       // 0 too: free objects
   }
  const long C = long( g() % 40 );
  long best = 0;
  for( int m = 0 ; m < ( 1 << n ) ; ++m ) {
   long pp = 0 , vv = 0;
   for( int i = 0 ; i < n ; ++i )
    if( m >> i & 1 ) {
     pp += p[ i ];
     vv += v[ i ];
     }
   if( vv <= C ) best = std::max( best , pp );
   }
  const auto [ val , S ] = knapsack( p , v , C );
  long sp = 0 , sv = 0;
  for( auto i : S ) { sp += p[ i ]; sv += v[ i ]; }
  CHECK( val == best && sp == best && sv <= C );
  CHECK( zaino( p , v , C ) == best );
  }
 CHECK( knapsack( { 5 , 1 } , { 0 , 3 } , 3 ).first == 6 &&   // a free
        zaino( { 5 , 1 } , { 0 , 3 } , 3 ) == 6 &&                 // object
        knapsack( { 5 } , { 0 } , 0 ).first == 5 );
 Longs p( 20 , 1 ) , v( 20 );
 for( int i = 0 ; i < 20 ; ++i ) v[ i ] = 1000 * ( i + 1 );
 long solved = 0;
 zaino( p , v , 100000 , &solved );
 CHECK( solved < 20 * 100000 / 100 );
 // no object, the capacity 0, no object fits, all fit, one exactly fits
 using Places = std::vector< std::size_t >;
 CHECK( knapsack( {} , {} , 5 ) == std::pair( 0L , Places() ) &&
        zaino( {} , {} , 5 ) == 0 );
 CHECK( knapsack( { 3 , 4 } , { 1 , 2 } , 0 ) == std::pair( 0L , Places() )
        && zaino( { 3 , 4 } , { 1 , 2 } , 0 ) == 0 );
 CHECK( knapsack( { 3 , 4 } , { 6 , 7 } , 5 ).first == 0 &&
        zaino( { 3 , 4 } , { 6 , 7 } , 5 ) == 0 );
 CHECK( knapsack( { 3 , 4 } , { 2 , 3 } , 5 ) ==
        std::pair( 7L , Places( { 0 , 1 } ) ) );
 CHECK( zaino( { 3 , 4 } , { 2 , 3 } , 100 ) == 7 );
 CHECK( knapsack( { 9 } , { 5 } , 5 ).first == 9 &&
        knapsack( { 9 } , { 5 } , 4 ).first == 0 );
 // zaino() with the largest capacity: the key of a subproblem must not
 // overflow (it was i ( C + 1 ) + c in one table)
 const long M = INFL;
 CHECK( zaino( { 3 , 4 , 5 } , { M / 2 , M / 2 , 1 } , M ) == 12 );
 CHECK( zaino( { 3 , 4 , 5 } , { M / 2 , M / 2 , 2 } , M ) == 9 );
 CHECK( zaino( { 3 , 4 } , { M , M } , M ) == 4 );
 // the contracts: lengths that differ, a negative profit, volume or
 // capacity
 CHECK_THROWS( knapsack( { 1 } , {} , 3 ) , std::logic_error );
 CHECK_THROWS( zaino( { 1 } , {} , 3 ) , std::logic_error );
 CHECK_THROWS( knapsack( { -1 } , { 1 } , 3 ) , std::logic_error );
 CHECK_THROWS( zaino( { 1 } , { -1 } , 3 ) , std::logic_error );
 CHECK_THROWS( knapsack( { 1 } , { 1 } , -1 ) , std::logic_error );
 CHECK_THROWS( zaino( { 1 } , { 1 } , -1 ) , std::logic_error );
 }

// the LCS: the table, the memoization, the subsequence, against the
// enumeration of the subsequences of short strings; empty strings, equal,
// disjoint, reversed, one inside the other
ADS_TEST( "sec13.7.lcs" )
{
 const std::string clrs1 = "ABCBDAB" , clrs2 = "BDCABA";
 CHECK( lcs( clrs1 , clrs2 ).size() == 4 );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto P = random_string( g() % 9 , 3 , g ) ,
             T = random_string( g() % 9 , 3 , g );
  std::size_t best = 0;
  for( unsigned m = 0 ; m < ( 1u << P.size() ) ; ++m ) {
   std::string x;
   for( std::size_t i = 0 ; i < P.size() ; ++i ) if( m >> i & 1 ) x += P[ i ];
   std::size_t k = 0;                                   // x in T?
   for( char c : T ) if( k < x.size() && x[ k ] == c ) ++k;
   if( k == x.size() ) best = std::max( best , x.size() );
   }
  const auto L = lcs( P , T );
  CHECK( L.size() == best && int( best ) == lcsMemo( P , T ) );
  CHECK( lcsTable( P , T )[ P.size() ][ T.size() ] == int( best ) );
  std::size_t a = 0 , b = 0;                            // L in P and in T
  for( char c : P ) if( a < L.size() && L[ a ] == c ) ++a;
  for( char c : T ) if( b < L.size() && L[ b ] == c ) ++b;
  CHECK( a == L.size() && b == L.size() );
  }
 // both empty, one empty, equal, no letter in common, one the reverse of
 // the other, one inside the other
 const std::string E , abc = "abc";
 for( const auto & [ P , T , r ] : std::vector< std::tuple< std::string ,
       std::string , int > >( { { E , E , 0 } , { E , abc , 0 } ,
       { abc , E , 0 } , { abc , abc , 3 } , { abc , "xyz" , 0 } ,
       { abc , "cba" , 1 } , { "b" , abc , 1 } , { "aaaa" , "aa" , 2 } } ) ) {
  CHECK( int( lcs( P , T ).size() ) == r && lcsMemo( P , T ) == r );
  CHECK( lcsTable( P , T )[ P.size() ][ T.size() ] == r );
  }
 CHECK( lcs( abc , abc ) == abc && lcs( abc , std::string( "xyz" ) ).empty() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the occurrence that ends in T[ j - 1 ]: from where it starts, the edit
// distance of P and T[ start .. j ) is the k of the table; the empty
// pattern, the empty text, P equal to T, P one letter of T
ADS_TEST( "ex13.1" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const auto P = random_string( 1 + g() % 6 , 3 , g ) ,
             T = random_string( 1 + g() % 15 , 3 , g );
  const auto D = matchingTable( P , T );
  for( std::size_t j = 0 ; j <= T.size() ; ++j ) {
   const auto s = occurrence( D , P , T , j );
   CHECK( s <= j );
   CHECK( editDistance( P , T.substr( s , j - s ) ) == D[ P.size() ][ j ] );
   }
  }
 // the empty pattern starts where it ends, P against the empty text starts
 // at 0, P equal to T starts at 0, P a letter of T starts at that letter
 const std::string E , abc = "abc";
 CHECK( occurrence( matchingTable( E , abc ) , E , abc , 2 ) == 2 );
 CHECK( occurrence( matchingTable( abc , E ) , abc , E , 0 ) == 0 );
 CHECK( occurrence( matchingTable( abc , abc ) , abc , abc , 3 ) == 0 );
 const std::string b = "b";
 CHECK( occurrence( matchingTable( b , abc ) , b , abc , 2 ) == 1 );
 }

// the longest common substring against all the pairs of substrings; empty
// strings, nothing in common, equal, a suffix, all one letter
ADS_TEST( "ex13.2" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto P = random_string( g() % 12 , 2 , g ) ,
             T = random_string( g() % 12 , 2 , g );
  std::size_t best = 0;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   for( std::size_t l = 1 ; i + l <= P.size() ; ++l )
    if( T.find( P.substr( i , l ) ) != std::string::npos )
     best = std::max( best , l );
  const auto [ len , ei , ej ] = longestCommonSubstring( P , T );
  CHECK( len == best );
  CHECK( P.substr( ei - len , len ) == T.substr( ej - len , len ) );
  }
 // empty strings, no letter in common, equal strings, one inside the
 // other at its end, all one letter
 const std::string E , abc = "abc";
 using Z3 = std::tuple< std::size_t , std::size_t , std::size_t >;
 CHECK( longestCommonSubstring( E , E ) == Z3( 0 , 0 , 0 ) );
 CHECK( longestCommonSubstring( E , abc ) == Z3( 0 , 0 , 0 ) );
 CHECK( longestCommonSubstring( abc , std::string( "xy" ) ) ==
        Z3( 0 , 0 , 0 ) );
 const auto [ l0 , i0 , j0 ] = longestCommonSubstring( abc , abc );
 CHECK( l0 == 3 && i0 == 3 && j0 == 3 );
 const auto [ l1 , i1 , j1 ] = longestCommonSubstring( std::string( "xxbc" ) ,
                                                       abc );
 CHECK( l1 == 2 && i1 == 4 && j1 == 3 );
 CHECK( std::get< 0 >( longestCommonSubstring( std::string( 7 , 'a' ) ,
                                               std::string( 4 , 'a' ) ) )
        == 4 );
 }

// Bellman-Ford-Moore as dynamic programming: d^{k+1}_v = min{ d^k_v ,
// min_h d^k_h + w( h , v ) }, n - 1 rounds of all the edges, O( n m )
ADS_TEST( "ex13.3" )
{
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 12;
  ListGraph< long > h( n , true );
  Ints pi( n );
  for( int i = 0 ; i < n ; ++i ) pi[ i ] = i;
  std::shuffle( pi.begin() , pi.end() , g );
  for( int i = 0 ; i < n ; ++i )                  // acyclic, so no negative
   for( int j = i + 1 ; j < n ; ++j )             // cycle
    if( g() % 3 == 0 )
     h.insertEdge( pi[ i ] , pi[ j ] , long( g() % 21 ) - 10 );
  Longs d( n , INFL );
  d[ 0 ] = 0;
  for( int k = 1 ; k < n ; ++k ) {
   Longs e = d;
   for( int u = 0 ; u < n ; ++u )
    if( d[ u ] < INFL )
     h.forEachEdge( u , [ & ]( int v , long w ) {
      e[ v ] = std::min( e[ v ] , d[ u ] + w ); } );
   d = e;
   }
  CHECK( d == bellmanFordMoore( h , 0 ).d );
  }
 }

// on a graph with ( u , v ) only for u < v, d_v = min_{h < v} d_h + w( h , v )
// in the order of the nodes, O( n + m )
ADS_TEST( "ex13.4" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 15;
  ListGraph< long > h( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 3 == 0 )
     h.insertEdge( u , v , long( g() % 21 ) - 10 );
  Longs d( n , INFL );
  d[ 0 ] = 0;
  for( int v = 1 ; v < n ; ++v )
   for( int u = 0 ; u < v ; ++u )
    if( d[ u ] < INFL )
     if( auto w = h.weight( u , v ) )
      d[ v ] = std::min( d[ v ] , d[ u ] + *w );
  CHECK( d == dagShortestPaths( h , 0 ).d );
  }
 }

// the insertion of an edge updates all the distances in O( n^2 ), as
// Floyd-Warshall from scratch; one that closes a negative cycle throws;
// an edge no better than the distance, a cycle of cost exactly 0, and the
// contract on a loop and on a node out of range
ADS_TEST( "ex13.5" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 20 ; ++t ) {
  const int n = 10;
  ListGraph< long > h( n , true );
  auto R = floydWarshall( h );
  for( int e = 0 ; e < 40 ; ++e ) {
   const int a = int( g() % n ) , b = int( g() % n );
   if( a == b || h.hasEdge( a , b ) ) continue;
   const long w = long( g() % 20 ) - 3;
   auto H = h;
   H.insertEdge( a , b , w );
   const auto F = floydWarshall( H );
   if( F.negativeCycle ) {
    CHECK_THROWS( addEdge( R , a , b , w ) , contract_violation );
    continue;
    }
   addEdge( R , a , b , w );
   h = H;
   CHECK( R.d == F.d );
   }
  }
 // an edge no better than the distance changes nothing; one that closes a
 // cycle of cost exactly 0 is allowed; a loop, a node out of range and a
 // negative cycle break the contract
 auto R = floydWarshall( make( 3 , { { 0 , 1 , 2 } , { 1 , 2 , 2 } } ) );
 const auto before = R;
 addEdge( R , 0 , 2 , 4L );
 CHECK( R.d == before.d && R.p == before.p );
 addEdge( R , 2 , 0 , -4L );
 CHECK( R.d[ 2 ][ 0 ] == -4 && R.d[ 1 ][ 0 ] == -2 && R.d[ 0 ][ 0 ] == 0 );
 CHECK_THROWS( addEdge( R , 1 , 1 , 0L ) , contract_violation );
 CHECK_THROWS( addEdge( R , 0 , 3 , 1L ) , contract_violation );
 CHECK_THROWS( addEdge( R , -1 , 0 , 1L ) , contract_violation );
 CHECK_THROWS( addEdge( R , 1 , 0 , -3L ) , contract_violation );
 }

// the longest increasing subsequence; the example of the text, 3 , 4 , 5 ,
// 10 in 9 , 15 , 3 , 6 , 4 , 2 , 5 , 10 , 3, has length 4; empty, one,
// all equal, decreasing, increasing, the extremes of int
ADS_TEST( "ex13.6" )
{
 const Ints L = { 9 , 15 , 3 , 6 , 4 , 2 , 5 , 10 , 3 };
 CHECK( lis( L ) == Ints( { 3 , 4 , 5 , 10 } ) && lisLength( L ) == 4 );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints A( g() % 13 );
  for( auto & a : A ) a = int( g() % 20 );
  std::size_t best = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m ) {
   Ints x;
   for( std::size_t i = 0 ; i < A.size() ; ++i )
    if( m >> i & 1 )
     x.push_back( A[ i ] );
   if( std::adjacent_find( x.begin() , x.end() ,
                           std::greater_equal<>() ) == x.end() )
    best = std::max( best , x.size() );
   }
  const auto s = lis( A );
  CHECK( s.size() == best && lisLength( A ) == best );
  CHECK( std::adjacent_find( s.begin() , s.end() ,
                             std::greater_equal<>() ) == s.end() );
  }
 // empty, one, all equal (strictly increasing: 1), decreasing (1),
 // increasing (all), the extremes of int
 const int MI = std::numeric_limits< int >::min() ,
           MA = std::numeric_limits< int >::max();
 CHECK( lis( Ints() ).empty() && lisLength( Ints() ) == 0 );
 CHECK( lis( Ints( { 4 } ) ) == Ints( { 4 } ) );
 CHECK( lis( Ints( 9 , 3 ) ) == Ints( { 3 } ) && lisLength( Ints( 9 , 3 ) )
        == 1 );
 CHECK( lis( Ints( { 5 , 4 , 3 } ) ).size() == 1 &&
        lisLength( Ints( { 5 , 4 , 3 } ) ) == 1 );
 CHECK( lis( Ints( { 1 , 2 , 3 } ) ) == Ints( { 1 , 2 , 3 } ) );
 CHECK( lis( Ints( { MA , MI , 0 , MA } ) ) == Ints( { MI , 0 , MA } ) );
 }

/// the score of an alignment of two strings of equal length, ' ' a space
int score( const std::string & a , const std::string & b )
{
 int s = 0;
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  s += ( a[ i ] == ' ' || b[ i ] == ' ' ) ? -2 : a[ i ] == b[ i ] ? 1 : -1;
 return( s );
 }

// the global alignment: the example of the text scores 6; the alignment
// read back has that score, and no space in front of a space; empty
// strings and equal ones
ADS_TEST( "ex13.7" )
{
 const std::string P = "GACGGATTAG" , T = "GATCGGAATAG";
 CHECK( alignmentScore( P , T , Alignment::GLOBAL ) == 6 );
 CHECK( score( "GA CGGATTAG" , "GATCGGAATAG" ) == 6 );
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto p = random_string( g() % 10 , 4 , g ) ,
             q = random_string( g() % 10 , 4 , g );
  const auto [ a , b ] = globalAlignment( p , q );
  CHECK( a.size() == b.size() );
  for( std::size_t i = 0 ; i < a.size() ; ++i )
   CHECK( a[ i ] != ' ' || b[ i ] != ' ' );
  CHECK( score( a , b ) == alignmentScore( p , q , Alignment::GLOBAL ) );
  }
 // both empty (0), one empty (all spaces, -2 each), equal (the length)
 const std::string E , abc = "abc";
 CHECK( alignmentScore( E , E , Alignment::GLOBAL ) == 0 );
 CHECK( globalAlignment( E , E ) == std::pair( E , E ) );
 CHECK( alignmentScore( abc , E , Alignment::GLOBAL ) == -6 );
 CHECK( globalAlignment( E , abc ) == std::pair( std::string( "   " ) ,
                                                 abc ) );
 CHECK( alignmentScore( abc , abc , Alignment::GLOBAL ) == 3 &&
        globalAlignment( abc , abc ) == std::pair( abc , abc ) );
 }

// the semiglobal alignment: the spaces at the ends are free, so a string
// inside the other one scores its length; an empty string, a suffix over
// a prefix, no letter in common
ADS_TEST( "ex13.8" )
{
 CHECK( alignmentScore( std::string( "CGGA" ) , std::string( "GATCGGAATAG" ) ,
                        Alignment::SEMIGLOBAL ) == 4 );
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto p = random_string( g() % 10 , 4 , g ) ,
             q = random_string( g() % 10 , 4 , g );
  CHECK( alignmentScore( p , q , Alignment::SEMIGLOBAL ) >=
         alignmentScore( p , q , Alignment::GLOBAL ) );
  }
 // an empty string costs nothing, an overlap of a suffix and a prefix
 // scores its length, with no letter in common the best is 0
 const std::string E , abc = "abc";
 CHECK( alignmentScore( E , abc , Alignment::SEMIGLOBAL ) == 0 &&
        alignmentScore( abc , E , Alignment::SEMIGLOBAL ) == 0 );
 CHECK( alignmentScore( std::string( "xxab" ) , abc ,
                        Alignment::SEMIGLOBAL ) == 2 );
 CHECK( alignmentScore( abc , std::string( "xyz" ) ,
                        Alignment::SEMIGLOBAL ) == 0 );
 }

// the local alignment: the best global alignment of two substrings,
// against all the pairs of substrings; empty strings, nothing in common,
// one letter in common
ADS_TEST( "ex13.9" )
{
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const auto p = random_string( g() % 8 , 3 , g ) ,
             q = random_string( g() % 8 , 3 , g );
  int best = 0;
  for( std::size_t i = 0 ; i <= p.size() ; ++i )
   for( std::size_t l = 0 ; i + l <= p.size() ; ++l )
    for( std::size_t j = 0 ; j <= q.size() ; ++j )
     for( std::size_t r = 0 ; j + r <= q.size() ; ++r )
      best = std::max( best , alignmentScore( p.substr( i , l ) ,
                               q.substr( j , r ) , Alignment::GLOBAL ) );
  CHECK( alignmentScore( p , q , Alignment::LOCAL ) == best );
  }
 // empty strings and no letter in common: 0, the empty alignment; one
 // letter in common: 1
 const std::string E , abc = "abc";
 CHECK( alignmentScore( E , E , Alignment::LOCAL ) == 0 );
 CHECK( alignmentScore( abc , E , Alignment::LOCAL ) == 0 );
 CHECK( alignmentScore( abc , std::string( "xyz" ) , Alignment::LOCAL ) == 0 );
 CHECK( alignmentScore( abc , std::string( "xbz" ) , Alignment::LOCAL ) == 1 );
 }

// diff on the lines of Figura 13.4; empty files, equal files
ADS_TEST( "ex13.10" )
{
 const Lines original = { "Questo è il testo originale" ,
  "alcune linee non dovrebbero" , "cambiare mai" , "altre invece vengono" ,
  "rimosse" , "altre vengono aggiunte" };
 const Lines nuovo = { "Questo è il testo nuovo" ,
  "alcune linee non dovrebbero" , "cambiare mai" , "altre invece vengono" ,
  "cancellate" , "altre vengono aggiunte" , "come questa" };
 const auto D = diff( original , nuovo );
 std::vector< std::pair< char , std::string > > fig = {
  { '-' , "Questo è il testo originale" } ,
  { '+' , "Questo è il testo nuovo" } ,
  { ' ' , "alcune linee non dovrebbero" } , { ' ' , "cambiare mai" } ,
  { ' ' , "altre invece vengono" } , { '-' , "rimosse" } ,
  { '+' , "cancellate" } , { ' ' , "altre vengono aggiunte" } ,
  { '+' , "come questa" } };
 CHECK( D == fig );
 // the lines kept and added give the new file, kept and removed the old
 std::mt19937 g( 16 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const auto P = random_string( g() % 10 , 3 , g ) ,
             T = random_string( g() % 10 , 3 , g );
  std::string a , b;
  int common = 0;
  for( auto [ c , x ] : diff( P , T ) ) {
   if( c != '+' ) a += x;
   if( c != '-' ) b += x;
   common += c == ' ';
   }
  CHECK( a == P && b == T && common == lcsMemo( P , T ) );
  }
 // both empty: nothing; from or to the empty file: all added or removed;
 // equal files: all kept
 const Lines none , two = { "x" , "y" };
 using Ops = std::vector< std::pair< char , std::string > >;
 CHECK( diff( none , none ).empty() );
 CHECK( diff( none , two ) == Ops( { { '+' , "x" } , { '+' , "y" } } ) );
 CHECK( diff( two , none ) == Ops( { { '-' , "x" } , { '-' , "y" } } ) );
 CHECK( diff( two , two ) == Ops( { { ' ' , "x" } , { ' ' , "y" } } ) );
 }

// Harry Potter: the requests are weighted intervals of the day, the
// candies the weights
ADS_TEST( "ex13.11" )
{
 const std::vector< WeightedInterval< long > > R = {
  { 9 , 11 , 5 } , { 10 , 12 , 6 } , { 11 , 13 , 5 } , { 14 , 16 , 4 } ,
  { 12 , 15 , 7 } };
 const auto [ S , w ] = maxinterval( R );
 CHECK( w == 14 && S == std::vector< std::size_t >( { 0 , 2 , 3 } ) );
 }

// the subvector of largest sum in O( n ), against the O( n^2 ) scan; empty,
// all negative, all positive, the best at either end, in double
ADS_TEST( "ex13.12" )
{
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs A( g() % 50 );
  for( auto & a : A ) a = long( g() % 21 ) - 10;
  long best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i ) {
   long s = 0;
   for( std::size_t j = i ; j < A.size() ; ++j )
    best = std::max( best , s += A[ j ] );
   }
  CHECK( maxSubarraySum( A ) == best );
  }
 // empty and all negative: the empty subvector, 0; all positive: the whole
 // vector; the best at either end; in double
 CHECK( maxSubarraySum( Longs() ) == 0 );
 CHECK( maxSubarraySum( Longs( { -3 , -1 , -2 } ) ) == 0 );
 CHECK( maxSubarraySum( Longs( { 3 , 1 , 2 } ) ) == 6 );
 CHECK( maxSubarraySum( Longs( { 9 , -10 , 1 , 1 } ) ) == 9 );
 CHECK( maxSubarraySum( Longs( { 1 , 1 , -10 , 9 } ) ) == 9 );
 CHECK( maxSubarraySum( std::vector< double >( { -0.5 , 1.5 , -0.25 ,
                                                 1.0 } ) ) == 2.25 );
 }

// is it Italian? "eraunanottebuiaetempestosa" with a small dictionary, at
// most n ( n + 1 ) / 2 lookups, n w if no word is longer than w; one word,
// one letter repeated, an empty dictionary, the bound around the longest
ADS_TEST( "ex13.13" )
{
 const std::set< std::string > h = { "era" , "un" , "una" , "notte" , "buia" ,
                                     "e" , "tempestosa" , "tempesta" };
 auto lookup = [ & ]( const std::string & w ) { return( h.count( w ) > 0 ); };
 const std::string s = "eraunanottebuiaetempestosa";
 long n1 = 0 , n2 = 0;
 const auto w = wordBreak( s , lookup , std::string::npos , &n1 );
 CHECK( w && w->size() == 6 );
 std::string joined;
 for( const auto & x : *w ) { CHECK( h.count( x ) ); joined += x; }
 CHECK( joined == s );
 CHECK( n1 <= long( s.size() * ( s.size() + 1 ) / 2 ) );
 CHECK( wordBreak( s , lookup , 10 , &n2 ) && n2 <= long( 10 * s.size() ) );
 CHECK( ! wordBreak( std::string( "eraunanottex" ) , lookup ) );
 CHECK( wordBreak( std::string() , lookup )->empty() );
 // the whole string one word, a word of one letter repeated, an empty
 // dictionary, and the bound on the length of the words just below and at
 // that of the longest one
 CHECK( *wordBreak( std::string( "notte" ) , lookup ) == Lines( {
                                                              "notte" } ) );
 CHECK( wordBreak( std::string( "eee" ) , lookup )->size() == 3 );
 auto never = []( const std::string & ) { return( false ); };
 CHECK( ! wordBreak( std::string( "e" ) , never ) );
 CHECK( wordBreak( std::string() , never )->empty() );
 CHECK( ! wordBreak( std::string( "tempestosa" ) , lookup , 9 ) );
 CHECK( wordBreak( std::string( "tempestosa" ) , lookup , 10 ) );
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
/*---------------------------- End File ch13.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
