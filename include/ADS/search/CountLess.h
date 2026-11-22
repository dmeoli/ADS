/*--------------------------------------------------------------------------*/
/*--------------------------- File CountLess.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The count of the items of a short array smaller than a key, the search in
 * a node of the B-trees in main memory [see `BTreeMap.h` and
 * `StaticSearch.h`]: on a sorted array of \f$ N \f$ items it is the place of
 * the key, found with no branch. `countLess< N >( a , x )` and
 * `countLess< N , true >( a , x )`, the items not larger than \f$ x \f$, read
 * all the \f$ N \f$ items, which must be there (the unused places of a node
 * are padded with the largest value). `greaterBits< N >( a , x )` gives the
 * items larger than \f$ x \f$ as the bits of a word, \f$ N \le 64 \f$ [see
 * `WordRangeMinimum.h`].
 *
 * On the integers of 32 and 64 bits and on the floating types, with
 * `ADS_WITH_HIGHWAY`, they are a few comparisons of the vectors of Highway
 * inlined at the instructions of the compiler [see `SimdInline.h`], each
 * giving a mask whose lanes a count of the lanes sums (16 items of 32 bits
 * at once on AVX-512, 8 on AVX2, 4 on SSE2 and NEON); without it, and on
 * the other types, the loop of the comparisons, which the compiler may
 * vectorize or not (GCC 13 does not: it chains the carries of the
 * comparisons, one item per cycle and a dependence from each to the
 * next). An item that is a NaN counts as in the loop, as not smaller.
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

#ifndef __ADS_CountLess
 #define __ADS_CountLess
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "ADS/SimdInline.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail
{

/// the number of the items of \f$ a[ 0 .. N ) \f$ smaller than \f$ x \f$
/// (not larger, if `Up`)
///
/// In pseudocode (the branch-free search in a node, beyond the book), the
/// comparisons made \f$ W \f$ at a time where the vectors allow it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countLess}( a , x ) \cr
/// 2 & \quad r \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ N - 1\ \textbf{do}\ r
///     \gets r + [ a[ i ] < x ]\quad \triangleright\ [ a[ i ] \le x ]
///     \text{ if } \mathit{Up} \cr
/// 4 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
template< std::size_t N , bool Up = false , class T >
[[nodiscard]] inline std::size_t countLess( const T * a , const T & x )
{
#if ADS_SIMD
 if constexpr( simd::here::kLanesOf< T > )
  return( simd::here::countLess< N , Up >( a , x ) );
#endif
 std::size_t r = 0;
 for( std::size_t i = 0 ; i < N ; ++i )
  if constexpr( Up )
   r += ! ( x < a[ i ] );
  else
   r += a[ i ] < x;
 return( r );
 }

/// the bits of the items of \f$ a[ 0 .. N ) \f$ larger than \f$ x \f$,
/// \f$ N \le 64 \f$: the bit \f$ q \f$ is set if \f$ a[ q ] > x \f$
///
/// In pseudocode (beyond the book), the comparisons made \f$ W \f$ at a time
/// where the vectors allow it, as in `countLess()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{greaterBits}( a , x ) \cr
/// 2 & \quad r \gets 0 \cr
/// 3 & \quad \textbf{for}\ q \gets 0\ \textbf{to}\ N - 1\ \textbf{do}
///     \ \textbf{if}\ a[ q ] > x\ \textbf{then}\ r \gets r + 2^q \cr
/// 4 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
template< std::size_t N , class T >
[[nodiscard]] inline std::uint64_t greaterBits( const T * a , const T & x )
{
 static_assert( N <= 64 , "the bits of a word" );
#if ADS_SIMD
 if constexpr( simd::here::kLanesOf< T > )
  return( simd::here::greaterBits< N >( a , x ) );
#endif
 std::uint64_t r = 0;
 for( std::size_t i = 0 ; i < N ; ++i )
  r |= std::uint64_t( x < a[ i ] ) << i;
 return( r );
 }

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif /* CountLess.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CountLess.h --------------------------*/
/*--------------------------------------------------------------------------*/
