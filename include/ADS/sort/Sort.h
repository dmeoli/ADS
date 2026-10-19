/*--------------------------------------------------------------------------*/
/*------------------------------ File Sort.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort that chooses its algorithm, beyond the book: sort() and
 * stableSort() look at what they sort and pick, among the sorts of ADS, the
 * one that suits it.
 *
 * - With the natural order (std::less) on integers or floating point
 *   numbers, a short vector goes to pdqSort(); a long one with at most
 *   kRuns runs (non-decreasing or strictly decreasing stretches, counted
 *   by a pass that stops at the run kRuns + 1) goes to powerSort() [see
 *   RunSorts.h], which merges them in \f$ O( n \log r ) \f$ for r runs,
 *   O( n ) if sorted or reversed; sort() sends to pdqSort() a vector
 *   whose sample of 64 keys shows at most 16 distinct ones differing in 4
 *   bytes or more, which it sorts in \f$ O( n \log k ) \f$ with fewer
 *   passes than the radix sort needs; the rest goes to radixSort() [see
 *   RadixSort.h], which makes no comparison and takes O( w n ) for keys of
 *   w bytes, skipping the bytes that all the keys share; radixSort() is
 *   stable, so stableSort() uses it too, with -0 and +0 as one key, since
 *   std::less finds them equal (radixSort() puts -0 first). A NaN breaks
 *   the natural order, so that a vector with a NaN has no defined order.
 * - With the natural order on strings, a vector of at most kRuns runs goes
 *   to powerSort() and any other to msdRadixSort() [see RadixSort.h],
 *   which reads one character at a time instead of comparing whole
 *   strings, skips the prefixes that a group shares and, on the benchmark,
 *   beats the in-place multikeyQuickSort() of Bentley and Sedgewick [see
 *   MultikeyQuickSort.h] (two equal strings cannot be told apart, so that
 *   stability means nothing there).
 * - Otherwise, i.e., with any other order or item, sort() uses pdqSort()
 *   [see EngineeredQuickSorts.h], \f$ O( n \log n ) \f$ in the worst case,
 *   O( n ) on a sorted or reversed vector and \f$ O( n \log k ) \f$ with
 *   k distinct keys, and stableSort() uses powerSort() [see RunSorts.h],
 *   O( n ) on few runs and nearly optimal on any number of them.
 *
 * The thresholds between the algorithms, kRadix items for the numbers,
 * kStrings for the strings and kRuns runs, are measured by the benchmark of
 * ADS [see bench/sort.cpp] against std::sort, std::stable_sort and the
 * sorts of Boost; they are constants of the namespace, so that a program
 * may read them.
 *
 * The contracts [see Contract.h]: the vector ends with the same items
 * sorted, an audit of the sort chosen. The functions are defined in
 * Sort.tpp.
 *
 * \see O. R. L. Peters, Pattern-defeating quicksort, arXiv:2106.05123,
 *      2021
 * \see J. I. Munro, S. Wild, Nearly-optimal mergesorts: fast, practical
 *      sorting methods that optimally adapt to existing runs, Proceedings
 *      of ESA 2018, LIPIcs 112, article 63, 2018
 * \see J. L. Bentley, R. Sedgewick, Fast algorithms for sorting and
 *      searching strings, Proceedings of the 8th ACM-SIAM Symposium on
 *      Discrete Algorithms (SODA), 360-369, 1997
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Sort
 #define __ADS_Sort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items from which the numbers go to radixSort()
inline constexpr std::size_t kRadix = 512;

/// the items from which the strings go to the sorts by characters
inline constexpr std::size_t kStrings = 64;

/// the runs up to which the numbers and the strings go to powerSort()
inline constexpr std::size_t kRuns = 16;

/// the sort chosen for A and less, not necessarily stable
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void sort( std::vector< T > & A , C less = C() );

/// the stable sort chosen for A and less
/// \post A is sorted, with the same items, the equal ones in their order
///       (an audit of the order)
template< class T , class C = std::less<> >
void stableSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Sort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Sort.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Sort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
