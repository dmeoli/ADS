/*--------------------------------------------------------------------------*/
/*-------------------------- File HuffmanCoder.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The code of Huffman of a string of bytes as the fast coders do it, beyond
 * the canonical code of chapter 12, Statistical Coding, of Ferragina's book
 * [see `CanonicalHuffman.h`], which decodes a bit at a time on a map of the
 * symbols:
 *
 * - the lengths are the optimal ones of at most 11 bits, by the
 *   package-merge of Larmore and Hirschberg, in \f$ O( \sigma L ) \f$
 *   steps for \f$ \sigma \f$ symbols and the limit \f$ L \f$ (the code of
 *   Huffman itself when its lengths fit), so that the decoding table,
 *   \f$ 2^{11} \f$ entries of 2 bytes, stays in the cache of level 1;
 * - the code is canonical, and the preamble is the lengths of the 256
 *   bytes, 4 bits each;
 * - the codewords are written from the least significant bit of a word of
 *   64 bits (the bits of a codeword turned around, so that the code stays
 *   prefix-free in the order of reading), 4 of them joined by shifts and
 *   written at once, whole bytes at a time;
 * - the decoder peeks at \f$ W \f$ bits and finds in a table of
 *   \f$ 2^W \f$ entries of 32 bits the symbols whose codes fit whole in
 *   them, up to 3 in the low 24 bits, then their bits in 4 and their number
 *   in 2, so that a lookup costs two shifts and a mask and no branch (it
 *   decodes \f$ \min( 3 , \lfloor W / \bar\ell \rfloor ) \f$ symbols or so
 *   for an average length \f$ \bar\ell \f$); \f$ W = 12 \f$ from
 *   \f$ 2^{18} \f$ bytes on (a table of 16 KB, more symbols a lookup) and
 *   \f$ W = 11 \f$ below (half the table to build); 4 lookups of 12 bits at
 *   most fit in the bits of a word after a refill, so that a load serves 4
 *   lookups;
 * - the string is cut in four streams, decoded together, so that the four
 *   chains of dependences (a lookup gives the length, which gives the next
 *   lookup) overlap in the pipeline of the processor.
 *
 * These are the decoders by tables of Moffat and Turpin, with the
 * interleaved streams of Zstandard; the speed comes from the table and from
 * the streams, the size of the code is that of Huffman, within one bit per
 * byte of the entropy, \f$ H_0 \le \bar\ell < H_0 + 1 \f$ [see
 * `Entropy.h`] (the limit of 11 bits costs a fraction of a percent on a
 * text of bytes, the optimum under it being found exactly). A string of one
 * distinct byte is written as that byte.
 *
 * The format: the size \f$ n \f$ in 8 bytes, the longest length (0 for one
 * distinct byte, which follows), the 128 bytes of the lengths, the sizes of
 * the first three streams in 4 bytes each, the four streams, and 8 bytes of
 * 0 that let a decoder load a word anywhere in its stream; the integers
 * are little-endian.
 *
 * The contracts [see `Contract.h`]: `huffmanDecompress()` gets the output
 * of `huffmanCompress()`, whose header (the lengths satisfy the inequality
 * of Kraft, the sizes of the streams fit in the input) it checks; the audit
 * of `huffmanCompress()` is that the output decompresses to the input. The
 * functions are defined in `HuffmanCoder.tpp`.
 *
 * \see Y. Collet, M. Kucherawy, Zstandard Compression and the
 *      'application/zstd' Media Type, RFC 8878, 2021,
 *      doi:10.17487/RFC8878
 * \see L. L. Larmore, D. S. Hirschberg, A fast algorithm for optimal
 *      length-limited Huffman codes, Journal of the ACM 37(3), 464-473,
 *      1990, doi:10.1145/79147.79150
 * \see A. Moffat, A. Turpin, On the implementation of minimum redundancy
 *      prefix codes, IEEE Transactions on Communications 45(10), 1200-1207,
 *      1997, doi:10.1109/26.634683
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

#ifndef __ADS_HuffmanCoder
 #define __ADS_HuffmanCoder
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the code of Huffman of the bytes of `in`, with the lengths and the size
///
/// In pseudocode (the coders by tables of Moffat and Turpin, beyond the
/// book), \f$ x \f$ the input of \f$ n \f$ bytes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{huffmanCompress}( x ) \cr
/// 2 & \quad f \gets \text{the counts of the bytes of } x \cr
/// 3 & \quad \textbf{if}\ \text{at most one byte has } f > 0
///     \ \textbf{then return}\ ( n , 0 , \text{that byte} ) \cr
/// 4 & \quad \ell \gets \mathrm{limitedHuffmanLengths}( f , 11 );
///     \ c \gets \text{the canonical codes of } \ell \text{, each turned }
///     \text{around} \cr
/// 5 & \quad \text{write } n \text{, } \max \ell \text{ and the lengths }
///     \ell \text{, 4 bits each} \cr
/// 6 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ 3\ \textbf{do}\quad
///     \triangleright\ \text{the four quarters of } x \cr
/// 7 & \qquad \textbf{foreach}\ \text{byte } b \text{ of the quarter } k
///     \ \textbf{do}\ \text{append } c[ b ] \text{ to the stream } k \cr
/// 8 & \qquad \text{pad the stream to a byte, and write its size if }
///     k < 3 \cr
/// 9 & \quad \textbf{return}\ \text{the header, the four streams and 8 }
///     \text{bytes of } 0
/// \end{array}}
/// \f]
/// \post the output decompresses to the input (audit)
[[nodiscard]] inline std::vector< std::uint8_t > huffmanCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that `huffmanCompress()` coded in `in`
///
/// In pseudocode (the decoding by tables of Moffat and Turpin, beyond the
/// book), \f$ T[ v ] \f$ for each value \f$ v \f$ of \f$ W \f$ bits the up
/// to 3 symbols whose codes fit whole in \f$ v \f$, their bits and their
/// number:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{huffmanDecompress}( y ) \cr
/// 2 & \quad \text{read } n \text{ and } \ell;\ \textbf{if}\ \max \ell = 0
///     \ \textbf{then return}\ n \text{ copies of the byte stored} \cr
/// 3 & \quad W \gets 12\ \textbf{if}\ n \ge 2^{18}\ \textbf{else}\ 11 \cr
/// 4 & \quad \textbf{foreach}\ v \in [ 0 , 2^W )\ \textbf{do}
///     \ T[ v ] \gets \text{the symbols decoded from } v \text{, at most 3}
///     \cr
/// 5 & \quad \textbf{while}\ \text{each of the 4 streams has room for}
///     \text{ 12 more symbols}\ \textbf{do} \cr
/// 6 & \qquad \textbf{for}\ j \gets 1\ \textbf{to}\ 4\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{for}\ k \gets 0\ \textbf{to}\ 3\ \textbf{do}\quad
///     \triangleright\ \text{the four chains overlap} \cr
/// 8 & \qquad\qquad e \gets T[ \text{the next } W \text{ bits of the stream }
///     k ];\ \text{output the symbols of } e \text{ and skip its bits} \cr
/// 9 & \qquad \text{reload the word of each stream} \cr
/// 10 & \quad \text{decode the rest one symbol at a time;}\ \textbf{return}
///     \ \text{ the output}
/// \end{array}}
/// \f]
/// \pre `in` is an output of `huffmanCompress()`
[[nodiscard]] inline std::vector< std::uint8_t > huffmanDecompress(
                                          std::span< const std::uint8_t > in );

/// the optimal lengths of at most \f$ L \f$ bits of the codes of the 256
/// bytes of frequencies \f$ f \f$, 0 for a byte of frequency 0, by
/// package-merge
///
/// In pseudocode (the package-merge of Larmore and Hirschberg, beyond the
/// book), \f$ m \f$ the bytes of positive frequency: a byte has the length
/// \f$ \ell \f$ if it is in the cheapest \f$ 2 m - 2 \f$ items at
/// \f$ \ell \f$ of the \f$ L \f$ lists, i.e., the lengths minimize
/// \f$ \sum f \ell \f$ subject to \f$ \sum 2^{-\ell} \le 1 \f$ and
/// \f$ \ell \le L \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{limitedHuffmanLengths}( f , L ) \cr
/// 2 & \quad S \gets \text{the bytes with } f > 0 \text{ by increasing }
///     f;\ \textbf{if}\ |S| \le 1\ \textbf{then return}\ \ldots \cr
/// 3 & \quad \mathit{list} \gets S\quad \triangleright\ \text{items with }
///     \text{the weight } f \cr
/// 4 & \quad \textbf{for}\ l \gets 1\ \textbf{to}\ L - 1\ \textbf{do} \cr
/// 5 & \qquad \mathit{packs} \gets \text{the pairs of consecutive items of }
///     \mathit{list} \text{, each weighing their sum} \cr
/// 6 & \qquad \mathit{list} \gets \mathrm{merge}( S , \mathit{packs} )
///     \quad \triangleright\ \text{by weight} \cr
/// 7 & \quad \ell \gets 0 \cr
/// 8 & \quad \textbf{foreach}\ \text{byte } b \text{ inside the first }
///     2 m - 2 \text{ items of } \mathit{list}\ \textbf{do}
///     \ \ell[ b ] \gets \ell[ b ] + 1 \cr
/// 9 & \quad \textbf{return}\ \ell
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le L \le 64 \f$, and \f$ 2^L \f$ is at least the number of
///      the bytes of positive frequency
[[nodiscard]] inline std::array< std::uint8_t , 256 > limitedHuffmanLengths(
                             const std::array< std::uint64_t , 256 > & f ,
                             int L );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/HuffmanCoder.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HuffmanCoder.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HuffmanCoder.h -------------------------*/
/*--------------------------------------------------------------------------*/
