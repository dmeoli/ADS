/*--------------------------------------------------------------------------*/
/*------------------------ File ArithmeticCoding.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic code (chapter 12, Statistical Coding, of Ferragina's book),
 * exact and in integers. A text \f$ S \f$ of \f$ n \f$ symbols, with the
 * probabilities \f$ P[ \sigma ] \f$ and the cumulative ones
 * \f$ f_\sigma = \sum_{c < \sigma} P[ c ] \f$, is the interval
 * \f$ [ l , l + s ) \f$ of \f$ [ 0 , 1 ) \f$ that the symbols narrow in turn,
 * \f$ s_i = s_{i - 1} P[ S[ i ] ] \f$ and
 * \f$ l_i = l_{i - 1} + s_{i - 1} f_{S[ i ]} \f$, so that
 * \f$ s_n = \prod_i P[ S[ i ] ] \f$; the code is the first
 * \f$ \lceil \log_2( 2 / s_n ) \rceil \f$ bits of \f$ l_n + s_n / 2 \f$, a
 * dyadic fraction inside the interval, since the truncation loses less than
 * \f$ s_n / 2 \f$, and hence at most \f$ 2 + n H_0 \f$ bits with the empirical
 * probabilities, \f$ \log_2( 1 / s_n ) \f$ being then \f$ n H_0 \f$ [see
 * `Entropy.h`], against the \f$ n + n H_0 \f$ of the code of Huffman.
 * `arithmeticInterval()`, `arithmeticCodeExact()` and
 * `arithmeticDecodeExact()` do it in exact rationals [see `Rational.h`], in
 * time that grows with the length of the numbers; they are the reference for
 * the integer coder, and they give the numbers of the example of the same
 * chapter.
 *
 * The integer coder is that of Witten, Neal and Cleary (the range coding of
 * the same chapter): \f$ [ 0 , 1 ) \f$ is \f$ [ 0 , M ) \f$, \f$ M = 2^w \f$,
 * a probability is a count over a total \f$ T \f$, and the interval
 * \f$ [ L , H ) \f$ of a symbol of cumulative counts \f$ [ c , c' ) \f$ is
 * \f$ [ L + \lfloor r c / T \rfloor , L + \lfloor r c' / T \rfloor ) \f$,
 * \f$ r = H - L \f$, so that the intervals of the symbols tile the current one
 * with no gap. After each symbol the interval is expanded: inside
 * \f$ [ 0 , M / 2 ) \f$ it emits 0, inside \f$ [ M / 2 , M ) \f$ it emits 1,
 * inside \f$ [ M / 4 , 3 M / 4 ) \f$ it emits nothing but counts an underflow,
 * whose bit, the opposite of the next one emitted, follows that bit; so
 * \f$ r \f$ stays above \f$ M / 4 \f$, and a total up to \f$ M / 4 \f$ keeps
 * every interval of a symbol nonempty. At the end, 01 or 10 (and the pending
 * bits) pick a point of the last interval. The decoder keeps the \f$ w \f$
 * bits read in \f$ v \f$ and finds the symbol whose interval holds \f$ v \f$,
 * with the same expansions; the bits past the end are zeros. With
 * \f$ w = 32 \f$ the loss against the exact code is about \f$ 10^{-4} \f$ bits
 * per symbol, as Witten, Neal and Cleary measured; the coders of the practice
 * emit bytes instead of bits (the range coder of Martin), or use the
 * asymmetric numeral systems of Duda [see `Rans.h` and `Tans.h`], which reach
 * the same compression with a table lookup per symbol.
 *
 * The model can change at each symbol, as long as the decoder changes it
 * in the same way. The adaptive coder starts from
 * \f$ P[ \sigma ] = 1 / |\Sigma| \f$ and, after a prefix of length
 * \f$ l \f$ with \f$ l_\sigma \f$ occurrences of \f$ \sigma \f$, takes
 * \f$ P[ \sigma ] = ( l_\sigma + 1 ) / ( l + |\Sigma| ) \f$, so nothing is
 * sent in advance; the counts are halved when their total passes
 * \f$ \max( 2^{16} , 2 |\Sigma| ) \f$, which keeps it under \f$ M / 4 \f$
 * and makes the model follow a text whose statistics change. The
 * cumulative counts are in a tree of Fenwick, where an increment, a prefix
 * sum and the search of the symbol of a count take
 * \f$ O( \log |\Sigma| ) \f$ time; so a symbol costs
 * \f$ O( \log |\Sigma| ) \f$ time and the code \f$ O( n \log |\Sigma| ) \f$.
 *
 * The contracts [see `Contract.h`]: the probabilities are positive and sum
 * to 1, the counts are positive with a total of at most \f$ M / 4 \f$,
 * \f$ 2 \le w \le 32 \f$, the alphabet of the adaptive coder has at most
 * \f$ 2^{20} \f$ symbols and a text only symbols of the model; the decoding
 * of an encoding gives the text back, an audit. The functions are defined
 * in `ArithmeticCoding.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12, Statistical Coding,
 *      doi:10.1017/9781009128933
 * \see I. H. Witten, R. M. Neal, J. G. Cleary, Arithmetic coding for data
 *      compression, Communications of the ACM 30(6), 520-540, 1987,
 *      doi:10.1145/214762.214771
 * \see P. G. Howard, J. S. Vitter, Arithmetic coding for data compression,
 *      Proceedings of the IEEE 82(6), 857-865, 1994, doi:10.1109/5.286189
 * \see G. N. N. Martin, Range encoding: an algorithm for removing
 *      redundancy from a digitised message, Video & Data Recording
 *      Conference, 1979
 * \see P. M. Fenwick, A new data structure for cumulative frequency tables,
 *      Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
 * \see J. Duda, Asymmetric numeral systems: entropy coding combining speed
 *      of Huffman coding with compression rate of arithmetic coding,
 *      arXiv:1311.2540, 2013
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

#ifndef __ADS_ArithmeticCoding
 #define __ADS_ArithmeticCoding
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/PackedBits.h"
#include "ADS/numeric/Rational.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- THE EXACT ARITHMETIC CODE ------------------------*/
/*--------------------------------------------------------------------------*/

/// the interval \f$ [ l , l + s ) \f$ of `text`, as the pair
/// \f$ ( l , s ) \f$
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book),
/// \f$ f_c \f$ the cumulative probability of the symbols before \f$ c \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{arithmeticInterval}( S , P ) \cr
/// 2 & \quad l \gets 0;\ s \gets 1 \cr
/// 3 & \quad \textbf{foreach}\ c \in S\ \textbf{do}\ l \gets l + s f_c;
///     \ s \gets s P[ c ] \cr
/// 4 & \quad \textbf{return}\ ( l , s )
/// \end{array}}
/// \f]
/// \pre the probabilities are positive with sum 1, and each symbol of
///      `text` is in `prob`
template< class C >
[[nodiscard]] std::pair< Rational , Rational > arithmeticInterval(
                                   std::span< const C > text ,
                                   const std::map< C , Rational > & prob );

/// the arithmetic code of `text`: the first
/// \f$ \lceil \log_2( 2 / s ) \rceil \f$ bits of \f$ l + s / 2 \f$, a string
/// of `'0'` and `'1'`
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book), the
/// bits of the fraction by doubling: the point \f$ x \f$ truncated to
/// \f$ d \f$ bits loses less than \f$ 2^{-d} \le s / 2 \f$, so it stays in the
/// interval:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{arithmeticCodeExact}( S , P ) \cr
/// 2 & \quad ( l , s ) \gets \mathrm{arithmeticInterval}( S , P );
///     \ d \gets \lceil \log_2( 2 / s ) \rceil;\ x \gets l + s / 2 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ d\ \textbf{do} \cr
/// 4 & \qquad x \gets 2 x \cr
/// 5 & \qquad \textbf{if}\ x < 1\ \textbf{then}\ b_i \gets 0
///     \ \textbf{else}\ b_i \gets 1;\ x \gets x - 1 \cr
/// 6 & \quad \textbf{return}\ b_1 \cdots b_d
/// \end{array}}
/// \f]
/// \pre as `arithmeticInterval()`
/// \post the code, as a fraction, is in \f$ [ l , l + s ) \f$
template< class C >
[[nodiscard]] std::string arithmeticCodeExact(
                                   std::span< const C > text ,
                                   const std::map< C , Rational > & prob );

/// the text of \f$ n \f$ symbols of the code `bits`
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book),
/// \f$ x \f$ the code as a fraction:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{arithmeticDecodeExact}( x , n , P ) \cr
/// 2 & \quad l \gets 0;\ s \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad c \gets \text{the symbol with } l + s f_c \le x < l + s (
///     f_c + P[ c ] ) \cr
/// 5 & \qquad S[ i ] \gets c;\ l \gets l + s f_c;\ s \gets s P[ c ] \cr
/// 6 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre the probabilities are positive with sum 1, `bits` is a string of
///      `'0'` and `'1'`
template< class C >
[[nodiscard]] std::vector< C > arithmeticDecodeExact(
                                   const std::string & bits , std::size_t n ,
                                   const std::map< C , Rational > & prob );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeEncoder ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the integer arithmetic encoder of Witten, Neal and Cleary on \f$ w \f$
/// bits
class RangeEncoder {

/*--------------------------------------------------------------------------*/
 public:

 /// the encoder on \f$ w \f$ bits, with the interval \f$ [ 0 , M ) \f$,
 /// \f$ M = 2^w \f$
 /// \pre \f$ 2 \le w \le 32 \f$
 explicit RangeEncoder( int w = 32 );

 /// the largest total of the counts, \f$ M / 4 \f$
 [[nodiscard]] std::uint64_t maxTotal( void ) const { return( f_M / 4 ); }

 /// narrows the interval to the symbol of cumulative counts
 /// \f$ [ \mathit{low} , \mathit{high} ) \f$ out of `total`
 ///
 /// In pseudocode (Witten, Neal and Cleary, beyond the book), \f$ [ L , H )
 /// \f$ the interval and \f$ p \f$ the underflows pending:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{encode}( a , b , T ) \cr
 /// 2 & \quad r \gets H - L;\ H \gets L + \lfloor r b / T \rfloor;
 ///     \ L \gets L + \lfloor r a / T \rfloor \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ H \le M / 2\ \textbf{then}\ \mathrm{emit}( 0 )
 ///     \quad \triangleright\ \text{then } 1^p \text{, and } p \gets 0 \cr
 /// 5 & \qquad \textbf{else if}\ L \ge M / 2\ \textbf{then}
 ///     \ \mathrm{emit}( 1 );\ L \gets L - M / 2;\ H \gets H - M / 2 \cr
 /// 6 & \qquad \textbf{else if}\ L \ge M / 4\ \textbf{and}\ H \le 3 M / 4
 ///     \ \textbf{then}\ p \gets p + 1;\ L \gets L - M / 4;\ H \gets H - M / 4
 ///     \cr
 /// 7 & \qquad \textbf{else return} \cr
 /// 8 & \qquad L \gets 2 L;\ H \gets 2 H
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{low} < \mathit{high} \le \mathit{total} \le \f$
 ///      `maxTotal()`
 /// \invariant \f$ H - L > M / 4 \f$ after the expansions
 void encode( std::uint64_t low , std::uint64_t high , std::uint64_t total );

 /// the bits that end the code, and the code
 ///
 /// It emits two bits and the pending ones, 01 if \f$ L < M / 4 \f$ and 10
 /// otherwise, which pick a point of \f$ [ L , H ) \f$ whatever follows.
 [[nodiscard]] std::vector< std::uint8_t > finish( void );

 /// the number of bits emitted
 [[nodiscard]] std::size_t bits( void ) const { return( f_out.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// emits \f$ b \f$ and the pending bits, the opposite of \f$ b \f$
 void emit( bool b );

 std::uint64_t f_M;                ///< \f$ M = 2^w \f$
 std::uint64_t f_low = 0;          ///< \f$ L \f$, the low end of the interval
 std::uint64_t f_high;             ///< its \f$ H \f$
 std::size_t f_pending = 0;        ///< the underflows to emit
 PackedBitWriter f_out;            ///< the bits emitted

 };  // end( class RangeEncoder )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeDecoder ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the decoder of a `RangeEncoder` on \f$ w \f$ bits
class RangeDecoder {

/*--------------------------------------------------------------------------*/
 public:

 /// the decoder of the code `bytes`, which must outlive it
 /// \pre \f$ 2 \le w \le 32 \f$
 explicit RangeDecoder( std::span< const std::uint8_t > bytes , int w = 32 );

 /// the largest total of the counts, \f$ M / 4 \f$
 [[nodiscard]] std::uint64_t maxTotal( void ) const { return( f_M / 4 ); }

 /// the cumulative count of the next symbol: it is the one whose counts
 /// \f$ [ \mathit{low} , \mathit{high} ) \f$ hold it
 ///
 /// It inverts the narrowing of `RangeEncoder::encode()`, \f$ v \f$ the
 /// \f$ w \f$ bits read:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{target}( T ) \cr
 /// 2 & \quad \textbf{return}\ \lfloor ( ( v - L + 1 ) T - 1 ) / ( H - L )
 ///     \rfloor
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 < \mathit{total} \le \f$ `maxTotal()`
 /// \post the result is below `total`
 [[nodiscard]] std::uint64_t target( std::uint64_t total ) const;

 /// narrows the interval to the symbol of cumulative counts
 /// \f$ [ \mathit{low} , \mathit{high} ) \f$ out of `total`, as the encoder
 /// did
 ///
 /// In pseudocode (Witten, Neal and Cleary, beyond the book), the
 /// expansions of `RangeEncoder::encode()` applied also to \f$ v \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{consume}( a , b , T ) \cr
 /// 2 & \quad r \gets H - L;\ H \gets L + \lfloor r b / T \rfloor;
 ///     \ L \gets L + \lfloor r a / T \rfloor \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ H \le M / 2\ \textbf{then}\ \text{nothing} \cr
 /// 5 & \qquad \textbf{else if}\ L \ge M / 2\ \textbf{then}\ \text{subtract }
 ///     M / 2 \text{ from } L , H , v \cr
 /// 6 & \qquad \textbf{else if}\ L \ge M / 4\ \textbf{and}\ H \le 3 M / 4
 ///     \ \textbf{then}\ \text{subtract } M / 4 \text{ from } L , H , v \cr
 /// 7 & \qquad \textbf{else return} \cr
 /// 8 & \qquad L \gets 2 L;\ H \gets 2 H;\ v \gets 2 v +
 ///     \mathrm{nextBit}()
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{low} < \mathit{high} \le \mathit{total} \le \f$
 ///      `maxTotal()`
 /// \invariant \f$ L \le v < H \f$
 void consume( std::uint64_t low , std::uint64_t high ,
               std::uint64_t total );

/*--------------------------------------------------------------------------*/
 private:

 std::uint64_t f_M;                ///< \f$ M = 2^w \f$
 std::uint64_t f_low = 0;          ///< \f$ L \f$, the low end of the interval
 std::uint64_t f_high;             ///< its \f$ H \f$
 std::uint64_t f_v = 0;            ///< \f$ v \f$, the \f$ w \f$ bits read
 PackedBitReader f_in;             ///< the bits of the code

 };  // end( class RangeDecoder )

/*--------------------------------------------------------------------------*/
/*---------------------- THE INTEGER ARITHMETIC CODE -----------------------*/
/*--------------------------------------------------------------------------*/

/// the arithmetic code of `text` with the static model of the counts
///
/// In pseudocode (Witten, Neal and Cleary, beyond the book), \f$ c_\sigma \f$
/// the cumulative counts of the symbols before \f$ \sigma \f$ and \f$ T \f$
/// their total:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{arithmeticEncode}( S , \mathit{counts} ,
///     w ) \cr
/// 2 & \quad E \gets \text{a } \mathrm{RangeEncoder} \text{ on } w
///     \text{ bits} \cr
/// 3 & \quad \textbf{foreach}\ \sigma \in S\ \textbf{do}
///     \ E.\mathrm{encode}( c_\sigma , c_\sigma + \mathit{counts}[ \sigma ] ,
///     T ) \cr
/// 4 & \quad \textbf{return}\ E.\mathrm{finish}()
/// \end{array}}
/// \f]
/// \pre the counts are positive with total at most \f$ 2^{w - 2} \f$, each
///      symbol of `text` is in `counts`, \f$ 2 \le w \le 32 \f$
/// \post `arithmeticDecode( result , text.size() , counts , w )` is `text`
///       (audit)
template< class C >
[[nodiscard]] std::vector< std::uint8_t > arithmeticEncode(
                         std::span< const C > text ,
                         const std::map< C , std::uint64_t > & counts ,
                         int w = 32 );

/// the \f$ n \f$ symbols of the code with the static model of the counts
///
/// In pseudocode (Witten, Neal and Cleary, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{arithmeticDecode}( y , n , \mathit{counts}
///     , w ) \cr
/// 2 & \quad D \gets \text{a } \mathrm{RangeDecoder} \text{ of } y \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad t \gets D.\mathrm{target}( T );\ \sigma \gets \text{the symbol}
///     \text{ with } c_\sigma \le t < c_\sigma + \mathit{counts}[ \sigma ] \cr
/// 5 & \qquad D.\mathrm{consume}( c_\sigma , c_\sigma + \mathit{counts}[
///     \sigma ] , T );\ S[ i ] \gets \sigma \cr
/// 6 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre as `arithmeticEncode()`
template< class C >
[[nodiscard]] std::vector< C > arithmeticDecode(
                         std::span< const std::uint8_t > code , std::size_t n ,
                         const std::map< C , std::uint64_t > & counts ,
                         int w = 32 );

/// the adaptive arithmetic code of the symbols of \f$ x \f$ in
/// \f$ [ 0 , \sigma ) \f$
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book, with
/// the halving of the counts), \f$ F \f$ a tree of Fenwick of the counts, all
/// 1 at the start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{adaptiveArithmeticEncode}( x , \sigma )
///     \cr
/// 2 & \quad E \gets \text{a } \mathrm{RangeEncoder};\ F \gets ( 1 , \ldots
///     , 1 ) \cr
/// 3 & \quad \textbf{foreach}\ c \in x\ \textbf{do} \cr
/// 4 & \qquad a \gets F.\mathrm{prefix}( c );\ E.\mathrm{encode}( a , a +
///     F.\mathrm{count}( c ) , F.\mathrm{total}() ) \cr
/// 5 & \qquad F.\mathrm{add}( c , 1 ) \cr
/// 6 & \qquad \textbf{if}\ F.\mathrm{total}() > \max( 2^{16} , 2 \sigma )
///     \ \textbf{then}\ \text{halve the counts, rounding up} \cr
/// 7 & \quad \textbf{return}\ E.\mathrm{finish}()
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le \sigma \le 2^{20} \f$, each symbol of \f$ x \f$ below
///      \f$ \sigma \f$
/// \post `adaptiveArithmeticDecode( result , x.size() , sigma )` is \f$ x \f$
///       (audit)
[[nodiscard]] inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                    std::span< const std::uint32_t > x , std::uint32_t sigma );

/// the \f$ n \f$ symbols of the adaptive arithmetic code in
/// \f$ [ 0 , \sigma ) \f$
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book), the
/// model of the encoder rebuilt symbol by symbol:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{adaptiveArithmeticDecode}( y , n , \sigma
///     ) \cr
/// 2 & \quad D \gets \text{a } \mathrm{RangeDecoder} \text{ of } y;\ F \gets
///     ( 1 , \ldots , 1 ) \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad c \gets F.\mathrm{find}( D.\mathrm{target}( F.\mathrm{total}()
///     ) );\ a \gets F.\mathrm{prefix}( c ) \cr
/// 5 & \qquad D.\mathrm{consume}( a , a + F.\mathrm{count}( c ) ,
///     F.\mathrm{total}() );\ x[ i ] \gets c \cr
/// 6 & \qquad F.\mathrm{add}( c , 1 );\ \textbf{if}\ F.\mathrm{total}() >
///     \max( 2^{16} , 2 \sigma )\ \textbf{then}\ \text{halve the counts} \cr
/// 7 & \quad \textbf{return}\ x
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le \sigma \le 2^{20} \f$
[[nodiscard]] inline std::vector< std::uint32_t > adaptiveArithmeticDecode(
                    std::span< const std::uint8_t > code , std::size_t n ,
                    std::uint32_t sigma );

/// the adaptive arithmetic code of the bytes of \f$ s \f$, those of
/// `adaptiveArithmeticEncode()` above with \f$ \sigma = 256 \f$
/// \post `adaptiveArithmeticDecode( result , s.size() )` is \f$ s \f$
///       (audit)
[[nodiscard]] inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                                                         std::string_view s );

/// the \f$ n \f$ bytes of the adaptive arithmetic code, by
/// `adaptiveArithmeticDecode()` above with \f$ \sigma = 256 \f$
[[nodiscard]] inline std::string adaptiveArithmeticDecode(
                    std::span< const std::uint8_t > code , std::size_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/ArithmeticCoding.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArithmeticCoding.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ArithmeticCoding.h -----------------------*/
/*--------------------------------------------------------------------------*/
