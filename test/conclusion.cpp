/*--------------------------------------------------------------------------*/
/*-------------------------- File conclusion.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the Conclusione: the pairs of Tabella 19.1, a problem solvable
 * in polynomial time against its NP-hard twin on random instances, and
 * CRICCA, INSIEME INDIPENDENTE and COLORAZIONE on the graphs of intervals,
 * polynomial there, against the enumerations of chapter 18.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Conclusione,
 *      https://algoritmicamente.it
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

#include "ADS/complexity/EasyCases.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/EulerianCycle.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"

#include <algorithm>
#include <array>
#include <random>
#include <vector>

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

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// 2-SODDISFATTIBILITA' by the components of the implications, against the
// enumeration, and the 3-SODDISFATTIBILITA' of the same variables
ADS_TEST( "table19.1.sat" )
{
 std::mt19937 g( 1 );
 int yes = 0;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 6 );
  Cnf F( 1 + g() % ( 3 * n ) );
  for( auto & c : F ) {
   c.resize( 1 + g() % 2 );
   for( auto & l : c ) l = ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 );
   }
  const auto x = twoSat( F , n );
  CHECK( x.has_value() == satisfiable( F , n ).has_value() );
  if( x ) CHECK( satCertificate( F , *x ) );
  yes += x.has_value();
  }
 CHECK( yes > 50 && yes < 250 );
 }

// 2-COLORAZIONE by a visit against the 2- and 3-colorings by enumeration
ADS_TEST( "table19.1.coloring" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  const auto G = random_graph( n , int( g() % 50 ) , g );
  const auto c = twoColoring( G );
  CHECK( c.has_value() == coloring( G , 2 ).has_value() );
  if( c )
   for( int u = 0 ; u < n ; ++u )
    for( int v = 0 ; v < n ; ++v )
     CHECK( ! G[ u ][ v ] || ( *c )[ u ] != ( *c )[ v ] );
  if( c ) CHECK( coloring( G , 3 ).has_value() );
  }
 }

// ABBINAMENTO 2-DIMENSIONALE by the maximum flow against all the subsets
// of edges, and the 3-dimensional matching of the triples that extend the
// pairs by a third coordinate equal to the first one, the same answer
ADS_TEST( "table19.1.matching" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int q = 1 + int( g() % 4 );
  std::vector< std::pair< int , int > > E;
  for( int i = 0 ; i < q ; ++i )
   for( int j = 0 ; j < q ; ++j )
    if( g() % 3 == 0 ) E.emplace_back( i , j );
  const auto M = bipartiteMatching( q , q , E );
  std::vector< bool > L( q , false ) , R( q , false );
  for( auto [ i , j ] : M ) {
   CHECK( ! L[ i ] && ! R[ j ] );
   L[ i ] = R[ j ] = true;
   }
  std::size_t best = 0;
  for( unsigned S = 0 ; S < ( 1u << E.size() ) ; ++S ) {
   std::vector< bool > l( q , false ) , r( q , false );
   bool ok = true;
   std::size_t k = 0;
   for( std::size_t e = 0 ; e < E.size() && ok ; ++e )
    if( S >> e & 1 ) {
     ok = ! l[ E[ e ].first ] && ! r[ E[ e ].second ];
     l[ E[ e ].first ] = r[ E[ e ].second ] = true;
     ++k;
     }
   if( ok ) best = std::max( best , k );
   }
  CHECK( M.size() == best );
  std::vector< std::array< int , 3 > > T;
  for( auto [ i , j ] : E ) T.push_back( { i , j , i } );
  CHECK( threeDimensionalMatching( T , q ).has_value() ==
         ( int( best ) == q ) );
  }
 }

// CIRCUITO EULERIANO in polynomial time and CIRCUITO HAMILTONIANO by
// enumeration: an Eulerian circuit uses each edge once, a Hamiltonian one
// each node once, and a cycle of n nodes has both
ADS_TEST( "table19.1.circuits" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 5 );
  ListGraph< int > C( n );
  Adjacency A( n , std::vector< bool >( n , false ) );
  for( int u = 0 ; u < n ; ++u ) {
   C.insertEdge( u , ( u + 1 ) % n );
   A[ u ][ ( u + 1 ) % n ] = A[ ( u + 1 ) % n ][ u ] = true;
   }
  const auto e = eulerianCycle( C );
  CHECK( e && int( e->size() ) == n + 1 && e->front() == e->back() );
  CHECK( hamiltonianCircuit( A ).has_value() );
  const int extra = int( g() % n ) , to = ( extra + 2 ) % n;
  C.insertEdge( extra , to );               // in and out degrees differ
  CHECK( ! eulerianCycle( C ) );
  }
 }

// ZAINO REALE above ZAINO INTERO
ADS_TEST( "table19.1.knapsack" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 10 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + long( g() % 30 );
   v[ i ] = 1 + long( g() % 15 );
   }
  const long C = long( g() % 50 );
  const std::vector< double > P( p.begin() , p.end() ) ,
                              V( v.begin() , v.end() );
  const auto x = fractionalKnapsack( P , V , double( C ) );
  double fp = 0;
  for( int i = 0 ; i < n ; ++i ) fp += x[ i ] * P[ i ];
  CHECK( fp >= double( knapsack( p , v , C ).first ) - 1e-9 );
  }
 }

// MINIMO ALBERO DI COPERTURA by Kruskal and with at most 2 connections
// per node by enumeration: a spanning tree exists if and only if the
// graph is connected, the bounded one is a Hamiltonian chain
ADS_TEST( "table19.1.spanning-tree" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 7 );
  const auto G = random_graph( n , 50 , g );
  std::vector< WeightedEdge< long > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( G[ u ][ v ] ) E.push_back( { u , v , 1 } );
  const bool connected = int( kruskal( n , E ).size() ) == n - 1;
  CHECK( boundedSpanningTree( G , n ).has_value() == connected );
  if( boundedSpanningTree( G , 2 ) ) CHECK( connected );
  }
 }

// on the graphs of intervals CRICCA, INSIEME INDIPENDENTE and COLORAZIONE
// are polynomial: the largest clique is the most intervals through a
// point, the greedy independent set is the largest, and the colors of the
// sweep are as many as the clique
ADS_TEST( "intervals" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 200 ; ++t ) {
  std::vector< std::pair< long , long > > I( 1 + g() % 8 );
  for( auto & [ a , b ] : I ) {
   a = long( g() % 20 );
   b = a + 1 + long( g() % 8 );
   }
  const auto G = intervalGraph( I );
  const int n = int( I.size() ) , w = intervalClique( I );
  CHECK( clique( G , w ).has_value() && ! clique( G , w + 1 ).has_value() );
  const int s = int( independentSet( I ).size() );
  CHECK( ADS::independentSet( G , s ).has_value() &&
         ! ADS::independentSet( G , s + 1 ).has_value() );
  const auto col = intervalColoring( I );
  CHECK( 1 + *std::max_element( col.begin() , col.end() ) == w );
  CHECK( coloring( G , w ).has_value() );
  CHECK( ( w == 1 ) || ! coloring( G , w - 1 ) );
  CHECK( n >= w );
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
/*------------------------ End File conclusion.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
