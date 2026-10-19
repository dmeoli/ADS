/*--------------------------------------------------------------------------*/
/*----------------------- File contract_priority.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the priority queues: every
 * operation of the specification of the book, its result as the specification
 * states it, and the violations of its preconditions, which must throw
 * ADS::contract_violation. The tests are compiled at the level 3, so that each
 * operation also checks its postconditions and the invariant of the structure,
 * on random sequences of operations against a reference of the standard
 * library.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/priority/TreePriorityQueue.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <list>
#include <optional>
#include <random>
#include <set>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// PRIORITYQUEUE on HeapPriorityQueue: the preconditions of min(),
// deleteMin(), insert() and decrease() violated, a handle of another
// queue refused, a decrease() to the same priority and to the least int
// allowed, the capacities 0 and 1, and random operations against
// std::multiset, each checking the order of the heap
ADS_TEST( "heap-priority-queue" )
{
 CHECK_THROWS( HeapPriorityQueue< int >( -1 ) , contract_violation );
 HeapPriorityQueue< int > Q( 3 ) , O( 3 );
 CHECK_THROWS( (void) Q.min() , contract_violation );
 CHECK_THROWS( (void) Q.minPriority() , contract_violation );
 CHECK_THROWS( Q.deleteMin() , contract_violation );
 auto h = Q.insert( 1 , 5 );
 Q.insert( 2 , 3 );
 Q.insert( 3 , 4 );
 CHECK_THROWS( Q.insert( 4 , 1 ) , contract_violation );
 CHECK_THROWS( Q.decrease( h , 9 ) , contract_violation );
 auto foreign = O.insert( 9 , 9 );
 CHECK_THROWS( Q.decrease( foreign , 0 ) , contract_violation );
 Q.decrease( h , 1 );
 CHECK( Q.min() == 1 && Q.minPriority() == 1 );
 Q.decrease( h , 1 );                         // the same priority: inside
 Q.decrease( h , std::numeric_limits< int >::min() );
 CHECK( Q.min() == 1 && Q.size() == 3 );
 HeapPriorityQueue< int > Z( 0 );             // capacity 0: never insert
 CHECK_THROWS( Z.insert( 1 , 1 ) , contract_violation );
 CHECK_THROWS( Z.deleteMin() , contract_violation );
 HeapPriorityQueue< int > C( 1 );             // capacity 1: full, emptied
 C.insert( 5 , 5 );
 CHECK_THROWS( C.insert( 6 , 6 ) , contract_violation );
 CHECK( C.deleteMin() == 5 && C.empty() );
 C.insert( 6 , 6 );
 CHECK( C.min() == 6 );
 std::mt19937 g( 2 );
 for( int d = 0 ; d < 3 ; ++d ) {
  HeapPriorityQueue< int , int , 4 > H( 500 );
  std::multiset< int > M;
  std::vector< HeapPriorityQueue< int , int , 4 >::Handle > handles;
  std::vector< int > prio;
  for( int op = 0 ; op < 2000 ; ++op ) {
   const int c = int( g() % 4 );
   if( ( c < 2 ) && ( H.size() < H.capacity() ) ) {
    const int p = int( g() % 1000 );
    handles.push_back( H.insert( int( handles.size() ) , p ) );
    prio.push_back( p );
    M.insert( p );
    }
   else if( ( c == 2 ) && ! M.empty() ) {
    const int v = H.min() , p = H.minPriority();
    CHECK( p == *M.begin() );
    CHECK( H.deleteMin() == v );
    M.erase( M.begin() );
    handles[ v ] = nullptr;
    }
   else if( ( c == 3 ) && ! M.empty() ) {
    int v = int( g() % handles.size() );
    if( ! handles[ v ] ) continue;
    const int p = prio[ v ] - int( g() % 50 );
    M.erase( M.find( prio[ v ] ) );
    M.insert( p );
    H.decrease( handles[ v ] , p );
    prio[ v ] = p;
    }
   }
  CHECK( H.size() == int( M.size() ) );
  }
 }

// a random sequence of insert(), deleteMin() and decrease() on the queue
// q, made by make(), against std::multiset; decrease() on the handles that
// are still in the queue
template< class Q , class Make >
void against_multiset( Make make , unsigned seed , int ops )
{
 std::mt19937 g( seed );
 Q q = make();
 std::multiset< int > M;
 std::vector< typename Q::Handle > handles;
 std::vector< int > prio;
 std::vector< bool > in;
 for( int op = 0 ; op < ops ; ++op ) {
  const int c = int( g() % 4 );
  if( c < 2 ) {
   const int p = int( g() % 1000 );
   handles.push_back( q.insert( int( handles.size() ) , p ) );
   prio.push_back( p );
   in.push_back( true );
   M.insert( p );
   }
  else if( ( c == 2 ) && ! M.empty() ) {
   const int v = q.min();
   CHECK( prio[ v ] == *M.begin() );
   CHECK( q.deleteMin() == v );
   M.erase( M.begin() );
   in[ v ] = false;
   }
  else if( ( c == 3 ) && ! M.empty() ) {
   const int v = int( g() % handles.size() );
   if( ! in[ v ] ) continue;
   const int p = prio[ v ] - int( g() % 50 );
   M.erase( M.find( prio[ v ] ) );
   M.insert( p );
   q.decrease( handles[ v ] , p );
   prio[ v ] = p;
   }
  }
 CHECK( q.size() == int( M.size() ) );
 }

// PRIORITYQUEUE on the sorted and the unsorted list: the preconditions of
// min(), deleteMin() and decrease() violated, a cell of another queue
// refused, a decrease() to the same priority allowed, and random
// operations against std::multiset
ADS_TEST( "list-priority-queue" )
{
 ListPriorityQueue< int , int , true > Q , O;
 CHECK_THROWS( (void) Q.min() , contract_violation );
 CHECK_THROWS( Q.deleteMin() , contract_violation );
 auto h = Q.insert( 1 , 5 );
 Q.insert( 2 , 3 );
 CHECK_THROWS( Q.decrease( h , 9 ) , contract_violation );
 auto foreign = O.insert( 9 , 9 );
 CHECK_THROWS( Q.decrease( foreign , 0 ) , contract_violation );
 Q.decrease( h , 1 );
 CHECK( Q.min() == 1 && Q.deleteMin() == 1 && Q.min() == 2 );
 ListPriorityQueue< int , int , false > U;
 CHECK_THROWS( U.deleteMin() , contract_violation );
 CHECK_THROWS( (void) U.min() , contract_violation );
 auto u = U.insert( 4 , 4 );
 U.decrease( u , 4 );                         // the same priority: inside
 CHECK_THROWS( U.decrease( u , 5 ) , contract_violation );
 CHECK( U.deleteMin() == 4 && U.empty() );
 against_multiset< ListPriorityQueue< int , int , true > >(
  [] { return( ListPriorityQueue< int , int , true >() ); } , 3 , 1000 );
 against_multiset< ListPriorityQueue< int , int , false > >(
  [] { return( ListPriorityQueue< int , int , false >() ); } , 4 , 1000 );
 }

// PRIORITYQUEUE on a Red-Black tree: the preconditions violated, a handle
// already removed refused, the handles -1 and n just outside, a decrease()
// to the same priority, and random operations against std::multiset
ADS_TEST( "tree-priority-queue" )
{
 TreePriorityQueue< int > Q;
 CHECK_THROWS( (void) Q.min() , contract_violation );
 CHECK_THROWS( Q.deleteMin() , contract_violation );
 const auto h = Q.insert( 1 , 5 ) , k = Q.insert( 2 , 3 );
 CHECK_THROWS( Q.decrease( h , 9 ) , contract_violation );
 CHECK_THROWS( Q.decrease( 7 , 0 ) , contract_violation );
 CHECK_THROWS( Q.decrease( -1 , 0 ) , contract_violation );
 CHECK_THROWS( Q.decrease( 2 , 0 ) , contract_violation );   // just out
 Q.decrease( k , 3 );                         // the same priority
 CHECK( Q.deleteMin() == 2 );
 CHECK_THROWS( Q.decrease( k , 0 ) , contract_violation );
 Q.decrease( h , 1 );
 CHECK( Q.min() == 1 && Q.size() == 1 );
 against_multiset< TreePriorityQueue< int > >(
  [] { return( TreePriorityQueue< int >() ); } , 5 , 2000 );
 }

// PRIORITYQUEUE on a Fibonacci heap: the preconditions violated, a node of
// another heap and nil refused, and random operations against
// std::multiset, each
// checking the trees at the level 3
ADS_TEST( "fibonacci-heap-priority-queue" )
{
 FibonacciHeapPriorityQueue< int > Q , O;
 CHECK_THROWS( (void) Q.min() , contract_violation );
 CHECK_THROWS( (void) Q.minPriority() , contract_violation );
 CHECK_THROWS( Q.deleteMin() , contract_violation );
 auto h = Q.insert( 1 , 5 );
 Q.insert( 2 , 3 );
 Q.insert( 3 , 4 );
 CHECK_THROWS( Q.decrease( h , 9 ) , contract_violation );
 auto foreign = O.insert( 9 , 9 );
 CHECK_THROWS( Q.decrease( foreign , 0 ) , contract_violation );
 CHECK_THROWS( Q.decrease( nullptr , 0 ) , contract_violation );
 CHECK( Q.deleteMin() == 2 );                 // the trees are consolidated
 Q.decrease( h , 1 );
 CHECK( Q.min() == 1 && Q.minPriority() == 1 && Q.size() == 2 );
 for( unsigned s = 6 ; s < 9 ; ++s )
  against_multiset< FibonacciHeapPriorityQueue< int > >(
   [] { return( FibonacciHeapPriorityQueue< int >() ); } , s , 2000 );
 }

// PRIORITYQUEUE on a heap with pointers: the preconditions violated, and
// random insertions and deletions against std::multiset, each checking the
// order and the shape of the heap
template< class Q >
void linked_heap( unsigned seed )
{
 Q H;
 CHECK_THROWS( (void) H.min() , contract_violation );
 CHECK_THROWS( (void) H.minPriority() , contract_violation );
 CHECK_THROWS( H.deleteMin() , contract_violation );
 std::mt19937 g( seed );
 std::multiset< int > M;
 for( int op = 0 ; op < 3000 ; ++op )
  if( M.empty() || ( g() % 3 ) ) {
   const int p = int( g() % 1000 );
   H.insert( p , p );
   M.insert( p );
   }
  else {
   CHECK( H.minPriority() == *M.begin() && H.deleteMin() == *M.begin() );
   M.erase( M.begin() );
   }
 CHECK( H.size() == int( M.size() ) );
 while( ! M.empty() ) {                     // down to the empty heap
  CHECK( H.deleteMin() == *M.begin() );
  M.erase( M.begin() );
  }
 CHECK( H.empty() );
 H.insert( 1 , 1 );
 CHECK( H.min() == 1 );
 }

ADS_TEST( "linked-heap-priority-queue" )
{
 linked_heap< LinkedHeapPriorityQueue< int > >( 9 );
 linked_heap< LinkedHeapPriorityQueue< int , int , true > >( 10 );
 }

// the axioms of PRIORITYQUEUE on every realization, the items being their
// own priorities: min( insert( x , create ) ) = x,
// deleteMin( insert( x , create ) ) = create and
// min( insert( x , Q ) ) = min( x , min( Q ) ); the realizations accept
// equal priorities, unlike the specification of a set of items with
// distinct priorities
template< class Q , class F >
void priority_axioms( F create , unsigned seed )
{
 std::mt19937 g( seed );
 Q E = create();
 E.insert( 7 , 7 );
 CHECK( E.min() == 7 );
 CHECK( E.deleteMin() == 7 && E.empty() );
 Q A = create();
 int m = 1 << 30;
 for( int t = 0 ; t < 200 ; ++t ) {
  const int x = int( g() % 50 );
  A.insert( x , x );
  m = std::min( m , x );
  CHECK( A.min() == m );
  if( g() % 3 == 0 ) {
   CHECK( A.deleteMin() == m );
   m = A.empty() ? 1 << 30 : A.min();
   }
  }
 }

ADS_TEST( "priority-axioms" )
{
 using H = HeapPriorityQueue< int , int >;
 using H4 = HeapPriorityQueue< int , int , 4 >;
 using LS = ListPriorityQueue< int , int , true >;
 using LU = ListPriorityQueue< int , int , false >;
 using T = TreePriorityQueue< int , int >;
 using FH = FibonacciHeapPriorityQueue< int , int >;
 using LH = LinkedHeapPriorityQueue< int , int >;
 using LL = LinkedHeapPriorityQueue< int , int , true >;
 priority_axioms< H >( [] { return( H( 300 ) ); } , 1 );
 priority_axioms< H4 >( [] { return( H4( 300 ) ); } , 2 );
 priority_axioms< LS >( [] { return( LS() ); } , 3 );
 priority_axioms< LU >( [] { return( LU() ); } , 4 );
 priority_axioms< T >( [] { return( T() ); } , 5 );
 priority_axioms< FH >( [] { return( FH() ); } , 6 );
 priority_axioms< LH >( [] { return( LH() ); } , 7 );
 priority_axioms< LL >( [] { return( LL() ); } , 8 );
 }

// the bucket queue: f , g >= 0 (0 just inside, -1 just outside), the
// readings and the removals of an empty queue, ties of f broken by the
// largest g, a queue emptied and filled again; the D-ary heap of 2 and 3
// children: top() and pop() on an empty heap, one item, equal items; the
// loser tree: no player refused, one player, every player exhausted, a
// comparator that is not an order caught by the invariant; kWayMerge()
// refusing an unsorted run, no run, empty runs
ADS_TEST( "bucket-dary-loser" )
{
 BucketQueue< int > B;
 CHECK_THROWS( B.push( 1 , -1 , 0 ) , contract_violation );
 CHECK_THROWS( B.push( 1 , 0 , -1 ) , contract_violation );
 CHECK_THROWS( (void) B.top() , contract_violation );
 CHECK_THROWS( (void) B.topF() , contract_violation );
 CHECK_THROWS( (void) B.topG() , contract_violation );
 CHECK_THROWS( B.pop() , contract_violation );
 CHECK_THROWS( B.popEntry() , contract_violation );
 B.push( 1 , 0 , 0 );
 B.push( 2 , 0 , 5 );
 B.push( 3 , 4 , 9 );
 CHECK( B.top() == 2 && B.topF() == 0 && B.topG() == 5 );
 CHECK( B.pop() == 2 && B.pop() == 1 );
 const auto e = B.popEntry();
 CHECK( e.f == 4 && e.g == 9 && B.empty() );
 CHECK_THROWS( B.pop() , contract_violation );
 B.push( 7 , 2 , 2 );                         // filled again, below
 CHECK( B.top() == 7 && B.size() == 1 );
 B.clear();
 CHECK_THROWS( (void) B.top() , contract_violation );

 DaryHeap< int , std::less< int > , 2 > H2;
 DaryHeap< int , std::greater< int > , 3 > H3;
 CHECK_THROWS( (void) H2.top() , contract_violation );
 CHECK_THROWS( H3.pop() , contract_violation );
 H2.push( 5 );
 CHECK( H2.top() == 5 && H2.pop() == 5 && H2.empty() );
 CHECK_THROWS( H2.pop() , contract_violation );
 for( int x : { 4 , 4 , 4 , std::numeric_limits< int >::min() ,
                std::numeric_limits< int >::max() } ) {
  H2.push( x );
  H3.push( x );
  }
 CHECK( H2.pop() == std::numeric_limits< int >::min() &&
        H3.pop() == std::numeric_limits< int >::max() );
 CHECK( H2.pop() == 4 && H3.pop() == 4 && H2.size() == 3 );

 using O = std::vector< std::optional< int > >;
 CHECK_THROWS( LoserTree< int >( O{} ) , contract_violation );
 LoserTree< int > L1( O{ 3 } );               // one player
 CHECK( L1.top() == 3 && L1.winner() == 0 );
 L1.replace( std::nullopt );
 CHECK( L1.empty() );
 CHECK_THROWS( (void) L1.top() , contract_violation );
 CHECK_THROWS( (void) L1.winner() , contract_violation );
 CHECK_THROWS( L1.replace( 1 ) , contract_violation );
 LoserTree< int > L0( O{ std::nullopt , std::nullopt } );
 CHECK( L0.empty() );
 const auto always = []( int , int ) { return( true ); };
 CHECK_THROWS( ( LoserTree< int , decltype( always ) >( O{ 1 , 2 } ,
                                                        always ) ) ,
               contract_violation );
 using R = std::vector< std::vector< int > >;
 CHECK( kWayMerge( R{} ).empty() && kWayMerge( R{ {} , {} } ).empty() );
 CHECK( kWayMerge( R{ { 2 , 2 } , {} , { 1 } } ) ==
        std::vector< int >( { 1 , 2 , 2 } ) );
 CHECK_THROWS( (void) kWayMerge( R{ { 1 } , { 3 , 2 } } ) ,
               contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- End File contract_priority.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
