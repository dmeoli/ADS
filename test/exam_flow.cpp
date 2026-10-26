/*--------------------------------------------------------------------------*/
/*--------------------------- File exam_flow.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the flow networks of the exams: each problem of the collection
 * that asks for an algorithm is solved by the reduction to a maximum flow
 * of its official solution, on FlowNetwork of MaxFlow.h (or on
 * bipartiteMatching() of EasyCases.h for the matchings), and checked on
 * the examples of the text, against a brute force on small instances with
 * a fixed seed, and on the edge cases. A test is named after the number of
 * the problem in the collection; Fratelli Eris (10) is the case k = 2 of
 * Cammini disgiunti (3) and shares its test, and the four parts of Archi
 * critici (12) ask for proofs and have none.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/complexity/EasyCases.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MaxFlow.h"

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;
using Pairs = std::vector< std::pair< int , int > >;
using Arc = std::tuple< int , int , long >;
using Bools = std::vector< std::vector< bool > >;
using Matrix = std::vector< Longs >;

/// a capacity that no cut of the tests reaches, the + inf of the texts
const long INF = std::numeric_limits< int >::max();

const Augment ways[] = { Augment::FORD_FULKERSON , Augment::EDMONDS_KARP ,
                         Augment::DINIC };

/// a uniform integer in [ a , b ]
int rnd( std::mt19937 & g , int a , int b )
{
 return( std::uniform_int_distribution< int >( a , b )( g ) );
 }

/// a random subset of [ 0 , n ), each element with probability 1 / q
Ints subset( std::mt19937 & g , int n , int q = 2 )
{
 Ints S;
 for( int i = 0 ; i < n ; ++i )
  if( rnd( g , 1 , q ) == 1 ) S.push_back( i );
 return( S );
 }

/// whether x is in S
bool in( const Ints & S , int x )
{
 return( std::find( S.begin() , S.end() , x ) != S.end() );
 }

/// calls f on every a with 0 <= a[ i ] < r[ i ], on none if an r[ i ] <= 0
template< class F >
void everyChoice( const Ints & r , F f )
{
 if( std::any_of( r.begin() , r.end() , []( int x ) { return( x <= 0 ); } ) )
  return;
 Ints a( r.size() , 0 );
 for( ; ; ) {
  f( a );
  std::size_t i = 0;
  while( ( i < a.size() ) && ( ++a[ i ] == r[ i ] ) )
   a[ i++ ] = 0;
  if( i == a.size() )
   return;
  }
 }

/// the maximum flow of the arcs from s to t
long maxflow( int n , const std::vector< Arc > & A , int s , int t ,
              Augment how = Augment::DINIC )
{
 FlowNetwork N( n );
 for( auto [ u , v , c ] : A )
  N.addEdge( u , v , c );
 return( N.maxFlow( s , t , how ) );
 }

/// the least capacity of a cut ( S , V - S ), s in S and t not, enumerated
long minCut( int n , const std::vector< Arc > & A , int s , int t )
{
 long best = -1;
 for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
  if( ! ( S >> s & 1 ) || ( S >> t & 1 ) )
   continue;
  long c = 0;
  for( auto [ u , v , w ] : A )
   if( ( S >> u & 1 ) && ! ( S >> v & 1 ) )
    c += w;
  if( ( best < 0 ) || ( c < best ) )
   best = c;
  }
 return( best );
 }

/*--------------------------------------------------------------------------*/
/*------------------- flow-1, 2011-07-18, Swimming pool --------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// a child at the course of a slot ( day , hour ), with its instructor
struct Lesson { int child , slot , teacher; };

/// Swimming pool: a course of one hour at each slot of days x hours; child
/// i attends at most weekly courses, one a day, at the slots pref[ i ]; a
/// course holds at most seats children and needs an instructor j
/// available at its slot, avail[ j ]; each lesson pays 10 euro. A maximum
/// flow S -> B_i ( weekly ) -> G_{i,d} ( 1 , one a day ) -> C_o ( 1 , a
/// preferred slot ) -> C'_o ( seats ) -> I_j ( seats , available ) -> P
/// ( + inf ), a lesson per unit: O( |f*| m ), |f*| <= weekly n, m = O( ( n
/// + k ) days hours ). The text joins C_o to each I_j with capacity seats,
/// so that a course with two instructors takes 2 seats children: the node
/// C'_o limits the course to seats, and it gets one of its instructors.
/// \pre pref[ i ] and avail[ j ] of days hours slots, seats , weekly >= 0
std::vector< Lesson > swimmingPool( const Bools & pref , const Bools & avail ,
                                    int days , int hours , long seats = 6 ,
                                    long weekly = 5 )
{
 const int n = int( pref.size() ) , k = int( avail.size() );
 const int H = days * hours;
 ADS_PRE( ( days >= 0 ) && ( hours >= 0 ) && ( seats >= 0 ) &&
          ( weekly >= 0 ) );
 ADS_PRE( std::all_of( pref.begin() , pref.end() , [ & ]( const auto & p ) {
  return( int( p.size() ) == H ); } ) );
 ADS_PRE( std::all_of( avail.begin() , avail.end() , [ & ]( const auto & a ) {
  return( int( a.size() ) == H ); } ) );
 const int s = 0 , t = 1 , B = 2 , G = B + n , C = G + n * days;
 const int C2 = C + H , I = C2 + H;
 FlowNetwork N( I + k );
 std::vector< std::tuple< int , int , int > > attend , teach;
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , B + i , weekly );
  for( int d = 0 ; d < days ; ++d ) {
   N.addEdge( B + i , G + i * days + d , 1 );
   for( int o = d * hours ; o < ( d + 1 ) * hours ; ++o )
    if( pref[ i ][ o ] )
     attend.emplace_back( N.addEdge( G + i * days + d , C + o , 1 ) , i , o );
   }
  }
 for( int o = 0 ; o < H ; ++o ) {
  N.addEdge( C + o , C2 + o , seats );
  for( int j = 0 ; j < k ; ++j )
   if( avail[ j ][ o ] )
    teach.emplace_back( N.addEdge( C2 + o , I + j , seats ) , o , j );
  }
 for( int j = 0 ; j < k ; ++j )
  N.addEdge( I + j , t , INF );
 N.maxFlow( s , t );
 Ints teacher( H , -1 );
 for( auto [ e , o , j ] : teach )
  if( ( N.flow( e ) > 0 ) && ( teacher[ o ] < 0 ) )
   teacher[ o ] = j;
 std::vector< Lesson > L;
 for( auto [ e , i , o ] : attend )
  if( N.flow( e ) > 0 )
   L.push_back( Lesson{ i , o , teacher[ o ] } );
 return( L );
 }

}  // end( namespace flow )

/// whether the lessons respect the rules of Swimming pool
bool lessonsOk( const Bools & pref , const Bools & avail , int days ,
                int hours , long seats , long weekly ,
                const std::vector< flow::Lesson > & L )
{
 const int n = int( pref.size() ) , H = days * hours;
 Longs week( n , 0 ) , seat( H , 0 );
 Ints day( n * days , 0 ) , teacher( H , -1 );
 for( auto [ i , o , j ] : L ) {
  if( ! pref[ i ][ o ] || ( j < 0 ) || ! avail[ j ][ o ] )
   return( false );
  if( ( teacher[ o ] >= 0 ) && ( teacher[ o ] != j ) )
   return( false );
  teacher[ o ] = j;
  if( ( ++week[ i ] > weekly ) || ( ++day[ i * days + o / hours ] > 1 ) ||
      ( ++seat[ o ] > seats ) )
   return( false );
  }
 return( true );
 }

/// the most lessons of Swimming pool, each child choosing an hour or none
/// on each day
long lessonsBrute( const Bools & pref , const Bools & avail , int days ,
                   int hours , long seats , long weekly )
{
 const int n = int( pref.size() ) , H = days * hours;
 Longs cap( H , 0 );
 for( int o = 0 ; o < H ; ++o )
  for( const auto & a : avail )
   if( a[ o ] )
    cap[ o ] = seats;
 long best = 0;
 everyChoice( Ints( n * days , hours + 1 ) , [ & ]( const Ints & a ) {
  Longs week( n , 0 ) , seat( H , 0 );
  long c = 0;
  for( int i = 0 ; i < n ; ++i )
   for( int d = 0 ; d < days ; ++d ) {
    const int h = a[ i * days + d ] - 1;
    if( h < 0 )
     continue;
    const int o = d * hours + h;
    if( ! pref[ i ][ o ] || ( ++week[ i ] > weekly ) ||
        ( ++seat[ o ] > cap[ o ] ) )
     return;
    ++c;
    }
  best = std::max( best , c );
  } );
 return( best );
 }

ADS_TEST( "flow-1" )
{
 // the sizes of the text: 7 days of 10 hours, 6 seats, 5 courses a week
 const int D = 7 , O = 10;
 {
  const Bools pref( 3 , std::vector< bool >( D * O , true ) );
  const Bools avail( 1 , std::vector< bool >( D * O , true ) );
  const auto L = flow::swimmingPool( pref , avail , D , O );
  CHECK( 10 * L.size() == 150 );
  CHECK( lessonsOk( pref , avail , D , O , 6 , 5 , L ) );
  }
 {
  // twenty children for the Monday at 9, two instructors there: one course
  // of 6, not two
  Bools pref( 20 , std::vector< bool >( D * O , false ) );
  for( auto & p : pref )
   p[ 0 ] = true;
  const Bools avail( 2 , std::vector< bool >( D * O , true ) );
  const auto L = flow::swimmingPool( pref , avail , D , O );
  CHECK( L.size() == 6 );
  CHECK( lessonsOk( pref , avail , D , O , 6 , 5 , L ) );
  CHECK( flow::swimmingPool( pref , Bools() , D , O ).empty() );
  CHECK( flow::swimmingPool( Bools() , avail , D , O ).empty() );
  // no seat, no course a week, no slot: no lesson
  CHECK( flow::swimmingPool( pref , avail , D , O , 0 ).empty() );
  CHECK( flow::swimmingPool( pref , avail , D , O , 6 , 0 ).empty() );
  const Bools none( 2 , std::vector< bool >() );
  CHECK( flow::swimmingPool( none , none , 0 , O ).empty() );
  CHECK( flow::swimmingPool( none , none , D , 0 ).empty() );
  }
 {
  // a child free all week and keen on 9 courses: one a day, 7
  const Bools pref( 1 , std::vector< bool >( D * O , true ) );
  const auto L = flow::swimmingPool( pref , pref , D , O , 6 , 9 );
  CHECK( L.size() == std::size_t( D ) );
  CHECK( lessonsOk( pref , pref , D , O , 6 , 9 , L ) );
  // the contracts: the sizes >= 0, the rows of days hours slots
  CHECK_THROWS( flow::swimmingPool( pref , pref , -1 , O ) ,
                contract_violation );
  CHECK_THROWS( flow::swimmingPool( pref , pref , D , -1 ) ,
                contract_violation );
  CHECK_THROWS( flow::swimmingPool( pref , pref , D , O , -1 ) ,
                contract_violation );
  CHECK_THROWS( flow::swimmingPool( pref , pref , D , O , 6 , -1 ) ,
                contract_violation );
  CHECK_THROWS( flow::swimmingPool( pref , pref , D , O - 1 ) ,
                contract_violation );
  CHECK_THROWS( flow::swimmingPool( Bools() , pref , D , O - 1 ) ,
                contract_violation );
  }
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int days = rnd( g , 1 , 2 ) , hours = rnd( g , 1 , 2 );
  const int n = rnd( g , 0 , 3 ) , k = rnd( g , 0 , 2 );
  const long seats = rnd( g , 0 , 2 ) , weekly = rnd( g , 0 , 2 );
  Bools pref( n , std::vector< bool >( days * hours ) );
  Bools avail( k , std::vector< bool >( days * hours ) );
  for( auto & p : pref )
   for( int o = 0 ; o < days * hours ; ++o )
    p[ o ] = rnd( g , 0 , 2 ) > 0;
  for( auto & a : avail )
   for( int o = 0 ; o < days * hours ; ++o )
    a[ o ] = rnd( g , 0 , 1 ) > 0;
  const auto L = flow::swimmingPool( pref , avail , days , hours , seats ,
                                     weekly );
  CHECK( lessonsOk( pref , avail , days , hours , seats , weekly , L ) );
  CHECK( long( L.size() ) ==
         lessonsBrute( pref , avail , days , hours , seats , weekly ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- flow-2, 2012-05-31, Matricola ----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Matricola: the network of the figure, the nodes A , ... , F as 0 , ... ,
/// 5, the capacities from the digits x[ 0 ] , ... , x[ 5 ] of a student
/// number, the most significant first. The figure is partly lost: the
/// edges B -> C ( 8 + x[ 1 ] ), B -> E ( 11 + x[ 0 ] ), C -> D ( 7 + x[ 3 ] )
/// and D -> E ( 5 + x[ 5 ] ) are read from where their labels stand,
/// directed from the source towards the sink.
/// \pre the digits in [ 0 , 9 ]
std::vector< Arc > matricolaNet( const std::array< int , 6 > & x )
{
 ADS_PRE( std::all_of( x.begin() , x.end() , []( int d ) {
  return( ( d >= 0 ) && ( d <= 9 ) ); } ) );
 return( std::vector< Arc >{ { 0 , 1 , 35 } , { 0 , 2 , 25 } ,
                             { 1 , 3 , 5 + x[ 2 ] } , { 1 , 4 , 11 + x[ 0 ] } ,
                             { 1 , 2 , 8 + x[ 1 ] } , { 2 , 3 , 7 + x[ 3 ] } ,
                             { 2 , 4 , 6 + x[ 4 ] } , { 3 , 4 , 5 + x[ 5 ] } ,
                             { 3 , 5 , 25 } , { 4 , 5 , 35 } } );
 }

/// the maximum flow from A to F of the network of the digits x
/// \pre the digits in [ 0 , 9 ]
long matricola( const std::array< int , 6 > & x ,
                Augment how = Augment::DINIC )
{
 return( maxflow( 6 , matricolaNet( x ) , 0 , 5 , how ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-2" )
{
 // by hand: the cut { A , B , C } of the edges out of B and C is the least
 CHECK( flow::matricola( { 1 , 2 , 3 , 4 , 5 , 6 } ) == 42 );
 CHECK( flow::matricola( { 0 , 0 , 0 , 0 , 0 , 0 } ) == 29 );
 // the cuts around A and around F, of 60, bind the largest digits
 CHECK( flow::matricola( { 9 , 9 , 9 , 9 , 9 , 9 } ) == 60 );
 // the contracts: a digit out of [ 0 , 9 ]
 CHECK_THROWS( flow::matricola( { 0 , 0 , 0 , 0 , 0 , 10 } ) ,
               contract_violation );
 CHECK_THROWS( flow::matricola( { -1 , 0 , 0 , 0 , 0 , 0 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 2000 ; ++it ) {
  std::array< int , 6 > x;
  for( auto & d : x )
   d = rnd( g , 0 , 9 );
  const long f = flow::matricola( x );
  for( auto how : ways )
   CHECK( flow::matricola( x , how ) == f );
  CHECK( minCut( 6 , flow::matricolaNet( x ) , 0 , 5 ) == f );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-3, 2013-06-17, Cammini disgiunti ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Cammini disgiunti: whether k paths from u to v with no common edge
/// exist in the digraph of n nodes and edges E. By Menger, the maximum
/// flow from u to v with unit capacities: edgeDisjointPaths() of the
/// library, a source joined to u with capacity k stopping it at k paths,
/// O( k ( n + m ) ) by augmenting paths. Fratelli Eris (2016-01-26) is the
/// case k = 2 on a map, each street an edge in each direction.
/// \pre u and v distinct nodes, k >= 0, the edges join nodes
bool disjointPaths( int n , const Pairs & E , int u , int v , long k )
{
 ADS_PRE( std::all_of( E.begin() , E.end() , [ & ]( const auto & e ) {
  return( ( e.first >= 0 ) && ( e.first < n ) && ( e.second >= 0 ) &&
          ( e.second < n ) ); } ) );
 std::vector< std::tuple< int , int , std::monostate > > T;
 for( auto [ a , b ] : E )                     // a loop is on no path
  if( a != b )
   T.emplace_back( a , b , std::monostate() );
 return( long( edgeDisjointPaths( ArrayGraph<>( n , T ) , u , v ,
                                  k ).size() ) >= k );
 }

}  // end( namespace flow )

/// the simple paths from x to v, as masks of the edges, extending mask
void simplePaths( const Pairs & E , int x , int v ,
                  std::uint64_t mask , std::vector< bool > & seen ,
                  std::vector< std::uint64_t > & P )
{
 if( x == v ) {
  P.push_back( mask );
  return;
  }
 seen[ x ] = true;
 for( std::size_t e = 0 ; e < E.size() ; ++e )
  if( ( E[ e ].first == x ) && ! seen[ E[ e ].second ] )
   simplePaths( E , E[ e ].second , v , mask | std::uint64_t( 1 ) << e ,
                seen , P );
 seen[ x ] = false;
 }

/// the most pairwise disjoint masks among P[ i ] , ... , none in used
int packing( const std::vector< std::uint64_t > & P , std::size_t i ,
             std::uint64_t used )
{
 if( i == P.size() )
  return( 0 );
 int best = packing( P , i + 1 , used );
 if( ! ( P[ i ] & used ) )
  best = std::max( best , 1 + packing( P , i + 1 , used | P[ i ] ) );
 return( best );
 }

/// the most paths from u to v with no common edge, by packing the simple
/// paths
int disjointBrute( int n , const Pairs & E , int u , int v )
{
 std::vector< bool > seen( n , false );
 std::vector< std::uint64_t > P;
 simplePaths( E , u , v , 0 , seen , P );
 return( packing( P , 0 , 0 ) );
 }

/// a random digraph of n nodes, no loop, each edge with probability 1 / q
Pairs randomDigraph( std::mt19937 & g , int n , int q )
{
 Pairs E;
 for( int a = 0 ; a < n ; ++a )
  for( int b = 0 ; b < n ; ++b )
   if( ( a != b ) && ( rnd( g , 1 , q ) == 1 ) )
    E.emplace_back( a , b );
 return( E );
 }

ADS_TEST( "flow-3" )
{
 // the complete digraph of 5 nodes: the 4 edges out of u
 Pairs K;
 for( int a = 0 ; a < 5 ; ++a )
  for( int b = 0 ; b < 5 ; ++b )
   if( a != b )
    K.emplace_back( a , b );
 CHECK( flow::disjointPaths( 5 , K , 0 , 4 , 4 ) );
 CHECK( ! flow::disjointPaths( 5 , K , 0 , 4 , 5 ) );
 CHECK( flow::disjointPaths( 2 , Pairs() , 0 , 1 , 0 ) );
 CHECK( ! flow::disjointPaths( 2 , Pairs() , 0 , 1 , 1 ) );
 // Fratelli Eris: a square of streets, home 0 and school 2, has two routes;
 // a street with a dead end in the middle has one
 const Pairs square = { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } , { 2 , 1 } ,
                        { 2 , 3 } , { 3 , 2 } , { 3 , 0 } , { 0 , 3 } };
 CHECK( flow::disjointPaths( 4 , square , 0 , 2 , 2 ) );
 const Pairs road = { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } , { 2 , 1 } ,
                      { 1 , 3 } , { 3 , 1 } };
 CHECK( ! flow::disjointPaths( 4 , road , 0 , 2 , 2 ) );
 // parallel edges are distinct paths, a loop and a reversed edge none
 CHECK( flow::disjointPaths( 2 , { { 0 , 1 } , { 0 , 1 } } , 0 , 1 , 2 ) );
 CHECK( ! flow::disjointPaths( 2 , { { 0 , 1 } , { 0 , 1 } } , 0 , 1 , 3 ) );
 CHECK( ! flow::disjointPaths( 2 , { { 0 , 0 } , { 1 , 1 } , { 1 , 0 } } , 0 ,
                               1 , 1 ) );
 CHECK( flow::disjointPaths( 2 , { { 1 , 0 } } , 1 , 0 , 1 ) );
 // the contracts: an edge out of the nodes, u = v, k < 0
 CHECK_THROWS( flow::disjointPaths( 2 , { { 0 , 2 } } , 0 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::disjointPaths( 2 , { { -1 , 1 } } , 0 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::disjointPaths( 2 , { { 0 , 1 } } , 0 , 0 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::disjointPaths( 2 , { { 0 , 1 } } , 0 , 2 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::disjointPaths( 2 , { { 0 , 1 } } , 0 , 1 , -1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 2 , 5 );
  const Pairs E = randomDigraph( g , n , rnd( g , 1 , 3 ) );
  const int u = rnd( g , 0 , n - 1 ) , v = ( u + rnd( g , 1 , n - 1 ) ) % n;
  const int b = disjointBrute( n , E , u , v );
  for( int k = 0 ; k <= n ; ++k )
   CHECK( flow::disjointPaths( n , E , u , v , k ) == ( k <= b ) );
  }
 // Menger on larger digraphs: the least cut of the edges out of a set
 for( int it = 0 ; it < 200 ; ++it ) {
  const int n = rnd( g , 2 , 8 );
  const Pairs E = randomDigraph( g , n , rnd( g , 1 , 4 ) );
  std::vector< Arc > A;
  for( auto [ a , b ] : E )
   A.emplace_back( a , b , 1 );
  const long c = minCut( n , A , 0 , n - 1 );
  CHECK( flow::disjointPaths( n , E , 0 , n - 1 , c ) );
  CHECK( ! flow::disjointPaths( n , E , 0 , n - 1 , c + 1 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-4, 2013-07-22, Ballo di fine anno -----------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Ballo di fine anno: n boys, m girls, the registered pairs P ( boy , girl
/// ); a pair dances at most pair times, a student at most dancer times.
/// A maximum flow s -> boy ( dancer ) -> girl ( pair , registered ) -> p (
/// dancer ) gives the dances of each pair, O( ( n + m + k ) |f*| ), |f*| <=
/// dancer min( n , m ).
/// \pre the pairs join [ 0 , n ) to [ 0 , m ), pair , dancer >= 0
Longs prom( int n , int m , const Pairs & P , long pair = 3 ,
            long dancer = 10 )
{
 ADS_PRE( ( n >= 0 ) && ( m >= 0 ) && ( pair >= 0 ) && ( dancer >= 0 ) );
 ADS_PRE( std::all_of( P.begin() , P.end() , [ & ]( const auto & e ) {
  return( ( e.first >= 0 ) && ( e.first < n ) && ( e.second >= 0 ) &&
          ( e.second < m ) ); } ) );
 FlowNetwork N( n + m + 2 );
 const int s = n + m , t = s + 1;
 for( int i = 0 ; i < n ; ++i )
  N.addEdge( s , i , dancer );
 for( int j = 0 ; j < m ; ++j )
  N.addEdge( n + j , t , dancer );
 Ints e;
 for( auto [ i , j ] : P )
  e.push_back( N.addEdge( i , n + j , pair ) );
 N.maxFlow( s , t );
 Longs x;
 for( int k : e )
  x.push_back( N.flow( k ) );
 return( x );
 }

}  // end( namespace flow )

/// whether the dances x of the pairs P respect the limits
bool danceOk( int n , int m , const Pairs & P , long pair , long dancer ,
              const Longs & x )
{
 Longs b( n , 0 ) , w( m , 0 );
 for( std::size_t k = 0 ; k < P.size() ; ++k ) {
  if( ( x[ k ] < 0 ) || ( x[ k ] > pair ) )
   return( false );
  b[ P[ k ].first ] += x[ k ];
  w[ P[ k ].second ] += x[ k ];
  }
 return( std::all_of( b.begin() , b.end() , [ & ]( long y ) {
          return( y <= dancer ); } ) &&
         std::all_of( w.begin() , w.end() , [ & ]( long y ) {
          return( y <= dancer ); } ) );
 }

ADS_TEST( "flow-4" )
{
 CHECK( flow::prom( 2 , 2 , Pairs() ).empty() );
 CHECK( flow::prom( 1 , 1 , { { 0 , 0 } } ) == Longs{ 3 } );
 // a boy registered with four girls dances 10 times, not 12
 const Pairs four = { { 0 , 0 } , { 0 , 1 } , { 0 , 2 } , { 0 , 3 } };
 const Longs x = flow::prom( 1 , 4 , four );
 CHECK( std::accumulate( x.begin() , x.end() , 0L ) == 10 );
 CHECK( danceOk( 1 , 4 , four , 3 , 10 , x ) );
 // no dance per pair or per student; a pair registered twice dances twice
 // as much, within the limit of the students
 CHECK( flow::prom( 1 , 4 , four , 0 ) == Longs( 4 , 0 ) );
 CHECK( flow::prom( 1 , 4 , four , 3 , 0 ) == Longs( 4 , 0 ) );
 CHECK( flow::prom( 1 , 1 , { { 0 , 0 } , { 0 , 0 } } ) == Longs( 2 , 3 ) );
 const Longs y = flow::prom( 1 , 1 , { { 0 , 0 } , { 0 , 0 } } , 3 , 4 );
 CHECK( y[ 0 ] + y[ 1 ] == 4 );
 // the contracts: the sizes and the limits >= 0, the pairs in range
 CHECK_THROWS( flow::prom( -1 , 1 , {} ) , contract_violation );
 CHECK_THROWS( flow::prom( 1 , -1 , {} ) , contract_violation );
 CHECK_THROWS( flow::prom( 1 , 1 , {} , -1 ) , contract_violation );
 CHECK_THROWS( flow::prom( 1 , 1 , {} , 3 , -1 ) , contract_violation );
 CHECK_THROWS( flow::prom( 1 , 1 , { { 1 , 0 } } ) , contract_violation );
 CHECK_THROWS( flow::prom( 1 , 1 , { { 0 , 1 } } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 3 ) , m = rnd( g , 0 , 3 );
  const bool text = it % 2 == 0;
  const long pair = text ? 3 : rnd( g , 0 , 2 );
  const long dancer = text ? 10 : rnd( g , 0 , 4 );
  Pairs P;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < m ; ++j )
    if( rnd( g , 0 , 1 ) && ( P.size() < 6 ) )
     P.emplace_back( i , j );
  const Longs y = flow::prom( n , m , P , pair , dancer );
  CHECK( danceOk( n , m , P , pair , dancer , y ) );
  long best = 0;
  everyChoice( Ints( P.size() , int( pair ) + 1 ) , [ & ]( const Ints & a ) {
   const Longs z( a.begin() , a.end() );
   if( danceOk( n , m , P , pair , dancer , z ) )
    best = std::max( best , std::accumulate( z.begin() , z.end() , 0L ) );
   } );
  CHECK( std::accumulate( y.begin() , y.end() , 0L ) == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ flow-5, 2014-07-21, Backup wireless -------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Backup wireless: n sensors at ( x[ i ] , y[ i ] ), two of them in
/// contact if closer than d; whether each sensor can keep its data on k
/// other sensors in contact, each one keeping the data of at most b
/// others. A maximum flow s -> i ( k ) -> j' ( 1 , i != j in contact ) ->
/// p ( b ): feasible if it is k n, O( k n^3 ).
/// \pre x and y of the same size, d , k , b >= 0
bool backup( const Longs & x , const Longs & y , long d , long k , long b )
{
 const int n = int( x.size() );
 ADS_PRE( ( y.size() == x.size() ) && ( d >= 0 ) && ( k >= 0 ) &&
          ( b >= 0 ) );
 FlowNetwork N( 2 * n + 2 );
 const int s = 2 * n , t = s + 1;
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , i , k );
  N.addEdge( n + i , t , b );
  for( int j = 0 ; j < n ; ++j ) {
   const long dx = x[ i ] - x[ j ] , dy = y[ i ] - y[ j ];
   if( ( i != j ) && ( dx * dx + dy * dy < d * d ) )
    N.addEdge( i , n + j , 1 );
   }
  }
 return( N.maxFlow( s , t ) == k * n );
 }

}  // end( namespace flow )

ADS_TEST( "flow-5" )
{
 CHECK( flow::backup( Longs() , Longs() , 1 , 3 , 1 ) );
 CHECK( flow::backup( { 0 } , { 0 } , 5 , 0 , 0 ) );
 CHECK( ! flow::backup( { 0 } , { 0 } , 5 , 1 , 1 ) );
 // three sensors in a row at distance 1: the middle one keeps two copies
 CHECK( flow::backup( { 0 , 1 , 2 } , { 0 , 0 , 0 } , 2 , 1 , 2 ) );
 CHECK( ! flow::backup( { 0 , 1 , 2 } , { 0 , 0 , 0 } , 2 , 1 , 1 ) );
 // the distance must be less than d
 CHECK( ! flow::backup( { 0 , 1 } , { 0 , 0 } , 1 , 1 , 1 ) );
 // d = 0 joins none; two sensors at the same place are in contact; far
 // sensors whose squared distance is near 2^31 (long may be of 32 bits)
 CHECK( ! flow::backup( { 0 , 0 } , { 0 , 0 } , 0 , 1 , 1 ) );
 CHECK( flow::backup( { 0 , 0 } , { 0 , 0 } , 0 , 0 , 0 ) );
 CHECK( flow::backup( { 5 , 5 } , { 7 , 7 } , 1 , 1 , 1 ) );
 CHECK( flow::backup( { 0 , 46000 } , { 0 , 0 } , 46001 , 1 , 1 ) );
 CHECK( ! flow::backup( { 0 , 46000 } , { 0 , 0 } , 46000 , 1 , 1 ) );
 // the complete contacts of 4 sensors: k = 3 needs b = 3
 const Longs z( 4 , 0 );
 CHECK( flow::backup( z , z , 1 , 3 , 3 ) );
 CHECK( ! flow::backup( z , z , 1 , 3 , 2 ) );
 CHECK( ! flow::backup( z , z , 1 , 4 , 4 ) );
 // the contracts: x and y of the same size, d , k , b >= 0
 CHECK_THROWS( flow::backup( { 0 } , {} , 1 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( flow::backup( z , z , -1 , 0 , 0 ) , contract_violation );
 CHECK_THROWS( flow::backup( z , z , 1 , -1 , 0 ) , contract_violation );
 CHECK_THROWS( flow::backup( z , z , 1 , 0 , -1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 4 );
  Longs x( n ) , y( n );
  for( int i = 0 ; i < n ; ++i ) {
   x[ i ] = rnd( g , 0 , 3 );
   y[ i ] = rnd( g , 0 , 3 );
   }
  const long d = rnd( g , 1 , 4 ) , k = rnd( g , 0 , 2 );
const long b = rnd( g , 0 , 2 );
  // each ordered pair ( i , j ) is a copy or not
  bool any = false;
  everyChoice( Ints( n * n , 2 ) , [ & ]( const Ints & a ) {
   if( any )
    return;
   Longs out( n , 0 ) , kept( n , 0 );
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j )
     if( a[ i * n + j ] ) {
      const long dx = x[ i ] - x[ j ] , dy = y[ i ] - y[ j ];
      if( ( i == j ) || ( dx * dx + dy * dy >= d * d ) )
       return;
      ++out[ i ];
      ++kept[ j ];
      }
   for( int i = 0 ; i < n ; ++i )
    if( ( out[ i ] != k ) || ( kept[ i ] > b ) )
     return;
   any = true;
   } );
  CHECK( flow::backup( x , y , d , k , b ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------- flow-6, 2014-12-19, Non contenti del (Regolamento 15/16) --------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Non contenti del: n courses, the requirements ( m_i , S_i ), m_i courses
/// out of S_i, each course counting for one requirement at most, t courses
/// in all. A maximum flow s -> R_i ( m_i ) -> c ( 1 , c in S_i ) -> p ( 1 ):
/// the rules hold if it is t, which needs t = sum m_i, O( t n k ).
/// \pre the courses of the S_i in [ 0 , n ), m_i >= 0
bool requirements( int n , const std::vector< std::pair< long , Ints > > & R ,
                   long t )
{
 const int k = int( R.size() );
 ADS_PRE( n >= 0 );
 ADS_PRE( std::all_of( R.begin() , R.end() , [ & ]( const auto & r ) {
  return( ( r.first >= 0 ) && std::all_of( r.second.begin() ,
          r.second.end() , [ & ]( int c ) {
           return( ( c >= 0 ) && ( c < n ) ); } ) ); } ) );
 FlowNetwork N( n + k + 2 );
 const int s = n + k , p = s + 1;
 long M = 0;
 for( int i = 0 ; i < k ; ++i ) {
  M += R[ i ].first;
  N.addEdge( s , n + i , R[ i ].first );
  for( int c : R[ i ].second )
   N.addEdge( n + i , c , 1 );
  }
 for( int c = 0 ; c < n ; ++c )
  N.addEdge( c , p , 1 );
 return( ( t == M ) && ( N.maxFlow( s , p ) == t ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-6" )
{
 // the example of the text, the courses A , ... , G as 0 , ... , 6
 const std::vector< std::pair< long , Ints > > R = {
  { 2 , { 0 , 1 , 2 } } , { 2 , { 1 , 2 , 3 , 4 } } ,
  { 1 , { 0 , 1 , 2 , 3 , 4 , 5 , 6 } } };
 CHECK( flow::requirements( 7 , R , 5 ) );
 CHECK( ! flow::requirements( 7 , R , 4 ) );
 CHECK( ! flow::requirements( 2 , { { 2 , { 0 , 1 } } , { 1 , { 0 , 1 } } } ,
                              3 ) );
 CHECK( flow::requirements( 0 , {} , 0 ) );
 CHECK( ! flow::requirements( 0 , {} , 1 ) );
 // a requirement of 0 courses out of none; one of more courses than its
 // set
 CHECK( flow::requirements( 3 , { { 0 , {} } } , 0 ) );
 CHECK( ! flow::requirements( 3 , { { 2 , { 1 } } } , 2 ) );
 CHECK( flow::requirements( 3 , { { 3 , { 0 , 1 , 2 } } } , 3 ) );
 // the contracts: n >= 0, m_i >= 0, the courses in [ 0 , n )
 CHECK_THROWS( flow::requirements( -1 , {} , 0 ) , contract_violation );
 CHECK_THROWS( flow::requirements( 2 , { { -1 , { 0 } } } , 0 ) ,
               contract_violation );
 CHECK_THROWS( flow::requirements( 2 , { { 1 , { 2 } } } , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::requirements( 2 , { { 1 , { -1 } } } , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 6 ) , k = rnd( g , 0 , 3 );
  std::vector< std::pair< long , Ints > > Q( k );
  long M = 0;
  for( auto & q : Q ) {
   q.second = subset( g , n );
   q.first = rnd( g , 0 , 2 );
   M += q.first;
   }
  // each course to a requirement that has it, or to none
  bool any = false;
  everyChoice( Ints( n , k + 1 ) , [ & ]( const Ints & a ) {
   Longs got( k , 0 );
   for( int c = 0 ; c < n ; ++c )
    if( a[ c ] > 0 ) {
     if( ! in( Q[ a[ c ] - 1 ].second , c ) )
      return;
     ++got[ a[ c ] - 1 ];
     }
   for( int i = 0 ; i < k ; ++i )
    if( got[ i ] < Q[ i ].first )
     return;
   any = true;
   } );
  CHECK( flow::requirements( n , Q , M ) == any );
  CHECK( ! flow::requirements( n , Q , M + 1 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ flow-7, 2015-02-02, Cubi alfabetici -------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Cubi alfabetici: whether the word w can be written with distinct cubes
/// of C, a letter on a face of each. The matching of the letters of w to
/// the cubes that have them has size |w|, by bipartiteMatching(), O( n^2 ).
/// \pre each cube of 6 faces, |w| <= |C|
bool cubes( const std::vector< std::string > & C , const std::string & w )
{
 ADS_PRE( w.size() <= C.size() );
 ADS_PRE( std::all_of( C.begin() , C.end() , []( const std::string & c ) {
  return( c.size() == 6 ); } ) );
 Pairs E;
 for( std::size_t i = 0 ; i < w.size() ; ++i )
  for( std::size_t j = 0 ; j < C.size() ; ++j )
   if( C[ j ].find( w[ i ] ) != std::string::npos )
    E.emplace_back( int( i ) , int( j ) );
 return( bipartiteMatching( int( w.size() ) , int( C.size() ) , E ).size() ==
         w.size() );
 }

}  // end( namespace flow )

ADS_TEST( "flow-7" )
{
 // the cubes of the text: D and E are only on the first cube
 const std::vector< std::string > C = { "ABCDEF" , "GHIJKL" , "AABBCC" ,
                                        "ISTUVW" };
 CHECK( ! flow::cubes( C , "IDEA" ) );
 CHECK( flow::cubes( C , "IDGA" ) );
 CHECK( flow::cubes( C , "" ) );
 CHECK( flow::cubes( { "AAAAAA" } , "A" ) );
 CHECK( ! flow::cubes( { "AAAAAA" , "BBBBBB" } , "AA" ) );
 // no cube and the empty word; a word as long as the cubes, each letter
 // on one cube only, in the reverse order; a letter on no cube
 CHECK( flow::cubes( {} , "" ) );
 CHECK( flow::cubes( C , "IGAD" ) );
 CHECK( flow::cubes( C , "SBLD" ) );
 CHECK( ! flow::cubes( C , "Z" ) );
 CHECK( ! flow::cubes( C , "a" ) );
 // the contracts: |w| <= |C|, cubes of 6 faces
 CHECK_THROWS( flow::cubes( {} , "A" ) , contract_violation );
 CHECK_THROWS( flow::cubes( { "ABCDE" } , "A" ) , contract_violation );
 CHECK_THROWS( flow::cubes( { "ABCDEFG" } , "" ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 5 ) , t = rnd( g , 0 , n );
  std::vector< std::string > D( n , std::string( 6 , 'A' ) );
  for( auto & c : D )
   for( auto & f : c )
    f = char( 'A' + rnd( g , 0 , 4 ) );
  std::string w( t , 'A' );
  for( auto & l : w )
   l = char( 'A' + rnd( g , 0 , 4 ) );
  bool any = false;
  everyChoice( Ints( t , n ) , [ & ]( const Ints & a ) {
   Ints b = a;
   std::sort( b.begin() , b.end() );
   if( std::adjacent_find( b.begin() , b.end() ) != b.end() )
    return;
   for( int i = 0 ; i < t ; ++i )
    if( D[ a[ i ] ].find( w[ i ] ) == std::string::npos )
     return;
   any = true;
   } );
  CHECK( flow::cubes( D , w ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- flow-8, 2015-09-01, SIESTA -----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// SIESTA: company i sends at most d[ i ] people, workshop j holds at most
/// c[ j ], company i asks for the workshops w[ i ], one person of a company
/// at most in a workshop; the most requests that can be met. A maximum
/// flow S -> i ( d_i ) -> j ( 1 , j in w_i ) -> P ( c_j ), O( min( D , W , C
/// ) ( m + n + W ) ).
/// \pre d , c >= 0, w of the size of d with workshops in [ 0 , |c| )
long siesta( const Longs & d , const Longs & c ,
             const std::vector< Ints > & w )
{
 const int n = int( d.size() ) , m = int( c.size() );
 ADS_PRE( w.size() == d.size() );
 ADS_PRE( std::all_of( d.begin() , d.end() , []( long x ) {
  return( x >= 0 ); } ) && std::all_of( c.begin() , c.end() , []( long x ) {
  return( x >= 0 ); } ) );
 FlowNetwork N( n + m + 2 );
 const int s = n + m , t = s + 1;
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , i , d[ i ] );
  for( int j : w[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < m ) );
   N.addEdge( i , n + j , 1 );
   }
  }
 for( int j = 0 ; j < m ; ++j )
  N.addEdge( n + j , t , c[ j ] );
 return( N.maxFlow( s , t ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-8" )
{
 CHECK( flow::siesta( {} , { 3 } , {} ) == 0 );
 CHECK( flow::siesta( { 0 , 0 } , { 3 } , { { 0 } , { 0 } } ) == 0 );
 CHECK( flow::siesta( { 2 , 2 } , { 1 , 2 } , { { 0 , 1 } , { 0 , 1 } } ) ==
        3 );
 // no workshop; workshops of no seat; a company asking for no workshop
 CHECK( flow::siesta( { 2 } , {} , { {} } ) == 0 );
 CHECK( flow::siesta( { 2 , 2 } , { 0 , 0 } , { { 0 , 1 } , { 0 , 1 } } ) ==
        0 );
 CHECK( flow::siesta( { 5 , 5 } , { 9 , 9 } , { { 0 , 1 } , {} } ) == 2 );
 // the contracts: w of the size of d, d , c >= 0, the workshops in range
 CHECK_THROWS( flow::siesta( { 1 } , { 1 } , {} ) , contract_violation );
 CHECK_THROWS( flow::siesta( { -1 } , { 1 } , { { 0 } } ) ,
               contract_violation );
 CHECK_THROWS( flow::siesta( { 1 } , { -1 } , { { 0 } } ) ,
               contract_violation );
 CHECK_THROWS( flow::siesta( { 1 } , { 1 } , { { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( flow::siesta( { 1 } , { 1 } , { { -1 } } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 3 ) , m = rnd( g , 0 , 3 );
  Longs d( n ) , c( m );
  for( auto & x : d )
   x = rnd( g , 0 , 2 );
  for( auto & x : c )
   x = rnd( g , 0 , 2 );
  std::vector< Ints > w( n );
  Pairs R;
  for( int i = 0 ; i < n ; ++i ) {
   w[ i ] = subset( g , m );
   for( int j : w[ i ] )
    R.emplace_back( i , j );
   }
  long best = 0;
  everyChoice( Ints( R.size() , 2 ) , [ & ]( const Ints & a ) {
   Longs di( n , 0 ) , cj( m , 0 );
   long k = 0;
   for( std::size_t r = 0 ; r < R.size() ; ++r )
    if( a[ r ] ) {
     if( ( ++di[ R[ r ].first ] > d[ R[ r ].first ] ) ||
         ( ++cj[ R[ r ].second ] > c[ R[ r ].second ] ) )
      return;
     ++k;
     }
   best = std::max( best , k );
   } );
  CHECK( flow::siesta( d , c , w ) == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ flow-9, 2015-12-17, Capacita ridotta ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// whether f is a flow from s to p within c, kept as f[ x ][ y ] = - f[ y ][
/// x ], f[ x ][ y ] <= c[ x ][ y ] and conserved out of s and p
bool flowOk( const Matrix & c , const Matrix & f , int s , int p )
{
 const int n = int( c.size() );
 for( int x = 0 ; x < n ; ++x ) {
  long net = 0;
  for( int y = 0 ; y < n ; ++y ) {
   if( ( f[ x ][ y ] != - f[ y ][ x ] ) || ( f[ x ][ y ] > c[ x ][ y ] ) )
    return( false );
   net += f[ x ][ y ];
   }
  if( ( x != s ) && ( x != p ) && ( net != 0 ) )
   return( false );
  }
 return( true );
 }

/// a path from s to p in the residual network c - f, by a breadth first
/// visit; its edges are added to f with its least residual capacity if add
bool residualPath( const Matrix & c , Matrix & f , int s , int p ,
                   bool add )
{
 const int n = int( c.size() );
 Ints parent( n , -1 );
 parent[ s ] = s;
 std::queue< int > Q;
 Q.push( s );
 while( ! Q.empty() && ( parent[ p ] < 0 ) ) {
  const int x = Q.front();
  Q.pop();
  for( int y = 0 ; y < n ; ++y )
   if( ( parent[ y ] < 0 ) && ( c[ x ][ y ] - f[ x ][ y ] > 0 ) ) {
    parent[ y ] = x;
    Q.push( y );
    }
  }
 if( parent[ p ] < 0 )
  return( false );
 if( add ) {
  long d = INF;
  for( int y = p ; y != s ; y = parent[ y ] )
   d = std::min( d , c[ parent[ y ] ][ y ] - f[ parent[ y ] ][ y ] );
  for( int y = p ; y != s ; y = parent[ y ] ) {
   f[ parent[ y ] ][ y ] += d;
   f[ y ][ parent[ y ] ] -= d;
   }
  }
 return( true );
 }

/// Capacita ridotta: c[ u ][ v ] decreases by one and the maximum flow f
/// from s to p becomes a maximum flow of the new c, without starting over:
/// decreaseCapacity() of the library. If ( u , v ) is saturated one unit
/// less goes on it, and on a path of flow from s to u and one from v to p;
/// the flow, one less, is then maximum or one augmenting path from s to p
/// makes it so, since the maximum flow decreases by one at most: three
/// visits, O( n^2 ) on the matrices. The text misses the case of a unit of
/// ( u , v ) on a cycle of flow, where no path of flow from s to u may
/// exist: a path of flow from v to u closes the cycle, which is cancelled,
/// and the value stays. The result is 0 if ( u , v ) was not saturated, 1
/// if a cycle was cancelled, 2 if the paths were.
/// \pre c square with c >= 0, s and p distinct nodes, c[ u ][ v ] > 0, f a
///      maximum flow within c (audit)
/// \post f a maximum flow within the new c (audit)
int reduceFlow( Matrix & c , int s , int p , Matrix & f , int u , int v )
{
 return( int( decreaseCapacity( c , f , s , p , u , v ) ) );
 }

}  // end( namespace flow )

/// the value of the flow f out of s
long value( const Matrix & f , int s )
{
 return( std::accumulate( f[ s ].begin() , f[ s ].end() , 0L ) );
 }

/// a maximum flow of the capacities c from s to p, as a matrix
Matrix flowMatrix( const Matrix & c , int s , int p )
{
 const int n = int( c.size() );
 FlowNetwork N( n );
 std::vector< std::tuple< int , int , int > > E;
 for( int x = 0 ; x < n ; ++x )
  for( int y = 0 ; y < n ; ++y )
   if( c[ x ][ y ] > 0 )
    E.emplace_back( N.addEdge( x , y , c[ x ][ y ] ) , x , y );
 N.maxFlow( s , p );
 Matrix f( n , Longs( n , 0 ) );
 for( auto [ e , x , y ] : E ) {
  f[ x ][ y ] += N.flow( e );
  f[ y ][ x ] -= N.flow( e );
  }
 return( f );
 }

ADS_TEST( "flow-9" )
{
 // a path 0 -> 1 -> 2 of capacity 1 and a cycle 1 -> 3 -> 4 -> 1 of flow
 // 1: the edge ( 1 , 3 ) is reduced, the cycle cancelled, the flow stays 1
 {
  Matrix c( 5 , Longs( 5 , 0 ) ) , f( 5 , Longs( 5 , 0 ) );
  for( auto [ x , y ] : Pairs{ { 0 , 1 } , { 1 , 2 } , { 1 , 3 } , { 3 , 4 } ,
                               { 4 , 1 } } ) {
   c[ x ][ y ] = f[ x ][ y ] = 1;
   f[ y ][ x ] = -1;
   }
  CHECK( flow::reduceFlow( c , 0 , 2 , f , 1 , 3 ) == 1 );
  CHECK( ( value( f , 0 ) == 1 ) && ( f[ 3 ][ 4 ] == 0 ) );
  // the edge ( 1 , 2 ) is the only way: the flow drops to 0
  CHECK( flow::reduceFlow( c , 0 , 2 , f , 1 , 2 ) == 2 );
  CHECK( value( f , 0 ) == 0 );
  }
 {
  // two paths of capacity 1 and a spare one: the flow keeps 2
  Matrix c = { { 0 , 1 , 2 , 0 } , { 0 , 0 , 0 , 1 } , { 0 , 0 , 0 , 2 } ,
               { 0 , 0 , 0 , 0 } };
  Matrix f = flowMatrix( c , 0 , 3 );
  CHECK( value( f , 0 ) == 3 );
  CHECK( flow::reduceFlow( c , 0 , 3 , f , 0 , 1 ) == 2 );
  CHECK( value( f , 0 ) == 2 );
  // the edge ( 0 , 1 ) is now at 0: the contract wants c[ u ][ v ] > 0
  CHECK_THROWS( flow::reduceFlow( c , 0 , 3 , f , 0 , 1 ) ,
                contract_violation );
  // ( 0 , 2 ) of 2 carries 2: down to 1 and to 0, the flow follows
  CHECK( flow::reduceFlow( c , 0 , 3 , f , 0 , 2 ) == 2 );
  CHECK( flow::reduceFlow( c , 0 , 3 , f , 0 , 2 ) == 2 );
  CHECK( ( value( f , 0 ) == 0 ) && flow::flowOk( c , f , 0 , 3 ) );
  // the edge ( 2 , 3 ) of 2 carries nothing now: slack, nothing changes
  CHECK( flow::reduceFlow( c , 0 , 3 , f , 2 , 3 ) == 0 );
  CHECK( ( c[ 2 ][ 3 ] == 1 ) && ( value( f , 0 ) == 0 ) );
  // the contracts: s = p, u = v, the nodes in range, matrices not square
  CHECK_THROWS( flow::reduceFlow( c , 0 , 0 , f , 2 , 3 ) ,
                contract_violation );
  CHECK_THROWS( flow::reduceFlow( c , 0 , 3 , f , 2 , 2 ) ,
                contract_violation );
  CHECK_THROWS( flow::reduceFlow( c , 0 , 4 , f , 2 , 3 ) ,
                contract_violation );
  Matrix f2 = f;
  f2.pop_back();
  CHECK_THROWS( flow::reduceFlow( c , 0 , 3 , f2 , 2 , 3 ) ,
                contract_violation );
  }
 std::mt19937 g( 1 );
 int cases[ 3 ] = { 0 , 0 , 0 };
 for( int it = 0 ; it < 1500 ; ++it ) {
  const int n = rnd( g , 2 , 6 );
  Matrix c( n , Longs( n , 0 ) );
  for( int x = 0 ; x < n ; ++x )
   for( int y = 0 ; y < n ; ++y )
    if( ( x != y ) && rnd( g , 0 , 1 ) )
     c[ x ][ y ] = rnd( g , 1 , 3 );
  const int s = rnd( g , 0 , n - 1 ) , p = ( s + rnd( g , 1 , n - 1 ) ) % n;
  Matrix f = flowMatrix( c , s , p );
  // cycles of flow along the residual capacities, the value unchanged
  for( int k = 0 ; k < 10 ; ++k ) {
   Ints C( n );
   std::iota( C.begin() , C.end() , 0 );
   std::shuffle( C.begin() , C.end() , g );
   C.resize( rnd( g , 2 , n ) );
   bool ok = true;
   for( std::size_t i = 0 ; i < C.size() ; ++i ) {
    const int x = C[ i ] , y = C[ ( i + 1 ) % C.size() ];
    ok = ok && ( c[ x ][ y ] - f[ x ][ y ] > 0 );
    }
   if( ok )
    for( std::size_t i = 0 ; i < C.size() ; ++i ) {
     const int x = C[ i ] , y = C[ ( i + 1 ) % C.size() ];
     ++f[ x ][ y ];
     --f[ y ][ x ];
     }
   }
  Pairs E;
  for( int x = 0 ; x < n ; ++x )
   for( int y = 0 ; y < n ; ++y )
    if( c[ x ][ y ] > 0 )
     E.emplace_back( x , y );
  if( E.empty() )
   continue;
  const auto [ u , v ] = E[ rnd( g , 0 , int( E.size() ) - 1 ) ];
  ++cases[ flow::reduceFlow( c , s , p , f , u , v ) ];
  CHECK( flow::flowOk( c , f , s , p ) );
  CHECK( value( f , s ) == value( flowMatrix( c , s , p ) , s ) );
  CHECK( ! flow::residualPath( c , f , s , p , false ) );
  }
 CHECK( ( cases[ 0 ] > 0 ) && ( cases[ 1 ] > 0 ) && ( cases[ 2 ] > 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-11, 2016-08-29, Consiglio dei CdL -----------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Consiglio dei CdL: n courses, k teachers of the categories cat[ j ] in {
/// 0 , 1 , 2 } (PO, PA, RU), teaching the courses teaches[ j ]; the
/// representative of each course, a teacher of it, no teacher for two
/// courses, n / 3 of each category, or nothing. A maximum flow s -> category
/// ( n / 3 ) -> teacher ( 1 ) -> course ( 1 , taught ) -> t ( 1 ): the
/// council exists if it is n, O( n k + n^2 ).
/// \pre n >= 0 divisible by 3, the categories in [ 0 , 2 ], the courses in
///      [ 0 , n )
std::optional< Ints > council( int n , const Ints & cat ,
                               const std::vector< Ints > & teaches )
{
 const int k = int( cat.size() );
 ADS_PRE( ( n >= 0 ) && ( n % 3 == 0 ) && ( teaches.size() == cat.size() ) );
 FlowNetwork N( n + k + 5 );
 const int s = n + k , t = s + 1 , P = t + 1;
 for( int c = 0 ; c < 3 ; ++c )
  N.addEdge( s , P + c , n / 3 );
 std::vector< std::tuple< int , int , int > > E;
 for( int j = 0 ; j < k ; ++j ) {
  ADS_PRE( ( cat[ j ] >= 0 ) && ( cat[ j ] <= 2 ) );
  N.addEdge( P + cat[ j ] , n + j , 1 );
  for( int c : teaches[ j ] ) {
   ADS_PRE( ( c >= 0 ) && ( c < n ) );
   E.emplace_back( N.addEdge( n + j , c , 1 ) , j , c );
   }
  }
 for( int c = 0 ; c < n ; ++c )
  N.addEdge( c , t , 1 );
 if( N.maxFlow( s , t ) < n )
  return( std::nullopt );
 Ints r( n , -1 );
 for( auto [ e , j , c ] : E )
  if( N.flow( e ) > 0 )
   r[ c ] = j;
 return( r );
 }

}  // end( namespace flow )

/// whether r gives each course a distinct teacher of it, n / 3 of each
/// category
bool councilOk( int n , const Ints & cat ,
                const std::vector< Ints > & teaches , const Ints & r )
{
 if( int( r.size() ) != n )
  return( false );
 Ints used( cat.size() , 0 ) , per( 3 , 0 );
 for( int c = 0 ; c < n ; ++c ) {
  if( ( r[ c ] < 0 ) || ! in( teaches[ r[ c ] ] , c ) || used[ r[ c ] ]++ )
   return( false );
  ++per[ cat[ r[ c ] ] ];
  }
 return( std::all_of( per.begin() , per.end() , [ & ]( int x ) {
  return( x == n / 3 ); } ) );
 }

ADS_TEST( "flow-11" )
{
 const auto e = flow::council( 0 , {} , {} );
 CHECK( e && e->empty() );
 // three courses, the PO and the PA teach all of them, no RU
 CHECK( ! flow::council( 3 , { 0 , 1 } , { { 0 , 1 , 2 } , { 0 , 1 , 2 } } ) );
 const auto r = flow::council( 3 , { 0 , 1 , 2 } ,
                               { { 2 } , { 0 , 2 } , { 0 , 1 , 2 } } );
 CHECK( r && ( *r == Ints{ 1 , 2 , 0 } ) );
 // teachers but no course; fewer teachers than courses; a teacher of all
 // the courses is one representative only
 const auto t = flow::council( 0 , { 0 , 1 } , { {} , {} } );
 CHECK( t && t->empty() );
 CHECK( ! flow::council( 3 , { 0 , 1 } , { { 0 } , { 1 } } ) );
 CHECK( ! flow::council( 3 , { 0 , 1 , 1 } , { { 0 , 1 , 2 } , { 0 } ,
                                              { 0 } } ) );
 // the contracts: n >= 0 and divisible by 3, the sizes, the categories in
 // [ 0 , 2 ], the courses in [ 0 , n )
 CHECK_THROWS( flow::council( 2 , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::council( -3 , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::council( 3 , { 0 } , {} ) , contract_violation );
 CHECK_THROWS( flow::council( 3 , { 3 } , { { 0 } } ) , contract_violation );
 CHECK_THROWS( flow::council( 3 , { -1 } , { { 0 } } ) , contract_violation );
 CHECK_THROWS( flow::council( 3 , { 0 } , { { 3 } } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 3 * rnd( g , 0 , 2 ) , k = rnd( g , 0 , 6 );
  Ints cat( k );
  std::vector< Ints > teaches( k );
  for( int j = 0 ; j < k ; ++j ) {
   cat[ j ] = rnd( g , 0 , 2 );
   teaches[ j ] = subset( g , n );
   if( teaches[ j ].size() > 3 )
    teaches[ j ].resize( 3 );
   }
  bool any = false;
  everyChoice( Ints( n , k ) , [ & ]( const Ints & a ) {
   any = any || councilOk( n , cat , teaches , a );
   } );
  const auto x = flow::council( n , cat , teaches );
  CHECK( bool( x ) == any );
  CHECK( ! x || councilOk( n , cat , teaches , *x ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- flow-13, 2017-01-24, Segherie ----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Segherie: w[ i ][ j ] trees grow ripe in year i in forest j and can be
/// cut then or later, at most u[ i ] in year i and v[ i ][ j ] of them in
/// forest j; the trees cut each year in each forest, as many as possible.
/// A maximum flow s -> ( i , j ) ( w_ij ) -> ( i + 1 , j ) ( + inf , the
/// ripe trees left ) and ( i , j ) -> a_i ( v_ij ) -> p ( u_i ), O( n m U ).
/// \pre w and v of m rows of n values >= 0, u of m values >= 0
Matrix sawmill( const Matrix & w , const Longs & u , const Matrix & v )
{
 const int m = int( u.size() ) , n = m ? int( w[ 0 ].size() ) : 0;
 ADS_PRE( ( w.size() == u.size() ) && ( v.size() == u.size() ) );
 FlowNetwork N( n * m + m + 2 );
 const int s = n * m + m , p = s + 1;
 std::vector< std::vector< int > > e( m , Ints( n ) );
 for( int i = 0 ; i < m ; ++i ) {
  ADS_PRE( ( int( w[ i ].size() ) == n ) && ( int( v[ i ].size() ) == n ) &&
           ( u[ i ] >= 0 ) );
  N.addEdge( n * m + i , p , u[ i ] );
  for( int j = 0 ; j < n ; ++j ) {
   N.addEdge( s , i * n + j , w[ i ][ j ] );
   if( i + 1 < m )
    N.addEdge( i * n + j , ( i + 1 ) * n + j , INF );
   e[ i ][ j ] = N.addEdge( i * n + j , n * m + i , v[ i ][ j ] );
   }
  }
 N.maxFlow( s , p );
 Matrix x( m , Longs( n ) );
 for( int i = 0 ; i < m ; ++i )
  for( int j = 0 ; j < n ; ++j )
   x[ i ][ j ] = N.flow( e[ i ][ j ] );
 return( x );
 }

}  // end( namespace flow )

/// whether the cuts x respect the ripe trees and the limits of Segherie
bool sawOk( const Matrix & w , const Longs & u , const Matrix & v ,
            const Matrix & x )
{
 const int m = int( u.size() ) , n = m ? int( w[ 0 ].size() ) : 0;
 Longs left( n , 0 );
 for( int i = 0 ; i < m ; ++i ) {
  long year = 0;
  for( int j = 0 ; j < n ; ++j ) {
   left[ j ] += w[ i ][ j ] - x[ i ][ j ];
   year += x[ i ][ j ];
   if( ( x[ i ][ j ] < 0 ) || ( x[ i ][ j ] > v[ i ][ j ] ) ||
    ( left[ j ] < 0 ) )
    return( false );
   }
  if( year > u[ i ] )
   return( false );
  }
 return( true );
 }

/// the total of the cuts x
long total( const Matrix & x )
{
 long t = 0;
 for( const auto & r : x )
  t += std::accumulate( r.begin() , r.end() , 0L );
 return( t );
 }

ADS_TEST( "flow-13" )
{
 CHECK( flow::sawmill( {} , {} , {} ).empty() );
 // the trees of the first year wait for the second, when the limits rise
 const Matrix w = { { 5 , 0 } , { 0 , 1 } };
 const Longs u = { 1 , 9 };
 const Matrix v = { { 9 , 9 } , { 9 , 9 } };
 const Matrix x = flow::sawmill( w , u , v );
 CHECK( sawOk( w , u , v , x ) && ( total( x ) == 6 ) );
 // one year of no cut, a forest of no limit: nothing is cut; the trees
 // ripe in the last year cannot wait
 CHECK( total( flow::sawmill( w , { 0 , 0 } , v ) ) == 0 );
 CHECK( total( flow::sawmill( w , u , { { 0 , 0 } , { 0 , 0 } } ) ) == 0 );
 CHECK( total( flow::sawmill( { { 0 } , { 7 } } , { 9 , 3 } ,
                              { { 9 } , { 9 } } ) ) == 3 );
 // years but no forest
 CHECK( flow::sawmill( { {} , {} } , { 1 , 1 } , { {} , {} } ) ==
        Matrix( 2 ) );
 // the contracts: the sizes, u >= 0, and w , v >= 0 (by the network)
 CHECK_THROWS( flow::sawmill( w , { 1 } , v ) , contract_violation );
 CHECK_THROWS( flow::sawmill( w , u , { { 9 , 9 } } ) , contract_violation );
 CHECK_THROWS( flow::sawmill( w , u , { { 9 , 9 } , { 9 } } ) ,
               contract_violation );
 CHECK_THROWS( flow::sawmill( w , { 1 , -1 } , v ) , contract_violation );
 CHECK_THROWS( flow::sawmill( { { -1 , 0 } , { 0 , 1 } } , u , v ) ,
               contract_violation );
 CHECK_THROWS( flow::sawmill( w , u , { { 9 , -1 } , { 9 , 9 } } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 300 ; ++it ) {
  const int m = rnd( g , 1 , 3 ) , n = rnd( g , 1 , 2 );
  Matrix W( m , Longs( n ) ) , V( m , Longs( n ) );
  Longs U( m );
  for( int i = 0 ; i < m ; ++i ) {
   U[ i ] = rnd( g , 0 , 3 );
   for( int j = 0 ; j < n ; ++j ) {
    W[ i ][ j ] = rnd( g , 0 , 2 );
    V[ i ][ j ] = rnd( g , 0 , 2 );
    }
   }
  long best = 0;
  everyChoice( Ints( n * m , 3 ) , [ & ]( const Ints & a ) {
   Matrix y( m , Longs( n ) );
   for( int i = 0 ; i < m ; ++i )
    for( int j = 0 ; j < n ; ++j )
     y[ i ][ j ] = a[ i * n + j ];
   if( sawOk( W , U , V , y ) )
    best = std::max( best , total( y ) );
   } );
  const Matrix y = flow::sawmill( W , U , V );
  CHECK( sawOk( W , U , V , y ) && ( total( y ) == best ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-14, 2017-06-29, Esperimento CERN ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Esperimento CERN: n properties, instrument i measures the properties
/// measures[ i ] and comes from the maker maker[ i ] out of t; whether each
/// property can be measured by r instruments of r distinct makers. A
/// maximum flow S -> s_i ( |p_i| ) -> d_{j,k} ( 1 , s_i measures j , k its
/// maker ) -> p_j ( 1 , a maker once ) -> P ( r ): feasible if it is n r,
/// O( m^2 ( n + t ) r ).
/// \pre the properties in [ 0 , n ), the makers in [ 0 , t ), r >= 0
bool cern( int n , const std::vector< Ints > & measures , const Ints & maker ,
           int t , long r )
{
 const int m = int( maker.size() );
 ADS_PRE( ( n >= 0 ) && ( t >= 0 ) && ( r >= 0 ) &&
          ( measures.size() == maker.size() ) );
 FlowNetwork N( m + n * t + n + 2 );
 const int D = m , P = D + n * t , s = P + n , p = s + 1;
 for( int i = 0 ; i < m ; ++i ) {
  ADS_PRE( ( maker[ i ] >= 0 ) && ( maker[ i ] < t ) );
  N.addEdge( s , i , long( measures[ i ].size() ) );
  for( int j : measures[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < n ) );
   N.addEdge( i , D + j * t + maker[ i ] , 1 );
   }
  }
 for( int j = 0 ; j < n ; ++j ) {
  for( int k = 0 ; k < t ; ++k )
   N.addEdge( D + j * t + k , P + j , 1 );
  N.addEdge( P + j , p , r );
  }
 return( N.maxFlow( s , p ) == n * r );
 }

}  // end( namespace flow )

ADS_TEST( "flow-14" )
{
 CHECK( flow::cern( 0 , {} , {} , 0 , 2 ) );
 CHECK( flow::cern( 1 , {} , {} , 1 , 0 ) );
 // two instruments of the same maker do not make two
 CHECK( ! flow::cern( 1 , { { 0 } , { 0 } } , { 0 , 0 } , 2 , 2 ) );
 CHECK( flow::cern( 1 , { { 0 } , { 0 } } , { 0 , 1 } , 2 , 2 ) );
 // r larger than the makers; no maker and r = 0; a property no instrument
 // measures
 CHECK( ! flow::cern( 1 , { { 0 } , { 0 } , { 0 } } , { 0 , 1 , 1 } , 2 ,
                      3 ) );
 CHECK( flow::cern( 2 , {} , {} , 0 , 0 ) );
 CHECK( ! flow::cern( 2 , { { 0 } } , { 0 } , 1 , 1 ) );
 // the contracts: n , t , r >= 0, the sizes, the makers in [ 0 , t ), the
 // properties in [ 0 , n )
 CHECK_THROWS( flow::cern( -1 , {} , {} , 1 , 0 ) , contract_violation );
 CHECK_THROWS( flow::cern( 1 , {} , {} , -1 , 0 ) , contract_violation );
 CHECK_THROWS( flow::cern( 1 , {} , {} , 1 , -1 ) , contract_violation );
 CHECK_THROWS( flow::cern( 1 , { { 0 } } , {} , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::cern( 1 , { { 0 } } , { 1 } , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::cern( 1 , { { 1 } } , { 0 } , 1 , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 3 ) , m = rnd( g , 0 , 4 ) , t = rnd( g , 1 , 3 );
  const long r = rnd( g , 0 , 2 );
  std::vector< Ints > ms( m );
  Ints mk( m );
  for( int i = 0 ; i < m ; ++i ) {
   ms[ i ] = subset( g , n );
   mk[ i ] = rnd( g , 0 , t - 1 );
   }
  // each property chooses a set of instruments
  bool any = false;
  everyChoice( Ints( n , 1 << m ) , [ & ]( const Ints & a ) {
   for( int j = 0 ; j < n ; ++j ) {
    if( std::popcount( unsigned( a[ j ] ) ) != r )
     return;
    Ints seen( t , 0 );
    for( int i = 0 ; i < m ; ++i )
     if( ( a[ j ] >> i & 1 ) && ( ! in( ms[ i ] , j ) || seen[ mk[ i ] ]++ ) )
      return;
    }
   any = true;
   } );
  CHECK( flow::cern( n , ms , mk , t , r ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ flow-15, 2018-08-21, Hateville Clubs ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Hateville Clubs: nc clubs, person i in the clubs clubs[ i ] and in the
/// party party[ i ] out of nz; a council of one representative per club, a
/// member of it, no person for two clubs, no party with more than nc / 2
/// seats: the person of each club, or nothing. A maximum flow S -> party (
/// floor( nc / 2 ) ) -> person ( 1 ) -> club ( 1 , a member ) -> P ( 1 ):
/// the council exists if it is nc, O( |P| nc^2 ).
/// \pre the clubs in [ 0 , nc ), the parties in [ 0 , nz )
std::optional< Ints > hateville( int nc , const std::vector< Ints > & clubs ,
                                 const Ints & party , int nz )
{
 const int np = int( party.size() );
 ADS_PRE( ( nc >= 0 ) && ( nz >= 0 ) && ( clubs.size() == party.size() ) );
 FlowNetwork N( nz + np + nc + 2 );
 const int Z = 0 , Q = nz , C = Q + np , s = C + nc , p = s + 1;
 for( int z = 0 ; z < nz ; ++z )
  N.addEdge( s , Z + z , nc / 2 );
 std::vector< std::tuple< int , int , int > > E;
 for( int i = 0 ; i < np ; ++i ) {
  ADS_PRE( ( party[ i ] >= 0 ) && ( party[ i ] < nz ) );
  N.addEdge( Z + party[ i ] , Q + i , 1 );
  for( int c : clubs[ i ] ) {
   ADS_PRE( ( c >= 0 ) && ( c < nc ) );
   E.emplace_back( N.addEdge( Q + i , C + c , 1 ) , i , c );
   }
  }
 for( int c = 0 ; c < nc ; ++c )
  N.addEdge( C + c , p , 1 );
 if( N.maxFlow( s , p ) < nc )
  return( std::nullopt );
 Ints x( nc , -1 );
 for( auto [ e , i , c ] : E )
  if( N.flow( e ) > 0 )
   x[ c ] = i;
 return( x );
 }

}  // end( namespace flow )

/// whether x gives each club a distinct member, no party over nc / 2
bool clubsOk( int nc , const std::vector< Ints > & clubs ,
                const Ints & party , int nz , const Ints & x )
{
 if( int( x.size() ) != nc )
  return( false );
 Ints used( party.size() , 0 ) , seats( nz , 0 );
 for( int c = 0 ; c < nc ; ++c ) {
  if( ( x[ c ] < 0 ) || ! in( clubs[ x[ c ] ] , c ) || used[ x[ c ] ]++ ||
      ( 2 * ++seats[ party[ x[ c ] ] ] > nc ) )
   return( false );
  }
 return( true );
 }

ADS_TEST( "flow-15" )
{
 const auto e = flow::hateville( 0 , {} , {} , 1 );
 CHECK( e && e->empty() );
 // one club: its only seat is more than half of the council
 CHECK( ! flow::hateville( 1 , { { 0 } } , { 0 } , 1 ) );
 // two clubs, the members of the second all of the same party as the only
 // member of the first
 CHECK( ! flow::hateville( 2 , { { 0 } , { 1 } , { 1 } } , { 0 , 0 , 0 } ,
                           2 ) );
 const auto x = flow::hateville( 2 , { { 0 } , { 1 } , { 1 } } ,
                                 { 0 , 0 , 1 } , 2 );
 CHECK( x && ( *x == Ints{ 0 , 2 } ) );
 // no party and no club; a club with no member; three clubs give each
 // party one seat only, floor( 3 / 2 )
 const auto z = flow::hateville( 0 , {} , {} , 0 );
 CHECK( z && z->empty() );
 CHECK( ! flow::hateville( 2 , { { 0 } , { 0 } } , { 0 , 1 } , 2 ) );
 CHECK( ! flow::hateville( 3 , { { 0 } , { 1 } , { 2 } } , { 0 , 0 , 1 } ,
                           2 ) );
 CHECK( flow::hateville( 3 , { { 0 } , { 1 } , { 2 } } , { 0 , 1 , 2 } , 3 ) );
 // the contracts: nc , nz >= 0, the sizes, the parties and the clubs in
 // range
 CHECK_THROWS( flow::hateville( -1 , {} , {} , 1 ) , contract_violation );
 CHECK_THROWS( flow::hateville( 1 , {} , {} , -1 ) , contract_violation );
 CHECK_THROWS( flow::hateville( 1 , { { 0 } } , {} , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::hateville( 1 , { { 0 } } , { 1 } , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::hateville( 1 , { { 1 } } , { 0 } , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int nc = rnd( g , 0 , 4 ) , np = rnd( g , 0 , 5 );
const int nz = rnd( g , 1 , 3 );
  std::vector< Ints > clubs( np );
  Ints party( np );
  for( int i = 0 ; i < np ; ++i ) {
   clubs[ i ] = subset( g , nc );
   party[ i ] = rnd( g , 0 , nz - 1 );
   }
  bool any = false;
  everyChoice( Ints( nc , np ) , [ & ]( const Ints & a ) {
   any = any || clubsOk( nc , clubs , party , nz , a );
   } );
  const auto y = flow::hateville( nc , clubs , party , nz );
  CHECK( bool( y ) == any );
  CHECK( ! y || clubsOk( nc , clubs , party , nz , *y ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------- flow-16, 2019-06-17, Cantieri ----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Cantieri: site i works in the months start[ i ] , ... , end[ i ] out of
/// 1 , ... , m and needs len[ i ] person-months; pm people a month, at most
/// pc of them on a site: whether all sites end in time. A maximum flow s ->
/// site ( len_i ) -> month ( pc , in its span ) -> p ( pm ): they do if it
/// is L = sum len_i, O( c m L ) by Ford and Fulkerson.
/// \pre 1 <= start[ i ] <= end[ i ] <= m, len , pm , pc >= 0
bool sites( const Ints & start , const Ints & end , const Longs & len , int m ,
            long pm , long pc )
{
 const int c = int( len.size() );
 ADS_PRE( ( start.size() == len.size() ) && ( end.size() == len.size() ) &&
          ( m >= 0 ) && ( pm >= 0 ) && ( pc >= 0 ) );
 FlowNetwork N( c + m + 2 );
 const int s = c + m , p = s + 1;
 long L = 0;
 for( int i = 0 ; i < c ; ++i ) {
  ADS_PRE( ( 1 <= start[ i ] ) && ( start[ i ] <= end[ i ] ) &&
           ( end[ i ] <= m ) && ( len[ i ] >= 0 ) );
  L += len[ i ];
  N.addEdge( s , i , len[ i ] );
  for( int j = start[ i ] ; j <= end[ i ] ; ++j )
   N.addEdge( i , c + j - 1 , pc );
  }
 for( int j = 0 ; j < m ; ++j )
  N.addEdge( c + j , p , pm );
 return( N.maxFlow( s , p ) == L );
 }

}  // end( namespace flow )

ADS_TEST( "flow-16" )
{
 CHECK( flow::sites( {} , {} , {} , 3 , 0 , 0 ) );
 CHECK( flow::sites( { 1 } , { 2 } , { 4 } , 2 , 2 , 2 ) );
 CHECK( ! flow::sites( { 1 } , { 2 } , { 5 } , 2 , 9 , 2 ) );
 // two sites in the same month compete for the people of the month
 CHECK( ! flow::sites( { 1 , 1 } , { 1 , 1 } , { 2 , 2 } , 1 , 3 , 2 ) );
 // no month and no site; a site of 0 person-months with no people; a
 // site of one month that needs more than pc
 CHECK( flow::sites( {} , {} , {} , 0 , 0 , 0 ) );
 CHECK( flow::sites( { 1 } , { 1 } , { 0 } , 1 , 0 , 0 ) );
 CHECK( ! flow::sites( { 1 } , { 1 } , { 3 } , 1 , 9 , 2 ) );
 CHECK( flow::sites( { 3 } , { 3 } , { 2 } , 3 , 2 , 2 ) );
 // the contracts: the sizes, m , pm , pc >= 0, 1 <= start <= end <= m,
 // len >= 0
 CHECK_THROWS( flow::sites( { 1 } , {} , { 1 } , 1 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::sites( {} , {} , {} , -1 , 1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::sites( {} , {} , {} , 1 , -1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::sites( {} , {} , {} , 1 , 1 , -1 ) , contract_violation );
 CHECK_THROWS( flow::sites( { 0 } , { 1 } , { 1 } , 1 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::sites( { 2 } , { 1 } , { 1 } , 2 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::sites( { 1 } , { 2 } , { 1 } , 1 , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::sites( { 1 } , { 1 } , { -1 } , 1 , 1 , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int c = rnd( g , 0 , 3 ) , m = rnd( g , 1 , 3 );
  const long pm = rnd( g , 0 , 3 ) , pc = rnd( g , 0 , 2 );
  Ints st( c ) , en( c );
  Longs len( c );
  for( int i = 0 ; i < c ; ++i ) {
   st[ i ] = rnd( g , 1 , m );
   en[ i ] = rnd( g , st[ i ] , m );
   len[ i ] = rnd( g , 0 , 4 );
   }
  bool any = false;
  everyChoice( Ints( c * m , int( pc ) + 1 ) , [ & ]( const Ints & a ) {
   for( int i = 0 ; i < c ; ++i ) {
    long d = 0;
    for( int j = 1 ; j <= m ; ++j ) {
     if( a[ i * m + j - 1 ] && ( ( j < st[ i ] ) || ( j > en[ i ] ) ) )
      return;
     d += a[ i * m + j - 1 ];
     }
    if( d != len[ i ] )
     return;
    }
   for( int j = 0 ; j < m ; ++j ) {
    long d = 0;
    for( int i = 0 ; i < c ; ++i )
     d += a[ i * m + j ];
    if( d > pm )
     return;
    }
   any = true;
   } );
  CHECK( flow::sites( st , en , len , m , pm , pc ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------- flow-17, 2019-07-18, Rete elettrica trentina --------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// the roles of the nodes of Rete elettrica trentina
enum Role { PLANT , TRANSFORMER , DISTRIBUTION };

/// the network of Rete elettrica trentina: the edges E of capacity k, a
/// source n joined to each plant v ( power[ v ] ), a sink n + 1 joined from
/// each transformer v ( consume[ v ] ) and distribution node ( r )
std::vector< Arc > theftNet( int n , const Pairs & E , const Ints & role ,
                             const Longs & power , const Longs & consume ,
                             long k , long r )
{
 std::vector< Arc > A;
 for( auto [ u , v ] : E )
  A.emplace_back( u , v , k );
 for( int v = 0 ; v < n ; ++v )
  if( role[ v ] == PLANT )
   A.emplace_back( n , v , power[ v ] );
  else
   A.emplace_back( v , n + 1 , role[ v ] == TRANSFORMER ? consume[ v ] : r );
 return( A );
 }

/// Rete elettrica trentina: the edges E of the nodes, plants that produce
/// power[ v ], transformers that need consume[ v ] and distribution nodes,
/// each edge carrying k at most; whether r can be stolen at each
/// distribution node with all the transformers served. A maximum flow from
/// a source joined to the plants ( power ) to a sink joined from the
/// transformers ( consume ) and the distribution nodes ( r ): it can, if
/// the flow is sum consume + r |D|, O( n m^2 ) by Edmonds and Karp. The
/// capacities of the text are real; here they are integer (rationals
/// scale to integers).
/// \pre the edges join nodes, the roles in { PLANT , TRANSFORMER ,
///      DISTRIBUTION }, the capacities >= 0
bool theft( int n , const Pairs & E , const Ints & role , const Longs & power ,
            const Longs & consume , long k , long r ,
            Augment how = Augment::EDMONDS_KARP )
{
 ADS_PRE( ( n >= 0 ) && ( int( role.size() ) == n ) && ( k >= 0 ) &&
          ( r >= 0 ) && ( int( power.size() ) == n ) &&
          ( int( consume.size() ) == n ) );
 ADS_PRE( std::all_of( E.begin() , E.end() , [ & ]( const auto & e ) {
  return( ( e.first >= 0 ) && ( e.first < n ) && ( e.second >= 0 ) &&
          ( e.second < n ) ); } ) );
 ADS_PRE( std::all_of( role.begin() , role.end() , []( int x ) {
  return( ( x >= PLANT ) && ( x <= DISTRIBUTION ) ); } ) );
 long need = 0;
 for( int v = 0 ; v < n ; ++v )
  if( role[ v ] != PLANT )
   need += role[ v ] == TRANSFORMER ? consume[ v ] : r;
 return( maxflow( n + 2 , theftNet( n , E , role , power , consume , k , r ) ,
                  n , n + 1 , how ) == need );
 }

}  // end( namespace flow )

ADS_TEST( "flow-17" )
{
 using flow::PLANT , flow::TRANSFORMER , flow::DISTRIBUTION;
 // a plant of 10 feeds a transformer of 4 and a distribution node by edges
 // of 5: 5 can be stolen, not 6
 const Pairs E = { { 0 , 1 } , { 0 , 2 } };
 const Ints role = { PLANT , TRANSFORMER , DISTRIBUTION };
 CHECK( flow::theft( 3 , E , role , { 10 , 0 , 0 } , { 0 , 4 , 0 } , 5 , 5 ) );
 CHECK( ! flow::theft( 3 , E , role , { 10 , 0 , 0 } , { 0 , 4 , 0 } , 5 ,
                       6 ) );
 CHECK( ! flow::theft( 3 , E , role , { 8 , 0 , 0 } , { 0 , 4 , 0 } , 5 ,
                      5 ) );
 CHECK( flow::theft( 0 , {} , {} , {} , {} , 1 , 1 ) );
 // nothing to steal and transformers of 0; edges of 0 with a demand; no
 // plant; a loop and parallel edges
 CHECK( flow::theft( 3 , E , role , { 0 , 0 , 0 } , { 0 , 0 , 0 } , 5 , 0 ) );
 CHECK( ! flow::theft( 3 , E , role , { 10 , 0 , 0 } , { 0 , 4 , 0 } , 0 ,
                       0 ) );
 CHECK( ! flow::theft( 2 , { { 0 , 1 } } , { TRANSFORMER , DISTRIBUTION } ,
                       { 0 , 0 } , { 1 , 0 } , 5 , 0 ) );
 CHECK( flow::theft( 2 , { { 0 , 0 } , { 0 , 1 } , { 0 , 1 } } ,
                     { PLANT , DISTRIBUTION } , { 9 , 0 } , { 0 , 0 } , 3 ,
                     6 ) );
 CHECK( ! flow::theft( 2 , { { 0 , 0 } , { 0 , 1 } , { 0 , 1 } } ,
                       { PLANT , DISTRIBUTION } , { 9 , 0 } , { 0 , 0 } , 3 ,
                       7 ) );
 // the contracts: the sizes, k , r >= 0, the edges join nodes, the roles
 // in range, the capacities >= 0 (by the network)
 const Longs p = { 10 , 0 , 0 } , c = { 0 , 4 , 0 };
 CHECK_THROWS( flow::theft( -1 , {} , {} , {} , {} , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , { PLANT } , p , c , 5 , 5 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , role , { 10 } , c , 5 , 5 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , role , p , { 4 } , 5 , 5 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , role , p , c , -1 , 5 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , role , p , c , 5 , -1 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , { { 0 , 3 } } , role , p , c , 5 , 5 ) ,
               contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , { PLANT , TRANSFORMER , 3 } , p , c , 5 ,
                            5 ) , contract_violation );
 CHECK_THROWS( flow::theft( 3 , E , role , { -1 , 0 , 0 } , c , 5 , 5 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 1 , 6 );
  const Pairs F = randomDigraph( g , n , 2 );
  Ints ro( n );
  Longs pw( n , 0 ) , cs( n , 0 );
  for( int v = 0 ; v < n ; ++v ) {
   ro[ v ] = rnd( g , 0 , 2 );
   if( ro[ v ] == PLANT )
    pw[ v ] = rnd( g , 0 , 6 );
   if( ro[ v ] == TRANSFORMER )
    cs[ v ] = rnd( g , 0 , 3 );
   }
  const long k = rnd( g , 0 , 4 ) , r = rnd( g , 0 , 2 );
  long need = 0;
  for( int v = 0 ; v < n ; ++v )
   need += ro[ v ] == TRANSFORMER ? cs[ v ] : ro[ v ] == DISTRIBUTION ? r : 0;
  const bool ok = flow::theft( n , F , ro , pw , cs , k , r );
  for( auto how : ways )
   CHECK( flow::theft( n , F , ro , pw , cs , k , r , how ) == ok );
  CHECK( ok == ( minCut( n + 2 , flow::theftNet( n , F , ro , pw , cs , k ,
                                                 r ) , n , n + 1 ) == need ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- flow-18, 2019-08-22, Torri -----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Torri: the most rooks on the white squares of the board M, no two in a
/// row or a column: the largest matching of the rows to the columns of
/// their white squares, by bipartiteMatching(), O( n^3 ).
/// \pre M square
int rooks( const Bools & M )
{
 const int n = int( M.size() );
 ADS_PRE( std::all_of( M.begin() , M.end() , [ & ]( const auto & r ) {
  return( int( r.size() ) == n ); } ) );
 Pairs E;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( M[ i ][ j ] )
    E.emplace_back( i , j );
 return( int( bipartiteMatching( n , n , E ).size() ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-18" )
{
 CHECK( flow::rooks( {} ) == 0 );
 CHECK( flow::rooks( Bools( 4 , std::vector< bool >( 4 , false ) ) ) == 0 );
 CHECK( flow::rooks( Bools( 4 , std::vector< bool >( 4 , true ) ) ) == 4 );
 // the white squares of the first column and of the last row: 2 rooks
 CHECK( flow::rooks( { { true , false , false } , { true , false , false } ,
                       { true , true , true } } ) == 2 );
 // one square, white or black; the white squares of one row; a
 // permutation, the anti-diagonal
 CHECK( flow::rooks( { { true } } ) == 1 );
 CHECK( flow::rooks( { { false } } ) == 0 );
 CHECK( flow::rooks( { { true , true , true } , { false , false , false } ,
                       { false , false , false } } ) == 1 );
 Bools A( 6 , std::vector< bool >( 6 , false ) );
 for( int i = 0 ; i < 6 ; ++i )
  A[ i ][ 5 - i ] = true;
 CHECK( flow::rooks( A ) == 6 );
 // the contracts: M square
 CHECK_THROWS( flow::rooks( { { true , true } } ) , contract_violation );
 CHECK_THROWS( flow::rooks( { { true } , { true , false } } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 1 , 5 );
  Bools M( n , std::vector< bool >( n ) );
  for( auto & r : M )
   for( int j = 0 ; j < n ; ++j )
    r[ j ] = rnd( g , 0 , 2 ) == 0;
  // each row a column or none
  int best = 0;
  everyChoice( Ints( n , n + 1 ) , [ & ]( const Ints & a ) {
   Ints used( n , 0 );
   int k = 0;
   for( int i = 0 ; i < n ; ++i )
    if( a[ i ] > 0 ) {
     if( ! M[ i ][ a[ i ] - 1 ] || used[ a[ i ] - 1 ]++ )
      return;
     ++k;
     }
   best = std::max( best , k );
   } );
  CHECK( flow::rooks( M ) == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------- flow-19, 2020-07-24, Didattica pandemica ----------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Didattica pandemica: na rooms of ns slots; course i may use the rooms
/// A[ i ] at the slots S[ i ], gets 0, 1 or 2 pairs ( room , slot ), each
/// pair for one course; course i is of the programme prog[ i ] out of nt,
/// each programme with k pairs at most: the most pairs given. A maximum
/// flow s -> programme ( k ) -> course ( 2 ) -> ( a , s ) ( 1 , allowed ) ->
/// p ( 1 ), O( m ( m n + t ) ). As in the text, a course may get one slot
/// in two rooms.
/// \pre the rooms in [ 0 , na ), the slots in [ 0 , ns ), the programmes in
///      [ 0 , nt ), k >= 0
long lectures( int na , int ns , const std::vector< Ints > & A ,
               const std::vector< Ints > & S , const Ints & prog , int nt ,
               long k )
{
 const int m = int( prog.size() );
 ADS_PRE( ( na >= 0 ) && ( ns >= 0 ) && ( nt >= 0 ) && ( k >= 0 ) &&
          ( A.size() == prog.size() ) && ( S.size() == prog.size() ) );
 FlowNetwork N( nt + m + na * ns + 2 );
 const int C = nt , R = C + m , s = R + na * ns , p = s + 1;
 for( int h = 0 ; h < nt ; ++h )
  N.addEdge( s , h , k );
 for( int i = 0 ; i < m ; ++i ) {
  ADS_PRE( ( prog[ i ] >= 0 ) && ( prog[ i ] < nt ) );
  N.addEdge( prog[ i ] , C + i , 2 );
  for( int a : A[ i ] )
   for( int o : S[ i ] ) {
    ADS_PRE( ( a >= 0 ) && ( a < na ) && ( o >= 0 ) && ( o < ns ) );
    N.addEdge( C + i , R + a * ns + o , 1 );
    }
  }
 for( int r = 0 ; r < na * ns ; ++r )
  N.addEdge( R + r , p , 1 );
 return( N.maxFlow( s , p ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-19" )
{
 CHECK( flow::lectures( 2 , 25 , {} , {} , {} , 1 , 5 ) == 0 );
 // a course gets two pairs at most, its programme limits it to one
 CHECK( flow::lectures( 2 , 2 , { { 0 , 1 } } , { { 0 , 1 } } , { 0 } , 1 ,
                        9 ) == 2 );
 CHECK( flow::lectures( 2 , 2 , { { 0 , 1 } } , { { 0 , 1 } } , { 0 } , 1 ,
                        1 ) == 1 );
 // k = 0; a course with no room or no slot; no room at all
 CHECK( flow::lectures( 2 , 2 , { { 0 , 1 } } , { { 0 , 1 } } , { 0 } , 1 ,
                        0 ) == 0 );
 CHECK( flow::lectures( 2 , 2 , { {} , { 1 } } , { { 0 } , {} } , { 0 , 0 } ,
                        1 , 9 ) == 0 );
 CHECK( flow::lectures( 0 , 0 , { {} } , { {} } , { 0 } , 1 , 9 ) == 0 );
 // two courses of one programme for a single pair
 CHECK( flow::lectures( 1 , 1 , { { 0 } , { 0 } } , { { 0 } , { 0 } } ,
                        { 0 , 0 } , 1 , 9 ) == 1 );
 // the contracts: the sizes >= 0 and equal, the programmes, the rooms and
 // the slots in range
 const std::vector< Ints > one = { { 0 } };
 CHECK_THROWS( flow::lectures( -1 , 1 , {} , {} , {} , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , -1 , {} , {} , {} , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , {} , {} , {} , -1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , {} , {} , {} , 1 , -1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , one , {} , { 0 } , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , one , one , { 1 } , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , { { 1 } } , one , { 0 } , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::lectures( 1 , 1 , one , { { 1 } } , { 0 } , 1 , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int na = rnd( g , 1 , 2 ) , ns = rnd( g , 1 , 2 );
const int m = rnd( g , 0 , 3 );
  const int nt = rnd( g , 1 , 2 );
  const long k = rnd( g , 0 , 3 );
  std::vector< Ints > A( m ) , S( m );
  Ints prog( m );
  for( int i = 0 ; i < m ; ++i ) {
   A[ i ] = subset( g , na );
   S[ i ] = subset( g , ns );
   prog[ i ] = rnd( g , 0 , nt - 1 );
   }
  const int P = na * ns;
  long best = 0;
  everyChoice( Ints( m * P , 2 ) , [ & ]( const Ints & x ) {
   Longs used( P , 0 ) , per( nt , 0 );
   long c = 0;
   for( int i = 0 ; i < m ; ++i ) {
    long mine = 0;
    for( int r = 0 ; r < P ; ++r )
     if( x[ i * P + r ] ) {
      if( ! in( A[ i ] , r / ns ) || ! in( S[ i ] , r % ns ) || used[ r ]++ )
       return;
      ++mine;
      }
    if( ( mine > 2 ) || ( ( per[ prog[ i ] ] += mine ) > k ) )
     return;
    c += mine;
    }
   best = std::max( best , c );
   } );
  CHECK( flow::lectures( na , ns , A , S , prog , nt , k ) == best );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-20, 2022-06-21, Piano strategico ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Piano strategico: group j has the budget b[ j ], action i needs exactly
/// x[ i ] from its groups grp[ i ] and goes to its delegates del[ i ] out of
/// d, each delegate receiving exactly K: whether the plan can be funded.
/// A maximum flow s -> G_j ( b_j ) -> A_i ( + inf , j in grp_i ) -> A'_i (
/// x_i ) -> D_l ( + inf , l in del_i ) -> t ( K ): it can if the flow is
/// sum x_i = d K, O( ( k + d ) n X ), X = sum x_i.
/// \pre b , x , K >= 0, the groups in [ 0 , |b| ), the delegates in [ 0 ,
///      d )
bool plan( const Longs & b , const Longs & x ,
           const std::vector< Ints > & grp ,
           const std::vector< Ints > & del , int d , long K )
{
 const int k = int( b.size() ) , n = int( x.size() );
 ADS_PRE( ( grp.size() == x.size() ) && ( del.size() == x.size() ) &&
          ( d >= 0 ) && ( K >= 0 ) );
 FlowNetwork N( k + 2 * n + d + 2 );
 const int A = k , A2 = A + n , D = A2 + n , s = D + d , t = s + 1;
 for( int j = 0 ; j < k ; ++j ) {
  ADS_PRE( b[ j ] >= 0 );
  N.addEdge( s , j , b[ j ] );
  }
 long X = 0;
 for( int i = 0 ; i < n ; ++i ) {
  ADS_PRE( x[ i ] >= 0 );
  X += x[ i ];
  N.addEdge( A + i , A2 + i , x[ i ] );
  for( int j : grp[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < k ) );
   N.addEdge( j , A + i , INF );
   }
  for( int l : del[ i ] ) {
   ADS_PRE( ( l >= 0 ) && ( l < d ) );
   N.addEdge( A2 + i , D + l , INF );
   }
  }
 for( int l = 0 ; l < d ; ++l )
  N.addEdge( D + l , t , K );
 return( ( X == d * K ) && ( N.maxFlow( s , t ) == X ) );
 }

}  // end( namespace flow )

/// whether each action i can split x[ i ] among the sets L[ i ] of
/// [ 0 , |cap| ), each one getting at most cap or exactly cap if exact
bool splits( const Longs & x , const std::vector< Ints > & L ,
             const Longs & cap , bool exact )
{
 Ints r , owner , to;
 for( std::size_t i = 0 ; i < x.size() ; ++i )
  for( int j : L[ i ] ) {
   r.push_back( int( x[ i ] ) + 1 );
   owner.push_back( int( i ) );
   to.push_back( j );
   }
 bool any = false;
 everyChoice( r , [ & ]( const Ints & a ) {
  Longs got( x.size() , 0 ) , sum( cap.size() , 0 );
  for( std::size_t e = 0 ; e < a.size() ; ++e ) {
   got[ owner[ e ] ] += a[ e ];
   sum[ to[ e ] ] += a[ e ];
   }
  if( got != x )
   return;
  for( std::size_t j = 0 ; j < cap.size() ; ++j )
   if( exact ? sum[ j ] != cap[ j ] : sum[ j ] > cap[ j ] )
    return;
  any = true;
  } );
 return( any );
 }

ADS_TEST( "flow-20" )
{
 CHECK( flow::plan( {} , {} , {} , {} , 0 , 5 ) );
 // one action of 4 from two groups of 2, to two delegates of 2
 CHECK( flow::plan( { 2 , 2 } , { 4 } , { { 0 , 1 } } , { { 0 , 1 } } , 2 ,
                    2 ) );
 CHECK( ! flow::plan( { 2 , 1 } , { 4 } , { { 0 , 1 } } , { { 0 , 1 } } , 2 ,
                      2 ) );
 // the delegate 1 is linked to no action
 CHECK( ! flow::plan( { 4 } , { 4 } , { { 0 } } , { { 0 } } , 2 , 2 ) );
 // K = 0 with actions of 0; delegates of K with no action; an action with
 // no group; budgets of 0
 CHECK( flow::plan( { 0 } , { 0 , 0 } , { {} , { 0 } } , { { 0 } , {} } , 3 ,
                    0 ) );
 CHECK( ! flow::plan( {} , {} , {} , {} , 1 , 1 ) );
 CHECK( ! flow::plan( { 4 } , { 4 } , { {} } , { { 0 } } , 1 , 4 ) );
 CHECK( ! flow::plan( { 0 , 0 } , { 1 } , { { 0 , 1 } } , { { 0 } } , 1 ,
                      1 ) );
 // the contracts: the sizes, d , K >= 0, b , x >= 0, the groups and the
 // delegates in range
 CHECK_THROWS( flow::plan( {} , { 0 } , {} , { {} } , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( flow::plan( {} , { 0 } , { {} } , {} , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( flow::plan( {} , {} , {} , {} , -1 , 0 ) , contract_violation );
 CHECK_THROWS( flow::plan( {} , {} , {} , {} , 0 , -1 ) , contract_violation );
 CHECK_THROWS( flow::plan( { -1 } , {} , {} , {} , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( flow::plan( {} , { -1 } , { {} } , { {} } , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( flow::plan( { 1 } , { 1 } , { { 1 } } , { { 0 } } , 1 , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::plan( { 1 } , { 1 } , { { 0 } } , { { 1 } } , 1 , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int k = rnd( g , 1 , 2 ) , n = rnd( g , 0 , 3 ) , d = rnd( g , 1 , 2 );
  const long K = rnd( g , 0 , 3 );
  Longs b( k ) , x( n );
  for( auto & y : b )
   y = rnd( g , 0 , 3 );
  std::vector< Ints > grp( n ) , del( n );
  for( int i = 0 ; i < n ; ++i ) {
   x[ i ] = rnd( g , 0 , 2 );
   grp[ i ] = subset( g , k );
   del[ i ] = subset( g , d );
   }
  // the action carries x_i from its groups to its delegates: the two sides
  // split apart
  const bool any = splits( x , grp , b , false ) &&
                   splits( x , del , Longs( d , K ) , true );
  CHECK( flow::plan( b , x , grp , del , d , K ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- flow-21, 2022-08-24, Esami -----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// the room, the slot and the supervisor of an exam
struct Exam { int room , slot , supervisor; };

/// Esami: course i of cs[ i ] students, room j of as[ j ] seats, no slots,
/// supervisor l available at the slots T[ l ], one exam at a time and d in
/// all for each; an exam per course, in a room that holds it, one exam per
/// room and slot, with a supervisor: the exams, or nothing. A maximum flow
/// s -> course ( 1 ) -> ( a , o ) ( 1 , cs_i <= as_j ) -> ( a , o )' ( 1 )
/// -> ( l , o ) ( 1 , o in T_l ) -> l ( 1 ) -> t ( d ): they exist if it is
/// C, O( A O C ( C + S ) + C S O ). The text has no edge ( a , o ) -> ( a ,
/// o )', so that two courses go to the same room and slot with two
/// supervisors; the edge of capacity 1 keeps one.
/// \pre the slots in [ 0 , no ), d >= 0
std::optional< std::vector< Exam > > exams( const Longs & cs ,
                                            const Longs & as , int no ,
                                            const std::vector< Ints > & T ,
                                            long d )
{
 const int nc = int( cs.size() ) , na = int( as.size() );
 const int nl = int( T.size() ) , AO = na * no , LO = nl * no;
 ADS_PRE( ( no >= 0 ) && ( d >= 0 ) );
 FlowNetwork N( nc + 2 * AO + LO + nl + 2 );
 const int R = nc , R2 = R + AO , Q = R2 + AO , L = Q + LO , s = L + nl;
 const int t = s + 1;
 std::vector< std::tuple< int , int , int > > room , sup;
 for( int i = 0 ; i < nc ; ++i ) {
  N.addEdge( s , i , 1 );
  for( int j = 0 ; j < na ; ++j )
   if( cs[ i ] <= as[ j ] )
    for( int o = 0 ; o < no ; ++o )
     room.emplace_back( N.addEdge( i , R + j * no + o , 1 ) , i ,
                        j * no + o );
  }
 for( int r = 0 ; r < AO ; ++r )
  N.addEdge( R + r , R2 + r , 1 );
 for( int l = 0 ; l < nl ; ++l ) {
  for( int o : T[ l ] ) {
   ADS_PRE( ( o >= 0 ) && ( o < no ) );
   for( int j = 0 ; j < na ; ++j )
    sup.emplace_back( N.addEdge( R2 + j * no + o , Q + l * no + o , 1 ) ,
                      j * no + o , l );
   N.addEdge( Q + l * no + o , L + l , 1 );
   }
  N.addEdge( L + l , t , d );
  }
 if( N.maxFlow( s , t ) < nc )
  return( std::nullopt );
 Ints who( AO , -1 );
 for( auto [ e , r , l ] : sup )
  if( N.flow( e ) > 0 )
   who[ r ] = l;
 std::vector< Exam > X( nc );
 for( auto [ e , i , r ] : room )
  if( N.flow( e ) > 0 )
   X[ i ] = Exam{ r / no , r % no , who[ r ] };
 return( X );
 }

}  // end( namespace flow )

/// whether the exams X respect the rules of Esami
bool examsOk( const Longs & cs , const Longs & as , int no ,
              const std::vector< Ints > & T , long d ,
              const std::vector< flow::Exam > & X )
{
 const int na = int( as.size() ) , nl = int( T.size() );
 if( X.size() != cs.size() )
  return( false );
 Ints room( na * no , 0 ) , busy( nl * no , 0 );
 Longs load( nl , 0 );
 for( std::size_t i = 0 ; i < X.size() ; ++i ) {
  const auto [ a , o , l ] = X[ i ];
  if( ( a < 0 ) || ( l < 0 ) || ( cs[ i ] > as[ a ] ) || ! in( T[ l ] , o ) ||
      room[ a * no + o ]++ || busy[ l * no + o ]++ || ( ++load[ l ] > d ) )
   return( false );
  }
 return( true );
 }

ADS_TEST( "flow-21" )
{
 using flow::Exam;
 const auto e = flow::exams( {} , { 10 } , 2 , { { 0 } } , 1 );
 CHECK( e && e->empty() );
 // one room and one slot for two courses, two supervisors: the network of
 // the text gives both, the room holds one
 CHECK( ! flow::exams( { 5 , 5 } , { 10 } , 1 , { { 0 } , { 0 } } , 1 ) );
 const auto x = flow::exams( { 5 , 5 } , { 10 } , 2 , { { 0 , 1 } } , 2 );
 CHECK( x && examsOk( { 5 , 5 } , { 10 } , 2 , { { 0 , 1 } } , 2 , *x ) );
 CHECK( ! flow::exams( { 5 , 5 } , { 10 } , 2 , { { 0 , 1 } } , 1 ) );
 CHECK( ! flow::exams( { 11 } , { 10 } , 2 , { { 0 , 1 } } , 1 ) );
 // no slot, no room, no supervisor, d = 0; a course exactly as large as
 // the room
 CHECK( ! flow::exams( { 5 } , { 10 } , 0 , { {} } , 1 ) );
 CHECK( ! flow::exams( { 5 } , {} , 2 , { { 0 , 1 } } , 1 ) );
 CHECK( ! flow::exams( { 5 } , { 10 } , 2 , {} , 1 ) );
 CHECK( ! flow::exams( { 5 } , { 10 } , 2 , { { 0 , 1 } } , 0 ) );
 CHECK( flow::exams( { 10 } , { 10 } , 1 , { { 0 } } , 1 ) );
 const auto z = flow::exams( {} , {} , 0 , {} , 0 );
 CHECK( z && z->empty() );
 // the contracts: no , d >= 0, the slots in [ 0 , no )
 CHECK_THROWS( flow::exams( {} , {} , -1 , {} , 0 ) , contract_violation );
 CHECK_THROWS( flow::exams( {} , {} , 1 , {} , -1 ) , contract_violation );
 CHECK_THROWS( flow::exams( { 1 } , { 1 } , 1 , { { 1 } } , 1 ) ,
               contract_violation );
 CHECK_THROWS( flow::exams( { 1 } , { 1 } , 1 , { { -1 } } , 1 ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int nc = rnd( g , 0 , 3 ) , na = rnd( g , 1 , 2 );
const int no = rnd( g , 1 , 2 );
  const int nl = rnd( g , 1 , 2 );
  const long d = rnd( g , 0 , 2 );
  Longs cs( nc ) , as( na );
  for( auto & y : cs )
   y = rnd( g , 1 , 3 );
  for( auto & y : as )
   y = rnd( g , 1 , 3 );
  std::vector< Ints > T( nl );
  for( auto & S : T )
   S = subset( g , no );
  bool any = false;
  everyChoice( Ints( nc , na * no * nl ) , [ & ]( const Ints & a ) {
   std::vector< Exam > X;
   for( int c : a )
    X.push_back( Exam{ c / ( no * nl ) , c / nl % no , c % nl } );
   any = any || examsOk( cs , as , no , T , d , X );
   } );
  const auto y = flow::exams( cs , as , no , T , d );
  CHECK( bool( y ) == any );
  CHECK( ! y || examsOk( cs , as , no , T , d , *y ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- flow-22, 2023-02-09, Facoltiadi ---------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Facoltiadi: k departments, a team of exactly 3 q students each (q = 7 in
/// the text), at least q male and q female; student i of the gender gen[ i
/// ] ( 0 male, 1 female, 2 not said ) joins the team of d1[ i ] or d2[ i ]
/// or none: the department of each student, -1 for none, or nothing. A
/// maximum flow s -> student ( 1 ) -> s^M_d , s^F_d (1 , the gender ), s^X_d
/// ( 1 ) for d = d1_i , d2_i -> t ( q each ): the teams exist if it is 3 q
/// k, O( 3 q k n ).
/// \pre k , q >= 0, the genders in [ 0 , 2 ], the departments in [ 0 , k )
std::optional< Ints > facoltiadi( int k , long q , const Ints & gen ,
                                  const Ints & d1 , const Ints & d2 )
{
 const int n = int( gen.size() );
 ADS_PRE( ( k >= 0 ) && ( q >= 0 ) && ( d1.size() == gen.size() ) &&
          ( d2.size() == gen.size() ) );
 FlowNetwork N( n + 3 * k + 2 );
 const int D = n , s = D + 3 * k , t = s + 1;
 std::vector< std::tuple< int , int , int > > E;
 for( int i = 0 ; i < n ; ++i ) {
  ADS_PRE( ( gen[ i ] >= 0 ) && ( gen[ i ] <= 2 ) && ( d1[ i ] >= 0 ) &&
           ( d1[ i ] < k ) && ( d2[ i ] >= 0 ) && ( d2[ i ] < k ) );
  N.addEdge( s , i , 1 );
  for( int j : { d1[ i ] , d2[ i ] } ) {
   if( gen[ i ] < 2 )
    E.emplace_back( N.addEdge( i , D + 3 * j + gen[ i ] , 1 ) , i , j );
   E.emplace_back( N.addEdge( i , D + 3 * j + 2 , 1 ) , i , j );
   }
  }
 for( int r = 0 ; r < 3 * k ; ++r )
  N.addEdge( D + r , t , q );
 if( N.maxFlow( s , t ) < 3 * q * k )
  return( std::nullopt );
 Ints x( n , -1 );
 for( auto [ e , i , j ] : E )
  if( N.flow( e ) > 0 )
   x[ i ] = j;
 return( x );
 }

}  // end( namespace flow )

/// whether x gives each department 3 q students, q male and q female
bool teamsOk( int k , long q , const Ints & gen , const Ints & d1 ,
              const Ints & d2 , const Ints & x )
{
 if( x.size() != gen.size() )
  return( false );
 Longs all( k , 0 ) , male( k , 0 ) , female( k , 0 );
 for( std::size_t i = 0 ; i < x.size() ; ++i ) {
  if( x[ i ] < 0 )
   continue;
  if( ( x[ i ] != d1[ i ] ) && ( x[ i ] != d2[ i ] ) )
   return( false );
  ++all[ x[ i ] ];
  male[ x[ i ] ] += gen[ i ] == 0;
  female[ x[ i ] ] += gen[ i ] == 1;
  }
 for( int j = 0 ; j < k ; ++j )
  if( ( all[ j ] != 3 * q ) || ( male[ j ] < q ) || ( female[ j ] < q ) )
   return( false );
 return( true );
 }

ADS_TEST( "flow-22" )
{
 // the sizes of the text: 14 departments, teams of 21, 294 students
 {
  Ints gen( 294 ) , d1( 294 ) , d2( 294 );
  for( int i = 0 ; i < 294 ; ++i ) {
   gen[ i ] = i % 3;
   d1[ i ] = i / 3 % 14;
   d2[ i ] = ( d1[ i ] + 1 ) % 14;
   }
  const auto x = flow::facoltiadi( 14 , 7 , gen , d1 , d2 );
  CHECK( x && teamsOk( 14 , 7 , gen , d1 , d2 , *x ) );
  CHECK( ! flow::facoltiadi( 14 , 7 , Ints( 294 , 0 ) , d1 , d2 ) );
  gen.pop_back();
  d1.pop_back();
  d2.pop_back();
  CHECK( ! flow::facoltiadi( 14 , 7 , gen , d1 , d2 ) );
  }
 const auto e = flow::facoltiadi( 1 , 0 , { 0 } , { 0 } , { 0 } );
 CHECK( e && ( *e == Ints{ -1 } ) );
 // no department; no student with q > 0; the smallest teams, q = 1: a
 // male, a female and one more, who may be of any gender
 const auto z = flow::facoltiadi( 0 , 7 , {} , {} , {} );
 CHECK( z && z->empty() );
 CHECK( ! flow::facoltiadi( 1 , 1 , {} , {} , {} ) );
 const auto t = flow::facoltiadi( 1 , 1 , { 0 , 1 , 1 } , { 0 , 0 , 0 } ,
                                  { 0 , 0 , 0 } );
 CHECK( t && ( *t == Ints{ 0 , 0 , 0 } ) );
 CHECK( ! flow::facoltiadi( 1 , 1 , { 2 , 2 , 2 } , { 0 , 0 , 0 } ,
                            { 0 , 0 , 0 } ) );
 // the contracts: k , q >= 0, the sizes, the genders and the departments
 // in range
 CHECK_THROWS( flow::facoltiadi( -1 , 0 , {} , {} , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , -1 , {} , {} , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , 0 , { 0 } , {} , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , 0 , { 0 } , { 0 } , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , 0 , { 3 } , { 0 } , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , 0 , { 0 } , { 1 } , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( flow::facoltiadi( 1 , 0 , { 0 } , { 0 } , { -1 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int k = rnd( g , 1 , 2 ) , n = rnd( g , 0 , 7 );
  const long q = rnd( g , 0 , 1 );
  Ints gen( n ) , d1( n ) , d2( n );
  for( int i = 0 ; i < n ; ++i ) {
   gen[ i ] = rnd( g , 0 , 2 );
   d1[ i ] = rnd( g , 0 , k - 1 );
   d2[ i ] = rnd( g , 0 , k - 1 );
   }
  bool any = false;
  everyChoice( Ints( n , 3 ) , [ & ]( const Ints & a ) {
   Ints x( n );
   for( int i = 0 ; i < n ; ++i )
    x[ i ] = a[ i ] == 0 ? -1 : a[ i ] == 1 ? d1[ i ] : d2[ i ];
   any = any || teamsOk( k , q , gen , d1 , d2 , x );
   } );
  const auto x = flow::facoltiadi( k , q , gen , d1 , d2 );
  CHECK( bool( x ) == any );
  CHECK( ! x || teamsOk( k , q , gen , d1 , d2 , *x ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- flow-23, 2023-06-20, Conferenza ---------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Conferenza: paper j of the area area[ j ], reviewer r expert in the
/// areas expert[ r ]; 3 distinct reviewers per paper, 2 of them experts at
/// least, k reviews per reviewer at most: the reviewers of each paper, or
/// nothing. A maximum flow (version 1 of the text) s -> r ( k ) -> p^ex_j
/// ( 1 , if r is expert in a_j ) or p^ne_j ( 1 , if not ), p^ne_j ->
/// p^tot_j ( 1 ), p^ex_j -> p^tot_j ( 3 ), p^tot_j -> t ( 3 ): one edge per
/// pair keeps a reviewer from reviewing a paper twice; it exists if the
/// flow is 3 n_p, O( n_p^2 n_r ).
/// \pre the areas of the papers in expert[ r ] or not, k >= 0
std::optional< std::vector< Ints > > conference(
                                    const Ints & area ,
                                    const std::vector< Ints > & expert ,
                                    long k )
{
 const int np = int( area.size() ) , nr = int( expert.size() );
 ADS_PRE( k >= 0 );
 FlowNetwork N( nr + 3 * np + 2 );
 const int EX = nr , NE = EX + np , TOT = NE + np , s = TOT + np , t = s + 1;
 std::vector< std::tuple< int , int , int > > E;
 for( int r = 0 ; r < nr ; ++r ) {
  N.addEdge( s , r , k );
  for( int j = 0 ; j < np ; ++j )
   E.emplace_back( N.addEdge( r , ( in( expert[ r ] , area[ j ] ) ? EX : NE ) +
                              j , 1 ) , r , j );
  }
 for( int j = 0 ; j < np ; ++j ) {
  N.addEdge( NE + j , TOT + j , 1 );
  N.addEdge( EX + j , TOT + j , 3 );
  N.addEdge( TOT + j , t , 3 );
  }
 if( N.maxFlow( s , t ) < 3 * np )
  return( std::nullopt );
 std::vector< Ints > R( np );
 for( auto [ e , r , j ] : E )
  if( N.flow( e ) > 0 )
   R[ j ].push_back( r );
 return( R );
 }

}  // end( namespace flow )

/// whether R gives each paper 3 distinct reviewers, 2 experts, k at most
/// each
bool reviewsOk( const Ints & area , const std::vector< Ints > & expert ,
                long k , const std::vector< Ints > & R )
{
 Longs load( expert.size() , 0 );
 for( std::size_t j = 0 ; j < area.size() ; ++j ) {
  Ints S = R[ j ];
  std::sort( S.begin() , S.end() );
  if( ( S.size() != 3 ) ||
      ( std::adjacent_find( S.begin() , S.end() ) != S.end() ) )
   return( false );
  int ex = 0;
  for( int r : S ) {
   ex += in( expert[ r ] , area[ j ] );
   if( ++load[ r ] > k )
    return( false );
   }
  if( ex < 2 )
   return( false );
  }
 return( true );
 }

ADS_TEST( "flow-23" )
{
 const auto e = flow::conference( {} , { { 0 } } , 1 );
 CHECK( e && e->empty() );
 // two experts and a third reviewer: one paper; a second paper of the
 // same area exceeds k = 1
 const std::vector< Ints > ex = { { 0 } , { 0 } , { 1 } };
 const auto x = flow::conference( { 0 } , ex , 1 );
 CHECK( x && reviewsOk( { 0 } , ex , 1 , *x ) );
 CHECK( ! flow::conference( { 0 , 0 } , ex , 1 ) );
 // one expert and two others: not enough experts
 CHECK( ! flow::conference( { 1 } , ex , 3 ) );
 // two experts only: a reviewer cannot count twice
 CHECK( ! flow::conference( { 0 } , { { 0 } , { 0 } } , 3 ) );
 // no reviewer, k = 0; three experts in all the areas
 const auto z = flow::conference( {} , {} , 0 );
 CHECK( z && z->empty() );
 CHECK( ! flow::conference( { 0 } , {} , 3 ) );
 CHECK( ! flow::conference( { 0 } , ex , 0 ) );
 const std::vector< Ints > all( 3 , Ints{ 0 , 1 } );
 const auto y = flow::conference( { 0 , 1 , 1 } , all , 3 );
 CHECK( y && reviewsOk( { 0 , 1 , 1 } , all , 3 , *y ) );
 CHECK( ! flow::conference( { 0 , 1 , 1 } , all , 2 ) );
 // the contracts: k >= 0
 CHECK_THROWS( flow::conference( {} , {} , -1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int np = rnd( g , 0 , 3 ) , nr = rnd( g , 3 , 4 );
const int na = rnd( g , 1 , 2 );
  const long k = rnd( g , 1 , 3 );
  Ints area( np );
  for( auto & a : area )
   a = rnd( g , 0 , na - 1 );
  std::vector< Ints > expert( nr );
  for( auto & E : expert )
   E = subset( g , na );
  bool any = false;
  everyChoice( Ints( np , 1 << nr ) , [ & ]( const Ints & a ) {
   std::vector< Ints > R( np );
   for( int j = 0 ; j < np ; ++j )
    for( int r = 0 ; r < nr ; ++r )
     if( a[ j ] >> r & 1 )
      R[ j ].push_back( r );
   any = any || reviewsOk( area , expert , k , R );
   } );
  const auto y = flow::conference( area , expert , k );
  CHECK( bool( y ) == any );
  CHECK( ! y || reviewsOk( area , expert , k , *y ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- flow-24, 2023-07-11, SWERC -----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// SWERC: university i brings n[ i ] students and is of the country
/// country[ i ] out of nc (6 in the text); nt tables of k seats; at most
/// one student of a university and per of a country (3 in the text) at a
/// table: the tables of the students of each university, or nothing. A
/// maximum flow s -> U_i ( n_i ) -> ( T_j , P( U_i ) ) ( 1 ) -> T_j ( per )
/// -> t ( k ): they exist if it is N = sum n_i, O( N n_t n_u ).
/// \pre n >= 0, the countries in [ 0 , nc ), nt , k , per >= 0
std::optional< std::vector< Ints > > swerc( const Longs & n ,
                                            const Ints & country , int nt ,
                                            long k , int nc = 6 ,
                                            long per = 3 )
{
 const int nu = int( n.size() );
 ADS_PRE( ( country.size() == n.size() ) && ( nt >= 0 ) && ( k >= 0 ) &&
          ( nc >= 0 ) && ( per >= 0 ) );
 FlowNetwork N( nu + nt * nc + nt + 2 );
 const int TP = nu , T = TP + nt * nc , s = T + nt , t = s + 1;
 std::vector< std::tuple< int , int , int > > E;
 long all = 0;
 for( int i = 0 ; i < nu ; ++i ) {
  ADS_PRE( ( n[ i ] >= 0 ) && ( country[ i ] >= 0 ) &&
           ( country[ i ] < nc ) );
  all += n[ i ];
  N.addEdge( s , i , n[ i ] );
  for( int j = 0 ; j < nt ; ++j )
   E.emplace_back( N.addEdge( i , TP + j * nc + country[ i ] , 1 ) , i , j );
  }
 for( int j = 0 ; j < nt ; ++j ) {
  for( int c = 0 ; c < nc ; ++c )
   N.addEdge( TP + j * nc + c , T + j , per );
  N.addEdge( T + j , t , k );
  }
 if( N.maxFlow( s , t ) < all )
  return( std::nullopt );
 std::vector< Ints > X( nu );
 for( auto [ e , i , j ] : E )
  if( N.flow( e ) > 0 )
   X[ i ].push_back( j );
 return( X );
 }

}  // end( namespace flow )

/// whether X seats the students of each university at distinct tables,
/// within the seats and per of a country at a table
bool tablesOk( const Longs & n , const Ints & country , int nt , long k ,
               int nc , long per , const std::vector< Ints > & X )
{
 Longs seat( nt , 0 ) , pc( nt * nc , 0 );
 for( std::size_t i = 0 ; i < n.size() ; ++i ) {
  Ints S = X[ i ];
  std::sort( S.begin() , S.end() );
  if( ( long( S.size() ) != n[ i ] ) ||
      ( std::adjacent_find( S.begin() , S.end() ) != S.end() ) )
   return( false );
  for( int j : S )
   if( ( ++seat[ j ] > k ) || ( ++pc[ j * nc + country[ i ] ] > per ) )
    return( false );
  }
 return( true );
 }

ADS_TEST( "flow-24" )
{
 const auto e = flow::swerc( {} , {} , 3 , 4 );
 CHECK( e && e->empty() );
 // four universities of one country at tables of 6: three a table
 const Longs four( 4 , 1 );
 const Ints same( 4 , 0 );
 CHECK( ! flow::swerc( four , same , 1 , 6 ) );
 const auto x = flow::swerc( four , same , 2 , 6 );
 CHECK( x && tablesOk( four , same , 2 , 6 , 6 , 3 , *x ) );
 // a university of three students and two tables
 CHECK( ! flow::swerc( { 3 } , { 0 } , 2 , 6 ) );
 // no table; universities of no student; k = 0; per = 0
 CHECK( ! flow::swerc( { 1 } , { 0 } , 0 , 6 ) );
 const auto z = flow::swerc( { 0 , 0 } , { 0 , 5 } , 0 , 0 );
 CHECK( z && ( *z == std::vector< Ints >( 2 ) ) );
 CHECK( ! flow::swerc( { 1 } , { 0 } , 3 , 0 ) );
 CHECK( ! flow::swerc( { 1 } , { 0 } , 3 , 6 , 6 , 0 ) );
 // a full table: k students of k distinct countries
 const auto f = flow::swerc( Longs( 6 , 1 ) , { 0 , 1 , 2 , 3 , 4 , 5 } , 1 ,
                             6 , 6 , 1 );
 CHECK( f && tablesOk( Longs( 6 , 1 ) , { 0 , 1 , 2 , 3 , 4 , 5 } , 1 , 6 , 6 ,
                       1 , *f ) );
 // the contracts: the sizes, nt , k , nc , per >= 0, n >= 0, the countries
 // in [ 0 , nc )
 CHECK_THROWS( flow::swerc( { 1 } , {} , 1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( {} , {} , -1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( {} , {} , 1 , -1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( {} , {} , 1 , 1 , -1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( {} , {} , 1 , 1 , 6 , -1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( { -1 } , { 0 } , 1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( { 1 } , { 6 } , 1 , 1 ) , contract_violation );
 CHECK_THROWS( flow::swerc( { 1 } , { -1 } , 1 , 1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int nu = rnd( g , 0 , 3 ) , nt = rnd( g , 1 , 3 );
const int nc = rnd( g , 1 , 2 );
  const long k = rnd( g , 0 , 3 ) , per = rnd( g , 0 , 2 );
  Longs n( nu );
  Ints country( nu );
  for( int i = 0 ; i < nu ; ++i ) {
   n[ i ] = rnd( g , 0 , 2 );
   country[ i ] = rnd( g , 0 , nc - 1 );
   }
  bool any = false;
  everyChoice( Ints( nu , 1 << nt ) , [ & ]( const Ints & a ) {
   std::vector< Ints > X( nu );
   for( int i = 0 ; i < nu ; ++i )
    for( int j = 0 ; j < nt ; ++j )
     if( a[ i ] >> j & 1 )
      X[ i ].push_back( j );
   any = any || tablesOk( n , country , nt , k , nc , per , X );
   } );
  const auto y = flow::swerc( n , country , nt , k , nc , per );
  CHECK( bool( y ) == any );
  CHECK( ! y || tablesOk( n , country , nt , k , nc , per , *y ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- flow-25, 2023-08-23, Rete idrica --------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// the network of Rete idrica: the pipes but the closed one, a source n
/// joined to each node v with prod[ v ] > 0, a sink n + 1 joined from each v
/// with use[ v ] > 0
std::vector< Arc > waterNet( int n , const std::vector< Arc > & pipes ,
                             const Longs & prod , const Longs & use ,
                             int closed )
{
 std::vector< Arc > A;
 for( int e = 0 ; e < int( pipes.size() ) ; ++e )
  if( e != closed )
   A.push_back( pipes[ e ] );
 for( int v = 0 ; v < n ; ++v ) {
  if( prod[ v ] > 0 )
   A.emplace_back( n , v , prod[ v ] );
  if( use[ v ] > 0 )
   A.emplace_back( v , n + 1 , use[ v ] );
  }
 return( A );
 }

/// Rete idrica: the pipes ( u , v , c ) of n nodes, the sources v producing
/// prod[ v ] > 0, the consumers using use[ v ] > 0; whether closing one of
/// the pipes per (by number) leaves some consumer short. For each one, a
/// maximum flow from a source joined to the sources ( prod ) to a sink
/// joined from the consumers ( use ) without it, compared with the demand
/// tot = sum use: O( |E_per| tot ( n + m ) ). As in the solution of the
/// text, true if one pipe at least leaves the flow under tot.
/// \pre the pipes join nodes with c >= 0, prod and use >= 0 and not both
///      positive at a node, per numbers of pipes
bool water( int n , const std::vector< Arc > & pipes , const Longs & prod ,
            const Longs & use , const Ints & per )
{
 ADS_PRE( ( n >= 0 ) && ( int( prod.size() ) == n ) &&
          ( int( use.size() ) == n ) );
 ADS_PRE( std::all_of( pipes.begin() , pipes.end() , [ & ]( const Arc & a ) {
  return( ( std::get< 0 >( a ) >= 0 ) && ( std::get< 0 >( a ) < n ) &&
          ( std::get< 1 >( a ) >= 0 ) && ( std::get< 1 >( a ) < n ) &&
          ( std::get< 2 >( a ) >= 0 ) ); } ) );
 long tot = 0;
 for( int v = 0 ; v < n ; ++v ) {
  ADS_PRE( ( prod[ v ] >= 0 ) && ( use[ v ] >= 0 ) &&
           ( ( prod[ v ] == 0 ) || ( use[ v ] == 0 ) ) );
  tot += use[ v ];
  }
 for( int e : per ) {
  ADS_PRE( ( e >= 0 ) && ( e < int( pipes.size() ) ) );
  if( maxflow( n + 2 , waterNet( n , pipes , prod , use , e ) , n , n + 1 ,
               Augment::FORD_FULKERSON ) < tot )
   return( true );
  }
 return( false );
 }

}  // end( namespace flow )

ADS_TEST( "flow-25" )
{
 // a source of 5 and a consumer of 3 by two pipes of 3, or by one
 const std::vector< Arc > two = { { 0 , 1 , 3 } , { 0 , 1 , 3 } };
 CHECK( ! flow::water( 2 , two , { 5 , 0 } , { 0 , 3 } , { 0 , 1 } ) );
 CHECK( flow::water( 2 , { { 0 , 1 , 3 } } , { 5 , 0 } , { 0 , 3 } , { 0 } ) );
 CHECK( ! flow::water( 2 , { { 0 , 1 , 3 } } , { 5 , 0 } , { 0 , 3 } , {} ) );
 // a distributor 1 between, two pipes in and one out
 const std::vector< Arc > mid = { { 0 , 1 , 2 } , { 0 , 1 , 2 } ,
                                  { 1 , 2 , 4 } };
 CHECK( flow::water( 3 , mid , { 9 , 0 , 0 } , { 0 , 0 , 3 } , { 0 } ) );
 CHECK( ! flow::water( 3 , mid , { 9 , 0 , 0 } , { 0 , 0 , 2 } , { 0 , 1 } ) );
 // no consumer: nobody is ever short; a consumer with no source is short
 // whatever pipe closes, but only if one does; a pipe of capacity 0
 CHECK( ! flow::water( 2 , two , { 5 , 0 } , { 0 , 0 } , { 0 , 1 } ) );
 CHECK( flow::water( 2 , two , { 0 , 0 } , { 0 , 3 } , { 1 } ) );
 CHECK( ! flow::water( 2 , two , { 0 , 0 } , { 0 , 3 } , {} ) );
 CHECK( ! flow::water( 2 , { { 0 , 1 , 3 } , { 0 , 1 , 0 } } , { 5 , 0 } ,
                       { 0 , 3 } , { 1 } ) );
 // the same pipe twice in per
 CHECK( flow::water( 2 , { { 0 , 1 , 3 } } , { 5 , 0 } , { 0 , 3 } ,
                     { 0 , 0 } ) );
 // the contracts: n >= 0, the sizes, the pipes join nodes with c >= 0,
 // prod and use >= 0 and not both positive, per numbers of pipes
 const Longs pr = { 5 , 0 } , us = { 0 , 3 };
 CHECK_THROWS( flow::water( -1 , {} , {} , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::water( 2 , two , { 5 } , us , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , two , pr , { 3 } , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , { { 0 , 2 , 3 } } , pr , us , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , { { 0 , 1 , -1 } } , pr , us , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , two , { -1 , 0 } , us , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , two , { 5 , 1 } , us , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , two , pr , us , { 2 } ) ,
               contract_violation );
 CHECK_THROWS( flow::water( 2 , two , pr , us , { -1 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 1 , 6 );
  std::vector< Arc > pipes;
  for( auto [ u , v ] : randomDigraph( g , n , 2 ) )
   pipes.emplace_back( u , v , rnd( g , 0 , 3 ) );
  Longs prod( n , 0 ) , use( n , 0 );
  for( int v = 0 ; v < n ; ++v ) {
   const int r = rnd( g , 0 , 2 );
   if( r == 1 )
    prod[ v ] = rnd( g , 1 , 4 );
   if( r == 2 )
    use[ v ] = rnd( g , 1 , 3 );
   }
  const long tot = std::accumulate( use.begin() , use.end() , 0L );
  const Ints per = subset( g , int( pipes.size() ) );
  bool any = false;
  for( int e : per )
   any = any || ( minCut( n + 2 ,
                          flow::waterNet( n , pipes , prod , use , e ) ,
                          n , n + 1 ) < tot );
  CHECK( flow::water( n , pipes , prod , use , per ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------- flow-26, 2024-01-09, Commissioni di laurea ---------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Commissioni di laurea: k areas and a committee per area, professor i
/// expert in the area area[ i ]; each committee of exactly z professors (5
/// in the text), e of them experts of its area at least (2 in the text),
/// no professor in two: the committee of each professor, -1 for none, or
/// nothing. A maximum flow s -> professor ( 1 ) -> c2_{p_i} ( 1 ) and ->
/// c3_j ( 1 , every j ), c2_j -> t ( e ), c3_j -> t ( z - e ): they exist if
/// it is z k, O( n k^2 ).
/// \pre the areas in [ 0 , k ), 0 <= e <= z
std::optional< Ints > committees( int k , const Ints & area , long z = 5 ,
                                  long e = 2 )
{
 const int n = int( area.size() );
 ADS_PRE( ( k >= 0 ) && ( e >= 0 ) && ( e <= z ) );
 FlowNetwork N( n + 2 * k + 2 );
 const int C2 = n , C3 = C2 + k , s = C3 + k , t = s + 1;
 std::vector< std::tuple< int , int , int > > E;
 for( int i = 0 ; i < n ; ++i ) {
  ADS_PRE( ( area[ i ] >= 0 ) && ( area[ i ] < k ) );
  N.addEdge( s , i , 1 );
  E.emplace_back( N.addEdge( i , C2 + area[ i ] , 1 ) , i , area[ i ] );
  for( int j = 0 ; j < k ; ++j )
   E.emplace_back( N.addEdge( i , C3 + j , 1 ) , i , j );
  }
 for( int j = 0 ; j < k ; ++j ) {
  N.addEdge( C2 + j , t , e );
  N.addEdge( C3 + j , t , z - e );
  }
 if( N.maxFlow( s , t ) < z * k )
  return( std::nullopt );
 Ints x( n , -1 );
 for( auto [ f , i , j ] : E )
  if( N.flow( f ) > 0 )
   x[ i ] = j;
 return( x );
 }

}  // end( namespace flow )

/// whether x makes each committee of z professors, e experts at least
bool committeesOk( int k , const Ints & area , long z , long e ,
                   const Ints & x )
{
 Longs all( k , 0 ) , ex( k , 0 );
 for( std::size_t i = 0 ; i < area.size() ; ++i )
  if( x[ i ] >= 0 ) {
   ++all[ x[ i ] ];
   ex[ x[ i ] ] += area[ i ] == x[ i ];
   }
 for( int j = 0 ; j < k ; ++j )
  if( ( all[ j ] != z ) || ( ex[ j ] < e ) )
   return( false );
 return( true );
 }

ADS_TEST( "flow-26" )
{
 // the sizes of the text: two areas, ten professors, two experts each
 const Ints area = { 0 , 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 };
 const auto x = flow::committees( 2 , area );
 CHECK( x && committeesOk( 2 , area , 5 , 2 , *x ) );
 CHECK( ! flow::committees( 2 , { 0 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } ) );
 CHECK( ! flow::committees( 1 , { 0 , 0 , 0 , 0 } ) );
 const auto e = flow::committees( 0 , { } );
 CHECK( e && e->empty() );
 // committees of 0 professors leave all out; e = z wants experts only;
 // e = 0 takes anybody
 const auto z = flow::committees( 2 , { 0 , 1 , 1 } , 0 , 0 );
 CHECK( z && ( *z == Ints{ -1 , -1 , -1 } ) );
 CHECK( flow::committees( 2 , { 0 , 0 , 1 , 1 } , 2 , 2 ) );
 CHECK( ! flow::committees( 2 , { 0 , 0 , 0 , 1 } , 2 , 2 ) );
 const auto a = flow::committees( 2 , { 0 , 0 , 0 , 0 } , 2 , 0 );
 CHECK( a && committeesOk( 2 , { 0 , 0 , 0 , 0 } , 2 , 0 , *a ) );
 // the contracts: k >= 0, 0 <= e <= z, the areas in [ 0 , k )
 CHECK_THROWS( flow::committees( -1 , {} ) , contract_violation );
 CHECK_THROWS( flow::committees( 1 , {} , 5 , -1 ) , contract_violation );
 CHECK_THROWS( flow::committees( 1 , {} , 2 , 3 ) , contract_violation );
 CHECK_THROWS( flow::committees( 1 , { 1 } ) , contract_violation );
 CHECK_THROWS( flow::committees( 1 , { -1 } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int k = rnd( g , 1 , 2 ) , n = rnd( g , 0 , 6 );
  const long z = rnd( g , 0 , 3 ) , ez = rnd( g , 0 , int( z ) );
  Ints ar( n );
  for( auto & a : ar )
   a = rnd( g , 0 , k - 1 );
  bool any = false;
  everyChoice( Ints( n , k + 1 ) , [ & ]( const Ints & a ) {
   Ints y( n );
   for( int i = 0 ; i < n ; ++i )
    y[ i ] = a[ i ] - 1;
   any = any || committeesOk( k , ar , z , ez , y );
   } );
  const auto y = flow::committees( k , ar , z , ez );
  CHECK( bool( y ) == any );
  CHECK( ! y || committeesOk( k , ar , z , ez , *y ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------- flow-27, 2024-07-02, Vie di fuga --------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Vie di fuga: N students at distinct cells S of an R x C classroom, each
/// border cell an exit, moves to the four neighbours; whether N escape
/// routes with no common cell exist. A maximum flow with unit capacities on
/// the cells, each cell in -> out, out -> in of its neighbours, s -> in of
/// each student, out of each border cell -> t: they do if it is N, O( R C N
/// ).
/// \pre R , C >= 0, the students at distinct cells of the room
bool escape( int R , int C , const Pairs & S )
{
 ADS_PRE( ( R >= 0 ) && ( C >= 0 ) );
 ADS_PRE( std::all_of( S.begin() , S.end() , [ & ]( const auto & c ) {
  return( ( c.first >= 0 ) && ( c.first < R ) && ( c.second >= 0 ) &&
          ( c.second < C ) ); } ) );
 const int RC = R * C , s = 2 * RC , t = s + 1;
 FlowNetwork N( 2 * RC + 2 );
 for( int r = 0 ; r < R ; ++r )
  for( int c = 0 ; c < C ; ++c ) {
   const int x = r * C + c;
   N.addEdge( x , RC + x , 1 );
   if( ( r == 0 ) || ( c == 0 ) || ( r == R - 1 ) || ( c == C - 1 ) )
    N.addEdge( RC + x , t , 1 );
   const int dr[] = { -1 , 1 , 0 , 0 } , dc[] = { 0 , 0 , -1 , 1 };
   for( int k = 0 ; k < 4 ; ++k ) {
    const int y = r + dr[ k ] , z = c + dc[ k ];
    if( ( y >= 0 ) && ( y < R ) && ( z >= 0 ) && ( z < C ) )
     N.addEdge( RC + x , y * C + z , 1 );
    }
   }
 for( auto [ r , c ] : S )
  N.addEdge( s , r * C + c , 1 );
 return( N.maxFlow( s , t ) == long( S.size() ) );
 }

}  // end( namespace flow )

bool routes( int R , int C , const Pairs & S , std::size_t i ,
             std::vector< bool > & used );

/// a route from the cell x of student i on, to the border, then the
/// routes of the next students; a route ends at the first border cell
bool route( int R , int C , const Pairs & S , std::size_t i ,
            std::vector< bool > & used , int x )
{
 const int r = x / C , c = x % C;
 if( ( r == 0 ) || ( c == 0 ) || ( r == R - 1 ) || ( c == C - 1 ) )
  return( routes( R , C , S , i + 1 , used ) );
 const int dr[] = { -1 , 1 , 0 , 0 } , dc[] = { 0 , 0 , -1 , 1 };
 for( int k = 0 ; k < 4 ; ++k ) {
  const int y = ( r + dr[ k ] ) * C + c + dc[ k ];
  if( used[ y ] )
   continue;
  used[ y ] = true;
  const bool ok = route( R , C , S , i , used , y );
  used[ y ] = false;
  if( ok )
   return( true );
  }
 return( false );
 }

/// the routes of the students i , ... , by backtracking on the cells
bool routes( int R , int C , const Pairs & S , std::size_t i ,
             std::vector< bool > & used )
{
 if( i == S.size() )
  return( true );
 const int x = S[ i ].first * C + S[ i ].second;
 if( used[ x ] )
  return( false );
 used[ x ] = true;
 const bool ok = route( R , C , S , i , used , x );
 used[ x ] = false;
 return( ok );
 }

ADS_TEST( "flow-27" )
{
 CHECK( flow::escape( 0 , 0 , {} ) );
 CHECK( flow::escape( 1 , 1 , { { 0 , 0 } } ) );
 // the ring of a 5 x 5 room full: its students escape, the one in the
 // middle cannot
 Pairs ring;
 for( int r = 1 ; r <= 3 ; ++r )
  for( int c = 1 ; c <= 3 ; ++c )
   if( ( r != 2 ) || ( c != 2 ) )
    ring.emplace_back( r , c );
 CHECK( flow::escape( 5 , 5 , ring ) );
 ring.emplace_back( 2 , 2 );
 CHECK( ! flow::escape( 5 , 5 , ring ) );
 CHECK( flow::escape( 3 , 3 , { { 1 , 1 } } ) );
 // a room of one row, all border: every student escapes where it sits;
 // a full 3 x 3 room, whose middle has no free route, even with a corner
 // free; without the middle all escape; a room of no cells
 Pairs row;
 for( int c = 0 ; c < 6 ; ++c )
  row.emplace_back( 0 , c );
 CHECK( flow::escape( 1 , 6 , row ) );
 CHECK( flow::escape( 6 , 1 , {} ) );
 Pairs full;
 for( int r = 0 ; r < 3 ; ++r )
  for( int c = 0 ; c < 3 ; ++c )
   full.emplace_back( r , c );
 CHECK( ! flow::escape( 3 , 3 , full ) );
 full.pop_back();
 CHECK( ! flow::escape( 3 , 3 , full ) );
 full.erase( full.begin() + 4 );
 CHECK( flow::escape( 3 , 3 , full ) );
 CHECK( flow::escape( 0 , 5 , {} ) );
 // the contracts: R , C >= 0, the students in the room
 CHECK_THROWS( flow::escape( -1 , 1 , {} ) , contract_violation );
 CHECK_THROWS( flow::escape( 1 , -1 , {} ) , contract_violation );
 CHECK_THROWS( flow::escape( 2 , 2 , { { 2 , 0 } } ) , contract_violation );
 CHECK_THROWS( flow::escape( 2 , 2 , { { 0 , -1 } } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int R = rnd( g , 1 , 5 ) , C = rnd( g , 1 , 5 );
  Ints cells( R * C );
  std::iota( cells.begin() , cells.end() , 0 );
  std::shuffle( cells.begin() , cells.end() , g );
  cells.resize( rnd( g , 0 , std::min( R * C , 6 ) ) );
  Pairs S;
  for( int x : cells )
   S.emplace_back( x / C , x % C );
  std::vector< bool > used( R * C , false );
  CHECK( flow::escape( R , C , S ) == routes( R , C , S , 0 , used ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------ flow-28, 2025-06-13, Braccia rubate alla ricerca ------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Braccia rubate alla ricerca: no offices, person l cleans at most m of
/// them, only among A[ l ]; whether each office gets one person. A maximum
/// flow s -> person ( m ) -> office ( 1 , in A_l ) -> t ( 1 ), the
/// b-matching of the people with capacity m, bipartiteBMatching() of the
/// library: it does if the matching has no pairs,
/// O( n_O ( n_L n_O + n_L + n_O ) ).
/// \pre the offices in [ 0 , no ), m >= 0
bool cleaning( int no , const std::vector< Ints > & A , long m )
{
 ADS_PRE( no >= 0 );
 using LL = std::vector< long long >;
 return( int( bipartiteBMatching( A , no , LL( A.size() , m ) ,
                                  LL( no , 1 ) ).size() ) == no );
 }

}  // end( namespace flow )

ADS_TEST( "flow-28" )
{
 CHECK( flow::cleaning( 0 , {} , 0 ) );
 CHECK( ! flow::cleaning( 1 , {} , 3 ) );
 CHECK( flow::cleaning( 3 , { { 0 , 1 , 2 } } , 3 ) );
 CHECK( ! flow::cleaning( 3 , { { 0 , 1 , 2 } } , 2 ) );
 CHECK( flow::cleaning( 3 , { { 0 , 1 , 2 } , { 2 } } , 2 ) );
 // m = 0; people with no office; no office and people; an office twice
 // in a list counts once
 CHECK( ! flow::cleaning( 1 , { { 0 } } , 0 ) );
 CHECK( flow::cleaning( 0 , { {} , {} } , 0 ) );
 CHECK( ! flow::cleaning( 2 , { {} , { 1 } } , 5 ) );
 CHECK( ! flow::cleaning( 2 , { { 0 , 0 } } , 5 ) );
 // the contracts: no >= 0, m >= 0, the offices in [ 0 , no )
 CHECK_THROWS( flow::cleaning( -1 , {} , 1 ) , contract_violation );
 CHECK_THROWS( flow::cleaning( 1 , { { 0 } } , -1 ) , contract_violation );
 CHECK_THROWS( flow::cleaning( 1 , { { 1 } } , 1 ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int no = rnd( g , 0 , 5 ) , nl = rnd( g , 1 , 3 );
  const long m = rnd( g , 0 , 3 );
  std::vector< Ints > A( nl );
  for( auto & S : A )
   S = subset( g , no );
  bool any = false;
  everyChoice( Ints( no , nl ) , [ & ]( const Ints & a ) {
   Longs load( nl , 0 );
   for( int o = 0 ; o < no ; ++o )
    if( ! in( A[ a[ o ] ] , o ) || ( ++load[ a[ o ] ] > m ) )
     return;
   any = true;
   } );
  CHECK( flow::cleaning( no , A , m ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- flow-29, 2025-08-25, Mensa -----------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Mensa: dish p of the type type[ p ] in { 0 , 1 , 2 } (primo, secondo,
/// dessert) with portions[ p ] portions, student s accepting the dishes
/// accept[ s ]; at most one dish of each type per student: the most
/// student-dish pairs. One maximum flow per type, s -> student ( 1 ) ->
/// dish of the type ( 1 , accepted ) -> t ( m_p ), and their sum, O( ( V_c
/// + E_c ) n ) each.
/// \pre the types in [ 0 , 2 ], portions >= 0, the dishes of accept in
///      [ 0 , |type| )
long canteen( const Ints & type , const Longs & portions ,
              const std::vector< Ints > & accept )
{
 const int np = int( type.size() ) , ns = int( accept.size() );
 ADS_PRE( portions.size() == type.size() );
 long all = 0;
 for( int c = 0 ; c < 3 ; ++c ) {
  FlowNetwork N( ns + np + 2 );
  const int s = ns + np , t = s + 1;
  for( int i = 0 ; i < ns ; ++i ) {
   N.addEdge( s , i , 1 );
   for( int p : accept[ i ] ) {
    ADS_PRE( ( p >= 0 ) && ( p < np ) );
    if( type[ p ] == c )
     N.addEdge( i , ns + p , 1 );
    }
   }
  for( int p = 0 ; p < np ; ++p ) {
   ADS_PRE( ( type[ p ] >= 0 ) && ( type[ p ] <= 2 ) &&
         ( portions[ p ] >= 0 ) );
   if( type[ p ] == c )
    N.addEdge( ns + p , t , portions[ p ] );
   }
  all += N.maxFlow( s , t );
  }
 return( all );
 }

}  // end( namespace flow )

ADS_TEST( "flow-29" )
{
 CHECK( flow::canteen( {} , {} , { {} , {} } ) == 0 );
 // a primo of 1 portion and a dessert of 2 for two students who want all
 CHECK( flow::canteen( { 0 , 2 } , { 1 , 2 } , { { 0 , 1 } , { 0 , 1 } } ) ==
        3 );
 // two primi: one per student
 CHECK( flow::canteen( { 0 , 0 } , { 5 , 5 } , { { 0 , 1 } } ) == 1 );
 // dishes of no portion; no student; a student of no dish; a student who
 // wants one dish of each type
 CHECK( flow::canteen( { 0 , 1 } , { 0 , 0 } , { { 0 , 1 } } ) == 0 );
 CHECK( flow::canteen( { 0 } , { 3 } , {} ) == 0 );
 CHECK( flow::canteen( { 0 } , { 3 } , { {} } ) == 0 );
 CHECK( flow::canteen( { 0 , 1 , 2 } , { 1 , 1 , 1 } , { { 0 , 1 , 2 } } ) ==
        3 );
 // the contracts: the sizes, the dishes in range, the types in [ 0 , 2 ],
 // portions >= 0
 CHECK_THROWS( flow::canteen( { 0 } , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::canteen( { 0 } , { 1 } , { { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( flow::canteen( { 3 } , { 1 } , {} ) , contract_violation );
 CHECK_THROWS( flow::canteen( { -1 } , { 1 } , {} ) , contract_violation );
 CHECK_THROWS( flow::canteen( { 0 } , { -1 } , {} ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int np = rnd( g , 0 , 5 ) , ns = rnd( g , 0 , 3 );
  Ints type( np );
  Longs por( np );
  for( int p = 0 ; p < np ; ++p ) {
   type[ p ] = rnd( g , 0 , 2 );
   por[ p ] = rnd( g , 0 , 2 );
   }
  std::vector< Ints > acc( ns );
  for( auto & S : acc )
   S = subset( g , np );
  // each student a dish or none, a type at a time
  long all = 0;
  for( int c = 0 ; c < 3 ; ++c ) {
   Ints D;
   for( int p = 0 ; p < np ; ++p )
    if( type[ p ] == c )
     D.push_back( p );
   long best = 0;
   everyChoice( Ints( ns , int( D.size() ) + 1 ) , [ & ]( const Ints & a ) {
    Longs used( np , 0 );
    long k = 0;
    for( int i = 0 ; i < ns ; ++i )
     if( a[ i ] > 0 ) {
      const int p = D[ a[ i ] - 1 ];
      if( ! in( acc[ i ] , p ) || ( ++used[ p ] > por[ p ] ) )
       return;
      ++k;
      }
    best = std::max( best , k );
    } );
   all += best;
   }
  CHECK( flow::canteen( type , por , acc ) == all );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-30, 2026-01-12, Junior vs senior ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Junior vs senior: volunteer i, a senior if senior[ i ], accepts the
/// services avail[ i ] and serves one at most; service j takes exactly
/// vt[ j ] volunteers, vs[ j ] seniors at least: whether it can be done. A
/// maximum flow s -> volunteer ( 1 ) -> sJ_j or sS_j ( 1 , accepted ),
/// sJ_j -> s_j ( vt_j - vs_j ), sS_j -> s_j ( vt_j ), s_j -> t ( vt_j ): it
/// can if the flow is sum vt_j, O( n^2 m ).
/// \pre 0 <= vs[ j ] <= vt[ j ], the services in [ 0 , m )
bool volunteers( const std::vector< bool > & senior ,
                 const std::vector< Ints > & avail , const Longs & vt ,
                 const Longs & vs )
{
 const int n = int( senior.size() ) , m = int( vt.size() );
 ADS_PRE( ( avail.size() == senior.size() ) && ( vs.size() == vt.size() ) );
 FlowNetwork N( n + 3 * m + 2 );
 const int J = n , S = J + m , V = S + m , s = V + m , t = s + 1;
 long all = 0;
 for( int j = 0 ; j < m ; ++j ) {
  ADS_PRE( ( vs[ j ] >= 0 ) && ( vs[ j ] <= vt[ j ] ) );
  all += vt[ j ];
  N.addEdge( J + j , V + j , vt[ j ] - vs[ j ] );
  N.addEdge( S + j , V + j , vt[ j ] );
  N.addEdge( V + j , t , vt[ j ] );
  }
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , i , 1 );
  for( int j : avail[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < m ) );
   N.addEdge( i , ( senior[ i ] ? S : J ) + j , 1 );
   }
  }
 return( N.maxFlow( s , t ) == all );
 }

}  // end( namespace flow )

ADS_TEST( "flow-30" )
{
 CHECK( flow::volunteers( {} , {} , {} , {} ) );
 CHECK( flow::volunteers( { false } , { { 0 } } , { 0 } , { 0 } ) );
 // a service of two with a senior: two juniors are not enough
 CHECK( ! flow::volunteers( { false , false } , { { 0 } , { 0 } } , { 2 } ,
                            { 1 } ) );
 CHECK( flow::volunteers( { false , true } , { { 0 } , { 0 } } , { 2 } ,
                          { 1 } ) );
 CHECK( flow::volunteers( { true , true } , { { 0 } , { 0 } } , { 2 } ,
                          { 1 } ) );
 // seniors only, vs = vt; a service of 0 with nobody; volunteers left
 // over stay free
 CHECK( ! flow::volunteers( { true , false } , { { 0 } , { 0 } } , { 2 } ,
                            { 2 } ) );
 CHECK( flow::volunteers( { true , true } , { { 0 } , { 0 } } , { 2 } ,
                          { 2 } ) );
 CHECK( flow::volunteers( {} , {} , { 0 , 0 } , { 0 , 0 } ) );
 CHECK( flow::volunteers( { true , false , false } , { { 0 } , { 0 } , {} } ,
                          { 1 } , { 1 } ) );
 // the contracts: the sizes, 0 <= vs <= vt, the services in range
 CHECK_THROWS( flow::volunteers( { true } , {} , {} , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::volunteers( {} , {} , { 1 } , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::volunteers( {} , {} , { 1 } , { -1 } ) ,
               contract_violation );
 CHECK_THROWS( flow::volunteers( {} , {} , { 1 } , { 2 } ) ,
               contract_violation );
 CHECK_THROWS( flow::volunteers( { true } , { { 1 } } , { 1 } , { 0 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 5 ) , m = rnd( g , 0 , 3 );
  std::vector< bool > sen( n );
  std::vector< Ints > av( n );
  for( int i = 0 ; i < n ; ++i ) {
   sen[ i ] = rnd( g , 0 , 1 );
   av[ i ] = subset( g , m );
   }
  Longs vt( m ) , vs( m );
  for( int j = 0 ; j < m ; ++j ) {
   vt[ j ] = rnd( g , 0 , 2 );
   vs[ j ] = rnd( g , 0 , int( vt[ j ] ) );
   }
  bool any = false;
  everyChoice( Ints( n , m + 1 ) , [ & ]( const Ints & a ) {
   Longs all( m , 0 ) , old( m , 0 );
   for( int i = 0 ; i < n ; ++i )
    if( a[ i ] > 0 ) {
     if( ! in( av[ i ] , a[ i ] - 1 ) )
      return;
     ++all[ a[ i ] - 1 ];
     old[ a[ i ] - 1 ] += sen[ i ];
     }
   for( int j = 0 ; j < m ; ++j )
    if( ( all[ j ] != vt[ j ] ) || ( old[ j ] < vs[ j ] ) )
     return;
   any = true;
   } );
  CHECK( flow::volunteers( sen , av , vt , vs ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------- flow-31, 2026-02-02, Personale ospedaliero ---------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Personale ospedaliero: operator i, a nurse if nurse[ i ] and an OSS if
/// not, goes to exactly one of the wards capable[ i ]; ward j gets exactly
/// mw[ j ] operators, mi[ j ] nurses and mo[ j ] OSS at least: whether it
/// can be done. If sum mw_j != n it cannot; else a maximum flow s ->
/// operator ( 1 ) -> rI_j or rO_j ( 1 , capable ), rI_j -> r_j ( mw_j -
/// mo_j ), rO_j -> r_j ( mw_j - mi_j ), r_j -> t ( mw_j ): it can if the
/// flow is n, O( n^2 m ).
/// \pre mi[ j ] + mo[ j ] <= mw[ j ], all >= 0, the wards in [ 0 , m )
bool hospital( const std::vector< bool > & nurse ,
               const std::vector< Ints > & capable , const Longs & mw ,
               const Longs & mi , const Longs & mo )
{
 const int n = int( nurse.size() ) , m = int( mw.size() );
 ADS_PRE( ( capable.size() == nurse.size() ) && ( mi.size() == mw.size() ) &&
          ( mo.size() == mw.size() ) );
 FlowNetwork N( n + 3 * m + 2 );
 const int I = n , O = I + m , W = O + m , s = W + m , t = s + 1;
 long all = 0;
 for( int j = 0 ; j < m ; ++j ) {
  ADS_PRE( ( mi[ j ] >= 0 ) && ( mo[ j ] >= 0 ) &&
           ( mi[ j ] + mo[ j ] <= mw[ j ] ) );
  all += mw[ j ];
  N.addEdge( I + j , W + j , mw[ j ] - mo[ j ] );
  N.addEdge( O + j , W + j , mw[ j ] - mi[ j ] );
  N.addEdge( W + j , t , mw[ j ] );
  }
 if( all != n )
  return( false );
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( s , i , 1 );
  for( int j : capable[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < m ) );
   N.addEdge( i , ( nurse[ i ] ? I : O ) + j , 1 );
   }
  }
 return( N.maxFlow( s , t ) == n );
 }

}  // end( namespace flow )

ADS_TEST( "flow-31" )
{
 CHECK( flow::hospital( {} , {} , {} , {} , {} ) );
 // every operator must be placed
 CHECK( ! flow::hospital( { true } , { { 0 } } , { 0 } , { 0 } , { 0 } ) );
 CHECK( flow::hospital( { true , false } , { { 0 } , { 0 } } , { 2 } , { 1 } ,
                        { 1 } ) );
 CHECK( ! flow::hospital( { true , true } , { { 0 } , { 0 } } , { 2 } , { 1 } ,
                          { 1 } ) );
 // operators and no ward; wards of 0; mi + mo = mw; an operator capable
 // of no ward
 CHECK( ! flow::hospital( { true } , { { } } , {} , {} , {} ) );
 CHECK( flow::hospital( {} , {} , { 0 , 0 } , { 0 , 0 } , { 0 , 0 } ) );
 CHECK( flow::hospital( { false , true } , { { 0 } , { 0 } } , { 2 } , { 1 } ,
                        { 1 } ) );
 CHECK( ! flow::hospital( { true , false } , { { 0 } , {} } , { 2 } , { 0 } ,
                          { 0 } ) );
 // the contracts: the sizes, mi , mo >= 0, mi + mo <= mw, the wards in
 // range
 CHECK_THROWS( flow::hospital( { true } , {} , {} , {} , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( {} , {} , { 1 } , {} , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( {} , {} , { 1 } , { 0 } , {} ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( {} , {} , { 1 } , { -1 } , { 0 } ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( {} , {} , { 1 } , { 0 } , { -1 } ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( {} , {} , { 1 } , { 1 } , { 1 } ) ,
               contract_violation );
 CHECK_THROWS( flow::hospital( { true } , { { 1 } } , { 1 } , { 0 } ,
                               { 0 } ) , contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 5 ) , m = rnd( g , 1 , 3 );
  std::vector< bool > nu( n );
  std::vector< Ints > cap( n );
  for( int i = 0 ; i < n ; ++i ) {
   nu[ i ] = rnd( g , 0 , 1 );
   cap[ i ] = subset( g , m , 3 );
   cap[ i ].push_back( rnd( g , 0 , m - 1 ) );
   }
  Longs mw( m ) , mi( m ) , mo( m );
  for( int j = 0 ; j < m ; ++j ) {
   mw[ j ] = rnd( g , 0 , 3 );
   mi[ j ] = rnd( g , 0 , int( mw[ j ] ) );
   mo[ j ] = rnd( g , 0 , int( mw[ j ] - mi[ j ] ) );
   }
  bool any = false;
  everyChoice( Ints( n , m ) , [ & ]( const Ints & a ) {
   Longs all( m , 0 ) , ni( m , 0 );
   for( int i = 0 ; i < n ; ++i ) {
    if( ! in( cap[ i ] , a[ i ] ) )
     return;
    ++all[ a[ i ] ];
    ni[ a[ i ] ] += nu[ i ];
    }
   for( int j = 0 ; j < m ; ++j )
    if( ( all[ j ] != mw[ j ] ) || ( ni[ j ] < mi[ j ] ) ||
        ( all[ j ] - ni[ j ] < mo[ j ] ) )
     return;
   any = true;
   } );
  CHECK( flow::hospital( nu , cap , mw , mi , mo ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------ flow-32, 2026-06-22, Corsi di laurea ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Corsi di laurea: course i of the programme prog[ i ] out of k,
/// professor j competent in the courses choices[ j ] and teaching at most
/// load courses (3 in the text), others too but not competently; one
/// professor per course, t[ h ] courses of programme h taught competently
/// at least: whether it can be done. A maximum flow s -> p_j ( load ) ->
/// cc_i ( 1 , if competent ) or ci_i ( 1 , if not ), cc_i -> CdLc_h ( 1 ),
/// ci_i -> CdLi_h ( 1 ), CdLc_h -> CdL_h ( m_h ), CdLi_h -> CdL_h ( m_h -
/// t_h ), CdL_h -> t ( m_h ): it can if the flow is m, O( m ( n m + k ) ).
/// A flow may take a course both from cc_i and ci_i and leave another
/// course of the programme empty; the professor of ci_i can always move
/// there, so the answer stands.
/// \pre the programmes in [ 0 , k ), the courses in [ 0 , |prog| ),
///      0 <= t[ h ] <= m_h, load >= 0
bool degrees( const Ints & prog , int k , const std::vector< Ints > & choices ,
              const Longs & t , long load = 3 )
{
 const int m = int( prog.size() ) , n = int( choices.size() );
 ADS_PRE( ( k >= 0 ) && ( int( t.size() ) == k ) && ( load >= 0 ) );
 FlowNetwork N( n + 2 * m + 3 * k + 2 );
 const int CC = n , CI = CC + m , HC = CI + m , HI = HC + k , H = HI + k;
 const int s = H + k , p = s + 1;
 Longs mh( k , 0 );
 for( int i = 0 ; i < m ; ++i ) {
  ADS_PRE( ( prog[ i ] >= 0 ) && ( prog[ i ] < k ) );
  ++mh[ prog[ i ] ];
  N.addEdge( CC + i , HC + prog[ i ] , 1 );
  N.addEdge( CI + i , HI + prog[ i ] , 1 );
  }
 for( int h = 0 ; h < k ; ++h ) {
  ADS_PRE( ( t[ h ] >= 0 ) && ( t[ h ] <= mh[ h ] ) );
  N.addEdge( HC + h , H + h , mh[ h ] );
  N.addEdge( HI + h , H + h , mh[ h ] - t[ h ] );
  N.addEdge( H + h , p , mh[ h ] );
  }
 for( int j = 0 ; j < n ; ++j ) {
  N.addEdge( s , j , load );
  for( int i = 0 ; i < m ; ++i )
   N.addEdge( j , ( in( choices[ j ] , i ) ? CC : CI ) + i , 1 );
  }
 return( N.maxFlow( s , p ) == m );
 }

}  // end( namespace flow )

ADS_TEST( "flow-32" )
{
 CHECK( flow::degrees( {} , 1 , { {} } , { 0 } ) );
 // four courses and one professor: three at most
 CHECK( ! flow::degrees( { 0 , 0 , 0 , 0 } , 1 , { { 0 , 1 , 2 , 3 } } ,
                         { 0 } ) );
 // two courses, both to be taught competently, two professors competent
 // in the first only
 CHECK( ! flow::degrees( { 0 , 0 } , 1 , { { 0 } , { 0 } } , { 2 } ) );
 CHECK( flow::degrees( { 0 , 0 } , 1 , { { 0 } , { 0 } } , { 1 } ) );
 CHECK( flow::degrees( { 0 , 0 } , 1 , { { 0 } , { 1 } } , { 2 } ) );
 // no programme and no course; load = 0; no professor; t = 0 lets anybody
 // teach
 CHECK( flow::degrees( {} , 0 , {} , {} ) );
 CHECK( ! flow::degrees( { 0 } , 1 , { { 0 } } , { 0 } , 0 ) );
 CHECK( ! flow::degrees( { 0 } , 1 , {} , { 0 } ) );
 CHECK( flow::degrees( { 0 , 0 , 0 } , 1 , { {} } , { 0 } ) );
 CHECK( ! flow::degrees( { 0 , 0 , 0 } , 1 , { {} } , { 1 } ) );
 // the contracts: k >= 0, |t| = k, load >= 0, the programmes in range,
 // 0 <= t[ h ] <= m_h
 CHECK_THROWS( flow::degrees( {} , -1 , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::degrees( {} , 1 , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::degrees( {} , 0 , {} , {} , -1 ) , contract_violation );
 CHECK_THROWS( flow::degrees( { 1 } , 1 , {} , { 0 } ) , contract_violation );
 CHECK_THROWS( flow::degrees( { 0 } , 1 , {} , { 2 } ) , contract_violation );
 CHECK_THROWS( flow::degrees( { 0 } , 1 , {} , { -1 } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int m = rnd( g , 0 , 4 ) , k = rnd( g , 1 , 2 ) , n = rnd( g , 1 , 3 );
  const long load = rnd( g , 0 , 3 );
  Ints prog( m );
  Longs mh( k , 0 ) , t( k );
  for( auto & h : prog ) {
   h = rnd( g , 0 , k - 1 );
   ++mh[ h ];
   }
  for( int h = 0 ; h < k ; ++h )
   t[ h ] = rnd( g , 0 , int( mh[ h ] ) );
  std::vector< Ints > ch( n );
  for( auto & S : ch )
   S = subset( g , m );
  bool any = false;
  everyChoice( Ints( m , n ) , [ & ]( const Ints & a ) {
   Longs used( n , 0 ) , good( k , 0 );
   for( int i = 0 ; i < m ; ++i ) {
    if( ++used[ a[ i ] ] > load )
     return;
    good[ prog[ i ] ] += in( ch[ a[ i ] ] , i );
    }
   for( int h = 0 ; h < k ; ++h )
    if( good[ h ] < t[ h ] )
     return;
   any = true;
   } );
  CHECK( flow::degrees( prog , k , ch , t , load ) == any );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------- flow-33, 2026-07-20, Banco alimentare ------------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// Banco alimentare: warehouse i has s[ i ] parcels, shelter j takes d[ j ]
/// at most, warehouse i supplies the shelters A[ i ]: the parcels from each
/// warehouse to each shelter, as many as possible. A maximum flow s -> i
/// ( s_i ) -> j ( min( s_i , d_j ) , j in A_i ) -> t ( d_j ), integral with
/// the capacities, O( |F*| ( |V| + |E| ) ).
/// \pre s , d >= 0, the shelters in [ 0 , |d| )
Matrix foodBank( const Longs & s , const Longs & d ,
                 const std::vector< Ints > & A )
{
 const int nm = int( s.size() ) , nc = int( d.size() );
 ADS_PRE( A.size() == s.size() );
 FlowNetwork N( nm + nc + 2 );
 const int src = nm + nc , t = src + 1;
 std::vector< std::tuple< int , int , int > > E;
 for( int i = 0 ; i < nm ; ++i ) {
  ADS_PRE( s[ i ] >= 0 );
  N.addEdge( src , i , s[ i ] );
  for( int j : A[ i ] ) {
   ADS_PRE( ( j >= 0 ) && ( j < nc ) );
   E.emplace_back( N.addEdge( i , nm + j , std::min( s[ i ] , d[ j ] ) ) , i ,
                   j );
   }
  }
 for( int j = 0 ; j < nc ; ++j ) {
  ADS_PRE( d[ j ] >= 0 );
  N.addEdge( nm + j , t , d[ j ] );
  }
 N.maxFlow( src , t );
 Matrix x( nm , Longs( nc , 0 ) );
 for( auto [ e , i , j ] : E )
  x[ i ][ j ] += N.flow( e );
 return( x );
 }

}  // end( namespace flow )

/// whether x respects the stocks s, the limits d and the links A
bool parcelsOk( const Longs & s , const Longs & d ,
                const std::vector< Ints > & A , const Matrix & x )
{
 Longs got( d.size() , 0 );
 for( std::size_t i = 0 ; i < s.size() ; ++i ) {
  long out = 0;
  for( std::size_t j = 0 ; j < d.size() ; ++j ) {
   if( ( x[ i ][ j ] < 0 ) || ( x[ i ][ j ] && ! in( A[ i ] , int( j ) ) ) )
    return( false );
   out += x[ i ][ j ];
   got[ j ] += x[ i ][ j ];
   }
  if( out > s[ i ] )
   return( false );
  }
 for( std::size_t j = 0 ; j < d.size() ; ++j )
  if( got[ j ] > d[ j ] )
   return( false );
 return( true );
 }

ADS_TEST( "flow-33" )
{
 CHECK( flow::foodBank( {} , { 3 } , {} ).empty() );
 // a warehouse of 5 and two shelters of 2 and 4
 const Matrix x = flow::foodBank( { 5 } , { 2 , 4 } , { { 0 , 1 } } );
 CHECK( parcelsOk( { 5 } , { 2 , 4 } , { { 0 , 1 } } , x ) &&
        ( total( x ) == 5 ) );
 CHECK( total( flow::foodBank( { 5 , 5 } , { 3 } , { { 0 } , {} } ) ) == 3 );
 // no shelter; stocks or limits of 0; a shelter listed twice
 CHECK( flow::foodBank( { 4 } , {} , { {} } ) == Matrix( 1 ) );
 CHECK( total( flow::foodBank( { 0 , 0 } , { 3 } , { { 0 } , { 0 } } ) ) ==
        0 );
 CHECK( total( flow::foodBank( { 5 } , { 0 , 0 } , { { 0 , 1 } } ) ) == 0 );
 const Matrix y = flow::foodBank( { 5 } , { 4 } , { { 0 , 0 } } );
 CHECK( ( y == Matrix{ { 4 } } ) );
 // the contracts: |A| = |s|, s , d >= 0, the shelters in range
 CHECK_THROWS( flow::foodBank( { 1 } , {} , {} ) , contract_violation );
 CHECK_THROWS( flow::foodBank( { -1 } , {} , { {} } ) , contract_violation );
 CHECK_THROWS( flow::foodBank( {} , { -1 } , {} ) , contract_violation );
 CHECK_THROWS( flow::foodBank( { 1 } , { 1 } , { { 1 } } ) ,
               contract_violation );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int nm = rnd( g , 0 , 2 ) , nc = rnd( g , 0 , 3 );
  Longs s( nm ) , d( nc );
  for( auto & y : s )
   y = rnd( g , 0 , 3 );
  for( auto & y : d )
   y = rnd( g , 0 , 2 );
  std::vector< Ints > A( nm );
  for( auto & S : A )
   S = subset( g , nc );
  long best = 0;
  everyChoice( Ints( nm * nc , 3 ) , [ & ]( const Ints & a ) {
   Matrix y( nm , Longs( nc ) );
   for( int i = 0 ; i < nm ; ++i )
    for( int j = 0 ; j < nc ; ++j )
     y[ i ][ j ] = a[ i * nc + j ];
   if( parcelsOk( s , d , A , y ) )
    best = std::max( best , total( y ) );
   } );
  const Matrix y = flow::foodBank( s , d , A );
  CHECK( parcelsOk( s , d , A , y ) && ( total( y ) == best ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------- flow-34, 2026-08-28, Numeri inquadrabili ----------------*/
/*--------------------------------------------------------------------------*/

namespace flow {

/// whether z is a perfect square; the root of the double is corrected by
/// comparisons with z / r, which do not overflow up to the largest long
bool square( long z )
{
 if( z < 2 )
  return( z >= 0 );
 long r = std::lround( std::sqrt( double( z ) ) );
 while( r > z / r )
  --r;
 while( r + 1 <= z / ( r + 1 ) )
  ++r;
 return( r * r == z );
 }

/// Numeri inquadrabili: the most pairs ( x , y ), x in X and y in Y with x
/// + y a perfect square, no number in two pairs. The largest matching of
/// the bipartite graph of the square sums, by bipartiteMatching(), O( n^3
/// ); the greedy choice of any pair fails (the second example).
/// \pre the numbers distinct, each sum x + y a long
int squarePairs( const Longs & X , const Longs & Y )
{
 Pairs E;
 for( std::size_t i = 0 ; i < X.size() ; ++i )
  for( std::size_t j = 0 ; j < Y.size() ; ++j )
   if( square( X[ i ] + Y[ j ] ) )
    E.emplace_back( int( i ) , int( j ) );
 return( int( bipartiteMatching( int( X.size() ) , int( Y.size() ) ,
                                 E ).size() ) );
 }

}  // end( namespace flow )

ADS_TEST( "flow-34" )
{
 CHECK( flow::squarePairs( { 6 , 1 , 15 } , { 10 , 3 , 7 } ) == 2 );
 CHECK( flow::squarePairs( { 6 , 1 } , { 3 , 10 } ) == 2 );
 CHECK( flow::squarePairs( {} , {} ) == 0 );
 CHECK( flow::squarePairs( { 2 } , { 3 } ) == 0 );
 CHECK( flow::squarePairs( { -5 } , { 5 } ) == 1 );
 for( long r = 0 ; r < 100 ; ++r )
  for( long z = r * r ; z < ( r + 1 ) * ( r + 1 ) ; ++z )
   CHECK( flow::square( z ) == ( z == r * r ) );
 // the negative numbers; the largest long (2^31 - 1 or 2^63 - 1, no
 // square either way), whose root the double rounds up; the squares near
 // the largest int, valid whatever the size of long
 CHECK( ! flow::square( -1 ) && ! flow::square( LONG_MIN ) );
 CHECK( ! flow::square( LONG_MAX ) && ! flow::square( LONG_MAX - 1 ) );
 const long R = 46340;                       // floor( sqrt( 2^31 - 1 ) )
 CHECK( flow::square( R * R ) && ! flow::square( R * R - 1 ) &&
        ! flow::square( R * R + 1 ) && ! flow::square( INT_MAX ) );
 CHECK( flow::square( ( R - 1 ) * ( R - 1 ) ) );
 // one side empty; a sum 0; the sums of one number with the others
 CHECK( flow::squarePairs( { 1 , 2 } , {} ) == 0 );
 CHECK( flow::squarePairs( {} , { 1 , 2 } ) == 0 );
 CHECK( flow::squarePairs( { 0 } , { 4 } ) == 1 );
 CHECK( flow::squarePairs( { 3 } , { -3 } ) == 1 );
 CHECK( flow::squarePairs( { 0 } , { 1 , 4 , 9 } ) == 1 );
 CHECK( flow::squarePairs( { R * R - 7 } , { 7 } ) == 1 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = rnd( g , 0 , 5 );
  Ints v( 40 );
  std::iota( v.begin() , v.end() , -10 );
  std::shuffle( v.begin() , v.end() , g );
  const Longs X( v.begin() , v.begin() + n );
  const Longs Y( v.begin() + n , v.begin() + 2 * n );
  int best = 0;
  everyChoice( Ints( n , n + 1 ) , [ & ]( const Ints & a ) {
   Ints used( n , 0 );
   int k = 0;
   for( int i = 0 ; i < n ; ++i )
    if( a[ i ] > 0 ) {
     if( ! flow::square( X[ i ] + Y[ a[ i ] - 1 ] ) || used[ a[ i ] - 1 ]++ )
      return;
     ++k;
     }
   best = std::max( best , k );
   } );
  CHECK( flow::squarePairs( X , Y ) == best );
  }
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
/*------------------------- End File exam_flow.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
