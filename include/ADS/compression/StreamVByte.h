/*--------------------------------------------------------------------------*/
/*--------------------------- File StreamVByte.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Stream VByte code of Lemire, Kurz and Rupp for integers of 32 bits,
 * beyond the variable-byte code of chapter 11, Integer Coding, of Ferragina's
 * book [see `IntegerCodes.h`]: each integer takes 1 to 4 bytes, as many as its
 * significant bytes, but the lengths are not in the bytes themselves (a flag
 * bit per byte, which the decoder of `decodeVByte()` tests one byte at a
 * time); they are apart, 2 bits per integer, 4 integers to a control byte,
 * before all the bytes of the data. A decoder then reads a control byte and
 * knows where the 4 integers start, with no test on the data: on a processor
 * with the shuffle of bytes of SSSE3, the 16 bytes from the place of the first
 * integer are moved to their 4 places of 4 bytes, the missing high bytes made
 * 0, by one shuffle whose mask is the entry of the control byte in a table of
 * 256 masks, and the place moves by the sum of the 4 lengths, from a table
 * too. The encoder, with SSE4.1, finds the lengths of 8 integers at once by
 * saturating arithmetic on their bytes (a byte not 0 made 1, each half of 16
 * bits made 0, 1 or 255, the pair of halves capped and lifted so that its two
 * high bits are the length less 1), the 2 control bytes being one mask of 16
 * bytes, and moves their bytes together by the shuffle of the inverse table;
 * the decoder takes 8 control bytes in a word and decodes 32 integers with no
 * test while 12 more follow; elsewhere the integers are moved one at a time,
 * still with no test. With
 * \f$ \ell_i = \max( 1 , \lceil \mathrm{bitwidth}( x_i ) / 8 \rceil ) \f$
 * the bytes of \f$ x_i \f$, the code takes
 * \f[
 *   \lceil n / 4 \rceil + \sum_{i=0}^{n-1} \ell_i
 * \f]
 * bytes, i.e., the variable-byte code with 2 control bits per integer
 * instead of 1 per byte.
 *
 * The format: the \f$ \lceil n / 4 \rceil \f$ control bytes, the lengths
 * less 1 of the integers \f$ 4 i \f$, \f$ 4 i + 1 \f$, \f$ 4 i + 2 \f$ and
 * \f$ 4 i + 3 \f$ in the bits 0-1, 2-3, 4-5 and 6-7 of the byte \f$ i \f$,
 * then the bytes of the integers, little-endian; \f$ n \f$ is not written,
 * and the decoder takes it.
 *
 * The contracts [see `Contract.h`]: the decoder gets the code of
 * \f$ n \f$ integers. The functions are defined in `StreamVByte.tpp`.
 *
 * \see D. Lemire, N. Kurz, C. Rupp, Stream VByte: faster byte-oriented
 *      integer compression, Information Processing Letters 130, 1-6, 2018,
 *      doi:10.1016/j.ipl.2017.09.011
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

/// the Stream VByte code of \f$ x \f$: the control bytes, then the data
///
/// In pseudocode (Stream VByte of Lemire, Kurz and Rupp, beyond the book),
/// one integer at a time, which the SIMD code does 8 at a time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{streamVByteEncode}( x_0 \cdots x_{n-1} )
///     \cr
/// 2 & \quad K \gets \lceil n / 4 \rceil \text{ control bytes of } 0;
///     \ D \gets \text{an empty string of bytes} \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \ell \gets \max( 1 , \lceil \mathrm{bitwidth}( x_i ) / 8
///     \rceil ) \cr
/// 5 & \qquad K[ \lfloor i / 4 \rfloor ] \gets K[ \lfloor i / 4 \rfloor ] +
///     ( \ell - 1 ) \cdot 4^{i \bmod 4} \cr
/// 6 & \qquad \text{append the } \ell \text{ low bytes of } x_i \text{ to }
///     D \cr
/// 7 & \quad \textbf{return}\ K \cdot D
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::uint8_t > streamVByteEncode(
                                         std::span< const std::uint32_t > x );

/// the \f$ n \f$ integers of the Stream VByte code
///
/// In pseudocode (Stream VByte, beyond the book), \f$ \mathit{shuffle}[ c ]
/// \f$ and \f$ \mathit{len}[ c ] \f$ the mask and the total length of the
/// control byte \f$ c \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{streamVByteDecode}( K \cdot D , n ) \cr
/// 2 & \quad p \gets 0 \cr
/// 3 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ \lceil n / 4 \rceil - 1
///     \ \textbf{do} \cr
/// 4 & \qquad c \gets K[ q ] \cr
/// 5 & \qquad ( x_{4 q} , \ldots , x_{4 q + 3} ) \gets \text{the bytes }
///     D[ p .. p + 15 ] \text{ moved by } \mathit{shuffle}[ c ] \cr
/// 6 & \qquad p \gets p + \mathit{len}[ c ] \cr
/// 7 & \quad \textbf{return}\ x_0 \cdots x_{n-1}
/// \end{array}}
/// \f]
/// Near the end of the data, where 16 bytes are not left, the integers are
/// gathered one at a time from their lengths.
/// \pre `code` is the code of \f$ n \f$ integers
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
