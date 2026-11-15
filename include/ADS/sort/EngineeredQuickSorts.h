/*--------------------------------------------------------------------------*/
/*---------------------- File EngineeredQuickSorts.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksorts beyond the quicksort of Montresor's book [see `QuickSort.h`
 * for it and its improvements of section 12.2]: the two of chapter 5,
 * Sorting Atomic Items, of Ferragina's book, and the one engineered for
 * speed, `pdqSort()`, which is in `PdqSort.h` [see `PdqSort.h`, which this
 * header includes]:
 *
 * - `dualPivotQuickSort()` of Yaroslavskiy (the sort of the primitive types of
 *   Java 7), as section 5.3.5 of Ferragina's book presents it: two pivots
 *   \f$ p \le q \f$, the 2nd and the 4th of five equally spaced items, split
 *   the part into three, smaller than \f$ p \f$, between and larger than
 *   \f$ q \f$, in one pass; with two random items as pivots it makes 5%
 *   fewer comparisons and 80% more swaps than the classical one on average
 *   (\f$ 1.9 n \ln n \f$ and \f$ 0.6 n \ln n \f$, against
 *   \f$ 2 n \ln n \f$ and \f$ n \ln n / 3 \f$), as Wild and Nebel
 *   computed, and fewer scans of the memory;
 * - `threeWayQuickSort()`, the quicksort of section 5.3 of Ferragina's book
 *   with the three-way partition of its Algorithm 5.4 (section 5.3.1): three
 *   pointers move rightward, and the items equal to the pivot gather in a
 *   middle part that no later call sorts again, so that \f$ k \f$ distinct
 *   keys cost \f$ O( n \log k ) \f$ on average; the pivot is drawn at random,
 *   as section 5.3.2 suggests, by a generator of fixed seed (the same sort on
 *   every run), and the recursion is bounded as in its Algorithm 5.6. It
 *   makes \f$ 2 n \ln n \f$ comparisons on average (Theorem 5.5 of the
 *   book), at most two per item and partition, but \f$ \Theta( n^2 ) \f$ in
 *   the worst case, when the pivots drawn are the smallest or the largest
 *   items of their parts, which happens with a probability that vanishes with
 *   \f$ n \f$ on every input, there being no fallback to heapsort.
 *
 * Both sort in place and are not stable; the recursion goes into the
 * smaller part and loops on the larger, so that the stack stays
 * \f$ O( \log n ) \f$.
 *
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit. The functions are defined in `EngineeredQuickSorts.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5, Sorting Atomic Items,
 *      doi:10.1017/9781009128933
 * \see V. Yaroslavskiy, Dual-pivot quicksort, 2009
 * \see S. Wild, M. E. Nebel, Average case analysis of Java 7's dual pivot
 *      quicksort, Proceedings of ESA 2012, LNCS 7501, 825-836, 2012,
 *      doi:10.1007/978-3-642-33090-2_71
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

#ifndef __ADS_EngineeredQuickSorts
 #define __ADS_EngineeredQuickSorts
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the dual-pivot quicksort
///
/// In pseudocode (section 5.3.5 of Ferragina's book, Yaroslavskiy, with the
/// pivots of Java 7),
/// \f$ A[ \mathit{lo} + 1 .. l - 1 ] < p \f$,
/// \f$ A[ g + 1 .. \mathit{hi} - 2 ] > q \f$ and the items between in
/// \f$ [ p , q ] \f$ at all times:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dual}( A , \mathit{lo} , \mathit{hi} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 27\ \textbf{then}
///     \ \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 3 & \quad p , q \gets \text{the 2nd and 4th of five spaced items, to } A[
///     \mathit{lo} ] \text{ and } A[ \mathit{hi} - 1 ] \cr
/// 4 & \quad l \gets \mathit{lo} + 1;\ g \gets \mathit{hi} - 2;\ k \gets l \cr
/// 5 & \quad \textbf{while}\ k \le g\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ A[ k ] < p\ \textbf{then}\ A[ k ] \leftrightarrow
///     A[ l ];\ l \gets l + 1 \cr
/// 7 & \qquad \textbf{else if}\ A[ k ] \ge q\ \textbf{then} \cr
/// 8 & \qquad\quad \textbf{while}\ A[ g ] > q\ \textbf{and}\ k < g
///     \ \textbf{do}\ g \gets g - 1 \cr
/// 9 & \qquad\quad A[ k ] \leftrightarrow A[ g ];\ g \gets g - 1 \cr
/// 10 & \qquad\quad \textbf{if}\ A[ k ] < p\ \textbf{then}\ A[ k ]
///     \leftrightarrow A[ l ];\ l \gets l + 1 \cr
/// 11 & \qquad k \gets k + 1 \cr
/// 12 & \quad \text{put } p \text{ in } A[ l - 1 ] \text{ and } q \text{ in }
///     \text{} A[ g + 1 ] \cr
/// 13 & \quad \text{sort the three parts, the middle one only if } p < q
/// \end{array}}
/// \f]
/// where the two smaller parts are sorted recursively and the largest by the
/// loop.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C = std::less<> >
void dualPivotQuickSort( Store && A , C less = C() );

/// `dualPivotQuickSort()` above on a vector
template< class T , class C = std::less<> >
void dualPivotQuickSort( std::vector< T > & A , C less = C() );

/// the quicksort with the three-way partition of Ferragina's book
///
/// In pseudocode (Algorithms 5.4 and 5.6 of chapter 5, Sorting Atomic Items,
/// of Ferragina's book, with \f$ n_0 = 16 \f$), the pivot \f$ P \f$ in
/// \f$ A[ \mathit{lo} ] \f$ and, while \f$ c \f$ scans,
/// \f$ A[ \mathit{lo} .. l - 1 ] < P \f$, \f$ A[ l .. r - 1 ] = P \f$ and
/// \f$ A[ r .. c - 1 ] > P \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{partition}( A , \mathit{lo} , \mathit{hi} )
///     \cr
/// 2 & \quad P \gets A[ \mathit{lo} ];\ l \gets \mathit{lo};\ r \gets
///     \mathit{lo} + 1 \cr
/// 3 & \quad \textbf{for}\ c \gets r\ \textbf{to}\ \mathit{hi} - 1
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ c ] = P\ \textbf{then}\ A[ c ] \leftrightarrow
///     A[ r ];\ r \gets r + 1 \cr
/// 5 & \qquad \textbf{else if}\ A[ c ] < P\ \textbf{then}\ A[ c ]
///     \leftrightarrow A[ l ];\ A[ c ] \leftrightarrow A[ r ];\ r \gets r +
///     1;\ l \gets l + 1 \cr
/// 6 & \quad \textbf{return}\ ( l , r ) \cr
/// 7 & \textbf{procedure}\ \mathrm{threeWay}( A , \mathit{lo} , \mathit{hi} )
///     \cr
/// 8 & \quad \textbf{while}\ \mathit{hi} - \mathit{lo} > n_0\ \textbf{do} \cr
/// 9 & \qquad A[ \mathit{lo} ] \leftrightarrow A[ x ] \text{ for } x \text{ }
///     \text{drawn at random in } [ \mathit{lo} , \mathit{hi} ) \cr
/// 10 & \qquad ( l , r ) \gets \mathrm{partition}( A , \mathit{lo} ,
///     \mathit{hi} ) \cr
/// 11 & \qquad \textbf{if}\ l - \mathit{lo} \le \mathit{hi} - r\ \textbf{then}
///     \ \mathrm{threeWay}( A , \mathit{lo} , l );\ \mathit{lo} \gets r \cr
/// 12 & \qquad \textbf{else}\ \mathrm{threeWay}( A , r , \mathit{hi} );
///     \ \mathit{hi} \gets l \cr
/// 13 & \quad \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} )
/// \end{array}}
/// \f]
/// where \f$ A[ c ] = P \f$ means that neither is smaller than the other;
/// \f$ \Theta( n^2 ) \f$ comparisons in the worst case.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C = std::less<> >
void threeWayQuickSort( Store && A , C less = C() );

/// `threeWayQuickSort()` above on a vector
template< class T , class C = std::less<> >
void threeWayQuickSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/
/*------------------------- THE SORTS OF A RANGE ---------------------------*/
/*--------------------------------------------------------------------------*/

/// `dualPivotQuickSort()` on the range \f$ A \f$ [see `SortRange`], the other
/// arguments as for a vector: in place on the span of its items if \f$ A \f$
/// is contiguous, through a vector of its items otherwise, since the code of
/// the sort reads the items by their addresses
template< SortRange R , class... P >
 requires requires( std::vector< std::ranges::range_value_t< R > > & v ,
		    P &&... p )
{
  dualPivotQuickSort( v , std::forward< P >( p )... );
  }
decltype( auto ) dualPivotQuickSort( R && A , P &&... p )
{
 const auto f = [ & ]( auto & v ) -> decltype( auto ) {
  return( dualPivotQuickSort( v , std::forward< P >( p )... ) );
  };
 return( detail::sortRange( A , f ) );
 }

/// `threeWayQuickSort()` on the range \f$ A \f$ [see `SortRange`], the other
/// arguments as for a vector: in place on the span of its items if \f$ A \f$
/// is contiguous, through a vector of its items otherwise, since the code of
/// the sort reads the items by their addresses
template< SortRange R , class... P >
 requires requires( std::vector< std::ranges::range_value_t< R > > & v ,
		    P &&... p )
{
  threeWayQuickSort( v , std::forward< P >( p )... );
  }
decltype( auto ) threeWayQuickSort( R && A , P &&... p )
{
 const auto f = [ & ]( auto & v ) -> decltype( auto ) {
  return( threeWayQuickSort( v , std::forward< P >( p )... ) );
  };
 return( detail::sortRange( A , f ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/EngineeredQuickSorts.tpp"

/*--------------------------------------------------------------------------*/
/*-------------------- THE PATTERN-DEFEATING QUICKSORT ---------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/PdqSort.h"

/*--------------------------------------------------------------------------*/

#endif /* EngineeredQuickSorts.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File EngineeredQuickSorts.h ---------------------*/
/*--------------------------------------------------------------------------*/
