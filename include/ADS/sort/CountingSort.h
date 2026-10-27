/*--------------------------------------------------------------------------*/
/*-------------------------- File CountingSort.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Counting Sort (section 2.7.5) of integers in \f$ [ 0 , k ) \f$:
 * \f$ B[ v ] \f$ counts the occurrences of \f$ v \f$, then each \f$ v \f$ is
 * written \f$ B[ v ] \f$ times in order; \f$ \Theta( n + k ) \f$ time and
 * \f$ \Theta( k ) \f$ space, linear when \f$ k = O( n ) \f$. The lower bound
 * \f$ \Omega( n \log n ) \f$ of the sorts by comparisons does not apply, since
 * it compares no item: it uses that the keys are small integers.
 *
 * The contracts [see `Contract.h`]: each key must be in \f$ [ 0 , k ) \f$, a
 * precondition checked key by key as the keys are counted (a key out of the
 * range throws `contract_violation` when the preconditions are checked); the
 * output sorted and made of the same keys is an audit. The functions are
 * defined in `CountingSort.tpp`.
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

/// \f$ A \f$, of integers in \f$ [ 0 , k ) \f$, in increasing order, by
/// counting the occurrences of each key
///
/// In pseudocode (section 2.7.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{countingSort}( A , n , k ) \cr
/// 2 & \quad \textbf{for}\ v \gets 0\ \textbf{to}\ k - 1\ \textbf{do}\ B[ v ]
///     \gets 0 \cr
/// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ B[ A[
///     i ] ] \gets B[ A[ i ] ] + 1 \cr
/// 4 & \quad j \gets 0 \cr
/// 5 & \quad \textbf{for}\ v \gets 0\ \textbf{to}\ k - 1\ \textbf{do} \cr
/// 6 & \qquad \textbf{while}\ B[ v ] > 0\ \textbf{do} \cr
/// 7 & \qquad\quad A[ j ] \gets v;\ j \gets j + 1;\ B[ v ] \gets B[ v ] - 1
/// \end{array}}
/// \f]
/// \pre each key of \f$ A \f$ is in \f$ [ 0 , k ) \f$
/// \post \f$ A \f$ is sorted, with the same keys (audit)
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
