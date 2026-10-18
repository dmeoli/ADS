/*--------------------------------------------------------------------------*/
/*------------------------ File MultikeyQuickSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multikey quicksort of Bentley and Sedgewick, beyond the book: the
 * quicksort of strings by one character at a time, a partition into three
 * parts, the strings whose character d is smaller, equal and larger than
 * that of the pivot, the equal part going on with the character d + 1 and
 * the others with d. It is the sort that corresponds to the ternary search
 * tree of the same paper, and makes \f$ O( D + n \log n ) \f$
 * comparisons of characters for D the total length of the distinguishing
 * prefixes, against the \f$ O( n \log n ) \f$ comparisons of whole
 * strings of a quicksort, each as long as their common prefix. It sorts
 * in place, not stable.
 *
 * The contracts [see Contract.h]: the vector ends with the same strings
 * sorted, an audit. The function is defined in MultikeyQuickSort.tpp.
 *
 * \see J. L. Bentley, R. Sedgewick, Fast algorithms for sorting and
 *      searching strings, Proceedings of the 8th ACM-SIAM Symposium on
 *      Discrete Algorithms (SODA), 360-369, 1997
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

#ifndef __ADS_MultikeyQuickSort
 #define __ADS_MultikeyQuickSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the multikey quicksort of strings
/// \post A is sorted, with the same strings (an audit)
inline void multikeyQuickSort( std::vector< std::string > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/MultikeyQuickSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultikeyQuickSort.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File MultikeyQuickSort.h ----------------------*/
/*--------------------------------------------------------------------------*/
