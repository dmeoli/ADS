/*--------------------------------------------------------------------------*/
/*---------------------------- File CellHeap.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A heap of cells of two fields with a garbage collector, beyond the book:
 * the memory of a LISP, where a field holds nil, an atom or the address of
 * a cell, and the cells not reachable from the roots are garbage.
 * `collect()` marks the reachable cells and sweeps the others onto the free
 * list, and `cons()` collects when the free list is empty. The marking is
 * that of Deutsch, Schorr and Waite (Algorithm 2.3.5E of TAOCP): the
 * depth-first visit keeps its path in the fields of the cells themselves,
 * reversing each pointer it follows and restoring it on the way back, with
 * one bit per cell telling which field holds the reversed pointer; it
 * needs no stack, which a collector cannot count on when the memory is
 * exhausted, and takes time linear in the cells reached. A recursive
 * marking with a stack is kept for the comparison. The sweep reads every
 * cell, so that a collection costs \f$ O( n ) \f$ time on a heap of
 * \f$ n \f$ cells, whatever the cells reached.
 *
 * The contracts [see `Contract.h`]: an address is a cell of the heap;
 * `cons()` needs a free cell after a collection; `collect()` ensures that it
 * frees exactly the cells that are not reachable and leaves the fields of
 * the others as they were, an audit. The operations are defined in
 * `CellHeap.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.5
 * \see H. Schorr, W. M. Waite, An efficient machine-independent procedure
 *      for garbage collection in various list structures, Communications of
 *      the ACM 10(8), 501-506, 1967, doi:10.1145/363534.363554
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

#ifndef __ADS_CellHeap
 #define __ADS_CellHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS CellHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a heap of cells of two fields, with a mark-and-sweep collector
class CellHeap {

/*--------------------------------------------------------------------------*/
 public:

 /// a field: `nil`, an atom (`atom( k )`, \f$ -2 - k \f$, for
 /// \f$ k \ge 0 \f$) or a cell (its address, \f$ \ge 0 \f$)
 using Value = long;

 /// the empty field
 static constexpr Value nil = -1;

 /// the atom \f$ k \f$
 /// \pre \f$ k \ge 0 \f$
 static Value atom( long k );

 /// whether \f$ v \f$ is the address of a cell
 [[nodiscard]] static bool isCell( Value v );

 /// the heap of \f$ n \f$ cells, all free
 /// \pre \f$ n \ge 1 \f$
 explicit CellHeap( std::size_t n );

 /// a new cell with the fields `car` and `cdr`, after a collection if no
 /// cell is free
 ///
 /// In pseudocode (section 2.3.5 of TAOCP, beyond the book), \f$ F \f$ the
 /// free list, linked through the field \f$ \mathit{car} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{cons}( a , d ) \cr
 /// 2 & \quad \textbf{if}\ F = \textbf{nil}\ \textbf{then}
 ///     \ \mathrm{collect}() \cr
 /// 3 & \quad c \gets F;\ F \gets c.\mathit{car} \cr
 /// 4 & \quad c.\mathit{car} \gets a;\ c.\mathit{cdr} \gets d;
 ///     \ \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// \pre a cell is free after the collection
 Value cons( Value car , Value cdr );

 /// the field `car` of the cell \f$ c \f$
 /// \pre \f$ c \f$ is a cell of the heap, not free
 [[nodiscard]] Value car( Value c ) const;

 /// the field `cdr` of the cell \f$ c \f$
 /// \pre \f$ c \f$ is a cell of the heap, not free
 [[nodiscard]] Value cdr( Value c ) const;

 /// the field `car` of the cell \f$ c \f$ set to \f$ v \f$
 /// \pre \f$ c \f$ is a cell of the heap, not free
 void setCar( Value c , Value v );

 /// the field `cdr` of the cell \f$ c \f$ set to \f$ v \f$
 /// \pre \f$ c \f$ is a cell of the heap, not free
 void setCdr( Value c , Value v );

 /// the cell \f$ c \f$ made a root, once more
 /// \pre \f$ c \f$ is a cell of the heap
 void addRoot( Value c );

 /// one of the roots \f$ c \f$ removed, if there is one
 void removeRoot( Value c );

 /// marks the cells reachable from the roots, by pointer reversal or with
 /// a stack, and frees the others; the number of the cells freed
 ///
 /// In pseudocode (Algorithm 2.3.5E of TAOCP, the marking of Deutsch,
 /// Schorr and Waite, beyond the book), \f$ \mathit{side}( x ) \f$ true when
 /// the reversed pointer of the cell \f$ x \f$ is in its
 /// \f$ \mathit{cdr} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{collect}() \cr
 /// 2 & \quad \textbf{foreach}\ \text{root } r\ \textbf{do}
 ///     \ \mathrm{mark}( r ) \cr
 /// 3 & \quad F \gets \textbf{nil} \cr
 /// 4 & \quad \textbf{for}\ i \gets n - 1\ \textbf{downto}\ 0
 ///     \ \textbf{do}\quad \triangleright\ \text{the sweep} \cr
 /// 5 & \qquad \textbf{if}\ i \text{ is marked}\ \textbf{then}\ \text{unmark}
 ///     \text{ } i\ \textbf{else}\ i.\mathit{car} \gets F;\ F \gets i \cr
 /// 6 & \quad \textbf{return}\ \text{the cells freed now} \cr
 /// 7 & \textbf{procedure}\ \mathrm{mark}( c ) \cr
 /// 8 & \quad p \gets \textbf{nil} \cr
 /// 9 & \quad \textbf{loop} \cr
 /// 10 & \qquad \textbf{while}\ c \text{ is a cell not marked}\ \textbf{do}
 ///     \quad \triangleright\ \text{down the car} \cr
 /// 11 & \qquad\quad \text{mark } c;\ \mathit{side}( c ) \gets
 ///     \textbf{false} \cr
 /// 12 & \qquad\quad ( c.\mathit{car} , p , c ) \gets ( p , c ,
 ///     c.\mathit{car} ) \cr
 /// 13 & \qquad \textbf{while}\ p \ne \textbf{nil}\ \textbf{and}
 ///     \ \mathit{side}( p )\ \textbf{do}\quad \triangleright\ \text{up from}
 ///     \text{ a cdr} \cr
 /// 14 & \qquad\quad ( p.\mathit{cdr} , c , p ) \gets ( c , p ,
 ///     p.\mathit{cdr} ) \cr
 /// 15 & \qquad \textbf{if}\ p = \textbf{nil}\ \textbf{then return} \cr
 /// 16 & \qquad \mathit{side}( p ) \gets \textbf{true}\quad
 ///     \triangleright\ \text{from the car to the cdr} \cr
 /// 17 & \qquad ( p.\mathit{car} , p.\mathit{cdr} , c ) \gets ( c ,
 ///     p.\mathit{car} , p.\mathit{cdr} )
 /// \end{array}}
 /// \f]
 /// \post the cells freed are the unreachable ones, the fields of the
 ///       others unchanged (audit)
 std::size_t collect( bool reversal = true );

 /// the number of the free cells
 [[nodiscard]] std::size_t freeCells( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  /// the two fields
  Value car = nil , cdr = nil;
  /// reached by the marking
  bool mark = false;
  bool cdrSide = false;    ///< the reversed pointer is in cdr
  /// on the free list
  bool free = true;
  };

 void markReversal( Value root );
 void markStack( Value root );

 /// the cells reachable from the roots, by a plain visit
 [[nodiscard]] std::vector< bool > reachable( void ) const;

 std::vector< Cell > f_C;
 Value f_free = nil;        ///< the free list, through car
 std::size_t f_nfree = 0;
 std::multiset< Value > f_roots;

 };  // end( class CellHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/impl/CellHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CellHeap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CellHeap.h ---------------------------*/
/*--------------------------------------------------------------------------*/
