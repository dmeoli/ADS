/*--------------------------------------------------------------------------*/
/*------------------------ File BlockedBloomFilter.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The blocked filter of Bloom, beyond the book: the engineered filter of
 * ADS [see BloomFilter.h for the standard one and BloomVariants.h for its
 * variants]. Its bits are cut into blocks of 256 bits, 8 words of 32 bits,
 * and an item sets 8 bits of one block, one in each word: the block is
 * \f$ \lfloor h_1 B / 2^{32} \rfloor \f$ for B blocks, the bit of the word
 * i is the top 5 bits of \f$ h_0 s_i \bmod 2^{32} \f$ for 8 odd constants
 * \f$ s_i \f$, \f$ h_1 \f$ and \f$ h_0 \f$ being the high and the low half
 * of the hash of the item. A query then reads one block, i.e., one line of
 * cache, instead of the k lines of the standard filter, and makes its 8
 * bits by one product, one shift and one variable shift of a vector of 8
 * words (AVX2), or by 8 products of 32 bits on any machine, with the same
 * result.
 *
 * Putting the bits of an item in one block costs false positives: the
 * blocks get different numbers of items (a Poisson number of mean \f$
 * \lambda = 256 n / m \f$), and the crowded ones answer yes more often,
 * so that the false positives are the mean over the loads j of the
 * probability \f$ ( 1 - ( 1 - 1 / 32 )^j )^8 \f$ of a block of j items
 * [see falsePositiveRate()]: about 1.26 % at 10 bits per item, against
 * 0.82 % of the standard filter with 7 hashes, and 0.31 % at 13.5 bits
 * against 0.15 % (Putze, Sanders and Singler). The split of the 8 bits
 * into 8 words is that of the filters of Impala and of the format
 * Parquet; the number of blocks is any, not a power of 2, so that the bits
 * per item are those asked.
 *
 * The hash of an item is h( x ), taken as it is if H declares
 * is_avalanching [see FastHash.h], and mixed by hashWord() otherwise (the
 * identity of std::hash on the integers); insertHash() and maybeHash() take
 * a hash of 64 bits that is already random.
 *
 * The contracts [see Contract.h]: at least one block; an item inserted is
 * maybe there, a postcondition; two filters united have the same number of
 * blocks. The operations are defined in BlockedBloomFilter.tpp.
 *
 * \see F. Putze, P. Sanders, J. Singler, Cache-, hash-, and space-efficient
 *      Bloom filters, Journal of Experimental Algorithmics 14, 4.4, 2009,
 *      doi:10.1145/1498698.1594230
 * \see Apache Parquet, Split block Bloom filter, 2019,
 *      https://github.com/apache/parquet-format/blob/master/BloomFilter.md
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3, 2019 (the
 *      block by a product instead of a remainder)
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

 /// the empty filter of about bitsPerItem bits for each of n items, in
 /// at least one block
 /// \pre bitsPerItem > 0
 explicit BlockedBloomFilter( std::size_t n , double bitsPerItem = 10 ,
                              H h = H() );

 /// \post maybe( x )
 void insert( const T & x );

 /// false only if x was never inserted
 [[nodiscard]] bool maybe( const T & x ) const;

 /// inserts the hash h of 64 bits, which must be random
 void insertHash( std::uint64_t h );

 /// false only if the hash h was never inserted
 [[nodiscard]] bool maybeHash( std::uint64_t h ) const;

 /// adds the items of o, as if they had been inserted here
 /// \pre o has the blocks of this filter
 void unite( const BlockedBloomFilter & o );

 /// removes all the items
 void clear( void );

 /// the insertions made
 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the bits, 256 per block
 [[nodiscard]] std::size_t bits( void ) const;

 [[nodiscard]] std::size_t blocks( void ) const;

 /// the expected false positives after the insertions made, the mean over
 /// the Poisson loads of the blocks
 [[nodiscard]] double falsePositiveRate( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a block of 8 words, aligned to its 32 bytes
 struct alignas( 32 ) Block {
  std::uint32_t w[ 8 ];
  };

 /// the hash of x, mixed unless H avalanches
 [[nodiscard]] std::uint64_t hash( const T & x ) const;

 /// the block of the hash h
 [[nodiscard]] std::size_t block( std::uint64_t h ) const;

 std::vector< Block > f_B;            ///< the blocks
 std::size_t f_n = 0;                 ///< the insertions
 H f_h;

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
