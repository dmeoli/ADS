/*--------------------------------------------------------------------------*/
/*------------------------- File IndexedDaryHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A \f$ D \f$-ary heap of the integers \f$ [ 0 , n ) \f$ with keys and
 * decrease-key, beyond the book: the heap of `DaryHeap` [see `DaryHeap.h`],
 * whose items are the pairs \f$ ( \mathit{key} , \mathit{id} ) \f$ with the
 * key inline, so that a comparison never leaves the heap, and a vector of
 * the places of the ids, which takes the place of the handles of
 * `HeapPriorityQueue` [see `HeapPriorityQueue.h`]. The children of the place
 * \f$ i \f$ are \f$ D i + 1 , \ldots , D i + D \f$ and its parent
 * \f$ \lfloor ( i - 1 ) / D \rfloor \f$, so that a heap of \f$ s \f$ items has
 * \f$ \lceil \log_D ( ( D - 1 ) s + 1 ) \rceil \f$ levels; `push()` and
 * `decrease()` move up at most that many levels, one comparison each, and
 * `pop()` moves down as many with \f$ D \f$ comparisons each:
 * \f[
 *   T_{\mathrm{push}} , T_{\mathrm{decrease}} = O( \log_D s ) , \qquad
 *   T_{\mathrm{pop}} = O( D \log_D s ) ,
 * \f]
 * which for Dijkstra's and Prim's algorithms, \f$ m \f$ decreases and
 * \f$ n \f$ pops, gives \f$ O( m \log_D n + n D \log_D n ) \f$, least near
 * \f$ D = m / n \f$ (Johnson). With \f$ D = 4 \f$ a `pop()` reads the four
 * children of a node in half a line of cache when the keys are 32-bit
 * integers, the choice that Larkin, Sen and Tarjan found the fastest in
 * practice; the space is \f$ ( 4 + k ) s + 4 n \f$ bytes, \f$ k \f$ the size
 * of a key. It is a PRIORITYQUEUE of section 10.1 [see `PriorityQueue.h`],
 * the ids its items and their own handles, so that the algorithms of the
 * book that take a priority queue run on it [see `ShortestPaths.h`,
 * `MinimumSpanningTree.h`].
 *
 * Beyond the PRIORITYQUEUE, the heap is addressable in full: `increase()`
 * raises the key of an id, moving it down as `pop()` moves the last item,
 * \f$ O( D \log_D s ) \f$, `update()` sets it either way, and `erase()`
 * removes an id from anywhere, the last item taking its place and moving
 * up or down from there, \f$ O( D \log_D s ) \f$; these are what a search
 * needs to take a node out of its frontier or to make it worse, e.g., when
 * a bound prunes it (see \ref theory_priority_heaps).
 *
 * The contracts [see `Contract.h`]: the ids are in \f$ [ 0 , n ) \f$,
 * `push()` needs an id not in the heap, `decrease()` one in the heap and a
 * key not greater than its own, `increase()` one in the heap and a key not
 * less than its own, `update()` and `erase()` one in the heap, `top()` and
 * `pop()` a heap that is not
 * empty; the invariant is the order of the heap, no key less than that of
 * the parent, with the places of the ids in their vector. The operations are
 * defined in `IndexedDaryHeap.tpp`.
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
 *         Università di Pisa \n
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

/// a \f$ D \f$-ary heap of the ids \f$ [ 0 , n ) \f$ by their keys, the
/// least on top, with decrease-key
template< class K , unsigned D = 4 >
class IndexedDaryHeap {

 static_assert( D >= 2 , "a heap needs at least two children per node" );

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;        ///< the type of the keys
 using Item = int;     ///< the items of PRIORITYQUEUE: the ids
 using Priority = K;   ///< and their priorities, the keys
 using Handle = int;   ///< what `insert()` gives: the id itself

 /// an item: an id with its key
 struct Entry {
  K key;    ///< the key
  int id;   ///< the id
  };

 /// the empty heap of the ids \f$ [ 0 , n ) \f$
 /// \pre \f$ n \ge 0 \f$
 explicit IndexedDaryHeap( int n = 0 );

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the room kept
 /// \pre \f$ n \ge 0 \f$
 void reset( int n );

 /// the number of ids, \f$ n \f$
 [[nodiscard]] int ids( void ) const { return( int( f_pos.size() ) ); }

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const { return( f_A.empty() ); }

 /// the number of items, \f$ s \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// whether the id is in the heap
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$
 [[nodiscard]] bool contains( int id ) const;

 /// the key of an id in the heap
 /// \pre the id is in the heap
 [[nodiscard]] const K & key( int id ) const;

 /// inserts the id with the key \f$ k \f$, in \f$ O( \log_D s ) \f$ time
 ///
 /// In pseudocode (the heap of section 10.1.3 with \f$ D \f$ children and
 /// the places of the ids, beyond the book), \f$ \mathit{pos} \f$ the place
 /// of each id:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( \mathit{id} , k ) \cr
 /// 2 & \quad s \gets s + 1;\ \mathrm{up}( s - 1 , ( k , \mathit{id} ) )
 ///     \cr
 /// 3 & \textbf{procedure}\ \mathrm{up}( i , x ) \cr
 /// 4 & \quad \textbf{while}\ i > 0\ \textbf{and}\ x.\mathit{key} < A[
 ///     \lfloor ( i - 1 ) / D \rfloor ].\mathit{key}\ \textbf{do} \cr
 /// 5 & \qquad A[ i ] \gets A[ \lfloor ( i - 1 ) / D \rfloor ];
 ///     \ \mathit{pos}[ A[ i ].\mathit{id} ] \gets i;\ i \gets \lfloor ( i
 ///     - 1 ) / D \rfloor \cr
 /// 6 & \quad A[ i ] \gets x;\ \mathit{pos}[ x.\mathit{id} ] \gets i
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$, not in the heap
 void push( int id , const K & k );

 /// lowers the key of the id to \f$ k \f$, in \f$ O( \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book), with \f$ \mathrm{up}() \f$ of
 /// `push()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( \mathit{id} , k ) \cr
 /// 2 & \quad \mathrm{up}( \mathit{pos}[ \mathit{id} ] , ( k , \mathit{id}
 ///     ) )
 /// \end{array}}
 /// \f]
 /// \pre the id is in the heap, \f$ k \f$ not greater than its key
 void decrease( int id , const K & k );

 /// raises the key of the id to \f$ k \f$, in \f$ O( D \log_D s ) \f$
 /// time
 ///
 /// In pseudocode (beyond the book), with \f$ \mathrm{down}() \f$ of
 /// `pop()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{increase}( \mathit{id} , k ) \cr
 /// 2 & \quad \mathrm{down}( \mathit{pos}[ \mathit{id} ] , ( k ,
 ///     \mathit{id} ) )
 /// \end{array}}
 /// \f]
 /// \pre the id is in the heap, \f$ k \f$ not less than its key
 void increase( int id , const K & k );

 /// sets the key of the id to \f$ k \f$, by `decrease()` or by
 /// `increase()`
 /// \pre the id is in the heap
 void update( int id , const K & k );

 /// removes the id from the heap, in \f$ O( D \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book): the last item takes the place of the
 /// id, and moves up or down from there, since it may be less than the
 /// parent of the place, if it came from another subtree, or greater than
 /// a child:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( \mathit{id} ) \cr
 /// 2 & \quad i \gets \mathit{pos}[ \mathit{id} ];\ \mathit{pos}[
 ///     \mathit{id} ] \gets -1;\ s \gets s - 1;\ x \gets A[ s ] \cr
 /// 3 & \quad \textbf{if}\ i = s\ \textbf{then return} \cr
 /// 4 & \quad \textbf{if}\ i > 0\ \textbf{and}\ x.\mathit{key} < A[
 ///     \lfloor ( i - 1 ) / D \rfloor ].\mathit{key}\ \textbf{then}
 ///     \ \mathrm{up}( i , x ) \cr
 /// 5 & \quad \textbf{else}\ \mathrm{down}( i , x )
 /// \end{array}}
 /// \f]
 /// \pre the id is in the heap
 void erase( int id );

 /// `push( id , k )` if the id is not in the heap, `decrease( id , k )` if
 /// \f$ k \f$ is less than its key, nothing otherwise; true if the key
 /// changed
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$
 bool pushOrDecrease( int id , const K & k );

 /// the item of least key
 /// \pre the heap is not empty
 [[nodiscard]] const Entry & top( void ) const;

 /// removes the item of least key and returns it, in
 /// \f$ O( D \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book), the last item moved down from the
 /// root:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad r \gets A[ 0 ];\ \mathit{pos}[ r.\mathit{id} ] \gets -1;
 ///     \ s \gets s - 1;\ x \gets A[ s ];\ i \gets 0 \cr
 /// 3 & \quad \textbf{if}\ s = 0\ \textbf{then return}\ r \cr
 /// 4 & \quad \textbf{while}\ D i + 1 < s\ \textbf{do} \cr
 /// 5 & \qquad b \gets \text{the child of } i \text{ of least key} \cr
 /// 6 & \qquad \textbf{if not}\ A[ b ].\mathit{key} < x.\mathit{key}
 ///     \ \textbf{then}\ \textbf{break} \cr
 /// 7 & \qquad A[ i ] \gets A[ b ];\ \mathit{pos}[ A[ i ].\mathit{id} ]
 ///     \gets i;\ i \gets b \cr
 /// 8 & \quad A[ i ] \gets x;\ \mathit{pos}[ x.\mathit{id} ] \gets i \cr
 /// 9 & \quad \textbf{return}\ r
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 Entry pop( void );

 /// removes all the items, in \f$ O( s ) \f$ time
 void clear( void );

 /// `push( id , k )`, the id its handle, as the `insert()` of PRIORITYQUEUE
 /// (section 10.1) [see `PriorityQueue.h`]
 /// \pre as `push()`
 Handle insert( int id , const K & k ) {
  push( id , k );
  return( id );
  }

 /// the id of least key, as the `min()` of PRIORITYQUEUE
 /// \pre the heap is not empty
 [[nodiscard]] int min( void ) const { return( top().id ); }

 /// `pop()`, the id of least key, as the `deleteMin()` of PRIORITYQUEUE
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
