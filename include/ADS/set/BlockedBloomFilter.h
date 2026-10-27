/*--------------------------------------------------------------------------*/
/*------------------------ File BlockedBloomFilter.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The blocked filter of Bloom, beyond the book: the engineered filter of
 * ADS [see `BloomFilter.h` for the standard one and `BloomVariants.h` for
 * its variants]. Its bits are cut into blocks of 256 bits, 8 words of 32
 * bits, and an item sets 8 bits of one block, one in each word: the block is
 * \f$ \lfloor h_1 B / 2^{32} \rfloor \f$ for \f$ B \f$ blocks, the bit of
 * the word \f$ i \f$ is the top 5 bits of \f$ h_0 s_i \bmod 2^{32} \f$ for 8
 * odd constants \f$ s_i \f$, \f$ h_1 \f$ and \f$ h_0 \f$ being the high and
 * the low half of the hash of the item. A query then reads one block, i.e.,
 * one line of cache, instead of the \f$ k \f$ lines of the standard filter,
 * and makes its 8 bits by one product, one shift and one variable shift of a
 * vector of 8 words (AVX2), or by 8 products of 32 bits on any machine, with
 * the same result.
 *
 * Putting the bits of an item in one block costs false positives: the
 * blocks get different numbers of items (a Poisson number of mean
 * \f$ \lambda = 256 n / m \f$), and the crowded ones answer yes more often,
 * so that the false positives are the mean over the loads \f$ j \f$ of the
 * probability \f$ ( 1 - ( 1 - 1 / 32 )^j )^8 \f$ of a block of \f$ j \f$
 * items [see `falsePositiveRate()`]: about \f$ 1.26 \% \f$ at 10 bits per
 * item, against \f$ 0.82 \% \f$ of the standard filter with 7 hashes, and
 * \f$ 0.31 \% \f$ at 13.5 bits against \f$ 0.15 \% \f$ (Putze, Sanders and
 * Singler). The split of the 8 bits into 8 words is that of the filters of
 * Impala and of the format Parquet; the number of blocks is any, not a power
 * of 2, so that the bits per item are those asked.
 *
 * The hash of an item is `h( x )`, taken as it is if `H` declares
 * `is_avalanching` [see `FastHash.h`], and mixed by `hashWord()` otherwise
 * (the identity of `std::hash` on the integers); `insertHash()` and
 * `maybeHash()` take a hash of 64 bits that is already random.
 *
 * The contracts [see `Contract.h`]: at least one block; an item inserted is
 * maybe there, a postcondition; two filters united have the same number of
 * blocks. The operations are defined in `BlockedBloomFilter.tpp`.
 *
 * \see F. Putze, P. Sanders, J. Singler, Cache-, hash-, and space-efficient
 *      Bloom filters, Journal of Experimental Algorithmics 14, 4.4, 2009,
 *      doi:10.1145/1498698.1594230
 * \see Apache Parquet, Split block Bloom filter, 2019,
 *      https://github.com/apache/parquet-format/blob/master/BloomFilter.md
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3, 2019,
 *      doi:10.1145/3230636 (the block by a product instead of a remainder)
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

#ifndef __ADS_BlockedBloomFilter
 #define __ADS_BlockedBloomFilter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BlockedBloomFilter ------------------------*/
/*--------------------------------------------------------------------------*/

/// a filter of Bloom in blocks of 256 bits, 8 bits per item
template< class T , class H = DefaultHash< T > >
class BlockedBloomFilter {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty filter of about `bitsPerItem` bits for each of \f$ n \f$
 /// items, in at least one block, \f$ \lceil n \cdot
 /// \mathit{bitsPerItem} / 256 \rceil \f$ blocks
 /// \pre \f$ \mathit{bitsPerItem} > 0 \f$
 explicit BlockedBloomFilter( std::size_t n , double bitsPerItem = 10 ,
                              H h = H() );

 /// \f$ x \f$ into the filter, by `insertHash()` of its hash
 /// \post `maybe( x )`
 void insert( const T & x );

 /// whether \f$ x \f$ may be in the set, by `maybeHash()` of its hash:
 /// false only if \f$ x \f$ was never inserted
 [[nodiscard]] bool maybe( const T & x ) const;

 /// the hash \f$ h \f$ of 64 bits, which must be random, into the filter
 ///
 /// In pseudocode (Putze, Sanders and Singler, with the 8 words of the
 /// format Parquet), \f$ W_b \f$ the 8 words of 32 bits of the block
 /// \f$ b \f$ and \f$ s_i \f$ the 8 odd constants:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHash}( h ) \cr
 /// 2 & \quad b \gets \lfloor \lfloor h / 2^{32} \rfloor \cdot B / 2^{32}
 ///     \rfloor;\ h_0 \gets h \bmod 2^{32} \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ 7\ \textbf{do}
 ///     \ W_b[ i ] \gets W_b[ i ] \vee 2^{\lfloor ( h_0 s_i \bmod 2^{32} ) /
 ///     2^{27} \rfloor}
 /// \end{array}}
 /// \f]
 void insertHash( std::uint64_t h );

 /// whether the hash \f$ h \f$ may be in the set: false only if it was
 /// never inserted
 ///
 /// In pseudocode (Putze, Sanders and Singler), the 8 bits of
 /// `insertHash()` tested at once:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{maybeHash}( h ) \cr
 /// 2 & \quad b \gets \text{the block of } h;\ M \gets \text{the 8 }
 ///     \text{words of the bits of } h \cr
 /// 3 & \quad \textbf{return}\ M[ i ] \wedge \lnot W_b[ i ] = 0
 ///     \ \text{for } i = 0 , \ldots , 7
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// the items of `o` added, as if they had been inserted here, by the OR of
 /// the blocks
 /// \pre `o` has the blocks of this filter
 void unite( const BlockedBloomFilter & o );

 /// removes all the items
 void clear( void );

 /// the insertions made
 [[nodiscard]] std::size_t size( void ) const;

 /// whether no item was inserted
 [[nodiscard]] bool empty( void ) const;

 /// the bits, 256 per block
 [[nodiscard]] std::size_t bits( void ) const;

 /// the blocks \f$ B \f$
 [[nodiscard]] std::size_t blocks( void ) const;

 /// the expected false positives after the insertions made, the mean over
 /// the Poisson loads of the blocks,
 /// \f$ \sum_j e^{-\lambda} \lambda^j / j! \, ( 1 - ( 31 / 32 )^j )^8 \f$
 /// with \f$ \lambda = n / B \f$
 ///
 /// In pseudocode (Putze, Sanders and Singler), the terms by the recurrence
 /// \f$ p_{j+1} = p_j \lambda / ( j + 1 ) \f$ from the mode
 /// \f$ j_0 = \lfloor \lambda \rfloor \f$ both ways, until they fall
 /// below \f$ 10^{-18} p_{j_0} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{falsePositiveRate}() \cr
 /// 2 & \quad \lambda \gets n / B;\ \textbf{if}\ \lambda = 0
 ///     \ \textbf{then return}\ 0 \cr
 /// 3 & \quad j_0 \gets \lfloor \lambda \rfloor;\ p_{j_0} \gets e^{-\lambda}
 ///     \lambda^{j_0} / j_0! \cr
 /// 4 & \quad \textbf{return}\ \sum_j p_j ( 1 - ( 31 / 32 )^j )^8
 ///     \ \text{over the } j \text{ with } p_j \ge 10^{-18} p_{j_0}
 /// \end{array}}
 /// \f]
 [[nodiscard]] double falsePositiveRate( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a block of 8 words, aligned to its 32 bytes
 struct alignas( 32 ) Block {
  std::uint32_t w[ 8 ];   ///< the words, 32 bits each
  };

 /// the hash of \f$ x \f$, mixed unless `H` avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the block of the hash \f$ h \f$
 [[nodiscard]] std::size_t block( std::uint64_t h ) const;

 std::vector< Block > f_B;            ///< the blocks
 std::size_t f_n = 0;                 ///< the insertions
 H f_h;                               ///< the hash of the items

 };  // end( class BlockedBloomFilter )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/BlockedBloomFilter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BlockedBloomFilter.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BlockedBloomFilter.h ---------------------*/
/*--------------------------------------------------------------------------*/
