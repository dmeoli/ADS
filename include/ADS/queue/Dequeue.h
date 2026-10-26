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
 * \see U. Pape, Implementation and efficiency of Moore-algorithms for the
 *      shortest route problem, Mathematical Programming 7(1), 212-222,
 *      1974, doi:10.1007/BF01585517
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

#ifndef __ADS_Dequeue
 #define __ADS_Dequeue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

#include <cstddef>

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

 [[nodiscard]] bool empty( void ) const;

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

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the bidirectional iterators of the list, from the head to the tail

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using iterator = typename List< T >::iterator;
 using const_iterator = typename List< T >::const_iterator;
 using reverse_iterator = typename List< T >::reverse_iterator;
 using const_reverse_iterator = typename List< T >::const_reverse_iterator;

 [[nodiscard]] iterator begin( void ) { return( f_L.begin() ); }
 [[nodiscard]] iterator end( void ) { return( f_L.end() ); }
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 [[nodiscard]] reverse_iterator rbegin( void ) { return( f_L.rbegin() ); }
 [[nodiscard]] reverse_iterator rend( void ) { return( f_L.rend() ); }
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( f_L.rbegin() );
  }
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( f_L.rend() );
  }

 /// insertTail() and insertHead(), for the inserters of std
 void push_back( const T & v ) { insertTail( v ); }
 void push_front( const T & v ) { insertHead( v ); }

/*--------------------------------------------------------------------------*/
 private:

 List< T > f_L;   ///< the items, from the head

 };  // end( class Dequeue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/Dequeue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Dequeue.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Dequeue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
