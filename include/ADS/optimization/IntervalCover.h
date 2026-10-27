/*--------------------------------------------------------------------------*/
/*-------------------------- File IntervalCover.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cover of a segment by intervals: among \f$ n \f$ closed intervals
 * \f$ [ a_i , b_i ] \f$, the fewest whose union contains the segment
 * \f$ [ \mathit{lo} , \mathit{hi} ] \f$, e.g., the sprinklers at \f$ D_i \f$
 * of radius \f$ R_i \f$, of intervals \f$ [ D_i - R_i , D_i + R_i ] \f$, that
 * water a lawn \f$ [ 0 , L ] \f$.
 *
 * The greedy choice of chapter 14: with the part \f$ [ \mathit{lo} , x ] \f$
 * already covered, among the intervals that start at \f$ x \f$ or before take
 * the one that ends farthest; if none goes past \f$ x \f$ the point just after
 * \f$ x \f$ stays uncovered and there is no cover. An exchange argument shows
 * that some optimal cover begins with the greedy interval: the interval of an
 * optimal cover that contains \f$ x \f$ ends no farther. On the intervals
 * sorted by left end, one scan suffices, \f$ O( n \log n ) \f$ time for the
 * sort. The problem is the dual of the independent set of intervals [see
 * `Intervals.h`], and the one-dimensional case of the set cover, which in
 * general is NP-hard and approximated within \f$ \ln n \f$ by the greedy on
 * the most new elements (Chvátal).
 *
 * The contracts [see `Contract.h`]: \f$ \mathit{lo} < \mathit{hi} \f$ and
 * \f$ a_i \le b_i \f$; the intervals chosen cover
 * \f$ [ \mathit{lo} , \mathit{hi} ] \f$ (checked at level 3). The functions
 * are defined in `IntervalCover.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see J. Kleinberg, E. Tardos, Algorithm Design, Addison-Wesley, 2005,
 *      section 4.1
 * \see V. Chvátal, A greedy heuristic for the set-covering problem,
 *      Mathematics of Operations Research 4(3), 233-235, 1979,
 *      doi:10.1287/moor.4.3.233
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

#ifndef __ADS_IntervalCover
 #define __ADS_IntervalCover
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the places of the fewest closed intervals \f$ I \f$ whose union contains
/// \f$ [ \mathit{lo} , \mathit{hi} ] \f$, by increasing left end; nothing if
/// there is none
///
/// In pseudocode (a greedy algorithm of chapter 14, beyond the book),
/// \f$ x \f$ the end of the part covered so far:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minimumIntervalCover}( I , \mathit{lo} ,
///     \mathit{hi} ) \cr
/// 2 & \quad \text{sort } I \text{ by left end};\ x \gets \mathit{lo};\ S
///     \gets \text{the empty sequence} \cr
/// 3 & \quad \textbf{while}\ x < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad [ a , b ] \gets \text{the interval with } a \le x \text{ that }
///     \text{ends farthest, not taken before} \cr
/// 5 & \qquad \textbf{if}\ \text{there is none}\ \textbf{or}\ b \le x
///     \ \textbf{then return nil} \cr
/// 6 & \qquad x \gets b;\ S \gets S \cdot ( [ a , b ] ) \cr
/// 7 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{lo} < \mathit{hi} \f$, and \f$ a \le b \f$ for each
///      interval \f$ [ a , b ] \f$
/// \post the intervals cover \f$ [ \mathit{lo} , \mathit{hi} ] \f$ (audit)
template< class T >
std::optional< std::vector< std::size_t > > minimumIntervalCover(
                  const std::vector< std::pair< T , T > > & I , T lo , T hi );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/IntervalCover.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IntervalCover.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File IntervalCover.h ------------------------*/
/*--------------------------------------------------------------------------*/
