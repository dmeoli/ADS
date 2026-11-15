/*--------------------------------------------------------------------------*/
/*------------------------ File RealTimeMinQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The QUEUE with the minimum of exercise 3.15 of Morin's book in
 * \f$ O( 1 ) \f$ worst-case time per operation, beyond the book [see
 * `ADT.h` and `MinQueue.h` for the amortized one]: `enqueue()`,
 * `dequeue()`, `top()`, `size()` and `min()`, by the real-time queue of
 * Hood and Melville on arrays with the minimum.
 *
 * The items are on two stacks, as in `MinQueue`: the front \f$ F \f$, the
 * first item on top, and the back \f$ R \f$, the last item on top, each
 * cell with the place of the least item at or below it. When \f$ R \f$
 * has one item more than \f$ F \f$, a rotation starts: \f$ R \f$ is frozen,
 * a new back takes the insertions, and a new front \f$ N \f$ is built from
 * the bottom, first the items of the frozen back from the last one, then
 * those of \f$ F \f$ from its bottom, three moves per operation, while the
 * removals still take the top of \f$ F \f$; the rotation ends when the
 * moves of \f$ F \f$ meet its top, and \f$ N \f$ becomes the front. With
 * \f$ |R| = |F| + 1 \f$ at the start, the \f$ |F| + 1 \f$ moves of the back
 * end within \f$ \lceil ( |F| + 1 ) / 3 \rceil \f$ operations, before
 * \f$ F \f$ empties, and the whole rotation within
 * \f$ \lceil ( 2 |F| + 1 ) / 3 \rceil \f$; the minimum is the least of the
 * minima of the parts, the one of \f$ F \f$ read in \f$ N \f$ when its
 * place has moved there. The arrays are allocated with their final size
 * when a rotation starts and emptied by the moves, so that no operation
 * copies or frees more than \f$ O( 1 ) \f$ items (the allocation counted as
 * \f$ O( 1 ) \f$). The rotation is that of the real-time windows of
 * `SlidingAggregate` [see `SlidingAggregate.h`], on the cells with the
 * least place of `detail::MinCells`. See \ref theory_queue_minimum.
 *
 * The contracts [see `Contract.h`]: `top()`, `dequeue()` and `min()` need a
 * queue that is not empty; `enqueue()` ensures one item more, `dequeue()`
 * one less, and the rotation ends before the front empties (an
 * invariant). The operations are defined in `RealTimeMinQueue.tpp`.
 *
 * \see R. Hood, R. Melville, Real-time queue operations in pure LISP,
 *      Information Processing Letters 13(2), 50-54, 1981,
 *      doi:10.1016/0020-0190(81)90030-2
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 3.15
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

#ifndef __ADS_RealTimeMinQueue
 #define __ADS_RealTimeMinQueue
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/queue/SlidingAggregate.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the cells of the stacks with the minimum [see `CellArray` in
/// `SlidingAggregate.h`]: each holds an item and the place of the least
/// item from the place 0 up to it, by the order `less`, the same on a
/// front and on a back
template< class T , class C >
struct MinCells
{
 using Item = T;               ///< the items
 using Summary = std::size_t;  ///< the place of the least item up to a cell

 /// the cells of the order `l`
 explicit MinCells( C l = C() ) : less( std::move( l ) ) { }

 /// \f$ x \f$ in a new cell on top of \f$ a \f$, with the least place by
 /// `less`
 ///
 /// In pseudocode, \f$ \mu( i ) \f$ the least place of the cell
 /// \f$ i \f$, on an array whose cells are live from the place 0:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( x ) \cr
 /// 2 & \quad a[ \mathit{hi} ] \gets x;\ \mu( \mathit{hi} ) \gets
 ///     \mathit{hi} \cr
 /// 3 & \quad \textbf{if}\ \mathit{hi} > \mathit{lo}\ \textbf{and not}
 ///     \ x < a[ \mu( \mathit{hi} - 1 ) ]\ \textbf{then}\ \mu(
 ///     \mathit{hi} ) \gets \mu( \mathit{hi} - 1 ) \cr
 /// 4 & \quad \mathit{hi} \gets \mathit{hi} + 1
 /// \end{array}}
 /// \f]
 template< class A , class U >
 void push( A & a , U && x ) const {
  const std::size_t h = a.hi();
  const std::size_t m =
    ( h > a.lo() ) && ! less( x , a.at( a.summary( h - 1 ) ) )
      ? a.summary( h - 1 )
      : h;
  a.push( std::forward< U >( x ) , m );
  }

 /// \f$ x \f$ on top of a front, by `push()`
 template< class A , class U >
 void pushFront( A & a , U && x ) const {
  push( a , std::forward< U >( x ) );
  }

 /// \f$ x \f$ on top of a back, by `push()`
 template< class A , class U >
 void pushBack( A & a , U && x ) const {
  push( a , std::forward< U >( x ) );
  }

 /// the least item of all the cells of \f$ a \f$, those from 0 to the top
 template< class A >
 [[nodiscard]] const T & min( const A & a ) const {
  return( a.at( a.summary( a.hi() - 1 ) ) );
  }

 ADS_NO_UNIQUE_ADDRESS C less;  ///< the order
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS RealTimeMinQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a QUEUE with its least item, every operation in \f$ O( 1 ) \f$
/// worst-case time
template< class T , class C = std::less<> >
class RealTimeMinQueue
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items

 /// the empty queue, ordered by `less`
 explicit RealTimeMinQueue( C less = C() )
     : f_q( Cells( std::move( less ) ) ) { }

 /// a copy, item by item, in \f$ O( n ) \f$ time
 RealTimeMinQueue( const RealTimeMinQueue & o ) = default;

 /// the items of `o`, which is left empty
 RealTimeMinQueue( RealTimeMinQueue && o ) noexcept = default;

 /// the items of `o`, by copy and swap
 RealTimeMinQueue & operator=( const RealTimeMinQueue & o ) = default;

 /// the items of `o`, exchanged with those of this queue
 RealTimeMinQueue & operator=( RealTimeMinQueue && o ) noexcept = default;

 /// exchanges the items with `o`
 void swap( RealTimeMinQueue & o ) noexcept { f_q.swap( o.f_q ); }

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_q.size() == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_q.size() ); }

 /// \f$ x \f$ at the back, on the stack \f$ R \f$, and three moves of a
 /// rotation if one is going on
 /// \post one item more
 void enqueue( const T & x );

 /// removes the item at the front, the top of \f$ F \f$, and returns it,
 /// with three moves of a rotation if one is going on
 ///
 /// In pseudocode (Hood and Melville, beyond the book), \f$ R' \f$ the
 /// frozen back, \f$ N \f$ the new front, \f$ i \f$ the next place of
 /// \f$ F \f$ to move:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{dequeue}() \cr
 /// 2 & \quad x \gets \mathrm{pop}( F ) \cr
 /// 3 & \quad \textbf{if}\ \text{a rotation is going on}\ \textbf{then}
 ///     \ \mathrm{steps}( 3 ) \cr
 /// 4 & \quad \textbf{else if}\ |R| > |F|\ \textbf{then}\ R' \gets R;
 ///     \ R \gets \emptyset;\ N \gets \emptyset;\ i \gets 0;
 ///     \ \mathrm{steps}( 3 ) \cr
 /// 5 & \quad \textbf{return}\ x \cr
 /// 6 & \textbf{procedure}\ \mathrm{steps}( k ) \cr
 /// 7 & \quad \textbf{repeat}\ k\ \textbf{times} \cr
 /// 8 & \qquad \textbf{if}\ R' \ne \emptyset\ \textbf{then}
 ///     \ \mathrm{push}( N , \mathrm{pop}( R' ) ) \cr
 /// 9 & \qquad \textbf{else if}\ i < |F|\ \textbf{then}\ \mathrm{push}( N ,
 ///     F[ i ] );\ i \gets i + 1 \cr
 /// 10 & \qquad \textbf{else}\ F \gets N;\ \text{the rotation ends};
 ///     \ \textbf{return}
 /// \end{array}}
 /// \f]
 /// (`enqueue()` runs the lines 3 and 4 after its push on \f$ R \f$).
 /// \pre the queue is not empty
 /// \post one item less
 T dequeue( void );

 /// the item at the front, the top of \f$ F \f$
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// the least item, the least of the minima of the parts
 ///
 /// In pseudocode, \f$ r_0 \f$ the size of \f$ R' \f$ when the
 /// rotation started, and \f$ \mu \f$ the least place of a cell:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{min}() \cr
 /// 2 & \quad X \gets \emptyset \cr
 /// 3 & \quad \textbf{if}\ F \ne \emptyset\ \textbf{then}
 ///     \ a \gets \mu( \text{the top of } F ) \cr
 /// 4 & \qquad \textbf{if}\ a \text{ is a live place of } F
 ///     \ \textbf{then}\ X \gets \{ F[ a ] \}\ \textbf{else}\ X \gets
 ///     \{ N[ r_0 + a ] \} \cr
 /// 5 & \quad \textbf{foreach}\ P \in \{ R' , N , R \}\ \textbf{do}
 ///     \ \textbf{if}\ P \ne \emptyset\ \textbf{then}\ X \gets X
 ///     \cup \{ \min P \} \cr
 /// 6 & \quad \textbf{return}\ \text{the least item of } X
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// `f( x )` on the items from the front to the back, \f$ O( n ) \f$
 template< class F >
 void forEach( F && f ) const;

 /// removes all the items
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:
 using Cells = detail::MinCells< T , C >;  ///< the cells with the minimum

 /// the rotation of Hood and Melville on the stacks with the minimum [see
 /// `SlidingAggregate.h`]
 detail::RealTimeRotation< Cells > f_q;

 };  // end( class RealTimeMinQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/RealTimeMinQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif /* RealTimeMinQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RealTimeMinQueue.h -----------------------*/
/*--------------------------------------------------------------------------*/
