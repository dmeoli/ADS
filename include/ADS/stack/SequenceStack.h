/*--------------------------------------------------------------------------*/
/*-------------------------- File SequenceStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * SequenceStack is the STACK [see ADT.h] simulated with the operations of
 * any SEQUENCE, the top being its first item:
 * \f[
 *   top() = read( head() ) \qquad pop() = remove( head() ) \qquad
 *   push( v ) = insert( head() , v ) \; ,
 * \f]
 * which takes O( 1 ) time on a list with pointers, even a singly linked one
 * (Esercizio 4.6), and has no capacity; std::logic_error is thrown on an
 * empty stack.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.1,
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

#ifndef __ADS_SequenceStack
 #define __ADS_SequenceStack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cstddef>
#include <memory>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SequenceStack ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a STACK simulated with the operations of a SEQUENCE S, the top first
template< class S >
class SequenceStack {

/*--------------------------------------------------------------------------*/
 public:

 using Item = typename S::Item;   ///< the type of the items

 [[nodiscard]] bool isEmpty( void ) const { return( f_s.isEmpty() ); }

 /// the item on top, read( head() )
 [[nodiscard]] const Item & top( void ) const {
  if( f_s.isEmpty() )
   throw( std::logic_error( "ADS::SequenceStack::top: empty stack" ) );
  return( f_s.read( f_s.head() ) );
  }

 /// removes the item on top and returns it, remove( head() )
 Item pop( void ) {
  if( f_s.isEmpty() )
   throw( std::logic_error( "ADS::SequenceStack::pop: empty stack" ) );
  Item v = f_s.read( f_s.head() );
  f_s.remove( f_s.head() );
  return( v );
  }

 /// puts v on top, insert( head() , v )
 void push( const Item & v ) { f_s.insert( f_s.head() , v ); }

/*--------------------------------------------------------------------------*/
 private:

 S f_s;   ///< the sequence, the top as its first item

 };  // end( class SequenceStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* SequenceStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SequenceStack.h ------------------------*/
/*--------------------------------------------------------------------------*/
