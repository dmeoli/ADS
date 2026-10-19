/*--------------------------------------------------------------------------*/
/*--------------------------- File LZ77Linear.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parse of LZ77 with no window in O( n ) time, the algorithm of
 * Karkkainen, Kempa and Puglisi, beyond the parse of the notes [see
 * LempelZiv.h], which searches the window for each phrase, in \f$ O( n w )
 * \f$ time. The phrases are those of lz77Parse() with an unbounded window:
 * at the place i the longest prefix of \f$ s[ i , n ) \f$ that starts at
 * an earlier place (and may overlap s[ i , n )), its length \f$ \ell \f$,
 * the distance back to one such start, and the symbol after it; the parse
 * moves \f$ \ell + 1 \f$ places.
 *
 * Among the suffixes that start before i, the one that shares the longest
 * prefix with the suffix i is next to it in the order of the suffixes: it
 * is either the previous smaller value or the next smaller value of i,
 * \f[ PSV( i ) = SA[ \max \{ j < r : SA[ j ] < i \} ] , \qquad
 *     NSV( i ) = SA[ \min \{ j > r : SA[ j ] < i \} ] , \f]
 * r the rank of i in the suffix array SA [see SuffixArray.h], since the
 * longest common prefix of two suffixes is the least over the suffixes
 * between them; so \f$ \ell = \max( lcp( i , PSV( i ) ) , lcp( i ,
 * NSV( i ) ) ) \f$. Both are computed for all i in one scan of SA with a
 * stack, each place pushed and popped once, and the two prefixes of a
 * phrase are compared symbol by symbol, \f$ \ell + 1 \f$ comparisons each,
 * which sum to O( n ) over the phrases. Hence O( n ) time with the suffix
 * array of SA-IS, and 4n + 8n bytes besides the text; the source found is
 * the previous or the next smaller value, not the closest one of the notes,
 * with the same lengths.
 *
 * The contracts [see Contract.h]: \f$ n < 2^{32} \f$; the audit is that the
 * phrases decode to the text [see LempelZiv.h]. The function is defined in
 * LZ77Linear.tpp.
 *
 * \see J. Karkkainen, D. Kempa, S. J. Puglisi, Linear time Lempel-Ziv
 *      factorization: simple, fast, small, Proc. 24th CPM, LNCS 7922,
 *      189-200, 2013, doi:10.1007/978-3-642-38905-4_19
 * \see G. Chen, S. J. Puglisi, W. F. Smyth, Lempel-Ziv factorization using
 *      less time & space, Mathematics in Computer Science 1(4), 605-623,
 *      2008, doi:10.1007/s11786-007-0024-4
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 13, doi:10.1017/9781009128933
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

/// the phrases of LZ77 of s with no window, in O( |s| ) time
/// \pre |s| < 2^32
/// \post lz77Decode( result ) == s (an audit)
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
