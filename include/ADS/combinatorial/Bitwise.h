/*--------------------------------------------------------------------------*/
/*----------------------------- File Bitwise.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The broadword tricks of Knuth (section 7.1.3 of The Art of Computer
 * Programming), beyond the book: operations on the 64 bits of a word as on a
 * vector of bits, with a few instructions and no loop over the bits. The
 * constants are written in hexadecimal with their repeated byte, e.g.,
 * \f$ \mathtt{0x5555 \ldots 55} \f$ the word of 8 bytes \f$ \mathtt{0x55} \f$,
 * and \f$ \& \f$, \f$ \mid \f$, \f$ \oplus \f$, \f$ \lnot \f$, \f$ \gg \f$ and
 * \f$ \ll \f$ are the operations of C++ on the bits.
 *
 * -  `sidewaysSum()` counts the 1 bits by adding them in parallel in fields of
 *   2, 4 and 8 bits and then the bytes by one multiplication;
 * -  `ruler()` (\f$ \rho \f$, the trailing zeros) as the sideways sum of
 *   \f$ x \oplus ( x - 1 ) \f$ minus 1, `lambda()` (\f$ \lambda \f$, the place
 *   of the leading 1) by halving the width, and `rightmostOne()` and
 *   `smearRight()` by the identities \f$ x \,\&\, -x \f$ and
 *   \f$ x \mid ( x - 1 ) \f$;
 * -  `reverseBits()` swaps halves, then quarters, and so on;
 * -  `gray()` and `grayInverse()` convert between binary and Gray code;
 * -  `nextSameWeight()` gives the next larger word with as many 1 bits (the
 *   trick of Gosper), which enumerates the \f$ k \f$-subsets of 64 items;
 * -  `interleave()` and `deinterleave()` give the Morton code of two
 *   coordinates of 32 bits by the perfect shuffle of their bits, whose order
 *   keeps nearby points of the plane nearby;
 * -  `hasZeroByte()` tells in \f$ O( 1 ) \f$ whether one of the 8 bytes of a
 *   word is 0, the test that makes `strlen()` fast.
 *
 * The contracts [see `Contract.h`]: `ruler()` and `lambda()` need
 * \f$ x \ne 0 \f$, and `nextSameWeight()` a word whose next one does not
 * overflow. The functions are defined in `Bitwise.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A, Addison-Wesley,
 *      2011, section 7.1.3
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012
 * \see M. Beeler, R. W. Gosper, R. Schroeppel, HAKMEM, MIT AI Memo 239,
 *      1972, item 175 (the next word with as many 1 bits)
 * \see F. Gray, Pulse code communication, U.S. Patent 2,632,058, 1953
 * \see G. M. Morton, A computer oriented geodetic data base and a new
 *      technique in file sequencing, IBM report, Ottawa, 1966
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

#ifndef __ADS_Bitwise
 #define __ADS_Bitwise
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstdint>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Word = std::uint64_t;   ///< a word of 64 bits

/// the number of 1 bits of \f$ x \f$, \f$ \nu( x ) \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), the sums of the
/// fields of 2, 4 and 8 bits formed in place:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sidewaysSum}( x ) \cr
/// 2 & \quad x \gets x - ( ( x \gg 1 ) \,\&\, \mathtt{0x5555 \ldots 55} )\quad
///     \triangleright\ \text{the fields of 2 bits} \cr
/// 3 & \quad x \gets ( x \,\&\, \mathtt{0x3333 \ldots 33} ) + ( ( x \gg 2 )
///     \,\&\, \mathtt{0x3333 \ldots 33} )\quad \triangleright\ \text{of 4} \cr
/// 4 & \quad x \gets ( x + ( x \gg 4 ) ) \,\&\, \mathtt{0x0F0F \ldots 0F}\quad
///     \triangleright\ \text{of 8, the bytes} \cr
/// 5 & \quad \textbf{return}\ ( x \cdot \mathtt{0x0101 \ldots 01} ) \gg
///     56\quad \triangleright\ \text{the sum of the bytes, in the top one}
/// \end{array}}
/// \f]
[[nodiscard]] constexpr int sidewaysSum( Word x );

/// the number of trailing zeros of \f$ x \f$, \f$ \rho( x ) \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book): \f$ x \oplus
/// ( x - 1 ) \f$ has a 1 in the place of the rightmost 1 of \f$ x \f$ and in
/// each place below it.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ruler}( x ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{sidewaysSum}( x \oplus ( x - 1 ) ) - 1
/// \end{array}}
/// \f]
/// \pre \f$ x \ne 0 \f$
[[nodiscard]] constexpr int ruler( Word x );

/// the place of the leading 1 of \f$ x \f$,
/// \f$ \lambda( x ) = \lfloor \log_2 x \rfloor \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), by halving the
/// width of the search:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lambda}( x ) \cr
/// 2 & \quad r \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ s \in ( 32 , 16 , 8 , 4 , 2 , 1 )\ \textbf{do}
///     \cr
/// 4 & \qquad \textbf{if}\ x \gg s \ne 0\ \textbf{then}\ x \gets x \gg s;\ r
///     \gets r + s \cr
/// 5 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ x \ne 0 \f$
[[nodiscard]] constexpr int lambda( Word x );

/// the rightmost 1 of \f$ x \f$ alone, 0 if \f$ x = 0 \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), \f$ -x \f$ the
/// complement of \f$ x - 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rightmostOne}( x ) \cr
/// 2 & \quad \textbf{return}\ x \,\&\, ( \lnot x + 1 )
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word rightmostOne( Word x );

/// \f$ x \f$ with the 0 bits below its rightmost 1 set, 0 if \f$ x = 0 \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{smearRight}( x ) \cr
/// 2 & \quad \textbf{if}\ x = 0\ \textbf{then return}\ 0
///     \ \textbf{else return}\ x \mid ( x - 1 )
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word smearRight( Word x );

/// the bits of \f$ x \f$ in reverse order
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), \f$ m_k \f$ the
/// mask of the fields of \f$ 2^k \f$ bits in the even places
/// (\f$ \mathtt{0x5555 \ldots 55} \f$, \f$ \mathtt{0x3333 \ldots 33} \f$,
/// \f$ \mathtt{0x0F0F \ldots 0F} \f$, \f$ \mathtt{0x00FF \ldots 00FF} \f$,
/// \f$ \mathtt{0x0000FFFF0000FFFF} \f$):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{reverseBits}( x ) \cr
/// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ 4\ \textbf{do}\quad
///     \triangleright\ \text{swap the fields of } 2^k \text{ bits} \cr
/// 3 & \qquad x \gets ( ( x \gg 2^k ) \,\&\, m_k ) \mid ( ( x \,\&\, m_k ) \ll
///     2^k ) \cr
/// 4 & \quad \textbf{return}\ ( x \gg 32 ) \mid ( x \ll 32 )\quad
///     \triangleright\ \text{the two halves}
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word reverseBits( Word x );

/// the Gray code of \f$ x \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{gray}( x ) \cr
/// 2 & \quad \textbf{return}\ x \oplus ( x \gg 1 )
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word gray( Word x );

/// the \f$ x \f$ whose Gray code is \f$ g \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book): each bit of
/// \f$ x \f$ is the exclusive or of the bits of \f$ g \f$ above it and in its
/// place, a prefix computed by doubling.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{grayInverse}( g ) \cr
/// 2 & \quad \textbf{foreach}\ s \in ( 1 , 2 , 4 , 8 , 16 , 32 )\ \textbf{do}
///     \ g \gets g \oplus ( g \gg s ) \cr
/// 3 & \quad \textbf{return}\ g
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word grayInverse( Word g );

/// the next larger word with as many 1 bits as \f$ x \f$
///
/// In pseudocode (the trick of Gosper, beyond the book): adding the
/// rightmost 1 carries the rightmost run of 1s one place up, and the 1s lost
/// in the carry, but one, go back to the bottom.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nextSameWeight}( x ) \cr
/// 2 & \quad u \gets x \,\&\, -x;\ v \gets x + u \cr
/// 3 & \quad \textbf{return}\ v + ( ( ( v \oplus x ) / u ) \gg 2 )
/// \end{array}}
/// \f]
/// \pre \f$ x \ne 0 \f$ and the result fits in 64 bits
[[nodiscard]] constexpr Word nextSameWeight( Word x );

/// the Morton code of \f$ ( x , y ) \f$, the bits of \f$ x \f$ in the even
/// places and those of \f$ y \f$ in the odd ones
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), the 32 bits of a
/// coordinate spread to the even places by halving the distance of their
/// blocks:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{interleave}( x , y ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{spread}( x ) \mid ( \mathrm{spread}( y )
///     \ll 1 ) \cr
/// 3 & \textbf{function}\ \mathrm{spread}( x ) \cr
/// 4 & \quad \textbf{foreach}\ ( s , m ) \in ( ( 16 ,
///     \mathtt{0x0000FFFF0000FFFF} ) , ( 8 , \mathtt{0x00FF \ldots 00FF} ) ,
///     ( 4 , \mathtt{0x0F0F \ldots 0F} ) , \cr
///   & \qquad ( 2 , \mathtt{0x3333 \ldots 33} ) , ( 1 , \mathtt{0x5555 \ldots
///     55} ) )\ \textbf{do}\ x \gets ( x \mid ( x \ll s ) ) \,\&\, m \cr
/// 5 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
[[nodiscard]] constexpr Word interleave( std::uint32_t x , std::uint32_t y );

/// the \f$ ( x , y ) \f$ of the Morton code \f$ z \f$
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book), the steps of
/// `interleave()` backwards:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{deinterleave}( z ) \cr
/// 2 & \quad \textbf{return}\ ( \mathrm{gather}( z ) , \mathrm{gather}( z \gg
///     1 ) ) \cr
/// 3 & \textbf{function}\ \mathrm{gather}( x ) \cr
/// 4 & \quad x \gets x \,\&\, \mathtt{0x5555 \ldots 55} \cr
/// 5 & \quad \textbf{foreach}\ ( s , m ) \in ( ( 1 , \mathtt{0x3333 \ldots
///     33} ) , ( 2 , \mathtt{0x0F0F \ldots 0F} ) , ( 4 , \mathtt{0x00FF \ldots
///     00FF} ) , \cr
///   & \qquad ( 8 , \mathtt{0x0000FFFF0000FFFF} ) , ( 16 ,
///     \mathtt{0x00000000FFFFFFFF} ) )\ \textbf{do}\ x \gets ( x \mid ( x \gg
///     s ) ) \,\&\, m \cr
/// 6 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
[[nodiscard]] constexpr std::pair< std::uint32_t , std::uint32_t >
deinterleave( Word z );

/// whether one of the 8 bytes of \f$ x \f$ is 0
///
/// In pseudocode (Knuth's section 7.1.3, beyond the book): the subtraction
/// borrows into the top bit of a byte that is 0, and the top bits of the
/// bytes of \f$ x \f$ that are set are masked out.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{hasZeroByte}( x ) \cr
/// 2 & \quad \textbf{return}\ ( ( x - \mathtt{0x0101 \ldots 01} ) \,\&\, \lnot
///     x \,\&\, \mathtt{0x8080 \ldots 80} ) \ne 0
/// \end{array}}
/// \f]
[[nodiscard]] constexpr bool hasZeroByte( Word x );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/impl/Bitwise.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Bitwise.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Bitwise.h ---------------------------*/
/*--------------------------------------------------------------------------*/
