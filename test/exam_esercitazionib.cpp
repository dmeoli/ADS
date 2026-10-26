/*--------------------------------------------------------------------------*/
/*---------------------- File exam_esercitazionib.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems of the exercise sessions on flows, graphs, greedy
 * algorithms and dynamic programming (the second half; the first is in
 * exam_esercitazionia.cpp): each one solved as in its official solution,
 * or by the function of the library that already solves it, and checked
 * on the examples of its text, against an independent oracle (a brute
 * force, the standard library, another algorithm of the library) on random
 * instances with a fixed seed, and on the edge cases.
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

#include "ADS/Contract.h"
#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/complexity/EasyCases.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/IntervalCover.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/search/Selection.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/SubsequenceProblems.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/// 64 bits everywhere: long has 32 bits on Windows
using I64 = std::int64_t;

namespace {

namespace esercitazioni {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a uniform integer in [ a , b ]
int rnd( std::mt19937 & g , int a , int b )
{
 return( std::uniform_int_distribution< int >( a , b )( g ) );
 }

/// a random vector of n integers in [ a , b ]
std::vector< int > rndVector( std::mt19937 & g , int n , int a , int b )
{
 std::vector< int > V( n );
 for( auto & v : V ) v = rnd( g , a , b );
 return( V );
 }

/// a random graph of n nodes, each edge present with probability p
ListGraph<> randomGraph( std::mt19937 & g , int n , double p ,
                         bool directed )
{
 ListGraph<> G( n , directed );
 std::bernoulli_distribution e( p );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && e( g ) )
    G.insertEdge( u , v );
 return( G );
 }

/// a point of the plane
struct Point {
 std::int64_t x , y;
 };

/// the square of the distance of p and q
std::int64_t dist2( const Point & p , const Point & q )
{
 return( ( p.x - q.x ) * ( p.x - q.x ) + ( p.y - q.y ) * ( p.y - q.y ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 34 - Ballo di fine anno -------------------------*/
/*--------------------------------------------------------------------------*/


/// the largest number of dances of n boys and m girls, each couple of C
/// dancing at most pairCap times and each student at most studentCap
/// times: a maximum flow from s through the boys (studentCap), the couples
/// (pairCap) and the girls (studentCap) to t; O( |f*| ( V + E ) ), |f*| <=
/// studentCap min( n , m ); the dances of each couple go to *dances
/// \pre the couples join [ 0 , n ) to [ 0 , m ), capacities >= 0
I64 maxDances( int n , int m ,
                const std::vector< std::pair< int , int > > & C ,
                I64 pairCap = 3 , I64 studentCap = 10 ,
                std::vector< I64 > * dances = nullptr )
{
 ADS_PRE( ( n >= 0 ) && ( m >= 0 ) && ( pairCap >= 0 ) &&
          ( studentCap >= 0 ) );
 for( const auto & [ b , f ] : C )
  ADS_PRE( ( 0 <= b ) && ( b < n ) && ( 0 <= f ) && ( f < m ) );
 const int s = n + m , t = s + 1;
 FlowNetwork N( n + m + 2 );
 for( int b = 0 ; b < n ; ++b ) N.addEdge( s , b , studentCap );
 for( int f = 0 ; f < m ; ++f ) N.addEdge( n + f , t , studentCap );
 std::vector< int > e;
 for( const auto & [ b , f ] : C )
  e.push_back( N.addEdge( b , n + f , pairCap ) );
 const I64 v = N.maxFlow( s , t );
 if( dances ) {
  dances->clear();
  for( int x : e ) dances->push_back( N.flow( x ) );
  }
 return( v );
 }

// against every choice of 0 ... pairCap dances per couple on 300 tiny
// instances, with small capacities so that both bind; the dances returned
// within the limits and of the total found; no couple, no student,
// capacities of 0, capacities beyond int, the preconditions
ADS_TEST( "esercitazioni-34" )
{
 std::mt19937 g( 34 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 1 , 3 ) , m = rnd( g , 1 , 3 ) ,
            pc = rnd( g , 1 , 3 ) , sc = rnd( g , 1 , 5 );
  std::set< std::pair< int , int > > S;
  for( int i = rnd( g , 0 , 5 ) ; i > 0 ; --i )
   S.insert( std::pair( rnd( g , 0 , n - 1 ) , rnd( g , 0 , m - 1 ) ) );
  const std::vector< std::pair< int , int > > C( S.begin() , S.end() );
  const int k = int( C.size() );
  I64 best = 0;
  std::vector< int > d( k , 0 );
  const I64 codes = std::llround( std::pow( pc + 1 , k ) );
  for( I64 code = 0 ; code < codes ; ++code ) {
   I64 c = code , tot = 0;
   std::vector< I64 > B( n , 0 ) , F( m , 0 );
   for( int i = 0 ; i < k ; ++i , c /= pc + 1 ) {
    d[ i ] = int( c % ( pc + 1 ) );
    B[ C[ i ].first ] += d[ i ];
    F[ C[ i ].second ] += d[ i ];
    tot += d[ i ];
    }
   if( ( *std::ranges::max_element( B ) <= sc ) &&
       ( *std::ranges::max_element( F ) <= sc ) )
    best = std::max( best , tot );
   }
  std::vector< I64 > D;
  CHECK( maxDances( n , m , C , pc , sc , &D ) == best );
  std::vector< I64 > B( n , 0 ) , F( m , 0 );
  for( int i = 0 ; i < k ; ++i ) {
   CHECK( ( D[ i ] >= 0 ) && ( D[ i ] <= pc ) );
   B[ C[ i ].first ] += D[ i ];
   F[ C[ i ].second ] += D[ i ];
   }
  CHECK( std::accumulate( D.begin() , D.end() , I64( 0 ) ) == best );
  CHECK( *std::ranges::max_element( B ) <= sc &&
         *std::ranges::max_element( F ) <= sc );
  }
 CHECK( maxDances( 2 , 2 , {} ) == 0 );
 CHECK( maxDances( 1 , 4 , { { 0 , 0 } , { 0 , 1 } , { 0 , 2 } , { 0 , 3 } } )
        == 10 );
 CHECK( maxDances( 0 , 0 , {} ) == 0 && maxDances( 0 , 3 , {} ) == 0 );
 CHECK( maxDances( 2 , 2 , { { 0 , 0 } , { 1 , 1 } } , 0 ) == 0 );
 CHECK( maxDances( 2 , 2 , { { 0 , 0 } , { 1 , 1 } } , 3 , 0 ) == 0 );
 const long big = 2000000000;           // FlowNetwork takes long capacities
 CHECK( maxDances( 1 , 1 , { { 0 , 0 } } , big , big ) == big );
 CHECK_THROWS( maxDances( -1 , 1 , {} ) , contract_violation );
 CHECK_THROWS( maxDances( 1 , 1 , {} , -1 ) , contract_violation );
 CHECK_THROWS( maxDances( 1 , 1 , {} , 1 , -1 ) , contract_violation );
 CHECK_THROWS( maxDances( 1 , 1 , { { 1 , 0 } } ) , contract_violation );
 CHECK_THROWS( maxDances( 1 , 1 , { { 0 , -1 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- 35 - Cammini indipendenti ------------------------*/
/*--------------------------------------------------------------------------*/


// the largest number of edge-disjoint paths from s to t: edgeDisjointPaths()
// of the library, the maximum flow with unit capacities, decomposed into as
// many paths; O( n ( n + m ) )

// the graph of the text (2 paths); against the smallest set of edges whose
// removal separates t from s (Menger), over all the subsets, on 300 random
// graphs of at most 12 edges; no edge, t unreachable, one edge from s to
// t, the preconditions (s = t, a node absent)
ADS_TEST( "esercitazioni-35" )
{
 ListGraph<> T( 6 );                          // s , b , c , d , e , t
 for( auto [ u , v ] : { std::pair( 0 , 1 ) , std::pair( 0 , 2 ) ,
                         std::pair( 1 , 2 ) , std::pair( 1 , 3 ) ,
                         std::pair( 2 , 4 ) , std::pair( 3 , 4 ) ,
                         std::pair( 3 , 5 ) , std::pair( 4 , 5 ) } )
  T.insertEdge( u , v );
 CHECK( edgeDisjointPaths( T , 0 , 5 ).size() == 2 );
 std::mt19937 g( 35 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 2 , 6 );
  std::vector< std::pair< int , int > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && ( rnd( g , 0 , 2 ) == 0 ) && ( E.size() < 12 ) )
     E.emplace_back( u , v );
  ListGraph<> G( n );
  for( auto [ u , v ] : E ) G.insertEdge( u , v );
  const int m = int( E.size() );
  int cut = m;
  for( int mask = 0 ; mask < ( 1 << m ) ; ++mask ) {
   if( std::popcount( unsigned( mask ) ) >= cut ) continue;
   ListGraph<> H( n );
   for( int i = 0 ; i < m ; ++i )
    if( ! ( ( mask >> i ) & 1 ) ) H.insertEdge( E[ i ].first , E[ i ].second );
   if( distance( H , 0 ).first[ n - 1 ] == INF )
    cut = std::popcount( unsigned( mask ) );
  }
  CHECK( int( edgeDisjointPaths( G , 0 , n - 1 ).size() ) == cut );
  }
 CHECK_THROWS( edgeDisjointPaths( T , 2 , 2 ) , contract_violation );
 CHECK_THROWS( edgeDisjointPaths( T , 0 , 6 ) , contract_violation );
 CHECK_THROWS( edgeDisjointPaths( T , -1 , 5 ) , contract_violation );
 CHECK( edgeDisjointPaths( T , 5 , 0 ).empty() );
 ListGraph<> E2( 2 );
 CHECK( edgeDisjointPaths( E2 , 0 , 1 ).empty() );
 E2.insertEdge( 0 , 1 );
 const std::vector< std::vector< int > > one( 1 , { 0 , 1 } );
 CHECK( edgeDisjointPaths( E2 , 0 , 1 ) == one );
 CHECK( edgeDisjointPaths( E2 , 1 , 0 ).empty() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- 36 - Cubi --------------------------------*/
/*--------------------------------------------------------------------------*/


/// canCompose(): whether the word can be written with t of the cubes, one
/// face of each: a matching that covers the positions of the word, by a
/// maximum flow from s to the positions (1), to the cubes holding their
/// letter (1), to t (1); O( t ( n + t + n t ) )
/// \pre each cube has 6 faces, t <= n
bool canCompose( const std::vector< std::string > & cubes ,
                 const std::string & word )
{
 const int n = int( cubes.size() ) , t = int( word.size() );
 ADS_PRE( t <= n );
 for( const auto & c : cubes ) ADS_PRE( c.size() == 6 );
 const int s = t + n , z = s + 1;
 FlowNetwork N( t + n + 2 );
 for( int i = 0 ; i < t ; ++i ) {
  N.addEdge( s , i , 1 );
  for( int j = 0 ; j < n ; ++j )
   if( cubes[ j ].find( word[ i ] ) != std::string::npos )
    N.addEdge( i , t + j , 1 );
  }
 for( int j = 0 ; j < n ; ++j ) N.addEdge( t + j , z , 1 );
 return( N.maxFlow( s , z ) == t );
 }

// the cubes of the text: IDEA cannot be written, since only ABCDEF has D
// and E, while IDA can; against every injective choice of the cubes and
// against bipartiteMatching() of the library, on 400 random instances over
// the letters A ... H; the empty word (also with no cube), a letter
// repeated on one cube counting once, a word as long as the cubes, the
// preconditions (a word longer than the cubes, a cube without 6 faces)
ADS_TEST( "esercitazioni-36" )
{
 CHECK( ! canCompose( { "ABCDEF" , "GHIJKL" , "AABBCC" , "ISTUVW" } ,
                      "IDEA" ) );
 CHECK( canCompose( { "ABCDEF" , "GHIJKL" , "AABBCC" , "ISTUVW" } , "IDA" ) );
 CHECK( ! canCompose( { "ABCDEF" , "GHIJKL" , "AABBCC" , "ISTUVW" } ,
                      "AAAA" ) );
 std::mt19937 g( 36 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const int n = rnd( g , 1 , 6 ) , t = rnd( g , 0 , n );
  std::vector< std::string > cubes( n , std::string( 6 , 'A' ) );
  for( auto & c : cubes )
   for( auto & f : c ) f = char( 'A' + rnd( g , 0 , 7 ) );
  std::string word( t , 'A' );
  for( auto & c : word ) c = char( 'A' + rnd( g , 0 , 5 ) );
  std::vector< int > P( n );
  std::iota( P.begin() , P.end() , 0 );
  bool brute = false;
  do {
   bool ok = true;
   for( int i = 0 ; ok && ( i < t ) ; ++i )
    ok = cubes[ P[ i ] ].find( word[ i ] ) != std::string::npos;
   brute = brute || ok;
   } while( ! brute && std::ranges::next_permutation( P ).found );
  std::vector< std::pair< int , int > > E;
  for( int i = 0 ; i < t ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( cubes[ j ].find( word[ i ] ) != std::string::npos )
     E.emplace_back( i , j );
  CHECK( canCompose( cubes , word ) == brute );
  CHECK( brute == ( int( bipartiteMatching( t , n , E ).size() ) == t ) );
  }
 CHECK( canCompose( { "ABCDEF" } , "" ) );
 CHECK_THROWS( canCompose( { "ABCDEF" } , "AB" ) , contract_violation );
 CHECK( canCompose( {} , "" ) );
 CHECK( ! canCompose( { "AAAAAA" , "BCDEFG" } , "AA" ) );
 CHECK( canCompose( { "AAAAAA" , "BCDEFG" } , "GA" ) );
 CHECK( canCompose( std::vector< std::string >( 26 , "ZZZZZZ" ) ,
                    std::string( 26 , 'Z' ) ) );
 CHECK_THROWS( canCompose( {} , "A" ) , contract_violation );
 CHECK_THROWS( canCompose( { "ABCDE" } , "A" ) , contract_violation );
 CHECK_THROWS( canCompose( { "ABCDEFG" } , "" ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------- 37 - Aggiornamento del flusso massimo ------------------*/
/*--------------------------------------------------------------------------*/


using Matrix = std::vector< std::vector< I64 > >;

/// a path from s to t in the residual network r, by a visit in breadth;
/// empty if there is none
std::vector< int > augmentingPath( const Matrix & r , int s , int t )
{
 const int n = int( r.size() );
 std::vector< int > p( n , -2 );
 std::queue< int > Q;
 Q.push( s );
 p[ s ] = -1;
 while( ! Q.empty() && ( p[ t ] == -2 ) ) {
  const int u = Q.front();
  Q.pop();
  for( int v = 0 ; v < n ; ++v )
   if( ( r[ u ][ v ] > 0 ) && ( p[ v ] == -2 ) ) {
    p[ v ] = u;
    Q.push( v );
    }
  }
 std::vector< int > path;
 if( p[ t ] == -2 ) return( path );
 for( int v = t ; v != -1 ; v = p[ v ] ) path.push_back( v );
 std::ranges::reverse( path );
 return( path );
 }

/// the residual network of the capacities c and the flow f
Matrix residualNetwork( const Matrix & c , const Matrix & f )
{
 Matrix r = c;
 for( std::size_t u = 0 ; u < c.size() ; ++u )
  for( std::size_t v = 0 ; v < c.size() ; ++v ) r[ u ][ v ] -= f[ u ][ v ];
 return( r );
 }

/// d more units along the path, the flow kept skew symmetric
void augmentFlow( Matrix & f , const std::vector< int > & path , I64 d )
{
 for( std::size_t i = 0 ; i + 1 < path.size() ; ++i ) {
  f[ path[ i ] ][ path[ i + 1 ] ] += d;
  f[ path[ i + 1 ] ][ path[ i ] ] -= d;
  }
 }

/// a maximum flow from s to t, skew symmetric, by augmenting paths
Matrix maxFlowMatrix( const Matrix & c , int s , int t )
{
 Matrix f( c.size() , std::vector< I64 >( c.size() , 0 ) );
 for( auto P = augmentingPath( c , s , t ) ; ! P.empty() ;
      P = augmentingPath( residualNetwork( c , f ) , s , t ) ) {
  const auto r = residualNetwork( c , f );
  I64 d = std::numeric_limits< I64 >::max();
  for( std::size_t i = 0 ; i + 1 < P.size() ; ++i )
   d = std::min( d , r[ P[ i ] ][ P[ i + 1 ] ] );
  augmentFlow( f , P , d );
  }
 return( f );
 }

/// the value of the flow f out of s
I64 flowValue( const Matrix & f , int s )
{
 return( std::accumulate( f[ s ].begin() , f[ s ].end() , I64( 0 ) ) );
 }

/// updateFlow(): the maximum flow f, skew symmetric, of the capacities c
/// updated after c[ u ][ v ] grows by 1: the optimum grows by at most 1,
/// so one augmenting path in the new residual network is enough;
/// O( n^2 ) with the matrices
/// \pre s, t, u, v nodes, s != t, c >= 0, f a maximum flow of c
void updateFlow( Matrix & c , int s , int t , Matrix & f , int u , int v )
{
 const int n = int( c.size() );
 ADS_PRE( ( 0 <= std::min( { s , t , u , v } ) ) &&
          ( std::max( { s , t , u , v } ) < n ) && ( s != t ) );
 ADS_AUDIT( augmentingPath( residualNetwork( c , f ) , s , t ).empty() );
 ++c[ u ][ v ];
 const auto path = augmentingPath( residualNetwork( c , f ) , s , t );
 if( ! path.empty() ) augmentFlow( f , path , 1 );
 }

// on 300 random networks: the flow updated after one capacity grows is a
// flow (capacities, skew symmetry, conservation) of the value that the
// maximum flow of the library finds from scratch, at most 1 more; an edge
// of a minimum cut grows it by 1, an edge out of every cut does not, nor
// an edge back from t to s; the preconditions (s = t, a node absent)
ADS_TEST( "esercitazioni-37" )
{
 std::mt19937 g( 37 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 2 , 7 ) , s = 0 , t = n - 1;
  Matrix c( n , std::vector< I64 >( n , 0 ) );
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b )
    if( ( a != b ) && rnd( g , 0 , 1 ) ) c[ a ][ b ] = rnd( g , 0 , 4 );
  Matrix f = maxFlowMatrix( c , s , t );
  const I64 before = flowValue( f , s );
  int u = rnd( g , 0 , n - 1 ) , v = rnd( g , 0 , n - 2 );
  if( v >= u ) ++v;
  updateFlow( c , s , t , f , u , v );
  FlowNetwork N( n );
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b )
    if( c[ a ][ b ] ) N.addEdge( a , b , c[ a ][ b ] );
  const I64 after = N.maxFlow( s , t );
  CHECK( flowValue( f , s ) == after && after - before <= 1 &&
         after >= before );
  for( int a = 0 ; a < n ; ++a ) {
   I64 net = 0;
   for( int b = 0 ; b < n ; ++b ) {
    CHECK( f[ a ][ b ] <= c[ a ][ b ] && f[ a ][ b ] == -f[ b ][ a ] );
    net += f[ a ][ b ];
    }
   CHECK( ( a == s ) || ( a == t ) || ( net == 0 ) );
   }
  }
 Matrix c{ { 0 , 1 , 0 } , { 0 , 0 , 1 } , { 0 , 0 , 0 } };
 Matrix f = maxFlowMatrix( c , 0 , 2 );
 updateFlow( c , 0 , 2 , f , 0 , 1 );
 CHECK( flowValue( f , 0 ) == 1 );
 updateFlow( c , 0 , 2 , f , 1 , 2 );
 CHECK( flowValue( f , 0 ) == 2 );
 updateFlow( c , 0 , 2 , f , 2 , 0 );
 CHECK( flowValue( f , 0 ) == 2 && c[ 2 ][ 0 ] == 1 );
 Matrix z{ { 0 , 0 } , { 0 , 0 } } , fz = maxFlowMatrix( z , 0 , 1 );
 CHECK( flowValue( fz , 0 ) == 0 );
 updateFlow( z , 0 , 1 , fz , 0 , 1 );
 CHECK( flowValue( fz , 0 ) == 1 );
 CHECK_THROWS( updateFlow( c , 1 , 1 , f , 0 , 1 ) , contract_violation );
 CHECK_THROWS( updateFlow( c , 0 , 3 , f , 0 , 1 ) , contract_violation );
 CHECK_THROWS( updateFlow( c , 0 , 2 , f , -1 , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- 38 - McDonald ------------------------------*/
/*--------------------------------------------------------------------------*/


/// a staffing problem: days x shifts shifts, the staff p[ i ] each needs,
/// the shifts in which each employee cannot work, the most shifts of an
/// employee in a week and in a day
struct Staffing {
 int days = 7 , shifts = 3;
 std::vector< int > p;
 std::vector< std::set< int > > off;
 int week = 5 , day = 2;
 };

/// the employees of each shift, or nothing if there is no schedule: a
/// maximum flow from s to the employees (week), to their days (day), to
/// the shifts of the day in which they can work (1), to t (p[ i ]); a
/// schedule exists iff it saturates sum p; O( |D|^2 ) for 21 shifts
/// \pre p has days x shifts places, all >= 0
std::optional< std::vector< std::vector< int > > > schedule(
                                                       const Staffing & P )
{
 const int T = P.days * P.shifts , D = int( P.off.size() );
 ADS_PRE( ( int( P.p.size() ) == T ) &&
          std::ranges::all_of( P.p , []( int x ) { return( x >= 0 ); } ) );
 const int s = D + D * P.days + T , t = s + 1;
 FlowNetwork N( t + 1 );
 std::vector< std::array< int , 3 > > E;                // edge , j , shift
 for( int j = 0 ; j < D ; ++j ) {
  N.addEdge( s , j , P.week );
  for( int d = 0 ; d < P.days ; ++d ) {
   const int w = D + j * P.days + d;
   N.addEdge( j , w , P.day );
   for( int h = 0 ; h < P.shifts ; ++h ) {
    const int i = d * P.shifts + h;
    if( ! P.off[ j ].contains( i ) )
     E.push_back( { N.addEdge( w , D + D * P.days + i , 1 ) , j , i } );
    }
   }
  }
 for( int i = 0 ; i < T ; ++i ) N.addEdge( D + D * P.days + i , t , P.p[ i ] );
 if( N.maxFlow( s , t ) < std::accumulate( P.p.begin() , P.p.end() ,
                                           I64( 0 ) ) )
  return( std::nullopt );
 std::vector< std::vector< int > > out( T );
 for( const auto & [ e , j , i ] : E )
  if( N.flow( e ) ) out[ i ].push_back( j );
 return( out );
 }

/// whether the schedule meets every rule of P
bool validSchedule( const Staffing & P ,
                    const std::vector< std::vector< int > > & S )
{
 const int D = int( P.off.size() );
 std::vector< int > week( D , 0 );
 std::vector< std::vector< int > > day( D , std::vector< int >( P.days , 0 ) );
 for( int i = 0 ; i < int( S.size() ) ; ++i ) {
  if( int( S[ i ].size() ) != P.p[ i ] ||
      ( std::set< int >( S[ i ].begin() , S[ i ].end() ).size() !=
        S[ i ].size() ) )
   return( false );
  for( int j : S[ i ] ) {
   if( P.off[ j ].contains( i ) ) return( false );
   ++week[ j ];
   ++day[ j ][ i / P.shifts ];
   }
  }
 for( int j = 0 ; j < D ; ++j ) {
  if( week[ j ] > P.week ) return( false );
  for( int d = 0 ; d < P.days ; ++d )
   if( day[ j ][ d ] > P.day ) return( false );
  }
 return( true );
 }

// against every choice of the shifts of each employee on 200 tiny
// problems (2 days of 3 shifts, 1 to 3 employees, small limits); each
// schedule found valid; a week of 21 shifts that needs 5 |D| + 1 staff; no
// employee (feasible only if nobody is needed), an employee off in every
// shift, a limit of 0 a day, the preconditions (p of the wrong size or
// negative)
ADS_TEST( "esercitazioni-38" )
{
 std::mt19937 g( 38 );
 for( int r = 0 ; r < 200 ; ++r ) {
  Staffing P;
  P.days = 2;
  P.week = rnd( g , 1 , 4 );
  P.day = rnd( g , 1 , 2 );
  const int D = rnd( g , 1 , 3 ) , T = 6;
  P.p = rndVector( g , T , 0 , 2 );
  P.off.resize( D );
  for( auto & o : P.off )
   for( int i = 0 ; i < T ; ++i ) if( rnd( g , 0 , 3 ) == 0 ) o.insert( i );
  bool brute = false;
  for( I64 code = 0 ; ! brute && ( code < ( I64( 1 ) << ( T * D ) ) ) ;
       ++code ) {
   std::vector< int > need( P.p );
   bool ok = true;
   for( int j = 0 ; ok && ( j < D ) ; ++j ) {
    const int mask = int( ( code >> ( T * j ) ) & 63 );
    ok = std::popcount( unsigned( mask ) ) <= P.week;
    for( int d = 0 ; ok && ( d < 2 ) ; ++d )
     ok = std::popcount( unsigned( ( mask >> ( 3 * d ) ) & 7 ) ) <= P.day;
    for( int i = 0 ; ok && ( i < T ) ; ++i )
     if( ( mask >> i ) & 1 ) {
      ok = ! P.off[ j ].contains( i );
      --need[ i ];
      }
    }
   brute = ok && std::ranges::all_of( need , []( int x ) {
                                       return( x <= 0 ); } );
   }
  const auto S = schedule( P );
  CHECK( S.has_value() == brute );
  if( S ) CHECK( validSchedule( P , *S ) );
  }
 Staffing W;
 W.off.resize( 4 );
 W.p.assign( 21 , 0 );
 for( int i = 0 ; i < 20 ; ++i ) W.p[ i ] = 1;
 const auto S = schedule( W );
 CHECK( S && validSchedule( W , *S ) );
 W.p[ 20 ] = 1;
 CHECK( ! schedule( W ) );
 Staffing Z;
 Z.p.assign( 21 , 0 );
 const auto SZ = schedule( Z );
 CHECK( SZ && SZ->size() == 21 && validSchedule( Z , *SZ ) );
 Z.p[ 3 ] = 1;
 CHECK( ! schedule( Z ) );
 Z.off.resize( 1 );
 CHECK( schedule( Z ) && validSchedule( Z , *schedule( Z ) ) );
 for( int i = 0 ; i < 21 ; ++i ) Z.off[ 0 ].insert( i );
 CHECK( ! schedule( Z ) );
 Z.off[ 0 ].clear();
 Z.day = 0;
 CHECK( ! schedule( Z ) );
 Z.p.pop_back();
 CHECK_THROWS( schedule( Z ) , contract_violation );
 Z.p.assign( 21 , 0 );
 Z.p[ 0 ] = -1;
 CHECK_THROWS( schedule( Z ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ 39 - Piscina ------------------------------*/
/*--------------------------------------------------------------------------*/


/// the swimming courses: days x hours slots, the largest course, the most
/// courses of a child in a day, and for each child the most courses in a
/// week and the slots it prefers, for each teacher the slots it can teach
struct Pool {
 int days = 7 , hours = 10 , course = 6 , perDay = 2;
 std::vector< int > most;
 std::vector< std::set< int > > prefs , teachers;
 };

/// an assignment of largest revenue: the slots of each child and the
/// teacher of each slot (-1 if the course is empty)
struct PoolPlan {
 I64 revenue = 0;
 std::vector< std::vector< int > > slots;
 std::vector< int > teacher;
 };

/// the plan of largest revenue: the slots without a teacher are dropped,
/// then a maximum flow from s to the children (their most), to the child
/// and day (perDay), to the preferred slots of the day (1), to t (course);
/// 10 euro per unit of flow, any available teacher for each course not
/// empty; O( b + k ) to build and O( b^2 ) for Ford-Fulkerson
/// \pre the slots in [ 0 , days hours ), the limits >= 0
PoolPlan swimming( const Pool & P )
{
 const int S = P.days * P.hours , b = int( P.most.size() );
 ADS_PRE( int( P.prefs.size() ) == b );
 std::vector< int > teach( S , -1 );
 for( int k = 0 ; k < int( P.teachers.size() ) ; ++k )
  for( int x : P.teachers[ k ] ) {
   ADS_PRE( ( 0 <= x ) && ( x < S ) );
   if( teach[ x ] < 0 ) teach[ x ] = k;
   }
 const int s = b + b * P.days + S , t = s + 1;
 FlowNetwork N( t + 1 );
 std::vector< std::array< int , 3 > > E;                // edge , i , slot
 for( int i = 0 ; i < b ; ++i ) {
  N.addEdge( s , i , P.most[ i ] );
  for( int d = 0 ; d < P.days ; ++d )
   N.addEdge( i , b + i * P.days + d , P.perDay );
  for( int x : P.prefs[ i ] ) {
   ADS_PRE( ( 0 <= x ) && ( x < S ) );
   if( teach[ x ] >= 0 )
    E.push_back( { N.addEdge( b + i * P.days + x / P.hours ,
                              b + b * P.days + x , 1 ) , i , x } );
   }
  }
 for( int x = 0 ; x < S ; ++x )
  if( teach[ x ] >= 0 ) N.addEdge( b + b * P.days + x , t , P.course );
 PoolPlan plan;
 plan.revenue = 10 * N.maxFlow( s , t );
 plan.slots.resize( b );
 plan.teacher.assign( S , -1 );
 for( const auto & [ e , i , x ] : E )
  if( N.flow( e ) ) {
   plan.slots[ i ].push_back( x );
   plan.teacher[ x ] = teach[ x ];
   }
 return( plan );
 }

// against every choice of the slots of each child on 200 tiny pools (2 days
// of 2 hours, courses of at most 2, up to 3 children); each plan checked on
// every rule; no teacher at all gives 0, so do no child, courses of 0 and
// a child of most 0; the preconditions (prefs of the wrong size, a slot
// out of range)
ADS_TEST( "esercitazioni-39" )
{
 std::mt19937 g( 39 );
 for( int r = 0 ; r < 200 ; ++r ) {
  Pool P;
  P.days = 2;
  P.hours = 2;
  P.course = rnd( g , 1 , 2 );
  P.perDay = rnd( g , 1 , 2 );
  const int b = rnd( g , 1 , 3 ) , S = 4;
  P.most = rndVector( g , b , 0 , 3 );
  P.prefs.resize( b );
  P.teachers.resize( rnd( g , 0 , 2 ) );
  for( auto & q : P.prefs ) for( int x = 0 ; x < S ; ++x )
   if( rnd( g , 0 , 2 ) ) q.insert( x );
  for( auto & q : P.teachers ) for( int x = 0 ; x < S ; ++x )
   if( rnd( g , 0 , 1 ) ) q.insert( x );
  std::vector< bool > taught( S , false );
  for( const auto & q : P.teachers ) for( int x : q ) taught[ x ] = true;
  I64 best = 0;
  for( int code = 0 ; code < ( 1 << ( S * b ) ) ; ++code ) {
   std::vector< int > size( S , 0 );
   bool ok = true;
   I64 tot = 0;
   for( int i = 0 ; ok && ( i < b ) ; ++i ) {
    const int mask = ( code >> ( S * i ) ) & 15;
    ok = std::popcount( unsigned( mask ) ) <= P.most[ i ] &&
         std::popcount( unsigned( mask & 3 ) ) <= P.perDay &&
         std::popcount( unsigned( mask >> 2 ) ) <= P.perDay;
    for( int x = 0 ; ok && ( x < S ) ; ++x )
     if( ( mask >> x ) & 1 ) {
      ok = P.prefs[ i ].contains( x ) && taught[ x ] &&
           ( ++size[ x ] <= P.course );
      ++tot;
      }
    }
   if( ok ) best = std::max( best , 10 * tot );
   }
  const auto plan = swimming( P );
  CHECK( plan.revenue == best );
  std::vector< int > size( S , 0 );
  I64 tot = 0;
  for( int i = 0 ; i < b ; ++i ) {
   CHECK( int( plan.slots[ i ].size() ) <= P.most[ i ] );
   std::vector< int > perDay( P.days , 0 );
   for( int x : plan.slots[ i ] ) {
    CHECK( P.prefs[ i ].contains( x ) && plan.teacher[ x ] >= 0 &&
           P.teachers[ plan.teacher[ x ] ].contains( x ) );
    CHECK( ++perDay[ x / P.hours ] <= P.perDay );
    ++size[ x ];
    ++tot;
    }
   }
  CHECK( 10 * tot == plan.revenue );
  for( int x = 0 ; x < S ; ++x ) CHECK( size[ x ] <= P.course );
  }
 Pool none;
 none.most = { 5 };
 none.prefs = { { 0 , 1 , 2 } };
 CHECK( swimming( none ).revenue == 0 );
 Pool empty;
 empty.teachers = { { 0 , 1 } };
 CHECK( swimming( empty ).revenue == 0 );
 Pool one = none;
 one.teachers = { { 0 , 1 , 2 } };
 CHECK( swimming( one ).revenue == 20 );          // perDay 2 on day 0
 one.course = 0;
 CHECK( swimming( one ).revenue == 0 );
 one.course = 6;
 one.most = { 0 };
 CHECK( swimming( one ).revenue == 0 );
 one.prefs.clear();
 CHECK_THROWS( swimming( one ) , contract_violation );
 one.prefs = { { 70 } };
 CHECK_THROWS( swimming( one ) , contract_violation );
 one.prefs = { { 0 } };
 one.teachers = { { -1 } };
 CHECK_THROWS( swimming( one ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 40 - Torri di controllo -------------------------*/
/*--------------------------------------------------------------------------*/


/// the tower of each plane, at most L planes per tower, each within
/// distance d of its tower, or nothing if impossible: a maximum flow from s
/// to the planes (1), to the towers within reach (1), to t (L); an
/// assignment exists iff the flow is n; O( n^2 m )
/// \pre L >= 0, d >= 0
std::optional< std::vector< int > > towers( const std::vector< Point > & A ,
                                            const std::vector< Point > & Q ,
                                            I64 L , std::int64_t d )
{
 ADS_PRE( ( L >= 0 ) && ( d >= 0 ) );
 const int n = int( A.size() ) , m = int( Q.size() ) , s = n + m , t = s + 1;
 FlowNetwork N( n + m + 2 );
 std::vector< std::array< int , 3 > > E;                // edge , plane , tower
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , i , 1 );
  for( int j = 0 ; j < m ; ++j )
   if( dist2( A[ i ] , Q[ j ] ) <= d * d )
    E.push_back( { N.addEdge( i , n + j , 1 ) , i , j } );
  }
 for( int j = 0 ; j < m ; ++j ) N.addEdge( n + j , t , L );
 if( N.maxFlow( s , t ) < n ) return( std::nullopt );
 std::vector< int > tower( n , -1 );
 for( const auto & [ e , i , j ] : E ) if( N.flow( e ) ) tower[ i ] = j;
 return( tower );
 }

// against every assignment of the planes to the towers on 300 tiny
// instances; each assignment found checked on load and distance; no plane,
// no tower, a distance of 0, a load of 0, the preconditions
ADS_TEST( "esercitazioni-40" )
{
 std::mt19937 g( 40 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 1 , 6 ) , m = rnd( g , 1 , 3 ) ,
            L = rnd( g , 1 , 3 ) ,
            d = rnd( g , 1 , 6 );
  std::vector< Point > A( n ) , Q( m );
  for( auto & p : A ) p = Point{ rnd( g , 0 , 8 ) , rnd( g , 0 , 8 ) };
  for( auto & q : Q ) q = Point{ rnd( g , 0 , 8 ) , rnd( g , 0 , 8 ) };
  bool brute = false;
  for( I64 code = 0 ; ! brute && ( code < std::llround( std::pow( m , n ) ) ) ;
       ++code ) {
   std::vector< int > load( m , 0 );
   bool ok = true;
   I64 c = code;
   for( int i = 0 ; ok && ( i < n ) ; ++i , c /= m ) {
    const int j = int( c % m );
    ok = ( dist2( A[ i ] , Q[ j ] ) <= std::int64_t( d ) * d ) &&
         ( ++load[ j ] <= L );
    }
   brute = ok;
   }
  const auto T = towers( A , Q , L , d );
  CHECK( T.has_value() == brute );
  if( T ) {
   std::vector< int > load( m , 0 );
   for( int i = 0 ; i < n ; ++i ) {
    const int j = ( *T )[ i ];
    CHECK( ( j >= 0 ) &&
           ( dist2( A[ i ] , Q[ j ] ) <= std::int64_t( d ) * d ) &&
           ( ++load[ j ] <= L ) );
    }
   }
  }
 CHECK( towers( {} , { Point{ 0 , 0 } } , 1 , 1 ) );
 CHECK( towers( {} , {} , 0 , 0 ) && ! towers( { Point{ 0 , 0 } } , {} , 1 ,
                                                5 ) );
 CHECK( towers( { Point{ 2 , 3 } } , { Point{ 2 , 3 } } , 1 , 0 ) );
 CHECK( ! towers( { Point{ 2 , 3 } } , { Point{ 2 , 4 } } , 1 , 0 ) );
 CHECK( towers( { Point{ 0 , 0 } } , { Point{ 3 , 4 } } , 1 , 5 ) &&
        ! towers( { Point{ 0 , 0 } } , { Point{ 3 , 4 } } , 1 , 4 ) );
 CHECK( ! towers( { Point{ 0 , 0 } } , { Point{ 0 , 0 } } , 0 , 1 ) );
 CHECK_THROWS( towers( {} , {} , -1 , 0 ) , contract_violation );
 CHECK_THROWS( towers( {} , {} , 0 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------- 41 - Grafi, grafi bipartiti (also 44, Good vs bad guys) ---------*/
/*--------------------------------------------------------------------------*/


// twoColor(), a coloring 0 / 1 of the unoriented graph with the ends of
// each edge apart, or nothing if there is none (an odd cycle), is
// twoColoring() of the library on a graph, by a visit from each node not
// colored yet; O( n + m )

/// canPartition(): whether the n >= 2 wrestlers split into two non-empty
/// groups with no rivalry inside a group: the graph of the rivalries is
/// 2-colorable (with n >= 2 both groups can be made non-empty); O( n + m )
/// \pre n >= 2, the rivals distinct in [ 0 , n )
bool canPartition( int n , const std::vector< std::pair< int , int > > & R )
{
 ADS_PRE( n >= 2 );
 ListGraph<> G( n , false );
 for( const auto & [ a , b ] : R ) {
  ADS_PRE( ( 0 <= std::min( a , b ) ) && ( std::max( a , b ) < n ) &&
           ( a != b ) );
  G.insertEdge( a , b );
  }
 return( twoColoring( G ).has_value() );
 }

// against twoColoring() of the library and the 2^n colorings on 500 random
// graphs, each coloring found checked; the wrestlers against every split
// into two non-empty groups; a triangle, an even cycle, no edge, two
// rivals, a long odd cycle, the preconditions (one wrestler, a rival of
// himself or out of range)
ADS_TEST( "esercitazioni-41-2" )
{
 std::mt19937 g( 41 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 2 , 9 );
  const auto G = randomGraph( g , n , r % 2 ? 0.2 : 0.4 , false );
  Adjacency M( n , std::vector< bool >( n , false ) );
  std::vector< std::pair< int , int > > R;
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) ) {
    M[ u ][ v ] = true;
    if( u < v ) R.emplace_back( u , v );
    }
  bool brute = false;
  for( int mask = 1 ; mask + 1 < ( 1 << n ) ; ++mask ) {
   bool ok = true;
   for( const auto & [ a , b ] : R )
    ok = ok && ( ( ( mask >> a ) & 1 ) != ( ( mask >> b ) & 1 ) );
   brute = brute || ok;
   }
  const auto C = twoColoring( G );
  CHECK( C.has_value() == brute && twoColoring( M ).has_value() == brute );
  if( C )
   for( const auto & [ a , b ] : R ) CHECK( ( *C )[ a ] != ( *C )[ b ] );
  CHECK( canPartition( n , R ) == brute );
  }
 CHECK( ! canPartition( 3 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } } ) );
 CHECK( canPartition( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                           { 3 , 0 } } ) );
 CHECK( canPartition( 2 , {} ) && canPartition( 2 , { { 0 , 1 } } ) );
 std::vector< std::pair< int , int > > odd , even;
 for( int i = 0 ; i < 1001 ; ++i ) odd.emplace_back( i , ( i + 1 ) % 1001 );
 for( int i = 0 ; i < 1000 ; ++i ) even.emplace_back( i , ( i + 1 ) % 1000 );
 CHECK( ! canPartition( 1001 , odd ) && canPartition( 1000 , even ) );
 CHECK_THROWS( canPartition( 1 , {} ) , contract_violation );
 CHECK_THROWS( canPartition( 3 , { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( canPartition( 3 , { { 0 , 3 } } ) , contract_violation );
 CHECK_THROWS( canPartition( 3 , { { -1 , 0 } } ) , contract_violation );
 CHECK_THROWS( twoColoring( ListGraph<>( 2 , true ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 42 - Grafi, connetti il grafo ----------------------*/
/*--------------------------------------------------------------------------*/


/// numberToConnect(): the fewest edges that make the unoriented graph
/// connected, its components less one, by the labelling of the
/// components of the library; O( n + m )
/// \pre g unoriented, not empty
template< Graph G >
int numberToConnect( const G & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) );
 const auto C = cc( g );
 return( *std::ranges::max_element( C ) - 1 );
 }

/// the root of x in the union-find forest p
int findRoot( std::vector< int > & p , int x )
{
 return( p[ x ] == x ? x : p[ x ] = findRoot( p , p[ x ] ) );
 }

/// the components of the unoriented graph by union-find
template< Graph G >
int components( const G & g )
{
 std::vector< int > p( g.bound() );
 std::iota( p.begin() , p.end() , 0 );
 int c = g.n();
 for( int u : g.V() )
  for( int v : g.adj( u ) )
   if( findRoot( p , u ) != findRoot( p , v ) ) {
    p[ findRoot( p , u ) ] = findRoot( p , v );
    --c;
    }
 return( c );
 }

// against the components counted by union-find on 500 random graphs; one
// node; no edge (n - 1); a complete graph (0); a deleted node, which is
// not a component; the preconditions (no node, oriented)
ADS_TEST( "esercitazioni-42-1" )
{
 std::mt19937 g( 421 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto G = randomGraph( g , rnd( g , 1 , 30 ) , 0.07 , false );
  CHECK( numberToConnect( G ) == components( G ) - 1 );
  }
 CHECK( numberToConnect( ListGraph<>( 1 , false ) ) == 0 );
 CHECK( numberToConnect( ListGraph<>( 9 , false ) ) == 8 );
 CHECK( numberToConnect( randomGraph( g , 7 , 1.0 , false ) ) == 0 );
 ListGraph<> D( 4 , false );
 D.insertEdge( 0 , 1 );
 D.insertEdge( 1 , 2 );
 D.deleteNode( 1 );
 CHECK( numberToConnect( D ) == components( D ) - 1 &&
        numberToConnect( D ) == 2 );
 CHECK_THROWS( numberToConnect( ListGraph<>( 0 , false ) ) ,
               contract_violation );
 CHECK_THROWS( numberToConnect( ListGraph<>( 3 , true ) ) ,
               contract_violation );
 }

/// connectGraph(): makes the unoriented graph connected with the fewest
/// edges, joining one node of each component (the first one met by the
/// labelling) to that of the next component; O( n + m ); the edges added
/// \pre g unoriented, not empty
/// \post g connected
template< DynamicGraph G >
int connectGraph( G & g )
{
 ADS_PRE( ! g.directed() && ( g.n() > 0 ) );
 const auto C = cc( g );
 std::vector< int > rep( *std::ranges::max_element( C ) + 1 , -1 );
 for( int u : g.V() ) if( rep[ C[ u ] ] < 0 ) rep[ C[ u ] ] = u;
 for( std::size_t k = 2 ; k < rep.size() ; ++k )
  g.insertEdge( rep[ k - 1 ] , rep[ k ] );
 ADS_POST( components( g ) == 1 );
 return( int( rep.size() ) - 2 );
 }

// on 500 random graphs: connected afterwards, with as many new edges as
// components less one; a connected graph gets no edge, one node neither,
// no edge n - 1 of them, a deleted node none; the preconditions
ADS_TEST( "esercitazioni-42-2" )
{
 std::mt19937 g( 422 );
 for( int r = 0 ; r < 500 ; ++r ) {
  auto G = randomGraph( g , rnd( g , 1 , 30 ) , 0.07 , false );
  const int c = components( G );
  int m = 0;
  for( int u : G.V() ) m += int( G.adj( u ).size() );
  m /= 2;
  CHECK( connectGraph( G ) == c - 1 && components( G ) == 1 );
  int m2 = 0;
  for( int u : G.V() ) m2 += int( G.adj( u ).size() );
  CHECK( m2 / 2 == m + c - 1 );
  }
 ListGraph<> K = randomGraph( g , 5 , 1.0 , false );
 CHECK( connectGraph( K ) == 0 );
 ListGraph<> one( 1 , false ) , bare( 6 , false ) , D( 4 , false );
 CHECK( connectGraph( one ) == 0 );
 CHECK( connectGraph( bare ) == 5 && components( bare ) == 1 );
 D.insertEdge( 0 , 1 );
 D.deleteNode( 2 );
 CHECK( connectGraph( D ) == 1 && components( D ) == 1 &&
        ! D.hasNode( 2 ) );
 ListGraph<> none( 0 , false ) , arcs( 2 , true );
 CHECK_THROWS( connectGraph( none ) , contract_violation );
 CHECK_THROWS( connectGraph( arcs ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 43 - Grafi, distanza fra partizioni -------------------*/
/*--------------------------------------------------------------------------*/


// mindist(), the fewest edges from a node of V1 to one of V2, 0 if they
// meet, INF if V2 is not reachable, is setDistance() of the library: a
// visit in breadth from all of V1 at once, stopped at the first node of V2;
// O( n + m )

// against the least distance() of the library from each node of V1, on 500
// random oriented and unoriented graphs; V1 meeting V2; V1 empty (INF), V2
// empty (INF), one node in both (0), the precondition on the sizes
ADS_TEST( "esercitazioni-43" )
{
 std::mt19937 g( 43 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 20 );
  const auto G = randomGraph( g , n , 0.12 , r % 2 );
  std::vector< bool > V1( n ) , V2( n );
  for( int u = 0 ; u < n ; ++u ) {
   const int k = rnd( g , 0 , 5 );
   V1[ u ] = k == 0;
   V2[ u ] = ( k == 1 ) || ( ( k == 2 ) && ( r % 3 == 0 ) );
   if( ( k == 2 ) && ( r % 3 == 0 ) ) V1[ u ] = true;
   }
  int best = INF;
  for( int u = 0 ; u < n ; ++u )
   if( V1[ u ] ) {
    const auto d = distance( G , u ).first;
    for( int v = 0 ; v < n ; ++v )
     if( V2[ v ] ) best = std::min( best , d[ v ] );
    }
  CHECK( setDistance( G , V1 , V2 ) == best );
  }
 ListGraph<> P( 3 );
 P.insertEdge( 0 , 1 );
 P.insertEdge( 1 , 2 );
 CHECK( setDistance( P , { true , false , false } ,
                     { false , false , true } ) == 2 );
 CHECK( setDistance( P , { false , false , true } ,
                     { true , false , false } ) == INF );
 CHECK( setDistance( P , { false , false , false } ,
                     { true , true , true } ) == INF );
 CHECK( setDistance( P , { true , true , true } ,
                     { false , false , false } ) == INF );
 CHECK( setDistance( ListGraph<>( 1 ) , { true } , { true } ) == 0 );
 CHECK( setDistance( P , { false , true , false } ,
                     { false , true , false } ) == 0 );
 CHECK_THROWS( setDistance( P , { true } , { false , false , true } ) ,
               contract_violation );
 CHECK_THROWS( setDistance( P , { true , false , false } , { true } ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- 45 - Griglia quadrata --------------------------*/
/*--------------------------------------------------------------------------*/


/// grid(): the fewest steps between neighbours by side from a cell of
/// color 1 to one of color 3, by a visit in breadth from all the 1 cells
/// at once; O( n^2 )
/// \pre M square with colors in { 1 , 2 , 3 }, a 1 and a 3 in it
int grid( const std::vector< std::vector< int > > & M )
{
 const int n = int( M.size() );
 std::vector< std::vector< int > > d( n , std::vector< int >( n , -1 ) );
 std::queue< std::pair< int , int > > Q;
 bool three = false;
 for( int i = 0 ; i < n ; ++i ) {
  ADS_PRE( int( M[ i ].size() ) == n );
  for( int j = 0 ; j < n ; ++j ) {
   ADS_PRE( ( 1 <= M[ i ][ j ] ) && ( M[ i ][ j ] <= 3 ) );
   three = three || ( M[ i ][ j ] == 3 );
   if( M[ i ][ j ] == 1 ) {
    d[ i ][ j ] = 0;
    Q.emplace( i , j );
    }
   }
  }
 ADS_PRE( three && ! Q.empty() );
 const int di[] = { 1 , -1 , 0 , 0 } , dj[] = { 0 , 0 , 1 , -1 };
 while( true ) {
  const auto [ i , j ] = Q.front();
  Q.pop();
  for( int k = 0 ; k < 4 ; ++k ) {
   const int a = i + di[ k ] , b = j + dj[ k ];
   if( ( a < 0 ) || ( b < 0 ) || ( a >= n ) || ( b >= n ) ||
       ( d[ a ][ b ] >= 0 ) )
    continue;
   d[ a ][ b ] = d[ i ][ j ] + 1;
   if( M[ a ][ b ] == 3 ) return( d[ a ][ b ] );
   Q.emplace( a , b );
   }
  }
 }

// the grid of the text (2); against the least Manhattan distance between a
// 1 and a 3 on 500 random grids; a 1 x ... grid with 1 next to 3; the 1
// and the 3 at opposite corners of a large grid; the preconditions (no 3,
// no 1, not square, a color out of { 1 , 2 , 3 })
ADS_TEST( "esercitazioni-45" )
{
 CHECK( grid( { { 1 , 2 , 2 , 3 } , { 2 , 1 , 2 , 3 } , { 2 , 2 , 2 , 3 } ,
                { 3 , 2 , 1 , 2 } } ) == 2 );
 std::mt19937 g( 45 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 2 , 12 );
  std::vector< std::vector< int > > M( n , std::vector< int >( n , 2 ) );
  for( auto & row : M )
   for( auto & x : row ) x = rnd( g , 0 , 9 ) ? 2 : rnd( g , 1 , 3 );
  M[ rnd( g , 0 , n - 1 ) ][ rnd( g , 0 , n - 1 ) ] = 1;
  int a , b;
  do {
   a = rnd( g , 0 , n - 1 );
   b = rnd( g , 0 , n - 1 );
   } while( M[ a ][ b ] == 1 );
  M[ a ][ b ] = 3;
  int best = INF;
  for( int i = 0 ; i < n ; ++i ) for( int j = 0 ; j < n ; ++j )
   for( int k = 0 ; k < n ; ++k ) for( int l = 0 ; l < n ; ++l )
    if( ( M[ i ][ j ] == 1 ) && ( M[ k ][ l ] == 3 ) )
     best = std::min( best , std::abs( i - k ) + std::abs( j - l ) );
  CHECK( grid( M ) == best );
  }
 CHECK( grid( { { 1 , 3 } , { 2 , 2 } } ) == 1 );
 CHECK_THROWS( grid( { { 1 , 2 } , { 2 , 2 } } ) , contract_violation );
 std::vector< std::vector< int > > B( 300 , std::vector< int >( 300 , 2 ) );
 B[ 0 ][ 0 ] = 1;
 B[ 299 ][ 299 ] = 3;
 CHECK( grid( B ) == 598 );
 CHECK_THROWS( grid( { { 3 , 2 } , { 2 , 2 } } ) , contract_violation );
 CHECK_THROWS( grid( { { 1 , 3 } } ) , contract_violation );
 CHECK_THROWS( grid( { { 1 , 3 } , { 2 , 4 } } ) , contract_violation );
 CHECK_THROWS( grid( { { 1 , 3 } , { 0 , 2 } } ) , contract_violation );
 CHECK_THROWS( grid( {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ 46 - Shangai ------------------------------*/
/*--------------------------------------------------------------------------*/


/// whether all the n sticks can be taken, X[ i ] lying on Y[ i ]: the graph
/// of the edges X[ i ] -> Y[ i ] must have no cycle, by hasCycle() of the
/// library; O( n + m )
/// \pre the sticks in [ 0 , n ), distinct in each pair
bool shangai( int n , const std::vector< int > & X ,
              const std::vector< int > & Y )
{
 ADS_PRE( ( n >= 0 ) && ( X.size() == Y.size() ) );
 ListGraph<> G( n );
 for( std::size_t i = 0 ; i < X.size() ; ++i ) {
  ADS_PRE( ( 0 <= std::min( X[ i ] , Y[ i ] ) ) &&
           ( std::max( X[ i ] , Y[ i ] ) < n ) && ( X[ i ] != Y[ i ] ) );
  G.insertEdge( X[ i ] , Y[ i ] );
  }
 return( ! hasCycle( G ) );
 }

// against the game played: take any stick with nothing on it, until none is
// left or none can be taken, on 500 random games; a cycle of two sticks,
// no stick, no pair, a pair repeated, a long chain and the same chain
// closed; the preconditions (sizes, a stick on itself or out of range)
ADS_TEST( "esercitazioni-46" )
{
 std::mt19937 g( 46 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 12 );
  std::vector< int > X , Y;
  for( int i = rnd( g , 0 , 2 * n ) ; i > 0 ; --i ) {
   int a = rnd( g , 0 , n - 1 ) , b = rnd( g , 0 , n - 1 );
   if( a == b ) continue;
   if( ( r % 2 ) && ( a > b ) ) std::swap( a , b );     // half acyclic
   X.push_back( a );
   Y.push_back( b );
   }
  std::vector< bool > gone( n , false );
  for( bool more = true ; more ; ) {
   more = false;
   for( int u = 0 ; u < n ; ++u ) {
    if( gone[ u ] ) continue;
    bool covered = false;
    for( std::size_t i = 0 ; i < X.size() ; ++i )
     covered = covered || ( ( Y[ i ] == u ) && ! gone[ X[ i ] ] );
    if( ! covered ) gone[ u ] = more = true;
    }
   }
  CHECK( shangai( n , X , Y ) == std::ranges::all_of( gone , []( bool b ) {
                                                      return( b ); } ) );
  }
 CHECK( ! shangai( 2 , { 0 , 1 } , { 1 , 0 } ) && shangai( 0 , {} , {} ) );
 CHECK( shangai( 5 , {} , {} ) && shangai( 2 , { 0 , 0 } , { 1 , 1 } ) );
 std::vector< int > X( 999 ) , Y( 999 );
 std::iota( X.begin() , X.end() , 0 );
 std::iota( Y.begin() , Y.end() , 1 );
 CHECK( shangai( 1000 , X , Y ) );
 X.push_back( 999 );
 Y.push_back( 0 );
 CHECK( ! shangai( 1000 , X , Y ) );
 CHECK_THROWS( shangai( 2 , { 0 } , {} ) , contract_violation );
 CHECK_THROWS( shangai( 2 , { 1 } , { 1 } ) , contract_violation );
 CHECK_THROWS( shangai( 2 , { 0 } , { 2 } ) , contract_violation );
 CHECK_THROWS( shangai( -1 , {} , {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 48 - Grafi, pozzo universale ----------------------*/
/*--------------------------------------------------------------------------*/


// universalSink(), the node with no edge out and an edge from every other
// node, -1 (NIL) if none, is universalSink() of the library on the
// adjacency matrix: each test A[ i ][ j ] rules out i (an edge out) or j (a
// missing edge in), so one scan leaves one candidate, then checked on its
// row and column; Theta( n ), while lists need Theta( n + m )

// against the degrees of every node on 1000 random matrices, half of them
// with a sink planted; one node (a sink); no node; two nodes either way,
// two nodes pointing at each other, no edge, the matrix not square
ADS_TEST( "esercitazioni-48" )
{
 std::mt19937 g( 48 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  const int n = rnd( g , 1 , 10 );
  std::vector< std::vector< int > > A( n , std::vector< int >( n , 0 ) );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) if( i != j ) A[ i ][ j ] = rnd( g , 0 , 1 );
  if( r % 2 ) {
   const int s = rnd( g , 0 , n - 1 );
   for( int k = 0 ; k < n ; ++k )
    if( k != s ) { A[ s ][ k ] = 0; A[ k ][ s ] = rnd( g , 0 , 5 ) ? 1 : 0; }
   }
  int brute = -1;
  for( int u = 0 ; u < n ; ++u ) {
   int out = 0 , in = 0;
   for( int k = 0 ; k < n ; ++k ) { out += A[ u ][ k ]; in += A[ k ][ u ]; }
   if( ( out == 0 ) && ( in == n - 1 ) ) brute = u;
   }
  CHECK( universalSink( A ) == brute );
  }
 using Adj = std::vector< std::vector< int > >;
 CHECK( universalSink( Adj( { { 0 } } ) ) == 0 &&
        universalSink( Adj() ) == -1 );
 CHECK( universalSink( Adj( { { 0 , 1 } , { 0 , 0 } } ) ) == 1 &&
        universalSink( Adj( { { 0 , 0 } , { 1 , 0 } } ) ) == 0 );
 CHECK( universalSink( Adj( { { 0 , 1 } , { 1 , 0 } } ) ) == -1 );
 CHECK( universalSink( Adj( 3 , std::vector< int >( 3 , 0 ) ) ) == -1 );
 CHECK_THROWS( universalSink( Adj( { { 0 , 1 } } ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------- 49 - Grafi, tutte le strade portano a Roma (2012-05-03) ---------*/
/*--------------------------------------------------------------------------*/


/// isRoma(): whether every node reaches v, i.e., whether v reaches every
/// node in the transposed graph, by one visit in depth there; O( n + m )
/// \pre v is a node
template< Graph G >
bool isRoma( const G & g , int v )
{
 ADS_PRE( g.hasNode( v ) );
 const auto T = transpose( g );
 std::vector< bool > visited( T.bound() , false );
 dfs( T , v , visited , []( int ) {} , []( int , int ) {} , []( int ) {} );
 for( int u : T.V() ) if( ! visited[ u ] ) return( false );
 return( true );
 }

// against a visit in breadth from each node, which must reach v, on 500
// random oriented graphs; one node; a path towards its last node; a cycle
// (every node), two nodes with no edge, v not a node
ADS_TEST( "esercitazioni-49-1" )
{
 std::mt19937 g( 491 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 12 );
  const auto G = randomGraph( g , n , r % 2 ? 0.15 : 0.35 , true );
  const int v = rnd( g , 0 , n - 1 );
  bool all = true;
  for( int w = 0 ; w < n ; ++w )
   all = all && ( distance( G , w ).first[ v ] < INF );
  CHECK( isRoma( G , v ) == all );
  }
 ListGraph<> P( 4 );
 for( int i = 0 ; i < 3 ; ++i ) P.insertEdge( i , i + 1 );
 CHECK( isRoma( P , 3 ) && ! isRoma( P , 2 ) );
 CHECK( isRoma( ListGraph<>( 1 ) , 0 ) );
 P.insertEdge( 3 , 0 );
 for( int v = 0 ; v < 4 ; ++v ) CHECK( isRoma( P , v ) );
 CHECK( ! isRoma( ListGraph<>( 2 ) , 0 ) );
 CHECK_THROWS( isRoma( P , 4 ) , contract_violation );
 CHECK_THROWS( isRoma( P , -1 ) , contract_violation );
 }

/// hasRoma(): whether some node is reached from all the others: the root of
/// the last tree of a visit in depth of the transposed graph is the only
/// candidate, then checked by isRoma(); O( n + m )
/// \pre g not empty
template< Graph G >
bool hasRoma( const G & g )
{
 ADS_PRE( g.n() > 0 );
 const auto T = transpose( g );
 std::vector< bool > visited( T.bound() , false );
 int candidate = NIL;
 for( int u : T.V() )
  if( ! visited[ u ] ) {
   dfs( T , u , visited , []( int ) {} , []( int , int ) {} , []( int ) {} );
   candidate = u;
   }
 return( isRoma( g , candidate ) );
 }

// against isRoma() of every node and against one sink component in the
// condensation by scc() of the library, on 500 random graphs; one node,
// two nodes apart, a cycle, a star towards its center, two sinks
ADS_TEST( "esercitazioni-49-2" )
{
 std::mt19937 g( 492 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 12 );
  const auto G = randomGraph( g , n , r % 2 ? 0.1 : 0.25 , true );
  bool brute = false;
  for( int v = 0 ; v < n ; ++v ) brute = brute || isRoma( G , v );
  const auto C = scc( G );
  const int k = *std::ranges::max_element( C );
  std::vector< bool > leaves( k + 1 , false );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) ) if( C[ u ] != C[ v ] ) leaves[ C[ u ] ] = true;
  CHECK( hasRoma( G ) == brute );
  CHECK( brute == ( std::count( leaves.begin() + 1 , leaves.end() , false )
                    == 1 ) );
  }
 CHECK( hasRoma( ListGraph<>( 1 ) ) && ! hasRoma( ListGraph<>( 2 ) ) );
 ListGraph<> cyc( 5 ) , star( 6 ) , two( 4 );
 for( int i = 0 ; i < 5 ; ++i ) cyc.insertEdge( i , ( i + 1 ) % 5 );
 for( int i = 1 ; i < 6 ; ++i ) star.insertEdge( i , 0 );
 two.insertEdge( 0 , 1 );
 two.insertEdge( 0 , 2 );
 two.insertEdge( 3 , 2 );
 CHECK( hasRoma( cyc ) && hasRoma( star ) && ! hasRoma( two ) );
 CHECK_THROWS( hasRoma( ListGraph<>( 0 ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 50 - Grafi, stessa distanza -----------------------*/
/*--------------------------------------------------------------------------*/


/// sameDistance(): the nodes reached from both s1 and s2 at the same
/// distance, by two visits in breadth (distance() of the library); O( n + m )
/// \pre s1 and s2 are nodes
template< Graph G >
int sameDistance( const G & g , int s1 , int s2 )
{
 ADS_PRE( g.hasNode( s1 ) && g.hasNode( s2 ) );
 const auto d1 = distance( g , s1 ).first , d2 = distance( g , s2 ).first;
 int counter = 0;
 for( int u : g.V() )
  if( ( d1[ u ] < INF ) && ( d1[ u ] == d2[ u ] ) ) ++counter;
 return( counter );
 }

// against the distances of Floyd and Warshall on 500 random oriented
// graphs; s1 = s2 counts the nodes reached; one node, no edge (0), the
// preconditions
ADS_TEST( "esercitazioni-50" )
{
 std::mt19937 g( 50 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 15 );
  const auto G = randomGraph( g , n , 0.15 , true );
  std::vector< std::vector< int > > D( n , std::vector< int >( n , INF ) );
  for( int u = 0 ; u < n ; ++u ) {
   D[ u ][ u ] = 0;
   for( int v : G.adj( u ) ) D[ u ][ v ] = 1;
   }
  for( int k = 0 ; k < n ; ++k ) for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ( D[ i ][ k ] < INF ) && ( D[ k ][ j ] < INF ) )
     D[ i ][ j ] = std::min( D[ i ][ j ] , D[ i ][ k ] + D[ k ][ j ] );
  const int s1 = rnd( g , 0 , n - 1 ) , s2 = rnd( g , 0 , n - 1 );
  int c = 0 , reached = 0;
  for( int u = 0 ; u < n ; ++u ) {
   c += ( D[ s1 ][ u ] < INF ) && ( D[ s1 ][ u ] == D[ s2 ][ u ] );
   reached += D[ s1 ][ u ] < INF;
   }
  CHECK( sameDistance( G , s1 , s2 ) == c );
  CHECK( sameDistance( G , s1 , s1 ) == reached );
  }
 CHECK( sameDistance( ListGraph<>( 1 ) , 0 , 0 ) == 1 );
 CHECK( sameDistance( ListGraph<>( 4 ) , 0 , 3 ) == 0 );
 ListGraph<> V( 3 );
 V.insertEdge( 0 , 2 );
 V.insertEdge( 1 , 2 );
 CHECK( sameDistance( V , 0 , 1 ) == 1 );
 CHECK_THROWS( sameDistance( V , 0 , 3 ) , contract_violation );
 CHECK_THROWS( sameDistance( V , -1 , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 51 - Batterie (numero di fermate) --------------------*/
/*--------------------------------------------------------------------------*/


/// minStops(): the fewest battery swaps from km 0 to km L, a battery
/// lasting R km and the stations at dist[]: refuelingStops() of the
/// library, greedily: when the next station is out of reach, stop at the
/// last one in reach; O( n )
/// \pre dist strictly increasing in [ 0 , L ), every gap from 0 to L of at
///      most R
int minStops( const std::vector< int > & dist , int L , int R )
{
 const auto S = refuelingStops( dist , L , R );
 ADS_PRE( S.has_value() );
 return( int( S->size() ) );
 }

// against a visit in breadth over the stations (fewest stops) on 1000
// random trips; L <= R needs no stop; no station; a trip of length 0,
// gaps of exactly R, a station at 0 and at L, the extremes of int (where
// D + R overflowed), the preconditions (a gap beyond R, D unsorted or out
// of [ 0 , L ], L or R negative)
ADS_TEST( "esercitazioni-51" )
{
 std::mt19937 g( 51 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  const int R = rnd( g , 1 , 10 );
  std::vector< int > dist;
  int x = rnd( g , 0 , R );
  for( int k = rnd( g , 0 , 15 ) ; k > 0 ; --k ) {
   dist.push_back( x );
   x += rnd( g , 1 , R );
   }
  const int L = x , n = int( dist.size() );
  if( n && ( dist.back() >= L ) ) continue;
  std::vector< int > best( n + 1 , INF );       // stops to reach i ( n = L )
  for( int i = 0 ; i <= n ; ++i ) {
   const int at = i < n ? dist[ i ] : L;
   if( at <= R ) best[ i ] = 0;
   for( int j = 0 ; j < i ; ++j )
    if( ( best[ j ] < INF ) && ( at - dist[ j ] <= R ) )
     best[ i ] = std::min( best[ i ] , best[ j ] + 1 );
   }
  CHECK( minStops( dist , L , R ) == best[ n ] );
  }
 CHECK( minStops( { 1 , 2 } , 3 , 5 ) == 0 && minStops( {} , 4 , 4 ) == 0 );
 CHECK( minStops( { 0 , 5 , 10 } , 15 , 5 ) == 2 );
 CHECK_THROWS( minStops( {} , 5 , 4 ) , contract_violation );
 CHECK( minStops( {} , 0 , 0 ) == 0 && minStops( {} , 0 , 3 ) == 0 );
 CHECK( minStops( { 3 , 6 , 9 } , 12 , 3 ) == 3 );
 CHECK( minStops( { 0 , 4 , 8 } , 8 , 4 ) == 1 );
 const int M = std::numeric_limits< int >::max();
 CHECK( minStops( { 5 } , M , M - 3 ) == 1 );
 CHECK( minStops( { M - 1 } , M , M - 1 ) == 1 );
 CHECK( minStops( {} , M , M ) == 0 );
 CHECK_THROWS( minStops( { 3 , 9 } , 12 , 4 ) , contract_violation );
 CHECK_THROWS( minStops( { 6 , 3 } , 12 , 6 ) , contract_violation );
 CHECK_THROWS( minStops( { -1 } , 3 , 4 ) , contract_violation );
 CHECK_THROWS( minStops( { 13 } , 12 , 20 ) , contract_violation );
 CHECK_THROWS( minStops( {} , -1 , 4 ) , contract_violation );
 CHECK_THROWS( minStops( {} , 3 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- 52 - High Line (2017-06-29) -----------------------*/
/*--------------------------------------------------------------------------*/


/// minSprinklers(): the fewest sprinklers whose intervals [ D[ i ] - R[ i ] ,
/// D[ i ] + R[ i ] ] cover [ 0 , L ], -1 if impossible, 0 for L = 0: by
/// increasing left end, among the intervals that start within the covered
/// part take the one that reaches farthest; O( n log n ),
/// minimumIntervalCover() of the library
/// \pre D and R of the same size, R >= 0, L >= 0
int minSprinklers( const std::vector< int > & D ,
                   const std::vector< int > & R ,
                   int L )
{
 ADS_PRE( ( D.size() == R.size() ) && ( L >= 0 ) &&
          std::ranges::all_of( R , []( int x ) { return( x >= 0 ); } ) );
 if( L == 0 )
  return( 0 );
 const int n = int( D.size() );
 std::vector< std::pair< I64 , I64 > > I( n );
 for( int i = 0 ; i < n ; ++i )
  I[ i ] = std::pair( I64( D[ i ] ) - R[ i ] , I64( D[ i ] ) + R[ i ] );
 const auto cover = minimumIntervalCover( I , I64( 0 ) , I64( L ) );
 return( cover ? int( cover->size() ) : -1 );
 }

// against every subset of at most 12 sprinklers, the union of its
// intervals swept to see if it covers [ 0 , L ]; L = 0; no sprinkler; a
// radius of 0, one sprinkler covering exactly [ 0 , L ], the extremes of
// int; the preconditions
ADS_TEST( "esercitazioni-52" )
{
 std::mt19937 g( 52 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = rnd( g , 0 , 12 ) , L = rnd( g , 1 , 30 );
  const auto D = rndVector( g , n , -3 , L + 3 ) ,
             R = rndVector( g , n , 0 , 6 );
  int best = -1;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   const int k = std::popcount( unsigned( mask ) );
   if( ( best >= 0 ) && ( k >= best ) ) continue;
   std::vector< std::pair< int , int > > I;
   for( int i = 0 ; i < n ; ++i )
    if( ( mask >> i ) & 1 )
     I.emplace_back( D[ i ] - R[ i ] , D[ i ] + R[ i ] );
   std::ranges::sort( I );
   int covered = 0;
   for( const auto & [ a , b ] : I )
    if( a <= covered ) covered = std::max( covered , b );
   if( covered >= L ) best = k;
   }
  CHECK( minSprinklers( D , R , L ) == best );
  }
 CHECK( minSprinklers( {} , {} , 0 ) == 0 );
 CHECK( minSprinklers( {} , {} , 1 ) == -1 );
 CHECK( minSprinklers( { 2 , 6 } , { 2 , 2 } , 8 ) == 2 );
 CHECK( minSprinklers( { 2 , 7 } , { 2 , 2 } , 8 ) == -1 );
 CHECK( minSprinklers( { 0 , 1 } , { 0 , 0 } , 1 ) == -1 );
 CHECK( minSprinklers( { 0 } , { 0 } , 0 ) == 0 );
 CHECK( minSprinklers( { 5 } , { 5 } , 10 ) == 1 &&
        minSprinklers( { 5 } , { 5 } , 11 ) == -1 );
 const int M = std::numeric_limits< int >::max();
 CHECK( minSprinklers( { M } , { M } , M ) == 1 );
 CHECK( minSprinklers( { -M , M } , { M , M } , M ) == 1 );
 CHECK_THROWS( minSprinklers( { 1 } , {} , 3 ) , contract_violation );
 CHECK_THROWS( minSprinklers( { 1 } , { -1 } , 3 ) , contract_violation );
 CHECK_THROWS( minSprinklers( {} , {} , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- 53 - Scheduling -----------------------------*/
/*--------------------------------------------------------------------------*/


/// maxGain(): the proposed greedy for unit jobs with deadlines D and gains
/// G, by decreasing gain, each run at the next time t if t <= D[ i ]; its
/// gain (not optimal)
I64 maxGainGreedy( const std::vector< int > & D ,
                    const std::vector< int > & G )
{
 std::vector< int > order( D.size() );
 std::iota( order.begin() , order.end() , 0 );
 std::ranges::stable_sort( order , [ & ]( int a , int b ) {
  return( G[ a ] > G[ b ] ); } );
 I64 gain = 0;
 int t = 1;
 for( int i : order )
  if( t <= D[ i ] ) {
   gain += G[ i ];
   ++t;
   }
 return( gain );
 }

/// the optimal gain: by decreasing gain, each job in the latest free time
/// not after its deadline, if any; O( n^2 )
/// \pre D >= 1 and G >= 1
I64 maxGainLatest( const std::vector< int > & D ,
                    const std::vector< int > & G )
{
 ADS_PRE( D.size() == G.size() );
 std::vector< int > order( D.size() );
 std::iota( order.begin() , order.end() , 0 );
 std::ranges::stable_sort( order , [ & ]( int a , int b ) {
  return( G[ a ] > G[ b ] ); } );
 std::vector< bool > busy( D.size() + 1 , false );
 I64 gain = 0;
 for( int i : order ) {
  ADS_PRE( ( D[ i ] >= 1 ) && ( G[ i ] >= 1 ) );
  for( int t = std::min( D[ i ] , int( D.size() ) ) ; t >= 1 ; --t )
   if( ! busy[ t ] ) {
    busy[ t ] = true;
    gain += G[ i ];
    break;
    }
  }
 return( gain );
 }

// the counterexample of the text (greedy 2, optimum 3); against the best
// subset (one is feasible iff its i-th earliest deadline is >= i) on 500
// random instances, the greedy never above it and below it on some; no
// job, deadlines beyond n (all run), all deadlines 1 (the best one), the
// preconditions
ADS_TEST( "esercitazioni-53" )
{
 CHECK( maxGainGreedy( { 2 , 1 } , { 2 , 1 } ) == 2 );
 CHECK( maxGainLatest( { 2 , 1 } , { 2 , 1 } ) == 3 );
 std::mt19937 g( 53 );
 int worse = 0;
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 1 , 10 );
  const auto D = rndVector( g , n , 1 , 5 ) , G = rndVector( g , n , 1 , 9 );
  I64 best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   std::vector< int > d;
   I64 s = 0;
   for( int i = 0 ; i < n ; ++i )
    if( ( mask >> i ) & 1 ) { d.push_back( D[ i ] ); s += G[ i ]; }
   std::ranges::sort( d );
   bool ok = true;
   for( int i = 0 ; i < int( d.size() ) ; ++i ) ok = ok && ( d[ i ] >= i + 1 );
   if( ok ) best = std::max( best , s );
   }
  CHECK( maxGainLatest( D , G ) == best && maxGainGreedy( D , G ) <= best );
  worse += maxGainGreedy( D , G ) < best;
  }
 CHECK( worse > 0 );
 CHECK( maxGainLatest( {} , {} ) == 0 && maxGainGreedy( {} , {} ) == 0 );
 const int M = std::numeric_limits< int >::max();
 CHECK( maxGainLatest( { M , M , M } , { M , M , M } ) == I64( 3 ) * M );
 CHECK( maxGainLatest( { 1 , 1 , 1 } , { 4 , 9 , 2 } ) == 9 &&
        maxGainGreedy( { 1 , 1 , 1 } , { 4 , 9 , 2 } ) == 9 );
 CHECK_THROWS( maxGainLatest( { 1 } , {} ) , contract_violation );
 CHECK_THROWS( maxGainLatest( { 0 } , { 1 } ) , contract_violation );
 CHECK_THROWS( maxGainLatest( { 1 } , { 0 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- 54 - Sciatori ------------------------------*/
/*--------------------------------------------------------------------------*/


/// the proposed greedy for skiers H and skis L: the closest pair still free
/// first, the first one among equals; its cost (not optimal)
I64 skiGreedy( const std::vector< int > & H , const std::vector< int > & L )
{
 const int n = int( H.size() );
 std::vector< bool > h( n , false ) , l( n , false );
 I64 cost = 0;
 for( int k = 0 ; k < n ; ++k ) {
  int bi = -1 , bj = -1;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( ! h[ i ] && ! l[ j ] &&
        ( ( bi < 0 ) || ( std::abs( H[ i ] - L[ j ] ) <
                          std::abs( H[ bi ] - L[ bj ] ) ) ) ) {
     bi = i;
     bj = j;
     }
  h[ bi ] = l[ bj ] = true;
  cost += std::abs( H[ bi ] - L[ bj ] );
  }
 return( cost );
 }

/// the least sum of | H[ i ] - L[ m( i ) ] | over the matchings m of the
/// skiers and the skis: lineAssignment() of the library, both sorted and
/// matched in order, by an exchange argument; O( n log n )
/// \pre H and L of the same size
I64 skiMatching( const std::vector< int > & H , const std::vector< int > & L )
{
 return( lineAssignment< int , I64 >( H , L ) );
 }

// the counterexample of the text (greedy 10, optimum 8); against every
// matching for n <= 7 on 400 random instances, the greedy never better;
// no skier, one skier, all heights equal, the extremes of int, sizes
// differing
ADS_TEST( "esercitazioni-54" )
{
 CHECK( skiGreedy( { 5 , 10 } , { 9 , 14 } ) == 10 );
 CHECK( skiMatching( { 5 , 10 } , { 9 , 14 } ) == 8 );
 std::mt19937 g( 54 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const int n = rnd( g , 1 , 7 );
  const auto H = rndVector( g , n , 150 , 200 ) ,
             L = rndVector( g , n , 150 , 200 );
  std::vector< int > P( n );
  std::iota( P.begin() , P.end() , 0 );
  I64 best = std::numeric_limits< I64 >::max();
  do {
   I64 c = 0;
   for( int i = 0 ; i < n ; ++i ) c += std::abs( H[ i ] - L[ P[ i ] ] );
   best = std::min( best , c );
   } while( std::ranges::next_permutation( P ).found );
  CHECK( skiMatching( H , L ) == best && skiGreedy( H , L ) >= best );
  }
 CHECK( skiMatching( {} , {} ) == 0 );
 CHECK( skiMatching( { 170 } , { 160 } ) == 10 &&
        skiGreedy( { 170 } , { 160 } ) == 10 );
 CHECK( skiMatching( std::vector< int >( 9 , 180 ) ,
                     std::vector< int >( 9 , 180 ) ) == 0 );
 const int M = std::numeric_limits< int >::max();
 CHECK( skiMatching( { -M - 1 , M } , { M , -M - 1 } ) == 0 );
 CHECK( skiMatching( { -M - 1 } , { M } ) == I64( 2 ) * M + 1 );
 CHECK_THROWS( skiMatching( { 1 } , {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------- 55 - Sfilatino alla Nutella (2013-05-31) ----------------*/
/*--------------------------------------------------------------------------*/


/// maxSegments(): the most people served from a sandwich of length L, the
/// person i wanting exactly V[ i ]: the shortest requests first; O( n log n )
/// \pre V > 0, L >= 0
int maxSegments( std::vector< int > V , int L )
{
 ADS_PRE( ( L >= 0 ) &&
          std::ranges::all_of( V , []( int x ) { return( x > 0 ); } ) );
 std::ranges::sort( V );
 int i = 0;
 for( ; ( i < int( V.size() ) ) && ( L >= V[ i ] ) ; ++i ) L -= V[ i ];
 return( i );
 }

// against the largest subset within L on 600 random instances; L = 0, one
// request too long, all equal, no request, L exactly the total, the
// extremes of int, the preconditions (a request of 0, L negative)
ADS_TEST( "esercitazioni-55" )
{
 std::mt19937 g( 55 );
 for( int r = 0 ; r < 600 ; ++r ) {
  const int n = rnd( g , 0 , 14 ) , L = rnd( g , 0 , 60 );
  const auto V = rndVector( g , n , 1 , 20 );
  int best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   int s = 0;
   for( int i = 0 ; i < n ; ++i ) if( ( mask >> i ) & 1 ) s += V[ i ];
   if( s <= L ) best = std::max( best , std::popcount( unsigned( mask ) ) );
   }
  CHECK( maxSegments( V , L ) == best );
  }
 CHECK( maxSegments( { 3 } , 0 ) == 0 && maxSegments( { 9 } , 8 ) == 0 );
 CHECK( maxSegments( { 4 , 4 , 4 , 4 } , 13 ) == 3 );
 CHECK( maxSegments( {} , 5 ) == 0 && maxSegments( {} , 0 ) == 0 );
 CHECK( maxSegments( { 4 , 4 , 4 , 4 } , 16 ) == 4 );
 const int M = std::numeric_limits< int >::max();
 CHECK( maxSegments( { M , M , 1 } , M ) == 1 &&
        maxSegments( { M - 1 , 1 } , M ) == 2 );
 CHECK_THROWS( maxSegments( { 0 , 1 } , 3 ) , contract_violation );
 CHECK_THROWS( maxSegments( { -2 } , 3 ) , contract_violation );
 CHECK_THROWS( maxSegments( { 1 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------ 57 - Esercizi di programmazione, cerca la coppia ------------*/
/*--------------------------------------------------------------------------*/


/// whether two items in distinct places of A sum to v: A sorted, then
/// pairWithSum() of the library, two indices from the ends, the left one up
/// if the sum is too small and the right one down otherwise; O( n log n )
bool pairSum( std::vector< int > A , I64 v )
{
 std::ranges::sort( A );
 return( pairWithSum( A , v ).has_value() );
 }

/// whether two items in distinct places of A sum to v, all the pairs
bool pairSumBrute( const std::vector< int > & A , I64 v )
{
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = i + 1 ; j < A.size() ; ++j )
   if( I64( A[ i ] ) + A[ j ] == v ) return( true );
 return( false );
 }

// against all the pairs on 2000 random vectors and sums; the empty vector,
// one item (never), two equal items, the extremes of int, all items equal,
// a sum beyond any pair
ADS_TEST( "esercitazioni-57-1" )
{
 std::mt19937 g( 571 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 20 ) , -30 , 30 );
  const I64 v = rnd( g , -60 , 60 );
  CHECK( pairSum( A , v ) == pairSumBrute( A , v ) );
  }
 CHECK( ! pairSum( {} , 0 ) && ! pairSum( { 4 } , 8 ) );
 CHECK( pairSum( { 4 , 4 } , 8 ) );
 const int M = std::numeric_limits< int >::max();
 CHECK( pairSum( { M , M } , I64( 2 ) * M ) &&
        pairSum( { -M - 1 , M } , -1 ) );
 CHECK( pairSum( { -M - 1 , -M - 1 } , -I64( 2 ) * M - 2 ) &&
        ! pairSum( { M } , I64( 2 ) * M ) );
 CHECK( pairSum( std::vector< int >( 50 , 3 ) , 6 ) &&
        ! pairSum( std::vector< int >( 50 , 3 ) , 3 ) );
 CHECK( ! pairSum( { 1 , 2 , 3 } , 6 ) && ! pairSum( { M , M } , 0 ) );
 }

/// whether two items in distinct places of A sum to 17, by pairSum()
bool pairSum17( const std::vector< int > & A ) { return( pairSum( A , 17 ) ); }

// against all the pairs on 2000 random vectors; the empty vector, 17 with
// 0, a negative item, 17 alone
ADS_TEST( "esercitazioni-57-2" )
{
 std::mt19937 g( 572 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 20 ) , -10 , 30 );
  CHECK( pairSum17( A ) == pairSumBrute( A , 17 ) );
  }
 CHECK( pairSum17( { 8 , 9 } ) && ! pairSum17( { 17 } ) );
 CHECK( ! pairSum17( {} ) && pairSum17( { 17 , 0 } ) &&
        pairSum17( { -3 , 20 } ) && ! pairSum17( { 8 , 8 , 10 } ) );
 }

/// whether two items in distinct places of the positive A sum to 17: flags
/// of the values 1 ... 16 present, then the pairs ( i , 17 - i ), of
/// distinct values since 17 is odd; O( n ) time, O( 1 ) space
/// \pre A > 0
bool pairSum17Positive( const std::vector< int > & A )
{
 ADS_PRE( std::ranges::all_of( A , []( int x ) { return( x > 0 ); } ) );
 std::array< bool , 17 > present{};
 for( int x : A ) if( x < 17 ) present[ x ] = true;
 for( int i = 1 ; i <= 8 ; ++i )
  if( present[ i ] && present[ 17 - i ] ) return( true );
 return( false );
 }

// against all the pairs on 2000 random positive vectors; values >= 17
// only; the empty vector; the pairs ( 1 , 16 ) and ( 8 , 9 ) at the ends
// of the flags, one value repeated, the largest int; a negative item
ADS_TEST( "esercitazioni-57-3" )
{
 std::mt19937 g( 573 );
 for( int r = 0 ; r < 2000 ; ++r ) {
  const auto A = rndVector( g , rnd( g , 0 , 15 ) , 1 , r % 2 ? 20 : 1000 );
  CHECK( pairSum17Positive( A ) == pairSumBrute( A , 17 ) );
  }
 CHECK( ! pairSum17Positive( {} ) && ! pairSum17Positive( { 17 , 34 } ) );
 CHECK_THROWS( pairSum17Positive( { 0 , 17 } ) , contract_violation );
 CHECK( pairSum17Positive( { 16 , 1 } ) && pairSum17Positive( { 9 , 8 } ) );
 CHECK( ! pairSum17Positive( { 8 , 8 , 8 } ) &&
        ! pairSum17Positive( { std::numeric_limits< int >::max() , 1 } ) );
 CHECK_THROWS( pairSum17Positive( { 18 , -1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- 59 - Vito's Family ---------------------------*/
/*--------------------------------------------------------------------------*/


/// the least sum of | s_i - x | over x: x is a median, found by
/// selection() of the library in linear expected time, then the sum of the
/// distances; O( n )
/// \pre s not empty
I64 vito( std::vector< int > s )
{
 ADS_PRE( ! s.empty() );
 const int x = selection( s , ( s.size() + 1 ) / 2 );
 I64 sum = 0;
 for( int v : s ) sum += std::abs( v - x );
 return( sum );
 }

// against every x between the least and the largest number on 500 random
// streets, with repetitions; one relative; the limits 0 < r < 500 and
// 0 < s_i < 30000 at their largest; all equal (0), an even number of
// relatives (any x between the two medians), no relative rejected
ADS_TEST( "esercitazioni-59" )
{
 std::mt19937 g( 59 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto s = rndVector( g , rnd( g , 1 , 30 ) , 1 , r % 2 ? 20 : 300 );
  I64 best = std::numeric_limits< I64 >::max();
  for( int x = *std::ranges::min_element( s ) ;
       x <= *std::ranges::max_element( s ) ; ++x ) {
   I64 t = 0;
   for( int v : s ) t += std::abs( v - x );
   best = std::min( best , t );
   }
  CHECK( vito( s ) == best );
  }
 CHECK( vito( { 7 } ) == 0 && vito( { 2 , 4 } ) == 2 );
 std::vector< int > big( 499 );
 for( int i = 0 ; i < 499 ; ++i ) big[ i ] = i % 2 ? 29999 : 1;
 CHECK( vito( big ) == I64( 249 ) * 29998 );
 CHECK( vito( std::vector< int >( 499 , 29999 ) ) == 0 );
 CHECK( vito( { 1 , 3 , 10 , 20 } ) == 26 );
 CHECK_THROWS( vito( {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*----------------- 60 - Batterie (costo di sostituzione) ------------------*/
/*--------------------------------------------------------------------------*/


/// minCostStops(): the least cost of the batteries from km 0 to the exit
/// dist[ n ] = L, cost[ n ] = 0, LONG_MAX if unreachable: DP[ i ] = cost[ i ]
/// if dist[ i ] <= R, else cost[ i ] plus the least DP[ j ] with dist[ i ] -
/// dist[ j ] <= R; O( n^2 )
/// \pre dist strictly increasing and positive, cost >= 0, cost[ n ] = 0
I64 minCostStops( const std::vector< int > & dist ,
                   const std::vector< int > & cost , int R )
{
 const int n = int( dist.size() ) - 1;
 ADS_PRE( ( n >= 0 ) && ( cost.size() == dist.size() ) && ( cost[ n ] == 0 ) &&
          std::ranges::adjacent_find( dist , std::greater_equal<>() ) ==
          dist.end() && ( dist[ 0 ] > 0 ) );
 const I64 inf = std::numeric_limits< I64 >::max();
 std::vector< I64 > DP( n + 1 , inf );
 for( int i = 0 ; i <= n ; ++i )
  if( dist[ i ] <= R )
   DP[ i ] = cost[ i ];
  else
   for( int j = i - 1 ; ( j >= 0 ) && ( dist[ i ] - dist[ j ] <= R ) ; --j )
    if( DP[ j ] < inf ) DP[ i ] = std::min( DP[ i ] , DP[ j ] + cost[ i ] );
 return( DP[ n ] );
 }

// against every subset of the stations whose gaps are within R on 500
// random trips, some of them impossible; the exit within R costs 0; the
// first station beyond R, a gap of exactly R, the largest int; the
// preconditions (no exit, sizes, cost of the exit, dist not increasing or
// not positive)
ADS_TEST( "esercitazioni-60" )
{
 std::mt19937 g( 60 );
 const I64 inf = std::numeric_limits< I64 >::max();
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 0 , 11 ) , R = rnd( g , 2 , 8 );
  std::vector< int > dist , cost;
  for( int i = 0 , x = 0 ; i <= n ; ++i ) {
   dist.push_back( x += rnd( g , 1 , R + 1 ) );
   cost.push_back( i < n ? rnd( g , 0 , 9 ) : 0 );
   }
  I64 best = inf;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   I64 c = 0;
   int at = 0;
   bool ok = true;
   for( int i = 0 ; ok && ( i <= n ) ; ++i )
    if( ( i == n ) || ( ( mask >> i ) & 1 ) ) {
     ok = dist[ i ] - at <= R;
     at = dist[ i ];
     c += cost[ i ];
     }
   if( ok ) best = std::min( best , c );
   }
  CHECK( minCostStops( dist , cost , R ) == best );
  }
 CHECK( minCostStops( { 5 } , { 0 } , 5 ) == 0 );
 CHECK( minCostStops( { 3 , 9 } , { 4 , 0 } , 5 ) == inf );
 CHECK( minCostStops( { 3 , 7 } , { 4 , 0 } , 5 ) == 4 );
 CHECK( minCostStops( { 6 , 8 } , { 1 , 0 } , 5 ) == inf );
 CHECK( minCostStops( { 5 , 10 } , { 7 , 0 } , 5 ) == 7 );
 CHECK( minCostStops( { 2 , 4 , 6 } , { 9 , 1 , 0 } , 4 ) == 1 );
 const int M = std::numeric_limits< int >::max();
 CHECK( minCostStops( { M } , { 0 } , M ) == 0 &&
        minCostStops( { 1 , M } , { M , 0 } , M - 1 ) == M );
 CHECK_THROWS( minCostStops( {} , {} , 5 ) , contract_violation );
 CHECK_THROWS( minCostStops( { 3 } , {} , 5 ) , contract_violation );
 CHECK_THROWS( minCostStops( { 3 } , { 1 } , 5 ) , contract_violation );
 CHECK_THROWS( minCostStops( { 3 , 3 } , { 1 , 0 } , 5 ) ,
               contract_violation );
 CHECK_THROWS( minCostStops( { 0 , 3 } , { 1 , 0 } , 5 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- 61 - D20 (2013-09-02) --------------------------*/
/*--------------------------------------------------------------------------*/


/// the ways of obtaining X with the dice of F[ i ] faces, counting the
/// permutations: DP[ x ][ i ] = sum_{ j = 1 }^{ F[ i - 1 ] }
/// DP[ x - j ][ i - 1 ];
/// O( n X M )
/// \pre F >= 1, X >= 0
std::uint64_t diceOrdered( const std::vector< int > & F , int X )
{
 ADS_PRE( ( X >= 0 ) &&
          std::ranges::all_of( F , []( int f ) { return( f >= 1 ); } ) );
 std::vector< std::uint64_t > DP( X + 1 , 0 );
 DP[ 0 ] = 1;
 for( int f : F ) {
  std::vector< std::uint64_t > N( X + 1 , 0 );
  for( int x = 1 ; x <= X ; ++x )
   for( int j = 1 ; ( j <= f ) && ( j <= x ) ; ++j ) N[ x ] += DP[ x - j ];
  DP = std::move( N );
  }
 return( DP[ X ] );
 }

/// the ways of obtaining X with the dice of F[ i ] faces, not counting the
/// permutations: the dice by decreasing faces, the last ones (the fewest
/// faces) take the least values, DP[ x ][ i ][ m ] the ways for the first i
/// dice with values >= m, = DP[ x - m ][ i - 1 ][ m ] + DP[ x ][ i ][ m + 1 ]
/// (the second term only if m < F[ i - 1 ]); O( n X M )
/// \pre n >= 1, F >= 1, X >= 0
std::uint64_t diceUnordered( std::vector< int > F , int X )
{
 ADS_PRE( ! F.empty() && ( X >= 0 ) &&
          std::ranges::all_of( F , []( int f ) { return( f >= 1 ); } ) );
 std::ranges::sort( F , std::greater<>() );
 const int n = int( F.size() ) , M = F[ 0 ];
 // DP[ i ][ x ][ m ], m in 1 ... M + 1
 std::vector< std::vector< std::vector< std::uint64_t > > > DP( n + 1 ,
  std::vector< std::vector< std::uint64_t > >( X + 1 ,
   std::vector< std::uint64_t >( M + 2 , 0 ) ) );
 for( int m = 1 ; m <= M + 1 ; ++m ) DP[ 0 ][ 0 ][ m ] = 1;
 for( int i = 1 ; i <= n ; ++i )
  for( int x = 1 ; x <= X ; ++x )
   for( int m = F[ i - 1 ] ; m >= 1 ; --m ) {
    DP[ i ][ x ][ m ] = x >= m ? DP[ i - 1 ][ x - m ][ m ] : 0;
    if( m < F[ i - 1 ] ) DP[ i ][ x ][ m ] += DP[ i ][ x ][ m + 1 ];
    }
 return( DP[ n ][ X ][ 1 ] );
 }

// the examples of the text (two d4 give 7 in 1 way, d4, d4, d6 give 8 in 5);
// against all the outcomes, counted and as distinct sorted tuples, on 300
// random sets of dice; X = 0 and X beyond the largest sum, X equal to the
// least and to the largest sum (1 way), dice of one face; the
// preconditions (no die, a die of 0 faces, X negative)
ADS_TEST( "esercitazioni-61" )
{
 CHECK( diceUnordered( { 4 , 4 } , 7 ) == 1 );
 CHECK( diceUnordered( { 4 , 4 , 6 } , 8 ) == 5 );
 CHECK( diceOrdered( { 4 , 4 } , 7 ) == 2 );
 std::mt19937 g( 61 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 1 , 4 );
  const auto F = rndVector( g , n , 1 , 7 );
  const int X = rnd( g , 0 , 30 );
  std::uint64_t ordered = 0;
  std::set< std::vector< int > > unordered;
  std::vector< int > v( n , 1 );
  while( true ) {
   if( std::accumulate( v.begin() , v.end() , 0 ) == X ) {
    ++ordered;
    auto s = v;
    std::ranges::sort( s );
    unordered.insert( s );
    }
   int i = 0;
   while( ( i < n ) && ( v[ i ] == F[ i ] ) ) v[ i++ ] = 1;
   if( i == n ) break;
   ++v[ i ];
   }
  CHECK( diceOrdered( F , X ) == ordered );
  CHECK( diceUnordered( F , X ) == unordered.size() );
  }
 CHECK( diceUnordered( { 6 } , 0 ) == 0 && diceOrdered( {} , 0 ) == 1 );
 CHECK( diceUnordered( { 6 , 6 } , 13 ) == 0 );
 CHECK( diceOrdered( { 6 , 6 } , 13 ) == 0 && diceOrdered( { 6 } , 0 ) == 0 &&
        diceOrdered( {} , 3 ) == 0 );
 CHECK( diceOrdered( { 4 , 6 , 8 } , 3 ) == 1 &&
        diceUnordered( { 4 , 6 , 8 } , 3 ) == 1 );
 CHECK( diceOrdered( { 4 , 6 , 8 } , 18 ) == 1 &&
        diceUnordered( { 4 , 6 , 8 } , 18 ) == 1 );
 CHECK( diceOrdered( std::vector< int >( 30 , 1 ) , 30 ) == 1 &&
        diceUnordered( std::vector< int >( 30 , 1 ) , 30 ) == 1 &&
        diceUnordered( std::vector< int >( 30 , 1 ) , 29 ) == 0 );
 CHECK( diceUnordered( { 1 , 6 } , 7 ) == 1 &&
        diceOrdered( { 1 , 6 } , 7 ) == 1 );
 CHECK_THROWS( diceUnordered( {} , 0 ) , contract_violation );
 CHECK_THROWS( diceUnordered( { 6 , 0 } , 3 ) , contract_violation );
 CHECK_THROWS( diceUnordered( { 6 } , -1 ) , contract_violation );
 CHECK_THROWS( diceOrdered( { 0 } , 3 ) , contract_violation );
 CHECK_THROWS( diceOrdered( { 6 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 62 - Donald Trump (2016-07-16) ---------------------*/
/*--------------------------------------------------------------------------*/


/// serveDJT(): the most people at rallies in cities at the strictly
/// increasing miles[], two rallies at least D apart: DP[ i ] =
/// max( DP[ i - 1 ] ,
/// people[ i - 1 ] + DP[ j ] ) over the compatible j < i, scanned from j = 1
/// while compatible; Theta( n^2 )
/// \pre miles strictly increasing, people >= 0
I64 serveDJT( const std::vector< int > & miles ,
               const std::vector< int > & people , int D )
{
 const int n = int( miles.size() );
 ADS_PRE( ( int( people.size() ) == n ) &&
          std::ranges::adjacent_find( miles , std::greater_equal<>() ) ==
          miles.end() );
 std::vector< I64 > DP( n + 1 , 0 );
 for( int i = 1 ; i <= n ; ++i ) {
  I64 best = people[ i - 1 ];
  for( int j = 1 ;
       ( j < i ) && ( I64( miles[ i - 1 ] ) - miles[ j - 1 ] >= D ) ;
       ++j )
   best = std::max( best , people[ i - 1 ] + DP[ j ] );
  DP[ i ] = std::max( best , DP[ i - 1 ] );
  }
 return( DP[ n ] );
 }

// against every subset of at most 14 cities and against maxinterval() of
// the library on the intervals [ miles[ i ] - D , miles[ i ] ), on 500
// random routes; no city; D = 0 takes them all; D beyond the route (the
// largest city alone), cities of 0 people, the extremes of int; the
// preconditions (sizes, miles not increasing)
ADS_TEST( "esercitazioni-62" )
{
 std::mt19937 g( 62 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 0 , 14 ) , D = rnd( g , 1 , 12 );
  std::vector< int > miles( n ) , people = rndVector( g , n , 0 , 50 );
  for( int i = 0 , x = 0 ; i < n ; ++i ) miles[ i ] = x += rnd( g , 1 , 6 );
  I64 best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   I64 s = 0;
   int last = -1;
   bool ok = true;
   for( int i = 0 ; ok && ( i < n ) ; ++i )
    if( ( mask >> i ) & 1 ) {
     ok = ( last < 0 ) || ( miles[ i ] - miles[ last ] >= D );
     last = i;
     s += people[ i ];
     }
   if( ok ) best = std::max( best , s );
   }
  std::vector< WeightedInterval< I64 > > I;
  for( int i = 0 ; i < n ; ++i )
   I.push_back( WeightedInterval< I64 >{ miles[ i ] - D , miles[ i ] ,
                                          people[ i ] } );
  CHECK( serveDJT( miles , people , D ) == best );
  CHECK( maxinterval( I ).second == best );
  }
 CHECK( serveDJT( {} , {} , 5 ) == 0 );
 CHECK( serveDJT( { 1 , 2 , 3 } , { 1 , 2 , 3 } , 0 ) == 6 );
 CHECK( serveDJT( { 1 , 2 , 3 } , { 4 , 9 , 2 } , 100 ) == 9 );
 CHECK( serveDJT( { 1 , 2 , 3 } , { 0 , 0 , 0 } , 1 ) == 0 );
 const int M = std::numeric_limits< int >::max();
 CHECK( serveDJT( { -M - 1 , M } , { M , M } , M ) == I64( 2 ) * M );
 CHECK( serveDJT( { -M - 1 , 1 , M } , { 1 , 5 , 1 } , M ) == 6 );
 CHECK_THROWS( serveDJT( { 1 } , {} , 1 ) , contract_violation );
 CHECK_THROWS( serveDJT( { 2 , 2 } , { 1 , 1 } , 1 ) , contract_violation );
 CHECK_THROWS( serveDJT( { 3 , 2 } , { 1 , 1 } , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------ 63 - Costo partizione di un vettore (2014-06-05) ------------*/
/*--------------------------------------------------------------------------*/


/// partition2(): the least cost of a 2-partition of V, the largest sum of
/// its two parts, by one scan of the prefix sums; O( n )
/// \pre n >= 2
I64 partition2( const std::vector< int > & V )
{
 const int n = int( V.size() );
 ADS_PRE( n >= 2 );
 const I64 tot = std::accumulate( V.begin() , V.end() , I64( 0 ) );
 I64 prefix = 0 , best = std::numeric_limits< I64 >::max();
 for( int i = 0 ; i < n - 1 ; ++i ) {
  prefix += V[ i ];
  best = std::min( best , std::max( prefix , tot - prefix ) );
  }
 return( best );
 }

/// the cost of the k-partition of V with the cuts c (the starts of the
/// parts after the first), by recomputing the sums
I64 partitionCost( const std::vector< int > & V ,
                    const std::vector< int > & c )
{
 I64 worst = std::numeric_limits< I64 >::min();
 for( std::size_t t = 0 ; t <= c.size() ; ++t ) {
  const int a = t ? c[ t - 1 ] : 0 ,
            b = t < c.size() ? c[ t ] : int( V.size() );
  worst = std::max( worst , std::accumulate( V.begin() + a ,
                                             V.begin() + b , I64( 0 ) ) );
  }
 return( worst );
 }

/// the least cost over all the k-partitions of V, every set of cuts
I64 partitionBrute( const std::vector< int > & V , int k )
{
 const int n = int( V.size() );
 std::vector< bool > pick( n - 1 , false );
 std::fill( pick.end() - ( k - 1 ) , pick.end() , true );
 I64 best = std::numeric_limits< I64 >::max();
 do {
  std::vector< int > c;
  for( int i = 0 ; i < n - 1 ; ++i ) if( pick[ i ] ) c.push_back( i + 1 );
  best = std::min( best , partitionCost( V , c ) );
  } while( std::next_permutation( pick.begin() , pick.end() ) );
 return( best );
 }

// the vector of the text (12, the cut after place 2); against every cut on
// 500 random vectors with negative values; two items, all equal, the
// extremes of int (sums beyond int); one item rejected
ADS_TEST( "esercitazioni-63-1" )
{
 const std::vector< int > V{ 2 , 3 , 7 , -7 , 15 , 2 };
 CHECK( partitionCost( V , { 3 } ) == 12 && partition2( V ) == 12 );
 std::mt19937 g( 631 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto W = rndVector( g , rnd( g , 2 , 15 ) , -20 , 20 );
  CHECK( partition2( W ) == partitionBrute( W , 2 ) );
  }
 CHECK( partition2( { -5 , 3 } ) == 3 );
 const int M = std::numeric_limits< int >::max();
 CHECK( partition2( std::vector< int >( 10 , 4 ) ) == 20 );
 CHECK( partition2( { M , M , M , M } ) == I64( 2 ) * M );
 CHECK( partition2( { -M - 1 , -M - 1 , -M - 1 } ) == -I64( 1 ) * M - 1 );
 CHECK_THROWS( partition2( { 1 } ) , contract_violation );
 CHECK_THROWS( partition2( {} ) , contract_violation );
 }

/// partition3(): the least cost of a 3-partition of V, over every pair of
/// cuts with the prefix sums; O( n^2 )
/// \pre n >= 3
I64 partition3( const std::vector< int > & V )
{
 const int n = int( V.size() );
 ADS_PRE( n >= 3 );
 std::vector< I64 > tot( n );            // partial_sum would add in int
 for( int i = 0 ; i < n ; ++i )
  tot[ i ] = ( i ? tot[ i - 1 ] : I64( 0 ) ) + V[ i ];
 I64 best = std::numeric_limits< I64 >::max();
 for( int i = 0 ; i < n - 2 ; ++i )
  for( int j = i + 1 ; j <= n - 2 ; ++j )
   best = std::min( best , std::max( { tot[ i ] , tot[ j ] - tot[ i ] ,
                                       tot[ n - 1 ] - tot[ j ] } ) );
 return( best );
 }

// the vector of the text (10); against every pair of cuts on 500 random
// vectors; three items, all equal, the extremes of int; fewer than three
// items rejected
ADS_TEST( "esercitazioni-63-2" )
{
 CHECK( partition3( { 2 , 3 , 7 , -7 , 15 , 2 } ) == 10 );
 std::mt19937 g( 632 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto W = rndVector( g , rnd( g , 3 , 15 ) , -20 , 20 );
  CHECK( partition3( W ) == partitionBrute( W , 3 ) );
  }
 CHECK( partition3( { 1 , 2 , 3 } ) == 3 );
 const int M = std::numeric_limits< int >::max();
 CHECK( partition3( std::vector< int >( 9 , 5 ) ) == 15 );
 CHECK( partition3( { M , M , M , M , M , M } ) == I64( 2 ) * M );
 CHECK_THROWS( partition3( { 1 , 2 } ) , contract_violation );
 }

/// partition(): the least cost of a k-partition of V into contiguous
/// non-empty parts, the cost being the largest sum of a part: DP[ t ][ i ] =
/// min_{ j < i } max( DP[ t - 1 ][ j ] , tot[ i ] - tot[ j ] ) for the first
/// i + 1 items in t parts; O( k n^2 ) time, linearPartition() of the
/// library
/// \pre 1 <= k <= n
I64 partition( const std::vector< int > & V , int k )
{
 ADS_PRE( ( 1 <= k ) && ( k <= int( V.size() ) ) );
 return( linearPartition( std::vector< I64 >( V.begin() , V.end() ) ,
                          std::size_t( k ) ).first );
 }

// the vector of the text (10 for k = 3); against every set of k - 1 cuts on
// 500 random vectors; k = 1 (the sum) and k = n (the largest item); one
// item, the extremes of int; k = 0, k > n and the empty vector rejected
ADS_TEST( "esercitazioni-63-3" )
{
 const std::vector< int > V{ 2 , 3 , 7 , -7 , 15 , 2 };
 CHECK( partition( V , 3 ) == 10 && partition( V , 2 ) == 12 );
 std::mt19937 g( 633 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const auto W = rndVector( g , rnd( g , 1 , 13 ) , -20 , 20 );
  const int k = rnd( g , 1 , int( W.size() ) );
  CHECK( partition( W , k ) == partitionBrute( W , k ) );
  CHECK( partition( W , 1 ) ==
         std::accumulate( W.begin() , W.end() , I64( 0 ) ) );
  CHECK( partition( W , int( W.size() ) ) == *std::ranges::max_element( W ) );
  }
 CHECK_THROWS( partition( V , 7 ) , contract_violation );
 const int M = std::numeric_limits< int >::max();
 CHECK( partition( { -4 } , 1 ) == -4 );
 CHECK( partition( std::vector< int >( 8 , M ) , 2 ) == I64( 4 ) * M );
 CHECK( partition( std::vector< int >( 8 , -M - 1 ) , 3 ) ==
        I64( 2 ) * ( -M - 1 ) );
 CHECK_THROWS( partition( V , 0 ) , contract_violation );
 CHECK_THROWS( partition( {} , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- 64 - I Promessi Sposi --------------------------*/
/*--------------------------------------------------------------------------*/


// the sequences of places of T that spell P: DP[ i ][ j ] = DP[ i - 1 ][ j ]
// + ( T[ i - 1 ] = P[ j - 1 ] ? DP[ i - 1 ][ j - 1 ] : 0 ), one row kept
// and updated from the right, modulo 2^64 when the count is larger:
// subsequenceOccurrences( P , T ) of the library [see
// SubsequenceProblems.h]

// the example of the text ("dog" in "did you go", 2); against every set of
// m places of T on 500 random short strings; the empty pattern (1), a
// pattern longer than the text (0), "a"^m in "a"^n (C( n , m ), from the
// triangle of the library); both empty (1), an empty text (0), the
// pattern equal to the text (1), a character missing from the text (0),
// "a"^50 in "a"^120 modulo 2^64 against the triangle in 64 bits
ADS_TEST( "esercitazioni-64" )
{
 CHECK( subsequenceOccurrences( "dog" , "did you go" ) == 2 );
 std::mt19937 g( 64 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::string T( rnd( g , 0 , 12 ) , 'a' ) , P( rnd( g , 0 , 4 ) , 'a' );
  for( auto & c : T ) c = char( 'a' + rnd( g , 0 , 2 ) );
  for( auto & c : P ) c = char( 'a' + rnd( g , 0 , 2 ) );
  const int n = int( T.size() ) , m = int( P.size() );
  std::uint64_t brute = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   if( std::popcount( unsigned( mask ) ) != m ) continue;
   std::string s;
   for( int i = 0 ; i < n ; ++i ) if( ( mask >> i ) & 1 ) s += T[ i ];
   brute += s == P;
   }
  CHECK( subsequenceOccurrences( P , T ) == brute );
  }
 CHECK( subsequenceOccurrences( "" , "abc" ) == 1 );
 CHECK( subsequenceOccurrences( "abc" , "ab" ) == 0 );
 CHECK( subsequenceOccurrences( std::string( 25 , 'a' ) ,
                                std::string( 60 , 'a' ) ) ==
        tartaglia( 60 )[ 60 ][ 25 ] );
 CHECK( subsequenceOccurrences( "" , "" ) == 1 &&
        subsequenceOccurrences( "a" , "" ) == 0 );
 CHECK( subsequenceOccurrences( "abcab" , "abcab" ) == 1 &&
        subsequenceOccurrences( "abz" , "aabbcc" ) == 0 );
 std::vector< std::uint64_t > C( 51 , 0 );        // C( n , m ) mod 2^64
 C[ 0 ] = 1;
 for( int n = 1 ; n <= 120 ; ++n )
  for( int m = std::min( n , 50 ) ; m >= 1 ; --m ) C[ m ] += C[ m - 1 ];
 CHECK( subsequenceOccurrences( std::string( 50 , 'a' ) ,
                                std::string( 120 , 'a' ) ) == C[ 50 ] );
 }

/*--------------------------------------------------------------------------*/
/*------------------ 65 - Massima copertura (2014-06-05) -------------------*/
/*--------------------------------------------------------------------------*/


/// maxCover(): disjoint half-open segments [ s[ i ] , f[ i ] ) covering the
/// most length: the weighted independent set of intervals of weights
/// f[ i ] - s[ i ], by maxinterval() of the library; Theta( n log n ); the
/// places of the segments and the length
/// \pre s[ i ] < f[ i ]
std::pair< std::vector< std::size_t > , I64 > maxCover(
                                             const std::vector< I64 > & s ,
                                             const std::vector< I64 > & f )
{
 ADS_PRE( s.size() == f.size() );
 std::vector< WeightedInterval< I64 > > I;
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  I.push_back( WeightedInterval< I64 >{ s[ i ] , f[ i ] , f[ i ] - s[ i ] } );
 return( maxinterval( I ) );
 }

// against every subset of at most 14 segments on 500 random instances, the
// segments returned disjoint and of the length found; no segment; two
// touching segments (both), the same segment twice (once), one inside
// another (the longer); sizes differing rejected
ADS_TEST( "esercitazioni-65" )
{
 std::mt19937 g( 65 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 0 , 14 );
  std::vector< I64 > s( n ) , f( n );
  for( int i = 0 ; i < n ; ++i ) {
   s[ i ] = rnd( g , 0 , 30 );
   f[ i ] = s[ i ] + rnd( g , 1 , 10 );
   }
  I64 best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   bool ok = true;
   I64 len = 0;
   for( int i = 0 ; i < n ; ++i ) {
    if( ! ( ( mask >> i ) & 1 ) ) continue;
    len += f[ i ] - s[ i ];
    for( int j = i + 1 ; j < n ; ++j )
     if( ( ( mask >> j ) & 1 ) && ( s[ i ] < f[ j ] ) && ( s[ j ] < f[ i ] ) )
      ok = false;
    }
   if( ok ) best = std::max( best , len );
   }
  const auto [ P , len ] = maxCover( s , f );
  CHECK( len == best );
  I64 sum = 0;
  for( std::size_t a = 0 ; a < P.size() ; ++a ) {
   sum += f[ P[ a ] ] - s[ P[ a ] ];
   for( std::size_t b = a + 1 ; b < P.size() ; ++b )
    CHECK( ( s[ P[ a ] ] >= f[ P[ b ] ] ) || ( s[ P[ b ] ] >= f[ P[ a ] ] ) );
   }
  CHECK( sum == best );
  }
 CHECK( maxCover( {} , {} ).second == 0 );
 CHECK( maxCover( { 0 , 5 } , { 5 , 10 } ).second == 10 );
 CHECK( maxCover( { 2 , 2 } , { 9 , 9 } ).second == 7 &&
        maxCover( { 2 , 2 } , { 9 , 9 } ).first.size() == 1 );
 CHECK( maxCover( { 0 , 3 } , { 10 , 4 } ).second == 10 );
 CHECK_THROWS( maxCover( { 0 } , {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- 66 - k-sottosequenza massimale ---------------------*/
/*--------------------------------------------------------------------------*/


/// the largest sum of a k-subsequence of V, which keeps no more than k
/// consecutive items: best[ i ] for V[ 0 ... i - 1 ] either drops V[ i - 1 ]
/// or ends with a run of r <= k kept items preceded by a dropped one,
/// best[ i ] = max( best[ i - 1 ] , max_r sum V[ i - r ... i - 1 ] +
/// best[ i - r - 1 ] ); O( n k ) with the prefix sums (the text gives no
/// official solution)
/// \pre k >= 1
I64 kSubsequence( const std::vector< int > & V , int k )
{
 ADS_PRE( k >= 1 );
 const int n = int( V.size() );
 std::vector< I64 > pre( n + 1 , 0 ) , best( n + 1 , 0 );
 for( int i = 0 ; i < n ; ++i ) pre[ i + 1 ] = pre[ i ] + V[ i ];
 for( int i = 1 ; i <= n ; ++i ) {
  best[ i ] = best[ i - 1 ];
  for( int r = 1 ; ( r <= k ) && ( r <= i ) ; ++r )
   best[ i ] = std::max( best[ i ] , pre[ i ] - pre[ i - r ] +
                                     ( i - r >= 1 ? best[ i - r - 1 ] : 0 ) );
  }
 return( best[ n ] );
 }

// the examples of the text (18, 44, 50); against every subset of places
// without k + 1 consecutive ones on 500 random vectors with negative
// values; the empty vector; k >= n takes all the positive items, none if
// all are negative; k = 1 on the largest int, k = 2 on equal items; k = 0
// rejected
ADS_TEST( "esercitazioni-66" )
{
 CHECK( kSubsequence( { 9 , 1 , 9 } , 2 ) == 18 );
 CHECK( kSubsequence( { 1 , 7 , 8 , 9 , 1 , 10 , 6 , 8 , 8 } , 2 ) == 44 );
 CHECK( kSubsequence( { 1 , 7 , 8 , 9 , 1 , 10 , 6 , 8 , 8 } , 3 ) == 50 );
 std::mt19937 g( 66 );
 for( int r = 0 ; r < 500 ; ++r ) {
  const int n = rnd( g , 0 , 14 ) , k = rnd( g , 1 , 4 );
  const auto V = rndVector( g , n , r % 2 ? -5 : 0 , 10 );
  I64 best = 0;
  for( int mask = 0 ; mask < ( 1 << n ) ; ++mask ) {
   const unsigned run = ( 1u << ( k + 1 ) ) - 1;
   bool ok = true;
   for( int i = 0 ; i + k < n ; ++i )
    ok = ok && ( ( unsigned( mask ) >> i & run ) != run );
   if( ! ok ) continue;
   I64 s = 0;
   for( int i = 0 ; i < n ; ++i ) if( ( mask >> i ) & 1 ) s += V[ i ];
   best = std::max( best , s );
   }
  CHECK( kSubsequence( V , k ) == best );
  }
 CHECK( kSubsequence( {} , 1 ) == 0 );
 CHECK( kSubsequence( { 3 , -1 , 4 } , 5 ) == 7 );
 const int M = std::numeric_limits< int >::max();
 CHECK( kSubsequence( { -3 , -1 , -4 } , 2 ) == 0 );
 CHECK( kSubsequence( std::vector< int >( 5 , M ) , 1 ) == I64( 3 ) * M );
 CHECK( kSubsequence( std::vector< int >( 6 , 1 ) , 2 ) == 4 );
 CHECK_THROWS( kSubsequence( { 1 } , 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- 67 - Palindroma -----------------------------*/
/*--------------------------------------------------------------------------*/


/// the fewest insertions that make s[ i ... j ] a palindrome, memoized
int minpalRec( const std::string & s , int i , int j ,
               std::vector< std::vector< int > > & DP )
{
 if( i >= j ) return( 0 );
 if( DP[ i ][ j ] < 0 )
  DP[ i ][ j ] = s[ i ] == s[ j ]
                 ? minpalRec( s , i + 1 , j - 1 , DP )
                 : 1 + std::min( minpalRec( s , i + 1 , j , DP ) ,
                                 minpalRec( s , i , j - 1 , DP ) );
 return( DP[ i ][ j ] );
 }

/// minpal(): the fewest characters to insert into s to make it a
/// palindrome, DP[ i ][ j ] = DP[ i + 1 ][ j - 1 ] if s[ i ] = s[ j ], else 1
/// + min( DP[ i + 1 ][ j ] , DP[ i ][ j - 1 ] ), memoized; Theta( n^2 )
int minpal( const std::string & s )
{
 const int n = int( s.size() );
 std::vector< std::vector< int > > DP( n , std::vector< int >( n , -1 ) );
 return( minpalRec( s , 0 , n - 1 , DP ) );
 }

// the examples of the text (casacca 2, anta 1); against n - | LCS( s ,
// reverse( s ) ) | by lcs() of the library on 500 random strings; the empty
// string, a palindrome (0), all distinct (n - 1), one character, two
// distinct, all equal, the whole alphabet
ADS_TEST( "esercitazioni-67" )
{
 CHECK( minpal( "casacca" ) == 2 && minpal( "anta" ) == 1 );
 std::mt19937 g( 67 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::string s( rnd( g , 0 , 30 ) , 'a' );
  for( auto & c : s ) c = char( 'a' + rnd( g , 0 , r % 2 ? 1 : 5 ) );
  const std::string rs( s.rbegin() , s.rend() );
  CHECK( minpal( s ) == int( s.size() ) - int( lcs( s , rs ).size() ) );
  }
 CHECK( minpal( "" ) == 0 && minpal( "osso" ) == 0 && minpal( "abcde" ) == 4 );
 CHECK( minpal( "x" ) == 0 && minpal( "xy" ) == 1 &&
        minpal( std::string( 300 , 'z' ) ) == 0 );
 std::string az;
 for( char c = 'a' ; c <= 'z' ; ++c ) az += c;
 CHECK( minpal( az ) == 25 && minpal( az + std::string( az.rbegin() ,
                                                         az.rend() ) ) == 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 68 - Quadrato binario (2012-09-10) -------------------*/
/*--------------------------------------------------------------------------*/


/// maxSquare(): the side of the largest square of ones in the 0 / 1 matrix
/// A: DP[ i ][ j ], the side of the largest one with its lower right corner
/// in ( i , j ), is 1 + the least of its three neighbours above and to the
/// left; Theta( n^2 ), largestSquare() of the library
/// \pre A square
int maxSquare( const std::vector< std::vector< int > > & A )
{
 const int n = int( A.size() );
 for( const auto & row : A ) ADS_PRE( int( row.size() ) == n );
 return( int( largestSquare( A ).side ) );
 }

// the matrix of the text (4); against every corner and side on 400 random
// matrices; all zeros, all ones, one cell, the empty matrix, a
// checkerboard (1), a large matrix of ones; a matrix not square rejected
ADS_TEST( "esercitazioni-68" )
{
 CHECK( maxSquare( { { 1 , 0 , 1 , 0 , 1 , 0 , 0 } ,
                    { 1 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                    { 0 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 0 , 0 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 1 , 1 , 1 , 1 , 1 , 0 } ,
                    { 1 , 0 , 1 , 0 , 1 , 0 , 1 } } ) == 4 );
 std::mt19937 g( 68 );
 for( int r = 0 ; r < 400 ; ++r ) {
  const int n = rnd( g , 1 , 10 );
  std::vector< std::vector< int > > A( n , std::vector< int >( n ) );
  for( auto & row : A ) for( auto & x : row ) x = rnd( g , 0 , 5 ) ? 1 : 0;
  int best = 0;
  for( int i = 0 ; i < n ; ++i ) for( int j = 0 ; j < n ; ++j )
   for( int s = 1 ; ( i + s <= n ) && ( j + s <= n ) ; ++s ) {
    bool ok = true;
    for( int a = i ; a < i + s ; ++a ) for( int b = j ; b < j + s ; ++b )
     ok = ok && A[ a ][ b ];
    if( ok ) best = std::max( best , s );
    }
  CHECK( maxSquare( A ) == best );
  }
 using M = std::vector< std::vector< int > >;
 CHECK( maxSquare( M( 5 , std::vector< int >( 5 , 0 ) ) )
        == 0 );
 CHECK( maxSquare( M( 5 , std::vector< int >( 5 , 1 ) ) )
        == 5 );
 CHECK( maxSquare( { { 1 } } ) == 1 && maxSquare( {} ) == 0 );
 CHECK( maxSquare( { { 0 } } ) == 0 );
 M C( 6 , std::vector< int >( 6 ) );
 for( int i = 0 ; i < 6 ; ++i ) for( int j = 0 ; j < 6 ; ++j )
  C[ i ][ j ] = ( i + j ) % 2;
 CHECK( maxSquare( C ) == 1 );
 CHECK( maxSquare( M( 300 , std::vector< int >( 300 , 1 ) ) ) == 300 );
 CHECK_THROWS( maxSquare( { { 1 , 1 } } ) , contract_violation );
 CHECK_THROWS( maxSquare( { { 1 , 1 } , { 1 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 69 - Mosse su scacchiera ------------------------*/
/*--------------------------------------------------------------------------*/


/// searchPath(): the largest profit of a pawn from the bottom row to the top
/// one, moving up, up-left or up-right: the best of each cell is its profit
/// plus the best of its three cells below; Theta( n^2 )
/// \pre P square, n >= 1
I64 searchPath( const std::vector< std::vector< int > > & P )
{
 const int n = int( P.size() );
 ADS_PRE( n >= 1 );
 for( const auto & row : P ) ADS_PRE( int( row.size() ) == n );
 std::vector< I64 > best( P[ n - 1 ].begin() , P[ n - 1 ].end() );
 for( int i = n - 2 ; i >= 0 ; --i ) {
  std::vector< I64 > next( n );
  for( int j = 0 ; j < n ; ++j ) {
   I64 b = best[ j ];
   if( j > 0 ) b = std::max( b , best[ j - 1 ] );
   if( j < n - 1 ) b = std::max( b , best[ j + 1 ] );
   next[ j ] = P[ i ][ j ] + b;
   }
  best = std::move( next );
  }
 return( *std::ranges::max_element( best ) );
 }

/// the best path from the cell ( i , j ) up to the row 0, every one tried
I64 searchBrute( const std::vector< std::vector< int > > & P , int i , int j )
{
 const int n = int( P.size() );
 if( ( j < 0 ) || ( j >= n ) )
  return( std::numeric_limits< I64 >::min() / 2 );
 if( i == 0 ) return( P[ 0 ][ j ] );
 return( P[ i ][ j ] + std::max( { searchBrute( P , i - 1 , j - 1 ) ,
                                   searchBrute( P , i - 1 , j ) ,
                                   searchBrute( P , i - 1 , j + 1 ) } ) );
 }

// the board of the text (33); against every path on 300 random boards with
// negative profits; one cell, the extremes of int (sums beyond int), a
// zig-zag forced by the profits; the empty or not square board rejected
ADS_TEST( "esercitazioni-69" )
{
 CHECK( searchPath( { { 6 , 7 , 4 , 7 , 8 } , { 7 , 6 , 1 , 1 , 4 } ,
                      { 3 , 5 , 7 , 8 , 2 } , { 2 , 6 , 7 , 0 , 2 } ,
                      { 7 , 3 , 5 , 6 , 1 } } ) == 33 );
 std::mt19937 g( 69 );
 for( int r = 0 ; r < 300 ; ++r ) {
  const int n = rnd( g , 1 , 7 );
  std::vector< std::vector< int > > P( n );
  for( auto & row : P ) row = rndVector( g , n , -9 , 9 );
  I64 best = std::numeric_limits< I64 >::min();
  for( int j = 0 ; j < n ; ++j )
   best = std::max( best , searchBrute( P , n - 1 , j ) );
  CHECK( searchPath( P ) == best );
  }
 CHECK( searchPath( { { -4 } } ) == -4 );
 const int M = std::numeric_limits< int >::max();
 using B = std::vector< std::vector< int > >;
 CHECK( searchPath( B( 3 , std::vector< int >( 3 , M ) ) ) == I64( 3 ) * M );
 CHECK( searchPath( B( 3 , std::vector< int >( 3 , -M - 1 ) ) ) ==
        I64( 3 ) * ( -M - 1 ) );
 CHECK( searchPath( { { 9 , 0 , 0 } , { 0 , 9 , 0 } , { 0 , 0 , 9 } } ) ==
        27 );
 CHECK( searchPath( { { 9 , 0 , 0 } , { 0 , 0 , 0 } , { 0 , 0 , 9 } } ) ==
        18 );
 CHECK_THROWS( searchPath( {} ) , contract_violation );
 CHECK_THROWS( searchPath( { { 1 , 2 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- 70 - Codice segreto ---------------------------*/
/*--------------------------------------------------------------------------*/


// the readings of the digits T with A = 1 ... Z = 26, from the right:
// C[ i ] = 0 if T[ i ] = 0, else C[ i + 1 ] plus C[ i + 2 ] if T[ i ]
// T[ i + 1 ] is in 10 ... 26, countDecodings() of the library [see
// Segmentations.h]

/// the readings of T[ i ... ], every split tried
std::uint64_t decodeBrute( const std::vector< int > & T , std::size_t i )
{
 if( i == T.size() ) return( 1 );
 if( T[ i ] == 0 ) return( 0 );
 std::uint64_t c = decodeBrute( T , i + 1 );
 if( ( i + 1 < T.size() ) && ( 10 * T[ i ] + T[ i + 1 ] <= 26 ) )
  c += decodeBrute( T , i + 2 );
 return( c );
 }

// the example of the text (13135, 4); against every split on 1000 random
// strings rich in 0, 1, 2; 0 alone; 10, 27; all ones (Fibonacci); the
// empty string (one reading), 26 against 27, two zeros, a 0 after 10; a
// digit out of 0 ... 9 rejected
ADS_TEST( "esercitazioni-70" )
{
 CHECK( countDecodings( { 1 , 3 , 1 , 3 , 5 } ) == 4 );
 std::mt19937 g( 70 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  std::vector< int > T( rnd( g , 1 , 18 ) );
  for( auto & d : T )
   d = rnd( g , 0 , 3 ) ? rnd( g , 0 , 2 ) : rnd( g , 0 , 9 );
  CHECK( countDecodings( T ) == decodeBrute( T , 0 ) );
  }
 CHECK( countDecodings( { 0 } ) == 0 && countDecodings( { 1 , 0 } ) == 1 );
 CHECK( countDecodings( { 2 , 7 } ) == 1 && countDecodings( { 3 , 0 } ) == 0 );
 CHECK( countDecodings( std::vector< int >( 80 , 1 ) ) == fibonacci( 80 ) );
 CHECK( countDecodings( {} ) == 1 );
 CHECK( countDecodings( { 2 , 6 } ) == 2 && countDecodings( { 0 , 0 } ) == 0 );
 CHECK( countDecodings( { 1 , 0 , 0 } ) == 0 &&
        countDecodings( { 1 , 0 , 1 } ) == 1 );
 CHECK_THROWS( countDecodings( { 1 , 10 } ) , contract_violation );
 CHECK_THROWS( countDecodings( { -1 } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 71 - Ottimizza la somma -------------------------*/
/*--------------------------------------------------------------------------*/


/// bestSum(): the largest X = sum x[ i ] V[ i ] <= W with x[ i ] >= 0
/// integers, and x: the unbounded knapsack with weights = profits = V,
/// DP[ w ] = max( DP[ w - V[ i ] ] + V[ i ] ) over the items that fit;
/// O( n W ) time, O( W ) space
/// \pre V > 0, W >= 0
std::pair< I64 , std::vector< int > > bestSum( const std::vector< int > & V ,
                                                int W )
{
 ADS_PRE( ( W >= 0 ) &&
          std::ranges::all_of( V , []( int v ) { return( v > 0 ); } ) );
 const int n = int( V.size() );
 std::vector< I64 > DP( W + 1 , 0 );
 std::vector< int > last( W + 1 , -1 );            // the item of the choice
 for( int w = 1 ; w <= W ; ++w ) {
  DP[ w ] = DP[ w - 1 ];
  last[ w ] = -2;                                   // the capacity w - 1
  for( int i = 0 ; i < n ; ++i )
   if( ( V[ i ] <= w ) && ( DP[ w - V[ i ] ] + V[ i ] > DP[ w ] ) ) {
    DP[ w ] = DP[ w - V[ i ] ] + V[ i ];
    last[ w ] = i;
    }
  }
 std::vector< int > x( n , 0 );
 for( int w = W ; w > 0 ; )
  if( last[ w ] == -2 ) --w;
  else {
   ++x[ last[ w ] ];
   w -= V[ last[ w ] ];
   }
 return( std::pair( DP[ W ] , x ) );
 }

// the example of the text (W = 17 gives 15, e.g., with x = [ 0 , 2 , 0 ,
// 1 , 0 , 0 ]);
// against the sums reachable by a boolean table on 500 random instances,
// the multipliers returned giving X; W = 0; no item fits; no item at all,
// an item of 1 (X = W), an item equal to W; the preconditions (an item of
// 0 or negative, W negative)
ADS_TEST( "esercitazioni-71" )
{
 const std::vector< int > V{ 18 , 3 , 21 , 9 , 12 , 24 };
 const auto [ X , x ] = bestSum( V , 17 );
 CHECK( X == 15 );
 CHECK( std::inner_product( V.begin() , V.end() , x.begin() , I64( 0 ) ) ==
        15 );
 std::mt19937 g( 71 );
 for( int r = 0 ; r < 500 ; ++r ) {
  std::set< int > S;
  for( int k = rnd( g , 1 , 6 ) ; k > 0 ; --k ) S.insert( rnd( g , 1 , 40 ) );
  const std::vector< int > U( S.begin() , S.end() );
  const int W = rnd( g , 0 , 200 );
  std::vector< bool > reach( W + 1 , false );
  reach[ 0 ] = true;
  for( int w = 1 ; w <= W ; ++w )
   for( int v : U ) if( ( v <= w ) && reach[ w - v ] ) reach[ w ] = true;
  int best = W;
  while( ! reach[ best ] ) --best;
  const auto [ Y , y ] = bestSum( U , W );
  CHECK( Y == best );
  CHECK( std::inner_product( U.begin() , U.end() , y.begin() , I64( 0 ) ) ==
         Y );
  CHECK( std::ranges::all_of( y , []( int c ) { return( c >= 0 ); } ) );
  }
 CHECK( bestSum( V , 0 ).first == 0 && bestSum( V , 2 ).first == 0 );
 CHECK( bestSum( {} , 10 ).first == 0 && bestSum( {} , 10 ).second.empty() );
 const auto [ X1 , x1 ] = bestSum( { 7 , 1 } , 100 );
 CHECK( X1 == 100 && I64( 7 ) * x1[ 0 ] + x1[ 1 ] == 100 );
 CHECK( bestSum( { 13 } , 13 ).first == 13 &&
        bestSum( { 13 } , 13 ).second == std::vector< int >( 1 , 1 ) );
 CHECK_THROWS( bestSum( { 0 } , 5 ) , contract_violation );
 CHECK_THROWS( bestSum( { 3 , -2 } , 5 ) , contract_violation );
 CHECK_THROWS( bestSum( { 3 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- 72 - Vettori ordinati --------------------------*/
/*--------------------------------------------------------------------------*/


/// sortedPerm(): the non-decreasing vectors of length n with values in
/// 1 ... k: DP[ i ][ t ] = DP[ i - 1 ][ t ] + DP[ i ][ t - 1 ], DP[ 1 ][ t ] =
/// t, DP[ i ][ 0 ] = 0; O( n k ); the closed form is binomial( n + k - 1 , n )
/// \pre n >= 1, k >= 0
std::uint64_t sortedPerm( int n , int k )
{
 ADS_PRE( ( n >= 1 ) && ( k >= 0 ) );
 std::vector< std::uint64_t > DP( k + 1 );
 for( int t = 0 ; t <= k ; ++t ) DP[ t ] = std::uint64_t( t );
 for( int i = 2 ; i <= n ; ++i )
  for( int t = 1 ; t <= k ; ++t ) DP[ t ] += DP[ t - 1 ];
 return( DP[ k ] );
 }

// the example of the text (n = 4, k = 3, 15); against tartaglia() of the
// library and against all the k^n vectors for small n and k; k = 0, 1,
// n = 1 (k vectors), the largest pair checked; n = 0 and k < 0 rejected
ADS_TEST( "esercitazioni-72" )
{
 CHECK( sortedPerm( 4 , 3 ) == 15 );
 const auto C = tartaglia( 59 );
 for( int n = 1 ; n <= 30 ; ++n )
  for( int k = 1 ; k <= 30 ; ++k )
   CHECK( sortedPerm( n , k ) == C[ n + k - 1 ][ n ] );
 for( int n = 1 ; n <= 5 ; ++n )
  for( int k = 1 ; k <= 5 ; ++k ) {
   std::uint64_t brute = 0;
   std::vector< int > v( n , 1 );
   while( true ) {
    brute += std::ranges::is_sorted( v );
    int i = 0;
    while( ( i < n ) && ( v[ i ] == k ) ) v[ i++ ] = 1;
    if( i == n ) break;
    ++v[ i ];
    }
   CHECK( sortedPerm( n , k ) == brute );
   }
 CHECK( sortedPerm( 3 , 0 ) == 0 && sortedPerm( 7 , 1 ) == 1 );
 CHECK( sortedPerm( 1 , 0 ) == 0 && sortedPerm( 1 , 1000 ) == 1000 );
 CHECK( sortedPerm( 30 , 30 ) == C[ 59 ][ 30 ] );
 CHECK_THROWS( sortedPerm( 0 , 3 ) , contract_violation );
 CHECK_THROWS( sortedPerm( 3 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------- 74 - Supersequenza comune minimale -------------------*/
/*--------------------------------------------------------------------------*/


// the length of the shortest common supersequence of P and T: DP[ i ][ 0 ]
// = i, DP[ 0 ][ j ] = j, DP[ i - 1 ][ j - 1 ] + 1 if the last characters
// are equal, else 1 + min( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] ), and
// n + m - | LCS |: scsLength() of the library [see SubsequenceProblems.h]

/// whether s is a subsequence of S
bool isSubsequence( const std::string & s , const std::string & S )
{
 std::size_t i = 0;
 for( char c : S ) if( ( i < s.size() ) && ( s[ i ] == c ) ) ++i;
 return( i == s.size() );
 }

// the examples of the text (AB, BC: 3; DAB, DCB: 4); against n + m - | LCS |
// by lcsTable() of the library, and against the shortest of all the strings
// over { a , b } holding both, on 300 random pairs; an empty string, equal
// strings (n), no common character (n + m), one string inside the other
ADS_TEST( "esercitazioni-74" )
{
 CHECK( scsLength( "AB" , "BC" ) == 3 && scsLength( "DAB" , "DCB" ) == 4 );
 std::mt19937 g( 74 );
 for( int r = 0 ; r < 300 ; ++r ) {
  std::string P( rnd( g , 0 , 4 ) , 'a' ) , T( rnd( g , 0 , 4 ) , 'a' );
  for( auto & c : P ) c = char( 'a' + rnd( g , 0 , 1 ) );
  for( auto & c : T ) c = char( 'a' + rnd( g , 0 , 1 ) );
  const int n = int( P.size() ) , m = int( T.size() );
  CHECK( int( scsLength( P , T ) ) == n + m - lcsTable( P , T )[ n ][ m ] );
  int brute = -1;
  for( int len = 0 ; brute < 0 ; ++len )
   for( int mask = 0 ; ( brute < 0 ) && ( mask < ( 1 << len ) ) ; ++mask ) {
    std::string S( len , 'a' );
    for( int i = 0 ; i < len ; ++i ) if( ( mask >> i ) & 1 ) S[ i ] = 'b';
    if( isSubsequence( P , S ) && isSubsequence( T , S ) ) brute = len;
    }
  CHECK( int( scsLength( P , T ) ) == brute );
  }
 CHECK( scsLength( "" , "abc" ) == 3 && scsLength( "" , "" ) == 0 );
 CHECK( scsLength( "abc" , "" ) == 3 && scsLength( "abc" , "abc" ) == 3 );
 CHECK( scsLength( "abc" , "xyz" ) == 6 && scsLength( "bd" , "abcde" ) == 5 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ 76 - Sequenza specchiata ------------------------*/
/*--------------------------------------------------------------------------*/


/// longestBalancedBlock(): the longest contiguous block of k >= 1 zeros
/// followed by k ones, 0 if none: during a run of ones, 2 min( zeros of the
/// run before , ones so far ) is the best block ending there; Theta( n )
/// time, O( 1 ) space
/// \pre S of 0 and 1
int longestBalancedBlock( const std::vector< int > & S )
{
 ADS_PRE( std::ranges::all_of( S , []( int x ) {
                                return( ( x == 0 ) || ( x == 1 ) ); } ) );
 int zeros = 0 , ones = 0 , best = 0;
 for( int x : S )
  if( x == 0 ) {
   if( ones > 0 ) zeros = ones = 0;
   ++zeros;
   }
  else {
   ++ones;
   best = std::max( best , 2 * std::min( zeros , ones ) );
   }
 return( best );
 }

// the example of the text (6); against every start and k on 1000 random
// vectors; all zeros, all ones (0), 01 (2), the empty vector, ones before
// zeros (0), unequal runs (the shorter), one long block and a long
// alternation; a value other than 0 and 1 rejected
ADS_TEST( "esercitazioni-76" )
{
 CHECK( longestBalancedBlock( { 0 , 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 ,
                               0 , 1 ,
                                1 , 1 , 1 , 0 , 0 , 0 , 0 } ) == 6 );
 std::mt19937 g( 76 );
 for( int r = 0 ; r < 1000 ; ++r ) {
  const auto S = rndVector( g , rnd( g , 0 , 25 ) , 0 , 1 );
  const int n = int( S.size() );
  int best = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int k = 1 ; i + 2 * k <= n ; ++k ) {
    bool ok = true;
    for( int t = 0 ; t < k ; ++t )
     ok = ok && ( S[ i + t ] == 0 ) && ( S[ i + k + t ] == 1 );
    if( ok ) best = std::max( best , 2 * k );
    }
  CHECK( longestBalancedBlock( S ) == best );
  }
 CHECK( longestBalancedBlock( { 0 , 0 } ) == 0 );
 CHECK( longestBalancedBlock( { 1 } ) == 0 );
 CHECK( longestBalancedBlock( { 0 , 1 } ) == 2 );
 CHECK( longestBalancedBlock( {} ) == 0 );
 CHECK( longestBalancedBlock( { 1 , 1 , 0 , 0 } ) == 0 );
 CHECK( longestBalancedBlock( { 0 , 0 , 0 , 1 } ) == 2 &&
        longestBalancedBlock( { 0 , 1 , 1 , 1 } ) == 2 );
 std::vector< int > block( 100000 , 1 ) , alt( 100000 );
 std::fill( block.begin() , block.begin() + 50000 , 0 );
 for( int i = 0 ; i < 100000 ; ++i ) alt[ i ] = i % 2;
 CHECK( longestBalancedBlock( block ) == 100000 &&
        longestBalancedBlock( alt ) == 2 );
 CHECK_THROWS( longestBalancedBlock( { 0 , 2 } ) , contract_violation );
 CHECK_THROWS( longestBalancedBlock( { -1 } ) , contract_violation );
 }

}  // end( namespace esercitazioni )

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File exam_esercitazionib.cpp --------------------*/
/*--------------------------------------------------------------------------*/
