/*--------------------------------------------------------------------------*/
/*------------------------------ File Tans.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tabled variant of the asymmetric numeral systems of Duda (tANS), the
 * entropy coder of order 0 of Zstandard (RFC 8878), beside the range
 * variant [see `Rans.h`]: the transitions of the state are read from
 * tables, so that a symbol costs a lookup, a shift and an addition, and no
 * multiplication.
 *
 * With \f$ L = 2^R \f$ states, \f$ R = 11 \f$, and the frequencies
 * \f$ f_s \f$ of the bytes scaled to sum to \f$ L \f$ (each byte of the
 * string kept at 1 at least [see `scaleFrequencies()` of `Rans.h`]), the
 * \f$ L \f$ states are dealt to the symbols, \f$ f_s \f$ each, by the step
 * \f$ L / 2 + L / 8 + 3 \f$ modulo \f$ L \f$, which visits them all (it is
 * odd) and spreads the states of a symbol over the table. The decoder in
 * the state \f$ x \in [ 0 , L ) \f$ of the symbol \f$ s \f$, its
 * \f$ i \f$-th one, emits \f$ s \f$ and moves to
 * \f[
 *   x' = ( ( f_s + i ) 2^{b} - L ) + r , \qquad
 *   b = R - \lfloor \log_2( f_s + i ) \rfloor ,
 * \f]
 * \f$ r \f$ the next \f$ b \f$ bits of the stream; the encoder in the state
 * \f$ X \in [ L , 2 L ) \f$ emits the low \f$ k \f$ bits of \f$ X \f$,
 * \f$ k \f$ the number that brings \f$ \lfloor X / 2^k \rfloor \f$ into
 * \f$ [ f_s , 2 f_s ) \f$ (one of two values, found by one addition and a
 * shift), and moves to \f$ L \f$ plus the state of the
 * \f$ ( \lfloor X / 2^k \rfloor - f_s ) \f$-th place of \f$ s \f$. A symbol
 * costs about \f$ \log_2( L / f_s ) \f$ bits, and the code comes within a
 * fraction of a percent of \f$ n H_0 \f$ [see `Entropy.h`] for \f$ L \f$ a
 * few times the alphabet (Duda; Dube and Yokoo for the analysis of the
 * spread).
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
 * The format: the size \f$ n \f$ in 8 bytes, the 256 scaled frequencies in
 * 2 bytes each, the bits of the two streams in 8 bytes each, the streams
 * (the 4 final states of the encoder at the end of each, \f$ R \f$ bits
 * each) and 8 bytes of 0; the integers are little-endian.
 *
 * The contracts [see `Contract.h`]: `tansDecompress()` gets the output of
 * `tansCompress()`, whose header (the frequencies sum to \f$ L \f$, the bits
 * of the stream fit in the input) it checks; the audit of `tansCompress()`
 * is that the output decompresses to the input. The functions are defined
 * in `Tans.tpp`.
 *
 * \see J. Duda, Asymmetric numeral systems: entropy coding combining speed
 *      of Huffman coding with compression rate of arithmetic coding,
 *      arXiv:1311.2540, 2013
 * \see J. Duda, K. Tahboub, N. J. Gadgil, E. J. Delp, The use of
 *      asymmetric numeral systems as an accurate replacement for Huffman
 *      coding, Proc. 31st Picture Coding Symposium (PCS 2015), 65-69, 2015,
 *      doi:10.1109/PCS.2015.7170048
 * \see D. Dube, H. Yokoo, Fast construction of almost optimal symbol
 *      distributions for asymmetric numeral systems, Proc. IEEE ISIT 2019,
 *      1682-1686, 2019, doi:10.1109/ISIT.2019.8849430
 * \see Y. Collet, M. Kucherawy, Zstandard Compression and the
 *      'application/zstd' Media Type, RFC 8878, 2021, doi:10.17487/RFC8878
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

/// the tANS code of order 0 of the bytes of `in`, with the frequencies and
/// the size
///
/// In pseudocode (the tANS of Duda, beyond the book), for one half of the
/// string, \f$ \mathit{next}[ c_s + j ] \f$ being \f$ L \f$ plus the
/// \f$ j \f$-th state dealt to \f$ s \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tansCompress}( s_0 \cdots s_{n-1} ) \cr
/// 2 & \quad f \gets \mathrm{scaleFrequencies}( \text{the counts of the }
///     \text{bytes} , R );\ \text{deal the states by the step} \cr
/// 3 & \quad X_0 , \ldots , X_3 \gets L \cr
/// 4 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \quad \triangleright\ \text{from the last symbol} \cr
/// 5 & \qquad X \gets X_{i \bmod 4};\ s \gets s_i \cr
/// 6 & \qquad k \gets \text{the number with } \lfloor X / 2^k \rfloor \in [
///     f_s , 2 f_s ) \cr
/// 7 & \qquad \text{write the low } k \text{ bits of } X \cr
/// 8 & \qquad X_{i \bmod 4} \gets \mathit{next}[ c_s + \lfloor X / 2^k
///     \rfloor - f_s ] \cr
/// 9 & \quad \text{write } X_0 - L , \ldots , X_3 - L \text{ on } R
///     \text{ bits each};\ \textbf{return}\ \text{the stream}
/// \end{array}}
/// \f]
/// \post the output decompresses to the input (audit)
[[nodiscard]] inline std::vector< std::uint8_t > tansCompress(
                                          std::span< const std::uint8_t > in );

/// the bytes that `tansCompress()` coded in `in`
///
/// In pseudocode (the tANS of Duda, beyond the book), for one half of the
/// string, the stream read backward:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{tansDecompress}( y ) \cr
/// 2 & \quad \text{read } n \text{ and } f;\ \text{deal the states by the }
///     \text{step} \cr
/// 3 & \quad \textbf{foreach}\ x \in [ 0 , L )\ \textbf{do} \cr
/// 4 & \qquad s \gets \text{the symbol of } x;\ i \gets \text{its rank}
///     \text{ among the states of } s;\ b \gets R - \lfloor \log_2( f_s + i
///     ) \rfloor \cr
/// 5 & \qquad D[ x ] \gets ( s , b , ( f_s + i ) 2^b - L ) \cr
/// 6 & \quad x_3 , \ldots , x_0 \gets \text{the last } 4 R \text{ bits of }
///     \text{the stream} \cr
/// 7 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 8 & \qquad ( s , b , \beta ) \gets D[ x_{i \bmod 4} ];\ s_i \gets s \cr
/// 9 & \qquad x_{i \bmod 4} \gets \beta + \text{the next } b \text{ bits,}
///     \text{ going backward} \cr
/// 10 & \quad \textbf{return}\ s_0 \cdots s_{n-1}
/// \end{array}}
/// \f]
/// \pre `in` is an output of `tansCompress()`
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
