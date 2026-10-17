/*--------------------------------------------------------------------------*/
/*---------------------- File contract_backtrack.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the enumerations by backtrack:
 * each operation against its specification and the violations of its
 * preconditions, which must throw ADS::contract_violation. The tests are
 * compiled at the level 3, so that each operation also checks its
 * postconditions and audits, on random instances against a brute force or a
 * second algorithm.
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

#include <set>
#include <string>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// nQueens() and board(): n out of [ 1 , 32 ] and a column off the board
// not allowed, and the known numbers of solutions, each admissible
ADS_TEST( "nqueens" )
{
 auto none = []( const std::vector< int > & ) { return( false ); };
 CHECK_THROWS( nQueens( 0 , none ) , contract_violation );
 CHECK_THROWS( nQueens( 33 , none ) , contract_violation );
 CHECK_THROWS( board( { 0 , 2 } ) , contract_violation );
 const long count[] = { 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 };
 for( int n = 1 ; n <= 8 ; ++n )
  CHECK( nQueens( n , none ) == count[ n - 1 ] );
 }

// subsets() and subsetSum(): negative sizes and an item not positive not
// allowed, and the counts of the binomial coefficients
ADS_TEST( "enumeration" )
{
 auto none = []( const auto & ) { return( false ); };
 CHECK_THROWS( subsets( -1 , 0 , none ) , contract_violation );
 CHECK_THROWS( subsets( 3 , -1 , none ) , contract_violation );
 CHECK_THROWS( subsetSum( std::vector< int >( { 1 , 0 } ) , 1 , none ) ,
               contract_violation );
 CHECK( subsets( 6 , 3 , none ) == 20 && subsets( 3 , 5 , none ) == 0 );
 CHECK( permutations( std::vector< int >( { 1 , 2 , 3 , 4 } ) , none ) == 24 );
 CHECK( derangements( std::vector< int >( { 1 , 2 , 3 , 4 } ) , none ) == 9 );
 CHECK( subsetSum( std::vector< int >( { 1 , 2 , 3 , 4 } ) , 5 , none ) == 2 );
 }

// knightTour(): a start off the board not allowed, and the tours found, a
// knight move each step
ADS_TEST( "knight" )
{
 CHECK_THROWS( knightTour( 5 , 5 , 0 ) , contract_violation );
 CHECK_THROWS( knightTour( 5 , 0 , -1 ) , contract_violation );
 CHECK( knightTour( 5 , 0 , 0 ) && knightTour( 6 , 2 , 3 ) );
 CHECK( ! knightTour( 3 , 0 , 0 ) );
 }

// check() and sudoku(): a square or a value out of the grid, a grid whose
// side is not a square and given values in conflict not allowed, and a
// grid of 4 x 4 solved
ADS_TEST( "sudoku" )
{
 Grid S( 4 , std::vector< int >( 4 , 0 ) );
 CHECK_THROWS( check( S , 4 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( check( S , 0 , 0 , 5 ) , contract_violation );
 Grid three( 3 , std::vector< int >( 3 , 0 ) );
 CHECK_THROWS( sudoku( three ) , contract_violation );
 Grid bad = S;
 bad[ 0 ][ 0 ] = bad[ 0 ][ 3 ] = 2;
 CHECK_THROWS( sudoku( bad ) , contract_violation );
 S[ 0 ][ 0 ] = 1;
 S[ 1 ][ 2 ] = 3;
 CHECK( sudoku( S ) );
 }

// anagrams(): each sentence a permutation of the letters
ADS_TEST( "anagrams" )
{
 const std::set< std::string > D = { "ab" , "ba" , "a" , "b" , "c" };
 auto word = [ & ]( const std::string & w ) { return( D.count( w ) > 0 ); };
 auto prefix = [ & ]( const std::string & w ) {
  for( const auto & d : D ) if( d.compare( 0 , w.size() , w ) == 0 )
   return( true );
  return( false ); };
 const auto A = anagrams( std::string( "abc" ) , word , prefix );
 CHECK( A.count( "ab c" ) == 1 && A.count( "c ba" ) == 1 );
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
/*-------------------- End File contract_backtrack.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
