/*--------------------------------------------------------------------------*/
/*---------------------- File contract_algorithm.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the binary counter, the towers of
 * Hanoi, the master theorem, the Monte Carlo algorithms and the subsequences:
 * the results as the specifications of the book state them, and the violations
 * of the preconditions, which must throw ADS::contract_violation. The tests
 * are compiled at the level 3, so that each call also checks its
 * postconditions and its audits, on random instances against a reference of
 * the standard library or an enumeration.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 2.6 and
 *      4.4, https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.4,
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

#include "ADS/algorithm/BinaryCounter.h"
#include "ADS/algorithm/Hanoi.h"
#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/algorithm/MonteCarlo.h"
#include "ADS/algorithm/Subsequences.h"

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

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the counter of k bits counts 0 , 1 , ... , 2^k - 1 and back to 0, at an
// amortized cost below 2; an item that is not a bit violates the contract
ADS_TEST( "binary-counter" )
{
 for( std::size_t k : { 1u , 4u , 10u } ) {
  Ints A( k , 0 );
  std::size_t cost = 0;
  for( std::size_t v = 1 ; v <= ( std::size_t( 1 ) << k ) ; ++v ) {
   cost += add( A );
   std::size_t x = 0;
   for( std::size_t i = k ; i-- > 0 ; ) x = 2 * x + std::size_t( A[ i ] );
   CHECK( x == v % ( std::size_t( 1 ) << k ) );
   }
  CHECK( cost < 2 * ( std::size_t( 1 ) << k ) + k );
  }
 Ints B = { 1 , 2 , 0 };
 CHECK_THROWS( (void) add( B ) , contract_violation );
 }

// the three towers of Hanoi make the same 2^n - 1 moves, each legal; no
// disk and two equal pegs violate the contracts
ADS_TEST( "hanoi" )
{
 for( int n = 1 ; n <= 10 ; ++n ) {
  std::vector< std::pair< int , int > > M[ 3 ];
  hanoi_recursive( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 0 ].emplace_back( a , b ); } );
  hanoi_iterative( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 1 ].emplace_back( a , b ); } );
  hanoi_iterative_labels( n , 0 , 2 , 1 , [ & ]( int a , int b ) {
   M[ 2 ].emplace_back( a , b ); } );
  CHECK( M[ 0 ].size() == ( std::size_t( 1 ) << n ) - 1 );
  CHECK( M[ 0 ] == M[ 1 ] && M[ 1 ] == M[ 2 ] );
  std::vector< Ints > peg( 3 );                  // the moves are legal
  for( int d = n ; d >= 1 ; --d ) peg[ 0 ].push_back( d );
  for( auto [ a , b ] : M[ 0 ] ) {
   CHECK( ! peg[ a ].empty() &&
          ( peg[ b ].empty() || peg[ b ].back() > peg[ a ].back() ) );
   peg[ b ].push_back( peg[ a ].back() );
   peg[ a ].pop_back();
   }
  CHECK( int( peg[ 2 ].size() ) == n );
  }
 auto none = []( int , int ) {};
 CHECK_THROWS( hanoi_recursive( 0 , 0 , 2 , 1 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative( 3 , 0 , 0 , 1 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative_labels( 3 , 0 , 2 , 2 , none ) ,
               contract_violation );
 }

// the three cases of the master theorem and the linear recurrences; a < 1
// and b <= 1 violate the contracts
ADS_TEST( "master" )
{
 CHECK( master( 2 , 2 , 1 ).q == 1 );            // Merge Sort: n log n
 CHECK( master( 8 , 2 , 2 ).p == 3 );            // the first case
 CHECK( master( 1 , 2 , 1 ).p == 1 );            // the third case
 CHECK( linear( 2 , 0 ).upper && linear( 1 , 1 ).p == 2 );
 CHECK_THROWS( (void) master( 0.5 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 2 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) linear( 0 , 1 ) , contract_violation );
 }

// Freivalds accepts the product and rejects a wrong one; the test of the
// null polynomial and the half-satisfiability; a negative number of
// repetitions and matrices that are not square violate the contracts
ADS_TEST( "monte-carlo" )
{
 std::mt19937 g( 4 );
 using M = std::vector< std::vector< long > >;
 for( int t = 0 ; t < 50 ; ++t ) {
  const std::size_t n = 1 + g() % 6;
  M A( n , std::vector< long >( n ) ) , B = A , C = A;
  for( auto & r : A ) for( auto & x : r ) x = long( g() % 7 ) - 3;
  for( auto & r : B ) for( auto & x : r ) x = long( g() % 7 ) - 3;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    for( std::size_t k = 0 ; k < n ; ++k )
     C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
  CHECK( freivalds( A , B , C , 20 , g ) );
  C[ g() % n ][ g() % n ] += 1;
  CHECK( ! freivalds( A , B , C , 40 , g ) );
  }
 auto zero = []( const std::vector< long > & x ) {
  return( ( x[ 0 ] + x[ 1 ] ) * ( x[ 0 ] - x[ 1 ] ) - x[ 0 ] * x[ 0 ] +
          x[ 1 ] * x[ 1 ] ); };
 auto one = []( const std::vector< long > & x ) { return( x[ 0 ] - x[ 1 ] ); };
 CHECK( zeroPolynomial( zero , 2 , 2 , 10 , g ) );
 CHECK( ! zeroPolynomial( one , 2 , 1 , 30 , g ) );
 auto mostly = []( const std::vector< bool > & x ) {
  return( x[ 0 ] || x[ 1 ] ); };
 CHECK( halfSatisfiable( mostly , 2 , 400 , g ) );
 const M sq( 2 , std::vector< long >( 2 ) ) ,
         rect( 2 , std::vector< long >( 3 ) );
 CHECK_THROWS( (void) freivalds( sq , rect , sq , 1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) freivalds( sq , sq , sq , -1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) zeroPolynomial( zero , 2 , 2 , -1 , g ) ,
               contract_violation );
 CHECK_THROWS( (void) halfSatisfiable( mostly , 2 , -1 , g ) ,
               contract_violation );
 }

// the longest increasing subsequence, checked by its audit against
// lisLength(), and the subvector of largest sum against the enumeration
ADS_TEST( "subsequences" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Ints A( g() % 40 );
  for( auto & a : A ) a = int( g() % 30 ) - 15;
  const Ints L = lis( A );
  CHECK( L.size() == lisLength( A ) );
  int best = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i ; j < A.size() ; ++j ) {
    int s = 0;
    for( std::size_t h = i ; h <= j ; ++h ) s += A[ h ];
    best = std::max( best , s );
    }
  CHECK( maxSubarraySum( A ) == best );
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
/*-------------------- End File contract_algorithm.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
