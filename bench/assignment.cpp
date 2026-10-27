/*--------------------------------------------------------------------------*/
/*--------------------------- File assignment.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the assignment problem of Assignment.h, from scratch
 * and after the change of a row, against the minimum cost flows of LEMON
 * (NetworkSimplex, LEMON=1; its CostScaling does not build in C++20, its
 * StaticDigraph using the members of std::allocator removed by it) on the
 * bipartite network of the assignment, and the weighted matching of the
 * Boost Graph Library (maximum_weighted_matching, Edmonds' blossoms on a
 * general graph, the weights a large constant minus the costs), when
 * their headers are found:
 *
 * - assign, the assignment of n rows to n columns of random costs in
 *   [ 0 , 10^6 ), from scratch, in nanoseconds per row (Boost for n up
 *   to 1000 only);
 * - reassign, 64 changes of a random row to new random costs, each
 *   followed by the total cost, in nanoseconds per change: Assignment by
 *   setRow() ("ADS warm"), against a new Assignment solved from scratch
 *   ("ADS scratch", the copy of the costs included) and NetworkSimplex
 *   run again on the costs changed.
 *
 * Each structure runs only if ADS_BENCH_WHO names it, or with no such
 * variable [see Bench.h]; the first structure of each cell is run twice,
 * the second time as "A/A". The contracts are off.
 *
 *   assignment [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/graph/Assignment.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#if __has_include( <lemon/network_simplex.h> ) && defined( ADS_HAS_LEMON )
 #include <lemon/network_simplex.h>
 #include <lemon/smart_graph.h>
 #define ADS_LEMON_MCF 1
#endif

#if __has_include( <boost/graph/maximum_weighted_matching.hpp> )
 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/maximum_weighted_matching.hpp>
 #define ADS_BOOST_MATCHING 1
#endif

using namespace ADS;
using ADS::bench::Options;

namespace {

#ifdef ADS_LEMON_MCF
/// the bipartite network of the assignment in LEMON: a unit of supply at
/// each row, of demand at each column, an arc per pair
struct LemonNet {
 lemon::SmartDigraph g;
 lemon::SmartDigraph::ArcMap< long long > cost{ g };
 lemon::SmartDigraph::NodeMap< long long > supply{ g };
 std::vector< lemon::SmartDigraph::Arc > arc;
 int n;

 LemonNet( int n0 , const std::vector< long long > & C ) : n( n0 ) {
  std::vector< lemon::SmartDigraph::Node > V;
  for( int i = 0 ; i < 2 * n ; ++i ) {
   V.push_back( g.addNode() );
   supply[ V.back() ] = i < n ? 1 : -1;
   }
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    arc.push_back( g.addArc( V[ i ] , V[ n + j ] ) );
    cost[ arc.back() ] = C[ std::size_t( i ) * std::size_t( n ) +
                           std::size_t( j ) ];
    }
  }

 template< class A >
 long long run( void ) {
  A a( g );
  a.costMap( cost ).supplyMap( supply );
  a.run();
  return( a.totalCost() );
  }
 };
#endif

#ifdef ADS_BOOST_MATCHING
/// the weighted matching of Boost on the complete bipartite graph
long long boostMatching( int n , const std::vector< long long > & C )
{
 using G = boost::adjacency_list< boost::vecS , boost::vecS ,
           boost::undirectedS , boost::no_property ,
           boost::property< boost::edge_weight_t , long long > >;
 G g( 2 * n );
 const long long big = 1000000000LL;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   boost::add_edge( i , n + j ,
                    big - C[ std::size_t( i ) * std::size_t( n ) +
                            std::size_t( j ) ] , g );
 std::vector< boost::graph_traits< G >::vertex_descriptor > mate( 2 * n );
 boost::maximum_weighted_matching( g , & mate[ 0 ] );
 long long s = 0;
 for( int i = 0 ; i < n ; ++i )
  s += C[ std::size_t( i ) * std::size_t( n ) + ( mate[ i ] - n ) ];
 return( s );
 }
#endif

/// the cells of the n rows
void assign( int n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< long long > C( std::size_t( n ) * std::size_t( n ) );
 for( auto & c : C )
  c = static_cast< long long >( g() % 1000000 );
 const std::size_t N = std::size_t( n );
 // from scratch
 for( int twice = 0 ; twice < 2 ; ++twice ) {
  const std::string who = twice ? "ADS A/A" : "ADS";
  if( ! ADS::bench::skip( who ) )
   ADS::bench::print( "assign" , "random" , N , who ,
    ADS::bench::median( [ & ]( void ) {
     return( std::make_unique< Assignment<> >( n , C ) ); } ,
     [ & ]( auto & A ) { ADS::bench::sink = std::uint64_t( A->total() ); } ,
     r , N ) );
  }
#ifdef ADS_LEMON_MCF
 if( ! ADS::bench::skip( "LEMON NetworkSimplex" ) )
  ADS::bench::print( "assign" , "random" , N , "LEMON NetworkSimplex" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< LemonNet >( n , C ) ); } , [ & ]( auto & L ) {
    ADS::bench::sink = std::uint64_t( L->template run<
                       lemon::NetworkSimplex< lemon::SmartDigraph ,
                       long long , long long > >() ); } , r , N ) );
#endif
#ifdef ADS_BOOST_MATCHING
 if( ( n <= 1000 ) && ! ADS::bench::skip( "Boost matching" ) )
  ADS::bench::print( "assign" , "random" , N , "Boost matching" ,
   ADS::bench::median( [ & ]( void ) { return( 0 ); } , [ & ]( int ) {
    ADS::bench::sink = std::uint64_t( boostMatching( n , C ) ); } , r ,
    N ) );
#endif

 // a row at a time
 const std::size_t k = 64;
 std::vector< std::pair< int , std::vector< long long > > > R( k );
 for( auto & [ i , row ] : R ) {
  i = int( g() % N );
  row.resize( N );
  for( auto & c : row )
   c = static_cast< long long >( g() % 1000000 );
  }
 for( int twice = 0 ; twice < 2 ; ++twice ) {
  const std::string who = twice ? "ADS warm A/A" : "ADS warm";
  if( ! ADS::bench::skip( who ) )
   ADS::bench::print( "reassign" , "random" , N , who ,
    ADS::bench::median( [ & ]( void ) {
     auto A = std::make_unique< Assignment<> >( n , C );
     ADS::bench::sink = std::uint64_t( A->total() );
     return( A ); } , [ & ]( auto & A ) {
     std::uint64_t h = 0;
     for( const auto & [ i , row ] : R ) {
      A->setRow( i , row );
      h += std::uint64_t( A->total() );
      }
     ADS::bench::sink = h; } , r , k ) );
  }
 if( ! ADS::bench::skip( "ADS scratch" ) )
  ADS::bench::print( "reassign" , "random" , N , "ADS scratch" ,
   ADS::bench::median( [ & ]( void ) { return( C ); } , [ & ]( auto & D ) {
    std::uint64_t h = 0;
    for( const auto & [ i , row ] : R ) {
     std::copy( row.begin() , row.end() ,
                D.begin() + std::ptrdiff_t( std::size_t( i ) * N ) );
     h += std::uint64_t( Assignment<>( n , D ).total() );
     }
    ADS::bench::sink = h; } , r , k ) );
#ifdef ADS_LEMON_MCF
 if( ! ADS::bench::skip( "LEMON NetworkSimplex" ) )
  ADS::bench::print( "reassign" , "random" , N , "LEMON NetworkSimplex" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< LemonNet >( n , C ) ); } , [ & ]( auto & L ) {
    std::uint64_t h = 0;
    for( const auto & [ i , row ] : R ) {
     for( int j = 0 ; j < n ; ++j )
      L->cost[ L->arc[ std::size_t( i ) * N + std::size_t( j ) ] ] =
                                                                  row[ j ];
     h += std::uint64_t( L->template run< lemon::NetworkSimplex<
                         lemon::SmartDigraph , long long , long long > >() );
     }
    ADS::bench::sink = h; } , r , k ) );
#endif
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 300 , 1000 } );
 for( std::size_t n : o.sizes )
  assign( int( n ) , o.runs , o.seed );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File assignment.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
