/*--------------------------------------------------------------------------*/
/*--------------------------- File FastBitwise.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered operations on words and on arrays of words, beyond the
 * book [see Bitwise.h for the broadword tricks of Knuth, which compute the
 * same results by the identities of 7.1.3 alone].
 *
 * - popcount( w , n ) counts the 1 bits of n words by the method of Harley
 *   and Seal: 16 vectors at a time are added by a tree of carry-save
 *   adders, each a full adder of three vectors of bits in two operations
 *   (one ternary logic each on AVX-512), and only the vector of the
 *   sixteens is counted, so that a block of 16 vectors costs one count
 *   instead of 16; a vector is counted by the instruction of the processor
 *   if it has one (AVX-512 VPOPCNTDQ), and otherwise by the table of the 16
 *   nibbles in a shuffle of bytes and the sums of 8 bytes by psadbw
 *   (Mula, Kurz and Lemire). The arrays shorter than a block, and every
 *   array on a machine without AVX2, are counted by the population count
 *   of the processor in four independent sums;
 * - popcountAnd( a , b , n ) the 1 bits of the intersection of two arrays
 *   in the same way, the size of the intersection of two sets of bits;
 * - pdep() and pext() scatter the low bits of x to the 1 bits of a mask
 *   and gather them back, by one instruction of BMI2, and otherwise by a
 *   loop over the 1 bits of the mask;
 * - select( x , r ), the place of the 1 of rank r of x, is the trailing
 *   zeros of pdep( 2^r , x ) with BMI2, and otherwise the byte of the 1 by
 *   the sums of the bytes of sidewaysSum() and a comparison of the 8 bytes
 *   in parallel (Vigna), then the 1 in the byte;
 * - mortonEncode() and mortonDecode() are interleave() and deinterleave()
 *   of Bitwise.h by pdep() and pext(), two instructions each way.
 *
 * The contracts [see Contract.h]: select( x , r ) needs r < the 1 bits of
 * x. The functions are defined in FastBitwise.tpp.
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
 *         Universita' di Pisa \n
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

/// the 1 bits of the n words from w
[[nodiscard]] inline std::uint64_t popcount( const Word * w ,
                                             std::size_t n );

/// the 1 bits of the n words of a & b
[[nodiscard]] inline std::uint64_t popcountAnd( const Word * a ,
                                                const Word * b ,
                                                std::size_t n );

/// the low bits of x to the places of the 1 bits of mask, in order
[[nodiscard]] inline Word pdep( Word x , Word mask );

/// the bits of x at the places of the 1 bits of mask, packed low
[[nodiscard]] inline Word pext( Word x , Word mask );

/// the place of the 1 of rank r of x, from 0
/// \pre r < the number of 1 bits of x
[[nodiscard]] inline int select( Word x , int r );

/// the Morton code of ( x , y ), the bits of x in the even places
[[nodiscard]] inline Word mortonEncode( std::uint32_t x , std::uint32_t y );

/// the ( x , y ) of the Morton code z
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
