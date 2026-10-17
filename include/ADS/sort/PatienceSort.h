/*--------------------------------------------------------------------------*/
/*-------------------------- File PatienceSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The patience sort, beyond the book: the items are dealt, as in the card
 * game, onto piles whose tops increase from left to right, each item on the
 * leftmost pile whose top is not smaller than it (found by a binary search
 * on the tops) or on a new pile; each pile is then decreasing from its
 * bottom, and a k-way merge of the piles by a heap of their tops [see
 * DaryHeap.h] gives the sorted sequence, in \f$ O( n \log n ) \f$ time.
 * The number of piles is the length of a longest strictly increasing
 * subsequence, as Aldous and Diaconis tell (and lisFast() of
 * Subsequences.h computes), so the sort takes O( n log k ) on an input with
 * a longest increasing subsequence of length k, e.g., O( n ) on a
 * decreasing one.
 *
 * The contracts [see Contract.h]: the vector ends with the same items
 * sorted, an audit. The function is defined in PatienceSort.tpp.
 *
 * \see C. L. Mallows, Patience sorting, SIAM Review 5(4), 375-376, 1963
 * \see D. Aldous, P. Diaconis, Longest increasing subsequences: from
 *      patience sorting to the Baik-Deift-Johansson theorem, Bulletin of
 *      the American Mathematical Society 36(4), 413-432, 1999,
 *      doi:10.1090/S0273-0979-99-00796-X
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

/// the patience sort; the number of piles goes to *piles, if given
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void patienceSort( std::vector< T > & A , C less = C() ,
                   std::size_t * piles = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/PatienceSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PatienceSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PatienceSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
