/*--------------------------------------------------------------------------*/
/*--------------------------- File KnightTour.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tour of the knight (exercise 16.6): a path of the knight that visits
 * every square of an n x n board once. cavallo() of the book tries the 8
 * moves in a fixed order and backtracks from the dead ends, which on an
 * 8 x 8 board may take very long; the rule of Warnsdorff tries first the
 * moves to the squares with the fewest onward moves, and with it the
 * backtrack seldom happens, a tour of a board of hundreds of squares in
 * linear time. Both return the board with the step of each square, from 1,
 * or nothing if there is no tour from the given square.
 *
 * The contracts [see Contract.h]: the start is on the board, and the tour
 * found moves as a knight from it (checked at level 3). The operations are
 * defined in KnightTour.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 16.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
 * \see H. C. von Warnsdorf, Des Roesselsprunges einfachste und
 *      allgemeinste Loesung, Schmalkalden, 1823
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

/// a tour of the knight from ( x , y ) on an n x n board, by backtrack,
/// the moves in the order of the book or by the rule of Warnsdorff
/// \pre ( x , y ) on the board
/// \post a tour of knight moves from ( x , y )
inline std::optional< std::vector< std::vector< int > > > knightTour(
                                    int n , int x , int y ,
                                    bool warnsdorff = true );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/KnightTour.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnightTour.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KnightTour.h --------------------------*/
/*--------------------------------------------------------------------------*/
