/*--------------------------------------------------------------------------*/
/*--------------------------- File Observable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A GRAPH that tells its changes, beyond the book: the wrapper
 * `Observable< G , O... >` routes the changes of a graph of any model
 * (`ListGraph` and `MatrixGraph`, and the weights of `ArrayGraph` and
 * `StaticGraph`) and turns each of them into an event, a value of the
 * `std::variant` `GraphEvent< W >`, which it sends to the observers that
 * want its kind (see \ref theory_graph_dynamic):
 *
 * - `NodeInserted` and `NodeDeleted`, by `insertNode()` and `deleteNode()`;
 * - `EdgeInserted` and `EdgeDeleted`, by `insertEdge()` of an edge that is
 *   not there and by `deleteEdge()`, with the weight that the edge gets or
 *   had;
 * - `WeightChanged`, by `setWeight()`, or by `insertEdge()` of an edge that
 *   is already there (which, in `ListGraph` and `MatrixGraph`, only
 *   replaces its weight).
 *
 * `deleteNode( u )` first sends the deletion of each edge of \f$ u \f$, so
 * that an observer never sees an edge whose end is gone, and in an
 * unoriented graph the ends of an edge in an event are in increasing order,
 * \f$ u < v \f$.
 *
 * The wrapper is `ObservedGraph< G , Policy >`, whose subject is a policy
 * of `Modification.h` (`NoObservers` by default, so that a change compiles
 * to the code of the graph), and whose events are the list
 * `GraphModifications< W >`, and which forwards the queries of
 * `Capabilities.h` to the graph; an observer of it is any object with an
 * `on( e )` for one of the five events (see `Modification.h` for the
 * optional parts). `Observable< G , O... >` is the wrapper of the
 * observers of the first form, which have `mask()`, the set of the kinds of
 * event that they want as a mask of bits (see `GraphEvents`), and
 * `notify( e )` for one event and `notify( s )` for a `std::span` of them
 * (the concept `GraphObserver`): it is `ObservedGraph` with the policy
 * `Both< Static< O... > , Dynamic<> >`, the observers of the first form
 * called through an adapter. Those given to the constructor are static,
 * kept in a `std::tuple` and called directly, with no indirection; those
 * attached later by `attach()` are dynamic, kept as a pointer and pointers
 * to functions that restore their type (the erasure of the type with no
 * allocation), and they can leave by `detach()`. The design follows four
 * rules: (i) no event is built when no one listens, since a change of kind
 * \f$ k \f$ tests whether someone wants \f$ k \f$ (at compile time, then in
 * the union \f$ M \f$ of the masks) before it reads anything that only the
 * event needs (the old weight of an edge, the edges of a node to delete);
 * (ii) each observer gets only the kinds in its mask, also within a batch;
 * (iii) `batch()` opens a channel of RAII, and the events of the channels,
 * which nest, are collected and sent as one span, coalesced into their net
 * effect per edge by (8) of \ref theory_graph_dynamic, when the outermost
 * one closes; (iv) an observer may defer its work to the next query, as
 * those of `Reoptimization.h` do, and an observer with a recompute policy
 * may get `Rebuilt` instead of a batch [see `mod::with_policy`]. The changes
 * of the edges have their inverses, so that `UndoLog` undoes them through
 * `apply()`, and the wrapper with observers is undoable within its
 * batches (`checkpoint()`, `rollback()`): a batch that exits by an
 * exception undoes its changes of the edges, and the graph and the
 * observers are as before it, when every kind is listened to and no node
 * was inserted or deleted (otherwise its events are sent, as on a
 * structure that cannot be undone). A change sent at once costs
 * \f$ O( 1 ) \f$ per observer that wants it, and a batch of \f$ k \f$
 * events costs \f$ O( k ) \f$ expected time to coalesce, plus
 * \f$ O( k ) \f$ for each observer that does not want all its kinds (a
 * filtered copy).
 *
 * The dynamic algorithms of the literature take a sequence of updates (see
 * the survey of Hanauer, Henzinger and Schulz), and a library that has them
 * (e.g., the dynamic algorithms of NetworKit, whose `update()` and
 * `updateBatch()` the caller feeds after changing the graph) leaves it to
 * the caller to tell each of them what changed; here the wrapper does it,
 * with the sets of interest of an event service and the coalescing of the
 * batches of Acar et al. and of Tseng, Dhulipala and Shun.
 *
 * The contracts [see `Contract.h`]: the changes of the graph go through the
 * wrapper (a change made on the graph directly is not seen); an observer is
 * neither attached nor detached while a channel is open, nor from within a
 * `notify()`; `close()` is idempotent. The operations are defined in
 * `Observable.tpp`.
 *
 * \see E. Gamma, R. Helm, R. Johnson, J. Vlissides, Design Patterns:
 *      Elements of Reusable Object-Oriented Software, Addison-Wesley,
 *      1994, ISBN 0-201-63361-2, the pattern Observer
 * \see G. Ramalingam, T. Reps, On the computational complexity of dynamic
 *      graph problems, Theoretical Computer Science 158(1-2), 233-277,
 *      1996, doi:10.1016/0304-3975(95)00079-8
 * \see U. A. Acar, D. Anderson, G. E. Blelloch, L. Dhulipala, Parallel
 *      batch-dynamic graph connectivity, Proceedings of SPAA, 381-392,
 *      2019, doi:10.1145/3323165.3323196
 * \see T. Tseng, L. Dhulipala, J. Shun, Parallel batch-dynamic minimum
 *      spanning forest and the efficiency of dynamic agglomerative graph
 *      clustering, Proceedings of SPAA, 233-245, 2022,
 *      doi:10.1145/3490148.3538584
 * \see K. Hanauer, M. Henzinger, C. Schulz, Recent advances in fully
 *      dynamic graph algorithms - a quick reference guide, ACM Journal of
 *      Experimental Algorithmics 27, 1-45, 2022, doi:10.1145/3555806
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

#ifndef __ADS_Observable
 #define __ADS_Observable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Capabilities.h"
#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"
#include "ADS/graph/Graph.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- EVENTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a node inserted
struct NodeInserted {
 static constexpr unsigned effects = mod::effect::grows;   ///< it grows
 int u;                                          ///< the node
 /// whether the two events are the same
 bool operator==( const NodeInserted & ) const = default;
 };

/// a node deleted, after the deletion of its edges
struct NodeDeleted {
 static constexpr unsigned effects = mod::effect::shrinks;   ///< it shrinks
 int u;                                          ///< the node
 /// whether the two events are the same
 bool operator==( const NodeDeleted & ) const = default;
 };

/// an edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$ with \f$ u < v \f$,
/// inserted with the weight \f$ w \f$
template< class W >
struct EdgeInserted {
 static constexpr unsigned effects = mod::effect::grows;   ///< it grows
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W w;                                            ///< the weight
 /// whether the two events are the same
 bool operator==( const EdgeInserted & ) const = default;
 };

/// an edge deleted, \f$ w \f$ the weight that it had
template< class W >
struct EdgeDeleted {
 static constexpr unsigned effects = mod::effect::shrinks;   ///< it shrinks
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W w;                                            ///< the weight it had
 /// whether the two events are the same
 bool operator==( const EdgeDeleted & ) const = default;
 };

/// the weight of an edge changed from `from` to `to`
template< class W >
struct WeightChanged {
 static constexpr unsigned effects = mod::effect::up | mod::effect::down;
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W from;                                         ///< the weight before
 W to;                                           ///< the weight after
 /// whether the two events are the same
 bool operator==( const WeightChanged & ) const = default;
 };

/// the inverse of an insertion of an edge, its deletion
template< class W >
[[nodiscard]] constexpr EdgeDeleted< W > inverse( const EdgeInserted< W > & e )
{
 return( EdgeDeleted< W >{ e.u , e.v , e.w } );
 }

/// the inverse of a deletion of an edge, its insertion with the weight it
/// had
template< class W >
[[nodiscard]] constexpr EdgeInserted< W > inverse( const EdgeDeleted< W > & e )
{
 return( EdgeInserted< W >{ e.u , e.v , e.w } );
 }

/// the inverse of a change of a weight, the change back
template< class W >
[[nodiscard]] constexpr WeightChanged< W > inverse(
                                               const WeightChanged< W > & e )
{
 return( WeightChanged< W >{ e.u , e.v , e.to , e.from } );
 }

/// the events of a graph of weights `W`, in the order of their kinds
template< class W >
using GraphModifications = mod::list< NodeInserted , NodeDeleted ,
                                      EdgeInserted< W > , EdgeDeleted< W > ,
                                      WeightChanged< W > >;

/// an event, whose kind is its index in the variant, the variant of
/// `GraphModifications< W >`
template< class W >
using GraphEvent = std::variant< NodeInserted , NodeDeleted ,
                                 EdgeInserted< W > , EdgeDeleted< W > ,
                                 WeightChanged< W > >;

/// the kinds of event as bits of a mask
struct GraphEvents {
 static constexpr unsigned NODE_INSERT = 1u;    ///< `NodeInserted`
 static constexpr unsigned NODE_DELETE = 2u;    ///< `NodeDeleted`
 static constexpr unsigned EDGE_INSERT = 4u;    ///< `EdgeInserted`
 static constexpr unsigned EDGE_DELETE = 8u;    ///< `EdgeDeleted`
 static constexpr unsigned WEIGHT = 16u;        ///< `WeightChanged`
 static constexpr unsigned EDGES = 28u;         ///< the three on edges
 static constexpr unsigned ALL = 31u;           ///< all of them
 };

/// the bit of the kind of \f$ e \f$
template< class W >
[[nodiscard]] constexpr unsigned kindOf( const GraphEvent< W > & e ) {
 return( 1u << e.index() );
 }

/// the ends of the edge of \f$ e \f$, \f$ ( -1 , -1 ) \f$ for an event
/// on a node
template< class W >
[[nodiscard]] std::pair< int , int > endsOf( const GraphEvent< W > & e );

/// the net effect of the events \f$ E \f$, i.e., the nodes inserted, the
/// edges in the order of their first event and the nodes deleted, by (8) of
/// \ref theory_graph_dynamic, in \f$ O( |E| ) \f$ expected time
///
/// In pseudocode (beyond the book; it is correct by Lemma 6 of
/// \ref theory_graph_dynamic):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{coalesce}( E ) \cr
/// 2 & \quad I \gets \emptyset;\ D \gets \emptyset;\ N \gets \text{an empty}
///     \text{ hash table on the edges} \cr
/// 3 & \quad \textbf{foreach}\ e \in E\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ e\ \text{inserts the node } u\ \textbf{then}
///     \ \text{append } u \text{ to } I \cr
/// 5 & \qquad \textbf{else if}\ e\ \text{deletes the node } u\ \textbf{then}
///     \ \textbf{if}\ u \in I\ \textbf{then}\ \text{mark } u
///     \text{ gone}\ \textbf{else}\ \text{append } u \text{ to } D \cr
/// 6 & \qquad \textbf{else if}\ \text{its edge } ( u , v ) \notin N
///     \ \textbf{then}\ N[ ( u , v ) ] \gets ( s_0 , s_1 )
///     \text{ of } e \cr
/// 7 & \qquad \textbf{else}\ \text{the } s_1 \text{ of } N[ ( u , v ) ]
///     \gets \text{the } s_1 \text{ of } e \cr
/// 8 & \quad \textbf{return}\ \text{the nodes of } I \text{ not gone, the}
///     \text{ events (8) of } N \text{ on no node gone, in order, then } D
/// \end{array}}
/// \f]
template< class W >
[[nodiscard]] std::vector< GraphEvent< W > > coalesce(
                               std::span< const GraphEvent< W > > E );

/// an observer of the events of weights `W`: the kinds that it wants, and
/// the two ways of getting them
template< class O , class W >
concept GraphObserver = requires( O & o , const GraphEvent< W > & e ,
                                  std::span< const GraphEvent< W > > s ) {
 { o.mask() } -> std::convertible_to< unsigned >;
 o.notify( e );
 o.notify( s );
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE COALESCING ------------------------------*/
/*--------------------------------------------------------------------------*/

namespace mod {

/// the net effect of a batch of a graph, by coalesce() [see Observable.h]
template< class W >
struct coalesce_traits< GraphModifications< W > > {
 /// `coalesce( B )`
 static std::vector< GraphEvent< W > > coalesce(
                                      std::span< const GraphEvent< W > > B ) {
  return( ADS::coalesce< W >( B ) );
  }
 };

}  // end( namespace mod )

/// \cond
namespace detail {

/// the adapter of an observer of the first form, with `mask()` and
/// `notify()` [see GraphObserver]
template< class W >
struct GraphNotify {
 /// `o.notify( e )`
 template< class O , class E >
  requires( mod::contains_v< E , GraphModifications< W > > &&
            requires( O & o , const GraphEvent< W > & e ) { o.notify( e ); } )
 static void one( O & o , const E & e ) {
  const GraphEvent< W > x( e );
  o.notify( x );
  }
 /// `o.notify( s )`
 template< class O >
  requires requires( O & o , std::span< const GraphEvent< W > > s ) {
   o.notify( s ); }
 static void many( O & o , std::span< const GraphEvent< W > > s ) {
  o.notify( s );
  }
 /// `o.mask()`
 template< class O >
 static std::uint64_t mask( const O & o ) {
  return( std::uint64_t( unsigned( o.mask() ) ) );
  }
 /// whether the policy of \f$ o \f$ wants `Rebuilt` instead of
 /// \f$ k \f$ events [see `mod::with_policy`]
 template< class O >
  requires( mod::with_policy< O > && mod::observes< O , mod::Rebuilt > )
 static bool early( O & o , std::size_t k ) {
  return( mod::Direct::early( o , k ) );
  }
 /// `o.on( Rebuilt{} )`
 template< class O >
  requires( mod::with_policy< O > && mod::observes< O , mod::Rebuilt > )
 static void rebuilt( O & o ) noexcept { mod::Direct::rebuilt( o ); }
 };

}  // end( namespace detail )
/// \endcond

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ObservedGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph \f$ g \f$, whose changes are sent through the subject policy
/// `Policy` [see Modification.h]
template< Graph G , class Policy = NoObservers >
class ObservedGraph {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;    ///< the type of the weights
 using Event = GraphEvent< Weight >;   ///< the type of the events
 /// the events
 using modifications = GraphModifications< Weight >;
 /// the subject
 using Subject = typename Policy::template bind< modifications , G >;
 /// the channel of a batch, open while it lives [see batch()]
 using Batch = ADS::Batch< ObservedGraph >;

 /// the wrapper of \f$ g \f$, the static observers `o...`; \f$ g \f$
 /// must stay alive, and change only through the wrapper
 template< class ... O >
 explicit ObservedGraph( G & g , O & ... o ) : f_g( g ) , f_sub( o... ) {}

 /// not copied
 ObservedGraph( const ObservedGraph & ) = delete;
 /// not assigned
 ObservedGraph & operator=( const ObservedGraph & ) = delete;

 /// the graph
 [[nodiscard]] const G & graph( void ) const { return( f_g ); }

 /// the answer to the forwarded query of the tag `T` [see
 /// `Capabilities.h`]: that of the graph, but for the events, which are
 /// those of the wrapper
 template< forwarding_query_tag T >
 [[nodiscard]] static constexpr auto query( T ) noexcept {
  if constexpr( std::same_as< T , event_set_t > )
   return( std::type_identity< modifications >() );
  else
   return( query_v< G , T > );
  }

 /// the union of the masks of the observers
 [[nodiscard]] unsigned mask( void ) const {
  return( unsigned( f_sub.mask() ) );
  }

 /// whether anyone listens to one of the kinds \f$ k \f$
 [[nodiscard]] bool listening( unsigned k = GraphEvents::ALL ) const {
  return( ( mask() & k ) != 0 );
  }

 /// the events built so far, sent or collected (none when no one listens)
 [[nodiscard]] std::size_t built( void ) const { return( f_sub.built() ); }

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }
 /// what the audits of the observers read: the graph
 [[nodiscard]] const G & observed( void ) const { return( f_g ); }

/*---------------------------- THE GRAPH -----------------------------------*/

 /// whether the graph is oriented
 [[nodiscard]] bool directed( void ) const { return( f_g.directed() ); }
 /// the number of nodes
 [[nodiscard]] int n( void ) const { return( f_g.n() ); }
 /// a bound of the numbers of the nodes
 [[nodiscard]] int bound( void ) const { return( f_g.bound() ); }
 /// whether \f$ u \f$ is a node
 [[nodiscard]] bool hasNode( int u ) const { return( f_g.hasNode( u ) ); }
 /// the nodes
 [[nodiscard]] std::vector< int > V( void ) const { return( f_g.V() ); }
 /// the nodes adjacent to \f$ u \f$
 [[nodiscard]] std::vector< int > adj( int u ) const {
  return( f_g.adj( u ) );
  }
 /// `f( v )` for each node \f$ v \f$ adjacent to \f$ u \f$
 template< class F >
 void forEachAdj( int u , F && f ) const {
  f_g.forEachAdj( u , std::forward< F >( f ) );
  }
 /// `f( v , w )` for each edge \f$ ( u , v ) \f$ of weight \f$ w \f$
 template< class F >
 void forEachEdge( int u , F && f ) const {
  f_g.forEachEdge( u , std::forward< F >( f ) );
  }
 /// the weight of \f$ ( u , v ) \f$, nothing if there is no such edge
 [[nodiscard]] std::optional< Weight > weight( int u , int v ) const {
  return( f_g.weight( u , v ) );
  }

/*---------------------------- THE CHANGES ---------------------------------*/

 /// `insertNode()` of the graph, and `NodeInserted`
 int insertNode( void ) requires DynamicGraph< G > {
  const int u = f_g.insertNode();
  if constexpr( Subject::template listened< NodeInserted > )
   if( f_sub.template live< NodeInserted >() ) [[unlikely]]
    tell( NodeInserted{ u } );
  return( u );
  }

 /// `insertEdge()` of the graph, and `EdgeInserted`, or `WeightChanged` if
 /// the edge was there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 void insertEdge( int u , int v , const Weight & w = Weight() )
  requires DynamicGraph< G > {
  if constexpr( Subject::template listened< EdgeInserted< Weight > > ||
                Subject::template listened< WeightChanged< Weight > > )
   if( f_sub.template live< EdgeInserted< Weight > >() ||
       f_sub.template live< WeightChanged< Weight > >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     insertEdgeCold( u , v , w );
    else
     insertEdgeTold( u , v , w );
    return;
    }
  f_g.insertEdge( u , v , w );
  }

 /// `deleteEdge()` of the graph, and `EdgeDeleted` if the edge was there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 void deleteEdge( int u , int v ) requires DynamicGraph< G > {
  if constexpr( Subject::template listened< EdgeDeleted< Weight > > )
   if( f_sub.template live< EdgeDeleted< Weight > >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     deleteEdgeCold( u , v );
    else
     deleteEdgeTold( u , v );
    return;
    }
  f_g.deleteEdge( u , v );
  }

 /// `EdgeDeleted` for each edge of \f$ u \f$, `deleteNode()` of the graph,
 /// and `NodeDeleted`
 /// \pre \f$ u \f$ is a node
 void deleteNode( int u ) requires DynamicGraph< G >;

 /// the weight of the edge \f$ ( u , v ) \f$ set to \f$ w \f$, and
 /// `WeightChanged`; false, and nothing, if there is no such edge
 ///
 /// In pseudocode (beyond the book), the pattern of every change, with
 /// \f$ k \f$ the kind of its event, \f$ M \f$ the union of the masks,
 /// \f$ d \f$ the depth of the channels and \f$ B \f$ the events collected:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{setWeight}( u , v , w ) \cr
 /// 2 & \quad \textbf{if}\ k \notin M\ \textbf{then}\ \text{change } g;
 ///     \ \textbf{return} \cr
 /// 3 & \quad w_0 \gets \text{the weight of } ( u , v ) \text{ in } g;
 ///     \ \text{change } g \cr
 /// 4 & \quad e \gets \mathrm{WeightChanged}( u , v , w_0 , w ) \cr
 /// 5 & \quad \textbf{if}\ d > 0\ \textbf{then}\ \text{append } e
 ///     \text{ to } B \cr
 /// 6 & \quad \textbf{else foreach}\ \text{observer } o\ \textbf{with}
 ///     \ k \in M_o\ \textbf{do}\ o.\mathrm{notify}( e )
 /// \end{array}}
 /// \f]
 /// The test of line 2 is false at compile time when nobody may listen
 /// (`NoObservers`, or static observers that do not want \f$ k \f$), and
 /// the change is then the code of the graph.
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 bool setWeight( int u , int v , const Weight & w ) {
  ADS_PRE( f_g.hasNode( u ) && f_g.hasNode( v ) );
  if constexpr( Subject::template listened< WeightChanged< Weight > > )
   if( f_sub.template live< WeightChanged< Weight > >() ) [[unlikely]] {
    if constexpr( Subject::dynamic )
     return( setWeightCold( u , v , w ) );
    else
     return( setWeightTold( u , v , w ) );
    }
  if constexpr( requires { f_g.setWeight( u , v , w ); } )
   return( f_g.setWeight( u , v , w ) );
  else {
   if( ! f_g.weight( u , v ) )
    return( false );
   f_g.insertEdge( u , v , w );         // only the weight changes
   return( true );
   }
  }

/*---------------------------- THE UNDO ------------------------------------*/

 /// the insertion of the edge of \f$ e \f$, for `UndoLog`
 void apply( const EdgeInserted< Weight > & e ) requires DynamicGraph< G > {
  insertEdge( e.u , e.v , e.w );
  }
 /// the deletion of the edge of \f$ e \f$, for `UndoLog`
 void apply( const EdgeDeleted< Weight > & e ) requires DynamicGraph< G > {
  deleteEdge( e.u , e.v );
  }
 /// the change of the weight of \f$ e \f$, for `UndoLog`
 void apply( const WeightChanged< Weight > & e ) {
  setWeight( e.u , e.v , e.to );
  }

 /// a checkpoint of the batches open, the number of their events so far:
 /// with `rollback()` it makes the wrapper undoable, so that a batch that
 /// exits by an exception leaves the graph and the observers as they were
 /// [see `Batch`]
 [[nodiscard]] std::size_t checkpoint( void ) const
  requires( ! Subject::none ) {
  return( f_sub.buffered() );
  }

 /// whether the changes since the checkpoint \f$ t \f$ can be undone: all
 /// on edges, and every kind listened to, so that each is collected
 [[nodiscard]] bool revertible( std::size_t t ) const
  requires( ! Subject::none );

 /// back to the checkpoint \f$ t \f$ of the batches open: the changes
 /// collected since undone by their inverses, the last first (under the
 /// mute of the batch, which tells nobody); the graph is then the one at
 /// \f$ t \f$, up to the order of the edges in its lists
 /// \pre `revertible( t )`
 void rollback( std::size_t t ) requires( ! Subject::none );

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// attaches the observer \f$ o \f$, of either form, its mask read now,
 /// and gives its number
 /// \pre no channel is open
 template< class D >
 int attach( D & o ) requires( Subject::dynamic ) {
  if constexpr( mod::observer_of< D , modifications > )
   return( f_sub.attach( o ) );
  else {
   static_assert( GraphObserver< D , Weight > ,
                  "ADS: neither an observer of the events of the graph nor "
                  "one with mask() and notify()" );
   return( f_sub.template attachVia< detail::GraphNotify< Weight > >( o ) );
   }
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre \f$ \mathit{id} \f$ was given by `attach()` and not yet
 ///      detached, no channel is open
 void detach( int id ) requires( Subject::dynamic ) { f_sub.detach( id ); }

 /// opens a channel: the events are collected until it is closed
 [[nodiscard]] Batch batch( void ) { return( Batch( *this ) ); }

 /// the depth of the channels open, \f$ 0 \f$ if none
 [[nodiscard]] int depth( void ) const { return( f_sub.depth() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the key of an edge, its ends in order if unoriented
 [[nodiscard]] std::pair< int , int > key( int u , int v ) const {
  if( ! f_g.directed() && ( v < u ) )
   return( std::pair< int , int >( v , u ) );
  return( std::pair< int , int >( u , v ) );
  }

 /// sends \f$ e \f$, or collects it in a batch, then the audits
 template< class E >
 void tell( const E & e ) {
  f_sub.emit( e );
  f_sub.audit( f_g );
  }

 /// insertEdge() when someone listens
 void insertEdgeTold( int u , int v , const Weight & w );
 /// insertEdge() when someone listens, out of the hot code
 ADS_COLD_PATH void insertEdgeCold( int u , int v , const Weight & w ) {
  insertEdgeTold( u , v , w );
  }

 /// deleteEdge() when someone listens
 void deleteEdgeTold( int u , int v );
 /// deleteEdge() when someone listens, out of the hot code
 ADS_COLD_PATH void deleteEdgeCold( int u , int v ) {
  deleteEdgeTold( u , v );
  }

 /// setWeight() when someone listens
 bool setWeightTold( int u , int v , const Weight & w );
 /// setWeight() when someone listens, out of the hot code
 ADS_COLD_PATH bool setWeightCold( int u , int v , const Weight & w ) {
  return( setWeightTold( u , v , w ) );
  }

 G & f_g;                                  ///< the graph
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;      ///< the observers

 };  // end( class ObservedGraph )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Observable -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph \f$ g \f$, whose changes are sent to the static observers
/// `O...` of the first form [see GraphObserver] and to those attached
/// later, of either form
template< Graph G , class ... O >
using Observable = ObservedGraph< G , Both< Static< mod::Via< O ,
                    detail::GraphNotify< typename G::Weight > >... > ,
                                            Dynamic<> > >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Observable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Observable.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Observable.h --------------------------*/
/*--------------------------------------------------------------------------*/
