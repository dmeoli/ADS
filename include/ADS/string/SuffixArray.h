/*--------------------------------------------------------------------------*/
/*--------------------------- File SuffixArray.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix array of a text \f$ T \f$ of \f$ n \f$ symbols, beyond the
 * book: \f$ \mathit{SA}[ r ] \f$ is the starting place of the \f$ r \f$-th
 * smallest suffix of \f$ T \f$ in the lexicographic order, a suffix that is
 * a prefix of another being the smaller. The occurrences of a pattern
 * \f$ P \f$ of \f$ p \f$ symbols are the suffixes prefixed by \f$ P \f$,
 * which lie in a contiguous range of \f$ \mathit{SA} \f$, found by binary
 * search; the array takes \f$ n \f$ integers besides \f$ T \f$, against the
 * 10 to 20 \f$ n \f$ bytes of a suffix tree [see `SuffixTree.h`]. The places
 * count from 0, and no terminator is stored: chapter 10, Searching Strings
 * by Substring, of Ferragina's book writes \f$ T \f$ with a final
 * \f$ \$ \f$, whose suffix would be the first of \f$ \mathit{SA} \f$.
 *
 * The constructions, each giving the same array:
 *
 * - `suffixArraySorting()`: the \f$ n \f$ suffixes sorted by a sort of
 *   comparisons (`std::sort`), each comparison as long as the common
 *   prefix, \f$ O( n \log n ) \f$ comparisons and \f$ O( n^2 \log n ) \f$
 *   time in the worst case (a text of one repeated symbol), near
 *   \f$ O( n \log^2 n ) \f$ on random texts;
 * - `suffixArrayDoubling()`, the prefix doubling of Manber and Myers: after
 *   round \f$ k \f$ each suffix has the rank of its prefix of \f$ 2^k \f$
 *   symbols, and the pair of the ranks of the suffixes at \f$ i \f$ and
 *   \f$ i + 2^k \f$ is the rank of the prefix of \f$ 2^{k+1} \f$ symbols;
 *   the pairs are sorted by the LSD radix sort of `RadixSort.h`,
 *   \f$ O( n \log n ) \f$ time in all, or \f$ O( n \log L ) \f$ for
 *   \f$ L \f$ the longest repeated prefix;
 * - `suffixArrayLarssonSadakane()`, the doubling of Larsson and Sadakane
 *   (qsufsort): the same ranks, but a round sorts only the groups of
 *   suffixes that are still tied, each by the rank of the suffix \f$ h \f$
 *   places ahead, and refines the ranks at once, so that the groups already
 *   sorted are skipped by a length stored in their first place. Their bound
 *   is \f$ O( n \log n ) \f$ with a ternary-split quicksort; with the sort
 *   of the library inside the groups it is \f$ O( n \log^2 n ) \f$ in the
 *   worst case, and close to linear on the usual texts;
 * - `suffixArraySkew()`, the Skew (DC3) algorithm of Kärkkäinen and
 *   Sanders: the suffixes at the places \f$ i \bmod 3 \ne 0 \f$ (the
 *   \f$ P_{2,0} \f$ of the same chapter, which counts from 1) are named by
 *   the ranks of their triples of symbols, sorted by three passes of radix
 *   sort, and sorted by the recursive suffix array of the string of the
 *   names, of \f$ 2 n / 3 \f$ symbols, unless all the names differ; the
 *   suffixes at \f$ i \bmod 3 = 0 \f$ are sorted by the pair
 *   \f$ ( T[ i ] , \mathrm{rank}( i + 1 ) ) \f$; the two lists are merged
 *   by comparing a pair or a triple whose last item is a rank in the
 *   sample. Hence \f$ T( n ) = T( 2 n / 3 ) + O( n ) = O( n ) \f$ on an
 *   integer alphabet of \f$ O( n ) \f$ symbols, and the reduction to
 *   sorting gives the optimal \f$ O( \mathrm{Sort}( n ) ) \f$ I/Os on
 *   disk, those of sorting \f$ n \f$ items;
 * - `suffixArraySais()`, the induced sorting of Nong, Zhang and Chan
 *   (SA-IS): each suffix is of type S or L as it is smaller or larger than
 *   the next one, and the leftmost S-suffixes of the runs (LMS) are at most
 *   \f$ n / 2 \f$; once the LMS-suffixes are sorted, one left-to-right scan
 *   of the buckets of the first symbols places the L-suffixes and one
 *   right-to-left scan the S-suffixes. The LMS-substrings are sorted by the
 *   same induction, named, and the string of the names is solved
 *   recursively when two of them are equal. It takes \f$ O( n ) \f$ time,
 *   the smallest working space of the linear algorithms, and in practice it
 *   is two to three times faster than the Skew algorithm; with divsufsort,
 *   which also induces from a sorted sample, it is the method of the
 *   libraries in use;
 * - `suffixArrayScan()`, the scan-based algorithm of Baeza-Yates, Gonnet and
 *   Snider for the disk: \f$ T \f$ is cut into blocks of \f$ m \f$ symbols,
 *   scanned from left to right; the suffixes that start in a block are
 *   sorted by comparisons, each earlier suffix is placed among them by
 *   binary search, and the counts \f$ C[ j ] \f$ of the earlier suffixes
 *   that fall between the block suffixes \f$ j - 1 \f$ and \f$ j \f$ merge
 *   the two arrays in one scan of the older one. It makes
 *   \f$ O( n^2 \log m / m ) \f$ comparisons of suffixes, i.e., cubic time
 *   in the worst case, but all its passes are sequential, so that on disk
 *   it reads and writes only by scans.
 *
 * `lcpArray()` is the algorithm of Kasai, Lee, Arimura, Arikawa and Park:
 * \f$ \mathit{LCP}[ r ] \f$ is the length of the longest common prefix of
 * the suffixes \f$ \mathit{SA}[ r ] \f$ and \f$ \mathit{SA}[ r + 1 ] \f$,
 * and since the suffix \f$ i + 1 \f$ shares at least \f$ h - 1 \f$ symbols
 * with its predecessor in \f$ \mathit{SA} \f$ when the suffix \f$ i \f$
 * shares \f$ h \f$ with its own, the suffixes taken in the order of the
 * text extend \f$ h \f$ without going back, \f$ 2 n \f$ comparisons of
 * symbols in all.
 *
 * `SuffixArray` keeps \f$ T \f$, \f$ \mathit{SA} \f$, its inverse,
 * \f$ \mathit{LCP} \f$ and two more arrays of the lcp,
 * \f$ \mathit{Llcp} \f$ and \f$ \mathit{Rlcp} \f$, of the suffix in the
 * middle of each range of the binary search with the suffixes at its two
 * ends:
 *
 * - `range()` is the binary search, twice (for \f$ P \f$
 *   and for \f$ P \f$ followed by a symbol larger than all),
 *   \f$ O( p \log n ) \f$ time;
 * - `rangeLcp()` is the search of Manber and Myers: knowing the lcp
 *   \f$ l \f$ and \f$ r \f$ of \f$ P \f$ with the suffixes at the ends of
 *   the range, a step that compares \f$ l \f$ with
 *   \f$ \mathit{Llcp}[ M ] \f$ (or \f$ r \f$ with
 *   \f$ \mathit{Rlcp}[ M ] \f$) either decides without reading \f$ P \f$
 *   or compares from the place \f$ \max( l , r ) \f$, which never
 *   decreases; hence \f$ O( p + \log n ) \f$ comparisons of symbols;
 * - `lce( i , j )`, the longest common extension of the suffixes \f$ i \f$
 *   and \f$ j \f$, is the minimum of \f$ \mathit{LCP} \f$ between their
 *   ranks, a query of `RangeMinimum.h`, \f$ O( 1 ) \f$ time after
 *   \f$ O( n ) \f$ preprocessing.
 *
 * A text is a `std::string_view`, whose symbols are its bytes, or a vector
 * of integers in \f$ [ 0 , K ) \f$, e.g., several strings joined by
 * separators [see `SuffixApplications.h`]. The doubling needs
 * \f$ n < 2^{32} \f$, since the radix sort takes a pair of ranks as one key
 * of 64 bits.
 *
 * The contracts [see `Contract.h`]: the symbols of an integer text are less
 * than \f$ K \f$; every construction gives a suffix array, checked in
 * \f$ O( n ) \f$ by comparing each adjacent pair through the ranks of the
 * following suffixes (an audit), and `lcpArray()` the lcp of each adjacent
 * pair (an audit); the two searches give the same range. The functions are
 * defined in `SuffixArray.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10, Searching Strings by Substring,
 *      doi:10.1017/9781009128933
 * \see U. Manber, G. Myers, Suffix arrays: a new method for on-line string
 *      searches, SIAM Journal on Computing 22(5), 935-948, 1993,
 *      doi:10.1137/0222058
 * \see N. J. Larsson, K. Sadakane, Faster suffix sorting, Theoretical
 *      Computer Science 387(3), 258-272, 2007,
 *      doi:10.1016/j.tcs.2007.07.017
 * \see J. Kärkkäinen, P. Sanders, S. Burkhardt, Linear work suffix array
 *      construction, Journal of the ACM 53(6), 918-936, 2006,
 *      doi:10.1145/1217856.1217858
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
 * \see T. Kasai, G. Lee, H. Arimura, S. Arikawa, K. Park, Linear-time
 *      longest-common-prefix computation in suffix arrays and its
 *      applications, Proceedings of the 12th Symposium on Combinatorial
 *      Pattern Matching (CPM), LNCS 2089, 181-192, 2001,
 *      doi:10.1007/3-540-48194-X_17
 * \see G. H. Gonnet, R. A. Baeza-Yates, T. Snider, New indices for text:
 *      PAT trees and PAT arrays, in W. B. Frakes, R. A. Baeza-Yates (eds.),
 *      Information Retrieval: Data Structures and Algorithms, Prentice-Hall,
 *      66-82, 1992
 * \see S. J. Puglisi, W. F. Smyth, A. H. Turpin, A taxonomy of suffix array
 *      construction algorithms, ACM Computing Surveys 39(2), 4, 2007,
 *      doi:10.1145/1242471.1242472
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

#ifndef __ADS_SuffixArray
 #define __ADS_SuffixArray
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/sort/RadixSort.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a text on an integer alphabet
using Symbols = std::vector< std::size_t >;

/// the bytes of \f$ T \f$ as symbols in \f$ [ 0 , 256 ) \f$
[[nodiscard]] inline Symbols toSymbols( std::string_view T );

/// the suffix array of \f$ T \f$ by a sort of comparisons of its suffixes
///
/// In pseudocode (chapter 10, Searching Strings by Substring, of
/// Ferragina's book, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArraySorting}( T ) \cr
/// 2 & \quad \mathit{SA} \gets ( 0 , 1 , \ldots , n - 1 ) \cr
/// 3 & \quad \text{sort } \mathit{SA} \text{ by comparing the suffixes }
///     T[ \mathit{SA}[ r ] .. n - 1 ] \text{ symbol by symbol} \cr
/// 4 & \quad \textbf{return}\ \mathit{SA}
/// \end{array}}
/// \f]
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySorting(
                                                         const Symbols & T );

/// the array of `suffixArraySorting()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArraySorting}( T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArraySorting}(
///     \mathrm{toSymbols}( T ) )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::size_t > suffixArraySorting(
                                                        std::string_view T );

/// the suffix array of \f$ T \f$ by the prefix doubling of Manber and Myers
///
/// In pseudocode (the doubling of Manber and Myers, beyond the book), after
/// the round of \f$ k \f$ the rank \f$ R[ i ] \f$ being that of the prefix
/// of \f$ k \f$ symbols of the suffix \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayDoubling}( T ) \cr
/// 2 & \quad \mathit{SA} \gets \text{the places sorted by } T[ i ] \text{ by }
///     \text{radix sort};\ R[ i ] \gets \text{the rank of } T[ i ] \cr
/// 3 & \quad k \gets 1 \cr
/// 4 & \quad \textbf{while}\ R[ \mathit{SA}[ n - 1 ] ] < n - 1\ \textbf{and}
///     \ k < n\ \textbf{do} \cr
/// 5 & \qquad \mathit{key}( i ) \gets ( R[ i ] , R[ i + k ] + 1 )\quad
///     \triangleright\ \text{the second item 0 if } i + k \ge n \cr
/// 6 & \qquad \text{sort } \mathit{SA} \text{ by } \mathit{key} \text{ by }
///     \text{LSD radix sort} \cr
/// 7 & \qquad R'[ \mathit{SA}[ 0 ] ] \gets 0 \cr
/// 8 & \qquad \textbf{for}\ r \gets 1\ \textbf{to}\ n - 1\ \textbf{do}
///     \ R'[ \mathit{SA}[ r ] ] \gets R'[ \mathit{SA}[ r - 1 ] ] + [
///     \mathit{key}( \mathit{SA}[ r ] ) \ne \mathit{key}( \mathit{SA}[ r - 1
///     ] ) ] \cr
/// 9 & \qquad R \gets R';\ k \gets 2 k \cr
/// 10 & \quad \textbf{return}\ \mathit{SA}
/// \end{array}}
/// \f]
/// \pre \f$ |T| < 2^{32} \f$
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayDoubling(
                                                         const Symbols & T );

/// the array of `suffixArrayDoubling()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayDoubling}( T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArrayDoubling}(
///     \mathrm{toSymbols}( T ) )
/// \end{array}}
/// \f]
/// \pre \f$ |T| < 2^{32} \f$
[[nodiscard]] inline std::vector< std::size_t > suffixArrayDoubling(
                                                        std::string_view T );

/// the suffix array of \f$ T \f$ by the doubling of Larsson and Sadakane
///
/// In pseudocode (qsufsort of Larsson and Sadakane, beyond the book), a
/// group being a run of \f$ I \f$ whose suffixes share their first
/// \f$ h \f$ symbols and \f$ V[ i ] \f$ the last place of the group of the
/// suffix \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayLarssonSadakane}( T ) \cr
/// 2 & \quad I \gets \text{the places sorted stably by } T[ i ];\ \text{the }
///     \text{groups of equal } T[ i ] \cr
/// 3 & \quad h \gets 1 \cr
/// 4 & \quad \textbf{while}\  \text{some group has two suffixes or more}
///     \ \textbf{do} \cr
/// 5 & \qquad \textbf{foreach}\ \text{group } I[ a .. b ] \text{ of two }
///     \text{suffixes or more}\ \textbf{do} \cr
/// 6 & \qquad\quad \text{sort } I[ a .. b ] \text{ by } V[ I[ k ] + h ]\quad
///     \triangleright\ -1 \text{ past the end of } T \cr
/// 7 & \qquad\quad \text{split } I[ a .. b ] \text{ into the runs of equal }
///     \text{keys, and set } V \text{ of each suffix to the end of its run}
///     \cr
/// 8 & \qquad \text{join the runs of groups of one suffix, to be skipped} \cr
/// 9 & \qquad h \gets 2 h \cr
/// 10 & \quad \textbf{return}\ I
/// \end{array}}
/// \f]
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                         const Symbols & T );

/// the array of `suffixArrayLarssonSadakane()` above, on the bytes of
/// \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayLarssonSadakane}( T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArrayLarssonSadakane}(
///     \mathrm{toSymbols}( T ) )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                        std::string_view T );

/// the suffix array of \f$ T \f$ by the Skew algorithm of Kärkkäinen and
/// Sanders
///
/// In pseudocode (the Skew algorithm of Kärkkäinen and Sanders, beyond the
/// book), \f$ T \f$ followed by three symbols smaller than all and
/// \f$ \mathrm{rank}( i ) \f$ the rank of the suffix \f$ i \f$ among the
/// sample:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{skew}( T , K ) \cr
/// 2 & \quad S_{12} \gets \text{the places } i \text{ with } i \bmod 3 \ne
///     0 \text{, sorted by } T[ i .. i + 2 ] \text{ by three passes of} \cr
///   & \qquad \text{radix sort} \cr
/// 3 & \quad N \gets \text{the ranks of the triples, the places } i \bmod 3
///     = 1 \text{ first and } i \bmod 3 = 2 \text{ after} \cr
/// 4 & \quad \textbf{if}\  \text{two triples are equal}\ \textbf{then}
///     \ S_{12} \gets \text{the places of } \mathrm{skew}( N , \max N ) \cr
/// 5 & \quad S_0 \gets \text{the places } i \bmod 3 = 0 \text{, sorted by }
///     ( T[ i ] , \mathrm{rank}( i + 1 ) ) \text{ by radix sort} \cr
/// 6 & \quad \textbf{return}\ \text{the merge of } S_0 \text{ and } S_{12}
///     \text{, comparing } i \in S_0 \text{ with } j \in S_{12} \text{ by} \cr
///   & \qquad ( T[ i ] , \mathrm{rank}( i + 1 ) ) \text{ if } j \bmod 3 = 1
///     \text{, by } ( T[ i ] , T[ i + 1 ] , \mathrm{rank}( i + 2 ) )
///     \text{ if } j \bmod 3 = 2
/// \end{array}}
/// \f]
/// \pre every symbol of \f$ T \f$ is less than \f$ K \f$
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySkew(
                                          const Symbols & T , std::size_t K );

/// the array of `suffixArraySkew()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArraySkew}( T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArraySkew}( \mathrm{toSymbols}(
///     T ) , 256 )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::size_t > suffixArraySkew(
                                                        std::string_view T );

/// the suffix array of \f$ T \f$ by the induced sorting of Nong, Zhang and
/// Chan
///
/// In pseudocode (SA-IS of Nong, Zhang and Chan, beyond the book), on
/// \f$ s \f$, the text followed by a symbol smaller than all, the suffix
/// \f$ i \f$ of type S if it is smaller than the suffix \f$ i + 1 \f$ and
/// L otherwise, and LMS if it is of type S and the suffix \f$ i - 1 \f$ of
/// type L:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sais}( s , K ) \cr
/// 2 & \quad t[ n - 1 ] \gets \text{S};\ \textbf{for}\ i \gets n - 2
///     \ \textbf{downto}\ 0\ \textbf{do}\ t[ i ] \gets \text{S if } s[ i ] <
///     s[ i + 1 ] \text{ or } \cr
///   & \qquad ( s[ i ] = s[ i + 1 ] \text{ and } t[ i + 1 ] = \text{S),}
///     \text{ L otherwise} \cr
/// 3 & \quad \mathrm{induce}( \text{the LMS places in the order of the text} )
///     \quad \triangleright\ \text{sorts the LMS-substrings} \cr
/// 4 & \quad s_1 \gets \text{the names of the LMS-substrings, in the order }
///     \text{of the text, equal ones named alike} \cr
/// 5 & \quad \textbf{if}\  \text{two names are equal}\ \textbf{then}
///     \ \mathit{SA}_1 \gets \mathrm{sais}( s_1 , \text{the number of names} )
///     \cr
/// 6 & \quad \textbf{else}\ \mathit{SA}_1 \gets \text{the inverse of } s_1 \cr
/// 7 & \quad \mathrm{induce}( \text{the LMS places in the order of }
///     \mathit{SA}_1 ) \cr
/// 8 & \quad \textbf{return}\ \mathit{SA} \cr
/// 9 & \textbf{procedure}\ \mathrm{induce}( L ) \cr
/// 10 & \quad \mathit{SA} \gets \text{empty};\ \textbf{foreach}\ i \in L
///     \text{, from the last}\ \textbf{do}\ \text{put } i \text{ at the }
///     \text{tail of the bucket of } s[ i ] \cr
/// 11 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 12 & \qquad \textbf{if}\ j = \mathit{SA}[ r ] > 0\ \textbf{and}\ t[ j - 1
///     ] = \text{L}\ \textbf{then}\ \text{put } j - 1 \text{ at the head of }
///     \text{the bucket of } s[ j - 1 ] \cr
/// 13 & \quad \textbf{for}\ r \gets n - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
/// 14 & \qquad \textbf{if}\ j = \mathit{SA}[ r ] > 0\ \textbf{and}\ t[ j - 1
///     ] = \text{S}\ \textbf{then}\ \text{put } j - 1 \text{ at the tail of }
///     \text{the bucket of } s[ j - 1 ]
/// \end{array}}
/// \f]
/// \pre every symbol of \f$ T \f$ is less than \f$ K \f$
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySais(
                                          const Symbols & T , std::size_t K );

/// the array of `suffixArraySais()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArraySais}( T ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArraySais}( \mathrm{toSymbols}(
///     T ) , 256 )
/// \end{array}}
/// \f]
[[nodiscard]] inline std::vector< std::size_t > suffixArraySais(
                                                        std::string_view T );

/// the suffix array of \f$ T \f$ by the scan-based algorithm of
/// Baeza-Yates, Gonnet and Snider, on blocks of \f$ m \f$ symbols
///
/// In pseudocode (the algorithm of Baeza-Yates, Gonnet and Snider, beyond
/// the book), \f$ \mathit{SA} \f$ the suffix array of the suffixes that
/// start before the block:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayScan}( T , m ) \cr
/// 2 & \quad \mathit{SA} \gets (\,) \cr
/// 3 & \quad \textbf{foreach}\ s \in \{ 0 , m , 2 m , \ldots \} \text{ with }
///     s < n\ \textbf{do} \cr
/// 4 & \qquad B \gets \text{the suffixes that start in } T[ s .. s + m - 1 ]
///     \text{, sorted by comparisons} \cr
/// 5 & \qquad C[ 0 .. |B| ] \gets 0 \cr
/// 6 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ s - 1\ \textbf{do}
///     \ C[ \text{the number of suffixes of } B \text{ not greater than } j ]
///     \gets C[ \ldots ] + 1 \cr
/// 7 & \qquad \mathit{SA} \gets \text{for } q = 0 , \ldots , |B| \text{, the }
///     \text{next } C[ q ] \text{ entries of } \mathit{SA} \text{ followed}
///     \cr
///   & \qquad\quad \text{by } B[ q ] \text{ if } q < |B| \cr
/// 8 & \quad \textbf{return}\ \mathit{SA}
/// \end{array}}
/// \f]
/// \pre \f$ m > 0 \f$
/// \post a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayScan(
                                          const Symbols & T , std::size_t m );

/// the array of `suffixArrayScan()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArrayScan}( T , m ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{suffixArrayScan}( \mathrm{toSymbols}(
///     T ) , m )
/// \end{array}}
/// \f]
/// \pre \f$ m > 0 \f$
[[nodiscard]] inline std::vector< std::size_t > suffixArrayScan(
                                       std::string_view T , std::size_t m );

/// the inverse of a permutation: \f$ R[ \mathit{SA}[ r ] ] = r \f$
[[nodiscard]] inline std::vector< std::size_t > inversePermutation(
                                       const std::vector< std::size_t > & SA );

/// the LCP array of Kasai et al., \f$ n - 1 \f$ entries (none if
/// \f$ n < 2 \f$)
///
/// In pseudocode (the algorithm of Kasai et al., beyond the book), the lcp
/// of the suffix \f$ i + 1 \f$ with its predecessor in \f$ \mathit{SA} \f$
/// being at least \f$ h - 1 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcpArray}( T , \mathit{SA} ) \cr
/// 2 & \quad R \gets \text{the inverse of } \mathit{SA};\ h \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ R[ i ] = 0\ \textbf{then}\ h \gets 0;
///     \ \textbf{continue} \cr
/// 5 & \qquad k \gets \mathit{SA}[ R[ i ] - 1 ] \cr
/// 6 & \qquad \textbf{while}\ i + h < n\ \textbf{and}\ k + h < n
///     \ \textbf{and}\ T[ i + h ] = T[ k + h ]\ \textbf{do}\ h \gets h + 1 \cr
/// 7 & \qquad \mathit{LCP}[ R[ i ] - 1 ] \gets h \cr
/// 8 & \qquad \textbf{if}\ h > 0\ \textbf{then}\ h \gets h - 1 \cr
/// 9 & \quad \textbf{return}\ \mathit{LCP}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{SA} \f$ is a suffix array of \f$ T \f$ (an audit)
/// \post \f$ \mathit{LCP}[ r ] \f$ is the lcp of the suffixes
///       \f$ \mathit{SA}[ r ] \f$ and \f$ \mathit{SA}[ r + 1 ] \f$ (an
///       audit)
[[nodiscard]] inline std::vector< std::size_t > lcpArray(
                   const Symbols & T , const std::vector< std::size_t > & SA );

/// the array of `lcpArray()` above, on the bytes of \f$ T \f$
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcpArray}( T , \mathit{SA} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{lcpArray}( \mathrm{toSymbols}( T ) ,
///     \mathit{SA} )
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{SA} \f$ is a suffix array of \f$ T \f$ (an audit)
[[nodiscard]] inline std::vector< std::size_t > lcpArray(
                  std::string_view T , const std::vector< std::size_t > & SA );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SuffixArray ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the suffix array of a text with its \f$ \mathit{LCP} \f$,
/// \f$ \mathit{Llcp} \f$ and \f$ \mathit{Rlcp} \f$ arrays and the range
/// minima of \f$ \mathit{LCP} \f$
class SuffixArray {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of the empty text
 SuffixArray( void ) = default;

 /// the index of the bytes of \f$ T \f$, built by `suffixArraySais()` and
 /// `lcpArray()`
 ///
 /// In pseudocode (the search of Manber and Myers, beyond the book), the
 /// arrays \f$ \mathit{Llcp} \f$ and \f$ \mathit{Rlcp} \f$ filled by the
 /// call \f$ \mathrm{build}( -1 , n ) \f$, which gives the lcp of the
 /// suffixes at the ends of the range \f$ ( L , R ) \f$ of a binary search:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( L , R ) \cr
 /// 2 & \quad \textbf{if}\ R - L = 1\ \textbf{then return}\ \mathit{LCP}[ L
 ///     ] \text{ if } L \ge 0 \text{ and } R < n \text{, and 0 otherwise} \cr
 /// 3 & \quad M \gets \lfloor ( L + R ) / 2 \rfloor \cr
 /// 4 & \quad \mathit{Llcp}[ M ] \gets \mathrm{build}( L , M );
 ///     \ \mathit{Rlcp}[ M ] \gets \mathrm{build}( M , R ) \cr
 /// 5 & \quad \textbf{return}\ \min\{ \mathit{Llcp}[ M ] , \mathit{Rlcp}[ M ]
 ///     \}
 /// \end{array}}
 /// \f]
 explicit SuffixArray( std::string_view T );

 /// the index of an integer text, built as the one of the bytes above
 /// \pre every symbol of \f$ T \f$ is less than \f$ K \f$
 SuffixArray( Symbols T , std::size_t K );

 /// the range \f$ [ \mathit{first} , \mathit{last} ) \f$ of
 /// \f$ \mathit{SA} \f$ of the suffixes prefixed by \f$ P \f$, by two
 /// binary searches; the comparisons of symbols go to `*comparisons`, if
 /// given
 ///
 /// In pseudocode (chapter 10, Searching Strings by Substring, of
 /// Ferragina's book, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( P ) \cr
 /// 2 & \quad \textbf{return}\ ( \mathrm{bound}( P , \textbf{false} ) ,
 ///     \mathrm{bound}( P , \textbf{true} ) ) \cr
 /// 3 & \textbf{function}\ \mathrm{bound}( P , \mathit{upper} ) \cr
 /// 4 & \quad L \gets 0;\ R \gets n \cr
 /// 5 & \quad \textbf{while}\ L < R\ \textbf{do} \cr
 /// 6 & \qquad M \gets \lfloor ( L + R ) / 2 \rfloor \cr
 /// 7 & \qquad \textbf{if}\ P \text{, followed by a symbol larger than all }
 ///     \text{if } \mathit{upper} \text{, is greater than the suffix }
 ///     \mathit{SA}[ M ]\ \textbf{then} \cr
 /// 8 & \qquad\quad L \gets M + 1 \cr
 /// 9 & \qquad \textbf{else}\ R \gets M \cr
 /// 10 & \quad \textbf{return}\ L
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
                const Symbols & P , long * comparisons = nullptr ) const;

 /// the range of `range()` above, of the bytes of \f$ P \f$
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{range}( P ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{range}( \mathrm{toSymbols}( P ) )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
             std::string_view P , long * comparisons = nullptr ) const;

 /// the range of `range()` by the search of Manber and Myers,
 /// \f$ O( p + \log n ) \f$ comparisons of symbols
 ///
 /// In pseudocode (the search of Manber and Myers, beyond the book), the
 /// virtual ranks \f$ -1 \f$ and \f$ n \f$ at the ends, \f$ P \f$ greater
 /// than the suffix of rank \f$ L \f$ and sharing \f$ l \f$ symbols with it,
 /// not greater than the suffix of rank \f$ R \f$ and sharing \f$ r \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{boundLcp}( P , \mathit{upper} ) \cr
 /// 2 & \quad L \gets -1;\ R \gets n;\ l \gets 0;\ r \gets 0 \cr
 /// 3 & \quad \textbf{while}\ R - L > 1\ \textbf{do} \cr
 /// 4 & \qquad M \gets \lfloor ( L + R ) / 2 \rfloor \cr
 /// 5 & \qquad \textbf{if}\ l \ge r\ \textbf{then} \cr
 /// 6 & \qquad\quad \textbf{if}\ \mathit{Llcp}[ M ] > l\ \textbf{then}\ ( h ,
 ///     g ) \gets ( l , \textbf{true} ) \cr
 /// 7 & \qquad\quad \textbf{else if}\ \mathit{Llcp}[ M ] < l\ \textbf{then}
 ///     \ ( h , g ) \gets ( \mathit{Llcp}[ M ] , \textbf{false} ) \cr
 /// 8 & \qquad\quad \textbf{else}\ ( h , g ) \gets \text{the lcp of } P
 ///     \text{ and the suffix } \mathit{SA}[ M ] \text{, compared from } l
 ///     \text{,} \cr
 ///   & \qquad\qquad \text{and whether } P \text{ is greater} \cr
 /// 9 & \qquad \textbf{else}\ \text{the same with } r \text{, }
 ///     \mathit{Rlcp}[ M ] \text{ and the two outcomes swapped} \cr
 /// 10 & \qquad \textbf{if}\ g\ \textbf{then}\ L \gets M;\ l \gets h
 ///     \ \textbf{else}\ R \gets M;\ r \gets h \cr
 /// 11 & \quad \textbf{return}\ R
 /// \end{array}}
 /// \f]
 /// \post the range of `range( P )` (an audit)
 [[nodiscard]] std::pair< std::size_t , std::size_t > rangeLcp(
                const Symbols & P , long * comparisons = nullptr ) const;

 /// the range of `rangeLcp()` above, of the bytes of \f$ P \f$
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rangeLcp}( P ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{rangeLcp}( \mathrm{toSymbols}( P ) )
 /// \end{array}}
 /// \f]
 /// \post the range of `range( P )` (an audit)
 [[nodiscard]] std::pair< std::size_t , std::size_t > rangeLcp(
             std::string_view P , long * comparisons = nullptr ) const;

 /// the number of occurrences of \f$ P \f$, the size of the range of
 /// `rangeLcp()`
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of \f$ P \f$, in the order of
 /// \f$ \mathit{SA} \f$: the entries of the range of `rangeLcp()`
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the places of `locate()` above, of an integer pattern
 [[nodiscard]] std::vector< std::size_t > locate( const Symbols & P ) const;

 /// the length of the longest common prefix of the suffixes \f$ i \f$ and
 /// \f$ j \f$, \f$ O( 1 ) \f$
 ///
 /// In pseudocode (beyond the book), by a query of range minimum on
 /// \f$ \mathit{LCP} \f$, \f$ R \f$ the inverse of \f$ \mathit{SA} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lce}( i , j ) \cr
 /// 2 & \quad \textbf{if}\ i = n\ \textbf{or}\ j = n\ \textbf{then return}
 ///     \ 0 \cr
 /// 3 & \quad \textbf{if}\ i = j\ \textbf{then return}\ n - i \cr
 /// 4 & \quad a \gets \min\{ R[ i ] , R[ j ] \};\ b \gets \max\{ R[ i ] ,
 ///     R[ j ] \} \cr
 /// 5 & \quad \textbf{return}\ \min_{a \le r < b} \mathit{LCP}[ r ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$ and \f$ j \le n \f$, \f$ n \f$ the value of
 ///      `size()`
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// the rank of the suffix \f$ i \f$ in \f$ \mathit{SA} \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] std::size_t rank( std::size_t i ) const;

 /// the place \f$ \mathit{SA}[ r ] \f$ of the suffix of rank \f$ r \f$
 /// \pre \f$ r < n \f$
 [[nodiscard]] std::size_t operator[]( std::size_t r ) const;

 /// the length \f$ n \f$ of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_SA.size() ); }

 /// the text \f$ T \f$
 [[nodiscard]] const Symbols & text( void ) const { return( f_T ); }

 /// the array \f$ \mathit{LCP} \f$, \f$ n - 1 \f$ entries
 [[nodiscard]] const std::vector< std::size_t > & lcp( void ) const {
  return( f_lcp );
  }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of SA, random access

 using value_type = std::size_t;                  ///< a place of the text
 using size_type = std::size_t;                   ///< the type of the ranks
 using difference_type = std::ptrdiff_t;          ///< a difference of ranks
 using const_reference = const std::size_t &;     ///< an entry of SA
 using reference = const_reference;               ///< an entry of SA
 /// a constant iterator on the entries of SA
 using const_iterator = std::vector< std::size_t >::const_iterator;
 using iterator = const_iterator;                 ///< on the entries of SA

 /// the first entry of \f$ \mathit{SA} \f$
 [[nodiscard]] const_iterator begin( void ) const { return( f_SA.begin() ); }
 /// one past the last entry of \f$ \mathit{SA} \f$
 [[nodiscard]] const_iterator end( void ) const { return( f_SA.end() ); }
 /// the first entry of \f$ \mathit{SA} \f$
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// one past the last entry of \f$ \mathit{SA} \f$
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the rank, \f$ \mathit{LCP} \f$, \f$ \mathit{Llcp} \f$,
 /// \f$ \mathit{Rlcp} \f$ and the range minima, from \f$ T \f$ and
 /// \f$ \mathit{SA} \f$
 void index( void );

 /// the lcp of the ends of the range \f$ ( L , R ) \f$ of the binary
 /// search, the ends \f$ -1 \f$ and \f$ n \f$ having lcp 0, filling
 /// \f$ \mathit{Llcp} \f$ and \f$ \mathit{Rlcp} \f$ of the midpoints below
 std::size_t build( long L , long R );

 /// \f$ P \f$ against the suffix \f$ \mathit{SA}[ M ] \f$ from the place
 /// \f$ h \f$: the length of their common prefix and whether \f$ P \f$ is
 /// greater, \f$ P \f$ being followed by a symbol larger than all if
 /// `upper`
 std::pair< std::size_t , bool > compare( const Symbols & P , std::size_t M ,
                                          std::size_t h , bool upper ,
                                          long * comparisons ) const;

 /// the first rank whose suffix is not smaller than \f$ P \f$ (followed by
 /// the largest symbol if `upper`), by the binary search and by Manber
 /// and Myers
 std::size_t bound( const Symbols & P , bool upper , long * c ) const;
 std::size_t boundLcp( const Symbols & P , bool upper , long * c ) const;

 Symbols f_T;                           ///< the text
 std::vector< std::size_t > f_SA;       ///< the suffix array
 std::vector< std::size_t > f_rank;     ///< its inverse
 std::vector< std::size_t > f_lcp;      ///< LCP
 std::vector< std::size_t > f_Llcp;     ///< the lcp with the left end
 std::vector< std::size_t > f_Rlcp;     ///< the lcp with the right end
 RangeMinimum< std::size_t > f_rmq;     ///< on LCP

 };  // end( class SuffixArray )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SuffixArray.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuffixArray.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SuffixArray.h -------------------------*/
/*--------------------------------------------------------------------------*/
