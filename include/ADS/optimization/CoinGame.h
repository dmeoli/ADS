/*--------------------------------------------------------------------------*/
/*---------------------------- File CoinGame.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The game of the coins in a line: \f$ n \f$ coins of values
 * \f$ V_0 , \ldots , V_{n-1} \f$ lie in a row, and two players take in turn
 * the first or the last coin left, each one to get the most;
 * `coinsInALine()` is the most the first player can be sure of, against any
 * play of the other.
 *
 * With \f$ D[ i ][ j ] \f$ the largest difference between the coins of the
 * player to move and those of the other on \f$ V[ i .. j - 1 ] \f$,
 * \f[
 *   D[ i ][ j ] = \max( V_i - D[ i + 1 ][ j ] , V_{j-1} - D[ i ][ j - 1 ] ) ,
 * \f]
 * \f$ D[ i ][ i ] = 0 \f$, and the first player gets
 * \f$ ( S + D[ 0 ][ n ] ) / 2 \f$, \f$ S \f$ the sum of the coins; the table
 * filled by increasing length keeps one row, \f$ \Theta( n^2 ) \f$ time and
 * \f$ \Theta( n ) \f$ space. The scheme is the minimax on the intervals; the
 * form with two moves ahead,
 * \f$ M[ i ][ j ] = \max( V_i + \min( M[ i + 2 ][ j ] ,
 * M[ i + 1 ][ j - 1 ] ) , \ldots ) \f$, gives the same value.
 *
 * For \f$ n \f$ even the first player never loses: the coins of even places
 * and those of odd places can each be taken all by the first player, who
 * chooses the larger set; this strategy, though, is not optimal in general.
 *
 * The contracts [see `Contract.h`]: none on the values, which may be
 * negative. The functions are defined in `CoinGame.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13 and the
 *      Esercizi Programmazione dinamica, exercise B4 of 22/07/2013,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see P. Winkler, Mathematical Puzzles: A Connoisseur's Collection,
 *      A K Peters, 2004
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
/// line \f$ V \f$, both players taking in turn the first or the last coin
///
/// In pseudocode (exercise B4 of 22/07/2013, Gioco di monete, of the Esercizi
/// Programmazione dinamica of the book), \f$ D[ i ] \f$ holding
/// \f$ D[ i ][ i + h ] \f$ for the current length \f$ h \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{coinsInALine}( V[ 0 .. n - 1 ] ) \cr
/// 2 & \quad D[ 0 .. n ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ h \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - h\ \textbf{do} \cr
/// 5 & \qquad\quad D[ i ] \gets \max\{ V[ i ] - D[ i + 1 ] , V[ i + h - 1 ]
///     - D[ i ] \} \cr
/// 6 & \quad \textbf{return}\ ( V[ 0 ] + \cdots + V[ n - 1 ] + D[ 0 ] ) / 2
/// \end{array}}
/// \f]
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
