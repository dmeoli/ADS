/*--------------------------------------------------------------------------*/
/*-------------------------- File engineered.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered structures, beyond the book: the bucket queue and
 * the d-ary heap of plain items, the hash table with control bytes, the
 * hash table of pointers, the arena, the pool, the allocator on huge
 * pages, the hashing with random tables, the queues between threads and
 * the concurrent cuckoo table. Each one is checked against a reference of
 * the standard library on random operations, at the level 3 of the
 * contracts, and the structures between threads on real threads. The
 * tables are those of ADS also where `ADS_WITH_BOOST` delegates them to
 * Boost, since these tests look at their places one by one [see
 * `engineered_boost.cpp` for the delegations].
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

// the code of ADS, not that of Boost
#define ADS_FLATHASH_BOOST 0
#define ADS_POINTERTABLE_BOOST 0

#include "ADSTest.h"

#include "ADS/concurrent/ConcurrentCuckooTable.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/concurrent/MPSCInbox.h"
#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/concurrent/SingleWriterFlatTable.h"
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
#include <atomic>
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
#include <variant>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

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
 CHECK_THROWS( ( void )Q.top() , contract_violation );
 CHECK_THROWS( Q.pop() , contract_violation );
 CHECK_THROWS( Q.popEntry() , contract_violation );
 CHECK_THROWS( ( void )Q.topEntry() , contract_violation );
 CHECK_THROWS( Q.push( 0 , -1 , 0 ) , contract_violation );
 std::mt19937 g( 1 );
 std::set< std::tuple< int , int , int , int > > R;  // f , -g , -t , x
 for( int t = 0 ; t < 5000 ; ++t )
  if( R.empty() || ( g() % 3 ) ) {
   const int f = int( g() % 40 ) , h = int( g() % 15 );
   Q.push( t , f , h );
   R.emplace( f , -h , -t , t );
   }
  else {
   const auto [ f , mg , mt , x ] = *R.begin();
   if( t % 2 ) {  // by top() and topEntry(), and by popEntry()
    CHECK( Q.topF() == f && Q.topG() == -mg && Q.top() == x );
    const auto e = Q.topEntry();
    CHECK( &e.item == &Q.top() && e.item == x && e.f == f && e.g == -mg );
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
 CHECK_THROWS( ( void )Q.topF() , contract_violation );
 CHECK_THROWS( ( void )Q.topG() , contract_violation );
 BucketQueue< int > Z;  // all ties: LIFO
 for( int x = 0 ; x < 5 ; ++x )
  Z.push( x , 0 , 0 );
 for( int x = 5 ; x-- > 0 ; )
  CHECK( Z.pop() == x );
 CHECK( Z.empty() && Z.begin() == Z.end() );
 CHECK_THROWS( Z.pop() , contract_violation );  // emptied after use
 Z.push( 1 , 3 , 4 );
 CHECK( Z.pop() == 1 );  // the layer 3 is freed
 Z.push( 2 , 3 , 1 );      // and taken again, lower g
 CHECK( Z.topF() == 3 && Z.topG() == 1 && Z.pop() == 2 );
 Z.push( 3 , 9 , 0 );
 CHECK( Z.topF() == 9 );  // the least f moved to 9
 Z.push( 4 , 2 , 0 );       // and back below it
 CHECK( Z.topF() == 2 && Z.pop() == 4 && Z.topF() == 9 && Z.pop() == 3 );
 BucketQueue< int , unsigned > U;  // unsigned keys
 U.push( 1 , 5u , 0u );
 U.push( 2 , 0u , 0u );
 U.push( 3 , 0u , 7u );
 CHECK( U.pop() == 3 && U.pop() == 2 && U.pop() == 1 && U.empty() );
 }

/// a bucket queue of the items 0 , 1 , ... in the order of their pushes,
/// with the set of ( f , -g , -t , t ) that orders them as it must, or of
/// ( f , g , -t , t ) if the least g comes first
template< class Tie >
struct BucketRef
{
 BucketQueue< int , int , Tie > q;
 std::set< std::tuple< int , int , int , int > > s;
 int t = 0;

 /// g as the set orders it, and back
 static int sg( int g ) { return( Tie::least ? g : -g ); }

 void push( int f , int g ) {
  q.push( t , f , g );
  s.emplace( f , sg( g ) , -t , t );
  ++t;
  }
 /// one pop, by popEntry() or by top(), topF(), topG() and pop()
 void pop( bool entry = true ) {
  const auto [ f , mg , mt , x ] = *s.begin();
  if( entry ) {
   const auto e = q.popEntry();
   CHECK( e.item == x && e.f == f && e.g == sg( mg ) );
   }
  else {
   CHECK( q.topF() == f && q.topG() == sg( mg ) && q.top() == x );
   CHECK( q.pop() == x );
   }
  s.erase( s.begin() );
  CHECK( q.size() == s.size() );
  }
 /// the view of the standard interface in the order of the set
 void view( void ) const {
  auto it = q.begin();
  for( const auto & e : s ) {
   CHECK( it != q.end() && *it == std::get< 3 >( e ) );
   ++it;
   }
  CHECK( it == q.end() );
  }
 void drain( void ) {
  view();
  while( ! s.empty() )
   pop( s.size() % 2 );
  CHECK( q.empty() && q.begin() == q.end() );
  }
 };

/// the reference of the default order, the largest g first
using BucketReference = BucketRef< LargestSecondKey >;

/// random operations on the bucket queue of the policy Tie against its
/// reference, from the seed: g small, around D and spread above D, so that
/// the layers keep runs, turn dense and are emptied, with the view of the
/// standard interface now and then
template< class Tie >
void bucketRandom( unsigned seed , int steps )
{
 constexpr int D = int( BucketQueue< int >::D );
 constexpr int R = int( BucketQueue< int >::R );
 BucketRef< Tie > B;
 std::mt19937 g( seed );
 const int spread = seed % 2 ? 3000 : 2 * R;
 for( int step = 0 ; step < steps ; ++step ) {
  const unsigned c = g() % 10;
  if( B.s.empty() || ( c < 6 ) ) {
   const unsigned w = g() % 4;
   const int x = w == 0   ? int( g() % 8 )
		 : w == 1 ? int( D - 3 + g() % 7 )
			  : int( D + g() % unsigned( spread ) );
   B.push( int( g() % 10 ) , x );
   }
  else
   B.pop( c % 2 );
  if( step % 1000 == 0 )
   B.view();
  if( step % 7000 == 6999 )
   B.drain();
  }
 B.drain();
 }

// the lists of large g of the bucket queue, kept as up to R sparse runs
// in a layer and then dense: g just below, at and above D, ties of f and g
// last pushed first in the runs, in the dense lists and across the
// conversion, the conversion of a layer with items in its runs and in its
// dense lists (by a new g and by the g of a run), the least f back below a
// layer freed, a layer emptied and filled again many times, crossing the
// conversion now and then, g as large as int and as long long in a run,
// and random operations against a set by least f, largest g and last
// pushed, with the view of the standard interface; each step is checked
// at the level 3 of the contracts, whose invariant covers the runs
ADS_TEST( "bucket-queue-sparse" )
{
 constexpr int D = int( BucketQueue< int >::D );
 constexpr int R = int( BucketQueue< int >::R );
 static_assert( ( D > 4 ) && ( R > 2 ) );
 {
  BucketReference B;  // empty, then one item
  CHECK_THROWS( ( void )B.q.topG() , contract_violation );
  B.push( 0 , D + 1000 );
  B.view();
  B.pop( false );
  CHECK_THROWS( B.q.pop() , contract_violation );
  B.push( 3 , std::numeric_limits< int >::max() );  // no array of g
  B.push( 3 , 0 );
  B.push( 3 , std::numeric_limits< int >::max() );
  B.drain();
  BucketQueue< int , long long > L;
  L.push( 1 , 2 , 1LL << 40 );
  L.push( 2 , 2 , 7 );
  CHECK( L.topG() == ( 1LL << 40 ) && L.topEntry().g == ( 1LL << 40 ) &&
	 L.pop() == 1 && L.topG() == 7 );
  const auto e = L.topEntry();  // a sparse layer, of a run
  CHECK( e.item == 2 && e.f == 2 && e.g == 7 && L.size() == 1 );
  BucketQueue< int , int , LeastSecondKey > S;
  S.push( 4 , 1 , 1 << 20 );
  S.push( 5 , 1 , 3 );
  const auto s = S.topEntry();
  CHECK( s.item == 5 && s.f == 1 && s.g == 3 && S.size() == 2 );
  }
 {
  BucketReference B;  // around D, with ties
  for( int g : { D - 1 , D , D + 1 , D - 1 , D , D + 1 , 0 , D } )
   B.push( 1 , g );
  B.view();
  CHECK( B.q.topG() == D + 1 );
  B.drain();
  }
 for( int how = 0 ; how < 2 ; ++how ) {  // the conversion, items in
  BucketReference B;
  std::mt19937 g( 5 + how );
  for( int x : { 0 , 3 , 4 , D - 1 , 3 , 0 } )
   B.push( 2 , x );
  std::vector< int > G;
  for( int k = 0 ; k < R ; ++k )
   G.push_back( D + 2 * k );
  std::shuffle( G.begin() , G.end() , g );
  for( int x : G ) {
   B.push( 2 , x );
   if( g() % 3 == 0 )
    B.push( 2 , x );  // a tie in a run
   }
  B.push( 7 , D + 5 );  // another layer, untouched
  B.view();
  B.push( 2 , how ? D + 4 : D + 1 );  // a run of R + 1, or a tie
  B.view();
  CHECK( B.q.topF() == 2 && B.q.topG() == D + 2 * ( R - 1 ) );
  for( int x : { D + 1 , 2 , D + 3 * R , D + 1 , D - 1 , D + 4 } )
   B.push( 2 , x );  // into the dense layer
  B.view();
  for( int k = 0 ; k < R / 2 ; ++k )
   B.pop( k % 2 );
  B.push( 2 , D + 6 );  // still dense
  B.push( 2 , D + 6 );
  B.drain();
  }
 {
  BucketReference B;  // the least f back below
  for( int k = 0 ; k < 5 ; ++k )
   B.push( 5 , D + 100 * k );
  B.push( 9 , D + 3 );
  for( int k = 0 ; k < 5 ; ++k )
   B.pop();  // the layer 5 freed
  CHECK( B.q.topF() == 9 );
  B.push( 5 , D + 50 );  // and filled again
  B.push( 3 , D + 7 );
  B.push( 5 , 1 );
  B.view();
  CHECK( B.q.topF() == 3 );
  for( int k = 0 ; k <= R ; ++k )
   B.push( 5 , D + 3 * k );  // dense
  B.pop();
  while( B.q.topF() == 5 )
   B.pop( false );     // freed when dense
  B.push( 5 , D + 8 );  // and sparse again
  B.push( 5 , D + 9 );
  B.drain();
  }
 {
  BucketReference B;  // emptied and filled again
  std::mt19937 g( 9 );
  B.push( 1 , D + 3 );  // stays below
  for( int round = 0 ; round < 3000 ; ++round ) {
   const int m = ( round % 100 == 99 ) ? R + 10 : 1 + round % 3;
   for( int k = 0 ; k < m ; ++k )
    B.push( 0 , int( D + g() % 8000 ) );
   if( round % 500 == 0 )
    B.view();
   for( int k = 0 ; k < m ; ++k )
    B.pop( k % 2 );
   CHECK( B.q.topF() == 1 );
   }
  B.drain();
  }
 for( unsigned seed = 1 ; seed <= 4 ; ++seed )  // random
  bucketRandom< LargestSecondKey >( seed , 20000 );
 }

// the bucket queue in the lexicographic order of LPA*, least f, then least
// g, then last pushed (LeastSecondKey): ties on both keys, g below, at and
// above D, the dense lists given before the runs of a layer, a layer of
// runs only, a push below the lazy bound of g, the conversion of a layer of
// R runs to dense with items in its dense lists, the layer emptied, freed
// and filled again, the keys of LPA* from a pair ( k1 , k2 ), and random
// operations against the set ordered so, with the view of the standard
// interface, at the level 3 of the contracts; the default order is that of
// bucket-queue and bucket-queue-sparse
ADS_TEST( "bucket-queue-lexicographic" )
{
 using Q = BucketQueue< int , int , LeastSecondKey >;
 constexpr int D = int( Q::D );
 constexpr int R = int( Q::R );
 static_assert( std::is_same_v< Q::Ties , LeastSecondKey > &&
		std::is_same_v< BucketQueue< int >::Ties ,
				LargestSecondKey > );
 {
  Q q;  // all ties: LIFO
  for( int x = 0 ; x < 5 ; ++x )
   q.push( x , 0 , 0 );
  for( int x = 5 ; x-- > 0 ; )
   CHECK( q.pop() == x );
  CHECK_THROWS( q.pop() , contract_violation );
  q.push( 1 , 4 , 9 );  // least g first
  q.push( 2 , 4 , 3 );
  q.push( 3 , 4 , 7 );
  q.push( 4 , 5 , 0 );
  CHECK( q.topF() == 4 && q.topG() == 3 && q.pop() == 2 );
  q.push( 5 , 4 , 1 );  // below the lazy bound
  CHECK( q.topG() == 1 && q.pop() == 5 && q.pop() == 3 && q.pop() == 1 );
  CHECK( q.topF() == 5 && q.pop() == 4 && q.empty() );
  }
 {
  BucketRef< LeastSecondKey > B;  // around D, with ties
  for( int g : { D + 1 , D , D - 1 , D + 1 , D , D - 1 , 0 , D } )
   B.push( 1 , g );
  B.view();
  CHECK( B.q.topG() == 0 );
  B.drain();
  }
 {
  BucketRef< LeastSecondKey > B;  // runs only, then dense too
  for( int k : { 5 , 2 , 9 , 2 , 7 } )
   B.push( 3 , D + k );
  B.view();
  CHECK( B.q.topG() == D + 2 );
  B.pop();
  B.push( 3 , 6 );  // before every run
  B.push( 3 , D - 1 );
  B.view();
  CHECK( B.q.topG() == 6 );
  B.pop();
  B.pop( false );                // the dense lists emptied
  CHECK( B.q.topG() == D + 2 );  // back to the runs
  B.push( 3 , 1 );
  B.drain();
  }
 for( int how = 0 ; how < 2 ; ++how ) {  // the conversion, items in
  BucketRef< LeastSecondKey > B;
  std::mt19937 g( 15 + how );
  for( int x : { 0 , 3 , 4 , D - 1 , 3 , 0 } )
   B.push( 2 , x );
  std::vector< int > G;
  for( int k = 0 ; k < R ; ++k )
   G.push_back( D + 2 * k );
  std::shuffle( G.begin() , G.end() , g );
  for( int x : G ) {
   B.push( 2 , x );
   if( g() % 3 == 0 )
    B.push( 2 , x );  // a tie in a run
   }
  B.push( 7 , D + 5 );  // another layer, untouched
  B.view();
  B.push( 2 , how ? D + 4 : D + 1 );  // a run of R + 1, or a tie
  B.view();
  CHECK( B.q.topF() == 2 && B.q.topG() == 0 );
  for( int k = 0 ; k < 6 ; ++k )
   B.pop( k % 2 );  // the dense g < D
  CHECK( B.q.topG() == D );
  for( int x : { D + 1 , 2 , D + 3 * R , D + 1 , D - 1 , D + 4 } )
   B.push( 2 , x );  // into the dense layer
  B.view();
  for( int k = 0 ; k < R / 2 ; ++k )
   B.pop( k % 2 );
  B.push( 2 , D + 6 );
  B.push( 2 , 1 );
  B.drain();
  B.push( 2 , D + 8 );  // freed, sparse again
  B.push( 2 , D + 9 );
  B.push( 2 , D + 3 );
  CHECK( B.q.topG() == D + 3 );
  B.drain();
  }
 {
  // the keys of LPA*, k1 = min( g , rhs ) + h and k2 = min( g , rhs ):
  // least k1, then least k2, against a sort of the pairs
  Q q;
  std::mt19937 g( 21 );
  std::vector< std::tuple< int , int , int > > v;
  for( int x = 0 ; x < 3000 ; ++x ) {
   const int k2 = int( g() % 400 ) , k1 = k2 + int( g() % 50 );
   q.push( x , k1 , k2 );
   v.emplace_back( k1 , k2 , -x );
   }
  std::sort( v.begin() , v.end() );
  for( const auto & [ k1 , k2 , mx ] : v ) {
   const auto e = q.popEntry();
   CHECK( e.f == k1 && e.g == k2 && e.item == -mx );
   }
  CHECK( q.empty() );
  }
 for( unsigned seed = 1 ; seed <= 4 ; ++seed )
  bucketRandom< LeastSecondKey >( seed , 20000 );
 }

// the d-ary heap of plain items for D = 2 , 3 , 4 , 8 against
// std::priority_queue, the items carrying f and g, the least f and then the
// largest g out first; the edge cases: n = 0 , 1 , D , D + 1 items, one
// item (popped onto itself), all equal items, sorted and reverse sorted
// pushes, the extremes of int, an odd D, a pushed item that lives in the
// heap while the vector grows, and the empty heap after clear(); there is
// no decrease-key to test
struct Entry
{
 int f , g , id;
 };

struct EntryBetter
{
 bool operator()( const Entry & a , const Entry & b ) const {
  return( ( a.f < b.f ) || ( ( a.f == b.f ) && ( a.g > b.g ) ) ||
	   ( ( a.f == b.f ) && ( a.g == b.g ) && ( a.id < b.id ) ) );
  }
 };

template< unsigned D >
void dary( unsigned seed )
{
 DaryHeap< Entry , EntryBetter , D > H;
 CHECK_THROWS( ( void )H.top() , contract_violation );
 CHECK_THROWS( H.pop() , contract_violation );
 auto worse = []( const Entry & a , const Entry & b ) {
  return( EntryBetter()( b , a ) );
  };
 std::priority_queue< Entry , std::vector< Entry > , decltype( worse ) > R(
   worse );
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

/// the heap of arity D on n = 0 , 1 , D , D + 1 , D^2 + 2 items, distinct
/// and all equal, built by pushes and by assign() over items already there,
/// against std::priority_queue; the ties come out by better()
template< unsigned D >
void dary_edges( void )
{
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
			std::size_t( D ) ,
			std::size_t( D + 1 ) , std::size_t( D * D + 2 ) } )
  for( int same = 0 ; same < 2 ; ++same )
   for( int built = 0 ; built < 2 ; ++built ) {
    DaryHeap< std::pair< int , int > , std::less<> , D > H;
    std::priority_queue< std::pair< int , int > ,
			 std::vector< std::pair< int , int > > ,
			 std::greater<> >
      R;
    std::vector< std::pair< int , int > > X;
    for( std::size_t i = 0 ; i < n ; ++i ) {  // reversed: all move up
     const std::pair< int , int > x( same ? 7 : int( n - i ) ,
				    same ? 0 : int( i ) );
     X.push_back( x );
     R.push( x );
     }
    if( built ) {
     H.push( { -1 , -1 } );  // replaced by assign()
     H.assign( X.begin() , X.end() );
     }
    else
     for( const auto & x : X )
      H.push( x );
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
 {                            // the pushed item aliases
  DaryHeap< std::string > A;  // the heap, which grows
  A.push( "b" );
  for( int i = 0 ; i < 40 ; ++i )
   A.push( A.top() );
  A.push( "a" );
  CHECK( A.size() == 42 && A.pop() == "a" && A.top() == "b" );
  }
 for( std::size_t n = 1 ; n <= 33 ; ++n )        // an item of the heap that
  for( std::size_t k = 0 ; k < n ; ++k ) {       // moves up, pushed when the
   DaryHeap< std::string , std::less<> , 2 > A;  // vector is full or not
   std::multiset< std::string > R;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::string s( 24 , char( 'a' + ( i * 7 ) % 26 ) );  // no SSO
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
 DaryHeap< int > I;  // the least first
 for( int x : { 5 , 3 , 9 , 1 } )
  I.push( x );
 CHECK( I.pop() == 1 && I.pop() == 3 && I.size() == 2 );
 I.clear();
 CHECK( I.empty() && I.begin() == I.end() );
 CHECK_THROWS( ( void )I.top() , contract_violation );
 CHECK_THROWS( I.pop() , contract_violation );
 DaryHeap< std::string > O;  // the root is the last
 O.push( "only" );
 CHECK( O.top() == "only" && O.pop() == "only" && O.empty() );
 DaryHeap< int , std::greater< int > , 3 > X;  // the largest first
 for( int x : { INT_MIN , 0 , INT_MAX , 0 , INT_MAX , INT_MIN } )
  X.push( x );
 for( int x : { INT_MAX , INT_MAX , 0 , 0 , INT_MIN , INT_MIN } )
  CHECK( X.pop() == x );
 CHECK( X.empty() );
 for( int dir = 0 ; dir < 3 ; ++dir ) {  // up, down, all equal
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
struct Node
{
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
   const auto [ p , fresh ] = T.find_or_insert(
     s , h , [ & ]( void ) { return( N.make( Node{ s , op } ) ); } );
   const auto it = R.find( s );
   CHECK( fresh == ( it == R.end() ) );
   if( fresh )
    R[ s ] = p;
   else
    CHECK( it->second == p );
   CHECK( T.find( s , h ) == p && p->state == s );
   }
  CHECK( T.size() == R.size() && N.size() == R.size() );
  CHECK( T.size() * 10 <= T.capacity() * 7 );
  std::size_t n = 0;
  T.forEach( [ & ]( Node * ) { ++n; } );
  CHECK( n == R.size() );
  CHECK( T.find( std::uint64_t( 999999 ) , splitmix64( 999999 ) ) == nullptr );
  }
 auto byG = []( const Node * p , int g ) { return( p->g == g ); };
 Arena< Node > M;
 PointerTable< Node , decltype( byG ) > G( 4 , byG );  // any equality
 CHECK( G.capacity() == 16 );
 auto make3 = [ & ]( void ) { return( M.make( Node{ 0 , 3 } ) ); };
 const auto a = G.find_or_insert( 3 , 3 , make3 );
 CHECK( a.second && ( G.find_or_insert( 3 , 3 , make3 ).first == a.first ) );
 auto nil = []( void ) { return( static_cast< Node * >( nullptr ) ); };
 CHECK_THROWS( G.find_or_insert( 4 , 4 , nil ) , contract_violation );
 CHECK( G.size() == 1 && G.find( 4 , 4 ) == nullptr );  // not inserted
 CHECK( PointerTable< Node >( 0 ).capacity() == 16 );
 CHECK( PointerTable< Node >( 16 ).capacity() == 16 );
 CHECK( PointerTable< Node >( 17 ).capacity() == 32 );
 Arena< Node > N;
 PointerTable< Node > E;
 CHECK( E.size() == 0 && E.find( std::uint64_t( 0 ) , 0 ) == nullptr );
 CHECK( E.begin() == E.end() );
 for( std::uint64_t s = 0 ; s < 12 ; ++s ) {
  E.find_or_insert( s , splitmix64( s ) ,
		    [ & ]( void ) { return( N.make( Node{ s , 0 } ) ); } );
  CHECK( E.capacity() == ( s < 11 ? 16u : 32u ) );  // 12 > 0.7 * 16
  }
 std::uint64_t h = 0;  // the last place of 1024
 while( fibonacciPlace( h , 10 ) != 1023 )
  ++h;
 PointerTable< Node > C( 1024 );
 for( std::uint64_t s = 0 ; s < 50 ; ++s ) {
  const auto r = C.find_or_insert(
    s , h , [ & ]( void ) { return( N.make( Node{ s , 1 } ) ); } );
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
  return( N.make( Node{ 7 , 2 } ) );
  } );
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
 CHECK( R.capacity() == 1024 );  // 700 <= 0.7 * 1024
 for( std::uint64_t s = 0 ; s < 700 ; ++s )
  R.find_or_insert( s , splitmix64( s ) ,
		    [ & ]( void ) { return( N.make( Node{ s , 3 } ) ); } );
 CHECK( R.capacity() == 1024 && R.size() == 700 );
 R.reserve( 10 );
 CHECK( R.capacity() == 1024 );
 R.reserve( 717 );  // 717 > 0.7 * 1024: 2048
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
 for( auto & w : W )
  w.join();
 CHECK( wrong == std::vector< int >( 4 , 0 ) );
 CHECK( C.probes() >= 1 );
 }


// the hash of FlatHashTable that declares itself avalanching, so that the
// table takes the key as its mixed hash and a test chooses the first group
// (the top bits), the fingerprint (the low 8 bits) and the bit of overflow
// (the bits 8 to 10) of each key; on 32 bits the table mixes it anyway
struct Raw
{
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
struct Same
{
 std::size_t operator()( std::uint64_t ) const { return( 42 ); }
 };

/// a value that counts its living copies
struct Alive
{
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
struct Fussy
{
 static long count , budget;
 std::string s;
 explicit Fussy( std::string t ) : s( std::move( t ) ) { ++count; }
 Fussy( const Fussy & o ) : s( o.s ) {
  if( budget-- == 0 )
   throw( std::runtime_error( "copy" ) );
  ++count;
  }
 Fussy( Fussy && o ) noexcept( false ) : s( o.s ) {
  if( budget-- == 0 )
   throw( std::runtime_error( "move" ) );
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
    if( o )
     CHECK( *o == it->second );
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
 for( const auto & [ k , v ] : t )
  f.push_back( k );
 for( auto e = t.end() ; e != t.begin() ; )
  b.push_back( ( *--e ).first );
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
 for( std::uint64_t a :
      { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 32 , 1ULL << 63 , ~0ULL ,
	0x9e3779b97f4a7c15ULL , 0x123456789abcdef0ULL } )
  for( std::uint64_t b : { 0ULL , 1ULL , 0xFFFFFFFFULL , 1ULL << 63 , ~0ULL ,
			   0x9e3779b97f4a7c15ULL } )
   CHECK( detail::mulx( a , b ) == detail::mulxPortable( a , b ) );
 {  // random, with tombstones
  FlatHashTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against( T , R , 3000 , 60000 , []( std::uint64_t x ) { return( x ); } , 1 );
  against( T , R , 1 << 20 , 20000 , []( std::uint64_t x ) { return( x ); } ,
	   2 );
  }
 for( std::uint64_t c : { std::uint64_t( 30720 ) , std::uint64_t( 2048 ) ,
			  std::uint64_t( 1 ) << 32 } ) {  // clustered
  FlatHashTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against(
    T , R , 40000 , 60000 , [ c ]( std::uint64_t x ) { return( x * c ); } ,
    5 );
  }
 FlatHashTable< std::uint64_t , int > E;  // empty, no places
 CHECK( E.empty() && E.capacity() == 0 && E.load() == 0.0 );
 CHECK( ! E.lookup( 0 ) && E.find( 7 ) == nullptr && ! E.contains( 7 ) );
 CHECK( ! E.remove( 7 ) && E.begin() == E.end() && E.size() == 0 );
 E.clear();
 E.reserve( 0 );
 CHECK( E.capacity() == 0 && E.deleted() == 0 );
 FlatHashTable< std::uint64_t , int > One( 1 );  // room for 1 pair
 CHECK( One.capacity() == 30 && One.insert( 5 , 50 ) && One.size() == 1 );
 CHECK( ! One.insert( 5 , 51 ) && One.lookup( 5 ) == 51 );
 *One.find( 5 ) = 52;
 CHECK( One.lookup( 5 ) == 52 && One.capacity() == 30 );
 for( std::size_t m : { 30 , 60 , 240 } ) {  // 7 / 8 of m, then one more
  FlatHashTable< std::uint64_t , int > G;
  for( std::uint64_t k = 0 ; k < m - m / 8 ; ++k )
   G.insert( k , 0 );
  CHECK( G.capacity() == m && G.size() == m - m / 8 );
  G.insert( m , 0 );
  CHECK( G.capacity() == 2 * m && G.size() == m - m / 8 + 1 );
  for( std::uint64_t k = 0 ; k <= m ; ++k )
   CHECK( G.contains( k ) == ( k < m - m / 8 || k == m ) );
  using Int = FlatHashTable< int , int >;
  CHECK( Int( m - m / 8 ).capacity() == m );
  CHECK( Int( m - m / 8 + 1 ).capacity() == 2 * m );
  }
 FlatHashTable< std::uint64_t , int , Same > S;  // one group, one tag
 for( std::uint64_t k = 0 ; k < 200 ; ++k )
  CHECK( S.insert( k , int( k ) ) );
 for( std::uint64_t k = 0 ; k < 200 ; ++k )
  CHECK( S.lookup( k ) == int( k ) );
 CHECK( ! S.contains( 200 ) );
 const std::size_t mS = S.capacity();
 for( std::uint64_t k = 0 ; k < 200 ; ++k )
  CHECK( S.remove( k ) );
 CHECK( S.empty() && S.deleted() > 0 && S.capacity() == mS );
 for( std::uint64_t k = 200 ; k < 300 ; ++k )
  CHECK( ! S.contains( k ) );
 for( std::uint64_t k = 0 ; k < 150 ; ++k )
  S.insert( k , 1 );  // reused
 CHECK( S.size() == 150 && S.capacity() == mS );
 for( std::uint64_t k = 0 ; k < 150 ; ++k )
  CHECK( S.lookup( k ) == 1 );
 {  // churn at a steady size near the bound: the
    // places counted recounted, few rebuilds
  FlatHashTable< std::uint64_t , std::uint64_t > C;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 7 );
  for( std::uint64_t k = 0 ; k < 1200 ; ++k ) {  // 1200 of 1920 places
   C.insert( k , k );
   R[ k ] = k;
   }
  const std::size_t m = C.capacity();
  CHECK( m == 1920 );
  std::vector< std::uint64_t > in( 1200 );
  for( std::uint64_t k = 0 ; k < 1200 ; ++k )
   in[ k ] = k;
  for( std::uint64_t t = 0 ; t < 100000 ; ++t ) {  // one out, one in
   const std::size_t i = g() % in.size();
   CHECK( C.remove( in[ i ] ) && R.erase( in[ i ] ) == 1 );
   in[ i ] = 1200 + t;
   CHECK( C.insert( in[ i ] , t ) );
   R[ in[ i ] ] = t;
   const std::size_t c = C.capacity();
   CHECK( C.size() + C.deleted() <= c - c / 8 );
   }
  CHECK( C.capacity() <= 2 * m && C.size() == R.size() );
  for( const auto & [ k , v ] : R )
   CHECK( C.lookup( k ) == v );
  for( const auto & [ k , v ] : C )
   CHECK( R.at( k ) == v );
  }
 {  // deleted places churned
  FlatHashTable< std::uint64_t , int , Same > D;
  for( int round = 0 ; round < 50 ; ++round ) {
   for( std::uint64_t k = 0 ; k < 20 ; ++k )
    D.insert( 100 * round + k , 0 );
   for( std::uint64_t k = 0 ; k < 20 ; ++k )
    D.remove( 100 * round + k );
   CHECK( ! D.contains( 100 * round ) && D.capacity() <= 60 );
   }
  CHECK( D.empty() );
  }
 {  // one fingerprint, 2 groups
  FlatHashTable< std::uint64_t , int , Raw > P( 1 );
  CHECK( P.capacity() == 30 );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )  // group 0 full, all tag 5
   CHECK( P.insert( raw( 0 , 1 , j , 5 ) , int( j ) ) );
  CHECK( P.deleted() == 0 );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )
   CHECK( P.lookup( raw( 0 , 1 , j , 5 ) ) == int( j ) );
  CHECK( ! P.contains( raw( 0 , 1 , 15 , 5 ) ) );
  CHECK( P.insert( raw( 0 , 1 , 15 , 5 ) , 15 ) );  // past group 0
  CHECK( P.lookup( raw( 0 , 1 , 15 , 5 ) ) == 15 );
  CHECK( ! P.contains( raw( 0 , 1 , 16 ,
			    5 ) ) );                           // the same bit
  CHECK( ! P.contains( raw( 0 , 1 , 16 , 5 | 0x700 ) ) );         // another
  CHECK( P.remove( raw( 0 , 1 , 3 , 5 ) ) && P.deleted() == 1 );  // under
  CHECK( P.lookup( raw( 0 , 1 , 15 ,
			5 ) ) == 15 );          // still past it
  CHECK( P.insert( raw( 0 , 1 , 20 , 0 ) , 20 ) );  // the hole, tag 1
  CHECK( P.deleted() == 0 && P.lookup( raw( 0 , 1 , 20 , 0 ) ) == 20 );
  CHECK( P.insert( raw( 0 , 1 , 21 , 1 ) , 21 ) );  // also tag 1
  CHECK( P.lookup( raw( 0 , 1 , 21 , 1 ) ) == 21 );
  CHECK( ! P.contains( raw( 0 , 1 , 22 , 0 ) ) );
  CHECK( ! P.contains( raw( 0 , 1 , 20 , 1 ) ) );
  CHECK( P.remove( raw( 0 , 1 , 21 , 1 ) ) && P.deleted() == 0 );
  std::vector< std::uint64_t > in;  // all of them out
  for( const auto & [ k , v ] : P )
   in.push_back( k );
  CHECK( in.size() == 16 );
  for( std::uint64_t k : in )
   CHECK( P.remove( k ) );
  CHECK( P.empty() && P.deleted() == 15 && P.capacity() == 30 );
  for( std::uint64_t j = 0 ; j < 12 ; ++j )  // group 1, 12 + 15 = 27
   P.insert( raw( 1 , 1 , j , 9 ) , 0 );
  CHECK( P.deleted() == 15 && P.capacity() == 30 );
  P.insert( raw( 1 , 1 , 12 , 9 ) , 0 );  // rebuilt, the same size
  CHECK( P.deleted() == 0 && P.capacity() == 30 && P.size() == 13 );
  CHECK( P.remove( raw( 1 , 1 , 0 , 9 ) ) && P.deleted() == 0 );  // none
  for( std::uint64_t j = 1 ; j < 13 ; ++j )
   CHECK( P.contains( raw( 1 , 1 , j , 9 ) ) );
  }
 if constexpr( sizeof( std::size_t ) == 8 ) {
  FlatHashTable< std::uint64_t , std::uint64_t , Raw > F;  // one tag
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  against(
    F , R , 5000 , 30000 ,
    []( std::uint64_t x ) { return( ( x << 40 ) | 5 ); } ,
    3 );
  FlatHashTable< std::uint64_t , int , Raw > W( 100 );  // 8 groups
  CHECK( W.capacity() == 120 );
  for( std::uint64_t j = 0 ; j < 45 ;
       ++j )        // from the last group: 7 , 0
   W.insert( raw( 7 , 3 , j , j + 1 ) , int( j ) );  // and 2
  CHECK( W.size() == 45 && W.capacity() == 120 && W.deleted() == 0 );
  for( std::uint64_t j = 0 ; j < 45 ; ++j )
   CHECK( W.lookup( raw( 7 , 3 , j , j + 1 ) ) == int( j ) );
  CHECK( ( *W.begin() ).first >> 61 == 7 );  // place 0, wrapped
  CHECK( ( *W.begin() ).first == raw( 7 , 3 , 15 , 16 ) );
  for( std::uint64_t j = 0 ; j < 45 ; j += 2 )
   W.remove( raw( 7 , 3 , j , j + 1 ) );
  CHECK( W.deleted() == 8 + 7 );  // in the groups 7 and 0
  for( std::uint64_t j = 0 ; j < 45 ; ++j )
   CHECK( W.contains( raw( 7 , 3 , j , j + 1 ) ) == ( j % 2 == 1 ) );
  }
 {  // iterators and erase( it )
  FlatHashTable< std::uint64_t , int > I;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   I.insert( k , int( k ) );
  auto it = I.begin();
  const std::uint64_t k0 = ( *it ).first;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   if( k != k0 && k % 3 )
    I.remove( k );
  I.insert( k0 , -1 );  // there: no rebuild
  CHECK( ( *it ).first == k0 && ( *it ).second == -1 );
  for( auto e = I.begin() ; e != I.end() ; )
   if( ( *e ).first % 2 )
    e = I.erase( e );
   else
    ++e;
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   CHECK( I.contains( k ) == ( ( k == k0 || k % 3 == 0 ) && k % 2 == 0 ) );
  std::size_t n = 0;  // backwards, the same pairs
  for( auto e = I.end() ; e != I.begin() ; ) {
   --e;
   ++n;
   }
  CHECK( n == I.size() );
  for( auto e = I.begin() ; e != I.end() ; )
   e = I.erase( e );
  CHECK( I.empty() && I.begin() == I.end() );
  I.insert( 7 , 7 );  // reused
  CHECK( I.size() == 1 && I.lookup( 7 ) == 7 );
  }
 {  // a value of the table
  FlatHashTable< std::uint64_t , std::string > V;
  for( std::uint64_t k = 0 ; k < 27 ; ++k )
   V.insert( k , "a value long enough for the heap " + std::to_string( k ) );
  CHECK( V.capacity() == 30 );
  V.insert( 100 , *V.find( 3 ) );  // grows: copied first
  CHECK( V.capacity() == 60 && *V.find( 100 ) == *V.find( 3 ) );
  }
 {  // copies that throw
  using FT = FlatHashTable< std::uint64_t , Fussy >;
  FT T;
  for( std::uint64_t k = 0 ; k < 27 ; ++k )
   T.insert( k , Fussy( "value " + std::to_string( k ) ) );
  CHECK( T.capacity() == 30 && Fussy::count == 27 );
  for( long b : { 0L , 5L } ) {  // the new pair, an old one
   Fussy::budget = b;
   CHECK_THROWS( T.insert( 99 , Fussy( "new" ) ) , std::runtime_error );
   Fussy::budget = -1;
   CHECK( T.size() == 27 && T.capacity() == 30 && ! T.contains( 99 ) );
   CHECK( Fussy::count == 27 );
   for( std::uint64_t k = 0 ; k < 27 ; ++k )
    CHECK( T.find( k )->s == "value " + std::to_string( k ) );
   }
  Fussy::budget = 7;  // the copy of the table
  CHECK_THROWS( FT( T ) , std::runtime_error );
  Fussy::budget = -1;
  CHECK( Fussy::count == 27 );
  }
 CHECK( Fussy::count == 0 );
 {                                        // the walk of the groups
  FlatHashTable< std::uint64_t , int > N;  // no places, then no pairs
  CHECK( walk( N ).empty() && N.begin() == N.end() );
  N.reserve( 100 );
  CHECK( walk( N ).empty() && N.cbegin() == N.cend() );
  N.insert( 1 , 1 );
  N.clear();
  CHECK( walk( N ).empty() && N.capacity() > 0 );
  FlatHashTable< std::uint64_t , int > M( std::move( N ) );  // moved
  CHECK( walk( N ).empty() && walk( M ).empty() );
  }
 if constexpr( sizeof( std::size_t ) == 8 ) {
  FlatHashTable< std::uint64_t , int , Raw > W( 100 );  // 8 groups
  CHECK( W.capacity() == 120 );
  W.insert( raw( 7 , 3 , 0 , 1 ) , 0 );  // the last group alone
  CHECK( walk( W ) == std::vector< std::uint64_t >{ raw( 7 , 3 , 0 , 1 ) } );
  CHECK( W.erase( W.begin() ) == W.end() && W.empty() );
  W.insert( raw( 0 , 3 , 0 , 1 ) , 0 );  // the first group alone
  CHECK( walk( W ).size() == 1 );
  CHECK( ( *--W.end() ).first == raw( 0 , 3 , 0 , 1 ) );
  W.remove( raw( 0 , 3 , 0 , 1 ) );
  for( std::uint64_t j = 0 ; j < 15 ; ++j )  // groups 2 and 7 full
   for( std::uint64_t g : { 2 , 7 } )
    W.insert( raw( g , 3 , j , j + 1 ) , int( j ) );
  CHECK( W.capacity() == 120 && walk( W ).size() == 30 );
  auto it = W.begin();  // place 0 of group 2
  CHECK( ( *it ).first == raw( 2 , 3 , 0 , 1 ) );
  W.remove( raw( 2 , 3 , 1 , 2 ) );    // the next place, skipped
  W.remove( raw( 2 , 3 , 14 , 15 ) );  // the last of the group
  CHECK( ( *++it ).first == raw( 2 , 3 , 2 , 3 ) );
  for( int s = 0 ; s < 11 ; ++s )
   ++it;
  CHECK( ( *it ).first == raw( 2 , 3 , 13 , 14 ) );
  CHECK( ( *++it ).first == raw( 7 , 3 , 0 , 1 ) );  // groups 3 to 6
  CHECK( ( *--it ).first == raw( 2 , 3 , 13 , 14 ) );
  auto last = W.end();  // the last place full
  CHECK( ( *--last ).first == raw( 7 , 3 , 14 , 15 ) );
  CHECK( ++last == W.end() && walk( W ).size() == 28 );
  for( auto e = W.end() ; e != W.begin() ; )  // erase backwards
   e = W.erase( --e );
  CHECK( W.empty() && W.begin() == W.end() && W.capacity() == 120 );
  }
 {  // one group, overflowing
  FlatHashTable< std::uint64_t , int , Same > S;
  std::set< std::uint64_t > in;
  for( std::uint64_t k = 0 ; k < 100 ; ++k ) {
   S.insert( k , 0 );
   in.insert( k );
   const auto w = walk( S );  // across every rebuild
   CHECK( std::set< std::uint64_t >( w.begin() , w.end() ) == in );
   }
  for( std::uint64_t k = 0 ; k < 100 ; k += 3 ) {
   S.remove( k );
   in.erase( k );
   }
  const auto w = walk( S );
  CHECK( std::set< std::uint64_t >( w.begin() , w.end() ) == in );
  auto & same = S;  // self-move
  S = std::move( same );
  CHECK( S.size() == in.size() && walk( S ).size() == in.size() );
  }
 {  // strings, copies, moves
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
  FlatHashTable< std::string , Alive > B( A ) , C;  // copy
  CHECK( B.size() == A.size() && Alive::count == 2 * long( A.size() ) );
  CHECK( B.deleted() == A.deleted() && B.capacity() == A.capacity() );
  for( const auto & [ k , v ] : R )
   CHECK( B.find( k )->s == v );
  C = std::move( B );
  CHECK( B.empty() && B.capacity() == 0 && C.size() == A.size() );
  B.insert( "again" , Alive( "x" ) );  // a moved-from table works
  CHECK( B.lookup( "again" )->s == "x" );
  const auto & same = C;  // self-assignment
  C = same;
  A.clear();
  CHECK( A.empty() && Alive::count == long( C.size() ) + 1 );
  CHECK( A.deleted() == 0 && ! A.contains( R.begin()->first ) );
  A.insert( "after clear" , Alive( "y" ) );
  CHECK( A.size() == 1 && A.lookup( "after clear" )->s == "y" );
  FlatHashTable< std::string , Alive > M( std::move( C ) );
  CHECK( C.empty() && M.size() == R.size() );
  }
 CHECK( Alive::count == 0 );  // nothing leaked
 }

// the arena: the addresses never change across many blocks, the objects
// are built from the arguments and destroyed with the arena; the edge
// cases: no object, blocks of one object, the last block exactly full, and
// a constructor that throws when a new block was just taken
struct Counted
{
 static int alive;
 int v;
 explicit Counted( int x ) : v( x ) {
  if( x < 0 )
   throw( std::invalid_argument( "negative" ) );
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
  for( int i = 0 ; i < 100 ; ++i )
   P.push_back( A.make( i ) );
  for( int i = 0 ; i < 100 ; ++i )
   CHECK( P[ i ]->v == i );
  CHECK( A.size() == 100 && Counted::alive == 100 );
  CHECK( A.bytes() == 15 * 7 * sizeof( Counted ) );
  for( int i = 1 ; i < 7 ; ++i )
   CHECK( P[ i ] == P[ i - 1 ] + 1 );
  }
 CHECK( Counted::alive == 0 );
 {
  Arena< Counted > E( 1 );  // nothing made
  CHECK( E.size() == 0 && E.bytes() == 0 && E.begin() == E.end() );
  Arena< Counted > O( 1 );  // a block per object
  for( int i = 0 ; i < 3 ; ++i )
   O.make( i );
  CHECK( O.bytes() == 3 * sizeof( Counted ) && Counted::alive == 3 );
  Arena< Counted > F( 4 );  // two blocks, both full
  for( int i = 0 ; i < 8 ; ++i )
   F.make( i );
  CHECK( F.bytes() == 8 * sizeof( Counted ) && Counted::alive == 11 );
  Arena< Counted > T( 2 );
  T.make( 0 );
  T.make( 1 );  // the block is full
  CHECK_THROWS( T.make( -1 ) , std::invalid_argument );
  CHECK( T.size() == 2 && Counted::alive == 13 );
  CHECK( T.make( 2 )->v == 2 && T.size() == 3 &&
	 T.bytes() == 4 * sizeof( Counted ) );  // the new block is used
  CHECK( T.begin()[ 2 ].v == 2 );
  }
 CHECK( Counted::alive == 0 );
 }

// the iterators of the arena stay valid across the appends of blocks,
// which move the array of the vector of the blocks: an iterator taken on
// the first object, and a constant one, read every object made after them
ADS_TEST( "arena-iterators" )
{
 Arena< int > A( 1 );  // a block per object
 A.make( 7 );
 const auto it = A.begin();
 const Arena< int > & C = A;
 const auto ct = C.cbegin();
 for( int i = 1 ; i < 1000 ; ++i )
  A.make( i );  // the vector reallocates
 CHECK( *it == 7 && *ct == 7 && A.end() - it == 1000 );
 bool same = true;
 for( int i = 1 ; i < 1000 ; ++i )
  same = same && ( it[ i ] == i );
 CHECK( same && ( ct + 999 )[ 0 ] == 999 );
 }

// adopt() leaves the pool with the larger of the two limits of the blocks,
// whether it had blocks (a chain hung) or not (a swap), and the other pool
// with its own: the bytes of the blocks made after it against those of
// pools that never adopted
ADS_TEST( "pool-adopt" )
{
 const auto bytes = []( std::size_t limit , int k ) {
  ObjectPool< long > r( limit );
  for( int i = 0 ; i < k ; ++i )
   r.make( i );
  return( r.bytes() );
  };
 ObjectPool< long > a( 1000 ) , b( 1 );
 b.make( 0 );
 a.adopt( b );  // a has no block: a swap
 for( int i = 1 ; i < 100 ; ++i )
  a.make( i );
 CHECK( a.size() == 100 && a.bytes() == bytes( 1000 , 100 ) );
 CHECK( b.size() == 0 && b.bytes() == 0 );
 for( int i = 0 ; i < 3 ; ++i )
  b.make( i );  // b keeps the limit 1
 CHECK( b.bytes() == bytes( 1 , 3 ) );
 ObjectPool< long > c( 1 ) , d( 1000 );
 c.make( 0 );
 d.make( 0 );
 c.adopt( d );  // c has blocks: a chain
 for( int i = 2 ; i < 100 ; ++i )
  c.make( i );
 CHECK( c.size() == 100 && c.bytes() == bytes( 1000 , 99 ) + bytes( 1 , 1 ) );
 for( int i = 0 ; i < 3 ; ++i )
  d.make( i );  // d keeps the limit 1000
 CHECK( d.bytes() == bytes( 1000 , 3 ) );
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
  const std::size_t h = ( 2 * sizeof( void * ) + z - 1 ) / z;  // header
  const auto at = []( const Counted * x ) {  // the address of a slot
   return( reinterpret_cast< std::uintptr_t >( x ) );
   };
  ObjectPool< Counted > A( 64 );
  std::vector< Counted * > P;
  for( int i = 0 ; i < 100 ; ++i )
   P.push_back( A.make( i ) );
  for( int i = 0 ; i < 100 ; ++i )
   CHECK( P[ i ]->v == i );
  for( int i = 64 ; i < 100 ; ++i )  // blocks of 1, 2, ..., 64
   CHECK( at( P[ i ] ) == at( P[ i - 1 ] ) + z );
  CHECK( A.size() == 100 && Counted::alive == 100 );
  CHECK( A.bytes() == ( 127 + 7 * h ) * z );
  A.free( P[ 7 ] );
  A.free( P[ 3 ] );
  CHECK( A.size() == 98 && Counted::alive == 98 );
  CHECK( A.make( 1000 ) == P[ 3 ] && A.make( 1001 ) == P[ 7 ] );
  CHECK( P[ 3 ]->v == 1000 && P[ 7 ]->v == 1001 && P[ 8 ]->v == 8 );
  CHECK_THROWS( A.make( -1 ) , std::invalid_argument );
  CHECK( A.size() == 100 );  // the slot of the throw first
  CHECK( at( A.make( 5 ) ) == at( P[ 99 ] ) + z );
  CHECK( at( A.make( 6 ) ) == at( P[ 99 ] ) + 2 * z );
  for( Counted * x : P )
   A.free( x );
  CHECK( A.size() == 2 && Counted::alive == 2 );
  ObjectPool< Counted > B( std::move( A ) );  // the objects stay
  CHECK( A.size() == 0 && A.bytes() == 0 && B.size() == 2 );
  ObjectPool< Counted > C( 1 );  // a block per slot
  Counted * c = C.make( 9 );
  C.swap( B );
  CHECK( B.size() == 1 && C.size() == 2 && B.bytes() == ( 1 + h ) * z );
  B.free( c );
  CHECK( Counted::alive == 2 );
  A = std::move( C );
  CHECK( A.size() == 2 && C.size() == 0 );
  CHECK( A.make( 3 )->v == 3 && Counted::alive == 3 );
  CHECK_THROWS( A.free( nullptr ) , contract_violation );
  ObjectPool< Counted > E;  // nothing made
  CHECK( E.size() == 0 && E.bytes() == 0 );
  for( Counted * x = A.make( 4 ) ; x ; x = nullptr )
   A.free( x );
  }
 CHECK( Counted::alive == 3 );  // the pool destroys none
 Counted::alive = 0;

 struct Cell
 {
  long v;
  Cell * next;
  };  // an aggregate
 ObjectPool< Cell > L;
 Cell * h = nullptr;
 for( long i = 0 ; i < 5 ; ++i )
  h = L.make( i , h );
 long t = 0;
 for( Cell * c = h ; c ; c = c->next )
  t = 10 * t + c->v;
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
 for( auto * x : in )
  N.insert( *x );
 CHECK( N == M );
 for( auto * x : in )
  S.free( x );
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
  for( std::size_t i = 0 ; i < n ; ++i )
   p[ i ] = long( i );
  CHECK( p[ n - 1 ] == long( n - 1 ) && p[ 0 ] == 0 );
  a.deallocate( p , n );
  }
 std::vector< long , H > V;
 for( long i = 0 ; i < long( 2 * P ) ; ++i )
  V.push_back( i );
 bool kept = true;
 for( long i = 0 ; i < long( V.size() ) ; ++i )
  kept = kept && V[ i ] == i;
 CHECK( kept && V.size() == 2 * P );
 CHECK( H() == HugePageAllocator< char >() );
 CHECK_THROWS(
   ( void )a.allocate( std::numeric_limits< std::size_t >::max() / 4 ) ,
   std::bad_array_new_length );
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
 for( std::uint64_t x = 0 ; x < 100000 ; ++x )
  S.insert( splitmix64( x ) );
 CHECK( S.size() == 100000 );
 for( int b : { 1 , 10 , 63 } )
  for( std::uint64_t h : { 0ULL , 12345ULL , ~0ULL } )
   CHECK( fibonacciPlace( h , b ) < ( 1ULL << b ) );
 CHECK( fibonacciPlace( ~0ULL , 64 ) != 0 );
 CHECK_THROWS( ( void )fibonacciPlace( 1 , 0 ) , contract_violation );
 ZobristTable Z( 9 , 9 , 7 );
 std::vector< int > B = { 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 0 };
 std::uint64_t h = Z.hash( B );
 h = Z.update( h , 7 , 8 , 0 );  // the hole moves left
 h = Z.update( h , 8 , 0 , 8 );
 std::swap( B[ 7 ] , B[ 8 ] );
 CHECK( h == Z.hash( B ) );
 CHECK_THROWS( ( void )Z( 9 , 0 ) , contract_violation );
 CHECK_THROWS( ( void )Z( 0 , 9 ) , contract_violation );
 CHECK( Z.update( h , 3 , 4 , 4 ) == h );  // a == b, no change
 CHECK_THROWS( ( void )Z.update( h , 3 , 9 , 0 ) , contract_violation );
 CHECK_THROWS( ( void )Z.update( h , 9 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( ( void )Z.hash( std::vector< int >( 8 , 0 ) ) ,
	       contract_violation );
 CHECK_THROWS( ZobristTable( 0 , 1 ) , contract_violation );
 CHECK_THROWS( ZobristTable( 1 , 0 ) , contract_violation );
 ZobristTable Z1( 1 , 1 );
 CHECK( Z1.hash( std::vector< int >( { 0 } ) ) == Z1( 0 , 0 ) );
 CHECK( ZobristTable( 1 , 1 , 2 )( 0 , 0 ) != Z1( 0 , 0 ) );
 for( int b : { 1 , 32 , 64 } )
  CHECK( fibonacciPlace( 0 , b ) == 0 );
 CHECK( fibonacciPlace( 1 , 64 ) == 0x9e3779b97f4a7c15ULL );
 CHECK( fibonacciPlace( 1 , 1 ) == 1 && fibonacciPlace( 2 , 1 ) == 0 );
 CHECK_THROWS( ( void )fibonacciPlace( 1 , 65 ) , contract_violation );
 CHECK_THROWS( ( void )fibonacciPlace( 1 , -1 ) , contract_violation );
 SplitMix64 g2( 0 );
 ( void )g2();
 CHECK( g2() == splitmix64( 0x9e3779b97f4a7c15ULL ) );  // the 2nd word
 TabulationHash T( 3 );
 std::unordered_set< std::uint64_t > U;
 std::vector< int > top( 16 , 0 );
 for( std::uint64_t x = 0 ; x < 4096 ; ++x ) {
  U.insert( T( x ) );
  ++top[ T( x ) >> 60 ];
  }
 CHECK( U.size() == 4096 );
 for( int c : top )
  CHECK( c > 128 && c < 384 );                 // about 256 each
 CHECK( T( 0 ) == TabulationHash( 3 )( 0 ) );  // the same seed
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
   if( Q.push( i ) )
    ++i;
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
 CHECK( F.pop() == 1 && F.push( 3 ) && ! F.push( 4 ) );  // one freed
 CHECK( F.pop() == 2 && F.pop() == 3 && ! F.pop() && F.size() == 0 );
 SPSCQueue< int > W( 4 );  // many laps of 4 places
 int in = 0 , out = 0;
 for( int lap = 0 ; lap < 1000 ; ++lap ) {
  while( W.push( in ) )
   ++in;
  CHECK( W.size() == 4 );
  for( int k = 0 ; k < 1 + lap % 4 ; ++k )
   CHECK( W.pop() == out++ );
  }
 while( auto v = W.pop() )
  CHECK( *v == out++ );
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
   for( int i = 0 ; i < n ; ++i )
    I.push( { k , i } );
   } );
 std::vector< int > next( p , 0 );
 bool ordered = true;
 for( int got = 0 ; got < p * n ; )
  for( auto [ k , i ] : I.take() ) {
   ordered = ordered && ( i == next[ k ] );
   ++next[ k ];
   ++got;
   }
 for( auto & t : T )
  t.join();
 CHECK( ordered && I.empty() );
 for( int k = 0 ; k < p ; ++k )
  CHECK( next[ k ] == n );
 MPSCInbox< int > S;
 S.push( 7 );
 CHECK( ! S.empty() && S.take() == std::vector< int >( { 7 } ) );
 for( int x : { 1 , 2 , 3 } )
  S.push( x );
 CHECK( S.take() == std::vector< int >( { 1 , 2 , 3 } ) );
 CHECK( S.empty() && S.take().empty() );
 MPSCInbox< int > L;  // freed if never taken
 L.push( 1 );
 L.push( 2 );
 }

/// a hash equal for every key: at most 8 keys fit in their two buckets
struct SameHash
{
 using is_avalanching = void;
 std::size_t operator()( std::uint64_t ) const {
  return( std::size_t( 0x9E3779B97F4A7C15ULL ) );
  }
 };

/// a hash with the same tag for every key and the key in the top bits
struct SameTag
{
 using is_avalanching = void;
 std::size_t operator()( std::uint64_t k ) const {
  return( std::size_t( k << 56 ) );
  }
 };

// the concurrent cuckoo table on one thread, against std::unordered_map:
// random operations from an empty table, which grows many times; the empty
// table and one key, a duplicate that keeps its value, insertOrAssign,
// update, reserve and clear; keys whose hashes are all equal, which fit up
// to 8 and then throw, leaving the table as it was; keys with the same
// tag; a hash that is not avalanching; strings
ADS_TEST( "concurrent-cuckoo" )
{
 {
  ConcurrentCuckooTable< std::uint64_t , std::uint64_t > T;
  CHECK( T.empty() && T.size() == 0 && T.capacity() == 8 );
  CHECK( ! T.find( 1 ) && ! T.contains( 1 ) && ! T.erase( 1 ) );
  CHECK( ! T.update( 1 , []( std::uint64_t & v ) { ++v; } ) );
  CHECK( T.insert( 1 , 10 ) && ! T.insert( 1 , 11 ) && T.find( 1 ) == 10u );
  CHECK( ! T.insertOrAssign( 1 , 12 ) && T.find( 1 ) == 12u );
  CHECK( T.update( 1 , []( std::uint64_t & v ) { ++v; } ) );
  CHECK( T.find( 1 ) == 13u && T.size() == 1 && T.consistent() );
  CHECK( T.erase( 1 ) && ! T.contains( 1 ) && T.empty() );
  CHECK( T.insertOrAssign( 2 , 20 ) && T.size() == 1 );
  T.reserve( 1000 );
  CHECK( T.capacity() >= 1000 && T.find( 2 ) == 20u && T.consistent() );
  const std::size_t cap = T.capacity();
  T.reserve( 10 );  // no shrinking
  CHECK( T.capacity() == cap );
  T.clear();
  CHECK( T.empty() && T.capacity() == cap && ! T.contains( 2 ) &&
	 T.consistent() );
  }
 {
  ConcurrentCuckooTable< std::uint64_t , std::uint64_t > T;
  std::unordered_map< std::uint64_t , std::uint64_t > R;
  std::mt19937_64 g( 41 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const std::uint64_t k = g() % 60000 , v = g();
   switch( g() % 6 ) {
    case( 0 ):
    case( 1 ):
     CHECK( T.insert( k , v ) == R.insert( { k , v } ).second );
     break;
    case( 2 ):
     CHECK( T.insertOrAssign( k , v ) == R.insert_or_assign( k , v ).second );
     break;
    case( 3 ):
     CHECK( T.erase( k ) == ( R.erase( k ) == 1 ) );
     break;
    case( 4 ): {
     const bool in = R.count( k );
     if( in )
      R[ k ] ^= v;
     CHECK( T.update( k , [ v ]( std::uint64_t & x ) { x ^= v; } ) == in );
     break;
     }
    default: {
     const auto it = R.find( k );
     const auto o = T.find( k );
     CHECK( o.has_value() == ( it != R.end() ) );
     if( o )
      CHECK( *o == it->second );
     }
    }
   CHECK( T.size() == R.size() );
   if( s % 20000 == 0 )
    CHECK( T.consistent() );
   }
  CHECK( T.consistent() && T.rehashes() > 8 );
  std::size_t n = 0;
  T.forEach( [ & ]( const std::uint64_t & k , std::uint64_t & v ) {
   n += ( R.at( k ) == v );
   ++v;
   } );
  CHECK( n == R.size() );
  const auto & C = T;
  n = 0;
  C.forEach( [ & ]( const std::uint64_t & k , const std::uint64_t & v ) {
   n += ( R.at( k ) + 1 == v );
   } );
  CHECK( n == R.size() );
  }
 {  // all hashes equal
  ConcurrentCuckooTable< std::uint64_t , int , SameHash > T;
  for( std::uint64_t k = 0 ; k < 8 ; ++k )
   CHECK( T.insert( k , int( k ) ) );
  CHECK( T.size() == 8 && T.consistent() );
  CHECK_THROWS( T.insert( 8 , 8 ) , std::length_error );
  CHECK( T.size() == 8 && T.consistent() && ! T.contains( 8 ) );
  for( std::uint64_t k = 0 ; k < 8 ; ++k )
   CHECK( T.find( k ) == int( k ) );
  CHECK( T.erase( 3 ) && T.insert( 8 , 8 ) && T.consistent() );
  }
 {  // all tags equal
  ConcurrentCuckooTable< std::uint64_t , int , SameTag > T;
  for( std::uint64_t k = 0 ; k < 256 ; ++k )
   CHECK( T.insert( k , int( k ) ) );
  CHECK( T.size() == 256 && T.consistent() );
  for( std::uint64_t k = 0 ; k < 256 ; ++k )
   CHECK( T.find( k ) == int( k ) );
  }
 {  // a plain hash, mixed
  ConcurrentCuckooTable< std::uint64_t , int , std::hash< std::uint64_t > > T(
    100 );
  const std::size_t cap = T.capacity();
  for( std::uint64_t k = 0 ; k < 100 ; ++k )
   CHECK( T.insert( k << 32 , int( k ) ) );
  CHECK( T.capacity() == cap && T.rehashes() == 0 && T.consistent() );
  }
 {  // strings
  ConcurrentCuckooTable< std::string , int > T;
  std::unordered_map< std::string , int > R;
  std::mt19937 g( 43 );
  for( int s = 0 ; s < 40000 ; ++s ) {
   const std::string k( 1 + g() % 24 , char( 'a' + g() % 4 ) );
   const std::string key = k + std::to_string( g() % 3000 );
   if( g() % 3 )
    CHECK( T.insertOrAssign( key , s ) == R.insert_or_assign( key ,
							      s ).second );
   else
    CHECK( T.erase( key ) == ( R.erase( key ) == 1 ) );
   CHECK( T.size() == R.size() );
   }
  CHECK( T.consistent() );
  for( const auto & [ k , v ] : R )
   CHECK( T.find( k ) == v );
  }
 }

// the concurrent cuckoo table on real threads: 2, 4 and 8 threads insert
// disjoint keys into a table of capacity 0, which doubles under them, while
// a reader looks keys up and another reserves room, takes the size and
// visits the table; then 4 threads insert, assign, update, remove and find
// their own keys, each keeping the set it expects, and read the keys of
// the others, whose values must be ones written for that key, and at the
// end the table holds the union of the expected sets, each key in one of
// its buckets; last, values that are strings, read under the locks,
// inserted and removed by 4 threads while a reader checks them
ADS_TEST( "concurrent-cuckoo-threads" )
{
 auto key = []( std::uint64_t i ) { return( i * 0x9E3779B97F4A7C15ULL ); };
 for( int p : { 2 , 4 , 8 } ) {
  const std::uint64_t n = 40000;
  ConcurrentCuckooTable< std::uint64_t , std::uint64_t > T;
  std::atomic< bool > done{ false };
  std::atomic< int > bad{ 0 };
  std::vector< std::thread > W;
  for( int t = 0 ; t < p ; ++t )
   W.emplace_back( [ & , t ] {
    for( std::uint64_t i = std::uint64_t( t ) ; i < n ;
	 i += std::uint64_t( p ) )
     if( ! T.insert( key( i ) , i ) )
      ++bad;
    } );
  std::thread reader( [ & ] {
   std::mt19937_64 g( 45 );
   while( ! done.load() ) {
    const std::uint64_t i = g() % n;
    const auto o = T.find( key( i ) );
    if( o && *o != i )
     ++bad;
    }
   } );
  std::thread visitor( [ & ] {
   T.reserve( n / 4 );  // a doubling of its own
   while( ! done.load() ) {
    if( T.size() > n )
     ++bad;
    std::uint64_t m = 0;
    T.forEach( [ & ]( const std::uint64_t & k , std::uint64_t & v ) {
     m += ( key( v ) != k );
     } );
    if( m )
     ++bad;
    std::this_thread::yield();
    }
   } );
  for( auto & w : W )
   w.join();
  done = true;
  reader.join();
  visitor.join();
  CHECK( bad == 0 && T.size() == n && T.consistent() );
  CHECK( T.rehashes() >= 3 );  // 2 doublings past 4096
  for( std::uint64_t i = 0 ; i < n ; ++i )
   CHECK( T.find( key( i ) ) == i );
  }
 {
  const std::uint64_t p = 4 , K = 4096;
  ConcurrentCuckooTable< std::uint64_t , std::uint64_t > T( 64 );
  std::vector< std::unordered_map< std::uint64_t , std::uint64_t > > E( p );
  std::atomic< int > bad{ 0 };
  // the value of the key i written at the step s: i in its top bits
  auto value = []( std::uint64_t i , std::uint64_t s ) {
   return( ( i << 32 ) | s );
   };
  std::vector< std::thread > W;
  for( std::uint64_t t = 0 ; t < p ; ++t )
   W.emplace_back( [ & , t ] {
    std::mt19937_64 g( 47 + t );
    auto & e = E[ t ];
    for( std::uint64_t s = 0 ; s < 60000 ; ++s ) {
     const std::uint64_t r = g() % 8;
     if( r == 7 ) {  // a key of any thread
      const std::uint64_t i = g() % K;
      const auto o = T.find( key( i ) );
      if( o && ( ( *o >> 32 ) != i ) )
       ++bad;
      continue;
      }
     const std::uint64_t i = ( g() % ( K / p ) ) * p + t;  // its own
     const std::uint64_t v = value( i , s );
     bool ok = true;
     switch( r ) {
      case( 0 ):
      case( 1 ):
       ok = T.insert( key( i ) , v ) == e.insert( { i , v } ).second;
       break;
      case( 2 ):
       ok =
	 T.insertOrAssign( key( i ) , v ) == e.insert_or_assign( i ,
								 v ).second;
       break;
      case( 3 ):
      case( 4 ):
       ok = T.erase( key( i ) ) == ( e.erase( i ) == 1 );
       break;
      case( 5 ): {
       const bool in = e.count( i );
       if( in )
	e[ i ] = v;
       ok = T.update( key( i ) , [ v ]( std::uint64_t & x ) { x = v; } ) == in;
       break;
       }
      default: {
       const auto it = e.find( i );
       const auto o = T.find( key( i ) );
       ok = ( it == e.end() ) ? ! o : ( o == it->second );
       }
      }
     if( ! ok )
      ++bad;
     }
    } );
  for( auto & w : W )
   w.join();
  CHECK( bad == 0 && T.consistent() );
  std::size_t n = 0;
  for( const auto & e : E ) {
   n += e.size();
   for( const auto & [ i , v ] : e )
    CHECK( T.find( key( i ) ) == v );
   }
  CHECK( T.size() == n );
  }
 {  // values read under locks
  using CT = ConcurrentCuckooTable< std::uint64_t , std::string >;
  static_assert( ! CT::optimistic );
  static_assert( ConcurrentCuckooTable< std::uint64_t , int >::optimistic );
  const std::uint64_t n = 20000;
  CT T;
  std::atomic< bool > done{ false };
  std::atomic< int > bad{ 0 };
  std::vector< std::thread > W;
  for( std::uint64_t t = 0 ; t < 4 ; ++t )
   W.emplace_back( [ & , t ] {
    for( std::uint64_t i = t ; i < n ; i += 4 ) {
     if( ! T.insert( key( i ) , std::to_string( i ) ) )
      ++bad;
     if( ( i % 3 == 0 ) && ! T.erase( key( i ) ) )
      ++bad;
     }
    } );
  std::thread reader( [ & ] {
   std::mt19937_64 g( 49 );
   while( ! done.load() ) {
    const std::uint64_t i = g() % n;
    const auto o = T.find( key( i ) );
    if( o && ( *o != std::to_string( i ) ) )
     ++bad;
    }
   } );
  for( auto & w : W )
   w.join();
  done = true;
  reader.join();
  CHECK( bad == 0 && T.consistent() && T.size() == n - ( n + 2 ) / 3 );
  for( std::uint64_t i = 0 ; i < n ; ++i )
   CHECK( T.contains( key( i ) ) == ( i % 3 != 0 ) );
  }
 }

// the concurrent flat table on one thread: pairs in the words (keys and
// values of 32 bits, keys of 64 bits with no value), against
// std::unordered_map, on random operations from an empty table that grows
// many times; the key of zero bytes, with the value 0 too, kept apart, and
// update() of it and of the others; pairs in nodes (values of 64 bits,
// keys that are strings), whose references stay valid while the table
// grows; the hash passed with the key, and a wrong one; all the keys on
// one place (SameHash), past the reads that make it grow; reserve(),
// clear() and the reuse of the nodes, forEach()
ADS_TEST( "concurrent-flat-table" )
{
 {
  ConcurrentFlatTable< std::uint32_t , std::uint32_t > T;
  static_assert( decltype( T )::packed );
  CHECK( T.empty() && T.capacity() == 64 && ! T.find( 1 ) &&
	 ! T.contains( 0 ) &&
	 ! T.update( 1 , []( std::uint32_t v ) { return( v + 1 ); } ) &&
	 T.consistent() );
  CHECK( T.insert( 0 , 0 ) && ! T.insert( 0 , 5 ) && T.find( 0 ) == 0u );
  CHECK( T.update( 0 , []( std::uint32_t v ) { return( v + 3 ); } ) &&
	 T.find( 0 ) == 3u && T.size() == 1 );
  const auto r = T.findOrInsert( 7 , 70 );
  CHECK( r.first == 70u && r.second );
  CHECK( T.findOrInsert( 7 , 71 ) == std::make_pair( 70u , false ) );
  CHECK( T.update( 7 , []( std::uint32_t v ) { return( v + 1 ); } ) &&
	 T.find( 7 ) == 71u && T.size() == 2 && T.consistent() );
  std::unordered_map< std::uint32_t , std::uint32_t > R{ { 0 , 3 } ,
							 { 7 , 71 } };
  std::mt19937_64 g( 71 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const auto k = std::uint32_t( g() % 50000 );
   const auto v = std::uint32_t( g() );
   switch( g() % 4 ) {
    case 0: {
     const auto o = T.find( k );
     const auto it = R.find( k );
     CHECK( it == R.end() ? ! o : ( o && *o == it->second ) );
     break;
     }
    case 1:
     CHECK( T.update( k , [ v ]( std::uint32_t ) { return( v ); } ) ==
	    ( R.count( k ) == 1 ) );
     if( R.count( k ) )
      R[ k ] = v;
     break;
    default: {
     const auto x = T.findOrInsert( k , T.hash( k ) , v );
     const bool in = R.emplace( k , v ).second;
     CHECK( x.second == in && x.first == R[ k ] );
     }
    }
   }
  CHECK( T.size() == R.size() && T.rehashes() >= 8 && T.consistent() );
  CHECK( 2 * T.size() <= T.capacity() + 64 * 64 );
  std::size_t c = 0;
  T.forEach(
    [ & ]( std::uint32_t k , std::uint32_t v ) { c += ( R.at( k ) == v ); } );
  CHECK( c == R.size() );
  }
 {
  ConcurrentFlatTable< std::uint64_t , std::monostate > S( 1000 );
  static_assert( decltype( S )::packed );
  CHECK( S.capacity() == 2048 );
  for( std::uint64_t k :
       { std::uint64_t( 0 ) , ~std::uint64_t( 0 ) ,
	 std::uint64_t( 1 ) << 63 } )
   CHECK( S.insert( k , {} ) && ! S.insert( k , {} ) && S.contains( k ) );
  CHECK( S.size() == 3 && ! S.contains( 5 ) && S.consistent() );
  S.clear();
  CHECK( S.empty() && ! S.contains( 0 ) && S.capacity() == 2048 &&
	 S.consistent() );
  }
 {  // a set of pointers compared by the objects: the key held is returned
  struct ByValue
  {
   std::uint64_t operator()( const std::uint64_t * p ) const { return( *p ); }
   };
  struct SameValue
  {
   bool operator()( const std::uint64_t * a , const std::uint64_t * b ) const {
    return( *a == *b );
    }
   };
  using S = ConcurrentFlatTable< const std::uint64_t * , std::monostate ,
				 ByValue , SameValue >;
  static_assert( S::packed );
  constexpr std::uint64_t n = 1 << 15;  // the odd steps permute it
  constexpr unsigned p = 8;
  S T;
  std::vector< std::vector< std::uint64_t > > O(
    p , std::vector< std::uint64_t >( n ) );
  std::vector< std::vector< const std::uint64_t * > > got(
    p , std::vector< const std::uint64_t * >( n ) );
  std::vector< std::atomic< unsigned > > ins( n );
  std::vector< std::thread > W;
  for( unsigned t = 0 ; t < p ; ++t )
   W.emplace_back( [ & , t ] {
    for( std::uint64_t i = 0 ; i < n ; ++i ) {
     const std::uint64_t j = ( i * ( 2 * t + 1 ) ) % n;
     O[ t ][ j ] = j * 0x9E3779B97F4A7C15ULL;
     T.prefetch( T.hash( &O[ t ][ ( j + 1 ) % n ] ) );  // not yet there
     const auto [ q , in ] =
       T.findOrInsertKey( &O[ t ][ j ] , T.hash( &O[ t ][ j ] ) );
     CHECK( *q == O[ t ][ j ] && ( ! in || q == &O[ t ][ j ] ) );
     got[ t ][ j ] = q;
     ins[ j ] += in;
     }
    } );
  for( auto & w : W )
   w.join();
  std::size_t bad = 0;
  for( std::uint64_t j = 0 ; j < n ; ++j ) {
   bad += ( ins[ j ] != 1 );
   for( unsigned t = 1 ; t < p ; ++t )
    bad += ( got[ t ][ j ] != got[ 0 ][ j ] );
   }
  CHECK( bad == 0 && T.size() == n && T.rehashes() >= 9 && T.consistent() );
  const std::uint64_t zero = 0 , other = 0;
  ConcurrentFlatTable< std::uint64_t , std::monostate > Z;
  CHECK( Z.findOrInsertKey( 0 , Z.hash( 0 ) ) ==
	   std::make_pair( std::uint64_t( 0 ) , true ) &&
	 Z.findOrInsertKey( 0 , Z.hash( 0 ) ) ==
	   std::make_pair( std::uint64_t( 0 ) , false ) );
  const auto r = T.findOrInsertKey( &other , T.hash( &other ) );
  CHECK( ! r.second && r.first != &other && *r.first == zero );
  }
 {
  using P = std::pair< std::uint64_t , std::uint64_t >;
  ConcurrentFlatTable< std::uint64_t , P > T;
  static_assert( ! decltype( T )::packed && ! decltype( T )::inlined );
  std::vector< std::pair< std::uint64_t , decltype( T )::Entry * > > E;
  for( std::uint64_t i = 0 ; i < 20000 ; ++i ) {
   const std::uint64_t k = i * 0x9E3779B97F4A7C15ULL;
   auto r = T.findOrInsert( k , T.hash( k ) , P( i , k ) );
   CHECK( r.second && r.first.key == k && r.first.value == P( i , k ) );
   E.push_back( { k , &r.first } );
   }
  CHECK( T.rehashes() >= 9 && T.consistent() );
  for( std::uint64_t i = 0 ; i < E.size() ; ++i ) {
   const auto [ k , p ] = E[ i ];
   CHECK( T.entry( k , T.hash( k ) ) == p && p->value.first == i );
   auto r = T.findOrInsert( k , P( 0 , 0 ) );
   CHECK( ! r.second && &r.first == p );
   }
  CHECK( ! T.entry( 3 , T.hash( 3 ) ) && T.find( E[ 5 ].first )->first == 5u );
  CHECK_THROWS( ( void )T.find( 1 , T.hash( 1 ) + 1 ) , contract_violation );
  T.clear();
  CHECK( T.empty() && ! T.contains( E[ 0 ].first ) && T.consistent() );
  for( std::uint64_t i = 0 ; i < 100 ; ++i )  // the nodes reused
   CHECK( T.insert( i , P( i , i ) ) );
  CHECK( T.size() == 100 && T.find( 99 )->first == 99u && T.consistent() );
  }
 {
  ConcurrentFlatTable< std::string , std::string > T;
  std::unordered_map< std::string , std::string > R;
  std::mt19937_64 g( 72 );
  for( int s = 0 ; s < 20000 ; ++s ) {
   const std::string k = "k" + std::to_string( g() % 5000 );
   const std::string v( std::size_t( g() % 40 ) , 'v' );
   const bool in = R.emplace( k , v ).second;
   auto r = T.findOrInsert( k , v );
   CHECK( r.second == in && r.first.value == R[ k ] );
   }
  CHECK( T.size() == R.size() && T.consistent() );
  std::size_t c = 0;
  T.forEach( [ & ]( const std::string & k , const std::string & v ) {
   c += ( R.at( k ) == v );
   } );
  CHECK( c == R.size() );
  }
 {
  ConcurrentFlatTable< std::uint64_t , std::uint64_t , SameHash > T;
  const std::uint64_t n = 3000;
  for( std::uint64_t i = 1 ; i <= n ; ++i )
   CHECK( T.insert( i , i ) );
  for( std::uint64_t i = 1 ; i <= n ; ++i )
   CHECK( T.find( i ) == i );
  // grown while an eighth full, then read on to the free place
  CHECK( ! T.contains( n + 1 ) && T.consistent() && T.capacity() <= 16 * n );
  }
 {
  ConcurrentFlatTable< std::uint64_t , std::uint32_t > T;
  T.reserve( 100000 );
  const std::size_t cap = T.capacity();
  CHECK( cap >= 200000 && T.rehashes() == 1 );
  T.reserve( 10 );  // no shrinking
  CHECK( T.capacity() == cap && T.consistent() );
  }
 }

/// the value of a pair returned by findOrInsert(): the copy, or that of
/// the node
template< class R >
std::uint64_t flatValue( const R & r )
{
 if constexpr( std::is_integral_v< R > )
  return( std::uint64_t( r ) );
 else if constexpr( requires { r.value.first; } )
  return( r.value.first );
 else
  return( std::uint64_t( r.value ) );
 }

/// the value of the key k in the round e of flatEpochs(), of the type V
template< class V >
V flatEpochValue( std::uint64_t k , std::uint64_t e )
{
 if constexpr( std::is_same_v< V ,
			       std::pair< std::uint64_t , std::uint64_t > > )
  return( V( k + e , e ) );
 else
  return( V( k + e ) );
 }

/// rounds of the table T, its policy of clear() of epochs, against a map:
/// in the round e the keys of a random prefix of a sequence, those of the
/// round before half overlapping, each found after its insertion with its
/// value and the others not, the key 0 included in some rounds, then a
/// clear(); the counts grow now and then, so that the table grows in a
/// later round over groups out of use, and the epochs wrap if the stamps
/// are small
template< class T >
void flatEpochs( int rounds , std::uint64_t most , unsigned seed )
{
 using K = typename T::Key;
 using V = typename T::Value;
 static_assert( T::epochs );
 T t;
 std::mt19937_64 g( seed );
 std::uint64_t e0 = t.epoch() , wraps = 0;
 for( int e = 0 ; e < rounds ; ++e ) {
  const std::uint64_t n = 1 + g() % ( e % 50 == 49 ? 4 * most : most );
  const std::uint64_t off = ( e % 2 ) * n / 2;
  std::unordered_map< K , std::uint64_t > R;
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   const auto k =
     K( ( off + i ) * 0x9E3779B97F4A7C15ULL + ( e % 3 == 0 ? 0 : 1 ) );
   const V v = flatEpochValue< V >( k , e );
   const auto x = t.findOrInsert( k , v );
   const bool in = R.emplace( k , flatValue( x.first ) ).second;
   std::uint64_t w;
   if constexpr( std::is_integral_v< V > )
    w = v;
   else
    w = v.first;
   CHECK( x.second == in && flatValue( x.first ) == w );
   }
  for( std::uint64_t i = 0 ; i < n + 50 ; ++i ) {
   const auto k =
     K( ( off + i ) * 0x9E3779B97F4A7C15ULL + ( e % 3 == 0 ? 0 : 1 ) );
   const auto o = t.find( k );
   CHECK( ( i < n ) == o.has_value() );
   const auto k2 =
     K( ( off + i ) * 0x9E3779B97F4A7C15ULL + ( e % 3 == 0 ? 1 : 0 ) );
   CHECK( ! t.contains( k2 ) );  // of the other rounds
   }
  CHECK( t.size() == R.size() && t.consistent() );
  std::size_t c = 0;
  t.forEach( [ & ]( const K & k , const V & ) { c += R.count( k ); } );
  CHECK( c == R.size() );
  const std::size_t cap = t.capacity();
  t.clear();
  CHECK( t.empty() && t.consistent() && t.capacity() == cap );
  wraps += ( t.epoch() < e0 );
  e0 = t.epoch();
  }
 CHECK( ( rounds < 300 ) || ( sizeof( typename T::Clear::Stamp ) > 1 ) ||
	( wraps >= 1 ) );
 }

// the concurrent flat table cleared in O( 1 ) by epochs (EpochClear): the
// pairs in the words, in places of two words or in nodes, and a set, over
// many rounds of insertions and clears against a map, with stamps of 32
// and of 8 bits, whose epochs wrap every 254 clears and the vector is then
// written whole; the key 0 kept apart and update() across a clear; then 2,
// 4 and 8 threads that insert the same keys at once in each round, so that
// they race to renew the same groups, while a reader checks that no key of
// the round before is found and that a key found stays found with its
// value; the table grows within the rounds over groups out of use
ADS_TEST( "concurrent-flat-table-epochs" )
{
 using P = std::pair< std::uint64_t , std::uint64_t >;
 using E8 = EpochClear< std::uint8_t >;
 using Hu32 = DefaultHash< std::uint32_t >;
 using Hu64 = DefaultHash< std::uint64_t >;
 using T32 =
   ConcurrentFlatTable< std::uint32_t , std::uint32_t , Hu32 ,
			std::equal_to< std::uint32_t > , EpochClear<> >;
 using T64 =
   ConcurrentFlatTable< std::uint64_t , std::uint64_t , Hu64 ,
			std::equal_to< std::uint64_t > , EpochClear<> >;
 using TP = ConcurrentFlatTable< std::uint64_t , P , Hu64 ,
				 std::equal_to< std::uint64_t > , E8 >;
 static_assert( T32::packed && ! TP::packed && ! TP::inlined &&
		! ConcurrentFlatTable< std::uint64_t , P >::epochs );
 flatEpochs< T32 >( 300 , 3000 , 1 );
 flatEpochs< T64 >( 300 , 3000 , 2 );
 flatEpochs< TP >( 300 , 3000 , 3 );
 flatEpochs< ConcurrentFlatTable< std::uint32_t , std::uint32_t , Hu32 ,
				  std::equal_to< std::uint32_t > , E8 > >(
   700 , 200 , 4 );
 {
  ConcurrentFlatTable< std::uint64_t , std::monostate , Hu64 ,
		       std::equal_to< std::uint64_t > , E8 >
    S;
  for( int e = 0 ; e < 600 ; ++e ) {
   for( std::uint64_t k = 0 ; k < 100 ; ++k )
    CHECK( S.insert( k * 7 + std::uint64_t( e % 2 ) , {} ) );
   CHECK( S.size() == 100 && S.contains( 7 + std::uint64_t( e % 2 ) ) &&
	  ! S.contains( 7 + std::uint64_t( 1 - e % 2 ) ) );
   CHECK( S.contains( 0 ) == ( e % 2 == 0 ) );  // the key 0, apart
   S.clear();
   CHECK( S.empty() && ! S.contains( 0 ) && S.consistent() );
   }
  }
 {
  T32 U;  // update() across a clear
  CHECK( U.insert( 5 , 1 ) && U.insert( 0 , 2 ) );
  U.clear();
  CHECK( ! U.update( 5 , []( std::uint32_t v ) { return( v + 1 ); } ) &&
	 ! U.update( 0 , []( std::uint32_t v ) { return( v + 1 ); } ) );
  CHECK( U.insert( 5 , 10 ) &&
	 U.update( 5 , []( std::uint32_t v ) { return( v + 1 ); } ) &&
	 U.find( 5 ) == 11u && U.size() == 1 );
  }
 auto key = []( std::uint64_t i , std::uint64_t e ) {
  return( ( 2 * i + ( e & 1 ) ) * 0x9E3779B97F4A7C15ULL );
  };
 for( int p : { 2 , 4 , 8 } ) {
  T64 T;
  TP N;
  for( std::uint64_t e = 0 ; e < 5 ; ++e ) {
   const std::uint64_t n = 4000 * ( e + 1 );  // growing over old groups
   std::vector< std::atomic< int > > won( n ) , wonN( n );
   for( std::uint64_t i = 0 ; i < n ; ++i )
    won[ i ] = wonN[ i ] = 0;
   std::atomic< bool > done{ false };
   std::atomic< int > bad{ 0 };
   std::vector< std::thread > W;
   for( int t = 0 ; t < p ; ++t )
    W.emplace_back( [ & , t ] {
     for( std::uint64_t j = 0 ; j < n ; ++j ) {
      const std::uint64_t i = ( j * std::uint64_t( 2 * t + 1 ) ) % n;
      const auto r = T.findOrInsert( key( i , e ) , i + e );
      won[ i ] += r.second;
      bad += ( flatValue( r.first ) != i + e );
      const auto q = N.findOrInsert( key( i , e ) , P( i , e ) );
      wonN[ i ] += q.second;
      bad += ( q.first.value.first != i ) || ( q.first.value.second != e );
      }
     } );
   std::thread reader( [ & ] {
    std::mt19937_64 g( 75 + e );
    std::vector< char > seen( n , 0 );
    while( ! done.load() ) {
     const std::uint64_t i = g() % n;
     const auto o = T.find( key( i , e ) );
     bad += o ? ( *o != i + e ) : ( seen[ i ] != 0 );
     seen[ i ] = o.has_value();
     bad += T.contains( key( i , e + 1 ) );  // of the round before
     const auto f = N.entry( key( i , e ) , N.hash( key( i , e ) ) );
     bad += f && ( f->value.first != i );
     }
    } );
   for( auto & w : W )
    w.join();
   done = true;
   reader.join();
   CHECK( bad == 0 && T.size() == n && N.size() == n );
   CHECK( T.consistent() && N.consistent() );
   CHECK(
     std::all_of( won.begin() , won.end() ,
		  []( const std::atomic< int > & x ) {
      return( x.load() == 1 );
      } ) );
   CHECK( std::all_of(
     wonN.begin() , wonN.end() ,
     []( const std::atomic< int > & x ) { return( x.load() == 1 ); } ) );
   T.clear();
   N.clear();
   CHECK( T.empty() && N.empty() && ! T.contains( key( 0 , e ) ) );
   }
  CHECK( T.rehashes() >= 5 );
  }
 }

// the concurrent flat table on 2, 4 and 8 threads, from the least vector:
// all the threads insert the same keys at once, each key inserted by one
// of them only and all of them given the same pair (the same node, or the
// same value), while a reader checks that a key found stays found with
// its value; then updates of the values by all the threads, none lost
ADS_TEST( "concurrent-flat-table-threads" )
{
 auto key = []( std::uint64_t i ) { return( i * 0x9E3779B97F4A7C15ULL ); };
 for( int p : { 2 , 4 , 8 } ) {
  const std::uint64_t n = 30000;
  using P = std::pair< std::uint64_t , std::uint64_t >;
  ConcurrentFlatTable< std::uint64_t , P > T;
  ConcurrentFlatTable< std::uint32_t , std::uint32_t > U;
  std::vector< std::atomic< int > > won( n ) , wonU( n );
  std::vector< std::atomic< const void * > > at( n );
  std::vector< std::atomic< std::uint32_t > > val( n );
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   won[ i ] = 0;
   wonU[ i ] = 0;
   at[ i ] = nullptr;
   val[ i ] = ~0u;
   }
  std::atomic< bool > done{ false };
  std::atomic< int > bad{ 0 };
  std::vector< std::thread > W;
  for( int t = 0 ; t < p ; ++t )
   W.emplace_back( [ & , t ] {
    for( std::uint64_t j = 0 ; j < n ; ++j ) {
     const std::uint64_t i = ( j * std::uint64_t( 2 * t + 1 ) ) % n;
     const auto r = T.findOrInsert( key( i ) , P( i , 0 ) );
     won[ i ] += r.second;
     const void * e = nullptr;
     if( ! at[ i ].compare_exchange_strong( e , &r.first ) &&
	 ( e != &r.first ) )
      ++bad;
     if( r.first.value.first != i )
      ++bad;
     const auto u =
       U.findOrInsert( std::uint32_t( i * 2654435761u ) , std::uint32_t( t ) );
     wonU[ i ] += u.second;
     std::uint32_t x = ~0u;
     if( ! val[ i ].compare_exchange_strong( x ,
					     u.first ) && ( x != u.first ) )
      ++bad;
     }
    } );
  std::thread reader( [ & ] {
   std::mt19937_64 g( 73 );
   std::vector< char > seen( n , 0 );
   while( ! done.load() ) {
    const std::uint64_t i = g() % n;
    const auto o = T.find( key( i ) );
    if( o ? ( o->first != i ) : ( seen[ i ] != 0 ) )
     ++bad;
    seen[ i ] = o.has_value();
    }
   } );
  for( auto & w : W )
   w.join();
  done = true;
  reader.join();
  CHECK( bad == 0 && T.size() == n && U.size() == n );
  CHECK( T.consistent() && U.consistent() && T.rehashes() >= 9 );
  CHECK(
    std::all_of( won.begin() , won.end() , []( const std::atomic< int > & x ) {
     return( x.load() == 1 );
     } ) );
  CHECK(
    std::all_of( wonU.begin() , wonU.end() ,
		 []( const std::atomic< int > & x ) {
     return( x.load() == 1 );
     } ) );
  W.clear();
  const std::uint64_t m = 512 , rounds = 200;
  ConcurrentFlatTable< std::uint32_t , std::uint32_t > C;
  for( std::uint32_t i = 0 ; i < m ; ++i )
   CHECK( C.insert( i , 0 ) );
  for( int t = 0 ; t < p ; ++t )
   W.emplace_back( [ & ] {
    for( std::uint64_t r = 0 ; r < rounds ; ++r )
     for( std::uint32_t i = 0 ; i < m ; ++i )
      C.update( i , []( std::uint32_t v ) { return( v + 1 ); } );
    // more keys meanwhile, so that the table grows under the updates
    for( std::uint32_t i = 0 ; i < 4000 ; ++i )
     C.insert( std::uint32_t( m ) + i , 1 );
    } );
  for( auto & w : W )
   w.join();
  for( std::uint32_t i = 0 ; i < m ; ++i )
   CHECK( C.find( i ) == std::uint32_t( p ) * rounds );
  CHECK( C.size() == m + 4000 && C.consistent() );
  }
 }

// the concurrent flat table of keys and values of 64 bits, in places of
// two words written by a compare-and-swap of 16 bytes where the processor
// has one (inlined), in nodes elsewhere, against std::unordered_map on
// random operations from an empty table that grows many times; the key 0
// with the value 0, kept apart, and update() of it and of the others when
// inlined; then 2, 4 and 8 threads that insert the same keys at once, each
// key inserted by one of them only and all of them given the same value,
// and that update the values of other keys while the table grows, none
// lost
ADS_TEST( "concurrent-flat-table-words" )
{
 using Table = ConcurrentFlatTable< std::uint64_t , std::uint64_t >;
 // the value of a pair returned, and update(), which an inlined table
 // has, through templates whose branches need not compile for both
 auto value = []( const auto & r ) -> std::uint64_t {
  if constexpr( std::is_same_v< std::decay_t< decltype( r ) > ,
				std::uint64_t > )
   return( r );
  else
   return( r.value );
  };
 auto update = []( auto & t , std::uint64_t k , auto f ) {
  if constexpr( std::decay_t< decltype( t ) >::inlined )
   return( t.update( k , f ) );
  else
   return( false );
  };
 {
  Table T;
  CHECK( T.insert( 0 , 0 ) && ! T.insert( 0 , 5 ) && T.find( 0 ) == 0u );
  CHECK( value( T.findOrInsert( 0 , 7 ).first ) == 0u && T.size() == 1 );
  if constexpr( Table::inlined ) {
   CHECK( update( T , 0 , []( std::uint64_t v ) { return( v + 3 ); } ) );
   CHECK( T.find( 0 ) == 3u );
   CHECK( ! update( T , 9 , []( std::uint64_t v ) { return( v ); } ) );
   }
  std::unordered_map< std::uint64_t ,
		      std::uint64_t > R{ { 0 , *T.find( 0 ) } };
  std::mt19937_64 g( 74 );
  for( int s = 0 ; s < 200000 ; ++s ) {
   const std::uint64_t k = ( g() % 60000 ) * 0x9E3779B97F4A7C15ULL;
   const std::uint64_t v = g();
   if( g() % 3 == 0 ) {
    const auto o = T.find( k );
    const auto it = R.find( k );
    CHECK( it == R.end() ? ! o : ( o && *o == it->second ) );
    continue;
    }
   const auto x = T.findOrInsert( k , T.hash( k ) , v );
   const bool in = R.emplace( k , v ).second;
   CHECK( x.second == in && value( x.first ) == R[ k ] );
   }
  CHECK( T.size() == R.size() && T.rehashes() >= 8 && T.consistent() );
  std::size_t c = 0;
  T.forEach(
    [ & ]( std::uint64_t k , std::uint64_t v ) { c += ( R.at( k ) == v ); } );
  CHECK( c == R.size() );
  T.clear();
  CHECK( T.empty() && ! T.contains( 0 ) && T.consistent() );
  }
 for( int p : { 2 , 4 , 8 } ) {
  const std::uint64_t n = 30000;
  Table T;
  std::vector< std::atomic< int > > won( n );
  std::vector< std::atomic< std::uint64_t > > val( n );
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   won[ i ] = 0;
   val[ i ] = ~std::uint64_t( 0 );
   }
  std::atomic< int > bad{ 0 };
  std::vector< std::thread > W;
  for( int t = 0 ; t < p ; ++t )
   W.emplace_back( [ & , t ] {
    for( std::uint64_t j = 0 ; j < n ; ++j ) {
     const std::uint64_t i = ( j * std::uint64_t( 2 * t + 1 ) ) % n;
     const auto r =
       T.findOrInsert( i * 0x9E3779B97F4A7C15ULL , std::uint64_t( t ) );
     won[ i ] += r.second;
     std::uint64_t x = ~std::uint64_t( 0 );
     if( ! val[ i ].compare_exchange_strong( x , value( r.first ) ) &&
	 ( x != value( r.first ) ) )
      ++bad;
     if constexpr( Table::inlined )
      if( j % 4 == 0 )  // keys inserted, under growths
       update( T , ( n + std::uint64_t( t ) ) * 0x9E3779B97F4A7C15ULL ,
	       []( std::uint64_t v ) { return( v + 1 ); } );
     if( j == 0 )
      T.insert( ( n + std::uint64_t( t ) ) * 0x9E3779B97F4A7C15ULL , 0 );
     }
    } );
  for( auto & w : W )
   w.join();
  CHECK( bad == 0 && T.size() == n + std::uint64_t( p ) && T.consistent() );
  CHECK(
    std::all_of( won.begin() , won.end() , []( const std::atomic< int > & x ) {
     return( x.load() == 1 );
     } ) );
  if constexpr( Table::inlined )
   for( int t = 0 ; t < p ; ++t )
    CHECK( T.find( ( n + std::uint64_t( t ) ) * 0x9E3779B97F4A7C15ULL ) ==
	   ( n - 1 ) / 4 );
  }
 }


// findKey() of the concurrent flat table: a set of pointers compared by
// the objects, which one thread fills while 3 others look up pointers to
// equal objects of their own, each getting nothing or the pointer held,
// and the key found staying found; then the words of 64 bits (inlined or
// in nodes) and the strings in nodes, the key 0 kept apart, and no
// insertion by findKey()
ADS_TEST( "concurrent-flat-table-find-key" )
{
 struct ByValue
 {
  std::uint64_t operator()( const std::uint64_t * p ) const { return( *p ); }
  };
 struct SameValue
 {
  bool operator()( const std::uint64_t * a , const std::uint64_t * b ) const {
   return( *a == *b );
   }
  };
 using S = ConcurrentFlatTable< const std::uint64_t * , std::monostate ,
				ByValue ,
				SameValue >;
 static_assert( S::packed );
 constexpr std::uint64_t n = 1 << 14;
 constexpr int readers = 3;
 S T;
 std::vector< std::uint64_t > O( n );
 std::vector< std::vector< std::uint64_t > > R(
   readers , std::vector< std::uint64_t >( n ) );
 for( std::uint64_t i = 0 ; i < n ; ++i ) {
  for( auto & r : R )
   r[ i ] = ( i + 1 ) * 0x9E3779B97F4A7C15ULL;
  }
 std::atomic< bool > done{ false };
 std::atomic< int > bad{ 0 };
 std::vector< std::thread > W;
 W.emplace_back( [ & ] {
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   O[ i ] = ( i + 1 ) * 0x9E3779B97F4A7C15ULL;
   if( ! T.findOrInsertKey( &O[ i ] , T.hash( &O[ i ] ) ).second )
    ++bad;
   }
  } );
 for( int t = 0 ; t < readers ; ++t )
  W.emplace_back( [ & , t ] {
   std::vector< char > seen( n , 0 );
   std::mt19937_64 g( 91 + t );
   for( bool last = false ; ! last ; ) {
    last = done.load();
    for( int r = 0 ; r < 4096 ; ++r ) {
     const std::uint64_t i = g() % n;
     const auto q = T.findKey( &R[ t ][ i ] );
     if( q ? ( *q != &O[ i ] ) : ( seen[ i ] != 0 ) )
      ++bad;
     seen[ i ] = q.has_value();
     }
    }
   for( std::uint64_t i = 0 ; i < n ; ++i )
    bad += T.findKey( &R[ t ][ i ] ) != &O[ i ];
   } );
 W[ 0 ].join();
 done = true;
 for( std::size_t t = 1 ; t < W.size() ; ++t )
  W[ t ].join();
 CHECK( bad == 0 && T.size() == n && T.rehashes() >= 8 && T.consistent() );
 {
  ConcurrentFlatTable< std::uint64_t , std::uint64_t > U;
  CHECK( ! U.findKey( 0 ) && ! U.findKey( 7 ) && U.empty() );
  U.insert( 0 , 3 );
  U.insert( 7 , 4 );
  CHECK( U.findKey( 0 ) == 0u && U.findKey( 7 ) == 7u && ! U.findKey( 8 ) &&
	 U.size() == 2 );
  ConcurrentFlatTable< std::string , int > N;
  static_assert( ! decltype( N )::packed && ! decltype( N )::inlined );
  N.insert( "state" , 1 );
  CHECK( N.findKey( "state" ) == std::string( "state" ) &&
	 ! N.findKey( "other" ) && N.size() == 1 );
  }
 }

/// the table of one writer W against std::unordered_map on random
/// operations of the keys made by key(), from an empty table that grows
/// many times, the key 0 included, cleared every 3000 operations
template< class W , class Key >
void singleWriterUnit( Key && key )
{
 W T;
 std::unordered_map< std::uint64_t , std::uint64_t > M;
 std::mt19937_64 g( 17 );
 for( int r = 0 ; r < 30000 ; ++r ) {
  const std::uint64_t k = key( g() % 5000 );
  switch( g() % 4 ) {
   case( 0 ):
   case( 1 ): {
    const auto [ v , in ] = T.findOrInsert( k , r );
    const auto [ it , in2 ] = M.try_emplace( k , std::uint64_t( r ) );
    CHECK( in == in2 && v == it->second );
    break;
    }
   case( 2 ): {
    const bool u = T.update( k , []( std::uint64_t x ) { return( x + 1 ); } );
    const auto it = M.find( k );
    CHECK( u == ( it != M.end() ) );
    if( u )
     ++it->second;
    break;
    }
   default: {
    const auto it = M.find( k );
    const auto f = T.find( k );
    CHECK( f.has_value() == ( it != M.end() ) &&
	   T.contains( k ) == f.has_value() );
    if( f )
     CHECK( *f == it->second && T.findKey( k ) == k );
    }
   }
  if( r % 3000 == 2999 ) {
   CHECK( T.size() == M.size() && T.consistent() );
   std::size_t c = 0;
   T.forEach( [ & ]( std::uint64_t a , std::uint64_t b ) {
    c += ( M.count( a ) == 1 ) && ( M[ a ] == b );
    } );
   CHECK( c == M.size() );
   T.clear();
   M.clear();
   CHECK( T.empty() && ! T.contains( key( 0 ) ) );
   }
  }
 CHECK( T.rehashes() >= 6 && T.consistent() );
 }

// the table of one writer on one thread: the pairs in two words against
// std::unordered_map, with ZeroingClear and with EpochClear of 8 bits,
// whose epochs wrap many times; the key 0 kept apart; a set of pointers
// compared by the objects, the pointer held returned; reserve()
ADS_TEST( "single-writer-flat-table" )
{
 auto mix = []( std::uint64_t i ) { return( i * 0x9E3779B97F4A7C15ULL ); };
 auto low = []( std::uint64_t i ) { return( i ); };
 using Z = SingleWriterFlatTable< std::uint64_t , std::uint64_t >;
 using E = SingleWriterFlatTable<
   std::uint64_t , std::uint64_t , DefaultHash< std::uint64_t > ,
   std::equal_to< std::uint64_t > , EpochClear< std::uint8_t > >;
 static_assert( ! Z::epochs && E::epochs );
 singleWriterUnit< Z >( mix );
 singleWriterUnit< Z >( low );
 singleWriterUnit< E >( mix );
 singleWriterUnit< E >( low );
 {
  E T;
  for( int c = 0 ; c < 600 ; ++c ) {  // the 8-bit epoch wraps twice
   for( std::uint64_t i = 0 ; i < 20 ; ++i )
    CHECK( T.insert( mix( i + std::uint64_t( c ) ) , i ) );
   CHECK( T.size() == 20 && T.consistent() &&
	  T.find( mix( std::uint64_t( c ) + 19 ) ) == 19u );
   T.clear();
   CHECK( ! T.contains( mix( std::uint64_t( c ) ) ) && T.empty() );
   }
  CHECK( T.epoch() >= 1 && T.epoch() < 255 );
  }
 {
  struct ByValue
  {
   std::uint64_t operator()( const std::uint64_t * p ) const { return( *p ); }
   };
  struct SameValue
  {
   bool operator()( const std::uint64_t * a , const std::uint64_t * b ) const {
    return( *a == *b );
    }
   };
  SingleWriterFlatTable< const std::uint64_t * , std::monostate , ByValue ,
			 SameValue >
    S( 1000 );
  const std::size_t cap = S.capacity();
  std::vector< std::uint64_t > a( 1000 ) , b( 1000 );
  for( std::uint64_t i = 0 ; i < 1000 ; ++i ) {
   a[ i ] = b[ i ] = mix( i + 1 );
   CHECK( S.findOrInsertKey( &a[ i ] , S.hash( &a[ i ] ) ).second );
   }
  for( std::uint64_t i = 0 ; i < 1000 ; ++i ) {
   const auto [ q , in ] = S.findOrInsertKey( &b[ i ] , S.hash( &b[ i ] ) );
   CHECK( ! in && q == &a[ i ] && S.findKey( &b[ i ] ) == &a[ i ] );
   }
  CHECK( S.size() == 1000 && S.capacity() == cap && S.rehashes() == 0 );
  S.reserve( 5000 );
  CHECK( S.capacity() >= 10000 && S.rehashes() == 1 && S.consistent() &&
	 S.findKey( &b[ 7 ] ) == &a[ 7 ] );
  }
 }

/// one writer that inserts n keys into T and raises their values, and p
/// readers that look them up meanwhile: a key found stays found, with the
/// value of its insertion or a later one, never less than one seen before
template< class T >
void singleWriterThreads( int p , T & tab , std::uint64_t n )
{
 auto key = []( std::uint64_t i ) { return( i * 0x9E3779B97F4A7C15ULL ); };
 std::atomic< bool > done{ false };
 std::atomic< int > bad{ 0 };
 std::vector< std::thread > R;
 for( int t = 0 ; t < p ; ++t )
  R.emplace_back( [ & , t ] {
   std::vector< std::uint64_t > seen( n , 0 );  // the value seen, plus 1
   std::mt19937_64 g( 5 + t );
   for( bool last = false ; ! last ; ) {
    last = done.load();
    for( int r = 0 ; r < 4096 ; ++r ) {
     const std::uint64_t i = g() % n;
     const auto f = tab.find( key( i ) );
     if( ! f ) {
      bad += seen[ i ] != 0;
      continue;
      }
     // the value of i is i at its insertion, raised by n at each round
     bad += ( *f % n != i ) || ( *f + 1 < seen[ i ] );
     seen[ i ] = *f + 1;
     }
    }
   } );
 for( std::uint64_t i = 0 ; i < n ; ++i )
  bad += ! tab.insert( key( i ) , i );
 for( int round = 0 ; round < 3 ; ++round )
  for( std::uint64_t i = 0 ; i < n ; ++i )
   tab.update( key( i ) , [ n ]( std::uint64_t x ) { return( x + n ); } );
 done = true;
 for( auto & r : R )
  r.join();
 CHECK( bad == 0 && tab.size() == n && tab.consistent() &&
	tab.rehashes() >= 8 );
 for( std::uint64_t i = 0 ; i < n ; ++i )
  bad += tab.find( key( i ) ) != i + 3 * n;
 CHECK( bad == 0 );
 }

// the table of one writer on 1 + p threads, p = 1, 3, 7: the writer grows
// the table from the least vector while the readers read on, and raises
// the values; with EpochClear, several rounds separated by clear()
ADS_TEST( "single-writer-flat-table-threads" )
{
 for( int p : { 1 , 3 , 7 } ) {
  SingleWriterFlatTable< std::uint64_t , std::uint64_t > T;
  singleWriterThreads( p , T , 30000 );
  SingleWriterFlatTable< std::uint64_t , std::uint64_t ,
			 DefaultHash< std::uint64_t > ,
			 std::equal_to< std::uint64_t > , EpochClear<> >
    E;
  for( int round = 0 ; round < 3 ; ++round ) {
   singleWriterThreads( p , E , 20000 );
   E.clear();
   CHECK( E.empty() && E.epoch() == std::uint64_t( round + 2 ) );
   }
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
/*------------------------ End File engineered.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
