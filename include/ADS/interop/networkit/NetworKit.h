/*--------------------------------------------------------------------------*/
/*---------------------------- File NetworKit.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graphs of ADS as graphs of NetworKit (https://networkit.github.io),
 * and the changes of a graph of ADS driving the dynamic algorithms of
 * NetworKit, beyond the book:
 *
 * - `toNetworKit( g )` copies a graph of ADS into a `NetworKit::Graph`,
 *   oriented and weighted as \f$ g \f$ is (the weights converted to
 *   `edgeweight`, a `double`): a `StaticGraph` or an `ArrayGraph` by
 *   `Graph::fromCSR()`, whose rows are of 64-bit indices and doubles and
 *   thus a copy of those of ADS (an unoriented edge given once, by its arc
 *   \f$ ( u , v ) \f$ with \f$ u < v \f$, since `fromCSR()` stores it from
 *   both ends), any other graph by `addEdge()`, its names that are not
 *   nodes removed after, so that the names of the nodes are the same in
 *   both; in \f$ O( n + m ) \f$ time;
 * - `NetworKitMirror< G >` is an observer of an `ObservedGraph` of ADS [see
 *   `Observable.h`] that keeps a `NetworKit::Graph` in step with the graph
 *   \f$ g \f$ of ADS and tells each change to the dynamic algorithms of
 *   NetworKit attached to it (`DynConnectedComponents`, `DynSSSP`,
 *   `DynBetweenness`, ..., any `NetworKit::DynAlgorithm`), as NetworKit
 *   wants: first the graph of NetworKit changed, then `update( e )` with the
 *   `NetworKit::GraphEvent` of the change, or, for a batch of ADS, all the
 *   changes of its span (already coalesced into their net effect per edge
 *   by ADS) and then one `updateBatch()`.
 *
 * The events map one to one: `NodeInserted` to `NODE_ADDITION` (or
 * `NODE_RESTORATION` of a name that NetworKit had removed), `NodeDeleted`
 * to `NODE_REMOVAL`, `EdgeInserted` to `EDGE_ADDITION`, `EdgeDeleted` to
 * `EDGE_REMOVAL` and `WeightChanged` to `EDGE_WEIGHT_UPDATE`, with the new
 * weight. Each algorithm is attached with the kinds of the events it takes
 * (the masks of `GraphEvents`, the three kinds of the edges by default,
 * since most dynamic algorithms of NetworKit reject the others): the
 * graph of NetworKit sees every change, the algorithm only those of its
 * kinds. A dynamic algorithm of NetworKit is told of the changes by its
 * caller, who must change the graph and call `update()` in the same order;
 * here the observer of ADS does both, for every change that goes through
 * the wrapper.
 *
 * NetworKit has its own graphs and many more algorithms of network
 * analysis, and ADS does not replace them: the copy lets them run on a
 * graph of ADS, and the mirror lets a program that changes its graphs
 * through ADS keep their results up to date. Its cost is that of the
 * change in NetworKit and of the update of the algorithms, plus
 * \f$ O( 1 ) \f$ per event.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs NetworKit 11.2 or later (with `fromCSR()`) and its tlx,
 * found by CMake when `ADS_WITH_NETWORKIT` is AUTO or ON (the target
 * `ADS::NetworKit`). The contracts [see `Contract.h`]: the mirror sees the
 * graph as it is at its construction and then every change through the
 * wrapper; the algorithms attached outlive the mirror or are detached
 * first, and they were run (`run()`) on its graph before the first change;
 * nobody attaches or detaches within a batch.
 *
 * \see C. L. Staudt, A. Sazonovs, H. Meyerhenke, NetworKit: a tool suite
 *      for large-scale complex network analysis, Network Science 4(4),
 *      508-530, 2016, doi:10.1017/nws.2016.20
 * \see E. Angriman, A. van der Grinten, M. Hamann, H. Meyerhenke,
 *      M. Penschuck, Algorithms for large-scale network analysis and the
 *      NetworKit toolkit, Algorithms for Big Data, LNCS 13201, 3-20, 2022,
 *      doi:10.1007/978-3-031-21534-6_1
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_interop_networkit_NetworKit
 #define __ADS_interop_networkit_NetworKit
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <networkit/graph/Graph.hpp> )
 #error "the interoperation with NetworKit needs the headers of NetworKit"
#endif

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Observable.h"

#include <networkit/base/DynAlgorithm.hpp>
#include <networkit/dynamics/GraphEvent.hpp>
#include <networkit/graph/Graph.hpp>

#include <cstddef>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::networkit
{

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE COPY --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the weight \f$ w \f$ of ADS as a weight of NetworKit, the default
/// weight of NetworKit if the graph is unweighted
template< class W >
[[nodiscard]] NetworKit::edgeweight weightOf( const W & w )
{
 if constexpr( std::is_arithmetic_v< W > )
  return( NetworKit::edgeweight( w ) );
 else
  return( NetworKit::defaultEdgeWeight );
 }

/// \f$ g \f$ copied into a graph of NetworKit, oriented and weighted as it
/// is, the names of the nodes kept, in \f$ O( n + m ) \f$ time
template< Graph G >
[[nodiscard]] NetworKit::Graph toNetworKit( const G & g )
{
 using W = typename G::Weight;
 constexpr bool weighted = ! std::is_same_v< W , std::monostate >;
 if constexpr( requires {
		g.offsets();
		g.arcHeads();
	        } ) {
  // the rows of fromCSR(), of 64-bit indices, an unoriented edge once
  const std::size_t n = std::size_t( g.n() );
  std::vector< NetworKit::index > off( n + 1 , 0 ) , col;
  std::vector< double > w;
  col.reserve( std::size_t( g.arcs() ) );
  const auto h = g.arcHeads();
  for( std::size_t u = 0 ; u < n ; ++u ) {
   for( int k = g.offsets()[ u ] ; k < g.offsets()[ u + 1 ] ; ++k )
    if( g.directed() || ( std::size_t( h[ k ] ) > u ) ) {
     col.push_back( NetworKit::index( h[ k ] ) );
     if constexpr( weighted )
      w.push_back( weightOf( g.arcWeights()[ k ] ) );
     }
   off[ u + 1 ] = NetworKit::index( col.size() );
   }
  return( NetworKit::Graph::fromCSR( off , col , w , g.directed() ,
				     weighted ) );
  }
 else {
  const int b = g.bound();
  NetworKit::Graph N( NetworKit::count( b ) , weighted , g.directed() );
  for( int u = 0 ; u < b ; ++u ) {
   if( ! g.hasNode( u ) )
    continue;
   if constexpr( weighted )
    g.forEachEdge( u , [ & ]( int v , const W & x ) {
     if( g.directed() || ( v > u ) )
      N.addEdge( NetworKit::node( u ) , NetworKit::node( v ) , weightOf( x ) );
     } );
   else
    g.forEachAdj( u , [ & ]( int v ) {
     if( g.directed() || ( v > u ) )
      N.addEdge( NetworKit::node( u ) , NetworKit::node( v ) );
     } );
   }
  for( int u = 0 ; u < b ; ++u )
   if( ! g.hasNode( u ) )
    N.removeNode( NetworKit::node( u ) );
  return( N );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS NetworKitMirror --------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph of NetworKit kept in step with a graph of ADS, as an observer of
/// its `ObservedGraph`, which tells each change to the dynamic algorithms
/// of NetworKit attached to it
template< Graph G >
class NetworKitMirror
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = typename G::Weight;   ///< the type of the weights
 using Event = GraphEvent< Weight >;  ///< the events of ADS

 /// the mirror of \f$ g \f$ as it is now, by `toNetworKit()`
 explicit NetworKitMirror( const G & g ) : f_nk( toNetworKit( g ) ) { }

 NetworKitMirror( const NetworKitMirror & ) = delete;
 NetworKitMirror & operator=( const NetworKitMirror & ) = delete;

 /// the graph of NetworKit, on which the algorithms are built
 [[nodiscard]] const NetworKit::Graph & graph( void ) const {
  return( f_nk );
  }

 /// the algorithm \f$ a \f$ attached, told of the events of the kinds
 /// `kinds` [see `GraphEvents`]; its number
 int attach( NetworKit::DynAlgorithm & a ,
	     unsigned kinds = GraphEvents::EDGES ) {
  for( std::size_t q = 0 ; q < f_algo.size() ; ++q )
   if( f_algo[ q ].a == nullptr ) {
    f_algo[ q ] = Slot{ &a , kinds };
    return( int( q ) );
    }
  f_algo.push_back( Slot{ &a , kinds } );
  return( int( f_algo.size() ) - 1 );
  }

 /// the algorithm number \f$ \mathit{id} \f$ detached
 /// \pre it is attached
 void detach( int id ) {
  ADS_PRE( ( id >= 0 ) && ( std::size_t( id ) < f_algo.size() ) &&
	   ( f_algo[ std::size_t( id ) ].a != nullptr ) );
  f_algo[ std::size_t( id ) ].a = nullptr;
  }

 /// the number of the events told to the algorithms, one per algorithm
 [[nodiscard]] std::size_t told( void ) const { return( f_told ); }

 /// the node inserted
 void on( const NodeInserted & e ) { one( Event( e ) ); }
 /// the node deleted, after its edges
 void on( const NodeDeleted & e ) noexcept { one( Event( e ) ); }
 /// the edge inserted
 void on( const EdgeInserted< Weight > & e ) { one( Event( e ) ); }
 /// the edge deleted
 void on( const EdgeDeleted< Weight > & e ) noexcept { one( Event( e ) ); }
 /// the weight of an edge changed
 void on( const WeightChanged< Weight > & e ) { one( Event( e ) ); }

 /// the events of a batch, coalesced: the graph changed by all of them,
 /// then one `updateBatch()` for each algorithm, with those of its kinds
 void on( std::span< const Event > s ) {
  std::vector< NetworKit::GraphEvent > E;
  std::vector< unsigned > K;
  E.reserve( s.size() );
  K.reserve( s.size() );
  for( const Event & e : s ) {
   E.push_back( apply( e ) );
   K.push_back( kindOf< Weight >( e ) );
   }
  std::vector< NetworKit::GraphEvent > F;
  for( const Slot & x : f_algo ) {
   if( x.a == nullptr )
    continue;
   F.clear();
   for( std::size_t q = 0 ; q < E.size() ; ++q )
    if( ( K[ q ] & x.kinds ) != 0 )
     F.push_back( E[ q ] );
   if( ! F.empty() ) {
    x.a->updateBatch( std::span< const NetworKit::GraphEvent >( F ) );
    f_told += F.size();
    }
   }
  }

/*--------------------------------------------------------------------------*/
 private:
 /// an algorithm attached and the kinds of the events it takes
 struct Slot
 {
  NetworKit::DynAlgorithm * a;  ///< the algorithm, null if detached
  unsigned kinds;               ///< the kinds of its events
  };

 /// the event \f$ e \f$ made on the graph of NetworKit, and its event
 NetworKit::GraphEvent apply( const Event & e ) {
  using NE = NetworKit::GraphEvent;
  using NetworKit::node;
  if( const auto * x = std::get_if< NodeInserted >( &e ) ) {
   const node u = node( x->u );
   if( u < f_nk.upperNodeIdBound() ) {
    f_nk.restoreNode( u );
    return( NE( NE::NODE_RESTORATION , u ) );
    }
   while( f_nk.upperNodeIdBound() <= u )
    ( void )f_nk.addNode();
   return( NE( NE::NODE_ADDITION , u ) );
   }
  if( const auto * x = std::get_if< NodeDeleted >( &e ) ) {
   f_nk.removeNode( node( x->u ) );
   return( NE( NE::NODE_REMOVAL , node( x->u ) ) );
   }
  if( const auto * x = std::get_if< EdgeInserted< Weight > >( &e ) ) {
   const NetworKit::edgeweight w = weightOf( x->w );
   f_nk.addEdge( node( x->u ) , node( x->v ) , w );
   return( NE( NE::EDGE_ADDITION , node( x->u ) , node( x->v ) , w ) );
   }
  if( const auto * x = std::get_if< EdgeDeleted< Weight > >( &e ) ) {
   f_nk.removeEdge( node( x->u ) , node( x->v ) );
   return( NE( NE::EDGE_REMOVAL , node( x->u ) , node( x->v ) ) );
   }
  const auto & x = std::get< WeightChanged< Weight > >( e );
  const NetworKit::edgeweight w = weightOf( x.to );
  f_nk.setWeight( node( x.u ) , node( x.v ) , w );
  return( NE( NE::EDGE_WEIGHT_UPDATE , node( x.u ) , node( x.v ) , w ) );
  }

 /// the event \f$ e \f$ made, and told to the algorithms of its kind
 void one( const Event & e ) {
  const NetworKit::GraphEvent x = apply( e );
  const unsigned k = kindOf< Weight >( e );
  for( const Slot & s : f_algo )
   if( ( s.a != nullptr ) && ( ( s.kinds & k ) != 0 ) ) {
    s.a->update( x );
    ++f_told;
    }
  }

 NetworKit::Graph f_nk;       ///< the graph of NetworKit
 std::vector< Slot > f_algo;  ///< the algorithms attached
 std::size_t f_told = 0;      ///< the events told

 };  // end( class NetworKitMirror )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::networkit )

/*--------------------------------------------------------------------------*/

#endif /* NetworKit.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File NetworKit.h --------------------------*/
/*--------------------------------------------------------------------------*/
