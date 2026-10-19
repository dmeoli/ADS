/*--------------------------------------------------------------------------*/
/*-------------------------- File SuffixSorting.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered construction of the suffix array and of its LCP array,
 * beyond the book: the ones that the benchmark of the suffix arrays
 * compares with libsais and libdivsufsort, while the constructions of
 * SuffixArray.h stay those of the textbook, on integers of the size of a
 * pointer.
 *
 * suffixArray() is the induced sorting of Nong, Zhang and Chan (SA-IS)
 * [see SuffixArray.h] on integers of 32 bits (or of 64, for the texts of
 * \f$ 2^{31} \f$ symbols or more). The suffix i of T (n symbols in
 * [ 0 , K )) is of type
 * \f[
 *   t_i = \left\{ \begin{array}{ll}
 *     L & i = n - 1 \mbox{ or } T[ i ] > T[ i + 1 ] \mbox{ or }
 *         ( T[ i ] = T[ i + 1 ] \mbox{ and } t_{i+1} = L ) \\
 *     S & \mbox{otherwise,}
 *   \end{array} \right.
 * \f]
 * S if it is smaller than the suffix i + 1, and i is an LMS place if
 * \f$ t_i = S \f$ and \f$ t_{i-1} = L \f$; two LMS places are never
 * adjacent, so that there are \f$ m \le \lfloor n / 2 \rfloor \f$ of them.
 * Once the LMS-suffixes are sorted and put at the ends of the buckets of
 * their first symbols, one scan from left to right puts each L-suffix
 * i - 1 at the head of its bucket after the suffix i, and one from right
 * to left each S-suffix i - 1 at the tail of its bucket, which sorts all
 * of them; the same two scans from the LMS places in any order sort the
 * LMS-substrings (from an LMS place to the next one), whose names give a
 * string of m symbols whose suffix array orders the LMS-suffixes, so that
 * \f[
 *   T( n ) \le T( \lfloor n / 2 \rfloor ) + O( n ) = O( n ) .
 * \f]
 * Besides T and the n words of SA, a level takes 2 K words for the
 * buckets and n / 64 words for a bit vector of the LMS places, and the
 * levels together \f$ O( n ) \f$ words. As engineered here:
 *
 * - no array of the types: in the two scans of the induction a suffix is
 *   stored complemented when the suffix before it is of the other type,
 *   which the two symbols at its start tell, so that each scan moves only
 *   the entries it must move; two entries a step, the text before the
 *   entries 64 places ahead prefetched, and on the alphabets of more than
 *   1024 symbols also their buckets (32 ahead) and the places these point
 *   to (16 ahead);
 * - the LMS-substrings, once sorted by the first induction (which leaves
 *   them complemented, and clears the other entries), are compacted at the
 *   front of SA, their lengths stored in the second half at the place p / 2
 *   of their start p, and two of them are equal if their lengths are and
 *   memcmp() finds their symbols equal (equal symbols and an equal end
 *   give equal types);
 * - the string of the names lies at the back of SA and its suffix array,
 *   at the front, is built recursively, the numbers of its symbols counted
 *   while naming, in the sorted order; but when at most m / 4 of the
 *   LMS-suffixes share their name with another (as on random texts over
 *   large alphabets, where most names occur once), it is built instead by
 *   a counting sort of the names followed by the doubling of Larsson and
 *   Sadakane on the groups of equal names alone: in the round h = 1, 2, 4,
 *   ... the suffixes of a group, equal on their first h names, are sorted
 *   by \f$ V[ i + h ] \f$, the rank of the suffix h places ahead (the last
 *   place of its group, -1 past the end), so that a group of g suffixes
 *   costs \f$ O( g \log g ) \f$ per round, and the doubling gives up to the
 *   recursion if more than m / 4 suffixes stay tied, or after 16 rounds;
 * - a text whose suffixes are all of type L never rises, so that each
 *   suffix is larger than the next one and SA is \f$ n - 1 , \ldots , 0
 *   \f$.
 *
 * lcpArrayPhi() is the algorithm of Karkkainen, Manzini and Puglisi: with
 * \f$ \Phi[ SA[ r ] ] = SA[ r - 1 ] \f$, the permuted LCP
 * \f[
 *   \mathit{PLCP}[ i ] = \mathrm{lcp}( T[ i .. ] , T[ \Phi[ i ] .. ] ) ,
 *   \qquad \mathit{PLCP}[ i + 1 ] \ge \mathit{PLCP}[ i ] - 1
 * \f]
 * (Lemma of Kasai et al.), is computed in the order of the text from the
 * value at i - 1 less one, \f$ 2 n \f$ comparisons of symbols in all, the
 * first two one by one and then 8 at a time; the text at \f$ \Phi[ i + 64
 * ] \f$ is prefetched, and \f$ \mathit{LCP}[ r ] = \mathit{PLCP}[ SA[ r + 1
 * ] ] \f$ costs one random read per entry. LCP[ r ] is the lcp of the
 * suffixes SA[ r ] and SA[ r + 1 ], n - 1 entries, as in lcpArray();
 * \f$ \Phi \f$ and PLCP share one array of n integers.
 *
 * The contracts [see Contract.h]: the symbols of an integer text are less
 * than K and n fits the integers; the arrays built are a suffix array and
 * its LCP array (audits, O( n ) and \f$ O( n + \sum_r \mathit{LCP}[ r ] )
 * \f$ time). The functions are defined in SuffixSorting.tpp.
 *
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
 * \see U. Manber, G. Myers, Suffix arrays: a new method for on-line string
 *      searches, SIAM Journal on Computing 22(5), 935-948, 1993,
 *      doi:10.1137/0222058
 * \see N. J. Larsson, K. Sadakane, Faster suffix sorting, Theoretical
 *      Computer Science 387(3), 258-272, 2007,
 *      doi:10.1016/j.tcs.2007.07.017
 * \see J. Karkkainen, G. Manzini, S. J. Puglisi, Permuted longest-common-
 *      prefix array, Proceedings of the 20th Symposium on Combinatorial
 *      Pattern Matching (CPM), LNCS 5577, 181-192, 2009,
 *      doi:10.1007/978-3-642-02441-2_17
 * \see T. Kasai, G. Lee, H. Arimura, S. Arikawa, K. Park, Linear-time
 *      longest-common-prefix computation in suffix arrays and its
 *      applications, Proceedings of the 12th Symposium on Combinatorial
 *      Pattern Matching (CPM), LNCS 2089, 181-192, 2001,
 *      doi:10.1007/3-540-48194-X_17
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

#ifndef __ADS_SuffixSorting
 #define __ADS_SuffixSorting
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the suffix array of the bytes of T, by SA-IS on integers I (32 or 64
/// bits, unsigned)
/// \pre T.size() < 2^31 if I has 32 bits
/// \post a suffix array of T (an audit)
template< class I = std::uint32_t >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
[[nodiscard]] std::vector< I > suffixArray( std::string_view T );

/// the suffix array of an integer text, by SA-IS
/// \pre every symbol of T is less than K, T.size() < 2^31 if I has 32 bits
/// \post a suffix array of T (an audit)
template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
[[nodiscard]] std::vector< I > suffixArray( const std::vector< I > & T ,
                                            std::size_t K );

/// the permuted LCP array: PLCP[ i ] is the lcp of the suffix i and the
/// one before it in SA (0 for the first)
/// \pre SA is a suffix array of T (an audit)
template< class I >
[[nodiscard]] std::vector< I > plcpArray( std::string_view T ,
                                          const std::vector< I > & SA );

/// the LCP array by the permuted LCP array, n - 1 entries (none if n < 2)
/// \pre SA is a suffix array of T (an audit)
/// \post LCP[ r ] is the lcp of the suffixes SA[ r ] and SA[ r + 1 ] (an
///       audit)
template< class I >
[[nodiscard]] std::vector< I > lcpArrayPhi( std::string_view T ,
                                            const std::vector< I > & SA );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SuffixSorting.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuffixSorting.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SuffixSorting.h ------------------------*/
/*--------------------------------------------------------------------------*/
