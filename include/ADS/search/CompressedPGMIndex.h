/*--------------------------------------------------------------------------*/
/*------------------------ File CompressedPGMIndex.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed PGM-index of Ferragina and Vinciguerra, beyond the book:
 * the segments of the PGM-index [see `PGMIndex.h`] on a sorted array of
 * arithmetic keys in a few tens of bits each instead of 28 bytes, with the
 * same search at the last level (see \ref theory_search_learned).
 *
 * A segment of the optimal approximation with error \f$ \epsilon \f$ does
 * not need its own slope: the lines within the error of its points are
 * those of the slopes in an interval \f$ [ \alpha^- , \alpha^+ ] \f$, the
 * two extreme lines of the algorithm of O'Rourke, and each slope of the
 * interval has an intercept that keeps the line within the error (the
 * feasible lines are a convex region of the plane of the slopes and the
 * intercepts, and the interval is its shadow). The fewest slopes that hit
 * every interval are found by the greedy of the stabbing of intervals: the
 * intervals sorted by their left end, a group grows while the next interval
 * meets the intersection of the group, and the middle of the intersection
 * is the slope of the group; the intervals of least right end of the groups
 * are pairwise disjoint, hence no set of fewer slopes hits them all. A
 * segment then keeps the index of its slope in a table and, with that
 * slope, the intercept in the middle of its interval of feasible
 * intercepts, as its distance from the first place of the segment, which
 * is at most \f$ \epsilon + 1 \f$; the first keys and the first places of
 * the segments, increasing, are kept by Elias-Fano [see `EliasFano.h`]:
 * \f[
 *   2 + \Big\lceil \log_2 \frac{u}{m} \Big\rceil +
 *   2 + \Big\lceil \log_2 \frac{n}{m} \Big\rceil +
 *   \lceil \log_2 \sigma \rceil + \lceil \log_2( 2 \epsilon + 3 ) \rceil
 * \f]
 * bits per segment for \f$ m \f$ segments, \f$ \sigma \f$ slopes and keys
 * in a universe of size \f$ u \f$, plus the directories of Elias-Fano and the
 * table of the slopes, in single precision as in the implementation of the
 * paper, \f$ 32 \sigma \f$ bits.
 *
 * The paper keeps the recursive levels, each compressed in the same way;
 * here the levels above the last one are replaced by the rank of Elias-Fano
 * on the first keys, `nextGEQ()`, in expected \f$ O( 1 ) \f$ time on keys
 * spread over their universe [see `EliasFano.h`], so that a search costs a
 * rank, three selects, a field of bits and the window of
 * \f$ 2 \epsilon + 5 \f$ places of the PGM-index. A segment whose points
 * the line of the shared slope, rounded, leaves out by more than
 * \f$ \epsilon + 1 \f$ (an effect of the floating point, which the
 * construction checks) keeps a slope of its own, appended to the table,
 * with the intercept in the middle of those of that slope.
 *
 * The contracts [see `Contract.h`]: the keys sorted, an audit, and fewer
 * than \f$ 2^{32} \f$ of them; each result \f$ r \f$ is such that the items
 * before \f$ r \f$ are smaller than the key and those from \f$ r \f$ on are
 * not, a postcondition. The functions are defined in
 * `CompressedPGMIndex.tpp`.
 *
 * \see P. Ferragina, G. Vinciguerra, The PGM-index: a fully-dynamic
 *      compressed learned index with provable worst-case bounds, Proceedings
 *      of the VLDB Endowment 13(8), 1162-1175, 2020,
 *      doi:10.14778/3389133.3389135
 * \see J. O'Rourke, An on-line algorithm for fitting straight lines between
 *      data ranges, Communications of the ACM 24(9), 574-578, 1981,
 *      doi:10.1145/358746.358758
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

#ifndef __ADS_CompressedPGMIndex
 #define __ADS_CompressedPGMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS CompressedPGMIndex -------------------------*/
/*--------------------------------------------------------------------------*/

/// the compressed PGM-index on a sorted array of arithmetic keys `K`, with
/// the error \f$ \epsilon = E \f$
template< class K , std::size_t E = 64 >
 requires std::is_arithmetic_v< K >
class CompressedPGMIndex {

 static_assert( E >= 1 , "the error is at least 1" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = K;   ///< the type of the items

 /// an empty index
 CompressedPGMIndex( void ) = default;

 /// the index on the sorted items of \f$ A \f$
 ///
 /// In pseudocode (Ferragina and Vinciguerra, beyond the book), the
 /// segments \f$ g \f$ of the optimal approximation with their intervals
 /// of slopes \f$ [ g.\alpha^- , g.\alpha^+ ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( A ) \cr
 /// 2 & \quad G \gets \mathrm{approximate}( \text{the distinct keys of } A
 ///     , \text{their first places} , \epsilon )\quad \triangleright
 ///     \ \text{see } \mathrm{PGMIndex} \cr
 /// 3 & \quad S \gets \text{an empty table};\ I \gets [ -\infty , \infty ]
 ///     \cr
 /// 4 & \quad \textbf{foreach}\ g \in G \text{ by increasing }
 ///     g.\alpha^-\ \textbf{do}\quad \triangleright\ \text{the stabbing of }
 ///     \text{the intervals} \cr
 /// 5 & \qquad \textbf{if}\ g.\alpha^- > \max I\ \textbf{then}
 ///     \ \mathrm{append}( S , \text{the middle of } I );\ I \gets [
 ///     g.\alpha^- , g.\alpha^+ ] \cr
 /// 6 & \qquad \textbf{else}\ I \gets I \cap [ g.\alpha^- , g.\alpha^+ ] \cr
 /// 7 & \qquad g.\sigma \gets | S | \cr
 /// 8 & \quad \mathrm{append}( S , \text{the middle of } I ) \cr
 /// 9 & \quad \textbf{foreach}\ g \in G\ \textbf{do} \cr
 /// 10 & \qquad \alpha \gets S[ g.\sigma ];\ \beta \gets \text{the middle }
 ///     \text{of } [ \max_i ( y_i - \alpha x_i ) - \epsilon ,
 ///     \min_i ( y_i - \alpha x_i ) + \epsilon ] \text{ on its points} \cr
 /// 11 & \qquad \textbf{if}\ \text{the line } ( \alpha , \beta )
 ///     \text{ misses a point by more than } \epsilon + 1\ \textbf{then}
 ///     \ \text{give } g \text{ its own slope} \cr
 /// 12 & \qquad \text{write } g.\sigma \text{ and } \beta \text{ less its }
 ///     \text{first place in a field of bits} \cr
 /// 13 & \quad \text{the first keys and the first places of } G
 ///     \text{ by Elias-Fano}
 /// \end{array}}
 /// \f]
 /// \pre \f$ A \f$ is sorted (audit), \f$ n < 2^{32} \f$
 explicit CompressedPGMIndex( std::vector< K > A );

 /// the first place of an item not smaller than \f$ v \f$, `size()` if
 /// none
 ///
 /// In pseudocode (Ferragina and Vinciguerra, beyond the book), \f$ k_s \f$
 /// and \f$ p_s \f$ the first key and the first place of the segment
 /// \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( v ) \cr
 /// 2 & \quad \textbf{if}\ n = 0 \text{ or } v < A[ 0 ]
 ///     \ \textbf{then return}\ 0 \cr
 /// 3 & \quad s \gets \mathrm{nextGEQ}( \text{the first keys} , v + 1 ) - 1
 ///     \quad \triangleright\ \text{the last first key not larger than } v
 ///     \cr
 /// 4 & \quad p \gets p_s + \delta_s + S[ \sigma_s ] ( v - k_s )
 ///     \text{, clamped to } [ p_s , p_{s+1} ] \cr
 /// 5 & \quad \textbf{return}\ \mathrm{lowerBound}( A , v , p - \epsilon - 2
 ///     , p + \epsilon + 3 )\quad \triangleright\ \text{galloping out of }
 ///     \text{the window if needed}
 /// \end{array}}
 /// \f]
 /// \post \f$ A[ r - 1 ] < v \le A[ r ] \f$
 [[nodiscard]] std::size_t lowerBound( const K & v ) const;

 /// true if \f$ v \f$ is an item, by `lowerBound( v )`
 [[nodiscard]] bool contains( const K & v ) const;

 /// the item of place \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const K & operator[]( std::size_t i ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no items
 [[nodiscard]] bool empty( void ) const;

 /// the segments, \f$ m \f$
 [[nodiscard]] std::size_t segments( void ) const;

 /// the distinct slopes, \f$ \sigma \f$
 [[nodiscard]] std::size_t slopes( void ) const;

 /// the bytes of the index, the items aside
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the first key of the segment \f$ s \f$, in the order of `pgmOrd()`
 [[nodiscard]] std::uint64_t key( std::size_t s ) const;

 std::vector< K , HugePageAllocator< K > > f_A;   ///< the items
 std::size_t f_m = 0;              ///< the segments
 std::uint64_t f_k0 = 0;           ///< the first key
 bool f_top = false;               ///< the last first key, f_k0 + 2^64 - 1
 EliasFano f_key;                  ///< the first keys less f_k0
 EliasFano f_start;                ///< the first places, and n
 std::vector< float > f_slope;     ///< the table of the slopes
 BitVector f_code;                 ///< the slope and the intercept of each
 int f_ws = 0;                     ///< the bits of the index of a slope
 int f_wd = 0;                     ///< the bits of an intercept
 std::int64_t f_dmin = 0;          ///< the least intercept less its place

 };  // end( class CompressedPGMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/CompressedPGMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CompressedPGMIndex.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CompressedPGMIndex.h ---------------------*/
/*--------------------------------------------------------------------------*/
