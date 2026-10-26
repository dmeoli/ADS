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
 * (prepareWait(), which adds 1 to the waiters and reads the epoch e in the
 * same read-modify-write), looks at the condition once more, and if it is
 * still false sleeps until the epoch is no longer e (commitWait( e ), by
 * std::atomic::wait); otherwise it withdraws (cancelWait( e )). A notifier
 * makes the condition true, then notify() reads the word after a fence
 * and, only if there are waiters, advances the epoch and sets the waiters
 * to 0 in one compare-and-swap, and wakes them (std::atomic::notify_all):
 * the notifications that follow, while the woken threads are still on
 * their way, find nobody to wake and cost no system call. The fences are
 * asymmetric where the system has them (membarrier(2) on Linux): the
 * waiter, which is about to sleep anyway, runs the heavy one, a system
 * call that makes every running thread of the process run a full
 * barrier, and the notifier the light one, which only stops the compiler,
 * so that when nobody waits a notification costs the read of a line that
 * stays shared (a full fence would wait, at each item, for the stores of
 * the item to reach the caches of the other threads); elsewhere both are
 * full fences.
 * No wake-up is lost: the fence of the notifier and the read-modify-write
 * and fence of the waiter are in the single total order of the sequentially
 * consistent operations (a pair of asymmetric fences orders as a pair of
 * full ones); if the notifier reads 0 waiters, its fence comes before
 * the announcement of the waiter, which then sees the condition made true
 * when it looks again, and if it reads 1 or more it advances the epoch by
 * a release on the same word, which the announcement either precedes (and
 * the wait on e returns at once or is woken) or follows, reading the new
 * epoch with an acquire and seeing the condition made true. A spurious
 * wake-up only makes the waiter look again.
 * await( p ) packs the loop: it returns when p() is true, and p is called
 * by the waiting thread only, so that it may take an item; it first looks
 * again and again for 20 us (a parameter), with a pause of the processor
 * between the looks, since an item that comes within that time costs no
 * sleep and no wake-up (each about the time of a system call and of a
 * switch of threads), and a waiter that does not sleep costs its notifier
 * only the fence.
 * The contracts [see Contract.h]: each prepareWait() is followed by one
 * commitWait() or cancelWait() of the same thread, with the epoch it
 * returned.
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
 *         Universita' di Pisa \n
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
/// expedited private barrier of membarrier(2), registered once
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

/// the sleep of threads until a condition holds, woken by notify()
class EventCount {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::uint32_t;   ///< the epoch a waiter saw

 EventCount( void ) = default;
 EventCount( const EventCount & ) = delete;
 EventCount & operator=( const EventCount & ) = delete;

 /// announces a waiter and returns the epoch, before the last look at the
 /// condition
 [[nodiscard]] Key prepareWait( void ) noexcept {
  const std::uint64_t v = f_state.fetch_add( 1 , std::memory_order_seq_cst );
#ifndef ADS_EVENTCOUNT_RMW
  detail::heavyFence();
#endif
  return( Key( v >> 32 ) );
  }

 /// withdraws the waiter, the condition being true, unless a notifier
 /// already withdrew it (the epoch is no longer k)
 void cancelWait( Key k ) noexcept {
  std::uint64_t v = f_state.load( std::memory_order_relaxed );
  while( ( Key( v >> 32 ) == k ) &&
         ! f_state.compare_exchange_weak( v , v - 1 ,
                                          std::memory_order_relaxed ) ) {}
  }

 /// sleeps while the epoch is k; the notifier that advanced it withdrew
 /// the waiter
 void commitWait( Key k ) noexcept {
  while( f_epoch.load( std::memory_order_acquire ) == k )
   f_epoch.wait( k , std::memory_order_acquire );
  }

 /// wakes the waiters, after the condition was made true: it advances the
 /// epoch and withdraws them all at once, so that the notifications that
 /// follow, before they run, find nobody to wake; when nobody waits, a
 /// light fence and a read
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
    f_epoch.store( Key( ( v >> 32 ) + 1 ) , std::memory_order_release );
    f_epoch.notify_all();
    return;
    }
  }

 /// returns when p() is true: it looks again and again for up to spin
 /// nanoseconds, with a pause of the processor between the looks, and then
 /// sleeps between them
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
