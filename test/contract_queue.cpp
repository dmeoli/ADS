/*--------------------------------------------------------------------------*/
/*------------------------ File contract_queue.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the queues: every operation of
 * the specification of the book, its result as the specification states it,
 * and the violations of its preconditions, which must throw
 * ADS::contract_violation. The tests are compiled at the level 3, so that each
 * operation also checks its postconditions and the invariant of the structure,
 * on random sequences of operations against a reference of the standard
 * library.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.3
 *      and exercise 4.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"

#include <cstddef>
#include <deque>
#include <random>
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

// QUEUE on ArrayQueue: the preconditions of the book, a queue not empty
// for top() and dequeue() and not full for enqueue(), and random
// operations going round the circular vector against std::deque
ADS_TEST( "array-queue" )
{
 ArrayQueue< int > Q( 2 );
 CHECK_THROWS( (void) Q.top() , contract_violation );
 CHECK_THROWS( Q.dequeue() , contract_violation );
 Q.enqueue( 1 );
 Q.enqueue( 2 );
 CHECK_THROWS( Q.enqueue( 3 ) , contract_violation );
 CHECK( Q.dequeue() == 1 && Q.top() == 2 && Q.size() == 1 );
 ArrayQueue< int > Z( 0 );
 CHECK_THROWS( Z.enqueue( 0 ) , contract_violation );
 std::mt19937 g( 6 );
 ArrayQueue< int > A( 7 );
 std::deque< int > R;
 for( int op = 0 ; op < 2000 ; ++op )
  if( ( R.size() < A.capacity() ) && ( R.empty() || g() % 2 ) ) {
   A.enqueue( op );
   R.push_back( op );
   }
  else {
   CHECK( A.top() == R.front() && A.dequeue() == R.front() );
   R.pop_front();
   }
 CHECK( A.size() == R.size() );
 }

// QUEUE on LinkedQueue: top() and dequeue() on an empty queue, random
// operations against std::deque, and a copy independent of the original
ADS_TEST( "linked-queue" )
{
 LinkedQueue< int > Q;
 CHECK_THROWS( (void) Q.top() , contract_violation );
 CHECK_THROWS( Q.dequeue() , contract_violation );
 std::mt19937 g( 8 );
 std::deque< int > R;
 for( int op = 0 ; op < 2000 ; ++op )
  if( R.empty() || g() % 2 ) {
   Q.enqueue( op );
   R.push_back( op );
   }
  else {
   CHECK( Q.top() == R.front() && Q.dequeue() == R.front() );
   R.pop_front();
   }
 CHECK( Q.size() == R.size() );
 LinkedQueue< int > C = Q;
 if( ! C.isEmpty() ) C.dequeue();
 C.enqueue( -1 );
 CHECK( Q.size() == R.size() && !( C == Q ) );
 LinkedQueue< int > M( std::move( C ) );
 CHECK( C.isEmpty() && ! M.isEmpty() );
 }

// the axioms of QUEUE [see ADT.h] on ArrayQueue and LinkedQueue, for queues
// Q built by random operations: isEmpty( create ) = true,
// top( enqueue( create , v ) ) = v, and for Q not empty
// top( enqueue( Q , v ) ) = top( Q ) and dequeue( enqueue( Q , v ) ) =
// enqueue( dequeue( Q ) , v ), the two sides built from the same
// operations and compared by emptying them
template< class Q , class F >
void queue_axioms( F create , unsigned seed )
{
 std::mt19937 g( seed );
 Q E = create();
 CHECK( E.isEmpty() );
 E.enqueue( 5 );
 CHECK( E.top() == 5 );
 std::vector< int > ops;                        // -1 is a dequeue()
 for( int t = 0 ; t < 100 ; ++t ) {
  const int v = int( g() % 1000 );
  auto build = [ & ]( void ) {
   Q q = create();
   for( int o : ops )
    if( o < 0 ) q.dequeue(); else q.enqueue( o );
   return( q );
   };
  Q a = build() , b = build();
  if( ! a.isEmpty() ) {
   const int front = a.top();
   a.enqueue( v );
   CHECK( a.top() == front );
   a.dequeue();
   b.dequeue();
   b.enqueue( v );
   while( ! a.isEmpty() && ! b.isEmpty() )
    CHECK( a.dequeue() == b.dequeue() );
   CHECK( a.isEmpty() && b.isEmpty() );
   }
  int size = 0;
  for( int o : ops ) size += o < 0 ? -1 : 1;
  ops.push_back( ( size > 0 ) && ( g() % 3 == 0 ) ? -1 : v );
  }
 }

ADS_TEST( "queue-axioms" )
{
 queue_axioms< ArrayQueue< int > >(
  [] { return( ArrayQueue< int >( 200 ) ); } , 9 );
 queue_axioms< LinkedQueue< int > >( [] { return( LinkedQueue< int >() ); } ,
                                     10 );
 }

// DEQUEUE: the four removals and readings on an empty dequeue, and random
// operations at both ends against std::deque
ADS_TEST( "dequeue" )
{
 Dequeue< int > D;
 CHECK_THROWS( D.removeHead() , contract_violation );
 CHECK_THROWS( D.removeTail() , contract_violation );
 CHECK_THROWS( (void) D.head() , contract_violation );
 CHECK_THROWS( (void) D.tail() , contract_violation );
 std::mt19937 g( 7 );
 std::deque< int > R;
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int c = int( g() % 4 );
  if( R.empty() || ( c == 0 ) ) {
   D.insertHead( op );
   R.push_front( op );
   }
  else if( c == 1 ) {
   D.insertTail( op );
   R.push_back( op );
   }
  else if( c == 2 ) {
   CHECK( D.head() == R.front() && D.removeHead() == R.front() );
   R.pop_front();
   }
  else {
   CHECK( D.tail() == R.back() && D.removeTail() == R.back() );
   R.pop_back();
   }
  }
 CHECK( D.isEmpty() == R.empty() );
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
/*---------------------- End File contract_queue.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
