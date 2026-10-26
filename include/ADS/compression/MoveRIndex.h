/*--------------------------------------------------------------------------*/
/*---------------------------- File MoveRIndex.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The r-index on move structures, after the move-r of Bertram, Fischer and
 * Nalbach: the operations of the r-index [see RIndex.h] with each step in
 * \f$ O( 1 ) \f$ time instead of a predecessor search, from two move
 * structures of Nishimoto and Tabei [see MoveStructure.h], at the price of a
 * few more words per run of the transform L of Burrows and Wheeler of the
 * text.
 *
 * The LF-mapping shifts each run of L alike onto the column F, and is a move
 * structure on the runs; the backward search keeps the first and the last row
 * of the range, [ s , e ], with their intervals, and for the symbol c moves s
 * to the first row at or after it whose symbol is c, the start of the next
 * interval of c if its own is not of c, and e to the last one at or before it,
 * the end of the previous interval of c (the rank and select of c on the tags
 * of the intervals, in a wavelet tree of Huffman on RankSelect [see
 * HuffmanWaveletTree.h], the fastest), and then maps both by LF in
 * \f$ O( 1 ) \f$ time each; count( P ) takes \f$ O( p ) \f$ steps and the
 * ranks of the intervals whose tag is not c.
 *
 * The function \f$ \phi( SA[ i ] ) = SA[ i - 1 ] \f$, taken cyclically
 * (\f$ \phi( SA[ 0 ] ) = SA[ n ] \f$), is a permutation of the places of
 * the text that shifts each stretch between two places of the starts of
 * the runs alike, and is a move structure too; locate( P ) keeps
 * \f$ SA[ e ] \f$ along the search (the toehold, from the samples of SA at
 * the end of each interval, with the interval of \f$ \phi \f$ of each),
 * and then lists the occurrences by \f$ e - s \f$ steps of \f$ \phi \f$,
 * each in \f$ O( 1 ) \f$ time.
 *
 * The symbols of L are the bytes plus 1, the end of the text being 0; the
 * transform comes from the suffix array of SA-IS [see BurrowsWheeler.h].
 *
 * The contracts [see Contract.h]: n is below \f$ 2^{32} \f$, a pattern is
 * not empty; each place found by locate() is an occurrence, an audit
 * against the text kept for it only at the level of the audits. The
 * functions are defined in MoveRIndex.tpp.
 *
 * \see N. Bertram, J. Fischer, L. Nalbach, Move-r: optimizing the r-index,
 *      Proc. 22nd SEA, LIPIcs 301, 1, 2024, doi:10.4230/LIPIcs.SEA.2024.1
 * \see T. Nishimoto, Y. Tabei, Optimal-time queries on BWT-runs compressed
 *      indexes, Proc. 48th ICALP, LIPIcs 198, 101, 2021,
 *      doi:10.4230/LIPIcs.ICALP.2021.101
 * \see T. Gagie, G. Navarro, N. Prezza, Fully functional suffix trees and
 *      optimal text searching in BWT-runs bounded space, Journal of the ACM
 *      67(1), 2, 2020, doi:10.1145/3375890
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

#ifndef __ADS_MoveRIndex
 #define __ADS_MoveRIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"
#include "ADS/succinct/HuffmanWaveletTree.h"
#include "ADS/succinct/MoveStructure.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MoveRIndex ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the r-index of a text on move structures: count and locate with each
/// step in \f$ O( 1 ) \f$ time
class MoveRIndex {

/*--------------------------------------------------------------------------*/
 public:

 MoveRIndex( void ) : MoveRIndex( std::string_view() ) {}

 /// the index of s
 /// \pre s.size() < 2^32
 explicit MoveRIndex( std::string_view s );

 /// the length n of the text
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number of the intervals of LF, after the balance
 [[nodiscard]] std::size_t intervals( void ) const {
  return( f_lf.intervals() );
  }

 /// the number of the occurrences of P
 /// \pre P not empty
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of P, in increasing order
 /// \pre P not empty
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the bits of the index: the two move structures, the ranks of the tags
 /// and the samples
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the backward search of P: the rows [ s , e ] with their intervals,
 /// and the place of the suffix of e with its interval of phi if Toehold;
 /// false if P does not occur
 template< bool Toehold >
 [[nodiscard]] bool search( std::string_view P , std::uint64_t & s ,
                            std::size_t & is , std::uint64_t & e ,
                            std::size_t & ie , std::uint64_t & v ,
                            std::size_t & iv ) const;

 std::size_t f_n = 0;
 int f_w = 1 , f_wi = 1;                 ///< a place, an interval of phi
 MoveStructure f_lf;                     ///< LF on the runs
 HuffmanWaveletTree< RankSelect > f_tag;   ///< their tags, ranked
 std::array< std::size_t , 257 > f_cnt{};   ///< the intervals of each tag
 MoveStructure f_phi;                    ///< phi on the places
 BitVector f_end;                        ///< SA at the end of each interval
 BitVector f_endPhi;                     ///< the interval of phi of it - 1
#if ADS_CONTRACTS >= 3
 std::string f_text;                     ///< the text, for the audits
#endif

 };  // end( class MoveRIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/MoveRIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MoveRIndex.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MoveRIndex.h -------------------------*/
/*--------------------------------------------------------------------------*/
