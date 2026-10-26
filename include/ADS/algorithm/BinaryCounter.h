/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryCounter.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary counter of k bits of the amortized analysis: A[ 0 ] is the
 * least significant bit, and add() increments the counter modulo
 * \f$ 2^k \f$. One increment changes up to k bits, but n increments change
 * at most
 * \f[
 *   \sum_{i=0}^{k-1} \left\lfloor \frac{n}{2^i} \right\rfloor
 *   < n \sum_{i=0}^{\infty} \frac{1}{2^i} = 2 n
 * \f]
 * bits, since A[ i ] changes every \f$ 2^i \f$ increments: the amortized cost
 * of an increment is \f$ O( 1 ) \f$. The same bound comes from the potential
 * \f$ \Phi \f$ = the number of bits equal to 1: an increment that clears d
 * bits and sets one costs d + 1 and changes \f$ \Phi \f$ by 1 - d, hence its
 * amortized cost is 2.
 *
 * The contracts [see Contract.h]: the counter holds only bits, an audit, and
 * add() changes at most k of them. The functions are defined in
 * BinaryCounter.tpp.
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

/// add(): increments the counter of A.size() bits modulo 2^k and returns the
/// number of bits it has changed, i.e., its actual cost
/// \pre the items of A are bits
/// \post at most k bits changed
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
