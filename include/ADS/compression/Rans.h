/*--------------------------------------------------------------------------*/
/*------------------------------ File Rans.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range variant of the asymmetric numeral systems of Duda (rANS), the
 * entropy coder of order 0 of the fast compressors, beyond the arithmetic
 * code of the notes [see ArithmeticCoding.h]: it reaches the size of the
 * arithmetic code, within a fraction of a percent of \f$ n H_0 \f$ [see
 * Entropy.h], at the speed of a table of Huffman.
 *
 * The state is an integer x, and a symbol s of frequency \f$ f_s \f$ and
 * cumulative frequency \f$ c_s \f$ over a total \f$ M = 2^{12} \f$ moves it
 * to \f$ C( s , x ) = M \lfloor x / f_s \rfloor + c_s + x \bmod f_s \f$,
 * which grows x by a factor \f$ M / f_s \f$, i.e., by
 * \f$ \log_2 ( M / f_s ) \f$ bits, the information of s; the decoder
 * inverts it, \f$ s \f$ being the symbol whose range \f$ [ c_s , c_s + f_s )
 * \f$ holds \f$ x \bmod M \f$ (a table of M entries) and
 * \f$ x = f_s \lfloor x / M \rfloor + x \bmod M - c_s \f$. The state is kept
 * in \f$ [ 2^{16} , 2^{32} ) \f$ by moving 16 bits at a time to the stream
 * (renormalization), at most once per symbol; the decoder reads them back
 * in the opposite order, so that the encoder works from the last symbol to
 * the first. Unlike the arithmetic coder, which narrows an interval and
 * emits its common bits, the state needs one multiplication per symbol and
 * no division in the decoder, and its renormalization has no carry.
 *
 * Four states take the symbols in turn, so that four chains of dependences
 * overlap in the pipeline of the processor (the interleaving of Giesen);
 * they share one stream. The frequencies are scaled to M with each byte of
 * the string kept at 1 at least, which costs a fraction of a percent on a
 * string of bytes; the tabled variant [see Tans.h] makes the state
 * transitions a lookup and needs no multiplication, at the price of a
 * table per alphabet built in the encoder too.
 *
 * The format: the size n in 8 bytes, the 256 frequencies scaled to M in 2
 * bytes each, the four final states in 4 bytes each, the stream of 16 bits
 * words, and 8 bytes of 0; the integers are little-endian.
 *
 * The contracts [see Contract.h]: ransDecompress() gets the output of
 * ransCompress(), whose header (the frequencies sum to M, the states and
 * the stream are in range) it checks; the audit of ransCompress() is that
 * the output decompresses to the input. The functions are defined in
 * Rans.tpp.
 *
 * \see J. Duda, Asymmetric numeral systems: entropy coding combining speed
 *      of Huffman coding with compression rate of arithmetic coding,
 *      arXiv:1311.2540, 2013
 * \see F. Giesen, Interleaved entropy coders, arXiv:1402.3392, 2014
 * \see T. Granlund, P. L. Montgomery, Division by invariant integers using
 *      multiplication, Proc. ACM PLDI 1994, 61-72, 1994,
 *      doi:10.1145/178243.178249
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

#ifndef __ADS_Rans
 #define __ADS_Rans
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the rANS code of order 0 of the bytes of in, with the frequencies and
/// the size
[[nodiscard]] inline std::vector< std::uint8_t > ransCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that ransCompress() coded in in
/// \pre in is an output of ransCompress()
[[nodiscard]] inline std::vector< std::uint8_t > ransDecompress(
                                          std::span< const std::uint8_t > in );

/// the frequencies f scaled to sum to 2^b, each positive one kept positive
/// \pre some frequency is positive, and at most 2^b are
[[nodiscard]] inline std::array< std::uint32_t , 256 > scaleFrequencies(
                       const std::array< std::uint64_t , 256 > & f , int b );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/Rans.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Rans.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Rans.h -----------------------------*/
/*--------------------------------------------------------------------------*/
