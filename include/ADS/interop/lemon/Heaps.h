/*--------------------------------------------------------------------------*/
/*----------------------------- File Heaps.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The heaps of ADS as heaps of abLEMON, beyond the book: models of the concept
 * `lemon::concepts::Heap< PR , IM , CMP >` (the file `lemon/concepts/heap.h`
 * of LEMON 1.3 and of abLEMON), so that an algorithm of abLEMON that takes its
 * heap by its traits (`Dijkstra`, `Prim`, `AStar`, the bidirectional
 * Dijkstra, `CapacityScaling` and the others) runs on a heap of ADS by one
 * named template parameter, e.g.,
 *
 *     using H = ADS::lemon_interop::DaryHeap< int , GR::NodeMap< int > >;
 *     lemon::Dijkstra< GR , LM >::SetStandardHeap< H >::Create d( g , l );
 *
 * (see \ref theory_graph_lemon). abLEMON's protocol has two parts that are not
 * in the signatures: the cross-reference map, from the items to `int`, is
 * owned by the caller, and the algorithm writes it directly
 * (`Dijkstra::init()` sets `PRE_HEAP` for every node without calling the
 * heap), so that a heap derives the state of an item from the value in the
 * map alone.
 *
 * - `DaryHeap< PR , IM , CMP , D , Policy >` runs on `IndexedDaryHeap< K , D
 *   , Policy >` [see `IndexedDaryHeap.h`]: the map holds, for an item in the
 *   heap, a slot of the heap of ADS (\f$ \ge 0 \f$), and `PRE_HEAP` or
 *   `POST_HEAP` otherwise, as abLEMON's `FibHeap` does; the slots are recycled
 *   by a list of the free ones, so that the heap of ADS has as many ids as
 *   the largest frontier, and the space is \f$ ( 4 + k ) s + 4 f + w f \f$
 *   bytes for \f$ s \f$ items, \f$ f \f$ slots, keys of \f$ k \f$ bytes and
 *   items of \f$ w \f$. Each operation costs that of the heap of ADS, in
 *   \f$ O( \log_D s ) \f$ or \f$ O( D \log_D s ) \f$, plus one read of the
 *   map; with `Policy = Dynamic<>` an observer attached to `subject()`
 *   receives the events of the heap of ADS for every operation of the
 *   algorithm on its frontier, the handles being the slots (`item()` gives
 *   their items), which no heap of abLEMON tells. The comparator must be
 *   stateless, since the heap of ADS compares its keys by `<`.
 * - `MonotoneHeap< Q , PR , IM >` puts under the same concept a monotone
 *   queue without decrease-key, `DialQueue` or `RadixHeap` [see
 *   `DialQueue.h`, `RadixHeap.h`]: `decrease()` and `increase()` push a
 *   second copy of the slot, and the copies whose key is not the present
 *   priority of their slot, or whose slot left, are skipped when they reach
 *   the top (the lazy deletion); a slot is never reused before `clear()`.
 *   It is valid while every key pushed is not less than the last one
 *   popped, i.e., for Dijkstra's algorithm with nonnegative integer lengths,
 *   where a node is pushed at most once and the slots are at most \f$ n \f$;
 *   the window of `DialQueue` takes \f$ O( C ) \f$ buckets for lengths at
 *   most \f$ C \f$.
 *
 * The concurrent queues of ADS (`MultiQueue`, `LockFreePriorityQueue`) are
 * relaxed, and cannot be given to an algorithm of abLEMON that takes a popped
 * item as final.
 *
 * The contracts [see `Contract.h`]: those of the concept of abLEMON (`top()`,
 * `prio()` and `pop()` on a heap that is not empty, `decrease()` and
 * `increase()` on an item in the heap with a priority not greater, or not
 * less, than its own), checked by those of the heap of ADS below, and, for
 * `MonotoneHeap`, the monotone use. The heaps are defined in this file.
 *
 * \see B. Dezső, A. Jüttner, P. Kovács, LEMON - an open source C++ graph
 *      template library, Electronic Notes in Theoretical Computer Science
 *      264(5), 23-45, 2011, doi:10.1016/j.entcs.2011.06.003
 * \see R. B. Dial, Algorithm 360: shortest-path forest with topological
 *      ordering, Communications of the ACM 12(11), 632-633, 1969,
 *      doi:10.1145/363269.363610
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

#ifndef __ADS_interop_lemon_Heaps
 #define __ADS_interop_lemon_Heaps
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <lemon/core.h> )
 #error "the interoperation with abLEMON needs the headers of abLEMON"
#endif

#include "ADS/Contract.h"
#include "ADS/Modification.h"
#include "ADS/priority/IndexedDaryHeap.h"

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS CmpKey --------------------------------*/
/*--------------------------------------------------------------------------*/

/// a priority of abLEMON as a key of ADS, ordered by the comparator `CMP`
template< class PR , class CMP >
struct CmpKey
{
 PR p;  ///< the priority
 /// whether \f$ a \f$ comes before \f$ b \f$
 friend bool operator<( const CmpKey & a , const CmpKey & b ) {
  return( CMP()( a.p , b.p ) );
  }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DaryHeap ------------------------------*/
/*--------------------------------------------------------------------------*/

/// an `IndexedDaryHeap` of ADS as a heap of abLEMON, on the cross-reference
/// map `IM` of the caller
template< class PR , class IM , class CMP = std::less< PR > , unsigned D = 4 ,
	  class Policy = NoObservers >
class DaryHeap
{

 static_assert( std::is_empty_v< CMP > , "the comparator is stateless" );

/*--------------------------------------------------------------------------*/
 public:
 using ItemIntMap = IM;                 ///< the cross-reference map
 using Prio = PR;                       ///< the priorities
 using Item = typename IM::Key;         ///< the items
 using Pair = std::pair< Item , Prio >;  ///< an item with its priority
 using Key = CmpKey< PR , CMP >;         ///< the keys of the heap of ADS
 using Heap = IndexedDaryHeap< Key , D , Policy >;  ///< the heap of ADS

 /// the states of an item, those of abLEMON
 enum State { IN_HEAP = 0 , PRE_HEAP = -1 , POST_HEAP = -2 };

 /// the empty heap on the map \f$ m \f$
 explicit DaryHeap( ItemIntMap & m ) : f_iim( m ) { }

 /// the empty heap on the map \f$ m \f$, the comparator not kept
 DaryHeap( ItemIntMap & m , const CMP & ) : f_iim( m ) { }

 /// the number of items
 [[nodiscard]] int size( void ) const { return( int( f_q.size() ) ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_q.empty() ); }

 /// all the items removed, the map left as it is (as in abLEMON)
 void clear( void ) {
  f_q.reset( 0 );
  f_item.clear();
  f_free.clear();
  }

 /// inserts the item \f$ i \f$ with the priority \f$ p \f$, in a free slot
 /// \pre \f$ i \f$ is not in the heap
 void push( const Item & i , const Prio & p );

 /// inserts the pair \f$ x \f$
 /// \pre its item is not in the heap
 void push( const Pair & x ) { push( x.first , x.second ); }

 /// the item of least priority
 /// \pre the heap is not empty
 [[nodiscard]] Item top( void ) const { return( f_item[ f_q.top().id ] ); }

 /// the least priority
 /// \pre the heap is not empty
 [[nodiscard]] Prio prio( void ) const { return( f_q.top().key.p ); }

 /// removes the item of least priority, `POST_HEAP` from now on
 /// \pre the heap is not empty
 void pop( void ) { release( f_q.pop().id ); }

 /// removes the item \f$ i \f$, if it is in the heap
 void erase( const Item & i ) {
  const int s = f_iim[ i ];
  if( s >= 0 ) {
   f_q.erase( s );
   release( s );
   }
  }

 /// the priority of the item \f$ i \f$
 /// \pre \f$ i \f$ is in the heap
 [[nodiscard]] Prio operator[]( const Item & i ) const {
  return( f_q.key( f_iim[ i ] ).p );
  }

 /// the priority of \f$ i \f$ set to \f$ p \f$, inserting it if needed
 void set( const Item & i , const Prio & p ) {
  if( state( i ) == IN_HEAP )
   f_q.update( f_iim[ i ] , Key{ p } );
  else
   push( i , p );
  }

 /// the priority of \f$ i \f$ lowered to \f$ p \f$
 /// \pre \f$ i \f$ is in the heap, \f$ p \f$ not after its priority
 void decrease( const Item & i , const Prio & p ) {
  f_q.decrease( f_iim[ i ] , Key{ p } );
  }

 /// the priority of \f$ i \f$ raised to \f$ p \f$
 /// \pre \f$ i \f$ is in the heap, \f$ p \f$ not before its priority
 void increase( const Item & i , const Prio & p ) {
  f_q.increase( f_iim[ i ] , Key{ p } );
  }

 /// the state of the item \f$ i \f$, read from the map
 [[nodiscard]] State state( const Item & i ) const {
  const int s = f_iim[ i ];
  return( s >= 0 ? IN_HEAP : State( s ) );
  }

 /// the state of \f$ i \f$ set to \f$ st \f$, removing it from the heap if
 /// it was there; `IN_HEAP` changes nothing
 void state( const Item & i , State st ) {
  if( st == IN_HEAP )
   return;
  if( state( i ) == IN_HEAP )
   erase( i );
  f_iim.set( i , int( st ) );
  }

 /// the heap of ADS, whose subject tells the changes, the slots their
 /// handles
 [[nodiscard]] Heap & heap( void ) { return( f_q ); }
 /// the heap of ADS
 [[nodiscard]] const Heap & heap( void ) const { return( f_q ); }

 /// the subject of the heap of ADS
 [[nodiscard]] typename Heap::Subject & subject( void ) {
  return( f_q.subject() );
  }

 /// the item of the slot \f$ s \f$, the handle of the events
 /// \pre \f$ s \f$ is a slot in use
 [[nodiscard]] const Item & item( int s ) const {
  ADS_PRE( ( s >= 0 ) && ( std::size_t( s ) < f_item.size() ) );
  return( f_item[ s ] );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the slot \f$ s \f$ freed, its item `POST_HEAP`
 void release( int s ) {
  f_iim.set( f_item[ s ] , int( POST_HEAP ) );
  f_free.push_back( s );
  }

 ItemIntMap & f_iim;          ///< the cross references
 Heap f_q;                    ///< the heap of ADS, of the slots
 std::vector< Item > f_item;  ///< the item of each slot
 std::vector< int > f_free;   ///< the slots not in use

 };  // end( class DaryHeap )

template< class PR , class IM , class CMP , unsigned D , class Policy >
void DaryHeap< PR , IM , CMP , D , Policy >::push( const Item & i ,
					       const Prio & p )
{
 int s;
 if( f_free.empty() ) {
  s = int( f_item.size() );
  f_item.push_back( i );
  f_q.grow( s + 1 );
  }
 else {
  s = f_free.back();
  f_free.pop_back();
  f_item[ s ] = i;
  }
 f_iim.set( i , s );
 f_q.push( s , Key{ p } );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MonotoneHeap ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a monotone queue of ADS without decrease-key, `Q` either
/// `DialQueue< int , K >` or `RadixHeap< int , K >`, as a heap of abLEMON by
/// the lazy deletion
template< class Q , class PR , class IM >
class MonotoneHeap
{

 static_assert( std::is_integral_v< PR > , "integer priorities" );
 static_assert( std::is_same_v< typename Q::Item , int > ,
		"a queue of the slots, int" );

/*--------------------------------------------------------------------------*/
 public:
 using ItemIntMap = IM;                 ///< the cross-reference map
 using Prio = PR;                       ///< the priorities
 using Item = typename IM::Key;         ///< the items
 using Pair = std::pair< Item , Prio >;  ///< an item with its priority

 /// the states of an item, those of abLEMON
 enum State { IN_HEAP = 0 , PRE_HEAP = -1 , POST_HEAP = -2 };

 /// the empty heap on the map \f$ m \f$
 explicit MonotoneHeap( ItemIntMap & m ) : f_iim( m ) { }

 /// the empty heap on the map \f$ m \f$, the comparator, which must be
 /// `std::less`, not kept
 template< class C >
 MonotoneHeap( ItemIntMap & m , const C & ) : f_iim( m ) { }

 /// the number of items
 [[nodiscard]] int size( void ) const { return( f_live ); }

 /// whether there is no item
 [[nodiscard]] bool empty( void ) const { return( f_live == 0 ); }

 /// all the items removed, the map left as it is
 void clear( void ) {
  f_q.clear();
  f_item.clear();
  f_prio.clear();
  f_in.clear();
  f_live = 0;
  }

 /// inserts \f$ i \f$ with the priority \f$ p \f$, in a new slot
 /// \pre \f$ i \f$ is not in the heap, \f$ p \ge 0 \f$ not less than the
 ///      last priority popped
 void push( const Item & i , const Prio & p ) {
  ADS_PRE( p >= Prio( 0 ) );
  const int s = int( f_item.size() );
  f_item.push_back( i );
  f_prio.push_back( p );
  f_in.push_back( 1 );
  f_iim.set( i , s );
  ++f_live;
  f_q.push( s , Key( p ) );
  }

 /// inserts the pair \f$ x \f$
 /// \pre as `push()`
 void push( const Pair & x ) { push( x.first , x.second ); }

 /// the item of least priority
 /// \pre the heap is not empty
 [[nodiscard]] Item top( void ) const {
  skip();
  return( f_item[ f_q.top() ] );
  }

 /// the least priority
 /// \pre the heap is not empty
 [[nodiscard]] Prio prio( void ) const {
  skip();
  return( f_prio[ f_q.top() ] );
  }

 /// removes the item of least priority, `POST_HEAP` from now on
 /// \pre the heap is not empty
 void pop( void ) {
  skip();
  const int s = f_q.pop();
  f_iim.set( f_item[ s ] , int( POST_HEAP ) );
  f_in[ s ] = 0;
  --f_live;
  }

 /// the priority of the item \f$ i \f$
 /// \pre \f$ i \f$ is in the heap
 [[nodiscard]] Prio operator[]( const Item & i ) const {
  return( f_prio[ f_iim[ i ] ] );
  }

 /// the priority of \f$ i \f$ set to \f$ p \f$ by a new copy, the old one
 /// stale
 /// \pre \f$ i \f$ is in the heap, \f$ p \f$ not less than the last
 ///      priority popped
 void decrease( const Item & i , const Prio & p ) {
  const int s = f_iim[ i ];
  ADS_PRE( s >= 0 );
  f_prio[ s ] = p;
  f_q.push( s , Key( p ) );
  }

 /// as `decrease()`: the copy of the smaller priority goes stale
 /// \pre as `decrease()`
 void increase( const Item & i , const Prio & p ) { decrease( i , p ); }

 /// removes the item \f$ i \f$, if it is in the heap; its copies go stale
 void erase( const Item & i ) {
  const int s = f_iim[ i ];
  if( s < 0 )
   return;
  f_iim.set( i , int( POST_HEAP ) );
  f_in[ s ] = 0;
  --f_live;
  }

 /// the priority of \f$ i \f$ set to \f$ p \f$, inserting it if needed
 void set( const Item & i , const Prio & p ) {
  if( state( i ) == IN_HEAP )
   decrease( i , p );
  else
   push( i , p );
  }

 /// the state of the item \f$ i \f$, read from the map
 [[nodiscard]] State state( const Item & i ) const {
  const int s = f_iim[ i ];
  return( s >= 0 ? IN_HEAP : State( s ) );
  }

 /// the state of \f$ i \f$ set to \f$ st \f$, removing it from the heap if
 /// it was there; `IN_HEAP` changes nothing
 void state( const Item & i , State st ) {
  if( st == IN_HEAP )
   return;
  if( state( i ) == IN_HEAP )
   erase( i );
  f_iim.set( i , int( st ) );
  }

/*--------------------------------------------------------------------------*/
 private:
 using Key = typename Q::Key;  ///< the keys of the queue

 /// the stale copies on top popped: those whose slot left or whose key is
 /// not the priority of the slot
 void skip( void ) const {
  while( ! f_q.empty() ) {
   const int s = f_q.top();
   if( f_in[ s ] && ( Key( f_prio[ s ] ) == f_q.topKey() ) )
    return;
   f_q.pop();
   }
  }

 ItemIntMap & f_iim;          ///< the cross references
 mutable Q f_q;               ///< the queue of the copies of the slots
 std::vector< Item > f_item;  ///< the item of each slot
 std::vector< Prio > f_prio;  ///< the priority of each slot
 std::vector< char > f_in;    ///< whether each slot is in the heap
 int f_live = 0;              ///< the items in the heap

 };  // end( class MonotoneHeap )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Heaps.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Heaps.h ----------------------------*/
/*--------------------------------------------------------------------------*/
