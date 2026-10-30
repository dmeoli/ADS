/*--------------------------------------------------------------------------*/
/*------------------------------ File Farm.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The farm with feedback on the workers of an executor [see `Executor.h`],
 * beyond the book: a master emits tasks, \f$ w \f$ workers turn each task
 * into a result, and the results go back to the master, which may emit more
 * tasks from them and decides when the farm ends, the pattern of the
 * master and workers of FastFlow (its farm with the channel of feedback
 * from the workers to the emitter). `feedbackFarm< T , R >( ex , w , start ,
 * onResult , work )` calls `start( emit )` once, where `emit( t )` emits the
 * task \f$ t \f$, then `onResult( r , emit )` on each result \f$ r \f$, which
 * returns false to stop the farm, while the workers call `work( t )` on the
 * tasks; the farm ends when the master stops it or when no task is left
 * (none emitted and not yet worked out), and returns when all the calls
 * have returned. The tasks reach the workers on demand, i.e., a task goes to
 * the first worker that is free, and the master runs on its own thread
 * (see \ref theory_parallel_farm).
 *
 * On `FastFlowExecutor` the farm is a farm of FastFlow (`ff_Farm`) with an
 * emitter, \f$ w \f$ workers, no collector, the scheduling on demand and the
 * channel of feedback (`wrap_around()`), the tasks and the results passed as
 * pointers, as FastFlow passes them; on the other executors it is emulated
 * on their `run()`, with the master as the call 0 and the workers as the
 * calls \f$ 1 , \ldots , w \f$, the tasks on a queue of many producers and
 * many consumers [see `MPMCQueue.h`] from which the free workers take them,
 * and the results on a queue with a lane per worker [see `MPSCQueue.h`];
 * with \f$ w = 0 \f$ (or on `Sequential`) the master works out its tasks
 * itself, in the order of their emission. When the master stops the farm,
 * the tasks not yet taken are dropped, the workers end after the task they
 * hold, and its result is dropped.
 *
 * The contracts [see `Contract.h`]: \f$ w + 1 \le \f$ `concurrency()` but on
 * FastFlow, where \f$ w \le \f$ `concurrency()` and the emitter has a thread
 * of its own; the first exception thrown by `onResult()` or by `work()`
 * stops the farm and is thrown again once all the calls have returned.
 *
 * \see M. Aldinucci, M. Danelutto, P. Kilpatrick, M. Torquati, FastFlow:
 *      high-level and efficient streaming on multicore, in S. Pllana,
 *      F. Xhafa (eds.), Programming Multi-core and Many-core Computing
 *      Systems, Wiley, 261-280, 2017, doi:10.1002/9781119332015.ch13
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

#ifndef __ADS_Farm
 #define __ADS_Farm
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/MPMCQueue.h"
#include "ADS/concurrent/MPSCQueue.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"

#include <atomic>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the farm of a master and \f$ w \f$ workers with feedback: `start( emit )`,
/// then `onResult( r , emit )` on each result of `work( t )`, until it
/// returns false or no task is left
///
/// In pseudocode (the farm with feedback of FastFlow, beyond the book),
/// \f$ Q \f$ the tasks, \f$ P \f$ the results, \f$ m \f$ the tasks emitted
/// and not yet back:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{feedbackFarm}( \mathit{ex} , w ,
///     \mathit{start} , \mathit{onResult} , \mathit{work} ) \cr
/// 2 & \quad \mathit{ex}.\mathrm{run}( w + 1 , i \mapsto \textbf{if}\ i = 0
///     \ \textbf{then}\ \mathrm{master}()\ \textbf{else}\ \mathrm{worker}()
///     ) \cr
/// 3 & \textbf{procedure}\ \mathrm{master}() \cr
/// 4 & \quad \mathit{start}( t \mapsto \mathrm{enqueue}( Q , t );\ m \gets
///     m + 1 ) \cr
/// 5 & \quad \textbf{while}\ m > 0\ \textbf{do} \cr
/// 6 & \qquad \textbf{foreach}\ r \text{ dequeued from } P\ \textbf{do} \cr
/// 7 & \qquad\quad m \gets m - 1 \cr
/// 8 & \qquad\quad \textbf{if}\ \textbf{not}\ \mathit{onResult}( r , t
///     \mapsto \mathrm{enqueue}( Q , t );\ m \gets m + 1 )\ \textbf{then}
///     \ m \gets 0;\ \textbf{break} \cr
/// 9 & \quad \text{tell the workers to end} \cr
/// 10 & \textbf{procedure}\ \mathrm{worker}() \cr
/// 11 & \quad \textbf{while}\ \text{not told to end}\ \textbf{do}
///     \ \textbf{if}\ t \text{ dequeued from } Q\ \textbf{then}
///     \ \mathrm{enqueue}( P , \mathit{work}( t ) )
/// \end{array}}
/// \f]
/// \pre \f$ w + 1 \le \f$ `concurrency()`, or \f$ w = 0 \f$
template< class T , class R , WorkerExecutor E , class S , class O ,
          class W >
void feedbackFarm( E & ex , std::size_t w , S && start , O && onResult ,
                   W && work );

#if defined( ADS_WITH_FASTFLOW )

/// the farm with feedback of `feedbackFarm()` above on a farm of FastFlow,
/// with an emitter of its own and \f$ w \f$ workers
///
/// In pseudocode (beyond the book), the emitter called by FastFlow on
/// nothing at the start and then on each result:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{emitter}( r ) \cr
/// 2 & \quad \textbf{if}\ r = \textbf{nil}\ \textbf{then}\ \mathit{start}(
///     \mathit{emit} ) \cr
/// 3 & \quad \textbf{else}\ m \gets m - 1;\ \textbf{if}\ \text{not stopped }
///     \textbf{and not}\ \mathit{onResult}( r , \mathit{emit} )\ \textbf{then}
///     \ \text{stop} \cr
/// 4 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ \text{the end of }
///     \text{the stream}\ \textbf{else return}\ \text{nothing}
/// \end{array}}
/// \f]
/// \pre \f$ w \le \f$ `concurrency()`
template< class T , class R , class S , class O , class W >
void feedbackFarm( FastFlowExecutor & ex , std::size_t w , S && start ,
                   O && onResult , W && work );

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/Farm.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Farm.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Farm.h -----------------------------*/
/*--------------------------------------------------------------------------*/
