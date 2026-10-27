/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_speculative.cpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the deterministic reservations and of the parallel greedy
 * algorithms on them, beyond the book: speculativeFor() on loops whose
 * iterations depend on the earlier ones, and Kruskal, filter-Kruskal, the
 * spanning forest, the maximal independent set and the maximal matching,
 * each against its sequential greedy algorithm on the same order, on
 * every executor (Sequential, pools of 1 to 8 threads, and the executors
 * of the libraries compiled in). On the edge cases (no node, one node,
 * no edge, loops, parallel edges, equal and negative weights,
 * disconnected graphs) and on random graphs above the size at which the
 * parallel paths start, at the level 3 of the contracts; the job of
 * ThreadSanitizer of the CI runs them too.
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

#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/ParallelGreedy.h"
#include "ADS/parallel/SpeculativeFor.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// f( ex ) on each executor: Sequential, pools of 1, 2, 3 and 8 threads,
/// and the executors of the libraries compiled in
template< class F >
void onEach( F && f )
{
 Sequential s;
 f( s );
 for( std::size_t p : { 1 , 2 , 3 , 8 } ) {
  ThreadPool pool( p );
  f( pool );
  }
#if defined( _OPENMP )
 OpenMPExecutor omp( 4 );
 f( omp );
#endif
#if defined( ADS_WITH_FASTFLOW )
 FastFlowExecutor ff( 3 , false );
 f( ff );
#endif
#if defined( ADS_WITH_TBB )
 TBBExecutor tbb( 4 );
 f( tbb );
#endif
 }

/// m random edges on n nodes, weights in [ lo , hi ], loops and parallel
/// edges included
std::vector< WeightedEdge< long long > > randomEdges( int n , int m ,
                                                      long long lo ,
                                                      long long hi ,
                                                      unsigned seed )
{
 std::mt19937 rng( seed );
 std::vector< WeightedEdge< long long > > E;
 for( int i = 0 ; i < m ; ++i )
  E.push_back( { int( rng() % unsigned( n ) ) , int( rng() % unsigned( n ) ) ,
                 lo + static_cast< long long >(
                      rng() % std::uint64_t( hi - lo + 1 ) ) } );
 return( E );
 }

/// the edges without weights
std::vector< std::pair< int , int > > pairsOf(
                         const std::vector< WeightedEdge< long long > > & E )
{
 std::vector< std::pair< int , int > > P;
 for( const auto & e : E )
  P.emplace_back( e.u , e.v );
 return( P );
 }

/// the greedy forest on the order ids, sequentially
std::vector< int > greedy( int n ,
                           const std::vector< std::pair< int , int > > & P ,
                           const std::vector< int > & ids )
{
 // a plain union-find, not UnionFind, whose audits cost n per merge
 std::vector< int > p( std::size_t( std::max( n , 0 ) ) );
 for( int x = 0 ; x < n ; ++x )
  p[ x ] = x;
 auto find = [ & ]( int x ) {
  while( p[ x ] != x )
   x = p[ x ] = p[ p[ x ] ];
  return( x );
  };
 std::vector< int > out;
 for( int i : ids ) {
  const int a = find( P[ i ].first ) , b = find( P[ i ].second );
  if( a != b ) {
   p[ a ] = b;
   out.push_back( i );
   }
  }
 return( out );
 }

/// the places of E by ( w , place )
std::vector< int > byWeight(
                   const std::vector< WeightedEdge< long long > > & E )
{
 std::vector< int > ids( E.size() );
 for( std::size_t i = 0 ; i < E.size() ; ++i )
  ids[ i ] = int( i );
 std::stable_sort( ids.begin() , ids.end() , [ & ]( int a , int b ) {
  return( E[ a ].w < E[ b ].w ); } );
 return( ids );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------- THE RESERVATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "speculative-for" )
{
 // the prefix sums as a loop where each iteration needs the one before:
 // the iteration i waits until i - 1 is done, the worst case, with a
 // round of one iteration committed each time at least
 onEach( [ & ]( auto & ex ) {
  for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                         std::size_t( 5000 ) } ) {
   std::vector< long long > x( n , 1 ) , y( n , 0 );
   std::vector< char > done( n , 0 );
   struct Step {
    std::vector< long long > & x , & y;
    std::vector< char > & done;
    bool reserve( std::size_t ) { return( true ); }
    bool commit( std::size_t i ) {
     if( ( i > 0 ) && ! done[ i - 1 ] )
      return( false );
     y[ i ] = x[ i ] + ( i > 0 ? y[ i - 1 ] : 0 );
     done[ i ] = 1;
     return( true );
     }
    } step{ x , y , done };
   SpeculativeCounts c;
   speculativeFor( ex , step , 0 , n , 50 , & c );
   for( std::size_t i = 0 ; i < n ; ++i )
    CHECK( y[ i ] == static_cast< long long >( i + 1 ) );
   CHECK( ( n > 0 ) || ( c.rounds == 0 ) );
   CHECK( c.tried >= static_cast< long long >( n ) );
   }
  // independent iterations: all done in the first rounds, g = 1
  std::vector< int > z( 10000 , 0 );
  struct Free {
   std::vector< int > & z;
   bool reserve( std::size_t ) { return( true ); }
   bool commit( std::size_t i ) { z[ i ] = int( i ); return( true ); }
   } free{ z };
  SpeculativeCounts c;
  speculativeFor( ex , free , 0 , z.size() , 1 , & c );
  CHECK( c.rounds == 1 );
  for( std::size_t i = 0 ; i < z.size() ; ++i )
   CHECK( z[ i ] == int( i ) );
  // a step that never commits violates the precondition
  struct Stuck {
   bool reserve( std::size_t ) { return( true ); }
   bool commit( std::size_t ) { return( false ); }
   } stuck;
  CHECK_THROWS( speculativeFor( ex , stuck , 0 , 10 ) , contract_violation );
  CHECK_THROWS( speculativeFor( ex , stuck , 5 , 4 ) , contract_violation );
  } );
 Reservation r;
 CHECK( ! r.reserved() );
 r.reserve( 7 );
 r.reserve( 9 );
 r.reserve( 3 );
 CHECK( r.check( 3 ) && ! r.checkReset( 7 ) && r.checkReset( 3 ) );
 CHECK( ! r.reserved() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE FORESTS -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "parallel-kruskal" )
{
 const std::vector< std::tuple< int , int , long long , long long ,
                                unsigned > > cases = {
  { 0 , 0 , 0 , 0 , 1 } ,                 // no node
  { 1 , 3 , 0 , 5 , 2 } ,                 // one node, loops only
  { 50 , 0 , 0 , 0 , 3 } ,                // no edge
  { 2000 , 20000 , 0 , 3 , 4 } ,          // many ties
  { 3000 , 40000 , -1000 , 1000 , 5 } ,   // negative weights
  { 20000 , 15000 , 1 , 1000000 , 6 } ,   // disconnected
  { 500 , 60000 , 7 , 7 , 7 } ,          // all equal, parallel edges
  { 300 , 3000 , -50 , 50 , 8 } };        // small, against the book's
 for( const auto & [ n , m , lo , hi , seed ] : cases ) {
  const auto E = randomEdges( std::max( n , 1 ) , m , lo , hi , seed );
  const auto P = pairsOf( E );
  const auto ref = greedy( n , P , byWeight( E ) );
  long long w = 0;
  for( int i : ref )
   w += E[ i ].w;
  if( m <= 5000 ) {                       // the weight of the book's,
   long long book = 0;                    // whose audit costs n m
   for( const auto & e : kruskal( n , E ) )
    book += e.w;
   CHECK( w == book );
   }
  std::vector< int > order( E.size() );
  for( std::size_t i = 0 ; i < E.size() ; ++i )
   order[ i ] = int( i );
  const auto plain = greedy( n , P , order );
  onEach( [ & ]( auto & ex ) {
   CHECK( parallelKruskal( ex , n , E ) == ref );
   CHECK( filterKruskal( ex , n , E ) == ref );
   CHECK( spanningForest( ex , n , P ) == plain );
   } );
  }
 Sequential s;
 CHECK_THROWS( parallelKruskal( s , 2 , std::vector< WeightedEdge< int > >{
               { 0 , 2 , 1 } } ) , contract_violation );
 CHECK_THROWS( spanningForest( s , -1 , {} ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- THE SET AND THE MATCHING --------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "parallel-mis-matching" )
{
 for( const auto & [ n , m , seed ] : std::vector< std::tuple< int , int ,
                                     unsigned > >{ { 0 , 0 , 1 } ,
                                                   { 1 , 0 , 2 } ,
                                                   { 100 , 0 , 3 } ,
                                                   { 20000 , 60000 , 4 } ,
                                                   { 6000 , 200000 , 5 } } ) {
  // the graph without loops for the set, with them for the matching
  const auto E = randomEdges( std::max( n , 1 ) , m , 0 , 0 , seed );
  std::vector< std::pair< int , int > > simple;
  for( const auto & e : E )
   if( e.u != e.v )
    simple.emplace_back( e.u , e.v );
  const StaticGraph<> g( n , n ? simple :
                         std::vector< std::pair< int , int > >() , false );
  const auto pi = randomOrder( n , seed );
  // the sequential greedy algorithms: v in the set if no neighbour of
  // an earlier place is
  std::vector< int > rank( n );
  for( int i = 0 ; i < n ; ++i )
   rank[ pi[ i ] ] = i;
  std::vector< char > set( n , 0 );
  for( int v : pi ) {
   bool ok = true;
   for( int u : g.adj( v ) )
    if( set[ u ] && ( rank[ u ] < rank[ v ] ) )
     ok = false;
   set[ v ] = ok;
   }
  std::vector< int > mis;
  for( int v = 0 ; v < n ; ++v )
   if( set[ v ] )
    mis.push_back( v );
  const auto P = pairsOf( E );
  const auto po = randomOrder( int( P.size() ) , seed + 100 );
  std::vector< char > busy( std::max( n , 1 ) , 0 ) , t( P.size() , 0 );
  for( int i : po ) {
   const auto [ u , v ] = P[ i ];
   if( ( u != v ) && ! busy[ u ] && ! busy[ v ] ) {
    busy[ u ] = busy[ v ] = 1;
    t[ i ] = 1;
    }
   }
  std::vector< int > match;
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   if( t[ i ] )
    match.push_back( int( i ) );
  onEach( [ & ]( auto & ex ) {
   CHECK( maximalIndependentSet( ex , g , pi ) == mis );
   CHECK( maximalMatching( ex , std::max( n , 1 ) , P , po ) == match );
   } );
  }
 // the order must be a permutation
 Sequential s;
 const StaticGraph<> g( 3 , std::vector< std::pair< int , int > >{
                         { 0 , 1 } } , false );
 CHECK_THROWS( maximalIndependentSet( s , g , { 0 , 0 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( maximalMatching( s , 2 , { { 0 , 1 } } , { 1 } ) ,
               contract_violation );
 const auto p = randomOrder( 1000 , 42 );
 CHECK( p == randomOrder( 1000 , 42 ) );
 CHECK( p != randomOrder( 1000 , 43 ) );
 std::vector< int > q( p );
 std::sort( q.begin() , q.end() );
 for( int i = 0 ; i < 1000 ; ++i )
  CHECK( q[ i ] == i );
 CHECK( randomOrder( 0 , 1 ).empty() );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File engineered_speculative.cpp ------------------*/
/*--------------------------------------------------------------------------*/
