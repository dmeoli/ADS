/*--------------------------------------------------------------------------*/
/*-------------------------- File PriorityQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE of section 10.1, with decreasing priority, in two
 * concepts. `PriorityQueue` is the queue without handles: `insert()` of an
 * item with the keys of its order, `min()` and `deleteMin()` the item of
 * smallest priority, `empty()`; `AddressablePriorityQueue` adds the
 * handles of the book, `insert( x , p )` giving a handle of \f$ x \f$ and
 * `decrease( y , p )` lowering to \f$ p \f$ the priority of the item of the
 * handle \f$ y \f$. Items of equal priority leave in no given order.
 *
 * The base concept is read through three customization point objects,
 * `queue_insert()`, `queue_min()` and `queue_deleteMin()`, in the form of
 * Niebler: each calls the member of the book (`insert()`, `min()`,
 * `deleteMin()`) if the queue has it, else a free function found by
 * argument-dependent lookup (`queue_min( q )` and the others), which is how
 * a type that ADS does not own is adapted, else the spelling of the
 * standard library (`push()`, `top()`, and `pop()`, which returns the item
 * or, as in `std::priority_queue`, nothing after `top()`). The keys of an
 * insertion are those of the order of the queue: a priority (`Priority`),
 * one or two keys (`Key`), or none, when the items are ordered by
 * themselves.
 *
 * `HeapPriorityQueue` realizes both concepts on a \f$ d \f$-ary heap,
 * `ListPriorityQueue` on a sorted or unsorted list, `TreePriorityQueue` on a
 * Red-Black tree; `FibonacciHeapPriorityQueue`, `PairingHeap` and
 * `IndexedDaryHeap` are realizations beyond the book.
 * `LinkedHeapPriorityQueue`, a heap on a tree with pointers, has no handles
 * and no `decrease()`, and is therefore a model of the base concept only,
 * as are `DaryHeap`, `BucketQueue`, `DialQueue`, `RadixHeap` and
 * `std::priority_queue`. How an algorithm lowers a priority on a queue of
 * either kind is `decrease_key_of()` of `Capabilities.h`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see E. Niebler, Suggested design for customization points, ISO/IEC JTC1
 *      SC22 WG21 N4381, 2015,
 *      https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html
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

#ifndef __ADS_PriorityQueue
 #define __ADS_PriorityQueue
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- THE ITEMS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail
{

template< class Q >
struct queue_item
{ };

template< class Q >
 requires(
   requires { typename Q::value_type; } && ! requires { typename Q::Item; } )
struct queue_item< Q >
{
 using type = typename Q::value_type;
 };

template< class Q >
 requires requires { typename Q::Item; }
struct queue_item< Q >
{
 using type = typename Q::Item;
 };

}  // end( namespace detail )
/// \endcond

/// the items of the queue `Q`: `Q::Item`, else `Q::value_type`
template< class Q >
using queue_item_t = typename detail::queue_item< Q >::type;

/*--------------------------------------------------------------------------*/
/*---------------------- THE CUSTOMIZATION POINTS --------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail
{

void queue_min( void ) = delete;  // the free ones, only by ADL
void queue_deleteMin( void ) = delete;
void queue_insert( void ) = delete;

template< class Q >
concept member_min = requires( Q & q )
{ q.min(); };
template< class Q >
concept free_min = requires( Q & q )
{ queue_min( q ); };
template< class Q >
concept top_min = requires( Q & q )
{ q.top(); };

struct QueueMin
{
 template< class Q >
  requires( member_min< Q > || free_min< Q > || top_min< Q > )
 constexpr decltype( auto ) operator()( Q & q ) const {
  if constexpr( member_min< Q > )
   return( q.min() );
  else if constexpr( free_min< Q > )
   return( queue_min( q ) );
  else
   return( q.top() );
  }
 };

template< class Q >
concept member_deleteMin = requires( Q & q )
{ q.deleteMin(); };
template< class Q >
concept free_deleteMin = requires( Q & q )
{ queue_deleteMin( q ); };
template< class Q >
concept item_pop = requires( Q & q )
{
 { q.pop() } -> std::convertible_to< typename queue_item< Q >::type >;
 };
template< class Q >
concept top_pop = requires( Q & q )
{
 typename queue_item< Q >::type( q.top() );
 q.pop();
 };

struct QueueDeleteMin
{
 template< class Q >
  requires( member_deleteMin< Q > || free_deleteMin< Q > || item_pop< Q > ||
	    top_pop< Q > )
 constexpr auto operator()( Q & q ) const {
  if constexpr( member_deleteMin< Q > )
   return( q.deleteMin() );
  else if constexpr( free_deleteMin< Q > )
   return( queue_deleteMin( q ) );
  else if constexpr( item_pop< Q > )
   return( typename queue_item< Q >::type( q.pop() ) );
  else {
   typename queue_item< Q >::type x = q.top();  // the pop() of the
   q.pop();                                     // standard: no item
   return( x );
   }
  }
 };

template< class Q , class... A >
concept member_insert =
  requires( Q & q , A &&... a )
{ q.insert( std::forward< A >( a )... ); };
template< class Q , class... A >
concept free_insert = requires( Q & q , A &&... a )
{
 queue_insert( q , std::forward< A >( a )... );
 };
template< class Q , class... A >
concept push_insert =
  requires( Q & q , A &&... a )
{ q.push( std::forward< A >( a )... ); };

struct QueueInsert
{
 template< class Q , class... A >
  requires( member_insert< Q , A... > || free_insert< Q , A... > ||
	    push_insert< Q , A... > )
 constexpr decltype( auto ) operator()( Q & q , A &&... a ) const {
  if constexpr( member_insert< Q , A... > )
   return( q.insert( std::forward< A >( a )... ) );
  else if constexpr( free_insert< Q , A... > )
   return( queue_insert( q , std::forward< A >( a )... ) );
  else
   return( q.push( std::forward< A >( a )... ) );
  }
 };

}  // end( namespace detail )
/// \endcond

/// the item of smallest priority of \f$ q \f$: `q.min()`, else
/// `queue_min( q )` found by ADL, else `q.top()`
/// \pre \f$ q \f$ is not empty
inline constexpr detail::QueueMin queue_min{};

/// the item of smallest priority of \f$ q \f$, removed: `q.deleteMin()`,
/// else `queue_deleteMin( q )` found by ADL, else `q.pop()` if it returns
/// the item, else `q.top()` and then `q.pop()`
/// \pre \f$ q \f$ is not empty
inline constexpr detail::QueueDeleteMin queue_deleteMin{};

/// \f$ x \f$ inserted in \f$ q \f$ with the keys \f$ a \f$ of its order:
/// `q.insert( x , a... )`, else `queue_insert( q , x , a... )` found by
/// ADL, else `q.push( x , a... )`
inline constexpr detail::QueueInsert queue_insert{};

/*--------------------------------------------------------------------------*/
/*------------------------------- CONCEPTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail
{

template< class Q , class... A >
concept queue_insertable =
  requires( Q & q , const queue_item_t< Q > & x , const A &... a )
{
   ADS::queue_insert( q , x , a... );
   };

template< class Q >
concept queue_keys =
  ( requires { typename Q::Priority; } &&
    queue_insertable< Q , typename Q::Priority > ) ||
  ( requires
{ typename Q::Key; } &&
    ( queue_insertable< Q , typename Q::Key > ||
      queue_insertable< Q , typename Q::Key , typename Q::Key > )) ||
  queue_insertable< Q >;

}  // end( namespace detail )
/// \endcond

/// the PRIORITYQUEUE of section 10.1 without handles: the type of the items
/// (`Item` or `value_type`), `empty()`, and `queue_insert()` of an item
/// with the keys of the order of the queue (a `Priority`, one or two
/// `Key`s, or none), `queue_min()` and `queue_deleteMin()`, the item of
/// smallest priority read and removed
template< class Q >
concept PriorityQueue = requires( Q & q , const Q & cq )
{
 typename queue_item_t< Q >;
 { cq.empty() } -> std::convertible_to< bool >;
 { ADS::queue_min( q ) } -> std::convertible_to< queue_item_t< Q > >;
 { ADS::queue_deleteMin( q ) } -> std::convertible_to< queue_item_t< Q > >;
 } && detail::queue_keys< Q >;

/// the PRIORITYQUEUE of section 10.1 with handles and decreasing priority:
/// the types `Item`, `Priority` and `Handle`, and `empty()`, `min()`,
/// `deleteMin()`, `insert( x , p )`, which gives the handle of \f$ x \f$,
/// and `decrease( y , p )`
template< class Q >
concept AddressablePriorityQueue =
  PriorityQueue< Q > &&
  requires( Q q , const Q cq , const typename Q::Item & x ,
	    const typename Q::Priority & p , typename Q::Handle y )
{
   { cq.empty() } -> std::same_as< bool >;
   { cq.min() } -> std::convertible_to< typename Q::Item >;
   { q.deleteMin() } -> std::convertible_to< typename Q::Item >;
   { q.insert( x , p ) } -> std::same_as< typename Q::Handle >;
   q.decrease( y , p );
   };

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* PriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
