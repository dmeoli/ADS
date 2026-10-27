/*--------------------------------------------------------------------------*/
/*---------------------------- File Knapsack.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The 0-1 knapsack of section 13.6: \f$ n \f$ objects of profit \f$ p_i \f$
 * and volume \f$ v_i \f$, positive integers, and a knapsack of capacity
 * \f$ C \f$; a set \f$ S \f$ of volume at most \f$ C \f$ and largest profit.
 * \f$ \mathit{DP}[ i ][ c ] \f$, the best profit of the first \f$ i \f$
 * objects in capacity \f$ c \f$, is \f$ -\infty \f$ if \f$ c < 0 \f$, 0 if
 * \f$ i = 0 \f$ or \f$ c = 0 \f$, and
 * \f$ \max\{ \mathit{DP}[ i - 1 ][ c ] , \mathit{DP}[ i - 1 ][ c - v_{i-1} ]
 * + p_{i-1} \} \f$ otherwise. The volumes here may also be 0, and then the
 * column \f$ c = 0 \f$ follows the same maximum instead of being 0.
 *
 * `knapsack()` fills the table bottom up in \f$ \Theta( n C ) \f$ time and
 * reads the set back; `zaino()` is the memoization of the book, top down,
 * which solves only the subproblems it reaches, in a hash table per object,
 * of key \f$ c \f$, rather than in a table of \f$ n C \f$ places, whose
 * initialization alone would cost \f$ \Theta( n C ) \f$; so any capacity up to
 * the largest `long` is allowed. Either is pseudopolynomial: \f$ C \f$ takes
 * \f$ \log C \f$ bits of the input.
 *
 * The contracts [see `Contract.h`]: profits and volumes of the same objects,
 * none negative, and the capacity not negative; the objects of `knapsack()`
 * fit and give the profit found. The operations are defined in
 * `Knapsack.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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

#ifndef __ADS_Knapsack
 #define __ADS_Knapsack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/ChainedHashTable.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the best profit and a set of objects, by places, of the 0-1 knapsack
///
/// In pseudocode (section 13.6), the table filled by rows and read back:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{knapsack}( p , v , C ) \cr
/// 2 & \quad \mathit{DP}[ 0 ][ 0 .. C ] \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ c \gets 0\ \textbf{to}\ C\ \textbf{do} \cr
/// 5 & \qquad\quad \mathit{DP}[ i ][ c ] \gets \mathit{DP}[ i - 1 ][ c ] \cr
/// 6 & \qquad\quad \textbf{if}\ v_{i-1} \le c\ \textbf{then}\ \mathit{DP}[ i
///     ][ c ] \gets \max\{ \mathit{DP}[ i ][ c ] , \mathit{DP}[ i - 1 ][ c -
///     v_{i-1} ] + p_{i-1} \} \cr
/// 7 & \quad S \gets \emptyset;\ c \gets C \cr
/// 8 & \quad \textbf{for}\ i \gets n\ \textbf{downto}\ 1\ \textbf{do} \cr
/// 9 & \qquad \textbf{if}\ \mathit{DP}[ i ][ c ] \ne \mathit{DP}[ i - 1 ][ c
///     ]\ \textbf{then}\ S \gets S \cup \{ i - 1 \};\ c \gets c - v_{i-1} \cr
/// 10 & \quad \textbf{return}\ ( \mathit{DP}[ n ][ C ] , S )
/// \end{array}}
/// \f]
/// \pre \f$ |p| = |v| \f$, \f$ p_i , v_i \ge 0 \f$, \f$ C \ge 0 \f$, the
///      \f$ C + 1 \f$ capacities within the largest vector
/// \post the objects fit and give the profit
inline std::pair< long , std::vector< std::size_t > > knapsack(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C );

/// the best profit by memoization in a hash table (the `zaino()` of the book);
/// the number of subproblems solved goes to `*solved`, if given
///
/// In pseudocode (section 13.6), \f$ T_i \f$ the hash table of the object
/// \f$ i \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zaino}( i , c ) \cr
/// 2 & \quad \textbf{if}\ c < 0\ \textbf{then return}\ -\infty \cr
/// 3 & \quad \textbf{if}\ i = 0\ \textbf{then return}\ 0 \cr
/// 4 & \quad \textbf{if}\ c \in T_i\ \textbf{then return}\ T_i[ c ] \cr
/// 5 & \quad r \gets \max\{ \mathrm{zaino}( i - 1 , c ) , \mathrm{zaino}( i -
///     1 , c - v_{i-1} ) + p_{i-1} \} \cr
/// 6 & \quad T_i[ c ] \gets r;\ \textbf{return}\ r
/// \end{array}}
/// \f]
/// called as \f$ \mathrm{zaino}( n , C ) \f$.
/// \pre \f$ |p| = |v| \f$, \f$ p_i , v_i \ge 0 \f$, \f$ C \ge 0 \f$
inline long zaino( const std::vector< long > & p ,
                   const std::vector< long > & v , long C ,
                   long * solved = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/optimization/impl/Knapsack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Knapsack.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Knapsack.h ---------------------------*/
/*--------------------------------------------------------------------------*/
