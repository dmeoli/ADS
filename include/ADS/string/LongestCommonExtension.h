/*--------------------------------------------------------------------------*/
/*-------------------- File LongestCommonExtension.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered longest common extensions of a text, beyond the book: the
 * lce( i , j ) of the suffixes i and j of T, the length of their longest
 * common prefix, which the benchmark of the suffix arrays compares with
 * the lce() of SuffixArray.h (the range minimum of Bender and
 * Farach-Colton), with a range minimum of sdsl-lite and with the direct
 * comparison of the two suffixes, while SuffixArray.h stays the textbook.
 *
 * With SA the suffix array of T, \f$ SA^{-1} \f$ its inverse and LCP its
 * LCP array (LCP[ r ] the lcp of the suffixes SA[ r ] and SA[ r + 1 ]),
 * \f[
 *   \mathrm{lce}( i , j ) = \min_{ a \le r < b } LCP[ r ] , \qquad
 *   a = \min( SA^{-1}[ i ] , SA^{-1}[ j ] ) , \;
 *   b = \max( SA^{-1}[ i ] , SA^{-1}[ j ] ) ,
 * \f]
 * a range minimum query; LceIndex builds SA by suffixArray() and LCP by
 * lcpArrayPhi() [see SuffixSorting.h], and answers in two stages:
 *
 * - the first 64 symbols of the two suffixes are compared directly, 32 at
 *   a time with AVX2 (8 else), since on most texts most extensions are
 *   short, and the direct comparison, which reads two lines of the text,
 *   is then faster than any query that reads the inverse and the LCP
 *   array at random (Ilie, Navarro and Tinta found it the fastest on the
 *   texts with few long repeats); the first chunk is inlined where lce()
 *   is called, the rest is out of line;
 * - the longer ones go to the range minimum on LCP, cut into blocks of 16
 *   entries, each one line of the cache. For the entry r of a block that
 *   starts at s a word \f$ M_r \f$ of 16 bits has the bit k set if
 *   \f$ LCP[ s + k ] < LCP[ s + k' ] \f$ for every \f$ k < k' \le r - s
 *   \f$, i.e., if s + k is on the stack of the suffix minima of the block
 *   up to r; the minimum of \f$ LCP[ s + a .. r ] \f$ is then
 *   \f$ LCP[ s + a + \mathrm{ctz}( M_r \gg a ) ] \f$, one shift and one
 *   count of trailing zeros (the words of Alstrup, Gavoille, Kaplan and
 *   Rauhe). The blocks form groups of 64, each with the sparse table of
 *   the minima of its blocks, \f$ \min_{ x \le d < x + 2^L } \f$ for
 *   \f$ L \le 6 \f$, and the minima of the groups are in a sparse table of
 *   their own; the blocks strictly between those of a and b - 1 are
 *   covered by two overlapping ranges of one group, or by those of two
 *   partial groups and of the groups between them. Every place that a
 *   query reads is known once the two ranks are, and the lines of the two
 *   blocks are requested with their words, so that a query waits for two
 *   reads of the memory in a row: the ranks, then all the rest. Building
 *   it costs O( n ) but for the table of the groups, \f$ O( ( n / 1024 )
 *   \log( n / 1024 ) ) \f$, and a query O( 1 ).
 *
 * The space is 4 n bytes for each of \f$ SA^{-1} \f$ and LCP (on 32 bits),
 * 2 n for the words, 1.75 n for the tables of the groups and a vanishing
 * share for the table of the groups, against the 2 n + o( n ) bits of the
 * succinct range minima of Fischer and Heun, whose practical form
 * (Baumstark, Gog, Heuer and Labeit) pays for its space with a select on
 * a balanced-parentheses sequence at each query.
 *
 * The contracts [see Contract.h]: a query asks for i , j <= n (n gives 0),
 * and its answer is the lcp of the two suffixes (an audit, linear in it).
 * The functions are defined in LongestCommonExtension.tpp.
 *
 * \see L. Ilie, G. Navarro, L. Tinta, The longest common extension problem
 *      revisited and applications to approximate string searching, Journal
 *      of Discrete Algorithms 8(4), 418-428, 2010,
 *      doi:10.1016/j.jda.2010.08.004
 * \see S. Alstrup, C. Gavoille, H. Kaplan, T. Rauhe, Nearest common
 *      ancestors: a survey and a new distributed algorithm, Proceedings of
 *      the 14th ACM Symposium on Parallel Algorithms and Architectures
 *      (SPAA), 258-264, 2002, doi:10.1145/564870.564914
 * \see J. Fischer, V. Heun, Space-efficient preprocessing schemes for range
 *      minimum queries on static arrays, SIAM Journal on Computing 40(2),
 *      465-492, 2011, doi:10.1137/090779759
 * \see N. Baumstark, S. Gog, T. Heuer, J. Labeit, Practical range minimum
 *      queries revisited, Proceedings of the 16th International Symposium
 *      on Experimental Algorithms (SEA), LIPIcs 75, 12:1-12:16, 2017,
 *      doi:10.4230/LIPIcs.SEA.2017.12
 * \see M. A. Bender, M. Farach-Colton, The LCA problem revisited,
 *      Proceedings of the 4th Latin American Symposium on Theoretical
 *      Informatics (LATIN), LNCS 1776, 88-94, 2000,
 *      doi:10.1007/10719839_9
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

#ifndef __ADS_LongestCommonExtension
 #define __ADS_LongestCommonExtension
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/SuffixSorting.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- LceIndex ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the longest common extensions of the suffixes of a text, O( 1 ) each,
/// on integers I (32 or 64 bits, unsigned)
template< class I = std::uint32_t >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
class LceIndex {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

 /// the index of T (a copy of it), O( n ) time but for the sparse table
 /// \pre T.size() < 2^31 if I has 32 bits
 explicit LceIndex( std::string_view T );

 /// the length of the longest common prefix of the suffixes i and j
 /// \pre i <= size() and j <= size()
 /// \post the lcp of the two suffixes (an audit)
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// the length of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_T.size() ); }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

 /// lce( i , j ) past the first chunk of symbols, which match
#if defined( __GNUC__ ) || defined( __clang__ )
 [[gnu::noinline]]
#endif
 [[nodiscard]] std::size_t longer( std::size_t i , std::size_t j ,
                                   std::size_t far ) const;

 /// the minimum of LCP[ a .. c ], a <= c
 [[nodiscard]] I minimum( std::size_t a , std::size_t c ) const;

 std::string f_T;                    ///< the text, 8 bytes of 0 after it
 std::size_t f_n = 0;                ///< its length
 std::vector< I > f_rank;            ///< the inverse of SA
 std::vector< I > f_line;            ///< LCP, with room to align it
 std::size_t f_shift = 0;            ///< where it starts at 64 bytes
 std::vector< std::uint16_t > f_word;  ///< the stack of each entry
 std::vector< I > f_inner;           ///< the table of each group
 std::vector< I > f_top;             ///< the table of the groups
 std::size_t f_blocks = 0;           ///< the number of the blocks
 std::size_t f_groups = 0;           ///< and of the groups

 };  // end( class LceIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/LongestCommonExtension.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LongestCommonExtension.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File LongestCommonExtension.h -------------------*/
/*--------------------------------------------------------------------------*/
