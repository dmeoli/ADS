/*--------------------------------------------------------------------------*/
/*--------------------------- File FastBitwise.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered operations on words and on arrays of words, beyond the book
 * [see `Bitwise.h` for the broadword tricks of Knuth, which compute the same
 * results by the identities of his section 7.1.3 alone].
 *
 * -  `popcount( w , n )` counts the 1 bits of \f$ n \f$ words by the method of
 *   Harley and Seal: 16 vectors at a time are added by a tree of carry-save
 *   adders, each a full adder of three vectors of bits in two operations (one
 *   ternary logic each on AVX-512), and only the vector of the sixteens is
 *   counted, so that a block of 16 vectors costs one count instead of 16; a
 *   vector is counted by the instruction of the processor if it has one
 *   (AVX-512 VPOPCNTDQ), and otherwise by the table of the 16 nibbles in a
 *   shuffle of bytes and the sums of 8 bytes by `psadbw` (Mula, Kurz and
 *   Lemire). The arrays shorter than a block, and every array on a machine
 *   without AVX2, are counted by the population count of the processor in four
 *   independent sums;
 * -  `popcountAnd( a , b , n )` the 1 bits of the intersection of two arrays
 *   in the same way, the size of the intersection of two sets of bits;
 * -  `pdep()` and `pext()` scatter the low bits of \f$ x \f$ to the 1 bits of
 *   a mask and gather them back, by one instruction of BMI2, and otherwise by
 *   a loop over the 1 bits of the mask;
 * -  `select( x , r )`, the place of the 1 of rank \f$ r \f$ of \f$ x \f$, is
 *   the trailing zeros of `pdep( 1 << r , x )` with BMI2, and otherwise the
 *   byte of the 1 by the sums of the bytes of `sidewaysSum()` and a comparison
 *   of the 8 bytes in parallel (Vigna), then the 1 in the byte;
 * -  `mortonEncode()` and `mortonDecode()` are `interleave()` and
 *   `deinterleave()` of `Bitwise.h` by `pdep()` and `pext()`, two instructions
 *   each way.
 *
 * The contracts [see `Contract.h`]: `select( x , r )` needs \f$ r \f$ smaller
 * than the number of 1 bits of \f$ x \f$. The functions are defined in
 * `FastBitwise.tpp`.
 *
 * \see W. Mula, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
 * \see S. Vigna, Broadword implementation of rank/select queries, WEA
 *      2008, LNCS 5038, 154-168, doi:10.1007/978-3-540-68552-4_12
 * \see P. Pandey, M. A. Bender, R. Johnson, A fast x86 implementation of
 *      select, arXiv:1706.00990, 2017
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

#ifndef __ADS_FastBitwise
 #define __ADS_FastBitwise
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

#if defined( __AVX2__ ) || defined( __AVX512F__ ) || defined( __BMI2__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the 1 bits of the \f$ n \f$ words from \f$ w \f$
///
/// In pseudocode (the method of Harley and Seal after Mula, Kurz and Lemire,
/// beyond the book), \f$ \nu \f$ the number of 1 bits, \f$ V \f$ the words
/// of a vector of the processor and \f$ \mathrm{csa}( a , b , c ) \f$ the
/// full adder of three vectors, which
/// gives the carries \f$ \mathrm{maj}( a , b , c ) \f$ and the sums
/// \f$ a \oplus b \oplus c \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{popcount}( w , n ) \cr
/// 2 & \quad \textbf{if}\ \text{there are no vectors, or } n < 16 V
///     \ \textbf{then return}\ \sum_i \nu( w_i ) \text{ in four sums} \cr
/// 3 & \quad o , t , f , e , T \gets 0\quad \triangleright\ \text{the ones,}
///     \text{ twos, fours, eights and the total, by vectors} \cr
/// 4 & \quad \textbf{foreach}\ \text{block of 16 vectors } v_0 , \ldots ,
///     v_{15}\ \textbf{do} \cr
/// 5 & \qquad ( t_A , o ) \gets \mathrm{csa}( o , v_0 , v_1 );\ ( t_B , o )
///     \gets \mathrm{csa}( o , v_2 , v_3 );\ ( f_A , t ) \gets \mathrm{csa}( t
///     , t_A , t_B ) \cr
/// 6 & \qquad ( f_B , t ) \gets \text{the same on } v_4 , \ldots , v_7;\ ( e_A
///     , f ) \gets \mathrm{csa}( f , f_A , f_B ) \cr
/// 7 & \qquad ( e_B , f ) \gets \text{the same on } v_8 , \ldots , v_{15};
///     \ ( s , e ) \gets \mathrm{csa}( e , e_A , e_B ) \cr
/// 8 & \qquad T \gets T + \nu( s )\quad \triangleright\ \text{the sixteens,}
///     \text{ the only count of the block} \cr
/// 9 & \quad T \gets 16 T + 8 \nu( e ) + 4 \nu( f ) + 2 \nu( t ) + \nu( o ) +
///     \sum \nu( v ) \text{ on the vectors left} \cr
/// 10 & \quad \textbf{return}\ \text{the sum of the lanes of } T + \sum_i
///      \nu( w_i ) \text{ on the words left}
/// \end{array}}
/// \f]
[[nodiscard]] inline std::uint64_t popcount( const Word * w ,
                                             std::size_t n );

/// the 1 bits of the \f$ n \f$ words of \f$ a \,\&\, b \f$
///
/// In pseudocode (as `popcount()` above, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{popcountAnd}( a , b , n ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{popcount}( ( a_i \,\&\, b_i )_{i < n} ,
///     n )\quad \triangleright\ \text{each } a_i \,\&\, b_i \text{ formed}
///     \text{ when loaded}
/// \end{array}}
/// \f]
[[nodiscard]] inline std::uint64_t popcountAnd( const Word * a ,
                                                const Word * b ,
                                                std::size_t n );

/// the low bits of \f$ x \f$ to the places of the 1 bits of `mask`, in order
///
/// In pseudocode (beyond the book), the instruction `pdep` of BMI2 if there
/// is one, and otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pdep}( x , m ) \cr
/// 2 & \quad r \gets 0;\ b \gets 1 \cr
/// 3 & \quad \textbf{while}\ m \ne 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \,\&\, b \ne 0\ \textbf{then}\ r \gets r \mid ( m
///     \,\&\, -m )\quad \triangleright\ \text{the lowest 1 of } m \cr
/// 5 & \qquad m \gets m \,\&\, ( m - 1 );\ b \gets 2 b \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
[[nodiscard]] inline Word pdep( Word x , Word mask );

/// the bits of \f$ x \f$ at the places of the 1 bits of `mask`, packed low
///
/// In pseudocode (beyond the book), the instruction `pext` of BMI2 if there
/// is one, and otherwise:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pext}( x , m ) \cr
/// 2 & \quad r \gets 0;\ b \gets 1 \cr
/// 3 & \quad \textbf{while}\ m \ne 0\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \,\&\, m \,\&\, -m \ne 0\ \textbf{then}\ r \gets
///     r \mid b \cr
/// 5 & \qquad m \gets m \,\&\, ( m - 1 );\ b \gets 2 b \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
[[nodiscard]] inline Word pext( Word x , Word mask );

/// the place of the 1 of rank \f$ r \f$ of \f$ x \f$, from 0
///
/// In pseudocode (Vigna's broadword select, beyond the book), \f$ \rho \f$
/// the trailing zeros and \f$ s_j \f$ the 1 bits of the bytes
/// \f$ 0 , \ldots , j \f$ of \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{select}( x , r ) \cr
/// 2 & \quad \textbf{if}\ \text{BMI2}\ \textbf{then return}\ \rho(
///     \mathrm{pdep}( 2^r , x ) ) \cr
/// 3 & \quad s \gets \text{the sums of the bytes of } x \text{, as in }
///     \mathrm{sidewaysSum}() \text{, times } \mathtt{0x0101 \ldots 01}\quad
///     \triangleright\ \text{the bytes } s_j \cr
/// 4 & \quad \ell \gets ( ( r \cdot \mathtt{0x0101 \ldots 01} \mid
///     \mathtt{0x8080 \ldots 80} ) - s ) \,\&\, \mathtt{0x8080 \ldots 80}\quad
///     \triangleright\ \text{the bytes with } s_j \le r \cr
/// 5 & \quad k \gets 8 \nu( \ell )\quad \triangleright\ \text{the place of}
///     \text{ the byte of the 1} \cr
/// 6 & \quad r \gets r - s_{k / 8 - 1}\ ( 0 \text{ if } k = 0 );\ b \gets ( x
///     \gg k ) \,\&\, \mathtt{0xFF} \cr
/// 7 & \quad \textbf{repeat}\ r \text{ times}:\ b \gets b \,\&\, ( b - 1 ) \cr
/// 8 & \quad \textbf{return}\ k + \rho( b )
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le r < \nu( x ) \f$, the number of 1 bits of \f$ x \f$
[[nodiscard]] inline int select( Word x , int r );

/// the Morton code of \f$ ( x , y ) \f$, the bits of \f$ x \f$ in the even
/// places and those of \f$ y \f$ in the odd ones
///
/// In pseudocode (beyond the book), `interleave()` of `Bitwise.h` without
/// BMI2:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mortonEncode}( x , y ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{pdep}( x , \mathtt{0x5555 \ldots 55} )
///     \mid \mathrm{pdep}( y , \mathtt{0xAAAA \ldots AA} )
/// \end{array}}
/// \f]
[[nodiscard]] inline Word mortonEncode( std::uint32_t x , std::uint32_t y );

/// the \f$ ( x , y ) \f$ of the Morton code \f$ z \f$
///
/// In pseudocode (beyond the book), `deinterleave()` of `Bitwise.h` without
/// BMI2:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mortonDecode}( z ) \cr
/// 2 & \quad \textbf{return}\ ( \mathrm{pext}( z , \mathtt{0x5555 \ldots 55} )
///     , \mathrm{pext}( z , \mathtt{0xAAAA \ldots AA} ) )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::pair< std::uint32_t , std::uint32_t >
mortonDecode( Word z );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/impl/FastBitwise.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FastBitwise.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastBitwise.h --------------------------*/
/*--------------------------------------------------------------------------*/
