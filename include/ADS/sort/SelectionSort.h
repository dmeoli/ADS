/*--------------------------------------------------------------------------*/
/*-------------------------- File SelectionSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Selection Sort (section 2.7.1): the minimum of the part still to sort
 * \f$ A[ i .. n - 1 ] \f$ is swapped with \f$ A[ i ] \f$, extending the sorted
 * prefix; \f$ \sum_{i=0}^{n-2} ( n - i - 1 ) = n ( n - 1 ) / 2 \f$
 * comparisons whatever the order of the input, \f$ \Theta( n^2 ) \f$ time,
 * \f$ O( 1 ) \f$ space, not stable.
 *
 * The contracts [see `Contract.h`]: the vector ends with the same items
 * sorted, an audit. The functions are defined in `SelectionSort.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.7.1,
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

#ifndef __ADS_SelectionSort
 #define __ADS_SelectionSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/Minimum.h"
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

/// \f$ A \f$ in increasing order by `less`, by selection of the minimum
///
/// In pseudocode (section 2.7.1), \f$ A[ 0 .. i - 1 ] \f$ holding the
/// \f$ i \f$ smallest items in order:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{selectionSort}( A , n ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 2\ \textbf{do} \cr
/// 3 & \qquad j \gets \mathrm{argmin}( A , i , n - 1 ) \cr
/// 4 & \qquad A[ i ] \leftrightarrow A[ j ]
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void selectionSort( std::vector< T > & A , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/SelectionSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SelectionSort.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SelectionSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
