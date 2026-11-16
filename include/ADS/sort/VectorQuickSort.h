/*--------------------------------------------------------------------------*/
/*------------------------- File VectorQuickSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksort in the lanes of the vector registers, beyond the book:
 * `vectorQuickSort()` sorts integers and floating point numbers of 32 or 64
 * bits by their natural order. With `ADS_WITH_HIGHWAY` it is the vectorized
 * quicksort of Wassenberg, Blacher, Giesen and Sanders, `hwy::VQSort()` of
 * the library of Highway, on the vectors of the processor found when the
 * program runs; without it, the quicksort below, in portable code that
 * partitions \f$ W \f$ keys per step, \f$ W = 32 / w \f$ for keys of
 * \f$ w \f$ bytes, and sorts the parts of at most \f$ 16 W \f$ keys by a
 * sorting network held in 16 registers of \f$ W \f$ lanes, arrays that the
 * compiler vectorizes as it can.
 *
 * - The keys: an integer is its own key, made signed by flipping its sign
 *   bit if it is unsigned. A floating point number is sorted as the signed
 *   integer \f$ b \f$ of its bits, with no conversion:
 *   \f$ b \f$ keeps the order of the numbers with the sign bit clear and
 *   reverses that of those with it set, which come all first, since for them
 *   \f$ b = -2^{w-1} + | x |_{\mathrm{bits}} \f$ grows with the magnitude;
 *   once sorted, their block, found by a binary search, is reversed in place,
 *   a pass over the negative numbers alone instead of a conversion of all of
 *   them before and after the sort. The order is that of IEEE 754 extended to
 *   a total order, \f$ -0 \f$ before \f$ +0 \f$ and the NaN at the ends, so
 *   that a NaN breaks nothing.
 * - The partition of a part \f$ a[ 0 .. n - 1 ] \f$ by a pivot \f$ p \f$, as
 *   in the quicksort of Bramas: the first and the last \f$ U W \f$ keys
 *   (\f$ U = 8 \f$) are loaded aside, which leaves \f$ U W \f$ free places at
 *   each end, and then \f$ U \f$ blocks of \f$ W \f$ keys at a time are read
 *   from the end with fewer free places; each block is compared with \f$ p \f$
 *   into a mask \f$ m \f$ of \f$ W \f$ bits, and its
 *   \f$ \ell = | m | \f$ keys below \f$ p \f$ are written at the left end and
 *   the \f$ W - \ell \f$ others at the right one. Before a read the two ends
 *   have \f$ f_L + f_R = 2 U W \f$ free places, and the read is from the end
 *   with \f$ \min\{ f_L , f_R \} \leq U W \f$, so that after it both have at
 *   least \f$ U W \f$: no write overtakes a key not yet read, and the
 *   partition is in place with no branch on the keys (the choice of the end is
 *   one branch per \f$ U \f$ blocks): the \f$ \ell \f$ keys and the others
 *   are put apart in an array of \f$ 2 W \f$ keys and copied to the two
 *   ends. Each read of \f$ U \f$ blocks prefetches the \f$ U \f$ blocks
 *   \f$ 8 U W \f$ keys further in the same direction, for a pass over a
 *   part larger than the cache of the second level, where the hardware
 *   prefetcher alone falls behind the two ends read in turn.
 * - The pivot is the median of a sample of \f$ S = \max\{ 8 , W \} \f$ keys,
 *   one from each of \f$ S \f$ equal stretches of the part at the same random
 *   offset, sorted by the network below. If the sample repeats the pivot,
 *   the partition keeps the minimum and the maximum of the part and a second
 *   pass puts the keys equal to \f$ p \f$ apart. A pivot that is the
 *   minimum of the part, so that no key is below it, also gets that second
 *   pass.
 * - The smaller side recurs and the larger one loops, with a stack of
 *   \f$ O( \log n ) \f$; past \f$ 2 \lfloor \log_2 n \rfloor \f$ levels a part
 *   is sorted by heapsort, as in the introsort of Musser, so that the worst
 *   case is \f$ O( n \log n ) \f$.
 * - A part of at most \f$ 16 W \f$ keys is loaded into
 *   \f$ M = 2^{\lceil \log_2 \lceil n / W \rceil \rceil} \f$ registers, the
 *   lanes past \f$ n \f$ filled with the largest key, and sorted by a network
 *   of compare-exchange steps, each a minimum and a maximum. With
 *   \f$ M \geq W \f$ the keys are held by columns, the key of index
 *   \f$ g = i M + h \f$ in the lane \f$ i \f$ of the register \f$ h \f$: the
 *   columns are sorted by the odd-even merge sort of Batcher, register against
 *   register (63 comparators for \f$ M = 16 \f$, each a minimum and a maximum
 *   of whole registers), then merged by the bitonic network in its variant
 *   whose comparators all go upwards, the index \f$ g \f$ compared first with
 *   \f$ g \oplus ( K - 1 ) \f$ for blocks of \f$ K \f$ keys and then with
 *   \f$ g \oplus J \f$ for \f$ J = K / 4 , \dots , 1 \f$, a step between
 *   registers for \f$ J < M \f$ and within them, by a shuffle of the lanes,
 *   otherwise, and at last each block of \f$ W \f$ registers is transposed by
 *   \f$ \log_2 W \f$ steps of exchanges of blocks of lanes. The steps within
 *   registers, the dearer ones (a shuffle besides the minimum and the
 *   maximum), are then \f$ \frac{1}{2} \log_2 W ( \log_2 W - 1 ) \f$ in all,
 *   besides a reversal per merge and the transposition, 14 for
 *   \f$ M = W = 16 \f$ against the 26 of the bitonic network on registers
 *   held by rows, which the smaller networks use, with
 *   constant masks of the lanes that keep the minimum.
 * - Two sorted runs are merged in the registers by `mergeRuns()`, as in the
 *   merge sort of Chhugani et al.: the register of the keys carried over is
 *   merged with the next register of the run whose next key is smaller (the
 *   second one reversed, a minimum and a maximum, then the steps of a bitonic
 *   merge within each register), the smaller \f$ W \f$ keys written and the
 *   larger carried; the writes never overtake the reads, so that one run may
 *   stay in place while the other is a copy aside, forward from the smallest
 *   keys or backward from the largest. `sort()` merges with it the runs of the
 *   integers of 32 bits [see `Sort.h`].
 *
 * `kVectorSort` tells whether the sort is that of Highway, and
 * `ADS_VECTOR_SORT` set to 0 takes the portable code with Highway too. The
 * keys of the floating point numbers are the same on both ways, so that
 * the order is the same.
 *
 * `vectorQuickSort()` is not stable (equal integers cannot be told apart, but
 * \f$ -0 \f$ and \f$ +0 \f$ can). The contracts [see `Contract.h`]: the vector
 * ends with the same items sorted, an audit. The functions are defined in
 * `VectorQuickSort.tpp`.
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
 *      Software: Practice and Experience 27(8), 983-993, 1997,
 *      doi:10.1002/(SICI)1097-024X(199708)27:8\<983::AID-SPE117\>3.0.CO;2-\#
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
#include "ADS/memory/Prefetch.h"
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
 #ifdef ADS_WITH_HIGHWAY
  /// 1 if the sort is the quicksort of Highway on the vectors of the
  /// processor (with ADS_WITH_HIGHWAY), 0 if the portable code; it may be
  /// set to 0 before the include
  #define ADS_VECTOR_SORT 1
 #else
  /// 1 if the sort is the quicksort of Highway on the vectors of the
  /// processor (with ADS_WITH_HIGHWAY), 0 if the portable code; it may be
  /// set to 0 before the include
  #define ADS_VECTOR_SORT 0
 #endif
#endif

#if ADS_VECTOR_SORT
 #include <hwy/contrib/sort/vqsort.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// true if `vectorQuickSort()` runs in the vectors of the processor, by
/// the quicksort of Highway, false if in the arrays of the portable code
inline constexpr bool kVectorSort = ADS_VECTOR_SORT != 0;

/// the types that `vectorQuickSort()` sorts: integers and floating point
/// numbers of 32 or 64 bits
template< class T >
concept VectorKey =
  ( std::is_integral_v< T > || std::is_floating_point_v< T > ) &&
  ( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 8 ) ) &&
  ( ! std::is_same_v< T , bool > );

/// the quicksort of \f$ A \f$ in the lanes of the vector registers, by the
/// natural order (the total order of IEEE 754 for floating point numbers)
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{vectorQuickSort}( A ) \cr
/// 2 & \quad \mathrm{vectorQuickSort}( A , n )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< SortStore Store , VectorKey T = std::ranges::range_value_t<
			     std::remove_cvref_t< Store > > >
void vectorQuickSort( Store && A );

/// `vectorQuickSort()` above on a vector
template< VectorKey T >
void vectorQuickSort( std::vector< T > & A );

/// the sort of `vectorQuickSort()` above on \f$ a[ 0 .. n - 1 ] \f$
///
/// In pseudocode (after the quicksorts of Bramas, of Gueron and Krasnov and of
/// Wassenberg et al., beyond the book), \f$ W \f$ the lanes of a register and
/// the keys of floating point numbers their bits as signed integers:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{vqs}( a , \mathit{lo} , \mathit{hi} ,
///     \mathit{depth} ) \cr
/// 2 & \quad \textbf{while}\ \mathit{hi} - \mathit{lo} > 16 W\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ \mathit{depth} = 0\ \textbf{then}\ \text{heapsort }
///     a[ \mathit{lo} .. \mathit{hi} - 1 ];\ \textbf{return} \cr
/// 4 & \qquad p \gets \text{the median of } \max\{ 8 , W \} \text{ keys }
///     \text{spaced evenly} \cr
/// 5 & \qquad \text{partition by } p \text{, } W \text{ keys per compare and }
///     \text{per compressed store} \cr
/// 6 & \qquad \textbf{if}\ \text{the sample repeats } p\ \textbf{then}
///     \ \text{put the keys equal to } p \text{ apart} \cr
/// 7 & \qquad \mathrm{vqs}( \text{the smaller side} , \mathit{depth} - 1 );
///     \ \text{loop on the larger} \cr
/// 8 & \quad \text{sort } a[ \mathit{lo} .. \mathit{hi} - 1 ] \text{ by a }
///     \text{network in at most 16 registers} \cr
/// 9 & \textbf{procedure}\ \mathrm{vectorQuickSort}( a , n ) \cr
/// 10 & \quad \textbf{if}\ \text{few keys in a small range}\ \textbf{then}
///     \ \text{count them};\ \textbf{return} \cr
/// 11 & \quad \mathrm{vqs}( a , 0 , n , 2 \lfloor \log_2 n \rfloor ) \cr
/// 12 & \quad \textbf{if}\ \text{floating point}\ \textbf{then}
///     \ \text{reverse the block of the negative numbers}
/// \end{array}}
/// \f]
template< VectorKey T >
void vectorQuickSort( T * a , std::size_t n );

/*--------------------------------------------------------------------------*/
/*------------------------- THE SORTS OF A RANGE ---------------------------*/
/*--------------------------------------------------------------------------*/

/// `vectorQuickSort()` on the range \f$ A \f$ [see `SortRange`], the other
/// arguments as for a vector: in place on the span of its items if \f$ A \f$
/// is contiguous, through a vector of its items otherwise, since the code of
/// the sort reads the items by their addresses
template< SortRange R , class... P >
 requires requires( std::vector< std::ranges::range_value_t< R > > & v ,
		    P &&... p )
{
  vectorQuickSort( v , std::forward< P >( p )... );
  }
decltype( auto ) vectorQuickSort( R && A , P &&... p )
{
 const auto f = [ & ]( auto & v ) -> decltype( auto ) {
  return( vectorQuickSort( v , std::forward< P >( p )... ) );
  };
 return( detail::sortRange( A , f ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/VectorQuickSort.tpp"

/*--------------------------------------------------------------------------*/

#endif /* VectorQuickSort.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File VectorQuickSort.h -----------------------*/
/*--------------------------------------------------------------------------*/
