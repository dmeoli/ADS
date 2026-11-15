/*--------------------------------------------------------------------------*/
/*------------------------ File engineered_interop.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the interoperation of ADS with Boost and with the standard
 * library, beyond the book: the algorithms of Boost.Graph on the graphs of
 * ADS [see `interop/boost/Graph.h`], against the same algorithms on
 * `compressed_sparse_row_graph` built from the same edges and against the
 * algorithms of ADS on the same graph, three answers that must agree; the
 * algorithms of ADS on the graphs of Boost.Graph [see `FromBGL.h`], the
 * changes made by `add_edge()` and `remove_edge()` heard by the observers of
 * ADS, the queues of ADS in the visit of Boost.Graph [see `Dijkstra.h`] and
 * the heaps of Boost.Heap in the shortest paths and in Prim's algorithm of
 * ADS [see `Heap.h`], the traversal of the iterators that build their item
 * [see `Iterator.h`]; and, with the standard library alone, `std::flat_set`
 * and `std::flat_map` on the sequences of ADS against `std::set` and
 * `std::map`, the memory resource of the pools, the sorts on ranges other
 * than a vector against the same sorts on a vector, the queues of the
 * standard library in the frontiers of ADS, and the conversions between the
 * numbers of ADS and those of GMP. The tests of Boost check nothing without
 * `ADS_WITH_BOOST`, and that of GMP nothing without `ADS_WITH_GMP`; built
 * with and without them, the program covers both, at the level 3 of the
 * contracts.
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

#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/interop/std/FlatMap.h"
#include "ADS/interop/std/Pmr.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/Rational.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/sequence/ContiguousDeque.h"
#include "ADS/sequence/CubishArrayStack.h"
#include "ADS/sequence/DualArrayDeque.h"
#include "ADS/sequence/RootishArrayStack.h"
#include "ADS/sequence/TieredVector.h"
#include "ADS/sequence/Treque.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/ExchangeSorts.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/tree/BTreeMap.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory_resource>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <version>

#if defined( ADS_WITH_BOOST )
 // the traversal first, since Boost reads it when its headers are compiled
 #include "ADS/interop/boost/Iterator.h"

 #include "ADS/interop/boost/Dijkstra.h"
 #include "ADS/interop/boost/FromBGL.h"
 #include "ADS/interop/boost/Graph.h"
 #include "ADS/interop/boost/Heap.h"

 #include <boost/concept/assert.hpp>
 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/bellman_ford_shortest_paths.hpp>
 #include <boost/graph/boykov_kolmogorov_max_flow.hpp>
 #include <boost/graph/breadth_first_search.hpp>
 #include <boost/graph/compressed_sparse_row_graph.hpp>
 #include <boost/graph/depth_first_search.hpp>
 #include <boost/graph/dijkstra_shortest_paths.hpp>
 #include <boost/graph/edmonds_karp_max_flow.hpp>
 #include <boost/graph/graph_concepts.hpp>
 #include <boost/graph/push_relabel_max_flow.hpp>
 #include <boost/graph/strong_components.hpp>
 #include <boost/graph/topological_sort.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/fibonacci_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/range/algorithm/count_if.hpp>
 #include <boost/range/algorithm/max_element.hpp>
#endif

#if defined( ADS_WITH_GMP ) && __has_include( <gmpxx.h> )
 #include <gmpxx.h>
 #define ADS_TEST_GMPXX 1
#else
 #define ADS_TEST_GMPXX 0
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the arcs ( u , v , w ) of a random graph of n nodes, with no loop and no
/// parallel arc, w in [ 1 , 20 ]; only u < v if dag
std::vector< std::tuple< int , int , double > >
randomArcs( int n , int m , unsigned seed , bool dag = false )
{
 std::mt19937 r( seed );
 std::set< std::pair< int , int > > seen;
 std::vector< std::tuple< int , int , double > > E;
 while( int( E.size() ) < m ) {
  int u = int( r() % unsigned( n ) ) , v = int( r() % unsigned( n ) );
  if( u == v )
   continue;
  if( dag && ( u > v ) )
   std::swap( u , v );
  if( ! seen.insert( { u , v } ).second )
   continue;
  E.emplace_back( u , v , double( 1 + r() % 20 ) );
  }
 return( E );
 }

/// a static observer that counts the insertions and the deletions of edges
struct EdgeCount
{
 int ins = 0;  ///< the insertions
 int del = 0;  ///< the deletions
 void on( const EdgeInserted< double > & ) { ++ins; }
 void on( const EdgeDeleted< double > & ) { ++del; }
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------------ BOOST.GRAPH -------------------------------*/
/*--------------------------------------------------------------------------*/

#if defined( ADS_WITH_BOOST )
namespace
{

/// the distances of ADS with the unreached nodes at the largest number, as
/// Boost.Graph leaves them
std::vector< double > bglDistances( const std::vector< double > & d )
{
 std::vector< double > r( d );
 for( double & x : r )
  if( x == infinity< double >() )
   x = std::numeric_limits< double >::max();
 return( r );
 }

/// whether two labelings of the nodes make the same partition
bool samePartition( const std::vector< int > & a ,
		    const std::vector< int > & b )
{
 if( a.size() != b.size() )
  return( false );
 std::map< int , int > ab , ba;
 for( std::size_t i = 0 ; i < a.size() ; ++i ) {
  if( ab.emplace( a[ i ] , b[ i ] ).first->second != b[ i ] )
   return( false );
  if( ba.emplace( b[ i ] , a[ i ] ).first->second != a[ i ] )
   return( false );
  }
 return( true );
 }

using SG = StaticGraph< double >;
using AG = ArrayGraph< double >;
using LG = ListGraph< double >;
using OLG = ObservedGraph< LG , Static< EdgeCount > >;
/// the graph in compressed rows of Boost.Graph, its weights bundled
struct EW
{
 double w;
 };
using CSR = boost::compressed_sparse_row_graph< boost::directedS ,
						boost::no_property , EW >;
using AL =
  boost::adjacency_list< boost::vecS , boost::vecS , boost::directedS ,
			 boost::no_property ,
			 boost::property< boost::edge_weight_t , double > >;

BOOST_CONCEPT_ASSERT( ( boost::IncidenceGraphConcept< SG > ));
BOOST_CONCEPT_ASSERT( ( boost::VertexListGraphConcept< SG > ));
BOOST_CONCEPT_ASSERT( ( boost::EdgeListGraphConcept< SG > ));
BOOST_CONCEPT_ASSERT( ( boost::AdjacencyGraphConcept< SG > ));
BOOST_CONCEPT_ASSERT( (
  boost::ReadablePropertyGraphConcept< SG , bgl::Arc ,
				       boost::edge_weight_t > ));
BOOST_CONCEPT_ASSERT( ( boost::IncidenceGraphConcept< AG > ));
BOOST_CONCEPT_ASSERT( ( boost::VertexListGraphConcept< AG > ));
BOOST_CONCEPT_ASSERT( ( boost::EdgeListGraphConcept< AG > ));
BOOST_CONCEPT_ASSERT( ( boost::AdjacencyGraphConcept< AG > ));
BOOST_CONCEPT_ASSERT( ( boost::IncidenceGraphConcept< LG > ));
BOOST_CONCEPT_ASSERT( ( boost::VertexListGraphConcept< LG > ));
BOOST_CONCEPT_ASSERT( ( boost::AdjacencyGraphConcept< LG > ));
BOOST_CONCEPT_ASSERT( ( boost::IncidenceGraphConcept< OLG > ));
BOOST_CONCEPT_ASSERT( ( boost::VertexListGraphConcept< OLG > ));
BOOST_CONCEPT_ASSERT( ( boost::EdgeMutableGraphConcept< OLG > ));
BOOST_CONCEPT_ASSERT(
  ( boost::ReadablePropertyMapConcept< bgl::RowWeightMap< SG > , bgl::Arc > ));
BOOST_CONCEPT_ASSERT( (
  boost::ReadablePropertyMapConcept< bgl::ListWeightMap< LG > , bgl::Edge > ));

/// the vector v as a property map by the vertex index (a pointer is not
/// one once compressed_sparse_row_graph.hpp is included)
template< class T >
auto byIndex( std::vector< T > & v )
{
 return( boost::make_iterator_property_map(
   v.data() , boost::typed_identity_property_map< int >() ) );
 }

/// the graph of Boost.Graph in compressed rows on the arcs E
CSR csrOf( int n , const std::vector< std::tuple< int , int , double > > & E )
{
 std::vector< std::pair< int , int > > A;
 std::vector< EW > W;
 for( auto [ u , v , w ] : E ) {
  A.emplace_back( u , v );
  W.push_back( { w } );
  }
 return( CSR( boost::edges_are_unsorted_multi_pass , A.begin() , A.end() ,
	       W.begin() , std::size_t( n ) ) );
 }

/// the distances from s of Dijkstra's algorithm of Boost.Graph on g
template< class G , class WM >
std::vector< double > bglDijkstra( const G & g , int s , WM w )
{
 const auto n = std::size_t( num_vertices( g ) );
 std::vector< double > d( n );
 std::vector< int > p( n );
 boost::dijkstra_shortest_paths( g , s ,
				 boost::weight_map( w )
				   .distance_map( byIndex( d ) )
				   .predecessor_map( byIndex( p ) ) );
 return( d );
 }

/// the components of Boost.Graph on g, and their number in k
template< class G >
std::vector< int > bglComponents( const G & g , int & k )
{
 std::vector< int > c( std::size_t( num_vertices( g ) ) );
 k = boost::strong_components( g , byIndex( c ) );
 return( c );
 }

/// whether the order T (in reverse, as Boost.Graph gives it) puts each head
/// of an arc of g before its tail
template< class G >
bool reverseTopological( const G & g , const std::vector< int > & T )
{
 std::vector< int > at( T.size() );
 for( std::size_t i = 0 ; i < T.size() ; ++i )
  at[ std::size_t( T[ i ] ) ] = int( i );
 for( auto [ b , e ] = edges( g ) ; b != e ; ++b )
  if( at[ std::size_t( target( *b , g ) ) ] >
      at[ std::size_t( source( *b , g ) ) ] )
   return( false );
 return( true );
 }

/// the maximum flows of Boost.Graph from s to t on the graph in compressed
/// rows G of capacities w, which has the reverse of each arc: Edmonds-Karp,
/// push-relabel and Boykov-Kolmogorov
template< class G >
std::array< double , 3 > bglFlows( const G & g , int s , int t )
{
 const auto m = std::size_t( num_edges( g ) );
 const auto n = std::size_t( num_vertices( g ) );
 std::vector< double > cap( m ) , res( m );
 std::vector< bgl::Arc > rev( m ) , pred( n );
 for( auto [ b , e ] = edges( g ) ; b != e ; ++b ) {
  const bgl::Arc a = *b;
  cap[ std::size_t( a.pos ) ] = get( boost::edge_weight , g , a );
  const int u = source( a , g ) , v = target( a , g );
  for( auto [ ob , oe ] = out_edges( v , g ) ; ob != oe ; ++ob )
   if( target( *ob , g ) == u )
    rev[ std::size_t( a.pos ) ] = *ob;
  }
 const bgl::ArcIndexMap ix;
 auto C = boost::make_iterator_property_map( cap.data() , ix );
 auto R = boost::make_iterator_property_map( res.data() , ix );
 auto V = boost::make_iterator_property_map( rev.data() , ix );
 auto P = boost::make_iterator_property_map(
   pred.data() , boost::typed_identity_property_map< int >() );
 std::vector< boost::default_color_type > col( n );
 std::vector< double > dist( n );
 std::array< double , 3 > f{};
 f[ 0 ] = boost::edmonds_karp_max_flow( g , s , t ,
					boost::capacity_map( C )
					  .residual_capacity_map( R )
					  .reverse_edge_map( V )
					  .color_map( byIndex( col ) )
					  .predecessor_map( P ) );
 f[ 1 ] = boost::push_relabel_max_flow(
   g , s , t , C , R , V , boost::typed_identity_property_map< int >() );
 std::ranges::fill( col , boost::white_color );
 f[ 2 ] = boost::boykov_kolmogorov_max_flow(
   g , C , R , V , P , byIndex( col ) , byIndex( dist ) ,
   boost::typed_identity_property_map< int >() , s , t );
 return( f );
 }

}  // end( namespace )
#endif

// Boost.Graph on StaticGraph and ArrayGraph against Boost.Graph on its CSR
// and against ADS: the distances of Dijkstra and of Bellman-Ford, the
// strong components, the topological order of a DAG, the depth-first visit
ADS_TEST( "bgl-row-graphs" )
{
#if defined( ADS_WITH_BOOST )
 for( unsigned seed = 1 ; seed <= 6 ; ++seed ) {
  const int n = 60;
  const auto E = randomArcs( n , 240 , seed );
  const SG sg( n , E );
  const AG ag( n , E );
  const CSR c = csrOf( n , E );
  const auto ref = bglDijkstra( c , 0 , get( &EW::w , c ) );
  CHECK( bglDijkstra( sg , 0 , get( boost::edge_weight , sg ) ) == ref );
  CHECK( bglDijkstra( ag , 0 , get( boost::edge_weight , ag ) ) == ref );
  CHECK( bglDistances( shortestPaths( sg , 0 , Frontier::HEAP ).d ) == ref );
  // Bellman-Ford on the list of the edges
  std::vector< double > d( std::size_t( n ) ,
			   std::numeric_limits< double >::max() );
  std::vector< int > p( std::size_t( n ) , 0 );
  d[ 0 ] = 0;
  CHECK( boost::bellman_ford_shortest_paths(
    sg , n ,
    boost::weight_map( get( boost::edge_weight , sg ) )
      .distance_map( byIndex( d ) )
      .predecessor_map( byIndex( p ) ) ) );
  CHECK( d == ref );
  // the strong components, three ways
  int k1 = 0 , k2 = 0 , k3 = 0;
  const auto c1 = bglComponents( sg , k1 );
  const auto c2 = bglComponents( ag , k2 );
  const auto c3 = bglComponents( c , k3 );
  CHECK( ( k1 == k3 ) && ( k2 == k3 ) && samePartition( c1 , c3 ) &&
	 samePartition( c2 , c3 ) && samePartition( scc( sg ) , c3 ) );
  // a DAG: the topological order of Boost.Graph is one
  const SG dag( n , randomArcs( n , 200 , seed , true ) );
  std::vector< int > T;
  boost::topological_sort( dag , std::back_inserter( T ) );
  CHECK( ( int( T.size() ) == n ) && reverseTopological( dag , T ) );
  // the adjacency and the edge index
  int arcs = 0;
  for( auto [ b , e ] = edges( sg ) ; b != e ; ++b , ++arcs )
   CHECK( get( boost::edge_index , sg , *b ) == arcs );
  CHECK( arcs == num_edges( sg ) && arcs == 240 );
  for( int u = 0 ; u < n ; ++u ) {
   std::vector< int > A( adjacent_vertices( u , ag ).first ,
			 adjacent_vertices( u , ag ).second );
   CHECK( A == ag.adj( u ) && out_degree( u , ag ) == int( A.size() ) );
   }
  }
 // the unoriented graph: both arcs of each edge
 const SG u( 4 , { { 0 , 1 , 1. } , { 1 , 2 , 2. } } , false );
 CHECK( num_edges( u ) == 4 && out_degree( 1 , u ) == 2 );
#endif
 }

// the maximum flows of Boost.Graph on StaticGraph and ArrayGraph, with the
// reverse arcs of capacity 0, against the same on its adjacency list
ADS_TEST( "bgl-max-flows" )
{
#if defined( ADS_WITH_BOOST )
 for( unsigned seed = 1 ; seed <= 4 ; ++seed ) {
  const int n = 30;
  // no two opposite arcs, so that each arc has a reverse of its own of
  // capacity 0, which push-relabel of Boost.Graph wants
  std::set< std::pair< int , int > > there;
  std::vector< std::tuple< int , int , double > > E;
  for( auto [ u , v , w ] : randomArcs( n , 120 , seed + 10 ) )
   if( ! there.count( { v , u } ) ) {
    there.insert( { u , v } );
    E.emplace_back( u , v , w );
    }
  const auto forward = E;
  for( const auto & [ u , v , w ] : forward )
   E.emplace_back( v , u , 0. );
  const SG sg( n , E );
  const AG ag( n , E );
  const auto f1 = bglFlows( sg , 0 , n - 1 );
  const auto f2 = bglFlows( ag , 0 , n - 1 );
  // the reference: Edmonds-Karp of Boost.Graph on its adjacency list
  using Tr =
    boost::adjacency_list_traits< boost::vecS , boost::vecS ,
				  boost::directedS >;
  using FL = boost::adjacency_list<
    boost::vecS , boost::vecS , boost::directedS , boost::no_property ,
    boost::property<
      boost::edge_capacity_t , double ,
      boost::property<
	boost::edge_residual_capacity_t , double ,
	boost::property< boost::edge_reverse_t , Tr::edge_descriptor > > > >;
  FL fl( n );
  auto cap = get( boost::edge_capacity , fl );
  auto rev = get( boost::edge_reverse , fl );
  std::map< std::pair< int , int > , FL::edge_descriptor > at;
  for( auto [ u , v , w ] : E ) {
   const auto e = add_edge( u , v , fl ).first;
   cap[ e ] = w;
   at[ { u , v } ] = e;
   }
  for( auto & [ uv , e ] : at )
   rev[ e ] = at[ { uv.second , uv.first } ];
  const double ref = boost::edmonds_karp_max_flow( fl , 0 , n - 1 );
  for( double f : f1 )
   CHECK( f == ref );
  for( double f : f2 )
   CHECK( f == ref );
  }
#endif
 }

// Boost.Graph on ListGraph and on an ObservedGraph of it: the visits and
// Dijkstra's algorithm, the changes by add_edge(), remove_edge(),
// add_vertex(), clear_vertex() and remove_vertex() heard by the observers,
// the names kept, and a reoptimizing observer driven by Boost.Graph that
// agrees with Dijkstra's algorithm of Boost.Graph after each change
ADS_TEST( "bgl-list-graphs" )
{
#if defined( ADS_WITH_BOOST )
 const int n = 40;
 const auto E = randomArcs( n , 160 , 7 );
 LG g( n );
 EdgeCount cnt;
 OLG og( g , cnt );
 for( auto [ u , v , w ] : E )
  CHECK( add_edge( u , v , w , og ).second );
 CHECK( ! add_edge( std::get< 0 >( E[ 0 ] ) , std::get< 1 >( E[ 0 ] ) , 1. ,
		    og )
	    .second );  // there already: no insertion, no event
 CHECK( cnt.ins == 160 && cnt.del == 0 );
 const SG sg( n , E );
 const auto ref = bglDijkstra( sg , 0 , get( boost::edge_weight , sg ) );
 CHECK( bglDijkstra( g , 0 , get( boost::edge_weight , g ) ) == ref );
 CHECK( bglDijkstra( og , 0 , get( boost::edge_weight , og ) ) == ref );
 // a visit in depth on the observed graph
 std::vector< boost::default_color_type > col( std::size_t( n ) ,
					       boost::white_color );
 boost::depth_first_search( og , boost::default_dfs_visitor() ,
			    byIndex( col ) );
 // the changes
 for( int k = 0 ; k < 10 ; ++k )
  remove_edge( std::get< 0 >( E[ std::size_t( k ) ] ) ,
	       std::get< 1 >( E[ std::size_t( k ) ] ) , og );
 CHECK( cnt.del == 10 &&
	! g.hasEdge( std::get< 0 >( E[ 0 ] ) , std::get< 1 >( E[ 0 ] ) ) );
 const int x = add_vertex( og );
 CHECK( x == n && num_vertices( og ) == n + 1 );
 add_edge( x , 0 , 1. , og );
 add_edge( 1 , x , 1. , og );
 const int before = cnt.del;
 clear_vertex( x , og );
 CHECK( cnt.del == before + 2 && out_degree( x , og ) == 0 );
 remove_vertex( 3 , og );
 CHECK( ! og.hasNode( 3 ) && og.hasNode( 4 ) && num_vertices( og ) == n + 1 );
 int seen = 0;
 for( auto [ b , e ] = vertices( og ) ; b != e ; ++b , ++seen )
  CHECK( *b != 3 );
 CHECK( seen == n );
 // a reoptimizing observer of the shortest paths
 LG h( n );
 for( auto [ u , v , w ] : E )
  h.insertEdge( u , v , w );
 ShortestPathsObserver< LG > sp( h , 0 );
 Observable< LG , ShortestPathsObserver< LG > > oh( h , sp );
 std::mt19937 r( 11 );
 for( int k = 0 ; k < 40 ; ++k ) {
  const int u = int( r() % unsigned( n ) ) , v = int( r() % unsigned( n ) );
  if( u == v )
   continue;
  if( h.hasEdge( u , v ) )
   remove_edge( u , v , oh );
  else
   add_edge( u , v , double( 1 + r() % 20 ) , oh );
  const auto d = bglDijkstra( h , 0 , get( boost::edge_weight , h ) );
  for( int z = 0 ; z < n ; ++z )
   CHECK( bglDistances( { sp.distance( z ) } )[ 0 ] == d[ std::size_t( z ) ] );
  }
#endif
 }

// the algorithms of ADS on the graphs of Boost.Graph, through FromBGL:
// the shortest paths, the strong components and Prim's tree, against the
// same algorithms on StaticGraph; an observed view changed by ADS, the
// events heard and the adjacency list of Boost.Graph changed with it
ADS_TEST( "bgl-from-bgl" )
{
#if defined( ADS_WITH_BOOST )
 const int n = 50;
 const auto E = randomArcs( n , 200 , 5 );
 AL al( n );
 for( auto [ u , v , w ] : E )
  add_edge( u , v , w , al );
 const CSR c = csrOf( n , E );
 using WA = decltype( get( boost::edge_weight , al ) );
 using WC = decltype( get( &EW::w , c ) );
 bgl::FromBGL< AL , WA > va( al , get( boost::edge_weight , al ) );
 const bgl::FromBGL< const CSR , WC > vc( c , get( &EW::w , c ) );
 static_assert( Graph< decltype( va ) > && Graph< decltype( vc ) > );
 static_assert( DynamicGraph< decltype( va ) > );
 const SG sg( n , E );
 const auto ref = shortestPaths( sg , 0 , Frontier::HEAP ).d;
 CHECK( shortestPaths( va , 0 , Frontier::HEAP ).d == ref );
 CHECK( shortestPaths( vc , 0 , Frontier::FIBONACCI ).d == ref );
 CHECK( samePartition( scc( va ) , scc( sg ) ) );
 CHECK( va.weight( std::get< 0 >( E[ 0 ] ) , std::get< 1 >( E[ 0 ] ) ) ==
	std::get< 2 >( E[ 0 ] ) );
 // the observed view: the changes of ADS reach the graph of Boost.Graph
 struct Count
 {
  int ins = 0 , del = 0 , nodes = 0;
  void on( const EdgeInserted< double > & ) { ++ins; }
  void on( const EdgeDeleted< double > & ) { ++del; }
  void on( const NodeInserted & ) { ++nodes; }
  };
 Count k;
 ObservedGraph< decltype( va ) , Static< Count > > ov( va , k );
 const int x = ov.insertNode();
 ov.insertEdge( 0 , x , 2. );
 ov.insertEdge( x , 1 , 3. );
 CHECK( x == n && num_vertices( al ) == std::size_t( n + 1 ) );
 CHECK( edge( 0 , x , al ).second &&
	get( boost::edge_weight , al , edge( x , 1 , al ).first ) == 3. );
 ov.deleteEdge( 0 , x );
 CHECK( ! edge( 0 , x , al ).second && k.ins == 2 && k.del == 1 &&
	k.nodes == 1 );
 ov.deleteNode( 2 );  // its vertex kept, isolated: no name changes
 CHECK( ! va.hasNode( 2 ) && va.n() == n && va.bound() == n + 1 &&
	out_degree( 2 , al ) == 0 );
#endif
 }

// the queues of ADS in the visit of Boost.Graph, native and lazy, against
// dijkstra_shortest_paths() of Boost.Graph on its own heap
ADS_TEST( "bgl-ads-queues" )
{
#if defined( ADS_WITH_BOOST )
 for( unsigned seed = 1 ; seed <= 4 ; ++seed ) {
  const int n = 80;
  const auto E = randomArcs( n , 400 , seed + 20 );
  const SG g( n , E );
  const auto ref = bglDijkstra( g , 0 , get( boost::edge_weight , g ) );
  const auto run = [ & ]( auto & q ) {
   std::vector< double > d( std::size_t( n ) , 0. );
   std::vector< int > p( std::size_t( n ) , 0 );
   bgl::dijkstraShortestPaths( g , 0 , q , byIndex( p ) , byIndex( d ) );
   return( d );
   };
  IndexedDaryHeap< double , 4 > h4( n );
  HeapPriorityQueue< int , std::pair< double , int > > hp( n );
  FibonacciHeapPriorityQueue< int , double > fh;
  PairingHeap< int , std::pair< double , int > > ph;
  using P = std::pair< double , int >;
  std::priority_queue< P , std::vector< P > , std::greater< P > > sq;
  boost::heap::fibonacci_heap< P , boost::heap::compare< std::greater< P > > >
    bf;
  CHECK( run( h4 ) == ref );
  CHECK( run( hp ) == ref );
  CHECK( run( fh ) == ref );
  CHECK( run( ph ) == ref );
  CHECK( run( sq ) == ref );
  CHECK( run( bf ) == ref );
  }
#endif
 }

// the heaps of Boost.Heap in the shortest paths and in Prim's algorithm of
// ADS: native with the free queue_decrease() of Heap.h, lazy without
// handles, and the same distances and trees as on the heaps of ADS
ADS_TEST( "boost-heap-frontier" )
{
#if defined( ADS_WITH_BOOST )
 using P = std::pair< double , int >;
 using G = std::greater< P >;
 using BF = boost::heap::fibonacci_heap< P , boost::heap::compare< G > >;
 using BP = boost::heap::pairing_heap< P , boost::heap::compare< G > >;
 using BD = boost::heap::d_ary_heap< P , boost::heap::arity< 4 > ,
				     boost::heap::mutable_< true > ,
				     boost::heap::compare< G > >;
 using BI = boost::heap::d_ary_heap< P , boost::heap::arity< 4 > ,
				     boost::heap::compare< G > >;
 static_assert( PriorityQueue< BF > && PriorityQueue< BI > );
 static_assert( decrease_key_of< BF >() == decrease_key_kind::native );
 static_assert( decrease_key_of< BP >() == decrease_key_kind::native );
 static_assert( decrease_key_of< BD >() == decrease_key_kind::native );
 static_assert( decrease_key_of< BI >() == decrease_key_kind::lazy );
 static_assert( std::same_as< queue_handle_t< BF > , BF::handle_type > );
 const int n = 70;
 const auto E = randomArcs( n , 350 , 3 );
 const SG g( n , E );
 const SG u( n , E , false );
 const auto ref = shortestPaths( g , 0 , Frontier::HEAP ).d;
 HeapPriorityQueue< int , std::pair< double , int > > hq( n );
 const auto tree = prim( u , 0 , hq );  // the ties by the names
 const auto run = [ & ]( auto q ) {
  PriorityFrontier S( q , n );
  CHECK( shortestPaths( g , 0 , S ).d == ref );
  decltype( q ) q2;
  CHECK( prim( u , 0 , q2 ) == tree );
  };
 run( BF() );
 run( BP() );
 run( BD() );
 run( BI() );
 // the direction of the decrease: increase() of Boost.Heap, toward the top
 BF h;
 const auto a = h.push( { 5. , 1 } );
 h.push( { 3. , 2 } );
 queue_decrease( h , a , P( 1. , 1 ) );
 CHECK( h.top().second == 1 );
#endif
 }

// the traversal of the iterators of ADS that build their item: forward or
// more for Boost, so that Boost.Range takes the dictionaries; the iterators
// of the other libraries keep the traversal they declare
ADS_TEST( "boost-iterator-traversal" )
{
#if defined( ADS_WITH_BOOST )
 using I = BTreeMap< int , int >::const_iterator;
 static_assert( bgl::built_reference_iterator< I > );
 static_assert( std::is_convertible_v<
		typename boost::iterators::iterator_traversal< I >::type ,
		boost::forward_traversal_tag > );
 using J = std::istream_iterator< int >;
 static_assert( ! bgl::built_reference_iterator< J > );
 static_assert(
   std::is_same_v< typename boost::iterators::iterator_traversal< J >::type ,
		   boost::single_pass_traversal_tag > );
 BTreeMap< int , int > b;
 for( int k = 0 ; k < 100 ; ++k )
  b.insert( k , ( k * 37 ) % 101 );
 const auto it = boost::max_element( b ,
				     []( const auto & x , const auto & y ) {
  return( x.second < y.second );
  } );
 CHECK( ( *it ).second == 100 );
 CHECK( boost::count_if(
	  b , []( const auto & x ) { return( x.first % 2 == 0 ); } ) == 50 );
#endif
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- THE STANDARD LIBRARY -------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// the members by iterator of the sequence S against std::vector, and, with
/// C++23, std::flat_set and std::flat_map on S against std::set and std::map
template< class S >
void stdSequence( void )
{
 std::mt19937 r( 9 );
 S s;
 std::vector< int > v;
 for( int k = 0 ; k < 400 ; ++k ) {
  const int x = int( r() % 1000 );
  const std::size_t i = v.empty() ? 0 : r() % ( v.size() + 1 );
  switch( r() % 4 ) {
   case( 0 ):
   case( 1 ):
    CHECK( *s.insert( s.cbegin() + std::ptrdiff_t( i ) , x ) == x );
    v.insert( v.begin() + std::ptrdiff_t( i ) , x );
    break;
   case( 2 ):
    s.emplace( s.cbegin() + std::ptrdiff_t( i ) , x );
    v.emplace( v.begin() + std::ptrdiff_t( i ) , x );
    break;
   default:
    if( i < v.size() ) {
     const auto it = s.erase( s.cbegin() + std::ptrdiff_t( i ) );
     v.erase( v.begin() + std::ptrdiff_t( i ) );
     CHECK( std::size_t( it - s.begin() ) == i );
     }
   }
  CHECK( std::ranges::equal( s , v ) );
  }
 const std::array< int , 3 > a{ 7 , 8 , 9 };
 s.insert( s.cbegin() + 1 , a.begin() , a.end() );
 v.insert( v.begin() + 1 , a.begin() , a.end() );
 s.erase( s.cbegin() + 2 , s.cbegin() + 20 );
 v.erase( v.begin() + 2 , v.begin() + 20 );
 CHECK( std::ranges::equal( s , v ) );
#if defined( __cpp_lib_flat_set ) && defined( __cpp_lib_flat_map )
 std::flat_set< int , std::less< int > , S > fs;
 std::set< int > rs;
 std::flat_map< int , int , std::less< int > , S , S > fm;
 std::map< int , int > rm;
 for( int k = 0 ; k < 2000 ; ++k ) {
  const int x = int( r() % 300 );
  if( r() % 3 ) {
   CHECK( fs.insert( x ).second == rs.insert( x ).second );
   fm[ x ] = k;
   rm[ x ] = k;
   }
  else {
   CHECK( fs.erase( x ) == rs.erase( x ) );
   CHECK( fm.erase( x ) == rm.erase( x ) );
   }
  }
 CHECK( std::ranges::equal( fs , rs ) );
 CHECK( std::ranges::equal( fm , rm ) );
 CHECK( fm.lower_bound( 150 ) - fm.begin() ==
	std::distance( rm.begin() , rm.lower_bound( 150 ) ) );
#endif
 }

}  // end( namespace )

// the members by iterator of the sequences by rank and StdSequence against
// std::vector, and std::flat_set and std::flat_map on them against std::set
// and std::map
ADS_TEST( "std-flat-map" )
{
 stdSequence< TieredVector< int > >();
 stdSequence< RootishArrayStack< int > >();
 stdSequence< Treque< int > >();
 stdSequence< CubishArrayStack< int > >();
 stdSequence< DualArrayDeque< int > >();
 stdSequence< StdSequence< ContiguousDeque< int > > >();
#if defined( __cpp_lib_flat_map )
 TieredFlatMap< std::string , int > m;
 m[ "b" ] = 2;
 m[ "a" ] = 1;
 CHECK( m.begin()->first == "a" && m.size() == 2 );
 TieredFlatSet< int > s{ 3 , 1 , 2 , 3 };
 CHECK( s.size() == 3 && *s.begin() == 1 );
#endif
 }

// the pools of ADS under the containers of std::pmr
ADS_TEST( "std-pmr" )
{
 PoolResource<> pool;
 PmrResource<> mr( pool );
 PmrResource<> same( pool );
 PoolResource<> other;
 PmrResource<> mo( other );
 CHECK( mr.is_equal( same ) && ! mr.is_equal( mo ) &&
	! mr.is_equal( *std::pmr::new_delete_resource() ) );
 CHECK( &mr.resource() == &pool );
 std::pmr::map< int , int > m( &mr );
 std::pmr::list< int > l( &mr );
 std::pmr::vector< std::pmr::string > v( &mr );
 for( int k = 0 ; k < 2000 ; ++k ) {
  m[ k ] = k * k;
  l.push_back( k );
  v.emplace_back( std::string( std::size_t( k % 40 ) , 'x' ) );
  }
 CHECK( m.size() == 2000 && m[ 44 ] == 1936 && l.size() == 2000 );
 CHECK( v[ 39 ].size() == 39 && v[ 39 ].get_allocator().resource() == &mr );
 for( int k = 0 ; k < 2000 ; k += 2 )
  m.erase( k );
 CHECK( m.size() == 1000 );
 }

namespace
{

/// whether sort( A ) on a span, a deque and a TieredVector of the items of
/// A gives what it gives on a vector of them
template< class T , class F >
bool sortsAlike( const std::vector< T > & A , F && sort )
{
 std::vector< T > v( A );
 sort( v );
 std::vector< T > w( A );
 sort( std::span< T >( w ) );
 std::deque< T > d( A.begin() , A.end() );
 sort( d );
 TieredVector< T > t;
 for( const T & x : A )
  t.push_back( x );
 sort( t );
 return( ( v == w ) && std::ranges::equal( v , d ) &&
	  std::ranges::equal( v , t ) );
 }

}  // end( namespace )

// the sorts of ADS on ranges other than a vector: a span, a deque and a
// TieredVector give what the vector gives, the results of the functions
// that return one included, and the parallel sorts on a span
ADS_TEST( "sort-ranges" )
{
 std::mt19937 r( 4 );
 std::vector< int > A( 3000 );
 for( int & x : A )
  x = int( r() % 100000 ) - 50000;
 std::vector< unsigned > U( 3000 );
 for( unsigned & x : U )
  x = unsigned( r() );
 std::vector< std::string > S( 500 );
 for( std::string & s : S )
  s = std::to_string( r() % 100000 );
 std::vector< std::pair< int , int > > Q( 2000 );
 for( auto & [ a , b ] : Q )
  a = int( r() % 50 ) , b = int( r() );
 const auto byFirst = []( const auto & x , const auto & y ) {
  return( x.first < y.first );
  };
 CHECK( sortsAlike( A , []( auto && a ) { ADS::sort( a ); } ) );
 CHECK(
   sortsAlike( A , []( auto && a ) { ADS::sort( a , std::greater<>() ); } ) );
 CHECK( sortsAlike( Q , [ & ]( auto && a ) { stableSort( a , byFirst ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { pdqSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { dualPivotQuickSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { threeWayQuickSort( a ); } ) );
 CHECK( sortsAlike( Q , [ & ]( auto && a ) { powerSort( a , byFirst ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { sampleSort( a ); } ) );
 CHECK( sortsAlike( U , []( auto && a ) { radixSort( a ); } ) );
 CHECK( sortsAlike( U , []( auto && a ) { americanFlagSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { learnedSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { heapsort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { mergeSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { mergeSortBottomUp( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { quickSort( a ); } ) );
 CHECK( sortsAlike( A , []( auto && a ) { shellSort( a ); } ) );
 CHECK( sortsAlike( S , []( auto && a ) { msdRadixSort( a ); } ) );
 CHECK( sortsAlike( S , []( auto && a ) { multikeyQuickSort( a ); } ) );
 const std::vector< int > B( A.begin() , A.begin() + 300 );
 CHECK( sortsAlike( B , []( auto && a ) { insertionSort( a ); } ) );
 CHECK( sortsAlike( B , []( auto && a ) { bubbleSort( a ); } ) );
 // what the sort gives back
 std::vector< int > c( A );
 std::deque< int > cd( A.begin() , A.end() );
 CHECK( countInversions( cd ) == countInversions( c ) );
 // the parallel sorts
 ThreadPool p( 3 );
 std::vector< int > v( A ) , w( A );
 ADS::sort( p , v );
 ADS::sort( p , std::span< int >( w ) );
 CHECK( v == w );
 std::deque< int > d( A.begin() , A.end() );
 ADS::stableSort( p , d , std::greater<>() );
 CHECK( std::ranges::equal( d , v | std::views::reverse ) );
 }

// the queues of the standard library and of ADS in the frontiers of ADS by
// the customization points: std::priority_queue of pairs lazy, the heaps
// of ADS native, the same distances and the same trees
ADS_TEST( "queue-cpo" )
{
 using P = std::pair< double , int >;
 using SQ = std::priority_queue< P , std::vector< P > , std::greater< P > >;
 static_assert( FrontierQueue< SQ > &&
		decrease_key_of< SQ >() == decrease_key_kind::lazy );
 static_assert( FrontierQueue< IndexedDaryHeap< double > > &&
		decrease_key_of< IndexedDaryHeap< double > >() ==
		  decrease_key_kind::native );
 static_assert(
   std::same_as< queue_handle_t< IndexedDaryHeap< double > > , int > );
 const int n = 60;
 const auto E = randomArcs( n , 300 , 8 );
 const StaticGraph< double > g( n , E );
 const StaticGraph< double > u( n , E , false );
 SQ q;
 PriorityFrontier S( q , n );
 CHECK( shortestPaths( g , 0 , S ).d ==
	shortestPaths( g , 0 , Frontier::HEAP ).d );
 SQ q2;
 HeapPriorityQueue< int , std::pair< double , int > > hq( n );
 const auto tree = prim( u , 0 , hq );  // the ties by the names
 CHECK( prim( u , 0 , q2 ) == tree );
 IndexedDaryHeap< std::pair< double , int > > h( n );
 CHECK( prim( u , 0 , h ) == tree );
 IndexedDaryHeap< double > h2( 2 );
 queue_decrease( h2 , ADS::queue_insert( h2 , 1 , 5. ) , 2. );
 CHECK( h2.key( 1 ) == 2. );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- GMP ----------------------------------*/
/*--------------------------------------------------------------------------*/

// the conversions between BigInteger, Rational and the numbers of GMP, of
// its interface of C (mpz_t, mpq_t) and of C++ (mpz_class, mpq_class): the
// round trips, the signs, zero, and the results of GMP and of ADS alike
ADS_TEST( "gmp-conversions" )
{
#if defined( ADS_WITH_GMP )
 std::mt19937_64 r( 2 );
 mpz_t z;
 mpz_init( z );
 for( int k = 0 ; k < 200 ; ++k ) {
  std::string s = ( k % 2 ) ? "-" : "";
  const int digits = 1 + k % 90;
  for( int i = 0 ; i < digits ; ++i )
   s += char( '0' + ( i == 0 ? 1 + r() % 9 : r() % 10 ) );
  const BigInteger x( s );
  x.toGmp( z );
  char * t = mpz_get_str( nullptr , 10 , z );
  CHECK( s == t );
  void ( *freef )( void * , std::size_t );
  mp_get_memory_functions( nullptr , nullptr , &freef );
  freef( t , std::strlen( t ) + 1 );
  CHECK( BigInteger::fromGmp( z ) == x );
  mpz_mul( z , z , z );
  CHECK( BigInteger::fromGmp( z ) == x * x );
  }
 mpz_set_ui( z , 0 );
 CHECK( BigInteger::fromGmp( z ) == BigInteger( 0 ) );
 BigInteger( 0 ).toGmp( z );
 CHECK( mpz_sgn( z ) == 0 );
 mpz_clear( z );
 mpq_t q;
 mpq_init( q );
 const Rational a( BigInteger( -6 ) , BigInteger( 4 ) );
 a.toGmp( q );
 CHECK( mpz_get_si( mpq_numref( q ) ) == -3 &&
	mpz_get_si( mpq_denref( q ) ) == 2 );
 CHECK( Rational::fromGmp( q ) == a );
 mpz_set_si( mpq_numref( q ) , 10 );  // not canonical: reduced
 mpz_set_si( mpq_denref( q ) , -4 );
 CHECK( Rational::fromGmp( q ) ==
	Rational( BigInteger( -5 ) , BigInteger( 2 ) ) );
 mpq_clear( q );
 #if ADS_TEST_GMPXX
 const mpz_class m( "-98765432109876543210987654321098765432109876543210" );
 const BigInteger y( m );
 CHECK( y.toString() == m.get_str() && y.toGmp< mpz_class >() == m );
 const mpq_class f( mpz_class( 22 ) , mpz_class( 7 ) );
 const Rational g( f );
 CHECK( g.toString() == "22/7" && g.toGmp< mpq_class >() == f );
 #endif
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File engineered_interop.cpp --------------------*/
/*--------------------------------------------------------------------------*/
