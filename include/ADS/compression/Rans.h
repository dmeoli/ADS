/*--------------------------------------------------------------------------*/
/*------------------------------ File Rans.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range variant of the asymmetric numeral systems of Duda (rANS), the
 * entropy coder of order 0 of the fast compressors, beyond the arithmetic code
 * of chapter 12, Statistical Coding, of Ferragina's book [see
 * `ArithmeticCoding.h`]: it reaches the size of the arithmetic code, within a
 * fraction of a percent of \f$ n H_0 \f$ [see `Entropy.h`], at the speed of a
 * table of Huffman.
 *
 * The state is an integer \f$ x \f$, and a symbol \f$ s \f$ of frequency
 * \f$ f_s \f$ and cumulative frequency \f$ c_s \f$ over a total
 * \f$ M = 2^{12} \f$ moves it to
 * \f$ C( s , x ) = M \lfloor x / f_s \rfloor + c_s + x \bmod f_s \f$, which
 * grows \f$ x \f$ by a factor \f$ M / f_s \f$, i.e., by
 * \f$ \log_2( M / f_s ) \f$ bits, the information of \f$ s \f$; the decoder
 * inverts it, \f$ s \f$ being the symbol whose range
 * \f$ [ c_s , c_s + f_s ) \f$ holds \f$ x \bmod M \f$ (a table of
 * \f$ M \f$ entries) and
 * \f$ x = f_s \lfloor x / M \rfloor + x \bmod M - c_s \f$. The state is
 * kept in \f$ [ 2^{16} , 2^{32} ) \f$ by moving 16 bits at a time to the
 * stream (renormalization), at most once per symbol; the decoder reads them
 * back in the opposite order, so that the encoder works from the last
 * symbol to the first. Unlike the arithmetic coder, which narrows an
 * interval and emits its common bits, the state needs one multiplication
 * per symbol and no division in the decoder, and its renormalization has
 * no carry.
 *
 * Four states take the symbols in turn, so that four chains of dependences
 * overlap in the pipeline of the processor (the interleaving of Giesen);
 * they share one stream. The frequencies are scaled to \f$ M \f$ with each
 * byte of the string kept at 1 at least, which costs a fraction of a
 * percent on a string of bytes; the tabled variant [see `Tans.h`] makes the
 * state transitions a lookup and needs no multiplication, at the price of a
 * table per alphabet built in the encoder too.
 *
 * The format: the size \f$ n \f$ in 8 bytes, the 256 frequencies scaled to
 * \f$ M \f$ in 2 bytes each, the four final states in 4 bytes each, the
 * stream of words of 16 bits, and 8 bytes of 0; the integers are
 * little-endian.
 *
 * The contracts [see `Contract.h`]: `ransDecompress()` gets the output of
 * `ransCompress()`, whose header (the frequencies sum to \f$ M \f$, the
 * states and the stream are in range) it checks; the audit of
 * `ransCompress()` is that the output decompresses to the input. The
 * functions are defined in `Rans.tpp`.
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

/// the rANS code of order 0 of the bytes of `in`, with the frequencies and
/// the size
///
/// In pseudocode (the rANS of Duda with the interleaving of Giesen, beyond
/// the book), \f$ x_0 , \ldots , x_3 \f$ the states and \f$ s_i \f$ the
/// \f$ i \f$-th byte, taken by the state \f$ x_{i \bmod 4} \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ransCompress}( s_0 \cdots s_{n-1} ) \cr
/// 2 & \quad f \gets \mathrm{scaleFrequencies}( \text{the counts of the }
///     \text{bytes} , 12 );\ c \gets \text{their prefix sums} \cr
/// 3 & \quad x_0 , \ldots , x_3 \gets 2^{16};\ W \gets \text{an empty stack}
///     \text{ of words} \cr
/// 4 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \quad \triangleright\ \text{from the last symbol} \cr
/// 5 & \qquad x \gets x_{i \bmod 4};\ s \gets s_i \cr
/// 6 & \qquad \textbf{if}\ x \ge 2^{20} f_s\ \textbf{then}
///     \ \mathrm{push}( W , x \bmod 2^{16} );\ x \gets \lfloor x / 2^{16}
///     \rfloor \cr
/// 7 & \qquad x_{i \bmod 4} \gets M \lfloor x / f_s \rfloor + c_s + x \bmod
///     f_s \cr
/// 8 & \quad \textbf{return}\ ( n , f , x_0 , \ldots , x_3 , \text{the }
///     \text{words of } W \text{ from the top} )
/// \end{array}}
/// \f]
/// The division by \f$ f_s \f$ is a multiplication by a reciprocal (Granlund
/// and Montgomery), exact on the states below \f$ 2^{32} \f$.
/// \post the output decompresses to the input (audit)
[[nodiscard]] inline std::vector< std::uint8_t > ransCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that `ransCompress()` coded in `in`
///
/// In pseudocode (the rANS of Duda, beyond the book), \f$ Z[ j ] \f$ the
/// symbol of the slot \f$ j \in [ 0 , M ) \f$, i.e., the \f$ s \f$ with
/// \f$ c_s \le j < c_s + f_s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ransDecompress}( y ) \cr
/// 2 & \quad \text{read } n \text{, } f \text{ and } x_0 , \ldots , x_3;
///     \ c \gets \text{the prefix sums of } f \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad x \gets x_{i \bmod 4};\ j \gets x \bmod M;\ s \gets Z[ j ];
///     \ s_i \gets s \cr
/// 5 & \qquad x \gets f_s \lfloor x / M \rfloor + j - c_s \cr
/// 6 & \qquad \textbf{if}\ x < 2^{16}\ \textbf{then}\ x \gets 2^{16} x +
///     \text{the next word of the stream} \cr
/// 7 & \qquad x_{i \bmod 4} \gets x \cr
/// 8 & \quad \textbf{return}\ s_0 \cdots s_{n-1}
/// \end{array}}
/// \f]
/// \pre `in` is an output of `ransCompress()`
[[nodiscard]] inline std::vector< std::uint8_t > ransDecompress(
                                          std::span< const std::uint8_t > in );

/// the frequencies \f$ f \f$ scaled to sum to \f$ 2^b \f$, each positive one
/// kept positive
///
/// It rounds each frequency, scaled by \f$ 2^b / N \f$, to the nearest
/// integer, at least 1, and then gives the difference to \f$ 2^b \f$ to the
/// largest one, or takes it from the largest ones, at most half of each at a
/// time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{scaleFrequencies}( f , b ) \cr
/// 2 & \quad N \gets \sum_s f[ s ];\ M \gets 2^b \cr
/// 3 & \quad \textbf{foreach}\ s\ \textbf{with}\ f[ s ] > 0\ \textbf{do}
///     \ g[ s ] \gets \max( 1 , \lfloor ( f[ s ] M + N / 2 ) / N \rfloor ) \cr
/// 4 & \quad \textbf{while}\ \sum_s g[ s ] \ne M\ \textbf{do} \cr
/// 5 & \qquad s \gets \text{the symbol of the largest } g[ s ];
///     \ \delta \gets \sum_s g[ s ] - M \cr
/// 6 & \qquad \textbf{if}\ \delta < 0\ \textbf{then}\ g[ s ] \gets g[ s ] -
///     \delta\ \textbf{else}\ g[ s ] \gets g[ s ] - \max( 1 , \min( \delta ,
///     \lfloor g[ s ] / 2 \rfloor ) ) \cr
/// 7 & \quad \textbf{return}\ g
/// \end{array}}
/// \f]
/// \pre some frequency is positive, and at most \f$ 2^b \f$ are
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
