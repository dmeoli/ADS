/*--------------------------------------------------------------------------*/
/*------------------------ File CanonicalHuffman.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The canonical code of Huffman (section 12.1.1 of the notes): the lengths
 * L( \f$ \sigma \f$ ) of the codes of Huffman [see Huffman.h, whose
 * huffman() gives them], and a new code with the same lengths, hence
 * as short, chosen so that neither the encoder nor the decoder needs the
 * tree. With num[ l ] the number of the symbols of length l and max the
 * longest length, the first code of length l is
 * \f$ fc[ \max ] = 0 \f$ and
 * \f$ fc[ l ] = \lceil ( fc[ l + 1 ] + num[ l + 1 ] ) / 2 \rceil \f$, the
 * parent of the first unused node of the level below, and the symbols of
 * length l, in their order, get the consecutive codes
 * \f$ fc[ l ] , fc[ l ] + 1 , \ldots \f$ on l bits: the tree is skewed to
 * the left, the longest codes being all zeros at the start. The notes
 * divide without rounding up, which is the same on the complete codes that
 * the algorithm of Huffman gives (the sum is even there); rounding up also
 * keeps the decoder right on a code that is not complete, e.g., one cut by
 * the limit on the lengths. The decoder reads one bit at a time, v being
 * the value of the l bits read, and stops at the first l with
 * \f$ v \ge fc[ l ] \f$, since the codes of length l are the values from
 * fc[ l ] on and every prefix of a longer code is smaller: the symbol is
 * symb[ l ][ v - fc[ l ] ]. Hence the preamble is the lengths alone, at
 * most \f$ \max^2 \f$ bits for fc and \f$ ( |\Sigma| + \max ) \log_2(
 * |\Sigma| + 1 ) \f$ for symb, instead of the pointers of the tree, and the
 * decoding of a code of l bits makes l steps on two arrays that fit in the
 * cache, instead of l steps along the pointers of the tree, each a possible
 * miss. The decoders of the practice go further, as the literature
 * explains: a table indexed by the next k bits gives the length of most
 * codes in one access (Moffat and Turpin), and the canonical code is that
 * of the Huffword of Witten, Moffat and Bell, whose symbols are the words
 * of a text.
 *
 * The average length of the code of Huffman is within one bit of the
 * entropy, \f$ H_0 \le L_H < H_0 + 1 \f$ (Theorem 12.2) [see Entropy.h],
 * and its codes can be long: a frequency that grows as the numbers of
 * Fibonacci gives a code of 33 bits after about \f$ 10^7 \f$ symbols
 * (section 12.1.2). limitedLengths() bounds them to a given L by the
 * iterative scaling of the notes: the frequencies are halved until the
 * code of Huffman has no length above L, which in the limit gives a code of
 * fixed length \f$ \lceil \log_2 |\Sigma| \rceil \f$; the
 * package-merge of Larmore and Hirschberg finds the optimal code of lengths
 * at most L in \f$ O( |\Sigma| L ) \f$ time, at the price of a less simple
 * algorithm. The construction takes \f$ O( |\Sigma| \log |\Sigma| ) \f$
 * time, the encoding of a symbol O( 1 ) and its decoding
 * \f$ O( L( \sigma ) ) \f$.
 *
 * The contracts [see Contract.h]: the lengths are positive, at most 63, and
 * satisfy the inequality of Kraft, \f$ \sum_\sigma 2^{-L( \sigma )} \le 1
 * \f$; the frequencies of limitedLengths() are positive and \f$ 2^L \ge
 * |\Sigma| \f$; a symbol encoded has a code, and the bits decoded are
 * codes; decode( encode( x ) ) == x, an audit. The operations are defined
 * in CanonicalHuffman.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12
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

/// the lengths of the codes of Huffman [see Huffman.h] of the symbols with
/// the given frequencies
/// \pre freq not empty, no frequency negative
template< class C , class F >
[[nodiscard]] std::map< C , int > huffmanLengths(
                                            const std::map< C , F > & freq );

/// the lengths of a code of Huffman of no length above L, by halving the
/// frequencies (rounding up) until the code of Huffman of the halved ones
/// has no length above L
/// \pre freq not empty, the frequencies integers and positive, 1 <= L <= 63
///      and 2^L >= the number of symbols
/// \post no length above L
template< class C , class F >
[[nodiscard]] std::map< C , int > limitedLengths(
                                     const std::map< C , F > & freq , int L );

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CanonicalHuffman -------------------------*/
/*--------------------------------------------------------------------------*/

/// the canonical prefix code of the given lengths of the symbols of type C
template< class C >
class CanonicalHuffman {

/*--------------------------------------------------------------------------*/
 public:

 using symbol_type = C;

 /// the canonical code of the lengths
 /// \pre lengths not empty, each in [ 1 , 63 ], sum of 2^-l at most 1
 explicit CanonicalHuffman( const std::map< C , int > & lengths );

 /// the canonical code of the code of Huffman of the frequencies
 /// \pre as huffmanLengths()
 template< class F >
 [[nodiscard]] static CanonicalHuffman fromFrequencies(
                                            const std::map< C , F > & freq );

 /// the lengths of the codes
 [[nodiscard]] const std::map< C , int > & lengths( void ) const {
  return( f_len );
  }

 /// the longest length
 [[nodiscard]] int maxLength( void ) const { return( f_max ); }

 /// fc[ l ], the value of the first code of length l
 /// \pre 1 <= l <= maxLength()
 [[nodiscard]] std::uint64_t firstCode( int l ) const;

 /// the code of c, a string of '0' and '1'
 /// \pre c has a code
 [[nodiscard]] std::string code( const C & c ) const;

 /// appends the code of c to w
 /// \pre c has a code
 void encode( const C & c , PackedBitWriter & w ) const;

 /// the codes of the symbols of text, packed in bytes
 /// \pre each symbol of text has a code
 /// \post decode( result , text.size() ) == text (an audit)
 [[nodiscard]] std::vector< std::uint8_t > encode(
                                         std::span< const C > text ) const;

 /// the symbol whose code is next in r
 /// \pre the next bits of r are a code
 [[nodiscard]] C decode( PackedBitReader & r ) const;

 /// the first n symbols encoded in bits
 /// \pre bits begins with n codes
 [[nodiscard]] std::vector< C > decode(
         const std::vector< std::uint8_t > & bits , std::size_t n ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::map< C , int > f_len;                ///< the lengths
 std::map< C , std::uint64_t > f_code;     ///< the values of the codes
 std::vector< std::uint64_t > f_fc;        ///< fc[ 1 .. max ]
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
