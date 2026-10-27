/*--------------------------------------------------------------------------*/
/*---------------------------- File PGMIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The piecewise geometric model index of Ferragina and Vinciguerra, beyond
 * the book: a learned index on a sorted array of arithmetic keys, which
 * finds the place of the first item not smaller than a key [see
 * `StaticSearch.h` for the static B+-tree] in a space that depends on the
 * regularity of the keys rather than on their number.
 *
 * The items \f$ A[ 0 .. n ) \f$ are the points \f$ ( A[ i ] , i ) \f$ of the
 * plane, a repeated key standing for its first place. An
 * \f$ \epsilon \f$-approximation is a sequence of segments, each a line
 * \f$ f( x ) = a + b ( x - k_s ) \f$ over a range of keys from its first key
 * \f$ k_s \f$, such that \f$ | f( A[ i ] ) - i | \le \epsilon \f$ for every
 * key of its range; the segments are those of the optimal approximation, the
 * fewest possible, built in one pass in \f$ O( n ) \f$ time by the algorithm
 * of O'Rourke as engineered by Xie et al.: the lines through the points of
 * the range with error at most \f$ \epsilon \f$ are those between the line
 * of least slope and the line of largest slope that the convex hulls of the
 * points moved up and down by \f$ \epsilon \f$ admit, the two hulls updated
 * by a new point in amortized \f$ O( 1 ) \f$, and a range ends when a point
 * leaves no line. The segment keeps the line of the average of the two
 * extreme slopes through the point where they cross (any line of slope
 * between them through that point stays within the error), its slope made
 * not negative so that the prediction never decreases. The first keys of the
 * segments are themselves approximated, with an error \f$ \epsilon_r \f$, by
 * the segments of a level above, and so on up to a single segment: for
 * \f$ m \f$ segments at the level below, a level has at most
 * \f$ m / ( 2 \epsilon_r ) \f$ of them (a segment of the optimal
 * approximation covers at least \f$ 2 \epsilon \f$ points, since the
 * horizontal line through the middle of any \f$ 2 \epsilon \f$ consecutive
 * ranks is within the error), the levels are
 * \f$ 1 + \lceil \log_{2 \epsilon_r} m \rceil \f$ at most, and the index
 * takes 28 bytes per segment (its first key, the two doubles of its line and
 * its first place), \f$ m \f$ at most \f$ n / ( 2 \epsilon ) \f$ and much
 * fewer on regular keys.
 *
 * A search starts from the segment at the top, and at each level the
 * prediction of its line, clamped to the range of the segment, is the place
 * of the next segment within \f$ \pm ( \epsilon_r + 1 ) \f$ places (one more
 * for the rounding of the line in `double`), found by a binary search with
 * no branch of the last first key not larger than the key; at the last level
 * the prediction is the place of the answer within
 * \f$ \pm ( \epsilon + 2 ) \f$, whose lines of cache are all prefetched at
 * once, so that their misses overlap, and searched in the same way:
 * \f$ O( \log \epsilon_r \log_{2 \epsilon_r} m + \log \epsilon ) \f$ time.
 * The items are a copy on huge pages [see `HugePageAllocator.h`], so that
 * the window of a search costs no miss of the TLB of its own. On repeated
 * keys, whose places a line cannot all approximate, the search goes on past
 * the window by galloping. The keys are mapped to integers of 64 bits in the
 * same order (the signed by flipping the top bit, the floating by their
 * bits), so that the hulls are kept with exact products of 128 bits.
 *
 * The contracts [see `Contract.h`]: the keys sorted, an audit;
 * \f$ \epsilon \ge 1 \f$, \f$ \epsilon_r \ge 1 \f$; each result \f$ r \f$ is
 * such that the items before \f$ r \f$ are smaller than the key and those
 * from \f$ r \f$ on are not, a postcondition. The functions are defined in
 * `PGMIndex.tpp`.
 *
 * \see P. Ferragina, G. Vinciguerra, The PGM-index: a fully-dynamic
 *      compressed learned index with provable worst-case bounds, Proceedings
 *      of the VLDB Endowment 13(8), 1162-1175, 2020,
 *      doi:10.14778/3389133.3389135
 * \see J. O'Rourke, An on-line algorithm for fitting straight lines between
 *      data ranges, Communications of the ACM 24(9), 574-578, 1981,
 *      doi:10.1145/358746.358758
 * \see Q. Xie, C. Pang, X. Zhou, X. Zhang, K. Deng, Maximum error-bounded
 *      piecewise linear representation for online stream approximation,
 *      The VLDB Journal 23(6), 915-937, 2014, doi:10.1007/s00778-014-0355-0
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

#ifndef __ADS_PGMIndex
 #define __ADS_PGMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/memory/HugePageAllocator.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS PGMIndex ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the PGM-index on a sorted array of arithmetic keys `K`, with the error
/// \f$ \epsilon = E \f$ at the last level and \f$ \epsilon_r = R \f$ at the
/// others
template< class K , std::size_t E = 64 , std::size_t R = 4 >
 requires std::is_arithmetic_v< K >
class PGMIndex {

 static_assert( ( E >= 1 ) && ( R >= 1 ) , "the errors are at least 1" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = K;   ///< the type of the items

 /// an empty index
 PGMIndex( void ) = default;

 /// the index on the sorted items of \f$ A \f$, a level of segments of
 /// error \f$ \epsilon \f$ on the distinct keys and their first places and
 /// the levels of error \f$ \epsilon_r \f$ above it
 ///
 /// In pseudocode (the optimal approximation of O'Rourke as engineered by
 /// Xie et al., beyond the book), for the points \f$ ( x_i , y_i ) \f$ of a
 /// level:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{approximate}( x , y , e ) \cr
 /// 2 & \quad L \gets \text{an empty level};\ \text{start a segment at }
 ///     ( x_0 , y_0 ) \cr
 /// 3 & \quad \textbf{foreach}\ i \ge 1\ \textbf{do} \cr
 /// 4 & \qquad u \gets ( x_i , y_i + e );\ d \gets ( x_i , y_i - e ) \cr
 /// 5 & \qquad \textbf{if}\ \text{no line between the hulls passes }
 ///     \text{between } u \text{ and } d\ \textbf{then} \cr
 /// 6 & \qquad\quad \text{close the segment into } L;\ \text{start one at }
 ///     ( x_i , y_i );\ \textbf{continue} \cr
 /// 7 & \qquad \text{update the lines of least and of largest slope}
 ///     \text{ with } u \text{ and } d \cr
 /// 8 & \qquad \text{add } u \text{ to the lower hull of the ups and } d
 ///     \text{ to the upper hull of the downs} \cr
 /// 9 & \quad \text{close the last segment into } L;\ \textbf{return}\ L \cr
 /// 10 & \textbf{procedure}\ \mathrm{build}( A ) \cr
 /// 11 & \quad ( x , y ) \gets \text{the distinct keys of } A \text{ and }
 ///     \text{their first places} \cr
 /// 12 & \quad L_0 \gets \mathrm{approximate}( x , y , \epsilon ) \cr
 /// 13 & \quad \textbf{while}\ L_h \text{ has more than one segment}
 ///     \ \textbf{do} \cr
 /// 14 & \qquad L_{h+1} \gets \mathrm{approximate}( \text{the first keys }
 ///     \text{of } L_h , 0 , 1 , 2 , \ldots , \epsilon_r )
 /// \end{array}}
 /// \f]
 /// where closing a segment keeps the line through the crossing of the two
 /// extreme lines with the average of their slopes, not negative.
 /// \pre \f$ A \f$ is sorted (audit), \f$ n < 2^{32} \f$
 explicit PGMIndex( std::vector< K > A );

 /// the first place of an item not smaller than \f$ v \f$, `size()` if
 /// none
 ///
 /// In pseudocode (Ferragina and Vinciguerra, beyond the book), the levels
 /// \f$ L_{H-1} , \ldots , L_0 \f$ from the top,
 /// \f$ \mathrm{predict}( L , s , x ) \f$ the line of the segment \f$ s \f$
 /// of \f$ L \f$ at \f$ x \f$, clamped to its range, and the places
 /// <em>near</em> \f$ p \f$ those in
 /// \f$ [ p - \epsilon_r - 2 , p + \epsilon_r + 2 ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( v ) \cr
 /// 2 & \quad \textbf{if}\ n = 0 \text{ or } v < A[ 0 ]
 ///     \ \textbf{then return}\ 0 \cr
 /// 3 & \quad s \gets 0 \cr
 /// 4 & \quad \textbf{for}\ h \gets H - 1\ \textbf{downto}\ 1\ \textbf{do}
 ///     \cr
 /// 5 & \qquad p \gets \mathrm{predict}( L_h , s , v ) \cr
 /// 6 & \qquad s \gets \text{the last segment of } L_{h-1} \text{ near }
 ///     p \text{ with first key not larger than } v \cr
 /// 7 & \quad p \gets \mathrm{predict}( L_0 , s , v ) \cr
 /// 8 & \quad r \gets \mathrm{lowerBound}( A , v , p - \epsilon - 2 , p +
 ///     \epsilon + 3 )\quad \triangleright\ \text{prefetched, no branch} \cr
 /// 9 & \quad \textbf{if}\ r \text{ is an end of the window and not the }
 ///     \text{answer}\ \textbf{then} \cr
 /// 10 & \qquad r \gets \text{the answer by galloping out of the window} \cr
 /// 11 & \quad \textbf{return}\ r
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

 /// the segments of the last level
 [[nodiscard]] std::size_t segments( void ) const;

 /// the levels of segments
 [[nodiscard]] int height( void ) const;

 /// the bytes of the index, the items aside
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the line of a segment: the predicted place of its first key and the
 /// slope
 struct Line {
  double icpt;    ///< the predicted place of the first key
  double slope;   ///< the slope
  };

 /// a level: the first keys of its segments, their lines, and the first
 /// place of each segment at the level below (and their number, at the
 /// end)
 struct Level {
  std::vector< std::uint64_t > key;      ///< the first keys
  std::vector< Line > line;              ///< the lines
  std::vector< std::uint32_t > start;    ///< the first places below
  };

 /// the key in an order of integers of 64 bits
 [[nodiscard]] static std::uint64_t ord( const K & k );

 /// the segments of the points \f$ ( x[ i ] , y[ i ] ) \f$ with error
 /// \f$ e \f$
 static Level approximate( const std::vector< std::uint64_t > & x ,
                           const std::vector< std::uint32_t > & y ,
                           std::int64_t e , std::size_t total );

 /// the place predicted by the segment \f$ s \f$ of \f$ l \f$ for \f$ x \f$,
 /// within its range
 [[nodiscard]] static std::size_t predict( const Level & l , std::size_t s ,
                                           std::uint64_t x );

 std::vector< K , HugePageAllocator< K > > f_A;   ///< the items
 std::vector< Level > f_L;              ///< the levels, the last first

 };  // end( class PGMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/PGMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PGMIndex.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File PGMIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
