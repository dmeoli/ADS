/*--------------------------------------------------------------------------*/
/*--------------------------- File MultiStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * k STACKs in one structure [see ADT.h], beyond the book: the items of all the
 * stacks are cells of a single vector, each linked to the cell below it in its
 * stack, and the free cells form a further list, so that the stacks share the
 * space and none is full while the others have room (the vector doubles when
 * all the cells are used). push(), pop() and top() on the stack i take
 * \f$ O( 1 ) \f$ amortized time; the stacks are numbered from 0. It
 * generalizes the two stacks in one vector of exercise 4.7.
 *
 * The contracts [see Contract.h]: k > 0, a stack i in [ 0 , k ), and a
 * stack that is not empty for top() and pop(); the invariant is that each
 * cell is in exactly one stack or in the free list, and that the sizes
 * count the cells of each stack. The operations are defined in
 * MultiStack.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 4.7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_MultiStack
 #define __ADS_MultiStack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <ranges>
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
 /// \pre k > 0
 explicit MultiStack( int k );

 [[nodiscard]] int stacks( void ) const;

 /// \pre 0 <= i < k
 [[nodiscard]] bool empty( int i ) const;

 /// \pre 0 <= i < k
 [[nodiscard]] int size( int i ) const;

 /// \pre 0 <= i < k, the stack i is not empty
 [[nodiscard]] const T & top( int i ) const;

 /// v on the stack i; v may be an item of the stacks, e.g., top( j )
 /// \pre 0 <= i < k
 /// \post one item more in the stack i
 void push( int i , const T & v );

 /// \pre 0 <= i < k, the stack i is not empty
 /// \post one item less in the stack i
 T pop( int i );

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  T value;
  int below;
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of each stack, from the top to the bottom, forward

 using value_type = T;
 using const_iterator =
  IndexIterator< Cell , T , &Cell::below , &Cell::value >;

 /// the items of the stack i, the top first
 /// \pre 0 <= i < stacks()
 [[nodiscard]] std::ranges::subrange< const_iterator > stack( int i ) const {
  ADS_PRE( ( i >= 0 ) && ( i < stacks() ) );
  return( std::ranges::subrange( const_iterator( &f_C , f_top[ i ] ) ,
                                 const_iterator( &f_C , -1 ) ) );
  }

 private:

 /// each cell in one stack or free, the sizes right, \f$ O( n ) \f$
 [[nodiscard]] bool partitioned( void ) const;

 std::vector< Cell > f_C;       ///< the cells of all the stacks
 std::vector< int > f_top;      ///< the top cell of each stack, -1 if none
 std::vector< int > f_size;
 int f_free = -1;               ///< the first free cell

 };  // end( class MultiStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/MultiStack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultiStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MultiStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
