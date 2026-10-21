/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_memory.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered allocators, beyond the book: the size classes of
 * PoolResource and of ConcurrentPoolResource at their limits, PoolAllocator
 * in the node containers of the standard library and in
 * std::allocate_shared against the same containers on std::allocator, the
 * propagation of the allocators by copies, moves and swaps, and the frees
 * from other threads, at the level 3 of the contracts.
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

#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/memory/ConcurrentPool.h"
#include "ADS/memory/PoolAllocator.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <set>
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
 void * p = r.allocate< 24 >();
 r.deallocate< 24 >( p );
 void * q = r.allocate< 32 >();             // the same class, the same slot
 CHECK( q == p && r.size() == 1 );
 r.deallocate< 32 >( q );
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
 {
  std::list< Counted , A > L{ A( r ) } , M{ A( s ) };
  for( int i = 0 ; i < 100 ; ++i ) {
   L.emplace_back( std::to_string( i ) );
   M.emplace_back( std::to_string( -i ) );
   }
  CHECK( r.size() == 100 && s.size() == 100 );
  M = L;                                    // the copy takes r
  CHECK( M.get_allocator().resource() == &r && s.size() == 0 &&
         r.size() == 200 && std::ranges::equal( L , M ) );
  std::list< Counted , A > N{ A( s ) };
  N.emplace_back( "x" );
  N = std::move( L );                       // the move takes r
  CHECK( N.get_allocator().resource() == &r && s.size() == 0 &&
         r.size() == 200 && N.size() == 100 );
  std::list< Counted , A > O{ A( s ) };
  O.emplace_back( "y" );
  O.swap( N );                              // the swap swaps them
  CHECK( O.get_allocator().resource() == &r &&
         N.get_allocator().resource() == &s && N.size() == 1 &&
         s.size() == 1 );
  std::list< Counted , A > C( O );          // the copy keeps r
  CHECK( C.get_allocator().resource() == &r && r.size() == 300 );
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
 void * p = r.allocate< 40 >();
 r.deallocate< 40 >( p );
 CHECK( r.allocate< 48 >() == p );
 r.deallocate< 48 >( p );
 r.collect();
 CHECK( r.allocate< 33 >() == p );
 void * q = s.allocate< 33 >();
 CHECK( q != p && s.heaps() == 1 );
 s.deallocate< 33 >( q );
 r.deallocate< 33 >( p );
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
