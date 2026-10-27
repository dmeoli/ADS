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

namespace ADS {

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
///     \triangleright\ \text{called as } \mathrm{queens}( 0 , \emptyset ,
///     \emptyset , \emptyset ) \cr
/// 2 & \quad \textbf{if}\ r = n\ \textbf{then return}\ f( Q ) \cr
/// 3 & \quad \textbf{foreach}\ c \in \{ 0 , \ldots , n - 1 \} \setminus ( C
///     \cup D_1 \cup D_2 )\ \textbf{do}\quad \triangleright\ \text{in }
///     \text{increasing order} \cr
/// 4 & \qquad Q[ r ] \gets c \cr
/// 5 & \qquad D'_1 \gets \{ x + 1 : x \in D_1 \cup \{ c \} , x + 1 < n \};
///     \ D'_2 \gets \{ x - 1 : x \in D_2 \cup \{ c \} , x \ge 1 \} \cr
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
/// 1 & \textbf{function}\ \mathrm{nQueensIterative}( n ) \cr
/// 2 & \quad \textbf{for}\ k \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ V[ k ]
///     \gets -1 \cr
/// 3 & \quad k \gets 0 \cr
/// 4 & \quad \textbf{while}\ k \ge 0\ \textbf{do} \cr
/// 5 & \qquad \textbf{repeat}\ V[ k ] \gets V[ k ] + 1\ \textbf{until}\ V[ k ]
///     = n\ \textbf{or}\ \mathrm{free}( k ) \cr
/// 6 & \qquad \textbf{if}\ V[ k ] = n\ \textbf{then}\ V[ k ] \gets -1;\ k
///     \gets k - 1\quad \triangleright\ \text{the row is over: back} \cr
/// 7 & \qquad \textbf{else if}\ k = n - 1\ \textbf{then} \cr
/// 8 & \qquad\quad \textbf{if}\ f( V )\ \textbf{then return}\quad
///     \triangleright\ \text{a solution} \cr
/// 9 & \qquad \textbf{else}\ k \gets k + 1 \cr
/// 10 & \textbf{function}\ \mathrm{free}( k ) \cr
/// 11 & \quad \textbf{return}\ V[ h ] \ne V[ k ] \text{ and } | V[ h ] - V[ k
///      ] | \ne k - h \text{ for each } h < k
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
/// 2 & \quad \textbf{foreach}\ i < j\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ Q[ i ] = Q[ j ]\ \textbf{or}\ | Q[ i ] - Q[ j ] | =
///     j - i\ \textbf{then return false} \cr
/// 4 & \quad \textbf{return true}
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

#endif  /* NQueens.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File NQueens.h ---------------------------*/
/*--------------------------------------------------------------------------*/
