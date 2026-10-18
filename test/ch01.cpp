/*--------------------------------------------------------------------------*/
/*----------------------------- File ch01.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 1, Introduzione: the minimum with n - 1 comparisons, the
 * binary search on the example of the book and against the standard
 * library, and the multiplication of Ahmes.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
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

#include "ADS/numeric/BinaryInteger.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/Minimum.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// min(): n - 1 comparisons, the first minimum of argmin(); the edges: one
// item, all equal (the first place), the minimum first or last, sorted and
// reverse sorted (n - 1 comparisons all the same), the extremes of int, a
// range of one item, and the preconditions (empty, start > end, end >= n)
ADS_TEST( "sec1.2.min" )
{
 long c = 0;
 auto less = [ & ]( int a , int b ) {      // not those of the contracts
  c += ! checkingContract();
  return( a < b ); };
 const Ints A = { 7 , 4 , 2 , 1 , 8 , 1 , 5 };
 CHECK( min( A , less ) == 1 && c == 6 );
 CHECK( argmin( A , 0 , 6 ) == 3 && argmin( A , 4 , 6 ) == 5 );
 CHECK( argmin( A , 2 , 2 ) == 2 && argmin( A , 6 , 6 ) == 6 );
 CHECK( min( Ints( { 9 } ) ) == 9 );
 CHECK( argmin( Ints( 5 , 3 ) , 0 , 4 ) == 0 );   // all equal: the first
 CHECK( argmin( Ints( 5 , 3 ) , 2 , 4 ) == 2 );
 const Ints S = { 1 , 2 , 3 , 4 , 5 } , R = { 5 , 4 , 3 , 2 , 1 };
 c = 0;
 CHECK( min( S , less ) == 1 && c == 4 && argmin( S , 0 , 4 ) == 0 );
 c = 0;
 CHECK( min( R , less ) == 1 && c == 4 && argmin( R , 0 , 4 ) == 4 );
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 CHECK( min( Ints( { hi , 0 , lo , -1 , lo } ) ) == lo );
 CHECK( argmin( Ints( { hi , 0 , lo , -1 , lo } ) , 0 , 4 ) == 2 );
 CHECK( min( Ints( { hi , hi } ) ) == hi );
 CHECK( argmin( Ints( { 2 , 1 } ) , 0 , 1 ) == 1 );
 CHECK_THROWS( (void) min( Ints() ) , contract_violation );
 CHECK_THROWS( (void) argmin( A , 3 , 2 ) , contract_violation );
 CHECK_THROWS( (void) argmin( A , 0 , 7 ) , contract_violation );
 CHECK_THROWS( (void) argmin( Ints() , 0 , 0 ) , contract_violation );
 }

// the example of the book, v = 13: the calls ( i , j , m , A[ m ] ) are
// ( 0 , 11 , 5 , 17 ) , ( 0 , 4 , 2 , 7 ) , ( 3 , 4 , 3 , 8 ) and
// ( 4 , 4 , 4 , 13 ); against std::binary_search, with at most
// floor( log2 n ) + 1 calls; the edges: empty, one item (v below, equal,
// above), v at the first and the last place or outside the range, the
// extremes of int (no overflow of the middle), all equal (some place of v),
// duplicates, and an unsorted vector (the audit of the precondition)
ADS_TEST( "sec1.2.binary-search" )
{
 const Ints A = { 1 , 5 , 7 , 8 , 13 , 17 , 21 , 27 , 33 , 37 , 40 , 42 };
 std::vector< std::tuple< long , long , long , int > > calls;
 CHECK( binarySearch( A , 13 , [ & ]( long i , long j , long m ) {
         calls.emplace_back( i , j , m , A[ m ] ); } ) == 4 );
 using Call = std::tuple< long , long , long , int >;
 const std::vector< Call > book = { { 0 , 11 , 5 , 17 } , { 0 , 4 , 2 , 7 } ,
                                    { 3 , 4 , 3 , 8 } , { 4 , 4 , 4 , 13 } };
 CHECK( calls == book );
 CHECK( binarySearch( A , 14 ) == -1 && binarySearch( Ints() , 1 ) == -1 );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Ints B( g() % 100 );
  for( auto & b : B ) b = g() % 200;
  std::sort( B.begin() , B.end() );
  B.erase( std::unique( B.begin() , B.end() ) , B.end() );
  for( int v = -1 ; v < 201 ; v += 3 ) {
   long n = 0;
   const long p = binarySearch( B , v , [ & ]( long , long , long ) { ++n; } );
   CHECK( ( p >= 0 ) == std::binary_search( B.begin() , B.end() , v ) );
   if( p >= 0 ) CHECK( B[ p ] == v );
   CHECK( n <= ( B.empty() ? 0 :
                 long( std::floor( std::log2( double( B.size() ) ) ) ) + 1 ) );
   }
  } const Ints one = { 7 };
 CHECK( binarySearch( one , 7 ) == 0 && binarySearch( one , 6 ) == -1 &&
        binarySearch( one , 8 ) == -1 );
 CHECK( binarySearch( A , 1 ) == 0 && binarySearch( A , 42 ) == 11 );
 CHECK( binarySearch( A , 0 ) == -1 && binarySearch( A , 43 ) == -1 );
 const int lo = std::numeric_limits< int >::min();
 const int hi = std::numeric_limits< int >::max();
 const Ints E = { lo , lo + 1 , -1 , 0 , 1 , hi - 1 , hi };
 for( long i = 0 ; i < long( E.size() ) ; ++i )
  CHECK( binarySearch( E , E[ i ] ) == i );
 CHECK( binarySearch( Ints( { lo + 1 , hi } ) , lo ) == -1 );
 CHECK( binarySearch( Ints( { lo , hi - 1 } ) , hi ) == -1 );
 const Ints Q( 9 , 4 );                    // all equal
 const long q = binarySearch( Q , 4 );
 CHECK( q >= 0 && q < 9 && binarySearch( Q , 3 ) == -1 &&
        binarySearch( Q , 5 ) == -1 );
 const Ints D = { 1 , 2 , 2 , 2 , 3 , 3 , 9 };
 for( const int v : { 1 , 2 , 3 , 9 } ) {
  const long p = binarySearch( D , v );
  CHECK( p >= 0 && D[ p ] == v );
  }
 CHECK_THROWS( (void) binarySearch( Ints( { 3 , 1 , 2 } ) , 2 ) ,
               contract_violation );
 }

// the papyrus of Ahmes: a b, with a step per bit of a; the edges: a = 0
// (no step), b = 0, a = 1, the powers of 2, and the product modulo 2^64
// when it overflows, up to a = b = 2^64 - 1 (64 steps)
ADS_TEST( "sec1.4.ahmes" )
{
 std::mt19937_64 g( 2 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  const std::uint64_t a = g() >> 33 , b = g() >> 33;
  long s = 0;
  CHECK( ahmes( a , b , &s ) == a * b );
  CHECK( s == long( toBits( a ).size() ) );
  }
 CHECK( ahmes( 13 , 11 ) == 143 && ahmes( 0 , 5 ) == 0 );
 long s = 0;
 CHECK( ahmes( 0 , 5 , &s ) == 0 && s == 0 );
 CHECK( ahmes( 5 , 0 , &s ) == 0 && s == 3 );
 s = 0;
 CHECK( ahmes( 1 , 77 , &s ) == 77 && s == 1 );
 for( int k = 0 ; k < 64 ; ++k ) {
  s = 0;
  CHECK( ahmes( std::uint64_t( 1 ) << k , 3 , &s ) ==
         ( std::uint64_t( 3 ) << k ) && s == k + 1 );
  }
 const std::uint64_t M = std::numeric_limits< std::uint64_t >::max();
 s = 0;
 CHECK( ahmes( M , M , &s ) == 1 && s == 64 );   // ( -1 ) ( -1 ) mod 2^64
 CHECK( ahmes( M , 2 ) == M - 1 && ahmes( 2 , M ) == M - 1 );
 for( int t = 0 ; t < 1000 ; ++t ) {             // the overflow wraps
  const std::uint64_t a = g() , b = g();
  CHECK( ahmes( a , b ) == a * b );
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
/*---------------------------- End File ch01.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
