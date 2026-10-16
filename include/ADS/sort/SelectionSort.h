/*--------------------------------------------------------------------------*/
/*-------------------------- File SelectionSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Selection Sort (section 2.7.1): the minimum of the part still to sort
 * A[ i .. n - 1 ] is swapped with A[ i ], extending the sorted prefix;
 * \f$ \sum_{i=0}^{n-2} ( n - i - 1 ) = n ( n - 1 ) / 2 \f$ comparisons
 * whatever the order of the input, \f$ \Theta( n^2 ) \f$ time, O( 1 )
 * space, not stable.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
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

#ifndef __ADS_SelectionSort
 #define __ADS_SelectionSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/Minimum.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// SelectionSort(): A in increasing order by less
template< class T , class C = std::less<> >
void selectionSort( std::vector< T > & A , C less = C() )
{
 for( std::size_t i = 0 ; i + 1 < A.size() ; ++i ) {
  const std::size_t pos = argmin( A , i , A.size() - 1 , less );
  std::swap( A[ i ] , A[ pos ] );            // extends the sorted prefix
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* SelectionSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SelectionSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
