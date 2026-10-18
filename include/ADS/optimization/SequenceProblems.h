/*--------------------------------------------------------------------------*/
/*------------------------ File SequenceProblems.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Problems of optimization on a sequence, solved by a scan or by the
 * dynamic programming of the longest increasing subsequence [see
 * Subsequences.h]:
 *
 * - trappingRainWater(): the water held by columns of heights h_i after
 *   the rain, \f$ \sum_i \min( \max_{j \leq i} h_j , \max_{j \geq i} h_j )
 *   - h_i \f$; two indices from the ends, the lower side moving inwards,
 *   since its level is fixed by its own maximum: \f$ \Theta( n ) \f$ time
 *   and O( 1 ) space, instead of the two arrays of the maxima or the
 *   stack of the decreasing columns.
 * - largestDivisibleSubset(): the largest subset of distinct positive
 *   integers in which of any two one divides the other: on the items
 *   sorted, a chain in the order of divisibility, \f$ L[ i ] = 1 +
 *   \max\{ L[ j ] : j < i , a_j \mid a_i \} \f$, right by the transitivity
 *   of divisibility; \f$ \Theta( n^2 ) \f$ time.
 * - maxSumIncreasingSubsequence(): the largest sum of a strictly
 *   increasing subsequence, the heaviest increasing subsequence with the
 *   items as weights: \f$ D[ i ] = a_i + \max\{ D[ j ] : j < i ,
 *   a_j < a_i \} \f$, the maximum over the smaller values from a Fenwick
 *   tree of the prefix maxima on the ranks, \f$ O( n \log n ) \f$ time
 *   instead of the \f$ \Theta( n^2 ) \f$ scan.
 * - longestChainSubsequence(): the longest subsequence of S each item of
 *   which precedes the next in a strict partial order, e.g. boxes each
 *   fitting in the next, \f$ L[ j ] = 1 + \max\{ L[ i ] : i < j ,
 *   S_i \prec S_j \} \f$, \f$ \Theta( n^2 ) \f$ comparisons; for a total
 *   order it is the longest increasing subsequence, in O( n log n ). In
 *   d dimensions, the boxes free to be sorted and rotated give the box
 *   stacking problem, the longest path in the DAG of the dominance.
 * - longestDistinctSegment(): the longest segment of A with no value
 *   repeated, by a window whose left end jumps past the last occurrence of
 *   a repeated value, with a hash table of the last occurrences;
 *   \f$ \Theta( n ) \f$ expected time.
 *
 * The contracts [see Contract.h]: the items of largestDivisibleSubset()
 * positive and distinct, and the subset divisible; the subsequences found
 * increasing in the given order. The functions are defined in
 * SequenceProblems.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see G. Jacobson, K.-P. Vo, Heaviest increasing/common subsequence
 *      problems, Proc. 3rd Symposium on Combinatorial Pattern Matching,
 *      LNCS 644, 52-66, 1992, doi:10.1007/3-540-56024-6_5
 * \see P. M. Fenwick, A new data structure for cumulative frequency
 *      tables, Software: Practice and Experience 24(3), 327-336, 1994,
 *      doi:10.1002/spe.4380240306
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

#ifndef __ADS_SequenceProblems
 #define __ADS_SequenceProblems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the water held by the columns of heights h, no walls outside
/// \post not negative
template< class T >
T trappingRainWater( const std::vector< T > & h );

/// a largest subset of A in which of any two items one divides the other,
/// sorted
/// \pre A positive and distinct
/// \post each item divides the next
template< std::integral T >
std::vector< T > largestDivisibleSubset( std::vector< T > A );

/// the largest sum of a strictly increasing subsequence of A, 0 for the
/// empty one
/// \post not negative
template< class T >
T maxSumIncreasingSubsequence( const std::vector< T > & A );

/// the places of a longest subsequence of S each item of which precedes the
/// next by less, a strict partial order
/// \post increasing places, less on each two consecutive items
template< class T , class Less >
std::vector< std::size_t > longestChainSubsequence(
                                    const std::vector< T > & S , Less less );

/// the start and the length of the first longest segment of A with no
/// value repeated
/// \post the values of the segment distinct
template< class T , class Hash = std::hash< T > >
std::pair< std::size_t , std::size_t > longestDistinctSegment(
                                                  const std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/SequenceProblems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceProblems.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File SequenceProblems.h -----------------------*/
/*--------------------------------------------------------------------------*/
