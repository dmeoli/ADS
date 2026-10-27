/*--------------------------------------------------------------------------*/
/*---------------------------- File Intervals.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Greedy algorithms on intervals \f$ [ a_i , b_i ) \f$ of the line:
 *
 * - `independentSet()` (section 14.2), a largest set of disjoint intervals:
 *   by increasing right end, an interval that does not meet the last one
 *   taken is taken; optimal, since an optimal set can always swap its first
 *   interval for the one that ends first; \f$ O( n \log n ) \f$;
 * - `intervalColoring()` (exercise 14.6), the fewest colors such that the
 *   intervals that meet have different colors: by scanning the \f$ 2 n \f$
 *   ends, a left end takes a free color from a stack and a right one gives it
 *   back; the colors are as many as the largest number of intervals over a
 *   point (exercise 9.8), which is a lower bound, \f$ O( n \log n ) \f$;
 * - `towers()` (exercise 14.7), the fewest points such that each of the
 *   given ones is within \f$ d \f$ of one of them: a tower at
 *   \f$ k_0 + d \f$ serves all the houses up to \f$ k_0 + 2 d \f$, and so on
 *   from the first one left out; \f$ O( n \log n ) \f$ for the sort.
 *
 * The contracts [see `Contract.h`]: each interval \f$ [ a , b ) \f$ has
 * \f$ a < b \f$, and \f$ d \ge 0 \f$; the independent set is made of disjoint
 * intervals, the coloring gives different colors to intersecting ones, and
 * every house is within \f$ d \f$ of a tower (checked at level 3). The
 * operations are defined in `Intervals.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.2 and
 *      exercises 14.6 and 14.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
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

#ifndef __ADS_Intervals
 #define __ADS_Intervals
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the places of a largest set of disjoint intervals \f$ [ a , b ) \f$, in
/// increasing order
///
/// In pseudocode (section 14.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{independentSet}( I ) \cr
/// 2 & \quad S \gets \emptyset;\ e \gets -\infty\quad \triangleright
///     \ \text{the right end of the last one taken} \cr
/// 3 & \quad \textbf{foreach}\ [ a , b ) \in I \text{ by increasing } b
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ a \ge e\ \textbf{then}\ S \gets S \cup \{ [ a , b
///     ) \};\ e \gets b \cr
/// 5 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ a < b \f$ for each interval
/// \post the intervals chosen are disjoint (audit)
inline std::vector< std::size_t > independentSet(
                           const std::vector< std::pair< long , long > > & I );

/// the colors, from 0, of the intervals \f$ [ a , b ) \f$, the fewest
/// possible
///
/// In pseudocode (exercise 14.6), the free colors on a stack \f$ F \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{intervalColoring}( I ) \cr
/// 2 & \quad F \gets \text{an empty stack};\ u \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ \text{end of } I \text{ by position, a right }
///     \text{end before a left one}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{it is the left end of } i\ \textbf{then} \cr
/// 5 & \qquad\quad \textbf{if}\ F = \emptyset\ \textbf{then}\ \mathrm{push}(
///     F , u );\ u \gets u + 1 \cr
/// 6 & \qquad\quad \mathit{color}[ i ] \gets \mathrm{pop}( F ) \cr
/// 7 & \qquad \textbf{else}\ \mathrm{push}( F , \mathit{color}[ i ] ) \cr
/// 8 & \quad \textbf{return}\ \mathit{color}
/// \end{array}}
/// \f]
/// \pre \f$ a < b \f$ for each interval
/// \post intersecting intervals have different colors (audit)
inline std::vector< int > intervalColoring(
                           const std::vector< std::pair< long , long > > & I );

/// the fewest towers such that each house is within \f$ d \f$ of one (the
/// `torri()` of the book)
///
/// In pseudocode (exercise 14.7):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{towers}( k , d ) \cr
/// 2 & \quad S \gets \text{the empty sequence} \cr
/// 3 & \quad \textbf{foreach}\ x \in k \text{ by increasing value}
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ S \text{ is empty}\ \textbf{or}\ x > \text{the }
///     \text{last of } S + d\ \textbf{then}\ S \gets S \cdot ( x + d ) \cr
/// 5 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ d \ge 0 \f$
/// \post each house within \f$ d \f$ of a tower (audit)
inline std::vector< double > towers( std::vector< double > k , double d );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/Intervals.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Intervals.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Intervals.h --------------------------*/
/*--------------------------------------------------------------------------*/
