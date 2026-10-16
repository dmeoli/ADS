/*--------------------------------------------------------------------------*/
/*----------------------------- File Dequeue.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The DEQUEUE (double ended queue) of exercise 4.9, a sequence changed at
 * both ends, which is a STACK when used at one end and a QUEUE when used at
 * both [see ADT.h]: realized with a LIST with pointers, every operation
 * takes O( 1 ) time. Removing from an empty dequeue throws
 * std::logic_error. It is the structure of the algorithm of Pape-D'Esopo
 * (section 11.8).
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

#include "ADS/sequence/List.h"

#include <stdexcept>

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

 [[nodiscard]] bool isEmpty( void ) const { return( f_L.isEmpty() ); }

 void insertHead( const T & v ) { f_L.insert( f_L.head() , v ); }

 void insertTail( const T & v ) {
  f_L.insert( f_L.next( f_L.tail() ) , v );
  }

 T removeHead( void ) {
  if( isEmpty() )
   throw( std::logic_error( "ADS::Dequeue::removeHead: empty dequeue" ) );
  T v = f_L.read( f_L.head() );
  f_L.remove( f_L.head() );
  return( v );
  }

 T removeTail( void ) {
  if( isEmpty() )
   throw( std::logic_error( "ADS::Dequeue::removeTail: empty dequeue" ) );
  T v = f_L.read( f_L.tail() );
  f_L.remove( f_L.tail() );
  return( v );
  }

 [[nodiscard]] const T & head( void ) const {
  if( isEmpty() )
   throw( std::logic_error( "ADS::Dequeue::head: empty dequeue" ) );
  return( f_L.read( f_L.head() ) );
  }

 [[nodiscard]] const T & tail( void ) const {
  if( isEmpty() )
   throw( std::logic_error( "ADS::Dequeue::tail: empty dequeue" ) );
  return( f_L.read( f_L.tail() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 List< T > f_L;   ///< the items, from the head

 };  // end( class Dequeue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Dequeue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Dequeue.h --------------------------*/
/*--------------------------------------------------------------------------*/
