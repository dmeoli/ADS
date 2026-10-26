/*--------------------------------------------------------------------------*/
/*---------------------------- File LempelZiv.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressors of Lempel and Ziv (chapter 13 of the notes), whose
 * dictionary is the text itself: they replace a string seen before with a
 * reference to it, and need no model of the source.
 *
 * - LZ77, lz77Parse(): the longest prefix \f$ \alpha \f$ of the text still
 *   to parse that starts in the window of the last w symbols (and may end
 *   past it, overlapping the prefix itself) gives the triple
 *   \f$ \langle d , |\alpha| , c \rangle \f$, d the distance of the copy
 *   and c the symbol after it (none at the end of the text), and the parse
 *   moves \f$ |\alpha| + 1 \f$ symbols; the decoder copies one symbol at a
 *   time, which is right on an overlapping copy too. Among the longest
 *   prefixes the closest is chosen. A search over the window takes
 *   \f$ O( w \ell ) \f$ time for a phrase of length \f$ \ell \f$, hence
 *   \f$ O( n w ) \f$ in the worst case.
 * - LZSS, lzssParse(): the variant of Storer and Szymanski emits either a
 *   copy \f$ \langle d , \ell \rangle \f$ or a literal
 *   \f$ \langle 0 , c \rangle \f$, a copy only if \f$ \ell \f$ reaches a
 *   minimum (1 in the notes, 3 in gzip). lzssParseHashed() finds the copies
 *   as gzip does: the positions are chained by the hash of the 3 symbols
 *   that start there, from the most recent, and the search follows the
 *   chain of the next 3 symbols within the window, up to a number of
 *   candidates (the levels of gzip trade the length of the copies for the
 *   speed with it); with no bound it gives the parse of lzssParse() with
 *   minimum 3. The encoding of the phrases (gzip uses one code of Huffman
 *   for the literals and the lengths and one for the distances) is apart:
 *   the codes of this folder apply to the fields.
 * - LZ78, lz78Parse(): an explicit dictionary of phrases, kept in a trie,
 *   starts with the empty phrase 0; the longest phrase f that prefixes the
 *   rest of the text gives \f$ \langle id( f ) , c \rangle \f$, and fc
 *   becomes a new phrase, so the dictionary is closed under prefixes and
 *   the trie has one symbol per edge; a phrase costs one step of the trie
 *   per symbol. A text that ends inside a phrase f ends with the pair of
 *   the parent of f and its last symbol. When the dictionary has the given
 *   number of phrases it is discarded and restarts empty, one of the
 *   choices of the notes (freezing it and replacing the least recently used
 *   phrase are the others).
 * - LZW, lzwEncode(): the variant of Welch starts with the 256 phrases of
 *   one byte and emits only the identifiers: the phrase f followed by c,
 *   fc not in the dictionary, gives id( f ), adds fc, and the next phrase
 *   starts at c. The decoder adds fc one step late, when it reads the next
 *   phrase f', whose first symbol is c; if f' is the phrase just to add,
 *   then \f$ c = f[ 1 ] \f$ and the phrase is \f$ f f[ 1 ] \f$.
 *
 * All of them are universal: on the text of an ergodic source their bits
 * per symbol tend to its entropy (Ziv and Lempel), and the number of
 * phrases bounds the entropy of order k of every text (Kosaraju and
 * Manzini), but with a term that vanishes slowly. The practice uses LZ77
 * with long windows and fast searches: the suffix trees or arrays of an
 * unbounded window, and the parse that minimizes the bits rather than the
 * phrases (Ferragina, Nitto and Venturini); LZMA, Zstandard and Brotli
 * add context models and codes of the arithmetic family to the fields of
 * the phrases.
 *
 * The contracts [see Contract.h]: the window and the number of phrases are
 * positive, the minimum length of lzssParseHashed() is at least 3, the
 * phrases decoded refer to symbols already decoded; decode( parse( x ) )
 * == x, an audit. The functions are defined in LempelZiv.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 13
 * \see J. Ziv, A. Lempel, A universal algorithm for sequential data
 *      compression, IEEE Transactions on Information Theory 23(3),
 *      337-343, 1977, doi:10.1109/TIT.1977.1055714
 * \see J. Ziv, A. Lempel, Compression of individual sequences via
 *      variable-rate coding, IEEE Transactions on Information Theory 24(5),
 *      530-536, 1978, doi:10.1109/TIT.1978.1055934
 * \see J. A. Storer, T. G. Szymanski, Data compression via textual
 *      substitution, Journal of the ACM 29(4), 928-951, 1982,
 *      doi:10.1145/322344.322346
 * \see T. A. Welch, A technique for high-performance data compression,
 *      IEEE Computer 17(6), 8-19, 1984, doi:10.1109/MC.1984.1659158
 * \see S. R. Kosaraju, G. Manzini, Compression of low entropy strings with
 *      Lempel-Ziv algorithms, SIAM Journal on Computing 29(3), 893-911,
 *      1999, doi:10.1137/S0097539797331105
 * \see P. Ferragina, I. Nitto, R. Venturini, On the bit-complexity of
 *      Lempel-Ziv compression, SIAM Journal on Computing 42(4), 1521-1541,
 *      2013, doi:10.1137/120869511
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

#ifndef __ADS_LempelZiv
 #define __ADS_LempelZiv
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a phrase of LZ77: copy length symbols from distance back, then next
/// (none at the end of the text); distance 0 when length is 0
struct LZ77Phrase {
 std::size_t distance , length;
 std::optional< char > next;
 friend bool operator==( const LZ77Phrase & a ,
                         const LZ77Phrase & b ) = default;
 };

/// a phrase of LZSS: a copy of length symbols from distance back, or the
/// literal when distance is 0
struct LZSSPhrase {
 std::size_t distance , length;
 char literal;
 friend bool operator==( const LZSSPhrase & a ,
                         const LZSSPhrase & b ) = default;
 };

/// a phrase of LZ78: the phrase id followed by next
struct LZ78Phrase {
 std::size_t id;
 char next;
 friend bool operator==( const LZ78Phrase & a ,
                         const LZ78Phrase & b ) = default;
 };

/// the parse of LZ77 of s with a window of w symbols, the closest among the
/// longest copies
/// \pre w > 0
/// \post lz77Decode( result ) == s (an audit)
[[nodiscard]] inline std::vector< LZ77Phrase > lz77Parse(
           std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases of LZ77
/// \pre each copy refers to symbols already decoded
[[nodiscard]] inline std::string lz77Decode(
                                     const std::vector< LZ77Phrase > & P );

/// the parse of LZSS of s with a window of w symbols, a copy for the
/// closest among the longest prefixes of length at least minLength, a
/// literal otherwise, by searching the whole window
/// \pre w > 0, minLength > 0
/// \post lzssDecode( result ) == s (an audit)
[[nodiscard]] inline std::vector< LZSSPhrase > lzssParse(
           std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() ,
           std::size_t minLength = 1 );

/// the parse of LZSS of s by the chains of the hashes of 3 symbols, as in
/// gzip, following at most maxChain candidates
/// \pre w > 0, minLength >= 3, maxChain > 0
/// \post lzssDecode( result ) == s (an audit); with no bound on maxChain,
///       the result of lzssParse( s , w , minLength )
[[nodiscard]] inline std::vector< LZSSPhrase > lzssParseHashed(
           std::string_view s , std::size_t w , std::size_t minLength = 3 ,
           std::size_t maxChain = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases of LZSS
/// \pre each copy refers to symbols already decoded
[[nodiscard]] inline std::string lzssDecode(
                                     const std::vector< LZSSPhrase > & P );

/// the parse of LZ78 of s, the dictionary restarting when it has
/// maxPhrases phrases besides the empty one
/// \pre maxPhrases > 0
/// \post lz78Decode( result , maxPhrases ) == s (an audit)
[[nodiscard]] inline std::vector< LZ78Phrase > lz78Parse(
      std::string_view s ,
      std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases of LZ78
/// \pre maxPhrases > 0, each identifier refers to a phrase
[[nodiscard]] inline std::string lz78Decode(
      const std::vector< LZ78Phrase > & P ,
      std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );

/// the identifiers of the code of LZW of s
/// \post lzwDecode( result ) == s (an audit)
[[nodiscard]] inline std::vector< std::uint32_t > lzwEncode(
                                                         std::string_view s );

/// the text of the code of LZW
/// \pre each identifier is a phrase, or the one being built
[[nodiscard]] inline std::string lzwDecode(
                                      const std::vector< std::uint32_t > & I );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/LempelZiv.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LempelZiv.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LempelZiv.h --------------------------*/
/*--------------------------------------------------------------------------*/
