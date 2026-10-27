/*--------------------------------------------------------------------------*/
/*-------------------------- File ExchangeSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts by exchanges of items, beyond the book, all in place:
 *
 * - `bubbleSort()`, passes from left to right that swap the adjacent items out
 *   of order, each ending at the last swap of the previous one, since the
 *   items after it are already in their places: \f$ O( n^2 ) \f$ comparisons,
 *   \f$ n - 1 \f$ on a sorted vector, stable;
 * - `cocktailShakerSort()`, the bubble sort with passes alternately to the
 *   right and to the left, so that a small item near the end (a turtle) moves
 *   to the front in one pass instead of \f$ n \f$, stable;
 * - `oddEvenSort()`, the odd-even transposition sort: the pairs
 *   \f$ ( 2 i , 2 i + 1 ) \f$ and then the pairs \f$ ( 2 i + 1 , 2 i + 2 ) \f$
 *   compared and swapped in turns, \f$ n \f$ rounds at most, each of
 *   independent comparisons, which is why it is a sort for \f$ n \f$
 *   processors on a line, stable;
 * - `combSort()`, the bubble sort with a gap that shrinks by 1.3 at each pass,
 *   which moves the turtles far, ending as a bubble sort with gap 1: not
 *   stable, near \f$ O( n \log n ) \f$ in practice, though \f$ O( n^2 ) \f$ in
 *   the worst case;
 * - `gnomeSort()`, one position that moves right while the two items behind it
 *   are in order and swaps them and moves left otherwise: the insertion sort
 *   with swaps and no nested loop, \f$ O( n^2 ) \f$, stable.
 *
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit. The functions are defined in `ExchangeSorts.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.2 (sorting by exchanging)
 * \see A. N. Habermann, Parallel neighbor-sort (or the glory of the
 *      induction principle), Technical Report AD-759 248, Carnegie Mellon
 *      University, 1972
 * \see W. Dobosiewicz, An efficient variation of bubble sort, Information
 *      Processing Letters 11(1), 5-6, 1980,
 *      doi:10.1016/0020-0190(80)90022-8
 * \see H. Sarbazi-Azad, Stupid sort: a new sorting algorithm, Newsletter
 *      599, Department of Computing Science, University of Glasgow, 2000
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

#ifndef __ADS_ExchangeSorts
 #define __ADS_ExchangeSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the bubble sort, each pass ending at the last swap of the previous one
///
/// In pseudocode (Knuth, section 5.2.2, beyond the book), the items from the
/// last swap on being in their final places:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bubbleSort}( A , n ) \cr
/// 2 & \quad e \gets n \cr
/// 3 & \quad \textbf{while}\ e > 1\ \textbf{do} \cr
/// 4 & \qquad \mathit{last} \gets 0 \cr
/// 5 & \qquad \textbf{for}\ i \gets 1\ \textbf{to}\ e - 1\ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ A[ i ] < A[ i - 1 ]\ \textbf{then}\ A[ i ]
///     \leftrightarrow A[ i - 1 ];\ \mathit{last} \gets i \cr
/// 7 & \qquad e \gets \mathit{last}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void bubbleSort( std::vector< T > & A , C less = C() );

/// the bubble sort with passes in both directions
///
/// In pseudocode (Knuth, section 5.2.2, beyond the book),
/// \f$ A[ \mathit{lo} .. \mathit{hi} - 1 ] \f$ the part still to sort:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{cocktailShakerSort}( A , n ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n \cr
/// 3 & \quad \textbf{while}\ \mathit{hi} - \mathit{lo} > 1\ \textbf{do} \cr
/// 4 & \qquad \text{a pass to the right on } A[ \mathit{lo} .. \mathit{hi} - 1
///     ];\ \mathit{hi} \gets \text{its last swap} \cr
/// 5 & \qquad \textbf{if}\ \mathit{hi} - \mathit{lo} \le 1\ \textbf{then}
///     \ \textbf{break} \cr
/// 6 & \qquad \text{a pass to the left on } A[ \mathit{lo} .. \mathit{hi} - 1
///     ];\ \mathit{lo} \gets \text{its last swap}
/// \end{array}}
/// \f]
/// where a pass with no swap sets the bound to the other one.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void cocktailShakerSort( std::vector< T > & A , C less = C() );

/// the odd-even transposition sort; the rounds go to `*rounds`, if given
///
/// In pseudocode (Habermann, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{oddEvenSort}( A , n ) \cr
/// 2 & \quad \textbf{repeat} \cr
/// 3 & \qquad \mathit{sorted} \gets \textbf{true} \cr
/// 4 & \qquad \textbf{foreach}\ s \in ( 0 , 1 )\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{for}\ i \gets s , s + 2 , \ldots \text{ while } i +
///     1 < n\ \textbf{do} \cr
/// 6 & \qquad\qquad \textbf{if}\ A[ i + 1 ] < A[ i ]\ \textbf{then}\ A[ i ]
///     \leftrightarrow A[ i + 1 ];\ \mathit{sorted} \gets \textbf{false} \cr
/// 7 & \quad \textbf{until}\ \mathit{sorted}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void oddEvenSort( std::vector< T > & A , C less = C() ,
                  std::size_t * rounds = nullptr );

/// the comb sort, the gap shrinking by 1.3
///
/// In pseudocode (Dobosiewicz, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{combSort}( A , n ) \cr
/// 2 & \quad g \gets n;\ \mathit{swapped} \gets \textbf{true} \cr
/// 3 & \quad \textbf{while}\ g > 1\ \textbf{or}\ \mathit{swapped}\ \textbf{do}
///     \cr
/// 4 & \qquad g \gets \max\{ 1 , \lfloor 10 g / 13 \rfloor \};
///     \ \mathit{swapped} \gets \textbf{false} \cr
/// 5 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - g - 1\ \textbf{do} \cr
/// 6 & \qquad\quad \textbf{if}\ A[ i + g ] < A[ i ]\ \textbf{then}\ A[ i ]
///     \leftrightarrow A[ i + g ];\ \mathit{swapped} \gets \textbf{true}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void combSort( std::vector< T > & A , C less = C() );

/// the gnome sort
///
/// In pseudocode (Sarbazi-Azad, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{gnomeSort}( A , n ) \cr
/// 2 & \quad i \gets 1 \cr
/// 3 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ i = 0\ \textbf{or}\ A[ i - 1 ] \le A[ i ]
///     \ \textbf{then}\ i \gets i + 1 \cr
/// 5 & \qquad \textbf{else}\ A[ i ] \leftrightarrow A[ i - 1 ];\ i \gets i - 1
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void gnomeSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/ExchangeSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExchangeSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ExchangeSorts.h ------------------------*/
/*--------------------------------------------------------------------------*/
