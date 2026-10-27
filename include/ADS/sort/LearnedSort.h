/*--------------------------------------------------------------------------*/
/*--------------------------- File LearnedSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The learned sort, beyond the book: `learnedSort()` distributes the numbers
 * of 32 or 64 bits into buckets by a model of the cumulative distribution
 * function (CDF) of their keys, learned from a sample, and sorts each bucket
 * by `vectorQuickSort()` [see `VectorQuickSort.h`]; it falls back to
 * `sampleSort()` [see `SampleSort.h`] when the sample shows a distribution
 * that the model does not fit. The parallel sorts of `ParallelSort.h` have
 * the same model as the classifier of their distribution, in
 * `learnedSort( ex , A )` and `learnedStableSort( ex , A )`. The order is the
 * natural one, the total order of IEEE 754 for the floating point numbers
 * (\f$ -0 \f$ before \f$ +0 \f$, the NaN at the ends), as in `sort()` [see
 * `Sort.h`], whose result `learnedSort()` gives bit for bit.
 *
 * A distribution into \f$ k \f$ buckets is balanced when the bucket of
 * \f$ x \f$ is \f$ \lfloor k F( x ) \rfloor \f$, \f$ F \f$ the CDF of the
 * keys; the model replaces \f$ F \f$ by the empirical CDF \f$ F_s \f$ of a
 * random sample of \f$ s = 16 k \f$ keys, which the inequality of Dvoretzky,
 * Kiefer and Wolfowitz keeps within a small multiple of \f$ 1 / \sqrt{s} \f$
 * of \f$ F \f$ with high probability, whatever \f$ F \f$. The splitters
 * \f$ t_1 \le \cdots \le t_{k-1} \f$ are the quantiles \f$ i / k \f$ of the
 * sorted sample, and the bucket of \f$ x \f$ is the number of splitters less
 * than \f$ x \f$, which samplesort finds by a search of \f$ \log_2 k \f$
 * steps. The model finds it instead by a grid of cells of equal width
 * \f$ w \f$, a power of 2, on the range \f$ [ S_{\min} , S_{\max} ] \f$ of
 * the sample \f$ S \f$: the cell of \f$ x \f$ is
 * \f$ \lfloor ( x - S_{\min} ) / w \rfloor \f$, a shift, and each cell keeps
 * the number of splitters before it, i.e., a histogram of the sample and its
 * prefix sums, a piecewise constant model of the CDF. If every cell holds at
 * most \f$ c_{\max} \f$ (`kCellSplitters`) splitters, the bucket of
 * \f$ x \f$ is the count of its cell plus \f$ c_{\max} \f$ comparisons with
 * no branch, \f$ O( 1 ) \f$ time instead of \f$ \log_2 k \f$ dependent
 * loads, which is where the learned sorts of Kristo et al. and of Ferragina
 * and Odorisio gain on samplesort (their models are linear splines and the
 * recursive model index of the learned indexes; the grid is the simplest
 * model with the same guarantee).
 *
 * The model fails when the keys are not spread on the range of the sample,
 * i.e., for the laws with a heavy tail (Zipf, Pareto) and for clusters far
 * apart, where some cell of the finest grid allowed holds more than
 * \f$ c_{\max} \f$ splitters, and when the sample repeats its quantiles,
 * i.e., for few distinct keys. Both are seen on the sample before any key
 * moves: with no grid of at most \f$ c_{\max} \f$ splitters per cell, or
 * fewer than \f$ k / 2 \f$ distinct splitters, `learnedSort()` leaves
 * \f$ A \f$ to `sampleSort()`, whose buckets of equal keys take care of the
 * duplicates. A sample that repeats a splitter only in part gives each
 * repeated splitter a bucket of the keys equal to it, which needs no sort
 * (the buckets of equality of IPS4o, as in LearnedSort 2.0). The argument
 * behind the model and its costs is on the theory page of the area.
 *
 * Costs: \f$ O( k \log k ) \f$ for the sample and the model, \f$ O( n ) \f$
 * for the classification and the moves, and the sorts of the buckets,
 * \f$ O( n \log( n / k ) ) \f$ in expectation when the model fits; the extra
 * room is a buffer of \f$ n \f$ items and \f$ n \f$ bucket numbers of 16
 * bits.
 *
 * `sort()` does not choose `learnedSort()`: on the measures of the benchmark
 * `vectorQuickSort()`, which `sort()` uses for these keys when the vector
 * registers are there, sorted the same inputs faster, since its passes in
 * the registers cost less than the scattered moves of a distribution; nor do
 * the parallel sorts take this classifier in place of their sample tree,
 * which on the measures it did not beat beyond the noise. The learned sorts
 * remain for the inputs where the model shines (keys of a smooth law, many
 * buckets) and as the reference of the technique.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit. The functions are defined in `LearnedSort.tpp`.
 *
 * \see A. Kristo, K. Vaidya, U. Çetintemel, S. Misra, T. Kraska, The case
 *      for a learned sorting algorithm, Proceedings of SIGMOD 2020,
 *      1001-1016, 2020, doi:10.1145/3318464.3389752
 * \see A. Kristo, K. Vaidya, T. Kraska, Defeating duplicates: a re-design
 *      of the LearnedSort algorithm, 2021, doi:10.48550/arXiv.2107.03290
 * \see P. Ferragina, M. Odorisio, Fast, robust, and learned
 *      distribution-based sorting, IEEE Access 13, 45198-45214, 2025,
 *      doi:10.1109/ACCESS.2025.3549626
 * \see I. Carvalho, Towards parallel learned sorting, 2022,
 *      doi:10.48550/arXiv.2208.06902
 * \see A. Sato, Y. Matsui, PCF learned sort: a learning augmented sort
 *      algorithm with \f$ O( n \log \log n ) \f$ expected complexity, 2024,
 *      doi:10.48550/arXiv.2405.07122
 * \see P. Massart, The tight constant in the Dvoretzky-Kiefer-Wolfowitz
 *      inequality, The Annals of Probability 18(3), 1269-1283, 1990,
 *      doi:10.1214/aop/1176990746
 * \see M. Axtmann, S. Witt, D. Ferizovic, P. Sanders, Engineering in-place
 *      (shared-memory) sorting algorithms, ACM Transactions on Parallel
 *      Computing 9(1), 1-62, 2022, doi:10.1145/3505286
 * \see J. Wassenberg, M. Blacher, J. Giesen, P. Sanders, Vectorized and
 *      performance-portable quicksort, Software: Practice and Experience
 *      52(12), 2684-2699, 2022, doi:10.1002/spe.3142
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

#ifndef __ADS_LearnedSort
 #define __ADS_LearnedSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/Sorted.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the learned sort of \f$ A \f$ in the natural order (the total order of
/// IEEE 754 for the floating point numbers): the buckets of a model of the
/// CDF of a sample, each sorted by `vectorQuickSort()`, and `sampleSort()`
/// when the sample shows a distribution that the model does not fit; not
/// stable
///
/// In pseudocode (the learned sort of Kristo et al., with the model of the
/// grid, beyond the book), \f$ b = 4096 \f$ the items that a bucket aims at
/// and \f$ c_{\max} = 4 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{learnedSort}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad \textbf{if}\ n < 4 b\ \textbf{then}\ \mathrm{sampleSort}( A
///     );\ \textbf{return} \cr
/// 3 & \quad k \gets \min( \max( 2^{\lfloor \log_2( n / b ) \rfloor} , 64 )
///     , 2048 ) \cr
/// 4 & \quad S \gets \text{the sorted keys of } \min( n , 16 k ) \text{ }
///     \text{items of } A \text{ at random} \cr
/// 5 & \quad t \gets \text{the distinct keys among } S[ i |S| / k ]
///     \text{ with } 1 \le i < k \cr
/// 6 & \quad \textbf{for}\ g \gets \log_2 k + 2\ \textbf{to}\ \log_2 k + 6
///     \ \textbf{do} \cr
/// 7 & \qquad w \gets 2^{\max( 0 , \lceil \log_2( S_{\max} - S_{\min} )
///     \rceil - g )} \cr
/// 8 & \qquad \mathit{first}[ \gamma ] \gets |\{ j : \lfloor ( t_j - S_{\min}
///     ) / w \rfloor < \gamma \}| \text{ for each cell } \gamma \cr
/// 9 & \qquad \textbf{if}\ \text{no cell holds more than } c_{\max} \text{ }
///     \text{splitters}\ \textbf{then}\ \textbf{break} \cr
/// 10 & \quad \textbf{if}\ \text{the model does not fit}\ \textbf{or}\ |t| <
///     k / 2\ \textbf{then} \cr
/// 11 & \qquad \mathrm{sampleSort}( A );\ \textbf{return} \cr
/// 12 & \quad \textbf{foreach}\ x \in A\ \textbf{do}\quad \triangleright
///     \ \text{one pass, the buckets counted} \cr
/// 13 & \qquad \gamma \gets \text{the cell of } x \cr
/// 14 & \qquad \beta( x ) \gets \mathit{first}[ \gamma ] + |\{ 0 \le i <
///     c_{\max} : t_{\mathit{first}[ \gamma ] + i} < x \}| \cr
/// 15 & \quad \text{move the items to a buffer by buckets, stably, and }
///     \text{back to } A \cr
/// 16 & \quad \textbf{foreach}\ \text{bucket } \beta\ \textbf{do}
///     \ \mathrm{vectorQuickSort}( \beta )
/// \end{array}}
/// \f]
/// The splitters are padded by \f$ c_{\max} \f$ copies of the greatest key,
/// since line 14 reads past the last cell, and a splitter of a later cell is
/// greater than \f$ x \f$, so that the count is exact. If the sample repeats
/// a splitter, the bucket of line 14 becomes
/// \f$ 2 \beta + [ t_\beta = x ] \f$, the odd buckets holding the keys equal
/// to a splitter, which are not sorted.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< VectorKey T >
void learnedSort( std::vector< T > & A );

namespace detail {

/// `learnedSort()` of \f$ A \f$ if the sample shows a distribution that the
/// model fits, and then true; false, with \f$ A \f$ unchanged, otherwise
template< VectorKey T >
bool learnedSortIf( std::vector< T > & A );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/LearnedSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LearnedSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File LearnedSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
