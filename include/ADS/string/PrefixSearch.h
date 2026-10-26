/*--------------------------------------------------------------------------*/
/*-------------------------- File PrefixSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search by prefix in a sorted dictionary of n strings, beyond the
 * book: the strings prefixed by P are consecutive, between the
 * lexicographic position of P (the number of strings smaller than P) and
 * that of P#, # being a character larger than all the others, so that two
 * lexicographic searches give their range.
 *
 * - lexicographicPosition(), prefixEnd() and prefixRange() search the
 *   array of the strings by a binary search, \f$ O( p \log n ) \f$ time for
 *   a pattern of p characters, each step a comparison of strings: simple,
 *   but each step reads a string far from the others, i.e., \f$ O( ( p / B
 *   ) \log n ) \f$ transfers of blocks of B characters, which the two-level
 *   dictionaries [see FrontCodedDictionary and TwoLevelDictionary] reduce
 *   to a search on a sample of the strings and the scan of one bucket;
 * - BinnedSearch, the search by interpolation of Demaine, Jones and
 *   Patrascu in a sorted array X of m integers: \f$ [ x_1 , x_m ] \f$ is cut
 *   into bins of width \f$ b = \lceil ( x_m - x_1 + 1 ) / m \rceil \f$, an
 *   array gives where each bin starts in X, and y is searched by a binary
 *   search in its bin \f$ \lfloor ( y - x_1 ) / b \rfloor \f$; a bin holds
 *   at most \f$ \Delta \f$ items, \f$ \Delta \f$ being the ratio between
 *   the largest and the smallest gap of X, since b is at most the largest
 *   gap (Theorem 9.4 of the notes), so that a search takes \f$ O( \log
 *   \min( \Delta , m ) ) \f$ time in the worst case, never worse than the
 *   binary search, and \f$ O( \log \log m ) \f$ with high probability on
 *   uniform keys, as the search by interpolation [see
 *   InterpolationSearch.h] whose worst case is O( m ), at the price of the
 *   array of the bins;
 * - BinnedStringSearch, the same on strings, read as the integers of
 *   their first 8 characters (padded with the character 0), which keep
 *   their order: the bin of a pattern is found by its integer, and the
 *   binary search in the bin compares whole strings.
 *
 * The contracts [see Contract.h]: the array sorted, an audit (for the
 * strings by the order of std::string, i.e., of the unsigned characters);
 * each position r is such that the items before r are smaller than the
 * pattern and those from r on are not, a postcondition. The functions are
 * defined in PrefixSearch.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9 (sections 9.1 and 9.2)
 * \see E. D. Demaine, T. Jones, M. Patrascu, Interpolation search for
 *      non-independent data, Proceedings of SODA 2004, 529-530, 2004
 * \see Y. Perl, A. Itai, H. Avni, Interpolation search: a log log N
 *      search, Communications of the ACM 21(7), 550-553, 1978,
 *      doi:10.1145/359545.359557
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

#ifndef __ADS_PrefixSearch
 #define __ADS_PrefixSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/BinarySearch.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the number of strings of the sorted A smaller than Q
/// \pre A is sorted (an audit)
/// \post A[ r - 1 ] < Q <= A[ r ]
[[nodiscard]] inline std::size_t lexicographicPosition(
                                       const std::vector< std::string > & A ,
                                       std::string_view Q );

/// the number of strings of the sorted A smaller than P#, i.e., smaller
/// than P or prefixed by P
/// \pre A is sorted (an audit)
/// \post the strings before r are smaller than P#, those from r on are not
[[nodiscard]] inline std::size_t prefixEnd(
                                       const std::vector< std::string > & A ,
                                       std::string_view P );

/// the range [ l , r ) of the strings of the sorted A prefixed by P
/// \pre A is sorted (an audit)
[[nodiscard]] inline std::pair< std::size_t , std::size_t > prefixRange(
                                       const std::vector< std::string > & A ,
                                       std::string_view P );

/// the length of the longest common prefix of a and b
[[nodiscard]] inline std::size_t commonPrefixLength( std::string_view a ,
                                                     std::string_view b );

namespace detail {

/// s < P, or s < P# if upper
[[nodiscard]] inline bool precedes( std::string_view s , std::string_view P ,
                                    bool upper );

/// the integer of the first 8 characters of s, the first one the most
/// significant, padded with the character 0
[[nodiscard]] inline std::uint64_t prefixKey( std::string_view s );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BinnedSearch ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the search by interpolation of a sorted array of integers by bins
class BinnedSearch {
 public:
 /// the bins of X
 /// \pre X is sorted (an audit)
 explicit BinnedSearch( std::vector< std::uint64_t > X );

 /// the number of items smaller than y
 /// \post X[ r - 1 ] < y <= X[ r ]
 [[nodiscard]] std::size_t position( std::uint64_t y ) const;

 [[nodiscard]] bool contains( std::uint64_t y ) const;

 /// the number of bins
 [[nodiscard]] std::size_t bins( void ) const;

 /// the width b of a bin
 [[nodiscard]] std::uint64_t width( void ) const;

 /// the places [ first , last ) of X in the bin j
 /// \pre j < bins()
 [[nodiscard]] std::pair< std::size_t , std::size_t > bin(
                                                      std::size_t j ) const;

 /// the bin of y, or none if y is out of [ x_1 , x_m ]
 [[nodiscard]] std::optional< std::size_t > binOf( std::uint64_t y ) const;

 /// the items of the largest bin
 [[nodiscard]] std::size_t largestBin( void ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] const std::vector< std::uint64_t > & items( void ) const;

 private:
 std::vector< std::uint64_t > f_X;
 std::uint64_t f_b = 1;                     ///< the width of a bin
 std::vector< std::size_t > f_start;        ///< where each bin starts
 };

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BinnedStringSearch -------------------------*/
/*--------------------------------------------------------------------------*/

/// the search by interpolation of a sorted array of strings by bins
class BinnedStringSearch {
 public:
 /// the bins of the integers of the first 8 characters of A
 /// \pre A is sorted (an audit)
 explicit BinnedStringSearch( std::vector< std::string > A );

 /// the number of strings smaller than Q
 /// \post A[ r - 1 ] < Q <= A[ r ]
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than P#
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range [ l , r ) of the strings prefixed by P
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 [[nodiscard]] const BinnedSearch & keys( void ) const;

 private:
 /// the number of strings that precede P (or P# if upper), the bin found
 /// by the integer k
 [[nodiscard]] std::size_t rank( std::string_view P , bool upper ,
                                 std::uint64_t k ) const;

 std::vector< std::string > f_A;
 BinnedSearch f_K;                          ///< on the integers of A
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/PrefixSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PrefixSearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PrefixSearch.h ------------------------*/
/*--------------------------------------------------------------------------*/
