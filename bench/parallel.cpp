/*--------------------------------------------------------------------------*/
/*--------------------------- File parallel.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the parallel algorithms on t threads (-t, 1 by
 * default): each algorithm of ADS on its own pool of threads (ThreadPool,
 * "pool") and, when they are compiled in, on the executors of FastFlow,
 * OpenMP and oneTBB, against the parallel versions of the libraries:
 * - the sorts: tbb::parallel_sort, std::sort and std::stable_sort with
 *   std::execution::par (libstdc++ on oneTBB), __gnu_parallel::sort and
 *   stable_sort (the parallel mode of libstdc++, on OpenMP), the parallel
 *   IPS4o and IPS2Ra, parlay::sort_inplace, stable_sort_inplace and
 *   integer_sort_inplace of ParlayLib, on 64-bit integers, doubles and
 *   strings in the patterns of bench/sort.cpp, the sequential ADS::sort and
 *   ADS::stableSort as well when t = 1;
 * - the patterns: the sum of 64-bit integers (parallelReduce against
 *   tbb::parallel_reduce, std::reduce with par, an OpenMP reduction,
 *   parlay::reduce and the ParallelForReduce of FastFlow), the inclusive
 *   prefix sums (parallelInclusiveScan against tbb::parallel_scan,
 *   std::inclusive_scan with par, parlay::scan_inclusive and a scan of
 *   OpenMP in two passes) and a loop that maps each item (parallelFor
 *   against tbb::parallel_for, std::for_each with par, an OpenMP loop,
 *   parlay::parallel_for and the ParallelFor of FastFlow);
 * - the stable merge of 4 and 64 sorted runs (multiwayMerge against
 *   __gnu_parallel::stable_multiway_merge and the parallel merge of tlx);
 * - the products of sparse matrices of 10^5 and 10^6 rows by a vector,
 *   8 nonzeros per row or rows of lengths from a power law (CsrMatrix
 *   and SellMatrix against loops on the rows of oneTBB, OpenMP and
 *   ParlayLib and the product of Eigen on OpenMP), per nonzero;
 * - the products of dense matrices of 1024 and 2048 (gemm against
 *   OpenBLAS and Eigen on OpenMP), per multiplication;
 * - the transforms of 2^20 and 2^22 points (fft against FFTW with its
 *   threads, planned with FFTW_MEASURE, the plan not timed), per point;
 * - the visit in breadth from the node of largest degree and the connected
 *   components of graphs of 2^16 and 2^19 nodes and 16 edges per node, at
 *   random or of Kronecker (bfsDistances and connectedComponents against
 *   the direction-optimizing visit, Afforest and Shiloach-Vishkin of the
 *   GAP benchmark suite, and the visit of the examples of ParlayLib), per
 *   arc;
 * - the LCP array of texts of 10^6 and 4 10^6 symbols, of 4 letters or of
 *   words of a Zipf law, from a suffix array of 32 bits (lcpArrayPhi
 *   against the one of the examples of ParlayLib), per symbol;
 * - the insertion and the lookup of n distinct keys in the concurrent hash
 *   tables, all driven by the same threads of the pool
 *   (ConcurrentCuckooTable against tbb::concurrent_hash_map,
 *   tbb::concurrent_unordered_map, the cuckoohash_map of libcuckoo and the
 *   hashtable of ParlayLib), per key, for 10^6 and 4 10^6 keys (the nodes
 *   of 10^7 keys of the tables of oneTBB take more than a GB).
 * Each structure runs only if ADS_BENCH_WHO names it, or with no such
 * variable, and ADS_BENCH_LIST lists them [see Bench.h], so that run.sh
 * runs each in a process of its own. Each cell is the median of r runs,
 * in nanoseconds per item, of the work on a fresh copy of the input, the
 * copy excluded, after a first run not timed that starts the threads of
 * the runtime; a result different from
 * the sequential one gets -1. The benchmark of a cell is "parallel
 * <algorithm> t<t>", so that the cells of each number of threads make a
 * table of their own; its first structure is run twice, the second time as
 * "A/A".
 *
 *   parallel [ -r runs ] [ -s seed ] [ -t threads ] [ -w family ]
 *            [ -b sort|patterns|merge|spmv|gemm|fft|graphs|lcp|hash ]
 *            n1 n2 ...
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

#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"
#include "ADS/concurrent/ConcurrentCuckooTable.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/numeric/FFT.h"
#include "ADS/numeric/SellMatrix.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/ParallelFFT.h"
#include "ADS/parallel/ParallelGraph.h"
#include "ADS/parallel/ParallelMerge.h"
#include "ADS/parallel/ParallelNumeric.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/ParallelText.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixSorting.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/sort/Sort.h"

#if defined( ADS_WITH_FASTFLOW )
 #include <ff/dc.hpp>
#endif

#if defined( ADS_HAS_TBB )
 #include <tbb/concurrent_hash_map.h>
 #include <tbb/concurrent_unordered_map.h>
 #include <tbb/global_control.h>
 #include <tbb/parallel_for.h>
 #include <tbb/parallel_reduce.h>
 #include <tbb/parallel_scan.h>
 #include <tbb/parallel_sort.h>
 #include <execution>
#endif

#if defined( _OPENMP )
 #include <omp.h>
 #include <parallel/algorithm>
#endif

#if __has_include( <ips4o.hpp> ) && defined( _REENTRANT )
 #include <ips4o.hpp>
 #define ADS_HAS_IPS4O 1
#endif

#if __has_include( <ips2ra.hpp> ) && defined( _REENTRANT )
 #include <ips2ra.hpp>
 #define ADS_HAS_IPS2RA 1
#endif

#if __has_include( <parlay/primitives.h> )
 #include <parlay/hash_table.h>
 #include <parlay/parallel.h>
 #include <parlay/primitives.h>
 #define ADS_HAS_PARLAY 1
 #if __has_include( <examples/BFS_ligra.h> )
  #include <examples/BFS_ligra.h>
  #include <examples/longest_common_prefix.h>
  #define ADS_HAS_PARLAY_EXAMPLES 1
 #endif
#endif

#if __has_include( <tlx/algorithm/parallel_multiway_merge.hpp> ) && \
    defined( _OPENMP )
 #include <tlx/algorithm/parallel_multiway_merge.hpp>
 #define ADS_HAS_TLX 1
#endif

#if __has_include( <libcuckoo/cuckoohash_map.hh> )
 #include <libcuckoo/cuckoohash_map.hh>
 #define ADS_HAS_LIBCUCKOO 1
#endif

#if __has_include( <Eigen/Sparse> ) && defined( _OPENMP )
 #include <Eigen/Dense>
 #include <Eigen/Sparse>
 #define ADS_HAS_EIGEN 1
#endif

#if defined( ADS_HAS_OPENBLAS )
 #include <cblas.h>
extern "C" void openblas_set_num_threads( int );
#endif

#if defined( ADS_HAS_FFTW )
 #include <fftw3.h>
#endif

// the kernels of the GAP benchmark suite, each in a namespace of its own
// (their files define main(), and helpers of the same names)
#if __has_include( <benchmark.h> ) && __has_include( <bfs.cc> ) && \
    defined( _OPENMP )
 #include <cinttypes>
 #include <iostream>
 #include <unordered_map>
 #include <benchmark.h>
 #include <bitmap.h>
 #include <builder.h>
 #include <command_line.h>
 #include <graph.h>
 #include <platform_atomics.h>
 #include <pvector.h>
 #include <sliding_queue.h>
 #include <timer.h>
 #define main gapbsMain
namespace gapbsBfs {
 #include <bfs.cc>
}
namespace gapbsCc {
 #include <cc.cc>
}
namespace gapbsSv {
 #include <cc_sv.cc>
}
 #undef main
 #define ADS_HAS_GAPBS 1
#endif

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- INPUTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

int threads = 1;                            ///< -t

const char * patterns[] = { "random" , "sorted" , "reversed" , "few" ,
                            "organ" , "runs16" };

/// the key of rank i of n in the pattern p, from the generator g, as in
/// bench/sort.cpp
std::uint64_t rank( int p , std::size_t i , std::size_t n ,
                    std::mt19937_64 & g )
{
 switch( p ) {
  case( 0 ): return( g() );
  case( 1 ): return( i );
  case( 2 ): return( n - i );
  case( 3 ): return( g() % 16 );
  case( 4 ): return( std::min( i , n - 1 - i ) );
  default: return( ( i % ( n / 16 + 1 ) ) * 16 + i / ( n / 16 + 1 ) );
  }
 }

template< class T >
T make( std::uint64_t k );

template<> std::uint64_t make( std::uint64_t k ) { return( k ); }
template<> double make( std::uint64_t k ) {
 return( double( std::int64_t( k ) ) / 1e3 );
 }
template<> std::string make( std::uint64_t k ) {
 std::string s = "item";
 for( int d = 0 ; d < 12 ; ++d , k /= 26 ) s += char( 'a' + k % 26 );
 return( s );
 }

/// the name of a benchmark on the threads of the run
std::string bench( const std::string & what )
{
 return( "parallel " + what + " t" + std::to_string( threads ) );
 }

/// the median of r times, in nanoseconds per item, of work on copies of A,
/// or -1 if ok( copy ) is false after a run (checked untimed); a first run,
/// not timed, starts the threads of the runtime
template< class V , class W , class K >
double timeIt( const V & A , W && work , int r , K && ok )
{
 {
  V B = A;
  work( B );
 }
 std::vector< double > t;
 for( int i = 0 ; i < r ; ++i ) {
  V B = A;
  const auto a = std::chrono::steady_clock::now();
  work( B );
  const auto b = std::chrono::steady_clock::now();
  if( ! ok( B ) )
   return( -1 );
  t.push_back( std::chrono::duration< double , std::nano >( b - a ).count()
               / double( std::max< std::size_t >( A.size() , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
 }

/// -w: the family of the structures run, "" for all: pool, fastflow (the
/// executor of FastFlow), ff (the loops of FastFlow itself), openmp (with
/// the parallel mode of libstdc++), tbb (with the policies of
/// std::execution), parlay, ips (IPS4o and IPS2Ra) or seq; the runtimes
/// keep their threads between the calls, spinning for a while, so that
/// two families in one process slow each other down
std::string family;

/// whether the family f runs
bool runs( const char * f )
{
 return( family.empty() || ( family == f ) );
 }

#if defined( ADS_WITH_FASTFLOW )
/// the loops of FastFlow itself, the family ff: one object, since its
/// workers spin between the calls; t - 1 workers, the caller waits
ff::ParallelForReduce< std::uint64_t > & ffNative( void )
{
 static ff::ParallelForReduce< std::uint64_t > f( std::max( threads - 1 ,
                                                           1 ) , true );
 return( f );
 }
#endif

#if defined( ADS_WITH_FASTFLOW )
/// the merge sort of A by the pattern ff_DC of FastFlow on t - 1 workers,
/// the leaves of at most n / 4t items sorted by ADS::stableSort, the two
/// sorted halves of a part merged through a buffer by std::merge; stable.
/// The pattern is built for each sort, as FastFlow uses it (its operand is
/// bound at the construction, and a pattern frozen and run again on a new
/// operand fails), so that the time includes the start of its farm
template< class T , class C >
void ffMergeSort( std::vector< T > & A , C less )
{
 struct Part {
  T * a;                                    // the items
  T * b;                                    // room for as many
  std::size_t n;
  };
 const std::size_t leaf = std::max< std::size_t >(
                           A.size() / ( 4 * std::size_t( threads ) ) , 1024 );
 std::vector< T > room( A.size() );
 Part all{ A.data() , room.data() , A.size() } , out{};
 ff::ff_DC< Part , Part > dc(
  []( const Part & p , std::vector< Part > & sub ) {
   const std::size_t h = p.n / 2;
   sub.push_back( Part{ p.a , p.b , h } );
   sub.push_back( Part{ p.a + h , p.b + h , p.n - h } ); } ,
  [ & ]( std::vector< Part > & r , Part & m ) {
   std::merge( r[ 0 ].a , r[ 0 ].a + r[ 0 ].n , r[ 1 ].a ,
               r[ 1 ].a + r[ 1 ].n , r[ 0 ].b , less );
   std::copy( r[ 0 ].b , r[ 0 ].b + r[ 0 ].n + r[ 1 ].n , r[ 0 ].a );
   m = Part{ r[ 0 ].a , r[ 0 ].b , r[ 0 ].n + r[ 1 ].n }; } ,
  [ & ]( const Part & p , Part & m ) {
   std::vector< T > v( p.a , p.a + p.n );
   ADS::stableSort( v , less );
   std::copy( v.begin() , v.end() , p.a );
   m = p; } ,
  [ & ]( const Part & p ) { return( p.n <= leaf ); } ,
  all , out , std::max( threads - 1 , 1 ) );
 dc.run_and_wait_end();
 }
#endif

/// the executors of ADS on the threads of the run, each built at its first
/// use
struct Executors {
 ADS::ThreadPool & pool( void ) {
  if( ! f_pool ) f_pool = std::make_unique< ADS::ThreadPool >( threads );
  return( *f_pool );
  }
#if defined( ADS_WITH_FASTFLOW )
 // the caller of FastFlow waits spinning: t - 1 workers on t cores
 ADS::FastFlowExecutor & ff( void ) {
  if( ! f_ff ) f_ff = std::make_unique< ADS::FastFlowExecutor >(
                                         std::max( threads - 1 , 1 ) );
  return( *f_ff );
  }
#endif
#if defined( _OPENMP )
 ADS::OpenMPExecutor & omp( void ) {
  if( ! f_omp ) f_omp = std::make_unique< ADS::OpenMPExecutor >( threads );
  return( *f_omp );
  }
#endif
#if defined( ADS_WITH_TBB )
 ADS::TBBExecutor & tbb( void ) {
  if( ! f_tbb ) f_tbb = std::make_unique< ADS::TBBExecutor >( threads );
  return( *f_tbb );
  }
#endif
 private:
 std::unique_ptr< ADS::ThreadPool > f_pool;
#if defined( ADS_WITH_FASTFLOW )
 std::unique_ptr< ADS::FastFlowExecutor > f_ff;
#endif
#if defined( _OPENMP )
 std::unique_ptr< ADS::OpenMPExecutor > f_omp;
#endif
#if defined( ADS_WITH_TBB )
 std::unique_ptr< ADS::TBBExecutor > f_tbb;
#endif
 };

/// a structure of a battery and its work on a copy of the input
template< class V >
using Battery = std::vector< std::pair< std::string ,
                                        std::function< void( V & ) > > >;

/// adds the structure of the family f to the battery, if f runs
template< class V , class W >
void add( Battery< V > & S , const char * f , const std::string & name ,
          W && work )
{
 if( runs( f ) )
  S.emplace_back( name , std::forward< W >( work ) );
 }

/// the work of an algorithm of ADS on each executor, named "<name> pool",
/// "<name> FastFlow", "<name> OpenMP" and "<name> TBB"
template< class V , class F >
void onExecutors( Battery< V > & S , Executors & X , const std::string & name ,
                  F && f )
{
 add( S , "pool" , name + " pool" , [ & X , f ]( V & A ) {
  f( X.pool() , A ); } );
#if defined( ADS_WITH_FASTFLOW )
 add( S , "fastflow" , name + " FastFlow" , [ & X , f ]( V & A ) {
  f( X.ff() , A ); } );
#endif
#if defined( _OPENMP )
 add( S , "openmp" , name + " OpenMP" , [ & X , f ]( V & A ) {
  f( X.omp() , A ); } );
#endif
#if defined( ADS_WITH_TBB )
 add( S , "tbb" , name + " TBB" , [ & X , f ]( V & A ) {
  f( X.tbb() , A ); } );
#endif
 }

/// runs a battery on A: its first structure twice, the second as "A/A"
template< class V , class K >
void runBattery( const std::string & b , const std::string & c ,
                 const V & A , Battery< V > & S , int r , K && ok )
{
 if( S.empty() )
  return;
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  if( ! ADS::bench::skip( S[ i ].first ) )
   ADS::bench::print( b.c_str() , c , A.size() , S[ i ].first ,
                      timeIt( A , S[ i ].second , r , ok ) );
  if( ( i == 0 ) && ! ADS::bench::skip( S[ i ].first + " A/A" ) )
   ADS::bench::print( b.c_str() , c , A.size() , S[ i ].first + " A/A" ,
                      timeIt( A , S[ i ].second , r , ok ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- SORTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the sorts of T, the stable ones apart
template< class T >
void sorts( const char * type , const std::vector< std::size_t > & sizes ,
            int r , unsigned seed , Executors & X )
{
 using V = std::vector< T >;
 std::less<> less;
 auto sorted = [ & ]( const V & B ) {
  return( std::is_sorted( B.begin() , B.end() , less ) ); };
 for( const bool stable : { false , true } ) {
  Battery< V > S;
#if defined( ADS_HAS_TBB )
  if( ! stable )
   add( S , "tbb" , "tbb::parallel_sort" , [ & ]( V & A ) {
    tbb::parallel_sort( A.begin() , A.end() , less ); } );
  else
   add( S , "tbb" , "std::stable_sort(par)" , [ & ]( V & A ) {
    std::stable_sort( std::execution::par , A.begin() , A.end() ,
                      less ); } );
#endif
  if( ! stable )
   onExecutors( S , X , "ADS::sort" , [ less ]( auto & ex , V & A ) {
    ADS::sort( ex , A , less ); } );
  else
   onExecutors( S , X , "ADS::stableSort" , [ less ]( auto & ex , V & A ) {
    ADS::stableSort( ex , A , less ); } );
  if( threads == 1 )
   add( S , "seq" , stable ? "ADS::stableSort (sequential)" :
                                 "ADS::sort (sequential)" ,
                   [ & , stable ]( V & A ) {
                    if( stable ) ADS::stableSort( A , less );
                    else ADS::sort( A , less ); } );
#if defined( ADS_HAS_TBB )
  if( ! stable )
   add( S , "tbb" , "std::sort(par)" , [ & ]( V & A ) {
    std::sort( std::execution::par , A.begin() , A.end() , less ); } );
#endif
#if defined( _OPENMP )
  if( ! stable )
   add( S , "openmp" , "__gnu_parallel::sort" , [ & ]( V & A ) {
    __gnu_parallel::sort( A.begin() , A.end() , less ); } );
  else
   add( S , "openmp" , "__gnu_parallel::stable_sort" , [ & ]( V & A ) {
    __gnu_parallel::stable_sort( A.begin() , A.end() , less ); } );
#endif
#if defined( ADS_HAS_IPS4O )
  if( ! stable )
   add( S , "ips" , "ips4o::parallel::sort" , [ & ]( V & A ) {
    static ips4o::DefaultThreadPool pool( threads );
    ips4o::parallel::sort( A.begin() , A.end() , less , pool ); } );
#endif
#if defined( ADS_HAS_IPS2RA )
  if constexpr( std::is_integral_v< T > )
   if( ! stable )
    add( S , "ips" , "ips2ra::parallel::sort" , [ & ]( V & A ) {
     ips2ra::parallel::sort( A.begin() , A.end() ,
                             []( const T & x ) { return( x ); } ,
                             threads ); } );
#endif
#if defined( ADS_WITH_FASTFLOW )
  // the divide and conquer of FastFlow: a merge sort whose halves go to
  // its workers down to n / 4t items, sorted by ADS::stableSort, and whose
  // merges (std::merge, on one thread each) climb back
  add( S , "ff" , "ff_DC mergesort (ADS leaves)" , [ & ]( V & A ) {
   ffMergeSort( A , less ); } );
#endif
#if defined( ADS_HAS_PARLAY )
  if( ! stable )
   add( S , "parlay" , "parlay::sort_inplace" , [ & ]( V & A ) {
    parlay::sort_inplace( A , less ); } );
  else
   add( S , "parlay" , "parlay::stable_sort_inplace" , [ & ]( V & A ) {
    parlay::stable_sort_inplace( A , less ); } );
  if constexpr( std::is_integral_v< T > )
   add( S , "parlay" , "parlay::integer_sort_inplace" , [ & ]( V & A ) {
    parlay::integer_sort_inplace( A ); } );
#endif
  for( std::size_t n : sizes )
   for( int p = 0 ; p < 6 ; ++p ) {
    std::mt19937_64 g( seed );
    V A( n );
    for( std::size_t i = 0 ; i < n ; ++i )
     A[ i ] = make< T >( rank( p , i , n , g ) );
    runBattery( bench( std::string( stable ? "stableSort " : "sort " ) +
                       type ) , patterns[ p ] , A , S , r , sorted );
    }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- PATTERNS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the reduction, the prefix sums and the loop on 64-bit integers
void patternsOn( const std::vector< std::size_t > & sizes , int r ,
                 unsigned seed , Executors & X )
{
 using V = std::vector< std::uint64_t >;
 for( std::size_t n : sizes ) {
  std::mt19937_64 g( seed );
  V A( n );
  for( auto & x : A )
   x = g() >> 20;
  // the sum, written into the first item so that the copy carries it
  const std::uint64_t sum = std::accumulate( A.begin() , A.end() ,
                                             std::uint64_t( 0 ) );
  {
   Battery< V > S;
   auto body = []( const V & B , std::size_t lo , std::size_t hi ) {
    std::uint64_t s = 0;
    for( std::size_t i = lo ; i < hi ; ++i ) s += B[ i ];
    return( s ); };
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "tbb::parallel_reduce" , [ & ]( V & B ) {
    B[ 0 ] = tbb::parallel_reduce(
     tbb::blocked_range< std::size_t >( 0 , B.size() ) , std::uint64_t( 0 ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & q , std::uint64_t s ) {
      return( s + body( B , q.begin() , q.end() ) ); } , std::plus<>() ); } );
#endif
   onExecutors( S , X , "ADS::parallelReduce" , [ body ]( auto & ex ,
                                                          V & B ) {
    B[ 0 ] = ADS::parallelReduce( ex , 0 , B.size() , std::uint64_t( 0 ) ,
                                  [ & ]( std::size_t lo , std::size_t hi ) {
                                   return( body( B , lo , hi ) ); } ,
                                  std::plus<>() ); } );
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "std::reduce(par)" , [ & ]( V & B ) {
    B[ 0 ] = std::reduce( std::execution::par , B.begin() , B.end() ,
                          std::uint64_t( 0 ) ); } );
#endif
#if defined( _OPENMP )
   add( S , "openmp" , "omp reduction" , [ & ]( V & B ) {
    std::uint64_t s = 0;
    const long m = long( B.size() );
    #pragma omp parallel for reduction( + : s ) num_threads( threads )
    for( long i = 0 ; i < m ; ++i ) s += B[ std::size_t( i ) ];
    B[ 0 ] = s; } );
#endif
#if defined( ADS_HAS_PARLAY )
   add( S , "parlay" , "parlay::reduce" , [ & ]( V & B ) {
    B[ 0 ] = parlay::reduce( B ); } );
#endif
#if defined( ADS_WITH_FASTFLOW )
   add( S , "ff" , "ff::ParallelForReduce" , [ & ]( V & B ) {
    std::uint64_t s = 0;
    ffNative().parallel_reduce_static( s , std::uint64_t( 0 ) , 0 ,
                                long( B.size() ) , 1 , 0 ,
                                [ & ]( const long i , std::uint64_t & t ) {
                                 t += B[ std::size_t( i ) ]; } ,
                                []( std::uint64_t & t ,
                                    const std::uint64_t e ) { t += e; } ,
                                std::max( threads - 1 , 1 ) );
    B[ 0 ] = s; } );
#endif
   runBattery( bench( "reduce" ) , "uint64" , A , S , r ,
               [ & ]( const V & B ) { return( B[ 0 ] == sum ); } );
  }
  // the prefix sums, in place
  V P( n );
  std::inclusive_scan( A.begin() , A.end() , P.begin() );
  {
   Battery< V > S;
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "tbb::parallel_scan" , [ & ]( V & B ) {
    tbb::parallel_scan( tbb::blocked_range< std::size_t >( 0 , B.size() ) ,
     std::uint64_t( 0 ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & q , std::uint64_t s ,
            bool final ) {
      for( std::size_t i = q.begin() ; i < q.end() ; ++i ) {
       s += B[ i ];
       if( final ) B[ i ] = s;
       }
      return( s ); } , std::plus<>() ); } );
#endif
   onExecutors( S , X , "ADS::parallelInclusiveScan" , []( auto & ex ,
                                                           V & B ) {
    ADS::parallelInclusiveScan( ex , B.begin() , B.size() , B.begin() ,
                                std::uint64_t( 0 ) ); } );
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "std::inclusive_scan(par)" , [ & ]( V & B ) {
    std::inclusive_scan( std::execution::par , B.begin() , B.end() ,
                         B.begin() ); } );
#endif
#if defined( _OPENMP )
   add( S , "openmp" , "omp scan" , [ & ]( V & B ) {
    // the sums of the blocks of the threads, then each block again
    std::vector< std::uint64_t > part( std::size_t( threads ) + 1 , 0 );
    const std::size_t m = B.size();
    #pragma omp parallel num_threads( threads )
    {
     const std::size_t t = std::size_t( omp_get_thread_num() ) ,
                       q = std::size_t( omp_get_num_threads() ) ,
                       lo = m * t / q , hi = m * ( t + 1 ) / q;
     std::uint64_t s = 0;
     for( std::size_t i = lo ; i < hi ; ++i ) s += B[ i ];
     part[ t + 1 ] = s;
     #pragma omp barrier
     #pragma omp single
     for( std::size_t i = 1 ; i <= q ; ++i ) part[ i ] += part[ i - 1 ];
     s = part[ t ];
     for( std::size_t i = lo ; i < hi ; ++i ) B[ i ] = s += B[ i ];
     } } );
#endif
#if defined( ADS_HAS_PARLAY )
   add( S , "parlay" , "parlay::scan_inclusive_inplace" , [ & ]( V & B ) {
    parlay::scan_inclusive_inplace( B ); } );
#endif
   runBattery( bench( "scan" ) , "uint64" , A , S , r ,
               [ & ]( const V & B ) { return( B == P ); } );
  }
  // a map: x -> a hash of x, in place
  auto mix = []( std::uint64_t x ) {
   x ^= x >> 33; x *= 0xFF51AFD7ED558CCDull; x ^= x >> 33; return( x ); };
  V M( n );
  std::transform( A.begin() , A.end() , M.begin() , mix );
  {
   Battery< V > S;
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "tbb::parallel_for" , [ & ]( V & B ) {
    tbb::parallel_for( tbb::blocked_range< std::size_t >( 0 , B.size() ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & q ) {
      for( std::size_t i = q.begin() ; i < q.end() ; ++i )
       B[ i ] = mix( B[ i ] ); } ); } );
#endif
   onExecutors( S , X , "ADS::parallelForBlocks" , [ mix ]( auto & ex ,
                                                            V & B ) {
    ADS::parallelForBlocks( ex , 0 , B.size() ,
                            [ & ]( std::size_t lo , std::size_t hi ) {
                             for( std::size_t i = lo ; i < hi ; ++i )
                              B[ i ] = mix( B[ i ] ); } ); } );
#if defined( ADS_HAS_TBB )
   add( S , "tbb" , "std::for_each(par)" , [ & ]( V & B ) {
    std::for_each( std::execution::par , B.begin() , B.end() ,
                   [ & ]( std::uint64_t & x ) { x = mix( x ); } ); } );
#endif
#if defined( _OPENMP )
   add( S , "openmp" , "omp for" , [ & ]( V & B ) {
    const long m = long( B.size() );
    #pragma omp parallel for num_threads( threads )
    for( long i = 0 ; i < m ; ++i )
     B[ std::size_t( i ) ] = mix( B[ std::size_t( i ) ] ); } );
#endif
#if defined( ADS_HAS_PARLAY )
   add( S , "parlay" , "parlay::parallel_for" , [ & ]( V & B ) {
    parlay::parallel_for( 0 , B.size() , [ & ]( std::size_t i ) {
     B[ i ] = mix( B[ i ] ); } ); } );
#endif
#if defined( ADS_WITH_FASTFLOW )
   add( S , "ff" , "ff::ParallelFor" , [ & ]( V & B ) {
    ffNative().parallel_for_static( 0 , long( B.size() ) , 1 , 0 ,
                              [ & ]( const long i ) {
                               B[ std::size_t( i ) ] =
                                mix( B[ std::size_t( i ) ] ); } ,
                              std::max( threads - 1 , 1 ) ); } );
#endif
   runBattery( bench( "map" ) , "uint64" , A , S , r ,
               [ & ]( const V & B ) { return( B == M ); } );
  }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ NO COPIES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the median of r runs of work(), in nanoseconds per one of n items, each
/// after an untimed prepare(), or -1 if ok() is false after a run; a first
/// run, not timed, starts the threads of the runtime
template< class P , class W , class K >
double timeRuns( std::size_t n , P && prepare , W && work , int r , K && ok )
{
 prepare();
 work();
 std::vector< double > t;
 for( int i = 0 ; i < r ; ++i ) {
  prepare();
  const auto a = std::chrono::steady_clock::now();
  work();
  const auto b = std::chrono::steady_clock::now();
  if( ! ok() )
   return( -1 );
  t.push_back( std::chrono::duration< double , std::nano >( b - a ).count()
               / double( std::max< std::size_t >( n , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
 }

/// a battery of works on inputs that need no copy
using Works = std::vector< std::pair< std::string ,
                                      std::function< void( void ) > > >;

void addW( Works & S , const char * f , const std::string & name ,
           std::function< void( void ) > work )
{
 if( runs( f ) )
  S.emplace_back( name , std::move( work ) );
 }

/// the work of an algorithm of ADS on each executor, f( ex )
template< class F >
void onExecutorsW( Works & S , Executors & X , const std::string & name ,
                   F && f )
{
 addW( S , "pool" , name + " pool" , [ & X , f ] { f( X.pool() ); } );
#if defined( ADS_WITH_FASTFLOW )
 addW( S , "fastflow" , name + " FastFlow" , [ & X , f ] { f( X.ff() ); } );
#endif
#if defined( _OPENMP )
 addW( S , "openmp" , name + " OpenMP" , [ & X , f ] { f( X.omp() ); } );
#endif
#if defined( ADS_WITH_TBB )
 addW( S , "tbb" , name + " TBB" , [ & X , f ] { f( X.tbb() ); } );
#endif
 }

/// runs a battery: its first structure twice, the second as "A/A"
template< class P , class K >
void runWorks( const std::string & b , const std::string & c , std::size_t n ,
               Works & S , int r , P && prepare , K && ok )
{
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  if( ! ADS::bench::skip( S[ i ].first ) )
   ADS::bench::print( b.c_str() , c , n , S[ i ].first ,
                      timeRuns( n , prepare , S[ i ].second , r , ok ) );
  if( ( i == 0 ) && ! ADS::bench::skip( S[ i ].first + " A/A" ) )
   ADS::bench::print( b.c_str() , c , n , S[ i ].first + " A/A" ,
                      timeRuns( n , prepare , S[ i ].second , r , ok ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MERGES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the stable merge of k sorted runs of 64-bit integers, k = 4 and 64
void merges( const std::vector< std::size_t > & sizes , int r ,
             unsigned seed , Executors & X )
{
 using U = std::uint64_t;
 using Range = std::pair< const U * , const U * >;
 for( std::size_t n : sizes )
  for( std::size_t k : { 4 , 64 } ) {
   std::mt19937_64 g( seed );
   std::vector< std::vector< U > > R( k );
   std::vector< Range > Q;
   // the merges of libstdc++ and tlx want iterators that can write
   std::vector< std::pair< U * , U * > > Qw;
   for( std::size_t i = 0 ; i < k ; ++i ) {
    R[ i ].resize( n / k + ( i < n % k ) );
    for( auto & x : R[ i ] ) x = g() % ( n / 2 + 1 );   // with ties
    std::sort( R[ i ].begin() , R[ i ].end() );
    Q.emplace_back( R[ i ].data() , R[ i ].data() + R[ i ].size() );
    Qw.emplace_back( R[ i ].data() , R[ i ].data() + R[ i ].size() );
    }
   std::vector< U > ref( n ) , out( n , 0 );
   ADS::multiwayMerge( Q , ref.begin() );
   Works S;
#if defined( _OPENMP )
   addW( S , "openmp" , "__gnu_parallel::stable_multiway_merge" , [ & ] {
    auto C = Qw;
    __gnu_parallel::stable_multiway_merge( C.begin() , C.end() ,
                                           out.begin() , n , std::less<>() ,
                                           __gnu_parallel::exact_tag() ); } );
#endif
   onExecutorsW( S , X , "ADS::multiwayMerge" , [ & ]( auto & ex ) {
    ADS::multiwayMerge( ex , Q , out.begin() ); } );
   if( threads == 1 )
    addW( S , "seq" , "ADS::multiwayMerge (sequential)" , [ & ] {
     ADS::multiwayMerge( Q , out.begin() ); } );
#if defined( ADS_HAS_TLX )
   addW( S , "openmp" , "tlx::stable_parallel_multiway_merge" , [ & ] {
    auto C = Qw;
    tlx::stable_parallel_multiway_merge( C.begin() , C.end() , out.begin() ,
                                         std::ptrdiff_t( n ) ,
                                         std::less<>() ,
                                         tlx::MWMA_ALGORITHM_DEFAULT ,
                                         tlx::MWMSA_DEFAULT ,
                                         std::size_t( threads ) ); } );
#endif
   runWorks( bench( "merge" ) , "k" + std::to_string( k ) , n , S , r ,
             [ & ] { std::fill( out.begin() , out.end() , 0 ); } ,
             [ & ] { return( out == ref ); } );
   }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- PRODUCTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the products y = A x of sparse matrices of n rows, 8 nonzeros per row
/// at random columns ("uniform") or rows of lengths from a power law
/// ("powerlaw", a Pareto law of exponent 1.5, 4 nonzeros at least), in
/// nanoseconds per nonzero
void sparse( const std::vector< std::size_t > & sizes , int r ,
             unsigned seed , Executors & X )
{
 for( std::size_t n : sizes )
  for( int shape = 0 ; shape < 2 ; ++shape ) {
   std::mt19937_64 g( seed );
   std::uniform_real_distribution< double > u( 0 , 1 );
   std::vector< ADS::CsrMatrix::Triplet > T;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    std::size_t d = 8;
    if( shape == 1 )
     d = std::min< std::size_t >( n / 4 , std::size_t(
                                  4 * std::pow( 1 - u( g ) , -2.0 / 3 ) ) );
    for( std::size_t e = 0 ; e < d ; ++e )
     T.emplace_back( i , g() % n , u( g ) - 0.5 );
    }
   const ADS::CsrMatrix A( n , n , T );
   const ADS::SellMatrix<> L( A );
   std::vector< double > x( n ) , y( n ) , ref;
   for( auto & v : x ) v = u( g );
   ref = A.multiply( x );
   const std::size_t nnz = A.nonzeros();
   Works S;
#if defined( ADS_HAS_TBB )
   addW( S , "tbb" , "tbb::parallel_for rows" , [ & ] {
    tbb::parallel_for( tbb::blocked_range< std::size_t >( 0 , n ) ,
     [ & ]( const tbb::blocked_range< std::size_t > & q ) {
      const std::size_t * st = A.rowStart();
      for( std::size_t i = q.begin() ; i < q.end() ; ++i ) {
       double s = 0;
       for( std::size_t k = st[ i ] ; k < st[ i + 1 ] ; ++k )
        s += A.values()[ k ] * x[ A.columns()[ k ] ];
       y[ i ] = s;
       } } ); } );
#endif
   onExecutorsW( S , X , "ADS::CsrMatrix" , [ & ]( auto & ex ) {
    ADS::multiply( ex , A , x.data() , y.data() ); } );
   onExecutorsW( S , X , "ADS::SellMatrix" , [ & ]( auto & ex ) {
    ADS::multiply( ex , L , x.data() , y.data() ); } );
   if( threads == 1 ) {
    addW( S , "seq" , "ADS::CsrMatrix (sequential)" , [ & ] {
     A.multiply( x.data() , y.data() ); } );
    addW( S , "seq" , "ADS::SellMatrix (sequential)" , [ & ] {
     L.multiply( x.data() , y.data() ); } );
    }
#if defined( _OPENMP )
   addW( S , "openmp" , "omp for rows" , [ & ] {
    const std::size_t * st = A.rowStart();
    const long m = long( n );
    #pragma omp parallel for schedule( dynamic , 256 ) num_threads( threads )
    for( long i = 0 ; i < m ; ++i ) {
     double s = 0;
     for( std::size_t k = st[ i ] ; k < st[ i + 1 ] ; ++k )
      s += A.values()[ k ] * x[ A.columns()[ k ] ];
     y[ std::size_t( i ) ] = s;
     } } );
#endif
#if defined( ADS_HAS_EIGEN )
   using ES = Eigen::SparseMatrix< double , Eigen::RowMajor , std::int64_t >;
   std::shared_ptr< ES > E;
   if( runs( "openmp" ) && ! ADS::bench::skip( "Eigen::SparseMatrix" ) ) {
    E = std::make_shared< ES >( long( n ) , long( n ) );
    std::vector< Eigen::Triplet< double , std::int64_t > > ET;
    for( const auto & [ i , j , v ] : T )
     ET.emplace_back( std::int64_t( i ) , std::int64_t( j ) , v );
    E->setFromTriplets( ET.begin() , ET.end() );
    E->makeCompressed();
    }
   addW( S , "openmp" , "Eigen::SparseMatrix" , [ & ] {
    Eigen::setNbThreads( threads );
    Eigen::Map< const Eigen::VectorXd > xv( x.data() , long( n ) );
    Eigen::Map< Eigen::VectorXd > yv( y.data() , long( n ) );
    yv.noalias() = *E * xv; } );
#endif
#if defined( ADS_HAS_PARLAY )
   addW( S , "parlay" , "parlay::parallel_for rows" , [ & ] {
    const std::size_t * st = A.rowStart();
    parlay::parallel_for( 0 , n , [ & ]( std::size_t i ) {
     double s = 0;
     for( std::size_t k = st[ i ] ; k < st[ i + 1 ] ; ++k )
      s += A.values()[ k ] * x[ A.columns()[ k ] ];
     y[ i ] = s; } ); } );
#endif
   // the products by rows sum in the same order; Eigen and the SELL
   // format may differ in the last bits
   runWorks( bench( "spmv" ) , shape ? "powerlaw" : "uniform" , nnz , S , r ,
             [ & ] { std::fill( y.begin() , y.end() , 0.0 ); } ,
             [ & ] {
              for( std::size_t i = 0 ; i < n ; ++i )
               if( std::abs( y[ i ] - ref[ i ] ) > 1e-9 ) return( false );
              return( true ); } );
   }
 }

/// the products C = A B of square dense matrices of doubles, n = 1024 and
/// 2048, in nanoseconds per multiplication of entries (n^3)
void dense( int r , unsigned seed , Executors & X )
{
 for( std::size_t n : { 1024 , 2048 } ) {
  std::mt19937_64 g( seed );
  std::uniform_real_distribution< double > u( -1 , 1 );
  ADS::DenseMatrix< double > A( n , n ) , B( n , n ) , C( n , n ) ;
  for( std::size_t i = 0 ; i < n * n ; ++i ) {
   A.data()[ i ] = u( g );
   B.data()[ i ] = u( g );
   }
  const auto ref = ADS::multiply( A , B );
  Works S;
#if defined( ADS_HAS_OPENBLAS )
  addW( S , "blas" , "OpenBLAS dgemm" , [ & ] {
   openblas_set_num_threads( threads );
   cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , int( n ) ,
                int( n ) , int( n ) , 1.0 , A.data() , int( n ) , B.data() ,
                int( n ) , 0.0 , C.data() , int( n ) ); } );
#endif
  onExecutorsW( S , X , "ADS::gemm" , [ & ]( auto & ex ) {
   ADS::gemm( ex , n , n , n , 1.0 , A.data() , n , B.data() , n , 0.0 ,
              C.data() , n ); } );
  if( threads == 1 )
   addW( S , "seq" , "ADS::gemm (sequential)" , [ & ] {
    ADS::gemm( n , n , n , 1.0 , A.data() , n , B.data() , n , 0.0 ,
               C.data() , n ); } );
#if defined( ADS_HAS_EIGEN )
  addW( S , "openmp" , "Eigen::Matrix" , [ & ] {
   using M = Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic ,
                            Eigen::RowMajor >;
   Eigen::setNbThreads( threads );
   Eigen::Map< const M > a( A.data() , long( n ) , long( n ) ) ,
                         b( B.data() , long( n ) , long( n ) );
   Eigen::Map< M > c( C.data() , long( n ) , long( n ) );
   c.noalias() = a * b; } );
#endif
  runWorks( bench( "gemm" ) , "double" , n * n * n , S , r ,
            [ & ] { std::fill( C.data() , C.data() + n * n , 0.0 ); } ,
            [ & ] {
             for( std::size_t i = 0 ; i < n * n ; ++i )
              if( std::abs( C.data()[ i ] - ref.data()[ i ] ) > 1e-9 )
               return( false );
             return( true ); } );
  }
 }

#if defined( ADS_HAS_FFTW )
/// the allocator of FFTW, so that every array of the benchmark has the
/// alignment of the one its plan was made on
template< class T >
struct FftwAllocator {
 using value_type = T;
 FftwAllocator( void ) = default;
 template< class U > FftwAllocator( const FftwAllocator< U > & ) {}
 T * allocate( std::size_t n ) {
  return( static_cast< T * >( fftw_malloc( n * sizeof( T ) ) ) );
  }
 void deallocate( T * p , std::size_t ) { fftw_free( p ); }
 template< class U > bool operator==( const FftwAllocator< U > & ) const {
  return( true );
  }
 };
#endif

/// the transforms of n = 2^20 and 2^22 complex points, in nanoseconds per
/// point; FFTW planned with FFTW_MEASURE (the plan not timed)
void ffts( int r , unsigned seed , Executors & X )
{
 for( std::size_t n : { std::size_t( 1 ) << 20 , std::size_t( 1 ) << 22 } ) {
  std::mt19937_64 g( seed );
  std::uniform_real_distribution< double > u( -1 , 1 );
  std::vector< ADS::Complex > A( n );
  for( auto & z : A ) z = ADS::Complex( u( g ) , u( g ) );
  std::vector< ADS::Complex > ref = A;
  ADS::fft( ref , false );
  auto close = [ & ]( const auto & B ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    if( std::abs( B[ i ] - ref[ i ] ) > 1e-8 ) return( false );
   return( true ); };
  using V = std::vector< ADS::Complex >;
  Battery< V > S;
  onExecutors( S , X , "ADS::fft" , []( auto & ex , V & B ) {
   ADS::fft( ex , B , false ); } );
  if( threads == 1 )
   add( S , "seq" , "ADS::fft (sequential)" , []( V & B ) {
    ADS::fft( B , false ); } );
  runBattery( bench( "fft" ) , "complex" , A , S , r , close );
#if defined( ADS_HAS_FFTW )
  if( runs( "fftw" ) && ! ADS::bench::skip( "FFTW" ) ) {
   using FV = std::vector< ADS::Complex , FftwAllocator< ADS::Complex > >;
   FV F( A.begin() , A.end() );
   fftw_init_threads();
   fftw_plan_with_nthreads( threads );
   FV P( n );
   auto * pp = reinterpret_cast< fftw_complex * >( P.data() );
   fftw_plan plan = fftw_plan_dft_1d( int( n ) , pp , pp , FFTW_FORWARD ,
                                      FFTW_MEASURE );
   Battery< FV > W;
   W.emplace_back( "FFTW" , [ & ]( FV & B ) {
    auto * b = reinterpret_cast< fftw_complex * >( B.data() );
    fftw_execute_dft( plan , b , b ); } );
   runBattery( bench( "fft" ) , "complex" , F , W , r , close );
   fftw_destroy_plan( plan );
   }
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- GRAPHS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the edges of an unoriented graph of 2^scale nodes and 16 edges per node:
/// at random ("uniform"), or by the recursive matrix of Kronecker with the
/// parameters of the Graph 500, a = 0.57 , b = c = 0.19 ("kronecker"), the
/// names of the nodes shuffled; no loop
std::vector< std::pair< int , int > > edges( int scale , bool kron ,
                                             unsigned seed )
{
 std::mt19937_64 g( seed );
 const int n = 1 << scale;
 std::vector< int > name( static_cast< std::size_t >( n ) );
 std::iota( name.begin() , name.end() , 0 );
 std::shuffle( name.begin() , name.end() , g );
 std::vector< std::pair< int , int > > E;
 std::uniform_real_distribution< double > u( 0 , 1 );
 for( std::size_t e = 0 ; e < 16 * std::size_t( n ) ; ++e ) {
  int a = 0 , b = 0;
  if( kron )
   for( int l = 0 ; l < scale ; ++l ) {
    // the quadrant: a ( 0 , 0 ), b ( 0 , 1 ), c ( 1 , 0 ), d ( 1 , 1 )
    const double x = u( g );
    a = 2 * a + int( x >= 0.76 );
    b = 2 * b + int( ( ( x >= 0.57 ) && ( x < 0.76 ) ) || ( x >= 0.95 ) );
    }
  else {
   a = int( g() % unsigned( n ) );
   b = int( g() % unsigned( n ) );
   }
  if( a != b )
   E.emplace_back( name[ std::size_t( a ) ] , name[ std::size_t( b ) ] );
  }
 return( E );
 }

/// the visit in breadth from a node of the largest component and the
/// connected components, on graphs of 2^scale nodes, in nanoseconds per
/// arc
void graphs( const std::vector< int > & scales , int r , unsigned seed ,
             Executors & X )
{
 using G = ADS::StaticGraph<>;
 for( int scale : scales )
  for( bool kron : { false , true } ) {
   const auto E = edges( scale , kron , seed );
   const G g( 1 << scale , E , false );
   const int n = g.n();
   const std::size_t arcs = std::size_t( g.arcs() );
   // the source: the node of largest degree
   int src = 0;
   for( int v = 0 ; v < n ; ++v )
    if( g.degree( v ) > g.degree( src ) ) src = v;
   ADS::Sequential seq;
   const auto ref = ADS::bfsDistances( seq , g , src );
   const auto lab = ADS::connectedComponents( seq , g );
   std::size_t reached = 0 , comps = 0;
   for( int v = 0 ; v < n ; ++v ) {
    reached += ref[ std::size_t( v ) ] >= 0;
    comps += lab[ std::size_t( v ) ] == v;
    }
   const char * c = kron ? "kronecker" : "uniform";
   std::vector< int > dist , comp;
   std::size_t count = 0;
#if defined( ADS_HAS_GAPBS )
   std::unique_ptr< Graph > gg;
   const bool gap = ! ADS::bench::skip( "gapbs DOBFS" ) |
                    ! ADS::bench::skip( "gapbs DOBFS A/A" ) |
                    ! ADS::bench::skip( "gapbs Afforest" ) |
                    ! ADS::bench::skip( "gapbs Afforest A/A" ) |
                    ! ADS::bench::skip( "gapbs ShiloachVishkin" );
   if( runs( "openmp" ) && gap ) {
    NodeID * neighs = new NodeID[ arcs ];
    pvector< SGOffset > off( std::size_t( n ) + 1 );
    for( int v = 0 ; v <= n ; ++v ) off[ std::size_t( v ) ] = g.offsets()[ v ];
    std::copy( g.arcData() , g.arcData() + arcs , neighs );
    gg = std::make_unique< Graph >( n , Graph::GenIndex( off , neighs ) ,
                                    neighs );
    }
#endif
#if defined( ADS_HAS_PARLAY_EXAMPLES )
   parlay::sequence< parlay::sequence< int > > pg;
   if( runs( "parlay" ) && ! ADS::bench::skip( "parlay BFS_ligra" ) )
    pg = parlay::tabulate( std::size_t( n ) , [ & ]( std::size_t v ) {
     return( parlay::to_sequence( g.out( int( v ) ) ) ); } );
#endif
   {
    Works S;
#if defined( ADS_HAS_GAPBS )
    addW( S , "openmp" , "gapbs DOBFS" , [ & ] {
     const auto p = gapbsBfs::DOBFS( *gg , src );
     count = std::size_t( std::count_if( p.begin() , p.end() , []( auto x ) {
      return( x >= 0 ); } ) ); } );
#endif
    onExecutorsW( S , X , "ADS::bfsDistances" , [ & ]( auto & ex ) {
     dist = ADS::bfsDistances( ex , g , src ); } );
    if( threads == 1 )
     addW( S , "seq" , "ADS::bfsDistances (sequential)" , [ & ] {
      dist = ADS::bfsDistances( seq , g , src ); } );
#if defined( ADS_HAS_PARLAY_EXAMPLES )
    addW( S , "parlay" , "parlay BFS_ligra" , [ & ] {
     const auto F = BFS( src , pg , pg );
     count = 0;
     for( const auto & f : F ) count += f.size(); } );
#endif
    runWorks( bench( "bfs" ) , c , arcs , S , r ,
              [ & ] { dist.clear(); count = 0; } ,
              [ & ] { return( dist.empty() ? count == reached
                                           : dist == ref ); } );
   }
   {
    Works S;
#if defined( ADS_HAS_GAPBS )
    auto distinct = [ & ]( const auto & p ) {
     std::vector< int > q( p.begin() , p.end() );
     std::sort( q.begin() , q.end() );
     return( std::size_t( std::unique( q.begin() , q.end() ) -
                          q.begin() ) ); };
    addW( S , "openmp" , "gapbs Afforest" , [ & ] {
     count = distinct( gapbsCc::Afforest( *gg ) ); } );
#endif
    onExecutorsW( S , X , "ADS::connectedComponents" , [ & ]( auto & ex ) {
     comp = ADS::connectedComponents( ex , g ); } );
    if( threads == 1 )
     addW( S , "seq" , "ADS::connectedComponents (sequential)" , [ & ] {
      comp = ADS::connectedComponents( seq , g ); } );
#if defined( ADS_HAS_GAPBS )
    addW( S , "openmp" , "gapbs ShiloachVishkin" , [ & ] {
     count = distinct( gapbsSv::ShiloachVishkin( *gg ) ); } );
#endif
    runWorks( bench( "cc" ) , c , arcs , S , r ,
              [ & ] { comp.clear(); count = 0; } ,
              [ & ] { return( comp.empty() ? count == comps
                                           : comp == lab ); } );
   }
   }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TEXTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the LCP array from the suffix array of n random symbols of 4 letters
/// ("dna") and of a text of words of a Zipf law ("words"), in nanoseconds
/// per symbol
void texts( const std::vector< std::size_t > & sizes , int r ,
            unsigned seed , Executors & X )
{
 for( std::size_t n : sizes )
  for( int kind = 0 ; kind < 2 ; ++kind ) {
   std::mt19937_64 g( seed );
   std::string T;
   if( kind == 0 )
    for( std::size_t i = 0 ; i < n ; ++i ) T += "acgt"[ g() % 4 ];
   else {
    std::vector< std::string > W;
    for( int w = 0 ; w < 5000 ; ++w ) {
     std::string s;
     for( int l = 0 ; l < 2 + int( g() % 8 ) ; ++l )
      s += char( 'a' + g() % 26 );
     W.push_back( s );
     }
    while( T.size() < n ) {
     const double x = std::uniform_real_distribution<>( 0 , 1 )( g );
     T += W[ std::min< std::size_t >( 4999 , std::size_t(
                                       std::pow( 5000.0 , x ) ) - 1 ) ];
     T += ' ';
     }
    T.resize( n );
    }
   const auto SA = ADS::suffixArray< std::uint32_t >( T );
   const auto ref = ADS::lcpArrayPhi( T , SA );
   std::vector< std::uint32_t > L;
   Works S;
   onExecutorsW( S , X , "ADS::lcpArrayPhi" , [ & ]( auto & ex ) {
    L = ADS::lcpArrayPhi( ex , T , SA ); } );
   if( threads == 1 )
    addW( S , "seq" , "ADS::lcpArrayPhi (sequential)" , [ & ] {
     L = ADS::lcpArrayPhi( T , SA ); } );
#if defined( ADS_HAS_PARLAY_EXAMPLES )
   addW( S , "parlay" , "parlay lcp" , [ & ] {
    const auto P = lcp( T , SA );
    L.assign( P.begin() , P.end() ); } );
#endif
   runWorks( bench( "lcp" ) , kind ? "words" : "dna" , n , S , r ,
             [ & ] { L.clear(); } , [ & ] { return( L == ref ); } );
   }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- HASHING --------------------------------*/
/*--------------------------------------------------------------------------*/

/// n distinct keys inserted into concurrent hash tables by the threads of
/// the pool of ADS (a parallelFor on blocks of 1024), the tables sized
/// for n keys, then looked up (by contains() where the table has it, as
/// libcuckoo); the same threads for all the tables, so that only the
/// tables differ
void hashing( const std::vector< std::size_t > & sizes , int r ,
              unsigned seed , Executors & X )
{
 if( ! runs( "pool" ) )
  return;
 using K = std::uint64_t;
 for( std::size_t n : sizes ) {
  const auto keys = ADS::bench::keys( n , seed );
  auto & pool = X.pool();
  std::atomic< std::size_t > found{ 0 };
  auto lookups = [ & ]( auto && find ) {
   found = 0;
   ADS::parallelForBlocks( pool , 0 , n , [ & ]( std::size_t lo ,
                                                 std::size_t hi ) {
    std::size_t f = 0;
    for( std::size_t i = lo ; i < hi ; ++i ) f += find( keys[ i ] );
    found.fetch_add( f , std::memory_order_relaxed ); } ); };
  // each table: a factory, the insertion and the lookups
  struct Table {
   std::string name;
   std::function< void( void ) > make , insert , find , drop;
   };
  std::vector< Table > T;
  std::unique_ptr< ADS::ConcurrentCuckooTable< K , K > > a;
  T.push_back( { "ADS::ConcurrentCuckooTable" ,
   [ & ] { a = std::make_unique< ADS::ConcurrentCuckooTable< K , K > >( n );
    } ,
   [ & ] { ADS::parallelFor( pool , 0 , n , [ & ]( std::size_t i ) {
    a->insert( keys[ i ] , K( i ) ); } ); } ,
   [ & ] { lookups( [ & ]( K k ) { return( a->contains( k ) ); } ); } ,
   [ & ] { a.reset(); } } );
#if defined( ADS_HAS_TBB )
  using HM = tbb::concurrent_hash_map< K , K >;
  std::unique_ptr< HM > b;
  T.push_back( { "tbb::concurrent_hash_map" ,
   [ & ] { b = std::make_unique< HM >( n ); } ,
   [ & ] { ADS::parallelFor( pool , 0 , n , [ & ]( std::size_t i ) {
    b->insert( { keys[ i ] , K( i ) } ); } ); } ,
   [ & ] { lookups( [ & ]( K k ) {
    HM::const_accessor c;
    return( b->find( c , k ) ); } ); } , [ & ] { b.reset(); } } );
  using UM = tbb::concurrent_unordered_map< K , K >;
  std::unique_ptr< UM > c;
  T.push_back( { "tbb::concurrent_unordered_map" ,
   [ & ] { c = std::make_unique< UM >( n ); } ,
   [ & ] { ADS::parallelFor( pool , 0 , n , [ & ]( std::size_t i ) {
    c->insert( { keys[ i ] , K( i ) } ); } ); } ,
   [ & ] { lookups( [ & ]( K k ) { return( c->find( k ) != c->end() ); } );
    } , [ & ] { c.reset(); } } );
#endif
#if defined( ADS_HAS_LIBCUCKOO )
  using LC = libcuckoo::cuckoohash_map< K , K >;
  std::unique_ptr< LC > d;
  T.push_back( { "libcuckoo::cuckoohash_map" ,
   [ & ] { d = std::make_unique< LC >( n ); } ,
   [ & ] { ADS::parallelFor( pool , 0 , n , [ & ]( std::size_t i ) {
    d->insert( keys[ i ] , K( i ) ); } ); } ,
   [ & ] { lookups( [ & ]( K k ) { return( d->contains( k ) ); } ); } ,
   [ & ] { d.reset(); } } );
#endif
#if defined( ADS_HAS_PARLAY )
  struct PH {
   using eType = K;
   using kType = K;
   eType empty( void ) { return( ~K( 0 ) ); }
   kType getKey( eType v ) { return( v ); }
   std::size_t hash( kType v ) {
    return( std::size_t( ( v * 0x9E3779B97F4A7C15ull ) >> 3 ) ); }
   int cmp( kType v , kType w ) { return( v > w ? 1 : ( v == w ? 0 : -1 ) ); }
   bool replaceQ( eType , eType ) { return( false ); }
   eType update( eType v , eType ) { return( v ); }
   bool cas( eType * p , eType o , eType nw ) {
    return( std::atomic_ref< K >( *p ).compare_exchange_strong( o , nw ) );
    }
   };
  using PT = parlay::hashtable< PH >;
  std::unique_ptr< PT > e;
  T.push_back( { "parlay::hashtable (keys only)" ,
   [ & ] { e = std::make_unique< PT >( n , PH() ); } ,
   [ & ] { ADS::parallelFor( pool , 0 , n , [ & ]( std::size_t i ) {
    e->insert( keys[ i ] ); } ); } ,
   [ & ] { lookups( [ & ]( K k ) { return( e->find( k ) != ~K( 0 ) ); } );
    } , [ & ] { e.reset(); } } );
#endif
  for( std::size_t i = 0 ; i < T.size() ; ++i )
   for( int aa = 0 ; aa < ( i == 0 ? 2 : 1 ) ; ++aa ) {
    const std::string who = T[ i ].name + ( aa ? " A/A" : "" );
    if( ADS::bench::skip( who ) )
     continue;
    ADS::bench::print( bench( "hash insert" ).c_str() , "uint64" , n , who ,
                       timeRuns( n , [ & ] { T[ i ].drop(); T[ i ].make(); } ,
                                 T[ i ].insert , r ,
                                 [] { return( true ); } ) );
    ADS::bench::print( bench( "hash find" ).c_str() , "uint64" , n , who ,
                       timeRuns( n , [] {} , T[ i ].find , r ,
                                 [ & ] { return( found == n ); } ) );
    T[ i ].drop();
    }
  }
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 // -t threads, -b battery and -w family, then the options of Bench.h
 std::vector< char * > rest{ argv[ 0 ] };
 std::string only;
 for( int i = 1 ; i < argc ; ++i ) {
  const std::string a = argv[ i ];
  if( ( a == "-t" ) && ( i + 1 < argc ) ) threads = std::atoi( argv[ ++i ] );
  else if( ( a == "-w" ) && ( i + 1 < argc ) ) family = argv[ ++i ];
  else if( ( a == "-b" ) && ( i + 1 < argc ) ) only = argv[ ++i ];
  else rest.push_back( argv[ i ] );
  }
 threads = std::max( threads , 1 );
 ADS::bench::Options o( int( rest.size() ) , rest.data() ,
                        { 1000000 , 10000000 } );
#if defined( ADS_HAS_TBB )
 tbb::global_control gc( tbb::global_control::max_allowed_parallelism ,
                         std::size_t( threads ) );
#endif
#if defined( _OPENMP )
 omp_set_num_threads( threads );
#endif
#if defined( ADS_HAS_PARLAY )
 setenv( "PARLAY_NUM_THREADS" , std::to_string( threads ).c_str() , 1 );
#endif
 Executors X;
 if( only.empty() || ( only == "sort" ) ) {
  sorts< std::uint64_t >( "uint64" , o.sizes , o.runs , o.seed , X );
  sorts< double >( "double" , o.sizes , o.runs , o.seed , X );
  std::vector< std::size_t > small;         // the strings, 10 times fewer
  for( auto n : o.sizes )
   small.push_back( n / 10 );
  sorts< std::string >( "string" , small , o.runs , o.seed , X );
  }
 if( only.empty() || ( only == "patterns" ) )
  patternsOn( o.sizes , o.runs , o.seed , X );
 if( only.empty() || ( only == "merge" ) )
  merges( o.sizes , o.runs , o.seed , X );
 if( only.empty() || ( only == "spmv" ) )
  sparse( { 100000 , 1000000 } , o.runs , o.seed , X );
 if( only.empty() || ( only == "gemm" ) )
  dense( o.runs , o.seed , X );
 if( only.empty() || ( only == "fft" ) )
  ffts( o.runs , o.seed , X );
 if( only.empty() || ( only == "graphs" ) )
  graphs( { 16 , 19 } , o.runs , o.seed , X );
 if( only.empty() || ( only == "lcp" ) )
  texts( { 1000000 , 4000000 } , o.runs , o.seed , X );
 if( only.empty() || ( only == "hash" ) )
  hashing( { 1000000 , 4000000 } , o.runs , o.seed , X );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File parallel.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
