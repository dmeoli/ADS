/*--------------------------------------------------------------------------*/
/*---------------------------- File Observers.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The changes of a graph of abLEMON told to the observers of ADS, and a graph
 * of abLEMON kept in step with one of ADS, beyond the book (see
 * \ref theory_graph_lemon):
 *
 * - `LemonTap< GR , Item , CostMap , Policy >`, an observer of the alteration
 *   notifier of abLEMON for the nodes or for the arcs of `GR` (an
 *   `ObserverBase` of `GR::NodeNotifier` or of `GR::ArcNotifier`) that tells
 *   each change through the subject policy `Policy` of ADS [see
 *   `Modification.h`]: `Inserted` after `add()`, `Erased` before the item
 *   leaves (`erase()`), `Rebuilt` after `build()` and `Cleared` before
 *   `clear()`, the handle being the id of the item, so that the parallel arcs
 *   of abLEMON stay distinct, and the key of an arc its ends ( u , v ), read
 *   at both times since abLEMON tells an insertion after it and an erasure
 *   before it. With a `CostMap` the erasure of an arc carries its cost, read
 *   before abLEMON removes it, and the insertion the cost that the map holds
 *   at `add()`, which in the idiom of abLEMON,
 *   `a = g.addArc( u , v ); cost[ a ] = c;`, is still the default, so that a
 *   write that must be seen goes through an `ObservedMap` [see `Maps.h`]. An
 *   insertion is told with the strong guarantee of ADS [see `Modification.h`]:
 *   if an observer of ADS throws, those already told receive its erasure, and
 *   the exception reaches abLEMON, which removes the item again (LEMON 1.3
 *   leaves it in the graph). The vector forms of `add()` and `erase()` open a
 *   batch of ADS [see `ADS::batch()`], whose events are coalesced into their
 *   net effect per id. abLEMON calls its observers in an order that its header
 *   does not state, and that we read in the code of the notifier: the oldest
 *   first on `add()` and `build()`, the newest first on `erase()` and
 *   `clear()`; a tap that reads a map must therefore be constructed after that
 *   map, so that at `add()` the map has grown and at `erase()` the value is
 *   still there. The `erase()` and `clear()` of abLEMON must not throw, and an
 *   exception of an observer of ADS within them is caught and marks the tap
 *   `spoiled()`. The operations of abLEMON that the notifier does not tell
 *   (`changeTarget()`, `changeSource()`, `reverseArc()`, the moves of
 *   `contract()` and `split()`), the writes of the maps that bypass
 *   `ObservedMap`, and the changes through an adaptor of abLEMON are not seen.
 *   Its cost is that of one more map of abLEMON, a virtual call per item
 *   inserted or erased, and one test of the subject when nobody of ADS
 *   listens;
 * - `LemonMirror< G >`, an observer of an `ObservedGraph` of ADS [see
 *   `Observable.h`] that keeps a `lemon::ListDigraph` and its map of the
 *   weights in step with the graph \f$ g \f$ of ADS, in \f$ O( 1 ) \f$
 *   expected time per event (a vector from the names of ADS to the nodes of
 *   abLEMON, a hash table from the pairs ( u , v ) to the arcs; an unoriented
 *   edge is mirrored by its two arcs), and that records the kinds of the
 *   events since the last `clearKinds()` (nodes, arcs, weights, and whether
 *   a weight fell or an arc left), so that the caller knows whether an
 *   algorithm of abLEMON that keeps a state must be built again. The events
 *   of ADS name an edge by its ends, and the graphs of ADS have no parallel
 *   edges, so that the mirror has none either.
 *
 * The contracts [see `Contract.h`]: the graph observed outlives the tap and
 * the mirror; the mirror sees the graph as it is at its construction and
 * then every change through the wrapper. The classes are defined in this
 * file.
 *
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
 * \see E. Gamma, R. Helm, R. Johnson, J. Vlissides, Design Patterns:
 *      Elements of Reusable Object-Oriented Software, Addison-Wesley,
 *      1994, ISBN 0-201-63361-2, the pattern Observer
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

#ifndef __ADS_interop_lemon_Observers
 #define __ADS_interop_lemon_Observers
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <lemon/core.h> )
 #error "the interoperation with abLEMON needs the headers of abLEMON"
#endif

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Observable.h"

#include <lemon/bits/alteration_notifier.h>
#include <lemon/bits/traits.h>
#include <lemon/core.h>
#include <lemon/list_graph.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS LemonTap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the ends of an arc, the key of its events
struct Ends
{
 int u;  ///< the tail
 int v;  ///< the head
 /// whether the two pairs are the same
 bool operator==( const Ends & ) const = default;
 };

/// \cond
namespace detail
{

/// the type of the values of the map `M`, as `std::type_identity` gives it
template< class M >
struct ValueOf
{
 using type = typename M::Value;  ///< the values
 };

/// the events of a tap of the items `Item` of `GR`, with the costs of the
/// arcs read from `CostMap` (none if `void`)
template< class GR , class Item , class CostMap >
struct TapEvents
{
 static constexpr bool arcs = std::is_same_v< Item , typename GR::Arc >;
 static constexpr bool cost = arcs && ! std::is_void_v< CostMap >;
 using Key = std::conditional_t< arcs , Ends , int >;
 /// the value of the events, `void` without costs
 using V = typename std::conditional_t< cost , ValueOf< CostMap > ,
					std::type_identity< void > >::type;
 using Ins = mod::Inserted< int , Key , V >;
 using Era = mod::Erased< int , Key , V >;
 };

}  // end( namespace detail )
/// \endcond

/// the alteration notifier of abLEMON for the items `Item` of `GR` told as
/// events of ADS, through `Policy`
template< class GR , class Item = typename GR::Arc , class CostMap = void ,
	  class Policy = Dynamic<> >
class LemonTap
    : public ::lemon::ItemSetTraits< GR , Item >::ItemNotifier::ObserverBase
{

 using Notifier = typename ::lemon::ItemSetTraits< GR , Item >::ItemNotifier;
 using Base = typename Notifier::ObserverBase;
 using T = detail::TapEvents< GR , Item , CostMap >;

 static constexpr bool withCost = ! std::is_void_v< CostMap >;

/*--------------------------------------------------------------------------*/
 public:
 using Key = typename T::Key;     ///< the key of an item: its ends, or its id
 using Insert = typename T::Ins;  ///< an insertion
 using Erase = typename T::Era;   ///< an erasure
 /// the events
 using modifications = mod::list< Insert , Erase , mod::Cleared ,
				  mod::Rebuilt >;
 /// the subject
 using Subject = typename Policy::template bind< modifications , LemonTap >;

 /// the tap of the items of \f$ g \f$, the static observers `o...`;
 /// \f$ g \f$ must outlive it
 template< class... O >
 explicit LemonTap( const GR & g , O &... o )
  requires( ! withCost )
     : f_g( g ) , f_c( nullptr ) , f_sub( o... ) {
  Base::attach( g.notifier( Item() ) );
  }

 /// the tap of the arcs of \f$ g \f$ with the costs \f$ c \f$, the static
 /// observers `o...`; it must be constructed after \f$ c \f$ [see the
 /// order of the observers above], and both must outlive it
 template< class C , class... O >
 LemonTap( const GR & g , const C & c , O &... o )
  requires( withCost && std::is_same_v< C , CostMap > )
     : f_g( g ) , f_c( &c ) , f_sub( o... ) {
  Base::attach( g.notifier( Item() ) );
  }

 /// whether an observer of ADS threw within `erase()` or `clear()`, which
 /// abLEMON forbids: its state may no longer match the graph
 [[nodiscard]] bool spoiled( void ) const { return( f_spoiled ); }

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

/*--------------------------------------------------------------------------*/
 protected:
 /// `Inserted` after the insertion of \f$ x \f$
 void add( const Item & x ) override {
  if constexpr( Subject::template listened< Insert > )
   if( f_sub.template live< Insert >() )
    f_sub.emit( inserted( x ) , []( void ) noexcept { } );
  }

 /// the insertions of \f$ X \f$, in a batch
 void add( const std::vector< Item > & X ) override {
  if constexpr( Subject::template listened< Insert > )
   if( f_sub.template live< Insert >() ) {
    auto b = ADS::batch( *this );
    for( const Item & x : X )
     add( x );
    }
  }

 /// `Erased` before the removal of \f$ x \f$, with its cost
 void erase( const Item & x ) noexcept override {
  if constexpr( Subject::template listened< Erase > )
   if( f_sub.template live< Erase >() )
    try {
     f_sub.emit( erased( x ) );
     }
    catch( ... ) {
     f_spoiled = true;
     }
  }

 /// the erasures of \f$ X \f$, in a batch
 void erase( const std::vector< Item > & X ) noexcept override {
  if constexpr( Subject::template listened< Erase > )
   if( f_sub.template live< Erase >() )
    try {
     auto b = ADS::batch( *this );
     for( const Item & x : X )
      f_sub.emit( erased( x ) );
     }
    catch( ... ) {
     f_spoiled = true;
     }
  }

 /// `Rebuilt` after the items were built anew
 void build( void ) override {
  if( f_sub.template live< mod::Rebuilt >() )
   f_sub.emit( mod::Rebuilt{} );
  }

 /// `Cleared` before all the items leave
 void clear( void ) noexcept override {
  if( f_sub.template live< mod::Cleared >() )
   try {
    f_sub.emit( mod::Cleared{} );
    }
   catch( ... ) {
    f_spoiled = true;
    }
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the key of \f$ x \f$
 [[nodiscard]] Key key( const Item & x ) const {
  if constexpr( T::arcs )
   return( Key{ f_g.id( f_g.source( x ) ) , f_g.id( f_g.target( x ) ) } );
  else
   return( f_g.id( x ) );
  }

 /// the erasure of \f$ x \f$, its cost read now
 [[nodiscard]] Erase erased( const Item & x ) const {
  if constexpr( withCost && T::arcs )
   return( Erase{ f_g.id( x ) , key( x ) , ( *f_c )[ x ] } );
  else
   return( Erase{ f_g.id( x ) , key( x ) } );
  }

 /// the insertion of \f$ x \f$, with the cost in the map now
 [[nodiscard]] Insert inserted( const Item & x ) const {
  if constexpr( withCost && T::arcs )
   return( Insert{ f_g.id( x ) , key( x ) , ( *f_c )[ x ] } );
  else
   return( Insert{ f_g.id( x ) , key( x ) } );
  }

 /// the type of the costs read, or a placeholder
 using CM = std::conditional_t< withCost , CostMap , int >;

 const GR & f_g;                       ///< the graph
 const CM * f_c;                       ///< the costs, if any
 bool f_spoiled = false;               ///< whether an erasure threw
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers of ADS

 };  // end( class LemonTap )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS LemonMirror ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a `lemon::ListDigraph` and its weights kept in step with a graph of ADS,
/// as an observer of its `ObservedGraph`
template< Graph G >
class LemonMirror
{

/*--------------------------------------------------------------------------*/
 public:
 using Weight = typename G::Weight;       ///< the type of the weights
 using LG = ::lemon::ListDigraph;         ///< the digraph of abLEMON
 using WeightMap = LG::ArcMap< Weight >;  ///< its weights

 /// what the events since the last `clearKinds()` changed
 struct Kinds
 {
  bool nodes = false;    ///< a node inserted or deleted
  bool arcs = false;     ///< an arc inserted or deleted
  bool weights = false;  ///< a weight changed
  bool down = false;     ///< a weight fell, or an arc left
  /// whether anything changed
  [[nodiscard]] bool any( void ) const { return( nodes || arcs || weights ); }
  };

 /// the mirror of \f$ g \f$ as it is now, in \f$ O( n + m ) \f$ expected
 /// time
 explicit LemonMirror( const G & g );

 /// not copied: the maps of abLEMON are bound to their digraph
 LemonMirror( const LemonMirror & ) = delete;
 /// not assigned
 LemonMirror & operator=( const LemonMirror & ) = delete;

 /// the node \f$ u \f$ inserted
 void on( const NodeInserted & e );
 /// the node \f$ u \f$ deleted, after its edges
 void on( const NodeDeleted & e ) noexcept;
 /// the edge inserted, with its weight
 void on( const EdgeInserted< Weight > & e );
 /// the edge deleted
 void on( const EdgeDeleted< Weight > & e ) noexcept;
 /// the weight of an edge changed
 void on( const WeightChanged< Weight > & e );

 /// the digraph of abLEMON
 [[nodiscard]] const LG & digraph( void ) const { return( f_lg ); }

 /// the weights of its arcs
 [[nodiscard]] const WeightMap & weights( void ) const { return( f_w ); }

 /// the node of abLEMON of the name \f$ u \f$ of ADS, `INVALID` if none
 [[nodiscard]] LG::Node node( int u ) const {
  return( ( u >= 0 ) && ( std::size_t( u ) < f_node.size() )
	     ? f_node[ std::size_t( u ) ]
	     : LG::Node( ::lemon::INVALID ) );
  }

 /// the arc of abLEMON of the arc \f$ ( u , v ) \f$ of ADS, `INVALID` if
 /// none
 [[nodiscard]] LG::Arc arc( int u , int v ) const {
  const auto it = f_arc.find( key( u , v ) );
  return( it == f_arc.end() ? LG::Arc( ::lemon::INVALID ) : it->second );
  }

 /// the kinds of the events since the last `clearKinds()`
 [[nodiscard]] const Kinds & kinds( void ) const { return( f_kinds ); }

 /// whether something changed since the last `clearKinds()`
 [[nodiscard]] bool stale( void ) const { return( f_kinds.any() ); }

 /// the kinds forgotten, e.g., after an algorithm of abLEMON ran again
 void clearKinds( void ) { f_kinds = Kinds{}; }

/*--------------------------------------------------------------------------*/
 private:
 /// the key of \f$ ( u , v ) \f$
 static std::uint64_t key( int u , int v ) {
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
	   std::uint32_t( v ) );
  }

 /// the arc \f$ ( u , v ) \f$ of weight \f$ w \f$ put in the digraph
 void put( int u , int v , const Weight & w );

 /// the arc \f$ ( u , v ) \f$ taken out of the digraph, if there
 void drop( int u , int v ) noexcept;

 bool f_directed;                                     ///< as the graph
 LG f_lg;                                             ///< the digraph
 WeightMap f_w;                                       ///< its weights
 std::vector< LG::Node > f_node;                      ///< name to node
 std::unordered_map< std::uint64_t , LG::Arc > f_arc;  ///< pair to arc
 Kinds f_kinds;                                       ///< the changes

 };  // end( class LemonMirror )

template< Graph G >
LemonMirror< G >::LemonMirror( const G & g )
    : f_directed( g.directed() ) , f_w( f_lg )
{
 const int n = g.bound();
 f_node.assign( std::size_t( n ) , LG::Node( ::lemon::INVALID ) );
 for( int u = 0 ; u < n ; ++u )
  if( g.hasNode( u ) )
   f_node[ std::size_t( u ) ] = f_lg.addNode();
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
   if( f_directed || ( u < v ) )
    put( u , v , w );
   } );
 }

template< Graph G >
void LemonMirror< G >::on( const NodeInserted & e )
{
 if( f_node.size() <= std::size_t( e.u ) )
  f_node.resize( std::size_t( e.u ) + 1 , LG::Node( ::lemon::INVALID ) );
 f_node[ std::size_t( e.u ) ] = f_lg.addNode();
 f_kinds.nodes = true;
 }

template< Graph G >
void LemonMirror< G >::on( const NodeDeleted & e ) noexcept
{
 f_lg.erase( f_node[ std::size_t( e.u ) ] );
 f_node[ std::size_t( e.u ) ] = ::lemon::INVALID;
 f_kinds.nodes = true;
 }

template< Graph G >
void LemonMirror< G >::on( const EdgeInserted< Weight > & e )
{
 put( e.u , e.v , e.w );
 f_kinds.arcs = true;
 }

template< Graph G >
void LemonMirror< G >::on( const EdgeDeleted< Weight > & e ) noexcept
{
 drop( e.u , e.v );
 if( ! f_directed )
  drop( e.v , e.u );
 f_kinds.arcs = true;
 f_kinds.down = true;
 }

template< Graph G >
void LemonMirror< G >::on( const WeightChanged< Weight > & e )
{
 f_w.set( f_arc.at( key( e.u , e.v ) ) , e.to );
 if( ! f_directed )
  f_w.set( f_arc.at( key( e.v , e.u ) ) , e.to );
 f_kinds.weights = true;
 if( e.to < e.from )
  f_kinds.down = true;
 }

template< Graph G >
void LemonMirror< G >::put( int u , int v , const Weight & w )
{
 const LG::Arc a =
   f_lg.addArc( f_node[ std::size_t( u ) ] , f_node[ std::size_t( v ) ] );
 f_w.set( a , w );
 f_arc[ key( u , v ) ] = a;
 if( ! f_directed ) {
  const LG::Arc b =
    f_lg.addArc( f_node[ std::size_t( v ) ] , f_node[ std::size_t( u ) ] );
  f_w.set( b , w );
  f_arc[ key( v , u ) ] = b;
  }
 }

template< Graph G >
void LemonMirror< G >::drop( int u , int v ) noexcept
{
 const auto it = f_arc.find( key( u , v ) );
 if( it != f_arc.end() ) {
  f_lg.erase( it->second );
  f_arc.erase( it );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Observers.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Observers.h --------------------------*/
/*--------------------------------------------------------------------------*/
