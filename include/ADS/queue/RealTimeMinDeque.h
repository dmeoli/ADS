/*--------------------------------------------------------------------------*/
/*------------------------ File RealTimeMinDeque.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The double ended queue with the minimum of exercise 3.16 of Morin's
 * book in \f$ O( 1 ) \f$ worst-case time per operation, beyond the book
 * [see `MinDeque.h` for the amortized one]: `push_front()`, `push_back()`,
 * `pop_front()`, `pop_back()`, `front()`, `back()`, `size()` and `min()`,
 * by the global rebuilding of Overmars applied to the two stacks of
 * `MinDeque`, as Gajewska and Tarjan do for the deques with heap order.
 *
 * The items are on two stacks back to back, \f$ F \f$ with the front on top
 * and \f$ B \f$ with the back on top, arrays whose cells hold the place of
 * the least item at or below them. When one side \f$ G \f$ has more than
 * \f$ 3 m + 1 \f$ items, \f$ m \f$ those of the other side \f$ S \f$, or a
 * side fills half its array, a rebuild starts: of the \f$ n_0 \f$ items,
 * the \f$ \lceil n_0 / 2 \rceil \f$ nearest the top of \f$ G \f$ will stay
 * on its side and the \f$ t \f$ at its bottom will pass to the other, and
 * two new arrays of \f$ 2 n_0 + 16 \f$ cells are filled from the bottom
 * with copies, twelve per operation: \f$ S' \f$ with the bottom of
 * \f$ G \f$ from \f$ G[ t - 1 ] \f$ down to \f$ G[ 0 ] \f$ and then the
 * items of \f$ S \f$ from its bottom, \f$ G' \f$ with
 * \f$ G[ t ] , G[ t + 1 ] , \ldots \f$. The operations go on on the old
 * stacks meanwhile, and a copy keeps a prefix of its stack, shortened by
 * one when a removal goes below its end; the rebuild ends when both
 * prefixes reach their tops, and the new arrays replace the old ones,
 * which are destroyed twelve cells per operation afterwards. A deque of
 * fewer than 32 items is rebuilt at once, in \f$ O( 1 ) \f$. A rebuild
 * takes fewer than \f$ n_0 / 11 + 2 \f$ operations, during which neither
 * side of the old stacks empties nor goes below the cells being copied,
 * so that the minimum is the lesser of the minima of the two old stacks;
 * the next rebuild comes \f$ \Omega( n_0 ) \f$ operations later, after the
 * old arrays are gone. See \ref theory_queue_minimum. The items are
 * copied, so that `T` must be copyable.
 *
 * The contracts [see `Contract.h`]: the removals, `front()`, `back()` and
 * `min()` need a deque that is not empty; the insertions ensure one item
 * more and the removals one less; a rebuild never reads a cell that a
 * removal has destroyed (an invariant). The operations are defined in
 * `RealTimeMinDeque.tpp`.
 *
 * \see H. Gajewska, R. E. Tarjan, Deques with heap order, Information
 *      Processing Letters 22(4), 197-200, 1986,
 *      doi:10.1016/0020-0190(86)90028-1
 * \see M. H. Overmars, The Design of Dynamic Data Structures, Lecture
 *      Notes in Computer Science 156, Springer, 1983,
 *      doi:10.1007/BFb0014927 (the global rebuilding)
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

#ifndef __ADS_RealTimeMinDeque
 #define __ADS_RealTimeMinDeque
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/queue/RealTimeMinQueue.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS RealTimeMinDeque -------------------------*/
/*--------------------------------------------------------------------------*/

/// a double ended queue with its least item, every operation in
/// \f$ O( 1 ) \f$ worst-case time
template< std::copy_constructible T , class C = std::less<> >
class RealTimeMinDeque
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the items

 /// the empty deque, ordered by `less`
 explicit RealTimeMinDeque( C less = C() )
     : f_A{ Array( 16 ) , Array( 16 ) } , f_cells( std::move( less ) ) { }

 /// a copy, item by item, in \f$ O( n ) \f$ time
 RealTimeMinDeque( const RealTimeMinDeque & o );

 /// the items of `o`, which is left empty
 RealTimeMinDeque( RealTimeMinDeque && o ) noexcept
     : f_A{ Array( 16 ) , Array( 16 ) } , f_cells( o.f_cells ) {
  swap( o );
  }

 /// the items of `o`, by copy and swap
 RealTimeMinDeque & operator=( const RealTimeMinDeque & o );

 /// the items of `o`, exchanged with those of this deque
 RealTimeMinDeque & operator=( RealTimeMinDeque && o ) noexcept {
  swap( o );
  return( *this );
  }

 /// exchanges the items with `o`
 void swap( RealTimeMinDeque & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// \f$ x \f$ at the front, on \f$ F \f$
 /// \post one item more
 void push_front( const T & x ) { push( 0 , x ); }

 /// \f$ x \f$ at the back, on \f$ B \f$
 /// \post one item more
 void push_back( const T & x ) { push( 1 , x ); }

 /// removes the item at the front and returns it
 /// \pre the deque is not empty
 /// \post one item less
 T pop_front( void ) { return( pop( 0 ) ); }

 /// removes the item at the back and returns it
 /// \pre the deque is not empty
 /// \post one item less
 T pop_back( void ) { return( pop( 1 ) ); }

 /// the item at the front
 /// \pre the deque is not empty
 [[nodiscard]] const T & front( void ) const { return( end( 0 ) ); }

 /// the item at the back
 /// \pre the deque is not empty
 [[nodiscard]] const T & back( void ) const { return( end( 1 ) ); }

 /// the least item, the lesser of the minima of the two stacks
 /// \pre the deque is not empty
 [[nodiscard]] const T & min( void ) const;

 /// `f( x )` on the items from the front to the back, \f$ O( n ) \f$
 template< class F >
 void forEach( F && f ) const;

 /// removes all the items
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:
 using Array = detail::CellArray< T , std::size_t >;  ///< a stack
 using Cells = detail::MinCells< T , C >;  ///< its cells, with the minimum

 /// the copies per operation, and the size below which a rebuild is done
 /// at once
 static constexpr int kSteps = 12;
 static constexpr std::size_t kSmall = 32;

 /// \f$ x \f$ on the side \f$ s \f$ (0 the front, 1 the back)
 void push( int s , const T & x );

 /// the item removed from the side \f$ s \f$, or from the other one if
 /// \f$ s \f$ is empty, which then has one item
 ///
 /// In pseudocode, \f$ A_0 = F \f$ and \f$ A_1 = B \f$, \f$ A'_x \f$
 /// the copy of the side \f$ x \f$ in a rebuild, and \f$ c_x \f$ the end
 /// of the prefix of \f$ A_x \f$ that it holds (\f$ c_G \f$ or
 /// \f$ c_S \f$ of `rebuild()`):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}( s ) \cr
 /// 2 & \quad x \gets s;\ \textbf{if}\ A_s = \emptyset\ \textbf{then}
 ///     \ x \gets 1 - s\quad \triangleright\ \text{the only item} \cr
 /// 3 & \quad v \gets \mathrm{pop}( A_x );\ n \gets n - 1 \cr
 /// 4 & \quad \textbf{if}\ \text{a rebuild is going on}\ \textbf{and}
 ///     \ |A_x| < c_x\ \textbf{then}\ \mathrm{pop}( A'_x );\ c_x
 ///     \gets c_x - 1 \cr
 /// 5 & \quad \text{destroy up to 12 cells of the old arrays} \cr
 /// 6 & \quad \textbf{if}\ \text{a rebuild is going on}\ \textbf{then}
 ///     \ \text{12 steps of } \mathrm{rebuild}()
 ///     \ \textbf{else if}\ \mathrm{due}()\ \textbf{then}\ \text{start }
 ///     \text{one} \cr
 /// 7 & \quad \textbf{return}\ v
 /// \end{array}}
 /// \f]
 /// (`push()` runs the lines 5 and 6 after its push on \f$ A_s \f$).
 T pop( int s );

 /// the item at the end \f$ s \f$
 const T & end( int s ) const;

 /// whether a rebuild is due: a side larger than \f$ 3 m + 1 \f$, or
 /// half full
 ///
 /// In pseudocode, \f$ \mathrm{cap}( A ) \f$ the cells of the array
 /// \f$ A \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{due}() \cr
 /// 2 & \quad \textbf{return}\ \max\{ |F| , |B| \} > 3 \min\{ |F| ,
 ///     |B| \} + 1\ \textbf{or}\ 2 |F| \ge \mathrm{cap}( F )
 ///     \ \textbf{or}\ 2 |B| \ge \mathrm{cap}( B )
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool due( void ) const;

 /// the work of an operation: the old arrays destroyed a little more, the
 /// copies of a rebuild going on, or a rebuild started if one is due
 void afterOp( void );

 /// a rebuild started, done at once for fewer than 32 items
 ///
 /// In pseudocode (Overmars, Gajewska and Tarjan, beyond the book),
 /// \f$ G \f$ the larger side and \f$ S \f$ the other one, \f$ n_0 \f$ the
 /// items, \f$ t = |G| - \lceil n_0 / 2 \rceil \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rebuild}() \cr
 /// 2 & \quad S' \gets \text{an empty array};\ G' \gets \text{an empty }
 ///     \text{array};\ p \gets t;\ c_S \gets 0;\ c_G \gets t \cr
 /// 3 & \quad \textbf{repeat}\ 12\ \textbf{times per operation} \cr
 /// 4 & \qquad \textbf{if}\ p > 0\ \textbf{then}\ p \gets p - 1;
 ///     \ \mathrm{push}( S' , G[ p ] ) \cr
 /// 5 & \qquad \textbf{else if}\ c_S < |S|\ \textbf{then}\ \mathrm{push}(
 ///     S' , S[ c_S ] );\ c_S \gets c_S + 1 \cr
 /// 6 & \qquad \textbf{else if}\ c_G < |G|\ \textbf{then}\ \mathrm{push}(
 ///     G' , G[ c_G ] );\ c_G \gets c_G + 1 \cr
 /// 7 & \qquad \textbf{else}\ ( G , S ) \gets ( G' , S' );\ \text{the }
 ///     \text{old arrays to destroy};\ \textbf{return}
 /// \end{array}}
 /// \f]
 /// with a removal from \f$ S \f$ (from \f$ G \f$) that leaves
 /// \f$ |S| < c_S \f$ (\f$ |G| < c_G \f$) taking also the top of
 /// \f$ S' \f$ (of \f$ G' \f$).
 void start( void );

 /// at most \f$ k \f$ copies of the rebuild going on
 void steps( int k );

 std::array< Array , 2 > f_A;           ///< the front and the back
 std::array< Array , 2 > f_new;         ///< the new arrays of a rebuild
 std::array< Array , 4 > f_old;         ///< the old arrays to destroy
 int f_g = 0;                          ///< the larger side of the rebuild
 std::size_t f_p = 0;                  ///< the bottom of G still to copy
 std::size_t f_cs = 0;                 ///< the prefix of S copied
 std::size_t f_cg = 0;                 ///< the end of the prefix of G
 std::size_t f_t = 0;                  ///< the bottom of G passing to S
 std::size_t f_n = 0;                  ///< the items
 bool f_rb = false;                    ///< a rebuild is going on
 ADS_NO_UNIQUE_ADDRESS Cells f_cells;  ///< the order, and the pushes

 };  // end( class RealTimeMinDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/RealTimeMinDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif /* RealTimeMinDeque.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RealTimeMinDeque.h -----------------------*/
/*--------------------------------------------------------------------------*/
