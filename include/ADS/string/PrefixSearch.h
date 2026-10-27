/*--------------------------------------------------------------------------*/
/*-------------------------- File PrefixSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search by prefix in a sorted dictionary of \f$ n \f$ strings, beyond
 * the book: the strings prefixed by \f$ P \f$ are consecutive, between the
 * lexicographic position of \f$ P \f$ (the number of strings smaller than
 * \f$ P \f$) and that of \f$ P\# \f$, \f$ \# \f$ being a character larger
 * than all the others, so that two lexicographic searches give their range.
 *
 * - `lexicographicPosition()`, `prefixEnd()` and `prefixRange()` search the
 *   array of the strings by a binary search, \f$ O( p \log n ) \f$ time for
 *   a pattern of \f$ p \f$ characters, each step a comparison of strings:
 *   simple, but each step reads a string far from the others, i.e.,
 *   \f$ O( ( p / B ) \log n ) \f$ transfers of blocks of \f$ B \f$
 *   characters, which the two-level dictionaries [see
 *   `FrontCodedDictionary` and `TwoLevelDictionary` of `FrontCoding.h`]
 *   reduce to a search on a sample of the strings and the scan of one
 *   bucket;
 * - `BinnedSearch`, the search by interpolation of Demaine, Jones and
 *   Pătrașcu in a sorted array \f$ X \f$ of \f$ m \f$ integers:
 *   \f$ [ x_1 , x_m ] \f$ is cut into bins of width
 *   \f$ b = \lceil ( x_m - x_1 + 1 ) / m \rceil \f$, an array gives where
 *   each bin starts in \f$ X \f$, and \f$ y \f$ is searched by a binary
 *   search in its bin \f$ \lfloor ( y - x_1 ) / b \rfloor \f$; a bin holds
 *   at most \f$ \Delta \f$ items, \f$ \Delta \f$ being the ratio between the
 *   largest and the smallest gap of \f$ X \f$, since \f$ b \f$ is at most
 *   the largest gap (\f$ x_m - x_1 \f$ is the sum of the \f$ m - 1 \f$
 *   gaps, and \f$ b \f$ about its \f$ m \f$-th part), so that a search takes
 *   \f$ O( \log \min( \Delta , m ) ) \f$ time in the worst case, never
 *   worse than the binary search, and \f$ O( \log \log m ) \f$ with high
 *   probability on uniform keys, as the search by interpolation [see
 *   `InterpolationSearch.h`] whose worst case is \f$ O( m ) \f$, at the
 *   price of the array of the bins;
 * - `BinnedStringSearch`, the same on strings, read as the integers of
 *   their first 8 characters (padded with the character 0), which keep
 *   their order: the bin of a pattern is found by its integer, and the
 *   binary search in the bin compares whole strings.
 *
 * The contracts [see `Contract.h`]: the array sorted, an audit (for the
 * strings by the order of `std::string`, i.e., of the unsigned characters);
 * each position \f$ r \f$ is such that the items before \f$ r \f$ are
 * smaller than the pattern and those from \f$ r \f$ on are not, a
 * postcondition. The functions are defined in `PrefixSearch.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9, Searching Strings by Prefix,
 *      doi:10.1017/9781009128933
 * \see E. D. Demaine, T. Jones, M. Pătrașcu, Interpolation search for
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

/// the number of strings of the sorted \f$ A \f$ smaller than \f$ Q \f$
///
/// In pseudocode (the binary search of chapter 9, Searching Strings by
/// Prefix, of Ferragina's book, beyond the book),
/// \f$ [ \mathit{lo} , \mathit{hi} ) \f$ the places that may still hold the
/// answer:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lexicographicPosition}( A , Q ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad c \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor \cr
/// 5 & \qquad \textbf{if}\ A[ c ] < Q\ \textbf{then}\ \mathit{lo} \gets c +
///     1\ \textbf{else}\ \mathit{hi} \gets c \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
/// \post \f$ A[ r - 1 ] < Q \le A[ r ] \f$
[[nodiscard]] inline std::size_t lexicographicPosition(
                                       const std::vector< std::string > & A ,
                                       std::string_view Q );

/// the number of strings of the sorted \f$ A \f$ smaller than \f$ P\# \f$,
/// i.e., smaller than \f$ P \f$ or prefixed by \f$ P \f$
///
/// In pseudocode (chapter 9, Searching Strings by Prefix, of Ferragina's
/// book, beyond the book), the binary search of `lexicographicPosition()`
/// with another test:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{prefixEnd}( A , P ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad c \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor \cr
/// 5 & \qquad \textbf{if}\ A[ c ] < P \text{ or } P \text{ is a prefix of }
///     A[ c ]\ \textbf{then}\ \mathit{lo} \gets c + 1\ \textbf{else}
///     \ \mathit{hi} \gets c \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
/// \post the strings before \f$ r \f$ are smaller than \f$ P\# \f$, those
///       from \f$ r \f$ on are not
[[nodiscard]] inline std::size_t prefixEnd(
                                       const std::vector< std::string > & A ,
                                       std::string_view P );

/// the range \f$ [ l , r ) \f$ of the strings of the sorted \f$ A \f$
/// prefixed by \f$ P \f$
///
/// In pseudocode (chapter 9, Searching Strings by Prefix, of Ferragina's
/// book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{prefixRange}( A , P ) \cr
/// 2 & \quad \textbf{return}\ ( \mathrm{lexicographicPosition}( A , P ) ,
///     \mathrm{prefixEnd}( A , P ) )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
[[nodiscard]] inline std::pair< std::size_t , std::size_t > prefixRange(
                                       const std::vector< std::string > & A ,
                                       std::string_view P );

/// the length of the longest common prefix of \f$ a \f$ and \f$ b \f$, by
/// comparing them from their start
[[nodiscard]] inline std::size_t commonPrefixLength( std::string_view a ,
                                                     std::string_view b );

namespace detail {

/// \f$ s < P \f$, or \f$ s < P\# \f$ if `upper`
[[nodiscard]] inline bool precedes( std::string_view s , std::string_view P ,
                                    bool upper );

/// the integer of the first 8 characters of \f$ s \f$, the first one the
/// most significant, padded with the character 0
[[nodiscard]] inline std::uint64_t prefixKey( std::string_view s );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BinnedSearch ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the search by interpolation of a sorted array of integers by bins
class BinnedSearch {
 public:
 /// the bins of \f$ X \f$, in \f$ O( m ) \f$
 ///
 /// In pseudocode (Demaine, Jones and Pătrașcu, beyond the book),
 /// \f$ \mathit{start}[ j ] \f$ the first place of \f$ X \f$ in the bin
 /// \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{BinnedSearch}( X ) \cr
 /// 2 & \quad b \gets \lfloor ( x_m - x_1 ) / m \rfloor + 1;\ \mathit{nb}
 ///     \gets \lfloor ( x_m - x_1 ) / b \rfloor + 1 \cr
 /// 3 & \quad i \gets 0 \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ \mathit{nb} - 1
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \textbf{while}\ i < m\ \textbf{and}\ X[ i ] < x_1 + j b
 ///     \ \textbf{do}\ i \gets i + 1 \cr
 /// 6 & \qquad \mathit{start}[ j ] \gets i \cr
 /// 7 & \quad \mathit{start}[ \mathit{nb} ] \gets m
 /// \end{array}}
 /// \f]
 /// \pre \f$ X \f$ is sorted (audit)
 explicit BinnedSearch( std::vector< std::uint64_t > X );

 /// the number of items smaller than \f$ y \f$
 ///
 /// In pseudocode (Demaine, Jones and Pătrașcu, beyond the book), a binary
 /// search in the bin of \f$ y \f$ alone:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{position}( y ) \cr
 /// 2 & \quad \textbf{if}\ m = 0\ \textbf{or}\ y \le x_1\ \textbf{then}
 ///     \ \textbf{return}\ 0 \cr
 /// 3 & \quad \textbf{if}\ y > x_m\ \textbf{then return}\ m \cr
 /// 4 & \quad j \gets \lfloor ( y - x_1 ) / b \rfloor \cr
 /// 5 & \quad \textbf{return}\ \text{the first place } r \in [
 ///     \mathit{start}[ j ] , \mathit{start}[ j + 1 ] ) \text{ with } y \le
 ///     X[ r ] \text{ (or } \mathit{start}[ j + 1 ] \text{), by a binary }
 ///     \text{search}
 /// \end{array}}
 /// \f]
 /// \post \f$ X[ r - 1 ] < y \le X[ r ] \f$
 [[nodiscard]] std::size_t position( std::uint64_t y ) const;

 /// whether \f$ y \f$ is an item, i.e., \f$ X[ r ] = y \f$ at the place
 /// \f$ r \f$ that `position( y )` gives
 [[nodiscard]] bool contains( std::uint64_t y ) const;

 /// the number of bins
 [[nodiscard]] std::size_t bins( void ) const;

 /// the width \f$ b \f$ of a bin
 [[nodiscard]] std::uint64_t width( void ) const;

 /// the places \f$ [ \mathit{first} , \mathit{last} ) \f$ of \f$ X \f$ in
 /// the bin \f$ j \f$
 /// \pre \f$ j \f$ is less than `bins()`
 [[nodiscard]] std::pair< std::size_t , std::size_t > bin(
                                                      std::size_t j ) const;

 /// the bin of \f$ y \f$, or none if \f$ y \f$ is out of
 /// \f$ [ x_1 , x_m ] \f$
 [[nodiscard]] std::optional< std::size_t > binOf( std::uint64_t y ) const;

 /// the number of items of the largest bin
 [[nodiscard]] std::size_t largestBin( void ) const;

 /// the number \f$ m \f$ of items
 [[nodiscard]] std::size_t size( void ) const;

 /// the sorted array \f$ X \f$
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
 /// the bins of the integers of the first 8 characters of \f$ A \f$, a
 /// `BinnedSearch` on them
 /// \pre \f$ A \f$ is sorted (audit)
 explicit BinnedStringSearch( std::vector< std::string > A );

 /// the number of strings smaller than \f$ Q \f$
 ///
 /// In pseudocode (Demaine, Jones and Pătrașcu, beyond the book),
 /// \f$ k( s ) \f$ the integer of the first 8 characters of \f$ s \f$, which
 /// keeps the order of the strings:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lexicographicPosition}( Q ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{or}\ k( Q ) < k( A[ 0 ] )
 ///     \ \textbf{then return}\ 0 \cr
 /// 3 & \quad \textbf{if}\ k( Q ) > k( A[ n - 1 ] )\ \textbf{then return}
 ///     \ n \cr
 /// 4 & \quad [ \mathit{lo} , \mathit{hi} ) \gets \text{the bin of } k( Q )
 ///     \cr
 /// 5 & \quad \textbf{return}\ \text{the first place } r \in [ \mathit{lo} ,
 ///     \mathit{hi} ) \text{ with } Q \le A[ r ] \text{ (or } \mathit{hi}
 ///     \text{), by a binary search}
 /// \end{array}}
 /// \f]
 /// \post \f$ A[ r - 1 ] < Q \le A[ r ] \f$
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than \f$ P\# \f$
 ///
 /// In pseudocode (Demaine, Jones and Pătrașcu, beyond the book), the
 /// search of `lexicographicPosition()` from the largest integer of a
 /// string prefixed by \f$ P \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prefixEnd}( P ) \cr
 /// 2 & \quad \kappa \gets k( P ) \text{ with the bytes past } P \text{ set }
 ///     \text{to } 255 \cr
 /// 3 & \quad \textbf{if}\ n = 0\ \textbf{or}\ \kappa < k( A[ 0 ] )
 ///     \ \textbf{then return}\ 0 \cr
 /// 4 & \quad \textbf{if}\ \kappa > k( A[ n - 1 ] )\ \textbf{then return}
 ///     \ n \cr
 /// 5 & \quad [ \mathit{lo} , \mathit{hi} ) \gets \text{the bin of } \kappa
 ///     \cr
 /// 6 & \quad \textbf{return}\ \text{the first place } r \in [ \mathit{lo} ,
 ///     \mathit{hi} ) \text{ with } A[ r ] \ge P\# \text{ (or } \mathit{hi}
 ///     \text{)}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range \f$ [ l , r ) \f$ of the strings prefixed by \f$ P \f$, by
 /// `lexicographicPosition()` and `prefixEnd()`
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the `BinnedSearch` of the integers of the strings
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
