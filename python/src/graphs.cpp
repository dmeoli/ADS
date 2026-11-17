/*--------------------------------------------------------------------------*/
/*----------------------------- File graphs.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graphs in compressed rows of the module `ads`: `StaticGraph` with no
 * weights and with weights of type `double` in both layouts of the arcs,
 * `CsrMatrix`, the pool of threads, and the algorithms of ADS that run on
 * them, sequential and parallel, each with the GIL released. A graph is
 * built from the arrays of its rows (one copy of each, through
 * `ArrayArchive`, with no sorting when the rows are sorted) or from its
 * edges, and its offsets, heads and weights are views of NumPy over the
 * arrays of the graph, also in the interleaved layout (by the strides of
 * the pairs) and in a graph opened by `restore_mapped()` (over the mapped
 * file).
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

#include "Bindings.h"

#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/StrongComponents.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/parallel/ParallelGraph.h"
#include "ADS/parallel/ParallelGreedy.h"
#include "ADS/parallel/ParallelShortestPaths.h"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ads_python
{

using namespace nb::literals;

namespace
{

/// the graphs of the module
using Plain = ADS::StaticGraph<>;
using Separate = ADS::StaticGraph< double , ADS::ArcLayout::Separate >;
using Interleaved = ADS::StaticGraph< double , ADS::ArcLayout::Interleaved >;

/// the matrix of the module
using Matrix = ADS::BasicCsrMatrix< double , std::uint32_t , std::size_t >;

/// the error of a negative cycle reached by the shortest paths
class NegativeCycle : public std::runtime_error
{
 public:
 using std::runtime_error::runtime_error;
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONSTRUCTION ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the rows of a graph checked and, where a row is not sorted by head,
/// sorted in a copy; the arrays to read are then those of \f$ h \f$ and
/// \f$ w \f$ if they are not empty
template< bool Weighted >
struct Rows
{
 std::vector< int > h;     ///< the sorted heads, if a row was not
 std::vector< double > w;  ///< their weights
 };

/// the rows of \f$ n \f$ nodes at the offsets \f$ o \f$, with the heads
/// \f$ h \f$ and the weights \f$ w \f$ (none if empty), checked; the
/// copies of `Rows` if a row must be sorted
template< bool Weighted >
Rows< Weighted > checkRows( std::span< const int > o ,
			    std::span< const int > h ,
			    std::span< const double > w , bool directed )
{
 if( o.empty() || ( o[ 0 ] != 0 ) )
  throw( std::invalid_argument( "the offsets must start with 0" ) );
 const std::size_t n = o.size() - 1;
 if( std::size_t( o[ n ] ) != h.size() )
  throw( std::invalid_argument(
    "the last offset must be the number of the heads" ) );
 if( Weighted && ( w.size() != h.size() ) )
  throw( std::invalid_argument( "one weight for each head" ) );
 if( ! directed && ( h.size() % 2 ) )
  throw( std::invalid_argument(
    "an unoriented graph has each edge in both rows" ) );
 bool sorted = true;
 for( std::size_t u = 0 ; u < n ; ++u ) {
  if( o[ u ] > o[ u + 1 ] )
   throw( std::invalid_argument( "the offsets must not decrease" ) );
  for( int k = o[ u ] ; k < o[ u + 1 ] ; ++k ) {
   const int v = h[ std::size_t( k ) ];
   if( ( v < 0 ) || ( std::size_t( v ) >= n ) )
    throw( std::invalid_argument( "a head is not a node" ) );
   if( std::size_t( v ) == u )
    throw( std::invalid_argument( "a loop, which a StaticGraph has not" ) );
   if( ( k > o[ u ] ) && ( h[ std::size_t( k ) - 1 ] > v ) )
    sorted = false;
   }
  }
 Rows< Weighted > r;
 if( ! sorted ) {
  r.h.assign( h.begin() , h.end() );
  if constexpr( Weighted )
   r.w.assign( w.begin() , w.end() );
  std::vector< std::size_t > p;
  for( std::size_t u = 0 ; u < n ; ++u ) {
   const std::size_t a = std::size_t( o[ u ] ) , b = std::size_t( o[ u + 1 ] );
   p.resize( b - a );
   std::iota( p.begin() , p.end() , a );
   std::stable_sort(
     p.begin() , p.end() ,
     [ & ]( std::size_t x , std::size_t y ) { return( h[ x ] < h[ y ] ); } );
   for( std::size_t k = a ; k < b ; ++k ) {
    r.h[ k ] = h[ p[ k - a ] ];
    if constexpr( Weighted )
     r.w[ k ] = w[ p[ k - a ] ];
    }
   }
  h = r.h;
  if constexpr( Weighted )
   w = r.w;
  }
 if( ! directed )  // each arc ( u , v ) with its twin ( v , u )
  for( std::size_t u = 0 ; u < n ; ++u )
   for( int k = o[ u ] ; k < o[ u + 1 ] ; ++k ) {
    const int v = h[ std::size_t( k ) ];
    const auto b = h.begin() + o[ v ] , e = h.begin() + o[ v + 1 ];
    const auto t = std::lower_bound( b , e , int( u ) );
    if( ( t == e ) || ( *t != int( u ) ) )
     throw( std::invalid_argument(
       "an unoriented graph has each edge in both rows" ) );
    if constexpr( Weighted )
     if( w[ std::size_t( t - h.begin() ) ] != w[ std::size_t( k ) ] )
      throw( std::invalid_argument(
	"the two arcs of an edge have different weights" ) );
    }
 return( r );
 }

/// the graph G of the rows of `from_csr()`
template< class G >
G fromCsr( const Input< int > & off , const Input< int > & head ,
	   const Input< double > * weight , bool directed )
{
 constexpr bool Weighted = G::weighted;
 std::span< const int > o = items( off ) , h = items( head );
 std::span< const double > w;
 if( weight )
  w = items( *weight );
 nb::gil_scoped_release r;
 const Rows< Weighted > s = checkRows< Weighted >( o , h , w , directed );
 if( ! s.h.empty() ) {
  h = s.h;
  w = s.w;
  }
 const long long m =
   directed ? ( long long )h.size() : ( long long )h.size() / 2;
 std::vector< ArrayArchive::Member > M{
  ( long long )directed , ArrayArchive::of( o.data() , o.size() ) ,
  ArrayArchive::of( h.data() , h.size() )
  };
 if constexpr( Weighted )
  M.push_back( ArrayArchive::of( w.data() , w.size() ) );
 M.push_back( m );
 ArrayArchive a( std::move( M ) );
 G g;
 g.persist( a );
 return( g );
 }

/// the graph G of the edges of `from_edges()`
template< class G >
G fromEdges( int n , const Input< int > & tail , const Input< int > & head ,
	     const Input< double > * weight , bool directed )
{
 const std::span< const int > s = items( tail ) , t = items( head );
 std::span< const double > w;
 if( weight )
  w = items( *weight );
 if( n < 0 )
  throw( std::invalid_argument( "a negative number of nodes" ) );
 if( s.size() != t.size() )
  throw( std::invalid_argument( "one head for each tail" ) );
 if( G::weighted && ( w.size() != s.size() ) )
  throw( std::invalid_argument( "one weight for each edge" ) );
 nb::gil_scoped_release r;
 for( std::size_t k = 0 ; k < s.size() ; ++k ) {
  if( ( s[ k ] < 0 ) || ( s[ k ] >= n ) || ( t[ k ] < 0 ) || ( t[ k ] >= n ) )
   throw( std::invalid_argument( "an end of an edge is not a node" ) );
  if( s[ k ] == t[ k ] )
   throw( std::invalid_argument( "a loop, which a StaticGraph has not" ) );
  }
 if constexpr( G::weighted ) {
  std::vector< std::tuple< int , int , double > > E( s.size() );
  for( std::size_t k = 0 ; k < s.size() ; ++k )
   E[ k ] = { s[ k ] , t[ k ] , w[ k ] };
  return( G( n , E , directed ) );
  }
 else {
  std::vector< std::pair< int , int > > E( s.size() );
  for( std::size_t k = 0 ; k < s.size() ; ++k )
   E[ k ] = { s[ k ] , t[ k ] };
  return( G( n , E , directed ) );
  }
 }

/// the node \f$ u \f$ of \f$ g \f$ checked
template< class G >
void checkNode( const G & g , int u )
{
 if( ! g.hasNode( u ) )
  throw( nb::index_error( "not a node of the graph" ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE VIEWS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the heads of the arcs from the \f$ k \f$-th on, \f$ c \f$ of them, as a
/// view that keeps \f$ o \f$ alive
template< class G >
Array< const int > headView( const G & g , std::size_t k , std::size_t c ,
			     nb::handle o )
{
 if( c == 0 )
  return( view< int >( g.offsets() , 0 , o ) );
 if constexpr( G::interleaved )
  return( view(
    &g.arcData()[ k ].to , c , o ,
    std::int64_t( sizeof( ADS::WeightedArc< double > ) / sizeof( int ) ) ) );
 else
  return( view( g.heads() + k , c , o ) );
 }

/// the weights of the arcs from the \f$ k \f$-th on, \f$ c \f$ of them, as
/// a view that keeps \f$ o \f$ alive
template< class G >
Array< const double > weightView( const G & g , std::size_t k , std::size_t c ,
				  nb::handle o )
{
 static const double none = 0;
 if( c == 0 )
  return( view( &none , 0 , o ) );
 if constexpr( G::interleaved )
  return( view( &g.arcData()[ k ].w , c , o ,
		 std::int64_t( sizeof( ADS::WeightedArc< double > ) /
			       sizeof( double ) ) ) );
 else
  return( view( g.weightData() + k , c , o ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE ALGORITHMS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the distances of `distance()`, -1 for the nodes not reached
inline void unreached( std::vector< int > & d )
{
 for( int & x : d )
  if( x == ADS::INF )
   x = -1;
 }

/// the structure of the shortest paths named \f$ s \f$
inline ADS::Frontier frontier( const std::string & s )
{
 if( s == "heap" )
  return( ADS::Frontier::HEAP );
 if( s == "list" )
  return( ADS::Frontier::LIST );
 if( s == "fibonacci" )
  return( ADS::Frontier::FIBONACCI );
 if( s == "queue" )
  return( ADS::Frontier::QUEUE );
 if( s == "stack" )
  return( ADS::Frontier::STACK );
 if( s == "deque" )
  return( ADS::Frontier::DEQUE );
 throw( std::invalid_argument(
   "the method is one of heap, list, fibonacci, queue, stack, deque" ) );
 }

/// whether \f$ g \f$ has a negative weight
template< class G >
bool negative( const G & g )
{
 const auto w = g.arcWeights();
 for( int k = 0 ; k < g.arcs() ; ++k )
  if( w[ k ] < 0 )
   return( true );
 return( false );
 }

/// each node labeled by the least node of its component, from the labels
/// \f$ c \f$ of any kind
inline std::vector< int > byLeast( std::vector< int > c )
{
 std::vector< int > least;
 int top = 0;
 for( const int x : c )
  top = std::max( top , x );
 least.assign( std::size_t( top ) + 1 , INT_MAX );
 for( std::size_t v = 0 ; v < c.size() ; ++v )
  least[ std::size_t( c[ v ] ) ] =
    std::min( least[ std::size_t( c[ v ] ) ] , int( v ) );
 for( int & x : c )
  x = least[ std::size_t( x ) ];
 return( c );
 }

/// the algorithms on the graph G
template< class G >
void bindAlgorithms( nb::module_ & m )
{
 m.def(
   "bfs" ,
   []( const G & g , int s , bool parallel , const G * gt ,
       ADS::ThreadPool * p ) {
    checkNode( g , s );
    if( gt && ( gt->n() != g.n() ) )
     throw( std::invalid_argument( "the transpose has other nodes" ) );
    std::vector< int > d;
    {
     nb::gil_scoped_release r;
     if( parallel )
      d = ADS::bfsDistances( pool( p ) , g , s , gt );
     else {
      d = ADS::distance( g , s ).first;
      unreached( d );
      }
     }
    return( owned( std::move( d ) ) );
    } ,
   "g"_a , "source"_a , "parallel"_a = false , "transpose"_a = nb::none() ,
   "pool"_a = nb::none() ,
   "The number of arcs of a shortest path from source to each node, -1 for "
   "the nodes not reached: the visit of the book, or, if parallel, the "
   "direction-optimizing visit of Beamer et al., which goes from the "
   "bottom up only on an unoriented graph or when the transpose is "
   "given." );
 m.def(
   "bfs_tree" ,
   []( const G & g , int s ) {
    checkNode( g , s );
    std::pair< std::vector< int > , std::vector< int > > t;
    {
     nb::gil_scoped_release r;
     t = ADS::distance( g , s );
     unreached( t.first );
     }
    return( std::make_pair( owned( std::move( t.first ) ) ,
			     owned( std::move( t.second ) ) ) );
    } ,
   "g"_a , "source"_a ,
   "The distances of bfs() and the parent of each node in the tree of the "
   "visit, -1 for the source and for the nodes not reached." );
 m.def(
   "connected_components" ,
   []( const G & g , bool parallel , ADS::ThreadPool * p ) {
    std::vector< int > c;
    {
     nb::gil_scoped_release r;
     if( parallel || g.directed() )
      c = ADS::connectedComponents( pool( p ) , g );
     else
      c = byLeast( ADS::cc( g ) );
     }
    return( owned( std::move( c ) ) );
    } ,
   "g"_a , "parallel"_a = false , "pool"_a = nb::none() ,
   "The least node of the component of each node, an oriented graph taken "
   "as unoriented (its weak components): the visits of the book, or "
   "Afforest of Sutton, Ben-Nun and Barak if parallel or if the graph is "
   "oriented." );
 m.def(
   "strongly_connected_components" ,
   []( const G & g , const std::string & how , bool topological ) {
    if( ( how != "pearce" ) && ( how != "tarjan" ) )
     throw( std::invalid_argument( "the method is pearce or tarjan" ) );
    std::vector< int > c;
    {
     nb::gil_scoped_release r;
     c = how == "pearce" ? ADS::sccPearce( g ) : ADS::sccTarjan( g );
     if( topological )
      for( int & x : c )
       --x;
     else
      c = byLeast( std::move( c ) );
     }
    return( owned( std::move( c ) ) );
    } ,
   "g"_a , "method"_a = "pearce" , "topological"_a = false ,
   "The least node of the strong component of each node, by the algorithm "
   "of Pearce (in n + O(1) words) or of Tarjan; if topological, the "
   "number of the component instead, from 0, in a topological order of "
   "the components (an arc between two goes to the greater number)." );
 m.def(
   "maximal_independent_set" ,
   []( const G & g , unsigned long long seed , ADS::ThreadPool * p ) {
    if( g.directed() )
     throw( std::invalid_argument( "the graph must be unoriented" ) );
    std::vector< int > s;
    {
     nb::gil_scoped_release r;
     s = ADS::maximalIndependentSet( pool( p ) , g ,
				     ADS::randomOrder( g.n() , seed ) );
     }
    return( owned( std::move( s ) ) );
    } ,
   "g"_a , "seed"_a = 1 , "pool"_a = nb::none() ,
   "The nodes, increasing, of the greedy maximal independent set in a "
   "random order of the seed, by the parallel algorithm of Blelloch, "
   "Fineman and Shun (the same set for every number of threads)." );
 m.def(
   "topological_sort" ,
   []( const G & g ) {
    std::vector< int > s;
    {
     nb::gil_scoped_release r;
     s = ADS::topsortKahn( g );
     }
    if( int( s.size() ) != g.n() )
     throw( std::invalid_argument( "the graph has a cycle" ) );
    return( owned( std::move( s ) ) );
    } ,
   "g"_a ,
   "The nodes in a topological order, by the algorithm of Kahn; "
   "ValueError if the graph has a cycle." );
 m.def(
   "has_cycle" ,
   []( const G & g ) {
    nb::gil_scoped_release r;
    return( ADS::hasCycle( g ) );
    } ,
   "g"_a , "Whether the graph has a cycle." );
 if constexpr( G::weighted ) {
  m.def(
    "shortest_paths" ,
    []( const G & g , int s , const std::string & how ) {
     checkNode( g , s );
     const ADS::Frontier f = frontier( how );
     ADS::ShortestPathTree< double > t;
     {
      nb::gil_scoped_release r;
      if( ( f != ADS::Frontier::QUEUE ) && negative( g ) )
       throw( std::invalid_argument(
	 "a negative weight: the method must be queue" ) );
      t = ADS::shortestPaths( g , s , f );
      }
     if( t.negativeCycle )
      throw( NegativeCycle( "a negative cycle is reached from the source" ) );
     return( std::make_pair( owned( std::move( t.d ) ) ,
			      owned( std::move( t.T ) ) ) );
     } ,
    "g"_a , "source"_a , "method"_a = "heap" ,
    "The distances from source (inf for the nodes not reached) and the "
    "parent of each node in the tree of the shortest paths (-1 for the "
    "source and the nodes not reached), by the prototype of the book with "
    "the structure named by method: heap (Johnson), list (Dijkstra), "
    "fibonacci (Fredman and Tarjan), queue (Bellman, Ford and Moore, the "
    "only one for negative weights), stack, deque (Pape and D'Esopo); "
    "NegativeCycle if a negative cycle is reached." );
  m.def(
    "delta_stepping" ,
    []( const G & g , int s , double delta , ADS::ThreadPool * p ) {
     checkNode( g , s );
     if( ! ( delta > 0 ) )
      throw( std::invalid_argument( "delta must be positive" ) );
     std::vector< double > d;
     {
      nb::gil_scoped_release r;
      if( negative( g ) )
       throw( std::invalid_argument( "a negative weight" ) );
      d = ADS::deltaStepping( pool( p ) , g , s , delta );
      }
     return( owned( std::move( d ) ) );
     } ,
    "g"_a , "source"_a , "delta"_a , "pool"_a = nb::none() ,
    "The distances from source (inf for the nodes not reached) by the "
    "delta-stepping of Meyer and Sanders, on the threads of the pool." );
  m.def(
    "rho_stepping" ,
    []( const G & g , int s , std::size_t rho , ADS::ThreadPool * p ) {
     checkNode( g , s );
     if( rho < 1 )
      throw( std::invalid_argument( "rho must be at least 1" ) );
     std::vector< double > d;
     {
      nb::gil_scoped_release r;
      if( negative( g ) )
       throw( std::invalid_argument( "a negative weight" ) );
      d = ADS::rhoStepping( pool( p ) , g , s , rho );
      }
     return( owned( std::move( d ) ) );
     } ,
    "g"_a , "source"_a , "rho"_a = std::size_t( 1 ) << 20 ,
    "pool"_a = nb::none() ,
    "The distances from source (inf for the nodes not reached) by the "
    "rho-stepping of Dong, Gu, Sun and Zhang, on the threads of the "
    "pool." );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE CLASSES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the class of the graph G, named \f$ s \f$
template< class G >
void bindGraph( nb::module_ & m , const char * s , const char * doc )
{
 nb::class_< G > c( m , s , doc );
 c.def( nb::init<>() , "A graph with no nodes." );
 if constexpr( G::weighted ) {
  c.def_static(
    "from_csr" ,
    []( const Input< int > & o , const Input< int > & h ,
	const Input< double > & w ,
	bool directed ) { return( fromCsr< G >( o , h , &w , directed ) ); } ,
    "offsets"_a , "heads"_a , "weights"_a , "directed"_a = true ,
    "The graph of the rows at offsets (n + 1 of them, from 0), with the "
    "heads and the weights of their arcs: one copy of each array, and a "
    "sort of the rows that are not sorted by head; an unoriented graph "
    "has each edge in the rows of both its ends." );
  c.def_static(
    "from_edges" ,
    []( int n , const Input< int > & s , const Input< int > & t ,
	const Input< double > & w , bool directed ) {
     return( fromEdges< G >( n , s , t , &w , directed ) );
     } ,
    "n"_a , "tails"_a , "heads"_a , "weights"_a , "directed"_a = true ,
    "The graph of n nodes and of the edges (tails[k], heads[k]) of "
    "weights[k], each once also if unoriented." );
  }
 else {
  c.def_static(
    "from_csr" ,
    []( const Input< int > & o , const Input< int > & h , bool directed ) {
     return( fromCsr< G >( o , h , nullptr , directed ) );
     } ,
    "offsets"_a , "heads"_a , "directed"_a = true ,
    "The graph of the rows at offsets (n + 1 of them, from 0), with the "
    "heads of their arcs: one copy of each array, and a sort of the rows "
    "that are not sorted by head; an unoriented graph has each edge in "
    "the rows of both its ends." );
  c.def_static(
    "from_edges" ,
    []( int n , const Input< int > & s , const Input< int > & t ,
	bool directed ) {
     return( fromEdges< G >( n , s , t , nullptr , directed ) );
     } ,
    "n"_a , "tails"_a , "heads"_a , "directed"_a = true ,
    "The graph of n nodes and of the edges (tails[k], heads[k]), each "
    "once also if unoriented." );
  }
 c.def_prop_ro( "n" , &G::n , "The number of nodes." )
   .def_prop_ro( "m" , &G::m , "The number of edges." )
   .def_prop_ro( "arcs" , &G::arcs ,
		 "The number of arcs, twice the edges if unoriented." )
   .def_prop_ro( "directed" , &G::directed , "Whether it is oriented." )
   .def( "__len__" , &G::n )
   .def(
     "degree" ,
     []( const G & g , int u ) {
      checkNode( g , u );
      return( g.degree( u ) );
      } ,
     "u"_a , "The arcs that leave u." )
   .def_prop_ro(
     "offsets" ,
     []( const G & g ) {
      return( view( g.offsets() , std::size_t( g.n() ) + 1 , nb::find( g ) ) );
      } ,
     "The start of the row of each node and the end of the last one, a "
     "view of the graph (no copy)." )
   .def_prop_ro(
     "heads" ,
     []( const G & g ) {
      return( headView( g , 0 , std::size_t( g.arcs() ) , nb::find( g ) ) );
      } ,
     "The heads of the arcs, row by row, a view of the graph (no copy; "
     "strided in the interleaved layout)." )
   .def(
     "neighbors" ,
     []( const G & g , int u ) {
      checkNode( g , u );
      const int k = g.offsets()[ u ];
      return( headView( g , std::size_t( k ) ,
			 std::size_t( g.offsets()[ u + 1 ] - k ) ,
			 nb::find( g ) ) );
      } ,
     "u"_a , "The heads of the arcs from u, sorted, a view (no copy)." )
   .def( "transpose" , &G::transpose ,
	 nb::call_guard< nb::gil_scoped_release >() ,
	 "The graph with each arc reversed." )
   .def(
     "relabel" ,
     []( const G & g , const Input< int > & p ) {
      const std::span< const int > s = items( p );
      if( s.size() != std::size_t( g.n() ) )
       throw( std::invalid_argument( "one new name for each node" ) );
      std::vector< bool > seen( s.size() , false );
      for( const int x : s ) {
       if( ( x < 0 ) || ( std::size_t( x ) >= s.size() ) || seen[ x ] )
	throw( std::invalid_argument( "not a permutation of the nodes" ) );
       seen[ x ] = true;
       }
      const std::vector< int > q( s.begin() , s.end() );
      nb::gil_scoped_release r;
      return( g.relabel( q ) );
      } ,
     "permutation"_a ,
     "The graph whose node permutation[u] is the node u of this one." )
   .def(
     "bfs_order" ,
     []( const G & g , int s ) {
      if( g.n() > 0 )
       checkNode( g , s );
      std::vector< int > o;
      {
       nb::gil_scoped_release r;
       o = ADS::bfsOrder( g , s );
       }
      return( owned( std::move( o ) ) );
      } ,
     "source"_a = 0 ,
     "A relabeling of the nodes in the order of a visit in breadth from "
     "source, the other components after it, for relabel(): the neighbors "
     "of a node then lie close in memory." )
   .def( "__repr__" , [ s ]( const G & g ) {
    return( std::string( s ) + "(n=" + std::to_string( g.n() ) +
	     ", m=" + std::to_string( g.m() ) +
	     ( g.directed() ? ", directed)" : ", undirected)" ) );
    } );
 if constexpr( G::weighted ) {
  c.def_prop_ro(
     "weights" ,
     []( const G & g ) {
      return( weightView( g , 0 , std::size_t( g.arcs() ) , nb::find( g ) ) );
      } ,
     "The weights of the arcs, in the order of heads, a view of the graph "
     "(no copy; strided in the interleaved layout)." )
    .def(
      "neighbor_weights" ,
      []( const G & g , int u ) {
       checkNode( g , u );
       const int k = g.offsets()[ u ];
       return( weightView( g , std::size_t( k ) ,
			    std::size_t( g.offsets()[ u + 1 ] - k ) ,
			    nb::find( g ) ) );
       } ,
      "u"_a , "The weights of the arcs from u, a view (no copy)." )
    .def(
      "weight" ,
      []( const G & g , int u , int v ) {
       checkNode( g , u );
       checkNode( g , v );
       return( g.weight( u , v ) );
       } ,
      "u"_a , "v"_a , "The weight of the arc (u, v), or None." )
    .def(
      "set_weight" ,
      []( G & g , int u , int v , double w ) {
       checkNode( g , u );
       checkNode( g , v );
       return( g.setWeight( u , v , w ) );
       } ,
      "u"_a , "v"_a , "w"_a ,
      "The weight of the arc (u, v) made w (of both arcs if unoriented); "
      "False if there is no such arc." );
  }
 bindPersistence< G >( c );
 bindAlgorithms< G >( m );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE AREA -------------------------------*/
/*--------------------------------------------------------------------------*/

void bindGraphs( nb::module_ & m )
{
 nb::exception< NegativeCycle >( m , "NegativeCycle" , PyExc_ValueError );
 nb::class_< ADS::ThreadPool >(
   m , "ThreadPool" ,
   "A pool of threads with work stealing, the caller included, on which "
   "the parallel algorithms run." )
   .def( nb::init< std::size_t >() , "threads"_a = 0 ,
	 "A pool of threads, one for each hardware thread if 0." )
   .def_prop_ro( "threads" , &ADS::ThreadPool::concurrency ,
		 "The number of threads, the caller included." );
 bindGraph< Plain >( m , "StaticGraph" ,
		     "A graph with no weights in compressed rows, static." );
 bindGraph< Separate >(
   m , "WeightedStaticGraph" ,
   "A graph with weights in compressed rows, static: the heads and the "
   "weights in two arrays." );
 bindGraph< Interleaved >(
   m , "InterleavedStaticGraph" ,
   "A graph with weights in compressed rows, static: each head and its "
   "weight side by side in one array, on the same line of the cache." );
 nb::class_< Matrix > c(
   m , "CsrMatrix" ,
   "A sparse matrix of doubles in compressed rows, the columns of 32 bits "
   "and the row starts of 64." );
 c.def( nb::init< std::size_t , std::size_t >() , "rows"_a = 0 , "cols"_a = 0 ,
	"A matrix of zeros." )
   .def_static(
     "from_arrays" ,
     [](
       std::size_t rows , std::size_t cols ,
       const Input< std::size_t > & indptr ,
       const Input< std::uint32_t > & indices ,
       const Input< double > & data ) {
      const auto p = items( indptr );
      const auto j = items( indices );
      const auto x = items( data );
      if( ( p.size() != rows + 1 ) || ( p[ 0 ] != 0 ) ||
	  ( p[ rows ] != j.size() ) || ( x.size() != j.size() ) )
       throw( std::invalid_argument( "not the rows of a CSR matrix" ) );
      nb::gil_scoped_release r;
      for( std::size_t i = 0 ; i < rows ; ++i ) {
       if( p[ i ] > p[ i + 1 ] )
	throw( std::invalid_argument( "the row starts must not decrease" ) );
       for( std::size_t k = p[ i ] ; k < p[ i + 1 ] ; ++k )
	if( ( j[ k ] >= cols ) ||
	    ( ( k > p[ i ] ) && ( j[ k - 1 ] >= j[ k ] ) ) )
	 throw( std::invalid_argument(
	   "the columns of a row must be increasing, within cols" ) );
       }
      ArrayArchive a( { ( long long )rows , ( long long )cols ,
			ArrayArchive::of( p.data() , p.size() ) ,
			ArrayArchive::of( j.data() , j.size() ) ,
			ArrayArchive::of( x.data() , x.size() ) } );
      Matrix M;
      M.persist( a );
      return( M );
      } ,
     "rows"_a , "cols"_a , "indptr"_a , "indices"_a , "data"_a ,
     "The matrix of the arrays of compressed rows (those of "
     "scipy.sparse.csr_array, the columns of each row increasing), one "
     "copy of each." )
   .def_prop_ro( "shape" ,
		 []( const Matrix & M ) {
		  return( std::make_tuple( M.rows() , M.cols() ) );
		  } )
   .def_prop_ro( "nnz" , &Matrix::nonzeros , "The nonzeros." )
   .def_prop_ro(
     "indptr" ,
     []( const Matrix & M ) {
      return( view( M.rowStart() , M.rows() + 1 , nb::find( M ) ) );
      } ,
     "The start of each row, a view of the matrix (no copy)." )
   .def_prop_ro(
     "indices" ,
     []( const Matrix & M ) {
      static const std::uint32_t none = 0;
      return( M.nonzeros() ? view( M.columns() , M.nonzeros() , nb::find( M ) )
			    : view( &none , 0 , nb::find( M ) ) );
      } ,
     "The column of each nonzero, a view of the matrix (no copy)." )
   .def_prop_ro(
     "data" ,
     []( const Matrix & M ) {
      static const double none = 0;
      return( M.nonzeros() ? view( M.values() , M.nonzeros() , nb::find( M ) )
			    : view( &none , 0 , nb::find( M ) ) );
      } ,
     "The value of each nonzero, a view of the matrix (no copy)." )
   .def(
     "get" ,
     []( const Matrix & M , std::size_t i , std::size_t j ) {
      if( ( i >= M.rows() ) || ( j >= M.cols() ) )
       throw( nb::index_error( "outside the matrix" ) );
      return( M.get( i , j ) );
      } ,
     "i"_a , "j"_a , "The entry (i, j)." )
   .def(
     "multiply" ,
     []( const Matrix & M , const Input< double > & x ) {
      if( x.shape( 0 ) != M.cols() )
       throw( std::invalid_argument( "x must have cols items" ) );
      std::vector< double > y( M.rows() );
      {
       nb::gil_scoped_release r;
       M.multiply( x.data() , y.data() );
       }
      return( owned( std::move( y ) ) );
      } ,
     "x"_a , "The product of the matrix by the vector x." )
   .def( "transpose" , &Matrix::transpose ,
	 nb::call_guard< nb::gil_scoped_release >() , "The transpose." );
 bindPersistence< Matrix >( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File graphs.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
