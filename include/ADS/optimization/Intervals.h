/*--------------------------------------------------------------------------*/
/*---------------------------- File Intervals.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Greedy algorithms on intervals [ a_i , b_i ) of the line:
 *
 * - independentSet() (section 14.2), a largest set of disjoint intervals:
 *   by increasing right end, an interval that does not meet the last one
 *   taken is taken; optimal, since an optimal set can always swap its
 *   first interval for the one that ends first; O( n log n );
 * - intervalColoring() (exercise 14.6), the fewest colors such that the
 *   intervals that meet have different colors: by scanning the 2 n ends,
 *   a left end takes a free color from a stack and a right one gives it
 *   back; the colors are as many as the largest number of intervals over
 *   a point (exercise 9.8), which is a lower bound, O( n log n );
 * - towers() (exercise 14.7), the fewest points such that each of the
 *   given ones is within d of one of them: a tower at k_0 + d serves all
 *   the houses up to k_0 + 2 d, and so on from the first one left out;
 *   O( n log n ) for the sort.
 *
 * The contracts [see Contract.h]: each interval [ a , b ) has a < b, and d >=
 * 0; the independent set is made of disjoint intervals, the coloring gives
 * different colors to intersecting ones, and every house is within d of a
 * tower (checked at level 3). The operations are defined in Intervals.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.2 and
 *      exercises 14.6 and 14.7,
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

#ifndef __ADS_Intervals
 #define __ADS_Intervals
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
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

/// independentSet(): the places of a largest set of disjoint intervals
/// [ a , b ), in increasing order
/// \pre a < b for each interval
/// \post the intervals chosen are disjoint
inline std::vector< std::size_t > independentSet(
                           const std::vector< std::pair< long , long > > & I );

/// the colors, from 0, of the intervals [ a , b ), the fewest possible
/// \pre a < b for each interval
/// \post intersecting intervals have different colors
inline std::vector< int > intervalColoring(
                           const std::vector< std::pair< long , long > > & I );

/// torri(): the fewest towers such that each house is within d of one
/// \pre d >= 0
/// \post each house within d of a tower
inline std::vector< double > towers( std::vector< double > k , double d );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/Intervals.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Intervals.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Intervals.h --------------------------*/
/*--------------------------------------------------------------------------*/
