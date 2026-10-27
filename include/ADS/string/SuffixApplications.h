/*--------------------------------------------------------------------------*/
/*----------------------- File SuffixApplications.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Problems on strings solved by a suffix array [see `SuffixArray.h`] or a
 * suffix tree [see `SuffixTree.h`], beyond the book (those of chapter 10,
 * Searching Strings by Substring, of Ferragina's book and their classical
 * companions). A substring that occurs twice is a common prefix of two
 * suffixes, and the suffixes that share a prefix of length \f$ L \f$ are a
 * range of \f$ \mathit{SA} \f$ whose \f$ \mathit{LCP} \f$ entries are all at
 * least \f$ L \f$: hence a scan of \f$ \mathit{LCP} \f$, or a visit of the
 * internal nodes of the suffix tree, answers the questions on the repeats in
 * \f$ O( n ) \f$ time.
 *
 * - `longestRepeatedSubstring()`: the largest entry of \f$ \mathit{LCP} \f$,
 *   \f$ O( n ) \f$, against the \f$ O( n^2 ) \f$ of the dynamic programming
 *   of a string against itself;
 * - `repeatedSubstring( SA , L , C )`, a query of text mining: a substring of
 *   length \f$ L \f$ that occurs at least \f$ C \f$ times is a run of at least
 *   \f$ C - 1 \f$ entries of \f$ \mathit{LCP} \f$ that are at least \f$ L \f$,
 *   one scan, \f$ O( n ) \f$, against the \f$ O( n^2 L ) \f$ of counting every
 *   substring;
 * - `distinctSubstrings()`: each suffix \f$ \mathit{SA}[ r ] \f$ adds the
 *   \f$ n - \mathit{SA}[ r ] \f$ prefixes less the
 *   \f$ \mathit{LCP}[ r - 1 ] \f$ it shares with the previous one, hence
 *   \f$ n ( n + 1 ) / 2 - \sum_r \mathit{LCP}[ r ] \f$, \f$ O( n ) \f$;
 * - `longestCommonSubstring( S , k )`: the longest substring common to at
 *   least \f$ k \f$ of the \f$ m \f$ strings of \f$ S \f$, on the suffix
 *   array of their join with \f$ m \f$ distinct separators (an integer
 *   alphabet): a window of ranks that covers suffixes of \f$ k \f$ distinct
 *   strings, slid by two pointers, whose common prefix is the `lce()` of its
 *   two ends, \f$ O( N ) \f$ for \f$ N \f$ the total length, against the
 *   \f$ O( |P| |T| ) \f$ of the dynamic programming for two strings [see
 *   `LongestCommonSubsequence.h`] and the generalized suffix tree with the
 *   colors of Hui for any \f$ k \f$;
 * - `maximalRepeats()`: the maximal repeats, the substrings that occur twice
 *   and cannot be extended to the right (an internal node)
 *   nor to the left (the symbols before their occurrences are not all the
 *   same, or one occurrence starts the text): the internal nodes whose
 *   leaves are left-diverse, by one visit from the leaves up,
 *   \f$ O( n ) \f$, as in Gusfield;
 * - `proximity( SA , P , Q , k )`, the proximity search:
 *   the occurrences of \f$ P \f$ and of \f$ Q \f$, sorted together, have an
 *   occurrence of each at distance at most \f$ k \f$ exactly when two
 *   adjacent ones of different patterns do,
 *   \f$ O( |P| + |Q| + \mathit{occ} \log \mathit{occ} ) \f$;
 * - `kMismatches( T , P , k )`: the places
 *   where \f$ P \f$ occurs with at most \f$ k \f$ substitutions, each
 *   checked by at most \f$ k + 1 \f$ longest common extensions of a suffix
 *   of \f$ T \f$ and a suffix of \f$ P \f$, both suffixes of
 *   \f$ T \# P \f$, by the range minima of its \f$ \mathit{LCP} \f$ in
 *   \f$ O( 1 ) \f$: \f$ O( n k ) \f$ after \f$ O( n + p ) \f$
 *   preprocessing, against the \f$ O( n p ) \f$ of trying every place, as in
 *   Landau and Vishkin;
 * - `lz77ByTree( T )`, the parsing of Ziv and Lempel by the suffix tree: the
 *   phrase at \f$ i \f$ copies the longest prefix of the suffix \f$ i \f$ that
 *   starts earlier, found by walking down along the suffix \f$ i \f$ while the
 *   least leaf below the next node is smaller than \f$ i \f$, the least leaves
 *   computed by one visit; the copy is taken from the farthest occurrence, the
 *   least leaf, \f$ O( n \log \sigma ) \f$ in all (\f$ O( n ) \f$ with perfect
 *   hashing at the nodes). The phrase is the triple (distance, length, next
 *   symbol), the last one without the symbol when the copy reaches the end of
 *   \f$ T \f$; `decodeCopyPhrases()` inverts it.
 *
 * The contracts [see `Contract.h`]: \f$ k \f$ is at least 1 and at most the
 * number of strings for `longestCommonSubstring()`, \f$ C \f$ at least 1;
 * the parsing decodes to the text (an audit). The functions are defined in
 * `SuffixApplications.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10, Searching Strings by Substring,
 *      doi:10.1017/9781009128933
 * \see D. Gusfield, Algorithms on Strings, Trees, and Sequences, Cambridge
 *      University Press, 1997, sections 7.4-7.12,
 *      doi:10.1017/CBO9780511574931
 * \see L. C. K. Hui, Color set size problem with applications to string
 *      matching, Proceedings of the 3rd Symposium on Combinatorial Pattern
 *      Matching (CPM), LNCS 644, 230-243, 1992, doi:10.1007/3-540-56024-6_19
 * \see M. I. Abouelhoda, S. Kurtz, E. Ohlebusch, Replacing suffix trees with
 *      enhanced suffix arrays, Journal of Discrete Algorithms 2(1), 53-86,
 *      2004, doi:10.1016/S1570-8667(03)00065-0
 * \see G. M. Landau, U. Vishkin, Efficient string matching with k
 *      mismatches, Theoretical Computer Science 43, 239-249, 1986,
 *      doi:10.1016/0304-3975(86)90178-7
 * \see J. Ziv, A. Lempel, A universal algorithm for sequential data
 *      compression, IEEE Transactions on Information Theory 23(3), 337-343,
 *      1977, doi:10.1109/TIT.1977.1055714
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

#ifndef __ADS_SuffixApplications
 #define __ADS_SuffixApplications
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixTree.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a phrase of the parsing of Ziv and Lempel: copy `length` symbols from
/// `distance` places back, then append `next`, if any
struct CopyPhrase {
 std::size_t distance = 0;    ///< how far back the copy starts
 std::size_t length = 0;      ///< the symbols copied
 std::optional< char > next;  ///< the symbol after the copy, none at the end
 /// the phrases are equal field by field
 friend bool operator==( const CopyPhrase & , const CopyPhrase & ) = default;
 };

/// the place and the length of a longest substring that occurs twice
/// (0 and 0 if none)
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestRepeatedSubstring}( \mathit{SA} )
///     \cr
/// 2 & \quad r \gets \text{the first rank with the largest }
///     \mathit{LCP}[ r ] \cr
/// 3 & \quad \textbf{return}\ ( \mathit{SA}[ r ] , \mathit{LCP}[ r ] )
///     \text{, or } ( 0 , 0 ) \text{ if } \mathit{LCP} \text{ is all 0}
/// \end{array}}
/// \f]
[[nodiscard]] inline std::pair< std::size_t , std::size_t >
                         longestRepeatedSubstring( const SuffixArray & SA );

/// the place of a substring of length \f$ L \f$ that occurs at least
/// \f$ C \f$ times, if any
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{repeatedSubstring}( \mathit{SA} , L , C )
///     \cr
/// 2 & \quad \textbf{if}\ L = 0\ \textbf{then return}\ 0 \text{ if } C \le n
///     + 1 \text{, and } \textbf{nil} \text{ otherwise} \cr
/// 3 & \quad \textbf{if}\ L > n\ \textbf{then return}\ \textbf{nil} \cr
/// 4 & \quad \textbf{if}\ C = 1\ \textbf{then return}\ 0 \cr
/// 5 & \quad \mathit{run} \gets 0 \cr
/// 6 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 2\ \textbf{do} \cr
/// 7 & \qquad \textbf{if}\ \mathit{LCP}[ r ] \ge L\ \textbf{then} \cr
/// 8 & \qquad\quad \mathit{run} \gets \mathit{run} + 1;\ \textbf{if}
///     \ \mathit{run} \ge C - 1\ \textbf{then return}\ \mathit{SA}[ r ] \cr
/// 9 & \qquad \textbf{else}\ \mathit{run} \gets 0 \cr
/// 10 & \quad \textbf{return}\ \textbf{nil}
/// \end{array}}
/// \f]
/// \pre \f$ C \ge 1 \f$
[[nodiscard]] inline std::optional< std::size_t > repeatedSubstring(
                  const SuffixArray & SA , std::size_t L , std::size_t C );

/// the number of distinct nonempty substrings,
/// \f$ n ( n + 1 ) / 2 - \sum_r \mathit{LCP}[ r ] \f$
[[nodiscard]] inline std::uint64_t distinctSubstrings(
                                                   const SuffixArray & SA );

/// a longest substring common to at least \f$ k \f$ of the strings
/// \f$ S \f$
///
/// In pseudocode (beyond the book, as in Abouelhoda, Kurtz and Ohlebusch),
/// \f$ \mathit{owner}( i ) \f$ the string to which the place \f$ i \f$ of
/// the join belongs:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{longestCommonSubstring}( S , k ) \cr
/// 2 & \quad \textbf{if}\ k = 1\ \textbf{then return}\ \text{a longest }
///     \text{string of } S \cr
/// 3 & \quad X \gets S_0 \, \$_0 \, S_1 \, \$_1 \cdots S_{m-1} \,
///     \$_{m-1}\quad \triangleright\ \text{distinct separators, smaller than }
///     \text{the bytes} \cr
/// 4 & \quad A \gets \text{the suffix array of } X;\ \mathit{best} \gets
///     0;\ \mathit{lo} \gets 0 \cr
/// 5 & \quad \textbf{for}\ \mathit{hi} \gets 0\ \textbf{to}\ |X| - 1
///     \ \textbf{do} \cr
/// 6 & \qquad \text{add } \mathit{owner}( A[ \mathit{hi} ] ) \text{ to the }
///     \text{counts of the window} \cr
/// 7 & \qquad \textbf{while}\  \text{the window } [ \mathit{lo} ,
///     \mathit{hi} ] \text{ has suffixes of } k \text{ distinct strings}
///     \ \textbf{do} \cr
/// 8 & \qquad\quad \mathit{best} \gets \max\{ \mathit{best} ,
///     A.\mathrm{lce}( A[ \mathit{lo} ] , A[ \mathit{hi} ] ) \} \cr
/// 9 & \qquad\quad \text{remove } \mathit{owner}( A[ \mathit{lo} ] )
///     \text{ from the counts};\ \mathit{lo} \gets \mathit{lo} + 1 \cr
/// 10 & \quad \textbf{return}\ \text{the prefix of length } \mathit{best}
///     \text{ of the suffix that gave it}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le k \le |S| \f$
/// \post a substring of at least \f$ k \f$ of the strings (an audit)
[[nodiscard]] inline std::string longestCommonSubstring(
                       const std::vector< std::string > & S , std::size_t k );

/// the maximal repeats, as the place of an occurrence and the length
///
/// In pseudocode (section 7.12 of Gusfield, beyond the book),
/// \f$ \mathit{left}( u ) \f$ the symbol before all the occurrences below
/// \f$ u \f$, or diverse:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maximalRepeats}( \mathit{ST} ) \cr
/// 2 & \quad \textbf{foreach}\ \text{node } u \text{, the children}
///     \text{ before the parents}\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ u \text{ is the leaf of the suffix } i
///     \ \textbf{then}\ \mathit{left}( u ) \gets T[ i - 1 ] \text{, or the }
///     \text{terminator if } i = 0 \cr
/// 4 & \qquad \textbf{else if}\  \text{its children have the same }
///     \mathit{left}\ \textbf{then}\ \mathit{left}( u ) \gets \text{that }
///     \text{symbol} \cr
/// 5 & \qquad \textbf{else}\ \mathit{left}( u ) \gets \text{diverse} \cr
/// 6 & \quad \textbf{return}\ \text{the internal nodes but the root with }
///     \mathit{left}( u ) = \text{diverse, in preorder}
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::pair< std::size_t , std::size_t > >
                                      maximalRepeats( const SuffixTree & ST );

/// whether an occurrence of \f$ P \f$ and one of \f$ Q \f$ start at most
/// \f$ k \f$ places apart
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{proximity}( \mathit{SA} , P , Q , k ) \cr
/// 2 & \quad O \gets \text{the occurrences of } P \text{ and of } Q
///     \text{, each marked with its pattern, sorted by place} \cr
/// 3 & \quad \textbf{return}\  \text{whether two adjacent items of } O
///     \text{ of different patterns are at most } k \text{ apart}
/// \end{array}}
/// \f]
[[nodiscard]] inline bool proximity( const SuffixArray & SA ,
                                     std::string_view P , std::string_view Q ,
                                     std::size_t k );

/// the places where \f$ P \f$ occurs in \f$ T \f$ with at most \f$ k \f$
/// mismatches
///
/// In pseudocode (after Landau and Vishkin, beyond the book), the extensions
/// computed on the suffix array of \f$ T \# P \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kMismatches}( T , P , k ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - p\ \textbf{do} \cr
/// 3 & \qquad e \gets 0;\ j \gets 0 \cr
/// 4 & \qquad \textbf{while}\ e \le k\ \textbf{and}\ j < p\ \textbf{do} \cr
/// 5 & \qquad\quad j \gets j + \mathrm{lce}( T[ i + j .. ] , P[ j .. ] ) \cr
/// 6 & \qquad\quad \textbf{if}\ j < p\ \textbf{then}\ e \gets e + 1;\ j
///     \gets j + 1\quad \triangleright\ \text{a mismatch, skipped} \cr
/// 7 & \qquad \textbf{if}\ e \le k\ \textbf{then}\ \text{report } i
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::size_t > kMismatches(
                                        std::string_view T ,
                                        std::string_view P , std::size_t k );

/// the parsing of Ziv and Lempel of \f$ T \f$, the copies from the
/// farthest occurrence, by the suffix tree
///
/// In pseudocode (beyond the book),
/// \f$ \mathit{least}( u ) \f$ the smallest suffix of a leaf below
/// \f$ u \f$, computed by one visit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz77ByTree}( T ) \cr
/// 2 & \quad i \gets 0 \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathit{root} \cr
/// 5 & \qquad \textbf{loop} \cr
/// 6 & \qquad\quad c \gets \text{the child of } u \text{ along the suffix }
///     i;\ \textbf{if}\ \mathit{least}( c ) \ge i\ \textbf{then}
///     \ \text{exit the loop} \cr
/// 7 & \qquad\quad u \gets c \cr
/// 8 & \qquad \ell \gets \mathit{depth}( u );\ d \gets i -
///     \mathit{least}( u ) \text{ if } \ell > 0 \text{, else 0} \cr
/// 9 & \qquad \text{append } ( d , \ell , T[ i + \ell ] ) \text{, without}
///     \text{ the symbol if } i + \ell = n;\ i \gets i + \ell + 1
/// \end{array}}
/// \f]
/// \post `decodeCopyPhrases()` gives \f$ T \f$ back (an audit)
[[nodiscard]] inline std::vector< CopyPhrase > lz77ByTree(
                                                        std::string_view T );

/// the text of a parsing, each phrase copying its symbols one at a time,
/// so that a copy may overlap the symbols it writes
/// \pre each distance is at most the length decoded before it, and
///      positive if its length is
[[nodiscard]] inline std::string decodeCopyPhrases(
                                     const std::vector< CopyPhrase > & Z );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SuffixApplications.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuffixApplications.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SuffixApplications.h ----------------------*/
/*--------------------------------------------------------------------------*/
