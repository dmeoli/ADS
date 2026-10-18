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
 * words of today: the absolute rank of each superblock of Z = 512 bits in
 * 64 bits, the rank of each word of z = 64 bits relative to its superblock
 * in 16 bits, and the 1s of the word up to i by the sideways sum of the
 * masked word [see Bitwise.h], which takes the place of the table of the
 * ranks of all the small blocks of the notes (a table of
 * \f$ 2^z z \log z \f$ bits is affordable only with z = log m / 2, while the
 * sum of 64 bits costs a few instructions); 3 memory accesses and O( 1 )
 * time, with \f$ 64 / 512 + 16 / 64 = 37.5 \% \f$ of m bits more. The
 * notes set \f$ Z = \log^2 m \f$ and \f$ z = \log m / 2 \f$ so that the
 * directory takes o( m ) bits (Theorem 15.1); with constant blocks it takes
 * a constant fraction, the trade of the practical designs (Vigna's rank9,
 * the 25% of the rank of Gonzalez, Grabowski, Makinen and Navarro).
 *
 * select is the sampled design: the superblock of every 4096-th 1 (and
 * 0) is kept, a query does a binary search on the absolute ranks of the
 * superblocks between two samples, then a scan of the 8 relative ranks of
 * its superblock, and ends in the word by the broadword selectInWord() [see
 * BitVector.h]; it takes O( 1 ) time where the 1s are spread evenly and
 * \f$ O( \log ( m / 512 ) ) \f$ in the worst case, with less than 1% of
 * m bits more. The notes give the scheme of Clark, which splits the blocks
 * of \f$ \log^2 m \f$ 1s into sparse ones, whose places are written
 * down, and dense ones, split again with \f$ ( \log \log m )^2 \f$ 1s
 * and ended by a table: O( 1 ) time in the worst case and o( m ) bits
 * (Theorem 15.2), at the price of three levels whose constants are large
 * for the sizes of practice; the sampling with a search is what the
 * practical libraries use (Navarro and Providel, sdsl of Gog and Petri), and
 * the worst case is kept small by the bounded search between samples.
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
#include <cstddef>
#include <cstdint>
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

 RankSelect( void );

 /// the directory of the bits b, which it keeps
 explicit RankSelect( BitVector b );

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

 static constexpr std::size_t kWords = 8;    ///< words in a superblock
 static constexpr std::size_t kSample = 4096;   ///< 1s between samples

 /// the ones before the word w
 [[nodiscard]] std::size_t onesBefore( std::size_t w ) const;

 /// the samples of the superblocks of every kSample-th bit b
 void sample( bool b );

 /// the place of the k-th bit b
 [[nodiscard]] std::size_t select( std::size_t k , bool b ) const;

 /// the directory agrees with a scan of the bits
 [[nodiscard]] bool consistent( void ) const;

 BitVector f_b;
 std::vector< std::uint64_t > f_super;   ///< the 1s before a superblock
 std::vector< std::uint16_t > f_rel;     ///< the 1s before a word in it
 std::vector< std::uint32_t > f_s1;      ///< superblock of the samples of 1
 std::vector< std::uint32_t > f_s0;      ///< superblock of the samples of 0
 std::size_t f_ones = 0;

 };  // end( class RankSelect )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/RankSelect.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RankSelect.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RankSelect.h --------------------------*/
/*--------------------------------------------------------------------------*/
