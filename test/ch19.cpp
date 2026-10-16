/*--------------------------------------------------------------------------*/
/*----------------------------- File ch19.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 19, Tecniche risolutive per problemi intrattabili: the
 * table of subsetSum(), the boxes of ff and ffd, the circuit of the
 * spanning tree for the triangular salesman and its worst case, the gap of
 * Teorema 19.2, the example of apSubsetSum(), the tree of the choices of
 * bbTsp() on Figura 19.5, greedyTsp() and the local search on the same
 * data, min-max-greedy, the configurations of the fifteen puzzle of
 * Figura 19.9, and the exercises 19.1 to 19.19, each a test named after
 * it; the algorithms are checked against the enumerations on random
 * instances.
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

#include "ADS/geometry/ConvexHull.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/intractable/BranchAndBound.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/intractable/Heuristics.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/optimization/Knapsack.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Longs = std::vector< long >;
using Matrix = std::vector< Longs >;

/// the distances of Figura 19.5, the cities 1 to 5 as 0 to 4
const Matrix fig195 = { { 0 , 3 , 4 , 2 , 7 } , { 3 , 0 , 4 , 6 , 3 } ,
                        { 4 , 4 , 0 , 5 , 8 } , { 2 , 6 , 5 , 0 , 6 } ,
                        { 7 , 3 , 8 , 6 , 0 } };

/// the cost of a shortest circuit, by all the permutations from 0
template< class T >
T bruteTsp( const std::vector< std::vector< T > > & D )
{
 Nodes s( D.size() );
 std::iota( s.begin() , s.end() , 0 );
 T best = tourCost( D , s );
 while( std::next_permutation( s.begin() + 1 , s.end() ) )
  best = std::min( best , tourCost( D , s ) );
 return( best );
 }

/// the nodes 0 , ... , n - 1
Nodes nodes( int n )
{
 Nodes S( n );
 std::iota( S.begin() , S.end() , 0 );
 return( S );
 }

/// a random matrix of distances in [ 1 , w ], symmetric or not
Matrix random_distances( int n , long w , bool symmetric , std::mt19937 & g )
{
 Matrix D( n , Longs( n , 0 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( u != v ) {
    if( symmetric && ( v < u ) ) D[ u ][ v ] = D[ v ][ u ];
    else D[ u ][ v ] = 1 + long( g() % w );
    }
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

/// the Euclidean distances of the points
std::vector< std::vector< double > > euclid( const std::vector< Point > & P )
{
 std::vector< std::vector< double > > D( P.size() ,
                                        std::vector< double >( P.size() ) );
 for( std::size_t i = 0 ; i < P.size() ; ++i )
  for( std::size_t j = 0 ; j < P.size() ; ++j )
   D[ i ][ j ] = std::hypot( double( P[ i ].x - P[ j ].x ) ,
                             double( P[ i ].y - P[ j ].y ) );
 return( D );
 }

/// the least number of boxes of capacity k, by backtrack
int bruteBins( const Longs & A , long k )
{
 int best = int( A.size() );
 std::vector< long > free;
 auto rec = [ & ]( auto & self , std::size_t i ) -> void {
  if( int( free.size() ) >= best ) return;
  if( i == A.size() ) {
   best = int( free.size() );
   return;
   }
  for( std::size_t b = 0 ; b < free.size() ; ++b )
   if( free[ b ] >= A[ i ] ) {
    free[ b ] -= A[ i ];
    self( self , i + 1 );
    free[ b ] += A[ i ];
    }
  free.push_back( k - A[ i ] );
  self( self , i + 1 );
  free.pop_back();
  };
 rec( rec , 0 );
 return( best );
 }

/// the number of moves of a shortest solution, by breadth first search
int bfsPuzzle( const Board & B , int n )
{
 const Board goal = puzzleGoal( n );
 std::map< Board , int > d;
 std::queue< Board > Q;
 d[ B ] = 0;
 Q.push( B );
 while( ! Q.empty() ) {
  Board X = Q.front();
  Q.pop();
  if( X == goal ) return( d[ X ] );
  for( const Board & Y : puzzleMoves( X , n ) )
   if( ! d.count( Y ) ) {
    d[ Y ] = d[ X ] + 1;
    Q.push( Y );
    }
  }
 return( -1 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the table of the text for A = [ 7 , 11 , 13 ] and k = 24, and the
// dynamic programming against the backtrack on random instances
ADS_TEST( "sec19.1.subset-sum" )
{
 std::vector< std::vector< bool > > rows;
 CHECK( subsetSumDP( { 7 , 11 , 13 } , 24 , &rows ) );
 const std::vector< std::vector< long > > ones = { { 0 } , { 0 , 7 } ,
  { 0 , 7 , 11 , 18 } , { 0 , 7 , 11 , 13 , 18 , 20 , 24 } };
 CHECK( rows.size() == 4 );
 for( std::size_t i = 0 ; i < 4 ; ++i ) {
  Longs J;
  for( long j = 0 ; j <= 24 ; ++j ) if( rows[ i ][ j ] ) J.push_back( j );
  CHECK( J == ones[ i ] );
  }
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs A( g() % 9 );
  for( auto & a : A ) a = 1 + long( g() % 20 );
  const long k = 1 + long( g() % 60 );
  const auto S = subsetSumSet( A , k );
  CHECK( subsetSumDP( A , k ) == subsetSumExists( A , k ).has_value() );
  CHECK( S.has_value() == subsetSumDP( A , k ) );
  if( S ) {
   long s = 0;
   for( int i : *S ) s += A[ i ];
   CHECK( s == k );
   }
  }
 }

// ff and ffd: every box within k, N < 2 N* when N > 1, and the bounds
// 17 / 10 N* + 2 and 11 / 9 N* + 4
ADS_TEST( "sec19.2.bin-packing" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const long k = 5 + long( g() % 20 );
  Longs A( 1 + g() % 9 );
  for( auto & a : A ) a = 1 + long( g() % k );
  const int opt = bruteBins( A , k );
  for( bool dec : { false , true } ) {
   const auto box = firstFit( A , k , dec );
   const int N = 1 + *std::max_element( box.begin() , box.end() );
   Longs load( N , 0 );
   for( std::size_t i = 0 ; i < A.size() ; ++i ) load[ box[ i ] ] += A[ i ];
   CHECK( *std::max_element( load.begin() , load.end() ) <= k );
   int half = 0;                         // boxes filled at most half
   for( long l : load ) half += 2 * l <= k;
   CHECK( half <= 1 );
   CHECK( N >= opt );
   CHECK( ( N == 1 ) || ( N < 2 * opt ) );
   CHECK( 10 * N < 17 * opt + 20 );
   if( dec ) CHECK( 9 * N < 11 * opt + 36 );
   }
  }
 }

// the circuit of the spanning tree within twice the optimum on random
// points of the plane, and the two polygons of Figura 19.3: the optimum
// x* of 2 n cities is n ( L + L' ) / 2 + n d, and the costs of x' and x*
// tend to 4 pi and 2 pi
ADS_TEST( "sec19.2.delta-tsp" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 60 ; ++t ) {
  std::vector< Point > P( 2 + g() % 7 );
  for( auto & p : P ) p = { long( g() % 50 ) , long( g() % 50 ) };
  const auto D = euclid( P );
  const Nodes S = deltaTsp( D );
  Nodes s = S;
  std::sort( s.begin() , s.end() );
  CHECK( s == nodes( int( P.size() ) ) );
  CHECK( tourCost( D , S ) <= 2 * bruteTsp( D ) + 1e-9 );
  }
 for( int n = 4 ; n <= 4 ; ++n ) {          // even n, the zigzag x*
  const double pi = std::numbers::pi , d = 1.0 / ( n * n );
  std::vector< std::pair< double , double > > Q;
  for( int i = 0 ; i < n ; ++i ) {
   Q.emplace_back( std::cos( 2 * pi * i / n ) , std::sin( 2 * pi * i / n ) );
   Q.emplace_back( ( 1 + d ) * std::cos( 2 * pi * i / n ) ,
                   ( 1 + d ) * std::sin( 2 * pi * i / n ) );
   }
  std::vector< std::vector< double > > D( 2 * n ,
                                         std::vector< double >( 2 * n ) );
  for( int i = 0 ; i < 2 * n ; ++i )
   for( int j = 0 ; j < 2 * n ; ++j )
    D[ i ][ j ] = std::hypot( Q[ i ].first - Q[ j ].first ,
                              Q[ i ].second - Q[ j ].second );
  const double L = 2 * std::sin( pi / n ) , L1 = ( 1 + d ) * L;
  CHECK( std::abs( bruteTsp( D ) - ( n * ( L + L1 ) / 2 + n * d ) ) < 1e-9 );
  CHECK( tourCost( D , deltaTsp( D ) ) <= ( n - 1 ) * ( L + L1 ) + 2 * d +
                                          1e-9 );
  }
 const double n = 1e6 , pi = std::numbers::pi;
 const double cx = 2 * ( n - 1 ) * ( 2 + 1 / ( n * n ) ) * std::sin( pi / n ) +
                   2 / ( n * n );
 const double cs = n * ( 2 + 1 / ( n * n ) ) * std::sin( pi / n ) + 1 / n;
 CHECK( std::abs( cx - 4 * pi ) < 1e-4 && std::abs( cs - 2 * pi ) < 1e-4 );
 }

// Teorema 19.2: with the distances 1 and s n, the optimum is n if G has a
// Hamiltonian circuit and more than s n otherwise
ADS_TEST( "sec19.2.gap" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 3 + int( g() % 5 );
  const long s = 1 + long( g() % 3 );
  const auto G = random_graph( n , 55 , g );
  const long opt = bruteTsp( tspGap( G , s ) );
  if( hamiltonianCircuit( G ) ) CHECK( opt == n );
  else CHECK( opt > s * n );
  }
 }

// the example of the text, n = 8, k = 45: 44 by { 23 , 13 , 8 } with
// h = 1 and the optimum 45 with h = 2; the error within 1 / ( h + 1 ) on
// random instances
ADS_TEST( "sec19.2.ap-subset-sum" )
{
 const Longs A = { 23 , 19 , 13 , 12 , 11 , 8 , 7 , 5 };
 const auto [ z1 , S1 ] = apSubsetSum( A , 45 , 1 );
 Longs v;
 for( int i : S1 ) v.push_back( A[ i ] );
 std::sort( v.rbegin() , v.rend() );
 CHECK( z1 == 44 && v == Longs( { 23 , 13 , 8 } ) );
 CHECK( apSubsetSum( A , 45 , 2 ).first == 45 );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs B( 1 + g() % 9 );
  for( auto & a : B ) a = 1 + long( g() % 30 );
  const long k = 1 + long( g() % 100 );
  const int h = 1 + int( g() % 3 );
  long opt = k;
  while( ! subsetSumDP( B , opt ) ) --opt;
  const auto [ z , S ] = apSubsetSum( B , k , h );
  long s = 0;
  for( int i : S ) s += B[ i ];
  CHECK( s == z && z <= k );
  CHECK( ( opt - z ) * ( h + 1 ) <= opt );
  }
 }

// Figura 19.5: lb = 18 for S = 1 , 3; from the circuit 0 3 2 1 4 of cost
// 21 the tree ends with 1 3 2 5 4 of cost 19, with 25 nodes of the 65
// possible (23 in Figura 19.6, two of whose bounds, 21 for 1 2 3 and for
// 1 5, are 19 and 20 by the formula of the text); on random instances the
// optimum, with both bounds
ADS_TEST( "sec19.3.bb-tsp" )
{
 CHECK( tspLowerBound( fig195 , { 0 , 2 } ) == 18 );
 long gen = 0;
 const auto [ c , S ] = bbTsp( fig195 , { 0 , 3 , 2 , 1 , 4 } ,
                               TspBound::BOOK , &gen );
 CHECK( c == 19 && S == Nodes( { 0 , 2 , 1 , 4 , 3 } ) && gen == 25 );
 CHECK( bbTsp( fig195 , {} , TspBound::ONE_TREE ).first == 19 );
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  const auto D = random_distances( n , 20 , g() % 2 , g );
  const auto [ cb , Sb ] = bbTsp( D );
  CHECK( cb == bruteTsp( D ) && tourCost( D , Sb ) == cb );
  const auto E = random_distances( n , 20 , true , g );
  CHECK( bbTsp( E , {} , TspBound::ONE_TREE ).first == bruteTsp( E ) );
  }
 }

// greedyTsp() on Figura 19.5: the edges [ 1 , 4 ] , [ 1 , 2 ] , [ 2 , 5 ] ,
// [ 3 , 4 ] and [ 3 , 5 ], the circuit 1 4 3 5 2 of cost 21
ADS_TEST( "sec19.4.greedy-tsp" )
{
 const auto [ E , C ] = greedyTsp( fig195 );
 const std::vector< std::pair< int , int > > book = { { 0 , 3 } , { 0 , 1 } ,
                                         { 1 , 4 } , { 2 , 3 } , { 2 , 4 } };
 CHECK( E == book );
 CHECK( C == Nodes( { 0 , 3 , 2 , 4 , 1 } ) && tourCost( fig195 , C ) == 21 );
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 8 );
  const auto D = random_distances( n , 30 , true , g );
  Nodes s = greedyTsp( D ).second;
  std::sort( s.begin() , s.end() );
  CHECK( s == nodes( n ) );
  }
 }

// the local search of Figura 19.8: 1 2 3 4 5 of cost 25, 1 2 3 5 4 of
// cost 23 by exchanging [ 1 , 5 ] , [ 3 , 4 ] with [ 1 , 4 ] , [ 3 , 5 ],
// and 1 3 2 5 4, the optimum 19; twoOpt() stops at a local optimum
ADS_TEST( "sec19.4.local-search" )
{
 const Nodes pi = { 0 , 1 , 2 , 3 , 4 };
 const Nodes p1 = twoOptMove( pi , 2 , 4 ) , p2 = twoOptMove( p1 , 0 , 2 );
 CHECK( tourCost( fig195 , pi ) == 25 );
 CHECK( p1 == Nodes( { 0 , 1 , 2 , 4 , 3 } ) );
 CHECK( tourCost( fig195 , p1 ) == 23 );
 CHECK( p2 == Nodes( { 0 , 2 , 1 , 4 , 3 } ) );
 CHECK( tourCost( fig195 , p2 ) == 19 );
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 4 + int( g() % 5 );
  const auto D = random_distances( n , 30 , true , g );
  Nodes s = nodes( n );
  std::shuffle( s.begin() , s.end() , g );
  Longs trace;
  const Nodes L = twoOpt( D , s , &trace );
  CHECK( std::is_sorted( trace.rbegin() , trace.rend() ) );
  CHECK( trace.back() == tourCost( D , L ) && trace.back() >= bruteTsp( D ) );
  for( int i = 0 ; i < n ; ++i )         // no neighbour is better
   for( int j = i + 2 ; j < n ; ++j )
    if( ( i > 0 ) || ( j < n - 1 ) )
     CHECK( tourCost( D , twoOptMove( L , i , j ) ) >= trace.back() );
  }
 }

// min-max-greedy: two halves, never better than the optimum, and the best
// of the starting pairs against it
ADS_TEST( "sec19.4.bisection" )
{
 std::mt19937 g( 9 );
 int hit = 0;
 for( int t = 0 ; t < 60 ; ++t ) {
  const int n = 2 * ( 1 + int( g() % 4 ) );
  const auto G = random_graph( n , 40 , g );
  int opt = 0;
  while( ! bisection( G , opt ) ) ++opt;
  int best = n * n;
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b ) {
    if( a == b ) continue;
    const auto side = minMaxGreedy( G , a , b );
    CHECK( std::count( side.begin() , side.end() , 0 ) == n / 2 );
    CHECK( side[ a ] == 0 && side[ b ] == 1 );
    best = std::min( best , cutSize( G , side ) );
    }
  CHECK( best >= opt );
  hit += best == opt;
  }
 CHECK( hit > 50 );
 }

// Figura 19.9: the bounds 8, 9 and 10 of (a), (b) and (c), the three
// children of (a), and the solution of (a); random scrambles of 3 x 3 and
// 4 x 4 boards against the breadth first search
ADS_TEST( "sec19.5.fifteen" )
{
 const Board a = { 2 , 3 , 0 , 4 , 1 , 5 , 7 , 8 , 9 , 6 , 10 , 12 ,
                   13 , 14 , 11 , 15 };
 const Board b = { 2 , 3 , 7 , 4 , 1 , 5 , 0 , 8 , 9 , 6 , 10 , 12 ,
                   13 , 14 , 11 , 15 };
 const Board c = { 2 , 3 , 7 , 4 , 1 , 5 , 8 , 0 , 9 , 6 , 10 , 12 ,
                   13 , 14 , 11 , 15 };
 CHECK( manhattan( a , 4 ) == 8 && manhattan( b , 4 ) == 9 &&
        manhattan( c , 4 ) == 10 );
 const auto M = puzzleMoves( a , 4 );
 CHECK( M.size() == 3 && std::count( M.begin() , M.end() , b ) == 1 );
 const auto Mb = puzzleMoves( b , 4 );
 CHECK( std::count( Mb.begin() , Mb.end() , c ) == 1 );
 long visited = 0;
 const auto S = solvePuzzle( a , 4 , &visited );
 CHECK( S && S->front() == a && S->back() == puzzleGoal( 4 ) );
 CHECK( int( S->size() ) - 1 == bfsPuzzle( a , 4 ) );
 for( std::size_t i = 0 ; i + 1 < S->size() ; ++i ) {
  const auto N = puzzleMoves( ( *S )[ i ] , 4 );
  CHECK( std::count( N.begin() , N.end() , ( *S )[ i + 1 ] ) == 1 );
  }
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 30 ; ++t ) {
  const int n = 3 + int( g() % 2 );
  Board B = puzzleGoal( n );
  for( int m = 0 ; m < 6 + int( g() % 10 ) ; ++m ) {
   const auto N = puzzleMoves( B , n );
   B = N[ g() % N.size() ];
   }
  const auto T = solvePuzzle( B , n );
  CHECK( T && int( T->size() ) - 1 == bfsPuzzle( B , n ) );
  }
 Board U = puzzleGoal( 3 );
 std::swap( U[ 0 ] , U[ 1 ] );           // an odd permutation
 CHECK( ! solvable( U , 3 ) && ! solvePuzzle( U , 3 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// PARTIZIONE by dynamic programming against the backtrack
ADS_TEST( "ex19.1" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs A( g() % 10 );
  for( auto & a : A ) a = 1 + long( g() % 15 );
  const auto S = partitionDP( A );
  CHECK( S.has_value() == partition( A ).has_value() );
  if( S ) {
   long s = 0;
   for( int i : *S ) s += A[ i ];
   CHECK( 2 * s == std::accumulate( A.begin() , A.end() , 0L ) );
   }
  }
 }

// SCHEDULING MULTIPROCESSORE by dynamic programming on the loads
ADS_TEST( "ex19.2" )
{
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs T( 1 + g() % 7 );
  for( auto & x : T ) x = 1 + long( g() % 8 );
  const int m = 1 + int( g() % 3 );
  const long d = long( g() % 20 );
  const auto a = schedulingDP( T , m , d );
  CHECK( a.has_value() == multiprocessorScheduling( T , m , d ).has_value() );
  if( a ) {
   Longs load( m , 0 );
   for( std::size_t i = 0 ; i < T.size() ; ++i ) load[ ( *a )[ i ] ] += T[ i ];
   CHECK( *std::max_element( load.begin() , load.end() ) <= d );
   }
  }
 }

// McNaughton: each program for its time, within d on each processor, its
// pieces never at the same time, and d the least possible
ADS_TEST( "ex19.3" )
{
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 200 ; ++t ) {
  std::vector< double > T( 1 + g() % 10 );
  for( auto & x : T ) x = 0.5 + double( g() % 100 ) / 10;
  const int m = 1 + int( g() % 4 );
  const auto [ d , P ] = mcNaughton( T , m );
  double lb = std::accumulate( T.begin() , T.end() , 0.0 ) / m;
  for( double x : T ) lb = std::max( lb , x );
  CHECK( std::abs( d - lb ) < 1e-9 );
  std::vector< std::vector< Piece > > of( T.size() );
  for( const auto & q : P )
   for( const auto & p : q ) {
    CHECK( p.start >= -1e-9 && p.end <= d + 1e-9 && p.start < p.end );
    of[ p.program ].push_back( p );
    }
  for( std::size_t i = 0 ; i < T.size() ; ++i ) {
   double s = 0;
   for( const auto & p : of[ i ] ) s += p.end - p.start;
   CHECK( std::abs( s - T[ i ] ) < 1e-9 && of[ i ].size() <= 2 );
   if( of[ i ].size() == 2 )
    CHECK( std::min( of[ i ][ 0 ].end , of[ i ][ 1 ].end ) <=
           std::max( of[ i ][ 0 ].start , of[ i ][ 1 ].start ) + 1e-9 );
   }
  }
 }

// REAL-TIME SCHEDULING: feasible if and only if the utilization is at
// most m and no task exceeds its period
ADS_TEST( "ex19.4" )
{
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const std::size_t k = 1 + g() % 6;
  std::vector< double > C( k ) , T( k );
  double u = 0 , umax = 0;
  for( std::size_t i = 0 ; i < k ; ++i ) {
   T[ i ] = 1 + double( g() % 10 );
   C[ i ] = 1 + double( g() % 10 );
   u += C[ i ] / T[ i ];
   umax = std::max( umax , C[ i ] / T[ i ] );
   }
  const int m = 1 + int( g() % 4 );
  const auto S = realTimeScheduling( C , T , m );
  CHECK( S.has_value() == ( ( u <= m + 1e-12 ) && ( umax <= 1 + 1e-12 ) ) );
  }
 }

// bottino() against the 3^n assignments
ADS_TEST( "ex19.5" )
{
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 200 ; ++t ) {
  Longs P( 1 + g() % 8 );
  for( auto & p : P ) p = 1 + long( g() % 9 );
  const long m = std::accumulate( P.begin() , P.end() , 0L );
  bool ref = false;
  if( m % 3 == 0 ) {
   int c = 1;
   for( std::size_t i = 0 ; i < P.size() ; ++i ) c *= 3;
   for( int x = 0 ; x < c && ! ref ; ++x ) {
    long s[ 3 ] = { 0 , 0 , 0 };
    for( int y = x , i = 0 ; i < int( P.size() ) ; ++i , y /= 3 )
     s[ y % 3 ] += P[ i ];
    ref = s[ 0 ] == m / 3 && s[ 1 ] == m / 3;
    }
   }
  const auto a = bottino( P );
  CHECK( a.has_value() == ref );
  if( a ) {
   long s[ 3 ] = { 0 , 0 , 0 };
   for( std::size_t i = 0 ; i < P.size() ; ++i ) s[ ( *a )[ i ] ] += P[ i ];
   CHECK( s[ 0 ] == m / 3 && s[ 1 ] == m / 3 && s[ 2 ] == m / 3 );
   }
  }
 }

// the cover by the ends of a matching: a cover, at most twice the least
ADS_TEST( "ex19.6" )
{
 std::mt19937 g( 16 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  const auto G = random_graph( n , int( g() % 70 ) , g );
  const Nodes S = vertexCoverApprox( G );
  std::vector< bool > in( n , false );
  for( int u : S ) in[ u ] = true;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    CHECK( ! G[ u ][ v ] || in[ u ] || in[ v ] );
  int opt = 0;
  while( ! vertexCover( G , opt ) ) ++opt;
  CHECK( int( S.size() ) <= 2 * opt );
  }
 }

// an optimal Euclidean circuit never crosses itself
ADS_TEST( "ex19.7" )
{
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 60 ; ++t ) {
  std::vector< Point > P( 4 + g() % 4 );
  for( auto & p : P ) p = { long( g() % 30 ) , long( g() % 30 ) };
  const auto D = euclid( P );
  Nodes s = nodes( int( P.size() ) ) , best = s;
  while( std::next_permutation( s.begin() + 1 , s.end() ) )
   if( tourCost( D , s ) < tourCost( D , best ) - 1e-9 ) best = s;
  const int n = int( best.size() );
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 2 ; j < n ; ++j ) {
    if( ( i == 0 ) && ( j == n - 1 ) ) continue;
    const Point a = P[ best[ i ] ] , b = P[ best[ i + 1 ] ] ,
                c = P[ best[ j ] ] , d = P[ best[ ( j + 1 ) % n ] ];
    const long c1 = cross( a , b , c ) , c2 = cross( a , b , d ) ,
               c3 = cross( c , d , a ) , c4 = cross( c , d , b );
    const bool proper = ( ( c1 > 0 ) != ( c2 > 0 ) ) &&
                        ( ( c3 > 0 ) != ( c4 > 0 ) ) && c1 && c2 && c3 && c4;
    CHECK( ! proper );
    }
  }
 }

// the epsilon-approximation of ZAINO: feasible and within ( 1 - eps )
ADS_TEST( "ex19.8" )
{
 std::mt19937 g( 18 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 10 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + long( g() % 100 );
   v[ i ] = 1 + long( g() % 20 );
   }
  const long C = long( g() % 60 );
  const double eps = 0.05 + double( g() % 50 ) / 100;
  const auto [ z , S ] = knapsackApprox( p , v , C , eps );
  long tp = 0 , tv = 0;
  for( int i : S ) {
   tp += p[ i ];
   tv += v[ i ];
   }
  const long opt = knapsack( p , v , C ).first;
  CHECK( tp == z && tv <= C && z <= opt );
  CHECK( double( z ) >= ( 1 - eps ) * double( opt ) - 1e-9 );
  }
 }

// CRICCA DI PESO MASSIMO by branch-&-bound against all the subsets
ADS_TEST( "ex19.9" )
{
 std::mt19937 g( 19 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  const auto G = random_graph( n , int( g() % 90 ) , g );
  Longs w( n );
  for( auto & x : w ) x = long( g() % 10 );
  long ref = 0;
  for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
   bool ok = true;
   long s = 0;
   for( int u = 0 ; u < n ; ++u ) {
    if( !( S >> u & 1 ) ) continue;
    s += w[ u ];
    for( int v = u + 1 ; v < n ; ++v )
     ok = ok && ( !( S >> v & 1 ) || G[ u ][ v ] );
    }
   if( ok ) ref = std::max( ref , s );
   }
  const auto [ z , S ] = maxWeightClique( G , w );
  long s = 0;
  for( int u : S ) s += w[ u ];
  CHECK( z == ref && s == z && cliqueCertificate( G , S , int( S.size() ) ) );
  }
 }

// the minimum 1-tree bounds the optimum, and bbTsp() with it on the data
// of Figura 19.5 finds 19
ADS_TEST( "ex19.10" )
{
 CHECK( oneTree( fig195 , 0 ) <= 19 );
 long gen = 0;
 CHECK( bbTsp( fig195 , { 0 , 3 , 2 , 1 , 4 } , TspBound::ONE_TREE ,
               &gen ).first == 19 );
 CHECK( gen <= 65 );
 std::mt19937 g( 20 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 3 + int( g() % 5 );
  const auto D = random_distances( n , 30 , true , g );
  const long opt = bruteTsp( D );
  for( int r = 0 ; r < n ; ++r ) CHECK( oneTree( D , r ) <= opt );
  }
 }

// the algorithm of Little on the data of Figura 19.5 and on random
// asymmetric distances
ADS_TEST( "ex19.11" )
{
 CHECK( littleTsp( fig195 ).first == 19 );
 std::mt19937 g( 21 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  const auto D = random_distances( n , 30 , g() % 2 , g );
  const auto [ c , S ] = littleTsp( D );
  Nodes s = S;
  std::sort( s.begin() , s.end() );
  CHECK( c == bruteTsp( D ) && tourCost( D , S ) == c && s == nodes( n ) );
  }
 }

// ZAINO by branch-&-bound against the dynamic programming
ADS_TEST( "ex19.12" )
{
 std::mt19937 g( 22 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = int( g() % 12 );
  Longs p( n ) , v( n );
  for( int i = 0 ; i < n ; ++i ) {
   p[ i ] = 1 + long( g() % 30 );
   v[ i ] = 1 + long( g() % 15 );
   }
  const long C = long( g() % 50 );
  const auto [ z , S ] = knapsackBB( p , v , C );
  long tp = 0 , tv = 0;
  for( int i : S ) {
   tp += p[ i ];
   tv += v[ i ];
   }
  CHECK( z == knapsack( p , v , C ).first && tp == z && tv <= C );
  }
 }

// the overlaps of the text, ACGA and GACA, and the greedy superstring
// contains every string
ADS_TEST( "ex19.13" )
{
 CHECK( overlap( "ACGA" , "GACA" ) == 2 && overlap( "GACA" , "ACGA" ) == 1 );
 CHECK( greedySuperstring( { "ACGA" , "GACA" } ) == "ACGACA" );
 std::mt19937 g( 23 );
 for( int t = 0 ; t < 200 ; ++t ) {
  std::vector< std::string > P( 1 + g() % 6 );
  for( auto & s : P ) {
   s.resize( 1 + g() % 6 );
   for( auto & ch : s ) ch = "ACGT"[ g() % 4 ];
   }
  const std::string T = greedySuperstring( P );
  std::size_t total = 0;
  for( const auto & s : P ) {
   CHECK( T.find( s ) != std::string::npos );
   total += s.size();
   }
  CHECK( T.size() <= total );
  }
 }

// an instance of K6 with a local optimum pi of I2 worse than the global
// optimum, which is not in I2( pi )
ADS_TEST( "ex19.14" )
{
 std::mt19937 g( 24 );
 bool found = false;
 for( int t = 0 ; t < 1000 && ! found ; ++t ) {
  const auto D = random_distances( 6 , 5 , true , g );
  Nodes s = nodes( 6 );
  std::shuffle( s.begin() + 1 , s.end() , g );
  const Nodes pi = twoOpt( D , s );
  const long opt = bruteTsp( D );
  if( tourCost( D , pi ) == opt ) continue;
  found = true;
  for( int i = 0 ; i < 6 ; ++i )
   for( int j = i + 2 ; j < 6 ; ++j )
    if( ( i > 0 ) || ( j < 5 ) )
     CHECK( tourCost( D , twoOptMove( pi , i , j ) ) > opt );
  }
 CHECK( found );
 }

// |I2| = n ( n - 1 ) / 2 - n, and |I3| exceeds the triples of edges that
// do not touch, n ( n - 4 ) ( n - 5 ) / 6, so that three edges are joined
// again in more than one way; |I_k| is O( n^k )
ADS_TEST( "ex19.15" )
{
 for( long n = 5 ; n <= 8 ; ++n ) {
  CHECK( kOptNeighbours( int( n ) , 2 ) == n * ( n - 1 ) / 2 - n );
  const long i3 = kOptNeighbours( int( n ) , 3 );
  CHECK( i3 <= n * n * n );
  if( n >= 6 ) CHECK( i3 > n * ( n - 4 ) * ( n - 5 ) / 6 );
  }
 }

// the local search of PARTIZIONE: a local optimum, never better than the
// optimum
ADS_TEST( "ex19.16" )
{
 std::mt19937 g( 25 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  Longs A( n );
  for( auto & a : A ) a = 1 + long( g() % 20 );
  std::vector< int > side( n );
  for( auto & s : side ) s = int( g() % 2 );
  const auto L = partitionLocalSearch( A , side );
  long diff = 0;
  for( int i = 0 ; i < n ; ++i ) diff += L[ i ] ? -A[ i ] : A[ i ];
  long opt = std::accumulate( A.begin() , A.end() , 0L );
  for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
   long d = 0;
   for( int i = 0 ; i < n ; ++i ) d += ( S >> i & 1 ) ? -A[ i ] : A[ i ];
   opt = std::min( opt , std::labs( d ) );
   }
  CHECK( std::labs( diff ) >= opt );
  for( int i = 0 ; i < n ; ++i ) {       // no move improves
   const long di = L[ i ] ? A[ i ] : -A[ i ];
   CHECK( std::labs( diff + 2 * di ) >= std::labs( diff ) );
   }
  }
 }

// the buckets against the counts kept by hand
ADS_TEST( "ex19.17" )
{
 std::mt19937 g( 26 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 1 + int( g() % 12 );
  Buckets B( n );
  std::vector< int > h[ 2 ] = { std::vector< int >( n , 0 ) ,
                                std::vector< int >( n , 0 ) };
  std::vector< bool > in( n , false );
  for( int op = 0 ; op < 200 ; ++op ) {
   const int u = int( g() % n );
   if( ! in[ u ] ) {
    B.insert( u );
    in[ u ] = true;
    h[ 0 ][ u ] = h[ 1 ][ u ] = 0;
    }
   else if( g() % 4 == 0 ) {
    B.remove( u );
    in[ u ] = false;
    }
   else if( h[ 0 ][ u ] < n && h[ 1 ][ u ] < n ) {
    const int k = int( g() % 2 );
    B.inc( k , u );
    ++h[ k ][ u ];
    }
   for( int k = 0 ; k < 2 ; ++k ) {
    int lo = n + 1 , hi = -1;
    for( int v = 0 ; v < n ; ++v )
     if( in[ v ] ) {
      CHECK( B.count( k , v ) == h[ k ][ v ] );
      lo = std::min( lo , h[ k ][ v ] );
      hi = std::max( hi , h[ k ][ v ] );
      }
    CHECK( B.min( k ) == lo && B.max( k ) == hi );
    if( hi >= 0 ) {
     int dim = 0;
     for( int v = 0 ; v < n ; ++v ) dim += in[ v ] && h[ k ][ v ] == hi;
     CHECK( B.dim( k , hi ) == dim );
     }
    }
   }
  }
 }

// star alignment: the center has the largest sum of scores, and each pair
// is an optimal global alignment of the center and the string
ADS_TEST( "ex19.18" )
{
 std::mt19937 g( 27 );
 for( int t = 0 ; t < 100 ; ++t ) {
  std::vector< std::string > P( 2 + g() % 4 );
  for( auto & s : P ) {
   s.resize( 1 + g() % 7 );
   for( auto & ch : s ) ch = "ACGT"[ g() % 4 ];
   }
  const auto [ h , A ] = starAlignment( P );
  const int k = int( P.size() );
  auto sum = [ & ]( int i ) {
   long s = 0;
   for( int j = 0 ; j < k ; ++j )
    if( j != i ) s += alignmentScore( P[ i ] , P[ j ] , Alignment::GLOBAL );
   return( s );
   };
  for( int i = 0 ; i < k ; ++i ) CHECK( sum( h ) >= sum( i ) );
  for( int j = 0 ; j < k ; ++j ) {
   auto [ a , b ] = A[ j ];
   CHECK( a.size() == b.size() );
   int score = 0;
   for( std::size_t x = 0 ; x < a.size() ; ++x )
    score += ( a[ x ] == '-' || b[ x ] == '-' ) ? -2
                                                : a[ x ] == b[ x ] ? 1 : -1;
   CHECK( score == alignmentScore( P[ h ] , P[ j ] , Alignment::GLOBAL ) );
   std::erase( a , '-' );
   std::erase( b , '-' );
   CHECK( a == P[ h ] && b == P[ j ] );
   }
  }
 }

// saturation degree: a coloring, never fewer colors than the least, and
// two colors on the bipartite graphs
ADS_TEST( "ex19.19" )
{
 std::mt19937 g( 28 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  auto G = random_graph( n , int( g() % 80 ) , g );
  if( t % 2 ) {                          // bipartite: even and odd
   for( int u = 0 ; u < n ; ++u )
    for( int v = 0 ; v < n ; ++v ) if( u % 2 == v % 2 ) G[ u ][ v ] = false;
   }
  const auto col = dsatur( G );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    CHECK( ! G[ u ][ v ] || col[ u ] != col[ v ] );
  const int k = 1 + *std::max_element( col.begin() , col.end() );
  int chi = 1;
  while( ! coloring( G , chi ) ) ++chi;
  CHECK( k >= chi );
  if( t % 2 ) CHECK( k <= 2 );
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
/*---------------------------- End File ch19.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
