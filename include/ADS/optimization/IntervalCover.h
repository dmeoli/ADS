/*--------------------------------------------------------------------------*/
/*-------------------------- File IntervalCover.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cover of a segment by intervals: among n closed intervals
 * [ a_i , b_i ], the fewest whose union contains the segment [ lo , hi ],
 * e.g. the sprinklers at D_i of radius R_i, of intervals
 * [ D_i - R_i , D_i + R_i ], that water a lawn [ 0 , L ].
 *
 * The greedy choice of chapter 14: with the part [ lo , x ] already
 * covered, among the intervals that start at x or before take the one that
 * ends farthest; if none goes past x the point just after x stays
 * uncovered and there is no cover. An exchange argument shows that some
 * optimal cover begins with the greedy interval: the interval of an
 * optimal cover that contains x ends no farther. On the intervals sorted
 * by left end, one scan suffices, O( n log n ) time for the sort.
 *
 * The problem is the dual of the independent set of intervals [see
 * Intervals.h], and the one-dimensional case of the set cover, which in
 * general is NP-hard and approximated within \f$ \ln n \f$ by the greedy
 * on the most new elements (Chvatal).
 *
 * The contracts [see Contract.h]: lo < hi and a_i <= b_i; the intervals
 * chosen cover [ lo , hi ] (checked at level 3). The functions are defined
 * in IntervalCover.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see J. Kleinberg, E. Tardos, Algorithm Design, Addison-Wesley, 2005,
 *      section 4.1
 * \see V. Chvatal, A greedy heuristic for the set-covering problem,
 *      Mathematics of Operations Research 4(3), 233-235, 1979,
 *      doi:10.1287/moor.4.3.233
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

/// the places of the fewest closed intervals I whose union contains
/// [ lo , hi ], by increasing left end; nothing if there is none
/// \pre lo < hi and first <= second for each interval
/// \post the intervals cover [ lo , hi ]
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
