/*--------------------------------------------------------------------------*/
/*------------------------- File SequenceQueries.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Questions on a sequence that a sort or a single scan answers faster than
 * the comparison of all the pairs: the smaller items on the right of each
 * item, the longest run with few distinct items, the longest stretch that a
 * set of intervals leaves uncovered, and the cheapest matching of two sets of
 * points on a line.
 *
 * - `smallerToTheRight()`: for each place \f$ i \f$, the number of places
 *   \f$ j > i \f$ with \f$ A[ j ] < A[ i ] \f$, by mergesort on the places
 *   [see `MergeSort.h`]: when an item of the left half goes to the output,
 *   the items of the right half gone before it are exactly the smaller ones
 *   on its right in that half, and the halves below added the others;
 *   \f$ O( n \log n ) \f$ time and \f$ O( n ) \f$ space, against the
 *   \f$ \Theta( n^2 ) \f$ of the double loop. Equal items do not count,
 *   since the left one goes first. The sum of the counts is the number of
 *   inversions [see `countInversions()`]; a Fenwick tree on the ranks gives
 *   the same counts in \f$ O( n \log n ) \f$ by a scan from the right;
 * - `longestKDistinctRun()`: the first longest run of consecutive items with
 *   at most \f$ k \f$ distinct values, by a window whose right end advances
 *   one place at a time and whose left end advances while the window holds
 *   more than \f$ k \f$ values, with the counts of its values in a search
 *   tree; each end moves \f$ n \f$ times, \f$ O( n \log k ) \f$;
 * - `largestGap()`: the longest stretch between the first start and the last
 *   end of a set of closed intervals \f$ [ a , b ] \f$ covered by none of
 *   them, the first one on ties: the intervals by start, and a gap wherever a
 *   start is beyond the farthest end so far, the union of intervals by a
 *   sweep; \f$ O( n \log n ) \f$;
 * - `lineAssignment()` (exercise 14.8): the least sum of
 *   \f$ | h_i - l_{\pi( i )} | \f$ over the matchings \f$ \pi \f$ of two sets
 *   of \f$ n \f$ points on a line, both sorted and matched in order: if
 *   \f$ h_1 \le h_2 \f$ and \f$ l_1 \le l_2 \f$ the crossed matching never
 *   costs less than the straight one, an exchange argument;
 *   \f$ O( n \log n ) \f$, against the \f$ O( n^3 ) \f$ of the Hungarian
 *   method on general costs. The same order is optimal for any convex function
 *   of the difference, the optimal transport on the line.
 *
 * The contracts [see `Contract.h`]: the counts of `smallerToTheRight()` sum
 * to the inversions of \f$ A \f$ (an audit); the run of
 * `longestKDistinctRun()` has at most \f$ k \f$ values (an audit); the
 * intervals of `largestGap()` must have \f$ a \le b \f$, and its gap meets
 * none of them (an audit); `lineAssignment()` needs two sets of the same
 * size. The functions are defined in `SequenceQueries.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 14.8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf, and
 *      the Esercizi Divide-et-impera, exercise A3 of 14/06/2024
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
 * \see H. W. Kuhn, The Hungarian method for the assignment problem, Naval
 *      Research Logistics Quarterly 2(1-2), 83-97, 1955,
 *      doi:10.1002/nav.3800020109
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

#ifndef __ADS_SequenceQueries
 #define __ADS_SequenceQueries
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the counts \f$ c[ i ] \f$ of the places \f$ j > i \f$ with
/// \f$ A[ j ] < A[ i ] \f$, by mergesort on the places; \f$ O( n \log n ) \f$
///
/// In pseudocode (exercise A3 of 14/06/2024, Smaller Count, of the
/// Esercizi Divide-et-impera of the book), the bottom-up
/// mergesort of the places \f$ P \f$, the runs of width \f$ w \f$ merged in
/// pairs:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{smallerToTheRight}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad c \gets ( 0 , \ldots , 0 );\ P \gets ( 0 , 1 , \ldots , n - 1 );
///     \ w \gets 1 \cr
/// 3 & \quad \textbf{while}\ w < n\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ \text{pair of runs } P[ \mathit{lo} .. m - 1
///     ] , P[ m .. \mathit{hi} - 1 ]\ \textbf{do} \cr
/// 5 & \qquad\quad \text{merge them by } A \text{, the left item first on a }
///     \text{tie} \cr
/// 6 & \qquad\quad \textbf{when}\ P[ i ] \text{ of the left run is output}
///     \ \textbf{do} \cr
/// 7 & \qquad\qquad c[ P[ i ] ] \gets c[ P[ i ] ] + ( \text{the items of }
///     \text{the right run output so far} ) \cr
/// 8 & \qquad w \gets 2 w \cr
/// 9 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \post the sum of \f$ c \f$ is the number of inversions of \f$ A \f$
///       (audit)
template< class T , class C = std::less<> >
std::vector< std::size_t > smallerToTheRight( const std::vector< T > & A ,
                                              C less = C() );

/// the start and the length of the first longest run of consecutive items
/// of \f$ S \f$ with at most \f$ k \f$ distinct values, by a sliding window;
/// \f$ O( n \log k ) \f$
///
/// In pseudocode (the sliding window, beyond the book), \f$ N \f$ a
/// search tree of the counts of the values in the window:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestKDistinctRun}( S[ 0 .. n - 1 ] , k
///     ) \cr
/// 2 & \quad s \gets 0;\ \ell \gets 0;\ b \gets 0;\ N \gets \emptyset \cr
/// 3 & \quad \textbf{for}\ e \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad N[ S[ e ] ] \gets N[ S[ e ] ] + 1 \cr
/// 5 & \qquad \textbf{while}\ N \text{ has more than } k \text{ values}
///     \ \textbf{do} \cr
/// 6 & \qquad\quad N[ S[ s ] ] \gets N[ S[ s ] ] - 1 \text{, removed at 0};
///     \ s \gets s + 1 \cr
/// 7 & \qquad \textbf{if}\ e + 1 - s > \ell\ \textbf{then}\ \ell \gets e + 1
///     - s;\ b \gets s \cr
/// 8 & \quad \textbf{return}\ ( b , \ell )
/// \end{array}}
/// \f]
/// \post the run has at most \f$ k \f$ distinct values (audit)
template< std::ranges::random_access_range R >
std::pair< std::size_t , std::size_t > longestKDistinctRun( const R & S ,
                                                            std::size_t k );

/// the longest stretch \f$ ( x , y ) \f$, between the first start and the
/// last end, covered by none of the closed intervals \f$ I \f$, the first one
/// on ties, or nothing; \f$ O( n \log n ) \f$
///
/// In pseudocode (the sweep of the line, beyond the book), the sweep of
/// the intervals by start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{largestGap}( I ) \cr
/// 2 & \quad \text{sort } I \text{ by start};\ g \gets \textbf{nil} \cr
/// 3 & \quad e \gets \text{the end of the first interval}\quad
///     \triangleright\ \text{the farthest end so far} \cr
/// 4 & \quad \textbf{foreach}\ [ a , b ] \in I\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ e < a\ \textbf{and}\ ( g = \textbf{nil}
///     \ \textbf{or}\ a - e > |g| )\ \textbf{then}\ g \gets ( e , a ) \cr
/// 6 & \qquad e \gets \max\{ e , b \} \cr
/// 7 & \quad \textbf{return}\ g
/// \end{array}}
/// \f]
/// \pre \f$ a \le b \f$ for each interval \f$ [ a , b ] \f$
/// \post no interval meets \f$ ( x , y ) \f$ (audit)
template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::optional< std::pair< T , T > > largestGap(
                                       std::vector< std::pair< T , T > > I );

/// the least sum of \f$ | H[ i ] - L[ \pi( i ) ] | \f$ over the permutations
/// \f$ \pi \f$, computed in the type `W`, by sorting; \f$ O( n \log n ) \f$
///
/// In pseudocode (exercise 14.8 of the book), the matching in
/// order being optimal by an exchange argument:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lineAssignment}( H , L ) \cr
/// 2 & \quad \text{sort } H \text{ and } L \cr
/// 3 & \quad \textbf{return}\ \sum_{i=0}^{n-1} | H[ i ] - L[ i ] |
/// \end{array}}
/// \f]
/// \pre \f$ H \f$ and \f$ L \f$ have the same size
template< class T , class W = T >
 requires std::is_arithmetic_v< T >
W lineAssignment( std::vector< T > H , std::vector< T > L );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/SequenceQueries.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceQueries.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SequenceQueries.h -----------------------*/
/*--------------------------------------------------------------------------*/
