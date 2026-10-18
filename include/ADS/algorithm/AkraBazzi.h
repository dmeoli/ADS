/*--------------------------------------------------------------------------*/
/*---------------------------- File AkraBazzi.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The recurrences of divide et impera with parts of different sizes,
 * \f$ T( n ) = \sum_{i=1}^k a_i T( b_i n ) + g( n ) \f$, a_i > 0 and
 * 0 < b_i < 1, which the master theorem [see MasterTheorem.h] does not
 * cover when the b_i differ, e.g. \f$ T( n ) = T( n / 5 ) + T( 7 n / 10 )
 * + n \f$ of the selection in linear time.
 *
 * By the theorem of Akra and Bazzi, with p the unique real root of
 * \f$ \sum_i a_i b_i^p = 1 \f$,
 * \f[
 *   T( n ) = \Theta\left( n^p \left( 1 + \int_1^n
 *            \frac{g( u )}{u^{p+1}} du \right) \right) ,
 * \f]
 * and for \f$ g( n ) = \Theta( n^\beta \log^k n ) \f$ the integral gives
 * the cases of the master theorem with p for \f$ \log_b a \f$:
 * \f$ \Theta( n^p ) \f$ if \f$ \beta < p \f$,
 * \f$ \Theta( n^\beta \log^k n ) \f$ if \f$ \beta > p \f$, and for
 * \f$ \beta = p \f$ \f$ \Theta( n^p \log^{k+1} n ) \f$ if k > -1,
 * \f$ \Theta( n^p \log \log n ) \f$ if k = -1, \f$ \Theta( n^p ) \f$ if
 * k < -1. The floors and the ceilings of the sizes, and perturbations of
 * \f$ O( n / \log^2 n ) \f$, do not change the order (Leighton).
 *
 * akraBazziExponent() finds p by bisection: \f$ f( p ) = \sum_i a_i b_i^p
 * \f$ is strictly decreasing, so a bracket found by doubling and 200
 * halvings give p to the precision of a double; akraBazzi() then returns
 * the order as a Growth.
 *
 * The contracts [see Contract.h]: at least one term, a_i > 0 and
 * 0 < b_i < 1; p is a root of the equation to 1e-9. The functions are
 * defined in AkraBazzi.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see M. Akra, L. Bazzi, On the solution of linear recurrence equations,
 *      Computational Optimization and Applications 10(2), 195-210, 1998,
 *      doi:10.1023/A:1018373005182
 * \see T. Leighton, Notes on better master theorems for divide-and-conquer
 *      recurrences, MIT, 1996
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

#ifndef __ADS_AkraBazzi
 #define __ADS_AkraBazzi
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/algorithm/MasterTheorem.h"

#include <cmath>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the exponent p of Akra and Bazzi of T( n ) = sum_i a_i T( b_i n ) +
/// g( n ), the root of sum_i a_i b_i^p = 1, the terms given as ( a_i , b_i )
/// \pre at least one term, a_i > 0 and 0 < b_i < 1
/// \post sum_i a_i b_i^p = 1 within 1e-9
inline double akraBazziExponent(
                      const std::vector< std::pair< double , double > > & ab );

/// T( n ) = sum_i a_i T( b_i n ) + Theta( n^beta log^k n )
/// \pre at least one term, a_i > 0 and 0 < b_i < 1
inline Growth akraBazzi(
                  const std::vector< std::pair< double , double > > & ab ,
                  double beta , double k = 0 );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/AkraBazzi.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AkraBazzi.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File AkraBazzi.h --------------------------*/
/*--------------------------------------------------------------------------*/
