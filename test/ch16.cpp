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
 *         Universita' di Pisa \n
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

// stessaparte(): the points on the line count on both sides
ADS_TEST( "sec16.1.same-side" )
{
 const Point a{ 0 , 0 } , b{ 4 , 4 };
 CHECK( sameSide( a , b , { 0 , 1 } , { 1 , 5 } ) );
 CHECK( ! sameSide( a , b , { 0 , 1 } , { 1 , 0 } ) );
 CHECK( sameSide( a , b , { 2 , 2 } , { 1 , 0 } ) );
 CHECK( sameSide( { 1 , 0 } , { 1 , 9 } , { 0 , 3 } , { -5 , -5 } ) );
 }

// Graham against the hull of O( n^3 ), on random points with many
// collinear ones; the hull is convex and contains every point
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
 }

// the subsets of k = 2 of n = 4 items, the leaves of Figura 16.5, and their
// number C( n , k ) (n! / ( n - k )! in the text counts their orderings)
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
 }

// the Sudoku of the reality check and its solution
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
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the pseudo-angle orders the vectors as their angle, with no arctangent
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
 }

// the permutations, n! of them, in the order of std::next_permutation
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
 }

// the derangements, !n = 1 , 0 , 1 , 2 , 9 , 44 , 265 , 1854 of them
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
 }

// the subsets of sum k, against all the subsets
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
 }

// the n queens: 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 , 352 , 724 solutions,
// each admissible by admissible()
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
// by the rule of Warnsdorff on 8 x 8 and 30 x 30; none on 3 x 3 and 4 x 4
ADS_TEST( "ex16.6" )
{
 const auto a = knightTour( 5 , 0 , 0 , false );
 CHECK( a && tour( *a ) && ( *a )[ 0 ][ 0 ] == 1 );
 for( int n : { 1 , 5 , 8 , 30 } ) {
  const auto b = knightTour( n , 0 , 0 );
  CHECK( b && tour( *b ) );
  }
 CHECK( ! knightTour( 3 , 0 , 0 ) && ! knightTour( 4 , 0 , 0 ) );
 }

// check() on the squares of a solved grid, and the Sudoku 4 x 4 (n = 2)
// against all the fillings
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
 }

// the anagrams of a string into words of a small dictionary, against all
// the permutations split in every way
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
