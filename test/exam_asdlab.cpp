/*--------------------------------------------------------------------------*/
/*-------------------------- File exam_asdlab.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the programming projects (batman, flatland, pokemon, lich,
 * laser, nassau, alpini, got) and of the laboratory exercises that go with
 * the book: each problem is solved by its official solution, in the
 * namespace asdlab, and checked on the examples of its statement, against an
 * independent oracle on small instances generated with a fixed seed (a
 * brute force, a closed formula, a simulation, the standard library), and
 * on the edge cases, at the level 3 of the contracts. The two optimization
 * projects (alpini and got) have heuristics as official solutions: their
 * outputs are checked by a feasibility test and scored against a simple
 * baseline and, on small instances, against the optimum.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, the programming
 *      projects and the laboratory exercises
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

#include "ADS/Contract.h"
#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/algorithm/Subsequences.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/Visit.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/Rational.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*------------------------------- SOLUTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace asdlab {

using std::int64_t;
using std::uint64_t;
using std::size_t;
using Arc = std::pair< int , int >;
using Arcs = std::vector< Arc >;
using Grid = std::vector< std::vector< int > >;

/// the static graph of n nodes and of the arcs A, the loops left out and
/// the parallel arcs kept
inline ArrayGraph<> arrayGraph( int n , const Arcs & A , bool directed = true )
{
 std::vector< std::tuple< int , int , std::monostate > > E;
 for( const auto & a : A )
  if( a.first != a.second )
   E.emplace_back( a.first , a.second , std::monostate() );
 return( ArrayGraph<>( n , E , directed ) );
 }

/// whether every arc of A joins two nodes in [ 0 , n )
inline bool validArcs( int n , const Arcs & A )
{
 return( std::all_of( A.begin() , A.end() , [ n ]( const Arc & a ) {
  return( ( a.first >= 0 ) && ( a.first < n ) && ( a.second >= 0 ) &&
          ( a.second < n ) );
  } ) );
 }

/*------------------------- 2014, Batman si annoia -------------------------*/

/// an arc x -> y of a directed graph holds an enemy when y does not reach
/// x; the number of distinct sets of enemies met by the walks from s to d.
/// The enemies are the arcs between distinct strongly connected components,
/// and a walk meets exactly the arcs of its path in the condensation, which
/// is acyclic: the sets are the paths from the component of s to that of
/// d, parallel arcs counted apart. scc(), then the number of paths in a
/// topological order of the condensation (Kahn); O( n + m ). The counts
/// are summed modulo 2^64, exact when the answer fits (the statement bounds
/// it by 10^9), where the official code sums in int.
/// \pre n > 0, s and d are nodes, the arcs join nodes
uint64_t batman( int n , const Arcs & A , int s , int d )
{
 ADS_PRE( ( n > 0 ) && ( s >= 0 ) && ( s < n ) && ( d >= 0 ) && ( d < n ) &&
          validArcs( n , A ) );
 const auto C = scc( arrayGraph( n , A ) );
 const int k = *std::max_element( C.begin() , C.end() );
 Arcs D;                                       // the condensation
 for( const auto & a : A )
  if( C[ a.first ] != C[ a.second ] )
   D.emplace_back( C[ a.first ] - 1 , C[ a.second ] - 1 );
 const auto cg = arrayGraph( k , D );
 std::vector< uint64_t > np( k , 0 );          // the paths from C[ s ]
 np[ C[ s ] - 1 ] = 1;
 for( int c : topsortKahn( cg ) )
  cg.forEachAdj( c , [ & ]( int e ) { np[ e ] += np[ c ]; } );
 return( np[ C[ d ] - 1 ] );
 }

/*----------------------- 2014, Natale a Flatlandia ------------------------*/

/// a row of triangles facing left ('s') or right ('d'); a triangle that is
/// neither the first nor the last of the row leaves it with the one it
/// faces, until one is left: the places of the triangles that can be the
/// last one, in increasing order. X is the last iff [ 0 , X ) and
/// ( X , n ) can be emptied; the pairs removed together form a non-crossing
/// matching, so an interval DP on the partner z of its first triangle x:
/// ( x , z ) is emptied first, between x and z, and ( z , y ] apart. The
/// flags of the write-up, x the first of the row (T) and y the last (T),
/// occur only as FF, TF (the left part, where x cannot be chosen, so z
/// faces left) and FT (the right part, where z can face left only if
/// something is left on its right); O( n^3 ) time, O( n^2 ) space.
/// \pre n is odd, every character is 's' or 'd'
std::vector< int > flatland( const std::string & s )
{
 const int n = int( s.size() );
 ADS_PRE( ( n % 2 == 1 ) && std::all_of( s.begin() , s.end() , []( char c ) {
  return( ( c == 's' ) || ( c == 'd' ) ); } ) );
 // whether [ x , y ) can be emptied, by the flags
 std::vector< std::vector< char > > FF( n + 1 ,
                                        std::vector< char >( n + 1 , 0 ) );
 auto TF = FF , FT = FF;
 for( int x = 0 ; x <= n ; ++x )
  FF[ x ][ x ] = TF[ x ][ x ] = FT[ x ][ x ] = 1;
 for( int len = 2 ; len <= n ; len += 2 )
  for( int x = 0 ; x + len <= n ; ++x ) {
   const int y = x + len;
   for( int z = x + 1 ; z < y ; z += 2 ) {      // the partner of x
    if( ! FF[ x + 1 ][ z ] )
     continue;
    const bool byX = s[ x ] == 'd' , byZ = s[ z ] == 's';
    if( ( byX || byZ ) && FF[ z + 1 ][ y ] )
     FF[ x ][ y ] = 1;
    if( byZ && FF[ z + 1 ][ y ] )
     TF[ x ][ y ] = 1;
    if( ( byX || ( byZ && ( z + 1 < y ) ) ) && FT[ z + 1 ][ y ] )
     FT[ x ][ y ] = 1;
    }
   }
 std::vector< int > S;
 for( int x = 0 ; x < n ; x += 2 )
  if( TF[ 0 ][ x ] && FT[ x + 1 ][ n ] )
   S.push_back( x );
 return( S );
 }

/*--------------------------- 2016, Pokemon Park ---------------------------*/

/// the adjacency of an undirected multigraph: ( node , edge ) pairs
using Incidence = std::vector< std::vector< std::pair< int , int > > >;

/// the bridges of the graph A without the edges off, found by the low
/// points of the depth-first visits (Tarjan) from the nodes of S; the edge
/// to the parent is skipped by its name, so that parallel edges are no
/// bridges; O( n + m )
inline std::vector< int > bridges( const Incidence & A ,
                                   const std::vector< char > & off ,
                                   const std::vector< int > & S )
{
 const int n = int( A.size() );
 std::vector< int > pre( n , -1 ) , low( n , 0 ) , B;
 int count = 0;
 auto visit = [ & ]( auto & self , int u , int pe ) -> void {
  pre[ u ] = low[ u ] = count++;
  for( const auto & [ v , e ] : A[ u ] ) {
   if( off[ e ] || ( e == pe ) )
    continue;
   if( pre[ v ] < 0 ) {
    self( self , v , e );
    low[ u ] = std::min( low[ u ] , low[ v ] );
    if( low[ v ] > pre[ u ] )
     B.push_back( e );
    }
   else
    low[ u ] = std::min( low[ u ] , pre[ v ] );
   }
  };
 for( int r : S )
  if( pre[ r ] < 0 )
   visit( visit , r , -1 );
 return( B );
 }

/// the answer of pokemon: the number K of species and the species of each
/// edge
struct Pokemon {
 int K;                        ///< the most species
 std::vector< int > species;   ///< the species of each edge
 };

/// the edges of an undirected graph get species 0 , ... , K - 1 so that
/// every closed trail meets every species equally often: the largest K and
/// such species. A cycle takes all or none of the edges of a class, two
/// edges being in one class when removing one makes the other a bridge;
/// the bridges are in no cycle. The bridges first, then, for each edge a
/// of no class yet, the bridges of the graph without a found from an end
/// of a: they and a are its class. K is the gcd of the sizes of the
/// classes, and each class takes its species in turn modulo K;
/// O( m ( n + m ) ). The official code skips the edge to the parent by
/// its node, which also skips a parallel edge.
/// \pre the edges join distinct nodes in [ 0 , n )
Pokemon pokemon( int n , const Arcs & E )
{
 ADS_PRE( ( n > 0 ) && validArcs( n , E ) &&
          std::all_of( E.begin() , E.end() , []( const Arc & e ) {
           return( e.first != e.second ); } ) );
 const int m = int( E.size() );
 Incidence A( n );
 for( int e = 0 ; e < m ; ++e ) {
  A[ E[ e ].first ].emplace_back( E[ e ].second , e );
  A[ E[ e ].second ].emplace_back( E[ e ].first , e );
  }
 std::vector< int > all( n );
 std::iota( all.begin() , all.end() , 0 );
 std::vector< char > off( m , 0 );
 for( int e : bridges( A , off , all ) )
  off[ e ] = 1;
 const std::vector< char > bridge = off;
 std::vector< int > group( m , -1 ) , sizes;
 for( int a = 0 ; a < m ; ++a ) {
  if( bridge[ a ] || ( group[ a ] >= 0 ) )
   continue;
  const int g = int( sizes.size() );
  group[ a ] = g;
  off[ a ] = 1;
  const auto B = bridges( A , off , { E[ a ].first } );
  off[ a ] = 0;
  for( int e : B )
   group[ e ] = g;
  sizes.push_back( 1 + int( B.size() ) );
  }
 U64 K = sizes.empty() ? 1 : 0;
 for( int c : sizes )
  K = gcd( U64( c ) , K );
 Pokemon P{ int( K ) , std::vector< int >( m , 0 ) };
 std::vector< int > next( sizes.size() , 0 );
 for( int e = 0 ; e < m ; ++e )
  if( ! bridge[ e ] )
   P.species[ e ] = next[ group[ e ] ]++ % P.K;
 return( P );
 }

/*--------------------- 2016, La vendetta del Re Lich ----------------------*/

/// an edge of a weighted tree
struct WEdge {
 int u , v , w;   ///< the ends and the weight
 };

/// the distance of each node of a weighted tree from its farthest node
/// (a leaf), by rerooting: the deepest leaf below each node in a visit
/// from 0, then the farthest one through the parent, from the two deepest
/// children of the parent; O( n ), with no recursion
/// \pre the edges make a tree on [ 0 , n ), n > 0
inline std::vector< int64_t > farthest( int n ,
                                       const std::vector< WEdge > & E )
{
 std::vector< std::vector< std::pair< int , int > > > A( n );
 for( const auto & e : E ) {
  A[ e.u ].emplace_back( e.v , e.w );
  A[ e.v ].emplace_back( e.u , e.w );
  }
 std::vector< int > order = { 0 } , par( n , -1 ) , pw( n , 0 );
 for( size_t i = 0 ; i < order.size() ; ++i )
  for( const auto & [ v , w ] : A[ order[ i ] ] )
   if( v != par[ order[ i ] ] ) {
    par[ v ] = order[ i ];
    pw[ v ] = w;
    order.push_back( v );
    }
 ADS_PRE( int( order.size() ) == n );
 std::vector< int64_t > down( n , 0 ) , best1( n , 0 ) , best2( n , 0 ) ,
                        up( n , 0 );
 std::vector< int > child( n , -1 );           // the deepest child
 for( int i = n - 1 ; i > 0 ; --i ) {
  const int v = order[ i ] , p = par[ v ];
  const int64_t h = down[ v ] + pw[ v ];
  if( h > best1[ p ] ) {
   best2[ p ] = best1[ p ];
   best1[ p ] = h;
   child[ p ] = v;
   }
  else if( h > best2[ p ] )
   best2[ p ] = h;
  down[ p ] = best1[ p ];
  }
 for( int i = 1 ; i < n ; ++i ) {
  const int v = order[ i ] , p = par[ v ];
  const int64_t via = child[ p ] == v ? best2[ p ] : best1[ p ];
  up[ v ] = pw[ v ] + std::max( up[ p ] , via );
  }
 for( int v = 0 ; v < n ; ++v )
  down[ v ] = std::max( down[ v ] , up[ v ] );
 return( down );
 }

/// the farthest-leaf distance d of each node of a weighted tree, and for
/// each L the largest connected set of nodes with max d - min d <= L.
/// Rooted at a node of least d, each parent has d at most that of its
/// children; the nodes by decreasing d, each joined to its children in a
/// MFSET, and those with d larger than the current d + L dropped from
/// their set by a second pointer: the set of the current node is then the
/// best one with it as the minimum; O( n log n + K n alpha( n ) ).
/// \pre the edges make a tree on [ 0 , n ), n > 0, every L >= 0
std::vector< int > lich( int n , const std::vector< WEdge > & E ,
                         const std::vector< int64_t > & Ls )
{
 ADS_PRE( ( n > 0 ) && ( int( E.size() ) == n - 1 ) &&
          std::all_of( Ls.begin() , Ls.end() ,
                       []( int64_t l ) { return( l >= 0 ); } ) );
 const auto d = farthest( n , E );
 std::vector< std::vector< int > > A( n );
 for( const auto & e : E ) {
  A[ e.u ].push_back( e.v );
  A[ e.v ].push_back( e.u );
  }
 const int r = int( std::min_element( d.begin() , d.end() ) - d.begin() );
 std::vector< int > par( n , -1 ) , order = { r };
 for( size_t i = 0 ; i < order.size() ; ++i )
  for( int v : A[ order[ i ] ] )
   if( v != par[ order[ i ] ] ) {
    par[ v ] = order[ i ];
    order.push_back( v );
    }
 std::stable_sort( order.begin() , order.end() ,
                   [ & ]( int a , int b ) { return( d[ a ] > d[ b ] ); } );
 std::vector< int > out;
 for( int64_t L : Ls ) {
  MFSet S( n );
  std::vector< int > cnt( n , 1 );             // the nodes left, by root
  int best = 0;
  size_t it = 0;
  for( int u : order ) {
   for( int v : A[ u ] )
    if( v != par[ u ] ) {
     const int ru = S.find( u ) , rv = S.find( v );
     if( S.merge( ru , rv ) )
      cnt[ S.find( ru ) ] = cnt[ ru ] + cnt[ rv ];
     }
   while( d[ order[ it ] ] - d[ u ] > L )
    --cnt[ S.find( order[ it++ ] ) ];
   best = std::max( best , cnt[ S.find( u ) ] );
   }
  out.push_back( best );
  }
 return( out );
 }

/*---------------------- 2017, 007: Pioggia di laser -----------------------*/

/// an edge of laser: crossing it takes w and must fit in one window
/// [ f + k ( off + on ) , f + k ( off + on ) + off ) when the laser is off
struct Laser {
 int s , t , w , f , off , on;   ///< ends, time, first off, periods
 };

/// the earliest time >= t at which e can be crossed
/// \pre the off period is at least the crossing time
inline int64_t laserDeparture( const Laser & e , int64_t t )
{
 ADS_PRE( e.off >= e.w );
 if( t < e.f )
  return( e.f );
 const int64_t c = int64_t( e.off ) + e.on , pos = ( t - e.f ) % c;
 return( e.off - pos >= e.w ? t : t - pos + c );
 }

/// the earliest arrival at n - 1 from 0 at time 0, waiting allowed, and a
/// path achieving it; -1 and no path if n - 1 is not reachable. The edges
/// shorter than their crossing are dropped, then Dijkstra with the arrival
/// through e at laserDeparture( e , t ) + w, which does not decrease in t
/// (the FIFO property), so the earliest arrival at a node is the best one
/// to leave from; O( ( n + m ) log n ) with a binary heap.
/// \pre n > 0, the edges join nodes, w > 0, f >= 0, off >= 0, on >= 0,
///      off + on > 0
std::pair< int64_t , std::vector< int > > laser(
                                       int n , const std::vector< Laser > & E )
{
 ADS_PRE( ( n > 0 ) && std::all_of( E.begin() , E.end() ,
                                     [ n ]( const Laser & e ) {
  return( ( e.s >= 0 ) && ( e.s < n ) && ( e.t >= 0 ) && ( e.t < n ) &&
          ( e.w > 0 ) && ( e.f >= 0 ) && ( e.off >= 0 ) && ( e.on >= 0 ) &&
          ( e.off + e.on > 0 ) ); } ) );
 std::vector< std::vector< int > > A( n );
 for( int i = 0 ; i < int( E.size() ) ; ++i )
  if( E[ i ].off >= E[ i ].w ) {
   A[ E[ i ].s ].push_back( i );
   A[ E[ i ].t ].push_back( i );
   }
 constexpr int64_t inf = std::numeric_limits< int64_t >::max();
 std::vector< int64_t > dist( n , inf );
 std::vector< int > pred( n , -1 );
 using Item = std::pair< int64_t , int >;
 std::priority_queue< Item , std::vector< Item > , std::greater<> > Q;
 dist[ 0 ] = 0;
 Q.emplace( 0 , 0 );
 while( ! Q.empty() ) {
  const auto [ t , u ] = Q.top();
  Q.pop();
  if( t > dist[ u ] )
   continue;
  for( int i : A[ u ] ) {
   const int v = E[ i ].s == u ? E[ i ].t : E[ i ].s;
   const int64_t a = laserDeparture( E[ i ] , t ) + E[ i ].w;
   if( a < dist[ v ] ) {
    dist[ v ] = a;
    pred[ v ] = u;
    Q.emplace( a , v );
    }
   }
  }
 if( dist[ n - 1 ] == inf )
  return( std::pair( int64_t( -1 ) , std::vector< int >() ) );
 std::vector< int > P;
 for( int u = n - 1 ; u != -1 ; u = pred[ u ] )
  P.push_back( u );
 std::reverse( P.begin() , P.end() );
 ADS_POST( P.front() == 0 );
 return( std::pair( dist[ n - 1 ] , P ) );
 }

/*------------------------- 2017, Assalto a Nassau -------------------------*/

/// V ships of 2 hit points and F of 1; M shots, each on a ship afloat
/// chosen uniformly: the expected final ( ships of 2 afloat ) x ( ships of
/// 1 afloat ). By the linearity of the expectation it is V F p, p the
/// probability that a given pair survives; p over the states ( hit points
/// of the other ships of 2, of those of 1, whether the ship of the pair is
/// hit ), the shots fired being implied by the hit points left, with two
/// layers over the ships of 2; O( V ( V + F ) ) time, O( V + F ) space.
/// \pre V , F , M >= 0
/// \post 0 <= r <= V F
double nassau( int V , int F , int M )
{
 ADS_PRE( ( V >= 0 ) && ( F >= 0 ) && ( M >= 0 ) );
 if( ( V == 0 ) || ( F == 0 ) )
  return( 0 );
 const int H = V + F + 1;
 std::array< std::array< std::vector< double > , 2 > , 2 > P;
 for( auto & layer : P )
  for( auto & row : layer )
   row.assign( H , 0.0 );
 for( int h2 = 0 ; h2 < V ; ++h2 )
  for( int w = 1 ; w >= 0 ; --w )
   for( int h1 = 0 ; h1 + 1 < H ; ++h1 ) {
    const int shot = ( 2 * V + F ) - ( 2 * h2 + h1 + ( 2 - w ) + 1 );
    double p = 0;
    if( shot == M )
     p = 1;
    else {
     const double tot = h1 + h2 + 2.0;
     if( w == 0 )
      p += P[ h2 % 2 ][ 1 ][ h1 ] / tot;
     if( h2 > 0 )
      p += P[ ( h2 - 1 ) % 2 ][ w ][ h1 + 1 ] * h2 / tot;
     if( h1 > 0 )
      p += P[ h2 % 2 ][ w ][ h1 - 1 ] * h1 / tot;
     }
    P[ h2 % 2 ][ w ][ h1 ] = p;
    }
 const double r = P[ ( V - 1 ) % 2 ][ 0 ][ F - 1 ] * V * F;
 ADS_POST( ( r >= -1e-9 ) && ( r <= double( V ) * F * ( 1 + 1e-9 ) ) );
 return( r );
 }

/*------------------------ 2017, Alpinocalypse Now -------------------------*/

/// a round of the greedy removal of alpini: the nodes with at most one
/// edge in the core leave it (they are in no cycle), then pick( deg , core )
/// removes a node of the core, until the core is empty; nothing if more
/// than limit nodes would be removed
template< class Pick >
std::optional< std::vector< int > > alpiniRound(
                          const std::vector< std::vector< int > > & A ,
                          Pick && pick , size_t limit )
{
 const int n = int( A.size() );
 std::vector< int > deg( n ) , Q , R;
 std::vector< char > core( n , 1 );
 int alive = n;
 for( int u = 0 ; u < n ; ++u )
  if( ( deg[ u ] = int( A[ u ].size() ) ) <= 1 )
   Q.push_back( u );
 auto leave = [ & ]( int u ) {
  core[ u ] = 0;
  --alive;
  for( int v : A[ u ] )
   if( core[ v ] && ( --deg[ v ] <= 1 ) )
    Q.push_back( v );
  };
 for( ; ; ) {
  while( ! Q.empty() ) {
   const int u = Q.back();
   Q.pop_back();
   if( core[ u ] )
    leave( u );
   }
  if( alive == 0 )
   return( R );
  if( R.size() >= limit )
   return( std::nullopt );
  const int u = pick( deg , core );
  R.push_back( u );
  leave( u );
  }
 }

/// the removed nodes R made minimal: each one, in turn, is kept again when
/// its kept neighbours lie in distinct trees of the forest (a MFSET)
inline std::vector< int > alpiniTrim( int n , const Arcs & E ,
                                      std::vector< int > R )
{
 std::vector< char > out( n , 0 );
 for( int u : R )
  out[ u ] = 1;
 std::vector< std::vector< int > > A( n );
 MFSet S( n );
 for( const auto & [ u , v ] : E ) {
  A[ u ].push_back( v );
  A[ v ].push_back( u );
  if( ! out[ u ] && ! out[ v ] )
   S.merge( u , v );
  }
 std::vector< int > K;
 for( int u : R ) {
  std::vector< int > roots;
  bool ok = true;
  for( int v : A[ u ] ) {
   if( v == u )
    ok = false;
   else if( ! out[ v ] )
    roots.push_back( S.find( v ) );
   }
  std::sort( roots.begin() , roots.end() );
  if( ok && ( std::adjacent_find( roots.begin() , roots.end() ) ==
              roots.end() ) ) {
   out[ u ] = 0;
   for( int v : A[ u ] )
    if( ! out[ v ] )
     S.merge( u , v );
   }
  else
   K.push_back( u );
  }
 return( K );
 }

/// a small set of nodes whose removal leaves the undirected multigraph
/// acyclic (a feedback vertex set, NP-hard), by the official heuristic:
/// the greedy removal of the node of largest degree in the 2-core, then
/// rounds removing a node with probability proportional to deg^5, each
/// stopped when it cannot beat the best; here each set is also made
/// minimal by alpiniTrim(). Fixed seed and rounds instead of the time
/// limit; O( n ( n + m ) ) per round. The removed nodes, increasing.
/// \pre n > 0, the edges join nodes (loops and parallel edges allowed)
std::vector< int > alpini( int n , const Arcs & E , int rounds = 300 ,
                           unsigned seed = 1 )
{
 ADS_PRE( ( n > 0 ) && validArcs( n , E ) && ( rounds >= 0 ) );
 std::vector< std::vector< int > > A( n );
 for( const auto & [ u , v ] : E ) {
  A[ u ].push_back( v );
  A[ v ].push_back( u );
  }
 auto maxDegree = []( const std::vector< int > & deg ,
                      const std::vector< char > & core ) {
  int b = -1;
  for( int u = 0 ; u < int( deg.size() ) ; ++u )
   if( core[ u ] && ( ( b < 0 ) || ( deg[ u ] > deg[ b ] ) ) )
    b = u;
  return( b );
  };
 auto best = alpiniTrim( n , E , *alpiniRound( A , maxDegree ,
                                               size_t( n ) + 1 ) );
 std::mt19937 g( seed );
 auto random = [ & ]( const std::vector< int > & deg ,
                      const std::vector< char > & core ) {
  std::vector< double > W( deg.size() , 0.0 );
  for( size_t u = 0 ; u < deg.size() ; ++u )
   if( core[ u ] )
    W[ u ] = std::pow( double( deg[ u ] ) , 5.0 );
  std::discrete_distribution< int > pickOne( W.begin() , W.end() );
  return( pickOne( g ) );
  };
 for( int r = 0 ; r < rounds ; ++r )
  if( auto R = alpiniRound( A , random , best.size() ) ) {
   auto T = alpiniTrim( n , E , *R );
   if( T.size() < best.size() )
    best = std::move( T );
   }
 std::sort( best.begin() , best.end() );
 return( best );
 }

/*------------------ 2018, Game of (Approximated) Thrones ------------------*/

/// a castle of got: its row, its column and its value
using Castle = std::array< int , 3 >;

/// a round of region growing of got: the castles in the order O, each one
/// not in a region yet growing a region of its value from its cell, one
/// admissible neighbour at a time, the best first (an unused castle of the
/// same value, then a cell next to one, then a cell next to no castle of
/// another value; among these, the one touching most cells of the region,
/// which keeps it compact, then the least rank), and kept only when
/// complete; a cell is admissible when it is free, holds no castle of
/// another value and touches no region of the same value. The number of
/// castles inside the regions; O( sum of v^2 ) over the castles of value v.
inline int gotRound( const Grid & A , const std::vector< Castle > & O ,
                     const Grid & rank , Grid & out )
{
 const int N = int( A.size() ) , M = int( A[ 0 ].size() );
 out.assign( N , std::vector< int >( M , 0 ) );
 Grid in( N , std::vector< int >( M , 0 ) );   // the region being grown
 int stamp = 0 , nv = 0;
 static constexpr int dx[] = { 1 , -1 , 0 , 0 } , dy[] = { 0 , 0 , 1 , -1 };
 auto inside = [ & ]( int x , int y ) {
  return( ( x >= 0 ) && ( x < N ) && ( y >= 0 ) && ( y < M ) );
  };
 auto admissible = [ & ]( int x , int y , int v ) {
  if( out[ x ][ y ] || ( in[ x ][ y ] == stamp ) ||
      ( A[ x ][ y ] && ( A[ x ][ y ] != v ) ) )
   return( false );
  for( int k = 0 ; k < 4 ; ++k )
   if( inside( x + dx[ k ] , y + dy[ k ] ) &&
       ( out[ x + dx[ k ] ][ y + dy[ k ] ] == v ) )
    return( false );
  return( true );
  };
 auto priority = [ & ]( int x , int y , int v ) {
  if( A[ x ][ y ] == v )
   return( 0 );
  int p = 2;
  for( int k = 0 ; k < 4 ; ++k ) {
   const int a = x + dx[ k ] , b = y + dy[ k ];
   if( ! inside( a , b ) || ( in[ a ][ b ] == stamp ) || out[ a ][ b ] ||
       ! A[ a ][ b ] )
    continue;
   if( A[ a ][ b ] == v )
    return( 1 );
   p = 3;
   }
  return( p );
  };
 for( const auto & [ x , y , v ] : O ) {
  ++stamp;
  if( ! admissible( x , y , v ) )
   continue;
  std::vector< std::pair< int , int > > R = { { x , y } };
  in[ x ][ y ] = stamp;
  while( int( R.size() ) < v ) {
   int bx = -1 , by = -1;
   std::tuple< int , int , int > bk( 4 , 0 , 0 );
   for( const auto & c : R )
    for( int k = 0 ; k < 4 ; ++k ) {
     const int a = c.first + dx[ k ] , b = c.second + dy[ k ];
     if( ! inside( a , b ) || ! admissible( a , b , v ) )
      continue;
     int touch = 0;                            // the cells of R next to it
     for( int h = 0 ; h < 4 ; ++h )
      touch += inside( a + dx[ h ] , b + dy[ h ] ) &&
               ( in[ a + dx[ h ] ][ b + dy[ h ] ] == stamp );
     const std::tuple< int , int , int > key( priority( a , b , v ) , -touch ,
                                               rank[ a ][ b ] );
     if( key < bk ) {
      bk = key;
      bx = a;
      by = b;
      }
     }
   if( bx < 0 )
    break;
   in[ bx ][ by ] = stamp;
   R.emplace_back( bx , by );
   }
  if( int( R.size() ) < v )
   continue;
  for( const auto & [ a , b ] : R ) {
   out[ a ][ b ] = v;
   if( A[ a ][ b ] == v )
    ++nv;
   }
  }
 return( nv );
 }

/// a grid of castles (0 for none); a grid of regions, each a 4-connected
/// set of k cells of value k with at least one castle k and none of another
/// value, the other cells 0, with many castles in the regions (a
/// Fillomino-like problem, NP-hard). The official heuristic: region growing
/// from each castle towards the unused castles of its value and away from
/// the others, repeated on shuffled orders of the castles, keeping the best;
/// here the first round takes the castles by increasing value, the second
/// by decreasing value, the others in random orders, and the ties are
/// broken by random ranks. Fixed seed and rounds instead of the time limit.
/// \pre the grid is not empty and rectangular, the values >= 0
Grid got( const Grid & A , int rounds = 40 , unsigned seed = 1 )
{
 ADS_PRE( ! A.empty() && ! A[ 0 ].empty() && ( rounds >= 2 ) &&
          std::all_of( A.begin() , A.end() , [ & ]( const auto & r ) {
           return( ( r.size() == A[ 0 ].size() ) &&
                   std::all_of( r.begin() , r.end() ,
                                []( int x ) { return( x >= 0 ); } ) );
           } ) );
 const int N = int( A.size() ) , M = int( A[ 0 ].size() );
 std::vector< Castle > O;
 for( int x = 0 ; x < N ; ++x )
  for( int y = 0 ; y < M ; ++y )
   if( A[ x ][ y ] > 0 )
    O.push_back( { x , y , A[ x ][ y ] } );
 std::stable_sort( O.begin() , O.end() , []( const Castle & a ,
                                            const Castle & b ) {
  return( a[ 2 ] < b[ 2 ] ); } );
 Grid rank( N , std::vector< int >( M , 0 ) ) , out;
 int bestNv = gotRound( A , O , rank , out );
 Grid best = out;
 std::mt19937 g( seed );
 for( int r = 1 ; r < rounds ; ++r ) {
  if( r == 1 )
   std::reverse( O.begin() , O.end() );
  else
   std::shuffle( O.begin() , O.end() , g );
  for( auto & row : rank )
   for( auto & x : row )
    x = int( g() >> 1 );
  const int nv = gotRound( A , O , rank , out );
  if( nv > bestNv ) {
   bestNv = nv;
   best = out;
   }
  }
 return( best );
 }

/*-------------------- 2026-09-24, Somma di due numeri ---------------------*/

/// N + M, in 64 bits
inline int64_t somma( int64_t N , int64_t M )
{
 return( N + M );
 }

/*--------------- 2026-09-24, Sottosequenza di somma massima ---------------*/

/// the largest sum of a run of consecutive items, the empty run counting
/// 0: maxSubarraySum() of the library (the scan of Kadane), O( n )
inline int64_t sottoseq( const std::vector< int64_t > & A )
{
 return( maxSubarraySum( A ) );
 }

/*--------------- 2026-09-24, Sottomatrice di somma massima ----------------*/

/// the largest sum of a rectangle of consecutive rows and columns, the
/// empty one counting 0: for each pair of rows, the sums of the columns
/// between them, and the largest run of these (Kadane); O( R^2 C ) time,
/// O( C ) space
/// \pre the matrix is rectangular
int64_t sottomat( const std::vector< std::vector< int64_t > > & A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( const auto & r ) {
  return( r.size() == A[ 0 ].size() ); } ) );
 int64_t best = 0;
 for( size_t top = 0 ; top < A.size() ; ++top ) {
  std::vector< int64_t > S( A[ top ].size() , 0 );
  for( size_t bot = top ; bot < A.size() ; ++bot ) {
   for( size_t c = 0 ; c < S.size() ; ++c )
    S[ c ] += A[ bot ][ c ];
   best = std::max( best , maxSubarraySum( S ) );
   }
  }
 return( best );
 }

/*-------------------------- 2026-10-22, Sorting ---------------------------*/

/// A sorted by mergeSort() of the library, O( n log n )
inline void sortLab( std::vector< int > & A )
{
 mergeSort( A );
 }

/*------------------------- 2026-10-22, Intervalli -------------------------*/

/// the longest period between the first start and the last end covered by
/// no closed interval [ a , b ], as ( start , end ), the first one on ties,
/// nothing if there is none: largestGap() of the library, the intervals by
/// start, and a gap whenever a start is past the farthest end so far;
/// O( n log n )
/// \pre I is not empty, a <= b for each interval
std::optional< std::pair< int , int > > intervalli(
                              const std::vector< std::pair< int , int > > & I )
{
 ADS_PRE( ! I.empty() );
 return( largestGap( I ) );
 }

/*----------------------- 2026-10-22, Funzioni ceil ------------------------*/

/// for each v of V the smallest item of the sorted A larger than v, -1 if
/// none: upperBound() of the library, O( |V| log |A| )
/// \pre A is sorted
std::vector< int > ceilLab( const std::vector< int > & V ,
                            const std::vector< int > & A )
{
 ADS_PRE( std::is_sorted( A.begin() , A.end() ) );
 std::vector< int > out;
 for( int v : V ) {
  const size_t r = upperBound( A , v );
  out.push_back( r < A.size() ? A[ r ] : -1 );
  }
 return( out );
 }

/*--------------------- 2026-10-22, Ordinamento pesato ---------------------*/

/// a permutation of 1 , ... , n sorted by swaps, each costing the sum of
/// the two items: the fewest swaps, n minus the number of cycles, and the
/// least cost, each cycle of k > 1 items of sum s and least item m costing
/// min( s + ( k - 2 ) m , s + m + ( k + 1 ) ), the second borrowing the
/// item 1: minimumSwapSort() of the library; O( n log n ). The two answers
/// may come from different sequences.
/// \pre A is a permutation of 1 , ... , n
std::pair< int , int64_t > sortpesato( const std::vector< int > & A )
{
 const int n = int( A.size() );
 std::vector< int > S( A );
 std::sort( S.begin() , S.end() );
 for( int i = 0 ; i < n ; ++i )
  ADS_PRE( S[ i ] == i + 1 );
 const auto [ swaps , cost ] = minimumSwapSort< int , int64_t >( A );
 return( std::pair( int( swaps ) , cost ) );
 }

/*-------------------- 2026-10-29, Visita di un albero ---------------------*/

/// the three visits of a tree
struct Visits {
 std::vector< int > pre , in , post;   ///< preorder, inorder, postorder
 };

/// the preorder, the inorder (the first subtree, the root, the others) and
/// the postorder of the tree of the parents P rooted in r, the children in
/// increasing order: pre_visit(), in_visit() and post_visit() of the
/// library on a Tree built from P; O( n )
/// \pre P[ r ] = -1 and P makes a tree rooted in r
Visits visitaAlbero( const std::vector< int > & P , int r )
{
 const int n = int( P.size() );
 ADS_PRE( ( r >= 0 ) && ( r < n ) && ( P[ r ] == -1 ) );
 for( int u = 0 ; u < n ; ++u )
  ADS_PRE( ( u == r ) || ( ( P[ u ] >= 0 ) && ( P[ u ] < n ) ) );
 std::vector< Tree< int > * > T( n );
 for( int u = 0 ; u < n ; ++u )
  T[ u ] = new Tree< int >( u );
 std::unique_ptr< Tree< int > > root( T[ r ] );
 std::vector< Tree< int > * > last( n , nullptr );
 for( int u = 0 ; u < n ; ++u )
  if( u != r ) {
   if( last[ P[ u ] ] )
    last[ P[ u ] ]->insertSibling( T[ u ] );
   else
    T[ P[ u ] ]->insertChild( T[ u ] );
   last[ P[ u ] ] = T[ u ];
   }
 Visits V;
 pre_visit( root.get() , [ & ]( Tree< int > * t ) {
  V.pre.push_back( t->read() ); } );
 in_visit( root.get() , 1 , [ & ]( Tree< int > * t ) {
  V.in.push_back( t->read() ); } );
 post_visit( root.get() , [ & ]( Tree< int > * t ) {
  V.post.push_back( t->read() ); } );
 ADS_POST( int( V.pre.size() ) == n );
 return( V );
 }

/*----------------- 2026-10-29, Visita di grafo orientato ------------------*/

/// the number of nodes reachable from s, s included: bfs() of the library
/// on the static graph; O( n + m )
/// \pre s is a node, the arcs join nodes
int visitaGrafo( int n , const Arcs & A , int s )
{
 ADS_PRE( ( s >= 0 ) && ( s < n ) && validArcs( n , A ) );
 int c = 0;
 bfs( arrayGraph( n , A ) , s , [ & ]( int ) { ++c; } , []( int , int ) {} );
 return( c );
 }

/*-------------- 2026-10-29, Diametro su grafo non orientato ---------------*/

/// the largest distance in edges between two nodes of an undirected graph,
/// over the pairs joined by a path: distance() of the library from each
/// node; O( n ( n + m ) )
/// \pre n > 0, the edges join nodes
int diametro( int n , const Arcs & E )
{
 ADS_PRE( ( n > 0 ) && validArcs( n , E ) );
 const auto g = arrayGraph( n , E , false );
 int best = 0;
 for( int s = 0 ; s < n ; ++s )
  for( int x : distance( g , s ).first )
   if( x != INF )
    best = std::max( best , x );
 return( best );
 }

/*---------------------- 2026-11-19, Alberi Red-Black ----------------------*/

/// the Red-Black tree of the keys inserted in order: the RedBlackTree of
/// the library, O( n log n )
inline RedBlackTree< int , int > redBlack( const std::vector< int > & K )
{
 RedBlackTree< int , int > T;
 for( int k : K )
  T.insert( k , 0 );
 return( T );
 }

/*----------------- 2026-11-19, Individuazione di un ciclo -----------------*/

/// whether a directed graph has a cycle: hasCycle() of the library, a
/// back edge of a depth-first visit; O( n + m )
/// \pre the arcs join nodes and none is a loop
inline bool ciclo( int n , const Arcs & A )
{
 ADS_PRE( validArcs( n , A ) );
 return( hasCycle( arrayGraph( n , A ) ) );
 }

/*------------------ 2026-11-19, Numero di cammini minimi ------------------*/

/// the number of shortest paths, in arcs, from s to t: countShortestPaths()
/// of the library, a breadth first visit in which each node passes its
/// count to the nodes one arc farther; O( n + m ), modulo 2^64
/// \pre s and t are nodes, the arcs join nodes
uint64_t numCammini( int n , const Arcs & A , int s , int t )
{
 ADS_PRE( ( s >= 0 ) && ( s < n ) && ( t >= 0 ) && ( t < n ) &&
          validArcs( n , A ) );
 return( countShortestPaths( arrayGraph( n , A ) , s )[ t ] );
 }

/*--------------- 2026-11-26, Componente fortemente connessa ---------------*/

/// the size of the largest strongly connected component: scc() of the
/// library; O( n + m )
/// \pre n > 0, the arcs join nodes
int componente( int n , const Arcs & A )
{
 ADS_PRE( ( n > 0 ) && validArcs( n , A ) );
 const auto C = scc( arrayGraph( n , A ) );
 std::vector< int > size( n + 1 , 0 );
 for( int c : C )
  ++size[ c ];
 return( *std::max_element( size.begin() , size.end() ) );
 }

/*------------------- 2026-11-26, Ordinamento topologico -------------------*/

/// a topological order of a directed acyclic graph: topsort() of the
/// library; O( n + m )
/// \pre the arcs join nodes
inline std::vector< int > topOrder( int n , const Arcs & A )
{
 ADS_PRE( validArcs( n , A ) );
 return( topsort( arrayGraph( n , A ) ) );
 }

/*--------------------- 2026-11-26, Cammino piu' lungo ---------------------*/

/// the most arcs of a path of a directed acyclic graph: the largest layer
/// of longestPathLayering() of the library, each arc u -> v giving L[ v ]
/// at least L[ u ] + 1 in topological order; O( n + m )
/// \pre the arcs join nodes and make no cycle
int camminoLungo( int n , const Arcs & A )
{
 ADS_PRE( validArcs( n , A ) );
 const auto L = longestPathLayering( arrayGraph( n , A ) );
 return( L.empty() ? 0 : *std::max_element( L.begin() , L.end() ) );
 }

}  // end( namespace asdlab )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using namespace asdlab;

/// the reachability of a directed graph of at most 64 nodes, by
/// Floyd-Warshall, each node reaching itself
std::vector< std::vector< char > > closure( int n , const Arcs & A )
{
 std::vector< std::vector< char > > R( n , std::vector< char >( n , 0 ) );
 for( int u = 0 ; u < n ; ++u )
  R[ u ][ u ] = 1;
 for( const auto & [ u , v ] : A )
  R[ u ][ v ] = 1;
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   if( R[ i ][ k ] )
    for( int j = 0 ; j < n ; ++j )
     if( R[ k ][ j ] )
      R[ i ][ j ] = 1;
 return( R );
 }

/// m random arcs on n nodes, loops if loops, none if no arc can be drawn
Arcs randomArcs( std::mt19937 & g , int n , int m , bool loops )
{
 Arcs A;
 while( ( loops || ( n > 1 ) ) && ( int( A.size() ) < m ) ) {
  const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( loops || ( u != v ) )
   A.emplace_back( u , v );
  }
 return( A );
 }

/// m distinct random arcs on n nodes, no loop, at most n ( n - 1 )
Arcs simpleArcs( std::mt19937 & g , int n , int m )
{
 std::set< Arc > S;
 m = std::min( m , n * ( n - 1 ) );
 while( int( S.size() ) < m ) {
  const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( u != v )
   S.emplace( u , v );
  }
 return( Arcs( S.begin() , S.end() ) );
 }

/// a random acyclic graph: m distinct arcs from a smaller to a larger name
/// of a random permutation
Arcs randomDag( std::mt19937 & g , int n , int m )
{
 std::vector< int > p( n );
 std::iota( p.begin() , p.end() , 0 );
 std::shuffle( p.begin() , p.end() , g );
 std::set< Arc > S;
 m = std::min( m , n * ( n - 1 ) / 2 );
 while( int( S.size() ) < m ) {
  int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
  if( u == v )
   continue;
  if( u > v )
   std::swap( u , v );
  S.emplace( p[ u ] , p[ v ] );
  }
 return( Arcs( S.begin() , S.end() ) );
 }

/*------------------------- 2014, Batman si annoia -------------------------*/

// the sets of enemies, as the states ( node , set of enemies met ) that
// the walks from s reach, the enemies found by the closure
uint64_t batmanBrute( int n , const Arcs & A , int s , int d )
{
 const auto R = closure( n , A );
 const int m = int( A.size() );
 std::set< std::pair< int , unsigned > > seen = { { s , 0u } };
 std::vector< std::pair< int , unsigned > > Q = { { s , 0u } };
 std::set< unsigned > at;
 while( ! Q.empty() ) {
  const auto [ u , mask ] = Q.back();
  Q.pop_back();
  if( u == d )
   at.insert( mask );
  for( int i = 0 ; i < m ; ++i )
   if( A[ i ].first == u ) {
    const unsigned nm = R[ A[ i ].second ][ u ] ? mask : mask | ( 1u << i );
    if( seen.emplace( A[ i ].second , nm ).second )
     Q.emplace_back( A[ i ].second , nm );
    }
  }
 return( at.size() );
 }

// the example of the statement, the walks of random small graphs against
// the brute force, and a long layered acyclic graph against the count of
// its paths layer by layer
ADS_TEST( "asdlab-batman" )
{
 const Arcs ex = { { 0 , 10 } , { 1 , 7 } , { 2 , 0 } , { 2 , 3 } ,
                   { 2 , 6 } , { 3 , 4 } , { 4 , 5 } , { 5 , 3 } ,
                   { 6 , 0 } , { 6 , 5 } , { 7 , 9 } , { 8 , 3 } ,
                   { 8 , 7 } , { 9 , 8 } , { 9 , 10 } , { 10 , 2 } };
 CHECK( batman( 11 , ex , 7 , 5 ) == 3 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 600 ; ++it ) {
  const int n = 1 + int( g() % 7 ) , m = int( g() % 13 );
  const auto A = randomArcs( g , n , m , true );
  const int s = int( g() % unsigned( n ) ) , d = int( g() % unsigned( n ) );
  CHECK( batman( n , A , s , d ) == batmanBrute( n , A , s , d ) );
  }
 CHECK( batman( 1 , {} , 0 , 0 ) == 1 );                 // the empty set
 CHECK( batman( 2 , { { 1 , 0 } } , 0 , 1 ) == 0 );      // no walk
 CHECK( batman( 3 , { { 0 , 1 } , { 1 , 0 } , { 1 , 2 } , { 2 , 1 } } , 0 ,
                2 ) == 1 );                              // one component
 Arcs P;                                       // 30 pairs of parallel arcs
 for( int i = 0 ; i < 30 ; ++i ) {
  P.emplace_back( i , i + 1 );
  P.emplace_back( i , i + 1 );
  }
 CHECK( batman( 31 , P , 0 , 30 ) == ( uint64_t( 1 ) << 30 ) );
 const int W = 5 , H = 400;                    // the layers
 Arcs L;
 std::vector< uint64_t > c( W , 1 );
 for( int h = 0 ; h + 1 < H ; ++h ) {
  std::vector< uint64_t > nc( W , 0 );
  for( int k = 0 ; k < 2 * W ; ++k ) {
   const int a = int( g() % W ) , b = int( g() % W );
   L.emplace_back( h * W + a , ( h + 1 ) * W + b );
   nc[ b ] += c[ a ];
   }
  c = nc;
  }
 for( int a = 0 ; a < W ; ++a )                // a source and a sink
  L.emplace_back( W * H , a );
 for( int b = 0 ; b < W ; ++b )
  L.emplace_back( ( H - 1 ) * W + b , W * H + 1 );
 uint64_t total = 0;
 for( uint64_t x : c )
  total += x;
 CHECK( batman( W * H + 2 , L , W * H , W * H + 1 ) == total );
 }

/*----------------------- 2014, Natale a Flatlandia ------------------------*/

// the last triangles by the exhaustive search over the rows left, as sets
// of places
std::vector< int > flatlandBrute( const std::string & s )
{
 const int n = int( s.size() );
 std::set< int > S;
 std::vector< char > seen( size_t( 1 ) << n , 0 );
 auto rec = [ & ]( auto & self , unsigned mask ) -> void {
  if( std::exchange( seen[ mask ] , 1 ) )
   return;
  if( std::popcount( mask ) == 1 ) {
   S.insert( std::countr_zero( mask ) );
   return;
   }
  std::vector< int > R;
  for( int i = 0 ; i < n ; ++i )
   if( ( mask >> i ) & 1u )
    R.push_back( i );
  for( size_t k = 1 ; k + 1 < R.size() ; ++k ) {
   const int j = s[ R[ k ] ] == 's' ? R[ k - 1 ] : R[ k + 1 ];
   self( self , mask & ~( 1u << R[ k ] ) & ~( 1u << j ) );
   }
  };
 rec( rec , ( 1u << n ) - 1 );
 return( std::vector< int >( S.begin() , S.end() ) );
 }

// the examples of the statement and the three smallest rows of the data,
// every row of up to 11 triangles and random rows of 13 and 15 against the
// exhaustive search, and the mirror symmetry on rows of 199 triangles
ADS_TEST( "asdlab-flatland" )
{
 CHECK( flatland( "sssddss" ) == std::vector< int >( { 0 , 2 , 6 } ) );
 CHECK( flatland( "ssddssdsd" ) == std::vector< int >( { 2 , 8 } ) );
 CHECK( flatland( "dssds" ) == std::vector< int >( { 0 , 2 } ) );
 CHECK( flatland( "dddsd" ) == std::vector< int >( { 0 , 4 } ) );
 CHECK( flatland( "s" ) == std::vector< int >( { 0 } ) );
 CHECK( flatland( "sss" ) == std::vector< int >( { 2 } ) );
 CHECK( flatland( "ddd" ) == std::vector< int >( { 0 } ) );
 for( int n = 1 ; n <= 11 ; n += 2 )
  for( unsigned b = 0 ; b < ( 1u << n ) ; ++b ) {
   std::string s( n , 's' );
   for( int i = 0 ; i < n ; ++i )
    if( ( b >> i ) & 1u )
     s[ i ] = 'd';
   CHECK( flatland( s ) == flatlandBrute( s ) );
   }
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 120 ; ++it ) {
  std::string s( it % 2 ? 15 : 13 , 's' );
  for( auto & c : s )
   c = g() % 2 ? 'd' : 's';
  CHECK( flatland( s ) == flatlandBrute( s ) );
  }
 for( int it = 0 ; it < 5 ; ++it ) {           // the mirror of the row
  std::string s( 199 , 's' ) , r;
  for( auto & c : s )
   c = g() % 2 ? 'd' : 's';
  for( auto c = s.rbegin() ; c != s.rend() ; ++c )
   r.push_back( *c == 's' ? 'd' : 's' );
  auto S = flatland( s ) , R = flatland( r );
  for( auto & x : R )
   x = 198 - x;
  std::sort( R.begin() , R.end() );
  CHECK( ( S == R ) && ! S.empty() );
  }
 CHECK_THROWS( flatland( "ss" ) , contract_violation );
 }

/*--------------------------- 2016, Pokemon Park ---------------------------*/

// the simple cycles of a small multigraph, as sets of edges
std::vector< unsigned > cycles( int n , const Arcs & E )
{
 Incidence A( n );
 for( int e = 0 ; e < int( E.size() ) ; ++e ) {
  A[ E[ e ].first ].emplace_back( E[ e ].second , e );
  A[ E[ e ].second ].emplace_back( E[ e ].first , e );
  }
 std::set< unsigned > C;
 for( int s = 0 ; s < n ; ++s ) {
  auto rec = [ & ]( auto & self , int u , unsigned nodes ,
                    unsigned edges ) -> void {
   for( const auto & [ v , e ] : A[ u ] ) {
    if( ( edges >> e ) & 1u )
     continue;
    if( v == s )
     C.insert( edges | ( 1u << e ) );
    else if( ( v > s ) && ! ( ( nodes >> v ) & 1u ) )
     self( self , v , nodes | ( 1u << v ) , edges | ( 1u << e ) );
    }
   };
  rec( rec , s , 1u << s , 0u );
  }
 return( std::vector< unsigned >( C.begin() , C.end() ) );
 }

// whether each cycle of C meets each species of P equally often
bool balanced( const std::vector< unsigned > & C ,
               const std::vector< int > & P , int K )
{
 for( unsigned c : C ) {
  std::vector< int > cnt( K , 0 );
  for( int e = 0 ; e < int( P.size() ) ; ++e )
   if( ( c >> e ) & 1u ) {
    if( ( P[ e ] < 0 ) || ( P[ e ] >= K ) )
     return( false );
    ++cnt[ P[ e ] ];
    }
  if( std::adjacent_find( cnt.begin() , cnt.end() ,
                          std::not_equal_to<>() ) != cnt.end() )
   return( false );
  }
 return( true );
 }

// the largest K with balanced species, by backtracking from the gcd of the
// lengths of the cycles down, a new species only after the used ones
int pokemonBrute( int m , const std::vector< unsigned > & C )
{
 U64 G = 0;
 for( unsigned c : C )
  G = gcd( U64( std::popcount( c ) ) , G );
 for( int K = int( G ) ; K > 1 ; --K ) {
  std::vector< int > P( m , -1 );
  std::function< bool( int , int ) > rec = [ & ]( int e , int used ) {
   if( e == m )
    return( balanced( C , P , K ) );
   for( int k = 0 ; k <= std::min( used , K - 1 ) ; ++k ) {
    P[ e ] = k;
    bool ok = true;                            // no species too frequent
    for( unsigned c : C ) {
     if( ! ( ( c >> e ) & 1u ) )
      continue;
     int cnt = 0;
     for( int f = 0 ; f <= e ; ++f )
      cnt += ( ( c >> f ) & 1u ) && ( P[ f ] == k );
     ok = ok && ( cnt * K <= std::popcount( c ) );
     }
    if( ok && rec( e + 1 , std::max( used , k + 1 ) ) )
     return( true );
    }
   P[ e ] = -1;
   return( false );
   };
  if( rec( 0 , 0 ) )
   return( K );
  }
 return( 1 );
 }

// the examples of the statement, the largest K of random small graphs
// against the backtracking and their species against all their cycles,
// and cactus graphs whose K is the gcd of the lengths of their cycles
ADS_TEST( "asdlab-pokemon" )
{
 const Arcs ex = { { 0 , 1 } , { 0 , 3 } , { 0 , 4 } , { 0 , 8 } ,
                   { 0 , 12 } , { 1 , 2 } , { 1 , 7 } , { 2 , 3 } ,
                   { 4 , 5 } , { 4 , 6 } , { 6 , 12 } , { 8 , 9 } ,
                   { 8 , 10 } , { 9 , 11 } , { 10 , 11 } };
 const auto P = pokemon( 13 , ex );
 const auto C = cycles( 13 , ex );
 CHECK( ( P.K == 4 ) && balanced( C , P.species , 4 ) );
 CHECK( balanced( C , { 0 , 1 , 2 , 3 , 3 , 2 , 0 , 3 , 3 , 1 , 0 , 0 , 2 ,
                        3 , 1 } , 4 ) );           // the statement's
 const Arcs sq = { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } , { 0 , 3 } };
 CHECK( pokemon( 4 , sq ).K == 4 );
 CHECK( pokemon( 4 , sq ).species == std::vector< int >( { 0 , 1 , 2 , 3 } ) );
 std::mt19937 g( 1 );
 int multi = 0;
 for( int it = 0 ; it < 300 ; ++it ) {
  const int n = 3 + int( g() % 5 );
  Arcs E;                                     // a random tree
  for( int u = 1 ; u < n ; ++u )
   E.emplace_back( int( g() % unsigned( u ) ) , u );
  std::set< Arc > S( E.begin() , E.end() );
  const int extra = 1 + int( g() % 3 );
  while( int( E.size() ) < n - 1 + extra ) {
   int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
   if( u > v )
    std::swap( u , v );
   if( ( u != v ) && S.emplace( u , v ).second )
    E.emplace_back( u , v );
   if( int( S.size() ) == n * ( n - 1 ) / 2 )
    break;
   }
  if( int( E.size() ) < n )
   continue;
  const auto Q = pokemon( n , E );
  const auto D = cycles( n , E );
  CHECK( balanced( D , Q.species , Q.K ) );
  CHECK( Q.K == pokemonBrute( int( E.size() ) , D ) );
  multi += Q.K > 1;
  }
 CHECK( multi > 30 );
 std::vector< int > T( 4500 );                 // one cycle of 4500
 Arcs ring;
 for( int u = 0 ; u < 4500 ; ++u )
  ring.emplace_back( std::min( u , ( u + 1 ) % 4500 ) ,
                     std::max( u , ( u + 1 ) % 4500 ) );
 const auto R = pokemon( 4500 , ring );
 CHECK( R.K == 4500 );
 std::iota( T.begin() , T.end() , 0 );
 auto sp = R.species;
 std::sort( sp.begin() , sp.end() );
 CHECK( sp == T );
 for( int it = 0 ; it < 20 ; ++it ) {          // cactus graphs
  Arcs E;
  int n = 1;
  U64 G = 0;
  const int base = 3 + int( g() % 4 );
  while( n < 300 ) {
   const int len = base * ( 1 + int( g() % 4 ) ) + ( it % 5 == 0 );
   int first = int( g() % unsigned( n ) );     // where the cycle hangs,
   if( g() % 3 == 0 ) {                        // maybe through a bridge
    E.emplace_back( first , n );
    first = n++;
    }
   int prev = first;
   for( int k = 1 ; k < len ; ++k ) {
    E.emplace_back( prev , n );
    prev = n++;
    }
   E.emplace_back( first , prev );
   G = gcd( U64( len ) , G );
   }
  CHECK( pokemon( n , E ).K == int( G ) );
  }
 }

/*--------------------- 2016, La vendetta del Re Lich ----------------------*/

// d by a visit from each node, then from each node r the nodes with d in
// [ d[ r ] , d[ r ] + L ] it reaches through such nodes; O( n^2 ) per L
std::vector< int > lichBrute( int n , const std::vector< WEdge > & E ,
                              const std::vector< int64_t > & Ls )
{
 std::vector< std::vector< std::pair< int , int > > > A( n );
 for( const auto & e : E ) {
  A[ e.u ].emplace_back( e.v , e.w );
  A[ e.v ].emplace_back( e.u , e.w );
  }
 std::vector< int64_t > d( n , 0 );
 for( int s = 0 ; s < n ; ++s ) {
  std::vector< int64_t > x( n , -1 );
  std::vector< int > Q = { s };
  x[ s ] = 0;
  while( ! Q.empty() ) {
   const int u = Q.back();
   Q.pop_back();
   d[ s ] = std::max( d[ s ] , x[ u ] );
   for( const auto & [ v , w ] : A[ u ] )
    if( x[ v ] < 0 ) {
     x[ v ] = x[ u ] + w;
     Q.push_back( v );
     }
   }
  }
 std::vector< int > out;
 for( int64_t L : Ls ) {
  int best = 0;
  for( int r = 0 ; r < n ; ++r ) {
   std::vector< char > in( n , 0 );
   std::vector< int > Q = { r };
   in[ r ] = 1;
   int c = 0;
   while( ! Q.empty() ) {
    const int u = Q.back();
    Q.pop_back();
    ++c;
    for( const auto & [ v , w ] : A[ u ] )
     if( ! in[ v ] && ( d[ v ] >= d[ r ] ) && ( d[ v ] <= d[ r ] + L ) ) {
      in[ v ] = 1;
      Q.push_back( v );
      }
    }
   best = std::max( best , c );
   }
  out.push_back( best );
  }
 return( out );
 }

// the examples of the statement, random trees against the brute force, the
// closed formula of the unweighted path, and a long weighted path
ADS_TEST( "asdlab-lich" )
{
 const std::vector< WEdge > ex1 = { { 0 , 5 , 3 } , { 1 , 10 , 7 } ,
  { 2 , 3 , 2 } , { 2 , 11 , 8 } , { 3 , 5 , 8 } , { 3 , 10 , 1 } ,
  { 4 , 5 , 4 } , { 5 , 6 , 4 } , { 5 , 8 , 1 } , { 5 , 9 , 4 } ,
  { 7 , 10 , 8 } };
 const std::vector< WEdge > ex2 = { { 0 , 5 , 1 } , { 1 , 4 , 1 } ,
  { 1 , 6 , 1 } , { 2 , 3 , 1 } , { 2 , 8 , 1 } , { 3 , 7 , 1 } ,
  { 4 , 5 , 1 } , { 6 , 10 , 1 } , { 7 , 9 , 1 } , { 8 , 10 , 1 } };
 const std::vector< int64_t > q = { 0 , 1 , 2 , 3 , 4 , 5 };
 CHECK( lich( 12 , ex1 , q ) == std::vector< int >( { 1 , 2 , 3 , 3 , 6 ,
                                                       6 } ) );
 CHECK( lich( 11 , ex2 , q ) == std::vector< int >( { 1 , 3 , 5 , 7 , 9 ,
                                                       11 } ) );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 40 );
  const int wmax = it % 2 ? 5 : 1000;
  std::vector< WEdge > E;
  for( int u = 1 ; u < n ; ++u )
   E.push_back( { int( g() % unsigned( u ) ) , u ,
                  1 + int( g() % unsigned( wmax ) ) } );
  std::vector< int64_t > Ls = { 0 , 1 , 1000000 };
  for( int k = 0 ; k < 4 ; ++k )
   Ls.push_back( int64_t( g() % unsigned( 3 * wmax ) ) );
  CHECK( lich( n , E , Ls ) == lichBrute( n , E , Ls ) );
  }
 for( int n = 1 ; n <= 30 ; ++n ) {            // the unweighted path
  std::vector< WEdge > E;
  for( int u = 1 ; u < n ; ++u )
   E.push_back( { u - 1 , u , 1 } );
  std::vector< int64_t > Ls( 36 );
  std::iota( Ls.begin() , Ls.end() , 0 );
  const auto r = lich( n , E , Ls );
  for( int64_t L = 0 ; L < 36 ; ++L )
   CHECK( r[ L ] == int( std::min< int64_t >( n , n % 2 ? 1 + 2 * L
                                                        : 2 * ( 1 + L ) ) ) );
  }
 std::vector< WEdge > E;                       // a long path
 for( int u = 1 ; u < 3000 ; ++u )
  E.push_back( { u - 1 , u , 1000 } );
 CHECK( lich( 3000 , E , { 0 , 7000 , 1000000 , 2999000 } ) ==
        std::vector< int >( { 2 , 16 , 2002 , 3000 } ) );
 }

/*---------------------- 2017, 007: Pioggia di laser -----------------------*/

// the earliest arrival at n - 1 by the graph expanded in time, ( node ,
// time ) for times up to T, waiting allowed; -1 if not by T
int64_t laserBrute( int n , const std::vector< Laser > & E , int T )
{
 std::vector< std::vector< char > > R( T + 1 , std::vector< char >( n , 0 ) );
 R[ 0 ][ 0 ] = 1;
 for( int t = 0 ; t <= T ; ++t ) {
  if( R[ t ][ n - 1 ] )
   return( t );
  for( int u = 0 ; u < n ; ++u ) {
   if( ! R[ t ][ u ] )
    continue;
   if( t < T )
    R[ t + 1 ][ u ] = 1;
   for( const auto & e : E ) {
    if( ( e.s != u ) && ( e.t != u ) )
     continue;
    const bool off = ( t >= e.f ) &&
                     ( ( t - e.f ) % ( e.off + e.on ) + e.w <= e.off );
    if( off && ( t + e.w <= T ) )
     R[ t + e.w ][ e.s == u ? e.t : e.s ] = 1;
    }
   }
  }
 return( -1 );
 }

// whether P goes from 0 to n - 1 along edges and, crossed as early as
// possible, arrives at T
bool laserPath( int n , const std::vector< Laser > & E ,
                const std::vector< int > & P , int64_t T )
{
 if( P.empty() || ( P.front() != 0 ) || ( P.back() != n - 1 ) )
  return( false );
 int64_t t = 0;
 for( size_t i = 0 ; i + 1 < P.size() ; ++i ) {
  int64_t best = -1;
  for( const auto & e : E )
   if( ( e.off >= e.w ) &&
       ( ( ( e.s == P[ i ] ) && ( e.t == P[ i + 1 ] ) ) ||
         ( ( e.t == P[ i ] ) && ( e.s == P[ i + 1 ] ) ) ) ) {
    const int64_t a = laserDeparture( e , t ) + e.w;
    if( ( best < 0 ) || ( a < best ) )
     best = a;
    }
  if( best < 0 )
   return( false );
  t = best;
  }
 return( t == T );
 }

// the examples of the statement, random small graphs against the graph
// expanded in time, and the edge cases
ADS_TEST( "asdlab-laser" )
{
 const std::vector< Laser > ex1 = { { 0 , 1 , 8 , 7 , 2 , 2 } ,
  { 0 , 2 , 1 , 5 , 1 , 2 } , { 1 , 3 , 6 , 16 , 5 , 5 } ,
  { 2 , 3 , 4 , 12 , 18 , 10 } , { 3 , 6 , 10 , 8 , 13 , 4 } ,
  { 4 , 1 , 13 , 6 , 10 , 16 } , { 5 , 3 , 2 , 11 , 4 , 2 } ,
  { 5 , 6 , 4 , 6 , 6 , 1 } , { 6 , 2 , 12 , 15 , 24 , 5 } };
 const auto r1 = laser( 7 , ex1 );
 CHECK( ( r1.first == 24 ) && laserPath( 7 , ex1 , r1.second , 24 ) );
 CHECK( r1.second == std::vector< int >( { 0 , 2 , 3 , 5 , 6 } ) );
 std::vector< Laser > ex2;
 for( const auto & [ s , t ] : Arcs( { { 0 , 6 } , { 1 , 0 } , { 2 , 5 } ,
                                       { 2 , 7 } , { 3 , 4 } , { 4 , 1 } ,
                                       { 4 , 5 } , { 4 , 9 } , { 7 , 5 } ,
                                       { 7 , 6 } } ) )
  ex2.push_back( { s , t , 1 , 0 , 1000000 , 1 } );
 const auto r2 = laser( 10 , ex2 );
 CHECK( ( r2.first == 3 ) && laserPath( 10 , ex2 , r2.second , 3 ) );
 std::mt19937 g( 1 );
 int reached = 0;
 for( int it = 0 ; it < 500 ; ++it ) {
  const int n = 2 + int( g() % 6 ) , m = int( g() % 11 );
  std::vector< Laser > E;
  for( int k = 0 ; k < m ; ++k ) {
   const int s = int( g() % unsigned( n ) ) , t = int( g() % unsigned( n ) );
   if( s != t )
    E.push_back( { s , t , 1 + int( g() % 5 ) , int( g() % 12 ) ,
                   int( g() % 9 ) , int( g() % 9 ) } );
   if( E.size() && ( E.back().off + E.back().on == 0 ) )
    E.back().on = 1;
   }
  const auto r = laser( n , E );
  const int64_t b = laserBrute( n , E , 400 );
  if( b >= 0 ) {
   CHECK( r.first == b );
   CHECK( laserPath( n , E , r.second , r.first ) );
   ++reached;
   }
  else
   CHECK( ( r.first == -1 ) || ( r.first > 400 ) );
  }
 CHECK( reached > 150 );
 CHECK( laser( 1 , {} ) == std::pair( int64_t( 0 ) ,
                                     std::vector< int >( { 0 } ) ) );
 CHECK( laser( 2 , {} ).first == -1 );
 CHECK( laser( 2 , { { 0 , 1 , 5 , 0 , 4 , 1 } } ).first == -1 );  // y < w
 CHECK( laser( 2 , { { 1 , 0 , 100 , 1000000 , 100 , 1 } } ).first ==
        1000100 );
 CHECK( laser( 2 , { { 0 , 1 , 2 , 0 , 2 , 3 } } ).first == 2 );
 CHECK( laserDeparture( { 0 , 1 , 2 , 0 , 3 , 3 } , 2 ) == 6 );
 }

/*------------------------- 2017, Assalto a Nassau -------------------------*/

// the expected power by the direct recurrence on ( ships of 2 unhurt ,
// ships of 2 hurt , ships of 1 , shots left ), in the number type T
template< class T , class Make >
T nassauBrute( int V , int F , int M , Make && make )
{
 std::map< std::array< int , 4 > , T > memo;
 std::function< T( int , int , int , int ) > EP = [ & ]( int v , int d ,
                                                        int f , int m ) {
  if( ( m == 0 ) || ( v + d + f == 0 ) )
   return( make( ( v + d ) * f ) );
  const std::array< int , 4 > key = { v , d , f , m };
  if( const auto it = memo.find( key ) ; it != memo.end() )
   return( it->second );
  T r = make( 0 );
  if( v )
   r = r + make( v ) * EP( v - 1 , d + 1 , f , m - 1 );
  if( d )
   r = r + make( d ) * EP( v , d - 1 , f , m - 1 );
  if( f )
   r = r + make( f ) * EP( v , d , f - 1 , m - 1 );
  r = r / make( v + d + f );
  memo.emplace( key , r );
  return( r );
  };
 return( EP( V , 0 , F , M ) );
 }

// the example of the statement and its print, the exact recurrence in
// rationals and in doubles on small fleets, and the edge cases
ADS_TEST( "asdlab-nassau" )
{
 std::ostringstream os;
 os << std::scientific << std::setprecision( 10 ) << nassau( 1 , 2 , 2 );
 CHECK( os.str() == "5.5555555556e-01" );
 auto rational = []( int x ) { return( Rational( BigInteger( x ) ) ); };
 for( int V = 1 ; V <= 3 ; ++V )
  for( int F = 1 ; F <= 3 ; ++F )
   for( int M = 0 ; M <= 2 * V + F + 1 ; ++M ) {
    const Rational e = nassauBrute< Rational >( V , F , M , rational );
    const double x = std::stod( e.num().toString() ) /
                     std::stod( e.den().toString() );
    CHECK( std::abs( nassau( V , F , M ) - x ) <= 1e-12 * ( 1 + x ) );
    }
 CHECK( nassauBrute< Rational >( 1 , 2 , 2 , rational ) ==
        Rational( BigInteger( 5 ) , BigInteger( 9 ) ) );
 std::mt19937 g( 1 );
 auto real = []( int x ) { return( double( x ) ); };
 for( int it = 0 ; it < 60 ; ++it ) {
  const int V = 1 + int( g() % 12 ) , F = 1 + int( g() % 12 ) ,
            M = int( g() % unsigned( 2 * V + F + 3 ) );
  const double x = nassauBrute< double >( V , F , M , real );
  CHECK( std::abs( nassau( V , F , M ) - x ) <= 1e-9 * ( 1 + x ) );
  }
 CHECK( nassau( 0 , 5 , 3 ) == 0 && nassau( 5 , 0 , 3 ) == 0 );
 CHECK( std::abs( nassau( 7 , 9 , 0 ) - 63 ) < 1e-9 );
 CHECK( std::abs( nassau( 7 , 9 , 23 ) ) < 1e-12 );       // all sunk
 double prev = 4999.0 * 4999.0;
 for( int M : { 0 , 1 , 100 , 2500 , 4999 } ) {           // the largest
  const double x = nassau( 4999 , 4999 , M );
  CHECK( ( x <= prev * ( 1 + 1e-12 ) ) && ( x > 0 ) );
  prev = x;
  }
 }

/*------------------------ 2017, Alpinocalypse Now -------------------------*/

// whether the multigraph without the nodes R is acyclic, by a forest of
// parents of its own
bool acyclicWithout( int n , const Arcs & E , const std::vector< int > & R )
{
 std::vector< int > p( n );
 std::iota( p.begin() , p.end() , 0 );
 std::vector< char > out( n , 0 );
 for( int u : R ) {
  if( ( u < 0 ) || ( u >= n ) || out[ u ] )
   return( false );
  out[ u ] = 1;
  }
 std::function< int( int ) > root = [ & ]( int x ) {
  return( p[ x ] == x ? x : p[ x ] = root( p[ x ] ) );
  };
 for( const auto & [ u , v ] : E ) {
  if( out[ u ] || out[ v ] )
   continue;
  const int a = root( u ) , b = root( v );
  if( a == b )
   return( false );
  p[ a ] = b;
  }
 return( true );
 }

// the official baseline: while a cycle is left, remove the node of largest
// degree
size_t alpiniGreedyDegree( int n , const Arcs & E )
{
 std::vector< int > deg( n , 0 ) , R;
 std::vector< char > out( n , 0 );
 for( const auto & [ u , v ] : E ) {
  ++deg[ u ];
  ++deg[ v ];
  }
 while( ! acyclicWithout( n , E , R ) ) {
  int b = -1;
  for( int u = 0 ; u < n ; ++u )
   if( ! out[ u ] && ( ( b < 0 ) || ( deg[ u ] > deg[ b ] ) ) )
    b = u;
  out[ b ] = 1;
  R.push_back( b );
  for( const auto & [ u , v ] : E ) {
   if( u == b )
    --deg[ v ];
   if( v == b )
    --deg[ u ];
   }
  }
 return( R.size() );
 }

// the least number of nodes to remove, by the subsets of up to 12 nodes
size_t alpiniBrute( int n , const Arcs & E )
{
 size_t best = size_t( n );
 for( unsigned mask = 0 ; mask < ( 1u << n ) ; ++mask ) {
  if( size_t( std::popcount( mask ) ) >= best )
   continue;
  std::vector< int > R;
  for( int u = 0 ; u < n ; ++u )
   if( ( mask >> u ) & 1u )
    R.push_back( u );
  if( acyclicWithout( n , E , R ) )
   best = R.size();
  }
 return( best );
 }

// the example of the statement, random small multigraphs against the
// optimum and the official baseline, and the known graphs
ADS_TEST( "asdlab-alpini" )
{
 const Arcs ex = { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } , { 2 , 3 } };
 const auto r = alpini( 4 , ex );
 CHECK( ( r.size() == 1 ) && acyclicWithout( 4 , ex , r ) );
 std::mt19937 g( 1 );
 int optimal = 0 , total = 0;
 for( int it = 0 ; it < 300 ; ++it ) {
  const int n = 3 + int( g() % 10 ) , m = int( g() % unsigned( 3 * n ) );
  const auto E = randomArcs( g , n , m , it % 10 == 0 );
  const auto R = alpini( n , E );
  const size_t opt = alpiniBrute( n , E );
  CHECK( acyclicWithout( n , E , R ) );
  CHECK( ( R.size() >= opt ) && ( R.size() <= alpiniGreedyDegree( n , E ) ) );
  CHECK( std::is_sorted( R.begin() , R.end() ) );
  optimal += R.size() == opt;
  ++total;
  }
 CHECK( optimal * 100 >= total * 97 );
 Arcs K;                                       // the complete graph K10
 for( int u = 0 ; u < 10 ; ++u )
  for( int v = u + 1 ; v < 10 ; ++v )
   K.emplace_back( u , v );
 CHECK( alpini( 10 , K ).size() == 8 );
 Arcs T;                                       // a tree, a cycle
 for( int u = 1 ; u < 50 ; ++u )
  T.emplace_back( int( g() % unsigned( u ) ) , u );
 CHECK( alpini( 50 , T ).empty() );
 T.emplace_back( 0 , 49 );
 const auto R1 = alpini( 50 , T );
 CHECK( ( R1.size() == 1 ) && acyclicWithout( 50 , T , R1 ) );
 CHECK( alpini( 3 , { { 0 , 1 } , { 0 , 1 } , { 2 , 2 } } ).size() == 2 );
 for( int it = 0 ; it < 3 ; ++it ) {           // the largest instances
  const auto E = simpleArcs( g , 99 , 2000 );
  const auto R = alpini( 99 , E , 30 );
  CHECK( acyclicWithout( 99 , E , R ) && ( R.size() <= 97 ) );
  CHECK( R.size() <= alpiniGreedyDegree( 99 , E ) );
  }
 }

/*------------------ 2018, Game of (Approximated) Thrones ------------------*/

// the castles of A in valid regions of B, -1 if B is not valid: each
// 4-connected set of equal positive values k has k cells, a castle k and
// no castle of another value
int gotScore( const Grid & A , const Grid & B )
{
 const int N = int( A.size() ) , M = int( A[ 0 ].size() );
 if( ( int( B.size() ) != N ) || ( int( B[ 0 ].size() ) != M ) )
  return( -1 );
 for( const auto & row : B )
  if( int( row.size() ) != M )
   return( -1 );
 std::vector< std::vector< char > > seen( N , std::vector< char >( M , 0 ) );
 int nv = 0;
 for( int i = 0 ; i < N ; ++i )
  for( int j = 0 ; j < M ; ++j ) {
   if( B[ i ][ j ] < 0 )
    return( -1 );
   if( ( B[ i ][ j ] == 0 ) || seen[ i ][ j ] )
    continue;
   const int k = B[ i ][ j ];
   std::vector< std::pair< int , int > > Q = { { i , j } };
   seen[ i ][ j ] = 1;
   int own = 0 , size = 0;
   while( ! Q.empty() ) {
    const auto [ x , y ] = Q.back();
    Q.pop_back();
    ++size;
    if( A[ x ][ y ] && ( A[ x ][ y ] != k ) )
     return( -1 );
    own += A[ x ][ y ] == k;
    const int dx[] = { 1 , -1 , 0 , 0 } , dy[] = { 0 , 0 , 1 , -1 };
    for( int d = 0 ; d < 4 ; ++d ) {
     const int a = x + dx[ d ] , b = y + dy[ d ];
     if( ( a >= 0 ) && ( a < N ) && ( b >= 0 ) && ( b < M ) &&
         ! seen[ a ][ b ] && ( B[ a ][ b ] == k ) ) {
      seen[ a ][ b ] = 1;
      Q.emplace_back( a , b );
      }
     }
    }
   if( ( size != k ) || ( own == 0 ) )
    return( -1 );
   nv += own;
   }
 return( nv );
 }

// the simple baseline: the castles 1, in reading order, each kept when no
// castle 1 next to it is
int gotBaseline( const Grid & A )
{
 const int N = int( A.size() ) , M = int( A[ 0 ].size() );
 Grid K( N , std::vector< int >( M , 0 ) );
 int c = 0;
 for( int i = 0 ; i < N ; ++i )
  for( int j = 0 ; j < M ; ++j )
   if( ( A[ i ][ j ] == 1 ) && ! ( i && K[ i - 1 ][ j ] ) &&
       ! ( j && K[ i ][ j - 1 ] ) ) {
    K[ i ][ j ] = 1;
    ++c;
    }
 return( c );
 }

// a random grid with a solution taking all its castles: random regions of
// up to vmax cells, and a castle in a region with probability 3/4 unless
// a region of the same size next to it has one already; the castles and
// their number
std::pair< Grid , int > gotInstance( std::mt19937 & g , int N , int M ,
                                     int vmax )
{
 Grid id( N , std::vector< int >( M , -1 ) );
 std::vector< std::vector< std::pair< int , int > > > R;
 const int dx[] = { 1 , -1 , 0 , 0 } , dy[] = { 0 , 0 , 1 , -1 };
 auto inside = [ & ]( int a , int b ) {
  return( ( a >= 0 ) && ( a < N ) && ( b >= 0 ) && ( b < M ) );
  };
 for( int i = 0 ; i < N ; ++i )
  for( int j = 0 ; j < M ; ++j ) {
   if( id[ i ][ j ] >= 0 )
    continue;
   const int want = 1 + int( g() % unsigned( vmax ) ) , r = int( R.size() );
   R.push_back( { { i , j } } );
   id[ i ][ j ] = r;
   for( int t = 0 ; ( int( R[ r ].size() ) < want ) && ( t < 8 * want ) ;
        ++t ) {
    const auto c = R[ r ][ g() % R[ r ].size() ];
    const int d = int( g() % 4 ) , a = c.first + dx[ d ] ,
              b = c.second + dy[ d ];
    if( inside( a , b ) && ( id[ a ][ b ] < 0 ) ) {
     id[ a ][ b ] = r;
     R[ r ].emplace_back( a , b );
     }
    }
   }
 std::vector< char > has( R.size() , 0 );
 Grid A( N , std::vector< int >( M , 0 ) ) , S = A;
 int nc = 0;
 for( size_t r = 0 ; r < R.size() ; ++r ) {
  if( g() % 4 == 0 )
   continue;
  bool clash = false;
  for( const auto & [ a , b ] : R[ r ] )
   for( int d = 0 ; d < 4 ; ++d ) {
    const int x = a + dx[ d ] , y = b + dy[ d ];
    if( inside( x , y ) && ( id[ x ][ y ] != int( r ) ) &&
        has[ id[ x ][ y ] ] && ( R[ id[ x ][ y ] ].size() == R[ r ].size() ) )
     clash = true;
    }
  if( clash )
   continue;
  has[ r ] = 1;
  const auto c = R[ r ][ g() % R[ r ].size() ];
  A[ c.first ][ c.second ] = int( R[ r ].size() );
  for( const auto & [ a , b ] : R[ r ] )
   S[ a ][ b ] = int( R[ r ].size() );
  ++nc;
  }
 CHECK( gotScore( A , S ) == nc );             // S takes all the castles
 return( std::pair( A , nc ) );
 }

// the example of the statement, random grids with a known optimum against
// the baseline and the optimum, and the edge cases
ADS_TEST( "asdlab-got" )
{
 const Grid ex = { { 0 , 0 , 0 , 6 , 0 } , { 0 , 2 , 0 , 0 , 0 } ,
                   { 4 , 0 , 0 , 8 , 0 } , { 0 , 0 , 0 , 0 , 0 } ,
                   { 0 , 0 , 0 , 0 , 5 } };
 const Grid sol = { { 6 , 6 , 6 , 6 , 8 } , { 2 , 2 , 6 , 6 , 8 } ,
                    { 4 , 8 , 8 , 8 , 8 } , { 4 , 4 , 5 , 8 , 8 } ,
                    { 4 , 5 , 5 , 5 , 5 } };
 CHECK( gotScore( ex , sol ) == 5 );           // the statement's
 CHECK( gotScore( ex , got( ex ) ) >= 4 );
 std::mt19937 g( 1 );
 int nv = 0 , nc = 0;
 for( int it = 0 ; it < 120 ; ++it ) {
  const int N = 3 + int( g() % 13 ) , M = 3 + int( g() % 13 );
  const auto [ A , opt ] = gotInstance( g , N , M , it % 3 ? 6 : 12 );
  const int s = gotScore( A , got( A ) );
  CHECK( ( s >= gotBaseline( A ) ) && ( s <= opt ) );
  nv += s;
  nc += opt;
  }
 CHECK( nv * 100 >= nc * 90 );
 CHECK( gotScore( { { 0 , 0 } } , got( { { 0 , 0 } } ) ) == 0 );
 CHECK( gotScore( { { 1 } } , got( { { 1 } } ) ) == 1 );
 CHECK( gotScore( { { 1 , 1 } } , got( { { 1 , 1 } } ) ) == 1 );
 CHECK( gotScore( { { 3 , 0 } } , got( { { 3 , 0 } } ) ) == 0 );
 CHECK( gotScore( { { 2 , 2 } } , got( { { 2 , 2 } } ) ) == 2 );
 const auto [ A , opt ] = gotInstance( g , 200 , 200 , 20 );  // the largest
 const int s = gotScore( A , got( A , 3 ) );
 CHECK( ( s >= gotBaseline( A ) ) && ( s * 100 >= opt * 80 ) );
 }

/*-------------------- 2026-09-24, Somma di due numeri ---------------------*/

// the example of the statement and random sums against the built-in one
ADS_TEST( "asdlab-2026-lab01-somma" )
{
 CHECK( somma( 2 , 3 ) == 5 );
 std::mt19937_64 g( 1 );
 for( int it = 0 ; it < 1000 ; ++it ) {
  const int64_t a = int64_t( g() >> 2 ) - ( int64_t( 1 ) << 61 ) ,
                b = int64_t( g() >> 2 ) - ( int64_t( 1 ) << 61 );
  CHECK( somma( a , b ) == a + b );
  }
 CHECK( somma( std::numeric_limits< int >::max() ,
               std::numeric_limits< int >::max() ) == 4294967294LL );
 CHECK( somma( 0 , 0 ) == 0 && somma( -7 , 7 ) == 0 );
 }

/*--------------- 2026-09-24, Sottosequenza di somma massima ---------------*/

// the example of the statement and random vectors against the sums of all
// the runs
ADS_TEST( "asdlab-2026-lab01-sottoseq" )
{
 CHECK( sottoseq( { 3 , -2 , 4 , 1 , 5 } ) == 11 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 500 ; ++it ) {
  std::vector< int64_t > A( g() % 30 );
  for( auto & x : A )
   x = int64_t( g() % 41 ) - 20;
  int64_t best = 0;
  for( size_t i = 0 ; i < A.size() ; ++i ) {
   int64_t s = 0;
   for( size_t j = i ; j < A.size() ; ++j )
    best = std::max( best , s += A[ j ] );
   }
  CHECK( sottoseq( A ) == best );
  }
 CHECK( sottoseq( {} ) == 0 && sottoseq( { -5 , -1 } ) == 0 );
 CHECK( sottoseq( { 7 , 7 , 7 } ) == 21 );
 CHECK( sottoseq( std::vector< int64_t >( 100000 , 1000000000 ) ) ==
        100000LL * 1000000000LL );
 }

/*--------------- 2026-09-24, Sottomatrice di somma massima ----------------*/

// the example of the statement and random matrices against the sums of
// all the rectangles by prefix sums
ADS_TEST( "asdlab-2026-lab01-sottomat" )
{
 CHECK( sottomat( { { 2 , -9 , 2 , 3 } , { 1 , 4 , 5 , 1 } ,
                    { -2 , 3 , 4 , 1 } } ) == 18 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 300 ; ++it ) {
  const size_t R = g() % 7 , C = 1 + g() % 7;
  std::vector< std::vector< int64_t > > A( R , std::vector< int64_t >( C ) );
  for( auto & row : A )
   for( auto & x : row )
    x = int64_t( g() % 21 ) - 10;
  std::vector< std::vector< int64_t > > P(
                               R + 1 , std::vector< int64_t >( C + 1 , 0 ) );
  for( size_t i = 0 ; i < R ; ++i )
   for( size_t j = 0 ; j < C ; ++j )
    P[ i + 1 ][ j + 1 ] = A[ i ][ j ] + P[ i ][ j + 1 ] + P[ i + 1 ][ j ] -
                          P[ i ][ j ];
  int64_t best = 0;
  for( size_t a = 0 ; a < R ; ++a )
   for( size_t b = a + 1 ; b <= R ; ++b )
    for( size_t c = 0 ; c < C ; ++c )
     for( size_t d = c + 1 ; d <= C ; ++d )
      best = std::max( best , P[ b ][ d ] - P[ a ][ d ] - P[ b ][ c ] +
                              P[ a ][ c ] );
  CHECK( sottomat( A ) == best );
  }
 CHECK( sottomat( { { -1 , -2 } , { -3 , -4 } } ) == 0 );
 CHECK( sottomat( { { 5 } } ) == 5 );
 CHECK( sottomat( std::vector< std::vector< int64_t > >(
         100 , std::vector< int64_t >( 100 , 3 ) ) ) == 30000 );
 }

/*-------------------------- 2026-10-22, Sorting ---------------------------*/

// random vectors against std::sort, and the edge cases
ADS_TEST( "asdlab-2026-lab02-sort" )
{
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 300 ; ++it ) {
  std::vector< int > A( g() % 200 );
  for( auto & x : A )
   x = int( g() % 50 ) - 25;
  auto B = A;
  sortLab( A );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 std::vector< int > E , one = { 4 } , eq( 50 , 7 ) , big( 200000 );
 sortLab( E );
 sortLab( one );
 sortLab( eq );
 CHECK( E.empty() && ( one == std::vector< int >( { 4 } ) ) &&
        ( eq == std::vector< int >( 50 , 7 ) ) );
 for( auto & x : big )
  x = int( g() );
 auto sb = big;
 sortLab( big );
 std::sort( sb.begin() , sb.end() );
 CHECK( big == sb );
 }

/*------------------------- 2026-10-22, Intervalli -------------------------*/

using Intervals = std::vector< std::pair< int , int > >;

// the longest uncovered period on the grid of the half units, whose even
// places are the points k / 2 and the odd ones the open segments between
// them, the first one on ties
std::optional< std::pair< int , int > > intervalliBrute( const Intervals & I )
{
 int lo = I[ 0 ].first , hi = I[ 0 ].second;
 for( const auto & [ a , b ] : I ) {
  lo = std::min( lo , a );
  hi = std::max( hi , b );
  }
 std::optional< std::pair< int , int > > best;
 int start = -1;
 for( int x = 2 * lo ; x <= 2 * hi ; ++x ) {
  bool cov = false;
  for( const auto & [ a , b ] : I )
   cov = cov || ( ( 2 * a <= x ) && ( x <= 2 * b ) );
  if( ! cov && ( start < 0 ) )
   start = x;
  if( cov && ( start >= 0 ) ) {                // the run [ start , x )
   const std::pair< int , int > gap( ( start - 1 ) / 2 , x / 2 );
   if( ! best || ( gap.second - gap.first > best->second - best->first ) )
    best = gap;
   start = -1;
   }
  }
 return( best );
 }

// the brute force on random small intervals, and the edge cases
ADS_TEST( "asdlab-2026-lab02-intervalli" )
{
 using P = std::vector< std::pair< int , int > >;
 CHECK( intervalli( P( { { 1 , 3 } , { 7 , 9 } , { 4 , 5 } } ) ) ==
        std::pair( 5 , 7 ) );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 1000 ; ++it ) {
  P I( 1 + g() % 6 );
  for( auto & [ a , b ] : I ) {
   a = int( g() % 30 );
   b = a + int( g() % 6 );
   }
  CHECK( intervalli( I ) == intervalliBrute( I ) );
  }
 CHECK( ! intervalli( P( { { 2 , 2 } } ) ) );
 CHECK( ! intervalli( P( { { 1 , 3 } , { 3 , 5 } } ) ) );  // touching
 CHECK( intervalli( P( { { 0 , 0 } , { 1 , 1 } , { 3 , 3 } } ) ) ==
        std::pair( 1 , 3 ) );
 CHECK( intervalli( P( { { 0 , 1 } , { 1000000000 , 1000000000 } } ) ) ==
        std::pair( 1 , 1000000000 ) );
 CHECK_THROWS( intervalli( P() ) , contract_violation );
 }

/*----------------------- 2026-10-22, Funzioni ceil ------------------------*/

// random queries against a linear scan, and the edge cases
ADS_TEST( "asdlab-2026-lab02-ceil" )
{
 CHECK( ceilLab( { 1 , 5 , 9 } , { 2 , 2 , 5 , 6 } ) ==
        std::vector< int >( { 2 , 6 , -1 } ) );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 500 ; ++it ) {
  std::vector< int > A( g() % 20 ) , V;
  for( auto & x : A )
   x = int( g() % 30 );
  std::sort( A.begin() , A.end() );
  std::set< int > S;
  const size_t q = g() % 15;
  while( S.size() < q )
   S.insert( int( g() % 34 ) - 2 );
  V.assign( S.begin() , S.end() );
  std::shuffle( V.begin() , V.end() , g );
  const auto r = ceilLab( V , A );
  for( size_t i = 0 ; i < V.size() ; ++i ) {
   int e = -1;
   for( int a : A )
    if( a > V[ i ] ) {
     e = a;
     break;
     }
   CHECK( r[ i ] == e );
   }
  }
 CHECK( ceilLab( { 3 } , {} ) == std::vector< int >( { -1 } ) );
 CHECK( ceilLab( {} , { 1 } ).empty() );
 CHECK( ceilLab( { 4 , 3 } , std::vector< int >( 9 , 4 ) ) ==
        std::vector< int >( { -1 , 4 } ) );
 }

/*--------------------- 2026-10-22, Ordinamento pesato ---------------------*/

// the fewest swaps and the least cost of sorting a permutation of up to 15
// items, by a shortest path search over the permutations (Dijkstra), each
// one coded in 4 bits per item
std::pair< int , int64_t > sortpesatoBrute( const std::vector< int > & A )
{
 const int n = int( A.size() );
 auto code = [ n ]( const std::vector< int > & p ) {
  uint64_t c = 0;
  for( int i = 0 ; i < n ; ++i )
   c |= uint64_t( p[ i ] ) << ( 4 * i );
  return( c );
  };
 std::vector< int > id( n );
 std::iota( id.begin() , id.end() , 1 );
 const uint64_t goal = code( id );
 auto search = [ & ]( bool weighted ) {
  std::map< uint64_t , int64_t > D = { { code( A ) , 0 } };
  using Item = std::pair< int64_t , uint64_t >;
  std::priority_queue< Item , std::vector< Item > , std::greater<> > Q;
  Q.emplace( 0 , code( A ) );
  while( ! Q.empty() ) {
   const auto [ d , c ] = Q.top();
   Q.pop();
   if( c == goal )
    return( d );
   if( d > D[ c ] )
    continue;
   for( int i = 0 ; i < n ; ++i )
    for( int j = i + 1 ; j < n ; ++j ) {
     const uint64_t a = ( c >> ( 4 * i ) ) & 15u ,
                    b = ( c >> ( 4 * j ) ) & 15u;
     const uint64_t q = c ^ ( ( a ^ b ) << ( 4 * i ) ) ^
                        ( ( a ^ b ) << ( 4 * j ) );
     const int64_t nd = d + ( weighted ? int64_t( a + b ) : 1 );
     const auto it = D.find( q );
     if( ( it == D.end() ) || ( nd < it->second ) ) {
      D[ q ] = nd;
      Q.emplace( nd , q );
      }
     }
   }
  return( int64_t( -1 ) );
  };
 return( std::pair( int( search( false ) ) , search( true ) ) );
 }

// every permutation of up to 6 items and random ones of 7 against the
// search, and the edge cases
ADS_TEST( "asdlab-2026-lab02-sortpesato" )
{
 for( int n = 1 ; n <= 6 ; ++n ) {
  std::vector< int > p( n );
  std::iota( p.begin() , p.end() , 1 );
  do
   CHECK( sortpesato( p ) == sortpesatoBrute( p ) );
  while( std::next_permutation( p.begin() , p.end() ) );
  }
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 25 ; ++it ) {
  std::vector< int > p( 7 );
  std::iota( p.begin() , p.end() , 1 );
  std::shuffle( p.begin() , p.end() , g );
  CHECK( sortpesato( p ) == sortpesatoBrute( p ) );
  }
 const std::vector< int > p8 = { 1 , 2 , 4 , 5 , 6 , 7 , 8 , 3 };
 CHECK( sortpesato( p8 ) == sortpesatoBrute( p8 ) );   // through the item 1
 CHECK( sortpesato( p8 ) == std::pair( 5 , int64_t( 33 + 3 + 7 ) ) );
 CHECK( sortpesato( {} ) == std::pair( 0 , int64_t( 0 ) ) );
 CHECK( sortpesato( { 2 , 1 } ) == std::pair( 1 , int64_t( 3 ) ) );
 // a cycle of large items, cheaper through the item 1
 CHECK( sortpesato( { 1 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 2 } ).second ==
        std::min< int64_t >( 54 + 7 * 2 , 54 + 2 + 10 ) );
 std::vector< int > big( 100000 );
 std::iota( big.begin() , big.end() , 1 );
 std::rotate( big.begin() , big.begin() + 1 , big.end() );
 CHECK( sortpesato( big ) == std::pair( 99999 , int64_t( 5000050000LL ) +
                                        99998 ) );
 CHECK_THROWS( sortpesato( { 1 , 1 } ) , contract_violation );
 }

/*-------------------- 2026-10-29, Visita di un albero ---------------------*/

// the three visits by stacks, on the lists of the children; the inorder
// takes a node when its first child is done, or at once if it is a leaf
Visits visitaAlberoBrute( const std::vector< int > & P , int r )
{
 const int n = int( P.size() );
 std::vector< std::vector< int > > C( n );
 for( int u = 0 ; u < n ; ++u )
  if( u != r )
   C[ P[ u ] ].push_back( u );
 Visits V;
 std::vector< std::tuple< int , size_t , bool > > S = { { r , 0 , false } };
 V.pre.push_back( r );
 while( ! S.empty() ) {
  auto & [ u , k , done ] = S.back();
  if( ! done && ( k == std::min< size_t >( 1 , C[ u ].size() ) ) ) {
   V.in.push_back( u );
   done = true;
   }
  if( k == C[ u ].size() ) {
   V.post.push_back( u );
   S.pop_back();
   continue;
   }
  const int v = C[ u ][ k++ ];
  V.pre.push_back( v );
  S.emplace_back( v , 0 , false );
  }
 return( V );
 }

// random trees, binary ones among them, against the visits by stacks
ADS_TEST( "asdlab-2026-lab03-visita-albero" )
{
 // a root 0 with children 1 and 2, the node 1 with the children 3 and 4
 const auto V = visitaAlbero( { -1 , 0 , 0 , 1 , 1 } , 0 );
 CHECK( V.pre == std::vector< int >( { 0 , 1 , 3 , 4 , 2 } ) );
 CHECK( V.in == std::vector< int >( { 3 , 1 , 4 , 0 , 2 } ) );
 CHECK( V.post == std::vector< int >( { 3 , 4 , 1 , 2 , 0 } ) );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 300 ; ++it ) {
  const int n = 1 + int( g() % 40 );
  std::vector< int > p( n ) , P( n , -1 ) , kids( n , 0 );
  std::iota( p.begin() , p.end() , 0 );
  std::shuffle( p.begin() , p.end() , g );
  for( int i = 1 ; i < n ; ++i ) {
   int j;
   do
    j = int( g() % unsigned( i ) );
   while( ( it % 3 == 0 ) && ( kids[ j ] == 2 ) );   // binary trees
   ++kids[ j ];
   P[ p[ i ] ] = p[ j ];
   }
  const auto A = visitaAlbero( P , p[ 0 ] ) ,
             B = visitaAlberoBrute( P , p[ 0 ] );
  CHECK( ( A.pre == B.pre ) && ( A.in == B.in ) && ( A.post == B.post ) );
  }
 const auto one = visitaAlbero( { -1 } , 0 );
 CHECK( one.pre == one.in && one.in == one.post && one.post.size() == 1 );
 std::vector< int > chain( 2000 );             // a path of 2000 nodes
 std::iota( chain.begin() , chain.end() , -1 );
 const auto C = visitaAlbero( chain , 0 );
 CHECK( ( C.pre.front() == 0 ) && ( C.post.front() == 1999 ) &&
        ( C.in.front() == 1999 ) );
 }

/*----------------- 2026-10-29, Visita di grafo orientato ------------------*/

// the examples of the graphs of the statement, random graphs against the
// closure, and the edge cases
ADS_TEST( "asdlab-2026-lab03-visita-grafo" )
{
 const Arcs g1 = { { 0 , 1 } , { 0 , 2 } , { 4 , 3 } , { 4 , 5 } ,
                   { 3 , 2 } , { 2 , 4 } , { 3 , 0 } };
 const Arcs g2 = { { 0 , 2 } , { 0 , 4 } , { 1 , 4 } , { 3 , 2 } ,
                   { 2 , 4 } , { 4 , 3 } };
 CHECK( visitaGrafo( 6 , g1 , 0 ) == 6 && visitaGrafo( 6 , g1 , 1 ) == 1 );
 CHECK( visitaGrafo( 5 , g2 , 0 ) == 4 && visitaGrafo( 5 , g2 , 1 ) == 4 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 12 ) , m = int( g() % 30 );
  const auto A = randomArcs( g , n , m , true );
  const auto R = closure( n , A );
  const int s = int( g() % unsigned( n ) );
  CHECK( visitaGrafo( n , A , s ) ==
         int( std::count( R[ s ].begin() , R[ s ].end() , 1 ) ) );
  }
 CHECK( visitaGrafo( 1 , {} , 0 ) == 1 );
 Arcs P;
 for( int u = 0 ; u + 1 < 100000 ; ++u )
  P.emplace_back( u , u + 1 );
 CHECK( visitaGrafo( 100000 , P , 0 ) == 100000 );
 CHECK( visitaGrafo( 100000 , P , 99999 ) == 1 );
 }

/*-------------- 2026-10-29, Diametro su grafo non orientato ---------------*/

// the largest finite distance by Floyd-Warshall
int diametroBrute( int n , const Arcs & E )
{
 const int inf = 1 << 20;
 std::vector< std::vector< int > > D( n , std::vector< int >( n , inf ) );
 for( int u = 0 ; u < n ; ++u )
  D[ u ][ u ] = 0;
 for( const auto & [ u , v ] : E )
  if( u != v )
   D[ u ][ v ] = D[ v ][ u ] = 1;
 for( int k = 0 ; k < n ; ++k )
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    D[ i ][ j ] = std::min( D[ i ][ j ] , D[ i ][ k ] + D[ k ][ j ] );
 int best = 0;
 for( const auto & row : D )
  for( int x : row )
   if( x < inf )
    best = std::max( best , x );
 return( best );
 }

// the graph of the statement, random graphs against Floyd-Warshall, and
// the edge cases
ADS_TEST( "asdlab-2026-lab03-diametro" )
{
 const Arcs g1 = { { 0 , 1 } , { 0 , 2 } , { 4 , 3 } , { 4 , 5 } ,
                   { 3 , 2 } , { 2 , 4 } , { 3 , 0 } };
 CHECK( diametro( 6 , g1 ) == 4 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 14 ) , m = int( g() % 25 );
  const auto E = randomArcs( g , n , m , false );
  CHECK( diametro( n , E ) == diametroBrute( n , E ) );
  }
 CHECK( diametro( 1 , {} ) == 0 && diametro( 3 , {} ) == 0 );
 Arcs P;
 for( int u = 0 ; u + 1 < 1500 ; ++u )
  P.emplace_back( u , u + 1 );
 CHECK( diametro( 1500 , P ) == 1499 );
 }

/*---------------------- 2026-11-19, Alberi Red-Black ----------------------*/

// a Red-Black tree of its own, with the insertion of Cormen, Leiserson,
// Rivest and Stein, on vectors; its preorder of ( key , red )
std::vector< std::pair< int , bool > > redBlackOther(
                                                const std::vector< int > & K )
{
 std::vector< int > key , l , r , p;
 std::vector< char > red;
 int root = -1;
 auto rotate = [ & ]( int x , bool left ) {
  auto & down = left ? r : l;
  auto & other = left ? l : r;
  const int y = down[ x ];
  down[ x ] = other[ y ];
  if( other[ y ] >= 0 )
   p[ other[ y ] ] = x;
  p[ y ] = p[ x ];
  if( p[ x ] < 0 )
   root = y;
  else if( x == l[ p[ x ] ] )
   l[ p[ x ] ] = y;
  else
   r[ p[ x ] ] = y;
  other[ y ] = x;
  p[ x ] = y;
  };
 for( int k : K ) {
  int y = -1 , x = root;
  while( x >= 0 ) {
   y = x;
   x = k < key[ x ] ? l[ x ] : r[ x ];
   }
  int z = int( key.size() );
  key.push_back( k );
  l.push_back( -1 );
  r.push_back( -1 );
  p.push_back( y );
  red.push_back( 1 );
  if( y < 0 )
   root = z;
  else if( k < key[ y ] )
   l[ y ] = z;
  else
   r[ y ] = z;
  while( ( p[ z ] >= 0 ) && red[ p[ z ] ] ) {
   const int zp = p[ z ] , gp = p[ zp ];
   const bool left = zp == l[ gp ];
   const int u = left ? r[ gp ] : l[ gp ];
   if( ( u >= 0 ) && red[ u ] ) {
    red[ zp ] = red[ u ] = 0;
    red[ gp ] = 1;
    z = gp;
    continue;
    }
   if( z == ( left ? r[ zp ] : l[ zp ] ) ) {
    z = zp;
    rotate( z , left );
    }
   red[ p[ z ] ] = 0;
   red[ p[ p[ z ] ] ] = 1;
   rotate( p[ p[ z ] ] , ! left );
   }
  red[ root ] = 0;
  }
 std::vector< std::pair< int , bool > > out;
 std::vector< int > S;
 if( root >= 0 )
  S.push_back( root );
 while( ! S.empty() ) {
  const int u = S.back();
  S.pop_back();
  out.emplace_back( key[ u ] , red[ u ] != 0 );
  if( r[ u ] >= 0 )
   S.push_back( r[ u ] );
  if( l[ u ] >= 0 )
   S.push_back( l[ u ] );
  }
 return( out );
 }

// the preorder of ( key , red ) of the tree of the library, and whether it
// keeps the order of the keys, a black root, no red node with a red child
// and one number of black nodes on every path to a leaf
std::pair< std::vector< std::pair< int , bool > > , bool > redBlackCheck(
                                          const RedBlackTree< int , int > & T )
{
 using Link = RedBlackTree< int , int >::Link;
 std::vector< std::pair< int , bool > > out;
 bool ok = T.isNil( T.root() ) || ( T.root()->color == Color::BLACK );
 std::function< int( const Link * , int64_t , int64_t ) > rec =
  [ & ]( const Link * u , int64_t lo , int64_t hi ) {
  if( T.isNil( u ) )
   return( 1 );
  const int k = T.key( u );
  const bool red = u->color == Color::RED;
  out.emplace_back( k , red );
  ok = ok && ( k > lo ) && ( k < hi );
  if( red )
   for( const Link * c : { u->left , u->right } )
    ok = ok && ( T.isNil( c ) || ( c->color == Color::BLACK ) );
  const int a = rec( u->left , lo , k ) , b = rec( u->right , k , hi );
  ok = ok && ( a == b );
  return( a + ( red ? 0 : 1 ) );
  };
 rec( T.root() , std::numeric_limits< int64_t >::min() ,
      std::numeric_limits< int64_t >::max() );
 return( std::pair( out , ok ) );
 }

// random, sorted and reversed insertions: the constraints of the tree of
// the library, and its shape and colours against the insertion of CLRS
ADS_TEST( "asdlab-2026-lab04-red-black" )
{
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 300 ; ++it ) {
  std::set< int > S;
  const size_t n = g() % 60;
  while( S.size() < n )
   S.insert( 1 + int( g() % 1000 ) );
  std::vector< int > K( S.begin() , S.end() );
  if( it % 3 == 1 )
   std::reverse( K.begin() , K.end() );
  else if( it % 3 == 2 )
   std::shuffle( K.begin() , K.end() , g );
  const auto [ pre , ok ] = redBlackCheck( redBlack( K ) );
  CHECK( ok && ( pre == redBlackOther( K ) ) && ( pre.size() == n ) );
  }
 const auto [ e , eok ] = redBlackCheck( redBlack( {} ) );
 CHECK( e.empty() && eok );
 const auto [ o , ook ] = redBlackCheck( redBlack( { 5 } ) );
 CHECK( ook && ( o == std::vector< std::pair< int , bool > >( { { 5 ,
                                                               false } } ) ) );
 std::vector< int > big( 3000 );
 std::iota( big.begin() , big.end() , 1 );
 const auto [ b , bok ] = redBlackCheck( redBlack( big ) );
 CHECK( bok && ( b == redBlackOther( big ) ) );
 }

/*----------------- 2026-11-19, Individuazione di un ciclo -----------------*/

// whether Kahn's removal of the nodes with no entering arc leaves some
std::pair< bool , std::vector< int > > kahn( int n , const Arcs & A )
{
 std::vector< int > in( n , 0 ) , S , out;
 std::vector< std::vector< int > > adj( n );
 for( const auto & [ u , v ] : A ) {
  adj[ u ].push_back( v );
  ++in[ v ];
  }
 for( int u = 0 ; u < n ; ++u )
  if( in[ u ] == 0 )
   S.push_back( u );
 while( ! S.empty() ) {
  const int u = S.back();
  S.pop_back();
  out.push_back( u );
  for( int v : adj[ u ] )
   if( --in[ v ] == 0 )
    S.push_back( v );
  }
 return( std::pair( int( out.size() ) < n , out ) );
 }

// connected graphs with at most one cycle (a tree and maybe one more arc)
// and random graphs against Kahn's removal, and the edge cases
ADS_TEST( "asdlab-2026-lab04-ciclo" )
{
 const Arcs g1 = { { 0 , 1 } , { 0 , 2 } , { 4 , 3 } , { 4 , 5 } ,
                   { 3 , 2 } , { 2 , 4 } , { 3 , 0 } };
 CHECK( ciclo( 6 , g1 ) );
 std::mt19937 g( 1 );
 int yes = 0;
 for( int it = 0 ; it < 500 ; ++it ) {
  const int n = 1 + int( g() % 15 );
  Arcs A;
  if( it % 2 ) {
   for( int u = 1 ; u < n ; ++u ) {
    const int v = int( g() % unsigned( u ) );
    if( g() % 2 )
     A.emplace_back( u , v );
    else
     A.emplace_back( v , u );
    }
   if( ( n > 1 ) && ( g() % 2 ) ) {
    const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
    if( u != v )
     A.emplace_back( u , v );
    }
   }
  else
   A = simpleArcs( g , n , int( g() % unsigned( 2 * n ) ) );
  const bool c = ciclo( n , A );
  CHECK( c == kahn( n , A ).first );
  yes += c;
  }
 CHECK( ( yes > 50 ) && ( yes < 450 ) );
 CHECK( ! ciclo( 1 , {} ) && ciclo( 2 , { { 0 , 1 } , { 1 , 0 } } ) );
 Arcs R;                                       // one cycle of 2000
 for( int u = 0 ; u < 2000 ; ++u )
  R.emplace_back( u , ( u + 1 ) % 2000 );
 CHECK( ciclo( 2000 , R ) );
 R.pop_back();
 CHECK( ! ciclo( 2000 , R ) );
 }

/*------------------ 2026-11-19, Numero di cammini minimi ------------------*/

// the number of shortest paths, by all the simple paths from s
uint64_t numCamminiBrute( int n , const Arcs & A , int s , int t )
{
 int best = n + 1;
 uint64_t count = 0;
 std::vector< char > in( n , 0 );
 auto rec = [ & ]( auto & self , int u , int len ) -> void {
  if( u == t ) {
   if( len < best ) {
    best = len;
    count = 0;
    }
   count += len == best;
   return;
   }
  in[ u ] = 1;
  for( const auto & [ a , b ] : A )
   if( ( a == u ) && ! in[ b ] )
    self( self , b , len + 1 );
  in[ u ] = 0;
  };
 rec( rec , s , 0 );
 return( count );
 }

// random graphs against the simple paths, a lattice with a binomial
// number of paths, and the edge cases
ADS_TEST( "asdlab-2026-lab04-numcammini" )
{
 const Arcs g1 = { { 0 , 1 } , { 0 , 2 } , { 1 , 3 } , { 2 , 3 } ,
                   { 3 , 4 } , { 0 , 4 } };
 CHECK( numCammini( 5 , g1 , 0 , 3 ) == 2 );
 CHECK( numCammini( 5 , g1 , 0 , 4 ) == 1 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 8 ) , m = int( g() % 25 );
  const auto A = simpleArcs( g , n , m );
  const int s = int( g() % unsigned( n ) ) , t = int( g() % unsigned( n ) );
  CHECK( numCammini( n , A , s , t ) == numCamminiBrute( n , A , s , t ) );
  }
 CHECK( numCammini( 3 , { { 0 , 1 } } , 0 , 2 ) == 0 );
 CHECK( numCammini( 1 , {} , 0 , 0 ) == 1 );
 const int k = 30;                              // a k x k lattice
 Arcs L;
 for( int i = 0 ; i < k ; ++i )
  for( int j = 0 ; j < k ; ++j ) {
   if( i + 1 < k )
    L.emplace_back( i * k + j , ( i + 1 ) * k + j );
   if( j + 1 < k )
    L.emplace_back( i * k + j , i * k + j + 1 );
   }
 uint64_t binom = 1;                           // C( 58 , 29 )
 for( int i = 1 ; i <= k - 1 ; ++i )
  binom = binom * uint64_t( k - 1 + i ) / uint64_t( i );
 CHECK( numCammini( k * k , L , 0 , k * k - 1 ) == binom );
 }

/*--------------- 2026-11-26, Componente fortemente connessa ---------------*/

// random graphs against the mutual reachability of the closure, and the
// edge cases
ADS_TEST( "asdlab-2026-lab05-componente" )
{
 const Arcs g2 = { { 0 , 2 } , { 0 , 4 } , { 1 , 4 } , { 3 , 2 } ,
                   { 2 , 4 } , { 4 , 3 } };
 CHECK( componente( 5 , g2 ) == 3 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 12 ) , m = int( g() % 30 );
  const auto A = randomArcs( g , n , m , true );
  const auto R = closure( n , A );
  int best = 0;
  for( int u = 0 ; u < n ; ++u ) {
   int c = 0;
   for( int v = 0 ; v < n ; ++v )
    c += R[ u ][ v ] && R[ v ][ u ];
   best = std::max( best , c );
   }
  CHECK( componente( n , A ) == best );
  }
 CHECK( componente( 1 , {} ) == 1 && componente( 4 , {} ) == 1 );
 Arcs R;
 for( int u = 0 ; u < 3000 ; ++u )
  R.emplace_back( u , ( u + 1 ) % 3000 );
 CHECK( componente( 3000 , R ) == 3000 );
 }

/*------------------- 2026-11-26, Ordinamento topologico -------------------*/

// random acyclic graphs: the order has every node once and every arc
// forward, and the edge cases
ADS_TEST( "asdlab-2026-lab05-toporder" )
{
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 30 ) , m = int( g() % 80 );
  const auto A = randomDag( g , n , m );
  const auto S = topOrder( n , A );
  std::vector< int > pos( n , -1 );
  for( int i = 0 ; i < int( S.size() ) ; ++i )
   if( ( S[ i ] >= 0 ) && ( S[ i ] < n ) && ( pos[ S[ i ] ] < 0 ) )
    pos[ S[ i ] ] = i;
  CHECK( ( int( S.size() ) == n ) &&
         std::none_of( pos.begin() , pos.end() ,
                       []( int x ) { return( x < 0 ); } ) );
  for( const auto & [ u , v ] : A )
   CHECK( pos[ u ] < pos[ v ] );
  }
 CHECK( topOrder( 1 , {} ) == std::vector< int >( { 0 } ) );
 CHECK( topOrder( 3 , { { 2 , 1 } , { 1 , 0 } } ) ==
        std::vector< int >( { 2 , 1 , 0 } ) );
 }

/*--------------------- 2026-11-26, Cammino piu' lungo ---------------------*/

// the most arcs of a path from u, by all the paths
int longestFrom( int u , const std::vector< std::vector< int > > & adj )
{
 int best = 0;
 for( int v : adj[ u ] )
  best = std::max( best , 1 + longestFrom( v , adj ) );
 return( best );
 }

// random acyclic graphs against all the paths, and the edge cases
ADS_TEST( "asdlab-2026-lab05-camminolungo" )
{
 CHECK( camminoLungo( 5 , { { 0 , 1 } , { 1 , 2 } , { 0 , 3 } , { 3 , 4 } ,
                            { 4 , 2 } } ) == 3 );
 std::mt19937 g( 1 );
 for( int it = 0 ; it < 400 ; ++it ) {
  const int n = 1 + int( g() % 12 ) , m = int( g() % 30 );
  const auto A = randomDag( g , n , m );
  std::vector< std::vector< int > > adj( n );
  for( const auto & [ u , v ] : A )
   adj[ u ].push_back( v );
  int best = 0;
  for( int u = 0 ; u < n ; ++u )
   best = std::max( best , longestFrom( u , adj ) );
  CHECK( camminoLungo( n , A ) == best );
  }
 CHECK( camminoLungo( 1 , {} ) == 0 && camminoLungo( 3 , {} ) == 0 );
 Arcs P;
 for( int u = 0 ; u + 1 < 3000 ; ++u )
  P.emplace_back( u , u + 1 );
 CHECK( camminoLungo( 3000 , P ) == 2999 );
 CHECK_THROWS( camminoLungo( 2 , { { 0 , 1 } , { 1 , 0 } } ) ,
               contract_violation );
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
/*------------------------ End File exam_asdlab.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
