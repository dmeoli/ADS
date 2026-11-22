/*--------------------------------------------------------------------------*/
/*----------------------- File interop_parallel.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the executors on the runtimes of a program, beyond the book:
 * `TBBExecutor` on an arena of the program and on the current arena, with
 * the threads of each call within the arena; `OpenMPExecutor` called inside
 * a region of the program, whose calls are tasks of its team (more than one
 * thread runs them); the executors of `ADS/interop/parallel/` (the
 * parallel algorithms of the standard library, the senders of stdexec,
 * Taskflow, Kokkos) on a sort, a scan, a reduction and the fork and join of
 * a recursion, against the results of `Sequential`, with the calls nested
 * in a call run inline where the runtime does not nest; and the concurrent
 * structures of ADS driven by the loops of oneTBB and by the tasks of
 * OpenMP. A test whose runtime is not compiled in passes with nothing to
 * check, and says so. The parallel tests of the library run on the same
 * executors too [see InteropExecutors.h].
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
#include "InteropExecutors.h"

#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/concurrent/MultiQueue.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelSort.h"

#if defined( ADS_WITH_TBB )
 #include <tbb/enumerable_thread_specific.h>
 #include <tbb/parallel_for.h>
 #include <tbb/task_arena.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/// the number of Fibonacci by the fork and join of its recursion
template< class E >
std::uint64_t fib( E & ex , int n )
{
 if( n < 2 )
  return( std::uint64_t( n ) );
 if( n < 12 )
  return( fib( ex , n - 1 ) + fib( ex , n - 2 ) );
 std::uint64_t a = 0 , b = 0;
 parallelInvoke(
   ex , [ & ] { a = fib( ex , n - 1 ); } , [ & ] { b = fib( ex , n - 2 ); } );
 return( a + b );
 }

/// a sort, an inclusive scan, a reduction of doubles and the fork and join
/// of a recursion on \f$ ex \f$, equal to those of `Sequential`
template< class E >
void sameAsSequential( E & ex )
{
 static_assert( Executor< E > );
 CHECK( ex.concurrency() >= 1 );
 std::mt19937_64 g( 3 );
 std::vector< double > A( 1 << 17 );
 for( auto & x : A )
  x = double( g() % 1000003 );
 auto B = A;
 std::sort( B.begin() , B.end() );
 ADS::sort( ex , A );
 CHECK( A == B );
 std::vector< long > x( 300007 ) , y( x.size() ) , z( x.size() );
 for( auto & v : x )
  v = long( g() % 100 );
 Sequential seq;
 const long ly =
   parallelInclusiveScan( ex , x.begin() , x.size() , y.begin() , 0L );
 const long lz =
   parallelInclusiveScan( seq , x.begin() , x.size() , z.begin() , 0L );
 CHECK( ( ly == lz ) && ( y == z ) );
 auto part = [ & ]( std::size_t lo , std::size_t hi ) {
  double s = 0;
  for( std::size_t i = lo ; i < hi ; ++i )
   s += 1.0 / double( 1 + x[ i ] );
  return( s );
  };
 CHECK( parallelReduce( ex , 0 , x.size() , 0.0 , part , std::plus<>() ) ==
	parallelReduce( seq , 0 , x.size() , 0.0 , part , std::plus<>() ) );
 CHECK( fib( ex , 24 ) == 46368 );
 // nested: each of 16 calls runs 16 more, each exactly once
 std::vector< std::atomic< int > > grid( 256 );
 ex.bulk( 16 , [ & ]( std::size_t i ) {
  ex.bulk( 16 , [ & ]( std::size_t j ) {
   grid[ 16 * i + j ].fetch_add( 1 , std::memory_order_relaxed );
   } );
  } );
 CHECK( std::all_of( grid.begin() , grid.end() ,
		     []( auto & h ) { return( h.load() == 1 ); } ) );
 }

/// a message for the tests whose runtime is not compiled in
[[maybe_unused]] void notCompiled( const char * what )
{
 std::cout << "  (" << what << " not compiled in: nothing to check)"
	   << std::endl;
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// TBBExecutor on an arena of the program, of 3 threads, and on the current
// arena inside another of 2: the concurrency of each, the threads of the
// calls within the arena, and the algorithms as on Sequential
ADS_TEST( "tbb-arena" )
{
#if defined( ADS_WITH_TBB )
 tbb::task_arena arena( 3 ) , other( 2 );
 TBBExecutor user( arena );
 CHECK( user.concurrency() == 3 );
 std::atomic< int > outside{ 0 };
 user.bulk( 200 , [ & ]( std::size_t ) {
  const int t = tbb::this_task_arena::current_thread_index();
  if( ( t < 0 ) || ( t >= 3 ) )
   outside.fetch_add( 1 );
  } );
 CHECK( outside.load() == 0 );
 sameAsSequential( user );
 other.execute( [ & ] {
  TBBExecutor current( TBBExecutor::current );
  CHECK( current.concurrency() == 2 );
  current.bulk( 200 , [ & ]( std::size_t ) {
   const int t = tbb::this_task_arena::current_thread_index();
   if( ( t < 0 ) || ( t >= 2 ) )
    outside.fetch_add( 1 );
   } );
  sameAsSequential( current );
  } );
 CHECK( outside.load() == 0 );
 // the workers of run() on the arena of the program
 std::atomic< int > workers{ 0 };
 user.run( 2 , [ & ]( std::size_t ) { workers.fetch_add( 1 ); } );
 CHECK( workers.load() == 2 );
#else
 notCompiled( "oneTBB" );
#endif
 }

// OpenMPExecutor inside a region of 3 threads of the program, from omp
// single: its concurrency is that of the team, its calls are tasks that
// more than one thread of the team runs, and the algorithms as on
// Sequential
ADS_TEST( "openmp-in-region" )
{
#if defined( _OPENMP ) && ( _OPENMP >= 201511 )
 std::exception_ptr error;
 std::set< int > threads;
 std::mutex lock;
 bool team = false;
 #pragma omp parallel num_threads( 3 )
 #pragma omp single
 {
  try {
   OpenMPExecutor ex;
   team = ( omp_get_num_threads() < 2 ) || ( ex.concurrency() == 3 );
   ex.bulk( 64 , [ & ]( std::size_t ) {
    volatile double s = 0;
    for( int k = 0 ; k < 200000 ; ++k )
     s = s + 1.0;
    std::lock_guard< std::mutex > held( lock );
    threads.insert( omp_get_thread_num() );
    } );
   sameAsSequential( ex );
   }
  catch( ... ) {
   error = std::current_exception();
   }
  }
 if( error )
  std::rethrow_exception( error );
 CHECK( team );
 CHECK( ( omp_get_max_threads() < 2 ) || ( threads.size() > 1 ) );
#else
 notCompiled( "OpenMP 4.5" );
#endif
 }

// the parallel algorithms of the standard library
ADS_TEST( "stdpar-executor" )
{
#if defined( ADS_WITH_STDPAR )
 StdParExecutor<> par( 4 );
 CHECK( par.concurrency() == 4 );
 sameAsSequential( par );
 StdParExecutor< std::execution::sequenced_policy > seq( 1 );
 sameAsSequential( seq );
#else
 notCompiled( "the parallel algorithms of the standard library" );
#endif
 }

// a scheduler of the senders, the static pool of stdexec of 3 threads: the
// nested calls inline, so that the recursion does not hold the pool
ADS_TEST( "sender-executor" )
{
#if defined( ADS_WITH_STDEXEC )
 exec::static_thread_pool pool( 3 );
 SenderExecutor ex( pool.get_scheduler() , 3 );
 CHECK( ex.concurrency() == 3 );
 sameAsSequential( ex );
#else
 notCompiled( "stdexec" );
#endif
 }

// an executor of Taskflow of 3 workers, the nested calls co-run
ADS_TEST( "taskflow-executor" )
{
#if defined( ADS_WITH_TASKFLOW )
 tf::Executor workers( 3 );
 TaskflowExecutor ex( workers );
 CHECK( ex.concurrency() == 3 );
 sameAsSequential( ex );
#else
 notCompiled( "Taskflow" );
#endif
 }

// the default execution space of the host of Kokkos, the nested calls and
// the calls inside a kernel of the program inline
ADS_TEST( "kokkos-executor" )
{
#if defined( ADS_WITH_KOKKOS )
 test::kokkosOnce();
 KokkosExecutor<> ex;
 sameAsSequential( ex );
 std::atomic< long > sum{ 0 };
 Kokkos::parallel_for(
   Kokkos::RangePolicy< Kokkos::DefaultHostExecutionSpace >( 0 , 4 ) ,
   [ & ]( int ) {
    ex.bulk( 10 , [ & ]( std::size_t i ) { sum.fetch_add( long( i ) ); } );
    } );
 Kokkos::fence();
 CHECK( sum.load() == 4 * 45 );
#else
 notCompiled( "Kokkos" );
#endif
 }

// the concurrent structures of ADS inside the loops of a program: the
// union-find and the MultiQueue (a handle per thread of oneTBB) in a
// tbb::parallel_for, the union-find in a taskloop of OpenMP
ADS_TEST( "structures-in-runtimes" )
{
 [[maybe_unused]] const int n = 1 << 15;  // the items, if a runtime is in
#if defined( ADS_WITH_TBB )
 {
  ConcurrentUnionFind uf( n );
  tbb::parallel_for( 0 , n - 1 , [ & ]( int i ) {
   if( i % 2 == 0 )
    ( void )uf.unite( i , i + 1 );
   } );
  for( int i = 0 ; i + 2 < n ; i += 2 )
   CHECK( ( uf.find( i ) == uf.find( i + 1 ) ) &&
	  ( uf.find( i ) != uf.find( i + 2 ) ) );
  MultiQueue< int , int > mq( 8 );
  tbb::enumerable_thread_specific< decltype( mq.handle() ) > h(
    [ & ] { return( mq.handle() ); } );
  tbb::parallel_for( 0 , 10000 , [ & ]( int i ) { h.local().push( i , i ); } );
  CHECK( mq.size() == 10000 );
  CHECK( mq.consistent() );
  }
#endif
#if defined( _OPENMP ) && ( _OPENMP >= 201511 )
 {
  ConcurrentUnionFind uf( n );
  #pragma omp parallel
  #pragma omp single
  #pragma omp taskloop grainsize( 64 )
  for( int i = 0 ; i < n - 1 ; ++i )
   if( i % 3 != 2 )
    ( void )uf.unite( i , i + 1 );
  for( int i = 0 ; i + 3 < n ; i += 3 )
   CHECK( ( uf.find( i ) == uf.find( i + 2 ) ) &&
	  ( uf.find( i ) != uf.find( i + 3 ) ) );
  }
#endif
#if ! defined( ADS_WITH_TBB ) && ! ( defined( _OPENMP ) && _OPENMP >= 201511 )
 notCompiled( "oneTBB and OpenMP" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File interop_parallel.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
