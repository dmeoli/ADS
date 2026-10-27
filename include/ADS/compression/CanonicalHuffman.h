/*--------------------------------------------------------------------------*/
/*------------------------ File CanonicalHuffman.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The canonical code of Huffman (chapter 12, Statistical Coding, of
 * Ferragina's book): the lengths \f$ L( \sigma ) \f$ of the codes of Huffman
 * [see `Huffman.h`, whose `huffman()` gives them], and a new code with the
 * same lengths, hence as short, chosen so that neither the encoder nor the
 * decoder needs the tree. With \f$ \mathit{num}[ l ] \f$ the number of the
 * symbols of length \f$ l \f$ and \f$ \max \f$ the longest length, the first
 * code of length \f$ l \f$ is \f$ \mathit{fc}[ \max ] = 0 \f$ and
 * \f$ \mathit{fc}[ l ] = \lceil ( \mathit{fc}[ l + 1 ] + \mathit{num}[ l + 1 ]
 * ) / 2 \rceil \f$, the parent
 of the first unused node of the level below, and
 * the symbols of length \f$ l \f$, in their order, get the consecutive codes
 * \f$ \mathit{fc}[ l ] , \mathit{fc}[ l ] + 1 , \ldots \f$ on \f$ l \f$ bits:
 * the tree is skewed to the left, the longest codes being all zeros at the
 * start. The same chapter divides without rounding up, which is the same on
 * the complete codes that the algorithm of Huffman gives (the sum is even
 * there); rounding up also keeps the decoder right on a code that is not
 * complete, e.g., one cut by the limit on the lengths. The decoder reads one
 * bit at a time, \f$ v \f$ being the value of the \f$ l \f$ bits read, and
 * stops at the first \f$ l \f$ with \f$ v \ge \mathit{fc}[ l ] \f$, since the
 * codes of length \f$ l \f$ are the values from \f$ \mathit{fc}[ l ] \f$ on
 * and every prefix of a longer code is smaller: the symbol is
 * \f$ \mathit{symb}[ l ][ v - \mathit{fc}[ l ] ] \f$. Hence the preamble is
 * the lengths alone, at most \f$ \max^2 \f$ bits for \f$ \mathit{fc} \f$ and
 * \f$ ( |\Sigma| + \max ) \log_2( |\Sigma| + 1 ) \f$ for
 * \f$ \mathit{symb} \f$, instead of the pointers of the tree, and the decoding
 * of a code of \f$ l \f$ bits makes \f$ l \f$ steps on two arrays that fit in
 * the cache, instead of \f$ l \f$ steps along the pointers of the tree, each a
 * possible miss. The decoders of the practice go further, as the literature
 * explains: a table indexed by the next \f$ k \f$ bits gives the length of
 * most codes in one access (Moffat and Turpin) [see `HuffmanCoder.h`], and the
 * canonical code is that of the Huffword of Witten, Moffat and Bell, whose
 * symbols are the words of a text.
 *
 * The average length of the code of Huffman is within one bit of the entropy,
 * \f$ H_0 \le \bar\ell < H_0 + 1 \f$, since its lengths are optimal and those
 * of Shannon, \f$ \lceil \log_2( 1 / P[ \sigma ] ) \rceil \f$, have an average
 * below \f$ H_0 + 1 \f$ [see `Entropy.h`], and its codes can be long: a
 * frequency that grows as the numbers of Fibonacci gives a code of 33 bits
 * after about \f$ 10^7 \f$ symbols (chapter 12, Statistical Coding, of
 * Ferragina's book). `limitedLengths()` bounds them to a given \f$ L \f$ by
 * the iterative scaling of the same chapter: the frequencies are halved until
 * the code of Huffman has no length above \f$ L \f$, which in the limit gives
 * a code of fixed length \f$ \lceil \log_2 |\Sigma| \rceil \f$; the
 * package-merge of Larmore and Hirschberg finds the optimal code of lengths at
 * most \f$ L \f$ in \f$ O( |\Sigma| L ) \f$ time, at the price of a less
 * simple algorithm [see `limitedHuffmanLengths()` of `HuffmanCoder.h`]. The
 * construction takes \f$ O( |\Sigma| \log |\Sigma| ) \f$ time, the encoding of
 * a symbol \f$ O( 1 ) \f$ and its decoding \f$ O( L( \sigma ) ) \f$.
 *
 * The contracts [see `Contract.h`]: the lengths are positive, at most 63,
 * and satisfy the inequality of Kraft,
 * \f$ \sum_\sigma 2^{-L( \sigma )} \le 1 \f$; the frequencies of
 * `limitedLengths()` are positive and \f$ 2^L \ge |\Sigma| \f$; a symbol
 * encoded has a code, and the bits decoded are codes; the decoding of an
 * encoding gives the text back, an audit. The operations are defined in
 * `CanonicalHuffman.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12, Statistical Coding,
 *      doi:10.1017/9781009128933
 * \see D. A. Huffman, A method for the construction of minimum-redundancy
 *      codes, Proceedings of the IRE 40(9), 1098-1101, 1952,
 *      doi:10.1109/JRPROC.1952.273898
 * \see E. S. Schwartz, B. Kallick, Generating a canonical prefix encoding,
 *      Communications of the ACM 7(3), 166-169, 1964,
 *      doi:10.1145/363958.363991
 * \see A. Moffat, A. Turpin, On the implementation of minimum redundancy
 *      prefix codes, IEEE Transactions on Communications 45(10),
 *      1200-1207, 1997, doi:10.1109/26.634683
 * \see L. L. Larmore, D. S. Hirschberg, A fast algorithm for optimal
 *      length-limited Huffman codes, Journal of the ACM 37(3), 464-473,
 *      1990, doi:10.1145/79147.79150
 * \see I. H. Witten, A. Moffat, T. C. Bell, Managing Gigabytes, 2nd ed.,
 *      Morgan Kaufmann, 1999
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

#ifndef __ADS_CanonicalHuffman
 #define __ADS_CanonicalHuffman
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/PackedBits.h"
#include "ADS/string/Huffman.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the lengths of the codes of Huffman [see `Huffman.h`] of the symbols with
/// the frequencies `freq`
///
/// It reads the lengths off the codes of `huffman()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{huffmanLengths}( f ) \cr
/// 2 & \quad \textbf{foreach}\ ( c , s ) \in \mathrm{huffman}( f )
///     \ \textbf{do}\ L[ c ] \gets |s| \cr
/// 3 & \quad \textbf{return}\ L
/// \end{array}}
/// \f]
/// \pre `freq` is not empty, no frequency is negative
template< class C , class F >
[[nodiscard]] std::map< C , int > huffmanLengths(
                                            const std::map< C , F > & freq );

/// the lengths of a code of Huffman of no length above \f$ L \f$, by halving
/// the frequencies (rounding up) until the code of Huffman of the halved
/// ones has no length above \f$ L \f$
///
/// In pseudocode (the iterative scaling of chapter 12, Statistical Coding, of
/// Ferragina's book), which ends since the frequencies become all 1 and the
/// code then has lengths at most \f$ \lceil \log_2 |\Sigma| \rceil \le L \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{limitedLengths}( f , L ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \ell \gets \mathrm{huffmanLengths}( f ) \cr
/// 4 & \qquad \textbf{if}\ \max_c \ell[ c ] \le L\ \textbf{then return}
///     \ \ell \cr
/// 5 & \qquad \textbf{foreach}\ c\ \textbf{do}\ f[ c ] \gets \lceil f[ c ] /
///     2 \rceil
/// \end{array}}
/// \f]
/// \pre `freq` is not empty, the frequencies are positive integers,
///      \f$ 1 \le L \le 63 \f$ and \f$ 2^L \f$ is at least the number of
///      symbols
/// \post no length is above \f$ L \f$
template< class C , class F >
[[nodiscard]] std::map< C , int > limitedLengths(
                                     const std::map< C , F > & freq , int L );

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CanonicalHuffman -------------------------*/
/*--------------------------------------------------------------------------*/

/// the canonical prefix code of the given lengths of the symbols of type `C`
template< class C >
class CanonicalHuffman {

/*--------------------------------------------------------------------------*/
 public:

 using symbol_type = C;   ///< the type of the symbols

 /// the canonical code of the lengths
 ///
 /// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book,
 /// rounding up), the symbols of each length in their order:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{CanonicalHuffman}( L ) \cr
 /// 2 & \quad \max \gets \max_c L[ c ];\ \mathit{symb}[ l ] \gets \text{the }
 ///     \text{symbols of length } l \text{, for each } l \cr
 /// 3 & \quad \mathit{fc}[ \max ] \gets 0 \cr
 /// 4 & \quad \textbf{for}\ l \gets \max - 1\ \textbf{downto}\ 1
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \mathit{fc}[ l ] \gets \lceil ( \mathit{fc}[ l + 1 ] +
 ///     |\mathit{symb}[ l + 1 ]| ) / 2 \rceil \cr
 /// 6 & \quad \textbf{for}\ l \gets 1\ \textbf{to}\ \max\ \textbf{do} \cr
 /// 7 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ |\mathit{symb}[ l ]| -
 ///     1\ \textbf{do}\ \mathit{code}[ \mathit{symb}[ l ][ i ] ] \gets
 ///     \mathit{fc}[ l ] + i
 /// \end{array}}
 /// \f]
 /// \pre `lengths` is not empty, each length in \f$ [ 1 , 63 ] \f$,
 ///      \f$ \sum_c 2^{-L[ c ]} \le 1 \f$
 explicit CanonicalHuffman( const std::map< C , int > & lengths );

 /// the canonical code of the code of Huffman of the frequencies
 /// \pre as `huffmanLengths()`
 template< class F >
 [[nodiscard]] static CanonicalHuffman fromFrequencies(
                                            const std::map< C , F > & freq );

 /// the lengths of the codes
 [[nodiscard]] const std::map< C , int > & lengths( void ) const {
  return( f_len );
  }

 /// the longest length
 [[nodiscard]] int maxLength( void ) const { return( f_max ); }

 /// \f$ \mathit{fc}[ l ] \f$, the value of the first code of length
 /// \f$ l \f$
 /// \pre \f$ 1 \le l \le \f$ `maxLength()`
 [[nodiscard]] std::uint64_t firstCode( int l ) const;

 /// the code of \f$ c \f$, a string of `'0'` and `'1'`
 /// \pre \f$ c \f$ has a code
 [[nodiscard]] std::string code( const C & c ) const;

 /// appends the code of \f$ c \f$ to \f$ w \f$
 /// \pre \f$ c \f$ has a code
 void encode( const C & c , PackedBitWriter & w ) const;

 /// the codes of the symbols of `text`, packed in bytes
 ///
 /// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{encode}( \mathit{text} ) \cr
 /// 2 & \quad \textbf{foreach}\ c \in \mathit{text}\ \textbf{do}
 ///     \ \text{append the } L[ c ] \text{ low bits of }
 ///     \mathit{code}[ c ] \text{ to } w \cr
 /// 3 & \quad \textbf{return}\ w
 /// \end{array}}
 /// \f]
 /// \pre each symbol of `text` has a code
 /// \post `decode( result , text.size() )` is `text` (audit)
 [[nodiscard]] std::vector< std::uint8_t > encode(
                                         std::span< const C > text ) const;

 /// the symbol whose code is next in \f$ r \f$
 ///
 /// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book): the
 /// codes of length \f$ l \f$ are the values from \f$ \mathit{fc}[ l ] \f$
 /// on, and every proper prefix of a longer code is smaller:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{decode}( r ) \cr
 /// 2 & \quad v \gets \mathrm{nextBit}( r );\ l \gets 1 \cr
 /// 3 & \quad \textbf{while}\ v < \mathit{fc}[ l ]\ \textbf{do}
 ///     \ v \gets 2 v + \mathrm{nextBit}( r );\ l \gets l + 1 \cr
 /// 4 & \quad \textbf{return}\ \mathit{symb}[ l ][ v - \mathit{fc}[ l ] ]
 /// \end{array}}
 /// \f]
 /// \pre the next bits of \f$ r \f$ are a code
 [[nodiscard]] C decode( PackedBitReader & r ) const;

 /// the first \f$ n \f$ symbols encoded in `bits`, each by `decode()` above
 /// \pre `bits` begins with \f$ n \f$ codes
 [[nodiscard]] std::vector< C > decode(
         const std::vector< std::uint8_t > & bits , std::size_t n ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::map< C , int > f_len;                ///< the lengths
 std::map< C , std::uint64_t > f_code;     ///< the values of the codes
 std::vector< std::uint64_t > f_fc;   ///< \f$ \mathit{fc}[ 1 .. \max ] \f$
 std::vector< std::vector< C > > f_symb;   ///< the symbols by length
 int f_max = 0;                            ///< the longest length

 };  // end( class CanonicalHuffman )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/CanonicalHuffman.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CanonicalHuffman.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CanonicalHuffman.h -----------------------*/
/*--------------------------------------------------------------------------*/
