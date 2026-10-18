/*--------------------------------------------------------------------------*/
/*------------------------------- File PPM.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Prediction by partial matching (section 12.3 of the notes), the model of
 * contexts of Cleary and Witten in the version C of Moffat, on the
 * arithmetic coder [see ArithmeticCoding.h]. For each context \f$ \alpha
 * \f$ of length \f$ 0 \le l \le K \f$ the model counts how many times each
 * symbol has followed \f$ \alpha \f$ in the text seen so far. The symbol
 * S[ i ] is coded in the longest context S[ i - l , i - 1 ] that has
 * already seen it, starting from l = min( K , i ): each context on the way
 * that does not predict it emits an escape, whose count in version C is the
 * number q of the distinct symbols seen in the context, so that a symbol of
 * count c( \f$ \sigma \f$ ) in a context seen n times has probability
 * \f$ c( \sigma ) / ( n + q ) \f$ and the escape \f$ q / ( n + q ) \f$;
 * after K + 1 escapes, the context of order -1 gives the same probability
 * to every symbol. By the principle of exclusion (section 12.3.2), a
 * context that escapes tells the decoder that the symbol is none of those
 * it predicts, so the shorter contexts leave them out of their counts (the
 * escape keeps its count q, as in the example of the notes), and the
 * context of order -1 gives \f$ 1 / ( |\Sigma| - q ) \f$; a context whose
 * symbols are all excluded emits nothing. After each symbol the counts of
 * all its contexts grow by one, and those of a context are halved when
 * their total passes \f$ 2^{16} \f$. The decoder repeats the same walk,
 * reading the escapes instead of writing them.
 *
 * Coding S[ i ] takes \f$ O( K ( K + \sigma_K ) \log n ) \f$ time, with
 * \f$ \sigma_K \f$ the largest number of symbols seen in a context, the
 * contexts being in a balanced tree keyed by their strings; the practice
 * keeps them in a trie of the contexts with links to the suffixes (Moffat),
 * where the next context is a step away. The compression approaches the
 * entropy of order K [see Entropy.h], and that of the text grows with K up
 * to 4 or 5 symbols, beyond which the escapes cost more than the longer
 * contexts gain (section 12.3.3). The escape methods of the literature are
 * A, \f$ 1 / ( n + 1 ) \f$, and B, \f$ q / n \f$, of Cleary and Witten; C,
 * here; D, \f$ q / 2 n \f$; P, X and XC of Witten and Bell, from the
 * symbols seen once; the PPMd of Shkarin estimates it from the
 * statistics of similar contexts, and PPM* of Cleary, Teahan and Witten
 * uses contexts of unbounded length.
 *
 * The contracts [see Contract.h]: \f$ K \ge 0 \f$,
 * \f$ 1 \le |\Sigma| \le 2^{16} \f$, a symbol in [ 0 , |\Sigma| );
 * ppmDecode( ppmEncode( x ) ) == x, an audit. The operations are defined in
 * PPM.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 12
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
 *         Universita' di Pisa \n
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

/// the model of contexts of PPM, version C with exclusion, of order K on
/// the symbols [ 0 , sigma )
class PPMModel {

/*--------------------------------------------------------------------------*/
 public:

 /// a step of the code: the cumulative counts [ low , high ) of the
 /// symbol or of the escape, out of total
 struct Step {
  std::uint64_t low , high , total;
  friend bool operator==( const Step & a , const Step & b ) = default;
  };

 /// \pre order >= 0, 1 <= sigma <= 2^16
 PPMModel( int order , std::uint32_t sigma = 256 );

 [[nodiscard]] int order( void ) const { return( f_order ); }

 [[nodiscard]] std::uint32_t sigma( void ) const { return( f_sigma ); }

 /// the counts of the symbols seen after the context ctx (the oldest
 /// symbol first), empty if ctx was never seen
 [[nodiscard]] std::map< std::uint32_t , std::uint64_t > counts(
                              const std::vector< std::uint32_t > & ctx ) const;

 /// the steps that code c after the symbols seen: the escapes, then c
 /// \pre c < sigma()
 [[nodiscard]] std::vector< Step > steps( std::uint32_t c ) const;

 /// codes c after the symbols seen
 /// \pre c < sigma()
 void encode( std::uint32_t c , RangeEncoder & e ) const;

 /// the symbol coded next in d, after the symbols seen
 [[nodiscard]] std::uint32_t decode( RangeDecoder & d ) const;

 /// adds c to the symbols seen
 /// \pre c < sigma()
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

/// the code of the bytes of s by PPM of order K
/// \pre K >= 0
/// \post ppmDecode( result , s.size() , K ) == s (an audit)
[[nodiscard]] inline std::vector< std::uint8_t > ppmEncode(
                                                 std::string_view s , int K );

/// the n bytes of the code by PPM of order K
/// \pre K >= 0
[[nodiscard]] inline std::string ppmDecode(
             std::span< const std::uint8_t > code , std::size_t n , int K );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/PPM.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PPM.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File PPM.h -----------------------------*/
/*--------------------------------------------------------------------------*/
