/*--------------------------------------------------------------------------*/
/*---------------------------- File MinDeque.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The double ended queue with the minimum of exercise 3.16 of Morin's
 * book, beyond the book: `push_front()`, `push_back()`, `pop_front()`,
 * `pop_back()`, `size()` and `min()`, all in \f$ O( 1 ) \f$ amortized time,
 * on two stacks with the minimum [see `MinStack.h`] back to back, as the
 * `DualArrayDeque` of Morin's book [see `DualArrayDeque.h`]: \f$ F \f$ with
 * the front on top and \f$ B \f$ with the back on top. A removal from an
 * empty side deals the items of the other side again, half to each, the
 * front half to \f$ F \f$, in \f$ O( k ) \f$ time for \f$ k \f$ items;
 * after it each side holds \f$ \lfloor k / 2 \rfloor \f$ items at least, so
 * that \f$ \Omega( k ) \f$ operations come before another deal, and with the
 * potential \f$ \Phi = 2 \, | |F| - |B| | \f$ each operation costs
 * \f$ O( 1 ) \f$ amortized. The least item is the lesser of the minima of
 * the two stacks. `RealTimeMinDeque` [see `RealTimeMinDeque.h`] spreads
 * the deals over the operations, for \f$ O( 1 ) \f$ in the worst case.
 *
 * The contracts [see `Contract.h`]: the removals, `front()`, `back()` and
 * `min()` need a deque that is not empty; the insertions ensure one item
 * more and the removals one less. The operations are defined in
 * `MinDeque.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 3.16
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

#ifndef __ADS_MinDeque
 #define __ADS_MinDeque
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/MinStack.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS MinDeque -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a double ended queue with its least item in \f$ O( 1 ) \f$ amortized
/// time
template< class T , class C = std::less<> >
class MinDeque {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty deque, ordered by `less`
 explicit MinDeque( C less = C() )
  : f_F( less ) , f_B( less ) , f_less( less ) {}

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const {
  return( f_F.empty() && f_B.empty() );
  }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const {
  return( f_F.size() + f_B.size() );
  }

 /// \f$ x \f$ at the front
 void push_front( const T & x ) { f_F.push( x ); }

 /// \f$ x \f$ at the back
 void push_back( const T & x ) { f_B.push( x ); }

 /// removes the item at the front and returns it
 ///
 /// In pseudocode (Morin, exercise 3.16), \f$ F \f$ and \f$ B \f$ the two
 /// stacks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popFront}() \cr
 /// 2 & \quad \textbf{if}\ F = \emptyset\ \textbf{then}\ \mathrm{deal}( B ,
 ///     F ) \cr
 /// 3 & \quad \textbf{return}\ \mathrm{pop}( F )
 /// \end{array}}
 /// \f]
 /// \pre the deque is not empty
 /// \post one item less
 T pop_front( void );

 /// removes the item at the back and returns it, the mirror of
 /// `pop_front()`
 /// \pre the deque is not empty
 /// \post one item less
 T pop_back( void );

 /// the item at the front
 /// \pre the deque is not empty
 [[nodiscard]] const T & front( void ) const;

 /// the item at the back
 /// \pre the deque is not empty
 [[nodiscard]] const T & back( void ) const;

 /// the least item, the lesser of the minima of the two stacks
 /// \pre the deque is not empty
 [[nodiscard]] const T & min( void ) const;

 /// removes all the items
 void clear( void ) { f_F.clear(); f_B.clear(); }

/*--------------------------------------------------------------------------*/
 private:

 /// the items of \f$ s \f$, which \f$ t \f$ (empty) lacks, dealt again:
 /// the half nearer to the end of \f$ t \f$ to \f$ t \f$, the end on top
 ///
 /// In pseudocode (Morin, exercise 3.16), \f$ s_0 , \ldots , s_{k-1} \f$
 /// the items of \f$ s \f$ from the bottom:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{deal}( s , t ) \cr
 /// 2 & \quad h \gets \lceil k / 2 \rceil \cr
 /// 3 & \quad \textbf{for}\ i \gets h - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \ \mathrm{push}( t , s_i ) \cr
 /// 4 & \quad s \gets s_h , \ldots , s_{k-1}
 /// \end{array}}
 /// \f]
 static void deal( MinStack< T , C > & s , MinStack< T , C > & t );

 MinStack< T , C > f_F;   ///< the front, the first item on top
 MinStack< T , C > f_B;   ///< the back, the last item on top
 [[no_unique_address]] C f_less;   ///< the order

 };  // end( class MinDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/MinDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MinDeque.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MinDeque.h ---------------------------*/
/*--------------------------------------------------------------------------*/
