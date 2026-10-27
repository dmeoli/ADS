/*--------------------------------------------------------------------------*/
/*------------------------- File FastGeneration.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered generation of permutations and combinations, beyond the book
 * [see `Generation.h` for the algorithms of Knuth (section 7.2.1 of The Art of
 * Computer Programming) as written there]: the same objects, with the work per
 * object brought down to a few instructions, so that `visit()` is the larger
 * part of the time. Each function calls `visit()` on every object and returns
 * their number; if `visit()` returns `bool`, true stops the generation, and if
 * it returns `void` there is no test at all.
 *
 * -  `forEachPermutation( a , visit )` visits the \f$ n! \f$ arrangements of
 *   \f$ a \f$ (any items, the duplicates not merged) by Heap's method, one
 *   swap each: the arrangements of the first 3 items are visited by
 *   straight-line code (the 5 swaps of the places 01, 02, 01, 02, 01 of Heap's
 *   method on 3 items), and the counters \f$ c_i \f$ of the iterative method
 *   drive only the places \f$ i \ge 3 \f$, i.e., once every 6 objects; the
 *   cost is \f$ \sum_{k \ge 1} 1 / k! = e - 1 \f$ swaps per object.
 * -  `forEachPermutationLex( a , visit )` visits the distinct permutations of
 *   the multiset \f$ a \f$ in lexicographic order (as `std::next_permutation`
 *   from \f$ a \f$ sorted): Knuth's Algorithm 7.2.1.2L for the places up to
 *   \f$ n - 4 \f$, and the permutations of the last 3 items written directly,
 *   6 of them if they are distinct and 3 or 1 otherwise; the loops of L then
 *   run once every 6 objects instead of at each.
 * -  `forEachCombination( n , t , visit )` visits the increasing
 *   \f$ t \f$-tuples of \f$ [ 0 , n ) \f$ in lexicographic order: the last
 *   place runs through its values by a loop of its own, and the successor of
 *   Algorithm 7.2.1.3L moves the others only when it is over, once every
 *   \f$ n - c_{t-2} - 1 \f$ objects.
 * -  `forEachCombinationMask( n , t , visit )` visits the words of \f$ t \f$ 1
 *   bits below \f$ 2^n \f$, \f$ n \le 64 \f$, in increasing order, by
 *   `nextCombination()`, the trick of Gosper with the trailing zeros of the
 *   processor in place of the division of `nextSameWeight()` in `Bitwise.h`:
 *   \f[
 *     u = x \mid ( x - 1 ) , \quad
 *     x' = ( u + 1 ) \mid
 *          \big( ( ( \lnot u \,\&\, ( u + 1 ) ) - 1 ) \gg
 *                ( \rho( x ) + 1 ) \big) ,
 *   \f]
 *   \f$ \rho \f$ the trailing zeros; while the lowest 1 of \f$ x \f$ has a 0
 *   above it, below the place \f$ n - 1 \f$, the next word is
 *   \f$ x + ( x \,\&\, -x ) \f$, which a loop of its own does in two
 *   operations, so that the step above runs once per run of the lowest 1,
 *   i.e., for a fraction of about \f$ t / n \f$ of the words.
 *
 * The counts are \f$ n! \f$, the multinomial \f$ n! / \prod_v m_v! \f$ of the
 * multiplicities and \f$ \binom{n}{t} \f$, as for `Generation.h`. The
 * contracts [see `Contract.h`]: \f$ 0 \le t \le n \f$, \f$ n \le 64 \f$ for
 * the words. The functions are defined in `FastGeneration.tpp`.
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
 *         Università di Pisa \n
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
#include "ADS/combinatorial/Generation.h"

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

/// the \f$ n! \f$ arrangements of \f$ a \f$ by Heap's method, `visit( a )` on
/// each
///
/// In pseudocode (Heap's method with its first 3 places unrolled, beyond the
/// book), \f$ c_i \f$ the counter of the place \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{forEachPermutation}( a ) \cr
/// 2 & \quad \textbf{if}\ n < 3\ \textbf{then}\ \text{visit } a \text{, and }
///     a \text{ with } a_0 , a_1 \text{ swapped if } n = 2;\ \textbf{return}
///     \cr
/// 3 & \quad c \gets ( 0 , \ldots , 0 ) \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \text{visit } a \text{, then swap the places } 01 , 02 , 01 , 02
///     , 01 \text{ visiting } a \text{ after each}\quad \triangleright
///     \ \text{the 6 of } a_0 a_1 a_2 \cr
/// 6 & \qquad i \gets 3;\ \textbf{while}\ i < n\ \textbf{and}\ c_i \ge i
///     \ \textbf{do}\ c_i \gets 0;\ i \gets i + 1 \cr
/// 7 & \qquad \textbf{if}\ i \ge n\ \textbf{then return} \cr
/// 8 & \qquad \textbf{if}\ i \text{ is odd }\ \textbf{then}\ \text{swap }
///     a_{c_i} \text{ and } a_i\ \textbf{else}\ \text{swap } a_0 \text{ and }
///     a_i \cr
/// 9 & \qquad c_i \gets c_i + 1
/// \end{array}}
/// \f]
template< class T , class F >
long forEachPermutation( std::vector< T > & a , F && visit );

/// the distinct permutations of the multiset \f$ a \f$ in lexicographic order
/// from \f$ a \f$ sorted, `visit( a )` on each; \f$ a \f$ ends sorted
///
/// In pseudocode (Knuth's Algorithm 7.2.1.2L with its last 3 places unrolled,
/// beyond the book): the last 3 items \f$ x \le y \le z \f$ are increasing
/// after each step of L, which they never enter, since they end decreasing.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{forEachPermutationLex}( a ) \cr
/// 2 & \quad \text{sort } a;\ \textbf{if}\ n < 4\ \textbf{then}\ \text{run}
///     \text{ Algorithm L alone};\ \textbf{return} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad ( x , y , z ) \gets ( a_{n-3} , a_{n-2} , a_{n-1} ) \cr
/// 5 & \qquad \textbf{if}\ x < y < z\ \textbf{then}\ \text{visit } xyz , xzy
///     , yxz , yzx , zxy , zyx \text{ in the last 3 places} \cr
/// 6 & \qquad \textbf{else if}\ x = y < z\ \textbf{then}\ \text{visit } xxz ,
///     xzx , zxx \cr
/// 7 & \qquad \textbf{else if}\ x < y = z\ \textbf{then}\ \text{visit } xyy ,
///     yxy , yyx \cr
/// 8 & \qquad \textbf{else}\ \text{visit } xxx \cr
/// 9 & \qquad j \gets n - 4;\ \textbf{while}\ j \ge 0\ \textbf{and not}\ a_j <
///     a_{j+1}\ \textbf{do}\ j \gets j - 1 \cr
/// 10 & \qquad \textbf{if}\ j < 0\ \textbf{then}\ \text{reverse } a;
///      \ \textbf{return}\quad \triangleright\ \text{back to sorted} \cr
/// 11 & \qquad l \gets n - 1;\ \textbf{while not}\ a_j < a_l\ \textbf{do}\ l
///      \gets l - 1 \cr
/// 12 & \qquad \text{swap } a_j \text{ and } a_l;\ \text{reverse } a_{j+1} ,
///      \ldots , a_{n-1}
/// \end{array}}
/// \f]
template< class T , class F >
long forEachPermutationLex( std::vector< T > & a , F && visit );

/// the increasing \f$ t \f$-tuples of \f$ [ 0 , n ) \f$ in lexicographic
/// order, `visit( c )` on each, \f$ c \f$ a `const std::vector< int >` of
/// size \f$ t \f$
///
/// In pseudocode (the successor of Knuth's Algorithm 7.2.1.3L with a loop of
/// its own for the last place, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{forEachCombination}( n , t ) \cr
/// 2 & \quad c_j \gets j \text{ for } 0 \le j < t;\ \textbf{if}\ t = 0
///     \ \textbf{then}\ \mathrm{visit}( c );\ \textbf{return} \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{while}\ c_{t-1} < n\ \textbf{do}\ \mathrm{visit}( c );
///     \ c_{t-1} \gets c_{t-1} + 1 \cr
/// 5 & \qquad j \gets t - 2;\ \textbf{while}\ j \ge 0\ \textbf{and}\ c_j = n -
///     t + j\ \textbf{do}\ j \gets j - 1 \cr
/// 6 & \qquad \textbf{if}\ j < 0\ \textbf{then}\ c_{t-1} \gets n - 1;
///     \ \textbf{return} \cr
/// 7 & \qquad c_j \gets c_j + 1;\ c_k \gets c_{k-1} + 1 \text{ for } j < k < t
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le t \le n \f$
template< class F >
long forEachCombination( int n , int t , F && visit );

/// the next larger word with as many 1 bits as \f$ x \f$
///
/// In pseudocode (the trick of Gosper as in Hacker's Delight, beyond the
/// book), \f$ \rho \f$ the trailing zeros:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nextCombination}( x ) \cr
/// 2 & \quad u \gets x \mid ( x - 1 )\quad \triangleright\ \text{the trailing}
///     \text{ 0s set} \cr
/// 3 & \quad \textbf{return}\ ( u + 1 ) \mid ( ( ( \lnot u \,\&\, ( u + 1 ) )
///     - 1 ) \gg ( \rho( x ) + 1 ) )
/// \end{array}}
/// \f]
/// \pre \f$ x \ne 0 \f$ and the result fits in 64 bits
[[nodiscard]] constexpr Word nextCombination( Word x );

/// the words of \f$ t \f$ 1 bits below \f$ 2^n \f$ in increasing order,
/// `visit( x )` on each
///
/// In pseudocode (beyond the book), the lowest 1 moved up alone while it can,
/// and `nextCombination()` only at the end of its run:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{forEachCombinationMask}( n , t ) \cr
/// 2 & \quad \textbf{if}\ t = 0\ \textbf{then}\ \mathrm{visit}( 0 );
///     \ \textbf{return} \cr
/// 3 & \quad x \gets 2^t - 1;\ \mathit{last} \gets x \ll ( n - t );
///     \ \mathit{top} \gets 2^{n-1} \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \mathrm{visit}( x );\ \ell \gets x \,\&\, -x \cr
/// 6 & \qquad \textbf{while}\ \ell < \mathit{top}\ \textbf{and}\ x \,\&\, 2
///     \ell = 0\ \textbf{do}\ x \gets x + \ell;\ \ell \gets 2 \ell;
///     \ \mathrm{visit}( x ) \cr
/// 7 & \qquad \textbf{if}\ x = \mathit{last}\ \textbf{then return} \cr
/// 8 & \qquad x \gets \mathrm{nextCombination}( x )
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le t \le n \le 64 \f$
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
