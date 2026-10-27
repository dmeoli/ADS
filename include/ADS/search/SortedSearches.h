/*--------------------------------------------------------------------------*/
/*------------------------- File SortedSearches.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Searches in sorted vectors beyond the place of a given item [see
 * `BinarySearch.h`]: the minimum of a rotated vector, the item closest to a
 * value, two items with a given sum, and the item of a given rank in the
 * union of two sorted vectors, whose middle ranks are the medians.
 *
 * - `rotatedMinimum()`: the place of the minimum of a strictly increasing
 *   vector rotated by an unknown number of places, which is that number;
 *   the minimum is in the right part of the range
 *   \f$ A[ \mathit{lo} .. \mathit{hi} ] \f$ if
 *   \f$ A[ m ] > A[ \mathit{hi} ] \f$, since then the rotation point is
 *   after \f$ m \f$, and in \f$ A[ \mathit{lo} .. m ] \f$ otherwise;
 *   \f$ \lceil \log_2 n \rceil \f$ comparisons. With repeated items no
 *   search can beat \f$ \Omega( n ) \f$ comparisons in the worst case, as a
 *   vector of equal items but one shows; hence the items must be distinct;
 * - `closestValue()`: the place of an item at the least distance from
 *   \f$ x \f$, the smaller one on ties, and its first place among equal
 *   items; the first item not smaller than \f$ x \f$ [see `lowerBound()`]
 *   and the one before it are the only candidates, \f$ O( \log n ) \f$. The
 *   distances are computed on the unsigned type of the same width for the
 *   integers, which cannot overflow; the test that compares
 *   \f$ | A[ m ] - x | \f$ with \f$ | A[ m + 1 ] - x | \f$ and goes right on
 *   ties misses a closer item on the left of a run of equal items, as 0 in
 *   \f$ ( 0 , 3 , 3 ) \f$ for \f$ x = 1 \f$;
 * - `pairWithSum()`: two places \f$ i < j \f$ with
 *   \f$ A[ i ] + A[ j ] = x \f$, by two indices from the ends, the left one
 *   moving right when the sum is too small and the right one moving left
 *   otherwise; each step excludes all the pairs of the item left behind,
 *   \f$ O( n ) \f$ against the \f$ O( n \log n ) \f$ of a binary search for
 *   each item; a hash table does the same in \f$ O( n ) \f$ expected time on
 *   a vector that is not sorted. The sum is computed in the type of
 *   \f$ x \f$, so that the items may be summed in a wider type;
 * - `kthOfTwoSorted()`: the item of rank \f$ k \f$ (from 0) of the multiset
 *   union of the sorted \f$ X \f$ and \f$ Y \f$, by a binary search of the
 *   number \f$ i \f$ of items taken from the shorter vector among the
 *   \f$ k + 1 \f$ smallest, \f$ j = k + 1 - i \f$ being taken from the other
 *   one: \f$ i \f$ is right when \f$ X[ i - 1 ] \le Y[ j ] \f$ and
 *   \f$ Y[ j - 1 ] \le X[ i ] \f$; \f$ O( \log \min( m , n ) ) \f$
 *   comparisons, against the \f$ O( k ) \f$ of a merge. The two medians of
 *   two sorted vectors of \f$ n \f$ items are the items of ranks
 *   \f$ n - 1 \f$ and \f$ n \f$.
 *
 * The contracts [see `Contract.h`]: the vectors must be sorted,
 * `rotatedMinimum()` needs a rotation of a strictly increasing vector, all
 * audits checked at the level 3 since they cost \f$ O( n ) \f$;
 * `closestValue()` and `rotatedMinimum()` need a vector that is not empty,
 * `kthOfTwoSorted()` a rank \f$ k < m + n \f$, and their results are checked
 * against a scan of the vector, an audit. The functions are defined in
 * `SortedSearches.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 1.2, and
 *      Esercizi Divide-et-impera and Esercizi vari, Raccolte da compiti
 *      scritti
 * \see G. N. Frederickson, D. B. Johnson, The complexity of selection and
 *      ranking in X + Y and matrices with sorted columns, Journal of
 *      Computer and System Sciences 24(2), 197-208, 1982,
 *      doi:10.1016/0022-0000(82)90048-4
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

/// the place of the minimum of \f$ A \f$, a strictly increasing vector
/// rotated right by that number of places; \f$ O( \log n ) \f$
///
/// In pseudocode (Esercizi Divide-et-impera of the book), the minimum in
/// \f$ A[ \mathit{lo} .. \mathit{hi} ] \f$ at all times:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rotatedMinimum}( A ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n - 1 \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad m \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor \cr
/// 5 & \qquad \textbf{if}\ A[ \mathit{hi} ] < A[ m ]\ \textbf{then}
///     \ \mathit{lo} \gets m + 1\ \textbf{else}\ \mathit{hi} \gets m \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty, and a rotation of a strictly increasing
///      vector
/// \post no item of \f$ A \f$ smaller than \f$ A[ r ] \f$ (audit)
template< class T , class C = std::less<> >
std::size_t rotatedMinimum( const std::vector< T > & A , C less = C() );

/// the first place of an item of the sorted \f$ A \f$ at the least distance
/// from \f$ x \f$, the smaller item on ties; \f$ O( \log n ) \f$
///
/// In pseudocode (Esercizi Divide-et-impera of the book), the distances
/// \f$ | a - b | \f$ computed with no overflow:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{closestValue}( A , x ) \cr
/// 2 & \quad r \gets \mathrm{lowerBound}( A , x ) \cr
/// 3 & \quad \textbf{if}\ r = n\ \textbf{then return}\ \mathrm{lowerBound}(
///     A , A[ n - 1 ] ) \cr
/// 4 & \quad \textbf{if}\ r > 0\ \textbf{and}\ x - A[ r - 1 ] \le A[ r ] -
///     x\ \textbf{then return}\ \mathrm{lowerBound}( A , A[ r - 1 ] ) \cr
/// 5 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not empty and sorted
/// \post no item of \f$ A \f$ closer to \f$ x \f$ than \f$ A[ r ] \f$
///       (audit)
template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::size_t closestValue( const std::vector< T > & A , const T & x );

/// two places \f$ i < j \f$ of the sorted \f$ A \f$ with
/// \f$ A[ i ] + A[ j ] = x \f$, the sum computed in the type `S`, or
/// nothing; \f$ O( n ) \f$
///
/// In pseudocode (Esercizi vari of the book, on one vector instead of two),
/// no pair out of \f$ A[ i .. j ] \f$ having the sum \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pairWithSum}( A , x ) \cr
/// 2 & \quad i \gets 0;\ j \gets n - 1 \cr
/// 3 & \quad \textbf{while}\ i < j\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ A[ i ] + A[ j ] = x\ \textbf{then return}\ ( i ,
///     j ) \cr
/// 5 & \qquad \textbf{else if}\ A[ i ] + A[ j ] < x\ \textbf{then}\ i \gets
///     i + 1 \cr
/// 6 & \qquad \textbf{else}\ j \gets j - 1 \cr
/// 7 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted
/// \post \f$ A[ i ] + A[ j ] = x \f$ if there is a pair
template< class T , class S >
std::optional< std::pair< std::size_t , std::size_t > >
pairWithSum( const std::vector< T > & A , const S & x );

/// the item of rank \f$ k \f$, from 0, of the sorted \f$ X \f$ and \f$ Y \f$
/// together; \f$ O( \log \min( m , n ) ) \f$
///
/// In pseudocode (Frederickson and Johnson, beyond the book), \f$ S \f$ the
/// shorter of the two vectors and \f$ L \f$ the other one, of
/// \f$ m \le n \f$ items, and \f$ i \f$ the items of \f$ S \f$ among the
/// \f$ k + 1 \f$ smallest:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kthOfTwoSorted}( X , Y , k ) \cr
/// 2 & \quad \mathit{lo} \gets \max\{ 0 , k + 1 - n \};\ \mathit{hi} \gets
///     \min\{ k + 1 , m \} \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad i \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor;\ j \gets k + 1 - i \cr
/// 5 & \qquad \textbf{if}\ S[ i ] < L[ j - 1 ]\ \textbf{then}\ \mathit{lo}
///     \gets i + 1\ \textbf{else}\ \mathit{hi} \gets i \cr
/// 6 & \quad i \gets \mathit{lo};\ j \gets k + 1 - i \cr
/// 7 & \quad \textbf{return}\ \text{the larger of } S[ i - 1 ] \text{ and }
///     L[ j - 1 ] \text{, those that exist}
/// \end{array}}
/// \f]
/// \pre \f$ X \f$ and \f$ Y \f$ are sorted, \f$ k < m + n \f$
/// \post the item of rank \f$ k \f$ of the merge of \f$ X \f$ and \f$ Y \f$
///       (audit)
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
