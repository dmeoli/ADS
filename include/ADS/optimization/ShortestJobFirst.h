/*--------------------------------------------------------------------------*/
/*------------------------ File ShortestJobFirst.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shortest Job First (exercise 14.5): n programs of durations t_i, run one
 * after the other, in the order that minimizes the average time of
 * completion: by increasing duration, since moving the shortest program to
 * the front of any order does not increase any time of completion;
 * O( n log n ).
 *
 * The contracts [see Contract.h]: the times are not negative, the order is by
 * increasing time, and the order given to totalCompletion() has places of t.
 * The operations are defined in ShortestJobFirst.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 14.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#ifndef __ADS_ShortestJobFirst
 #define __ADS_ShortestJobFirst
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the order of the programs, by increasing duration
/// \pre no time negative
/// \post the places by increasing time
template< class T >
std::vector< std::size_t > shortestJobFirst( const std::vector< T > & t );

/// the sum of the times of completion of the programs in the order A
/// \pre the places of A are places of t
template< class T >
T totalCompletion( const std::vector< T > & t ,
                   const std::vector< std::size_t > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/ShortestJobFirst.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ShortestJobFirst.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ShortestJobFirst.h -----------------------*/
/*--------------------------------------------------------------------------*/
