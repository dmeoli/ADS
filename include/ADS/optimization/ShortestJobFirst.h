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
 * With the weights w_i of the programs (the probabilities of the files on
 * a tape, read at the times of completion), the order of least weighted
 * sum \f$ \sum_i w_i C_i \f$ of the times of completion C_i is Smith's
 * rule, by increasing \f$ t_i / w_i \f$, compared as
 * \f$ t_a w_b < t_b w_a \f$ (no division, and a program of weight 0 goes
 * last): the exchange of two adjacent programs a , b changes the sum by
 * \f$ t_a w_b - t_b w_a \f$; O( n log n ). With precedence constraints the
 * problem is NP-hard in general, and solved by Lawler's merging of chains
 * on series-parallel orders.
 *
 * The contracts [see Contract.h]: the times are not negative, the order is by
 * increasing time, and the order given to totalCompletion() has places of t;
 * the weights are not negative and no program has both time and weight 0,
 * and the order of smithOrder() is by increasing t / w. The operations are
 * defined in ShortestJobFirst.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 14.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see W. E. Smith, Various optimizers for single-stage production, Naval
 *      Research Logistics Quarterly 3(1-2), 59-66, 1956,
 *      doi:10.1002/nav.3800030106
 * \see E. L. Lawler, Sequencing jobs to minimize total weighted completion
 *      time subject to precedence constraints, Annals of Discrete
 *      Mathematics 2, 75-90, 1978, doi:10.1016/S0167-5060(08)70323-6
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

/// the order of the programs of times t and weights w of least weighted
/// sum of the times of completion, by increasing t / w (Smith's rule)
/// \pre |t| = |w|, t and w not negative, t_i + w_i > 0
/// \post the places by increasing t / w
template< class T , class W >
std::vector< std::size_t > smithOrder( const std::vector< T > & t ,
                                       const std::vector< W > & w );

/// the sum of w_i C_i in the order A, C_i the time of completion of i
/// \pre |t| = |w| and the places of A are places of t
template< class T , class W >
auto weightedCompletion( const std::vector< T > & t ,
                         const std::vector< W > & w ,
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
