/*--------------------------------------------------------------------------*/
/*----------------------------- File FMIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The FM-index of Ferragina and Manzini (section 14.5 of the notes): the
 * transform of Burrows and Wheeler of s [see BurrowsWheeler.h] as an index
 * of its substrings, which replaces the suffix array and the text.
 *
 * - count( P ) is the backward search (Algorithm 14.3): the rows of M'
 *   prefixed by \f$ P[ i , p - 1 ] \f$ are a range [ first , last ), and
 *   those prefixed by \f$ c P[ i , p - 1 ] \f$, c = P[ i - 1 ], are
 *   [ C[ c ] + rank_c( first ) , C[ c ] + rank_c( last ) ), the LF-mapping
 *   of the first and of the last occurrence of c in L[ first , last ); so p
 *   steps, each two ranks, give the range of P and its number of
 *   occurrences, last - first, with no access to the text.
 * - locate( P ) finds the place of each row of the range: the rows of the
 *   suffixes that start at a multiple of \f$ \mu \f$ are marked, with their
 *   places, and from any row the LF-mapping, which moves one symbol to the
 *   left in s, reaches a marked row in t < \f$ \mu \f$ steps, the place
 *   being that of the marked row plus t; \f$ O( \mu ) \f$ steps per
 *   occurrence and \f$ O( ( n / \mu ) \log n ) \f$ bits of samples
 *   (Theorem 14.8).
 * - extract( i , j ) gives s[ i , j ) from the row of the first sampled
 *   suffix at or after j, by \f$ j - i + O( \mu ) \f$ steps of LF.
 *
 * The rank of a symbol in L, the number of its occurrences in a prefix, is
 * the count stored every b places for each symbol of the text plus a scan
 * of fewer than b symbols, 8 at a time by the zero bytes of a word, O( b )
 * time and \f$ \sigma ( n / b ) \f$ counts of 32 bits; the marks of the
 * rows are a vector of bits with the rank directory of RankSelect [see
 * RankSelect.h], so a row is tested and its sample found in O( 1 ) time.
 * The counts of L are the simplest rank on a sequence, which any structure
 * of rank and select on sequences can replace [see WaveletMatrix.h]: the
 * wavelet tree of Grossi, Gupta and Vitter
 * gives the rank in \f$ O( \log \sigma ) \f$ time on \f$ n H_0 + o( n \log
 * \sigma ) \f$ bits, and the compressed representations of Ferragina,
 * Manzini, Makinen and Navarro reach \f$ n H_k + o( n \log \sigma ) \f$
 * bits, the space of the compressed text (Lemma 14.3 and Theorem 14.7).
 * The transform is built from the suffix array of SA-IS, O( n ) time [see
 * BurrowsWheeler.h]. Against the suffix array, which counts by
 * binary search in \f$ O( p \log n ) \f$ time and \f$ n \log n \f$ bits
 * plus the text, the FM-index counts in O( p b ) time here, O( p ) with a
 * rank in O( 1 ), at the price of random accesses to L at each step; the
 * r-index of Gagie, Navarro and Prezza samples only the ends of the runs
 * of L, and takes space proportional to their number r, small on
 * repetitive texts.
 *
 * The contracts [see Contract.h]: b and \f$ \mu \f$ are positive, n is
 * below \f$ 2^{32} \f$, a pattern is not empty, \f$ i \le j \le n \f$; each
 * place found by locate() is an occurrence, an audit. The operations are
 * defined in FMIndex.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14
 * \see P. Ferragina, G. Manzini, Opportunistic data structures with
 *      applications, IEEE Symposium on Foundations of Computer Science,
 *      390-398, 2000, doi:10.1109/SFCS.2000.892127
 * \see P. Ferragina, G. Manzini, Indexing compressed text, Journal of the
 *      ACM 52(4), 552-581, 2005, doi:10.1145/1082036.1082039
 * \see R. Grossi, A. Gupta, J. S. Vitter, High-order entropy-compressed
 *      text indexes, ACM-SIAM Symposium on Discrete Algorithms, 841-850,
 *      2003
 * \see P. Ferragina, G. Manzini, V. Makinen, G. Navarro, Compressed
 *      representations of sequences and full-text indexes, ACM Transactions
 *      on Algorithms 3(2), 20, 2007, doi:10.1145/1240233.1240243
 * \see G. Navarro, V. Makinen, Compressed full-text indexes, ACM Computing
 *      Surveys 39(1), 2, 2007, doi:10.1145/1216370.1216372
 * \see T. Gagie, G. Navarro, N. Prezza, Fully functional suffix trees and
 *      optimal text searching in BWT-runs bounded space, Journal of the ACM
 *      67(1), 2, 2020, doi:10.1145/3375890
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

#ifndef __ADS_FMIndex
 #define __ADS_FMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS FMIndex ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the FM-index of a text: its transform with the counts for the rank every
/// b places and the places of the suffixes at the multiples of mu
class FMIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of s
 /// \pre b > 0, mu > 0, s.size() < 2^32
 explicit FMIndex( std::string_view s , std::size_t b = 64 ,
                   std::size_t mu = 32 );

 /// the length n of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the transform of the text
 [[nodiscard]] BWT transform( void ) const;

 /// the occurrences of c in L[ 0 , k ), the $ excluded
 /// \pre k <= n + 1
 [[nodiscard]] std::size_t rank( char c , std::size_t k ) const;

 /// the range [ first , last ) of the rows of M' prefixed by P, empty if
 /// none, by backward search
 /// \pre P not empty
 [[nodiscard]] std::pair< std::size_t , std::size_t > range(
                                                  std::string_view P ) const;

 /// the number of the occurrences of P
 /// \pre P not empty
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of P, in increasing order
 /// \pre P not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// s[ i , j )
 /// \pre i <= j <= n
 [[nodiscard]] std::string extract( std::size_t i , std::size_t j ) const;

 /// the bits of the index: L, the counts, the marks with their directory,
 /// the samples and the arrays of the alphabet
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the row of the suffix one place to the left of that of row r
 /// \pre r is not the row of the suffix 0
 [[nodiscard]] std::size_t lf( std::size_t r ) const;

 /// the place in s of the suffix of row r
 [[nodiscard]] std::size_t place( std::size_t r ) const;

 /// whether row r is marked
 [[nodiscard]] bool marked( std::size_t r ) const { return( f_mark[ r ] ); }

 std::size_t f_n , f_b , f_mu;
 std::string f_L;                       ///< L, a 0 byte for the $
 std::size_t f_dollar;                  ///< the row of the $
 std::array< std::size_t , 257 > f_C;   ///< 1 + the symbols below c
 std::array< int , 256 > f_id;          ///< the index of a symbol, or -1
 std::size_t f_sigma = 0;               ///< the distinct symbols
 std::vector< std::uint32_t > f_occ;    ///< the counts every b rows
 RankSelect f_mark;                     ///< the marked rows, with rank
 std::vector< std::size_t > f_place;    ///< the places of the marked rows
 std::vector< std::size_t > f_row;      ///< the row of the suffix k mu

 };  // end( class FMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/FMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FMIndex.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File FMIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
