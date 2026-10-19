/*--------------------------------------------------------------------------*/
/*--------------------------- File IntegerCodes.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The codes of the integers of chapter 11 of the notes: self-delimiting
 * binary codewords for a sequence of positive integers, short for the
 * small ones, written one after the other on a stream of bits [see
 * BitStream.h] or of bytes. Each code is optimal, by Shannon, for the
 * distribution \f$ P[ x ] = 2^{-| c( x ) |} \f$, and the choice among
 * them is the choice of the distribution of the data (e.g., the d-gaps of
 * the lists of an inverted index, dGaps()). With \f$ B( x ) \f$ the binary
 * code of x, of \f$ | B( x ) | = 1 + \lfloor \log_2 x \rfloor \f$ bits:
 *
 * - the unary code \f$ U( x ) = 0^{x-1} 1 \f$, x bits, optimal for
 *   \f$ 2^{-x} \f$ (Fact 11.1);
 * - Elias' \f$ \gamma( x ) = 0^{| B( x ) | - 1} B( x ) \f$, \f$ 2 \lfloor
 *   \log_2 x \rfloor + 1 \f$ bits, optimal for \f$ 1 / ( 2 x^2 ) \f$
 *   (Fact 11.3);
 * - Elias' \f$ \delta( x ) = \gamma( | B( x ) | ) B( x ) \f$, \f$ 1 + 2
 *   \lfloor \log_2 | B( x ) | \rfloor + | B( x ) | \f$ bits, \f$ 1 + o( 1
 *   ) \f$ times the binary code (Fact 11.4); the notes keep the leading 1
 *   of \f$ B( x ) \f$, which Elias drops since the length tells it, and so
 *   do these functions (one bit more than Elias' code);
 * - Rice's \f$ R_k( x ) = U( q + 1 ) B_k( r ) \f$, \f$ q = \lfloor ( x - 1 )
 *   / 2^k \rfloor \f$ and r the remainder in k bits, \f$ q + 1 + k \f$
 *   bits (Fact 11.5), and Golomb's code of modulus b, of which Rice's is the
 *   case \f$ b = 2^k \f$, whose remainder is in the truncated binary code
 *   (\f$ \lfloor \log_2 b \rfloor \f$ or \f$ \lceil \log_2 b \rceil \f$
 *   bits): optimal for the geometric distribution \f$ ( 1 - p )^{x-1} p \f$
 *   with \f$ b \approx \ln 2 / p \f$ (Gallager and van Voorhis);
 * - PForDelta, a block of values coded in b bits each after subtracting a
 *   base, the values out of \f$ [ base , base + 2^b - 2 ] \f$ being
 *   exceptions marked by the escape \f$ 2^b - 1 \f$ and written apart in 64
 *   bits (Fact 11.6), with b chosen so that 90% of the values fit; it is
 *   decoded with no branch per value, the design of Zukowski, Heman, Nes and
 *   Boncz, whose exceptions are linked in place of the escapes, and of the
 *   later OptPFD of Yan, Ding and Suel, which chooses b per block;
 * - the variable-byte code, \f$ B( x ) \f$ in groups of 7 bits from the
 *   most significant, each in a byte whose high bit is 1 except in the last,
 *   \f$ 8 \lceil | B( x ) | / 7 \rceil \f$ bits (Fact 11.7), and the
 *   \f$ ( s , c ) \f$-dense codes of Brisaboa, Farina, Navarro and Parama,
 *   which split the \f$ s + c = 2^w \f$ values of a word of w bits into s
 *   stoppers and c continuers: \f$ s c^{k-1} \f$ integers take k words,
 *   and denseOptimalS() gives the s of the smallest output for given
 *   frequencies (the notes recall that the minimum is unique); the
 *   variable-byte code has \f$ s = c = 128 \f$ but wastes the codes of the
 *   leading groups of zeros, which the dense code with \f$ s = c = 128 \f$
 *   does not;
 * - the interpolative code of Moffat and Stuiver, for a strictly increasing
 *   sequence in \f$ [ low , hi ] \f$: the middle item is written in
 *   \f$ \lceil \log_2 ( hi - low - r + l + 1 ) \rceil \f$ bits between the
 *   bounds left by the items around it, then the two halves recursively
 *   (Algorithm 11.1), no bit at all for a run of consecutive integers; it
 *   adapts to the clusters of the sequence and is not prefix-free, which is
 *   why it can beat Huffman's code, optimal only among the static
 *   prefix-free codes, at the price of a slow, recursive decoding.
 *
 * The Elias-Fano code of section 11.6, which can be searched in place, is
 * in EliasFano.h. signedToNatural() maps an integer of any sign to a
 * natural number as the notes do (x to 2x, -x to 2x + 1), so that the
 * codes of the positive integers apply to it.
 *
 * Each code writes its bits with the most significant first, so that the
 * stream equals the codeword on paper (\f$ \gamma( 9 ) = 0001001 \f$,
 * \f$ R_4( 83 ) = 0000010010 \f$), and has a function of its length, the
 * formula of the notes, which the encoder ensures. The decoders read a
 * word at a time [see BitStream.h]; the fast decoders of the literature
 * (e.g., the tables of the unary parts, or the SIMD decoding of the
 * variable-byte code by Lemire and Boytsov) are refinements of these.
 *
 * The contracts [see Contract.h]: the integers are positive where the code
 * needs it (unary, gamma, delta, Rice, Golomb), the parameters are in
 * range (\f$ 0 \le k < 64 \f$, \f$ b \ge 1 \f$, \f$ s , c \ge 1 \f$), the
 * interpolative code gets a strictly increasing sequence in
 * \f$ [ low , hi ] \f$, and each encoder ensures that it wrote as many bits
 * as its length function says; decode( encode( x ) ) == x is the audit of
 * PForDelta. The functions are defined in IntegerCodes.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 11
 * \see P. Elias, Universal codeword sets and representations of the
 *      integers, IEEE Transactions on Information Theory 21(2), 194-203,
 *      1975, doi:10.1109/TIT.1975.1055349
 * \see S. W. Golomb, Run-length encodings, IEEE Transactions on Information
 *      Theory 12(3), 399-401, 1966, doi:10.1109/TIT.1966.1053907
 * \see R. G. Gallager, D. C. van Voorhis, Optimal source codes for
 *      geometrically distributed integer alphabets, IEEE Transactions on
 *      Information Theory 21(2), 228-230, 1975,
 *      doi:10.1109/TIT.1975.1055357
 * \see R. F. Rice, Some practical universal noiseless coding techniques,
 *      JPL Publication 79-22, Jet Propulsion Laboratory, 1979
 * \see M. Zukowski, S. Heman, N. Nes, P. Boncz, Super-scalar RAM-CPU cache
 *      compression, Proc. 22nd ICDE, 59, 2006, doi:10.1109/ICDE.2006.150
 * \see H. Yan, S. Ding, T. Suel, Inverted index compression and query
 *      processing with optimized document ordering, Proc. 18th WWW,
 *      401-410, 2009, doi:10.1145/1526709.1526764
 * \see N. R. Brisaboa, A. Farina, G. Navarro, J. R. Parama, Lightweight
 *      natural language text compression, Information Retrieval 10(1),
 *      1-33, 2007, doi:10.1007/s10791-006-9001-9
 * \see A. Moffat, L. Stuiver, Binary interpolative coding for effective
 *      index compression, Information Retrieval 3(1), 25-47, 2000,
 *      doi:10.1023/A:1013002601898
 * \see D. Lemire, L. Boytsov, Decoding billions of integers per second
 *      through vectorization, Software: Practice and Experience 45(1),
 *      1-29, 2015, doi:10.1002/spe.2203
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

#ifndef __ADS_IntegerCodes
 #define __ADS_IntegerCodes
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/succinct/BitStream.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*---------------------------- PREPROCESSING -------------------------------*/

/// | B( x ) |, the bits of the binary code of x, 1 + floor( log2 x )
/// \pre x >= 1
[[nodiscard]] constexpr int binaryLength( std::uint64_t x );

/// x as a natural number: 2 x if x >= 0, -2 x + 1 if x < 0
/// \pre |x| < 2^62
[[nodiscard]] constexpr std::uint64_t signedToNatural( std::int64_t x );

/// the inverse of signedToNatural()
[[nodiscard]] constexpr std::int64_t naturalToSigned( std::uint64_t y );

/// the d-gaps of a non-decreasing sequence: the first item, then the
/// differences between consecutive items
/// \pre x is non-decreasing
[[nodiscard]] inline std::vector< std::uint64_t > dGaps(
                                     const std::vector< std::uint64_t > & x );

/// the sequence of the d-gaps g, their prefix sums
[[nodiscard]] inline std::vector< std::uint64_t > prefixSums(
                                     const std::vector< std::uint64_t > & g );

/*------------------------------ UNARY, GAMMA, DELTA -----------------------*/

/// U( x ), x - 1 zeros and a 1
/// \pre x >= 1
/// \post x bits written
inline void encodeUnary( BitWriter & w , std::uint64_t x );
[[nodiscard]] inline std::uint64_t decodeUnary( BitReader & r );
[[nodiscard]] constexpr std::uint64_t unaryLength( std::uint64_t x );

/// gamma( x ), | B( x ) | - 1 zeros and B( x )
/// \pre x >= 1
/// \post gammaLength( x ) bits written
inline void encodeGamma( BitWriter & w , std::uint64_t x );
[[nodiscard]] inline std::uint64_t decodeGamma( BitReader & r );
[[nodiscard]] constexpr int gammaLength( std::uint64_t x );

/// delta( x ), gamma( | B( x ) | ) and B( x )
/// \pre x >= 1
/// \post deltaLength( x ) bits written
inline void encodeDelta( BitWriter & w , std::uint64_t x );
[[nodiscard]] inline std::uint64_t decodeDelta( BitReader & r );
[[nodiscard]] constexpr int deltaLength( std::uint64_t x );

/*------------------------------ RICE, GOLOMB ------------------------------*/

/// R_k( x ), U( q + 1 ) and the remainder r in k bits, x - 1 = q 2^k + r
/// \pre x >= 1 and 0 <= k < 64
/// \post riceLength( x , k ) bits written
inline void encodeRice( BitWriter & w , std::uint64_t x , int k );
[[nodiscard]] inline std::uint64_t decodeRice( BitReader & r , int k );
[[nodiscard]] constexpr std::uint64_t riceLength( std::uint64_t x , int k );

/// Golomb's code of modulus b, U( q + 1 ) and the remainder r in the
/// truncated binary code, x - 1 = q b + r
/// \pre x >= 1 and b >= 1
/// \post golombLength( x , b ) bits written
inline void encodeGolomb( BitWriter & w , std::uint64_t x , std::uint64_t b );
[[nodiscard]] inline std::uint64_t decodeGolomb( BitReader & r ,
                                                 std::uint64_t b );
[[nodiscard]] constexpr std::uint64_t golombLength( std::uint64_t x ,
                                                    std::uint64_t b );

/*-------------------------------- PFORDELTA -------------------------------*/

/// a block of values coded by PForDelta
struct PForDeltaBlock {
 int b = 1;                   ///< the bits of a slot
 std::uint64_t base = 0;      ///< subtracted from the values in range
 BitVector slots;             ///< the n slots of b bits, escapes included
 std::vector< std::uint64_t > exceptions;   ///< the values out of range

 /// the number of values
 [[nodiscard]] std::size_t size( void ) const;

 /// the bits of the code, n b + 64 for each exception
 [[nodiscard]] std::size_t codeBits( void ) const;
 };

/// the smallest b whose range [ base , base + 2^b - 2 ] holds at least the
/// fraction f of the values (all of them if f = 1)
/// \pre 0 < f <= 1
[[nodiscard]] inline int pforDeltaWidth(
                                    const std::vector< std::uint64_t > & x ,
                                    std::uint64_t base , double f = 0.9 );

/// the values x in slots of b bits after subtracting base
/// \pre 1 <= b <= 63
/// \post decodePForDelta() gives x back (an audit)
[[nodiscard]] inline PForDeltaBlock encodePForDelta(
                                   const std::vector< std::uint64_t > & x ,
                                   int b , std::uint64_t base = 0 );
[[nodiscard]] inline std::vector< std::uint64_t > decodePForDelta(
                                                 const PForDeltaBlock & p );

/*---------------------- VARIABLE BYTE, (s,c)-DENSE ------------------------*/

/// the variable-byte code of x appended to out
/// \post vbyteLength( x ) bytes written
inline void encodeVByte( std::vector< std::uint8_t > & out , std::uint64_t x );

/// the integer coded from in[ p ], p moved past it
/// \pre a whole codeword from in[ p ]
[[nodiscard]] inline std::uint64_t decodeVByte(
                                      const std::vector< std::uint8_t > & in ,
                                      std::size_t & p );

/// the bytes of the variable-byte code of x, ceil( | B( x ) | / 7 )
[[nodiscard]] constexpr int vbyteLength( std::uint64_t x );

/// the ( s , c )-dense code of x appended to out, a word per item: the
/// continuers s , ... , s + c - 1 and then a stopper 0 , ... , s - 1
/// \pre s >= 1 and c >= 1
/// \post denseLength( x , s , c ) words written
inline void encodeDense( std::vector< std::uint32_t > & out , std::uint64_t x ,
                         std::uint32_t s , std::uint32_t c );

/// the integer coded from in[ p ], p moved past it
/// \pre a whole codeword from in[ p ]
[[nodiscard]] inline std::uint64_t decodeDense(
                                     const std::vector< std::uint32_t > & in ,
                                     std::size_t & p , std::uint32_t s ,
                                     std::uint32_t c );

/// the words of the ( s , c )-dense code of x
/// \pre s >= 1 and c >= 1
[[nodiscard]] constexpr int denseLength( std::uint64_t x , std::uint32_t s ,
                                         std::uint32_t c );

/// the number s of stoppers, c = 2^bits - s, that minimizes the words of the
/// integers 0 , 1 , ... of frequencies f, by trying each s
/// \pre 1 <= bits <= 16
[[nodiscard]] inline std::uint32_t denseOptimalS(
                       const std::vector< std::uint64_t > & f , int bits = 8 );

/*------------------------------ INTERPOLATIVE -----------------------------*/

/// x - a in ceil( log2( b - a + 1 ) ) bits, the BinaryCode of the notes
/// \pre a <= x <= b
inline void encodeBounded( BitWriter & w , std::uint64_t x ,
                             std::uint64_t a , std::uint64_t b );
[[nodiscard]] inline std::uint64_t decodeBounded( BitReader & r ,
                                                    std::uint64_t a ,
                                                    std::uint64_t b );

/// the interpolative code of x, which low and hi bound
/// \pre x strictly increasing in [ low , hi ]
inline void encodeInterpolative( BitWriter & w ,
                                 const std::vector< std::uint64_t > & x ,
                                 std::uint64_t low , std::uint64_t hi );

/// the n integers coded in [ low , hi ]
[[nodiscard]] inline std::vector< std::uint64_t > decodeInterpolative(
                                       BitReader & r , std::size_t n ,
                                       std::uint64_t low , std::uint64_t hi );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/IntegerCodes.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IntegerCodes.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File IntegerCodes.h ------------------------*/
/*--------------------------------------------------------------------------*/
