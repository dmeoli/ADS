/*--------------------------------------------------------------------------*/
/*----------------------------- File Catalan.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The numbers of Catalan, 1, 1, 2, 5, 14, 42, and so on: \f$ C_n \f$ counts
 * the binary trees of \f$ n \f$ nodes (so the binary search trees on
 * \f$ n \f$ keys, the trees whose symmetric visit is \f$ 1 , \ldots , n \f$),
 * the strings of \f$ n \f$ pairs of balanced parentheses, the full binary
 * trees of \f$ n + 1 \f$ leaves (the parenthesizations of a product of
 * \f$ n + 1 \f$ factors), the triangulations of a convex polygon of
 * \f$ n + 2 \f$ sides, and the monotone lattice paths that never cross the
 * diagonal.
 *
 * `catalan()` fills the table of the recurrence on the size of the left
 * subtree, \f$ C_0 = 1 \f$ and \f$ C_n = \sum_{k=0}^{n-1} C_k C_{n-1-k} \f$,
 * with \f$ \Theta( n^2 ) \f$ operations on numbers of \f$ \Theta( n ) \f$
 * bits, and with `T` a `BigInteger` for any \f$ n \f$ [see `BigInteger.h`].
 * The closed form \f$ C_n = \binom{2n}{n} / ( n + 1 ) \f$ and the recurrence
 * \f$ C_{n+1} = 2 ( 2 n + 1 ) C_n / ( n + 2 ) \f$ take \f$ O( n ) \f$
 * operations, but their intermediate values exceed the result, so that in
 * 64 bits they stop before the table does, which reaches \f$ C_{36} \f$;
 * asymptotically \f$ C_n \sim 4^n / ( n^{3/2} \sqrt{\pi} ) \f$.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 0 \f$, and \f$ C_n \f$
 * representable in `T` (\f$ n \le 36 \f$ for 64 bits unsigned). The
 * functions are defined in `Catalan.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Programmazione dinamica (the binary trees of \f$ n \f$ nodes)
 * \see R. P. Stanley, Catalan Numbers, Cambridge University Press, 2015,
 *      doi:10.1017/CBO9781139871495
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.4.4
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

#ifndef __ADS_Catalan
 #define __ADS_Catalan
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the number \f$ C_n \f$ of Catalan, the binary trees of \f$ n \f$ nodes
///
/// In pseudocode (the exercise on the binary trees of \f$ n \f$ nodes of
/// Esercizi Programmazione dinamica), by the size \f$ k \f$ of the left
/// subtree:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{catalan}( n ) \cr
/// 2 & \quad C[ 0 ] \gets 1 \cr
/// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
/// 4 & \qquad C[ i ] \gets 0 \cr
/// 5 & \qquad \textbf{for}\ k \gets 0\ \textbf{to}\ i - 1\ \textbf{do}\ C[ i
///     ] \gets C[ i ] + C[ k ] \cdot C[ i - 1 - k ] \cr
/// 6 & \quad \textbf{return}\ C[ n ]
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$ and \f$ C_n \f$ representable in `T`
template< class T = std::uint64_t >
T catalan( int n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Catalan.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Catalan.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Catalan.h ---------------------------*/
/*--------------------------------------------------------------------------*/
