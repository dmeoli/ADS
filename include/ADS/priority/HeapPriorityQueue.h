/*--------------------------------------------------------------------------*/
/*------------------------ File HeapPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see `PriorityQueue.h`] on a heap.
 *
 * `HeapPriorityQueue` is the heap of section 10.1.3, a vector
 * \f$ H[ 0 .. n - 1 ] \f$ of capacity \f$ c \f$ holding a \f$ d \f$-ary tree
 * that is full but for the rightmost leaves of the last level, each node not
 * smaller than its parent: the children of the place \f$ i \f$ are
 * \f$ d i + 1 , \ldots , d i + d \f$ and its parent is
 * \f$ \lfloor ( i - 1 ) / d \rfloor \f$ (with \f$ d = 2 \f$,
 * \f$ l( i ) = 2 i + 1 \f$, \f$ r( i ) = 2 i + 2 \f$ and
 * \f$ p( i ) = \lfloor ( i - 1 ) / 2 \rfloor \f$). `insert()` and
 * `decrease()` move the item up, `deleteMin()` moves the last leaf down from
 * the root [see `minHeapRestore()`], in \f$ O( \log_d n ) \f$ and
 * \f$ O( d \log_d n ) \f$ time, the tree having
 * \f$ \lceil \log_d ( ( d - 1 ) n + 1 ) \rceil \f$ levels; each item knows its
 * place, kept by `swap()`, and so the handle finds it (exercise 10.6 for
 * \f$ d > 2 \f$). Beyond the book, the items come from an `ObjectPool` [see
 * `ObjectPool.h`] of the queue, which takes back the item of `deleteMin()`
 * for the next `insert()`, so that an item costs no call to the allocator
 * and the items in use stay close in memory.
 *
 * The contracts [see `Contract.h`]: `min()`, `minPriority()` and
 * `deleteMin()` need a queue that is not empty, `insert()` one that is not
 * full, and `decrease()` a handle of an item of this queue and a priority not
 * larger than its own; the invariant, checked at level 3 in \f$ O( n ) \f$
 * time, is the order of the heap together with the places that the items
 * remember. On a heap of more than 1024 items only the root and the last item
 * are checked, so that a long sequence of operations at the level 3 stays
 * linear. The operations are defined in `HeapPriorityQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see J. W. J. Williams, Algorithm 232: Heapsort, Communications of the
 *      ACM 7(6), 347-348, 1964, doi:10.1145/512274.512284
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0
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

#ifndef __ADS_HeapPriorityQueue
 #define __ADS_HeapPriorityQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/priority/PriorityQueue.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS HeapPriorityQueue -------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority queue on a \f$ d \f$-ary min-heap of bounded capacity,
/// \f$ d \f$ the parameter `D`
template< class T , std::totally_ordered P = int , int D = 2 >
 requires( D >= 2 )
class HeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 /// the PRIORITYITEM of the book: priority, item, place in the vector
 struct PriorityItem {
  P priority;   ///< the priority
  T value;      ///< the item
  int pos;      ///< the place in the vector of the heap
  };

 using Item = T;                  ///< the type of the items
 using Priority = P;              ///< the type of the priorities
 using Handle = PriorityItem *;   ///< what `insert()` gives

 /// the empty queue of capacity \f$ n \f$ (the `PriorityQueue( n )` of the
 /// book)
 /// \pre \f$ n \ge 0 \f$
 explicit HeapPriorityQueue( int n );

 /// no copy: the handles point into the queue
 HeapPriorityQueue( const HeapPriorityQueue & ) = delete;

 /// no copy: the handles point into the queue
 HeapPriorityQueue & operator=( const HeapPriorityQueue & ) = delete;

 /// the items of `other`, which is left empty; its handles stay valid here
 HeapPriorityQueue( HeapPriorityQueue && other ) noexcept;

 /// the items of `other`, which is left empty, in place of these; its
 /// handles stay valid here
 HeapPriorityQueue & operator=( HeapPriorityQueue && other ) noexcept;

 /// the queue and its items destroyed
 ~HeapPriorityQueue();

 /// whether the queue has no item
 [[nodiscard]] bool empty( void ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] int size( void ) const;

 /// the largest number of items, \f$ c \f$
 [[nodiscard]] int capacity( void ) const;

 /// the item of smallest priority, the root \f$ H[ 0 ] \f$
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the smallest priority
 /// \pre the queue is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// removes the item of smallest priority and returns it, the last leaf
 /// moved to the root and down, in \f$ O( d \log_d n ) \f$ time; the handle
 /// of the item removed is no longer valid
 ///
 /// In pseudocode (section 10.1.3), on the places
 /// \f$ H[ 0 .. n - 1 ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{deleteMin}() \cr
 /// 2 & \quad x \gets H[ 0 ];\ n \gets n - 1 \cr
 /// 3 & \quad \mathrm{swap}( H , 0 , n );\ \mathrm{minHeapRestore}( H ,
 ///     0 , n ) \cr
 /// 4 & \quad \textbf{return}\ x \cr
 /// 5 & \textbf{procedure}\ \mathrm{minHeapRestore}( H , i , \mathit{dim}
 ///     ) \cr
 /// 6 & \quad \mathit{min} \gets i \cr
 /// 7 & \quad \textbf{for}\ j \gets 1\ \textbf{to}\ d\ \textbf{do} \cr
 /// 8 & \qquad c \gets d i + j \cr
 /// 9 & \qquad \textbf{if}\ c < \mathit{dim}\ \textbf{and}\ H[ c
 ///     ].\mathit{priority} < H[ \mathit{min} ].\mathit{priority}
 ///     \ \textbf{then}\ \mathit{min} \gets c \cr
 /// 10 & \quad \textbf{if}\ i \ne \mathit{min}\ \textbf{then} \cr
 /// 11 & \qquad \mathrm{swap}( H , i , \mathit{min} );
 ///     \ \mathrm{minHeapRestore}( H , \mathit{min} , \mathit{dim} )
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less, and none of smaller priority than the one removed
 T deleteMin( void );

 /// inserts \f$ x \f$ with the priority \f$ p \f$ and gives its handle, a
 /// new leaf moved up, in \f$ O( \log_d n ) \f$ time
 ///
 /// In pseudocode (section 10.1.3), \f$ \mathrm{parent}( i ) \f$ the parent
 /// of the place \f$ i \f$ and \f$ \mathrm{swap}() \f$ keeping the place of
 /// each item:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , p ) \cr
 /// 2 & \quad i \gets n;\ n \gets n + 1 \cr
 /// 3 & \quad H[ i ] \gets \text{a new item } ( p , x , i ) \cr
 /// 4 & \quad \textbf{while}\ i > 0\ \textbf{and}\ H[ i ].\mathit{priority}
 ///     < H[ \mathrm{parent}( i ) ].\mathit{priority}\ \textbf{do} \cr
 /// 5 & \qquad \mathrm{swap}( H , i , \mathrm{parent}( i ) );\ i \gets
 ///     \mathrm{parent}( i ) \cr
 /// 6 & \quad \textbf{return}\ \text{the new item}
 /// \end{array}}
 /// \f]
 /// \pre the queue is not full
 /// \post one item more, the handle at its place
 Handle insert( const T & x , const P & p );

 /// lowers to \f$ p \f$ the priority of the item of the handle \f$ y \f$,
 /// which moves up, in \f$ O( \log_d n ) \f$ time
 ///
 /// In pseudocode (section 10.1.3):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( y , p ) \cr
 /// 2 & \quad y.\mathit{priority} \gets p;\ i \gets y.\mathit{pos} \cr
 /// 3 & \quad \textbf{while}\ i > 0\ \textbf{and}\ H[ i ].\mathit{priority}
 ///     < H[ \mathrm{parent}( i ) ].\mathit{priority}\ \textbf{do} \cr
 /// 4 & \qquad \mathrm{swap}( H , i , \mathrm{parent}( i ) );\ i \gets
 ///     \mathrm{parent}( i )
 /// \end{array}}
 /// \f]
 /// \pre \f$ y \f$ is an item of this queue and \f$ p \f$ is not larger
 ///      than its priority
 /// \post \f$ y \f$ has the priority \f$ p \f$, at its place
 void decrease( Handle y , const P & p );

 /// the heap: the priorities, by their place
 [[nodiscard]] std::vector< P > priorities( void ) const;

 /// the parent \f$ \lfloor ( i - 1 ) / d \rfloor \f$ of the place
 /// \f$ i > 0 \f$
 static int parent( int i ) { return( ( i - 1 ) / D ); }

 /// the child \f$ d i + j + 1 \f$ of the place \f$ i \f$,
 /// \f$ 0 \le j < d \f$
 static int child( int i , int j ) { return( D * i + j + 1 ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items with their priorities, PriorityItem, in
 // the order of the heap (the minimum first, the rest in no useful order),
 // random access; a change through it would break the heap

 /// the type of the items of the view
 using value_type = PriorityItem;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between iterators
 using difference_type = std::ptrdiff_t;
 /// the type of a reference to an item of the view
 using reference = const PriorityItem &;
 /// the type of a constant reference to an item
 using const_reference = const PriorityItem &;
 /// the iterator of the view, constant
 using const_iterator = IndirectIterator<
  typename std::vector< PriorityItem * >::const_iterator >;
 /// the same as `const_iterator`
 using iterator = const_iterator;

 /// the first item of the view
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_H.begin() ) );
  }
 /// the end of the view
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_H.end() ) );
  }
 /// the same as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the same as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// swaps the places i and j, keeping the positions of the items
 void swap( int i , int j );

 void up( int i );

 /// the subtrees of the children of i are heaps in [ 0 , dim ): the item in
 /// i goes down, swapped with its smallest child
 void minHeapRestore( int i , int dim );

 /// y is the item in its own place
 [[nodiscard]] bool owns( Handle y ) const;

 /// no item is smaller than its parent, and each knows its place,
 /// \f$ O( n ) \f$; on more than 1024 items, the root and the last item only
 [[nodiscard]] bool isHeap( void ) const;

 int f_capacity;                       ///< the capacity
 std::vector< PriorityItem * > f_H;    ///< the heap
 ObjectPool< PriorityItem > f_pool;    ///< the items

 };  // end( class HeapPriorityQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/HeapPriorityQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* HeapPriorityQueue.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File HeapPriorityQueue.h ----------------------*/
/*--------------------------------------------------------------------------*/
