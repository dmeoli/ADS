/*--------------------------------------------------------------------------*/
/*---------------------------- File runtime.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the runtime of the long-lived workers on t threads (-t,
 * 4 by default), the workers started by run() of each executor compiled in
 * (the pool of ADS, "pool", and those of OpenMP, oneTBB and FastFlow):
 * - case "all-to-all b<b>": each worker sends n messages of 64 bits to the
 *   others in turn, in batches of b (1 and 64), draining its inbox between
 *   two batches and at the end until it has received its messages, per
 *   message of all the workers; the families of channels of Channels.h
 *   (the inboxes under the lock of the executor, "locked", the stacks of
 *   batches, "inbox", the lanes, "lanes", and the mesh of 1024 places per
 *   pair, "mesh") against the mesh of the SWSR_Ptr_Buffer of FastFlow,
 *   the queues of its farms and all-to-all ("ff-mesh", a pointer per
 *   message, as FastFlow passes them, when FastFlow is compiled in);
 * - case "round-trip": two workers send a message back and forth n times
 *   through a Mailbox with batches of 1, per round trip;
 * - case "tree": a random tree of about n nodes (2 to 4 children per node,
 *   down to the depth of 3^depth >= n / 3) explored by the workers over a
 *   Mailbox with batches of 64, each node sent to the worker that owns its
 *   key, until the detection of the termination, per node of the tree;
 * - case "end": the time of a run of t workers that start with no work
 *   and call quiescent() until they see the termination, in ns, and case
 *   "run", the time of a run of t workers that return at once, which the
 *   first includes.
 * Each cell is the median of r runs, after a run not timed; the first
 * structure of each case runs again as "A/A". The benchmark of a cell is
 * "runtime t<t>".
 *
 *   runtime [ -r runs ] [ -s seed ] [ -t threads ] n1 n2 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/parallel/Channels.h"
#include "ADS/parallel/FastFlowExecutor.h"
#include "ADS/parallel/Mailbox.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"

#if defined( ADS_WITH_FASTFLOW )
 #include <ff/buffer.hpp>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <span>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

using namespace ADS;
using namespace ADS::bench;

int threads = 4;                            ///< -t
std::string bname;                          ///< "runtime t<t>"

#if defined( ADS_WITH_FASTFLOW )
/// the mesh of FastFlow: a SWSR_Ptr_Buffer per pair, a pointer per
/// message (the message plus 1, never null)
class FFMesh {
 public:
 using Message = std::uint64_t;
 static constexpr bool bounded = true;
 explicit FFMesh( std::size_t p , std::size_t c = 1024 ) : f_p( p ) {
  for( std::size_t i = 0 ; i < p * p ; ++i ) {
   f_q.push_back( std::make_unique< ff::SWSR_Ptr_Buffer >( c ) );
   f_q.back()->init();
   }
  }
 [[nodiscard]] std::size_t size( void ) const { return( f_p ); }
 std::size_t tryPush( std::size_t from , std::size_t to ,
                      std::span< const std::uint64_t > m ) {
  auto & q = *f_q[ to * f_p + from ];
  std::size_t k = 0;
  while( ( k < m.size() ) &&
         q.push( reinterpret_cast< void * >( m[ k ] + 1 ) ) )
   ++k;
  return( k );
  }
 template< class F >
 std::size_t drain( std::size_t me , F && f ) {
  std::size_t n = 0;
  for( std::size_t from = 0 ; from < f_p ; ++from ) {
   auto & q = *f_q[ me * f_p + from ];
   void * x;
   while( q.pop( &x ) ) {
    f( std::uint64_t( reinterpret_cast< std::uintptr_t >( x ) ) - 1 );
    ++n;
    }
   }
  return( n );
  }
 private:
 std::size_t f_p;
 std::vector< std::unique_ptr< ff::SWSR_Ptr_Buffer > > f_q;
 };
#endif

/// the all-to-all of n messages per worker in batches of b on the
/// channels C, on the workers of ex
template< class C , class E >
void allToAll( E & ex , const std::string & who , std::size_t n ,
               std::size_t b , int r )
{
 if( skip( who ) )
  return;
 const std::size_t p = std::size_t( threads );
 // the batch j of the worker q goes to the j-th of the others in turn
 auto dest = [ p ]( std::size_t q , std::size_t j ) {
  return( p == 1 ? q : ( q + 1 + j % ( p - 1 ) ) % p );
  };
 std::vector< std::uint64_t > expect( p , 0 );
 for( std::size_t q = 0 ; q < p ; ++q )
  for( std::size_t k = 0 , j = 0 ; k < n ; k += b , ++j )
   expect[ dest( q , j ) ] += std::min( b , n - k );
 auto run = [ & ]( C & c ) {
  ex.run( p , [ & ]( std::size_t me ) {
   std::vector< std::uint64_t > batch( b );
   std::uint64_t got = 0 , sum = 0;
   auto take = [ & ]( std::uint64_t && x ) { ++got; sum += x; };
   for( std::size_t k = 0 , j = 0 ; k < n ; k += b , ++j ) {
    const std::size_t to = dest( me , j );
    const std::size_t m = std::min( b , n - k );
    for( std::size_t j = 0 ; j < m ; ++j )
     batch[ j ] = k + j;
    std::size_t sent = 0;
    while( sent < m ) {
     sent += c.tryPush( me , to , std::span< const std::uint64_t >(
                                   batch.data() + sent , m - sent ) );
     c.drain( me , take );
     }
    }
   while( got < expect[ me ] )
    c.drain( me , take );
   sink = sink + sum;
   } );
  };
 print( bname.c_str() , "all-to-all b" + std::to_string( b ) , n , who ,
        median( [ & ] { return( std::make_unique< C >( p ) ); } ,
                [ & ]( auto & c ) { run( *c ); } , r , n * p ) );
 }

/// n round trips of a message between two workers of ex, through a
/// mailbox of batches of 1 on the channels C
template< class C , class E >
void roundTrip( E & ex , const std::string & who , std::size_t n , int r )
{
 if( skip( who ) || ( threads < 2 ) )
  return;
 print( bname.c_str() , "round-trip" , n , who , median(
  [ & ] { return( std::make_unique< Mailbox< std::uint64_t , C > >( 2 ,
                                                                      1 ) );
          } ,
  [ & ]( auto & mb ) {
   ex.run( 2 , [ & ]( std::size_t me ) {
    std::uint64_t seen = 0;
    if( me == 0 )
     mb->send( 0 , 1 , 0 );
    while( seen < n ) {
     mb->poll( me , [ & ]( std::uint64_t && x ) {
      ++seen;
      if( x + 1 < 2 * n )
       mb->send( me , 1 - me , x + 1 );
      } );
     }
    } );
   } , r , n ) );
 }

/// the mixing of 64 bits of SplitMix64
std::uint64_t mix( std::uint64_t x )
{
 x += 0x9E3779B97F4A7C15ull;
 x = ( x ^ ( x >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
 x = ( x ^ ( x >> 27 ) ) * 0x94D049BB133111EBull;
 return( x ^ ( x >> 31 ) );
 }

/// the depth of the random tree of about n nodes, 3 children per node
std::uint32_t depthOf( std::size_t n )
{
 std::uint32_t d = 0;
 for( std::size_t m = 1 ; m * 3 < n ; m *= 3 )
  ++d;
 return( d );
 }

/// a node of the random tree: its key and its depth
struct Node {
 std::uint64_t key;
 std::uint32_t depth;
 };

/// the random tree of about n nodes explored by the workers of ex over a
/// mailbox of batches of 64 on the channels C: 2 to 4 children per node
/// above the depth, so about 3^depth nodes
template< class C , class E >
void tree( E & ex , const std::string & who , std::size_t n , int r ,
           unsigned seed )
{
 if( skip( who ) )
  return;
 const std::size_t p = std::size_t( threads );
 const std::uint32_t d = depthOf( n );
 // the children of a node: 2 to 4 above the depth
 auto children = [ d ]( const Node & v , auto && f ) {
  if( v.depth >= d )
   return;
  const std::uint64_t h = mix( v.key );
  const unsigned c = 2 + unsigned( h % 3 );
  for( unsigned k = 0 ; k < c ; ++k )
   f( Node{ mix( h + k ) , v.depth + 1 } );
  };
 // the nodes of the tree, counted on one thread
 std::uint64_t size = 0;
 {
  std::vector< Node > stack{ Node{ seed , 0 } };
  while( ! stack.empty() ) {
   const Node v = stack.back();
   stack.pop_back();
   ++size;
   children( v , [ & ]( Node w ) { stack.push_back( w ); } );
   }
  }
 print( bname.c_str() , "tree" , n , who , median(
  [ & ] { return( std::make_unique< Mailbox< Node , C > >( p , 64 ) ); } ,
  [ & ]( auto & mb ) {
   ex.run( p , [ & ]( std::size_t me ) {
    std::vector< Node > stack;
    if( ownerOf( std::uint64_t( seed ) , p ) == me )
     stack.push_back( Node{ seed , 0 } );
    for( ;; ) {
     mb->poll( me , [ & ]( Node && v ) { stack.push_back( v ); } );
     if( ! stack.empty() ) {
      for( int j = 0 ; ( j < 64 ) && ! stack.empty() ; ++j ) {
       const Node v = stack.back();
       stack.pop_back();
       children( v , [ & ]( Node w ) {
        const std::size_t o = ownerOf( w.key , p );
        if( o == me )
         stack.push_back( w );
        else
         mb->send( me , o , w );
        } );
       }
      continue;
      }
     if( mb->quiescent( me ) )
      break;
     }
    } );
   } , r , std::size_t( size ) ) );
 }

/// the latency of the detection: the time from the start of the workers,
/// none with work, to the moment the last one sees the termination, and
/// the time of a run of workers that return at once
template< class C , class E >
void endLatency( E & ex , const std::string & who , int r )
{
 if( skip( who ) )
  return;
 const std::size_t p = std::size_t( threads );
 print( bname.c_str() , "run" , p , who , median(
  [ & ] { return( 0 ); } ,
  [ & ]( int ) { ex.run( p , []( std::size_t ) {} ); } , r , 1 ) );
 print( bname.c_str() , "end" , p , who , median(
  [ & ] { return( std::make_unique< Mailbox< std::uint64_t , C > >( p ) );
          } ,
  [ & ]( auto & mb ) {
   ex.run( p , [ & ]( std::size_t me ) {
    while( ! mb->quiescent( me ) )
     ;
    } );
   } , r , 1 ) );
 }

/// the families of channels, for any type of messages
template< class L >
struct LockedOf {
 template< class M > using type = LockedChannels< M , L >;
 };
struct InboxOf {
 template< class M > using type = InboxChannels< M >;
 };
struct LanesOf {
 template< class M > using type = LaneChannels< M >;
 };
struct MeshOf {
 template< class M > using type = MeshChannels< M >;
 };

/// all the cases of the family F on the executor ex, as who
template< class F , class E >
void cases( E & ex , const std::string & who , std::size_t n , int r ,
            unsigned seed )
{
 using C = typename F::template type< std::uint64_t >;
 for( std::size_t b : { 1 , 64 } )
  allToAll< C >( ex , who , n , b , r );
 roundTrip< C >( ex , who , std::min< std::size_t >( n , 100000 ) , r );
 tree< typename F::template type< Node > >( ex , who , n , r , seed );
 endLatency< C >( ex , who , r );
 }

/// the families on the executor ex, the first one again as "A/A" when
/// first is true
template< class E >
void families( E & ex , const std::string & name , std::size_t n , int r ,
               unsigned seed , bool first )
{
 using L = typename E::lock_type;
 cases< LockedOf< L > >( ex , name + " locked" , n , r , seed );
 if( first )
  cases< LockedOf< L > >( ex , "A/A" , n , r , seed );
 cases< LockedOf< std::mutex > >( ex , name + " locked-std" , n , r , seed );
 cases< InboxOf >( ex , name + " inbox" , n , r , seed );
 cases< LanesOf >( ex , name + " lanes" , n , r , seed );
 cases< MeshOf >( ex , name + " mesh" , n , r , seed );
#if defined( ADS_WITH_FASTFLOW )
 for( std::size_t b : { 1 , 64 } )
  allToAll< FFMesh >( ex , name + " ff-mesh" , n , b , r );
#endif
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 std::vector< char * > rest{ argv[ 0 ] };
 for( int i = 1 ; i < argc ; ++i )
  if( ( std::string( argv[ i ] ) == "-t" ) && ( i + 1 < argc ) )
   threads = std::max( std::atoi( argv[ ++i ] ) , 1 );
  else
   rest.push_back( argv[ i ] );
 Options o( int( rest.size() ) , rest.data() , { 1000000 } );
 bname = "runtime t" + std::to_string( threads );
 const std::size_t t = std::size_t( threads );
 for( std::size_t n : o.sizes ) {
  ThreadPool pool( t );
  families( pool , "pool" , n , o.runs , o.seed , true );
#if defined( _OPENMP )
  OpenMPExecutor omp( t );
  families( omp , "omp" , n , o.runs , o.seed , false );
#endif
#if defined( ADS_WITH_TBB )
  TBBExecutor tbb( t );
  families( tbb , "tbb" , n , o.runs , o.seed , false );
#endif
#if defined( ADS_WITH_FASTFLOW )
  FastFlowExecutor ff( t , false );
  families( ff , "ff" , n , o.runs , o.seed , false );
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File runtime.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
