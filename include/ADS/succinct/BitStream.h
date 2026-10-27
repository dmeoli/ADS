/*--------------------------------------------------------------------------*/
/*---------------------------- File BitStream.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The streams of bits on which the integer codes are written and read [see
 * `IntegerCodes.h`]: `BitWriter` appends bits to a `BitVector` [see
 * `BitVector.h`] and `BitReader` reads them back from a place that moves
 * forward, both in \f$ O( 1 ) \f$ time per field of up to 64 bits.
 *
 * - `write( v , len )` and `read( len )` move a field with its least
 *   significant bit first, the order of the words, with two shifts;
 * - `writeBinary( v , len )` and `readBinary( len )` move it with its most
 *   significant bit first, the order in which the codes are written on
 *   paper (\f$ \gamma( 9 ) = 0001001 \f$), so that the bits of the stream
 *   read from the first one are the codewords of chapter 11, Integer
 *   Coding, of Ferragina's book; the field is turned around by three
 *   rounds of masks and a swap of the bytes (the five rounds of
 *   `reverseBits()` of `Bitwise.h` in a constant expression);
 * - `writeUnary( q )` writes \f$ q \f$ zeros and a 1, the unary code
 *   \f$ U( q + 1 ) \f$ of the same chapter, and `readUnary()` counts the
 *   zeros up to the 1 a word at a time, by the trailing zeros of the word,
 *   rather than a bit at a time, in \f$ O( 1 + q / 64 ) \f$ time.
 *
 * Writing the codes into words of 64 bits, rather than bytes, is what the
 * fast decoders of the literature do (e.g., the bit streams of the
 * WebGraph framework of Boldi and Vigna and of the succinct libraries): a
 * codeword of up to 64 bits spans at most two words, and the unary part is
 * decoded with one instruction per word.
 *
 * The contracts [see `Contract.h`]: a field has at most 64 bits and its
 * value fits in them, and a reader never reads past the end of its bits.
 * The functions are defined in `BitStream.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 11, Integer Coding, doi:10.1017/9781009128933
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

/// a stream of bits written at its end, on a `BitVector`
class BitWriter {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty stream
 BitWriter( void ) = default;

 /// the bits written so far
 [[nodiscard]] std::size_t size( void ) const;

 /// appends the bit \f$ b \f$
 void writeBit( bool b );

 /// the \f$ \mathit{len} \f$ bits of \f$ v \f$, the least significant first
 /// \pre \f$ \mathit{len} \le 64 \f$ and \f$ v < 2^{\mathit{len}} \f$
 void write( Word v , int len );

 /// the \f$ \mathit{len} \f$ bits of \f$ v \f$, the most significant first
 ///
 /// In pseudocode (the binary code of chapter 11, Integer Coding, of
 /// Ferragina's book, beyond the book), the field turned around and
 /// appended as `write()` does:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{writeBinary}( v , \mathit{len} ) \cr
 /// 2 & \quad r \gets \text{the } \mathit{len} \text{ low bits of } v
 ///     \text{ in the reverse order} \cr
 /// 3 & \quad \mathrm{write}( r , \mathit{len} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{len} \le 64 \f$ and \f$ v < 2^{\mathit{len}} \f$
 void writeBinary( Word v , int len );

 /// \f$ q \f$ zeros and a 1, the unary code \f$ U( q + 1 ) \f$
 ///
 /// In pseudocode (the unary code of chapter 11, Integer Coding, of
 /// Ferragina's book, beyond the book), whole words of zeros first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{writeUnary}( q ) \cr
 /// 2 & \quad \textbf{while}\ q \ge 64\ \textbf{do}\ \mathrm{write}( 0 , 64
 ///     );\ q \gets q - 64 \cr
 /// 3 & \quad \mathrm{write}( 2^q , q + 1 )\quad \triangleright\ q
 ///     \ \text{zeros, then a 1}
 /// \end{array}}
 /// \f]
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

/// a stream of bits read forward from a place of a `BitVector`, which must
/// outlive the reader
class BitReader {

/*--------------------------------------------------------------------------*/
 public:

 /// the bits of \f$ b \f$ from the place \f$ p \f$
 /// \pre \f$ p \le \f$ `b.size()`
 explicit BitReader( const BitVector & b , std::size_t p = 0 );

 /// the place of the next bit
 [[nodiscard]] std::size_t position( void ) const;

 /// moves to the place \f$ p \f$
 /// \pre \f$ p \f$ is at most the size of the bits
 void seek( std::size_t p );

 /// whether all the bits have been read
 [[nodiscard]] bool atEnd( void ) const;

 /// the next bit
 /// \pre not `atEnd()`
 [[nodiscard]] bool readBit( void );

 /// the next \f$ \mathit{len} \f$ bits, the first in the least significant
 /// place
 /// \pre \f$ \mathit{len} \le 64 \f$ and \f$ \mathit{len} \f$ bits are
 ///      left
 [[nodiscard]] Word read( int len );

 /// the next \f$ \mathit{len} \f$ bits, the first in the most significant
 /// place, the field of `writeBinary()`
 /// \pre \f$ \mathit{len} \le 64 \f$ and \f$ \mathit{len} \f$ bits are
 ///      left
 [[nodiscard]] Word readBinary( int len );

 /// the number of zeros before the next 1, which is read too
 ///
 /// In pseudocode (the unary code of chapter 11, Integer Coding, of
 /// Ferragina's book, beyond the book), a word at a time, \f$ p \f$ the
 /// place of the reader and
 /// \f$ \mathrm{ctz} \f$ the trailing zeros of a word:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{readUnary}() \cr
 /// 2 & \quad q \gets 0 \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \mathit{len} \gets \min\{ 64 , \text{the bits left} \};
 ///     \ w \gets \mathrm{get}( p , \mathit{len} ) \cr
 /// 5 & \qquad \textbf{if}\ w \ne 0\ \textbf{then} \cr
 /// 6 & \qquad\quad z \gets \mathrm{ctz}( w );\ p \gets p + z + 1;
 ///     \ \textbf{return}\ q + z \cr
 /// 7 & \qquad q \gets q + \mathit{len};\ p \gets p + \mathit{len}
 /// \end{array}}
 /// \f]
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
