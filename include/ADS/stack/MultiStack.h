/*--------------------------------------------------------------------------*/
/*--------------------------- File MultiStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * k STACKs in one structure [see ADT.h], beyond the book: the items of
 * all the stacks are cells of a single vector, each linked to the cell
 * below it in its stack, and the free cells form a further list, so that
 * the stacks share the space and none is full while the others have room
 * (the vector doubles when all the cells are used). push(), pop() and
 * top() on the stack i take O( 1 ) amortized time; the stacks are numbered
 * from 0. It generalizes the two stacks in one vector of exercise 4.7.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.7,
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

#ifndef __ADS_MultiStack
 #define __ADS_MultiStack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MultiStack ----------------------------*/
/*--------------------------------------------------------------------------*/

/// k stacks sharing a vector of linked cells
template< class T >
class MultiStack {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// k empty stacks
 explicit MultiStack( int k ) : f_top( check_k( k ) , -1 ) , f_size( k , 0 ) {}

 [[nodiscard]] int stacks( void ) const { return( int( f_top.size() ) ); }

 [[nodiscard]] bool isEmpty( int i ) const {
  return( f_top[ check( i ) ] < 0 );
  }

 [[nodiscard]] int size( int i ) const { return( f_size[ check( i ) ] ); }

 [[nodiscard]] const T & top( int i ) const {
  if( isEmpty( i ) )
   throw( std::logic_error( "ADS::MultiStack::top: empty stack" ) );
  return( f_C[ f_top[ i ] ].value );
  }

 void push( int i , const T & v ) {
  check( i );
  int c = f_free;
  if( c < 0 ) {
   f_C.push_back( {} );
   c = int( f_C.size() ) - 1;
   }
  else f_free = f_C[ c ].below;
  f_C[ c ] = { v , f_top[ i ] };
  f_top[ i ] = c;
  ++f_size[ i ];
  }

 T pop( int i ) {
  if( isEmpty( i ) )
   throw( std::logic_error( "ADS::MultiStack::pop: empty stack" ) );
  const int c = f_top[ i ];
  T v = std::move( f_C[ c ].value );
  f_top[ i ] = f_C[ c ].below;
  f_C[ c ].below = f_free;
  f_free = c;
  --f_size[ i ];
  return( v );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  T value;
  int below;
  };

 static int check_k( int k ) {
  if( k <= 0 ) throw( std::invalid_argument( "ADS::MultiStack: k <= 0" ) );
  return( k );
  }

 int check( int i ) const {
  if( ( i < 0 ) || ( i >= stacks() ) )
   throw( std::out_of_range( "ADS::MultiStack: no such stack" ) );
  return( i );
  }

 std::vector< Cell > f_C;       ///< the cells of all the stacks
 std::vector< int > f_top;      ///< the top cell of each stack, -1 if none
 std::vector< int > f_size;
 int f_free = -1;               ///< the first free cell

 };  // end( class MultiStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MultiStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MultiStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
