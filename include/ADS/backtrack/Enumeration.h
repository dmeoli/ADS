/*--------------------------------------------------------------------------*/
/*--------------------------- File Enumeration.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The enumeration of the feasible solutions by backtrack (section 16.2):
 * a solution is a vector S[ 0 .. n - 1 ], each S[ i ] taken from the
 * choices allowed by S[ 0 .. i - 1 ]; each choice is tried, the solutions
 * are processed, and the search goes on recursively from S[ 0 .. i ], a
 * depth first visit of the tree of the choices. The processing returns
 * true to stop at the first solution.
 *
 * - subsets(), the subsets of k of the n items, by the choices in or out,
 *   pruned when k items are in or when the items left are too few;
 * - permutations() and derangements(), each item not chosen yet, the
 *   latter not in its own place (exercises 16.2 and 16.3);
 * - subsetSum(), the subsets of positive items of sum k, pruned when the
 *   sum exceeds k (exercise 16.4).
 *
 * Each calls f( S ) on the solutions, in increasing order of the choices,
 * and returns how many there are, or stops when f returns true. The
 * subsets of k items are \f$ \binom{n}{k} = n! / ( k! ( n - k )! ) \f$ (the
 * text writes n! / ( n - k )!, the number of their orderings).
 *
 * The contracts [see Contract.h]: n and k are not negative, and the items of
 * subsetSum() are positive. The operations are defined in Enumeration.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 16.2 and
 *      exercises 16.2-16.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf
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

#ifndef __ADS_Enumeration
 #define __ADS_Enumeration
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include "ADS/Contract.h"

#include <algorithm>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the subsets of k of the items 0 , ... , n - 1, as vectors of n booleans;
/// f( S ) returns true to stop
/// \pre n , k >= 0
template< class F >
long subsets( int n , int k , F && f );

namespace detail {

template< class T , class F >
long permute( const std::vector< T > & A , bool derange , F && f );

}  // end( namespace detail )

/// the permutations of the items, f( S ) on each, f returns true to stop;
/// the items are told apart by their places, so equal items give each
/// permutation of the values more than once, n! in all
template< class T , class F >
long permutations( const std::vector< T > & A , F && f );

/// the derangements, no item in its own place, f( S ) on each, f returns
/// true to stop; the places are deranged, not the values, as above
template< class T , class F >
long derangements( const std::vector< T > & A , F && f );

/// the subsets of the positive items of sum k, as the places taken, f
/// returns true to stop; k = 0 gives the empty subset only, equal items
/// in different places give different subsets
/// \pre the items positive
template< class T , class F >
long subsetSum( const std::vector< T > & A , T k , F && f );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/Enumeration.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Enumeration.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Enumeration.h -------------------------*/
/*--------------------------------------------------------------------------*/
