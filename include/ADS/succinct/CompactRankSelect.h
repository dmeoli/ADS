/*--------------------------------------------------------------------------*/
/*------------------------ File CompactRankSelect.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits B of m bits in about 3.5% of
 * m bits more, the compact counterpart of RankSelect [see RankSelect.h],
 * which spends about 50% for the fastest queries: the flat directory of
 * Kurpicz, after the poppy of Zhou, Andersen and Kaminsky.
 *
 * The bits are cut in superblocks of 4096 bits, each one in 8 blocks of 512
 * bits, a line of the cache [see BitVector.h]. One entry of 128 bits per
 * superblock holds the 1s before it since its top region of \f$ 2^{32} \f$
 * bits, in 32 bits, and the 1s before each of its blocks since the
 * superblock, \f$ c_0 = 0 , c_1 , \ldots , c_7 \le 3584 \f$, in 12 bits
 * each; a count of 64 bits per top region completes the rank. Hence
 * \f[ rank_1( i ) = T[ \lfloor i / 2^{32} \rfloor ] +
 *     S[ \lfloor i / 4096 \rfloor ] + c_{\lfloor i / 512 \rfloor \bmod 8}
 *     + \sum_{j < 8} | w_j \wedge ( 2^{\min( 64 , \max( 0 , o - 64 j ) )}
 *     - 1 ) | , \f]
 * with o = i mod 512 and \f$ w_j \f$ the words of the block: one access to
 * the entries, one to the line of the block, and the sideways sums of its 8
 * words cut at i, with no branch (with AVX2, the bytes of the line before
 * the one of i counted 32 at a time by a table of the 16 nibbles, the
 * method of Mula, Kurz and Lemire); \f$ 128 / 4096 = 3.125 \% \f$ of m
 * bits.
 *
 * select keeps the superblock of every 4096-th bit b, 32 bits each, at most
 * \f$ 32 / 4096 < 0.8 \% \f$ of m bits for each value, and finds the
 * k-th bit b by a binary search on the entries between two samples, then
 * the block by the 7 counts of the superblock and the word by the sums of
 * the 8 words of its line, both by comparisons with no branch, and the bit
 * by selectInWord(); \f$ O( \log ( m / 4096 ) ) \f$ steps in the worst case
 * and O( 1 ) where the bits b are spread evenly (a sample every 2 to 8
 * superblocks for a density between 1/2 and 1/8). The trade with RankSelect
 * is the one of the literature: a sum of up to 8 words and a search of the
 * entries instead of one word and an inventory, in 15 times fewer bits.
 *
 * The directory and the samples are built in one pass on the words, a
 * superblock at a time with no test on the words that exist; from words
 * in memory the words are copied in the same pass, above \f$ 2^{23} \f$
 * bits, as in RankSelect.
 *
 * The contracts [see Contract.h]: \f$ i \le m \f$ for rank and
 * \f$ 1 \le k \f$ up to the number of the bits b for select; the
 * postconditions are that the bit at \f$ select_b( k ) \f$ is b and that
 * its rank is k - 1, and the audit of the construction is that the
 * directory agrees with a scan. The functions are defined in
 * CompactRankSelect.tpp.
 *
 * \see F. Kurpicz, Engineering compact data structures for rank and select
 *      queries on bit vectors, Proc. 29th SPIRE, LNCS 13617, 257-272, 2022,
 *      doi:10.1007/978-3-031-20643-6_19
 * \see D. Zhou, D. G. Andersen, M. Kaminsky, Space-efficient,
 *      high-performance rank and select structures on uncompressed bit
 *      sequences, Proc. 12th SEA, LNCS 7933, 151-163, 2013,
 *      doi:10.1007/978-3-642-38527-8_15
 * \see G. Navarro, Compact Data Structures, Cambridge University Press,
 *      2016, chapter 4, doi:10.1017/CBO9781316588284
 * \see W. Mula, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
 * \see P. Pandey, M. A. Bender, R. Johnson, A fast x86 implementation of
 *      select, arXiv:1706.00990, 2017
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

#ifndef __ADS_CompactRankSelect
 #define __ADS_CompactRankSelect
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CompactRankSelect ------------------------*/
/*--------------------------------------------------------------------------*/

/// a static vector of bits with rank and select in about 3.5% of its bits
class CompactRankSelect {

/*--------------------------------------------------------------------------*/
 public:

 CompactRankSelect( void ) : CompactRankSelect( BitVector() ) {}

 /// the directory of the bits b, which it keeps
 explicit CompactRankSelect( BitVector b );

 /// the directory of the n bits of the words w, copied as they are
 /// counted
 /// \pre w.size() >= ceil( n / 64 )
 CompactRankSelect( std::span< const Word > w , std::size_t n );

 /// the bits
 [[nodiscard]] const BitVector & bits( void ) const { return( f_b ); }

 [[nodiscard]] std::size_t size( void ) const { return( f_b.size() ); }

 /// the bit i
 /// \pre i < size()
 [[nodiscard]] bool operator[]( std::size_t i ) const { return( f_b[ i ] ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const {
  return( f_b.size() - f_ones );
  }

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
 /// \post the bit there is 1 and rank1() of it is k - 1
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the k-th 0
 /// \pre 1 <= k <= zeros()
 /// \post the bit there is 0 and rank0() of it is k - 1
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the directory, beyond those of the vector
 [[nodiscard]] std::size_t directoryBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = bool;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = bool;
 using const_reference = bool;
 using const_iterator = SubscriptIterator< CompactRankSelect , bool >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kSuper = 4096;  ///< the bits of a superblock
 static constexpr std::size_t kBlock = 512;   ///< the bits of a block
 static constexpr std::size_t kSample = 4096; ///< bits b between samples
#ifndef ADS_COMPACT_TOP
 static constexpr int kTop = 32;              ///< log of a top region
#else
 static constexpr int kTop = ADS_COMPACT_TOP;  // a test of the top level
#endif

 /// an entry: the 1s before the superblock since its top region in the
 /// low 32 bits of lo, and the 1s before its block k since the superblock
 /// in the 12 bits from 32 + 12 k of the 128 bits ( hi , lo )
 struct Entry {
  std::uint64_t lo = 0 , hi = 0;
  };

 /// the 1s before the block k of the entry e since its superblock
 [[nodiscard]] static std::size_t inSuper( const Entry & e , std::size_t k ) {
  const std::size_t s = 32 + 12 * k;        // 32 , 44 , ... , 116
  const std::uint64_t v = s < 64 ? ( e.lo >> s ) | ( e.hi << ( 64 - s ) )
                                 : e.hi >> ( s - 64 );
  return( std::size_t( v & 0xFFF ) );
  }

 /// the bits from which the words are copied as they are counted, where
 /// reading them twice costs more than a copy in the caches
#ifndef ADS_RANKSELECT_FUSE
 static constexpr std::size_t kFuse = std::size_t( 1 ) << 23;
#else
 static constexpr std::size_t kFuse = ADS_RANKSELECT_FUSE;   // a test
#endif

 /// the directory of the bits, the words first copied from w where w is
 /// not null
 void directory( const Word * w );

 /// the bits b before the superblock s
 [[nodiscard]] std::size_t before( std::size_t s , bool b ) const;

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 /// the directory agrees with a scan of the bits
 [[nodiscard]] bool consistent( void ) const;

 BitVector f_b;
 std::vector< Entry > f_e;                ///< for each superblock and one more
 std::vector< std::uint64_t > f_top;      ///< the 1s before each top region
 std::vector< std::uint32_t > f_s[ 2 ];   ///< superblocks of the samples
 std::size_t f_ones = 0;

 };  // end( class CompactRankSelect )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/CompactRankSelect.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CompactRankSelect.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CompactRankSelect.h ----------------------*/
/*--------------------------------------------------------------------------*/
