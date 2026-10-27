/*--------------------------------------------------------------------------*/
/*-------------------------- File RRRBitVector.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed vector of bits of Raman, Raman and Rao (RRR), in the
 * practical form of Claude and Navarro: a static vector \f$ B \f$ of
 * \f$ m \f$ bits with \f$ n \f$ 1s in at most
 * \f$ m H_0( B ) + 0.175 m \f$ bits, i.e., below \f$ m \f$ when the 1s (or
 * the 0s) are rare, with access, rank and select.
 *
 * \f$ B \f$ is cut in blocks of \f$ b = 63 \f$ bits; a block of \f$ c \f$
 * 1s is written as its class \f$ c \f$, in 6 bits, and as its offset, its
 * place among the \f$ \binom{b}{c} \f$ blocks of class \f$ c \f$, in
 * \f$ \lceil \log_2 \binom{b}{c} \rceil \f$ bits. The offset of the block
 * with its 1s at the places \f$ p_1 < \cdots < p_c \f$ is its rank in the
 * colexicographic order,
 * \f[ o = \sum_{t=1}^{c} \binom{p_t}{t} , \f]
 * the combinatorial number system, and the block is rebuilt from \f$ o \f$
 * from its highest place down: the place \f$ p \f$ holds the \f$ t \f$-th
 * 1 from the lowest, \f$ t \f$ the 1s not yet met, if and only if
 * \f$ \binom{p}{t} \le o \f$, which is then taken away from \f$ o \f$. A
 * query walks only the places it needs: the rank stops at its place or at
 * the last 1, the select at its bit. Since
 * \f$ \sum_j \log_2 \binom{b}{c_j} \le \log_2 \binom{m}{n} \le
 * m H_0( B ) \f$ (the product of the binomials of the blocks counts a part
 * of the vectors of \f$ n \f$ 1s), the offsets take at most
 * \f$ m H_0( B ) + m / b \f$ bits, the rounding up costing a bit per block,
 * and the classes \f$ 6 m / b \f$; a superblock of 32 blocks (2016 bits)
 * keeps the 1s before it and the place of its first offset, at most
 * \f$ 128 / 2016 = 6.3 \% \f$ of \f$ m \f$, in all
 * \f$ m H_0( B ) + ( 7 / 63 + 128 / 2016 ) m < m H_0( B ) + 0.175 m \f$
 * bits. Hence:
 *
 * - `operator[]( i )` and `rank1( i )` sum the classes of the blocks of the
 *   superblock before the one of \f$ i \f$, and of their offsets the
 *   lengths, and rebuild the block of \f$ i \f$: \f$ O( 32 + b ) \f$ time;
 * - `select1( k )` finds the superblock by a binary search on the counts,
 *   the block by the classes, and the 1 in the rebuilt block:
 *   \f$ O( \log( m / 2016 ) + 32 + b ) \f$ time; `select0()` likewise.
 *
 * The contracts [see `Contract.h`]: the places and the ranks are in range
 * as for `RankSelect` [see `RankSelect.h`]; the audit of the construction
 * is that the vector gives back each bit. The functions are defined in
 * `RRRBitVector.tpp`.
 *
 * \see R. Raman, V. Raman, S. S. Rao, Succinct indexable dictionaries with
 *      applications to encoding k-ary trees, prefix sums and multisets,
 *      ACM Transactions on Algorithms 3(4), 43, 2007,
 *      doi:10.1145/1290672.1290680
 * \see F. Claude, G. Navarro, Practical rank/select queries over arbitrary
 *      sequences, Proc. 15th SPIRE, LNCS 5280, 176-187, 2008,
 *      doi:10.1007/978-3-540-89097-3_18
 * \see G. Navarro, E. Providel, Fast, small, simple rank/select on bitmaps,
 *      Proc. 11th SEA, LNCS 7276, 295-306, 2012,
 *      doi:10.1007/978-3-642-30850-5_26
 * \see G. Navarro, Compact Data Structures, Cambridge University Press,
 *      2016, section 4.1, doi:10.1017/CBO9781316588284
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

#ifndef __ADS_RRRBitVector
 #define __ADS_RRRBitVector
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RRRBitVector ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a static vector of bits compressed to its entropy, with rank and select
class RRRBitVector {

/*--------------------------------------------------------------------------*/
 public:

 /// the vector of no bit
 RRRBitVector( void ) : RRRBitVector( BitVector() ) {}

 /// the bits of \f$ b \f$, compressed
 ///
 /// In pseudocode (Raman, Raman and Rao, beyond the book), \f$ C \f$ the
 /// classes, \f$ O \f$ the offsets, \f$ R \f$ and \f$ P \f$ the 1s and the
 /// place of the first offset of each superblock:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( B ) \cr
 /// 2 & \quad n \gets 0 \cr
 /// 3 & \quad \textbf{foreach}\ \text{block } j \text{ of } 63 \text{ bits }
 ///     x \text{ of } B\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ j \bmod 32 = 0\ \textbf{then}
 ///     \ \mathrm{append}( R , n );\ \mathrm{append}( P , | O | ) \cr
 /// 5 & \qquad c \gets \mathrm{popcount}( x );\ \mathrm{append}( C , c ) \cr
 /// 6 & \qquad o \gets \sum_{t=1}^{c} \binom{p_t}{t}\quad \triangleright
 ///     \ p_1 < \cdots < p_c \text{ the places of the 1s of } x \cr
 /// 7 & \qquad \mathrm{append}( O , o \text{ in } \lceil \log_2
 ///     \binom{63}{c} \rceil \text{ bits} );\ n \gets n + c \cr
 /// 8 & \quad \mathrm{append}( R , n );\ \mathrm{append}( P , | O | )
 /// \end{array}}
 /// \f]
 explicit RRRBitVector( const BitVector & b );

 /// the number of bits, \f$ m \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_m ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const { return( f_m - f_ones ); }

 /// the bit \f$ i \f$, i.e., whether the rank changes past it within its
 /// block
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// the number of 1s in the places \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (Raman, Raman and Rao, beyond the book), the block
 /// \f$ j \f$ of \f$ i \f$ located from the counts of its superblock
 /// \f$ s \f$, and its 1s from the place \f$ i \bmod 63 \f$ up rebuilt from
 /// its offset:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank1}( i ) \cr
 /// 2 & \quad j \gets \lfloor i / 63 \rfloor;\ s \gets \lfloor j / 32
 ///     \rfloor;\ r \gets R[ s ];\ q \gets P[ s ] \cr
 /// 3 & \quad \textbf{for}\ h \gets 32 s\ \textbf{to}\ j - 1\ \textbf{do}\ r
 ///     \gets r + C[ h ];\ q \gets q + \lceil \log_2 \binom{63}{C[ h ]}
 ///     \rceil \cr
 /// 4 & \quad c \gets C[ j ];\ o \gets \text{the offset at the place } q
 ///     \text{ of } O;\ t \gets c \cr
 /// 5 & \quad \textbf{for}\ p \gets 62\ \textbf{downto}\ i \bmod 63
 ///     \ \textbf{while}\ t > 0\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ \binom{p}{t} \le o\ \textbf{then}\ o \gets o -
 ///     \binom{p}{t};\ t \gets t - 1 \cr
 /// 7 & \quad \textbf{return}\ r + t\quad \triangleright\ t \text{ the 1s }
 ///     \text{below the place } i \bmod 63
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le \f$ `size()`
 [[nodiscard]] std::size_t rank1( std::size_t i ) const;

 /// the number of 0s in the places \f$ [ 0 , i ) \f$, i.e.,
 /// \f$ i - \mathrm{rank}_1( i ) \f$
 /// \pre \f$ i \le \f$ `size()`
 [[nodiscard]] std::size_t rank0( std::size_t i ) const {
  return( i - rank1( i ) );
  }

 /// the place of the \f$ k \f$-th 1
 ///
 /// In pseudocode (Raman, Raman and Rao, beyond the book), the names as in
 /// `rank1()` above:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select1}( k ) \cr
 /// 2 & \quad s \gets \text{the last superblock with } R[ s ] < k \text{, by}
 ///     \text{ a binary search} \cr
 /// 3 & \quad r \gets k - R[ s ];\ q \gets P[ s ];\ j \gets 32 s \cr
 /// 4 & \quad \textbf{while}\ r > C[ j ]\ \textbf{do}\ r \gets r - C[ j ];
 ///     \ q \gets q + \lceil \log_2 \binom{63}{C[ j ]} \rceil;\ j \gets j + 1
 ///     \cr
 /// 5 & \quad o \gets \text{the offset at the place } q \text{ of } O;\ t
 ///     \gets C[ j ] \cr
 /// 6 & \quad \textbf{for}\ p \gets 62\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 7 & \qquad \textbf{if}\ \binom{p}{t} \le o\ \textbf{then} \cr
 /// 8 & \qquad\quad \textbf{if}\ t = r\ \textbf{then return}\ 63 j + p \cr
 /// 9 & \qquad\quad o \gets o - \binom{p}{t};\ t \gets t - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \f$ `ones()`
 /// \post the bit there is 1 and `rank1()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the \f$ k \f$-th 0, as `select1()` above with the 0s of
 /// each block counted as its length less its class, and the block rebuilt
 /// whole when it is sparse in 0s
 /// \pre \f$ 1 \le k \le \f$ `zeros()`
 /// \post the bit there is 0 and `rank0()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the classes, of the offsets and of the superblocks
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kB = 63;              ///< the bits of a block
 static constexpr std::size_t kSuper = 32;  ///< the blocks of a superblock

 /// the binomials C( p , t ) for p , t <= 63, and the bits of the offsets
 struct Tables {
  /// the binomial coefficients \f$ \binom{p}{t} \f$
  std::array< std::array< std::uint64_t , 64 > , 64 > C{};
  /// the bits of the offset of a block with \f$ t \f$ 1s
  std::array< std::uint8_t , 64 > len{};
  };
 static const Tables & tables( void );

 /// the offset of the block x of class c
 static std::uint64_t offset( std::uint64_t x , int c );

 /// the 1s below the place q of the block of offset o and class c: the
 /// places from the highest down to q, with no branch
 static int below( std::uint64_t o , int c , int q );

 /// the offset and the class of the block j, and the 1s before it
 [[nodiscard]] std::uint64_t locate( std::size_t j , int & c ,
                                     std::size_t & ones ) const;

 /// the class of the block j
 [[nodiscard]] int cls( std::size_t j ) const {
  return( int( f_cls.get( 6 * j , 6 ) ) );
  }

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 std::size_t f_m = 0 , f_ones = 0;
 BitVector f_cls;                          ///< 6 bits a block
 BitVector f_off;                          ///< the offsets
 BitVector f_rank;                         ///< 1s before each superblock
 BitVector f_ptr;                          ///< its first bit of offset
 int f_wr = 1 , f_wp = 1;                  ///< the widths of the two

 };  // end( class RRRBitVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/RRRBitVector.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RRRBitVector.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RRRBitVector.h -------------------------*/
/*--------------------------------------------------------------------------*/
