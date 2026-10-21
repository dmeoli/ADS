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
 *   parlay::parallel_for and the ParallelFor of FastFlow).
 * Each cell is the median of r runs, in nanoseconds per item, of the work
 * on a fresh copy of the input, the copy excluded; a result different from
 * the sequential one gets -1. The benchmark of a cell is "parallel
 * <algorithm> t<t>", so that the cells of each number of threads make a
 * table of their own; its first structure is run twice, the second time as
 * "A/A".
 *
 *   parallel [ -r runs ] [ -s seed ] [ -t threads ] [ -b sort|patterns ]
 *            [ -w family ] n1 n2 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/sort/Sort.h"

#if defined( ADS_HAS_TBB )
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
 #include <parlay/parallel.h>
 #include <parlay/primitives.h>
 #define ADS_HAS_PARLAY 1
#endif

#include <algorithm>
#include <bit>
#include <chrono>
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
/// or -1 if ok( copy ) is false after a run (checked untimed)
template< class V , class W , class K >
double timeIt( const V & A , W && work , int r , K && ok )
{
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
  ADS::bench::print( b.c_str() , c , A.size() , S[ i ].first ,
                     timeIt( A , S[ i ].second , r , ok ) );
  if( i == 0 )
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
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File parallel.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
