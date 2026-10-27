/*--------------------------------------------------------------------------*/
/*-------------------------- File EditDistance.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered edit distance, longest common subsequence and approximate
 * search, beyond the book: the ones that the benchmark of the distances
 * compares with edlib and rapidfuzz-cpp, while `editDistance()`,
 * `stringMatching()` [see `ApproximateMatching.h`] and `lcs()` [see
 * `LongestCommonSubsequence.h`] stay the dynamic programming of the
 * textbooks, \f$ \Theta( m n ) \f$ time. For a pattern \f$ P \f$ of
 * \f$ m \f$ bytes (the column) and a text \f$ T \f$ of \f$ n \f$ bytes, the
 * table of the distances is
 * \f[
 *   D[ i ][ j ] = \min\{ D[ i - 1 ][ j - 1 ] + [ P[ i - 1 ] \ne T[ j - 1 ] ]
 *                 , D[ i - 1 ][ j ] + 1 , D[ i ][ j - 1 ] + 1 \} ,
 * \f]
 * with \f$ D[ i ][ 0 ] = i \f$ and \f$ D[ 0 ][ j ] = j \f$ for the distance
 * (0 for the search), whose adjacent cells differ by \f$ -1 \f$, 0 or
 * \f$ +1 \f$. The bit vectors of Myers keep a column as the vertical
 * differences \f$ D[ i ][ j ] - D[ i - 1 ][ j ] \f$, \f$ +1 \f$ in
 * \f$ P_v \f$ and \f$ -1 \f$ in \f$ M_v \f$, 64 rows a word (a block), and
 * \f$ \mathit{Eq} = \mathit{Peq}[ T[ j - 1 ] ] \f$, the places of
 * \f$ P \f$ that hold the byte, moves a block to the next column with the
 * horizontal difference \f$ h_{in} \f$ at its top, in the formulation of
 * Hyyrö:
 * \f[
 *   \begin{array}{l}
 *     X_v = \mathit{Eq} \vee M_v , \quad \mathit{Eq}' = \mathit{Eq} \vee
 *     [ h_{in} < 0 ] , \quad
 *     X_h = ( ( ( \mathit{Eq}' \wedge P_v ) + P_v ) \oplus P_v ) \vee
 *     \mathit{Eq}' , \cr
 *     P_h = M_v \vee \neg( X_h \vee P_v ) , \quad M_h = P_v \wedge X_h ,
 *     \quad h_{out} = P_h[ 63 ] - M_h[ 63 ] , \cr
 *     P_h \leftarrow ( P_h \ll 1 ) \vee [ h_{in} > 0 ] , \quad
 *     M_h \leftarrow ( M_h \ll 1 ) \vee [ h_{in} < 0 ] , \quad
 *     P_v \leftarrow M_h \vee \neg( X_v \vee P_h ) , \quad
 *     M_v \leftarrow P_h \wedge X_v ,
 *   \end{array}
 * \f]
 * a dozen operations on words with no branch, so that a column costs
 * \f$ O( \lceil m / 64 \rceil ) \f$ of them.
 *
 * - `levenshtein( a , b )` strips the common prefix and suffix, then puts
 *   the shorter string, of \f$ m \f$ bytes, in the column and scans the
 *   longer one, of \f$ n \f$ bytes: one word if \f$ m \le 64 \f$,
 *   \f$ O( n ) \f$. Otherwise, the diagonal transitions of Landau and
 *   Vishkin (the furthest reaching paths of Myers, as in the wavefront
 *   alignment of Marco-Sola et al.): \f$ F_h( k ) \f$, the furthest row of
 *   the diagonal \f$ k = j - i \f$ reached by \f$ h \f$ edits, is
 *   \f[
 *     F_h( k ) = \mathit{slide}_k\bigl( \max\{ F_{h-1}( k ) + 1 ,
 *                F_{h-1}( k + 1 ) + 1 , F_{h-1}( k - 1 ) \} \bigr) ,
 *   \f]
 *   \f$ \mathit{slide}_k( i ) \f$ the row reached from \f$ i \f$ along the
 *   matches of the diagonal (compared 8 bytes at a time), and the distance
 *   is the least \f$ d \f$ with \f$ F_d( n - m ) = m \f$,
 *   \f$ O( n + d^2 ) \f$ expected time on random strings and
 *   \f$ O( d ) \f$ space. They are used for \f$ d \f$ up to
 *   \f$ n / 48 + 128 \f$ (if \f$ m > 128 \f$), where they cost about as
 *   much as the blocks, and give up once the rows \f$ r \f$ reached by
 *   \f$ h = 32 , 64 , 128 , \ldots \f$ edits suggest a distance
 *   \f$ h m / r \f$ above twice as much. Then the blocks, restricted to the
 *   diagonal band of Ukkonen of the cells that a path of cost at most
 *   \f$ k \f$ can cross, \f$ i - j \in [ -k , k - ( n - m ) ] \f$, and in
 *   it, as in edlib, to the blocks that may still lead to the end within
 *   \f$ k \f$: a cell \f$ ( i , j ) \f$ costs at least
 *   \f$ D[ i ][ j ] + | ( m - i ) - ( n - j ) | \f$ to the end, and a cell
 *   of a block at least the bottom of the block minus its distance from it,
 *   so that the first and the last block are dropped when the least of
 *   these bounds exceeds \f$ k \f$; the block below the last one enters,
 *   all \f$ +1 \f$, when the bottom of the last is at most \f$ k + 1 \f$,
 *   since the cells at most \f$ k \f$ grow by at most one row per column.
 *   The cells outside the band are taken as larger, which overestimates
 *   only cells that no path of cost \f$ k \f$ crosses. The bound \f$ k \f$
 *   starts from the guess of the diagonals and a quarter more, and doubles
 *   until the distance is at most \f$ k \f$, \f$ O( n d / 64 ) \f$ for
 *   distance \f$ d \f$ in the usual cases. Once the bands have computed a
 *   quarter of the \f$ n \lceil m / 64 \rceil \f$ blocks of the table, or
 *   span half of the column, all the blocks are computed instead, four
 *   columns at a time: the block \f$ b \f$ of the column \f$ j + q \f$
 *   needs only the block \f$ b - 1 \f$ of the same column and the block
 *   \f$ b \f$ of the column before, so that the blocks \f$ i - q \f$ of the
 *   columns \f$ j + q \f$, \f$ q < 4 \f$, are independent, the four lanes
 *   of an AVX2 vector (the bands of 8 blocks or more go the same way).
 *   `levenshtein( a , b , k )` does the same with \f$ k \f$ fixed (the
 *   diagonals up to \f$ k \f$, or the band of \f$ k \f$) and gives
 *   \f$ k + 1 \f$ for any larger distance.
 * - `lcsLength( a , b )` is the bit-parallel LCS of Allison and Dix in the
 *   form of Crochemore, Iliopoulos, Pinzon and Reid: with \f$ V = 1^m \f$
 *   at first and \f$ M = \mathit{Peq}[ b[ j ] ] \f$,
 *   \f[
 *     V \leftarrow ( V + ( V \wedge M ) ) \vee ( V \wedge \neg M ) ,
 *   \f]
 *   the addition carried between the blocks, and the length is the number
 *   of 0 among the \f$ m \f$ bits of \f$ V \f$,
 *   \f$ O( n \lceil m / 64 \rceil ) \f$ after the common prefix and suffix
 *   are stripped.
 * - `approximateMatches( T , P , k )` gives the ends \f$ j \f$ (1 to
 *   \f$ n \f$) of the substrings of \f$ T \f$ at edit distance at most
 *   \f$ k \f$ from \f$ P \f$, \f$ D[ m ][ j ] \le k \f$ with the top row 0,
 *   with their distances, by the search of Myers: only the blocks up to the
 *   last one with a cell at most \f$ k + 1 \f$ at its bottom are computed
 *   (the cut-off of Ukkonen), \f$ O( n k / 64 ) \f$ expected on random
 *   texts.
 *
 * The bits \f$ \mathit{Peq} \f$ take \f$ 256 \lceil m / 64 \rceil \f$ words
 * (in the object up to 4 blocks, with only the bytes of the two strings
 * cleared beyond). The contracts [see `Contract.h`]: the results equal
 * those of the dynamic programming, `editDistance()`, `lcs()` and the last
 * row of `matchingTable()` (audits, \f$ O( m n ) \f$ time). The functions
 * are defined in `EditDistance.tpp`.
 *
 * \see V. I. Levenshtein, Binary codes capable of correcting deletions,
 *      insertions, and reversals, Soviet Physics Doklady 10(8), 707-710,
 *      1966
 * \see G. Myers, A fast bit-vector algorithm for approximate string
 *      matching based on dynamic programming, Journal of the ACM 46(3),
 *      395-415, 1999, doi:10.1145/316542.316550
 * \see H. Hyyrö, A bit-vector algorithm for computing Levenshtein and
 *      Damerau edit distances, Nordic Journal of Computing 10(1), 29-39,
 *      2003, https://cs.helsinki.fi/njc/Contents_of_Vol10.html
 * \see G. M. Landau, U. Vishkin, Fast parallel and serial approximate
 *      string matching, Journal of Algorithms 10(2), 157-169, 1989,
 *      doi:10.1016/0196-6774(89)90010-2
 * \see E. W. Myers, An \f$ O( N D ) \f$ difference algorithm and its
 *      variations, Algorithmica 1(1-4), 251-266, 1986,
 *      doi:10.1007/BF01840446
 * \see S. Marco-Sola, J. C. Moure, M. Moreto, A. Espinosa, Fast gap-affine
 *      pairwise alignment using the wavefront algorithm, Bioinformatics
 *      37(4), 456-463, 2021, doi:10.1093/bioinformatics/btaa777
 * \see E. Ukkonen, Algorithms for approximate string matching, Information
 *      and Control 64(1-3), 100-118, 1985,
 *      doi:10.1016/S0019-9958(85)80046-2
 * \see M. Šošić, M. Šikić, Edlib: a C/C++ library for fast, exact sequence
 *      alignment using edit distance, Bioinformatics 33(9), 1394-1395,
 *      2017, doi:10.1093/bioinformatics/btw753
 * \see L. Allison, T. I. Dix, A bit-string longest-common-subsequence
 *      algorithm, Information Processing Letters 23(5), 305-310, 1986,
 *      doi:10.1016/0020-0190(86)90091-8
 * \see M. Crochemore, C. S. Iliopoulos, Y. J. Pinzon, J. F. Reid, A fast
 *      and practical bit-vector algorithm for the longest common
 *      subsequence problem, Information Processing Letters 80(6), 279-285,
 *      2001, doi:10.1016/S0020-0190(01)00182-X
 * \see E. Ukkonen, Finding approximate patterns in strings, Journal of
 *      Algorithms 6(1), 132-137, 1985, doi:10.1016/0196-6774(85)90023-9
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

#ifndef __ADS_EditDistance
 #define __ADS_EditDistance
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the edit distance of Levenshtein of \f$ a \f$ and \f$ b \f$
///
/// In pseudocode (the diagonal transitions of Landau and Vishkin and the
/// bit vectors of Myers in the band of Ukkonen, beyond the book), \f$ a \f$
/// the shorter string after the common prefix and suffix are stripped:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{levenshtein}( a , b ) \cr
/// 2 & \quad \text{strip the common prefix and suffix};\ m \gets |a|;\ n
///     \gets |b|\quad \triangleright\ m \le n \cr
/// 3 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ n \cr
/// 4 & \quad \textbf{if}\ m \le 64\ \textbf{then return}\ \text{the distance }
///     \text{by one word of Myers per column} \cr
/// 5 & \quad \textbf{if}\ m > 128\ \textbf{then} \cr
/// 6 & \qquad \textbf{for}\ h \gets 0 , 1 , \ldots \text{ up to } n / 48 +
///     128\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{foreach}\ \text{diagonal } k\ \textbf{do}
///     \ F_h( k ) \gets \mathit{slide}_k( \max\{ F_{h-1}( k ) + 1 , F_{h-1}( k
///     + 1 ) + 1 , F_{h-1}( k - 1 ) \} ) \cr
/// 8 & \qquad\quad \textbf{if}\ F_h( n - m ) = m\ \textbf{then return}\ h \cr
/// 9 & \qquad\quad \textbf{if}\ h \in \{ 32 , 64 , \ldots \} \text{ and the }
///     \text{guess } h m / r \text{ is too large}\ \textbf{then}
///     \ \text{give up} \cr
/// 10 & \quad t \gets \max\{ n - m , 64 , \text{the guess and a quarter} \}
///     \cr
/// 11 & \quad \textbf{loop} \cr
/// 12 & \qquad \textbf{if}\  \text{the band of } t \text{ spans half the }
///     \text{column or a quarter of the table is spent}\ \textbf{then} \cr
/// 13 & \qquad\quad \textbf{return}\ \text{the distance by all the blocks, }
///     \text{four columns at a time} \cr
/// 14 & \qquad r \gets \text{the distance by the blocks in the band }
///     i - j \in [ -t , t - ( n - m ) ] \cr
/// 15 & \qquad \textbf{if}\ r \le t\ \textbf{then return}\ r \cr
/// 16 & \qquad t \gets 2 t
/// \end{array}}
/// \f]
/// \post `editDistance( a , b )` (an audit)
[[nodiscard]] inline std::size_t levenshtein( std::string_view a ,
                                              std::string_view b );

/// the edit distance of \f$ a \f$ and \f$ b \f$ if at most \f$ k \f$,
/// \f$ k + 1 \f$ otherwise
///
/// In pseudocode (beyond the book), the steps of `levenshtein()` above
/// with \f$ k \f$ in place of the bounds it guesses:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{levenshtein}( a , b , k ) \cr
/// 2 & \quad \text{strip the common prefix and suffix};\ m \gets |a|;\ n
///     \gets |b|\quad \triangleright\ m \le n \cr
/// 3 & \quad \textbf{if}\ n - m > k\ \textbf{then return}\ k + 1 \cr
/// 4 & \quad \textbf{if}\ m = 0\ \textbf{then return}\ n \cr
/// 5 & \quad \textbf{if}\ m \le 64\ \textbf{then return}\ \min\{ \text{the }
///     \text{distance by one word} , k + 1 \} \cr
/// 6 & \quad \textbf{if}\ m > 128 \text{ and } k \le n / 48 + 128
///     \ \textbf{then return}\ \min\{ \text{the diagonals up to } k , k + 1
///     \} \cr
/// 7 & \quad \textbf{return}\ \text{the distance by the blocks in the band }
///     \text{of } k \text{ (all the blocks if it is wide), or } k + 1
/// \end{array}}
/// \f]
/// \post \f$ \min\{ \mathrm{editDistance}( a , b ) , k + 1 \} \f$ (an
///       audit)
[[nodiscard]] inline std::size_t levenshtein( std::string_view a ,
                                              std::string_view b ,
                                              std::size_t k );

/// the length of a longest common subsequence of \f$ a \f$ and \f$ b \f$
///
/// In pseudocode (the algorithm of Allison and Dix in the form of
/// Crochemore et al., beyond the book), \f$ a \f$ the shorter string, of
/// \f$ m \f$ bytes, in blocks of 64 bits:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcsLength}( a , b ) \cr
/// 2 & \quad c \gets \text{the length of the common prefix and suffix,}
///     \text{ stripped} \cr
/// 3 & \quad V \gets 1^m \cr
/// 4 & \quad \textbf{foreach}\ \text{byte } x \text{ of } b\ \textbf{do} \cr
/// 5 & \qquad M \gets \mathit{Peq}[ x ];\ V \gets ( V + ( V \wedge M ) )
///     \vee ( V \wedge \neg M ) \cr
/// 6 & \quad \textbf{return}\ c + \text{the number of 0 among the } m
///     \text{ bits of } V
/// \end{array}}
/// \f]
/// \post the length of `lcs( a , b )` (an audit)
[[nodiscard]] inline std::size_t lcsLength( std::string_view a ,
                                            std::string_view b );

/// the ends \f$ j \f$, from 1 to \f$ n \f$, of the substrings of \f$ T \f$
/// at edit distance at most \f$ k \f$ from \f$ P \f$, in increasing order,
/// with their distances
///
/// In pseudocode (the search of Myers with the cut-off of Ukkonen, beyond
/// the book), \f$ \mathit{score}[ b ] \f$ the cell at the bottom of the
/// block \f$ b \f$ in the current column and \f$ y \f$ the last block
/// computed:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{approximateMatches}( T , P , k ) \cr
/// 2 & \quad P_v \gets 1 \ldots 1;\ M_v \gets 0;\ y \gets \min\{ B - 1 ,
///     \lfloor k / 64 \rfloor \} \cr
/// 3 & \quad \textbf{for}\ j \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad h \gets 0\quad \triangleright\ \text{the top row, }
///     D[ 0 ][ j ] = 0 \cr
/// 5 & \qquad \textbf{for}\ b \gets 0\ \textbf{to}\ y\ \textbf{do}
///     \ \text{move the block } b \text{ by } \mathit{Peq}[ T[ j - 1 ] ]
///     \text{ and } h \text{, and update } \mathit{score}[ b ] \cr
/// 6 & \qquad \textbf{if}\ y + 1 < B\ \textbf{and}\ \mathit{score}[ y ] \le
///     k + 1\ \textbf{then}\ y \gets y + 1 \text{, the block entering all }
///     +1 \cr
/// 7 & \qquad \textbf{else while}\ y > 0\ \textbf{and}\ \mathit{score}[ y ]
///     > k + 64\ \textbf{do}\ y \gets y - 1 \cr
/// 8 & \qquad \textbf{if}\ y = B - 1\ \textbf{and}\ D[ m ][ j ] \le k
///     \ \textbf{then}\ \text{report } ( j , D[ m ][ j ] )
/// \end{array}}
/// \f]
/// \post the \f$ j \f$ with
///       \f$ \mathrm{matchingTable}( P , T )[ m ][ j ] \le k \f$ (an audit)
[[nodiscard]] inline std::vector< std::pair< std::size_t , std::size_t > >
 approximateMatches( std::string_view T , std::string_view P ,
                     std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/EditDistance.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EditDistance.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EditDistance.h -------------------------*/
/*--------------------------------------------------------------------------*/
