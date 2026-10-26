/*--------------------------------------------------------------------------*/
/*------------------------------ File RIndex.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The r-index of Gagie, Navarro and Prezza: the FM-index of a text s of n
 * bytes in space proportional to the number r of the runs of its
 * transform L of Burrows and Wheeler, rather than to n [see FMIndex.h,
 * HuffmanFMIndex.h], so that a repetitive text (the versions of a
 * document, the genomes of a species), where r is far smaller than n,
 * takes far less; count( P ) in \f$ O( p ) \f$ steps of rank on the runs
 * and locate( P ) in \f$ O( p + occ ) \f$ more of predecessor searches.
 *
 * L is kept as its runs: the start of each run in an Elias-Fano sequence
 * [see EliasFano.h], the symbol of each run (its head) in a wavelet tree
 * of Huffman [see HuffmanWaveletTree.h], and the starts of the runs in the
 * column F, the runs of each symbol c after those of the symbols before
 * it, in one more Elias-Fano sequence: the run k of c starts in F at
 * \f$ C[ c ] + S_c[ k ] \f$, \f$ S_c[ k ] \f$ the lengths of the first k
 * runs of c, at the place \f$ R_c + k \f$ of the sequence, \f$ R_c \f$ the
 * runs of the symbols below c (one sequence of r items in
 * \f$ [ 0 , n ] \f$ rather than one for each symbol, which would cost the
 * entropy of the heads more per run). The rank of c in L[ 0 , i ) is then
 * \f[ rank_c( i ) = F_{R_c + k} - C[ c ] + [ h_j = c ] \, ( i - p_j ) ,
 *     \qquad k = rank_c( h , j ) , \f]
 * with j the run of the place i - 1, \f$ p_j \f$ its start and
 * \f$ h_j \f$ its head (one predecessor search, one descent of the tree
 * for both ranks of c around j), and count( P ) is the backward search of
 * the FM-index with it.
 *
 * locate( P ) keeps, along the backward search, the place in s of the
 * suffix of the last row of the range, \f$ SA[ e - 1 ] \f$ (the toehold):
 * if \f$ L[ e - 1 ] = c \f$ the row moves by LF and the place decreases by
 * 1, and else the last c before e - 1 ends a run of c, whose place is kept
 * (a sample of SA at each end of a run). The other occurrences follow from
 * the function \f$ \phi( SA[ i ] ) = SA[ i - 1 ] \f$, which shifts every
 * stretch between two places of the starts of the runs alike:
 * \f[ \phi( x ) = \phi( y ) + ( x - y ) , \qquad y = \max \{ SA[ p_j ] :
 *     SA[ p_j ] \le x \} , \f]
 * so that the places \f$ SA[ p_j ] \f$ of the starts of the runs, sorted, in
 * Elias-Fano, give it by one predecessor search: \f$ \phi( SA[ p_j ] ) = SA[
 * p_j - 1 ] \f$ is the sample at the end of the run j - 1, and only j is kept,
 * in \f$ \lceil \log_2 r \rceil \f$ bits. All of it takes \f$ O( r ) \f$
 * words: 5 sequences of r integers of \f$ \lceil \log_2 n \rceil \f$ bits or
 * less and the heads, whose rank and select are those of CompactRankSelect
 * [see CompactRankSelect.h], the smallest.
 *
 * The symbols of L are the bytes plus 1, the end of the text being 0. The
 * transform comes from the suffix array of SA-IS [see BurrowsWheeler.h],
 * \f$ O( n ) \f$ time and words; the r-index of the literature is built in
 * \f$ O( r ) \f$ words from the run-length transform, which this one does not
 * attempt.
 *
 * The contracts [see Contract.h]: n is below \f$ 2^{32} \f$, a pattern is
 * not empty; each place found by locate() is an occurrence, an audit
 * against the text kept for it only at the level of the audits. The
 * functions are defined in RIndex.tpp.
 *
 * \see T. Gagie, G. Navarro, N. Prezza, Fully functional suffix trees and
 *      optimal text searching in BWT-runs bounded space, Journal of the ACM
 *      67(1), 2, 2020, doi:10.1145/3375890
 * \see T. Gagie, G. Navarro, N. Prezza, Optimal-time text indexing in
 *      BWT-runs bounded space, Proc. 29th SODA, 1459-1477, 2018,
 *      doi:10.1137/1.9781611975031.96
 * \see V. Makinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see G. Navarro, Indexing highly repetitive string collections, part
 *      II: compressed indexes, ACM Computing Surveys 54(2), 26, 2021,
 *      doi:10.1145/3432999
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

#ifndef __ADS_RIndex
 #define __ADS_RIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"
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
/*------------------------------ CLASS RIndex ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the r-index of a text: count and locate in space proportional to the
/// runs of its transform
class RIndex {

/*--------------------------------------------------------------------------*/
 public:

 RIndex( void ) : RIndex( std::string_view() ) {}

 /// the index of s
 /// \pre s.size() < 2^32
 explicit RIndex( std::string_view s );

 /// the length n of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number r of the runs of the transform
 [[nodiscard]] std::size_t runs( void ) const { return( f_start.size() ); }

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

 /// the bits of the index: the runs, their heads, the sums of their
 /// lengths, the samples and the function phi
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the occurrences of c in L[ 0 , i )
 [[nodiscard]] std::size_t rank( std::uint32_t c , std::size_t i ) const;

 /// phi( x ), the place of the suffix of the row before that of x
 [[nodiscard]] std::size_t phi( std::size_t x ) const;

 /// the item i of a sequence of fixed width w
 [[nodiscard]] static std::size_t get( const BitVector & v , int w ,
                                       std::size_t i ) {
  return( std::size_t( v.get( i * std::size_t( w ) , w ) ) );
  }

 std::size_t f_n = 0;
 int f_w = 1;                            ///< the bits of a place or a row
 EliasFano f_start;                      ///< the start of each run
 HuffmanWaveletTree< CompactRankSelect > f_head;   ///< the head of each run
 EliasFano f_F;                          ///< the starts in F, by symbol
 std::array< std::size_t , 258 > f_R{};  ///< the runs of the symbols below
 std::array< std::size_t , 258 > f_C{};  ///< the symbols of L below each
 BitVector f_end;                        ///< SA at the end of each run
 EliasFano f_key;                        ///< SA at the starts, sorted
 BitVector f_phi;                        ///< the run of each of them
 int f_wr = 1;                           ///< the bits of a run
#if ADS_CONTRACTS >= 3
 std::string f_text;                     ///< the text, for the audits
#endif

 };  // end( class RIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/RIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RIndex.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File RIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
