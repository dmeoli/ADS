/*--------------------------------------------------------------------------*/
/*---------------------- File EngineeredQuickSorts.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksorts beyond the quicksort of Montresor's book [see `QuickSort.h`
 * for it and its improvements of section 12.2]: one engineered for speed, and
 * two of chapter 5, Sorting Atomic Items, of Ferragina's book:
 *
 * - `pdqSort()`, the pattern-defeating quicksort of Peters: the introsort of
 *   Musser, i.e., the quicksort whose depth of recursion is bounded by
 *   \f$ 2 \lfloor \log_2 n \rfloor \f$, past which the part is sorted by
 *   heapsort, so that the worst case is \f$ O( n \log n ) \f$, with the
 *   pivot by the median of three, or by the ninther of Tukey on large parts,
 *   and three defences against the patterns: a partition that moved nothing
 *   is checked by an insertion sort that gives up after 8 moves, so that a
 *   sorted or reversed run takes \f$ O( n ) \f$; a pivot equal to the item
 *   before the part, i.e., a run of equal keys, gets a partition that puts
 *   the equals on its left and skips them, so that \f$ k \f$ distinct keys
 *   take \f$ O( n \log k ) \f$; a partition too unbalanced (one side below
 *   \f$ n / 8 \f$) swaps a few items to break the pattern, and too many of
 *   them fall back to heapsort. On the arithmetic types with their natural
 *   order the partition goes by blocks, as in the BlockQuicksort of Edelkamp
 *   and Weiss: the items to swap are first collected, 64 at a time, as offsets
 *   by an increment with no branch, and then moved along a cycle, or swapped
 *   in pairs when the two blocks have as many (the first with the last, so
 *   that a part in reverse order leaves both its sides sorted, which the
 *   insertion sort above then finds in \f$ O( n ) \f$), so that no
 *   comparison causes a mispredicted branch;
 * - `dualPivotQuickSort()` of Yaroslavskiy (the sort of the primitive types of
 *   Java 7), as section 5.3.5 of Ferragina's book presents it: two pivots
 *   \f$ p \le q \f$, the 2nd and the 4th of five equally spaced items, split
 *   the part into three, smaller than \f$ p \f$, between and larger than
 *   \f$ q \f$, in one pass; it makes 5% fewer comparisons and 20% more swaps
 *   than the classical one on average, as Wild and Nebel computed, and fewer
 *   scans of the memory, which is what makes it faster;
 * - `threeWayQuickSort()`, the quicksort of section 5.3 of Ferragina's book
 *   with the three-way partition of its Algorithm 5.4 (section 5.3.1): three
 *   pointers move rightward, and the items equal to the pivot gather in a
 *   middle part that no later call sorts again, so that \f$ k \f$ distinct
 *   keys cost \f$ O( n \log k ) \f$ on average; the pivot is drawn at random,
 *   as section 5.3.2 suggests, by a generator of fixed seed (the same sort on
 *   every run), and the recursion is bounded as in its Algorithm 5.6. It
 *   makes \f$ 2 n \ln n \f$ comparisons on average (Theorem 5.5 of the
 *   book), at most two per item and partition, but \f$ \Theta( n^2 ) \f$ in
 *   the worst case, when the pivots drawn are the smallest or the largest
 *   items of their parts, which happens with a probability that vanishes with
 *   \f$ n \f$ on every input, there being no fallback to heapsort.
 *
 * All sort in place and are not stable; the recursion goes into the smaller
 * part and loops on the larger, so that the stack stays \f$ O( \log n ) \f$.
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit. The functions are defined in `EngineeredQuickSorts.tpp`.
 *
 * \see D. R. Musser, Introspective sorting and selection algorithms,
 *      Software: Practice and Experience 27(8), 983-993, 1997,
 *      doi:10.1002/(SICI)1097-024X(199708)27:8\<983::AID-SPE117\>3.0.CO;2-\#
 * \see O. R. L. Peters, Pattern-defeating quicksort, arXiv:2106.05123,
 *      2021
 * \see J. W. Tukey, The ninther, a technique for low-effort robust
 *      (resistant) location in large samples, in H. A. David (ed.),
 *      Contributions to Survey Sampling and Applied Statistics, Academic
 *      Press, 251-257, 1978
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: avoiding branch
 *      mispredictions in quicksort, Proceedings of ESA 2016, LIPIcs 57,
 *      article 38, 2016, doi:10.4230/LIPIcs.ESA.2016.38
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: avoiding branch
 *      mispredictions in quicksort, ACM Journal of Experimental
 *      Algorithmics 24, article 1.4, 2019, doi:10.1145/3274660
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5, Sorting Atomic Items,
 *      doi:10.1017/9781009128933
 * \see V. Yaroslavskiy, Dual-pivot quicksort, 2009
 * \see S. Wild, M. E. Nebel, Average case analysis of Java 7's dual pivot
 *      quicksort, Proceedings of ESA 2012, LNCS 7501, 825-836, 2012,
 *      doi:10.1007/978-3-642-33090-2_71
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

#ifndef __ADS_EngineeredQuickSorts
 #define __ADS_EngineeredQuickSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the pattern-defeating quicksort
///
/// In pseudocode (Peters, beyond the book), \f$ \mathit{bad} \f$ starting from
/// \f$ \lfloor \log_2 n \rfloor + 1 \f$ and the part leftmost when no item
/// precedes it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{pdq}( A , \mathit{lo} , \mathit{hi} ,
///     \mathit{bad} ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \textbf{if}\ \mathit{hi} - \mathit{lo} < 24\ \textbf{then}
///     \ \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 4 & \qquad \text{the median of three, or the ninther, to } A[ \mathit{lo} ]
///     \cr
/// 5 & \qquad \textbf{if}\ \text{not leftmost and } A[ \mathit{lo} - 1 ] \ge
///     A[ \mathit{lo} ]\ \textbf{then} \cr
/// 6 & \qquad\quad \mathit{lo} \gets 1 + \text{the end of the equals of the }
///     \text{pivot, partitioned left};\ \textbf{continue} \cr
/// 7 & \qquad p \gets \text{the place of the pivot, the equals on the right}
///     \cr
/// 8 & \qquad \textbf{if}\ \text{a side is smaller than } ( \mathit{hi} -
///     \mathit{lo} ) / 8\ \textbf{then} \cr
/// 9 & \qquad\quad \mathit{bad} \gets \mathit{bad} - 1 \cr
/// 10 & \qquad\quad \textbf{if}\ \mathit{bad} = 0\ \textbf{then}
///     \ \text{heapsort } A[ \mathit{lo} .. \mathit{hi} - 1
///     ];\ \textbf{return}
///     \cr
/// 11 & \qquad\quad \text{swap a few items of each side} \cr
/// 12 & \qquad \textbf{else if}\ \text{nothing moved and both sides sort by }
///     \text{insertion in 8 moves}\ \textbf{then return} \cr
/// 13 & \qquad \mathrm{pdq}( A , \mathit{lo} , p , \mathit{bad} );
///     \ \mathit{lo} \gets p + 1
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void pdqSort( std::vector< T > & A , C less = C() );

/// the dual-pivot quicksort
///
/// In pseudocode (section 5.3.5 of Ferragina's book, Yaroslavskiy, with the
/// pivots of Java 7),
/// \f$ A[ \mathit{lo} + 1 .. l - 1 ] < p \f$,
/// \f$ A[ g + 1 .. \mathit{hi} - 2 ] > q \f$ and the items between in
/// \f$ [ p , q ] \f$ at all times:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dual}( A , \mathit{lo} , \mathit{hi} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 27\ \textbf{then}
///     \ \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 3 & \quad p , q \gets \text{the 2nd and 4th of five spaced items, to } A[
///     \mathit{lo} ] \text{ and } A[ \mathit{hi} - 1 ] \cr
/// 4 & \quad l \gets \mathit{lo} + 1;\ g \gets \mathit{hi} - 2;\ k \gets l \cr
/// 5 & \quad \textbf{while}\ k \le g\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ A[ k ] < p\ \textbf{then}\ A[ k ] \leftrightarrow
///     A[ l ];\ l \gets l + 1 \cr
/// 7 & \qquad \textbf{else if}\ A[ k ] \ge q\ \textbf{then} \cr
/// 8 & \qquad\quad \textbf{while}\ A[ g ] > q\ \textbf{and}\ k < g
///     \ \textbf{do}\ g \gets g - 1 \cr
/// 9 & \qquad\quad A[ k ] \leftrightarrow A[ g ];\ g \gets g - 1 \cr
/// 10 & \qquad\quad \textbf{if}\ A[ k ] < p\ \textbf{then}\ A[ k ]
///     \leftrightarrow A[ l ];\ l \gets l + 1 \cr
/// 11 & \qquad k \gets k + 1 \cr
/// 12 & \quad \text{put } p \text{ in } A[ l - 1 ] \text{ and } q \text{ in }
///     \text{} A[ g + 1 ] \cr
/// 13 & \quad \text{sort the three parts, the middle one only if } p < q
/// \end{array}}
/// \f]
/// where the two smaller parts are sorted recursively and the largest by the
/// loop.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void dualPivotQuickSort( std::vector< T > & A , C less = C() );

/// the quicksort with the three-way partition of Ferragina's book
///
/// In pseudocode (Algorithms 5.4 and 5.6 of chapter 5, Sorting Atomic Items,
/// of Ferragina's book, with \f$ n_0 = 16 \f$), the pivot \f$ P \f$ in
/// \f$ A[ \mathit{lo} ] \f$ and, while \f$ c \f$ scans,
/// \f$ A[ \mathit{lo} .. l - 1 ] < P \f$, \f$ A[ l .. r - 1 ] = P \f$ and
/// \f$ A[ r .. c - 1 ] > P \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{partition}( A , \mathit{lo} , \mathit{hi} )
///     \cr
/// 2 & \quad P \gets A[ \mathit{lo} ];\ l \gets \mathit{lo};\ r \gets
///     \mathit{lo} + 1 \cr
/// 3 & \quad \textbf{for}\ c \gets r\ \textbf{to}\ \mathit{hi} - 1
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ c ] = P\ \textbf{then}\ A[ c ] \leftrightarrow
///     A[ r ];\ r \gets r + 1 \cr
/// 5 & \qquad \textbf{else if}\ A[ c ] < P\ \textbf{then}\ A[ c ]
///     \leftrightarrow A[ l ];\ A[ c ] \leftrightarrow A[ r ];\ r \gets r +
///     1;\ l \gets l + 1 \cr
/// 6 & \quad \textbf{return}\ ( l , r ) \cr
/// 7 & \textbf{procedure}\ \mathrm{threeWay}( A , \mathit{lo} , \mathit{hi} )
///     \cr
/// 8 & \quad \textbf{while}\ \mathit{hi} - \mathit{lo} > n_0\ \textbf{do} \cr
/// 9 & \qquad A[ \mathit{lo} ] \leftrightarrow A[ x ] \text{ for } x \text{ }
///     \text{drawn at random in } [ \mathit{lo} , \mathit{hi} ) \cr
/// 10 & \qquad ( l , r ) \gets \mathrm{partition}( A , \mathit{lo} ,
///     \mathit{hi} ) \cr
/// 11 & \qquad \textbf{if}\ l - \mathit{lo} \le \mathit{hi} - r\ \textbf{then}
///     \ \mathrm{threeWay}( A , \mathit{lo} , l );\ \mathit{lo} \gets r \cr
/// 12 & \qquad \textbf{else}\ \mathrm{threeWay}( A , r , \mathit{hi} );
///     \ \mathit{hi} \gets l \cr
/// 13 & \quad \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} )
/// \end{array}}
/// \f]
/// where \f$ A[ c ] = P \f$ means that neither is smaller than the other;
/// \f$ \Theta( n^2 ) \f$ comparisons in the worst case.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void threeWayQuickSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/EngineeredQuickSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EngineeredQuickSorts.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File EngineeredQuickSorts.h ---------------------*/
/*--------------------------------------------------------------------------*/
