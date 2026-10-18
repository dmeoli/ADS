/*--------------------------------------------------------------------------*/
/*--------------------------- File CoinChange.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The change with the fewest coins (introduction of chapter 14): the
 * greedy choice takes the largest coin that fits, and is optimal for
 * coins of 50, 10, 5 and 1 cents, but not for coins of 11, 5 and 1, where
 * it pays 15 with five coins instead of three of 5. optimalChange() is the
 * dynamic programming, the fewest coins N[ c ] of each amount c,
 * \f$ N[ c ] = 1 + \min_{d \leq c} N[ c - d ] \f$, O( n C ).
 *
 * The contracts [see Contract.h]: the coins are positive and the amount is not
 * negative; the change of greedyChange() sums to the amount, and
 * optimalChange() uses at most c coins. The operations are defined in
 * CoinChange.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
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

#ifndef __ADS_CoinChange
 #define __ADS_CoinChange
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the coins taken by the greedy choice, the coins in any order; nothing
/// if it cannot pay
/// \pre c >= 0 and the coins positive
/// \post the change sums to c
inline std::optional< std::vector< long > > greedyChange(
                                       std::vector< long > coins , long c );

/// the fewest coins that pay c, nothing if none
/// \pre c >= 0 and the coins positive
inline std::optional< long > optimalChange( const std::vector< long > & coins ,
                                            long c );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/CoinChange.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CoinChange.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File CoinChange.h --------------------------*/
/*--------------------------------------------------------------------------*/
