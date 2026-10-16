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
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/List.h"
#include "ADS/sort/PigeonholeSort.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/SequenceStack.h"

#include <string>
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

/*--------------------------------------------------------------------------*/
/// Esercizio 4.6: a SEQUENCE as a singly linked list, not circular and with
/// no sentinel; every operation but tail() and prev() takes O( 1 ) time
/// since \f$ pos_i \f$ is the address of the cell of \f$ a_{i-1} \f$ for
/// i > 0 and nil for i = 0, hence the cell before an item is always at
/// hand. \f$ pos_{-1} \f$ is a distinguished address, f_before.
template< class T >
class SinglyList {
 struct Cell { T value; Cell * next; };
 Cell * f_first = nullptr;
 Cell f_before_cell{ T() , nullptr };
 Cell * const f_before = & f_before_cell;

 public:

 using Item = T;
 using Pos = Cell *;

 SinglyList( void ) = default;
 SinglyList( std::initializer_list< T > items ) {
  for( const auto & v : items )
   insert( next( tail() ) , v );
  }
 SinglyList( const SinglyList & ) = delete;
 SinglyList & operator=( const SinglyList & ) = delete;
 ~SinglyList() { while( ! isEmpty() ) remove( head() ); }

 [[nodiscard]] bool isEmpty( void ) const { return( ! f_first ); }

 // pos_n is the address of the last cell, or nil if the list is empty
 [[nodiscard]] bool finished( Pos p ) const {
  if( p == f_before )
   return( true );
  return( p ? ( p->next == nullptr ) : ( f_first == nullptr ) );
  }

 [[nodiscard]] Pos head( void ) const { return( nullptr ); }

 // O( n ): the address of the cell before the last one
 [[nodiscard]] Pos tail( void ) const {
  if( ! f_first )
   return( f_before );
  Pos p = nullptr;
  while( next( p ) && ( ! finished( next( p ) ) ) )
   p = next( p );
  return( p );
  }

 [[nodiscard]] Pos next( Pos p ) const {
  if( p == f_before )
   return( nullptr );
  return( p ? p->next : f_first );
  }

 // O( n ): a scan for the cell whose next one is p
 [[nodiscard]] Pos prev( Pos p ) const {
  if( ! p )
   return( f_before );
  if( p == f_first )
   return( nullptr );
  Pos q = f_first;
  while( q->next != p )
   q = q->next;
  return( q );
  }

 Pos insert( Pos p , const T & v ) {
  if( ! p )
   f_first = new Cell{ v , f_first };
  else
   p->next = new Cell{ v , p->next };
  return( p );   // the position of the new item is the one it got
  }

 Pos remove( Pos p ) {
  Cell * t = p ? p->next : f_first;
  ( p ? p->next : f_first ) = t->next;
  delete t;
  return( p );
  }

 [[nodiscard]] const T & read( Pos p ) const {
  return( p ? p->next->value : f_first->value );
  }

 void write( Pos p , const T & v ) {
  ( p ? p->next : f_first )->value = v;
  }
 };

static_assert( Sequence< List< int > > );
static_assert( Sequence< ArraySequence< int > > );
static_assert( Sequence< SinglyList< int > > );
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

/// the specification of SEQUENCE [section 3.2.1] checked on a realization
template< Sequence S >
void sequence_spec( void )
{
 using V = std::vector< int >;

 // the empty sequence: head() and tail() are pos_n and pos_-1
 S s;
 CHECK( s.isEmpty() );
 CHECK( s.finished( s.head() ) );
 CHECK( s.finished( s.tail() ) );

 // insert( pos_n , v ) appends, insert( pos_0 , v ) prepends
 append( s , 2 );
 CHECK( ! s.isEmpty() );
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
 CHECK( s.isEmpty() );
 CHECK( items( s ).empty() );

 // the sequence works again after being emptied, also with duplicates
 for( int i : { 5 , 5 , 7 } )
  append( s , i );
 CHECK( items( s ) == V( { 5 , 5 , 7 } ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------- REALIZATIONS OF SEQUENCE --------------------------*/
/*--------------------------------------------------------------------------*/

// section 4.1, the LIST with pointers
ADS_TEST( "spec.List" ) { sequence_spec< List< int > >(); }

// section 4.2, the vector
ADS_TEST( "spec.ArraySequence" ) { sequence_spec< ArraySequence< int > >(); }

// Esercizio 4.6, the singly linked list with no sentinel
ADS_TEST( "ex4.6" ) { sequence_spec< SinglyList< int > >(); }

// section 4.2: the vector doubles, so its size is always a power of 2 and
// less than twice the number of items
ADS_TEST( "sec4.2.doubling" )
{
 ArraySequence< int > s;
 for( int i = 1 ; i <= 1000 ; ++i ) {
  append( s , i );
  CHECK( s.size() == i );
  CHECK( ( s.capacity() & ( s.capacity() - 1 ) ) == 0 );
  CHECK( s.capacity() < 2 * i );
  }
 }

// section 4.2.1: n increments of a counter of k bits change fewer than 2n
// bits, and the counter goes back to 0 after 2^k of them
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
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- STACK AND QUEUE -----------------------------*/
/*--------------------------------------------------------------------------*/

// section 4.3.2: LIFO, and the preconditions
ADS_TEST( "spec.ArrayStack" )
{
 ArrayStack< int > S( 3 );
 CHECK( S.isEmpty() );
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
 CHECK( S.isEmpty() );
 }

// section 4.3.1: the stack simulated with a sequence, on the LIST and on the
// singly linked list of Esercizio 4.6, with no capacity
template< class S >
void stack_on_sequence( void )
{
 SequenceStack< S > P;
 CHECK( P.isEmpty() );
 CHECK_THROWS( P.pop() , std::logic_error );
 for( int i = 0 ; i < 1000 ; ++i )
  P.push( i );
 CHECK( P.top() == 999 );
 for( int i = 999 ; i >= 0 ; --i )
  CHECK( P.pop() == i );
 CHECK( P.isEmpty() );
 CHECK_THROWS( P.top() , std::logic_error );
 }

ADS_TEST( "sec4.3.1.stack-on-list" )
{
 stack_on_sequence< List< int > >();
 stack_on_sequence< SinglyList< int > >();
 }

// section 4.3.3: FIFO, the queue going round the circular vector
ADS_TEST( "spec.ArrayQueue" )
{
 ArrayQueue< int > Q( 3 );
 CHECK( Q.isEmpty() );
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
 while( ! Q.isEmpty() )
  CHECK( Q.dequeue() == next_out++ );
 CHECK( next_out == next_in );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- EXAMPLES OF THE TEXT ---------------------------*/
/*--------------------------------------------------------------------------*/

// beginning of chapter 4: donGiovanni() removes the duchesses from the
// catalogue and copies the Spanish women in L, in the same order
ADS_TEST( "sec4.donGiovanni" )
{
 struct Conquest {
  std::string name , nation , rank;
  bool operator==( const Conquest & ) const = default;
  };
 List< Conquest > catalogo{ { "Elvira" , "spagnola" , "duchessa" } ,
                            { "Zerlina" , "italiana" , "contadina" } ,
                            { "Anna" , "spagnola" , "cittadina" } ,
                            { "Sophie" , "francese" , "duchessa" } };
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

 std::vector< std::string > in_L , left;
 for( const auto & c : items( L ) ) in_L.push_back( c.name );
 for( const auto & c : items( catalogo ) ) left.push_back( c.name );
 CHECK( in_L == std::vector< std::string >( { "Elvira" , "Anna" } ) );
 CHECK( left == std::vector< std::string >( { "Zerlina" , "Anna" } ) );
 }

// section 4.4, Figura 4.8: the moves for n = 3, and 2^n - 1 moves for each
// n, by the recursive procedure and by the two iterative ones
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

// Esercizio 4.1: L = 1, ..., n and M = n, ..., 1
ADS_TEST( "ex4.1" )
{
 for( int n : { 1 , 2 , 7 } ) {
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
 CHECK( L.isEmpty() && M.isEmpty() );
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

// Esercizio 4.2: the items that appear exactly twice go
ADS_TEST( "ex4.2" )
{
 using V = std::vector< int >;
 CHECK( items( epurazione( { 5 , 7 , 3 , 2 , 2 , 1 , 2 , 3 } ) ) ==
        V( { 5 , 7 , 2 , 2 , 1 , 2 } ) );
 CHECK( items( epurazione( {} ) ).empty() );
 CHECK( items( epurazione( { 4 , 4 } ) ).empty() );
 CHECK( items( epurazione( { 4 } ) ) == V( { 4 } ) );
 }

namespace {

// Esercizio 4.3: the sum of two numbers given as lists of digits, the most
// significant first; the digits are added from the tails with a carry
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
 if( C.isEmpty() )
  C.insert( C.head() , 0 );
 return( C );
 }

}  // end( namespace )

// Esercizio 4.3: 190 + 17 = 207, and the cases with carries and zeros
ADS_TEST( "ex4.3" )
{
 using V = std::vector< int >;
 CHECK( items( sum( { 1 , 9 , 0 } , { 1 , 7 } ) ) == V( { 2 , 0 , 7 } ) );
 CHECK( items( sum( { 9 , 9 , 9 } , { 1 } ) ) == V( { 1 , 0 , 0 , 0 } ) );
 CHECK( items( sum( { 0 } , { 0 } ) ) == V( { 0 } ) );
 CHECK( items( sum( {} , { 4 , 2 } ) ) == V( { 4 , 2 } ) );
 }

namespace {

// Soluzione Esercizio 4.4: tangentopoli(), the order of the victims when
// every k-th person of the circle is killed (the problem of Josephus)
std::vector< std::string > tangentopoli( List< std::string > L , int k )
{
 std::vector< std::string > order;
 auto p = L.head();
 while( ! L.isEmpty() ) {
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

// Esercizio 4.4: Caio, Sempronio, Tizio, Tullio and k = 3
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

// Esercizio 4.5: L = 4, 6, 7, 3, 2, 5 becomes 7, 7, 7, 3, 3, 3, 5, 5, 5, 5
ADS_TEST( "ex4.5" )
{
 using V = std::vector< int >;
 List< int > L{ 4 , 6 , 7 , 3 , 2 , 5 };
 replicate( L , L.head() , 0 );
 CHECK( items( L ) == V( { 7 , 7 , 7 , 3 , 3 , 3 , 5 , 5 , 5 , 5 } ) );
 List< int > E{ 2 , 4 };   // only even items: nothing is left
 replicate( E , E.head() , 0 );
 CHECK( E.isEmpty() );
 List< int > O{ 1 , 3 };   // no even item: the odd ones stay as they are
 replicate( O , O.head() , 0 );
 CHECK( items( O ) == V( { 1 , 3 } ) );
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

 bool isEmpty( int s ) const { return( f_top[ s ] == 0 ); }

 void push( int s , int v ) {
  if( f_top[ 0 ] + f_top[ 1 ] == f_A.size() )
   throw( std::logic_error( "TwoStacks::push: full vector" ) );
  const std::size_t i = s == 0 ? f_top[ 0 ] : f_A.size() - 1 - f_top[ 1 ];
  f_A[ i ] = v;
  ++f_top[ s ];
  }

 int top( int s ) const {
  if( isEmpty( s ) )
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

// Esercizio 4.7: two stacks share the vector until it is full
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
 CHECK( T.isEmpty( b ) );
 CHECK_THROWS( T.pop( b ) , std::logic_error );
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
 while( ! S.isEmpty() )   // END: the calls return in turn
  S.pop();
 }

}  // end( namespace )

// Esercizio 4.8: the same lists as firstN()
ADS_TEST( "ex4.8" )
{
 for( int n : { 0 , 1 , 5 , 50 } ) {
  List< int > L1 , M1 , L2 , M2;
  firstN( n , L1 , M1 );
  firstN_iterative( n , L2 , M2 );
  CHECK( items( L1 ) == items( L2 ) );
  CHECK( items( M1 ) == items( M2 ) );
  }
 }

// Esercizio 4.9: both ends, and a stack and a queue made of a dequeue
ADS_TEST( "ex4.9" )
{
 Dequeue< int > D;
 CHECK_THROWS( D.removeHead() , std::logic_error );
 D.insertTail( 2 ); D.insertHead( 1 ); D.insertTail( 3 );
 CHECK( D.head() == 1 && D.tail() == 3 );
 CHECK( D.removeTail() == 3 );
 CHECK( D.removeHead() == 1 );
 CHECK( D.removeHead() == 2 );
 CHECK( D.isEmpty() );
 CHECK_THROWS( D.removeTail() , std::logic_error );
 }

namespace {

// Esercizio 4.10: a QUEUE as a singly linked list with a pointer to the
// first cell and one to the last, so that enqueue() takes O( 1 ) time too
template< class T >
class LinkedQueue {
 struct Cell { T value; Cell * next; };
 Cell * f_testa = nullptr;
 Cell * f_fondo = nullptr;
 public:
 using Item = T;
 LinkedQueue( void ) = default;
 LinkedQueue( const LinkedQueue & ) = delete;
 LinkedQueue & operator=( const LinkedQueue & ) = delete;
 ~LinkedQueue() { while( ! isEmpty() ) dequeue(); }
 bool isEmpty( void ) const { return( ! f_testa ); }
 void enqueue( const T & v ) {
  Cell * c = new Cell{ v , nullptr };
  ( f_fondo ? f_fondo->next : f_testa ) = c;
  f_fondo = c;
  }
 T dequeue( void ) {
  if( isEmpty() )
   throw( std::logic_error( "LinkedQueue::dequeue: empty queue" ) );
  Cell * c = f_testa;
  T v = std::move( c->value );
  f_testa = c->next;
  if( ! f_testa )
   f_fondo = nullptr;
  delete c;
  return( v );
  }
 const T & top( void ) const {
  if( isEmpty() )
   throw( std::logic_error( "LinkedQueue::top: empty queue" ) );
  return( f_testa->value );
  }
 };

static_assert( Queue< LinkedQueue< int > > );

}  // end( namespace )

// Esercizio 4.10: FIFO, also after being emptied
ADS_TEST( "ex4.10" )
{
 LinkedQueue< int > Q;
 CHECK_THROWS( Q.dequeue() , std::logic_error );
 for( int i = 0 ; i < 100 ; ++i )
  Q.enqueue( i );
 for( int i = 0 ; i < 100 ; ++i )
  CHECK( Q.dequeue() == i );
 CHECK( Q.isEmpty() );
 Q.enqueue( 7 );
 CHECK( Q.top() == 7 );
 }

// Esercizio 4.11: sorted by key, the values of a key in their first order
ADS_TEST( "ex4.11" )
{
 using P = std::vector< std::pair< int , std::string > >;
 const P A = { { 2 , "c" } , { 0 , "a" } , { 2 , "d" } , { 1 , "b" } ,
               { 0 , "e" } };
 CHECK( pigeonholeSort( A , 3 ) ==
        P( { { 0 , "a" } , { 0 , "e" } , { 1 , "b" } , { 2 , "c" } ,
             { 2 , "d" } } ) );
 CHECK( pigeonholeSort( P() , 4 ).empty() );
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
