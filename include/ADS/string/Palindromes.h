/*--------------------------------------------------------------------------*/
/*--------------------------- File Palindromes.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Three classic problems on the palindromes of a sequence \f$ s \f$ of
 * \f$ n \f$ items, and the eertree.
 *
 * - `longestPalindromicSubstring()`: the longest contiguous palindrome, by
 *   the algorithm of Manacher in \f$ \Theta( n ) \f$ time and space. The
 *   dynamic programming of the textbook fills the table
 *   \f$ P[ i ][ j ] \f$, whether \f$ s[ i .. j ] \f$ is a palindrome, by
 *   increasing length, in \f$ \Theta( n^2 ) \f$ time and space, and the
 *   expansion about each of the \f$ 2 n - 1 \f$ centres takes
 *   \f$ O( n^2 ) \f$ time and \f$ O( 1 ) \f$ space; Manacher keeps the
 *   rightmost palindrome found so far and starts each new centre from the
 *   radius of its mirror inside it, so that the right end only moves
 *   forward. The suffix tree of \f$ s \f$ and of its reverse gives the same
 *   answer in linear time with a much larger constant [see
 *   `SuffixTree.h`].
 * - `longestPalindromicSubsequence()`: the longest palindrome that is a
 *   subsequence of \f$ s \f$; \f$ \mathit{DP}[ i ][ j ] \f$, its length for
 *   \f$ s[ i .. j ] \f$, is 1 if \f$ i = j \f$,
 *   \f$ \mathit{DP}[ i + 1 ][ j - 1 ] + 2 \f$ if \f$ s_i = s_j \f$, and the
 *   larger of \f$ \mathit{DP}[ i + 1 ][ j ] \f$ and
 *   \f$ \mathit{DP}[ i ][ j - 1 ] \f$ otherwise, in \f$ \Theta( n^2 ) \f$
 *   time and space, the palindrome read back from both ends. It is the
 *   longest common subsequence of \f$ s \f$ and of its reverse [see
 *   `LongestCommonSubsequence.h`], so the bit-parallel and the sparse
 *   algorithms of the LCS apply to it too.
 * - `palindromicPartition()`: the fewest palindromes into which \f$ s \f$
 *   can be cut; \f$ D[ j ] \f$, the fewest for \f$ s[ 0 .. j ) \f$, is the
 *   least \f$ D[ i ] + 1 \f$ over the palindromes \f$ s[ i .. j ) \f$, and
 *   \f$ D[ 0 ] = 0 \f$. The textbook builds the table of the palindromes
 *   first, in \f$ \Theta( n^2 ) \f$ space; here each palindrome
 *   \f$ s[ i .. j ) \f$ relaxes \f$ D[ j ] \f$ while it is found by the
 *   expansion about its centre, the centres taken left to right, so that
 *   \f$ D[ i ] \f$ is final when it is used: \f$ O( n^2 ) \f$ time and
 *   \f$ O( n ) \f$ space.
 * - `Eertree`, beyond the book, is the eertree (palindromic tree) of
 *   Rubinchik and Shur: a node per distinct nonempty palindrome (at most
 *   \f$ n \f$ of them, since a byte added at the end creates at most one new
 *   palindrome, the longest palindromic suffix), two roots of lengths
 *   \f$ -1 \f$ and 0, the edge \f$ c \f$ from \f$ v \f$ to \f$ c v c \f$, and
 *   the suffix link of \f$ v \f$ to its longest proper palindromic suffix;
 *   `add()` finds the longest palindromic suffix of the new string on the
 *   links of the previous one, \f$ O( 1 ) \f$ amortized steps for a
 *   constant alphabet (the edges of a node are a short vector here,
 *   \f$ O( \sigma ) \f$ each).
 * - `palindromicFactorization()`, beyond the book, gives the same fewest
 *   palindromes in \f$ O( n \log n ) \f$ time (Fici, Gagie, Kärkkäinen and
 *   Kempa; Rubinchik and Shur): the lengths of the palindromic suffixes of
 *   a prefix form \f$ O( \log n ) \f$ arithmetic series, with the
 *   difference \f$ \mathit{diff}( v ) \f$ of the lengths of \f$ v \f$ and
 *   of \f$ \mathit{link}( v ) \f$ constant along a series and the
 *   series link to the first node past it, and the best of a series at the
 *   place \f$ i \f$ is the best of the same series at
 *   \f$ i - \mathit{diff}( v ) \f$ together with its shortest palindrome,
 *   \f[
 *     D[ i ] = 1 + \min_{v} \mathit{best}_i( v ) , \qquad
 *     \mathit{best}_i( v ) = \min\{ D[ i - \mathit{len}( \mathit{series}(
 *     v ) ) - \mathit{diff}( v ) ] , \mathit{best}_{i - \mathit{diff}( v )}(
 *     \mathit{link}( v ) ) \} ,
 *   \f]
 *   the second term only when
 *   \f$ \mathit{diff}( v ) = \mathit{diff}( \mathit{link}( v ) ) \f$.
 *
 * Each function of the textbook takes any sequence `S` with `size()`,
 * `operator[]`, an iterator constructor and the comparison of its items;
 * the overloads on `std::string` also take string literals; the eertree and
 * its factorization take bytes.
 *
 * The contracts [see `Contract.h`]: the result is a palindrome; the
 * subsequence is one of \f$ s \f$ as long as the LCS of \f$ s \f$ and its
 * reverse, and the pieces of the partition give \f$ s \f$ back (both
 * audits), and the factorization by the eertree has as many as the
 * partition (an audit). The functions are defined in `Palindromes.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see G. Manacher, A new linear-time "on-line" algorithm for finding the
 *      smallest initial palindrome of a string, Journal of the ACM 22(3),
 *      346-351, 1975, doi:10.1145/321892.321896
 * \see G. Fici, T. Gagie, J. Kärkkäinen, D. Kempa, A subquadratic algorithm
 *      for minimum palindromic factorization, Journal of Discrete Algorithms
 *      28, 41-48, 2014, doi:10.1016/j.jda.2014.08.001
 * \see M. Rubinchik, A. M. Shur, EERTREE: an efficient data structure for
 *      processing palindromes in strings, European Journal of Combinatorics
 *      68, 249-265, 2018, doi:10.1016/j.ejc.2017.07.021
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

#ifndef __ADS_Palindromes
 #define __ADS_Palindromes
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/LongestCommonSubsequence.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// where a longest palindromic substring of \f$ s \f$ starts and its
/// length, the leftmost one among the longest; \f$ ( 0 , 0 ) \f$ for the
/// empty \f$ s \f$
///
/// In pseudocode (Manacher's algorithm, beyond the book),
/// \f$ [ l , r ) \f$ the rightmost palindrome found so far, from which a
/// centre inside it starts with the radius of its mirror; \f$ r \f$ only
/// moves forward, hence \f$ \Theta( n ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPalindrome}( s ) \cr
/// 2 & \quad l \gets 0;\ r \gets 0\quad \triangleright\ \text{the odd }
///     \text{palindromes, centred at } i \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i < r\ \textbf{then}\ k \gets \min\{ d_1[ l + r
///     - 1 - i ] , r - i \}\ \textbf{else}\ k \gets 1 \cr
/// 5 & \qquad \textbf{while}\ i + k < n\ \textbf{and}\ i \ge k
///     \ \textbf{and}\ s[ i - k ] = s[ i + k ]\ \textbf{do}\ k \gets k + 1 \cr
/// 6 & \qquad d_1[ i ] \gets k;\ \textbf{if}\ i + k > r\ \textbf{then}\ l
///     \gets i + 1 - k;\ r \gets i + k \cr
/// 7 & \qquad \text{keep } ( i + 1 - k , 2 k - 1 ) \cr
/// 8 & \quad l \gets 0;\ r \gets 0\quad \triangleright\ \text{the even }
///     \text{palindromes, right half from } i \cr
/// 9 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 10 & \qquad \textbf{if}\ i < r\ \textbf{then}\ k \gets \min\{ d_2[ l + r
///     - i ] , r - i \}\ \textbf{else}\ k \gets 0 \cr
/// 11 & \qquad \textbf{while}\ i + k < n\ \textbf{and}\ i \ge k + 1
///     \ \textbf{and}\ s[ i - k - 1 ] = s[ i + k ]\ \textbf{do}\ k \gets k + 1
///     \cr
/// 12 & \qquad d_2[ i ] \gets k;\ \textbf{if}\ i + k > r\ \textbf{then}\ l
///     \gets i - k;\ r \gets i + k \cr
/// 13 & \qquad \textbf{if}\ k > 0\ \textbf{then}\ \text{keep } ( i - k , 2 k
///     ) \cr
/// 14 & \quad \textbf{return}\ \text{the leftmost of the longest kept}
/// \end{array}}
/// \f]
/// \post \f$ s[ \mathit{start} .. \mathit{start} + \mathit{length} ) \f$
///       is a palindrome
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::pair< std::size_t , std::size_t > longestPalindrome(
                                                              const S & s );

/// the leftmost of the longest palindromic substrings of \f$ s \f$
///
/// In pseudocode (Manacher's algorithm, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPalindromicSubstring}( s ) \cr
/// 2 & \quad ( a , \ell ) \gets \mathrm{longestPalindrome}( s );
///     \ \textbf{return}\ s[ a .. a + \ell - 1 ]
/// \end{array}}
/// \f]
/// \post a palindrome occurring in \f$ s \f$, empty only if \f$ s \f$ is
template< class S > requires std::is_class_v< S >
[[nodiscard]] S longestPalindromicSubstring( const S & s );

/// the substring of `longestPalindromicSubstring()` above, on a
/// `std::string` (or a literal)
[[nodiscard]] inline std::string longestPalindromicSubstring(
                                                     const std::string & s );

/// a longest palindromic subsequence of \f$ s \f$
///
/// In pseudocode (chapter 13), the table filled by decreasing \f$ i \f$ and
/// read back from both ends:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestPalindromicSubsequence}( s ) \cr
/// 2 & \quad \textbf{foreach}\ i = n - 1 , \ldots , 0\ \textbf{do} \cr
/// 3 & \qquad \mathit{DP}[ i ][ i ] \gets 1 \cr
/// 4 & \qquad \textbf{for}\ j \gets i + 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ s_i = s_j\ \textbf{then}\ \mathit{DP}[ i ][
///     j ] \gets \mathit{DP}[ i + 1 ][ j - 1 ] + 2\quad \triangleright\ 0
///     \text{ if } i + 1 > j - 1 \cr
/// 6 & \qquad\quad \textbf{else}\ \mathit{DP}[ i ][ j ] \gets \max\{
///     \mathit{DP}[ i + 1 ][ j ] , \mathit{DP}[ i ][ j - 1 ] \} \cr
/// 7 & \quad i \gets 0;\ j \gets n - 1;\ L \gets \text{an empty sequence} \cr
/// 8 & \quad \textbf{while}\ i < j\ \textbf{do} \cr
/// 9 & \qquad \textbf{if}\ s_i = s_j\ \textbf{then}\ \text{append } s_i
///     \text{ to } L;\ i \gets i + 1;\ j \gets j - 1 \cr
/// 10 & \qquad \textbf{else if}\ \mathit{DP}[ i + 1 ][ j ] \ge \mathit{DP}[
///     i ][ j - 1 ]\ \textbf{then}\ i \gets i + 1\ \textbf{else}\ j \gets j
///     - 1 \cr
/// 11 & \quad \textbf{return}\ L \cdot ( s_i \text{ if } i = j ) \cdot
///     \text{the reverse of } L
/// \end{array}}
/// \f]
/// \post a palindrome, a subsequence of \f$ s \f$ as long as the LCS of
///       \f$ s \f$ and its reverse (audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] S longestPalindromicSubsequence( const S & s );

/// the subsequence of `longestPalindromicSubsequence()` above, on a
/// `std::string` (or a literal)
[[nodiscard]] inline std::string longestPalindromicSubsequence(
                                                     const std::string & s );

/// the fewest palindromes into which \f$ s \f$ can be cut, in their order;
/// none for the empty \f$ s \f$
///
/// In pseudocode (chapter 13, with the table of the palindromes replaced by
/// the expansion about the centres), the centres by increasing
/// \f$ h = a + b - 1 \f$ for the palindrome \f$ s[ a .. b ) \f$, so that
/// \f$ D[ a ] \f$ is final when it is used:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{palindromicPartition}( s ) \cr
/// 2 & \quad D[ 0 ] \gets 0;\ D[ j ] \gets \infty \text{ for } j > 0 \cr
/// 3 & \quad \textbf{for}\ h \gets 0\ \textbf{to}\ 2 n - 2\ \textbf{do} \cr
/// 4 & \qquad ( a , b ) \gets \text{the shortest palindrome of centre } h
///     \text{ (none for an even centre with } s_{a-1} \ne s_a \text{)} \cr
/// 5 & \qquad \textbf{loop} \cr
/// 6 & \qquad\quad \textbf{if}\ D[ a ] + 1 < D[ b ]\ \textbf{then}\ D[ b ]
///     \gets D[ a ] + 1;\ \mathit{from}[ b ] \gets a \cr
/// 7 & \qquad\quad \textbf{if}\ a = 0\ \textbf{or}\ b = n\ \textbf{or}\ s_{a
///     - 1} \ne s_b\ \textbf{then}\ \textbf{break} \cr
/// 8 & \qquad\quad a \gets a - 1;\ b \gets b + 1 \cr
/// 9 & \quad \textbf{return}\ \text{the pieces read back from } n \text{ on }
///     \mathit{from}
/// \end{array}}
/// \f]
/// \post every piece a palindrome, and the pieces joined give \f$ s \f$
///       (audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::vector< S > palindromicPartition( const S & s );

/// the pieces of `palindromicPartition()` above, on a `std::string` (or a
/// literal)
[[nodiscard]] inline std::vector< std::string > palindromicPartition(
                                                     const std::string & s );

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Eertree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the eertree (palindromic tree) of a string, built a byte at a time
class Eertree {

/*--------------------------------------------------------------------------*/
 public:

 /// the eertree of the empty string: the two roots, of length \f$ -1 \f$
 /// and 0
 Eertree( void );

 /// the eertree of \f$ s \f$, by `add()` on each of its bytes
 explicit Eertree( std::string_view s );

 /// adds the byte \f$ c \f$ at the end; whether the longest palindromic
 /// suffix of the new string occurs there for the first time
 ///
 /// In pseudocode (Rubinchik and Shur, beyond the book),
 /// \f$ \mathrm{ext}( v , c ) \f$ the first palindromic suffix on the links
 /// from \f$ v \f$ that is preceded by \f$ c \f$, and \f$ \mathit{last} \f$
 /// the longest palindromic suffix:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{add}( c ) \cr
 /// 2 & \quad \text{append } c \text{ to } s;\ v \gets \mathrm{ext}(
 ///     \mathit{last} , c ) \cr
 /// 3 & \quad \textbf{if}\ v \text{ has the edge } c \text{ to } w
 ///     \ \textbf{then}\ \mathit{last} \gets w;\ \textbf{return false} \cr
 /// 4 & \quad u \gets \text{a new node};\ \mathit{len}( u ) \gets
 ///     \mathit{len}( v ) + 2 \cr
 /// 5 & \quad \textbf{if}\ \mathit{len}( u ) = 1\ \textbf{then}
 ///     \ \mathit{link}( u ) \gets \text{the root of length } 0 \cr
 /// 6 & \quad \textbf{else}\ \mathit{link}( u ) \gets \text{the child } c
 ///     \text{ of } \mathrm{ext}( \mathit{link}( v ) , c ) \cr
 /// 7 & \quad \mathit{diff}( u ) \gets \mathit{len}( u ) - \mathit{len}(
 ///     \mathit{link}( u ) ) \cr
 /// 8 & \quad \mathit{series}( u ) \gets \mathit{series}( \mathit{link}( u ) )
 ///     \text{ if } \mathit{diff}( u ) = \mathit{diff}( \mathit{link}( u ) )
 ///     \text{, else } \mathit{link}( u ) \cr
 /// 9 & \quad \text{add the edge } c \text{ from } v \text{ to } u;
 ///     \ \mathit{last} \gets u;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool add( unsigned char c );

 /// the number of the distinct nonempty palindromes of the string
 [[nodiscard]] std::size_t distinct( void ) const {
  return( f_len.size() - 2 );
  }

 /// the length of the longest palindromic suffix of the string
 [[nodiscard]] std::size_t suffixLength( void ) const {
  return( std::size_t( f_len[ f_last ] ) );
  }

 /// the length \f$ n \f$ of the string
 [[nodiscard]] std::size_t size( void ) const { return( f_s.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 friend std::vector< std::string > palindromicFactorization(
                                                        std::string_view s );

 /// the first palindromic suffix of the string from v (v itself or a
 /// suffix of it on the links) that c before it extends
 [[nodiscard]] std::uint32_t extensible( std::uint32_t v ,
                                         unsigned char c ) const;

 std::string f_s;
 std::vector< long > f_len;                         ///< -1 , 0 , ...
 std::vector< std::uint32_t > f_link;               ///< suffix palindrome
 std::vector< long > f_diff;                        ///< len - len( link )
 std::vector< std::uint32_t > f_series;  ///< the link past the series
 std::vector< std::vector< std::pair< unsigned char , std::uint32_t > > >
  f_next;                                           ///< c v c
 std::uint32_t f_last = 1;               ///< the longest suffix palindrome

 };  // end( class Eertree )

/// the fewest palindromes into which \f$ s \f$ can be cut, in their order,
/// by the series of the eertree, in \f$ O( n \log n ) \f$; none for the
/// empty \f$ s \f$
///
/// In pseudocode (Fici, Gagie, Kärkkäinen and Kempa, and Rubinchik and
/// Shur, beyond the book), \f$ \mathit{best}( v ) \f$ the least \f$ D \f$
/// before a palindrome of the series of \f$ v \f$ that ends at \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{palindromicFactorization}( s ) \cr
/// 2 & \quad E \gets \text{an empty eertree};\ D[ 0 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad E.\mathrm{add}( s[ i - 1 ] );\ D[ i ] \gets \infty \cr
/// 5 & \qquad \textbf{for}\ v \gets \mathit{last}\ \textbf{while}
///     \ \mathit{len}( v ) > 0\ \textbf{do}\ v \gets \mathit{series}( v ) \cr
/// 6 & \qquad\quad L \gets \mathit{len}( \mathit{series}( v ) ) +
///     \mathit{diff}( v );\ \mathit{best}( v ) \gets D[ i - L ] \cr
/// 7 & \qquad\quad \textbf{if}\ \mathit{diff}( v ) = \mathit{diff}(
///     \mathit{link}( v ) )\ \textbf{then}\ \mathit{best}( v ) \gets \min\{
///     \mathit{best}( v ) , \mathit{best}( \mathit{link}( v ) ) \} \cr
/// 8 & \qquad\quad D[ i ] \gets \min\{ D[ i ] , \mathit{best}( v ) + 1 \} \cr
/// 9 & \quad \textbf{return}\ \text{the pieces read back from } n
/// \end{array}}
/// \f]
/// \post as many pieces as `palindromicPartition( s )`, palindromes, that
///       give \f$ s \f$ back (audit)
[[nodiscard]] inline std::vector< std::string > palindromicFactorization(
                                                        std::string_view s );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Palindromes.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Palindromes.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Palindromes.h -------------------------*/
/*--------------------------------------------------------------------------*/
