/*--------------------------------------------------------------------------*/
/*---------------------------- File Heapsort.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The max-heap on a vector and heapsort (section 10.1.4).
 *
 * maxHeapRestore( A , i , dim ) moves the item in A[ i ] down, swapping it
 * with its largest child, when the subtrees of the children are max-heaps
 * in A[ 0 .. dim - 1 ]: O( m ) time on a node of height m. heapBuild()
 * calls it from \f$ \lfloor n / 2 \rfloor - 1 \f$ down to 0, the places
 * from \f$ \lfloor n / 2 \rfloor \f$ on being leaves; since a heap of n
 * items has at most \f$ \lceil n / 2^{m+1} \rceil \f$ nodes of height m,
 * its time is
 * \f[
 *   O\left( \sum_{m=0}^{\lfloor \log n \rfloor} \frac{n}{2^{m+1}} m
 *   \right) = O\left( n \sum_{m=0}^{\infty} \frac{m}{2^m} \right) = O( n ),
 * \f]
 * the series being 2. heapsort() builds the heap and then swaps the maximum
 * A[ 0 ] with the last place of the heap, which shrinks by one, and
 * restores it, in \f$ O( n \log n ) \f$ time and O( 1 ) extra space, since
 * maxHeapRestore() is a loop rather than the recursion of the book
 * (exercise 10.4).
 *
 * The contracts [see Contract.h]: maxHeapRestore() and isMaxHeap() need dim <=
 * n, maxHeapRestore() also i < dim, and heapBuild() ends with a heap and
 * heapsort() with the same items sorted, two audits. The functions are defined
 * in Heapsort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.512284
 * \see R. W. Floyd, Algorithm 245: Treesort 3, Communications of the ACM
 *      7(12), 701, 1964, doi:10.1145/355588.365103
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

#ifndef __ADS_Heapsort
 #define __ADS_Heapsort
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

/*--------------------------------------------------------------------------*/

/// maxHeapRestore(): A[ i ] goes down in the heap A[ 0 .. dim - 1 ], with
/// the order less, O( 1 ) extra space
/// \pre i < dim <= n
template< class T , class C = std::less<> >
void maxHeapRestore( std::vector< T > & A , std::size_t i , std::size_t dim ,
                     C less = C() );

/// heapBuild(): A becomes a max-heap, O( n ) time
/// \post A is a max-heap
template< class T , class C = std::less<> >
void heapBuild( std::vector< T > & A , C less = C() );

/// heapsort(): A in increasing order, O( n log n ) time
/// \post A is sorted, with the same items
template< class T , class C = std::less<> >
void heapsort( std::vector< T > & A , C less = C() );

/// true if A[ 0 .. dim - 1 ] is a max-heap
/// \pre dim <= n
template< class T , class C = std::less<> >
bool isMaxHeap( const std::vector< T > & A , std::size_t dim , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/Heapsort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Heapsort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Heapsort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
