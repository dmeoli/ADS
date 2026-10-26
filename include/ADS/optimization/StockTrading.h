/*--------------------------------------------------------------------------*/
/*-------------------------- File StockTrading.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The largest gain of buying and selling a stock with at most k, or with
 * exactly k, transactions, given its prices on n days: a transaction buys
 * on a day b and sells on a later day s, and the transactions do not
 * overlap, \f$ b_1 < s_1 < b_2 < s_2 < \cdots < b_k < s_k \f$.
 *
 * The dynamic programming scans the days keeping, for each l <= k, the
 * best balance free[ l ] with l transactions closed and no stock, and the
 * best balance hold[ l ] with the stock of the l-th transaction in hand:
 * on the day of price x,
 * \f$ free[ l ] = \max( free[ l ] , hold[ l ] + x ) \f$ and
 * \f$ hold[ l ] = \max( hold[ l ] , free[ l - 1 ] - x ) \f$, each from the
 * values of the days before (l going down), which keeps the days distinct;
 * O( n k ) time and O( k ) space. The text of the exercise writes the
 * O( k n^2 ) table on the day of the last sale.
 *
 * With at most k transactions the answer is not negative, and for
 * \f$ k \geq n / 2 \f$ it is the sum of the positive differences of
 * consecutive prices, O( n ); with exactly k the answer may be negative,
 * and there is none for n < 2 k. The case k = 1 is the largest
 * difference \f$ x_s - x_b \f$, s > b, which is the maximum sum segment of
 * the differences [see MaximumSegments.h].
 *
 * The contracts [see Contract.h]: the gain of at most k transactions is
 * not negative. The functions are defined in StockTrading.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see J. Bentley, Programming Pearls, 2nd ed., Addison-Wesley, 2000,
 *      column 8
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

#ifndef __ADS_StockTrading
 #define __ADS_StockTrading
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the largest gain of at most k transactions on the prices
/// \post not negative
template< class T >
T maxProfit( const std::vector< T > & prices , std::size_t k );

/// the largest gain of exactly k transactions on the prices, nothing if
/// there are fewer than 2 k days
template< class T >
std::optional< T > maxProfitExactly( const std::vector< T > & prices ,
                                     std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/StockTrading.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StockTrading.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File StockTrading.h -------------------------*/
/*--------------------------------------------------------------------------*/
