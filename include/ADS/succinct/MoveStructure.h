/*--------------------------------------------------------------------------*/
/*-------------------------- File MoveStructure.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The move structure of Nishimoto and Tabei: a permutation \f$ \pi \f$ of
 * \f$ [ 0 , n ) \f$ that shifts each of m intervals alike, the interval
 * \f$ [ p_i , p_{i+1} ) \f$ onto \f$ [ q_i , q_i + p_{i+1} - p_i ) \f$ (the
 * LF-mapping of a transform of Burrows and Wheeler on its runs, the
 * function \f$ \phi \f$ of the r-index on the places of the starts of the
 * runs [see RIndex.h]), with \f$ \pi( x ) \f$ and the interval of
 * \f$ \pi( x ) \f$ in O( 1 ) time from x and its interval, in O( m ) words
 * rather than n.
 *
 * Each interval i keeps \f$ p_i \f$, \f$ q_i \f$ and the interval
 * \f$ d_i \f$ that holds \f$ q_i \f$, so that
 * \f[ \pi( x ) = q_i + ( x - p_i ) , \qquad x \in [ p_i , p_{i+1} ) , \f]
 * and its interval is found from \f$ d_i \f$ forward, past the starts
 * \f$ p_j \le \pi( x ) \f$. The structure is balanced: no image
 * \f$ [ q_i , q_i + p_{i+1} - p_i ) \f$ holds 4 starts or more, which the
 * construction ensures by cutting an interval whose image holds 4 starts
 * at the third of them, so that the first part holds 2 (the new start may
 * weigh on another image, which is checked in turn); the intervals become
 * at most 2m (Nishimoto and Tabei, for this balance), and the search
 * forward takes at most 3 steps. Each interval carries a tag of
 * its own (the symbol of its run), which its parts keep.
 *
 * The contracts [see Contract.h]: the intervals partition [ 0 , n ) and
 * their images too, and the places and the intervals of move() agree; the
 * audit of the construction is the balance. The functions are defined in
 * MoveStructure.tpp.
 *
 * \see T. Nishimoto, Y. Tabei, Optimal-time queries on BWT-runs compressed
 *      indexes, Proc. 48th ICALP, LIPIcs 198, 101, 2021,
 *      doi:10.4230/LIPIcs.ICALP.2021.101
 * \see N. K. Brown, T. Gagie, M. Rossi, RLBWT tricks, Proc. 20th SEA,
 *      LIPIcs 233, 16, 2022, doi:10.4230/LIPIcs.SEA.2022.16
 * \see N. Bertram, J. Fischer, L. Nalbach, Move-r: optimizing the r-index,
 *      Proc. 22nd SEA, LIPIcs 301, 1, 2024, doi:10.4230/LIPIcs.SEA.2024.1
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

#ifndef __ADS_MoveStructure
 #define __ADS_MoveStructure
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
#include <map>
#include <span>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MoveStructure --------------------------*/
/*--------------------------------------------------------------------------*/

/// a permutation of [ 0 , n ) that shifts each of its intervals alike,
/// with the image of a place and its interval in O( 1 ) time
class MoveStructure {

/*--------------------------------------------------------------------------*/
 public:

 MoveStructure( void ) = default;

 /// the permutation of the intervals that start at p (sorted, the first
 /// at 0) onto those that start at q, of [ 0 , n ), each with its tag
 /// \pre p and q of the same size, the images a partition of [ 0 , n )
 MoveStructure( std::span< const std::uint64_t > p ,
                std::span< const std::uint64_t > q ,
                std::span< const std::uint32_t > tag , std::uint64_t n );

 /// the number of the intervals, after the balance
 [[nodiscard]] std::size_t intervals( void ) const { return( f_m ); }

 [[nodiscard]] std::uint64_t size( void ) const { return( f_n ); }

 /// the start of the interval i, n for i = intervals()
 [[nodiscard]] std::uint64_t start( std::size_t i ) const {
  return( i == f_m ? f_n : f_P.get( i * std::size_t( f_wp ) , f_wp ) );
  }

 /// the tag of the interval i
 [[nodiscard]] std::uint32_t tag( std::size_t i ) const {
  return( std::uint32_t( f_tag.get( i * std::size_t( f_wt ) , f_wt ) ) );
  }

 /// the image of x, of the interval i, and its interval
 /// \pre start( i ) <= x < start( i + 1 )
 [[nodiscard]] std::pair< std::uint64_t , std::size_t > move(
                                    std::uint64_t x , std::size_t i ) const;

 /// the interval of x, by a binary search (for the first place of a
 /// walk)
 [[nodiscard]] std::size_t interval( std::uint64_t x ) const;

 /// the bits of the starts, of the images, of their intervals and of the
 /// tags
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::uint64_t f_n = 0;
 std::size_t f_m = 0;
 int f_wp = 1 , f_wd = 1 , f_wt = 1;   ///< a place, an interval, a tag
 BitVector f_P;                         ///< the starts
 BitVector f_Q;                         ///< the images of the starts
 BitVector f_D;                         ///< the intervals of the images
 BitVector f_tag;                       ///< the tags

 };  // end( class MoveStructure )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/MoveStructure.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MoveStructure.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MoveStructure.h ------------------------*/
/*--------------------------------------------------------------------------*/
