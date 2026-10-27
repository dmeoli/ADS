/*--------------------------------------------------------------------------*/
/*---------------------------- File Selection.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The selection of section 17.1: the item of rank \f$ k \f$, the
 * \f$ k \f$-th smallest, \f$ 1 \le k \le n \f$, the median for
 * \f$ k = \lceil n / 2 \rceil \f$, without sorting.
 *
 * - `selection()`, of Hoare: the perno() of the book (`partition()`) puts
 *   its pivot in its place \f$ j \f$, the \f$ q = j - \mathit{start} + 1 \f$
 *   smallest items before it, and the search goes on in the part that holds
 *   rank \f$ k \f$; with the pivot chosen at random, swapped to
 *   \f$ A[ \mathit{start} ] \f$ before the partition, the expected number of
 *   comparisons satisfies
 *   \f[
 *     T( n ) \le n + \frac{2}{n} \sum_{q = \lfloor n / 2 \rfloor}^{n-1} T( q )
 *     \le c n ,
 *   \f]
 *   linear for every input;
 * - `medianOfMedians()`, of Blum, Floyd, Pratt, Rivest and Tarjan, which
 *   section 17.1 names without giving it, linear in the worst case: the
 *   pivot is the median of the medians of the groups of 5, which has at
 *   least \f$ 3 n / 10 \f$ items on each side, so that
 *   \f$ T( n ) \le T( n / 5 ) + T( 7 n / 10 ) + O( n ) = O( n ) \f$.
 *
 * Both rearrange \f$ A \f$.
 *
 * The contracts [see `Contract.h`]: \f$ 1 \le k \le n \f$, and the item
 * returned has rank \f$ k \f$, i.e., fewer than \f$ k \f$ items are smaller
 * and at least \f$ k \f$ are not larger, an audit in \f$ O( n ) \f$. The
 * functions are defined in `Selection.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see C. A. R. Hoare, Algorithm 65: Find, Communications of the ACM 4(7),
 *      321-322, 1961, doi:10.1145/366622.366647
 * \see M. Blum, R. W. Floyd, V. Pratt, R. L. Rivest, R. E. Tarjan, Time
 *      bounds for selection, Journal of Computer and System Sciences 7(4),
 *      448-461, 1973, doi:10.1016/S0022-0000(73)80033-9
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

#ifndef __ADS_Selection
 #define __ADS_Selection
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/QuickSort.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the item of rank \f$ k \f$, \f$ 1 \le k \le n \f$, of \f$ A \f$ (the
/// selezione() of the book); with a generator the pivots are random,
/// otherwise the first item of the part
///
/// In pseudocode (section 17.1), the tail recursion of the book as a loop:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{selection}( A , k ) \cr
/// 2 & \quad \mathit{start} \gets 0;\ \mathit{end} \gets n - 1 \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathit{start} = \mathit{end}\ \textbf{then
///     return}\ A[ \mathit{start} ] \cr
/// 5 & \qquad \text{swap } A[ \mathit{start} ] \text{ with an item of }
///     A[ \mathit{start} .. \mathit{end} ] \text{ at random} \cr
/// 6 & \qquad j \gets \mathrm{partition}( A , \mathit{start} , \mathit{end}
///     );\ q \gets j - \mathit{start} + 1 \cr
/// 7 & \qquad \textbf{if}\ k = q\ \textbf{then return}\ A[ j ] \cr
/// 8 & \qquad \textbf{else if}\ k < q\ \textbf{then}\ \mathit{end} \gets j
///     - 1 \cr
/// 9 & \qquad \textbf{else}\ \mathit{start} \gets j + 1;\ k \gets k - q
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le k \le n \f$
/// \post the result has rank \f$ k \f$ in \f$ A \f$ (audit)
template< class T , class R = std::mt19937 >
T selection( std::vector< T > & A , std::size_t k , R * rng = nullptr );

/// the item of rank \f$ k \f$, \f$ 1 \le k \le n \f$, in linear time in the
/// worst case
///
/// In pseudocode (Blum, Floyd, Pratt, Rivest and Tarjan, beyond the book),
/// with a partition in three ways:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{select}( A , \mathit{lo} , \mathit{hi} , r
///     ) \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} + 1 \le 10\ \textbf{then}
///     \cr
/// 3 & \qquad \text{sort } A[ \mathit{lo} .. \mathit{hi} ] \text{ by }
///     \text{insertion};\ \textbf{return}\ A[ \mathit{lo} + r - 1 ] \cr
/// 4 & \quad \text{sort each group of 5 and move its median to the front, }
///     \text{to } A[ \mathit{lo} .. m - 1 ] \cr
/// 5 & \quad x \gets \mathrm{select}( A , \mathit{lo} , m - 1 , \lceil ( m -
///     \mathit{lo} ) / 2 \rceil ) \cr
/// 6 & \quad \text{split } A[ \mathit{lo} .. \mathit{hi} ] \text{ into } < x
///     \text{, } = x \text{ and } > x \text{, of sizes } a \text{, } b
///     \text{ and } c \cr
/// 7 & \quad \textbf{if}\ r \le a\ \textbf{then return}\ \mathrm{select}(
///     \text{the part } < x , r ) \cr
/// 8 & \quad \textbf{else if}\ r \le a + b\ \textbf{then return}\ x \cr
/// 9 & \quad \textbf{else return}\ \mathrm{select}( \text{the part } > x , r
///     - a - b ) \cr
/// 10 & \textbf{function}\ \mathrm{medianOfMedians}( A , k ) \cr
/// 11 & \quad \textbf{return}\ \mathrm{select}( A , 0 , n - 1 , k )
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le k \le n \f$
/// \post the result has rank \f$ k \f$ in \f$ A \f$ (audit)
template< class T >
T medianOfMedians( std::vector< T > & A , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/Selection.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Selection.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Selection.h --------------------------*/
/*--------------------------------------------------------------------------*/
