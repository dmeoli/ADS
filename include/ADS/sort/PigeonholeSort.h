/*--------------------------------------------------------------------------*/
/*------------------------- File PigeonholeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Pigeonhole Sort (exercise 4.11) of the pairs ( key , value ) with keys in [
 * 0 , k ): the value of each pair goes at the end of the LIST of its key, then
 * the lists are read in the order of the keys; \f$ O( n + k ) \f$ time, and
 * stable, unlike Counting Sort, which only counts the keys.
 *
 * The contracts [see Contract.h]: k >= 0 and each key must be in [ 0 , k ),
 * checked key by key, and the output has as many pairs as the input, its
 * keys sorted. The functions are defined in PigeonholeSort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.11,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_PigeonholeSort
 #define __ADS_PigeonholeSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the pairs sorted by key, the values of a key in their first order
/// \pre k >= 0 and each key of A is in [ 0 , k )
/// \post as many pairs as in A, the keys sorted and each key with its
///       items in their order
template< class V >
std::vector< std::pair< int , V > > pigeonholeSort(
                    const std::vector< std::pair< int , V > > & A , int k );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/PigeonholeSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PigeonholeSort.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PigeonholeSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
