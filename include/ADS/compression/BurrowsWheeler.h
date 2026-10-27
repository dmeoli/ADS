/*--------------------------------------------------------------------------*/
/*------------------------- File BurrowsWheeler.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The transform of Burrows and Wheeler and the compressor built on it (chapter
 * 14, Block-Sorting Compression, of Ferragina's book, beyond the book of
 * Montresor). The rows of the matrix \f$ M' \f$ are the rotations of
 * \f$ s\$ \f$, with \f$ \$ \f$ a sentinel smaller than every symbol, in sorted
 * order; the transform `bwt()` is its last column \f$ L \f$ without the
 * \f$ \$ \f$, with the row \f$ r \f$ of the \f$ \$ \f$. Since the rows that
 * start with a string \f$ w \f$ are contiguous, the symbols that precede the
 * occurrences of \f$ w \f$ are a substring of \f$ L \f$, and on a text whose
 * symbols depend on their contexts, as those of a language do, \f$ L \f$ is
 * made of runs of few distinct symbols. The rows of \f$ M' \f$ are the
 * suffixes of \f$ s\$ \f$ in order, so that
 * \f$ L[ i ] = s[ \mathit{SA}[ i ] - 1 ] \f$; the suffix array
 * \f$ \mathit{SA} \f$ is that of the library [see `SuffixArray.h`], built by
 * the induced sorting SA-IS of Nong, Zhang and Chan in \f$ O( n ) \f$ time,
 * with the suffix \f$ \$ \f$ put first. The inverse transform `inverseBwt()`
 * uses the LF-mapping: the \f$ k \f$-th occurrence of \f$ c \f$ in \f$ L \f$
 * and the \f$ k \f$-th in the first column \f$ F \f$ are the same symbol of
 * \f$ s \f$, since the rows that end with \f$ c \f$ and those that start with
 * it are in the same order, that of the rest of their rotations; hence
 * \f$ \mathit{LF}[ i ] = C[ L[ i ] ] + \mathrm{rank}_{L[ i ]}( L , i ) \f$,
 * with \f$ C[ c ] \f$ the number of the symbols smaller than \f$ c \f$. From
 * the row 0, which is \f$ \$ s \f$ and ends with \f$ s[ n - 1 ] \f$, each step
 * of \f$ \mathit{LF} \f$ moves one symbol to the left in \f$ s \f$, and
 * \f$ n \f$ steps take \f$ O( n ) \f$ time, each step a likely miss of the
 * cache.
 *
 * The transforms that follow are described in the same chapter. The
 * move-to-front of Bentley, Sleator, Tarjan and Wei replaces each symbol with
 * its place in a list of the alphabet and moves it to the front, so that the
 * runs of few symbols of \f$ L \f$ become runs of small integers, in
 * \f$ O( |\Sigma| ) \f$ time per symbol here; its \f$ \gamma \f$-code takes at
 * most \f$ 2 H_0 + 1 \f$ bits per symbol, since the place of a symbol is at
 * most its distance from its previous occurrence and the logarithm is concave.
 * The run-length encoding replaces a run of \f$ \ell \f$ symbols \f$ c \f$
 * with \f$ \langle \ell , c \rangle \f$. The bzip chains them: the transform
 * of each block, the move-to-front of \f$ L \f$, then the run-length encoding
 * RLE0 of the runs of 0 alone, the other integers grown by 1 and a run of
 * \f$ m \f$ zeros written in the code of Wheeler (the binary digits of
 * \f$ m + 1 \f$ after its leading 1, as the symbols 0 and 1), and at last an
 * entropy code, here the canonical code of Huffman of the block with lengths
 * up to 20 [see `CanonicalHuffman.h`], as in bzip2 (which writes the digits of
 * the runs from the least significant one and splits the block in groups coded
 * by up to 6 tables). Coding \f$ L \f$ by contexts gives the entropy of order
 * \f$ k \f$ for every \f$ k \f$ at once (the boosting of Ferragina, Giancarlo,
 * Manzini and Sciortino), and Manzini bounds the chain of the transform, MTF,
 * RLE and an arithmetic code by \f$ ( 5 + \epsilon ) n H_k^*( s ) \f$ bits
 * plus lower terms, \f$ H_k^* \f$ a variant of the entropy of order \f$ k \f$
 * [see `Entropy.h`]. The transform is also an index [see `FMIndex.h`].
 *
 * The contracts [see `Contract.h`]: the row \f$ r \f$ is that of a
 * transform, each symbol of a move-to-front is in its alphabet of distinct
 * symbols, a run is positive, the integers of RLE0 are at most 256, a block
 * is positive; the inverse of each transform gives back its input, an
 * audit. The functions are defined in `BurrowsWheeler.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14, Block-Sorting Compression,
 *      doi:10.1017/9781009128933
 * \see M. Burrows, D. J. Wheeler, A block-sorting lossless data compression
 *      algorithm, Technical Report 124, Digital Systems Research Center,
 *      1994
 * \see J. L. Bentley, D. D. Sleator, R. E. Tarjan, V. K. Wei, A locally
 *      adaptive data compression scheme, Communications of the ACM 29(4),
 *      320-330, 1986, doi:10.1145/5684.5688
 * \see G. Manzini, An analysis of the Burrows-Wheeler transform, Journal
 *      of the ACM 48(3), 407-430, 2001, doi:10.1145/382780.382782
 * \see P. Ferragina, R. Giancarlo, G. Manzini, M. Sciortino, Boosting
 *      textual compression in optimal linear time, Journal of the ACM
 *      52(4), 688-713, 2005, doi:10.1145/1082036.1082043
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
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

#ifndef __ADS_BurrowsWheeler
 #define __ADS_BurrowsWheeler
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/PackedBits.h"
#include "ADS/string/SuffixArray.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the transform of Burrows and Wheeler: the last column of \f$ M' \f$
/// without the \f$ \$ \f$, and the row of the \f$ \$ \f$
struct BWT {
 std::string last;      ///< the last column \f$ L \f$ without the \f$ \$ \f$
 std::size_t primary;   ///< the row \f$ r \f$ of the \f$ \$ \f$ in \f$ L \f$
 /// whether the two transforms have the same column and row
 friend bool operator==( const BWT & a , const BWT & b ) = default;
 };

/// the transform of \f$ s \f$
///
/// In pseudocode (the transform of Burrows and Wheeler, beyond the book), on
/// the suffix array of \f$ s\$ \f$, whose row \f$ i \f$ ends with the symbol
/// before the suffix \f$ \mathit{SA}[ i ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bwt}( s ) \cr
/// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s\$ \text{, with }
///     \mathit{SA}[ 0 ] = n \cr
/// 3 & \quad L \gets \text{an empty string} \cr
/// 4 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \mathit{SA}[ i ] = 0\ \textbf{then}\ r \gets i \cr
/// 6 & \qquad \textbf{else}\ L \gets L \cdot s[ \mathit{SA}[ i ] - 1 ] \cr
/// 7 & \quad \textbf{return}\ ( L , r )
/// \end{array}}
/// \f]
/// \post `inverseBwt( result ) == s` (an audit)
[[nodiscard]] inline BWT bwt( std::string_view s );

/// the LF-mapping of the \f$ n + 1 \f$ rows of the transform \f$ b \f$
///
/// In pseudocode (the LF-mapping of Burrows and Wheeler, beyond the book),
/// with the \f$ \$ \f$ at the row \f$ r \f$ the smallest symbol; the rows
/// with the same last symbol keep their order in the first column:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lfMapping}( L , r ) \cr
/// 2 & \quad C[ c ] \gets \text{the number of the symbols of } L\$ \text{ }
///     \text{smaller than } c \text{, for each } c \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad c \gets \text{the symbol of the row } i\quad \triangleright
///     \ \text{the } \$ \text{ if } i = r \cr
/// 5 & \qquad \mathit{LF}[ i ] \gets C[ c ];\ C[ c ] \gets C[ c ] + 1 \cr
/// 6 & \quad \textbf{return}\ \mathit{LF}
/// \end{array}}
/// \f]
/// \pre `b.primary` is 0 if `b.last` is empty, in \f$ [ 1 , n ] \f$
///      otherwise
[[nodiscard]] inline std::vector< std::size_t > lfMapping( const BWT & b );

/// the text of the transform \f$ b \f$
///
/// In pseudocode (the inverse transform of Burrows and Wheeler, beyond the
/// book), each step of \f$ \mathit{LF} \f$ going one symbol to the left in
/// \f$ s \f$ from the row 0, which is \f$ \$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{inverseBwt}( L , r ) \cr
/// 2 & \quad \mathit{LF} \gets \mathrm{lfMapping}( L , r );\ k \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
/// 4 & \qquad s[ i ] \gets \text{the symbol of the row } k \text{ of } L \cr
/// 5 & \qquad k \gets \mathit{LF}[ k ] \cr
/// 6 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre as `lfMapping()`
[[nodiscard]] inline std::string inverseBwt( const BWT & b );

/// the places of the symbols of \f$ s \f$ in the list `alphabet`, each moved
/// to the front after its use
///
/// In pseudocode (the move-to-front of Bentley, Sleator, Tarjan and Wei,
/// beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{moveToFront}( s , \Sigma ) \cr
/// 2 & \quad \mathit{list} \gets \text{the symbols of } \Sigma \text{ in }
///     \text{their order} \cr
/// 3 & \quad \textbf{foreach}\ c \in s\ \textbf{do} \cr
/// 4 & \qquad p \gets \text{the place of } c \text{ in } \mathit{list};
///     \ \text{append } p \text{ to } x \cr
/// 5 & \qquad \text{move } c \text{ to the front of } \mathit{list} \cr
/// 6 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre `alphabet` has distinct symbols, each symbol of \f$ s \f$ in it
/// \post `inverseMoveToFront( result , alphabet ) == s` (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > moveToFront(
                              std::string_view s , std::string_view alphabet );

/// the move-to-front of `moveToFront()` above, with the 256 bytes in their
/// order
///
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{moveToFront}( s ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{moveToFront}( s , \text{the bytes } 0 ,
///     \ldots , 255 )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::uint8_t > moveToFront(
                                                         std::string_view s );

/// the symbols of the places \f$ x \f$ in the list `alphabet`
///
/// In pseudocode (the move-to-front of Bentley, Sleator, Tarjan and Wei,
/// beyond the book), the list moved as `moveToFront()` moves it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{inverseMoveToFront}( x , \Sigma ) \cr
/// 2 & \quad \mathit{list} \gets \text{the symbols of } \Sigma \text{ in }
///     \text{their order} \cr
/// 3 & \quad \textbf{foreach}\ p \in x\ \textbf{do} \cr
/// 4 & \qquad c \gets \mathit{list}[ p ];\ s \gets s \cdot c \cr
/// 5 & \qquad \text{move } c \text{ to the front of } \mathit{list} \cr
/// 6 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre `alphabet` has distinct symbols, each place below its size
[[nodiscard]] inline std::string inverseMoveToFront(
                                       const std::vector< std::uint8_t > & x ,
                                       std::string_view alphabet );

/// the inverse of `inverseMoveToFront()` above, with the 256 bytes in their
/// order
///
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{inverseMoveToFront}( x ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{inverseMoveToFront}( x , \text{the }
///     \text{bytes } 0 , \ldots , 255 )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::string inverseMoveToFront(
                                      const std::vector< std::uint8_t > & x );

/// the maximal runs of \f$ s \f$ as pairs \f$ ( \ell , c ) \f$ of a length
/// and a symbol
///
/// In pseudocode (the run-length encoding, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{runLengthEncode}( s ) \cr
/// 2 & \quad i \gets 0;\ R \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 4 & \qquad j \gets i \cr
/// 5 & \qquad \textbf{while}\ j < n\ \textbf{and}\ s[ j ] = s[ i ]
///     \ \textbf{do}\ j \gets j + 1 \cr
/// 6 & \qquad \text{append } ( j - i , s[ i ] ) \text{ to } R;\ i \gets j \cr
/// 7 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post `runLengthDecode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< std::pair< std::size_t , char > >
                                        runLengthEncode( std::string_view s );

/// the text of the runs \f$ R \f$
///
/// In pseudocode (the run-length encoding, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{runLengthDecode}( R ) \cr
/// 2 & \quad s \gets \text{an empty string} \cr
/// 3 & \quad \textbf{foreach}\ ( \ell , c ) \in R\ \textbf{do}\ s \gets s
///     \cdot c^{\ell} \cr
/// 4 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre each length positive
[[nodiscard]] inline std::string runLengthDecode(
                   const std::vector< std::pair< std::size_t , char > > & R );

/// the RLE0 of bzip: the integers of \f$ x \f$ but 0 grown by 1, each run of
/// \f$ m \f$ zeros in the code of Wheeler, the digits of \f$ m + 1 \f$ after
/// the leading 1
///
/// In pseudocode (the RLE0 of bzip2, beyond the book), the digits of a run
/// written from the most significant one:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zeroRunEncode}( x ) \cr
/// 2 & \quad i \gets 0;\ y \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ i < |x|\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x[ i ] \ne 0\ \textbf{then} \cr
/// 5 & \qquad\quad \text{append } x[ i ] + 1 \text{ to } y;\ i \gets i + 1;
///     \ \textbf{continue} \cr
/// 6 & \qquad j \gets i \cr
/// 7 & \qquad \textbf{while}\ j < |x|\ \textbf{and}\ x[ j ] = 0\ \textbf{do}
///     \ j \gets j + 1 \cr
/// 8 & \qquad \text{append the binary digits of } j - i + 1 \text{, but its }
///     \text{leading 1, to } y \cr
/// 9 & \qquad i \gets j \cr
/// 10 & \quad \textbf{return}\ y
/// \end{array}}
/// \f]
/// \post `zeroRunDecode( result ) == x` (an audit)
[[nodiscard]] inline std::vector< std::uint32_t > zeroRunEncode(
                                       const std::vector< std::uint8_t > & x );

/// the integers of an RLE0
///
/// In pseudocode (the RLE0 of bzip2, beyond the book), a maximal run of the
/// digits 0 and 1 read as a binary number with a leading 1:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zeroRunDecode}( y ) \cr
/// 2 & \quad i \gets 0;\ x \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ i < |y|\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ y[ i ] \ge 2\ \textbf{then} \cr
/// 5 & \qquad\quad \text{append } y[ i ] - 1 \text{ to } x;\ i \gets i + 1;
///     \ \textbf{continue} \cr
/// 6 & \qquad v \gets 1 \cr
/// 7 & \qquad \textbf{while}\ i < |y|\ \textbf{and}\ y[ i ] < 2\ \textbf{do}
///     \ v \gets 2 v + y[ i ];\ i \gets i + 1 \cr
/// 8 & \qquad \text{append } v - 1 \text{ zeros to } x \cr
/// 9 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre each integer at most 256
[[nodiscard]] inline std::vector< std::uint8_t > zeroRunDecode(
                                      const std::vector< std::uint32_t > & y );

/// the bzip of \f$ s \f$: blocks of at most `block` symbols, each
/// transformed, moved to front, coded by RLE0 and by its canonical code of
/// Huffman
///
/// In pseudocode (the chain of bzip2, beyond the book), the lengths of the
/// code limited to 20 bits and written in a preamble of 5 bits each:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bzipCompress}( s , \mathit{block} ) \cr
/// 2 & \quad \text{write } n \cr
/// 3 & \quad \textbf{foreach}\ t \text{ block of } \mathit{block} \text{ }
///     \text{symbols of } s \text{, in order}\ \textbf{do} \cr
/// 4 & \qquad ( L , r ) \gets \mathrm{bwt}( t );\ y \gets
///     \mathrm{zeroRunEncode}( \mathrm{moveToFront}( L ) ) \cr
/// 5 & \qquad H \gets \text{the canonical code of Huffman of the }
///     \text{frequencies of } y \cr
/// 6 & \qquad \text{write } |t| , r , |y| \text{ and the lengths of } H \cr
/// 7 & \qquad \textbf{foreach}\ v \in y\ \textbf{do}\ \text{write } H( v ) \cr
/// 8 & \quad \textbf{return}\ \text{the bits written}
/// \end{array}}
/// \f]
/// \pre `block > 0`
/// \post `bzipDecompress( result ) == s` (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > bzipCompress(
                            std::string_view s , std::size_t block = 900000 );

/// the text of a bzip
///
/// In pseudocode (the chain of bzip2, beyond the book), each block undone in
/// the opposite order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bzipDecompress}( \mathit{code} ) \cr
/// 2 & \quad \text{read } n;\ s \gets \text{an empty string} \cr
/// 3 & \quad \textbf{while}\ |s| < n\ \textbf{do} \cr
/// 4 & \qquad \text{read } m , r , k \text{ and the lengths of the code } H
///     \cr
/// 5 & \qquad y \gets \text{the } k \text{ symbols decoded by } H \cr
/// 6 & \qquad L \gets \mathrm{inverseMoveToFront}( \mathrm{zeroRunDecode}( y
///     ) ) \cr
/// 7 & \qquad s \gets s \cdot \mathrm{inverseBwt}( L , r ) \cr
/// 8 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre `code` is a bzip
[[nodiscard]] inline std::string bzipDecompress(
                                     std::span< const std::uint8_t > code );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/BurrowsWheeler.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BurrowsWheeler.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BurrowsWheeler.h ------------------------*/
/*--------------------------------------------------------------------------*/
