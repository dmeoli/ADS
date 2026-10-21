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
 * Besides T and the n words of SA, a level takes 8 K words for the
 * counts and the parts of the buckets and n / 32 words for two bit
 * vectors, and the levels together \f$ O( n ) \f$ words. As engineered
 * here:
 *
 * - the types, 64 places a word: with \f$ G \f$ the bits of the places
 *   where \f$ T[ i ] < T[ i + 1 ] \f$ and \f$ E \f$ of those where they
 *   are equal, both from vector comparisons (AVX2) and read from the
 *   higher places to the lower ones, the bits of type S are the carries
 *   of the one addition \f$ G + ( G \vee E ) \f$, since
 *   \f$ t_i = S \f$ if and only if \f$ G_i \vee ( E_i \wedge t_{i+1} = S )
 *   \f$, the carry rule of a sum; a word costs a few operations, and only
 *   the carry passes from word to word. The suffixes i >= 1 fall into the
 *   four kinds of the types of i and i - 1 (LL, LS, LMS and SS), counted
 *   per first symbol with no branch in two histograms, one for the even
 *   places and one for the odd, so that equal symbols in a row do not
 *   wait for each other, the counters 4 T[ i ] + kind of 64 places built
 *   by vector operations from the bits of the types;
 * - on a text of bytes the LMS-suffixes are first sorted directly, as
 *   strings, and the induction only follows (the two stages of Itoh and
 *   Tanaka and of DivSufSort, on the LMS-suffixes of SA-IS): the \f$
 *   \sigma \f$ bytes that occur are ranked, and when \f$ \sigma \le 64
 *   \f$ the text is packed in codes of \f$ b = \lceil \log_2 \sigma
 *   \rceil \f$ bits (the bytes themselves otherwise, b = 8), so that the
 *   window of 64 bits at the bit b p of the packed text holds the first
 *   \f$ 64 / b \f$ symbols of the suffix p and two suffixes compare as
 *   their windows, then as the windows 64 bits on. The items ( top bits
 *   of the window , p ), one word on 32 bits, are sorted by a radix sort
 *   on the top \f$ N = \lceil \log_2 ( m + 1 ) \rceil + 6 \f$ bits of
 *   their windows (into the 256 buckets of the top 8 bits first when \f$
 *   m \ge 2^{16} \f$, then each bucket in the cache by passes of at most
 *   11 bits), so that on a text with no long repeats about \f$ m^2 /
 *   2^{N+1} \le m / 128 \f$ pairs stay equal, settled by insertion; the
 *   suffixes of equal windows go on to the next windows, a suffix that
 *   ends inside a window before those that go on. The whole of it reads
 *   the text in order, against the 2 random reads a suffix of the partial
 *   induction, and needs no names and no recursion. It is tried when a
 *   sample of 1024 keys of 32 bits has at most one equal pair in 256 (on a
 *   text whose windows repeat, as in English or in long repeats, the
 *   induction does better), and given up for the partial induction once
 *   the ties read more than m windows;
 * - the partial induction, which sorts the LMS-substrings, splits the
 *   bucket of each symbol c into parts by the type of the suffix before:
 *   an area A, with the LL part and the LMS part (the seeds) of each
 *   symbol, holds the entries that the scan from left to right moves, and
 *   an area C, with the LS part and the SS part, those that the scan from
 *   right to left moves, so that no scan visits an entry with nothing to
 *   do. An entry is marked when it starts a new name: a moved suffix is
 *   marked unless the last one moved into the same part came from a
 *   suffix of the same name, the name being the count d of the marks the
 *   scan has met; the scans keep the arrays in functions that declare
 *   them not to overlap, two entries a step, the text 64 entries ahead
 *   prefetched and, beyond 65536 symbols, the parts and the places they
 *   point to;
 * - the names are written only when some are tied, and two LMS-suffixes
 *   of equal names (equal LMS-substrings, of the same length l) compare
 *   as the text that follows them: on the bytes, when the groups of equal
 *   names have at most 16 suffixes on average, each group is sorted by
 *   keys of 8 bytes of the text read at p + l, p + l + 8, ..., round
 *   after round on the parts still tied (at most 8 rounds and 2 m keys,
 *   and none if a first round on a sample of the groups leaves half of
 *   them tied, as on long repeats), small groups by insertion and large
 *   ones by a radix sort on the bits that their keys do not share,
 *   gathered by pext; the groups left are sorted by the names that
 *   follow, within a budget of m names, and only then by the doubling of
 *   Larsson and Sadakane on the groups of equal names (when few, or small)
 *   or by the recursion on the string of the names, at the back of SA:
 *   in the round h = 1, 2, 4, ... of the doubling the suffixes of a group,
 *   equal on their first h names, are sorted by \f$ V[ i + h ] \f$, the
 *   rank of the suffix h places ahead, a group of g suffixes costing
 *   \f$ O( g \log g ) \f$;
 * - the sorted LMS-suffixes go to the ends of their buckets a group of
 *   symbol at a time, by memmove(), and the final induction stores a
 *   suffix with its top bit set when the suffix before it is of the other
 *   type, by a shift and no branch, the text it prefetches at a place
 *   chosen by a select;
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
 * first four one by one (a load of 8 bytes would often reach a line not
 * prefetched) and then 8 at a time; the text at \f$ \Phi[ i + 64 ] \f$ is
 * prefetched, and \f$ \mathit{LCP}[ r ] = \mathit{PLCP}[ SA[ r + 1 ] ] \f$
 * costs one random read per entry, prefetched too. LCP[ r ] is the lcp of
 * the suffixes SA[ r ] and SA[ r + 1 ], n - 1 entries, as in lcpArray();
 * \f$ \Phi \f$ and PLCP share one array of n integers, and the three
 * passes keep their arrays in functions that declare them not to overlap.
 *
 * The contracts [see Contract.h]: the symbols of an integer text are less
 * than K and n fits the integers; the arrays built are a suffix array and
 * its LCP array (audits, O( n ) and \f$ O( n + \sum_r \mathit{LCP}[ r ] )
 * \f$ time). The functions are defined in SuffixSorting.tpp.
 *
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
 * \see H. Itoh, H. Tanaka, An efficient method for in memory construction
 *      of suffix arrays, Proceedings of the 6th Symposium on String
 *      Processing and Information Retrieval (SPIRE), 81-88, 1999,
 *      doi:10.1109/SPIRE.1999.796581
 * \see J. Fischer, F. Kurpicz, Dismantling DivSufSort, Proceedings of the
 *      Prague Stringology Conference (PSC), 62-76, 2017,
 *      https://arxiv.org/abs/1710.01896
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.5 (radix sorting)
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
