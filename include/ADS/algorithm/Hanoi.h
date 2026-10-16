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
template< class F >
void hanoi_recursive( int n , int source , int dest , int middle , F && f )
{
 if( n == 1 )
  f( source , dest );
 else {
  hanoi_recursive( n - 1 , source , middle , dest , f );   // (1)
  f( source , dest );
  hanoi_recursive( n - 1 , middle , dest , source , f );   // (2)
  }
 }

/*--------------------------------------------------------------------------*/

/// the first hanoi-iterative(): the calls of (1) are pushed on a stack, and
/// each pop resumes the call that was left open, moving a disk and
/// simulating the call (2)
template< class F >
void hanoi_iterative( int n , int source , int dest , int middle , F && f )
{
 ArrayStack< std::tuple< int , int , int , int > > S( n > 0 ? n : 1 );
 bool end_of_the_world = false;
 while( ! end_of_the_world ) {
  while( n != 1 ) {
   S.push( { n , source , dest , middle } );
   --n;
   std::swap( dest , middle );
   }
  f( source , dest );
  if( S.isEmpty() )
   end_of_the_world = true;
  else {
   std::tie( n , source , dest , middle ) = S.pop();
   f( source , dest );
   --n;
   std::swap( source , middle );
   }
  }
 }

/*--------------------------------------------------------------------------*/

/// the second hanoi-iterative(): the systematic rules, saving on the stack
/// the parameters and the label from which the call resumes
template< class F >
void hanoi_iterative_labels( int n , int source , int dest , int middle ,
                             F && f )
{
 enum Label { START , AFTER1 , END , STOP };
 using Frame = std::tuple< int , int , int , int , Label >;
 ArrayStack< Frame > S( 2 * ( n > 0 ? n : 1 ) );
 Label pos = START;
 while( pos != STOP )
  if( pos == START ) {
   if( n == 1 ) {
    f( source , dest );
    pos = END;
    }
   else {
    S.push( { n , source , dest , middle , AFTER1 } );
    --n;
    std::swap( dest , middle );
    }
   }
  else
   if( pos == AFTER1 ) {
    f( source , dest );
    S.push( { n , source , dest , middle , END } );
    --n;
    std::swap( source , middle );
    pos = START;
    }
   else   // END
    if( S.isEmpty() )
     pos = STOP;
    else
     std::tie( n , source , dest , middle , pos ) = S.pop();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Hanoi.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Hanoi.h ----------------------------*/
/*--------------------------------------------------------------------------*/
