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
 *   order of the others, with an auxiliary stack;
 * - forEachBottomUp(), which visits the items from the bottom to the top
 *   and leaves the stack as it was: the top is popped, the rest visited
 *   recursively, then the top visited and pushed back, in O( n ) time and
 *   O( n ) space for the recursion.
 *
 * The contracts [see Contract.h]: sortStack() ensures a sorted stack with
 * the same number of items, and removeGreater() a stack with no item
 * larger than k, and forEachBottomUp() the stack as it was; all are
 * audits, checked by popping the stack and pushing it back. The algorithms
 * are defined in StackAlgorithms.tpp.
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
#include "ADS/Contract.h"
#include "ADS/sequence/List.h"
#include "ADS/stack/SequenceStack.h"

#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// whether s reads the same from both ends
inline bool isPalindrome( const std::string & s );

/// sorts s with the least item on top
/// \post s is sorted, the least on top, with the same items (an audit)
template< Stack S >
void sortStack( S & s );

/// removes the items larger than k, the others in their order
/// \post no item is larger than k (an audit)
template< Stack S >
void removeGreater( S & s , const typename S::Item & k );

/// f( x ) on each item x, from the bottom to the top
/// \post s is as it was (an audit)
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
