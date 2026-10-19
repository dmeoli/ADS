/*--------------------------------------------------------------------------*/
/*-------------------------- File HuffmanCoder.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The code of Huffman of a string of bytes as the fast coders do it, beyond
 * the canonical code of the notes [see CanonicalHuffman.h], which decodes
 * a bit at a time on a map of the symbols:
 *
 * - the lengths are the optimal ones of at most 11 bits, by the
 *   package-merge of Larmore and Hirschberg, in \f$ O( \sigma L ) \f$
 *   steps (the code of Huffman itself when its lengths fit), so that the
 *   decoding table, \f$ 2^{11} \f$ entries of 2 bytes, stays in the cache
 *   of level 1;
 * - the code is canonical, and the preamble is the lengths of the 256
 *   bytes, 4 bits each;
 * - the codewords are written from the least significant bit of a word of
 *   64 bits (the bits of a codeword turned around, so that the code stays
 *   prefix-free in the order of reading), 4 of them joined by shifts and
 *   written at once, whole bytes at a time;
 * - the decoder peeks at L <= 11 bits and finds in a table of
 *   \f$ 2^L \f$ entries the symbols whose codes fit whole in them, up to
 *   3, with their number and their bits, in a few instructions and no
 *   branch (one lookup decodes \f$ \min( 3 , \lfloor 11 / \bar\ell
 *   \rfloor ) \f$ symbols or so for an average length \f$ \bar\ell
 *   \f$, about 2.4 on English text); 4 lookups of 11 bits at most fit in
 *   the bits of a word after a refill, so that a load serves 4 lookups;
 * - the string is cut in four streams, decoded together, so that the four
 *   chains of dependences (a lookup gives the length, which gives the next
 *   lookup) overlap in the pipeline of the processor.
 *
 * These are the decoders by tables of Moffat and Turpin, with the
 * interleaved streams of Zstandard; the speed comes from the table and from
 * the streams, the size of the code is that of Huffman, within one bit per
 * byte of the entropy, \f$ H_0 \le \bar\ell < H_0 + 1 \f$ [see
 * Entropy.h] (the limit of 11 bits costs a fraction of a percent on a text
 * of bytes, the optimum under it being found exactly). A string of one
 * distinct byte is written as that byte.
 *
 * The format: the size n in 8 bytes, the longest length (0 for one
 * distinct byte, which follows), the 128 bytes of the lengths, the sizes of
 * the first three streams in 4 bytes each, the four streams, and 8 bytes of
 * 0 that let a decoder load a word anywhere in its stream; the integers
 * are little-endian.
 *
 * The contracts [see Contract.h]: huffmanDecompress() gets the output of
 * huffmanCompress(), whose header (the lengths satisfy the inequality of
 * Kraft, the sizes of the streams fit in the input) it checks; the audit
 * of huffmanCompress() is that the output decompresses to the input. The
 * functions are defined in HuffmanCoder.tpp.
 *
 * \see Y. Collet, huff0 and FSE, in Zstandard, RFC 8878, 2021,
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
 *         Universita' di Pisa \n
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

/// the code of Huffman of the bytes of in, with the lengths and the size
[[nodiscard]] inline std::vector< std::uint8_t > huffmanCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that huffmanCompress() coded in in
/// \pre in is an output of huffmanCompress()
[[nodiscard]] inline std::vector< std::uint8_t > huffmanDecompress(
                                          std::span< const std::uint8_t > in );

/// the optimal lengths of at most L bits of the codes of the 256 bytes of
/// frequencies f, 0 for a byte of frequency 0, by package-merge
/// \pre 2^L is at least the number of the bytes of positive frequency
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
