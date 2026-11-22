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
 *   adders, each a full adder of three vectors of bits in five logical
 *   operations, and only the vector of the sixteens is counted, so that a
 *   block of 16 vectors costs one count instead of 16; a vector is counted
 *   by the count of the lanes of Google Highway, one instruction where the
 *   processor has it (AVX-512 VPOPCNTDQ, NEON) and otherwise the table of
 *   the 16 nibbles in a shuffle of bytes (Mula, Kurz and Lemire). The
 *   kernel is that of `ADS_WITH_HIGHWAY` [see `Simd.h`], which runs the
 *   vectors of the processor found when the program runs; the arrays of
 *   fewer than 16 words, and every array without Highway, are counted by
 *   the population count of `<bit>` in four independent sums;
 * -  `popcountAnd( a , b , n )` the 1 bits of the intersection of two arrays
 *   in the same way, the size of the intersection of two sets of bits;
 * -  `pdep()` and `pext()` scatter the low bits of \f$ x \f$ to the 1 bits of
 *   a mask and gather them back, by a loop over the 1 bits of the mask;
 * -  `select( x , r )`, the place of the 1 of rank \f$ r \f$ of \f$ x \f$, is
 *   `selectInWord()` [see `BitVector.h`];
 * -  `mortonEncode()` and `mortonDecode()` are `interleave()` and
 *   `deinterleave()` of `Bitwise.h`, by masks and shifts.
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
#include "ADS/Simd.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/succinct/BitVector.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

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
/// 2 & \quad \textbf{if}\ \text{there is no Highway, or } n < 16
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
[[nodiscard]] inline std::uint64_t popcount( const Word * w , std::size_t n );

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
/// In pseudocode (beyond the book):
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
/// In pseudocode (beyond the book):
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
/// In pseudocode (the compress of the bytes of Highway and the broadword
/// select of Vigna of `selectInWord()`, beyond the book), \f$ \beta_j \f$
/// the bit \f$ j \f$ of \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{select}( x , r ) \cr
/// 2 & \quad \textbf{if}\ \text{the compress of 64 bytes is one}
///     \text{ instruction}\ \textbf{then} \cr
/// 3 & \qquad v \gets \text{the bytes } 0 , \ldots , 63 \text{ compressed}
///     \text{ by the mask } ( \beta_0 , \ldots , \beta_{63} )\quad
///     \triangleright\ \text{the places of the 1s in order} \cr
/// 4 & \qquad \textbf{return}\ \text{the byte } r \text{ of } v \cr
/// 5 & \quad \textbf{return}\ \mathrm{selectInWord}( x , r )\quad
///     \triangleright\ \text{the broadword select}
/// \end{array}}
/// \f]
/// The test of line 2 is on the target of Highway at compile time (AVX-512
/// VBMI2), and it fails without `ADS_WITH_HIGHWAY` and in a constant
/// expression; both branches are those of `selectInWord()`
/// \pre \f$ 0 \le r < \nu( x ) \f$, the number of 1 bits of \f$ x \f$
[[nodiscard]] inline int select( Word x , int r );

/// the Morton code of \f$ ( x , y ) \f$, the bits of \f$ x \f$ in the even
/// places and those of \f$ y \f$ in the odd ones
///
/// In pseudocode (beyond the book), the deposit that `interleave()` of
/// `Bitwise.h` computes by masks and shifts:
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
/// In pseudocode (beyond the book), the extraction that `deinterleave()`
/// of `Bitwise.h` computes by masks and shifts:
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

#endif /* FastBitwise.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastBitwise.h --------------------------*/
/*--------------------------------------------------------------------------*/
