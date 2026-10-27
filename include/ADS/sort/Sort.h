/*--------------------------------------------------------------------------*/
/*------------------------------ File Sort.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort that chooses its algorithm, beyond the book: `sort()` and
 * `stableSort()` look at what they sort and pick, among the sorts of ADS, the
 * one that suits it.
 *
 * - With the natural order (`std::less`) on integers or floating point numbers
 *   of 32 or 64 bits, when the vector registers are there (`kVectorSort`,
 *   AVX-512 or AVX2), a short vector goes to `vectorQuickSort()` [see
 *   `VectorQuickSort.h`]; a long one with at most `kRuns` runs (non-decreasing
 *   or strictly decreasing stretches, counted by a pass that stops at the run
 *   \f$ \mathtt{kRuns} + 1 \f$ and keeps where they end, with no branch inside
 *   blocks of 32 items, which vectorize) has its runs merged with no second
 *   search for them: those of integers of 32 bits two by two in the vector
 *   registers (`mergeVectorRuns()`, on AVX-512), the others, up to 16 of them,
 *   by the merges of `powerSort()` [see `RunSorts.h`], \f$ O( n \log r ) \f$
 *   for \f$ r \f$ runs; one of integers of 64 bits whose sample of 64 keys
 *   differs in one byte and spans at least `kCountRange` values goes to the
 *   counting sort of `radixSort()`, and any other to `vectorQuickSort()`,
 *   \f$ O( n \log n ) \f$, \f$ O( n ) \f$ for few keys in a small range or for
 *   at most 16 distinct keys (counted in the registers) and
 *   \f$ O( n \log k ) \f$ for \f$ k \f$ distinct keys otherwise, \f$ W \f$
 *   keys per instruction. `stableSort()` does the same, since two equal
 *   numbers cannot be told apart, but a vector of floating point numbers that
 *   holds a \f$ -0 \f$, which `std::less` finds equal to \f$ +0 \f$, and has
 *   more than 8 or `kRuns` runs goes to `powerSort()` below `kRadix` items and
 *   to `radixSortBy()` with \f$ -0 \f$ and \f$ +0 \f$ as one key from there.
 * - A vector of `kOneRun` items or more copied as bytes, of any type and
 *   order, is first checked for one run: strictly decreasing, it is reversed
 *   in the same pass that checks it, 32 pairs of items from the two ends at a
 *   time, checked before they are swapped (and swapped back if a later block
 *   is out of order), one read and one write of each item instead of a scan
 *   and then a reversal; otherwise it is scanned for an item smaller than the
 *   one before it, 64 items at a time with the block 4 blocks ahead
 *   prefetched, by quarters in step if its items have at most 8 bytes and fill
 *   more than the cache of the second level (four streams of reads), and the
 *   scan of the first run goes on from where that stopped.
 * - With the natural order on the other integers and floating point numbers,
 *   or with no vector registers, a short vector goes to `pdqSort()` and a long
 *   one in few runs to `powerSort()` in the same way; `sort()` sends to
 *   `pdqSort()` a vector whose sample of 64 keys shows at most 16 distinct
 *   ones differing in 4 bytes or more, which it sorts in \f$ O( n \log k ) \f$
 *   with fewer passes than the radix sort needs; the rest goes to
 *   `radixSort()` [see `RadixSort.h`], which makes no comparison and takes
 *   \f$ O( w n ) \f$ for keys of \f$ w \f$ bytes, skipping the bytes that all
 *   the keys share; `radixSort()` is stable, so `stableSort()` uses it too,
 *   with \f$ -0 \f$ and \f$ +0 \f$ as one key, since `std::less` finds them
 *   equal (`radixSort()` puts \f$ -0 \f$ first). A NaN breaks the natural
 *   order, so that a vector with a NaN has no defined order.
 * - With the natural order on strings, a vector of at most `kRuns` runs goes
 *   to the merges of `powerSort()` in the same way; any other goes, when the
 *   vector registers are there, to `prefixKeySort()` [see `StringSort.h`],
 *   which reads each string a few times in all and sorts integer keys of 8 of
 *   its characters in the registers, unless a sample of 64 strings holds at
 *   most 16 distinct ones, and otherwise to `msdRadixSort()` [see
 *   `RadixSort.h`], which reads one character at a time instead of comparing
 *   whole strings, skips the prefixes that a group shares and splits few
 *   distinct strings at once, and on the benchmark beats the in-place
 *   `multikeyQuickSort()` of Bentley and Sedgewick [see `MultikeyQuickSort.h`]
 *   (two equal strings cannot be told apart, so that stability means nothing
 *   there).
 * - With any other order, items that are copied as bytes (trivially copyable)
 *   go the same way (`sort()` sends fewer than `kRadix` of them to
 *   `pdqSort()`): at most `kRuns` runs to the merges of `powerSort()`, and the
 *   others, for `sort()`, to `sampleSort()` [see `SampleSort.h`], the in-place
 *   super scalar samplesort, whose buckets of equal keys sort \f$ k \f$
 *   distinct keys in \f$ O( n \log k ) \f$; for `stableSort()`, if a sample of
 *   64 shows at most 16 distinct keys that all the items have, to a counting
 *   sort by those keys, stable, each item found among them by a binary search
 *   with no branch, through a buffer of \f$ n / 2 \f$ items,
 *   \f$ O( n \log k ) \f$ comparisons for \f$ k \f$ keys, and otherwise to a
 *   merge sort through a buffer of \f$ n / 2 \f$ items, as the spinsort of
 *   Boost: its halves are sorted with the other array as their room and merged
 *   back, one move per item and level, with no branch on the order, from both
 *   ends at once, so that no step checks where a half ends.
 * - Any other item goes, for `sort()`, to `pdqSort()` [see
 *   `EngineeredQuickSorts.h`] if fewer than `kRadix`, \f$ O( n \log n ) \f$ in
 *   the worst case, \f$ O( n ) \f$ on a sorted or reversed vector and
 *   \f$ O( n \log k ) \f$ with \f$ k \f$ distinct keys, and to `sampleSort()`
 *   otherwise; for `stableSort()`, to `powerSort()` [see `RunSorts.h`],
 *   \f$ O( n ) \f$ on few runs and nearly optimal on any number of them.
 *
 * The thresholds between the algorithms, `kRadix` items for the numbers,
 * `kStrings` for the strings and `kRuns` runs, are measured by the benchmark
 * of ADS [see `bench/sort.cpp`] against `std::sort`, `std::stable_sort` and
 * the sorts of Boost; they are constants of the namespace, so that a program
 * may read them.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit of the sort chosen. The functions are defined in
 * `Sort.tpp`.
 *
 * \see O. R. L. Peters, Pattern-defeating quicksort, arXiv:2106.05123,
 *      2021
 * \see J. I. Munro, S. Wild, Nearly-optimal mergesorts: fast, practical
 *      sorting methods that optimally adapt to existing runs, Proceedings
 *      of ESA 2018, LIPIcs 112, article 63, 2018,
 *      doi:10.4230/LIPIcs.ESA.2018.63
 * \see J. L. Bentley, R. Sedgewick, Fast algorithms for sorting and
 *      searching strings, Proceedings of the 8th ACM-SIAM Symposium on
 *      Discrete Algorithms (SODA), 360-369, 1997
 * \see F. J. Tapia, Boost.Sort: spinsort, flat_stable_sort, Boost C++
 *      Libraries 1.67, 2018,
 *      https://www.boost.org/doc/libs/1_67_0/libs/sort/doc/html/index.html
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
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/StringSort.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items from which the numbers go to `radixSort()`
inline constexpr std::size_t kRadix = 512;

/// the items from which the strings go to the sorts by characters
inline constexpr std::size_t kStrings = 64;

/// the runs up to which the numbers and the strings go to `powerSort()`
inline constexpr std::size_t kRuns = 16;

/// the sort chosen for \f$ A \f$ and `less`, not necessarily stable
///
/// In pseudocode (beyond the book), \f$ r \f$ the runs of \f$ A \f$, counted
/// up to \f$ \mathtt{kRuns} + 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{sort}( A ) \cr
/// 2 & \quad \textbf{if}\ \text{numbers of 32 or 64 bits, natural order, }
///     \text{vector registers}\ \textbf{then} \cr
/// 3 & \qquad \text{merge the runs if } r \le \mathtt{kRuns} \text{, else }
///     \mathrm{vectorQuickSort}( A ) \cr
/// 4 & \quad \textbf{else if}\ \text{other numbers, natural order}
///     \ \textbf{then} \cr
/// 5 & \qquad \textbf{if}\ n < \mathtt{kRadix}\ \textbf{then}
///     \ \mathrm{pdqSort}( A ) \cr
/// 6 & \qquad \textbf{else if}\ r \le \mathtt{kRuns}\ \textbf{then}
///     \ \text{merge the runs as } \mathrm{powerSort}() \cr
/// 7 & \qquad \textbf{else if}\ \text{a sample shows few keys wide apart}
///     \ \textbf{then}\ \mathrm{pdqSort}( A ) \cr
/// 8 & \qquad \textbf{else}\ \mathrm{radixSort}( A ) \cr
/// 9 & \quad \textbf{else if}\ \text{strings, natural order}\ \textbf{then}
///     \cr
/// 10 & \qquad \textbf{if}\ n < \mathtt{kStrings}\ \textbf{then}
///     \ \mathrm{pdqSort}( A ) \cr
/// 11 & \qquad \textbf{else if}\ r \le \mathtt{kRuns}\ \textbf{then}
///     \ \text{merge the runs as } \mathrm{powerSort}() \cr
/// 12 & \qquad \textbf{else if}\ \text{vector registers and not few distinct }
///     \text{strings}\ \textbf{then}\ \mathrm{prefixKeySort}( A ) \cr
/// 13 & \qquad \textbf{else}\ \mathrm{msdRadixSort}( A ) \cr
/// 14 & \quad \textbf{else if}\ \text{trivially copyable items}\ \textbf{then}
///     \cr
/// 15 & \qquad \textbf{if}\ n < \mathtt{kRadix}\ \textbf{then}
///     \ \mathrm{pdqSort}( A ) \cr
/// 16 & \qquad \textbf{else if}\ r \le \mathtt{kRuns}\ \textbf{then}
///     \ \text{merge the runs as } \mathrm{powerSort}() \cr
/// 17 & \qquad \textbf{else}\ \mathrm{sampleSort}( A ) \cr
/// 18 & \quad \textbf{else if}\ n < \mathtt{kRadix}\ \textbf{then}
///     \ \mathrm{pdqSort}( A )\ \textbf{else}\ \mathrm{sampleSort}( A )
/// \end{array}}
/// \f]
/// where one run is reversed if decreasing and left as it is otherwise.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void sort( std::vector< T > & A , C less = C() );

/// the stable sort chosen for \f$ A \f$ and `less`
///
/// In pseudocode (beyond the book), \f$ r \f$ the runs of \f$ A \f$ as in
/// `sort()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{stableSort}( A ) \cr
/// 2 & \quad \textbf{if}\ \text{numbers, natural order}\ \textbf{then} \cr
/// 3 & \qquad \text{as } \mathrm{sort}( A ) \text{, but } \mathrm{powerSort}(
///     A ) \text{ for the short vectors and } -0 = +0 \cr
/// 4 & \quad \textbf{else if}\ \text{strings, natural order}\ \textbf{then}
///     \cr
/// 5 & \qquad \text{as } \mathrm{sort}( A ) \text{, with } \mathrm{powerSort}(
///     A ) \text{ for the short vectors} \cr
/// 6 & \quad \textbf{else if}\ \text{trivially copyable items}\ \textbf{then}
///     \cr
/// 7 & \qquad \textbf{if}\ r \le \mathtt{kRuns}\ \textbf{then}\ \text{merge }
///     \text{the runs as } \mathrm{powerSort}() \cr
/// 8 & \qquad \textbf{else if}\ n \ge \mathtt{kRadix} \text{ and a sample }
///     \text{shows at most 16 keys}\ \textbf{then}\ \text{count by them} \cr
/// 9 & \qquad \textbf{else}\ \text{merge sort through a buffer of } n / 2
///     \text{ items} \cr
/// 10 & \quad \textbf{else}\ \mathrm{powerSort}( A )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items, the equal ones in their
///       order (audit)
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
