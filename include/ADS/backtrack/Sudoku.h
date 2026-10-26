/*--------------------------------------------------------------------------*/
/*----------------------------- File Sudoku.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Sudoku (reality check of section 16.3 and exercise 16.7), of size
 * n^2 x n^2 with the numbers 1 , ... , n^2, n = 3 the classic one: the
 * squares are filled in order, each with the numbers that check() allows,
 * no equal number on its row, its column and its block of n x n, and the
 * search backtracks when a square has none. The cost is a constant for
 * n = 3, a huge one, and superpolynomial in n. 0 marks an empty square.
 *
 * The contracts [see Contract.h]: check() needs a square of the grid and a
 * value in [ 1 , n^2 ]; sudoku() a grid of n^2 x n^2 squares with values in [
 * 0 , n^2 ], those given consistent, and the solution is checked too (both at
 * level 3). The operations are defined in Sudoku.tpp.
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

/// check(): c can be written in the square ( x , y )
/// \pre ( x , y ) a square and c in [ 1 , n^2 ]
inline bool check( const Grid & S , std::size_t x , std::size_t y , int c );

/// sudoku(): fills the empty squares of S, true if it can
/// \pre a grid of n^2 x n^2 squares, the values given consistent
/// \post if true, a solution
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
