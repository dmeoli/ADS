/*--------------------------------------------------------------------------*/
/*-------------------------- File MoveStructure.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The move structure of Nishimoto and Tabei: a permutation \f$ \pi \f$ of
 * \f$ [ 0 , n ) \f$ that shifts each of \f$ m \f$ intervals alike, the
 * interval \f$ [ p_i , p_{i+1} ) \f$ onto
 * \f$ [ q_i , q_i + p_{i+1} - p_i ) \f$ (the LF-mapping of a transform of
 * Burrows and Wheeler on its runs, the function \f$ \phi \f$ of the r-index
 * on the places of the starts of the runs [see `RIndex.h`]), with
 * \f$ \pi( x ) \f$ and the interval of \f$ \pi( x ) \f$ in \f$ O( 1 ) \f$
 * time from \f$ x \f$ and its interval, in \f$ O( m ) \f$ words rather than
 * \f$ n \f$.
 *
 * Each interval \f$ i \f$ keeps \f$ p_i \f$, \f$ q_i \f$ and the interval
 * \f$ d_i \f$ that holds \f$ q_i \f$, so that
 * \f[ \pi( x ) = q_i + ( x - p_i ) , \qquad x \in [ p_i , p_{i+1} ) , \f]
 * and its interval is found from \f$ d_i \f$ forward, past the starts
 * \f$ p_j \le \pi( x ) \f$. The structure is balanced: no image
 * \f$ [ q_i , q_i + p_{i+1} - p_i ) \f$ holds 4 starts or more, which the
 * construction ensures by cutting an interval whose image holds 4 starts
 * at the third of them, so that the first part holds 2 (the new start may
 * weigh on another image, which is checked in turn); the intervals become
 * at most \f$ 2 m \f$ (Nishimoto and Tabei, for this balance), and the
 * search forward takes at most 3 steps. The construction keeps the
 * intervals and the images in two ordered maps, \f$ O( m \log m ) \f$ time.
 * Each interval carries a tag of its own (the symbol of its run), which its
 * parts keep.
 *
 * The contracts [see `Contract.h`]: the intervals partition
 * \f$ [ 0 , n ) \f$ and their images too, and the places and the intervals
 * of `move()` agree; the audit of the construction is the balance. The
 * functions are defined in `MoveStructure.tpp`.
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
 *         Università di Pisa \n
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

/// a permutation of \f$ [ 0 , n ) \f$ that shifts each of its intervals
/// alike, with the image of a place and its interval in \f$ O( 1 ) \f$ time
class MoveStructure {

/*--------------------------------------------------------------------------*/
 public:

 /// the permutation of no place
 MoveStructure( void ) = default;

 /// the permutation of the intervals that start at \f$ p \f$ (sorted, the
 /// first at 0) onto those that start at \f$ q \f$, of \f$ [ 0 , n ) \f$,
 /// each with its tag
 ///
 /// In pseudocode (the balance of Nishimoto and Tabei, beyond the book),
 /// \f$ I \f$ the intervals by start, each with its image, and \f$ P \f$
 /// the starts:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{balance}( I ) \cr
 /// 2 & \quad S \gets \text{the images of } I \cr
 /// 3 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad [ q , q + \lambda ) \gets \mathrm{pop}( S ) \text{, the image }
 ///     \text{of } [ p , p + \lambda ) \cr
 /// 5 & \qquad \textbf{if}\ [ q , q + \lambda ) \text{ holds 4 starts or }
 ///     \text{more of } P\ \textbf{then} \cr
 /// 6 & \qquad\quad t \gets \text{the third of them} - q \cr
 /// 7 & \qquad\quad \text{cut } [ p , p + \lambda ) \text{ into } [ p , p + t
 ///     ) \text{ and } [ p + t , p + \lambda ) \text{, the tag kept} \cr
 /// 8 & \qquad\quad \mathrm{push}( S , [ q , q + t ) );\ \mathrm{push}( S , [
 ///     q + t , q + \lambda ) ) \cr
 /// 9 & \qquad\quad \mathrm{push}( S , \text{the image that holds the new }
 ///     \text{start } p + t )
 /// \end{array}}
 /// \f]
 /// \pre \f$ p \f$, \f$ q \f$ and the tags of the same size, the images a
 ///      partition of \f$ [ 0 , n ) \f$
 MoveStructure( std::span< const std::uint64_t > p ,
                std::span< const std::uint64_t > q ,
                std::span< const std::uint32_t > tag , std::uint64_t n );

 /// the number of the intervals, after the balance
 [[nodiscard]] std::size_t intervals( void ) const { return( f_m ); }

 /// the number of places, \f$ n \f$
 [[nodiscard]] std::uint64_t size( void ) const { return( f_n ); }

 /// the start of the interval \f$ i \f$, \f$ n \f$ for \f$ i \f$ equal to
 /// `intervals()`
 [[nodiscard]] std::uint64_t start( std::size_t i ) const {
  return( i == f_m ? f_n : f_P.get( i * std::size_t( f_wp ) , f_wp ) );
  }

 /// the tag of the interval \f$ i \f$
 [[nodiscard]] std::uint32_t tag( std::size_t i ) const {
  return( std::uint32_t( f_tag.get( i * std::size_t( f_wt ) , f_wt ) ) );
  }

 /// the image of \f$ x \f$, of the interval \f$ i \f$, and its interval
 ///
 /// In pseudocode (Nishimoto and Tabei, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{move}( x , i ) \cr
 /// 2 & \quad y \gets q_i + ( x - p_i );\ j \gets d_i \cr
 /// 3 & \quad \textbf{while}\ p_{j+1} \le y\ \textbf{do}\ j \gets j + 1\quad
 ///     \triangleright\ \text{at most 3 steps, by the balance} \cr
 /// 4 & \quad \textbf{return}\ ( y , j )
 /// \end{array}}
 /// \f]
 /// \pre `start( i )` \f$ \le x < \f$ `start( i + 1 )`
 [[nodiscard]] std::pair< std::uint64_t , std::size_t > move(
                                    std::uint64_t x , std::size_t i ) const;

 /// the interval of \f$ x \f$, by a binary search on the starts (for the
 /// first place of a walk)
 /// \pre \f$ x < n \f$
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
