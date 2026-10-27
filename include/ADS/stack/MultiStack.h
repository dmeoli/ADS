/*--------------------------------------------------------------------------*/
/*--------------------------- File MultiStack.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * \f$ k \f$ STACKs in one structure [see `ADT.h`], beyond the book: the
 * items of all the stacks are cells of a single vector, each linked to the
 * cell below it in its stack, and the free cells form a further list, so
 * that the stacks share the space and none is full while the others have
 * room (the vector doubles when all the cells are used). `push()`, `pop()`
 * and `top()` on the stack \f$ i \f$ take \f$ O( 1 ) \f$ amortized time; the
 * stacks are numbered from 0. It generalizes the two stacks in one vector of
 * exercise 4.7.
 *
 * The contracts [see `Contract.h`]: \f$ k > 0 \f$, a stack \f$ i \f$ in
 * \f$ [ 0 , k ) \f$, and a stack that is not empty for `top()` and `pop()`;
 * the invariant is that each cell is in exactly one stack or in the free
 * list, and that the sizes count the cells of each stack. The operations
 * are defined in `MultiStack.tpp`.
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

/// \f$ k \f$ stacks sharing a vector of linked cells
template< class T >
class MultiStack {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// \f$ k \f$ empty stacks
 /// \pre \f$ k > 0 \f$
 explicit MultiStack( int k );

 /// the number \f$ k \f$ of stacks
 [[nodiscard]] int stacks( void ) const;

 /// true if the stack \f$ i \f$ has no item
 /// \pre \f$ 0 \le i < k \f$
 [[nodiscard]] bool empty( int i ) const;

 /// the number of items of the stack \f$ i \f$
 /// \pre \f$ 0 \le i < k \f$
 [[nodiscard]] int size( int i ) const;

 /// the item on top of the stack \f$ i \f$
 /// \pre \f$ 0 \le i < k \f$, the stack \f$ i \f$ is not empty
 [[nodiscard]] const T & top( int i ) const;

 /// \f$ v \f$ on the stack \f$ i \f$; \f$ v \f$ may be an item of the
 /// stacks, e.g., `top( j )`
 ///
 /// In pseudocode (exercise 4.7, with a free list as in Knuth, section
 /// 2.2.3), \f$ C \f$ the cells, \f$ \mathit{top}[ i ] \f$ the top cell of
 /// the stack \f$ i \f$, -1 for none:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( i , v ) \cr
 /// 2 & \quad \textbf{if}\ \mathit{free} = -1\ \textbf{then}\ c \gets
 ///     \text{a new cell at the end of } C\quad \triangleright
 ///     \ \text{doubling } C \cr
 /// 3 & \quad \textbf{else}\ c \gets \mathit{free};\ \mathit{free} \gets
 ///     C[ c ].\mathit{below} \cr
 /// 4 & \quad C[ c ] \gets ( v , \mathit{top}[ i ] );\ \mathit{top}[ i ]
 ///     \gets c
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le i < k \f$
 /// \post one item more in the stack \f$ i \f$
 void push( int i , const T & v );

 /// removes the item on top of the stack \f$ i \f$ and returns it
 ///
 /// In pseudocode (as `push()`), the cell given back to the free list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}( i ) \cr
 /// 2 & \quad c \gets \mathit{top}[ i ];\ v \gets C[ c ].\mathit{value} \cr
 /// 3 & \quad \mathit{top}[ i ] \gets C[ c ].\mathit{below};\ C[ c
 ///     ].\mathit{below} \gets \mathit{free};\ \mathit{free} \gets c \cr
 /// 4 & \quad \textbf{return}\ v
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le i < k \f$, the stack \f$ i \f$ is not empty
 /// \post one item less in the stack \f$ i \f$
 T pop( int i );

/*--------------------------------------------------------------------------*/
 private:

 /// a cell: an item and the cell below it
 struct Cell {
  T value;     ///< the item
  int below;   ///< the cell below in its stack, or the next free cell
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of each stack, from the top to the bottom, forward

 using value_type = T;   ///< the type of the items
 /// a constant iterator from a top down along the cells
 using const_iterator =
  IndexIterator< Cell , T , &Cell::below , &Cell::value >;

 /// the items of the stack \f$ i \f$, the top first
 /// \pre \f$ 0 \le i < k \f$
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
 std::vector< int > f_size;     ///< the number of items of each stack
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
