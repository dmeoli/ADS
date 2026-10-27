/*--------------------------------------------------------------------------*/
/*---------------------------- File Heapsort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The max-heap on a vector and heapsort (section 10.1.4): the children of the
 * place \f$ i \f$ are \f$ 2 i + 1 \f$ and \f$ 2 i + 2 \f$, and a max-heap has
 * \f$ A[ \lfloor ( i - 1 ) / 2 \rfloor ] \ge A[ i ] \f$ for each
 * \f$ i > 0 \f$.
 *
 * `maxHeapRestore( A , i , dim )` moves the item in \f$ A[ i ] \f$ down,
 * swapping it with its largest child, when the subtrees of the children are
 * max-heaps in \f$ A[ 0 .. \mathit{dim} - 1 ] \f$: \f$ O( m ) \f$ time on a
 * node of height \f$ m \f$. `heapBuild()` calls it from
 * \f$ \lfloor n / 2 \rfloor - 1 \f$ down to 0, the places from
 * \f$ \lfloor n / 2 \rfloor \f$ on being leaves; since a heap of \f$ n \f$
 * items has at most \f$ \lceil n / 2^{m+1} \rceil \f$ nodes of height
 * \f$ m \f$, its time is
 * \f[
 *   O\left( \sum_{m=0}^{\lfloor \log n \rfloor} \frac{n}{2^{m+1}} m
 *   \right) = O\left( n \sum_{m=0}^{\infty} \frac{m}{2^m} \right) = O( n ),
 * \f]
 * the series being 2. `heapsort()` builds the heap and then swaps the maximum
 * \f$ A[ 0 ] \f$ with the last place of the heap, which shrinks by one, and
 * restores it, in \f$ O( n \log n ) \f$ time and \f$ O( 1 ) \f$ extra space,
 * since `maxHeapRestore()` is a loop rather than the recursion of the book
 * (exercise 10.4). Not stable.
 *
 * The contracts [see `Contract.h`]: `maxHeapRestore()` and `isMaxHeap()` need
 * \f$ \mathit{dim} \le n \f$, `maxHeapRestore()` also
 * \f$ i < \mathit{dim} \f$, and `heapBuild()` ends with a heap and
 * `heapsort()` with the same items sorted, two audits. The functions are
 * defined in `Heapsort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.3734138
 * \see R. W. Floyd, Algorithm 245: Treesort 3, Communications of the ACM
 *      7(12), 701, 1964, doi:10.1145/355588.365103
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

#ifndef __ADS_Heapsort
 #define __ADS_Heapsort
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

/*--------------------------------------------------------------------------*/

/// \f$ A[ i ] \f$ moved down in the heap \f$ A[ 0 .. \mathit{dim} - 1 ] \f$,
/// with the order `less`, \f$ O( 1 ) \f$ extra space
///
/// In pseudocode (section 10.1.4, a loop instead of the recursion of the
/// book), the subtrees of the children of \f$ i \f$ being max-heaps:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{maxHeapRestore}( A , i , \mathit{dim} ) \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad \mathit{max} \gets i;\ l \gets 2 i + 1;\ r \gets 2 i + 2 \cr
/// 4 & \qquad \textbf{if}\ l < \mathit{dim}\ \textbf{and}\ A[ l ] > A[
///     \mathit{max} ]\ \textbf{then}\ \mathit{max} \gets l \cr
/// 5 & \qquad \textbf{if}\ r < \mathit{dim}\ \textbf{and}\ A[ r ] > A[
///     \mathit{max} ]\ \textbf{then}\ \mathit{max} \gets r \cr
/// 6 & \qquad \textbf{if}\ \mathit{max} = i\ \textbf{then return} \cr
/// 7 & \qquad A[ i ] \leftrightarrow A[ \mathit{max} ];\ i \gets \mathit{max}
/// \end{array}}
/// \f]
/// \pre \f$ i < \mathit{dim} \le n \f$
template< class T , class C = std::less<> >
void maxHeapRestore( std::vector< T > & A , std::size_t i , std::size_t dim ,
                     C less = C() );

/// \f$ A \f$ made a max-heap, \f$ O( n ) \f$ time
///
/// In pseudocode (section 10.1.4), the leaves being heaps already:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{heapBuild}( A , n ) \cr
/// 2 & \quad \textbf{for}\ i \gets \lfloor n / 2 \rfloor - 1\ \textbf{downto}
///    \ 0\ \textbf{do} \cr
/// 3 & \qquad \mathrm{maxHeapRestore}( A , i , n )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is a max-heap (audit)
template< class T , class C = std::less<> >
void heapBuild( std::vector< T > & A , C less = C() );

/// \f$ A \f$ in increasing order, \f$ O( n \log n ) \f$ time
///
/// In pseudocode (section 10.1.4), \f$ A[ i .. n - 1 ] \f$ holding the largest
/// items in order and \f$ A[ 0 .. i - 1 ] \f$ a heap of the others:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{heapsort}( A , n ) \cr
/// 2 & \quad \mathrm{heapBuild}( A , n ) \cr
/// 3 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 1\ \textbf{do} \cr
/// 4 & \qquad A[ 0 ] \leftrightarrow A[ i ] \cr
/// 5 & \qquad \mathrm{maxHeapRestore}( A , 0 , i )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void heapsort( std::vector< T > & A , C less = C() );

/// true if \f$ A[ 0 .. \mathit{dim} - 1 ] \f$ is a max-heap, i.e., if no item
/// is larger than its parent
/// \pre \f$ \mathit{dim} \le n \f$
template< class T , class C = std::less<> >
bool isMaxHeap( const std::vector< T > & A , std::size_t dim , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Heapsort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Heapsort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Heapsort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
