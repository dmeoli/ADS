/*--------------------------------------------------------------------------*/
/*------------------------ File ExternalSorting.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of n atomic items in the two-level memory [see
 * TwoLevelMemory.h], by merging and by distribution, with the lower
 * bounds that make both optimal, written as functions that a test can
 * compare with the I/Os counted.
 *
 * - binaryMergeSort() cuts the input in runs of \f$ R = \lfloor M / B
 *   \rfloor B \f$ items, sorted in memory (the recursion of the mergesort
 *   stopped where the subsequence fits, which saves its \f$ \log_2 M \f$
 *   lowest levels), and then merges the runs two by two, three blocks of
 *   memory; each pass reads and writes every block once, hence at most
 *   \f$ 2 \lceil n / B \rceil ( 1 + \lceil \log_2 ( n / R ) \rceil ) \f$
 *   I/Os, mergeSortIOs() with k = 2.
 * - multiwayMergeSort() merges \f$ k = M / B - 1 \f$ runs at a time, a block
 *   per run and one for the output, the minimum chosen by a tree of losers
 *   [see LoserTree.h] or by any structure with its interface (a parameter, so
 *   that the same algorithm runs on the trees and the heaps of ADS and of the
 *   other libraries, with the same I/Os), so that the passes drop to \f$
 *   \lceil \log_k ( n / R ) \rceil \f$ and the I/Os to \f$ O( ( n / B )
 *   \log_{M / B} ( n / M ) ) \f$ (Theorem 5.1 of the notes), which is optimal.
 *   With snowPlow the runs are made by replacement selection, the snow plow of
 *   Knuth [see replacementSelection() of ExternalSort.h, the same runs in
 *   memory], a heap of M - 2 B items that gives runs of 2 M items on
 *   average on a random input and a single run on a sorted one.
 * - sortingLowerBound() is the bound of Aggarwal and Vitter by the tree of
 *   the I/Os: a read of B items into a memory that holds M - B others can
 *   have at most \f$ \binom{M}{B} \f$ outcomes, times \f$ B! \f$ when the B
 *   items are read for the first time, and the leaves must be at least n!;
 *   the smallest t with \f$ \binom{M}{B}^t ( B! )^{\lceil n / B \rceil}
 *   \geq n! \f$ bounds the reads, \f$ t = \Omega( ( n / B ) \log_{M / B} (
 *   n / M ) ) \f$, and the output adds its \f$ \lceil n / B \rceil \f$
 *   writes. permutingLowerBound() counts also the writes, each of which can
 *   go to n + 1 places: \f$ ( ( n + 1 ) \binom{M}{B} )^t ( B! )^{\lceil n /
 *   B \rceil} \geq n! \f$, which gives \f$ \Omega( \min\{ n , ( n / B )
 *   \log_{M / B} ( n / M ) \} ) \f$ (Theorem 5.4): moving the items, not
 *   finding their order, is the cost of a sort on the disk.
 * - permuteBySorting() is the algorithm of Theorem 5.2: the pairs \f$
 *   \langle i , \pi( i ) \rangle \f$ sorted by \f$ \pi( i ) \f$, a
 *   coordinated scan with the input that fills in the items, a sort back by
 *   i, two sorts and three scans; permuteDirect() moves one item per read,
 *   up to n reads, better only when B is below the logarithm of the sort.
 * - distributionSort() is the multiway quicksort of section 5.3.4: k - 1
 *   pivots taken from a sample of \f$ ( a + 1 ) k - 1 \f$ random items,
 *   \f$ a + 1 = \lceil 12 \ln k \rceil \f$ (Algorithm 5.7 and Lemma 5.1:
 *   with probability at least 1/2 every bucket gets fewer than 4 n / k
 *   items, else the sample is drawn again, at most twice), the items
 *   distributed by one scan into the buckets between the pivots and into
 *   one bucket per pivot for the items equal to it (the three-way
 *   partition of section 5.3.1, so that equal keys stop the recursion),
 *   and the buckets sorted in turn, in memory once they fit. The memory
 *   holds the input block, the 2 k - 1 blocks of the buckets, the block of
 *   the output and the pivots, hence \f$ k = \lfloor ( M - B ) / ( 2 B + 1 )
 *   \rfloor \f$, half the fan-in of the merge, and the same \f$ O( ( n / B
 *   ) \log_{M / B} ( n / M ) ) \f$ I/Os on average.
 *
 * Compared with the literature: the merge with a tree of losers is the
 * standard of the external sorters (Knuth 5.4.1, the STXXL library of
 * Dementiev, Kettner and Sanders), the distribution with oversampling the
 * sample sort of Frazer and McKellar, here as in the analysis of Vitter's
 * survey; on D disks the striping turns either into a sorter with blocks
 * of D B items, \f$ O( ( n / D B ) \log_{M / D B} ( n / M ) ) \f$, which
 * loses a factor \f$ 1 - \log_{M / B} D \f$ in the base of the logarithm,
 * and Greed Sort of Nodine and Vitter recovers the optimum (section 5.4,
 * not simulated here: one disk is the model of these functions). The
 * items of the model are atomic: a pair or a triple counts as one item.
 *
 * The contracts [see Contract.h]: \f$ M \geq 3 B \f$ for the merges and
 * \f$ M \geq 5 B + 2 \f$ for the distribution; the files share the memory,
 * the output file is empty; the output holds the items of the input,
 * sorted, an audit; pi is a permutation of 0 , ... , n - 1, an audit. The
 * functions are defined in ExternalSorting.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5
 * \see A. Aggarwal, J. S. Vitter, The input/output complexity of sorting
 *      and related problems, Communications of the ACM 31(9), 1116-1127,
 *      1988, doi:10.1145/48529.48535
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, sections 5.4.1 and 5.4.9
 * \see W. D. Frazer, A. C. McKellar, Samplesort: a sampling approach to
 *      minimal storage tree sorting, Journal of the ACM 17(3), 496-507,
 *      1970, doi:10.1145/321592.321600
 * \see M. H. Nodine, J. S. Vitter, Greed Sort: optimal deterministic
 *      sorting on parallel disks, Journal of the ACM 42(4), 919-933, 1995,
 *      doi:10.1145/210332.210337
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

/// the I/Os of a mergesort of fan-in k at most, 2 ceil( n / B ) ( 1 +
/// ceil( log_k( ceil( n / R ) ) ) ), R = floor( M / B ) B
/// \pre B >= 1, M >= B and k >= 2
[[nodiscard]] inline std::uint64_t mergeSortIOs( std::size_t n ,
                                                 std::size_t M ,
                                                 std::size_t B ,
                                                 std::size_t k );

/// the least I/Os of a sort of n items by comparisons, ceil( n / B )
/// writes plus the reads that the tree of the I/Os needs
/// \pre B >= 1 and M >= 2 B
[[nodiscard]] inline std::uint64_t sortingLowerBound( std::size_t n ,
                                                      std::size_t M ,
                                                      std::size_t B );

/// the least I/Os of a permutation of n items
/// \pre B >= 1 and M >= 2 B
[[nodiscard]] inline std::uint64_t permutingLowerBound( std::size_t n ,
                                                        std::size_t M ,
                                                        std::size_t B );

/// the items of in, sorted, into out, by the binary mergesort
/// \pre M >= 3 B, out empty, the two files on the same memory
/// \post out sorted, with the items of in (an audit)
template< class T , class C = std::less<> >
void binaryMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                      C less = C() );

/// the items of in, sorted, into out, by the mergesort of fan-in M / B - 1;
/// the runs by replacement selection if snowPlow; the least of the heads
/// of the runs chosen by a Sel, the tree of losers of the notes by default
/// [see LoserTree.h for its interface]
/// \pre M >= 3 B, out empty, the two files on the same memory
/// \post out sorted, with the items of in (an audit)
template< class T , class C = std::less<> , class Sel = LoserTree< T , C > >
void multiwayMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                        C less = C() , bool snowPlow = false );

/// the items of in, sorted, into out, by the distribution with pivots
/// from a random sample drawn by g
/// \pre M >= 5 B + 2, out empty, the two files on the same memory
/// \post out sorted, with the items of in (an audit)
template< class T , class R , class C = std::less<> >
void distributionSort( const DiskFile< T > & in , DiskFile< T > & out ,
                       R & g , C less = C() );

/// out[ i ] = in[ pi[ i ] ], by two sorts and three scans
/// \pre pi a permutation of 0 , ... , n - 1 (an audit), M >= 3 B, out
///      empty, the three files on the same memory
template< class T >
void permuteBySorting( const DiskFile< T > & in ,
                       const DiskFile< std::size_t > & pi ,
                       DiskFile< T > & out );

/// out[ i ] = in[ pi[ i ] ], one read per item at most
/// \pre pi a permutation of 0 , ... , n - 1 (an audit), out empty, the
///      three files on the same memory
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
