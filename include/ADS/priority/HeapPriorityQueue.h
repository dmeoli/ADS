/*--------------------------------------------------------------------------*/
/*------------------------ File HeapPriorityQueue.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The PRIORITYQUEUE [see PriorityQueue.h] on a heap.
 *
 * HeapPriorityQueue is the heap of section 10.1.3, a vector H of capacity
 * n holding a d-ary tree that is full but for the rightmost leaves of
 * the last level, each node not smaller than its parent: the children of
 * the place i are \f$ d i + 1 , \ldots , d i + d \f$ and its parent is
 * \f$ \lfloor ( i - 1 ) / d \rfloor \f$ (with d = 2, l( i ) = 2 i + 1,
 * r( i ) = 2 i + 2, p( i ) = \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$).
 * insert() and decrease() move the item up, deleteMin() moves the last
 * leaf down from the root [see minHeapRestore()], in
 * \f$ O( \log_d n ) \f$ and \f$ O( d \log_d n ) \f$ time; each item knows
 * its place, kept by swap(), and so the handle finds it (exercise 10.6
 * for d > 2). Beyond the book, the items come from an ObjectPool [see
 * ObjectPool.h] of the queue, which takes back the item of deleteMin() for
 * the next insert(), so that an item costs no call to the allocator and
 * the items in use stay close in memory.
 *
 * The contracts [see Contract.h]: min(), minPriority() and deleteMin() need a
 * queue that is not empty, insert() one that is not full, and decrease() a
 * handle of an item of this queue and a priority not larger than its own; the
 * invariant, checked at level 3 in \f$ O( n ) \f$ time, is the order of the
 * heap together with the places that the items remember. On a heap of more
 * than 1024 items only the root and the last item are checked, so that a long
 * sequence of operations at the level 3 stays linear. The operations are
 * defined in HeapPriorityQueue.tpp.
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

/// a priority queue on a d-ary min-heap of bounded capacity
template< class T , std::totally_ordered P = int , int D = 2 >
 requires( D >= 2 )
class HeapPriorityQueue {

/*--------------------------------------------------------------------------*/
 public:

 /// the PRIORITYITEM of the book: priority, item, place in the vector
 struct PriorityItem {
  P priority;
  T value;
  int pos;
  };

 using Item = T;                  ///< the type of the items
 using Priority = P;              ///< the type of the priorities
 using Handle = PriorityItem *;   ///< what insert() gives

 /// PriorityQueue( n ): the empty queue of capacity n
 /// \pre n >= 0
 explicit HeapPriorityQueue( int n );

 HeapPriorityQueue( const HeapPriorityQueue & ) = delete;
 HeapPriorityQueue & operator=( const HeapPriorityQueue & ) = delete;

 /// the items of other, which is left empty; its handles stay valid here
 HeapPriorityQueue( HeapPriorityQueue && other ) noexcept;

 HeapPriorityQueue & operator=( HeapPriorityQueue && other ) noexcept;

 ~HeapPriorityQueue();

 [[nodiscard]] bool empty( void ) const;

 [[nodiscard]] int size( void ) const;

 [[nodiscard]] int capacity( void ) const;

 /// the item of smallest priority
 /// \pre the queue is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the smallest priority
 /// \pre the queue is not empty
 [[nodiscard]] const P & minPriority( void ) const;

 /// the last leaf goes to the root and down, \f$ O( d \log_d n ) \f$ time; the
 /// handle of the item removed is no longer valid
 /// \pre the queue is not empty
 /// \post one item less, and none of smaller priority than the one removed
 T deleteMin( void );

 /// a new leaf goes up, \f$ O( \log_d n ) \f$ time
 /// \pre the queue is not full
 /// \post one item more, the handle at its place
 Handle insert( const T & x , const P & p );

 /// lowers the priority of y to p
 /// \pre y is an item of this queue and p is not larger than its priority
 /// \post y has priority p, at its place
 void decrease( Handle y , const P & p );

 /// the heap: the priorities, by their place
 [[nodiscard]] std::vector< P > priorities( void ) const;

 /// the parent of the place i > 0
 static int parent( int i ) { return( ( i - 1 ) / D ); }

 /// the j-th child, 0 <= j < d, of the place i
 static int child( int i , int j ) { return( D * i + j + 1 ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items with their priorities, PriorityItem, in
 // the order of the heap (the minimum first, the rest in no useful order),
 // random access; a change through it would break the heap

 using value_type = PriorityItem;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const PriorityItem &;
 using const_reference = const PriorityItem &;
 using const_iterator = IndirectIterator<
  typename std::vector< PriorityItem * >::const_iterator >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_H.begin() ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_H.end() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
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
