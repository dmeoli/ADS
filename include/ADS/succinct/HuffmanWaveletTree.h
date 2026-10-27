/*--------------------------------------------------------------------------*/
/*------------------------ File HuffmanWaveletTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The wavelet tree shaped by the code of Huffman of Mäkinen and Navarro: a
 * sequence \f$ x_0 , \ldots , x_{n-1} \f$ of symbols of
 * \f$ [ 0 , \sigma ) \f$ in \f$ n ( H_0 + 1 ) \f$ bits and a little more,
 * with access, rank and select of any symbol in time proportional to the
 * length of its code, \f$ H_0 + 1 \f$ steps on average, against the
 * \f$ \lceil \log_2 \sigma \rceil \f$ levels of the balanced tree and of the
 * wavelet matrix [see `WaveletMatrix.h`], which take
 * \f$ n \lceil \log_2 \sigma \rceil \f$ bits.
 *
 * The symbol \f$ c \f$ has the canonical code of Huffman of its frequency in
 * \f$ x \f$, of length \f$ \ell_c \f$, its bits from the highest; the tree is
 * the trie of the codes, and its internal node \f$ v \f$, reached by the
 * prefix \f$ \pi_v \f$, keeps the bit after \f$ \pi_v \f$ of the code of
 * each item whose code starts with \f$ \pi_v \f$, in the order of the
 * sequence. The bits of all the nodes are one vector \f$ B \f$ with rank and
 * select [see `RankSelect.h` and `CompactRankSelect.h`], the node \f$ v \f$
 * from the place \f$ o_v \f$ with \f$ R_v = \mathrm{rank}_1( B , o_v ) \f$
 * kept; the item of place \f$ i \f$ in the node \f$ v \f$ moves to the place
 * \f[
 *   i' = \mathrm{rank}_b( B , o_v + i ) - \mathrm{rank}_b( B , o_v )
 * \f]
 * of its child by the bit \f$ b = B[ o_v + i ] \f$, so that a descent along
 * the code of \f$ c \f$ gives `access( i )`, with the rank of the symbol
 * found (`accessRank()`, the LF-mapping of an FM-index in one descent), and
 * \f$ \mathrm{rank}_c( i ) \f$, one rank a step, while
 * \f$ \mathrm{select}_c( k ) \f$ climbs from the leaf of \f$ c \f$ by one
 * select a step,
 * \f[
 *   k' = \mathrm{select}_b( B , \mathrm{rank}_b( B , o_v ) + k ) - o_v + 1 .
 * \f]
 * The bits of \f$ B \f$ are \f$ \sum_c f_c \ell_c < n ( H_0 + 1 ) \f$ for
 * the frequencies \f$ f_c \f$, by the bound of the code of Huffman, plus the
 * directory of rank and select, 25% of them with `RankSelect`, 3.5% with
 * `CompactRankSelect` (the parameter `RS`: time against bits), and
 * \f$ O( \sigma ) \f$ words of the tree and of the codes.
 *
 * The contracts [see `Contract.h`]: the places and the counts are in range,
 * select asks for an occurrence that exists (and the tree was built with
 * select); the audit of the construction is that `access()` gives back each
 * item. The functions are defined in `HuffmanWaveletTree.tpp`.
 *
 * \see V. Mäkinen, G. Navarro, Succinct suffix arrays based on run-length
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
 *         Università di Pisa \n
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

/// a static sequence of symbols in about \f$ H_0 + 1 \f$ bits each, with
/// access, rank and select in time proportional to the length of a code;
/// `RS` is the rank and select of its bits, `RankSelect` or
/// `CompactRankSelect`
template< class RS = RankSelect >
class HuffmanWaveletTree {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty sequence
 HuffmanWaveletTree( void ) = default;

 /// the sequence \f$ x \f$, of symbols below its largest one plus 1, with
 /// select if asked for (else the directory of `RankSelect` keeps no
 /// inventory)
 ///
 /// In pseudocode (Mäkinen and Navarro, beyond the book), the lengths of
 /// Huffman by the two queues of the sorted leaves and of the internal
 /// nodes, which are made in increasing weight:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( x ) \cr
 /// 2 & \quad f_c \gets \text{the occurrences of each } c \text{ in } x;
 ///     \ \text{sort the symbols present by } f_c \cr
 /// 3 & \quad \textbf{repeat}\ \text{join the two lightest of the fronts of}
 ///     \text{ the two queues into a node at the tail of the second} \cr
 /// 4 & \quad \textbf{until}\ \text{one node is left};\ \ell_c \gets
 ///     \text{the depth of the leaf of } c \cr
 /// 5 & \quad \text{give the canonical codes, by length and then by symbol}
 ///     \cr
 /// 6 & \quad \text{build the trie of the codes};\ \mathit{items}[ v ] \gets
 ///     \text{the items whose code passes by } v \cr
 /// 7 & \quad \text{lay the nodes in } B \text{ by levels: } o_v \gets
 ///     \text{the sum of } \mathit{items} \text{ of the nodes before} \cr
 /// 8 & \quad \textbf{foreach}\ c \in x \text{, in order}\ \textbf{do} \cr
 /// 9 & \qquad \textbf{foreach}\ \text{node } v \text{ on the code of } c
 ///     \text{ and its bit } b \text{ there}\ \textbf{do} \cr
 /// 10 & \qquad\quad B[ \mathit{at}[ v ] ] \gets b;\ \mathit{at}[ v ] \gets
 ///     \mathit{at}[ v ] + 1 \cr
 /// 11 & \quad R_v \gets \mathrm{rank}_1( B , o_v ) \text{ for each node } v
 /// \end{array}}
 /// \f]
 /// \pre \f$ |x| < 2^{40} \f$
 explicit HuffmanWaveletTree( std::span< const std::uint32_t > x ,
                              bool withSelect = true );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the bound of the symbols, which are below it
 [[nodiscard]] std::size_t sigma( void ) const { return( f_len.size() ); }

 /// the item \f$ i \f$, by `accessRank()`
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] std::uint32_t access( std::size_t i ) const;

 /// the item \f$ i \f$, by `access()`
 [[nodiscard]] std::uint32_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the item \f$ i \f$ and its occurrences in \f$ [ 0 , i ) \f$, by one
 /// descent
 ///
 /// In pseudocode (Mäkinen and Navarro, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{accessRank}( i ) \cr
 /// 2 & \quad v \gets \text{the root} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad b \gets B[ o_v + i ];\ r \gets \mathrm{rank}_1( B , o_v + i )
 ///     - R_v \cr
 /// 5 & \qquad \textbf{if}\ b = 1\ \textbf{then}\ i \gets r\ \textbf{else}
 ///     \ i \gets i - r \cr
 /// 6 & \qquad \textbf{if}\ \text{the child } b \text{ of } v \text{ is the}
 ///     \text{ leaf of } c\ \textbf{then return}\ ( c , i ) \cr
 /// 7 & \qquad v \gets \text{the child } b \text{ of } v
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] std::pair< std::uint32_t , std::size_t > accessRank(
                                                       std::size_t i ) const;

 /// the occurrences of \f$ c \f$ in \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (Mäkinen and Navarro, beyond the book), along the code
 /// \f$ b_0 \cdots b_{\ell_c - 1} \f$ of \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank}( c , i ) \cr
 /// 2 & \quad \textbf{if}\ c \text{ is not in the sequence }\ \textbf{then
 ///     return}\ 0 \cr
 /// 3 & \quad v \gets \text{the root} \cr
 /// 4 & \quad \textbf{for}\ d \gets 0\ \textbf{to}\ \ell_c - 1\ \textbf{do}
 ///     \cr
 /// 5 & \qquad r \gets \mathrm{rank}_1( B , o_v + i ) - R_v \cr
 /// 6 & \qquad \textbf{if}\ b_d = 1\ \textbf{then}\ i \gets r
 ///     \ \textbf{else}\ i \gets i - r \cr
 /// 7 & \qquad v \gets \text{the child } b_d \text{ of } v \cr
 /// 8 & \quad \textbf{return}\ i
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le \mathrm{size}() \f$
 [[nodiscard]] std::size_t rank( std::uint32_t c , std::size_t i ) const;

 /// the occurrences of \f$ c \f$ in \f$ [ 0 , i ) \f$ and in
 /// \f$ [ 0 , j ) \f$, by one descent that moves both places as `rank()`
 /// above moves one
 /// \pre \f$ i \le j \le \mathrm{size}() \f$
 [[nodiscard]] std::pair< std::size_t , std::size_t > rank( std::uint32_t c ,
                                                           std::size_t i ,
                                                           std::size_t j )
  const;

 /// the place of the \f$ k \f$-th occurrence of \f$ c \f$
 ///
 /// In pseudocode (Mäkinen and Navarro, beyond the book), up from the leaf
 /// of \f$ c \f$, the ranks of select counted from 1:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( c , k ) \cr
 /// 2 & \quad v_0 , \ldots , v_{\ell_c - 1} \gets \text{the nodes on the }
 ///     \text{code } b_0 \cdots b_{\ell_c - 1} \text{ of } c \cr
 /// 3 & \quad \textbf{for}\ d \gets \ell_c - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ b_d = 1\ \textbf{then}\ p \gets
 ///     \mathrm{select}_1( B , R_{v_d} + k ) \cr
 /// 5 & \qquad \textbf{else}\ p \gets \mathrm{select}_0( B , o_{v_d} -
 ///     R_{v_d} + k ) \cr
 /// 6 & \qquad k \gets p - o_{v_d} + 1 \cr
 /// 7 & \quad \textbf{return}\ k - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \mathrm{rank}( c , \mathrm{size}() ) \f$, and
 ///      select was asked for
 /// \post the item at the place is \f$ c \f$
 [[nodiscard]] std::size_t select( std::uint32_t c , std::size_t k ) const;

 /// the length of the code of \f$ c \f$, 0 if \f$ c \f$ is not in the
 /// sequence
 [[nodiscard]] int codeLength( std::uint32_t c ) const {
  return( c < f_len.size() ? f_len[ c ] : 0 );
  }

 /// the bits of the tree: its vector with the directory, the nodes and the
 /// codes
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// an internal node: the place of its bits in \f$ B \f$ and the 1s before
 /// them, and its children by the bits 0 and 1, a node or the leaf
 /// \f$ -1 - c \f$
 struct Node {
  std::uint64_t off = 0 ,    ///< \f$ o_v \f$, the place of its bits
                ones = 0;    ///< \f$ R_v \f$, the 1s before them
  std::int32_t child[ 2 ] = { 0 , 0 };   ///< its children by the bit
  };

 /// the rank and select of the bits \f$ b \f$, with select if asked for
 [[nodiscard]] static RS directory( BitVector b , bool withSelect ) {
  if constexpr( std::is_same_v< RS , RankSelect > )
   return( RS( std::move( b ) , withSelect ? RankSelect::Selects::BOTH
                                           : RankSelect::Selects::NONE ) );
  else return( RS( std::move( b ) ) );
  }

 /// the bit \f$ d \f$ of the code of \f$ c \f$, from the highest
 [[nodiscard]] bool bit( std::uint32_t c , int d ) const {
  return( ( f_code[ c ] >> ( f_len[ c ] - 1 - d ) ) & 1 );
  }

 std::size_t f_n = 0;                   ///< the number of items
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
