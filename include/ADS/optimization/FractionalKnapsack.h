/*--------------------------------------------------------------------------*/
/*----------------------- File FractionalKnapsack.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The real (fractional) knapsack of section 14.1: a fraction x_i in [ 0 , 1 ]
 * of each object, of largest profit \f$ \sum_i p_i x_i \f$ and volume
 * \f$ \sum_i v_i x_i \leq C \f$. The greedy choice by decreasing specific
 * profit \f$ p_i / v_i \f$ is optimal: the most profitable objects per unit of
 * volume go in whole, the next one in part, the others not;
 * \f$ O( n \log n ) \f$ time for the sort. By decreasing profit or by
 * increasing volume it is not, and neither is any of them for the 0-1 knapsack
 * (exercise 14.1).
 *
 * The contracts [see Contract.h]: profits and volumes of the same objects, the
 * volumes positive, the profits and the capacity not negative; the fractions
 * are in [ 0 , 1 ] and fill at most the capacity. The operations are defined
 * in FractionalKnapsack.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see G. B. Dantzig, Discrete-variable extremum problems, Operations
 *      Research 5(2), 266-288, 1957, doi:10.1287/opre.5.2.266
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

#ifndef __ADS_FractionalKnapsack
 #define __ADS_FractionalKnapsack
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

/// zaino(): the fractions x of the objects, by decreasing p / v
/// \pre |p| = |v|, v > 0, p >= 0, C >= 0
/// \post fractions in [ 0 , 1 ] of volume at most C
inline std::vector< double > fractionalKnapsack(
                                            const std::vector< double > & p ,
                                            const std::vector< double > & v ,
                                            double C );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/FractionalKnapsack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FractionalKnapsack.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File FractionalKnapsack.h ----------------------*/
/*--------------------------------------------------------------------------*/
