/*--------------------------------------------------------------------------*/
/*-------------------------- File LockFreeStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A stack shared by threads, lock-free and unbounded, beyond the book: the
 * stack of Treiber, a list of nodes whose top any thread swings by a
 * compare-and-swap, with the memory of the nodes popped reclaimed by a policy
 * [see `Reclaim.h`], the epochs of Fraser with an attempt to raise the epoch
 * per 64 retirements by default [see `EpochReclaim.h`], or the hazard pointers
 * [see `HazardPointer.h`]. A push links a new node above the top it read; a
 * pop protects the top \f$ t \f$, reads its successor and swings the top from
 * \f$ t \f$ to it, then moves the item out of \f$ t \f$ and retires it. Since
 * a node is pushed once, popped once and freed only when no thread protects
 * it, a pop that finds the top still \f$ t \f$ finds it with the successor
 * read, and the ABA problem of the stack, which the inbox of many producers
 * [see `MPSCInbox.h`] avoids by taking the whole stack at once, cannot occur
 * (see \ref theory_concurrent_reclaim).
 *
 * Both operations are linearizable at their successful compare-and-swap,
 * or, for a pop that finds the stack empty, at its read of the top, and
 * lock-free but for the allocation of the nodes and for the reclamation,
 * which is blocking under the epochs and lock-free under the hazard
 * pointers. Each thread uses the stack through a `Handle`, which holds its
 * record of the reclamation.
 *
 * The items are movable without exceptions. The contracts [see
 * `Contract.h`]: no handle is alive when the stack is destroyed; `size()`
 * and `empty()` of the stack are called with no operation running, while
 * the operations of the handles may run at once on any threads, each
 * handle used by one thread at a time. The operations are defined in
 * `LockFreeStack.tpp`.
 *
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986
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

#ifndef __ADS_LockFreeStack
 #define __ADS_LockFreeStack
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/EpochReclaim.h"
#include "ADS/concurrent/Reclaim.h"

#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LockFreeStack ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a STACK shared by threads, the one of Treiber, its nodes reclaimed by
/// the policy \f$ R \f$
template< class T , reclamation_policy R = FraserEpochs< 64 > >
class LockFreeStack
{

 static_assert( std::is_nothrow_move_constructible_v< T > ,
		"the items are moved without exceptions" );

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;                     ///< the type of the items
 using Policy = R;                   ///< the reclamation
 using Domain = typename R::Domain;  ///< the domain of the reclamation

 class Handle;

 /// the empty stack
 LockFreeStack( void ) = default;
 /// no copy, since the threads share it by its address
 LockFreeStack( const LockFreeStack & ) = delete;
 /// no copy, since the threads share it by its address
 LockFreeStack & operator=( const LockFreeStack & ) = delete;

 /// the frees of the nodes in the stack, then, by the domain, of those
 /// retired
 /// \pre no handle is alive
 ~LockFreeStack();

 /// a handle for one thread, on a record of the reclamation
 [[nodiscard]] Handle handle( void ) { return( Handle( *this ) ); }

 /// the number of the items; with no operation running
 [[nodiscard]] std::size_t size( void ) const;

 /// whether `size()` is 0; with no operation running
 [[nodiscard]] bool empty( void ) const {
  return( ! f_top.load( std::memory_order_acquire ) );
  }

 /// the domain of the reclamation, for its counters
 [[nodiscard]] const Domain & domain( void ) const { return( f_dom ); }

/*--------------------------------------------------------------------------*/
 private:
 /// a node: its link of the reclamation, its item and the node below
 struct Node : Retirable
 {
  T item;       ///< the item, moved out by its pop
  Node * next;  ///< the node below, fixed before the push
  };

 /// the address or the free of a node retired
 static void * reclaimNode( Retirable * r , Retirable::Op op ) noexcept {
  Node * n = static_cast< Node * >( r );
  if( op == Retirable::Op::address )
   return( n );
  delete n;
  return( nullptr );
  }

 /// the records of the threads, and the nodes retired
 Domain f_dom;
 /// the top, nil when empty
 alignas( 64 ) std::atomic< Node * > f_top{ nullptr };

 };  // end( class LockFreeStack )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS LockFreeStack::Handle ----------------------*/
/*--------------------------------------------------------------------------*/

/// the access of one thread to a `LockFreeStack`: its record of the
/// reclamation, with one slot
template< class T , reclamation_policy R >
class LockFreeStack< T , R >::Handle
{

/*--------------------------------------------------------------------------*/
 public:
 /// the record moved
 Handle( Handle && ) noexcept = default;
 /// no copy, since a record has one owner
 Handle( const Handle & ) = delete;
 /// no assignment, since a record has one owner
 Handle & operator=( const Handle & ) = delete;

 /// pushes x, linearizable at the compare-and-swap that links its node
 ///
 /// In pseudocode (Treiber, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad n \gets \text{a node of } x \cr
 /// 3 & \quad \textbf{repeat}\ n.\mathit{next} \gets \mathit{top}
 ///     \ \textbf{until}\ \mathrm{CAS}( \mathit{top} , n.\mathit{next} , n
 ///     )
 /// \end{array}}
 /// \f]
 void push( T x );

 /// the item on the top, popped; nothing if the stack was empty at the
 /// linearization point
 ///
 /// In pseudocode (Treiber, with the reclamation of Michael, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPop}() \cr
 /// 2 & \quad \textbf{loop} \cr
 /// 3 & \qquad t \gets \mathrm{protect}( \mathit{top} )\quad
 ///     \triangleright\ t \text{ is not freed until the guard ends} \cr
 /// 4 & \qquad \textbf{if}\ t = \textbf{nil}\ \textbf{then return nil} \cr
 /// 5 & \qquad \textbf{if}\ \mathrm{CAS}( \mathit{top} , t ,
 ///     t.\mathit{next} )\ \textbf{then} \cr
 /// 6 & \qquad\quad x \gets t.x\text{, moved out};\ \mathrm{retire}( t );
 ///     \ \textbf{return}\ x
 /// \end{array}}
 /// \f]
 std::optional< T > tryPop( void );

/*--------------------------------------------------------------------------*/
 private:
 friend class LockFreeStack;

 /// the handle of s, on a record of its domain
 explicit Handle( LockFreeStack & s ) : f_s( &s ) , f_l( s.f_dom ) { }

 using Local = typename R::template Local< 1 >;  ///< the record

 LockFreeStack * f_s;  ///< the stack
 Local f_l;            ///< the record of the reclamation

 };  // end( class LockFreeStack::Handle )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/LockFreeStack.tpp"

/*--------------------------------------------------------------------------*/

#endif /* LockFreeStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LockFreeStack.h ------------------------*/
/*--------------------------------------------------------------------------*/
