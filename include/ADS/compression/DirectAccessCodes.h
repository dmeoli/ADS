/*--------------------------------------------------------------------------*/
/*------------------------ File DirectAccessCodes.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The directly addressable codes of Brisaboa, Ladra and Navarro (DACs): a
 * sequence \f$ x_0 , \ldots , x_{n-1} \f$ of integers of variable length in
 * little more than the bits of their binary codes, with access to any item
 * in time proportional to its length rather than to its place, which the
 * codes of the integers [see `IntegerCodes.h`] read one after the other do
 * not give.
 *
 * The bits of an integer are cut into chunks, the low \f$ b_0 \f$ bits, the
 * next \f$ b_1 \f$ and so on; the level \f$ \ell \f$ holds, for each integer
 * that has bits beyond the first
 * \f$ s_\ell = b_0 + \cdots + b_{\ell - 1} \f$, its chunk \f$ \ell \f$ in
 * \f$ b_\ell \f$ bits, in an array \f$ A_\ell \f$ of fixed width, and a bit
 * of \f$ B_\ell \f$ telling whether it continues past the level (none on
 * the last level). The integer of place \f$ i \f$ at a level is at the
 * place \f$ \mathrm{rank}_1( B_\ell , i ) \f$ of the next one, among those
 * that continue [see `RankSelect.h`, with no select], so that
 * \f[
 *   x_i = \sum_{\ell} A_\ell[ i_\ell ] \, 2^{s_\ell} , \quad
 *   i_0 = i , \quad i_{\ell + 1} = \mathrm{rank}_1( B_\ell , i_\ell ) ,
 * \f]
 * over the levels that it reaches: \f$ O( 1 + \text{the levels of } x_i ) \f$
 * time. With \f$ n_k \f$ the integers of more than \f$ k \f$ bits
 * (\f$ n_0 = n \f$), the level of the chunk \f$ [ s , s + b ) \f$ costs
 * \f$ n_s ( b + 1 ) \f$ bits (\f$ n_s b \f$ on the last), plus 25\% of the
 * continuation bits for their ranks (one load and one sideways sum), and
 * the widths \f$ b_0 , b_1 , \ldots \f$ that minimize the sum are found by
 * the dynamic program of the authors over the places of a cut,
 * \f[
 *   C( s ) = \min_{s < s' \le w} \big( n_s ( s' - s ) +
 *   [ s' < w ] \, n_s ( 1 + 0.25 ) + C( s' ) \big) , \quad C( w ) = 0 ,
 * \f]
 * \f$ w \f$ the bits of the largest integer, in \f$ O( w^2 ) \f$ steps on
 * the counts \f$ n_s \f$; a fixed width \f$ b \f$ for every level trades
 * bits for fewer levels, i.e., fewer ranks per access; a chunk of 8 or 16
 * bits is read as its byte or its pair of bytes, one load that stays in its
 * line of the cache. The construction packs the chunks and the continuation
 * bits of a level in order, a field of 64 bits at a time. The chunks of one
 * width are a code with a fixed alphabet, the variable-byte code of chapter
 * 11, Integer Coding, of Ferragina's book when every \f$ b_\ell = 7 \f$, the
 * continuation bits moved to their own vector with rank.
 *
 * The contracts [see `Contract.h`]: the places asked for are in the
 * sequence; the audit of the construction is that `access()` gives back
 * each item. The functions are defined in `DirectAccessCodes.tpp`.
 *
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, DACs: bringing direct access to
 *      variable-length codes, Information Processing and Management 49(1),
 *      392-404, 2013, doi:10.1016/j.ipm.2012.08.003
 * \see G. Navarro, Compact Data Structures, Cambridge University Press,
 *      2016, section 3.4.2, doi:10.1017/CBO9781316588284
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
#include <cstring>
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

 /// the empty sequence
 DirectAccessCodes( void ) = default;

 /// the sequence \f$ x \f$, with the widths of the levels that minimize its
 /// bits
 ///
 /// In pseudocode (the dynamic program of Brisaboa, Ladra and Navarro,
 /// beyond the book), \f$ n_s \f$ the integers of more than \f$ s \f$ bits
 /// (\f$ n_0 = n \f$) and \f$ w \f$ the bits of the largest one:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{DirectAccessCodes}( x ) \cr
 /// 2 & \quad C[ w ] \gets 0 \cr
 /// 3 & \quad \textbf{for}\ s \gets w - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \cr
 /// 4 & \qquad \mathit{cut}[ s ] \gets \text{the } t \in ( s , w ] \text{ of}
 ///     \text{ least } n_s ( t - s ) + [ t < w ] \, 1.25 \, n_s + C[ t ] \cr
 /// 5 & \qquad C[ s ] \gets \text{that least cost} \cr
 /// 6 & \quad \mathrm{build}( x , \mathit{cut} ) \cr
 /// 7 & \textbf{procedure}\ \mathrm{build}( x , \mathit{cut} ) \cr
 /// 8 & \quad s \gets 0;\ y \gets x \cr
 /// 9 & \quad \textbf{while}\ s < w\ \textbf{do} \cr
 /// 10 & \qquad b \gets \mathit{cut}[ s ] - s;\ A \gets \text{the bits } [ s
 ///     , s + b ) \text{ of each item of } y \cr
 /// 11 & \qquad \textbf{if}\ \mathit{cut}[ s ] < w\ \textbf{then}\ B \gets (
 ///     [ y_j \ge 2^{s + b} ] )_j \text{ with rank};\ y \gets \text{the } y_j
 ///     \text{ with } y_j \ge 2^{s + b} \cr
 /// 12 & \qquad \text{add the level } ( A , B );\ s \gets \mathit{cut}[ s ]
 /// \end{array}}
 /// \f]
 /// \post `access()` gives each item of \f$ x \f$ back (audit)
 explicit DirectAccessCodes( std::span< const std::uint64_t > x );

 /// the sequence \f$ x \f$, with levels of \f$ b \f$ bits each (the last
 /// one up to the bits of the largest item), which trades bits for fewer
 /// levels: the levels of `build()` above with
 /// \f$ \mathit{cut}[ s ] = \min( w , s + b ) \f$
 /// \pre \f$ 1 \le b \le 64 \f$
 /// \post `access()` gives each item of \f$ x \f$ back (audit)
 DirectAccessCodes( std::span< const std::uint64_t > x , int b );

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the item \f$ x_i \f$
 ///
 /// In pseudocode (Brisaboa, Ladra and Navarro, beyond the book), the
 /// levels \f$ ( A_\ell , B_\ell ) \f$ from the first:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad v \gets 0;\ \ell \gets 0 \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad v \gets v + A_\ell[ i ] \cdot 2^{s_\ell} \cr
 /// 5 & \qquad \textbf{if}\ \ell \text{ is the last level}\ \textbf{or}
 ///     \ B_\ell[ i ] = 0\ \textbf{then return}\ v \cr
 /// 6 & \qquad i \gets \mathrm{rank}_1( B_\ell , i );\ \ell \gets \ell + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::uint64_t access( std::size_t i ) const;

 /// the item \f$ x_i \f$, by `access()`
 [[nodiscard]] std::uint64_t operator[]( std::size_t i ) const {
  return( access( i ) );
  }

 /// the widths of the chunks of the levels
 [[nodiscard]] std::vector< int > widths( void ) const;

 /// the bits of the chunks, of the continuation bits and of their ranks
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 using value_type = std::uint64_t;        ///< the type of the items
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the type of the distances
 using reference = std::uint64_t;         ///< an item, by value
 using const_reference = std::uint64_t;   ///< an item, by value
 /// the iterator, by `access()`
 using const_iterator = SubscriptIterator< DirectAccessCodes ,
                                           std::uint64_t >;
 using iterator = const_iterator;         ///< the same as `const_iterator`

 /// the iterator at the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }

 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }

 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// a level: its width, its chunks and the bits of the integers that
 /// continue past it, with rank (none on the last level)
 struct Level {
  int width = 0 ,   ///< the bits of a chunk of the level
      shift = 0;    ///< the bits of the integers on the levels before it
  BitVector chunks;   ///< the chunks, one per integer that reaches the level
  /// a 1 for each integer that continues past the level, with rank
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
