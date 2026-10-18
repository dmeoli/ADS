/*--------------------------------------------------------------------------*/
/*------------------------- File Rearrangements.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Rearrangements of a vector in place, by swaps or with a small buffer:
 * the partition into three classes, the alternating order, the sort of a
 * vector whose items are near their places, and the cost of sorting by
 * swaps.
 *
 * - dutchNationalFlag(): the items of colour 0, then those of colour 1,
 *   then those of colour 2, by Dijkstra's invariant A[ 0 .. i ) of colour
 *   0, A[ i .. j ) of colour 1, A[ j .. k ] not examined, A( k .. n ) of
 *   colour 2: each step examines A[ j ] and swaps it to its class, n
 *   colours read and at most n swaps, O( 1 ) space. It is the partition
 *   of quicksort with three ways, the keys smaller than, equal to and
 *   larger than the pivot [see EngineeredQuickSorts.h for the partition of
 *   Bentley and McIlroy, which moves the equal keys fewer times];
 * - wiggleSort(): A[ 0 ] <= A[ 1 ] >= A[ 2 ] <= A[ 3 ] >= ..., by a pass
 *   of swaps of neighbours: when A[ 0 .. i ] alternates, a swap of A[ i ]
 *   and A[ i + 1 ] when they are in the wrong order keeps the order with
 *   A[ i - 1 ] by transitivity; n - 1 comparisons. With distinct items the
 *   order is strict; the strict order with repeated items, when it
 *   exists, needs the median and a partition into three ways about it,
 *   still in \f$ O( n ) \f$ time;
 * - sortKSorted(): the sort of a vector each of whose items is at most k
 *   places away from its place in the sorted order: the item of the place
 *   i is the smallest of A[ i .. i + k ], hence a heap of k + 1 items
 *   gives each item in turn, \f$ O( n \log k ) \f$ time and \f$ O( k ) \f$
 *   space, against the \f$ O( n k ) \f$ of insertion sort [see
 *   InsertionSort.h]; \f$ \Omega( n \log k ) \f$ comparisons are needed,
 *   since about \f$ ( k! )^{n / k} \f$ orders are k-sorted;
 * - minimumSwapSort(): the sort of distinct items by swaps, the fewest
 *   swaps and the least total cost when a swap costs the sum of the two
 *   items swapped. Each cycle of the permutation that sorts A, of k > 1
 *   items, needs k - 1 swaps, hence n minus the number of cycles in all;
 *   its items, of sum s and minimum m, are sorted at the least cost
 *   \f$ \min( s + ( k - 2 ) m , s + m + ( k + 1 ) M ) \f$, M the minimum
 *   of A: the minimum of the cycle swapped with each of the others in
 *   turn, or M brought into the cycle by a swap with m, used in the same
 *   way, and swapped back. The two optima may come from different
 *   sequences of swaps; \f$ O( n \log n ) \f$ for the sort that gives the
 *   permutation.
 *
 * The contracts [see Contract.h]: the colours must be 0, 1 or 2, and come out
 * in order (an audit); wiggleSort() and sortKSorted() give a permutation of
 * the items (an audit) alternating and sorted; sortKSorted() needs a vector
 * k-sorted and minimumSwapSort() distinct items, audits. The functions are
 * defined in Rearrangements.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti, and the collection of
 *      exercises on the analysis of algorithms
 * \see E. W. Dijkstra, A Discipline of Programming, Prentice-Hall, 1976,
 *      chapter 14
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 1.3.3
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

#ifndef __ADS_Rearrangements
 #define __ADS_Rearrangements
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// dutchNationalFlag(): the items of A by colour( x ), 0 then 1 then 2,
/// with n reads of the colours and at most n swaps, counted in *swaps if
/// it is given; ( i , j ) with A[ 0 .. i ) of colour 0, A[ i .. j ) of
/// colour 1 and A[ j .. n ) of colour 2
/// \pre colour( x ) is 0, 1 or 2
/// \post the colours in order (an audit)
template< class T , class F >
std::pair< std::size_t , std::size_t > dutchNationalFlag(
                  std::vector< T > & A , F && colour ,
                  std::size_t * swaps = nullptr );

/// wiggleSort(): A[ 0 ] <= A[ 1 ] >= A[ 2 ] <= ..., by swaps of
/// neighbours; n - 1 comparisons
/// \post A alternates, and holds its items (an audit)
template< class T , class C = std::less<> >
void wiggleSort( std::vector< T > & A , C less = C() );

/// sortKSorted(): A sorted, each item at most k places away from its place
/// in the sorted order, by a heap of k + 1 items; O( n log k )
/// \pre A is k-sorted (an audit)
/// \post A sorted, a permutation of its items (an audit)
template< class T , class C = std::less<> >
void sortKSorted( std::vector< T > & A , std::size_t k , C less = C() );

/// minimumSwapSort(): the fewest swaps that sort A, and the least total
/// cost of the swaps that sort it, a swap costing the sum of its two
/// items, computed in the type W; O( n log n )
/// \pre the items of A are distinct (an audit)
template< class T , class W = T >
 requires std::is_arithmetic_v< T >
std::pair< std::size_t , W > minimumSwapSort( const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/Rearrangements.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Rearrangements.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Rearrangements.h ------------------------*/
/*--------------------------------------------------------------------------*/
