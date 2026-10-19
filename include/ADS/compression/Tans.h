/*--------------------------------------------------------------------------*/
/*------------------------------ File Tans.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tabled variant of the asymmetric numeral systems of Duda (tANS), the
 * entropy coder of order 0 of Zstandard (its FSE), beside the range variant
 * [see Rans.h]: the transitions of the state are read from tables, so that
 * a symbol costs a lookup, a shift and an addition, and no multiplication.
 *
 * With \f$ L = 2^R \f$ states, R = 11, and the frequencies \f$ f_s \f$ of
 * the bytes scaled to sum to L (each byte of the string kept at 1 at least
 * [see Rans.h]), the L states are dealt to the symbols, \f$ f_s \f$ each, by
 * the step \f$ L / 2 + L / 8 + 3 \f$ modulo L, which visits them all (it is
 * odd) and spreads the states of a symbol over the table. The decoder in
 * the state \f$ x \in [ 0 , L ) \f$ of the symbol s, its i-th one, emits
 * s and moves to
 * \f[ x' = ( ( f_s + i ) 2^{b} - L ) + r , \qquad
 *     b = R - \lfloor \log_2 ( f_s + i ) \rfloor , \f]
 * r the next b bits of the stream; the encoder in the state
 * \f$ X \in [ L , 2 L ) \f$ emits the low k bits of X, k the number that
 * brings \f$ X / 2^k \f$ into \f$ [ f_s , 2 f_s ) \f$ (one of two values,
 * found by one addition and a shift), and moves to L plus the state of the
 * \f$ ( X / 2^k - f_s ) \f$-th place of s. A symbol costs about
 * \f$ \log_2 ( L / f_s ) \f$ bits, and the code comes within a fraction of a
 * percent of \f$ n H_0 \f$ [see Entropy.h] for \f$ L \f$ a few times the
 * alphabet (Duda; Yokoo and Dubé for the analysis of the spread).
 *
 * The string is cut in two halves, each with a stream of bits of its own
 * and four states that take its symbols in turn; a stream is written
 * forward from the low bit of a word and read backward from its end, 4
 * symbols of 11 bits at most between two loads of the reader, and the two
 * halves are decoded together, so that their chains of dependences
 * overlap. The encoder works from the last symbol to the first, so that
 * the decoder, which reads the stream from its end, meets the bits in the
 * order of the symbols.
 *
 * The format: the size n in 8 bytes, the 256 scaled frequencies in 2 bytes
 * each, the bits of the two streams in 8 bytes each, the streams (the 4
 * final states of the encoder at the end of each, R bits each) and 8 bytes
 * of 0; the integers are little-endian.
 *
 * The contracts [see Contract.h]: tansDecompress() gets the output of
 * tansCompress(), whose header (the frequencies sum to L, the bits of the
 * stream fit in the input) it checks; the audit of tansCompress() is that
 * the output decompresses to the input. The functions are defined in
 * Tans.tpp.
 *
 * \see J. Duda, Asymmetric numeral systems: entropy coding combining speed
 *      of Huffman coding with compression rate of arithmetic coding,
 *      arXiv:1311.2540, 2013
 * \see J. Duda, K. Tahboub, N. J. Gadgil, E. J. Delp, The use of
 *      asymmetric numeral systems as an accurate replacement for Huffman
 *      coding, Proc. 31st Picture Coding Symposium (PCS 2015), 65-69, 2015,
 *      doi:10.1109/PCS.2015.7170048
 * \see H. Yokoo, T. Dube, A study on the effect of the spread function in
 *      tANS coding, arXiv:2104.14434, 2021
 * \see Y. Collet, M. Kucherawy, Zstandard Compression and the
 *      'application/zstd' Media Type, RFC 8878, 2021, doi:10.17487/RFC8878
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

#ifndef __ADS_Tans
 #define __ADS_Tans
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/Rans.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#ifdef __BMI2__
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the tANS code of order 0 of the bytes of in, with the frequencies and
/// the size
[[nodiscard]] inline std::vector< std::uint8_t > tansCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that tansCompress() coded in in
/// \pre in is an output of tansCompress()
[[nodiscard]] inline std::vector< std::uint8_t > tansDecompress(
                                          std::span< const std::uint8_t > in );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Tans.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Tans.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Tans.h -----------------------------*/
/*--------------------------------------------------------------------------*/
