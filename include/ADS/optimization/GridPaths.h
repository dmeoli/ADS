/*--------------------------------------------------------------------------*/
/*---------------------------- File GridPaths.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two problems of dynamic programming on a matrix of \f$ m \f$ rows and
 * \f$ n \f$ columns, each cell solved from its neighbours above and to the
 * left:
 *
 * - `largestSquare()`: the largest square of nonzero cells of a 0/1 matrix.
 *   With \f$ S[ i ][ j ] \f$ the side of the largest one with its lower right
 *   corner in \f$ ( i , j ) \f$, \f$ S[ i ][ j ] = 0 \f$ on a zero cell and
 *   otherwise 1 plus the least of \f$ S[ i - 1 ][ j ] \f$,
 *   \f$ S[ i - 1 ][ j - 1 ] \f$ and \f$ S[ i ][ j - 1 ] \f$, since the three
 *   squares cover all the square of side one more but its corner;
 *   \f$ \Theta( m n ) \f$ time, \f$ \Theta( n ) \f$ space keeping one row. The
 *   largest rectangle of ones takes \f$ \Theta( m n ) \f$ too, by the largest
 *   rectangle in the histogram of each row;
 * - `countLatticePaths()`: the monotone lattice paths from the upper left to
 *   the lower right cell, a step right or down, through the nonzero cells:
 *   \f$ N[ i ][ j ] = N[ i - 1 ][ j ] + N[ i ][ j - 1 ] \f$ on a free cell and
 *   0 on a blocked one, \f$ \Theta( m n ) \f$ time and \f$ \Theta( n ) \f$
 *   space. With no blocked cell they are \f$ \binom{m + n - 2}{m - 1} \f$;
 *   with few blocked cells, the inclusion-exclusion on the blocked cells
 *   sorted takes \f$ O( b^2 ) \f$ binomials for \f$ b \f$ of them.
 *
 * The contracts [see `Contract.h`]: the matrix is a rectangle; the square
 * found is inside the matrix and all nonzero (checked at level 3). The
 * functions are defined in `GridPaths.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13, the
 *      Esercizi vari and the Esercizi Programmazione dinamica,
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

/// the largest square of nonzero cells of \f$ A \f$, the first in the order
/// of the rows of its lower right cell
///
/// In pseudocode (exercise B3 of 10/09/2012, Max square, of the Esercizi
/// vari of the book), one row \f$ S[ 1 .. n ] \f$ of the table kept,
/// \f$ \mathit{diag} \f$ the entry above and to the left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{largestSquare}( A ) \cr
/// 2 & \quad S[ 0 .. n ] \gets 0;\ \mathit{best} \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
/// 4 & \qquad \mathit{diag} \gets 0 \cr
/// 5 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 6 & \qquad\quad u \gets S[ j + 1 ] \cr
/// 7 & \qquad\quad \textbf{if}\ A[ i ][ j ] \ne 0\ \textbf{then}\ S[ j + 1 ]
///     \gets 1 + \min\{ u , \mathit{diag} , S[ j ] \} \cr
/// 8 & \qquad\quad \textbf{else}\ S[ j + 1 ] \gets 0 \cr
/// 9 & \qquad\quad \mathit{diag} \gets u \cr
/// 10 & \qquad\quad \textbf{if}\ S[ j + 1 ] > \mathit{best}\ \textbf{then}
///     \ \mathit{best} \gets S[ j + 1 ] \text{, at } ( i , j ) \cr
/// 11 & \quad \textbf{return}\ \mathit{best} \text{ and its upper left cell}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is a rectangle
/// \post all the cells of the square inside \f$ A \f$ and nonzero (audit)
template< class T >
GridSquare largestSquare( const std::vector< std::vector< T > > & A );

/// the number of the paths from the upper left to the lower right cell of
/// \f$ B \f$, each step right or down, through nonzero cells only; 0 for
/// \f$ B \f$ empty
///
/// In pseudocode (exercise B3 of 07/01/2013, Conta percorsi, of the Esercizi
/// Programmazione dinamica of the book), one row \f$ N[ 0 .. n - 1 ] \f$ of
/// the table kept, each entry adding the one on its left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{countLatticePaths}( B ) \cr
/// 2 & \quad N[ 0 ] \gets 1;\ N[ 1 .. n - 1 ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad\quad \textbf{if}\ B[ i ][ j ] = 0\ \textbf{then}\ N[ j ] \gets 0
///     \cr
/// 6 & \qquad\quad \textbf{else if}\ j > 0\ \textbf{then}\ N[ j ] \gets N[ j ]
///     + N[ j - 1 ] \cr
/// 7 & \quad \textbf{return}\ N[ n - 1 ]
/// \end{array}}
/// \f]
/// \pre \f$ B \f$ is a rectangle
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
