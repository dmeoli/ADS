/*--------------------------------------------------------------------------*/
/*------------------------- File QueueAlgorithms.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms that use a QUEUE [see `ADT.h`] only through its operations,
 * beyond the book:
 *
 * - `extractPositive()` moves the positive items of \f$ q \f$, in their
 *   order, to the back of another queue, and leaves \f$ q \f$ empty, in
 *   \f$ O( n ) \f$;
 * - `copyPositive()` does the same but leaves \f$ q \f$ as it was, by
 *   moving each item to an auxiliary queue and then back.
 *
 * The contracts [see `Contract.h`]: the two queues are distinct objects, and
 * `copyPositive()` ensures that \f$ q \f$ is left as it was (an audit). The
 * algorithms are defined in `QueueAlgorithms.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_QueueAlgorithms
 #define __ADS_QueueAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"
#include "ADS/queue/LinkedQueue.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// moves the positive items of \f$ q \f$, in their order, to the back of
/// `out`, leaving \f$ q \f$ empty
///
/// In pseudocode (section 4.3.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{extractPositive}( q , \mathit{out} ) \cr
/// 2 & \quad \textbf{while not}\ \mathrm{empty}( q )\ \textbf{do} \cr
/// 3 & \qquad x \gets \mathrm{dequeue}( q );\ \textbf{if}\ x > 0
///     \ \textbf{then}\ \mathrm{enqueue}( \mathit{out} , x )
/// \end{array}}
/// \f]
/// \pre \f$ q \f$ and `out` are distinct
/// \post \f$ q \f$ is empty
template< Queue Q >
void extractPositive( Q & q , Q & out );

/// copies the positive items of \f$ q \f$, in their order, to the back of
/// `out`, leaving \f$ q \f$ as it was
///
/// In pseudocode (section 4.3.1), an auxiliary queue \f$ A \f$ keeping the
/// order of \f$ q \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{copyPositive}( q , \mathit{out} ) \cr
/// 2 & \quad A \gets \text{an empty queue} \cr
/// 3 & \quad \textbf{while not}\ \mathrm{empty}( q )\ \textbf{do} \cr
/// 4 & \qquad x \gets \mathrm{dequeue}( q );\ \mathrm{enqueue}( A , x ) \cr
/// 5 & \qquad \textbf{if}\ x > 0\ \textbf{then}\ \mathrm{enqueue}(
///     \mathit{out} , x ) \cr
/// 6 & \quad \textbf{while not}\ \mathrm{empty}( A )\ \textbf{do}
///     \ \mathrm{enqueue}( q , \mathrm{dequeue}( A ) )
/// \end{array}}
/// \f]
/// \pre \f$ q \f$ and `out` are distinct
/// \post \f$ q \f$ is as it was (an audit)
template< Queue Q >
void copyPositive( Q & q , Q & out );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/QueueAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* QueueAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File QueueAlgorithms.h -----------------------*/
/*--------------------------------------------------------------------------*/
