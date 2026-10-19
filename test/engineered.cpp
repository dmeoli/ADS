/*--------------------------------------------------------------------------*/
/*-------------------------- File engineered.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered structures, beyond the book: the bucket queue and
 * the d-ary heap of plain items, the hash table with control bytes, the
 * hash table of pointers, the arena, the hashing with random tables, and
 * the queues between threads. Each one is checked against a reference of
 * the standard library on random operations, at the level 3 of the
 * contracts, and the queues between threads on real threads.
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
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
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
 CHECK( Q.isEmpty() );
 Q.push( 7 , 3 , 1 );
 CHECK( Q.pop() == 7 && Q.isEmpty() );
 CHECK_THROWS( Q.push( 0 , 0 , -1 ) , contract_violation );
 CHECK_THROWS( (void) Q.topF() , contract_violation );
 CHECK_THROWS( (void) Q.topG() , contract_violation );
 BucketQueue< int > Z;                          // all ties: LIFO
 for( int x = 0 ; x < 5 ; ++x ) Z.push( x , 0 , 0 );
 for( int x = 5 ; x-- > 0 ; ) CHECK( Z.pop() == x );
 CHECK( Z.isEmpty() && Z.begin() == Z.end() );
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
 CHECK( U.pop() == 3 && U.pop() == 2 && U.pop() == 1 && U.isEmpty() );
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
   CHECK( H.isEmpty() );
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
 DaryHeap< int > I;                             // the least first
 for( int x : { 5 , 3 , 9 , 1 } ) I.push( x );
 CHECK( I.pop() == 1 && I.pop() == 3 && I.size() == 2 );
 I.clear();
 CHECK( I.isEmpty() && I.begin() == I.end() );
 CHECK_THROWS( (void) I.top() , contract_violation );
 CHECK_THROWS( I.pop() , contract_violation );
 DaryHeap< std::string > O;                     // the root is the last
 O.push( "only" );
 CHECK( O.top() == "only" && O.pop() == "only" && O.isEmpty() );
 DaryHeap< int , std::greater< int > , 3 > X;   // the largest first
 for( int x : { INT_MIN , 0 , INT_MAX , 0 , INT_MAX , INT_MIN } )
  X.push( x );
 for( int x : { INT_MAX , INT_MAX , 0 , 0 , INT_MIN , INT_MIN } )
  CHECK( X.pop() == x );
 CHECK( X.isEmpty() );
 for( int dir = 0 ; dir < 3 ; ++dir ) {         // up, down, all equal
  DaryHeap< int , std::less< int > , 2 > H;
  H.reserve( 0 );
  for( int k = 0 ; k < 100 ; ++k )
   H.push( dir == 0 ? k : dir == 1 ? 99 - k : 42 );
  for( int k = 0 ; k < 100 ; ++k )
   CHECK( H.pop() == ( dir == 2 ? 42 : k ) );
  CHECK( H.isEmpty() );
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


// the hash of FlatHashTable that undoes its product by floor( 2^64 / phi ),
// so that the key is the mixed hash and a test chooses the first group
// (the top bits) and the fingerprint (the low 7 bits XOR the bits 32 to
// 38) of each key; on 32 bits the identity
struct Unmix {
 std::size_t operator()( std::uint64_t x ) const {
  std::uint64_t c = 0x9e3779b97f4a7c15ULL , r = c;   // r = 1 / c mod 2^64
  for( int i = 0 ; i < 6 ; ++i ) r *= 2 - c * r;
  return( std::size_t( x * r ) );
  }
 };

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

// the table with control bytes against std::unordered_map, on integers
// with the identity hash, on keys that are their own mixed hash, all with
// one fingerprint in different groups, and on strings; the edges: the
// empty table with no places, a table for 1 pair, the growth exactly past
// 7 / 8 of the places, the removals down to empty and the reuse, many
// deleted places and then lookups of absent keys (which end), every key
// in one group with one fingerprint, a group that wraps around the end,
// the iterators across removals and erase( it ) while iterating, the
// copies and the moves, and no value leaked
ADS_TEST( "flat-hash-table" )
{
 {                                             // random, with tombstones
  FlatHashTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( T , R , 3000 , 60000 , []( std::uint64_t x ) { return( x ); } ,
           1 );
  against( T , R , 1 << 20 , 20000 , []( std::uint64_t x ) {
   return( x ); } , 2 );
  }
 FlatHashTable< std::uint64_t , int > E;       // empty, no places
 CHECK( E.isEmpty() && E.capacity() == 0 && E.load() == 0.0 );
 CHECK( ! E.lookup( 0 ) && E.find( 7 ) == nullptr && ! E.contains( 7 ) );
 CHECK( ! E.remove( 7 ) && E.begin() == E.end() && E.size() == 0 );
 E.clear();
 E.reserve( 0 );
 CHECK( E.capacity() == 0 );
 FlatHashTable< std::uint64_t , int > One( 1 );   // room for 1 pair
 CHECK( One.capacity() == 8 && One.insert( 5 , 50 ) && One.size() == 1 );
 CHECK( ! One.insert( 5 , 51 ) && One.lookup( 5 ) == 51 );
 *One.find( 5 ) = 52;
 CHECK( One.lookup( 5 ) == 52 && One.capacity() == 8 );
 for( std::size_t m : { 8 , 16 , 64 } ) {      // 7 / 8 of m, then one more
  FlatHashTable< std::uint64_t , int > G;
  for( std::uint64_t k = 0 ; k < m - m / 8 ; ++k ) G.insert( k , 0 );
  CHECK( G.capacity() == m && G.size() == m - m / 8 );
  G.insert( m , 0 );
  CHECK( G.capacity() == 2 * m );
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
 CHECK( S.isEmpty() && S.deleted() > 0 && S.capacity() == mS );
 for( std::uint64_t k = 200 ; k < 300 ; ++k ) CHECK( ! S.contains( k ) );
 for( std::uint64_t k = 0 ; k < 150 ; ++k ) S.insert( k , 1 );   // reused
 CHECK( S.size() == 150 && S.capacity() == mS );
 for( std::uint64_t k = 0 ; k < 150 ; ++k ) CHECK( S.lookup( k ) == 1 );
 {                                             // deleted places churned
  FlatHashTable< std::uint64_t , int , Same > D;
  for( int round = 0 ; round < 50 ; ++round ) {
   for( std::uint64_t k = 0 ; k < 13 ; ++k ) D.insert( 100 * round + k , 0 );
   for( std::uint64_t k = 0 ; k < 13 ; ++k ) D.remove( 100 * round + k );
   CHECK( ! D.contains( 100 * round ) && D.capacity() <= 32 );
   }
  CHECK( D.isEmpty() );
  }
 if constexpr( sizeof( std::size_t ) == 8 ) {
  FlatHashTable< std::uint64_t , std::uint64_t , Unmix > F;   // one tag
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( F , R , 5000 , 30000 , []( std::uint64_t x ) {
   return( ( x << 40 ) | 5 ); } , 3 );
  FlatHashTable< std::uint64_t , int , Unmix > W( 100 );   // 16 groups
  CHECK( W.capacity() == 128 );
  for( std::uint64_t j = 0 ; j < 30 ; ++j )   // all in the last group
   W.insert( ( std::uint64_t( 15 ) << 60 ) | ( j << 8 ) , int( j ) );
  CHECK( W.size() == 30 && W.capacity() == 128 );
  for( std::uint64_t j = 0 ; j < 30 ; ++j )
   CHECK( W.lookup( ( std::uint64_t( 15 ) << 60 ) | ( j << 8 ) ) ==
          int( j ) );
  CHECK( ( *W.begin() ).first >> 60 == 15 );   // place 0, wrapped
  for( std::uint64_t j = 0 ; j < 30 ; j += 2 )
   W.remove( ( std::uint64_t( 15 ) << 60 ) | ( j << 8 ) );
  for( std::uint64_t j = 0 ; j < 30 ; ++j )
   CHECK( W.contains( ( std::uint64_t( 15 ) << 60 ) | ( j << 8 ) ) ==
          ( j % 2 == 1 ) );
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
  for( auto e = I.begin() ; e != I.end() ; ) e = I.erase( e );
  CHECK( I.isEmpty() && I.begin() == I.end() );
  I.insert( 7 , 7 );                           // reused
  CHECK( I.size() == 1 && I.lookup( 7 ) == 7 );
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
  for( const auto & [ k , v ] : R ) CHECK( B.find( k )->s == v );
  C = std::move( B );
  CHECK( B.isEmpty() && B.capacity() == 0 && C.size() == A.size() );
  B.insert( "again" , Alive( "x" ) );           // a moved-from table works
  CHECK( B.lookup( "again" )->s == "x" );
  const auto & same = C;                      // self-assignment
  C = same;
  A.clear();
  CHECK( A.isEmpty() && Alive::count == long( C.size() ) + 1 );
  FlatHashTable< std::string , Alive > M( std::move( C ) );
  CHECK( C.isEmpty() && M.size() == R.size() );
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
 CHECK( I.isEmpty() && I.take().empty() );
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
 CHECK( ordered && I.isEmpty() );
 for( int k = 0 ; k < p ; ++k ) CHECK( next[ k ] == n );
 MPSCInbox< int > S;
 S.push( 7 );
 CHECK( ! S.isEmpty() && S.take() == std::vector< int >( { 7 } ) );
 for( int x : { 1 , 2 , 3 } ) S.push( x );
 CHECK( S.take() == std::vector< int >( { 1 , 2 , 3 } ) );
 CHECK( S.isEmpty() && S.take().empty() );
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
