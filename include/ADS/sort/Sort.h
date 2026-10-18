/*--------------------------------------------------------------------------*/
/*------------------------------ File Sort.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort that chooses its algorithm, beyond the book: sort() and
 * stableSort() look at what they sort and pick, among the sorts of ADS, the
 * one that suits it.
 *
 * - With the natural order (std::less) on integers or floating point
 *   numbers, a short vector goes to pdqSort() and a long one, unless a
 *   first pass finds it already sorted or strictly decreasing (and then
 *   reverses it), to radixSort() [see RadixSort.h], which makes no
 *   comparison and takes
 *   O( w n ) for keys of w bytes, skipping the bytes that all the keys
 *   share; radixSort() is stable, so stableSort() uses it too.
 * - With the natural order on strings, both use msdRadixSort() [see
 *   RadixSort.h], which reads one character at a time instead of comparing
 *   whole strings and, on the benchmark, beats the in-place
 *   multikeyQuickSort() [see MultikeyQuickSort.h] (two equal strings cannot
 *   be told apart, so that stability means nothing there).
 * - Otherwise, i.e., with any other order or item, sort() uses pdqSort()
 *   [see EngineeredQuickSorts.h], \f$ O( n \log n ) \f$ in the worst case,
 *   O( n ) on a sorted or reversed vector and \f$ O( n \log k ) \f$ with
 *   k distinct keys, and stableSort() uses powerSort() [see RunSorts.h],
 *   O( n ) on few runs and nearly optimal on any number of them.
 *
 * The thresholds between the algorithms, kRadix items for the numbers and
 * kStrings for the strings, are measured by the benchmark of ADS
 * [see bench/sort.cpp] against std::sort, std::stable_sort and the sorts
 * of Boost; they are constants of the namespace, so that a program may
 * read them.
 *
 * The contracts [see Contract.h]: the vector ends with the same items
 * sorted, an audit of the sort chosen. The functions are defined in
 * Sort.tpp.
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

#ifndef __ADS_Sort
 #define __ADS_Sort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the items from which the numbers go to radixSort()
inline constexpr std::size_t kRadix = 512;

/// the items from which the strings go to the sorts by characters
inline constexpr std::size_t kStrings = 64;

/// the sort chosen for A and less, not necessarily stable
/// \post A is sorted, with the same items (an audit)
template< class T , class C = std::less<> >
void sort( std::vector< T > & A , C less = C() );

/// the stable sort chosen for A and less
/// \post A is sorted, with the same items, the equal ones in their order
///       (an audit of the order)
template< class T , class C = std::less<> >
void stableSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Sort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Sort.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Sort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
