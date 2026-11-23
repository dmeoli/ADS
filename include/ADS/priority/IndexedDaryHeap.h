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
 * a bound prunes it (see \ref theory_priority_heaps; its events are in
 * \ref theory_priority_events).
 *
 * The range of the ids is set by the constructor and by `reset( n )`, which
 * empties the heap, and raised by `grow( n )`, which keeps it, in
 * \f$ O( 1 ) \f$ amortized time per new id.
 *
 * Two operations serve the searches that take or recompute many keys at
 * once: `extract( theta , out )` removes every id of key at most
 * \f$ \theta \f$ (the stepping algorithms), and `rekey( f )` sets the key of
 * every id to \f$ f( \mathit{id} ) \f$ and makes the order again from the
 * bottom in \f$ O( s ) \f$ (a change of the function of the keys, as the
 * \f$ \epsilon \f$ of AD*). `TableHeap`, below, keeps the keys in the
 * table of a search and reads them from it.
 *
 * The heap tells its changes through the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, with which every operation
 * compiles to the code of the plain heap: `Inserted` by `push()`, `Erased` by
 * `erase()` and `pop()` (the id and its key, before the removal), `KeyChanged`
 * by `decrease()`, `increase()` and `update()` (the old and the new key),
 * `Cleared` after `clear()` and `reset()`, `Rebuilt` after `rekey()` and at
 * the end of a mute, and `Relocated` on demand, only if an observer wants it,
 * for each step of the item that an operation moves: the item from the place
 * `from` to the place `to` and the item at `to`, if any, to `from`, an
 * exchange, so that an observer that applies them in their order to a copy of
 * the vector of the heap keeps it equal to the vector. The event of the item
 * comes first (the new item at the place \f$ s \f$, the last one, before its
 * moves; the key changed in its place; the place of the item erased left
 * empty, the last item moved into it by the first `Relocated`). The ids are
 * the handles of the events, stable while an id is in the heap, and the places
 * of the vector, which are internal maintenance, travel only on `Relocated`. A
 * heap of keys has no shift of all its keys in \f$ O( 1 ) \f$, hence no
 * `Shifted`.
 *
 * The vectors come from the allocator `A`, `std::allocator` by default,
 * given to the constructor that takes `std::allocator_arg` first, with the
 * rules of the standard [see `AllocatorAware.h`].
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
#include "ADS/Modification.h"
#include "ADS/memory/AllocatorAware.h"
#include "ADS/Portable.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS IndexedDaryHeap --------------------------*/
/*--------------------------------------------------------------------------*/

/// a \f$ D \f$-ary heap of the ids \f$ [ 0 , n ) \f$ by their keys, the
/// least on top, with decrease-key, its changes told through `Policy`
template< class K , unsigned D = 4 , class Policy = NoObservers ,
	  class A = std::allocator< K > >
class IndexedDaryHeap
{

 static_assert( D >= 2 , "a heap needs at least two children per node" );

/*--------------------------------------------------------------------------*/
 public:
 using Key = K;       ///< the type of the keys
 using Item = int;    ///< the items of PRIORITYQUEUE: the ids
 using Priority = K;  ///< and their priorities, the keys
 using Handle = int;  ///< what `insert()` gives: the id itself

 /// an item: an id with its key
 struct Entry
 {
  K key;   ///< the key
  int id;  ///< the id
  };

 /// the events: an id inserted or erased with its key, its key changed,
 /// an item moved from a place to another, the heap emptied or rebuilt
 using modifications =
   mod::list< mod::Inserted< int , K > , mod::Erased< int , K > ,
	      mod::KeyChanged< int , K > , mod::Relocated< std::size_t > ,
	      mod::Cleared , mod::Rebuilt >;
 /// the subject
 using Subject =
   typename Policy::template bind< modifications , IndexedDaryHeap >;

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the static observers
 /// `o...`
 /// \pre \f$ n \ge 0 \f$
 template< class... O >
 explicit IndexedDaryHeap( int n = 0 , O &... o );

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the static observers
 /// `o...`, its vectors from `alloc`
 /// \pre \f$ n \ge 0 \f$
 template< class... O >
 IndexedDaryHeap( std::allocator_arg_t , const A & alloc , int n = 0 ,
		  O &... o );

 /// the type of the allocator
 using allocator_type = A;
 /// the allocator of the vectors
 [[nodiscard]] allocator_type get_allocator( void ) const {
  return( allocator_type( f_A.get_allocator() ) );
  }

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the room kept
 /// \pre \f$ n \ge 0 \f$
 void reset( int n );

 /// the ids raised to \f$ [ 0 , n ) \f$, the new ones not in the heap and
 /// the items kept, in \f$ O( 1 ) \f$ amortized time per new id: the
 /// vector of the places grows at least twice as large when it is full, so
 /// that a consumer whose ids appear one by one (the nodes of a search
 /// numbered as they are generated) grows the heap with them
 /// \pre \f$ n \ge \f$ `ids()`
 void grow( int n );

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

 /// every id of key at most \f$ \theta \f$ appended to `out`, by
 /// increasing keys, each removed as by `pop()`, in
 /// \f$ O( ( k + 1 ) D \log_D s ) \f$ time for \f$ k \f$ ids
 void extract( const K & theta , std::vector< int > & out );

 /// the key of each id in the heap set to `f( id )`, and the order of the
 /// heap made again from the bottom, in \f$ O( s ) \f$ time; `Rebuilt` to
 /// the observers, since any key and any place may change
 ///
 /// In pseudocode (the construction of Floyd on \f$ D \f$ children, beyond
 /// the book), with \f$ \mathrm{down}() \f$ of `pop()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rekey}( f ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ s - 1\ \textbf{do}
 ///     \ A[ i ].\mathit{key} \gets f( A[ i ].\mathit{id} ) \cr
 /// 3 & \quad \textbf{for}\ i \gets \lfloor ( s - 2 ) / D \rfloor
 ///     \ \textbf{downto}\ 0\ \textbf{do}\ \mathrm{down}( i , A[ i ] )
 /// \end{array}}
 /// \f]
 template< class F >
 void rekey( F && f );

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

/*--------------------------- THE OBSERVERS --------------------------------*/

 /// the subject, for `ADS::batch()` and `ADS::quiet()`
 [[nodiscard]] Subject & subject( void ) { return( f_sub ); }
 /// the subject
 [[nodiscard]] const Subject & subject( void ) const { return( f_sub ); }

 /// attaches the observer \f$ o \f$ and gives its number
 /// \pre no batch is open
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic )
 {
  return( f_sub.attach( o ) );
  }

 /// detaches the observer number \f$ \mathit{id} \f$
 /// \pre it is attached, no batch is open
 void detach( int id )
  requires( Subject::dynamic )
 {
  f_sub.detach( id );
  }

/*--------------------------------------------------------------------------*/
 private:
 using Ins = mod::Inserted< int , K >;        ///< an insertion
 using Era = mod::Erased< int , K >;          ///< an erasure
 using Chg = mod::KeyChanged< int , K >;      ///< a change of key
 using Rel = mod::Relocated< std::size_t >;  ///< a move of an item

 /// whether someone may want one of the events `E...`, at compile time
 template< class... E >
 static constexpr bool heard = ( Subject::template listened< E > || ... );

 /// whether someone wants one of the events `E...` now
 template< class... E >
 [[nodiscard]] bool alive( void ) const {
  return( ( f_sub.template live< E >() || ... ) );
  }

 /// \f$ x \f$ moved up from the place \f$ i \f$; with `T`, `Relocated`
 /// from the place `from` of \f$ x \f$ to \f$ i \f$, if they differ, and
 /// for each exchange of \f$ x \f$ with its parent
 template< bool T = false >
 void up( std::size_t i , Entry x , std::size_t from = npos );

 /// \f$ x \f$ moved down from the place \f$ i \f$; with `T`, as `up()`
 template< bool T = false >
 void down( std::size_t i , Entry x , std::size_t from = npos );

 /// the change of an operation when someone listens
 void pushTold( int id , const K & k );
 void decreaseTold( int id , const K & k );  ///< as pushTold()
 void increaseTold( int id , const K & k );  ///< as pushTold()
 void eraseTold( int id );                  ///< as pushTold()
 Entry popTold( void );                     ///< as pushTold()

 /// the changes when someone listens, out of the hot code
 ADS_COLD_PATH void pushCold( int id , const K & k ) { pushTold( id , k ); }
 ADS_COLD_PATH void decreaseCold( int id , const K & k ) {
  decreaseTold( id , k );
  }
 ADS_COLD_PATH void increaseCold( int id , const K & k ) {
  increaseTold( id , k );
  }
 ADS_COLD_PATH void eraseCold( int id ) { eraseTold( id ); }
 ADS_COLD_PATH Entry popCold( void ) { return( popTold() ); }

 /// no place
 static constexpr std::size_t npos = std::size_t( -1 );

 /// no key less than that of the parent, and the places of the ids right;
 /// on more than 1024 items, those of the root and of the last item only
 [[nodiscard]] bool ordered( void ) const;

 detail::AllocVector< A , Entry > f_A;  ///< the heap, the root in f_A[ 0 ]
 detail::AllocVector< A , int > f_pos;  ///< the place of each id, -1 if none
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class IndexedDaryHeap )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS TableHeap -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the ids \f$ [ 0 , n ) \f$ in a \f$ D \f$-ary heap by the keys that the
/// table `T` holds for them, read from it by `update( id )`: the queue of
/// the frontier as an observer of the table of a search, in the form of
/// the LaB-PQ of Dong et al.
///
/// The table is anything that `t[ id ]` reads and `t.size()` measures (a
/// `std::vector`, an `ObservableArray` [see `ObservableArray.h`]); the queue
/// copies the key of an id when it reads it, and `update( id )` makes the id
/// enter the queue with its present key, or move to it, in
/// \f$ O( D \log_D s ) \f$ time, so that a search that writes the table
/// calls `update()` on the ids it wrote, or attaches the queue to an
/// observable table, whose `Assigned` and `Inserted` call it, `Erased`
/// takes the id out, `Cleared` empties the queue and `Rebuilt` reads the
/// keys of all its ids again. `extract( theta , out )` takes every id of key
/// at most \f$ \theta \f$, as the stepping algorithms do. Unlike the
/// tournament tree of the paper, the queue updates an id at once instead of
/// marking it for the next extraction, which costs a logarithm per update
/// and gives the same ids to each `extract()`.
///
/// \see X. Dong, Y. Gu, Y. Sun, Y. Zhang, Efficient stepping algorithms and
///      implementations for parallel shortest paths, Proceedings of SPAA,
///      184-197, 2021, doi:10.1145/3409964.3461782
template< class T , unsigned D = 4 >
class TableHeap
{

/*--------------------------------------------------------------------------*/
 public:
 /// the type of the keys, those of the table
 using Key = std::remove_cvref_t<
   decltype( std::declval< const T & >()[ std::size_t() ] ) >;
 using Item = int;      ///< the items: the ids
 using Priority = Key;  ///< and their priorities, the keys
 using Handle = int;    ///< the id itself

 /// the empty queue of the ids \f$ [ 0 , n ) \f$ of the table \f$ t \f$,
 /// which must outlive it
 /// \pre \f$ n \ge 0 \f$
 explicit TableHeap( const T & t , int n = 0 ) : f_t( &t ) , f_h( n ) { }

 /// the id in the queue with its key in the table, inserted or moved, the
 /// ids raised to \f$ \mathit{id} + 1 \f$ if fewer
 /// \pre \f$ 0 \le \mathit{id} < \f$ `t.size()`
 void update( int id ) {
  ADS_PRE( ( id >= 0 ) && ( std::size_t( id ) < f_t->size() ) );
  if( id >= f_h.ids() )
   f_h.grow( id + 1 );
  const Key k = ( *f_t )[ std::size_t( id ) ];
  if( f_h.contains( id ) )
   f_h.update( id , k );
  else
   f_h.push( id , k );
  }

 /// the id out of the queue, if it is there
 void erase( int id ) {
  if( contains( id ) )
   f_h.erase( id );
  }

 /// whether the id is in the queue
 [[nodiscard]] bool contains( int id ) const {
  return( ( id >= 0 ) && ( id < f_h.ids() ) && f_h.contains( id ) );
  }

 /// whether the queue is empty
 [[nodiscard]] bool empty( void ) const { return( f_h.empty() ); }

 /// the number of ids in the queue
 [[nodiscard]] std::size_t size( void ) const { return( f_h.size() ); }

 /// the key of an id in the queue, as read last from the table
 /// \pre the id is in the queue
 [[nodiscard]] const Key & key( int id ) const { return( f_h.key( id ) ); }

 /// the id of least key
 /// \pre the queue is not empty
 [[nodiscard]] int min( void ) const { return( f_h.min() ); }

 /// removes the id of least key and returns it
 /// \pre the queue is not empty
 int deleteMin( void ) { return( f_h.deleteMin() ); }

 /// every id of key at most \f$ \theta \f$ appended to `out`, by
 /// increasing keys, each removed
 void extract( const Key & theta , std::vector< int > & out ) {
  f_h.extract( theta , out );
  }

 /// the queue emptied
 void clear( void ) { f_h.clear(); }

 /// the ids that the table no longer has taken out, and the keys of the
 /// others read again, in \f$ O( s ) \f$ time
 void refresh( void ) {
  std::vector< int > gone;
  for( int id = int( f_t->size() ) ; id < f_h.ids() ; ++id )
   if( f_h.contains( id ) )
    gone.push_back( id );
  for( int id : gone )
   f_h.erase( id );
  f_h.rekey(
    [ this ]( int id ) -> Key { return( ( *f_t )[ std::size_t( id ) ] ); } );
  }

 /// the heap, for the reads
 [[nodiscard]] const IndexedDaryHeap< Key , D > & heap( void ) const {
  return( f_h );
  }

/*------------------------- AS AN OBSERVER OF THE TABLE --------------------*/

 /// a value of the table written: its id updated
 template< class H , class V >
 void on( const mod::Changed< mod::Value , H , V > & e ) {
  update( int( e.h ) );
  }

 /// a value of the table inserted: its id updated
 template< class H , class K , class V >
 void on( const mod::Inserted< H , K , V > & e ) {
  update( int( e.h ) );
  }

 /// a value of the table erased: its id out of the queue
 template< class H , class K , class V >
 void on( const mod::Erased< H , K , V > & e ) noexcept {
  erase( int( e.h ) );
  }

 /// the table emptied: the queue too
 void on( const mod::Cleared & ) noexcept { f_h.clear(); }

 /// the table rebuilt: `refresh()`
 void on( const mod::Rebuilt & ) noexcept { refresh(); }

/*--------------------------------------------------------------------------*/
 private:
 const T * f_t;                  ///< the table
 IndexedDaryHeap< Key , D > f_h;  ///< the heap of the ids

 };  // end( class TableHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/IndexedDaryHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif /* IndexedDaryHeap.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File IndexedDaryHeap.h -----------------------*/
/*--------------------------------------------------------------------------*/
