/*--------------------------------------------------------------------------*/
/*------------------------- File MonotoneSearch.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search of the first place where a monotone predicate becomes true,
 * when no upper bound on that place is known: on the integers, on a
 * sequence of unknown length, and on the integer square root.
 *
 * - `exponentialSearch()`: the least \f$ i \ge \mathit{lo} \f$ with
 *   \f$ p( i ) \f$, for a predicate false and then true; the probes
 *   \f$ \mathit{lo} \f$, \f$ \mathit{lo} + 1 \f$, \f$ \mathit{lo} + 3 \f$,
 *   \f$ \mathit{lo} + 7 \f$, and so on, double the distance from
 *   \f$ \mathit{lo} \f$ until \f$ p \f$ holds, and a binary search between
 *   the last two probes finds the place \f$ h \f$, in
 *   \f$ 2 \lceil \log_2 ( h - \mathit{lo} + 1 ) \rceil + 1 \f$ evaluations at
 *   most, the method \f$ B_1 \f$ of Bentley and Yao; their
 *   methods \f$ B_k \f$, which find the bound itself by an unbounded search
 *   of the previous level, reach
 *   \f$ \log_2 h + \log_2 \log_2 h + \cdots \f$, close to the lower bound.
 *   The probes never overflow the type of the integers, and the middle of
 *   the binary search is computed on the unsigned distance;
 * - `unboundedSearch()`: the first place of a sequence read one item at a
 *   time, `get( i )` giving no item past its end, at which there is no item
 *   or the item satisfies a predicate false and then true on the items;
 *   the count of the items before the first one that is not smaller than
 *   a value, in a sorted sequence of unknown length, is the classic case;
 * - `integerSqrt()`: \f$ \lfloor \sqrt n \rfloor \f$, the largest \f$ r \f$
 *   with \f$ r^2 \le n \f$, by a binary search on \f$ [ 0 , 2^{w / 2} ) \f$
 *   for \f$ w \f$ bits, where \f$ m^2 \le n \f$ is tested as
 *   \f$ m \le \lfloor n / m \rfloor \f$, so that no product overflows;
 *   \f$ w / 2 \f$ steps. Newton's iteration
 *   \f$ x \leftarrow \lfloor ( x + \lfloor n / x \rfloor ) / 2 \rfloor \f$
 *   converges quadratically from a start above the root, in
 *   \f$ O( \log \log n ) \f$ steps from a start within a constant factor;
 *   `isPerfectSquare()` compares the square of the root with \f$ n \f$.
 *
 * The contracts [see `Contract.h`]: `exponentialSearch()` needs a place up
 * to the largest integer where the predicate holds, a precondition checked
 * when the probes reach it, and checks that \f$ p \f$ holds at the result
 * and not before it, evaluations made with `checkingContract()` true [see
 * `Sorted.h`]; `integerSqrt()` checks \f$ r^2 \le n < ( r + 1 )^2 \f$. The
 * functions are defined in `MonotoneSearch.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Esercizi
 *      Divide-et-impera, Raccolte da compiti scritti
 * \see J. L. Bentley, A. C.-C. Yao, An almost optimal algorithm for
 *      unbounded searching, Information Processing Letters 5(3), 82-87,
 *      1976, doi:10.1016/0020-0190(76)90071-5
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

#ifndef __ADS_MonotoneSearch
 #define __ADS_MonotoneSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the least \f$ i \ge \mathit{lo} \f$ with \f$ p( i ) \f$, \f$ p \f$ false
/// and then true from \f$ \mathit{lo} \f$;
/// \f$ O( \log( i - \mathit{lo} + 1 ) ) \f$ evaluations of \f$ p \f$
///
/// In pseudocode (the method \f$ B_1 \f$ of Bentley and Yao, as in exercise
/// 12.9), \f$ p \f$ false
/// on \f$ [ \mathit{lo} , a ) \f$ at all times, the probes cut at the largest
/// integer:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{exponentialSearch}( p , \mathit{lo} ) \cr
/// 2 & \quad a \gets \mathit{lo};\ b \gets \mathit{lo};\ s \gets 1 \cr
/// 3 & \quad \textbf{while not}\ p( b )\ \textbf{do} \cr
/// 4 & \qquad a \gets b + 1;\ b \gets a + s - 1;\ s \gets 2 s \cr
/// 5 & \quad \textbf{while}\ a < b\ \textbf{do}\quad \triangleright
///     \ \text{a binary search, } p( b ) \text{ true} \cr
/// 6 & \qquad m \gets a + \lfloor ( b - a ) / 2 \rfloor \cr
/// 7 & \qquad \textbf{if}\ p( m )\ \textbf{then}\ b \gets m\ \textbf{else}
///     \ a \gets m + 1 \cr
/// 8 & \quad \textbf{return}\ a
/// \end{array}}
/// \f]
/// \pre \f$ p( i ) \f$ for some \f$ i \ge \mathit{lo} \f$ of the type `I`
/// \post \f$ p( r ) \f$, and \f$ r = \mathit{lo} \f$ or not \f$ p( r - 1 ) \f$
template< std::integral I , class P >
I exponentialSearch( P && p , I lo = I( 0 ) );

/// the first place \f$ i \f$ where `get( i )`, which gives an optional item,
/// empty past the end of the sequence, gives no item or an item \f$ x \f$
/// with `pred( x )`; \f$ O( \log i ) \f$ calls of `get()`
///
/// In pseudocode:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{unboundedSearch}( \mathit{get} ,
///     \mathit{pred} ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{exponentialSearch}( i \mapsto
///     \mathit{get}( i ) = \textbf{nil}\ \textbf{or}\ \mathit{pred}(
///     \mathit{get}( i ) ) , 0 )
/// \end{array}}
/// \f]
/// \pre `pred` false and then true on the items of the sequence
/// \post `get( r )` is empty or satisfies `pred`, and `get( r - 1 )` does
///       not
template< class G , class P >
std::size_t unboundedSearch( G && get , P && pred );

/// the largest \f$ r \f$ with \f$ r^2 \le n \f$, by a binary search with no
/// overflow; \f$ w / 2 \f$ steps for \f$ w \f$ bits
///
/// In pseudocode (Esercizi Divide-et-impera of the book), the invariant
/// being \f$ \mathit{lo}^2 \le n < \mathit{hi}^2 \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{integerSqrt}( n ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets 2^{w / 2} \cr
/// 3 & \quad \textbf{while}\ \mathit{hi} - \mathit{lo} > 1\ \textbf{do} \cr
/// 4 & \qquad m \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo} ) / 2
///     \rfloor \cr
/// 5 & \qquad \textbf{if}\ m \le \lfloor n / m \rfloor\ \textbf{then}
///     \ \mathit{lo} \gets m\ \textbf{else}\ \mathit{hi} \gets m \cr
/// 6 & \quad \textbf{return}\ \mathit{lo}
/// \end{array}}
/// \f]
/// \post \f$ r^2 \le n < ( r + 1 )^2 \f$
template< std::unsigned_integral U >
U integerSqrt( U n );

/// true if \f$ n = k^2 \f$ for an integer \f$ k \f$, i.e., if the square of
/// `integerSqrt( n )` is \f$ n \f$
template< std::unsigned_integral U >
bool isPerfectSquare( U n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/MonotoneSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MonotoneSearch.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File MonotoneSearch.h ------------------------*/
/*--------------------------------------------------------------------------*/
