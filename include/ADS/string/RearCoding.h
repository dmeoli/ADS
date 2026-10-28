/*--------------------------------------------------------------------------*/
/*--------------------------- File RearCoding.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The rear coding of a sorted dictionary of strings, beyond the book, and a
 * compact dictionary on it (see \ref theory_string_dict): the front coding
 * [see `FrontCoding.h`] writes the string \f$ s_i \f$ as the length
 * \f$ \ell_i \f$ of the prefix it shares with \f$ s_{i-1} \f$ and the rest
 * \f$ \hat s_i \f$; the rear coding writes instead
 * \f$ r_i = | s_{i-1} | - \ell_i \f$, the characters to take off the end of
 * \f$ s_{i-1} \f$ before appending \f$ \hat s_i \f$. Both decode a string
 * from the one before, and they carry the same information; but
 * \f$ r_i \f$ is the smaller number where the strings share long prefixes
 * (the URLs, the paths, the words of a language), and a variable-length
 * code of it takes fewer bytes. The rear coding is the variant of the front
 * coding on which Ferragina, Grossi, Gupta, Shah and Vitter build their
 * compressed dictionaries of strings for the external memory.
 *
 * `RearCodedDictionary` is the front coding with buckets of the practical
 * compressed string dictionaries of Martínez-Prieto, Brisaboa, Cánovas,
 * Claude and Navarro, in the rear coding and with its characters
 * entropy-coded as in their HTFC, by a code of Huffman instead of their
 * code of Hu and Tucker (whose order the search here does not need, since
 * it decodes the strings it compares): one string of bits in which each
 * bucket of \f$ b \f$ strings begins with its first string (its length, then
 * its characters past the prefix that all the strings share and past the 16
 * bytes after it, which are kept apart in two words) and goes on with a triple
 * \f$ ( r_i , | \hat s_i | , \hat s_i ) \f$ per string, the characters in a
 * canonical code of Huffman of their frequencies, the rears and the lengths in
 * two more, of their own frequencies (255 escaping the larger values, written
 * in 30 bits), all the codewords of at most 11 bits [see
 * `limitedHuffmanLengths()`], so that a
 * table of \f$ 2^{11} \f$ entries decodes a symbol by one read of 11 bits,
 * and one of \f$ 2^{12} \f$ entries up to 3 characters by one of 12;
 * the places of the buckets in the string are kept by Elias-Fano [see
 * `EliasFano.h`]. With \f$ \mathit{RC}_H( D ) \f$ the bits of the coding,
 * within one bit per symbol of the entropy of each of the three streams,
 * the dictionary takes
 * \f[
 *   \mathit{RC}_H( D ) + \frac{n}{b} \Big( 130 + \Big\lceil \log_2
 *   \frac{b \, \mathit{RC}_H( D )}{n} \Big\rceil \Big) \text{ bits}
 * \f]
 * besides the directory of Elias-Fano and the three tables of decoding,
 * the 128 bits being those of the sample of each bucket below.
 *
 * A search is a binary search on the first strings of the buckets and the
 * scan of one bucket. The binary search reads, for each bucket, the 16
 * bytes of its first string kept apart, and decodes the rest of the first
 * string only when those bytes do not decide, and then only up to the
 * first character where it differs from the pattern. In the scan no string
 * is rebuilt: each one keeps the first \f$ | s_{i-1} | - r_i \f$
 * characters of the one before, its rest is decoded while it is compared
 * with the pattern \f$ Q \f$, and the comparison resumes from where it
 * stopped: with
 * \f$ \lambda \f$ the prefix that the current string shares with \f$ Q \f$,
 * a next string that keeps more than \f$ \lambda \f$ characters has the
 * same character as the current one where it differs from \f$ Q \f$, hence
 * the same order with respect to \f$ Q \f$, and is passed with no
 * comparison; one that keeps fewer is compared from its rest. A search
 * costs \f$ O( | Q | \log( n / b ) ) \f$ for the binary search and
 * \f$ O( | Q | + \sum_i | \hat s_i | ) \f$ for the bucket.
 *
 * The contracts [see `Contract.h`]: the dictionary sorted, an audit, the
 * strings and the coding shorter than \f$ 2^{30} \f$ and \f$ 2^{63} \f$
 * bytes, \f$ b \ge 1 \f$, and a pair of `rearDecode()` taking at most the
 * characters of the string before; the decoding of the coding gives the
 * strings back, an audit. The functions are defined in `RearCoding.tpp`.
 *
 * \see P. Ferragina, R. Grossi, A. Gupta, R. Shah, J. S. Vitter, On
 *      searching compressed string collections cache-obliviously,
 *      Proceedings of the 27th PODS, 181-190, 2008,
 *      doi:10.1145/1376916.1376943
 * \see M. A. Martínez-Prieto, N. R. Brisaboa, R. Cánovas, F. Claude,
 *      G. Navarro, Practical compressed string dictionaries, Information
 *      Systems 56, 73-108, 2016, doi:10.1016/j.is.2015.08.008
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9, Searching Strings by Prefix, and chapter 12,
 *      Statistical Coding, doi:10.1017/9781009128933
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

#ifndef __ADS_RearCoding
 #define __ADS_RearCoding
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/HuffmanCoder.h"
#include "ADS/string/FrontCoding.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a pair \f$ ( r , \hat s ) \f$ of the rear coding, the characters taken
/// off the end of the string before and the rest
using RearPair = std::pair< std::size_t , std::string >;

/// the rear coding of \f$ D \f$, a pair per string
///
/// In pseudocode (Ferragina, Grossi, Gupta, Shah and Vitter, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rearCode}( D ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 3 & \qquad \ell \gets 0 \text{ if } i = 0 \text{, else }
///     \mathrm{commonPrefixLength}( s_{i-1} , s_i ) \cr
/// 4 & \qquad R[ i ] \gets ( | s_{i-1} | - \ell , s_i[ \ell .. |s_i| - 1 ]
///     )\quad \triangleright\ | s_{-1} | = 0 \cr
/// 5 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \post `rearDecode()` of the result is \f$ D \f$ (audit)
[[nodiscard]] inline std::vector< RearPair > rearCode(
                                       const std::vector< std::string > & D );

/// the strings of the rear coding \f$ R \f$
///
/// In pseudocode (Ferragina, Grossi, Gupta, Shah and Vitter, beyond the
/// book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rearDecode}( R ) \cr
/// 2 & \quad s \gets \text{the empty string} \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |R| - 1\ \textbf{do} \cr
/// 4 & \qquad ( r , \hat s ) \gets R[ i ];\ s \gets s[ 0 .. |s| - r - 1 ]
///     \cdot \hat s;\ s_i \gets s \cr
/// 5 & \quad \textbf{return}\ ( s_0 , \ldots , s_{|R| - 1} )
/// \end{array}}
/// \f]
/// \pre each \f$ r \f$ is at most the length of the string before (0 for
///      the first one)
[[nodiscard]] inline std::vector< std::string > rearDecode(
                                       const std::vector< RearPair > & R );

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS RearCodedDictionary ------------------------*/
/*--------------------------------------------------------------------------*/

/// a sorted dictionary of strings in the rear coding with buckets, in one
/// string of bytes
class RearCodedDictionary {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty dictionary
 RearCodedDictionary( void ) = default;

 /// the rear coding of \f$ D \f$ in buckets of \f$ b \f$ strings
 ///
 /// In pseudocode (Martínez-Prieto et al. in the rear coding, beyond the
 /// book), \f$ \mathrm{code}( v ) \f$ the codeword of \f$ v \f$ in its code
 /// of Huffman, built first on the frequencies of the characters, of the
 /// rears and of the lengths:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( D , b ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 3 & \qquad \textbf{if}\ i \bmod b = 0\ \textbf{then} \cr
 /// 4 & \qquad\quad \text{append the length of } C \text{ to } P;
 ///     \ \text{append } \mathrm{code}( | s_i | ) \cdot \mathrm{code}( s_i[
 ///     g + 16 .. | s_i | - 1 ] ) \text{ to } C \text{, with } g
 ///     \text{ the prefix that all the strings share} \cr
 /// 5 & \qquad \textbf{else} \cr
 /// 6 & \qquad\quad \ell \gets \mathrm{commonPrefixLength}( s_{i-1} , s_i )
 ///     \cr
 /// 7 & \qquad\quad \text{append } \mathrm{code}( | s_{i-1} | - \ell ) \cdot
 ///     \mathrm{code}( | s_i | - \ell ) \cdot \mathrm{code}( s_i[ \ell ..
 ///     | s_i | - 1 ] ) \text{ to } C \cr
 /// 8 & \quad \text{keep } P \text{ by Elias-Fano, and the 16 bytes after }
 ///     \text{the common prefix of each first string}
 /// \end{array}}
 /// \f]
 /// \pre \f$ D \f$ is sorted (audit), its strings shorter than
 ///      \f$ 2^{30} \f$ bytes, and \f$ b \ge 1 \f$
 explicit RearCodedDictionary( const std::vector< std::string > & D ,
                               std::size_t b = 16 );

 /// the number \f$ n \f$ of strings
 [[nodiscard]] std::size_t size( void ) const;

 /// whether there is no string
 [[nodiscard]] bool empty( void ) const;

 /// the strings of a bucket, \f$ b \f$
 [[nodiscard]] std::size_t bucketSize( void ) const;

 /// the string \f$ i \f$, decoded from the first string of its bucket
 ///
 /// In pseudocode (Martínez-Prieto et al. in the rear coding, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad s \gets \text{the first string of the bucket }
 ///     \lfloor i / b \rfloor \text{, from the place } P[ \lfloor i / b
 ///     \rfloor ] \text{ of } C \cr
 /// 3 & \quad \textbf{for}\ j \gets b \lfloor i / b \rfloor + 1\ \textbf{to}
 ///     \ i\ \textbf{do}\ s \gets s[ 0 .. |s| - r_j - 1 ] \cdot \hat s_j \cr
 /// 4 & \quad \textbf{return}\ s
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::string access( std::size_t i ) const;

 /// the place of \f$ Q \f$, `size()` if it is not a string of the
 /// dictionary, by the search of `lexicographicPosition()`, which tells
 /// whether the string where it stops is \f$ Q \f$
 [[nodiscard]] std::size_t lookup( std::string_view Q ) const;

 /// the number of strings smaller than \f$ Q \f$
 ///
 /// In pseudocode (the two-level search of chapter 9, Searching Strings by
 /// Prefix, of Ferragina's book, in the rear coding), \f$ \lambda \f$ the
 /// prefix shared by \f$ s \f$ and \f$ Q \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lexicographicPosition}( Q ) \cr
 /// 2 & \quad k \gets \text{the buckets whose first string is smaller than }
 ///     Q\text{, by a binary search} \cr
 /// 3 & \quad \textbf{if}\ k = 0\ \textbf{then return}\ 0 \cr
 /// 4 & \quad i \gets b ( k - 1 );\ s \gets \text{the first string of the }
 ///     \text{bucket } k - 1;\ \lambda \gets
 ///     \mathrm{commonPrefixLength}( s , Q ) \cr
 /// 5 & \quad \textbf{while}\ i + 1 < \min\{ b k , n \}\ \textbf{do}\quad
 ///     \triangleright\ s < Q \text{ here} \cr
 /// 6 & \qquad \kappa \gets | s | - r_{i+1};\ s \gets s[ 0 .. \kappa - 1 ]
 ///     \cdot \hat s_{i+1} \cr
 /// 7 & \qquad \textbf{if}\ \kappa \le \lambda\ \textbf{then}\ \lambda \gets
 ///     \kappa + \mathrm{commonPrefixLength}( \hat s_{i+1} , Q[ \kappa ..
 ///     ] ) \cr
 /// 8 & \qquad\quad \textbf{if}\ s \ge Q\ \textbf{then return}\ i + 1\quad
 ///     \triangleright\ \text{by the character after } \lambda \cr
 /// 9 & \qquad i \gets i + 1\quad \triangleright\ \kappa > \lambda
 ///     \text{: } s \text{ still smaller than } Q \cr
 /// 10 & \quad \textbf{return}\ \min\{ b k , n \}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than \f$ P\# \f$, i.e., smaller than
 /// \f$ P \f$ or prefixed by it, by the search of
 /// `lexicographicPosition()` with that order
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range \f$ [ l , r ) \f$ of the strings prefixed by \f$ P \f$, by
 /// `lexicographicPosition()` and `prefixEnd()`
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the bytes of the coding, \f$ \mathit{RC}_H( D ) \f$ with the first
 /// strings and the lengths
 [[nodiscard]] std::size_t codeBytes( void ) const;

 /// all the bytes: the coding, the places and the samples of the
 /// buckets, and the codes with their tables
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the longest codeword, \f$ 2^{11} \f$ entries in a table of decoding
 static constexpr int kMax = 11;

 /// the bits of a window of the table that decodes up to 3 symbols
 static constexpr int kWide = 12;

 /// the bytes of the first string of a bucket, after the prefix that all
 /// the strings share, kept apart in kHead / 8 words
 static constexpr std::size_t kHead = 16;

 /// a canonical code of Huffman on the 256 bytes, of codewords of at most
 /// `kMax` bits written with their first bit in the lowest place, and its
 /// table of decoding by the next `kMax` bits
 struct Code {
  std::array< std::uint8_t , 256 > len{};     ///< the lengths, 0 if unused
  std::array< std::uint32_t , 256 > word{};   ///< the codewords, turned
  std::vector< std::uint16_t > table;         ///< a byte and its length
  /// the bytes of up to 3 codewords whole in a window of kWide bits, their
  /// number and their bits, from the bit 24 and 26
  std::vector< std::uint32_t > multi;

  /// the code of the frequencies \f$ f \f$, by the lengths of
  /// `limitedHuffmanLengths()`
  void build( const std::array< std::uint64_t , 256 > & f );
  };

 /// the byte of the code \f$ c \f$ at the bit `pos`, moved past it
 /// the next 56 bits of the coding at the bit `pos` at least, the first
 /// one the lowest, by one load of 8 bytes
 [[nodiscard]] std::uint64_t peek( std::size_t pos ) const;

 [[nodiscard]] unsigned decode( const Code & c , std::size_t & pos ) const;

 /// the length of the code \f$ c \f$ at the bit `pos`, 255 followed by 30
 /// bits for one of 255 or more, moved past it
 [[nodiscard]] std::size_t getLength( const Code & c ,
                                      std::size_t & pos ) const;

 /// appends the length \f$ v \f$ in the code \f$ c \f$
 void putLength( const Code & c , std::size_t v );

 /// the first 8 bytes of \f$ s \f$, the first one the highest, 0 past its
 /// end
 [[nodiscard]] static std::uint64_t prefix8( std::string_view s );

 /// the byte \f$ j \f$ of those kept apart of the first string of the
 /// bucket \f$ k \f$, 0 past its end
 [[nodiscard]] unsigned byte( std::size_t k , std::size_t j ) const;

 /// appends to \f$ s \f$ the \f$ \mathit{len} \f$ characters at the bit
 /// `pos`, moved past them
 void readRest( std::string & s , std::size_t len , std::size_t & pos )
  const;

 /// moves `pos` past the \f$ n \f$ characters at it, which stand at the
 /// places from \f$ a \f$ of a string that shares its first \f$ l \f$
 /// characters with \f$ Q \f$; if \f$ l = a \f$, \f$ l \f$ grows over the
 /// characters equal to those of \f$ Q \f$, and the result is the first
 /// one that differs (256 if none does before one of the two ends), past
 /// which `pos` stops if not `S`
 template< bool S >
 [[nodiscard]] unsigned follow( std::string_view Q , std::size_t & l ,
                                std::size_t a , std::size_t n ,
                                std::size_t & pos ) const;

 /// the first string of the bucket \f$ k \f$, and where the coding of the
 /// next string begins
 [[nodiscard]] std::string head( std::size_t k , std::size_t & pos ) const;

 /// the number of strings smaller than \f$ Q \f$, or than \f$ Q\# \f$ if
 /// `upper`, and in `*eq`, if given, whether the string of that rank is
 /// \f$ Q \f$
 [[nodiscard]] std::size_t rank( std::string_view Q , bool upper ,
                                 bool * eq = nullptr ) const;

 std::size_t f_n = 0;           ///< the strings
 std::size_t f_b = 16;          ///< the strings of a bucket
 BitVector f_C;                 ///< the coding
 EliasFano f_P;                 ///< the bits where the buckets begin
 std::string f_common;          ///< the prefix of all the strings
 std::vector< std::uint64_t > f_head;   ///< kHead bytes of each first
 Code f_char;                   ///< the code of the characters
 Code f_rear;                   ///< the code of the rears
 Code f_size;                   ///< the code of the lengths of the rests

 };  // end( class RearCodedDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/RearCoding.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RearCoding.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RearCoding.h --------------------------*/
/*--------------------------------------------------------------------------*/
