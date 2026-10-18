/*--------------------------------------------------------------------------*/
/*------------------------- File StringBreaking.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Breaking a string: a string of length L has to be cut at the places
 * 0 < c_1 < ... < c_n < L, and each cut costs the length of the piece it
 * splits; the order of the cuts decides the total cost. With the sentinels
 * w_0 = 0 and w_{n+1} = L, D[ i ][ j ], the least cost of the cuts strictly
 * between w_i and w_j, is 0 if j = i + 1 and otherwise
 * \f$ w_j - w_i + \min_{i < k < j} ( D[ i ][ k ] + D[ k ][ j ] ) \f$, the
 * scheme of the chain of matrices and of the optimal search tree, which
 * takes \f$ \Theta( n^3 ) \f$ time. The cost \f$ w_j - w_i \f$ satisfies
 * the quadrangle inequality as an equality and is monotone on the
 * intervals, so the largest optimal split K[ i ][ j ] lies between
 * K[ i ][ j - 1 ] and K[ i + 1 ][ j ] (Knuth for the search trees, Yao in
 * general): the candidates of all the intervals of one length add up to
 * O( n ), and the table costs \f$ \Theta( n^2 ) \f$ time and space.
 * stringBreakingOrder() reads an optimal order back from the splits, each
 * cut before those of its two pieces.
 *
 * The cut places are any integer type; the overloads on std::int64_t also
 * take a braced list.
 *
 * The contracts [see Contract.h]: the places are strictly increasing and
 * inside ( 0 , L ); the order is a permutation of the places whose cost is
 * the least one (an audit). The functions are defined in
 * StringBreaking.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, problem 15-9
 * \see D. E. Knuth, Optimum binary search trees, Acta Informatica 1(1),
 *      14-25, 1971
 * \see F. F. Yao, Efficient dynamic programming using quadrangle
 *      inequalities, Proceedings of the 12th ACM Symposium on Theory of
 *      Computing, 429-435, 1980
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
/// the places cuts
/// \pre the cuts strictly increasing and inside ( 0 , length )
template< class I >
[[nodiscard]] I stringBreakingCost( const std::vector< I > & cuts ,
                                    std::type_identity_t< I > length );

/// stringBreakingCost() on std::int64_t (or a braced list)
[[nodiscard]] inline std::int64_t stringBreakingCost(
              const std::vector< std::int64_t > & cuts , std::int64_t length );

/// the places cuts in an order of least total cost
/// \pre the cuts strictly increasing and inside ( 0 , length )
/// \post a permutation of cuts of cost stringBreakingCost() (an audit)
template< class I >
[[nodiscard]] std::vector< I > stringBreakingOrder(
           const std::vector< I > & cuts , std::type_identity_t< I > length );

/// stringBreakingOrder() on std::int64_t (or a braced list)
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
