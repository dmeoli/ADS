/*--------------------------------------------------------------------------*/
/*---------------------------- File PGMIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The piecewise geometric model index of Ferragina and Vinciguerra, beyond
 * the book: a learned index on a sorted array of arithmetic keys, which
 * finds the place of the first item not smaller than a key [see
 * StaticSearch.h for the static B+-tree] in a space that depends on the
 * regularity of the keys rather than on their number.
 *
 * The items A[ 0 .. n ) are the points \f$ ( A[ i ] , i ) \f$ of the plane,
 * a repeated key standing for its first place. An \f$ \epsilon
 * \f$-approximation is a sequence of segments, each a line \f$ f( x ) = a
 * + b ( x - k_s ) \f$ over a range of keys from its first key \f$ k_s \f$,
 * such that \f$ | f( A[ i ] ) - i | \le \epsilon \f$ for every key of its
 * range; the segments are those of the optimal approximation, the fewest
 * possible, built in one pass in O( n ) time by the algorithm of O'Rourke
 * as engineered by Xie et al.: the lines through the points of the range
 * with error at most \f$ \epsilon \f$ are those between the line of least
 * slope and the line of largest slope that the convex hulls of the points
 * moved up and down by \f$ \epsilon \f$ admit, the two hulls updated by a
 * new point in amortized O( 1 ), and a range ends when a point leaves no
 * line. The segment keeps the line of the average of the two extreme
 * slopes through the point where they cross (any line of slope between
 * them through that point stays within the error), its slope made not
 * negative so that the prediction never decreases. The first keys of the
 * segments are themselves approximated, with an error \f$ \epsilon_r \f$,
 * by the segments of a level above, and so on up to a single segment: for
 * m segments at the level below, a level has at most \f$ m / ( 2
 * \epsilon_r ) \f$ of them (a segment of the optimal approximation covers at
 * least \f$ 2 \epsilon \f$ points, since the horizontal line through the
 * middle of any \f$ 2 \epsilon \f$ consecutive ranks is within the error),
 * the levels are \f$ 1 + \lceil \log_{2 \epsilon_r} m \rceil \f$ at most,
 * and the index takes 28 bytes per segment (its first key, the two doubles of
 * its line and its first place), m at most \f$ n / ( 2
 * \epsilon ) \f$ and much fewer on regular keys.
 *
 * A search starts from the segment at the top, and at each level the
 * prediction of its line, clamped to the range of the segment, is the
 * place of the next segment within \f$ \pm ( \epsilon_r + 1 ) \f$ places
 * (one more for the rounding of the line in double), found by a binary
 * search with no branch of the last first key not larger than the key;
 * at the last level the prediction is the place of the answer within \f$
 * \pm ( \epsilon + 2 ) \f$, whose lines of cache are all prefetched at
 * once, so that their misses overlap, and searched in the same way: \f$
 * O( \log \epsilon_r \log_{2 \epsilon_r} m + \log \epsilon ) \f$ time.
 * The items are a copy on huge pages [see HugePageAllocator.h], so that the
 * window of a search costs no miss of the TLB of its own. On repeated keys,
 * whose places a line cannot all approximate, the search goes on past the
 * window by galloping. The keys are mapped to integers of 64 bits in the same order
 * (the signed by flipping the top bit, the floating by their bits), so that
 * the hulls are kept with exact products of 128 bits.
 *
 * The contracts [see Contract.h]: the keys sorted, an audit; \f$ \epsilon
 * \ge 1 \f$, \f$ \epsilon_r \ge 1 \f$; each result r is such that the items
 * before r are smaller than the key and those from r on are not, a
 * postcondition. The functions are defined in PGMIndex.tpp.
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
 *         Universita' di Pisa \n
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

/// the PGM-index on a sorted array of arithmetic keys K, with the error
/// E at the last level and R at the others
template< class K , std::size_t E = 64 , std::size_t R = 4 >
 requires std::is_arithmetic_v< K >
class PGMIndex {

 static_assert( ( E >= 1 ) && ( R >= 1 ) , "the errors are at least 1" );

/*--------------------------------------------------------------------------*/
 public:

 using Item = K;   ///< the type of the items

 PGMIndex( void ) = default;

 /// the index on the sorted items of A
 /// \pre A is sorted (an audit)
 explicit PGMIndex( std::vector< K > A );

 /// the first place of an item not smaller than v, size() if none
 [[nodiscard]] std::size_t lowerBound( const K & v ) const;

 [[nodiscard]] bool contains( const K & v ) const;

 /// the item of place i
 [[nodiscard]] const K & operator[]( std::size_t i ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the segments of the last level
 [[nodiscard]] std::size_t segments( void ) const;

 /// the levels of segments
 [[nodiscard]] int height( void ) const;

 /// the bytes of the index, the items aside
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the line of a segment: the predicted place of its first key, the
 /// slope, and the first place of the segment
 struct Line {
  double icpt;
  double slope;
  };

 /// a level: the first keys of its segments, their lines, and the first
 /// place of each segment at the level below (and their number, at the
 /// end)
 struct Level {
  std::vector< std::uint64_t > key;
  std::vector< Line > line;
  std::vector< std::uint32_t > start;
  };

 /// the key in an order of integers of 64 bits
 [[nodiscard]] static std::uint64_t ord( const K & k );

 /// the segments of the points ( x[ i ] , y[ i ] ) with error e
 static Level approximate( const std::vector< std::uint64_t > & x ,
                           const std::vector< std::uint32_t > & y ,
                           std::int64_t e , std::size_t total );

 /// the place predicted by the segment s of l for x, within its range
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
