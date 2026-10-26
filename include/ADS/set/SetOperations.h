/*--------------------------------------------------------------------------*/
/*-------------------------- File SetOperations.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of chapter 8 on any SET that can be copied and has the
 * foreach, forEach( f ): the union copies A and inserts the items of B,
 * the intersection copies the smaller set and removes its items not in the
 * other one, the difference copies A and removes its items in B, the
 * symmetric difference adds to A - B the items of B not in A. On the hash
 * tables, with n_X keys in a table of size m_X, they take
 * \f$ O( \max( m_A , n_A ) + \max( m_B , n_B ) ) \f$ expected time
 * (exercise 8.6), on the Red-Black trees \f$ O( n + m \log( n + m ) ) \f$
 * (exercise 8.7). The realizations that do better have their own [see
 * BoolSet.h, ListSet.h and SortedListSet.h].
 *
 * The contracts [see Contract.h]: the result of each operation holds, item
 * by item, what its name says, an audit [see detail::isSetOperation()].
 * The operations are defined in SetOperations.tpp.
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

/// the union: a copy of A, plus the items of B
template< IterableSet S >
S setUnion( const S & A , const S & B );

/// the intersection: a copy of the smaller set X, less its items not in
/// the other one
template< IterableSet S >
S setIntersection( const S & A , const S & B );

/// the difference: a copy of A, less its items in B
template< IterableSet S >
S setDifference( const S & A , const S & B );

/// the symmetric difference: A - B, plus the items of B not in A
template< IterableSet S >
S setSymmetricDifference( const S & A , const S & B );

namespace detail {

/// C is A op B, op one of the characters | & - ^: each item of C is in A op B,
/// and each item of A or B in A op B is in C, \f$ O( n_A + n_B + n_C ) \f$
/// lookups; the sets need contains() and forEach()
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
