/*--------------------------------------------------------------------------*/
/*--------------------------- File CoinChange.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The change with the fewest coins (introduction of chapter 14): the
 * greedy choice takes the largest coin that fits, and is optimal for coins
 * of 50, 10, 5 and 1 cents, but not for coins of 11, 5 and 1, where it pays
 * 15 with five coins instead of three of 5. `optimalChange()` is the dynamic
 * programming, the fewest coins \f$ N[ c ] \f$ of each amount \f$ c \f$,
 * \f$ N[ c ] = 1 + \min_{d \le c} N[ c - d ] \f$, \f$ O( n C ) \f$ for
 * \f$ n \f$ coins and the amount \f$ C \f$.
 *
 * The contracts [see `Contract.h`]: the coins are positive and the amount is
 * not negative; the change of `greedyChange()` sums to the amount, and
 * `optimalChange()` uses at most \f$ c \f$ coins. The operations are defined
 * in `CoinChange.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 14,
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

#ifndef __ADS_CoinChange
 #define __ADS_CoinChange
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

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

/// the coins taken by the greedy choice, the coins given in any order;
/// nothing if it cannot pay
///
/// In pseudocode (the example of the introduction of chapter 14), the
/// largest coin first, as many times as it
/// fits:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{greedyChange}( D , c ) \cr
/// 2 & \quad S \gets \text{the empty sequence} \cr
/// 3 & \quad \textbf{foreach}\ d \in D \text{ by decreasing value}
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{while}\ c \ge d\ \textbf{do}\ S \gets S \cdot ( d );\ c
///     \gets c - d \cr
/// 5 & \quad \textbf{if}\ c \ne 0\ \textbf{then return nil} \cr
/// 6 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ c \ge 0 \f$ and the coins positive
/// \post the change sums to \f$ c \f$
inline std::optional< std::vector< long > > greedyChange(
                                       std::vector< long > coins , long c );

/// the fewest coins that pay \f$ c \f$, nothing if none
///
/// In pseudocode (beyond the book, the example of chapter 14 solved by the
/// dynamic programming of chapter 13), the amounts by increasing value:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{optimalChange}( D , c ) \cr
/// 2 & \quad N[ 0 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ x \gets 1\ \textbf{to}\ c\ \textbf{do} \cr
/// 4 & \qquad N[ x ] \gets \infty \cr
/// 5 & \qquad \textbf{foreach}\ d \in D\ \textbf{with}\ d \le x\ \textbf{do}
///     \ N[ x ] \gets \min\{ N[ x ] , N[ x - d ] + 1 \} \cr
/// 6 & \quad \textbf{if}\ N[ c ] = \infty\ \textbf{then return nil} \cr
/// 7 & \quad \textbf{return}\ N[ c ]
/// \end{array}}
/// \f]
/// \pre \f$ c \ge 0 \f$, the \f$ c + 1 \f$ amounts within the largest
///      vector, and the coins positive
/// \post at most \f$ c \f$ coins
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
