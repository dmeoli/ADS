/*--------------------------------------------------------------------------*/
/*------------------------------- File ALP.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Adaptive Lossless floating-Point compression (ALP) of Afroozeh,
 * Kuffó and Boncz for columns of `double` or `float`, bit for bit lossless,
 * by vectors of 1024 values and row groups of 100 vectors.
 *
 * Most columns of floating-point numbers hold decimals of a few digits
 * (prices, measures), which the binary format cannot write exactly but
 * which a decimal integer \f$ d \f$ and an exponent give back. ALP encodes
 * a value \f$ x \f$ by the exponent \f$ e \f$ and the factor \f$ f \le e
 * \f$ of its vector as
 * \f[
 *   d = \mathrm{round}( x \cdot 10^e \cdot 10^{-f} ) , \qquad
 *   \tilde x = d \cdot 10^f \cdot 10^{-e} ,
 * \f]
 * both in the arithmetic of the type, with \f$ 10^e \f$ exact and
 * \f$ 10^{-f} \f$ the nearest value of the type, \f$ e \le 18 \f$ for
 * `double` and \f$ e \le 10 \f$ for `float`; the rounding is the addition
 * and the subtraction of \f$ 2^{52} + 2^{51} \f$ (\f$ 2^{23} + 2^{22} \f$
 * for `float`), which rounds to the nearest integer every value below
 * \f$ 2^{51} \f$ (\f$ 2^{22} \f$) in magnitude with no branch. A value
 * whose \f$ \tilde x \f$ differs from \f$ x \f$ in any bit (a value with
 * more digits, \f$ -0 \f$, an infinity, a NaN, a denormal, a value too
 * large) is an exception: its place (2 bytes) and its bits are written
 * apart, and its \f$ d \f$ becomes the one of the first value of the vector
 * that is not an exception, so that it does not widen the frame. The
 * integers \f$ d \f$ of a vector are then written by the frame of
 * reference and the bit-packing of FastLanes [see `FastLanes.h`].
 *
 * The pair \f$ ( e , f ) \f$ is searched in two levels of samples: for each
 * row group, 8 of its vectors, 32 values of each, try all the pairs
 * (190 for `double`, 66 for `float`), and the 5 pairs that are best for most
 * of these vectors are kept; then each vector tries these on 32 of its
 * values, in order, and stops when two in a row do not improve on the best.
 * The cost of a pair on \f$ s \f$ values is \f$ s \, b + c \, ( W + 16 )
 * \f$, \f$ b \f$ the bits of the frame of its \f$ d \f$ and \f$ c \f$ its
 * exceptions, \f$ W \f$ the bits of the type.
 *
 * The values with many significant digits (the "real" doubles of
 * scientific data) leave too many exceptions; for them ALP falls back to
 * ALP-RD, which cuts the bits of each value into the \f$ \lambda \le 16 \f$
 * high ones, whose few distinct values (the sign, the exponent and the top
 * of the mantissa) take an index of 3 bits at most into a dictionary of up
 * to 8 entries, and the \f$ W - \lambda \f$ low ones, packed as they are;
 * a high part outside the dictionary is an exception, its place and its
 * \f$ \lambda \f$ bits in 2 bytes each. The cut \f$ \lambda \f$ and the
 * dictionary minimize the cost on the sample of the row group, which takes
 * ALP-RD instead of ALP when its cost per value is lower.
 *
 * The format: for each row group, a byte of its scheme (0 ALP, 1 ALP-RD)
 * and, for ALP-RD, \f$ \lambda \f$ and the entries \f$ q \f$ in a byte
 * each and the \f$ q \f$ entries in 2 bytes each; then its vectors, the
 * last one filled. A vector of ALP is \f$ e \f$, \f$ f \f$ and \f$ b \f$
 * in a byte each, the exceptions \f$ c \f$ in 2 bytes, the base of the
 * frame in \f$ W / 8 \f$ bytes, the \f$ 128 b \f$ bytes of the packing,
 * the \f$ c \f$ places and the \f$ c \f$ values; a vector of ALP-RD is
 * \f$ c \f$ in 2 bytes, the indices packed on 16 bits with \f$
 * \mathrm{bitwidth}( q - 1 ) \f$ bits, the low parts packed with \f$ W -
 * \lambda \f$ bits, the \f$ c \f$ places and the \f$ c \f$ high parts; all
 * little-endian. The number of the values is not written, and the decoder
 * takes it.
 *
 * The contracts [see `Contract.h`]: the decoder gets the code of
 * \f$ n \f$ values of the same type, whose schemes, exponents, widths,
 * dictionaries and places fit. The functions are defined in `ALP.tpp`.
 *
 * \see A. Afroozeh, L. X. Kuffó, P. Boncz, ALP: Adaptive Lossless
 *      floating-Point compression, Proceedings of the ACM on Management of
 *      Data 1(4), 230, 2023 (SIGMOD 2024), doi:10.1145/3626717
 * \see A. Afroozeh, P. Boncz, The FastLanes compression layout: decoding
 *      >100 billion integers per second with scalar code, Proceedings of
 *      the VLDB Endowment 16(9), 2132-2144, 2023,
 *      doi:10.14778/3598581.3598587
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

#ifndef __ADS_ALP
 #define __ADS_ALP
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/FastLanes.h"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the floating-point types of ALP, `double` and `float`
template< class F >
concept ALPFloat = std::same_as< F , double > || std::same_as< F , float >;

/// the ALP code of the values \f$ x \f$, bit for bit lossless
///
/// In pseudocode (ALP of Afroozeh, Kuffó and Boncz, beyond the book), with
/// \f$ \mathrm{cost}_S( e , f ) = | S | b + c ( W + 16 ) \f$ on the values
/// \f$ S \f$, \f$ b \f$ the bits of the frame of their \f$ d \f$ and
/// \f$ c \f$ their exceptions:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{alpCompress}( x ) \cr
/// 2 & \quad \textbf{foreach}\ \text{row group } G \text{ of 100 vectors}
///     \ \textbf{do} \cr
/// 3 & \qquad S_1 , \ldots , S_8 \gets 32 \text{ values of each of 8
///     vectors of } G \cr
/// 4 & \qquad P \gets \text{the 5 pairs } ( e , f ) \text{ of least }
///     \mathrm{cost}_{S_j} \text{ for most } j \cr
/// 5 & \qquad \textbf{if}\ \text{ALP-RD costs less on } \bigcup_j S_j
///     \ \textbf{then}\ \text{write } G \text{ by ALP-RD};\ \textbf{continue}
///     \cr
/// 6 & \qquad \textbf{foreach}\ \text{vector } v \text{ of } G\ \textbf{do}
///     \cr
/// 7 & \qquad\quad ( e , f ) \gets \text{the pair of } P \text{ of least }
///     \mathrm{cost} \text{ on 32 values of } v \text{, stopping after two}
///     \text{ that do not improve} \cr
/// 8 & \qquad\quad d_i \gets \mathrm{round}( v_i \cdot 10^e \cdot 10^{-f}
///     );\ E \gets \{ i : d_i \cdot 10^f \cdot 10^{-e} \ne v_i \} \cr
/// 9 & \qquad\quad d_i \gets d_{\min \bar E} \text{ for } i \in E;\ m \gets
///     \min d;\ b \gets \mathrm{bitwidth}( \max d - m ) \cr
/// 10 & \qquad\quad \text{write } e , f , b , | E | , m ,
///     \mathrm{fastLanesPack}( d , b , m ) \text{, the places of } E
///     \text{ and their values}
/// \end{array}}
/// \f]
/// \post `alpDecompress< F >( result , x.size() )` is \f$ x \f$ bit for
///       bit (audit)
template< std::ranges::contiguous_range R >
 requires ALPFloat< std::ranges::range_value_t< R > >
[[nodiscard]] std::vector< std::uint8_t > alpCompress( const R & x );

/// the \f$ n \f$ values of type `F` of the ALP code `code`
///
/// In pseudocode (ALP, beyond the book), for a vector of ALP and one of
/// ALP-RD with the cut \f$ \lambda \f$ and the dictionary \f$ D \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{alpDecompress}( y , n ) \cr
/// 2 & \quad \textbf{foreach}\ \text{vector of each row group}\ \textbf{do}
///     \cr
/// 3 & \qquad \textbf{if}\ \text{ALP}\ \textbf{then}\ d \gets
///     \mathrm{fastLanesUnpack}( y , b , m );\ v_i \gets d_i \cdot 10^f
///     \cdot 10^{-e} \cr
/// 4 & \qquad \textbf{else}\ v_i \gets \text{the bits } D[ \iota_i ] \cdot
///     2^{W - \lambda} + \ell_i \text{ of the indices } \iota \text{ and the}
///     \text{ low parts } \ell \text{ unpacked} \cr
/// 5 & \qquad \textbf{foreach}\ \text{exception}\ \textbf{do}\ \text{set the}
///     \text{ value, or the high part, at its place} \cr
/// 6 & \quad \textbf{return}\ \text{the first } n \text{ values}
/// \end{array}}
/// \f]
/// \pre `code` is the ALP code of \f$ n \f$ values of type `F`
template< ALPFloat F >
[[nodiscard]] std::vector< F >
alpDecompress( std::span< const std::uint8_t > code , std::size_t n );

/// the code of `alpCompress( x )` written in `out` in place of what it
/// held, in the room that `out` already has when it suffices
template< std::ranges::contiguous_range R >
 requires ALPFloat< std::ranges::range_value_t< R > >
void alpCompress( const R & x , std::vector< std::uint8_t > & out );

/// the values of `alpDecompress< F >( code , out.size() )` written in `out`
/// \pre `code` is the ALP code of `out.size()` values of type `F`
template< ALPFloat F >
void alpDecompress( std::span< const std::uint8_t > code ,
		    std::span< F > out );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/ALP.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ALP.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ALP.h ------------------------------*/
/*--------------------------------------------------------------------------*/
