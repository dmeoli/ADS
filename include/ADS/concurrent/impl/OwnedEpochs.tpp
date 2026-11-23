/*--------------------------------------------------------------------------*/
/*-------------------------- File OwnedEpochs.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The reclamation by epochs of Fraser that a structure of nodes owns, kept
 * to the few calls that `ConcurrentBTreeMap` needs [see
 * `ConcurrentBTreeMap.h`]: a domain with one epoch, a record per thread
 * that announces the epoch in which its thread runs an operation, and the
 * nodes retired by a thread, waiting in its record until the epoch has
 * grown by 2 since their retirement, which happens only after every
 * thread that was running an operation has announced a newer epoch. It is
 * the scheme of `LockFreePriorityQueue` [see \ref theory_concurrent_lfpq],
 * with the attempt to raise the epoch made once per \f$ R = 64 \f$
 * retirements of a record, as in the epochs of Fraser.
 *
 * The calls are those of a policy of reclamation by regions: `acquire()`
 * gives a record to a thread, `release()` gives it back, `enter()` and
 * `leave()` delimit an operation, inside which every node that the thread
 * reaches is protected, and `retire()`, inside an operation, hands a node
 * already unlinked to the reclamation, which frees it by the function of
 * its `EpochLink`. The contracts [see `Contract.h`]: a record is used by
 * one thread at a time and its operations do not overlap; a node is
 * retired once, after it has been unlinked; no record is in use when the
 * domain is destroyed, and the destructor frees the nodes still retired.
 *
 * \see K. Fraser, Practical lock-freedom, PhD thesis, technical report
 *      UCAM-CL-TR-579, University of Cambridge, 2004, doi:10.48456/tr-579
 * \see T. E. Hart, P. E. McKenney, A. D. Brown, J. Walpole, Performance of
 *      memory reclamation for lockless synchronization, Journal of Parallel
 *      and Distributed Computing 67(12), 1270-1285, 2007,
 *      doi:10.1016/j.jpdc.2007.04.010
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#ifndef __ADS_OwnedEpochs
 #define __ADS_OwnedEpochs
/* self-identification: #endif constant */

#include "ADS/Contract.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the link of a node retired: the next node of its list, and the function
/// that frees the node
struct EpochLink
{
 EpochLink * retired_next = nullptr;  ///< the next node retired
 void ( *reclaim )( EpochLink * ) noexcept = nullptr;  ///< the free
 };

/// the epochs of Fraser owned by one structure, with a record per thread
class OwnedEpochs
{

/*--------------------------------------------------------------------------*/
 public:
 /// the attempts to raise the epoch, one per R retirements of a record
 static constexpr unsigned kAttempt = 64;

 /// the record of a thread: the epoch it announces, the nodes it retired
 /// by the epoch of their retirement modulo 3, and whether it is held
 struct alignas( 64 ) Record
 {
  /// \f$ 2 e + 1 \f$ inside an operation of the epoch \f$ e \f$, 0 out
  std::atomic< std::uint64_t > local{ 0 };
  /// held by a thread
  std::atomic< bool > used{ true };
  /// the next record, fixed once the record is published
  Record * next = nullptr;
  /// the nodes retired, by the epoch of their retirement modulo 3
  EpochLink * limbo[ 3 ] = { nullptr , nullptr , nullptr };
  /// the epoch of the nodes of each limbo
  std::uint64_t epoch[ 3 ] = { 0 , 0 , 0 };
  /// the retirements since the last attempt to raise the epoch
  unsigned since = 0;
  /// the nodes retired and not yet freed
  std::size_t pending = 0;
  };

 OwnedEpochs( void ) = default;
 /// no copy, since the records point to the domain
 OwnedEpochs( const OwnedEpochs & ) = delete;
 /// no copy, since the records point to the domain
 OwnedEpochs & operator=( const OwnedEpochs & ) = delete;

 /// the frees of the nodes still retired and of the records
 /// \pre no record is in use
 ~OwnedEpochs() {
  for( Record * r = f_records.load( std::memory_order_acquire ) ; r ; ) {
   Record * q = r->next;
   for( EpochLink * l : r->limbo )
    freeAll( l );
   delete r;
   r = q;
   }
  }

 /// a record for one thread: one given back, else a new one
 [[nodiscard]] Record * acquire( void ) {
  for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
       r = r->next )
   if( ( ! r->used.load( std::memory_order_relaxed ) ) &&
       ( ! r->used.exchange( true , std::memory_order_acquire ) ) )
    return( r );
  Record * r = new Record;
  r->next = f_records.load( std::memory_order_relaxed );
  while( ! f_records.compare_exchange_weak(
    r->next , r , std::memory_order_release , std::memory_order_relaxed ) ) {
   }
  return( r );
  }

 /// the record r given back, for another thread
 static void release( Record * r ) noexcept {
  r->used.store( false , std::memory_order_release );
  }

 /// the epoch announced by r, before any read of the structure, and the
 /// nodes of r freed whose epoch is 2 behind
 ///
 /// The announce is a sequentially consistent exchange, the reads of the
 /// words through which a node is reached are sequentially consistent
 /// loads, and so is the read of the epoch by `retire()`; a read that sees
 /// a word older than the write that unlinks a node then precedes that
 /// write, and the announce precedes both, in the total order of the
 /// sequentially consistent operations [see \ref theory_concurrent_lfpq].
 void enter( Record & r ) noexcept {
  const std::uint64_t e = f_epoch.load( std::memory_order_seq_cst );
  r.local.exchange( 2 * e + 1 , std::memory_order_seq_cst );
  for( int b = 0 ; b < 3 ; ++b )
   if( r.limbo[ b ] && ( r.epoch[ b ] + 2 <= e ) ) {
    r.pending -= freeAll( r.limbo[ b ] );
    r.limbo[ b ] = nullptr;
    }
  }

 /// the end of the operation of r
 static void leave( Record & r ) noexcept {
  r.local.store( 0 , std::memory_order_release );
  }

 /// the node p, already unlinked, handed to the reclamation in the epoch
 /// now, inside an operation of r; once per R retirements of r, the epoch
 /// raised if every record inside an operation announces it
 void retire( Record & r , EpochLink * p ) noexcept {
  const std::uint64_t e = f_epoch.load( std::memory_order_seq_cst );
  const int b = int( e % 3 );
  if( r.epoch[ b ] != e ) {
   // the limbo of the same residue holds nodes of the epoch e - 3 or older
   ADS_INVARIANT( ( ! r.limbo[ b ] ) || ( r.epoch[ b ] + 2 <= e ) );
   r.pending -= freeAll( r.limbo[ b ] );
   r.limbo[ b ] = nullptr;
   r.epoch[ b ] = e;
   }
  p->retired_next = r.limbo[ b ];
  r.limbo[ b ] = p;
  ++r.pending;
  if( ++r.since >= kAttempt ) {
   r.since = 0;
   advance();
   }
  }

 /// the epoch, from 2
 [[nodiscard]] std::uint64_t epoch( void ) const {
  return( f_epoch.load( std::memory_order_relaxed ) );
  }

 /// the nodes retired and not yet freed, with no operation running
 [[nodiscard]] std::size_t pending( void ) const {
  std::size_t n = 0;
  for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
       r = r->next )
   n += r->pending;
  return( n );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the nodes of the list from p freed, and their number
 static std::size_t freeAll( EpochLink * p ) noexcept {
  std::size_t n = 0;
  while( p ) {
   EpochLink * q = p->retired_next;
   p->reclaim( p );
   p = q;
   ++n;
   }
  return( n );
  }

 /// the epoch raised if every record inside an operation announces it
 void advance( void ) noexcept {
  std::uint64_t e = f_epoch.load( std::memory_order_seq_cst );
  for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
       r = r->next ) {
   const std::uint64_t v = r->local.load( std::memory_order_seq_cst );
   if( ( v & 1 ) && ( v != 2 * e + 1 ) )
    return;  // a thread still in an older epoch
   }
  f_epoch.compare_exchange_strong( e , e + 1 , std::memory_order_seq_cst );
  }

 alignas( 64 ) std::atomic< std::uint64_t > f_epoch{ 2 };
 ///< the epoch, from 2
 alignas( 64 ) std::atomic< Record * > f_records{ nullptr };
 ///< the records

 };  // end( class OwnedEpochs )

}  // end( namespace detail )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* OwnedEpochs.tpp included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File OwnedEpochs.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
