/*--------------------------------------------------------------------------*/
/*----------------------------- File FromBGL.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A graph of Boost.Graph as a graph of ADS, beyond the book, compiled only
 * with `ADS_WITH_BOOST`: `FromBGL< BG , WeightMap >` is a view of a graph
 * of Boost.Graph that is an `IncidenceGraph` and a `VertexListGraph` with a
 * `vertex_index`, which models the concept `Graph` of `Graph.h`, with
 * `forEachEdge()` reading the weights from a property map, so that the
 * algorithms of ADS (the visits, the shortest paths, the spanning trees,
 * the reoptimizing observers) run on it with no copy of the graph. The
 * names of the nodes are the vertex indices.
 *
 * On a `MutableGraph` whose vertex descriptors are the indices
 * (`boost::adjacency_list` on `vecS`) the view is also a `DynamicGraph`:
 * `insertNode()`, `insertEdge()`, `deleteEdge()` and `deleteNode()` call
 * `add_vertex()`, `add_edge()` (and `put()` of the weight), `remove_edge()`
 * and `clear_vertex()`, so that an `ObservedGraph` on the view sends the
 * events of `Observable.h` for the changes made through it. A deleted node
 * keeps its vertex, isolated and marked as not a node, since
 * `remove_vertex()` on `vecS` renumbers the vertices after it, which the
 * stable names of ADS forbid.
 *
 * The contracts [see `Contract.h`] are those of the graphs of ADS [see
 * `Graph.h`]: `insertEdge()` of an edge already there replaces its weight,
 * `deleteEdge()` of one not there does nothing; the graph must change only
 * through the view while the view lives.
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

#ifndef __ADS_interop_boost_FromBGL
 #define __ADS_interop_boost_FromBGL
/* self-identification: #endif constant */

#if defined( ADS_WITH_BOOST )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/graph/Graph.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>

#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::bgl
{

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS FromBGL -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph \f$ g \f$ of Boost.Graph as a graph of ADS, the weights read
/// from the map `WeightMap` (none if `void`)
template< class BG , class WeightMap = void >
class FromBGL
{

 using Traits = boost::graph_traits< BG >;           ///< those of Boost.Graph
 using Vertex = typename Traits::vertex_descriptor;  ///< its vertices
 /// the vertex index of \f$ g \f$
 using IndexMap =
   typename boost::property_map< BG , boost::vertex_index_t >::const_type;
 /// the weight map, or nothing
 using Map = std::conditional_t< std::is_void_v< WeightMap > , std::monostate ,
				 WeightMap >;

 /// whether the vertices are their indices, so that the view may change
 /// the graph
 static constexpr bool indexed = std::is_integral_v< Vertex >;

/*--------------------------------------------------------------------------*/
 public:
 /// the type of the weights, `std::monostate` if none
 using Weight = typename decltype( []() {
  if constexpr( std::is_void_v< WeightMap > )
   return( std::type_identity< std::monostate >() );
  else
   return( std::type_identity<
	    typename boost::property_traits< WeightMap >::value_type >() );
  }() )::type;

 /// the view of \f$ g \f$, unweighted; \f$ g \f$ must stay alive
 explicit FromBGL( BG & g )
  requires std::is_void_v< WeightMap >
     : f_g( &g ) , f_i( get( boost::vertex_index , g ) ) {
  init();
  }

 /// the view of \f$ g \f$ with the weights of \f$ w \f$; \f$ g \f$ must stay
 /// alive
 FromBGL( BG & g , WeightMap w )
  requires( ! std::is_void_v< WeightMap > )
     : f_g( &g ) , f_i( get( boost::vertex_index , g ) ) , f_w( w ) {
  init();
  }

 /// the graph of Boost.Graph
 [[nodiscard]] const BG & graph( void ) const { return( *f_g ); }

 /// whether the graph is oriented, `directed_category` of its traits
 [[nodiscard]] bool directed( void ) const {
  return( boost::is_directed( *f_g ) );
  }

 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the names are \f$ [ 0 , \mathrm{bound}() ) \f$, the vertex indices
 [[nodiscard]] int bound( void ) const { return( int( f_v.size() ) ); }

 /// whether \f$ u \f$ is a node
 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < bound() ) && ! f_dead[ std::size_t( u ) ] );
  }

 /// the nodes
 [[nodiscard]] std::vector< int > V( void ) const {
  std::vector< int > r;
  r.reserve( std::size_t( f_n ) );
  for( int u = 0 ; u < bound() ; ++u )
   if( ! f_dead[ std::size_t( u ) ] )
    r.push_back( u );
  return( r );
  }

 /// \f$ f( v ) \f$ for each node \f$ v \f$ adjacent to \f$ u \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachAdj( int u , F && f ) const {
  ADS_PRE( hasNode( u ) );
  for( auto [ b , e ] = out_edges( f_v[ std::size_t( u ) ] , *f_g ) ; b != e ;
       ++b )
   f( int( get( f_i , target( *b , *f_g ) ) ) );
  }

 /// \f$ f( v , w ) \f$ for each edge \f$ ( u , v ) \f$ of weight \f$ w \f$
 /// \pre \f$ u \f$ is a node
 template< class F >
 void forEachEdge( int u , F && f ) const
  requires( ! std::is_void_v< WeightMap > )
 {
  ADS_PRE( hasNode( u ) );
  for( auto [ b , e ] = out_edges( f_v[ std::size_t( u ) ] , *f_g ) ; b != e ;
       ++b )
   f( int( get( f_i , target( *b , *f_g ) ) ) , Weight( get( f_w , *b ) ) );
  }

 /// the nodes adjacent to \f$ u \f$
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] std::vector< int > adj( int u ) const {
  std::vector< int > r;
  forEachAdj( u , [ & ]( int v ) { r.push_back( v ); } );
  return( r );
  }

 /// the weight of \f$ ( u , v ) \f$, if it is an edge (`std::monostate`
 /// without a weight map), by `edge()` of the graph
 [[nodiscard]] std::optional< Weight > weight( int u , int v ) const {
  if( ! hasNode( u ) || ! hasNode( v ) )
   return( std::nullopt );
  const auto [ e , there ] =
    edge( f_v[ std::size_t( u ) ] , f_v[ std::size_t( v ) ] , *f_g );
  if( ! there )
   return( std::nullopt );
  if constexpr( std::is_void_v< WeightMap > )
   return( Weight() );
  else
   return( Weight( get( f_w , e ) ) );
  }

 /// a new node, `add_vertex()`, and its name
 int insertNode( void )
  requires indexed
 {
  const Vertex x = add_vertex( *f_g );
  f_v.push_back( x );
  f_dead.push_back( false );
  ++f_n;
  return( int( get( f_i , x ) ) );
  }

 /// the edge \f$ ( u , v ) \f$, `add_edge()`, of weight \f$ w \f$ by `put()`;
 /// only the weight changes if it is there, as in the graphs of ADS
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 void insertEdge( int u , int v , const Weight & w = Weight() )
  requires indexed
 {
  ADS_PRE( hasNode( u ) && hasNode( v ) && ( u != v ) );
  const Vertex x = f_v[ std::size_t( u ) ] , y = f_v[ std::size_t( v ) ];
  auto [ e , there ] = edge( x , y , *f_g );
  if( ! there )
   e = add_edge( x , y , *f_g ).first;
  if constexpr( ! std::is_void_v< WeightMap > )
   put( f_w , e , w );
  else
   ( void )w;
  }

 /// the edge \f$ ( u , v ) \f$ removed by `remove_edge()`; nothing if it is
 /// not there
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 void deleteEdge( int u , int v )
  requires indexed
 {
  ADS_PRE( hasNode( u ) && hasNode( v ) );
  remove_edge( f_v[ std::size_t( u ) ] , f_v[ std::size_t( v ) ] , *f_g );
  }

 /// the node \f$ u \f$ removed: its edges by `clear_vertex()`, its vertex
 /// kept and marked, so that no other name changes
 /// \pre \f$ u \f$ is a node
 void deleteNode( int u )
  requires indexed
 {
  ADS_PRE( hasNode( u ) );
  clear_vertex( f_v[ std::size_t( u ) ] , *f_g );
  f_dead[ std::size_t( u ) ] = true;
  --f_n;
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the vertex of each index
 void init( void ) {
  const auto n = std::size_t( num_vertices( *f_g ) );
  f_v.resize( n );
  f_dead.assign( n , false );
  f_n = int( n );
  for( auto [ b , e ] = vertices( *f_g ) ; b != e ; ++b )
   f_v[ std::size_t( get( f_i , *b ) ) ] = *b;
  }

 BG * f_g;                         ///< the graph
 IndexMap f_i;                     ///< its vertex index
 ADS_NO_UNIQUE_ADDRESS Map f_w{};  ///< its weights
 std::vector< Vertex > f_v;        ///< the vertex of each index
 std::vector< bool > f_dead;       ///< the deleted names
 int f_n = 0;                      ///< the nodes

 };  // end( class FromBGL )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::bgl )

#endif /* ADS_WITH_BOOST */

/*--------------------------------------------------------------------------*/

#endif /* FromBGL.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File FromBGL.h ---------------------------*/
/*--------------------------------------------------------------------------*/
