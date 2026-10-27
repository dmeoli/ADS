/*--------------------------------------------------------------------------*/
/*-------------------------- File Subsequences.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems on a sequence \f$ A[ 0 .. n - 1 ] \f$ solved by the dynamic
 * programming of chapter 13:
 *
 * - the longest increasing subsequence (exercise 13.6): \f$ L[ i ] \f$, the
 *   length of the longest one that ends in \f$ A[ i ] \f$, is 1 plus the
 *   largest \f$ L[ j ] \f$, \f$ j < i \f$, with \f$ A[ j ] < A[ i ] \f$,
 *   \f$ O( n^2 ) \f$ (`lis()`); `lisLength()` keeps, for each length
 *   \f$ \ell \f$, the least item that ends an increasing subsequence of
 *   length \f$ \ell \f$, an increasing vector, and puts each item in place by
 *   a binary search, \f$ O( n \log n ) \f$;
 * - the subvector of largest sum (exercises 12.2 and 13.12): \f$ t_i \f$,
 *   the best sum of a subvector ending in \f$ A[ i ] \f$, is
 *   \f$ \max\{ t_{i-1} + A[ i ] , 0 \} \f$, the empty subvector of sum 0
 *   allowed, \f$ O( n ) \f$ (the scan of Kadane, `maxSubarraySum()`).
 *
 * The contracts [see `Contract.h`]: `lis()` gives a strictly increasing
 * subsequence of \f$ A \f$, as long as `lisLength()` says, an audit;
 * `maxSubarraySum()` is never negative, the empty subvector counting 0. The
 * functions are defined in `Subsequences.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercises 13.6 and
 *      13.12, https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see M. L. Fredman, On computing the length of longest increasing
 *      subsequences, Discrete Mathematics 11(1), 29-35, 1975,
 *      doi:10.1016/0012-365X(75)90103-X
 * \see J. Bentley, Programming pearls: algorithm design techniques,
 *      Communications of the ACM 27(9), 865-873, 1984,
 *      doi:10.1145/358234.381162
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

#ifndef __ADS_Subsequences
 #define __ADS_Subsequences
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a longest increasing subsequence, \f$ O( n^2 ) \f$
///
/// In pseudocode (exercise 13.6), \f$ \mathit{prev}[ i ] \f$ the item before
/// \f$ A[ i ] \f$ in the best subsequence ending there:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lis}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad L[ i ] \gets 1;\ \mathit{prev}[ i ] \gets \textbf{nil} \cr
/// 4 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ i - 1\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ A[ j ] < A[ i ]\ \textbf{and}\ L[ j ] + 1 >
///     L[ i ]\ \textbf{then} \cr
/// 6 & \qquad\qquad L[ i ] \gets L[ j ] + 1;\ \mathit{prev}[ i ] \gets j \cr
/// 7 & \quad b \gets \text{the first place of the largest } L[ b ] \cr
/// 8 & \quad \textbf{return}\ \text{the items } A[ b ] ,
///     A[ \mathit{prev}[ b ] ] , \ldots \text{ in reverse order}
/// \end{array}}
/// \f]
/// \post a strictly increasing subsequence of \f$ A \f$ of the largest length
///       (audit)
template< class T >
std::vector< T > lis( const std::vector< T > & A );

/// the length of a longest increasing subsequence, \f$ O( n \log n ) \f$
///
/// In pseudocode (Fredman, beyond the book), \f$ \mathit{tails}[ \ell ] \f$
/// the least item that ends an increasing subsequence of \f$ \ell + 1 \f$
/// items, an increasing vector:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lisLength}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \mathit{tails} \gets \text{an empty vector} \cr
/// 3 & \quad \textbf{foreach}\ a \in A\ \textbf{do} \cr
/// 4 & \qquad \ell \gets \text{the first place with } \mathit{tails}[ \ell ]
///     \ge a \text{, by binary search} \cr
/// 5 & \qquad \textbf{if}\ \ell = |\mathit{tails}|\ \textbf{then}
///     \ \text{append } a \text{ to } \mathit{tails}\ \textbf{else}
///     \ \mathit{tails}[ \ell ] \gets a \cr
/// 6 & \quad \textbf{return}\ |\mathit{tails}|
/// \end{array}}
/// \f]
template< class T >
std::size_t lisLength( const std::vector< T > & A );

/// the largest sum of a subvector, the empty one included, \f$ O( n ) \f$
///
/// In pseudocode (exercise 13.12), \f$ t \f$ the best sum of a subvector
/// ending in the current item:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSubarraySum}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \mathit{best} \gets 0;\ t \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ a \in A\ \textbf{do} \cr
/// 4 & \qquad t \gets \max\{ t + a , 0 \};\ \mathit{best} \gets \max\{
///     \mathit{best} , t \} \cr
/// 5 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \post the result is not negative
template< class T >
T maxSubarraySum( const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/Subsequences.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Subsequences.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Subsequences.h -------------------------*/
/*--------------------------------------------------------------------------*/
