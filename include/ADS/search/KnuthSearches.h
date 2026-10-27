/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthSearches.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two searches of a sorted vector from Knuth, beyond the book [see
 * `BinarySearch.h` for the binary search of section 1.2]:
 *
 * - `fibonacciSearch()` (Algorithm 6.2.1F) splits the part still to examine
 *   at a Fibonacci number instead of the middle, so that the next part is
 *   found by subtractions alone, with no division: the part has length
 *   \f$ F_{k+1} - 1 \f$, the item compared is the \f$ F_k \f$-th, and the
 *   two parts left have lengths \f$ F_k - 1 \f$ and \f$ F_{k-1} - 1 \f$;
 *   at most about \f$ 1.44 \log_2 n \f$ comparisons, a little more than
 *   the binary search, for an average cost per step that was lower on the
 *   machines of the time (and may still be, where division is dear);
 * - `uniformBinarySearch()` (Algorithm 6.2.1U, with the table of Algorithm
 *   C) keeps the place \f$ i \f$ of the last item compared and the width
 *   \f$ \delta_j \f$ of the step \f$ j \f$,
 *   \f$ \delta_j = \lfloor ( n + 2^{j-1} ) / 2^j \rfloor \f$, precomputed:
 *   a step adds or subtracts the next width, so that the search moves on a
 *   fixed tree of differences with no endpoint to update.
 *
 * Both return the place of \f$ v \f$, or -1; when \f$ n \f$ is not a
 * Fibonacci number, a probe of the Fibonacci search past the end of the
 * vector is moved to its last item, which is the same as imagining items
 * larger than all the others up to the next Fibonacci number; the uniform
 * search may step to the places 0 and \f$ n + 1 \f$, which stand for the
 * keys \f$ -\infty \f$ and \f$ +\infty \f$ of the book and are compared
 * without reading the vector. The contracts [see `Contract.h`]: the vector
 * is sorted (an audit), and the result is -1 if and only if \f$ v \f$ is not
 * there. The functions are defined in `KnuthSearches.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.2.1
 * \see D. E. Ferguson, Fibonaccian searching, Communications of the ACM
 *      3(12), 648, 1960, doi:10.1145/367487.367496
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

#ifndef __ADS_KnuthSearches
 #define __ADS_KnuthSearches
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

/// the Fibonacci search of \f$ v \f$ in the sorted \f$ A \f$: its place, or
/// -1; the comparisons go to `*cmp`, if given
///
/// In pseudocode (Algorithm 6.2.1F of Knuth, beyond the book), the part still
/// to examine being \f$ A[ o + 1 .. o + F_k - 1 ] \f$, with
/// \f$ ( f_2 , f_1 , f ) = ( F_{k-2} , F_{k-1} , F_k ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{fibonacciSearch}( A , v ) \cr
/// 2 & \quad ( f_2 , f_1 , f ) \gets \text{three consecutive Fibonacci }
///     \text{numbers, the least } f \ge n \cr
/// 3 & \quad o \gets -1 \cr
/// 4 & \quad \textbf{while}\ f > 1\ \textbf{do} \cr
/// 5 & \qquad i \gets \min\{ o + f_2 , n - 1 \} \cr
/// 6 & \qquad \textbf{if}\ A[ i ] < v\ \textbf{then}\ ( f_2 , f_1 , f )
///     \gets ( f_1 - f_2 , f_2 , f_1 );\ o \gets i \cr
/// 7 & \qquad \textbf{else if}\ v < A[ i ]\ \textbf{then}\ ( f_2 , f_1 , f )
///     \gets ( 2 f_2 - f_1 , f_1 - f_2 , f_2 ) \cr
/// 8 & \qquad \textbf{else return}\ i \cr
/// 9 & \quad \textbf{if}\ f_1 = 1 \text{ and } o + 1 < n \text{ and }
///     A[ o + 1 ] = v\ \textbf{then return}\ o + 1 \cr
/// 10 & \quad \textbf{return}\ -1
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
/// \post the result is -1 if and only if \f$ v \f$ is not in \f$ A \f$
template< class T >
long fibonacciSearch( const std::vector< T > & A , const T & v ,
                      long * cmp = nullptr );

/// the uniform binary search of \f$ v \f$ in the sorted \f$ A \f$: its place,
/// or -1; the comparisons go to `*cmp`, if given
///
/// In pseudocode (Algorithms 6.2.1U and 6.2.1C of Knuth, beyond the book),
/// \f$ A[ 1 .. n ] \f$ with \f$ A[ 0 ] = -\infty \f$ and
/// \f$ A[ n + 1 ] = +\infty \f$, never read:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{uniformBinarySearch}( A , v ) \cr
/// 2 & \quad \textbf{foreach}\ j \ge 1\ \textbf{do}\ \delta_j \gets \lfloor (
///     n + 2^{j-1} ) / 2^j \rfloor\quad \triangleright\ \text{until }
///     \delta_j = 0 \cr
/// 3 & \quad i \gets \delta_1;\ j \gets 1 \cr
/// 4 & \quad \textbf{loop} \cr
/// 5 & \qquad \textbf{if}\ v < A[ i ]\ \textbf{then} \cr
/// 6 & \qquad\quad j \gets j + 1;\ \textbf{if}\ \delta_j = 0
///     \ \textbf{then return}\ -1;\ i \gets i - \delta_j \cr
/// 7 & \qquad \textbf{else if}\ v > A[ i ]\ \textbf{then} \cr
/// 8 & \qquad\quad j \gets j + 1;\ \textbf{if}\ \delta_j = 0
///     \ \textbf{then return}\ -1;\ i \gets i + \delta_j \cr
/// 9 & \qquad \textbf{else return}\ i
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
/// \post the result is -1 if and only if \f$ v \f$ is not in \f$ A \f$
template< class T >
long uniformBinarySearch( const std::vector< T > & A , const T & v ,
                          long * cmp = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/KnuthSearches.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthSearches.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnuthSearches.h ------------------------*/
/*--------------------------------------------------------------------------*/
