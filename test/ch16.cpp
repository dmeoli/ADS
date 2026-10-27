/*--------------------------------------------------------------------------*/
/*----------------------------- File ch16.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 16, Backtrack: the side of a point, the convex hull by
 * Graham against the one of O( n^3 ), the subsets of k items, the Sudoku
 * of the reality check, and the exercises 16.1 to 16.8, each a test named
 * after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 16,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#include "ADS/backtrack/Anagrams.h"
#include "ADS/backtrack/Enumeration.h"
#include "ADS/backtrack/KnightTour.h"
#include "ADS/backtrack/NQueens.h"
#include "ADS/backtrack/Sudoku.h"
#include "ADS/geometry/ConvexHull.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Points = std::vector< Point >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// stessaparte(): the points on the line count on both sides; the ends,
// the degenerate line, the symmetries, coordinates of 10^9
ADS_TEST( "sec16.1.same-side" )
{
 const Point a{ 0 , 0 } , b{ 4 , 4 };
 CHECK( sameSide( a , b , { 0 , 1 } , { 1 , 5 } ) );
 CHECK( ! sameSide( a , b , { 0 , 1 } , { 1 , 0 } ) );
 CHECK( sameSide( a , b , { 2 , 2 } , { 1 , 0 } ) );
 CHECK( sameSide( { 1 , 0 } , { 1 , 9 } , { 0 , 3 } , { -5 , -5 } ) );
 // the edges: both points on the line; a point equal to an end; the
 // degenerate line p1 = p2, on which every point lies; the order of p and
 // q, and of p1 and p2, does not matter; coordinates of 10^9
 CHECK( sameSide( a , b , { 1 , 1 } , { -3 , -3 } ) );
 CHECK( sameSide( a , b , a , { 9 , -9 } ) && sameSide( a , b , b , b ) );
 CHECK( sameSide( a , a , { 5 , 1 } , { -5 , 2 } ) );
 for( Point p : { Point{ 0 , 1 } , Point{ 1 , 0 } , Point{ 2 , 2 } } )
  for( Point q : { Point{ 0 , 1 } , Point{ 1 , 0 } , Point{ 2 , 2 } } )
   CHECK( sameSide( a , b , p , q ) == sameSide( a , b , q , p ) &&
          sameSide( a , b , p , q ) == sameSide( b , a , p , q ) );
 const long G = 1000000000;
 CHECK( ! sameSide( { -G , -G } , { G , G } , { -G , G } , { G , -G } ) );
 CHECK( sameSide( { -G , -G } , { G , G } , { -G , G } , { G - 1 , G } ) );
 }

// Graham against the hull of O( n^3 ), on random points with many
// collinear ones; the hull is convex and contains every point; no point,
// one repeated, two, all collinear, points on the sides, coordinates of
// 10^9
ADS_TEST( "sec16.1.hull" )
{
 const Points square = { { 0 , 0 } , { 4 , 0 } , { 4 , 4 } , { 0 , 4 } ,
                         { 2 , 2 } , { 2 , 0 } , { 1 , 3 } };
 CHECK( graham( square ) == Points( { { 4 , 0 } , { 4 , 4 } , { 0 , 4 } ,
                                      { 0 , 0 } } ) );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Points P( 1 + g() % 25 );
  for( auto & p : P ) { p.x = long( g() % 8 ); p.y = long( g() % 8 ); }
  const auto H = graham( P );
  CHECK( H == convexHullBrute( P ) );
  if( H.size() < 3 ) continue;
  for( std::size_t i = 0 ; i < H.size() ; ++i ) {
   const Point & a = H[ i ] , & b = H[ ( i + 1 ) % H.size() ];
   CHECK( cross( a , b , H[ ( i + 2 ) % H.size() ] ) > 0 );     // convex
   for( const Point & p : P ) CHECK( cross( a , b , p ) >= 0 );  // inside
   }
  }
 // the edges: no point; one, repeated; two; all collinear, horizontal,
 // vertical, diagonal, only the two ends; a triangle with points on its
 // sides; coordinates of 10^9, the largest allowed, whose cross products
 // need 63 bits; one coordinate beyond them violates the precondition
 CHECK( graham( {} ).empty() && convexHullBrute( {} ).empty() );
 CHECK( graham( Points( 5 , { 3 , -2 } ) ) == Points( { { 3 , -2 } } ) );
 CHECK( graham( { { 1 , 1 } , { 0 , 0 } , { 1 , 1 } } ) ==
        Points( { { 0 , 0 } , { 1 , 1 } } ) );
 for( Point d : { Point{ 1 , 0 } , Point{ 0 , 1 } , Point{ 1 , 1 } ,
                  Point{ -1 , 1 } } ) {
  Points L;
  for( long i = 0 ; i < 10 ; ++i ) L.push_back( { d.x * ( i * 7 % 10 ) ,
                                                  d.y * ( i * 7 % 10 ) } );
  const auto H = graham( L );
  CHECK( H.size() == 2 && H == convexHullBrute( L ) );
  CHECK( std::find( H.begin() , H.end() , Point{ 0 , 0 } ) != H.end() &&
         std::find( H.begin() , H.end() , Point{ 9 * d.x , 9 * d.y } ) !=
         H.end() );
  }
 CHECK( graham( { { 0 , 0 } , { 2 , 0 } , { 4 , 0 } , { 2 , 2 } , { 3 , 1 } ,
                  { 1 , 1 } , { 0 , 4 } , { 0 , 2 } , { 1 , 0 } } ) ==
        Points( { { 4 , 0 } , { 0 , 4 } , { 0 , 0 } } ) );
 const long G = 1000000000;
 const Points far = { { -G , -G } , { G , -G } , { G , G } , { -G , G } ,
                      { 0 , 0 } , { G , 0 } , { 0 , G - 1 } };
 CHECK( graham( far ) == Points( { { G , -G } , { G , G } , { -G , G } ,
                                   { -G , -G } } ) );
 CHECK( graham( far ) == convexHullBrute( far ) );
 const Points out = { { 0 , 0 } , { 1 , 0 } , { 0 , -G - 1 } };
 CHECK_THROWS( graham( out ) , contract_violation );
 CHECK_THROWS( convexHullBrute( out ) , contract_violation );
 }

// the subsets of k = 2 of n = 4 items, the leaves of figure 16.5, and their
// number C( n , k ) (n! / ( n - k )! in the text counts their orderings);
// k = 0, k = n, k > n, and the precondition
ADS_TEST( "sec16.2.subsets" )
{
 std::vector< std::vector< bool > > S;
 CHECK( subsets( 4 , 2 , [ & ]( const auto & s ) {
         S.push_back( s ); return( false ); } ) == 6 );
 CHECK( S.front() == std::vector< bool >( { true , true , false , false } ) );
 CHECK( S.back() == std::vector< bool >( { false , false , true , true } ) );
 auto C = []( int n , int k ) {
  long c = 1;
  for( int i = 1 ; i <= k ; ++i ) c = c * ( n - k + i ) / i;
  return( c );
  };
 for( int n = 0 ; n <= 12 ; ++n )
  for( int k = 0 ; k <= n ; ++k )
   CHECK( subsets( n , k , []( const auto & ) { return( false ); } ) ==
          C( n , k ) );
 CHECK( subsets( 10 , 3 , []( const auto & ) { return( true ); } ) == 1 );
 // the edges: k = 0, one empty subset, also of n = 0; k = n, the whole
 // set; k > n, none and no call; n , k negative
 std::vector< std::vector< bool > > E;
 auto keep = [ & ]( const auto & s ) { E.push_back( s ); return( false ); };
 CHECK( subsets( 0 , 0 , keep ) == 1 && E.back().empty() );
 CHECK( subsets( 3 , 0 , keep ) == 1 &&
        E.back() == std::vector< bool >( 3 , false ) );
 CHECK( subsets( 3 , 3 , keep ) == 1 &&
        E.back() == std::vector< bool >( 3 , true ) );
 E.clear();
 CHECK( subsets( 3 , 4 , keep ) == 0 && subsets( 0 , 1 , keep ) == 0 &&
        E.empty() );
 CHECK_THROWS( subsets( -1 , 0 , keep ) , contract_violation );
 CHECK_THROWS( subsets( 2 , -1 , keep ) , contract_violation );
 }

// the Sudoku of the reality check and its solution; the grids 0 x 0 and
// 1 x 1, full and empty, and the bad grids the contracts refuse
ADS_TEST( "sec16.3.sudoku" )
{
 const Grid solution = { { 2 , 5 , 3 , 8 , 9 , 1 , 4 , 7 , 6 } ,
                         { 8 , 9 , 7 , 2 , 6 , 4 , 3 , 1 , 5 } ,
                         { 6 , 4 , 1 , 5 , 7 , 3 , 9 , 2 , 8 } ,
                         { 7 , 8 , 9 , 4 , 3 , 5 , 2 , 6 , 1 } ,
                         { 1 , 3 , 6 , 7 , 2 , 9 , 8 , 5 , 4 } ,
                         { 4 , 2 , 5 , 6 , 1 , 8 , 7 , 3 , 9 } ,
                         { 9 , 6 , 8 , 3 , 5 , 2 , 1 , 4 , 7 } ,
                         { 5 , 1 , 2 , 9 , 4 , 7 , 6 , 8 , 3 } ,
                         { 3 , 7 , 4 , 1 , 8 , 6 , 5 , 9 , 2 } };
 // the columns given in each row of the table of the text
 const std::vector< Ints > given = { { 0 , 1 , 4 , 7 , 8 } , { 3 , 5 } ,
  { 2 , 3 , 5 , 6 } , { 1 , 2 , 3 , 5 , 6 , 7 } , { 0 , 4 , 8 } ,
  { 1 , 2 , 3 , 6 , 7 } , { 2 , 3 , 5 , 6 } , { 3 , 5 } ,
  { 0 , 1 , 4 , 7 , 8 } };
 Grid S( 9 , Ints( 9 , 0 ) );
 for( int r = 0 ; r < 9 ; ++r )
  for( int c : given[ r ] ) S[ r ][ c ] = solution[ r ][ c ];
 CHECK( sudoku( S ) && S == solution );
 Grid bad( 9 , Ints( 9 , 0 ) );                    // ( 0 , 0 ) has no
 for( int c = 1 ; c < 9 ; ++c ) bad[ 0 ][ c ] = c;  // number left
 bad[ 1 ][ 0 ] = 9;
 CHECK( ! sudoku( bad ) );
 // the edges: the grid 0 x 0 and the grid 1 x 1; a full grid is left as
 // it is; an empty 9 x 9 grid is filled; the givens inconsistent, or a
 // grid not of side n^2, or a value out of range, violate the audit
 Grid none;
 CHECK( sudoku( none ) && none.empty() );
 Grid one = { { 0 } };
 CHECK( sudoku( one ) && one == Grid( { { 1 } } ) );
 Grid full = solution;
 CHECK( sudoku( full ) && full == solution );
 Grid empty( 9 , Ints( 9 , 0 ) );
 CHECK( sudoku( empty ) );
 for( int r = 0 ; r < 9 ; ++r )
  for( int c = 0 ; c < 9 ; ++c ) {
   const int v = empty[ r ][ c ];
   empty[ r ][ c ] = 0;
   CHECK( v >= 1 && v <= 9 && check( empty , r , c , v ) );
   empty[ r ][ c ] = v;
   }
 Grid twice( 9 , Ints( 9 , 0 ) );
 twice[ 0 ][ 0 ] = twice[ 8 ][ 0 ] = 5;
 CHECK_THROWS( sudoku( twice ) , contract_violation );
 Grid five( 5 , Ints( 5 , 0 ) );
 CHECK_THROWS( sudoku( five ) , contract_violation );
 Grid ten( 4 , Ints( 4 , 0 ) );
 ten[ 1 ][ 1 ] = 10;
 CHECK_THROWS( sudoku( ten ) , contract_violation );
 CHECK_THROWS( sudoku( one , 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the pseudo-angle orders the vectors as their angle, with no arctangent;
// the axes, the diagonals, the null vector, multiples, near 4, 10^18
ADS_TEST( "ex16.1" )
{
 std::mt19937 g( 2 );
 auto angle = []( long dx , long dy ) {
  double a = std::atan2( double( dy ) , double( dx ) );
  return( a < 0 ? a + 2 * std::numbers::pi : a );
  };
 for( int t = 0 ; t < 10000 ; ++t ) {
  const long a = long( g() % 41 ) - 20 , b = long( g() % 41 ) - 20 ,
             c = long( g() % 41 ) - 20 , d = long( g() % 41 ) - 20;
  if( ( a == 0 && b == 0 ) || ( c == 0 && d == 0 ) ) continue;
  const double x = angle( a , b ) , y = angle( c , d );
  if( std::abs( x - y ) < 1e-12 ) continue;
  CHECK( ( x < y ) == ( pseudoAngle( a , b ) < pseudoAngle( c , d ) ) );
  }
 // the edges: the axes give 0 , 1 , 2 , 3, the diagonals the halves; the
 // null vector 0; a vector and its multiples the same value; just below
 // the positive x axis, close to 4 but below it; the extreme components
 // of long, whatever its width
 CHECK( pseudoAngle( 5 , 0 ) == 0 && pseudoAngle( 0 , 5 ) == 1 &&
        pseudoAngle( -5 , 0 ) == 2 && pseudoAngle( 0 , -5 ) == 3 );
 CHECK( pseudoAngle( 1 , 1 ) == 0.5 && pseudoAngle( -1 , 1 ) == 1.5 &&
        pseudoAngle( -1 , -1 ) == 2.5 && pseudoAngle( 1 , -1 ) == 3.5 );
 CHECK( pseudoAngle( 0 , 0 ) == 0 );
 CHECK( pseudoAngle( 3 , -7 ) == pseudoAngle( 30 , -70 ) );
 const double below = pseudoAngle( 1000000 , -1 );
 CHECK( below < 4 && below > 3.999 );
 const long E = std::numeric_limits< long >::max() ,
            F = std::numeric_limits< long >::min();
 CHECK( pseudoAngle( E , E ) == 0.5 && pseudoAngle( -E , -E ) == 2.5 );
 CHECK( pseudoAngle( F , F ) == 2.5 && pseudoAngle( F , 0 ) == 2 &&
        pseudoAngle( 0 , F ) == 3 );
 }

// the permutations, n! of them, in the order of std::next_permutation;
// equal items, and the stop
ADS_TEST( "ex16.2" )
{
 for( int n = 0 ; n <= 7 ; ++n ) {
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  std::vector< Ints > got;
  permutations( A , [ & ]( const Ints & S ) {
   got.push_back( S ); return( false ); } );
  std::vector< Ints > ref;
  do ref.push_back( A ); while( std::next_permutation( A.begin() , A.end() ) );
  CHECK( got == ref );
  }
 // the edges: equal items in different places count as different, so
 // { 1 , 1 , 2 } gives 3! = 6 permutations, each repeated twice; f true
 // stops at the first one
 std::vector< Ints > rep;
 CHECK( permutations( Ints( { 1 , 1 , 2 } ) , [ & ]( const Ints & S ) {
         rep.push_back( S ); return( false ); } ) == 6 );
 CHECK( std::count( rep.begin() , rep.end() , Ints( { 1 , 2 , 1 } ) ) == 2 );
 CHECK( permutations( Ints( { 3 , 2 , 1 } ) , []( const Ints & S ) {
         return( S == Ints( { 3 , 2 , 1 } ) ); } ) == 1 );
 }

// the derangements, !n = 1 , 0 , 1 , 2 , 9 , 44 , 265 , 1854 of them;
// equal items, and the stop
ADS_TEST( "ex16.3" )
{
 const std::vector< long > subfactorial = { 1 , 0 , 1 , 2 , 9 , 44 , 265 ,
                                            1854 };
 for( int n = 0 ; n <= 7 ; ++n ) {
  Ints A( n );
  std::iota( A.begin() , A.end() , 0 );
  CHECK( derangements( A , [ & ]( const Ints & S ) {
          for( int i = 0 ; i < n ; ++i ) CHECK( S[ i ] != i );
          return( false ); } ) == subfactorial[ n ] );
  }
 // the edges: the places are deranged, not the values, so the equal
 // items of { 7 , 7 } give the one derangement 7 , 7; f true stops
 std::vector< Ints > got;
 CHECK( derangements( Ints( { 7 , 7 } ) , [ & ]( const Ints & S ) {
         got.push_back( S ); return( false ); } ) == 1 &&
        got == std::vector< Ints >( { { 7 , 7 } } ) );
 CHECK( derangements( Ints( { 0 , 1 , 2 , 3 } ) , []( const Ints & ) {
         return( true ); } ) == 1 );
 }

// the subsets of sum k, against all the subsets; k = 0, no item, k out of
// reach, equal items, and the precondition
ADS_TEST( "ex16.4" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints A( g() % 12 );
  for( auto & a : A ) a = 1 + int( g() % 10 );
  const int k = int( g() % 30 );
  long ref = 0;
  for( unsigned m = 0 ; m < ( 1u << A.size() ) ; ++m ) {
   int s = 0;
   for( std::size_t i = 0 ; i < A.size() ; ++i ) if( m >> i & 1 ) s += A[ i ];
   ref += s == k;
   }
  CHECK( subsetSum( A , k , [ & ]( const auto & S ) {
          int s = 0;
          for( auto i : S ) s += A[ i ];
          CHECK( s == k );
          return( false ); } ) == ref );
  }
 // the edges: k = 0, the empty subset only, also of no item; no item and
 // k > 0, none; k negative or above the total, none; equal items in
 // different places give different subsets; k equal to the total, all;
 // an item 0 or negative
 auto count = []( const Ints & A , int k ) {
  return( subsetSum( A , k , []( const auto & ) { return( false ); } ) ); };
 CHECK( count( {} , 0 ) == 1 && count( { 3 , 4 } , 0 ) == 1 );
 CHECK( count( {} , 5 ) == 0 && count( { 3 , 4 } , -1 ) == 0 &&
        count( { 3 , 4 } , 8 ) == 0 );
 CHECK( count( Ints( 6 , 2 ) , 4 ) == 15 && count( { 1 , 2 , 3 } , 6 ) == 1 );
 CHECK_THROWS( count( { 1 , 0 } , 1 ) , contract_violation );
 CHECK_THROWS( count( { -2 , 3 } , 1 ) , contract_violation );
 }

// the n queens: 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 , 352 , 724 solutions,
// each admissible by admissible(), the same by the iterative backtrack;
// n = 1 , 2 , 3, admissible() and board() on few queens, the
// preconditions
ADS_TEST( "ex16.5" )
{
 const std::vector< long > count = { 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 , 352 ,
                                     724 };
 for( int n = 1 ; n <= 10 ; ++n )
  CHECK( nQueens( n , []( const Ints & Q ) {
          CHECK( admissible( Q ) );
          return( false ); } ) == count[ n - 1 ] );
 Ints first;
 nQueens( 4 , [ & ]( const Ints & Q ) { first = Q; return( true ); } );
 CHECK( first == Ints( { 1 , 3 , 0 , 2 } ) );
 CHECK( board( first ) == ".Q..\n...Q\nQ...\n..Q.\n" );
 CHECK( ! admissible( { 0 , 1 } ) && nQueens( 32 , []( const Ints & ) {
         return( true ); } ) == 1 );
 // the edges: n = 1 , 2 , 3; the iterative backtracking finds the same
 // solutions in the same order; admissible() on no queen, one, two on a
 // column, on a diagonal far apart; board() of no queen and of one; n out
 // of [ 1 , 32 ], a column out of the board
 auto all = []( auto solve , int n ) {
  std::vector< Ints > S;
  solve( n , [ & ]( const Ints & Q ) { S.push_back( Q ); return( false ); } );
  return( S );
  };
 auto rec = []( int n , auto f ) { return( nQueens( n , f ) ); };
 auto it = []( int n , auto f ) { return( nQueensIterative( n , f ) ); };
 CHECK( all( rec , 1 ) == std::vector< Ints >( { { 0 } } ) );
 CHECK( all( rec , 2 ).empty() && all( rec , 3 ).empty() );
 for( int n = 1 ; n <= 8 ; ++n )
  CHECK( all( rec , n ) == all( it , n ) );
 CHECK( nQueensIterative( 6 , []( const Ints & ) { return( true ); } ) ==
        1 );
 CHECK( admissible( {} ) && admissible( { 5 } ) );
 CHECK( ! admissible( { 2 , 0 , 2 } ) && ! admissible( { 0 , 2 , 4 , 3 } ) );
 CHECK( ! admissible( { 3 , 1 , 4 , 0 } ) );       // rows 0 and 3 only
 CHECK( board( {} ).empty() && board( { 0 } ) == "Q\n" );
 auto none = []( const Ints & ) { return( false ); };
 CHECK_THROWS( nQueens( 0 , none ) , contract_violation );
 CHECK_THROWS( nQueens( 33 , none ) , contract_violation );
 CHECK_THROWS( nQueensIterative( 0 , none ) , contract_violation );
 CHECK_THROWS( board( { 0 , 2 } ) , contract_violation );
 CHECK_THROWS( board( { -1 } ) , contract_violation );
 }

/// a tour visits each square once, by moves of the knight
bool tour( const std::vector< Ints > & A )
{
 const int n = int( A.size() );
 std::vector< std::pair< int , int > > at( n * n + 1 , { -1 , -1 } );
 for( int x = 0 ; x < n ; ++x )
  for( int y = 0 ; y < n ; ++y ) {
   if( A[ x ][ y ] < 1 || A[ x ][ y ] > n * n ) return( false );
   at[ A[ x ][ y ] ] = { x , y };
   }
 for( int k = 1 ; k < n * n ; ++k ) {
  const int dx = std::abs( at[ k ].first - at[ k + 1 ].first ) ,
            dy = std::abs( at[ k ].second - at[ k + 1 ].second );
  if( dx * dy != 2 ) return( false );
  }
 return( true );
 }

// the tour of the knight: by the plain backtrack on 5 x 5 from a corner,
// by the rule of Warnsdorff on 8 x 8 and 30 x 30; none on 3 x 3 and 4 x 4;
// n = 0 (a regression: a negative n built a vector), 1 , 2, 5 x 5 from
// a square of the wrong color, the corner, and the precondition
ADS_TEST( "ex16.6" )
{
 const auto a = knightTour( 5 , 0 , 0 , false );
 CHECK( a && tour( *a ) && ( *a )[ 0 ][ 0 ] == 1 );
 for( int n : { 1 , 5 , 8 , 30 } ) {
  const auto b = knightTour( n , 0 , 0 );
  CHECK( b && tour( *b ) );
  }
 CHECK( ! knightTour( 3 , 0 , 0 ) && ! knightTour( 4 , 0 , 0 ) );
 // the edges: n = 0, no board and no tour; n = 1, the tour of one square
 // by both ways; n = 2; on 5 x 5 a tour starts only from the squares of
 // the color of the corners, so none from ( 0 , 1 ) by either way, and
 // one from the center; the last square of the board as a start; a
 // start off the board
 CHECK( ! knightTour( 0 , 0 , 0 ) && ! knightTour( -3 , 7 , 7 ) );
 for( bool w : { false , true } ) {
  CHECK( *knightTour( 1 , 0 , 0 , w ) == std::vector< Ints >( { { 1 } } ) );
  CHECK( ! knightTour( 2 , 1 , 1 , w ) );
  CHECK( ! knightTour( 5 , 0 , 1 , w ) );
  const auto c = knightTour( 5 , 2 , 2 , w );
  CHECK( c && tour( *c ) && ( *c )[ 2 ][ 2 ] == 1 );
  }
 const auto e = knightTour( 8 , 7 , 7 );
 CHECK( e && tour( *e ) && ( *e )[ 7 ][ 7 ] == 1 );
 CHECK_THROWS( knightTour( 5 , 5 , 0 ) , contract_violation );
 CHECK_THROWS( knightTour( 5 , 0 , -1 ) , contract_violation );
 }

// check() on the squares of a solved grid, and the Sudoku 4 x 4 (n = 2)
// against all the fillings; a value refused by the row, the column or
// the block alone, the grid 1 x 1, and the precondition
ADS_TEST( "ex16.7" )
{
 Grid S = { { 1 , 2 , 3 , 4 } , { 3 , 4 , 1 , 2 } , { 2 , 1 , 4 , 3 } ,
            { 4 , 3 , 2 , 1 } };
 for( int x = 0 ; x < 4 ; ++x )
  for( int y = 0 ; y < 4 ; ++y ) {
   const int c = S[ x ][ y ];
   S[ x ][ y ] = 0;
   for( int d = 1 ; d <= 4 ; ++d )
    CHECK( check( S , x , y , d ) == ( d == c ) );
   S[ x ][ y ] = c;
   }
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Grid P = S;
  for( auto & r : P ) for( auto & v : r ) if( g() % 3 ) v = 0;
  Grid Q = P;
  CHECK( sudoku( Q ) );
  for( int x = 0 ; x < 4 ; ++x )
   for( int y = 0 ; y < 4 ; ++y ) {
    if( P[ x ][ y ] ) CHECK( Q[ x ][ y ] == P[ x ][ y ] );
    const int c = Q[ x ][ y ];
    Q[ x ][ y ] = 0;
    CHECK( check( Q , x , y , c ) );
    Q[ x ][ y ] = c;
    }
  }
 // the edges: a value present only in the row, only in the column, only
 // in the block refuses it; the grid 1 x 1; the square and the value out
 // of range
 Grid T( 4 , Ints( 4 , 0 ) );
 T[ 0 ][ 3 ] = 2;
 T[ 3 ][ 1 ] = 3;
 T[ 1 ][ 0 ] = 4;
 CHECK( ! check( T , 0 , 1 , 2 ) && ! check( T , 0 , 1 , 3 ) &&
        ! check( T , 0 , 1 , 4 ) && check( T , 0 , 1 , 1 ) );
 CHECK( check( T , 2 , 2 , 4 ) && ! check( T , 2 , 0 , 3 ) );
 CHECK( check( Grid( { { 0 } } ) , 0 , 0 , 1 ) );
 CHECK_THROWS( check( T , 4 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( check( T , 0 , 4 , 1 ) , contract_violation );
 CHECK_THROWS( check( T , 0 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( check( T , 0 , 0 , 5 ) , contract_violation );
 }

// the anagrams of a string into words of a small dictionary, against all
// the permutations split in every way; the empty string, one letter, all
// equal letters, letters left over, no word
ADS_TEST( "ex16.8" )
{
 const std::set< std::string > dict = { "a" , "amo" , "amor" , "mora" ,
  "ramo" , "roma" , "ora" , "re" , "mare" , "rame" , "me" , "ma" , "ro" ,
  "e" };
 std::set< std::string > prefixes;
 for( const auto & w : dict )
  for( std::size_t l = 1 ; l <= w.size() ; ++l )
   prefixes.insert( w.substr( 0 , l ) );
 auto word = [ & ]( const std::string & w ) {
  return( dict.count( w ) > 0 ); };
 auto prefix = [ & ]( const std::string & w ) {
  return( prefixes.count( w ) > 0 ); };
 for( std::string s : { std::string( "amor" ) , std::string( "mare" ) ,
                        std::string( "aemr" ) , std::string( "xyz" ) } ) {
  std::set< std::string > ref;
  std::sort( s.begin() , s.end() );
  do {
   const std::size_t n = s.size();
   for( unsigned cut = 0 ; cut < ( 1u << ( n - 1 ) ) ; ++cut ) {
    std::string sentence , w;
    bool ok = true;
    for( std::size_t i = 0 ; i < n && ok ; ++i ) {
     w += s[ i ];
     if( i + 1 == n || ( cut >> i & 1 ) ) {
      ok = word( w );
      sentence += ( sentence.empty() ? "" : " " ) + w;
      w.clear();
      }
     }
    if( ok ) ref.insert( sentence );
    }
   } while( std::next_permutation( s.begin() , s.end() ) );
  CHECK( anagrams( s , word , prefix ) == ref );
  }
 CHECK( anagrams( std::string( "amor" ) , word , prefix ).count( "roma" ) );
 // the edges: the empty string has the one anagram ""; one letter, a word
 // or not; all the letters equal, "a a a" only; letters left over that
 // make no word; a dictionary with no word
 using Set = std::set< std::string >;
 CHECK( anagrams( std::string() , word , prefix ) == Set( { "" } ) );
 CHECK( anagrams( std::string( "e" ) , word , prefix ) == Set( { "e" } ) );
 CHECK( anagrams( std::string( "z" ) , word , prefix ).empty() );
 CHECK( anagrams( std::string( "aaa" ) , word , prefix ) ==
        Set( { "a a a" } ) );
 CHECK( anagrams( std::string( "amoz" ) , word , prefix ).empty() );
 auto never = []( const std::string & ) { return( false ); };
 CHECK( anagrams( std::string( "amor" ) , never , never ).empty() );
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
/*---------------------------- End File ch16.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
