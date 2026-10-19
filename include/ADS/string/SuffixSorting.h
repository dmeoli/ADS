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
 * \f$ 2^{31} \f$ symbols or more), in the space of SA and O( K ) words for
 * the buckets besides the text, as engineered by Mori and by Grebnov:
 *
 * - no array of the types: in the two scans of the induction a suffix is
 *   stored complemented when the suffix before it is of the other type,
 *   which the two symbols at its start tell, so that each scan visits only
 *   the entries it must move, and the bucket of the last symbol written
 *   stays in a register while the symbols repeat;
 * - the LMS-substrings, once sorted by the first induction, are compacted
 *   at the front of SA, their lengths stored in the second half at the
 *   place p / 2 of their start p (two LMS places are never adjacent), and
 *   two of them are equal if their lengths are and memcmp() finds their
 *   symbols equal (equal symbols and an equal end give equal types);
 * - the reduced string of the names lies at the back of SA and its suffix
 *   array, at the front, is built by the same function, recursively, the
 *   numbers of its symbols counted while naming, in the sorted order;
 *   but when at most a quarter of the LMS-suffixes share their name with
 *   another (as on random texts over large alphabets, where most names
 *   occur once), the reduced string is sorted instead by a counting sort
 *   of its names followed by the doubling of Larsson and Sadakane on the
 *   groups of equal names alone (each by the rank of the suffix h places
 *   ahead, h = 1, 2, 4, ...), which costs little more than one pass;
 * - a text with no LMS-suffix (one that never rises) is sorted by the
 *   final induction alone;
 * - the scans prefetch the symbol before the suffix a fixed distance ahead,
 *   so that the random reads of the text overlap.
 *
 * lcpArrayPhi() is the algorithm of Karkkainen, Manzini and Puglisi: the
 * array \f$ \Phi[ SA[ r ] ] = SA[ r - 1 ] \f$ turns into the permuted LCP,
 * PLCP[ i ] the lcp of the suffix i with the one before it in SA, in the
 * order of the text, where PLCP[ i + 1 ] >= PLCP[ i ] - 1 as in the
 * algorithm of Kasai et al. [see SuffixArray.h], but the text is read
 * left to right, 8 bytes at a time, and only \f$ \Phi \f$ at random; LCP[
 * r ] = PLCP[ SA[ r + 1 ] ] then costs one random read per entry. LCP[ r ]
 * is the lcp of the suffixes SA[ r ] and SA[ r + 1 ], n - 1 entries, as in
 * lcpArray().
 *
 * The contracts [see Contract.h]: the symbols of an integer text are less
 * than K and n fits the integers; the arrays built are a suffix array and
 * its LCP array (audits, O( n ) and O( n + sum of LCP ) time). The
 * functions are defined in SuffixSorting.tpp.
 *
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
 * \see Y. Mori, sais: an implementation of the induced sorting algorithm,
 *      2010, https://sites.google.com/site/yuta256/sais
 * \see I. Grebnov, libsais, 2021-2026, https://github.com/IlyaGrebnov/libsais
 * \see N. J. Larsson, K. Sadakane, Faster suffix sorting, Theoretical
 *      Computer Science 387(3), 258-272, 2007,
 *      doi:10.1016/j.tcs.2007.07.017
 * \see J. Karkkainen, G. Manzini, S. J. Puglisi, Permuted longest-common-
 *      prefix array, Proceedings of the 20th Symposium on Combinatorial
 *      Pattern Matching (CPM), LNCS 5577, 181-192, 2009,
 *      doi:10.1007/978-3-642-02441-2_17
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
