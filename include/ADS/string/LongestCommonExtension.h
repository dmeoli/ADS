/*--------------------------------------------------------------------------*/
/*-------------------- File LongestCommonExtension.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered longest common extensions of a text, beyond the book: the
 * value \f$ \mathrm{lce}( i , j ) \f$ of the suffixes \f$ i \f$ and
 * \f$ j \f$ of \f$ T \f$, the length of their longest common prefix, which
 * the benchmark of the suffix arrays compares with the `lce()` of
 * `SuffixArray.h` (the range minimum of Bender and Farach-Colton), with a
 * range minimum of sdsl-lite and with the direct comparison of the two
 * suffixes, while `SuffixArray.h` stays the textbook.
 *
 * With \f$ \mathit{SA} \f$ the suffix array of \f$ T \f$,
 * \f$ \mathit{SA}^{-1} \f$ its inverse and \f$ \mathit{LCP} \f$ its LCP
 * array (\f$ \mathit{LCP}[ r ] \f$ the lcp of the suffixes
 * \f$ \mathit{SA}[ r ] \f$ and \f$ \mathit{SA}[ r + 1 ] \f$),
 * \f[
 *   \mathrm{lce}( i , j ) = \min_{ a \le r < b } \mathit{LCP}[ r ] , \qquad
 *   a = \min( \mathit{SA}^{-1}[ i ] , \mathit{SA}^{-1}[ j ] ) , \;
 *   b = \max( \mathit{SA}^{-1}[ i ] , \mathit{SA}^{-1}[ j ] ) ,
 * \f]
 * a range minimum query; `LceIndex` builds \f$ \mathit{SA} \f$ by
 * `suffixArray()` and \f$ \mathit{LCP} \f$ by `lcpArrayPhi()` [see
 * `SuffixSorting.h`], and answers in two stages:
 *
 * - the first 64 symbols of the two suffixes are compared directly, 32 at
 *   a time with AVX2 (8 else), since on most texts most extensions are
 *   short, and the direct comparison, which reads two lines of the text,
 *   is then faster than any query that reads the inverse and the LCP
 *   array at random (Ilie, Navarro and Tinta found it the fastest on the
 *   texts with few long repeats); the first chunk is inlined where `lce()`
 *   is called, the rest is out of line;
 * - the longer ones go to the range minimum on \f$ \mathit{LCP} \f$, cut
 *   into blocks of 16 entries, each one line of the cache. For the entry
 *   \f$ r \f$ of a block that starts at \f$ s \f$ a word \f$ M_r \f$ of 16
 *   bits has the bit \f$ k \f$ set if
 *   \f$ \mathit{LCP}[ s + k ] < \mathit{LCP}[ s + k' ] \f$ for every
 *   \f$ k < k' \le r - s \f$, i.e., if \f$ s + k \f$ is on the stack of the
 *   suffix minima of the block up to \f$ r \f$; the minimum of
 *   \f$ \mathit{LCP}[ s + a .. r ] \f$ is then
 *   \f$ \mathit{LCP}[ s + a + \mathrm{ctz}( M_r \gg a ) ] \f$, one shift and
 *   one count of trailing zeros (the words of Alstrup, Gavoille, Kaplan and
 *   Rauhe). The blocks form groups of 64, each with the sparse table of the
 *   minima of its blocks, \f$ \min_{ x \le d < x + 2^L } \f$ for
 *   \f$ L \le 6 \f$, and the minima of the groups are in a sparse table of
 *   their own; the blocks strictly between those of \f$ a \f$ and
 *   \f$ b - 1 \f$ are covered by two overlapping ranges of one group, or by
 *   those of two partial groups and of the groups between them. Every place
 *   that a query reads is known once the two ranks are, and the lines of
 *   the two blocks are requested with their words, so that a query waits
 *   for two reads of the memory in a row: the ranks, then all the rest.
 *   Building it costs \f$ O( n ) \f$ but for the table of the groups,
 *   \f$ O( ( n / 1024 ) \log( n / 1024 ) ) \f$, and a query \f$ O( 1 ) \f$.
 *
 * The space is \f$ 4 n \f$ bytes for each of \f$ \mathit{SA}^{-1} \f$ and
 * \f$ \mathit{LCP} \f$ (on 32 bits), \f$ 2 n \f$ for the words,
 * \f$ 1.75 n \f$ for the tables inside the groups and a vanishing share for
 * the table of the groups, against the \f$ 2 n + o( n ) \f$ bits of the
 * succinct range minima of Fischer and Heun, whose practical form
 * (Baumstark, Gog, Heuer and Labeit) pays for its space with a select on a
 * balanced-parentheses sequence at each query.
 *
 * The contracts [see `Contract.h`]: a query asks for \f$ i , j \le n \f$
 * (\f$ n \f$ gives 0), and its answer is the lcp of the two suffixes (an
 * audit, linear in it). The functions are defined in
 * `LongestCommonExtension.tpp`.
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
 *         Università di Pisa \n
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

/// the longest common extensions of the suffixes of a text, \f$ O( 1 ) \f$
/// each, on unsigned integers `I` of 32 or 64 bits
template< class I = std::uint32_t >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
class LceIndex {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

 /// the index of \f$ T \f$ (a copy of it), in \f$ O( n ) \f$ time but for
 /// the sparse table of the groups
 ///
 /// In pseudocode (the words of Alstrup, Gavoille, Kaplan and Rauhe and the
 /// sparse tables, beyond the book), blocks of 16 entries of
 /// \f$ \mathit{LCP} \f$ and groups of 64 blocks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{LceIndex}( T ) \cr
 /// 2 & \quad \mathit{SA} \gets \mathrm{suffixArray}( T );\ \mathit{LCP}
 ///     \gets \mathrm{lcpArrayPhi}( T , \mathit{SA} ) \cr
 /// 3 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ \mathit{SA}^{-1}[ \mathit{SA}[ r ] ] \gets r \cr
 /// 4 & \quad \textbf{foreach}\ \text{block starting at } s\ \textbf{do} \cr
 /// 5 & \qquad M \gets 0 \cr
 /// 6 & \qquad \textbf{foreach}\ \text{entry } r \text{ of the block}
 ///     \ \textbf{do} \cr
 /// 7 & \qquad\quad \textbf{while}\ M \ne 0\ \textbf{and}\ \mathit{LCP}[ s +
 ///     \text{the highest bit of } M ] \ge \mathit{LCP}[ r ]\ \textbf{do} \cr
 /// 8 & \qquad\qquad \text{clear the highest bit of } M \cr
 /// 9 & \qquad\quad \text{set the bit } r - s \text{ of } M;\ M_r \gets M \cr
 /// 10 & \qquad \text{the minimum of the block} \gets \mathit{LCP}[ s +
 ///     \mathrm{ctz}( M ) ] \cr
 /// 11 & \quad \textbf{foreach}\ \text{group}\ \textbf{do}\ \text{the }
 ///     \text{sparse table of the minima of its 64 blocks} \cr
 /// 12 & \quad \text{the sparse table of the minima of the groups}
 /// \end{array}}
 /// \f]
 /// \pre \f$ |T| < 2^{31} \f$ if `I` has 32 bits
 explicit LceIndex( std::string_view T );

 /// the length of the longest common prefix of the suffixes \f$ i \f$ and
 /// \f$ j \f$
 ///
 /// In pseudocode (Ilie, Navarro and Tinta for the direct comparison,
 /// beyond the book), \f$ \mathit{far} = n - \max( i , j ) \f$ the longest
 /// possible extension:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lce}( i , j ) \cr
 /// 2 & \quad \textbf{if}\ i = j\ \textbf{then return}\ n - i \cr
 /// 3 & \quad \textbf{for}\ h \gets 0 \text{ by chunks of 32 (or 8) }
 ///     \textbf{while}\ h < 64\ \textbf{do} \cr
 /// 4 & \qquad d \gets \text{the first mismatch of } T[ i + h .. ]
 ///     \text{ and } T[ j + h .. ] \text{ in the chunk} \cr
 /// 5 & \qquad \textbf{if}\ \text{there is one}\ \textbf{then return}\ \min\{
 ///     h + d , \mathit{far} \} \cr
 /// 6 & \qquad \textbf{if}\ h + \text{the chunk} \ge \mathit{far}
 ///     \ \textbf{then return}\ \mathit{far} \cr
 /// 7 & \quad a \gets \min\{ \mathit{SA}^{-1}[ i ] , \mathit{SA}^{-1}[ j ]
 ///     \};\ b \gets \max\{ \mathit{SA}^{-1}[ i ] , \mathit{SA}^{-1}[ j ] \}
 ///     \cr
 /// 8 & \quad \textbf{return}\ \min\{ \mathit{LCP}[ a .. b - 1 ] \}
 ///     \text{ by the words of the two end blocks and the sparse tables}
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \f$ and \f$ j \f$ are at most `size()`
 /// \post the lcp of the two suffixes (audit)
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// the length \f$ n \f$ of the text
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
