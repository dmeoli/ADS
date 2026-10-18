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
#include "ADS/backtrack/DancingLinks.h"
#include "ADS/backtrack/Enumeration.h"
#include "ADS/backtrack/KnightTour.h"
#include "ADS/backtrack/NQueens.h"
#include "ADS/backtrack/SatSolvers.h"
#include "ADS/backtrack/Sudoku.h"

#include <limits>
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

// nQueens(), nQueensIterative() and board(): n out of [ 1 , 32 ] (0, -1,
// 33) and a column off the board ( -1 , n ) not allowed, n = 1 and 32
// allowed, the empty board, and the known numbers of solutions of both,
// each admissible
ADS_TEST( "nqueens" )
{
 auto none = []( const std::vector< int > & ) { return( false ); };
 CHECK_THROWS( nQueens( 0 , none ) , contract_violation );
 CHECK_THROWS( nQueens( 33 , none ) , contract_violation );
 CHECK_THROWS( board( { 0 , 2 } ) , contract_violation );
 const long count[] = { 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 };
 for( int n = 1 ; n <= 8 ; ++n )
  CHECK( nQueens( n , none ) == count[ n - 1 ] &&
         nQueensIterative( n , none ) == count[ n - 1 ] );
 // the bounds: 1 just inside, 0 , -1 , 33 just outside, 32 inside (the
 // first solution only, the visit stops)
 auto first = []( const std::vector< int > & ) { return( true ); };
 CHECK( nQueens( 32 , first ) == 1 );
 CHECK_THROWS( nQueens( -1 , none ) , contract_violation );
 CHECK_THROWS( nQueensIterative( 0 , none ) , contract_violation );
 CHECK_THROWS( nQueensIterative( -5 , none ) , contract_violation );
 CHECK_THROWS( board( { -1 } ) , contract_violation );
 CHECK_THROWS( board( { 1 } ) , contract_violation );
 CHECK( board( { 0 } ).size() > 0 && board( {} ).empty() );
 CHECK( ! board( { 1 , 0 } ).empty() );      // columns 0 and n - 1
 }

// subsets() and subsetSum(): negative sizes and an item not positive (0,
// -1, the least int) not allowed, and the counts of the binomial
// coefficients; n = 0, k = 0, k = n, k > n, no item, one item, equal
// items, an unreachable sum, the permutations and derangements of 0 , 1
// and 2 items
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
 // n = 0, k = 0, k = n, k > n; no item, one item, k = 0, an unreachable
 // k, equal items, a negative item and the least int
 CHECK( subsets( 0 , 0 , none ) == 1 && subsets( 0 , 1 , none ) == 0 );
 CHECK( subsets( 4 , 0 , none ) == 1 && subsets( 4 , 4 , none ) == 1 &&
        subsets( 4 , 5 , none ) == 0 );
 CHECK( permutations( std::vector< int >() , none ) == 1 &&
        permutations( std::vector< int >( { 7 } ) , none ) == 1 &&
        permutations( std::vector< int >( { 7 , 7 } ) , none ) == 2 );
 CHECK( derangements( std::vector< int >() , none ) == 1 &&
        derangements( std::vector< int >( { 7 } ) , none ) == 0 &&
        derangements( std::vector< int >( { 7 , 7 } ) , none ) == 1 );
 CHECK( subsetSum( std::vector< int >() , 0 , none ) == 1 &&
        subsetSum( std::vector< int >() , 3 , none ) == 0 &&
        subsetSum( std::vector< int >( { 3 } ) , 3 , none ) == 1 &&
        subsetSum( std::vector< int >( { 3 } ) , 0 , none ) == 1 &&
        subsetSum( std::vector< int >( { 2 , 2 } ) , 2 , none ) == 2 &&
        subsetSum( std::vector< int >( { 2 , 4 } ) , 5 , none ) == 0 );
 CHECK_THROWS( subsetSum( std::vector< int >( { 2 , -1 } ) , 1 , none ) ,
               contract_violation );
 CHECK_THROWS( subsetSum( std::vector< int >(
                { std::numeric_limits< int >::min() } ) , 0 , none ) ,
               contract_violation );
 CHECK_THROWS( subsets( -1 , -1 , none ) , contract_violation );
 }

// knightTour(): a start off the board not allowed (each coordinate at -1
// and at n), the corners allowed, and the tours found, a knight move each
// step; n = 1 , 2 , 3 , 4, and n < 1 with no board and any start
ADS_TEST( "knight" )
{
 CHECK_THROWS( knightTour( 5 , 5 , 0 ) , contract_violation );
 CHECK_THROWS( knightTour( 5 , 0 , -1 ) , contract_violation );
 CHECK( knightTour( 5 , 0 , 0 ) && knightTour( 6 , 2 , 3 ) );
 CHECK( ! knightTour( 3 , 0 , 0 ) );
 // the corners just inside, each coordinate just outside; n = 1 and 2;
 // n < 1 has no board, any start allowed
 CHECK( knightTour( 5 , 4 , 4 ) && knightTour( 5 , 0 , 4 , true ) );
 CHECK_THROWS( knightTour( 5 , -1 , 0 ) , contract_violation );
 CHECK_THROWS( knightTour( 5 , 0 , 5 ) , contract_violation );
 CHECK_THROWS( knightTour( 1 , 1 , 0 ) , contract_violation );
 const auto one = knightTour( 1 , 0 , 0 );
 CHECK( one && one->size() == 1 && ( *one )[ 0 ][ 0 ] >= 0 );
 CHECK( ! knightTour( 2 , 1 , 1 ) && ! knightTour( 4 , 0 , 0 , true ) );
 CHECK( ! knightTour( 0 , 9 , -9 ) && ! knightTour( -3 , 0 , 0 ) );
 }

// check() and sudoku(): a square or a value out of the grid, a grid whose
// side is not a square and given values in conflict not allowed, and a
// grid of 4 x 4 solved; each bound of check() just inside and outside, the
// start i at n^2 + 1 (refused) and at n^2 (an incomplete grid refused by
// the audit), values out of [ 0 , n ], a ragged grid, the grids of 1 x 1
// and 0 x 0, a consistent grid with no solution
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
 // check(): x , y just inside ( n - 1 ) and outside ( n ), c in [ 1 , n ]
 Grid E( 4 , std::vector< int >( 4 , 0 ) );
 CHECK( check( E , 3 , 3 , 4 ) && check( E , 0 , 0 , 1 ) );
 CHECK_THROWS( check( E , 0 , 4 , 1 ) , contract_violation );
 CHECK_THROWS( check( E , 0 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( check( E , 0 , 0 , -1 ) , contract_violation );
 // sudoku(): i in [ 0 , n^2 ]; at n^2 the grid must be complete, so the
 // empty grid fails the audit there, while the solved one is accepted
 CHECK_THROWS( sudoku( E , 17 ) , contract_violation );
 CHECK_THROWS( sudoku( E , 16 ) , contract_violation );
 CHECK( sudoku( S , 16 ) && sudoku( S , 15 ) );
 Grid out = E;                               // a value out of [ 0 , n ]
 out[ 2 ][ 2 ] = 5;
 CHECK_THROWS( sudoku( out ) , contract_violation );
 out[ 2 ][ 2 ] = -1;
 CHECK_THROWS( sudoku( out ) , contract_violation );
 Grid one( 1 , std::vector< int >( 1 , 0 ) ) , none;
 CHECK( sudoku( one ) && one[ 0 ][ 0 ] == 1 && sudoku( none ) );
 Grid ragged = E;                            // not square
 ragged[ 1 ].pop_back();
 CHECK_THROWS( sudoku( ragged ) , contract_violation );
 Grid blocked = E;                           // consistent, no solution:
 blocked[ 0 ] = { 1 , 2 , 3 , 0 };           // ( 0 , 3 ) needs the 4 that
 blocked[ 2 ][ 3 ] = 4;                      // its column already has
 CHECK( ! sudoku( blocked ) );
 }

// ExactCover: primary , secondary >= 0, an option not empty, of distinct
// items in [ 0 , primary + secondary ), its least item primary; no item
// (one empty solution), a single item, items covered by no option
ADS_TEST( "exact-cover" )
{
 CHECK_THROWS( ExactCover( -1 ) , contract_violation );
 CHECK_THROWS( ExactCover( 2 , -1 ) , contract_violation );
 ExactCover X( 3 , 1 );
 auto none = []( const std::vector< int > & ) { return( false ); };
 CHECK_THROWS( X.addOption( {} ) , contract_violation );
 CHECK_THROWS( X.addOption( { -1 , 0 } ) , contract_violation );
 CHECK_THROWS( X.addOption( { 0 , 4 } ) , contract_violation );
 CHECK_THROWS( X.addOption( { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( X.addOption( { 3 } ) , contract_violation );   // secondary
 CHECK( X.addOption( { 0 , 3 } ) == 0 );     // the last item, secondary
 CHECK( X.addOption( { 2 , 1 } ) == 1 );     // the last primary, unsorted
 CHECK( X.solve( none ) == 1 );
 CHECK( X.addOption( { 0 , 1 , 2 } ) == 2 );
 CHECK( X.solve( none ) == 2 );
 ExactCover Z( 0 );                          // no item: the empty cover
 CHECK( Z.solve( none ) == 1 );
 CHECK_THROWS( Z.addOption( { 0 } ) , contract_violation );
 ExactCover U( 2 );                          // the item 1 in no option
 U.addOption( { 0 } );
 CHECK( U.solve( none ) == 0 );
 }

// dpll() and Cdcl: n >= 0, the literals in [ -n , n ] and not 0, just
// inside and just outside; the empty formula, an empty clause, a unit
// clause and its negation, a budget of 0 conflicts
ADS_TEST( "sat" )
{
 CHECK_THROWS( (void) dpll( Cnf() , -1 ) , contract_violation );
 CHECK_THROWS( Cdcl( Cnf() , -1 ) , contract_violation );
 const Cnf F = { { 1 , -2 } , { 2 } };
 CHECK( dpll( F , 2 ) && Cdcl( F , 2 ).solve() == Cdcl::Result::SAT );
 for( int l : { 0 , 3 , -3 } ) {
  const Cnf G = { { 1 , l } };
  CHECK_THROWS( (void) dpll( G , 2 ) , contract_violation );
  CHECK_THROWS( Cdcl( G , 2 ) , contract_violation );
  }
 CHECK( dpll( Cnf() , 0 ) && dpll( Cnf() , 0 )->empty() );
 CHECK( Cdcl( Cnf() , 0 ).solve() == Cdcl::Result::SAT );
 CHECK( ! dpll( Cnf( { {} } ) , 0 ) &&
        Cdcl( Cnf( { {} } ) , 0 ).solve() == Cdcl::Result::UNSAT );
 const Cnf N = { { 1 } , { -1 } };
 CHECK( ! dpll( N , 1 ) && Cdcl( N , 1 ).solve() == Cdcl::Result::UNSAT );
 CHECK( dpll( Cnf( { { -1 , -1 } } ) , 1 ) );            // repeated
 CHECK( dpll( Cnf( { { 1 , -1 } } ) , 1 ) );             // tautology
 Cdcl P( Cnf( { { 1 , 2 } , { -1 , 2 } , { 1 , -2 } , { -1 , -2 } } ) , 2 );
 const auto r = P.solve( 0 );
 CHECK( r != Cdcl::Result::SAT );
 }

// anagrams(): each sentence a permutation of the letters; the empty
// string, a letter in no word, all the letters equal
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
 // the empty string (one empty sentence or none), a letter in no word,
 // all the letters equal
 const auto E = anagrams( std::string() , word , prefix );
 CHECK( E.size() <= 1 );
 CHECK( anagrams( std::string( "z" ) , word , prefix ).empty() );
 CHECK( anagrams( std::string( "aa" ) , word , prefix ) ==
        std::set< std::string >( { "a a" } ) );
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
