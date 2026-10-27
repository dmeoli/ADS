/*--------------------------------------------------------------------------*/
/*-------------------------- File HouseProblems.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems of dynamic programming on a row of \f$ n \f$ houses, where the
 * choice of each house depends only on the one of its neighbour:
 *
 * - the largest sum of the items of \f$ A \f$, no two of them adjacent (the
 *   donations of houses no two of which are neighbours, the maximum weight
 *   independent set of a path):
 *   \f$ D[ i ] = \max( D[ i - 1 ] , D[ i - 2 ] + A_{i-1} ) \f$,
 *   \f$ D[ 0 ] = D[ -1 ] = 0 \f$; `maxNonAdjacentSum()` in
 *   \f$ \Theta( n ) \f$ time and \f$ O( 1 ) \f$ space,
 *   `nonAdjacentSelection()` with the places read back from the table,
 *   \f$ \Theta( n ) \f$ space. It is the weighted intervals of section 13.2
 *   [see `WeightedIntervals.h`] with the unit intervals \f$ [ i , i + 2 ) \f$;
 * - `paintHouses()`: the cheapest painting of the houses with \f$ k \f$
 *   colors, \f$ \mathit{cost}[ h ][ c ] \f$ for the house \f$ h \f$ of color
 *   \f$ c \f$, two adjacent houses of different colors: \f$ D[ h ][ c ] =
 *   \mathit{cost}[ h ][ c ] + \min_{p \ne c} D[ h - 1 ][ p ] \f$, which is
 *   \f$ O( n k^2 ) \f$ as written; keeping of each row its least value, its
 *   color and its second least value, the least over \f$ p \ne c \f$ is one of
 *   the two, and the time is \f$ O( n k ) \f$, the colors read back from the
 *   table of \f$ O( n k ) \f$ space.
 *
 * Both are shortest paths in the DAG of the states (house, choice), and
 * generalize to a tree of houses by the same recurrence on the children.
 *
 * The contracts [see `Contract.h`]: the rows of `cost` have the same number
 * \f$ k \f$ of colors, \f$ k \ge 2 \f$ for two houses or more; the places
 * chosen are increasing and not adjacent, the colors of adjacent houses
 * differ, and both give the value found. The functions are defined in
 * `HouseProblems.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.2 and
 *      the Esercizi Programmazione dinamica, exercises B3 of 17/06/2013
 *      and of 02/02/2026,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see J. Kleinberg, E. Tardos, Algorithm Design, Addison-Wesley, 2005,
 *      section 6.1
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

#ifndef __ADS_HouseProblems
 #define __ADS_HouseProblems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the largest sum of items of \f$ A \f$ no two of them adjacent, 0 for none
///
/// In pseudocode (exercise B3 of 17/06/2013, Fundraising, of the Esercizi
/// Programmazione dinamica of the book), the last two entries of the table
/// kept:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxNonAdjacentSum}( A ) \cr
/// 2 & \quad \mathit{prev} \gets 0;\ \mathit{cur} \gets 0\quad
///     \triangleright\ D[ i - 2 ] \text{ and } D[ i - 1 ] \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{do} \cr
/// 4 & \qquad ( \mathit{prev} , \mathit{cur} ) \gets ( \mathit{cur} , \max\{
///     \mathit{cur} , \mathit{prev} + x \} ) \cr
/// 5 & \quad \textbf{return}\ \mathit{cur}
/// \end{array}}
/// \f]
/// \post the result is not negative
template< class T >
T maxNonAdjacentSum( const std::vector< T > & A );

/// the sum of `maxNonAdjacentSum()` and the places of its items, increasing;
/// an item is taken only if it makes the sum larger
///
/// In pseudocode (the same exercise), the table read back from its end:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nonAdjacentSelection}( A[ 0 .. n - 1 ] ) \cr
/// 2 & \quad D[ 0 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do}\ D[ i ]
///     \gets \max\{ D[ i - 1 ] , D[ i - 2 ] + A[ i - 1 ] \}\quad
///     \triangleright\ D[ -1 ] = 0 \cr
/// 4 & \quad S \gets \emptyset;\ i \gets n \cr
/// 5 & \quad \textbf{while}\ i > 0\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ D[ i - 1 ] < D[ i ]\ \textbf{then}\ S \gets S \cup
///     \{ i - 1 \};\ i \gets \max\{ i - 2 , 0 \} \cr
/// 7 & \qquad \textbf{else}\ i \gets i - 1 \cr
/// 8 & \quad \textbf{return}\ ( D[ n ] , S )
/// \end{array}}
/// \f]
/// \post no two places adjacent, the items of the sum found
template< class T >
std::pair< T , std::vector< std::size_t > > nonAdjacentSelection(
                                                  const std::vector< T > & A );

/// the least cost of a painting of the houses, adjacent houses of different
/// colors, \f$ \mathit{cost}[ h ][ c ] \f$ for the house \f$ h \f$ of the
/// color \f$ c \f$, and the colors
///
/// In pseudocode (exercise B3 of 02/02/2026, Hateville PNRR, of the Esercizi
/// Programmazione dinamica of the book), \f$ b \f$ and \f$ s \f$ the colors of
/// the least and of the second least entry of the row before:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{paintHouses}( \mathit{cost} ) \cr
/// 2 & \quad D[ 0 ] \gets \mathit{cost}[ 0 ] \cr
/// 3 & \quad \textbf{for}\ h \gets 1\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 4 & \qquad b , s \gets \text{the colors of the two least entries of } D[
///     h - 1 ] \cr
/// 5 & \qquad \textbf{foreach}\ \text{color } c\ \textbf{do} \cr
/// 6 & \qquad\quad q \gets s\ \textbf{if}\ c = b\ \textbf{else}\ b \cr
/// 7 & \qquad\quad D[ h ][ c ] \gets \mathit{cost}[ h ][ c ] + D[ h - 1 ][ q
///     ] \cr
/// 8 & \quad c_{n-1} \gets \text{the color of the least entry of } D[ n - 1 ]
///     \cr
/// 9 & \quad \textbf{for}\ h \gets n - 1\ \textbf{downto}\ 1\ \textbf{do}
///     \ c_{h-1} \gets \text{the least of } D[ h - 1 ] \text{ but } c_h \cr
/// 10 & \quad \textbf{return}\ ( D[ n - 1 ][ c_{n-1} ] , c )
/// \end{array}}
/// \f]
/// \pre the rows of the same size \f$ k \f$, \f$ k \ge 2 \f$ if there are two
///      rows or more, \f$ k \ge 1 \f$ if one
/// \post adjacent colors differ and give the cost found
template< class T >
std::pair< T , std::vector< std::size_t > > paintHouses(
                                const std::vector< std::vector< T > > & cost );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/HouseProblems.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HouseProblems.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File HouseProblems.h ------------------------*/
/*--------------------------------------------------------------------------*/
