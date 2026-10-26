/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryPacking.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary packing of Lemire and Boytsov, SIMD-BP128, for integers of 32
 * bits: the sequence is cut into blocks of 128 integers, and a block is
 * written with b bits per integer, b the bits of its largest one, so that
 * it takes \f$ 16 b \f$ bytes; the widths of 16 blocks are 16 bytes before
 * them. The integers of a block are laid out vertically over the 4 lanes
 * of a register of 128 bits, the integer i in the lane i mod 4, at the
 * place \f$ \lfloor i / 4 \rfloor b \f$ of the 32 b bits of its lane, so
 * that 4 integers are packed or unpacked at once by shifts and masks of
 * the 4 lanes together:
 * \f[ x_{4 q + l} = ( w_{\lfloor q b / 32 \rfloor} \gg ( q b \bmod 32 ) \;
 *     | \; w_{\lfloor q b / 32 \rfloor + 1} \ll ( 32 - q b \bmod 32 ) )
 *     \wedge ( 2^b - 1 ) \f]
 * on the lane l of the words w of the block, the second word only where
 * the field crosses a word. A block of each width has its own unrolled
 * code (a template, with every shift a constant), chosen by a table of 33
 * functions. Against the variable-byte codes [see IntegerCodes.h,
 * StreamVByte.h], whose unit is the byte, the code takes the bits of the
 * largest integer of each block, to the bit, at the price of the integers
 * that are much larger than the others of their block (the patched codes,
 * PFor, write those apart).
 *
 * The sorted sequences are coded by their differences, at a distance of 4
 * (the differential code D4 of Lemire, Boytsov and Kurz):
 * \f$ d_i = x_i - x_{i-4} \f$, \f$ x_{-4} = \cdots = x_{-1} = 0 \f$, so
 * that the decoder adds the previous register of 4 integers to each
 * register it unpacks, one addition per 4 integers instead of the chain of
 * a prefix sum, for a fraction of a bit more per integer.
 *
 * The patched code, PFor of Zukowski, Heman, Nes and Boncz in the form
 * of the FastPFor of Lemire and Boytsov, packs a block with a width b
 * below that of its largest integer \f$ \beta \f$ and writes apart the
 * c(b) integers of \f$ 2^b \f$ or more, the exceptions: their places in
 * a byte each and their high bits, \f$ \beta - b \f$ each, packed after
 * the block. The width is the one of least cost,
 * \f[ b^* = \arg\min_{0 \le b \le \beta} \big( 128 \, b + c( b ) \,
 *     ( 8 + \beta - b ) \big) , \f]
 * found from the counts of the widths of the integers of the block, and
 * the decoder unpacks the block as above and then sets the high bits of
 * the exceptions (each one by a load of 8 bytes); a sorted sequence is
 * summed at a distance of 4 in a pass of its own once its exceptions are
 * set, as a sum read right after the scattered writes of the exceptions
 * would wait for them to reach the cache. A block of the patched
 * code is its width b and c in a byte each, \f$ \beta \f$ in a byte if
 * c > 0, the block of width b, the places and the high bits, to a byte;
 * 8 bytes of 0 end the code.
 *
 * The format: for each group of up to 16 blocks, the 16 widths in a byte
 * each and then the blocks, the last one filled with 0s (with differences
 * of 0); the number of the integers is not written, and the decoder takes
 * it. On a processor with no SSE2 (or with ADS_BP_SCALAR, for the tests)
 * the same layout is packed and unpacked a lane at a time.
 *
 * The contracts [see Contract.h]: the sorted sequence of the differential
 * code is non-decreasing (an audit); the decoder gets the code of n
 * integers, whose widths are at most 32 and whose blocks fit in the input.
 * The functions are defined in BinaryPacking.tpp.
 *
 * \see D. Lemire, L. Boytsov, Decoding billions of integers per second
 *      through vectorization, Software: Practice and Experience 45(1),
 *      1-29, 2015, doi:10.1002/spe.2203
 * \see D. Lemire, L. Boytsov, N. Kurz, SIMD compression and the
 *      intersection of sorted integers, Software: Practice and Experience
 *      46(6), 723-749, 2016, doi:10.1002/spe.2326
 * \see M. Zukowski, S. Heman, N. Nes, P. Boncz, Super-scalar RAM-CPU
 *      cache compression, Proc. 22nd ICDE, 59, 2006,
 *      doi:10.1109/ICDE.2006.150
 * \see G. E. Pibiri, R. Venturini, Techniques for inverted index
 *      compression, ACM Computing Surveys 53(6), 125, 2020,
 *      doi:10.1145/3415148
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

#ifndef __ADS_BinaryPacking
 #define __ADS_BinaryPacking
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
#include <utility>
#include <vector>

#if ( defined( __SSE2__ ) || defined( _M_X64 ) ) && \
    ! defined( ADS_BP_SCALAR )
 #include <emmintrin.h>
 #define ADS_BP_SSE 1
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the binary packing of x in blocks of 128 integers, of their
/// differences at a distance of 4 if sorted
/// \pre if sorted, x is non-decreasing (an audit)
/// \post binaryUnpack( result , x.size() , sorted ) == x (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > binaryPack(
                         std::span< const std::uint32_t > x ,
                         bool sorted = false );

/// the n integers of the code in, packed with the same sorted
/// \pre in is the code of n integers
[[nodiscard]] inline std::vector< std::uint32_t > binaryUnpack(
                         std::span< const std::uint8_t > in , std::size_t n ,
                         bool sorted = false );

/// the patched binary packing (PFor) of x in blocks of 128 integers, of
/// their differences at a distance of 4 if sorted
/// \pre if sorted, x is non-decreasing (an audit)
/// \post patchedUnpack( result , x.size() , sorted ) == x (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > patchedPack(
                         std::span< const std::uint32_t > x ,
                         bool sorted = false );

/// the n integers of the patched code in, packed with the same sorted
/// \pre in is the patched code of n integers
[[nodiscard]] inline std::vector< std::uint32_t > patchedUnpack(
                         std::span< const std::uint8_t > in , std::size_t n ,
                         bool sorted = false );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/BinaryPacking.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinaryPacking.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryPacking.h ------------------------*/
/*--------------------------------------------------------------------------*/
