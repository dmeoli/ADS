/*--------------------------------------------------------------------------*/
/*------------------------- File HeapByInsertion.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The max-heap on a vector built by insertions, the construction of
 * Williams, beside the bottom-up one of Floyd [see Heapsort.h].
 *
 * maxHeapSiftUp( A , i ) moves the item in A[ i ] up, swapping it with its
 * parent while it is larger, when A[ 0 .. i - 1 ] is a max-heap: the item
 * climbs at most the \f$ \lfloor \log_2( i + 1 ) \rfloor \f$ levels above
 * it, and A[ 0 .. i ] becomes a max-heap. It is the insertion into a heap
 * of a priority queue [see HeapPriorityQueue.h].
 *
 * heapBuildByInsertion( A ) makes A a max-heap by inserting its items one
 * at a time, i.e., by calling maxHeapSiftUp() on the places 1 to n - 1:
 * \f$ \Theta( n \log n ) \f$ time in the worst case, an increasing A, where
 * each item climbs to the root, against the O( n ) of heapBuild(), whose
 * items go down from the bottom, where most of them already are. On a
 * random permutation the insertions take O( n ) time on average as well
 * (about 2.28 n comparisons, against the 1.88 n of heapBuild(), after
 * Hayward and McDiarmid); the method is still the one to use when the
 * items arrive one by one and the heap must be valid after each of them.
 * The two constructions may give different heaps of the same items, e.g.,
 * [ 10 , 9 , 7 , 4 ] by insertions and [ 10 , 7 , 9 , 4 ] by heapBuild()
 * from [ 4 , 7 , 9 , 10 ].
 *
 * The contracts [see Contract.h]: maxHeapSiftUp() needs i < n and a heap
 * in A[ 0 .. i - 1 ] (an audit), and leaves a heap in A[ 0 .. i ] (an
 * audit); heapBuildByInsertion() ends with a heap (a postcondition). The
 * functions are defined in HeapByInsertion.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.512284
 * \see R. W. Floyd, Algorithm 245: Treesort 3, Communications of the ACM
 *      7(12), 701, 1964, doi:10.1145/355588.365103
 * \see R. Hayward, C. McDiarmid, Average case analysis of heap building by
 *      repeated insertion, Journal of Algorithms 12(1), 126-153, 1991,
 *      doi:10.1016/0196-6774(91)90027-V
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

#ifndef __ADS_HeapByInsertion
 #define __ADS_HeapByInsertion
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Heapsort.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/

/// maxHeapSiftUp(): A[ i ] goes up in the heap A[ 0 .. i ], with the order
/// less; O( log i ) time
/// \pre i < n and A[ 0 .. i - 1 ] is a max-heap (an audit)
/// \post A[ 0 .. i ] is a max-heap (an audit)
template< class T , class C = std::less<> >
void maxHeapSiftUp( std::vector< T > & A , std::size_t i , C less = C() );

/// heapBuildByInsertion(): A becomes a max-heap by inserting its items one
/// at a time; O( n log n ) time
/// \post A is a max-heap
template< class T , class C = std::less<> >
void heapBuildByInsertion( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/HeapByInsertion.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HeapByInsertion.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File HeapByInsertion.h -----------------------*/
/*--------------------------------------------------------------------------*/
