/*--------------------------------------------------------------------------*/
/*----------------------------- File Minimum.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum of a vector A[ 0 .. n - 1 ] (section 1.2): min() keeps the
 * minimum so far and compares it with each of the other items, n - 1
 * comparisons, which are needed, since each comparison can exclude at most
 * one of the two items from being the minimum (section 2.6). argmin()
 * gives the place of the minimum of A[ start .. end ], the first one if it
 * occurs more than once.
 *
 * The contracts [see Contract.h]: argmin() needs start <= end < n and min() a
 * vector that is not empty; that no item of the range is smaller than the one
 * found is an audit, as costly as the search. The functions are defined in
 * Minimum.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and 2.7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
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

#ifndef __ADS_Minimum
 #define __ADS_Minimum
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// argmin(): the place of the first minimum of A[ start .. end ]
/// \pre start <= end < n
/// \post no item of A[ start .. end ] is smaller than A[ pos ]
template< class T , class C = std::less<> >
std::size_t argmin( const std::vector< T > & A , std::size_t start ,
                    std::size_t end , C less = C() );

/// min(): the minimum of a vector that is not empty, n - 1 comparisons
/// \pre A is not empty
template< class T , class C = std::less<> >
const T & min( const std::vector< T > & A , C less = C() );

/// the minimum and the maximum together, by pairs: the two items of a pair
/// are compared, the smaller with the minimum and the larger with the
/// maximum, \f$ \lceil 3 n / 2 \rceil - 2 \f$ comparisons instead of the
/// 2 ( n - 1 ) of two searches
/// \pre A is not empty
/// \post the minimum and the maximum of A (an audit)
template< class T , class C = std::less<> >
std::pair< T , T > minMax( const std::vector< T > & A , C less = C() );

/// minMax() by divide et impera: the halves solved recursively and their
/// minima and maxima compared, T( 2 ) = 1, T( n ) = 2 T( n / 2 ) + 2, i.e.,
/// 3 n / 2 - 2 comparisons for n a power of 2
/// \pre A is not empty
/// \post the minimum and the maximum of A (an audit)
template< class T , class C = std::less<> >
std::pair< T , T > minMaxDivide( const std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/Minimum.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Minimum.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Minimum.h ---------------------------*/
/*--------------------------------------------------------------------------*/
