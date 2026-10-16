/*--------------------------------------------------------------------------*/
/*------------------------- File StackAlgorithms.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Algorithms that use a STACK [see ADT.h] only through its operations,
 * beyond the book:
 *
 * - isPalindrome(), the characters pushed and then popped against the
 *   string from its start, O( n );
 * - sortStack(), which sorts a stack with its own operations and the
 *   recursion: a pass pops two items, pushes back the larger, sorts the
 *   rest recursively and pushes the smaller on top, so that the least item
 *   rises to the top; n passes sort the stack, the least on top, in
 *   \f$ O( n^2 ) \f$;
 * - removeGreater(), which removes the items larger than k keeping the
 *   order of the others, with an auxiliary stack.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_StackAlgorithms
 #define __ADS_StackAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/sequence/List.h"
#include "ADS/stack/SequenceStack.h"

#include <string>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether s reads the same from both ends
inline bool isPalindrome( const std::string & s )
{
 SequenceStack< List< char > > S;
 for( char c : s ) S.push( c );
 for( char c : s )
  if( S.pop() != c ) return( false );
 return( true );
 }

namespace detail {

/// the items of s, counted by popping them and pushing them back
template< Stack S >
long stackSize( S & s )
{
 if( s.isEmpty() ) return( 0 );
 auto x = s.pop();
 const long n = 1 + stackSize( s );
 s.push( x );
 return( n );
 }

/// a pass: the least item rises to the top
template< Stack S >
void risePass( S & s )
{
 if( s.isEmpty() ) return;
 auto a = s.pop();
 if( s.isEmpty() ) {
  s.push( a );
  return;
  }
 auto b = s.pop();
 s.push( a < b ? b : a );
 risePass( s );
 s.push( a < b ? a : b );
 }

}  // end( namespace detail )

/// sorts s with the least item on top
template< Stack S >
void sortStack( S & s )
{
 for( long n = detail::stackSize( s ) ; n > 0 ; --n ) detail::risePass( s );
 }

/// removes the items larger than k, the others in their order
template< Stack S >
void removeGreater( S & s , const typename S::Item & k )
{
 SequenceStack< List< typename S::Item > > aux;
 while( ! s.isEmpty() ) {
  auto x = s.pop();
  if( !( k < x ) ) aux.push( x );
  }
 while( ! aux.isEmpty() ) s.push( aux.pop() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* StackAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StackAlgorithms.h -----------------------*/
/*--------------------------------------------------------------------------*/
