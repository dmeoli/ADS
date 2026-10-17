/*--------------------------------------------------------------------------*/
/*----------------------------- File NQueens.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The n queens (exercise 16.5): n queens on an n x n board, no two on a row, a
 * column or a diagonal. A row holds exactly one queen, and so the solution is
 * the vector Q of their columns, a permutation: n! candidates instead of
 * 2^{n^2} placements; the backtrack prunes the columns and the two diagonals
 * already taken, kept as masks of bits, so that a choice is checked in O( 1 ).
 * The numbers of solutions are 1, 0, 0, 2, 10, 4, 40, 92, 352, 724 for n = 1 ,
 * ... , 10.
 *
 * The contracts [see Contract.h]: 1 <= n <= 32, the solutions are admissible
 * (checked at level 3), and board() needs a column of the board for each row.
 * The operations are defined in NQueens.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see J. Bell, B. Stevens, A survey of known results and research areas
 *      for n-queens, Discrete Mathematics 309(1), 1-31, 2009,
 *      doi:10.1016/j.disc.2007.12.043
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

#ifndef __ADS_NQueens
 #define __ADS_NQueens
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>
#include "ADS/Contract.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the placements of n <= 32 queens, f( Q ) on each, Q[ r ] the column of
/// the queen of row r; f returns true to stop; the number found
/// \pre 1 <= n <= 32
/// \post each solution admissible
template< class F >
long nQueens( int n , F && f );

/// the placements of n queens by the iterative backtracking, with the
/// stage k and the column V[ k ] of the queen of row k: V[ k ] goes to the
/// next column not attacked, k goes on when one is found and back when the
/// columns of row k are over; f( V ) on each, true to stop; the number
/// found
/// \pre n >= 1
/// \post each solution admissible
template< class F >
long nQueensIterative( int n , F && f );

/// admissible(): no two queens attack each other
inline bool admissible( const std::vector< int > & Q );

/// the board, a line per row, "Q" a queen and "." an empty square
/// \pre each column in [ 0 , n )
inline std::string board( const std::vector< int > & Q );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/NQueens.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* NQueens.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File NQueens.h ---------------------------*/
/*--------------------------------------------------------------------------*/
