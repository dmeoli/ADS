/*--------------------------------------------------------------------------*/
/*--------------------------- File Reductions.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial reductions of chapter 18: a function f from the inputs
 * of A to the inputs of B, computable in polynomial time, such that x is a
 * YES of A if and only if f( x ) is a YES of B; then A is no harder than B
 * (A \f$ \propto \f$ B).
 *
 * - DOMINO LIMITATO to CRICCA (Teorema 18.2): a node u_hij for the tile h
 *   in the square ( i , j ), only u_000 in ( 0 , 0 ), joined to the nodes
 *   of the squares not adjacent and to those of the adjacent squares whose
 *   tile fits, and k = n^2;
 * - DOMINO LIMITATO to SODDISFATTIBILITA' (Teorema 18.3): u_000, a tile at
 *   least and one at most in each square, and no two tiles that do not fit
 *   side by side;
 * - DOMINO LIMITATO to PROGRAMMAZIONE LINEARE 0/1 (Teorema 18.4): x_000 =
 *   1, one tile per square, and x_hij plus the tiles that do not fit next
 *   to it at most 1 (the example of the text writes one inequality per
 *   pair, which is the same since a square holds one tile);
 * - CRICCA to INSIEME INDIPENDENTE (Teorema 18.5), the complement graph;
 *   INSIEME INDIPENDENTE to COPERTURA CON NODI (exercise 18.11), the
 *   complement of a set is a cover, k' = n - k;
 * - CIRCUITO HAMILTONIANO to COMMESSO VIAGGIATORE (Teorema 18.6): the
 *   distance 1 on the edges and 2 elsewhere, k = n;
 * - PARTIZIONE to SCHEDULING MULTIPROCESSORE, SOMMA DI SOTTOINSIEME and
 *   ZAINO (Teoremi 18.7-18.9), half of the sum as the bound;
 * - CIRCUITO HAMILTONIANO to CICLO HAMILTONIANO (exercise 18.12), both the
 *   orientations of each edge; CICLO HAMILTONIANO to CAMMINO HAMILTONIANO
 *   (exercise 18.13), the node 0 split into a start with its edges out and
 *   an end with its edges in; CICLO HAMILTONIANO to CATENA HAMILTONIANA
 *   (exercise 18.14), that path with each node made a chain of three and a
 *   pendant node at each end;
 * - CAMMINO HAMILTONIANO from r to t to CAMMINO MASSIMO, k = n - 1, and
 *   CATENA HAMILTONIANA to ALBERO DI COPERTURA LIMITATO, k = 2 (exercises
 *   18.15 and 18.16);
 * - ORDINAMENTO to INVILUPPO CONVESSO: the points ( a_i , a_i^2 ) on the
 *   parabola are all vertices of the hull, in the order of the a_i, so
 *   the hull costs \f$ \Omega( n \log n ) \f$.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 18.5 and 18.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Reductions
 #define __ADS_Reductions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Domino.h"
#include "ADS/complexity/Problems.h"
#include "ADS/geometry/ConvexHull.h"

#include <array>
#include <cstddef>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the place of the variable or node of the tile h in the square ( i , j )
inline int tileVar( int h , int i , int j , int n )
{
 return( ( h * n + i ) * n + j );
 }

/// DOMINO LIMITATO to CRICCA: the graph on the m n^2 places of tileVar(),
/// the ones of ( 0 , 0 ) but u_000 isolated, and k
inline std::pair< Adjacency , int > dominoToClique(
                                   const std::vector< Tile > & D , int n )
{
 const int m = int( D.size() ) , N = m * n * n;
 Adjacency G( N , std::vector< bool >( N , false ) );
 auto alive = [ & ]( int h , int i , int j ) {
  return( ( i != 0 ) || ( j != 0 ) || ( h == 0 ) ); };
 for( int h = 0 ; h < m ; ++h )
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int g = 0 ; g < m ; ++g )
     for( int a = 0 ; a < n ; ++a )
      for( int b = 0 ; b < n ; ++b ) {
       if( ! alive( h , i , j ) || ! alive( g , a , b ) ) continue;
       if( ( a == i ) && ( b == j ) ) continue;
       const int di = a - i , dj = b - j;
       bool edge;
       if( std::abs( di ) + std::abs( dj ) != 1 )
        edge = true;                                         // (3)
       else if( di == 1 ) edge = D[ h ].right == D[ g ].left;   // (4)
       else if( di == -1 ) edge = D[ g ].right == D[ h ].left;
       else if( dj == 1 ) edge = D[ h ].up == D[ g ].down;
       else edge = D[ g ].up == D[ h ].down;
       G[ tileVar( h , i , j , n ) ][ tileVar( g , a , b , n ) ] = edge;
       }
 return( std::pair( G , n * n ) );
 }

/// DOMINO LIMITATO to SODDISFATTIBILITA', on the m n^2 variables of
/// tileVar()
inline Cnf dominoToSat( const std::vector< Tile > & D , int n )
{
 const int m = int( D.size() );
 auto u = [ & ]( int h , int i , int j ) {
  return( tileVar( h , i , j , n ) + 1 ); };
 Cnf F = { { u( 0 , 0 , 0 ) } };                                 // (1)
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j ) {
   Clause c;                                                     // (2)
   for( int h = 0 ; h < m ; ++h ) c.push_back( u( h , i , j ) );
   F.push_back( c );
   }
 for( int i = 0 ; i < n ; ++i )                                  // (3)
  for( int j = 0 ; j < n ; ++j )
   for( int h = 0 ; h < m ; ++h )
    for( int g = h + 1 ; g < m ; ++g )
     F.push_back( { -u( h , i , j ) , -u( g , i , j ) } );
 for( int i = 0 ; i + 1 < n ; ++i )                              // (4)
  for( int j = 0 ; j < n ; ++j )
   for( int h = 0 ; h < m ; ++h )
    for( int g = 0 ; g < m ; ++g )
     if( D[ h ].right != D[ g ].left )
      F.push_back( { -u( h , i , j ) , -u( g , i + 1 , j ) } );
 for( int i = 0 ; i < n ; ++i )                                  // (5)
  for( int j = 0 ; j + 1 < n ; ++j )
   for( int h = 0 ; h < m ; ++h )
    for( int g = 0 ; g < m ; ++g )
     if( D[ h ].up != D[ g ].down )
      F.push_back( { -u( h , i , j ) , -u( g , i , j + 1 ) } );
 return( F );
 }

/// DOMINO LIMITATO to PROGRAMMAZIONE LINEARE 0/1: A x <= b on the m n^2
/// variables of tileVar(), each equation as two inequalities
inline std::pair< std::vector< std::vector< long > > , std::vector< long > >
dominoToLp01( const std::vector< Tile > & D , int n )
{
 const int m = int( D.size() ) , N = m * n * n;
 std::vector< std::vector< long > > A;
 std::vector< long > b;
 auto row = [ & ]( const std::vector< int > & vars , long rhs , bool eq ) {
  std::vector< long > r( N , 0 );
  for( int v : vars ) r[ v ] += 1;
  A.push_back( r );
  b.push_back( rhs );
  if( eq ) {                                      // and - r <= - rhs
   for( auto & x : r ) x = -x;
   A.push_back( r );
   b.push_back( -rhs );
   }
  };
 row( { tileVar( 0 , 0 , 0 , n ) } , 1 , true );                  // (1)
 for( int i = 0 ; i < n ; ++i )                                  // (2)
  for( int j = 0 ; j < n ; ++j ) {
   std::vector< int > v;
   for( int h = 0 ; h < m ; ++h ) v.push_back( tileVar( h , i , j , n ) );
   row( v , 1 , true );
   }
 for( int h = 0 ; h < m ; ++h )                                  // (3), (4)
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    std::vector< int > right = { tileVar( h , i , j , n ) } ,
                       up = { tileVar( h , i , j , n ) };
    for( int g = 0 ; g < m ; ++g ) {
     if( i + 1 < n && D[ h ].right != D[ g ].left )
      right.push_back( tileVar( g , i + 1 , j , n ) );
     if( j + 1 < n && D[ h ].up != D[ g ].down )
      up.push_back( tileVar( g , i , j + 1 , n ) );
     }
    if( right.size() > 1 ) row( right , 1 , false );
    if( up.size() > 1 ) row( up , 1 , false );
    }
 return( std::pair( A , b ) );
 }

/// the complement graph: CRICCA of size k to INSIEME INDIPENDENTE of size k
inline Adjacency complement( const Adjacency & G )
{
 Adjacency C = G;
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = 0 ; v < G.size() ; ++v )
   C[ u ][ v ] = ( u != v ) && ! G[ u ][ v ];
 return( C );
 }

/// CIRCUITO HAMILTONIANO to COMMESSO VIAGGIATORE: the distances, k = n
inline std::vector< std::vector< long > > hamiltonToTsp( const Adjacency & G )
{
 const std::size_t n = G.size();
 std::vector< std::vector< long > > Dist( n , std::vector< long >( n , 0 ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j )
   if( i != j ) Dist[ i ][ j ] = G[ i ][ j ] ? 1 : 2;
 return( Dist );
 }

/// half the sum, or -1 if the sum is odd (then no partition: the bound
/// -1 has no YES in any of the three problems)
inline long halfSum( const std::vector< long > & A )
{
 const long s = std::accumulate( A.begin() , A.end() , 0L );
 return( s % 2 ? -1 : s / 2 );
 }

/// CICLO HAMILTONIANO to CAMMINO HAMILTONIANO from n to 0: the node 0 split,
/// a new node n with the edges into 0
inline Adjacency cycleToPath( const Adjacency & G )
{
 const std::size_t n = G.size();
 Adjacency H( n + 1 , std::vector< bool >( n + 1 , false ) );
 for( std::size_t u = 0 ; u < n ; ++u )
  for( std::size_t v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] ) {
    if( u == 0 ) H[ n ][ v ] = true;              // out of 0: out of n
    else H[ u ][ v ] = true;                      // into 0 stays into 0
    }
 return( H );
 }

/// CICLO HAMILTONIANO to CATENA HAMILTONIANA: the path of cycleToPath(), each
/// node v a chain v_in - v_mid - v_out, each edge ( u , v ) an edge
/// u_out - v_in, and pendant nodes at the start and at the end
inline Adjacency cycleToChain( const Adjacency & G )
{
 const Adjacency P = cycleToPath( G );
 const int N = int( P.size() ) , M = 3 * N + 2;
 Adjacency H( M , std::vector< bool >( M , false ) );
 auto link = [ & ]( int a , int b ) { H[ a ][ b ] = H[ b ][ a ] = true; };
 for( int v = 0 ; v < N ; ++v ) {
  link( 3 * v , 3 * v + 1 );
  link( 3 * v + 1 , 3 * v + 2 );
  }
 for( int u = 0 ; u < N ; ++u )
  for( int v = 0 ; v < N ; ++v )
   if( P[ u ][ v ] ) link( 3 * u + 2 , 3 * v );
 link( 3 * N , 3 * ( N - 1 ) );        // before the start, the node N - 1
 link( 3 * N + 1 , 2 );                // after the end, the node 0
 return( H );
 }

/// ORDINAMENTO to INVILUPPO CONVESSO: the points ( a , a^2 )
inline std::vector< Point > sortToHull( const std::vector< long > & a )
{
 std::vector< Point > P;
 for( long x : a ) P.push_back( { x , x * x } );
 return( P );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Reductions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Reductions.h --------------------------*/
/*--------------------------------------------------------------------------*/
