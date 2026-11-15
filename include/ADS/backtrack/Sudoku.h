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

namespace ADS
{

using Grid = std::vector< std::vector< int > >;  ///< the squares, 0 empty

/// whether \f$ c \f$ can be written in the square \f$ ( x , y ) \f$ of
/// \f$ S \f$, i.e., it is not on its row, its column or its block
///
/// In pseudocode (section 16.3), \f$ N = n^2 \f$ the side of the grid:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{check}( S , x , y , c )\quad
///     \triangleright\ N = n^2 \text{, the blocks } n \times n \cr
/// 2 & \quad \textbf{if}\ c \text{ is in the row } x
///     \text{ or in the column } y\ \textbf{then return false} \cr
/// 3 & \quad ( b_x , b_y ) \gets ( n \lfloor x / n \rfloor , n \lfloor y / n
///     \rfloor )\quad \triangleright
///     \ \text{the upper left square of the block} \cr
/// 4 & \quad \textbf{return}\ \text{whether } c
///     \text{ is in none of the squares } S[ b_x + i ][ b_y + j ] \text{, }
///     0 \le i , j < n
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
/// 1 & \textbf{function}\ \mathrm{sudoku}( S , i )\quad \triangleright
///     \ \text{the squares in the order of the rows} \cr
/// 2 & \quad \textbf{if}\ i = N^2\ \textbf{then return true} \cr
/// 3 & \quad ( x , y ) \gets ( \lfloor i / N \rfloor , i \bmod N ) \cr
/// 4 & \quad \textbf{if}\ S[ x ][ y ] \ne 0\ \textbf{then return}
///     \ \mathrm{sudoku}( S , i + 1 )\quad \triangleright
///     \ \text{a given number stays} \cr
/// 5 & \quad \textbf{foreach}\ c \in [ 1 , N ] \text{ with } \mathrm{check}(
///     S , x , y , c )\ \textbf{do} \cr
/// 6 & \qquad S[ x ][ y ] \gets c;\ \textbf{if}\ \mathrm{sudoku}( S , i + 1
///     )\ \textbf{then return true} \cr
/// 7 & \quad S[ x ][ y ] \gets 0;\ \textbf{return false}\quad
///     \triangleright\ \text{the square is emptied on the way back}
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

#endif /* Sudoku.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sudoku.h ----------------------------*/
/*--------------------------------------------------------------------------*/
