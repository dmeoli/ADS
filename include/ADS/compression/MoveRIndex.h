/*--------------------------------------------------------------------------*/
/*---------------------------- File MoveRIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The r-index on move structures, after the move-r of Bertram, Fischer and
 * Nalbach (beyond the book): the operations of the r-index [see `RIndex.h`]
 * with each step in \f$ O( 1 ) \f$ time instead of a predecessor search, from
 * two move structures of Nishimoto and Tabei [see `MoveStructure.h`], at the
 * price of a few more words per run of the transform \f$ L \f$ of Burrows and
 * Wheeler of the text.
 *
 * The LF-mapping shifts each run of \f$ L \f$ alike onto the column
 * \f$ F \f$, and is therefore a move structure on the runs. The backward
 * search keeps the first and the last row of the range,
 * \f$ [ \mathit{s} , \mathit{e} ] \f$, with their intervals; for the symbol
 * \f$ c \f$ it moves \f$ \mathit{s} \f$ to the first row at or after it whose
 * symbol is \f$ c \f$ (the start of the next interval of \f$ c \f$ if its own
 * is not of \f$ c \f$) and \f$ \mathit{e} \f$ to the last one at or before it
 * (the end of the previous interval of \f$ c \f$), by the rank and select of
 * \f$ c \f$ on the tags of the intervals, kept in a wavelet tree of Huffman
 * on `RankSelect` [see `HuffmanWaveletTree.h`], the fastest; it then maps
 * both rows by the LF-mapping in \f$ O( 1 ) \f$ time each. Hence
 * `count( P )` takes \f$ O( p ) \f$ steps, plus the ranks for the intervals
 * whose tag is not \f$ c \f$.
 *
 * The function \f$ \phi( \mathit{SA}[ i ] ) = \mathit{SA}[ i - 1 ] \f$, taken
 * cyclically (\f$ \phi( \mathit{SA}[ 0 ] ) = \mathit{SA}[ n ] \f$), is a
 * permutation of the places of the text that shifts each stretch between two
 * places of the starts of the runs alike, and is a move structure too;
 * `locate( P )` keeps \f$ \mathit{SA}[ \mathit{e} ] \f$ along the search (the
 * toehold, from the samples of \f$ \mathit{SA} \f$ at the end of each
 * interval, with the interval of \f$ \phi \f$ of each), and then lists the
 * occurrences by \f$ \mathit{e} - \mathit{s} \f$ steps of \f$ \phi \f$, each
 * in \f$ O( 1 ) \f$ time.
 *
 * The symbols of \f$ L \f$ are the bytes plus 1, the end of the text being 0;
 * the transform comes from the suffix array of SA-IS [see
 * `BurrowsWheeler.h`].
 *
 * The contracts [see `Contract.h`]: \f$ n < 2^{32} \f$, a pattern is not
 * empty; each place found by `locate()` is an occurrence, an audit against
 * the text kept for it only at the level of the audits. The functions are
 * defined in `MoveRIndex.tpp`.
 *
 * \see N. Bertram, J. Fischer, L. Nalbach, Move-r: optimizing the r-index,
 *      Proc. 22nd SEA, LIPIcs 301, 1, 2024, doi:10.4230/LIPIcs.SEA.2024.1
 * \see T. Nishimoto, Y. Tabei, Optimal-time queries on BWT-runs compressed
 *      indexes, Proc. 48th ICALP, LIPIcs 198, 101, 2021,
 *      doi:10.4230/LIPIcs.ICALP.2021.101
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

#ifndef __ADS_MoveRIndex
 #define __ADS_MoveRIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"
#include "ADS/succinct/HuffmanWaveletTree.h"
#include "ADS/succinct/MoveStructure.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MoveRIndex ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the r-index of a text on move structures: count and locate with each
/// step in \f$ O( 1 ) \f$ time
class MoveRIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of the empty text
 MoveRIndex( void ) : MoveRIndex( std::string_view() ) {}

 /// the index of \f$ s \f$
 ///
 /// In pseudocode (the move-r of Bertram, Fischer and Nalbach, beyond the
 /// book), with the runs of \f$ L \f$ from \f$ p_j \f$ with the head
 /// \f$ h_j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{MoveRIndex}( s ) \cr
 /// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s\$;\ L[ i ]
 ///     \gets s[ \mathit{SA}[ i ] - 1 ] + 1 \text{, 0 for the } \$ \cr
 /// 3 & \quad \textbf{foreach}\ \text{run } j \text{ of } L\ \textbf{do}
 ///     \ q_j \gets C[ h_j ] + \text{the } h_j \text{ in } L[ 0 , p_j ) \cr
 /// 4 & \quad \mathit{LF} \gets \text{the move structure of } p \mapsto q
 ///     \text{, tagged by } h \cr
 /// 5 & \quad \mathit{tag} \gets \text{the tags of its intervals, in a }
 ///     \text{wavelet tree of Huffman} \cr
 /// 6 & \quad \phi \gets \text{the move structure of } \mathit{SA}[ p_j ]
 ///     \mapsto \mathit{SA}[ p_j - 1 ] \text{, cyclically} \cr
 /// 7 & \quad \textbf{foreach}\ \text{interval } i \text{ of }
 ///     \mathit{LF}\ \textbf{do} \cr
 /// 8 & \qquad x \gets \mathit{SA}[ \text{the last row of } i ];
 ///     \ \mathit{end}[ i ] \gets x \cr
 /// 9 & \qquad \mathit{endPhi}[ i ] \gets \text{the interval of } \phi
 ///     \text{ of } x - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ |s| < 2^{32} \f$
 explicit MoveRIndex( std::string_view s );

 /// the length \f$ n \f$ of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number of the intervals of the LF-mapping, after the balance
 [[nodiscard]] std::size_t intervals( void ) const {
  return( f_lf.intervals() );
  }

 /// the number of the occurrences of \f$ P \f$
 ///
 /// In pseudocode (the backward search of the move-r, beyond the book),
 /// \f$ \mathit{s} \f$ and \f$ \mathit{e} \f$ with their intervals
 /// \f$ i_s \f$ and \f$ i_e \f$, which \f$ \mathrm{move} \f$ keeps:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{count}( P ) \cr
 /// 2 & \quad ( \mathit{s} , i_s ) \gets ( 0 , 0 );\ ( \mathit{e} , i_e )
 ///     \gets ( n , \text{the last interval} ) \cr
 /// 3 & \quad \textbf{for}\ i \gets p - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad c \gets P[ i ] + 1 \cr
 /// 5 & \qquad \textbf{if}\ \mathit{tag}[ i_s ] \ne c\ \textbf{then} \cr
 /// 6 & \qquad\quad i_s \gets \text{the next interval of tag } c \text{ }
 ///     \text{after } i_s \text{, or } \textbf{return}\ 0 \cr
 /// 7 & \qquad\quad \mathit{s} \gets \text{the start of } i_s \cr
 /// 8 & \qquad \textbf{if}\ \mathit{tag}[ i_e ] \ne c\ \textbf{then} \cr
 /// 9 & \qquad\quad i_e \gets \text{the previous interval of tag } c \text{ }
 ///     \text{before } i_e \text{, or } \textbf{return}\ 0 \cr
 /// 10 & \qquad\quad \mathit{e} \gets \text{the end of } i_e \cr
 /// 11 & \qquad \textbf{if}\ \mathit{s} > \mathit{e}\ \textbf{then return}
 ///     \ 0 \cr
 /// 12 & \qquad ( \mathit{s} , i_s ) \gets \mathit{LF}.\mathrm{move}(
 ///     \mathit{s} , i_s );\ ( \mathit{e} , i_e ) \gets
 ///     \mathit{LF}.\mathrm{move}( \mathit{e} , i_e ) \cr
 /// 13 & \quad \textbf{return}\ \mathit{e} - \mathit{s} + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of \f$ P \f$, in increasing order
 ///
 /// In pseudocode (the move-r of Bertram, Fischer and Nalbach, beyond the
 /// book), the search of `count()` keeping the toehold
 /// \f$ v = \mathit{SA}[ \mathit{e} ] \f$ with its interval \f$ i_v \f$ of
 /// \f$ \phi \f$, and with \f$ i_e \f$ in the line 5 already moved to the
 /// previous interval of \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locate}( P ) \cr
 /// 2 & \quad v \gets \mathit{end}[ \text{the last interval} ];\ i_v \gets
 ///     \text{the interval of } \phi \text{ of } v \cr
 /// 3 & \quad \text{run the search of } \mathrm{count}( P ) \text{, and at }
 ///     \text{each step, before the moves:} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{tag}[ i_e ] = c\ \textbf{then}\ v \gets v
 ///     - 1;\ \textbf{if}\ v \text{ is before the start of } i_v
 ///     \ \textbf{then}\ i_v \gets i_v - 1 \cr
 /// 5 & \qquad \textbf{else}\ v \gets \mathit{end}[ i_e ] - 1;\ i_v \gets
 ///     \mathit{endPhi}[ i_e ] \cr
 /// 6 & \quad \textbf{if}\ P \text{ does not occur}\ \textbf{then return}
 ///     \ \emptyset \cr
 /// 7 & \quad R \gets \{ v \} \cr
 /// 8 & \quad \textbf{for}\ k \gets \mathit{s}\ \textbf{to}\ \mathit{e} - 1
 ///     \ \textbf{do}\ ( v , i_v ) \gets \phi.\mathrm{move}( v , i_v );\ R
 ///     \gets R \cup \{ v \} \cr
 /// 9 & \quad \textbf{return}\ R \text{ sorted}
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the bits of the index: the two move structures, the ranks of the tags
 /// and the samples
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the backward search of \f$ P \f$: the rows \f$ [ s , e ] \f$ with their
 /// intervals, and the place of the suffix of \f$ e \f$ with its interval
 /// of \f$ \phi \f$ if `Toehold`; false if \f$ P \f$ does not occur
 template< bool Toehold >
 [[nodiscard]] bool search( std::string_view P , std::uint64_t & s ,
                            std::size_t & is , std::uint64_t & e ,
                            std::size_t & ie , std::uint64_t & v ,
                            std::size_t & iv ) const;

 std::size_t f_n = 0;
 int f_w = 1 , f_wi = 1;                 ///< the widths of the samples
 MoveStructure f_lf;                     ///< LF on the runs
 HuffmanWaveletTree< RankSelect > f_tag;   ///< their tags, ranked
 std::array< std::size_t , 257 > f_cnt{};   ///< the intervals of each tag
 MoveStructure f_phi;                    ///< \f$ \phi \f$ on the places
 BitVector f_end;                        ///< the samples at the interval ends
 BitVector f_endPhi;                     ///< the interval of each sample - 1
#if ADS_CONTRACTS >= 3
 std::string f_text;                     ///< the text, for the audits
#endif

 };  // end( class MoveRIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/MoveRIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MoveRIndex.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MoveRIndex.h -------------------------*/
/*--------------------------------------------------------------------------*/
