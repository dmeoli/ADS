/*--------------------------------------------------------------------------*/
/*---------------------------- File CycleSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two sorts that minimize an operation other than the comparison, beyond
 * the book:
 *
 * - cycleSort() of Haddon writes each item once into its final place,
 *   found by counting the items smaller than it, and follows the cycle of
 *   the permutation that the item displaces: at most n writes, the least
 *   possible, at the price of \f$ \Theta( n^2 ) \f$ comparisons, which
 *   suits a memory whose writes are dear (e.g., flash); not stable;
 * - pancakeSort() sorts by reversals of prefixes (flips) only: the largest
 *   item of the unsorted prefix is flipped to the front and then to the end
 *   of the prefix, at most 2 n - 3 flips, while Gates and Papadimitriou
 *   proved that ( 5 n + 5 ) / 3 always suffice and that 17 n / 16 may be
 *   needed.
 *
 * The contracts [see Contract.h]: each sort ends with the same items
 * sorted, an audit; cycleSort() makes at most n writes and pancakeSort() at
 * most max( 0 , 2 n - 3 ) flips. The functions are defined in
 * CycleSort.tpp.
 *
 * \see B. K. Haddon, Cycle-sort: a linear sorting method, The Computer
 *      Journal 33(4), 365-367, 1990, doi:10.1093/comjnl/33.4.365
 * \see W. H. Gates, C. H. Papadimitriou, Bounds for sorting by prefix
 *      reversal, Discrete Mathematics 27(1), 47-57, 1979,
 *      doi:10.1016/0012-365X(79)90068-2
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

/// the cycle sort; the number of writes in A
/// \post A is sorted, with the same items (an audit), and the writes are
///       at most n
template< class T , class C = std::less<> >
std::size_t cycleSort( std::vector< T > & A , C less = C() );

/// the pancake sort; the number of flips, none on a sorted vector, also
/// with equal items (the last of the maxima goes to the end)
/// \post A is sorted, with the same items (an audit), and the flips are at
///       most max( 0 , 2 n - 3 )
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
