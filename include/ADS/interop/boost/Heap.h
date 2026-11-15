/*--------------------------------------------------------------------------*/
/*------------------------------ File Heap.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The mutable heaps of Boost.Heap as queues of ADS that lower their keys
 * natively, beyond the book, compiled only with `ADS_WITH_BOOST`. The heaps
 * of Boost.Heap already model `PriorityQueue` of `PriorityQueue.h`, through
 * the customization points that read `push()`, `top()` and `pop()`, and
 * their handles are `queue_handle_t`, their `handle_type`; this header
 * gives the free `queue_decrease( q , h , p )` that those points find by
 * argument-dependent lookup on a mutable heap, whose `push()` gives a
 * handle (not on a `d_ary_heap` without `mutable_< true >`, which declares
 * the members of the handles but does not compile them),
 * so that `decrease_key_of()` answers `native` and the shortest paths and
 * Prim's algorithm of ADS (`PriorityFrontier`) lower a key on the handle of
 * the heap instead of inserting a copy.
 *
 * A heap of Boost.Heap has on top the largest item in the order of its
 * comparator, so that with `std::greater` (the least distance on top, as
 * the shortest paths want) the fall of a key moves the item toward the
 * top, which Boost.Heap calls `increase()` (constant amortized time in the
 * Fibonacci heap), while its `decrease()` moves an item away from the top.
 * The function of this header is therefore `increase()`, whatever the
 * comparator: for ADS, to decrease is to move toward the minimum, i.e.,
 * the top. The items of such a heap are the pairs ( distance , node ),
 * the priority and the node at once [see `PriorityFrontier`].
 *
 * \see T. Blechmann, Boost.Heap, https://www.boost.org/libs/heap
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

#ifndef __ADS_interop_boost_Heap
 #define __ADS_interop_boost_Heap
/* self-identification: #endif constant */

#if defined( ADS_WITH_BOOST )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/PriorityQueue.h"

#include <boost/heap/policies.hpp>

#include <concepts>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace boost::heap
{

/// the priority of the item of the handle \f$ h \f$ of the mutable heap
/// \f$ q \f$ of Boost.Heap lowered to \f$ p \f$ in the sense of ADS, i.e.,
/// the item moved toward the top: `q.increase( h , p )`, found by
/// `ADS::queue_decrease()`
/// \pre \f$ h \f$ is the handle of an item of \f$ q \f$, \f$ p \f$ is not
///      farther from the top than its value
template< class H >
 requires requires( H & q , typename H::handle_type h ,
		    const typename H::value_type & p )
{
  { q.push( p ) } -> std::same_as< typename H::handle_type >;
  q.increase( h , p );
  }
void queue_decrease( H & q , typename H::handle_type h ,
		     const typename H::value_type & p )
{
 q.increase( h , p );
 }

}  // end( namespace boost::heap )

#endif /* ADS_WITH_BOOST */

/*--------------------------------------------------------------------------*/

#endif /* Heap.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File Heap.h ----------------------------*/
/*--------------------------------------------------------------------------*/
