/*--------------------------------------------------------------------------*/
/*------------------------ File ShortestJobFirst.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Shortest Job First (exercise 14.5): \f$ n \f$ programs of durations
 * \f$ t_i \f$, run one after the other, in the order that minimizes the
 * average time of completion: by increasing duration, since moving the
 * shortest program to the front of any order does not increase any time of
 * completion; \f$ O( n \log n ) \f$.
 *
 * With the weights \f$ w_i \f$ of the programs (the probabilities of the files
 * on a tape, read at the times of completion), the order of least weighted
 * sum \f$ \sum_i w_i C_i \f$ of the times of completion \f$ C_i \f$ is
 * Smith's rule, by increasing \f$ t_i / w_i \f$, compared as
 * \f$ t_a w_b < t_b w_a \f$ (no division, and a program of weight 0 goes
 * last): the exchange of two adjacent programs \f$ a \f$, \f$ b \f$ changes
 * the sum by \f$ t_a w_b - t_b w_a \f$; \f$ O( n \log n ) \f$. With
 * precedence constraints the problem is NP-hard in general, and solved by
 * Lawler's merging of chains on series-parallel orders.
 *
 * The contracts [see `Contract.h`]: the times are not negative, the order is
 * by increasing time, and the order given to `totalCompletion()` has places
 * of \f$ t \f$; the weights are not negative and no program has both time and
 * weight 0, and the order of `smithOrder()` is by increasing \f$ t / w \f$.
 * The operations are defined in `ShortestJobFirst.tpp`.
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
 *         Università di Pisa \n
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
///
/// In pseudocode (exercise 14.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{shortestJobFirst}( t ) \cr
/// 2 & \quad \textbf{return}\ \text{the places } 0 , \ldots , n - 1 \text{ }
///     \text{sorted by increasing } t_i \text{, stably}
/// \end{array}}
/// \f]
/// \pre no time negative
/// \post the places by increasing time
template< class T >
std::vector< std::size_t > shortestJobFirst( const std::vector< T > & t );

/// the sum of the times of completion of the programs in the order \f$ A \f$,
/// \f$ \sum_k ( t_{A[ 0 ]} + \cdots + t_{A[ k ]} ) \f$
/// \pre the places of \f$ A \f$ are places of \f$ t \f$
template< class T >
T totalCompletion( const std::vector< T > & t ,
                   const std::vector< std::size_t > & A );

/// the order of the programs of times \f$ t \f$ and weights \f$ w \f$ of least
/// weighted sum of the times of completion, by increasing \f$ t / w \f$
/// (Smith's rule)
///
/// In pseudocode (Smith's rule, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{smithOrder}( t , w ) \cr
/// 2 & \quad \textbf{return}\ \text{the places sorted stably, } a \text{ }
///     \text{before } b \text{ when } t_a w_b < t_b w_a
/// \end{array}}
/// \f]
/// \pre \f$ |t| = |w| \f$, \f$ t \f$ and \f$ w \f$ not negative,
///      \f$ t_i + w_i > 0 \f$
/// \post the places by increasing \f$ t / w \f$
template< class T , class W >
std::vector< std::size_t > smithOrder( const std::vector< T > & t ,
                                       const std::vector< W > & w );

/// the sum of \f$ w_i C_i \f$ in the order \f$ A \f$, \f$ C_i \f$ the time of
/// completion of \f$ i \f$
/// \pre \f$ |t| = |w| \f$ and the places of \f$ A \f$ are places of \f$ t \f$
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
