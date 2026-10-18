/*--------------------------------------------------------------------------*/
/*--------------------------- File Palindromes.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Three classic problems on the palindromes of a sequence s of n items.
 *
 * - longestPalindromicSubstring(): the longest contiguous palindrome, by the
 *   algorithm of Manacher in \f$ \Theta( n ) \f$ time and space. The
 *   dynamic programming of the textbook fills the table P[ i ][ j ], whether
 *   s[ i .. j ] is a palindrome, by increasing length, in \f$ \Theta( n^2 )
 *   \f$ time and space, and the expansion about each of the 2 n - 1 centres
 *   takes \f$ O( n^2 ) \f$ time and O( 1 ) space; Manacher keeps the
 *   rightmost palindrome found so far and starts each new centre from the
 *   radius of its mirror inside it, so that the right end only moves
 *   forward. The suffix tree of s and of its reverse gives the same answer
 *   in linear time with a much larger constant [see SuffixTree.h].
 * - longestPalindromicSubsequence(): the longest palindrome that is a
 *   subsequence of s: DP[ i ][ j ], its length for s[ i .. j ], is 1 if
 *   i = j, DP[ i + 1 ][ j - 1 ] + 2 if s_i = s_j, the larger of
 *   DP[ i + 1 ][ j ] and DP[ i ][ j - 1 ] otherwise, \f$ \Theta( n^2 ) \f$
 *   time and space, the palindrome read back from both ends. It is the
 *   longest common subsequence of s and of its reverse [see
 *   LongestCommonSubsequence.h], so the bit-parallel and the sparse
 *   algorithms of the LCS apply to it too.
 * - palindromicPartition(): the fewest palindromes into which s can be cut:
 *   D[ j ], the fewest for s[ 0 .. j ), is the least D[ i ] + 1 over the
 *   palindromes s[ i .. j ), D[ 0 ] = 0. The textbook builds the table of
 *   the palindromes first, in \f$ \Theta( n^2 ) \f$ space; here each
 *   palindrome s[ i .. j ) relaxes D[ j ] while it is found by the expansion
 *   about its centre, the centres taken left to right, so that D[ i ] is
 *   final when it is used: \f$ O( n^2 ) \f$ time and O( n ) space. The
 *   factorization into the fewest palindromes takes \f$ O( n \log n ) \f$
 *   time with the palindromic series of the eertree (Fici, Gagie,
 *   Karkkainen and Kempa; Rubinchik and Shur).
 *
 * Each function takes any sequence S with size(), operator[], an iterator
 * constructor and the comparison of its items; the overloads on std::string
 * also take string literals.
 *
 * The contracts [see Contract.h]: the result is a palindrome; the
 * subsequence is one of s as long as the LCS of s and its reverse, and the
 * pieces of the partition give s back (both audits). The functions are
 * defined in Palindromes.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see G. Manacher, A new linear-time "on-line" algorithm for finding the
 *      smallest initial palindrome of a string, Journal of the ACM 22(3),
 *      346-351, 1975
 * \see G. Fici, T. Gagie, J. Karkkainen, D. Kempa, A subquadratic algorithm
 *      for minimum palindromic factorization, Journal of Discrete Algorithms
 *      28, 41-48, 2014
 * \see M. Rubinchik, A. M. Shur, EERTREE: an efficient data structure for
 *      processing palindromes in strings, European Journal of Combinatorics
 *      68, 249-265, 2018
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
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// where a longest palindromic substring of s starts and its length, the
/// leftmost one among the longest; ( 0 , 0 ) for the empty s
/// \post s[ start .. start + length ) is a palindrome
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::pair< std::size_t , std::size_t > longestPalindrome(
                                                              const S & s );

/// the leftmost of the longest palindromic substrings of s
/// \post a palindrome occurring in s, empty only if s is
template< class S > requires std::is_class_v< S >
[[nodiscard]] S longestPalindromicSubstring( const S & s );

/// longestPalindromicSubstring() on a std::string (or a literal)
[[nodiscard]] inline std::string longestPalindromicSubstring(
                                                     const std::string & s );

/// a longest palindromic subsequence of s
/// \post a palindrome, a subsequence of s as long as lcs( s , reverse( s ) )
///       (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] S longestPalindromicSubsequence( const S & s );

/// longestPalindromicSubsequence() on a std::string (or a literal)
[[nodiscard]] inline std::string longestPalindromicSubsequence(
                                                     const std::string & s );

/// the fewest palindromes into which s can be cut, in their order; none
/// for the empty s
/// \post every piece a palindrome, and the pieces joined give s (an audit)
template< class S > requires std::is_class_v< S >
[[nodiscard]] std::vector< S > palindromicPartition( const S & s );

/// palindromicPartition() on a std::string (or a literal)
[[nodiscard]] inline std::vector< std::string > palindromicPartition(
                                                     const std::string & s );

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
