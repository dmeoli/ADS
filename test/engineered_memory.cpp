/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_memory.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered allocators, beyond the book: the size classes of
 * PoolResource and of ConcurrentPoolResource at their limits, PoolAllocator
 * in the node containers of the standard library and in
 * std::allocate_shared against the same containers on std::allocator, the
 * propagation of the allocators by copies, moves and swaps, the frees
 * from other threads, and the epochs of Arena and ObjectPool, by one
 * thread and by the workers of a search, at the level 3 of the contracts.
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

#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/ConcurrentPool.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/memory/PoolAllocator.h"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// an item that counts the live ones, to find leaks and double frees
struct Counted {
 static inline std::atomic< long > alive{ 0 };
 std::string s;
 explicit Counted( std::string v = "" ) : s( std::move( v ) ) { ++alive; }
 Counted( const Counted & o ) : s( o.s ) { ++alive; }
 Counted( Counted && o ) noexcept : s( std::move( o.s ) ) { ++alive; }
 Counted & operator=( const Counted & ) = default;
 Counted & operator=( Counted && ) = default;
 ~Counted() { --alive; }
 bool operator==( const Counted & o ) const { return( s == o.s ); }
 };

/// a message of a few words, as a change of a model
struct Message {
 std::uint64_t id;
 std::uint64_t check;
 double data[ 4 ];
 explicit Message( std::uint64_t i ) : id( i ) , check( ~i ) ,
                                       data{ 1 , 2 , 3 , 4 } {}
 [[nodiscard]] bool intact( void ) const { return( check == ~id ); }
 };

/// an object aligned more than a class
struct alignas( 64 ) Wide {
 std::uint64_t x[ 8 ];
 };

/// true if p is aligned to a
bool aligned( const void * p , std::size_t a )
{
 return( reinterpret_cast< std::uintptr_t >( p ) % a == 0 );
 }

/// the requests of b bytes from the resource R, written whole and freed in
/// a shuffled order, twice; their addresses distinct and aligned
template< class R >
void requests( R & r , std::size_t b , std::size_t a )
{
 std::mt19937_64 g( b );
 for( int round = 0 ; round < 2 ; ++round ) {
  std::vector< unsigned char * > P;
  for( int i = 0 ; i < 300 ; ++i ) {
   auto * p = static_cast< unsigned char * >( r.allocate( b , a ) );
   CHECK( aligned( p , a ) );
   std::fill( p , p + b , static_cast< unsigned char >( i ) );
   P.push_back( p );
   }
  for( int i = 0 ; i < 300 ; ++i )
   CHECK( std::all_of( P[ i ] , P[ i ] + b , [ i ]( unsigned char c ) {
           return( c == static_cast< unsigned char >( i ) ); } ) );
  CHECK( std::set< unsigned char * >( P.begin() , P.end() ).size() ==
         P.size() );
  std::shuffle( P.begin() , P.end() , g );
  for( auto * p : P )
   r.deallocate( p , b , a );
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// PoolResource: every size from 1 to past the largest class, at the
// alignments 1 to 64, by the runtime and the compile time requests; the
// slot freed last given first, the counts of the objects, the memory kept
// after the frees and taken again, the contracts
ADS_TEST( "pool-resource" )
{
 PoolResource<> r;
 CHECK( r.size() == 0 && r.bytes() == 0 );
 CHECK( PoolResource<>::pooled( 1 , 1 ) && PoolResource<>::pooled( 256 , 16 )
        && ! PoolResource<>::pooled( 257 , 8 ) &&
        ! PoolResource<>::pooled( 16 , 32 ) &&
        ! PoolResource<>::pooled( 0 , 8 ) );
 for( std::size_t b : { 1 , 2 , 15 , 16 , 17 , 31 , 32 , 33 , 100 , 255 ,
                        256 , 257 , 1000 , 5000 } )
  for( std::size_t a : { 1 , 8 , 16 , 32 , 64 } )
   requests( r , b , a );
 CHECK( r.size() == 0 );
 const std::size_t kept = r.bytes();
 CHECK( kept > 0 );
 void * p = r.allocate< 24 , 16 >();       // aligned to 16, as max_align_t
 r.deallocate< 24 , 16 >( p );              // is not everywhere
 void * q = r.allocate< 32 , 16 >();        // the same class, the same slot
 CHECK( q == p && r.size() == 1 );
 r.deallocate< 32 , 16 >( q );
 void * big = r.allocate< 4096 >();         // operator new
 CHECK( r.size() == 0 && r.bytes() == kept );
 r.deallocate< 4096 >( big );
 void * w = r.allocate< 64 , 64 >();
 CHECK( aligned( w , 64 ) );
 r.deallocate< 64 , 64 >( w );
 void * z = r.allocate( 0 );                // no class: operator new
 r.deallocate( z , 0 );
 CHECK( r.bytes() == kept );                // nothing given back
 CHECK_THROWS( r.deallocate( nullptr , 8 ) , contract_violation );
 }

// PoolAllocator in std::list, std::map, std::unordered_map and std::vector
// against the same containers on std::allocator, on random operations; the
// traits of the standard, the rebinding, the equality by the resource
ADS_TEST( "pool-allocator-containers" )
{
 using A = PoolAllocator< int >;
 using T = std::allocator_traits< A >;
 static_assert( T::propagate_on_container_copy_assignment::value &&
                T::propagate_on_container_move_assignment::value &&
                T::propagate_on_container_swap::value &&
                ! T::is_always_equal::value );
 static_assert( std::is_same_v< T::rebind_alloc< double > ,
                                PoolAllocator< double > > );
 static_assert( ! std::is_default_constructible_v< A > );
 PoolResource<> r , s;
 CHECK( A( r ) == PoolAllocator< double >( r ) && A( r ) != A( s ) );
 CHECK( PoolAllocator< char >( A( r ) ).resource() == &r );
 {
  std::mt19937_64 g( 7 );
  std::list< Counted , PoolAllocator< Counted > > L{ PoolAllocator<
                                                     Counted >( r ) };
  std::list< Counted > M;
  std::map< int , Counted , std::less< int > ,
            PoolAllocator< std::pair< const int , Counted > > >
   P{ PoolAllocator< std::pair< const int , Counted > >( r ) };
  std::map< int , Counted > Q;
  std::unordered_map< int , int , std::hash< int > , std::equal_to< int > ,
                      PoolAllocator< std::pair< const int , int > > >
   U( 0 , std::hash< int >() , std::equal_to< int >() ,
      PoolAllocator< std::pair< const int , int > >( r ) );
  std::unordered_map< int , int > V;
  std::vector< int , PoolAllocator< int > > X{ PoolAllocator< int >( r ) };
  std::vector< int > Y;
  for( int t = 0 ; t < 100000 ; ++t ) {
   const int k = int( g() % 2000 );
   const std::string v = std::to_string( g() % 100 );
   switch( g() % 6 ) {
    case 0: L.emplace_back( v ); M.emplace_back( v ); break;
    case 1: if( ! M.empty() ) { L.pop_front(); M.pop_front(); } break;
    case 2: P.insert_or_assign( k , Counted( v ) );
            Q.insert_or_assign( k , Counted( v ) ); break;
    case 3: P.erase( k ); Q.erase( k ); break;
    case 4: U[ k ] = int( t ); V[ k ] = int( t ); break;
    default: U.erase( k ); V.erase( k );
             X.push_back( k ); Y.push_back( k ); break;
    }
   }
  CHECK( std::ranges::equal( L , M ) );
  CHECK( std::ranges::equal( P , Q ) );
  CHECK( U.size() == V.size() );
  for( const auto & [ k , v ] : V )
   CHECK( U.at( k ) == v );
  CHECK( std::ranges::equal( X , Y ) );
  CHECK( r.size() > 0 && s.size() == 0 );
  }
 CHECK( r.size() == 0 && Counted::alive == 0 );
 }

// the propagation: a container copied, moved and swapped across two
// resources gives each node back to the pool it came from
ADS_TEST( "pool-allocator-propagation" )
{
 PoolResource<> r , s;
 using A = PoolAllocator< Counted >;
 std::size_t h = 0;                         // the slots of an empty list,
 {                                          // its sentinel on MSVC
  const std::list< Counted , A > E{ A( r ) };
  h = r.size();
  }
 {
  std::list< Counted , A > L{ A( r ) } , M{ A( s ) };
  for( int i = 0 ; i < 100 ; ++i ) {
   L.emplace_back( std::to_string( i ) );
   M.emplace_back( std::to_string( -i ) );
   }
  CHECK( r.size() == 100 + h && s.size() == 100 + h );
  M = L;                                    // the copy takes r
  CHECK( M.get_allocator().resource() == &r && s.size() == 0 &&
         r.size() == 200 + 2 * h && std::ranges::equal( L , M ) );
  std::list< Counted , A > N{ A( s ) };
  N.emplace_back( "x" );
  N = std::move( L );                       // the move takes r
  CHECK( N.get_allocator().resource() == &r && s.size() == 0 &&
         r.size() == 200 + 3 * h && N.size() == 100 );
  std::list< Counted , A > O{ A( s ) };
  O.emplace_back( "y" );
  O.swap( N );                              // the swap swaps them
  CHECK( O.get_allocator().resource() == &r &&
         N.get_allocator().resource() == &s && N.size() == 1 &&
         s.size() == 1 + h );
  std::list< Counted , A > C( O );          // the copy keeps r
  CHECK( C.get_allocator().resource() == &r && r.size() == 300 + 4 * h );
  }
 CHECK( r.size() == 0 && s.size() == 0 && Counted::alive == 0 );
 }

// std::allocate_shared: the control block and the object in one slot of
// a class, the weak pointers that keep the block after the object, the
// objects aligned beyond a class on operator new, the empty shared
// pointers
ADS_TEST( "pool-allocate-shared" )
{
 PoolResource<> r;
 {
  std::vector< std::shared_ptr< Message > > S;
  for( std::uint64_t i = 0 ; i < 10000 ; ++i )
   S.push_back( std::allocate_shared< Message >(
                 PoolAllocator< Message >( r ) , i ) );
  CHECK( r.size() == 10000 );                // one slot each
  std::weak_ptr< Message > w = S[ 5 ];
  std::mt19937_64 g( 3 );
  std::shuffle( S.begin() , S.end() , g );
  S.resize( 5000 );
  CHECK( std::all_of( S.begin() , S.end() , []( const auto & p ) {
          return( p->intact() ); } ) );
  S.clear();
  CHECK( w.expired() && r.size() == 1 );    // the block of w
  auto x = std::allocate_shared< Wide >( PoolAllocator< Wide >( r ) );
  CHECK( aligned( x.get() , 64 ) );
  auto c = std::allocate_shared< Counted >( PoolAllocator< Counted >( r ) ,
                                            "c" );
  CHECK( c->s == "c" && Counted::alive == 1 );
  }
 CHECK( r.size() == 0 && Counted::alive == 0 );
 }

// ConcurrentPoolResource on one thread: the classes at their limits, the
// slot freed last given first, the memory of the segments, collect() with
// nothing to collect, two resources apart
ADS_TEST( "concurrent-pool" )
{
 ConcurrentPoolResource<> r , s;
 CHECK( r.heaps() == 0 && r.bytes() == 0 );
 for( std::size_t b : { 1 , 15 , 16 , 17 , 128 , 255 , 256 , 257 , 3000 } )
  for( std::size_t a : { 1 , 16 , 64 } )
   requests( r , b , a );
 CHECK( r.heaps() == 1 && r.bytes() == ConcurrentPoolResource<>::SEGMENT &&
        s.heaps() == 0 );
 void * p = r.allocate< 40 , 16 >();       // aligned to 16, as max_align_t
 r.deallocate< 40 , 16 >( p );              // is not everywhere
 CHECK( ( r.allocate< 48 , 16 >() == p ) );
 r.deallocate< 48 , 16 >( p );
 r.collect();
 CHECK( ( r.allocate< 33 , 16 >() == p ) );
 void * q = s.allocate< 33 >();
 CHECK( q != p && s.heaps() == 1 );
 s.deallocate< 33 >( q );
 r.deallocate< 33 , 16 >( p );
 // more pages than a segment holds, all written
 std::vector< void * > P;
 for( int i = 0 ; i < 20000 ; ++i ) {
  P.push_back( r.allocate< 256 >() );
  std::memset( P.back() , i & 0xFF , 256 );
  }
 CHECK( r.bytes() > ConcurrentPoolResource<>::SEGMENT );
 for( void * x : P )
  r.deallocate< 256 >( x );
 CHECK_THROWS( r.deallocate( nullptr , 8 ) , contract_violation );
 }

// the objects made by producer threads and freed by consumer threads,
// shared pointers through queues of one producer and one consumer: each
// message intact when it is freed, every slot given back (the producers
// reuse the returns, so that the memory stays bounded over the rounds),
// and the heap of a thread that ended taken by the frees of the others
ADS_TEST( "concurrent-pool-threads" )
{
 ConcurrentPoolResource<> r;
 using A = PoolAllocator< Message , ConcurrentPoolResource<> >;
 constexpr int PAIRS = 3 , N = 30000 , ROUNDS = 3;
 std::atomic< long > bad{ 0 } , freed{ 0 };
 std::size_t after_first = 0;
 for( int round = 0 ; round < ROUNDS ; ++round ) {
  std::vector< std::unique_ptr< SPSCQueue< std::shared_ptr< Message > > > >
   Q;
  for( int k = 0 ; k < PAIRS ; ++k )
   Q.push_back( std::make_unique< SPSCQueue< std::shared_ptr< Message > > >(
                 1024 ) );
  std::vector< std::thread > T;
  for( int k = 0 ; k < PAIRS ; ++k ) {
   T.emplace_back( [ & , k ]() {
    for( std::uint64_t i = 0 ; i < N ; ++i ) {
     auto m = std::allocate_shared< Message >( A( r ) , i );
     while( ! Q[ k ]->push( m ) )
      std::this_thread::yield();
     }
    while( ! Q[ k ]->push( nullptr ) )      // the end
     std::this_thread::yield();
    } );
   T.emplace_back( [ & , k ]() {
    for( std::uint64_t i = 0 ; ; ) {
     std::shared_ptr< Message > m;
     if( ! Q[ k ]->tryPop( m ) ) {
      std::this_thread::yield();
      continue;
      }
     if( ! m )
      break;
     if( ! m->intact() || ( m->id != i++ ) )
      ++bad;
     m.reset();                             // the free of another thread
     ++freed;
     }
    } );
   }
  for( auto & t : T )
   t.join();
  if( round == 0 )
   after_first = r.bytes();
  }
 CHECK( bad == 0 && freed == long( PAIRS ) * N * ROUNDS );
 // the memory of the later rounds: at most a segment more per heap
 CHECK( r.bytes() <= after_first +
        r.heaps() * ConcurrentPoolResource<>::SEGMENT );
 // the objects of a thread that ended, freed here
 std::vector< void * > P;
 std::thread( [ & ]() {
  for( int i = 0 ; i < 1000 ; ++i )
   P.push_back( r.allocate< 64 >() );
  } ).join();
 for( void * p : P )
  r.deallocate< 64 >( p );
 // a shared pointer whose last copy dies in another thread, by the
 // default allocator on the resource of the program
 using G = PoolAllocator< Counted , ConcurrentPoolResource<> >;
 static_assert( std::is_default_constructible_v< G > );
 auto c = std::allocate_shared< Counted >( G() , "global" );
 std::thread( [ c = std::move( c ) ]() mutable { c.reset(); } ).join();
 CHECK( Counted::alive == 0 );
 std::list< int , PoolAllocator< int , ConcurrentPoolResource<> > > L;
 for( int i = 0 ; i < 1000 ; ++i )
  L.push_back( i );
 std::thread( [ & ]() { L.clear(); } ).join();
 CHECK( L.empty() );
 }

// Arena::clear(): the objects of an epoch destroyed at once, the blocks
// kept and given again in their order, so that the places of the new
// epoch are those of the old one and the memory does not grow; fresh() of
// the pointers of the old epoch, the iterators of the new one, a
// constructor that throws after a clear(), and the places of the old epoch
// forbidden to the sanitizer of addresses until they are given again
ADS_TEST( "arena-epochs" )
{
 Arena< long > E( 4 );
 E.clear();                                 // no block: nothing to do
 CHECK( E.size() == 0 && E.bytes() == 0 );
 std::vector< long * > P;
 for( long i = 0 ; i < 10 ; ++i )
  P.push_back( E.make( i ) );
 const std::size_t b = E.bytes();
 CHECK( b == 3 * 4 * sizeof( long ) );
 for( long * p : P )
  CHECK( E.fresh( p ) );
 for( int e = 1 ; e <= 5 ; ++e ) {
  E.clear();
  CHECK( E.size() == 0 && E.begin() == E.end() && E.bytes() == b );
  for( long * p : P )
   CHECK( ! E.fresh( p ) );
#if ADS_ASAN
  for( long * p : P )
   CHECK( __asan_address_is_poisoned( p ) );
#endif
  for( long i = 0 ; i < 10 ; ++i ) {
   CHECK( E.make( 100 * e + i ) == P[ i ] );
   CHECK( E.fresh( P[ i ] ) );
   CHECK( ( i == 9 ) || ! E.fresh( P[ i + 1 ] ) );
   }
  CHECK( E.bytes() == b && E.size() == 10 );
  long s = 0;
  for( long x : E )
   s += x;
  CHECK( s == 1000 * e + 45 );
  }
 for( long i = 0 ; i < 3 ; ++i )            // past the blocks kept
  E.make( i );
 CHECK( E.bytes() == b + 4 * sizeof( long ) && E.size() == 13 );
 CHECK( ! E.fresh( nullptr ) );
 // the objects with a destructor: destroyed by clear()
 {
  Arena< Counted > C( 3 );
  for( int i = 0 ; i < 8 ; ++i )
   C.make( std::string( 40 , char( 'a' + i ) ) );
  CHECK( Counted::alive == 8 );
  C.clear();
  CHECK( Counted::alive == 0 && C.size() == 0 );
  for( int i = 0 ; i < 4 ; ++i )
   C.make( "again" );
  CHECK( Counted::alive == 4 );
  }
 CHECK( Counted::alive == 0 );
 // a constructor that throws in a block kept: the arena as before it
 struct Thrower {
  long x;
  explicit Thrower( long v ) : x( v ) {
   if( v < 0 )
    throw std::runtime_error( "no" );
   }
  };
 Arena< Thrower > T( 2 );
 for( long i = 0 ; i < 5 ; ++i )
  T.make( i );
 T.clear();
 T.make( 1L );
 T.make( 2L );
 CHECK_THROWS( T.make( -1L ) , std::runtime_error );
 CHECK( T.size() == 2 );
 const Thrower * t = T.make( 3L );
 CHECK( t->x == 3 && T.size() == 3 && T.fresh( t ) );
 }

// ObjectPool::clear(): the objects and the free slots of an epoch dropped
// at once, the blocks kept and given again from the last one down before a
// new block is taken, so that a pool cleared at each restart keeps the
// memory of its largest epoch; fresh() and the audit of free() on a stale
// pointer, adopt() while blocks kept wait to be used again, the moves and
// the swaps with blocks kept, and the slots dropped forbidden to the
// sanitizer of addresses
ADS_TEST( "object-pool-epochs" )
{
 using Pool = ObjectPool< long , std::allocator< long > >;
 Pool Q( 8 );
 Q.clear();
 CHECK( Q.size() == 0 && Q.bytes() == 0 );
 std::vector< long * > P;
 for( long i = 0 ; i < 23 ; ++i )          // blocks of 1, 2, 4, 8 and 8
  P.push_back( Q.make( i ) );
 const std::size_t b = Q.bytes();
 for( int i = 0 ; i < 23 ; i += 2 )
  Q.free( P[ i ] );
 for( int e = 1 ; e <= 4 ; ++e ) {
  Q.clear();
  CHECK( Q.size() == 0 && Q.bytes() == b );
  for( long * p : P )
   CHECK( ! Q.fresh( p ) );
#if ADS_ASAN
  for( long * p : P )
   CHECK( __asan_address_is_poisoned( p ) );
#endif
  // a stale pointer whose slot has not been given again: the audit
  std::set< long * > S{ Q.make( 0L ) };
  CHECK_THROWS( Q.free( P[ 0 ] ) , contract_violation );
  CHECK( Q.size() == 1 );
  for( long i = 1 ; i < 23 ; ++i ) {
   long * p = Q.make( i );
   CHECK( Q.fresh( p ) );
   S.insert( p );
   }
  CHECK( S == std::set< long * >( P.begin() , P.end() ) );
  CHECK( Q.bytes() == b && Q.size() == 23 );
  // the frees of the epoch, and the slots taken again
  std::vector< long * > F( S.begin() , S.end() );
  for( int i = 0 ; i < 10 ; ++i )
   Q.free( F[ i ] );
  for( int i = 0 ; i < 10 ; ++i )
   Q.make( 0L );
  CHECK( Q.bytes() == b );
  }
 Q.make( 1L );                              // a new block, past the kept
 CHECK( Q.bytes() > b && Q.size() == 24 );
 // adopt() while blocks kept wait: the objects of the other pool alive
 // and never overwritten by the slots given again
 Pool A( 4 ) , B( 4 );
 for( long i = 0 ; i < 15 ; ++i )
  A.make( i );
 A.clear();
 A.make( -1L );                             // blocks kept still waiting
 std::vector< long * > Bp;
 for( long i = 0 ; i < 9 ; ++i )
  Bp.push_back( B.make( 1000 + i ) );
 A.adopt( B );
 CHECK( A.size() == 10 && B.size() == 0 && B.bytes() == 0 );
 for( long i = 0 ; i < 30 ; ++i )
  *A.make( i ) = -7;
 for( long i = 0 ; i < 9 ; ++i )
  CHECK( *Bp[ i ] == 1000 + i && A.fresh( Bp[ i ] ) );
 for( long * p : Bp )
  A.free( p );
 A.clear();
 const std::size_t a = A.bytes();
 for( long i = 0 ; i < 42 ; ++i )           // all the slots, the adopted
  A.make( i );                              // ones too: 15 + 11 + 16
 CHECK( A.bytes() == a );
 A.make( 0L );
 CHECK( A.bytes() > a );
 // the moves and the swaps carry the blocks kept
 Pool M( 4 );
 for( long i = 0 ; i < 15 ; ++i )
  M.make( i );
 M.clear();
 Pool N( std::move( M ) );
 CHECK( M.bytes() == 0 && N.size() == 0 );
 const std::size_t n = N.bytes();
 for( long i = 0 ; i < 15 ; ++i )
  N.make( i );
 CHECK( N.bytes() == n );
 N.clear();
 Pool O( 4 );
 O.swap( N );
 for( long i = 0 ; i < 15 ; ++i )
  O.make( i );
 CHECK( O.bytes() == n && N.bytes() == 0 );
 }

// the epochs of the pools of the workers of a parallel search: each of 4
// threads owns an Arena and an ObjectPool, makes nodes whose parents are
// nodes of the other workers, reads the nodes of the others after a
// barrier, gives back to their owners some nodes of the others (the
// deferred return: each node by one worker, the owner freeing it after
// the next barrier) and clears its pools after the last barrier of the
// epoch; the nodes intact, the memory of the later epochs that of the
// first one, and no race (the test runs under the sanitizer of threads)
ADS_TEST( "pool-epochs-threads" )
{
 struct Node {
  int owner , epoch;
  long value;
  const Node * parent;
  };
 constexpr int P = 4 , EPOCHS = 12 , M = 3000;
 std::vector< std::vector< Node * > > pub( P ) , pooled( P );
 // ret[ o ][ w ]: the nodes of the owner o that the worker w gives back
 std::vector< std::vector< std::vector< Node * > > > ret(
  P , std::vector< std::vector< Node * > >( P ) );
 std::vector< std::size_t > bytes( P , 0 );
 std::atomic< long > bad{ 0 };
 std::barrier sync( P );
 auto work = [ & ]( int w ) {
  Arena< Node > ar( 256 );
  ObjectPool< Node > pool( 512 );
  for( int e = 0 ; e < EPOCHS ; ++e ) {
   // phase 1: the own nodes, published
   pub[ w ].clear();
   for( int i = 0 ; i < M ; ++i )
    pub[ w ].push_back( ar.make( Node{ w , e , long( w ) * M + i ,
                                       nullptr } ) );
   sync.arrive_and_wait();
   // phase 2: the nodes of the others read, and pooled nodes whose
   // parents are theirs; a third of the pooled nodes freed by the owner,
   // a third of those of the others given back to their owner
   pooled[ w ].clear();
   for( int o = 0 ; o < P ; ++o )
    for( int i = w ; i < M ; i += 7 ) {
     const Node * x = pub[ o ][ i ];
     if( ( x->owner != o ) || ( x->epoch != e ) ||
         ( x->value != long( o ) * M + i ) || x->parent )
      ++bad;
     Node * y = pool.make( Node{ w , e , x->value , x } );
     if( i % 3 == 0 )
      pool.free( y );
     else
      pooled[ w ].push_back( y );
     }
   sync.arrive_and_wait();
   for( int o = 0 ; o < P ; ++o )
    if( o != w ) {
     for( std::size_t i = std::size_t( w ) ; i < pooled[ o ].size() ;
          i += 2 * P ) {
      const Node * y = pooled[ o ][ i ];
      if( ( y->owner != o ) || ( y->epoch != e ) ||
          ( y->parent->value != y->value ) )
       ++bad;
      ret[ o ][ w ].push_back( pooled[ o ][ i ] );
      }
     }
   sync.arrive_and_wait();
   // phase 3: the deferred returns freed by their owner, then the epoch
   // closed; nobody reads a node of w after the barrier above
   for( int x = 0 ; x < P ; ++x ) {
    for( Node * y : ret[ w ][ x ] ) {
     if( ! pool.fresh( y ) )
      ++bad;
     pool.free( y );
     }
    ret[ w ][ x ].clear();
    }
   ar.clear();
   pool.clear();
   if( ( ar.size() != 0 ) || ( pool.size() != 0 ) ||
       ar.fresh( pub[ w ].front() ) )
    ++bad;
   if( e == 0 )
    bytes[ w ] = ar.bytes() + pool.bytes();
   else if( ar.bytes() + pool.bytes() != bytes[ w ] )
    ++bad;
   sync.arrive_and_wait();
   }
  };
 std::vector< std::thread > T;
 for( int w = 0 ; w < P ; ++w )
  T.emplace_back( work , w );
 for( auto & t : T )
  t.join();
 CHECK( bad == 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_memory.cpp --------------------*/
/*--------------------------------------------------------------------------*/
