/*--------------------------------------------------------------------------*/
/*-------------------------- File engineered.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered structures, beyond the book: the bucket queue and
 * the d-ary heap of plain items, the hash table with control bytes, the
 * hash table of pointers, the arena, the pool, the allocator on huge
 * pages, the hashing with random tables, and the queues between threads.
 * Each one is checked against a reference of the standard library on
 * random operations, at the level 3 of the contracts, and the queues
 * between threads on real threads.
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

#include "ADS/concurrent/MPSCInbox.h"
#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/ADT.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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

// the bucket queue against a set ordered by least f, largest g and last
// pushed; negative keys and an empty queue violate the contracts; ties on
// both keys come out last pushed first, a layer emptied and freed takes a
// lower g, a key below the lazy least f is found, the keys 0 and an
// unsigned type of keys work
ADS_TEST( "bucket-queue" )
{
 BucketQueue< int > Q;
 CHECK_THROWS( (void) Q.top() , contract_violation );
 CHECK_THROWS( Q.pop() , contract_violation );
 CHECK_THROWS( Q.popEntry() , contract_violation );
 CHECK_THROWS( Q.push( 0 , -1 , 0 ) , contract_violation );
 std::mt19937 g( 1 );
 std::set< std::tuple< int , int , int , int > > R;   // f , -g , -t , x
 for( int t = 0 ; t < 5000 ; ++t )
  if( R.empty() || ( g() % 3 ) ) {
   const int f = int( g() % 40 ) , h = int( g() % 15 );
   Q.push( t , f , h );
   R.emplace( f , -h , -t , t );
   }
  else {
   const auto [ f , mg , mt , x ] = *R.begin();
   if( t % 2 ) {                            // by top(), and by popEntry()
    CHECK( Q.topF() == f && Q.topG() == -mg && Q.top() == x );
    CHECK( Q.pop() == x );
    }
   else {
    const auto e = Q.popEntry();
    CHECK( e.item == x && e.f == f && e.g == -mg );
    }
   R.erase( R.begin() );
   }
 CHECK( Q.size() == R.size() );
 Q.clear();
 CHECK( Q.empty() );
 Q.push( 7 , 3 , 1 );
 CHECK( Q.pop() == 7 && Q.empty() );
 CHECK_THROWS( Q.push( 0 , 0 , -1 ) , contract_violation );
 CHECK_THROWS( (void) Q.topF() , contract_violation );
 CHECK_THROWS( (void) Q.topG() , contract_violation );
 BucketQueue< int > Z;                          // all ties: LIFO
 for( int x = 0 ; x < 5 ; ++x ) Z.push( x , 0 , 0 );
 for( int x = 5 ; x-- > 0 ; ) CHECK( Z.pop() == x );
 CHECK( Z.empty() && Z.begin() == Z.end() );
 CHECK_THROWS( Z.pop() , contract_violation );  // emptied after use
 Z.push( 1 , 3 , 4 );
 CHECK( Z.pop() == 1 );                         // the layer 3 is freed
 Z.push( 2 , 3 , 1 );                           // and taken again, lower g
 CHECK( Z.topF() == 3 && Z.topG() == 1 && Z.pop() == 2 );
 Z.push( 3 , 9 , 0 );
 CHECK( Z.topF() == 9 );                        // the least f moved to 9
 Z.push( 4 , 2 , 0 );                           // and back below it
 CHECK( Z.topF() == 2 && Z.pop() == 4 && Z.topF() == 9 && Z.pop() == 3 );
 BucketQueue< int , unsigned > U;               // unsigned keys
 U.push( 1 , 5u , 0u );
 U.push( 2 , 0u , 0u );
 U.push( 3 , 0u , 7u );
 CHECK( U.pop() == 3 && U.pop() == 2 && U.pop() == 1 && U.empty() );
 }

// the d-ary heap of plain items for D = 2 , 3 , 4 , 8 against
// std::priority_queue, the items carrying f and g, the least f and then the
// largest g out first; the edge cases: n = 0 , 1 , D , D + 1 items, one
// item (popped onto itself), all equal items, sorted and reverse sorted
// pushes, the extremes of int, an odd D, a pushed item that lives in the
// heap while the vector grows, and the empty heap after clear(); there is
// no decrease-key to test
struct Entry {
 int f , g , id;
 };

struct EntryBetter {
 bool operator()( const Entry & a , const Entry & b ) const {
  return( ( a.f < b.f ) || ( ( a.f == b.f ) && ( a.g > b.g ) ) ||
          ( ( a.f == b.f ) && ( a.g == b.g ) && ( a.id < b.id ) ) );
  }
 };

template< unsigned D >
void dary( unsigned seed )
{
 DaryHeap< Entry , EntryBetter , D > H;
 CHECK_THROWS( (void) H.top() , contract_violation );
 CHECK_THROWS( H.pop() , contract_violation );
 auto worse = []( const Entry & a , const Entry & b ) {
  return( EntryBetter()( b , a ) ); };
 std::priority_queue< Entry , std::vector< Entry > , decltype( worse ) >
  R( worse );
 std::mt19937 g( seed );
 H.reserve( 100 );
 for( int t = 0 ; t < 3000 ; ++t )
  if( R.empty() || ( g() % 3 ) ) {
   const Entry e{ int( g() % 50 ) , int( g() % 10 ) , t };
   H.push( e );
   R.push( e );
   }
  else {
   CHECK( H.top().id == R.top().id && H.pop().id == R.top().id );
   R.pop();
   }
 CHECK( H.size() == R.size() );
 }

/// the heap of arity D on n = 0 , 1 , D , D + 1 items, distinct and all
/// equal, against std::priority_queue; the ties come out by better()
template< unsigned D >
void dary_edges( void )
{
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                        std::size_t( D ) , std::size_t( D + 1 ) } )
  for( int same = 0 ; same < 2 ; ++same ) {
   DaryHeap< std::pair< int , int > , std::less<> , D > H;
   std::priority_queue< std::pair< int , int > ,
                        std::vector< std::pair< int , int > > ,
                        std::greater<> > R;
   for( std::size_t i = 0 ; i < n ; ++i ) {   // reversed: all move up
    const std::pair< int , int > x( same ? 7 : int( n - i ) ,
                                    same ? 0 : int( i ) );
    H.push( x );
    R.push( x );
    }
   CHECK( H.size() == n && std::size_t( H.end() - H.begin() ) == n );
   while( ! R.empty() ) {
    CHECK( H.top() == R.top() && H.pop() == R.top() );
    R.pop();
    }
   CHECK( H.empty() );
   }
 }

ADS_TEST( "dary-heap" )
{
 dary< 2 >( 2 );
 dary< 3 >( 5 );
 dary< 4 >( 3 );
 dary< 8 >( 4 );
 dary_edges< 2 >();
 dary_edges< 3 >();
 dary_edges< 4 >();
 dary_edges< 8 >();
 {                                             // the pushed item aliases
  DaryHeap< std::string > A;                   // the heap, which grows
  A.push( "b" );
  for( int i = 0 ; i < 40 ; ++i ) A.push( A.top() );
  A.push( "a" );
  CHECK( A.size() == 42 && A.pop() == "a" && A.top() == "b" );
  }
 for( std::size_t n = 1 ; n <= 33 ; ++n )      // an item of the heap that
  for( std::size_t k = 0 ; k < n ; ++k ) {     // moves up, pushed when the
   DaryHeap< std::string , std::less<> , 2 > A;   // vector is full or not
   std::multiset< std::string > R;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::string s( 24 , char( 'a' + ( i * 7 ) % 26 ) );   // no SSO
    A.push( s );
    R.insert( s );
    }
   const std::string & x = A.begin()[ k ];
   R.insert( x );
   A.push( x );
   for( const auto & s : R )
    CHECK( A.pop() == s );
   CHECK( A.empty() );
   }
 DaryHeap< int > I;                             // the least first
 for( int x : { 5 , 3 , 9 , 1 } ) I.push( x );
 CHECK( I.pop() == 1 && I.pop() == 3 && I.size() == 2 );
 I.clear();
 CHECK( I.empty() && I.begin() == I.end() );
 CHECK_THROWS( (void) I.top() , contract_violation );
 CHECK_THROWS( I.pop() , contract_violation );
 DaryHeap< std::string > O;                     // the root is the last
 O.push( "only" );
 CHECK( O.top() == "only" && O.pop() == "only" && O.empty() );
 DaryHeap< int , std::greater< int > , 3 > X;   // the largest first
 for( int x : { INT_MIN , 0 , INT_MAX , 0 , INT_MAX , INT_MIN } )
  X.push( x );
 for( int x : { INT_MAX , INT_MAX , 0 , 0 , INT_MIN , INT_MIN } )
  CHECK( X.pop() == x );
 CHECK( X.empty() );
 for( int dir = 0 ; dir < 3 ; ++dir ) {         // up, down, all equal
  DaryHeap< int , std::less< int > , 2 > H;
  H.reserve( 0 );
  for( int k = 0 ; k < 100 ; ++k )
   H.push( dir == 0 ? k : dir == 1 ? 99 - k : 42 );
  for( int k = 0 ; k < 100 ; ++k )
   CHECK( H.pop() == ( dir == 2 ? 42 : k ) );
  CHECK( H.empty() );
  }
 }

// the table of pointers to the nodes of an arena, found by their states,
// against std::unordered_map, with the hashes of a mix and with hashes
// whose low bits are all 0, which the Fibonacci hashing spreads anyway;
// the edge cases: the capacity rounded up to a power of 2 at least 16, the
// growth exactly past 7/10 of the places, the empty table, and many keys
// with the same hash, whose first place is the last one, so that the
// probes wrap around the end
struct Node {
 std::uint64_t state;
 int g;
 };

ADS_TEST( "pointer-table" )
{
 for( int kind = 0 ; kind < 2 ; ++kind ) {
  Arena< Node > N( 64 );
  PointerTable< Node > T;
  std::unordered_map< std::uint64_t , Node * > R;
  std::mt19937 gen( 5 + kind );
  for( int op = 0 ; op < 20000 ; ++op ) {
   const std::uint64_t s = gen() % 6000;
   const std::uint64_t h = kind ? s << 20 : splitmix64( s );
   T.prefetch( h );
   const auto [ p , fresh ] = T.find_or_insert( s , h , [ & ]( void ) {
    return( N.make( Node{ s , op } ) ); } );
   const auto it = R.find( s );
   CHECK( fresh == ( it == R.end() ) );
   if( fresh ) R[ s ] = p; else CHECK( it->second == p );
   CHECK( T.find( s , h ) == p && p->state == s );
   }
  CHECK( T.size() == R.size() && N.size() == R.size() );
  CHECK( T.size() * 10 <= T.capacity() * 7 );
  std::size_t n = 0;
  T.forEach( [ & ]( Node * ) { ++n; } );
  CHECK( n == R.size() );
  CHECK( T.find( std::uint64_t( 999999 ) , splitmix64( 999999 ) ) ==
         nullptr );
  }
 auto byG = []( const Node * p , int g ) { return( p->g == g ); };
 Arena< Node > M;
 PointerTable< Node , decltype( byG ) > G( 4 , byG );   // any equality
 CHECK( G.capacity() == 16 );
 auto make3 = [ & ]( void ) { return( M.make( Node{ 0 , 3 } ) ); };
 const auto a = G.find_or_insert( 3 , 3 , make3 );
 CHECK( a.second && ( G.find_or_insert( 3 , 3 , make3 ).first == a.first ) );
 auto nil = []( void ) { return( static_cast< Node * >( nullptr ) ); };
 CHECK_THROWS( G.find_or_insert( 4 , 4 , nil ) , contract_violation );
 CHECK( G.size() == 1 && G.find( 4 , 4 ) == nullptr );   // not inserted
 CHECK( PointerTable< Node >( 0 ).capacity() == 16 );
 CHECK( PointerTable< Node >( 16 ).capacity() == 16 );
 CHECK( PointerTable< Node >( 17 ).capacity() == 32 );
 Arena< Node > N;
 PointerTable< Node > E;
 CHECK( E.size() == 0 && E.find( std::uint64_t( 0 ) , 0 ) == nullptr );
 CHECK( E.begin() == E.end() );
 for( std::uint64_t s = 0 ; s < 12 ; ++s ) {
  E.find_or_insert( s , splitmix64( s ) , [ & ]( void ) {
   return( N.make( Node{ s , 0 } ) ); } );
  CHECK( E.capacity() == ( s < 11 ? 16u : 32u ) );   // 12 > 0.7 * 16
  }
 std::uint64_t h = 0;                          // the last place of 1024
 while( fibonacciPlace( h , 10 ) != 1023 ) ++h;
 PointerTable< Node > C( 1024 );
 for( std::uint64_t s = 0 ; s < 50 ; ++s ) {
  const auto r = C.find_or_insert( s , h , [ & ]( void ) {
   return( N.make( Node{ s , 1 } ) ); } );
  CHECK( r.second && r.first->state == s );
  }
 for( std::uint64_t s = 0 ; s < 50 ; ++s )
  CHECK( C.find( s , h ) && C.find( s , h )->state == s );
 CHECK( C.find( std::uint64_t( 50 ) , h ) == nullptr );
 CHECK( C.size() == 50 && C.capacity() == 1024 );
 // moved from: empty, found nothing, refilled; clear() keeps the places,
 // reserve() leaves no growth up to its n, and no shrink below it
 PointerTable< Node > P( std::move( C ) );
 CHECK( P.size() == 50 && P.find( std::uint64_t( 7 ) , h )->state == 7 );
 CHECK( C.size() == 0 && C.find( std::uint64_t( 7 ) , h ) == nullptr );
 CHECK( C.begin() == C.end() );
 C.prefetch( h );
 const auto r = C.find_or_insert( std::uint64_t( 7 ) , h , [ & ]( void ) {
  return( N.make( Node{ 7 , 2 } ) ); } );
 CHECK( r.second && C.size() == 1 && C.capacity() == 16 );
 PointerTable< Node > A;
 A = std::move( P );
 CHECK( A.size() == 50 && P.size() == 0 &&
        P.find( std::uint64_t( 1 ) , h ) == nullptr );
 PointerTable< Node > & self = A;
 A = std::move( self );
 CHECK( A.size() == 50 && A.find( std::uint64_t( 49 ) , h ) );
 A.clear();
 CHECK( A.size() == 0 && A.capacity() == 1024 &&
        A.find( std::uint64_t( 3 ) , h ) == nullptr );
 PointerTable< Node > R;
 R.reserve( 700 );
 CHECK( R.capacity() == 1024 );                // 700 <= 0.7 * 1024
 for( std::uint64_t s = 0 ; s < 700 ; ++s )
  R.find_or_insert( s , splitmix64( s ) , [ & ]( void ) {
   return( N.make( Node{ s , 3 } ) ); } );
 CHECK( R.capacity() == 1024 && R.size() == 700 );
 R.reserve( 10 );
 CHECK( R.capacity() == 1024 );
 R.reserve( 717 );                             // 717 > 0.7 * 1024: 2048
 CHECK( R.capacity() == 2048 && R.size() == 700 );
 for( std::uint64_t s = 0 ; s < 700 ; ++s )
  CHECK( R.find( s , splitmix64( s ) )->state == s );
 P.reserve( 0 );
 CHECK( P.capacity() == 16 && P.size() == 0 );
 }
// the lookups of a const OpenHashTable from 4 threads at once, each against
// the keys inserted: the lookups write probes(), which must not race (a
// test for ThreadSanitizer), and the answers are those of one thread
ADS_TEST( "open-hash-readers" )
{
 OpenHashTable< long , long > T( 1024 , Probing::linear() );
 for( long k = 0 ; k < 500 ; ++k )
  T.insert( 2 * k , k );
 const auto & C = T;
 std::vector< int > wrong( 4 , 0 );
 std::vector< std::thread > W;
 for( int t = 0 ; t < 4 ; ++t )
  W.emplace_back( [ & , t ]( void ) {
   for( int r = 0 ; r < 50 ; ++r )
    for( long k = 0 ; k < 1000 ; ++k ) {
     const auto v = C.lookup( k );
     if( ( C.contains( k ) != ( k % 2 == 0 ) ) ||
         ( v.has_value() != ( k % 2 == 0 ) ) || ( v && ( *v != k / 2 ) ) )
      ++wrong[ t ];
     }
   } );
 for( auto & w : W ) w.join();
 CHECK( wrong == std::vector< int >( 4 , 0 ) );
 CHECK( C.probes() >= 1 );
 }


// the hash of FlatHashTable that declares itself avalanching, so that the
// table takes the key as its mixed hash and a test chooses the first group
// (the top bits), the fingerprint (the low 8 bits) and the bit of overflow
// (the bits 8 to 10) of each key; on 32 bits the table mixes it anyway
struct Raw {
 using is_avalanching = void;
 std::size_t operator()( std::uint64_t x ) const {
  return( std::size_t( x ) );
  }
 };

/// the key of Raw in the group g of a table of 2^lg groups, its low bits
/// low and j in the bits 11 and up
std::uint64_t raw( std::uint64_t g , int lg , std::uint64_t j ,
                   std::uint64_t low )
{
 return( ( g << ( 64 - lg ) ) | ( j << 11 ) | low );
 }

/// a hash that puts every key in one group with one fingerprint
struct Same {
 std::size_t operator()( std::uint64_t ) const { return( 42 ); }
 };

/// a value that counts its living copies
struct Alive {
 static long count;
 std::string s;
 Alive( void ) { ++count; }
 explicit Alive( std::string t ) : s( std::move( t ) ) { ++count; }
 Alive( const Alive & o ) : s( o.s ) { ++count; }
 Alive( Alive && o ) noexcept : s( std::move( o.s ) ) { ++count; }
 Alive & operator=( const Alive & ) = default;
 Alive & operator=( Alive && ) noexcept = default;
 ~Alive() { --count; }
 };
long Alive::count = 0;

/// a value whose copies throw once a budget runs out, and whose moves may
/// throw, so that a rebuild copies the pairs
struct Fussy {
 static long count , budget;
 std::string s;
 explicit Fussy( std::string t ) : s( std::move( t ) ) { ++count; }
 Fussy( const Fussy & o ) : s( o.s ) {
  if( budget-- == 0 ) throw std::runtime_error( "copy" );
  ++count;
  }
 Fussy( Fussy && o ) noexcept( false ) : s( o.s ) {
  if( budget-- == 0 ) throw std::runtime_error( "move" );
  ++count;
  }
 Fussy & operator=( const Fussy & ) = default;
 ~Fussy() { --count; }
 };
long Fussy::count = 0 , Fussy::budget = -1;

/// t against r on random insertions, removals and lookups of keys in
/// [ 0 , range ), where the range keeps many deleted places
template< class T , class R , class G >
void against( T & t , R & r , std::uint64_t range , int ops , G && key ,
              unsigned seed )
{
 std::mt19937_64 g( seed );
 for( int op = 0 ; op < ops ; ++op ) {
  const auto k = key( g() % range );
  switch( g() % 4 ) {
   case( 0 ):
   case( 1 ): {
    const auto v = key( g() % 1000 );
    CHECK( t.insert( k , v ) == ( r.find( k ) == r.end() ) );
    r[ k ] = v;
    break;
    }
   case( 2 ):
    CHECK( t.remove( k ) == ( r.erase( k ) == 1 ) );
    break;
   default: {
    const auto it = r.find( k );
    const auto o = t.lookup( k );
    CHECK( o.has_value() == ( it != r.end() ) );
    if( o ) CHECK( *o == it->second );
    }
   }
  }
 CHECK( t.size() == r.size() );
 std::size_t n = 0;
 for( const auto & [ k , v ] : t ) {
  ++n;
  CHECK( r.count( k ) == 1 && r.at( k ) == v );
  }
 CHECK( n == r.size() );
 }

static_assert( Dictionary< FlatHashTable< int , int > > );

/// the keys of t by its iterators forward, checked against the walk
/// backward from the end, the same keys in the reverse order
template< class T >
std::vector< std::uint64_t > walk( const T & t )
{
 std::vector< std::uint64_t > f , b;
 for( const auto & [ k , v ] : t ) f.push_back( k );
 for( auto e = t.end() ; e != t.begin() ; ) b.push_back( ( *--e ).first );
 std::reverse( b.begin() , b.end() );
 CHECK( f == b && f.size() == t.size() );
 return( f );
 }

// the product of 128 bits by 32 bits as by the machine, at the extremes
static_assert( detail::mulxPortable( 0 , 0x9e3779b97f4a7c15ULL ) == 0 );
static_assert( detail::mulxPortable( 1ULL << 63 , 2 ) == 1 );
static_assert( detail::mulxPortable( ~0ULL , ~0ULL ) == ~0ULL );
static_assert( detail::mulxPortable( 1ULL << 32 , 1ULL << 32 ) == 1 );

// the table with control bytes against std::unordered_map, on integers
// with the identity hash, random and clustered (multiples of the places,
// of the groups and of 2^32), on keys that are their own mixed hash, all
// with one fingerprint, and on strings; the edges: the product of the
// mixing on the machine and by 32 bits, the empty table with no places, a
// table for 1 pair, the growth exactly past 7 / 8 of the places, the
// removals down to empty and the reuse, many deleted places and then
// lookups of absent keys (which end), every key in one group with one
// fingerprint, the 15 places of a group with one fingerprint and the 16th
// key past them, the fingerprint 0 taken as 1, the bit of overflow set,
// the place emptied under it counted and reused, the bits cleared by a
// rebuild of the same size, a group that wraps around the end, the
// iterators across removals and erase( it ) while iterating, a value of
// the table inserted with a new key that grows it, a copy that throws in
// the rebuild and in the copy of the table, the copies and the moves, and
// no value leaked; the walk of the groups by the iterators, forward and
// backward: no places, after reserve(), after clear() and after a move, the
// first and the last group alone, two full groups, places removed under
// an iterator and skipped, the empty groups between them, erase() from the
// end back to the beginning, every rebuild of one overflowing group, and a
// self-move; run also with ADS_FLAT_SIMD=0, the broadword fallback
ADS_TEST( "flat-hash-table" )
{
 for( std::uint64_t a : { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 32 ,
                          1ULL << 63 , ~0ULL , 0x9e3779b97f4a7c15ULL ,
                          0x123456789abcdef0ULL } )
  for( std::uint64_t b : { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 63 ,
                           ~0ULL , 0x9e3779b97f4a7c15ULL } )
   CHECK( detail::mulx( a , b ) == detail::mulxPortable( a , b ) );
 {                                             // random, with tombstones
  FlatHashTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( T , R , 3000 , 60000 , []( std::uint64_t x ) { return( x ); } ,
           1 );
  against( T , R , 1 << 20 , 20000 , []( std::uint64_t x ) {
   return( x ); } , 2 );
  }
 for( std::uint64_t c : { std::uint64_t( 30720 ) , std::uint64_t( 2048 ) ,
                          std::uint64_t( 1 ) << 32 } ) {   // clustered
  FlatHashTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( T , R , 40000 , 60000 , [ c ]( std::uint64_t x ) {
   return( x * c ); } , 5 );
  }
 FlatHashTable< std::uint64_t , int > E;       // empty, no places
 CHECK( E.empty() && E.capacity() == 0 && E.load() == 0.0 );
 CHECK( ! E.lookup( 0 ) && E.find( 7 ) == nullptr && ! E.contains( 7 ) );
 CHECK( ! E.remove( 7 ) && E.begin() == E.end() && E.size() == 0 );
 E.clear();
 E.reserve( 0 );
 CHECK( E.capacity() == 0 && E.deleted() == 0 );
 FlatHashTable< std::uint64_t , int > One( 1 );   // room for 1 pair
 CHECK( One.capacity() == 30 && One.insert( 5 , 50 ) && One.size() == 1 );
 CHECK( ! One.insert( 5 , 51 ) && One.lookup( 5 ) == 51 );
 *One.find( 5 ) = 52;
 CHECK( One.lookup( 5 ) == 52 && One.capacity() == 30 );
 for( std::size_t m : { 30 , 60 , 240 } ) {    // 7 / 8 of m, then one more
  FlatHashTable< std::uint64_t , int > G;
  for( std::uint64_t k = 0 ; k < m - m / 8 ; ++k ) G.insert( k , 0 );
  CHECK( G.capacity() == m && G.size() == m - m / 8 );
  G.insert( m , 0 );
  CHECK( G.capacity() == 2 * m && G.size() == m - m / 8 + 1 );
  for( std::uint64_t k = 0 ; k <= m ; ++k )
   CHECK( G.contains( k ) == ( k < m - m / 8 || k == m ) );
  using Int = FlatHashTable< int , int >;
  CHECK( Int( m - m / 8 ).capacity() == m );
  CHECK( Int( m - m / 8 + 1 ).capacity() == 2 * m );
  }
 FlatHashTable< std::uint64_t , int , Same > S;   // one group, one tag
 for( std::uint64_t k = 0 ; k < 200 ; ++k ) CHECK( S.insert( k , int( k ) ) );
 for( std::uint64_t k = 0 ; k < 200 ; ++k ) CHECK( S.lookup( k ) == int( k ) );
 CHECK( ! S.contains( 200 ) );
 const std::size_t mS = S.capacity();
 for( std::uint64_t k = 0 ; k < 200 ; ++k ) CHECK( S.remove( k ) );
 CHECK( S.empty() && S.deleted() > 0 && S.capacity() == mS );
 for( std::uint64_t k = 200 ; k < 300 ; ++k ) CHECK( ! S.contains( k ) );
 for( std::uint64_t k = 0 ; k < 150 ; ++k ) S.insert( k , 1 );   // reused
 CHECK( S.size() == 150 && S.capacity() == mS );
 for( std::uint64_t k = 0 ; k < 150 ; ++k ) CHECK( S.lookup( k ) == 1 );
 {                          // churn at a steady size near the bound: the
                              // places counted recounted, few rebuilds
  FlatHashTable< std::uint64_t , std::uint64_t > C;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 7 );
  for( std::uint64_t k = 0 ; k < 1200 ; ++k ) {   // 1200 of 1920 places
   C.insert( k , k );
   R[ k ] = k;
   }
  const std::size_t m = C.capacity();
  CHECK( m == 1920 );
  std::vector< std::uint64_t > in( 1200 );
  for( std::uint64_t k = 0 ; k < 1200 ; ++k ) in[ k ] = k;
  for( std::uint64_t t = 0 ; t < 100000 ; ++t ) {   // one out, one in
   const std::size_t i = g() % in.size();
   CHECK( C.remove( in[ i ] ) && R.erase( in[ i ] ) == 1 );
   in[ i ] = 1200 + t;
   CHECK( C.insert( in[ i ] , t ) );
   R[ in[ i ] ] = t;
   const std::size_t c = C.capacity();
   CHECK( C.size() + C.deleted() <= c - c / 8 );
   }
  CHECK( C.capacity() <= 2 * m && C.size() == R.size() );
  for( const auto & [ k , v ] : R ) CHECK( C.lookup( k ) == v );
  for( const auto & [ k , v ] : C ) CHECK( R.at( k ) == v );
  }
 {                                             // deleted places churned
  FlatHashTable< std::uint64_t , int , Same > D;
  for( int round = 0 ; round < 50 ; ++round ) {
   for( std::uint64_t k = 0 ; k < 20 ; ++k ) D.insert( 100 * round + k , 0 );
   for( std::uint64_t k = 0 ; k < 20 ; ++k ) D.remove( 100 * round + k );
   CHECK( ! D.contains( 100 * round ) && D.capacity() <= 60 );
   }
  CHECK( D.empty() );
  }
 {                                             // one fingerprint, 2 groups
  FlatHashTable< std::uint64_t , int , Raw > P( 1 );
  CHECK( P.capacity() == 30 );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )   // group 0 full, all tag 5
   CHECK( P.insert( raw( 0 , 1 , j , 5 ) , int( j ) ) );
  CHECK( P.deleted() == 0 );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )
   CHECK( P.lookup( raw( 0 , 1 , j , 5 ) ) == int( j ) );
  CHECK( ! P.contains( raw( 0 , 1 , 15 , 5 ) ) );
  CHECK( P.insert( raw( 0 , 1 , 15 , 5 ) , 15 ) );   // past group 0
  CHECK( P.lookup( raw( 0 , 1 , 15 , 5 ) ) == 15 );
  CHECK( ! P.contains( raw( 0 , 1 , 16 , 5 ) ) );   // the same bit
  CHECK( ! P.contains( raw( 0 , 1 , 16 , 5 | 0x700 ) ) );   // another
  CHECK( P.remove( raw( 0 , 1 , 3 , 5 ) ) && P.deleted() == 1 );   // under
  CHECK( P.lookup( raw( 0 , 1 , 15 , 5 ) ) == 15 );   // still past it
  CHECK( P.insert( raw( 0 , 1 , 20 , 0 ) , 20 ) );   // the hole, tag 1
  CHECK( P.deleted() == 0 && P.lookup( raw( 0 , 1 , 20 , 0 ) ) == 20 );
  CHECK( P.insert( raw( 0 , 1 , 21 , 1 ) , 21 ) );   // also tag 1
  CHECK( P.lookup( raw( 0 , 1 , 21 , 1 ) ) == 21 );
  CHECK( ! P.contains( raw( 0 , 1 , 22 , 0 ) ) );
  CHECK( ! P.contains( raw( 0 , 1 , 20 , 1 ) ) );
  CHECK( P.remove( raw( 0 , 1 , 21 , 1 ) ) && P.deleted() == 0 );
  std::vector< std::uint64_t > in;            // all of them out
  for( const auto & [ k , v ] : P ) in.push_back( k );
  CHECK( in.size() == 16 );
  for( std::uint64_t k : in ) CHECK( P.remove( k ) );
  CHECK( P.empty() && P.deleted() == 15 && P.capacity() == 30 );
  for( std::uint64_t j = 0 ; j < 12 ; ++j )   // group 1, 12 + 15 = 27
   P.insert( raw( 1 , 1 , j , 9 ) , 0 );
  CHECK( P.deleted() == 15 && P.capacity() == 30 );
  P.insert( raw( 1 , 1 , 12 , 9 ) , 0 );      // rebuilt, the same size
  CHECK( P.deleted() == 0 && P.capacity() == 30 && P.size() == 13 );
  CHECK( P.remove( raw( 1 , 1 , 0 , 9 ) ) && P.deleted() == 0 );   // none
  for( std::uint64_t j = 1 ; j < 13 ; ++j )
   CHECK( P.contains( raw( 1 , 1 , j , 9 ) ) );
  }
 if constexpr( sizeof( std::size_t ) == 8 ) {
  FlatHashTable< std::uint64_t , std::uint64_t , Raw > F;   // one tag
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( F , R , 5000 , 30000 , []( std::uint64_t x ) {
   return( ( x << 40 ) | 5 ); } , 3 );
  FlatHashTable< std::uint64_t , int , Raw > W( 100 );   // 8 groups
  CHECK( W.capacity() == 120 );
  for( std::uint64_t j = 0 ; j < 45 ; ++j )   // from the last group: 7 , 0
   W.insert( raw( 7 , 3 , j , j + 1 ) , int( j ) );   // and 2
  CHECK( W.size() == 45 && W.capacity() == 120 && W.deleted() == 0 );
  for( std::uint64_t j = 0 ; j < 45 ; ++j )
   CHECK( W.lookup( raw( 7 , 3 , j , j + 1 ) ) == int( j ) );
  CHECK( ( *W.begin() ).first >> 61 == 7 );   // place 0, wrapped
  CHECK( ( *W.begin() ).first == raw( 7 , 3 , 15 , 16 ) );
  for( std::uint64_t j = 0 ; j < 45 ; j += 2 )
   W.remove( raw( 7 , 3 , j , j + 1 ) );
  CHECK( W.deleted() == 8 + 7 );               // in the groups 7 and 0
  for( std::uint64_t j = 0 ; j < 45 ; ++j )
   CHECK( W.contains( raw( 7 , 3 , j , j + 1 ) ) == ( j % 2 == 1 ) );
  }
 {                                             // iterators and erase( it )
  FlatHashTable< std::uint64_t , int > I;
  for( std::uint64_t k = 0 ; k < 100 ; ++k ) I.insert( k , int( k ) );
  auto it = I.begin();
  const std::uint64_t k0 = ( *it ).first;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   if( k != k0 && k % 3 ) I.remove( k );
  I.insert( k0 , -1 );                         // there: no rebuild
  CHECK( ( *it ).first == k0 && ( *it ).second == -1 );
  for( auto e = I.begin() ; e != I.end() ; )
   if( ( *e ).first % 2 ) e = I.erase( e ); else ++e;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   CHECK( I.contains( k ) == ( ( k == k0 || k % 3 == 0 ) && k % 2 == 0 ) );
  std::size_t n = 0;                           // backwards, the same pairs
  for( auto e = I.end() ; e != I.begin() ; ) { --e; ++n; }
  CHECK( n == I.size() );
  for( auto e = I.begin() ; e != I.end() ; ) e = I.erase( e );
  CHECK( I.empty() && I.begin() == I.end() );
  I.insert( 7 , 7 );                           // reused
  CHECK( I.size() == 1 && I.lookup( 7 ) == 7 );
  }
 {                                             // a value of the table
  FlatHashTable< std::uint64_t , std::string > V;
  for( std::uint64_t k = 0 ; k < 27 ; ++k )
   V.insert( k , "a value long enough for the heap " + std::to_string( k ) );
  CHECK( V.capacity() == 30 );
  V.insert( 100 , *V.find( 3 ) );              // grows: copied first
  CHECK( V.capacity() == 60 && *V.find( 100 ) == *V.find( 3 ) );
  }
 {                                             // copies that throw
  using FT = FlatHashTable< std::uint64_t , Fussy >;
  FT T;
  for( std::uint64_t k = 0 ; k < 27 ; ++k )
   T.insert( k , Fussy( "value " + std::to_string( k ) ) );
  CHECK( T.capacity() == 30 && Fussy::count == 27 );
  for( long b : { 0L , 5L } ) {                // the new pair, an old one
   Fussy::budget = b;
   CHECK_THROWS( T.insert( 99 , Fussy( "new" ) ) , std::runtime_error );
   Fussy::budget = -1;
   CHECK( T.size() == 27 && T.capacity() == 30 && ! T.contains( 99 ) );
   CHECK( Fussy::count == 27 );
   for( std::uint64_t k = 0 ; k < 27 ; ++k )
    CHECK( T.find( k )->s == "value " + std::to_string( k ) );
   }
  Fussy::budget = 7;                           // the copy of the table
  CHECK_THROWS( FT( T ) , std::runtime_error );
  Fussy::budget = -1;
  CHECK( Fussy::count == 27 );
  }
 CHECK( Fussy::count == 0 );
 {                                             // the walk of the groups
  FlatHashTable< std::uint64_t , int > N;     // no places, then no pairs
  CHECK( walk( N ).empty() && N.begin() == N.end() );
  N.reserve( 100 );
  CHECK( walk( N ).empty() && N.cbegin() == N.cend() );
  N.insert( 1 , 1 );
  N.clear();
  CHECK( walk( N ).empty() && N.capacity() > 0 );
  FlatHashTable< std::uint64_t , int > M( std::move( N ) );   // moved
  CHECK( walk( N ).empty() && walk( M ).empty() );
  }
 if constexpr( sizeof( std::size_t ) == 8 ) {
  FlatHashTable< std::uint64_t , int , Raw > W( 100 );   // 8 groups
  CHECK( W.capacity() == 120 );
  W.insert( raw( 7 , 3 , 0 , 1 ) , 0 );        // the last group alone
  CHECK( walk( W ) == std::vector< std::uint64_t >{ raw( 7 , 3 , 0 , 1 ) } );
  CHECK( W.erase( W.begin() ) == W.end() && W.empty() );
  W.insert( raw( 0 , 3 , 0 , 1 ) , 0 );        // the first group alone
  CHECK( walk( W ).size() == 1 );
  CHECK( ( *--W.end() ).first == raw( 0 , 3 , 0 , 1 ) );
  W.remove( raw( 0 , 3 , 0 , 1 ) );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )   // groups 2 and 7 full
   for( std::uint64_t g : { 2 , 7 } )
    W.insert( raw( g , 3 , j , j + 1 ) , int( j ) );
  CHECK( W.capacity() == 120 && walk( W ).size() == 30 );
  auto it = W.begin();                         // place 0 of group 2
  CHECK( ( *it ).first == raw( 2 , 3 , 0 , 1 ) );
  W.remove( raw( 2 , 3 , 1 , 2 ) );            // the next place, skipped
  W.remove( raw( 2 , 3 , 14 , 15 ) );          // the last of the group
  CHECK( ( *++it ).first == raw( 2 , 3 , 2 , 3 ) );
  for( int s = 0 ; s < 11 ; ++s ) ++it;
  CHECK( ( *it ).first == raw( 2 , 3 , 13 , 14 ) );
  CHECK( ( *++it ).first == raw( 7 , 3 , 0 , 1 ) );   // groups 3 to 6
  CHECK( ( *--it ).first == raw( 2 , 3 , 13 , 14 ) );
  auto last = W.end();                         // the last place full
  CHECK( ( *--last ).first == raw( 7 , 3 , 14 , 15 ) );
  CHECK( ++last == W.end() && walk( W ).size() == 28 );
  for( auto e = W.end() ; e != W.begin() ; )  // erase backwards
   e = W.erase( --e );
  CHECK( W.empty() && W.begin() == W.end() && W.capacity() == 120 );
  }
 {                                             // one group, overflowing
  FlatHashTable< std::uint64_t , int , Same > S;
  std::set< std::uint64_t > in;
  for( std::uint64_t k = 0 ; k < 100 ; ++k ) {
   S.insert( k , 0 );
   in.insert( k );
   const auto w = walk( S );                   // across every rebuild
   CHECK( std::set< std::uint64_t >( w.begin() , w.end() ) == in );
   }
  for( std::uint64_t k = 0 ; k < 100 ; k += 3 ) {
   S.remove( k );
   in.erase( k );
   }
  const auto w = walk( S );
  CHECK( std::set< std::uint64_t >( w.begin() , w.end() ) == in );
  auto & same = S;                             // self-move
  S = std::move( same );
  CHECK( S.size() == in.size() && walk( S ).size() == in.size() );
  }
 {                                             // strings, copies, moves
  FlatHashTable< std::string , Alive > A;
  std::unordered_map< std::string , std::string > R;
  std::mt19937 g( 4 );
  for( int op = 0 ; op < 20000 ; ++op ) {
   const std::string k = "key-of-some-length-" + std::to_string( g() % 900 );
   if( g() % 3 ) {
    A.insert( k , Alive( "value-" + std::to_string( op ) ) );
    R[ k ] = "value-" + std::to_string( op );
    }
   else
    CHECK( A.remove( k ) == ( R.erase( k ) == 1 ) );
   }
  CHECK( A.size() == R.size() && Alive::count == long( A.size() ) );
  FlatHashTable< std::string , Alive > B( A ) , C;   // copy
  CHECK( B.size() == A.size() && Alive::count == 2 * long( A.size() ) );
  CHECK( B.deleted() == A.deleted() && B.capacity() == A.capacity() );
  for( const auto & [ k , v ] : R ) CHECK( B.find( k )->s == v );
  C = std::move( B );
  CHECK( B.empty() && B.capacity() == 0 && C.size() == A.size() );
  B.insert( "again" , Alive( "x" ) );           // a moved-from table works
  CHECK( B.lookup( "again" )->s == "x" );
  const auto & same = C;                      // self-assignment
  C = same;
  A.clear();
  CHECK( A.empty() && Alive::count == long( C.size() ) + 1 );
  CHECK( A.deleted() == 0 && ! A.contains( R.begin()->first ) );
  A.insert( "after clear" , Alive( "y" ) );
  CHECK( A.size() == 1 && A.lookup( "after clear" )->s == "y" );
  FlatHashTable< std::string , Alive > M( std::move( C ) );
  CHECK( C.empty() && M.size() == R.size() );
  }
 CHECK( Alive::count == 0 );                   // nothing leaked
 }

// the arena: the addresses never change across many blocks, the objects
// are built from the arguments and destroyed with the arena; the edge
// cases: no object, blocks of one object, the last block exactly full, and
// a constructor that throws when a new block was just taken
struct Counted {
 static int alive;
 int v;
 explicit Counted( int x ) : v( x ) {
  if( x < 0 ) throw std::invalid_argument( "negative" );
  ++alive;
  }
 ~Counted() { --alive; }
 };
int Counted::alive = 0;

ADS_TEST( "arena" )
{
 CHECK_THROWS( Arena< int >( 0 ) , contract_violation );
 {
  Arena< Counted > A( 7 );
  std::vector< Counted * > P;
  for( int i = 0 ; i < 100 ; ++i ) P.push_back( A.make( i ) );
  for( int i = 0 ; i < 100 ; ++i ) CHECK( P[ i ]->v == i );
  CHECK( A.size() == 100 && Counted::alive == 100 );
  CHECK( A.bytes() == 15 * 7 * sizeof( Counted ) );
  for( int i = 1 ; i < 7 ; ++i ) CHECK( P[ i ] == P[ i - 1 ] + 1 );
  }
 CHECK( Counted::alive == 0 );
 {
  Arena< Counted > E( 1 );                      // nothing made
  CHECK( E.size() == 0 && E.bytes() == 0 && E.begin() == E.end() );
  Arena< Counted > O( 1 );                      // a block per object
  for( int i = 0 ; i < 3 ; ++i ) O.make( i );
  CHECK( O.bytes() == 3 * sizeof( Counted ) && Counted::alive == 3 );
  Arena< Counted > F( 4 );                      // two blocks, both full
  for( int i = 0 ; i < 8 ; ++i ) F.make( i );
  CHECK( F.bytes() == 8 * sizeof( Counted ) && Counted::alive == 11 );
  Arena< Counted > T( 2 );
  T.make( 0 );
  T.make( 1 );                                  // the block is full
  CHECK_THROWS( T.make( -1 ) , std::invalid_argument );
  CHECK( T.size() == 2 && Counted::alive == 13 );
  CHECK( T.make( 2 )->v == 2 && T.size() == 3 && T.bytes() ==
         4 * sizeof( Counted ) );              // the new block is used
  CHECK( T.begin()[ 2 ].v == 2 );
  }
 CHECK( Counted::alive == 0 );
 }

// the pool: the slot freed last is the next one made, the blocks double up
// to their limit and the objects keep their addresses across them, a long
// random sequence of make() and free() against the set of the objects
// alive; the edge cases: no object, blocks of one slot, a constructor that
// throws (its slot stays free), an aggregate built from its fields, the
// pools moved and swapped with their objects, and free() of nil
ADS_TEST( "pool" )
{
 CHECK_THROWS( ObjectPool< int >( 0 ) , contract_violation );
 {
  const std::size_t z = std::max( sizeof( Counted ) , sizeof( void * ) );
  const std::size_t h = ( 2 * sizeof( void * ) + z - 1 ) / z;   // header
  const auto at = []( const Counted * x ) {     // the address of a slot
   return( reinterpret_cast< std::uintptr_t >( x ) ); };
  ObjectPool< Counted > A( 64 );
  std::vector< Counted * > P;
  for( int i = 0 ; i < 100 ; ++i ) P.push_back( A.make( i ) );
  for( int i = 0 ; i < 100 ; ++i ) CHECK( P[ i ]->v == i );
  for( int i = 64 ; i < 100 ; ++i )          // blocks of 1, 2, ..., 64
   CHECK( at( P[ i ] ) == at( P[ i - 1 ] ) + z );
  CHECK( A.size() == 100 && Counted::alive == 100 );
  CHECK( A.bytes() == ( 127 + 7 * h ) * z );
  A.free( P[ 7 ] );
  A.free( P[ 3 ] );
  CHECK( A.size() == 98 && Counted::alive == 98 );
  CHECK( A.make( 1000 ) == P[ 3 ] && A.make( 1001 ) == P[ 7 ] );
  CHECK( P[ 3 ]->v == 1000 && P[ 7 ]->v == 1001 && P[ 8 ]->v == 8 );
  CHECK_THROWS( A.make( -1 ) , std::invalid_argument );
  CHECK( A.size() == 100 );                     // the slot of the throw first
  CHECK( at( A.make( 5 ) ) == at( P[ 99 ] ) + z );
  CHECK( at( A.make( 6 ) ) == at( P[ 99 ] ) + 2 * z );
  for( Counted * x : P ) A.free( x );
  CHECK( A.size() == 2 && Counted::alive == 2 );
  ObjectPool< Counted > B( std::move( A ) );    // the objects stay
  CHECK( A.size() == 0 && A.bytes() == 0 && B.size() == 2 );
  ObjectPool< Counted > C( 1 );                 // a block per slot
  Counted * c = C.make( 9 );
  C.swap( B );
  CHECK( B.size() == 1 && C.size() == 2 && B.bytes() == ( 1 + h ) * z );
  B.free( c );
  CHECK( Counted::alive == 2 );
  A = std::move( C );
  CHECK( A.size() == 2 && C.size() == 0 );
  CHECK( A.make( 3 )->v == 3 && Counted::alive == 3 );
  CHECK_THROWS( A.free( nullptr ) , contract_violation );
  ObjectPool< Counted > E;                      // nothing made
  CHECK( E.size() == 0 && E.bytes() == 0 );
  for( Counted * x = A.make( 4 ) ; x ; x = nullptr ) A.free( x );
  }
 CHECK( Counted::alive == 3 );                  // the pool destroys none
 Counted::alive = 0;

 struct Cell { long v; Cell * next; };          // an aggregate
 ObjectPool< Cell > L;
 Cell * h = nullptr;
 for( long i = 0 ; i < 5 ; ++i ) h = L.make( i , h );
 long t = 0;
 for( Cell * c = h ; c ; c = c->next ) t = 10 * t + c->v;
 CHECK( t == 43210 && L.size() == 5 );

 std::mt19937 g( 5 );
 ObjectPool< std::string > S( 100 );
 std::vector< std::string * > in;
 std::multiset< std::string > M;
 for( int op = 0 ; op < 20000 ; ++op )
  if( in.empty() || ( g() % 5 < 3 ) ) {
   const std::string x( 20 + g() % 10 , char( 'a' + g() % 26 ) );
   in.push_back( S.make( x ) );
   M.insert( x );
   }
  else {
   const std::size_t i = g() % in.size();
   M.erase( M.find( *in[ i ] ) );
   S.free( in[ i ] );
   in[ i ] = in.back();
   in.pop_back();
   }
 CHECK( S.size() == in.size() && S.size() == M.size() );
 std::multiset< std::string > N;
 for( auto * x : in ) N.insert( *x );
 CHECK( N == M );
 for( auto * x : in ) S.free( x );
 CHECK( S.size() == 0 );
 }

// the allocator on huge pages: a small request from std::allocator, a
// large one aligned to a huge page and wholly writable, the sizes just
// below and at one huge page, a vector that grows across the threshold
// with its items kept, the allocators equal across types, and a request
// too large for the address space
ADS_TEST( "huge-page-allocator" )
{
 using H = HugePageAllocator< long >;
 constexpr std::size_t P = H::PAGE / sizeof( long );
 H a;
 for( std::size_t n : { std::size_t( 1 ) , P - 1 , P , P + 1 , 3 * P } ) {
  long * p = a.allocate( n );
  if( n >= P )
   CHECK( reinterpret_cast< std::uintptr_t >( p ) % H::PAGE == 0 );
  for( std::size_t i = 0 ; i < n ; ++i ) p[ i ] = long( i );
  CHECK( p[ n - 1 ] == long( n - 1 ) && p[ 0 ] == 0 );
  a.deallocate( p , n );
  }
 std::vector< long , H > V;
 for( long i = 0 ; i < long( 2 * P ) ; ++i ) V.push_back( i );
 bool kept = true;
 for( long i = 0 ; i < long( V.size() ) ; ++i ) kept = kept && V[ i ] == i;
 CHECK( kept && V.size() == 2 * P );
 CHECK( H() == HugePageAllocator< char >() );
 CHECK_THROWS( (void) a.allocate( std::numeric_limits< std::size_t >::max()
                                  / 4 ) , std::bad_array_new_length );
 }

// the hashing with random tables: the first word of SplitMix64 from 0,
// a bijection on many inputs, the places of the Fibonacci hashing within
// the table, the update of a Zobrist hash equal to the hash computed again,
// and the tabulation spreading consecutive keys; the edge cases: the
// places of 0 and of b = 1 and 64, b out of [ 1 , 64 ], a Zobrist table of
// one feature with one value, an update that does not change the value,
// and every precondition of the Zobrist table
ADS_TEST( "random-hashing" )
{
 SplitMix64 g( 0 );
 CHECK( g() == 0xe220a8397b1dcdafULL );
 std::unordered_set< std::uint64_t > S;
 for( std::uint64_t x = 0 ; x < 100000 ; ++x ) S.insert( splitmix64( x ) );
 CHECK( S.size() == 100000 );
 for( int b : { 1 , 10 , 63 } )
  for( std::uint64_t h : { 0ULL , 12345ULL , ~0ULL } )
   CHECK( fibonacciPlace( h , b ) < ( 1ULL << b ) );
 CHECK( fibonacciPlace( ~0ULL , 64 ) != 0 );
 CHECK_THROWS( (void) fibonacciPlace( 1 , 0 ) , contract_violation );
 ZobristTable Z( 9 , 9 , 7 );
 std::vector< int > B = { 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 0 };
 std::uint64_t h = Z.hash( B );
 h = Z.update( h , 7 , 8 , 0 );                 // the hole moves left
 h = Z.update( h , 8 , 0 , 8 );
 std::swap( B[ 7 ] , B[ 8 ] );
 CHECK( h == Z.hash( B ) );
 CHECK_THROWS( (void) Z( 9 , 0 ) , contract_violation );
 CHECK_THROWS( (void) Z( 0 , 9 ) , contract_violation );
 CHECK( Z.update( h , 3 , 4 , 4 ) == h );       // a == b, no change
 CHECK_THROWS( (void) Z.update( h , 3 , 9 , 0 ) , contract_violation );
 CHECK_THROWS( (void) Z.update( h , 9 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( (void) Z.hash( std::vector< int >( 8 , 0 ) ) ,
               contract_violation );
 CHECK_THROWS( ZobristTable( 0 , 1 ) , contract_violation );
 CHECK_THROWS( ZobristTable( 1 , 0 ) , contract_violation );
 ZobristTable Z1( 1 , 1 );
 CHECK( Z1.hash( std::vector< int >( { 0 } ) ) == Z1( 0 , 0 ) );
 CHECK( ZobristTable( 1 , 1 , 2 )( 0 , 0 ) != Z1( 0 , 0 ) );
 for( int b : { 1 , 32 , 64 } ) CHECK( fibonacciPlace( 0 , b ) == 0 );
 CHECK( fibonacciPlace( 1 , 64 ) == 0x9e3779b97f4a7c15ULL );
 CHECK( fibonacciPlace( 1 , 1 ) == 1 && fibonacciPlace( 2 , 1 ) == 0 );
 CHECK_THROWS( (void) fibonacciPlace( 1 , 65 ) , contract_violation );
 CHECK_THROWS( (void) fibonacciPlace( 1 , -1 ) , contract_violation );
 SplitMix64 g2( 0 );
 (void) g2();
 CHECK( g2() == splitmix64( 0x9e3779b97f4a7c15ULL ) );   // the 2nd word
 TabulationHash T( 3 );
 std::unordered_set< std::uint64_t > U;
 std::vector< int > top( 16 , 0 );
 for( std::uint64_t x = 0 ; x < 4096 ; ++x ) {
  U.insert( T( x ) );
  ++top[ T( x ) >> 60 ];
  }
 CHECK( U.size() == 4096 );
 for( int c : top ) CHECK( c > 128 && c < 384 );   // about 256 each
 CHECK( T( 0 ) == TabulationHash( 3 )( 0 ) );   // the same seed
 CHECK( T( 0 ) != TabulationHash( 4 )( 0 ) && T( 0 ) != T( ~0ULL ) );
 }

// one producer and one consumer on real threads: the items in order, none
// lost, through a queue much smaller than their number; the edge cases:
// the capacities 0, 1 and not a power of 2, the least capacity 2, full
// and empty in turn, and the wrap-around of the indices on one thread
ADS_TEST( "spsc-queue" )
{
 CHECK_THROWS( SPSCQueue< int >( 3 ) , contract_violation );
 CHECK_THROWS( SPSCQueue< int >( 0 ) , contract_violation );
 CHECK_THROWS( SPSCQueue< int >( 1 ) , contract_violation );
 SPSCQueue< int > Q( 64 );
 CHECK( ! Q.pop() && Q.capacity() == 64 );
 const int n = 200000;
 std::thread producer( [ & ] {
  for( int i = 0 ; i < n ; )
   if( Q.push( i ) ) ++i;
  } );
 long long sum = 0;
 bool ordered = true;
 for( int next = 0 ; next < n ; )
  if( auto v = Q.pop() ) {
   ordered = ordered && ( *v == next );
   sum += *v;
   ++next;
   }
 producer.join();
 CHECK( ordered && sum == ( long long )( n ) * ( n - 1 ) / 2 &&
        Q.size() == 0 );
 SPSCQueue< int > F( 2 );
 CHECK( F.push( 1 ) && F.push( 2 ) && ! F.push( 3 ) && F.size() == 2 );
 CHECK( F.pop() == 1 && F.push( 3 ) && ! F.push( 4 ) );   // one freed
 CHECK( F.pop() == 2 && F.pop() == 3 && ! F.pop() && F.size() == 0 );
 SPSCQueue< int > W( 4 );                       // many laps of 4 places
 int in = 0 , out = 0;
 for( int lap = 0 ; lap < 1000 ; ++lap ) {
  while( W.push( in ) ) ++in;
  CHECK( W.size() == 4 );
  for( int k = 0 ; k < 1 + lap % 4 ; ++k ) CHECK( W.pop() == out++ );
  }
 while( auto v = W.pop() ) CHECK( *v == out++ );
 CHECK( out == in && W.size() == 0 );
 }

// four producers and one consumer on real threads: every message taken
// once, those of each producer in the order of sending; on one thread, the
// messages in the order of sending, one message alone, and a take after
// all was taken
ADS_TEST( "mpsc-inbox" )
{
 MPSCInbox< std::pair< int , int > > I;
 CHECK( I.empty() && I.take().empty() );
 const int p = 4 , n = 20000;
 std::vector< std::thread > T;
 for( int k = 0 ; k < p ; ++k )
  T.emplace_back( [ & , k ] {
   for( int i = 0 ; i < n ; ++i ) I.push( { k , i } );
   } );
 std::vector< int > next( p , 0 );
 bool ordered = true;
 for( int got = 0 ; got < p * n ; )
  for( auto [ k , i ] : I.take() ) {
   ordered = ordered && ( i == next[ k ] );
   ++next[ k ];
   ++got;
   }
 for( auto & t : T ) t.join();
 CHECK( ordered && I.empty() );
 for( int k = 0 ; k < p ; ++k ) CHECK( next[ k ] == n );
 MPSCInbox< int > S;
 S.push( 7 );
 CHECK( ! S.empty() && S.take() == std::vector< int >( { 7 } ) );
 for( int x : { 1 , 2 , 3 } ) S.push( x );
 CHECK( S.take() == std::vector< int >( { 1 , 2 , 3 } ) );
 CHECK( S.empty() && S.take().empty() );
 MPSCInbox< int > L;                            // freed if never taken
 L.push( 1 );
 L.push( 2 );
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
/*------------------------ End File engineered.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
