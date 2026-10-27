/*--------------------------------------------------------------------------*/
/*---------------------------- File LempelZiv.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressors of Lempel and Ziv (chapter 13, Dictionary-Based Compressors,
 * of Ferragina's book, beyond the book of Montresor), whose dictionary is the
 * text itself: they replace a string seen before with a reference to it, and
 * need no model of the source.
 *
 * - LZ77, `lz77Parse()`: the longest prefix \f$ \alpha \f$ of the text still
 *   to parse that starts in the window of the last \f$ w \f$ symbols (and
 *   may end past it, overlapping the prefix itself) gives the triple
 *   \f$ \langle d , |\alpha| , c \rangle \f$, with \f$ d \f$ the distance of
 *   the copy and \f$ c \f$ the symbol after it (none at the end of the
 *   text), and the parse moves \f$ |\alpha| + 1 \f$ symbols; the decoder
 *   copies one symbol at a time, which is right on an overlapping copy too.
 *   Among the longest prefixes the closest is chosen. A search over the
 *   window takes \f$ O( w \ell ) \f$ time for a phrase of length
 *   \f$ \ell \f$, hence \f$ O( n w ) \f$ in the worst case;
 * - LZSS, `lzssParse()`: the variant of Storer and Szymanski emits either a
 *   copy \f$ \langle d , \ell \rangle \f$ or a literal
 *   \f$ \langle 0 , c \rangle \f$, a copy only if \f$ \ell \f$ reaches a
 *   minimum (1 in the same chapter, 3 in gzip). `lzssParseHashed()` finds
 *   the copies as gzip does: the positions are chained by the hash of the 3
 *   symbols that start there, from the most recent, and the search follows
 *   the chain of the next 3 symbols within the window, up to a number of
 *   candidates (the levels of gzip trade the length of the copies for the
 *   speed with it); with no bound it gives the parse of `lzssParse()` with
 *   minimum 3. The encoding of the phrases (gzip uses one code of Huffman
 *   for the literals and the lengths and one for the distances) is apart:
 *   the codes of this folder apply to the fields;
 * - LZ78, `lz78Parse()`: an explicit dictionary of phrases, kept in a trie,
 *   starts with the empty phrase 0; the longest phrase \f$ f \f$ that
 *   prefixes the rest of the text gives
 *   \f$ \langle \mathit{id}( f ) , c \rangle \f$, and \f$ f c \f$ becomes a
 *   new phrase, so that the dictionary is closed under prefixes and the trie
 *   has one symbol per edge; a phrase costs one step of the trie per symbol.
 *   A text that ends inside a phrase \f$ f \f$ ends with the pair of the
 *   parent of \f$ f \f$ and its last symbol. When the dictionary has the
 *   given number of phrases it is discarded and restarts empty, one of the
 *   choices of the same chapter (freezing it and replacing the least
 *   recently used phrase are the others);
 * - LZW, `lzwEncode()`: the variant of Welch starts with the 256 phrases of
 *   one byte and emits only the identifiers: the phrase \f$ f \f$ followed
 *   by \f$ c \f$, with \f$ f c \f$ not in the dictionary, gives
 *   \f$ \mathit{id}( f ) \f$ and adds \f$ f c \f$, and the next phrase
 *   starts at \f$ c \f$. The decoder adds \f$ f c \f$ one step late, when it
 *   reads the next phrase \f$ f' \f$, whose first symbol is \f$ c \f$; if
 *   \f$ f' \f$ is the phrase just to add, then \f$ c = f[ 1 ] \f$ and the
 *   phrase is \f$ f f[ 1 ] \f$.
 *
 * All of them are universal: on the text of an ergodic source their bits
 * per symbol tend to its entropy (Ziv and Lempel), and the number of
 * phrases bounds the entropy of order \f$ k \f$ of every text (Kosaraju and
 * Manzini), but with a term that vanishes slowly. The practice uses LZ77
 * with long windows and fast searches: the suffix trees or arrays of an
 * unbounded window, and the parse that minimizes the bits rather than the
 * phrases (Ferragina, Nitto and Venturini); LZMA, Zstandard and Brotli add
 * context models and codes of the arithmetic family to the fields of the
 * phrases.
 *
 * The contracts [see `Contract.h`]: the window and the number of phrases
 * are positive, the minimum length of `lzssParseHashed()` is at least 3, the
 * phrases decoded refer to symbols already decoded; the decoding of each
 * parse gives back its text, an audit. The functions are defined in
 * `LempelZiv.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 13, Dictionary-Based Compressors,
 *      doi:10.1017/9781009128933
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
 *      2000, doi:10.1137/S0097539797331105
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

/// a phrase of LZ77: copy `length` symbols from `distance` back, then
/// `next` (none at the end of the text); `distance` is 0 when `length` is 0
struct LZ77Phrase {
 std::size_t distance ,   ///< how far back the copy starts
             length;      ///< the symbols copied
 std::optional< char > next;   ///< the symbol after the copy, if any
 /// whether the two phrases are equal
 friend bool operator==( const LZ77Phrase & a ,
                         const LZ77Phrase & b ) = default;
 };

/// a phrase of LZSS: a copy of `length` symbols from `distance` back, or
/// the literal when `distance` is 0
struct LZSSPhrase {
 std::size_t distance ,   ///< how far back the copy starts, 0 for a literal
             length;      ///< the symbols copied, 1 for a literal
 char literal;            ///< the symbol of a literal
 /// whether the two phrases are equal
 friend bool operator==( const LZSSPhrase & a ,
                         const LZSSPhrase & b ) = default;
 };

/// a phrase of LZ78: the phrase `id` followed by `next`
struct LZ78Phrase {
 std::size_t id;   ///< the phrase of the dictionary, 0 the empty one
 char next;        ///< the symbol that follows it
 /// whether the two phrases are equal
 friend bool operator==( const LZ78Phrase & a ,
                         const LZ78Phrase & b ) = default;
 };

/// the parse of LZ77 of \f$ s \f$ with a window of \f$ w \f$ symbols, the
/// closest among the longest copies
///
/// In pseudocode (LZ77 of Ziv and Lempel, beyond the book), the copy found
/// by trying each start in the window, from the closest one:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz77Parse}( s , w ) \cr
/// 2 & \quad p \gets 0;\ P \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ p < n\ \textbf{do} \cr
/// 4 & \qquad ( d , \ell ) \gets \mathrm{longestCopy}( s , p , w ) \cr
/// 5 & \qquad c \gets s[ p + \ell ] \text{ if } p + \ell < n \text{, none }
///     \text{otherwise} \cr
/// 6 & \qquad \text{append } \langle d , \ell , c \rangle \text{ to } P;\ p
///     \gets p + \ell + 1 \cr
/// 7 & \quad \textbf{return}\ P \cr
/// 8 & \textbf{function}\ \mathrm{longestCopy}( s , p , w ) \cr
/// 9 & \quad ( d , \ell ) \gets ( 0 , 0 ) \cr
/// 10 & \quad \textbf{for}\ j \gets p - 1\ \textbf{downto}\ \max( 0 , p - w
///     )\ \textbf{do} \cr
/// 11 & \qquad l \gets \text{the largest } l \text{ with } s[ j , j + l ) =
///     s[ p , p + l ) \text{ and } p + l \le n \cr
/// 12 & \qquad \textbf{if}\ l > \ell\ \textbf{then}\ ( d , \ell ) \gets ( p -
///     j , l ) \cr
/// 13 & \quad \textbf{return}\ ( d , \ell )
/// \end{array}}
/// \f]
/// \pre \f$ w > 0 \f$
/// \post `lz77Decode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< LZ77Phrase > lz77Parse(
           std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases \f$ P \f$ of LZ77
///
/// In pseudocode (LZ77 of Ziv and Lempel, beyond the book), one symbol at a
/// time, so that a copy may overlap the symbols it writes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz77Decode}( P ) \cr
/// 2 & \quad s \gets \text{an empty string} \cr
/// 3 & \quad \textbf{foreach}\ \langle d , \ell , c \rangle \in P
///     \ \textbf{do} \cr
/// 4 & \qquad a \gets |s| - d \cr
/// 5 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ \ell - 1\ \textbf{do}\ s
///     \gets s \cdot s[ a + i ] \cr
/// 6 & \qquad \textbf{if}\ c \text{ is not none}\ \textbf{then}\ s \gets s
///     \cdot c \cr
/// 7 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre each copy refers to symbols already decoded
[[nodiscard]] inline std::string lz77Decode(
                                     const std::vector< LZ77Phrase > & P );

/// the parse of LZSS of \f$ s \f$ with a window of \f$ w \f$ symbols, a copy
/// for the closest among the longest prefixes of length at least
/// `minLength`, a literal otherwise, by searching the whole window
///
/// In pseudocode (LZSS of Storer and Szymanski, beyond the book), with the
/// `longestCopy()` of `lz77Parse()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzssParse}( s , w , \mathit{minLength} ) \cr
/// 2 & \quad p \gets 0;\ P \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ p < n\ \textbf{do} \cr
/// 4 & \qquad ( d , \ell ) \gets \mathrm{longestCopy}( s , p , w ) \cr
/// 5 & \qquad \textbf{if}\ \ell \ge \mathit{minLength}\ \textbf{then}
///     \ \text{append } \langle d , \ell \rangle \text{ to } P;\ p \gets p +
///     \ell \cr
/// 6 & \qquad \textbf{else}\ \text{append } \langle 0 , s[ p ] \rangle
///     \text{ to } P;\ p \gets p + 1 \cr
/// 7 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre \f$ w > 0 \f$, `minLength > 0`
/// \post `lzssDecode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< LZSSPhrase > lzssParse(
           std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() ,
           std::size_t minLength = 1 );

/// the parse of LZSS of \f$ s \f$ by the chains of the hashes of 3 symbols,
/// as in gzip, following at most `maxChain` candidates
///
/// In pseudocode (the search of gzip, beyond the book), with
/// \f$ \mathit{head}[ h ] \f$ the last position of hash \f$ h \f$ and
/// \f$ \mathit{prev}[ j ] \f$ the one before \f$ j \f$ with the same hash:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzssParseHashed}( s , w ,
///     \mathit{minLength} , \mathit{maxChain} ) \cr
/// 2 & \quad p \gets 0;\ P \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ p < n\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ i < p \text{ not inserted yet, with } i + 3
///     \le n\ \textbf{do} \cr
/// 5 & \qquad\quad h \gets \mathrm{hash}( s[ i , i + 3 ) );
///     \ \mathit{prev}[ i ] \gets \mathit{head}[ h ];\ \mathit{head}[ h ]
///     \gets i \cr
/// 6 & \qquad ( d , \ell ) \gets ( 0 , 0 ) \cr
/// 7 & \qquad \textbf{if}\ p + 3 \le n\ \textbf{then} \cr
/// 8 & \qquad\quad j \gets \mathit{head}[ \mathrm{hash}( s[ p , p + 3 ) ) ]
///     \cr
/// 9 & \qquad\quad \textbf{while}\ j \ne \textbf{nil}\ \textbf{and}\ p - j
///     \le w\ \textbf{and}\ \text{fewer than } \mathit{maxChain} \text{ }
///     \text{tried}\ \textbf{do} \cr
/// 10 & \qquad\qquad l \gets \text{the largest } l \text{ with } s[ j , j +
///     l ) = s[ p , p + l ) \text{ and } p + l \le n \cr
/// 11 & \qquad\qquad \textbf{if}\ l > \ell\ \textbf{then}\ ( d , \ell )
///     \gets ( p - j , l ) \cr
/// 12 & \qquad\qquad j \gets \mathit{prev}[ j ] \cr
/// 13 & \qquad \textbf{if}\ \ell \ge \mathit{minLength}\ \textbf{then}
///     \ \text{append } \langle d , \ell \rangle \text{ to } P;\ p \gets p +
///     \ell \cr
/// 14 & \qquad \textbf{else}\ \text{append } \langle 0 , s[ p ] \rangle
///     \text{ to } P;\ p \gets p + 1 \cr
/// 15 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre \f$ w > 0 \f$, `minLength >= 3`, `maxChain > 0`
/// \post `lzssDecode( result ) == s` (an audit); with no bound on
///       `maxChain`, the result of `lzssParse( s , w , minLength )`
[[nodiscard]] inline std::vector< LZSSPhrase > lzssParseHashed(
           std::string_view s , std::size_t w , std::size_t minLength = 3 ,
           std::size_t maxChain = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases \f$ P \f$ of LZSS
///
/// In pseudocode (LZSS of Storer and Szymanski, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzssDecode}( P ) \cr
/// 2 & \quad s \gets \text{an empty string} \cr
/// 3 & \quad \textbf{foreach}\ \text{phrase } f \in P\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ f \text{ is a literal } \langle 0 , c \rangle
///     \ \textbf{then}\ s \gets s \cdot c \cr
/// 5 & \qquad \textbf{else}\ \textbf{for}\ i \gets 0\ \textbf{to}\ \ell - 1
///     \ \textbf{do}\ s \gets s \cdot s[ |s| - d ]\quad \triangleright
///     \ f = \langle d , \ell \rangle \cr
/// 6 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre each copy refers to symbols already decoded
[[nodiscard]] inline std::string lzssDecode(
                                     const std::vector< LZSSPhrase > & P );

/// the parse of LZ78 of \f$ s \f$, the dictionary restarting when it has
/// `maxPhrases` phrases besides the empty one
///
/// In pseudocode (LZ78 of Ziv and Lempel, beyond the book), the dictionary
/// a trie whose root 0 is the empty phrase:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz78Parse}( s , \mathit{maxPhrases} ) \cr
/// 2 & \quad T \gets \text{the trie of the root } 0 \text{ alone};\ p \gets
///     0;\ P \gets \text{an empty list} \cr
/// 3 & \quad \textbf{while}\ p < n\ \textbf{do} \cr
/// 4 & \qquad x \gets 0 \cr
/// 5 & \qquad \textbf{while}\ p < n\ \textbf{and}\ x \text{ has a child }
///     y \text{ by } s[ p ]\ \textbf{do}\ x \gets y;\ p \gets p + 1 \cr
/// 6 & \qquad \textbf{if}\ p = n\ \textbf{then} \cr
/// 7 & \qquad\quad \text{append } \langle \text{the parent of } x ,
///     \text{the symbol into } x \rangle \text{ to } P;\ \textbf{return}\ P
///     \cr
/// 8 & \qquad \text{append } \langle x , s[ p ] \rangle \text{ to } P;
///     \ \text{add to } x \text{ a child by } s[ p ];\ p \gets p + 1 \cr
/// 9 & \qquad \textbf{if}\ T \text{ has } \mathit{maxPhrases} \text{ nodes }
///     \text{besides } 0\ \textbf{then}\ T \gets \text{the root alone} \cr
/// 10 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre `maxPhrases > 0`
/// \post `lz78Decode( result , maxPhrases ) == s` (an audit)
[[nodiscard]] inline std::vector< LZ78Phrase > lz78Parse(
      std::string_view s ,
      std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );

/// the text of the phrases \f$ P \f$ of LZ78
///
/// In pseudocode (LZ78 of Ziv and Lempel, beyond the book), the dictionary
/// \f$ D \f$ of the phrases rebuilt as the parse built it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz78Decode}( P , \mathit{maxPhrases} ) \cr
/// 2 & \quad D \gets ( \varepsilon );\ s \gets \text{an empty string} \cr
/// 3 & \quad \textbf{foreach}\ \langle \mathit{id} , c \rangle \in P
///     \ \textbf{do} \cr
/// 4 & \qquad \text{append } D[ \mathit{id} ] \cdot c \text{ to } D;\ s
///     \gets s \cdot D[ \mathit{id} ] \cdot c \cr
/// 5 & \qquad \textbf{if}\ |D| - 1 = \mathit{maxPhrases}\ \textbf{then}\ D
///     \gets ( \varepsilon ) \cr
/// 6 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre `maxPhrases > 0`, each identifier refers to a phrase
[[nodiscard]] inline std::string lz78Decode(
      const std::vector< LZ78Phrase > & P ,
      std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );

/// the identifiers of the code of LZW of \f$ s \f$
///
/// In pseudocode (LZW of Welch, beyond the book), the phrases of one byte
/// numbered by the byte:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzwEncode}( s ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ \text{an empty list}
///     \cr
/// 3 & \quad D \gets \text{the 256 phrases of one byte};\ f \gets s[ 0 ];\ I
///     \gets \text{an empty list} \cr
/// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad c \gets s[ i ] \cr
/// 6 & \qquad \textbf{if}\ f c \in D\ \textbf{then}\ f \gets f c \cr
/// 7 & \qquad \textbf{else}\ \text{append } \mathit{id}( f ) \text{ to } I;
///     \ \text{add } f c \text{ to } D;\ f \gets c \cr
/// 8 & \quad \text{append } \mathit{id}( f ) \text{ to } I;\ \textbf{return}
///     \ I
/// \end{array}}
/// \f]
/// \post `lzwDecode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< std::uint32_t > lzwEncode(
                                                         std::string_view s );

/// the text of the code \f$ I \f$ of LZW
///
/// In pseudocode (LZW of Welch, beyond the book), each phrase added one
/// step late, and an identifier not yet in \f$ D \f$ being the phrase
/// \f$ f f[ 1 ] \f$ about to be added:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzwDecode}( I ) \cr
/// 2 & \quad \textbf{if}\ I \text{ is empty}\ \textbf{then return}
///     \ \text{an empty string} \cr
/// 3 & \quad D \gets \text{the 256 phrases of one byte};\ f \gets D[ I[ 0 ]
///     ];\ s \gets f \cr
/// 4 & \quad \textbf{for}\ k \gets 1\ \textbf{to}\ |I| - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ I[ k ] < |D|\ \textbf{then}\ g \gets D[ I[ k ] ]
///     \ \textbf{else}\ g \gets f \cdot f[ 1 ] \cr
/// 6 & \qquad s \gets s \cdot g;\ \text{add } f \cdot g[ 1 ] \text{ to } D;
///     \ f \gets g \cr
/// 7 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
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
