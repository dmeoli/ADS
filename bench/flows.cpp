/*--------------------------------------------------------------------------*/
/*----------------------------- File flows.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the maximum flows of MaxFlow.h, from scratch and from
 * the flow there is after a change of the capacities, against the
 * push-relabel of LEMON (Preflow, two phases, the highest label first, the
 * global relabelings and the gaps) and of the Boost Graph Library
 * (push_relabel_max_flow), when their headers are found:
 *
 * - flow, the maximum flow from scratch, in nanoseconds per arc, on two
 *   cases: "random", G( n , 4 n ) oriented, capacities in [ 1 , 1000 ],
 *   from the node 0 to the node n - 1; "grid", the square grid of n
 *   nodes, each joined to its 4 neighbours by arcs of capacity in
 *   [ 1 , 100 ], and from the source and to the sink by arcs of capacity
 *   in [ 0 , 50 ] (the networks of the cuts of the images); FlowNetwork
 *   by PUSH_RELABEL ("ADS push-relabel") and by DINIC ("ADS Dinic"),
 *   SmartDigraph and Preflow ("LEMON Preflow"), adjacency_list and
 *   push_relabel_max_flow ("Boost push-relabel"), the building of the
 *   graphs not timed;
 * - warm, on the same two networks, 256 changes of the capacity of a
 *   random arc to a random value (below its flow too), each followed by a
 *   maximum flow, in nanoseconds per change: FlowNetwork by setCapacity()
 *   and maxFlow() from the flow there is, by PUSH_RELABEL ("ADS warm
 *   push-relabel") and by DINIC ("ADS warm Dinic"), against Preflow and
 *   push_relabel_max_flow run again from scratch on the capacities
 *   changed, the only way that the two libraries have, on the first 16
 *   changes only (each costs a whole maximum flow).
 *
 * Each structure runs only if ADS_BENCH_WHO names it, or with no such
 * variable [see Bench.h]; the first structure of each cell is run twice,
 * the second time as "A/A". The contracts are off.
 *
 *   flows [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/graph/MaxFlow.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#if __has_include( <lemon/preflow.h> ) && defined( ADS_HAS_LEMON )
 #include <lemon/preflow.h>
 #include <lemon/smart_graph.h>
 #define ADS_LEMON_FLOW 1
#endif

#if __has_include( <boost/graph/push_relabel_max_flow.hpp> )
 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/push_relabel_max_flow.hpp>
 #define ADS_BOOST_FLOW 1
#endif

using namespace ADS;
using ADS::bench::Options;

namespace {

/// a network: its nodes, the source, the sink and the arcs ( u , v , c )
struct Net {
 int n = 0 , s = 0 , t = 0;
 std::vector< std::tuple< int , int , long long > > A;
 };

/// G( n , 4 n ), oriented, from 0 to n - 1
Net randomNet( int n , unsigned seed )
{
 std::mt19937_64 g( seed );
 Net N;
 N.n = n;
 N.s = 0;
 N.t = n - 1;
 while( int( N.A.size() ) < 4 * n ) {
  const int u = int( g() % std::uint64_t( n ) ) ,
            v = int( g() % std::uint64_t( n ) );
  if( u != v )
   N.A.emplace_back( u , v , 1 + static_cast< long long >( g() % 1000 ) );
  }
 return( N );
 }

/// the grid of about n nodes with the terminals of the cuts of the images
Net gridNet( int n , unsigned seed )
{
 std::mt19937_64 g( seed );
 const int k = std::max( 2 , int( std::sqrt( double( n ) ) ) );
 Net N;
 N.n = k * k + 2;
 N.s = k * k;
 N.t = k * k + 1;
 auto cap = [ & ]( long long hi ) {
  return( static_cast< long long >( g() % std::uint64_t( hi ) ) ); };
 for( int i = 0 ; i < k ; ++i )
  for( int j = 0 ; j < k ; ++j ) {
   const int p = i * k + j;
   if( j + 1 < k ) {
    N.A.emplace_back( p , p + 1 , 1 + cap( 100 ) );
    N.A.emplace_back( p + 1 , p , 1 + cap( 100 ) );
    }
   if( i + 1 < k ) {
    N.A.emplace_back( p , p + k , 1 + cap( 100 ) );
    N.A.emplace_back( p + k , p , 1 + cap( 100 ) );
    }
   N.A.emplace_back( N.s , p , cap( 51 ) );
   N.A.emplace_back( p , N.t , cap( 51 ) );
   }
 return( N );
 }

/// the network of ADS, and the numbers of its arcs
std::pair< FlowNetwork , std::vector< int > > adsNet( const Net & N )
{
 std::pair< FlowNetwork , std::vector< int > > R{ FlowNetwork( N.n ) , {} };
 for( const auto & [ u , v , c ] : N.A )
  R.second.push_back( R.first.addEdge( u , v , c ) );
 return( R );
 }

#ifdef ADS_LEMON_FLOW
/// the network of LEMON
struct LemonNet {
 lemon::SmartDigraph g;
 lemon::SmartDigraph::ArcMap< long long > cap{ g };
 std::vector< lemon::SmartDigraph::Arc > arc;
 lemon::SmartDigraph::Node s , t;

 explicit LemonNet( const Net & N ) {
  std::vector< lemon::SmartDigraph::Node > V;
  for( int i = 0 ; i < N.n ; ++i )
   V.push_back( g.addNode() );
  for( const auto & [ u , v , c ] : N.A ) {
   arc.push_back( g.addArc( V[ u ] , V[ v ] ) );
   cap[ arc.back() ] = c;
   }
  s = V[ N.s ];
  t = V[ N.t ];
  }

 long long run( void ) {
  lemon::Preflow< lemon::SmartDigraph ,
                  lemon::SmartDigraph::ArcMap< long long > > p( g , cap , s ,
                                                                 t );
  p.run();
  return( p.flowValue() );
  }
 };
#endif

#ifdef ADS_BOOST_FLOW
/// the network of Boost, each arc with its reverse of capacity 0
struct BoostNet {
 using Traits = boost::adjacency_list_traits< boost::vecS , boost::vecS ,
                                              boost::directedS >;
 using G = boost::adjacency_list< boost::vecS , boost::vecS ,
           boost::directedS , boost::no_property ,
           boost::property< boost::edge_capacity_t , long long ,
           boost::property< boost::edge_residual_capacity_t , long long ,
           boost::property< boost::edge_reverse_t ,
                            Traits::edge_descriptor > > > >;
 G g;
 std::vector< Traits::edge_descriptor > arc;
 int s , t;

 explicit BoostNet( const Net & N ) : g( N.n ) , s( N.s ) , t( N.t ) {
  auto cap = boost::get( boost::edge_capacity , g );
  auto rev = boost::get( boost::edge_reverse , g );
  for( const auto & [ u , v , c ] : N.A ) {
   const auto e = boost::add_edge( u , v , g ).first;
   const auto r = boost::add_edge( v , u , g ).first;
   cap[ e ] = c;
   cap[ r ] = 0;
   rev[ e ] = r;
   rev[ r ] = e;
   arc.push_back( e );
   }
  }

 long long run( void ) {
  return( boost::push_relabel_max_flow( g , s , t ) );
  }

 void setCapacity( int i , long long c ) {
  boost::get( boost::edge_capacity , g )[ arc[ i ] ] = c;
  }
 };
#endif

/// a cell of one structure: its preparation and its work, ops of them
template< class P , class W >
void cell( const char * bench , const char * c , int n ,
           const std::string & who , int r , std::size_t ops , P && prepare ,
           W && work )
{
 for( int twice = 0 ; twice < 2 ; ++twice ) {
  const std::string w = who + ( twice ? " A/A" : "" );
  if( ADS::bench::skip( w ) )
   continue;
  ADS::bench::print( bench , c , std::size_t( n ) , w ,
                     ADS::bench::median( prepare , work , r , ops ) );
  }
 }

/// the cells of a network
void flows( const char * c , const Net & N , int n , int r , unsigned seed )
{
 const std::size_t m = N.A.size();
 // from scratch
 cell( "flow" , c , n , "ADS push-relabel" , r , m ,
       [ & ]( void ) { return( adsNet( N ) ); } ,
       [ & ]( auto & F ) {
        ADS::bench::sink = std::uint64_t( F.first.maxFlow( N.s , N.t ,
                                          Augment::PUSH_RELABEL ) ); } );
 if( ! ADS::bench::skip( "ADS Dinic" ) )
  ADS::bench::print( "flow" , c , std::size_t( n ) , "ADS Dinic" ,
   ADS::bench::median( [ & ]( void ) { return( adsNet( N ) ); } ,
                       [ & ]( auto & F ) {
    ADS::bench::sink = std::uint64_t( F.first.maxFlow( N.s , N.t ,
                                      Augment::DINIC ) ); } , r , m ) );
#ifdef ADS_LEMON_FLOW
 if( ! ADS::bench::skip( "LEMON Preflow" ) )
  ADS::bench::print( "flow" , c , std::size_t( n ) , "LEMON Preflow" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< LemonNet >( N ) ); } , [ & ]( auto & L ) {
    ADS::bench::sink = std::uint64_t( L->run() ); } , r , m ) );
#endif
#ifdef ADS_BOOST_FLOW
 if( ! ADS::bench::skip( "Boost push-relabel" ) )
  ADS::bench::print( "flow" , c , std::size_t( n ) , "Boost push-relabel" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< BoostNet >( N ) ); } , [ & ]( auto & B ) {
    ADS::bench::sink = std::uint64_t( B->run() ); } , r , m ) );
#endif

 // from the flow there is, after each change
 const std::size_t k = 256;
 std::mt19937_64 g( seed + 7 );
 std::vector< std::pair< int , long long > > C( k );
 for( auto & x : C ) {
  x.first = int( g() % m );
  x.second = static_cast< long long >(
             g() % std::uint64_t( std::get< 2 >( N.A[ x.first ] ) + 50 ) );
  }
 const std::vector< std::pair< int , long long > > C16( C.begin() ,
                                                        C.begin() + 16 );
 ( void ) C16;
 for( Augment how : { Augment::PUSH_RELABEL , Augment::DINIC } ) {
  const std::string who = how == Augment::PUSH_RELABEL ?
                          "ADS warm push-relabel" : "ADS warm Dinic";
  auto work = [ & ]( auto & F ) {
   std::uint64_t h = 0;
   for( const auto & [ i , cap ] : C ) {
    F.first.setCapacity( F.second[ i ] , cap );
    h += std::uint64_t( F.first.maxFlow( N.s , N.t , how ) );
    }
   ADS::bench::sink = h; };
  auto prepare = [ & ]( void ) {
   auto F = adsNet( N );
   F.first.maxFlow( N.s , N.t , how );
   return( F ); };
  if( how == Augment::PUSH_RELABEL )
   cell( "warm" , c , n , who , r , k , prepare , work );
  else if( ! ADS::bench::skip( who ) )
   ADS::bench::print( "warm" , c , std::size_t( n ) , who ,
                      ADS::bench::median( prepare , work , r , k ) );
  }
#ifdef ADS_LEMON_FLOW
 if( ! ADS::bench::skip( "LEMON Preflow" ) )
  ADS::bench::print( "warm" , c , std::size_t( n ) , "LEMON Preflow" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< LemonNet >( N ) ); } , [ & ]( auto & L ) {
    std::uint64_t h = 0;
    for( const auto & [ i , cap ] : C16 ) {
     L->cap[ L->arc[ i ] ] = cap;
     h += std::uint64_t( L->run() );
     }
    ADS::bench::sink = h; } , r , C16.size() ) );
#endif
#ifdef ADS_BOOST_FLOW
 if( ! ADS::bench::skip( "Boost push-relabel" ) )
  ADS::bench::print( "warm" , c , std::size_t( n ) , "Boost push-relabel" ,
   ADS::bench::median( [ & ]( void ) {
    return( std::make_unique< BoostNet >( N ) ); } , [ & ]( auto & B ) {
    std::uint64_t h = 0;
    for( const auto & [ i , cap ] : C16 ) {
     B->setCapacity( i , cap );
     h += std::uint64_t( B->run() );
     }
    ADS::bench::sink = h; } , r , C16.size() ) );
#endif
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 10000 , 100000 } );
 for( std::size_t n0 : o.sizes ) {
  const int n = int( n0 );
  flows( "random" , randomNet( n , o.seed ) , n , o.runs , o.seed );
  flows( "grid" , gridNet( n , o.seed ) , n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File flows.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
