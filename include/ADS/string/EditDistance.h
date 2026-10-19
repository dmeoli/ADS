/*--------------------------------------------------------------------------*/
/*-------------------------- File EditDistance.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered edit distance, longest common subsequence and approximate
 * search, beyond the book: the ones that the benchmark of the distances
 * compares with edlib and rapidfuzz-cpp, while editDistance(),
 * stringMatching() [see ApproximateMatching.h] and lcs() [see
 * LongestCommonSubsequence.h] stay the dynamic programming of the
 * textbook, \f$ \Theta( m n ) \f$ time.
 *
 * All of them pack a column of the table of the dynamic programming in
 * words of 64 bits, by the differences between adjacent cells, which are
 * -1, 0 or +1 (the bit vectors of Myers), so that one column costs
 * \f$ O( \lceil m / 64 \rceil ) \f$ operations on words; the bits of the
 * pattern, one word per block of 64 places and per byte (Peq), say where
 * each byte occurs.
 *
 * - levenshtein( a , b ) strips the common prefix and suffix, then puts
 *   the shorter string, of m bytes, in the column and scans the longer one,
 *   of n bytes: one word if m <= 64, \f$ O( n ) \f$. Otherwise, the
 *   diagonal transitions of Landau and Vishkin (the furthest reaching
 *   paths of Myers, as in the wavefront alignment of Marco-Sola et al.):
 *   the furthest row of each diagonal reached with h edits, from those of
 *   h - 1 edits, slid along the matches 8 bytes at a time, \f$ O( n + d^2
 *   ) \f$ expected for distance d, up to d near n / 48 + 128, where they
 *   cost as much as the blocks (if m > 128); they give up as soon as the
 *   rows reached by 32, 64, 128, ... edits suggest a distance above twice
 *   as much. Then the blocks of
 *   Myers in the formulation of Hyyro, restricted to the diagonal band
 *   of Ukkonen of the cells that a path of cost at most k can cross, \f$ i
 *   - j \in [ -k , k - ( n - m ) ] \f$, and in it, as in edlib, to the
 *   blocks that may still lead to the end within k (a cell of value v
 *   costs at least v plus its distance from the diagonal of the end), with
 *   k doubled from the distance suggested by the diagonals until the
 *   distance is at most k, \f$ O( n d / 64 ) \f$ for distance d in the
 *   usual cases; the cells
 *   outside the band are taken as larger, which only overestimates cells
 *   that no path of cost k crosses. Once the bands have computed a quarter
 *   of the blocks of the table, or span half of the column, all the blocks
 *   are computed instead, \f$ \Theta( n \lceil m / 64 \rceil ) \f$: four
 *   columns at a time, the block b of the column j + q with the block b +
 *   1 of the column j + q - 1, which are independent, in the four lanes of
 *   an AVX2 vector (the bands of 8 blocks or more go the same way).
 *   levenshtein( a , b , k ) does the same with k fixed (the diagonals up
 *   to k, or the band of k) and gives k + 1 for any larger distance.
 * - lcsLength( a , b ) is the bit-parallel LCS of Allison and Dix in the
 *   form of Hyyro, \f$ V' = ( V + ( V \wedge M ) ) \vee ( V \wedge \neg M )
 *   \f$ with the carries between the blocks, \f$ O( n \lceil m / 64
 *   \rceil ) \f$ after the common prefix and suffix are stripped.
 * - approximateMatches( T , P , k ) gives the ends j (1 to n) of the
 *   substrings of T at edit distance at most k from P, with their
 *   distances, by the search of Myers: the top row of the table is 0, and
 *   only the blocks up to the last one with a cell at most k are computed
 *   (the cut-off of Ukkonen), \f$ O( n k / 64 ) \f$ expected on random
 *   texts.
 *
 * The contracts [see Contract.h]: the results equal those of the dynamic
 * programming, editDistance(), lcs() and the last row of matchingTable()
 * (audits, \f$ O( m n ) \f$ time). The functions are defined in
 * EditDistance.tpp.
 *
 * \see G. Myers, A fast bit-vector algorithm for approximate string
 *      matching based on dynamic programming, Journal of the ACM 46(3),
 *      395-415, 1999, doi:10.1145/316542.316550
 * \see H. Hyyro, A bit-vector algorithm for computing Levenshtein and
 *      Damerau edit distances, Nordic Journal of Computing 10(1), 29-39,
 *      2003
 * \see H. Hyyro, Bit-parallel LCS-length computation revisited,
 *      Proceedings of the 15th Australasian Workshop on Combinatorial
 *      Algorithms (AWOCA), 16-27, 2004
 * \see L. Allison, T. I. Dix, A bit-string longest-common-subsequence
 *      algorithm, Information Processing Letters 23(5), 305-310, 1986,
 *      doi:10.1016/0020-0190(86)90091-8
 * \see G. M. Landau, U. Vishkin, Fast parallel and serial approximate
 *      string matching, Journal of Algorithms 10(2), 157-169, 1989,
 *      doi:10.1016/0196-6774(89)90010-2
 * \see E. W. Myers, An O(ND) difference algorithm and its variations,
 *      Algorithmica 1, 251-266, 1986, doi:10.1007/BF01840446
 * \see S. Marco-Sola, J. C. Moure, M. Moreto, A. Espinosa, Fast gap-affine
 *      pairwise alignment using the wavefront algorithm, Bioinformatics
 *      37(4), 456-463, 2021, doi:10.1093/bioinformatics/btaa777
 * \see E. Ukkonen, Algorithms for approximate string matching, Information
 *      and Control 64(1-3), 100-118, 1985,
 *      doi:10.1016/S0019-9958(85)80046-2
 * \see M. Sosic, M. Sikic, Edlib: a C/C++ library for fast, exact sequence
 *      alignment using edit distance, Bioinformatics 33(9), 1394-1395,
 *      2017, doi:10.1093/bioinformatics/btw753
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
