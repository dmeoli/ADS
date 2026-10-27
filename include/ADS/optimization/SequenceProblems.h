/*--------------------------------------------------------------------------*/
/*------------------------ File SequenceProblems.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Problems of optimization on a sequence, solved by a scan or by the dynamic
 * programming of the longest increasing subsequence [see `Subsequences.h`]:
 *
 * - `trappingRainWater()`: the water held by columns of heights \f$ h_i \f$
 *   after the rain,
 *   \f$ \sum_i \min( \max_{j \le i} h_j , \max_{j \ge i} h_j ) - h_i \f$; two
 *   indices from the ends, the lower side moving inwards, since its level is
 *   fixed by its own maximum: \f$ \Theta( n ) \f$ time and \f$ O( 1 ) \f$
 *   space, instead of the two arrays of the maxima or the stack of the
 *   decreasing columns;
 * - `largestDivisibleSubset()`: the largest subset of distinct positive
 *   integers in which of any two one divides the other: on the items sorted,
 *   a chain in the order of divisibility,
 *   \f$ L[ i ] = 1 + \max\{ L[ j ] : j < i , a_j \mid a_i \} \f$, right by the
 *   transitivity of divisibility; \f$ \Theta( n^2 ) \f$ time;
 * - `maxSumIncreasingSubsequence()`: the largest sum of a strictly increasing
 *   subsequence, the heaviest increasing subsequence with the items as
 *   weights: \f$ D[ i ] = a_i + \max\{ D[ j ] : j < i , a_j < a_i \} \f$, the
 *   maximum over the smaller values from a Fenwick tree of the prefix maxima
 *   on the ranks, \f$ O( n \log n ) \f$ time instead of the
 *   \f$ \Theta( n^2 ) \f$ scan;
 * - `longestChainSubsequence()`: the longest subsequence of \f$ S \f$ each
 *   item of which precedes the next in a strict partial order, e.g., boxes
 *   each fitting in the next,
 *   \f$ L[ j ] = 1 + \max\{ L[ i ] : i < j , S_i \prec S_j \} \f$,
 *   \f$ \Theta( n^2 ) \f$ comparisons; for a total order it is the longest
 *   increasing subsequence, in \f$ O( n \log n ) \f$. In \f$ d \f$
 *   dimensions, the boxes free to be sorted and rotated give the box stacking
 *   problem, the longest path in the DAG of the dominance;
 * - `longestDistinctSegment()`: the longest segment of \f$ A \f$ with no value
 *   repeated, by a window whose left end jumps past the last occurrence of a
 *   repeated value, with a hash table of the last occurrences;
 *   \f$ \Theta( n ) \f$ expected time.
 *
 * The contracts [see `Contract.h`]: the items of `largestDivisibleSubset()`
 * positive and distinct, and the subset divisible; the subsequences found
 * increasing in the given order. The functions are defined in
 * `SequenceProblems.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13 and the
 * Esercizi Programmazione dinamica, exercises B4 of 06/06/2011, * A3 of
 04/09/2020, B3 of 16/06/2021, B2 of 20/07/2026 and B3 of * 28/08/2026,
 https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see G. Jacobson, K.-P. Vo, Heaviest increasing/common subsequence
 *      problems, Proc. 3rd Symposium on Combinatorial Pattern Matching,
 *      LNCS 644, 52-66, 1992, doi:10.1007/3-540-56024-6_5
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
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

#ifndef __ADS_SequenceProblems
 #define __ADS_SequenceProblems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the water held by the columns of heights \f$ h \f$, no walls outside
///
/// In pseudocode (exercise B2 of 20/07/2026, Acqua alta, of the Esercizi
/// Programmazione dinamica of the book), \f$ \ell \f$ and \f$ r \f$ the
/// highest columns met from the left and from the right:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{trappingRainWater}( h[ 0 .. n - 1 ] ) \cr
/// 2 & \quad i \gets 0;\ j \gets n - 1;\ \ell \gets h[ 0 ];\ r \gets h[ n - 1
///     ];\ w \gets 0 \cr
/// 3 & \quad \textbf{while}\ i < j\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \ell < r\ \textbf{then} \cr
/// 5 & \qquad\quad i \gets i + 1;\ \ell \gets \max\{ \ell , h[ i ] \};\ w
///     \gets w + \ell - h[ i ] \cr
/// 6 & \qquad \textbf{else} \cr
/// 7 & \qquad\quad j \gets j - 1;\ r \gets \max\{ r , h[ j ] \};\ w \gets w +
///     r - h[ j ] \cr
/// 8 & \quad \textbf{return}\ w
/// \end{array}}
/// \f]
/// \post the result is not negative
template< class T >
T trappingRainWater( const std::vector< T > & h );

/// a largest subset of \f$ A \f$ in which of any two items one divides the
/// other, sorted
///
/// In pseudocode (exercise B3 of 28/08/2026, Sottoinsieme divisibile, of
/// the Esercizi Programmazione dinamica of the book), the longest chain of
/// divisors on the items
/// sorted:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{largestDivisibleSubset}( A ) \cr
/// 2 & \quad \text{sort } A \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad L[ i ] \gets 1;\ \mathit{pred}[ i ] \gets \textbf{nil} \cr
/// 5 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ i - 1\ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ A[ j ] \mid A[ i ]\ \textbf{and}\ L[ j ] + 1 >
///     L[ i ]\ \textbf{then}\ L[ i ] \gets L[ j ] + 1;\ \mathit{pred}[ i ]
///     \gets j \cr
/// 7 & \quad \textbf{return}\ \text{the chain of } \mathit{pred} \text{ from }
///     \text{the first largest } L[ i ] \text{, reversed}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ positive and distinct
/// \post each item divides the next
template< std::integral T >
std::vector< T > largestDivisibleSubset( std::vector< T > A );

/// the largest sum of a strictly increasing subsequence of \f$ A \f$, 0 for
/// the empty one
///
/// In pseudocode (Jacobson and Vo, beyond the book, for the exercise B3 of
/// 16/06/2021, Massima somma crescente, of its Esercizi Programmazione
/// dinamica), \f$ F \f$ a Fenwick tree of the prefix maxima of \f$ D \f$ on
/// the ranks of the values:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxSumIncreasingSubsequence}( A ) \cr
/// 2 & \quad \mathit{best} \gets 0;\ F \gets \text{all } 0 \cr
/// 3 & \quad \textbf{foreach}\ a \in A\ \textbf{do} \cr
/// 4 & \qquad r \gets \text{the rank of } a \text{ among the distinct values}
///     \cr
/// 5 & \qquad d \gets \max\{ a + \text{the maximum of } F \text{ over the }
///     \text{ranks below } r , 0 \} \cr
/// 6 & \qquad \mathit{best} \gets \max\{ \mathit{best} , d \};\ \text{raise }
///     F \text{ at } r \text{ to } d \cr
/// 7 & \quad \textbf{return}\ \mathit{best}
/// \end{array}}
/// \f]
/// \post the result is not negative
template< class T >
T maxSumIncreasingSubsequence( const std::vector< T > & A );

/// the places of a longest subsequence of \f$ S \f$ each item of which
/// precedes the next by `less`, a strict partial order
///
/// In pseudocode (exercise B4 of 06/06/2011, Scatole, of the
/// Esercizi Programmazione dinamica of the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestChainSubsequence}( S , \prec ) \cr
/// 2 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad L[ j ] \gets 1;\ \mathit{pred}[ j ] \gets \textbf{nil} \cr
/// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ j - 1\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ L[ i ] + 1 > L[ j ]\ \textbf{and}\ S[ i ]
///     \prec S[ j ]\ \textbf{then}\ L[ j ] \gets L[ i ] + 1;
///     \ \mathit{pred}[ j ] \gets i \cr
/// 6 & \quad \textbf{return}\ \text{the chain of } \mathit{pred} \text{ from }
///     \text{the first largest } L[ j ] \text{, reversed}
/// \end{array}}
/// \f]
/// \post increasing places, `less` on each two consecutive items (audit)
template< class T , class Less >
std::vector< std::size_t > longestChainSubsequence(
                                    const std::vector< T > & S , Less less );

/// the start and the length of the first longest segment of \f$ A \f$ with no
/// value repeated
///
/// In pseudocode (exercise A3 of 04/09/2020, Longest single, of the Esercizi
/// Programmazione dinamica of the book), \f$ \mathit{last} \f$ a hash table of
/// the last place of each value:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestDistinctSegment}( A[ 0 .. n - 1 ]
///     ) \cr
/// 2 & \quad s \gets 0;\ \ell \gets 0;\ b \gets 0 \cr
/// 3 & \quad \textbf{for}\ e \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ e ] \in \mathit{last}\ \textbf{and}
///     \ \mathit{last}[ A[ e ] ] \ge s\ \textbf{then}\ s \gets \mathit{last}[
///     A[
///     e ] ] + 1 \cr
/// 5 & \qquad \mathit{last}[ A[ e ] ] \gets e \cr
/// 6 & \qquad \textbf{if}\ e + 1 - s > \ell\ \textbf{then}\ ( b , \ell ) \gets
///     ( s , e + 1 - s ) \cr
/// 7 & \quad \textbf{return}\ ( b , \ell )
/// \end{array}}
/// \f]
/// \post the values of the segment distinct (audit)
template< class T , class Hash = std::hash< T > >
std::pair< std::size_t , std::size_t > longestDistinctSegment(
                                                  const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/SequenceProblems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceProblems.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File SequenceProblems.h -----------------------*/
/*--------------------------------------------------------------------------*/
