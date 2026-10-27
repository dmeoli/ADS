/*--------------------------------------------------------------------------*/
/*------------------------- File LinearPartition.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The linear partition problem (also the painter's partition, or the
 * bottleneck partition of a chain): the items \f$ A[ 0 .. n - 1 ] \f$ split
 * into \f$ k \f$ contiguous parts so that the largest sum of a part is the
 * least, e.g., \f$ n \f$ boards of lengths \f$ A_i \f$ to be painted by
 * \f$ k \f$ painters, each one a block of consecutive boards, in the least
 * time.
 *
 * - `linearPartition()` solves it for items of any sign and exactly \f$ k \f$
 *   nonempty parts by the dynamic programming on the prefix sums \f$ S \f$,
 *   \f$ D[ t ][ i ] = \min_{t - 1 \le j < i} \max( D[ t - 1 ][ j ] , S[ i ] -
 *   S[ j ] ) \f$, \f$ D[ 1 ][ i ] = S[ i ] \f$, with the best cut kept for
 *   each entry; \f$ O( k n^2 ) \f$ time and \f$ O( k n ) \f$ space;
 * - `painterPartition()` solves it for integer items not negative and at most
 *   \f$ k \f$ parts by a binary search on the answer \f$ x \f$: the greedy
 *   that closes a part only when the next item does not fit in \f$ x \f$ uses
 *   the fewest parts, so \f$ x \f$ is feasible if and only if the greedy needs
 *   at most \f$ k \f$ of them; \f$ O( n \log \sum_i A_i ) \f$ time and
 *   \f$ O( 1 ) \f$ space. With items not negative and \f$ k \le n \f$, at most
 *   \f$ k \f$ parts and exactly \f$ k \f$ nonempty ones give the same value.
 *
 * In the literature, the problem on a chain of \f$ n \f$ items is solved in
 * \f$ O( n ) \f$ time for \f$ k \f$ fixed, and in
 * \f$ O( n + k^2 \log^2 n ) \f$ in general, by parametric search on the
 * prefix sums (Frederickson); the dynamic programming above has the
 * quadrangle inequality, so that the monotone cuts of Knuth and Yao bring it
 * to \f$ O( k n ) \f$ for items not negative.
 *
 * The contracts [see `Contract.h`]: \f$ 1 \le k \le n \f$ for
 * `linearPartition()`, and its cuts give the cost found (checked at level 3);
 * \f$ k \ge 1 \f$ and the items not negative for `painterPartition()`. The
 * functions are defined in `LinearPartition.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13, the
 *      Esercizi Programmazione dinamica, exercise B4 of 05/06/2014, and the
 *      Esercizi vari, exercise B2 of 21/01/2019,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see S. S. Skiena, The Algorithm Design Manual, 3rd ed., Springer, 2020,
 *      section 10.7
 * \see G. N. Frederickson, Optimal algorithms for tree partitioning, Proc.
 *      2nd ACM-SIAM Symposium on Discrete Algorithms, 168-177, 1991
 * \see F. F. Yao, Efficient dynamic programming using quadrangle
 *      inequalities, Proc. 12th ACM Symposium on Theory of Computing,
 *      429-435, 1980, doi:10.1145/800141.804691
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

#ifndef __ADS_LinearPartition
 #define __ADS_LinearPartition
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the least largest sum of a part over the splits of \f$ A \f$ in \f$ k \f$
/// nonempty contiguous parts, and the starts of the parts after the first one
///
/// In pseudocode (exercise B4 of 05/06/2014, Partition k, of the Esercizi
/// Programmazione dinamica of the book), \f$ S \f$ the prefix sums and
/// \f$ \mathit{cut}[ t ][ i ] \f$ the start of the last part of a best split
/// of the first \f$ i \f$ items in \f$ t \f$ parts:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{linearPartition}( A , k ) \cr
/// 2 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do}\ D[ 1 ][ i
///     ] \gets S[ i ] \cr
/// 3 & \quad \textbf{for}\ t \gets 2\ \textbf{to}\ k\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ i \gets t\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad\quad D[ t ][ i ] \gets \infty \cr
/// 6 & \qquad\quad \textbf{for}\ j \gets t - 1\ \textbf{to}\ i - 1
///     \ \textbf{do} \cr
/// 7 & \qquad\qquad x \gets \max\{ D[ t - 1 ][ j ] , S[ i ] - S[ j ] \} \cr
/// 8 & \qquad\qquad \textbf{if}\ x < D[ t ][ i ]\ \textbf{then}\ D[ t ][ i ]
///     \gets x;\ \mathit{cut}[ t ][ i ] \gets j \cr
/// 9 & \quad \text{read the starts back from } \mathit{cut}[ k ][ n ] \cr
/// 10 & \quad \textbf{return}\ D[ k ][ n ] \text{ and the starts}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le k \le n \f$
/// \post \f$ k - 1 \f$ increasing starts in \f$ [ 1 , n ) \f$ of the cost
///       found (audit)
template< class T >
std::pair< T , std::vector< std::size_t > > linearPartition(
                                  const std::vector< T > & A , std::size_t k );

/// the least largest sum of a part over the splits of \f$ A \f$ in at most
/// \f$ k \f$ contiguous parts; 0 for \f$ A \f$ empty
///
/// In pseudocode (beyond the book, for the exercise B2 of 21/01/2019,
/// Partition, of its Esercizi vari), the binary search on the answer, each
/// candidate checked by the greedy:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fits}( A , k , x ) \cr
/// 2 & \quad \mathit{parts} \gets 1;\ s \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ a \in A\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ s + a \le x\ \textbf{then}\ s \gets s + a \cr
/// 5 & \qquad \textbf{else}\ \mathit{parts} \gets \mathit{parts} + 1;\ s \gets
///     a;\ \textbf{if}\ \mathit{parts} > k\ \textbf{then return false} \cr
/// 6 & \quad \textbf{return true} \cr
/// 7 & \textbf{function}\ \mathrm{painterPartition}( A , k ) \cr
/// 8 & \quad \mathit{lo} \gets \max_i A_i;\ \mathit{hi} \gets \sum_i A_i \cr
/// 9 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 10 & \qquad m \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor \cr
/// 11 & \qquad \textbf{if}\ \mathrm{fits}( A , k , m )\ \textbf{then}
///     \ \mathit{hi} \gets m\ \textbf{else}\ \mathit{lo} \gets m + 1 \cr
/// 12 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ k \ge 1 \f$ and \f$ A \f$ not negative
template< std::integral T >
T painterPartition( const std::vector< T > & A , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/LinearPartition.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LinearPartition.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File LinearPartition.h -----------------------*/
/*--------------------------------------------------------------------------*/
