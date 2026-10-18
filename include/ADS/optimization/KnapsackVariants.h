/*--------------------------------------------------------------------------*/
/*------------------------ File KnapsackVariants.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The relatives of the knapsack in which an item can be taken many times,
 * and the counting versions of the knapsack, all solved by the dynamic
 * programming on the capacity of section 13.3, in pseudopolynomial time:
 *
 * - rodCutting(): a rod of length L sold in pieces, price[ t - 1 ] the
 *   price of a piece of length t, the largest gain
 *   \f$ R[ l ] = \max_{1 \leq t \leq \min( l , m )} price[ t - 1 ] +
 *   R[ l - t ] \f$, R[ 0 ] = 0, with the pieces read back; O( L m ) time
 *   and O( L ) space for m prices;
 * - unboundedKnapsack(): the largest profit of objects of volumes v and
 *   profits p in unlimited supply in a knapsack of capacity C,
 *   \f$ P[ c ] = \max( P[ c - 1 ] , \max_{v_i \leq c} P[ c - v_i ] + p_i )
 *   \f$, and how many copies of each object; O( n C ) time, O( C ) space;
 * - minValueExactWeight(): the least value of coins of weights w and values
 *   v in unlimited supply that weigh exactly W, nothing if no such coins
 *   exist; with all the values 1 it is the fewest coins of the change
 *   (optimalChange() of CoinChange.h); O( n W ) time, O( W ) space;
 * - coinChangeWays(): the number of ways of paying T with coins of the
 *   given values in unlimited supply, the order not counting: the coins
 *   enter one value at a time, \f$ N[ x ] \mathrel{+}= N[ x - d ] \f$ for
 *   x from d up, so that each multiset is counted once; O( n T ) time,
 *   O( T ) space;
 * - subsetSumCount(): the number of subsets of places of A of sum k, the
 *   same scan with x going down, each item used at most once; O( n k )
 *   time, O( k ) space;
 * - minimumSquares(): the fewest squares of positive integers that sum to
 *   n. The dynamic programming \f$ S[ i ] = 1 + \min_{t^2 \leq i}
 *   S[ i - t^2 ] \f$ takes \f$ \Theta( n \sqrt{n} ) \f$ time; number theory
 *   answers in \f$ O( \sqrt{n} ) \f$: 0 for n = 0, 1 for a square, 4 for
 *   \f$ n = 4^a ( 8 b + 7 ) \f$ (Legendre's three-square theorem), 2 if
 *   \f$ n - t^2 \f$ is a square for some t, 3 otherwise; at most 4 by
 *   Lagrange's four-square theorem. Taking the largest square first is not
 *   optimal: 18 = 9 + 9, not 16 + 1 + 1.
 *
 * The running times are pseudopolynomial, exponential in the size of the
 * numbers: the 0-1 decision versions are NP-complete [see
 * Pseudopolynomial.h]. The bounded knapsack, each object available b_i
 * times, reduces to the 0-1 one by splitting b_i in powers of two, and the
 * counting versions are the coefficients of the generating functions
 * \f$ \prod_d ( 1 - z^d )^{-1} \f$ and \f$ \prod_i ( 1 + z^{a_i} ) \f$.
 *
 * The contracts [see Contract.h]: the capacities and the amounts are not
 * negative, the volumes, the weights and the values of the coins positive,
 * the items of subsetSumCount() not negative; the pieces of rodCutting()
 * and the copies of unboundedKnapsack() fit and give the value found. The
 * functions are defined in KnapsackVariants.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 4th ed., MIT Press, 2022, section 14.1
 * \see S. Martello, P. Toth, Knapsack Problems: Algorithms and Computer
 *      Implementations, Wiley, 1990, chapters 3 and 5
 * \see M. O. Rabin, J. O. Shallit, Randomized algorithms in number theory,
 *      Communications on Pure and Applied Mathematics 39(S1), S239-S256,
 *      1986, doi:10.1002/cpa.3160390713
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

#ifndef __ADS_KnapsackVariants
 #define __ADS_KnapsackVariants
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the largest gain of a rod of length L cut in pieces of lengths 1 ..
/// |price|, price[ t - 1 ] the price of a piece of length t, and the
/// lengths of the pieces
/// \pre L = 0 or price not empty
/// \post the pieces sum to L and their prices to the gain
template< class T >
std::pair< T , std::vector< std::size_t > > rodCutting(
                              const std::vector< T > & price , std::size_t L );

/// the largest profit of the objects of profits p and volumes v, each in
/// unlimited supply, in a knapsack of capacity C, and the copies of each
/// \pre |p| = |v|, v positive, C >= 0
/// \post the copies fit and give the profit
template< class T , class V >
std::pair< T , std::vector< std::size_t > > unboundedKnapsack(
               const std::vector< T > & p , const std::vector< V > & v , V C );

/// the least value of coins of weights w and values v, in unlimited supply,
/// weighing exactly W; nothing if there is none
/// \pre |w| = |v|, w positive, W >= 0
template< class T , class V >
std::optional< T > minValueExactWeight( const std::vector< V > & w ,
                                        const std::vector< T > & v , V W );

/// the number of multisets of the coins of sum T, each coin in unlimited
/// supply; equal values in coins count as distinct kinds
/// \pre T >= 0 and the coins positive
template< class C = std::uint64_t , class V >
C coinChangeWays( const std::vector< V > & coins , V T );

/// the number of subsets of the places of A whose items sum to k
/// \pre k >= 0 and A not negative
template< class C = std::uint64_t , class V >
C subsetSumCount( const std::vector< V > & A , V k );

/// the fewest squares of positive integers summing to n
/// \pre n >= 0
/// \post at most 4
inline int minimumSquares( std::int64_t n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/KnapsackVariants.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnapsackVariants.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File KnapsackVariants.h -----------------------*/
/*--------------------------------------------------------------------------*/
