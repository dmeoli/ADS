/*--------------------------------------------------------------------------*/
/*------------------------- File FastGeneration.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered generation of permutations and combinations, beyond the
 * book [see Generation.h for the algorithms of Knuth (7.2.1) as written
 * there]: the same objects, with the work per object brought down to a few
 * instructions, so that visit() is the larger part of the time. Each
 * function calls visit() on every object and returns their number; if
 * visit() returns bool, true stops the generation, and if it returns void
 * there is no test at all.
 *
 * - forEachPermutation( a , visit ) visits the n! arrangements of a (any
 *   items, the duplicates not merged) by Heap's method, one swap each: the
 *   arrangements of the first 3 items are visited by straight-line code
 *   (the 5 swaps 01, 02, 01, 02, 01 of Heap's method on 3 items), and the
 *   counters \f$ c_i \f$ of the iterative method drive only the places
 *   \f$ i \geq 3 \f$, i.e., once every 6 objects; the cost is
 *   \f$ \sum_{k \geq 1} 1 / k! = e - 1 \f$ swaps per object.
 * - forEachPermutationLex( a , visit ) visits the distinct permutations of
 *   the multiset a in lexicographic order (as std::next_permutation from
 *   the sorted a): Algorithm 7.2.1.2L of Knuth for the places up to n - 4,
 *   and the permutations of the last 3 items written directly, 6 of them
 *   if they are distinct and 3 or 1 otherwise; the loops of L then run
 *   once every 6 objects instead of at each.
 * - forEachCombination( n , t , visit ) visits the increasing t-tuples of
 *   [ 0 , n ) in lexicographic order: the last place runs through its
 *   values by a loop of its own, and Algorithm 7.2.1.3L moves the others
 *   only when it is over, once every n - c_{t-2} - 1 objects.
 * - forEachCombinationMask( n , t , visit ) visits the words of t 1 bits
 *   below \f$ 2^n \f$, n <= 64, in increasing order, by nextCombination(),
 *   the trick of Gosper with the trailing zeros of the processor in place
 *   of the division of nextSameWeight() in Bitwise.h:
 *   \f[
 *     u = x \mid ( x - 1 ) , \quad
 *     x' = ( u + 1 ) \mid
 *          \big( ( ( \lnot u \,\&\, ( u + 1 ) ) - 1 ) \gg
 *                ( \rho( x ) + 1 ) \big) ,
 *   \f]
 *   \f$ \rho \f$ the trailing zeros; while the lowest 1 of x has a 0 above
 *   it, below the place n - 1, the next word is \f$ x + ( x \,\&\, -x ) \f$,
 *   which a loop of its own does in two operations, so that the step above
 *   runs once per run of the lowest 1, i.e., for a fraction of about
 *   t / n of the words.
 *
 * The counts are n!, the multinomial \f$ n! / \prod_v m_v! \f$ of the
 * multiplicities and \f$ \binom{n}{t} \f$, as for Generation.h. The
 * contracts [see Contract.h]: 0 <= t <= n, n <= 64 for the words. The
 * functions are defined in FastGeneration.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A, Addison-Wesley,
 *      2011, sections 7.2.1.2 and 7.2.1.3
 * \see B. R. Heap, Permutations by interchanges, The Computer Journal 6(3),
 *      293-298, 1963, doi:10.1093/comjnl/6.3.293
 * \see R. Sedgewick, Permutation generation methods, ACM Computing Surveys
 *      9(2), 137-164, 1977, doi:10.1145/356689.356692
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012,
 *      section 2-1 (the next word with as many 1 bits)
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

#ifndef __ADS_FastGeneration
 #define __ADS_FastGeneration
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/combinatorial/Bitwise.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the n! arrangements of a by Heap's method, visit( a ) on each
template< class T , class F >
long forEachPermutation( std::vector< T > & a , F && visit );

/// the distinct permutations of the multiset a in lexicographic order
/// from the sorted a, visit( a ) on each; a ends sorted
template< class T , class F >
long forEachPermutationLex( std::vector< T > & a , F && visit );

/// the increasing t-tuples of [ 0 , n ) in lexicographic order,
/// visit( c ) on each, c a const std::vector< int > of size t
/// \pre 0 <= t <= n
template< class F >
long forEachCombination( int n , int t , F && visit );

/// the next larger word with as many 1 bits as x
/// \pre x != 0 and the result fits in 64 bits
[[nodiscard]] constexpr Word nextCombination( Word x );

/// the words of t 1 bits below 2^n in increasing order, visit( x ) on
/// each
/// \pre 0 <= t <= n <= 64
template< class F >
long forEachCombinationMask( int n , int t , F && visit );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/impl/FastGeneration.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FastGeneration.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File FastGeneration.h ------------------------*/
/*--------------------------------------------------------------------------*/
