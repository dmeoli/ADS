/*--------------------------------------------------------------------------*/
/*---------------------------- File RankSelect.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits B of m bits with n 1s (section
 * 15.1.1 of the notes): \f$ rank_b( i ) \f$, the number of the bits b in
 * \f$ B[ 0 , i ) \f$, and \f$ select_b( k ) \f$, the place of the k-th bit
 * b, the primitives on which every succinct structure of ADS is built
 * (Elias-Fano, the trees by levels and by parentheses, the graphs).
 *
 * rank is the two-level directory of Jacobson, with the sizes that suit the
 * words and the caches of today: for each block of Z = 256 bits, one word
 * of 64 bits holds the absolute rank of the block in its 40 high bits and
 * the ranks of its words 1, 2 and 3 relative to the block in 8 bits each,
 * and the 1s of the word up to i are the sideways sum of the masked word,
 * which takes the place of the table of the ranks of all the small blocks
 * of the notes (a table of \f$ 2^z z \log z \f$ bits is affordable only
 * with z = log m / 2, while the sum of 64 bits is one instruction); one
 * access to the directory and one to the bits, no branch, O( 1 ) time, with
 * \f$ 64 / 256 = 25 \% \f$ of m bits more. The notes set
 * \f$ Z = \log^2 m \f$ and \f$ z = \log m / 2 \f$ so that the directory
 * takes o( m ) bits (Theorem 15.1); with constant blocks it takes a
 * constant fraction, the trade of the practical designs: Vigna's rank9
 * and the directory of Gog and Petri take the same 25% with two words per
 * 512 bits, the poppy of Zhou, Andersen and Kaminsky and the flat rank of
 * Kurpicz 3% with a sum of up to 8 words per query [see
 * CompactRankSelect.h]. Above \f$ 2^{40} \f$
 * bits a third level of 64 bits counts every \f$ 2^{40} \f$ bits.
 *
 * select is the inventory of Vigna's simple select, for the 1s and for
 * the 0s: the 1s are cut in groups of \f$ 2^\ell \f$, with
 * \f$ \ell = \lfloor \log_2 ( 1.5 \cdot 4096 \, k / m ) \rfloor \f$ for k
 * of them, so that a group spans between 3072 and 6144 bits; for each the
 * place of its first 1 is kept in 64 bits, and the places of its 1s of rank
 * multiple of \f$ 2^{\ell - 5} \f$ in 16 bits from it, a line of the
 * cache. \f$ select_1( k ) \f$ reads the place of the 1 of rank k rounded
 * down to \f$ 2^{\ell - 5} \f$ and counts the rest of the 1s word by
 * word, by the sideways sums, and ends in the word by the broadword
 * selectInWord() [see BitVector.h]: three accesses to memory and a few
 * words, O( 1 ) time where the 1s are spread evenly, with about
 * \f$ 576 / 6144 \f$ to \f$ 576 / 3072 \f$, 9.4% to 18.8%, of m bits
 * more for each value. A group that
 * spans \f$ 2^{16} \f$ bits or more, whose offsets do not fit, is sparse,
 * and its 1s are found by a binary search on the directory of rank between
 * its ends, \f$ O( \log ( m / 256 ) ) \f$ in the worst case. The notes
 * give the scheme of Clark, which splits the blocks of \f$ \log^2 m \f$
 * 1s into sparse ones, whose places are written down, and dense ones, split
 * again with \f$ ( \log \log m )^2 \f$ 1s and ended by a table: O( 1 )
 * time in the worst case and o( m ) bits (Theorem 15.2), at the price of
 * three levels whose constants are large for the sizes of practice; the
 * inventories with constant groups are what the practical designs use
 * (Vigna; Gog and Petri; the darray of Okanohara and Sadakane). A group
 * of fewer than 32 bits b, below about 1% of them, keeps the place of each
 * one, and select reads no bit at all.
 *
 * The directory is built in one pass on the words: the sideways sums
 * \f$ p_0 , \ldots , p_3 \f$ of the words of a block, at most 64 each,
 * are put in the bytes of a word, and one product gives their prefix sums
 * in the bytes, which do not overflow,
 * \f[ ( p_0 + 2^8 p_1 + 2^{16} p_2 ) \cdot ( 1 + 2^8 + 2^{16} ) \bmod
 *     2^{24} = p_0 + 2^8 ( p_0 + p_1 ) + 2^{16} ( p_0 + p_1 + p_2 ) , \f]
 * the three relative ranks of the entry; from words in memory the words
 * are copied in the same pass, above \f$ 2^{23} \f$ bits, where reading
 * them twice costs more than a copy in the caches. The inventory of select
 * of a value is built by the first select of that value, in a pass on the
 * directory, so that a structure that only ranks never pays for it; the
 * first call builds it once, whichever the threads that call it (with
 * std::call_once, and a flag read with acquire semantics before it), and a
 * copy keeps the inventories already built.
 *
 * The places count from 0: \f$ rank_1( i ) \f$ here is \f$ Rank_1( i ) \f$
 * of the notes on the places \f$ 1 , \ldots , i \f$, and \f$ select_1( k )
 * \f$ here is \f$ Select_1( k ) - 1 \f$ (in Example 15.1, \f$ rank_1( 6 ) =
 * 2 \f$ and \f$ select_1( 3 ) = 7 \f$).
 *
 * The contracts [see Contract.h]: \f$ i \le m \f$ for rank and \f$ 1 \le k
 * \f$ up to the number of the bits b for select; the postconditions are
 * that the bit at \f$ select_b( k ) \f$ is b and that its rank is k - 1, and
 * the audit of the construction is that the directory agrees with a scan.
 * The functions are defined in RankSelect.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see D. Clark, Compact Pat Trees, PhD thesis, University of Waterloo,
 *      1996
 * \see R. Gonzalez, S. Grabowski, V. Makinen, G. Navarro, Practical
 *      implementation of rank and select queries, Proc. 4th WEA, 27-38,
 *      2005
 * \see S. Vigna, Broadword implementation of rank/select queries, Proc.
 *      WEA 2008, LNCS 5038, 154-168, 2008,
 *      doi:10.1007/978-3-540-68552-4_12
 * \see D. Zhou, D. G. Andersen, M. Kaminsky, Space-efficient, high-
 *      performance rank and select structures on uncompressed bit
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
 *         Universita' di Pisa \n
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
#include "ADS/succinct/BitVector.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RankSelect ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static vector of bits with rank and select in O( 1 ) time
class RankSelect {

/*--------------------------------------------------------------------------*/
 public:

 /// the values whose select the directory answers: a structure that needs
 /// only rank, or one of the selects, spares the inventories of the others
 enum class Selects { NONE , ONES , ZEROS , BOTH };

 RankSelect( void );

 /// the directory of the bits b, which it keeps, with the selects s; the
 /// inventory of select of a value is built by its first select
 explicit RankSelect( BitVector b , Selects s = Selects::BOTH );

 /// the directory of the n bits of the words w, copied as they are
 /// counted, with the selects s
 /// \pre w.size() >= ceil( n / 64 )
 RankSelect( std::span< const Word > w , std::size_t n ,
             Selects s = Selects::BOTH );

 /// a copy, with the inventories that o has built
 RankSelect( const RankSelect & o );

 /// a move, with the inventories that o has built
 RankSelect( RankSelect && o ) noexcept;

 RankSelect & operator=( const RankSelect & o );

 RankSelect & operator=( RankSelect && o ) noexcept;

 ~RankSelect( void ) = default;

 /// the bits
 [[nodiscard]] const BitVector & bits( void ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the bit i
 /// \pre i < size()
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const;

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const;

 /// the number of 1s in the places [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank1( std::size_t i ) const;

 /// the number of 0s in the places [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank0( std::size_t i ) const;

 /// the place of the k-th 1
 /// \pre 1 <= k <= ones(), and the selects of the 1s were asked for
 /// \post the bit there is 1 and rank1() of it is k - 1
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the k-th 0
 /// \pre 1 <= k <= zeros(), and the selects of the 0s were asked for
 /// \post the bit there is 0 and rank0() of it is k - 1
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the directory, beyond those of the vector, with those of
 /// the inventories of the selects asked for, built or not
 [[nodiscard]] std::size_t directoryBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = bool;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = bool;
 using const_reference = bool;
 using const_iterator = SubscriptIterator< RankSelect , bool >;
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
  int l = 0 , g = 0 , s = 0;
  std::vector< std::uint64_t > pos;
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

 /// the inventory of the bits b, built by the first call
 const Inventory & inventory( bool b ) const;

 /// the building of the inventory of the bits b
 void build( bool b ) const;

 /// the first call of inventory( b ), out of the line of select
 void first( bool b ) const;

 /// the inventories that o has built taken (by a copy, or by a move)
 void adopt( const RankSelect & o , bool move );

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
 /// the flags that build each inventory once, and whether it is built
 mutable std::once_flag f_flag[ 2 ];
 mutable std::atomic< bool > f_ready[ 2 ] = { false , false };
 bool f_has[ 2 ] = { true , true };     ///< whether each one is built
 std::size_t f_ones = 0;

 };  // end( class RankSelect )

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
