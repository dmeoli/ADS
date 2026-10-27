/*--------------------------------------------------------------------------*/
/*------------------------- File IndexedDaryHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A d-ary heap of the integers [ 0 , n ) with keys and decrease-key, beyond
 * the book: the heap of DaryHeap [see DaryHeap.h], whose items are the
 * pairs ( key , id ) with the key inline, so that a comparison never leaves
 * the heap, and a vector of the places of the ids, which takes the place of
 * the handles of HeapPriorityQueue [see HeapPriorityQueue.h]. The children
 * of the place i are \f$ D i + 1 , \ldots , D i + D \f$ and its parent
 * \f$ \lfloor ( i - 1 ) / D \rfloor \f$, so that a heap of s items has
 * height \f$ \lceil \log_D ( ( D - 1 ) s + 1 ) \rceil \f$; push() and
 * decrease() move up at most that many levels, one comparison each, and
 * pop() moves down as many with D comparisons each:
 * \f[
 *   T_{\mathrm{push}} , T_{\mathrm{decrease}} = O( \log_D s ) , \qquad
 *   T_{\mathrm{pop}} = O( D \log_D s ) ,
 * \f]
 * which for Dijkstra's and Prim's algorithms, m decreases and n pops, gives
 * \f$ O( m \log_D n + n D \log_D n ) \f$, least near \f$ D = m / n \f$
 * (Johnson). With D = 4 a pop() reads the four children of a node in half
 * a line of cache when the keys are 32-bit integers, the choice that
 * Larkin, Sen and Tarjan found the fastest in practice; the space is
 * \f$ ( 4 + k ) s + 4 n \f$ bytes, k the size of a key. It is a
 * PRIORITYQUEUE of section 10.1 [see PriorityQueue.h], the ids its items
 * and their own handles, so that the algorithms of the book that take a
 * priority queue run on it [see ShortestPaths.h, MinimumSpanningTree.h].
 *
 * The contracts [see Contract.h]: the ids are in [ 0 , n ), push() needs
 * an id not in the heap, decrease() one in the heap and a key not greater
 * than its own, top() and pop() a heap that is not empty; the invariant is
 * the order of the heap, no key less than that of the parent, with the
 * places of the ids in their vector. The operations are defined in
 * IndexedDaryHeap.tpp.
 *
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0
 * \see D. H. Larkin, S. Sen, R. E. Tarjan, A back-to-basics empirical
 *      study of priority queues, Proceedings of ALENEX, 61-72, 2014,
 *      doi:10.1137/1.9781611973198.7
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_IndexedDaryHeap
 #define __ADS_IndexedDaryHeap
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS IndexedDaryHeap --------------------------*/
/*--------------------------------------------------------------------------*/

/// a D-ary heap of the ids [ 0 , n ) by their keys, the least on top, with
/// decrease-key
template< class K , unsigned D = 4 >
class IndexedDaryHeap {

 static_assert( D >= 2 , "a heap needs at least two children per node" );

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;        ///< the type of the keys
 using Item = int;     ///< the items of PRIORITYQUEUE: the ids
 using Priority = K;   ///< and their priorities, the keys
 using Handle = int;   ///< what insert() gives: the id itself

 /// an item: an id with its key
 struct Entry {
  K key;
  int id;
  };

 /// the empty heap of the ids [ 0 , n )
 /// \pre n >= 0
 explicit IndexedDaryHeap( int n = 0 );

 /// the empty heap of the ids [ 0 , n ), the room kept
 /// \pre n >= 0
 void reset( int n );

 /// the number of ids, n
 [[nodiscard]] int ids( void ) const { return( int( f_pos.size() ) ); }

 [[nodiscard]] bool empty( void ) const { return( f_A.empty() ); }

 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// whether the id is in the heap
 /// \pre id in [ 0 , n )
 [[nodiscard]] bool contains( int id ) const;

 /// the key of an id in the heap
 /// \pre id is in the heap
 [[nodiscard]] const K & key( int id ) const;

 /// inserts the id with the key k
 /// \pre id in [ 0 , n ), not in the heap
 void push( int id , const K & k );

 /// lowers the key of the id to k
 /// \pre id is in the heap, k not greater than its key
 void decrease( int id , const K & k );

 /// push( id , k ) if the id is not in the heap, decrease( id , k ) if k is
 /// less than its key, nothing otherwise; true if the key changed
 /// \pre id in [ 0 , n )
 bool pushOrDecrease( int id , const K & k );

 /// the item of least key
 /// \pre the heap is not empty
 [[nodiscard]] const Entry & top( void ) const;

 /// removes the item of least key and returns it
 /// \pre the heap is not empty
 Entry pop( void );

 /// removes all the items, in O( size() )
 void clear( void );

 /// PRIORITYQUEUE (section 10.1) [see PriorityQueue.h]: push( id , k ),
 /// the id its handle
 /// \pre as push()
 Handle insert( int id , const K & k ) {
  push( id , k );
  return( id );
  }

 /// PRIORITYQUEUE: the id of least key
 /// \pre the heap is not empty
 [[nodiscard]] int min( void ) const { return( top().id ); }

 /// PRIORITYQUEUE: pop(), the id of least key
 /// \pre the heap is not empty
 int deleteMin( void ) { return( pop().id ); }

/*--------------------------------------------------------------------------*/
 private:

 void up( std::size_t i , Entry x );

 void down( std::size_t i , Entry x );

 /// no key less than that of the parent, and the places of the ids right;
 /// on more than 1024 items, those of the root and of the last item only
 [[nodiscard]] bool ordered( void ) const;

 std::vector< Entry > f_A;   ///< the heap, the root in f_A[ 0 ]
 std::vector< int > f_pos;   ///< the place of each id, -1 if absent

 };  // end( class IndexedDaryHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/IndexedDaryHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IndexedDaryHeap.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File IndexedDaryHeap.h -----------------------*/
/*--------------------------------------------------------------------------*/
