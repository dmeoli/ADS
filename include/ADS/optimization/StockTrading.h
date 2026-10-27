/*--------------------------------------------------------------------------*/
/*-------------------------- File StockTrading.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The largest gain of buying and selling a stock with at most \f$ k \f$, or
 * with exactly \f$ k \f$, transactions, given its prices on \f$ n \f$ days: a
 * transaction buys on a day \f$ b \f$ and sells on a later day \f$ s \f$, and
 * the transactions do not overlap,
 * \f$ b_1 < s_1 < b_2 < s_2 < \cdots < b_k < s_k \f$.
 *
 * The dynamic programming scans the days keeping, for each
 * \f$ l \le k \f$, the best balance \f$ \mathit{free}[ l ] \f$ with \f$ l \f$
 * transactions closed and no stock, and the best balance
 * \f$ \mathit{hold}[ l ] \f$ with the stock of the \f$ l \f$-th transaction
 * in hand: on the day of price \f$ x \f$,
 * \f$ \mathit{free}[ l ] = \max( \mathit{free}[ l ] , \mathit{hold}[ l ] + x )
 * \f$ and \f$ \mathit{hold}[ l ] = \max( \mathit{hold}[ l ] ,
 * \mathit{free}[ l - 1 ] - x ) \f$, each from the values of the days before
 * (\f$ l \f$ going down), which keeps the days distinct; \f$ O( n k ) \f$ time
 * and \f$ O( k ) \f$ space. The text of the exercise writes the
 * \f$ O( k n^2 ) \f$ table on the day of the last sale.
 *
 * With at most \f$ k \f$ transactions the answer is not negative, and for
 * \f$ k \ge n / 2 \f$ it is the sum of the positive differences of
 * consecutive prices, \f$ O( n ) \f$; with exactly \f$ k \f$ the answer may
 * be negative, and there is none for \f$ n < 2 k \f$. The case \f$ k = 1 \f$
 * is the largest difference \f$ x_s - x_b \f$, \f$ s > b \f$, which is the
 * maximum sum segment of the differences [see `MaximumSegments.h`].
 *
 * The contracts [see `Contract.h`]: the gain of at most \f$ k \f$
 * transactions is not negative. The functions are defined in
 * `StockTrading.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13 and the
 *      Esercizi Programmazione dinamica, exercise B3 of 12/01/2026,
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

/// the largest gain of at most \f$ k \f$ transactions on the prices
///
/// In pseudocode (beyond the book, the scan of the exercise B3 of
/// 12/01/2026, Borsa, of its Esercizi Programmazione dinamica),
/// \f$ \mathit{free} \f$ and \f$ \mathit{hold} \f$
/// starting at \f$ \textbf{nil} \f$ but \f$ \mathit{free}[ 0 ] = 0 \f$, a
/// \f$ \textbf{nil} \f$ losing every maximum:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxProfit}( x[ 0 .. n - 1 ] , k ) \cr
/// 2 & \quad \textbf{if}\ k \ge n / 2\ \textbf{then return}\ \sum_{i \ge 1}
///     \max\{ x[ i ] - x[ i - 1 ] , 0 \} \cr
/// 3 & \quad \textbf{foreach}\ \text{price } y \text{ of } x\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ l \gets k\ \textbf{downto}\ 1\ \textbf{do} \cr
/// 5 & \qquad\quad \mathit{free}[ l ] \gets \max\{ \mathit{free}[ l ] ,
///     \mathit{hold}[ l ] + y \} \cr
/// 6 & \qquad\quad \mathit{hold}[ l ] \gets \max\{ \mathit{hold}[ l ] ,
///     \mathit{free}[ l - 1 ] - y \} \cr
/// 7 & \quad \textbf{return}\ \max_l \mathit{free}[ l ]
/// \end{array}}
/// \f]
/// \post the result is not negative
template< class T >
T maxProfit( const std::vector< T > & prices , std::size_t k );

/// the largest gain of exactly \f$ k \f$ transactions on the prices, nothing
/// if there are fewer than \f$ 2 k \f$ days
///
/// In pseudocode (exercise B3 of 12/01/2026, Borsa, of the Esercizi
/// Programmazione dinamica of the book), the scan of `maxProfit()` with no
/// shortcut:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxProfitExactly}( x[ 0 .. n - 1 ] , k ) \cr
/// 2 & \quad \textbf{if}\ n < 2 k\ \textbf{then return nil} \cr
/// 3 & \quad \text{run lines 3 to 6 of } \mathrm{maxProfit} \cr
/// 4 & \quad \textbf{return}\ \mathit{free}[ k ]
/// \end{array}}
/// \f]
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
