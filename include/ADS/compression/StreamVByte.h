/*--------------------------------------------------------------------------*/
/*--------------------------- File StreamVByte.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Stream VByte code of Lemire, Kurz and Rupp for integers of 32 bits,
 * beyond the variable-byte code of the notes [see IntegerCodes.h]: each
 * integer takes 1 to 4 bytes, as many as its significant bytes, but the
 * lengths are not in the bytes themselves (a flag bit per byte, which the
 * decoder of the notes tests one byte at a time); they are apart, 2 bits
 * per integer, 4 integers to a control byte, before all the bytes of the
 * data. A decoder then reads a control byte and knows where the 4 integers
 * start, with no test on the data: on a processor with the shuffle of
 * bytes of SSSE3, the 16 bytes from the place of the first integer are
 * moved to their 4 places of 4 bytes, the missing high bytes made 0, by one
 * shuffle whose mask is the entry of the control byte in a table of 256
 * masks, and the place moves by the sum of the 4 lengths, from a table too.
 * The encoder, with SSE4.1, finds the lengths of 8 integers at once by
 * saturating arithmetic on their bytes (a byte not 0 made 1, each half of
 * 16 bits made 0, 1 or 255, the pair of halves capped and lifted so that
 * its two high bits are the length less 1), the 2 control bytes being one
 * mask of 16 bytes, and moves their bytes together by the shuffle of the
 * inverse table; the decoder takes 8 control bytes in a word and decodes
 * 32 integers with no test while 12 more follow; elsewhere the integers
 * are moved one at a time, still with no test. With \f$ \ell_i =
 * \max( 1 , \lceil \mathrm{bitwidth}( x_i ) / 8 \rceil ) \f$ the bytes of
 * \f$ x_i \f$, the code takes
 * \f[ \lceil n / 4 \rceil + \sum_{i=0}^{n-1} \ell_i \f]
 * bytes, i.e., the variable-byte code with 2 control bits per integer
 * instead of 1 per byte.
 *
 * The format: the \f$ \lceil n / 4 \rceil \f$ control bytes, the lengths
 * less 1 of the integers 4i, 4i + 1, 4i + 2 and 4i + 3 in the bits 0-1,
 * 2-3, 4-5 and 6-7 of the byte i, then the bytes of the integers,
 * little-endian; n is not written, and the decoder takes it.
 *
 * The contracts [see Contract.h]: the decoder gets the code of n integers.
 * The functions are defined in StreamVByte.tpp.
 *
 * \see D. Lemire, N. Kurz, C. Rupp, Stream VByte: faster byte-oriented
 *      integer compression, Information Processing Letters 130, 1-6, 2018,
 *      doi:10.1016/j.ipl.2017.09.011
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

#ifndef __ADS_StreamVByte
 #define __ADS_StreamVByte
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#if defined( __SSSE3__ ) || defined( __SSE4_1__ ) || defined( __BMI2__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the Stream VByte code of x: the control bytes, then the data
[[nodiscard]] inline std::vector< std::uint8_t > streamVByteEncode(
                                         std::span< const std::uint32_t > x );

/// the n integers of the Stream VByte code
/// \pre code is the code of n integers
[[nodiscard]] inline std::vector< std::uint32_t > streamVByteDecode(
                        std::span< const std::uint8_t > code , std::size_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/StreamVByte.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StreamVByte.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File StreamVByte.h -------------------------*/
/*--------------------------------------------------------------------------*/
