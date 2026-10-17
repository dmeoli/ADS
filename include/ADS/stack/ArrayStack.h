/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The STACK of the book [see ADT.h] realized with a vector of a given
 * capacity: the n items are in the first n places, the top in the place
 * n - 1, so that every operation takes O( 1 ) time.
 *
 * The contracts [see Contract.h] are the preconditions of the book: top()
 * and pop() need a stack that is not empty (size > 0), push() one that is
 * not full (size < capacity); push() ensures one item more and v on top,
 * pop() one item less and the old top returned, which is the constructive
 * form of the axioms \f$ top( push( S , v ) ) = v \f$ and
 * \f$ pop( push( S , v ) ) = S \f$ [see ADT.h]. The invariant is
 * \f$ 0 \leq n \leq capacity \f$. A stack is copied item by item and
 * moved in O( 1 ) time, and two stacks are equal if they hold the same
 * items in the same order, whatever their capacities. The operations are
 * defined in ArrayStack.tpp.
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

#include "ADS/Contract.h"

#include <cstddef>
#include <memory>
#include <utility>

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
 explicit ArrayStack( std::size_t dim );

 ArrayStack( const ArrayStack & other );

 /// the items of other, which is left empty with no capacity
 ArrayStack( ArrayStack && other ) noexcept;

 ArrayStack & operator=( ArrayStack other ) noexcept;

 void swap( ArrayStack & other ) noexcept;

 [[nodiscard]] bool isEmpty( void ) const;

 /// the item on top
 /// \pre the stack is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the item on top and returns it
 /// \pre the stack is not empty
 /// \post one item less, the old top returned
 T pop( void );

 /// puts v on top
 /// \pre the stack is not full
 /// \post one item more, v on top
 void push( const T & v );

 /// the number of items
 [[nodiscard]] std::size_t size( void ) const;

 /// the maximum number of items
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the same items in the same order, whatever the capacities
 [[nodiscard]] bool operator==( const ArrayStack & o ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::unique_ptr< T[] > f_A;   ///< the items, the top in f_A[ f_size - 1 ]
 std::size_t f_size;           ///< the number of items
 std::size_t f_capacity;       ///< the maximum number of items

 };  // end( class ArrayStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/ArrayStack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArrayStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
