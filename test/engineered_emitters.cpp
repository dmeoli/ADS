/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_emitters.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tests of the structures that tell their changes and of the
 * aggregates they feed, beyond the book: the events of IndexedDaryHeap,
 * its exchanges applied to a copy of its vector, the events of RadixHeap,
 * DialQueue and BucketQueue, each kept by an observer as a multiset of
 * pairs and checked against the queue and against a multiset kept by
 * hand, alone, in a batch whose net events coalesce the copies of an item
 * by their keys, and attached at run time, the events of
 * ObservableArray, their batches and their undo, the concepts of
 * Algebra.h, Fenwick over a group, a commutative monoid, a monoid and a
 * group that does not commute, FenwickObserver audited by Shadow against
 * a recomputation from scratch, and SlidingAggregate by every algorithm
 * against a scan of its window and against MinQueue and RealTimeMinQueue.
 *
 *   engineered_emitters <test>
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

#include "ADS/ADT.h"
#include "ADS/Algebra.h"
#include "ADS/Modification.h"
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DialQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/queue/MinQueue.h"
#include "ADS/queue/RealTimeMinQueue.h"
#include "ADS/queue/SlidingAggregate.h"
#include "ADS/sequence/Fenwick.h"
#include "ADS/sequence/ObservableArray.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <random>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ALGEBRAS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the monoid of the concatenations, which does not commute
struct Concat
{
 using Value = std::string;
 static Value identity( void ) { return( Value() ); }
 static Value combine( const Value & a , const Value & b ) { return( a + b ); }
 };

/// the maps x -> s x + b with s = 1 or -1, a group that does not commute:
/// combine( f , g ) is f first, then g
struct Affine
{
 struct Value
 {
  long long s = 1 , b = 0;
  bool operator==( const Value & ) const = default;
  };
 static Value identity( void ) { return( Value() ); }
 static Value combine( const Value & f , const Value & g ) {
  return( Value{ g.s * f.s , g.s * f.b + g.b } );
  }
 static Value inverse( const Value & f ) {
  return( Value{ f.s , -f.s * f.b } );
  }
 };

/// the first value, a selective monoid that does not commute
struct First
{
 struct Value
 {
  int v = -1;
  bool operator==( const Value & ) const = default;
  };
 static constexpr bool selective = true;
 static Value identity( void ) { return( Value() ); }
 static Value combine( const Value & a , const Value & b ) {
  return( a.v < 0 ? b : a );
  }
 };

/// the action of adding to a sum, n values at a time
struct AddTo
{
 using Map = long long;
 static Map id( void ) { return( 0 ); }
 static Map compose( Map f , Map g ) { return( f + g ); }
 static long long apply( Map f , long long a , std::size_t n ) {
  return( a + f * ( long long )( n ) );
  }
 };

static_assert( AbelianGroup< SumOf< long long > > );
static_assert( CommutativeMonoid< MinOf< int > > &&
	       Selective< MinOf< int > > && ! Group< MinOf< int > > );
static_assert( Selective< MaxOf< double > > && ! exact_v< MaxOf< double > > );
static_assert( Monoid< Concat > && ! CommutativeMonoid< Concat > &&
	       ! Group< Concat > );
static_assert( Group< Affine > && ! AbelianGroup< Affine > );
static_assert( Selective< First > && ! CommutativeMonoid< First > );
static_assert( Action< AddTo , SumOf< long long > > );
static_assert( exact_v< SumOf< long long > > && ! exact_v< SumOf< double > > );
static_assert( MinOf< int >::identity() == std::numeric_limits< int >::max() );
static_assert( MaxOf< double >::identity() ==
	       -std::numeric_limits< double >::infinity() );

/*--------------------------------------------------------------------------*/
/*--------------------------- THE HEAP OBSERVERS ---------------------------*/
/*--------------------------------------------------------------------------*/

using HIns = mod::Inserted< int , int >;
using HEra = mod::Erased< int , int >;
using HChg = mod::KeyChanged< int , int >;
using HRel = mod::Relocated< std::size_t >;

/// an observer that keeps the keys of the ids and a copy of the vector of
/// the heap, its places of the ids, from the events alone
struct HeapMirror
{
 std::vector< int > key , at;  ///< the key of each id, the id of a place
 std::vector< int > pos;      ///< the place of each id, -1 if absent
 int rel = 0;                 ///< the exchanges received
 explicit HeapMirror( int n )
     : key( std::size_t( n ) , 0 ) , pos( std::size_t( n ) , -1 ) { }
 void on( const HIns & e ) {
  key[ std::size_t( e.h ) ] = e.key;
  pos[ std::size_t( e.h ) ] = int( at.size() );
  at.push_back( e.h );
  }
 void on( const HEra & e ) noexcept {
  const auto p = std::size_t( pos[ std::size_t( e.h ) ] );
  at[ p ] = -1;  // a hole, filled by Relocated
  pos[ std::size_t( e.h ) ] = -1;
  if( p + 1 == at.size() )
   at.pop_back();
  }
 void on( const HChg & e ) { key[ std::size_t( e.h ) ] = e.to; }
 void on( const HRel & e ) {
  ++rel;
  std::swap( at[ e.from ] , at[ e.to ] );
  for( std::size_t p : { e.from , e.to } )
   if( at[ p ] >= 0 )
    pos[ std::size_t( at[ p ] ) ] = int( p );
  while( ! at.empty() && ( at.back() < 0 ) )
   at.pop_back();
  }
 void on( const mod::Cleared & ) noexcept {
  for( int id : at )
   if( id >= 0 )
    pos[ std::size_t( id ) ] = -1;
  at.clear();
  }
 /// whether the copy is a heap of the keys and agrees with the heap
 template< class H >
 [[nodiscard]] bool audit( const H & h ) const {
  if( at.size() != h.size() )
   return( false );
  for( std::size_t p = 0 ; p < at.size() ; ++p ) {
   const int id = at[ p ];
   if( ( id < 0 ) || ! h.contains( id ) ||
       ( key[ std::size_t( id ) ] != h.key( id ) ) ||
       ( pos[ std::size_t( id ) ] != int( p ) ) )
    return( false );
   if( ( p > 0 ) && ( key[ std::size_t( id ) ] <
		      key[ std::size_t( at[ ( p - 1 ) / 4 ] ) ] ) )
    return( false );
   }
  return( at.empty() || ( at[ 0 ] == h.top().id ) );
  }
 };

/// an observer of the ids and keys only, which never wants `Relocated`
struct HeapKeys
{
 std::vector< int > key;
 std::vector< bool > in;
 explicit HeapKeys( int n )
     : key( std::size_t( n ) , 0 ) , in( std::size_t( n ) , false ) { }
 void on( const HIns & e ) {
  key[ std::size_t( e.h ) ] = e.key;
  in[ std::size_t( e.h ) ] = true;
  }
 void on( const HEra & e ) noexcept { in[ std::size_t( e.h ) ] = false; }
 void on( const HChg & e ) { key[ std::size_t( e.h ) ] = e.to; }
 void on( const mod::Cleared & ) noexcept {
  for( std::size_t i = 0 ; i < in.size() ; ++i )
   in[ i ] = false;
  }
 template< class H >
 [[nodiscard]] bool audit( const H & h ) const {
  for( int id = 0 ; id < h.ids() ; ++id )
   if( ( in[ std::size_t( id ) ] != h.contains( id ) ) ||
       ( h.contains( id ) && ( key[ std::size_t( id ) ] != h.key( id ) ) ) )
    return( false );
  return( true );
  }
 };

/// random operations on a heap of n ids, each checked by its observers'
/// audits (level 3 of the contracts) and by the checks of `check`
template< class H , class C >
void heapRun( H & h , int n , unsigned seed , int ops , C && check )
{
 std::mt19937 rng( seed );
 for( int t = 0 ; t < ops ; ++t ) {
  const int id = int( rng() % unsigned( n ) );
  const int r = int( rng() % 6 );
  if( ! h.contains( id ) )
   h.push( id , int( rng() % 1000 ) );
  else if( r == 0 )
   h.decrease( id , h.key( id ) - int( rng() % 100 ) );
  else if( r == 1 )
   h.increase( id , h.key( id ) + int( rng() % 100 ) );
  else if( r == 2 )
   h.erase( id );
  else if( r == 3 )
   h.pop();
  else if( r == 4 )
   h.update( id , int( rng() % 1000 ) );
  else
   h.pushOrDecrease( id , int( rng() % 1000 ) );
  check();
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE ARRAY LOGS -----------------------------*/
/*--------------------------------------------------------------------------*/

using Arr = ObservableArray< long long , Dynamic<> >;

/// an observer that keeps the events of an array of long long
struct ArrayLog
{
 std::vector< mod::variant_t< Arr::modifications > > ev;
 int groups = 0;
 void on( const Arr::Assign & e ) { ev.emplace_back( e ); }
 void on( const Arr::Insert & e ) { ev.emplace_back( e ); }
 void on( const Arr::Erase & e ) noexcept {
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void on( const mod::Cleared & e ) noexcept {
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void on( const mod::Rebuilt & e ) noexcept {
  try {
   ev.emplace_back( e );
   }
  catch( ... ) {
   }
  }
 void group_begin( void ) { ++groups; }
 void group_end( void ) { }
 };

/// the prefix sums of an array recomputed from scratch: its values kept
/// from the events, each prefix by a scan
struct ScratchPrefix
{
 std::vector< long long > a;
 const std::vector< long long > * src = nullptr;
 bool stale = false;
 void on( const Arr::Assign & e ) {
  if( ! stale )
   a[ e.h ] = e.to;
  }
 void on( const Arr::Insert & e ) {
  if( ! stale )
   a.push_back( e.value );
  }
 void on( const Arr::Erase & ) noexcept {
  if( ! stale )
   a.pop_back();
  }
 void on( const mod::Cleared & ) noexcept {
  a.clear();
  stale = false;
  }
 void on( const mod::Rebuilt & ) noexcept { stale = true; }
 [[nodiscard]] long long prefix( std::size_t r ) {
  if( stale ) {
   a = *src;
   stale = false;
   }
  long long s = 0;
  for( std::size_t i = 0 ; i < r ; ++i )
   s += a[ i ];
  return( s );
  }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- THE WINDOW CHECKS ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the aggregate of a window by a scan
template< class G >
typename G::Value scan( const std::deque< typename G::Value > & q )
{
 typename G::Value s = G::identity();
 for( const auto & x : q )
  s = G::combine( s , x );
 return( s );
 }

/// random operations on a window of values made by `make`, each compared
/// with a scan of a deque, with the items in order
template< class W , class Make >
void windowRun( W & w , Make && make , unsigned seed , int ops )
{
 using G = typename W::Algebra;
 std::mt19937 rng( seed );
 std::deque< typename G::Value > q;
 for( int t = 0 ; t < ops ; ++t ) {
  // phases of growth and of shrinking, for long rotations
  const bool grow = ( ( t / 200 ) % 2 ) == 0;
  if( q.empty() || ( rng() % 10 < ( grow ? 7u : 3u ) ) ) {
   const auto x = make( rng );
   w.enqueue( x );
   q.push_back( x );
   }
  else {
   CHECK( w.top() == q.front() );
   CHECK( w.dequeue() == q.front() );
   q.pop_front();
   }
  CHECK( w.size() == q.size() );
  CHECK( w.aggregate() == scan< G >( q ) );
  if( t % 97 == 0 ) {
   std::size_t i = 0;
   bool same = true;
   w.forEach( [ & ]( const auto & x ) { same = same && ( x == q[ i++ ] ); } );
   CHECK( same && ( i == q.size() ) );
   }
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE HEAP -------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-heap" )
{
 // nobody listens: the plain heap, no storage for the subject
 static_assert( sizeof( IndexedDaryHeap< int > ) ==
		sizeof( std::vector< IndexedDaryHeap< int >::Entry > ) +
		  sizeof( std::vector< int > ) );
 static_assert( ! IndexedDaryHeap< int >::Subject::template listened< HRel > );
 static_assert(
   ! IndexedDaryHeap< int , 4 ,
		      Static< HeapKeys > >::Subject::template listened<
     HRel > );
 // a static observer of the ids and keys, the heap checked by its audit
 const int n = 64;
 HeapKeys k( n );
 IndexedDaryHeap< int , 4 , Static< HeapKeys > > h( n , k );
 heapRun( h , n , 7 , 4000 , [ & ] { CHECK( k.audit( h ) ); } );
 h.clear();
 CHECK( k.audit( h ) );
 // the same operations on the plain heap give the same heap
 IndexedDaryHeap< int > p( n );
 HeapKeys k2( n );
 IndexedDaryHeap< int , 4 , Static< HeapKeys > > h2( n , k2 );
 heapRun( p , n , 11 , 2000 , [] { } );
 heapRun( h2 , n , 11 , 2000 , [] { } );
 CHECK( p.size() == h2.size() );
 while( ! p.empty() ) {
  const auto a = p.pop();
  const auto b = h2.pop();
  CHECK( ( a.id == b.id ) && ( a.key == b.key ) );
  }
 }

ADS_TEST( "emit-heap-relocated" )
{
 // a copy of the vector of the heap kept by the exchanges alone
 const int n = 50;
 HeapMirror m( n );
 IndexedDaryHeap< int , 4 , Static< HeapMirror > > h( n , m );
 heapRun( h , n , 3 , 5000 , [ & ] { CHECK( m.audit( h ) ); } );
 CHECK( m.rel > 0 );
 // and attached at run time, with an observer that does not want them
 IndexedDaryHeap< int , 4 , Dynamic<> > d( n );
 HeapMirror m2( n );
 HeapKeys k2( n );
 d.attach( k2 );
 heapRun( d , n , 5 , 500 , [ & ] { CHECK( k2.audit( d ) ); } );
 while( ! d.empty() )
  d.pop();
 const int id = d.attach( m2 );
 heapRun( d , n , 9 , 3000 ,
	  [ & ] { CHECK( m2.audit( d ) && k2.audit( d ) ); } );
 d.detach( id );
 d.reset( n );
 CHECK( k2.audit( d ) );
 }

ADS_TEST( "emit-heap-batch" )
{
 // the erasure told before the removal, then a batch coalesced
 struct Before
 {
  const IndexedDaryHeap< int , 4 , Dynamic<> > * h = nullptr;
  int seen = 0 , present = 0 , changes = 0 , inserts = 0 , groups = 0;
  void on( const HEra & e ) noexcept {
   ++seen;
   present += h->contains( e.h ) ? 1 : 0;
   }
  void on( const HChg & ) { ++changes; }
  void on( const HIns & ) { ++inserts; }
  void group_begin( void ) { ++groups; }
  void group_end( void ) { }
  };
 IndexedDaryHeap< int , 4 , Dynamic<> > h( 8 );
 Before b;
 b.h = &h;
 h.attach( b );
 h.push( 1 , 10 );
 h.push( 2 , 20 );
 h.push( 3 , 30 );
 h.erase( 2 );
 CHECK( ( b.seen == 1 ) && ( b.present == 1 ) && ( b.inserts == 3 ) );
 {
  auto t = batch( h );
  h.decrease( 3 , 5 );  // a change then an erasure: the
  h.pop();             // erasure of the first key
  h.push( 4 , 7 );      // an insertion then two changes:
  h.decrease( 4 , 6 );  // the insertion of the last key
  h.decrease( 4 , 2 );
  h.decrease( 1 , 9 );  // two changes: one
  h.increase( 1 , 12 );
  CHECK( b.seen == 1 );
  }
 CHECK( ( b.groups == 1 ) && ( b.seen == 2 ) && ( b.inserts == 4 ) &&
	( b.changes == 1 ) );
 CHECK( ( h.top().id == 4 ) && ( h.key( 1 ) == 12 ) );
 // a mute: Rebuilt to whoever wants it, nothing else
 struct Rb
 {
  int r = 0;
  void on( const mod::Rebuilt & ) noexcept { ++r; }
  };
 Rb rb;
 h.attach( rb );
 {
  auto q = quiet( h );
  h.push( 5 , 1 );
  h.pop();
  }
 CHECK( ( rb.r == 1 ) && ( b.inserts == 4 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE QUEUES WITH NO HANDLES ---------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// an observer of a queue with no handles, items int and keys K (and the
/// second keys V, if not void), that keeps from its events alone the
/// multiset of the pairs ( item , code ), the code of a pair its key, or
/// \f$ 1000 f + g \f$, and the events it received
template< class K , class V = void >
struct QueueMirror
{
 using Ins = mod::Inserted< int , K , V >;
 using Era = mod::Erased< int , K , V >;
 using Pairs = std::multiset< std::pair< int , long > >;
 Pairs pairs;                     ///< the pairs
 std::vector< std::string > log;  ///< the events
 bool missing = false;            ///< an erasure of a pair not there
 template< class E >
 static long code( const E & e ) {
  if constexpr( std::is_void_v< V > )
   return( long( e.key ) );
  else
   return( long( e.key ) * 1000 + long( e.value ) );
  }
 void on( const Ins & e ) {
  pairs.emplace( e.h , code( e ) );
  log.push_back( "+" + std::to_string( e.h ) + ":" +
		 std::to_string( code( e ) ) );
  }
 void on( const Era & e ) noexcept {
  const auto it = pairs.find( { e.h , code( e ) } );
  if( it == pairs.end() )
   missing = true;
  else
   pairs.erase( it );
  try {
   log.push_back( "-" + std::to_string( e.h ) + ":" +
		  std::to_string( code( e ) ) );
   }
  catch( ... ) {
   }
  }
 void on( const mod::Cleared & ) noexcept {
  pairs.clear();
  try {
   log.push_back( "C" );
   }
  catch( ... ) {
   }
  }
 /// as many pairs as the queue, none erased without being there
 template< class S >
 [[nodiscard]] bool audit( const S & q ) const {
  return( ! missing && ( pairs.size() == q.size() ) );
  }
 };

using Mirror1 = QueueMirror< int >;       ///< of a queue of one key
using Mirror2 = QueueMirror< int , int >;  ///< of a queue of two keys

/// the three queues with keys int and the subject policy P
template< class P >
using Radix = RadixHeap< int , int , 4 , std::allocator< int > , P >;
template< class P >
using Dial = DialQueue< int , int , P >;
template< class P >
using Bucket = BucketQueue< int , int , LargestSecondKey , P >;

/// random pushes and pops on q, the copies of an item allowed, with the
/// multiset of the pairs kept by hand; the keys pushed are never below
/// the last one extracted, as the monotone queues ask
template< class Q , class C >
void queueRun( Q & q , Mirror1::Pairs & ref , unsigned seed , int ops ,
	       C && check )
{
 std::mt19937 rng( seed );
 int last = 0;
 for( int t = 0 ; t < ops ; ++t ) {
  if( q.empty() || ( rng() % 3 != 0 ) ) {
   const int x = int( rng() % 20 ) , k = last + int( rng() % 200 );
   if constexpr( requires { q.topG(); } ) {
    const int g = int( rng() % 5 );
    q.push( x , k , g );
    ref.emplace( x , long( k ) * 1000 + g );
    }
   else {
    q.push( x , k );
    ref.emplace( x , long( k ) );
    }
   }
  else {
   long c;
   int x;
   if constexpr( requires { q.topG(); } ) {
    const auto e = q.popEntry();
    x = e.item;
    c = long( e.f ) * 1000 + e.g;
    last = e.f;
    }
   else {
    last = int( q.topKey() );
    c = last;
    x = q.pop();
    }
   const auto it = ref.find( { x , c } );
   CHECK( it != ref.end() );
   if( it != ref.end() )
    ref.erase( it );
   }
  check();
  }
 }

/// a static observer of the queue Q, checked by its audit and against the
/// multiset kept by hand after each operation; the same operations on the
/// queue with no observer give the same items
template< class Q , class M , class Plain >
void queueStatic( unsigned seed )
{
 M m;
 Q q( m );
 Mirror1::Pairs ref;
 queueRun( q , ref , seed , 3000 ,
	   [ & ] { CHECK( m.audit( q ) && ( m.pairs == ref ) ); } );
 Plain p;
 Mirror1::Pairs ref2;
 queueRun( p , ref2 , seed , 3000 , [] { } );
 CHECK( ref == ref2 );
 while( p.size() > 1 )
  CHECK( p.pop() == q.pop() );
 CHECK( ( q.size() == 1 ) && ( m.pairs.size() == 1 ) && ! m.missing );
 q.clear();
 CHECK( m.pairs.empty() && ( m.log.back() == "C" ) );
 }

}  // end( namespace )

ADS_TEST( "emit-queues" )
{
 // nobody listens: no storage and no test for the subject, the default
 // policy NoObservers
 static_assert( std::is_empty_v< RadixHeap< int >::Subject > &&
		std::is_empty_v< DialQueue< int >::Subject > &&
		std::is_empty_v< BucketQueue< int >::Subject > );
 static_assert( ! RadixHeap< int >::Subject::template listened<
		mod::Inserted< int , std::uint32_t > > );
 static_assert(
   std::is_same_v< Radix< NoObservers > , RadixHeap< int , int > > &&
   std::is_same_v< Dial< NoObservers > , DialQueue< int , int > > &&
   std::is_same_v< Bucket< NoObservers > , BucketQueue< int > > );
 queueStatic< Radix< Static< Mirror1 > > , Mirror1 ,
	      Radix< NoObservers > >( 3 );
 queueStatic< Bucket< Static< Mirror2 > > , Mirror2 , Bucket< NoObservers > >(
   5 );
 // the Dial queue, whose constructor takes the window first, with the
 // overflow in use
 Mirror1 m;
 Dial< Static< Mirror1 > > d( 15 , m );
 Mirror1::Pairs ref;
 queueRun( d , ref , 7 , 3000 ,
	   [ & ] { CHECK( m.audit( d ) && ( m.pairs == ref ) ); } );
 }

ADS_TEST( "emit-queues-records" )
{
 // the exact records of a short sequence, an erasure told before the
 // removal, and the items extracted by extract() and extractBelow()
 Mirror1 m;
 Radix< Static< Mirror1 > > r( m );
 r.push( 4 , 10 );
 r.push( 4 , 7 );  // a second copy of the item 4
 r.push( 2 , 12 );
 CHECK( r.pop() == 4 );
 std::vector< int > out;
 r.extract( 11 , out );
 r.extractBelow( 100 , []( int , int ) { } );
 CHECK( ( out == std::vector< int >{ 4 } ) && r.empty() );
 CHECK( ( m.log == std::vector< std::string >{ "+4:10" , "+4:7" , "+2:12" ,
					       "-4:7" , "-4:10" ,
					       "-2:12" } ) );
 Mirror2 m2;
 Bucket< Static< Mirror2 > > b( m2 );
 b.push( 1 , 3 , 2 );
 b.push( 2 , 3 , 4 );
 b.push( 3 , 1 , 0 );
 CHECK( ( b.pop() == 3 ) && ( b.popEntry().item == 2 ) );
 b.extract( 3 , out );
 CHECK( ( m2.log == std::vector< std::string >{ "+1:3002" , "+2:3004" ,
						"+3:1000" , "-3:1000" ,
						"-2:3004" , "-1:3002" } ) );
 }

ADS_TEST( "emit-queues-batch" )
{
 // attached at run time; a batch coalesces the copies of an item by their
 // keys, so that its net events rebuild the same multiset
 Dial< Dynamic<> > q( 31 );
 Mirror1 m , late;
 q.attach( m );
 q.push( 5 , 20 );  // a copy before the batch
 {
  auto t = batch( q );
  q.push( 5 , 40 );  // the copies of the item 5 of keys 20, 40 and 30
  q.push( 5 , 30 );
  q.push( 6 , 25 );
  CHECK( q.pop() == 5 );  // the copy of key 20, from before the batch
  CHECK( q.pop() == 6 );  // an insertion and its erasure cancel
  q.push( 7 , 35 );
  CHECK( m.log.size() == 1 );
  }
 // the erasure of 5:20 does not cancel the insertion of 5:40 or 5:30
 CHECK( ( m.pairs == Mirror1::Pairs{ { 5 , 40 } , { 5 , 30 } ,
				     { 7 , 35 } } ) );
 CHECK( ( m.log == std::vector< std::string >{ "+5:20" , "+5:40" , "+5:30" ,
					       "-5:20" , "+7:35" } ) );
 // a second observer, attached now, and a batch of a random run
 m.pairs.clear();
 q.clear();
 q.attach( late );
 Mirror1::Pairs ref;
 {
  auto t = batch( q );
  queueRun( q , ref , 13 , 500 , [] { } );
  }
 CHECK( ( m.pairs == ref ) && ( late.pairs == ref ) && ! m.missing &&
	! late.missing );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE ARRAY ------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "emit-array" )
{
 static_assert( sizeof( ObservableArray< int > ) ==
		sizeof( std::vector< int > ) );
 Arr a( 4 , 1 );
 ArrayLog L;
 a.attach( L );
 a.set( 2 , 5 );
 a.add( 2 , 3 );
 a.push_back( 9 );
 a.pop_back();
 a.clear();
 const std::vector< long long > v{ 1 , 2 , 3 };
 a.assign( v.begin() , v.end() );
 CHECK( L.ev.size() == 6 );
 CHECK( ( std::get< Arr::Assign >( L.ev[ 0 ] ) == Arr::Assign{ 2 , 1 , 5 } ) &&
	( std::get< Arr::Assign >( L.ev[ 1 ] ) == Arr::Assign{ 2 , 5 , 8 } ) &&
	( std::get< Arr::Insert >( L.ev[ 2 ] ) == Arr::Insert{ 4 , 4 , 9 } ) &&
	( std::get< Arr::Erase >( L.ev[ 3 ] ) == Arr::Erase{ 4 , 4 , 9 } ) &&
	std::holds_alternative< mod::Cleared >( L.ev[ 4 ] ) &&
	std::holds_alternative< mod::Rebuilt >( L.ev[ 5 ] ) );
 CHECK( ( a.size() == 3 ) && ( a[ 2 ] == 3 ) );
 // a batch: the net effect per index
 L.ev.clear();
 {
  auto b = batch( a );
  a.set( 0 , 7 );
  a.set( 0 , 8 );  // one change, 1 -> 8
  a.set( 1 , 4 );
  a.set( 1 , 2 );  // back to 2: nothing
  a.push_back( 5 );
  a.set( 3 , 6 );  // an insertion of 6
  a.set( 2 , 0 );
  a.pop_back();  // the insertion and erasure cancel
  a.pop_back();  // the erasure of 3, not of 0
  }
 CHECK( L.groups == 1 );
 CHECK( L.ev.size() == 2 );
 bool c0 = false , e2 = false;
 for( const auto & e : L.ev ) {
  if( const auto * x = std::get_if< Arr::Assign >( &e ) )
   c0 = ( *x == Arr::Assign{ 0 , 1 , 8 } );
  if( const auto * x = std::get_if< Arr::Erase >( &e ) )
   e2 = ( *x == Arr::Erase{ 2 , 2 , 3 } );
  }
 CHECK( c0 && e2 );
 CHECK( ( a.size() == 2 ) && ( a[ 0 ] == 8 ) && ( a[ 1 ] == 2 ) );
 // undone by an UndoLog
 ObservableArray< long long , Dynamic<> > u( 3 , 0 );
 UndoLog< Arr::modifications > log;
 u.attach( log );
 const std::size_t t = log.checkpoint();
 u.set( 1 , 4 );
 u.push_back( 6 );
 u.add( 0 , 2 );
 u.pop_back();
 log.rollback( u , t );
 CHECK( ( u.values() == std::vector< long long >{ 0 , 0 , 0 } ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- FENWICK ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "fenwick" )
{
 std::mt19937 rng( 17 );
 // a group: set by the difference, ranges
 {
  std::vector< long long > a( 37 );
  for( auto & x : a )
   x = ( long long )( rng() % 100 ) - 50;
  Fenwick< SumOf< long long > > f( a.begin() , a.end() );
  for( int t = 0 ; t < 3000 ; ++t ) {
   const std::size_t i = rng() % a.size();
   const int r = int( rng() % 5 );
   if( r == 0 ) {
    a[ i ] = ( long long )( rng() % 100 ) - 50;
    f.set( i , a[ i ] );
    }
   else if( r == 1 ) {
    a[ i ] += 3;
    f.add( i , 3 );
    }
   else if( r == 2 ) {
    a.push_back( ( long long )( rng() % 100 ) );
    f.push_back( a.back() );
    }
   else if( ( r == 3 ) && ( a.size() > 1 ) ) {
    a.pop_back();
    f.pop_back();
    }
   const std::size_t l = rng() % ( a.size() + 1 );
   const std::size_t h = l + rng() % ( a.size() - l + 1 );
   long long s = 0 , p = 0;
   for( std::size_t k = l ; k < h ; ++k )
    s += a[ k ];
   for( std::size_t k = 0 ; k < h ; ++k )
    p += a[ k ];
   CHECK( ( f.range( l , h ) == s ) && ( f.prefix( h ) == p ) );
   CHECK( f.get( i < a.size() ? i : 0 ) == a[ i < a.size() ? i : 0 ] );
   }
  }
 // a commutative monoid: add() by combination, set() by the parts
 {
  std::vector< int > a( 50 , 0 );
  Fenwick< MaxOf< int > > f( a.begin() , a.end() );
  for( int t = 0 ; t < 2000 ; ++t ) {
   const std::size_t i = rng() % a.size();
   const int v = int( rng() % 1000 );
   if( t % 2 ) {
    a[ i ] = std::max( a[ i ] , v );
    f.add( i , v );
    }
   else {
    a[ i ] = v;  // may lower it
    f.set( i , v );
    }
   const std::size_t r = rng() % ( a.size() + 1 );
   int m = MaxOf< int >::identity();
   for( std::size_t k = 0 ; k < r ; ++k )
    m = std::max( m , a[ k ] );
   CHECK( f.prefix( r ) == m );
   }
  }
 // a monoid that does not commute: the order of the combinations
 {
  Fenwick< Concat > f;
  std::string s;
  for( char c = 'a' ; c <= 'z' ; ++c ) {
   f.push_back( std::string( 1 , c ) );
   s += c;
   }
  for( std::size_t r = 0 ; r <= s.size() ; ++r )
   CHECK( f.prefix( r ) == s.substr( 0 , r ) );
  f.set( 9 , "JJ" );
  s.replace( 9 , 1 , "JJ" );
  for( std::size_t r = 0 ; r <= 26 ; ++r )
   CHECK( f.prefix( r ) == s.substr( 0 , r + ( r > 9 ? 1 : 0 ) ) );
  std::vector< std::string > w{ "x" , "y" , "z" };
  f.assign( w.begin() , w.end() );
  CHECK( ( f.prefix( 3 ) == "xyz" ) && ( f.size() == 3 ) );
  }
 // a group that does not commute: set() by the parts, ranges by inverses
 {
  std::vector< Affine::Value > a;
  for( int k = 0 ; k < 40 ; ++k )
   a.push_back(
     Affine::Value{ ( rng() % 2 ) ? 1 : -1 , ( long long )( rng() % 20 ) } );
  Fenwick< Affine > f( a.begin() , a.end() );
  for( int t = 0 ; t < 500 ; ++t ) {
   const std::size_t i = rng() % a.size();
   a[ i ] =
     Affine::Value{ ( rng() % 2 ) ? 1 : -1 , ( long long )( rng() % 20 ) };
   f.set( i , a[ i ] );
   const std::size_t l = rng() % ( a.size() + 1 );
   const std::size_t h = l + rng() % ( a.size() - l + 1 );
   Affine::Value x = Affine::identity();
   for( std::size_t k = l ; k < h ; ++k )
    x = Affine::combine( x , a[ k ] );
   CHECK( f.range( l , h ) == x );
   }
  }
 // floating point: the drift dropped at each period
 {
  Fenwick< SumOf< double > > f( 8 );
  f.period( 16 );
  for( int t = 0 ; t < 1000 ; ++t )
   f.set( std::size_t( t ) % 8 , 0.1 * double( t % 7 ) + 1e8 * ( t % 2 ) );
  for( std::size_t i = 0 ; i < 8 ; ++i )
   f.set( i , 0.1 );
  CHECK( f.period() == 16 );
  CHECK( std::fabs( f.prefix( 8 ) - 0.8 ) < 1e-6 );
  }
 }

ADS_TEST( "fenwick-observer" )
{
 // a static observer, given its array after the construction
 using FO = FenwickObserver< SumOf< long long > >;
 FO s;
 ObservableArray< long long , Static< FO > > b( 10 , 1 , s );
 s.rebuild( b );
 b.set( 3 , 5 );
 b.push_back( 2 );
 CHECK( ( s.prefix( 11 ) == 16 ) && ( s.range( 3 , 4 ) == 5 ) );
 // Shadow: the incremental observer against the recomputation, on random
 // changes, batches, clearings and rebuildings
 Arr a( 20 , 0 );
 FO inc( a );
 ScratchPrefix scr;
 scr.a = a.values();
 scr.src = &a.values();
 Shadow< FO , ScratchPrefix > sh( inc , scr );
 a.attach( sh );
 std::mt19937 rng( 23 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const int r = int( rng() % 20 );
  const std::size_t i = a.empty() ? 0 : rng() % a.size();
  if( ( r < 8 ) && ! a.empty() )
   a.set( i , ( long long )( rng() % 100 ) );
  else if( ( r < 11 ) && ! a.empty() )
   a.add( i , ( long long )( rng() % 10 ) );
  else if( r < 14 )
   a.push_back( ( long long )( rng() % 100 ) );
  else if( ( r < 16 ) && ! a.empty() )
   a.pop_back();
  else if( r < 18 ) {
   auto g = batch( a );
   for( int k = 0 ; k < 5 ; ++k ) {
    a.push_back( k );
    if( a.size() > 2 )
     a.set( a.size() - 2 , k );
    }
   if( rng() % 2 )
    a.pop_back();
   }
  else if( r == 18 ) {
   std::vector< long long > v( rng() % 30 , 4 );
   auto g = batch( a );
   a.push_back( 1 );
   a.assign( v.begin() , v.end() );  // Rebuilt, then more changes
   a.push_back( 6 );
   }
  else if( rng() % 4 == 0 )
   a.clear();
  const std::size_t q = rng() % ( a.size() + 1 );
  sh.query( [ q ]( auto & o ) { return( o.prefix( q ) ); } );
  CHECK( inc.audit( a ) );
  }
 CHECK( ( sh.counts().queries == 3000 ) && ( sh.counts().mismatches == 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- WINDOWS ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "sliding-aggregate" )
{
 static_assert( SlidingAggregate< SumOf< int > >::kind ==
		window_kind::subtract_on_evict );
 static_assert( SlidingAggregate< MinOf< int > >::kind ==
		window_kind::monotone_deque );
 static_assert( SlidingAggregate< Concat >::kind == window_kind::two_stacks );
 static_assert( SlidingAggregate< Concat , WorstCase >::kind ==
		window_kind::real_time );
 static_assert( Queue< SlidingAggregate< Concat > > );
 auto small = []( std::mt19937 & r ) { return( int( r() % 50 ) - 25 ); };
 auto letter = []( std::mt19937 & r ) {
  return( std::string( 1 , char( 'a' + r() % 26 ) ) );
  };
 auto affine = []( std::mt19937 & r ) {
  return( Affine::Value{ ( r() % 2 ) ? 1 : -1 , ( long long )( r() % 9 ) } );
  };
 auto first = []( std::mt19937 & r ) {
  return( First::Value{ int( r() % 9 ) } );
  };
 SlidingAggregate< SumOf< int > > w1;
 windowRun( w1 , small , 1 , 3000 );
 SlidingAggregate< MinOf< int > > w2;
 windowRun( w2 , small , 2 , 3000 );
 SlidingAggregate< MaxOf< int > > w3;
 windowRun( w3 , small , 3 , 3000 );
 SlidingAggregate< Concat > w4;
 windowRun( w4 , letter , 4 , 2000 );
 SlidingAggregate< Affine > w5;
 windowRun( w5 , affine , 5 , 3000 );
 SlidingAggregate< First > w6;
 windowRun( w6 , first , 6 , 3000 );
 SlidingAggregate< Concat , WorstCase > w7;
 windowRun( w7 , letter , 7 , 2000 );
 SlidingAggregate< MinOf< int > , WorstCase > w8;
 windowRun( w8 , small , 8 , 3000 );
 SlidingAggregate< Affine , WorstCase > w9;
 windowRun( w9 , affine , 9 , 3000 );
 // copies of a window in the middle of a rotation
 SlidingAggregate< Concat , WorstCase > c;
 for( int k = 0 ; k < 9 ; ++k )
  c.enqueue( std::string( 1 , char( 'a' + k ) ) );
 c.dequeue();
 c.enqueue( "z" );
 const auto d = c;
 CHECK( ( d.aggregate() == "bcdefghiz" ) && ( d.size() == 9 ) );
 c.clear();
 CHECK( c.empty() && ( c.aggregate() == "" ) );
 // floating point sums rebuilt at each period
 SlidingAggregate< SumOf< double > > f;
 f.period( 8 );
 for( int k = 0 ; k < 1000 ; ++k ) {
  f.enqueue( k % 2 ? 1e9 : 0.1 );
  if( f.size() > 4 )
   f.dequeue();
  }
 while( f.size() > 1 )
  f.dequeue();
 CHECK( ( f.period() == 8 ) &&
	( std::fabs( f.aggregate() - f.top() ) < 1e-4 ) );
 }

ADS_TEST( "sliding-minqueue" )
{
 // the same minima as the queues of exercise 3.15
 SlidingAggregate< MinOf< int > > a;
 SlidingAggregate< MinOf< int > , WorstCase > b;
 MinQueue< int > q;
 RealTimeMinQueue< int > r;
 std::mt19937 rng( 31 );
 for( int t = 0 ; t < 5000 ; ++t ) {
  if( q.empty() || ( rng() % 3 != 0 ) ) {
   const int x = int( rng() % 1000 );
   a.enqueue( x );
   b.enqueue( x );
   q.enqueue( x );
   r.enqueue( x );
   }
  else {
   const int x = q.dequeue();
   CHECK( ( a.dequeue() == x ) && ( b.dequeue() == x ) &&
	  ( r.dequeue() == x ) );
   }
  if( ! q.empty() )
   CHECK( ( a.aggregate() == q.min() ) && ( b.aggregate() == q.min() ) &&
	  ( r.min() == q.min() ) );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_emitters.cpp --------------------*/
/*--------------------------------------------------------------------------*/
