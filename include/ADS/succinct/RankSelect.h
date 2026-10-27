/*--------------------------------------------------------------------------*/
/*---------------------------- File RankSelect.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits \f$ B \f$ of \f$ m \f$ bits
 * with \f$ n \f$ 1s (chapter 15, Compressed Data Structures, of Ferragina's
 * book):
 * \f$ \mathrm{rank}_b( i ) \f$, the number of the bits \f$ b \f$ in
 * \f$ B[ 0 , i ) \f$, and \f$ \mathrm{select}_b( k ) \f$, the place of the
 * \f$ k \f$-th bit \f$ b \f$, the primitives on which every succinct
 * structure of ADS is built (Elias-Fano, the trees by levels and by
 * parentheses, the graphs).
 *
 * The rank is the two-level directory of Jacobson, with the sizes that suit
 * the words and the caches of today: for each block of \f$ Z = 256 \f$
 * bits, one word of 64 bits holds the absolute rank of the block in its 40
 * high bits and the ranks of its words 1, 2 and 3 relative to the block in
 * 8 bits each, and the 1s of the word up to \f$ i \f$ are the sideways sum
 * of the masked word, which takes the place of the table of the ranks of
 * all the small blocks of the same chapter (a table of
 * \f$ 2^z z \log z \f$ bits is affordable only with
 * \f$ z = \log m / 2 \f$, while the sum of 64 bits is one instruction); one
 * access to the directory and one to the bits, no branch, \f$ O( 1 ) \f$
 * time, with \f$ 64 / 256 = 25 \% \f$ of \f$ m \f$ bits more. The classical
 * analysis sets \f$ Z = \log^2 m \f$ and \f$ z = \log m / 2 \f$, so that
 * the directory takes \f$ o( m ) \f$ bits with the rank in \f$ O( 1 ) \f$
 * time; with constant blocks it takes a constant fraction, the trade of
 * the practical designs: the rank9 of Vigna and the directory of Gog and
 * Petri take the same 25% with two words per 512 bits, the poppy of Zhou,
 * Andersen and Kaminsky and the flat rank of Kurpicz 3% with a sum of up
 * to 8 words per query [see `CompactRankSelect.h`]. Above
 * \f$ 2^{40} \f$ bits a third level of 64 bits counts every
 * \f$ 2^{40} \f$ bits.
 *
 * The select is the inventory of the simple select of Vigna, for the 1s and
 * for the 0s: the 1s are cut in groups of \f$ 2^\ell \f$, with
 * \f$ \ell = \lfloor \log_2( 1.5 \cdot 4096 \, k / m ) \rfloor \f$ for
 * \f$ k \f$ of them, so that a group spans between 3072 and 6144 bits on
 * average; for each the place of its first 1 is kept in 64 bits, and the
 * places of its 1s of rank multiple of \f$ 2^{\ell - 5} \f$ in 16 bits from
 * it, a line of the cache. \f$ \mathrm{select}_1( k ) \f$ reads the place
 * of the 1 of rank \f$ k \f$ rounded down to \f$ 2^{\ell - 5} \f$, counts
 * the rest of the 1s word by word, by the sideways sums, and ends in the
 * word by the broadword `selectInWord()` [see `BitVector.h`]: three
 * accesses to memory and a few words, \f$ O( 1 ) \f$ time where the 1s are
 * spread evenly, with about \f$ 576 / 6144 \f$ to \f$ 576 / 3072 \f$, 9.4%
 * to 18.8%, of \f$ m \f$ bits more for each value. A group that spans
 * \f$ 2^{16} \f$ bits or more, whose offsets do not fit, is sparse, and its
 * 1s are found by a binary search on the directory of rank between its
 * ends, \f$ O( \log( m / 256 ) ) \f$ in the worst case. The same chapter
 * gives the scheme of Clark, which splits the blocks of \f$ \log^2 m \f$
 * 1s into sparse ones, whose places are written down, and dense ones, split
 * again with \f$ ( \log \log m )^2 \f$ 1s and ended by a table:
 * \f$ O( 1 ) \f$ time in the worst case and \f$ o( m ) \f$ bits, at the
 * price of three levels whose constants are large for
 * the sizes of practice; the inventories with constant groups are what the
 * practical designs use (Vigna; Gog and Petri; the darray of Okanohara and
 * Sadakane). A group of fewer than 32 bits \f$ b \f$, below about 1% of
 * them, keeps the place of each one, and the select reads no bit at all.
 *
 * The directory is built in one pass on the words: the sideways sums
 * \f$ p_0 , \ldots , p_3 \f$ of the words of a block, at most 64 each,
 * are put in the bytes of a word, and one product gives their prefix sums
 * in the bytes, which do not overflow,
 * \f[ ( p_0 + 2^8 p_1 + 2^{16} p_2 ) \cdot ( 1 + 2^8 + 2^{16} ) \bmod
 *     2^{24} = p_0 + 2^8 ( p_0 + p_1 ) + 2^{16} ( p_0 + p_1 + p_2 ) , \f]
 * the three relative ranks of the entry; from words in memory the words
 * are copied in the same pass, above \f$ 2^{23} \f$ bits, where reading
 * them twice costs more than a copy in the caches. The inventories of
 * select are built in the constructor (`RankSelect`, the default), or by
 * the first select of each value (`LazyRankSelect`), in a pass on the
 * directory, so that a structure that only ranks never pays for them; the
 * first call builds an inventory once, whichever the threads that call it
 * (with `std::call_once`, and a flag read with acquire semantics before
 * it), and a copy keeps the inventories already built. The lazy form costs
 * the select 10 to 15%: the load of the flag with acquire semantics makes
 * the compiler read the fields of the inventory again at each query, which
 * the eager form, with no flag at all (the template parameter `Lazy`),
 * keeps in plain fields.
 *
 * The places count from 0: \f$ \mathrm{rank}_1( i ) \f$ here is
 * \f$ \mathrm{Rank}_1( i ) \f$ of Ferragina's book, which counts the places
 * from 1, on the places \f$ 1 , \ldots , i \f$, and
 * \f$ \mathrm{select}_1( k ) \f$ here is \f$ \mathrm{Select}_1( k ) - 1 \f$.
 *
 * The contracts [see `Contract.h`]: \f$ i \le m \f$ for the rank and
 * \f$ 1 \le k \f$ up to the number of the bits \f$ b \f$ for the select;
 * the postconditions are that the bit at \f$ \mathrm{select}_b( k ) \f$ is
 * \f$ b \f$ and that its rank is \f$ k - 1 \f$, and the audit of the
 * construction is that the directory agrees with a scan. The functions are
 * defined in `RankSelect.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see D. Clark, Compact Pat Trees, PhD thesis, University of Waterloo,
 *      1996
 * \see R. González, S. Grabowski, V. Mäkinen, G. Navarro, Practical
 *      implementation of rank and select queries, Proc. 4th WEA, 27-38,
 *      2005
 * \see S. Vigna, Broadword implementation of rank/select queries, Proc.
 *      WEA 2008, LNCS 5038, 154-168, 2008,
 *      doi:10.1007/978-3-540-68552-4_12
 * \see D. Okanohara, K. Sadakane, Practical entropy-compressed rank/select
 *      dictionary, Proc. 9th ALENEX, 60-70, 2007,
 *      doi:10.1137/1.9781611972870.6
 * \see D. Zhou, D. G. Andersen, M. Kaminsky, Space-efficient,
 *      high-performance rank and select structures on uncompressed bit
 *      sequences, Proc. SEA 2013, LNCS 7933, 151-163, 2013,
 *      doi:10.1007/978-3-642-38527-8_15
 * \see F. Kurpicz, Engineering compact data structures for rank and select
 *      queries on bit vectors, Proc. SPIRE 2022, LNCS 13617, 257-272,
 *      2022, doi:10.1007/978-3-031-20643-6_19
 * \see G. Navarro, E. Providel, Fast, small, simple rank/select on bitmaps,
 *      Proc. SEA 2012, LNCS 7276, 295-306, 2012,
 *      doi:10.1007/978-3-642-30850-5_26
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

#ifndef __ADS_RankSelect
 #define __ADS_RankSelect
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/parallel/Executor.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BasicRankSelect --------------------------*/
/*--------------------------------------------------------------------------*/

/// the values whose select a directory answers: a structure that needs
/// only rank, or one of the selects, spares the inventories of the others
struct RankSelectOptions {
 /// the selects that a directory answers
 enum class Selects {
  NONE ,    ///< no select, the rank only
  ONES ,    ///< `select1()`
  ZEROS ,   ///< `select0()`
  BOTH      ///< `select1()` and `select0()`
  };
 };

/// a static vector of bits with rank and select in \f$ O( 1 ) \f$ time, the
/// inventories of select built in the constructor if not `Lazy`, and at the
/// first select of each value if `Lazy`
template< bool Lazy >
class BasicRankSelect : public RankSelectOptions {

/*--------------------------------------------------------------------------*/
 public:

 /// the directory of no bit
 BasicRankSelect( void );

 /// the directory of the bits \f$ b \f$, which it keeps, with the selects
 /// \f$ s \f$
 ///
 /// In pseudocode (the directory of Jacobson, with the blocks of 256 bits,
 /// beyond the book),
 /// \f$ D[ t ] \f$ the entry of the block \f$ t \f$ and \f$ W \f$ the words:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{directory}( W ) \cr
 /// 2 & \quad c \gets 0 \cr
 /// 3 & \quad \textbf{foreach}\ \text{block } t \text{ of four words}
 ///     \ W[ 4 t .. 4 t + 3 ]\ \textbf{do} \cr
 /// 4 & \qquad p_j \gets \mathrm{popcount}( W[ 4 t + j ] )\ \text{for }
 ///     j = 0 , 1 , 2 , 3 \cr
 /// 5 & \qquad r \gets ( p_0 + 2^8 p_1 + 2^{16} p_2 ) \cdot ( 1 + 2^8 +
 ///     2^{16} ) \bmod 2^{24}\quad \triangleright\ \text{the relative ranks}
 ///     \cr
 /// 6 & \qquad D[ t ] \gets 2^{24} c + r;\ c \gets c + p_0 + p_1 + p_2 +
 ///     p_3 \cr
 /// 7 & \quad \textbf{foreach}\ \text{value } b \text{ of the selects } s
 ///     \ \textbf{do}\ \text{build the inventory of } b \text{ (if not }
 ///     \mathit{Lazy} \text{)}
 /// \end{array}}
 /// \f]
 explicit BasicRankSelect( BitVector b , Selects s = Selects::BOTH );

 /// the directory of the \f$ n \f$ bits of the words \f$ w \f$, copied as
 /// they are counted, with the selects \f$ s \f$
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 BasicRankSelect( std::span< const Word > w , std::size_t n ,
                  Selects s = Selects::BOTH );

 /// the directory of `BasicRankSelect( b , s )` above, the same entries
 /// and inventories, built on the threads of the executor `ex` (see
 /// \ref theory_succinct_parallel); if `Lazy`, the inventories are built
 /// by the first selects, on the calling thread
 ///
 /// In pseudocode (the counts of the chunks, their prefix sums and the
 /// entries of each chunk from its prefix, beyond the book), the blocks
 /// cut into chunks \f$ C_0 , \ldots , C_{q-1} \f$ of consecutive blocks,
 /// those of a top block never in two chunks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{directory}( \mathit{ex} , W ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{chunk } C_k\ \textbf{do in parallel}
 ///     \ a_k \gets \text{the 1s of the words of } C_k \cr
 /// 3 & \quad c_0 \gets 0;\ c_{k + 1} \gets c_k + a_k \text{ for }
 ///     k = 0 , \ldots , q - 2\quad \triangleright\ \text{the 1s before}
 ///     \text{ each chunk} \cr
 /// 4 & \quad T[ j ] \gets c_k \text{ for the chunk } C_k \text{ that }
 ///     \text{starts the top block } j \cr
 /// 5 & \quad \textbf{foreach}\ \text{chunk } C_k\ \textbf{do in parallel}
 ///     \ \text{the entries of its blocks as in } \mathrm{directory}( W )
 ///     \text{, from } c = c_k \cr
 /// 6 & \quad \textbf{foreach}\ \text{value } b \text{ of the selects }
 ///     s\ \textbf{do}\ \mathrm{inventory}( \mathit{ex} , b ) \text{ (if }
 ///     \text{not } \mathit{Lazy} \text{)}
 /// \end{array}}
 /// \f]
 template< Executor E >
 BasicRankSelect( E & ex , BitVector b , Selects s = Selects::BOTH );

 /// the directory of `BasicRankSelect( w , n , s )` above, the words
 /// copied by the chunks that count them, on the threads of `ex`
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 template< Executor E >
 BasicRankSelect( E & ex , std::span< const Word > w , std::size_t n ,
                  Selects s = Selects::BOTH );

 /// a copy, with the inventories that \f$ o \f$ has built
 BasicRankSelect( const BasicRankSelect & o );

 /// a move, with the inventories that \f$ o \f$ has built
 BasicRankSelect( BasicRankSelect && o ) noexcept;

 /// a copy, with the inventories that \f$ o \f$ has built
 BasicRankSelect & operator=( const BasicRankSelect & o );

 /// a move, with the inventories that \f$ o \f$ has built
 BasicRankSelect & operator=( BasicRankSelect && o ) noexcept;

 /// the directory and its bits released
 ~BasicRankSelect( void ) = default;

 /// the bits
 [[nodiscard]] const BitVector & bits( void ) const;

 /// the number of bits, \f$ m \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// the bit \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const;

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const;

 /// the number of 1s in the places \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (the directory of Jacobson, with the sideways sum of a
 /// word in place of the table of the small blocks, beyond the book),
 /// \f$ D \f$ the entries of the blocks of the constructor above,
 /// \f$ W \f$ the words and \f$ T \f$ the counts of the top level:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank1}( i ) \cr
 /// 2 & \quad e \gets D[ \lfloor i / 256 \rfloor ];\ q \gets \lfloor i / 64
 ///     \rfloor \bmod 4 \cr
 /// 3 & \quad r \gets \lfloor e / 2^{24} \rfloor + \text{the count of the }
 ///     \text{word } q \text{ in } e\ \text{(0 for } q = 0 \text{)} \cr
 /// 4 & \quad \textbf{if}\ m > 2^{40}\ \textbf{then}\ r \gets r + T[ \lfloor i
 ///     / 2^{40} \rfloor ] \cr
 /// 5 & \quad \textbf{return}\ r + \mathrm{popcount}( W[ \lfloor i / 64
 ///     \rfloor ] \bmod 2^{i \bmod 64} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le \f$ `size()`
 [[nodiscard]] std::size_t rank1( std::size_t i ) const;

 /// the number of 0s in the places \f$ [ 0 , i ) \f$, i.e.,
 /// \f$ i - \mathrm{rank}_1( i ) \f$
 /// \pre \f$ i \le \f$ `size()`
 [[nodiscard]] std::size_t rank0( std::size_t i ) const;

 /// the place of the \f$ k \f$-th 1
 ///
 /// In pseudocode (the simple select of Vigna, beyond the book), for the
 /// bits \f$ b = 1 \f$ (and with the words complemented for the 0s), the
 /// group \f$ j \f$ of \f$ 2^\ell \f$ of them starting at \f$ P[ j ] \f$ and
 /// its 1s of rank multiple of \f$ 2^g \f$ at the offsets \f$ O \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( k ) \cr
 /// 2 & \quad r \gets k - 1;\ j \gets \lfloor r / 2^\ell \rfloor \cr
 /// 3 & \quad \textbf{if}\ \text{the group } j \text{ is sparse }
 ///     \ \textbf{then} \cr
 /// 4 & \qquad \textbf{return}\ \text{the place by a binary search of the }
 ///     \text{blocks of } D \text{ between } P[ j ] \text{ and } P[ j + 1 ]
 ///     \cr
 /// 5 & \quad p \gets P[ j ] + O[ \lfloor r / 2^g \rfloor ]\quad
 ///     \triangleright\ \text{the 1 of rank } 2^g \lfloor r / 2^g \rfloor \cr
 /// 6 & \quad \textbf{if}\ g = 0\ \textbf{then return}\ p \cr
 /// 7 & \quad t \gets r \bmod 2^g;\ w \gets \lfloor p / 64 \rfloor;\ x \gets
 ///     W[ w ] \text{ with the bits before } p \text{ cleared} \cr
 /// 8 & \quad \textbf{while}\ t \ge \mathrm{popcount}( x )\ \textbf{do}
 ///     \ t \gets t - \mathrm{popcount}( x );\ w \gets w + 1;\ x \gets W[ w ]
 ///     \cr
 /// 9 & \quad \textbf{return}\ 64 w + \mathrm{selectInWord}( x , t )
 /// \end{array}}
 /// \f]
 /// \pre \f$ 1 \le k \le \f$ `ones()`, and the selects of the 1s were asked
 ///      for
 /// \post the bit there is 1 and `rank1()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the \f$ k \f$-th 0, by the select of `select1()` above on
 /// the complemented words
 /// \pre \f$ 1 \le k \le \f$ `zeros()`, and the selects of the 0s were asked
 ///      for
 /// \post the bit there is 0 and `rank0()` of it is \f$ k - 1 \f$
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the directory, beyond those of the vector, with those of
 /// the inventories of the selects asked for, built or not
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
 using const_iterator = SubscriptIterator< BasicRankSelect , bool >;
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

 /// the bits of a block of the directory, and of a quarter (a word)
 static constexpr std::size_t kBlock = 256;
 static constexpr int kRel = 8;
 static constexpr std::size_t kQuarter = kBlock / 4;
 static constexpr std::uint64_t kRelMask = ( std::uint64_t( 1 ) << kRel ) - 1;
 /// the log of the bits counted by an absolute count of the top level,
 /// which the 64 - 3 kRel bits of a block can count
#ifndef ADS_RANKSELECT_TOP
 static constexpr int kTop = 64 - 3 * kRel;
#else
 static constexpr int kTop = ADS_RANKSELECT_TOP;   // a test of the top level
#endif
 /// the bits from which the words are copied as they are counted, where
 /// reading them twice costs more than a copy in the caches
#ifndef ADS_RANKSELECT_FUSE
 static constexpr std::size_t kFuse = std::size_t( 1 ) << 23;
#else
 static constexpr std::size_t kFuse = ADS_RANKSELECT_FUSE;   // a test
#endif

 /// the fewest blocks of a chunk of the parallel construction
#ifndef ADS_RANKSELECT_CHUNK
 static constexpr std::size_t kChunk = 4096;
#else
 static constexpr std::size_t kChunk = ADS_RANKSELECT_CHUNK;   // a test
#endif

 /// the bits of the vector spanned by a group of select, on average,
 /// and the most of its places that its 16 bits offsets can tell apart
 static constexpr std::size_t kGroup = 4096;
 static constexpr std::size_t kSparse = std::size_t( 1 ) << 16;

 /// the inventory of select for the bits of one value: the bits b are in
 /// groups of 2^l, a group starts at the place pos[ j ] (whose top bit
 /// marks a sparse group, which spans kSparse places or more), and the
 /// bits b at each multiple of 2^g in it are at 16 bits offsets from that
 /// place, 2^s = 2^( l - g ) <= 32 of them a group; the last place is the
 /// size of the vector
 struct Inventory {
  int l = 0 ,   ///< \f$ l \f$, a group has \f$ 2^l \f$ bits \f$ b \f$
      g = 0 ,   ///< \f$ g \f$, an offset every \f$ 2^g \f$ of them
      s = 0;    ///< \f$ s = l - g \f$
  std::vector< std::uint64_t > pos;   ///< the place where each group starts
  /// the offsets of the bits \f$ b \f$ of each group from its place
  std::vector< std::uint16_t , detail::LineAllocator< std::uint16_t > > off;
  };

 /// the bits b before the block t
 [[nodiscard]] std::size_t before( std::size_t t , bool b ) const;

 /// the selects s asked for, and the directory of the bits, the words
 /// first copied from w where w is not null
 void directory( Selects s , const Word * w );

 /// the shape of the inventory of the bits b: its l, g and s, and the
 /// number of its groups
 void shape( bool b , Inventory & I , std::size_t & groups ) const;

 /// the inventory of the bits b (built by the first call if Lazy)
 const Inventory & inventory( bool b ) const;

 /// the building of the inventory of the bits b
 void build( bool b ) const;

 /// the selects s asked for, and the directory of the bits, the words
 /// first copied from w where w is not null, on the threads of ex
 template< Executor E >
 void directory( E & ex , Selects s , const Word * w );

 /// the building of the inventory of the bits b on the threads of ex: the
 /// places of the groups, then the offsets from them
 template< Executor E >
 void build( E & ex , bool b ) const;

 /// f( c , p ) for each bit b of the blocks [ t0 , t1 ) whose rank c
 /// (from 0) is a multiple of 2^e, p its place, c below total
 template< class F >
 void samples( bool b , std::size_t t0 , std::size_t t1 , int e ,
               std::size_t total , F && f ) const;

 /// the first call of inventory( b ) if Lazy, out of the line of select
 void first( bool b ) const;

 /// the inventories that o has built taken (by a copy, or by a move)
 void adopt( const BasicRankSelect & o , bool move );

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 /// the place of the k-th bit b, in the sparse group from the place p to
 /// the place q, by a search of the directory
 [[nodiscard]] std::size_t selectSparse( std::size_t k , bool b ,
                                         std::size_t p ,
                                         std::size_t q ) const;

 /// the directory agrees with a scan of the bits
 [[nodiscard]] bool consistent( void ) const;

 BitVector f_b;
 /// for each block and one more, the 1s before it since its top block in
 /// the high bits, and those before its words 1, 2 and 3 since the block
 /// in kRel bits each, the word 1 in the lowest
 std::vector< std::uint64_t , detail::LineAllocator< std::uint64_t > >
                                                                     f_dir;
 /// the 1s before each 2^kTop bits, when there are more (f_big)
 std::vector< std::uint64_t > f_top;
 bool f_big = false;                    ///< more than 2^kTop bits
 mutable Inventory f_inv[ 2 ];          ///< of the 0s and of the 1s
 bool f_has[ 2 ] = { true , true };     ///< whether each one is built
 std::size_t f_ones = 0;
 /// if Lazy, the flags that build each inventory once, and whether it is
 /// built; nothing if not
 struct Once {
  mutable std::once_flag flag[ 2 ];   ///< the flag of each inventory
  /// whether each inventory is built
  mutable std::atomic< bool > ready[ 2 ] = { false , false };
  };
 struct None {};
 [[no_unique_address]] std::conditional_t< Lazy , Once , None > f_once;

 };  // end( class BasicRankSelect )

/// the directory with its inventories of select built in the constructor,
/// the fastest select
using RankSelect = BasicRankSelect< false >;

/// the directory with its inventories of select built at the first select
/// of each value, the fastest construction (for a directory that may never
/// select, or that selects only some values)
using LazyRankSelect = BasicRankSelect< true >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/RankSelect.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RankSelect.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RankSelect.h --------------------------*/
/*--------------------------------------------------------------------------*/
