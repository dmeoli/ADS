/*--------------------------------------------------------------------------*/
/*------------------------- File VectorQuickSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksort in the lanes of the vector registers, beyond the book:
 * vectorQuickSort() sorts integers and floating point numbers of 32 or 64
 * bits by their natural order, partitioning W of them per instruction,
 * where W is the number of lanes of a register (16 or 8 on AVX-512, 8 or 4
 * on AVX2), and sorting the parts of at most 16 W items by a sorting
 * network held in 16 registers.
 *
 * - The keys: an integer is its own key, compared signed or not on
 *   AVX-512 and made signed on AVX2 by flipping its sign bit. A floating
 *   point number is sorted as the signed integer b of its bits, with no
 *   conversion: b keeps the order of the numbers with the sign bit clear
 *   and reverses that of those with it set, which come all first, since
 *   for them \f$ b = -2^{w-1} + | x |_{bits} \f$ grows with the magnitude;
 *   once sorted, their block, found by a binary search, is reversed in
 *   place, a pass over the negative numbers alone instead of a conversion
 *   of all of them before and after the sort. The order is that of IEEE
 *   754 extended to a total order, -0 before +0 and the NaN at the ends,
 *   so that a NaN breaks nothing.
 * - Few keys in a small range: on AVX-512, from 4096 integers, if a sample
 *   of 64 spans fewer than \f$ A L / 4 \f$ values, for L the bits of a lane
 *   and \f$ A \in \{ 1 , 2 , 4 \} \f$ registers (32 values for either
 *   width), the keys are counted in one pass with no comparison: the key
 *   \f$ x = lo + d \f$ sets the bit \f$ 4 ( d - a L / 4 ) \f$ of its lane
 *   in the register a by a variable shift (none if out of reach), so that
 *   each lane counts L / 4 values in its nibbles; every 15 registers the
 *   nibbles are added into bytes, every 17 of those the bytes into the
 *   counts, and the largest offset is checked, a key out of the window
 *   giving up the counting for the quicksort. A second pass writes the
 *   keys, whole registers at a time: \f$ O( n ) \f$ against the
 *   \f$ O( n \log k ) \f$ of the partitions by three ways.
 * - The partition of a part a[ 0 .. n ) by a pivot p, as in the quicksort
 *   of Bramas: the first and the last U W keys (U = 8) are loaded aside,
 *   which leaves U W free places at each end, and then U blocks of W keys
 *   at a time are read from the end with fewer free places; each block is
 *   compared with p in one instruction into a mask m of W bits, and its
 *   \f$ \ell = | m | \f$ keys below p are written at the left end and the
 *   \f$ W - \ell \f$ others at the right one. Before a read the two ends
 *   have \f$ f_L + f_R = 2 U W \f$ free places, and the read is from the
 *   end with \f$ \min( f_L , f_R ) \leq U W \f$, so that after it both
 *   have at least U W: no write overtakes a key not yet read, and the
 *   partition is in place with no branch on the keys (the choice of the
 *   end is one branch per U blocks). On AVX-512 the two pieces are
 *   compressed into the memory (vpcompress); on AVX2 a table of
 *   \f$ 2^W \f$ permutations puts the lanes of m first and the others after
 *   them in one register, which is written whole at the left end and whole
 *   at the right one, as in the quicksort of Gueron and Krasnov; on
 *   AVX-512 the keys of 64 bits take the same way, with a table of 256
 *   permutations of 8 lanes and one vpermq, cheaper than two compressions
 *   into the memory. Each read of U blocks prefetches into the first
 *   level the U blocks 8 U W keys further in the same direction, which
 *   nearly halves the time of a pass over a part larger than the cache of
 *   the second level, where the hardware prefetcher alone falls behind
 *   the two ends read in turn.
 * - The pivot is the median of a sample of \f$ S = \max( 8 , W ) \f$ keys,
 *   one from each of S equal stretches of the part at the same random
 *   offset, sorted by the network below. If the sample repeats the pivot,
 *   on AVX-512 the partition goes three ways in one pass: the keys equal to
 *   p are not written, and the gap they leave between the two sides is
 *   filled with p at the end, while the minimum and the maximum of each
 *   side are kept by masked instructions, so that a side of one key is
 *   done; elsewhere the partition keeps the minimum and the maximum of the
 *   part and a second pass puts the keys equal to p apart. A pivot that is
 *   the minimum of the part, so that no key is below it, also gets that
 *   second pass. Hence k distinct keys take \f$ O( n \log k ) \f$.
 * - The smaller side recurs and the larger one loops, with a stack of
 *   \f$ O( \log n ) \f$; past \f$ 2 \lfloor \log_2 n \rfloor \f$ levels a
 *   part is sorted by heapsort, as in the introsort of Musser, so that the
 *   worst case is \f$ O( n \log n ) \f$.
 * - A part of at most 16 W keys is loaded into \f$ M = 2^{\lceil \log_2
 *   \lceil n / W \rceil \rceil} \f$ registers, the lanes past n filled
 *   with the largest key, and sorted by a network of compare-exchange
 *   steps, each a minimum and a maximum. With \f$ M \geq W \f$ the keys
 *   are held by columns, the key of index \f$ g = i M + h \f$ in the lane i
 *   of the register h: the columns are sorted by the odd-even merge sort of
 *   Batcher, register against register (63 comparators for M = 16, each a
 *   minimum and a maximum of whole registers), then merged by the bitonic
 *   network in its variant whose comparators all go upwards, the index g
 *   compared first with \f$ g \oplus ( K - 1 ) \f$ for blocks of K keys and
 *   then with \f$ g \oplus J \f$ for \f$ J = K / 4 , \dots , 1 \f$, a step
 *   between registers for J < M and within them, by a shuffle of the
 *   lanes, otherwise, and at last each block of W registers is transposed
 *   by \f$ \log_2 W \f$ steps of exchanges of blocks of lanes. The steps
 *   within registers, the dearer ones (a shuffle besides the minimum and
 *   the maximum), are then \f$ \frac{1}{2} \log_2 W ( \log_2 W - 1 ) \f$
 *   in all, besides a reversal per merge (one permutation of the lanes on
 *   AVX-512) and the transposition, 14 for M = W = 16 against the 26 of
 *   the bitonic network on registers held by rows, which the smaller
 *   networks use, with constant masks of the lanes that keep the minimum.
 *   On AVX-512 the column network also serves \f$ W / 2 \leq M < W \f$:
 *   each block of M lanes is transposed by \f$ \log_2 M \f$ steps, after
 *   which the lanes \f$ [ c M , c M + M ) \f$ of the register h hold the M
 *   keys from \f$ ( c M + h ) M \f$, each block written by a masked store.
 *
 * - Two sorted runs are merged in the registers by mergeRuns(), as in the
 *   merge sort of Chhugani et al.: the register of the keys carried over
 *   is merged with the next register of the run whose next key is smaller
 *   (the second one reversed, a minimum and a maximum, then the steps of a
 *   bitonic merge within each register), the smaller W keys written and
 *   the larger carried; the writes never overtake the reads, so that one
 *   run may stay in place while the other is a copy aside, forward from
 *   the smallest keys or backward from the largest. sort() merges with it
 *   the runs of the integers of 32 bits [see Sort.h].
 *
 * The instructions are chosen when compiling: AVX-512 (F) if __AVX512F__
 * is defined, AVX2 if __AVX2__ is, with GCC or Clang; otherwise, or with
 * ADS_VECTOR_SORT=0, a register is an array of W = 32 / w lanes for keys
 * of w bytes, the same algorithm in portable code, which the compiler
 * vectorizes as it can. kVectorSort tells whether the registers are real.
 *
 * vectorQuickSort() is not stable (equal integers cannot be told apart,
 * but -0 and +0 can). The contracts [see Contract.h]: the vector ends with
 * the same items sorted, an audit. The functions are defined in
 * VectorQuickSort.tpp.
 *
 * \see B. Bramas, A novel hybrid quicksort algorithm vectorized using
 *      AVX-512 on Intel Skylake, International Journal of Advanced
 *      Computer Science and Applications 8(10), article 44, 2017,
 *      doi:10.14569/IJACSA.2017.081044
 * \see S. Gueron, V. Krasnov, Fast quicksort implementation using AVX
 *      instructions, The Computer Journal 59(1), 83-90, 2016,
 *      doi:10.1093/comjnl/bxv063
 * \see J. Wassenberg, M. Blacher, J. Giesen, P. Sanders, Vectorized and
 *      performance-portable quicksort, Software: Practice and Experience
 *      52(12), 2684-2699, 2022, doi:10.1002/spe.3142
 * \see J. Chhugani, A. D. Nguyen, V. W. Lee, W. Macy, M. Hagog, Y.-K. Chen,
 *      A. Baransi, S. Kumar, P. Dubey, Efficient implementation of sorting
 *      on multi-core SIMD CPU architecture, Proceedings of the VLDB
 *      Endowment 1(2), 1313-1324, 2008, doi:10.14778/1454159.1454171
 * \see K. E. Batcher, Sorting networks and their applications, Proceedings
 *      of the AFIPS Spring Joint Computer Conference 32, 307-314, 1968,
 *      doi:10.1145/1468075.1468121
 * \see D. R. Musser, Introspective sorting and selection algorithms,
 *      Software: Practice and Experience 27(8), 983-993, 1997
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

#ifndef __ADS_VectorQuickSort
 #define __ADS_VectorQuickSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef ADS_VECTOR_SORT
 #if defined( __GNUC__ ) && ( defined( __AVX512F__ ) || \
                              defined( __AVX2__ ) )
  #define ADS_VECTOR_SORT 1
 #else
  #define ADS_VECTOR_SORT 0
 #endif
#endif

#if ADS_VECTOR_SORT
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// true if vectorQuickSort() runs in vector registers (AVX-512 or AVX2),
/// false if in the arrays of the portable code
inline constexpr bool kVectorSort = ADS_VECTOR_SORT != 0;

/// the types that vectorQuickSort() sorts: integers and floating point
/// numbers of 32 or 64 bits
template< class T >
concept VectorKey = ( std::is_integral_v< T > ||
                      std::is_floating_point_v< T > ) &&
                    ( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 8 ) ) &&
                    ( ! std::is_same_v< T , bool > );

/// the quicksort of A in the lanes of the vector registers, by the natural
/// order (the total order of IEEE 754 for floating point numbers)
/// \post A is sorted, with the same items (an audit)
template< VectorKey T >
void vectorQuickSort( std::vector< T > & A );

/// the same on a[ 0 .. n )
template< VectorKey T >
void vectorQuickSort( T * a , std::size_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/VectorQuickSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* VectorQuickSort.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File VectorQuickSort.h -----------------------*/
/*--------------------------------------------------------------------------*/
