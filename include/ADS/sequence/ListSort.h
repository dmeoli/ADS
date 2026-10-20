/*--------------------------------------------------------------------------*/
/*---------------------------- File ListSort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of a List [see List.h] by relinking its cells, beyond the book,
 * stable, the engineered counterpart of naturalMergeSort() [see
 * SequenceAlgorithms.h], which copies the items through the positions as
 * the book does; no item is copied or moved, and the positions stay on
 * their items.
 *
 * - listSort() cuts the list into its natural runs, the maximal stretches
 *   not decreasing and those strictly decreasing, which it reverses, and
 *   merges them bottom up with an array of at most 64 runs, the run i
 *   made of about \f$ 2^i \f$ runs of the input, as a binary counter
 *   (the merge of std::list::sort, on the runs of the input instead of
 *   its single cells): \f$ O( n \log r ) \f$ comparisons for r runs, n -
 *   1 on a sorted or reversed list, and O( 1 ) extra space. The merges
 *   follow the successors only, and the predecessors are set once at the
 *   end;
 * - on the numbers in their natural order (an arithmetic type and less
 *   std::less), the cells are sorted instead by an array of pairs ( key ,
 *   cell ) and the LSD radix sort [see RadixSort.h], stable, and then
 *   relinked in one pass: O( n ) time and space, two walks of the cells
 *   instead of \f$ \log r \f$ of them, each of which misses the cache
 *   once per cell when the cells are scattered.
 *
 * The contracts [see Contract.h]: the list ends sorted by less, with the
 * same cells (an audit). The functions are defined in ListSort.tpp.
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
 *         Universita' di Pisa \n
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

/// the list L sorted by relinking its cells, stable
/// \post L is sorted by less, with the same cells (an audit)
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
