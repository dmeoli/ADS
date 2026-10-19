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
 * The encoder, with SSE4.1, finds the 4 lengths by comparisons of the 4
 * integers at once and moves their bytes together by the shuffle of the
 * inverse table. Elsewhere the integers are moved one at a time, still
 * with no test. The
 * size is that of the variable-byte code up to the control bits, 2 bits
 * per integer instead of 1 per byte, and an integer is at least one byte.
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
#include <vector>

#if defined( __SSSE3__ ) || defined( __SSE4_1__ )
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
