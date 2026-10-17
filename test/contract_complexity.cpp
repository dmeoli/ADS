/*--------------------------------------------------------------------------*/
/*---------------------- File contract_complexity.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the problems of NP and of
 * their reductions: each precondition violated must throw
 * ADS::contract_violation, and on random instances, at the level 3, each
 * certificate returned by an enumeration, each output of a reduction and
 * each solution of the polynomial twins of Tabella 19.1 is checked by the
 * postconditions.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
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

#include "ADS/complexity/Domino.h"
#include "ADS/complexity/EasyCases.h"
#include "ADS/complexity/Nondeterminism.h"
#include "ADS/complexity/Problems.h"
#include "ADS/complexity/Reductions.h"

#include <algorithm>
#include <limits>
#include <random>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Longs = std::vector< long >;

/// a random unoriented graph, each edge with probability p percent
Adjacency random_graph( int n , int p , std::mt19937 & g )
{
 Adjacency G( n , std::vector< bool >( n , false ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( int( g() % 100 ) < p ) G[ u ][ v ] = G[ v ][ u ] = true;
 return( G );
 }

/// a random oriented graph
Adjacency random_digraph( int n , int p , std::mt19937 & g )
{
 Adjacency G( n , std::vector< bool >( n , false ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && ( int( g() % 100 ) < p ) ) G[ u ][ v ] = true;
 return( G );
 }

/// the path 0 -> 1 and 1 -> 0, a graph that is oriented and not symmetric
const Adjacency arc = { { false , true } , { false , false } };

/// random distances in [ 1 , w ], symmetric
std::vector< Longs > random_distances( int n , long w , std::mt19937 & g )
{
 std::vector< Longs > D( n , Longs( n , 0 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v ) D[ u ][ v ] = D[ v ][ u ] = 1 + g() % w;
 return( D );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the problems of NP: the preconditions violated, and random instances
// whose certificates the postconditions check
ADS_TEST( "problems" )
{
 const Adjacency ragged = { { false , true } , { true } };
 CHECK_THROWS( clique( arc , 1 ) , contract_violation );
 CHECK_THROWS( cliqueCertificate( ragged , { 0 } , 1 ) , contract_violation );
 CHECK_THROWS( independentSet( arc , 1 ) , contract_violation );
 CHECK_THROWS( vertexCover( arc , 1 ) , contract_violation );
 CHECK_THROWS( coloring( Adjacency( 2 , std::vector< bool >( 2 , false ) ) ,
                         -1 ) , contract_violation );
 CHECK_THROWS( bisection( arc , 1 ) , contract_violation );
 CHECK_THROWS( hamiltonianCircuit( arc ) , contract_violation );
 CHECK_THROWS( hamiltonianPath( arc , 2 ) , contract_violation );
 CHECK_THROWS( longestPath( arc , 0 , 5 , 1 ) , contract_violation );
 const Adjacency cycle = { { false , true } , { true , false } };
 CHECK_THROWS( longestPathDag( cycle , 0 , 1 ) , contract_violation );
 CHECK( longestPathDag( arc , 0 , 1 ) == 1 );
 CHECK_THROWS( satisfiable( { { 1 , 3 } } , 2 ) , contract_violation );
 CHECK_THROWS( satisfiable( { { 0 } } , 2 ) , contract_violation );
 CHECK_THROWS( satCertificate( { { 2 } } , { true } ) , contract_violation );
 CHECK_THROWS( lp01( { { 1 , 1 } } , { 1 } , 3 ) , contract_violation );
 CHECK_THROWS( lp01( { { 1 } } , { 1 , 2 } , 1 ) , contract_violation );
 CHECK_THROWS( subsetSumExists( { 1 , -2 } , 1 ) , contract_violation );
 CHECK_THROWS( partition( { -1 , 1 } ) , contract_violation );
 CHECK_THROWS( knapsackDecision( { 1 } , { 1 , 2 } , 3 , 1 ) ,
               contract_violation );
 CHECK_THROWS( multiprocessorScheduling( { 1 } , 0 , 5 ) ,
               contract_violation );
 CHECK_THROWS( exactCover3( { { 0 , 1 , 3 } } , 3 ) , contract_violation );
 CHECK_THROWS( exactCover3( { { 0 , 0 , 1 } } , 3 ) , contract_violation );
 CHECK_THROWS( threeDimensionalMatching( { { 0 , 1 , 2 } } , 2 ) ,
               contract_violation );
 CHECK_THROWS( bribes( { 1 , 2 } , { true } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 ) , k = int( g() % ( n + 1 ) );
  const auto G = random_graph( n , int( g() % 100 ) , g );
  const auto H = random_digraph( n , 40 , g );
  ( void ) clique( G , k );
  ( void ) independentSet( G , k );
  ( void ) vertexCover( G , k );
  ( void ) coloring( G , k );
  ( void ) bisection( G , k );
  ( void ) hamiltonianCircuit( G );
  ( void ) hamiltonianChain( G );
  ( void ) hamiltonianCycle( H );
  ( void ) hamiltonianPath( H , int( g() % n ) , int( g() % n ) - 1 );
  ( void ) longestPath( H , 0 , n - 1 , k );
  ( void ) boundedSpanningTree( G , 1 + int( g() % 3 ) );
  ( void ) tsp( random_distances( n , 9 , g ) , long( g() % 40 ) );
  Cnf F( 1 + g() % 6 );
  for( auto & c : F ) {
   c.resize( 1 + g() % 3 );
   for( auto & l : c ) l = ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 );
   }
  ( void ) satisfiable( F , n );
  Longs A( g() % 8 );
  for( auto & a : A ) a = long( g() % 10 );
  ( void ) subsetSumExists( A , long( g() % 30 ) );
  ( void ) partition( A );
  ( void ) knapsackDecision( A , A , long( g() % 30 ) , long( g() % 30 ) );
  ( void ) multiprocessorScheduling( A , 1 + int( g() % 3 ) ,
                                     long( g() % 20 ) );
  ( void ) bribes( A , std::vector< bool >( A.size() , g() % 2 ) );
  }
 }

// the reductions: their inputs checked, their outputs instances of the
// target problems
ADS_TEST( "reductions" )
{
 const std::vector< Tile > D = { { 1 , 1 , 0 , 0 } , { 0 , 0 , 1 , 1 } };
 CHECK_THROWS( dominoToClique( D , 0 ) , contract_violation );
 CHECK_THROWS( dominoToSat( {} , 2 ) , contract_violation );
 CHECK_THROWS( dominoToLp01( D , -1 ) , contract_violation );
 CHECK_THROWS( tileVar( 0 , 2 , 0 , 2 ) , contract_violation );
 CHECK_THROWS( halfSum( { 3 , -1 } ) , contract_violation );
 CHECK_THROWS( hamiltonToTsp( arc ) , contract_violation );
 CHECK_THROWS( complement( { { false , true } } ) , contract_violation );
 CHECK_THROWS( cycleToChain( { { false } , {} } ) , contract_violation );
 CHECK_THROWS( sortToHull( { std::numeric_limits< long >::max() } ) ,
               contract_violation );
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 3 );
  std::vector< Tile > T( 1 + g() % 3 );
  for( auto & x : T )
   x = { int( g() % 2 ) , int( g() % 2 ) , int( g() % 2 ) , int( g() % 2 ) };
  ( void ) dominoToClique( T , n );
  ( void ) dominoToSat( T , n );
  ( void ) dominoToLp01( T , n );
  const int m = 1 + int( g() % 6 );
  const auto G = random_graph( m , 50 , g );
  CHECK( complement( complement( G ) ) == G );
  ( void ) hamiltonToTsp( G );
  ( void ) cycleToChain( random_digraph( m , 50 , g ) );
  Longs A( g() % 6 );
  for( auto & a : A ) a = long( g() % 10 );
  ( void ) halfSum( A );
  ( void ) sortToHull( A );
  }
 }

// the polynomial twins of Tabella 19.1: inputs checked, solutions checked
ADS_TEST( "easy-cases" )
{
 CHECK_THROWS( literalNode( 0 ) , contract_violation );
 CHECK_THROWS( twoSat( { { 1 , 2 , 3 } } , 3 ) , contract_violation );
 CHECK_THROWS( twoSat( { { 1 , 4 } } , 3 ) , contract_violation );
 CHECK_THROWS( twoColoring( arc ) , contract_violation );
 CHECK_THROWS( bipartiteMatching( 1 , 1 , { { 0 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( intervalGraph( { { 3 , 1 } } ) , contract_violation );
 CHECK_THROWS( intervalClique( { { 3 , 1 } } ) , contract_violation );
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 6 );
  Cnf F( 1 + g() % 8 );
  for( auto & c : F ) {
   c.resize( 1 + g() % 2 );
   for( auto & l : c ) l = ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 );
   }
  CHECK( twoSat( F , n ).has_value() == satisfiable( F , n ).has_value() );
  const auto G = random_graph( n , 40 , g );
  CHECK( twoColoring( G ).has_value() == coloring( G , 2 ).has_value() );
  std::vector< std::pair< int , int > > E;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) if( g() % 3 == 0 ) E.emplace_back( i , j );
  ( void ) bipartiteMatching( n , n , E );
  std::vector< std::pair< long , long > > I( n );
  for( auto & [ a , b ] : I ) {
   a = long( g() % 10 );
   b = a + long( g() % 5 );
   }
  CHECK( clique( intervalGraph( I ) , intervalClique( I ) ).has_value() );
  }
 }

// the square tiling, whose covering legalTiling() checks, and choice()
ADS_TEST( "domino-nondeterminism" )
{
 Chooser c;
 CHECK_THROWS( c.choice( 0 ) , contract_violation );
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  std::vector< Tile > D( 1 + g() % 3 );
  for( auto & x : D )
   x = { int( g() % 2 ) , int( g() % 2 ) , int( g() % 2 ) , int( g() % 2 ) };
  ( void ) squareTiling( D , 1 + int( g() % 3 ) );
  const int k = 1 + int( g() % 4 );
  long runs = 0;
  CHECK( ! c.run( [ & ]( Chooser & ch ) { ( void ) ch.choice( k ) ;
                                          return( false ); } , &runs ) );
  CHECK( runs == k );
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
/*-------------------- End File contract_complexity.cpp --------------------*/
/*--------------------------------------------------------------------------*/
