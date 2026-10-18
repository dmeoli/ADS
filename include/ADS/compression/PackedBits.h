/*--------------------------------------------------------------------------*/
/*--------------------------- File PackedBits.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The bits of the compressors of this folder, packed in bytes, since their
 * input and their output are streams of bytes: a PackedBitWriter appends
 * bits, the first one in the most significant bit of the first byte, and a
 * PackedBitReader reads them back in the same order. A code of k bits goes
 * in with one call, the most significant bit first, so that the value of
 * the bits read so far is the integer whose binary digits they are; this is
 * the order in which the canonical code of Huffman [see
 * CanonicalHuffman.h] and the arithmetic coder [see ArithmeticCoding.h]
 * read their input, one bit at a time, and in which a dyadic fraction
 * \f$ v / 2^k \f$ is written as \f$ 0.b_1 b_2 \ldots b_k \f$. The last
 * byte is padded with zeros, and a PackedBitReader gives zeros past the end
 * of its bytes, which is what the decoder of the arithmetic code expects;
 * the decoders that must not read past the end check it with position().
 * The streams of bits on words of 64 bits, with the fields read a word at
 * a time, are those of the integer codes [see BitStream.h].
 *
 * The contracts [see Contract.h]: a code has at most 64 bits. The
 * operations are defined in PackedBits.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12
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

#ifndef __ADS_PackedBits
 #define __ADS_PackedBits
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS PackedBitWriter --------------------------*/
/*--------------------------------------------------------------------------*/

/// a growing sequence of bits, packed in bytes, the first bit in the most
/// significant bit of the first byte
class PackedBitWriter {

/*--------------------------------------------------------------------------*/
 public:

 /// appends the bit b
 void put( bool b );

 /// appends the k low bits of v, the most significant first
 /// \pre 0 <= k <= 64, v < 2^k
 void put( std::uint64_t v , int k );

 /// appends the bits of s, a string of '0' and '1'
 /// \pre s has only '0' and '1'
 void put( const std::string & s );

 /// the number of bits written
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the bytes, the last one padded with zeros
 [[nodiscard]] const std::vector< std::uint8_t > & bytes( void ) const {
  return( f_bytes );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::uint8_t > f_bytes;
 std::size_t f_size = 0;               ///< the bits written

 };  // end( class PackedBitWriter )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS PackedBitReader --------------------------*/
/*--------------------------------------------------------------------------*/

/// reads the bits of a sequence of bytes in the order of a
/// PackedBitWriter, and zeros past its end
class PackedBitReader {

/*--------------------------------------------------------------------------*/
 public:

 /// the reader of the bytes b, which must outlive it
 explicit PackedBitReader( std::span< const std::uint8_t > b )
  : f_bytes( b ) {}

 /// the next bit, 0 past the end
 bool get( void );

 /// the value of the next k bits, the first the most significant
 /// \pre 0 <= k <= 64
 std::uint64_t get( int k );

 /// the number of bits read, past the end too
 [[nodiscard]] std::size_t position( void ) const { return( f_pos ); }

 /// the number of bits of the bytes
 [[nodiscard]] std::size_t size( void ) const {
  return( 8 * f_bytes.size() );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::span< const std::uint8_t > f_bytes;
 std::size_t f_pos = 0;                ///< the bits read

 };  // end( class PackedBitReader )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/PackedBits.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PackedBits.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PackedBits.h --------------------------*/
/*--------------------------------------------------------------------------*/
