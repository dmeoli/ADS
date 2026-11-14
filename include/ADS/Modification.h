/*--------------------------------------------------------------------------*/
/*-------------------------- File Modification.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The typed modifications, beyond the book: the vocabulary by which a
 * structure tells the observers attached to it what changed, by small
 * values that carry the old and the new state, and the policies that
 * decide, in the type of the structure, who listens and at what cost (see
 * \ref theory_ADS_modifications).
 *
 * The events are the templates of the namespace `ADS::mod`, each an
 * aggregate with a static mask `effects` (a subset of `effect::grows`,
 * `shrinks`, `up`, `down`) and a static `role`: `Inserted< H , K , V >` and
 * `Erased< H , K , V >` of the item of handle \f$ h \f$, key \f$ k \f$ and
 * value \f$ v \f$ (no value when `V` is `void`), `Changed< A , H , V >` of
 * the attribute `A` of an item from `from` to `to` (`Assigned` and
 * `KeyChanged` for the attributes `Value` and `Key`), `Relocated`,
 * `Permuted`, `BulkInserted`, `BulkErased`, `Shifted< A , D >` (every value
 * of `A` moved by `delta`), `Replaced< A >`, `Cleared` and `Rebuilt` (anything
 * may have changed). A structure declares the events it emits as a type
 * list, `using modifications = mod::list< ... >`.
 *
 * An observer of a structure is any object with an `on( e )` for at least
 * one of its events; the other parts are optional and found at compile
 * time: `on( s )` for a `std::span` of one kind or of the variant of the
 * events, `group_begin()` and `group_end()` around a batch, `prepare( e )`,
 * which may refuse a change, `audit( s )`, `mask()`, a filter of the kinds
 * at run time, `using requires_modifications = mod::list< ... >` and
 * `using depends_on = mod::list< ... >`; an `on()` may return `Keep` to
 * leave. The subject policy is a parameter of the structure: `NoObservers`
 * (no member and no code), `Static< O... >` (references called directly,
 * in the order of the arguments), `Dynamic<>` (observers attached at run
 * time, a slot of erased type each and a union of their masks) and
 * `Both< Static< O... > , Dynamic<> >`, the static ones first. A mutator
 * tests `listened< E >` at compile time and `live< E >()` at run time
 * before it builds an event, so that with `NoObservers` it compiles to the
 * code of the plain structure. `batch( s )` opens a group of RAII, nested
 * groups only count, and the outermost one sends the events coalesced into
 * their net effect per handle (by `coalesce_traits`), as one span per
 * observer; `quiet( s )` suspends the events and sends `Rebuilt` at its end
 * (nothing with `no_rebuild`); `emit( e , undo )` gives the strong
 * guarantee against an observer that throws, by the inverse events and the
 * undo of the structure; each kind has a revision counter. `UndoLog` makes
 * any structure with `apply()` undoable, and `Shadow` checks an incremental
 * observer against one that computes from scratch.
 *
 * The contracts [see `Contract.h`]: `on()` of `Erased`, `BulkErased`,
 * `Cleared`, `Rebuilt` and `Replaced` is `noexcept` (checked at compile
 * time); an observer neither changes its subject nor attaches or detaches
 * from within `on()`; nobody attaches or detaches while a batch is open;
 * at the level of the audits the subject calls `audit()` of its observers
 * after each event outside a batch and after each batch, and counts the
 * events of each observer. The operations are defined in
 * `Modification.tpp`.
 *
 * \see E. Gamma, R. Helm, R. Johnson, J. Vlissides, Design Patterns:
 *      Elements of Reusable Object-Oriented Software, Addison-Wesley,
 *      1994, ISBN 0-201-63361-2, the pattern Observer
 * \see A. Alexandrescu, Modern C++ Design: Generic Programming and Design
 *      Patterns Applied, Addison-Wesley, 2001, ISBN 0-201-70431-5
 * \see D. Abrahams, Exception-safety in generic components, Generic
 *      Programming, LNCS 1766, 69-79, 2000, doi:10.1007/3-540-39953-4_6
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
 * \see D. Gregor, A. Lumsdaine, Lifting sequential graph algorithms for
 *      distributed-memory parallel computation, Proceedings of OOPSLA,
 *      423-437, 2005, doi:10.1145/1094811.1094844
 * \see T. A. Davis, Algorithm 1000: SuiteSparse:GraphBLAS: graph algorithms
 *      in the language of sparse linear algebra, ACM Transactions on
 *      Mathematical Software 45(4), 1-25, 2019, doi:10.1145/3322125
 * \see U. A. Acar, G. E. Blelloch, R. Harper, Adaptive functional
 *      programming, Proceedings of POPL, 247-259, 2002,
 *      doi:10.1145/503272.503296
 * \see J. Basch, L. J. Guibas, J. Hershberger, Data structures for mobile
 *      data, Journal of Algorithms 31(1), 1-28, 1999,
 *      doi:10.1006/jagm.1998.0988
 * \see A. Frangioni, A. Manca, A computational study of cost
 *      reoptimization for min-cost flow problems, INFORMS Journal on
 *      Computing 18(1), 61-70, 2006, doi:10.1287/ijoc.1040.0081
 * \see Y. Solodkyy, G. Dos Reis, B. Stroustrup, Open and efficient type
 *      switch for C++, Proceedings of OOPSLA, 963-982, 2012,
 *      doi:10.1145/2384616.2384686
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

#ifndef __ADS_Modification
 #define __ADS_Modification
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
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
/*------------------------------ THE ANSWERS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// what an `on()` that returns it asks: to stay, or to be detached after
/// the event, with no exception
enum class Keep { Yes , Detach };

/// a change refused by the `prepare()` of an observer, before anything
/// changed
class Refused : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
 };

/// the promise of `quiet( s , no_rebuild )`: nobody needs to know
struct no_rebuild_t {
 explicit constexpr no_rebuild_t( void ) = default;
 };

/// the promise of `quiet( s , no_rebuild )`
inline constexpr no_rebuild_t no_rebuild{};

namespace mod {

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EFFECTS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the effects of an event, as bits: a bit not set is a guarantee
struct effect {
 static constexpr unsigned grows = 1u;     ///< the structure may grow
 static constexpr unsigned shrinks = 2u;   ///< it may shrink
 static constexpr unsigned up = 4u;        ///< a value may increase
 static constexpr unsigned down = 8u;      ///< a value may decrease
 static constexpr unsigned all = 15u;      ///< all of them
 };

/// the role of an event in the coalescing of a batch
enum class Role { Insert , Erase , Change , Relocate , Permute ,
                  BulkInsert , BulkErase , Shift , Replace , Clear ,
                  Rebuild , Other };

/// the role of the event `E`, `Role::Other` if it declares none
template< class E >
inline constexpr Role role_of = [] {
 if constexpr( requires { E::role; } )
  return( E::role );
 else
  return( Role::Other );
 }();

/// the effects of the event `E`, all of them if it declares none
template< class E >
inline constexpr unsigned effects_of = [] {
 if constexpr( requires { E::effects; } )
  return( unsigned( E::effects ) );
 else
  return( effect::all );
 }();

/// whether `on()` of the event `E` must not throw: the erasures, the
/// clearing, the rebuilding and the replacement, after which an observer
/// left half updated could not be repaired
template< class E >
inline constexpr bool must_not_throw =
 ( role_of< E > == Role::Erase ) || ( role_of< E > == Role::BulkErase ) ||
 ( role_of< E > == Role::Clear ) || ( role_of< E > == Role::Rebuild ) ||
 ( role_of< E > == Role::Replace );

/*--------------------------------------------------------------------------*/
/*------------------------- THE ATTRIBUTES AND LISTS -----------------------*/
/*--------------------------------------------------------------------------*/

struct Value {};   ///< the attribute of the value of an item
struct Key {};     ///< the attribute of a key that orders the items
struct Cost {};    ///< the cost of an arc
struct Upper {};   ///< the upper bound of an arc
struct Lower {};   ///< the lower bound of an arc
struct Supply {};  ///< the supply of a node

/// how two changes of one attribute of one item compose in a batch: the
/// last value (`last`), the least or the largest of the two (`min`,
/// `max`), or the sum of the differences (`sum`)
namespace reduce {
struct last {};   ///< the value after the second change
struct min {};    ///< the least of the two values after
struct max {};    ///< the largest of the two values after
struct sum {};    ///< the first value after plus the second difference
}  // end( namespace reduce )

/// the reduction of the attribute `A`: `A::reduction` if it has one,
/// `reduce::last` otherwise
template< class A >
struct reduction_of {
 using type = reduce::last;   ///< the reduction
 };

/// the reduction of an attribute that declares one
template< class A >
 requires requires { typename A::reduction; }
struct reduction_of< A > {
 using type = typename A::reduction;   ///< the reduction
 };

/// a list of types, the events of a structure
template< class ... E >
struct list {
 static constexpr std::size_t size = sizeof...( E );   ///< their number
 };

/// \cond
namespace detail {

template< class E , class L >
struct index_in;

template< class E , class ... T >
struct index_in< E , list< T... > > {
 static constexpr std::size_t value = [] {
  constexpr bool same[] = { std::is_same_v< E , T >... , false };
  std::size_t i = 0;
  while( ( i < sizeof...( T ) ) && ! same[ i ] )
   ++i;
  return( i );
  }();
 };

template< class L >
struct variant_of;

template< class ... T >
struct variant_of< list< T... > > {
 using type = std::variant< T... >;
 };

}  // end( namespace detail )
/// \endcond

/// the position of `E` in the list `L`, its size if absent
template< class E , class L >
inline constexpr std::size_t index_v = detail::index_in< E , L >::value;

/// whether `E` is in the list `L`
template< class E , class L >
inline constexpr bool contains_v = index_v< E , L > < L::size;

/// the variant of the events of the list `L`, the type of a buffer
template< class L >
using variant_t = typename detail::variant_of< L >::type;

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EVENTS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the item of handle \f$ h \f$, key \f$ k \f$ and value \f$ v \f$ inserted
template< class H , class K , class V = void >
struct Inserted {
 static constexpr unsigned effects = effect::grows;   ///< it grows
 static constexpr Role role = Role::Insert;           ///< an insertion
 using handle_type = H;                               ///< the handle
 using key_type = K;                                  ///< the key
 using value_type = V;                                ///< the value
 H h;                                                 ///< the handle
 K key;                                               ///< the key
 V value;                                             ///< the value
 /// whether the two events are the same
 bool operator==( const Inserted & ) const = default;
 };

/// the item of handle \f$ h \f$ and key \f$ k \f$ inserted, with no value
template< class H , class K >
struct Inserted< H , K , void > {
 static constexpr unsigned effects = effect::grows;   ///< it grows
 static constexpr Role role = Role::Insert;           ///< an insertion
 using handle_type = H;                               ///< the handle
 using key_type = K;                                  ///< the key
 using value_type = void;                             ///< no value
 H h;                                                 ///< the handle
 K key;                                               ///< the key
 /// whether the two events are the same
 bool operator==( const Inserted & ) const = default;
 };

/// the item of handle \f$ h \f$, key \f$ k \f$ and value \f$ v \f$ erased,
/// told before the removal outside a batch
template< class H , class K , class V = void >
struct Erased {
 static constexpr unsigned effects = effect::shrinks;  ///< it shrinks
 static constexpr Role role = Role::Erase;             ///< an erasure
 using handle_type = H;                                ///< the handle
 using key_type = K;                                   ///< the key
 using value_type = V;                                 ///< the value
 H h;                                                  ///< the handle
 K key;                                                ///< the key
 V value;                                              ///< the value
 /// whether the two events are the same
 bool operator==( const Erased & ) const = default;
 };

/// the item of handle \f$ h \f$ and key \f$ k \f$ erased, with no value
template< class H , class K >
struct Erased< H , K , void > {
 static constexpr unsigned effects = effect::shrinks;  ///< it shrinks
 static constexpr Role role = Role::Erase;             ///< an erasure
 using handle_type = H;                                ///< the handle
 using key_type = K;                                   ///< the key
 using value_type = void;                              ///< no value
 H h;                                                  ///< the handle
 K key;                                                ///< the key
 /// whether the two events are the same
 bool operator==( const Erased & ) const = default;
 };

/// the attribute `A` of the item of handle \f$ h \f$ changed from `from`
/// to `to`
template< class A , class H , class V >
struct Changed {
 static constexpr unsigned effects = effect::up | effect::down;  ///< both
 static constexpr Role role = Role::Change;   ///< a change
 using attribute = A;                         ///< the attribute
 using handle_type = H;                       ///< the handle
 using value_type = V;                        ///< the value
 H h;                                         ///< the handle
 V from;                                      ///< the value before
 V to;                                        ///< the value after
 /// `effect::up` or `effect::down` by the order of `from` and `to`, 0 if
 /// they are equivalent, both if `V` has no order
 [[nodiscard]] constexpr unsigned direction( void ) const {
  if constexpr( std::totally_ordered< V > )
   return( to < from ? effect::down : from < to ? effect::up : 0u );
  else
   return( effects );
  }
 /// whether the two events are the same
 bool operator==( const Changed & ) const = default;
 };

/// the value of an item changed
template< class H , class V >
using Assigned = Changed< Value , H , V >;

/// the key of an item changed
template< class H , class K >
using KeyChanged = Changed< Key , H , K >;

/// the item of handle `from` moved to the handle `to`, for the structures
/// whose handles are not stable
template< class H >
struct Relocated {
 static constexpr unsigned effects = 0u;      ///< no value changes
 static constexpr Role role = Role::Relocate; ///< a relocation
 using handle_type = H;                       ///< the handle
 H from;                                      ///< the handle before
 H to;                                        ///< the handle after
 /// whether the two events are the same
 bool operator==( const Relocated & ) const = default;
 };

/// the items permuted: the handle before of each handle after
template< class H >
struct Permuted {
 static constexpr unsigned effects = 0u;      ///< no value changes
 static constexpr Role role = Role::Permute;  ///< a permutation
 std::span< const H > old_of_new;             ///< the old handle of each
 };

/// the items of the handles `hs` inserted by one bulk operation
template< class H >
struct BulkInserted {
 static constexpr unsigned effects = effect::grows;  ///< it grows
 static constexpr Role role = Role::BulkInsert;      ///< a bulk insertion
 std::span< const H > hs;                            ///< the handles
 };

/// the items of the handles `hs` erased by one bulk operation
template< class H >
struct BulkErased {
 static constexpr unsigned effects = effect::shrinks;  ///< it shrinks
 static constexpr Role role = Role::BulkErase;         ///< a bulk erasure
 std::span< const H > hs;                              ///< the handles
 };

/// every value of the attribute `A` moved by `delta`
template< class A , class D >
struct Shifted {
 static constexpr unsigned effects = effect::up | effect::down;  ///< both
 static constexpr Role role = Role::Shift;   ///< a shift of all
 using attribute = A;                        ///< the attribute
 D delta;                                    ///< the move
 /// `effect::up` or `effect::down` by the sign of `delta`
 [[nodiscard]] constexpr unsigned direction( void ) const {
  return( D() < delta ? effect::up : delta < D() ? effect::down : 0u );
  }
 /// whether the two events are the same
 bool operator==( const Shifted & ) const = default;
 };

/// every value of the attribute `A` replaced
template< class A >
struct Replaced {
 static constexpr unsigned effects = effect::all;  ///< anything
 static constexpr Role role = Role::Replace;       ///< a replacement
 using attribute = A;                              ///< the attribute
 /// whether the two events are the same
 bool operator==( const Replaced & ) const = default;
 };

/// the structure emptied
struct Cleared {
 static constexpr unsigned effects = effect::all;  ///< anything
 static constexpr Role role = Role::Clear;         ///< a clearing
 /// whether the two events are the same
 bool operator==( const Cleared & ) const = default;
 };

/// anything may have changed: forget and recompute
struct Rebuilt {
 static constexpr unsigned effects = effect::all;  ///< anything
 static constexpr Role role = Role::Rebuild;       ///< a rebuilding
 /// whether the two events are the same
 bool operator==( const Rebuilt & ) const = default;
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE INVERSES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the erasure that undoes an insertion
template< class H , class K , class V >
[[nodiscard]] constexpr Erased< H , K , V > inverse(
                                         const Inserted< H , K , V > & e ) {
 if constexpr( std::is_void_v< V > )
  return( Erased< H , K , V >{ e.h , e.key } );
 else
  return( Erased< H , K , V >{ e.h , e.key , e.value } );
 }

/// the insertion that undoes an erasure
template< class H , class K , class V >
[[nodiscard]] constexpr Inserted< H , K , V > inverse(
                                           const Erased< H , K , V > & e ) {
 if constexpr( std::is_void_v< V > )
  return( Inserted< H , K , V >{ e.h , e.key } );
 else
  return( Inserted< H , K , V >{ e.h , e.key , e.value } );
 }

/// the change back
template< class A , class H , class V >
[[nodiscard]] constexpr Changed< A , H , V > inverse(
                                           const Changed< A , H , V > & e ) {
 return( Changed< A , H , V >{ e.h , e.to , e.from } );
 }

/// the move back
template< class H >
[[nodiscard]] constexpr Relocated< H > inverse( const Relocated< H > & e ) {
 return( Relocated< H >{ e.to , e.from } );
 }

/// the shift back
template< class A , class D >
[[nodiscard]] constexpr Shifted< A , D > inverse( const Shifted< A , D > & e )
{
 return( Shifted< A , D >{ D() - e.delta } );
 }

/// an event that has an inverse
template< class E >
concept invertible = requires( const E & e ) { inverse( e ); };

/*--------------------------------------------------------------------------*/
/*------------------------------- THE GROUPS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// what a batch tells `group_begin()`: the versions of the subject before
/// and after it, and the number of its events after the coalescing
struct Group {
 std::uint64_t from = 0;   ///< the version before the batch
 std::uint64_t to = 0;     ///< the version after it
 std::size_t size = 0;     ///< its events
 };

/// the counts of an observer kept by the subject at the level of the audits
struct Counts {
 std::uint64_t events = 0;     ///< the events received
 std::uint64_t groups = 0;     ///< the batches received
 std::uint64_t rebuilds = 0;   ///< the `Rebuilt` received
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ THE OBSERVERS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer of the event `E`
template< class O , class E >
concept observes = requires( O & o , const E & e ) { o.on( e ); };

/// an observer of the spans of `E`, an event or the variant of the events
template< class O , class E >
concept observes_span = requires( O & o , std::span< const E > s ) {
 o.on( s );
 };

/// an observer told of the start and of the end of each batch
template< class O >
concept groups = requires( O & o , const Group & g ) {
                  o.group_begin( g ); o.group_end(); } ||
                 requires( O & o ) { o.group_begin(); o.group_end(); };

/// an observer that may refuse the change `E` before it is made
template< class O , class E >
concept refuses = requires( const O & o , const E & e ) {
 { o.prepare( e ) } -> std::same_as< bool >;
 };

/// an observer that can check itself against the structure \f$ s \f$
template< class O , class S >
concept auditable = requires( const O & o , const S & s ) {
 { o.audit( s ) } -> std::convertible_to< bool >;
 };

/// an observer that queues the events and works at its next query
template< class O >
concept lazy = requires( O & o ) { o.flush(); };

/// an observer that reports counts of its own
template< class O >
concept counted = requires( const O & o ) { o.counts(); };

/// an observer with a filter of the kinds at run time: the bit \f$ i \f$
/// of `mask()` for the \f$ i \f$-th event of the structure
template< class O >
concept masked = requires( const O & o ) {
 { o.mask() } -> std::convertible_to< std::uint64_t >;
 };

/// \cond
namespace detail {

template< class O , class L >
struct any_observed;

template< class O , class ... E >
struct any_observed< O , list< E... > > {
 static constexpr bool value = ( observes< O , E > || ... );
 };

}  // end( namespace detail )
/// \endcond

/// an observer of at least one of the events of the list `L`
template< class O , class L >
concept observer_of = detail::any_observed< O , L >::value;

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ADAPTERS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// how a subject calls an observer: `Direct` calls `on()` and the optional
/// parts by their names; an adapter with the same static functions lets an
/// object with another interface be an observer [see `Via`]
struct Direct {
 /// `o.on( e )`
 template< class O , class E >
  requires observes< O , E >
 static decltype( auto ) one( O & o , const E & e )
  noexcept( noexcept( o.on( e ) ) ) {
  return( o.on( e ) );
  }
 /// `o.on( s )`
 template< class O , class E >
  requires observes_span< O , E >
 static void many( O & o , std::span< const E > s ) { o.on( s ); }
 /// `o.group_begin( g )`, or `o.group_begin()`
 template< class O >
  requires groups< O >
 static void begin( O & o , const Group & g ) {
  if constexpr( requires { o.group_begin( g ); } )
   o.group_begin( g );
  else
   o.group_begin();
  }
 /// `o.group_end()`
 template< class O >
  requires groups< O >
 static void end( O & o ) { o.group_end(); }
 /// `o.prepare( e )`
 template< class O , class E >
  requires refuses< O , E >
 static bool prepare( const O & o , const E & e ) { return( o.prepare( e ) ); }
 /// `o.audit( s )`
 template< class O , class S >
  requires auditable< O , S >
 static bool audit( const O & o , const S & s ) {
  return( bool( o.audit( s ) ) );
  }
 /// `o.mask()`
 template< class O >
  requires masked< O >
 static std::uint64_t mask( const O & o ) {
  return( std::uint64_t( o.mask() ) );
  }
 };

/// the observer `O` called through the adapter `A`, an argument of `Static`
template< class O , class A = Direct >
struct Via {};

/*--------------------------------------------------------------------------*/
/*------------------------------ THE COALESCING ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the net effect of the events of a batch of the structures whose events
/// are the list `L`: by default the rule of \ref theory_ADS_modifications
/// (an insertion and an erasure of one handle cancel, an erasure and an
/// insertion become changes, consecutive changes of one attribute compose,
/// everything before the last `Cleared` or `Rebuilt` is dropped, the
/// shifts add up); a structure with a rule of its own specializes it
template< class L >
struct coalesce_traits {
 /// the net effect of the events \f$ B \f$, in \f$ O( |B| ) \f$ expected
 /// time
 static std::vector< variant_t< L > > coalesce(
                                     std::span< const variant_t< L > > B );
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SUBJECT PARTS ---------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail {

/// the object and the adapter of an argument of `Static`
template< class X >
struct held {
 using object = X;
 using adapter = Direct;
 };

template< class O , class A >
struct held< Via< O , A > > {
 using object = O;
 using adapter = A;
 };

/// whether the adapter `A` calls `on()` of `O` for `E`
template< class A , class O , class E >
concept calls = requires( O & o , const E & e ) {
 A::one( o , e );
 };

/// whether that call returns `Keep`
template< class A , class O , class E >
inline constexpr bool calls_keep = [] {
 if constexpr( calls< A , O , E > )
  return( std::same_as< decltype( A::one( std::declval< O & >() ,
                                          std::declval< const E & >() ) ) ,
                        Keep > );
 else
  return( false );
 }();

/// whether that call does not throw
template< class A , class O , class E >
inline constexpr bool calls_nothrow = [] {
 if constexpr( calls< A , O , E > )
  return( noexcept( A::one( std::declval< O & >() ,
                            std::declval< const E & >() ) ) );
 else
  return( true );
 }();

/// whether the adapter `A` gives `O` the spans of `E`
template< class A , class O , class E >
concept calls_span = requires( O & o ,
                                             std::span< const E > s ) {
 A::many( o , s );
 };

/// whether `O` has the group hooks through `A`
template< class A , class O >
concept calls_group = requires( O & o , const Group & g ) {
 A::begin( o , g ); A::end( o );
 };

/// whether `O` may refuse `E` through `A`
template< class A , class O , class E >
concept calls_prepare = requires( const O & o ,
                                                const E & e ) {
 { A::prepare( o , e ) } -> std::same_as< bool >;
 };

/// whether `O` audits itself against `S` through `A`
template< class A , class O , class S >
concept calls_audit = requires( const O & o , const S & s ) {
 { A::audit( o , s ) } -> std::convertible_to< bool >;
 };

/// whether `O` has a mask through `A`
template< class A , class O >
concept calls_mask = requires( const O & o ) {
 { A::mask( o ) } -> std::convertible_to< std::uint64_t >;
 };

/// the kinds of the list `L` that `O` wants through `A`, as bits
template< class A , class O , class L >
struct kinds_of;

template< class A , class O , class ... E >
struct kinds_of< A , O , list< E... > > {
 static constexpr std::uint64_t value = [] {
  std::uint64_t m = 0;
  std::size_t i = 0;
  ( ( m |= ( calls< A , O , E > ? ( std::uint64_t( 1 ) << i ) : 0 ) ,
      ++i ) , ... );
  return( m );
  }();
 static constexpr bool keep = ( calls_keep< A , O , E > || ... );
 static constexpr bool nothrow_ok =
  ( ( ! must_not_throw< E > || calls_nothrow< A , O , E > ) && ... );
 };

/// whether every type of the list `R` is in the list `L`
template< class R , class L >
struct all_in;

template< class ... R , class L >
struct all_in< list< R... > , L > {
 static constexpr bool value = ( contains_v< R , L > && ... );
 };

/// the events `O` requires, `list<>` if none
template< class O >
struct required {
 using type = list<>;
 };

template< class O >
 requires requires { typename O::requires_modifications; }
struct required< O > {
 using type = typename O::requires_modifications;
 };

/// the checks of an observer `O` called through `A` by a structure of the
/// events `L`, at compile time
template< class A , class O , class L >
constexpr void check( void )
{
 static_assert( kinds_of< A , O , L >::value != 0 ,
                "ADS: the observer has an on() for none of the events of "
                "the structure" );
 static_assert( all_in< typename required< O >::type , L >::value ,
                "ADS: the observer requires an event that the structure "
                "never emits" );
 static_assert( kinds_of< A , O , L >::nothrow_ok ,
                "ADS: on() of Erased, BulkErased, Cleared, Rebuilt and "
                "Replaced must be noexcept" );
 }

/// calls f.template operator()< E >() for the event E of index k of L
template< class L , class F >
void visitKind( std::size_t k , F && f );

/// the events of the run R, all of kind E, given to o through A: as one
/// span when it takes them, one by one otherwise; whether it asked to leave
template< class A , class O , class E , class Var >
bool giveRun( O & o , std::span< const Var > R );

/// the detached static observers, as bits, kept only if one may leave
template< bool B >
struct OffBits {
 [[nodiscard]] constexpr bool off( std::size_t ) const { return( false ); }
 constexpr void set( std::size_t ) {}
 };

template<>
struct OffBits< true > {
 std::uint64_t bits = 0;
 [[nodiscard]] bool off( std::size_t i ) const {
  return( ( bits >> i ) & 1u );
  }
 void set( std::size_t i ) { bits |= std::uint64_t( 1 ) << i; }
 };

/// the counts of the static observers, kept only at the level of the
/// audits
template< std::size_t N , bool B = ( ADS_CONTRACTS >= 3 ) >
struct StaticCounts {
 [[nodiscard]] static Counts at( std::size_t ) { return( Counts() ); }
 static void event( std::size_t ) {}
 static void group( std::size_t ) {}
 static void rebuild( std::size_t ) {}
 };

template< std::size_t N >
struct StaticCounts< N , true > {
 std::array< Counts , N > c{};
 [[nodiscard]] Counts at( std::size_t i ) const { return( c[ i ] ); }
 void event( std::size_t i ) { ++c[ i ].events; }
 void group( std::size_t i ) { ++c[ i ].groups; }
 void rebuild( std::size_t i ) { ++c[ i ].rebuilds; }
 };

/// where an event outside a batch got to: the static observers told, as
/// bits, and the dynamic slot that threw (the size if none)
struct Trail {
 std::uint64_t st = 0;
 std::size_t dy = 0;
 };

/// the part of a subject with no static observers
template< class L >
struct NoStatic {
 template< class E >
 static constexpr bool listened = false;
 template< class ... A >
 constexpr explicit NoStatic( A & ... ) {
  static_assert( sizeof...( A ) == 0 ,
                 "ADS: observers given to a subject with no static part" );
  }
 template< class E >
 [[nodiscard]] constexpr bool live( void ) const { return( false ); }
 [[nodiscard]] constexpr std::uint64_t wanted( void ) const { return( 0 ); }
 template< class E , bool T >
 constexpr void deliver( const E & , Trail & ) {}
 template< class E , class Inv >
 constexpr void undo( const Inv & , const Trail & ) noexcept {}
 template< class E >
 [[nodiscard]] constexpr bool prepare( const E & ) const { return( true ); }
 template< class S >
 [[nodiscard]] constexpr bool audit( const S & ) const { return( true ); }
 template< class Var >
 constexpr void deliverGroup( std::span< const Var > , std::uint64_t ,
                              const Group & , std::vector< Var > & ) {}
 };

/// the static observers `X...` of a structure of the events `L` and of
/// the observed object `Obs`
template< class L , class Obs , class ... X >
class StaticSet {

 static constexpr std::size_t N = sizeof...( X );
 using Objects = std::tuple< typename held< X >::object & ... >;
 template< std::size_t I >
 using Xi = std::tuple_element_t< I , std::tuple< X... > >;
 template< std::size_t I >
 using Oi = typename held< Xi< I > >::object;
 template< std::size_t I >
 using Ai = typename held< Xi< I > >::adapter;

 static constexpr bool may_leave =
  ( kinds_of< typename held< X >::adapter , typename held< X >::object ,
              L >::keep || ... );

 public:

 template< class E >
 static constexpr bool listened =
  ( calls< typename held< X >::adapter , typename held< X >::object , E >
    || ... );

 template< class ... A >
 explicit StaticSet( A & ... a ) : f_o( a... ) {
  static_assert( N <= 64 , "ADS: at most 64 static observers" );
  ( check< typename held< X >::adapter , typename held< X >::object ,
           L >() , ... );
  checkOrder( std::make_index_sequence< N >() );
  }

 /// whether the observer I wants E now
 template< std::size_t I , class E >
 [[nodiscard]] bool wants( void ) const {
  if constexpr( ! calls< Ai< I > , Oi< I > , E > )
   return( false );
  else {
   bool w = true;
   if constexpr( calls_mask< Ai< I > , Oi< I > > )
    w = ( Ai< I >::mask( std::get< I >( f_o ) ) >> index_v< E , L > ) & 1u;
   if constexpr( may_leave )
    w = w && ! f_off.off( I );
   return( w );
   }
  }

 template< class E >
 [[nodiscard]] bool live( void ) const {
  return( [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   return( ( wants< I , E >() || ... ) ); }(
           std::make_index_sequence< N >() ) );
  }

 /// the kinds wanted now by someone, as bits
 [[nodiscard]] std::uint64_t wanted( void ) const {
  return( [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   return( ( wantedBy< I >() | ... | std::uint64_t( 0 ) ) ); }(
           std::make_index_sequence< N >() ) );
  }

 template< class E , bool T >
 void deliver( const E & e , Trail & t ) {
  [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   ( step< I , T >( e , t ) , ... ); }( std::make_index_sequence< N >() );
  }

 template< class E , class Inv >
 void undo( const Inv & e , const Trail & t ) noexcept {
  std::uint64_t leave = 0;
  [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   ( back< N - 1 - I , E >( e , t , leave ) , ... ); }(
     std::make_index_sequence< N >() );
  if constexpr( may_leave )
   for( std::size_t i = 0 ; i < N ; ++i )
    if( ( leave >> i ) & 1u )
     f_off.set( i );
  }

 template< class E >
 [[nodiscard]] bool prepare( const E & e ) const {
  return( [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   return( ( prepareOne< I >( e ) && ... ) ); }(
           std::make_index_sequence< N >() ) );
  }

 template< class S >
 [[nodiscard]] bool audit( const S & s ) const {
  return( [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   return( ( auditOne< I >( s ) && ... ) ); }(
           std::make_index_sequence< N >() ) );
  }

 template< class Var >
 void deliverGroup( std::span< const Var > C , std::uint64_t kinds ,
                    const Group & g , std::vector< Var > & tmp ) {
  [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
   ( groupOne< I >( C , kinds , g , tmp ) , ... ); }(
     std::make_index_sequence< N >() );
  }

 [[nodiscard]] Counts counts( std::size_t i ) const {
  return( f_counts.at( i ) );
  }

 private:

 template< std::size_t I >
 [[nodiscard]] std::uint64_t wantedBy( void ) const {
  std::uint64_t m = kinds_of< Ai< I > , Oi< I > , L >::value;
  if constexpr( calls_mask< Ai< I > , Oi< I > > )
   m &= Ai< I >::mask( std::get< I >( f_o ) );
  if constexpr( may_leave )
   if( f_off.off( I ) )
    m = 0;
  return( m );
  }

 template< std::size_t I , bool T , class E >
 void step( const E & e , Trail & t ) {
  if constexpr( calls< Ai< I > , Oi< I > , E > ) {
   if( ! wants< I , E >() )
    return;
   f_counts.event( I );
   if constexpr( std::is_same_v< E , Rebuilt > )
    f_counts.rebuild( I );
   if constexpr( calls_keep< Ai< I > , Oi< I > , E > ) {
    if( Ai< I >::one( std::get< I >( f_o ) , e ) == Keep::Detach )
     f_off.set( I );
    }
   else
    Ai< I >::one( std::get< I >( f_o ) , e );
   if constexpr( T )
    t.st |= std::uint64_t( 1 ) << I;
   }
  }

 template< std::size_t I , class E , class Inv >
 void back( const Inv & e , const Trail & t ,
            std::uint64_t & leave ) noexcept {
  if constexpr( calls< Ai< I > , Oi< I > , Inv > ) {
   if( ( ( t.st >> I ) & 1u ) == 0 )
    return;
   try {
    if constexpr( calls_keep< Ai< I > , Oi< I > , Inv > ) {
     if( Ai< I >::one( std::get< I >( f_o ) , e ) == Keep::Detach )
      leave |= std::uint64_t( 1 ) << I;
     }
    else
     Ai< I >::one( std::get< I >( f_o ) , e );
    }
   catch( ... ) {}                      // the first exception wins
   }
  }

 template< std::size_t I , class E >
 [[nodiscard]] bool prepareOne( const E & e ) const {
  if constexpr( calls_prepare< Ai< I > , Oi< I > , E > ) {
   if( ! wants< I , E >() )
    return( true );
   return( Ai< I >::prepare( std::get< I >( f_o ) , e ) );
   }
  else
   return( true );
  }

 template< std::size_t I , class S >
 [[nodiscard]] bool auditOne( const S & s ) const {
  if constexpr( calls_audit< Ai< I > , Oi< I > , S > )
   return( Ai< I >::audit( std::get< I >( f_o ) , s ) );
  else
   return( true );
  }

 template< std::size_t I , class Var >
 void groupOne( std::span< const Var > C , std::uint64_t kinds ,
                const Group & g , std::vector< Var > & tmp ) {
  const std::uint64_t m = wantedBy< I >();
  if( ( m & kinds ) == 0 )
   return;
  auto & o = std::get< I >( f_o );
  f_counts.group( I );
  if constexpr( calls_group< Ai< I > , Oi< I > > )
   Ai< I >::begin( o , g );
  bool leave = false;
  if constexpr( calls_span< Ai< I > , Oi< I > , Var > ) {
   if( ( kinds & ~m ) == 0 )
    Ai< I >::many( o , C );
   else {
    tmp.clear();
    for( const auto & e : C )
     if( ( m >> e.index() ) & 1u )
      tmp.push_back( e );
    Ai< I >::many( o , std::span< const Var >( tmp ) );
    }
   }
  else {
   std::size_t i = 0;
   while( i < C.size() ) {
    const std::size_t k = C[ i ].index();
    std::size_t j = i + 1;
    while( ( j < C.size() ) && ( C[ j ].index() == k ) )
     ++j;
    if( ( m >> k ) & 1u )
     visitKind< L >( k , [ & ]< class E >() {
      if constexpr( calls< Ai< I > , Oi< I > , E > )
       leave = giveRun< Ai< I > , Oi< I > , E >( o , C.subspan( i ,
                                                                j - i ) ) ||
               leave; } );
    i = j;
    }
   }
  if constexpr( calls_group< Ai< I > , Oi< I > > )
   Ai< I >::end( o );
  if constexpr( may_leave )
   if( leave )
    f_off.set( I );
  }

 template< std::size_t ... I >
 static constexpr void checkOrder( std::index_sequence< I... > ) {
  ( checkDeps< I >() , ... );
  }

 template< std::size_t I >
 static constexpr void checkDeps( void ) {
  if constexpr( requires { typename Oi< I >::depends_on; } )
   static_assert( precede< I >( typename Oi< I >::depends_on() ) ,
                  "ADS: a static observer precedes one it depends on" );
  }

 template< std::size_t I , class ... D >
 static constexpr bool precede( list< D... > ) {
  return( ( before< I , D >() && ... ) );
  }

 template< std::size_t I , class D >
 static constexpr bool before( void ) {
  return( [] < std::size_t ... J >( std::index_sequence< J... > ) {
   return( ( std::is_same_v< Oi< J > , D > || ... ) ); }(
           std::make_index_sequence< I >() ) );
  }

 Objects f_o;
 ADS_NO_UNIQUE_ADDRESS OffBits< may_leave > f_off;
 ADS_NO_UNIQUE_ADDRESS StaticCounts< N > f_counts;

 };  // end( class StaticSet )

/// the part of a subject with no dynamic observers
template< class L >
struct NoSlots {
 static constexpr bool slots = false;
 template< class E >
 static constexpr bool listened = false;
 template< class E >
 [[nodiscard]] constexpr bool live( void ) const { return( false ); }
 [[nodiscard]] constexpr std::uint64_t wanted( void ) const { return( 0 ); }
 template< class E , bool T >
 constexpr void deliver( const E & , Trail & ) {}
 template< class E , class Inv >
 constexpr void undo( const Inv & , const Trail & ) noexcept {}
 template< class E >
 [[nodiscard]] constexpr bool prepare( const E & ) const { return( true ); }
 template< class S >
 [[nodiscard]] constexpr bool audit( const S & ) const { return( true ); }
 template< class Var >
 constexpr void deliverGroup( std::span< const Var > , std::uint64_t ,
                              const Group & , std::vector< Var > & ) {}
 };

/// the dynamic observers of a structure of the events `L` and of the
/// observed object `Obs`, each in a slot of erased type, in the order of
/// their attachment
template< class L , class Obs >
class Slots {

 static constexpr std::size_t K = L::size;
 using Var = variant_t< L >;

 /// an observer, its type erased: one function per kind of event
 struct Slot {
  void * o = nullptr;                  ///< the observer, null if gone
  std::uint64_t mask = 0;              ///< the kinds it wants
  /// its `on( e )` of each kind
  std::array< Keep ( * )( void * , const void * ) , K > one{};
  /// its `prepare( e )` of each kind
  std::array< bool ( * )( const void * , const void * ) , K > prep{};
  /// a run of events of each kind
  std::array< bool ( * )( void * , std::span< const Var > ) , K > run{};
  /// its `on( s )` of the variant
  void ( * all )( void * , std::span< const Var > ) = nullptr;
  /// its group hooks
  void ( * begin )( void * , const Group & ) = nullptr;
  void ( * end )( void * ) = nullptr;   ///< its `group_end()`
  /// its `audit( s )`
  bool ( * audit )( const void * , const void * ) = nullptr;
  Counts counts;                       ///< its counts
  };

 public:

 static constexpr bool slots = true;

 template< class E >
 static constexpr bool listened = contains_v< E , L >;

 template< class E >
 [[nodiscard]] bool live( void ) const {
  return( ( f_mask >> index_v< E , L > ) & 1u );
  }

 [[nodiscard]] std::uint64_t wanted( void ) const { return( f_mask ); }

 template< class A , class D >
 int attach( D & o );

 void detach( int id );

 [[nodiscard]] bool attached( int id ) const {
  return( ( id >= 0 ) && ( id < int( f_s.size() ) ) &&
          ( f_s[ std::size_t( id ) ].o != nullptr ) );
  }

 template< class E , bool T >
 void deliver( const E & e , Trail & t );

 template< class E , class Inv >
 void undo( const Inv & inv , const Trail & t ) noexcept;

 template< class E >
 [[nodiscard]] bool prepare( const E & e ) const;

 template< class S >
 [[nodiscard]] bool audit( const S & s ) const;

 void deliverGroup( std::span< const Var > C , std::uint64_t kinds ,
                    const Group & g , std::vector< Var > & tmp );

 [[nodiscard]] Counts counts( int id ) const {
  return( f_s[ std::size_t( id ) ].counts );
  }

 /// whether a slot is being called, when attach() and detach() are not
 /// allowed
 bool f_busy = false;

 private:

 /// the functions of the kind I of the observer D called through A
 template< class A , class D , std::size_t I >
 static void fill( Slot & s );

 /// the union of the masks
 void remask( void );

 std::vector< Slot > f_s;          ///< the slots
 std::uint64_t f_mask = 0;         ///< the union of their masks

 };  // end( class Slots )

/*--------------------------------------------------------------------------*/
/*------------------------------- THE SUBJECTS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the subject of `NoObservers`: no member and no code
template< class L >
struct NoSubject {
 static constexpr bool none = true;      ///< nobody may listen
 static constexpr bool dynamic = false;  ///< no attachment
 using modifications = L;                ///< the events
 template< class E >
 static constexpr bool listened = false;
 constexpr NoSubject( void ) = default;
 template< class E >
 [[nodiscard]] constexpr bool live( void ) const { return( false ); }
 template< class E >
 constexpr void emit( const E & ) {}
 template< class E , class U >
 constexpr void emit( const E & , U && ) {}
 template< class E >
 constexpr void prepare( const E & ) const {}
 template< class S >
 constexpr void audit( const S & ) const {}
 constexpr void open( void ) {}
 template< class S >
 constexpr void close( const S & ) {}
 [[nodiscard]] constexpr std::size_t buffered( void ) const { return( 0 ); }
 constexpr void discard( std::size_t ) {}
 constexpr void spoil( void ) {}
 constexpr void mute( bool ) {}
 template< class S >
 constexpr void unmute( const S & ) {}
 [[nodiscard]] constexpr int depth( void ) const { return( 0 ); }
 [[nodiscard]] constexpr bool muted( void ) const { return( false ); }
 [[nodiscard]] constexpr std::size_t built( void ) const { return( 0 ); }
 [[nodiscard]] constexpr std::uint64_t mask( void ) const { return( 0 ); }
 template< class E >
 [[nodiscard]] constexpr std::uint64_t revision( void ) const { return( 0 ); }
 [[nodiscard]] constexpr std::uint64_t version( void ) const { return( 0 ); }
 };

/// a subject of the events `L` and of the observed object `Obs`, with the
/// static part `St` and the dynamic part `Dy` [see `Static`, `Dynamic`,
/// `Both`]
template< class L , class Obs , class St , class Dy >
class Subject {

 static constexpr std::size_t K = L::size;
 static_assert( K <= 64 , "ADS: at most 64 kinds of event" );

 public:

 static constexpr bool none = false;            ///< someone may listen
 static constexpr bool dynamic = Dy::slots;     ///< attachment allowed
 using modifications = L;                       ///< the events
 using Event = variant_t< L >;                  ///< one of them

 /// whether someone may want `E`, at compile time
 template< class E >
 static constexpr bool listened = St::template listened< E > ||
                                  Dy::template listened< E >;

 /// the subject of the static observers `o...`
 template< class ... A >
 explicit Subject( A & ... a ) : f_st( a... ) {}

 /// not copied
 Subject( const Subject & ) = delete;
 /// not assigned
 Subject & operator=( const Subject & ) = delete;

 /// whether someone wants `E` now
 template< class E >
 [[nodiscard]] bool live( void ) const {
  return( f_st.template live< E >() || f_dy.template live< E >() );
  }

 /// tells \f$ e \f$, or collects it in a batch; nothing when muted
 template< class E >
 void emit( const E & e ) {
  static_assert( contains_v< E , L > ,
                 "ADS: an event that the structure does not declare" );
  ++f_built;
  if( ( f_depth | f_quiet ) != 0 ) [[unlikely]] {
   defer( e );
   return;
   }
  if( ! live< E >() )
   return;
  ++f_rev[ index_v< E , L > ];
  Trail t;
  if constexpr( dynamic ) {
   f_st.template deliver< E , false >( e , t );
   f_dy.f_busy = true;
   struct Done {
    bool & b;
    ~Done() { b = false; }
    } done{ f_dy.f_busy };
   f_dy.template deliver< E , false >( e , t );
   }
  else
   f_st.template deliver< E , false >( e , t );
  }

 /// tells \f$ e \f$ with the strong guarantee: if an observer throws, the
 /// inverse of \f$ e \f$ goes to those already told, in reverse order,
 /// `undo()` (which must not throw) undoes the change of the structure,
 /// and the exception goes on
 template< class E , class U >
 void emit( const E & e , U && undo );

 /// the `prepare( e )` of every observer, before the change: `Refused` if
 /// one refuses, and nothing changed
 template< class E >
 void prepare( const E & e ) const {
  if constexpr( listened< E > )
   if( ! ( f_st.prepare( e ) && f_dy.prepare( e ) ) )
    throw( Refused( "ADS: a change refused by an observer" ) );
  }

 /// `audit( s )` of every observer that has it, at the level of the
 /// audits, outside batches and mutes
 template< class S >
 void audit( [[maybe_unused]] const S & s ) const {
  if constexpr( ADS_CONTRACTS >= 3 ) {
   if( ( f_depth | f_quiet ) != 0 )
    return;
   const bool ok = f_st.audit( s ) && f_dy.audit( s );
   ADS_AUDIT( ok );
   }
  }

/*-------------------------------- BATCHES ---------------------------------*/

 /// opens a batch: the events are collected until it is closed
 void open( void ) { ++f_depth; }

 /// closes a batch: the outermost one sends the events, coalesced, and
 /// audits the observers against \f$ s \f$
 template< class S >
 void close( const S & s );

 /// the events collected so far
 [[nodiscard]] std::size_t buffered( void ) const { return( f_buf.size() ); }

 /// closes a batch, dropping the events collected after the first
 /// \f$ m \f$
 void discard( std::size_t m );

 /// drops the events collected, and collects `Rebuilt` in their place
 void spoil( void ) {
  f_buf.clear();
  f_keep.clear();
  if constexpr( contains_v< Rebuilt , L > )
   f_buf.emplace_back( Rebuilt{} );
  }

 /// the depth of the batches open, \f$ 0 \f$ if none
 [[nodiscard]] int depth( void ) const { return( f_depth ); }

/*--------------------------------- MUTES ----------------------------------*/

 /// suspends the events; with `rebuild`, `Rebuilt` at the end
 void mute( bool rebuild );

 /// ends a mute: `Rebuilt` if asked by one of the mutes nested, the audit
 /// against \f$ s \f$ otherwise
 template< class S >
 void unmute( const S & s );

 /// whether the events are suspended
 [[nodiscard]] bool muted( void ) const { return( f_quiet > 0 ); }

/*------------------------------- THE COUNTS -------------------------------*/

 /// the events built so far, sent, collected or muted
 [[nodiscard]] std::size_t built( void ) const { return( f_built ); }

 /// the kinds wanted now by someone, as bits
 [[nodiscard]] std::uint64_t mask( void ) const {
  return( f_st.wanted() | f_dy.wanted() );
  }

 /// the revision of the kind `E`: the events of that kind delivered to
 /// someone
 template< class E >
 [[nodiscard]] std::uint64_t revision( void ) const {
  return( f_rev[ index_v< E , L > ] );
  }

 /// the revision of the kind \f$ k \f$
 [[nodiscard]] std::uint64_t revision( std::size_t k ) const {
  return( f_rev[ k ] );
  }

 /// the version: the sum of the revisions
 [[nodiscard]] std::uint64_t version( void ) const {
  std::uint64_t v = 0;
  for( const auto r : f_rev )
   v += r;
  return( v );
  }

 /// the counts of the static observer \f$ i \f$ (zero below the level of
 /// the audits)
 [[nodiscard]] Counts staticCounts( std::size_t i ) const
  requires( ! std::is_same_v< St , NoStatic< L > > ) {
  return( f_st.counts( i ) );
  }

/*--------------------------- THE DYNAMIC OBSERVERS ------------------------*/

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open, no observer is being told
 template< class D >
 int attach( D & o ) requires dynamic {
  ADS_PRE( ( f_depth == 0 ) && ! f_dy.f_busy );
  return( f_dy.template attach< Direct >( o ) );
  }

 /// attaches the observer \f$ o \f$ called through the adapter `A`
 template< class A , class D >
 int attachVia( D & o ) requires dynamic {
  ADS_PRE( ( f_depth == 0 ) && ! f_dy.f_busy );
  return( f_dy.template attach< A >( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open, no observer is being told
 void detach( int id ) requires dynamic {
  ADS_PRE( ( f_depth == 0 ) && ! f_dy.f_busy && f_dy.attached( id ) );
  f_dy.detach( id );
  }

 /// whether the observer number \f$ \mathit{id} \f$ is attached
 [[nodiscard]] bool attached( int id ) const requires dynamic {
  return( f_dy.attached( id ) );
  }

 /// the counts of the dynamic observer \f$ \mathit{id} \f$
 [[nodiscard]] Counts slotCounts( int id ) const requires dynamic {
  return( f_dy.counts( id ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// collects \f$ e \f$ in the batch, unless muted, out of the hot code
 template< class E >
 ADS_COLD_PATH void defer( const E & e );

 /// sends the batch coalesced
 void flush( void );

 /// sends `Rebuilt` to everyone who wants it
 void rebuilt( void );

 ADS_NO_UNIQUE_ADDRESS St f_st;          ///< the static observers
 ADS_NO_UNIQUE_ADDRESS Dy f_dy;          ///< the dynamic ones
 int f_depth = 0;                        ///< the batches open
 int f_quiet = 0;                        ///< the mutes open
 bool f_rebuild = false;                 ///< whether a mute asked Rebuilt
 std::size_t f_built = 0;                ///< the events built
 std::vector< Event > f_buf;             ///< the events collected
 /// the copies of the handles of the bulk events collected
 std::vector< std::shared_ptr< const void > > f_keep;
 std::array< std::uint64_t , K > f_rev{};   ///< the revisions

 };  // end( class Subject )

/// the events `M` of a structure, checked against the list `L` given to
/// `Dynamic`, if any
template< class L , class M >
struct events_of {
 static_assert( std::is_void_v< L > || std::is_same_v< L , M > ,
                "ADS: Dynamic< L > on a structure whose events are not L" );
 using type = M;
 };

/// whether \f$ s \f$ can go back to a checkpoint
template< class S >
concept undoable = requires( S & s ) { s.rollback( s.checkpoint() ); };

/// the type of the checkpoints of \f$ s \f$, `int` if it has none
template< class S >
struct checkpoint_of {
 using type = int;
 };

template< class S >
 requires undoable< S >
struct checkpoint_of< S > {
 using type = decltype( std::declval< S & >().checkpoint() );
 };

/// what the audits of the observers of \f$ s \f$ receive: `s.observed()`
/// if it has it, \f$ s \f$ otherwise
template< class S >
decltype( auto ) observed( S & s )
{
 if constexpr( requires { s.observed(); } )
  return( s.observed() );
 else
  return( std::as_const( s ) );
 }

}  // end( namespace detail )
/// \endcond

}  // end( namespace mod )

/*--------------------------------------------------------------------------*/
/*------------------------------ THE POLICIES ------------------------------*/
/*--------------------------------------------------------------------------*/

/// nobody listens: the subject is empty and every test is false at compile
/// time, so that the structure compiles to its plain code
struct NoObservers {
 /// the subject of a structure of the events `L`
 template< class L , class Obs = void >
 using bind = mod::detail::NoSubject< L >;
 };

/// the static observers `O...` (or `mod::Via< O , A >`), references called
/// directly in the order of the arguments
template< class ... O >
struct Static {
 /// the subject of a structure of the events `L`
 template< class L , class Obs = void >
 using bind = mod::detail::Subject< L , Obs ,
                                    mod::detail::StaticSet< L , Obs , O... > ,
                                    mod::detail::NoSlots< L > >;
 };

/// observers attached at run time, the events those of the structure (or
/// the list `L`, which must be them)
template< class L = void >
struct Dynamic {
 /// the subject of a structure of the events `M`
 template< class M , class Obs = void >
 using bind = mod::detail::Subject<
               typename mod::detail::events_of< L , M >::type , Obs ,
               mod::detail::NoStatic< M > , mod::detail::Slots< M , Obs > >;
 };

/// the static observers of `St`, then those attached at run time
template< class St , class Dy = Dynamic<> >
struct Both;

/// the static observers `O...`, then those attached at run time
template< class ... O , class L >
struct Both< Static< O... > , Dynamic< L > > {
 /// the subject of a structure of the events `M`
 template< class M , class Obs = void >
 using bind = mod::detail::Subject<
               typename mod::detail::events_of< L , M >::type , Obs ,
                                    mod::detail::StaticSet< M , Obs , O... > ,
                                    mod::detail::Slots< M , Obs > >;
 };

/*--------------------------------------------------------------------------*/
/*----------------------------- BATCHES AND MUTES --------------------------*/
/*--------------------------------------------------------------------------*/

/// a batch of RAII on the structure \f$ s \f$ [see batch()]
template< class S >
class Batch {

 using T = typename mod::detail::checkpoint_of< S >::type;

 public:

 /// opens a batch on \f$ s \f$
 explicit Batch( S & s )
  : f_s( & s ) , f_ex( std::uncaught_exceptions() ) ,
    f_mark( s.subject().buffered() ) {
  if constexpr( mod::detail::undoable< S > )
   f_t = s.checkpoint();
  s.subject().open();
  }

 /// the batch of \f$ b \f$, which no longer has it
 Batch( Batch && b ) noexcept
  : f_s( std::exchange( b.f_s , nullptr ) ) , f_ex( b.f_ex ) ,
    f_mark( b.f_mark ) , f_t( b.f_t ) {}

 /// not copied
 Batch( const Batch & ) = delete;
 /// not assigned
 Batch & operator=( const Batch & ) = delete;
 /// not assigned
 Batch & operator=( Batch && ) = delete;

 /// closes the batch, if still open; if its scope exits by an exception,
 /// the structure goes back to the checkpoint when it is undoable and the
 /// events are dropped, else the observers get `Rebuilt` (or the events,
 /// if the structure has no `Rebuilt`)
 ~Batch() noexcept( false ) {
  if( ! f_s )
   return;
  if( std::uncaught_exceptions() > f_ex )
   abandon();
  else
   close();
  }

 /// closes the batch: the outermost one sends the events, coalesced;
 /// idempotent
 ///
 /// In pseudocode (beyond the book), with \f$ d \f$ the depth of the
 /// batches, \f$ B \f$ the events collected and \f$ M_o \f$ the kinds that
 /// the observer \f$ o \f$ wants:
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
 ///     \ o.\mathrm{group\_begin}();\ o.\mathrm{on}( C_o );
 ///     \ o.\mathrm{group\_end}()
 /// \end{array}}
 /// \f]
 void close( void ) {
  if( f_s ) {
   S * s = std::exchange( f_s , nullptr );
   s->subject().close( mod::detail::observed( *s ) );
   }
  }

 /// brings the structure back to where it was when the batch opened, and
 /// drops its events: the observers see nothing
 void rollback( void ) requires mod::detail::undoable< S > {
  if( f_s )
   undo( *std::exchange( f_s , nullptr ) );
  }

 private:

 void abandon( void ) noexcept {
  S * s = std::exchange( f_s , nullptr );
  using L = typename std::remove_cvref_t<
                     decltype( s->subject() ) >::modifications;
  if constexpr( mod::detail::undoable< S > )
   undo( *s );
  else {
   if constexpr( mod::contains_v< mod::Rebuilt , L > )
    s->subject().spoil();
   try {
    s->subject().close( mod::detail::observed( *s ) );
    }
   catch( ... ) {}                      // the first exception wins
   }
  }

 void undo( S & s ) noexcept {
  auto & sub = s.subject();
  sub.mute( false );                    // the undo tells nobody
  try {
   s.rollback( f_t );
   }
  catch( ... ) {}
  try {
   sub.unmute( mod::detail::observed( s ) );   // in the batch: silent
   }
  catch( ... ) {}
  sub.discard( f_mark );
  }

 S * f_s;                       ///< the structure, if open
 int f_ex;                      ///< the exceptions in flight at the open
 std::size_t f_mark;            ///< the events collected before the open
 T f_t{};                       ///< the checkpoint, if undoable

 };  // end( class Batch )

/// a batch on the structure \f$ s \f$, open while it lives: its events are
/// collected, and the outermost batch sends their net effect at its end
template< class S >
[[nodiscard]] Batch< S > batch( S & s )
{
 return( Batch< S >( s ) );
 }

/// a mute of RAII on the structure \f$ s \f$ [see quiet()]
template< class S >
class Quiet {
 public:

 /// suspends the events of \f$ s \f$
 Quiet( S & s , bool rebuild ) : f_s( & s ) { s.subject().mute( rebuild ); }

 /// not copied
 Quiet( const Quiet & ) = delete;
 /// not assigned
 Quiet & operator=( const Quiet & ) = delete;

 /// ends the mute, if not yet ended
 ~Quiet() noexcept( false ) { end(); }

 /// ends the mute: `Rebuilt`, or the audit; idempotent
 void end( void ) {
  if( f_s ) {
   S * s = std::exchange( f_s , nullptr );
   s->subject().unmute( mod::detail::observed( *s ) );
   }
  }

 private:

 S * f_s;                       ///< the structure, if muted

 };  // end( class Quiet )

/// a mute on the structure \f$ s \f$: no event while it lives, `Rebuilt`
/// to every observer at its end (inside a batch, the events collected
/// before are dropped, and the batch sends `Rebuilt` and the events
/// after the mute)
template< class S >
[[nodiscard]] Quiet< S > quiet( S & s )
{
 using L = typename std::remove_cvref_t<
                    decltype( s.subject() ) >::modifications;
 static_assert( mod::contains_v< mod::Rebuilt , L > ,
                "ADS: quiet( s ) sends Rebuilt, which the structure does not "
                "emit; quiet( s , no_rebuild ) promises that nobody needs to "
                "know" );
 return( Quiet< S >( s , true ) );
 }

/// a mute on the structure \f$ s \f$ that nobody needs to know: no event,
/// and at the level of the audits the observers audited at its end
template< class S >
[[nodiscard]] Quiet< S > quiet( S & s , no_rebuild_t )
{
 return( Quiet< S >( s , false ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS UndoLog -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer that records the invertible events of `L`, which carry the
/// old values, and brings a structure back to a checkpoint by replaying
/// their inverses through its `apply()`, in one batch: any structure with
/// `apply()` becomes undoable, at one event per change
///
/// In pseudocode (beyond the book), with \f$ R \f$ the events recorded:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{rollback}( s , t ) \cr
/// 2 & \quad \text{open a batch on } s;\ \text{stop recording} \cr
/// 3 & \quad \textbf{while}\ |R| > t\ \textbf{do} \cr
/// 4 & \qquad e \gets \text{the last event of } R;\ \text{remove it};
///     \ s.\mathrm{apply}( e^{-1} ) \cr
/// 5 & \quad \text{close the batch};\ \text{record again}
/// \end{array}}
/// \f]
template< class L >
class UndoLog {
 public:

 using Event = mod::variant_t< L >;   ///< an event recorded

 /// records \f$ e \f$
 template< class E >
  requires( mod::contains_v< E , L > && mod::invertible< E > )
 void on( const E & e ) noexcept( mod::must_not_throw< E > ) {
  if( f_replay )
   return;
  if constexpr( mod::must_not_throw< E > ) {
   try {
    f_log.push_back( Event( e ) );
    }
   catch( ... ) {                       // a hole: no rollback before it
    forget();
    ++f_base;
    f_floor = f_base;
    }
   }
  else
   f_log.push_back( Event( e ) );
  }

 /// forgets the events recorded, which can no longer be undone
 void on( const mod::Cleared & ) noexcept { forget(); }
 /// forgets the events recorded
 void on( const mod::Rebuilt & ) noexcept { forget(); }
 /// forgets the events recorded
 template< class A >
 void on( const mod::Replaced< A > & ) noexcept { forget(); }

 /// the checkpoint of now
 [[nodiscard]] std::size_t checkpoint( void ) const {
  return( f_base + f_log.size() );
  }

 /// the events recorded since the oldest checkpoint still valid
 [[nodiscard]] std::size_t size( void ) const { return( f_log.size() ); }

 /// \f$ s \f$ back to the checkpoint \f$ t \f$, by `s.apply()` of the
 /// inverses, in one batch, so that the other observers get their net
 /// effect
 /// \pre \f$ t \f$ is a checkpoint of this log, not older than its last
 ///      `Cleared`, `Rebuilt` or `Replaced`
 template< class S >
 void rollback( S & s , std::size_t t );

 private:

 void forget( void ) noexcept {
  f_base += f_log.size();
  f_log.clear();
  f_floor = f_base;
  }

 std::vector< Event > f_log;    ///< the events recorded
 std::size_t f_base = 0;        ///< the checkpoint of the first of them
 std::size_t f_floor = 0;       ///< the oldest checkpoint valid
 bool f_replay = false;         ///< whether a rollback is replaying

 };  // end( class UndoLog )

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Shadow ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an observer that forwards every event to an incremental observer and
/// to one that computes from scratch, and compares their answers at each
/// query by `Eq`: an audit of the incremental one by recomputation, at a
/// cost of about twice the work
template< class Inc , class Scratch , class Eq = std::equal_to<> >
class Shadow {

 template< class E >
 static constexpr bool forwarded =
  mod::observes< Inc , E > || mod::observes< Scratch , E >;

 template< class E >
 static constexpr bool nothrow_for =
  ( ! mod::observes< Inc , E > ||
    noexcept( std::declval< Inc & >().on( std::declval< const E & >() ) ) ) &&
  ( ! mod::observes< Scratch , E > ||
    noexcept( std::declval< Scratch & >().on(
                                       std::declval< const E & >() ) ) );

 public:

 /// what a shadow counts
 struct Counts {
  std::uint64_t events = 0;       ///< the events forwarded
  std::uint64_t queries = 0;      ///< the queries compared
  std::uint64_t mismatches = 0;   ///< the queries whose answers differ
  };

 /// the shadow of \f$ i \f$ by \f$ s \f$, compared by \f$ \mathit{eq} \f$
 Shadow( Inc & i , Scratch & s , Eq eq = Eq() )
  : f_inc( i ) , f_scr( s ) , f_eq( std::move( eq ) ) {}

 /// forwards \f$ e \f$ to both, to those that want it
 template< class E >
  requires( forwarded< E > )
 void on( const E & e ) noexcept( nothrow_for< E > ) {
  ++f_counts.events;
  if constexpr( mod::observes< Inc , E > )
   f_inc.on( e );
  if constexpr( mod::observes< Scratch , E > )
   f_scr.on( e );
  }

 /// the group hooks of both, where they have them
 void group_begin( const mod::Group & g ) {
  begin( f_inc , g );
  begin( f_scr , g );
  }

 /// the end of a group for both
 void group_end( void ) {
  if constexpr( mod::groups< Inc > )
   f_inc.group_end();
  if constexpr( mod::groups< Scratch > )
   f_scr.group_end();
  }

 /// `flush()` of both, where they have it
 void flush( void ) {
  if constexpr( mod::lazy< Inc > )
   f_inc.flush();
  if constexpr( mod::lazy< Scratch > )
   f_scr.flush();
  }

 /// the answer of the incremental observer to \f$ f \f$, compared with
 /// that of the other one (a violation at the level of the audits if they
 /// differ)
 template< class F >
 auto query( F && f ) {
  auto a = f( f_inc );
  const bool same = agreeOn( a , f( f_scr ) );
  ADS_AUDIT( same );
  return( a );
  }

 /// whether the two observers give the same answer to \f$ f \f$
 template< class F >
 [[nodiscard]] bool agree( F && f ) {
  return( agreeOn( f( f_inc ) , f( f_scr ) ) );
  }

 /// the audits of both, where they have them
 template< class S >
 [[nodiscard]] bool audit( const S & s ) const {
  bool ok = true;
  if constexpr( mod::auditable< Inc , S > )
   ok = ok && bool( f_inc.audit( s ) );
  if constexpr( mod::auditable< Scratch , S > )
   ok = ok && bool( f_scr.audit( s ) );
  return( ok );
  }

 /// the counts
 [[nodiscard]] const Counts & counts( void ) const { return( f_counts ); }

 /// the incremental observer
 [[nodiscard]] Inc & incremental( void ) { return( f_inc ); }
 /// the observer from scratch
 [[nodiscard]] Scratch & scratch( void ) { return( f_scr ); }

 private:

 template< class O >
 static void begin( O & o , const mod::Group & g ) {
  if constexpr( mod::groups< O > )
   mod::Direct::begin( o , g );
  }

 template< class A , class B >
 bool agreeOn( const A & a , const B & b ) {
  ++f_counts.queries;
  const bool same = f_eq( a , b );
  if( ! same )
   ++f_counts.mismatches;
  return( same );
  }

 Inc & f_inc;                   ///< the incremental observer
 Scratch & f_scr;               ///< the observer from scratch
 Eq f_eq;                       ///< the equality of the answers
 Counts f_counts;               ///< the counts

 };  // end( class Shadow )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/impl/Modification.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Modification.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Modification.h -------------------------*/
/*--------------------------------------------------------------------------*/
