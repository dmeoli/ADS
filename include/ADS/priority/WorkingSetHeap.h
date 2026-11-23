/*--------------------------------------------------------------------------*/
/*-------------------------- File WorkingSetHeap.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A heap of the integers \f$ [ 0 , n ) \f$ with keys and decrease-key that
 * has the working-set bound of Haeupler, Hladik, Rozhon, Tarjan and Tetek,
 * beyond the book: if the working set \f$ W( x ) \f$ of an item \f$ x \f$ is
 * the number of the insertions made from that of \f$ x \f$ to its removal,
 * that of \f$ x \f$ included, then `push()` and `decrease()` take
 * \f$ O( 1 ) \f$ amortized time and a `pop()` that returns \f$ x \f$ takes
 * \f$ O( 1 + \log W( x ) ) \f$, instead of the \f$ O( \log s ) \f$ of a heap
 * of \f$ s \f$ items. On this heap the algorithm of Dijkstra runs in
 * \f$ O( m + \log D ) \f$ time, \f$ D \f$ the number of the orders of the
 * nodes by their distances that some costs of the arcs give, which no
 * algorithm beats on any graph for the worst costs on it: it is
 * *universally optimal* (Theorem 7.4 of the paper; `universalDijkstra()`,
 * below). The interface is that of `IndexedDaryHeap` [see
 * `IndexedDaryHeap.h`], whose ids are the items and their own handles, so
 * that the shortest paths and the algorithm of Prim of the book take this
 * heap as they take that one [see `ShortestPaths.h`,
 * `MinimumSpanningTree.h`].
 *
 * The items are split among at most 5 inner heaps \f$ H_1 , \ldots , H_k \f$
 * by the order of their insertions, the latest in \f$ H_1 \f$, so that every
 * item of \f$ H_i \f$ was inserted after every item of \f$ H_j \f$ for
 * \f$ i < j \f$, and \f$ | H_i | \le 2^{2^i} \f$. An insertion makes a heap
 * \f$ H_0 \f$ of one item, melds \f$ H_j \f$ into \f$ H_{j + 1} \f$ for the
 * least \f$ j \f$ with \f$ | H_j | + | H_{j + 1} | \le 2^{2^{j + 1}} \f$
 * (\f$ H_{k + 1} \f$ empty), and moves \f$ H_0 , \ldots , H_{j - 1} \f$ one
 * place up, so that an item of \f$ H_i \f$, \f$ i > 1 \f$, has more than
 * \f$ 2^{2^{i - 3}} \f$ items in its working set (Corollary 10.5 of the
 * paper), which pays for the \f$ O( 2^i ) \f$ of a deletion from
 * \f$ H_i \f$. A bit \f$ b_i \f$ tells whether the least key of \f$ H_i \f$
 * is below those of all the heaps after it (a *suffix minimum*): the
 * minimum is in the heap of the lowest bit, `pop()` sets again the bits up
 * to its heap, `decrease()` clears those of the suffix minima before its
 * heap that its key now beats, and all of them fit in a word. The heap of an
 * item is that whose interval of the numbers of the insertions holds the
 * number of the insertion of the item, found by comparing it with the first
 * number of each heap, 7 comparisons with no branch; this takes the
 * place of the union-find of section 10.3 of the paper, and its bound is that
 * of the bit vector of section 10.4, the number of heaps being below the
 * size of a word.
 *
 * The inner heaps are type-1 rank-pairing heaps of Haeupler, Sen and Tarjan
 * on the nodes of the ids, all in one vector, so that a meld concatenates
 * two lists of roots in \f$ O( 1 ) \f$: each is a list of half-ordered
 * half-trees (a binary tree whose root has no right child, each node not
 * greater than the nodes of its left subtree), whose roots are linked
 * through their right pointers, its least root first; a link of two roots
 * of equal rank makes the greater the left child of the other, the old left
 * child of the winner the right child of the loser, and raises the rank of
 * the winner by one. A deletion takes the root out, makes a root of each
 * node on the right spine of its left child, of rank one more than its own
 * left child, and links in one pass the roots of equal rank; a decrease
 * cuts the node with its left subtree, puts its right child in its place and
 * lowers the ranks above it until the rule of type 1 holds again (every
 * child has the rank differences 1 and 1, or 0 and any positive one), in
 * \f$ O( 1 ) \f$ amortized time, \f$ O( \log s ) \f$ for a deletion.
 *
 * Beyond the paper, the heap is addressable in full, as `IndexedDaryHeap`:
 * `erase()` takes an id out of its inner heap \f$ H_i \f$ as a deletion
 * does, in \f$ O( \log | H_i | + i ) \f$ amortized time, `increase()` takes
 * it out and inserts it again with the new key, as a new item, and
 * `update()` sets the key either way; `extract( theta , out )` removes every
 * id of key at most \f$ \theta \f$. The range of the ids is set by the
 * constructor and by `reset( n )`, which empties the heap, and raised by
 * `grow( n )`, which keeps it.
 *
 * The heap tells its changes through the subject policy `Policy` [see
 * `Modification.h`], `NoObservers` by default, with which every operation
 * compiles to the code of the plain heap: `Inserted` by `push()`, after it,
 * `Erased` by `erase()` and `pop()` (the id and its key, before the
 * removal), `KeyChanged` by `decrease()`, `increase()` and `update()` (the
 * old and the new key), `Cleared` after `clear()` and `reset()`, `Rebuilt`
 * at the end of a mute. The nodes stay at the places of their ids, which are
 * the handles of the events, and hence there is no `Relocated`.
 *
 * The vector of the nodes comes from the allocator `A`, `std::allocator` by
 * default, given to the constructor that takes `std::allocator_arg` first,
 * with the rules of the standard [see `AllocatorAware.h`]; a node holds the
 * key, three links, the rank and the number of its insertion, i.e.,
 * \f$ k + 24 \f$ bytes rounded to the alignment, \f$ k \f$ the size of a key.
 *
 * The numbers of the lemmas and of the sections of Haeupler et al. cited
 * here are those of the extended version of their paper (arXiv:2311.11793v4,
 * 2025), whose heap, simpler than that of the conference version, is the one
 * realized.
 *
 * The contracts [see `Contract.h`]: the ids are in \f$ [ 0 , n ) \f$,
 * `push()` needs an id not in the heap, `decrease()` one in the heap and a
 * key not greater than its own, `increase()` one in the heap and a key not
 * less than its own, `update()` and `erase()` one in the heap, `top()` and
 * `pop()` a heap that is not empty; the invariant, checked on heaps of at
 * most 1024 items, is the order of the half-trees, their links and the rule
 * of type 1 on their ranks, the numbers of the insertions within the
 * interval of their heap, the sizes of the heaps within their bounds and the
 * bits of the suffix minima. The operations are defined in
 * `WorkingSetHeap.tpp`.
 *
 * \see B. Haeupler, R. Hladik, V. Rozhon, R. E. Tarjan, J. Tetek, Universal
 *      optimality of Dijkstra via beyond-worst-case heaps, 65th IEEE Annual
 *      Symposium on Foundations of Computer Science (FOCS), 2099-2130, 2024,
 *      doi:10.1109/FOCS61266.2024.00125
 * \see B. Haeupler, S. Sen, R. E. Tarjan, Rank-pairing heaps, SIAM Journal
 *      on Computing 40(6), 1463-1485, 2011, doi:10.1137/100785351
 * \see J. Iacono, Improved upper bounds for pairing heaps, Algorithm
 *      Theory - SWAT 2000, Lecture Notes in Computer Science 1851, 32-45,
 *      2000, doi:10.1007/3-540-44985-X_5
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

#ifndef __ADS_WorkingSetHeap
 #define __ADS_WorkingSetHeap
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/Portable.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/memory/AllocatorAware.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS WorkingSetHeap --------------------------*/
/*--------------------------------------------------------------------------*/

/// a heap of the ids \f$ [ 0 , n ) \f$ by their keys, the least on top,
/// with decrease-key and the working-set bound, its changes told through
/// `Policy`
template< class K , class Policy = NoObservers ,
	  class A = std::allocator< K > >
class WorkingSetHeap
{

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
 /// the heap emptied or rebuilt
 using modifications =
   mod::list< mod::Inserted< int , K > , mod::Erased< int , K > ,
	      mod::KeyChanged< int , K > , mod::Cleared , mod::Rebuilt >;
 /// the subject
 using Subject =
   typename Policy::template bind< modifications , WorkingSetHeap >;

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the static observers
 /// `o...`
 /// \pre \f$ n \ge 0 \f$
 template< class... O >
 explicit WorkingSetHeap( int n = 0 , O &... o );

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the static observers
 /// `o...`, its vector from `alloc`
 /// \pre \f$ n \ge 0 \f$
 template< class... O >
 WorkingSetHeap( std::allocator_arg_t , const A & alloc , int n = 0 ,
		 O &... o );

 /// the type of the allocator
 using allocator_type = A;
 /// the allocator of the vector of the nodes
 [[nodiscard]] allocator_type get_allocator( void ) const {
  return( allocator_type( f_N.get_allocator() ) );
  }

 /// the empty heap of the ids \f$ [ 0 , n ) \f$, the room kept
 /// \pre \f$ n \ge 0 \f$
 void reset( int n );

 /// the ids raised to \f$ [ 0 , n ) \f$, the new ones not in the heap and
 /// the items kept, in \f$ O( 1 ) \f$ amortized time per new id
 /// \pre \f$ n \ge \f$ `ids()`
 void grow( int n );

 /// the number of ids, \f$ n \f$
 [[nodiscard]] int ids( void ) const { return( int( f_N.size() ) ); }

 /// whether the heap has no item
 [[nodiscard]] bool empty( void ) const { return( f_s == 0 ); }

 /// the number of items, \f$ s \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_s ); }

 /// the number of the inner heaps, at most 5: a heap of index \f$ j \f$
 /// exists only after more than \f$ 2^{2^{j - 1}} \f$ items were in the
 /// heap at once (Lemma 10.3 of Haeupler et al.)
 [[nodiscard]] int heaps( void ) const { return( f_k ); }

 /// whether the id is in the heap
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$
 [[nodiscard]] bool contains( int id ) const;

 /// the key of an id in the heap
 /// \pre the id is in the heap
 [[nodiscard]] const K & key( int id ) const;

 /// inserts the id with the key \f$ k \f$, in \f$ O( 1 ) \f$ amortized
 /// time
 ///
 /// In pseudocode (section 10 of Haeupler et al., beyond the book), the
 /// heaps \f$ H_1 , \ldots , H_k \f$ from the latest, \f$ b \f$ the bits
 /// of their suffix minima and \f$ c_i = 2^{2^i} \f$ the bound of the size
 /// of \f$ H_i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( \mathit{id} , k ) \cr
 /// 2 & \quad H_0 \gets \text{a rank-pairing heap of } ( k , \mathit{id}
 ///     ) \text{ alone};\ t \gets t + 1;\ \mathit{stamp}[ \mathit{id} ]
 ///     \gets t \cr
 /// 3 & \quad j \gets 0;\ \textbf{while}\ j < k\ \textbf{and}\ | H_j | +
 ///     | H_{j + 1} | > c_{j + 1}\ \textbf{do}\ j \gets j + 1 \cr
 /// 4 & \quad \textbf{if}\ j < k\ \textbf{then}\ H_{j + 1} \gets
 ///     \mathrm{meld}( H_j , H_{j + 1} )\ \textbf{else}\ H_{j + 1} \gets
 ///     H_j;\ k \gets k + 1 \cr
 /// 5 & \quad \textbf{for}\ i \gets j - 1\ \textbf{downto}\ 0\ \textbf{do}
 ///     \ H_{i + 1} \gets H_i \cr
 /// 6 & \quad \textbf{if}\ j > 0\ \textbf{then}\ \mathrm{suffix}( j + 1
 ///     )\ \textbf{else if}\ \mathit{id} \text{ is the first root of }
 ///     H_1\ \textbf{then}\ \text{the bits as in lines 12 to 14 of}
 ///     \ \mathrm{decrease}() \cr
 /// 7 & \textbf{procedure}\ \mathrm{suffix}( h )\quad \triangleright
 ///     \ b_h , \ldots , b_1 \text{ set again} \cr
 /// 8 & \quad \mu \gets \text{the least key of } H_{\min \{ i > h : b_i
 ///     = 1 \}} \text{, or } + \infty \cr
 /// 9 & \quad \textbf{for}\ i \gets h\ \textbf{downto}\ 1\ \textbf{do} \cr
 /// 10 & \qquad b_i \gets ( H_i \ne \emptyset\ \textbf{and}\ \min H_i <
 ///     \mu );\ \textbf{if}\ b_i\ \textbf{then}\ \mu \gets \min H_i
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$, not in the heap
 void push( int id , const K & k );

 /// lowers the key of the id to \f$ k \f$, in \f$ O( 1 ) \f$ amortized
 /// time
 ///
 /// In pseudocode (sections 10.4 of Haeupler et al. and 4 of Haeupler,
 /// Sen and Tarjan, beyond the book), \f$ r( v ) \f$ the rank of \f$ v \f$
 /// and \f$ -1 \f$ that of a missing child:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{decrease}( x , k ) \cr
 /// 2 & \quad i \gets \text{the heap whose interval of numbers holds }
 ///     \mathit{stamp}[ x ];\ x.\mathit{key} \gets k \cr
 /// 3 & \quad \textbf{if}\ x \text{ is not a root}\ \textbf{then} \cr
 /// 4 & \qquad y \gets x.\mathit{right};\ \text{put } y \text{ in the}
 ///     \text{ place of } x \text{ under } u = x.\mathit{parent} \cr
 /// 5 & \qquad r( x ) \gets r( x.\mathit{left} ) + 1;\ \text{add } x
 ///     \text{ to the roots of } H_i \cr
 /// 6 & \qquad \textbf{while}\ u \text{ is not a root}\ \textbf{do}
 ///     \quad \triangleright\ \text{the ranks of type 1} \cr
 /// 7 & \qquad\quad ( a , c ) \gets ( r( u.\mathit{left} ) ,
 ///     r( u.\mathit{right} ) );\ \rho \gets \max( a , c ) +
 ///     [ a = c ] \cr
 /// 8 & \qquad\quad \textbf{if}\ \rho \ge r( u )\ \textbf{then}
 ///     \ \textbf{break} \cr
 /// 9 & \qquad\quad r( u ) \gets \rho;\ u \gets u.\mathit{parent} \cr
 /// 10 & \qquad \textbf{if}\ u \text{ is a root}\ \textbf{then}\ r( u )
 ///     \gets r( u.\mathit{left} ) + 1 \cr
 /// 11 & \quad \textbf{if}\ k < \min H_i\ \textbf{then}\ x \text{ becomes}
 ///     \text{ the first root of } H_i \cr
 /// 12 & \quad \textbf{if}\ x \text{ is first and}\ k < \min H_{\min \{ j
 ///     > i : b_j = 1 \}} \text{ (or none)}\ \textbf{then} \cr
 /// 13 & \qquad b_i \gets 1;\ \textbf{for each}\ j < i \text{ with } b_j
 ///     = 1 \text{, downward, while } \min H_j \ge k\ \textbf{do}\ b_j
 ///     \gets 0 \cr
 /// 14 & \quad \textbf{else if}\ x \text{ is first}\ \textbf{then}
 ///     \ b_i \gets 0
 /// \end{array}}
 /// \f]
 /// \pre the id is in the heap, \f$ k \f$ not greater than its key
 void decrease( int id , const K & k );

 /// raises the key of the id to \f$ k \f$: the id taken out as by
 /// `erase()` and inserted again as by `push()`, a new item, in
 /// \f$ O( \log s ) \f$ amortized time
 /// \pre the id is in the heap, \f$ k \f$ not less than its key
 void increase( int id , const K & k );

 /// sets the key of the id to \f$ k \f$, by `decrease()` or by
 /// `increase()`
 /// \pre the id is in the heap
 void update( int id , const K & k );

 /// removes the id from the heap: cut from its half-tree as by
 /// `decrease()`, then taken out of the roots of its heap as by `pop()`,
 /// in \f$ O( \log s ) \f$ amortized time
 /// \pre the id is in the heap
 void erase( int id );

 /// `push( id , k )` if the id is not in the heap, `decrease( id , k )` if
 /// \f$ k \f$ is less than its key, nothing otherwise; true if the key
 /// changed
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$
 bool pushOrDecrease( int id , const K & k );

 /// the item of least key
 /// \pre the heap is not empty
 [[nodiscard]] Entry top( void ) const;

 /// removes the item of least key and returns it, in
 /// \f$ O( 1 + \log W ) \f$ amortized time, \f$ W \f$ its working set
 ///
 /// In pseudocode (sections 10.1 and 10.4 of Haeupler et al. and 3 of
 /// Haeupler, Sen and Tarjan, beyond the book), \f$ \mathrm{suffix}() \f$
 /// that of `push()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad j \gets \min \{ i \ge 1 : b_i = 1 \};\ x \gets
 ///     \text{the first root of } H_j \cr
 /// 3 & \quad R \gets \text{the other roots of } H_j \text{ and the}
 ///     \text{ nodes on the right spine of } x.\mathit{left} \cr
 /// 4 & \quad \textbf{for each}\ y\ \text{on that spine}\ \textbf{do}
 ///     \ y.\mathit{right} \gets \textbf{nil};\ r( y ) \gets r(
 ///     y.\mathit{left} ) + 1 \cr
 /// 5 & \quad \textbf{for each}\ y \in R\ \textbf{do}\quad
 ///     \triangleright\ \text{one pass} \cr
 /// 6 & \qquad \textbf{if}\ B[ r( y ) ] = \textbf{nil}\ \textbf{then}
 ///     \ B[ r( y ) ] \gets y \cr
 /// 7 & \qquad \textbf{else}\ \text{put } \mathrm{link}( B[ r( y ) ] , y )
 ///     \text{ among the roots};\ B[ r( y ) ] \gets \textbf{nil} \cr
 /// 8 & \quad \text{put the half-trees left in } B \text{ among the}
 ///     \text{ roots of } H_j \text{, the least first} \cr
 /// 9 & \quad \mathrm{suffix}( j );\ \textbf{return}\ ( x.\mathit{key} , x
 ///     ) \cr
 /// 10 & \textbf{function}\ \mathrm{link}( u , v )\quad \triangleright
 ///     \ r( u ) = r( v ) \cr
 /// 11 & \quad \textbf{if}\ v.\mathit{key} < u.\mathit{key}\ \textbf{then}
 ///     \ \text{swap } u \text{ and } v \cr
 /// 12 & \quad v.\mathit{right} \gets u.\mathit{left};\ u.\mathit{left}
 ///     \gets v;\ r( u ) \gets r( u ) + 1;\ \textbf{return}\ u
 /// \end{array}}
 /// \f]
 /// \pre the heap is not empty
 Entry pop( void );

 /// removes all the items, in \f$ O( s ) \f$ time
 void clear( void );

 /// every id of key at most \f$ \theta \f$ appended to `out`, by
 /// increasing keys, each removed as by `pop()`
 void extract( const K & theta , std::vector< int > & out );

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
 using Ins = mod::Inserted< int , K >;    ///< an insertion
 using Era = mod::Erased< int , K >;      ///< an erasure
 using Chg = mod::KeyChanged< int , K >;  ///< a change of key

 static constexpr int OUT = -2;   ///< the parent of an id not in the heap
 static constexpr int ROOT = -1;  ///< the parent of a root
 static constexpr int L = 8;      ///< the places of the heaps, H_0 to H_7

 /// a node: the key, the links, the rank, the number of the insertion
 struct Node
 {
  K key{};                  ///< the key
  int left = -1;            ///< the left child
  int right = -1;           ///< the right child, or the next root
  int parent = OUT;         ///< the parent, ROOT or OUT
  int rank = 0;             ///< the rank
  std::uint64_t stamp = 0;  ///< the number of the insertion
  };

 /// whether someone may want the event `E`, at compile time
 template< class E >
 static constexpr bool heard = Subject::template listened< E >;

 /// whether someone wants the event `E` now
 template< class E >
 [[nodiscard]] bool alive( void ) const {
  return( f_sub.template live< E >() );
  }

 /// the bound \f$ 2^{2^i} \f$ of the size of H_i, saturated
 [[nodiscard]] static constexpr std::size_t cap( int i ) {
  return( i >= 6 ? std::numeric_limits< std::size_t >::max()
		  : std::size_t( 1 ) << ( 1u << unsigned( i ) ) );
  }

 /// the rank of v, -1 if v is missing
 [[nodiscard]] int rank( int v ) const {
  return( v < 0 ? -1 : f_N[ std::size_t( v ) ].rank );
  }

 /// whether the key of u is less than that of v
 [[nodiscard]] bool less( int u , int v ) const {
  return( f_N[ std::size_t( u ) ].key < f_N[ std::size_t( v ) ].key );
  }

 /// the least i >= h with b_i = 1, -1 if none
 [[nodiscard]] int next( int h ) const {
  const unsigned m = f_bits & ~( ( 1u << unsigned( h ) ) - 1u );
  return( m ? std::countr_zero( m ) : -1 );
  }

 /// the greatest i <= h with b_i = 1, -1 if none
 [[nodiscard]] int prev( int h ) const {
  if( h < 0 )
   return( -1 );
  const unsigned m = f_bits & ( ( 2u << unsigned( h ) ) - 1u );
  return( m ? int( std::bit_width( m ) ) - 1 : -1 );
  }

 /// the heap of the insertion number s, by its first number
 [[nodiscard]] int heapOf( std::uint64_t s ) const {
  int i = 1;  // the places past f_k have first number 0
  for( int j = 1 ; j < L ; ++j )
   i += ( f_start[ j ] > s ) ? 1 : 0;
  return( i );
  }

 /// the bits b_h , ... , b_1 set again
 void suffix( int h );

 /// the bits after the least key of H_i fell to that of x, its first
 /// root: b_i set if the key is below the least of the heaps after H_i,
 /// and then cleared for the suffix minima before H_i that it beats
 void lowered( int i , int x );

 /// the root x, with no next root, added to the roots of H_i
 void addRoot( int i , int x );

 /// the root of the link of the two roots u and v of equal rank
 int link( int u , int v );

 /// x, not a root of H_i, cut with its left subtree and made a root, the
 /// ranks above it lowered to the rule of type 1
 void cut( int i , int x );

 /// the root x out of H_i, its half-tree undone and the roots linked in
 /// one pass; the size of H_i lowered by one
 void removeRoot( int i , int x );

 /// the operations, nobody told
 void pushPlain( int id , const K & k );
 void decreasePlain( int id , const K & k );  ///< as pushPlain()
 void erasePlain( int id );                  ///< as pushPlain()
 Entry popPlain( void );                     ///< as pushPlain()

 /// the operations when someone listens, out of the hot code
 ADS_COLD_PATH void pushCold( int id , const K & k );
 ADS_COLD_PATH void decreaseCold( int id , const K & k );
 ADS_COLD_PATH void increaseCold( int id , const K & k );
 ADS_COLD_PATH void eraseCold( int id );
 ADS_COLD_PATH Entry popCold( void );

 /// the half-tree of v under the bound \f$ \beta \f$ of its keys, whose
 /// parent is p, half-ordered with the links back and the ranks of type 1,
 /// its insertion numbers in [ lo , hi ), its nodes counted in c
 [[nodiscard]] bool tree( int v , int p , int beta , std::uint64_t lo ,
			  std::uint64_t hi , std::size_t & c ) const;

 /// the invariant, on heaps of at most 1024 items
 [[nodiscard]] bool ordered( void ) const;

 detail::AllocVector< A , Node > f_N;        ///< the nodes, at their ids
 std::array< int , L > f_first{};            ///< the least root of each heap
 std::array< std::size_t , L > f_size{};     ///< the size of each heap
 std::array< std::uint64_t , L > f_start{};  ///< its first number
 std::array< int , 64 > f_B{};          ///< the roots by rank of a deletion
 int f_k = 0;                          ///< the number of heaps
 unsigned f_bits = 0;                  ///< the bits of the suffix minima
 std::uint64_t f_t = 0;                ///< the insertions made
 std::size_t f_s = 0;                  ///< the number of items
 ADS_NO_UNIQUE_ADDRESS Subject f_sub;  ///< the observers

 };  // end( class WorkingSetHeap )

/*--------------------------------------------------------------------------*/
/*------------------------- THE UNIVERSAL DIJKSTRA -------------------------*/
/*--------------------------------------------------------------------------*/

/// Dijkstra on a heap with the working-set bound: the prototype of section
/// 11.2 with `WorkingSetHeap` as its priority queue, in
/// \f$ O( m + \log D ) \f$ time, \f$ D \f$ the number of the orders of the
/// nodes by their distances from \f$ r \f$ over all the costs of the arcs,
/// which is universally optimal (Theorem 7.4 of Haeupler et al.)
///
/// The prototype with its structure S (section 11.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{universalDijkstra}( G , r ) \cr
/// 2 & \quad \textbf{return}\ \mathrm{shortestPaths}( G , r , \text{a }
///     \text{heap with the working-set bound} )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node, the costs are not negative
/// \post the conditions of Bellman (audit)
template< Graph G >
auto universalDijkstra( const G & g , int r );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/WorkingSetHeap.tpp"

/*--------------------------------------------------------------------------*/

#endif /* WorkingSetHeap.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File WorkingSetHeap.h ------------------------*/
/*--------------------------------------------------------------------------*/
