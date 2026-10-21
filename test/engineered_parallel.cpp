/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_parallel.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the parallel algorithms, beyond the book: the executors (the
 * pool of ADS, and those of OpenMP, FastFlow and oneTBB when they are
 * compiled in) on empty jobs, jobs of one call, fewer calls than threads,
 * nested jobs and exceptions; the loops, the reductions and the prefix
 * sums against their sequential results, with operations that do not
 * commute; the parallel sorts against std::sort and std::stable_sort on
 * the inputs that stress their classifiers, at the level 3 of the
 * contracts.
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

#include "ADS/parallel/Executor.h"
#include "ADS/parallel/FastFlowExecutor.h"
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
#include "ADS/string/SuffixArray.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <array>
#include <bit>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// f( ex , name ) on each executor: Sequential, pools of 1, 2, 3 and 8
/// threads, and the executors of the libraries compiled in
template< class F >
void onEach( F && f )
{
 Sequential s;
 f( s );
 for( std::size_t p : { 1 , 2 , 3 , 8 } ) {
  ThreadPool pool( p );
  f( pool );
  }
 {
  ThreadPool lazy( 4 , 0 );                 // workers that sleep at once
  f( lazy );
  }
#if defined( _OPENMP )
 OpenMPExecutor omp( 4 );
 f( omp );
#endif
#if defined( ADS_WITH_FASTFLOW )
 FastFlowExecutor ff( 3 , false );
 f( ff );
#endif
#if defined( ADS_WITH_TBB )
 TBBExecutor tbb( 4 );
 f( tbb );
#endif
 }

/// the number of Fibonacci by the fork and join of its recursion
template< class E >
std::uint64_t fib( E & ex , int n )
{
 if( n < 2 )
  return( std::uint64_t( n ) );
 if( n < 12 )
  return( fib( ex , n - 1 ) + fib( ex , n - 2 ) );
 std::uint64_t a = 0 , b = 0;
 parallelInvoke( ex , [ & ] { a = fib( ex , n - 1 ); } ,
                 [ & ] { b = fib( ex , n - 2 ); } );
 return( a + b );
 }

/// a record with a key and the position it came from, to check stability
struct Rec {
 std::uint32_t key;
 std::uint32_t pos;
 bool operator==( const Rec & ) const = default;
 };

struct ByKey {
 bool operator()( const Rec & a , const Rec & b ) const {
  return( a.key < b.key );
  }
 };

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the executors: no call, one call, fewer calls than threads, each call
// once, nested jobs on all the threads, the fork and join of a recursion,
// an exception thrown by one call (and the executor still works after
// it), two threads outside a pool calling it at once
ADS_TEST( "parallel-executors" )
{
 onEach( []( auto & ex ) {
  int calls = 0;
  ex.bulk( 0 , [ & ]( std::size_t ) { ++calls; } );
  CHECK( calls == 0 );
  ex.bulk( 1 , [ & ]( std::size_t i ) { calls += int( i ) + 1; } );
  CHECK( calls == 1 );
  for( std::size_t n : { 2 , 3 , 7 , 100 , 5000 } ) {
   std::vector< std::atomic< int > > hit( n );
   ex.bulk( n , [ & ]( std::size_t i ) {
    hit[ i ].fetch_add( 1 , std::memory_order_relaxed ); } );
   CHECK( std::all_of( hit.begin() , hit.end() , []( auto & h ) {
    return( h.load() == 1 ); } ) );
   }
  // nested: 20 x 30 calls
  std::vector< std::atomic< int > > grid( 600 );
  ex.bulk( 20 , [ & ]( std::size_t i ) {
   ex.bulk( 30 , [ & ]( std::size_t j ) {
    grid[ 30 * i + j ].fetch_add( 1 , std::memory_order_relaxed ); } );
   } );
  CHECK( std::all_of( grid.begin() , grid.end() , []( auto & h ) {
   return( h.load() == 1 ); } ) );
  CHECK( fib( ex , 25 ) == 75025 );
  // an exception: thrown again, then the executor still runs jobs
  bool thrown = false;
  try {
   parallelFor( ex , 0 , 10000 , [ & ]( std::size_t i ) {
    if( i == 7777 ) throw std::runtime_error( "x" ); } , 10 );
   }
  catch( const std::runtime_error & ) {
   thrown = true;
   }
  CHECK( thrown );
  thrown = false;
  try {
   parallelInvoke( ex , [] {} , [] { throw std::logic_error( "y" ); } );
   }
  catch( const std::logic_error & ) {
   thrown = true;
   }
  CHECK( thrown );
  std::atomic< int > after{ 0 };
  ex.bulk( 50 , [ & ]( std::size_t ) { after.fetch_add( 1 ); } );
  CHECK( after.load() == 50 );
  } );
 // two threads outside the pool at once: one at a time takes the place
 // of the caller
 ThreadPool pool( 4 );
 std::atomic< long > sum{ 0 };
 auto user = [ & ] {
  for( int k = 0 ; k < 50 ; ++k )
   pool.bulk( 64 , [ & ]( std::size_t i ) {
    sum.fetch_add( long( i ) , std::memory_order_relaxed ); } );
  };
 std::thread a( user ) , b( user );
 a.join();
 b.join();
 CHECK( sum.load() == 2 * 50 * ( 63 * 64 / 2 ) );
 }

// the loops, the reductions and the scans: empty, one item, fewer items
// than threads, sizes around the tiles; the reduction of doubles equal on
// every executor (the blocks do not depend on the threads); the scans with
// an operation that does not commute (the composition of the affine maps
// x -> a x + b modulo a prime), in place and not, inclusive and exclusive
ADS_TEST( "parallel-patterns" )
{
 std::mt19937_64 g( 7 );
 std::vector< double > D( 300001 );
 for( auto & x : D )
  x = std::ldexp( double( g() >> 11 ) , int( g() % 40 ) - 20 );
 Sequential seq;
 const double ref = parallelReduce( seq , 0 , D.size() , 0.0 ,
                                    [ & ]( std::size_t lo , std::size_t hi ) {
                                     double s = 0;
                                     for( std::size_t i = lo ; i < hi ; ++i )
                                      s += D[ i ];
                                     return( s ); } , std::plus<>() );
 using Affine = std::pair< std::uint64_t , std::uint64_t >;
 constexpr std::uint64_t P = 1000000007;
 auto compose = []( const Affine & f , const Affine & h ) {
  // h after f: x -> h.a ( f.a x + f.b ) + h.b
  return( Affine( h.first * f.first % P ,
                  ( h.first * f.second + h.second ) % P ) );
  };
 onEach( [ & ]( auto & ex ) {
  for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                         std::size_t( 3 ) , std::size_t( 1000 ) ,
                         std::size_t( 4095 ) , std::size_t( 4096 ) ,
                         std::size_t( 4097 ) , std::size_t( 100000 ) ,
                         std::size_t( 262145 ) } ) {
   std::vector< std::uint64_t > A( n );
   for( auto & x : A )
    x = g() % 1000;
   // the loop and the blocks
   std::vector< std::uint64_t > B( n , 0 );
   parallelFor( ex , 0 , n , [ & ]( std::size_t i ) { B[ i ] = 2 * A[ i ]; } ,
                7 );
   bool ok = true;
   for( std::size_t i = 0 ; i < n ; ++i )
    ok = ok && ( B[ i ] == 2 * A[ i ] );
   CHECK( ok );
   std::vector< std::atomic< int > > seen( n );
   parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo , std::size_t hi ) {
    for( std::size_t i = lo ; i < hi ; ++i ) seen[ i ].fetch_add( 1 ); } ,
                      1 );
   CHECK( std::all_of( seen.begin() , seen.end() , []( auto & h ) {
    return( h.load() == 1 ); } ) );
   // the reduction
   const std::uint64_t s = parallelReduce( ex , 0 , n , std::uint64_t( 5 ) ,
    [ & ]( std::size_t lo , std::size_t hi ) {
     return( std::accumulate( A.begin() + std::ptrdiff_t( lo ) ,
                              A.begin() + std::ptrdiff_t( hi ) ,
                              std::uint64_t( 0 ) ) ); } , std::plus<>() ,
    3 );
   CHECK( s == std::accumulate( A.begin() , A.end() ,
                                std::uint64_t( 5 ) ) );
   // the scans of integers, in place and not
   std::vector< std::uint64_t > In( n ) , Ex( n ) , C = A;
   std::inclusive_scan( A.begin() , A.end() , In.begin() , std::plus<>() ,
                        std::uint64_t( 9 ) );
   std::exclusive_scan( A.begin() , A.end() , Ex.begin() ,
                        std::uint64_t( 9 ) );
   const std::uint64_t t = parallelInclusiveScan( ex , C.begin() , n ,
                                                  C.begin() ,
                                                  std::uint64_t( 9 ) );
   CHECK( ( C == In ) && ( t == ( n ? In.back() : 9 ) ) );
   std::vector< std::uint64_t > E( n );
   const std::uint64_t u = parallelExclusiveScan( ex , A.begin() , n ,
                                                  E.begin() ,
                                                  std::uint64_t( 9 ) );
   CHECK( ( E == Ex ) && ( u == ( n ? In.back() : 9 ) ) );
   // the scan of the affine maps, which do not commute
   std::vector< Affine > M( n ) , R( n ) , S( n );
   for( auto & m : M )
    m = Affine( g() % P , g() % P );
   const Affine id( 1 , 0 );
   Affine run = id;
   for( std::size_t i = 0 ; i < n ; ++i )
    R[ i ] = run = compose( run , M[ i ] );
   parallelInclusiveScan( ex , M.begin() , n , S.begin() , id , compose );
   CHECK( S == R );
   }
  // the same sum of doubles on every executor
  const double s = parallelReduce( ex , 0 , D.size() , 0.0 ,
                                   [ & ]( std::size_t lo , std::size_t hi ) {
                                    double r = 0;
                                    for( std::size_t i = lo ; i < hi ; ++i )
                                     r += D[ i ];
                                    return( r ); } , std::plus<>() );
  CHECK( s == ref );
  } );
 }

// the parallel sorts against std::sort and std::stable_sort: sizes below,
// at and above kParallelSort; integers, doubles with -0 and +0, strings,
// records by key (stability); random keys, all equal, two values, few
// distinct (the buckets of equality), sorted, reversed, strictly reversed
// with ties (which the stable sort may not reverse), organ pipe, skewed
// (a power law: one bucket of the radix holds most items), the extremes
// of the type
ADS_TEST( "parallel-sort" )
{
 std::mt19937_64 g( 11 );
 const std::size_t K = kParallelSort;
 const std::vector< std::size_t > sizes = { 0 , 1 , 2 , 100 , K - 1 , K ,
                                            K + 1 , 3 * K + 17 , 200000 };
 auto keys = [ & ]( int pattern , std::size_t n ) {
  std::vector< std::uint64_t > k( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   switch( pattern ) {
    case( 0 ): k[ i ] = g(); break;
    case( 1 ): k[ i ] = 42; break;
    case( 2 ): k[ i ] = g() & 1 ? std::numeric_limits< std::uint64_t >::max()
                                : 0; break;
    case( 3 ): k[ i ] = g() % 16; break;
    case( 4 ): k[ i ] = i; break;
    case( 5 ): k[ i ] = n - i; break;
    case( 6 ): k[ i ] = ( n - i ) / 3; break;
    case( 7 ): k[ i ] = std::min( i , n - 1 - i ); break;
    case( 8 ): k[ i ] = std::uint64_t( std::pow( double( g() % 1000000 + 1 ) ,
                                                 -2.0 ) * 1e18 ); break;
    default: k[ i ] = g() % 3 ? std::uint64_t( 1 ) << 63 : g(); break;
    }
  return( k );
  };
 onEach( [ & ]( auto & ex ) {
  for( std::size_t n : sizes )
   for( int pattern = 0 ; pattern < 10 ; ++pattern ) {
    const auto k = keys( pattern , n );
    // 64-bit integers, signed and not
    {
     std::vector< std::uint64_t > A = k , B = k;
     ADS::sort( ex , A );
     std::sort( B.begin() , B.end() );
     CHECK( A == B );
     std::vector< std::int64_t > S( n ) , T;
     for( std::size_t i = 0 ; i < n ; ++i )
      S[ i ] = std::int64_t( k[ i ] );
     T = S;
     ADS::stableSort( ex , S );
     std::sort( T.begin() , T.end() );
     CHECK( S == T );
     }
    // doubles with -0 and +0 among them: the stable sort keeps their
    // order, as std::stable_sort
    {
     std::vector< double > A( n );
     for( std::size_t i = 0 ; i < n ; ++i )
      A[ i ] = ( k[ i ] % 5 == 0 ) ? ( i % 2 ? -0.0 : 0.0 )
                                   : double( std::int64_t( k[ i ] ) ) / 7;
     std::vector< double > B = A , C = A , D = A;
     ADS::stableSort( ex , A );
     std::stable_sort( B.begin() , B.end() );
     bool same = true;
     for( std::size_t i = 0 ; i < n ; ++i )
      same = same && ( std::bit_cast< std::uint64_t >( A[ i ] ) ==
                       std::bit_cast< std::uint64_t >( B[ i ] ) );
     CHECK( same );
     ADS::sort( ex , C , std::greater<>() );
     std::sort( D.begin() , D.end() , std::greater<>() );
     CHECK( C == D );
     }
    // records by key: the stable sort equals std::stable_sort, the other
    // has the same keys in order and the same records
    {
     std::vector< Rec > A( n );
     for( std::size_t i = 0 ; i < n ; ++i )
      A[ i ] = Rec{ std::uint32_t( k[ i ] >> 40 ) , std::uint32_t( i ) };
     std::vector< Rec > B = A , C = A;
     ADS::stableSort( ex , A , ByKey() );
     std::stable_sort( B.begin() , B.end() , ByKey() );
     CHECK( A == B );
     ADS::sort( ex , C , ByKey() );
     CHECK( std::is_sorted( C.begin() , C.end() , ByKey() ) );
     std::sort( C.begin() , C.end() , []( const Rec & a , const Rec & b ) {
      return( a.pos < b.pos ); } );
     bool same = true;
     for( std::size_t i = 0 ; i < n ; ++i )
      same = same && ( C[ i ].pos == i ) &&
             ( C[ i ].key == std::uint32_t( k[ i ] >> 40 ) );
     CHECK( same );
     }
    // strings, on the smaller sizes
    if( n <= K + 1 ) {
     std::vector< std::string > A( n );
     for( std::size_t i = 0 ; i < n ; ++i )
      A[ i ] = "k" + std::to_string( k[ i ] % 100000 );
     std::vector< std::string > B = A;
     ADS::sort( ex , A );
     std::sort( B.begin() , B.end() );
     CHECK( A == B );
     }
    }
  } );
 }

// the parallel merge against the sequential one: no range, empty ranges,
// one range, many short ranges, few long ones with many equal items
// across the ranges (the order of the ranges on ties), records whose
// position tells the range they came from; the splits sum to their rank
ADS_TEST( "parallel-merge" )
{
 std::mt19937_64 g( 3 );
 onEach( [ & ]( auto & ex ) {
  for( const auto & shape : std::vector< std::pair< std::size_t ,
                                                    std::size_t > >{
        { 0 , 0 } , { 1 , 0 } , { 1 , 100000 } , { 5 , 0 } , { 2 , 40000 } ,
        { 7 , 30000 } , { 64 , 3000 } , { 3 , 70000 } } ) {
   const std::size_t k = shape.first , len = shape.second;
   for( std::uint32_t mod : { 4u , 1000000u } ) {
    std::vector< std::vector< Rec > > S( k );
    std::size_t N = 0;
    for( std::size_t i = 0 ; i < k ; ++i ) {
     const std::size_t m = len ? g() % ( 2 * len ) : 0;
     for( std::size_t j = 0 ; j < m ; ++j )
      S[ i ].push_back( Rec{ std::uint32_t( g() % mod ) ,
                             std::uint32_t( i ) } );
     std::sort( S[ i ].begin() , S[ i ].end() , ByKey() );
     N += m;
     }
    std::vector< std::pair< Rec * , Rec * > > R;
    for( auto & v : S )
     R.emplace_back( v.data() , v.data() + v.size() );
    std::vector< Rec > A( N ) , B( N );
    multiwayMerge( R , A.begin() , ByKey() );
    CHECK( multiwayMerge( ex , R , B.begin() , ByKey() ) == B.end() );
    CHECK( A == B );
    for( std::size_t r : { std::size_t( 0 ) , N / 3 , N } ) {
     const auto s = multisequenceSplit( R , r , ByKey() );
     CHECK( std::accumulate( s.begin() , s.end() , std::size_t( 0 ) ) ==
            r );
     }
    }
   }
  } );
 }

// the products of sparse and dense matrices against the sequential ones,
// bit for bit: an empty matrix, a single row, rows of very different
// lengths (one row with half the nonzeros, empty rows), a square random
// matrix; the products of dense matrices below and above the threshold
// of the small products, by rows and by columns, with alpha and beta
ADS_TEST( "parallel-numeric" )
{
 std::mt19937_64 g( 5 );
 auto value = [ & ] { return( double( std::int64_t( g() % 2001 ) - 1000 ) /
                              7.0 ); };
 std::vector< CsrMatrix > M;
 M.emplace_back( 0 , 5 );
 for( const auto & dims : std::vector< std::pair< std::size_t ,
                                                  std::size_t > >{
       { 1 , 50000 } , { 60000 , 300 } , { 20000 , 20000 } } ) {
  const std::size_t m = dims.first , n = dims.second;
  std::vector< CsrMatrix::Triplet > T;
  for( std::size_t e = 0 ; e < 4 * std::max( m , n ) ; ++e )
   T.emplace_back( g() % m , g() % n , value() );
  if( m > 1000 )                           // a row with half of them
   for( std::size_t e = 0 ; e < 4 * std::max( m , n ) ; ++e )
    T.emplace_back( m / 2 , g() % n , value() );
  M.emplace_back( m , n , T );
  }
 std::vector< DenseMatrix< double > > DA , DB;
 for( const auto & d : std::vector< std::array< std::size_t , 3 > >{
       { 5 , 7 , 3 } , { 100 , 120 , 90 } , { 600 , 500 , 400 } ,
       { 70 , 900 , 300 } , { 1000 , 64 , 300 } } ) {
  DenseMatrix< double > A( d[ 0 ] , d[ 2 ] ) , B( d[ 2 ] , d[ 1 ] );
  for( std::size_t i = 0 ; i < d[ 0 ] ; ++i )
   for( std::size_t j = 0 ; j < d[ 2 ] ; ++j ) A( i , j ) = value();
  for( std::size_t i = 0 ; i < d[ 2 ] ; ++i )
   for( std::size_t j = 0 ; j < d[ 1 ] ; ++j ) B( i , j ) = value();
  DA.push_back( A );
  DB.push_back( B );
  }
 onEach( [ & ]( auto & ex ) {
  for( const auto & A : M ) {
   std::vector< double > x( A.cols() );
   for( auto & v : x ) v = value();
   CHECK( multiply( ex , A , x ) == A.multiply( x ) );
   if( A.rows() > 0 ) {
    const SellMatrix<> S( A );
    CHECK( multiply( ex , S , x ) == S.multiply( x ) );
    }
   }
  for( std::size_t t = 0 ; t < DA.size() ; ++t ) {
   const auto C = multiply( ex , DA[ t ] , DB[ t ] ) ,
              D = multiply( DA[ t ] , DB[ t ] );
   bool same = true;
   for( std::size_t i = 0 ; i < C.rows() ; ++i )
    for( std::size_t j = 0 ; j < C.cols() ; ++j )
     same = same && ( C( i , j ) == D( i , j ) );
   CHECK( same );
   // alpha and beta on a C already there
   auto X = D , Y = D;
   const std::size_t m = DA[ t ].rows() , n = DB[ t ].cols() ,
                     k = DA[ t ].cols();
   gemm( ex , m , n , k , 0.5 , DA[ t ].data() , k , DB[ t ].data() , n ,
         -2.0 , X.data() , n );
   gemm( m , n , k , 0.5 , DA[ t ].data() , k , DB[ t ].data() , n , -2.0 ,
         Y.data() , n );
   CHECK( std::equal( X.data() , X.data() + m * n , Y.data() ) );
   }
  } );
 }

// the parallel transform against the sequential one and the definition:
// sizes below and above the threshold, odd and even powers of 2, the
// inverse giving back the input, an impulse (all ones out)
ADS_TEST( "parallel-fft" )
{
 std::mt19937_64 g( 13 );
 std::uniform_real_distribution< double > u( -1 , 1 );
 onEach( [ & ]( auto & ex ) {
  for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                         std::size_t( 8 ) , kParallelFFT / 2 ,
                         kParallelFFT , 2 * kParallelFFT ,
                         std::size_t( 1 ) << 18 } ) {
   std::vector< Complex > a( n );
   for( auto & z : a ) z = Complex( u( g ) , u( g ) );
   std::vector< Complex > b = a , c = a;
   fft( ex , b , false );
   fft( c , false );
   double err = 0 , norm = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    err = std::max( err , std::abs( b[ i ] - c[ i ] ) );
    norm = std::max( norm , std::abs( c[ i ] ) );
    }
   CHECK( err <= 1e-12 * std::max( norm , 1.0 ) );
   fft( ex , b , true );
   err = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    err = std::max( err , std::abs( b[ i ] - a[ i ] ) );
   CHECK( err <= 1e-12 );
   if( n > 0 ) {
    std::vector< Complex > d( n );
    d[ 0 ] = 1;
    fft( ex , d , false );
    CHECK( std::all_of( d.begin() , d.end() , []( Complex z ) {
     return( std::abs( z - Complex( 1 ) ) < 1e-12 ); } ) );
    }
   }
  } );
 }

// the visit in breadth and the components against a sequential visit by a
// queue: a graph with no arc, a path (one level per node: the visit stays
// from the top down), a star (one level with all the nodes: from the
// bottom up), a random graph of small diameter in several components,
// oriented (with and without its transpose) and not, a source isolated
ADS_TEST( "parallel-graph" )
{
 std::mt19937_64 g( 17 );
 using Graph = StaticGraph<>;
 std::vector< std::pair< int , std::vector< std::pair< int , int > > > > I;
 I.push_back( { 1000 , {} } );
 {
  std::vector< std::pair< int , int > > E;
  for( int i = 0 ; i + 1 < 50000 ; ++i ) E.emplace_back( i , i + 1 );
  I.push_back( { 50000 , E } );
  }
 {
  std::vector< std::pair< int , int > > E;
  for( int i = 1 ; i < 60000 ; ++i ) E.emplace_back( 0 , i );
  I.push_back( { 60000 , E } );
  }
 {
  const int n = 100000;
  std::vector< std::pair< int , int > > E;
  for( int e = 0 ; e < 4 * n ; ++e ) {
   // three blocks of nodes with no arc between them, and isolated nodes
   const int b = int( g() % 3 ) , lo = b * 30000;
   const int u = lo + int( g() % 30000 ) , v = lo + int( g() % 30000 );
   if( u != v ) E.emplace_back( u , v );
   }
  I.push_back( { n , E } );
  }
 auto reference = []( const Graph & G , int s ) {
  std::vector< int > d( std::size_t( G.n() ) , -1 );
  std::queue< int > Q;
  d[ std::size_t( s ) ] = 0;
  Q.push( s );
  while( ! Q.empty() ) {
   const int u = Q.front();
   Q.pop();
   for( int v : G.out( u ) )
    if( d[ std::size_t( v ) ] < 0 ) {
     d[ std::size_t( v ) ] = d[ std::size_t( u ) ] + 1;
     Q.push( v );
     }
   }
  return( d );
  };
 onEach( [ & ]( auto & ex ) {
  for( const auto & [ n , E ] : I )
   for( bool directed : { false , true } ) {
    const Graph G( n , E , directed ) , GT = G.transpose();
    for( int s : { 0 , n / 2 , n - 1 } ) {
     const auto d = reference( G , s );
     CHECK( bfsDistances( ex , G , s , directed ? &GT : nullptr ) == d );
     if( directed )
      CHECK( bfsDistances( ex , G , s ) == d );
     }
    // the least node of each component, by the visits of the unoriented
    // graph
    const Graph U( n , E , false );
    std::vector< int > label( std::size_t( n ) , -1 );
    for( int v = 0 ; v < n ; ++v )
     if( label[ std::size_t( v ) ] < 0 ) {
      const auto d = reference( U , v );
      for( int w = 0 ; w < n ; ++w )
       if( d[ std::size_t( w ) ] >= 0 ) label[ std::size_t( w ) ] = v;
      }
    CHECK( connectedComponents( ex , G ) == label );
    }
  } );
 }

// the parallel LCP array against the sequential one: texts shorter than
// the threshold, random texts on 2 and 26 letters, a periodic text and a
// text of one letter (the longest lcp), on bytes and on symbols
ADS_TEST( "parallel-lcp" )
{
 std::mt19937_64 g( 19 );
 std::vector< std::string > texts = { "" , "a" , "banana" };
 for( int sigma : { 2 , 26 } ) {
  std::string t( 100000 , 'a' );
  for( auto & c : t ) c = char( 'a' + g() % unsigned( sigma ) );
  texts.push_back( t );
  }
 {
  std::string t;
  while( t.size() < 70000 ) t += "abracadabra";
  texts.push_back( t );
  texts.push_back( std::string( 40000 , 'z' ) );
  }
 onEach( [ & ]( auto & ex ) {
  for( const auto & t : texts ) {
   const auto SA = suffixArraySais( t );
   CHECK( lcpArray( ex , t , SA ) == lcpArray( t , SA ) );
   const Symbols S = toSymbols( t );
   CHECK( lcpArray( ex , S , SA ) == lcpArray( S , SA ) );
   }
  } );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_parallel.cpp --------------------*/
/*--------------------------------------------------------------------------*/
