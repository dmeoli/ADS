/*--------------------------------------------------------------------------*/
/*------------------------ File ExternalSorting.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of \f$ n \f$ atomic items in the two-level memory [see
 * `TwoLevelMemory.h`], by merging and by distribution, with the lower
 * bounds that make both optimal, written as functions that a test can
 * compare with the I/Os counted.
 *
 * - `binaryMergeSort()` cuts the input in runs of
 *   \f$ R = \lfloor M / B \rfloor B \f$ items, sorted in memory (the
 *   recursion of the mergesort stopped where the subsequence fits, which
 *   saves its \f$ \log_2 M \f$ lowest levels), and then merges the runs two
 *   by two, three blocks of memory; each pass reads and writes every block
 *   once, hence at most
 *   \f$ 2 \lceil n / B \rceil ( 1 + \lceil \log_2 ( n / R ) \rceil ) \f$
 *   I/Os, `mergeSortIOs()` with \f$ k = 2 \f$.
 * - `multiwayMergeSort()` merges \f$ k = M / B - 1 \f$ runs at a time, a
 *   block per run and one for the output, the minimum chosen by a tree of
 *   losers [see `LoserTree.h`] or by any structure with its interface (a
 *   parameter, so that the same algorithm runs on the trees and the heaps of
 *   ADS and of the other libraries, with the same I/Os), so that the passes
 *   drop to \f$ \lceil \log_k ( n / R ) \rceil \f$ and the I/Os to
 *   \f$ O( ( n / B ) \log_{M / B} ( n / M ) ) \f$, which is optimal by
 *   the lower bound below (chapter 5, Sorting Atomic Items, of Ferragina's
 *   book). With `snowPlow` the runs are made
 *   by replacement selection, the snow plow of Knuth [see
 *   `replacementSelection()` of `ExternalSort.h`, the same runs in memory],
 *   a heap of \f$ M - 2 B \f$ items that gives runs of \f$ 2 M \f$ items on
 *   average on a random input and a single run on a sorted one.
 * - `sortingLowerBound()` is the bound of Aggarwal and Vitter by the tree of
 *   the I/Os: a read of \f$ B \f$ items into a memory that holds
 *   \f$ M - B \f$ others can have at most \f$ \binom{M}{B} \f$ outcomes,
 *   times \f$ B! \f$ when the \f$ B \f$ items are read for the first time,
 *   and the leaves must be at least \f$ n! \f$; the smallest \f$ t \f$ with
 *   \f$ \binom{M}{B}^t ( B! )^{\lceil n / B \rceil} \ge n! \f$ bounds the
 *   reads, \f$ t = \Omega( ( n / B ) \log_{M / B} ( n / M ) ) \f$, and the
 *   output adds its \f$ \lceil n / B \rceil \f$ writes.
 *   `permutingLowerBound()` counts also the writes, each of which can go to
 *   \f$ n + 1 \f$ places:
 *   \f$ ( ( n + 1 ) \binom{M}{B} )^t ( B! )^{\lceil n / B \rceil} \ge n! \f$,
 *   which gives \f$ \Omega( \min\{ n , ( n / B ) \log_{M / B} ( n / M ) \} )
 *   \f$: moving the items, not finding their order, is the cost of a sort
 *   on the disk.
 * - `permuteBySorting()` reaches that bound by sorting: the pairs
 *   \f$ \langle i , \pi( i ) \rangle \f$ sorted by \f$ \pi( i ) \f$, a
 *   coordinated scan with the input that fills in the
 *   items, a sort back by \f$ i \f$, two sorts and three scans;
 *   `permuteDirect()` moves one item per read, up to \f$ n \f$ reads, better
 *   only when \f$ B \f$ is below the logarithm of the sort.
 * - `distributionSort()` is the multiway quicksort of the same chapter:
 *   \f$ k - 1 \f$ pivots taken from a sample of \f$ ( a + 1 ) k - 1 \f$
 *   random items, \f$ a + 1 = \lceil 12 \ln k \rceil \f$ (with
 *   probability at least \f$ 1 / 2 \f$ every bucket gets fewer than
 *   \f$ 4 n / k \f$ items, else the sample is drawn again, at most twice),
 *   the items distributed by one scan into the buckets between the pivots
 *   and into one bucket per pivot for the items equal to it (the three-way
 *   partition of quicksort, so that equal keys stop the recursion), and the
 *   buckets sorted in turn, in memory once they fit. The memory holds the
 *   input block, the
 *   \f$ 2 k - 1 \f$ blocks of the buckets, the block of the output and the
 *   pivots, hence \f$ k = \lfloor ( M - B ) / ( 2 B + 1 ) \rfloor \f$, half
 *   the fan-in of the merge, and the same
 *   \f$ O( ( n / B ) \log_{M / B} ( n / M ) ) \f$ I/Os on average.
 *
 * Compared with the literature: the merge with a tree of losers is the
 * standard of the external sorters (section 5.4.1 of Knuth, the STXXL
 * library of Dementiev, Kettner and Sanders), the distribution with
 * oversampling the sample sort of Frazer and McKellar, here as in the
 * analysis of Vitter's survey; on \f$ D \f$ disks the striping turns either
 * into a sorter with blocks of \f$ D B \f$ items,
 * \f$ O( ( n / D B ) \log_{M / D B} ( n / M ) ) \f$, which loses a factor
 * \f$ 1 - \log_{M / B} D \f$ in the base of the logarithm, and Greed Sort of
 * Nodine and Vitter recovers the optimum (not simulated here: one disk is
 * the model of these functions). The items
 * of the model are atomic: a pair or a triple counts as one item.
 *
 * The contracts [see `Contract.h`]: \f$ M \ge 3 B \f$ for the merges and
 * \f$ M \ge 5 B + 2 \f$ for the distribution; the files share the memory,
 * the output file is empty; the output holds the items of the input,
 * sorted, an audit; \f$ \pi \f$ is a permutation of
 * \f$ 0 , \ldots , n - 1 \f$, an audit. The functions are defined in
 * `ExternalSorting.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5, Sorting Atomic Items,
 *      doi:10.1017/9781009128933
 * \see A. Aggarwal, J. S. Vitter, The input/output complexity of sorting
 *      and related problems, Communications of the ACM 31(9), 1116-1127,
 *      1988, doi:10.1145/48529.48535
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.4.1 and 5.4.9
 * \see W. D. Frazer, A. C. McKellar, Samplesort: a sampling approach to
 *      minimal storage tree sorting, Journal of the ACM 17(3), 496-507,
 *      1970, doi:10.1145/321592.321600
 * \see J. S. Vitter, External memory algorithms and data structures:
 *      dealing with massive data, ACM Computing Surveys 33(2), 209-271,
 *      2001, doi:10.1145/384192.384193
 * \see M. H. Nodine, J. S. Vitter, Greed Sort: optimal deterministic
 *      sorting on parallel disks, Journal of the ACM 42(4), 919-933, 1995,
 *      doi:10.1145/210332.210343
 * \see R. Dementiev, L. Kettner, P. Sanders, STXXL: standard template
 *      library for XXL data sets, Software: Practice and Experience 38(6),
 *      589-637, 2008, doi:10.1002/spe.844
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

#ifndef __ADS_ExternalSorting
 #define __ADS_ExternalSorting
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/sort/ExternalSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the I/Os of a mergesort of fan-in at most \f$ k \f$,
/// \f$ 2 \lceil n / B \rceil ( 1 + \lceil \log_k \lceil n / R \rceil \rceil )
/// \f$ with \f$ R = \lfloor M / B \rfloor B \f$: a pass that forms the runs
/// and one per level of the merges, each reading and writing every block
/// \pre \f$ B \ge 1 \f$, \f$ M \ge B \f$ and \f$ k \ge 2 \f$
[[nodiscard]] inline std::uint64_t mergeSortIOs( std::size_t n ,
                                                 std::size_t M ,
                                                 std::size_t B ,
                                                 std::size_t k );

/// the least I/Os of a sort of \f$ n \f$ items by comparisons,
/// \f$ \lceil n / B \rceil \f$ writes plus the reads that the tree of the
/// I/Os needs, never fewer than \f$ \lceil n / B \rceil \f$
///
/// The reads are the smallest \f$ t \f$ with
/// \f$ t \log_2 \binom{M}{B} \ge \log_2 n! - \lceil n / B \rceil
/// \log_2 B! \f$, the logarithms of the factorials computed by the
/// function \f$ \Gamma \f$.
/// \pre \f$ B \ge 1 \f$ and \f$ M \ge 2 B \f$
[[nodiscard]] inline std::uint64_t sortingLowerBound( std::size_t n ,
                                                      std::size_t M ,
                                                      std::size_t B );

/// the least I/Os of a permutation of \f$ n \f$ items, the larger of
/// \f$ 2 \lceil n / B \rceil \f$ and the smallest \f$ t \f$ with
/// \f$ t ( \log_2 ( n + 1 ) + \log_2 \binom{M}{B} ) \ge \log_2 n! -
/// \lceil n / B \rceil \log_2 B! \f$
/// \pre \f$ B \ge 1 \f$ and \f$ M \ge 2 B \f$
[[nodiscard]] inline std::uint64_t permutingLowerBound( std::size_t n ,
                                                        std::size_t M ,
                                                        std::size_t B );

/// the items of `in`, sorted, into `out`, by the binary mergesort
///
/// In pseudocode (the binary mergesort of chapter 5, Sorting Atomic Items,
/// of Ferragina's book, beyond the book), \f$ R = \lfloor M / B \rfloor B \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{binaryMergeSort}( \mathit{in} ,
///     \mathit{out} ) \cr
/// 2 & \quad \mathcal{R} \gets \text{the runs of } R \text{ items of }
///     \mathit{in} \text{, each read, sorted in memory and written} \cr
/// 3 & \quad \textbf{while}\ |\mathcal{R}| > 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ \text{pair } ( X , Y ) \text{ of }
///     \text{consecutive runs of } \mathcal{R}\ \textbf{do} \cr
/// 5 & \qquad\quad \text{merge } X \text{ and } Y \text{ into a new run, }
///     \text{a block of each in memory} \cr
/// 6 & \qquad \mathcal{R} \gets \text{the new runs, and a lone last run }
///     \text{as it is} \cr
/// 7 & \quad \mathit{out} \gets \text{the only run of } \mathcal{R}
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 3 B \f$, `out` empty, the two files on the same memory
/// \post `out` sorted, with the items of `in` (an audit)
template< class T , class C = std::less<> >
void binaryMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                      C less = C() );

/// the items of `in`, sorted, into `out`, by the mergesort of fan-in
/// \f$ M / B - 1 \f$; the runs by replacement selection if `snowPlow`; the
/// least of the heads of the runs chosen by a `Sel`, a tree of losers by
/// default [see `LoserTree.h` for its interface]
///
/// In pseudocode (the multiway mergesort of chapter 5, Sorting Atomic
/// Items, of Ferragina's book, beyond the book),
/// \f$ k = \lfloor M / B \rfloor - 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{multiwayMergeSort}( \mathit{in} ,
///     \mathit{out} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{snowPlow}\ \textbf{then}\ \mathcal{R}
///     \gets \text{the runs of a replacement selection on a heap of }
///     M - 2 B \text{ items} \cr
/// 3 & \quad \textbf{else}\ \mathcal{R} \gets \text{the runs of }
///     \lfloor M / B \rfloor B \text{ items, each sorted in memory} \cr
/// 4 & \quad \textbf{while}\ |\mathcal{R}| > 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{foreach}\ \text{group } G \text{ of } k
///     \text{ consecutive runs of } \mathcal{R}\ \textbf{do} \cr
/// 6 & \qquad\quad L \gets \text{a tree of losers on the first item of }
///     \text{each run of } G \cr
/// 7 & \qquad\quad \textbf{while}\ L \ne \emptyset\ \textbf{do} \cr
/// 8 & \qquad\qquad j \gets \text{the run of the winner};
///     \ \text{write } \mathrm{top}( L ) \cr
/// 9 & \qquad\qquad \mathrm{replace}( L , \text{the next item of run } j
///     \text{, or none} ) \cr
/// 10 & \qquad \mathcal{R} \gets \text{the merged runs} \cr
/// 11 & \quad \mathit{out} \gets \text{the only run of } \mathcal{R}
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 3 B \f$, `out` empty, the two files on the same memory
/// \post `out` sorted, with the items of `in` (an audit)
template< class T , class C = std::less<> , class Sel = LoserTree< T , C > >
void multiwayMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                        C less = C() , bool snowPlow = false );

/// the items of `in`, sorted, into `out`, by the distribution with pivots
/// from a random sample drawn by `g`
///
/// In pseudocode (the multiway quicksort of chapter 5, Sorting Atomic
/// Items, of Ferragina's book, beyond the book), with
/// \f$ k = \lfloor ( M - B ) / ( 2 B + 1 ) \rfloor \f$ and
/// \f$ s = \min\{ \lceil 12 \ln k \rceil k - 1 , ( M - 2 B ) / 2 , n \} \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{distribute}( F , \mathit{out} ) \cr
/// 2 & \quad \textbf{if}\ |F| \le M - B\ \textbf{then}\ \text{read } F
///     \text{, sort it in memory, append it to } \mathit{out};
///     \ \textbf{return} \cr
/// 3 & \quad \textbf{repeat} \cr
/// 4 & \qquad S \gets s \text{ random items of } F \text{, sorted};
///     \ p_i \gets S[ i s / k ] \text{ for } 1 \le i < k \text{, without }
///     \text{repetitions} \cr
/// 5 & \qquad \textbf{foreach}\ x \in F\ \textbf{do} \cr
/// 6 & \qquad\quad j \gets \text{the least } j \text{ with } x \le p_j \cr
/// 7 & \qquad\quad \textbf{if}\ x = p_j\ \textbf{then}\ \text{append } x
///     \text{ to } E_j\ \textbf{else}\ \text{append } x \text{ to } F_j \cr
/// 8 & \quad \textbf{until}\ \text{each } F_j \text{ fits in memory or }
///     \text{has fewer than } 4 |F| / k \text{ items, or the third }
///     \text{attempt} \cr
/// 9 & \quad \textbf{foreach}\ j\ \textbf{do}\ \mathrm{distribute}( F_j ,
///     \mathit{out} );\ \text{append } E_j \text{ to } \mathit{out}
/// \end{array}}
/// \f]
/// \pre \f$ M \ge 5 B + 2 \f$, `out` empty, the two files on the same memory
/// \post `out` sorted, with the items of `in` (an audit)
template< class T , class R , class C = std::less<> >
void distributionSort( const DiskFile< T > & in , DiskFile< T > & out ,
                       R & g , C less = C() );

/// \f$ \mathit{out}[ i ] = \mathit{in}[ \pi[ i ] ] \f$, by two sorts and
/// three scans
///
/// In pseudocode (chapter 5, Sorting Atomic Items, of Ferragina's book,
/// beyond the book), the permutation reduced to two sorts:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{permuteBySorting}( \mathit{in} , \pi ,
///     \mathit{out} ) \cr
/// 2 & \quad P \gets \text{the triples } \langle i , \pi[ i ] , \cdot
///     \rangle \text{, by a scan of } \pi \cr
/// 3 & \quad \text{sort } P \text{ by the second component} \cr
/// 4 & \quad \text{fill the third component of the } i \text{-th triple }
///     \text{of } P \text{ with } \mathit{in}[ i ] \text{, by a coordinated }
///     \text{scan} \cr
/// 5 & \quad \text{sort } P \text{ by the first component} \cr
/// 6 & \quad \mathit{out} \gets \text{the third components of } P
///     \text{, by a scan}
/// \end{array}}
/// \f]
/// \pre \f$ \pi \f$ a permutation of \f$ 0 , \ldots , n - 1 \f$ (an audit),
///      \f$ M \ge 3 B \f$, `out` empty, the three files on the same memory
template< class T >
void permuteBySorting( const DiskFile< T > & in ,
                       const DiskFile< std::size_t > & pi ,
                       DiskFile< T > & out );

/// \f$ \mathit{out}[ i ] = \mathit{in}[ \pi[ i ] ] \f$, one read per item at
/// most
///
/// In pseudocode (the direct permutation of chapter 5, Sorting Atomic
/// Items, of Ferragina's book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{permuteDirect}( \mathit{in} , \pi ,
///     \mathit{out} ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad \text{append } \mathit{in}[ \pi[ i ] ] \text{ to }
///     \mathit{out}\quad \triangleright\ \text{a read unless its block is }
///     \text{in the buffer}
/// \end{array}}
/// \f]
/// \pre \f$ \pi \f$ a permutation of \f$ 0 , \ldots , n - 1 \f$ (an audit),
///      `out` empty, the three files on the same memory
template< class T >
void permuteDirect( const DiskFile< T > & in ,
                    const DiskFile< std::size_t > & pi ,
                    DiskFile< T > & out );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/ExternalSorting.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExternalSorting.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ExternalSorting.h -----------------------*/
/*--------------------------------------------------------------------------*/
