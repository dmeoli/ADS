/*--------------------------------------------------------------------------*/
/*---------------------------- File BitStream.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The streams of bits on which the integer codes are written and read [see
 * IntegerCodes.h]: BitWriter appends bits to a BitVector [see BitVector.h]
 * and BitReader reads them back from a place that moves forward, both in
 * \f$ O( 1 ) \f$ time per field of up to 64 bits.
 *
 * - write( v , len ) and read( len ) move a field with its least
 *   significant bit first, the order of the words, with two shifts;
 * - writeBinary( v , len ) and readBinary( len ) move it with its most
 *   significant bit first, the order in which the codes are written on
 *   paper (\f$ \gamma( 9 ) = 0001001 \f$), so that the bits of the stream
 *   read from the first one are the codewords of the notes; the field is
 *   turned around by reverseBits() [see Bitwise.h], 5 rounds of masks;
 * - writeUnary( q ) writes q zeros and a 1, the unary code \f$ U( q + 1 )
 *   \f$ of the notes, and readUnary() counts the zeros up to the 1 a word at
 *   a time, by the trailing zeros of the word, ruler() [see Bitwise.h],
 *   rather than a bit at a time, in \f$ O( 1 + q / 64 ) \f$ time.
 *
 * Writing the codes into words of 64 bits, rather than bytes, is what the
 * fast decoders of the literature do (e.g., the bit streams of the
 * WebGraph framework of Boldi and Vigna and of the succinct libraries): a
 * codeword of up to 64 bits spans at most two words, and the unary part is
 * decoded with one instruction per word.
 *
 * The contracts [see Contract.h]: a field has at most 64 bits and its value
 * fits in them, and a reader never reads past the end of its bits. The
 * functions are defined in BitStream.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 11
 * \see P. Boldi, S. Vigna, The WebGraph framework I: compression
 *      techniques, Proc. 13th WWW, 595-602, 2004,
 *      doi:10.1145/988672.988752
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

#ifndef __ADS_BitStream
 #define __ADS_BitStream
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/succinct/BitVector.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BitWriter -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a stream of bits written at its end, on a BitVector
class BitWriter {

/*--------------------------------------------------------------------------*/
 public:

 BitWriter( void ) = default;

 /// the bits written so far
 [[nodiscard]] std::size_t size( void ) const;

 void writeBit( bool b );

 /// the len bits of v, the least significant first
 /// \pre len <= 64 and v < 2^len
 void write( Word v , int len );

 /// the len bits of v, the most significant first
 /// \pre len <= 64 and v < 2^len
 void writeBinary( Word v , int len );

 /// q zeros and a 1
 void writeUnary( std::uint64_t q );

 /// the bits written
 [[nodiscard]] const BitVector & bits( void ) const;

 /// the bits written, which the writer leaves empty
 [[nodiscard]] BitVector release( void );

/*--------------------------------------------------------------------------*/
 private:

 BitVector f_b;

 };  // end( class BitWriter )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BitReader -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a stream of bits read forward from a place of a BitVector, which must
/// outlive the reader
class BitReader {

/*--------------------------------------------------------------------------*/
 public:

 /// the bits of b from the place p
 /// \pre p <= b.size()
 explicit BitReader( const BitVector & b , std::size_t p = 0 );

 /// the place of the next bit
 [[nodiscard]] std::size_t position( void ) const;

 /// moves to the place p
 /// \pre p <= the size of the bits
 void seek( std::size_t p );

 /// whether all the bits have been read
 [[nodiscard]] bool atEnd( void ) const;

 /// \pre ! atEnd()
 [[nodiscard]] bool readBit( void );

 /// the next len bits, the first in the least significant place
 /// \pre len <= 64 and len bits are left
 [[nodiscard]] Word read( int len );

 /// the next len bits, the first in the most significant place
 /// \pre len <= 64 and len bits are left
 [[nodiscard]] Word readBinary( int len );

 /// the number of zeros before the next 1, which is read too
 /// \pre a 1 is left
 [[nodiscard]] std::uint64_t readUnary( void );

/*--------------------------------------------------------------------------*/
 private:

 const BitVector * f_b;
 std::size_t f_p;

 };  // end( class BitReader )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BitStream.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BitStream.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BitStream.h --------------------------*/
/*--------------------------------------------------------------------------*/
