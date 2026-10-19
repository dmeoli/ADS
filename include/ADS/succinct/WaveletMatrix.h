/*--------------------------------------------------------------------------*/
/*-------------------------- File WaveletMatrix.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The wavelet matrix of Claude, Navarro and Ordonez: a static sequence
 * \f$ S[ 0 , n ) \f$ of integers of w bits, i.e., on the alphabet
 * \f$ [ 0 , 2^w ) \f$, in \f$ n w \f$ bits plus the directories of rank and
 * select [see RankSelect.h], with access, rank and select of any symbol in
 * O( w ) time, and the queries on a range of places that a sequence of
 * integers needs (the k-th smallest item of a range, the items below a
 * value), also in O( w ) time.
 *
 * The level \f$ \ell = 0 , \ldots , w - 1 \f$ is a vector of bits
 * \f$ B_\ell \f$ that holds the bit \f$ w - 1 - \ell \f$ (from the most
 * significant one) of each item, in the order that the levels above leave:
 * \f$ B_0 \f$ is in the order of S, and the items go to the next level
 * stably sorted by the bit, the \f$ Z_\ell \f$ ones with a 0 first and those
 * with a 1 after them. Hence the item at the place i of a level is at the
 * place \f$ rank_0( i ) \f$ of the next one if its bit is 0, and at
 * \f$ Z_\ell + rank_1( i ) \f$ if it is 1, and the items equal to c, in the
 * order of S, are consecutive at the last level, at a place that the bits
 * of c alone determine:
 *
 * - access( i ) follows the item down, a bit and a rank per level;
 * - rank( c , i ), the number of c in \f$ S[ 0 , i ) \f$, follows the
 *   places 0 and i down along the bits of c, and the difference at the
 *   last level is the answer;
 * - select( c , k ), the place of the k-th c, goes down to the place of
 *   the k-th c at the last level and back up by select, \f$ select_0 \f$
 *   or \f$ select_1 \f$ as the bit of c at each level;
 * - quantile( i , j , k ), the k-th smallest item of \f$ S[ i , j ) \f$,
 *   goes down the levels with the range, to the 0s if the range has more
 *   than k of them and to the 1s otherwise, and countLess( i , j , c )
 *   sums the 0s that the range leaves at the levels where c has a 1.
 *
 * Unlike the wavelet tree of Grossi, Gupta and Vitter, whose nodes split
 * the alphabet in halves and need a pointer or an offset each, the matrix
 * keeps one vector of n bits per level and no tree, at the same costs: it
 * is the form of choice for the large alphabets (Claude, Navarro and
 * Ordonez), and the base of the compressed indices of the integers and of
 * the r-index. A wavelet tree shaped by the code of Huffman (the wt_huff of
 * sdsl) takes \f$ n ( H_0 + 1 ) \f$ bits instead of n w and is faster on
 * a skewed alphabet; the matrix here is the form for the alphabets of
 * integers with no known distribution.
 *
 * The contracts [see Contract.h]: the items fit in w bits, \f$ 1 \le w
 * \le 64 \f$, the places are in the sequence, i <= j for a range, and k
 * counts from 1 for select and from 0 for quantile, below the number of
 * the items; the audit of the construction is that access() gives back
 * each item. The functions are defined in WaveletMatrix.tpp.
 *
 * \see F. Claude, G. Navarro, A. Ordonez, The wavelet matrix: an efficient
 *      wavelet tree for large alphabets, Information Systems 47, 15-32,
 *      2015, doi:10.1016/j.is.2014.06.002
 * \see R. Grossi, A. Gupta, J. S. Vitter, High-order entropy-compressed
 *      text indexes, Proc. 14th SODA, 841-850, 2003
 * \see G. Navarro, Wavelet trees for all, Journal of Discrete Algorithms
 *      25, 2-20, 2014, doi:10.1016/j.jda.2013.07.004
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

#ifndef __ADS_WaveletMatrix
 #define __ADS_WaveletMatrix
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS WaveletMatrix --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static sequence of integers of w bits with access, rank, select and
/// the range quantiles in O( w ) time
class WaveletMatrix {

/*--------------------------------------------------------------------------*/
 public:

 WaveletMatrix( void ) = default;

 /// the sequence x, its items of w bits (0: the bits of the largest one)
 /// \pre 0 <= w <= 64 and the items are below 2^w
 explicit WaveletMatrix( std::span< const std::uint64_t > x , int w = 0 );

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the bits of an item
 [[nodiscard]] int width( void ) const { return( int( f_lev.size() ) ); }

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the number of c in the places [ 0 , i )
 /// \pre i <= size()
 [[nodiscard]] std::size_t rank( std::uint64_t c , std::size_t i ) const;

 /// the place of the k-th c
 /// \pre 1 <= k <= rank( c , size() )
 [[nodiscard]] std::size_t select( std::uint64_t c , std::size_t k ) const;

 /// the k-th smallest item of the places [ i , j ), from k = 0
 /// \pre i <= j <= size() and k < j - i
 [[nodiscard]] std::uint64_t quantile( std::size_t i , std::size_t j ,
                                       std::size_t k ) const;

 /// the number of the items below c in the places [ i , j )
 /// \pre i <= j <= size()
 [[nodiscard]] std::size_t countLess( std::size_t i , std::size_t j ,
                                      std::uint64_t c ) const;

 /// the bits of the levels and of their directories
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::uint64_t;
 using const_reference = std::uint64_t;
 using const_iterator = SubscriptIterator< WaveletMatrix , std::uint64_t >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the bit of c at the level l
 [[nodiscard]] bool bit( std::uint64_t c , std::size_t l ) const {
  return( ( c >> ( f_lev.size() - 1 - l ) ) & 1 );
  }

 /// the place of c at the last level, where its items start
 [[nodiscard]] std::size_t start( std::uint64_t c ) const;

 std::size_t f_n = 0;
 std::vector< RankSelect > f_lev;        ///< the levels, the top first
 std::vector< std::size_t > f_zeros;     ///< the 0s of each level

 };  // end( class WaveletMatrix )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/WaveletMatrix.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WaveletMatrix.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File WaveletMatrix.h ------------------------*/
/*--------------------------------------------------------------------------*/
