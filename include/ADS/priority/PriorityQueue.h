/*--------------------------------------------------------------------------*/
/*-------------------------- File PriorityQueue.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE of section 10.1, with decreasing priority: insert( x ,
 * p ) gives a handle of x, min() and deleteMin() the item of smallest
 * priority, decrease( y , p ) lowers to p the priority of the item of the
 * handle y. Items of equal priority leave in no given order.
 *
 * HeapPriorityQueue realizes it on a d-ary heap, ListPriorityQueue on a
 * sorted or unsorted list, TreePriorityQueue on a Red-Black tree.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#ifndef __ADS_PriorityQueue
 #define __ADS_PriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPT ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// PRIORITYQUEUE (section 10.1), with decreasing priority
template< class Q >
concept PriorityQueue = requires( Q q , const Q cq ,
                                  const typename Q::Item & x ,
                                  const typename Q::Priority & p ,
                                  typename Q::Handle y ) {
 { cq.isEmpty() } -> std::same_as< bool >;
 { cq.min() } -> std::convertible_to< typename Q::Item >;
 { q.deleteMin() } -> std::convertible_to< typename Q::Item >;
 { q.insert( x , p ) } -> std::same_as< typename Q::Handle >;
 q.decrease( y , p );
 };

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* PriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
