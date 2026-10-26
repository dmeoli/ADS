/*--------------------------------------------------------------------------*/
/*--------------------------- File ParallelSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel sorts, beyond the book: sort( ex , A , less ) and
 * stableSort( ex , A , less ) sort the vector A on the threads of the
 * executor ex [see Executor.h], with the same result as sort( A , less )
 * and stableSort( A , less ) [see Sort.h], which they call on the parts
 * they cut A into; with fewer than kParallelSortThreads threads, or fewer
 * than kParallelSort items, they are those sequential sorts.
 *
 * A first pass, a parallel reduction by blocks, finds whether A is
 * already sorted (then nothing moves) or in decreasing order (strictly,
 * for the stable sort: then a parallel reversal), by stretches of 32
 * pairs with no branch inside (which vectorize); a block stops at the
 * first stretch out of both orders, so that on most inputs the pass
 * reads little. A second pass, by blocks as well, cuts each of 8 blocks (or p
 * blocks, if more) into its maximal runs, non-decreasing or strictly
 * decreasing, and gives up at the ninth run of a block, which again on
 * most inputs comes after a few items; if no block gave up, A is made of
 * at most 8 runs per block (e.g., the vectors made of a few sorted
 * sequences, or of an increasing and a decreasing one), whose decreasing
 * ones are reversed and which are merged by the parallel multiwayMerge()
 * [see ParallelMerge.h] through a buffer, stably, in \f$ O( n \log r / p
 * ) \f$ time for r runs, as powerSort() [see RunSorts.h] merges them in
 * the sequential sort (a distribution would forget that order). Otherwise
 * the items are distributed into k buckets whose concatenation is sorted,
 * by one of two classifiers:
 * - numbers in their natural order: with \f$ \ell \f$ and u the least and
 *   the greatest of the keys of radixKey() [see RadixSort.h] (-0 taken as
 *   +0) in a sample of 1024 items, the bucket of x is
 *   \f[
 *     \min\bigl( \max( key( x ) - \ell , 0 ) \gg s , ( u - \ell ) \gg s
 *     \bigr) ,
 *   \f]
 *   with s the least shift that leaves at most 256 buckets, i.e., the 8
 *   highest bits in which the keys of the sample differ (the parallel
 *   radix sort of IPS2Ra takes them from the least and the greatest of all
 *   the keys, a pass more over the input); the keys out of the sample's
 *   range go to the two buckets at the ends, which the sort of the
 *   buckets takes care of. The bits must spread the keys, which they do
 *   not when the keys are the floating point numbers of a few exponents,
 *   whose highest bits are the sign and the exponent, or integers of a
 *   law with a heavy tail: if a bucket gets more than 4 times its share
 *   of the sample (and 32 more), the numbers go to the other classifier.
 *   When the sample shows at most 32 distinct values that the bits do not
 *   separate, the numbers are counted instead (not the floating point
 *   numbers of the stable sort): each item is found among those values by
 *   counting the keys less than its own (32 comparisons in vectors), the
 *   keys telling apart all the values, -0 and +0 included, and A is
 *   filled with them in order, 2 passes over A; an item of another value
 *   sends A to the other classifier, whose buckets of equality move the
 *   items once.
 *   When the sample holds at most 256 distinct keys, each key has a bucket
 *   of its own; if the integers of A all fall within the bounds of the
 *   sample (which the pass of the counts checks), the counts are those of
 *   a counting sort, and each bucket is filled with its key, read in the
 *   same pass, with no move of the items: 2 passes over A in all;
 * - any other item: k - 1 splitters taken from a sorted random sample of
 *   16 k items (oversampling, so that the buckets are within a small
 *   factor of n / k with high probability), placed in an implicit binary
 *   tree that a search descends with no branch, as in the super scalar
 *   samplesort of Sanders and Winkel; if the sample repeats a splitter,
 *   the items equal to each splitter go to a bucket of their own, which
 *   needs no sort (the buckets of equality of IPS4o), so that k distinct
 *   keys cost \f$ O( n \log k ) \f$.
 * k is 256, or 4 p rounded up to a power of 2 when n is small, with p the
 * threads. The classification is a parallel loop on blocks of A, each of
 * which counts its items per bucket (and keeps the bucket of each item
 * when the classifier compares, so that the items are classified once);
 * the counts give, by a prefix sum in the order (bucket, block), where
 * each block puts its items of each bucket in a buffer of n items, and a
 * second parallel loop moves them there. Items copied as bytes, several
 * to a line of the cache, go first to a line of 64 bytes per bucket on
 * the stack of the thread, which is copied to the buffer when full (the
 * software write-combining of the radix sorts): the k streams of stores
 * then leave whole lines, instead of reading each line of the buffer
 * before writing an item into it. The buffer is kept for the next call
 * [see Scratch.h], since the faults of a fresh one cost as much as the
 * moves of the items into it. A block moves its items in their order and
 * the blocks take consecutive places, so that the distribution is stable.
 * Last, the threads claim the buckets from the largest down (the
 * schedule LPT of Graham, within 4/3 of the best), and each moves the
 * bucket into a vector of its own, sorts it by the sequential sort, and
 * moves it into its place in A; a
 * bucket of numbers of 32 or 64 bits in their natural order goes instead
 * straight to its place, where vectorQuickSort() [see VectorQuickSort.h],
 * the sort that sort() would choose for it, sorts it unless a scan finds
 * it already in order (e.g., a bucket of equal keys at an end) or made of
 * at most kRuns runs, which go to the sequential sort and its merges (one
 * move
 * less per
 * item; not for the stable sort of floating point numbers, which must
 * keep the order of -0 and +0), and a bucket of equal items is moved back
 * as it is. A bucket larger than n / p, which one thread would take
 * too long on, is sorted first by the same parallel sort, on all the
 * threads.
 *
 * With n items and p threads the sort moves each item 4 times (into the
 * buffer, into the vector of its bucket, back to A, plus the moves of the
 * sequential sort) and makes \f$ O( n \log k ) \f$ comparisons to
 * classify, in \f$ O( n / p + k p ) \f$ time for the classification and
 * \f$ O( ( n / p ) \log( n / k ) ) \f$ for the buckets when they are
 * balanced; the buffer takes n items and the counts \f$ k B \f$ words
 * for B blocks.
 *
 * The contracts [see Contract.h]: less is a strict weak order, and its
 * calls from several threads at once do not race; the vector ends with
 * the same items sorted (an audit).
 * \see P. Sanders, S. Winkel, Super scalar sample sort, European
 *      Symposium on Algorithms (ESA), LNCS 3221, 784-796, 2004,
 *      doi:10.1007/978-3-540-30140-0_69
 * \see M. Axtmann, S. Witt, D. Ferizovic, P. Sanders, Engineering in-place
 *      (shared-memory) sorting algorithms, ACM Transactions on Parallel
 *      Computing 9(1), 1-62, 2022, doi:10.1145/3505286
 * \see R. L. Graham, Bounds on multiprocessing timing anomalies, SIAM
 *      Journal on Applied Mathematics 17(2), 416-429, 1969,
 *      doi:10.1137/0117039
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// the items from which the parallel sorts use more than one thread
inline constexpr std::size_t kParallelSort = std::size_t( 1 ) << 15;

/// the threads from which the parallel sorts distribute: on 2 threads the
/// passes of the distribution cost more than the second thread saves (on
/// the benchmark, 0.6 to 1.5 times the time of the sequential sort, 0.8
/// in the median), and the sequential sort runs instead
inline constexpr std::size_t kParallelSortThreads = 3;

/// A sorted by less on the threads of ex, not necessarily stable
/// \post A is sorted, with the same items (an audit)
template< Executor E , class T , class C = std::less<> >
void sort( E & ex , std::vector< T > & A , C less = C() );

/// A sorted by less on the threads of ex, the equal items in their order
/// \post A is sorted, with the same items (an audit)
template< Executor E , class T , class C = std::less<> >
void stableSort( E & ex , std::vector< T > & A , C less = C() );

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
