/*--------------------------------------------------------------------------*/
/*------------------------- File Rearrangements.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rearrangements of a vector in place, by swaps or with a small buffer:
 * the partition into three classes, the alternating order, the sort of a
 * vector whose items are near their places, and the cost of sorting by
 * swaps.
 *
 * - `dutchNationalFlag()`: the items of color 0, then those of color 1,
 *   then those of color 2, by Dijkstra's invariant \f$ A[ 0 .. i ) \f$ of
 *   color 0, \f$ A[ i .. j ) \f$ of color 1, \f$ A[ j .. k ) \f$ not
 *   examined, \f$ A[ k .. n ) \f$ of color 2: each step examines
 *   \f$ A[ j ] \f$ and swaps it to its class, \f$ n \f$ colors read and at
 *   most \f$ n \f$ swaps, \f$ O( 1 ) \f$ space. It is the partition of
 *   quicksort with three ways, the keys smaller than, equal to and larger
 *   than the pivot [see `EngineeredQuickSorts.h` for the partition of
 *   Bentley and McIlroy, which moves the equal keys fewer times];
 * - `wiggleSort()`: \f$ A[ 0 ] \le A[ 1 ] \ge A[ 2 ] \le A[ 3 ] \ge \cdots
 *   \f$, by a pass of swaps of neighbors: when \f$ A[ 0 .. i ] \f$
 *   alternates, a swap of \f$ A[ i ] \f$ and \f$ A[ i + 1 ] \f$ when they are
 *   in the wrong order keeps the order with \f$ A[ i - 1 ] \f$ by
 *   transitivity; \f$ n - 1 \f$ comparisons. With distinct items the order
 *   is strict; the strict order with repeated items, when it exists, needs
 *   the median and a partition into three ways about it, still in
 *   \f$ O( n ) \f$ time;
 * - `sortKSorted()`: the sort of a vector each of whose items is at most
 *   \f$ k \f$ places away from its place in the sorted order: the item of
 *   the place \f$ i \f$ is the smallest of \f$ A[ i .. i + k ] \f$, hence a
 *   heap of \f$ k + 1 \f$ items gives each item in turn,
 *   \f$ O( n \log k ) \f$ time and \f$ O( k ) \f$ space, against the
 *   \f$ O( n k ) \f$ of insertion sort [see `InsertionSort.h`];
 *   \f$ \Omega( n \log k ) \f$ comparisons are needed, since about
 *   \f$ ( k! )^{n / k} \f$ orders are \f$ k \f$-sorted;
 * - `minimumSwapSort()`: the sort of distinct items by swaps, the fewest
 *   swaps and the least total cost when a swap costs the sum of the two
 *   items swapped. Each cycle of the permutation that sorts \f$ A \f$, of
 *   \f$ k > 1 \f$ items, needs \f$ k - 1 \f$ swaps, hence \f$ n \f$ minus the
 *   number of cycles in all; its items, of sum \f$ s \f$ and minimum
 *   \f$ m \f$, are sorted at the least cost
 *   \f$ \min( s + ( k - 2 ) m , s + m + ( k + 1 ) M ) \f$, \f$ M \f$ the
 *   minimum of \f$ A \f$: the minimum of the cycle swapped with each of the
 *   others in turn, or \f$ M \f$ brought into the cycle by a swap with
 *   \f$ m \f$, used in the same way, and swapped back. The two optima may
 *   come from different sequences of swaps; \f$ O( n \log n ) \f$ for the
 *   sort that gives the permutation.
 *
 * The contracts [see `Contract.h`]: the colors must be 0, 1 or 2, and come
 * out in order (an audit); `wiggleSort()` and `sortKSorted()` give a
 * permutation of the items (an audit) alternating and sorted;
 * `sortKSorted()` needs a vector \f$ k \f$-sorted and `minimumSwapSort()`
 * distinct items, audits. The functions are defined in
 * `Rearrangements.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti, and the collection of
 *      exercises on the analysis of algorithms
 * \see E. W. Dijkstra, A Discipline of Programming, Prentice-Hall, 1976,
 *      chapter 14
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 1.3.3
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

#ifndef __ADS_Rearrangements
 #define __ADS_Rearrangements
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items of \f$ A \f$ by `colour( x )`, 0 then 1 then 2, with \f$ n \f$
/// reads of the colors and at most \f$ n \f$ swaps, counted in `*swaps` if
/// it is given; \f$ ( i , j ) \f$ with \f$ A[ 0 .. i ) \f$ of color 0,
/// \f$ A[ i .. j ) \f$ of color 1 and \f$ A[ j .. n ) \f$ of color 2
///
/// In pseudocode (Dijkstra, chapter 14, beyond the book), keeping the
/// invariant (4) of the theory of the area:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dutchNationalFlag}( A ) \cr
/// 2 & \quad i \gets 0;\ j \gets 0;\ k \gets n \cr
/// 3 & \quad \textbf{while}\ j < k\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{colour}( A[ j ] ) = 0\ \textbf{then}
///     \ \text{swap } A[ i ] \text{ and } A[ j ];\ i \gets i + 1;\ j \gets j
///     + 1 \cr
/// 5 & \qquad \textbf{else if}\ \mathrm{colour}( A[ j ] ) = 1
///     \ \textbf{then}\ j \gets j + 1 \cr
/// 6 & \qquad \textbf{else}\ k \gets k - 1;\ \text{swap } A[ j ]
///     \text{ and } A[ k ] \cr
/// 7 & \quad \textbf{return}\ ( i , j )
/// \end{array}}
/// \f]
/// \pre `colour( x )` is 0, 1 or 2
/// \post the colors in order (an audit)
template< class T , class F >
std::pair< std::size_t , std::size_t > dutchNationalFlag(
                  std::vector< T > & A , F && colour ,
                  std::size_t * swaps = nullptr );

/// \f$ A[ 0 ] \le A[ 1 ] \ge A[ 2 ] \le \cdots \f$, by swaps of
/// neighbors; \f$ n - 1 \f$ comparisons
///
/// In pseudocode (an exercise of the collections of the book), one pass of
/// swaps of neighbors, each keeping the order of the pair before it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{wiggleSort}( A ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 2\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ ( i \text{ even }\ \textbf{and}\ A[ i + 1 ] <
///     A[ i ] )\ \textbf{or}\ ( i \text{ odd }\ \textbf{and}\ A[ i ] < A[
///     i + 1 ] )\ \textbf{then} \cr
/// 4 & \qquad\quad \text{swap } A[ i ] \text{ and } A[ i + 1 ]
/// \end{array}}
/// \f]
/// \post \f$ A \f$ alternates, and holds its items (an audit)
template< class T , class C = std::less<> >
void wiggleSort( std::vector< T > & A , C less = C() );

/// \f$ A \f$ sorted, each item at most \f$ k \f$ places away from its place
/// in the sorted order, by a heap of \f$ k + 1 \f$ items;
/// \f$ O( n \log k ) \f$
///
/// In pseudocode (an exercise of the collections of the book), \f$ H \f$ a
/// heap of the least item on the window \f$ A[ i .. i + k ] \f$ of the items
/// not yet placed:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{sortKSorted}( A , k ) \cr
/// 2 & \quad w \gets \min( k , n - 1 ) + 1;\ H \gets \text{a heap of }
///     A[ 0 .. w ) \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad A[ i ] \gets \mathrm{deleteMin}( H ) \cr
/// 5 & \qquad \textbf{if}\ i + w < n\ \textbf{then}\ \mathrm{insert}( H , A[
///     i + w ] )\quad \triangleright\ \text{one sift for both}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is \f$ k \f$-sorted (an audit)
/// \post \f$ A \f$ sorted, a permutation of its items (an audit)
template< class T , class C = std::less<> >
void sortKSorted( std::vector< T > & A , std::size_t k , C less = C() );

/// the fewest swaps that sort \f$ A \f$, and the least total cost of the
/// swaps that sort it, a swap costing the sum of its two items, computed in
/// the type `W`; \f$ O( n \log n ) \f$
///
/// In pseudocode (Knuth, section 1.3.3, on the cycles of a permutation,
/// beyond the book), each cycle of \f$ k > 1 \f$ items needing
/// \f$ k - 1 \f$ swaps at the cost (5) of the theory of the area:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minimumSwapSort}( A ) \cr
/// 2 & \quad t[ i ] \gets \text{the rank of } A[ i ] \text{ in the sorted }
///     \text{order};\ M \gets \min_i A[ i ] \cr
/// 3 & \quad \mathit{swaps} \gets 0;\ \mathit{cost} \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ i\ \textbf{with}\ i \text{ not seen}
///     \ \textbf{do} \cr
/// 5 & \qquad \text{follow } i , t[ i ] , t[ t[ i ] ] , \ldots \text{ to }
///     i \text{, marking them seen:}\ k \text{ items, of sum } s
///     \text{ and least } m \cr
/// 6 & \qquad \textbf{if}\ k > 1\ \textbf{then} \cr
/// 7 & \qquad\quad \mathit{swaps} \gets \mathit{swaps} + k - 1 \cr
/// 8 & \qquad\quad \mathit{cost} \gets \mathit{cost} + \min( s + ( k - 2 )
///     m , s + m + ( k + 1 ) M ) \cr
/// 9 & \quad \textbf{return}\ ( \mathit{swaps} , \mathit{cost} )
/// \end{array}}
/// \f]
/// \pre the items of \f$ A \f$ are distinct (an audit)
template< class T , class W = T >
 requires std::is_arithmetic_v< T >
std::pair< std::size_t , W > minimumSwapSort( const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/Rearrangements.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Rearrangements.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Rearrangements.h ------------------------*/
/*--------------------------------------------------------------------------*/
