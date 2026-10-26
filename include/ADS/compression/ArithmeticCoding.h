/*--------------------------------------------------------------------------*/
/*------------------------ File ArithmeticCoding.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The arithmetic code (section 12.2 of the notes), exact and in integers.
 * A text S of n symbols, with the probabilities P[ \f$ \sigma \f$ ] and
 * the cumulative ones \f$ f_\sigma = \sum_{c < \sigma} P[ c ] \f$, is the
 * interval [ l , l + s ) of [ 0 , 1 ) that the symbols narrow in turn,
 * \f$ s_i = s_{i - 1} P[ S[ i ] ] \f$ and
 * \f$ l_i = l_{i - 1} + s_{i - 1} f_{S[ i ]} \f$, so that
 * \f$ s_n = \prod_i P[ S[ i ] ] \f$; the code is the first
 * \f$ \lceil \log_2 ( 2 / s_n ) \rceil \f$ bits of \f$ l_n + s_n / 2 \f$,
 * a dyadic fraction inside the interval (Corollary 12.1), and hence at most
 * \f$ 2 + n H_0 \f$ bits (Theorem 12.3) [see Entropy.h], against the
 * \f$ n + n H_0 \f$ of the code of Huffman. arithmeticInterval(),
 * arithmeticCodeExact() and arithmeticDecodeExact() do it in exact
 * rationals [see Rational.h], in time that grows with the length of the
 * numbers; they are the reference for the integer coder, and they give the
 * numbers of the example of the notes.
 *
 * The integer coder is that of Witten, Neal and Cleary (section 12.2.6,
 * the range coding of the notes): [ 0 , 1 ) is [ 0 , M ), \f$ M = 2^w \f$,
 * a probability is a count over a total T, and the interval [ L , H ) of a
 * symbol of cumulative counts [ c , c' ) is
 * [ L + \f$ \lfloor r c / T \rfloor \f$ , L + \f$ \lfloor r c' / T
 * \rfloor \f$ ), r = H - L, so that the intervals of the symbols tile the
 * current one with no gap. After each symbol the interval is expanded:
 * inside [ 0 , M / 2 ) it emits 0, inside [ M / 2 , M ) it emits 1, inside
 * [ M / 4 , 3 M / 4 ) it emits nothing but counts an underflow, whose bit,
 * the opposite of the next one emitted, follows that bit; so r stays above
 * M / 4, and a total up to M / 4 keeps every interval of a symbol
 * nonempty. At the end, 01 or 10 (and the pending bits) pick a point of
 * the last interval. The decoder keeps the w bits read in v and finds the
 * symbol whose interval holds v, with the same expansions; the bits past
 * the end are zeros. With w = 32 the loss against the exact code is about
 * \f$ 10^{-4} \f$ bits per symbol, as Witten, Neal and Cleary measured; the
 * coders of the practice emit bytes instead of bits (the range coder of
 * Martin), or use the asymmetric numeral systems of Duda, which reach the
 * same compression with a table lookup per symbol.
 *
 * The model can change at each symbol, as long as the decoder changes it
 * in the same way. The adaptive coder starts from P[ \f$ \sigma \f$ ] =
 * 1 / |\f$ \Sigma \f$| and, after a prefix of length l with
 * \f$ l_\sigma \f$ occurrences of \f$ \sigma \f$, takes
 * P[ \f$ \sigma \f$ ] = \f$ ( l_\sigma + 1 ) / ( l + |\Sigma| ) \f$, so
 * nothing is sent in advance; the counts are halved when their total
 * passes \f$ 2^{16} \f$, which keeps it under M / 4 and makes the model
 * follow a text whose statistics change. The cumulative counts are in a
 * tree of Fenwick, where an increment, a prefix sum and the search of the
 * symbol of a count take \f$ O( \log |\Sigma| ) \f$ time; so a symbol costs
 * \f$ O( \log |\Sigma| ) \f$ time and the code \f$ O( n \log |\Sigma| ) \f$.
 *
 * The contracts [see Contract.h]: the probabilities are positive and sum to
 * 1, the counts are positive with a total of at most M / 4,
 * \f$ 2 \le w \le 32 \f$, the alphabet of the adaptive coder has at most
 * \f$ 2^{20} \f$ symbols and a text only symbols of the model;
 * decode( encode( x ) ) == x, an audit. The functions are defined in
 * ArithmeticCoding.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12
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

/// the interval [ l , l + s ) of text, as the pair ( l , s )
/// \pre the probabilities positive with sum 1, each symbol of text in prob
template< class C >
[[nodiscard]] std::pair< Rational , Rational > arithmeticInterval(
                                   std::span< const C > text ,
                                   const std::map< C , Rational > & prob );

/// the arithmetic code of text: the first ceil( log2( 2 / s ) ) bits of
/// l + s / 2, a string of '0' and '1'
/// \pre as arithmeticInterval()
/// \post the code, as a fraction, is in [ l , l + s )
template< class C >
[[nodiscard]] std::string arithmeticCodeExact(
                                   std::span< const C > text ,
                                   const std::map< C , Rational > & prob );

/// the text of n symbols of the code bits
/// \pre the probabilities positive with sum 1, bits a string of '0' and '1'
template< class C >
[[nodiscard]] std::vector< C > arithmeticDecodeExact(
                                   const std::string & bits , std::size_t n ,
                                   const std::map< C , Rational > & prob );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeEncoder ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the integer arithmetic encoder of Witten, Neal and Cleary on w bits
class RangeEncoder {

/*--------------------------------------------------------------------------*/
 public:

 /// \pre 2 <= w <= 32
 explicit RangeEncoder( int w = 32 );

 /// the largest total of the counts, M / 4
 [[nodiscard]] std::uint64_t maxTotal( void ) const { return( f_M / 4 ); }

 /// narrows the interval to the symbol of cumulative counts
 /// [ low , high ) out of total
 /// \pre low < high <= total <= maxTotal()
 void encode( std::uint64_t low , std::uint64_t high , std::uint64_t total );

 /// the bits that end the code, and the code
 [[nodiscard]] std::vector< std::uint8_t > finish( void );

 /// the number of bits emitted
 [[nodiscard]] std::size_t bits( void ) const { return( f_out.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// emits b and the pending bits, the opposite of b
 void emit( bool b );

 std::uint64_t f_M;                ///< 2^w
 std::uint64_t f_low = 0;          ///< the interval [ f_low , f_high )
 std::uint64_t f_high;
 std::size_t f_pending = 0;        ///< the underflows to emit
 PackedBitWriter f_out;

 };  // end( class RangeEncoder )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeDecoder ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the decoder of a RangeEncoder on w bits
class RangeDecoder {

/*--------------------------------------------------------------------------*/
 public:

 /// the decoder of the code bytes, which must outlive it
 /// \pre 2 <= w <= 32
 explicit RangeDecoder( std::span< const std::uint8_t > bytes , int w = 32 );

 /// the largest total of the counts, M / 4
 [[nodiscard]] std::uint64_t maxTotal( void ) const { return( f_M / 4 ); }

 /// the cumulative count of the next symbol: it is the one whose counts
 /// [ low , high ) hold it
 /// \pre 0 < total <= maxTotal()
 /// \post result < total
 [[nodiscard]] std::uint64_t target( std::uint64_t total ) const;

 /// narrows the interval to the symbol of cumulative counts
 /// [ low , high ) out of total, as the encoder did
 /// \pre low < high <= total <= maxTotal()
 void consume( std::uint64_t low , std::uint64_t high ,
               std::uint64_t total );

/*--------------------------------------------------------------------------*/
 private:

 std::uint64_t f_M;                ///< 2^w
 std::uint64_t f_low = 0;          ///< the interval [ f_low , f_high )
 std::uint64_t f_high;
 std::uint64_t f_v = 0;            ///< the w bits read
 PackedBitReader f_in;

 };  // end( class RangeDecoder )

/*--------------------------------------------------------------------------*/
/*---------------------- THE INTEGER ARITHMETIC CODE -----------------------*/
/*--------------------------------------------------------------------------*/

/// the arithmetic code of text with the static model of the counts
/// \pre the counts positive with total at most 2^( w - 2 ), each symbol of
///      text in counts, 2 <= w <= 32
/// \post arithmeticDecode( result , text.size() , counts , w ) == text
///       (an audit)
template< class C >
[[nodiscard]] std::vector< std::uint8_t > arithmeticEncode(
                         std::span< const C > text ,
                         const std::map< C , std::uint64_t > & counts ,
                         int w = 32 );

/// the n symbols of the code with the static model of the counts
/// \pre as arithmeticEncode()
template< class C >
[[nodiscard]] std::vector< C > arithmeticDecode(
                         std::span< const std::uint8_t > code , std::size_t n ,
                         const std::map< C , std::uint64_t > & counts ,
                         int w = 32 );

/// the adaptive arithmetic code of the symbols of x in [ 0 , sigma )
/// \pre 1 <= sigma <= 2^20, each symbol of x below sigma
/// \post adaptiveArithmeticDecode( result , x.size() , sigma ) == x (an
///       audit)
[[nodiscard]] inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                    std::span< const std::uint32_t > x , std::uint32_t sigma );

/// the n symbols of the adaptive arithmetic code in [ 0 , sigma )
/// \pre 1 <= sigma <= 2^20
[[nodiscard]] inline std::vector< std::uint32_t > adaptiveArithmeticDecode(
                    std::span< const std::uint8_t > code , std::size_t n ,
                    std::uint32_t sigma );

/// the adaptive arithmetic code of the bytes of s
/// \post adaptiveArithmeticDecode( result , s.size() ) == s (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                                                         std::string_view s );

/// the n bytes of the adaptive arithmetic code
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
