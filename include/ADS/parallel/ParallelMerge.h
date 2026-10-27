/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelMerge.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel merge of \f$ k \f$ sorted ranges, beyond the book:
 * `multiwayMerge( ex , R , out , less )` writes into `out` what
 * `multiwayMerge( R , out , less )` [see `MultiwayMerge.h`] writes, the same
 * items in the same order (on equal items the range of lower index first),
 * on the threads of the executor `ex` [see `Executor.h`].
 *
 * The output of \f$ N \f$ items is cut into \f$ B \f$ parts of about
 * \f$ N / B \f$ items, \f$ B = 4 p \f$ for \f$ p \f$ threads (at least
 * \f$ 2^{14} \f$ items each), and the part that starts at rank \f$ r \f$
 * takes from each range \f$ i \f$ the items of ranks
 * \f$ [ s_i( r ) , s_i( r' ) ) \f$, where \f$ s( r ) \f$ is the exact
 * multisequence selection: the split with \f$ \sum_i s_i( r ) = r \f$ that
 * leaves on its left the \f$ r \f$ least items in the total order of the
 * triples \f$ ( \mathit{item} , \mathit{range} , \mathit{position} ) \f$,
 * which is the order of the stable merge. Each part is then merged by the
 * sequential `multiwayMerge()` into its place of the output, in parallel,
 * so that no part waits for another. A split is found by narrowing a window
 * \f$ [ \mathit{lo}_i , \mathit{hi}_i ) \f$ on each range: the pivot is the
 * middle item \f$ x \f$ of the widest window, of range \f$ j \f$ and
 * position \f$ m \f$, and a binary search in each window counts the items
 * before \f$ ( x , j , m ) \f$, those \f$ \le x \f$ in the ranges
 * \f$ i < j \f$ and those \f$ < x \f$ in the ranges \f$ i > j \f$; if they
 * are fewer than \f$ r \f$, \f$ x \f$ and all of them go to the left (each
 * \f$ \mathit{lo}_i \f$ rises to its count, \f$ \mathit{lo}_j \f$ past
 * \f$ x \f$), otherwise to the right. The pivot is the middle item of a
 * window, the weighted median of the middles of all the windows by their
 * widths: the windows whose middles are on the side of \f$ x \f$ that goes
 * lose half of their width, at least a quarter of the total, so that a split
 * takes \f$ O( \log( k N ) ) \f$ steps of \f$ k \f$ binary searches and a
 * sort of \f$ k \f$ middles, \f$ O( k \log N \log( k N ) ) \f$ comparisons
 * (the bound of the selection of Varman et al., up to a factor
 * \f$ \log k \f$); the splits are found in parallel, one per part, and cost
 * far less than the merge, \f$ O( N \log k ) \f$.
 *
 * The contracts [see `Contract.h`]: the iterators are random access, each
 * range sorted by `less` (an audit, as in the sequential merge), `out` does
 * not overlap the ranges; the output sorted, with the items of the ranges
 * (an audit). The functions are defined in `ParallelMerge.tpp`.
 *
 * \see P. J. Varman, S. D. Scheufler, B. R. Iyer, G. R. Ricard, Merging
 *      multiple lists on hierarchical-memory multiprocessors, Journal of
 *      Parallel and Distributed Computing 12(2), 171-177, 1991,
 *      doi:10.1016/0743-7315(91)90022-2
 * \see J. Singler, P. Sanders, F. Putze, MCSTL: the multi-core standard
 *      template library, Euro-Par 2007 Parallel Processing, LNCS 4641,
 *      682-694, 2007, doi:10.1007/978-3-540-74466-5_72
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

#ifndef __ADS_ParallelMerge
 #define __ADS_ParallelMerge
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/priority/MultiwayMerge.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the split of the sorted ranges \f$ R_1 , \ldots , R_k \f$ at the rank
/// \f$ r \f$ of the stable merge, i.e., the number of items that each range
/// gives to the \f$ r \f$ least
///
/// In pseudocode (the multisequence selection of Varman et al., beyond the
/// book), \f$ R_i[ 0 .. |R_i| - 1 ] \f$ and the items ordered by
/// \f$ ( \mathit{item} , \mathit{range} , \mathit{position} ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multisequenceSplit}( R_1 , \ldots , R_k ,
///     r ) \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ \mathit{lo}_i \gets 0;
///     \ \mathit{hi}_i \gets |R_i| \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad C \gets \{ i : \mathit{lo}_i < \mathit{hi}_i \};\ \textbf{if}
///     \ C = \emptyset\ \textbf{then return}\ \mathit{lo} \cr
/// 5 & \qquad \textbf{foreach}\ i \in C\ \textbf{do}\ m_i \gets
///     \mathit{lo}_i + \lfloor ( \mathit{hi}_i - \mathit{lo}_i ) / 2 \rfloor
///     \cr
/// 6 & \qquad j \gets \text{the median of } C \text{ by } ( R_i[ m_i ] , i )
///     \text{, weighted by } \mathit{hi}_i - \mathit{lo}_i \cr
/// 7 & \qquad x \gets R_j[ m_j ];\ c_j \gets m_j \cr
/// 8 & \qquad \textbf{foreach}\ i < j\ \textbf{do}\ c_i \gets \text{the }
///     \text{first place of } [ \mathit{lo}_i , \mathit{hi}_i ) \text{ with }
///     R_i[ c_i ] > x \cr
/// 9 & \qquad \textbf{foreach}\ i > j\ \textbf{do}\ c_i \gets \text{the }
///     \text{first place of } [ \mathit{lo}_i , \mathit{hi}_i ) \text{ with }
///     R_i[ c_i ] \ge x \cr
/// 10 & \qquad \textbf{if}\ \sum_i c_i < r\ \textbf{then}\ \mathit{lo} \gets
///     c;\ \mathit{lo}_j \gets m_j + 1\quad \triangleright\ x \text{ to the }
///     \text{left} \cr
/// 11 & \qquad \textbf{else}\ \mathit{hi} \gets c;\ \mathit{hi}_j \gets m_j
/// \end{array}}
/// \f]
/// \pre each range sorted by `less`, \f$ r \le \sum_i |R_i| \f$
template< std::random_access_iterator I , class C = std::less<> >
[[nodiscard]] std::vector< std::size_t > multisequenceSplit(
                         const std::vector< std::pair< I , I > > & R ,
                         std::size_t r , C less = C() );

/// the items of the sorted ranges \f$ R_1 , \ldots , R_k \f$ merged into
/// `out` on the threads of `ex`, the range of lower index first on equal
/// items; the end of the output
///
/// In pseudocode (the parallel merge of Varman et al., beyond the book), on
/// \f$ p \f$ threads:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{multiwayMerge}( \mathit{ex} , R_1 , \ldots
///     , R_k , \mathit{out} ) \cr
/// 2 & \quad N \gets \sum_i |R_i| \cr
/// 3 & \quad \textbf{if}\ p = 1\ \textbf{or}\ k < 2\ \textbf{or}\ N <
///     2^{16}\ \textbf{then return}\ \mathrm{multiwayMerge}( R_1 , \ldots ,
///     R_k , \mathit{out} ) \cr
/// 4 & \quad B \gets \max\{ 2 , \min\{ 4 p , \lfloor N / 2^{14} \rfloor \}
///     \} \cr
/// 5 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ B
///     \ \textbf{do in parallel}\ S[ q ] \gets \mathrm{multisequenceSplit}(
///     R_1 , \ldots , R_k , \lfloor N q / B \rfloor ) \cr
/// 6 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ B - 1
///     \ \textbf{do in parallel} \cr
/// 7 & \qquad \mathrm{multiwayMerge}( \text{the pieces } R_i[ S[ q ]_i ..
///     S[ q + 1 ]_i - 1 ] \text{ of each } i , \mathit{out} + \lfloor N q / B
///     \rfloor ) \cr
/// 8 & \quad \textbf{return}\ \mathit{out} + N
/// \end{array}}
/// \f]
/// \pre each range sorted by `less` (an audit), `out` not overlapping them
/// \post the output sorted, with the items of the ranges (an audit)
template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class C = std::less<> >
O multiwayMerge( E & ex , const std::vector< std::pair< I , I > > & R ,
                 O out , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelMerge.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ParallelMerge.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelMerge.h ------------------------*/
/*--------------------------------------------------------------------------*/
