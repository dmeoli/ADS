/*--------------------------------------------------------------------------*/
/*--------------------------- File KnightTour.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tour of the knight (exercise 16.6): a path of the knight that visits
 * every square of an \f$ n \times n \f$ board once. The procedure of the book
 * tries the 8 moves in a fixed order and backtracks from the dead ends, which
 * on an \f$ 8 \times 8 \f$ board may take very long; the rule of Warnsdorff
 * tries first the moves to the squares with the fewest onward moves, and with
 * it the backtrack seldom happens, a tour of a board of hundreds of squares in
 * linear time. Both return the board with the step of each square, from 1, or
 * nothing if there is no tour from the given square.
 *
 * The contracts [see `Contract.h`]: the start is on the board, and the tour
 * found moves as a knight from it (checked at level 3). The operations are
 * defined in `KnightTour.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see H. C. von Warnsdorf, Des Roesselsprunges einfachste und
 *      allgemeinste Loesung, Schmalkalden, 1823
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

#ifndef __ADS_KnightTour
 #define __ADS_KnightTour
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include "ADS/Contract.h"

#include <array>
#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a tour of the knight from \f$ ( x , y ) \f$ on an \f$ n \times n \f$
/// board, by backtrack, the moves in the order of the book or by the rule of
/// Warnsdorff, as the board of the steps from 1; none for \f$ n < 1 \f$,
/// where there is no board
///
/// In pseudocode (exercise 16.6), \f$ A \f$ the board, 0 on the squares not
/// visited yet:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{knightTour}( n , x , y ) \cr
/// 2 & \quad A \gets \text{an } n \times n \text{ board of } 0 \cr
/// 3 & \quad \textbf{if}\ \mathrm{tour}( 0 , x , y )\ \textbf{then return}\ A
///     \ \textbf{else return nil} \cr
/// 4 & \textbf{function}\ \mathrm{tour}( k , a , b ) \cr
/// 5 & \quad A[ a ][ b ] \gets k + 1 \cr
/// 6 & \quad \textbf{if}\ k = n^2 - 1\ \textbf{then return true} \cr
/// 7 & \quad M \gets \text{the 8 moves to a free square of the board, in the}
///     \text{ order of the book} \cr
/// 8 & \quad \textbf{if}\ \mathit{warnsdorff}\ \textbf{then}\ \text{sort } M
///     \text{ stably by the free squares one move away from each} \cr
/// 9 & \quad \textbf{foreach}\ ( a' , b' ) \in M\ \textbf{do} \cr
/// 10 & \qquad \textbf{if}\ \mathrm{tour}( k + 1 , a' , b' )\ \textbf{then
///      return true} \cr
/// 11 & \quad A[ a ][ b ] \gets 0\quad \triangleright\ \text{a dead end: back}
///      \cr
/// 12 & \quad \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ ( x , y ) \f$ is on the board if \f$ n \ge 1 \f$
/// \post a tour of moves of the knight from \f$ ( x , y ) \f$ (audit)
inline std::optional< std::vector< std::vector< int > > > knightTour(
                                    int n , int x , int y ,
                                    bool warnsdorff = true );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/KnightTour.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnightTour.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KnightTour.h --------------------------*/
/*--------------------------------------------------------------------------*/
