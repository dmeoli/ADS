/*--------------------------------------------------------------------------*/
/*---------------------------- File QuickSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Quicksort (section 12.2): perno() rearranges A[ start .. end ] around
 * the pivot x = A[ start ], so that the items before its final place j are
 * smaller and the ones after it are not, and the two parts are sorted
 * recursively. With P( n ) = n - 1 comparisons per partition,
 * \f[
 *   T( n ) = n - 1 + \frac{2}{n} \sum_{j=1}^{n-1} T( j )
 *   \leq 2 ( n + 1 ) \sum_{k=3}^{n+1} \frac{1}{k} = O( n \log n )
 * \f]
 * on average over the permutations, but \f$ n ( n - 1 ) / 2 \f$ when the
 * pivot is always the smallest item, e.g., on a sorted vector, and also
 * when all the items are equal (exercise 12.4), and then the recursion of
 * quickSort() and of quickSort( L ) is n - 1 calls deep, which the stack of
 * the program must hold. Not stable.
 *
 * quickSortTuned() combines the improvements of the section: the pivot is
 * the median of A[ start ], A[ ( start + end ) / 2 ] and A[ end ]; the
 * partition of Hoare scans from both ends, swapping the pairs on the wrong
 * sides, so that equal items split evenly and the case of all equal items
 * takes O( n log n ); the parts of at most cutoff items are left to
 * insertionSort(); the recursion is an explicit stack that holds the
 * larger part while the smaller one is sorted, so that it never holds more
 * than \f$ \lfloor \log_2 n \rfloor \f$ parts. quickSort( L ) sorts a LIST
 * with pointers by moving its items into three lists, smaller, equal and
 * larger than the pivot (exercise 12.1).
 *
 * The contracts [see Contract.h]: partition() needs start <= end < n and ends
 * with the pivot in its place, the smaller items before it and the others
 * after it, an audit; quickSort() on a range needs end < n unless the range is
 * empty; the sorts end with the same items sorted, an audit. The functions are
 * defined in QuickSort.tpp.
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
 *         Universita' di Pisa \n
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

/// perno(): the pivot A[ start ] goes to its place j, the smaller items
/// before it, the others after it; end - start comparisons
/// \pre start <= end < n
/// \post A[ start .. j - 1 ] < A[ j ] <= A[ j + 1 .. end ]
template< class T , class C = std::less<> >
std::size_t partition( std::vector< T > & A , std::size_t start ,
                       std::size_t end , C less = C() );

/// the partition of A[ start .. end ] around a value x that need not be
/// in it: the items not larger than x first, the larger ones after, in one
/// pass from both ends; the number of the items not larger than x
/// \pre start <= end < n
/// \post A[ start .. start + r ) <= x < A[ start + r .. end ], the same
///       items (an audit)
template< class T , class C = std::less<> >
std::size_t partitionByValue( std::vector< T > & A , std::size_t start ,
                              std::size_t end , const T & x ,
                              C less = C() );

/// QuickSort() of A[ start .. end ], the book's
/// \pre start > end or end < n
/// \post A[ start .. end ] is sorted
template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , std::size_t start , std::size_t end ,
                C less = C() );

/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void quickSort( std::vector< T > & A , C less = C() );

/// Quicksort with the improvements of section 12.2; the largest number of
/// parts on the stack goes to depth, if given
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void quickSortTuned( std::vector< T > & A , std::size_t cutoff = 16 ,
                     C less = C() , std::size_t * depth = nullptr );

/// Quicksort of a list with pointers: the items go to three lists,
/// smaller, equal and larger than the first one, the first and the last
/// are sorted and the three are joined
/// \post L is sorted
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
