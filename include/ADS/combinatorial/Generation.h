/*--------------------------------------------------------------------------*/
/*--------------------------- File Generation.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The generation of the basic combinatorial objects of Knuth (7.2.1),
 * beyond the book [see Enumeration.h for the enumerations by backtrack of
 * chapter 16]: each function calls visit() on every object, in a given
 * order, and returns their number; a visit that returns true stops the
 * generation.
 *
 * - mixedRadix() (Algorithm 7.2.1.1M) the n-tuples with
 *   \f$ 0 \leq a_j < m_j \f$ in lexicographic order, and grayTuples()
 *   (Algorithm 7.2.1.1H, loopless) in the reflected mixed-radix Gray order,
 *   where two consecutive tuples differ by 1 in one place;
 * - permutationsLex() (Algorithm 7.2.1.2L, of Narayana Pandita) the
 *   permutations of a multiset in lexicographic order, plainChanges()
 *   (Algorithm 7.2.1.2P, Steinhaus-Johnson-Trotter) those of n distinct
 *   items where two consecutive ones differ by a swap of adjacent items,
 *   and heapPermutations() (Heap's method, 7.2.1.2G) by one swap each,
 *   with the least work per permutation;
 * - combinationsLex() (Algorithm 7.2.1.3L) the t-combinations of n in
 *   lexicographic order, and revolvingDoor() (Algorithm 7.2.1.3R) in the
 *   order where two consecutive ones differ by one item in and one out;
 * - integerPartitions() (Algorithm 7.2.1.4P) the partitions of n in reverse
 *   lexicographic order, and setPartitions() (Algorithm 7.2.1.5H) the
 *   partitions of a set of n by their restricted growth strings
 *   \f$ a_1 = 0 , a_{j+1} \leq 1 + \max( a_1 , \ldots , a_j ) \f$;
 * - nestedParentheses() (Algorithm 7.2.1.6P) the strings of n pairs of
 *   balanced parentheses in lexicographic order, i.e., the forests and the
 *   binary trees with n nodes.
 *
 * The counts are \f$ \prod m_j \f$, the multinomial, n!, \f$ \binom{n}{t}
 * \f$, p( n ), the Bell number \f$ \varpi_n \f$ and the Catalan number
 * \f$ C_n \f$, and the tests check them. The contracts [see Contract.h]:
 * the radices are positive, 0 <= t <= n, n >= 0. The functions are defined
 * in Generation.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A, Addison-Wesley,
 *      2011, section 7.2.1
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

#ifndef __ADS_Generation
 #define __ADS_Generation
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Tuple = std::vector< int >;   ///< a tuple, a permutation, a string

/// the tuples with 0 <= a_j < m_j, lexicographic
/// \pre each m_j >= 1
template< class F >
long mixedRadix( const std::vector< int > & m , F && visit );

/// the tuples with 0 <= a_j < m_j, reflected Gray order, loopless
/// \pre each m_j >= 2
template< class F >
long grayTuples( const std::vector< int > & m , F && visit );

/// the permutations of the multiset a, lexicographic from a sorted
template< class F >
long permutationsLex( Tuple a , F && visit );

/// the permutations of 0 , ... , n - 1 by adjacent swaps
/// \pre n >= 0
template< class F >
long plainChanges( int n , F && visit );

/// the permutations of 0 , ... , n - 1 by Heap's method, one swap each
/// \pre n >= 0
template< class F >
long heapPermutations( int n , F && visit );

/// the t-combinations of 0 , ... , n - 1, lexicographic, increasing tuples
/// \pre 0 <= t <= n
template< class F >
long combinationsLex( int n , int t , F && visit );

/// the t-combinations of 0 , ... , n - 1 in revolving-door order
/// \pre 0 <= t <= n
template< class F >
long revolvingDoor( int n , int t , F && visit );

/// the partitions of n, nonincreasing parts, reverse lexicographic
/// \pre n >= 0
template< class F >
long integerPartitions( int n , F && visit );

/// the restricted growth strings of the partitions of a set of n
/// \pre n >= 0
template< class F >
long setPartitions( int n , F && visit );

/// the strings of n pairs of balanced parentheses, lexicographic
/// \pre n >= 0
template< class F >
long nestedParentheses( int n , F && visit );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/Generation.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Generation.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Generation.h --------------------------*/
/*--------------------------------------------------------------------------*/
