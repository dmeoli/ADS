/*--------------------------------------------------------------------------*/
/*---------------------------- File dynamic.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graph that tells its changes, in the module `ads`: a `ListGraph` of
 * weights of type `double` behind `ObservedGraph`, whose typed events
 * (`NodeInserted`, `NodeDeleted`, `EdgeInserted`, `EdgeDeleted` and
 * `WeightChanged`) are recorded and read from Python as tuples, one by
 * one or, for the changes made in a batch, once each net change when the
 * outermost batch closes, as ADS coalesces them; a batch left by an
 * exception undoes its changes of the edges. The tree of the shortest
 * paths from a root can be kept by `ShortestPathsObserver`, which after
 * each change certifies, repairs or recomputes it at the next query.
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

#include "ADS/Modification.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/StaticGraph.h"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ads_python
{

using namespace nb::literals;

namespace
{

using LG = ADS::ListGraph< double >;
using Event = ADS::GraphEvent< double >;
using Observed = ADS::ObservedGraph< LG , ADS::Dynamic<> >;
using Tracker = ADS::ShortestPathsObserver< LG >;

/// the observer that keeps every event it is told
struct Recorder
{
 std::vector< Event > events;  ///< the events not yet read

 [[nodiscard]] unsigned mask( void ) const {
  return( ADS::GraphEvents::ALL );
  }

 void notify( const Event & e ) { events.push_back( e ); }

 void notify( std::span< const Event > s ) {
  events.insert( events.end() , s.begin() , s.end() );
  }
 };

/// the event \f$ e \f$ as a tuple of Python
nb::tuple event( const Event & e )
{
 return( std::visit(
   []( const auto & x ) -> nb::tuple {
    using X = std::decay_t< decltype( x ) >;
    if constexpr( std::is_same_v< X , ADS::NodeInserted > )
     return( nb::make_tuple( "node_inserted" , x.u ) );
    else if constexpr( std::is_same_v< X , ADS::NodeDeleted > )
     return( nb::make_tuple( "node_deleted" , x.u ) );
    else if constexpr( std::is_same_v< X , ADS::EdgeInserted< double > > )
     return( nb::make_tuple( "edge_inserted" , x.u , x.v , x.w ) );
    else if constexpr( std::is_same_v< X , ADS::EdgeDeleted< double > > )
     return( nb::make_tuple( "edge_deleted" , x.u , x.v , x.w ) );
    else
     return( nb::make_tuple( "weight_changed" , x.u , x.v , x.from , x.to ) );
    } ,
   e ) );
 }

/// the exception that leaves a batch by unwinding
struct Unwind
{ };

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DynamicGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph of the module, its observers and its open batches
class DynamicGraph
{
 public:
 DynamicGraph( int n , bool directed ) : f_g( n , directed ) , f_o( f_g ) {
  f_o.attach( f_r );
  }

 DynamicGraph( const DynamicGraph & ) = delete;
 DynamicGraph & operator=( const DynamicGraph & ) = delete;

 ~DynamicGraph() {
  try {
   while( ! f_b.empty() )
    f_b.pop_back();
   }
  catch( ... ) {
   }
  }

 [[nodiscard]] const LG & graph( void ) const { return( f_g ); }

 [[nodiscard]] bool directed( void ) const { return( f_g.directed() ); }

 [[nodiscard]] int n( void ) const { return( f_g.n() ); }

 [[nodiscard]] int bound( void ) const { return( f_g.bound() ); }

 void node( int u ) const {
  if( ! f_g.hasNode( u ) )
   throw( nb::index_error( "not a node of the graph" ) );
  }

 int insertNode( void ) { return( f_o.insertNode() ); }

 void deleteNode( int u ) {
  node( u );
  if( f_t && ( u == f_root ) )
   throw( std::invalid_argument( "the root of the shortest paths" ) );
  f_o.deleteNode( u );
  }

 void insertEdge( int u , int v , double w ) {
  node( u );
  node( v );
  if( u == v )
   throw( std::invalid_argument( "a loop" ) );
  f_o.insertEdge( u , v , w );
  }

 void deleteEdge( int u , int v ) {
  node( u );
  node( v );
  if( ! f_g.weight( u , v ) )
   throw( nb::key_error( "no such edge" ) );
  f_o.deleteEdge( u , v );
  }

 bool setWeight( int u , int v , double w ) {
  node( u );
  node( v );
  return( f_o.setWeight( u , v , w ) );
  }

 [[nodiscard]] std::optional< double > weight( int u , int v ) const {
  node( u );
  node( v );
  return( f_g.weight( u , v ) );
  }

 void open( void ) { f_b.push_back( f_o.batch() ); }

 void close( bool undo ) {
  if( f_b.empty() )
   throw( std::logic_error( "no batch is open" ) );
  if( ! undo ) {
   Observed::Batch b = std::move( f_b.back() );
   f_b.pop_back();
   b.close();
   return;
   }
  // the batch destroyed while an exception unwinds the stack, so that it
  // undoes its changes where it can, and else tells them, as it would
  // in C++ when the block of the batch throws
  try {
   Observed::Batch b = std::move( f_b.back() );
   f_b.pop_back();
   throw( Unwind() );
   }
  catch( const Unwind & ) {
   }
  }

 [[nodiscard]] std::size_t open_batches( void ) const {
  return( f_b.size() );
  }

 nb::list events( void ) {
  nb::list l;
  for( const Event & e : f_r.events )
   l.append( event( e ) );
  f_r.events.clear();
  return( l );
  }

 void track( int r ) {
  node( r );
  if( ! f_b.empty() )
   throw( std::logic_error( "not within a batch" ) );
  untrack();
  f_t = std::make_unique< Tracker >( f_g , r );
  f_tid = f_o.attach( *f_t );
  f_root = r;
  }

 void untrack( void ) {
  if( f_t ) {
   if( ! f_b.empty() )
    throw( std::logic_error( "not within a batch" ) );
   f_o.detach( f_tid );
   f_t.reset();
   }
  }

 Tracker & tracker( void ) {
  if( ! f_t )
   throw( std::logic_error( "no shortest paths are kept: call track()" ) );
  return( *f_t );
  }

 private:
 LG f_g;                              ///< the graph
 Recorder f_r;                        ///< the events
 Observed f_o;                        ///< the graph observed
 std::vector< Observed::Batch > f_b;  ///< the open batches
 std::unique_ptr< Tracker > f_t;      ///< the shortest paths, if kept
 int f_tid = -1;                      ///< their id among the observers
 int f_root = -1;                     ///< and their root
 };  // end( class DynamicGraph )

/// the scope of a batch, for `with`
struct BatchScope
{
 DynamicGraph * g;  ///< the graph
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE AREA -------------------------------*/
/*--------------------------------------------------------------------------*/

void bindDynamic( nb::module_ & m )
{
 nb::class_< BatchScope >( m , "_BatchScope" )
   .def(
     "__enter__" ,
     []( BatchScope & s ) {
      s.g->open();
      return( s.g );
      } ,
     nb::rv_policy::reference )
   .def(
     "__exit__" ,
     []( BatchScope & s , nb::handle t , nb::handle , nb::handle ) {
      s.g->close( ! t.is_none() );
      return( false );
      } ,
     nb::arg().none() , nb::arg().none() , nb::arg().none() );
 nb::class_< DynamicGraph >(
   m , "DynamicGraph" ,
   "A graph with weights that tells its changes: each change is an event "
   "of ADS, read by events() as a tuple (node_inserted, u), (node_deleted, "
   "u), (edge_inserted, u, v, w), (edge_deleted, u, v, w) or "
   "(weight_changed, u, v, before, after); the changes made within "
   "`with g.batch():` are told once each net change when the outermost "
   "batch closes, and undone if the block raises." )
   .def( nb::init< int , bool >() , "n"_a = 0 , "directed"_a = true ,
	 "A graph of n nodes and no edges." )
   .def_prop_ro( "n" , &DynamicGraph::n , "The number of nodes." )
   .def_prop_ro( "bound" , &DynamicGraph::bound ,
		 "One more than the greatest name of a node." )
   .def_prop_ro( "directed" , &DynamicGraph::directed )
   .def( "add_node" , &DynamicGraph::insertNode , "Inserts a node; its name." )
   .def( "remove_node" , &DynamicGraph::deleteNode , "u"_a ,
	 "Deletes the node u and its edges, each deletion told first." )
   .def( "add_edge" , &DynamicGraph::insertEdge , "u"_a , "v"_a , "w"_a = 1.0 ,
	 "Inserts the edge (u, v) of weight w, or changes its weight if it "
	 "is there." )
   .def( "remove_edge" , &DynamicGraph::deleteEdge , "u"_a , "v"_a ,
	 "Deletes the edge (u, v)." )
   .def( "set_weight" , &DynamicGraph::setWeight , "u"_a , "v"_a , "w"_a ,
	 "Changes the weight of the edge (u, v); False if there is none." )
   .def( "weight" , &DynamicGraph::weight , "u"_a , "v"_a ,
	 "The weight of the edge (u, v), or None." )
   .def(
     "batch" , []( DynamicGraph & g ) { return( BatchScope{ &g } ); } ,
     nb::keep_alive< 0 , 1 >() ,
     "A batch, for `with`: its events are coalesced into their net effect "
     "per edge and told when the outermost one closes." )
   .def_prop_ro( "open_batches" , &DynamicGraph::open_batches )
   .def( "events" , &DynamicGraph::events ,
	 "The events told since the last call, in order." )
   .def(
     "to_static" ,
     []( const DynamicGraph & g ) {
      using S = ADS::StaticGraph< double >;
      nb::gil_scoped_release r;
      return( S( g.graph() ) );
      } ,
     "The graph as a WeightedStaticGraph (nodes 0 to bound - 1)." )
   .def( "track_shortest_paths" , &DynamicGraph::track , "root"_a ,
	 "Keeps the tree of the shortest paths from root up to date: after "
	 "each change it is certified, repaired or recomputed at the next "
	 "query." )
   .def( "untrack_shortest_paths" , &DynamicGraph::untrack )
   .def(
     "distance" ,
     []( DynamicGraph & g , int v ) {
      g.node( v );
      return( g.tracker().distance( v ) );
      } ,
     "v"_a , "The distance of v from the root (inf if not reached)." )
   .def(
     "distances" ,
     []( DynamicGraph & g ) {
      std::vector< double > d = g.tracker().tree().d;
      return( owned( std::move( d ) ) );
      } ,
     "The distances from the root of each node." )
   .def(
     "parents" ,
     []( DynamicGraph & g ) {
      std::vector< int > p = g.tracker().tree().T;
      return( owned( std::move( p ) ) );
      } ,
     "The parent of each node in the tree, -1 for the root and the nodes "
     "not reached." )
   .def(
     "reoptimization" ,
     []( DynamicGraph & g ) {
      Tracker & t = g.tracker();
      t.solve();
      switch( t.outcome() ) {
       case ADS::Reopt::CERTIFIED:
	return( "certified" );
       case ADS::Reopt::WARM:
	return( "repaired" );
       default:
	return( "recomputed" );
       }
      } ,
     "How the last update of the shortest paths was made: certified (no "
     "work), repaired, or recomputed from scratch." );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File dynamic.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
