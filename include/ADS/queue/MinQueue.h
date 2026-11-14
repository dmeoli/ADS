/*--------------------------------------------------------------------------*/
/*---------------------------- File MinQueue.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The QUEUE with the minimum of exercise 3.15 of Morin's book, beyond the
 * book [see `ADT.h`]: `enqueue()`, `dequeue()`, `top()`, `size()` and
 * `min()`, all in \f$ O( 1 ) \f$ amortized time, on two stacks with the
 * minimum [see `MinStack.h`]: the items enter the stack \f$ I \f$ and
 * leave from the stack \f$ O \f$, which, when empty, takes all the items of
 * \f$ I \f$ in the reverse order, the front on top. Each item is pushed and
 * popped twice at most, so that a sequence of \f$ m \f$ operations costs
 * \f$ O( m ) \f$, and the least item is the lesser of the minima of the two
 * stacks.
 *
 * The contracts [see `Contract.h`]: `top()`, `dequeue()` and `min()` need a
 * queue that is not empty; `enqueue()` ensures one item more, `dequeue()`
 * one less. The operations are defined in `MinQueue.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, exercise 3.15
 * \see R. Hood, R. Melville, Real-time queue operations in pure LISP,
 *      Information Processing Letters 13(2), 50-54, 1981,
 *      doi:10.1016/0020-0190(81)90030-2 (the queue on two stacks)
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

#ifndef __ADS_MinQueue
 #define __ADS_MinQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/MinStack.h"

#include <cstddef>
#include <functional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS MinQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE with its least item in \f$ O( 1 ) \f$ amortized time
template< class T , class C = std::less<> >
class MinQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue, ordered by `less`
 explicit MinQueue( C less = C() )
  : f_I( less ) , f_O( less ) , f_less( less ) {}

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const {
  return( f_I.empty() && f_O.empty() );
  }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const {
  return( f_I.size() + f_O.size() );
  }

 /// \f$ x \f$ at the back, on the stack \f$ I \f$
 /// \post one item more
 void enqueue( const T & x );

 /// removes the item at the front and returns it, from the stack
 /// \f$ O \f$, which takes the items of \f$ I \f$ when it is empty
 ///
 /// In pseudocode (Morin, exercise 3.15):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{dequeue}() \cr
 /// 2 & \quad \textbf{if}\ O = \emptyset\ \textbf{then}\ \textbf{while}
 ///     \ I \ne \emptyset\ \textbf{do}\ \mathrm{push}( O , \mathrm{pop}( I )
 ///     ) \cr
 /// 3 & \quad \textbf{return}\ \mathrm{pop}( O )
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less
 T dequeue( void );

 /// the item at the front, the top of \f$ O \f$ or the bottom of
 /// \f$ I \f$ if \f$ O \f$ is empty
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// the least item, the lesser of the minima of the two stacks
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// removes all the items
 void clear( void ) { f_I.clear(); f_O.clear(); }

/*--------------------------------------------------------------------------*/
 private:

 /// the items of \f$ I \f$ moved to \f$ O \f$, the front on top
 void transfer( void );

 MinStack< T , C > f_I;   ///< the back, the last item on top
 MinStack< T , C > f_O;   ///< the front, the first item on top
 [[no_unique_address]] C f_less;   ///< the order

 };  // end( class MinQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/MinQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MinQueue.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MinQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
