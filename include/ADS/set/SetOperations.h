/*--------------------------------------------------------------------------*/
/*-------------------------- File SetOperations.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of chapter 8 on any SET that can be copied and has the
 * foreach, `forEach( f )`: the union copies \f$ A \f$ and inserts the items
 * of \f$ B \f$, the intersection copies the smaller set and removes its
 * items not in the other one, the difference copies \f$ A \f$ and removes
 * its items in \f$ B \f$, the symmetric difference adds to
 * \f$ A \setminus B \f$ the items of \f$ B \f$ not in \f$ A \f$. On the hash
 * tables, with \f$ n_X \f$ keys in a table of size \f$ m_X \f$, they take
 * \f$ O( \max( m_A , n_A ) + \max( m_B , n_B ) ) \f$ expected time
 * (exercise 8.6), on the Red-Black trees \f$ O( n + m \log( n + m ) ) \f$
 * (exercise 8.7). The realizations that do better have their own [see
 * `BoolSet.h`, `ListSet.h` and `SortedListSet.h`].
 *
 * The contracts [see `Contract.h`]: the result of each operation holds,
 * item by item, what its name says, an audit [see
 * `detail::isSetOperation()`]. The operations are defined in
 * `SetOperations.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
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

#ifndef __ADS_SetOperations
 #define __ADS_SetOperations
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- GENERIC OPERATIONS ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a SET that can be copied and has the foreach
template< class S >
concept IterableSet = Set< S > && std::copy_constructible< S > &&
 requires( const S s ) {
  s.forEach( []( const typename S::Item & ) {} );
  };

/// the union: a copy of \f$ A \f$, plus the items of \f$ B \f$
///
/// In pseudocode (chapter 8):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setUnion}( A , B ) \cr
/// 2 & \quad C \gets \text{a copy of } A \cr
/// 3 & \quad \textbf{foreach}\ x \in B\ \textbf{do}\ C.\mathrm{insert}( x )
///     \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< IterableSet S >
S setUnion( const S & A , const S & B );

/// the intersection: a copy of the smaller set \f$ X \f$, less its items
/// not in the other one, \f$ Y \f$
///
/// In pseudocode (chapter 8):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setIntersection}( A , B ) \cr
/// 2 & \quad ( X , Y ) \gets ( A , B )\ \text{if } | A | \le | B | \text{, }
///     ( B , A ) \text{ otherwise} \cr
/// 3 & \quad C \gets \text{a copy of } X \cr
/// 4 & \quad \textbf{foreach}\ x \in X\ \textbf{with not}
///     \ Y.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{remove}( x ) \cr
/// 5 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< IterableSet S >
S setIntersection( const S & A , const S & B );

/// the difference: a copy of \f$ A \f$, less its items in \f$ B \f$
///
/// In pseudocode (chapter 8):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setDifference}( A , B ) \cr
/// 2 & \quad C \gets \text{a copy of } A \cr
/// 3 & \quad \textbf{foreach}\ x \in A\ \textbf{with}
///     \ B.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{remove}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< IterableSet S >
S setDifference( const S & A , const S & B );

/// the symmetric difference: \f$ A \setminus B \f$, plus the items of
/// \f$ B \f$ not in \f$ A \f$
///
/// In pseudocode (chapter 8):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{setSymmetricDifference}( A , B ) \cr
/// 2 & \quad C \gets \mathrm{setDifference}( A , B ) \cr
/// 3 & \quad \textbf{foreach}\ x \in B\ \textbf{with not}
///     \ A.\mathrm{contains}( x )\ \textbf{do}\ C.\mathrm{insert}( x ) \cr
/// 4 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
template< IterableSet S >
S setSymmetricDifference( const S & A , const S & B );

namespace detail {

/// whether \f$ C \f$ is \f$ A \f$ `op` \f$ B \f$, `op` one of the
/// characters `|`, `&`, `-` and `^`: each item of \f$ C \f$ is in
/// \f$ A \f$ `op` \f$ B \f$, and each item of \f$ A \f$ or \f$ B \f$
/// in \f$ A \f$ `op` \f$ B \f$ is in \f$ C \f$,
/// \f$ O( n_A + n_B + n_C ) \f$ lookups; the sets need `contains()` and
/// `forEach()`
template< class C , class A , class B >
bool isSetOperation( const C & c , const A & a , const B & b , char op );

}  // end( namespace detail )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/SetOperations.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SetOperations.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SetOperations.h ------------------------*/
/*--------------------------------------------------------------------------*/
