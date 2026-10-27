/*--------------------------------------------------------------------------*/
/*--------------------------- File MatrixChain.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multiplication of a chain of matrices of section 13.3: \f$ A_i \f$ of
 * size \f$ c[ i ] \times c[ i + 1 ] \f$, \f$ 0 \le i < n \f$, and the order of
 * the products with the fewest scalar multiplications. The number of
 * parenthesizations is the number of Catalan \f$ C_{n-1} \f$ [see
 * `Catalan.h`], \f$ \Omega( 4^n / n^{3/2} ) \f$, but
 * \f[
 *   \mathit{DP}[ i ][ j ] = \min_{i \leq k < j} \{ \mathit{DP}[ i ][ k ] +
 *   \mathit{DP}[ k + 1 ][ j ] + c[ i ] c[ k + 1 ] c[ j + 1 ] \} , \qquad
 *   \mathit{DP}[ i ][ i ] = 0 ,
 * \f]
 * filled by increasing length \f$ h = j - i + 1 \f$ in \f$ O( n^3 ) \f$ time,
 * with \f$ S[ i ][ j ] \f$ the best \f$ k \f$, from which `parenthesization()`
 * (the stampaPar() of the book) writes the parenthesization in
 * \f$ O( n ) \f$.
 *
 * The contracts [see `Contract.h`]: the vector of the sizes
 * \f$ c_0 , \ldots , c_n \f$ has \f$ n + 1 \ge 1 \f$ positive items, and the
 * parenthesization needs \f$ i \le j < n \f$. The functions are defined in
 * `MatrixChain.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see T. C. Hu, M. T. Shing, Computation of matrix chain products, part I,
 *      SIAM Journal on Computing 11(2), 362-373, 1982,
 *      doi:10.1137/0211028
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

#ifndef __ADS_MatrixChain
 #define __ADS_MatrixChain
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the least cost \f$ \mathit{DP}[ 0 ][ n - 1 ] \f$ and the table \f$ S \f$
/// of the splits, for the \f$ n \f$ matrices of sizes \f$ c[ 0 .. n ] \f$ (the
/// parentesizzazione() of the book)
///
/// In pseudocode (section 13.3), by increasing length of the chain:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{matrixChain}( c )\quad \triangleright
///     \ n = |c| - 1 \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ \mathit{DP}[ i ][ i ] \gets 0
///     \cr
/// 3 & \quad \textbf{for}\ h \gets 2\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - h\ \textbf{do} \cr
/// 5 & \qquad\quad j \gets i + h - 1;\ \mathit{DP}[ i ][ j ] \gets \infty \cr
/// 6 & \qquad\quad \textbf{for}\ k \gets i\ \textbf{to}\ j - 1\ \textbf{do}
///     \cr
/// 7 & \qquad\qquad t \gets \mathit{DP}[ i ][ k ] + \mathit{DP}[ k + 1 ][ j
///     ] + c[ i ] c[ k + 1 ] c[ j + 1 ] \cr
/// 8 & \qquad\qquad \textbf{if}\ t < \mathit{DP}[ i ][ j ]\ \textbf{then}
///     \ \mathit{DP}[ i ][ j ] \gets t;\ S[ i ][ j ] \gets k \cr
/// 9 & \quad \textbf{return}\ ( \mathit{DP}[ 0 ][ n - 1 ] , S )
/// \end{array}}
/// \f]
/// \pre \f$ c \f$ is not empty and its items are positive (audit)
inline std::pair< long , std::vector< std::vector< std::size_t > > >
matrixChain( const std::vector< long > & c );

/// the parenthesization of \f$ A_i \cdots A_j \f$ by the splits \f$ S \f$,
/// e.g., `((A0·A1)·A2)` (the stampaPar() of the book)
///
/// In pseudocode (section 13.3):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{parenthesization}( S , i , j ) \cr
/// 2 & \quad \textbf{if}\ i = j\ \textbf{then return}\ \text{the name }
///     A_i \cr
/// 3 & \quad k \gets S[ i ][ j ] \cr
/// 4 & \quad \textbf{return}\ ( \mathrm{parenthesization}( S , i , k ) \cdot
///     \mathrm{parenthesization}( S , k + 1 , j ) )
/// \end{array}}
/// \f]
/// \pre \f$ i \le j < n \f$
inline std::string parenthesization(
                         const std::vector< std::vector< std::size_t > > & S ,
                         std::size_t i , std::size_t j );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/MatrixChain.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MatrixChain.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MatrixChain.h -------------------------*/
/*--------------------------------------------------------------------------*/
