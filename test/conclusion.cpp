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
#include <cmath>
#include <limits>
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
// enumeration, and the 3-SODDISFATTIBILITA' of the same variables; no
// clause (with no variable or some), a tautology, a repeated literal, the
// contradictions of one and two variables, a chain of 10^5 implications,
// and the preconditions (n < 0, the literal 0 or beyond n, clauses of 0
// or 3 literals)
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
 CHECK( twoSat( Cnf() , 0 ) && twoSat( Cnf() , 0 )->empty() );
 CHECK( twoSat( Cnf() , 3 ) && twoSat( Cnf() , 3 )->size() == 3 );
 const auto t = twoSat( { { 1 , -1 } , { 2 , 2 } } , 2 );   // tautology,
 CHECK( t && ( *t )[ 1 ] );                                  // x2 or x2
 CHECK( ! twoSat( { { 1 } , { -1 } } , 1 ) );               // x and not x
 CHECK( ! twoSat( { { 1 , 2 } , { 1 , -2 } , { -1 , 2 } , { -1 , -2 } } ,
                  2 ) );                                    // all 4 pairs
 const int n = 100000;                       // x1 -> x2 -> ... -> xn: a
 Cnf C = { { 1 } };                          // chain of 2 n implications
 for( int i = 1 ; i < n ; ++i ) C.push_back( { -i , i + 1 } );
 const auto all = twoSat( C , n );
 CHECK( all && std::all_of( all->begin() , all->end() , []( bool b ) {
         return( b ); } ) );
 C.push_back( { -n } );
 CHECK( ! twoSat( C , n ) );
 CHECK_THROWS( twoSat( Cnf() , -1 ) , contract_violation );
 CHECK_THROWS( twoSat( { { 0 , 1 } } , 1 ) , contract_violation );
 CHECK_THROWS( twoSat( { { 2 } } , 1 ) , contract_violation );
 CHECK_THROWS( twoSat( { {} } , 1 ) , contract_violation );
 CHECK_THROWS( twoSat( { { 1 , 1 , 1 } } , 1 ) , contract_violation );
 CHECK_THROWS( literalNode( 0 ) , contract_violation );
 CHECK( literalNode( 1 ) == 0 && literalNode( -1 ) == 1 &&
        literalNode( -3 ) == 5 );
 }

// 2-COLORAZIONE by a visit against the 2- and 3-colorings by enumeration;
// no node, one, self-loops (ignored, as by coloring()), even and odd
// cycles up to 1001
// nodes, a bipartite component next to an odd one, isolated nodes, and a
// graph not square or not symmetric refused
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
 const Adjacency E , One( 1 , std::vector< bool >( 1 , false ) );
 CHECK( twoColoring( E ) && twoColoring( E )->empty() );
 CHECK( twoColoring( One ) && twoColoring( One )->size() == 1 );
 auto cycle = []( int n ) {
  Adjacency G( n , std::vector< bool >( n , false ) );
  for( int u = 0 ; u < n ; ++u )
   G[ u ][ ( u + 1 ) % n ] = G[ ( u + 1 ) % n ][ u ] = true;
  return( G );
  };
 Adjacency L = cycle( 4 );                       // the self-loops ignored,
 for( int u = 0 ; u < 4 ; ++u ) L[ u ][ u ] = true;   // as by coloring()
 CHECK( twoColoring( L ) && coloring( L , 2 ) );
 CHECK( twoColoring( cycle( 1000 ) ) && ! twoColoring( cycle( 1001 ) ) );
 CHECK( ! twoColoring( cycle( 3 ) ) && coloring( cycle( 3 ) , 3 ) );
 Adjacency D( 7 , std::vector< bool >( 7 , false ) );   // a path of 4
 for( int u = 0 ; u < 3 ; ++u ) D[ u ][ u + 1 ] = D[ u + 1 ][ u ] = true;
 CHECK( twoColoring( D ) );                      // and 3 isolated nodes
 D[ 4 ][ 5 ] = D[ 5 ][ 4 ] = D[ 5 ][ 6 ] = D[ 6 ][ 5 ] = true;
 D[ 4 ][ 6 ] = D[ 6 ][ 4 ] = true;               // a triangle apart
 CHECK( ! twoColoring( D ) );
 Adjacency N = One;
 N.push_back( { true } );                        // not square
 CHECK_THROWS( twoColoring( N ) , contract_violation );
 Adjacency A( 2 , std::vector< bool >( 2 , false ) );
 A[ 0 ][ 1 ] = true;                             // not symmetric
 CHECK_THROWS( twoColoring( A ) , contract_violation );
 }

// ABBINAMENTO 2-DIMENSIONALE by the maximum flow against all the subsets
// of edges, and the 3-dimensional matching of the triples that extend the
// pairs by a third coordinate equal to the first one, the same answer; no
// node on a side, no edge, a repeated edge, K( 3 , 5 ), a star, a chain of
// 2000 pairs, and the ends out of their sides refused; q = 0 matched by
// no triple, a triple out of [ 0 , q ) refused
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
 using Es = std::vector< std::pair< int , int > >;
 CHECK( bipartiteMatching( 0 , 0 , {} ).empty() &&
        bipartiteMatching( 0 , 3 , {} ).empty() &&
        bipartiteMatching( 3 , 3 , {} ).empty() );
 CHECK( bipartiteMatching( 1 , 1 , { { 0 , 0 } , { 0 , 0 } } ).size() == 1 );
 Es K;                                           // K( 3 , 5 ): 3 pairs
 for( int i = 0 ; i < 3 ; ++i )
  for( int j = 0 ; j < 5 ; ++j ) K.emplace_back( i , j );
 CHECK( bipartiteMatching( 3 , 5 , K ).size() == 3 );
 Es S;                                           // a star: 1 pair
 for( int j = 0 ; j < 6 ; ++j ) S.emplace_back( 0 , j );
 CHECK( bipartiteMatching( 4 , 6 , S ).size() == 1 );
 const int m = 2000;                             // a long augmenting path
 Es P;
 for( int i = 0 ; i < m ; ++i ) {
  P.emplace_back( i , i );
  if( i + 1 < m ) P.emplace_back( i + 1 , i );
  }
 CHECK( int( bipartiteMatching( m , m , P ).size() ) == m );
 CHECK_THROWS( bipartiteMatching( -1 , 2 , {} ) , contract_violation );
 CHECK_THROWS( bipartiteMatching( 2 , 2 , { { 0 , 2 } } ) ,
               contract_violation );
 CHECK_THROWS( bipartiteMatching( 2 , 2 , { { -1 , 0 } } ) ,
               contract_violation );
 CHECK( threeDimensionalMatching( {} , 0 ).has_value() &&
        threeDimensionalMatching( {} , 0 )->empty() );
 CHECK( ! threeDimensionalMatching( {} , 1 ) );
 CHECK( threeDimensionalMatching( { { 0 , 0 , 0 } } , 1 ) );
 CHECK( ! threeDimensionalMatching( { { 0 , 0 , 0 } , { 1 , 1 , 0 } } ,
                                    2 ) );         // the third coordinate
 CHECK_THROWS( threeDimensionalMatching( { { 0 , 0 , 1 } } , 1 ) ,
               contract_violation );
 }

// CIRCUITO EULERIANO in polynomial time and CIRCUITO HAMILTONIANO by
// enumeration: an Eulerian circuit uses each edge once, a Hamiltonian one
// each node once, and a cycle of n nodes has both; the empty Eulerian
// cycle of a graph with no edge, two opposite arcs, two disjoint cycles
// (none)
// then joined, a cycle of 10^5 nodes, an unoriented graph refused; no
// Hamiltonian circuit below 3 nodes, even with self-loops, or on a path
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
 ListGraph< int > N( 4 );                        // no edge: empty
 CHECK( eulerianCycle( N ) && eulerianCycle( N )->empty() );
 ListGraph< int > S( 3 );                        // two arcs, both ways
 S.insertEdge( 1 , 2 );
 S.insertEdge( 2 , 1 );
 const auto e2 = eulerianCycle( S );
 CHECK( e2 && e2->size() == 3 && isEulerianCycle( S , *e2 ) );
 ListGraph< int > T( 6 );                        // two disjoint triangles
 for( int u = 0 ; u < 3 ; ++u ) {
  T.insertEdge( u , ( u + 1 ) % 3 );
  T.insertEdge( 3 + u , 3 + ( u + 1 ) % 3 );
  }
 CHECK( ! eulerianCycle( T ) );
 T.insertEdge( 0 , 3 );                          // joined both ways: one
 T.insertEdge( 3 , 0 );                          // cycle of 8 edges
 const auto e8 = eulerianCycle( T );
 CHECK( e8 && e8->size() == 9 && isEulerianCycle( T , *e8 ) );
 const int m = 100000;                           // a cycle of 10^5 nodes
 ListGraph< int > R( m );
 for( int u = 0 ; u < m ; ++u ) R.insertEdge( u , ( u + 1 ) % m );
 const auto er = eulerianCycle( R );
 CHECK( er && int( er->size() ) == m + 1 );
 CHECK_THROWS( eulerianCycle( ListGraph< int >( 2 , false ) ) ,
               contract_violation );
 for( int n : { 0 , 1 , 2 } )                    // no circuit below 3
  CHECK( ! hamiltonianCircuit( Adjacency( n , std::vector< bool >( n ,
                                                                 true ) ) ) );
 Adjacency P( 4 , std::vector< bool >( 4 , false ) );   // a path
 for( int u = 0 ; u < 3 ; ++u ) P[ u ][ u + 1 ] = P[ u + 1 ][ u ] = true;
 CHECK( ! hamiltonianCircuit( P ) );
 P[ 3 ][ 0 ] = P[ 0 ][ 3 ] = true;
 CHECK( hamiltonianCircuit( P ) );
 }

// ZAINO REALE above ZAINO INTERO, by less than the largest profit; a
// capacity of 0, room for all (the same), no object, an object too big
// (taken in part by the real one), the classical gap 22 against 24, and
// the preconditions (a volume of 0 for the real one, a negative capacity,
// sizes that differ)
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
  const long top = *std::max_element( p.begin() , p.end() );
  CHECK( fp <= double( knapsack( p , v , C ).first + top ) + 1e-9 );
  }
 auto gain = []( const std::vector< double > & x , const Longs & p ) {
  double s = 0;
  for( std::size_t i = 0 ; i < x.size() ; ++i ) s += x[ i ] * double( p[ i ] );
  return( s ); };
 const Longs p = { 6 , 10 , 12 } , v = { 1 , 2 , 3 };
 const std::vector< double > P( p.begin() , p.end() ) ,
                             V( v.begin() , v.end() );
 CHECK( knapsack( p , v , 0 ).first == 0 );      // no room
 CHECK( gain( fractionalKnapsack( P , V , 0.0 ) , p ) == 0.0 );
 CHECK( knapsack( p , v , 6 ).first == 28 );     // all fit: the same
 CHECK( gain( fractionalKnapsack( P , V , 100.0 ) , p ) == 28.0 );
 CHECK( knapsack( Longs() , Longs() , 5 ).first == 0 &&
        fractionalKnapsack( {} , {} , 5.0 ).empty() );
 CHECK( knapsack( { 9 } , { 7 } , 6 ).first == 0 );   // too big, but
 CHECK( std::abs( gain( fractionalKnapsack( { 9.0 } , { 7.0 } , 6.0 ) ,
                        { 9 } ) - 9.0 * 6 / 7 ) < 1e-12 );   // a part
 CHECK( knapsack( p , v , 5 ).first == 22 );     // the classical gap
 CHECK( gain( fractionalKnapsack( P , V , 5.0 ) , p ) == 24.0 );
 CHECK_THROWS( fractionalKnapsack( { 1.0 } , { 0.0 } , 1.0 ) ,
               contract_violation );
 CHECK_THROWS( fractionalKnapsack( P , V , -1.0 ) , contract_violation );
 CHECK_THROWS( fractionalKnapsack( { 1.0 } , V , 1.0 ) ,
               contract_violation );
 CHECK_THROWS( knapsack( p , v , -1 ) , contract_violation );
 CHECK_THROWS( knapsack( { 1 } , v , 1 ) , contract_violation );
 }

// MINIMO ALBERO DI COPERTURA by Kruskal and with at most 2 connections
// per node by enumeration: a spanning tree exists if and only if the
// graph is connected, the bounded one is a Hamiltonian chain; one node
// (no edge, also with k = 0), two nodes apart or joined with k = 0 and 1,
// a star of 4 leaves with k = 3 and 4 (a self-loop no help), and a graph
// not symmetric refused
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
 const Adjacency One( 1 , std::vector< bool >( 1 , false ) );
 CHECK( boundedSpanningTree( One , 0 ) &&       // one node: no edge
        boundedSpanningTree( One , 0 )->empty() );
 Adjacency Two( 2 , std::vector< bool >( 2 , false ) );
 CHECK( ! boundedSpanningTree( Two , 1 ) );      // disconnected
 Two[ 0 ][ 1 ] = Two[ 1 ][ 0 ] = true;
 CHECK( ! boundedSpanningTree( Two , 0 ) && boundedSpanningTree( Two , 1 ) );
 Adjacency Star( 5 , std::vector< bool >( 5 , false ) );   // the center 0
 for( int u = 1 ; u < 5 ; ++u ) Star[ 0 ][ u ] = Star[ u ][ 0 ] = true;
 CHECK( ! boundedSpanningTree( Star , 3 ) && boundedSpanningTree( Star , 4 ) );
 Star[ 1 ][ 1 ] = true;                          // a self-loop: no help
 CHECK( ! boundedSpanningTree( Star , 3 ) );
 Adjacency A = Two;
 A[ 1 ][ 0 ] = false;
 CHECK_THROWS( boundedSpanningTree( A , 1 ) , contract_violation );
 }

// on the graphs of intervals CRICCA, INSIEME INDIPENDENTE and COLORAZIONE
// are polynomial: the largest clique is the most intervals through a
// point, the greedy independent set is the largest, and the colors of the
// sweep are as many as the clique; with empty intervals [ a , a ), which
// meet none, the clique of the graph still the one of the sweep; no
// interval, intervals that touch (half open: apart), all equal, nested,
// at the extremes of long, and the preconditions (b < a, and a = b for
// the greedy set and the coloring)
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
 for( int t = 0 ; t < 200 ; ++t ) {              // with empty intervals
  std::vector< std::pair< long , long > > I( g() % 8 );
  for( auto & [ a , b ] : I ) {
   a = long( g() % 10 );
   b = a + long( g() % 3 );
   }
  const int w = intervalClique( I );
  const auto G = intervalGraph( I );
  CHECK( ( w == 0 ) == std::all_of( I.begin() , I.end() , []( auto x ) {
          return( x.first == x.second ); } ) );
  CHECK( ( w == 0 ) || ( clique( G , w ) && ! clique( G , w + 1 ) ) );
  }
 using Is = std::vector< std::pair< long , long > >;
 CHECK( intervalClique( {} ) == 0 && intervalGraph( {} ).empty() &&
        independentSet( Is() ).empty() && intervalColoring( {} ).empty() );
 const Is touch = { { 0 , 2 } , { 2 , 4 } , { 4 , 6 } };   // [ a , b )
 CHECK( intervalClique( touch ) == 1 && ! intervalGraph( touch )[ 0 ][ 1 ] &&
        independentSet( touch ).size() == 3 &&
        intervalColoring( touch ) == std::vector< int >( 3 , 0 ) );
 const Is same( 5 , { 3 , 7 } );                 // equal: a clique of 5
 const auto cs = intervalColoring( same );
 CHECK( intervalClique( same ) == 5 && independentSet( same ).size() == 1 &&
        *std::max_element( cs.begin() , cs.end() ) == 4 );
 const Is nest = { { 0 , 10 } , { 1 , 9 } , { 2 , 8 } , { 20 , 21 } };
 CHECK( intervalClique( nest ) == 3 && independentSet( nest ).size() == 2 );
 const Is empty = { { 3 , 3 } , { 2 , 5 } , { 3 , 3 } };   // [ 3 , 3 )
 CHECK( intervalClique( empty ) == 1 && ! intervalGraph( empty )[ 0 ][ 1 ] &&
        ! intervalGraph( empty )[ 0 ][ 2 ] );
 constexpr long lo = std::numeric_limits< long >::min() ,
                hi = std::numeric_limits< long >::max();
 const Is wide = { { lo , hi } , { lo , 0 } , { 0 , hi } };
 CHECK( intervalClique( wide ) == 2 && independentSet( wide ).size() == 2 );
 CHECK_THROWS( intervalGraph( { { 2 , 1 } } ) , contract_violation );
 CHECK_THROWS( intervalClique( { { 2 , 1 } } ) , contract_violation );
 CHECK_THROWS( independentSet( Is( { { 2 , 2 } } ) ) , contract_violation );
 CHECK_THROWS( intervalColoring( { { 2 , 2 } } ) , contract_violation );
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
