/*--------------------------------------------------------------------------*/
/*----------------------- File QueueCustomization.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The access to the priority queues of the two concepts of section 10.1,
 * whose definitions are in `PriorityQueue.h` [see `PriorityQueue.h`, which
 * includes this header], beyond the book: the items of a queue,
 * `queue_item_t< Q >`, and the operations by which a queue of ADS, of the
 * standard library or of another library is read alike.
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
 * themselves. The handles are read in the same way: `queue_handle_t< Q >`
 * is `Q::Handle`, else the `handle_type` of the standard spelling, and
 * `queue_decrease( q , h , p )` calls `q.decrease( h , p )` of a queue with
 * `Handle`, else the free function found by ADL, so that a mutable queue of
 * another library lowers its keys natively once such a function is given
 * for it.
 *
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

#ifndef __ADS_QueueCustomization
 #define __ADS_QueueCustomization
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <type_traits>
#include <utility>

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
void queue_decrease( void ) = delete;

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
 requires std::constructible_from< typename queue_item< Q >::type ,
				   decltype( q.top() ) >;
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

template< class Q >
struct queue_handle
{ };

template< class Q >
 requires(
   requires { typename Q::handle_type; } &&
   ! requires { typename Q::Handle; } )
struct queue_handle< Q >
{
 using type = typename Q::handle_type;
 };

template< class Q >
 requires requires { typename Q::Handle; }
struct queue_handle< Q >
{
 using type = typename Q::Handle;
 };

template< class Q , class H , class P >
concept member_decrease =
  requires { typename Q::Handle; } && requires( Q & q , H && h , P && p )
{
   q.decrease( std::forward< H >( h ) , std::forward< P >( p ) );
   };
template< class Q , class H , class P >
concept free_decrease = requires( Q & q , H && h , P && p )
{
 queue_decrease( q , std::forward< H >( h ) , std::forward< P >( p ) );
 };

struct QueueDecrease
{
 template< class Q , class H , class P >
  requires( member_decrease< Q , H , P > || free_decrease< Q , H , P > )
 constexpr void operator()( Q & q , H && h , P && p ) const {
  if constexpr( member_decrease< Q , H , P > )
   q.decrease( std::forward< H >( h ) , std::forward< P >( p ) );
  else
   queue_decrease( q , std::forward< H >( h ) , std::forward< P >( p ) );
  }
 };

}  // end( namespace detail )
/// \endcond

/// the handles of the queue `Q`, which its insertions return: `Q::Handle`,
/// else `Q::handle_type` (the mutable heaps of Boost.Heap)
template< class Q >
using queue_handle_t = typename detail::queue_handle< Q >::type;

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

/// the priority of the item of the handle \f$ h \f$ of \f$ q \f$ lowered
/// to \f$ p \f$, i.e., the item moved toward the minimum: `q.decrease( h ,
/// p )` if `Q` declares a `Handle`, the handles of the book, else
/// `queue_decrease( q , h , p )` found by ADL. There is no step on the
/// spelling of another library, since the names disagree on the direction:
/// Boost.Heap calls `increase()` the move toward the top of its heap, and
/// `ADS/interop/boost/Heap.h` gives the free function that says so
/// \pre \f$ h \f$ is the handle of an item in \f$ q \f$, whose priority is
///      not below \f$ p \f$
inline constexpr detail::QueueDecrease queue_decrease{};

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* QueueCustomization.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File QueueCustomization.h ----------------------*/
/*--------------------------------------------------------------------------*/
