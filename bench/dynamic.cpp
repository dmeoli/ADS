/*--------------------------------------------------------------------------*/
/*---------------------------- File dynamic.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the graphs that change: the observers of
 * Reoptimization.h, which keep a solution up to date through the events
 * of an Observable, against the algorithms of the book run from scratch
 * after each change; and the union-find with rollback with the offline
 * dynamic connectivity on it.
 *
 * - reopt sssp, the shortest paths from the node 0 of a random oriented
 *   graph G( n , 4 n ), costs in [ 1 , 1000 ]: ShortestPathsObserver
 *   ("observer") against shortestPaths() on a binary heap from scratch
 *   ("scratch");
 * - reopt mst, the minimum spanning forest of the same graph unoriented:
 *   SpanningForestObserver against kruskal() from scratch;
 * - reopt flow, the maximum flow from 0 to n - 1, capacities in
 *   [ 1 , 1000 ]: MaxFlowObserver against Dinic on a network built from
 *   scratch;
 * each on two cases, "single", a query after each change of the weight
 * of a random edge (to a random weight), and "batch", a query after each
 * batch of m / 100 such changes; in nanoseconds per change, for 64
 * changes (scratch) and 1024 (observer), the building of the graph and
 * the first solution not timed;
 * - rollback, n merges of random pairs and their rollback, on
 *   RollbackUnionFind, against the n merges alone on UnionFind (the union
 *   by size with the paths halved, which cannot undo them), per merge;
 * - offline, 4 n random insertions, deletions and questions of
 *   connectivity on n nodes: offlineConnectivity() against a union-find
 *   built again at each question ("rebuild", for n up to 10^4), per
 *   operation.
 * The dynamic algorithms of NetworKit (DynDijkstra, DynBFS, fed the
 * changes by hand) are not among the competitors, since NetworKit is not
 * installed on the machines of the benchmarks. Each structure runs only if
 * ADS_BENCH_WHO names it, or with no such variable [see Bench.h]; the
 * first structure of each cell is run twice, the second time as "A/A".
 * The contracts are off.
 *
 *   dynamic [ -r runs ] [ -s seed ] n1 n2 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/mfset/RollbackUnionFind.h"
#include "ADS/mfset/UnionFind.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

using namespace ADS;
using ADS::bench::Options;

namespace {

using LG = ListGraph< long long >;

/// the edges ( u , v , w ) of G( n , 4 n ), no loop, no repetition
std::vector< std::tuple< int , int , long long > > gnm( int n ,
                                                        unsigned seed )
{
 std::mt19937_64 g( seed );
 std::unordered_set< std::uint64_t > seen;
 std::vector< std::tuple< int , int , long long > > E;
 while( int( E.size() ) < 4 * n ) {
  const int u = int( g() % std::uint64_t( n ) ) ,
            v = int( g() % std::uint64_t( n ) );
  if( ( u == v ) ||
      ! seen.insert( std::uint64_t( std::min( u , v ) ) << 32 |
                     std::uint64_t( std::max( u , v ) ) ).second )
   continue;
  E.emplace_back( u , v , 1 + static_cast< long long >( g() % 1000 ) );
  }
 return( E );
 }

/// the changes: the place of an edge and its new weight
std::vector< std::pair< int , long long > > changes( std::size_t k ,
                                                     std::size_t m ,
                                                     unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::pair< int , long long > > C( k );
 for( auto & c : C )
  c = { int( g() % m ) , 1 + static_cast< long long >( g() % 1000 ) };
 return( C );
 }

/// a graph, its wrapper and an observer, built and solved
template< class O >
struct State {
 LG g;
 std::unique_ptr< O > o;
 std::unique_ptr< Observable< LG > > og;
 };

/// the cells of a problem: make( g ) the observer, query( o ) its answer,
/// fresh( g ) the answer from scratch
template< class Make , class Query , class Fresh >
void reopt( const char * bench , int n , bool directed , int r ,
            unsigned seed , Make && make , Query && query , Fresh && fresh )
{
 const auto E = gnm( n , seed );
 const std::size_t m = E.size();
 for( const char * c : { "single" , "batch" } ) {
  const std::size_t batch = std::string( c ) == "single" ? 1 :
                            std::max< std::size_t >( m / 100 , 1 );
  auto build = [ & ]( void ) {
   using O = std::decay_t< decltype( * make( std::declval< LG & >() ) ) >;
   auto s = std::make_unique< State< O > >();
   s->g = LG( n , directed );
   for( const auto & [ u , v , w ] : E )
    s->g.insertEdge( u , v , w );
   s->o = make( s->g );
   s->og = std::make_unique< Observable< LG > >( s->g );
   s->og->attach( *s->o );
   s->o->setRecomputeRatio( 1 );
   ADS::bench::sink = std::uint64_t( query( *s->o ) );
   return( s );
   };
  const auto C1 = changes( 1024 * batch , m , seed + 1 );
  const auto C0 = changes( 64 * batch , m , seed + 2 );
  for( const std::string who : { "observer" , "observer A/A" ,
                                 "scratch" } ) {
   if( ADS::bench::skip( who ) )
    continue;
   const bool obs = who != "scratch";
   const auto & C = obs ? C1 : C0;
   const double ns = ADS::bench::median( build , [ & ]( auto & s ) {
    std::uint64_t h = 0;
    for( std::size_t i = 0 ; i < C.size() ; i += batch ) {
     {
      auto b = s->og->batch();
      for( std::size_t j = i ; j < std::min( C.size() , i + batch ) ; ++j ) {
       const auto & [ u , v , w0 ] = E[ C[ j ].first ];
       s->og->setWeight( u , v , C[ j ].second );
       ( void ) w0;
       }
      }
     h += std::uint64_t( obs ? query( *s->o ) : fresh( s->g ) );
     }
    ADS::bench::sink = h; } , r , C.size() );
   ADS::bench::print( bench , c , std::size_t( n ) , who , ns );
   }
  }
 }

/// the rollback against the union-find that cannot undo
void rollback( int n , int r , unsigned seed )
{
 std::mt19937 g( seed );
 std::vector< std::pair< int , int > > P( n );
 for( auto & p : P )
  p = { int( g() % unsigned( n ) ) , int( g() % unsigned( n ) ) };
 for( const std::string who : { "RollbackUnionFind" ,
                                "RollbackUnionFind A/A" , "UnionFind" } ) {
  if( ADS::bench::skip( who ) )
   continue;
  double ns;
  if( who != "UnionFind" )
   ns = ADS::bench::median( [ & ]( void ) {
    return( RollbackUnionFind( n ) ); } , [ & ]( auto & U ) {
    // merges in groups of 64, each group rolled back half of the times
    for( std::size_t i = 0 ; i < P.size() ; i += 64 ) {
     const int t = U.checkpoint();
     for( std::size_t j = i ; j < std::min( P.size() , i + 64 ) ; ++j )
      U.merge( P[ j ].first , P[ j ].second );
     if( ( i / 64 ) % 2 )
      U.rollback( t );
     }
    ADS::bench::sink = std::uint64_t( U.count() ); } , r , P.size() );
  else
   ns = ADS::bench::median( [ & ]( void ) { return( UnionFind( n ) ); } ,
                            [ & ]( auto & U ) {
    for( const auto & [ x , y ] : P )
     U.merge( x , y );
    ADS::bench::sink = std::uint64_t( U.count() ); } , r , P.size() );
  ADS::bench::print( "rollback" , "random" , std::size_t( n ) , who , ns );
  }
 }

/// the offline connectivity against a union-find at each question
void offline( int n , int r , unsigned seed )
{
 using Op = ConnectivityOp;
 std::mt19937 g( seed );
 std::vector< Op > ops;
 std::vector< std::pair< int , int > > alive;
 for( int i = 0 ; i < 4 * n ; ++i ) {
  const unsigned k = g() % 4;
  if( ( k < 2 ) || alive.empty() ) {
   const int u = int( g() % unsigned( n ) ) , v = int( g() % unsigned( n ) );
   ops.push_back( { Op::INSERT , u , v } );
   alive.emplace_back( u , v );
   }
  else if( k == 2 ) {
   const std::size_t j = g() % alive.size();
   ops.push_back( { Op::DELETE , alive[ j ].first , alive[ j ].second } );
   alive[ j ] = alive.back();
   alive.pop_back();
   }
  else
   ops.push_back( { Op::CONNECTED , int( g() % unsigned( n ) ) ,
                    int( g() % unsigned( n ) ) } );
  }
 for( const std::string who : { "offlineConnectivity" ,
                                "offlineConnectivity A/A" , "rebuild" } ) {
  if( ADS::bench::skip( who ) || ( ( who == "rebuild" ) && ( n > 10000 ) ) )
   continue;
  double ns;
  if( who != "rebuild" )
   ns = ADS::bench::median( [ & ]( void ) { return( 0 ); } , [ & ]( int ) {
    ADS::bench::sink = std::uint64_t( offlineConnectivity( n , ops ).size() );
    } , r , ops.size() );
  else
   ns = ADS::bench::median( [ & ]( void ) { return( 0 ); } , [ & ]( int ) {
    std::vector< std::pair< int , int > > live;
    std::uint64_t h = 0;
    for( const auto & o : ops )
     if( o.kind == Op::INSERT )
      live.emplace_back( o.u , o.v );
     else if( o.kind == Op::DELETE ) {
      for( std::size_t j = live.size() ; j-- > 0 ; )
       if( live[ j ] == std::pair< int , int >( o.u , o.v ) ) {
        live.erase( live.begin() + std::ptrdiff_t( j ) );
        break;
        }
      }
     else {
      UnionFind U( n );
      for( const auto & [ u , v ] : live )
       U.merge( u , v );
      h += U.same( o.u , o.v );
      }
    ADS::bench::sink = h; } , r , ops.size() );
  ADS::bench::print( "offline" , "random" , std::size_t( n ) , who , ns );
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 10000 , 100000 } );
 for( std::size_t n0 : o.sizes ) {
  const int n = int( n0 );
  reopt( "reopt sssp" , n , true , o.runs , o.seed ,
         []( LG & g ) {
          return( std::make_unique< ShortestPathsObserver< LG > >( g , 0 ) );
          } ,
         []( auto & x ) { return( x.distance( 0 ) + x.tree().d.back() ); } ,
         []( const LG & g ) {
          return( shortestPaths( g , 0 , Frontier::HEAP ).d.back() ); } );
  reopt( "reopt mst" , n , false , o.runs , o.seed ,
         []( LG & g ) {
          return( std::make_unique< SpanningForestObserver< LG > >( g ) );
          } ,
         []( auto & x ) { return( x.weight() ); } ,
         []( const LG & g ) {
          long long s = 0;
          for( const auto & e : kruskal( g ) )
           s += e.w;
          return( s ); } );
  reopt( "reopt flow" , n , true , o.runs , o.seed ,
         [ n ]( LG & g ) {
          return( std::make_unique< MaxFlowObserver< LG > >( g , 0 ,
                                                             n - 1 ) );
          } ,
         []( auto & x ) { return( x.value() ); } ,
         [ n ]( const LG & g ) {
          FlowNetwork N( g.bound() );
          for( int u : g.V() )
           g.forEachEdge( u , [ & ]( int v , long long w ) {
            N.addEdge( u , v , w ); } );
          return( N.maxFlow( 0 , n - 1 ) ); } );
  rollback( n , o.runs , o.seed );
  offline( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File dynamic.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
