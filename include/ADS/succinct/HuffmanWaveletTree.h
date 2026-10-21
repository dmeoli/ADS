/*--------------------------------------------------------------------------*/
/*------------------------ File HuffmanWaveletTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The wavelet tree shaped by the code of Huffman of Makinen and Navarro: a
 * sequence \f$ x_0 , \ldots , x_{n-1} \f$ of symbols of \f$ [ 0 , \sigma )
 * \f$ in \f$ n ( H_0 + 1 ) \f$ bits and a little more, with access, rank
 * and select of any symbol in time proportional to the length of its code,
 * \f$ H_0 + 1 \f$ steps on average, against the \f$ \lceil \log_2 \sigma
 * \rceil \f$ levels of the balanced tree and of the wavelet matrix [see
 * WaveletMatrix.h], which take \f$ n \lceil \log_2 \sigma \rceil \f$ bits.
 *
 * The symbol c has the canonical code of Huffman of its frequency in x,
 * of length \f$ \ell_c \f$, its bits from the highest; the tree is the
 * trie of the codes, and its internal node v, reached by the prefix
 * \f$ \pi_v \f$, keeps the bit after \f$ \pi_v \f$ of the code of each
 * item whose code starts with \f$ \pi_v \f$, in the order of the
 * sequence. The bits of all the nodes are one vector B with rank and
 * select [see RankSelect.h, CompactRankSelect.h], node v from the place
 * \f$ o_v \f$ with \f$ R_v = rank_1( B , o_v ) \f$ kept; the item of place
 * i in the node v moves to the place
 * \f[ i' = rank_b( B , o_v + i ) - rank_b( B , o_v ) \f]
 * of its child by the bit \f$ b = B[ o_v + i ] \f$, so that a descent
 * along the code of c gives access( i ), with the rank of the symbol
 * found (accessRank(), the LF-mapping of an FM-index in one descent), and
 * \f$ rank_c( i ) \f$, one rank a step, and \f$ select_c( k ) \f$ climbs
 * from the leaf of c by one select a step,
 * \f[ k' = select_b( B , rank_b( B , o_v ) + k ) - o_v + 1 . \f]
 * The bits of B are \f$ \sum_c f_c \ell_c < n ( H_0 + 1 ) \f$ for the
 * frequencies \f$ f_c \f$, by the bound of the code of Huffman, plus the
 * directory of rank and select, 25% of them with RankSelect, 3.5% with
 * CompactRankSelect (the parameter RS: time against bits), and
 * \f$ O( \sigma ) \f$ words of the tree and of the codes.
 *
 * The contracts [see Contract.h]: the places and the counts are in range,
 * select asks for an occurrence that exists (and the tree was built with
 * select); the audit of the construction
 * is that access() gives back each item. The functions are defined in
 * HuffmanWaveletTree.tpp.
 *
 * \see V. Makinen, G. Navarro, Succinct suffix arrays based on run-length
 *      encoding, Proc. 16th CPM, LNCS 3537, 45-56, 2005,
 *      doi:10.1007/11496656_5
 * \see R. Grossi, A. Gupta, J. S. Vitter, High-order entropy-compressed
 *      text indexes, Proc. 14th SODA, 841-850, 2003
 * \see D. A. Huffman, A method for the construction of
 *      minimum-redundancy codes, Proceedings of the IRE 40(9), 1098-1101,
 *      1952, doi:10.1109/JRPROC.1952.273898
 * \see G. Navarro, Wavelet trees for all, Journal of Discrete Algorithms
 *      25, 2-20, 2014, doi:10.1016/j.jda.2013.07.004
 * \see G. Navarro, Compact Data Structures, Cambridge University Press,
 *      2016, section 6.2.4, doi:10.1017/CBO9781316588284
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

#ifndef __ADS_HuffmanWaveletTree
 #define __ADS_HuffmanWaveletTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS HuffmanWaveletTree ------------------------*/
/*--------------------------------------------------------------------------*/

/// a static sequence of symbols in about H0 + 1 bits each, with access,
/// rank and select in time proportional to the length of a code; RS is the
/// rank and select of its bits, RankSelect or CompactRankSelect
template< class RS = RankSelect >
class HuffmanWaveletTree {

/*--------------------------------------------------------------------------*/
 public:

 HuffmanWaveletTree( void ) = default;

 /// the sequence x, of symbols below its largest one plus 1, with select
 /// if asked for (else the directory of RankSelect keeps no inventory)
 /// \pre x.size() < 2^40
 explicit HuffmanWaveletTree( std::span< const std::uint32_t > x ,
                              bool withSelect = true );

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the symbols are below sigma()
 [[nodiscard]] std::size_t sigma( void ) const { return( f_len.size() ); }

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint32_t access( std::size_t i ) const;

 [[nodiscard]] std::uint32_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the item i and its occurrences in [ 0 , i ), by one descent
 /// \pre i < size()
 [[nodiscard]] std::pair< std::uint32_t , std::size_t > accessRank(
                                                       std::size_t i ) const;

 /// the occurrences of c in [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank( std::uint32_t c , std::size_t i ) const;

 /// the occurrences of c in [ 0 , i ) and in [ 0 , j ), by one descent
 /// \pre i <= j <= size()
 [[nodiscard]] std::pair< std::size_t , std::size_t > rank( std::uint32_t c ,
                                                           std::size_t i ,
                                                           std::size_t j )
  const;

 /// the place of the k-th occurrence of c
 /// \pre 1 <= k <= rank( c , size() ), and select was asked for
 [[nodiscard]] std::size_t select( std::uint32_t c , std::size_t k ) const;

 /// the length of the code of c, 0 if c is not in the sequence
 [[nodiscard]] int codeLength( std::uint32_t c ) const {
  return( c < f_len.size() ? f_len[ c ] : 0 );
  }

 /// the bits of the tree: its vector with the directory, the nodes and the
 /// codes
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// an internal node: the place of its bits in B and the 1s before them,
 /// and its children by the bits 0 and 1, a node or the leaf -1 - c
 struct Node {
  std::uint64_t off = 0 , ones = 0;
  std::int32_t child[ 2 ] = { 0 , 0 };
  };

 /// the rank and select of the bits b, with select if asked for
 [[nodiscard]] static RS directory( BitVector b , bool withSelect ) {
  if constexpr( std::is_same_v< RS , RankSelect > )
   return( RS( std::move( b ) , withSelect ? RankSelect::Selects::BOTH
                                           : RankSelect::Selects::NONE ) );
  else return( RS( std::move( b ) ) );
  }

 /// the code of c, its bit d (from the highest)
 [[nodiscard]] bool bit( std::uint32_t c , int d ) const {
  return( ( f_code[ c ] >> ( f_len[ c ] - 1 - d ) ) & 1 );
  }

 std::size_t f_n = 0;
 std::vector< std::uint64_t > f_code;   ///< the code of each symbol
 std::vector< std::uint8_t > f_len;     ///< its length, 0 if absent
 std::vector< Node > f_node;            ///< the root first
 RS f_B;                                ///< the bits of all the nodes

 };  // end( class HuffmanWaveletTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/HuffmanWaveletTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HuffmanWaveletTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File HuffmanWaveletTree.h ---------------------*/
/*--------------------------------------------------------------------------*/
