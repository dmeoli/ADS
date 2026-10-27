/*--------------------------------------------------------------------------*/
/*--------------------------- File Smoothsort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Smoothsort of Dijkstra, beyond the book: a heapsort on a forest of Leonardo
 * heaps, whose sizes are Leonardo numbers \f$ L_0 = L_1 = 1 \f$,
 * \f$ L_{k+2} = L_{k+1} + L_k + 1 \f$, laid side by side in the vector with
 * the roots increasing from left to right; the forest grows by one item at a
 * time, merging the two rightmost heaps when their orders are consecutive, and
 * shrinks by removing the root of the rightmost heap, which is the maximum. It
 * sorts in place in \f$ O( n \log n ) \f$ time like heapsort, but in
 * \f$ O( n ) \f$ on a vector that is already sorted, the transition between
 * the two being smooth, whence the name; it is not stable. The forest is kept
 * as a word of bits \f$ p \f$, the orders of its heaps relative to the order
 * \f$ \mathit{pshift} \f$ of the rightmost one, as in the classical
 * implementations.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit. The function is defined in `Smoothsort.tpp`.
 *
 * \see E. W. Dijkstra, Smoothsort, an alternative for sorting in situ,
 *      Science of Computer Programming 1(3), 223-233, 1982,
 *      doi:10.1016/0167-6423(82)90016-8
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

#ifndef __ADS_Smoothsort
 #define __ADS_Smoothsort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// \f$ A \f$ in increasing order by smoothsort
///
/// In pseudocode (Dijkstra's algorithm, beyond the book), a Leonardo heap of
/// order \f$ k \ge 2 \f$ being a root above two Leonardo heaps of orders
/// \f$ k - 1 \f$ and \f$ k - 2 \f$, and the roots of the forest kept in
/// increasing order from left to right:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{smoothsort}( A , n ) \cr
/// 2 & \quad F \gets \text{an empty forest} \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \text{the two rightmost heaps of } F \text{ have }
///     \text{orders } k + 1 \text{ and } k\ \textbf{then} \cr
/// 5 & \qquad\quad \text{make } A[ i ] \text{ the root of a heap of order } k
///     + 2 \text{ over them} \cr
/// 6 & \qquad \textbf{else}\ \text{make } A[ i ] \text{ a heap of order 1, }
///     \text{or of order 0 after one of order 1} \cr
/// 7 & \qquad \text{sift } A[ i ] \text{ down its heap, or move it left }
///     \text{along the roots and then sift it down} \cr
/// 8 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 1\ \textbf{do} \cr
/// 9 & \qquad \text{remove the root } A[ i ] \text{ of the rightmost heap, }
///     \text{the maximum} \cr
/// 10 & \qquad \textbf{if}\ \text{its order was } k \ge 2\ \textbf{then} \cr
/// 11 & \qquad\quad \text{its two subheaps join } F \text{; move their roots }
///     \text{left along the roots and sift them down}
/// \end{array}}
/// \f]
/// where a new heap that will be merged into a larger one before the end is
/// only sifted down, the merge restoring the order of the roots later, and a
/// root moves left only past the roots larger than it, so that an input
/// already sorted makes no move.
/// \pre \f$ n < L_{64} = 34335360355129 \f$, as the forest is a word of 64
///      bits [see `Smoothsort.tpp`]
/// \post \f$ A \f$ is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void smoothsort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Smoothsort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Smoothsort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Smoothsort.h --------------------------*/
/*--------------------------------------------------------------------------*/
