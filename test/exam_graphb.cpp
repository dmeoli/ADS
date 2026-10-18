/*--------------------------------------------------------------------------*/
/*-------------------------- File exam_graphb.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems 25 to 49 of the collection Esercizi Grafi: each
 * problem of code or of computation is solved as its official solution
 * says, by a function of the namespace graph or by the function of ADS that
 * already solves it, and checked on the examples of the statement, against
 * an independent oracle on small random instances (a brute force, an
 * enumeration of the paths, a transitive closure, Floyd-Warshall, a
 * merge-find set, a simulation) and on the edge cases. The tests are named
 * after the problems, graph-N, or graph-N-K for the part K of the problem
 * N; where the official solution has an error, the function is right and
 * its comment says how it differs from the text.
 *
 * \see A. Montresor, Esercizi Grafi, Soluzioni, Algoritmicamente, 2026
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapters 9 and 11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/complexity/EasyCases.h"
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*--------------------------------- COMMON ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace graph {

using Ints = std::vector< int >;
using Matrix = std::vector< std::vector< int > >;
using BoolMatrix = std::vector< std::vector< bool > >;

/// the four moves of a cell, as differences of the row and of the column
inline constexpr int DR[ 4 ] = { -1 , 0 , 1 , 0 };
inline constexpr int DC[ 4 ] = { 0 , -1 , 0 , 1 };

/// whether ( r , c ) is a cell of an n x n matrix
inline bool inside( int n , int r , int c )
{
 return( ( 0 <= r ) && ( r < n ) && ( 0 <= c ) && ( c < n ) );
 }

/// whether M is square
template< class T >
bool square( const std::vector< std::vector< T > > & M )
{
 for( const auto & row : M )
  if( row.size() != M.size() )
   return( false );
 return( true );
 }

/// whether a visit of g from v reaches every node: the ccdfs() of the book
/// followed by the check that no node is left unvisited
/// \pre v is a node
template< Graph G >
bool allReached( const G & g , int v )
{
 ADS_PRE( g.hasNode( v ) );
 std::vector< bool > visited( g.bound() , false );
 auto none = []( auto... ) {};
 dfs( g , v , visited , none , none , none );
 for( int u : g.V() )
  if( ! visited[ u ] )
   return( false );
 return( true );
 }

}  // end( namespace graph )

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using graph::BoolMatrix;
using graph::Ints;
using graph::Matrix;
using Edges = std::vector< std::pair< int , int > >;

/// an integer in [ lo , hi ]
int uniform( std::mt19937 & r , int lo , int hi )
{
 return( std::uniform_int_distribution< int >( lo , hi )( r ) );
 }

/// a real in [ 0 , 1 )
double real( std::mt19937 & r )
{
 return( std::uniform_real_distribution< double >( 0 , 1 )( r ) );
 }

/// the graph of n nodes and of the edges E
template< class G = ListGraph<> >
G make( int n , const Edges & E , bool directed )
{
 G g( n , directed );
 for( auto [ u , v ] : E )
  g.insertEdge( u , v );
 return( g );
 }

/// a random graph of n nodes, each edge with probability p
ListGraph<> randomGraph( std::mt19937 & r , int n , double p , bool directed )
{
 ListGraph<> g( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && ( real( r ) < p ) )
    g.insertEdge( u , v );
 return( g );
 }

/// a random acyclic graph, its edges from a smaller to a larger place of a
/// random permutation
ListGraph<> randomDag( std::mt19937 & r , int n , double p )
{
 Ints pi( n );
 for( int i = 0 ; i < n ; ++i )
  pi[ i ] = i;
 std::shuffle( pi.begin() , pi.end() , r );
 ListGraph<> g( n , true );
 for( int i = 0 ; i < n ; ++i )
  for( int j = i + 1 ; j < n ; ++j )
   if( real( r ) < p )
    g.insertEdge( pi[ i ] , pi[ j ] );
 return( g );
 }

/// a random unoriented tree of n nodes, each node joined to an earlier one
/// with fewer than k adjacent nodes
/// \pre k >= 2
ListGraph<> randomTree( std::mt19937 & r , int n , int k )
{
 ListGraph<> g( n , false );
 for( int v = 1 ; v < n ; ++v )
  for( ; ; ) {
   const int u = uniform( r , 0 , v - 1 );
   if( int( g.adj( u ).size() ) < k ) {
    g.insertEdge( u , v );
    break;
    }
   }
 return( g );
 }

/// a random connected unoriented graph: a random tree and more edges, each
/// with probability p
ListGraph<> randomConnected( std::mt19937 & r , int n , double p )
{
 auto g = randomTree( r , n , std::max( n , 2 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( ! g.hasEdge( u , v ) && ( real( r ) < p ) )
    g.insertEdge( u , v );
 return( g );
 }

/// R[ u ][ v ]: a path from u to v of at least one edge, or of any length
/// if reflexive
template< Graph G >
BoolMatrix closure( const G & g , bool reflexive )
{
 const int n = g.bound();
 BoolMatrix R( n , std::vector< bool >( n , false ) );
 for( int u : g.V() ) {
  R[ u ][ u ] = reflexive;
  g.forEachAdj( u , [ & ]( int v ) { R[ u ][ v ] = true; } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   if( R[ i ][ k ] )
    for( int j = 0 ; j < n ; ++j )
     if( R[ k ][ j ] )
      R[ i ][ j ] = true;
 return( R );
 }

/// the distances with unit weights, by Floyd-Warshall
template< Graph G >
Matrix unitDistances( const G & g )
{
 const int n = g.bound();
 Matrix d( n , Ints( n , INF ) );
 for( int u : g.V() ) {
  d[ u ][ u ] = 0;
  g.forEachAdj( u , [ & ]( int v ) { d[ u ][ v ] = 1; } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ( d[ i ][ k ] < INF ) && ( d[ k ][ j ] < INF ) )
     d[ i ][ j ] = std::min( d[ i ][ j ] , d[ i ][ k ] + d[ k ][ j ] );
 return( d );
 }

/// whether the nodes of g are all in one component (g unoriented)
template< Graph G >
bool connected( const G & g )
{
 const auto id = cc( g );
 for( int u : g.V() )
  if( id[ u ] != 1 )
   return( false );
 return( true );
 }

/// the unoriented edges of g, as ( smaller , larger ) pairs
template< Graph G >
std::set< std::pair< int , int > > edgeSet( const G & g )
{
 std::set< std::pair< int , int > > E;
 for( int u : g.V() )
  for( int v : g.adj( u ) )
   E.insert( { std::min( u , v ) , std::max( u , v ) } );
 return( E );
 }

/// a random n x n matrix of values in [ lo , hi ]
Matrix randomMatrix( std::mt19937 & r , int n , int lo , int hi )
{
 Matrix M( n , Ints( n ) );
 for( auto & row : M )
  for( int & x : row )
   x = uniform( r , lo , hi );
 return( M );
 }

/// a random n x n matrix, each item true with probability p
BoolMatrix randomBools( std::mt19937 & r , int n , double p )
{
 BoolMatrix M( n , std::vector< bool >( n ) );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   M[ i ][ j ] = real( r ) < p;
 return( M );
 }

/// the components of the cells of an n x n matrix, two adjacent cells
/// joined when joined( r , c , r2 , c2 ): the representative of the cell
/// ( r , c ) at the place r n + c, by a merge-find set
template< class F >
Ints cellComponents( int n , F && joined )
{
 MFSet S( n * n );
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c ) {
   if( ( c + 1 < n ) && joined( r , c , r , c + 1 ) )
    S.merge( r * n + c , r * n + c + 1 );
   if( ( r + 1 < n ) && joined( r , c , r + 1 , c ) )
    S.merge( r * n + c , ( r + 1 ) * n + c );
   }
 Ints rep( n * n );
 for( int i = 0 ; i < n * n ; ++i )
  rep[ i ] = S.find( i );
 return( rep );
 }

/// the independent check of a Nurikabe solution: the components by a
/// merge-find set, then each rule on its own
bool nurikabeOracle( const Matrix & G , const Matrix & S )
{
 const int n = int( G.size() );
 auto same = [ & ]( int color ) {
  return( [ & S , color ]( int a , int b , int c , int d ) {
   return( ( S[ a ][ b ] == color ) && ( S[ c ][ d ] == color ) ); } );
  };
 const auto white = cellComponents( n , same( 0 ) );
 const auto black = cellComponents( n , same( 1 ) );
 std::set< int > walls;
 Ints size( n * n , 0 ) , numbers( n * n , 0 ) , value( n * n , 0 );
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c ) {
   const int i = r * n + c;
   if( S[ r ][ c ] == 1 ) {
    if( G[ r ][ c ] > 0 )
     return( false );                       // a number on the wall
    walls.insert( black[ i ] );
    }
   else {
    ++size[ white[ i ] ];
    if( G[ r ][ c ] > 0 ) {
     ++numbers[ white[ i ] ];
     value[ white[ i ] ] = G[ r ][ c ];
     }
    }
   if( ( r + 1 < n ) && ( c + 1 < n ) && S[ r ][ c ] && S[ r ][ c + 1 ] &&
       S[ r + 1 ][ c ] && S[ r + 1 ][ c + 1 ] )
    return( false );                        // a black square
   }
 if( walls.size() != 1 )
  return( false );
 for( int i = 0 ; i < n * n ; ++i )
  if( ( size[ i ] > 0 ) &&
      ( ( numbers[ i ] != 1 ) || ( value[ i ] != size[ i ] ) ) )
   return( false );
 return( true );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- PROBLEMS --------------------------------*/
/*--------------------------------------------------------------------------*/

/*------------------------ 25. 2018-07-25, Shangai -------------------------*/

namespace graph {

/// shangai(): whether all the n sticks can be taken away, the stick X[ i ]
/// lying on Y[ i ] (sticks numbered from 0, not from 1 as in the text);
/// they can if and only if the graph of the edges X[ i ] -> Y[ i ] is
/// acyclic, which hasCycle() decides by a depth first visit, O( n + m )
/// \pre X and Y have the same size, values in [ 0 , n ), X[ i ] != Y[ i ]
inline bool shangai( int n , const Ints & X , const Ints & Y )
{
 ADS_PRE( ( n >= 0 ) && ( X.size() == Y.size() ) );
 ListGraph<> g( n , true );
 for( std::size_t i = 0 ; i < X.size() ; ++i )
  g.insertEdge( X[ i ] , Y[ i ] );
 return( ! hasCycle( g ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-25" )
{
 // the sticks taken away one by one, any free one first
 std::mt19937 r( 28 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 ) , m = uniform( r , 0 , 12 );
  Ints X , Y;
  for( int i = 0 ; i < m ; ++i ) {
   const int x = uniform( r , 0 , n - 1 ) , y = uniform( r , 0 , n - 1 );
   if( x != y ) {
    X.push_back( x );
    Y.push_back( y );
    }
   }
  std::vector< bool > in( n , true );
  bool progress = true;
  int left = n;
  while( progress ) {
   progress = false;
   for( int v = 0 ; v < n ; ++v )
    if( in[ v ] ) {
     bool covered = false;
     for( std::size_t i = 0 ; i < X.size() ; ++i )
      covered = covered || ( ( Y[ i ] == v ) && in[ X[ i ] ] );
     if( ! covered ) {
      in[ v ] = false;
      --left;
      progress = true;
      }
     }
   }
  CHECK( graph::shangai( n , X , Y ) == ( left == 0 ) );
  }
 CHECK( graph::shangai( 1 , {} , {} ) );
 CHECK( graph::shangai( 3 , { 0 , 1 } , { 1 , 2 } ) );
 CHECK( ! graph::shangai( 2 , { 0 , 1 } , { 1 , 0 } ) );
 // no stick; a pair given twice; a pile of 10^5 sticks one on the other,
 // then the last on the first
 CHECK( graph::shangai( 0 , {} , {} ) );
 CHECK( graph::shangai( 2 , { 0 , 0 } , { 1 , 1 } ) );
 Ints X , Y;
 for( int i = 1 ; i < 100000 ; ++i ) {
  X.push_back( i );
  Y.push_back( i - 1 );
  }
 CHECK( graph::shangai( 100000 , X , Y ) );
 X.push_back( 0 );
 Y.push_back( 99999 );
 CHECK( ! graph::shangai( 100000 , X , Y ) );
 // the contracts: n >= 0, X and Y of the same size, values in [ 0 , n ),
 // X[ i ] != Y[ i ]
 CHECK_THROWS( graph::shangai( -1 , {} , {} ) , contract_violation );
 CHECK_THROWS( graph::shangai( 2 , { 0 } , {} ) , contract_violation );
 CHECK_THROWS( graph::shangai( 2 , { 0 } , { 2 } ) , contract_violation );
 CHECK_THROWS( graph::shangai( 2 , { 1 } , { 1 } ) , contract_violation );
 }

/*----------------------- 26. 2018-08-21, Minecraft ------------------------*/

namespace graph {

/// topIsland(): the largest average height of an island, a 4-connected set
/// of cells > 0, 0 if there is no land; from each land cell not visited
/// yet a depth first visit sums the heights of its island and counts its
/// cells, setting them to 0 on a copy of A, O( n^2 ). The text tests
/// 1 <= r < n and 1 <= c < n on indices from 1, which misses the last row
/// and the last column: here the whole matrix
/// \pre A is square
inline double topIsland( Matrix A )
{
 ADS_PRE( square( A ) );
 const int n = int( A.size() );
 long long height = 0;
 int count = 0;
 auto rec = [ & ]( auto & self , int r , int c ) -> void {
  if( inside( n , r , c ) && ( A[ r ][ c ] > 0 ) ) {
   height += A[ r ][ c ];
   ++count;
   A[ r ][ c ] = 0;                          // visited
   for( int k = 0 ; k < 4 ; ++k )
    self( self , r + DR[ k ] , c + DC[ k ] );
   }
  };
 double best = 0;
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c )
   if( A[ r ][ c ] > 0 ) {
    height = 0;
    count = 0;
    rec( rec , r , c );
    best = std::max( best , double( height ) / count );
    }
 return( best );
 }

}  // end( namespace graph )

ADS_TEST( "graph-26" )
{
 // the example: islands of average 1.5, 1 and 2, the last in the last row
 const Matrix A = { { 1 , 1 , 1 , 0 , 1 } , { 2 , 2 , 2 , -1 , 1 } ,
                    { 0 , -1 , 0 , -2 , 1 } , { 0 , 0 , -1 , -2 , 1 } ,
                    { 2 , 2 , -1 , 0 , 0 } };
 CHECK( graph::topIsland( A ) == 2 );
 // the islands by a merge-find set
 std::mt19937 r( 29 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  const auto M = randomMatrix( r , n , -2 , 3 );
  const auto rep = cellComponents( n , [ & ]( int a , int b , int c , int d ) {
   return( ( M[ a ][ b ] > 0 ) && ( M[ c ][ d ] > 0 ) ); } );
  std::vector< long long > sum( n * n , 0 );
  Ints count( n * n , 0 );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( M[ i ][ j ] > 0 ) {
     sum[ rep[ i * n + j ] ] += M[ i ][ j ];
     ++count[ rep[ i * n + j ] ];
     }
  double ref = 0;
  for( int k = 0 ; k < n * n ; ++k )
   if( count[ k ] > 0 )
    ref = std::max( ref , double( sum[ k ] ) / count[ k ] );
  CHECK( std::abs( graph::topIsland( M ) - ref ) < 1e-12 );
  }
 CHECK( graph::topIsland( Matrix( 3 , Ints( 3 , -1 ) ) ) == 0 );
 CHECK( graph::topIsland( Matrix( 6 , Ints( 6 , 5 ) ) ) == 5 );
 CHECK( graph::topIsland( { { 7 } } ) == 7 );
 // no cell; islands of the largest int, whose sums need 64 bits; islands
 // touching only at a corner are two; one island of 100 x 100 cells
 CHECK( graph::topIsland( {} ) == 0 );
 CHECK( graph::topIsland( Matrix( 3 , Ints( 3 , INT_MAX ) ) ) == INT_MAX );
 CHECK( graph::topIsland( { { 1 , 0 } , { 0 , 3 } } ) == 3 );
 Matrix big( 100 , Ints( 100 , 1 ) );
 big[ 99 ][ 99 ] = 10001;
 CHECK( graph::topIsland( big ) == 2 );
 // the contracts: A square
 CHECK_THROWS( graph::topIsland( { { 1 , 1 } } ) , contract_violation );
 }

/*----------------------- 27. 2019-06-17, Coast len ------------------------*/

namespace graph {

/// coastLen(): the number of cells of the coast of the sea of ( r , c ),
/// the cells >= 0 next to a cell of that sea, the 4-connected cells < 0; a
/// depth first visit from ( r , c ) that goes on from the sea cells and
/// counts a land cell when it first reaches it, O( n^2 )
/// \pre M is square, ( r , c ) is a cell with M[ r ][ c ] < 0
inline int coastLen( const Matrix & M , int r , int c )
{
 const int n = int( M.size() );
 ADS_PRE( square( M ) && inside( n , r , c ) && ( M[ r ][ c ] < 0 ) );
 BoolMatrix visited( n , std::vector< bool >( n , false ) );
 auto rec = [ & ]( auto & self , int x , int y ) -> int {
  if( ! inside( n , x , y ) || visited[ x ][ y ] )
   return( 0 );
  visited[ x ][ y ] = true;
  if( M[ x ][ y ] >= 0 )
   return( 1 );
  int t = 0;
  for( int k = 0 ; k < 4 ; ++k )
   t += self( self , x + DR[ k ] , y + DC[ k ] );
  return( t );
  };
 return( rec( rec , r , c ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-27" )
{
 // the example, the cell ( 2 , 4 ) of the text counting from 1
 const Matrix M = { { 1 , 2 , 3 , 4 , 5 , 6 , 1 } ,
                    { 3 , -2 , -3 , -3 , -1 , 3 , 3 } ,
                    { 4 , -2 , 1 , -2 , -3 , 4 , 5 } ,
                    { 2 , -1 , 2 , -1 , -1 , 7 , 7 } ,
                    { 1 , 2 , 2 , -1 , 1 , -3 , -2 } ,
                    { 4 , 4 , 4 , -1 , 2 , -2 , -3 } ,
                    { 4 , 4 , 4 , 4 , 3 , 2 , 2 } };
 CHECK( graph::coastLen( M , 1 , 3 ) == 18 );
 // the sea flooded, then its land neighbours counted
 std::mt19937 r( 30 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  auto A = randomMatrix( r , n , -3 , 3 );
  const int sr = uniform( r , 0 , n - 1 ) , sc = uniform( r , 0 , n - 1 );
  A[ sr ][ sc ] = -1;
  BoolMatrix sea( n , std::vector< bool >( n , false ) );
  std::deque< std::pair< int , int > > Q = { { sr , sc } };
  sea[ sr ][ sc ] = true;
  while( ! Q.empty() ) {
   const auto [ x , y ] = Q.front();
   Q.pop_front();
   for( int k = 0 ; k < 4 ; ++k ) {
    const int a = x + graph::DR[ k ] , b = y + graph::DC[ k ];
    if( graph::inside( n , a , b ) && ! sea[ a ][ b ] &&
        ( A[ a ][ b ] < 0 ) ) {
     sea[ a ][ b ] = true;
     Q.push_back( { a , b } );
     }
    }
   }
  int ref = 0;
  for( int x = 0 ; x < n ; ++x )
   for( int y = 0 ; y < n ; ++y )
    if( A[ x ][ y ] >= 0 ) {
     bool coast = false;
     for( int k = 0 ; k < 4 ; ++k ) {
      const int a = x + graph::DR[ k ] , b = y + graph::DC[ k ];
      coast = coast || ( graph::inside( n , a , b ) && sea[ a ][ b ] );
      }
     ref += coast ? 1 : 0;
     }
  CHECK( graph::coastLen( A , sr , sc ) == ref );
  }
 CHECK( graph::coastLen( { { -1 } } , 0 , 0 ) == 0 );
 CHECK( graph::coastLen( { { 0 , 0 , 0 } , { 0 , -5 , 0 } , { 0 , 0 , 0 } } ,
                         1 , 1 ) == 4 );
 CHECK( graph::coastLen( { { -1 , 0 } , { 0 , 0 } } , 0 , 0 ) == 2 );
 // all sea; a land cell next to the sea twice counts once; another sea
 // beyond the land does not count
 CHECK( graph::coastLen( Matrix( 4 , Ints( 4 , -1 ) ) , 3 , 0 ) == 0 );
 CHECK( graph::coastLen( { { -1 , -1 } , { -1 , 5 } } , 0 , 0 ) == 1 );
 CHECK( graph::coastLen( { { -1 , 0 , -1 } , { -1 , 0 , -1 } ,
                           { -1 , 0 , -1 } } , 0 , 0 ) == 3 );
 // a sea of 100 x 100 cells with one rock in the middle
 Matrix big( 100 , Ints( 100 , -1 ) );
 big[ 50 ][ 50 ] = 0;
 CHECK( graph::coastLen( big , 0 , 0 ) == 1 );
 // the contracts: M square, ( r , c ) a cell of the sea
 CHECK_THROWS( graph::coastLen( { { -1 , 0 } } , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( graph::coastLen( { { -1 } } , 1 , 0 ) , contract_violation );
 CHECK_THROWS( graph::coastLen( { { -1 } } , 0 , -1 ) , contract_violation );
 CHECK_THROWS( graph::coastLen( { { 0 } } , 0 , 0 ) , contract_violation );
 }

/*------------------------ 28. 2019-07-18, Nurikabe ------------------------*/

namespace graph {

/// isSolution(): whether S (0 white, 1 black) solves the Nurikabe G (0 no
/// number, > 0 a number); scanning the cells, a numbered cell already
/// visited (a room with two numbers) or black, and four black cells in a
/// square, fail; a white numbered cell not visited starts a visit of its
/// room, whose size must be the number, and a black one not visited a visit
/// of a wall; at the end there must be one wall and no cell left out (a
/// room with no number), O( n^2 )
/// \pre G and S are square of the same size, S of 0 and 1
inline bool nurikabe( const Matrix & G , const Matrix & S )
{
 const int n = int( G.size() );
 ADS_PRE( square( G ) && square( S ) && ( int( S.size() ) == n ) );
 ADS_PRE( std::all_of( S.begin() , S.end() , []( const Ints & row ) {
  return( std::all_of( row.begin() , row.end() , []( int x ) {
   return( ( x == 0 ) || ( x == 1 ) ); } ) ); } ) );
 BoolMatrix visited( n , std::vector< bool >( n , false ) );
 auto rec = [ & ]( auto & self , int r , int c , int color ) -> int {
  if( ! inside( n , r , c ) || visited[ r ][ c ] || ( S[ r ][ c ] != color ) )
   return( 0 );
  visited[ r ][ c ] = true;
  int k = 1;
  for( int i = 0 ; i < 4 ; ++i )
   k += self( self , r + DR[ i ] , c + DC[ i ] , color );
  return( k );
  };
 int walls = 0;
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c ) {
   if( ( G[ r ][ c ] > 0 ) && ( visited[ r ][ c ] || ( S[ r ][ c ] == 1 ) ) )
    return( false );
   if( ( r + 1 < n ) && ( c + 1 < n ) &&
       ( S[ r ][ c ] + S[ r ][ c + 1 ] + S[ r + 1 ][ c ] +
         S[ r + 1 ][ c + 1 ] == 4 ) )
    return( false );
   if( ! visited[ r ][ c ] && ( S[ r ][ c ] == 0 ) && ( G[ r ][ c ] > 0 ) &&
       ( rec( rec , r , c , 0 ) != G[ r ][ c ] ) )
    return( false );
   if( ! visited[ r ][ c ] && ( S[ r ][ c ] == 1 ) ) {
    ++walls;
    rec( rec , r , c , 1 );
    }
   }
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c )
   if( ! visited[ r ][ c ] )
    return( false );
 return( walls == 1 );
 }

}  // end( namespace graph )

ADS_TEST( "graph-28" )
{
 // a puzzle and its solution, then each rule broken
 const Matrix G = { { 2 , 0 , 0 , 2 } , { 0 , 0 , 0 , 0 } ,
                    { 0 , 0 , 1 , 0 } , { 0 , 2 , 0 , 0 } };
 const Matrix S = { { 0 , 1 , 0 , 0 } , { 0 , 1 , 1 , 1 } ,
                    { 1 , 1 , 0 , 1 } , { 0 , 0 , 1 , 1 } };
 CHECK( graph::nurikabe( G , S ) && nurikabeOracle( G , S ) );
 auto G2 = G;
 G2[ 0 ][ 0 ] = 3;                                     // a wrong size
 CHECK( ! graph::nurikabe( G2 , S ) );
 G2 = G;
 G2[ 1 ][ 0 ] = 2;                                     // two numbers
 CHECK( ! graph::nurikabe( G2 , S ) );
 G2 = G;
 G2[ 0 ][ 1 ] = 1;                                     // a number on the wall
 CHECK( ! graph::nurikabe( G2 , S ) );
 G2 = G;
 G2[ 2 ][ 2 ] = 0;                                     // a room with no number
 CHECK( ! graph::nurikabe( G2 , S ) );
 auto S2 = S;
 S2[ 1 ][ 2 ] = 0;                                     // two walls
 CHECK( ! graph::nurikabe( G , S2 ) );
 S2 = S;
 S2[ 2 ][ 2 ] = 1;                                     // a black square
 CHECK( ! graph::nurikabe( G , S2 ) );
 // random colorings with a number placed in each room, sometimes changed
 std::mt19937 r( 31 );
 int solved = 0;
 for( int t = 0 ; t < 2000 ; ++t ) {
  const int n = uniform( r , 1 , 4 );
  const auto B = randomMatrix( r , n , 0 , 1 );
  const auto rep = cellComponents( n , [ & ]( int a , int b , int c , int d ) {
   return( ( B[ a ][ b ] == 0 ) && ( B[ c ][ d ] == 0 ) ); } );
  Ints size( n * n , 0 );
  for( int i = 0 ; i < n * n ; ++i )
   if( B[ i / n ][ i % n ] == 0 )
    ++size[ rep[ i ] ];
  Matrix P( n , Ints( n , 0 ) );
  for( int i = 0 ; i < n * n ; ++i )
   if( ( B[ i / n ][ i % n ] == 0 ) && ( rep[ i ] == i ) )
    P[ i / n ][ i % n ] = size[ i ];
  if( real( r ) < 0.3 )
   P[ uniform( r , 0 , n - 1 ) ][ uniform( r , 0 , n - 1 ) ] =
                                                        uniform( r , 0 , 3 );
  const bool ref = nurikabeOracle( P , B );
  CHECK( graph::nurikabe( P , B ) == ref );
  solved += ref ? 1 : 0;
  }
 CHECK( solved > 100 );
 // all white, and a single black cell
 CHECK( ! graph::nurikabe( { { 4 , 0 } , { 0 , 0 } } ,
                           { { 0 , 0 } , { 0 , 0 } } ) );
 CHECK( graph::nurikabe( { { 0 } } , { { 1 } } ) );
 // no cell: no wall, no solution; one white cell numbered 1, with no
 // wall; a full black square of 2 x 2
 CHECK( ! graph::nurikabe( {} , {} ) && ! nurikabeOracle( {} , {} ) );
 CHECK( ! graph::nurikabe( { { 1 } } , { { 0 } } ) );
 CHECK( ! graph::nurikabe( { { 0 , 0 } , { 0 , 0 } } ,
                           { { 1 , 1 } , { 1 , 1 } } ) );
 // an L of wall and a room of 1
 CHECK( graph::nurikabe( { { 1 , 0 } , { 0 , 0 } } ,
                         { { 0 , 1 } , { 1 , 1 } } ) );
 // the contracts: G and S square of the same size, S of 0 and 1
 CHECK_THROWS( graph::nurikabe( G , { { 0 } } ) , contract_violation );
 CHECK_THROWS( graph::nurikabe( { { 0 , 0 } } , { { 0 , 0 } } ) ,
               contract_violation );
 CHECK_THROWS( graph::nurikabe( { { 0 } } , { { 2 } } ) ,
               contract_violation );
 }

/*---------------------- 29. 2019-08-22, Min sessions ----------------------*/

namespace graph {

/// minSessions(): the least number of sessions for the exams of the
/// acyclic graph of the prerequisites, the number of nodes of a longest
/// path: the layers of longestPathLayering() of the library, dist[ v ] =
/// max( dist[ v ] , dist[ u ] + 1 ) on each edge ( u , v ) in topological
/// order, and the answer is the largest layer plus 1, O( n + m )
/// \pre g is acyclic with a node
template< Graph G >
int minSessions( const G & g )
{
 ADS_PRE( g.n() > 0 );
 const Ints L = longestPathLayering( g );
 return( *std::max_element( L.begin() , L.end() ) + 1 );
 }

}  // end( namespace graph )

ADS_TEST( "graph-29" )
{
 // the figure: A and B before C, C before D
 CHECK( graph::minSessions( make( 4 , { { 0 , 2 } , { 1 , 2 } , { 2 , 3 } } ,
                                  true ) ) == 3 );
 // the rounds of removal of the nodes with no entering edge
 std::mt19937 r( 32 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 9 );
  const auto g = randomDag( r , n , real( r ) );
  Ints in( n , 0 );
  for( int u = 0 ; u < n ; ++u )
   for( int v : g.adj( u ) )
    ++in[ v ];
  std::vector< bool > gone( n , false );
  int rounds = 0 , left = n;
  while( left > 0 ) {
   Ints now;
   for( int u = 0 ; u < n ; ++u )
    if( ! gone[ u ] && ( in[ u ] == 0 ) )
     now.push_back( u );
   for( int u : now ) {
    gone[ u ] = true;
    --left;
    for( int v : g.adj( u ) )
     --in[ v ];
    }
   ++rounds;
   }
  CHECK( graph::minSessions( g ) == rounds );
  }
 CHECK( graph::minSessions( ListGraph<>( 1 ) ) == 1 );
 CHECK( graph::minSessions( ListGraph<>( 9 ) ) == 1 );
 ListGraph<> c( 30 , true );
 for( int u = 0 ; u + 1 < 30 ; ++u )
  c.insertEdge( u + 1 , u );
 CHECK( graph::minSessions( c ) == 30 );
 // a chain of 10^5 exams; a deleted node
 ListGraph<> p( 100000 , true );
 for( int u = 0 ; u + 1 < 100000 ; ++u )
  p.insertEdge( u , u + 1 );
 CHECK( graph::minSessions( p ) == 100000 );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 2 } } , true );
 h.deleteNode( 1 );
 CHECK( graph::minSessions( h ) == 1 );
 // the contracts: g with a node, acyclic
 CHECK_THROWS( graph::minSessions( ListGraph<>( 0 ) ) , contract_violation );
 CHECK_THROWS( graph::minSessions( make( 2 , { { 0 , 1 } , { 1 , 0 } } ,
                                         true ) ) , contract_violation );
 }

/*------------------------ 30. 2020-02-04, Has path ------------------------*/

namespace graph {

/// hasPath(): whether there is a path with no repeated cell from ( 0 , 0 )
/// to ( n - 1 , n - 1 ), each move from ( i , j ) going A[ i ][ j ] cells
/// up, down, left or right; a depth first visit of the implicit oriented
/// graph of the moves, Theta( n^2 )
/// \pre A is square, not empty, of positive values
inline bool hasPath( const Matrix & A )
{
 const int n = int( A.size() );
 ADS_PRE( ( n > 0 ) && square( A ) );
 BoolMatrix visited( n , std::vector< bool >( n , false ) );
 auto rec = [ & ]( auto & self , int i , int j ) -> bool {
  if( ( i == n - 1 ) && ( j == n - 1 ) )
   return( true );
  if( ! inside( n , i , j ) || visited[ i ][ j ] )
   return( false );
  visited[ i ][ j ] = true;
  const int a = A[ i ][ j ];
  ADS_PRE( a > 0 );
  if( a >= n )                     // off the board, and i + a may overflow
   return( false );
  return( self( self , i + a , j ) || self( self , i , j + a ) ||
          self( self , i - a , j ) || self( self , i , j - a ) );
  };
 return( rec( rec , 0 , 0 ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-30" )
{
 // the example of the text
 CHECK( graph::hasPath( { { 1 , 2 , 4 , 3 } , { 1 , 2 , 3 , 2 } ,
                          { 1 , 2 , 3 , 2 } , { 1 , 4 , 4 , 5 } } ) );
 // a breadth first visit of the explicit graph of the jumps
 std::mt19937 r( 33 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  const auto A = randomMatrix( r , n , 1 , n );
  ListGraph<> h( n * n , true );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    for( int k = 0 ; k < 4 ; ++k ) {
     const int a = i + graph::DR[ k ] * A[ i ][ j ] ,
               b = j + graph::DC[ k ] * A[ i ][ j ];
     if( graph::inside( n , a , b ) )
      h.insertEdge( i * n + j , a * n + b );
     }
  CHECK( graph::hasPath( A ) == ( distance( h , 0 ).first[ n * n - 1 ] !=
                                  INF ) );
  }
 CHECK( graph::hasPath( { { 5 } } ) );
 CHECK( ! graph::hasPath( Matrix( 3 , Ints( 3 , 3 ) ) ) );
 // jumps longer than the board, up to the largest int, whose sum with a
 // place overflowed; the last cell whatever its value; a board of 70 x 70
 // of ones
 CHECK( ! graph::hasPath( { { INT_MAX , 1 } , { 1 , 1 } } ) );
 CHECK( ! graph::hasPath( { { 1 , INT_MAX } , { INT_MAX , 1 } } ) );
 CHECK( graph::hasPath( { { 1 , 1 } , { 1 , 99 } } ) );
 Matrix big( 70 , Ints( 70 , 1 ) );
 CHECK( graph::hasPath( big ) );
 // the contracts: A square, not empty, of positive values on the way
 CHECK_THROWS( graph::hasPath( {} ) , contract_violation );
 CHECK_THROWS( graph::hasPath( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( graph::hasPath( { { 0 , 1 } , { 1 , 1 } } ) ,
               contract_violation );
 CHECK_THROWS( graph::hasPath( { { 1 , 1 } , { -1 , 1 } } ) ,
               contract_violation );
 }

/*--------------------- 31. 2020-09-04, Meeting point ----------------------*/

namespace graph {

/// meetingPoint(): a node z with the least d( u , z ) + d( v , z ) +
/// d( w , z ) in edges, by three breadth first visits, O( n + m ); on a
/// tie, the last such node in the order of g.V()
/// \pre g is unoriented and connected, u, v and w are nodes
template< Graph G >
int meetingPoint( const G & g , int u , int v , int w )
{
 ADS_PRE( g.hasNode( u ) && g.hasNode( v ) && g.hasNode( w ) );
 const auto du = distance( g , u ).first , dv = distance( g , v ).first ,
            dw = distance( g , w ).first;
 long long best = -1;
 int z = NIL;
 for( int i : g.V() ) {
  ADS_PRE( ( du[ i ] != INF ) && ( dv[ i ] != INF ) && ( dw[ i ] != INF ) );
  const long long s = 0LL + du[ i ] + dv[ i ] + dw[ i ];
  if( ( best < 0 ) || ( s <= best ) ) {
   best = s;
   z = i;
   }
  }
 return( z );
 }

}  // end( namespace graph )

ADS_TEST( "graph-31" )
{
 // the figure: the 3 x 3 grid, with z in the centre joined to the corners
 Edges E;
 for( int i = 0 ; i < 3 ; ++i )
  for( int j = 0 ; j < 2 ; ++j ) {
   E.push_back( { 3 * i + j , 3 * i + j + 1 } );
   E.push_back( { 3 * j + i , 3 * j + i + 3 } );
   }
 for( int c : { 0 , 2 , 6 , 8 } )
  E.push_back( { 4 , c } );
 const auto f = make( 9 , E , false );
 const auto F = unitDistances( f );
 CHECK( graph::meetingPoint( f , 0 , 2 , 8 ) == 4 );
 CHECK( F[ 0 ][ 4 ] + F[ 2 ][ 4 ] + F[ 8 ][ 4 ] == 3 );
 CHECK( F[ 0 ][ 2 ] + F[ 2 ][ 2 ] + F[ 8 ][ 2 ] == 4 );
 CHECK( graph::meetingPoint( f , 0 , 4 , 8 ) == 4 );
 // Floyd-Warshall: the node returned has the least sum
 std::mt19937 r( 34 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  const auto g = randomConnected( r , n , 0.3 * real( r ) );
  const auto D = unitDistances( g );
  const int u = uniform( r , 0 , n - 1 ) , v = uniform( r , 0 , n - 1 ) ,
            w = uniform( r , 0 , n - 1 );
  int best = INF;
  for( int z = 0 ; z < n ; ++z )
   best = std::min( best , D[ u ][ z ] + D[ v ][ z ] + D[ w ][ z ] );
  const int z = graph::meetingPoint( g , u , v , w );
  CHECK( D[ u ][ z ] + D[ v ][ z ] + D[ w ][ z ] == best );
  if( ( u == v ) && ( v == w ) )
   CHECK( z == u );
  }
 // one node; a triangle, all tied, the last node; two of the three at
 // the same node
 CHECK( graph::meetingPoint( ListGraph<>( 1 , false ) , 0 , 0 , 0 ) == 0 );
 const auto k3 = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 0 , 2 } } , false );
 CHECK( graph::meetingPoint( k3 , 0 , 1 , 2 ) == 2 );
 CHECK( graph::meetingPoint( f , 0 , 0 , 8 ) == 0 );
 // the contracts: u, v and w nodes, every node reached (connected)
 CHECK_THROWS( graph::meetingPoint( k3 , 0 , 1 , 3 ) , contract_violation );
 CHECK_THROWS( graph::meetingPoint( ListGraph<>( 2 , false ) , 0 , 0 , 0 ) ,
               contract_violation );
 }

/*------------------------ 32. 2022-02-07, Gazprom -------------------------*/

namespace graph {

/// gazprom(): the largest total capacity of a connected component of the
/// unoriented graph of the deposits, from the components of cc(),
/// Theta( n + m )
/// \pre g has a node, C has a place per name, positive
template< Graph G >
long long gazprom( const G & g , const Ints & C )
{
 ADS_PRE( ( g.n() > 0 ) && ( int( C.size() ) == g.bound() ) );
 const auto id = cc( g );
 std::vector< long long > capacity( g.bound() , 0 );
 for( int u : g.V() ) {
  ADS_PRE( C[ u ] > 0 );
  capacity[ id[ u ] - 1 ] += C[ u ];
  }
 return( *std::max_element( capacity.begin() , capacity.end() ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-32" )
{
 // the components by a merge-find set
 std::mt19937 r( 35 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 9 );
  const auto g = randomGraph( r , n , 0.4 * real( r ) , false );
  Ints C( n );
  for( int & c : C )
   c = uniform( r , 1 , 100 );
  MFSet S( n );
  for( auto [ u , v ] : edgeSet( g ) )
   S.merge( u , v );
  std::vector< long long > sum( n , 0 );
  for( int u = 0 ; u < n ; ++u )
   sum[ S.find( u ) ] += C[ u ];
  CHECK( graph::gazprom( g , C ) ==
         *std::max_element( sum.begin() , sum.end() ) );
  }
 CHECK( graph::gazprom( ListGraph<>( 1 , false ) , { 7 } ) == 7 );
 CHECK( graph::gazprom( ListGraph<>( 3 , false ) , { 4 , 9 , 2 } ) == 9 );
 const auto p = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 CHECK( graph::gazprom( p , { 1000000000 , 1000000000 , 1000000000 } ) ==
        3000000000LL );
 // the largest int at every deposit of a component of 10^4, summed in 64
 // bits; a deleted node does not count
 ListGraph<> big( 10000 , false );
 for( int u = 1 ; u < 10000 ; ++u )
  big.insertEdge( u - 1 , u );
 CHECK( graph::gazprom( big , Ints( 10000 , INT_MAX ) ) ==
        10000LL * INT_MAX );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 h.deleteNode( 1 );
 CHECK( graph::gazprom( h , { 3 , 100 , 4 } ) == 4 );
 // the contracts: g with a node, C of a place per name, positive
 CHECK_THROWS( graph::gazprom( ListGraph<>( 0 , false ) , {} ) ,
               contract_violation );
 CHECK_THROWS( graph::gazprom( p , { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( graph::gazprom( p , { 1 , 0 , 1 } ) , contract_violation );
 }

/*----------------------- 33. 2022-06-21, Mele marce -----------------------*/

namespace graph {

/// badApples(): the number of days after which no apple rots, M[ i ][ j ]
/// being 1 for a good apple, -1 for a rotten one and 0 for an empty place;
/// a breadth first visit from all the rotten apples at once, at distance
/// 0, through the good ones, and the answer is the largest distance,
/// Theta( n^2 )
/// \pre M is square, not empty
inline int badApples( const Matrix & M )
{
 const int n = int( M.size() );
 ADS_PRE( ( n > 0 ) && square( M ) );
 Matrix dist( n , Ints( n , -1 ) );
 std::deque< std::pair< int , int > > Q;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( M[ i ][ j ] == -1 ) {
    dist[ i ][ j ] = 0;
    Q.push_back( { i , j } );
    }
 int days = 0;
 while( ! Q.empty() ) {
  const auto [ i , j ] = Q.front();
  Q.pop_front();
  days = std::max( days , dist[ i ][ j ] );
  for( int k = 0 ; k < 4 ; ++k ) {
   const int x = i + DR[ k ] , y = j + DC[ k ];
   if( inside( n , x , y ) && ( M[ x ][ y ] == 1 ) &&
       ( dist[ x ][ y ] < 0 ) ) {
    dist[ x ][ y ] = dist[ i ][ j ] + 1;
    Q.push_back( { x , y } );
    }
   }
  }
 return( days );
 }

}  // end( namespace graph )

ADS_TEST( "graph-33" )
{
 // the examples: one rotten apple in a corner, on a full crate and on a
 // ring; the sweep in place of the text gives 1 day on the full crate
 Matrix full( 5 , Ints( 5 , 1 ) ) , ring = full;
 full[ 0 ][ 0 ] = ring[ 0 ][ 0 ] = -1;
 for( int i = 1 ; i < 4 ; ++i )
  for( int j = 1 ; j < 4 ; ++j )
   ring[ i ][ j ] = 0;
 CHECK( graph::badApples( full ) == 8 );
 CHECK( graph::badApples( ring ) == 8 );
 auto sweep = []( Matrix M ) {
  const int n = int( M.size() );
  int count = 0;
  for( bool infected = true ; infected ; ++count ) {
   infected = false;
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j )
     if( M[ i ][ j ] == -1 )
      for( int k = 0 ; k < 4 ; ++k ) {
       const int a = i + graph::DR[ k ] , b = j + graph::DC[ k ];
       if( graph::inside( n , a , b ) && ( M[ a ][ b ] == 1 ) ) {
        M[ a ][ b ] = -1;
        infected = true;
        }
       }
   }
  return( count - 1 );
  };
 CHECK( sweep( full ) == 1 );
 // the days simulated on a copy of the crate
 std::mt19937 r( 36 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  const auto M0 = randomMatrix( r , n , -1 , 1 );
  auto M = M0;
  int days = 0;
  for( ; ; ) {
   auto N = M;
   bool changed = false;
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j )
     if( M[ i ][ j ] == -1 )
      for( int k = 0 ; k < 4 ; ++k ) {
       const int a = i + graph::DR[ k ] , b = j + graph::DC[ k ];
       if( graph::inside( n , a , b ) && ( N[ a ][ b ] == 1 ) ) {
        N[ a ][ b ] = -1;
        changed = true;
        }
       }
   if( ! changed )
    break;
   M = N;
   ++days;
   }
  CHECK( graph::badApples( M0 ) == days );
  }
 // one place, no rotten apple, no good apple, a good apple out of reach
 CHECK( graph::badApples( { { 1 } } ) == 0 );
 CHECK( graph::badApples( Matrix( 4 , Ints( 4 , 1 ) ) ) == 0 );
 CHECK( graph::badApples( Matrix( 4 , Ints( 4 , -1 ) ) ) == 0 );
 CHECK( graph::badApples( { { -1 , 1 , 0 , 1 } , { 0 , 0 , 0 , 0 } ,
                            { 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 } } ) == 1 );
 // all empty; two rotten apples racing from the two ends of a row; a
 // crate of 100 x 100 from a corner
 CHECK( graph::badApples( Matrix( 3 , Ints( 3 , 0 ) ) ) == 0 );
 CHECK( graph::badApples( { { -1 , 1 , 1 , 1 , -1 } , { 0 , 0 , 0 , 0 , 0 } ,
                            { 0 , 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 , 0 } ,
                            { 0 , 0 , 0 , 0 , 0 } } ) == 2 );
 Matrix big( 100 , Ints( 100 , 1 ) );
 big[ 0 ][ 0 ] = -1;
 CHECK( graph::badApples( big ) == 198 );
 // the contracts: M square, not empty
 CHECK_THROWS( graph::badApples( {} ) , contract_violation );
 CHECK_THROWS( graph::badApples( { { 1 , -1 } } ) , contract_violation );
 }

/*--------------------- 34. 2022-07-13, Lago ricorsivo ---------------------*/

namespace graph {

/// containsLake(): whether the island of the map M (1 land, 0 water, the
/// border all water) holds a lake; a depth first visit floods the sea from
/// ( 0 , 0 ), marking its cells -1 on a copy of M, and any water cell left
/// is a lake, O( n^2 )
/// \pre M is square, not empty, with water on the border
inline bool containsLake( Matrix M )
{
 const int n = int( M.size() );
 ADS_PRE( ( n > 0 ) && square( M ) );
 for( int i = 0 ; i < n ; ++i )
  ADS_PRE( ( M[ 0 ][ i ] == 0 ) && ( M[ n - 1 ][ i ] == 0 ) &&
           ( M[ i ][ 0 ] == 0 ) && ( M[ i ][ n - 1 ] == 0 ) );
 auto sea = [ & ]( auto & self , int i , int j ) -> void {
  if( inside( n , i , j ) && ( M[ i ][ j ] == 0 ) ) {
   M[ i ][ j ] = -1;
   for( int k = 0 ; k < 4 ; ++k )
    self( self , i + DR[ k ] , j + DC[ k ] );
   }
  };
 sea( sea , 0 , 0 );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( M[ i ][ j ] == 0 )
    return( true );
 return( false );
 }

}  // end( namespace graph )

ADS_TEST( "graph-34" )
{
 // four maps, true , true , false , false
 const Matrix m1 = { { 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 1 , 1 , 1 , 0 , 0 } ,
                     { 0 , 1 , 0 , 1 , 1 , 0 } , { 0 , 1 , 1 , 0 , 1 , 0 } ,
                     { 0 , 0 , 1 , 1 , 1 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 } };
 const Matrix m2 = { { 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 1 , 0 , 1 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 0 , 0 , 0 , 1 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 } };
 const Matrix m3 = { { 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 1 , 1 , 1 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 0 , 0 , 0 , 1 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 } };
 const Matrix m4 = { { 0 , 0 , 0 , 0 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 1 , 0 , 0 , 0 , 0 } , { 0 , 1 , 1 , 1 , 1 , 0 } ,
                     { 0 , 0 , 0 , 0 , 1 , 0 } , { 0 , 0 , 0 , 0 , 0 , 0 } };
 CHECK( graph::containsLake( m1 ) && graph::containsLake( m2 ) );
 CHECK( ! graph::containsLake( m3 ) && ! graph::containsLake( m4 ) );
 // more than one component of water, by a merge-find set
 std::mt19937 r( 37 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 7 );
  Matrix M( n , Ints( n , 0 ) );
  for( int i = 1 ; i + 1 < n ; ++i )
   for( int j = 1 ; j + 1 < n ; ++j )
    M[ i ][ j ] = real( r ) < 0.6 ? 1 : 0;
  const auto rep = cellComponents( n , [ & ]( int a , int b , int c , int d ) {
   return( ( M[ a ][ b ] == 0 ) && ( M[ c ][ d ] == 0 ) ); } );
  std::set< int > water;
  for( int i = 0 ; i < n * n ; ++i )
   if( M[ i / n ][ i % n ] == 0 )
    water.insert( rep[ i ] );
  CHECK( graph::containsLake( M ) == ( water.size() > 1 ) );
  }
 CHECK( ! graph::containsLake( { { 0 } } ) );
 CHECK( ! graph::containsLake( Matrix( 3 , Ints( 3 , 0 ) ) ) );
 Matrix ring( 5 , Ints( 5 , 0 ) );
 for( int i = 1 ; i < 4 ; ++i )
  for( int j = 1 ; j < 4 ; ++j )
   ring[ i ][ j ] = ( ( i == 2 ) && ( j == 2 ) ) ? 0 : 1;
 CHECK( graph::containsLake( ring ) );
 // four lakes; a bay open to the sea is no lake; a lake touching the sea
 // at a corner only is a lake; a sea of 100 x 100 with no island
 Matrix two( 7 , Ints( 7 , 0 ) );
 for( int i = 1 ; i < 6 ; ++i )
  for( int j = 1 ; j < 6 ; ++j )
   two[ i ][ j ] = ( ( i == 2 ) || ( i == 4 ) ) && ( j % 2 == 0 ) ? 0 : 1;
 CHECK( graph::containsLake( two ) );
 Matrix corner( 4 , Ints( 4 , 0 ) );
 corner[ 1 ][ 2 ] = corner[ 2 ][ 1 ] = 1;
 corner[ 2 ][ 2 ] = 1;
 CHECK( ! graph::containsLake( corner ) );
 Matrix diag( 5 , Ints( 5 , 0 ) );
 diag[ 1 ][ 2 ] = diag[ 2 ][ 1 ] = diag[ 2 ][ 3 ] = diag[ 3 ][ 2 ] = 1;
 CHECK( graph::containsLake( diag ) );
 CHECK( ! graph::containsLake( Matrix( 100 , Ints( 100 , 0 ) ) ) );
 // the contracts: M square, not empty, water on the border
 CHECK_THROWS( graph::containsLake( {} ) , contract_violation );
 CHECK_THROWS( graph::containsLake( { { 0 , 0 } } ) , contract_violation );
 CHECK_THROWS( graph::containsLake( { { 1 } } ) , contract_violation );
 Matrix shore( 3 , Ints( 3 , 0 ) );
 shore[ 2 ][ 1 ] = 1;
 CHECK_THROWS( graph::containsLake( shore ) , contract_violation );
 }

/*--------------------- 35. 2022-08-24, DAG massimale ----------------------*/

namespace graph {

/// maxDag(): the largest number of edges that can be added to the acyclic
/// graph without a cycle, n ( n - 1 ) / 2 - m since a topological order
/// allows only its n ( n - 1 ) / 2 forward pairs; n and m are counted on the
/// lists, O( n + m )
/// \pre g is acyclic (audit)
template< Graph G >
long long maxDag( const G & g )
{
 ADS_AUDIT( ! hasCycle( g ) );
 long long n = 0 , m = 0;
 for( int u : g.V() ) {
  ++n;
  m += static_cast< long long >( g.adj( u ).size() );
  }
 return( n * ( n - 1 ) / 2 - m );
 }

}  // end( namespace graph )

ADS_TEST( "graph-35" )
{
 // the forward pairs of a topological order that are not edges
 std::mt19937 r( 38 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 9 );
  const auto g = randomDag( r , n , real( r ) );
  const auto S = topsort( g );
  long long ref = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 1 ; j < n ; ++j )
    ref += g.hasEdge( S[ i ] , S[ j ] ) ? 0 : 1;
  CHECK( graph::maxDag( g ) == ref );
  CHECK( graph::maxDag( g ) == 1LL * n * ( n - 1 ) / 2 - g.m() );
  }
 CHECK( graph::maxDag( ListGraph<>( 0 ) ) == 0 );
 CHECK( graph::maxDag( ListGraph<>( 1 ) ) == 0 );
 CHECK( graph::maxDag( ListGraph<>( 1000 ) ) == 499500 );
 CHECK( graph::maxDag( randomDag( r , 8 , 1 ) ) == 0 );
 // a deleted node is not counted; 10^5 nodes, whose pairs need 64 bits
 auto h = make( 3 , { { 0 , 1 } , { 1 , 2 } } , true );
 h.deleteNode( 1 );
 CHECK( graph::maxDag( h ) == 1 );
 CHECK( graph::maxDag( ListGraph<>( 100000 ) ) == 4999950000LL );
 // the contracts: g acyclic (audit)
 CHECK_THROWS( graph::maxDag( make( 2 , { { 0 , 1 } , { 1 , 0 } } , true ) ) ,
               contract_violation );
 }

/*---------------- 36. 2023-01-17, Albero di altezza minima ----------------*/

namespace graph {

/// minHeight(): the least height of a binary tree rooted at a node of the
/// unoriented tree g, of degree at most 3; only a node of degree below 3
/// can be the root, and its height is its largest distance, by a breadth
/// first visit from each such node, O( n^2 )
/// \pre g is an unoriented tree with a node, of degree at most 3
template< Graph G >
int minHeight( const G & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) );
 int best = INF;
 for( int u : g.V() ) {
  ADS_PRE( g.adj( u ).size() <= 3 );
  if( g.adj( u ).size() < 3 ) {
   const auto d = distance( g , u ).first;
   int h = 0;
   for( int v : g.V() )
    h = std::max( h , d[ v ] );
   best = std::min( best , h );
   }
  }
 return( best );
 }

}  // end( namespace graph )

ADS_TEST( "graph-36" )
{
 // the figures: A - B - C , A - D , B - E , C - F - G gives 3; a star
 // with centre B of degree 3 gives 2, not the 1 of B as a root
 const auto f = make( 7 , { { 0 , 1 } , { 1 , 2 } , { 0 , 3 } , { 1 , 4 } ,
                            { 2 , 5 } , { 5 , 6 } } , false );
 CHECK( graph::minHeight( f ) == 3 );
 const auto star = make( 4 , { { 1 , 0 } , { 1 , 2 } , { 1 , 3 } } , false );
 CHECK( graph::minHeight( star ) == 2 );
 // the heights of the rooted trees by recursion, and the eccentricities by
 // Floyd-Warshall
 std::mt19937 r( 39 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 12 );
  const auto g = randomTree( r , n , 3 );
  const auto D = unitDistances( g );
  auto height = [ & ]( auto & self , int u , int parent ) -> int {
   int h = 0;
   for( int v : g.adj( u ) )
    if( v != parent )
     h = std::max( h , 1 + self( self , v , u ) );
   return( h );
   };
  int ref = INF , ref2 = INF;
  for( int u = 0 ; u < n ; ++u )
   if( g.adj( u ).size() < 3 ) {
    ref = std::min( ref , height( height , u , NIL ) );
    ref2 = std::min( ref2 , *std::max_element( D[ u ].begin() ,
                                               D[ u ].end() ) );
    }
  CHECK( ( graph::minHeight( g ) == ref ) && ( ref == ref2 ) );
  }
 CHECK( graph::minHeight( ListGraph<>( 1 , false ) ) == 0 );
 CHECK( graph::minHeight( make( 2 , { { 0 , 1 } } , false ) ) == 1 );
 ListGraph<> p( 7 , false );
 for( int u = 0 ; u + 1 < 7 ; ++u )
  p.insertEdge( u , u + 1 );
 CHECK( graph::minHeight( p ) == 3 );
 // a path of 8 nodes, an even one: the root in the middle, 4; a complete
 // tree of degree 3 whose leaves alone may be the root
 ListGraph<> q( 8 , false );
 for( int u = 0 ; u + 1 < 8 ; ++u )
  q.insertEdge( u , u + 1 );
 CHECK( graph::minHeight( q ) == 4 );
 const auto t3 = make( 10 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } , { 1 , 4 } ,
                              { 1 , 5 } , { 2 , 6 } , { 2 , 7 } , { 3 , 8 } ,
                              { 3 , 9 } } , false );
 CHECK( graph::minHeight( t3 ) == 4 );
 // the contracts: g unoriented with a node, the degrees at most 3
 CHECK_THROWS( graph::minHeight( ListGraph<>( 0 , false ) ) ,
               contract_violation );
 CHECK_THROWS( graph::minHeight( make( 2 , { { 0 , 1 } } , true ) ) ,
               contract_violation );
 CHECK_THROWS( graph::minHeight( make( 5 , { { 0 , 1 } , { 0 , 2 } ,
                                             { 0 , 3 } , { 0 , 4 } } ,
                                       false ) ) , contract_violation );
 }

/*--------------------- 38. 2023-08-23, Diretto o no? ----------------------*/

namespace graph {

/// isUndirected(): whether ( y , x ) is an edge for each edge ( x , y ); a
/// set with hashing of the adjacent nodes of each node makes each test
/// O( 1 ) expected, O( n + m ) expected time
template< Graph G >
bool isUndirected( const G & g )
{
 std::vector< std::unordered_set< int > > N( g.bound() );
 for( int x : g.V() )
  g.forEachAdj( x , [ & ]( int y ) { N[ x ].insert( y ); } );
 for( int x : g.V() )
  for( int y : g.adj( x ) )
   if( ! N[ y ].count( x ) )
    return( false );
 return( true );
 }

}  // end( namespace graph )

ADS_TEST( "graph-38" )
{
 // the figures, A , B , C as 0 , 1 , 2: every edge in both directions on
 // the left, ( C , A ) missing on the right
 const auto left = make( 3 , { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } ,
                               { 2 , 1 } , { 0 , 2 } , { 2 , 0 } } , true );
 const auto right = make( 3 , { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } ,
                                { 2 , 1 } , { 0 , 2 } } , true );
 CHECK( graph::isUndirected( left ) && ! graph::isUndirected( right ) );
 // the set of the pairs
 std::mt19937 r( 40 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 0 , 8 );
  auto g = randomGraph( r , n , 0.4 * real( r ) , true );
  if( real( r ) < 0.5 )
   for( int u = 0 ; u < n ; ++u )
    for( int v : g.adj( u ) )
     if( real( r ) < 0.9 )
      g.insertEdge( v , u );
  std::set< std::pair< int , int > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v : g.adj( u ) )
    E.insert( { u , v } );
  bool ref = true;
  for( auto [ u , v ] : E )
   ref = ref && E.count( { v , u } );
  CHECK( graph::isUndirected( g ) == ref );
  }
 CHECK( graph::isUndirected( ListGraph<>( 0 ) ) );
 CHECK( graph::isUndirected( randomGraph( r , 8 , 0.5 , false ) ) );
 // nodes with no edge; one edge one way; a node deleted with the edge
 // that had no twin
 CHECK( graph::isUndirected( ListGraph<>( 3 ) ) );
 CHECK( ! graph::isUndirected( make( 2 , { { 1 , 0 } } , true ) ) );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 0 } , { 2 , 0 } } , true );
 CHECK( ! graph::isUndirected( h ) );
 h.deleteNode( 2 );
 CHECK( graph::isUndirected( h ) );
 }

/*-------------------- 39. 2024-01-09, Tree Repair Shop --------------------*/

namespace graph {

/// repairTree(): makes a tree of the unoriented graph made of a tree and
/// one more edge, deleting an edge of its cycle; a depth first visit from
/// 0 that never goes back to the node it comes from deletes the first edge
/// that leads to a visited node and stops, O( n )
/// \pre g is unoriented and connected, with nodes 0 , ... , n - 1 and n
///      edges
inline void repairTreeDelete( ListGraph<> & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) && ( g.bound() == g.n() ) &&
          ( g.m() == g.n() ) );
 std::vector< bool > visited( g.bound() , false );
 auto rec = [ & ]( auto & self , int curr , int prev ) -> bool {
  visited[ curr ] = true;
  for( int next : g.adj( curr ) )
   if( next != prev ) {
    if( visited[ next ] ) {
     g.deleteEdge( curr , next );
     return( true );
     }
    if( self( self , next , curr ) )
     return( true );
    }
  return( false );
  };
 rec( rec , 0 , NIL );
 }

}  // end( namespace graph )

ADS_TEST( "graph-39" )
{
 // a tree and one more edge: a tree again, on edges of the graph
 std::mt19937 r( 41 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 3 , 10 );
  auto g = randomTree( r , n , n );
  for( ; ; ) {
   const int u = uniform( r , 0 , n - 1 ) , v = uniform( r , 0 , n - 1 );
   if( ( u != v ) && ! g.hasEdge( u , v ) ) {
    g.insertEdge( u , v );
    break;
    }
   }
  const auto E = edgeSet( g );
  graph::repairTreeDelete( g );
  CHECK( ( g.m() == n - 1 ) && connected( g ) );
  for( const auto & e : edgeSet( g ) )
   CHECK( E.count( e ) );
  }
 auto c = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , false );
 graph::repairTreeDelete( c );
 CHECK( ( c.m() == 2 ) && connected( c ) );
 // the cycle away from 0, at the end of a path; the whole graph a cycle
 // of 5000 nodes, the recursion as deep
 auto t = make( 5 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } , { 3 , 4 } ,
                      { 4 , 2 } } , false );
 graph::repairTreeDelete( t );
 CHECK( ( t.m() == 4 ) && connected( t ) && t.hasEdge( 0 , 1 ) &&
        t.hasEdge( 1 , 2 ) );
 ListGraph<> big( 5000 , false );
 for( int u = 0 ; u < 5000 ; ++u )
  big.insertEdge( u , ( u + 1 ) % 5000 );
 graph::repairTreeDelete( big );
 CHECK( ( big.m() == 4999 ) && connected( big ) );
 // the contracts: g unoriented, nodes 0 , ... , n - 1, n edges
 auto o = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , true );
 CHECK_THROWS( graph::repairTreeDelete( o ) , contract_violation );
 auto tree = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 CHECK_THROWS( graph::repairTreeDelete( tree ) , contract_violation );
 ListGraph<> empty( 0 , false );
 CHECK_THROWS( graph::repairTreeDelete( empty ) , contract_violation );
 auto hole = make( 4 , { { 1 , 2 } , { 2 , 3 } , { 3 , 1 } } , false );
 hole.deleteNode( 0 );
 CHECK_THROWS( graph::repairTreeDelete( hole ) , contract_violation );
 }

/*------------------- 40. 2024-02-01, Tree Repair Shop 2 -------------------*/

namespace graph {

/// repairTree(): makes a tree of the unoriented forest, adding edges; the
/// component of 0 is visited, then each node not visited yet is joined to
/// 0 and its component visited, so that each new edge joins two components,
/// Theta( n + m )
/// \pre g is an unoriented forest with nodes 0 , ... , n - 1, n > 0
inline void repairTreeInsert( ListGraph<> & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) && ( g.bound() == g.n() ) );
 std::vector< bool > visited( g.bound() , false );
 auto none = []( auto... ) {};
 dfs( g , 0 , visited , none , none , none );
 for( int u : g.V() )
  if( ! visited[ u ] ) {
   g.insertEdge( 0 , u );
   dfs( g , u , visited , none , none , none );
   }
 }

}  // end( namespace graph )

ADS_TEST( "graph-40" )
{
 // a tree with edges removed: a tree again, the edges left kept
 std::mt19937 r( 42 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 10 );
  auto g = randomTree( r , n , std::max( n , 2 ) );
  for( auto [ u , v ] : edgeSet( g ) )
   if( real( r ) < 0.5 )
    g.deleteEdge( u , v );
  const auto E = edgeSet( g );
  graph::repairTreeInsert( g );
  CHECK( ( g.m() == n - 1 ) && connected( g ) );
  for( auto [ u , v ] : E )
   CHECK( g.hasEdge( u , v ) );
  }
 ListGraph<> one( 1 , false ) , none( 5 , false );
 graph::repairTreeInsert( one );
 graph::repairTreeInsert( none );
 CHECK( ( one.m() == 0 ) && ( none.m() == 4 ) && connected( none ) );
 for( int u = 1 ; u < 5 ; ++u )
  CHECK( none.hasEdge( 0 , u ) );
 // a tree already: nothing added; 10^5 isolated nodes
 auto tree = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 graph::repairTreeInsert( tree );
 CHECK( tree.m() == 2 );
 ListGraph<> many( 100000 , false );
 graph::repairTreeInsert( many );
 CHECK( ( many.m() == 99999 ) && connected( many ) );
 // the contracts: g unoriented, nodes 0 , ... , n - 1, n > 0
 ListGraph<> o( 2 , true ) , empty( 0 , false );
 CHECK_THROWS( graph::repairTreeInsert( o ) , contract_violation );
 CHECK_THROWS( graph::repairTreeInsert( empty ) , contract_violation );
 ListGraph<> hole( 3 , false );
 hole.deleteNode( 1 );
 CHECK_THROWS( graph::repairTreeInsert( hole ) , contract_violation );
 }

/*------------------------ 41. 2024-06-14, Big Fire ------------------------*/

namespace graph {

/// bigfire(): the number of cells that burn when the fire starts at
/// ( i , j ), a burning forest cell (1) igniting its four neighbours and
/// any other burning cell only the forest ones; a depth first visit that
/// goes from ( x , y ) to a neighbour if ( x , y ) is forest or the
/// neighbour is, O( n^2 )
/// \pre A is square, ( i , j ) is a cell
inline int bigFire( const Matrix & A , int i , int j )
{
 const int n = int( A.size() );
 ADS_PRE( square( A ) && inside( n , i , j ) );
 BoolMatrix visited( n , std::vector< bool >( n , false ) );
 auto rec = [ & ]( auto & self , int x , int y ) -> int {
  if( visited[ x ][ y ] )
   return( 0 );
  visited[ x ][ y ] = true;
  int count = 1;
  for( int k = 0 ; k < 4 ; ++k ) {
   const int a = x + DR[ k ] , b = y + DC[ k ];
   if( inside( n , a , b ) && ! visited[ a ][ b ] &&
       ( ( A[ x ][ y ] == 1 ) || ( A[ a ][ b ] == 1 ) ) )
    count += self( self , a , b );
   }
  return( count );
  };
 return( rec( rec , i , j ) );
 }

}  // end( namespace graph )

ADS_TEST( "graph-41" )
{
 // the examples of the text
 CHECK( graph::bigFire( { { 1 , 1 , 1 , 1 } , { 0 , 0 , 0 , 0 } ,
                          { 0 , 0 , 0 , 0 } , { 1 , 1 , 1 , 1 } } ,
                        0 , 0 ) == 8 );
 CHECK( graph::bigFire( { { 1 , 1 , 1 , 1 } , { 0 , 0 , 0 , 1 } ,
                          { 0 , 0 , 0 , 0 } , { 0 , 0 , 1 , 1 } } ,
                        0 , 0 ) == 13 );
 // the fire spread round after round
 std::mt19937 r( 43 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  const auto A = randomMatrix( r , n , 0 , 1 );
  const int i = uniform( r , 0 , n - 1 ) , j = uniform( r , 0 , n - 1 );
  BoolMatrix B( n , std::vector< bool >( n , false ) );
  B[ i ][ j ] = true;
  for( bool more = true ; more ; ) {
   more = false;
   for( int x = 0 ; x < n ; ++x )
    for( int y = 0 ; y < n ; ++y )
     if( B[ x ][ y ] )
      for( int k = 0 ; k < 4 ; ++k ) {
       const int a = x + graph::DR[ k ] , b = y + graph::DC[ k ];
       if( graph::inside( n , a , b ) && ! B[ a ][ b ] &&
           ( ( A[ x ][ y ] == 1 ) || ( A[ a ][ b ] == 1 ) ) ) {
        B[ a ][ b ] = true;
        more = true;
        }
       }
   }
  int ref = 0;
  for( const auto & row : B )
   ref += int( std::count( row.begin() , row.end() , true ) );
  CHECK( graph::bigFire( A , i , j ) == ref );
  }
 CHECK( graph::bigFire( { { 0 } } , 0 , 0 ) == 1 );
 CHECK( graph::bigFire( Matrix( 5 , Ints( 5 , 0 ) ) , 2 , 2 ) == 1 );
 CHECK( graph::bigFire( Matrix( 5 , Ints( 5 , 1 ) ) , 2 , 2 ) == 25 );
 // a fire on bare ground lights its four neighbours only, and those of a
 // corner are two
 CHECK( graph::bigFire( Matrix( 5 , Ints( 5 , 0 ) ) , 0 , 0 ) == 1 );
 Matrix A( 5 , Ints( 5 , 0 ) );
 A[ 2 ][ 3 ] = 1;
 CHECK( graph::bigFire( A , 2 , 2 ) == 5 );
 CHECK( graph::bigFire( A , 0 , 4 ) == 1 );
 // two forests joined by a bare cell between them burn together
 const Matrix B = { { 1 , 0 , 1 } , { 0 , 0 , 0 } , { 0 , 0 , 0 } };
 CHECK( graph::bigFire( B , 0 , 0 ) == 5 );
 // the contracts: A square, ( i , j ) a cell
 CHECK_THROWS( graph::bigFire( { { 1 , 1 } } , 0 , 0 ) , contract_violation );
 CHECK_THROWS( graph::bigFire( B , 3 , 0 ) , contract_violation );
 CHECK_THROWS( graph::bigFire( B , 0 , -1 ) , contract_violation );
 CHECK_THROWS( graph::bigFire( {} , 0 , 0 ) , contract_violation );
 }

/*------------------ 42. 2024-07-02, Circuiti matriciali -------------------*/

namespace graph {

/// hasCircuit(): whether the free cells (false) of M hold a cycle of the
/// unoriented grid graph; a depth first visit from each free cell not
/// visited yet, which finds a cycle when a neighbour other than the cell it
/// comes from is already visited, O( n^2 )
/// \pre M is square
inline bool hasCircuit( const BoolMatrix & M )
{
 const int n = int( M.size() );
 ADS_PRE( square( M ) );
 BoolMatrix visited( n , std::vector< bool >( n , false ) );
 auto rec = [ & ]( auto & self , int r , int c , int pr , int pc ) -> bool {
  visited[ r ][ c ] = true;
  for( int k = 0 ; k < 4 ; ++k ) {
   const int a = r + DR[ k ] , b = c + DC[ k ];
   if( inside( n , a , b ) && ! ( ( a == pr ) && ( b == pc ) ) ) {
    if( visited[ a ][ b ] )
     return( true );
    if( ! M[ a ][ b ] && self( self , a , b , r , c ) )
     return( true );
    }
   }
  return( false );
  };
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c )
   if( ! visited[ r ][ c ] && ! M[ r ][ c ] && rec( rec , r , c , -1 , -1 ) )
    return( true );
 return( false );
 }

}  // end( namespace graph )

ADS_TEST( "graph-42" )
{
 // a free 2 x 2 square is a circuit; free cells in a tree are not
 CHECK( graph::hasCircuit( { { false , false , true } ,
                             { false , false , true } ,
                             { true , true , true } } ) );
 CHECK( ! graph::hasCircuit( { { false , false , false } ,
                               { true , false , true } ,
                               { false , false , false } } ) );
 // a merge-find set on the edges between free cells: a cycle closes when
 // an edge joins two cells already in one component
 std::mt19937 r( 44 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  const auto M = randomBools( r , n , 0.2 + 0.4 * real( r ) );
  MFSet S( n * n );
  bool ref = false;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ! M[ i ][ j ] ) {
     if( ( j + 1 < n ) && ! M[ i ][ j + 1 ] )
      ref = ! S.merge( i * n + j , i * n + j + 1 ) || ref;
     if( ( i + 1 < n ) && ! M[ i + 1 ][ j ] )
      ref = ! S.merge( i * n + j , ( i + 1 ) * n + j ) || ref;
     }
  CHECK( graph::hasCircuit( M ) == ref );
  }
 CHECK( ! graph::hasCircuit( { { false } } ) );
 CHECK( ! graph::hasCircuit( BoolMatrix( 4 , std::vector< bool >( 4 ,
                                                                 true ) ) ) );
 CHECK( graph::hasCircuit( BoolMatrix( 2 , std::vector< bool >( 2 ,
                                                               false ) ) ) );
 // no cell; a free row; the free ring around a wall, a cycle of 8; two
 // free cells touching at a corner only
 CHECK( ! graph::hasCircuit( {} ) );
 CHECK( ! graph::hasCircuit( { { false , false , false } ,
                               { true , true , true } ,
                               { true , true , true } } ) );
 CHECK( graph::hasCircuit( { { false , false , false } ,
                             { false , true , false } ,
                             { false , false , false } } ) );
 CHECK( ! graph::hasCircuit( { { false , true } , { true , false } } ) );
 // the contracts: M square
 CHECK_THROWS( graph::hasCircuit( { { false , false } } ) ,
               contract_violation );
 }

/*----------------------- 43. 2025-01-14, Flood fill -----------------------*/

namespace graph {

/// floodfill(): paints with color the cell ( x , y ) of M and all the cells
/// 4-connected to it of its old color: floodFill() of the library, a
/// breadth first visit that paints a cell when it enters the queue, so that
/// it needs no visited matrix, and does nothing if the color does not
/// change, O( n^2 )
using ADS::floodFill;

}  // end( namespace graph )

ADS_TEST( "graph-43" )
{
 // the example of the text
 Matrix M = { { 7 , 5 , 3 , 4 , 4 , 6 , 7 , 8 } ,
              { 6 , 3 , 3 , 3 , 3 , 3 , 3 , 5 } ,
              { 4 , 3 , 3 , 3 , 2 , 1 , 3 , 4 } ,
              { 4 , 5 , 2 , 1 , 7 , 8 , 2 , 3 } ,
              { 5 , 2 , 1 , 3 , 3 , 3 , 3 , 2 } ,
              { 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 } ,
              { 1 , 4 , 1 , 4 , 1 , 4 , 1 , 4 } ,
              { 4 , 1 , 1 , 4 , 4 , 1 , 8 , 8 } };
 const Matrix R = { { 7 , 5 , 5 , 4 , 4 , 6 , 7 , 8 } ,
                    { 6 , 5 , 5 , 5 , 5 , 5 , 5 , 5 } ,
                    { 4 , 5 , 5 , 5 , 2 , 1 , 5 , 4 } ,
                    { 4 , 5 , 2 , 1 , 7 , 8 , 2 , 3 } ,
                    { 5 , 2 , 1 , 3 , 3 , 3 , 3 , 2 } ,
                    { 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 } ,
                    { 1 , 4 , 1 , 4 , 1 , 4 , 1 , 4 } ,
                    { 4 , 1 , 1 , 4 , 4 , 1 , 8 , 8 } };
 graph::floodFill( M , 1 , 6 , 5 );
 CHECK( M == R );
 // a recursive depth first fill of a copy
 std::mt19937 r( 45 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 6 );
  auto A = randomMatrix( r , n , 0 , 2 ) , B = A;
  const int x = uniform( r , 0 , n - 1 ) , y = uniform( r , 0 , n - 1 ) ,
            color = uniform( r , 0 , 2 ) , old = B[ x ][ y ];
  auto rec = [ & ]( auto & self , int i , int j ) -> void {
   if( graph::inside( n , i , j ) && ( B[ i ][ j ] == old ) ) {
    B[ i ][ j ] = color;
    for( int k = 0 ; k < 4 ; ++k )
     self( self , i + graph::DR[ k ] , j + graph::DC[ k ] );
    }
   };
  if( color != old )
   rec( rec , x , y );
  graph::floodFill( A , x , y , color );
  CHECK( A == B );
  }
 Matrix one = { { 3 } };
 graph::floodFill( one , 0 , 0 , 3 );
 CHECK( one == Matrix( { { 3 } } ) );
 graph::floodFill( one , 0 , 0 , 4 );
 CHECK( one == Matrix( { { 4 } } ) );
 // a whole board of one color; the cells of the old color touching at a
 // corner only stay; the colors of the extremes of int
 Matrix all( 50 , Ints( 50 , 0 ) );
 graph::floodFill( all , 49 , 0 , 1 );
 CHECK( all == Matrix( 50 , Ints( 50 , 1 ) ) );
 Matrix d = { { 1 , 0 } , { 0 , 1 } };
 graph::floodFill( d , 0 , 0 , 2 );
 CHECK( d == Matrix( { { 2 , 0 } , { 0 , 1 } } ) );
 Matrix e = { { INT_MIN , INT_MIN } , { INT_MAX , INT_MIN } };
 graph::floodFill( e , 0 , 1 , INT_MAX );
 CHECK( e == Matrix( Matrix( 2 , Ints( 2 , INT_MAX ) ) ) );
 // the contracts: ( x , y ) a cell
 CHECK_THROWS( graph::floodFill( d , 2 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( graph::floodFill( d , 0 , -1 , 1 ) , contract_violation );
 }

/*------------------------ 44. 2025-06-13, 1D Board ------------------------*/

namespace graph {

/// minSteps(): the least number of moves from start to the cell 0, -1 if
/// there is no way, a move going from x to x + f or to x - b, on the board,
/// onto a walkable cell; a breadth first visit, O( n )
/// \pre A[ start ] is walkable, f > 0, b > 0
inline int minSteps( const std::vector< bool > & A , int start , int f ,
                     int b )
{
 const int n = int( A.size() );
 ADS_PRE( ( 0 <= start ) && ( start < n ) && A[ start ] && ( f > 0 ) &&
          ( b > 0 ) );
 std::deque< std::pair< int , int > > Q = { { start , 0 } };
 std::vector< bool > visited( n , false );
 visited[ start ] = true;
 while( ! Q.empty() ) {
  const auto [ x , d ] = Q.front();
  Q.pop_front();
  if( x == 0 )
   return( d );
  for( long long y : { 0LL + x - b , 0LL + x + f } )  // no int overflow
   if( ( 0 <= y ) && ( y < n ) && A[ y ] && ! visited[ y ] ) {
    visited[ y ] = true;
    Q.push_back( { int( y ) , d + 1 } );
    }
  }
 return( -1 );
 }

}  // end( namespace graph )

ADS_TEST( "graph-44" )
{
 // the example, and the trap of 4 , 5 , 6 between two blocked cells
 CHECK( graph::minSteps( std::vector< bool >( 6 , true ) , 4 , 1 , 2 ) == 2 );
 const std::vector< bool > trap = { true , true , true , false , true ,
                                    true , true , false , true , true };
 CHECK( graph::minSteps( trap , 5 , 1 , 1 ) == -1 );
 // Bellman-Ford on the graph of the moves
 std::mt19937 r( 46 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 1 , 12 );
  std::vector< bool > A( n );
  for( int i = 0 ; i < n ; ++i )
   A[ i ] = real( r ) < 0.7;
  const int start = uniform( r , 0 , n - 1 ) , f = uniform( r , 1 , 4 ) ,
            b = uniform( r , 1 , 4 );
  A[ start ] = true;
  Ints d( n , INF );
  d[ start ] = 0;
  for( int k = 0 ; k < n ; ++k )
   for( int x = 0 ; x < n ; ++x )
    if( d[ x ] != INF )
     for( int y : { x - b , x + f } )
      if( ( 0 <= y ) && ( y < n ) && A[ y ] )
       d[ y ] = std::min( d[ y ] , d[ x ] + 1 );
  CHECK( graph::minSteps( A , start , f , b ) == ( d[ 0 ] == INF ? -1
                                                                 : d[ 0 ] ) );
  }
 CHECK( graph::minSteps( { true } , 0 , 1 , 1 ) == 0 );
 CHECK( graph::minSteps( { false , true } , 1 , 1 , 1 ) == -1 );
 // steps of the largest int, whose sums with a place overflowed; a step
 // back as long as the board; forward steps only, the way round
 CHECK( graph::minSteps( { true , true , true } , 2 , INT_MAX , INT_MAX ) ==
        -1 );
 CHECK( graph::minSteps( { true , true , true } , 2 , INT_MAX , 2 ) == 1 );
 CHECK( graph::minSteps( { true , true , true } , 2 , 1 , 3 ) == -1 );
 CHECK( graph::minSteps( std::vector< bool >( 7 , true ) , 6 , 2 , 3 ) == 2 );
 // a board of 10^5 cells, steps back of 1
 CHECK( graph::minSteps( std::vector< bool >( 100000 , true ) , 99999 , 5 ,
                         1 ) == 99999 );
 // the contracts: start a walkable cell, f , b > 0
 CHECK_THROWS( graph::minSteps( { true } , 1 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( graph::minSteps( { true } , -1 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( graph::minSteps( { true , false } , 1 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( graph::minSteps( { true } , 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( graph::minSteps( { true } , 0 , 1 , 0 ) , contract_violation );
 }

/*---------------------- 45. 2025-07-14, Most distant ----------------------*/

namespace graph {

/// mostDistant(): the largest distance of a sea cell (false) from the land
/// (true), the distance being the Manhattan one to the nearest land cell;
/// a breadth first visit from all the land cells at once, Theta( n^2 )
/// \pre A is square with a land cell and a sea cell
inline int mostDistant( const BoolMatrix & A )
{
 const int n = int( A.size() );
 ADS_PRE( square( A ) );
 Matrix d( n , Ints( n , INF ) );
 std::deque< std::pair< int , int > > Q;
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c )
   if( A[ r ][ c ] ) {
    d[ r ][ c ] = 0;
    Q.push_back( { r , c } );
    }
 ADS_PRE( ! Q.empty() && ( int( Q.size() ) < n * n ) );
 int best = 0;
 while( ! Q.empty() ) {
  const auto [ r , c ] = Q.front();
  Q.pop_front();
  best = std::max( best , d[ r ][ c ] );
  for( int k = 0 ; k < 4 ; ++k ) {
   const int a = r + DR[ k ] , b = c + DC[ k ];
   if( inside( n , a , b ) && ( d[ a ][ b ] == INF ) ) {
    d[ a ][ b ] = d[ r ][ c ] + 1;
    Q.push_back( { a , b } );
    }
   }
  }
 return( best );
 }

}  // end( namespace graph )

ADS_TEST( "graph-45" )
{
 // the example: land in the four corners of a 5 x 5 sea
 BoolMatrix A( 5 , std::vector< bool >( 5 , false ) );
 A[ 0 ][ 0 ] = A[ 0 ][ 4 ] = A[ 4 ][ 0 ] = A[ 4 ][ 4 ] = true;
 CHECK( graph::mostDistant( A ) == 4 );
 // each sea cell against each land cell
 std::mt19937 r( 47 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 2 , 7 );
  auto M = randomBools( r , n , real( r ) );
  M[ 0 ][ 0 ] = true;
  M[ n - 1 ][ uniform( r , 0 , n - 1 ) ] = false;
  int ref = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ! M[ i ][ j ] ) {
     int d = INF;
     for( int x = 0 ; x < n ; ++x )
      for( int y = 0 ; y < n ; ++y )
       if( M[ x ][ y ] )
        d = std::min( d , std::abs( i - x ) + std::abs( j - y ) );
     ref = std::max( ref , d );
     }
  CHECK( graph::mostDistant( M ) == ref );
  }
 CHECK( graph::mostDistant( { { true , false } , { false , false } } ) == 2 );
 BoolMatrix big( 60 , std::vector< bool >( 60 , false ) );
 big[ 0 ][ 0 ] = true;
 CHECK( graph::mostDistant( big ) == 118 );
 // one sea cell among land; a sea row between two land rows
 BoolMatrix land( 4 , std::vector< bool >( 4 , true ) );
 land[ 2 ][ 1 ] = false;
 CHECK( graph::mostDistant( land ) == 1 );
 CHECK( graph::mostDistant( { { true , true , true } ,
                              { false , false , false } ,
                              { true , true , true } } ) == 1 );
 // the contracts: A square, with a land cell and a sea cell
 CHECK_THROWS( graph::mostDistant( { { true , false } } ) ,
               contract_violation );
 CHECK_THROWS( graph::mostDistant( BoolMatrix( 3 , std::vector< bool >( 3 ,
                                   false ) ) ) , contract_violation );
 CHECK_THROWS( graph::mostDistant( BoolMatrix( 3 , std::vector< bool >( 3 ,
                                   true ) ) ) , contract_violation );
 CHECK_THROWS( graph::mostDistant( {} ) , contract_violation );
 }

/*---------------------- 46. 2025-08-25, Tassografia -----------------------*/

namespace graph {

/// categorizeGraph(): 1 if g is an arborescence, 2 if it is acyclic but not
/// an arborescence, 3 if it has a cycle; an acyclic graph with one node of
/// in-degree 0 and all the others of in-degree 1 is an arborescence,
/// O( n + m )
/// \pre g has a node
template< Graph G >
int categorize( const G & g )
{
 ADS_PRE( g.n() > 0 );
 if( hasCycle( g ) )
  return( 3 );
 Ints in( g.bound() , 0 );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++in[ v ]; } );
 int zero = 0;
 for( int u : g.V() )
  if( in[ u ] == 0 ) {
   if( ++zero > 1 )
    return( 2 );
   }
  else if( in[ u ] > 1 )
   return( 2 );
 return( 1 );
 }

}  // end( namespace graph )

ADS_TEST( "graph-46" )
{
 // Kahn for the cycles, then one root, n - 1 edges and every node reached
 std::mt19937 r( 48 );
 int kind[ 4 ] = { 0 , 0 , 0 , 0 };
 for( int t = 0 ; t < 600 ; ++t ) {
  const int n = uniform( r , 1 , 8 );
  ListGraph<> g( n , true );
  if( t % 3 == 0 )
   g = randomGraph( r , n , 0.2 + 0.4 * real( r ) , true );
  else if( t % 3 == 1 )
   g = randomDag( r , n , 0.5 * real( r ) );
  else {                                      // an arborescence
   const auto tree = randomTree( r , n , 3 );
   const int root = uniform( r , 0 , n - 1 );
   const auto p = distance( tree , root ).second;
   for( int v = 0 ; v < n ; ++v )
    if( v != root )
     g.insertEdge( p[ v ] , v );
   }
  int ref = 3;
  if( int( topsortKahn( g ).size() ) == n ) {
   Ints in( n , 0 );
   for( int u = 0 ; u < n ; ++u )
    for( int v : g.adj( u ) )
     ++in[ v ];
   const int roots = int( std::count( in.begin() , in.end() , 0 ) );
   ref = 2;
   if( ( roots == 1 ) && ( g.m() == n - 1 ) ) {
    const int root = int( std::find( in.begin() , in.end() , 0 ) -
                          in.begin() );
    if( graph::allReached( g , root ) )
     ref = 1;
    }
   }
  CHECK( graph::categorize( g ) == ref );
  ++kind[ ref ];
  }
 CHECK( ( kind[ 1 ] > 50 ) && ( kind[ 2 ] > 50 ) && ( kind[ 3 ] > 50 ) );
 CHECK( graph::categorize( ListGraph<>( 1 ) ) == 1 );
 CHECK( graph::categorize( ListGraph<>( 2 ) ) == 2 );
 CHECK( graph::categorize( make( 2 , { { 0 , 1 } , { 1 , 0 } } , true ) ) ==
        3 );
 CHECK( graph::categorize( make( 3 , { { 0 , 1 } , { 0 , 2 } , { 1 , 2 } } ,
                                 true ) ) == 2 );
 // a chain of 10^5 nodes, then closed into a cycle; a node with two
 // parents and one root; a deleted node leaving an arborescence
 ListGraph<> p( 100000 , true );
 for( int u = 1 ; u < 100000 ; ++u )
  p.insertEdge( u - 1 , u );
 CHECK( graph::categorize( p ) == 1 );
 p.insertEdge( 99999 , 0 );
 CHECK( graph::categorize( p ) == 3 );
 CHECK( graph::categorize( make( 4 , { { 0 , 1 } , { 0 , 2 } , { 1 , 3 } ,
                                       { 2 , 3 } } , true ) ) == 2 );
 auto h = make( 3 , { { 0 , 2 } , { 1 , 2 } } , true );
 CHECK( graph::categorize( h ) == 2 );
 h.deleteNode( 1 );
 CHECK( graph::categorize( h ) == 1 );
 // the contracts: g has a node
 CHECK_THROWS( graph::categorize( ListGraph<>( 0 ) ) , contract_violation );
 }

/*----------------- 47. 2026-01-12, Massimo raggiungibile ------------------*/

namespace graph {

/// maxReachable(): M[ u ], the largest value of a node reachable from u in
/// the unoriented graph, the largest of its connected component, from the
/// components of cc() (numbered from 1, from 0 in the text),
/// Theta( n + m )
/// \pre value has a place per name
template< Graph G >
Ints maxReachable( const G & g , const Ints & value )
{
 ADS_PRE( int( value.size() ) == g.bound() );
 if( g.n() == 0 )
  return( Ints() );
 const auto id = cc( g );
 int k = 0;
 for( int u : g.V() )
  k = std::max( k , id[ u ] );
 std::vector< std::optional< int > > top( k + 1 );
 for( int u : g.V() )
  if( ! top[ id[ u ] ] || ( *top[ id[ u ] ] < value[ u ] ) )
   top[ id[ u ] ] = value[ u ];
 Ints M( g.bound() , 0 );
 for( int u : g.V() )
  M[ u ] = *top[ id[ u ] ];
 return( M );
 }

}  // end( namespace graph )

ADS_TEST( "graph-47" )
{
 // the example: the path 1 - 2 - 3 gives 3 to every node
 CHECK( graph::maxReachable( make( 3 , { { 0 , 1 } , { 1 , 2 } } , false ) ,
                             { 1 , 2 , 3 } ) == Ints( { 3 , 3 , 3 } ) );
 // a breadth first visit from every node
 std::mt19937 r( 49 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 0 , 8 );
  const auto g = randomGraph( r , n , 0.4 * real( r ) , false );
  Ints value( n );
  for( int & x : value )
   x = uniform( r , -5 , 5 );
  Ints ref( n );
  for( int u = 0 ; u < n ; ++u ) {
   const auto d = distance( g , u ).first;
   ref[ u ] = value[ u ];
   for( int v = 0 ; v < n ; ++v )
    if( d[ v ] != INF )
     ref[ u ] = std::max( ref[ u ] , value[ v ] );
   }
  CHECK( graph::maxReachable( g , value ) == ref );
  }
 CHECK( graph::maxReachable( ListGraph<>( 0 , false ) , {} ).empty() );
 CHECK( graph::maxReachable( ListGraph<>( 3 , false ) , { -1 , -7 , 4 } ) ==
        Ints( { -1 , -7 , 4 } ) );
 // the extremes of int; equal values; a deleted node, its place left at 0
 const auto two = make( 4 , { { 0 , 1 } , { 2 , 3 } } , false );
 CHECK( graph::maxReachable( two , { INT_MIN , INT_MIN , INT_MAX , 5 } ) ==
        Ints( { INT_MIN , INT_MIN , INT_MAX , INT_MAX } ) );
 CHECK( graph::maxReachable( two , { 2 , 2 , 2 , 2 } ) == Ints( 4 , 2 ) );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 2 } } , false );
 h.deleteNode( 1 );
 CHECK( graph::maxReachable( h , { 1 , 9 , 3 } ) == Ints( { 1 , 0 , 3 } ) );
 // the contracts: value of a place per name
 CHECK_THROWS( graph::maxReachable( two , { 1 , 2 , 3 } ) ,
               contract_violation );
 }

/*--------------------- 48. 2026-06-22, Nodi aciclici ----------------------*/

namespace graph {

/// acyclicNodes(): the number of nodes on no cycle, those whose strongly
/// connected component has one node, from scc(), O( n + m )
template< Graph G >
int acyclicNodes( const G & g )
{
 const auto id = scc( g );
 Ints size( g.bound() + 1 , 0 );
 for( int u : g.V() )
  ++size[ id[ u ] ];
 int count = 0;
 for( int u : g.V() )
  if( size[ id[ u ] ] == 1 )
   ++count;
 return( count );
 }

/// the same count by a depth first visit from each node s that looks for
/// an edge back to s (reachesStart()), O( n ( n + m ) )
template< Graph G >
int acyclicNodesNaive( const G & g )
{
 int count = 0;
 for( int s : g.V() ) {
  std::vector< bool > visited( g.bound() , false );
  auto rec = [ & ]( auto & self , int u ) -> bool {
   visited[ u ] = true;
   for( int v : g.adj( u ) )
    if( ( v == s ) || ( ! visited[ v ] && self( self , v ) ) )
     return( true );
   return( false );
   };
  if( ! rec( rec , s ) )
   ++count;
  }
 return( count );
 }

}  // end( namespace graph )

ADS_TEST( "graph-48" )
{
 // a -> b -> c -> d -> b has only a acyclic, a -> b -> c all three; and a
 // graph with seven acyclic nodes around a cycle of 3 and one of 2
 CHECK( graph::acyclicNodes( make( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                                         { 3 , 1 } } , true ) ) == 1 );
 CHECK( graph::acyclicNodes( make( 3 , { { 0 , 1 } , { 1 , 2 } } , true ) ) ==
        3 );
 const auto f = make( 12 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } , { 3 , 4 } ,
                             { 4 , 3 } , { 5 , 0 } , { 2 , 6 } , { 6 , 7 } ,
                             { 4 , 8 } , { 9 , 3 } , { 10 , 9 } , { 7 , 3 } } ,
                      true );
 CHECK( graph::acyclicNodes( f ) == 7 );
 CHECK( graph::acyclicNodesNaive( f ) == 7 );
 // the transitive closure: s is acyclic if it does not reach itself
 std::mt19937 r( 50 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = uniform( r , 0 , 8 );
  const auto g = randomGraph( r , n , 0.4 * real( r ) , true );
  const auto R = closure( g , false );
  int ref = 0;
  for( int s = 0 ; s < n ; ++s )
   ref += R[ s ][ s ] ? 0 : 1;
  CHECK( graph::acyclicNodes( g ) == ref );
  CHECK( graph::acyclicNodesNaive( g ) == ref );
  }
 // no node; a cycle of two; a deleted node breaking a cycle; a cycle of
 // 3000 nodes, the naive recursion as deep
 CHECK( graph::acyclicNodes( ListGraph<>( 0 ) ) == 0 );
 CHECK( graph::acyclicNodesNaive( ListGraph<>( 0 ) ) == 0 );
 const auto two = make( 3 , { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } } , true );
 CHECK( ( graph::acyclicNodes( two ) == 1 ) &&
        ( graph::acyclicNodesNaive( two ) == 1 ) );
 auto h = make( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } , true );
 h.deleteNode( 1 );
 CHECK( ( graph::acyclicNodes( h ) == 2 ) &&
        ( graph::acyclicNodesNaive( h ) == 2 ) );
 ListGraph<> c( 3000 , true );
 for( int u = 0 ; u < 3000 ; ++u )
  c.insertEdge( u , ( u + 1 ) % 3000 );
 CHECK( ( graph::acyclicNodes( c ) == 0 ) &&
        ( graph::acyclicNodesNaive( c ) == 0 ) );
 }

/*----------------- 49. 2026-08-28, Cavallo su scacchiera ------------------*/

namespace graph {

/// minKnightMoves(): the least number of moves of a knight from
/// ( sr , sc ) to ( er , ec ) on an n x n board, -1 if it cannot get
/// there: knightDistance() of the library, a breadth first visit of the
/// graph of the knight moves that stops when it takes the destination out
/// of the queue, Theta( n^2 )
/// \pre n > 0, both squares on the board
inline int minKnightMoves( int n , int sr , int sc , int er , int ec )
{
 const int d = knightDistance( n , GridCell( sr , sc ) , GridCell( er , ec ) );
 return( d == INF ? -1 : d );
 }

}  // end( namespace graph )

ADS_TEST( "graph-49" )
{
 // a relaxation to the fixed point on the graph of the knight moves, from
 // every square of the boards up to 7 x 7
 static constexpr int R[ 8 ] = { -2 , -2 , -1 , -1 , 1 , 1 , 2 , 2 };
 static constexpr int C[ 8 ] = { -1 , 1 , -2 , 2 , -2 , 2 , -1 , 1 };
 for( int n = 1 ; n <= 7 ; ++n )
  for( int s = 0 ; s < n * n ; ++s ) {
   Ints d( n * n , INF );
   d[ s ] = 0;
   for( bool more = true ; more ; ) {
    more = false;
    for( int u = 0 ; u < n * n ; ++u )
     if( d[ u ] != INF )
      for( int k = 0 ; k < 8 ; ++k ) {
       const int a = u / n + R[ k ] , b = u % n + C[ k ];
       if( graph::inside( n , a , b ) && ( d[ u ] + 1 < d[ a * n + b ] ) ) {
        d[ a * n + b ] = d[ u ] + 1;
        more = true;
        }
       }
    }
   for( int e = 0 ; e < n * n ; ++e ) {
    const int x = graph::minKnightMoves( n , s / n , s % n , e / n , e % n );
    CHECK( x == ( d[ e ] == INF ? -1 : d[ e ] ) );
    if( n >= 4 )
     CHECK( x >= 0 );
    }
   }
 // the small boards of the text
 CHECK( graph::minKnightMoves( 1 , 0 , 0 , 0 , 0 ) == 0 );
 CHECK( graph::minKnightMoves( 2 , 0 , 0 , 1 , 1 ) == -1 );
 CHECK( graph::minKnightMoves( 3 , 1 , 1 , 0 , 0 ) == -1 );
 CHECK( graph::minKnightMoves( 3 , 0 , 0 , 1 , 1 ) == -1 );
 CHECK( graph::minKnightMoves( 3 , 0 , 0 , 0 , 1 ) >= 0 );
 // a board of 1000 x 1000: the same square, one move, the square next to
 // a corner from it (3 moves), and the far corner, ( 999 + 999 ) / 3 = 666
 // moves since each move goes 3 at most along the diagonal
 CHECK( graph::minKnightMoves( 1000 , 5 , 5 , 5 , 5 ) == 0 );
 CHECK( graph::minKnightMoves( 1000 , 0 , 0 , 1 , 2 ) == 1 );
 CHECK( graph::minKnightMoves( 1000 , 0 , 0 , 1 , 0 ) == 3 );
 CHECK( graph::minKnightMoves( 1000 , 0 , 0 , 999 , 999 ) == 666 );
 // the contracts: n > 0, both squares on the board
 CHECK_THROWS( graph::minKnightMoves( 0 , 0 , 0 , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( graph::minKnightMoves( 3 , 3 , 0 , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( graph::minKnightMoves( 3 , 0 , 0 , 0 , -1 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File exam_graphb.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
