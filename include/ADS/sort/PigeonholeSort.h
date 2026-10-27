/*--------------------------------------------------------------------------*/
/*------------------------- File PigeonholeSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Pigeonhole Sort (exercise 4.11) of the pairs
 * \f$ ( \mathit{key} , \mathit{value} ) \f$ with keys in \f$ [ 0 , k ) \f$:
 * the value of each pair goes at the end of the LIST of its key, then the
 * lists are read in the order of the keys; \f$ O( n + k ) \f$ time, and
 * stable, unlike Counting Sort, which only counts the keys.
 *
 * The contracts [see `Contract.h`]: \f$ k \ge 0 \f$ and each key must be in
 * \f$ [ 0 , k ) \f$, checked key by key, and the output has as many pairs as
 * the input, its keys sorted. The functions are defined in
 * `PigeonholeSort.tpp`.
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
///
/// In pseudocode (exercise 4.11), one LIST per key:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{pigeonholeSort}( A , k ) \cr
/// 2 & \quad \textbf{for}\ v \gets 0\ \textbf{to}\ k - 1\ \textbf{do}\ B[ v ]
///     \gets \text{an empty list} \cr
/// 3 & \quad \textbf{foreach}\ ( \mathit{key} , x ) \in A\ \textbf{do}
///     \ \text{append } x \text{ to } B[ \mathit{key} ] \cr
/// 4 & \quad R \gets \text{an empty vector} \cr
/// 5 & \quad \textbf{for}\ v \gets 0\ \textbf{to}\ k - 1\ \textbf{do} \cr
/// 6 & \qquad \textbf{foreach}\ x \in B[ v ] \text{ from its head}
///     \ \textbf{do}\ \text{append } ( v , x ) \text{ to } R \cr
/// 7 & \quad \textbf{return}\ R
/// \end{array}}
/// \f]
/// \pre \f$ k \ge 0 \f$ and each key of \f$ A \f$ is in \f$ [ 0 , k ) \f$
/// \post as many pairs as in \f$ A \f$, the keys sorted and each key with its
///       items in their order (audit)
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
