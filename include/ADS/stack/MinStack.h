/*--------------------------------------------------------------------------*/
/*---------------------------- File MinStack.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The STACK with the minimum of exercise 3.14 of Morin's book, beyond the
 * book [see `ADT.h`]: `push()`, `pop()`, `top()`, `size()` and `min()`,
 * the least item inside, all in \f$ O( 1 ) \f$ amortized time (worst case
 * but for the doublings of the vectors). Beside the vector of the items, a
 * second vector keeps the minima: an item goes there too when it is not
 * larger than the minimum on its top, and leaves it with the item; hence
 * the top of the minima is the least item of the stack, since every item
 * above the item that put it there is not smaller. The minima are one item
 * per record low from the bottom, \f$ n \f$ at most (a decreasing input)
 * and far fewer on most inputs, where the pairs of an item and its minimum
 * would store \f$ 2 n \f$ items.
 *
 * The order is that of `C`, a strict weak order, `std::less` by default;
 * an item equal to the minimum goes to the minima too, so that removing it
 * leaves the minimum of the others. The contracts [see `Contract.h`]:
 * `top()`, `pop()` and `min()` need a stack that is not empty; the
 * invariant is that the minima do not increase from the bottom and that
 * their top is the least item, an \f$ O( n ) \f$ check. The operations are
 * defined in `MinStack.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 3.14
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

#ifndef __ADS_MinStack
 #define __ADS_MinStack
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS MinStack -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a STACK with its least item in \f$ O( 1 ) \f$ time
template< class T , class C = std::less<> >
class MinStack
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items

 /// the empty stack, ordered by `less`
 explicit MinStack( C less = C() ) : f_less( std::move( less ) ) { }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_A.empty() ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item on top
 /// \pre the stack is not empty
 [[nodiscard]] const T & top( void ) const {
  ADS_PRE( ! f_A.empty() );
  return( f_A.back() );
  }

 /// the least item
 /// \pre the stack is not empty
 [[nodiscard]] const T & min( void ) const {
  ADS_PRE( ! f_A.empty() );
  return( f_M.back() );
  }

 /// puts \f$ x \f$ on top, and among the minima if it is not larger than
 /// the least item
 ///
 /// In pseudocode (Morin, exercise 3.14), \f$ A \f$ the items and \f$ M \f$
 /// the minima:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad \mathrm{push}( A , x ) \cr
 /// 3 & \quad \textbf{if}\ M = \emptyset\ \textbf{or not}\ \mathrm{top}( M )
 ///     < x\ \textbf{then}\ \mathrm{push}( M , x )
 /// \end{array}}
 /// \f]
 /// \post one item more, \f$ x \f$ on top
 void push( const T & x );

 /// removes the item on top and returns it, from the minima too if it is
 /// their top
 ///
 /// In pseudocode (Morin, exercise 3.14):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad x \gets \mathrm{pop}( A ) \cr
 /// 3 & \quad \textbf{if not}\ \mathrm{top}( M ) < x\ \textbf{then}
 ///     \ \mathrm{pop}( M ) \cr
 /// 4 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the stack is not empty
 /// \post one item less
 T pop( void );

 /// removes all the items
 void clear( void ) {
  f_A.clear();
  f_M.clear();
  }

 /// the items from the bottom to the top
 [[nodiscard]] const std::vector< T > & items( void ) const { return( f_A ); }

 /// the number of minima kept, \f$ n \f$ at most
 [[nodiscard]] std::size_t minima( void ) const { return( f_M.size() ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the minima do not increase and their top is the least item
 [[nodiscard]] bool ordered( void ) const;

 std::vector< T > f_A;  ///< the items, the top at the back
 std::vector< T > f_M;  ///< the minima, not increasing from the bottom
 ADS_NO_UNIQUE_ADDRESS C f_less;  ///< the order

 };  // end( class MinStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/MinStack.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MinStack.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MinStack.h ---------------------------*/
/*--------------------------------------------------------------------------*/
