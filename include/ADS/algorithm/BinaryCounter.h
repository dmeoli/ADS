/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryCounter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary counter of \f$ k \f$ bits of the amortized analysis:
 * \f$ A[ 0 ] \f$ is the least significant bit, and `add()` increments the
 * counter modulo \f$ 2^k \f$. One increment changes up to \f$ k \f$ bits, but
 * \f$ n \f$ increments change at most
 * \f[
 *   \sum_{i=0}^{k-1} \left\lfloor \frac{n}{2^i} \right\rfloor
 *   < n \sum_{i=0}^{\infty} \frac{1}{2^i} = 2 n
 * \f]
 * bits, since \f$ A[ i ] \f$ changes every \f$ 2^i \f$ increments: the
 * amortized cost of an increment is \f$ O( 1 ) \f$. The same bound comes from
 * the potential \f$ \Phi \f$, the number of bits equal to 1: an increment that
 * clears \f$ d \f$ bits and sets one costs \f$ d + 1 \f$ and changes
 * \f$ \Phi \f$ by \f$ 1 - d \f$, hence its amortized cost is 2.
 *
 * The contracts [see `Contract.h`]: the counter holds only bits, an audit, and
 * `add()` changes at most \f$ k \f$ of them. The functions are defined in
 * `BinaryCounter.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.2.1,
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

#ifndef __ADS_BinaryCounter
 #define __ADS_BinaryCounter
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the increment of the counter \f$ A \f$ of \f$ k \f$ bits modulo
/// \f$ 2^k \f$, returning the number of bits it has changed, i.e., its actual
/// cost
///
/// In pseudocode (section 4.2.1), the carry running over the ones:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{add}( A[ 0 .. k - 1 ] ) \cr
/// 2 & \quad i \gets 0 \cr
/// 3 & \quad \textbf{while}\ i < k\ \textbf{and}\ A[ i ] = 1\ \textbf{do} \cr
/// 4 & \qquad A[ i ] \gets 0;\ i \gets i + 1 \cr
/// 5 & \quad \textbf{if}\ i < k\ \textbf{then}\ A[ i ] \gets 1;
///     \ \textbf{return}\ i + 1 \cr
/// 6 & \quad \textbf{return}\ k
/// \end{array}}
/// \f]
/// \pre the items of \f$ A \f$ are bits (audit)
/// \post at most \f$ k \f$ bits changed
inline std::size_t add( std::vector< int > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/BinaryCounter.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BinaryCounter.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryCounter.h ------------------------*/
/*--------------------------------------------------------------------------*/
