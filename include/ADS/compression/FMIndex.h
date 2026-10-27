/*--------------------------------------------------------------------------*/
/*----------------------------- File FMIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The FM-index of Ferragina and Manzini (chapter 14, Block-Sorting
 * Compression, of Ferragina's book, beyond the book of Montresor): the
 * transform of Burrows and Wheeler of \f$ s \f$ [see `BurrowsWheeler.h`] as an
 * index of its substrings, which replaces the suffix array and the text.
 *
 * - `count( P )` is the backward search: the rows of \f$ M' \f$ prefixed by
 *   \f$ P[ i , p - 1 ] \f$ are a range
 *   \f$ [ \mathit{first} , \mathit{last} ) \f$, and those prefixed by
 *   \f$ c P[ i , p - 1 ] \f$, with \f$ c = P[ i - 1 ] \f$, are
 *   \f$ [ C[ c ] + \mathrm{rank}_c( \mathit{first} ) , C[ c ] +
 *   \mathrm{rank}_c( \mathit{last} ) ) \f$,
 *   the LF-mapping of the first and of the last occurrence of \f$ c \f$ in
 *   \f$ L[ \mathit{first} , \mathit{last} ) \f$; hence \f$ p \f$ steps, each
 *   of two ranks, give the range of \f$ P \f$ and its number of occurrences,
 *   \f$ \mathit{last} - \mathit{first} \f$, with no access to the text;
 * - `locate( P )` finds the place of each row of the range: the rows of the
 *   suffixes that start at a multiple of \f$ \mu \f$ are marked, with their
 *   places, and from any row the LF-mapping, which moves one symbol to the
 *   left in \f$ s \f$, reaches a marked row in \f$ t < \mu \f$ steps, the
 *   place being that of the marked row plus \f$ t \f$; this gives
 *   \f$ O( \mu ) \f$ steps per occurrence and
 *   \f$ O( ( n / \mu ) \log n ) \f$ bits of samples;
 * - `extract( i , j )` gives \f$ s[ i , j ) \f$ from the row of the first
 *   sampled suffix at or after \f$ j \f$, by \f$ j - i + O( \mu ) \f$ steps
 *   of the LF-mapping.
 *
 * The rank of a symbol in \f$ L \f$, the number of its occurrences in a
 * prefix, is the count stored every \f$ b \f$ places for each symbol of the
 * text plus a scan of fewer than \f$ b \f$ symbols, 8 at a time by the zero
 * bytes of a word, i.e., \f$ O( b ) \f$ time and \f$ \sigma ( n / b ) \f$
 * counts of 32 bits; the marks of the rows are a vector of bits with the rank
 * directory of `RankSelect` [see `RankSelect.h`], so that a row is tested and
 * its sample found in \f$ O( 1 ) \f$ time. The counts of \f$ L \f$ are the
 * simplest rank on a sequence, which any structure of rank and select on
 * sequences can replace [see `WaveletMatrix.h`]: the wavelet tree of Grossi,
 * Gupta and Vitter gives the rank in \f$ O( \log \sigma ) \f$ time on
 * \f$ n H_0 + o( n \log \sigma ) \f$ bits, and the compressed representations
 * of Ferragina, Manzini, Mäkinen and Navarro reach
 * \f$ n H_k + o( n \log \sigma ) \f$ bits, the space of the compressed text.
 * The transform is built from the suffix array of SA-IS, in \f$ O( n ) \f$
 * time [see `BurrowsWheeler.h`]. Against the suffix array, which counts by
 * binary search in \f$ O( p \log n ) \f$ time on \f$ n \log n \f$ bits plus
 * the text, the FM-index counts in \f$ O( p b ) \f$ time here, \f$ O( p ) \f$
 * with a rank in \f$ O( 1 ) \f$, at the price of random accesses to \f$ L \f$
 * at each step; the r-index of Gagie, Navarro and Prezza samples only the ends
 * of the runs of \f$ L \f$, and takes space proportional to their number
 * \f$ r \f$, small on repetitive texts [see `RIndex.h`].
 *
 * The contracts [see `Contract.h`]: \f$ b \f$ and \f$ \mu \f$ are positive,
 * \f$ n < 2^{32} \f$, a pattern is not empty, \f$ i \le j \le n \f$; each
 * place found by `locate()` is an occurrence, an audit. The operations are
 * defined in `FMIndex.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14, Block-Sorting Compression,
 *      doi:10.1017/9781009128933
 * \see P. Ferragina, G. Manzini, Opportunistic data structures with
 *      applications, IEEE Symposium on Foundations of Computer Science,
 *      390-398, 2000, doi:10.1109/SFCS.2000.892127
 * \see P. Ferragina, G. Manzini, Indexing compressed text, Journal of the
 *      ACM 52(4), 552-581, 2005, doi:10.1145/1082036.1082039
 * \see R. Grossi, A. Gupta, J. S. Vitter, High-order entropy-compressed
 *      text indexes, ACM-SIAM Symposium on Discrete Algorithms, 841-850,
 *      2003
 * \see P. Ferragina, G. Manzini, V. Mäkinen, G. Navarro, Compressed
 *      representations of sequences and full-text indexes, ACM Transactions
 *      on Algorithms 3(2), 20, 2007, doi:10.1145/1240233.1240243
 * \see G. Navarro, V. Mäkinen, Compressed full-text indexes, ACM Computing
 *      Surveys 39(1), 2, 2007, doi:10.1145/1216370.1216372
 * \see T. Gagie, G. Navarro, N. Prezza, Fully functional suffix trees and
 *      optimal text searching in BWT-runs bounded space, Journal of the ACM
 *      67(1), 2, 2020, doi:10.1145/3375890
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

#ifndef __ADS_FMIndex
 #define __ADS_FMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS FMIndex ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the FM-index of a text: its transform with the counts for the rank every
/// \f$ b \f$ places and the places of the suffixes at the multiples of
/// \f$ \mu \f$
class FMIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of \f$ s \f$
 ///
 /// In pseudocode (the FM-index of Ferragina and Manzini, beyond the book),
 /// \f$ \sigma \f$ the number of the distinct symbols of \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{FMIndex}( s , b , \mu ) \cr
 /// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s\$;\ L[ i ]
 ///     \gets s[ \mathit{SA}[ i ] - 1 ] \text{ for each row } i \cr
 /// 3 & \quad C[ c ] \gets 1 + \text{the number of the symbols of } s \text{ }
 ///     \text{smaller than } c \text{, for each } c \cr
 /// 4 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n + 1\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ i \bmod b = 0\ \textbf{then}\ \mathit{occ}[ i / b
 ///     ] \gets \text{the occurrences of each symbol in } L[ 0 , i ) \cr
 /// 6 & \quad \textbf{foreach}\ \text{row } i \text{ with } \mathit{SA}[ i ]
 ///     \bmod \mu = 0\ \textbf{do} \cr
 /// 7 & \qquad \text{mark } i;\ \mathit{place}[ \mathrm{rank}_1( i ) ] \gets
 ///     \mathit{SA}[ i ];\ \mathit{row}[ \mathit{SA}[ i ] / \mu ] \gets i
 /// \end{array}}
 /// \f]
 /// \pre `b > 0`, `mu > 0`, \f$ |s| < 2^{32} \f$
 explicit FMIndex( std::string_view s , std::size_t b = 64 ,
                   std::size_t mu = 32 );

 /// the length \f$ n \f$ of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the transform of the text
 [[nodiscard]] BWT transform( void ) const;

 /// the occurrences of \f$ c \f$ in \f$ L[ 0 , k ) \f$, the \f$ \$ \f$
 /// excluded
 ///
 /// In pseudocode (the FM-index of Ferragina and Manzini, beyond the book),
 /// the count stored at the start of the block of \f$ k \f$ and a scan of the
 /// rest, 8 symbols at a time in the code:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank}( c , k ) \cr
 /// 2 & \quad \textbf{if}\ c \text{ is not in } s\ \textbf{then return}\ 0 \cr
 /// 3 & \quad a \gets \lfloor k / b \rfloor b;\ r \gets \mathit{occ}[ a / b
 ///     ][ c ] \cr
 /// 4 & \quad \textbf{for}\ i \gets a\ \textbf{to}\ k - 1\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ L[ i ] = c \text{ and } i \text{ is not the row }
 ///     \text{of the } \$\ \textbf{then}\ r \gets r + 1 \cr
 /// 6 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \le n + 1 \f$
 [[nodiscard]] std::size_t rank( char c , std::size_t k ) const;

 /// the range \f$ [ \mathit{first} , \mathit{last} ) \f$ of the rows of
 /// \f$ M' \f$ prefixed by \f$ P \f$, empty if none, by backward search
 ///
 /// In pseudocode (the backward search of Ferragina and Manzini, beyond the
 /// book), each step prepending the symbol \f$ P[ i ] \f$ to the suffix of
 /// \f$ P \f$ already matched:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( P ) \cr
 /// 2 & \quad \mathit{first} \gets 0;\ \mathit{last} \gets n + 1 \cr
 /// 3 & \quad \textbf{for}\ i \gets p - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{first} \ge \mathit{last}\ \textbf{then
 ///     return}\ \text{an empty range} \cr
 /// 5 & \qquad c \gets P[ i ] \cr
 /// 6 & \qquad \mathit{first} \gets C[ c ] + \mathrm{rank}( c ,
 ///     \mathit{first} );\ \mathit{last} \gets C[ c ] + \mathrm{rank}( c ,
 ///     \mathit{last} ) \cr
 /// 7 & \quad \textbf{return}\ [ \mathit{first} , \mathit{last} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
                                                  std::string_view P ) const;

 /// the number of the occurrences of \f$ P \f$
 ///
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{count}( P ) \cr
 /// 2 & \quad [ \mathit{first} , \mathit{last} ) \gets \mathrm{range}( P );
 ///     \ \textbf{return}\ \mathit{last} - \mathit{first}
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of \f$ P \f$, in increasing order
 ///
 /// In pseudocode (the FM-index of Ferragina and Manzini, beyond the book),
 /// each row walked back by the LF-mapping to a marked one, fewer than
 /// \f$ \mu \f$ steps away:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locate}( P ) \cr
 /// 2 & \quad [ \mathit{first} , \mathit{last} ) \gets \mathrm{range}( P );
 ///     \ R \gets \emptyset \cr
 /// 3 & \quad \textbf{for}\ r \gets \mathit{first}\ \textbf{to}
 ///     \ \mathit{last} - 1\ \textbf{do} \cr
 /// 4 & \qquad t \gets 0 \cr
 /// 5 & \qquad \textbf{while}\ r \text{ is not marked}\ \textbf{do}\ r \gets
 ///     \mathrm{lf}( r );\ t \gets t + 1 \cr
 /// 6 & \qquad R \gets R \cup \{ \mathit{place}[ \mathrm{rank}_1( r ) ] + t
 ///     \} \cr
 /// 7 & \quad \textbf{return}\ R \text{ sorted}
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the substring \f$ s[ i , j ) \f$
 ///
 /// In pseudocode (the FM-index of Ferragina and Manzini, beyond the book),
 /// from the row of the first sampled suffix at or after \f$ j \f$, each
 /// step of the LF-mapping reading the symbol before the current suffix:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{extract}( i , j ) \cr
 /// 2 & \quad p \gets \lceil j / \mu \rceil \mu \cr
 /// 3 & \quad \textbf{if}\ p \ge n\ \textbf{then}\ p \gets n;\ r \gets 0
 ///     \ \textbf{else}\ r \gets \mathit{row}[ p / \mu ] \cr
 /// 4 & \quad \textbf{for}\ t \gets p\ \textbf{downto}\ i + 1\ \textbf{do}
 ///     \quad \triangleright\ r \text{ is the row of the suffix } t \cr
 /// 5 & \qquad \textbf{if}\ t - 1 < j\ \textbf{then}\ s[ t - 1 ] \gets L[ r ]
 ///     \cr
 /// 6 & \qquad r \gets \mathrm{lf}( r ) \cr
 /// 7 & \quad \textbf{return}\ s[ i , j )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j \le n \f$
 [[nodiscard]] std::string extract( std::size_t i , std::size_t j ) const;

 /// the bits of the index: \f$ L \f$, the counts, the marks with their
 /// directory, the samples and the arrays of the alphabet
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the row of the suffix one place to the left of that of the row
 /// \f$ r \f$
 /// \pre \f$ r \f$ is not the row of the suffix 0
 [[nodiscard]] std::size_t lf( std::size_t r ) const;

 /// the place in \f$ s \f$ of the suffix of the row \f$ r \f$
 [[nodiscard]] std::size_t place( std::size_t r ) const;

 /// whether the row \f$ r \f$ is marked
 [[nodiscard]] bool marked( std::size_t r ) const { return( f_mark[ r ] ); }

 std::size_t f_n , f_b , f_mu;
 std::string f_L;                       ///< \f$ L \f$, 0 for the \f$ \$ \f$
 std::size_t f_dollar;                  ///< the row of the \f$ \$ \f$
 std::array< std::size_t , 257 > f_C;   ///< 1 + the symbols below \f$ c \f$
 std::array< int , 256 > f_id;          ///< the index of a symbol, or -1
 std::size_t f_sigma = 0;               ///< the distinct symbols
 std::vector< std::uint32_t > f_occ;    ///< the counts every \f$ b \f$ rows
 RankSelect f_mark;                     ///< the marked rows, with rank
 std::vector< std::size_t > f_place;    ///< the places of the marked rows
 std::vector< std::size_t > f_row;      ///< the row of suffix \f$ k \mu \f$

 };  // end( class FMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/FMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FMIndex.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File FMIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
