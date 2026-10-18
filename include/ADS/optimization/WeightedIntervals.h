/*--------------------------------------------------------------------------*/
/*------------------------ File WeightedIntervals.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The independent set of weighted intervals of section 13.2: among n
 * intervals [ a_i , b_i ), open on the right, of weights w_i, a set of
 * disjoint ones of largest weight. With the intervals by increasing right
 * end and pred[ i ] the largest j < i with b_{j-1} <= a_{i-1} (0 if none),
 * \f[
 *   DP[ i ] = \max\{ DP[ i - 1 ] , w_{i-1} + DP[ pred[ i ] ] \} ,
 * \f]
 * DP[ 0 ] = 0, and the set is read back from DP[ n ]; the predecessors come
 * from one scan of the 2 n ends sorted, so that maxinterval() takes
 * O( n log n ) time for the sort and O( n ) for the rest.
 *
 * The contracts [see Contract.h]: each interval has a < b and a weight not
 * negative; the intervals chosen are disjoint and weigh the value found
 * (checked at level 3). The operations are defined in WeightedIntervals.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_WeightedIntervals
 #define __ADS_WeightedIntervals
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
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

/// an interval [ a , b ) of weight w
template< class W >
struct WeightedInterval {
 long long a , b;
 W w;
 };

/// maxinterval(): the places of the intervals of a largest independent
/// set, in increasing order, and its weight
/// \pre a < b and w >= 0 for each interval
/// \post disjoint intervals of the weight found
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
