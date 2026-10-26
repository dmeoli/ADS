/*--------------------------------------------------------------------------*/
/*---------------------------- File GridPaths.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems of dynamic programming on a matrix of m rows and n columns,
 * each cell solved from its neighbours above and to the left:
 *
 * - largestSquare(): the largest square of nonzero cells of a 0/1 matrix.
 *   With S[ i ][ j ] the side of the largest one with its lower right
 *   corner in ( i , j ), S[ i ][ j ] = 0 on a zero cell and otherwise
 *   \f$ 1 + \min( S[ i - 1 ][ j ] , S[ i - 1 ][ j - 1 ] , S[ i ][ j - 1 ] )
 *   \f$, since the three squares cover all the square of side one more
 *   but its corner; \f$ \Theta( m n ) \f$ time, \f$ \Theta( n ) \f$ space
 *   keeping one row. The largest rectangle of ones takes \f$ \Theta( m n )
 *   \f$ too, by the largest rectangle in the histogram of each row.
 * - countLatticePaths(): the monotone lattice paths from the upper left
 *   to the lower right cell, a step right or down, through the nonzero
 *   cells: \f$ N[ i ][ j ] = N[ i - 1 ][ j ] + N[ i ][ j - 1 ] \f$ on a
 *   free cell and 0 on a blocked one, \f$ \Theta( m n ) \f$ time and
 *   \f$ \Theta( n ) \f$ space. With no blocked cell they are
 *   \f$ \binom{m + n - 2}{m - 1} \f$; with few blocked cells, the
 *   inclusion-exclusion on the blocked cells sorted takes
 *   \f$ O( b^2 ) \f$ binomials for b of them.
 *
 * The contracts [see Contract.h]: the matrix is a rectangle; the square
 * found is inside the matrix and all nonzero (checked at level 3). The
 * functions are defined in GridPaths.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see S. S. Skiena, M. A. Revilla, Programming Challenges, Springer,
 *      2003, chapters 6 and 11
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

#ifndef __ADS_GridPaths
 #define __ADS_GridPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a square of a matrix: its side and its upper left cell
struct GridSquare {
 std::size_t side = 0;         ///< the side, 0 for none
 std::size_t row = 0;          ///< the row of the upper left cell
 std::size_t col = 0;          ///< the column of the upper left cell
 };

/// the largest square of nonzero cells of A, the first in the order of the
/// rows of its lower right cell
/// \pre A a rectangle
/// \post all the cells of the square inside A and nonzero
template< class T >
GridSquare largestSquare( const std::vector< std::vector< T > > & A );

/// the number of the paths from the upper left to the lower right cell of
/// B, each step right or down, through nonzero cells only; 0 for B empty
/// \pre B a rectangle
template< class C = std::uint64_t , class T >
C countLatticePaths( const std::vector< std::vector< T > > & B );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/GridPaths.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* GridPaths.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File GridPaths.h --------------------------*/
/*--------------------------------------------------------------------------*/
