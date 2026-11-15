/*--------------------------------------------------------------------------*/
/*------------------------- File SenderExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on a scheduler of the senders and the
 * receivers (the `std::execution` of C++26, P2300), beyond the book, on the
 * implementation of reference, stdexec: `SenderExecutor< Sch >` runs
 * `bulk( n , f )` as the sender `schedule( s ) | bulk( par , n , f )` of the
 * scheduler \f$ s \f$, waited by `sync_wait()`, so that the parallel
 * algorithms of ADS run on whatever scheduler the program has: the
 * `static_thread_pool` of stdexec, its system scheduler, the pools of
 * oneTBB and of Taskflow that stdexec wraps, a scheduler of HPX, or one of
 * the program itself. The standard has no query of the threads of a
 * scheduler, so that `concurrency()` is the number given to the
 * constructor.
 *
 * A `sync_wait()` holds the thread that waits. On a thread of the pool of
 * the scheduler, the fork and join of ADS (the recursion of
 * `parallelInvoke()`, the trees by join, the sorts) would hold all of them
 * and wait for ever; a call of `bulk()` made inside a call of the executor
 * therefore runs its calls in order on that thread
 * [see `detail::InsideCall`], which the contract of the executors allows
 * (a thread may run several calls one after the other). The cost is the
 * parallelism of the nested calls, which is lost, while the algorithms
 * whose parallelism is in one level (the loops, the reductions, the prefix
 * sums, the sorts at their top level, the visits of the graphs) keep all of
 * theirs; on the executors that nest (`ThreadPool`, `TBBExecutor`,
 * `OpenMPExecutor`) nothing is lost. There is no `run()`, since a scheduler
 * does not promise that the calls of a bulk run at the same time.
 *
 * The header is compiled where the headers of stdexec are found
 * (`<stdexec/execution.hpp>`, or `ADS_WITH_STDEXEC`, which the CMake target
 * `ADS::stdexec` defines). The interface of stdexec still changes, and the
 * header follows the wording of C++26 (the policy as the first argument of
 * `bulk`), which stdexec has since 2025.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; an exception thrown by
 * \f$ f \f$ stops the claims and is thrown again by `bulk()`.
 *
 * \see M. Dominiak, G. Evtushenko, L. Baker, L. R. Teodorescu, L. Howes,
 *      K. Shoop, M. Garland, E. Niebler, B. A. Lelbach, std::execution,
 *      P2300R10, ISO/IEC JTC1/SC22/WG21, 2024,
 *      https://wg21.link/p2300r10
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

#ifndef __ADS_SenderExecutor
 #define __ADS_SenderExecutor
/* self-identification: #endif constant */

#if defined( ADS_WITH_STDEXEC ) || __has_include( <stdexec/execution.hpp> )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <stdexec/execution.hpp>

#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SenderExecutor ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on a scheduler of the senders
template< stdexec::scheduler Sch >
class SenderExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the executor of the scheduler \f$ s \f$, which tells the algorithms of
 /// ADS that it has \f$ p \f$ threads
 /// \pre \f$ p \ge 1 \f$
 SenderExecutor( Sch s , std::size_t p ) : f_s( std::move( s ) ) , f_p( p ) {
  ADS_PRE( p >= 1 );
  }

 /// the number of threads given to the constructor
 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// the scheduler
 [[nodiscard]] const Sch & scheduler( void ) const { return( f_s ); }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ by a bulk sender on the
 /// scheduler, in order on the calling thread inside a call
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{or}
 ///     \ \text{inside a call of the executor}\ \textbf{then} \cr
 /// 3 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ f( i ) \cr
 /// 4 & \quad \textbf{else}\ \mathrm{syncWait}( \mathrm{schedule}( s )
 ///     \mid \mathrm{bulk}( \mathit{par} , n , i \mapsto f( i )
 ///     \text{ marked inside} ) ) \cr
 /// 5 & \quad \text{throw again the first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( f_p == 1 ) || Inside::active() ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  detail::FirstError error;
  auto body = [ & ]( std::size_t i ) noexcept {
   if( error.failed() )
    return;
   Inside mark;
   try {
    f( i );
    }
   catch( ... ) {
    error.keep();
    }
   };
  stdexec::sync_wait( stdexec::schedule( f_s ) |
		      stdexec::bulk( stdexec::par , n , body ) );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 struct Tag;  ///< the tag of the mark of the calls
 using Inside = detail::InsideCall< Tag >;

 Sch f_s;          ///< the scheduler
 std::size_t f_p;  ///< the threads
 };  // end( class SenderExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_STDEXEC */

/*--------------------------------------------------------------------------*/

#endif /* SenderExecutor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SenderExecutor.h ------------------------*/
/*--------------------------------------------------------------------------*/
