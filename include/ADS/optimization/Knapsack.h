/*--------------------------------------------------------------------------*/
/*---------------------------- File Knapsack.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The 0-1 knapsack of section 13.6: n objects of profit p_i and volume
 * v_i, positive integers, and a knapsack of capacity C; a set S of volume
 * at most C and largest profit. DP[ i ][ c ], the best profit of the first
 * i objects in capacity c, is \f$ -\infty \f$ if c < 0, 0 if i = 0 or
 * c = 0, and \f$ \max\{ DP[ i - 1 ][ c ] , DP[ i - 1 ][ c - v_{i-1} ] +
 * p_{i-1} \} \f$ otherwise. The volumes here may also be 0, and then the
 * column c = 0 follows the same maximum instead of being 0.
 *
 * knapsack() fills the table bottom up in Theta( n C ) time and reads the
 * set back; zaino() is the memoization of the book, top down, which solves
 * only the subproblems it reaches, in a hash table per object, of key c,
 * rather than in a table of n C places, whose initialization alone would
 * cost Theta( n C ); so any capacity up to the largest long is allowed.
 * Either is pseudopolynomial: C takes log C bits of the input.
 *
 * The contracts [see Contract.h]: profits and volumes of the same objects,
 * none negative, and the capacity not negative; the objects of knapsack() fit
 * and give the profit found. The operations are defined in Knapsack.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
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
/// \pre \f$ |p| = |v| \f$, \f$ p_i , v_i \ge 0 \f$, \f$ C \ge 0 \f$, the
///      \f$ C + 1 \f$ capacities within the largest vector
/// \post the objects fit and give the profit
inline std::pair< long , std::vector< std::size_t > > knapsack(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C );

/// zaino(): the best profit by memoization in a hash table; the number of
/// subproblems solved goes to *solved, if given
/// \pre |p| = |v|, p , v >= 0, C >= 0
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
