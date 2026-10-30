/*--------------------------------------------------------------------------*/
/*----------------------------- File Backoff.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The exponential backoff of a thread that waits for a word of memory to
 * change, beyond the book: the \f$ k \f$-th call of `pause()` runs
 * \f$ 2^{\min\{ k , S \}} \f$ pauses of the processor (`pause` on x86,
 * `yield` on AArch64) while \f$ k \le S \f$, and from then on gives the
 * processor back to the system (`std::this_thread::yield()`), so that a
 * wait that ends soon costs no system call and one that lasts lets the
 * other threads run; `reset()` starts again from one pause, after the
 * wait has ended or the thread has done some work. With \f$ S = 6 \f$, the
 * default, the spinning lasts \f$ 2^7 - 1 = 127 \f$ pauses, a few
 * microseconds. The growth by doubling spreads the retries of threads that
 * compete for one line of cache, whose loads and failed compare-and-swaps
 * would otherwise keep the line moving between the caches (Anderson;
 * Mellor-Crummey and Scott); `spinning()` tells whether the next call still
 * spins, so that a waiter can move on to sleep, e.g., on an `EventCount`
 * [see `EventCount.h`], once it yields (see \ref theory_concurrent_wait).
 *
 * The contracts [see `Contract.h`]: \f$ 0 \le S \le 16 \f$. There is no
 * other operation to define.
 *
 * \see T. E. Anderson, The performance of spin lock alternatives for
 *      shared-memory multiprocessors, IEEE Transactions on Parallel and
 *      Distributed Systems 1(1), 6-16, 1990, doi:10.1109/71.80120
 * \see J. M. Mellor-Crummey, M. L. Scott, Algorithms for scalable
 *      synchronization on shared-memory multiprocessors, ACM Transactions
 *      on Computer Systems 9(1), 21-65, 1991, doi:10.1145/103727.103729
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

#ifndef __ADS_Backoff
 #define __ADS_Backoff
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/impl/Spin.tpp"

#include <thread>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Backoff -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the exponential backoff of a waiting thread: \f$ 2^k \f$ pauses at the
/// \f$ k \f$-th call up to \f$ S \f$, then a yield to the system
class Backoff {

/*--------------------------------------------------------------------------*/
 public:

 /// the backoff that spins on the first \f$ S + 1 \f$ calls
 /// \pre \f$ 0 \le S \le 16 \f$
 explicit Backoff( int S = 6 ) : f_limit( S ) {
  ADS_PRE( ( S >= 0 ) && ( S <= 16 ) );
  }

 /// waits a little: \f$ 2^k \f$ pauses of the processor at the call
 /// \f$ k \f$ from the last `reset()` if \f$ k \le S \f$, a yield of the
 /// thread otherwise
 ///
 /// In pseudocode (Anderson, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{pause}() \cr
 /// 2 & \quad \textbf{if}\ k \le S\ \textbf{then} \cr
 /// 3 & \qquad \textbf{for}\ i \gets 1\ \textbf{to}\ 2^k\ \textbf{do}
 ///     \ \text{a pause of the processor} \cr
 /// 4 & \qquad k \gets k + 1 \cr
 /// 5 & \quad \textbf{else}\ \text{yield the processor}
 /// \end{array}}
 /// \f]
 void pause( void ) noexcept {
  if( f_k <= f_limit ) {
   for( unsigned i = 0 , n = 1u << f_k ; i < n ; ++i )
    detail::spinPause();
   ++f_k;
   }
  else
   std::this_thread::yield();
  }

 /// starts again from one pause
 void reset( void ) noexcept { f_k = 0; }

 /// whether the next `pause()` spins, i.e., \f$ k \le S \f$
 [[nodiscard]] bool spinning( void ) const noexcept {
  return( f_k <= f_limit );
  }

 /// the calls of `pause()` since the last `reset()` that spun, \f$ k \f$
 [[nodiscard]] int step( void ) const noexcept { return( f_k ); }

/*--------------------------------------------------------------------------*/
 private:

 int f_limit;    ///< the last step that spins, \f$ S \f$
 int f_k = 0;    ///< the step, \f$ k \f$

 };  // end( class Backoff )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Backoff.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Backoff.h ---------------------------*/
/*--------------------------------------------------------------------------*/
