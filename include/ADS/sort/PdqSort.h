/*--------------------------------------------------------------------------*/
/*----------------------------- File PdqSort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pattern-defeating quicksort, beyond the book and beyond the
 * quicksorts of Ferragina's book, which are in `EngineeredQuickSorts.h`
 * [see `EngineeredQuickSorts.h`, which includes this header]:
 *
 * - `pdqSort()`, the pattern-defeating quicksort of Peters: the introsort of
 *   Musser in the form of Peters, i.e., the quicksort that allows
 *   \f$ \lfloor \log_2 n \rfloor + 1 \f$ partitions too unbalanced (see
 *   below), past which the part is sorted by heapsort, so that the worst
 *   case is \f$ O( n \log n ) \f$, with the
 *   pivot by the median of three, or by the ninther of Tukey on large parts,
 *   and three defences against the patterns: a partition that moved nothing
 *   is checked by an insertion sort that gives up after 8 moves, so that a
 *   sorted or reversed run takes \f$ O( n ) \f$; a pivot equal to the item
 *   before the part, i.e., a run of equal keys, gets a partition that puts
 *   the equals on its left and skips them, so that \f$ k \f$ distinct keys
 *   take \f$ O( n \log k ) \f$; a partition too unbalanced (one side below
 *   \f$ n / 8 \f$) swaps a few items to break the pattern, and too many of
 *   them fall back to heapsort. On the arithmetic types with their natural
 *   order the partition goes by blocks, as in the BlockQuicksort of Edelkamp
 *   and Weiss: the items to swap are first collected, 64 at a time, as offsets
 *   by an increment with no branch, and then moved along a cycle, or swapped
 *   in pairs when the two blocks have as many (the first with the last, so
 *   that a part in reverse order leaves both its sides sorted, which the
 *   insertion sort above then finds in \f$ O( n ) \f$), so that no
 *   comparison causes a mispredicted branch.
 *
 * It sorts in place and is not stable; the recursion goes into the left
 * part and loops on the right one, the other partitions leaving at most
 * \f$ 7 / 8 \f$ of the items on each side, so that the stack stays
 * \f$ O( \log n ) \f$.
 * Besides its own use, `pdqSort()` is the engine of `sort()` [see `Sort.h`]
 * for the short vectors, for few distinct keys and for the items that no
 * faster route takes, and it stays so also where Boost is found:
 * `boost::sort::pdqsort` is no slower on the benchmark of the sorts, and a
 * program that wants it calls it directly.
 *
 * The contracts [see `Contract.h`]: the sort ends with the same items
 * sorted, an audit. The functions are defined in `PdqSort.tpp`.
 *
 * \see D. R. Musser, Introspective sorting and selection algorithms,
 *      Software: Practice and Experience 27(8), 983-993, 1997,
 *      doi:10.1002/(SICI)1097-024X(199708)27:8\<983::AID-SPE117\>3.0.CO;2-\#
 * \see O. R. L. Peters, Pattern-defeating quicksort, arXiv:2106.05123,
 *      2021, doi:10.48550/arXiv.2106.05123
 * \see J. W. Tukey, The ninther, a technique for low-effort robust
 *      (resistant) location in large samples, in H. A. David (ed.),
 *      Contributions to Survey Sampling and Applied Statistics, Academic
 *      Press, 251-257, 1978, doi:10.1016/B978-0-12-204750-3.50024-1
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: avoiding branch
 *      mispredictions in quicksort, Proceedings of ESA 2016, LIPIcs 57,
 *      article 38, 2016, doi:10.4230/LIPIcs.ESA.2016.38
 * \see S. Edelkamp, A. Weiss, BlockQuicksort: avoiding branch
 *      mispredictions in quicksort, ACM Journal of Experimental
 *      Algorithmics 24, article 1.4, 2019, doi:10.1145/3274660
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

#ifndef __ADS_PdqSort
 #define __ADS_PdqSort
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/EngineeredQuickSorts.h"
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

/// the pattern-defeating quicksort
///
/// In pseudocode (Peters, beyond the book), \f$ \mathit{bad} \f$ starting from
/// \f$ \lfloor \log_2 n \rfloor + 1 \f$ and the part leftmost when no item
/// precedes it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{pdq}( A , \mathit{lo} , \mathit{hi} ,
///     \mathit{bad} ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \textbf{if}\ \mathit{hi} - \mathit{lo} < 24\ \textbf{then}
///     \ \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} );
///     \ \textbf{return} \cr
/// 4 & \qquad \text{the median of three, or the ninther, to } A[ \mathit{lo} ]
///     \cr
/// 5 & \qquad \textbf{if}\ \text{not leftmost and } A[ \mathit{lo} - 1 ] \ge
///     A[ \mathit{lo} ]\ \textbf{then} \cr
/// 6 & \qquad\quad \mathit{lo} \gets 1 + \text{the end of the equals of the }
///     \text{pivot, partitioned left};\ \textbf{continue} \cr
/// 7 & \qquad p \gets \text{the place of the pivot, the equals on the right}
///     \cr
/// 8 & \qquad \textbf{if}\ \text{a side is smaller than } ( \mathit{hi} -
///     \mathit{lo} ) / 8\ \textbf{then} \cr
/// 9 & \qquad\quad \mathit{bad} \gets \mathit{bad} - 1 \cr
/// 10 & \qquad\quad \textbf{if}\ \mathit{bad} = 0\ \textbf{then}
///     \ \text{heapsort } A[ \mathit{lo} .. \mathit{hi} - 1
///     ];\ \textbf{return}
///     \cr
/// 11 & \qquad\quad \text{swap a few items of each side} \cr
/// 12 & \qquad \textbf{else if}\ \text{nothing moved and both sides sort by }
///     \text{insertion in 8 moves}\ \textbf{then return} \cr
/// 13 & \qquad \mathrm{pdq}( A , \mathit{lo} , p , \mathit{bad} );
///     \ \mathit{lo} \gets p + 1
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C = std::less<> >
void pdqSort( Store && A , C less = C() );

/// `pdqSort()` above on a vector
template< class T , class C = std::less<> >
void pdqSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/
/*------------------------- THE SORTS OF A RANGE ---------------------------*/
/*--------------------------------------------------------------------------*/

/// `pdqSort()` on the range \f$ A \f$ [see `SortRange`], the other arguments
/// as for a vector: in place on the span of its items if \f$ A \f$ is
/// contiguous, through a vector of its items otherwise, since the code of the
/// sort reads the items by their addresses
template< SortRange R , class... P >
 requires requires( std::vector< std::ranges::range_value_t< R > > & v ,
		    P &&... p )
{ pdqSort( v , std::forward< P >( p )... ); }
decltype( auto ) pdqSort( R && A , P &&... p )
{
 const auto f = [ & ]( auto & v ) -> decltype( auto ) {
  return( pdqSort( v , std::forward< P >( p )... ) );
  };
 return( detail::sortRange( A , f ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/PdqSort.tpp"

/*--------------------------------------------------------------------------*/

#endif /* PdqSort.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File PdqSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
