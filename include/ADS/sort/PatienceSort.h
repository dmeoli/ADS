/*--------------------------------------------------------------------------*/
/*-------------------------- File PatienceSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The patience sort, beyond the book: the items are dealt, as in the card
 * game, onto piles whose tops increase from left to right, each item on the
 * leftmost pile whose top is not smaller than it (found by a binary search on
 * the tops) or on a new pile; each pile is then non-increasing from its
 * bottom, and a \f$ k \f$-way merge of the piles by a heap of their tops [see
 * `DaryHeap.h`] gives the sorted sequence, in \f$ O( n \log n ) \f$ time. The
 * number of piles is the length of a longest strictly increasing subsequence,
 * as Aldous and Diaconis tell (and `lisFast()` of `Subsequences.h` computes),
 * so the sort takes \f$ O( n \log k ) \f$ on an input with a longest
 * increasing subsequence of length \f$ k \f$, e.g., \f$ O( n ) \f$ on a
 * decreasing one. Not stable.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit. The function is defined in `PatienceSort.tpp`.
 *
 * \see C. L. Mallows, Patience sorting, SIAM Review 5(4), 375-376, 1963
 * \see D. Aldous, P. Diaconis, Longest increasing subsequences: from
 *      patience sorting to the Baik-Deift-Johansson theorem, Bulletin of
 *      the American Mathematical Society 36(4), 413-432, 1999,
 *      doi:10.1090/S0273-0979-99-00796-X
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

#ifndef __ADS_PatienceSort
 #define __ADS_PatienceSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/DaryHeap.h"
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

/// the patience sort; the number of piles goes to `*piles`, if given
///
/// In pseudocode (Mallows, beyond the book), the tops of the piles increasing
/// from left to right at all times:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{patienceSort}( A , n ) \cr
/// 2 & \quad P \gets \text{an empty sequence of piles} \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{do} \cr
/// 4 & \qquad j \gets \text{the leftmost pile with top} \ge x \text{, by}
///     \text{ binary search} \cr
/// 5 & \qquad \textbf{if}\ \text{there is none}\ \textbf{then}\ \text{append }
///     \text{a new pile } P[ j ] \text{ to } P \cr
/// 6 & \qquad \mathrm{push}( P[ j ] , x ) \cr
/// 7 & \quad H \gets \text{a min-heap of the piles by their tops} \cr
/// 8 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 9 & \qquad j \gets \mathrm{deleteMin}( H );\ A[ i ] \gets \mathrm{pop}( P[
///     j ] ) \cr
/// 10 & \qquad \textbf{if}\ P[ j ] \text{ is not empty}\ \textbf{then}
///     \ \mathrm{insert}( H , j )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void patienceSort( std::vector< T > & A , C less = C() ,
                   std::size_t * piles = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/PatienceSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PatienceSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PatienceSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
