/*--------------------------------------------------------------------------*/
/*---------------------------- File QuickSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Quicksort (section 12.2): `partition()`, the perno() of the book, rearranges
 * \f$ A[ \mathit{start} .. \mathit{end} ] \f$ around the pivot
 * \f$ x = A[ \mathit{start} ] \f$, so that the items before its final place
 * \f$ j \f$ are smaller and the ones after it are not, and the two parts are
 * sorted recursively. With \f$ n - 1 \f$ comparisons per partition,
 * \f[
 *   T( n ) = n - 1 + \frac{2}{n} \sum_{j=1}^{n-1} T( j )
 *   \leq 2 ( n + 1 ) \sum_{k=3}^{n+1} \frac{1}{k} = O( n \log n )
 * \f]
 * on average over the permutations, but \f$ n ( n - 1 ) / 2 \f$ when the pivot
 * is always the smallest item, e.g., on a sorted vector, and also when all the
 * items are equal (exercise 12.4), and then the recursion of `quickSort()` and
 * of `quickSort( L )` is \f$ n - 1 \f$ calls deep, which the stack of the
 * program must hold. Not stable.
 *
 * `quickSortTuned()` combines the improvements of the section: the pivot is
 * the median of \f$ A[ \mathit{start} ] \f$,
 * \f$ A[ \lfloor ( \mathit{start} + \mathit{end} ) / 2 \rfloor ] \f$ and
 * \f$ A[ \mathit{end} ] \f$; the partition of Hoare scans from both ends,
 * swapping the pairs on the wrong sides, so that equal items split evenly and
 * the case of all equal items takes \f$ O( n \log n ) \f$; the parts of at
 * most \f$ \mathit{cutoff} \f$ items are left to `insertionSort()`; the
 * recursion is an explicit stack that holds the larger part while the smaller
 * one is sorted, so that it never holds more than
 * \f$ \lfloor \log_2 n \rfloor \f$ parts. `quickSort( L )` sorts a LIST with
 * pointers by moving its items into three lists, smaller, equal and larger
 * than the pivot (exercise 12.1).
 *
 * The contracts [see `Contract.h`]: `partition()` needs
 * \f$ \mathit{start} \le \mathit{end} < n \f$ and ends with the pivot in its
 * place, the smaller items before it and the others after it, an audit;
 * `quickSort()` on a range needs \f$ \mathit{end} < n \f$ unless the range is
 * empty; the sorts end with the same items sorted, an audit. The functions are
 * defined in `QuickSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see C. A. R. Hoare, Quicksort, The Computer Journal 5(1), 10-16, 1962,
 *      doi:10.1093/comjnl/5.1.10
 * \see R. Sedgewick, Implementing Quicksort programs, Communications of the
 *      ACM 21(10), 847-857, 1978, doi:10.1145/359619.359631
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

#ifndef __ADS_QuickSort
 #define __ADS_QuickSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the pivot \f$ A[ \mathit{start} ] \f$ moved to its place \f$ j \f$, the
/// smaller items before it, the others after it (the perno() of the book);
/// \f$ \mathit{end} - \mathit{start} \f$ comparisons
///
/// In pseudocode (section 12.2), \f$ A[ \mathit{start} + 1 .. j ] \f$ holding
/// the items smaller than \f$ x \f$ seen so far:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{partition}( A , \mathit{start} ,
///     \mathit{end} ) \cr
/// 2 & \quad x \gets A[ \mathit{start} ];\ j \gets \mathit{start} \cr
/// 3 & \quad \textbf{for}\ i \gets \mathit{start} + 1\ \textbf{to}
///     \ \mathit{end}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ i ] < x\ \textbf{then}\ j \gets j + 1;\ A[ i ]
///     \leftrightarrow A[ j ] \cr
/// 5 & \quad A[ \mathit{start} ] \gets A[ j ];\ A[ j ] \gets x \cr
/// 6 & \quad \textbf{return}\ j
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} \le \mathit{end} < n \f$
/// \post \f$ A[ \mathit{start} .. j - 1 ] < A[ j ] \f$ and
///       \f$ A[ j ] \le A[ j + 1 .. \mathit{end} ] \f$ (audit)
template< class T , class C = std::less<> >
std::size_t partition( std::vector< T > & A , std::size_t start ,
                       std::size_t end , C less = C() );

/// the partition of \f$ A[ \mathit{start} .. \mathit{end} ] \f$ around a value
/// \f$ x \f$ that need not be in it, in one pass from both ends: the items not
/// larger than \f$ x \f$ first, the larger ones after; the number \f$ r \f$ of
/// the items not larger than \f$ x \f$
///
/// In pseudocode (the partition of Hoare, section 12.2), with
/// \f$ A[ \mathit{start} .. i - 1 ] \le x < A[ j .. \mathit{end} ] \f$ at all
/// times:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{partitionByValue}( A , \mathit{start} ,
///     \mathit{end} , x ) \cr
/// 2 & \quad i \gets \mathit{start};\ j \gets \mathit{end} + 1 \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{while}\ i < j\ \textbf{and}\ A[ i ] \le x\ \textbf{do}
///     \ i \gets i + 1 \cr
/// 5 & \qquad \textbf{while}\ i < j\ \textbf{and}\ A[ j - 1 ] > x
///     \ \textbf{do}\ j \gets j - 1 \cr
/// 6 & \qquad \textbf{if}\ i + 1 \ge j\ \textbf{then return}\ i -
///     \mathit{start} \cr
/// 7 & \qquad A[ i ] \leftrightarrow A[ j - 1 ];\ i \gets i + 1;\ j \gets j -
///     1
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} \le \mathit{end} < n \f$
/// \post \f$ A[ \mathit{start} .. \mathit{start} + r - 1 ] \le x \f$ and
///       \f$ x < A[ \mathit{start} + r .. \mathit{end} ] \f$, the same items
///       (audit)
template< class T , class C = std::less<> >
std::size_t partitionByValue( std::vector< T > & A , std::size_t start ,
                              std::size_t end , const T & x ,
                              C less = C() );

/// the sort of \f$ A[ \mathit{start} .. \mathit{end} ] \f$, the QuickSort() of
/// the book
///
/// In pseudocode (section 12.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{quickSort}( A , \mathit{start} ,
///     \mathit{end} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{start} < \mathit{end}\ \textbf{then} \cr
/// 3 & \qquad j \gets \mathrm{partition}( A , \mathit{start} , \mathit{end} )
///     \cr
/// 4 & \qquad \mathrm{quickSort}( A , \mathit{start} , j - 1 ) \cr
/// 5 & \qquad \mathrm{quickSort}( A , j + 1 , \mathit{end} )
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} > \mathit{end} \f$ or \f$ \mathit{end} < n \f$
/// \post \f$ A[ \mathit{start} .. \mathit{end} ] \f$ is sorted
template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , std::size_t start , std::size_t end ,
                C less = C() );

/// \f$ A \f$ in increasing order by `less`, the Quicksort of the whole vector
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{quickSort}( A ) \cr
/// 2 & \quad \textbf{if}\ n > 1\ \textbf{then}\ \mathrm{quickSort}( A , 0 , n
///     - 1 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , C less = C() );

/// Quicksort with the improvements of section 12.2; the largest number of
/// parts on the stack goes to `*depth`, if given
///
/// In pseudocode (section 12.2), the median of three placed in
/// \f$ A[ \mathit{hi} - 1 ] \f$, so that \f$ A[ \mathit{lo} ] \le x \f$ and
/// \f$ A[ \mathit{hi} - 1 ] = x \f$ stop the two scans:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{quickSortTuned}( A , \mathit{cutoff} ) \cr
/// 2 & \quad S \gets \text{an empty stack};\ ( \mathit{lo} , \mathit{hi} )
///     \gets ( 0 , n - 1 ) \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathit{hi} - \mathit{lo} + 1 \le \mathit{cutoff}
///     \ \textbf{then} \cr
/// 5 & \qquad\quad \mathrm{insertionSort}( A , \mathit{lo} , \mathit{hi} + 1
///     ) \cr
/// 6 & \qquad\quad \textbf{if}\ S = \emptyset\ \textbf{then return} \cr
/// 7 & \qquad\quad ( \mathit{lo} , \mathit{hi} ) \gets \mathrm{pop}( S );
///     \ \textbf{continue} \cr
/// 8 & \qquad \text{sort } A[ \mathit{lo} ] , A[ \mathit{mid} ] , A[
///     \mathit{hi} ];\ A[ \mathit{mid} ] \leftrightarrow A[ \mathit{hi} - 1
///     ];\ x \gets A[ \mathit{hi} - 1 ] \cr
/// 9 & \qquad i \gets \mathit{lo};\ j \gets \mathit{hi} - 1 \cr
/// 10 & \qquad \textbf{loop} \cr
/// 11 & \qquad\quad \textbf{repeat}\ i \gets i + 1\ \textbf{until}\ A[ i ] \ge
///     x \cr
/// 12 & \qquad\quad \textbf{repeat}\ j \gets j - 1\ \textbf{until}\ A[ j ] \le
///     x \cr
/// 13 & \qquad\quad \textbf{if}\ i \ge j\ \textbf{then}\ \textbf{break} \cr
/// 14 & \qquad\quad A[ i ] \leftrightarrow A[ j ] \cr
/// 15 & \qquad A[ i ] \leftrightarrow A[ \mathit{hi} - 1 ] \quad
///     \triangleright\ \text{the pivot in its place } i \cr
/// 16 & \qquad \mathrm{push}( S , \text{the larger of } [ \mathit{lo} , i - 1
///     ] \text{ and } [ i + 1 , \mathit{hi} ] ) \cr
/// 17 & \qquad ( \mathit{lo} , \mathit{hi} ) \gets \text{the smaller one}
/// \end{array}}
/// \f]
/// where a part that is empty is neither pushed nor sorted.
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void quickSortTuned( std::vector< T > & A , std::size_t cutoff = 16 ,
                     C less = C() , std::size_t * depth = nullptr );

/// Quicksort of a list with pointers: the items go to three lists, smaller,
/// equal and larger than the first one, the first and the last are sorted and
/// the three are joined
///
/// In pseudocode (exercise 12.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{quickSort}( L ) \cr
/// 2 & \quad \textbf{if}\ L \text{ has fewer than two items}\ \textbf{then
///     return} \cr
/// 3 & \quad x \gets \text{the first item of } L \cr
/// 4 & \quad S , E , G \gets \text{three empty lists} \cr
/// 5 & \quad \textbf{foreach}\ y \in L\ \textbf{do}\ \text{move } y \text{ }
///     \text{to the end of } S \text{, } E \text{ or } G \text{ as } y < x
///     \text{, } y = x \text{ or } y > x \cr
/// 6 & \quad \mathrm{quickSort}( S );\ \mathrm{quickSort}( G ) \cr
/// 7 & \quad L \gets S \cdot E \cdot G
/// \end{array}}
/// \f]
/// \post \f$ L \f$ is sorted (audit)
template< class T , class C = std::less<> >
void quickSort( List< T > & L , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/QuickSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* QuickSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File QuickSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
