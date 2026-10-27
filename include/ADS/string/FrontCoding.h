/*--------------------------------------------------------------------------*/
/*-------------------------- File FrontCoding.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The front coding of a sorted dictionary of strings, beyond the book:
 * adjacent strings share a prefix, and `frontCode()` writes the string
 * \f$ s_i \f$ as the pair \f$ ( \ell_i , \hat s_i ) \f$, \f$ \ell_i \f$
 * being the length of the longest common prefix of \f$ s_i \f$ and
 * \f$ s_{i-1} \f$ (0 for the first one) and \f$ \hat s_i \f$ the rest of
 * \f$ s_i \f$; `frontDecode()` copies \f$ \ell_i \f$ characters of the
 * string before and appends \f$ \hat s_i \f$. The length goes in the
 * byte-aligned code of chapter 9, Searching Strings by Prefix, of
 * Ferragina's book [see `encodeLength()`]: the binary of \f$ \ell \f$
 * padded to a whole number of bytes, the first two bits of the padding
 * giving the number of bytes, so that \f$ \ell < 2^{30} \f$ takes 1 to 4
 * bytes instead of the \f$ \ell \f$ characters of the prefix. On the
 * dictionaries of URLs the shared prefixes are up to 70\% of the
 * characters.
 *
 * A string is decoded from the last one stored whole, and with no more of
 * them the decoding of \f$ s_i \f$ may scan the whole dictionary before it;
 * `FrontCodedDictionary` stores some strings whole (the
 * <em>anchors</em>) and decodes a string from the anchor before it, copying
 * only the \f$ \ell \f$ characters that each pair takes from the previous
 * one:
 *
 * - `bucketed()` is the front coding with bucketing \f$ \mathit{FC}_B \f$ of
 *   the same chapter: an anchor every \f$ b \f$ strings, i.e., buckets of
 *   \f$ b \f$ strings coded on their own; a larger \f$ b \f$ compresses
 *   better and scans more (up to \f$ \Theta( b^2 ) \f$ characters for a
 *   bucket like \f$ a , aa , aaa , \ldots \f$);
 * - `localityPreserving()` is the locality-preserving front coding of
 *   Bender, Farach-Colton and Kuszmaul: \f$ s_i \f$ is front coded only if
 *   the anchor before it begins within the last \f$ c | s_i | \f$
 *   characters of the coding, else it is stored whole; a string is then
 *   decoded by reading at most \f$ ( c + 1 ) | s_i | \f$ characters, the
 *   optimal \f$ O( | s_i | / B ) \f$ transfers of blocks, and the coding
 *   takes at most \f$ ( 1 + 2 / ( c - 2 ) ) \, \mathit{FC}( D ) \f$
 *   characters for \f$ c > 2 \f$, \f$ \mathit{FC}( D ) \f$ being those of
 *   the front coding (each string stored whole is paid for by the
 *   \f$ c | s_i | \f$ characters coded since the anchor before it, as
 *   Bender, Farach-Colton and Kuszmaul prove): the time controls the space.
 *
 * Both search by prefix as the two-level scheme of chapter 9, Searching
 * Strings by Prefix, of Ferragina's book: a binary search on the anchors,
 * which are whole, and the scan of the one bucket where the pattern falls,
 * decoded on the fly; the binary search may be replaced by a Patricia trie
 * on the anchors [see `TwoLevelDictionary.h`]. The string B-tree of
 * Ferragina and Grossi and its cache-oblivious version by Bender,
 * Farach-Colton and Kuszmaul, which also works on the locality-preserving
 * coding, bring the search in external memory to
 * \f$ O( ( p + N_{occ} ) / B + \log_B n ) \f$ transfers.
 *
 * The contracts [see `Contract.h`]: \f$ \ell < 2^{30} \f$ for
 * `encodeLength()`, a pair that takes at most the characters of the string
 * before for `frontDecode()`, the dictionary sorted for
 * `FrontCodedDictionary` (an audit), \f$ b \ge 1 \f$ and \f$ c > 0 \f$; the
 * decoding of the coding gives the strings back, an audit, and the
 * locality-preserving coding is within its bounds of space and time, a
 * postcondition for \f$ c > 2 \f$. The functions are defined in
 * `FrontCoding.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9, Searching Strings by Prefix,
 *      doi:10.1017/9781009128933
 * \see I. H. Witten, A. Moffat, T. C. Bell, Managing Gigabytes, 2nd ed.,
 *      Morgan Kaufmann, 1999 (the front coding of a lexicon)
 * \see M. A. Bender, M. Farach-Colton, B. C. Kuszmaul, Cache-oblivious
 *      string B-trees, Proceedings of PODS 2006, 233-242, 2006,
 *      doi:10.1145/1142351.1142385
 * \see P. Ferragina, R. Grossi, The string B-tree: a new data structure
 *      for string search in external memory and its applications, Journal
 *      of the ACM 46(2), 236-280, 1999, doi:10.1145/301970.301973
 * \see D. R. Morrison, PATRICIA - practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
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

#ifndef __ADS_FrontCoding
 #define __ADS_FrontCoding
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/PrefixSearch.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a pair \f$ ( \ell , \hat s ) \f$ of the front coding, the length of the
/// prefix shared with the string before and the rest
using FrontPair = std::pair< std::size_t , std::string >;

/// the front coding of \f$ D \f$, a pair per string
///
/// In pseudocode (the front coding of chapter 9, Searching Strings by
/// Prefix, of Ferragina's book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{frontCode}( D ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad \ell \gets 0 \text{ if } i = 0 \text{, else }
///     \mathrm{commonPrefixLength}( s_{i-1} , s_i ) \cr
/// 4 & \qquad F[ i ] \gets ( \ell , s_i[ \ell .. |s_i| - 1 ] ) \cr
/// 5 & \quad \textbf{return}\ F
/// \end{array}}
/// \f]
/// \post `frontDecode()` of the result is \f$ D \f$ (audit)
[[nodiscard]] inline std::vector< FrontPair > frontCode(
                                       const std::vector< std::string > & D );

/// the strings of the front coding \f$ F \f$
///
/// In pseudocode (the front coding of chapter 9, Searching Strings by
/// Prefix, of Ferragina's book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{frontDecode}( F ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |F| - 1\ \textbf{do} \cr
/// 3 & \qquad ( \ell , \hat s ) \gets F[ i ] \cr
/// 4 & \qquad s_i \gets s_{i-1}[ 0 .. \ell - 1 ] \cdot \hat s\quad
///     \triangleright\ \text{the empty prefix for } i = 0 \cr
/// 5 & \quad \textbf{return}\ ( s_0 , \ldots , s_{|F| - 1} )
/// \end{array}}
/// \f]
/// \pre the first pair has \f$ \ell = 0 \f$, and each \f$ \ell \f$ is at
///      most the length of the string before
[[nodiscard]] inline std::vector< std::string > frontDecode(
                                       const std::vector< FrontPair > & F );

/// appends to `out` the code of \f$ \ell \f$ in 1 to 4 bytes, the first two
/// bits giving the number of bytes minus 1
///
/// In pseudocode (the byte-aligned code of chapter 9, Searching Strings by
/// Prefix, of Ferragina's book, beyond the book), the most significant byte
/// first:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{encodeLength}( \ell , \mathit{out} ) \cr
/// 2 & \quad k \gets \min\{ k \ge 1 : \ell < 2^{8 k - 2} \text{ or } k = 4 \}
///     \cr
/// 3 & \quad v \gets ( k - 1 ) \cdot 2^{8 k - 2} + \ell \cr
/// 4 & \quad \textbf{foreach}\ j = k - 1 , \ldots , 0\ \textbf{do}
///     \ \text{append the byte } \lfloor v / 2^{8 j} \rfloor \bmod 256
///     \text{ to } \mathit{out}
/// \end{array}}
/// \f]
/// \pre \f$ \ell < 2^{30} \f$
/// \post `decodeLength()` reads \f$ \ell \f$ back (audit)
inline void encodeLength( std::uint32_t l , std::string & out );

/// the length coded at `in[ pos ]`, with `pos` moved past its code
///
/// In pseudocode (the byte-aligned code of chapter 9, Searching Strings by
/// Prefix, of Ferragina's book, beyond the book), \f$ x \f$ the first byte:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decodeLength}( \mathit{in} , \mathit{pos} )
///     \cr
/// 2 & \quad x \gets \mathit{in}[ \mathit{pos} ];\ k \gets \lfloor x / 64
///     \rfloor + 1;\ v \gets x \bmod 64 \cr
/// 3 & \quad \textbf{for}\ j \gets 1\ \textbf{to}\ k - 1\ \textbf{do}\ v
///     \gets 256 v + \mathit{in}[ \mathit{pos} + j ] \cr
/// 4 & \quad \mathit{pos} \gets \mathit{pos} + k;\ \textbf{return}\ v
/// \end{array}}
/// \f]
/// \pre a code begins at `pos` and is within `in`
[[nodiscard]] inline std::uint32_t decodeLength( std::string_view in ,
                                                 std::size_t & pos );

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS FrontCodedDictionary ------------------------*/
/*--------------------------------------------------------------------------*/

/// a sorted dictionary of strings in the front coding, with anchors
class FrontCodedDictionary {

/*--------------------------------------------------------------------------*/
 public:

 /// the front coding of \f$ D \f$ with an anchor every \f$ b \f$ strings
 ///
 /// In pseudocode (the front coding with bucketing of chapter 9, Searching
 /// Strings by Prefix, of Ferragina's book, beyond the book), the coding
 /// that `localityPreserving()` shares with another rule for the anchors:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{bucketed}( D , b ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ i \bmod b = 0\ \textbf{then}\ \ell_i \gets 0;
 ///     \ \text{make } i \text{ an anchor} \cr
 /// 4 & \qquad \textbf{else}\ \ell_i \gets \mathrm{commonPrefixLength}(
 ///     s_{i-1} , s_i ) \cr
 /// 5 & \qquad \text{append } s_i[ \ell_i .. |s_i| - 1 ] \text{ to the }
 ///     \text{characters of the coding} \cr
 /// 6 & \quad \textbf{return}\ \text{the coding}
 /// \end{array}}
 /// \f]
 /// \pre \f$ D \f$ is sorted (audit) and \f$ b \ge 1 \f$
 [[nodiscard]] static FrontCodedDictionary bucketed(
                           const std::vector< std::string > & D ,
                           std::size_t b );

 /// the locality-preserving front coding of \f$ D \f$, of parameter
 /// \f$ c \f$
 ///
 /// In pseudocode (Bender, Farach-Colton and Kuszmaul, beyond the book),
 /// \f$ \mathit{chars} \f$ the characters of the coding since the last
 /// anchor began:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{localityPreserving}( D , c ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 3 & \qquad \ell_i \gets 0 \text{ if } i = 0 \text{, else }
 ///     \mathrm{commonPrefixLength}( s_{i-1} , s_i ) \cr
 /// 4 & \qquad \textbf{if}\ i = 0\ \textbf{or}\ \ell_i = 0\ \textbf{or}
 ///     \ \mathit{chars} > c | s_i |\ \textbf{then} \cr
 /// 5 & \qquad\quad \ell_i \gets 0;\ \text{make } i \text{ an anchor};
 ///     \ \mathit{chars} \gets 0 \cr
 /// 6 & \qquad \text{append } s_i[ \ell_i .. |s_i| - 1 ] \text{ to the }
 ///     \text{coding};\ \mathit{chars} \gets \mathit{chars} + |s_i| -
 ///     \ell_i \cr
 /// 7 & \quad \textbf{return}\ \text{the coding}
 /// \end{array}}
 /// \f]
 /// \pre \f$ D \f$ is sorted (audit) and \f$ c > 0 \f$
 /// \post for \f$ c > 2 \f$, `characters()` is at most
 ///       \f$ ( 1 + 2 / ( c - 2 ) ) \, \mathit{FC}( D ) \f$ and
 ///       `decodeCost( i )` at most \f$ ( c + 1 ) | s_i | \f$
 [[nodiscard]] static FrontCodedDictionary localityPreserving(
                           const std::vector< std::string > & D , double c );

 /// the string \f$ i \f$, decoded from the anchor before it
 ///
 /// In pseudocode (the front coding of chapter 9, Searching Strings by
 /// Prefix, of Ferragina's book, beyond the book), \f$ a \f$ the last anchor
 /// at or before \f$ i \f$, found by a binary search:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad a \gets \text{the last anchor} \le i;\ s \gets \hat s_a \cr
 /// 3 & \quad \textbf{for}\ j \gets a + 1\ \textbf{to}\ i\ \textbf{do}\ s
 ///     \gets s[ 0 .. \ell_j - 1 ] \cdot \hat s_j \cr
 /// 4 & \quad \textbf{return}\ s
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ is less than `size()`
 [[nodiscard]] std::string access( std::size_t i ) const;

 /// the characters that `access( i )` reads, those of the rests from the
 /// anchor before \f$ i \f$ to \f$ i \f$
 /// \pre \f$ i \f$ is less than `size()`
 [[nodiscard]] std::size_t decodeCost( std::size_t i ) const;

 /// the number of strings smaller than \f$ Q \f$
 ///
 /// In pseudocode (the two-level search of chapter 9, Searching Strings by
 /// Prefix, of Ferragina's book, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lexicographicPosition}( Q ) \cr
 /// 2 & \quad k \gets \text{the number of anchors smaller than } Q
 ///     \text{, by a binary search} \cr
 /// 3 & \quad \textbf{return}\ \mathrm{rankInBucket}( k , Q ,
 ///     \textbf{false} )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than \f$ P\# \f$
 ///
 /// In pseudocode (the two-level search of chapter 9, Searching Strings by
 /// Prefix, of Ferragina's book, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prefixEnd}( P ) \cr
 /// 2 & \quad k \gets \text{the number of anchors smaller than } P\#
 ///     \text{, by a binary search} \cr
 /// 3 & \quad \textbf{return}\ \mathrm{rankInBucket}( k , P , \textbf{true}
 ///     )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range \f$ [ l , r ) \f$ of the strings prefixed by \f$ P \f$, by
 /// `lexicographicPosition()` and `prefixEnd()`
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the number of strings smaller than \f$ P \f$ (or \f$ P\# \f$ if
 /// `upper`), given that \f$ k \f$ anchors are: the scan of the bucket
 /// \f$ k - 1 \f$
 ///
 /// In pseudocode (the two-level search of chapter 9, Searching Strings by
 /// Prefix, of Ferragina's book, beyond the book), the strings of the bucket
 /// decoded on the fly, \f$ e \f$ the next anchor
 /// (or \f$ n \f$):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rankInBucket}( k , P , \mathit{upper} ) \cr
 /// 2 & \quad \textbf{if}\ k = 0\ \textbf{then return}\ 0 \cr
 /// 3 & \quad i \gets \text{the anchor } k - 1;\ s \gets \hat s_i \cr
 /// 4 & \quad \textbf{while}\ i < e\ \textbf{and}\ s \text{ precedes } P
 ///     \text{ (or } P\# \text{)}\ \textbf{do} \cr
 /// 5 & \qquad i \gets i + 1 \cr
 /// 6 & \qquad \textbf{if}\ i < e\ \textbf{then}\ s \gets s[ 0 .. \ell_i - 1
 ///     ] \cdot \hat s_i \cr
 /// 7 & \quad \textbf{return}\ i
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \f$ is at most `anchors()`, and it is the number of anchors
 ///      that precede the pattern
 [[nodiscard]] std::size_t rankInBucket( std::size_t k , std::string_view P ,
                                         bool upper ) const;

 /// the number \f$ n \f$ of strings
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of anchors, the strings stored whole
 [[nodiscard]] std::size_t anchors( void ) const;

 /// the place in the dictionary of the anchor \f$ k \f$
 /// \pre \f$ k \f$ is less than `anchors()`
 [[nodiscard]] std::size_t anchor( std::size_t k ) const;

 /// the string of the anchor \f$ k \f$
 /// \pre \f$ k \f$ is less than `anchors()`
 [[nodiscard]] std::string_view anchorString( std::size_t k ) const;

 /// the characters of the coding, those of the rests and of the anchors
 [[nodiscard]] std::size_t characters( void ) const;

 /// the bytes of the coding, the characters and the codes of the lengths
 [[nodiscard]] std::size_t bytes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// a constant view of the strings in their order, decoded one after the
 /// other, forward; `*it` is the string decoded in the iterator
 class const_iterator {
  public:
  using reference = const std::string &;   ///< the string decoded
  using value_type = std::string;          ///< a string
  using difference_type = std::ptrdiff_t;  ///< a distance of iterators
  /// a forward iterator for the concepts
  using iterator_concept = std::forward_iterator_tag;
  /// an input iterator for the algorithms of C++17, since `*it` is a
  /// reference into the iterator
  using iterator_category = std::input_iterator_tag;

  /// an iterator on no dictionary
  const_iterator( void ) = default;
  /// the iterator on the string \f$ i \f$ of `*d`, decoded from the
  /// string 0
  const_iterator( const FrontCodedDictionary * d , std::size_t i )
   : f_d( d ) , f_i( i ) { load(); }

  /// the string decoded
  reference operator*( void ) const { return( f_s ); }
  /// the next string, decoded from this one
  const_iterator & operator++( void ) {
   ++f_i;
   load();
   return( *this );
   }
  /// the next string, this one returned
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  /// whether the two iterators are at the same place
  bool operator==( const const_iterator & o ) const {
   return( f_i == o.f_i );
   }

  private:
  /// the string f_i from the one before
  void load( void ) {
   if( f_d && ( f_i < f_d->size() ) ) {
    f_s.resize( f_d->f_l[ f_i ] );
    f_s.append( f_d->rest( f_i ) );
    }
   }

  const FrontCodedDictionary * f_d = nullptr;
  std::size_t f_i = 0;
  std::string f_s;
  };

 using value_type = std::string;                ///< a string
 using size_type = std::size_t;                 ///< a number of strings
 using difference_type = std::ptrdiff_t;        ///< a distance of places
 using reference = const std::string &;         ///< a string decoded
 using const_reference = const std::string &;   ///< a string decoded
 using iterator = const_iterator;               ///< the constant view

 /// the first string
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// past the last string
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
  }
 /// the first string
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// past the last string
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 FrontCodedDictionary( void ) = default;

 /// the coding of D, s_i whole if anchor( i , l , chars , | s_i | ) says
 /// so, l being its prefix shared with s_{i-1} and chars the characters of
 /// the coding since the last anchor
 template< class A >
 static FrontCodedDictionary build( const std::vector< std::string > & D ,
                                    A anchor );

 /// the rest of the string i
 [[nodiscard]] std::string_view rest( std::size_t i ) const;

 /// the anchor at or before i
 [[nodiscard]] std::size_t anchorBefore( std::size_t i ) const;

 /// the number of anchors smaller than P (or P# if upper)
 [[nodiscard]] std::size_t anchorRank( std::string_view P ,
                                       bool upper ) const;

 std::vector< std::size_t > f_l;   ///< the shared prefixes, 0 at anchors
 std::vector< std::size_t > f_off; ///< where each rest begins in f_chars
 std::string f_chars;              ///< the rests, one after the other
 std::vector< std::size_t > f_a;   ///< the places of the anchors
 std::size_t f_bytes = 0;

 };  // end( class FrontCodedDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/FrontCoding.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FrontCoding.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FrontCoding.h -------------------------*/
/*--------------------------------------------------------------------------*/
