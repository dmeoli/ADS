/*--------------------------------------------------------------------------*/
/*----------------------------- File ch15.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 15, Ricerca locale: the local search of the minimum
 * spanning tree on Figure 15.1 and 15.3, the neighbourhood of a sequence,
 * Shell Sort on Figura 15.4 with four sequences of gaps, the maximum flow
 * on Figura 15.5 with three ways of augmenting and against the minimum
 * cuts enumerated, and the exercises 15.1 to 15.10, each a test named
 * after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 15,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
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

#include "ADS/graph/EulerianCycle.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/ShellSort.h"

#include <algorithm>
#include <bit>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< int >;
using Longs = std::vector< long >;
using Edge = WeightedEdge< long >;
using Arc = std::tuple< int , int , long >;

const Augment ways[] = { Augment::FORD_FULKERSON , Augment::EDMONDS_KARP ,
                         Augment::DINIC };

/// the maximum flow from s to t of the arcs, and the network
long maxflow( int n , const std::vector< Arc > & A , int s , int t ,
              Augment how = Augment::DINIC )
{
 FlowNetwork N( n );
 for( auto [ u , v , c ] : A )
  N.addEdge( u , v , c );
 return( N.maxFlow( s , t , how ) );
 }

/// the least capacity of a cut ( S , V - S ), s in S, t not, enumerated
long min_cut( int n , const std::vector< Arc > & A , int s , int t ,
              std::vector< unsigned > * cuts = nullptr )
{
 long best = -1;
 for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
  if( ! ( S >> s & 1 ) || ( S >> t & 1 ) ) continue;
  long c = 0;
  for( auto [ u , v , w ] : A )
   if( ( S >> u & 1 ) && ! ( S >> v & 1 ) ) c += w;
  if( best < 0 || c < best ) {
   best = c;
   if( cuts ) cuts->clear();
   }
  if( cuts && c == best ) cuts->push_back( S );
  }
 return( best );
 }

/// a random network of n nodes
std::vector< Arc > random_arcs( int n , int m , int maxc , std::mt19937 & g )
{
 std::vector< Arc > A;
 for( int k = 0 ; k < m ; ++k ) {
  const int u = int( g() % n ) , v = int( g() % n );
  if( u != v ) A.emplace_back( u , v , long( g() % maxc ) );
  }
 return( A );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// Figure 15.1 and 15.3: the local search of the minimum spanning tree from
// the tree of weight 68, whose neighbourhood has 13 trees (a missing edge
// added, an edge of the circuit removed); with the best improvement it
// goes 68, 56, 46, 43, the global optimum
ADS_TEST( "sec15.local-search" )
{
 const std::vector< Edge > E = { { 2 , 4 , 1 } , { 3 , 4 , 2 } ,
  { 5 , 7 , 3 } , { 1 , 3 , 6 } , { 1 , 2 , 9 } , { 2 , 3 , 11 } ,
  { 4 , 5 , 13 } , { 4 , 7 , 15 } , { 3 , 6 , 18 } , { 2 , 5 , 20 } ,
  { 6 , 7 , 25 } , { 4 , 6 , 28 } };
 std::set< int > T = { 4 , 0 , 1 , 6 , 11 , 7 };   // [1,2] [2,4] [3,4] ...
 auto weight = [ & ]( const std::set< int > & X ) {
  long w = 0;
  for( int i : X ) w += E[ i ].w;
  return( w );
  };
 // the tree path between the ends of an edge, as edges of T
 auto circuit = [ & ]( const std::set< int > & X , int e ) {
  std::map< int , std::vector< std::pair< int , int > > > adj;
  for( int i : X ) {
   adj[ E[ i ].u ].push_back( { E[ i ].v , i } );
   adj[ E[ i ].v ].push_back( { E[ i ].u , i } );
   }
  std::map< int , std::pair< int , int > > via;     // node -> ( from , e )
  std::vector< int > Q = { E[ e ].u };
  via[ E[ e ].u ] = { -1 , -1 };
  for( std::size_t i = 0 ; i < Q.size() ; ++i )
   for( auto [ v , k ] : adj[ Q[ i ] ] )
    if( ! via.count( v ) ) { via[ v ] = { Q[ i ] , k }; Q.push_back( v ); }
  Ints path;
  for( int v = E[ e ].v ; v != E[ e ].u ; v = via[ v ].first )
   path.push_back( via[ v ].second );
  return( path );
  };
 Longs weights = { weight( T ) };
 int neighbours = 0;
 for( int e = 0 ; e < int( E.size() ) ; ++e )
  if( ! T.count( e ) ) neighbours += int( circuit( T , e ).size() );
 CHECK( neighbours == 13 );
 for( ; ; ) {
  long best = 0;
  int add = -1 , drop = -1;
  for( int e = 0 ; e < int( E.size() ) ; ++e )
   if( ! T.count( e ) )
    for( int f : circuit( T , e ) )
     if( E[ e ].w - E[ f ].w < best ) {
      best = E[ e ].w - E[ f ].w;
      add = e;
      drop = f;
      }
  if( add < 0 ) break;
  T.erase( drop );
  T.insert( add );
  weights.push_back( weight( T ) );
  }
 CHECK( weights == Longs( { 68 , 56 , 46 , 43 } ) );
 CHECK( weight( T ) == [ & ] {
         long w = 0;
         for( const auto & e : kruskal( 8 , E ) ) w += e.w;
         return( w ); }() );
 }

// the neighbourhood of 1 , 7 , 3 , 4, an item moved: 9 sequences, two of
// which lower the inversions; countInversions() on empty, sorted, equal
// and reversed sequences and against the pairs counted
ADS_TEST( "sec15.1.neighbourhood" )
{
 const Ints A = { 1 , 7 , 3 , 4 };
 std::set< Ints > I;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < A.size() ; ++j ) {
   Ints B = A;
   const int x = B[ i ];
   B.erase( B.begin() + i );
   B.insert( B.begin() + j , x );
   if( B != A ) I.insert( B );
   }
 CHECK( I.size() == 9 );
 auto inv = []( Ints X ) { return( countInversions( X ) ); };
 std::set< Ints > better;
 for( const auto & B : I ) if( inv( B ) < inv( A ) ) better.insert( B );
 const std::set< Ints > two = { { 1 , 3 , 7 , 4 } , { 1 , 3 , 4 , 7 } };
 CHECK( inv( A ) == 2 && better == two );
 // the edges of countInversions(): none in the empty, in one item, in a
 // sorted sequence and among equal items; n ( n - 1 ) / 2 in a reversed
 // one; the equal items not counted; against the pairs counted
 CHECK( inv( {} ) == 0 && inv( { 5 } ) == 0 && inv( { 1 , 2 , 3 } ) == 0 );
 CHECK( inv( Ints( 50 , 7 ) ) == 0 );
 Ints R( 1000 );
 std::iota( R.rbegin() , R.rend() , 0 );
 CHECK( inv( R ) == 1000L * 999 / 2 );
 CHECK( inv( { 2 , 1 , 2 , 1 } ) == 3 );
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 100 ; ++t ) {
  Ints X( g() % 30 );
  for( auto & x : X ) x = int( g() % 5 );
  long ref = 0;
  for( std::size_t i = 0 ; i < X.size() ; ++i )
   for( std::size_t j = i + 1 ; j < X.size() ; ++j ) ref += X[ i ] > X[ j ];
  CHECK( inv( X ) == ref );
  }
 }

// Figura 15.4: 15 , 14 , ... , 1 with the gaps 13 , 4 , 1; every sequence
// of gaps against std::sort; the gaps of n = 0 , 1 and 10^6, equal,
// sorted and reversed items, gaps without 1, and the precondition
ADS_TEST( "sec15.1.shell-sort" )
{
 Ints A( 15 );
 std::iota( A.rbegin() , A.rend() , 1 );
 CHECK( knuthGaps( 15 ) == std::vector< std::size_t >( { 1 , 4 , 13 } ) );
 shellSort( A , std::vector< std::size_t >( { 13 } ) );
 CHECK( A == Ints( { 2 , 1 , 13 , 12 , 11 , 10 , 9 , 8 , 7 , 6 , 5 , 4 , 3 ,
                     15 , 14 } ) );
 shellSort( A , std::vector< std::size_t >( { 4 } ) );
 CHECK( A == Ints( { 2 , 1 , 5 , 4 , 3 , 6 , 9 , 8 , 7 , 10 , 13 , 12 , 11 ,
                     15 , 14 } ) );
 shellSort( A , std::vector< std::size_t >( { 1 } ) );
 CHECK( std::is_sorted( A.begin() , A.end() ) );
 std::mt19937 g( 1 );
 for( int n : { 0 , 1 , 2 , 100 , 5000 } ) {
  Ints B( n );
  for( auto & b : B ) b = int( g() % 1000 );
  Ints R = B;
  std::sort( R.begin() , R.end() );
  for( const auto & gaps : { knuthGaps( n ) , shellGaps( n ) , prattGaps( n ) ,
                             ciuraGaps( n ) } ) {
   Ints C = B;
   shellSort( C , gaps );
   CHECK( C == R && gaps.front() == 1 );
   }
  }
 // the edges: all equal, sorted, reversed, by another order; the gaps of
 // n = 0 , 1 are { 1 } and the gaps of a large n increase below it; gaps
 // larger than n; without the gap 1 the sequence is only h-sorted; gaps
 // empty, with 0 or not increasing
 for( const auto & gaps : { knuthGaps( 0 ) , shellGaps( 1 ) , prattGaps( 0 ) ,
                            ciuraGaps( 1 ) , hibbardGaps( 0 ) } )
  CHECK( gaps == std::vector< std::size_t >( { 1 } ) );
 for( const auto & gaps : { knuthGaps( 1000000 ) , shellGaps( 1000000 ) ,
                            prattGaps( 1000000 ) , ciuraGaps( 1000000 ) ,
                            hibbardGaps( 1000000 ) } ) {
  CHECK( gaps.front() == 1 && gaps.back() <= 1000000 );
  CHECK( std::adjacent_find( gaps.begin() , gaps.end() ,
                             std::greater_equal<>() ) == gaps.end() );
  }
 for( int kind = 0 ; kind < 3 ; ++kind ) {
  Ints B( 777 );
  if( kind == 0 ) std::fill( B.begin() , B.end() , 4 );
  if( kind == 1 ) std::iota( B.begin() , B.end() , -300 );
  if( kind == 2 ) std::iota( B.rbegin() , B.rend() , -300 );
  Ints R = B;
  std::sort( R.begin() , R.end() );
  Ints C = B;
  shellSort( C );
  CHECK( C == R );
  shellSort( C , std::greater<>() );
  CHECK( std::is_sorted( C.rbegin() , C.rend() ) );
  }
 Ints D = { 3 , 1 , 2 };
 shellSort( D , std::vector< std::size_t >( { 1 , 10 , 100 } ) );
 CHECK( D == Ints( { 1 , 2 , 3 } ) );
 Ints E = { 5 , 4 , 3 , 2 , 1 , 0 };
 shellSort( E , std::vector< std::size_t >( { 2 } ) );
 CHECK( E == Ints( { 1 , 0 , 3 , 2 , 5 , 4 } ) );
 CHECK_THROWS( shellSort( E , std::vector< std::size_t >() ) ,
               contract_violation );
 CHECK_THROWS( shellSort( E , std::vector< std::size_t >( { 0 , 1 } ) ) ,
               contract_violation );
 CHECK_THROWS( shellSort( E , std::vector< std::size_t >( { 1 , 4 , 4 } ) ) ,
               contract_violation );
 }

// Figura 15.5: |f| = 7, the cut S = { 1 } of capacity 7, by every way of
// augmenting; against the minimum cuts enumerated on random networks; no
// edge, t unreachable, parallel, null, looping and antiparallel edges, a
// second maxFlow(), huge capacities, a deep chain, and the preconditions
ADS_TEST( "sec15.2.max-flow" )
{
 // the nodes from 1, s = 1, t = 4
 const std::vector< Arc > A = { { 1 , 2 , 5 } , { 1 , 3 , 2 } , { 2 , 3 , 2 } ,
                                { 2 , 4 , 4 } , { 3 , 4 , 3 } };
 for( Augment how : ways ) {
  FlowNetwork N( 5 );
  for( auto [ u , v , c ] : A ) N.addEdge( u , v , c );
  CHECK( N.maxFlow( 1 , 4 , how ) == 7 );
  const auto S = N.minCut( 1 );
  CHECK( S[ 1 ] && ! S[ 2 ] && ! S[ 3 ] && ! S[ 4 ] );
  }
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 2 + int( g() % 7 );
  const auto R = random_arcs( n , int( g() % 20 ) , 10 , g );
  const long ref = min_cut( n , R , 0 , n - 1 );
  for( Augment how : ways )
   CHECK( maxflow( n , R , 0 , n - 1 , how ) == ref );
  }
 // the edges, by every way: no edge, and t unreachable, give 0 and the cut
 // { s }; parallel edges add up, an edge of capacity 0 and a self-loop
 // carry nothing, the antiparallel ones are kept apart; a second maxFlow()
 // starts from the flow there is and finds nothing more; capacities near
 // the largest long; a chain of 2 10^4 nodes, the deepest augmenting
 // path, whose levels overflowed the stack of the recursive blocking flow
 // of Dinic (each addEdge() checks the network at ADS_CONTRACTS=3, so a
 // longer chain costs quadratic time here)
 const long big = std::numeric_limits< long >::max() / 4;
 for( Augment how : ways ) {
  FlowNetwork E( 3 );
  CHECK( E.maxFlow( 0 , 2 , how ) == 0 );
  CHECK( E.minCut( 0 ) == std::vector< bool >( { true , false , false } ) );
  FlowNetwork N( 4 );
  const int a = N.addEdge( 0 , 1 , 3 ) , b = N.addEdge( 0 , 1 , 4 );
  N.addEdge( 1 , 1 , 100 );
  N.addEdge( 1 , 0 , 50 );
  N.addEdge( 1 , 2 , 0 );
  const int c = N.addEdge( 1 , 3 , 100 );
  N.addEdge( 2 , 3 , 9 );
  CHECK( N.maxFlow( 0 , 3 , how ) == 7 && N.maxFlow( 0 , 3 , how ) == 0 );
  CHECK( N.flow( a ) == 3 && N.flow( b ) == 4 && N.flow( c ) == 7 );
  CHECK( N.flow( c - 6 ) == 0 && N.flow( c - 4 ) == 0 );   // loop, back
  CHECK( N.capacity( c ) == 100 && N.flow( c + 1 ) == -7 );
  CHECK( N.maxFlow( 3 , 2 , how ) == 0 );                  // unreachable
  CHECK( N.minCut( 0 ) == std::vector< bool >( { true , false , false ,
                                                 false } ) );
  FlowNetwork B( 3 );
  B.addEdge( 0 , 1 , big );
  B.addEdge( 0 , 1 , big );
  B.addEdge( 1 , 2 , 3 * big );
  CHECK( B.maxFlow( 0 , 2 , how ) == 2 * big );
  const int L = 20000;
  FlowNetwork C( L );
  for( int v = 1 ; v < L ; ++v ) C.addEdge( v - 1 , v , 1 + v % 5 );
  CHECK( C.maxFlow( 0 , L - 1 , how ) == 1 );
  CHECK( C.maxFlow( L - 1 , 0 , how ) == 1 );       // cancels the unit
  }
 FlowNetwork N( 2 );
 CHECK_THROWS( FlowNetwork( -1 ) , contract_violation );
 CHECK_THROWS( N.addEdge( 0 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( N.addEdge( -1 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( N.addEdge( 0 , 1 , -1 ) , contract_violation );
 CHECK_THROWS( N.maxFlow( 1 , 1 ) , contract_violation );
 CHECK_THROWS( N.maxFlow( 0 , 2 ) , contract_violation );
 CHECK_THROWS( N.flow( 0 ) , contract_violation );
 CHECK_THROWS( N.capacity( -1 ) , contract_violation );
 CHECK_THROWS( N.minCut( 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the minimum cuts of Figura 15.5( a ): S = { 1 } and S = { 1 , 2 , 3 },
// capacity 7 (the solution of the book lists cuts of another graph, of
// six nodes)
ADS_TEST( "ex15.1" )
{
 const std::vector< Arc > A = { { 0 , 1 , 5 } , { 0 , 2 , 2 } , { 1 , 2 , 2 } ,
                                { 1 , 3 , 4 } , { 2 , 3 , 3 } };
 std::vector< unsigned > cuts;
 CHECK( min_cut( 4 , A , 0 , 3 , &cuts ) == 7 );
 CHECK( cuts == std::vector< unsigned >( { 0b0001 , 0b0111 } ) );
 }

// the schedule by local search: swapping two adjacent programs when it
// lowers the total time of completion ends at the order of Shortest Job
// First
ADS_TEST( "ex15.2" )
{
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 50 ; ++t ) {
  Longs d( 1 + g() % 10 );
  for( auto & x : d ) x = 1 + long( g() % 20 );
  std::vector< std::size_t > A( d.size() );
  std::iota( A.begin() , A.end() , 0 );
  std::shuffle( A.begin() , A.end() , g );
  for( bool better = true ; better ; ) {
   better = false;
   for( std::size_t i = 0 ; i + 1 < A.size() ; ++i ) {
    auto B = A;
    std::swap( B[ i ] , B[ i + 1 ] );
    if( totalCompletion( d , B ) < totalCompletion( d , A ) ) {
     A = B;
     better = true;
     }
    }
   }
  CHECK( totalCompletion( d , A ) ==
         totalCompletion( d , shortestJobFirst( d ) ) );
  }
 }

// the Eulerian cycle: each edge once, closed; none if a node has more
// edges out than in or the edges are in two components; no node, isolated
// nodes, a long cycle, isEulerianCycle() on wrong cycles, and the
// precondition
ADS_TEST( "ex15.3" )
{
 ListGraph<> g( 5 , true );
 for( auto [ u , v ] : std::vector< std::pair< int , int > >(
       { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } , { 0 , 3 } , { 3 , 4 } ,
         { 4 , 0 } } ) )
  g.insertEdge( u , v );
 const auto c = eulerianCycle( g );
 CHECK( c && c->size() == 7 && c->front() == c->back() );
 std::multiset< std::pair< int , int > > used;
 for( std::size_t i = 0 ; i + 1 < c->size() ; ++i )
  used.insert( { ( *c )[ i ] , ( *c )[ i + 1 ] } );
 CHECK( used.size() == 6 && used.count( { 0 , 3 } ) == 1 );
 g.deleteEdge( 4 , 0 );
 CHECK( ! eulerianCycle( g ) );
 ListGraph<> h( 4 , true );
 h.insertEdge( 0 , 1 ); h.insertEdge( 1 , 0 );
 h.insertEdge( 2 , 3 ); h.insertEdge( 3 , 2 );
 CHECK( ! eulerianCycle( h ) );
 CHECK( eulerianCycle( ListGraph<>( 3 ) )->empty() );
 // the edges: no node; a cycle of two edges with an isolated node beside
 // it; two cycles through one node; the oriented cycle of 10^4 nodes; an
 // unoriented graph; isEulerianCycle() refuses an open path, a repeated
 // edge and a missing one
 CHECK( eulerianCycle( ListGraph<>( 0 , true ) )->empty() );
 ListGraph<> loop( 3 , true );
 loop.insertEdge( 1 , 2 );
 loop.insertEdge( 2 , 1 );
 const auto lc = eulerianCycle( loop );
 CHECK( lc && lc->size() == 3 && isEulerianCycle( loop , *lc ) );
 loop.insertEdge( 1 , 0 );
 loop.insertEdge( 0 , 1 );
 const auto l2 = eulerianCycle( loop );
 CHECK( l2 && l2->size() == 5 && isEulerianCycle( loop , *l2 ) );
 const int L = 10000;
 ListGraph<> ring( L , true );
 for( int v = 0 ; v < L ; ++v ) ring.insertEdge( v , ( v + 1 ) % L );
 const auto rc = eulerianCycle( ring );
 CHECK( rc && int( rc->size() ) == L + 1 && isEulerianCycle( ring , *rc ) );
 ListGraph<> tri( 3 , true );
 tri.insertEdge( 0 , 1 ); tri.insertEdge( 1 , 2 ); tri.insertEdge( 2 , 0 );
 CHECK( isEulerianCycle( tri , { 0 , 1 , 2 , 0 } ) );
 CHECK( ! isEulerianCycle( tri , { 0 , 1 , 2 } ) );
 CHECK( ! isEulerianCycle( tri , { 0 , 1 , 2 , 0 , 1 } ) );
 CHECK( ! isEulerianCycle( tri , { 0 , 1 , 0 , 0 } ) );
 CHECK( ! isEulerianCycle( tri , {} ) && ! isEulerianCycle( h , {} ) );
 CHECK( isEulerianCycle( ListGraph<>( 2 , true ) , {} ) );
 CHECK_THROWS( eulerianCycle( ListGraph<>( 2 , false ) ) ,
               contract_violation );
 }

// the matrix with given sums of rows and columns exists if and only if the
// two sums agree: the northwest corner fills it in O( n^2 ), and so does a
// maximum flow from the rows to the columns
ADS_TEST( "ex15.4" )
{
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int n = 1 + int( g() % 5 );
  Longs r( n ) , c( n );
  for( auto & x : r ) x = long( g() % 10 );
  for( auto & x : c ) x = long( g() % 10 );
  if( g() % 2 ) {                                    // agreeing sums
   c = r;
   std::shuffle( c.begin() , c.end() , g );
   }
  const bool agree = std::accumulate( r.begin() , r.end() , 0L ) ==
                     std::accumulate( c.begin() , c.end() , 0L );
  // the northwest corner
  std::vector< Longs > M( n , Longs( n , 0 ) );
  Longs rr = r , cc = c;
  for( int i = 0 , j = 0 ; i < n && j < n ; ) {
   const long x = std::min( rr[ i ] , cc[ j ] );
   M[ i ][ j ] = x;
   rr[ i ] -= x;
   cc[ j ] -= x;
   if( rr[ i ] == 0 ) ++i; else ++j;
   }
  bool ok = true;
  for( int i = 0 ; i < n ; ++i ) {
   long sr = 0 , sc = 0;
   for( int j = 0 ; j < n ; ++j ) { sr += M[ i ][ j ]; sc += M[ j ][ i ]; }
   ok = ok && sr == r[ i ] && sc == c[ i ];
   }
  CHECK( ok == agree );
  std::vector< Arc > A;                              // s = 2n, t = 2n + 1
  for( int i = 0 ; i < n ; ++i ) {
   A.emplace_back( 2 * n , i , r[ i ] );
   A.emplace_back( n + i , 2 * n + 1 , c[ i ] );
   for( int j = 0 ; j < n ; ++j ) A.emplace_back( i , n + j , 1000 );
   }
  const long f = maxflow( 2 * n + 2 , A , 2 * n , 2 * n + 1 );
  CHECK( ( f == std::accumulate( r.begin() , r.end() , 0L ) &&
           f == std::accumulate( c.begin() , c.end() , 0L ) ) == agree );
  }
 }

// M = M1 + M2, the lines of M_i summing to at most l_i and all of M_i to
// at most t_i: a circulation where the edge ( row i , column j ) carries
// M1[ i ][ j ] in [ 0 , M[ i ][ j ] ], the edge ( s , row i ) the row of
// M1 in [ row_i - l2 , l1 ], the edge ( column j , t ) its column in
// [ col_j - l2 , l1 ], the edge ( t , s ) the total in [ T - t2 , t1 ];
// against the enumeration of the M1 of small matrices; the edges of
// feasibleCirculation() and its preconditions
ADS_TEST( "ex15.5" )
{
 std::mt19937 g( 5 );
 int yes = 0;
 for( int k = 0 ; k < 200 ; ++k ) {
  const int m = 1 + int( g() % 2 ) , n = 1 + int( g() % 2 );
  std::vector< Longs > M( m , Longs( n ) );
  long total = 0;
  for( auto & r : M ) for( auto & x : r ) { x = long( g() % 4 ); total += x; }
  const long l1 = long( g() % 6 ) , l2 = long( g() % 6 ) ,
             t1 = long( g() % 10 ) , t2 = long( g() % 10 );
  // the circulation: rows 0 .. m - 1, columns m .. m + n - 1, s , t
  const int s = m + n , t = m + n + 1;
  std::vector< BoundedEdge > E;
  for( int i = 0 ; i < m ; ++i ) {
   long row = 0;
   for( int j = 0 ; j < n ; ++j ) {
    E.push_back( { i , m + j , 0 , M[ i ][ j ] } );
    row += M[ i ][ j ];
    }
   E.push_back( { s , i , std::max( 0L , row - l2 ) , l1 } );
   }
  for( int j = 0 ; j < n ; ++j ) {
   long col = 0;
   for( int i = 0 ; i < m ; ++i ) col += M[ i ][ j ];
   E.push_back( { m + j , t , std::max( 0L , col - l2 ) , l1 } );
   }
  E.push_back( { t , s , std::max( 0L , total - t2 ) , t1 } );
  const bool flow = feasibleCirculation( m + n + 2 , E );
  // the enumeration of M1
  bool brute = false;
  const int cells = m * n;
  Longs X( cells , 0 );
  for( ; ; ) {
   bool ok = true;
   long s1 = 0;
   for( int i = 0 ; i < m ; ++i ) {
    long a = 0 , b = 0;
    for( int j = 0 ; j < n ; ++j ) {
     a += X[ i * n + j ];
     b += M[ i ][ j ] - X[ i * n + j ];
     }
    ok = ok && a <= l1 && b <= l2;
    s1 += a;
    }
   for( int j = 0 ; j < n ; ++j ) {
    long a = 0 , b = 0;
    for( int i = 0 ; i < m ; ++i ) {
     a += X[ i * n + j ];
     b += M[ i ][ j ] - X[ i * n + j ];
     }
    ok = ok && a <= l1 && b <= l2;
    }
   ok = ok && s1 <= t1 && total - s1 <= t2;
   if( ok ) { brute = true; break; }
   int c = 0;                                       // the next X
   while( c < cells && X[ c ] == M[ c / n ][ c % n ] ) X[ c++ ] = 0;
   if( c == cells ) break;
   ++X[ c ];
   }
  CHECK( flow == brute );
  yes += brute;
  }
 CHECK( yes > 20 && yes < 180 );
 // the edges of feasibleCirculation(): no node and no edge circulate; a
 // low bound above the high one does not; a self-loop circulates any flow
 // in its bounds; a cycle forced by its low bounds, with the flows given;
 // a low bound with no way back does not; ends out of range
 std::vector< long long > f;
 CHECK( feasibleCirculation( 0 , {} , &f ) && f.empty() );
 CHECK( ! feasibleCirculation( 2 , { { 0 , 1 , 3 , 2 } ,
                                     { 1 , 0 , 0 , 9 } } ) );
 CHECK( feasibleCirculation( 1 , { { 0 , 0 , 4 , 6 } } , &f ) &&
        f.size() == 1 && f[ 0 ] >= 4 && f[ 0 ] <= 6 );
 CHECK( feasibleCirculation( 3 , { { 0 , 1 , 2 , 5 } , { 1 , 2 , 0 , 9 } ,
                                   { 2 , 0 , 4 , 4 } } , &f ) &&
        f == std::vector< long long >( { 4 , 4 , 4 } ) );
 CHECK( ! feasibleCirculation( 3 , { { 0 , 1 , 2 , 5 } , { 1 , 2 , 0 , 9 } ,
                                     { 2 , 0 , 0 , 1 } } ) );
 CHECK( ! feasibleCirculation( 2 , { { 0 , 1 , 1 , 1 } } ) );
 CHECK_THROWS( feasibleCirculation( -1 , {} ) , contract_violation );
 CHECK_THROWS( feasibleCirculation( 2 , { { 0 , 2 , 0 , 1 } } ) ,
               contract_violation );
 }

// the variants: many sources and sinks (a super source and a super sink),
// capacities on the nodes (each node split in two by an edge of its
// capacity), an unoriented graph (both orientations of each edge)
ADS_TEST( "ex15.6" )
{
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 4 + int( g() % 4 );
  const auto A = random_arcs( n , int( g() % 15 ) , 8 , g );
  // sources 0 , 1 and sinks n - 2 , n - 1
  auto B = A;
  B.emplace_back( n , 0 , 1000 );
  B.emplace_back( n , 1 , 1000 );
  B.emplace_back( n - 2 , n + 1 , 1000 );
  B.emplace_back( n - 1 , n + 1 , 1000 );
  long best = -1;                                   // the cuts enumerated
  for( unsigned S = 0 ; S < ( 1u << n ) ; ++S ) {
   if( !( S & 1 ) || !( S >> 1 & 1 ) || ( S >> ( n - 1 ) & 1 ) ||
       ( S >> ( n - 2 ) & 1 ) ) continue;
   long c = 0;
   for( auto [ u , v , w ] : A )
    if( ( S >> u & 1 ) && !( S >> v & 1 ) ) c += w;
   best = best < 0 ? c : std::min( best , c );
   }
  CHECK( maxflow( n + 2 , B , n , n + 1 ) == best );
  // the capacities of the nodes: u_in = u , u_out = n + u
  Longs cap( n );
  for( auto & x : cap ) x = long( g() % 6 );
  std::vector< Arc > C;
  for( int u = 0 ; u < n ; ++u )
   C.emplace_back( u , n + u , ( u == 0 || u == n - 1 ) ? 1000 : cap[ u ] );
  for( auto [ u , v , w ] : A ) C.emplace_back( n + u , v , w );
  long nb = -1;              // cuts of edges and nodes: brute on the split
  const int N = 2 * n;
  for( unsigned S = 0 ; S < ( 1u << N ) ; ++S ) {
   if( !( S & 1 ) || ( S >> ( 2 * n - 1 ) & 1 ) ) continue;
   long c = 0;
   for( auto [ u , v , w ] : C )
    if( ( S >> u & 1 ) && !( S >> v & 1 ) ) c += w;
   nb = nb < 0 ? c : std::min( nb , c );
   }
  CHECK( maxflow( N , C , 0 , 2 * n - 1 ) == nb );
  // unoriented: both orientations
  std::vector< Arc > U;
  for( auto [ u , v , w ] : A ) {
   U.emplace_back( u , v , w );
   U.emplace_back( v , u , w );
   }
  CHECK( maxflow( n , U , 0 , n - 1 ) == min_cut( n , U , 0 , n - 1 ) );
  }
 }

// the edge independent paths from u to v: the maximum flow with unit
// capacities; on two triangles joined at a node, 1, and on the cube, 3
ADS_TEST( "ex15.7" )
{
 const std::vector< std::pair< int , int > > bow = { { 0 , 1 } , { 1 , 2 } ,
  { 2 , 0 } , { 2 , 3 } , { 3 , 4 } , { 4 , 2 } };
 std::vector< Arc > A;
 for( auto [ u , v ] : bow ) {
  A.emplace_back( u , v , 1 );
  A.emplace_back( v , u , 1 );
  }
 CHECK( maxflow( 5 , A , 0 , 4 ) == 2 );          // two edges at node 2
 std::vector< Arc > Q;
 for( int u = 0 ; u < 8 ; ++u )
  for( int b = 0 ; b < 3 ; ++b )
   Q.emplace_back( u , u ^ ( 1 << b ) , 1 );
 CHECK( maxflow( 8 , Q , 0 , 7 ) == 3 );
 }

// the neighbourhood of a matching: an augmenting chain between two free
// nodes; the local search of Kuhn on bipartite graphs against the flow
// and against the enumeration of the matchings
ADS_TEST( "ex15.8" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 100 ; ++t ) {
  const int a = 1 + int( g() % 4 ) , b = 1 + int( g() % 4 );
  std::vector< std::pair< int , int > > E;
  for( int i = 0 ; i < a ; ++i )
   for( int j = 0 ; j < b ; ++j )
    if( g() % 2 ) E.emplace_back( i , j );
  std::vector< int > mate( b , -1 );
  int size = 0;
  for( int i = 0 ; i < a ; ++i ) {
   std::vector< bool > seen( b , false );
   std::function< bool( int ) > aug = [ & ]( int x ) {
    for( auto [ u , v ] : E )
     if( u == x && ! seen[ v ] ) {
      seen[ v ] = true;
      if( mate[ v ] < 0 || aug( mate[ v ] ) ) {
       mate[ v ] = x;
       return( true );
       }
      }
    return( false );
    };
   size += aug( i );
   }
  int best = 0;
  for( unsigned S = 0 ; S < ( 1u << E.size() ) ; ++S ) {
   std::set< int > L , R;
   bool ok = true;
   for( std::size_t k = 0 ; k < E.size() && ok ; ++k )
    if( S >> k & 1 )
     ok = L.insert( E[ k ].first ).second && R.insert( E[ k ].second ).second;
   if( ok ) best = std::max( best , std::popcount( S ) );
   }
  CHECK( size == best );
  }
 }

// the bipartite matching as a flow with unit capacities, by Dinic in
// O( m sqrt( n ) ): the example of Figura 15.6 has a matching of 3; no
// edge between the sides, the complete graph, all on one node
ADS_TEST( "ex15.9" )
{
 // U = { 1 , 2 , 3 }, V = { 4 , 5 , 6 , 7 }, s = 0 , t = 8
 const std::vector< std::pair< int , int > > B = { { 1 , 4 } , { 1 , 5 } ,
  { 2 , 5 } , { 2 , 6 } , { 3 , 6 } , { 3 , 7 } };
 std::vector< Arc > A;
 for( int u = 1 ; u <= 3 ; ++u ) A.emplace_back( 0 , u , 1 );
 for( int v = 4 ; v <= 7 ; ++v ) A.emplace_back( v , 8 , 1 );
 for( auto [ u , v ] : B ) A.emplace_back( u , v , 1 );
 for( Augment how : ways ) CHECK( maxflow( 9 , A , 0 , 8 , how ) == 3 );
 // the edges: no edge between the sides, 0; the complete K_{3,4}, 3; all
 // of U on one node of V, 1
 std::vector< Arc > Z( A.begin() , A.begin() + 7 ) , K = Z , S = Z;
 for( int u = 1 ; u <= 3 ; ++u ) {
  for( int v = 4 ; v <= 7 ; ++v ) K.emplace_back( u , v , 1 );
  S.emplace_back( u , 5 , 1 );
  }
 for( Augment how : ways ) {
  CHECK( maxflow( 9 , Z , 0 , 8 , how ) == 0 );
  CHECK( maxflow( 9 , K , 0 , 8 , how ) == 3 );
  CHECK( maxflow( 9 , S , 0 , 8 , how ) == 1 );
  }
 // a large one: Dinic needs few phases
 std::mt19937 g( 9 );
 const int n = 2000;
 FlowNetwork N( 2 * n + 2 );
 for( int i = 0 ; i < n ; ++i ) {
  N.addEdge( 2 * n , i , 1 );
  N.addEdge( n + i , 2 * n + 1 , 1 );
  N.addEdge( i , n + i , 1 );                        // a perfect matching
  for( int k = 0 ; k < 3 ; ++k ) N.addEdge( i , n + int( g() % n ) , 1 );
  }
 long long paths = 0;
 CHECK( N.maxFlow( 2 * n , 2 * n + 1 , Augment::DINIC , &paths ) == n );
 }

// the edge connectivity: the least maximum flow from a node to the
// others, with unit capacities both ways, against the cuts enumerated
ADS_TEST( "ex15.10" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 50 ; ++t ) {
  const int n = 2 + int( g() % 6 );
  std::vector< std::pair< int , int > > E;
  for( int v = 1 ; v < n ; ++v ) E.emplace_back( int( g() % v ) , v );
  for( int k = 0 ; k < int( g() % 10 ) ; ++k ) {
   const int u = int( g() % n ) , v = int( g() % n );
   if( u != v ) E.emplace_back( u , v );
   }
  std::vector< Arc > A;
  for( auto [ u , v ] : E ) {
   A.emplace_back( u , v , 1 );
   A.emplace_back( v , u , 1 );
   }
  long lambda = -1;
  for( int v = 1 ; v < n ; ++v ) {
   const long f = maxflow( n , A , 0 , v );
   lambda = lambda < 0 ? f : std::min( lambda , f );
   }
  long best = -1;
  for( unsigned S = 1 ; S + 1 < ( 1u << n ) ; ++S ) {
   long c = 0;
   for( auto [ u , v ] : E ) c += ( S >> u & 1 ) != ( S >> v & 1 );
   best = best < 0 ? c : std::min( best , c );
   }
  CHECK( lambda == best );
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
/*---------------------------- End File ch15.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
