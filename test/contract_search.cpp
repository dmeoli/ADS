/*--------------------------------------------------------------------------*/
/*------------------------ File contract_search.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the searches, the minimum and the
 * selection: the results as the specifications of the book state them, and the
 * violations of the preconditions, which must throw ADS::contract_violation.
 * The tests are compiled at the level 3, so that each call also checks its
 * postconditions and its audits, on random instances against a reference of
 * the standard library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and
 *      12.1, https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
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

#include "ADS/search/BinarySearch.h"
#include "ADS/search/InterpolationSearch.h"
#include "ADS/search/Minimum.h"
#include "ADS/search/Selection.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;

/// a random vector of n items in [ 0 , k )
Ints random_ints( std::size_t n , int k , std::mt19937 & g )
{
 Ints A( n );
 for( auto & a : A ) a = int( g() % k );
 return( A );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the binary searches on sorted vectors against std::find,
// std::lower_bound and std::upper_bound; an unsorted vector and a range
// out of [ 0 , n ] violate their contracts
ADS_TEST( "binary-search" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints A = random_ints( g() % 40 , 30 , g );
  std::sort( A.begin() , A.end() );
  const int v = int( g() % 32 ) - 1;
  const long r = binarySearch( A , v );
  CHECK( ( r < 0 ) == ( std::find( A.begin() , A.end() , v ) == A.end() ) );
  if( r >= 0 ) CHECK( A[ r ] == v );
  const auto lb = std::size_t( std::lower_bound( A.begin() , A.end() , v ) -
                               A.begin() );
  CHECK( lowerBound( A , v ) == lb && gallopSearch( A , v ) == lb );
  CHECK( upperBound( A , v ) == std::size_t(
         std::upper_bound( A.begin() , A.end() , v ) - A.begin() ) );
  const std::size_t lo = A.empty() ? 0 : g() % A.size();
  const std::size_t hi = lo + g() % ( A.size() - lo + 1 );
  const auto lr = std::lower_bound( A.begin() + lo , A.begin() + hi , v );
  CHECK( lowerBound( A , v , lo , hi ) == std::size_t( lr - A.begin() ) );
  }
 const Ints U = { 3 , 1 , 2 };
 CHECK_THROWS( (void) binarySearch( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) lowerBound( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) upperBound( U , 2 ) , contract_violation );
 CHECK_THROWS( (void) gallopSearch( U , 2 ) , contract_violation );
 const Ints S = { 1 , 2 , 3 };
 CHECK_THROWS( (void) lowerBound( S , 2 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) lowerBound( S , 2 , 0 , 4 ) , contract_violation );
 CHECK( lowerBound( S , 2 , 1 , 1 ) == 1 );
 }

// the search by interpolation against std::find; an unsorted vector
// violates its contract
ADS_TEST( "interpolation-search" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Ints A = random_ints( g() % 40 , 50 , g );
  std::sort( A.begin() , A.end() );
  const int v = int( g() % 52 ) - 1;
  const long r = interpolationSearch( A , v );
  CHECK( ( r < 0 ) == ( std::find( A.begin() , A.end() , v ) == A.end() ) );
  if( r >= 0 ) CHECK( A[ r ] == v );
  }
 CHECK_THROWS( (void) interpolationSearch( Ints( { 5 , 1 , 9 } ) , 9 ) ,
               contract_violation );
 }

// argmin() and min() against std::min_element; a range out of the vector
// and an empty vector violate their contracts
ADS_TEST( "minimum" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Ints A = random_ints( 1 + g() % 30 , 20 , g );
  const std::size_t s = g() % A.size() , e = s + g() % ( A.size() - s );
  const auto m = argmin( A , s , e );
  CHECK( m >= s && m <= e &&
         A[ m ] == *std::min_element( A.begin() + s , A.begin() + e + 1 ) );
  CHECK( min( A ) == *std::min_element( A.begin() , A.end() ) );
  }
 const Ints A = { 4 , 2 , 7 };
 CHECK_THROWS( (void) argmin( A , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) argmin( A , 0 , 3 ) , contract_violation );
 CHECK_THROWS( (void) min( Ints() ) , contract_violation );
 }

// selection() and medianOfMedians() against std::nth_element, for every
// rank; the ranks 0 and n + 1 violate their contracts
ADS_TEST( "selection" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const Ints A = random_ints( 1 + g() % 60 , 25 , g );
  Ints S = A;
  std::sort( S.begin() , S.end() );
  const std::size_t k = 1 + g() % A.size();
  Ints B = A , C = A , D = A;
  CHECK( selection( B , k ) == S[ k - 1 ] );
  CHECK( selection( C , k , &g ) == S[ k - 1 ] );
  CHECK( medianOfMedians( D , k ) == S[ k - 1 ] );
  }
 Ints E = { 3 , 1 , 2 };
 CHECK_THROWS( (void) selection< int >( E , 0 ) , contract_violation );
 CHECK_THROWS( (void) selection< int >( E , 4 ) , contract_violation );
 CHECK_THROWS( (void) medianOfMedians( E , 0 ) , contract_violation );
 CHECK_THROWS( (void) medianOfMedians( E , 4 ) , contract_violation );
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
/*---------------------- End File contract_search.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
