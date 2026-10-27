/*--------------------------------------------------------------------------*/
/*------------------------- File StackAlgorithms.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms that use a STACK [see `ADT.h`] only through its operations,
 * beyond the book:
 *
 * - `isPalindrome()`, the characters pushed and then popped against the
 *   string from its start, \f$ O( n ) \f$;
 * - `sortStack()`, which sorts a stack with its own operations and the
 *   recursion: a pass pops two items, pushes back the larger, makes a pass
 *   on the rest recursively and pushes the smaller on top, so that the
 *   largest item sinks to the bottom and the smaller ones rise by one place;
 *   \f$ n \f$ passes sort the stack, the least on top, in
 *   \f$ O( n^2 ) \f$;
 * - `removeGreater()`, which removes the items larger than \f$ k \f$
 *   keeping the order of the others, with an auxiliary stack;
 * - `forEachBottomUp()`, which visits the items from the bottom to the top
 *   and leaves the stack as it was: the top is popped, the rest visited
 *   recursively, then the top visited and pushed back, in \f$ O( n ) \f$
 *   time and \f$ O( n ) \f$ space for the recursion.
 *
 * The contracts [see `Contract.h`]: `sortStack()` ensures a sorted stack
 * with the same number of items, and `removeGreater()` a stack with no item
 * larger than \f$ k \f$, and `forEachBottomUp()` the stack as it was; all
 * are audits, checked by popping the stack and pushing it back. The
 * algorithms are defined in `StackAlgorithms.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.1,
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

#ifndef __ADS_StackAlgorithms
 #define __ADS_StackAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/stack/SequenceStack.h"

#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether \f$ s \f$ reads the same from both ends
///
/// In pseudocode (section 4.3.1), the stack reversing the string:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{isPalindrome}( s ) \cr
/// 2 & \quad S \gets \text{an empty stack} \cr
/// 3 & \quad \textbf{foreach}\ \text{character } c \text{ of } s
///     \ \textbf{do}\ \mathrm{push}( S , c ) \cr
/// 4 & \quad \textbf{foreach}\ \text{character } c \text{ of } s
///     \ \textbf{do}\ \textbf{if}\ \mathrm{pop}( S ) \ne c\ \textbf{then
///     return false} \cr
/// 5 & \quad \textbf{return true}
/// \end{array}}
/// \f]
inline bool isPalindrome( const std::string & s );

/// sorts \f$ s \f$ with the least item on top
///
/// In pseudocode (section 4.3.1, the bubble sort on a stack), \f$ n \f$
/// passes, each carrying the largest item it meets to the bottom:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{pass}( s ) \cr
/// 2 & \quad \textbf{if}\ \mathrm{empty}( s )\ \textbf{then return} \cr
/// 3 & \quad a \gets \mathrm{pop}( s ) \cr
/// 4 & \quad \textbf{if}\ \mathrm{empty}( s )\ \textbf{then}\ \mathrm{push}(
///     s , a );\ \textbf{return} \cr
/// 5 & \quad b \gets \mathrm{pop}( s );\ \mathrm{push}( s , \max( a , b ) )
///     \cr
/// 6 & \quad \mathrm{pass}( s );\ \mathrm{push}( s , \min( a , b ) ) \cr
/// 7 & \textbf{procedure}\ \mathrm{sortStack}( s ) \cr
/// 8 & \quad \textbf{for}\ k \gets 1\ \textbf{to}\ n\ \textbf{do}
///     \ \mathrm{pass}( s )
/// \end{array}}
/// \f]
/// \post \f$ s \f$ is sorted, the least on top, with the same items (an
///       audit)
template< Stack S >
void sortStack( S & s );

/// removes the items larger than \f$ k \f$, the others in their order
///
/// In pseudocode (section 4.3.1), the auxiliary stack reversing the order
/// twice:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{removeGreater}( s , k ) \cr
/// 2 & \quad A \gets \text{an empty stack} \cr
/// 3 & \quad \textbf{while not}\ \mathrm{empty}( s )\ \textbf{do} \cr
/// 4 & \qquad x \gets \mathrm{pop}( s );\ \textbf{if}\ x \le k
///     \ \textbf{then}\ \mathrm{push}( A , x ) \cr
/// 5 & \quad \textbf{while not}\ \mathrm{empty}( A )\ \textbf{do}
///     \ \mathrm{push}( s , \mathrm{pop}( A ) )
/// \end{array}}
/// \f]
/// \post no item is larger than \f$ k \f$ (an audit)
template< Stack S >
void removeGreater( S & s , const typename S::Item & k );

/// `f( x )` on each item \f$ x \f$, from the bottom to the top
///
/// In pseudocode (section 4.3.1), the stack of the calls holding the items
/// popped:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{forEachBottomUp}( s , f ) \cr
/// 2 & \quad \textbf{if}\ \mathrm{empty}( s )\ \textbf{then return} \cr
/// 3 & \quad x \gets \mathrm{pop}( s ) \cr
/// 4 & \quad \mathrm{forEachBottomUp}( s , f ) \cr
/// 5 & \quad f( x );\ \mathrm{push}( s , x )
/// \end{array}}
/// \f]
/// \post \f$ s \f$ is as it was (an audit)
template< Stack S , class F >
void forEachBottomUp( S & s , F && f );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/StackAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StackAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StackAlgorithms.h -----------------------*/
/*--------------------------------------------------------------------------*/
