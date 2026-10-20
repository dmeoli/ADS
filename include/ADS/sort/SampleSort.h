/*--------------------------------------------------------------------------*/
/*--------------------------- File SampleSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The in-place super scalar samplesort, beyond the book: sampleSort()
 * splits the vector into up to 256 buckets by splitters drawn from a
 * sample, in place with O( k b ) extra items for k buckets and blocks of b
 * items (about 2 KiB), and sorts each bucket in the same way, those of at
 * most 64 items by insertion. It sorts by comparisons alone, so that it
 * takes any order, and it is not stable.
 *
 * - The splitters: a sample of \f$ \alpha k - 1 \f$ items, with
 *   \f$ \alpha = \max( 1 , \lfloor 0.2 \log_2 n \rfloor ) \f$, is drawn at
 *   random into the front of the part and sorted, and every \f$ \alpha
 *   \f$-th item of it is a splitter; the k - 1 splitters
 *   \f$ s_1 \leq \dots \leq s_{k-1} \f$, k a power of 2, are laid out as
 *   a complete binary search tree in an array t[ 1 .. k ), the children of
 *   t[ j ] in t[ 2 j ] and t[ 2 j + 1 ]. An item x finds its bucket
 *   \f$ i = | \{ h : s_h \leq x \} | \f$ by \f$ \log_2 k \f$ steps \f$ j
 *   \leftarrow 2 j + [ t_j \leq x ] \f$ from j = 1, \f$ i = j - k \f$ at the
 *   end: an addition per step and no branch, so that no comparison is
 *   mispredicted, and 4 items go down the tree together, so that their
 *   loads overlap (the super scalar sample sort of Sanders and Winkel).
 *   The number of buckets aims at buckets of 16 items after the last
 *   level: \f$ k = 2^\ell \f$ with \f$ \ell = \lfloor \log_2( n / 16 )
 *   \rfloor \f$ if at most 8, half of it (rounded up) if at most 16, so
 *   that two levels split alike, and 8 otherwise.
 *   If the sample has equal splitters, they are kept once and each bucket
 *   i > 0 is split into the items equal to \f$ s_i \f$, which are done,
 *   and the others, so that k distinct keys take \f$ O( n \log k ) \f$.
 * - The classification in place: the items are read in order, and each
 *   goes into a buffer of b items of its bucket; a full buffer is written
 *   back to the front of the part as a block, which never overtakes the
 *   reading since the items written and buffered are those read. Then the
 *   part is a sequence of blocks of one bucket each, followed by free
 *   room, and each bucket has its count.
 * - The permutation of the blocks: the area of bucket i is the blocks from
 *   the first boundary of a block at or after its start; for each bucket,
 *   its blocks not yet placed are read from the back of its area, and each
 *   goes to the next place of its own bucket, swapping out the block there
 *   if not yet placed, which then goes to its own bucket in turn, so that
 *   each block moves once (twice through the two blocks of room).
 * - The cleanup: each bucket gets, in the places of its range that no
 *   block of it covers, the items of its buffer and those of its last block
 *   that went past its end; the buckets are done in order, so that the
 *   places that a bucket frees at the start of the next one are taken by
 *   the latter. A bucket of at most 64 items is then sorted at once, while
 *   it is in the cache, by an insertion sort that first compares an item
 *   with the front and otherwise needs no test of the front.
 *
 * Each item is read and written about twice per level, and with
 * \f$ k = 256 \f$ there are about \f$ \lceil \log_{256}( n / 16 ) \rceil \f$
 * levels; the expected number of comparisons is
 * \f$ n \log_2 n + O( n ) \f$, of which none is a branch in the
 * classification. The buckets differ from n / k by
 * a factor \f$ 1 + \epsilon \f$ with a probability that grows with
 * \f$ \alpha \f$; a bucket larger than half the part, which only an
 * unlucky sample gives, is sorted by pdqSort(), so that the worst case is
 * \f$ O( n \log n ) \f$.
 *
 * The contracts [see Contract.h]: the vector ends with the same items
 * sorted, an audit. The functions are defined in SampleSort.tpp.
 *
 * \see P. Sanders, S. Winkel, Super scalar sample sort, Proceedings of ESA
 *      2004, LNCS 3221, 784-796, 2004, doi:10.1007/978-3-540-30140-0_69
 * \see M. Axtmann, S. Witt, D. Ferizovic, P. Sanders, In-place parallel
 *      super scalar samplesort (IPSSSSo), Proceedings of ESA 2017, LIPIcs
 *      87, 9:1-9:14, 2017, doi:10.4230/LIPIcs.ESA.2017.9
 * \see M. Axtmann, S. Witt, D. Ferizovic, P. Sanders, Engineering in-place
 *      (shared-memory) sorting algorithms, ACM Transactions on Parallel
 *      Computing 9(1), 1-62, 2022, doi:10.1145/3505286
 * \see W. D. Frazer, A. C. McKellar, Samplesort: a sampling approach to
 *      minimal storage tree sorting, Journal of the ACM 17(3), 496-507,
 *      1970, doi:10.1145/321592.321600
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

#ifndef __ADS_SampleSort
 #define __ADS_SampleSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the in-place super scalar samplesort of A by less
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void sampleSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/SampleSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SampleSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SampleSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
