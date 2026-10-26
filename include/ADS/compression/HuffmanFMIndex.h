/*--------------------------------------------------------------------------*/
/*-------------------------- File HuffmanFMIndex.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The FM-index on the wavelet tree shaped by the code of Huffman, the
 * succinct suffix array of Makinen and Navarro: the transform L of
 * Burrows and Wheeler of a text s of n bytes as a HuffmanWaveletTree [see
 * HuffmanWaveletTree.h], in \f$ n ( H_0 + 1 ) ( 1 + o( 1 ) ) \f$ bits, with
 * the same operations as the FM-index of the notes [see FMIndex.h], which
 * keeps L as it is with counts every b places, \f$ 8 n + 32 \sigma n / b
 * \f$ bits.
 *
 * - count( P ) is the backward search: the rows prefixed by \f$ c P' \f$
 *   are \f$ [ C[ c ] + rank_c( first ) , C[ c ] + rank_c( last ) ) \f$ for
 *   the range [ first , last ) of P', in \f$ O( p ( H_0 + 1 ) ) \f$ steps
 *   on average, each one a rank of the bits of the tree;
 * - locate( P ) walks each row of the range by the LF-mapping,
 *   \f$ LF( r ) = C[ L[ r ] ] + rank_{L[ r ]}( r ) \f$, found by one
 *   descent of the tree (accessRank()), until a row whose suffix starts at
 *   a multiple of \f$ \mu \f$, marked and sampled;
 * - extract( i , j ) starts from the row of the sampled suffix at or after
 *   j and walks back by LF.
 *
 * The symbols of L are the bytes plus 1, the end of the text being 0.
 * The backward search takes the two ranks of a step by one descent of the
 * tree, as they meet the same nodes. The samples are
 * \f$ \lceil n / \mu \rceil \f$ places and as many rows of
 * \f$ \lceil \log_2 ( n + 1 ) \rceil \f$ bits each, and the marks n + 1
 * bits with the compact rank of CompactRankSelect [see
 * CompactRankSelect.h], since a step of locate() tests the bit alone and
 * the rank is needed once per occurrence; the parameter RS is the rank and
 * select of the tree, RankSelect or CompactRankSelect. The transform comes
 * from the suffix array of SA-IS [see BurrowsWheeler.h], O( n ) time.
 *
 * The contracts [see Contract.h]: \f$ \mu > 0 \f$, n is below
 * \f$ 2^{32} \f$, a pattern is not empty, \f$ i \le j \le n \f$; each
 * place found by locate() is an occurrence, an audit. The functions are
 * defined in HuffmanFMIndex.tpp.
 *
 * \see V. Makinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see P. Ferragina, G. Manzini, Indexing compressed text, Journal of the
 *      ACM 52(4), 552-581, 2005, doi:10.1145/1082036.1082039
 * \see P. Ferragina, G. Manzini, V. Makinen, G. Navarro, Compressed
 *      representations of sequences and full-text indexes, ACM
 *      Transactions on Algorithms 3(2), 20, 2007,
 *      doi:10.1145/1240233.1240243
 * \see G. Navarro, V. Makinen, Compressed full-text indexes, ACM Computing
 *      Surveys 39(1), 2, 2007, doi:10.1145/1216370.1216372
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 14
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

#ifndef __ADS_HuffmanFMIndex
 #define __ADS_HuffmanFMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/HuffmanWaveletTree.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS HuffmanFMIndex --------------------------*/
/*--------------------------------------------------------------------------*/

/// the FM-index of a text on the wavelet tree of Huffman of its transform,
/// with the places of the suffixes at the multiples of mu
template< class RS = RankSelect >
class HuffmanFMIndex {

/*--------------------------------------------------------------------------*/
 public:

 /// the index of s
 /// \pre mu > 0, s.size() < 2^32
 explicit HuffmanFMIndex( std::string_view s , std::size_t mu = 32 );

 /// the length n of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the range [ first , last ) of the rows prefixed by P, empty if none
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

 /// the bits of the index: the tree, the marks with their directory, the
 /// samples and the counts of the symbols
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place in s of the suffix of row r
 [[nodiscard]] std::size_t place( std::size_t r ) const;

 std::size_t f_n = 0 , f_mu = 1;
 int f_w = 1;                            ///< the bits of a place or a row
 HuffmanWaveletTree< RS > f_L;           ///< L, the bytes plus 1, 0 the $
 std::array< std::size_t , 258 > f_C{};  ///< the symbols of L below each
 CompactRankSelect f_mark;               ///< the rows of the samples
 BitVector f_place;                      ///< their places, by row
 BitVector f_row;                        ///< the row of the suffix k mu

 };  // end( class HuffmanFMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/HuffmanFMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HuffmanFMIndex.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HuffmanFMIndex.h -----------------------*/
/*--------------------------------------------------------------------------*/
