/*--------------------------------------------------------------------------*/
/*---------------------- File InterleavedRankSelect.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits B of m bits with the counts
 * inside the lines of the bits, the layout of SPIDER of Laws, Bliven,
 * Conklin, Laalai, McCauley and Sturdevant: a line of the cache of 512
 * bits holds 496 bits of B and, in its first 16 bits, the 1s before it
 * since its superblock of 128 lines (63488 bits, so that the count fits),
 * and a superblock keeps the 1s before it in 64 bits apart. Hence
 * \f[ rank_1( i ) = S[ \lfloor \ell / 128 \rfloor ] + h_\ell +
 *     | B_\ell [ 0 , i \bmod 496 ) | , \qquad \ell = \lfloor i / 496
 *     \rfloor , \f]
 * \f$ h_\ell \f$ the count of the line \f$ \ell \f$ and \f$ B_\ell \f$ its
 * bits, with one access to the memory of the vector, the line, against
 * two of the directories kept apart [see RankSelect.h, CompactRankSelect.h]
 * (the array of the superblocks, 1 word per 63488 bits, stays in the
 * caches); the sum of the line is that of CompactRankSelect, 32 bytes at a
 * time by a table of the nibbles with AVX2. The counts take
 * \f$ 16 / 512 = 3.125 \% \f$ of the line.
 *
 * select keeps the line of every 8192-th bit b with the bits b before
 * that line (96 bits each, at most \f$ 96 / 8192 < 1.2 \% \f$ of m bits
 * for each value) and guesses the line of the k-th bit b between two
 * samples by interpolation, as SPIDER does,
 * \f[ \hat\ell = \ell_j + \frac{ ( k - 1 - c_j ) ( \ell_{j+1} - \ell_j ) }
 *     { c_{j+1} - c_j } , \f]
 * \f$ c_j \f$ the bits b before the line \f$ \ell_j \f$, corrects the
 * guess a line at a time by the counts of the lines (a step or two where
 * the bits b are spread evenly, \f$ \ell_{j+1} - \ell_j \f$ in the worst
 * case), and ends in the line by the sums of its words and selectInWord()
 * [see BitVector.h].
 *
 * The contracts [see Contract.h]: \f$ i \le m \f$ for rank and
 * \f$ 1 \le k \f$ up to the number of the bits b for select; the
 * postconditions are that the bit at \f$ select_b( k ) \f$ is b and that
 * its rank is k - 1, and the audit of the construction is that the
 * counts agree with a scan. The functions are defined in
 * InterleavedRankSelect.tpp.
 *
 * \see M. D. Laws, J. Bliven, K. Conklin, E. Laalai, S. McCauley,
 *      Z. S. Sturdevant, SPIDER: improved succinct rank and select
 *      performance, Proc. 22nd SEA, LIPIcs 301, 21, 2024,
 *      doi:10.4230/LIPIcs.SEA.2024.21
 * \see F. Kurpicz, Engineering compact data structures for rank and select
 *      queries on bit vectors, Proc. 29th SPIRE, LNCS 13617, 257-272, 2022,
 *      doi:10.1007/978-3-031-20643-6_19
 * \see W. Mula, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
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

#ifndef __ADS_InterleavedRankSelect
 #define __ADS_InterleavedRankSelect
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"

#include <algorithm>
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
/*----------------------- CLASS InterleavedRankSelect ----------------------*/
/*--------------------------------------------------------------------------*/

/// a static vector of bits with its counts inside its lines, rank with one
/// access to the memory
class InterleavedRankSelect {

/*--------------------------------------------------------------------------*/
 public:

 InterleavedRankSelect( void ) : InterleavedRankSelect( BitVector() ) {}

 /// the directory of the bits b, laid out with them
 explicit InterleavedRankSelect( const BitVector & b );

 /// the directory of the n bits of the words w, laid out with them
 /// \pre w.size() >= ceil( n / 64 )
 InterleavedRankSelect( std::span< const Word > w , std::size_t n );

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const { return( f_n - f_ones ); }

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
 /// \post the bit there is 1 and rank1() of it is k - 1
 [[nodiscard]] std::size_t select1( std::size_t k ) const;

 /// the place of the k-th 0
 /// \pre 1 <= k <= zeros()
 /// \post the bit there is 0 and rank0() of it is k - 1
 [[nodiscard]] std::size_t select0( std::size_t k ) const;

 /// the bits of the counts in the lines, of the superblocks and of the
 /// samples, beyond the m bits
 [[nodiscard]] std::size_t directoryBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kData = 496;  ///< the bits of B in a line
 static constexpr std::size_t kLines = 128; ///< the lines of a superblock
 static constexpr std::size_t kSample = 8192;   ///< bits b between samples

 /// the lines from the words w of n bits (nullptr for none)
 void build( const Word * w , std::size_t n );

 /// the 1s before the line l
 [[nodiscard]] std::size_t before( std::size_t l ) const {
  return( std::size_t( f_S[ l / kLines ] ) +
          std::size_t( f_L[ 8 * l ] & 0xFFFF ) );
  }

 /// the 1s among the bits [ 16 , p ) of the line l
 [[nodiscard]] std::size_t inLine( std::size_t l , unsigned p ) const;

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 /// the counts agree with a scan
 [[nodiscard]] bool consistent( void ) const;

 std::size_t f_n = 0 , f_ones = 0;
 std::vector< Word , detail::LineAllocator< Word > > f_L;   ///< the lines
 std::vector< std::uint64_t > f_S;      ///< the 1s before each superblock
 std::vector< std::uint32_t > f_sl[ 2 ];   ///< the lines of the samples
 std::vector< std::uint64_t > f_sc[ 2 ];   ///< the bits b before them

 };  // end( class InterleavedRankSelect )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/InterleavedRankSelect.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* InterleavedRankSelect.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File InterleavedRankSelect.h --------------------*/
/*--------------------------------------------------------------------------*/
