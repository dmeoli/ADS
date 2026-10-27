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
 * An observer is any object with `mask()`, the set of the kinds of event
 * that it wants as a mask of bits (see `GraphEvents`), and with
 * `notify( e )` for one event and `notify( s )` for a `std::span` of them
 * (the concept `GraphObserver`). Those given to the constructor are static,
 * kept in a `std::tuple` and called directly, with no indirection; those
 * attached later by `attach()` are dynamic, kept as a pointer and two
 * pointers to functions that restore their type (the erasure of the type
 * with no allocation), and they can leave by `detach()`. The design follows
 * four rules: (i) no event is built when no one listens, since the union
 * \f$ M \f$ of the masks is kept and a change of kind \f$ k \f$ tests
 * \f$ k \in M \f$ before it reads anything that only the event needs (the
 * old weight of an edge, the edges of a node to delete); (ii) each observer
 * gets only the kinds in its mask, also within a batch; (iii) `batch()`
 * opens a channel of RAII, and the events of the channels, which nest, are
 * collected and sent as one span, coalesced into their net effect per edge
 * by (8) of \ref theory_graph_dynamic, when the outermost one closes;
 * (iv) an observer may defer its work to the next query, as those of
 * `Reoptimization.h` do. A change sent at once costs \f$ O( 1 ) \f$ per
 * observer that wants it, and a batch of \f$ k \f$ events costs
 * \f$ O( k ) \f$ expected time to coalesce, plus \f$ O( k ) \f$ for each
 * observer that does not want all its kinds (a filtered copy).
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

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
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
 int u;                                          ///< the node
 /// whether the two events are the same
 bool operator==( const NodeInserted & ) const = default;
 };

/// a node deleted, after the deletion of its edges
struct NodeDeleted {
 int u;                                          ///< the node
 /// whether the two events are the same
 bool operator==( const NodeDeleted & ) const = default;
 };

/// an edge \f$ ( u , v ) \f$, or \f$ [ u , v ] \f$ with \f$ u < v \f$,
/// inserted with the weight \f$ w \f$
template< class W >
struct EdgeInserted {
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W w;                                            ///< the weight
 /// whether the two events are the same
 bool operator==( const EdgeInserted & ) const = default;
 };

/// an edge deleted, \f$ w \f$ the weight that it had
template< class W >
struct EdgeDeleted {
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W w;                                            ///< the weight it had
 /// whether the two events are the same
 bool operator==( const EdgeDeleted & ) const = default;
 };

/// the weight of an edge changed from `from` to `to`
template< class W >
struct WeightChanged {
 int u;                                          ///< the tail
 int v;                                          ///< the head
 W from;                                         ///< the weight before
 W to;                                           ///< the weight after
 /// whether the two events are the same
 bool operator==( const WeightChanged & ) const = default;
 };

/// an event, whose kind is its index in the variant
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
/*--------------------------- CLASS Observable -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the graph \f$ g \f$, whose changes are sent to the static observers
/// `O...` and to those attached later
template< Graph G , class ... O >
class Observable {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;    ///< the type of the weights
 using Event = GraphEvent< Weight >;   ///< the type of the events

 /// the channel of a batch, open while it lives [see batch()]
 class Batch {
  public:

  /// the channel of \f$ b \f$, which no longer has it
  Batch( Batch && b ) noexcept : f_o( std::exchange( b.f_o , nullptr ) ) {}
  /// not copied
  Batch( const Batch & ) = delete;
  /// not assigned
  Batch & operator=( const Batch & ) = delete;
  /// not assigned
  Batch & operator=( Batch && ) = delete;
  /// closes the channel, if still open
  ~Batch() { close(); }

  /// closes the channel: the outermost one sends the batch, coalesced
  ///
  /// In pseudocode (beyond the book), with \f$ d \f$ the depth of the
  /// channels, \f$ B \f$ the events collected and \f$ M_o \f$ the mask of
  /// the observer \f$ o \f$:
  /// \f[
  /// \class{pseudocode}{\begin{array}{r@{\quad}l}
  /// 1 & \textbf{procedure}\ \mathrm{close}() \cr
  /// 2 & \quad d \gets d - 1;\ \textbf{if}\ d > 0\ \textbf{then}
  ///     \ \textbf{return} \cr
  /// 3 & \quad C \gets \mathrm{coalesce}( B );\ B \gets \emptyset \cr
  /// 4 & \quad \textbf{foreach}\ \text{observer } o\ \textbf{do} \cr
  /// 5 & \qquad C_o \gets \text{the events of } C \text{ whose kind is in }
  ///     M_o \cr
  /// 6 & \qquad \textbf{if}\ C_o \ne \emptyset\ \textbf{then}
  ///     \ o.\mathrm{notify}( C_o )
  /// \end{array}}
  /// \f]
  void close( void ) {
   if( f_o )
    std::exchange( f_o , nullptr )->closeBatch();
   }

  private:

  friend class Observable;
  /// the channel of \f$ o \f$
  explicit Batch( Observable * o ) : f_o( o ) {}
  Observable * f_o;                              ///< the wrapper, if open
  };

 /// the wrapper of \f$ g \f$, the static observers `o...`; \f$ g \f$
 /// must stay alive, and change only through the wrapper
 explicit Observable( G & g , O & ... o );

 /// not copied
 Observable( const Observable & ) = delete;
 /// not assigned
 Observable & operator=( const Observable & ) = delete;

 /// the graph
 [[nodiscard]] const G & graph( void ) const { return( f_g ); }

 /// the union of the masks of the observers
 [[nodiscard]] unsigned mask( void ) const { return( f_mask ); }

 /// whether anyone listens to one of the kinds \f$ k \f$
 [[nodiscard]] bool listening( unsigned k = GraphEvents::ALL ) const {
  return( ( f_mask & k ) != 0 );
  }

 /// the events built so far, sent or collected (none when no one listens)
 [[nodiscard]] std::size_t built( void ) const { return( f_built ); }

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
 int insertNode( void ) requires DynamicGraph< G >;

 /// `insertEdge()` of the graph, and `EdgeInserted`, or `WeightChanged` if
 /// the edge was there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 void insertEdge( int u , int v , const Weight & w = Weight() )
  requires DynamicGraph< G >;

 /// `deleteEdge()` of the graph, and `EdgeDeleted` if the edge was there
 /// \pre \f$ u \f$ and \f$ v \f$ are distinct nodes
 void deleteEdge( int u , int v ) requires DynamicGraph< G >;

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
 /// \pre \f$ u \f$ and \f$ v \f$ are nodes
 bool setWeight( int u , int v , const Weight & w );

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// attaches the observer \f$ o \f$, its mask read now, and gives its
 /// number
 /// \pre no channel is open
 template< GraphObserver< Weight > D >
 int attach( D & o ) {
  ADS_PRE( f_depth == 0 );
  Slot s;
  s.o = & o;
  s.mask = unsigned( o.mask() );
  s.one = []( void * p , const Event & e ) {
   static_cast< D * >( p )->notify( e ); };
  s.many = []( void * p , std::span< const Event > E ) {
   static_cast< D * >( p )->notify( E ); };
  f_dynamic.push_back( s );
  f_mask |= s.mask;
  return( int( f_dynamic.size() ) - 1 );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre \f$ \mathit{id} \f$ was given by `attach()` and not yet
 ///      detached, no channel is open
 void detach( int id );

 /// opens a channel: the events are collected until it is closed
 [[nodiscard]] Batch batch( void ) {
  ++f_depth;
  return( Batch( this ) );
  }

 /// the depth of the channels open, \f$ 0 \f$ if none
 [[nodiscard]] int depth( void ) const { return( f_depth ); }

/*--------------------------------------------------------------------------*/
 private:

 /// a dynamic observer, its type erased
 struct Slot {
  void * o = nullptr;                            ///< the observer
  unsigned mask = 0;                             ///< its mask
  /// its `notify( e )`
  void ( * one )( void * , const Event & ) = nullptr;
  /// its `notify( s )`
  void ( * many )( void * , std::span< const Event > ) = nullptr;
  };

 /// the key of an edge, its ends in order if unoriented
 [[nodiscard]] std::pair< int , int > key( int u , int v ) const;

 /// sends \f$ e \f$, or collects it in a batch
 void emit( Event && e );

 /// the closing of the outermost channel sends the batch
 void closeBatch( void );

 /// the events of \f$ E \f$ whose kind is in \f$ m \f$, by `send()`,
 /// \f$ E \f$ itself if \f$ m \f$ has all the kinds of \f$ E \f$ and a
 /// filtered copy in `tmp` if not
 template< class F >
 static void deliver( F && send , unsigned m , std::span< const Event > E ,
                      unsigned kinds , std::vector< Event > & tmp );

 /// the union of the masks
 void remask( void );

 G & f_g;                          ///< the graph
 std::tuple< O & ... > f_static;   ///< the static observers
 std::vector< Slot > f_dynamic;    ///< the dynamic ones, `o` null if gone
 unsigned f_mask = 0;              ///< the union of the masks
 int f_depth = 0;                  ///< the channels open
 std::vector< Event > f_batch;     ///< the events collected
 std::size_t f_built = 0;          ///< the events built

 };  // end( class Observable )

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
