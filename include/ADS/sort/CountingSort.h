/*--------------------------------------------------------------------------*/
/*-------------------------- File CountingSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Counting Sort (section 2.7.5) of integers in [ 0 , k ): B[ v ] counts the
 * occurrences of v, then each v is written B[ v ] times in order;
 * \f$ \Theta( n + k ) \f$ time and \f$ \Theta( k ) \f$ space, linear when
 * \f$ k = O( n ) \f$. The lower bound \f$ \Omega( n \log n ) \f$ of the sorts
 * by comparisons does not apply, since it compares no item: it uses that the
 * keys are small integers. A key out of [ 0 , k ) throws std::out_of_range.
 *
 * The contracts [see Contract.h]: each key must be in [ 0 , k ), a
 * precondition checked key by key as the keys are counted; the output sorted
 * and made of the same keys is an audit. The functions are defined in
 * CountingSort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_CountingSort
 #define __ADS_CountingSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// countingSort(): A, of integers in [ 0 , k ), in increasing order
/// \pre each key of A is in [ 0 , k )
/// \post A is sorted, with the same keys
template< class T >
void countingSort( std::vector< T > & A , std::size_t k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/CountingSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CountingSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File CountingSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
