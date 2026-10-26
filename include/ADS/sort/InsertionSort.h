/*--------------------------------------------------------------------------*/
/*-------------------------- File InsertionSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Insertion Sort (section 2.7.2), as one sorts a hand of cards: A[ i ] is
 * inserted in the sorted prefix A[ 0 .. i - 1 ], the larger items moving
 * one place to the right. \f$ \Theta( n ) \f$ time on a sorted vector,
 * \f$ \Theta( n^2 ) \f$ on a reversed one and on average (n ( n - 1 ) / 4
 * moves), O( 1 ) space, stable; with its simple inner loop it is the
 * fastest on few items, hence the base case of the hybrid sorts.
 *
 * The contracts [see Contract.h]: lo <= hi <= n, and the range ends sorted
 * with the same items, an audit. The functions are defined in
 * InsertionSort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_InsertionSort
 #define __ADS_InsertionSort
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

/// insertionSort() of A[ lo .. hi - 1 ]
/// \pre lo <= hi <= n
/// \post A[ lo .. hi ) is sorted, with the same items
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                    C less = C() );

/// insertionSort() with a sentinel: the minimum first goes to A[ 0 ], with
/// n - 1 comparisons and one swap, so that the inner loop of each insertion
/// needs no test on the index, since it stops at A[ 0 ] at the latest
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void insertionSortSentinel( std::vector< T > & A , C less = C() );

/// insertionSort(): A in increasing order by less
template< class T , class C = std::less<> >
void insertionSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/InsertionSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* InsertionSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File InsertionSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
