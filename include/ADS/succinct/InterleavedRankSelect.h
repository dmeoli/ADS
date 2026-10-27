/*--------------------------------------------------------------------------*/
/*---------------------- File InterleavedRankSelect.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rank and select on a static vector of bits \f$ B \f$ of \f$ m \f$ bits
 * with the counts inside the lines of the bits, the layout of SPIDER of
 * Laws, Bliven, Conklin, Laalai, McCauley and Sturdevant: a line of the
 * cache of 512 bits holds 496 bits of \f$ B \f$ and, in its first 16 bits,
 * the 1s before it since its superblock of 128 lines (63488 bits, so that
 * the count fits), and a superblock keeps the 1s before it in 64 bits
 * apart. Hence
 * \f[ \mathrm{rank}_1( i ) = S[ \lfloor \ell / 128 \rfloor ] + h_\ell +
 *     | B_\ell [ 0 , i \bmod 496 ) | , \qquad \ell = \lfloor i / 496
 *     \rfloor , \f]
 * \f$ h_\ell \f$ the count of the line \f$ \ell \f$, \f$ B_\ell \f$ its
 * bits and \f$ | \cdot | \f$ the number of 1s, with one access to the
 * memory of the vector, the line, against two of the directories kept apart
 * [see `RankSelect.h`, `CompactRankSelect.h`] (the array of the
 * superblocks, 1 word per 63488 bits, stays in the caches); the sum of the
 * line is that of `CompactRankSelect`, 32 bytes at a time by a table of the
 * nibbles with AVX2. The counts take \f$ 16 / 512 = 3.125 \% \f$ of the
 * line.
 *
 * The select keeps the line of every 8192-th bit \f$ b \f$ with the bits
 * \f$ b \f$ before that line (96 bits each, at most
 * \f$ 96 / 8192 < 1.2 \% \f$ of \f$ m \f$ bits for each value) and guesses
 * the line of the \f$ k \f$-th bit \f$ b \f$ between two samples by
 * interpolation, as SPIDER does,
 * \f[ \hat\ell = \ell_j + \frac{ ( k - 1 - c_j ) ( \ell_{j+1} - \ell_j ) }
 *     { c_{j+1} - c_j } , \f]
 * \f$ c_j \f$ the bits \f$ b \f$ before the line \f$ \ell_j \f$, corrects
 * the guess a line at a time by the counts of the lines (a step or two
 * where the bits \f$ b \f$ are spread evenly, \f$ \ell_{j+1} - \ell_j \f$
 * in the worst case), and ends in the line by the sums of its words and
 * `selectInWord()` [see `BitVector.h`].
 *
 * The contracts [see `Contract.h`]: \f$ i \le m \f$ for the rank and
 * \f$ 1 \le k \f$ up to the number of the bits \f$ b \f$ for the select;
 * the postconditions are that the bit at \f$ \mathrm{select}_b( k ) \f$ is
 * \f$ b \f$ and that its rank is \f$ k - 1 \f$, and the audit of the
 * construction is that the counts agree with a scan. The functions are
 * defined in `InterleavedRankSelect.tpp`.
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
 *         Università di Pisa \n
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

 /// the directory of no bit
 InterleavedRankSelect( void ) : InterleavedRankSelect( BitVector() ) {}

 /// the directory of the bits \f$ b \f$, laid out with them
 explicit InterleavedRankSelect( const BitVector & b );

 /// the directory of the \f$ n \f$ bits of the words \f$ w \f$, laid out
 /// with them
 /// \pre `w.size()` \f$ \ge \lceil n / 64 \rceil \f$
 InterleavedRankSelect( std::span< const Word > w , std::size_t n );

 /// the number of bits, \f$ m \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the number of 1s
 [[nodiscard]] std::size_t ones( void ) const { return( f_ones ); }

 /// the number of 0s
 [[nodiscard]] std::size_t zeros( void ) const { return( f_n - f_ones ); }

 /// the bit \f$ i \f$
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// the number of 1s in the places \f$ [ 0 , i ) \f$
 ///
 /// In pseudocode (the rank of SPIDER, beyond the book), \f$ L_\ell \f$ the
 /// line \f$ \ell \f$, its count \f$ h_\ell \f$ in its first 16 bits, and
 /// \f$ S \f$ the counts of the superblocks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{rank1}( i ) \cr
 /// 2 & \quad \ell \gets \lfloor i / 496 \rfloor \cr
 /// 3 & \quad \textbf{return}\ S[ \lfloor \ell / 128 \rfloor ] + h_\ell +
 ///     \text{the 1s of } L_\ell \text{ in its bits } [ 16 , 16 + i \bmod
 ///     496 )
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
 /// In pseudocode (the select of SPIDER, beyond the book), for the bits
 /// \f$ b = 1 \f$ (the words complemented for the 0s), the samples
 /// \f$ ( \ell_j , c_j ) \f$ and \f$ \mathrm{before}( \ell ) \f$ the bits
 /// \f$ b \f$ before the line \f$ \ell \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{select}( k ) \cr
 /// 2 & \quad j \gets \lfloor ( k - 1 ) / 8192 \rfloor \cr
 /// 3 & \quad \ell \gets \min\{ \ell_{j+1} , \ell_j + \lfloor ( k - 1 - c_j
 ///     ) ( \ell_{j+1} - \ell_j ) / ( c_{j+1} - c_j ) \rfloor \} \cr
 /// 4 & \quad \textbf{while}\ \ell > \ell_j\ \textbf{and}\ \mathrm{before}(
 ///     \ell ) \ge k\ \textbf{do}\ \ell \gets \ell - 1 \cr
 /// 5 & \quad \textbf{while}\ \ell < \ell_{j+1}\ \textbf{and}
 ///     \ \mathrm{before}( \ell + 1 ) < k\ \textbf{do}\ \ell \gets \ell + 1
 ///     \cr
 /// 6 & \quad r \gets k - \mathrm{before}( \ell ) \cr
 /// 7 & \quad \textbf{foreach}\ \text{word } x \text{ of } L_\ell \text{,}
 ///     \text{ its count cleared}\ \textbf{do} \cr
 /// 8 & \qquad \textbf{if}\ r \le \mathrm{popcount}( x )
 ///     \ \textbf{then return}\ \text{the place of } x +
 ///     \mathrm{selectInWord}(
 ///     x , r - 1 ) \cr
 /// 9 & \qquad r \gets r - \mathrm{popcount}( x )
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

 /// the bits of the counts in the lines, of the superblocks and of the
 /// samples, beyond the \f$ m \f$ bits
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
