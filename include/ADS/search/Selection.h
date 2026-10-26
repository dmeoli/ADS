/*--------------------------------------------------------------------------*/
/*---------------------------- File Selection.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The selection of section 17.1: the item of rank k, the k-th smallest,
 * 1 <= k <= n, the median for k = ceil( n / 2 ), without sorting.
 *
 * - selection(), of Hoare: perno() puts its pivot in its place j, the
 *   q = j - start + 1 smallest items before it, and the search goes on in
 *   the part that holds rank k; with the pivot chosen at random, swapped
 *   to A[ start ] before perno(), the expected number of comparisons
 *   satisfies \f$ T( n ) \leq n + \frac{2}{n} \sum_{q = \lfloor n / 2
 *   \rfloor}^{n-1} T( q ) \leq c n \f$, linear for every input;
 * - medianOfMedians(), of Blum, Floyd, Pratt, Rivest and Tarjan, linear in
 *   the worst case: the pivot is the median of the medians of the groups of
 *   5, which has at least 3 n / 10 items on each side, so that
 *   \f$ T( n ) \leq T( n / 5 ) + T( 7 n / 10 ) + O( n ) = O( n ) \f$.
 *
 * Both rearrange A.
 *
 * The contracts [see Contract.h]: 1 <= k <= n, and the item returned has rank
 * k, i.e., fewer than k items are smaller and at least k are not larger, an
 * audit in O( n ). The functions are defined in Selection.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 17.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see C. A. R. Hoare, Algorithm 65: Find, Communications of the ACM 4(7),
 *      321-322, 1961, doi:10.1145/366622.366647
 * \see M. Blum, R. W. Floyd, V. Pratt, R. L. Rivest, R. E. Tarjan, Time
 *      bounds for selection, Journal of Computer and System Sciences 7(4),
 *      448-461, 1973, doi:10.1016/S0022-0000(73)80033-9
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

#ifndef __ADS_Selection
 #define __ADS_Selection
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/QuickSort.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// selezione(): the item of rank k, 1 <= k <= n, of A; with a generator
/// the pivots are random, otherwise the first item of the part
/// \pre 1 <= k <= n
/// \post the result has rank k in A
template< class T , class R = std::mt19937 >
T selection( std::vector< T > & A , std::size_t k , R * rng = nullptr );

/// the item of rank k, 1 <= k <= n, in linear time in the worst case
/// \pre 1 <= k <= n
/// \post the result has rank k in A
template< class T >
T medianOfMedians( std::vector< T > & A , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/Selection.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Selection.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Selection.h --------------------------*/
/*--------------------------------------------------------------------------*/
