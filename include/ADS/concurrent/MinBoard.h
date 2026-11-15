/*--------------------------------------------------------------------------*/
/*---------------------------- File MinBoard.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A board of the least keys of \f$ p \f$ threads, beyond the book: each
 * thread \f$ i \f$ owns a slot, where `publish( i , k )` writes its key
 * \f$ k \f$ (e.g., the least \f$ f \f$ of the OPEN list of a worker of a
 * parallel search), and any thread reads by `min()` the least of the
 * slots (the bound that the workers compare their own with). The slots are
 * atomic words on lines of cache of their own, so that a publish writes
 * one line that no other slot shares, and `publish()` stores only when the
 * key changes, so that a worker whose key stays leaves its line in the
 * caches of the readers; every access is relaxed, with no read-modify-write
 * and no fence, so that a publish costs a load and at most a store, and a
 * `min()` \f$ p \f$ loads.
 *
 * The board gives coherence only. Each slot has one writer, its owner, and
 * a modification order, that of its publishes; a read of a slot reads one
 * of its values, not older than any value of the slot whose publish
 * happens before the read (in particular, the last one of the reader
 * itself, if it is the owner), and two reads of one slot by one thread
 * never read the second older than the first. `min()` returns the least of
 * the \f$ p \f$ values that it reads, one per slot and not at the same
 * time, so that it need not be the least of the slots at any instant; when
 * the keys of each slot only decrease (or only grow), the value returned
 * lies between the least key of the board at the end of the call and that
 * at its start (resp. the reverse), and the values returned to one thread
 * only decrease (resp. grow). The relaxed accesses order nothing else: a
 * thread that reads \f$ k \f$ in a slot does not see by that read the writes
 * that the owner made before publishing it, which need their own
 * synchronization (see \ref theory_concurrent_board).
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; a slot \f$ i < p \f$
 * is written only by one thread at a time (its owner); `reset()` runs with
 * no other operation.
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

#ifndef __ADS_MinBoard
 #define __ADS_MinBoard
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MinBoard ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the least of the keys of \f$ p \f$ threads, one slot of a line of cache
/// per thread, published and read by relaxed accesses
template< class K >
 requires( std::totally_ordered< K > && std::is_trivially_copyable_v< K > )
class MinBoard
{

 static_assert( std::atomic< K >::is_always_lock_free ,
		"the keys are atomic words without locks" );

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;  ///< the type of the keys

 /// the bytes of a slot, a line of cache
 static constexpr std::size_t kLine = 64;

 /// the board of \f$ p \f$ slots, each holding \f$ k \f$ (by default the
 /// largest key, which no published key is above)
 /// \pre \f$ p \ge 1 \f$
 explicit MinBoard( std::size_t p , K k = std::numeric_limits< K >::max() )
     : f_p( p ) , f_s( new Slot[ p ] ) {
  ADS_PRE( p >= 1 );
  for( std::size_t i = 0 ; i < p ; ++i )
   f_s[ i ].k.store( k , std::memory_order_relaxed );
  }

 /// the number \f$ p \f$ of the slots
 [[nodiscard]] std::size_t size( void ) const { return( f_p ); }

 /// \f$ k \f$ in the slot \f$ i \f$, by a relaxed store made only if the
 /// slot holds another key, read by a relaxed load
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{publish}( i , k ) \cr
 /// 2 & \quad \textbf{if}\ S[ i ] \ne k\ \textbf{then}\ S[ i ] \gets k
 ///     \quad \triangleright\ \text{relaxed, the line written only then}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < p \f$; the caller is the only writer of the slot
 void publish( std::size_t i , K k ) {
  ADS_PRE( i < f_p );
  std::atomic< K > & s = f_s[ i ].k;
  if( s.load( std::memory_order_relaxed ) != k )
   s.store( k , std::memory_order_relaxed );
  }

 /// the key of the slot \f$ i \f$, by a relaxed load
 /// \pre \f$ i < p \f$
 [[nodiscard]] K at( std::size_t i ) const {
  ADS_PRE( i < f_p );
  return( f_s[ i ].k.load( std::memory_order_relaxed ) );
  }

 /// the least of the keys read in the slots, one relaxed load each: a
 /// value of each slot not older than those that happen before the call,
 /// not a snapshot of the board
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{min}() \cr
 /// 2 & \quad m \gets S[ 0 ] \cr
 /// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ p - 1\ \textbf{do}
 ///     \ m \gets \min( m , S[ i ] )\quad \triangleright\ \text{relaxed} \cr
 /// 4 & \quad \textbf{return}\ m
 /// \end{array}}
 /// \f]
 [[nodiscard]] K min( void ) const {
  K m = f_s[ 0 ].k.load( std::memory_order_relaxed );
  for( std::size_t i = 1 ; i < f_p ; ++i ) {
   const K x = f_s[ i ].k.load( std::memory_order_relaxed );
   if( x < m )
    m = x;
   }
  return( m );
  }

 /// every slot set to \f$ k \f$
 /// \pre no other thread accesses the board
 void reset( K k = std::numeric_limits< K >::max() ) {
  for( std::size_t i = 0 ; i < f_p ; ++i )
   f_s[ i ].k.store( k , std::memory_order_relaxed );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// a slot on a line of its own
 struct alignas( kLine ) Slot
 {
  std::atomic< K > k;  ///< the key
  };

 static_assert( sizeof( Slot ) == kLine );

 std::size_t f_p;                ///< the slots
 std::unique_ptr< Slot[] > f_s;  ///< the slots, on lines of their own

 };  // end( class MinBoard )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* MinBoard.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MinBoard.h ---------------------------*/
/*--------------------------------------------------------------------------*/
