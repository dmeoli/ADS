/*--------------------------------------------------------------------------*/
/*---------------------------- File FastLanes.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The light-weight codes of FastLanes of Afroozeh and Boncz for vectors of
 * 1024 unsigned integers of \f$ W = 8 \f$, 16, 32 or 64 bits: the
 * bit-packing, the frame of reference (FOR), the differences (DELTA) and
 * the runs (RLE), all on one layout that a loop over the lanes of a
 * virtual register of 1024 bits vectorizes, whatever the width of the
 * registers of the processor.
 *
 * The vector is seen as \f$ L = 1024 / W \f$ lanes of \f$ W \f$ rows: the
 * value at the place \f$ p \f$ of the layout is in the lane \f$ p \bmod L
 * \f$ and in the row \f$ \lfloor p / L \rfloor \f$, and a packing of width
 * \f$ b \f$ writes the row \f$ r \f$ of the lane \f$ l \f$ at the bit
 * \f$ r b \f$ of the \f$ b \f$ words of that lane, the word \f$ k \f$ of the
 * lane \f$ l \f$ at the place \f$ k L + l \f$ of the output. Every lane runs
 * the same shifts and masks, which are constants once \f$ b \f$ is (a
 * template for each width, chosen by a table of \f$ W + 1 \f$ functions),
 * and the loop over the \f$ L \f$ lanes of each row has no dependence
 * between its iterations, so that the compiler turns it into the
 * instructions of the vectors of the processor (of 128, 256 or 512 bits),
 * or leaves it scalar, from the same plain C++; this is the point of the
 * paper, which writes no intrinsics. A vector of \f$ b \f$ bits takes
 * \f$ 128 b \f$ bytes for every \f$ W \f$, as SIMD-BP128 [see
 * `BinaryPacking.h`] on lanes of 1024 bits instead of 128.
 *
 * The differences cannot run along the vector, where each one waits for the
 * one before it; FastLanes reorders the vector so that each lane holds a
 * run of consecutive values, and the prefix sums of the \f$ L \f$ lanes
 * run side by side, row after row. The reorder is the same for every
 * \f$ W \f$ (the unified transposed layout): the place \f$ p \f$ of the
 * layout holds the value
 * \f[
 *   \tau( p ) = 64 \, ( p \bmod 16 ) + 8 \, \rho( \lfloor p / 16 \rfloor
 *   \bmod 8 ) + \lfloor p / 128 \rfloor
 * \f]
 * of the vector, \f$ \rho \f$ the reversal of 3 bits, i.e., the order
 * 0, 4, 2, 6, 1, 5, 3, 7 of the tiles of 8 rows; then the row \f$ r =
 * 8 o + s \f$ of the lane \f$ l \f$ of the chain of differences is the place
 * \f$ 16 \rho( o ) + 128 s + l \f$, which is in the lane \f$ l \f$ of the
 * packing, and each lane holds \f$ W \f$ consecutive values. A vector of
 * differences keeps the first value of each lane, its \f$ L \f$ bases, in
 * \f$ L W / 8 = 128 \f$ bytes, i.e., a bit per value, and the decoder
 * unpacks a row and adds it to the sums of the lanes in the same loop.
 * The runs are the values of the runs and, for each value of the vector,
 * the number of its run, which is nondecreasing and is coded by its
 * differences on 16 bits (of 0 or 1 bit each), so that the decoder of the
 * runs is the decoder of the differences followed by a gather.
 *
 * The format: for each vector of 1024 values, the last one filled with its
 * last value, (FOR) the width \f$ b \f$ in a byte, the base \f$ m \f$, the
 * smallest value, in \f$ W / 8 \f$ bytes and the values less \f$ m \f$ in
 * \f$ 128 b \f$ bytes; (DELTA) \f$ b \f$, the \f$ L \f$ bases and the
 * differences modulo \f$ 2^W \f$ in \f$ 128 b \f$ bytes; (RLE) the runs
 * \f$ k \f$ in 2 bytes, their values in \f$ k W / 8 \f$ bytes and the
 * numbers of the runs as DELTA on 16 bits; all little-endian. The number of
 * the values and the scheme are not written, and the decoder takes them.
 *
 * The contracts [see `Contract.h`]: the kernels get widths of at most
 * \f$ W \f$; the decoder gets the code of \f$ n \f$ values of the same
 * scheme, whose widths, runs and numbers of the runs fit. The functions are
 * defined in `FastLanes.tpp`.
 *
 * \see A. Afroozeh, P. Boncz, The FastLanes compression layout: decoding
 *      >100 billion integers per second with scalar code, Proceedings of
 *      the VLDB Endowment 16(9), 2132-2144, 2023,
 *      doi:10.14778/3598581.3598587
 * \see D. Lemire, L. Boytsov, Decoding billions of integers per second
 *      through vectorization, Software: Practice and Experience 45(1),
 *      1-29, 2015, doi:10.1002/spe.2203
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

#ifndef __ADS_FastLanes
 #define __ADS_FastLanes
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the values of a vector of FastLanes
inline constexpr std::size_t fastLanesVector = 1024;

/// the unsigned integers of 8, 16, 32 and 64 bits that FastLanes codes
template< class T >
concept FastLanesWord =
  std::unsigned_integral< T > && ! std::same_as< T , bool > &&
  ( sizeof( T ) == 1 || sizeof( T ) == 2 || sizeof( T ) == 4 ||
    sizeof( T ) == 8 );

/// the schemes of `fastLanesEncode()`: the frame of reference (with the
/// bit-packing of its offsets), the differences and the runs
enum class FastLanesScheme : std::uint8_t {
 FOR = 0 ,    ///< the values less the smallest of their vector, packed
 DELTA = 1 ,  ///< the differences along the lanes, packed
 RLE = 2     ///< the values of the runs and the numbers of the runs
 };

/// the place of the vector whose value is at the place \f$ p \f$ of the
/// unified transposed layout, \f$ \tau( p ) \f$ of the file
[[nodiscard]] constexpr std::size_t
fastLanesTransposed( std::size_t p ) noexcept;

/// the 1024 values of `in`, less `base` modulo \f$ 2^W \f$, packed into
/// the \f$ 128 b \f$ bytes at `out`, the row \f$ r \f$ of the lane \f$ l
/// \f$ at the bit \f$ r b \f$ of the lane
///
/// In pseudocode (the bit-packing of FastLanes, beyond the book), for the
/// \f$ W \f$-bit values \f$ x \f$, \f$ L = 1024 / W \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fastLanesPack}( x , b , m ) \cr
/// 2 & \quad \textbf{foreach}\ \text{lane } l < L \text{, all at once}
///     \ \textbf{do} \cr
/// 3 & \qquad a \gets 0 \cr
/// 4 & \qquad \textbf{for}\ r \gets 0\ \textbf{to}\ W - 1\ \textbf{do}
///     \ \text{(unrolled, } r b \text{ a constant)} \cr
/// 5 & \qquad\quad v \gets x_{r L + l} - m;\ a \gets a \vee ( v \ll ( r b
///     \bmod W ) ) \cr
/// 6 & \qquad\quad \textbf{if}\ r b \bmod W + b \ge W\ \textbf{then}
///     \ y_{\lfloor r b / W \rfloor L + l} \gets a;\ a \gets v \gg ( W - r b
///     \bmod W ) \cr
/// 7 & \quad \textbf{return}\ y
/// \end{array}}
/// \f]
/// \pre \f$ b \le W \f$ and every value less `base` is below \f$ 2^b \f$
template< FastLanesWord T >
void fastLanesPack( const T * in , std::uint8_t * out , unsigned b ,
		    T base = 0 );

/// the 1024 values of \f$ b \f$ bits packed at `in` by `fastLanesPack()`,
/// plus `base` modulo \f$ 2^W \f$, into `out`
///
/// In pseudocode (the bit-unpacking of FastLanes, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fastLanesUnpack}( y , b , m ) \cr
/// 2 & \quad \textbf{foreach}\ \text{lane } l < L \text{, all at once}
///     \ \textbf{do} \cr
/// 3 & \qquad \textbf{for}\ r \gets 0\ \textbf{to}\ W - 1\ \textbf{do}
///     \ \text{(unrolled)} \cr
/// 4 & \qquad\quad k \gets \lfloor r b / W \rfloor;\ t \gets r b \bmod W \cr
/// 5 & \qquad\quad v \gets y_{k L + l} \gg t;\ \textbf{if}\ t + b > W
///     \ \textbf{then}\ v \gets v \vee ( y_{( k + 1 ) L + l} \ll ( W - t ) )
///     \cr
/// 6 & \qquad\quad x_{r L + l} \gets ( v \wedge ( 2^b - 1 ) ) + m \cr
/// 7 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre \f$ b \le W \f$
template< FastLanesWord T >
void fastLanesUnpack( const std::uint8_t * in , T * out , unsigned b ,
		      T base = 0 );

/// the FastLanes code of the unsigned integers \f$ x \f$ by the scheme
/// \f$ s \f$, a vector of 1024 at a time
///
/// In pseudocode (FastLanes, beyond the book), \f$ \tau \f$ and
/// \f$ \rho \f$ as in the file:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fastLanesEncode}( x , s ) \cr
/// 2 & \quad \textbf{foreach}\ \text{vector } v \text{ of 1024 values}
///     \text{ (the last one filled with its last value)}\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ s = \mathrm{FOR}\ \textbf{then}\ m \gets \min v;
///     \ b \gets \mathrm{bitwidth}( \max v - m );\ \text{write } b , m ,
///     \mathrm{fastLanesPack}( v , b , m ) \cr
/// 4 & \qquad \textbf{if}\ s = \mathrm{DELTA}\ \textbf{then} \cr
/// 5 & \qquad\quad \textbf{foreach}\ \text{lane } l\text{, row } r = 8 o +
///     s'\ \textbf{do}\ p \gets 16 \rho( o ) + 128 s' + l;\ d_p \gets
///     v_{\tau( p )} - v_{\tau( p ) - 1}\ ( 0 \text{ if } r = 0 ) \cr
/// 6 & \qquad\quad b \gets \mathrm{bitwidth}( \bigvee_p d_p );\ \text{write }
///     b \text{, the } v_{\tau( l )} \text{ and } \mathrm{fastLanesPack}( d
///     , b , 0 ) \cr
/// 7 & \qquad \textbf{if}\ s = \mathrm{RLE}\ \textbf{then}\ \text{write the }
///     k \text{ runs, their values, and their numbers by DELTA}
/// \end{array}}
/// \f]
template< std::ranges::contiguous_range R >
 requires FastLanesWord< std::ranges::range_value_t< R > >
[[nodiscard]] std::vector< std::uint8_t >
fastLanesEncode( const R & x , FastLanesScheme s = FastLanesScheme::FOR );

/// the \f$ n \f$ values of type `T` of the FastLanes code `code` by the
/// scheme \f$ s \f$
///
/// In pseudocode (FastLanes, beyond the book), for the differences:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fastLanesDecode}( y , n , \mathrm{DELTA} )
///     \cr
/// 2 & \quad \textbf{foreach}\ \text{vector of the code}\ \textbf{do} \cr
/// 3 & \qquad \text{read } b \text{ and the bases } a_0 , \ldots , a_{L-1}
///     \cr
/// 4 & \qquad \textbf{foreach}\ \text{lane } l\text{, all at once,
///     and row } r = 8 o + s \text{ (unrolled)}\ \textbf{do} \cr
/// 5 & \qquad\quad p \gets 16 \rho( o ) + 128 s + l;\ a_l \gets a_l +
///     \text{the row } \lfloor p / L \rfloor \text{ of the lane } l
///     \text{ unpacked};\ t_p \gets a_l \cr
/// 6 & \qquad \textbf{for}\ p \gets 0\ \textbf{to}\ 1023\ \textbf{do}
///     \ v_{\tau( p )} \gets t_p \cr
/// 7 & \quad \textbf{return}\ \text{the first } n \text{ values}
/// \end{array}}
/// \f]
/// \pre `code` is the code of \f$ n \f$ values of type `T` by \f$ s \f$
template< FastLanesWord T >
[[nodiscard]] std::vector< T >
fastLanesDecode( std::span< const std::uint8_t > code , std::size_t n ,
		 FastLanesScheme s = FastLanesScheme::FOR );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/FastLanes.tpp"

/*--------------------------------------------------------------------------*/

#endif /* FastLanes.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FastLanes.h ---------------------------*/
/*--------------------------------------------------------------------------*/
