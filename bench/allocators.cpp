/*--------------------------------------------------------------------------*/
/*--------------------------- File allocators.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the standard allocators in the node containers and in
 * std::allocate_shared: PoolAllocator of ADS on PoolResource and on
 * ConcurrentPoolResource against std::allocator (glibc's malloc),
 * std::pmr::polymorphic_allocator on unsynchronized_pool_resource and, if
 * Boost is found, boost::fast_pool_allocator (without its mutex), and
 * against mi_stl_allocator of mimalloc when ADS_HAS_MIMALLOC is defined
 * (MIMALLOC in the makefile, mimalloc built without the override of
 * malloc). The resources live across the runs of a cell, as the heap of
 * malloc does. The cases, on n items:
 *
 * - list, map and unordered_map: n items inserted (random keys of 64 bits
 *   for the maps), all erased in a random order, n inserted again and the
 *   container destroyed, which frees the nodes in a scrambled order and
 *   then takes them again;
 * - shared_ptr: n messages of 48 bytes by std::allocate_shared, the
 *   control block and the message in one allocation, released in a random
 *   order, made again and released;
 * - cross-thread: n messages of 48 bytes made by std::allocate_shared in a
 *   thread and sent through a queue of 4096 places to a second thread,
 *   which releases them, so that each is freed by a thread other than the
 *   one that made it: the thread-safe allocators only, i.e., ADS on
 *   ConcurrentPoolResource, std::allocator, mimalloc, boost on its mutex
 *   and std::pmr::synchronized_pool_resource. The two threads run on the
 *   cores that taskset allows, the same one if it allows one.
 *
 * Each cell is the median of r runs, in nanoseconds per item and per pass
 * (the four passes of a container counted as one item each, i.e., the time
 * over 4 n; the two of the cross-thread case as one); the contracts are
 * off (ADS_CONTRACTS=0).
 *
 *   allocators [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/memory/ConcurrentPool.h"
#include "ADS/memory/PoolAllocator.h"

#if __has_include( <boost/pool/pool_alloc.hpp> )
 #include <boost/pool/pool_alloc.hpp>
 #define ADS_HAS_BOOST 1
#endif
#ifdef ADS_HAS_MIMALLOC
 #include <mimalloc.h>
#endif

#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <thread>
#include <unordered_map>

using namespace ADS::bench;

namespace {

/// a message of 48 bytes, as a change of a model
struct Message {
 std::uint64_t id;
 std::uint64_t data[ 5 ];
 explicit Message( std::uint64_t i ) : id( i ) , data{} {}
 };

/// the keys of the maps, and a random order of 0 .. n - 1
std::vector< std::uint64_t > K;
std::vector< std::uint32_t > order;

/// the four passes of a container C built by make()
template< class C , class M , class Insert >
double passes( int r , std::size_t n , M && make , Insert && insert )
{
 return( median( [](){ return( 0 ); } , [ & ]( int ) {
  C c = make();
  using It = typename C::iterator;
  std::vector< It > at( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   at[ i ] = insert( c , i );
  for( std::size_t i = 0 ; i < n ; ++i )
   c.erase( at[ order[ i ] ] );
  for( std::size_t i = 0 ; i < n ; ++i )
   insert( c , i );
  sink = sink + c.size(); } , r , 4 * n ) );
 }

/// the cases on one thread for the allocator a of the given name
template< class A >
void cases( const std::string & who , std::size_t n , int r , const A & a )
{
 if( skip( who ) )
  return;
 using T = std::allocator_traits< A >;
 using LA = typename T::template rebind_alloc< std::uint64_t >;
 using MA = typename T::template rebind_alloc<
             std::pair< const std::uint64_t , std::uint64_t > >;
 using L = std::list< std::uint64_t , LA >;
 using M = std::map< std::uint64_t , std::uint64_t ,
                     std::less< std::uint64_t > , MA >;
 using U = std::unordered_map< std::uint64_t , std::uint64_t ,
                               std::hash< std::uint64_t > ,
                               std::equal_to< std::uint64_t > , MA >;
 print( "allocators" , "list" , n , who , passes< L >( r , n ,
         [ & ](){ return( L( LA( a ) ) ); } ,
         []( L & c , std::size_t i ) {
          return( c.insert( c.end() , i ) ); } ) );
 print( "allocators" , "map" , n , who , passes< M >( r , n ,
         [ & ](){ return( M( MA( a ) ) ); } ,
         []( M & c , std::size_t i ) {
          return( c.emplace( K[ i ] , i ).first ); } ) );
 print( "allocators" , "unordered_map" , n , who , passes< U >( r , n ,
         [ & ](){ return( U( 0 , std::hash< std::uint64_t >() ,
                             std::equal_to< std::uint64_t >() , MA( a ) ) );
         } ,
         []( U & c , std::size_t i ) {
          return( c.emplace( K[ i ] , i ).first ); } ) );
 using SA = typename T::template rebind_alloc< Message >;
 print( "allocators" , "shared_ptr" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          std::vector< std::shared_ptr< Message > > S( n );
          for( std::size_t i = 0 ; i < n ; ++i )
           S[ i ] = std::allocate_shared< Message >( SA( a ) , i );
          for( std::size_t i = 0 ; i < n ; ++i )
           S[ order[ i ] ].reset();
          for( std::size_t i = 0 ; i < n ; ++i )
           S[ i ] = std::allocate_shared< Message >( SA( a ) , i );
          std::uint64_t t = 0;
          for( const auto & p : S )
           t += p->id;
          S.clear();
          sink = sink + t; } , r , 4 * n ) );
 }

/// the case cross-thread for the allocator a of the given name
template< class A >
void crossThread( const std::string & who , std::size_t n , int r ,
                  const A & a )
{
 if( skip( who ) )
  return;
 using SA = typename std::allocator_traits< A >::template rebind_alloc<
             Message >;
 print( "allocators" , "cross-thread" , n , who ,
        median( [](){ return( 0 ); } , [ & ]( int ) {
         ADS::SPSCQueue< std::shared_ptr< Message > > Q( 4096 );
         std::thread consumer( [ & ]() {
          std::uint64_t t = 0;
          for( std::size_t i = 0 ; i < n ; ) {
           std::shared_ptr< Message > m;
           if( Q.tryPop( m ) ) {
            t += m->id;
            m.reset();                      // the free of another thread
            ++i;
            }
           else
            std::this_thread::yield();
           }
          sink = sink + t; } );
         for( std::size_t i = 0 ; i < n ; ++i ) {
          auto m = std::allocate_shared< Message >( SA( a ) , i );
          while( ! Q.push( std::move( m ) ) )
           std::this_thread::yield();
          }
         consumer.join(); } , r , n ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  K = keys( n , o.seed );
  order.resize( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   order[ i ] = std::uint32_t( i );
  std::mt19937_64 g( o.seed );
  std::shuffle( order.begin() , order.end() , g );

  for( const char * who : { "std::allocator" , "std::allocator A/A" } )
   cases( who , n , r , std::allocator< std::uint64_t >() );
  {
   std::pmr::unsynchronized_pool_resource m;
   cases( "std::pmr::unsynchronized_pool_resource" , n , r ,
          std::pmr::polymorphic_allocator< std::uint64_t >( &m ) );
   }
#ifdef ADS_HAS_BOOST
  cases( "boost::fast_pool_allocator" , n , r ,
         boost::fast_pool_allocator< std::uint64_t ,
          boost::default_user_allocator_new_delete ,
          boost::details::pool::null_mutex >() );
#endif
#ifdef ADS_HAS_MIMALLOC
  cases( "mimalloc" , n , r , mi_stl_allocator< std::uint64_t >() );
#endif
  {
   ADS::PoolResource<> p;
   cases( "ADS::PoolAllocator" , n , r ,
          ADS::PoolAllocator< std::uint64_t >( p ) );
   }
  {
   ADS::ConcurrentPoolResource<> p;
   cases( "ADS::PoolAllocator concurrent" , n , r ,
          ADS::PoolAllocator< std::uint64_t ,
                              ADS::ConcurrentPoolResource<> >( p ) );
   }

  // the frees from another thread

  for( const char * who : { "std::allocator" , "std::allocator A/A" } )
   crossThread( who , n , r , std::allocator< std::uint64_t >() );
  {
   std::pmr::synchronized_pool_resource m;
   crossThread( "std::pmr::synchronized_pool_resource" , n , r ,
                std::pmr::polymorphic_allocator< std::uint64_t >( &m ) );
   }
#ifdef ADS_HAS_BOOST
  crossThread( "boost::fast_pool_allocator mutex" , n , r ,
               boost::fast_pool_allocator< std::uint64_t >() );
#endif
#ifdef ADS_HAS_MIMALLOC
  crossThread( "mimalloc" , n , r , mi_stl_allocator< std::uint64_t >() );
#endif
  {
   ADS::ConcurrentPoolResource<> p;
   crossThread( "ADS::PoolAllocator concurrent" , n , r ,
                ADS::PoolAllocator< std::uint64_t ,
                                    ADS::ConcurrentPoolResource<> >( p ) );
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File allocators.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
