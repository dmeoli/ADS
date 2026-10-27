/*--------------------------------------------------------------------------*/
/*--------------------------- File MonteCarlo.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Monte Carlo algorithms of chapter 17, quick and wrong with a bounded
 * probability, which \f$ K \f$ independent runs lower to its \f$ K \f$-th
 * power:
 *
 * - `zeroPolynomial()`, whether a polynomial of total degree \f$ d \f$, given
 *   as a function of its \f$ n \f$ variables, is identically 0: by the lemma
 *   of Schwartz and Zippel a random point of \f$ \{ 1 , \ldots , 2 d \}^n \f$
 *   is a root of a nonzero one with probability at most \f$ 1 / 2 \f$; the
 *   answer zero may be wrong, the answer not zero never;
 * - `freivalds()`, whether \f$ A B = C \f$ for \f$ n \times n \f$ matrices:
 *   \f$ A ( B x ) \f$ against \f$ C x \f$ for a random \f$ x \f$ of
 *   \f$ \pm 1 \f$, \f$ O( n^2 ) \f$ per run instead of the product, wrong
 *   with probability at most \f$ 1 / 2 \f$ when \f$ A B \ne C \f$ (exercise
 *   17.4);
 * - `halfSatisfiable()`, whether a boolean formula of \f$ n \f$ variables is
 *   true on more than half of the assignments, from the fraction of the true
 *   ones among \f$ K \f$ random assignments (exercise 17.5): by the bound of
 *   Hoeffding it errs with probability at most \f$ e^{-2 K \epsilon^2} \f$
 *   when the true fraction is \f$ 1 / 2 + \epsilon \f$ or
 *   \f$ 1 / 2 - \epsilon \f$.
 *
 * The contracts [see `Contract.h`]: \f$ K \ge 0 \f$ repetitions, and
 * `freivalds()` needs three \f$ n \times n \f$ matrices. The functions are
 * defined in `MonteCarlo.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 17 and
 *      exercises 17.4 and 17.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf
 * \see J. T. Schwartz, Fast probabilistic algorithms for verification of
 *      polynomial identities, Journal of the ACM 27(4), 701-717, 1980,
 *      doi:10.1145/322217.322225
 * \see R. Zippel, Probabilistic algorithms for sparse polynomials,
 *      EUROSAM 1979, LNCS 72, 216-226, 1979, doi:10.1007/3-540-09519-5_73
 * \see R. Freivalds, Probabilistic machines can use less running time,
 *      IFIP Congress 1977, 839-842
 * \see W. Hoeffding, Probability inequalities for sums of bounded random
 *      variables, Journal of the American Statistical Association
 *      58(301), 13-30, 1963, doi:10.1080/01621459.1963.10500830
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

#ifndef __ADS_MonteCarlo
 #define __ADS_MonteCarlo
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether \f$ p \f$, of \f$ n \f$ variables and total degree \f$ d \f$, is
/// identically 0, by \f$ K \f$ random points
///
/// In pseudocode (chapter 17, the lemma of Schwartz and Zippel), one-sided:
/// a point where \f$ p \ne 0 \f$ proves that \f$ p \f$ is not zero:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{zeroPolynomial}( p , n , d , K ) \cr
/// 2 & \quad \textbf{repeat}\ K \text{ times} \cr
/// 3 & \qquad x \gets \text{a point of } \{ 1 , \ldots , 2 \max( d , 1 )
///     \}^n \text{ at random} \cr
/// 4 & \qquad \textbf{if}\ p( x ) \ne 0\ \textbf{then return false} \cr
/// 5 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ K \ge 0 \f$
template< class F , class R >
bool zeroPolynomial( F && p , std::size_t n , long d , int K , R & rng );

/// whether \f$ A B = C \f$, by \f$ K \f$ runs of \f$ O( n^2 ) \f$
///
/// In pseudocode (exercise 17.4, the algorithm of Freivalds):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{freivalds}( A , B , C , K ) \cr
/// 2 & \quad \textbf{repeat}\ K \text{ times} \cr
/// 3 & \qquad x \gets \text{a vector of } n \text{ random items of }
///     \{ -1 , +1 \} \cr
/// 4 & \qquad \textbf{if}\ A ( B x ) \ne C x\ \textbf{then return false} \cr
/// 5 & \quad \textbf{return true}
/// \end{array}}
/// \f]
/// \pre \f$ A \f$, \f$ B \f$ and \f$ C \f$ are \f$ n \times n \f$, and
///      \f$ K \ge 0 \f$
template< class T , class R >
bool freivalds( const std::vector< std::vector< T > > & A ,
                const std::vector< std::vector< T > > & B ,
                const std::vector< std::vector< T > > & C , int K , R & rng );

/// whether the formula \f$ f \f$ of \f$ n \f$ variables is true on more than
/// half of the assignments, from \f$ K \f$ random ones
///
/// In pseudocode (exercise 17.5), a majority of \f$ K \f$ samples:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{halfSatisfiable}( f , n , K ) \cr
/// 2 & \quad y \gets 0 \cr
/// 3 & \quad \textbf{repeat}\ K \text{ times} \cr
/// 4 & \qquad x \gets n \text{ random bits};\ \textbf{if}\ f( x )
///     \ \textbf{then}\ y \gets y + 1 \cr
/// 5 & \quad \textbf{return}\ 2 y > K
/// \end{array}}
/// \f]
/// \pre \f$ K \ge 0 \f$
template< class F , class R >
bool halfSatisfiable( F && f , std::size_t n , int K , R & rng );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/MonteCarlo.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MonteCarlo.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MonteCarlo.h --------------------------*/
/*--------------------------------------------------------------------------*/
