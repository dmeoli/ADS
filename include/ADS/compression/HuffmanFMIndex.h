/*--------------------------------------------------------------------------*/
/*-------------------------- File HuffmanFMIndex.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The FM-index on the wavelet tree shaped by the code of Huffman, i.e., the
 * succinct suffix array of Mäkinen and Navarro (beyond the book): the
 * transform \f$ L \f$ of Burrows and Wheeler of a text \f$ s \f$ of \f$ n \f$
 * bytes as a `HuffmanWaveletTree` [see `HuffmanWaveletTree.h`], in
 * \f$ n ( H_0 + 1 ) ( 1 + o( 1 ) ) \f$ bits, with the same operations as the
 * FM-index of chapter 14, Block-Sorting Compression, of Ferragina's book [see
 * `FMIndex.h`], which keeps \f$ L \f$ as it is with counts every \f$ b \f$
 * places, \f$ 8 n + 32 \sigma n / b \f$ bits.
 *
 * - `count( P )` is the backward search: the rows prefixed by
 *   \f$ c P' \f$ are
 *   \f$ [ C[ c ] + \mathrm{rank}_c( \mathit{first} ) , C[ c ] +
 *   \mathrm{rank}_c( \mathit{last} ) ) \f$ for the range
 *   \f$ [ \mathit{first} , \mathit{last} ) \f$ of \f$ P' \f$, in
 *   \f$ O( p ( H_0 + 1 ) ) \f$ steps on average, each one a rank of the bits
 *   of the tree;
 * - `locate( P )` walks each row of the range by the LF-mapping,
 *   \f$ \mathit{LF}( r ) = C[ L[ r ] ] + \mathrm{rank}_{L[ r ]}( r ) \f$,
 *   found by one descent of the tree (`accessRank()`), until a row whose
 *   suffix starts at a multiple of \f$ \mu \f$, marked and sampled;
 * - `extract( i , j )` starts from the row of the sampled suffix at or after
 *   \f$ j \f$ and walks back by the LF-mapping.
 *
 * The symbols of \f$ L \f$ are the bytes plus 1, the end of the text being 0.
 * The backward search takes the two ranks of a step by one descent of the
 * tree, as they meet the same nodes. The samples are
 * \f$ \lceil n / \mu \rceil \f$ places and as many rows of
 * \f$ \lceil \log_2 ( n + 1 ) \rceil \f$ bits each, and the marks are
 * \f$ n + 1 \f$ bits with the compact rank of `CompactRankSelect` [see
 * `CompactRankSelect.h`], since a step of `locate()` tests the bit alone and
 * the rank is needed once per occurrence; the parameter `RS` is the rank and
 * select of the tree, `RankSelect` or `CompactRankSelect`. The transform
 * comes from the suffix array of SA-IS [see `BurrowsWheeler.h`], in
 * \f$ O( n ) \f$ time.
 *
 * The contracts [see `Contract.h`]: \f$ \mu > 0 \f$, \f$ n < 2^{32} \f$, a
 * pattern is not empty, \f$ i \le j \le n \f$; each place found by
 * `locate()` is an occurrence, an audit. The functions are defined in
 * `HuffmanFMIndex.tpp`.
 *
 * \see V. Mäkinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see P. Ferragina, G. Manzini, Indexing compressed text, Journal of the
 *      ACM 52(4), 552-581, 2005, doi:10.1145/1082036.1082039
 * \see P. Ferragina, G. Manzini, V. Mäkinen, G. Navarro, Compressed
 *      representations of sequences and full-text indexes, ACM
 *      Transactions on Algorithms 3(2), 20, 2007,
 *      doi:10.1145/1240233.1240243
 * \see G. Navarro, V. Mäkinen, Compressed full-text indexes, ACM Computing
 *      Surveys 39(1), 2, 2007, doi:10.1145/1216370.1216372
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14, Block-Sorting Compression,
 *      doi:10.1017/9781009128933
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

#ifndef __ADS_HuffmanFMIndex
 #define __ADS_HuffmanFMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/HuffmanWaveletTree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS HuffmanFMIndex --------------------------*/
/*--------------------------------------------------------------------------*/

/// the FM-index of a text on the wavelet tree of Huffman of its transform,
/// with the places of the suffixes at the multiples of \f$ \mu \f$
template< class RS = RankSelect >
class HuffmanFMIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of \f$ s \f$
 ///
 /// In pseudocode (the succinct suffix array of Mäkinen and Navarro, beyond
 /// the book), the places and the rows of the samples in
 /// \f$ w = \lceil \log_2 ( n + 1 ) \rceil \f$ bits each:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{HuffmanFMIndex}( s , \mu ) \cr
 /// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s\$ \cr
 /// 3 & \quad L[ i ] \gets s[ \mathit{SA}[ i ] - 1 ] + 1 \text{ for each row }
 ///     i \text{, and } 0 \text{ for the row of the } \$ \cr
 /// 4 & \quad C[ c ] \gets \text{the number of the symbols of } L \text{ }
 ///     \text{smaller than } c \text{, for each } c \cr
 /// 5 & \quad \text{build the wavelet tree of Huffman of } L \cr
 /// 6 & \quad \textbf{foreach}\ \text{row } i \text{ with } \mathit{SA}[ i ]
 ///     \bmod \mu = 0\ \textbf{do} \cr
 /// 7 & \qquad \text{mark } i;\ \mathit{place}[ \mathrm{rank}_1( i ) ] \gets
 ///     \mathit{SA}[ i ];\ \mathit{row}[ \mathit{SA}[ i ] / \mu ] \gets i
 /// \end{array}}
 /// \f]
 /// \pre `mu > 0`, \f$ |s| < 2^{32} \f$
 explicit HuffmanFMIndex( std::string_view s , std::size_t mu = 32 );

 /// the length \f$ n \f$ of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the range \f$ [ \mathit{first} , \mathit{last} ) \f$ of the rows
 /// prefixed by \f$ P \f$, empty if none
 ///
 /// In pseudocode (the backward search of Ferragina and Manzini, beyond the
 /// book), the two ranks of a step taken by one descent of the tree:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( P ) \cr
 /// 2 & \quad \mathit{first} \gets 0;\ \mathit{last} \gets n + 1 \cr
 /// 3 & \quad \textbf{for}\ i \gets p - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad c \gets P[ i ] + 1 \cr
 /// 5 & \qquad ( a , b ) \gets ( \mathrm{rank}_c( \mathit{first} ) ,
 ///     \mathrm{rank}_c( \mathit{last} ) ) \text{ on the tree} \cr
 /// 6 & \qquad \mathit{first} \gets C[ c ] + a;\ \mathit{last} \gets C[ c ] +
 ///     b \cr
 /// 7 & \qquad \textbf{if}\ \mathit{first} \ge \mathit{last}\ \textbf{then
 ///     return}\ \text{an empty range} \cr
 /// 8 & \quad \textbf{return}\ [ \mathit{first} , \mathit{last} )
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
 /// In pseudocode (the succinct suffix array of Mäkinen and Navarro, beyond
 /// the book), each step of the LF-mapping one descent of the tree that
 /// gives the symbol \f$ c = L[ r ] \f$ and its rank \f$ k \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locate}( P ) \cr
 /// 2 & \quad [ \mathit{first} , \mathit{last} ) \gets \mathrm{range}( P );
 ///     \ R \gets \emptyset \cr
 /// 3 & \quad \textbf{for}\ r \gets \mathit{first}\ \textbf{to}
 ///     \ \mathit{last} - 1\ \textbf{do} \cr
 /// 4 & \qquad t \gets 0 \cr
 /// 5 & \qquad \textbf{while}\ r \text{ is not marked}\ \textbf{do} \cr
 /// 6 & \qquad\quad ( c , k ) \gets \mathrm{accessRank}( r );\ r \gets C[ c ]
 ///     + k;\ t \gets t + 1 \cr
 /// 7 & \qquad R \gets R \cup \{ \mathit{place}[ \mathrm{rank}_1( r ) ] + t
 ///     \} \cr
 /// 8 & \quad \textbf{return}\ R \text{ sorted}
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the substring \f$ s[ i , j ) \f$
 ///
 /// In pseudocode (the succinct suffix array of Mäkinen and Navarro, beyond
 /// the book), from the row of the first sampled suffix at or after
 /// \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{extract}( i , j ) \cr
 /// 2 & \quad p \gets \lceil j / \mu \rceil \mu \cr
 /// 3 & \quad \textbf{if}\ p \ge n\ \textbf{then}\ p \gets n;\ r \gets 0
 ///     \ \textbf{else}\ r \gets \mathit{row}[ p / \mu ] \cr
 /// 4 & \quad \textbf{for}\ t \gets p\ \textbf{downto}\ i + 1\ \textbf{do}
 ///     \quad \triangleright\ r \text{ is the row of the suffix } t \cr
 /// 5 & \qquad ( c , k ) \gets \mathrm{accessRank}( r ) \cr
 /// 6 & \qquad \textbf{if}\ t - 1 < j\ \textbf{then}\ s[ t - 1 ] \gets c - 1
 ///     \cr
 /// 7 & \qquad r \gets C[ c ] + k \cr
 /// 8 & \quad \textbf{return}\ s[ i , j )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j \le n \f$
 [[nodiscard]] std::string extract( std::size_t i , std::size_t j ) const;

 /// the bits of the index: the tree, the marks with their directory, the
 /// samples and the counts of the symbols
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place in \f$ s \f$ of the suffix of the row \f$ r \f$
 [[nodiscard]] std::size_t place( std::size_t r ) const;

 std::size_t f_n = 0 , f_mu = 1;
 int f_w = 1;                            ///< the bits of a place or a row
 HuffmanWaveletTree< RS > f_L;           ///< \f$ L \f$, the bytes plus 1
 std::array< std::size_t , 258 > f_C{};  ///< the symbols of L below each
 CompactRankSelect f_mark;               ///< the rows of the samples
 BitVector f_place;                      ///< their places, by row
 BitVector f_row;                        ///< the row of suffix \f$ k \mu \f$

 };  // end( class HuffmanFMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/HuffmanFMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HuffmanFMIndex.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HuffmanFMIndex.h -----------------------*/
/*--------------------------------------------------------------------------*/
