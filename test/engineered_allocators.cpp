/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_allocators.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the allocators of the containers, beyond the book: each
 * container that takes an allocator on a std::pmr::monotonic_buffer_resource,
 * on a counting allocator whose ledger must be empty when the containers
 * die, propagating or not on copies, moves and swaps, and on an
 * ArenaResource under a PoolAllocator, at the level 3 of the contracts; the
 * join trees on a stateless counting allocator. The paths of ADS of
 * FlatHashTable and SmallVector are the ones tested here, those of Boost in
 * engineered_boost.
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

#define ADS_FLATHASH_BOOST 0
#define ADS_SMALLVECTOR_BOOST 0

#include "ADSTest.h"

#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/CuckooHashing.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/PoolAllocator.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/sequence/RootishArrayStack.h"
#include "ADS/sequence/SmallVector.h"
#include "ADS/sequence/TieredVector.h"
#include "ADS/tree/BTreeMap.h"
#include "ADS/tree/JoinTree.h"

#include <cstddef>
#include <memory_resource>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/// the bytes given and not yet returned by the counting allocators of a
/// ledger, and the calls
struct Ledger
{
 long live = 0;   ///< the bytes not returned
 long calls = 0;  ///< the allocations
 };

/// the ledger of the copies of a container on an allocator that does not
/// propagate, which select_on_container_copy_construction() gives
Ledger & copies( void )
{
 static Ledger c;
 return( c );
 }

/// a stateful allocator that counts its bytes in a ledger, propagating on
/// copies, moves and swaps if P
template< class T , bool P >
struct Counting
{
 using value_type = T;
 using propagate_on_container_copy_assignment = std::bool_constant< P >;
 using propagate_on_container_move_assignment = std::bool_constant< P >;
 using propagate_on_container_swap = std::bool_constant< P >;
 using is_always_equal = std::false_type;

 template< class U >
 struct rebind
 {
  using other = Counting< U , P >;
  };

 explicit Counting( Ledger & l ) noexcept : f_l( &l ) { }

 template< class U >
 Counting( const Counting< U , P > & o ) noexcept : f_l( o.f_l ) { }

 T * allocate( std::size_t n ) {
  f_l->live += long( n * sizeof( T ) );
  ++f_l->calls;
  return( static_cast< T * >(
    ::operator new( n * sizeof( T ) , std::align_val_t( alignof( T ) ) ) ) );
  }

 void deallocate( T * p , std::size_t n ) noexcept {
  f_l->live -= long( n * sizeof( T ) );
  ::operator delete( p , std::align_val_t( alignof( T ) ) );
  }

 /// the same ledger if P, that of the copies otherwise
 Counting select_on_container_copy_construction( void ) const {
  return( P ? *this : Counting( copies() ) );
  }

 template< class U >
 bool operator==( const Counting< U , P > & o ) const noexcept {
  return( f_l == o.f_l );
  }

 Ledger * f_l;  ///< the ledger
 };

/// the ledger of the stateless counting allocator
Ledger & stateless( void )
{
 static Ledger s;
 return( s );
 }

/// a stateless allocator that counts its bytes in one ledger
template< class T >
struct Stateless
{
 using value_type = T;

 Stateless( void ) = default;

 template< class U >
 Stateless( const Stateless< U > & ) noexcept { }

 T * allocate( std::size_t n ) {
  stateless().live += long( n * sizeof( T ) );
  ++stateless().calls;
  return( std::allocator< T >().allocate( n ) );
  }

 void deallocate( T * p , std::size_t n ) noexcept {
  stateless().live -= long( n * sizeof( T ) );
  std::allocator< T >().deallocate( p , n );
  }

 template< class U >
 bool operator==( const Stateless< U > & ) const noexcept {
  return( true );
  }
 };

/// a resource above the monotonic buffers that counts what they take
struct Upstream : std::pmr::memory_resource
{
 long live = 0;  ///< the bytes not returned

 void * do_allocate( std::size_t b , std::size_t a ) override {
  live += long( b );
  return( std::pmr::new_delete_resource()->allocate( b , a ) );
  }

 void do_deallocate( void * p , std::size_t b , std::size_t a ) override {
  live -= long( b );
  std::pmr::new_delete_resource()->deallocate( p , b , a );
  }

 bool do_is_equal(
   const std::pmr::memory_resource & o ) const noexcept override {
  return( this == &o );
  }
 };

/*--------------------------------------------------------------------------*/
/*------------------------- THE CONTAINERS TESTED --------------------------*/
/*--------------------------------------------------------------------------*/

// a container is described by a struct with the type on the allocator Al,
// its construction on an allocator, the items put in, the check of them,
// and whether it is copied

constexpr int N = 3000;  ///< the items put in each container

/// the value of the key i
std::string val( int i )
{
 return( "value of a key that does not fit in a short string " +
	  std::to_string( i ) );
 }

/// a map of int to std::string with insert(), contains(), lookup()
template< template< class > class M >
struct MapSpec
{
 template< class Al >
 using type = M< Al >;
 static constexpr bool copied = true;
 template< class C >
 static void fill( C & c ) {
  for( int i = 0 ; i < N ; ++i )
   c.insert( i , val( i ) );
  for( int i = 0 ; i < N ; i += 3 )
   c.remove( i );
  }
 template< class C >
 static bool same( const C & c ) {
  if( c.size() != std::size_t( N - ( N + 2 ) / 3 ) )
   return( false );
  for( int i = 0 ; i < N ; ++i )
   if( c.contains( i ) != ( i % 3 != 0 ) ||
       ( ( i % 3 != 0 ) && ( *c.lookup( i ) != val( i ) ) ) )
    return( false );
  return( true );
  }
 };

template< class Al >
using Flat = FlatHashTable< int , std::string , DefaultHash< int > ,
			    std::equal_to< int > , Al >;
template< class Al >
using Dense = DenseHashMap< int , std::string , DefaultHash< int > ,
			    std::equal_to< int > , Al >;
template< class Al >
using Bucket = BucketCuckooTable< int , std::string , DefaultHash< int > ,
				  std::equal_to< int > , Al >;
template< class Al >
using Cuckoo = CuckooHashTable< int , std::string , std::hash< int > , Al >;
template< class Al >
using BTree = BTreeMap< int , std::string , std::less< int > , 256 , false ,
			Al >;

/// the maps, made on an allocator
struct Flats : MapSpec< Flat >
{
 template< class Al >
 static Flat< Al > make( const Al & a ) {
  return( Flat< Al >( 0 , {} , {} , a ) );
  }
 };
struct Denses : MapSpec< Dense >
{
 template< class Al >
 static Dense< Al > make( const Al & a ) {
  return( Dense< Al >( 0 , {} , {} , a ) );
  }
 };
struct Buckets : MapSpec< Bucket >
{
 template< class Al >
 static Bucket< Al > make( const Al & a ) {
  return( Bucket< Al >( 0 , {} , {} , a ) );
  }
 };
struct Cuckoos : MapSpec< Cuckoo >
{
 template< class Al >
 static Cuckoo< Al > make( const Al & a ) {
  return( Cuckoo< Al >( 16 , 0.4 , 4 , 1 , {} , a ) );
  }
 };
struct BTrees : MapSpec< BTree >
{
 template< class Al >
 static BTree< Al > make( const Al & a ) {
  return( BTree< Al >( a ) );
  }
 };

/// a heap of std::string by push() and pop(), the least first
template< template< class > class H >
struct HeapSpec
{
 template< class Al >
 using type = H< Al >;
 static constexpr bool copied = true;
 template< class C >
 static void fill( C & c ) {
  for( int i = N ; i-- > 0 ; )
   c.push( val( 100000 + i ) );
  }
 template< class C >
 static bool same( const C & c0 ) {
  C c( c0 );
  for( int i = 0 ; i < N ; ++i )
   if( pop( c ) != val( 100000 + i ) )
    return( false );
  return( c.size() == 0 );
  }
 template< class C >
 static std::string pop( C & c ) {
  if constexpr( requires { c.popMin(); } )
   return( c.popMin() );
  else
   return( c.pop() );
  }
 };

template< class Al >
using Dary = DaryHeap< std::string , std::less< std::string > , 4 , false ,
		       Al >;

struct Darys : HeapSpec< Dary >
{
 template< class Al >
 static Dary< Al > make( const Al & a ) {
  return( Dary< Al >( {} , a ) );
  }
 };

/// the radix heap of std::string by keys
template< class Al >
using Radix = RadixHeap< std::string , std::uint32_t , 4 , Al >;
struct Radixes
{
 template< class Al >
 using type = Radix< Al >;
 static constexpr bool copied = true;
 template< class Al >
 static Radix< Al > make( const Al & a ) {
  return( Radix< Al >( a ) );
  }
 template< class C >
 static void fill( C & c ) {
  for( int i = N ; i-- > 0 ; )
   c.push( val( i ) , std::uint32_t( i ) );
  }
 template< class C >
 static bool same( const C & c0 ) {
  C c( c0 );
  for( int i = 0 ; i < N ; ++i )
   if( c.pop() != val( i ) )
    return( false );
  return( c.size() == 0 );
  }
 };

/// the indexed heap of the ids by keys
template< class Al >
using Indexed = IndexedDaryHeap< double , 4 , NoObservers , Al >;
struct Indexeds
{
 template< class Al >
 using type = Indexed< Al >;
 static constexpr bool copied = true;
 template< class Al >
 static Indexed< Al > make( const Al & a ) {
  return( Indexed< Al >( std::allocator_arg , a , N ) );
  }
 template< class C >
 static void fill( C & c ) {
  for( int i = 0 ; i < N ; ++i )
   c.push( i , double( N - i ) );
  }
 template< class C >
 static bool same( const C & c ) {
  return( ( c.size() == std::size_t( N ) ) && ( c.min() == N - 1 ) );
  }
 };

/// a sequence of std::string by push_back() and operator[]
template< template< class > class S , bool Copied >
struct SeqSpec
{
 template< class Al >
 using type = S< Al >;
 static constexpr bool copied = Copied;
 template< class Al >
 static S< Al > make( const Al & a ) {
  return( S< Al >( a ) );
  }
 template< class C >
 static void fill( C & c ) {
  for( int i = 0 ; i < N ; ++i )
   c.push_back( val( i ) );
  }
 template< class C >
 static bool same( const C & c ) {
  if( c.size() != std::size_t( N ) )
   return( false );
  for( int i = 0 ; i < N ; ++i )
   if( c[ std::size_t( i ) ] != val( i ) )
    return( false );
  return( true );
  }
 };

template< class Al >
using Rootish = RootishArrayStack< std::string , Al >;
template< class Al >
using Tiered = TieredVector< std::string , Al >;
template< class Al >
using Small = SmallVector< std::string , 4 , Al >;

using Rootishes = SeqSpec< Rootish , true >;
using Tiereds = SeqSpec< Tiered , false >;
using Smalls = SeqSpec< Small , true >;

/// a heap with handles of int by priorities, never copied
template< template< class > class H >
struct HandleSpec
{
 template< class Al >
 using type = H< Al >;
 static constexpr bool copied = false;
 template< class Al >
 static H< Al > make( const Al & a ) {
  return( H< Al >( a ) );
  }
 template< class C >
 static void fill( C & c ) {
  c.insert( -1 , -1 );
  for( int i = 0 ; i < N ; ++i )
   c.insert( i , N - i );
  CHECK( c.deleteMin() == -1 );  // the least priority first
  }
 template< class C >
 static bool same( const C & c ) {
  return( c.size() == N );
  }
 };

template< class Al >
using Pairing = PairingHeap< int , int , false , Al >;
template< class Al >
using Fibonacci = FibonacciHeapPriorityQueue< int , int , Al >;

using Pairings = HandleSpec< Pairing >;
using Fibonaccis = HandleSpec< Fibonacci >;

/*--------------------------------------------------------------------------*/
/*------------------------------ THE CHECKS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the container of S on a monotonic buffer: its memory from the buffer,
/// its copy on the allocator `alloc` the same buffer
template< class S >
void monotonic( void )
{
 Upstream up;
 {
  std::pmr::monotonic_buffer_resource m( &up );
  using Al = std::pmr::polymorphic_allocator< std::byte >;
  auto c = S::make( Al( &m ) );
  S::fill( c );
  CHECK( S::same( c ) );
  CHECK( up.live > 0 );
  CHECK( c.get_allocator().resource() == &m );
  using C = typename S::template type< Al >;
  if constexpr( std::is_constructible_v< C , const C & , const Al & > ) {
   C d( c , Al( &m ) );
   CHECK( S::same( d ) );
   CHECK( d.get_allocator().resource() == &m );
   }
  auto e = std::move( c );
  CHECK( S::same( e ) );
  }
 CHECK( up.live == 0 );
 }

/// the container of S on counting allocators that do not propagate: copies
/// on the ledger of the copies, assignments that keep the allocator of the
/// target, moves between unequal allocators item by item, every byte back
template< class S >
void counting( void )
{
 using Al = Counting< std::byte , false >;
 using C = typename S::template type< Al >;
 Ledger L , M;
 const long c0 = copies().live;
 {
  C a = S::make( Al( L ) );
  S::fill( a );
  CHECK( ( L.live > 0 ) && S::same( a ) );
  if constexpr( S::copied ) {
   C b( a );  // select_on_container_copy_construction()
   CHECK( S::same( b ) && ( b.get_allocator() == Al( copies() ) ) );
   C c = S::make( Al( M ) );
   c = a;  // the allocator of c stays
   CHECK( S::same( c ) && ( c.get_allocator() == Al( M ) ) );
   c = std::move( b );  // unequal: the items moved one by one
   CHECK( S::same( c ) && ( c.get_allocator() == Al( M ) ) );
   if constexpr( std::is_constructible_v< C , const C & , const Al & > ) {
    C d( a , Al( M ) );
    CHECK( S::same( d ) && ( d.get_allocator() == Al( M ) ) );
    C e( std::move( d ) , Al( L ) );  // unequal: item by item
    CHECK( S::same( e ) && ( e.get_allocator() == Al( L ) ) );
    C f = S::make( Al( L ) );
    f = std::move( e );  // equal: the memory of e
    CHECK( S::same( f ) );
    }
   }
  C g( std::move( a ) );
  CHECK( S::same( g ) && ( g.get_allocator() == Al( L ) ) );
  }
 CHECK( ( L.live == 0 ) && ( M.live == 0 ) && ( copies().live == c0 ) );
 }

/// the container of S on counting allocators that propagate: assignments
/// and swaps take the allocator of the source, every byte back
template< class S >
void propagating( void )
{
 using Al = Counting< std::byte , true >;
 using C = typename S::template type< Al >;
 Ledger L , M;
 {
  C a = S::make( Al( L ) );
  S::fill( a );
  if constexpr( S::copied ) {
   C b( a );
   CHECK( S::same( b ) && ( b.get_allocator() == Al( L ) ) );
   C c = S::make( Al( M ) );
   S::fill( c );
   c = a;
   CHECK( S::same( c ) && ( c.get_allocator() == Al( L ) ) );
   C d = S::make( Al( M ) );
   d = std::move( b );
   CHECK( S::same( d ) && ( d.get_allocator() == Al( L ) ) );
   C e = S::make( Al( M ) );
   using std::swap;
   if constexpr( requires { e.swap( d ); } )
    e.swap( d );
   else
    swap( e , d );
   CHECK( S::same( e ) && ( e.get_allocator() == Al( L ) ) &&
	  ( d.get_allocator() == Al( M ) ) );
   }
  CHECK( S::same( a ) );
  }
 CHECK( ( L.live == 0 ) && ( M.live == 0 ) );
 }

/// the container of S on an ArenaResource under a PoolAllocator: its memory
/// from the blocks of the arena
template< class S >
void arena( void )
{
 ArenaResource<> r;
 using Al = PoolAllocator< std::byte , ArenaResource<> >;
 {
  auto c = S::make( Al( r ) );
  S::fill( c );
  CHECK( S::same( c ) && ( r.bytes() > 0 ) && ( r.size() > 0 ) );
  if constexpr( S::copied ) {
   typename S::template type< Al > d( c );
   CHECK( S::same( d ) && ( d.get_allocator() == Al( r ) ) );
   }
  }
 CHECK( r.bytes() > 0 );  // given back with the resource only
 }

/// all the checks of S
template< class S >
void all( void )
{
 monotonic< S >();
 counting< S >();
 propagating< S >();
 arena< S >();
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "alloc-flat-hash" )
{
 all< Flats >();
 }

ADS_TEST( "alloc-dense-hash" )
{
 all< Denses >();
 }

ADS_TEST( "alloc-cuckoo" )
{
 all< Buckets >();
 all< Cuckoos >();
 }

ADS_TEST( "alloc-btree-map" )
{
 all< BTrees >();
 }

ADS_TEST( "alloc-heaps" )
{
 all< Darys >();
 all< Radixes >();
 all< Indexeds >();
 }

ADS_TEST( "alloc-node-heaps" )
{
 all< Pairings >();
 all< Fibonaccis >();
 }

ADS_TEST( "alloc-sequences" )
{
 all< Rootishes >();
 all< Tiereds >();
 all< Smalls >();
 }

ADS_TEST( "alloc-join-trees" )
{
 // the join trees take only a stateless allocator, all of whose bytes come
 // back when the trees die
 const long s0 = stateless().live;
 {
  JoinTree< int , std::string , AVLBalance , NoAugmentation , NoObservers ,
	    Stateless< int > >
    t;
  WeightBalancedTree< int , std::string , NoAugmentation , NoObservers ,
		      Stateless< int > >
    w;
  for( int i = 0 ; i < N ; ++i ) {
   t.insert( i , val( i ) );
   w.insert( N - i , val( i ) );
   }
  auto u = t;
  CHECK( ( u.size() == std::size_t( N ) ) && ( stateless().live > s0 ) );
  for( int i = 0 ; i < N ; i += 2 )
   u.remove( i );
  CHECK( u.size() == std::size_t( N / 2 ) );
  JoinTree< int , std::string , AVLBalance , NoAugmentation , NoObservers ,
	    HugePageAllocator< int > >
    h;
  h.insert( 1 , val( 1 ) );
  CHECK( h.contains( 1 ) );
  }
 CHECK( stateless().live == s0 );
 }

ADS_TEST( "alloc-arena-resource" )
{
 // the requests of the arena aligned as asked, a large one in a block of
 // its own, all the blocks given back by release()
 Upstream up;
 {
  using Al = std::pmr::polymorphic_allocator< std::max_align_t >;
  ArenaResource< Al > r( 1024 , Al( &up ) );
  for( std::size_t a = 1 ; a <= 64 ; a *= 2 ) {
   void * p = r.allocate( 3 * a , a );
   CHECK( reinterpret_cast< std::uintptr_t >( p ) % a == 0 );
   }
  void * big = r.allocate( 5000 , 16 );
  CHECK( ( big != nullptr ) && ( r.bytes() >= 5000 ) && ( up.live > 0 ) );
  r.release();
  CHECK( ( r.bytes() == 0 ) && ( r.size() == 0 ) && ( up.live == 0 ) );
  CHECK( r.allocate( 8 ) != nullptr );
  }
 CHECK( up.live == 0 );
 }

ADS_TEST( "alloc-swap-unequal" )
{
 // a swap of two containers whose allocators are unequal and do not
 // propagate violates its contract: it throws, both containers as they
 // were, and it is noexcept only where it cannot violate it, as for the
 // containers of the standard
 using No = Counting< std::byte , false >;
 using Yes = Counting< std::byte , true >;
 const auto check = []< class S >( S ) {
  using C = typename S::template type< No >;
  static_assert(
    ! noexcept( std::declval< C & >().swap( std::declval< C & >() ) ) );
  static_assert(
    noexcept( std::declval< typename S::template type< Yes > & >().swap(
      std::declval< typename S::template type< Yes > & >() ) ) );
  Ledger L , M;
  {
   C a = S::make( No( L ) );
   S::fill( a );
   C b = S::make( No( M ) );
   CHECK_THROWS( a.swap( b ) , contract_violation );
   CHECK( S::same( a ) && ( a.get_allocator() == No( L ) ) );
   CHECK( ( b.size() == 0 ) && ( b.get_allocator() == No( M ) ) );
   C c = S::make( No( L ) );
   c.swap( a );  // equal: no check fails
   CHECK( S::same( c ) && ( a.size() == 0 ) );
   }
  CHECK( ( L.live == 0 ) && ( M.live == 0 ) );
  };
 check( Flats() );
 check( Denses() );
 check( Rootishes() );
 check( BTrees() );
 static_assert( noexcept( std::declval< FlatHashTable< int , int > & >().swap(
   std::declval< FlatHashTable< int , int > & >() ) ) );
 }

ADS_TEST( "alloc-default-layout" )
{
 // the default allocators take no room: the layouts are those of the
 // containers before they took an allocator
 CHECK( sizeof( FlatHashTable< int , int > ) == 10 * sizeof( void * ) );
 CHECK( sizeof( SmallVector< int , 8 > ) ==
	3 * sizeof( void * ) + 8 * sizeof( int ) );
 CHECK( sizeof( DaryHeap< int > ) == 4 * sizeof( void * ) );
 CHECK( sizeof( RootishArrayStack< int > ) == 4 * sizeof( void * ) );
 CHECK( ( std::is_same_v< FlatHashTable< int , int >::allocator_type ,
			  std::allocator< std::pair< const int , int > > > ));
 CHECK( ( std::is_same_v< PairingHeap< int >::allocator_type ,
			  HugePageAllocator< int > > ));
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_allocators.cpp ------------------*/
/*--------------------------------------------------------------------------*/
