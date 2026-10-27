/*--------------------------------------------------------------------------*/
/*---------------------------- File CycleSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two sorts that minimize an operation other than the comparison, beyond the
 * book:
 *
 * - `cycleSort()` of Haddon writes each item once into its final place, found
 *   by counting the items smaller than it, and follows the cycle of the
 *   permutation that the item displaces: at most \f$ n \f$ writes, the least
 *   possible, at the price of \f$ \Theta( n^2 ) \f$ comparisons, which suits a
 *   memory whose writes are dear (e.g., flash); not stable;
 * - `pancakeSort()` sorts by reversals of prefixes (flips) only: the largest
 *   item of the unsorted prefix is flipped to the front and then to the end of
 *   the prefix, at most \f$ 2 n - 3 \f$ flips, while Gates and Papadimitriou
 *   proved that \f$ ( 5 n + 5 ) / 3 \f$ always suffice and that
 *   \f$ 17 n / 16 \f$ may be needed.
 *
 * The contracts [see `Contract.h`]: each sort ends with the same items sorted,
 * an audit; `cycleSort()` makes at most \f$ n \f$ writes and `pancakeSort()`
 * at most \f$ \max\{ 0 , 2 n - 3 \} \f$ flips. The functions are defined in
 * `CycleSort.tpp`.
 *
 * \see B. K. Haddon, Cycle-sort: a linear sorting method, The Computer
 *      Journal 33(4), 365-367, 1990, doi:10.1093/comjnl/33.4.365
 * \see W. H. Gates, C. H. Papadimitriou, Bounds for sorting by prefix
 *      reversal, Discrete Mathematics 27(1), 47-57, 1979,
 *      doi:10.1016/0012-365X(79)90068-2
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

#ifndef __ADS_CycleSort
 #define __ADS_CycleSort
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

/// the cycle sort; the number of writes in \f$ A \f$
///
/// In pseudocode (Haddon's algorithm, beyond the book), the place of an item
/// being \f$ \mathit{start} \f$ plus the number of the items smaller than it
/// in \f$ A[ \mathit{start} + 1 .. n - 1 ] \f$, after its equals already
/// placed:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cycleSort}( A , n ) \cr
/// 2 & \quad w \gets 0 \cr
/// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ n - 2\ \textbf{do} \cr
/// 4 & \qquad x \gets A[ s ];\ p \gets \text{the place of } x \cr
/// 5 & \qquad \textbf{if}\ p = s\ \textbf{then continue} \cr
/// 6 & \qquad \textbf{repeat} \cr
/// 7 & \qquad\quad p \gets \text{the place of } x \text{, after its equals}
///     \cr
/// 8 & \qquad\quad x \leftrightarrow A[ p ];\ w \gets w + 1 \cr
/// 9 & \qquad \textbf{until}\ p = s \cr
/// 10 & \quad \textbf{return}\ w
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit), and the writes are
///       at most \f$ n \f$
template< class T , class C = std::less<> >
std::size_t cycleSort( std::vector< T > & A , C less = C() );

/// the pancake sort; the number of flips, none on a sorted vector, also with
/// equal items (the last of the maxima goes to the end)
///
/// In pseudocode (beyond the book), \f$ \mathrm{flip}( A , i ) \f$ reversing
/// \f$ A[ 0 .. i ] \f$ and \f$ A[ k .. n - 1 ] \f$ holding the largest items
/// in order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pancakeSort}( A , n ) \cr
/// 2 & \quad f \gets 0 \cr
/// 3 & \quad \textbf{for}\ k \gets n\ \textbf{downto}\ 2\ \textbf{do} \cr
/// 4 & \qquad m \gets \text{the last place of a maximum of } A[ 0 .. k - 1 ]
///     \cr
/// 5 & \qquad \textbf{if}\ m = k - 1\ \textbf{then continue} \cr
/// 6 & \qquad \textbf{if}\ m > 0\ \textbf{then}\ \mathrm{flip}( A , m );\ f
///     \gets f + 1 \cr
/// 7 & \qquad \mathrm{flip}( A , k - 1 );\ f \gets f + 1 \cr
/// 8 & \quad \textbf{return}\ f
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit), and the flips are
///       at most \f$ \max\{ 0 , 2 n - 3 \} \f$
template< class T , class C = std::less<> >
std::size_t pancakeSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/CycleSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CycleSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CycleSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
