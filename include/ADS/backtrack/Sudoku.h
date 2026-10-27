/*--------------------------------------------------------------------------*/
/*----------------------------- File Sudoku.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Sudoku (reality check of section 16.3 and exercise 16.7), of size
 * \f$ n^2 \times n^2 \f$ with the numbers \f$ 1 , \ldots , n^2 \f$,
 * \f$ n = 3 \f$ the classic one: the squares are filled in order, each with
 * the numbers that `check()` allows, no equal number on its row, its column
 * and its block of \f$ n \times n \f$, and the search backtracks when a square
 * has none. The cost is a constant for \f$ n = 3 \f$, a huge one, and
 * superpolynomial in \f$ n \f$. 0 marks an empty square.
 *
 * The contracts [see `Contract.h`]: `check()` needs a square of the grid and a
 * value in \f$ [ 1 , n^2 ] \f$; `sudoku()` a grid of \f$ n^2 \times n^2 \f$
 * squares with values in \f$ [ 0 , n^2 ] \f$, those given consistent, and the
 * solution is checked too (both at level 3). The operations are defined in
 * `Sudoku.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.3 and
 *      exercise 16.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Sudoku
 #define __ADS_Sudoku
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include "ADS/Contract.h"

#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Grid = std::vector< std::vector< int > >;   ///< the squares, 0 empty

/// whether \f$ c \f$ can be written in the square \f$ ( x , y ) \f$ of
/// \f$ S \f$, i.e., it is not on its row, its column or its block
///
/// In pseudocode (section 16.3), \f$ N = n^2 \f$ the side of the grid:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{check}( S , x , y , c ) \cr
/// 2 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ N - 1\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ S[ x ][ j ] = c\ \textbf{or}\ S[ j ][ y ] = c
///     \ \textbf{then return false}\quad \triangleright\ \text{the row, the}
///     \text{ column} \cr
/// 4 & \quad b_x \gets n \lfloor x / n \rfloor;\ b_y \gets n \lfloor y / n
///     \rfloor\quad \triangleright\ \text{the corner of the block} \cr
/// 5 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 6 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 7 & \qquad\quad \textbf{if}\ S[ b_x + i ][ b_y + j ] = c\ \textbf{then
///     return false} \cr
/// 8 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ ( x , y ) \f$ is a square and \f$ c \in [ 1 , n^2 ] \f$
inline bool check( const Grid & S , std::size_t x , std::size_t y , int c );

/// fills the empty squares of \f$ S \f$ from the square number \f$ i \f$ on,
/// in the order of the rows; true if it can
///
/// In pseudocode (section 16.3), \f$ N = n^2 \f$ the side of the grid and the
/// square \f$ i \f$ that of row \f$ \lfloor i / N \rfloor \f$ and column
/// \f$ i \bmod N \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sudoku}( S , i ) \cr
/// 2 & \quad \textbf{if}\ i = N^2\ \textbf{then return true}\quad
///     \triangleright\ \text{every square filled} \cr
/// 3 & \quad x \gets \lfloor i / N \rfloor;\ y \gets i \bmod N \cr
/// 4 & \quad \textbf{if}\ S[ x ][ y ] \ne 0\ \textbf{then return}
///     \ \mathrm{sudoku}( S , i + 1 )\quad \triangleright\ \text{a number}
///     \text{ given} \cr
/// 5 & \quad \textbf{for}\ c \gets 1\ \textbf{to}\ N\ \textbf{do} \cr
/// 6 & \qquad \textbf{if}\ \mathrm{check}( S , x , y , c )\ \textbf{then} \cr
/// 7 & \qquad\quad S[ x ][ y ] \gets c \cr
/// 8 & \qquad\quad \textbf{if}\ \mathrm{sudoku}( S , i + 1 )\ \textbf{then
///     return true} \cr
/// 9 & \quad S[ x ][ y ] \gets 0\quad \triangleright\ \text{no number fits:}
///     \text{ back} \cr
/// 10 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre a grid of \f$ n^2 \times n^2 \f$ squares, the values given
///      consistent, \f$ i \le n^4 \f$
/// \post if true, a solution (audit)
inline bool sudoku( Grid & S , std::size_t i = 0 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/Sudoku.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Sudoku.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sudoku.h ----------------------------*/
/*--------------------------------------------------------------------------*/
