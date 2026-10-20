/*--------------------------------------------------------------------------*/
/*-------------------------- File RRRBitVector.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed vector of bits of Raman, Raman and Rao (RRR), in the
 * practical form of Claude and Navarro: a static vector B of m bits with n
 * 1s in at most \f$ m H_0( B ) + 0.175 m \f$ bits, i.e., below m when the
 * 1s (or the 0s) are rare or clustered, with access, rank and select.
 *
 * B is cut in blocks of b = 63 bits; a block of c 1s is written as its
 * class c, in 6 bits, and as its offset, its place among the
 * \f$ \binom{b}{c} \f$ blocks of class c, in \f$ \lceil \log_2 \binom{b}{c}
 * \rceil \f$ bits. The offset of the block with its 1s at the places
 * \f$ p_1 < \cdots < p_c \f$ is its rank in the colexicographic order,
 * \f[ o = \sum_{t=1}^{c} \binom{p_t}{t} , \f]
 * the combinatorial number system, and the block is rebuilt from o by
 * taking, for t = c down to 1, the largest p with \f$ \binom{p}{t} \le o
 * \f$, in at most b steps. Since \f$ \sum_j \log_2 \binom{b}{c_j} \le
 * \log_2 \binom{m}{n} \le m H_0( B ) \f$ (the product of the binomials of
 * the blocks counts a part of the vectors of n 1s), the offsets take at
 * most \f$ m H_0( B ) + m / b \f$ bits, the rounding up costing a bit per
 * block, and the classes \f$ 6 m / b \f$; a superblock of 32 blocks (2016
 * bits) keeps the 1s before it and the place of its first offset,
 * \f$ 128 / 2016 = 6.3 \% \f$ of m, in all \f$ m H_0( B ) + ( 7 / 63 + 128
 * / 2016 ) m < m H_0( B ) + 0.175 m \f$ bits. Hence:
 *
 * - access( i ) and rank1( i ) sum the classes of the blocks of the
 *   superblock before the one of i, and of their offsets the lengths, and
 *   rebuild the block of i: O( 32 + b ) time;
 * - select1( k ) finds the superblock by a binary search on the counts,
 *   the block by the classes, and the 1 in the rebuilt block:
 *   \f$ O( \log ( m / 2016 ) + 32 + b ) \f$ time; select0 likewise.
 *
 * The contracts [see Contract.h]: the places and the ranks are in range
 * as for RankSelect [see RankSelect.h]; the audit of the construction is
 * that the vector gives back each bit. The functions are defined in
 * RRRBitVector.tpp.
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
 *         Universita' di Pisa \n
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

 RRRBitVector( void ) : RRRBitVector( BitVector() ) {}

 /// the bits of b, compressed
 explicit RRRBitVector( const BitVector & b );

 [[nodiscard]] std::size_t size( void ) const { return( f_m ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const { return( f_m - f_ones ); }

 /// the bit i
 /// \pre i < size()
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// the number of 1s in the places [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank1( std::size_t i ) const;

 /// the number of 0s in the places [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank0( std::size_t i ) const {
  return( i - rank1( i ) );
  }

 /// the place of the k-th 1
 /// \pre 1 <= k <= ones()
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the k-th 0
 /// \pre 1 <= k <= zeros()
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the classes, of the offsets and of the superblocks
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr int kB = 63;              ///< the bits of a block
 static constexpr std::size_t kSuper = 32;  ///< the blocks of a superblock

 /// the binomials C( p , t ) for p , t <= 63, and the bits of the offsets
 struct Tables {
  std::array< std::array< std::uint64_t , 64 > , 64 > C{};
  std::array< std::uint8_t , 64 > len{};
  };
 static const Tables & tables( void );

 /// the offset of the block x of class c, and the block of an offset
 static std::uint64_t offset( std::uint64_t x , int c );
 static std::uint64_t block( std::uint64_t o , int c );

 /// the class and the block j, its first place of offset known
 [[nodiscard]] int cls( std::size_t j ) const {
  return( int( f_cls.get( 6 * j , 6 ) ) );
  }

 /// the block j decoded, from the superblock of j: its class, the 1s
 /// before it
 [[nodiscard]] std::uint64_t decode( std::size_t j , std::size_t & ones )
                                                                    const;

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 std::size_t f_m = 0 , f_ones = 0;
 BitVector f_cls;                          ///< 6 bits a block
 BitVector f_off;                          ///< the offsets
 std::vector< std::uint64_t > f_rank;      ///< 1s before each superblock
 std::vector< std::uint64_t > f_ptr;       ///< its first bit of offset

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
