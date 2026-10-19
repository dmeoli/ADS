/*--------------------------------------------------------------------------*/
/*-------------------------- File EditDistance.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered edit distance, longest common subsequence and approximate
 * search, beyond the book: the ones that the benchmark of the distances
 * compares with edlib and rapidfuzz-cpp, while editDistance(),
 * stringMatching() [see ApproximateMatching.h] and lcs() [see
 * LongestCommonSubsequence.h] stay the dynamic programming of the
 * textbook, \f$ \Theta( m n ) \f$ time. For a pattern P of m bytes (the
 * column) and a text T of n bytes, the table of the distances is
 * \f[
 *   D[ i ][ j ] = \min\{ D[ i - 1 ][ j - 1 ] + [ P[ i - 1 ] \ne T[ j - 1 ] ]
 *                 , D[ i - 1 ][ j ] + 1 , D[ i ][ j - 1 ] + 1 \} ,
 * \f]
 * with \f$ D[ i ][ 0 ] = i \f$ and \f$ D[ 0 ][ j ] = j \f$ for the distance
 * (0 for the search), whose adjacent cells differ by -1, 0 or +1. The bit
 * vectors of Myers keep a column as the vertical differences \f$ D[ i ][ j
 * ] - D[ i - 1 ][ j ] \f$, +1 in Pv and -1 in Mv, 64 rows a word (a
 * block), and \f$ Eq = Peq[ T[ j - 1 ] ] \f$, the places of P that hold the
 * byte, moves a block to the next column with the horizontal difference
 * \f$ h_{in} \f$ at its top, in the formulation of Hyyro:
 * \f[
 *   \begin{array}{l}
 *     X_v = Eq \vee M_v , \quad Eq' = Eq \vee [ h_{in} < 0 ] , \quad
 *     X_h = ( ( ( Eq' \wedge P_v ) + P_v ) \oplus P_v ) \vee Eq' , \\
 *     P_h = M_v \vee \neg( X_h \vee P_v ) , \quad M_h = P_v \wedge X_h ,
 *     \quad h_{out} = P_h[ 63 ] - M_h[ 63 ] , \\
 *     P_h \leftarrow ( P_h \ll 1 ) \vee [ h_{in} > 0 ] , \quad
 *     M_h \leftarrow ( M_h \ll 1 ) \vee [ h_{in} < 0 ] , \quad
 *     P_v \leftarrow M_h \vee \neg( X_v \vee P_h ) , \quad
 *     M_v \leftarrow P_h \wedge X_v ,
 *   \end{array}
 * \f]
 * a dozen operations on words with no branch, so that a column costs
 * \f$ O( \lceil m / 64 \rceil ) \f$ of them.
 *
 * - levenshtein( a , b ) strips the common prefix and suffix, then puts
 *   the shorter string, of m bytes, in the column and scans the longer one,
 *   of n bytes: one word if \f$ m \le 64 \f$, O( n ). Otherwise, the
 *   diagonal transitions of Landau and Vishkin (the furthest reaching
 *   paths of Myers, as in the wavefront alignment of Marco-Sola et al.):
 *   \f$ F_h( k ) \f$, the furthest row of the diagonal k = j - i reached by
 *   h edits, is
 *   \f[
 *     F_h( k ) = \mathit{slide}_k\bigl( \max\{ F_{h-1}( k ) + 1 ,
 *                F_{h-1}( k + 1 ) + 1 , F_{h-1}( k - 1 ) \} \bigr) ,
 *   \f]
 *   \f$ \mathit{slide}_k( i ) \f$ the row reached from i along the matches
 *   of the diagonal (compared 8 bytes at a time), and the distance is the
 *   least d with \f$ F_d( n - m ) = m \f$, \f$ O( n + d^2 ) \f$ expected
 *   time on random strings and O( d ) space. They are used for d up to
 *   \f$ n / 48 + 128 \f$ (if m > 128), where they cost about as much as
 *   the blocks, and give up once the rows r reached by h = 32, 64, 128, ...
 *   edits suggest a distance \f$ h m / r \f$ above twice as much. Then the
 *   blocks, restricted to the diagonal band of Ukkonen of the cells that a
 *   path of cost at most k can cross, \f$ i - j \in [ -k , k - ( n - m ) ]
 *   \f$, and in it, as in edlib, to the blocks that may still lead to the
 *   end within k: a cell ( i , j ) costs at least \f$ D[ i ][ j ] + | ( m -
 *   i ) - ( n - j ) | \f$ to the end, and a cell of a block at least the
 *   bottom of the block minus its distance from it, so that the first and
 *   the last block are dropped when the least of these bounds exceeds k;
 *   the block below the last one enters, all +1, when the bottom of the
 *   last is at most k + 1, since the cells at most k grow by at most one
 *   row per column. The cells outside the band are taken as larger, which
 *   overestimates only cells that no path of cost k crosses. k starts from
 *   the guess of the diagonals and a quarter more, and doubles until the
 *   distance is at most k, \f$ O( n d / 64 ) \f$ for distance d in the
 *   usual cases. Once the bands have computed a quarter of the \f$ n \lceil
 *   m / 64 \rceil \f$ blocks of the table, or span half of the column, all
 *   the blocks are computed instead, four columns at a time: the block b of
 *   the column j + q needs only the block b - 1 of the same column and the
 *   block b of the column before, so that the blocks i - q of the columns j
 *   + q, q < 4, are independent, the four lanes of an AVX2 vector (the
 *   bands of 8 blocks or more go the same way). levenshtein( a , b , k )
 *   does the same with k fixed (the diagonals up to k, or the band of k)
 *   and gives k + 1 for any larger distance.
 * - lcsLength( a , b ) is the bit-parallel LCS of Allison and Dix in the
 *   form of Crochemore, Iliopoulos, Pinzon and Reid: with \f$ V = 1^m \f$
 *   at first and \f$ M = Peq[ b[ j ] ] \f$,
 *   \f[
 *     V \leftarrow ( V + ( V \wedge M ) ) \vee ( V \wedge \neg M ) ,
 *   \f]
 *   the addition carried between the blocks, and the length is the number
 *   of 0 among the m bits of V, \f$ O( n \lceil m / 64 \rceil ) \f$ after
 *   the common prefix and suffix are stripped.
 * - approximateMatches( T , P , k ) gives the ends j (1 to n) of the
 *   substrings of T at edit distance at most k from P, \f$ D[ m ][ j ] \le
 *   k \f$ with the top row 0, with their distances, by the search of
 *   Myers: only the blocks up to the last one with a cell at most k + 1
 *   at its bottom are computed (the cut-off of Ukkonen), \f$ O( n k / 64 )
 *   \f$ expected on random texts.
 *
 * The bits Peq take \f$ 256 \lceil m / 64 \rceil \f$ words (in the object
 * up to 4 blocks, with only the bytes of the two strings cleared beyond).
 * The contracts [see Contract.h]: the results equal those of the dynamic
 * programming, editDistance(), lcs() and the last row of matchingTable()
 * (audits, \f$ O( m n ) \f$ time). The functions are defined in
 * EditDistance.tpp.
 *
 * \see V. I. Levenshtein, Binary codes capable of correcting deletions,
 *      insertions, and reversals, Soviet Physics Doklady 10(8), 707-710,
 *      1966
 * \see G. Myers, A fast bit-vector algorithm for approximate string
 *      matching based on dynamic programming, Journal of the ACM 46(3),
 *      395-415, 1999, doi:10.1145/316542.316550
 * \see H. Hyyro, A bit-vector algorithm for computing Levenshtein and
 *      Damerau edit distances, Nordic Journal of Computing 10(1), 29-39,
 *      2003, https://cs.helsinki.fi/njc/Contents_of_Vol10.html
 * \see G. M. Landau, U. Vishkin, Fast parallel and serial approximate
 *      string matching, Journal of Algorithms 10(2), 157-169, 1989,
 *      doi:10.1016/0196-6774(89)90010-2
 * \see E. W. Myers, An O(ND) difference algorithm and its variations,
 *      Algorithmica 1(1-4), 251-266, 1986, doi:10.1007/BF01840446
 * \see S. Marco-Sola, J. C. Moure, M. Moreto, A. Espinosa, Fast gap-affine
 *      pairwise alignment using the wavefront algorithm, Bioinformatics
 *      37(4), 456-463, 2021, doi:10.1093/bioinformatics/btaa777
 * \see E. Ukkonen, Algorithms for approximate string matching, Information
 *      and Control 64(1-3), 100-118, 1985,
 *      doi:10.1016/S0019-9958(85)80046-2
 * \see M. Sosic, M. Sikic, Edlib: a C/C++ library for fast, exact sequence
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
 *         Universita' di Pisa \n
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

/// the edit distance of Levenshtein of a and b
/// \post editDistance( a , b ) (an audit)
[[nodiscard]] inline std::size_t levenshtein( std::string_view a ,
                                              std::string_view b );

/// the edit distance of a and b if at most k, k + 1 otherwise
/// \post min( editDistance( a , b ) , k + 1 ) (an audit)
[[nodiscard]] inline std::size_t levenshtein( std::string_view a ,
                                              std::string_view b ,
                                              std::size_t k );

/// the length of a longest common subsequence of a and b
/// \post the length of lcs( a , b ) (an audit)
[[nodiscard]] inline std::size_t lcsLength( std::string_view a ,
                                            std::string_view b );

/// the ends j, from 1 to T.size(), of the substrings of T at edit
/// distance at most k from P, in increasing order, with their distances
/// \post the j with matchingTable( P , T )[ m ][ j ] <= k (an audit)
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
