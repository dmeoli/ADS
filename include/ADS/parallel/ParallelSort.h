/*--------------------------------------------------------------------------*/
/*--------------------------- File ParallelSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel sorts, beyond the book: `sort( ex , A , less )` and
 * `stableSort( ex , A , less )` sort the vector \f$ A \f$ on the threads of
 * the executor `ex` [see `Executor.h`], with the same result as
 * `sort( A , less )` and `stableSort( A , less )` [see `Sort.h`], which they
 * call on the parts they cut \f$ A \f$ into; with fewer than
 * `kParallelSortThreads` threads, or fewer than `kParallelSort` items, they
 * are those sequential sorts.
 *
 * A first pass, a parallel reduction by blocks, finds whether \f$ A \f$ is
 * already sorted (then nothing moves) or in decreasing order (strictly, for
 * the stable sort: then a parallel reversal), by stretches of 32 pairs with
 * no branch inside (which vectorize); a block stops at the first stretch out
 * of both orders, so that on most inputs the pass reads little. A second
 * pass, by blocks as well, cuts each of 8 blocks (or \f$ p \f$ blocks, if
 * more) into its maximal runs, non-decreasing or strictly decreasing, and
 * gives up at the ninth run of a block, which again on most inputs comes
 * after a few items. If no block gave up, \f$ A \f$ is made of at most 8 runs
 * per block (e.g., the vectors made of a few sorted sequences, or of an
 * increasing and a decreasing one), whose decreasing ones are reversed and
 * which are merged by the parallel `multiwayMerge()` [see `ParallelMerge.h`]
 * through a buffer, stably, in \f$ O( ( n / p ) \log r ) \f$ time for
 * \f$ r \f$ runs, as `powerSort()` [see `RunSorts.h`] merges them in the
 * sequential sort (a distribution would forget that order). Otherwise the
 * items are distributed into \f$ k \f$ buckets whose concatenation is
 * sorted, by one of two classifiers:
 *
 * - numbers in their natural order: with \f$ \ell \f$ and \f$ u \f$ the
 *   least and the greatest of the keys of `radixKey()` [see `RadixSort.h`]
 *   (\f$ -0 \f$ taken as \f$ +0 \f$) in a sample of 1024 items, the bucket of
 *   \f$ x \f$ is
 *   \f[
 *     \min\bigl( \max( \mathrm{key}( x ) - \ell , 0 ) \gg s ,
 *                ( u - \ell ) \gg s \bigr) ,
 *   \f]
 *   with \f$ s \f$ the least shift that leaves at most 256 buckets, i.e.,
 *   the 8 highest bits in which the keys of the sample differ (the parallel
 *   radix sort of IPS2Ra takes them from the least and the greatest of all
 *   the keys, a pass more over the input); the keys out of the range of the
 *   sample go to the two buckets at the ends, which the sort of the buckets
 *   takes care of. The bits must spread the keys, which they do not when the
 *   keys are the floating point numbers of a few exponents, whose highest
 *   bits are the sign and the exponent, or integers of a law with a heavy
 *   tail: if a bucket gets more than 4 times its share of the sample (and 32
 *   more), the numbers go to the other classifier. When the sample shows at
 *   most 32 distinct values that the bits do not separate, the numbers are
 *   counted instead (not the floating point numbers of the stable sort):
 *   each item is found among those values by counting the keys less than its
 *   own (32 comparisons in vectors), the keys telling apart all the values,
 *   \f$ -0 \f$ and \f$ +0 \f$ included, and \f$ A \f$ is filled with them in
 *   order, 2 passes over \f$ A \f$; an item of another value sends \f$ A \f$
 *   to the other classifier, whose buckets of equality move the items once.
 *   When the sample holds at most 256 distinct keys, each key has a bucket
 *   of its own; if the integers of \f$ A \f$ all fall within the bounds of
 *   the sample (which the pass of the counts checks), the counts are those
 *   of a counting sort, and each bucket is filled with its key, read in the
 *   same pass, with no move of the items: 2 passes over \f$ A \f$ in all;
 * - any other item: \f$ k - 1 \f$ splitters taken from a sorted random
 *   sample of \f$ 16 k \f$ items (oversampling, so that the buckets are
 *   within a small factor of \f$ n / k \f$ with high probability), placed in
 *   an implicit binary tree that a search descends with no branch, as in the
 *   super scalar samplesort of Sanders and Winkel; if the sample repeats a
 *   splitter, the items equal to each splitter go to a bucket of their own,
 *   which needs no sort (the buckets of equality of IPS4o), so that
 *   \f$ k \f$ distinct keys cost \f$ O( n \log k ) \f$.
 *
 * \f$ k \f$ is 256, or \f$ 4 p \f$ rounded up to a power of 2 when \f$ n \f$
 * is small, with \f$ p \f$ the threads. The classification is a parallel
 * loop on blocks of \f$ A \f$, each of which counts its items per bucket (and
 * keeps the bucket of each item when the classifier compares, so that the
 * items are classified once); the counts give, by a prefix sum in the order
 * (bucket, block), where each block puts its items of each bucket in a buffer
 * of \f$ n \f$ items, and a second parallel loop moves them there. Items
 * copied as bytes, several to a line of the cache, go first to a line of 64
 * bytes per bucket on the stack of the thread, which is copied to the buffer
 * when full (the software write-combining of the radix sorts): the
 * \f$ k \f$ streams of stores then leave whole lines, instead of reading each
 * line of the buffer before writing an item into it. The buffer is kept for
 * the next call [see `Scratch.h`], since the faults of a fresh one cost as
 * much as the moves of the items into it. A block moves its items in their
 * order and the blocks take consecutive places, so that the distribution is
 * stable.
 *
 * Last, the threads claim the buckets from the largest down (the schedule
 * LPT of Graham, within \f$ 4 / 3 \f$ of the best), and each moves the bucket
 * into a vector of its own, sorts it by the sequential sort, and moves it
 * into its place in \f$ A \f$. A bucket of numbers of 32 or 64 bits in their
 * natural order goes instead straight to its place, where
 * `vectorQuickSort()` [see `VectorQuickSort.h`], the sort that `sort()`
 * would choose for it, sorts it unless a scan finds it already in order
 * (e.g., a bucket of equal keys at an end) or made of at most `kRuns` runs,
 * which go to the sequential sort and its merges (one move less per item;
 * not for the stable sort of floating point numbers, which must keep the
 * order of \f$ -0 \f$ and \f$ +0 \f$), and a bucket of equal items is moved
 * back as it is. A bucket larger than \f$ n / p \f$, which one thread would
 * take too long on, is sorted first by the same parallel sort, on all the
 * threads, down to a depth of 3 recursions, below which the sequential sort
 * runs.
 *
 * With \f$ n \f$ items and \f$ p \f$ threads the sort moves each item 4 times
 * (into the buffer, into the vector of its bucket, back to \f$ A \f$, plus
 * the moves of the sequential sort) and makes \f$ O( n \log k ) \f$
 * comparisons to classify, in \f$ O( n / p + k p ) \f$ time for the
 * classification and \f$ O( ( n / p ) \log( n / k ) ) \f$ for the buckets
 * when they are balanced; the buffer takes \f$ n \f$ items and the counts
 * \f$ k B \f$ words for \f$ B \f$ blocks.
 *
 * `learnedSort( ex , A )` and `learnedStableSort( ex , A )` distribute the
 * numbers of 32 or 64 bits by the model of the CDF of `LearnedSort.h`
 * instead of the two classifiers above, a grid on the range of a sample of
 * 4096 keys that finds the bucket of a key among 256 by its cell and 4
 * comparisons with no branch (the tree takes 8 dependent steps), when the
 * sample shows a distribution that the model fits, and are
 * `sort( ex , A )` and `stableSort( ex , A )` otherwise; the buckets go on
 * as above. `sort( ex , A )` does not choose them by itself, since on the
 * measures they did not beat the bits and the tree beyond the noise.
 *
 * The contracts [see `Contract.h`]: `less` is a strict weak order, and its
 * calls from several threads at once do not race; the vector ends with the
 * same items sorted (an audit). The functions are defined in
 * `ParallelSort.tpp`.
 *
 * \see P. Sanders, S. Winkel, Super scalar sample sort, European
 *      Symposium on Algorithms (ESA), LNCS 3221, 784-796, 2004,
 *      doi:10.1007/978-3-540-30140-0_69
 * \see M. Axtmann, S. Witt, D. Ferizovic, P. Sanders, Engineering in-place
 *      (shared-memory) sorting algorithms, ACM Transactions on Parallel
 *      Computing 9(1), 1-62, 2022, doi:10.1145/3505286
 * \see R. L. Graham, Bounds on multiprocessing timing anomalies, SIAM
 *      Journal on Applied Mathematics 17(2), 416-429, 1969,
 *      doi:10.1137/0117039
 * \see A. Kristo, K. Vaidya, U. Çetintemel, S. Misra, T. Kraska, The case
 *      for a learned sorting algorithm, Proceedings of SIGMOD 2020,
 *      1001-1016, 2020, doi:10.1145/3318464.3389752
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

#ifndef __ADS_ParallelSort
 #define __ADS_ParallelSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelMerge.h"
#include "ADS/parallel/Scratch.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items from which the parallel sorts use more than one thread,
/// \f$ 2^{15} \f$
inline constexpr std::size_t kParallelSort = std::size_t( 1 ) << 15;

/// the threads from which the parallel sorts distribute: on 2 threads the
/// passes of the distribution cost more than the second thread saves (on
/// the benchmark, 0.6 to 1.5 times the time of the sequential sort, 0.8
/// in the median), and the sequential sort runs instead
inline constexpr std::size_t kParallelSortThreads = 3;

/// the vector \f$ A \f$ sorted by `less` on the threads of `ex`, not
/// necessarily stable
///
/// In pseudocode (the samplesort of Sanders and Winkel and of IPS4o, beyond
/// the book), on \f$ p \f$ threads, with the recursion of line 18 at most 3
/// deep:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{sort}( \mathit{ex} , A , \mathit{less} )
///     \cr
/// 2 & \quad \textbf{if}\ p < 3\ \textbf{or}\ n < 2^{15}\ \textbf{then}
///     \ \mathrm{sort}( A , \mathit{less} );\ \textbf{return}\quad
///     \triangleright\ \text{sequential} \cr
/// 3 & \quad \textbf{if}\ A \text{ is sorted}\ \textbf{then return}\quad
///     \triangleright\ \text{a parallel reduction by blocks} \cr
/// 4 & \quad \textbf{if}\ A \text{ is decreasing}\ \textbf{then}
///     \ \text{reverse } A \text{ in parallel};\ \textbf{return} \cr
/// 5 & \quad \textbf{if}\ \text{each block of } A \text{ has at most 8 }
///     \text{runs}\ \textbf{then} \cr
/// 6 & \qquad \text{reverse the decreasing runs};\ \mathrm{multiwayMerge}(
///     \mathit{ex} , \text{the runs} , A );\ \textbf{return} \cr
/// 7 & \quad \textbf{if}\ \text{the items are numbers in their natural }
///     \text{order}\ \textbf{then} \cr
/// 8 & \qquad K \gets \text{the classifier by the highest bits of the keys }
///     \text{of a sample of 1024} \cr
/// 9 & \qquad \textbf{if}\ K \text{ spreads the sample}\ \textbf{then}
///     \ \mathrm{distribute}( \mathit{ex} , A , K );\ \textbf{return} \cr
/// 10 & \qquad \textbf{if}\ \text{the sample has at most 32 values, and }
///     \text{so has } A\ \textbf{then} \cr
/// 11 & \qquad\quad \text{count the items of each value, fill } A \text{ in }
///     \text{order};\ \textbf{return} \cr
/// 12 & \quad K \gets \text{the classifier by the } k - 1 \text{ splitters }
///     \text{of a sorted sample of } 16 k \cr
/// 13 & \quad \mathrm{distribute}( \mathit{ex} , A , K ) \cr
/// 14 & \textbf{procedure}\ \mathrm{distribute}( \mathit{ex} , A , K ) \cr
/// 15 & \quad \textbf{for}\ b \gets 1\ \textbf{to}\ B\ \textbf{do in }
///     \textbf{parallel}\ c[ \beta ][ b ] \gets \text{the number of the }
///     \text{items of the block } b \text{ in the bucket } \beta \cr
/// 16 & \quad \mathit{start} \gets \text{the prefix sums of } c \text{ in }
///     \text{the order } ( \beta , b ) \cr
/// 17 & \quad \textbf{for}\ b \gets 1\ \textbf{to}\ B\ \textbf{do in }
///     \textbf{parallel}\ \text{move the items of } b \text{, in order, to }
///     \text{their places in a buffer} \cr
/// 18 & \quad \textbf{foreach}\ \text{bucket } \beta \text{ with } |\beta| >
///     n / p\ \textbf{do}\ \mathrm{sort}( \mathit{ex} , \beta , \mathit{less}
///     ) \text{ into its place in } A \cr
/// 19 & \quad \textbf{foreach}\ \text{other bucket } \beta \text{, from the }
///     \text{largest, claimed by a free thread}\ \textbf{do} \cr
/// 20 & \qquad \mathrm{sort}( \beta , \mathit{less} ) \text{ into its place }
///     \text{in } A\quad \triangleright\ \text{a bucket of equal items as }
///     \text{it is}
/// \end{array}}
/// \f]
/// \pre `less` is a strict weak order, whose calls from several threads do
///      not race
/// \post \f$ A \f$ is sorted, with the same items (an audit)
template< Executor E , class T , class C = std::less<> >
void sort( E & ex , std::vector< T > & A , C less = C() );

/// the vector \f$ A \f$ sorted by `less` on the threads of `ex`, the equal
/// items in their order
///
/// In pseudocode: the procedure of `sort( ex , A , less )`, whose
/// distribution is stable, with \f$ A \f$ decreasing only if strictly, the
/// stable sequential sort on the buckets and the stable merge of the runs.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{stableSort}( \mathit{ex} , A ,
///     \mathit{less} ) \cr
/// 2 & \quad \mathrm{sort}( \mathit{ex} , A , \mathit{less} )\quad
///     \triangleright\ \text{with } \mathrm{stableSort}( \beta ,
///     \mathit{less} ) \text{ on the buckets}
/// \end{array}}
/// \f]
/// \pre `less` is a strict weak order, whose calls from several threads do
///      not race
/// \post \f$ A \f$ is sorted, with the same items (an audit)
template< Executor E , class T , class C = std::less<> >
void stableSort( E & ex , std::vector< T > & A , C less = C() );

/// the vector \f$ A \f$ of numbers sorted in their natural order on the
/// threads of `ex`, distributed by the model of the CDF of a sample [see
/// `LearnedSort.h`] when it fits, as `sort( ex , A )` otherwise and for the
/// buckets; not necessarily stable
///
/// In pseudocode (the learned sort of Kristo et al., beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{learnedSort}( \mathit{ex} , A ) \cr
/// 2 & \quad \textbf{if}\ p \ge 3\ \textbf{and}\ n \ge 2^{15}\ \textbf{then}
///     \cr
/// 3 & \qquad M \gets \text{the model of the CDF of a sample of 4096 keys, }
///     \text{for 256 buckets} \cr
/// 4 & \qquad \textbf{if}\ M \text{ fits the sample}\ \textbf{then}
///     \ \mathrm{distribute}( \mathit{ex} , A , M );\ \textbf{return} \cr
/// 5 & \quad \mathrm{sort}( \mathit{ex} , A )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (an audit)
template< Executor E , VectorKey T >
void learnedSort( E & ex , std::vector< T > & A );

/// the vector \f$ A \f$ of numbers sorted as `learnedSort( ex , A )` sorts
/// it, the equal items in their order (\f$ -0 \f$ and \f$ +0 \f$ among
/// them), as `stableSort( ex , A )`
///
/// In pseudocode: the procedure of `learnedSort( ex , A )`, with the stable
/// distribution and `stableSort( ex , A )` in place of `sort( ex , A )`.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{learnedStableSort}( \mathit{ex} , A ) \cr
/// 2 & \quad \mathrm{learnedSort}( \mathit{ex} , A )\quad \triangleright
///     \ \text{with } \mathrm{stableSort} \text{ for } \mathrm{sort}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (an audit)
template< Executor E , VectorKey T >
void learnedStableSort( E & ex , std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
