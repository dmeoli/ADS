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

// min(): n - 1 comparisons, the first minimum of argmin()
ADS_TEST( "sec1.2.min" )
{
 long c = 0;
 auto less = [ & ]( int a , int b ) { ++c; return( a < b ); };
 const Ints A = { 7 , 4 , 2 , 1 , 8 , 1 , 5 };
 CHECK( min( A , less ) == 1 && c == 6 );
 CHECK( argmin( A , 0 , 6 ) == 3 && argmin( A , 4 , 6 ) == 5 );
 CHECK( min( Ints( { 9 } ) ) == 9 );
 CHECK_THROWS( (void) min( Ints() ) , std::invalid_argument );
 }

// the example of the book, v = 13: the calls ( i , j , m , A[ m ] ) are
// ( 0 , 11 , 5 , 17 ) , ( 0 , 4 , 2 , 7 ) , ( 3 , 4 , 3 , 8 ) and
// ( 4 , 4 , 4 , 13 ); against std::binary_search, with at most
// floor( log2 n ) + 1 calls
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
  }
 }

// the papyrus of Ahmes: a b, with a step per bit of a
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
