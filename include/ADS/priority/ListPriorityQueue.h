/*--------------------------------------------------------------------------*/
/*------------------------ File ListPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a list.
 *
 * `ListPriorityQueue` is the lists of section 10.1.1: sorted, `min()` and
 * `deleteMin()` in \f$ O( 1 ) \f$, `insert()` and `decrease()` in
 * \f$ O( n ) \f$, since the place of an item is found by a scan from the
 * head; unsorted, the other way round, since an item goes to the head and
 * the minimum is found by a scan. The handle is the position of the cell.
 *
 * The contracts [see `Contract.h`]: `min()` and `deleteMin()` need a queue
 * that is not empty, `decrease()` a handle of an item of this queue (checked
 * in \f$ O( n ) \f$ with the invariants) and a priority not larger than its
 * own; after `deleteMin()` no item has a smaller priority than the one
 * removed; the invariant is that the list is sorted when it must be and that
 * `size()` counts its cells. The operations are defined in
 * `ListPriorityQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
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

#ifndef __ADS_ListPriorityQueue
 #define __ADS_ListPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS ListPriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a list, sorted by priority if `Sorted`, unsorted
/// otherwise
template< class T , std::totally_ordered P = int , bool Sorted = true >
class ListPriorityQueue {

 using Pair = std::pair< P , T >;

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;                                ///< the type of the items
 using Priority = P;                            ///< the type of priorities
 using Handle = typename List< Pair >::Pos;     ///< the cell of an item

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the item of smallest priority, in \f$ O( 1 ) \f$ time if sorted and
 /// \f$ O( n ) \f$ otherwise
 ///
 /// In pseudocode (section 10.1.1), \f$ L \f$ the list of the pairs
 /// \f$ ( p , x ) \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{min}() \cr
 /// 2 & \quad \textbf{if}\ \mathit{Sorted}\ \textbf{then return}
 ///     \ \text{the item of the head of } L \cr
 /// 3 & \quad \textbf{return}\ \text{the item of the first cell of } L
 ///     \text{ of least } p \text{, by a scan}
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// removes the item of smallest priority and returns it, in \f$ O( 1 ) \f$
 /// time if sorted and \f$ O( n ) \f$ otherwise
 ///
 /// In pseudocode (section 10.1.1):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad q \gets \text{the head of } L \cr
 /// 3 & \quad \textbf{if not}\ \mathit{Sorted}\ \textbf{then}\ q \gets
 ///     \text{the first cell of } L \text{ of least } p \cr
 /// 4 & \quad x \gets \text{the item of } q;\ \text{remove } q
 ///     \text{ from } L;\ n \gets n - 1 \cr
 /// 5 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less, none of smaller priority than the one removed
 T deleteMin( void );

 /// inserts \f$ x \f$ with the priority \f$ p \f$ and gives its handle, in
 /// \f$ O( n ) \f$ time if sorted and \f$ O( 1 ) \f$ otherwise
 ///
 /// In pseudocode (section 10.1.1), an item after those of equal priority:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad n \gets n + 1 \cr
 /// 3 & \quad \textbf{if}\ \mathit{Sorted}\ \textbf{then} \cr
 /// 4 & \qquad q \gets \text{the first cell of } L \text{ of priority}
 ///     \text{ larger than } p \text{, the end if none} \cr
 /// 5 & \qquad \textbf{return}\ \text{the cell of } ( p , x )
 ///     \text{ inserted before } q \cr
 /// 6 & \quad \textbf{return}\ \text{the cell of } ( p , x )
 ///     \text{ inserted at the head of } L
 /// \end{array}}
 /// \f]
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// lowers to \f$ p \f$ the priority of the item of the handle \f$ y \f$,
 /// which stays valid, in \f$ O( n ) \f$ time if sorted and \f$ O( 1 ) \f$
 /// otherwise
 ///
 /// In pseudocode (section 10.1.1):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( y , p ) \cr
 /// 2 & \quad \textbf{if}\ \mathit{Sorted}\ \textbf{then} \cr
 /// 3 & \qquad x \gets \text{the item of } y;\ \text{remove } y
 ///     \text{ from } L \cr
 /// 4 & \qquad y \gets \mathrm{insert}( x , p )\quad \triangleright
 ///     \ \text{without counting it again} \cr
 /// 5 & \quad \textbf{else}\ \text{the priority of } y \gets p
 /// \end{array}}
 /// \f]
 /// \pre \f$ y \f$ is an item of this queue and \f$ p \f$ is not larger
 ///      than its priority
 /// \post \f$ y \f$ has the priority \f$ p \f$
 void decrease( Handle & y , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( priority , item ), by priority when the
 // list is sorted, bidirectional; a change through it would break the order

 /// the type of the items of the view
 using value_type = Pair;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between iterators
 using difference_type = std::ptrdiff_t;
 /// the type of a reference to an item of the view
 using reference = const Pair &;
 /// the type of a constant reference to an item
 using const_reference = const Pair &;
 /// the same as `const_iterator`
 using iterator = typename List< Pair >::const_iterator;
 /// the iterator of the view, constant
 using const_iterator = typename List< Pair >::const_iterator;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the cell of smallest priority
 [[nodiscard]] Handle first( void ) const;

 /// the position of the first item of priority larger than p
 [[nodiscard]] Handle after( const P & p ) const;

 /// whether y is a cell of the list, \f$ O( n ) \f$
 [[nodiscard]] bool owns( Handle y ) const;

 /// sorted if it must be, and size() its length, \f$ O( n ) \f$
 [[nodiscard]] bool consistent( void ) const;

 List< Pair > f_L;   ///< the items
 int f_size = 0;     ///< their number

 };  // end( class ListPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/ListPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ListPriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
