/*--------------------------------------------------------------------------*/
/*------------------------ File DirectAccessCodes.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The directly addressable codes of Brisaboa, Ladra and Navarro (DACs): a
 * sequence \f$ x_0 , \ldots , x_{n-1} \f$ of integers of variable length in
 * little more than the bits of their binary codes, with access to any item
 * in time proportional to its length rather than to its place, which the
 * codes of the integers [see IntegerCodes.h] read one after the other do
 * not give.
 *
 * The bits of an integer are cut into chunks, the low \f$ b_0 \f$ bits, the
 * next \f$ b_1 \f$ and so on; the level \f$ \ell \f$ holds, for each integer
 * that has bits beyond the first \f$ s_\ell = b_0 + \cdots + b_{\ell - 1}
 * \f$, its chunk \f$ \ell \f$ in \f$ b_\ell \f$ bits, in an array of fixed
 * width, and a bit telling whether it continues past the level (none on
 * the last level). The integer of place i at a level is at the place
 * \f$ rank_1( i ) \f$ of the next one, among those that continue [see
 * RankSelect.h, with no select], so that
 * \f[ x_i = \sum_{\ell} A_\ell[ i_\ell ] \, 2^{s_\ell} , \quad
 *     i_0 = i , \quad i_{\ell + 1} = rank_1( B_\ell , i_\ell ) , \f]
 * over the levels that it reaches: O( 1 + the levels of \f$ x_i \f$ )
 * time. With \f$ n_k \f$ the integers of more than k bits, the level of
 * the chunk \f$ [ s , s + b ) \f$ costs \f$ n_s ( b + 1 ) \f$ bits (b + 0
 * on the last), plus 25% of the continuation bits for their ranks (one
 * load and one sideways sum), and the widths
 * \f$ b_0 , b_1 , \ldots \f$ that minimize the sum are found by the dynamic
 * program of the authors over the 64 places of a cut,
 * \f[ C( s ) = \min_{s < s' \le 64} \big( n_s ( s' - s ) +
 *     [ s' < w ] \, n_s ( 1 + 0.25 ) + C( s' ) \big) , \quad
 *     C( w ) = 0 , \f]
 * w the bits of the largest integer, in \f$ O( w^2 ) \f$ steps on the counts
 * \f$ n_s \f$; a fixed width b for every level trades bits for fewer
 * levels, i.e., fewer ranks per access. The chunks of one width are a code
 * with a fixed alphabet,
 * the vbyte of the notes when every \f$ b_\ell = 7 \f$, the continuation
 * bits moved to their own vector with rank.
 *
 * The contracts [see Contract.h]: the places asked for are in the
 * sequence; the audit of the construction is that access() gives back each
 * item. The functions are defined in DirectAccessCodes.tpp.
 *
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, DACs: bringing direct access to
 *      variable-length codes, Information Processing and Management 49(1),
 *      392-404, 2013, doi:10.1016/j.ipm.2012.08.003
 * \see G. Navarro, Compact Data Structures, Cambridge University Press,
 *      2016, section 3.4.2, doi:10.1017/CBO9781316588284
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

#ifndef __ADS_DirectAccessCodes
 #define __ADS_DirectAccessCodes
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <array>
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
/*------------------------- CLASS DirectAccessCodes ------------------------*/
/*--------------------------------------------------------------------------*/

/// a static sequence of integers of variable length with direct access
class DirectAccessCodes {

/*--------------------------------------------------------------------------*/
 public:

 DirectAccessCodes( void ) = default;

 /// the sequence x, with the widths of the levels that minimize its bits
 explicit DirectAccessCodes( std::span< const std::uint64_t > x );

 /// the sequence x, with levels of b bits each (the last one up to the
 /// bits of the largest item), which trades bits for fewer levels
 /// \pre 1 <= b <= 64
 DirectAccessCodes( std::span< const std::uint64_t > x , int b );

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the item i
 /// \pre i < size()
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the widths of the chunks of the levels
 [[nodiscard]] std::vector< int > widths( void ) const;

 /// the bits of the chunks, of the continuation bits and of their ranks
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::uint64_t;
 using const_reference = std::uint64_t;
 using const_iterator = SubscriptIterator< DirectAccessCodes ,
                                           std::uint64_t >;
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

 /// a level: its width, its chunks and the bits of the integers that
 /// continue past it, with rank (none on the last level)
 struct Level {
  int width = 0 , shift = 0;
  BitVector chunks;
  RankSelect more;
  };

 /// the levels of the cuts at the places cut[ s ] from s = 0 to w
 void build( std::span< const std::uint64_t > x ,
             const std::array< int , 65 > & cut , int w );

 std::size_t f_n = 0;
 std::vector< Level > f_lev;

 };  // end( class DirectAccessCodes )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/DirectAccessCodes.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DirectAccessCodes.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File DirectAccessCodes.h ----------------------*/
/*--------------------------------------------------------------------------*/
