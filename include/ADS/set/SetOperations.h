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
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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
S setUnion( const S & A , const S & B )
{
 S C = A;
 B.forEach( [ & ]( const auto & x ) { C.insert( x ); } );
 return( C );
 }

/// the intersection: a copy of the smaller set X, less its items not in
/// the other one
template< IterableSet S >
S setIntersection( const S & A , const S & B )
{
 const bool a = A.size() <= B.size();
 const S & X = a ? A : B;
 const S & Y = a ? B : A;
 S C = X;
 X.forEach( [ & ]( const auto & x ) {
  if( ! Y.contains( x ) )
   C.remove( x );
  } );
 return( C );
 }

/// the difference: a copy of A, less its items in B
template< IterableSet S >
S setDifference( const S & A , const S & B )
{
 S C = A;
 A.forEach( [ & ]( const auto & x ) {
  if( B.contains( x ) )
   C.remove( x );
  } );
 return( C );
 }

/// the symmetric difference: A - B, plus the items of B not in A
template< IterableSet S >
S setSymmetricDifference( const S & A , const S & B )
{
 S C = setDifference( A , B );
 B.forEach( [ & ]( const auto & x ) {
  if( ! A.contains( x ) )
   C.insert( x );
  } );
 return( C );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* SetOperations.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SetOperations.h ------------------------*/
/*--------------------------------------------------------------------------*/
