/*--------------------------------------------------------------------------*/
/*---------------------------- File CoinGame.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The game of the coins in a line: n coins of values V_0 .. V_{n-1} lie in
 * a row, and two players take in turn the first or the last coin left,
 * each one to get the most; coinsInALine() is the most the first player
 * can be sure of, against any play of the other.
 *
 * With D[ i ][ j ] the largest difference between the coins of the player
 * to move and those of the other on V[ i .. j - 1 ],
 * \f[
 *   D[ i ][ j ] = \max( V_i - D[ i + 1 ][ j ] , V_{j-1} - D[ i ][ j - 1 ] ) ,
 * \f]
 * D[ i ][ i ] = 0, and the first player gets ( S + D[ 0 ][ n ] ) / 2, S the
 * sum of the coins; the table filled by increasing length keeps one row,
 * \f$ \Theta( n^2 ) \f$ time and \f$ \Theta( n ) \f$ space. The scheme is
 * the minimax on the intervals; the form with two moves ahead,
 * \f$ M[ i ][ j ] = \max( V_i + \min( M[ i + 2 ][ j ] , M[ i + 1 ][ j - 1 ] )
 * , \ldots ) \f$, gives the same value.
 *
 * For n even the first player never loses: the coins of even places and
 * those of odd places can each be taken all by the first player, who
 * chooses the larger set; this strategy, though, is not optimal in
 * general.
 *
 * The contracts [see Contract.h]: none on the values, which may be
 * negative. The functions are defined in CoinGame.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see P. Winkler, Mathematical Puzzles: A Connoisseur's Collection,
 *      A K Peters, 2004
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

#ifndef __ADS_CoinGame
 #define __ADS_CoinGame
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the most the first player can be sure of in the game of the coins in a
/// line V, both players taking in turn the first or the last coin
template< class T >
T coinsInALine( const std::vector< T > & V );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/CoinGame.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CoinGame.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CoinGame.h ---------------------------*/
/*--------------------------------------------------------------------------*/
