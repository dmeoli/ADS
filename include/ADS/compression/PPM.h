/*--------------------------------------------------------------------------*/
/*------------------------------- File PPM.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Prediction by partial matching (chapter 12, Statistical Coding, of
 * Ferragina's book), the model of contexts of Cleary and Witten in the version
 * C of Moffat, on the arithmetic coder [see `ArithmeticCoding.h`]. For each
 * context \f$ \alpha \f$ of length \f$ 0 \le l \le K \f$ the model counts how
 * many times each symbol has followed \f$ \alpha \f$ in the text seen so far.
 * The symbol \f$ S[ i ] \f$ is coded in the longest context
 * \f$ S[ i - l .. i - 1 ] \f$ that has already seen it, starting from
 * \f$ l = \min( K , i ) \f$: each context on the way that does not predict it
 * emits an escape, whose count in version C is the number \f$ q \f$ of the
 * distinct symbols seen in the context, so that a symbol of count
 * \f$ c( \sigma ) \f$ in a context seen \f$ n \f$ times has probability
 * \f$ c( \sigma ) / ( n + q ) \f$ and the escape \f$ q / ( n + q ) \f$; after
 * \f$ K + 1 \f$ escapes, the context of order \f$ -1 \f$ gives the same
 * probability to every symbol. By the principle of exclusion, a context that
 * escapes tells the decoder that the symbol is none of those it predicts, so
 * the shorter contexts leave them out of their counts (the escape keeps its
 * count \f$ q \f$, as in the example of the same chapter), and the context of
 * order \f$ -1 \f$ gives \f$ 1 / ( |\Sigma| - q ) \f$; a context whose symbols
 * are all excluded emits nothing. After each symbol the counts of all its
 * contexts grow by one, and those of a context are halved when their total
 * passes \f$ 2^{16} \f$. The decoder repeats the same walk, reading the
 * escapes instead of writing them.
 *
 * Coding \f$ S[ i ] \f$ takes \f$ O( K ( K + \sigma_K ) \log n ) \f$ time,
 * with \f$ \sigma_K \f$ the largest number of symbols seen in a context, the
 * contexts being in a balanced tree keyed by their strings; the practice keeps
 * them in a trie of the contexts with links to the suffixes (Moffat), where
 * the next context is a step away. The compression approaches the entropy of
 * order \f$ K \f$ [see `Entropy.h`], and that of the text grows with \f$ K \f$
 * up to 4 or 5 symbols, beyond which the escapes cost more than the longer
 * contexts gain, as the same chapter observes. The escape methods of the
 * literature are A, \f$ 1 / ( n + 1 ) \f$, and B, \f$ q / n \f$, of Cleary and
 * Witten; C, here; D, \f$ q / 2 n \f$; P, X and XC of Witten and Bell, from
 * the symbols seen once; the PPMd of Shkarin estimates it from the statistics
 * of similar contexts, and PPM* of Cleary, Teahan and Witten uses contexts of
 * unbounded length.
 *
 * The contracts [see `Contract.h`]: \f$ K \ge 0 \f$,
 * \f$ 1 \le |\Sigma| \le 2^{16} \f$, a symbol in \f$ [ 0 , |\Sigma| ) \f$;
 * the decoding of an encoding gives the text back, an audit. The operations
 * are defined in `PPM.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12, Statistical Coding,
 *      doi:10.1017/9781009128933
 * \see J. G. Cleary, I. H. Witten, Data compression using adaptive coding
 *      and partial string matching, IEEE Transactions on Communications
 *      32(4), 396-402, 1984, doi:10.1109/TCOM.1984.1096090
 * \see A. Moffat, Implementing the PPM data compression scheme, IEEE
 *      Transactions on Communications 38(11), 1917-1921, 1990,
 *      doi:10.1109/26.61469
 * \see I. H. Witten, T. C. Bell, The zero-frequency problem: estimating the
 *      probabilities of novel events in adaptive text compression, IEEE
 *      Transactions on Information Theory 37(4), 1085-1094, 1991,
 *      doi:10.1109/18.87000
 * \see J. G. Cleary, W. J. Teahan, I. H. Witten, Unbounded length contexts
 *      for PPM, Data Compression Conference, 52-61, 1995,
 *      doi:10.1109/DCC.1995.515495
 * \see D. Shkarin, PPM: one step to practicality, Data Compression
 *      Conference, 202-211, 2002, doi:10.1109/DCC.2002.999958
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

#ifndef __ADS_PPM
 #define __ADS_PPM
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/ArithmeticCoding.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS PPMModel -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the model of contexts of PPM, version C with exclusion, of order
/// \f$ K \f$ on the symbols \f$ [ 0 , \sigma ) \f$
class PPMModel {

/*--------------------------------------------------------------------------*/
 public:

 /// a step of the code: the cumulative counts
 /// \f$ [ \mathit{low} , \mathit{high} ) \f$ of the symbol or of the escape,
 /// out of `total`
 struct Step {
  std::uint64_t low ,     ///< the cumulative count before the item
                high ,    ///< the cumulative count after the item
                total;    ///< the total of the counts, with the escape
  /// whether two steps are the same
  friend bool operator==( const Step & a , const Step & b ) = default;
  };

 /// the model of order \f$ K \f$ = `order` on \f$ \sigma \f$ = `sigma`
 /// symbols, with no symbol seen
 /// \pre \f$ K \ge 0 \f$, \f$ 1 \le \sigma \le 2^{16} \f$
 PPMModel( int order , std::uint32_t sigma = 256 );

 /// the order \f$ K \f$
 [[nodiscard]] int order( void ) const { return( f_order ); }

 /// the size \f$ \sigma \f$ of the alphabet
 [[nodiscard]] std::uint32_t sigma( void ) const { return( f_sigma ); }

 /// the counts of the symbols seen after the context `ctx` (the oldest
 /// symbol first), empty if `ctx` was never seen
 [[nodiscard]] std::map< std::uint32_t , std::uint64_t > counts(
                              const std::vector< std::uint32_t > & ctx ) const;

 /// the steps that code \f$ c \f$ after the symbols seen: the escapes, then
 /// \f$ c \f$
 ///
 /// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book, with
 /// the exclusion of the symbols of the longer contexts), \f$ h \f$ the last
 /// \f$ K \f$ symbols seen and \f$ X \f$ the symbols excluded:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{steps}( c ) \cr
 /// 2 & \quad X \gets \emptyset;\ P \gets ( ) \cr
 /// 3 & \quad \textbf{for}\ l \gets |h|\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \alpha \gets \text{the last } l \text{ symbols of } h;
 ///     \ A \gets \text{the symbols seen after } \alpha \text{, not in } X \cr
 /// 5 & \qquad \textbf{if}\ A = \emptyset\ \textbf{then continue} \cr
 /// 6 & \qquad N \gets \sum_{a \in A} \mathit{count}_\alpha( a ) + q_\alpha
 ///     \quad \triangleright\ q_\alpha \text{ the symbols seen after }
 ///     \alpha \cr
 /// 7 & \qquad \textbf{if}\ c \in A\ \textbf{then return}\ P \cdot (
 ///     \text{the range of } c \text{ in } A \text{, out of } N ) \cr
 /// 8 & \qquad P \gets P \cdot ( \text{the range of the escape, out of }
 ///     N );\ X \gets X \cup A \cr
 /// 9 & \quad \textbf{return}\ P \cdot ( \text{the range of } c \text{ among}
 ///     \text{ the symbols not in } X \text{, each of count } 1 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ c < \f$ `sigma()`
 [[nodiscard]] std::vector< Step > steps( std::uint32_t c ) const;

 /// codes \f$ c \f$ after the symbols seen, by the steps of `steps()`
 /// \pre \f$ c < \f$ `sigma()`
 void encode( std::uint32_t c , RangeEncoder & e ) const;

 /// the symbol coded next in \f$ d \f$, after the symbols seen, by the walk
 /// of `steps()` that reads in \f$ d \f$ whether each context escapes
 [[nodiscard]] std::uint32_t decode( RangeDecoder & d ) const;

 /// adds \f$ c \f$ to the symbols seen
 ///
 /// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book), with
 /// the halving of the counts:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{update}( c ) \cr
 /// 2 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ |h|\ \textbf{do} \cr
 /// 3 & \qquad \alpha \gets \text{the last } l \text{ symbols of } h;
 ///     \ \mathit{count}_\alpha( c ) \gets \mathit{count}_\alpha( c ) + 1 \cr
 /// 4 & \qquad \textbf{if}\ \sum_a \mathit{count}_\alpha( a ) > 2^{16}
 ///     \ \textbf{then}\ \text{halve the counts of } \alpha
 ///     \text{, rounding up}
 ///     \cr
 /// 5 & \quad h \gets h \cdot c;\ \textbf{if}\ |h| > K\ \textbf{then}
 ///     \ \text{drop the first symbol of } h
 /// \end{array}}
 /// \f]
 /// \pre \f$ c < \f$ `sigma()`
 void update( std::uint32_t c );

/*--------------------------------------------------------------------------*/
 private:

 using Counts = std::map< std::uint32_t , std::uint64_t >;

 /// the walk from the longest context: at each context that codes, with
 /// sym its symbols not excluded, cum their cumulative counts (one more
 /// item) and total the total with the escape, choose( sym , cum , total )
 /// gives the index of the symbol, sym.size() for the escape; the symbol
 template< class Choose >
 std::uint32_t walk( Choose choose ) const;

 static constexpr std::uint64_t kLimit = std::uint64_t( 1 ) << 16;

 int f_order;                                        ///< K
 std::uint32_t f_sigma;                              ///< the alphabet
 std::map< std::vector< std::uint32_t > , Counts > f_ctx;  ///< the counts
 std::vector< std::uint32_t > f_hist;                ///< the last K seen

 };  // end( class PPMModel )

/*--------------------------------------------------------------------------*/

/// the code of the bytes of \f$ s \f$ by PPM of order \f$ K \f$
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ppmEncode}( s , K ) \cr
/// 2 & \quad M \gets \text{a } \mathrm{PPMModel} \text{ of order } K;
///     \ E \gets \text{a } \mathrm{RangeEncoder} \cr
/// 3 & \quad \textbf{foreach}\ c \in s\ \textbf{do}\ M.\mathrm{encode}( c ,
///     E );\ M.\mathrm{update}( c ) \cr
/// 4 & \quad \textbf{return}\ E.\mathrm{finish}()
/// \end{array}}
/// \f]
/// \pre \f$ K \ge 0 \f$
/// \post `ppmDecode( result , s.size() , K )` is \f$ s \f$ (audit)
[[nodiscard]] inline std::vector< std::uint8_t > ppmEncode(
                                                 std::string_view s , int K );

/// the \f$ n \f$ bytes of the code by PPM of order \f$ K \f$
///
/// In pseudocode (chapter 12, Statistical Coding, of Ferragina's book), the
/// model of the encoder rebuilt symbol by symbol:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{ppmDecode}( y , n , K ) \cr
/// 2 & \quad M \gets \text{a } \mathrm{PPMModel} \text{ of order } K;
///     \ D \gets \text{a } \mathrm{RangeDecoder} \text{ of } y \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do}\ s[ i ]
///     \gets M.\mathrm{decode}( D );\ M.\mathrm{update}( s[ i ] ) \cr
/// 4 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre \f$ K \ge 0 \f$
[[nodiscard]] inline std::string ppmDecode(
             std::span< const std::uint8_t > code , std::size_t n , int K );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/PPM.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PPM.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File PPM.h -----------------------------*/
/*--------------------------------------------------------------------------*/
