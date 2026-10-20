/*--------------------------------------------------------------------------*/
/*---------------------------- File heaps.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered priority queues beyond the d-ary heap: the radix
 * heap and the pairing heap, each against a reference of the standard
 * library on random operations and on the edge cases, at the level 3 of
 * the contracts.
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
#include "ADS/priority/DialQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/priority/SequenceHeap.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <limits>
#include <functional>
#include <map>
#include <queue>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <utility>
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

// the bucket queue on its cells: g on both sides of the heads kept in the
// layer (0 to 3) and in its vector (4 and beyond, up to 200), against a
// set by least f, largest g and last pushed, the visit of the standard
// interface in the same order, the cells freed and taken again, strings
// that move out of their cells, and a layer emptied and filled again with
// a smaller g
ADS_TEST( "bucket-queue-cells" )
{
 BucketQueue< std::string > Q;
 std::set< std::tuple< int , int , int , std::string > > R;
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 6000 ; ++t )
  if( R.empty() || ( g() % 5 < 3 ) ) {
   const int f = int( g() % 30 );
   const int h = ( g() % 2 ) ? int( g() % 6 ) : int( g() % 200 );
   const std::string x = std::to_string( t ) + std::string( 20 , 'x' );
   Q.push( x , f , h );
   R.emplace( f , -h , -t , x );
   }
  else {
   CHECK( Q.top() == std::get< 3 >( *R.begin() ) );
   CHECK( Q.pop() == std::get< 3 >( *R.begin() ) );
   R.erase( R.begin() );
   }
 CHECK( Q.size() == R.size() );
 auto it = Q.begin();                       // the order of the extractions
 for( const auto & e : R ) {
  CHECK( it != Q.end() && *it == std::get< 3 >( e ) );
  ++it;
  }
 CHECK( it == Q.end() );
 while( ! R.empty() ) {
  const auto e = Q.popEntry();
  CHECK( e.item == std::get< 3 >( *R.begin() ) );
  CHECK( e.f == std::get< 0 >( *R.begin() ) &&
         e.g == -std::get< 1 >( *R.begin() ) );
  R.erase( R.begin() );
  }
 CHECK( Q.empty() && Q.begin() == Q.end() );
 Q.push( "a" , 2 , 150 );                   // a layer with a large g,
 CHECK( Q.pop() == "a" );                   // emptied, and filled again
 Q.push( "b" , 2 , 0 );
 Q.push( "c" , 2 , 4 );
 CHECK( Q.topG() == 4 && Q.pop() == "c" && Q.topG() == 0 );
 CHECK( Q.pop() == "b" && Q.empty() );
 }

/// the Dial queue of window for c against a multiset, the keys pushed up
/// to C above the last one extracted
template< class K >
void dial( unsigned seed , std::size_t c , std::uint64_t C , int steps )
{
 DialQueue< int , K > Q( c );
 std::multiset< K > R;
 std::mt19937_64 g( seed );
 K last = std::is_signed_v< K > ? K( -1000000 ) : K( 0 );
 for( int t = 0 ; t < steps ; ++t )
  if( R.empty() || ( g() % 5 < 3 ) ) {
   const K k = K( last + K( g() % ( C + 1 ) ) );
   Q.push( t , k );
   R.insert( k );
   }
  else {
   CHECK( Q.topKey() == *R.begin() );
   last = Q.topKey();
   Q.pop();
   R.erase( R.begin() );
   }
 CHECK( Q.size() == R.size() );
 while( ! R.empty() ) {
  CHECK( Q.topKey() == *R.begin() );
  Q.pop();
  R.erase( R.begin() );
  }
 CHECK( Q.empty() );
 }

// the Dial queue against a multiset: keys within the window (never the
// overflow), up to 10 and 1000 times beyond it (through the overflow), on
// windows of 64 and 1024 buckets, signed and unsigned keys; the edge
// cases: an empty queue violates the contracts, a key below the last
// extracted as well, equal keys come out last pushed first, a key just
// at the edge of the window, any key after clear(), a window of one word
// of bits and the wrap around the circle
ADS_TEST( "dial-queue" )
{
 DialQueue< int > E( 10 );
 CHECK( E.window() == 64 );
 CHECK_THROWS( (void) E.top() , contract_violation );
 CHECK_THROWS( E.pop() , contract_violation );
 E.push( 0 , 50u );                         // the window from 50
 CHECK( E.pop() == 0 );
 E.push( 1 , 100u );
 E.push( 2 , 100u );
 E.push( 3 , 113u );                        // the last bucket of the window
 E.push( 4 , 114u );                        // the first beyond it
 CHECK( E.top() == 2 && E.pop() == 2 && E.pop() == 1 );
 CHECK_THROWS( E.push( 5 , 99u ) , contract_violation );
 CHECK( E.topKey() == 113u && E.pop() == 3 && E.pop() == 4 );
 CHECK( E.empty() );
 E.clear();
 E.push( 6 , 7u );                          // any key after clear()
 E.push( 7 , 7u + 64u * 5u );
 CHECK( E.pop() == 6 && E.topKey() == 7u + 320u && E.pop() == 7 );
 dial< std::uint32_t >( 1 , 63 , 63 , 5000 );        // in the window
 dial< std::uint32_t >( 2 , 63 , 640 , 5000 );       // overflow
 dial< std::uint32_t >( 3 , 1023 , 1000 , 5000 );
 dial< std::uint64_t >( 4 , 1023 , 1000000 , 5000 );
 dial< std::int32_t >( 5 , 100 , 1000 , 5000 );
 dial< std::uint32_t >( 6 , 255 , 255 , 30000 );     // past 1024 items
 dial< std::uint32_t >( 7 , 63 , 0 , 2000 );         // all equal
 }

/// the radix heap of digits of B bits against a multiset of ( key , id ),
/// on keys pushed at most C above the last one extracted
template< class K , unsigned B >
void radix( unsigned seed , std::uint64_t C , int steps )
{
 RadixHeap< int , K , B > H;
 std::multiset< K > R;
 std::mt19937_64 g( seed );
 K last = std::is_signed_v< K > ? std::numeric_limits< K >::min() / 2 : 0;
 for( int t = 0 ; t < steps ; ++t )
  if( R.empty() || ( g() % 5 < 3 ) ) {
   const K k = K( last + K( g() % ( C + 1 ) ) );
   H.push( t , k );
   R.insert( k );
   }
  else {
   CHECK( H.peekKey() == *R.begin() && H.topKey() == *R.begin() );
   last = H.topKey();
   H.pop();
   R.erase( R.begin() );
   }
 CHECK( H.size() == R.size() );
 while( ! R.empty() ) {
  CHECK( H.topKey() == *R.begin() );
  H.pop();
  R.erase( R.begin() );
  }
 CHECK( H.empty() );
 }

// the radix heap against a multiset, for digits of 1, 3, 4 and 8 bits, on
// keys of 8, 32 and 64 bits, signed and unsigned, in bands of width 0
// (all equal), 1, 1000 and the whole type; the edge cases: an empty heap
// violates the contracts, a key below the last extracted as well, equal
// keys come out last pushed first, the extremes of the types, clear()
// lets any key in again, and items with a destructor
ADS_TEST( "radix-heap" )
{
 RadixHeap< int > E;
 CHECK_THROWS( (void) E.top() , contract_violation );
 CHECK_THROWS( E.pop() , contract_violation );
 E.push( 1 , 10u );
 E.push( 2 , 10u );
 E.push( 3 , 10u );
 CHECK( E.top() == 3 && E.topKey() == 10u );
 CHECK( E.pop() == 3 && E.pop() == 2 );
 CHECK_THROWS( E.push( 4 , 9u ) , contract_violation );
 E.push( 4 , 10u );
 CHECK( E.pop() == 4 );
 E.push( 8 , 20u );                         // peekKey() keeps the base,
 CHECK( E.pop() == 1 && E.peekKey() == 20u );   // so 15 may come next
 E.push( 9 , 15u );
 CHECK( E.topKey() == 15u && E.pop() == 9 && E.pop() == 8 );
 E.clear();
 E.push( 5 , 0u );                          // any key after clear()
 CHECK( E.topKey() == 0u && E.size() == 1 );

 RadixHeap< char , std::int64_t , 4 > X;    // the extremes
 X.push( 'c' , LLONG_MAX );
 X.push( 'a' , LLONG_MIN );
 X.push( 'b' , 0 );
 CHECK( X.pop() == 'a' && X.pop() == 'b' && X.topKey() == LLONG_MAX );
 CHECK( X.pop() == 'c' && X.empty() );
 RadixHeap< int , std::uint8_t , 3 > S;     // a type of 8 bits, rows of
 for( int k = 255 ; k >= 0 ; --k )          // 3 + 3 + 2 bits
  S.push( k , std::uint8_t( k ) );
 for( int k = 0 ; k < 256 ; ++k )
  CHECK( S.topKey() == k && S.pop() == k );

 RadixHeap< std::string , unsigned > N;     // items with a destructor
 for( int i = 0 ; i < 100 ; ++i )
  N.push( std::string( 40 , char( 'a' + i % 26 ) ) , unsigned( i % 7 ) );
 CHECK( N.pop().size() == 40 && N.size() == 99 );

 radix< std::uint32_t , 1 >( 1 , 1000 , 4000 );
 radix< std::uint32_t , 4 >( 2 , 0 , 2000 );
 radix< std::uint32_t , 4 >( 3 , 1 , 4000 );
 radix< std::uint32_t , 4 >( 4 , 1000 , 4000 );
 radix< std::uint64_t , 8 >( 5 , 1000 , 4000 );
 radix< std::uint64_t , 3 >( 6 , std::uint64_t( 1 ) << 40 , 4000 );
 radix< std::int32_t , 4 >( 7 , 1000 , 4000 );
 radix< std::int64_t , 4 >( 8 , std::uint64_t( 1 ) << 40 , 4000 );
 radix< std::uint64_t , 4 >( 9 , 100 , 20000 );   // past 1024 items
 }

/// the heap H against a map of the priorities by
/// handle, on random inserts, deleteMin()s and decreases to a random
/// priority, to the minimum and to the same priority
template< class H >
void handled( unsigned seed , int steps , int range )
{
 H Q;
 std::map< typename H::Handle , int > R;   // handle -> priority
 std::set< std::pair< int , typename H::Handle > > O;
 std::mt19937 g( seed );
 for( int t = 0 ; t < steps ; ++t ) {
  const unsigned c = g() % 10;
  if( R.empty() || ( c < 4 ) ) {
   const int p = int( g() % unsigned( range ) );
   auto h = Q.insert( t , p );
   R[ h ] = p;
   O.insert( { p , h } );
   }
  else if( c < 7 ) {
   auto it = R.begin();
   std::advance( it , g() % R.size() );
   auto h = it->first;
   const int p = it->second;
   const int q = ( c == 4 ) ? O.begin()->first - 1 :   // a new minimum
                 ( c == 5 ) ? p :                      // the same
                 p - int( g() % unsigned( range ) );
   O.erase( { p , h } );
   Q.decrease( h , q );
   it->second = q;
   O.insert( { q , h } );
   }
  else {
   CHECK( Q.minPriority() == O.begin()->first );
   const int v = Q.min();
   auto h = std::find_if( R.begin() , R.end() , [ & ]( auto & e ) {
    return( e.first->value == v && e.second == O.begin()->first ); } );
   CHECK( h != R.end() );
   CHECK( Q.deleteMin() == v );
   O.erase( { h->second , h->first } );
   R.erase( h );
   }
  CHECK( Q.size() == int( R.size() ) );
  }
 while( ! O.empty() ) {
  CHECK( Q.minPriority() == O.begin()->first );
  Q.deleteMin();
  O.erase( O.begin() );
  }
 CHECK( Q.empty() );
 }

// the pairing heap, two-pass and auxiliary two-pass, against a map of the
// handles, with decreases to a new minimum, to the same priority and to a
// random lower one; the edge cases: the empty heap violates the
// contracts, an increase as well, one item, all equal priorities, sorted
// and reverse sorted inserts, a decrease of the root and of a node of the
// list, a move, items with a destructor left in the heap
template< class H , class HS >
void handled_edges( void )
{
 H E;
 CHECK_THROWS( (void) E.min() , contract_violation );
 CHECK_THROWS( E.deleteMin() , contract_violation );
 auto h = E.insert( 7 , 5 );
 CHECK_THROWS( E.decrease( h , 6 ) , contract_violation );
 E.decrease( h , 5 );                       // the root, to the same
 E.decrease( h , 1 );
 CHECK( E.min() == 7 && E.minPriority() == 1 && E.size() == 1 );
 CHECK( E.deleteMin() == 7 && E.empty() );
 for( int dir = 0 ; dir < 3 ; ++dir ) {     // sorted, reversed, equal
  H Q;
  std::vector< typename H::Handle > hs;
  for( int i = 0 ; i < 300 ; ++i )
   hs.push_back( Q.insert( i , dir == 0 ? i : dir == 1 ? 300 - i : 4 ) );
  Q.decrease( hs[ 150 ] , -1 );             // a node of the list (Aux)
  CHECK( Q.min() == 150 && Q.deleteMin() == 150 );
  int last = INT_MIN;
  while( ! Q.empty() ) {
   CHECK( Q.minPriority() >= last );
   last = Q.minPriority();
   Q.deleteMin();
   }
  }
 HS S;                                      // a move, and items with a
 for( int i = 0 ; i < 50 ; ++i )            // destructor left in it
  S.insert( std::string( 30 , char( 'a' + i % 26 ) ) , i % 9 );
 S.deleteMin();
 HS M( std::move( S ) );
 CHECK( S.empty() && M.size() == 49 && M.minPriority() == 0 );
 }

ADS_TEST( "pairing-heap" )
{
 handled_edges< PairingHeap< int , int > ,
                PairingHeap< std::string , int > >();
 handled_edges< PairingHeap< int , int , true > ,
                PairingHeap< std::string , int , true > >();
 using P2 = PairingHeap< int , int >;
 using PA = PairingHeap< int , int , true >;
 handled< P2 >( 1 , 3000 , 100 );
 handled< PA >( 2 , 3000 , 100 );
 handled< P2 >( 3 , 3000 , 3 );             // many ties
 handled< PA >( 4 , 3000 , 3 );
 handled< P2 >( 5 , 6000 , 1000000 );       // past 1024 nodes
 handled< PA >( 6 , 6000 , 1000000 );
 }

/// the sequence heap against std::priority_queue on steps random pushes
/// and pops, the pushes a fraction push of 10, the keys in [ 0 , range )
void sequence( unsigned seed , int steps , unsigned push , int range )
{
 SequenceHeap< int > H;
 std::priority_queue< int , std::vector< int > , std::greater<> > R;
 std::mt19937 g( seed );
 for( int t = 0 ; t < steps ; ++t )
  if( R.empty() || ( g() % 10 < push ) ) {
   const int x = int( g() % unsigned( range ) );
   H.push( x );
   R.push( x );
   }
  else {
   CHECK( H.top() == R.top() && H.pop() == R.top() );
   R.pop();
   }
 CHECK( H.size() == R.size() );
 while( ! R.empty() ) {
  CHECK( H.pop() == R.top() );
  R.pop();
  }
 CHECK( H.empty() );
 }

/// a bulk of n pushes of keys in [ 0 , range ) to a fresh heap, a pop by
/// the scan, k more pushes into the bulk, and then random pushes and pops
/// (the bulk by the incremental quicksort, the rest by the sequences)
/// against std::priority_queue, down to empty and once more
template< class T , class Make >
void bulk( unsigned seed , std::size_t n , int k , Make && make )
{
 SequenceHeap< T > H;
 std::priority_queue< T , std::vector< T > , std::greater<> > R;
 std::mt19937 g( seed );
 for( int round = 0 ; round < 2 ; ++round ) {
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const T x = make( g );
   H.push( x );
   R.push( x );
   }
  CHECK( H.pop() == R.top() );               // the scan of the bulk
  R.pop();
  for( int i = 0 ; i < k ; ++i ) {           // still in the bulk
   const T x = make( g );
   H.push( x );
   R.push( x );
   }
  CHECK( H.top() == R.top() );               // the bulk closed
  for( std::size_t t = 0 ; t < 2 * n ; ++t )
   if( g() % 2 ) {
    const T x = make( g );
    H.push( x );
    R.push( x );
    }
   else if( ! R.empty() ) {
    CHECK( H.pop() == R.top() );
    R.pop();
    }
  while( ! R.empty() ) {
   CHECK( H.pop() == R.top() );
   R.pop();
   }
  CHECK( H.empty() );
  }
 }

// the sequence heap against std::priority_queue: a bulk of pushes to an
// empty heap (one scan for a first look, then the incremental quicksort,
// on random keys, on 3 keys, on strings, within one buffer); with few
// items (all in the insertion heap), past one insertion heap (a sequence
// in the group 1, merged with the deletion buffer), past k sequences (the group 2), with
// the pops between, with many equal keys and with all equal; the edge
// cases: the empty heap violates the contracts, sorted and reversed
// pushes, pops that empty the deletion buffer while the groups have
// items, strings, a pushed item of the heap itself, clear(), and a max
// heap by std::greater
ADS_TEST( "sequence-heap" )
{
 SequenceHeap< int > E;
 CHECK_THROWS( (void) E.top() , contract_violation );
 CHECK_THROWS( E.pop() , contract_violation );
 E.push( 3 );
 CHECK( E.top() == 3 && E.pop() == 3 && E.empty() );
 sequence( 1 , 500 , 6 , 1000 );
 sequence( 2 , 3000 , 7 , 1000 );
 sequence( 3 , 3000 , 7 , 2 );               // many equal
 sequence( 4 , 3000 , 7 , 1 );               // all equal
 sequence( 5 , 60000 , 9 , 1000000 );        // past the group 1
 sequence( 6 , 200000 , 6 , 100000 );        // pops all along
 const std::size_t big = SequenceHeap< int >::M * SequenceHeap< int >::K * 3;
 for( int dir = 0 ; dir < 2 ; ++dir ) {      // sorted, reversed
  SequenceHeap< int > S;
  for( std::size_t i = 0 ; i < big ; ++i )
   S.push( dir ? int( big - i ) : int( i ) );
  for( std::size_t i = 0 ; i < big ; ++i )
   CHECK( S.pop() == ( dir ? int( i + 1 ) : int( i ) ) );
  CHECK( S.empty() );
  }
 SequenceHeap< std::string > T;              // strings, and an item of
 for( int i = 0 ; i < 1000 ; ++i )           // the heap pushed again
  T.push( std::to_string( 100000 + i * 7919 % 1000 ) );
 for( int i = 0 ; i < 600 ; ++i )
  T.push( T.top() );
 std::string last;
 while( ! T.empty() ) {
  const std::string x = T.pop();
  CHECK( last <= x );
  last = x;
  }
 bulk< int >( 7 , 100000 , 10 , []( auto & g ) {   // a bulk of random
  return( int( g() % 1000000 ) ); } );             // keys, of 3 keys,
 bulk< int >( 8 , 50000 , 0 , []( auto & g ) {     // and of strings
  return( int( g() % 3 ) ); } );
 bulk< std::string >( 9 , 20000 , 5 , []( auto & g ) {
  return( std::to_string( g() % 100000 ) ); } );
 bulk< int >( 10 , 3000 , 3 , []( auto & g ) {     // within one buffer
  return( int( g() % 100 ) ); } );
 {                                           // the keys of OrderKey.h:
  SequenceHeap< std::pair< int , short > > A;    // a pair with negative
  SequenceHeap< double , std::greater<> > B;     // items, doubles largest
  std::priority_queue< std::pair< int , short > ,   // first
   std::vector< std::pair< int , short > > , std::greater<> > RA;
  std::priority_queue< double > RB;
  std::mt19937 g( 12 );
  for( int t = 0 ; t < 30000 ; ++t )
   if( RA.empty() || ( g() % 3 ) ) {
    const std::pair< int , short > p( int( g() % 2001 ) - 1000 ,
                                      short( int( g() % 201 ) - 100 ) );
    const double d = double( int( g() % 2001 ) - 1000 ) / 7;
    A.push( p );
    RA.push( p );
    B.push( d );
    RB.push( d );
    }
   else {
    CHECK( A.pop() == RA.top() && B.pop() == RB.top() );
    RA.pop();
    RB.pop();
    }
  }
 SequenceHeap< int , std::greater<> > X;     // the largest first
 for( int i = 0 ; i < 2000 ; ++i ) X.push( i % 777 );
 CHECK( X.top() == 776 && X.size() == 2000 );
 X.clear();
 CHECK( X.empty() );
 X.push( 1 );
 CHECK( X.pop() == 1 );
 }

// the heap of the book with its items in a pool: the slot of deleteMin()
// taken by the next insert(), a move that keeps the handles valid, a
// move-assignment over a queue with items, strings left in the queue
// (destroyed with it), and the capacity 0
ADS_TEST( "heap-priority-queue-pool" )
{
 HeapPriorityQueue< std::string , int , 4 > Q( 100 );
 std::vector< HeapPriorityQueue< std::string , int , 4 >::Handle > hs;
 for( int i = 0 ; i < 50 ; ++i )
  hs.push_back( Q.insert( std::string( 30 , char( 'a' + i % 26 ) ) ,
                          100 - i ) );
 const std::string m = Q.min();
 auto * slot = hs[ 49 ];
 CHECK( Q.deleteMin() == m );
 auto h = Q.insert( "new" , 1000 );          // the slot freed last
 CHECK( h == slot && h->value == "new" );
 hs[ 49 ] = h;
 HeapPriorityQueue< std::string , int , 4 > M( std::move( Q ) );
 CHECK( Q.empty() && M.size() == 50 );
 M.decrease( hs[ 0 ] , -5 );                 // a handle of the moved one
 CHECK( M.minPriority() == -5 && M.min() == hs[ 0 ]->value );
 HeapPriorityQueue< std::string , int , 4 > N( 10 );
 N.insert( "x" , 1 );
 N = std::move( M );
 CHECK( N.size() == 50 && N.minPriority() == -5 );
 HeapPriorityQueue< int > Z( 0 );
 CHECK( Z.empty() && Z.capacity() == 0 );
 CHECK_THROWS( Z.insert( 1 , 1 ) , contract_violation );
 }

/// the heap with pointers of L, its nodes in a pool, against a multiset:
/// random inserts and deleteMin()s, a move, a move-assignment over a heap
/// with items, strings left in the heap
template< bool L >
void linked( void )
{
 LinkedHeapPriorityQueue< std::string , int , L > Q;
 std::multiset< int > R;
 std::mt19937 g( L ? 7 : 8 );
 for( int t = 0 ; t < 3000 ; ++t )
  if( R.empty() || ( g() % 3 ) ) {
   const int p = int( g() % 500 );
   Q.insert( std::string( 20 , char( 'a' + p % 26 ) ) , p );
   R.insert( p );
   }
  else {
   CHECK( Q.minPriority() == *R.begin() );
   Q.deleteMin();
   R.erase( R.begin() );
   }
 LinkedHeapPriorityQueue< std::string , int , L > M( std::move( Q ) );
 CHECK( Q.empty() && M.size() == int( R.size() ) );
 LinkedHeapPriorityQueue< std::string , int , L > N;
 N.insert( "x" , -1 );
 N = std::move( M );
 CHECK( N.size() == int( R.size() ) && N.minPriority() == *R.begin() );
 Q.insert( "again" , 3 );                   // the one moved from, reused
 CHECK( Q.min() == "again" && Q.deleteMin() == "again" && Q.empty() );
 }

// the heap with pointers, both ways of finding the last leaf, its nodes in
// a pool: against a multiset, with moves and items left in it
ADS_TEST( "linked-heap-pool" )
{
 linked< false >();
 linked< true >();
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File heaps.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
