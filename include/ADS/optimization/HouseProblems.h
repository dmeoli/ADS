/*--------------------------------------------------------------------------*/
/*-------------------------- File HouseProblems.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems of dynamic programming on a row of n houses, where the
 * choice of each house depends only on the one of its neighbour:
 *
 * - the largest sum of the items of A, no two of them adjacent (the
 *   donations of houses no two of which are neighbours, the maximum weight
 *   independent set of a path): \f$ D[ i ] = \max( D[ i - 1 ] , D[ i - 2 ]
 *   + A_{i-1} ) \f$, D[ 0 ] = D[ -1 ] = 0; maxNonAdjacentSum() in
 *   \f$ \Theta( n ) \f$ time and O( 1 ) space, nonAdjacentSelection() with
 *   the places read back from the table, \f$ \Theta( n ) \f$ space. It is
 *   the weighted intervals of section 13.2 [see WeightedIntervals.h] with
 *   the unit intervals [ i , i + 2 ).
 * - paintHouses(): the cheapest painting of the houses with k colors,
 *   cost[ h ][ c ] for the house h of color c, two adjacent houses of
 *   different colors: \f$ D[ h ][ c ] = cost[ h ][ c ] + \min_{p \neq c}
 *   D[ h - 1 ][ p ] \f$, which is O( n k^2 ) as written; keeping of each
 *   row its least value, its color and its second least value, the least
 *   over \f$ p \neq c \f$ is one of the two, and the time is O( n k ), the
 *   colors read back from the table of O( n k ) space.
 *
 * Both are shortest paths in the DAG of the states (house, choice), and
 * generalize to a tree of houses by the same recurrence on the children.
 *
 * The contracts [see Contract.h]: the rows of cost have the same number k
 * of colors, k >= 2 for two houses or more; the places chosen are
 * increasing and not adjacent, the colors of adjacent houses differ, and
 * both give the value found. The functions are defined in
 * HouseProblems.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.2,
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

/// the largest sum of items of A no two of them adjacent, 0 for none
/// \post not negative
template< class T >
T maxNonAdjacentSum( const std::vector< T > & A );

/// the same, and the places of the items, increasing; an item is taken
/// only if it makes the sum larger
/// \post no two places adjacent, the items of the sum found
template< class T >
std::pair< T , std::vector< std::size_t > > nonAdjacentSelection(
                                                  const std::vector< T > & A );

/// the least cost of a painting of the houses, adjacent houses of different
/// colors, cost[ h ][ c ] for the house h of the color c, and the colors
/// \pre the rows of the same size k, k >= 2 if there are two rows or more,
///      k >= 1 if one
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
