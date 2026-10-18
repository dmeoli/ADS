/*--------------------------------------------------------------------------*/
/*------------------------- File QueueAlgorithms.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms that use a QUEUE [see ADT.h] only through its operations,
 * beyond the book:
 *
 * - extractPositive() moves the positive items of q, in their order, to
 *   the back of another queue, and leaves q empty, in O( n );
 * - copyPositive() does the same but leaves q as it was, by moving each
 *   item to an auxiliary queue and then back.
 *
 * The contracts [see Contract.h]: the two queues are distinct objects, and
 * copyPositive() ensures that q is left as it was (an audit). The
 * algorithms are defined in QueueAlgorithms.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// moves the positive items of q, in their order, to the back of out,
/// leaving q empty
/// \pre q and out are distinct
/// \post q is empty
template< Queue Q >
void extractPositive( Q & q , Q & out );

/// copies the positive items of q, in their order, to the back of out,
/// leaving q as it was
/// \pre q and out are distinct
/// \post q is as it was (an audit)
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
