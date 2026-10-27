/*--------------------------------------------------------------------------*/
/*--------------------------- File LZ77Linear.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parse of LZ77 with no window in \f$ O( n ) \f$ time, the algorithm of
 * Kärkkäinen, Kempa and Puglisi (beyond the book), against the parse of
 * chapter 13, Dictionary-Based Compressors, of Ferragina's book [see
 * `LempelZiv.h`], which searches the window for each phrase, in
 * \f$ O( n w ) \f$ time. The phrases are those of `lz77Parse()` with an
 * unbounded window: at the place \f$ i \f$ the longest prefix of
 * \f$ s[ i , n ) \f$ that starts at an earlier place (and may overlap
 * \f$ s[ i , n ) \f$), its length \f$ \ell \f$, the distance back to one such
 * start, and the symbol after it; the parse moves \f$ \ell + 1 \f$ places.
 *
 * Among the suffixes that start before \f$ i \f$, the one that shares the
 * longest prefix with the suffix \f$ i \f$ is next to it in the order of the
 * suffixes: it is either the previous smaller value or the next smaller
 * value of \f$ i \f$,
 * \f[ \mathit{PSV}( i ) = \mathit{SA}[ \max \{ j < r : \mathit{SA}[ j ] < i
 *     \} ] , \qquad \mathit{NSV}( i ) = \mathit{SA}[ \min \{ j > r :
 *     \mathit{SA}[ j ] < i \} ] , \f]
 * with \f$ r \f$ the rank of \f$ i \f$ in the suffix array \f$ \mathit{SA} \f$
 * [see `SuffixArray.h`], since the longest common prefix of two suffixes is
 * the least over the suffixes between them; hence
 * \f$ \ell = \max( \mathrm{lcp}( i , \mathit{PSV}( i ) ) , \mathrm{lcp}( i ,
 * \mathit{NSV}( i ) ) ) \f$. Both are computed for all \f$ i \f$ in one scan
 * of \f$ \mathit{SA} \f$ with a stack, each place pushed and popped once, and
 * the two prefixes of a phrase are compared symbol by symbol, at most
 * \f$ \ell + 1 \f$ comparisons each, which sum to \f$ O( n ) \f$ over the
 * phrases. Hence \f$ O( n ) \f$ time with the suffix array of SA-IS, and two
 * arrays of \f$ n \f$ integers of 32 bits besides the text and the suffix
 * array, which is freed before the parse; the source found is the previous
 * or the next smaller value, not the closest one of the same chapter, with
 * the same lengths.
 *
 * The contracts [see `Contract.h`]: \f$ n < 2^{32} - 1 \f$, since
 * \f$ 2^{32} - 1 \f$ marks a missing value; the audit is that the phrases
 * decode to the text [see `LempelZiv.h`]. The function is defined in
 * `LZ77Linear.tpp`.
 *
 * \see J. Kärkkäinen, D. Kempa, S. J. Puglisi, Linear time Lempel-Ziv
 *      factorization: simple, fast, small, Proc. 24th CPM, LNCS 7922,
 *      189-200, 2013, doi:10.1007/978-3-642-38905-4_19
 * \see G. Chen, S. J. Puglisi, W. F. Smyth, Lempel-Ziv factorization using
 *      less time & space, Mathematics in Computer Science 1(4), 605-623,
 *      2008, doi:10.1007/s11786-007-0024-4
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 13, Dictionary-Based Compressors,
 *      doi:10.1017/9781009128933
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

#ifndef __ADS_LZ77Linear
 #define __ADS_LZ77Linear
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/LempelZiv.h"
#include "ADS/string/SuffixArray.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the phrases of LZ77 of \f$ s \f$ with no window, in \f$ O( |s| ) \f$ time
///
/// In pseudocode (the algorithm of Kärkkäinen, Kempa and Puglisi, beyond the
/// book), the previous and the next smaller values found by one scan of the
/// suffix array with a stack \f$ S \f$ of places, increasing from the
/// bottom:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lz77ParseLinear}( s ) \cr
/// 2 & \quad \mathit{SA} \gets \text{the suffix array of } s;\ S \gets
///     \text{an empty stack} \cr
/// 3 & \quad \textbf{foreach}\ x \in \mathit{SA} \text{, in order}
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{while}\ S \ne \emptyset\ \textbf{and}
///     \ \mathrm{top}( S ) > x\ \textbf{do}\ \mathit{NSV}[ \mathrm{pop}( S ) ]
///     \gets x \cr
/// 5 & \qquad \textbf{if}\ S \ne \emptyset\ \textbf{then}\ \mathit{PSV}[ x ]
///     \gets \mathrm{top}( S ) \cr
/// 6 & \qquad \mathrm{push}( S , x ) \cr
/// 7 & \quad i \gets 0;\ P \gets \text{an empty list} \cr
/// 8 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 9 & \qquad a \gets \mathrm{lcp}( i , \mathit{PSV}[ i ] );\ b \gets
///     \mathrm{lcp}( i , \mathit{NSV}[ i ] )\quad \triangleright\ 0 \text{ }
///     \text{if missing} \cr
/// 10 & \qquad \ell \gets \max( a , b ) \cr
/// 11 & \qquad d \gets 0 \text{ if } \ell = 0 \text{, else } i - (
///     \mathit{PSV}[ i ] \text{ if } a \ge b \text{, else } \mathit{NSV}[ i ]
///     ) \cr
/// 12 & \qquad c \gets s[ i + \ell ] \text{ if } i + \ell < n \text{, none }
///     \text{otherwise} \cr
/// 13 & \qquad \text{append } \langle d , \ell , c \rangle \text{ to } P;\ i
///     \gets i + \ell + 1 \cr
/// 14 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \pre \f$ |s| < 2^{32} - 1 \f$
/// \post `lz77Decode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< LZ77Phrase > lz77ParseLinear(
                                                        std::string_view s );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/LZ77Linear.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LZ77Linear.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File LZ77Linear.h --------------------------*/
/*--------------------------------------------------------------------------*/
