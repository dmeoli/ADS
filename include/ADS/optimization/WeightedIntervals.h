/*--------------------------------------------------------------------------*/
/*------------------------ File WeightedIntervals.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The independent set of weighted intervals of section 13.2: among
 * \f$ n \f$ intervals \f$ [ a_i , b_i ) \f$, open on the right, of weights
 * \f$ w_i \f$, a set of disjoint ones of largest weight. With the intervals
 * by increasing right end and \f$ \mathit{pred}[ i ] \f$ the largest
 * \f$ j < i \f$ with \f$ b_{j-1} \le a_{i-1} \f$ (0 if none),
 * \f[
 *   \mathit{DP}[ i ] = \max\{ \mathit{DP}[ i - 1 ] ,
 *                      w_{i-1} + \mathit{DP}[ \mathit{pred}[ i ] ] \} ,
 * \f]
 * \f$ \mathit{DP}[ 0 ] = 0 \f$, and the set is read back from
 * \f$ \mathit{DP}[ n ] \f$; the predecessors come from one scan of the
 * \f$ 2 n \f$ ends sorted, so that `maxinterval()` takes
 * \f$ O( n \log n ) \f$ time for the sort and \f$ O( n ) \f$ for the rest.
 *
 * The contracts [see `Contract.h`]: each interval has \f$ a < b \f$ and a
 * weight not negative; the intervals chosen are disjoint and weigh the value
 * found (checked at level 3). The operations are defined in
 * `WeightedIntervals.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_WeightedIntervals
 #define __ADS_WeightedIntervals
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an interval \f$ [ a , b ) \f$ of weight \f$ w \f$
template< class W >
struct WeightedInterval {
 long long a ,   ///< the left end, included
           b;   ///< the right end, excluded
 W w;           ///< the weight
 };

/// the places of the intervals of a largest independent set, in increasing
/// order, and its weight
///
/// In pseudocode (section 13.2), the intervals numbered from 1 by increasing
/// right end:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxinterval}( I ) \cr
/// 2 & \quad \text{sort } I \text{ by right end} \cr
/// 3 & \quad \textbf{foreach}\ \text{end by position, a right end before a }
///     \text{left one}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{it is the left end of } i\ \textbf{then}
///     \ \mathit{pred}[ i ] \gets h \cr
/// 5 & \qquad \textbf{else}\ h \gets \max\{ h , \text{its interval} \} \cr
/// 6 & \quad \mathit{DP}[ 0 ] \gets 0 \cr
/// 7 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do}
///     \ \mathit{DP}[ i ] \gets \max\{ \mathit{DP}[ i - 1 ] , w_i +
///     \mathit{DP}[
///     \mathit{pred}[ i ] ] \} \cr
/// 8 & \quad S \gets \emptyset;\ i \gets n \cr
/// 9 & \quad \textbf{while}\ i > 0\ \textbf{do} \cr
/// 10 & \qquad \textbf{if}\ \mathit{DP}[ i - 1 ] \ge w_i + \mathit{DP}[
///     \mathit{pred}[ i ] ]\ \textbf{then}\ i \gets i - 1 \cr
/// 11 & \qquad \textbf{else}\ S \gets S \cup \{ i \};\ i \gets \mathit{pred}[
///     i ] \cr
/// 12 & \quad \textbf{return}\ ( S , \mathit{DP}[ n ] )
/// \end{array}}
/// \f]
/// \pre \f$ a < b \f$ and \f$ w \ge 0 \f$ for each interval
/// \post disjoint intervals of the weight found (audit)
template< class W >
std::pair< std::vector< std::size_t > , W > maxinterval(
                              const std::vector< WeightedInterval< W > > & I );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/WeightedIntervals.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WeightedIntervals.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File WeightedIntervals.h ----------------------*/
/*--------------------------------------------------------------------------*/
