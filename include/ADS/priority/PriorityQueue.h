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
 * The base concept is read through the customization point objects
 * `queue_insert()`, `queue_min()` and `queue_deleteMin()`, and the handles
 * through `queue_handle_t` and `queue_decrease()`, which are in
 * `QueueCustomization.h` [see `QueueCustomization.h`, which this header
 * includes], so that a type that ADS does not own is a model of the
 * concepts once it is adapted there.
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

#include "ADS/priority/QueueCustomization.h"

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
