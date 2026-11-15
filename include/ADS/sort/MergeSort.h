/*--------------------------------------------------------------------------*/
/*---------------------------- File MergeSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Merge Sort (section 2.7.3): the two halves of the vector are sorted
 * recursively and merged by `mergeParts()`, the Merge() of the book, through a
 * vector \f$ B \f$ allocated once,
 * \f$ T( n ) = 2 T( n / 2 ) + \Theta( n ) = \Theta( n \log n ) \f$ time,
 * optimal among the sorts by comparisons, \f$ \Theta( n ) \f$ extra space,
 * stable (an item of the left part goes first when equal). When the right
 * part runs out first, the rest of the left one is copied; when the left one
 * does, the rest of the right one is already in place.
 *
 * - `mergeSort( A , cutoff )` sorts the parts of at most
 *   \f$ \mathit{cutoff} \f$ items with `insertionSort()`, which is faster on
 *   few items (section 2.8);
 * - `mergeSortBottomUp()` merges the runs of length \f$ 1 , 2 , 4 , \ldots \f$
 *   with no recursion and no stack (exercise 12.5);
 * - `mergeSort( L )` sorts a LIST with pointers in \f$ O( n \log n ) \f$ by
 *   splitting it in two halves, sorting them and merging them (exercise
 *   12.1), only through the operations of the SEQUENCE: each item is copied
 *   once per level of the recursion, since the abstract data type moves no
 *   cells from a list to another;
 * - `countInversions()` counts the pairs \f$ i < j \f$ with
 *   \f$ A[ i ] > A[ j ] \f$: when the merge takes \f$ A[ j ] \f$ from the
 *   right part, it is smaller than all the \f$ \mathit{mid} - i + 1 \f$ items
 *   left in the left part (exercise 12.3).
 *
 * The contracts [see `Contract.h`]: `mergeParts()` needs
 * \f$ \mathit{start} \le \mathit{mid} < \mathit{end} < n \f$, a buffer
 * \f$ B \f$ as large as \f$ A \f$ and two sorted halves, the latter an audit;
 * `mergeSortRec()` needs \f$ \mathit{start} \le \mathit{end} < n \f$ and
 * \f$ \mathit{cutoff} \ge 1 \f$; the sorts end with the same items sorted, an
 * audit. The functions are defined in `MergeSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 5.2.4
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

#ifndef __ADS_MergeSort
 #define __ADS_MergeSort
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
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// \f$ A[ \mathit{start} .. \mathit{mid} ] \f$ and
/// \f$ A[ \mathit{mid} + 1 .. \mathit{end} ] \f$, sorted, merged through
/// \f$ B \f$ (the Merge() of the book); the inversions met are added to
/// `*inv`, if given
///
/// In pseudocode (section 2.7.3), the smaller head moving to \f$ B \f$, the
/// left one on a tie:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mergeParts}( A , B , \mathit{start} ,
///     \mathit{mid} , \mathit{end} ) \cr
/// 2 & \quad i \gets \mathit{start};\ j \gets \mathit{mid} + 1;\ k \gets
///     \mathit{start} \cr
/// 3 & \quad \textbf{while}\ i \le \mathit{mid}\ \textbf{and}\ j \le
///     \mathit{end}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ j ] < A[ i ]\ \textbf{then}\quad \triangleright
///     \ A[ j ] \text{ passes } A[ i .. \mathit{mid} ] \cr
/// 5 & \qquad\quad \mathit{inv} \gets \mathit{inv} + \mathit{mid} - i + 1;
///     \ B[ k ] \gets A[ j ];\ j \gets j + 1 \cr
/// 6 & \qquad \textbf{else}\ B[ k ] \gets A[ i ];\ i \gets i + 1 \cr
/// 7 & \qquad k \gets k + 1 \cr
/// 8 & \quad \text{append } A[ i .. \mathit{mid} ] \text{ to } B \text{ from }
///     B[ k ] \text{, advancing } k \cr
/// 9 & \quad \text{copy } B[ \mathit{start} .. k - 1 ] \text{ back to } A
///     \quad \triangleright\ A[ k .. \mathit{end} ] \text{ is in place}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} \le \mathit{mid} < \mathit{end} < n \le |B| \f$,
///      \f$ A[ \mathit{start} .. \mathit{mid} ] \f$ and
///      \f$ A[ \mathit{mid} + 1 .. \mathit{end} ] \f$ sorted (audit)
/// \post \f$ A[ \mathit{start} .. \mathit{end} ] \f$ sorted (audit)
template< class T , class C = std::less<> >
void mergeParts( std::vector< T > & A , std::vector< T > & B ,
		 std::size_t start ,
		 std::size_t mid , std::size_t end , C less = C() ,
		 long long * inv = nullptr );

/// the sort of \f$ A[ \mathit{start} .. \mathit{end} ] \f$ (the MergeSort() of
/// the book), `insertionSort()` on at most \f$ \mathit{cutoff} \f$ items; with
/// \f$ \mathit{cutoff} > 1 \f$ also no merge of two parts already in order,
/// i.e., with \f$ A[ \mathit{mid} ] \le A[ \mathit{mid} + 1 ] \f$, which makes
/// the sort of a sorted vector take \f$ n - 1 \f$ comparisons
///
/// In pseudocode (section 2.7.3, with the cutoff of section 2.8):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mergeSortRec}( A , B , \mathit{start} ,
///     \mathit{end} , \mathit{cutoff} ) \cr
/// 2 & \quad \textbf{if}\ \mathit{end} - \mathit{start} + 1 \le
///     \mathit{cutoff}\ \textbf{then} \cr
/// 3 & \qquad \mathrm{insertionSort}( A , \mathit{start} , \mathit{end} + 1
///     );\ \textbf{return} \cr
/// 4 & \quad \mathit{mid} \gets \mathit{start} + \lfloor ( \mathit{end} -
///     \mathit{start} ) / 2 \rfloor \cr
/// 5 & \quad \mathrm{mergeSortRec}( A , B , \mathit{start} , \mathit{mid} ,
///     \mathit{cutoff} ) \cr
/// 6 & \quad \mathrm{mergeSortRec}( A , B , \mathit{mid} + 1 , \mathit{end} ,
///     \mathit{cutoff} ) \cr
/// 7 & \quad \textbf{if}\ \mathit{cutoff} = 1\ \textbf{or}\ A[ \mathit{mid} +
///     1 ] < A[ \mathit{mid} ]\ \textbf{then} \cr
/// 8 & \qquad \mathrm{mergeParts}( A , B , \mathit{start} , \mathit{mid} ,
///     \mathit{end} )\quad \triangleright\ \text{else already in order}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{start} \le \mathit{end} < n \le |B| \f$ and
///      \f$ \mathit{cutoff} \ge 1 \f$
template< class T , class C = std::less<> >
void mergeSortRec( std::vector< T > & A , std::vector< T > & B ,
		   std::size_t start , std::size_t end , std::size_t cutoff ,
		   C less = C() , long long * inv = nullptr );

/// \f$ A \f$ in increasing order, stable; the parts of at most
/// \f$ \mathit{cutoff} \f$ items by `insertionSort()` (1 is the book)
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mergeSort}( A , \mathit{cutoff} ) \cr
/// 2 & \quad \textbf{if}\ n \ge 2\ \textbf{then}\ \mathrm{mergeSortRec}( A ,
///     \text{a vector of } n \text{ places} , 0 , n - 1 , \mathit{cutoff} )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void mergeSort( std::vector< T > & A , std::size_t cutoff = 1 , C less = C() );

/// the iterative Merge Sort: the runs of length \f$ w \f$ are merged in pairs,
/// for \f$ w = 1 , 2 , 4 , \ldots \f$
///
/// In pseudocode (exercise 12.5), after the pass of width \f$ w \f$ each run
/// of \f$ 2 w \f$ items from a multiple of \f$ 2 w \f$ being sorted:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mergeSortBottomUp}( A , n ) \cr
/// 2 & \quad \textbf{for}\ w \gets 1 , 2 , 4 , \ldots \text{ while } w < n
///     \ \textbf{do}\quad \triangleright\ \text{the runs of length } w
///     \text{ sorted} \cr
/// 3 & \qquad \textbf{for}\ \mathit{start} \gets 0 , 2 w , 4 w , \ldots
///     \text{ while } \mathit{start} + w < n\ \textbf{do} \cr
/// 4 & \qquad\quad \mathrm{mergeParts}( A , B , \mathit{start} ,
///     \mathit{start} + w - 1 , \min\{ \mathit{start} + 2 w , n \} - 1 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void mergeSortBottomUp( std::vector< T > & A , C less = C() );

/// the number of inversions of \f$ A \f$, the pairs \f$ i < j \f$ with
/// \f$ A[ j ] < A[ i ] \f$, by Merge Sort, \f$ O( n \log n ) \f$; \f$ A \f$
/// ends sorted
///
/// In pseudocode (exercise 12.3), each merge counting the items of the left
/// part that an item of the right part overtakes:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countInversions}( A ) \cr
/// 2 & \quad \mathit{inv} \gets 0 \cr
/// 3 & \quad \textbf{if}\ n \ge 2\ \textbf{then}\ \mathrm{mergeSortRec}( A , B
///     , 0 , n - 1 , 1 )\quad \triangleright\ \mathrm{mergeParts} \text{ adds}
///     \cr
/// 4 & \quad \textbf{return}\ \mathit{inv}
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
long long countInversions( std::vector< T > & A , C less = C() );

/// Merge Sort of a list with pointers, through the operations of the SEQUENCE
/// alone: the items are copied from a list to another, once per level, since
/// the abstract data type moves no cells
///
/// In pseudocode (exercise 12.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{mergeSort}( L ) \cr
/// 2 & \quad n \gets \text{the length of } L;\ \textbf{if}\ n < 2
///     \ \textbf{then return} \cr
/// 3 & \quad R \gets \text{an empty list} \cr
/// 4 & \quad \text{move the items of } L \text{ after its first } \lfloor n /
///     2 \rfloor \text{ to the end of } R \cr
/// 5 & \quad \mathrm{mergeSort}( L );\ \mathrm{mergeSort}( R );\ q \gets
///     L.\mathrm{head}() \cr
/// 6 & \quad \textbf{foreach}\ x \in R \text{ from its head}\ \textbf{do} \cr
/// 7 & \qquad \textbf{while}\ \textbf{not}\ L.\mathrm{finished}( q )
///     \ \textbf{and}\ \textbf{not}\ x < L.\mathrm{read}( q )\ \textbf{do} \cr
/// 8 & \qquad\quad q \gets L.\mathrm{next}( q ) \cr
/// 9 & \qquad L.\mathrm{insert}( q , x )\quad \triangleright\ \text{before the
///     first larger item}
/// \end{array}}
/// \f]
/// \post \f$ L \f$ is sorted (audit)
template< class T , class C = std::less<> >
void mergeSort( List< T > & L , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/MergeSort.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MergeSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MergeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
