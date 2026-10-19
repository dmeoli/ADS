/*--------------------------------------------------------------------------*/
/*----------------------------- File ch04.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 4, Strutture di dati elementari: the realizations of the
 * SEQUENCE (List, ArraySequence and the singly linked list of Esercizio
 * 4.6) are checked against the same specification, then the STACK and the
 * QUEUE with vectors, the examples of the text (the catalogue of Don
 * Giovanni, the binary counter, the towers of Hanoi) and the exercises 4.1
 * to 4.11, each a test named after it. Each test cites the place of the book
 * it comes from.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 4,
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

#include "ADS/ADT.h"
#include "ADS/algorithm/BinaryCounter.h"
#include "ADS/algorithm/Hanoi.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sort/PigeonholeSort.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/SequenceStack.h"

#include <algorithm>
#include <bit>
#include <deque>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*--------------------------- REALIZATIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

static_assert( Sequence< List< int > > );
static_assert( Sequence< ArraySequence< int > > );
static_assert( Sequence< SinglyList< int > > );
static_assert( Sequence< CursorList< int > > );
static_assert( Stack< ArrayStack< int > > );
static_assert( Stack< SequenceStack< List< int > > > );
static_assert( Queue< ArrayQueue< int > > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the items of a sequence, in their order
template< Sequence S >
std::vector< typename S::Item > items( const S & s )
{
 std::vector< typename S::Item > v;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  v.push_back( s.read( p ) );
 return( v );
 }

/// the items of a sequence, from the last one to the first one
template< Sequence S >
std::vector< typename S::Item > items_backwards( const S & s )
{
 std::vector< typename S::Item > v;
 for( auto p = s.tail() ; ! s.finished( p ) ; p = s.prev( p ) )
  v.push_back( s.read( p ) );
 return( v );
 }

/// appends v, i.e., inserts it in the position after the last item
template< Sequence S >
void append( S & s , const typename S::Item & v )
{
 s.insert( s.next( s.tail() ) , v );
 }

/// the specification of SEQUENCE [section 3.2.1] checked on a realization;
/// the edges: the empty sequence and the sequence of one item, every
/// position of short sequences, the positions past the ends, the
/// preconditions, the copies, and a long run of random insertions and
/// removals against std::vector, down to empty and up again
template< Sequence S >
void sequence_spec( void )
{
 using V = std::vector< int >;

 // the empty sequence: head() and tail() are pos_n and pos_-1
 S s;
 CHECK( s.empty() );
 CHECK( s.finished( s.head() ) );
 CHECK( s.finished( s.tail() ) );

 // insert( pos_n , v ) appends, insert( pos_0 , v ) prepends
 append( s , 2 );
 CHECK( ! s.empty() );
 append( s , 3 );
 s.insert( s.head() , 1 );
 CHECK( items( s ) == V( { 1 , 2 , 3 } ) );
 CHECK( items_backwards( s ) == V( { 3 , 2 , 1 } ) );

 // next() of the last item and prev() of the first one are past the ends
 CHECK( s.finished( s.next( s.tail() ) ) );
 CHECK( s.finished( s.prev( s.head() ) ) );

 // insert( pos_i , v ) puts v at index i and returns its position
 auto p = s.next( s.head() );   // pos_1
 p = s.insert( p , 9 );
 CHECK( s.read( p ) == 9 );
 CHECK( items( s ) == V( { 1 , 9 , 2 , 3 } ) );

 // write() and read()
 s.write( p , 8 );
 CHECK( s.read( p ) == 8 );

 // remove( pos_i ) returns the position of the new item at index i
 p = s.remove( p );
 CHECK( s.read( p ) == 2 );
 CHECK( items( s ) == V( { 1 , 2 , 3 } ) );

 // removing the last item gives pos_n
 p = s.remove( s.tail() );
 CHECK( s.finished( p ) );
 CHECK( items( s ) == V( { 1 , 2 } ) );

 // removing all the items from the head leaves the empty sequence
 p = s.head();
 while( ! s.finished( p ) )
  p = s.remove( p );
 CHECK( s.empty() );
 CHECK( items( s ).empty() );

 // the sequence works again after being emptied, also with duplicates
 for( int i : { 5 , 5 , 7 } )
  append( s , i );
 CHECK( items( s ) == V( { 5 , 5 , 7 } ) );

 // the specification as axioms, at every position of sequences of length
 // 0 to 5: empty() is finished( head() ), so that it is redundant;
 // read( insert( p , a ) ) = a; remove( insert( p , a ) ) leaves the
 // sequence as it was; read( write( p , a ) ) = a, the other items
 // unchanged
 S t;
 for( int n = 0 ; n <= 5 ; ++n ) {
  CHECK( t.empty() == t.finished( t.head() ) );
  const V before = items( t );
  int i = 0;
  for( auto q = t.head() ; ; q = t.next( q ) , ++i ) {
   const auto r = t.insert( q , 100 + i );
   CHECK( t.read( r ) == 100 + i && ! t.empty() );
   t.remove( r );
   CHECK( items( t ) == before );
   if( t.finished( q ) )
    break;
   const auto old = t.read( q );
   t.write( q , -1 );
   V w = before;
   w[ i ] = -1;
   CHECK( t.read( q ) == -1 && items( t ) == w );
   t.write( q , old );
   }
  append( t , n );
  }

 // the preconditions: read(), write() and remove() of a position with no
 // item, pos_n of the empty sequence and of one of length 1 and 6
 S e;
 CHECK_THROWS( (void) e.read( e.head() ) , contract_violation );
 CHECK_THROWS( e.write( e.head() , 1 ) , contract_violation );
 CHECK_THROWS( (void) e.remove( e.head() ) , contract_violation );
 append( e , 4 );
 CHECK_THROWS( (void) e.read( e.next( e.tail() ) ) , contract_violation );
 CHECK_THROWS( (void) e.remove( e.next( e.tail() ) ) , contract_violation );
 CHECK( e.read( e.head() ) == 4 && e.head() == e.tail() );   // one item
 CHECK( s.finished( s.prev( s.head() ) ) );
 CHECK_THROWS( (void) t.read( t.next( t.tail() ) ) , contract_violation );
 CHECK_THROWS( t.write( t.next( t.tail() ) , 1 ) , contract_violation );

 // a copy is equal and independent, also of the empty sequence
 S c = t;
 CHECK( c == t && items( c ) == items( t ) );
 c.write( c.head() , -5 );
 CHECK( ! ( c == t ) && t.read( t.head() ) == 0 );
 const S z0 , z1 = z0;
 CHECK( z1 == z0 && z1.empty() );

 // against std::vector, random insertions and removals at the head, in
 // the middle and at the end, growing to 300 items and back to empty
 std::mt19937 g( 11 );
 V o;
 S r;
 for( int step = 0 ; step < 1200 ; ++step ) {
  const bool grow = ( step < 600 ) ? ( g() % 4 != 0 ) : ( g() % 4 == 0 );
  if( grow || o.empty() ) {
   const std::size_t i = g() % ( o.size() + 1 );
   auto p = r.head();
   for( std::size_t j = 0 ; j < i ; ++j ) p = r.next( p );
   CHECK( r.read( r.insert( p , step ) ) == step );
   o.insert( o.begin() + i , step );
   }
  else {
   const std::size_t i = g() % o.size();
   auto p = r.head();
   for( std::size_t j = 0 ; j < i ; ++j ) p = r.next( p );
   p = r.remove( p );
   o.erase( o.begin() + i );
   CHECK( ( i == o.size() ) ? r.finished( p ) : ( r.read( p ) == o[ i ] ) );
   }
  if( step % 50 == 0 )
   CHECK( items( r ) == o );
  }
 while( ! r.empty() ) { r.remove( r.head() ); o.erase( o.begin() ); }
 CHECK( items( r ).empty() && o.empty() );
 append( r , 1 );
 CHECK( items( r ) == V( { 1 } ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- REALIZATIONS OF SEQUENCE --------------------------*/
/*--------------------------------------------------------------------------*/

// section 4.1, the LIST with pointers
ADS_TEST( "spec.List" )
{
 sequence_spec< List< int > >();
 sequence_spec< List< int , false > >();       // the cells with no pool
 }

// the list of section 4.1 on its pool: random insertions and removals at
// random positions against std::deque, the cells removed taken again by
// the insertions, the walk backwards and std::reverse on the iterators;
// the edges: the empty list, one item in and out, copies, moves and swaps
// between lists of different pools whose cells stay valid and are freed
// by the list that holds them, a list moved from and used again, a
// self-swap, and items with a destructor (std::string, beyond the
// short-string buffer) left in the list when it dies
ADS_TEST( "sec4.1.pool" )
{
 std::mt19937 g( 41 );
 List< std::string > L;
 std::deque< std::string > R;
 CHECK( L.empty() && L.begin() == L.end() );
 L.remove( L.insert( L.head() , std::string( 30 , 'x' ) ) );
 CHECK( L.empty() && L.finished( L.head() ) );
 for( int op = 0 ; op < 20000 ; ++op ) {
  const std::size_t k = R.empty() ? 0 : g() % ( R.size() + 1 );
  auto p = L.head();
  for( std::size_t i = 0 ; i < k ; ++i ) p = L.next( p );
  if( R.empty() || ( g() % 5 < 3 ) ) {
   const std::string v( 20 + g() % 20 , char( 'a' + op % 26 ) );
   L.insert( p , v );
   R.insert( R.begin() + std::ptrdiff_t( k ) , v );
   }
  else if( k < R.size() ) {
   L.remove( p );
   R.erase( R.begin() + std::ptrdiff_t( k ) );
   }
  if( op % 4000 == 0 )
   CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
  }
 CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
 CHECK( std::equal( L.rbegin() , L.rend() , R.rbegin() , R.rend() ) );
 std::reverse( L.begin() , L.end() );
 std::reverse( R.begin() , R.end() );
 CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
 List< std::string > C( L );                   // a copy, a pool of its own
 List< std::string > M( std::move( L ) );      // the cells move along
 CHECK( L.empty() && C == M );
 L.push_back( "again" );                       // the list moved from
 CHECK( L.read( L.head() ) == "again" && L.head() == L.tail() );
 L.swap( C );
 CHECK( L == M && C.read( C.head() ) == "again" );
 L.swap( L );
 CHECK( L == M );
 C = M;
 M.remove( M.head() );
 CHECK( C == L && !( M == L ) );
 List< int > I;
 for( int i = 0 ; i < 100000 ; ++i ) I.push_back( i );
 int expect = 0;
 bool ok = true;
 for( int x : I ) ok = ok && ( x == expect++ );
 CHECK( ok && expect == 100000 );
 }

// section 4.2, the vector
ADS_TEST( "spec.ArraySequence" ) { sequence_spec< ArraySequence< int > >(); }

// Esercizio 4.6, the singly linked list with no sentinel
ADS_TEST( "ex4.6" ) { sequence_spec< SinglyList< int > >(); }

// the singly linked list on its pool: random insertions and removals at
// random positions against std::deque, the cells removed taken again by
// the insertions; the edges: the empty list, one item in and out, copies,
// moves and swaps between lists of different pools whose cells stay valid
// and are freed by the list that holds them, a list moved from and used
// again, and items with a destructor (std::string, beyond the short-string
// buffer) left in the list when it dies
ADS_TEST( "ex4.6.pool" )
{
 std::mt19937 g( 46 );
 SinglyList< std::string > L;
 std::deque< std::string > R;
 CHECK( L.empty() && L.begin() == L.end() );
 L.insert( L.head() , std::string( 30 , 'x' ) );
 L.remove( L.head() );
 CHECK( L.empty() );
 for( int op = 0 ; op < 20000 ; ++op ) {
  const std::size_t k = R.empty() ? 0 : g() % ( R.size() + 1 );
  auto p = L.head();
  for( std::size_t i = 0 ; i < k ; ++i ) p = L.next( p );
  if( R.empty() || ( g() % 5 < 3 ) ) {
   const std::string v( 20 + g() % 20 , char( 'a' + op % 26 ) );
   L.insert( p , v );
   R.insert( R.begin() + std::ptrdiff_t( k ) , v );
   }
  else if( k < R.size() ) {
   L.remove( p );
   R.erase( R.begin() + std::ptrdiff_t( k ) );
   }
  if( op % 4000 == 0 )
   CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
  }
 CHECK( std::equal( L.begin() , L.end() , R.begin() , R.end() ) );
 SinglyList< std::string > C( L );             // a copy, a pool of its own
 SinglyList< std::string > M( std::move( L ) );  // the cells move along
 CHECK( L.empty() && C == M );
 L.insert( L.head() , "again" );                // the list moved from
 CHECK( L.read( L.head() ) == "again" );
 L.swap( C );
 CHECK( L == M && C.read( C.head() ) == "again" );
 C = M;
 M.remove( M.head() );
 CHECK( C == L && !( M == L ) );
 SinglyList< int > I;
 for( int i = 0 ; i < 100000 ; ++i ) I.push_front( i );
 int expect = 99999;
 bool ok = true;
 for( int x : I ) ok = ok && ( x == expect-- );
 CHECK( ok && expect == -1 );
 }

// beyond the book, the SEQUENCE with cursors
ADS_TEST( "spec.CursorList" ) { sequence_spec< CursorList< int > >(); }

// section 4.2: the vector doubles, so its size is always a power of 2 and
// less than twice the number of items; the edges: the empty sequence has
// no vector, the first insertion makes one of one place, the removals
// halve it when it is a quarter full (between n and 4 n places, none when
// empty again), a sequence moved from is left with no vector, and the
// insertions and removals at the boundary of a resize, back and forth
ADS_TEST( "sec4.2.doubling" )
{
 ArraySequence< int > s;
 CHECK( s.capacity() == 0 );
 for( int i = 1 ; i <= 1000 ; ++i ) {
  append( s , i );
  CHECK( s.size() == i );
  CHECK( ( s.capacity() & ( s.capacity() - 1 ) ) == 0 );
  CHECK( s.capacity() < 2 * i );
  }
 CHECK( s.capacity() == 1024 );
 for( int i = 999 ; i >= 0 ; --i ) {
  s.remove( s.tail() );
  CHECK( s.size() == i );
  CHECK( ( s.capacity() & ( s.capacity() - 1 ) ) == 0 );
  CHECK( s.capacity() >= i && ( i == 0 || s.capacity() <= 4 * i ) );
  }
 CHECK( s.capacity() == 0 && s.empty() );
 append( s , 7 );
 CHECK( s.capacity() == 1 );
 for( int i = 0 ; i < 8 ; ++i ) append( s , i );      // 9 items, 16 places
 CHECK( s.capacity() == 16 );
 for( int t = 0 ; t < 10 ; ++t ) {                   // across 4 and 5 items
  while( s.size() > 4 ) s.remove( s.head() );
  CHECK( s.capacity() == 8 );
  append( s , t );
  CHECK( s.capacity() == 8 );
  }
 ArraySequence< int > m = std::move( s );
 CHECK( s.capacity() == 0 && s.empty() && m.size() == 5 );
 append( s , 1 );
 CHECK( s.size() == 1 && s.capacity() == 1 );
 }

// section 4.2.1: n increments of a counter of k bits change fewer than 2n
// bits, and the counter goes back to 0 after 2^k of them; the edges: the
// value i mod 2^k after i increments, the wrap from all ones (k bits
// changed), k = 1, k = 0 (nothing to change), the exact total cost
// sum_i floor( n / 2^i ), and an item that is not a bit (the audit)
ADS_TEST( "sec4.2.1.counter" )
{
 std::vector< int > A( 10 , 0 );
 std::size_t cost = 0;
 const std::size_t n = 1024;
 for( std::size_t i = 1 ; i <= n ; ++i ) {
  cost += add( A );
  CHECK( cost < 2 * i + A.size() );
  }
 CHECK( A == std::vector< int >( 10 , 0 ) );   // 2^10 increments
 add( A );
 CHECK( A[ 0 ] == 1 );
 auto value = []( const std::vector< int > & B ) {
  unsigned v = 0;
  for( std::size_t i = B.size() ; i-- > 0 ; ) v = 2 * v + unsigned( B[ i ] );
  return( v ); };
 std::vector< int > C( 5 , 0 );
 std::size_t total = 0;
 for( unsigned i = 1 ; i <= 100 ; ++i ) {
  const std::size_t c = add( C );
  total += c;
  CHECK( value( C ) == i % 32 );
  CHECK( c == ( i % 32 == 0 ? 5 : std::size_t( std::countr_zero( i ) ) + 1 ) );
  }
 std::size_t want = 0;                          // the bits that changed
 for( unsigned b = 0 ; b < 5 ; ++b ) want += 100 >> b;
 CHECK( total == want );
 std::vector< int > F( 6 , 1 );                 // all ones: the wrap
 CHECK( add( F ) == 6 && F == std::vector< int >( 6 , 0 ) );
 std::vector< int > O = { 0 };                  // k = 1
 CHECK( add( O ) == 1 && O[ 0 ] == 1 && add( O ) == 1 && O[ 0 ] == 0 );
 std::vector< int > Z;                          // k = 0
 CHECK( add( Z ) == 0 && Z.empty() );
 std::vector< int > N = { 0 , 2 };
 CHECK_THROWS( (void) add( N ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- STACK AND QUEUE -----------------------------*/
/*--------------------------------------------------------------------------*/

// section 4.3.2: LIFO, and the preconditions; the edges: capacity 0 (full
// and empty at once) and 1, filled and emptied several times, the copies
// equal whatever the capacity, a stack moved from left empty with no
// capacity
ADS_TEST( "spec.ArrayStack" )
{
 ArrayStack< int > S( 3 );
 CHECK( S.empty() );
 CHECK_THROWS( S.pop() , std::logic_error );
 CHECK_THROWS( S.top() , std::logic_error );
 S.push( 1 ); S.push( 2 ); S.push( 3 );
 CHECK_THROWS( S.push( 4 ) , std::logic_error );
 CHECK( S.top() == 3 );
 CHECK( S.pop() == 3 );
 CHECK( S.pop() == 2 );
 S.push( 5 );
 CHECK( S.pop() == 5 );
 CHECK( S.pop() == 1 );
 CHECK( S.empty() );
 ArrayStack< int > Z( 0 );
 CHECK( Z.empty() && Z.capacity() == 0 );
 CHECK_THROWS( Z.push( 1 ) , contract_violation );
 CHECK_THROWS( Z.pop() , contract_violation );
 ArrayStack< int > U( 1 );
 for( int t = 0 ; t < 3 ; ++t ) {
  U.push( t );
  CHECK( U.top() == t && U.size() == 1 );
  CHECK_THROWS( U.push( 9 ) , contract_violation );
  CHECK( U.pop() == t && U.empty() );
  }
 for( int t = 0 ; t < 3 ; ++t ) {               // filled and emptied
  for( int i = 0 ; i < 3 ; ++i ) S.push( 10 * t + i );
  CHECK( S.size() == 3 );
  for( int i = 2 ; i >= 0 ; --i ) CHECK( S.pop() == 10 * t + i );
  }
 ArrayStack< int > L( 10 );
 S.push( 1 ); S.push( 2 );
 L.push( 1 ); L.push( 2 );
 CHECK( S == L && ! ( S == U ) && Z == ArrayStack< int >( 5 ) );
 ArrayStack< int > C = S;
 C.pop();
 CHECK( S.size() == 2 && S.top() == 2 && C.top() == 1 );
 ArrayStack< int > M = std::move( S );
 CHECK( S.empty() && S.capacity() == 0 && M.top() == 2 );
 CHECK_THROWS( S.push( 1 ) , contract_violation );
 }

// section 4.3.1: the stack simulated with a sequence, on the LIST, on the
// singly linked list of Esercizio 4.6, on the vector and on the cursors,
// with no capacity
template< class S >
void stack_on_sequence( void )
{
 SequenceStack< S > P;
 CHECK( P.empty() );
 CHECK_THROWS( P.pop() , std::logic_error );
 for( int i = 0 ; i < 1000 ; ++i )
  P.push( i );
 CHECK( P.top() == 999 );
 for( int i = 999 ; i >= 0 ; --i )
  CHECK( P.pop() == i );
 CHECK( P.empty() );
 CHECK_THROWS( P.top() , std::logic_error );
 P.push( 4 );                                   // one item, after emptying
 CHECK( P.top() == 4 && ! P.empty() );
 CHECK( P.pop() == 4 && P.empty() );
 CHECK_THROWS( P.pop() , std::logic_error );
 for( int t = 0 ; t < 100 ; ++t ) {             // push and pop interleaved
  P.push( t );
  P.push( -t );
  CHECK( P.pop() == -t );
  }
 for( int t = 99 ; t >= 0 ; --t ) CHECK( P.pop() == t );
 CHECK( P.empty() );
 }

// the edges: the empty stack, one item, the stack used again after being
// emptied, pushes and pops interleaved
ADS_TEST( "sec4.3.1.stack-on-list" )
{
 stack_on_sequence< List< int > >();
 stack_on_sequence< SinglyList< int > >();
 stack_on_sequence< ArraySequence< int > >();
 stack_on_sequence< CursorList< int > >();
 }

// section 4.3.3: FIFO, the queue going round the circular vector; the
// edges: capacity 0 and 1 (the front wraps at every operation), contains()
// on the empty queue, on items that straddle the end of the vector, on an
// item dequeued and on one absent, and the preconditions
ADS_TEST( "spec.ArrayQueue" )
{
 ArrayQueue< int > Q( 3 );
 CHECK( Q.empty() );
 CHECK_THROWS( Q.dequeue() , std::logic_error );
 CHECK_THROWS( Q.top() , std::logic_error );
 int next_in = 0 , next_out = 0;
 for( int round = 0 ; round < 5 ; ++round ) {   // head wraps several times
  while( Q.size() < 3 )
   Q.enqueue( next_in++ );
  CHECK_THROWS( Q.enqueue( -1 ) , std::logic_error );
  CHECK( Q.top() == next_out );
  CHECK( Q.dequeue() == next_out++ );
  CHECK( Q.dequeue() == next_out++ );
  }
 while( ! Q.empty() )
  CHECK( Q.dequeue() == next_out++ );
 CHECK( next_out == next_in );
 CHECK( ! Q.contains( 0 ) && Q.capacity() == 3 );
 Q.enqueue( 1 ); Q.enqueue( 2 ); Q.dequeue(); Q.dequeue();
 Q.enqueue( 3 ); Q.enqueue( 4 ); Q.enqueue( 5 );   // 4 and 5 wrap round
 CHECK( Q.contains( 3 ) && Q.contains( 4 ) && Q.contains( 5 ) );
 CHECK( ! Q.contains( 1 ) && ! Q.contains( 2 ) && ! Q.contains( 6 ) );
 CHECK( Q.dequeue() == 3 && ! Q.contains( 3 ) && Q.top() == 4 );
 ArrayQueue< int > Z( 0 );
 CHECK( Z.empty() && ! Z.contains( 0 ) );
 CHECK_THROWS( Z.enqueue( 1 ) , contract_violation );
 CHECK_THROWS( Z.dequeue() , contract_violation );
 ArrayQueue< int > U( 1 );
 for( int t = 0 ; t < 5 ; ++t ) {
  U.enqueue( t );
  CHECK( U.top() == t && U.size() == 1 && U.contains( t ) );
  CHECK_THROWS( U.enqueue( 9 ) , contract_violation );
  CHECK( U.dequeue() == t && U.empty() );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------- EXAMPLES OF THE TEXT ---------------------------*/
/*--------------------------------------------------------------------------*/

// beginning of chapter 4: donGiovanni() removes the duchesses from the
// catalogue and copies the Spanish women in L, in the same order; the
// edges: the empty catalogue, all duchesses (all removed), consecutive
// duchesses (the position returned by remove() is a duchess again), a
// duchess first and last, nobody Spanish, a Spanish duchess
ADS_TEST( "sec4.donGiovanni" )
{
 struct Conquest {
  std::string name , nation , rank;
  bool operator==( const Conquest & ) const = default;
  };
 using Names = std::vector< std::string >;
 auto donGiovanni = [ & ]( List< Conquest > catalogo ) {
  List< Conquest > L;
  auto q = catalogo.head();
  while( ! catalogo.finished( q ) ) {
   const auto v = catalogo.read( q );
   if( v.nation == "spagnola" )
    L.insert( L.next( L.tail() ) , v );
   if( v.rank == "duchessa" )
    q = catalogo.remove( q );
   else
    q = catalogo.next( q );
   }
  Names in_L , left;
  for( const auto & c : items( L ) ) in_L.push_back( c.name );
  for( const auto & c : items( catalogo ) ) left.push_back( c.name );
  return( std::pair( in_L , left ) );
  };

 const Conquest E{ "Elvira" , "spagnola" , "duchessa" } ,
                Z{ "Zerlina" , "italiana" , "contadina" } ,
                A{ "Anna" , "spagnola" , "cittadina" } ,
                S{ "Sophie" , "francese" , "duchessa" } ,
                M{ "Mia" , "tedesca" , "duchessa" };
 auto [ in_L , left ] = donGiovanni( { E , Z , A , S } );
 CHECK( in_L == Names( { "Elvira" , "Anna" } ) );
 CHECK( left == Names( { "Zerlina" , "Anna" } ) );
 std::tie( in_L , left ) = donGiovanni( {} );
 CHECK( in_L.empty() && left.empty() );
 std::tie( in_L , left ) = donGiovanni( { E , S , M } );      // all
 CHECK( in_L == Names( { "Elvira" } ) && left.empty() );
 std::tie( in_L , left ) = donGiovanni( { Z , S , M , E , A } );
 CHECK( in_L == Names( { "Elvira" , "Anna" } ) &&
        left == Names( { "Zerlina" , "Anna" } ) );
 std::tie( in_L , left ) = donGiovanni( { S , Z , M } );      // first, last
 CHECK( in_L.empty() && left == Names( { "Zerlina" } ) );
 std::tie( in_L , left ) = donGiovanni( { Z } );
 CHECK( in_L.empty() && left == Names( { "Zerlina" } ) );
 }

// section 4.4, Figura 4.8: the moves for n = 3, and 2^n - 1 moves for each
// n, by the recursive procedure and by the two iterative ones; the edges:
// n = 1 (one move), every assignment of the three pegs, n = 20 (2^20 - 1
// moves, the stack of the iterative procedures deep 20), and the
// preconditions n >= 1 and three distinct pegs
ADS_TEST( "sec4.4.hanoi" )
{
 using Moves = std::vector< std::pair< int , int > >;
 const Moves fig48 = { { 1 , 3 } , { 1 , 2 } , { 3 , 2 } , { 1 , 3 } ,
                       { 2 , 1 } , { 2 , 3 } , { 1 , 3 } };
 for( int n = 1 ; n <= 12 ; ++n ) {
  Moves r , i1 , i2;
  hanoi_recursive( n , 1 , 3 , 2 , [ & ]( int a , int b ) {
   r.emplace_back( a , b ); } );
  hanoi_iterative( n , 1 , 3 , 2 , [ & ]( int a , int b ) {
   i1.emplace_back( a , b ); } );
  hanoi_iterative_labels( n , 1 , 3 , 2 , [ & ]( int a , int b ) {
   i2.emplace_back( a , b ); } );
  CHECK( r.size() == ( std::size_t( 1 ) << n ) - 1 );
  CHECK( i1 == r );
  CHECK( i2 == r );
  if( n == 3 )
   CHECK( r == fig48 );

  // the moves are legal: a disk never goes on a smaller one, and all of
  // them end on peg 3
  std::vector< std::vector< int > > peg( 4 );
  for( int d = n ; d >= 1 ; --d )
   peg[ 1 ].push_back( d );
  for( auto [ a , b ] : r ) {
   CHECK( ! peg[ a ].empty() );
   CHECK( peg[ b ].empty() || ( peg[ b ].back() > peg[ a ].back() ) );
   peg[ b ].push_back( peg[ a ].back() );
   peg[ a ].pop_back();
   }
  CHECK( int( peg[ 3 ].size() ) == n );
  }
 Moves one;
 hanoi_recursive( 1 , 2 , 1 , 3 , [ & ]( int a , int b ) {
  one.emplace_back( a , b ); } );
 CHECK( one == Moves( { { 2 , 1 } } ) );
 int pegs[] = { 1 , 2 , 3 };
 do {                                       // every source, dest, middle
  const int s = pegs[ 0 ] , d = pegs[ 1 ] , m = pegs[ 2 ];
  for( int n : { 1 , 2 , 5 } ) {
   Moves r , i1 , i2;
   hanoi_recursive( n , s , d , m , [ & ]( int a , int b ) {
    r.emplace_back( a , b ); } );
   hanoi_iterative( n , s , d , m , [ & ]( int a , int b ) {
    i1.emplace_back( a , b ); } );
   hanoi_iterative_labels( n , s , d , m , [ & ]( int a , int b ) {
    i2.emplace_back( a , b ); } );
   CHECK( i1 == r && i2 == r && r.size() == ( std::size_t( 1 ) << n ) - 1 );
   std::vector< std::vector< int > > peg( 4 );
   for( int k = n ; k >= 1 ; --k ) peg[ s ].push_back( k );
   for( auto [ a , b ] : r ) {
    CHECK( ! peg[ a ].empty() &&
           ( peg[ b ].empty() || peg[ b ].back() > peg[ a ].back() ) );
    peg[ b ].push_back( peg[ a ].back() );
    peg[ a ].pop_back();
    }
   CHECK( int( peg[ d ].size() ) == n );
   }
  } while( std::next_permutation( pegs , pegs + 3 ) );
 long c1 = 0 , c2 = 0 , c3 = 0;
 hanoi_recursive( 20 , 1 , 3 , 2 , [ & ]( int , int ) { ++c1; } );
 hanoi_iterative( 20 , 1 , 3 , 2 , [ & ]( int , int ) { ++c2; } );
 hanoi_iterative_labels( 20 , 1 , 3 , 2 , [ & ]( int , int ) { ++c3; } );
 CHECK( c1 == ( 1L << 20 ) - 1 && c2 == c1 && c3 == c1 );
 auto none = []( int , int ) {};
 CHECK_THROWS( hanoi_recursive( 0 , 1 , 3 , 2 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative( 0 , 1 , 3 , 2 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative_labels( -1 , 1 , 3 , 2 , none ) ,
               contract_violation );
 CHECK_THROWS( hanoi_recursive( 2 , 1 , 1 , 2 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative( 2 , 1 , 3 , 3 , none ) , contract_violation );
 CHECK_THROWS( hanoi_iterative_labels( 2 , 2 , 3 , 2 , none ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- EXERCISES --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

// Soluzione Esercizio 4.1: firstN()
void firstN( int n , List< int > & L , List< int > & M )
{
 if( n >= 1 ) {
  L.insert( L.head() , n );
  M.insert( M.next( M.tail() ) , n );
  firstN( n - 1 , L , M );
  }
 }

}  // end( namespace )

// Esercizio 4.1: L = 1, ..., n and M = n, ..., 1; the edges: n = 0 and a
// negative n (nothing), n = 1, and n = 5000 (the depth of the recursion)
ADS_TEST( "ex4.1" )
{
 for( int n : { 1 , 2 , 7 , 5000 } ) {
  List< int > L , M;
  firstN( n , L , M );
  std::vector< int > up , down;
  for( int i = 1 ; i <= n ; ++i ) {
   up.push_back( i );
   down.push_back( n - i + 1 );
   }
  CHECK( items( L ) == up );
  CHECK( items( M ) == down );
  }
 List< int > L , M;
 firstN( 0 , L , M );   // nothing for n = 0
 CHECK( L.empty() && M.empty() );
 firstN( -3 , L , M );
 CHECK( L.empty() && M.empty() );
 }

namespace {

// Soluzione Esercizio 4.2: epurazione(), O( n^2 )
List< int > epurazione( const List< int > & L )
{
 List< int > M;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) ) {
  int count = 0;
  for( auto q = L.head() ; ! L.finished( q ) ; q = L.next( q ) )
   if( L.read( q ) == L.read( p ) )
    ++count;
  if( count != 2 )
   M.insert( M.next( M.tail() ) , L.read( p ) );
  }
 return( M );
 }

}  // end( namespace )

// Esercizio 4.2: the items that appear exactly twice go; the edges: empty,
// one item, a pair (nothing left), three and four copies (kept), two
// pairs apart, negative items and 0, against counting on a vector
ADS_TEST( "ex4.2" )
{
 using V = std::vector< int >;
 CHECK( items( epurazione( { 5 , 7 , 3 , 2 , 2 , 1 , 2 , 3 } ) ) ==
        V( { 5 , 7 , 2 , 2 , 1 , 2 } ) );
 CHECK( items( epurazione( {} ) ).empty() );
 CHECK( items( epurazione( { 4 , 4 } ) ).empty() );
 CHECK( items( epurazione( { 4 } ) ) == V( { 4 } ) );
 CHECK( items( epurazione( { 4 , 4 , 4 } ) ) == V( { 4 , 4 , 4 } ) );
 CHECK( items( epurazione( { 4 , 4 , 4 , 4 } ) ) == V( 4 , 4 ) );
 CHECK( items( epurazione( { 1 , 2 , 0 , 2 , 1 } ) ) == V( { 0 } ) );
 CHECK( items( epurazione( { -1 , 0 , -1 , 0 } ) ).empty() );
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 100 ; ++t ) {
  V v( g() % 12 );
  for( auto & x : v ) x = int( g() % 5 ) - 2;
  V want;
  for( int x : v )
   if( std::count( v.begin() , v.end() , x ) != 2 ) want.push_back( x );
  List< int > L;
  for( int x : v ) append( L , x );
  CHECK( items( epurazione( L ) ) == want );
  }
 }

namespace {

// Esercizio 4.3: the sum of two numbers given as lists of digits, the most
// significant first; the digits are added from the tails with a carry, so
// that the sum has the digits of the longer list, its leading zeros kept,
// plus one for a last carry, and two empty lists give 0
List< int > sum( const List< int > & A , const List< int > & B )
{
 List< int > C;
 auto p = A.tail();
 auto q = B.tail();
 int carry = 0;
 while( ( ! A.finished( p ) ) || ( ! B.finished( q ) ) || carry ) {
  int d = carry;
  if( ! A.finished( p ) ) { d += A.read( p ); p = A.prev( p ); }
  if( ! B.finished( q ) ) { d += B.read( q ); q = B.prev( q ); }
  C.insert( C.head() , d % 10 );
  carry = d / 10;
  }
 if( C.empty() )
  C.insert( C.head() , 0 );
 return( C );
 }

}  // end( namespace )

// Esercizio 4.3: 190 + 17 = 207, and the cases with carries and zeros; the
// edges: both lists empty (the number 0), one empty, the shorter list
// first or second, a carry through every digit and out of the longer
// list, leading zeros (kept, as many digits as the longer list), against
// the arithmetic of long long on numbers of up to 17 digits
ADS_TEST( "ex4.3" )
{
 using V = std::vector< int >;
 CHECK( items( sum( { 1 , 9 , 0 } , { 1 , 7 } ) ) == V( { 2 , 0 , 7 } ) );
 CHECK( items( sum( { 9 , 9 , 9 } , { 1 } ) ) == V( { 1 , 0 , 0 , 0 } ) );
 CHECK( items( sum( { 0 } , { 0 } ) ) == V( { 0 } ) );
 CHECK( items( sum( {} , { 4 , 2 } ) ) == V( { 4 , 2 } ) );
 CHECK( items( sum( {} , {} ) ) == V( { 0 } ) );
 CHECK( items( sum( { 4 , 2 } , {} ) ) == V( { 4 , 2 } ) );
 CHECK( items( sum( { 1 } , { 9 , 9 , 9 } ) ) == V( { 1 , 0 , 0 , 0 } ) );
 CHECK( items( sum( { 9 , 9 } , { 9 , 9 } ) ) == V( { 1 , 9 , 8 } ) );
 CHECK( items( sum( { 0 , 0 , 5 } , { 5 } ) ) == V( { 0 , 1 , 0 } ) );
 std::mt19937_64 g( 13 );
 auto digits = []( long long x ) {
  List< int > L;
  do { L.insert( L.head() , int( x % 10 ) ); x /= 10; } while( x );
  return( L );
  };
 auto value = []( const V & v ) {
  long long x = 0;
  for( int d : v ) x = 10 * x + d;
  return( x ); };
 auto random = [ & ]( void ) {             // up to 17 digits, often fewer
  return( (long long)( ( g() % 100000000000000000ULL ) >> ( g() % 57 ) ) );
  };
 for( int t = 0 ; t < 300 ; ++t ) {
  const long long a = random() , b = random();
  const V s = items( sum( digits( a ) , digits( b ) ) );
  CHECK( value( s ) == a + b && ( s.size() == 1 || s[ 0 ] != 0 ) );
  CHECK( std::all_of( s.begin() , s.end() , []( int d ) {
   return( d >= 0 && d <= 9 ); } ) );
  }
 }

namespace {

// Soluzione Esercizio 4.4: tangentopoli(), the order of the victims when
// every k-th person of the circle is killed (the problem of Josephus); k
// may exceed the people left, the count going round the circle, and k < 1
// is not a count, std::invalid_argument
std::vector< std::string > tangentopoli( List< std::string > L , int k )
{
 if( k < 1 )
  throw( std::invalid_argument( "tangentopoli: k < 1" ) );
 std::vector< std::string > order;
 auto p = L.head();
 while( ! L.empty() ) {
  for( int i = 1 ; i <= k - 1 ; ++i ) {
   p = L.next( p );
   if( L.finished( p ) )
    p = L.head();
   }
  order.push_back( L.read( p ) );
  p = L.remove( p );
  if( L.finished( p ) )
   p = L.head();
  }
 return( order );
 }

}  // end( namespace )

// Esercizio 4.4: Caio, Sempronio, Tizio, Tullio and k = 3; the edges: the
// empty circle, one person, k = 1 (the order of the circle), k = n, k much
// larger than n, n = 2, and k < 1, against a simulation on a vector
ADS_TEST( "ex4.4" )
{
 using V = std::vector< std::string >;
 CHECK( tangentopoli( { "Caio" , "Sempronio" , "Tizio" , "Tullio" } , 3 ) ==
        V( { "Tizio" , "Sempronio" , "Tullio" , "Caio" } ) );
 CHECK( tangentopoli( { "A" , "B" , "C" } , 1 ) == V( { "A" , "B" , "C" } ) );
 CHECK( tangentopoli( { "A" } , 1 ) == V( { "A" } ) );

 // the survivor of the classical problem: n = 41, k = 3 leaves the 31st
 List< std::string > L;
 for( int i = 1 ; i <= 41 ; ++i )
  append( L , std::to_string( i ) );
 CHECK( tangentopoli( L , 3 ).back() == "31" );
 CHECK( tangentopoli( {} , 3 ).empty() );
 CHECK( tangentopoli( { "A" } , 7 ) == V( { "A" } ) );
 CHECK( tangentopoli( { "A" , "B" } , 2 ) == V( { "B" , "A" } ) );
 CHECK( tangentopoli( { "A" , "B" , "C" } , 3 ) == V( { "C" , "A" , "B" } ) );
 CHECK_THROWS( tangentopoli( { "A" } , 0 ) , std::invalid_argument );
 CHECK_THROWS( tangentopoli( {} , -1 ) , std::invalid_argument );
 for( int n = 1 ; n <= 9 ; ++n )
  for( int k : { 1 , 2 , 3 , n , n + 1 , 2 * n + 3 , 100 } ) {
   List< std::string > C;
   std::vector< std::string > v , want;
   for( int i = 0 ; i < n ; ++i ) {
    append( C , std::to_string( i ) );
    v.push_back( std::to_string( i ) );
    }
   for( std::size_t i = 0 ; ! v.empty() ; ) {
    i = ( i + std::size_t( k ) - 1 ) % v.size();
    want.push_back( v[ i ] );
    v.erase( v.begin() + long( i ) );
    if( ! v.empty() ) i %= v.size();
    }
   CHECK( tangentopoli( C , k ) == want );
   }
 }

namespace {

// Esercizio 4.5: recursively, the even items go and each odd one is
// replicated as many times as the even ones before it, i.e., that many
// copies are added to it
void replicate( List< int > & L , List< int >::Pos p , int evens )
{
 if( L.finished( p ) )
  return;
 const int v = L.read( p );
 if( v % 2 == 0 )
  replicate( L , L.remove( p ) , evens + 1 );
 else {
  for( int i = 0 ; i < evens ; ++i )
   L.insert( p , v );
  replicate( L , L.next( p ) , evens );
  }
 }

}  // end( namespace )

// Esercizio 4.5: L = 4, 6, 7, 3, 2, 5 becomes 7, 7, 7, 3, 3, 3, 5, 5, 5, 5;
// the edges: empty, one even and one odd item, an even item last (no
// copy after it), 0 and negative items (-3 odd, -2 even), against the
// definition on random lists
ADS_TEST( "ex4.5" )
{
 using V = std::vector< int >;
 List< int > L{ 4 , 6 , 7 , 3 , 2 , 5 };
 replicate( L , L.head() , 0 );
 CHECK( items( L ) == V( { 7 , 7 , 7 , 3 , 3 , 3 , 5 , 5 , 5 , 5 } ) );
 List< int > E{ 2 , 4 };   // only even items: nothing is left
 replicate( E , E.head() , 0 );
 CHECK( E.empty() );
 List< int > O{ 1 , 3 };   // no even item: the odd ones stay as they are
 replicate( O , O.head() , 0 );
 CHECK( items( O ) == V( { 1 , 3 } ) );
 for( const auto & [ in , out ] : { std::pair( V() , V() ) ,
        std::pair( V{ 2 } , V() ) , std::pair( V{ 1 } , V{ 1 } ) ,
        std::pair( V{ 1 , 2 } , V{ 1 } ) ,
        std::pair( V{ 0 , -3 , -2 , 5 } , V{ -3 , -3 , 5 , 5 , 5 } ) } ) {
  List< int > R;
  for( int x : in ) append( R , x );
  replicate( R , R.head() , 0 );
  CHECK( items( R ) == out );
  }
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 100 ; ++t ) {
  V in( g() % 10 ) , want;
  for( auto & x : in ) x = int( g() % 9 ) - 4;
  int evens = 0;
  for( int x : in )
   if( x % 2 == 0 ) ++evens;
   else want.insert( want.end() , std::size_t( evens + 1 ) , x );
  List< int > R;
  for( int x : in ) append( R , x );
  replicate( R , R.head() , 0 );
  CHECK( items( R ) == want );
  }
 }

namespace {

// Esercizio 4.7: two stacks in one vector, one growing from the place 0 and
// the other from the place capacity - 1; a third one cannot be created
class TwoStacks {
 std::vector< int > f_A;
 std::size_t f_top[ 2 ] = { 0 , 0 };   // the number of items of each
 int f_created = 0;

 public:

 explicit TwoStacks( std::size_t capacity ) : f_A( capacity ) {}

 /// a stack in the vector: 0, then 1, then an error
 int create( void ) {
  if( f_created == 2 )
   throw( std::logic_error( "TwoStacks::create: two stacks already" ) );
  return( f_created++ );
  }

 bool empty( int s ) const { return( f_top[ s ] == 0 ); }

 void push( int s , int v ) {
  if( f_top[ 0 ] + f_top[ 1 ] == f_A.size() )
   throw( std::logic_error( "TwoStacks::push: full vector" ) );
  const std::size_t i = s == 0 ? f_top[ 0 ] : f_A.size() - 1 - f_top[ 1 ];
  f_A[ i ] = v;
  ++f_top[ s ];
  }

 int top( int s ) const {
  if( empty( s ) )
   throw( std::logic_error( "TwoStacks::top: empty stack" ) );
  return( s == 0 ? f_A[ f_top[ 0 ] - 1 ] : f_A[ f_A.size() - f_top[ 1 ] ] );
  }

 int pop( int s ) {
  const int v = top( s );
  --f_top[ s ];
  return( v );
  }
 };

}  // end( namespace )

// Esercizio 4.7: two stacks share the vector until it is full; the edges:
// a vector of 0 places (no push) and of 1 (whichever stack comes first),
// one stack taking the whole vector, top() and pop() of an empty stack
// while the other one is full, and the stacks against two std::vector
ADS_TEST( "ex4.7" )
{
 TwoStacks T( 5 );
 const int a = T.create() , b = T.create();
 CHECK_THROWS( T.create() , std::logic_error );
 T.push( a , 1 ); T.push( a , 2 ); T.push( a , 3 );
 T.push( b , 10 ); T.push( b , 20 );
 CHECK_THROWS( T.push( a , 4 ) , std::logic_error );
 CHECK_THROWS( T.push( b , 30 ) , std::logic_error );
 CHECK( T.pop( b ) == 20 );
 T.push( a , 4 );   // a grows in the place b has left
 CHECK( T.pop( a ) == 4 );
 CHECK( T.pop( a ) == 3 );
 CHECK( T.top( b ) == 10 );
 CHECK( T.pop( b ) == 10 );
 CHECK( T.empty( b ) );
 CHECK_THROWS( T.pop( b ) , std::logic_error );
 TwoStacks Z( 0 );
 Z.create(); Z.create();
 CHECK( Z.empty( 0 ) && Z.empty( 1 ) );
 CHECK_THROWS( Z.push( 0 , 1 ) , std::logic_error );
 CHECK_THROWS( Z.push( 1 , 1 ) , std::logic_error );
 CHECK_THROWS( Z.top( 1 ) , std::logic_error );
 TwoStacks U( 1 );
 U.create(); U.create();
 U.push( 1 , 8 );
 CHECK_THROWS( U.push( 0 , 9 ) , std::logic_error );
 CHECK( U.top( 1 ) == 8 && U.empty( 0 ) );
 CHECK_THROWS( U.top( 0 ) , std::logic_error );
 CHECK( U.pop( 1 ) == 8 );
 U.push( 0 , 9 );
 CHECK( U.top( 0 ) == 9 && U.empty( 1 ) );
 for( int s : { 0 , 1 } ) {                     // one stack takes it all
  TwoStacks W( 4 );
  W.create(); W.create();
  for( int i = 0 ; i < 4 ; ++i ) W.push( s , i );
  CHECK_THROWS( W.push( 1 - s , 0 ) , std::logic_error );
  CHECK_THROWS( W.pop( 1 - s ) , std::logic_error );
  for( int i = 3 ; i >= 0 ; --i ) CHECK( W.pop( s ) == i );
  CHECK( W.empty( s ) );
  }
 std::mt19937 g( 15 );                         // against two std::vector
 TwoStacks R( 7 );
 R.create(); R.create();
 std::vector< int > o[ 2 ];
 for( int t = 0 ; t < 2000 ; ++t ) {
  const int s = int( g() % 2 );
  if( g() % 2 ) {
   if( o[ 0 ].size() + o[ 1 ].size() == 7 )
    CHECK_THROWS( R.push( s , t ) , std::logic_error );
   else { R.push( s , t ); o[ s ].push_back( t ); }
   }
  else
   if( o[ s ].empty() )
    CHECK_THROWS( R.pop( s ) , std::logic_error );
   else { CHECK( R.pop( s ) == o[ s ].back() ); o[ s ].pop_back(); }
  }
 }

namespace {

// Esercizio 4.8: firstN() made iterative by the rules of section 4.4: the
// recursive call is the last statement, so it needs no label, and the
// stack only remembers that there is a call to return from
void firstN_iterative( int n , List< int > & L , List< int > & M )
{
 ArrayStack< int > S( n > 0 ? n : 1 );
 while( n >= 1 ) {   // START: the body of the call, then the call n - 1
  L.insert( L.head() , n );
  M.insert( M.next( M.tail() ) , n );
  S.push( n );
  n = n - 1;
  }
 while( ! S.empty() )     // END: the calls return in turn
  S.pop();
 }

}  // end( namespace )

// Esercizio 4.8: the same lists as firstN(); the edges: a negative n and
// n = 0 (nothing, a stack of one place), n = 1, and n = 5000
ADS_TEST( "ex4.8" )
{
 for( int n : { -2 , 0 , 1 , 5 , 50 , 5000 } ) {
  List< int > L1 , M1 , L2 , M2;
  firstN( n , L1 , M1 );
  firstN_iterative( n , L2 , M2 );
  CHECK( items( L1 ) == items( L2 ) );
  CHECK( items( M1 ) == items( M2 ) );
  }
 }

// Esercizio 4.9: both ends, and a stack and a queue made of a dequeue; the
// edges: the empty dequeue (the four preconditions), one item (head and
// tail the same, removed from either end), emptied and used again, and
// random operations at both ends against std::deque
ADS_TEST( "ex4.9" )
{
 Dequeue< int > D;
 CHECK_THROWS( D.removeHead() , std::logic_error );
 D.insertTail( 2 ); D.insertHead( 1 ); D.insertTail( 3 );
 CHECK( D.head() == 1 && D.tail() == 3 );
 CHECK( D.removeTail() == 3 );
 CHECK( D.removeHead() == 1 );
 CHECK( D.removeHead() == 2 );
 CHECK( D.empty() );
 CHECK_THROWS( D.removeTail() , std::logic_error );
 CHECK_THROWS( (void) D.head() , contract_violation );
 CHECK_THROWS( (void) D.tail() , contract_violation );
 D.insertHead( 5 );
 CHECK( D.head() == 5 && D.tail() == 5 && D.removeTail() == 5 );
 D.insertTail( 6 );
 CHECK( D.head() == 6 && D.tail() == 6 && D.removeHead() == 6 );
 CHECK( D.empty() );
 std::mt19937 g( 16 );
 std::deque< int > o;
 for( int t = 0 ; t < 3000 ; ++t ) {
  switch( g() % 4 ) {
   case 0: D.insertHead( t ); o.push_front( t ); break;
   case 1: D.insertTail( t ); o.push_back( t ); break;
   case 2:
    if( o.empty() ) CHECK_THROWS( D.removeHead() , contract_violation );
    else { CHECK( D.removeHead() == o.front() ); o.pop_front(); }
    break;
   default:
    if( o.empty() ) CHECK_THROWS( D.removeTail() , contract_violation );
    else { CHECK( D.removeTail() == o.back() ); o.pop_back(); }
   }
  CHECK( D.empty() == o.empty() );
  if( ! o.empty() ) CHECK( D.head() == o.front() && D.tail() == o.back() );
  }
 }

static_assert( Queue< LinkedQueue< int > > );

// Esercizio 4.10: FIFO, also after being emptied; the edges: top() and
// dequeue() of the empty queue, one item (the front and the back the same
// cell) enqueued and dequeued in turn, enqueue() after a dequeue() that
// empties the queue (the back reset), the copies equal and independent, a
// queue moved from left empty, and the size
ADS_TEST( "ex4.10" )
{
 LinkedQueue< int > Q;
 CHECK_THROWS( Q.dequeue() , std::logic_error );
 CHECK_THROWS( (void) Q.top() , std::logic_error );
 for( int i = 0 ; i < 100 ; ++i )
  Q.enqueue( i );
 CHECK( Q.size() == 100 );
 for( int i = 0 ; i < 100 ; ++i )
  CHECK( Q.dequeue() == i );
 CHECK( Q.empty() && Q.size() == 0 );
 Q.enqueue( 7 );
 CHECK( Q.top() == 7 );
 for( int t = 0 ; t < 5 ; ++t ) {               // one item at a time
  CHECK( Q.dequeue() == ( t ? t - 1 : 7 ) && Q.empty() );
  Q.enqueue( t );
  CHECK( Q.top() == t && Q.size() == 1 );
  }
 Q.enqueue( 10 );
 Q.enqueue( 11 );                               // 4 , 10 , 11
 LinkedQueue< int > C = Q;
 CHECK( C == Q && C.size() == 3 );
 CHECK( C.dequeue() == 4 && ! ( C == Q ) && Q.top() == 4 );
 C.enqueue( 12 );
 CHECK( Q.size() == 3 && C.size() == 3 );
 const LinkedQueue< int > E , F = E;
 CHECK( E == F && F.empty() && ! ( E == Q ) );
 LinkedQueue< int > M = std::move( Q );
 CHECK( Q.empty() && M.size() == 3 && M.top() == 4 );
 Q.enqueue( 1 );                                // usable after the move
 CHECK( Q.top() == 1 && Q.dequeue() == 1 && Q.empty() );
 }

// Esercizio 4.11: sorted by key, the values of a key in their first order;
// the edges: empty (also with k = 0), one pair, k = 1 (all the same key,
// the order kept), the keys 0 and k - 1, sorted and reverse sorted keys,
// against std::stable_sort, and the preconditions (a key k, a negative
// key, a negative k)
ADS_TEST( "ex4.11" )
{
 using P = std::vector< std::pair< int , std::string > >;
 const P A = { { 2 , "c" } , { 0 , "a" } , { 2 , "d" } , { 1 , "b" } ,
               { 0 , "e" } };
 CHECK( pigeonholeSort( A , 3 ) ==
        P( { { 0 , "a" } , { 0 , "e" } , { 1 , "b" } , { 2 , "c" } ,
             { 2 , "d" } } ) );
 CHECK( pigeonholeSort( P() , 4 ).empty() );
 CHECK( pigeonholeSort( P() , 0 ).empty() );
 CHECK( pigeonholeSort( P( { { 0 , "x" } } ) , 1 ) == P( { { 0 , "x" } } ) );
 const P same = { { 0 , "c" } , { 0 , "a" } , { 0 , "b" } };
 CHECK( pigeonholeSort( same , 1 ) == same );
 const P ends = { { 9 , "z" } , { 0 , "a" } , { 9 , "y" } , { 0 , "b" } };
 CHECK( pigeonholeSort( ends , 10 ) ==
        P( { { 0 , "a" } , { 0 , "b" } , { 9 , "z" } , { 9 , "y" } } ) );
 std::mt19937 g( 17 );
 auto byKey = []( const auto & x , const auto & y ) {
  return( x.first < y.first ); };
 for( int k : { 1 , 2 , 7 , 50 } )
  for( int order = 0 ; order < 3 ; ++order ) {   // random, sorted, reverse
   P R( g() % 40 );
   for( std::size_t i = 0 ; i < R.size() ; ++i )
    R[ i ] = { int( g() % k ) , std::to_string( i ) };
   if( order ) std::stable_sort( R.begin() , R.end() , byKey );
   if( order == 2 ) std::reverse( R.begin() , R.end() );
   P S = R;
   std::stable_sort( S.begin() , S.end() , byKey );
   CHECK( pigeonholeSort( R , k ) == S );
   }
 CHECK_THROWS( (void) pigeonholeSort( P( { { 3 , "a" } } ) , 3 ) ,
               contract_violation );
 CHECK_THROWS( (void) pigeonholeSort( P( { { -1 , "a" } } ) , 3 ) ,
               contract_violation );
 CHECK_THROWS( (void) pigeonholeSort( P( { { 0 , "a" } } ) , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) pigeonholeSort( P() , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch04.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
