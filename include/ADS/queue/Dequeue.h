/*--------------------------------------------------------------------------*/
/*----------------------------- File Dequeue.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The DEQUEUE (double ended queue) of exercise 4.9, a sequence changed at
 * both ends, which is a STACK when used at one end and a QUEUE when used at
 * both [see ADT.h]: realized with a LIST with pointers, every operation
 * takes O( 1 ) time. It is the structure of the algorithm of Pape-D'Esopo
 * (section 11.8).
 *
 * The contracts [see Contract.h]: removeHead(), removeTail(), head() and
 * tail() need a dequeue that is not empty, and the insertions ensure one
 * that is not, with the item at the end where it was put. The operations
 * are defined in Dequeue.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.9,
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

#ifndef __ADS_Dequeue
 #define __ADS_Dequeue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Dequeue ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DEQUEUE on a bidirectional list with pointers
template< class T >
class Dequeue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 [[nodiscard]] bool isEmpty( void ) const;

 /// \post the dequeue is not empty
 void insertHead( const T & v );

 /// \post the dequeue is not empty
 void insertTail( const T & v );

 /// \pre the dequeue is not empty
 T removeHead( void );

 /// \pre the dequeue is not empty
 T removeTail( void );

 /// \pre the dequeue is not empty
 [[nodiscard]] const T & head( void ) const;

 /// \pre the dequeue is not empty
 [[nodiscard]] const T & tail( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 List< T > f_L;   ///< the items, from the head

 };  // end( class Dequeue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/Dequeue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Dequeue.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Dequeue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
