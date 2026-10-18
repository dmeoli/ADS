/*--------------------------------------------------------------------------*/
/*------------------------------ File Hanoi.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The towers of Hanoi: moving n disks from the peg source to the peg dest,
 * one at a time and never on a smaller one, with the peg middle as a spare.
 * The recursive procedure moves n - 1 disks on middle, the last one on dest
 * and the n - 1 disks on dest, hence it makes
 * \f[
 *   T( n ) = 2 T( n - 1 ) + 1 = 2^n - 1
 * \f]
 * moves. The two iterative procedures of the book simulate its calls with a
 * STACK of their parameters: the first one by hand, the second one by the
 * systematic rules that also save the point where a call resumes (START,
 * AFTER1, END, STOP). All of them hand each move to a function f( from ,
 * to ), so that the sequence of moves can be checked.
 *
 * The contracts [see Contract.h]: n >= 1 disks and three distinct pegs. The
 * functions are defined in Hanoi.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.4,
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

#ifndef __ADS_Hanoi
 #define __ADS_Hanoi
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/ArrayStack.h"

#include <tuple>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// hanoi-recursive(): moves n >= 1 disks from source to dest
/// \pre n >= 1 and source, dest and middle distinct
template< class F >
void hanoi_recursive( int n , int source , int dest , int middle , F && f );

/*--------------------------------------------------------------------------*/

/// the first hanoi-iterative(): the calls of (1) are pushed on a stack, and
/// each pop resumes the call that was left open, moving a disk and
/// simulating the call (2)
/// \pre n >= 1 and source, dest and middle distinct
template< class F >
void hanoi_iterative( int n , int source , int dest , int middle , F && f );

/*--------------------------------------------------------------------------*/

/// the second hanoi-iterative(): the systematic rules, saving on the stack
/// the parameters and the label from which the call resumes
/// \pre n >= 1 and source, dest and middle distinct
template< class F >
void hanoi_iterative_labels( int n , int source , int dest , int middle ,
                             F && f );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/algorithm/impl/Hanoi.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Hanoi.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Hanoi.h ----------------------------*/
/*--------------------------------------------------------------------------*/
