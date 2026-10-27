/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The STACK of the book [see `ADT.h`] realized with a vector of a given
 * capacity \f$ c \f$: the \f$ n \f$ items are in the first \f$ n \f$
 * places, the top in the place \f$ n - 1 \f$, so that every operation takes
 * \f$ O( 1 ) \f$ time.
 *
 * The contracts [see `Contract.h`] are the preconditions of the book:
 * `top()` and `pop()` need a stack that is not empty (\f$ n > 0 \f$),
 * `push()` one that is not full (\f$ n < c \f$); `push()` ensures one item
 * more and \f$ v \f$ on top, `pop()` one item less and the old top returned,
 * which is the constructive form of the axioms
 * \f$ \mathrm{top}( \mathrm{push}( S , v ) ) = v \f$ and
 * \f$ \mathrm{pop}( \mathrm{push}( S , v ) ) = S \f$ [see `ADT.h`]. The
 * invariant is \f$ 0 \leq n \leq c \f$. A stack is copied item by item and
 * moved in \f$ O( 1 ) \f$ time, and two stacks are equal if they hold the
 * same items in the same order, whatever their capacities. The operations
 * are defined in `ArrayStack.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.2,
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

 /// the empty stack of at most \f$ \mathit{dim} \f$ items
 explicit ArrayStack( std::size_t dim );

 /// a copy of `other`, item by item
 ArrayStack( const ArrayStack & other );

 /// the items of `other`, which is left empty with no capacity
 ArrayStack( ArrayStack && other ) noexcept;

 /// the items of `other`, by copy and swap
 ArrayStack & operator=( ArrayStack other ) noexcept;

 /// exchanges the items and the capacities with `other`
 void swap( ArrayStack & other ) noexcept;

 /// true if there is no item, i.e., \f$ n = 0 \f$
 [[nodiscard]] bool empty( void ) const;

 /// the item on top, \f$ A[ n - 1 ] \f$
 /// \pre the stack is not empty
 [[nodiscard]] const T & top( void ) const;

 /// removes the item on top and returns it
 ///
 /// In pseudocode (section 4.3.2), \f$ A[ 0 .. c - 1 ] \f$ the vector:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad n \gets n - 1 \cr
 /// 3 & \quad \textbf{return}\ A[ n ]
 /// \end{array}}
 /// \f]
 /// \pre the stack is not empty
 /// \post one item less, the old top returned
 T pop( void );

 /// puts \f$ v \f$ on top
 ///
 /// In pseudocode (section 4.3.2):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( v ) \cr
 /// 2 & \quad A[ n ] \gets v;\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre the stack is not full
 /// \post one item more, \f$ v \f$ on top
 void push( const T & v );

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

 /// the maximum number of items, \f$ c \f$
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the same items in the same order, whatever the capacities
 [[nodiscard]] bool operator==( const ArrayStack & o ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items, from the bottom to the top, contiguous;
 // the operations of the stack stay the only way to change it

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const T &;              ///< a constant reference
 using const_reference = const T &;        ///< a constant reference
 using iterator = const T *;               ///< a constant iterator
 using const_iterator = const T *;         ///< a constant iterator

 /// the iterator on the item at the bottom
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.get() ); }

 /// the iterator past the item on top
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.get() + f_size );
  }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 std::unique_ptr< T[] > f_A;   ///< the items, the top in `f_A[ f_size - 1 ]`
 std::size_t f_size;           ///< the number of items
 std::size_t f_capacity;       ///< the maximum number of items

 };  // end( class ArrayStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/ArrayStack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArrayStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
