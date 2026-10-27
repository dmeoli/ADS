/*--------------------------------------------------------------------------*/
/*---------------------------- File ListSort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of a `List` [see `List.h`] by relinking its cells, beyond the
 * book, stable, the engineered counterpart of `naturalMergeSort()` [see
 * `SequenceAlgorithms.h`], which copies the items through the positions as
 * the book does; no item is copied or moved, and the positions stay on
 * their items.
 *
 * - `listSort()` cuts the list into its natural runs, the maximal stretches
 *   not decreasing and those strictly decreasing, which it reverses, and
 *   merges them bottom up with an array of at most 64 runs, the run
 *   \f$ i \f$ made of about \f$ 2^i \f$ runs of the input, as a binary
 *   counter (the merge of `std::list::sort`, on the runs of the input
 *   instead of its single cells): \f$ O( n \log r ) \f$ comparisons for
 *   \f$ r \f$ runs, \f$ n - 1 \f$ on a sorted or reversed list, and
 *   \f$ O( 1 ) \f$ extra space. The merges follow the successors only, and
 *   the predecessors are set once at the end;
 * - on the numbers in their natural order (an arithmetic type and `less`
 *   `std::less`), the cells are sorted instead by an array of pairs
 *   ( key , cell ) and the LSD radix sort [see `RadixSort.h`], stable, and
 *   then relinked in one pass: \f$ O( n ) \f$ time and space, two walks of
 *   the cells instead of \f$ \log r \f$ of them, each of which misses the
 *   cache once per cell when the cells are scattered.
 *
 * The contracts [see `Contract.h`]: the list ends sorted by `less`, with
 * the same cells (an audit). The functions are defined in `ListSort.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.2.4 (Algorithm L, the merge sort of
 *      a list) and section 5.2.5 (the radix sort)
 * \see P. M. McIlroy, Optimistic sorting and information theoretic
 *      complexity, Proceedings of the 4th ACM-SIAM Symposium on Discrete
 *      Algorithms (SODA), 467-474, 1993
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

#ifndef __ADS_ListSort
 #define __ADS_ListSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/RadixSort.h"

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the list \f$ L \f$ sorted by relinking its cells, stable
///
/// In pseudocode (the natural merge of Knuth, section 5.2.4, by runs and by
/// a binary counter as McIlroy, beyond the book), \f$ R[ 0 .. 63 ] \f$ the
/// runs merged so far, \f$ R[ i ] \f$ made of about \f$ 2^i \f$ runs of the
/// input and the higher places holding the earlier runs:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{listSort}( L ) \cr
/// 2 & \quad \textbf{if}\ \text{the items are numbers in their natural }
///     \text{order}\ \textbf{then} \cr
/// 3 & \qquad A \gets \text{the pairs } ( \mathrm{key}( x ) , x ) \text{ of }
///     \text{the cells } x \text{ of } L;\ \text{sort } A \text{ by radix} \cr
/// 4 & \qquad \text{relink the cells in the order of } A;\ \textbf{return} \cr
/// 5 & \quad R[ 0 .. 63 ] \gets \textbf{nil} \cr
/// 6 & \quad \textbf{while}\ \text{cells are left}\ \textbf{do} \cr
/// 7 & \qquad r \gets \text{the next run, not decreasing, or strictly }
///     \text{decreasing and reversed} \cr
/// 8 & \qquad i \gets 0 \cr
/// 9 & \qquad \textbf{while}\ R[ i ] \ne \textbf{nil}\ \textbf{do}\ r
///     \gets \mathrm{merge}( R[ i ] , r );\ R[ i ] \gets \textbf{nil};\ i
///     \gets i + 1 \cr
/// 10 & \qquad R[ i ] \gets r \cr
/// 11 & \quad r \gets \textbf{nil} \cr
/// 12 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ 63\ \textbf{do}
///     \ \textbf{if}\ R[ i ] \ne \textbf{nil}\ \textbf{then}\ r \gets
///     \mathrm{merge}( R[ i ] , r ) \cr
/// 13 & \quad \text{relink the cells in the order of } r \text{, setting }
///     \text{the predecessors}
/// \end{array}}
/// \f]
/// where \f$ \mathrm{merge}( a , b ) \f$ follows the successors only and
/// takes the cell of \f$ a \f$ on equal items, which keeps the sort stable.
/// \post \f$ L \f$ is sorted by `less`, with the same cells (an audit)
template< class T , bool P , class C = std::less<> >
void listSort( List< T , P > & L , C less = C() );

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/ListSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
