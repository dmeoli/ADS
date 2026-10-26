/*--------------------------------------------------------------------------*/
/*--------------------------- File SuffixArray.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix array of a text T of n symbols, beyond the book: SA[ r ] is
 * the starting place of the r-th smallest suffix of T in the lexicographic
 * order, a suffix that is a prefix of another being the smaller. The
 * occurrences of a pattern P of p symbols are the suffixes prefixed by P,
 * which lie in a contiguous range of SA, found by binary search; the array
 * takes n integers besides T, against the 10 to 20 n bytes of a suffix tree
 * [see SuffixTree.h]. The positions count from 0, and no terminator is
 * stored: the chapter writes T with a final $, whose suffix would be the
 * first of SA.
 *
 * The constructions, each giving the same array:
 *
 * - suffixArraySorting(), Algorithm 10.3: the n suffixes sorted by a sort of
 *   comparisons (std::sort), each comparison as long as the common prefix,
 *   \f$ O( n \log n ) \f$ comparisons and \f$ O( n^2 \log n ) \f$ time in
 *   the worst case (a text of one repeated symbol), near \f$ O( n \log^2 n )
 *   \f$ on random texts;
 * - suffixArrayDoubling(), the prefix doubling of Manber and Myers: after
 *   round k each suffix has the rank of its prefix of \f$ 2^k \f$ symbols,
 *   and the pair of the ranks of the suffixes at i and \f$ i + 2^k \f$ is
 *   the rank of the prefix of \f$ 2^{k+1} \f$ symbols; the pairs are sorted
 *   by the LSD radix sort of RadixSort.h, \f$ O( n \log n ) \f$ time in all,
 *   or \f$ O( n \log L ) \f$ for L the longest repeated prefix;
 * - suffixArrayLarssonSadakane(), the doubling of Larsson and Sadakane
 *   (qsufsort): the same ranks, but a round sorts only the groups of
 *   suffixes that are still tied, each by the rank of the suffix h places
 *   ahead, and refines the ranks at once, so that the groups already sorted
 *   are skipped by a length stored in their first place. Their bound is
 *   \f$ O( n \log n ) \f$ with a ternary-split quicksort; with the sort of
 *   the library inside the groups it is \f$ O( n \log^2 n ) \f$ in the worst
 *   case, and close to linear on the usual texts;
 * - suffixArraySkew(), the Skew (DC3) algorithm of Karkkainen and Sanders:
 *   the suffixes at the places \f$ i \bmod 3 \ne 0 \f$ (the \f$ P_{2,0} \f$
 *   of the chapter, which counts from 1) are named by the ranks of their
 *   triples of symbols, sorted by three passes of radix sort, and sorted by
 *   the recursive suffix array of the string of the names, of 2 n / 3
 *   symbols, unless all the names differ; the suffixes at \f$ i \bmod 3 = 0
 *   \f$ are sorted by the pair ( T[ i ] , rank of i + 1 ); the two lists are
 *   merged by comparing a pair or a triple whose last item is a rank in the
 *   sample. \f$ T( n ) = T( 2 n / 3 ) + O( n ) = O( n ) \f$ on an integer
 *   alphabet of \f$ O( n ) \f$ symbols, and the reduction to sorting gives the
 *   optimal \f$ O( Sort( n ) ) \f$ I/Os on disk (Theorem 10.3);
 * - suffixArraySais(), the induced sorting of Nong, Zhang and Chan (SA-IS):
 *   each suffix is of type S or L as it is smaller or larger than the next
 *   one, and the leftmost S-suffixes of the runs (LMS) are at most n / 2; once
 *   the LMS-suffixes are sorted, one left-to-right scan of the buckets of the
 *   first symbols places the L-suffixes and one right-to-left scan the
 *   S-suffixes. The LMS-substrings are sorted by the same induction, named,
 *   and the string of the names is solved recursively when two of them are
 *   equal. \f$ O( n ) \f$ time, the smallest working space of the linear
 *   algorithms, and in practice two to three times faster than the Skew
 *   algorithm; with divsufsort, which also induces from a sorted sample, it is
 *   the method of the libraries in use;
 * - suffixArrayScan(), the scan-based algorithm of Baeza-Yates, Gonnet and
 *   Snider for the disk: T is cut into blocks of m symbols, scanned from
 *   left to right; the suffixes that start in a block are sorted by
 *   comparisons, each earlier suffix is placed among them by binary search,
 *   and the counts C[ j ] of the earlier suffixes that fall between the
 *   block suffixes j - 1 and j merge the two arrays in one scan of the
 *   older one. \f$ O( n^2 \log m / m ) \f$ comparisons of suffixes, i.e.,
 *   cubic time in the worst case, but all its passes are sequential
 *   (Theorem 10.4).
 *
 * lcpArray() is the algorithm of Kasai, Lee, Arimura, Arikawa and Park:
 * LCP[ r ] is the length of the longest common prefix of the suffixes
 * SA[ r ] and SA[ r + 1 ], and since the suffix i + 1 shares at least h - 1
 * symbols with its predecessor in SA when the suffix i shares h with its
 * own (Facts 10.1 and 10.2), the suffixes taken in the order of the text
 * extend h without going back, 2 n comparisons of symbols in all (Algorithm
 * 10.2).
 *
 * SuffixArray keeps T, SA, its inverse, LCP and two more arrays of the lcp,
 * Llcp and Rlcp, of the suffix in the middle of each range of the binary
 * search with the suffixes at its two ends:
 *
 * - range() is the binary search of Algorithm 10.1, twice (for P and for P
 *   followed by a symbol larger than all), \f$ O( p \log n ) \f$ time
 *   (Lemma 10.1);
 * - rangeLcp() is the search of Manber and Myers: knowing the lcp l and r
 *   of P with the suffixes at the ends of the range, a step that compares
 *   l with Llcp[ M ] (or r with Rlcp[ M ]) either decides without reading
 *   P or compares from the place max( l , r ), which never decreases, hence
 *   \f$ O( p + \log n ) \f$ comparisons of symbols (Lemma 10.2);
 * - lce( i , j ), the longest common extension of the suffixes i and j, is
 *   the minimum of LCP between their ranks, a query of RangeMinimum.h,
 *   \f$ O( 1 ) \f$ time after \f$ O( n ) \f$ preprocessing.
 *
 * A text is a std::string_view, whose symbols are its bytes, or a vector
 * of integers in [ 0 , K ), e.g., several strings joined by separators
 * [see SuffixApplications.h]. The doubling needs \f$ n < 2^{32} \f$, since
 * the radix sort takes a pair of ranks as one key of 64 bits.
 *
 * The contracts [see Contract.h]: the symbols of an integer text are less than
 * K; every construction gives a suffix array, checked in \f$ O( n ) \f$ by
 * comparing each adjacent pair through the ranks of the following suffixes (an
 * audit), and lcpArray() the lcp of each adjacent pair (an audit); the two
 * searches give the same range. The functions are defined in SuffixArray.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10
 * \see U. Manber, G. Myers, Suffix arrays: a new method for on-line string
 *      searches, SIAM Journal on Computing 22(5), 935-948, 1993,
 *      doi:10.1137/0222058
 * \see N. J. Larsson, K. Sadakane, Faster suffix sorting, Theoretical
 *      Computer Science 387(3), 258-272, 2007,
 *      doi:10.1016/j.tcs.2007.07.017
 * \see J. Karkkainen, P. Sanders, S. Burkhardt, Linear work suffix array
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

/// the bytes of T as symbols in [ 0 , 256 )
[[nodiscard]] inline Symbols toSymbols( std::string_view T );

/// the suffix array by a sort of comparisons, Algorithm 10.3
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySorting(
                                                         const Symbols & T );
[[nodiscard]] inline std::vector< std::size_t > suffixArraySorting(
                                                        std::string_view T );

/// the suffix array by the prefix doubling of Manber and Myers
/// \pre T.size() < 2^32
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayDoubling(
                                                         const Symbols & T );
[[nodiscard]] inline std::vector< std::size_t > suffixArrayDoubling(
                                                        std::string_view T );

/// the suffix array by the doubling of Larsson and Sadakane
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                         const Symbols & T );
[[nodiscard]] inline std::vector< std::size_t > suffixArrayLarssonSadakane(
                                                        std::string_view T );

/// the suffix array by the Skew algorithm of Karkkainen and Sanders
/// \pre every symbol of T is less than K
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySkew(
                                          const Symbols & T , std::size_t K );
[[nodiscard]] inline std::vector< std::size_t > suffixArraySkew(
                                                        std::string_view T );

/// the suffix array by the induced sorting of Nong, Zhang and Chan
/// \pre every symbol of T is less than K
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArraySais(
                                          const Symbols & T , std::size_t K );
[[nodiscard]] inline std::vector< std::size_t > suffixArraySais(
                                                        std::string_view T );

/// the suffix array by the scan-based algorithm of Baeza-Yates, Gonnet and
/// Snider, on blocks of m symbols
/// \pre m > 0
/// \post a suffix array of T (an audit)
[[nodiscard]] inline std::vector< std::size_t > suffixArrayScan(
                                          const Symbols & T , std::size_t m );
[[nodiscard]] inline std::vector< std::size_t > suffixArrayScan(
                                       std::string_view T , std::size_t m );

/// the inverse of a permutation: R[ SA[ r ] ] = r
[[nodiscard]] inline std::vector< std::size_t > inversePermutation(
                                       const std::vector< std::size_t > & SA );

/// the LCP array of Kasai et al., n - 1 entries (none if n < 2)
/// \pre SA is a suffix array of T (an audit)
/// \post LCP[ r ] is the lcp of the suffixes SA[ r ] and SA[ r + 1 ] (an
///       audit)
[[nodiscard]] inline std::vector< std::size_t > lcpArray(
                   const Symbols & T , const std::vector< std::size_t > & SA );
[[nodiscard]] inline std::vector< std::size_t > lcpArray(
                  std::string_view T , const std::vector< std::size_t > & SA );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SuffixArray ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the suffix array of a text with its LCP, Llcp and Rlcp arrays and the
/// range minima of LCP
class SuffixArray {

/*--------------------------------------------------------------------------*/
 public:

 SuffixArray( void ) = default;

 /// the index of the bytes of T, built by SA-IS and Kasai et al.
 explicit SuffixArray( std::string_view T );

 /// the index of an integer text
 /// \pre every symbol of T is less than K
 SuffixArray( Symbols T , std::size_t K );

 /// the range [ first , last ) of SA of the suffixes prefixed by P, by
 /// Algorithm 10.1; the comparisons of symbols go to *comparisons, if given
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
                const Symbols & P , long * comparisons = nullptr ) const;
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
             std::string_view P , long * comparisons = nullptr ) const;

 /// range() by the search of Manber and Myers, \f$ O( p + \log n ) \f$
 /// \post the range of range( P ) (an audit)
 [[nodiscard]] std::pair< std::size_t , std::size_t > rangeLcp(
                const Symbols & P , long * comparisons = nullptr ) const;
 [[nodiscard]] std::pair< std::size_t , std::size_t > rangeLcp(
             std::string_view P , long * comparisons = nullptr ) const;

 /// the number of occurrences of P
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of P, in the order of SA
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;
 [[nodiscard]] std::vector< std::size_t > locate( const Symbols & P ) const;

 /// the length of the longest common prefix of the suffixes i and j,
 /// \f$ O( 1 ) \f$
 /// \pre i <= size() and j <= size()
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// the rank of the suffix i in SA
 /// \pre i < size()
 [[nodiscard]] std::size_t rank( std::size_t i ) const;

 /// \pre r < size()
 [[nodiscard]] std::size_t operator[]( std::size_t r ) const;

 [[nodiscard]] std::size_t size( void ) const { return( f_SA.size() ); }

 [[nodiscard]] const Symbols & text( void ) const { return( f_T ); }

 [[nodiscard]] const std::vector< std::size_t > & lcp( void ) const {
  return( f_lcp );
  }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of SA, random access

 using value_type = std::size_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using const_reference = const std::size_t &;
 using reference = const_reference;
 using const_iterator = std::vector< std::size_t >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_SA.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_SA.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the rank, LCP, Llcp, Rlcp and the range minima, from T and SA
 void index( void );

 /// lcp( L , R ) for the range ( L , R ) of the binary search, the ends
 /// -1 and n having lcp 0, filling Llcp and Rlcp of the midpoints below
 std::size_t build( long L , long R );

 /// P against the suffix SA[ M ] from the place h: the length of their
 /// common prefix and whether P is greater, P being followed by a symbol
 /// larger than all if upper
 std::pair< std::size_t , bool > compare( const Symbols & P , std::size_t M ,
                                          std::size_t h , bool upper ,
                                          long * comparisons ) const;

 /// the first rank whose suffix is not smaller than P (P followed by the
 /// largest symbol if upper), by Algorithm 10.1 and by Manber and Myers
 std::size_t bound( const Symbols & P , bool upper , long * c ) const;
 std::size_t boundLcp( const Symbols & P , bool upper , long * c ) const;

 Symbols f_T;
 std::vector< std::size_t > f_SA;
 std::vector< std::size_t > f_rank;
 std::vector< std::size_t > f_lcp;
 std::vector< std::size_t > f_Llcp;
 std::vector< std::size_t > f_Rlcp;
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
