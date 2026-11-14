/*--------------------------------------------------------------------------*/
/*-------------------------- File Capabilities.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The capabilities of the structures, beyond the book: what an algorithm
 * may ask of its argument to choose the faster path at compile time, in
 * one vocabulary shared by all the areas (see
 * \ref theory_ADS_capabilities). A capability is of one of three forms.
 *
 * - A syntactic capability is a concept on members, e.g., `splittable< S >`
 *   for `split()`, `addressable< Q >` for the handles of a priority queue
 *   with `decrease()`, `increase()` and `erase()`, `undoable< S >` for
 *   `checkpoint()` and `rollback( t )`. A concept proves the syntax only.
 * - A semantic capability, which the syntax cannot show, is an explicit
 *   opt-in, as `std::ranges::enable_borrowed_range` of the standard: the
 *   variable templates `enable_stable_references< S >` (the addresses of
 *   the items do not change while they are in the structure) and
 *   `nonmonotone_values< T >`, the static members `Q::decrease_key` (how
 *   `decrease_key_of()` should take the queue) and `S::monotone` (the only
 *   direction in which the values of a structure move, read by
 *   `monotone_of()`).
 * - A property that a wrapper must forward without knowing it is a query,
 *   `s.query( ADS::stable_references_t{} )`, in the form of the
 *   environments of P2300: a tag derived from `forwarding_query`, answered
 *   by a static member `query( tag )` of the structure if it has one and by
 *   the default of the tag otherwise, and read at compile time by
 *   `query_v< S , Tag >` or by the customization point object `query()`.
 *   `ObservedGraph` and `Observable` forward every query to the graph they
 *   wrap (and answer the events themselves), `Shadow` intersects the
 *   answers of the two observers it composes by the `meet()` of the tag.
 *
 * `decrease_key_of< Q >()` says how a priority queue lowers a key:
 * `native` if it has `decrease( h , p )` on its handles, `lazy` if it is a
 * `PriorityQueue` without them, so that an item whose priority improves is
 * inserted again and its stale copies are skipped when they come out,
 * `none` if it is not a priority queue; the opt-in `Q::decrease_key` wins.
 * The shortest paths and Prim's algorithm choose by it.
 *
 * The free customization point objects, in the form of Niebler, exist only
 * for the operations that a type ADS does not own may lack, one per
 * operation and each with a default: `reserve( s , n )` (nothing),
 * `prefetch( s , k )` (nothing) and `insert_bulk( s , r )` (one insertion
 * per item); those of the priority queues are in `PriorityQueue.h`.
 *
 * Every capability says, in its comment, the algorithm that uses it; a
 * capability whose members no structure of ADS has yet is defined all the
 * same, since it is the name under which the next one will be found.
 *
 * \see E. Niebler, Suggested design for customization points, ISO/IEC JTC1
 *      SC22 WG21 N4381, 2015,
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html
 * \see M. Dominiak, G. Evtushenko, L. Baker, L. R. Teodorescu, L. Howes,
 *      K. Shoop, M. Garland, E. Niebler, B. A. Lelbach, std::execution,
 *      ISO/IEC JTC1 SC22 WG21 P2300R10, 2024,
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html
 * \see G. E. Blelloch, D. Ferizovic, Y. Sun, Just join for parallel ordered
 *      sets, Proceedings of SPAA, 253-264, 2016,
 *      doi:10.1145/2935764.2935768
 * \see M. Chen, R. A. Chowdhury, V. Ramachandran, D. Lan Roche, L. Tong,
 *      Priority queues and Dijkstra's algorithm, Technical Report TR-07-54,
 *      The University of Texas at Austin, 2007,
 *      https://www3.cs.stonybrook.edu/~rezaul/papers/TR-07-54.pdf
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

#ifndef __ADS_Capabilities
 #define __ADS_Capabilities
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace mod {

/// a list of types, the events of a structure [see `Modification.h`]
template< class ... E >
struct list;

}  // end( namespace mod )

/*--------------------------------------------------------------------------*/
/*--------------------------- THE ASSOCIATED TYPES -------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail {

template< class S >
constexpr auto key_of( void )
{
 if constexpr( requires { typename S::Key; } )
  return( std::type_identity< typename S::Key >() );
 else if constexpr( requires { typename S::key_type; } )
  return( std::type_identity< typename S::key_type >() );
 else if constexpr( requires { typename S::value_type; } )
  return( std::type_identity< typename S::value_type >() );
 else
  return( std::type_identity< std::size_t >() );
 }

template< class S >
struct modifications_of {
 using type = mod::list<>;
 };

template< class S >
 requires requires { typename S::modifications; }
struct modifications_of< S > {
 using type = typename S::modifications;
 };

template< class Q >
struct priority_of {
 using type = queue_item_t< Q >;
 };

template< class Q >
 requires requires { typename Q::Priority; }
struct priority_of< Q > {
 using type = typename Q::Priority;
 };

}  // end( namespace detail )
/// \endcond

/// the keys of `S`, which its lookups take: `S::Key`, else `S::key_type`,
/// else `S::value_type`, else `std::size_t` (a position)
template< class S >
using key_type_t = typename decltype( detail::key_of< S >() )::type;

/// the events of `S` [see `Modification.h`], `S::modifications` if it
/// declares them, the empty list otherwise: what an observer is checked
/// against when it is attached
template< class S >
using modifications_of = typename detail::modifications_of< S >::type;

/// the priorities of the queue `Q`: `Q::Priority`, else its items
template< class Q >
using priority_t = typename detail::priority_of< Q >::type;

/*--------------------------------------------------------------------------*/
/*--------------------------- THE SEMANTIC OPT-INS -------------------------*/
/*--------------------------------------------------------------------------*/

/// whether the items of `S` keep their addresses while they are in it, so
/// that a handle may be a pointer and the structure never emits
/// `Relocated`: an explicit opt-in, false unless a structure specializes
/// it, since no syntax shows it; read by the default of
/// `stable_references_t`, which the wrappers forward (the handles of the
/// observers, the node tables of a search)
template< class S >
inline constexpr bool enable_stable_references = false;

/// whether the values of the nodes of the table `T` may grow as well as
/// fall, so that its concurrent fields are atomics under a seqlock instead
/// of the monotone ones read without a lock: an explicit opt-in for the
/// incremental searches whose \f$ g \f$ may rise (LPA*, D* Lite), false by
/// default, so that A* pays nothing
template< class T >
inline constexpr bool nonmonotone_values = false;

/// the directions in which the values of `S` may move, as the bits `up` and
/// `down` of `mod::effect`: `S::monotone` if it declares it (e.g.,
/// `static constexpr unsigned monotone = mod::effect::down;` for a table of
/// \f$ g \f$ under A*), every bit otherwise; an observer compiles out the
/// path of a direction whose bit is not set
template< class S >
constexpr unsigned monotone_of( void )
{
 if constexpr( requires { S::monotone; } )
  return( unsigned( S::monotone ) );
 else
  return( ~0u );
 }

/// whether `S` declares the only direction in which its values move
template< class S >
concept declares_monotone = requires {
 { S::monotone } -> std::convertible_to< unsigned >;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUERIES -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the base of the tags of the queries that a wrapper forwards to what it
/// wraps
struct forwarding_query {};

/// a tag of a forwarded query
template< class T >
concept forwarding_query_tag = std::derived_from< T , forwarding_query > &&
                               std::is_empty_v< T >;

/// the query of whether the items keep their addresses; by default
/// `enable_stable_references< S >`, and both structures of a composition
/// must have them
struct stable_references_t : forwarding_query {
 /// the answer of a structure that does not give one
 template< class S >
 static constexpr bool fallback( void ) {
  return( enable_stable_references< S > );
  }
 /// the answer of the composition of two structures
 static constexpr bool meet( bool a , bool b ) { return( a && b ); }
 };

/// the query of whether reads may run with other reads and with writes
/// from other threads; false by default, and both structures of a
/// composition must allow them
struct thread_safe_reads_t : forwarding_query {
 /// the answer of a structure that does not give one
 template< class S >
 static constexpr bool fallback( void ) { return( false ); }
 /// the answer of the composition of two structures
 static constexpr bool meet( bool a , bool b ) { return( a && b ); }
 };

/// a class of time, from the cheapest
enum class complexity { constant , logarithmic , linear };

/// the query of the time of `rank()`, so that an \f$ O( n ) \f$ rank is
/// not taken for an \f$ O( 1 ) \f$ one; linear by default, the worse of
/// the two in a composition
struct rank_complexity_t : forwarding_query {
 /// the answer of a structure that does not give one
 template< class S >
 static constexpr complexity fallback( void ) {
  return( complexity::linear );
  }
 /// the answer of the composition of two structures
 static constexpr complexity meet( complexity a , complexity b ) {
  return( std::max( a , b ) );
  }
 };

/// the query of the events of a structure, as `std::type_identity` of a
/// `mod::list`; by default `modifications_of< S >`, and with no meet: a
/// composition emits the events it declares
struct event_set_t : forwarding_query {
 /// the answer of a structure that does not give one
 template< class S >
 static constexpr auto fallback( void ) {
  return( std::type_identity< modifications_of< S > >() );
  }
 };

/// the tags of the queries
inline constexpr stable_references_t stable_references{};
inline constexpr thread_safe_reads_t thread_safe_reads{};
inline constexpr rank_complexity_t rank_complexity{};
inline constexpr event_set_t event_set{};

/// \cond
namespace detail {

template< class S , class T >
constexpr auto query_of( void )
{
 if constexpr( requires { S::query( T() ); } )
  return( S::query( T() ) );
 else
  return( T::template fallback< S >() );
 }

}  // end( namespace detail )
/// \endcond

/// the answer of `S` to the query `T`, at compile time: the static
/// `S::query( T() )` if it has one, `T::fallback< S >()` otherwise
template< class S , forwarding_query_tag T >
inline constexpr auto query_v = detail::query_of< std::remove_cvref_t< S > ,
                                                  T >();

/// whether the query `T` composes two answers
template< class T >
concept meetable_query = forwarding_query_tag< T > && requires {
 T::meet( T::template fallback< int >() , T::template fallback< int >() );
 };

/// the answer of the composition of `A` and `B` to the query `T`
template< class A , class B , meetable_query T >
inline constexpr auto query_meet_v = T::meet( query_v< A , T > ,
                                              query_v< B , T > );

/// \cond
namespace detail {

struct Query {
 template< class S , forwarding_query_tag T >
 constexpr auto operator()( const S & , T ) const noexcept {
  return( query_v< S , T > );
  }
 };

}  // end( namespace detail )
/// \endcond

/// the answer of \f$ s \f$ to the query of the tag \f$ t \f$,
/// `query_v< S , T >`
inline constexpr detail::Query query{};

/*--------------------------------------------------------------------------*/
/*------------------------ THE CUSTOMIZATION POINTS ------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail {

void reserve( void ) = delete;          // the free ones, only by ADL
void prefetch( void ) = delete;
void insert_bulk( void ) = delete;

struct Reserve {
 template< class S >
 constexpr void operator()( S & s , std::size_t n ) const {
  if constexpr( requires { s.reserve( n ); } )
   s.reserve( n );
  else if constexpr( requires { reserve( s , n ); } )
   reserve( s , n );
  }
 };

struct Prefetch {
 template< class S , class K >
 constexpr void operator()( const S & s , const K & k ) const {
  if constexpr( requires { s.prefetch( k ); } )
   s.prefetch( k );
  else if constexpr( requires { prefetch( s , k ); } )
   prefetch( s , k );
  }
 };

template< class S , class R >
concept member_bulk = requires( S & s , R && r ) {
 s.insert_bulk( std::forward< R >( r ) );
 };
template< class S , class R >
concept free_bulk = requires( S & s , R && r ) {
 insert_bulk( s , std::forward< R >( r ) );
 };
template< class S , class R >
concept one_by_one = std::ranges::input_range< R > &&
                     requires( S & s ,
                               std::ranges::range_reference_t< R > x ) {
 s.insert( x );
 };

struct InsertBulk {
 template< class S , class R >
  requires( member_bulk< S , R > || free_bulk< S , R > || one_by_one< S , R > )
 constexpr void operator()( S & s , R && r ) const {
  if constexpr( member_bulk< S , R > )
   s.insert_bulk( std::forward< R >( r ) );
  else if constexpr( free_bulk< S , R > )
   insert_bulk( s , std::forward< R >( r ) );
  else
   for( auto && x : r )
    s.insert( x );
  }
 };

}  // end( namespace detail )
/// \endcond

/// room for \f$ n \f$ items in \f$ s \f$: `s.reserve( n )`, else
/// `reserve( s , n )` found by ADL, else nothing (the builders)
inline constexpr detail::Reserve reserve{};

/// a hint that \f$ k \f$ is looked up next: `s.prefetch( k )`, else
/// `prefetch( s , k )` found by ADL, else nothing (a batch of lookups that
/// prefetches the next ones, by group prefetching)
inline constexpr detail::Prefetch prefetch{};

/// the items of the range \f$ r \f$ inserted in \f$ s \f$:
/// `s.insert_bulk( r )`, else `insert_bulk( s , r )` found by ADL, else
/// `s.insert( x )` for each item \f$ x \f$ of \f$ r \f$
inline constexpr detail::InsertBulk insert_bulk{};

/*--------------------------------------------------------------------------*/
/*------------------------ THE CAPABILITIES OF SETS ------------------------*/
/*--------------------------------------------------------------------------*/

/// `insert_bulk( r )` of the range \f$ r \f$ of items as one operation (the
/// generic insertion of a range, the event `BulkInserted`)
template< class S ,
          class R = std::span< const key_type_t< S > > >
concept bulk_insertable = requires( S & s , R r ) { s.insert_bulk( r ); };

/// `erase_bulk( r )` of the range \f$ r \f$ of keys as one operation (the
/// event `BulkErased`)
template< class S ,
          class R = std::span< const key_type_t< S > > >
concept bulk_erasable = requires( S & s , R r ) { s.erase_bulk( r ); };

/// `std::move( s ).split( k )`, the keys before \f$ k \f$, its value and the
/// keys after it (the set operations of Blelloch, Ferizovic and Sun, the
/// cut of a batch)
template< class S >
concept splittable = requires( S && s , const key_type_t< S > & k ) {
 std::move( s ).split( k );
 };

/// `S::join2( l , r )`, the tree of the keys of \f$ l \f$ and of
/// \f$ r \f$ when those of \f$ l \f$ are smaller (the union of Blelloch,
/// Ferizovic and Sun in \f$ O( m \log( n / m + 1 ) ) \f$, the link of a
/// batch)
template< class S >
concept joinable = requires( S && l , S && r ) {
 { S::join2( std::move( l ) , std::move( r ) ) } -> std::same_as< S >;
 };

/// `rank( k )`, the number of keys smaller than \f$ k \f$ (the order
/// statistics); its time is the query `rank_complexity_t`
template< class S , class K = key_type_t< S > >
concept rankable = requires( const S & s , const K & k ) {
 { s.rank( k ) } -> std::convertible_to< std::size_t >;
 };

/// `select( i )`, the key of rank \f$ i \f$ (the order statistics); with an
/// augmentation `G`, `augSelect( p )`, the first key whose prefix of the
/// values of `G` satisfies the monotone predicate \f$ p \f$
template< class S , class G = void >
concept selectable =
 ( std::is_void_v< G > &&
   requires( const S & s , std::size_t i ) { s.select( i ); } ) ||
 ( ! std::is_void_v< G > &&
   requires( const S & s , bool ( *p )( const typename G::Value & ) ) {
    s.augSelect( p );
    } );

/// `prefetch( k )`, a hint that \f$ k \f$ (a key or its hash) is looked up
/// next (a batch of lookups by group prefetching)
template< class S , class K = key_type_t< S > >
concept prefetchable = requires( const S & s , const K & k ) {
 s.prefetch( k );
 };

/// `reserve( n )`, room for \f$ n \f$ items (the builders)
template< class S >
concept reservable = requires( S & s , std::size_t n ) { s.reserve( n ); };

/// `augRange( lo , hi )`, the values of the augmentation `G` of the keys
/// in \f$ [ \mathit{lo} , \mathit{hi} ] \f$ combined, kept by the
/// structure as `S::Augmented` (the aggregates of a range without a scan)
template< class S , class G >
concept augmented_by = std::same_as< typename S::Augmented , G > &&
                       requires( const S & s , const key_type_t< S > & k ) {
 { s.augRange( k , k ) } -> std::convertible_to< typename G::Value >;
 };

/// `applyRange( lo , hi , a )`, the action \f$ a \f$ applied to every value
/// of the keys in \f$ [ \mathit{lo} , \mathit{hi} ] \f$ in
/// \f$ O( \log n ) \f$ by a lazy tag (the lazy updates of a range)
template< class S , class A >
concept range_actionable = requires( S & s , const key_type_t< S > & k ,
                                     const A & a ) {
 s.applyRange( k , k , a );
 };

/*--------------------------------------------------------------------------*/
/*----------------------- THE CAPABILITIES OF QUEUES -----------------------*/
/*--------------------------------------------------------------------------*/

/// how a priority queue lowers the priority of an item: it cannot
/// (`none`), by a new insertion and the stale copies skipped when they come
/// out (`lazy`), by `decrease()` on its handle (`native`)
enum class decrease_key_kind { none , lazy , native };

/// how the queue `Q` lowers a priority: `Q::decrease_key` if it declares
/// it (e.g., a queue whose `decrease()` is an insertion), else `native` if
/// it has `decrease( h , p )` on its handles, else `lazy` if it is a
/// `PriorityQueue`, else `none` (the shortest paths and Prim's algorithm)
//! [decrease_key_of]
template< class Q >
constexpr decrease_key_kind decrease_key_of( void )
{
 if constexpr( requires { Q::decrease_key; } )
  return( Q::decrease_key );                         // the opt-in wins
 else if constexpr( requires( Q & q , typename Q::Handle h ,
                              const typename Q::Priority & p ) {
                     q.decrease( h , p ); } )
  return( decrease_key_kind::native );
 else if constexpr( PriorityQueue< Q > )
  return( decrease_key_kind::lazy );
 else
  return( decrease_key_kind::none );
 }
//! [decrease_key_of]

/// an `AddressablePriorityQueue` with `increase( h , p )` and
/// `erase( h )` on its handles as well (the open lists whose priorities
/// rise, the native decrease-key)
template< class Q >
concept addressable = AddressablePriorityQueue< Q > &&
                      requires( Q & q , typename Q::Handle h ,
                                const typename Q::Priority & p ) {
 q.increase( h , p );
 q.erase( h );
 };

/// `update( id )`, the key of the item \f$ \mathit{id} \f$ read again from
/// the table the queue observes (the queue of the open nodes as an observer
/// of the node table, in the form of LaB-PQ)
template< class Q >
concept observes_values = requires( Q & q , int id ) { q.update( id ); };

/// `extract( theta , out )`, every item of priority at most
/// \f$ \theta \f$ moved to `out` at once (the stepping algorithms, the
/// bulk-synchronous search)
template< class Q >
concept batch_extract = requires( Q & q , const priority_t< Q > & theta ,
                                  std::vector< queue_item_t< Q > > & out ) {
 q.extract( theta , out );
 };

/// `shift_all( delta )`, every key moved by \f$ \delta \f$ in
/// \f$ O( 1 ) \f$ (D* Lite, Generalized Adaptive A*)
template< class Q >
concept shiftable_keys = requires( Q & q , const priority_t< Q > & d ) {
 q.shift_all( d );
 };

/// a queue whose extractions never decrease, with `rebase()`, which lets
/// the next episode start below the last key extracted (the radix and Dial
/// queues between the episodes of a search)
template< class Q >
concept monotone_extract = PriorityQueue< Q > &&
                           requires( Q & q , const priority_t< Q > & k ) {
 q.rebase( k );
 };

/// \cond
namespace detail {

template< class Q >
struct same_key {
 const priority_t< Q > & operator()( const queue_item_t< Q > & ) const;
 };

}  // end( namespace detail )
/// \endcond

/// `rekey( f )`, the key of each item recomputed by \f$ f \f$ and the
/// order rebuilt in \f$ O( n ) \f$ (the change of the key function)
template< class Q , class F = detail::same_key< Q > >
concept rekeyable = requires( Q & q , F f ) { q.rekey( f ); };

/*--------------------------------------------------------------------------*/
/*-------------------- THE CAPABILITIES OF NODE TABLES ---------------------*/
/*--------------------------------------------------------------------------*/

/// `erase( k )` of a key (the node tables that reuse their places within an
/// episode)
template< class T >
concept erasable = requires( T & t , const key_type_t< T > & k ) {
 t.erase( k );
 };

/// `clear()` in \f$ O( 1 ) \f$ by epochs: a policy of `clear()`, `T::Clear`,
/// that stamps the places, and `epoch()` (the node tables of a sequence of
/// searches)
template< class T >
concept epoch_clearable = requires( T & t , const T & ct ) {
 typename T::Clear::Stamp;
 t.clear();
 { ct.epoch() } -> std::convertible_to< std::uint64_t >;
 };

/*--------------------------------------------------------------------------*/
/*------------------- THE CAPABILITIES OF UNDO AND STATE -------------------*/
/*--------------------------------------------------------------------------*/

/// `checkpoint()` and `rollback( t )` back to a checkpoint (a speculative
/// change, a batch that exits by an exception)
template< class S >
concept undoable = requires( S & s ) { s.rollback( s.checkpoint() ); };

/// `snapshot()`, a view of the present state in \f$ O( 1 ) \f$ which later
/// changes do not touch (the audits, a persistent structure)
template< class S >
concept snapshotable = requires( const S & s ) { s.snapshot(); };

/*--------------------------------------------------------------------------*/
/*----------------------- THE CAPABILITIES OF GRAPHS -----------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail {

struct any_arc {
 template< class ... A >
 void operator()( A && ... ) const;
 };

}  // end( namespace detail )
/// \endcond

/// `forEachIn( v , f )`, the arcs entering \f$ v \f$ (the observers that
/// repair a tree from the heads of the arcs that changed)
template< class G >
concept in_arcs = requires( const G & g , int v , detail::any_arc f ) {
 g.forEachIn( v , f );
 };

/// `arcId( u , v )`, an id of the arc that does not change while the arc
/// is in the graph (the observers that keep their data by arc)
template< class G >
concept stable_edge_ids = requires( const G & g , int u ) {
 { g.arcId( u , u ) } -> std::convertible_to< std::size_t >;
 };

/// `batch()`, a group of changes told as one (the observers of a batch of
/// changes)
template< class G >
concept batch_updatable = requires( G & g ) { g.batch(); };

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Capabilities.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Capabilities.h -------------------------*/
/*--------------------------------------------------------------------------*/
