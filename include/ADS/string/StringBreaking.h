/*--------------------------------------------------------------------------*/
/*------------------------- File StringBreaking.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Breaking a string: a string of length \f$ L \f$ has to be cut at the
 * places \f$ 0 < c_1 < \cdots < c_n < L \f$, and each cut costs the length
 * of the piece it splits, so that the order of the cuts decides the total
 * cost. With the sentinels \f$ w_0 = 0 \f$ and \f$ w_{n+1} = L \f$, the
 * value \f$ D[ i ][ j ] \f$, the least cost of the cuts strictly between
 * \f$ w_i \f$ and \f$ w_j \f$, is 0 if \f$ j = i + 1 \f$ and otherwise
 * \f$ w_j - w_i + \min_{i < k < j} ( D[ i ][ k ] + D[ k ][ j ] ) \f$, the
 * scheme of the chain of matrices and of the optimal search tree, which
 * takes \f$ \Theta( n^3 ) \f$ time. The cost \f$ w_j - w_i \f$ satisfies
 * the quadrangle inequality as an equality and is monotone on the
 * intervals, so the largest optimal split \f$ K[ i ][ j ] \f$ lies between
 * \f$ K[ i ][ j - 1 ] \f$ and \f$ K[ i + 1 ][ j ] \f$ (Knuth for the search
 * trees, Yao in general): the candidates of all the intervals of one length
 * add up to \f$ O( n ) \f$, and the table costs \f$ \Theta( n^2 ) \f$ time
 * and space. `stringBreakingOrder()` reads an optimal order back from the
 * splits, each cut before those of its two pieces.
 *
 * The cut places are any integer type; the overloads on `std::int64_t`
 * also take a braced list.
 *
 * The contracts [see `Contract.h`]: the places are strictly increasing and
 * inside \f$ ( 0 , L ) \f$; the order is a permutation of the places whose
 * cost is the least one (an audit). The functions are defined in
 * `StringBreaking.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, problem 15-9
 * \see D. E. Knuth, Optimum binary search trees, Acta Informatica 1(1),
 *      14-25, 1971, doi:10.1007/BF00264289
 * \see F. F. Yao, Efficient dynamic programming using quadrangle
 *      inequalities, Proceedings of the 12th ACM Symposium on Theory of
 *      Computing, 429-435, 1980, doi:10.1145/800141.804691
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

#ifndef __ADS_StringBreaking
 #define __ADS_StringBreaking
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the least total cost of the cuts of a string of the given length at
/// the places `cuts`
///
/// In pseudocode (chapter 13, with the speedup of Knuth and Yao), the
/// intervals by increasing length \f$ d \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{stringBreakingCost}( c , L ) \cr
/// 2 & \quad w \gets ( 0 , c_1 , \ldots , c_n , L ) \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n\ \textbf{do}\ D[ i ][ i
///     + 1 ] \gets 0;\ K[ i ][ i + 1 ] \gets i + 1 \cr
/// 4 & \quad \textbf{for}\ d \gets 2\ \textbf{to}\ n + 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n + 1 - d\ \textbf{do} \cr
/// 6 & \qquad\quad j \gets i + d \cr
/// 7 & \qquad\quad k \gets \text{the largest } k \in [ \max\{ K[ i ][ j - 1 ]
///     , i + 1 \} , \min\{ K[ i + 1 ][ j ] , j - 1 \} ] \text{ of least }
///     D[ i ][ k ] + D[ k ][ j ] \cr
/// 8 & \qquad\quad D[ i ][ j ] \gets D[ i ][ k ] + D[ k ][ j ] + w_j - w_i;
///     \ K[ i ][ j ] \gets k \cr
/// 9 & \quad \textbf{return}\ D[ 0 ][ n + 1 ]
/// \end{array}}
/// \f]
/// \pre the cuts are strictly increasing and inside
///      \f$ ( 0 , \mathit{length} ) \f$
template< class I >
[[nodiscard]] I stringBreakingCost( const std::vector< I > & cuts ,
                                    std::type_identity_t< I > length );

/// the cost of `stringBreakingCost()` above, on `std::int64_t` (or a
/// braced list)
[[nodiscard]] inline std::int64_t stringBreakingCost(
              const std::vector< std::int64_t > & cuts , std::int64_t length );

/// the places `cuts` in an order of least total cost
///
/// In pseudocode (chapter 13), the splits \f$ K \f$ of
/// `stringBreakingCost()` read back with a stack, each cut before those of
/// its two pieces:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{stringBreakingOrder}( c , L ) \cr
/// 2 & \quad K \gets \text{the splits of } \mathrm{stringBreakingCost}( c ,
///     L );\ T \gets ( ( 0 , n + 1 ) );\ O \gets \text{an empty sequence} \cr
/// 3 & \quad \textbf{while}\ T \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad ( i , j ) \gets \mathrm{pop}( T ) \cr
/// 5 & \qquad \textbf{if}\ j > i + 1\ \textbf{then} \cr
/// 6 & \qquad\quad k \gets K[ i ][ j ];\ \text{append } w_k \text{ to } O \cr
/// 7 & \qquad\quad \mathrm{push}( T , ( k , j ) );\ \mathrm{push}( T , ( i ,
///     k ) ) \cr
/// 8 & \quad \textbf{return}\ O
/// \end{array}}
/// \f]
/// \pre the cuts are strictly increasing and inside
///      \f$ ( 0 , \mathit{length} ) \f$
/// \post a permutation of `cuts` of the cost of `stringBreakingCost()`
///       (audit)
template< class I >
[[nodiscard]] std::vector< I > stringBreakingOrder(
           const std::vector< I > & cuts , std::type_identity_t< I > length );

/// the order of `stringBreakingOrder()` above, on `std::int64_t` (or a
/// braced list)
[[nodiscard]] inline std::vector< std::int64_t > stringBreakingOrder(
              const std::vector< std::int64_t > & cuts , std::int64_t length );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/StringBreaking.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StringBreaking.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StringBreaking.h ------------------------*/
/*--------------------------------------------------------------------------*/
