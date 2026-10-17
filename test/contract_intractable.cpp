/*--------------------------------------------------------------------------*/
/*--------------------- File contract_intractable.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the techniques for the
 * intractable problems: each precondition violated must throw
 * ADS::contract_violation, the triangle inequality of deltaTsp() included,
 * and on random instances, at the level 3, each solution of the
 * pseudopolynomial, approximation, branch-&-bound and heuristic algorithms
 * is checked as feasible by the postconditions, and the structure with
 * buckets keeps its invariant.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 19,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
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

#include "ADS/intractable/Approximation.h"
#include "ADS/intractable/BranchAndBound.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/intractable/Heuristics.h"
#include "ADS/intractable/Pseudopolynomial.h"

#include <random>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Longs = std::vector< long >;
using Matrix = std::vector< Longs >;

/// random distances in [ 1 , w ], symmetric
Matrix random_distances( int n , long w , std::mt19937 & g )
{
 Matrix D( n , Longs( n , 0 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v ) D[ u ][ v ] = D[ v ][ u ] = 1 + g() % w;
 return( D );
 }

/// a random unoriented graph, each edge with probability p percent
Adjacency random_graph( int n , int p , std::mt19937 & g )
{
 Adjacency G( n , std::vector< bool >( n , false ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( int( g() % 100 ) < p ) G[ u ][ v ] = G[ v ][ u ] = true;
 return( G );
 }

const Adjacency arc = { { false , true } , { false , false } };
const Matrix skew = { { 0 , 1 } , { 2 , 0 } };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// subset sum, partition, scheduling, McNaughton and the three thieves
ADS_TEST( "pseudopolynomial" )
{
 CHECK_THROWS( subsetSumDP( { 1 , 2 } , -1 ) , contract_violation );
 CHECK_THROWS( subsetSumDP( { -1 } , 2 ) , contract_violation );
 CHECK_THROWS( subsetSumSet( { -1 } , 2 ) , contract_violation );
 CHECK_THROWS( partitionDP( { -1 , 1 } ) , contract_violation );
 CHECK_THROWS( schedulingDP( { 1 } , 0 , 3 ) , contract_violation );
 CHECK_THROWS( mcNaughton( { 1.0 , -1.0 } , 2 ) , contract_violation );
 CHECK_THROWS( mcNaughton( { 1.0 } , 0 ) , contract_violation );
 CHECK_THROWS( realTimeScheduling( { 1.0 } , { 0.0 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( realTimeScheduling( { 1.0 } , { 1.0 , 2.0 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( bottino( { 3 , -3 , 3 } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs A( g() % 8 );
  for( auto & a : A ) a = long( g() % 12 );
  std::vector< std::vector< bool > > rows;
  CHECK( subsetSumDP( A , long( g() % 30 ) , &rows ) ||
         ( rows.size() == A.size() + 1 ) );
  ( void ) subsetSumSet( A , long( g() % 30 ) );
  ( void ) partitionDP( A );
  ( void ) schedulingDP( A , 1 + int( g() % 3 ) , long( g() % 20 ) );
  ( void ) bottino( A );
  std::vector< double > T( g() % 8 );
  for( auto & x : T ) x = double( g() % 100 ) / 10;
  ( void ) mcNaughton( T , 1 + int( g() % 4 ) );
  }
 }

// ff, the circuit of the spanning tree on triangular distances only, the
// gap, apSubsetSum, the approximate knapsack and the cover
ADS_TEST( "approximation" )
{
 CHECK_THROWS( firstFit( { 3 , 7 } , 5 ) , contract_violation );
 CHECK_THROWS( tourCost( skew , { 0 , 0 } ) , contract_violation );
 CHECK_THROWS( deltaTsp( skew ) , contract_violation );
 const Matrix far = { { 0 , 1 , 9 } , { 1 , 0 , 1 } , { 9 , 1 , 0 } };
 CHECK_THROWS( deltaTsp( far ) , contract_violation );   // 9 > 1 + 1
 CHECK_THROWS( tspGap( arc , 1 ) , contract_violation );
 CHECK_THROWS( tspGap( Adjacency( 2 , std::vector< bool >( 2 ) ) , 0 ) ,
               contract_violation );
 CHECK_THROWS( apSubsetSum( { 1 } , 1 , -1 ) , contract_violation );
 CHECK_THROWS( knapsackApprox( { 1 } , { 1 } , 1 , 0.0 ) ,
               contract_violation );
 CHECK_THROWS( knapsackApprox( { 1 } , { 1 , 1 } , 1 , 0.5 ) ,
               contract_violation );
 CHECK_THROWS( vertexCoverApprox( arc ) , contract_violation );
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const long k = 5 + long( g() % 10 );
  Longs A( 1 + g() % 9 );
  for( auto & a : A ) a = 1 + long( g() % k );
  ( void ) firstFit( A , k , g() % 2 );
  ( void ) apSubsetSum( A , long( g() % 40 ) , int( g() % 3 ) );
  ( void ) knapsackApprox( A , A , long( g() % 30 ) ,
                           0.05 + double( g() % 90 ) / 100 );
  const int n = 1 + int( g() % 7 );
  Matrix D( n , Longs( n , 0 ) );          // on a line: triangular
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v ) D[ u ][ v ] = std::labs( 3L * ( u - v ) );
  ( void ) deltaTsp( D );
  ( void ) vertexCoverApprox( random_graph( n , 50 , g ) );
  ( void ) tspGap( random_graph( n , 50 , g ) , 1 + long( g() % 3 ) );
  }
 }

// the lower bounds, bbTsp() and littleTsp(), the clique of largest
// weight and the knapsack
ADS_TEST( "branch-and-bound" )
{
 CHECK_THROWS( bbTsp( skew , { 0 , 0 } ) , contract_violation );
 CHECK_THROWS( bbTsp( skew , {} , TspBound::ONE_TREE ) , contract_violation );
 CHECK_THROWS( tspLowerBound( skew , {} ) , contract_violation );
 CHECK_THROWS( oneTree( skew , 0 ) , contract_violation );
 std::mt19937 g0( 0 );
 const auto D3 = random_distances( 3 , 5 , g0 );
 CHECK_THROWS( oneTree( D3 , 3 ) , contract_violation );
 CHECK_THROWS( littleTsp( Matrix( { { 0 , 1 } } ) ) , contract_violation );
 CHECK_THROWS( maxWeightClique( arc , { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( maxWeightClique( Adjacency( 2 , std::vector< bool >( 2 ) ) ,
                                { 1 } ) , contract_violation );
 CHECK_THROWS( knapsackBB( { 1 } , { 0 } , 1 ) , contract_violation );
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 1 + int( g() % 6 );
  const auto D = random_distances( n , 20 , g );
  const auto a = bbTsp( D ) , b = bbTsp( D , {} , TspBound::ONE_TREE ) ,
             c = littleTsp( D );
  CHECK( a.first == b.first && a.first == c.first );
  Longs w( n );
  for( auto & x : w ) x = long( g() % 10 );
  ( void ) maxWeightClique( random_graph( n , 50 , g ) , w );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = long( g() % 20 );
   v[ i ] = 1 + long( g() % 10 );
   }
  ( void ) knapsackBB( p , v , long( g() % 30 ) );
  }
 }

// the configurations of the fifteen puzzle and its solutions
ADS_TEST( "fifteen-puzzle" )
{
 CHECK_THROWS( puzzleGoal( 0 ) , contract_violation );
 CHECK_THROWS( manhattan( { 1 , 1 , 2 , 0 } , 2 ) , contract_violation );
 CHECK_THROWS( puzzleMoves( { 0 , 1 , 2 } , 2 ) , contract_violation );
 CHECK_THROWS( solvable( { 0 , 1 , 2 , 4 } , 2 ) , contract_violation );
 CHECK_THROWS( solvePuzzle( {} , 3 ) , contract_violation );
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 2 + int( g() % 2 );
  Board B = puzzleGoal( n );
  for( int m = 0 ; m < 12 ; ++m ) {
   const auto N = puzzleMoves( B , n );
   B = N[ g() % N.size() ];
   }
  CHECK( solvePuzzle( B , n ).has_value() );
  }
 }

// greedyTsp(), the local search, the partition, min-max-greedy, the
// superstring, the star alignment and the saturation degree
ADS_TEST( "heuristics" )
{
 std::mt19937 g0( 0 );
 const auto D3 = random_distances( 3 , 5 , g0 );
 CHECK_THROWS( greedyTsp( Matrix( 2 , Longs( 2 , 1 ) ) ) ,
               contract_violation );
 CHECK_THROWS( twoOptMove( { 0 , 1 , 2 } , 1 , 1 ) , contract_violation );
 CHECK_THROWS( twoOpt( D3 , { 0 , 0 , 1 } ) , contract_violation );
 CHECK_THROWS( kOptNeighbours( 2 , 1 ) , contract_violation );
 CHECK_THROWS( partitionLocalSearch( { 1 , 2 } , { 0 , 2 } ) ,
               contract_violation );
 CHECK_THROWS( minMaxGreedy( Adjacency( 3 , std::vector< bool >( 3 ) ) , 0 ,
                             1 ) , contract_violation );
 CHECK_THROWS( minMaxGreedy( Adjacency( 2 , std::vector< bool >( 2 ) ) , 0 ,
                             0 ) , contract_violation );
 CHECK_THROWS( cutSize( arc , { 0 } ) , contract_violation );
 CHECK_THROWS( starAlignment( {} ) , contract_violation );
 CHECK_THROWS( dsatur( arc ) , contract_violation );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 6 );
  const auto D = random_distances( n , 30 , g );
  Nodes pi = greedyTsp( D ).second;
  std::shuffle( pi.begin() , pi.end() , g );
  ( void ) twoOpt( D , pi );
  Longs A( n );
  for( auto & a : A ) a = long( g() % 20 );
  std::vector< int > side( n );
  for( auto & s : side ) s = int( g() % 2 );
  ( void ) partitionLocalSearch( A , side );
  const auto G = random_graph( 2 * ( n / 2 ) , 40 , g );
  ( void ) minMaxGreedy( G , 0 , 1 );
  ( void ) dsatur( G );
  std::vector< std::string > P( 1 + g() % 4 );
  for( auto & s : P ) {
   s.resize( 1 + g() % 5 );
   for( auto & ch : s ) ch = "ACGT"[ g() % 4 ];
   }
  ( void ) greedySuperstring( P );
  ( void ) starAlignment( P );
  }
 }

// the structure with buckets: its preconditions, and its invariant on
// random operations
ADS_TEST( "buckets" )
{
 CHECK_THROWS( Buckets( -1 ) , contract_violation );
 Buckets B( 3 );
 CHECK_THROWS( ( void ) B.choose( 0 ) , contract_violation );
 B.insert( 1 );
 CHECK_THROWS( B.insert( 1 ) , contract_violation );
 CHECK_THROWS( B.remove( 2 ) , contract_violation );
 CHECK_THROWS( B.inc( 2 , 1 ) , contract_violation );
 CHECK_THROWS( ( void ) B.count( 0 , 3 ) , contract_violation );
 CHECK_THROWS( ( void ) B.dim( 1 , 4 ) , contract_violation );
 for( int i = 0 ; i < 3 ; ++i ) B.inc( 0 , 1 );
 CHECK_THROWS( B.inc( 0 , 1 ) , contract_violation );   // n edges already
 CHECK( B.choose( 1 ) == 1 && B.max( 0 ) == 3 );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 1 + int( g() % 10 );
  Buckets C( n );
  for( int op = 0 ; op < 200 ; ++op ) {
   const int u = int( g() % n );
   if( ! C.contains( u ) ) C.insert( u );
   else if( g() % 4 == 0 ) C.remove( u );
   else {
    const int k = int( g() % 2 );
    if( C.count( k , u ) < n ) C.inc( k , u );
    }
   }
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
/*------------------- End File contract_intractable.cpp --------------------*/
/*--------------------------------------------------------------------------*/
