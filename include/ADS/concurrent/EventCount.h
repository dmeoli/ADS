/*--------------------------------------------------------------------------*/
/*--------------------------- File EventCount.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An event count, beyond the book: the way for a thread to sleep until a
 * condition on a lock-free structure holds, instead of looking at it again
 * and again or between sleeps of a fixed length, with no lock and nearly no
 * cost for the threads that make the condition true while nobody waits.
 * It holds an epoch and the number of the waiters, the two halves of one
 * word of 64 bits on a line of its own. A waiter announces itself
 * (`prepareWait()`, which adds 1 to the waiters and reads the epoch
 * \f$ e \f$ in the same read-modify-write), looks at the condition once
 * more, and if it is still false sleeps until the epoch is no longer
 * \f$ e \f$ (`commitWait( e )`, by `std::atomic::wait`); otherwise it
 * withdraws (`cancelWait( e )`). A notifier makes the condition true, then
 * `notify()` reads the word after a fence and, only if there are waiters,
 * advances the epoch and sets the waiters to 0 in one compare-and-swap, and
 * wakes them (`std::atomic::notify_all`): the notifications that follow,
 * while the woken threads are still on their way, find nobody to wake and
 * cost no system call.
 *
 * The fences are asymmetric where the system has them (membarrier(2) on
 * Linux): the waiter, which is about to sleep anyway, runs the heavy one, a
 * system call that makes every running thread of the process run a full
 * barrier, and the notifier the light one, which only stops the compiler,
 * so that when nobody waits a notification costs the read of a line that
 * stays shared (a full fence would wait, at each item, for the stores of
 * the item to reach the caches of the other threads); elsewhere both are
 * full fences.
 *
 * No wake-up is lost: the fence of the notifier and the read-modify-write
 * and fence of the waiter are in the single total order of the
 * sequentially consistent operations (a pair of asymmetric fences orders as
 * a pair of full ones); if the notifier reads 0 waiters, its fence comes
 * before the announcement of the waiter, which then sees the condition made
 * true when it looks again, and if it reads 1 or more it advances the epoch
 * by a release on the same word, which the announcement either precedes
 * (and the wait on \f$ e \f$ returns at once or is woken) or follows,
 * reading the new epoch with an acquire and seeing the condition made true.
 * A spurious wake-up only makes the waiter look again.
 *
 * `await( p )` packs the loop: it returns when `p()` is true, and `p` is
 * called by the waiting thread only, so that it may take an item; it first
 * looks again and again for 20 \f$ \mu\mathrm{s} \f$ (a parameter), with
 * a pause of the processor between the looks, since an item that comes
 * within that time costs no sleep and no wake-up (each about the time of a
 * system call and of a switch of threads), and a waiter that does not sleep
 * costs its notifier only the fence.
 *
 * The contracts [see `Contract.h`]: each `prepareWait()` is followed by one
 * `commitWait()` or `cancelWait()` of the same thread, with the epoch it
 * returned.
 *
 * \see D. P. Reed, R. K. Kanodia, Synchronization with eventcounts and
 *      sequencers, Communications of the ACM 22(2), 115-123, 1979,
 *      doi:10.1145/359060.359076 (the event count)
 * \see A. Williams, C++ Concurrency in Action, 2nd ed., Manning, 2019,
 *      chapter 5 (the total order of the sequentially consistent
 *      operations and the fences)
 * \see H. Boehm, S. V. Adve, Foundations of the C++ concurrency memory
 *      model, ACM SIGPLAN Conference on Programming Language Design and
 *      Implementation (PLDI), 68-78, 2008, doi:10.1145/1375581.1375591
 * \see E. Ladan-Mozes, I.-T. A. Lee, D. Vyukov, Location-based memory
 *      fences, ACM Symposium on Parallelism in Algorithms and
 *      Architectures (SPAA), 75-84, 2011, doi:10.1145/1989493.1989503
 *      (the asymmetric fences)
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_EventCount
 #define __ADS_EventCount
                      /* self-identification: #endif constant */

// under the sanitizer of threads, which does not model the fences, the
// notifier reads the waiters by a read-modify-write instead (reading the
// last value in the order of the modifications, it either sees the
// waiter or is seen by it through the release sequence)
#if defined( __SANITIZE_THREAD__ )
 #define ADS_EVENTCOUNT_RMW
#elif defined( __has_feature )
 #if __has_feature( thread_sanitizer )
  #define ADS_EVENTCOUNT_RMW
 #endif
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <atomic>
#include <chrono>
#include <cstdint>

#if defined( __linux__ ) && ! defined( ADS_EVENTCOUNT_RMW ) && \
    __has_include( <linux/membarrier.h> )
 #define ADS_EVENTCOUNT_MEMBARRIER
 #include <linux/membarrier.h>
 #include <sys/syscall.h>
 #include <unistd.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// true if the process may use the asymmetric fences: on Linux, the
/// expedited private barrier of `membarrier(2)`, registered once
inline bool asymmetricFences( void ) noexcept
{
#ifdef ADS_EVENTCOUNT_MEMBARRIER
 static const bool ok = [] {
  const long q = syscall( SYS_membarrier , MEMBARRIER_CMD_QUERY , 0 );
  return( ( q >= 0 ) && ( q & MEMBARRIER_CMD_PRIVATE_EXPEDITED ) &&
          ( syscall( SYS_membarrier ,
                     MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED , 0 ) == 0 ) );
  }();
 return( ok );
#else
 return( false );
#endif
 }

/// the light side of a fence: only the compiler's when the heavy side
/// makes every running thread of the process run a full barrier, a full
/// fence otherwise
inline void lightFence( void ) noexcept
{
 if( asymmetricFences() )
  std::atomic_signal_fence( std::memory_order_seq_cst );
 else
  std::atomic_thread_fence( std::memory_order_seq_cst );
 }

/// the heavy side: a full barrier on every running thread of the process
inline void heavyFence( void ) noexcept
{
#ifdef ADS_EVENTCOUNT_MEMBARRIER
 if( asymmetricFences() &&
     ( syscall( SYS_membarrier , MEMBARRIER_CMD_PRIVATE_EXPEDITED , 0 ) ==
       0 ) )
  return;
#endif
 std::atomic_thread_fence( std::memory_order_seq_cst );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS EventCount -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the sleep of threads until a condition holds, woken by `notify()`
class EventCount {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::uint32_t;   ///< the epoch a waiter saw

 /// the event count of epoch 0 and no waiter
 EventCount( void ) = default;
 /// no copy, since the waiters sleep on its address
 EventCount( const EventCount & ) = delete;
 /// no copy, since the waiters sleep on its address
 EventCount & operator=( const EventCount & ) = delete;

 /// announces a waiter and returns the epoch, before the last look at the
 /// condition
 ///
 /// In pseudocode (beyond the book), \f$ ( e , w ) \f$ the word of the
 /// epoch and of the waiters:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prepareWait}() \cr
 /// 2 & \quad ( e' , w' ) \gets \mathrm{fetchAdd}( ( e , w ) , 1 )\quad
 ///     \triangleright\ \text{sequentially consistent} \cr
 /// 3 & \quad \text{the heavy fence} \cr
 /// 4 & \quad \textbf{return}\ e'
 /// \end{array}}
 /// \f]
 [[nodiscard]] Key prepareWait( void ) noexcept {
  const std::uint64_t v = f_state.fetch_add( 1 , std::memory_order_seq_cst );
#ifndef ADS_EVENTCOUNT_RMW
  detail::heavyFence();
#endif
  return( Key( v >> 32 ) );
  }

 /// withdraws the waiter, the condition being true, unless a notifier
 /// already withdrew it (the epoch is no longer \f$ k \f$)
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{cancelWait}( k ) \cr
 /// 2 & \quad \textbf{repeat}\ ( e' , w' ) \gets ( e , w )
 ///     \ \textbf{until}\ e' \ne k\ \textbf{or}\ \mathrm{CAS}( ( e , w ) , (
 ///     e' , w' ) , ( e' , w' - 1 ) )
 /// \end{array}}
 /// \f]
 void cancelWait( Key k ) noexcept {
  std::uint64_t v = f_state.load( std::memory_order_relaxed );
  while( ( Key( v >> 32 ) == k ) &&
         ! f_state.compare_exchange_weak( v , v - 1 ,
                                          std::memory_order_relaxed ) ) {}
  }

 /// sleeps while the epoch is \f$ k \f$; the notifier that advanced it
 /// withdrew the waiter
 ///
 /// In pseudocode (beyond the book), \f$ E \f$ the copy of the epoch on
 /// which the waiters sleep:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{commitWait}( k ) \cr
 /// 2 & \quad \textbf{while}\ E = k\ \textbf{do}\ \text{sleep until } E
 ///     \text{ changes}\quad \triangleright\ \text{acquire}
 /// \end{array}}
 /// \f]
 void commitWait( Key k ) noexcept {
  while( f_epoch.load( std::memory_order_acquire ) == k )
   f_epoch.wait( k , std::memory_order_acquire );
  }

 /// wakes the waiters, after the condition was made true: it advances the
 /// epoch and withdraws them all at once, so that the notifications that
 /// follow, before they run, find nobody to wake; when nobody waits, a
 /// light fence and a read
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{notify}() \cr
 /// 2 & \quad \text{the light fence};\ ( e' , w' ) \gets ( e , w ) \cr
 /// 3 & \quad \textbf{while}\ w' > 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \mathrm{CAS}( ( e , w ) , ( e' , w' ) , ( e'
 ///     + 1 , 0 ) )\ \textbf{then}\quad \triangleright\ \text{acquire and}
 ///     \text{ release} \cr
 /// 5 & \qquad\quad E \gets e' + 1\quad \triangleright\ \text{release};
 ///     \ \text{wake all the threads asleep on } E \cr
 /// 6 & \qquad\quad \textbf{return} \cr
 /// 7 & \qquad ( e' , w' ) \gets ( e , w )\quad \triangleright\ \text{read}
 ///     \text{ by the failed CAS}
 /// \end{array}}
 /// \f]
 void notify( void ) noexcept {
#ifndef ADS_EVENTCOUNT_RMW
  detail::lightFence();
  std::uint64_t v = f_state.load( std::memory_order_relaxed );
#else
  std::uint64_t v = f_state.fetch_add( 0 , std::memory_order_seq_cst );
#endif
  while( v & waiterMask ) [[unlikely]]
   if( f_state.compare_exchange_weak( v , ( ( v >> 32 ) + 1 ) << 32 ,
                                      std::memory_order_acq_rel ,
                                      std::memory_order_relaxed ) ) {
    raiseEpoch( Key( ( v >> 32 ) + 1 ) );
    f_epoch.notify_all();
    return;
    }
  }

 /// returns when `p()` is true: it looks again and again for up to `spin`
 /// nanoseconds, with a pause of the processor between the looks, and then
 /// sleeps between them
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{await}( p , \mathit{spin} ) \cr
 /// 2 & \quad \textbf{repeat}\ \textbf{if}\ p()\ \textbf{then return};
 ///     \ \text{a pause}\ \textbf{until}\ \mathit{spin} \text{ ns have}
 ///     \text{ passed} \cr
 /// 3 & \quad \textbf{while not}\ p()\ \textbf{do} \cr
 /// 4 & \qquad k \gets \mathrm{prepareWait}() \cr
 /// 5 & \qquad \textbf{if}\ p()\ \textbf{then}\ \mathrm{cancelWait}( k );
 ///     \ \textbf{return} \cr
 /// 6 & \qquad \mathrm{commitWait}( k )
 /// \end{array}}
 /// \f]
 template< class P >
 void await( P && p , std::int64_t spin = 20000 ) {
  if( p() )
   return;
  const auto end = std::chrono::steady_clock::now() +
                   std::chrono::nanoseconds( spin );
  for( unsigned i = 1 ; ; ++i ) {           // the clock every 64 looks
   relax();
   if( p() )
    return;
   if( ( ( i & 63 ) == 0 ) && ( std::chrono::steady_clock::now() >= end ) )
    break;
   }
  while( ! p() ) {
   const Key k = prepareWait();
   if( p() ) {
    cancelWait( k );
    return;
    }
   commitWait( k );
   }
  }

 /// the number of the threads announced as waiters at some moment
 [[nodiscard]] std::uint32_t waiters( void ) const noexcept {
  return( std::uint32_t( f_state.load( std::memory_order_relaxed ) &
                         waiterMask ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a pause of the processor in a loop that looks at memory
 static void relax( void ) noexcept {
#if defined( __x86_64__ ) || defined( __i386__ )
  __builtin_ia32_pause();
#elif defined( __aarch64__ )
  asm volatile( "yield" );
#endif
  }

 /// the copy of the epoch raised to \f$ e \f$, never lowered: two notifiers
 /// that advanced the epoch of `f_state` to \f$ e - 1 \f$ and \f$ e \f$ may
 /// reach `f_epoch` in the other order, and a store of \f$ e - 1 \f$ after
 /// \f$ e \f$ would put back the epoch of a waiter that the second one
 /// withdrew, which would then sleep with the condition true and nobody to
 /// wake it. The epochs are compared modulo \f$ 2^{32} \f$, as the keys
 /// are, \f$ e \f$ being ahead of the copy \f$ c \f$ when
 /// \f$ 0 < ( e - c ) \bmod 2^{32} < 2^{31} \f$. Each change is a
 /// read-modify-write with release: a waiter that reads \f$ e \f$ with an
 /// acquire synchronizes with the notifier that wrote it, or with a later
 /// one in its release sequence, and that notifier read the epochs before
 /// \f$ e \f$ in `f_state` by acquire, after the notifiers that advanced
 /// them made their conditions true.
 void raiseEpoch( Key e ) noexcept {
  Key c = f_epoch.load( std::memory_order_relaxed );
  while( ( std::int32_t( e - c ) > 0 ) &&
         ! f_epoch.compare_exchange_weak( c , e ,
                                          std::memory_order_release ,
                                          std::memory_order_relaxed ) ) {}
  }

 static constexpr std::uint64_t waiterMask = 0xFFFFFFFFu;

 /// the epoch in the high half, the waiters in the low one
 alignas( 64 ) std::atomic< std::uint64_t > f_state{ 0 };
 /// a copy of the epoch, of the size of a futex, on which the waiters
 /// sleep (the wait of std::atomic on a word of 64 bits goes through a
 /// table of proxies, and may wake late)
 std::atomic< Key > f_epoch{ 0 };
 [[maybe_unused]]
 char f_pad[ 64 - sizeof( std::uint64_t ) - sizeof( Key ) ] = {};

 };  // end( class EventCount )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* EventCount.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File EventCount.h --------------------------*/
/*--------------------------------------------------------------------------*/
