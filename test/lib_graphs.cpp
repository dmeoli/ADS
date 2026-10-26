/*--------------------------------------------------------------------------*/
/*-------------------------- File lib_graphs.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems on graphs of Grid.h, Paths.h, Properties.h,
 * Matching.h and FlowApplications.h: each function against an independent
 * oracle on small random instances with a fixed seed (the visits of
 * Visit.h on the explicit graph, the enumeration of the paths, of the
 * subsets of nodes or of edges, of the permutations, a relaxation to the
 * fixed point, a fresh maximum flow), at the level 3 of the contracts, and
 * on the edge cases (no node, one node, no edge, the largest counts).
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/tree/BinaryTree.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Matrix = std::vector< std::vector< int > >;

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a random integer in [ lo , hi ]
int rnd( std::mt19937 & g , int lo , int hi )
{
 return( lo + int( g() % unsigned( hi - lo + 1 ) ) );
 }

/// true with probability p
bool coin( std::mt19937 & g , double p )
{
 return( std::uniform_real_distribution< double >( 0 , 1 )( g ) < p );
 }

/// a random graph of n nodes, each edge with probability p
ListGraph<> randomGraph( std::mt19937 & g , int n , double p , bool directed )
{
 ListGraph<> G( n , directed );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && coin( g , p ) )
    G.insertEdge( u , v );
 return( G );
 }

/// a random acyclic graph, the edges from smaller to larger names of a
/// random permutation
ListGraph<> randomDag( std::mt19937 & g , int n , double p )
{
 Ints pi( n );
 std::iota( pi.begin() , pi.end() , 0 );
 std::shuffle( pi.begin() , pi.end() , g );
 ListGraph<> G( n , true );
 for( int i = 0 ; i < n ; ++i )
  for( int j = i + 1 ; j < n ; ++j )
   if( coin( g , p ) )
    G.insertEdge( pi[ i ] , pi[ j ] );
 return( G );
 }

/// a random free tree of n nodes, each node joined to an earlier one
ListGraph<> randomTree( std::mt19937 & g , int n )
{
 ListGraph<> G( n , false );
 for( int v = 1 ; v < n ; ++v )
  G.insertEdge( rnd( g , 0 , v - 1 ) , v );
 return( G );
 }

/// f( P ) on each simple path P from s, the path { s } included
template< Graph G , class F >
void simplePaths( const G & g , int s , F && f )
{
 Ints P = { s };
 std::vector< bool > on( g.bound() , false );
 on[ s ] = true;
 auto rec = [ & ]( auto & self ) -> void {
  f( P );
  for( int v : g.adj( P.back() ) )
   if( ! on[ v ] ) {
    on[ v ] = true;
    P.push_back( v );
    self( self );
    P.pop_back();
    on[ v ] = false;
    }
  };
 rec( rec );
 }

/// the explicit graph of the free cells of a grid for the moves, the cell
/// ( r , c ) as the node r * cols + c
template< class Moves >
ListGraph<> gridGraph( const Matrix & M , const Moves & moves )
{
 const int rows = int( M.size() ) , cols = rows ? int( M[ 0 ].size() ) : 0;
 ListGraph<> G( rows * cols , true );
 for( int r = 0 ; r < rows ; ++r )
  for( int c = 0 ; c < cols ; ++c )
   for( const auto & [ dr , dc ] : moves ) {
    const int a = r + dr , b = c + dc;
    if( ( M[ r ][ c ] == 0 ) && ( 0 <= a ) && ( a < rows ) && ( 0 <= b ) &&
        ( b < cols ) && ( M[ a ][ b ] == 0 ) )
     G.insertEdge( r * cols + c , a * cols + b );
    }
 return( G );
 }

/// the most pairs of a b-matching, over all the subsets of the edges
int bMatchingBrute( const std::vector< std::pair< int , int > > & E , int nl ,
                    int nr , const std::vector< long long > & cl ,
                    const std::vector< long long > & cr )
{
 int best = 0;
 for( unsigned m = 0 ; m < ( 1u << E.size() ) ; ++m ) {
  std::vector< long > dl( nl , 0 ) , dr( nr , 0 );
  bool ok = true;
  for( std::size_t i = 0 ; i < E.size() ; ++i )
   if( ( m >> i ) & 1u )
    ok = ok && ( ++dl[ E[ i ].first ] <= cl[ E[ i ].first ] ) &&
         ( ++dr[ E[ i ].second ] <= cr[ E[ i ].second ] );
  if( ok )
   best = std::max( best , std::popcount( m ) );
  }
 return( best );
 }

/// the value of the flow f out of s
long value( const std::vector< std::vector< long > > & f , int s )
{
 return( std::accumulate( f[ s ].begin() , f[ s ].end() , 0L ) );
 }

/// a maximum flow of the capacities c from s to t, as a skew matrix
std::vector< std::vector< long > > flowMatrix(
                       const std::vector< std::vector< long > > & c , int s ,
                       int t )
{
 const int n = int( c.size() );
 FlowNetwork N( n );
 std::vector< std::tuple< int , int , int > > E;
 for( int x = 0 ; x < n ; ++x )
  for( int y = 0 ; y < n ; ++y )
   if( c[ x ][ y ] > 0 )
    E.emplace_back( N.addEdge( x , y , c[ x ][ y ] ) , x , y );
 N.maxFlow( s , t );
 std::vector< std::vector< long > > f( n , std::vector< long >( n , 0 ) );
 for( const auto & [ e , x , y ] : E ) {
  f[ x ][ y ] += N.flow( e );
  f[ y ][ x ] -= N.flow( e );
  }
 return( f );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the distances from several sources on random grids with obstacles, for
// the four, the eight and the knight moves, against distance() on the
// explicit graph from each source; gridDistance() against them; the empty
// grid, a source on an obstacle
ADS_TEST( "grid-distances" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int rows = rnd( g , 1 , 6 ) , cols = rnd( g , 1 , 6 );
  Matrix M( rows , Ints( cols , 0 ) );
  for( auto & row : M )
   for( int & x : row )
    x = coin( g , 0.3 ) ? 1 : 0;
  std::vector< GridCell > S;
  for( int k = rnd( g , 0 , 3 ) ; k > 0 ; --k ) {
   const int r = rnd( g , 0 , rows - 1 ) , c = rnd( g , 0 , cols - 1 );
   M[ r ][ c ] = 0;
   S.emplace_back( r , c );
   }
  auto free = [ & ]( GridCell x ) { return( M[ x.first ][ x.second ] == 0 ); };
  auto check = [ & ]( const auto & moves ) {
   const auto G = gridGraph( M , moves );
   std::vector< Ints > ref( S.size() );
   for( std::size_t i = 0 ; i < S.size() ; ++i )
    ref[ i ] = distance( G , S[ i ].first * cols + S[ i ].second ).first;
   const auto D = gridDistances( rows , cols , S , free , moves );
   for( int r = 0 ; r < rows ; ++r )
    for( int c = 0 ; c < cols ; ++c ) {
     int e = INF;
     for( const auto & d : ref )
      e = std::min( e , d[ r * cols + c ] );
     CHECK( D[ r ][ c ] == e );
     if( ! S.empty() )
      CHECK( gridDistance( rows , cols , S[ 0 ] , GridCell( r , c ) , free ,
                           moves ) == ref[ 0 ][ r * cols + c ] );
     }
   };
  check( FOUR_MOVES );
  check( EIGHT_MOVES );
  check( KNIGHT_MOVES );
  }
 // a 40 x 40 room, corner to corner: 78 moves of a rook, 39 of a king
 auto all = []( GridCell ) { return( true ); };
 CHECK( gridDistance( 40 , 40 , GridCell( 0 , 0 ) , GridCell( 39 , 39 ) ,
                      all ) == 78 );
 CHECK( gridDistance( 40 , 40 , GridCell( 0 , 0 ) , GridCell( 39 , 39 ) ,
                      all , EIGHT_MOVES ) == 39 );
 CHECK( gridDistances( 0 , 0 , {} , all ).empty() );
 CHECK( gridDistances( 2 , 3 , {} , all ) ==
        std::vector< Ints >( 2 , Ints( 3 , INF ) ) );
 CHECK_THROWS( gridDistances( 2 , 2 , { GridCell( 0 , 0 ) } ,
                              []( GridCell ) { return( false ); } ) ,
               contract_violation );
 CHECK_THROWS( gridDistance( 2 , 2 , GridCell( 0 , 0 ) , GridCell( 2 , 0 ) ,
                             all ) , contract_violation );
 }

// the fill of random matrices of three colours against a recursive fill
// of a copy, for the four and the eight moves, with the cells painted
// counted; the same colour paints nothing; a cell outside
ADS_TEST( "flood-fill" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const int rows = rnd( g , 1 , 7 ) , cols = rnd( g , 1 , 7 );
  Matrix A( rows , Ints( cols ) );
  for( auto & row : A )
   for( int & x : row )
    x = rnd( g , 0 , 2 );
  Matrix B = A;
  const int r = rnd( g , 0 , rows - 1 ) , c = rnd( g , 0 , cols - 1 ) ,
            color = rnd( g , 0 , 2 ) , old = A[ r ][ c ];
  const bool eight = t % 2 == 1;
  std::size_t painted = 0;
  auto rec = [ & ]( auto & self , int i , int j ) -> void {
   if( ( i < 0 ) || ( i >= rows ) || ( j < 0 ) || ( j >= cols ) ||
       ( B[ i ][ j ] != old ) )
    return;
   B[ i ][ j ] = color;
   ++painted;
   for( int di = -1 ; di <= 1 ; ++di )
    for( int dj = -1 ; dj <= 1 ; ++dj )
     if( ( di != 0 ) || ( dj != 0 ) )
      if( eight || ( di == 0 ) || ( dj == 0 ) )
       self( self , i + di , j + dj );
   };
  if( color != old )
   rec( rec , r , c );
  const std::size_t k = eight ? floodFill( A , r , c , color , EIGHT_MOVES )
                              : floodFill( A , r , c , color );
  CHECK( ( A == B ) && ( k == painted ) );
  }
 Matrix one = { { 3 } };
 CHECK( ( floodFill( one , 0 , 0 , 3 ) == 0 ) && ( one[ 0 ][ 0 ] == 3 ) );
 CHECK( ( floodFill( one , 0 , 0 , 4 ) == 1 ) && ( one[ 0 ][ 0 ] == 4 ) );
 Matrix big( 50 , Ints( 60 , 0 ) );
 CHECK( floodFill( big , 49 , 59 , 1 ) == 3000 );
 CHECK_THROWS( floodFill( one , 1 , 0 , 5 ) , contract_violation );
 Matrix ragged = { { 0 , 0 } , { 0 } };
 CHECK_THROWS( floodFill( ragged , 0 , 0 , 1 ) , contract_violation );
 }

// the knight on every pair of squares of the boards up to 7 x 7 against a
// relaxation to the fixed point; reachable everywhere from 4 x 4 on; the
// corners of the chessboard, 6 moves apart
ADS_TEST( "knight-distance" )
{
 for( int n = 1 ; n <= 7 ; ++n )
  for( int s = 0 ; s < n * n ; ++s ) {
   Ints d( n * n , INF );
   d[ s ] = 0;
   for( bool more = true ; more ; ) {
    more = false;
    for( int u = 0 ; u < n * n ; ++u )
     if( d[ u ] != INF )
      for( const auto & [ dr , dc ] : KNIGHT_MOVES ) {
       const int a = u / n + dr , b = u % n + dc;
       if( ( 0 <= a ) && ( a < n ) && ( 0 <= b ) && ( b < n ) &&
           ( d[ u ] + 1 < d[ a * n + b ] ) ) {
        d[ a * n + b ] = d[ u ] + 1;
        more = true;
        }
       }
    }
   for( int e = 0 ; e < n * n ; ++e ) {
    const int x = knightDistance( n , GridCell( s / n , s % n ) ,
                                  GridCell( e / n , e % n ) );
    CHECK( x == d[ e ] );
    if( n >= 4 )
     CHECK( x != INF );
    }
   }
 CHECK( knightDistance( 8 , GridCell( 0 , 0 ) , GridCell( 7 , 7 ) ) == 6 );
 CHECK( knightDistance( 3 , GridCell( 1 , 1 ) , GridCell( 0 , 0 ) ) == INF );
 CHECK_THROWS( knightDistance( 0 , GridCell( 0 , 0 ) , GridCell( 0 , 0 ) ) ,
               contract_violation );
 }

// the shortest paths counted against the simple paths as long as the
// distance, on random graphs; a chain of 40 diamonds, 2^40 paths; the
// parallel edges of an ArrayGraph counted apart
ADS_TEST( "count-shortest-paths" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 1 , 7 );
  const auto G = randomGraph( g , n , 0.5 , t % 2 == 0 );
  const int s = rnd( g , 0 , n - 1 );
  const auto d = distance( G , s ).first;
  std::vector< std::uint64_t > ref( n , 0 );
  simplePaths( G , s , [ & ]( const Ints & P ) {
   if( int( P.size() ) - 1 == d[ P.back() ] )
    ++ref[ P.back() ];
   } );
  CHECK( countShortestPaths( G , s ) == ref );
  }
 ListGraph<> h( 1 + 3 * 40 , true );
 for( int i = 0 ; i < 40 ; ++i ) {
  h.insertEdge( 3 * i , 3 * i + 1 );
  h.insertEdge( 3 * i , 3 * i + 2 );
  h.insertEdge( 3 * i + 1 , 3 * i + 3 );
  h.insertEdge( 3 * i + 2 , 3 * i + 3 );
  }
 CHECK( countShortestPaths( h , 0 )[ 120 ] == ( std::uint64_t( 1 ) << 40 ) );
 const ArrayGraph<> a( 3 , { { 0 , 1 , {} } , { 0 , 1 , {} } , { 1 , 2 , {} } ,
                             { 0 , 2 , {} } } );
 CHECK( countShortestPaths( a , 0 ) ==
        std::vector< std::uint64_t >( { 1 , 2 , 1 } ) );
 CHECK( countShortestPaths( ListGraph<>( 1 ) , 0 ) ==
        std::vector< std::uint64_t >( { 1 } ) );
 CHECK_THROWS( countShortestPaths( ListGraph<>( 1 ) , 1 ) ,
               contract_violation );
 }

// the distance from a set against the least of distance() from each of
// its nodes, and the distance between two sets against the least over
// the pairs; empty sets, a common node, a long path
ADS_TEST( "multi-source-distance" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 1 , 9 );
  const auto G = randomGraph( g , n , 0.25 , t % 2 == 0 );
  std::vector< bool > A( n ) , B( n );
  Ints S;
  for( int u = 0 ; u < n ; ++u ) {
   A[ u ] = coin( g , 0.3 );
   B[ u ] = coin( g , 0.3 );
   if( A[ u ] )
    S.push_back( u );
   }
  Ints ref( n , INF );
  int between = INF;
  for( int s : S ) {
   const auto d = distance( G , s ).first;
   for( int v = 0 ; v < n ; ++v ) {
    ref[ v ] = std::min( ref[ v ] , d[ v ] );
    if( B[ v ] )
     between = std::min( between , d[ v ] );
    }
   }
  const auto [ d , p ] = multiSourceDistance( G , S );
  CHECK( d == ref );
  CHECK( setDistance( G , A , B ) == between );
  }
 ListGraph<> P( 1000 , true );
 for( int u = 0 ; u + 1 < 1000 ; ++u )
  P.insertEdge( u , u + 1 );
 std::vector< bool > a( 1000 , false ) , b( 1000 , false );
 b[ 999 ] = true;
 CHECK( setDistance( P , a , b ) == INF );
 a[ 0 ] = true;
 CHECK( setDistance( P , a , b ) == 999 );
 CHECK( setDistance( P , b , a ) == INF );
 b[ 0 ] = true;
 CHECK( setDistance( P , a , b ) == 0 );
 CHECK( multiSourceDistance( P , { 500 , 0 } ).first[ 999 ] == 499 );
 CHECK( multiSourceDistance( P , {} ).first == Ints( 1000 , INF ) );
 CHECK_THROWS( setDistance( P , a , std::vector< bool >( 3 ) ) ,
               contract_violation );
 }

// the layers of random acyclic graphs against the rounds of removal of the
// nodes with no entering edge, and against the longest of the simple paths
// ending at each node; a long chain; a cycle
ADS_TEST( "longest-path-layering" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 1 , 8 );
  const auto G = randomDag( g , n , 0.4 );
  Ints in( n , 0 ) , round( n , 0 ) , longest( n , 0 );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) )
    ++in[ v ];
  std::vector< bool > gone( n , false );
  for( int k = 0 , left = n ; left > 0 ; ++k ) {
   Ints now;
   for( int u = 0 ; u < n ; ++u )
    if( ! gone[ u ] && ( in[ u ] == 0 ) )
     now.push_back( u );
   for( int u : now ) {
    gone[ u ] = true;
    round[ u ] = k;
    --left;
    for( int v : G.adj( u ) )
     --in[ v ];
    }
   }
  for( int s = 0 ; s < n ; ++s )
   simplePaths( G , s , [ & ]( const Ints & P ) {
    longest[ P.back() ] = std::max( longest[ P.back() ] ,
                                    int( P.size() ) - 1 );
    } );
  const auto L = longestPathLayering( G );
  CHECK( ( L == round ) && ( L == longest ) );
  }
 ListGraph<> c( 3000 , true );
 for( int u = 0 ; u + 1 < 3000 ; ++u )
  c.insertEdge( u + 1 , u );
 CHECK( longestPathLayering( c )[ 0 ] == 2999 );
 CHECK( longestPathLayering( ListGraph<>( 0 ) ).empty() );
 c.insertEdge( 0 , 2999 );
 CHECK_THROWS( longestPathLayering( c ) , contract_violation );
 }

// the longest weighted paths of random acyclic graphs against the
// enumeration of the paths from each node; the downhill paths of an
// unoriented graph with altitudes; unit lengths without weights; a cycle
// among the edges kept
ADS_TEST( "dag-longest-paths" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 1 , 7 );
  const auto D = randomDag( g , n , 0.5 );
  ListGraph< int > W( n , true );
  for( int u = 0 ; u < n ; ++u )
   for( int v : D.adj( u ) )
    W.insertEdge( u , v , rnd( g , -3 , 9 ) );
  std::vector< int > ref( n , 0 ) , unit( n , 0 );
  for( int s = 0 ; s < n ; ++s )
   simplePaths( W , s , [ & ]( const Ints & P ) {
    int len = 0;
    for( std::size_t i = 1 ; i < P.size() ; ++i )
     len += *W.weight( P[ i - 1 ] , P[ i ] );
    ref[ s ] = std::max( ref[ s ] , len );
    unit[ s ] = std::max( unit[ s ] , int( P.size() ) - 1 );
    } );
  CHECK( dagLongestPaths( W ) == ref );
  CHECK( dagLongestPaths( D ) == unit );
  // downhill on an unoriented graph
  ListGraph< int > U( n , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( coin( g , 0.5 ) )
     U.insertEdge( u , v , rnd( g , 1 , 9 ) );
  Ints z( n );
  for( int & x : z )
   x = rnd( g , 0 , 3 );
  Ints down( n , 0 );
  auto rec = [ & ]( auto & self , int s , int u , int len ) -> void {
   down[ s ] = std::max( down[ s ] , len );
   U.forEachEdge( u , [ & ]( int v , int w ) {
    if( z[ v ] < z[ u ] )
     self( self , s , v , len + w );
    } );
   };
  for( int s = 0 ; s < n ; ++s )
   rec( rec , s , s , 0 );
  CHECK( dagLongestPaths( U , [ & ]( int u , int v ) {
          return( z[ v ] < z[ u ] ); } ) == down );
  }
 ListGraph< double > p( 3 , true );
 p.insertEdge( 0 , 1 , 0.5 );
 p.insertEdge( 1 , 2 , 0.25 );
 CHECK( dagLongestPaths( p ) == std::vector< double >( { 0.75 , 0.25 , 0 } ) );
 ListGraph<> e( 2 , false );
 e.insertEdge( 0 , 1 );
 CHECK_THROWS( dagLongestPaths( e ) , contract_violation );
 CHECK( dagLongestPaths( e , []( int u , int v ) { return( u < v ); } ) ==
        Ints( { 1 , 0 } ) );
 }

// the cycles through s of random digraphs against the sequences of
// distinct nodes from s closed by an edge, each found once; the complete
// digraphs, floor( e ( n - 1 )! ) - 1 cycles; an acyclic graph
ADS_TEST( "simple-cycles" )
{
 std::mt19937 g( 7 );
 auto run = []( const ListGraph<> & G , int s ) {
  std::vector< Ints > out;
  simpleCycles( G , s , [ & ]( const Ints & C ) { out.push_back( C ); } );
  return( out );
  };
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 1 , 6 ) , s = rnd( g , 0 , n - 1 );
  const auto G = randomGraph( g , n , 0.45 , true );
  std::vector< Ints > want;
  simplePaths( G , s , [ & ]( const Ints & P ) {
   if( ( P.size() >= 2 ) && G.hasEdge( P.back() , s ) ) {
    Ints C( P );
    C.push_back( s );
    want.push_back( C );
    }
   } );
  auto got = run( G , s );
  std::sort( got.begin() , got.end() );
  std::sort( want.begin() , want.end() );
  CHECK( got == want );
  }
 const long count[] = { 1 , 4 , 15 , 64 , 325 , 1956 };
 for( int n = 2 ; n <= 7 ; ++n ) {
  const auto K = randomGraph( g , n , 1.0 , true );
  auto C = run( K , n - 1 );
  CHECK( long( C.size() ) == count[ n - 2 ] );
  std::sort( C.begin() , C.end() );
  CHECK( std::adjacent_find( C.begin() , C.end() ) == C.end() );
  }
 CHECK( run( randomDag( g , 8 , 0.6 ) , 3 ).empty() );
 CHECK( run( ListGraph<>( 1 ) , 0 ).empty() );
 CHECK_THROWS( run( ListGraph<>( 2 , false ) , 0 ) , contract_violation );
 }

// the greedy stops against a visit in breadth over the points within
// reach of each other, on random trips, possible or not; no station
ADS_TEST( "refueling-stops" )
{
 std::mt19937 g( 8 );
 int impossible = 0;
 for( int t = 0 ; t < 2000 ; ++t ) {
  const int R = rnd( g , 0 , 10 ) , L = rnd( g , 0 , 60 );
  Ints D;
  for( int k = rnd( g , 0 , 12 ) ; k > 0 ; --k )
   D.push_back( rnd( g , 0 , L ) );
  std::sort( D.begin() , D.end() );
  Ints P = { 0 };                        // the start, the stations, L
  P.insert( P.end() , D.begin() , D.end() );
  P.push_back( L );
  Ints best( P.size() , INF );
  best[ 0 ] = 0;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   if( best[ i ] != INF )
    for( std::size_t j = i + 1 ; j < P.size() ; ++j )
     if( P[ j ] - P[ i ] <= R )
      best[ j ] = std::min( best[ j ] , best[ i ] + ( j + 1 < P.size() ) );
  const auto S = refuelingStops( D , L , R );
  CHECK( S.has_value() == ( best.back() != INF ) );
  if( S )
   CHECK( int( S->size() ) == best.back() );
  impossible += S ? 0 : 1;
  }
 CHECK( ( impossible > 100 ) && ( impossible < 1900 ) );
 CHECK( refuelingStops( Ints() , 4 , 4 )->empty() );
 CHECK( ! refuelingStops( Ints() , 5 , 4 ) );
 CHECK( *refuelingStops( Ints( { 0 , 5 , 10 } ) , 15 , 5 ) ==
        std::vector< std::size_t >( { 1 , 2 } ) );
 CHECK( refuelingStops( std::vector< double >( { 0.5 } ) , 1.0 , 0.5 )->size()
        == 1 );
 CHECK_THROWS( refuelingStops( Ints( { 3 , 1 } ) , 5 , 4 ) ,
               contract_violation );
 }

// random forests and graphs against one component of cc() and n - 1
// edges; the diameter against Floyd-Warshall with unit weights and the
// largest distance(); the trees by two visits against diameter(); the
// binary trees against their graphs; a path of 100000 nodes
ADS_TEST( "trees-and-diameters" )
{
 std::mt19937 g( 9 );
 int trees = 0;
 for( int t = 0 ; t < 1000 ; ++t ) {
  const int n = rnd( g , 1 , 12 );
  ListGraph<> G = t % 2 ? randomTree( g , n )
                        : randomGraph( g , n , 2.0 / n , false );
  const auto c = cc( G );
  int m = 0;
  for( int u = 0 ; u < n ; ++u )
   m += int( G.adj( u ).size() );
  const bool e = ( *std::max_element( c.begin() , c.end() ) == 1 ) &&
                 ( m == 2 * ( n - 1 ) );
  CHECK( isTree( G ) == e );
  trees += e ? 1 : 0;
  if( e )
   CHECK( treeDiameter( G ) == diameter( G ) );
  }
 CHECK( ( trees > 400 ) && ( trees < 1000 ) );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = rnd( g , 1 , 20 );
  const bool directed = t % 2 == 0;
  ListGraph< int > G( n , directed );
  for( int v = 1 ; v < n ; ++v ) {           // strongly connected
   const int u = rnd( g , 0 , v - 1 );
   G.insertEdge( u , v , 1 );
   G.insertEdge( v , u , 1 );
   }
  for( int k = rnd( g , 0 , n ) ; k > 0 ; --k ) {
   const int u = rnd( g , 0 , n - 1 ) , v = rnd( g , 0 , n - 1 );
   if( u != v )
    G.insertEdge( u , v , 1 );
   }
  const auto R = floydWarshall( G );
  int e = 0;
  for( const auto & row : R.d )
   e = std::max( e , *std::max_element( row.begin() , row.end() ) );
  CHECK( diameter( G ) == e );
  }
 for( int t = 0 ; t < 200 ; ++t ) {
  const int n = rnd( g , 1 , 30 );
  std::vector< std::unique_ptr< BinaryTree< int > > > keep;
  std::vector< BinaryTree< int > * > node;
  ListGraph<> G( n , false );
  for( int v = 0 ; v < n ; ++v ) {
   node.push_back( new BinaryTree< int >( v ) );
   bool placed = v == 0;
   while( ! placed ) {
    BinaryTree< int > * u = node[ rnd( g , 0 , v - 1 ) ];
    placed = ! u->left() || ! u->right();
    if( ! u->left() && ( u->right() || coin( g , 0.5 ) ) )
     u->insertLeft( node[ v ] );
    else if( ! u->right() )
     u->insertRight( node[ v ] );
    if( placed )
     G.insertEdge( u->read() , v );
    }
   }
  keep.emplace_back( node[ 0 ] );
  int h = 0;
  const auto d = distance( G , 0 ).first;
  for( int x : d )
   h = std::max( h , x );
  CHECK( treeDiameter( node[ 0 ] ) == std::pair( h , diameter( G ) ) );
  }
 CHECK( treeDiameter< int >( nullptr ) == std::pair( -1 , 0 ) );
 CHECK( isTree( ListGraph<>( 1 , false ) ) && ! isTree( ListGraph<>( 2 ,
                                                                   false ) ) );
 CHECK( ( diameter( ListGraph<>( 1 ) ) == 0 ) &&
        ( treeDiameter( ListGraph<>( 1 , false ) ) == 0 ) );
 CHECK_THROWS( isTree( ListGraph<>( 0 , false ) ) , contract_violation );
 CHECK_THROWS( isTree( ListGraph<>( 3 , true ) ) , contract_violation );
 CHECK_THROWS( diameter( ListGraph<>( 2 , false ) ) , contract_violation );
 CHECK_THROWS( treeDiameter( ListGraph<>( 2 , false ) ) ,
               contract_violation );
 // a path of 100000 nodes: the visits are not recursive
 const int n = 100000;
 ListGraph<> path( n , false );
 for( int u = 0 ; u + 1 < n ; ++u ) path.insertEdge( u , u + 1 );
 CHECK( isTree( path ) && treeDiameter( path ) == n - 1 );
 }

// the minimum vertex cover of random forests against all the subsets of
// the nodes; a star, a long path, a cycle
ADS_TEST( "tree-vertex-cover" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 0 , 12 );
  ListGraph<> F( n , false );
  for( int v = 1 ; v < n ; ++v )
   if( coin( g , 0.8 ) )
    F.insertEdge( rnd( g , 0 , v - 1 ) , v );
  int best = n;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   bool cover = true;
   for( int u = 0 ; u < n ; ++u )
    for( int v : F.adj( u ) )
     cover = cover && ( ( ( m >> u ) & 1u ) || ( ( m >> v ) & 1u ) );
   if( cover )
    best = std::min( best , std::popcount( m ) );
   }
  const auto C = treeVertexCover( F );
  CHECK( int( C.size() ) == best );
  CHECK( std::is_sorted( C.begin() , C.end() ) );
  }
 ListGraph<> star( 11 , false ) , path( 2001 , false );
 for( int v = 1 ; v < 11 ; ++v )
  star.insertEdge( 0 , v );
 CHECK( treeVertexCover( star ) == Ints( { 0 } ) );
 for( int v = 1 ; v <= 2000 ; ++v )
  path.insertEdge( v - 1 , v );
 CHECK( treeVertexCover( path ).size() == 1000 );
 CHECK( treeVertexCover( ListGraph<>( 1 , false ) ).empty() );
 path.insertEdge( 0 , 2000 );
 CHECK_THROWS( treeVertexCover( path ) , contract_violation );
 CHECK_THROWS( treeVertexCover( ListGraph<>( 2 , true ) ) ,
               contract_violation );
 }

// the triangles of random graphs against all the triples; the complete
// graphs, binom( n , 3 )
ADS_TEST( "count-triangles" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = rnd( g , 0 , 20 );
  const auto G = randomGraph( g , n , coin( g , 0.5 ) ? 0.3 : 0.7 , false );
  std::int64_t e = 0;
  for( int a = 0 ; a < n ; ++a )
   for( int b = a + 1 ; b < n ; ++b )
    for( int c = b + 1 ; c < n ; ++c )
     e += ( G.hasEdge( a , b ) && G.hasEdge( b , c ) && G.hasEdge( a , c ) )
          ? 1 : 0;
  CHECK( countTriangles( G ) == e );
  }
 for( int n = 0 ; n <= 40 ; n += 5 )
  CHECK( countTriangles( randomGraph( g , n , 1.0 , false ) ) ==
         ( n < 3 ? 0 : std::int64_t( binomial( n , 3 ) ) ) );
 CHECK_THROWS( countTriangles( ListGraph<>( 2 , true ) ) ,
               contract_violation );
 }

// the colourings of random graphs against all the 2^n colourings; odd and
// even cycles; no edge
ADS_TEST( "two-coloring" )
{
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int n = rnd( g , 1 , 10 );
  const auto G = randomGraph( g , n , t % 2 ? 0.2 : 0.4 , false );
  bool brute = false;
  for( unsigned m = 0 ; ( m < ( 1u << n ) ) && ! brute ; ++m ) {
   bool ok = true;
   for( int u = 0 ; u < n ; ++u )
    for( int v : G.adj( u ) )
     ok = ok && ( ( ( m >> u ) & 1u ) != ( ( m >> v ) & 1u ) );
   brute = ok;
   }
  const auto C = twoColoring( G );
  CHECK( C.has_value() == brute );
  if( C )
   for( int u = 0 ; u < n ; ++u ) {
    CHECK( ( ( *C )[ u ] == 0 ) || ( ( *C )[ u ] == 1 ) );
    for( int v : G.adj( u ) )
     CHECK( ( *C )[ u ] != ( *C )[ v ] );
    }
  }
 for( int n = 3 ; n <= 9 ; ++n ) {
  ListGraph<> c( n , false );
  for( int u = 0 ; u < n ; ++u )
   c.insertEdge( u , ( u + 1 ) % n );
  CHECK( twoColoring( c ).has_value() == ( n % 2 == 0 ) );
  }
 CHECK( twoColoring( ListGraph<>( 0 , false ) )->empty() );
 CHECK_THROWS( twoColoring( ListGraph<>( 2 , true ) ) , contract_violation );
 }

// random matrices, some with a sink planted, against the degrees of all
// the nodes; one node; the empty matrix; a ragged one
ADS_TEST( "universal-sink" )
{
 std::mt19937 g( 13 );
 int sinks = 0;
 for( int t = 0 ; t < 3000 ; ++t ) {
  const int n = rnd( g , 1 , 8 );
  std::vector< std::vector< bool > > M( n , std::vector< bool >( n ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    M[ i ][ j ] = ( i != j ) && coin( g , t % 2 ? 0.5 : 0.9 );
  if( t % 3 == 0 ) {
   const int s = rnd( g , 0 , n - 1 );
   for( int j = 0 ; j < n ; ++j ) {
    M[ s ][ j ] = false;
    if( j != s )
     M[ j ][ s ] = true;
    }
   }
  int e = NIL;
  for( int c = 0 ; c < n ; ++c ) {
   int out = 0 , in = 0;
   for( int j = 0 ; j < n ; ++j ) {
    out += M[ c ][ j ] ? 1 : 0;
    in += M[ j ][ c ] ? 1 : 0;
    }
   if( ( out == 0 ) && ( in == n - 1 ) )
    e = c;
   }
  CHECK( universalSink( M ) == e );
  sinks += e != NIL ? 1 : 0;
  }
 CHECK( ( sinks > 1000 ) && ( sinks < 2900 ) );
 CHECK( universalSink( Matrix( 1 , Ints( 1 , 0 ) ) ) == 0 );
 CHECK( universalSink( Matrix() ) == NIL );
 CHECK_THROWS( universalSink( Matrix( { { 0 , 1 } , { 0 } } ) ) ,
               contract_violation );
 }

// the order of random graphs against the count of the topological
// permutations of their nodes; a chain, a cycle, two nodes alone
ADS_TEST( "unique-topological-order" )
{
 std::mt19937 g( 14 );
 int unique = 0;
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 0 , 6 );
  const auto G = t % 4 ? randomDag( g , n , 0.7 )
                       : randomGraph( g , n , 0.3 , true );
  Ints pi( n );
  std::iota( pi.begin() , pi.end() , 0 );
  int count = 0;
  Ints last;
  do
   if( isTopologicalOrder( G , pi ) ) {
    ++count;
    last = pi;
    }
  while( std::next_permutation( pi.begin() , pi.end() ) );
  const auto O = uniqueTopologicalOrder( G );
  CHECK( O.has_value() == ( count == 1 ) );
  if( O ) {
   CHECK( *O == last );
   ++unique;
   }
  }
 CHECK( unique > 50 );
 ListGraph<> c( 4 , true );
 c.insertEdge( 2 , 0 );
 c.insertEdge( 0 , 3 );
 c.insertEdge( 3 , 1 );
 CHECK( uniqueTopologicalOrder( c ) == Ints( { 2 , 0 , 3 , 1 } ) );
 c.insertEdge( 1 , 2 );
 CHECK( ! uniqueTopologicalOrder( c ) );
 CHECK( ! uniqueTopologicalOrder( ListGraph<>( 2 , true ) ) );
 CHECK_THROWS( uniqueTopologicalOrder( ListGraph<>( 2 , false ) ) ,
               contract_violation );
 }

// the b-matchings of random bipartite graphs against all the subsets of
// their edges; all the capacities 1; no edge
ADS_TEST( "bipartite-b-matching" )
{
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const int nl = rnd( g , 0 , 4 ) , nr = rnd( g , 0 , 4 );
  std::vector< Ints > A( nl );
  std::vector< std::pair< int , int > > E;
  for( int u = 0 ; u < nl ; ++u )
   for( int v = 0 ; v < nr ; ++v )
    if( coin( g , 0.5 ) && ( E.size() < 12 ) ) {
     A[ u ].push_back( v );
     E.emplace_back( u , v );
     }
  std::vector< long long > cl( nl ) , cr( nr );
  for( long long & c : cl )
   c = t % 3 ? rnd( g , 0 , 3 ) : 1;
  for( long long & c : cr )
   c = t % 3 ? rnd( g , 0 , 3 ) : 1;
  const auto M = bipartiteBMatching( A , nr , cl , cr );
  CHECK( int( M.size() ) == bMatchingBrute( E , nl , nr , cl , cr ) );
  for( const auto & [ u , v ] : M )
   CHECK( std::find( A[ u ].begin() , A[ u ].end() , v ) != A[ u ].end() );
  }
 CHECK( bipartiteBMatching( {} , 0 , {} , {} ).empty() );
 CHECK( bipartiteBMatching( { { 0 , 1 , 2 } } , 3 , { 3 } , { 1 , 1 , 1 } )
        .size() == 3 );
 CHECK_THROWS( bipartiteBMatching( { { 1 } } , 1 , { 1 } , { 1 } ) ,
               contract_violation );
 CHECK_THROWS( bipartiteBMatching( { { 0 } } , 1 , { -1 } , { 1 } ) ,
               contract_violation );
 }

// the matchings of largest weight of random graphs against the dynamic
// programming on the subsets of the nodes; weights of any sign; a path, a
// triangle, a loop
ADS_TEST( "max-weight-matching" )
{
 std::mt19937 g( 16 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = rnd( g , 0 , 9 );
  std::vector< std::tuple< int , int , long > > E;
  Matrix W( n , Ints( n , 0 ) );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( coin( g , 0.5 ) ) {
     const int w = rnd( g , t % 2 ? -3 : 1 , 9 );
     if( coin( g , 0.5 ) )
      E.emplace_back( u , v , w );
     else
      E.emplace_back( v , u , w );
     W[ u ][ v ] = W[ v ][ u ] = std::max( W[ u ][ v ] , w );
     }
  std::vector< long > F( std::size_t( 1 ) << n , 0 );
  for( std::size_t S = 1 ; S < F.size() ; ++S ) {
   const int u = std::countr_zero( S );
   const std::size_t R = S & ( S - 1 );
   F[ S ] = F[ R ];
   for( int v = u + 1 ; v < n ; ++v )
    if( ( R >> v ) & 1u )
     F[ S ] = std::max( F[ S ] , W[ u ][ v ] +
                                 F[ R & ~( std::size_t( 1 ) << v ) ] );
   }
  const auto [ w , M ] = maxWeightMatching( n , E );
  CHECK( w == F.back() );
  long sum = 0;
  for( std::size_t e : M )
   sum += std::get< 2 >( E[ e ] );
  CHECK( ( sum == w ) && std::is_sorted( M.begin() , M.end() ) );
  }
 using E3 = std::vector< std::tuple< int , int , double > >;
 CHECK( maxWeightMatching( 4 , E3( { { 0 , 1 , 2 } , { 1 , 2 , 3 } ,
                                     { 2 , 3 , 2 } } ) ).first == 4 );
 CHECK( maxWeightMatching( 3 , E3( { { 0 , 1 , 5 } , { 1 , 2 , 5 } ,
                                     { 0 , 2 , 5 } } ) ).second.size() == 1 );
 CHECK( maxWeightMatching( 0 , E3() ).first == 0 );
 CHECK_THROWS( maxWeightMatching( 2 , E3( { { 0 , 0 , 1 } } ) ) ,
               contract_violation );
 }

// the edge-disjoint paths of random graphs, oriented or not, against the
// fewest edges whose removal separates t from s (Menger) over all the
// subsets of the edges, and the bound k; parallel edges; s = t
ADS_TEST( "edge-disjoint-paths" )
{
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = rnd( g , 2 , 6 );
  const bool directed = t % 2 == 0;
  std::vector< std::pair< int , int > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
    if( ( u != v ) && coin( g , 0.4 ) && ( E.size() < 11 ) )
     E.emplace_back( u , v );
  ListGraph<> G( n , directed );
  for( const auto & [ u , v ] : E )
   G.insertEdge( u , v );
  const int m = int( E.size() );
  int cut = m;
  for( unsigned mask = 0 ; mask < ( 1u << m ) ; ++mask ) {
   if( std::popcount( mask ) >= cut )
    continue;
   ListGraph<> H( n , directed );
   for( int i = 0 ; i < m ; ++i )
    if( ! ( ( mask >> i ) & 1u ) )
     H.insertEdge( E[ i ].first , E[ i ].second );
   if( distance( H , 0 ).first[ n - 1 ] == INF )
    cut = std::popcount( mask );
   }
  CHECK( int( edgeDisjointPaths( G , 0 , n - 1 ).size() ) == cut );
  for( long k = 0 ; k <= 3 ; ++k )
   CHECK( long( edgeDisjointPaths( G , 0 , n - 1 , k ).size() ) ==
          std::min( k , long( cut ) ) );
  }
 const ArrayGraph<> a( 3 , { { 0 , 1 , {} } , { 0 , 1 , {} } , { 1 , 2 , {} } ,
                             { 1 , 2 , {} } , { 1 , 2 , {} } } );
 CHECK( edgeDisjointPaths( a , 0 , 2 ).size() == 2 );
 CHECK( edgeDisjointPaths( ListGraph<>( 2 ) , 0 , 1 ).empty() );
 CHECK_THROWS( edgeDisjointPaths( ListGraph<>( 2 ) , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( edgeDisjointPaths( ListGraph<>( 2 ) , 0 , 1 , -1 ) ,
               contract_violation );
 }

// the decrease of a capacity on random networks, with cycles of flow
// added, against a fresh maximum flow: each of the three cases met; a
// cycle of flow cancelled by hand
ADS_TEST( "decrease-capacity" )
{
 using LMatrix = std::vector< std::vector< long > >;
 {
  LMatrix c( 5 , std::vector< long >( 5 , 0 ) ) , f = c;
  for( const auto & [ x , y ] : std::vector< std::pair< int , int > >(
        { { 0 , 1 } , { 1 , 2 } , { 1 , 3 } , { 3 , 4 } , { 4 , 1 } } ) ) {
   c[ x ][ y ] = f[ x ][ y ] = 1;
   f[ y ][ x ] = -1;
   }
  CHECK( decreaseCapacity( c , f , 0 , 2 , 1 , 3 ) ==
         CapacityDecrease::CYCLE );
  CHECK( ( value( f , 0 ) == 1 ) && ( f[ 3 ][ 4 ] == 0 ) );
  CHECK( decreaseCapacity( c , f , 0 , 2 , 1 , 2 ) == CapacityDecrease::PATH );
  CHECK( value( f , 0 ) == 0 );
  CHECK_THROWS( decreaseCapacity( c , f , 0 , 2 , 1 , 2 ) ,
                contract_violation );
 }
 std::mt19937 g( 18 );
 int cases[ 3 ] = { 0 , 0 , 0 };
 for( int t = 0 ; t < 1500 ; ++t ) {
  const int n = rnd( g , 2 , 6 );
  LMatrix c( n , std::vector< long >( n , 0 ) );
  for( int x = 0 ; x < n ; ++x )
   for( int y = 0 ; y < n ; ++y )
    if( ( x != y ) && coin( g , 0.5 ) )
     c[ x ][ y ] = rnd( g , 1 , 3 );
  const int s = rnd( g , 0 , n - 1 ) , p = ( s + rnd( g , 1 , n - 1 ) ) % n;
  LMatrix f = flowMatrix( c , s , p );
  for( int k = 0 ; k < 10 ; ++k ) {       // cycles along the residuals
   Ints C( n );
   std::iota( C.begin() , C.end() , 0 );
   std::shuffle( C.begin() , C.end() , g );
   C.resize( rnd( g , 2 , n ) );
   bool ok = true;
   for( std::size_t i = 0 ; i < C.size() ; ++i )
    ok = ok && ( c[ C[ i ] ][ C[ ( i + 1 ) % C.size() ] ] -
                 f[ C[ i ] ][ C[ ( i + 1 ) % C.size() ] ] > 0 );
   if( ok )
    for( std::size_t i = 0 ; i < C.size() ; ++i ) {
     ++f[ C[ i ] ][ C[ ( i + 1 ) % C.size() ] ];
     --f[ C[ ( i + 1 ) % C.size() ] ][ C[ i ] ];
     }
   }
  std::vector< std::pair< int , int > > E;
  for( int x = 0 ; x < n ; ++x )
   for( int y = 0 ; y < n ; ++y )
    if( c[ x ][ y ] > 0 )
     E.emplace_back( x , y );
  if( E.empty() )
   continue;
  const auto [ u , v ] = E[ rnd( g , 0 , int( E.size() ) - 1 ) ];
  const long before = value( f , s );
  const auto r = decreaseCapacity( c , f , s , p , u , v );
  ++cases[ int( r ) ];
  CHECK( value( f , s ) == value( flowMatrix( c , s , p ) , s ) );
  CHECK( r == CapacityDecrease::PATH ? value( f , s ) >= before - 1
                                     : value( f , s ) == before );
  }
 CHECK( ( cases[ 0 ] > 0 ) && ( cases[ 1 ] > 0 ) && ( cases[ 2 ] > 0 ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File lib_graphs.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
