/*--------------------------------------------------------------------------*/
/*----------------------------- File Bitwise.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The broadword tricks of Knuth (7.1.3), beyond the book: operations on
 * the 64 bits of a word as on a vector of bits, with a few instructions and
 * no loop over the bits.
 *
 * - sidewaysSum() counts the 1 bits by adding them in parallel in fields of
 *   2, 4 and 8 bits and then the bytes by one multiplication;
 * - ruler() (\f$ \rho \f$, the trailing zeros) as the sideways sum of
 *   \f$ x \oplus ( x - 1 ) \f$ minus 1, lambda() (\f$ \lambda \f$,
 *   the place of the leading 1) by halving the width, and rightmostOne() and
 *   smearRight() by the identities \f$ x \& -x \f$ and
 *   \f$ x | ( x - 1 ) \f$;
 * - reverseBits() swaps halves, then quarters, and so on;
 * - gray() and grayInverse() convert between binary and Gray code;
 * - nextSameWeight() gives the next larger word with as many 1 bits (the
 *   trick of Gosper), which enumerates the k-subsets of 64 items;
 * - interleave() and deinterleave() give the Morton code of two 32-bit
 *   coordinates by the perfect shuffle of their bits, whose order keeps
 *   nearby points of the plane nearby;
 * - hasZeroByte() tells in O( 1 ) whether one of the 8 bytes of a word is 0,
 *   the test that makes strlen() fast.
 *
 * The contracts [see Contract.h]: ruler() and lambda() need x != 0, and
 * nextSameWeight() a word whose next one does not overflow. The functions
 * are defined in Bitwise.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A, Addison-Wesley,
 *      2011, section 7.1.3
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

/// the number of 1 bits of x
[[nodiscard]] constexpr int sidewaysSum( Word x );

/// the number of trailing zeros of x
/// \pre x != 0
[[nodiscard]] constexpr int ruler( Word x );

/// the place of the leading 1 of x, floor( log2 x )
/// \pre x != 0
[[nodiscard]] constexpr int lambda( Word x );

/// the rightmost 1 of x alone, 0 if x = 0
[[nodiscard]] constexpr Word rightmostOne( Word x );

/// x with the 0 bits below its rightmost 1 set
[[nodiscard]] constexpr Word smearRight( Word x );

/// the bits of x in reverse order
[[nodiscard]] constexpr Word reverseBits( Word x );

/// the Gray code of x
[[nodiscard]] constexpr Word gray( Word x );

/// the x whose Gray code is g
[[nodiscard]] constexpr Word grayInverse( Word g );

/// the next larger word with as many 1 bits as x
/// \pre x != 0 and the result fits in 64 bits
[[nodiscard]] constexpr Word nextSameWeight( Word x );

/// the Morton code of ( x , y ), the bits of x in the even places
[[nodiscard]] constexpr Word interleave( std::uint32_t x , std::uint32_t y );

/// the ( x , y ) of the Morton code z
[[nodiscard]] constexpr std::pair< std::uint32_t , std::uint32_t >
deinterleave( Word z );

/// whether one of the 8 bytes of x is 0
[[nodiscard]] constexpr bool hasZeroByte( Word x );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/Bitwise.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Bitwise.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Bitwise.h ---------------------------*/
/*--------------------------------------------------------------------------*/
