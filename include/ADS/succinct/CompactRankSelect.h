/*--------------------------------------------------------------------------*/
/*------------------------ File CompactRankSelect.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits \f$ B \f$ of \f$ m \f$ bits in
 * about 3.5% of \f$ m \f$ bits more, the compact counterpart of
 * `RankSelect` [see `RankSelect.h`], which spends about 50% for the fastest
 * queries: the flat directory of Kurpicz, after the poppy of Zhou, Andersen
 * and Kaminsky.
 *
 * The bits are cut in superblocks of 4096 bits, each one in 8 blocks of 512
 * bits, a line of the cache [see `BitVector.h`]. One entry of 128 bits per
 * superblock holds the 1s before it since its top region of \f$ 2^{32} \f$
 * bits, in 32 bits, and the 1s before each of its blocks since the
 * superblock, \f$ c_0 = 0 , c_1 , \ldots , c_7 \le 3584 \f$, in 12 bits
 * each; a count of 64 bits per top region completes the rank. Hence
 * \f[ \mathrm{rank}_1( i ) = T[ \lfloor i / 2^{32} \rfloor ] +
 *     S[ \lfloor i / 4096 \rfloor ] + c_{\lfloor i / 512 \rfloor \bmod 8}
 *     + \sum_{j < 8} | w_j \wedge ( 2^{\min( 64 , \max( 0 , o - 64 j ) )}
 *     - 1 ) | , \f]
 * with \f$ o = i \bmod 512 \f$, \f$ w_j \f$ the words of the block and
 * \f$ | x | \f$ the 1s of the word \f$ x \f$: one access to the entries,
 * one to the line of the block, and the sideways sums of its 8 words cut at
 * \f$ i \f$, with no branch (with AVX2, the bytes of the line before the one
 * of \f$ i \f$ counted 32 at a time by a table of the 16 nibbles, the
 * method of Mula, Kurz and Lemire); \f$ 128 / 4096 = 3.125 \% \f$ of
 * \f$ m \f$ bits.
 *
 * The select keeps the superblock of every 4096-th bit \f$ b \f$, 32 bits
 * each, at most \f$ 32 / 4096 < 0.8 \% \f$ of \f$ m \f$ bits for each
 * value, and finds the \f$ k \f$-th bit \f$ b \f$ by a binary search on the
 * entries between two samples, then the block by the 7 counts of the
 * superblock and the word by the sums of the 8 words of its line, both by
 * comparisons with no branch, and the bit by `selectInWord()`;
 * \f$ O( \log( m / 4096 ) ) \f$ steps in the worst case and \f$ O( 1 ) \f$
 * where the bits \f$ b \f$ are spread evenly (a sample every 2 to 8
 * superblocks for a density between 1/2 and 1/8). The trade with
 * `RankSelect` is the one of the literature: a sum of up to 8 words and a
 * search of the entries instead of one word and an inventory, in 15 times
 * fewer bits.
 *
 * The directory and the samples are built in one pass on the words, a
 * superblock at a time with no test on the words that exist; from words
 * in memory the words are copied in the same pass, above \f$ 2^{23} \f$
 * bits, as in `RankSelect`.
 *
 * The contracts [see `Contract.h`]: \f$ i \le m \f$ for the rank and
 * \f$ 1 \le k \f$ up to the number of the bits \f$ b \f$ for the select;
 * the postconditions are that the bit at \f$ \mathrm{select}_b( k ) \f$ is
 * \f$ b \f$ and that its rank is \f$ k - 1 \f$, and the audit of the
 * construction is that the directory agrees with a scan. The functions are
 * defined in `CompactRankSelect.tpp`.
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
 *         Università di Pisa \n
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
#include "ADS/parallel/Executor.h"
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
/// more
class CompactRankSelect {

/*--------------------------------------------------------------------------*/
 public:

 /// the directory of no bit
 CompactRankSelect( void ) : CompactRankSelect( BitVector() ) {}

 /// the directory of the bits \f$ b \f$, which it keeps
 explicit CompactRankSelect( BitVector b );

 /// the directory of the \f$ n \f$ bits of the words \f$ w \f$, copied as
 /// they are counted
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 CompactRankSelect( std::span< const Word > w , std::size_t n );

 /// the directory of `CompactRankSelect( b )` above, the same entries and
 /// samples, built on the threads of the executor `ex` (see
 /// \ref theory_succinct_parallel)
 ///
 /// In pseudocode (the counts of the chunks, their prefix sums and the
 /// entries and samples of each chunk from its prefix, beyond the book),
 /// the superblocks cut into chunks \f$ C_0 , \ldots , C_{q-1} \f$, those
 /// of a top region never in two chunks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{directory}( \mathit{ex} , W ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{chunk } C_k\ \textbf{do in parallel}
 ///     \ a_k \gets \text{the 1s of the words of } C_k \cr
 /// 3 & \quad c_0 \gets 0;\ c_{k + 1} \gets c_k + a_k \text{ for }
 ///     k = 0 , \ldots , q - 2 \cr
 /// 4 & \quad \textbf{foreach}\ \text{chunk } C_k\ \textbf{do in parallel}
 ///     \cr
 /// 5 & \qquad \text{the entries of its superblocks from } c = c_k
 ///     \text{, and its samples from the } \lceil c_k / 4096 \rceil
 ///     \text{-th} \cr
 /// 6 & \quad \text{the closing samples, as in } \mathrm{directory}( W )
 /// \end{array}}
 /// \f]
 template< Executor E >
 CompactRankSelect( E & ex , BitVector b );

 /// the directory of `CompactRankSelect( w , n )` above, the words copied
 /// by the chunks that count them, on the threads of `ex`
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 template< Executor E >
 CompactRankSelect( E & ex , std::span< const Word > w , std::size_t n );

 /// the bits
 [[nodiscard]] const BitVector & bits( void ) const { return( f_b ); }

 /// the number of bits, \f$ m \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_b.size() ); }

 /// the bit \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] bool operator[]( std::size_t i ) const { return( f_b[ i ] ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const {
  return( f_b.size() - f_ones );
  }

 /// the number of 1s in the places \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (the flat rank of Kurpicz, beyond the book), \f$ T \f$ the
 /// counts of the top regions, \f$ S \f$ and \f$ c \f$ those of the entries
 /// and \f$ w_0 , \ldots , w_7 \f$ the words of the block of \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank1}( i ) \cr
 /// 2 & \quad r \gets T[ \lfloor i / 2^{32} \rfloor ] + S[ \lfloor i / 4096
 ///     \rfloor ] + c_{\lfloor i / 512 \rfloor \bmod 8} \cr
 /// 3 & \quad o \gets i \bmod 512 \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ 7\ \textbf{do}\quad
 ///     \triangleright\ \text{with no branch} \cr
 /// 5 & \qquad r \gets r + \mathrm{popcount}( w_j \bmod 2^{\min( 64 , \max(
 ///     0 , o - 64 j ) )} ) \cr
 /// 6 & \quad \textbf{return}\ r
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
 /// In pseudocode (the select of Kurpicz, beyond the book), for the bits
 /// \f$ b = 1 \f$ (the words complemented for the 0s), \f$ Z \f$ the
 /// superblocks of the samples and \f$ \mathrm{before}( s ) \f$ the bits
 /// \f$ b \f$ before the superblock \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( k ) \cr
 /// 2 & \quad j \gets \lfloor ( k - 1 ) / 4096 \rfloor \cr
 /// 3 & \quad s \gets \text{the last superblock in } [ Z[ j ] , Z[ j + 1 ] ]
 ///     \text{ with } \mathrm{before}( s ) < k\text{, by a binary search} \cr
 /// 4 & \quad r \gets k - \mathrm{before}( s ) \cr
 /// 5 & \quad q \gets \text{the last block } h \text{ of } s \text{ with }
 ///     c_h < r;\ r \gets r - c_q \cr
 /// 6 & \quad p \gets \text{the last word } h \text{ of the block with }
 ///     \sum_{h' < h} \mathrm{popcount}( w_{h'} ) < r \cr
 /// 7 & \quad r \gets r - \sum_{h' < p} \mathrm{popcount}( w_{h'} ) \cr
 /// 8 & \quad \textbf{return}\ \text{the place of } w_p +
 ///     \mathrm{selectInWord}( w_p , r - 1 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \f$ `ones()`
 /// \post the bit there is 1 and `rank1()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the \f$ k \f$-th 0, by the select of `select1()` above on
 /// the complemented words
 /// \pre \f$ 1 \le k \le \f$ `zeros()`
 /// \post the bit there is 0 and `rank0()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the directory, beyond those of the vector
 [[nodiscard]] std::size_t directoryBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the type of an item
 using value_type = bool;
 /// the type of a size
 using size_type = std::size_t;
 /// the type of the distance of two places
 using difference_type = std::ptrdiff_t;
 /// an item, returned by value
 using reference = bool;
 /// an item, returned by value
 using const_reference = bool;
 /// the constant iterator, random access
 using const_iterator = SubscriptIterator< CompactRankSelect , bool >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
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
  std::uint64_t lo = 0 ,   ///< the low 64 bits of the entry
                hi = 0;   ///< the high 64 bits of the entry
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

 /// the fewest superblocks of a chunk of the parallel construction
#ifndef ADS_COMPACT_CHUNK
 static constexpr std::size_t kChunk = 256;
#else
 static constexpr std::size_t kChunk = ADS_COMPACT_CHUNK;   // a test
#endif

 /// the directory of the bits on the threads of ex, the words first copied
 /// from w where w is not null
 template< Executor E >
 void directory( E & ex , const Word * w );

 /// the samples closed by the superblock of the last bit b of each value
 void closeSamples( void );

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
