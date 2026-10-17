/*--------------------------------------------------------------------------*/
/*--------------------------- File contract.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h]: for each structure, every
 * operation of the specification of the book, its result as the
 * specification states it, and the violations of its preconditions, which
 * must throw ADS::contract_violation. The tests are compiled at the level
 * 3, so that each operation also checks its postconditions and the
 * invariant of the structure, on random sequences of operations against a
 * reference of the standard library.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2 and
 *      4.1, https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf
 *      and https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/sequence/List.h"

#include <iterator>
#include <list>
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

// SEQUENCE on List: the operations as the specification states them, the
// preconditions violated, and random insertions and removals against
// std::list, each checking its postcondition and the invariant
ADS_TEST( "list" )
{
 List< int > L;
 CHECK( L.isEmpty() && L.finished( L.head() ) && L.finished( L.tail() ) );
 const auto p = L.insert( L.next( L.tail() ) , 1 );   // insert at pos_n
 CHECK( ! L.isEmpty() && L.head() == p && L.tail() == p && L.read( p ) == 1 );
 const auto q = L.insert( p , 0 );                    // the predecessor of p
 CHECK( L.next( q ) == p && L.prev( p ) == q && L.head() == q );
 const auto r = L.remove( q );                        // the successor of q
 CHECK( r == p && L.head() == p && L.finished( L.prev( p ) ) );
 L.write( p , 7 );
 CHECK( L.read( p ) == 7 );
 const auto end = L.next( L.tail() );                 // the sentinel
 CHECK_THROWS( (void) L.read( end ) , contract_violation );
 CHECK_THROWS( L.write( end , 0 ) , contract_violation );
 CHECK_THROWS( L.remove( end ) , contract_violation );
 CHECK_THROWS( L.insert( nullptr , 0 ) , contract_violation );
 CHECK_THROWS( (void) L.next( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 List< int > A;
 std::list< int > R;
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int k = R.empty() ? 0 : int( g() % ( R.size() + 1 ) );
  auto pa = A.head();
  auto pr = R.begin();
  for( int i = 0 ; i < k ; ++i , pa = A.next( pa ) , ++pr ) {}
  if( ( g() % 3 ) || R.empty() ) {
   A.insert( pa , op );
   R.insert( pr , op );
   }
  else if( pr != R.end() ) {
   A.remove( pa );
   R.erase( pr );
   }
  }
 std::vector< int > a;
 for( auto x = A.head() ; ! A.finished( x ) ; x = A.next( x ) )
  a.push_back( A.read( x ) );
 CHECK( a == std::vector< int >( R.begin() , R.end() ) );
 }

// PRIORITYQUEUE on HeapPriorityQueue: the preconditions of min(),
// deleteMin(), insert() and decrease() violated, a handle of another
// queue refused, and random operations against std::multiset, each
// checking the order of the heap
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

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File contract.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
