/*--------------------------------------------------------------------------*/
/*----------------------- File engineered_search.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the structures of the searches in large spaces of states, beyond
 * the book: the table of pointers sized in advance and on huge pages, the
 * heaps whose items are erased and raised by their handles, the pair of
 * indexed heaps by a key and by a bound, the B+-tree as an ordered
 * frontier, the arrays of packed fields and of trits built on the threads
 * of an executor, the Zobrist hash of a state of any range, the backoff of
 * the threads that wait and their placement on the processors, at the
 * level 3 of the contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/concurrent/Backoff.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/parallel/Affinity.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/IndexedHeapPair.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/tree/BTreeMap.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <span>
#include <thread>
#include <tuple>
#include <unordered_map>
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

/// a node of a search: its state and its cost
struct Node {
 std::uint64_t state;
 int g;
 };

// the places of slotsFor() at the edges of the load factor (0, 11 and 12
// objects, a power of 2 of them, the first n past each doubling), a table
// built with slotsFor( n ) or reserved for n that takes n objects with no
// growth, a reserve() below the size that changes nothing, the places on
// huge pages (the allocator of HugePageAllocator.h) and the hashes biased by
// an owner in their low bits, which the Fibonacci hashing spreads
ADS_TEST( "pointer-table-presize" )
{
 using T = PointerTable< Node >;
 CHECK( T::slotsFor( 0 ) == 16 && T::slotsFor( 11 ) == 16 );
 CHECK( T::slotsFor( 12 ) == 32 && T::slotsFor( 22 ) == 32 );
 CHECK( T::slotsFor( 23 ) == 64 );
 static_assert( T::slotsFor( 1 << 20 ) == ( 1 << 21 ) );
 for( std::size_t n = 1 ; n < 100000 ; n = n * 3 + 1 ) {
  const std::size_t m = T::slotsFor( n );
  CHECK( std::has_single_bit( m ) && ( n * 10 <= m * 7 ) );
  CHECK( ( m == 16 ) || ( n * 10 > ( m / 2 ) * 7 ) );   // the least one
  }
 Arena< Node > N;
 for( std::size_t n : { std::size_t( 11 ) , std::size_t( 12 ) ,
                        std::size_t( 700 ) , std::size_t( 5000 ) } ) {
  T A( T::slotsFor( n ) );
  T R;
  R.reserve( n );
  CHECK( R.capacity() == T::slotsFor( n ) );
  const std::size_t ca = A.capacity();
  for( std::uint64_t s = 0 ; s < n ; ++s )
   for( T * t : { &A , &R } )
    t->find_or_insert( s , splitmix64( s ) , [ & ]( void ) {
     return( N.make( Node{ s , 0 } ) ); } );
  CHECK( A.capacity() == ca && R.capacity() == T::slotsFor( n ) );
  CHECK( A.size() == n && R.size() == n );
  R.reserve( 1 );                               // below the size: no change
  CHECK( R.capacity() == T::slotsFor( n ) && R.size() == n );
  }
 const std::size_t n = test::scaled< std::size_t >( 200000 , 5000 );
 PointerTable< Node , StateEqual , HugePageAllocator< std::byte > >
  H( T::slotsFor( n ) );
 const std::size_t c = H.capacity();
 std::vector< Node * > P( n );
 for( std::uint64_t s = 0 ; s < n ; ++s ) {     // the hashes of owner 3
  const std::uint64_t h = ( splitmix64( s ) << 3 ) | 3;   // of 8
  H.prefetch( h );
  const auto [ p , fresh ] = H.find_or_insert( s , h , [ & ]( void ) {
   return( N.make( Node{ s , 3 } ) ); } );
  CHECK( fresh && p->state == s );
  P[ s ] = p;
  }
 CHECK( H.capacity() == c && H.size() == n );
 for( std::uint64_t s = 0 ; s < n ; s += 7 ) {
  const std::uint64_t h = ( splitmix64( s ) << 3 ) | 3;
  CHECK( H.find( s , h ) == P[ s ] );
  CHECK( ! H.find_or_insert( s , h , [ & ]( void ) {
   return( static_cast< Node * >( nullptr ) ); } ).second );
  }
 }

// the indexed heap against a map of the keys on random pushes, decreases,
// increases, updates either way and to the same key, erasures of the root,
// of the last item and of the middle, and pops; the edges: an erasure that
// empties the heap, the last item put in the hole of the root and of a leaf,
// and the contracts of increase(), update() and erase()
ADS_TEST( "indexed-heap-erase" )
{
 for( unsigned run = 0 ; run < 2 ; ++run ) {
  constexpr int n = 300;
  IndexedDaryHeap< int > H( n );
  std::map< int , int > K;                       // id -> key
  std::mt19937 g( 3 + run );
  auto least = [ & ]( void ) {
   int b = K.begin()->second;
   for( const auto & e : K ) b = std::min( b , e.second );
   return( b );
   };
  for( int op = 0 ; op < 20000 ; ++op ) {
   const int id = int( g() % n );
   const int k = int( g() % 1000 ) - 500;
   const unsigned c = g() % 7;
   if( ! H.contains( id ) ) {
    if( c < 5 ) { H.push( id , k ); K[ id ] = k; }
    }
   else if( c == 0 ) {
    const int d = std::min( k , K[ id ] );
    H.decrease( id , d );
    K[ id ] = d;
    }
   else if( c == 1 ) {
    const int u = std::max( k , K[ id ] );
    H.increase( id , u );
    K[ id ] = u;
    }
   else if( c == 2 ) { H.update( id , k ); K[ id ] = k; }
   else if( c == 3 ) { H.update( id , K[ id ] ); }   // the same key
   else if( c == 4 ) { H.erase( id ); K.erase( id ); }
   else if( c == 5 ) {                         // the root itself
    const int r = H.top().id;
    H.erase( r );
    K.erase( r );
    }
   else {
    const auto e = H.pop();
    CHECK( K.count( e.id ) && K[ e.id ] == e.key );
    K.erase( e.id );
    }
   CHECK( H.size() == K.size() );
   if( ! K.empty() ) {
    CHECK( H.top().key == least() );
    CHECK( H.key( H.top().id ) == H.top().key );
    }
   }
  for( const auto & e : K ) CHECK( H.contains( e.first ) );
  }
 IndexedDaryHeap< int , 2 > B( 8 );            // the holes, by hand:
 for( int k : { 0 , 1 , 100 , 2 , 3 , 101 , 102 , 4 } )   // the places in
  B.push( k < 100 ? k : k - 95 , k );           // the order of the pushes
 B.erase( 6 );                                  // the key 101: the last
 CHECK( B.key( 5 ) == 100 && B.size() == 7 );   // item, 4, goes up
 std::vector< int > O;
 IndexedDaryHeap< int , 2 > C = B;
 while( ! C.empty() ) O.push_back( C.pop().key );
 CHECK( O == std::vector< int >( { 0 , 1 , 2 , 3 , 4 , 100 , 102 } ) );
 B.erase( 7 );                                  // the last item itself
 B.erase( 0 );                                  // the root: 100 sinks
 CHECK( B.top().id == 1 && B.size() == 5 );
 while( ! B.empty() ) B.erase( B.top().id );
 CHECK( B.empty() );
 CHECK_THROWS( B.erase( 1 ) , contract_violation );
 CHECK_THROWS( B.update( 1 , 0 ) , contract_violation );
 B.push( 1 , 5 );
 CHECK_THROWS( B.increase( 1 , 4 ) , contract_violation );
 CHECK_THROWS( B.decrease( 1 , 6 ) , contract_violation );
 B.increase( 1 , 5 );                           // not less: allowed
 CHECK( B.key( 1 ) == 5 );
 B.erase( 1 );
 CHECK( B.empty() && ! B.contains( 1 ) );
 }

// the pairing heap, two-pass and auxiliary, against a set of ( priority ,
// id ) on random insertions, deletions of the minimum, decreases,
// increases, updates and erasures by handle; the erasure of the root, of
// the least item of the list beside the root, of the root when it is not
// the least, of a leaf and of the last item, an increase of the root and
// of the minimum, all at the level 3 of the contracts, which checks the
// order and the links after each operation on heaps of up to 1024 nodes
template< bool Aux >
void pairingErase( unsigned seed )
{
 using H = PairingHeap< int , int , Aux >;
 H Q;
 std::set< std::pair< int , int > > R;           // ( priority , id )
 std::unordered_map< int , typename H::Handle > h;
 std::unordered_map< int , int > p;
 std::mt19937 g( seed );
 int next = 0;
 auto drop = [ & ]( int id ) {
  R.erase( { p[ id ] , id } );
  h.erase( id );
  p.erase( id );
  };
 auto any = [ & ]( void ) {                     // an id in the heap
  auto it = h.begin();
  std::advance( it , g() % h.size() );
  return( it->first );
  };
 for( int op = 0 ; op < 6000 ; ++op ) {
  const unsigned c = R.empty() ? 0 : g() % 8;
  const int k = int( g() % 200 );
  if( c <= 1 ) {
   const int id = next++;
   h[ id ] = Q.insert( id , k );
   p[ id ] = k;
   R.emplace( k , id );
   }
  else if( c == 2 ) {
   const int q = Q.minPriority();
   CHECK( q == R.begin()->first );
   const int id = Q.deleteMin();
   CHECK( p[ id ] == q );
   drop( id );
   }
  else if( c == 3 ) {
   const int id = any() , d = std::min( k , p[ id ] );
   Q.decrease( h[ id ] , d );
   R.erase( { p[ id ] , id } );
   R.emplace( p[ id ] = d , id );
   }
  else if( c == 4 ) {
   const int id = any() , u = std::max( k , p[ id ] );
   Q.increase( h[ id ] , u );
   R.erase( { p[ id ] , id } );
   R.emplace( p[ id ] = u , id );
   }
  else if( c == 5 ) {
   const int id = any();
   Q.update( h[ id ] , k );
   R.erase( { p[ id ] , id } );
   R.emplace( p[ id ] = k , id );
   }
  else if( c == 6 ) {
   const int id = any();
   CHECK( Q.erase( h[ id ] ) == id );
   drop( id );
   }
  else {                                        // the minimum, by handle
   const int id = Q.min();
   CHECK( p[ id ] == R.begin()->first );
   if( g() % 2 ) {
    CHECK( Q.erase( h[ id ] ) == id );
    drop( id );
    }
   else {
    Q.increase( h[ id ] , p[ id ] + 1 + k );
    R.erase( { p[ id ] , id } );
    R.emplace( p[ id ] += 1 + k , id );
    }
   }
  CHECK( std::size_t( Q.size() ) == R.size() );
  if( ! R.empty() ) CHECK( Q.minPriority() == R.begin()->first );
  }
 while( ! R.empty() ) {                         // emptied by erasures
  const int id = any();
  CHECK( Q.erase( h[ id ] ) == id );
  drop( id );
  if( ! R.empty() ) CHECK( Q.minPriority() == R.begin()->first );
  }
 CHECK( Q.empty() );
 auto a = Q.insert( 1 , 10 );                    // one item: the root
 Q.increase( a , 20 );
 CHECK( Q.minPriority() == 20 );
 auto b = Q.insert( 2 , 5 );
 auto c = Q.insert( 3 , 30 );
 CHECK( Q.erase( b ) == 2 && Q.min() == 1 );    // the least
 CHECK( Q.erase( a ) == 1 && Q.min() == 3 );    // the root, alone
 CHECK_THROWS( Q.increase( c , 29 ) , contract_violation );
 CHECK( Q.erase( c ) == 3 && Q.empty() );
 }

ADS_TEST( "pairing-heap-erase" )
{
 pairingErase< false >( 1 );
 pairingErase< true >( 2 );
 PairingHeap< int , int , true > A;             // the root not the least
 auto r = A.insert( 0 , 50 );                   // the root
 auto x = A.insert( 1 , 10 );                   // the least, in the list
 A.insert( 2 , 70 );
 CHECK( A.min() == 1 );
 CHECK( A.erase( r ) == 0 && A.min() == 1 );    // the list goes on
 A.increase( x , 90 );
 CHECK( A.min() == 2 && A.size() == 2 );
 auto y = A.insert( 3 , 80 );                   // in the list, no child
 CHECK( A.erase( y ) == 3 && A.min() == 2 && A.size() == 2 );
 }

// the pair of heaps against two sets, by the key and by the bound, on
// random pushes, updates, erasures, pops by the key and by the bound;
// ties of the keys and of the bounds, an update to the same values, the
// last id taken out by each way, and the contracts
ADS_TEST( "indexed-heap-pair" )
{
 constexpr int n = 200;
 IndexedHeapPair< std::pair< int , int > , int > Q( n );
 std::set< std::tuple< int , int , int > > SK;   // key , id
 std::set< std::pair< int , int > > SB;          // bound , id
 std::vector< std::pair< int , int > > key( n );
 std::vector< int > bound( n );
 std::mt19937 g( 11 );
 auto out = [ & ]( int id ) {
  SK.erase( { key[ id ].first , key[ id ].second , id } );
  SB.erase( { bound[ id ] , id } );
  };
 for( int op = 0 ; op < 20000 ; ++op ) {
  const int id = int( g() % n );
  const std::pair< int , int > k( int( g() % 20 ) , - int( g() % 5 ) );
  const int b = int( g() % 30 );
  const unsigned c = g() % 6;
  if( ! Q.contains( id ) ) {
   if( c < 4 ) {
    Q.push( id , k , b );
    key[ id ] = k;
    bound[ id ] = b;
    SK.emplace( k.first , k.second , id );
    SB.emplace( b , id );
    }
   }
  else if( c == 0 ) {
   out( id );
   Q.update( id , k , b );
   key[ id ] = k;
   bound[ id ] = b;
   SK.emplace( k.first , k.second , id );
   SB.emplace( b , id );
   }
  else if( c == 1 ) Q.update( id , key[ id ] , bound[ id ] );
  else if( c == 2 ) { Q.erase( id ); out( id ); }
  else if( ( c == 3 ) && ! Q.empty() ) {
   const int t = Q.pop();
   CHECK( std::pair( key[ t ].first , key[ t ].second ) ==
          std::pair( std::get< 0 >( *SK.begin() ) ,
                     std::get< 1 >( *SK.begin() ) ) );
   out( t );
   }
  else if( ! Q.empty() ) {
   const int t = Q.popMinBound();
   CHECK( bound[ t ] == SB.begin()->first );
   out( t );
   }
  CHECK( Q.size() == SK.size() && SK.size() == SB.size() );
  if( ! Q.empty() ) {
   CHECK( Q.minBound() == SB.begin()->first );
   CHECK( Q.bound( Q.minBoundId() ) == Q.minBound() );
   CHECK( Q.topKey().first == std::get< 0 >( *SK.begin() ) &&
          Q.topKey().second == std::get< 1 >( *SK.begin() ) );
   CHECK( Q.key( Q.top() ) == Q.topKey() );
   }
  }
 Q.clear();
 CHECK( Q.empty() );
 CHECK_THROWS( (void) Q.pop() , contract_violation );
 CHECK_THROWS( (void) Q.popMinBound() , contract_violation );
 CHECK_THROWS( Q.erase( 3 ) , contract_violation );
 CHECK_THROWS( Q.update( 3 , { 0 , 0 } , 0 ) , contract_violation );
 Q.push( 3 , { 1 , 0 } , 9 );
 CHECK_THROWS( Q.push( 3 , { 1 , 0 } , 9 ) , contract_violation );
 CHECK_THROWS( Q.push( n , { 1 , 0 } , 9 ) , contract_violation );
 CHECK( Q.top() == 3 && Q.minBoundId() == 3 && Q.pop() == 3 );
 CHECK( Q.empty() );
 IndexedHeapPair< int > E( 0 );                 // no id at all
 CHECK( E.empty() && E.ids() == 0 );
 }

// the B+-tree as the ordered frontier of a search, keyed by ( f , -g ,
// id ): the first items from the least read in order and erased by their
// iterators while reading, against a set; the erasure of the first item,
// of the last, of every item from the least on, and lowerBound() on a key
// with no item, the tree emptied and filled again
ADS_TEST( "ordered-frontier" )
{
 using Key = std::tuple< int , int , int >;
 BTreeMap< Key , char , std::less< Key > , 256 > T;
 std::set< Key > R;
 std::mt19937 g( 21 );
 int id = 0;
 for( int round = 0 ; round < 60 ; ++round ) {
  for( int i = 0 ; i < 200 ; ++i ) {
   const Key k( int( g() % 50 ) , - int( g() % 10 ) , id++ );
   CHECK( T.insert( k , 0 ) );
   R.insert( k );
   }
  auto t = T.begin();                           // the first 64, every
  auto r = R.begin();                           // third one erased
  for( int i = 0 ; ( i < 64 ) && ( t != T.end() ) ; ++i ) {
   CHECK( r != R.end() && ( *t ).first == *r );
   if( i % 3 == 0 ) {
    t = T.erase( t );
    r = R.erase( r );
    }
   else {
    ++t;
    ++r;
    }
   }
  CHECK( T.size() == R.size() );
  if( round % 10 == 9 ) {                       // the last one
   auto l = std::ranges::prev( T.end() );
   CHECK( ( *l ).first == *R.rbegin() );
   CHECK( T.erase( l ) == T.end() );
   R.erase( std::prev( R.end() ) );
   }
  const Key probe( int( g() % 50 ) , 1 , 0 );   // g = -1: no such key
  auto lb = T.lowerBound( probe );
  auto rb = R.lower_bound( probe );
  CHECK( ( lb == T.end() ) == ( rb == R.end() ) );
  if( rb != R.end() ) CHECK( ( *lb ).first == *rb );
  }
 while( T.begin() != T.end() ) {                // all, from the least on
  CHECK( ( *T.begin() ).first == *R.begin() );
  T.erase( T.begin() );
  R.erase( R.begin() );
  }
 CHECK( T.empty() && R.empty() );
 T.insert( Key( 1 , 2 , 3 ) , 0 );
 CHECK( T.size() == 1 && std::get< 2 >( ( *T.begin() ).first ) == 3 );
 }

/// the checks of a PackedArray< B , S > against a vector of the values
template< unsigned B , bool S >
void packedArray( void )
{
 using P = PackedArray< B , S >;
 constexpr std::uint64_t M = P::MAX;
 std::mt19937_64 g( B );
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                        P::PER - 1 , P::PER , P::PER + 1 ,
                        3 * P::PER + 5 } ) {
  P A( n , M );                                 // all ones but past n
  CHECK( A.size() == n && A.words().size() == ( n + P::PER - 1 ) / P::PER );
  for( std::size_t i = 0 ; i < n ; ++i ) CHECK( A[ i ] == M );
  if( n % P::PER )
   CHECK( ( A.words().back() >> ( ( n % P::PER ) * B ) ) == 0 );
  std::vector< std::uint64_t > R( n , M );
  for( int k = 0 ; k < 200 && n ; ++k ) {
   const std::size_t i = g() % n;
   const std::uint64_t v = g() & M;
   CHECK( ! A.set( i , v ) );
   R[ i ] = v;
   }
  for( std::size_t i = 0 ; i < n ; ++i ) CHECK( A.get( i ) == R[ i ] );
  CHECK_THROWS( (void) A.get( n ) , contract_violation );
  P C;                                          // the same, by push_back
  for( std::uint64_t v : R ) C.push_back( v );
  CHECK( C == A );
  }
 if constexpr( B < 32 ) {                       // a value too large
  P A( 3 );
  if constexpr( S ) {
   CHECK( A.set( 1 , M + 1 ) && A.get( 1 ) == M && A.clamped() == 1 );
   CHECK( A.get( 0 ) == 0 && A.get( 2 ) == 0 );
   A.push_back( ~std::uint64_t( 0 ) );
   CHECK( A.get( 3 ) == M && A.clamped() == 2 );
   P F( 5 , M + 7 );
   CHECK( F.get( 4 ) == M && F.clamped() == 5 );
   }
  else {
   CHECK_THROWS( A.set( 1 , M + 1 ) , contract_violation );
   CHECK_THROWS( A.push_back( M + 1 ) , contract_violation );
   CHECK_THROWS( P( 2 , M + 1 ) , contract_violation );
   CHECK( A.get( 1 ) == 0 && A.size() == 3 && A.clamped() == 0 );
   }
  }
 // the parallel construction, on executors of 1, 3 and 8 threads, equal
 // to the sequential one; the blocks of 2048 words make many of them
 const std::size_t n = test::scaled< std::size_t >( 2048 * P::PER * 9 + 3 ,
                                                    4096 * P::PER / 64 + 3 );
 auto f = [ & ]( std::size_t i ) {
  return( splitmix64( i ) % ( S && ( B < 32 ) ? 2 * M + 2 : M + 1 ) );
  };
 P Q;
 std::size_t over = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  over += f( i ) > M;
  Q.push_back( f( i ) );
  }
 CHECK( Q.clamped() == ( S ? over : 0 ) );
 Sequential s;
 CHECK( P( s , n , f ) == Q );
 for( std::size_t p : { 1 , 3 , 8 } ) {
  ThreadPool pool( p );
  P W( pool , n , f );
  CHECK( W == Q && W.clamped() == Q.clamped() );
  if constexpr( ! S && ( B < 32 ) )             // a value too large, in a
   CHECK_THROWS( P( pool , n , [ & ]( std::size_t i ) {   // worker
    return( i == n / 2 ? M + 1 : 0 ); } ) , contract_violation );
  }
 P Z( s , 0 , f );
 CHECK( Z.empty() && Z.words().empty() );
 }

// the packed arrays of 1, 2, 4, 8, 16 and 32 bits, plain and saturated:
// the lengths 0, 1, a word less one, a word, a word and one, against a
// vector on random sets; the bits past the last field 0 after a fill of
// ones, push_back() equal to set(), a value too large clamped and counted
// or a violation of the contract, also in a worker of the parallel
// construction, which is equal to the sequential one on every executor
ADS_TEST( "packed-array" )
{
 packedArray< 1 , false >();
 packedArray< 2 , false >();
 packedArray< 4 , false >();
 packedArray< 8 , false >();
 packedArray< 16 , false >();
 packedArray< 32 , false >();
 packedArray< 1 , true >();
 packedArray< 4 , true >();
 packedArray< 8 , true >();
 packedArray< 32 , true >();
 CHECK( PackedArray< 2 >( 33 ).bytes() == 16 );   // 32 fields a word
 }

// the trits, five to a byte: every length up to 12 against a vector on
// random sets, a fill of 2s (the byte 242 and the digits past the end 0),
// a value 3 refused, push_back() equal to set(), 1.6 bits per value, the
// parallel construction equal to the sequential one on every executor
ADS_TEST( "trit-array" )
{
 std::mt19937 g( 5 );
 for( std::size_t n = 0 ; n <= 12 ; ++n ) {
  TritArray A( n , 2 );
  CHECK( A.size() == n && A.bytes() == ( n + 4 ) / 5 );
  if( n >= 5 ) CHECK( A.data()[ 0 ] == 242 );
  for( std::size_t i = 0 ; i < n ; ++i ) CHECK( A[ i ] == 2 );
  std::vector< unsigned > R( n , 2 );
  for( int k = 0 ; k < 100 && n ; ++k ) {
   const std::size_t i = g() % n;
   const unsigned v = g() % 3;
   A.set( i , v );
   R[ i ] = v;
   for( std::size_t j = 0 ; j < n ; ++j ) CHECK( A.get( j ) == R[ j ] );
   }
  TritArray C;
  for( unsigned v : R ) C.push_back( v );
  CHECK( C == A );
  CHECK_THROWS( (void) A.get( n ) , contract_violation );
  }
 TritArray A( 4 );
 CHECK_THROWS( A.set( 0 , 3 ) , contract_violation );
 CHECK_THROWS( A.push_back( 3 ) , contract_violation );
 CHECK_THROWS( TritArray( 2 , 3 ) , contract_violation );
 CHECK( A.get( 0 ) == 0 && A.size() == 4 );
 const std::size_t n = test::scaled< std::size_t >( 16384 * 5 * 7 + 4 ,
                                                    16384 * 2 + 4 );
 auto f = []( std::size_t i ) { return( unsigned( splitmix64( i ) % 3 ) ); };
 TritArray Q;
 for( std::size_t i = 0 ; i < n ; ++i ) Q.push_back( f( i ) );
 CHECK( Q.bytes() * 5 >= n && Q.bytes() * 5 < n + 5 );
 Sequential s;
 CHECK( TritArray( s , n , f ) == Q );
 for( std::size_t p : { 1 , 3 , 8 } ) {
  ThreadPool pool( p );
  CHECK( TritArray( pool , n , f ) == Q );
  CHECK_THROWS( TritArray( pool , n , [ & ]( std::size_t i ) {
   return( i == n - 1 ? 3u : 0u ); } ) , contract_violation );
  }
 }

// the Zobrist hash of a state as a vector, an array and a span, the same
// words; the update of a move of the sliding tiles (the blank and a tile
// swap places) equal to the hash computed again, and a span of the wrong
// size refused
ADS_TEST( "zobrist-ranges" )
{
 ZobristTable Z( 9 , 9 , 3 );
 std::array< std::uint8_t , 9 > S = { 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 0 };
 const std::vector< int > V( S.begin() , S.end() );
 const std::uint64_t h = Z.hash( S );
 CHECK( h == Z.hash( V ) && h == Z.hash( std::span( S ) ) );
 std::uint64_t u = h;                           // the blank goes up
 u = Z.update( u , 8 , 0 , 6 );
 u = Z.update( u , 5 , 6 , 0 );
 std::swap( S[ 5 ] , S[ 8 ] );
 CHECK( u == Z.hash( S ) && u != h );
 CHECK_THROWS( (void) Z.hash( std::span( S ).first( 8 ) ) ,
               contract_violation );
 }

// the steps of the backoff: S + 1 calls that spin, then yields, a reset
// that spins again, S = 0 (one call spins) and S out of [ 0 , 16 ]; four
// threads that wait with it for a flag set by a fifth one, each counting
// once
ADS_TEST( "backoff-threads" )
{
 for( int S : { 0 , 3 , 6 } ) {
  Backoff b( S );
  for( int k = 0 ; k <= S ; ++k ) {
   CHECK( b.spinning() && b.step() == k );
   b.pause();
   }
  CHECK( ! b.spinning() );
  b.pause();                                     // a yield
  CHECK( ! b.spinning() && b.step() == S + 1 );
  b.reset();
  CHECK( b.spinning() && b.step() == 0 );
  }
 CHECK_THROWS( Backoff( -1 ) , contract_violation );
 CHECK_THROWS( Backoff( 17 ) , contract_violation );
 std::atomic< bool > go{ false };
 std::atomic< int > done{ 0 };
 std::vector< std::thread > T;
 for( int t = 0 ; t < 4 ; ++t )
  T.emplace_back( [ & ]( void ) {
   Backoff b;
   while( ! go.load( std::memory_order_acquire ) )
    b.pause();
   done.fetch_add( 1 , std::memory_order_relaxed );
   } );
 std::thread s( [ & ]( void ) {
  std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
  go.store( true , std::memory_order_release );
  } );
 s.join();
 for( auto & t : T ) t.join();
 CHECK( done.load() == 4 );
 }

// the processors of the process sorted and distinct, as many as
// availableCpus() where the placement exists; a thread pinned to the
// processor i mod c runs there, for i past the count too, and pinThread()
// is false where the placement does not exist
ADS_TEST( "affinity" )
{
 const std::vector< int > & C = affinityCpus();
 CHECK( std::is_sorted( C.begin() , C.end() ) );
 CHECK( std::adjacent_find( C.begin() , C.end() ) == C.end() );
 CHECK( &C == &affinityCpus() );                // computed once
 if( C.empty() ) {
  CHECK( ! pinThread( 0 ) );
  return;
  }
 CHECK( availableCpus() == C.size() );
 for( unsigned i : { 0u , 1u , unsigned( C.size() ) , 1000u } ) {
  bool ok = false;
  int cpu = -2;
  std::thread t( [ & ]( void ) {
   ok = pinThread( i );
   cpu = currentCpu();
   } );
  t.join();
  if( ok ) CHECK( cpu == C[ i % C.size() ] );
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
/*---------------------- End File engineered_search.cpp --------------------*/
/*--------------------------------------------------------------------------*/
