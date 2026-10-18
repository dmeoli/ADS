/*--------------------------------------------------------------------------*/
/*------------------------- File SortedSearches.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Searches in sorted vectors beyond the place of a given item [see
 * BinarySearch.h]: the minimum of a rotated vector, the item closest to a
 * value, two items with a given sum, and the item of a given rank in the
 * union of two sorted vectors, whose middle ranks are the medians.
 *
 * - rotatedMinimum(): the place of the minimum of a strictly increasing
 *   vector rotated by an unknown number of places, which is that number;
 *   the minimum is in the right part of A[ lo .. hi ] if A[ m ] > A[ hi ],
 *   since then the rotation point is after m, and in A[ lo .. m ]
 *   otherwise; \f$ \lceil \log_2 n \rceil \f$ comparisons. With repeated
 *   items no search can beat \f$ \Omega( n ) \f$ comparisons in the worst
 *   case, as a vector of equal items but one shows, hence the items must
 *   be distinct;
 * - closestValue(): the place of an item at the least distance from x, the
 *   smaller one on ties, and its first place among equal items; the first
 *   item not smaller than x [see lowerBound()] and the one before it are
 *   the only candidates, \f$ O( \log n ) \f$. The distances are computed
 *   on the unsigned type of the same width for the integers, which cannot
 *   overflow; the test that compares | A[ m ] - x | with
 *   | A[ m + 1 ] - x | and goes right on ties misses a closer item on the
 *   left of a run of equal items, as 0 in [ 0 , 3 , 3 ] for x = 1;
 * - pairWithSum(): two places i < j with A[ i ] + A[ j ] = x, by two
 *   indices from the ends, the left one moving right when the sum is too
 *   small and the right one moving left otherwise; each step excludes all
 *   the pairs of the item left behind, \f$ O( n ) \f$ against the
 *   \f$ O( n \log n ) \f$ of a binary search for each item; a hash table
 *   does the same in \f$ O( n ) \f$ expected time on a vector that is not
 *   sorted. The sum is computed in the type of x, so that the items may be
 *   summed in a wider type;
 * - kthOfTwoSorted(): the item of rank k (from 0) of the multiset union of
 *   the sorted X and Y, by a binary search of the number i of items taken
 *   from the shorter vector among the k + 1 smallest, j = k + 1 - i being
 *   taken from the other one: i is right when
 *   \f$ X[ i - 1 ] \le Y[ j ] \f$ and \f$ Y[ j - 1 ] \le X[ i ] \f$;
 *   \f$ O( \log \min( m , n ) ) \f$ comparisons, against the
 *   \f$ O( k ) \f$ of a merge. The two medians of two sorted vectors of n
 *   items are the items of ranks n - 1 and n.
 *
 * The contracts [see Contract.h]: the vectors must be sorted, rotatedMinimum()
 * needs a rotation of a strictly increasing vector, all audits checked at the
 * level 3 since they cost O( n ); closestValue() and rotatedMinimum() need a
 * vector that is not empty, kthOfTwoSorted() a rank k < m + n, and their
 * results are checked against a scan of the vector, an audit. The functions
 * are defined in SortedSearches.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2 and
 *      Esercizi Divide-et-impera, Raccolte da compiti scritti
 * \see G. N. Frederickson, D. B. Johnson, The complexity of selection and
 *      ranking in X + Y and matrices with sorted columns, Journal of
 *      Computer and System Sciences 24(2), 197-208, 1982
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

#ifndef __ADS_SortedSearches
 #define __ADS_SortedSearches
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// rotatedMinimum(): the place of the minimum of A, a strictly increasing
/// vector rotated right by that number of places; O( log n )
/// \pre A is not empty, and a rotation of a strictly increasing vector
/// \post no item of A smaller than A[ r ] (an audit)
template< class T , class C = std::less<> >
std::size_t rotatedMinimum( const std::vector< T > & A , C less = C() );

/// closestValue(): the first place of an item of the sorted A at the least
/// distance from x, the smaller item on ties; O( log n )
/// \pre A is not empty and sorted
/// \post no item of A closer to x than A[ r ] (an audit)
template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::size_t closestValue( const std::vector< T > & A , const T & x );

/// pairWithSum(): two places i < j of the sorted A with A[ i ] + A[ j ] = x,
/// the sum computed in the type S, or nothing; O( n )
/// \pre A is sorted
/// \post A[ i ] + A[ j ] = x if there is a pair
template< class T , class S >
std::optional< std::pair< std::size_t , std::size_t > >
pairWithSum( const std::vector< T > & A , const S & x );

/// kthOfTwoSorted(): the item of rank k, from 0, of the sorted X and Y
/// together; O( log min( m , n ) )
/// \pre X and Y are sorted, k < m + n
/// \post the item of rank k of the merge of X and Y (an audit)
template< class T , class C = std::less<> >
T kthOfTwoSorted( const std::vector< T > & X , const std::vector< T > & Y ,
                  std::size_t k , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/SortedSearches.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortedSearches.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SortedSearches.h ------------------------*/
/*--------------------------------------------------------------------------*/
