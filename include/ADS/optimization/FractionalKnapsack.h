/*--------------------------------------------------------------------------*/
/*----------------------- File FractionalKnapsack.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The real (fractional) knapsack of section 14.1: a fraction
 * \f$ x_i \in [ 0 , 1 ] \f$ of each object, of largest profit
 * \f$ \sum_i p_i x_i \f$ and volume \f$ \sum_i v_i x_i \le C \f$. The greedy
 * choice by decreasing specific profit \f$ p_i / v_i \f$ is optimal: the
 * most profitable objects per unit of volume go in whole, the next one in
 * part, the others not; \f$ O( n \log n ) \f$ time for the sort. By
 * decreasing profit or by increasing volume it is not, and neither is any
 * of them for the 0-1 knapsack (exercise 14.1).
 *
 * The contracts [see `Contract.h`]: profits and volumes of the same objects,
 * the volumes positive, the profits and the capacity not negative; the
 * fractions are in \f$ [ 0 , 1 ] \f$ and fill at most the capacity. The
 * operations are defined in `FractionalKnapsack.tpp`.
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

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the fractions \f$ x \f$ of the objects, by decreasing \f$ p_i / v_i \f$
/// (the `zaino()` of the book)
///
/// In pseudocode (section 14.1), the ratios compared as
/// \f$ p_a v_b > p_b v_a \f$, with no division:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fractionalKnapsack}( p , v , C ) \cr
/// 2 & \quad x[ 1 .. n ] \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ i \text{ by decreasing } p_i / v_i
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ C \le 0\ \textbf{then}\ \textbf{break} \cr
/// 5 & \qquad x[ i ] \gets \min\{ C / v_i , 1 \};\ C \gets C - x[ i ] v_i \cr
/// 6 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre \f$ |p| = |v| \f$, \f$ v_i > 0 \f$, \f$ p_i \ge 0 \f$, \f$ C \ge 0 \f$
/// \post fractions in \f$ [ 0 , 1 ] \f$ of volume at most \f$ C \f$
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
