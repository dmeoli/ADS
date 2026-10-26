/*--------------------------------------------------------------------------*/
/*------------------------- File SequenceQueries.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Questions on a sequence that a sort or a single scan answers faster than
 * the comparison of all the pairs: the smaller items on the right of each
 * item, the longest run with few distinct items, the longest stretch that
 * a set of intervals leaves uncovered, and the cheapest matching of two
 * sets of points on a line.
 *
 * - smallerToTheRight(): for each place i, the number of places j > i with
 *   A[ j ] < A[ i ], by mergesort on the places [see MergeSort.h]: when an
 *   item of the left half goes to the output, the items of the right half
 *   gone before it are exactly the smaller ones on its right in that
 *   half, and the halves below added the others; \f$ O( n \log n ) \f$
 *   time and \f$ O( n ) \f$ space, against the \f$ \Theta( n^2 ) \f$ of
 *   the double loop. Equal items do not count, since the left one goes
 *   first. The sum of the counts is the number of inversions [see
 *   countInversions()]; a Fenwick tree on the ranks gives the same counts
 *   in \f$ O( n \log n ) \f$ by a scan from the right;
 * - longestKDistinctRun(): the first longest run of consecutive items with
 *   at most k distinct values, by a window whose right end advances one
 *   place at a time and whose left end advances while the window holds
 *   more than k values, with the counts of its values in a search tree;
 *   each end moves n times, \f$ O( n \log k ) \f$;
 * - largestGap(): the longest stretch between the first start and the
 *   last end of a set of closed intervals [ a , b ] covered by none of
 *   them, the first one on ties: the intervals by start, and a gap
 *   wherever a start is beyond the farthest end so far, the union of
 *   intervals by a sweep; \f$ O( n \log n ) \f$;
 * - lineAssignment(): the least sum of \f$ | h_i - l_{\pi( i )} | \f$ over
 *   the matchings \f$ \pi \f$ of two sets of n points on a line, both
 *   sorted and matched in order: if \f$ h_1 \le h_2 \f$ and
 *   \f$ l_1 \le l_2 \f$ the crossed matching never costs less than the
 *   straight one, an exchange argument; \f$ O( n \log n ) \f$, against the
 *   \f$ O( n^3 ) \f$ of the Hungarian method on general costs. The same
 *   order is optimal for any convex function of the difference, the
 *   optimal transport on the line.
 *
 * The contracts [see Contract.h]: the counts of smallerToTheRight() sum to
 * the inversions of A (an audit); the run of longestKDistinctRun() has at
 * most k values (an audit); the intervals of largestGap() must have a <= b,
 * and its gap meets none of them (an audit); lineAssignment() needs two sets
 * of the same size. The functions are defined in SequenceQueries.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
 * \see H. W. Kuhn, The Hungarian method for the assignment problem, Naval
 *      Research Logistics Quarterly 2(1-2), 83-97, 1955,
 *      doi:10.1002/nav.3800020109
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

#ifndef __ADS_SequenceQueries
 #define __ADS_SequenceQueries
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// smallerToTheRight(): c[ i ], the number of j > i with A[ j ] < A[ i ],
/// by mergesort on the places; O( n log n )
/// \post the sum of c is the number of inversions of A (an audit)
template< class T , class C = std::less<> >
std::vector< std::size_t > smallerToTheRight( const std::vector< T > & A ,
                                              C less = C() );

/// longestKDistinctRun(): the start and the length of the first longest run
/// of consecutive items of S with at most k distinct values, by a sliding
/// window; O( n log k )
/// \post the run has at most k distinct values (an audit)
template< std::ranges::random_access_range R >
std::pair< std::size_t , std::size_t > longestKDistinctRun( const R & S ,
                                                            std::size_t k );

/// largestGap(): the longest stretch ( x , y ), between the first start and
/// the last end, covered by none of the closed intervals I, the first one
/// on ties, or nothing; O( n log n )
/// \pre a <= b for each interval [ a , b ]
/// \post no interval meets ( x , y ) (an audit)
template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::optional< std::pair< T , T > > largestGap(
                                       std::vector< std::pair< T , T > > I );

/// lineAssignment(): the least sum of | H[ i ] - L[ p( i ) ] | over the
/// permutations p, computed in the type W, by sorting; O( n log n )
/// \pre H and L have the same size
template< class T , class W = T >
 requires std::is_arithmetic_v< T >
W lineAssignment( std::vector< T > H , std::vector< T > L );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/SequenceQueries.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceQueries.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SequenceQueries.h -----------------------*/
/*--------------------------------------------------------------------------*/
