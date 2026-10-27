/*--------------------------------------------------------------------------*/
/*------------------------------ File RIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The r-index of Gagie, Navarro and Prezza (beyond the book): the FM-index of
 * a text \f$ s \f$ of \f$ n \f$ bytes in space proportional to the number
 * \f$ r \f$ of the runs of its transform \f$ L \f$ of Burrows and Wheeler,
 * rather than to \f$ n \f$ [see `FMIndex.h`, `HuffmanFMIndex.h`], so that a
 * repetitive text (the versions of a document, the genomes of a species),
 * where \f$ r \f$ is far smaller than \f$ n \f$, takes far less;
 * `count( P )` takes \f$ O( p ) \f$ steps of rank on the runs, and
 * `locate( P )` \f$ O( p + \mathit{occ} ) \f$ more of predecessor searches.
 *
 * \f$ L \f$ is kept as its runs: the start of each run in an Elias-Fano
 * sequence [see `EliasFano.h`], the symbol of each run (its head) in a
 * wavelet tree of Huffman [see `HuffmanWaveletTree.h`], and the starts of the
 * runs in the column \f$ F \f$, the runs of each symbol \f$ c \f$ after those
 * of the symbols before it, in one more Elias-Fano sequence: the run
 * \f$ k \f$ of \f$ c \f$ starts in \f$ F \f$ at \f$ C[ c ] + S_c[ k ] \f$,
 * with \f$ S_c[ k ] \f$ the lengths of the first \f$ k \f$ runs of
 * \f$ c \f$, at the place \f$ R_c + k \f$ of the sequence, with \f$ R_c \f$
 * the runs of the symbols below \f$ c \f$ (one sequence of \f$ r \f$ items in
 * \f$ [ 0 , n ] \f$ rather than one for each symbol, which would cost the
 * entropy of the heads more per run). The rank of \f$ c \f$ in
 * \f$ L[ 0 , i ) \f$ is then
 * \f[ \mathrm{rank}_c( i ) = F_{R_c + k} - C[ c ] + [ h_j = c ] \,
 *     ( i - p_j ) , \qquad k = \mathrm{rank}_c( h , j ) , \f]
 * with \f$ j \f$ the run of the place \f$ i - 1 \f$, \f$ p_j \f$ its start
 * and \f$ h_j \f$ its head (one predecessor search, and one descent of the
 * tree for both ranks of \f$ c \f$ around \f$ j \f$), and `count( P )` is
 * the backward search of the FM-index with it.
 *
 * `locate( P )` keeps, along the backward search, the place in \f$ s \f$ of
 * the suffix of the last row of the range, \f$ \mathit{SA}[ e - 1 ] \f$ (the
 * toehold): if \f$ L[ e - 1 ] = c \f$ the row moves by the LF-mapping and the
 * place decreases by 1, and otherwise the last \f$ c \f$ before
 * \f$ e - 1 \f$ ends a run of \f$ c \f$, whose place is kept (a sample of
 * \f$ \mathit{SA} \f$ at each end of a run). The other occurrences follow
 * from the function \f$ \phi( \mathit{SA}[ i ] ) = \mathit{SA}[ i - 1 ] \f$,
 * which shifts every stretch between two places of the starts of the runs
 * alike:
 * \f[ \phi( x ) = \phi( y ) + ( x - y ) , \qquad y = \max \{
 *     \mathit{SA}[ p_j ] : \mathit{SA}[ p_j ] \le x \} , \f]
 * so that the places \f$ \mathit{SA}[ p_j ] \f$ of the starts of the runs,
 * sorted, in Elias-Fano, give it by one predecessor search:
 * \f$ \phi( \mathit{SA}[ p_j ] ) = \mathit{SA}[ p_j - 1 ] \f$ is the sample
 * at the end of the run \f$ j - 1 \f$, and only \f$ j \f$ is kept, in
 * \f$ \lceil \log_2 r \rceil \f$ bits. All of it takes \f$ O( r ) \f$ words:
 * 5 sequences of \f$ r \f$ integers of \f$ \lceil \log_2 n \rceil \f$ bits or
 * less and the heads, whose rank and select are those of
 * `CompactRankSelect` [see `CompactRankSelect.h`], the smallest.
 *
 * The symbols of \f$ L \f$ are the bytes plus 1, the end of the text being
 * 0. The transform comes from the suffix array of SA-IS [see
 * `BurrowsWheeler.h`], in \f$ O( n ) \f$ time and words; the r-index of the
 * literature is built in \f$ O( r ) \f$ words from the run-length transform,
 * which this one does not attempt.
 *
 * The contracts [see `Contract.h`]: \f$ n < 2^{32} \f$, a pattern is not
 * empty; each place found by `locate()` is an occurrence, an audit against
 * the text kept for it only at the level of the audits. The functions are
 * defined in `RIndex.tpp`.
 *
 * \see T. Gagie, G. Navarro, N. Prezza, Fully functional suffix trees and
 *      optimal text searching in BWT-runs bounded space, Journal of the ACM
 *      67(1), 2, 2020, doi:10.1145/3375890
 * \see T. Gagie, G. Navarro, N. Prezza, Optimal-time text indexing in
 *      BWT-runs bounded space, Proc. 29th SODA, 1459-1477, 2018,
 *      doi:10.1137/1.9781611975031.96
 * \see V. Mäkinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see G. Navarro, Indexing highly repetitive string collections, part
 *      II: compressed indexes, ACM Computing Surveys 54(2), 26, 2021,
 *      doi:10.1145/3432999
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

#ifndef __ADS_RIndex
 #define __ADS_RIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"
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
/*------------------------------ CLASS RIndex ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the r-index of a text: count and locate in space proportional to the
/// runs of its transform
class RIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of the empty text
 RIndex( void ) : RIndex( std::string_view() ) {}

 /// the index of \f$ s \f$
 ///
 /// In pseudocode (the r-index of Gagie, Navarro and Prezza, beyond the
 /// book), with the runs of \f$ L \f$ numbered \f$ 0 , \ldots , r - 1 \f$,
 /// the run \f$ j \f$ from \f$ p_j \f$ to \f$ e_j \f$ with the head
 /// \f$ h_j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{RIndex}( s ) \cr
 /// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s\$;\ L[ i ]
 ///     \gets s[ \mathit{SA}[ i ] - 1 ] + 1 \text{, 0 for the } \$ \cr
 /// 3 & \quad \text{split } L \text{ in its maximal runs;}\ \text{store the }
 ///     \text{starts } p_j \text{ in Elias-Fano} \cr
 /// 4 & \quad \text{store the heads } h_j \text{ in a wavelet tree of }
 ///     \text{Huffman} \cr
 /// 5 & \quad C[ c ] \gets \text{the symbols of } L \text{ below } c;\ R_c
 ///     \gets \text{the runs of the symbols below } c \cr
 /// 6 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ r - 1\ \textbf{do}\quad
 ///     \triangleright\ \text{the starts in } F \cr
 /// 7 & \qquad c \gets h_j;\ k \gets \text{the number of the runs of } c
 ///     \text{ before } j \cr
 /// 8 & \qquad F_{R_c + k} \gets C[ c ] + \text{the length of these runs} \cr
 /// 9 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ r - 1\ \textbf{do}
 ///     \ \mathit{end}[ j ] \gets \mathit{SA}[ e_j ] \cr
 /// 10 & \quad \mathit{key} \gets \text{the values } \mathit{SA}[ p_j ]
 ///     \text{ for } j \ge 1 \text{, sorted, in Elias-Fano} \cr
 /// 11 & \quad \mathit{run}[ k ] \gets \text{the } j \text{ of }
 ///     \mathit{key}[ k ] \text{, for each } k
 /// \end{array}}
 /// \f]
 /// \pre \f$ |s| < 2^{32} \f$
 explicit RIndex( std::string_view s );

 /// the length \f$ n \f$ of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number \f$ r \f$ of the runs of the transform
 [[nodiscard]] std::size_t runs( void ) const { return( f_start.size() ); }

 /// the range \f$ [ \mathit{first} , \mathit{last} ) \f$ of the rows
 /// prefixed by \f$ P \f$, empty if none
 ///
 /// In pseudocode (the backward search of the r-index, beyond the book), the
 /// rank on the runs of the description of the header:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( P ) \cr
 /// 2 & \quad \mathit{first} \gets 0;\ \mathit{last} \gets n + 1 \cr
 /// 3 & \quad \textbf{for}\ i \gets p - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad c \gets P[ i ] + 1 \cr
 /// 5 & \qquad \mathit{first} \gets C[ c ] + \mathrm{rank}_c( \mathit{first}
 ///     );\ \mathit{last} \gets C[ c ] + \mathrm{rank}_c( \mathit{last} ) \cr
 /// 6 & \qquad \textbf{if}\ \mathit{first} \ge \mathit{last}\ \textbf{then
 ///     return}\ \text{an empty range} \cr
 /// 7 & \quad \textbf{return}\ [ \mathit{first} , \mathit{last} ) \cr
 /// 8 & \textbf{function}\ \mathrm{rank}_c( i ) \cr
 /// 9 & \quad \textbf{if}\ i = 0\ \textbf{then return}\ 0 \cr
 /// 10 & \quad j \gets \text{the run of } i - 1;\ k \gets \text{the runs of }
 ///     c \text{ before } j \cr
 /// 11 & \quad x \gets F_{R_c + k} - C[ c ] \cr
 /// 12 & \quad \textbf{if}\ h_j = c\ \textbf{then}\ x \gets x + i - p_j \cr
 /// 13 & \quad \textbf{return}\ x
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
 /// In pseudocode (the toehold and the function \f$ \phi \f$ of Gagie,
 /// Navarro and Prezza, beyond the book), \f$ v \f$ the place of the suffix
 /// of the last row of the range:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locate}( P ) \cr
 /// 2 & \quad \mathit{first} \gets 0;\ \mathit{last} \gets n + 1;\ v \gets
 ///     \mathit{end}[ r - 1 ] \cr
 /// 3 & \quad \textbf{for}\ i \gets p - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad c \gets P[ i ] + 1;\ j \gets \text{the run of }
 ///     \mathit{last} - 1 \cr
 /// 5 & \qquad \textbf{if}\ h_j = c\ \textbf{then}\ v \gets v - 1 \cr
 /// 6 & \qquad \textbf{else if}\ \text{a run of } c \text{ comes before } j
 ///     \ \textbf{then} \cr
 /// 7 & \qquad\quad v \gets \mathit{end}[ \text{the last such run} ] - 1 \cr
 /// 8 & \qquad \mathit{first} \gets C[ c ] + \mathrm{rank}_c( \mathit{first}
 ///     );\ \mathit{last} \gets C[ c ] + \mathrm{rank}_c( \mathit{last} ) \cr
 /// 9 & \qquad \textbf{if}\ \mathit{first} \ge \mathit{last}\ \textbf{then
 ///     return}\ \emptyset \cr
 /// 10 & \quad R \gets \{ v \} \cr
 /// 11 & \quad \textbf{for}\ k \gets \mathit{first} + 1\ \textbf{to}
 ///     \ \mathit{last} - 1\ \textbf{do}\ v \gets \phi( v );\ R \gets R \cup
 ///     \{ v \} \cr
 /// 12 & \quad \textbf{return}\ R \text{ sorted} \cr
 /// 13 & \textbf{function}\ \phi( x ) \cr
 /// 14 & \quad k \gets \text{the place of the largest } \mathit{key} \le x \cr
 /// 15 & \quad \textbf{return}\ \mathit{end}[ \mathit{run}[ k ] - 1 ] + x -
 ///     \mathit{key}[ k ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the bits of the index: the runs, their heads, the sums of their
 /// lengths, the samples and the function \f$ \phi \f$
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the occurrences of \f$ c \f$ in \f$ L[ 0 , i ) \f$
 [[nodiscard]] std::size_t rank( std::uint32_t c , std::size_t i ) const;

 /// \f$ \phi( x ) \f$, the place of the suffix of the row before that of
 /// \f$ x \f$
 [[nodiscard]] std::size_t phi( std::size_t x ) const;

 /// the item \f$ i \f$ of a sequence of fixed width \f$ w \f$
 [[nodiscard]] static std::size_t get( const BitVector & v , int w ,
                                       std::size_t i ) {
  return( std::size_t( v.get( i * std::size_t( w ) , w ) ) );
  }

 std::size_t f_n = 0;
 int f_w = 1;                            ///< the bits of a place or a row
 EliasFano f_start;                      ///< the start of each run
 HuffmanWaveletTree< CompactRankSelect > f_head;   ///< the head of each run
 EliasFano f_F;                          ///< the starts in \f$ F \f$
 std::array< std::size_t , 258 > f_R{};  ///< the runs of the symbols below
 std::array< std::size_t , 258 > f_C{};  ///< the symbols of L below each
 BitVector f_end;                        ///< the samples at the run ends
 EliasFano f_key;                        ///< \f$ \mathit{SA} \f$ at the starts
 BitVector f_phi;                        ///< the run of each of them
 int f_wr = 1;                           ///< the bits of a run
#if ADS_CONTRACTS >= 3
 std::string f_text;                     ///< the text, for the audits
#endif

 };  // end( class RIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/RIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RIndex.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File RIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
