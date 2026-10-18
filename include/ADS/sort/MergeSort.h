/*--------------------------------------------------------------------------*/
/*---------------------------- File MergeSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Merge Sort (section 2.7.3): the two halves of the vector are sorted
 * recursively and merged by Merge() through a vector B allocated once,
 * \f$ T( n ) = 2 T( n / 2 ) + \Theta( n ) = \Theta( n \log n ) \f$ time,
 * optimal among the sorts by comparisons, \f$ \Theta( n ) \f$ extra space,
 * stable (an item of the left part goes first when equal). When the right
 * part runs out first, the rest of the left one is copied; when the left
 * one does, the rest of the right one is already in place.
 *
 * - mergeSort( A , cutoff ) sorts the parts of at most cutoff items with
 *   insertionSort(), which is faster on few items (section 2.8);
 * - mergeSortBottomUp() merges the runs of length 1 , 2 , 4 , ... with no
 *   recursion and no stack (exercise 12.5);
 * - mergeSort( L ) sorts a LIST with pointers in O( n log n ) by splitting
 *   it in two halves, sorting them and merging them (exercise 12.1), only
 *   through the operations of the SEQUENCE: each item is copied once per
 *   level of the recursion, since the abstract data type moves no cells
 *   from a list to another;
 * - countInversions() counts the pairs i < j with A[ i ] > A[ j ]: when
 *   the merge takes A[ j ] from the right part, it is smaller than all the
 *   mid - i + 1 items left in the left part (exercise 12.3).
 *
 * The contracts [see Contract.h]: mergeParts() needs start <= mid < end < n, a
 * buffer B as large as A and two sorted halves, the latter an audit;
 * mergeSortRec() needs start <= end < n and cutoff >= 1; the sorts end with
 * the same items sorted, an audit. The functions are defined in MergeSort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, Sorting and
 *      Searching, 2nd ed., Addison-Wesley, 1998, section 5.2.4
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

namespace ADS {

/// Merge() of the book: A[ start .. mid ] and A[ mid + 1 .. end ], sorted,
/// merged through B; the inversions met are added to inv
/// \pre start <= mid < end < n <= | B |, A[ start .. mid ] and
///      A[ mid + 1 .. end ] sorted
/// \post A[ start .. end ] sorted
template< class T , class C = std::less<> >
void mergeParts( std::vector< T > & A , std::vector< T > & B ,
                 std::size_t start , std::size_t mid , std::size_t end ,
                 C less = C() , long * inv = nullptr );

/// MergeSortRec() of A[ start .. end ], insertionSort() below cutoff items;
/// with cutoff > 1 also no merge of two parts already in order, i.e., with
/// A[ mid ] <= A[ mid + 1 ], which makes the sort of a sorted vector take
/// n - 1 comparisons
/// \pre start <= end < n <= | B | and cutoff >= 1
template< class T , class C = std::less<> >
void mergeSortRec( std::vector< T > & A , std::vector< T > & B ,
                   std::size_t start , std::size_t end , std::size_t cutoff ,
                   C less = C() , long * inv = nullptr );

/// MergeSort(): A in increasing order, stable; parts of at most cutoff
/// items by insertionSort() (1 is the book)
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void mergeSort( std::vector< T > & A , std::size_t cutoff = 1 ,
                C less = C() );

/// the iterative Merge Sort: the runs of length w are merged in pairs, for
/// w = 1 , 2 , 4 , ...
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void mergeSortBottomUp( std::vector< T > & A , C less = C() );

/// the number of inversions of A, which ends sorted, O( n log n )
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
long countInversions( std::vector< T > & A , C less = C() );

/// Merge Sort of a list with pointers: the cells are moved, not the items
/// \post L is sorted
template< class T , class C = std::less<> >
void mergeSort( List< T > & L , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/MergeSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MergeSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MergeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
