/*--------------------------------------------------------------------------*/
/*----------------------------- File Reclaim.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * What the reclamations of the memory of the lock-free structures have in
 * common, beyond the book: the link by which a node handed to a
 * reclamation (<em>retired</em>) waits to be freed, and the interface of a
 * policy of reclamation, which a structure of nodes takes as a parameter
 * of its template. A node is retired once no thread can reach it any
 * longer from the shared structure, i.e., after it has been unlinked; it
 * may still be read by the threads that reached it before, and the policy
 * frees it only when none of them may still hold it. The policies are
 * the hazard pointers of Michael [see `HazardPointer.h`], under which
 * a thread protects each node that it is about to read, and the epochs of
 * Fraser [see `EpochReclaim.h`], under which a thread
 * protects, for the whole of an operation, everything it may reach (see
 * \ref theory_concurrent_reclaim).
 *
 * A policy \f$ R \f$ gives the types
 * - `R::Domain`, the state shared by the threads of one structure (the
 *   records of the threads, and, for the hazard pointers, their slots),
 *   which frees in its destructor the nodes still retired;
 * - `R::Local< K >`, the record of one thread on a domain, made from the
 *   domain and movable, with \f$ K \f$ slots of protection when the policy
 *   protects node by node; its `retire( p )` hands the node \f$ p \f$ to the
 *   reclamation, inside a guard of the same record;
 * - `R::Local< K >::Guard`, the span of an operation on a record, made
 *   from it and ended by its destructor; its `protect( i , src )` reads the
 *   pointer of the atomic `src` and keeps the node it reaches from being
 *   freed until the slot \f$ i < K \f$ protects another node or the guard
 *   ends, and `reset( i )` drops the protection of the slot \f$ i \f$;
 * - `R::kRegions`, true when a guard protects every node that its
 *   operation reaches, whether or not it was read by `protect()` (the
 *   epochs), false when only the nodes read by `protect()` are protected
 *   (the hazard pointers), in which case a structure whose operations walk
 *   through nodes that may have been retired cannot take the policy.
 *
 * A node of a structure derives from `Retirable`, whose function
 * `reclaim` the structure sets: it frees the node, or gives the address
 * that `protect()` stores for it, i.e., the address of the object of the
 * type that the atomics of the structure point to. The contracts [see
 * `Contract.h`]: a node is retired once, after it has been unlinked, and
 * never linked again; the guard of a record is used by one thread at a
 * time, and the guards of one record do not overlap. There is no
 * operation to define.
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

#ifndef __ADS_Reclaim
 #define __ADS_Reclaim
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Retirable -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the link of a node retired: the next node of its list of the retired,
/// and the function that frees it or gives its address
struct Retirable
{
 /// what `reclaim` does: the address of the object, or its free
 enum class Op { address , free };

 /// the next node of the list of the retired that holds it
 Retirable * retired_next = nullptr;
 /// the address of the object that `protect()` stores for the node, or,
 /// with `Op::free`, the free of the node and nil
 void * ( *reclaim )( Retirable * , Op ) noexcept = nullptr;
 };

namespace detail
{

/// the nodes of the list of the retired from p freed, and their number
inline std::size_t freeRetired( Retirable * p ) noexcept
{
 std::size_t n = 0;
 while( p ) {
  Retirable * q = p->retired_next;
  p->reclaim( p , Retirable::Op::free );
  p = q;
  ++n;
  }
 return( n );
 }

/// the counter x, written only by its owner and read by any thread,
/// raised by n with no read-modify-write
inline void ownerAdd( std::atomic< std::uint64_t > & x ,
		      std::uint64_t n ) noexcept
{
 x.store( x.load( std::memory_order_relaxed ) + n ,
	  std::memory_order_relaxed );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------ CONCEPT reclamation_policy ----------------------*/
/*--------------------------------------------------------------------------*/

/// a policy of reclamation: its domain, its records with their guards, and
/// whether a guard protects all that its operation reaches
template< class R >
concept reclamation_policy = requires {
 typename R::Domain;
 typename R::template Local< 1 >;
 typename R::template Local< 1 >::Guard;
 { R::kRegions } -> std::convertible_to< bool >;
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Reclaim.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Reclaim.h ---------------------------*/
/*--------------------------------------------------------------------------*/
