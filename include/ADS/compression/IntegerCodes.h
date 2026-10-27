/*--------------------------------------------------------------------------*/
/*--------------------------- File IntegerCodes.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The codes of the integers of chapter 11, Integer Coding, of Ferragina's
 * book: self-delimiting binary codewords for a sequence of positive
 * integers, short for the small ones, written one after the other on a
 * stream of bits [see `BitStream.h`] or of bytes. Each code is optimal, by
 * Shannon, for the distribution \f$ P[ x ] = 2^{-| c( x ) |} \f$, and the
 * choice among them is the choice of the distribution of the data (e.g.,
 * the d-gaps of the lists of an inverted index, `dGaps()`). With
 * \f$ B( x ) \f$ the binary code of \f$ x \f$, of
 * \f$ | B( x ) | = 1 + \lfloor \log_2 x \rfloor \f$ bits:
 *
 * - the unary code \f$ U( x ) = 0^{x-1} 1 \f$, \f$ x \f$ bits, optimal for
 *   \f$ 2^{-x} \f$;
 * - Elias' \f$ \gamma( x ) = 0^{| B( x ) | - 1} B( x ) \f$,
 *   \f$ 2 \lfloor \log_2 x \rfloor + 1 \f$ bits, optimal for
 *   \f$ 1 / ( 2 x^2 ) \f$;
 * - Elias' \f$ \delta( x ) = \gamma( | B( x ) | ) B( x ) \f$,
 *   \f$ 1 + 2 \lfloor \log_2 | B( x ) | \rfloor + | B( x ) | \f$ bits,
 *   \f$ 1 + o( 1 ) \f$ times the binary code; the same chapter keeps the
 *   leading 1 of \f$ B( x ) \f$, which Elias drops since the length tells
 *   it, and so do these functions (one bit more than Elias' code);
 * - Rice's \f$ R_k( x ) = U( q + 1 ) B_k( r ) \f$,
 *   \f$ q = \lfloor ( x - 1 ) / 2^k \rfloor \f$ and \f$ r \f$ the remainder
 *   in \f$ k \f$ bits, \f$ q + 1 + k \f$ bits, and Golomb's code of modulus
 *   \f$ b \f$, of which Rice's is the case \f$ b = 2^k \f$, whose remainder
 *   is in the truncated binary code (\f$ \lfloor \log_2 b \rfloor \f$ or
 *   \f$ \lceil \log_2 b \rceil \f$ bits): optimal for the geometric
 *   distribution \f$ ( 1 - p )^{x-1} p \f$ with \f$ b \approx \ln 2 / p \f$
 *   (Gallager and van Voorhis);
 * - PForDelta, a block of values coded in \f$ b \f$ bits each after
 *   subtracting a base, the values out of
 *   \f$ [ \mathit{base} , \mathit{base} + 2^b - 2 ] \f$ being exceptions
 *   marked by the escape \f$ 2^b - 1 \f$ and written apart in 64 bits, with
 *   \f$ b \f$ chosen so that 90\% of the values fit; it is decoded with no
 *   branch per value, the design of Zukowski, Heman, Nes and Boncz, whose
 *   exceptions are linked in place of the escapes, and of the later OptPFD
 *   of Yan, Ding and Suel, which chooses \f$ b \f$ per block;
 * - the variable-byte code, \f$ B( x ) \f$ in groups of 7 bits from the
 *   most significant, each in a byte whose high bit is 1 except in the last,
 *   \f$ 8 \lceil | B( x ) | / 7 \rceil \f$ bits, and the
 *   \f$ ( s , c ) \f$-dense codes of Brisaboa, Farina, Navarro and Parama,
 *   which split the \f$ s + c = 2^w \f$ values of a word of \f$ w \f$ bits
 *   into \f$ s \f$ stoppers and \f$ c \f$ continuers: \f$ s c^{k-1} \f$
 *   integers take \f$ k \f$ words, and `denseOptimalS()` gives the
 *   \f$ s \f$ of the smallest output for given frequencies (the same chapter
 *   recalls that the minimum is unique); the variable-byte code has
 *   \f$ s = c = 128 \f$ but wastes the codes of the leading groups of zeros,
 *   which the dense code with \f$ s = c = 128 \f$ does not;
 * - the interpolative code of Moffat and Stuiver, for a strictly increasing
 *   sequence in \f$ [ \mathit{low} , \mathit{hi} ] \f$: the middle item is
 *   written in
 *   \f$ \lceil \log_2( \mathit{hi} - \mathit{low} - r + l + 1 ) \rceil \f$
 *   bits between the bounds left by the items around it, then the two
 *   halves recursively, no bit at all for a run of consecutive integers; it
 *   adapts to the clusters of the sequence and is not prefix-free, which is
 *   why it can beat Huffman's code, optimal only among the static
 *   prefix-free codes, at the price of a slow, recursive decoding.
 *
 * The Elias-Fano code of the same chapter, which can be searched in place,
 * is in `EliasFano.h`. `signedToNatural()` maps an integer of any sign to a
 * natural number as the same chapter does (\f$ x \f$ to \f$ 2 x \f$,
 * \f$ -x \f$ to \f$ 2 x + 1 \f$), so that the codes of the positive integers
 * apply to it.
 *
 * Each code writes its bits with the most significant first, so that the
 * stream equals the codeword on paper (\f$ \gamma( 9 ) = 0001001 \f$,
 * \f$ R_4( 83 ) = 0000010010 \f$), and has a function of its length, the
 * formula of the same chapter, which the encoder ensures. The decoders read
 * a word at a time [see `BitStream.h`]; the fast decoders of the literature
 * (e.g., the tables of the unary parts, or the SIMD decoding of the
 * variable-byte code by Lemire and Boytsov) are refinements of these.
 *
 * The contracts [see `Contract.h`]: the integers are positive where the
 * code needs it (unary, gamma, delta, Rice, Golomb), the parameters are in
 * range (\f$ 0 \le k < 64 \f$, \f$ b \ge 1 \f$, \f$ s , c \ge 1 \f$), the
 * interpolative code gets a strictly increasing sequence in
 * \f$ [ \mathit{low} , \mathit{hi} ] \f$, and each encoder ensures that it
 * wrote as many bits as its length function says; the decoding of an
 * encoding is the audit of PForDelta. The functions are defined in
 * `IntegerCodes.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 11, Integer Coding, doi:10.1017/9781009128933
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
 *         Università di Pisa \n
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

/// \f$ | B( x ) | \f$, the bits of the binary code of \f$ x \f$,
/// \f$ 1 + \lfloor \log_2 x \rfloor \f$
/// \pre \f$ x \ge 1 \f$
[[nodiscard]] constexpr int binaryLength( std::uint64_t x );

/// \f$ x \f$ as a natural number: \f$ 2 x \f$ if \f$ x \ge 0 \f$,
/// \f$ -2 x + 1 \f$ if \f$ x < 0 \f$
/// \pre \f$ |x| < 2^{62} \f$
[[nodiscard]] constexpr std::uint64_t signedToNatural( std::int64_t x );

/// the inverse of `signedToNatural()`: \f$ y / 2 \f$ if \f$ y \f$ is even,
/// \f$ -( y - 1 ) / 2 \f$ if odd
[[nodiscard]] constexpr std::int64_t naturalToSigned( std::uint64_t y );

/// the d-gaps of a non-decreasing sequence: the first item, then the
/// differences between consecutive items
/// \pre \f$ x \f$ is non-decreasing
[[nodiscard]] inline std::vector< std::uint64_t > dGaps(
                                     const std::vector< std::uint64_t > & x );

/// the sequence of the d-gaps \f$ g \f$, i.e., their prefix sums
[[nodiscard]] inline std::vector< std::uint64_t > prefixSums(
                                     const std::vector< std::uint64_t > & g );

/*------------------------------ UNARY, GAMMA, DELTA -----------------------*/

/// \f$ U( x ) \f$, \f$ x - 1 \f$ zeros and a 1
/// \pre \f$ x \ge 1 \f$
/// \post \f$ x \f$ bits written
inline void encodeUnary( BitWriter & w , std::uint64_t x );

/// the integer \f$ x \ge 1 \f$ whose \f$ U( x ) \f$ is next in \f$ r \f$: one
/// plus the zeros before the next 1
[[nodiscard]] inline std::uint64_t decodeUnary( BitReader & r );

/// the bits of \f$ U( x ) \f$, i.e., \f$ x \f$
[[nodiscard]] constexpr std::uint64_t unaryLength( std::uint64_t x );

/// \f$ \gamma( x ) \f$, \f$ | B( x ) | - 1 \f$ zeros and \f$ B( x ) \f$
///
/// In pseudocode (Elias' gamma code, chapter 11, Integer Coding, of
/// Ferragina's book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeGamma}( w , x ) \cr
/// 2 & \quad L \gets \lfloor \log_2 x \rfloor \cr
/// 3 & \quad \text{write } L \text{ zeros};\ \text{write the } L + 1
///     \text{ bits of } x \text{, the most significant first}
/// \end{array}}
/// \f]
/// \pre \f$ x \ge 1 \f$
/// \post `gammaLength( x )` bits written
inline void encodeGamma( BitWriter & w , std::uint64_t x );

/// the integer whose \f$ \gamma \f$ is next in \f$ r \f$
///
/// In pseudocode (Elias' gamma code, chapter 11, Integer Coding, of
/// Ferragina's book), the zeros giving the length:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decodeGamma}( r ) \cr
/// 2 & \quad L \gets \text{the zeros before the next 1} \cr
/// 3 & \quad \textbf{return}\ 2^L + \text{the next } L \text{ bits, as a}
///     \text{ binary number}
/// \end{array}}
/// \f]
/// \pre the next bits of \f$ r \f$ are a codeword of at most 127 bits
[[nodiscard]] inline std::uint64_t decodeGamma( BitReader & r );

/// the bits of \f$ \gamma( x ) \f$, \f$ 2 \lfloor \log_2 x \rfloor + 1 \f$
[[nodiscard]] constexpr int gammaLength( std::uint64_t x );

/// \f$ \delta( x ) \f$, \f$ \gamma( | B( x ) | ) \f$ and \f$ B( x ) \f$
///
/// In pseudocode (Elias' delta code, chapter 11, Integer Coding, of
/// Ferragina's book, the leading 1 of \f$ B( x ) \f$ kept):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeDelta}( w , x ) \cr
/// 2 & \quad L \gets | B( x ) |;\ \mathrm{encodeGamma}( w , L );
///     \ \text{write the } L \text{ bits of } x
/// \end{array}}
/// \f]
/// \pre \f$ x \ge 1 \f$
/// \post `deltaLength( x )` bits written
inline void encodeDelta( BitWriter & w , std::uint64_t x );

/// the integer whose \f$ \delta \f$ is next in \f$ r \f$
///
/// In pseudocode (Elias' delta code, chapter 11, Integer Coding, of
/// Ferragina's book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decodeDelta}( r ) \cr
/// 2 & \quad L \gets \mathrm{decodeGamma}( r );\ \textbf{return}
///     \ \text{the next } L \text{ bits, as a binary number}
/// \end{array}}
/// \f]
/// \pre the next bits of \f$ r \f$ are a codeword
[[nodiscard]] inline std::uint64_t decodeDelta( BitReader & r );

/// the bits of \f$ \delta( x ) \f$,
/// \f$ 1 + 2 \lfloor \log_2 | B( x ) | \rfloor + | B( x ) | \f$
[[nodiscard]] constexpr int deltaLength( std::uint64_t x );

/*------------------------------ RICE, GOLOMB ------------------------------*/

/// \f$ R_k( x ) \f$, \f$ U( q + 1 ) \f$ and the remainder \f$ r \f$ in
/// \f$ k \f$ bits, \f$ x - 1 = q 2^k + r \f$
///
/// In pseudocode (Rice's code, chapter 11, Integer Coding, of Ferragina's
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeRice}( w , x , k ) \cr
/// 2 & \quad q \gets \lfloor ( x - 1 ) / 2^k \rfloor;\ r \gets ( x - 1 )
///     \bmod 2^k \cr
/// 3 & \quad \mathrm{encodeUnary}( w , q + 1 );\ \text{write the } k
///     \text{ bits of } r
/// \end{array}}
/// \f]
/// \pre \f$ x \ge 1 \f$ and \f$ 0 \le k < 64 \f$
/// \post `riceLength( x , k )` bits written
inline void encodeRice( BitWriter & w , std::uint64_t x , int k );

/// the integer whose \f$ R_k \f$ is next in \f$ r \f$:
/// \f$ q 2^k + r + 1 \f$, \f$ q + 1 \f$ read in unary and \f$ r \f$ in
/// \f$ k \f$ bits
/// \pre \f$ 0 \le k < 64 \f$
[[nodiscard]] inline std::uint64_t decodeRice( BitReader & r , int k );

/// the bits of \f$ R_k( x ) \f$,
/// \f$ \lfloor ( x - 1 ) / 2^k \rfloor + 1 + k \f$
[[nodiscard]] constexpr std::uint64_t riceLength( std::uint64_t x , int k );

/// Golomb's code of modulus \f$ b \f$, \f$ U( q + 1 ) \f$ and the remainder
/// \f$ r \f$ in the truncated binary code, \f$ x - 1 = q b + r \f$
///
/// In pseudocode (Golomb's code, beyond the book), the truncated binary code
/// giving the first \f$ t \f$ remainders one bit less:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeGolomb}( w , x , b ) \cr
/// 2 & \quad q \gets \lfloor ( x - 1 ) / b \rfloor;\ r \gets ( x - 1 )
///     \bmod b;\ \mathrm{encodeUnary}( w , q + 1 ) \cr
/// 3 & \quad \textbf{if}\ b > 1\ \textbf{then} \cr
/// 4 & \qquad c \gets \lceil \log_2 b \rceil;\ t \gets 2^c - b \cr
/// 5 & \qquad \textbf{if}\ r < t\ \textbf{then}\ \text{write } r \text{ in }
///     c - 1 \text{ bits}\ \textbf{else}\ \text{write } r + t \text{ in } c
///     \text{ bits}
/// \end{array}}
/// \f]
/// \pre \f$ x \ge 1 \f$ and \f$ 1 \le b \le 2^{63} \f$
/// \post `golombLength( x , b )` bits written
inline void encodeGolomb( BitWriter & w , std::uint64_t x , std::uint64_t b );

/// the integer whose Golomb's code of modulus \f$ b \f$ is next in \f$ r \f$
///
/// In pseudocode (Golomb's code, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decodeGolomb}( r , b ) \cr
/// 2 & \quad q \gets \mathrm{decodeUnary}( r ) - 1;\ v \gets 0 \cr
/// 3 & \quad \textbf{if}\ b > 1\ \textbf{then} \cr
/// 4 & \qquad c \gets \lceil \log_2 b \rceil;\ t \gets 2^c - b;
///     \ v \gets \text{the next } c - 1 \text{ bits} \cr
/// 5 & \qquad \textbf{if}\ v \ge t\ \textbf{then}\ v \gets 2 v +
///     \text{the next bit} - t \cr
/// 6 & \quad \textbf{return}\ q b + v + 1
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le b \le 2^{63} \f$
[[nodiscard]] inline std::uint64_t decodeGolomb( BitReader & r ,
                                                 std::uint64_t b );

/// the bits of Golomb's code of \f$ x \f$ of modulus \f$ b \f$
[[nodiscard]] constexpr std::uint64_t golombLength( std::uint64_t x ,
                                                    std::uint64_t b );

/*-------------------------------- PFORDELTA -------------------------------*/

/// a block of values coded by PForDelta
struct PForDeltaBlock {
 int b = 1;                   ///< the bits of a slot
 std::uint64_t base = 0;      ///< subtracted from the values in range
 BitVector slots;             ///< the \f$ n \f$ slots of \f$ b \f$ bits
 std::vector< std::uint64_t > exceptions;   ///< the values out of range

 /// the number of values
 [[nodiscard]] std::size_t size( void ) const;

 /// the bits of the code, \f$ n b \f$ and 64 for each exception
 [[nodiscard]] std::size_t codeBits( void ) const;
 };

/// the smallest \f$ b \f$ whose range
/// \f$ [ \mathit{base} , \mathit{base} + 2^b - 2 ] \f$ holds at least the
/// fraction \f$ f \f$ of the values (all of them if \f$ f = 1 \f$), at most
/// 63
/// \pre \f$ 0 < f \le 1 \f$
[[nodiscard]] inline int pforDeltaWidth(
                                    const std::vector< std::uint64_t > & x ,
                                    std::uint64_t base , double f = 0.9 );

/// the values \f$ x \f$ in slots of \f$ b \f$ bits after subtracting
/// `base`
///
/// In pseudocode (PForDelta, chapter 11, Integer Coding, of Ferragina's book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{encodePForDelta}( x , b , \mathit{base} )
///     \cr
/// 2 & \quad e \gets 2^b - 1\quad \triangleright\ \text{the escape} \cr
/// 3 & \quad \textbf{foreach}\ v \in x\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathit{base} \le v < \mathit{base} + e
///     \ \textbf{then}\ \text{append } v - \mathit{base} \text{ in } b
///     \text{ bits to the slots} \cr
/// 5 & \qquad \textbf{else}\ \text{append } e \text{ to the slots, and } v
///     \text{ to the exceptions} \cr
/// 6 & \quad \textbf{return}\ ( b , \mathit{base} , \text{the slots, the }
///     \text{exceptions} )
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le b \le 63 \f$
/// \post `decodePForDelta()` gives \f$ x \f$ back (audit)
[[nodiscard]] inline PForDeltaBlock encodePForDelta(
                                   const std::vector< std::uint64_t > & x ,
                                   int b , std::uint64_t base = 0 );

/// the values of the block \f$ p \f$: each slot plus the base, or the next
/// exception for an escape
[[nodiscard]] inline std::vector< std::uint64_t > decodePForDelta(
                                                 const PForDeltaBlock & p );

/*---------------------- VARIABLE BYTE, (s,c)-DENSE ------------------------*/

/// the variable-byte code of \f$ x \f$ appended to `out`
///
/// In pseudocode (the variable-byte code, chapter 11, Integer Coding, of
/// Ferragina's book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeVByte}( \mathit{out} , x ) \cr
/// 2 & \quad k \gets \max( 1 , \lceil | B( x ) | / 7 \rceil ) \cr
/// 3 & \quad \textbf{for}\ i \gets k - 1\ \textbf{downto}\ 0\ \textbf{do}
///     \ \text{append the byte } \lfloor x / 2^{7 i} \rfloor \bmod 2^7 +
///     ( 2^7 \text{ if } i > 0 )
/// \end{array}}
/// \f]
/// \post `vbyteLength( x )` bytes written
inline void encodeVByte( std::vector< std::uint8_t > & out , std::uint64_t x );

/// the integer coded from \f$ \mathit{in}[ p ] \f$, \f$ p \f$ moved past it:
/// the groups of 7 bits gathered up to the byte below \f$ 2^7 \f$
/// \pre a whole codeword from \f$ \mathit{in}[ p ] \f$
[[nodiscard]] inline std::uint64_t decodeVByte(
                                      const std::vector< std::uint8_t > & in ,
                                      std::size_t & p );

/// the bytes of the variable-byte code of \f$ x \f$,
/// \f$ \lceil | B( x ) | / 7 \rceil \f$, and 1 for \f$ x = 0 \f$
[[nodiscard]] constexpr int vbyteLength( std::uint64_t x );

/// the \f$ ( s , c ) \f$-dense code of \f$ x \f$ appended to `out`, a word
/// per item: the continuers \f$ s , \ldots , s + c - 1 \f$ and then a stopper
/// \f$ 0 , \ldots , s - 1 \f$
///
/// In pseudocode (the dense codes of Brisaboa, Farina, Navarro and Parama,
/// beyond the book), the words found from the last:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeDense}( \mathit{out} , x , s , c )
///     \cr
/// 2 & \quad C \gets ( x \bmod s );\ y \gets \lfloor x / s \rfloor \cr
/// 3 & \quad \textbf{while}\ y > 0\ \textbf{do}\ y \gets y - 1;\ C \gets ( s
///     + y \bmod c ) \cdot C;\ y \gets \lfloor y / c \rfloor \cr
/// 4 & \quad \text{append } C \text{ to } \mathit{out}
/// \end{array}}
/// \f]
/// \pre \f$ s \ge 1 \f$ and \f$ c \ge 1 \f$
/// \post `denseLength( x , s , c )` words written
inline void encodeDense( std::vector< std::uint32_t > & out , std::uint64_t x ,
                         std::uint32_t s , std::uint32_t c );

/// the integer coded from \f$ \mathit{in}[ p ] \f$, \f$ p \f$ moved past it
///
/// In pseudocode (the dense codes, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decodeDense}( \mathit{in} , p , s , c ) \cr
/// 2 & \quad y \gets 0 \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad d \gets \mathit{in}[ p ];\ p \gets p + 1 \cr
/// 5 & \qquad \textbf{if}\ d < s\ \textbf{then return}\ y s + d \cr
/// 6 & \qquad y \gets y c + ( d - s ) + 1
/// \end{array}}
/// \f]
/// \pre a whole codeword from \f$ \mathit{in}[ p ] \f$
[[nodiscard]] inline std::uint64_t decodeDense(
                                     const std::vector< std::uint32_t > & in ,
                                     std::size_t & p , std::uint32_t s ,
                                     std::uint32_t c );

/// the words of the \f$ ( s , c ) \f$-dense code of \f$ x \f$
/// \pre \f$ s \ge 1 \f$ and \f$ c \ge 1 \f$
[[nodiscard]] constexpr int denseLength( std::uint64_t x , std::uint32_t s ,
                                         std::uint32_t c );

/// the number \f$ s \f$ of stoppers, \f$ c = 2^{\mathit{bits}} - s \f$, that
/// minimizes the words of the integers \f$ 0 , 1 , \ldots \f$ of
/// frequencies \f$ f \f$, by trying each \f$ s \f$
///
/// In pseudocode (the dense codes, beyond the book), in
/// \f$ O( 2^{\mathit{bits}} |f| \log |f| ) \f$ time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{denseOptimalS}( f , \mathit{bits} ) \cr
/// 2 & \quad W \gets 2^{\mathit{bits}} \cr
/// 3 & \quad \textbf{return}\ \text{the least } s \in [ 1 , W ) \text{ that}
///     \text{ minimizes } \sum_x f[ x ] \cdot \mathrm{denseLength}( x , s , W
///     - s )
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le \mathit{bits} \le 16 \f$
[[nodiscard]] inline std::uint32_t denseOptimalS(
                       const std::vector< std::uint64_t > & f , int bits = 8 );

/*------------------------------ INTERPOLATIVE -----------------------------*/

/// \f$ x - a \f$ in \f$ \lceil \log_2( b - a + 1 ) \rceil \f$ bits, the
/// BinaryCode of chapter 11, Integer Coding, of Ferragina's book
/// \pre \f$ a \le x \le b \f$
inline void encodeBounded( BitWriter & w , std::uint64_t x ,
                             std::uint64_t a , std::uint64_t b );

/// the integer in \f$ [ a , b ] \f$ coded next in \f$ r \f$ by
/// `encodeBounded()`
/// \pre \f$ a \le b \f$
[[nodiscard]] inline std::uint64_t decodeBounded( BitReader & r ,
                                                    std::uint64_t a ,
                                                    std::uint64_t b );

/// the interpolative code of \f$ x \f$, which \f$ \mathit{low} \f$ and
/// \f$ \mathit{hi} \f$ bound
///
/// In pseudocode (the interpolative code of Moffat and Stuiver, chapter 11,
/// Integer Coding, of Ferragina's book), on \f$ x[ l .. r - 1 ] \f$, starting
/// from \f$ l = 0 \f$ and \f$ r = |x| \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{interpolative}( w , x , l , r ,
///     \mathit{low} , \mathit{hi} ) \cr
/// 2 & \quad \textbf{if}\ l \ge r\ \textbf{then return} \cr
/// 3 & \quad m \gets \lfloor ( l + r - 1 ) / 2 \rfloor \cr
/// 4 & \quad \mathrm{encodeBounded}( w , x[ m ] , \mathit{low} + m - l ,
///     \mathit{hi} - ( r - 1 - m ) ) \cr
/// 5 & \quad \mathrm{interpolative}( w , x , l , m , \mathit{low} , x[ m ]
///     - 1 ) \cr
/// 6 & \quad \mathrm{interpolative}( w , x , m + 1 , r , x[ m ] + 1 ,
///     \mathit{hi} )
/// \end{array}}
/// \f]
/// \pre \f$ x \f$ is strictly increasing in
///      \f$ [ \mathit{low} , \mathit{hi} ] \f$
inline void encodeInterpolative( BitWriter & w ,
                                 const std::vector< std::uint64_t > & x ,
                                 std::uint64_t low , std::uint64_t hi );

/// the \f$ n \f$ integers coded in \f$ [ \mathit{low} , \mathit{hi} ] \f$,
/// by the recursion of `encodeInterpolative()` with `decodeBounded()` in
/// place of `encodeBounded()`
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
