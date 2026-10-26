/*--------------------------------------------------------------------------*/
/*----------------------------- File greedy.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the parallel greedy algorithms of ParallelGreedy.h on
 * t threads (-t, 1 by default), on the pool of ADS (ThreadPool, "pool"),
 * against the examples of ParlayLib that follow the same deterministic
 * reservations, when its directory is given (PARLAY=<dir>):
 * - kruskal, parallelKruskal() and filterKruskal() against
 *   min_spanning_forest() of kruskal.h and of filter_kruskal.h, on random
 *   graphs G( n , 8 n ) with weights in [ 0 , 2^{30} ), per edge;
 * - forest, spanningForest() against spanning_forest() of
 *   spanning_tree.h (which is not deterministic), per edge;
 * - mis, maximalIndependentSet() on the order of the nodes against MIS()
 *   of maximal_independent_set.h, per arc;
 * - matching, maximalMatching() on the order of the edges against
 *   maximal_matching() of maximal_matching.h, per edge.
 * On one thread the sequential greedy algorithms run too ("sequential",
 * the calls on the executor Sequential). GBBS, whose filter-Kruskal and
 * maximal independent set are of the same family, is not on the machines
 * of the benchmarks. Each structure runs only if ADS_BENCH_WHO names it,
 * or with no such variable [see Bench.h]; the first structure of each
 * cell is run twice, the second time as "A/A". The benchmark of a cell is
 * "greedy <algorithm> t<t>", so that the cells of each number of threads
 * make a table of their own. The contracts are off.
 *
 *   greedy [ -r runs ] [ -s seed ] [ -t threads ] n1 n2 ...
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

#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelGreedy.h"
#include "ADS/parallel/ThreadPool.h"

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

// the examples of ParlayLib, each in a namespace of its own, since two of
// them define the same names; their helpers, guarded, first
#if __has_include( <parlay/primitives.h> ) && \
    __has_include( <examples/kruskal.h> )
 #include <parlay/primitives.h>
 #include <parlay/sequence.h>
 #include <examples/helper/speculative_for.h>
 #include <examples/helper/union_find.h>
 #define ADS_HAS_PARLAY_GREEDY 1
namespace pk {
 #include <examples/kruskal.h>
 }
namespace pf {
 #include <examples/filter_kruskal.h>
 }
namespace pm {
 #include <examples/maximal_independent_set.h>
 #include <examples/maximal_matching.h>
 #include <examples/spanning_tree.h>
 }
#endif

using namespace ADS;
using ADS::bench::Options;

namespace {

int threads = 1;                            ///< -t

/// a cell: the structures by name, each a work on nothing, its first one
/// twice, the second time as "A/A"
void cell( const std::string & bench , const char * c , std::size_t n ,
           std::size_t ops , int r ,
           const std::vector< std::pair< std::string ,
                                    std::function< void( void ) > > > & S )
{
 const std::string b = bench + " t" + std::to_string( threads );
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  for( int twice = 0 ; twice < ( i == 0 ? 2 : 1 ) ; ++twice ) {
   const std::string who = S[ i ].first + ( twice ? " A/A" : "" );
   if( ADS::bench::skip( who ) )
    continue;
   S[ i ].second();                         // the threads started
   const double ns = ADS::bench::median( []( void ) { return( 0 ); } ,
                                         [ & ]( int ) { S[ i ].second(); } ,
                                         r , ops );
   ADS::bench::print( b.c_str() , c , n , who , ns );
   }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 // -t threads, before the other options
 std::vector< char * > rest = { argv[ 0 ] };
 for( int i = 1 ; i < argc ; ++i )
  if( ( std::string( argv[ i ] ) == "-t" ) && ( i + 1 < argc ) )
   threads = std::atoi( argv[ ++i ] );
  else
   rest.push_back( argv[ i ] );
 const Options o( int( rest.size() ) , rest.data() , { 1000000 } );
#if defined( ADS_HAS_PARLAY_GREEDY )
 setenv( "PARLAY_NUM_THREADS" , std::to_string( threads ).c_str() , 1 );
#endif
 ThreadPool pool( std::size_t( threads > 0 ? threads : 1 ) );
 Sequential seq;
 for( std::size_t n0 : o.sizes ) {
  const int n = int( n0 );
  std::mt19937_64 g( o.seed );
  std::vector< WeightedEdge< long long > > E;
  std::vector< std::pair< int , int > > P;
  for( std::size_t i = 0 ; i < 8 * n0 ; ++i ) {
   const int u = int( g() % n0 ) , v = int( g() % n0 );
   const long long w = static_cast< long long >( g() % ( 1u << 30 ) );
   E.push_back( { u , v , w } );
   P.emplace_back( u , v );
   }
  std::vector< std::pair< int , int > > simple;
  for( const auto & p : P )
   if( p.first != p.second )
    simple.push_back( p );
  const StaticGraph<> G( n , simple , false );
  std::vector< int > id( n ) , ide( P.size() );
  for( int i = 0 ; i < n ; ++i )
   id[ i ] = i;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   ide[ i ] = int( i );
#if defined( ADS_HAS_PARLAY_GREEDY )
  parlay::sequence< std::tuple< int , int , long long > > PE( E.size() );
  parlay::sequence< std::pair< int , int > > PP( P.size() );
  for( std::size_t i = 0 ; i < E.size() ; ++i ) {
   PE[ i ] = { E[ i ].u , E[ i ].v , E[ i ].w };
   PP[ i ] = P[ i ];
   }
  parlay::sequence< parlay::sequence< int > > PG( n );
  for( int u = 0 ; u < n ; ++u )
   for( int v : G.adj( u ) )
    PG[ u ].push_back( v );
#endif
  using Battery = std::vector< std::pair< std::string ,
                                          std::function< void( void ) > > >;
  auto sink = []( std::size_t x ) { ADS::bench::sink = x; };
  Battery K = {
   { "ADS pool" , [ & ] { sink( parallelKruskal( pool , n , E ).size() ); } } ,
   { "ADS pool filter" , [ & ] {
     sink( filterKruskal( pool , n , E ).size() ); } } };
  Battery F = { { "ADS pool" , [ & ] {
                  sink( spanningForest( pool , n , P ).size() ); } } };
  Battery M = { { "ADS pool" , [ & ] {
                  sink( maximalIndependentSet( pool , G , id ).size() ); } } };
  Battery X = { { "ADS pool" , [ & ] {
                  sink( maximalMatching( pool , n , P , ide ).size() ); } } };
  if( threads == 1 ) {
   K.push_back( { "sequential" , [ & ] {
                  sink( parallelKruskal( seq , n , E ).size() ); } } );
   F.push_back( { "sequential" , [ & ] {
                  sink( spanningForest( seq , n , P ).size() ); } } );
   M.push_back( { "sequential" , [ & ] {
                  sink( maximalIndependentSet( seq , G , id ).size() ); } } );
   X.push_back( { "sequential" , [ & ] {
                  sink( maximalMatching( seq , n , P , ide ).size() ); } } );
   }
#if defined( ADS_HAS_PARLAY_GREEDY )
  K.push_back( { "parlay" , [ & ] {
                 sink( pk::min_spanning_forest( PE , n ).size() ); } } );
  K.push_back( { "parlay filter" , [ & ] {
                 sink( pf::min_spanning_forest( PE , n ).size() ); } } );
  F.push_back( { "parlay" , [ & ] {
                 sink( pm::spanning_forest( PP , n ).size() ); } } );
  M.push_back( { "parlay" , [ & ] { sink( pm::MIS( PG ).size() ); } } );
  X.push_back( { "parlay" , [ & ] {
                 sink( pm::maximal_matching( PP , n ).size() ); } } );
#endif
  cell( "greedy kruskal" , "gnm8" , n0 , E.size() , o.runs , K );
  cell( "greedy forest" , "gnm8" , n0 , P.size() , o.runs , F );
  cell( "greedy mis" , "gnm8" , n0 , std::size_t( G.arcs() ) , o.runs ,
        M );
  cell( "greedy matching" , "gnm8" , n0 , P.size() , o.runs , X );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File greedy.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
