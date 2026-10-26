/*--------------------------------------------------------------------------*/
/*---------------------- File EngineeredQuickSorts.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksorts engineered for speed, beyond the book [see QuickSort.h
 * for the one of the book and its improvements of section 12.2]:
 *
 * - introSort() of Musser: the quicksort with the median of three, whose
 *   depth of recursion is bounded by \f$ 2 \lfloor \log_2 n \rfloor \f$,
 *   past which the part is sorted by heapsort, so that the worst case is
 *   \f$ O( n \log n ) \f$; the small parts are sorted by insertion (the
 *   std::sort of the C++ libraries);
 * - pdqSort(), the pattern-defeating quicksort of Peters: the introsort with
 *   the pivot by the median of three, or by the ninther of Tukey on large
 *   parts, and three defences against the patterns: a partition that moved
 *   nothing is checked by an insertion sort that gives up after 8 moves, so
 *   that a sorted or reversed run takes \f$ O( n ) \f$; a pivot equal to the
 *   item before the part, i.e., a run of equal keys, gets a partition that
 *   puts the equals on its left and skips them, so that k distinct keys take
 *   \f$ O( n \log k ) \f$; a partition too unbalanced (one side below n / 8)
 *   swaps a few items to break the pattern, and too many of them fall back to
 *   heapsort. On the arithmetic types with their natural order the partition
 *   goes by blocks, as in the BlockQuicksort of Edelkamp and Weiss: the items
 *   to swap are first collected, 64 at a time, as offsets by an increment with
 *   no branch, and then moved along a cycle, so that no comparison causes a
 *   mispredicted branch;
 * - dualPivotQuickSort() of Yaroslavskiy (the sort of the primitive types
 *   of Java 7): two pivots p <= q, the 2nd and the 4th of five equally
 *   spaced items, split the part into three, smaller than p, between and
 *   larger than q, in one pass; it makes 5 % fewer comparisons and 20 % more
 *   swaps than the classical one on average, as Wild and Nebel computed,
 *   and fewer scans of the memory, which is what makes it faster;
 * - threeWayQuickSort() with the partition of Bentley and McIlroy: the keys
 *   equal to the pivot are kept at the two ends during the partition and
 *   swapped to the middle at its end, so that they are never sorted again,
 *   with the ninther on large parts.
 *
 * All sort in place and are not stable; the recursion goes into the smaller
 * part and loops on the larger, so that the stack stays
 * \f$ O( \log n ) \f$. The contracts [see Contract.h]: each sort ends with
 * the same items sorted, an audit. The functions are defined in
 * EngineeredQuickSorts.tpp.
 *
 * \see D. R. Musser, Introspective sorting and selection algorithms,
 *      Software: Practice and Experience 27(8), 983-993, 1997
 * \see O. R. L. Peters, Pattern-defeating quicksort, arXiv:2106.05123,
 *      2021
 * \see J. W. Tukey, The ninther, a technique for low-effort robust
 *      (resistant) location in large samples, in H. A. David (ed.),
 *      Contributions to Survey Sampling and Applied Statistics, Academic
 *      Press, 251-257, 1978
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: how branch mispredictions
 *      don't affect quicksort, Proceedings of ESA 2016, LIPIcs 57, article
 *      38, 2016
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: avoiding branch
 *      mispredictions in quicksort, ACM Journal of Experimental
 *      Algorithmics 24, article 1.4, 2019, doi:10.1145/3274660
 * \see V. Yaroslavskiy, Dual-pivot quicksort, 2009
 * \see S. Wild, M. E. Nebel, Average case analysis of Java 7's dual pivot
 *      quicksort, Proceedings of ESA 2012, LNCS 7501, 825-836, 2012
 * \see J. L. Bentley, M. D. McIlroy, Engineering a sort function,
 *      Software: Practice and Experience 23(11), 1249-1265, 1993
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
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the introsort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void introSort( std::vector< T > & A , C less = C() );

/// the pattern-defeating quicksort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void pdqSort( std::vector< T > & A , C less = C() );

/// the dual-pivot quicksort
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void dualPivotQuickSort( std::vector< T > & A , C less = C() );

/// the three-way quicksort of Bentley and McIlroy
/// \post A is sorted, with the same items (an audit)
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
