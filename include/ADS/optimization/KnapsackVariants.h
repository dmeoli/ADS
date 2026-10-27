/*--------------------------------------------------------------------------*/
/*------------------------ File KnapsackVariants.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The relatives of the knapsack in which an item can be taken many times,
 * and the counting versions of the knapsack, all solved by the dynamic
 * programming on the capacity of the 0-1 knapsack of section 13.6, in
 * pseudopolynomial time:
 *
 * - `rodCutting()`: a rod of length \f$ L \f$ sold in pieces,
 *   \f$ \mathit{price}[ t - 1 ] \f$ the price of a piece of length \f$ t \f$,
 *   the largest gain
 *   \f$ R[ l ] = \max_{1 \le t \le \min( l , m )} \mathit{price}[ t - 1 ] +
 *   R[ l - t ] \f$, \f$ R[ 0 ] = 0 \f$, with the pieces read back;
 *   \f$ O( L m ) \f$ time and \f$ O( L ) \f$ space for \f$ m \f$ prices;
 * - `unboundedKnapsack()`: the largest profit of objects of volumes
 *   \f$ v \f$ and profits \f$ p \f$ in unlimited supply in a knapsack of
 *   capacity \f$ C \f$,
 *   \f$ P[ c ] = \max( P[ c - 1 ] , \max_{v_i \le c} P[ c - v_i ] + p_i ) \f$,
 *   and how many copies of each object; \f$ O( n C ) \f$ time, \f$ O( C ) \f$
 *   space;
 * - `minValueExactWeight()`: the least value of coins of weights \f$ w \f$
 *   and values \f$ v \f$ in unlimited supply that weigh exactly \f$ W \f$,
 *   nothing if no such coins exist; with all the values 1 it is the fewest
 *   coins of the change (`optimalChange()` of `CoinChange.h`);
 *   \f$ O( n W ) \f$ time, \f$ O( W ) \f$ space;
 * - `coinChangeWays()`: the number of ways of paying \f$ T \f$ with coins of
 *   the given values in unlimited supply, the order not counting: the coins
 *   enter one value at a time, \f$ N[ x ] \mathrel{+}= N[ x - d ] \f$ for
 *   \f$ x \f$ from \f$ d \f$ up, so that each multiset is counted once;
 *   \f$ O( n T ) \f$ time, \f$ O( T ) \f$ space;
 * - `subsetSumCount()`: the number of subsets of places of \f$ A \f$ of sum
 *   \f$ k \f$, the same scan with \f$ x \f$ going down, each item used at most
 *   once; \f$ O( n k ) \f$ time, \f$ O( k ) \f$ space;
 * - `minimumSquares()`: the fewest squares of positive integers that sum to
 *   \f$ n \f$. The dynamic programming
 *   \f$ S[ i ] = 1 + \min_{t^2 \le i} S[ i - t^2 ] \f$ takes
 *   \f$ \Theta( n \sqrt{n} ) \f$ time; number theory answers in
 *   \f$ O( \sqrt{n} ) \f$: 0 for \f$ n = 0 \f$, 1 for a square, 4 for
 *   \f$ n = 4^a ( 8 b + 7 ) \f$ (Legendre's three-square theorem), 2 if
 *   \f$ n - t^2 \f$ is a square for some \f$ t \f$, 3 otherwise; at most 4 by
 *   Lagrange's four-square theorem. Taking the largest square first is not
 *   optimal: \f$ 18 = 9 + 9 \f$, not \f$ 16 + 1 + 1 \f$.
 *
 * The running times are pseudopolynomial, exponential in the size of the
 * numbers: the 0-1 decision versions are NP-complete [see
 * `Pseudopolynomial.h`]. The bounded knapsack, each object available
 * \f$ b_i \f$ times, reduces to the 0-1 one by splitting \f$ b_i \f$ in
 * powers of two, and the counting versions are the coefficients of the
 * generating functions \f$ \prod_d ( 1 - z^d )^{-1} \f$ and
 * \f$ \prod_i ( 1 + z^{a_i} ) \f$.
 *
 * The contracts [see `Contract.h`]: the capacities and the amounts are not
 * negative, the volumes, the weights and the values of the coins positive,
 * the items of `subsetSumCount()` not negative; the pieces of `rodCutting()`
 * and the copies of `unboundedKnapsack()` fit and give the value found. The
 * functions are defined in `KnapsackVariants.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.6 and the
 *      Esercizi Programmazione dinamica, exercises B4 of 01/02/2012,
 *      B3 of 31/05/2012, B4 of 02/02/2015 and B4 of 26/01/2016,
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
 *         Università di Pisa \n
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

/// the largest gain of a rod of length \f$ L \f$ cut in pieces of lengths
/// \f$ 1 , \ldots , m \f$, \f$ m = |\mathit{price}| \f$,
/// \f$ \mathit{price}[ t - 1 ] \f$ the price of a piece of length \f$ t \f$,
/// and the lengths of the pieces
///
/// In pseudocode (exercise B4 of 01/02/2012, Best cut, of the Esercizi
/// Programmazione dinamica of the book), \f$ \mathit{cut}[ l ] \f$ the first
/// piece of a best cut of the length \f$ l \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rodCutting}( \mathit{price} , L ) \cr
/// 2 & \quad R[ 0 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ l \gets 1\ \textbf{to}\ L\ \textbf{do} \cr
/// 4 & \qquad R[ l ] \gets -\infty \cr
/// 5 & \qquad \textbf{for}\ t \gets 1\ \textbf{to}\ \min( l , m )\ \textbf{do}
///     \cr
/// 6 & \qquad\quad \textbf{if}\ \mathit{price}[ t - 1 ] + R[ l - t ] > R[ l
///     ]\ \textbf{then} \cr
/// 7 & \qquad\qquad R[ l ] \gets \mathit{price}[ t - 1 ] + R[ l - t ];
///     \ \mathit{cut}[ l ] \gets t \cr
/// 8 & \quad \textbf{return}\ R[ L ] \text{ and the pieces } \mathit{cut}[ L
///     ] , \mathit{cut}[ L - \mathit{cut}[ L ] ] , \ldots
/// \end{array}}
/// \f]
/// \pre \f$ L = 0 \f$ or `price` not empty
/// \post the pieces sum to \f$ L \f$ and their prices to the gain (audit)
template< class T >
std::pair< T , std::vector< std::size_t > > rodCutting(
                              const std::vector< T > & price , std::size_t L );

/// the largest profit of the objects of profits \f$ p \f$ and volumes
/// \f$ v \f$, each in unlimited supply, in a knapsack of capacity \f$ C \f$,
/// and the copies of each
///
/// In pseudocode (beyond the book, after Martello and Toth),
/// \f$ \mathit{last}[ c ] \f$ the last object
/// added for the capacity \f$ c \f$, \f$ \textbf{nil} \f$ for a unit left
/// empty:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{unboundedKnapsack}( p , v , C ) \cr
/// 2 & \quad P[ 0 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ c \gets 1\ \textbf{to}\ C\ \textbf{do} \cr
/// 4 & \qquad P[ c ] \gets P[ c - 1 ];\ \mathit{last}[ c ] \gets \textbf{nil}
///     \cr
/// 5 & \qquad \textbf{foreach}\ i\ \textbf{with}\ v_i \le c \text{ and }
///     P[ c - v_i ] + p_i > P[ c ]\ \textbf{do} \cr
/// 6 & \qquad\quad P[ c ] \gets P[ c - v_i ] + p_i;\ \mathit{last}[ c ] \gets
///     i \cr
/// 7 & \quad \text{read the copies back from } \mathit{last}[ C ] \text{, }
///     \text{a step of } v_i \text{ or of 1 for } \textbf{nil} \cr
/// 8 & \quad \textbf{return}\ P[ C ] \text{ and the copies}
/// \end{array}}
/// \f]
/// \pre \f$ |p| = |v| \f$, \f$ v_i > 0 \f$, \f$ C \ge 0 \f$
/// \post the copies fit and give the profit (audit)
template< class T , class V >
std::pair< T , std::vector< std::size_t > > unboundedKnapsack(
               const std::vector< T > & p , const std::vector< V > & v , V C );

/// the least value of coins of weights \f$ w \f$ and values \f$ v \f$, in
/// unlimited supply, weighing exactly \f$ W \f$; nothing if there is none
///
/// In pseudocode (exercise B4 of 02/02/2015, Piggy bank, of the Esercizi
/// Programmazione dinamica of the book), \f$ D[ x ] = \textbf{nil} \f$ for a
/// weight that no coins make:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minValueExactWeight}( w , v , W ) \cr
/// 2 & \quad D[ 0 ] \gets 0;\ D[ 1 .. W ] \gets \textbf{nil} \cr
/// 3 & \quad \textbf{for}\ x \gets 1\ \textbf{to}\ W\ \textbf{do} \cr
/// 4 & \qquad \textbf{foreach}\ i\ \textbf{with}\ w_i \le x \text{ and } D[ x
///     - w_i ] \ne \textbf{nil}\ \textbf{do} \cr
/// 5 & \qquad\quad D[ x ] \gets \min\{ D[ x ] , D[ x - w_i ] + v_i \}\quad
///     \triangleright\ \textbf{nil} \text{ the largest} \cr
/// 6 & \quad \textbf{return}\ D[ W ]
/// \end{array}}
/// \f]
/// \pre \f$ |w| = |v| \f$, \f$ w_i > 0 \f$, \f$ W \ge 0 \f$
template< class T , class V >
std::optional< T > minValueExactWeight( const std::vector< V > & w ,
                                        const std::vector< T > & v , V W );

/// the number of multisets of the coins of sum \f$ T \f$, each coin in
/// unlimited supply; equal values in `coins` count as distinct kinds
///
/// In pseudocode (exercise B3 of 31/05/2012, Resto, of the Esercizi
/// Programmazione dinamica of the book), the coins entering one kind at a
/// time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{coinChangeWays}( D , T ) \cr
/// 2 & \quad N[ 0 ] \gets 1;\ N[ 1 .. T ] \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ d \in D\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ x \gets d\ \textbf{to}\ T\ \textbf{do}\ N[ x ]
///     \gets N[ x ] + N[ x - d ] \cr
/// 5 & \quad \textbf{return}\ N[ T ]
/// \end{array}}
/// \f]
/// \pre \f$ T \ge 0 \f$ and the coins positive
template< class C = std::uint64_t , class V >
C coinChangeWays( const std::vector< V > & coins , V T );

/// the number of subsets of the places of \f$ A \f$ whose items sum to
/// \f$ k \f$
///
/// In pseudocode (beyond the book), the sums scanned downwards so that each
/// item is used once:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsetSumCount}( A , k ) \cr
/// 2 & \quad N[ 0 ] \gets 1;\ N[ 1 .. k ] \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ a \in A\ \textbf{with}\ a \le k\ \textbf{do}
///     \cr
/// 4 & \qquad \textbf{for}\ x \gets k\ \textbf{downto}\ a\ \textbf{do}\ N[ x
///     ] \gets N[ x ] + N[ x - a ] \cr
/// 5 & \quad \textbf{return}\ N[ k ]
/// \end{array}}
/// \f]
/// \pre \f$ k \ge 0 \f$ and \f$ A \f$ not negative
template< class C = std::uint64_t , class V >
C subsetSumCount( const std::vector< V > & A , V k );

/// the fewest squares of positive integers summing to \f$ n \f$
///
/// In pseudocode (the theorems of Lagrange and Legendre, beyond the book,
/// whose exercise B4 of 26/01/2016, Somma di quadrati, of the
/// Esercizi Programmazione dinamica asks the dynamic programming):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minimumSquares}( n ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ 0 \cr
/// 3 & \quad \textbf{if}\ n \text{ is a square}\ \textbf{then return}\ 1 \cr
/// 4 & \quad m \gets n \text{ with its factors 4 removed} \cr
/// 5 & \quad \textbf{if}\ m \bmod 8 = 7\ \textbf{then return}\ 4 \cr
/// 6 & \quad \textbf{for}\ t \gets 1\ \textbf{to}\ \lfloor \sqrt n \rfloor
///     \ \textbf{do} \cr
/// 7 & \qquad \textbf{if}\ n - t^2 \text{ is a square}\ \textbf{then return}
///     \ 2 \cr
/// 8 & \quad \textbf{return}\ 3
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$
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
