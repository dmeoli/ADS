/*--------------------------------------------------------------------------*/
/*-------------------------- File TinyRankSelect.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits \f$ B \f$ of \f$ m \f$ bits in
 * about 0.8% of \f$ m \f$ bits more for the rank and both selects, with
 * the select in \f$ O( 1 ) \f$ time in the worst case: the summary tree of
 * two levels and the sample tree of three levels of 3\f$ \star \f$ of
 * Kurpicz, Rigi-Luperti and Sanders, with about 4 times fewer bits for the
 * rank than `CompactRankSelect` [see `CompactRankSelect.h`] at the price
 * of blocks 4 times as long.
 *
 * The bits are cut in superblocks of \f$ L = 32 L_0 \f$ bits, each one in
 * 32 blocks of \f$ L_0 \f$ bits (the parameter `L0`, 512, 1024 or 2048,
 * i.e., 1, 2 or 4 lines of the cache), and each superblock has an entry
 * of 512 bits, a line: the 1s before it in its first word, and the 1s
 * before each of its blocks since the superblock in the 7 other words, cut
 * in 8 fields of 56 bits, one for each group of 4 blocks
 * \f$ 4 g , \ldots , 4 g + 3 \f$, which hold the 1s before the group,
 * \f$ c_{4 g} \le 28 L_0 \f$, in 16 bits, and the 1s of its blocks before
 * the block \f$ 4 g + t \f$, \f$ d_{g , t} \le 3 L_0 \f$ for
 * \f$ t = 1 , 2 , 3 \f$, in 13 bits each (the paper keeps the count of each
 * block in 12 bits and rebuilds the prefix sums by additions of vectors;
 * the prefix sums in 13 bits fit the same line and let a rank read one
 * field). Hence
 * \f[ \mathrm{rank}_1( i ) = S[ \lfloor i / L \rfloor ] + c_{4 g} +
 *     d_{g , t} + | B[ L_0 \lfloor i / L_0 \rfloor , i ) | , \qquad
 *     \lfloor i / L_0 \rfloor \bmod 32 = 4 g + t , \f]
 * with \f$ d_{g , 0} = 0 \f$ and \f$ | \cdot | \f$ the 1s: one entry and the
 * lines of the block up to \f$ i \f$, counted with no branch by the kernel
 * `detail::lineRank()` of `BitVector.h` on \f$ L_0 / 512 \f$ lines;
 * \f$ 512 / L \f$ of \f$ m \f$ bits, i.e., 3.125%, 1.5625% and 0.78125% for
 * \f$ L_0 = 512 , 1024 , 2048 \f$.
 *
 * The select of the bits \f$ b \f$ finds the superblock of the
 * \f$ k \f$-th of them by a tree of samples of three levels. The top level
 * keeps the superblocks of the bits \f$ b \f$ of rank
 * \f$ 0 , a , 2 a , \ldots \f$ (from 0), with \f$ a = 2^A \f$ about twice
 * the bits \f$ b \f$ of a superblock, which is enough where two samples are
 * fewer than \f$ \alpha = 16 \f$ superblocks apart (with the bits \f$ b \f$
 * spread evenly, nearly always); a range of \f$ \alpha \f$ superblocks or
 * more keeps the superblocks of the ranks of its bits \f$ b \f$ multiple of
 * \f$ b' = 2^{\lceil A / 2 \rceil} \f$, relative to its first one and in
 * as many bits as its length needs, and a range of \f$ \alpha \f$ or more
 * of these keeps the superblock of each of its \f$ b' \f$ bits \f$ b \f$,
 * in as many bits as its own length needs. The range found spans fewer
 * than \f$ \alpha \f$ superblocks, and a binary search on the counts of
 * their entries ends in at most 4 steps; the block is found by the 7 counts
 * of the groups and the 3 of its group, both by comparisons with no branch,
 * the line by the counts of the lines of the block, the word by the sums
 * of the 8 words of its line, and the bit by `selectInWord()`. The levels
 * are packed in vectors of bits [see `get()` of `BitVector.h`] with the
 * widths that their largest values need, so that with the bits \f$ b \f$
 * spread evenly the tree is about one field of
 * \f$ \lceil \log_2( m / L ) \rceil \f$ bits every 1 to 2 superblocks, a
 * few hundredths of a percent of \f$ m \f$ bits for each value (for
 * \f$ L_0 = 2048 \f$); the lower levels are bounded by
 * the number of the long ranges, at most one every \f$ \alpha \f$
 * superblocks (see \ref theory_succinct_rank).
 *
 * The entries and the trees are built in one pass on the words and one on
 * the entries; on the threads of an executor, the entries of the
 * superblocks are filled in parallel, then their counts summed and the
 * trees built on the calling thread, from the entries alone.
 *
 * The contracts [see `Contract.h`]: \f$ i \le m \f$ for the rank and
 * \f$ 1 \le k \f$ up to the number of the bits \f$ b \f$ for the select;
 * the postconditions are that the bit at \f$ \mathrm{select}_b( k ) \f$ is
 * \f$ b \f$ and that its rank is \f$ k - 1 \f$, and the audit of the
 * construction is that the entries agree with a scan of the words and the
 * trees with a scan of the entries, at every rank below \f$ 2^{20} \f$
 * and at the ranks multiple of \f$ b' \f$ above.
 * The functions are defined in `TinyRankSelect.tpp`.
 *
 * \see F. Kurpicz, N. Rigi-Luperti, P. Sanders, Theory meets practice for
 *      bit vectors supporting rank and select, arXiv:2509.17819, 2025,
 *      doi:10.48550/arXiv.2509.17819
 * \see F. Kurpicz, N. Rigi-Luperti, P. Sanders, Practical bit vectors
 *      supporting constant time rank and select in optimal space, Proc.
 *      34th ESA, LIPIcs 388, 8, 2026, doi:10.4230/LIPIcs.ESA.2026.8
 * \see F. Kurpicz, Engineering compact data structures for rank and select
 *      queries on bit vectors, Proc. 29th SPIRE, LNCS 13617, 257-272, 2022,
 *      doi:10.1007/978-3-031-20643-6_19
 * \see A. Golynski, Optimal lower bounds for rank and select indexes,
 *      Theoretical Computer Science 387(3), 348-359, 2007,
 *      doi:10.1016/j.tcs.2007.07.041
 * \see D. Zhou, D. G. Andersen, M. Kaminsky, Space-efficient,
 *      high-performance rank and select structures on uncompressed bit
 *      sequences, Proc. 12th SEA, LNCS 7933, 151-163, 2013,
 *      doi:10.1007/978-3-642-38527-8_15
 * \see W. Mula, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
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

#ifndef __ADS_TinyRankSelect
 #define __ADS_TinyRankSelect
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/MappedFile.h"
#include "ADS/parallel/Executor.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS BasicTinyRankSelect -----------------------*/
/*--------------------------------------------------------------------------*/

/// a static vector of bits with rank and select in \f$ 512 / ( 32 L_0 ) \f$
/// of its bits more, and a select in \f$ O( 1 ) \f$ time in the worst case
template< unsigned L0 = 2048 >
class BasicTinyRankSelect
{
 static_assert( ( L0 == 512 ) || ( L0 == 1024 ) || ( L0 == 2048 ) ,
		"BasicTinyRankSelect: blocks of 512, 1024 or 2048 bits" );

/*--------------------------------------------------------------------------*/
 public:
 /// the directory of no bit
 BasicTinyRankSelect( void ) : BasicTinyRankSelect( BitVector() ) { }

 /// the directory of the bits \f$ b \f$, which it keeps
 ///
 /// In pseudocode (the sample tree of 3\f$ \star \f$, beyond the book), for
 /// the \f$ c \f$ bits \f$ b \f$, \f$ \sigma( x ) \f$ the superblock of the
 /// one of rank \f$ \min( x , c - 1 ) \f$ (from 0), \f$ a = 2^A \f$ and
 /// \f$ b' = 2^{\lceil A / 2 \rceil} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{tree}( c ) \cr
 /// 2 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ \lceil c / a \rceil
 ///     \ \textbf{do}\ T[ j ] \gets \sigma( j a ) \cr
 /// 3 & \quad \textbf{foreach}\ j\ \text{with}\ T[ j + 1 ] - T[ j ] \ge
 ///     \alpha\ \textbf{do}\quad \triangleright\ \text{a group of } M \cr
 /// 4 & \qquad M_j[ i ] \gets \sigma( j a + i b' ) - T[ j ]\ \text{for}\ i =
 ///     0 , \ldots , a / b'\text{, in } \lceil \log_2( T[ j + 1 ] - T[ j ] +
 ///     1 ) \rceil \text{ bits} \cr
 /// 5 & \qquad \textbf{foreach}\ i\ \text{with}\ M_j[ i + 1 ] - M_j[ i ] \ge
 ///     \alpha\ \textbf{do}\quad \triangleright\ \text{a group of } D \cr
 /// 6 & \qquad\quad D_{j , i}[ t ] \gets \sigma( j a + i b' + t ) - T[ j ] -
 ///     M_j[ i ]\ \text{for}\ t = 0 , \ldots , b' - 1 \cr
 /// 7 & \quad \text{the widths of the fields are the largest that the values
 ///     of each array (of each group of } M\text{) need}
 /// \end{array}}
 /// \f]
 explicit BasicTinyRankSelect( BitVector b );

 /// the directory of the \f$ n \f$ bits of the words \f$ w \f$
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 BasicTinyRankSelect( std::span< const Word > w , std::size_t n )
     : BasicTinyRankSelect( BitVector( w , n ) ) { }

 /// the directory of `BasicTinyRankSelect( b )` above, the same entries and
 /// trees, the entries filled on the threads of the executor `ex`
 template< Executor E >
 BasicTinyRankSelect( E & ex , BitVector b );

 /// the directory of `BasicTinyRankSelect( w , n )` above on the threads of
 /// `ex`
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 template< Executor E >
 BasicTinyRankSelect( E & ex , std::span< const Word > w , std::size_t n )
     : BasicTinyRankSelect( ex , BitVector( w , n ) ) { }

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
 /// In pseudocode (the rank of 3\f$ \star \f$, beyond the book), \f$ S \f$
 /// the counts of the superblocks, \f$ c \f$ and \f$ d \f$ those of the
 /// fields of their entries:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank1}( i ) \cr
 /// 2 & \quad s \gets \lfloor i / L \rfloor;\ j \gets \lfloor i / L_0
 ///     \rfloor \bmod 32;\ g \gets \lfloor j / 4 \rfloor;\ t \gets j \bmod
 ///     4 \cr
 /// 3 & \quad r \gets S[ s ] + c_{4 g} + d_{g , t}\quad \triangleright
 ///     \ d_{g , 0} = 0 \text{, with no branch} \cr
 /// 4 & \quad \textbf{return}\ r + \text{the 1s of the lines of the block }
 ///     \lfloor i / L_0 \rfloor \text{ before } i \text{, with no branch}
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
 /// In pseudocode (the select of 3\f$ \star \f$, beyond the book), for the
 /// bits \f$ b = 1 \f$ (the counts and the words complemented for the 0s),
 /// \f$ x = k - 1 \f$ and \f$ \mathrm{before}( s ) \f$ the bits \f$ b \f$
 /// before the superblock \f$ s \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( k ) \cr
 /// 2 & \quad j \gets \lfloor x / a \rfloor;\ [ l , h ] \gets [ T[ j ] ,
 ///     T[ j + 1 ] ] \cr
 /// 3 & \quad \textbf{if}\ h - l \ge \alpha\ \textbf{then}\quad
 ///     \triangleright\ \text{the group of } M \text{ of } j \cr
 /// 4 & \qquad i \gets \lfloor ( x \bmod a ) / b' \rfloor;\ [ l , h ]
 ///     \gets [ T[ j ] + M_j[ i ] , T[ j ] + M_j[ i + 1 ] ] \cr
 /// 5 & \qquad \textbf{if}\ h - l \ge \alpha\ \textbf{then}\ l \gets h
 ///     \gets l + D_{j , i}[ x \bmod b' ] \cr
 /// 6 & \quad s \gets \text{the last superblock in } [ l , h ] \text{ with }
 ///     \mathrm{before}( s ) \le x \text{, by a binary search} \cr
 /// 7 & \quad r \gets x - \mathrm{before}( s );\ g \gets \text{the last group
 ///     with } c_{4 g} \le r;\ t \gets \text{the last } t \text{ with }
 ///     c_{4 g} + d_{g , t} \le r \cr
 /// 8 & \quad r \gets r - c_{4 g} - d_{g , t};\ q \gets \text{the line of
 ///     the block } 4 g + t \text{ with the } r \text{-th bit, by the counts
 ///     of the lines} \cr
 /// 9 & \quad p \gets \text{the last word } u \text{ of the line with }
 ///     \sum_{u' < u} \mathrm{popcount}( w_{u'} ) \le r \cr
 /// 10 & \quad \textbf{return}\ \text{the place of } w_p +
 ///     \mathrm{selectInWord}( w_p , r - \sum_{u' < p} \mathrm{popcount}(
 ///     w_{u'} ) )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \f$ `ones()`
 /// \post the bit there is 1 and `rank1()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the \f$ k \f$-th 0, by the select of `select1()` above on
 /// the complemented counts and words
 /// \pre \f$ 1 \le k \le \f$ `zeros()`
 /// \post the bit there is 0 and `rank0()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the directory, beyond those of the vector: the entries and
 /// the trees of both values
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
 using const_iterator = SubscriptIterator< BasicTinyRankSelect , bool >;
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

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "TinyRankSelect";

 /// the parameters of the layout of the directory, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix( { L0 , kAlpha } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 static constexpr std::size_t kSuper = 32 * std::size_t( L0 );  ///< bits
 static constexpr std::size_t kWords = kSuper / 64;  ///< words of a super
 static constexpr unsigned kLines = L0 / 512;        ///< the lines of a block
 static constexpr std::size_t kAlpha = 16;  ///< the longest range scanned

 /// the rank in the lines by the scalar kernel [see detail::lineRank()]
#ifndef ADS_TINY_SCALAR
 static constexpr bool kScalar = false;
#else
 static constexpr bool kScalar = true;  // a test of the scalar kernel
#endif

 /// the fewest superblocks of a chunk of the parallel construction
#ifndef ADS_TINY_CHUNK
 static constexpr std::size_t kChunk = 64;
#else
 static constexpr std::size_t kChunk = ADS_TINY_CHUNK;  // a test
#endif

 /// the tree of samples of the bits of one value
 struct Tree
 {
  unsigned A = 0;   ///< log2 of the ranks a between the top samples
  unsigned Bb = 0;  ///< log2 of the ranks b' between the middle samples
  unsigned wo = 1;  ///< the bits of a superblock at the top
  unsigned wg = 0;  ///< the bits of the number of a group of M at the top
  };

 /// the field of the group g of the entry e: the 1s before the group in
 /// its low 16 bits, and those of its blocks before the block t = 1 , 2 ,
 /// 3 in the 13 bits from 3 + 13 t
 [[nodiscard]] static std::uint64_t field( const Word * e , unsigned g ) {
  const unsigned at = 64 + 56 * g , q = at / 64 , o = at % 64;
  // the bits from 64 - o in the next word, which the mask drops for
  // o <= 8, where the field lies in the word q and q may be the last
  const Word hi = ( e[ std::min( q + 1 , 7u ) ] << 1 ) << ( 63 - o );
  return( ( ( e[ q ] >> o ) | hi ) & detail::lowMask( 56 ) );
  }

 /// the 1s before the block j of the superblock of the entry e
 [[nodiscard]] static std::size_t inSuper( const Word * e , unsigned j ) {
  const std::uint64_t f = field( e , j / 4 );
  const unsigned t = j % 4;
  return( std::size_t( ( f & 0xFFFF ) +
			( ( f >> ( 3 + 13 * t ) ) & ( t ? 0x1FFF : 0 ) ) ) );
  }

 /// the entry of the superblock s and its 1s, from its words
 std::size_t fill( std::size_t s );

 /// the counts of the entries summed, the 1s and the trees
 void finish( void );

 /// the bits b before the superblock s
 [[nodiscard]] std::size_t before( std::size_t s , bool b ) const {
  const std::size_t o = std::size_t( f_e[ 8 * s ] );
  return( b ? o : s * kSuper - o );
  }

 /// the tree of the bits b
 void tree( bool b );

 /// the superblock of the bit b of rank x, from 0, by the tree of b
 [[nodiscard]] std::size_t super( std::size_t x , bool b ) const;

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 /// the entries agree with a scan of the words, and the trees with a scan
 /// of the entries at every rank below 2^20 and at the multiples of b'
 [[nodiscard]] bool consistent( void ) const;

 BitVector f_b;
 /// 8 words for each superblock and one more
 std::vector< Word , detail::LineAllocator< Word > > f_e;
 Tree f_t[ 2 ];         ///< the shapes of the trees of each value
 BitVector f_top[ 2 ];  ///< the top samples of each value
 BitVector f_mid[ 2 ];  ///< the groups of M of each value
 BitVector f_bot[ 2 ];  ///< the groups of D of each value
 /// for each group of M three words: the bit where it starts, the bit
 /// where its groups of D start, and its widths
 std::vector< std::uint64_t > f_head[ 2 ];
 std::size_t f_ones = 0;

 };  // end( class BasicTinyRankSelect )

/// the rank and select of 3\f$ \star \f$ with blocks of 2048 bits, 0.78% of
/// the bits for the rank
using TinyRankSelect = BasicTinyRankSelect<>;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/TinyRankSelect.tpp"

/*--------------------------------------------------------------------------*/

#endif /* TinyRankSelect.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File TinyRankSelect.h ------------------------*/
/*--------------------------------------------------------------------------*/
