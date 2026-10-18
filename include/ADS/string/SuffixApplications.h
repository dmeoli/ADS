/*--------------------------------------------------------------------------*/
/*----------------------- File SuffixApplications.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Problems on strings solved by a suffix array [see SuffixArray.h] or a
 * suffix tree [see SuffixTree.h], beyond the book (section 10.4 of the
 * notes and their classical companions). A substring that occurs twice is
 * a common prefix of two suffixes, and the suffixes that share a prefix of
 * length L are a range of SA whose LCP entries are all at least L: hence a
 * scan of LCP, or a visit of the internal nodes of the suffix tree, answers
 * the questions on the repeats in O( n ) time.
 *
 * - longestRepeatedSubstring(): the largest entry of LCP, O( n ), against
 *   the \f$ O( n^2 ) \f$ of the dynamic programming of a string against
 *   itself;
 * - repeatedSubstring( SA , L , C ), the query of text mining of section
 *   10.4.3: a substring of length L that occurs at least C times is a run
 *   of at least C - 1 entries of LCP that are at least L, one scan, O( n ),
 *   against the \f$ O( n^2 L ) \f$ of counting every substring;
 * - distinctSubstrings(): each suffix SA[ r ] adds the n - SA[ r ] prefixes
 *   less the LCP[ r - 1 ] it shares with the previous one, hence
 *   \f$ n ( n + 1 ) / 2 - \sum_r LCP[ r ] \f$, O( n );
 * - longestCommonSubstring( S , k ): the longest substring common to at
 *   least k of the m strings of S, on the suffix array of their join with m
 *   distinct separators (an integer alphabet): a window of ranks that
 *   covers suffixes of k distinct strings, slid by two pointers, whose
 *   common prefix is the lce() of its two ends, \f$ O( N ) \f$ for N the
 *   total length, against the \f$ O( |P| |T| ) \f$ of the dynamic
 *   programming for two strings [see LongestCommonSubsequence.h] and the
 *   generalized suffix tree with the colors of Hui for any k;
 * - maximalRepeats(): the maximal repeats, the substrings that occur twice
 *   and cannot be extended to the right (an internal node, Property 10.8)
 *   nor to the left (the symbols before their occurrences are not all the
 *   same, or one occurrence starts the text): the internal nodes whose
 *   leaves are left-diverse, by one visit from the leaves up, O( n ), as in
 *   Gusfield;
 * - proximity( SA , P , Q , k ), the proximity search of section 10.4.3:
 *   the occurrences of P and of Q, sorted together, have an occurrence of
 *   each at distance at most k exactly when two adjacent ones of different
 *   patterns do, \f$ O( |P| + |Q| + occ \log occ ) \f$;
 * - kMismatches( T , P , k ), Algorithm 10.4 (section 10.4.1): the places
 *   where P occurs with at most k substitutions, each checked by at most k
 *   + 1 longest common extensions of a suffix of T and a suffix of P, both
 *   suffixes of T#P, by the range minima of its LCP in O( 1 ): \f$ O( n k
 *   ) \f$ after O( n + p ) preprocessing, against the O( n p ) of trying
 *   every place, as in Landau and Vishkin;
 * - lz77ByTree( T ), the parsing of Ziv and Lempel by the suffix tree of
 *   section 10.4.2: the phrase at i copies the longest prefix of the suffix
 *   i that starts earlier, found by walking down along the suffix i while
 *   the least leaf below the next node is smaller than i, the least leaves
 *   computed by one visit; the copy is taken from the farthest occurrence,
 *   the least leaf, \f$ O( n \log \sigma ) \f$ in all (Theorem 10.13,
 *   which states O( n ) with perfect hashing at the nodes). The phrase is
 *   the triple ( distance , length , next symbol ), the last one without
 *   the symbol when the copy reaches the end of T; decodeCopyPhrases()
 *   inverts it.
 *
 * The contracts [see Contract.h]: k is at least 1 and at most the number of
 * strings for longestCommonSubstring(), C at least 1; the parsing decodes
 * to the text (an audit). The functions are defined in
 * SuffixApplications.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10
 * \see D. Gusfield, Algorithms on Strings, Trees, and Sequences, Cambridge
 *      University Press, 1997, doi:10.1017/CBO9780511574931, sections
 *      7.4-7.12
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
 *         Universita' di Pisa \n
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

/// a phrase of the parsing of Ziv and Lempel: copy length symbols from
/// distance places back, then append next, if any
struct CopyPhrase {
 std::size_t distance = 0;
 std::size_t length = 0;
 std::optional< char > next;
 friend bool operator==( const CopyPhrase & , const CopyPhrase & ) = default;
 };

/// the place and the length of a longest substring that occurs twice
/// (0 and 0 if none)
[[nodiscard]] inline std::pair< std::size_t , std::size_t >
                         longestRepeatedSubstring( const SuffixArray & SA );

/// the place of a substring of length L that occurs at least C times, if
/// any
/// \pre C >= 1
[[nodiscard]] inline std::optional< std::size_t > repeatedSubstring(
                  const SuffixArray & SA , std::size_t L , std::size_t C );

/// the number of distinct nonempty substrings
[[nodiscard]] inline std::uint64_t distinctSubstrings(
                                                   const SuffixArray & SA );

/// a longest substring common to at least k of the strings S
/// \pre 1 <= k <= S.size()
/// \post a substring of at least k of the strings (an audit)
[[nodiscard]] inline std::string longestCommonSubstring(
                       const std::vector< std::string > & S , std::size_t k );

/// the maximal repeats, as the place of an occurrence and the length
[[nodiscard]] inline std::vector< std::pair< std::size_t , std::size_t > >
                                      maximalRepeats( const SuffixTree & ST );

/// an occurrence of P and one of Q start at most k places apart
[[nodiscard]] inline bool proximity( const SuffixArray & SA ,
                                     std::string_view P , std::string_view Q ,
                                     std::size_t k );

/// the places where P occurs in T with at most k mismatches
[[nodiscard]] inline std::vector< std::size_t > kMismatches(
                                        std::string_view T ,
                                        std::string_view P , std::size_t k );

/// the parsing of Ziv and Lempel of T, the copies from the farthest
/// occurrence, by the suffix tree
/// \post decodeCopyPhrases() gives T back (an audit)
[[nodiscard]] inline std::vector< CopyPhrase > lz77ByTree(
                                                        std::string_view T );

/// the text of a parsing
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
