/*--------------------------------------------------------------------------*/
/*------------------------ File TreePriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a tree.
 *
 * `TreePriorityQueue` is a Red-Black tree of the pairs
 * \f$ ( p , i ) \f$ of a priority and of a number of insertion (section
 * 10.1.2), the number making the keys distinct: the minimum is the leftmost
 * node, and every operation takes \f$ O( \log n ) \f$ time, since the
 * height of the tree is \f$ O( \log n ) \f$ [see `SearchTree.h`]. The
 * handle of an item is its number of insertion, and a vector of the
 * priorities by number gives the key under which `decrease()` finds it.
 *
 * The contracts [see `Contract.h`]: `min()` and `deleteMin()` need a queue
 * that is not empty, `decrease()` the handle of an item still in the queue
 * and a priority not larger than its own; the invariant is that each item
 * of the tree has the priority recorded for its handle. The operations are
 * defined in `TreePriorityQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1.2,
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

#ifndef __ADS_TreePriorityQueue
 #define __ADS_TreePriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/tree/SearchTree.h"

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS TreePriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a Red-Black tree of the pairs of a priority and of a
/// number of insertion
template< class T , std::totally_ordered P = int >
class TreePriorityQueue {

 using Key = std::pair< P , long >;   ///< priority, number of insertion

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;        ///< the type of the items
 using Priority = P;    ///< the type of the priorities
 using Handle = long;   ///< the number of insertion of an item

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the item of smallest priority, the leftmost node of the tree, in
 /// \f$ O( \log n ) \f$ time
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// removes the item of smallest priority and returns it, in
 /// \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (section 10.1.2), \f$ T \f$ the tree of the keys
 /// \f$ ( p , i ) \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad ( k , x ) \gets \text{the leftmost node of } T \cr
 /// 3 & \quad \mathrm{remove}( T , k ) \cr
 /// 4 & \quad \textbf{return}\ x
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less
 T deleteMin( void );

 /// inserts \f$ x \f$ with the priority \f$ p \f$ and gives its handle, in
 /// \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (section 10.1.2), \f$ \mathit{pr} \f$ the vector of the
 /// priorities by number of insertion:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad i \gets |\mathit{pr}|;\ \mathrm{insert}( T , ( p , i ) , x
 ///     ) \cr
 /// 3 & \quad \text{append } p \text{ to } \mathit{pr} \cr
 /// 4 & \quad \textbf{return}\ i
 /// \end{array}}
 /// \f]
 /// \post one item more
 Handle insert( const T & x , const P & p );

 /// lowers to \f$ p \f$ the priority of the item of the handle \f$ y \f$,
 /// in \f$ O( \log n ) \f$ time
 ///
 /// In pseudocode (section 10.1.2), the item moved to its new key:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( y , p ) \cr
 /// 2 & \quad x \gets \mathrm{lookup}( T , ( \mathit{pr}[ y ] , y ) );
 ///     \ \mathrm{remove}( T , ( \mathit{pr}[ y ] , y ) ) \cr
 /// 3 & \quad \mathrm{insert}( T , ( p , y ) , x );\ \mathit{pr}[ y ]
 ///     \gets p
 /// \end{array}}
 /// \f]
 /// \pre \f$ y \f$ is an item still in the queue and \f$ p \f$ is not
 ///      larger than its priority
 void decrease( Handle y , const P & p );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( ( priority , number of insertion ) ,
 // item ) in increasing order of priority, bidirectional, those of the tree

 /// the type of the items of the view
 using value_type = typename RedBlackTree< Key , T >::value_type;
 /// the iterator of the view, constant
 using const_iterator = typename RedBlackTree< Key , T >::const_iterator;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const { return( f_t.begin() ); }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const { return( f_t.end() ); }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// each item of the tree with the priority of its handle, \f$ O( n ) \f$
 [[nodiscard]] bool consistent( void ) const;

 RedBlackTree< Key , T > f_t;   ///< the items by priority
 std::vector< P > f_p;          ///< the priority of each insertion

 };  // end( class TreePriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/TreePriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TreePriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File TreePriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
