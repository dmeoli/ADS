/*--------------------------------------------------------------------------*/
/*------------------------- File MonotoneSearch.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search of the first place where a monotone predicate becomes true,
 * when no upper bound on that place is known: on the integers, on a
 * sequence of unknown length, and on the integer square root.
 *
 * - exponentialSearch(): the least i >= lo with p( i ), for a predicate
 *   false and then true; the probes lo , lo + 1 , lo + 3 , lo + 7 , ...
 *   double the distance from lo until p holds, and a binary search between
 *   the last two probes finds the place h, in
 *   \f$ 2 \lceil \log_2 ( h - lo + 1 ) \rceil + 1 \f$ evaluations at most,
 *   the method B_1 of Bentley and Yao; their methods B_k, which find the
 *   bound itself by an unbounded search of the previous level, reach
 *   \f$ \log_2 h + \log_2 \log_2 h + \dots \f$, close to the lower bound.
 *   The probes never overflow the type of the integers, and the middle of
 *   the binary search is computed on the unsigned distance;
 * - unboundedSearch(): the first place of a sequence read one item at a
 *   time, get( i ) giving no item past its end, at which there is no item
 *   or the item satisfies a predicate false and then true on the items;
 *   the count of the items before the first one that is not smaller than
 *   a value, in a sorted sequence of unknown length, is the classic case;
 * - integerSqrt(): \f$ \lfloor \sqrt n \rfloor \f$, the largest r with
 *   \f$ r^2 \le n \f$, by a binary search on [ 0 , 2^{w / 2} ) for w bits,
 *   where \f$ m^2 \le n \f$ is tested as \f$ m \le \lfloor n / m \rfloor \f$,
 *   so that no product overflows; w / 2 steps. Newton's iteration
 *   \f$ x \leftarrow \lfloor ( x + \lfloor n / x \rfloor ) / 2 \rfloor \f$
 *   converges quadratically from a start above the root, in
 *   \f$ O( \log \log n ) \f$ steps from a start within a constant factor;
 *   isPerfectSquare() compares the square of the root with n.
 *
 * The contracts [see Contract.h]: exponentialSearch() needs a place up to the
 * largest integer where the predicate holds, a precondition checked when the
 * probes reach it, and checks that p holds at the result and not before it,
 * evaluations made with checkingContract() true [see Sorted.h];
 * integerSqrt() checks \f$ r^2 \le n < ( r + 1 )^2 \f$. The functions are
 * defined in MonotoneSearch.tpp.
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

/// exponentialSearch(): the least i >= lo with p( i ), p false and then
/// true from lo; \f$ O( \log( i - lo + 1 ) ) \f$ evaluations of p
/// \pre p( i ) for some i >= lo of the type I
/// \post p( r ), and r == lo or ! p( r - 1 )
template< std::integral I , class P >
I exponentialSearch( P && p , I lo = I( 0 ) );

/// unboundedSearch(): the first place i where get( i ), which gives an
/// optional item, empty past the end of the sequence, gives no item or an
/// item x with pred( x ); \f$ O( \log i ) \f$ calls of get
/// \pre pred false and then true on the items of the sequence
/// \post get( r ) is empty or satisfies pred, and get( r - 1 ) does not
template< class G , class P >
std::size_t unboundedSearch( G && get , P && pred );

/// integerSqrt(): the largest r with r^2 <= n, by a binary search with no
/// overflow; w / 2 steps for w bits
/// \post r^2 <= n < ( r + 1 )^2
template< std::unsigned_integral U >
U integerSqrt( U n );

/// isPerfectSquare(): n = k^2 for an integer k
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
