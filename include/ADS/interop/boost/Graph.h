/*--------------------------------------------------------------------------*/
/*------------------------------ File Graph.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graphs of ADS as graphs of Boost.Graph, beyond the book, compiled only
 * with `ADS_WITH_BOOST` [see `cmake/ADSBoost.cmake`]: the specializations of
 * `boost::graph_traits` and `boost::property_map`, and the free functions
 * that the concepts of Boost.Graph ask for, so that its algorithms
 * (`breadth_first_search()`, `dijkstra_shortest_paths()`,
 * `bellman_ford_shortest_paths()`, `strong_components()`,
 * `topological_sort()`, the maximum flows) run on an ADS graph with no copy.
 *
 * `StaticGraph` and `ArrayGraph`, the two graphs in compressed rows, are an
 * `IncidenceGraph`, an `AdjacencyGraph`, a `VertexListGraph` and an
 * `EdgeListGraph`: an edge is the pair ( tail , place ), the place of the
 * arc in the array of the rows, which is also its `edge_index`, stable since
 * the graph is static; the out-arcs of \f$ u \f$ are the places of its row,
 * the weight of an arc is read where it lies (`edge_weight`), and the
 * vertex index is the name of the node. `ListGraph` and `ObservedGraph` on
 * a `ListGraph` are an `IncidenceGraph`, an `AdjacencyGraph` and a
 * `VertexListGraph` with the edge ( tail , head ), their weight looked up
 * in \f$ O( \deg u ) \f$, and a `MutableGraph`: `add_edge()`,
 * `remove_edge()`, `add_vertex()`, `remove_vertex()` and `clear_vertex()`
 * are the operations of the graph, so that a program of Boost.Graph that
 * edits an `ObservedGraph` sends the events of `Observable.h` to its
 * observers. Since the names of the nodes of ADS are stable, a removed
 * vertex keeps the names of the others (unlike the vertices of a
 * `boost::adjacency_list` on `vecS`), and `num_vertices()` is `bound()`, the
 * range of the names, so that the maps of Boost.Graph indexed by
 * `vertex_index` have a place for every name.
 *
 * Every ADS graph is seen as oriented (`directed_tag`): the orientation of
 * an ADS graph is a flag at run time, and an unoriented graph has both arcs
 * of each edge, which the oriented algorithms of Boost.Graph read
 * correctly; the algorithms that ask for `undirected_tag` do not take it.
 * The free functions are in the namespace of the graphs, where
 * argument-dependent lookup finds them whatever the order of the includes,
 * and only the traits are in `namespace boost`; a program must not
 * specialize them again for the same graphs, which would break the one
 * definition rule.
 *
 * \see J. G. Siek, L.-Q. Lee, A. Lumsdaine, The Boost Graph Library: User
 *      Guide and Reference Manual, Addison-Wesley, 2002
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

#ifndef __ADS_interop_boost_Graph
 #define __ADS_interop_boost_Graph
/* self-identification: #endif constant */

#if defined( ADS_WITH_BOOST )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/StaticGraph.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_map/property_map.hpp>

#include <cstddef>
#include <ranges>
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
/*----------------------- THE GRAPHS IN COMPRESSED ROWS --------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
/// the rows of a graph in compressed rows: the offsets, the head and the
/// weight of the arc of a place, for `StaticGraph` and `ArrayGraph`
template< class G >
struct Rows;

/// the rows of a `StaticGraph`, its arrays `offsets()` and `arcData()`
template< class W >
struct Rows< StaticGraph< W > >
{
 using Weight = W;  ///< the type of the weights
 static constexpr bool weighted = StaticGraph< W >::weighted;
 /// the offsets of the rows
 static const int * offsets( const StaticGraph< W > & g ) {
  return( g.offsets() );
  }
 /// the head of the arc of the place \f$ k \f$
 static int head( const StaticGraph< W > & g , int k ) {
  return( StaticGraph< W >::head( g.arcData()[ k ] ) );
  }
 /// the weight of the arc of the place \f$ k \f$
 static const W & weight( const StaticGraph< W > & g , int k )
  requires weighted
 {
  return( g.arcData()[ k ].w );
  }
 };

/// the rows of an `ArrayGraph`, its arrays `offsets()`, `heads()` and
/// `weightData()`
template< class W >
struct Rows< ArrayGraph< W > >
{
 using Weight = W;  ///< the type of the weights
 static constexpr bool weighted = ! std::is_same_v< W , std::monostate >;
 /// the offsets of the rows
 static const int * offsets( const ArrayGraph< W > & g ) {
  return( g.offsets() );
  }
 /// the head of the arc of the place \f$ k \f$
 static int head( const ArrayGraph< W > & g , int k ) {
  return( g.heads()[ k ] );
  }
 /// the weight of the arc of the place \f$ k \f$
 static const W & weight( const ArrayGraph< W > & g , int k )
  requires weighted
 {
  return( g.weightData()[ k ] );
  }
 };

/// \endcond

/// a graph in compressed rows that the adapter knows
template< class G >
concept RowGraph = requires { Rows< G >::weighted; };

/// an arc of a graph in compressed rows: its tail and its place in the
/// rows, which is also its edge index
struct Arc
{
 int src = -1;  ///< the tail
 int pos = -1;  ///< the place
 friend bool operator==( Arc , Arc ) = default;
 };

/// \cond
/// the out-arcs of \f$ u \f$, the places of its row, random access
class OutArcIterator
    : public boost::iterator_facade< OutArcIterator , Arc ,
				     boost::random_access_traversal_tag , Arc >
{
 public:
 /// an iterator on no row
 OutArcIterator( void ) = default;
 /// on the place \f$ k \f$ of the row of \f$ u \f$
 OutArcIterator( int u , int k ) : f_u( u ) , f_k( k ) { }

 private:
 friend class boost::iterator_core_access;
 Arc dereference( void ) const { return( Arc{ f_u , f_k } ); }
 bool equal( const OutArcIterator & o ) const { return( f_k == o.f_k ); }
 void increment( void ) { ++f_k; }
 void decrement( void ) { --f_k; }
 void advance( std::ptrdiff_t d ) { f_k += int( d ); }
 std::ptrdiff_t distance_to( const OutArcIterator & o ) const {
  return( o.f_k - f_k );
  }

 int f_u = -1;  ///< the tail
 int f_k = -1;  ///< the place
 };

/// all the arcs, row by row: the tail advances when the place reaches the
/// end of its row, \f$ O( 1 ) \f$ amortized per step
template< class G >
class AllArcIterator
    : public boost::iterator_facade< AllArcIterator< G > , Arc ,
				     boost::forward_traversal_tag , Arc >
{
 public:
 /// an iterator on no graph
 AllArcIterator( void ) = default;
 /// on the place \f$ k \f$ of \f$ g \f$
 AllArcIterator( const G & g , int k ) : f_g( &g ) , f_k( k ) { skip(); }

 private:
 friend class boost::iterator_core_access;
 /// the tail of the place, past the empty rows
 void skip( void ) {
  const int * off = Rows< G >::offsets( *f_g );
  while( ( f_u < f_g->n() ) && ( off[ f_u + 1 ] <= f_k ) )
   ++f_u;
  }
 Arc dereference( void ) const { return( Arc{ f_u , f_k } ); }
 bool equal( const AllArcIterator & o ) const { return( f_k == o.f_k ); }
 void increment( void ) {
  ++f_k;
  skip();
  }

 const G * f_g = nullptr;  ///< the graph
 int f_k = 0;              ///< the place
 int f_u = 0;              ///< its tail
 };

/// the heads of the out-arcs of \f$ u \f$, random access
template< class G >
class HeadIterator
    : public boost::iterator_facade< HeadIterator< G > , int ,
				     boost::random_access_traversal_tag , int >
{
 public:
 /// an iterator on no graph
 HeadIterator( void ) = default;
 /// on the place \f$ k \f$ of \f$ g \f$
 HeadIterator( const G & g , int k ) : f_g( &g ) , f_k( k ) { }

 private:
 friend class boost::iterator_core_access;
 int dereference( void ) const { return( Rows< G >::head( *f_g , f_k ) ); }
 bool equal( const HeadIterator & o ) const { return( f_k == o.f_k ); }
 void increment( void ) { ++f_k; }
 void decrement( void ) { --f_k; }
 void advance( std::ptrdiff_t d ) { f_k += int( d ); }
 std::ptrdiff_t distance_to( const HeadIterator & o ) const {
  return( o.f_k - f_k );
  }

 const G * f_g = nullptr;  ///< the graph
 int f_k = -1;             ///< the place
 };

/// \endcond

/// the weight of an arc, read in place: a readable property map
template< class G >
struct RowWeightMap
{
 using key_type = Arc;                               ///< the arcs
 using value_type = typename Rows< G >::Weight;      ///< their weights
 using reference = const value_type &;               ///< read in place
 using category = boost::readable_property_map_tag;  ///< read only
 const G * g = nullptr;                              ///< the graph
 /// the weight of \f$ a \f$
 friend reference get( const RowWeightMap & m , Arc a ) {
  return( Rows< G >::weight( *m.g , a.pos ) );
  }
 };

/// the edge index of an arc, its place in the rows
struct ArcIndexMap
{
 using key_type = Arc;                               ///< the arcs
 using value_type = int;                             ///< their places
 using reference = int;                              ///< by value
 using category = boost::readable_property_map_tag;  ///< read only
 /// the place of \f$ a \f$
 friend int get( ArcIndexMap , Arc a ) { return( a.pos ); }
 };

/// \cond
/// the traversal of the graphs in compressed rows
struct row_traversal : virtual boost::incidence_graph_tag ,
		       virtual boost::adjacency_graph_tag ,
		       virtual boost::vertex_list_graph_tag ,
		       virtual boost::edge_list_graph_tag
{ };

/// the traits of Boost.Graph of a graph in compressed rows
template< class G >
struct RowTraits
{
 using vertex_descriptor = int;                  ///< the names of the nodes
 using edge_descriptor = Arc;                    ///< ( tail , place )
 using directed_category = boost::directed_tag;  ///< see above
 using edge_parallel_category = boost::allow_parallel_edge_tag;
 using traversal_category = row_traversal;  ///< the concepts modeled
 using vertex_iterator = boost::counting_iterator< int >;  ///< the names
 using out_edge_iterator = OutArcIterator;                 ///< a row
 using adjacency_iterator = HeadIterator< G >;  ///< the heads of a row
 using edge_iterator = AllArcIterator< G >;     ///< the rows
 using in_edge_iterator = void;                 ///< no in-arcs
 using vertices_size_type = int;                ///< the number of nodes
 using edges_size_type = int;                   ///< of arcs
 using degree_size_type = int;                  ///< of out-arcs
 /// no node
 static vertex_descriptor null_vertex( void ) { return( -1 ); }
 };
/// \endcond

/*--------------------------------------------------------------------------*/
/*------------------------- THE GRAPHS OF LISTS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
/// a graph of adjacency lists that the adapter knows, and the `ListGraph`
/// it reads: the graph itself, or the one an `ObservedGraph` wraps
template< class G >
struct Lists
{ };

/// a `ListGraph`, read and changed directly
template< class W , class T >
struct Lists< ListGraph< W , T > >
{
 using Weight = W;  ///< the type of the weights
 /// the lists read
 static const ListGraph< W , T > & lists( const ListGraph< W , T > & g ) {
  return( g );
  }
 };

/// an `ObservedGraph` on a `ListGraph`, read through the wrapper and
/// changed by it, so that each change is an event for its observers
template< class W , class T , class P >
struct Lists< ObservedGraph< ListGraph< W , T > , P > >
{
 using Weight = W;  ///< the type of the weights
 /// the lists read
 static const ListGraph< W , T > &
 lists( const ObservedGraph< ListGraph< W , T > , P > & g ) {
  return( g.graph() );
  }
 };

/// \endcond

/// a graph of adjacency lists that the adapter knows
template< class G >
concept ListedGraph = requires { typename Lists< G >::Weight; };

/// an edge of a graph of lists: its tail and its head
struct Edge
{
 int src = -1;  ///< the tail
 int dst = -1;  ///< the head
 friend bool operator==( Edge , Edge ) = default;
 };

/// \cond
/// the nodes, past the deleted names, forward
template< class G >
class NodeIterator
    : public boost::iterator_facade< NodeIterator< G > , int ,
				     boost::forward_traversal_tag , int >
{
 public:
 /// an iterator on no graph
 NodeIterator( void ) = default;
 /// on the first node from the name \f$ u \f$ of \f$ g \f$
 NodeIterator( const G & g , int u ) : f_g( &g ) , f_u( u ) { skip(); }

 private:
 friend class boost::iterator_core_access;
 void skip( void ) {
  while( ( f_u < f_g->bound() ) && ! f_g->hasNode( f_u ) )
   ++f_u;
  }
 int dereference( void ) const { return( f_u ); }
 bool equal( const NodeIterator & o ) const { return( f_u == o.f_u ); }
 void increment( void ) {
  ++f_u;
  skip();
  }

 const G * f_g = nullptr;  ///< the graph
 int f_u = 0;              ///< the name
 };

/// the out-edges of \f$ u \f$, on the iterator `I` of its list
template< class I >
class OutEdgeIterator
    : public boost::iterator_facade< OutEdgeIterator< I > , Edge ,
				     boost::forward_traversal_tag , Edge >
{
 public:
 /// an iterator on no list
 OutEdgeIterator( void ) = default;
 /// the edge from \f$ u \f$ to the node of \f$ i \f$
 OutEdgeIterator( int u , I i ) : f_u( u ) , f_i( i ) { }

 private:
 friend class boost::iterator_core_access;
 Edge dereference( void ) const { return( Edge{ f_u , int( *f_i ) } ); }
 bool equal( const OutEdgeIterator & o ) const { return( f_i == o.f_i ); }
 void increment( void ) { ++f_i; }

 int f_u = -1;  ///< the tail
 I f_i{};       ///< the place in the list
 };

/// the heads of the out-edges of \f$ u \f$, on the iterator `I` of its list
template< class I >
class AdjacentIterator
    : public boost::iterator_facade< AdjacentIterator< I > , int ,
				     boost::forward_traversal_tag , int >
{
 public:
 /// an iterator on no list
 AdjacentIterator( void ) = default;
 /// on the node of \f$ i \f$
 explicit AdjacentIterator( I i ) : f_i( i ) { }

 private:
 friend class boost::iterator_core_access;
 int dereference( void ) const { return( int( *f_i ) ); }
 bool equal( const AdjacentIterator & o ) const { return( f_i == o.f_i ); }
 void increment( void ) { ++f_i; }

 I f_i{};  ///< the place in the list
 };

/// \endcond

/// the weight of an edge of a graph of lists, looked up in
/// \f$ O( \deg u ) \f$: a readable property map
template< class G >
struct ListWeightMap
{
 using key_type = Edge;                              ///< the edges
 using value_type = typename Lists< G >::Weight;     ///< their weights
 using reference = value_type;                       ///< by value
 using category = boost::readable_property_map_tag;  ///< read only
 const G * g = nullptr;                              ///< the graph
 /// the weight of \f$ e \f$
 /// \pre \f$ e \f$ is an edge of the graph
 friend value_type get( const ListWeightMap & m , Edge e ) {
  return( *Lists< G >::lists( *m.g ).weight( e.src , e.dst ) );
  }
 };

/// \cond
/// the traversal of the graphs of lists
struct list_traversal : virtual boost::incidence_graph_tag ,
			virtual boost::adjacency_graph_tag ,
			virtual boost::vertex_list_graph_tag
{ };

/// the traits of Boost.Graph of a graph of lists
template< class G >
struct ListTraits
{
 /// the iterator of a list of the graph
 using List = std::ranges::iterator_t<
   decltype( std::declval< const decltype( Lists< G >::lists(
	       std::declval< const G & >() ) ) & >()
	       .neighbors( 0 ) ) >;
 using vertex_descriptor = int;                  ///< the names of the nodes
 using edge_descriptor = Edge;                   ///< ( tail , head )
 using directed_category = boost::directed_tag;  ///< see above
 using edge_parallel_category = boost::disallow_parallel_edge_tag;
 using traversal_category = list_traversal;          ///< the concepts modeled
 using vertex_iterator = NodeIterator< G >;          ///< the nodes
 using out_edge_iterator = OutEdgeIterator< List >;  ///< a list
 using adjacency_iterator = AdjacentIterator< List >;  ///< its heads
 using edge_iterator = void;      ///< no list of the edges
 using in_edge_iterator = void;   ///< no in-edges
 using vertices_size_type = int;  ///< the names
 using edges_size_type = int;     ///< the edges
 using degree_size_type = int;    ///< the out-edges
 /// no node
 static vertex_descriptor null_vertex( void ) { return( -1 ); }
 };
/// \endcond

}  // end( namespace ADS::bgl )

/*--------------------------------------------------------------------------*/
/*------------------------ THE TRAITS OF BOOST.GRAPH -----------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace boost
{

template< class W >
struct graph_traits< ADS::StaticGraph< W > >
    : ADS::bgl::RowTraits< ADS::StaticGraph< W > >
{ };

template< class W >
struct graph_traits< ADS::ArrayGraph< W > >
    : ADS::bgl::RowTraits< ADS::ArrayGraph< W > >
{ };

template< class W , class T >
struct graph_traits< ADS::ListGraph< W , T > >
    : ADS::bgl::ListTraits< ADS::ListGraph< W , T > >
{ };

template< class W , class T , class P >
struct graph_traits< ADS::ObservedGraph< ADS::ListGraph< W , T > , P > >
    : ADS::bgl::ListTraits< ADS::ObservedGraph< ADS::ListGraph< W , T > , P > >
{ };

template< class G >
 requires( ADS::bgl::RowGraph< G > || ADS::bgl::ListedGraph< G > )
struct graph_traits< const G > : graph_traits< G >
{ };

template< class G >
 requires ADS::bgl::RowGraph< std::remove_const_t< G > >
struct property_map< G , edge_weight_t >
{
 using type = ADS::bgl::RowWeightMap< std::remove_const_t< G > >;
 using const_type = type;
 };

template< class G >
 requires ADS::bgl::ListedGraph< std::remove_const_t< G > >
struct property_map< G , edge_weight_t >
{
 using type = ADS::bgl::ListWeightMap< std::remove_const_t< G > >;
 using const_type = type;
 };

template< class G >
 requires( ADS::bgl::RowGraph< std::remove_const_t< G > > ||
	   ADS::bgl::ListedGraph< std::remove_const_t< G > > )
struct property_map< G , vertex_index_t >
{
 using type = typed_identity_property_map< int >;
 using const_type = type;
 };

template< class G >
 requires ADS::bgl::RowGraph< std::remove_const_t< G > >
struct property_map< G , edge_index_t >
{
 using type = ADS::bgl::ArcIndexMap;
 using const_type = type;
 };

}  // end( namespace boost )
/// \endcond

/*--------------------------------------------------------------------------*/
/*---------------------- THE FUNCTIONS OF THE CONCEPTS ---------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// \cond
// the graphs in compressed rows

template< bgl::RowGraph G >
std::pair< boost::counting_iterator< int > , boost::counting_iterator< int > >
vertices( const G & g )
{
 return( std::pair( boost::counting_iterator< int >( 0 ) ,
		     boost::counting_iterator< int >( g.n() ) ) );
 }

template< bgl::RowGraph G >
int num_vertices( const G & g )
{
 return( g.n() );
 }

template< bgl::RowGraph G >
int num_edges( const G & g )
{
 return( bgl::Rows< G >::offsets( g )[ g.n() ] );
 }

template< bgl::RowGraph G >
std::pair< bgl::OutArcIterator , bgl::OutArcIterator >
out_edges( int u , const G & g )
{
 const int * off = bgl::Rows< G >::offsets( g );
 return( std::pair( bgl::OutArcIterator( u , off[ u ] ) ,
		     bgl::OutArcIterator( u , off[ u + 1 ] ) ) );
 }

template< bgl::RowGraph G >
int out_degree( int u , const G & g )
{
 const int * off = bgl::Rows< G >::offsets( g );
 return( off[ u + 1 ] - off[ u ] );
 }

template< bgl::RowGraph G >
int source( bgl::Arc a , const G & )
{
 return( a.src );
 }

template< bgl::RowGraph G >
int target( bgl::Arc a , const G & g )
{
 return( bgl::Rows< G >::head( g , a.pos ) );
 }

template< bgl::RowGraph G >
std::pair< bgl::HeadIterator< G > , bgl::HeadIterator< G > >
adjacent_vertices( int u , const G & g )
{
 const int * off = bgl::Rows< G >::offsets( g );
 return( std::pair( bgl::HeadIterator< G >( g , off[ u ] ) ,
		     bgl::HeadIterator< G >( g , off[ u + 1 ] ) ) );
 }

// edges() for each graph, more specialized than edges( g ) of
// MinimumSpanningTree.h, which is ADS::edges< G >( g ) then
template< class W >
std::pair< bgl::AllArcIterator< StaticGraph< W > > ,
	   bgl::AllArcIterator< StaticGraph< W > > >
edges( const StaticGraph< W > & g )
{
 using It = bgl::AllArcIterator< StaticGraph< W > >;
 return( std::pair( It( g , 0 ) , It( g , num_edges( g ) ) ) );
 }

template< class W >
std::pair< bgl::AllArcIterator< ArrayGraph< W > > ,
	   bgl::AllArcIterator< ArrayGraph< W > > >
edges( const ArrayGraph< W > & g )
{
 using It = bgl::AllArcIterator< ArrayGraph< W > >;
 return( std::pair( It( g , 0 ) , It( g , num_edges( g ) ) ) );
 }

template< bgl::RowGraph G >
 requires bgl::Rows< G >::weighted
bgl::RowWeightMap< G > get( boost::edge_weight_t , const G & g )
{
 return( bgl::RowWeightMap< G >{ &g } );
 }

template< bgl::RowGraph G >
 requires bgl::Rows< G >::weighted
const typename bgl::Rows< G >::Weight & get( boost::edge_weight_t ,
					     const G & g ,
					     bgl::Arc a )
{
 return( bgl::Rows< G >::weight( g , a.pos ) );
 }

template< bgl::RowGraph G >
bgl::ArcIndexMap get( boost::edge_index_t , const G & )
{
 return( bgl::ArcIndexMap() );
 }

template< bgl::RowGraph G >
int get( boost::edge_index_t , const G & , bgl::Arc a )
{
 return( a.pos );
 }

// the graphs of lists

template< bgl::ListedGraph G >
std::pair< bgl::NodeIterator< G > , bgl::NodeIterator< G > >
vertices( const G & g )
{
 return( std::pair( bgl::NodeIterator< G >( g , 0 ) ,
		     bgl::NodeIterator< G >( g , g.bound() ) ) );
 }

template< bgl::ListedGraph G >
int num_vertices( const G & g )
{
 return( g.bound() );
 }

template< bgl::ListedGraph G >
auto out_edges( int u , const G & g )
{
 using It = typename boost::graph_traits< G >::out_edge_iterator;
 const auto r = bgl::Lists< G >::lists( g ).neighbors( u );
 return( std::pair< It , It >( It( u , r.begin() ) , It( u , r.end() ) ) );
 }

template< bgl::ListedGraph G >
int out_degree( int u , const G & g )
{
 const auto r = bgl::Lists< G >::lists( g ).neighbors( u );
 return( int( std::ranges::distance( r ) ) );
 }

template< bgl::ListedGraph G >
int source( bgl::Edge e , const G & )
{
 return( e.src );
 }

template< bgl::ListedGraph G >
int target( bgl::Edge e , const G & )
{
 return( e.dst );
 }

template< bgl::ListedGraph G >
auto adjacent_vertices( int u , const G & g )
{
 using It = typename boost::graph_traits< G >::adjacency_iterator;
 const auto r = bgl::Lists< G >::lists( g ).neighbors( u );
 return( std::pair< It , It >( It( r.begin() ) , It( r.end() ) ) );
 }

template< bgl::ListedGraph G >
std::pair< bgl::Edge , bool > edge( int u , int v , const G & g )
{
 return( std::pair( bgl::Edge{ u , v } ,
		     bgl::Lists< G >::lists( g ).hasEdge( u , v ) ) );
 }

template< bgl::ListedGraph G >
std::pair< bgl::Edge , bool > add_edge( int u , int v , G & g )
{
 if( bgl::Lists< G >::lists( g ).hasEdge( u , v ) )
  return( std::pair( bgl::Edge{ u , v } , false ) );
 g.insertEdge( u , v );
 return( std::pair( bgl::Edge{ u , v } , true ) );
 }

template< bgl::ListedGraph G >
std::pair< bgl::Edge , bool >
add_edge( int u , int v , const typename bgl::Lists< G >::Weight & w , G & g )
{
 if( bgl::Lists< G >::lists( g ).hasEdge( u , v ) )
  return( std::pair( bgl::Edge{ u , v } , false ) );
 g.insertEdge( u , v , w );
 return( std::pair( bgl::Edge{ u , v } , true ) );
 }

template< bgl::ListedGraph G >
void remove_edge( int u , int v , G & g )
{
 if( bgl::Lists< G >::lists( g ).hasEdge( u , v ) )
  g.deleteEdge( u , v );
 }

template< bgl::ListedGraph G >
void remove_edge( bgl::Edge e , G & g )
{
 remove_edge( e.src , e.dst , g );
 }

template< bgl::ListedGraph G >
int add_vertex( G & g )
{
 return( g.insertNode() );
 }

template< bgl::ListedGraph G >
void clear_vertex( int u , G & g )
{
 const auto & L = bgl::Lists< G >::lists( g );
 for( int v : L.adj( u ) )
  if( L.hasEdge( u , v ) )  // gone with [ v , u ] if unoriented
   g.deleteEdge( u , v );
 if( L.directed() )
  for( int v : L.inAdj( u ) )
   g.deleteEdge( v , u );
 }

template< bgl::ListedGraph G >
void remove_vertex( int u , G & g )
{
 g.deleteNode( u );
 }

template< bgl::ListedGraph G >
bgl::ListWeightMap< G > get( boost::edge_weight_t , const G & g )
{
 return( bgl::ListWeightMap< G >{ &g } );
 }

template< bgl::ListedGraph G >
typename bgl::Lists< G >::Weight get( boost::edge_weight_t , const G & g ,
				      bgl::Edge e )
{
 return( *bgl::Lists< G >::lists( g ).weight( e.src , e.dst ) );
 }

// both

template< class G >
 requires( bgl::RowGraph< G > || bgl::ListedGraph< G > )
boost::typed_identity_property_map< int > get( boost::vertex_index_t ,
					       const G & )
{
 return( boost::typed_identity_property_map< int >() );
 }

template< class G >
 requires( bgl::RowGraph< G > || bgl::ListedGraph< G > )
int get( boost::vertex_index_t , const G & , int u )
{
 return( u );
 }
/// \endcond

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_BOOST */

/*--------------------------------------------------------------------------*/

#endif /* Graph.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File Graph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
