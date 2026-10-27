/*--------------------------------------------------------------------------*/
/*--------------------------- File Generation.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The generation of the basic combinatorial objects of Knuth (section 7.2.1 of
 * The Art of Computer Programming), beyond the book [see `Enumeration.h` for
 * the enumerations by backtrack of chapter 16]: each function calls `visit()`
 * on every object, in a given order, and returns their number; a visit that
 * returns true stops the generation, and one that returns `void` never does
 * (as in `FastGeneration.h`).
 *
 * - `mixedRadix()` (Algorithm 7.2.1.1M) the \f$ n \f$-tuples with
 *   \f$ 0 \le a_j < m_j \f$ in lexicographic order, and `grayTuples()`
 *   (Algorithm 7.2.1.1H, loopless) in the reflected mixed-radix Gray order,
 *   where two consecutive tuples differ by 1 in one place;
 * - `permutationsLex()` (Algorithm 7.2.1.2L, of Narayana Pandita) the
 *   permutations of a multiset in lexicographic order, `plainChanges()`
 *   (Algorithm 7.2.1.2P, of Steinhaus, Johnson and Trotter) those of \f$ n \f$
 *   distinct items where two consecutive ones differ by a swap of adjacent
 *   items, and `heapPermutations()` (Heap's method, Algorithm 7.2.1.2G) by one
 *   swap each, with the least work per permutation;
 * - `combinationsLex()` the \f$ t \f$-combinations of \f$ n \f$ in
 *   lexicographic order (the order of Algorithm 7.2.1.3L), and
 *   `revolvingDoor()` in the order of Algorithm 7.2.1.3R, where two
 *   consecutive ones differ by one item in and one out;
 * - `integerPartitions()` the partitions of \f$ n \f$ in reverse
 *   lexicographic order (that of Algorithm 7.2.1.4P), and `setPartitions()`
 *   the partitions of a set of \f$ n \f$ by their restricted growth strings
 *   \f$ a_1 = 0 \f$, \f$ a_{j+1} \le 1 + \max( a_1 , \ldots , a_j ) \f$ (as
 *   Algorithm 7.2.1.5H);
 * - `nestedParentheses()` the strings of \f$ n \f$ pairs of balanced
 *   parentheses in lexicographic order (that of Algorithm 7.2.1.6P), i.e., the
 *   forests and the binary trees with \f$ n \f$ nodes.
 *
 * The counts are \f$ \prod_j m_j \f$, the multinomial
 * \f$ n! / \prod_v m_v! \f$ of the multiplicities, \f$ n! \f$,
 * \f$ \binom{n}{t} \f$, \f$ p( n ) \f$, the Bell number \f$ \varpi_n \f$ and
 * the Catalan number \f$ C_n \f$, and the tests check them. The contracts [see
 * `Contract.h`]: the radices are positive, \f$ 0 \le t \le n \f$,
 * \f$ n \ge 0 \f$. The functions are defined in `Generation.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4A, Addison-Wesley,
 *      2011, section 7.2.1
 * \see H. F. Trotter, Algorithm 115: Perm, Communications of the ACM 5(8),
 *      434-435, 1962, doi:10.1145/368637.368660
 * \see S. M. Johnson, Generation of permutations by adjacent transposition,
 *      Mathematics of Computation 17(83), 282-285, 1963,
 *      doi:10.1090/S0025-5718-1963-0159764-2
 * \see B. R. Heap, Permutations by interchanges, The Computer Journal 6(3),
 *      293-298, 1963, doi:10.1093/comjnl/6.3.293
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
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Tuple = std::vector< int >;   ///< a tuple, a permutation, a string

/// the tuples \f$ ( a_0 , \ldots , a_{n-1} ) \f$ with
/// \f$ 0 \le a_j < m_j \f$, in lexicographic order
///
/// In pseudocode (Knuth's Algorithm 7.2.1.1M, beyond the book), adding 1 with
/// the carries:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mixedRadix}( m ) \cr
/// 2 & \quad a \gets ( 0 , \ldots , 0 ) \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 5 & \qquad j \gets n - 1 \cr
/// 6 & \qquad \textbf{while}\ j \ge 0\ \textbf{and}\ a_j = m_j - 1
///     \ \textbf{do}\ a_j \gets 0;\ j \gets j - 1 \cr
/// 7 & \qquad \textbf{if}\ j < 0\ \textbf{then return} \cr
/// 8 & \qquad a_j \gets a_j + 1
/// \end{array}}
/// \f]
/// \pre each \f$ m_j \ge 1 \f$
template< class F >
long mixedRadix( const std::vector< int > & m , F && visit );

/// the tuples with \f$ 0 \le a_j < m_j \f$ in the reflected Gray order,
/// loopless
///
/// In pseudocode (Knuth's Algorithm 7.2.1.1H, beyond the book), the places
/// counted from the right (\f$ a_j \f$ is `a[ n - 1 - j ]`), \f$ o_j \f$ the
/// direction of the place \f$ j \f$ and \f$ f_j \f$ the focus pointers, which
/// give the next place to change in \f$ O( 1 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{grayTuples}( m ) \cr
/// 2 & \quad a \gets ( 0 , \ldots , 0 );\ o \gets ( 1 , \ldots , 1 );
///     \ f_j \gets j \text{ for } 0 \le j \le n \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 5 & \qquad j \gets f_0;\ f_0 \gets 0 \cr
/// 6 & \qquad \textbf{if}\ j = n\ \textbf{then return} \cr
/// 7 & \qquad a_j \gets a_j + o_j \cr
/// 8 & \qquad \textbf{if}\ a_j = 0\ \textbf{or}\ a_j = m_j - 1\ \textbf{then}
///     \quad \triangleright\ \text{the end of its range} \cr
/// 9 & \qquad\quad o_j \gets -o_j;\ f_j \gets f_{j+1};\ f_{j+1} \gets j + 1
/// \end{array}}
/// \f]
/// \pre each \f$ m_j \ge 2 \f$
template< class F >
long grayTuples( const std::vector< int > & m , F && visit );

/// the permutations of the multiset \f$ a \f$, in lexicographic order from
/// \f$ a \f$ sorted
///
/// In pseudocode (Knuth's Algorithm 7.2.1.2L, beyond the book): the suffix
/// after the last ascent is decreasing, and the next permutation raises the
/// place of the ascent by the least larger item of the suffix and sorts the
/// suffix.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{permutationsLex}( a ) \cr
/// 2 & \quad \text{sort } a \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 5 & \qquad j \gets n - 2;\ \textbf{while}\ j \ge 0\ \textbf{and}\ a_j \ge
///     a_{j+1}\ \textbf{do}\ j \gets j - 1 \cr
/// 6 & \qquad \textbf{if}\ j < 0\ \textbf{then return} \cr
/// 7 & \qquad l \gets n - 1;\ \textbf{while}\ a_j \ge a_l\ \textbf{do}\ l
///     \gets l - 1 \cr
/// 8 & \qquad \text{swap } a_j \text{ and } a_l;\ \text{reverse } a_{j+1} ,
///     \ldots , a_{n-1}
/// \end{array}}
/// \f]
template< class F >
long permutationsLex( Tuple a , F && visit );

/// the permutations of \f$ 0 , \ldots , n - 1 \f$ by swaps of adjacent items
///
/// In pseudocode (Knuth's Algorithm 7.2.1.2P, beyond the book), the places
/// from 1, \f$ c_j \f$ and \f$ o_j \f$ the counter and the direction of the
/// item \f$ j \f$, and \f$ s \f$ the items above it already at an end:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{plainChanges}( n ) \cr
/// 2 & \quad a_j \gets j - 1;\ c_j \gets 0;\ o_j \gets 1 \text{ for } 1 \le j
///     \le n \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 5 & \qquad j \gets n;\ s \gets 0 \cr
/// 6 & \qquad \textbf{loop} \cr
/// 7 & \qquad\quad \textbf{if}\ j < 1\ \textbf{then return} \cr
/// 8 & \qquad\quad q \gets c_j + o_j \cr
/// 9 & \qquad\quad \textbf{if}\ q < 0\ \textbf{then}\ o_j \gets -o_j;\ j \gets
///     j - 1 \cr
/// 10 & \qquad\quad \textbf{else if}\ q = j\ \textbf{then} \cr
/// 11 & \qquad\qquad \textbf{if}\ j = 1\ \textbf{then return} \cr
/// 12 & \qquad\qquad s \gets s + 1;\ o_j \gets -o_j;\ j \gets j - 1 \cr
/// 13 & \qquad\quad \textbf{else} \cr
/// 14 & \qquad\qquad \text{swap } a_{j - c_j + s} \text{ and } a_{j - q + s};
///      \ c_j \gets q;\ \text{leave the inner loop}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$
template< class F >
long plainChanges( int n , F && visit );

/// the permutations of \f$ 0 , \ldots , n - 1 \f$ by Heap's method, one swap
/// each
///
/// In pseudocode (Heap's method, the iterative form, beyond the book),
/// \f$ c_i \f$ the counter of the place \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{heapPermutations}( n ) \cr
/// 2 & \quad a \gets ( 0 , \ldots , n - 1 );\ c \gets ( 0 , \ldots , 0 );
///     \ \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 3 & \quad i \gets 1 \cr
/// 4 & \quad \textbf{while}\ i < n\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ c_i < i\ \textbf{then} \cr
/// 6 & \qquad\quad \textbf{if}\ i \text{ is odd }\ \textbf{then}\ \text{swap }
///     a_{c_i} \text{ and } a_i\ \textbf{else}\ \text{swap } a_0 \text{ and }
///     a_i \cr
/// 7 & \qquad\quad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 8 & \qquad\quad c_i \gets c_i + 1;\ i \gets 1 \cr
/// 9 & \qquad \textbf{else}\ c_i \gets 0;\ i \gets i + 1
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$
template< class F >
long heapPermutations( int n , F && visit );

/// the \f$ t \f$-combinations of \f$ 0 , \ldots , n - 1 \f$ as increasing
/// tuples, in lexicographic order
///
/// In pseudocode (the lexicographic successor, the order of Knuth's
/// Algorithm 7.2.1.3L, beyond the book): the last place that can grow grows
/// by 1, and the places after it follow it by steps of 1.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{combinationsLex}( n , t ) \cr
/// 2 & \quad c_j \gets j \text{ for } 0 \le j < t \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad \textbf{if}\ \mathrm{visit}( c )\ \textbf{then return} \cr
/// 5 & \qquad j \gets t - 1;\ \textbf{while}\ j \ge 0\ \textbf{and}\ c_j = n -
///     t + j\ \textbf{do}\ j \gets j - 1 \cr
/// 6 & \qquad \textbf{if}\ j < 0\ \textbf{then return} \cr
/// 7 & \qquad c_j \gets c_j + 1;\ c_k \gets c_{k-1} + 1 \text{ for } j < k < t
/// \end{array}}
/// \f]
/// \pre \f$ 0 \le t \le n \f$
template< class F >
long combinationsLex( int n , int t , F && visit );

/// the \f$ t \f$-combinations of \f$ 0 , \ldots , n - 1 \f$ in the
/// revolving-door order
///
/// In pseudocode (the recursion behind Knuth's Algorithm 7.2.1.3R, beyond the
/// book): the order \f$ R( n , t ) \f$ is \f$ R( n - 1 , t ) \f$ followed by
/// \f$ R( n - 1 , t - 1 ) \f$ reversed, each with \f$ n - 1 \f$ added, so that
/// the last combination of the first part and the first of the second differ
/// by one item in and one out.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{door}( n , t , \mathit{rev} )\quad
///     \triangleright\ \text{called as } \mathrm{door}( n , t ,
///     \textbf{false} ) \cr
/// 2 & \quad \textbf{if}\ t = 0\ \textbf{then}\ \mathrm{visit}( c );
///     \ \textbf{return} \cr
/// 3 & \quad \textbf{if}\ n = t\ \textbf{then}\ c \gets ( 0 , \ldots , t - 1
///     );\ \mathrm{visit}( c );\ \textbf{return} \cr
/// 4 & \quad \textbf{if not}\ \mathit{rev}\ \textbf{then} \cr
/// 5 & \qquad \mathrm{door}( n - 1 , t , \textbf{false} );\ c_{t-1} \gets n -
///     1;\ \mathrm{door}( n - 1 , t - 1 , \textbf{true} ) \cr
/// 6 & \quad \textbf{else} \cr
/// 7 & \qquad c_{t-1} \gets n - 1;\ \mathrm{door}( n - 1 , t - 1 ,
///     \textbf{false} );\ \mathrm{door}( n - 1 , t , \textbf{true} )
/// \end{array}}
/// \f]
/// The generation stops as soon as a visit returns true.
/// \pre \f$ 0 \le t \le n \f$
template< class F >
long revolvingDoor( int n , int t , F && visit );

/// the partitions of \f$ n \f$, their parts nonincreasing, in reverse
/// lexicographic order
///
/// In pseudocode (the order of Knuth's Algorithm 7.2.1.4P, beyond the book):
/// the last part larger than 1 drops by 1, and the 1s after it, with the unit
/// taken from it, are split again into parts as large as it.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{integerPartitions}( n ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then}\ \mathrm{visit}( () );
///     \ \textbf{return} \cr
/// 3 & \quad a \gets ( n ) \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 6 & \qquad k \gets \text{the 1s at the end of } a \text{, removed} \cr
/// 7 & \qquad \textbf{if}\ a = ()\ \textbf{then return} \cr
/// 8 & \qquad x \gets \text{the last part of } a \text{, decreased by 1};\ r
///     \gets k + 1 \cr
/// 9 & \qquad \textbf{while}\ r > x\ \textbf{do}\ \text{append } x \text{ to }
///     a;\ r \gets r - x \cr
/// 10 & \qquad \textbf{if}\ r > 0\ \textbf{then}\ \text{append } r \text{ to }
///      a
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$
template< class F >
long integerPartitions( int n , F && visit );

/// the restricted growth strings of the partitions of a set of \f$ n \f$
/// items, in lexicographic order
///
/// In pseudocode (the order of Knuth's Algorithm 7.2.1.5H, beyond the book),
/// \f$ b_j = 1 + \max( a_0 , \ldots , a_{j-1} ) \f$ the largest value that
/// \f$ a_j \f$ may take:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setPartitions}( n ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then}\ \mathrm{visit}( () );
///     \ \textbf{return} \cr
/// 3 & \quad a \gets ( 0 , \ldots , 0 );\ b \gets ( 1 , \ldots , 1 ) \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \textbf{if}\ \mathrm{visit}( a )\ \textbf{then return} \cr
/// 6 & \qquad j \gets n - 1;\ \textbf{while}\ j > 0\ \textbf{and}\ a_j = b_j
///     \ \textbf{do}\ j \gets j - 1 \cr
/// 7 & \qquad \textbf{if}\ j = 0\ \textbf{then return} \cr
/// 8 & \qquad a_j \gets a_j + 1 \cr
/// 9 & \qquad \textbf{for}\ k \gets j + 1\ \textbf{to}\ n - 1\ \textbf{do}
///     \ a_k \gets 0;\ b_k \gets \max( b_{k-1} , a_{k-1} + 1 )
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$
template< class F >
long setPartitions( int n , F && visit );

/// the strings of \f$ n \f$ pairs of balanced parentheses, in lexicographic
/// order
///
/// In pseudocode (a backtrack in the order of Knuth's Algorithm 7.2.1.6P,
/// beyond the book), \f$ o \f$ and \f$ c \f$ the parentheses opened and
/// closed in \f$ s_0 , \ldots , s_{i-1} \f$, the opening one tried first:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{parentheses}( i , o , c )\quad
///     \triangleright\ \text{called as } \mathrm{parentheses}( 0 , 0 , 0 ) \cr
/// 2 & \quad \textbf{if}\ i = 2 n\ \textbf{then}\ \mathrm{visit}( s );
///     \ \textbf{return} \cr
/// 3 & \quad \textbf{if}\ o < n\ \textbf{then}\ s_i \gets \text{(};
///     \ \mathrm{parentheses}( i + 1 , o + 1 , c ) \cr
/// 4 & \quad \textbf{if}\ c < o\ \textbf{then}\ s_i \gets \text{)};
///     \ \mathrm{parentheses}( i + 1 , o , c + 1 )
/// \end{array}}
/// \f]
/// The generation stops as soon as a visit returns true.
/// \pre \f$ n \ge 0 \f$
template< class F >
long nestedParentheses( int n , F && visit );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/combinatorial/impl/Generation.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Generation.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Generation.h --------------------------*/
/*--------------------------------------------------------------------------*/
