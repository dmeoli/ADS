/*--------------------------------------------------------------------------*/
/*-------------------------- File LockFreeQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A queue of many producers and many consumers, lock-free and unbounded,
 * beyond the book: the queue of Michael and Scott, a list of nodes from a
 * head to a tail with a dummy node first, with the memory of the nodes
 * dequeued reclaimed by a policy [see `Reclaim.h`], the epochs of Fraser
 * with an attempt to raise the epoch per 64 retirements by default [see
 * `EpochReclaim.h`], or the hazard pointers [see `HazardPointer.h`], two
 * slots per thread. An enqueue links a new
 * node after the last one by a compare-and-swap on its pointer, from nil,
 * and then swings the tail to it; a thread that finds the tail behind the
 * last node swings it first, so that no operation waits for a stopped one.
 * A dequeue protects the head \f$ h \f$ and its successor \f$ n \f$,
 * checks that the head is still \f$ h \f$, swings the head from \f$ h \f$
 * to \f$ n \f$, which becomes the dummy, moves the item out of \f$ n \f$ and
 * retires \f$ h \f$; the head never passes the tail, since a dequeue that
 * finds them equal with a successor swings the tail first, so that the node
 * reached by the tail is never retired (see \ref theory_concurrent_reclaim).
 * Unlike `MPMCQueue` [see `MPMCQueue.h`], the queue is unbounded and a
 * stopped thread never holds up the others, at the price of an allocation
 * per item and of the reclamation.
 *
 * An enqueue is linearizable at the compare-and-swap that links its node,
 * a dequeue at the one that swings the head, or, when it finds the queue
 * empty, at its read of a nil successor of the head; both are lock-free
 * but for the allocation of the nodes and for the reclamation, which is
 * blocking under the epochs and lock-free under the hazard pointers. Each
 * thread uses the queue through a `Handle`, which holds its record of the
 * reclamation; the items of one producer come out in the order in which
 * it enqueued them.
 *
 * The items are movable without exceptions. The contracts [see
 * `Contract.h`]: no handle is alive when the queue is destroyed; `size()`
 * and `empty()` of the queue are called with no operation running, while
 * the operations of the handles may run at once on any threads, each
 * handle used by one thread at a time. The operations are defined in
 * `LockFreeQueue.tpp`.
 *
 * \see M. M. Michael, M. L. Scott, Simple, fast, and practical
 *      non-blocking and blocking concurrent queue algorithms, ACM Symposium
 *      on Principles of Distributed Computing (PODC), 267-275, 1996,
 *      doi:10.1145/248052.248106
 * \see M. M. Michael, Hazard pointers: safe memory reclamation for
 *      lock-free objects, IEEE Transactions on Parallel and Distributed
 *      Systems 15(6), 491-504, 2004, doi:10.1109/TPDS.2004.8
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

#ifndef __ADS_LockFreeQueue
 #define __ADS_LockFreeQueue
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/EpochReclaim.h"
#include "ADS/concurrent/Reclaim.h"

#include <atomic>
#include <cstddef>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LockFreeQueue ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE of many producers and many consumers, the one of Michael and
/// Scott, its nodes reclaimed by the policy \f$ R \f$
template< class T , reclamation_policy R = FraserEpochs< 64 > >
class LockFreeQueue
{

 static_assert( std::is_nothrow_move_constructible_v< T > ,
		"the items are moved without exceptions" );

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;                     ///< the type of the items
 using Policy = R;                   ///< the reclamation
 using Domain = typename R::Domain;  ///< the domain of the reclamation

 class Handle;

 /// the empty queue, its dummy node made
 LockFreeQueue( void );
 /// no copy, since the threads share it by its address
 LockFreeQueue( const LockFreeQueue & ) = delete;
 /// no copy, since the threads share it by its address
 LockFreeQueue & operator=( const LockFreeQueue & ) = delete;

 /// the frees of the nodes in the queue and of their items, then, by the
 /// domain, of those retired
 /// \pre no handle is alive
 ~LockFreeQueue();

 /// a handle for one thread, on a record of the reclamation
 [[nodiscard]] Handle handle( void ) { return( Handle( *this ) ); }

 /// the number of the items; with no operation running
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0; with no operation running
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the domain of the reclamation, for its counters
 [[nodiscard]] const Domain & domain( void ) const { return( f_dom ); }

/*--------------------------------------------------------------------------*/
 private:
 /// a node: its link of the reclamation, its successor, and the room of
 /// its item, empty in the dummy
 struct Node : Retirable
 {
  /// the successor, nil for the last node
  std::atomic< Node * > next{ nullptr };
  /// the room of the item, built by its enqueue and emptied by its dequeue
  alignas( T ) unsigned char raw[ sizeof( T ) ];
  /// the item
  [[nodiscard]] T * item( void ) {
   return( std::launder( reinterpret_cast< T * >( raw ) ) );
   }
  };

 /// a node with no item
 static Node * makeNode( void ) {
  Node * n = new Node;
  n->reclaim = &LockFreeQueue::reclaimNode;
  return( n );
  }

 /// the address or the free of a node retired, whose item is gone
 static void * reclaimNode( Retirable * r , Retirable::Op op ) noexcept {
  Node * n = static_cast< Node * >( r );
  if( op == Retirable::Op::address )
   return( n );
  delete n;
  return( nullptr );
  }

 /// the records of the threads, and the nodes retired
 Domain f_dom;
 /// the dummy node, whose successor holds the first item
 alignas( 64 ) std::atomic< Node * > f_head;
 /// the last node, or one behind it
 alignas( 64 ) std::atomic< Node * > f_tail;

 };  // end( class LockFreeQueue )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS LockFreeQueue::Handle ----------------------*/
/*--------------------------------------------------------------------------*/

/// the access of one thread to a `LockFreeQueue`: its record of the
/// reclamation, with two slots
template< class T , reclamation_policy R >
class LockFreeQueue< T , R >::Handle
{

/*--------------------------------------------------------------------------*/
 public:
 /// the record moved
 Handle( Handle && ) noexcept = default;
 /// no copy, since a record has one owner
 Handle( const Handle & ) = delete;
 /// no assignment, since a record has one owner
 Handle & operator=( const Handle & ) = delete;

 /// enqueues x, linearizable at the compare-and-swap that links its node
 ///
 /// In pseudocode (Michael and Scott, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enqueue}( x ) \cr
 /// 2 & \quad n \gets \text{a node of } x \cr
 /// 3 & \quad \textbf{loop}\ t \gets \mathrm{protect}( \mathit{tail} );
 ///     \ s \gets t.\mathit{next} \cr
 /// 4 & \qquad \textbf{if}\ s \ne \textbf{nil}\ \textbf{then}
 ///     \ \mathrm{CAS}( \mathit{tail} , t , s )\quad \triangleright
 ///     \ \text{the tail behind: help} \cr
 /// 5 & \qquad \textbf{else if}\ \mathrm{CAS}( t.\mathit{next} ,
 ///     \textbf{nil} , n )\ \textbf{then}\ \mathrm{CAS}( \mathit{tail} , t ,
 ///     n );\ \textbf{return}
 /// \end{array}}
 /// \f]
 void enqueue( T x );

 /// the first item, dequeued; nothing if the queue was empty at the
 /// linearization point
 ///
 /// In pseudocode (Michael and Scott, with the reclamation of Michael,
 /// beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryDequeue}() \cr
 /// 2 & \quad \textbf{loop}\ h \gets \mathrm{protect}_0( \mathit{head} );
 ///     \ n \gets \mathrm{protect}_1( h.\mathit{next} ) \cr
 /// 3 & \qquad \textbf{if}\ \mathit{head} \ne h\ \textbf{then continue}
 ///     \quad \triangleright\ n \text{ may have been retired} \cr
 /// 4 & \qquad \textbf{if}\ n = \textbf{nil}\ \textbf{then return nil}
 ///     \quad \triangleright\ \text{empty} \cr
 /// 5 & \qquad \textbf{if}\ \mathit{tail} = h\ \textbf{then}
 ///     \ \mathrm{CAS}( \mathit{tail} , h , n )\quad \triangleright
 ///     \ \text{the tail behind: help} \cr
 /// 6 & \qquad \textbf{else if}\ \mathrm{CAS}( \mathit{head} , h , n )
 ///     \ \textbf{then} \cr
 /// 7 & \qquad\quad x \gets n.x\text{, moved out};\ \mathrm{retire}( h );
 ///     \ \textbf{return}\ x
 /// \end{array}}
 /// \f]
 std::optional< T > tryDequeue( void );

/*--------------------------------------------------------------------------*/
 private:
 friend class LockFreeQueue;

 /// the handle of q, on a record of its domain
 explicit Handle( LockFreeQueue & q ) : f_q( &q ) , f_l( q.f_dom ) { }

 using Local = typename R::template Local< 2 >;  ///< the record

 LockFreeQueue * f_q;  ///< the queue
 Local f_l;            ///< the record of the reclamation

 };  // end( class LockFreeQueue::Handle )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/LockFreeQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif /* LockFreeQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LockFreeQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
