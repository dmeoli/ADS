/*--------------------------------------------------------------------------*/
/*--------------------------- File Observable.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A GRAPH that tells its changes, beyond the book [see Graph.h]: the
 * wrapper Observable< G , O... > routes the changes of a graph g of any
 * model (ListGraph and MatrixGraph, and the weights of ArrayGraph and
 * StaticGraph) and turns each of them into an event, i.e., a value of the
 * std::variant GraphEvent< W >, sent to the observers attached to it:
 *
 * - NodeInserted{ u } and NodeDeleted{ u }, by insertNode() and
 *   deleteNode();
 * - EdgeInserted{ u , v , w } and EdgeDeleted{ u , v , w }, by insertEdge()
 *   of an edge not there and by deleteEdge(), w the weight that the edge
 *   gets or had;
 * - WeightChanged{ u , v , from , to }, by setWeight(), or by insertEdge()
 *   of an edge already there (which, in ListGraph and MatrixGraph, only
 *   replaces its weight).
 *
 * deleteNode( u ) first sends the deletion of each edge of u, so that an
 * observer never sees an edge whose end is gone. In an unoriented graph
 * the ends of an edge in an event are in increasing order, \f$ u < v \f$.
 *
 * An observer is any object with mask(), the set of the kinds of event it
 * wants as a mask of bits [see GraphEvents], notify( e ) for one event
 * and notify( s ) for a std::span of them [see the concept GraphObserver];
 * those given to the constructor are static, kept in a std::tuple and
 * called directly, with no indirection, and those attached later by
 * attach() are dynamic, kept as a pointer and two pointers to functions
 * that restore their type (the erasure of the type with no allocation),
 * and they can leave by detach(). The design follows four rules.
 *
 * 1. No event is built when no one listens: the union \f$ M \f$ of the
 *    masks is kept, and an operation of kind k tests \f$ k \in M \f$
 *    before it reads anything that only the event needs (the old weight
 *    of an edge, the edges of a node to delete); an operation that no one
 *    listens to costs what it costs on g, plus one test.
 * 2. Each observer gets only the kinds in its mask, also within a batch.
 * 3. batch() opens a channel, an object of RAII: while it is open the
 *    events are not sent but collected, and when it is closed (by
 *    close() or by its destructor) they are coalesced and sent, all
 *    together, as one span. Channels nest: only the closing of the
 *    outermost one sends. Coalescing replaces the events on each edge by
 *    their net effect, comparing the state \f$ s_0 \f$ of the edge before
 *    the first event on it with the state \f$ s_1 \f$ after the last one
 *    (absent, or present with a weight):
 *    \f[
 *     ( s_0 , s_1 ) \mapsto \left\{ \begin{array}{ll}
 *      \emptyset & s_0 = s_1 \\
 *      \mathrm{EdgeInserted}( w_1 ) & s_0 = \bot , s_1 = w_1 \\
 *      \mathrm{EdgeDeleted}( w_0 ) & s_0 = w_0 , s_1 = \bot \\
 *      \mathrm{WeightChanged}( w_0 , w_1 ) & \mbox{otherwise}
 *     \end{array} \right.
 *    \f]
 *    so that the insertion and the deletion of the same edge cancel out,
 *    and of many changes of a weight only the last one counts (the
 *    weights compared by ==, when W has it, else never equal). A node
 *    both inserted and deleted within the batch vanishes with all its
 *    edges. The span holds the nodes inserted, then the net events of
 *    the edges in the order of their first event, then the nodes deleted,
 *    which is again a sequence that an observer can replay. coalesce() is
 *    the same function on any sequence of events, in \f$ O( k ) \f$
 *    expected time for k events (a hash table on the edges).
 * 4. An observer may defer its work: it queues the events and works at
 *    the next query (the lazy flush), choosing then between updating its
 *    solution and computing it again from scratch by a threshold on the
 *    size of the queue [see Reoptimization.h].
 *
 * The pattern is the observer of Gamma, Helm, Johnson and Vlissides, with
 * the sets of interest of an event service, the coalescing of the events
 * of a batch into their net effect, and the queue that the dynamic
 * algorithms of the literature take (the event loop of Ramalingam and
 * Reps, the batches of Acar et al. and of Tseng, Dhulipala and Shun): the
 * algorithms of the survey of Hanauer, Henzinger and Schulz take a
 * sequence of updates, and a library that has them (e.g., the dynamic
 * algorithms of NetworKit, whose update() and updateBatch() the caller
 * feeds by hand after changing the graph) leaves it to the caller to tell
 * each of them what changed.
 *
 * The pseudocode of a change, say setWeight( u , v , w ), and of the
 * closing of a channel (k the kind of the event, \f$ M \f$ the union of
 * the masks, d the depth of the channels, B the events collected, \f$ M_o
 * \f$ the mask of the observer o):
 * <pre>
 *   setWeight( u , v , w )
 *  1  if \f$ k \notin M \f$ then change g; return
 *  2  \f$ w_0 \f$ ← the weight of ( u , v ) in g; change g
 *  3  e ← WeightChanged( u , v , \f$ w_0 \f$ , w )
 *  4  if d > 0 then append e to B
 *  5  else for each observer o with \f$ k \in M_o \f$: o.notify( e )
 *   close()
 *  1  d ← d - 1; if d > 0 then return
 *  2  C ← coalesce( B ); B ← \f$ \emptyset \f$
 *  3  for each observer o
 *  4   \f$ C_o \f$ ← the events of C whose kind is in \f$ M_o \f$
 *  5   if \f$ C_o \neq \emptyset \f$ then o.notify( \f$ C_o \f$ )
 * </pre>
 * A change sent at once costs \f$ O( 1 ) \f$ per observer that wants it
 * (the static ones inlined); a batch of k events costs \f$ O( k ) \f$ to
 * coalesce and \f$ O( k ) \f$ per observer that does not want all its
 * kinds (the filtered copy), none for one that wants them all.
 *
 * The contracts [see Contract.h]: the changes of the graph go through the
 * wrapper (a change made on g directly is not seen); an observer is not
 * attached nor detached while a channel is open, nor from within a
 * notify(); close() is idempotent. The operations are defined in
 * Observable.tpp.
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
 int u;
 bool operator==( const NodeInserted & ) const = default;
 };

/// a node deleted, after the deletion of its edges
struct NodeDeleted {
 int u;
 bool operator==( const NodeDeleted & ) const = default;
 };

/// an edge ( u , v ), or [ u , v ] with u < v, inserted with the weight w
template< class W >
struct EdgeInserted {
 int u , v;
 W w;
 bool operator==( const EdgeInserted & ) const = default;
 };

/// an edge deleted, w the weight it had
template< class W >
struct EdgeDeleted {
 int u , v;
 W w;
 bool operator==( const EdgeDeleted & ) const = default;
 };

/// the weight of an edge changed from from to to
template< class W >
struct WeightChanged {
 int u , v;
 W from , to;
 bool operator==( const WeightChanged & ) const = default;
 };

/// an event: the kind is its index in the variant
template< class W >
using GraphEvent = std::variant< NodeInserted , NodeDeleted ,
                                 EdgeInserted< W > , EdgeDeleted< W > ,
                                 WeightChanged< W > >;

/// the kinds of event as bits of a mask
struct GraphEvents {
 static constexpr unsigned NODE_INSERT = 1u;    ///< NodeInserted
 static constexpr unsigned NODE_DELETE = 2u;    ///< NodeDeleted
 static constexpr unsigned EDGE_INSERT = 4u;    ///< EdgeInserted
 static constexpr unsigned EDGE_DELETE = 8u;    ///< EdgeDeleted
 static constexpr unsigned WEIGHT = 16u;        ///< WeightChanged
 static constexpr unsigned EDGES = 28u;         ///< the three on edges
 static constexpr unsigned ALL = 31u;           ///< all of them
 };

/// the bit of the kind of e
template< class W >
[[nodiscard]] constexpr unsigned kindOf( const GraphEvent< W > & e ) {
 return( 1u << e.index() );
 }

/// the ends of the edge of e, ( -1 , -1 ) for an event on a node
template< class W >
[[nodiscard]] std::pair< int , int > endsOf( const GraphEvent< W > & e );

/// the net effect of the events E, in the order of the rule 3 above: the
/// nodes inserted, the edges by their first event, the nodes deleted
template< class W >
[[nodiscard]] std::vector< GraphEvent< W > > coalesce(
                               std::span< const GraphEvent< W > > E );

/// an observer of the events of weights W: the kinds it wants, and the
/// two ways of getting them
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

/// the graph g, whose changes are sent to the static observers O... and to
/// those attached later
template< Graph G , class ... O >
class Observable {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = typename G::Weight;    ///< the type of the weights
 using Event = GraphEvent< Weight >;   ///< the type of the events

 /// the channel of a batch, open while it lives [see batch()]
 class Batch {
  public:

  Batch( Batch && b ) noexcept : f_o( std::exchange( b.f_o , nullptr ) ) {}
  Batch( const Batch & ) = delete;
  Batch & operator=( const Batch & ) = delete;
  Batch & operator=( Batch && ) = delete;
  ~Batch() { close(); }

  /// closes the channel: the outermost one sends the batch, coalesced
  void close( void ) {
   if( f_o )
    std::exchange( f_o , nullptr )->closeBatch();
   }

  private:

  friend class Observable;
  explicit Batch( Observable * o ) : f_o( o ) {}
  Observable * f_o;
  };

 /// the wrapper of g, the static observers o...; g must stay alive, and
 /// change only through the wrapper
 explicit Observable( G & g , O & ... o );

 Observable( const Observable & ) = delete;
 Observable & operator=( const Observable & ) = delete;

 /// the graph
 [[nodiscard]] const G & graph( void ) const { return( f_g ); }

 /// the union of the masks of the observers
 [[nodiscard]] unsigned mask( void ) const { return( f_mask ); }

 /// whether anyone listens to one of the kinds k
 [[nodiscard]] bool listening( unsigned k = GraphEvents::ALL ) const {
  return( ( f_mask & k ) != 0 );
  }

 /// the events built so far, sent or collected (none when no one listens)
 [[nodiscard]] std::size_t built( void ) const { return( f_built ); }

/*---------------------------- THE GRAPH -----------------------------------*/

 [[nodiscard]] bool directed( void ) const { return( f_g.directed() ); }
 [[nodiscard]] int n( void ) const { return( f_g.n() ); }
 [[nodiscard]] int bound( void ) const { return( f_g.bound() ); }
 [[nodiscard]] bool hasNode( int u ) const { return( f_g.hasNode( u ) ); }
 [[nodiscard]] std::vector< int > V( void ) const { return( f_g.V() ); }
 [[nodiscard]] std::vector< int > adj( int u ) const {
  return( f_g.adj( u ) );
  }
 template< class F >
 void forEachAdj( int u , F && f ) const {
  f_g.forEachAdj( u , std::forward< F >( f ) );
  }
 template< class F >
 void forEachEdge( int u , F && f ) const {
  f_g.forEachEdge( u , std::forward< F >( f ) );
  }
 [[nodiscard]] std::optional< Weight > weight( int u , int v ) const {
  return( f_g.weight( u , v ) );
  }

/*---------------------------- THE CHANGES ---------------------------------*/

 /// insertNode() of g, and NodeInserted
 int insertNode( void ) requires DynamicGraph< G >;

 /// insertEdge() of g, and EdgeInserted, or WeightChanged if the edge was
 /// there
 /// \pre u and v are distinct nodes
 void insertEdge( int u , int v , const Weight & w = Weight() )
  requires DynamicGraph< G >;

 /// deleteEdge() of g, and EdgeDeleted if the edge was there
 /// \pre u and v are distinct nodes
 void deleteEdge( int u , int v ) requires DynamicGraph< G >;

 /// EdgeDeleted for each edge of u, deleteNode() of g, and NodeDeleted
 /// \pre u is a node
 void deleteNode( int u ) requires DynamicGraph< G >;

 /// the weight of the edge ( u , v ) set to w, and WeightChanged; false,
 /// and nothing, if there is no such edge
 /// \pre u and v are nodes
 bool setWeight( int u , int v , const Weight & w );

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// attaches the observer o, its mask read now, and gives its number
 /// \pre no channel is open
 template< GraphObserver< Weight > D >
 int attach( D & o );

 /// detaches the observer number id
 /// \pre id was given by attach() and not yet detached, no channel is open
 void detach( int id );

 /// opens a channel: the events are collected until it is closed
 [[nodiscard]] Batch batch( void ) {
  ++f_depth;
  return( Batch( this ) );
  }

 /// the depth of the channels open, 0 if none
 [[nodiscard]] int depth( void ) const { return( f_depth ); }

/*--------------------------------------------------------------------------*/
 private:

 /// a dynamic observer, its type erased
 struct Slot {
  void * o = nullptr;
  unsigned mask = 0;
  void ( * one )( void * , const Event & ) = nullptr;
  void ( * many )( void * , std::span< const Event > ) = nullptr;
  };

 /// the key of an edge, its ends in order if unoriented
 [[nodiscard]] std::pair< int , int > key( int u , int v ) const;

 /// sends e, or collects it in a batch
 void emit( Event && e );

 /// the closing of the outermost channel sends the batch
 void closeBatch( void );

 /// the events of E whose kind is in m, by send(), E itself if m has all
 /// the kinds of E and a filtered copy in tmp if not
 template< class F >
 static void deliver( F && send , unsigned m , std::span< const Event > E ,
                      unsigned kinds , std::vector< Event > & tmp );

 /// the union of the masks
 void remask( void );

 G & f_g;                          ///< the graph
 std::tuple< O & ... > f_static;   ///< the static observers
 std::vector< Slot > f_dynamic;    ///< the dynamic ones, o null if gone
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
