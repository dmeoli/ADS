/*--------------------------------------------------------------------------*/
/*------------------------- File HeapByInsertion.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The max-heap on a vector built by insertions, the construction of
 * Williams, beside the bottom-up one of Floyd [see `Heapsort.h`].
 *
 * `maxHeapSiftUp( A , i )` moves the item in \f$ A[ i ] \f$ up, swapping it
 * with its parent while it is larger, when \f$ A[ 0 .. i - 1 ] \f$ is a
 * max-heap: the item climbs at most the
 * \f$ \lfloor \log_2( i + 1 ) \rfloor \f$ levels above it, and
 * \f$ A[ 0 .. i ] \f$ becomes a max-heap. It is the insertion into a heap of
 * a priority queue [see `HeapPriorityQueue.h`].
 *
 * `heapBuildByInsertion( A )` makes \f$ A \f$ a max-heap by inserting its
 * items one at a time, i.e., by calling `maxHeapSiftUp()` on the places 1 to
 * \f$ n - 1 \f$: \f$ \Theta( n \log n ) \f$ time in the worst case, an
 * increasing \f$ A \f$, where each item climbs to the root, against the
 * \f$ O( n ) \f$ of `heapBuild()`, whose items go down from the bottom,
 * where most of them already are. On a random permutation the insertions
 * take \f$ O( n ) \f$ time on average as well (about \f$ 2.28 n \f$
 * comparisons, against the \f$ 1.88 n \f$ of `heapBuild()`, after Hayward
 * and McDiarmid); the method is still the one to use when the items arrive
 * one by one and the heap must be valid after each of them. The two
 * constructions may give different heaps of the same items, e.g.,
 * \f$ [ 10 , 9 , 7 , 4 ] \f$ by insertions and \f$ [ 10 , 7 , 9 , 4 ] \f$ by
 * `heapBuild()` from \f$ [ 4 , 7 , 9 , 10 ] \f$.
 *
 * The contracts [see `Contract.h`]: `maxHeapSiftUp()` needs \f$ i < n \f$
 * and a heap in \f$ A[ 0 .. i - 1 ] \f$ (an audit), and leaves a heap in
 * \f$ A[ 0 .. i ] \f$ (an audit); `heapBuildByInsertion()` ends with a heap
 * (a postcondition). The functions are defined in `HeapByInsertion.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.3734138
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

/// \f$ A[ i ] \f$ up in the heap \f$ A[ 0 .. i ] \f$, with the order
/// `less`; \f$ O( \log i ) \f$ time
///
/// In pseudocode (section 10.1, the insertion into a heap):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{maxHeapSiftUp}( A , i ) \cr
/// 2 & \quad \textbf{while}\ i > 0\ \textbf{and}\ A[ \lfloor ( i - 1 ) / 2
///     \rfloor ] < A[ i ]\ \textbf{do} \cr
/// 3 & \qquad \text{swap } A[ i ] \text{ and } A[ \lfloor ( i - 1 ) / 2
///     \rfloor ];\ i \gets \lfloor ( i - 1 ) / 2 \rfloor
/// \end{array}}
/// \f]
/// \pre \f$ i < n \f$ and \f$ A[ 0 .. i - 1 ] \f$ is a max-heap (an
///      audit)
/// \post \f$ A[ 0 .. i ] \f$ is a max-heap (an audit)
template< class T , class C = std::less<> >
void maxHeapSiftUp( std::vector< T > & A , std::size_t i , C less = C() );

/// \f$ A \f$ made a max-heap by inserting its items one at a time;
/// \f$ O( n \log n ) \f$ time
///
/// In pseudocode (Williams, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{heapBuildByInsertion}( A ) \cr
/// 2 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n - 1\ \textbf{do}
///     \ \mathrm{maxHeapSiftUp}( A , i )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is a max-heap
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
