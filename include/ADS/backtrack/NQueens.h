/*--------------------------------------------------------------------------*/
/*----------------------------- File NQueens.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The \f$ n \f$ queens (exercise 16.5): \f$ n \f$ queens on an
 * \f$ n \times n \f$ board, no two on a row, a column or a diagonal. A row
 * holds exactly one queen, and so the solution is the vector \f$ Q \f$ of
 * their columns, a permutation: \f$ n! \f$ candidates instead of the
 * \f$ 2^{n^2} \f$ placements; the backtrack prunes the columns and the two
 * diagonals already taken, kept as masks of bits, so that a choice is checked
 * in \f$ O( 1 ) \f$. The numbers of solutions are 1, 0, 0, 2, 10, 4, 40, 92,
 * 352, 724 for \f$ n = 1 , \ldots , 10 \f$.
 *
 * The contracts [see `Contract.h`]: \f$ 1 \le n \le 32 \f$, the solutions are
 * admissible (checked at level 3), and `board()` needs a column of the board
 * for each row. The operations are defined in `NQueens.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see J. Bell, B. Stevens, A survey of known results and research areas
 *      for n-queens, Discrete Mathematics 309(1), 1-31, 2009,
 *      doi:10.1016/j.disc.2007.12.043
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

namespace ADS
{

/// the placements of \f$ n \le 32 \f$ queens, `f( Q )` on each, \f$ Q[ r ] \f$
/// the column of the queen of the row \f$ r \f$, and true from `f` to stop;
/// the number found
///
/// In pseudocode (exercise 16.5), the columns and the two diagonals attacked
/// on the row \f$ r \f$ kept as sets \f$ C \f$, \f$ D_1 \f$ and \f$ D_2 \f$
/// (masks of bits in the code), which move by one column at each row:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{queens}( r , C , D_1 , D_2 )\quad
///     \triangleright\ \text{the columns of the row } r
///     \text{ that the queens above attack} \cr
/// 2 & \quad \textbf{if}\ r = n\ \textbf{then return}\ f( Q ) \cr
/// 3 & \quad \textbf{foreach}\ c \notin C \cup D_1 \cup D_2 \text{, from } 0
///     \text{ to } n - 1\ \textbf{do} \cr
/// 4 & \qquad Q[ r ] \gets c\quad \triangleright
///     \ \text{the diagonals move one column at the next row} \cr
/// 5 & \qquad D'_1 \gets \{ x + 1 : x \in D_1 \cup \{ c \} \} \cap [ 0 , n
///     );\ D'_2 \gets \{ x - 1 : x \in D_2 \cup \{ c \} \} \cap [ 0 , n ) \cr
/// 6 & \qquad \textbf{if}\ \mathrm{queens}( r + 1 , C \cup \{ c \} , D'_1 ,
///     D'_2 )\ \textbf{then return true} \cr
/// 7 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ 1 \le n \le 32 \f$
/// \post each solution is admissible (audit)
template< class F >
long nQueens( int n , F && f );

/// the placements of \f$ n \f$ queens by the iterative backtracking, with the
/// stage \f$ k \f$ and the column \f$ V[ k ] \f$ of the queen of the row
/// \f$ k \f$, `f( V )` on each and true from `f` to stop; the number found
///
/// In pseudocode (exercise 16.5, without recursion): \f$ V[ k ] \f$ goes to
/// the next column not attacked, \f$ k \f$ goes on when one is found and back
/// when the columns of the row \f$ k \f$ are over.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{nQueensIterative}( n )\quad
///     \triangleright\ V[ k ] \text{ the column of row } k \text{, } -1
///     \text{ none yet} \cr
/// 2 & \quad V \gets ( -1 , \ldots , -1 );\ k \gets 0 \cr
/// 3 & \quad \textbf{while}\ k \ge 0\ \textbf{do} \cr
/// 4 & \qquad V[ k ] \gets \text{the first column after } V[ k ]
///     \text{ with } \mathrm{free}( k ) \text{, or } n \text{ if none} \cr
/// 5 & \qquad \textbf{if}\ V[ k ] = n\ \textbf{then}\ V[ k ] \gets -1;\ k
///     \gets k - 1\quad \triangleright\ \text{back to the row above} \cr
/// 6 & \qquad \textbf{else if}\ k < n - 1\ \textbf{then}\ k \gets k + 1 \cr
/// 7 & \qquad \textbf{else if}\ f( V )\ \textbf{then return}\quad
///     \triangleright\ \text{a solution, and the visitor stops} \cr
/// 8 & \textbf{function}\ \mathrm{free}( k )\quad \triangleright
///     \ \text{no queen above shares a column or a diagonal} \cr
/// 9 & \quad \textbf{return}\ V[ h ] \ne V[ k ] \text{ and } | V[ k ] - V[ h
///     ] | \ne k - h \text{ for every } h < k
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 1 \f$
/// \post each solution is admissible (audit)
template< class F >
long nQueensIterative( int n , F && f );

/// whether no two queens of \f$ Q \f$ attack each other, \f$ Q[ r ] \f$ the
/// column of the queen of the row \f$ r \f$
///
/// In pseudocode (exercise 16.5), in \f$ O( n^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{admissible}( Q ) \cr
/// 2 & \quad \textbf{return}\ \text{whether } Q[ i ] \ne Q[ j ] \text{ and }
///     | Q[ j ] - Q[ i ] | \ne j - i \text{ for every } i < j
/// \end{array}}
/// \f]
inline bool admissible( const std::vector< int > & Q );

/// the board of \f$ Q \f$, a line per row, with the letter Q for a queen and
/// a dot for an empty square
/// \pre each column in \f$ [ 0 , n ) \f$
inline std::string board( const std::vector< int > & Q );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/NQueens.tpp"

/*--------------------------------------------------------------------------*/

#endif /* NQueens.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File NQueens.h ---------------------------*/
/*--------------------------------------------------------------------------*/
