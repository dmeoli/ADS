/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The STACK of the book [see ADT.h] realized with a vector of a given
 * capacity: the n items are in the first n places, the top in the place
 * n - 1, so that every operation takes O( 1 ) time. Popping from an empty
 * stack and pushing on a full one are errors of the program, which the book
 * states as preconditions; since checking them costs O( 1 ) here, they are
 * checked, and std::logic_error is thrown.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.2,
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

#ifndef __ADS_ArrayStack
 #define __ADS_ArrayStack
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
/*---------------------------- CLASS ArrayStack ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a STACK realized with a vector of a given capacity
template< class T >
class ArrayStack {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty stack of at most dim items
 explicit ArrayStack( std::size_t dim )
  : f_A( new T[ dim ] ) , f_size( 0 ) , f_capacity( dim ) {}

 [[nodiscard]] bool isEmpty( void ) const { return( f_size == 0 ); }

 /// the item on top
 [[nodiscard]] const T & top( void ) const {
  if( f_size == 0 )
   throw( std::logic_error( "ADS::ArrayStack::top: empty stack" ) );
  return( f_A[ f_size - 1 ] );
  }

 /// removes the item on top and returns it
 T pop( void ) {
  if( f_size == 0 )
   throw( std::logic_error( "ADS::ArrayStack::pop: empty stack" ) );
  return( std::move( f_A[ --f_size ] ) );
  }

 /// puts v on top
 void push( const T & v ) {
  if( f_size == f_capacity )
   throw( std::logic_error( "ADS::ArrayStack::push: full stack" ) );
  f_A[ f_size++ ] = v;
  }

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

/*--------------------------------------------------------------------------*/
 private:

 std::unique_ptr< T[] > f_A;   ///< the items, the top in f_A[ f_size - 1 ]
 std::size_t f_size;           ///< the number of items
 std::size_t f_capacity;       ///< the maximum number of items

 };  // end( class ArrayStack )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ArrayStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
