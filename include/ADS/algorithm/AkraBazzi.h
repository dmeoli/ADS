/*--------------------------------------------------------------------------*/
/*---------------------------- File AkraBazzi.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The recurrences of divide et impera with parts of different sizes,
 * \f$ T( n ) = \sum_{i=1}^k a_i T( b_i n ) + g( n ) \f$, \f$ a_i > 0 \f$
 * and \f$ 0 < b_i < 1 \f$, which the master theorem [see `MasterTheorem.h`]
 * does not cover when the \f$ b_i \f$ differ, e.g.,
 * \f$ T( n ) = T( n / 5 ) + T( 7 n / 10 ) + n \f$ of the selection in linear
 * time.
 *
 * By the theorem of Akra and Bazzi, with \f$ p \f$ the unique real root of
 * \f$ \sum_i a_i b_i^p = 1 \f$,
 * \f[
 *   T( n ) = \Theta\left( n^p \left( 1 + \int_1^n
 *            \frac{g( u )}{u^{p+1}} du \right) \right) ,
 * \f]
 * and for \f$ g( n ) = \Theta( n^\beta \log^k n ) \f$ the integral gives
 * the cases of the master theorem with \f$ p \f$ for \f$ \log_b a \f$:
 * \f$ \Theta( n^p ) \f$ if \f$ \beta < p \f$,
 * \f$ \Theta( n^\beta \log^k n ) \f$ if \f$ \beta > p \f$, and for
 * \f$ \beta = p \f$ \f$ \Theta( n^p \log^{k+1} n ) \f$ if \f$ k > -1 \f$,
 * \f$ \Theta( n^p \log \log n ) \f$ if \f$ k = -1 \f$, \f$ \Theta( n^p ) \f$
 * if \f$ k < -1 \f$. The floors and the ceilings of the sizes, and
 * perturbations of \f$ O( n / \log^2 n ) \f$, do not change the order
 * (Leighton).
 *
 * `akraBazziExponent()` finds \f$ p \f$ by bisection: the function
 * \f$ f( p ) = \sum_i a_i b_i^p \f$ is strictly decreasing, so a bracket
 * found by doubling and 200 halvings give \f$ p \f$ to the precision of a
 * `double`; `akraBazzi()` then returns the order as a `Growth`.
 *
 * The contracts [see `Contract.h`]: at least one term, \f$ a_i > 0 \f$ and
 * \f$ 0 < b_i < 1 \f$; \f$ p \f$ is a root of the equation to \f$ 10^{-9} \f$.
 * The functions are defined in `AkraBazzi.tpp`.
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
 *         Università di Pisa \n
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

/// the exponent \f$ p \f$ of Akra and Bazzi of
/// \f$ T( n ) = \sum_i a_i T( b_i n ) + g( n ) \f$, the root of
/// \f$ \sum_i a_i b_i^p = 1 \f$, the terms given as \f$ ( a_i , b_i ) \f$
///
/// In pseudocode (the bisection, beyond the book), the function
/// \f$ f( p ) = \sum_i a_i b_i^p \f$ being strictly decreasing:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{akraBazziExponent}( ( a_i , b_i )_{i=1}^k
///     ) \cr
/// 2 & \quad \mathit{lo} \gets -1;\ \textbf{while}\ f( \mathit{lo} ) < 1
///     \ \textbf{do}\ \mathit{lo} \gets 2 \mathit{lo} \cr
/// 3 & \quad \mathit{hi} \gets 1;\ \textbf{while}\ f( \mathit{hi} ) > 1
///     \ \textbf{do}\ \mathit{hi} \gets 2 \mathit{hi} \cr
/// 4 & \quad \textbf{repeat}\ 200 \text{ times} \cr
/// 5 & \qquad m \gets ( \mathit{lo} + \mathit{hi} ) / 2 \cr
/// 6 & \qquad \textbf{if}\ f( m ) > 1\ \textbf{then}\ \mathit{lo} \gets m
///     \ \textbf{else}\ \mathit{hi} \gets m \cr
/// 7 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \pre at least one term, \f$ a_i > 0 \f$ and \f$ 0 < b_i < 1 \f$
/// \post \f$ | \sum_i a_i b_i^p - 1 | < 10^{-9} \f$
inline double akraBazziExponent(
                      const std::vector< std::pair< double , double > > & ab );

/// the order of \f$ T( n ) = \sum_i a_i T( b_i n ) + g( n ) \f$ with
/// \f$ g( n ) = \Theta( n^\beta \log^k n ) \f$
///
/// In pseudocode (the theorem of Akra and Bazzi), the cases of `master()`
/// with \f$ p \f$ in place of \f$ \log_b a \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{akraBazzi}( ( a_i , b_i )_{i=1}^k , \beta ,
///     k ) \cr
/// 2 & \quad p \gets \mathrm{akraBazziExponent}( ( a_i , b_i )_{i=1}^k ) \cr
/// 3 & \quad \textbf{if}\ \beta < p\ \textbf{then return}\ \Theta( n^p ) \cr
/// 4 & \quad \textbf{if}\ \beta > p\ \textbf{then return}\ \Theta( n^\beta
///     \log^k n ) \cr
/// 5 & \quad \textbf{if}\ k > -1\ \textbf{then return}\ \Theta( n^p
///     \log^{k+1} n ) \cr
/// 6 & \quad \textbf{if}\ k = -1\ \textbf{then return}\ \Theta( n^p \log
///     \log n ) \cr
/// 7 & \quad \textbf{return}\ \Theta( n^p )
/// \end{array}}
/// \f]
/// \pre at least one term, \f$ a_i > 0 \f$ and \f$ 0 < b_i < 1 \f$
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
