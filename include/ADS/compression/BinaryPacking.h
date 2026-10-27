/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryPacking.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary packing of Lemire and Boytsov, SIMD-BP128, for integers of 32
 * bits: the sequence is cut into blocks of 128 integers, and a block is
 * written with \f$ b \f$ bits per integer, \f$ b \f$ the bits of its
 * largest one, so that it takes \f$ 16 b \f$ bytes; the widths of 16 blocks
 * are 16 bytes before them. The integers of a block are laid out vertically
 * over the 4 lanes of a register of 128 bits, the integer \f$ i \f$ in the
 * lane \f$ i \bmod 4 \f$, at the place \f$ \lfloor i / 4 \rfloor b \f$ of
 * the \f$ 32 b \f$ bits of its lane, so that 4 integers are packed or
 * unpacked at once by shifts and masks of the 4 lanes together:
 * \f[
 *   x_{4 q + l} = ( w_{\lfloor q b / 32 \rfloor} \gg ( q b \bmod 32 ) \;
 *   | \; w_{\lfloor q b / 32 \rfloor + 1} \ll ( 32 - q b \bmod 32 ) )
 *   \wedge ( 2^b - 1 )
 * \f]
 * on the lane \f$ l \f$ of the words \f$ w \f$ of the block, the second
 * word only where the field crosses a word. A block of each width has its
 * own unrolled code (a template, with every shift a constant), chosen by a
 * table of 33 functions. Against the variable-byte codes [see
 * `IntegerCodes.h` and `StreamVByte.h`], whose unit is the byte, the code
 * takes the bits of the largest integer of each block, to the bit, at the
 * price of the integers that are much larger than the others of their
 * block (the patched codes, PFor, write those apart).
 *
 * The sorted sequences are coded by their differences, at a distance of 4
 * (the differential code D4 of Lemire, Boytsov and Kurz):
 * \f$ d_i = x_i - x_{i-4} \f$, \f$ x_{-4} = \cdots = x_{-1} = 0 \f$, so
 * that the decoder adds the previous register of 4 integers to each
 * register it unpacks, one addition per 4 integers instead of the chain of
 * a prefix sum, for a fraction of a bit more per integer.
 *
 * The patched code, PFor of Zukowski, Heman, Nes and Boncz in the form
 * of the FastPFor of Lemire and Boytsov, packs a block with a width
 * \f$ b \f$ below that of its largest integer \f$ \beta \f$ and writes
 * apart the \f$ c( b ) \f$ integers of \f$ 2^b \f$ or more, the exceptions:
 * their places in a byte each and their high bits, \f$ \beta - b \f$ each,
 * packed after the block. The width is the one of least cost,
 * \f[
 *   b^* = \arg\min_{0 \le b \le \beta} \big( 128 \, b + c( b ) \,
 *   ( 8 + \beta - b ) + 8 \, [ b < \beta ] \big) ,
 * \f]
 * the last term the byte of \f$ \beta \f$, found from the counts of the
 * widths of the integers of the block, and the decoder unpacks the block as
 * above and then sets the high bits of the exceptions (each one by a load
 * of 8 bytes); a sorted sequence is summed at a distance of 4 in a pass of
 * its own once its exceptions are set, as a sum read right after the
 * scattered writes of the exceptions would wait for them to reach the
 * cache. A block of the patched code is its width \f$ b \f$ and \f$ c \f$
 * in a byte each, \f$ \beta \f$ in a byte if \f$ c > 0 \f$, the block of
 * width \f$ b \f$, the places and the high bits, to a byte; 8 bytes of 0
 * end the code.
 *
 * The format: for each group of up to 16 blocks, the 16 widths in a byte
 * each and then the blocks, the last one filled with 0s (with differences
 * of 0); the number of the integers is not written, and the decoder takes
 * it. On a processor with no SSE2 (or with `ADS_BP_SCALAR`, for the tests)
 * the same layout is packed and unpacked a lane at a time.
 *
 * The contracts [see `Contract.h`]: the sorted sequence of the differential
 * code is non-decreasing (an audit); the decoder gets the code of
 * \f$ n \f$ integers, whose widths are at most 32 and whose blocks fit in
 * the input. The functions are defined in `BinaryPacking.tpp`.
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

/// the binary packing of \f$ x \f$ in blocks of 128 integers, of their
/// differences at a distance of 4 if `sorted`
///
/// In pseudocode (SIMD-BP128 of Lemire and Boytsov, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binaryPack}( x , \mathit{sorted} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{sorted}\ \textbf{then foreach}\ i
///     \ \textbf{do}\ v_i \gets x_i - x_{i-4}\ \textbf{else}\ v \gets x \cr
/// 3 & \quad \text{fill the last block of } v \text{ with } 0 \text{s} \cr
/// 4 & \quad \textbf{foreach}\ \text{block } k \text{ of 128 integers of }
///     v\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ k \bmod 16 = 0\ \textbf{then}\ \text{write the}
///     \text{ widths of the blocks } k , \ldots , k + 15 \cr
/// 6 & \qquad b_k \gets \mathrm{bitwidth}( \text{the or of the block} );
///     \ \text{pack the block in } 16 b_k \text{ bytes, in 4 lanes}
/// \end{array}}
/// \f]
/// \pre if `sorted`, \f$ x \f$ is non-decreasing (audit)
/// \post `binaryUnpack( result , x.size() , sorted )` is \f$ x \f$ (audit)
[[nodiscard]] inline std::vector< std::uint8_t > binaryPack(
                         std::span< const std::uint32_t > x ,
                         bool sorted = false );

/// the \f$ n \f$ integers of the code `in`, packed with the same `sorted`
///
/// In pseudocode (SIMD-BP128, beyond the book), \f$ p \f$ the last 4
/// integers decoded:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{binaryUnpack}( y , n , \mathit{sorted} ) \cr
/// 2 & \quad p \gets ( 0 , 0 , 0 , 0 ) \cr
/// 3 & \quad \textbf{foreach}\ \text{block } k \text{ of the }
///     \lceil n / 128 \rceil\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ \text{register } r \text{ of 4 integers}
///     \text{ unpacked from its } 16 b_k \text{ bytes}\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ \mathit{sorted}\ \textbf{then}\ r \gets r +
///     p;\ p \gets r \cr
/// 6 & \qquad\quad \text{append } r \cr
/// 7 & \quad \textbf{return}\ \text{the first } n \text{ integers}
/// \end{array}}
/// \f]
/// \pre `in` is the code of \f$ n \f$ integers
[[nodiscard]] inline std::vector< std::uint32_t > binaryUnpack(
                         std::span< const std::uint8_t > in , std::size_t n ,
                         bool sorted = false );

/// the patched binary packing (PFor) of \f$ x \f$ in blocks of 128
/// integers, of their differences at a distance of 4 if `sorted`
///
/// In pseudocode (the FastPFor of Lemire and Boytsov, beyond the book),
/// \f$ c( t ) \f$ the integers of the block of \f$ 2^t \f$ or more:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{patchedPack}( x , \mathit{sorted} ) \cr
/// 2 & \quad \textbf{foreach}\ \text{block } v \text{ of 128 integers}
///     \text{ (differences if sorted, filled with 0s)}\ \textbf{do} \cr
/// 3 & \qquad \beta \gets \max_i \mathrm{bitwidth}( v_i ) \cr
/// 4 & \qquad b \gets \text{the } t \le \beta \text{ of least } 128 t + c(
///     t ) ( 8 + \beta - t ) + 8 [ t < \beta ] \cr
/// 5 & \qquad \text{write } b \text{ and } c( b ) \text{, and } \beta
///     \text{ if } c( b ) > 0 \cr
/// 6 & \qquad \text{pack the low } b \text{ bits of each } v_i \cr
/// 7 & \qquad \text{write the places of the } v_i \ge 2^b \text{, then their}
///     \text{ high } \beta - b \text{ bits packed} \cr
/// 8 & \quad \text{write 8 bytes of } 0
/// \end{array}}
/// \f]
/// \pre if `sorted`, \f$ x \f$ is non-decreasing (audit)
/// \post `patchedUnpack( result , x.size() , sorted )` is \f$ x \f$ (audit)
[[nodiscard]] inline std::vector< std::uint8_t > patchedPack(
                         std::span< const std::uint32_t > x ,
                         bool sorted = false );

/// the \f$ n \f$ integers of the patched code `in`, packed with the same
/// `sorted`
///
/// In pseudocode (the FastPFor, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{patchedUnpack}( y , n , \mathit{sorted} )
///     \cr
/// 2 & \quad \textbf{foreach}\ \text{block } k\ \textbf{do} \cr
/// 3 & \qquad \text{read } b \text{, } c \text{ and } \beta;\ \text{unpack}
///     \text{ the 128 integers of } b \text{ bits} \cr
/// 4 & \qquad \textbf{for}\ e \gets 0\ \textbf{to}\ c - 1\ \textbf{do}
///     \ v_{\mathit{pos}[ e ]} \gets v_{\mathit{pos}[ e ]} + 2^b \cdot
///     \text{the high bits of } e \cr
/// 5 & \quad \textbf{if}\ \mathit{sorted}\ \textbf{then foreach}\ i \ge 4
///     \ \textbf{do}\ v_i \gets v_i + v_{i-4} \cr
/// 6 & \quad \textbf{return}\ \text{the first } n \text{ integers}
/// \end{array}}
/// \f]
/// \pre `in` is the patched code of \f$ n \f$ integers
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
