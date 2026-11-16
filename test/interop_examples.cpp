/*--------------------------------------------------------------------------*/
/*------------------------ File interop_examples.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The examples of appendix C of the manual, Working with other libraries,
 * run: each test holds the code of one example as the manual prints it
 * (`declcheck.py --tests` checks that it does), followed by the checks of
 * its result. The examples use the standard library, Boost.Graph,
 * Boost.Heap, Boost.Unordered, LEMON, GMP, a CBLAS, oneTBB, OpenMP, the
 * parallel algorithms of the standard library, stdexec, Taskflow and Kokkos;
 * a test whose library is not compiled in passes with nothing to check, and
 * says so. Those of MPI and of Boost.MPI are in interop_examples_mpi.cpp.
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

#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/std/FlatMap.h"
#include "ADS/interop/std/Pmr.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/TBBExecutor.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/sort/Sort.h"
#include "ADS/tree/BTreeMap.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <memory_resource>
#include <queue>
#include <random>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <version>

#if defined( ADS_WITH_BOOST )
 #include "ADS/interop/boost/Dijkstra.h"
 #include "ADS/interop/boost/FromBGL.h"
 #include "ADS/interop/boost/Graph.h"
 #include "ADS/interop/boost/Heap.h"

 #include <boost/graph/adjacency_list.hpp>
 #include <boost/graph/dijkstra_shortest_paths.hpp>
 #include <boost/heap/fibonacci_heap.hpp>
 #include <boost/unordered/unordered_flat_map.hpp>
#endif

#if defined( ADS_WITH_LEMON )
 #include "ADS/interop/lemon/Digraphs.h"
 #include "ADS/interop/lemon/Heaps.h"
 #include "ADS/interop/lemon/Observers.h"

 #include <lemon/dijkstra.h>
 #include <lemon/list_graph.h>
 #include <lemon/preflow.h>
#endif

#if defined( ADS_WITH_GMP ) && __has_include( <gmpxx.h> )
 #include <gmpxx.h>
 #define ADS_EXAMPLES_GMPXX 1
#else
 #define ADS_EXAMPLES_GMPXX 0
#endif

#if defined( ADS_WITH_BLAS ) && __has_include( <cblas.h> )
 #include <cblas.h>
 #define ADS_EXAMPLES_CBLAS 1
#else
 #define ADS_EXAMPLES_CBLAS 0
#endif

#if defined( ADS_WITH_TBB )
 #include <tbb/parallel_for.h>
 #include <tbb/task_arena.h>
#endif

#if defined( ADS_WITH_STDPAR )
 #include "ADS/interop/parallel/StdParExecutor.h"
#endif
#if defined( ADS_WITH_STDEXEC )
 #include "ADS/interop/parallel/SenderExecutor.h"

 #include <exec/static_thread_pool.hpp>
#endif
#if defined( ADS_WITH_TASKFLOW )
 #include "ADS/interop/parallel/TaskflowExecutor.h"
#endif
#if defined( ADS_WITH_KOKKOS )
 #include "ADS/interop/parallel/KokkosExecutor.h"
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// a message for the tests whose library is not compiled in
[[maybe_unused]] void notCompiled( const char * what )
{
 std::cout << "  (" << what << " not compiled in: nothing to check)"
	   << std::endl;
 }

/// whether A holds the items of a vector of 1000000 random numbers filled
/// as the examples fill it, in order
[[maybe_unused]] bool sortedSample( const std::vector< double > & A )
{
 std::vector< double > B( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : B )
  x = double( r() % 1000003 );
 std::sort( B.begin() , B.end() );
 return( A == B );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------ THE STANDARD LIBRARY ----------------------------*/
/*--------------------------------------------------------------------------*/

// section C.1: std::flat_map on two TieredVector
ADS_TEST( "std-flat-map" )
{
#if defined( __cpp_lib_flat_map )
 ADS::TieredFlatMap< std::string , int > m;  // was std::flat_map< ... >
 m[ "pear" ] = 3;
 m[ "apple" ] = 5;
 m.erase( "pear" );
 CHECK( ( m.size() == 1 ) && ( m.begin()->first == "apple" ) &&
	( m[ "apple" ] == 5 ) );
#else
 notCompiled( "std::flat_map of C++23" );
#endif
 }

// section C.1: the algorithms of std::ranges on a dictionary of ADS
ADS_TEST( "std-ranges-dictionary" )
{
 ADS::BTreeMap< int , int > b;
 for( int k = 0 ; k < 100 ; ++k )
  b.insert( k , ( k * 37 ) % 101 );
 const auto even = std::ranges::count_if(
   b , []( const auto & x ) { return( x.first % 2 == 0 ); } );
 const auto top = std::ranges::max_element(
   b , {} , []( const auto & x ) { return( x.second ); } );
 // even is 50, and ( *top ).first is 30, whose value is 100
 CHECK( ( even == 50 ) && ( ( *top ).first == 30 ) &&
	( ( *top ).second == 100 ) );
 }

// section C.1: the sorts of ADS on the ranges of the standard library, the
// memory resource of a pool, a queue of the standard library as frontier
ADS_TEST( "std-sorts-pmr-queues" )
{
 std::deque< int > d = { 5 , 3 , 9 , 1 , 7 };
 ADS::sort( d );  // 1 3 5 7 9
 std::vector< int > v = { 9 , 8 , 7 , 6 , 5 , 4 };
 ADS::sort( std::span< int >( v ).subspan( 2 , 3 ) );  // 9 8 5 6 7 4
 CHECK( std::ranges::is_sorted( d ) &&
	( v == std::vector< int >{ 9 , 8 , 5 , 6 , 7 , 4 } ) );
 ADS::PoolResource<> pool;
 ADS::PmrResource<> mr( pool );       // a std::pmr::memory_resource
 std::pmr::map< int , int > m( &mr );  // its nodes in the pool
 for( int k = 0 ; k < 1000 ; ++k )
  m[ k ] = k * k;
 CHECK( ( m.size() == 1000 ) && ( m[ 30 ] == 900 ) );
 const ADS::StaticGraph< double > g(
   4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } , { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
 using P = std::pair< double , int >;
 std::priority_queue< P , std::vector< P > , std::greater< P > > q;
 ADS::PriorityFrontier S( q , g.n() );           // lazy: no decrease-key
 const auto T = ADS::shortestPaths( g , 0 , S );  // T.d is { 0 , 2 , 3 , 4 }
 CHECK( ( T.d == std::vector< double >{ 0. , 2. , 3. , 4. } ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ BOOST.GRAPH -------------------------------*/
/*--------------------------------------------------------------------------*/

// section C.2: Dijkstra's algorithm of Boost.Graph on a StaticGraph
ADS_TEST( "bgl-dijkstra-on-ads" )
{
#if defined( ADS_WITH_BOOST )
 const ADS::StaticGraph< double > g(
   4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } , { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
 std::vector< double > d( num_vertices( g ) );
 std::vector< int > p( num_vertices( g ) );
 boost::dijkstra_shortest_paths(
   g , 0 ,
   boost::weight_map( get( boost::edge_weight , g ) )
     .distance_map( d.data() )
     .predecessor_map( p.data() ) );  // d is { 0 , 2 , 3 , 4 }
 CHECK( ( d == std::vector< double >{ 0. , 2. , 3. , 4. } ) );
 CHECK( ( p[ 3 ] == 2 ) && ( p[ 2 ] == 1 ) );
#else
 notCompiled( "Boost" );
#endif
 }

// section C.2: the algorithm of Johnson of ADS on a graph of Boost.Graph,
// and a heap of ADS in the visit of Boost.Graph
ADS_TEST( "bgl-ads-on-boost" )
{
#if defined( ADS_WITH_BOOST )
 using AL =
   boost::adjacency_list< boost::vecS , boost::vecS , boost::directedS ,
			  boost::no_property ,
			  boost::property< boost::edge_weight_t , double > >;
 AL al( 4 );
 add_edge( 0 , 1 , 2. , al );
 add_edge( 1 , 2 , 1. , al );
 add_edge( 0 , 2 , 4. , al );
 add_edge( 2 , 3 , 1. , al );
 const auto w = get( boost::edge_weight , al );
 const ADS::bgl::FromBGL< AL , decltype( w ) > g( al , w );  // no copy
 const auto T = ADS::johnson( g , 0 );  // T.d is { 0 , 2 , 3 , 4 }
 CHECK( ( T.d == std::vector< double >{ 0. , 2. , 3. , 4. } ) );
 std::vector< double > d( 4 );
 std::vector< int > p( 4 );
 ADS::IndexedDaryHeap< double , 4 > q( 4 );  // was the 4-ary heap of Boost
 ADS::bgl::dijkstraShortestPaths( al , 0 , q , p.data() , d.data() );
 CHECK( d == T.d );
#else
 notCompiled( "Boost" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*----------------------- BOOST.HEAP AND BOOST.UNORDERED -------------------*/
/*--------------------------------------------------------------------------*/

// section C.3: a heap of Boost.Heap as the frontier of the shortest paths
// of ADS, and the hash of ADS in a table of Boost.Unordered
ADS_TEST( "boost-heap-unordered" )
{
#if defined( ADS_WITH_BOOST )
 const ADS::StaticGraph< double > g(
   4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } , { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
 using P = std::pair< double , int >;
 using H =
   boost::heap::fibonacci_heap< P ,
				boost::heap::compare< std::greater< P > > >;
 H h;
 ADS::PriorityFrontier S( h , g.n() );           // native decrease-key
 const auto T = ADS::shortestPaths( g , 0 , S );  // T.d is { 0 , 2 , 3 , 4 }
 CHECK( ( T.d == std::vector< double >{ 0. , 2. , 3. , 4. } ) );
 boost::unordered_flat_map< std::string , int , ADS::FastHash< std::string > >
   m;  // the hash of ADS, avalanching: no mixing by Boost
 m[ "pear" ] = 3;
 m[ "apple" ] = 5;
 CHECK( ( m.size() == 2 ) && ( m[ "pear" ] == 3 ) );
#else
 notCompiled( "Boost" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- LEMON ----------------------------------*/
/*--------------------------------------------------------------------------*/

// section C.4: LEMON's Dijkstra on a heap of ADS, and the shortest paths of
// ADS on the same digraph of LEMON
ADS_TEST( "lemon-dijkstra-ads-heap" )
{
#if defined( ADS_WITH_LEMON )
 using GR = lemon::ListDigraph;
 GR g;
 GR::ArcMap< int > l( g );
 const GR::Node s = g.addNode() , a = g.addNode() , t = g.addNode();
 l.set( g.addArc( s , a ) , 2 );
 l.set( g.addArc( a , t ) , 1 );
 l.set( g.addArc( s , t ) , 4 );
 using H = ADS::lemon_interop::DaryHeap< int , GR::NodeMap< int > >;
 lemon::Dijkstra< GR , GR::ArcMap< int > >::SetStandardHeap< H >::Create d( g ,
									   l );
 d.run( s );  // d.dist( t ) is 3
 CHECK( d.dist( t ) == 3 );
 const ADS::lemon_interop::GraphView< GR , GR::ArcMap< int > > view( g , l );
 const auto T = ADS::johnson( view , g.id( s ) );  // T.d[ g.id( t ) ] is 3
 CHECK( T.d[ std::size_t( g.id( t ) ) ] == 3 );
#else
 notCompiled( "LEMON" );
#endif
 }

// section C.4: LEMON's Preflow on a StaticGraph, and a LemonTap that tells
// the changes of a digraph of LEMON to an observer of ADS
ADS_TEST( "lemon-preflow-tap" )
{
#if defined( ADS_WITH_LEMON )
 using V = ADS::lemon_interop::StaticDigraphView< long long >;
 const ADS::StaticGraph< long long > g(
   4 , { { 0 , 1 , 3 } , { 0 , 2 , 2 } , { 1 , 3 , 2 } , { 2 , 3 , 3 } } );
 const V lg( g );  // no copy of the rows
 const auto c = lg.weights();
 lemon::Preflow< V , V::WeightMap > pf( lg , c , lg.nodeFromId( 0 ) ,
				       lg.nodeFromId( 3 ) );
 pf.run();  // pf.flowValue() is 4
 CHECK( pf.flowValue() == 4 );
 using GR = lemon::ListDigraph;
 using Tap = ADS::lemon_interop::LemonTap< GR , GR::Arc >;
 struct Count
 {
  int live = 0;  ///< the arcs in the digraph
  void on( const Tap::Insert & ) { ++live; }
  void on( const Tap::Erase & ) noexcept { --live; }
  };
 GR h;
 Tap tap( h );
 Count k;
 tap.attach( k );
 const GR::Node u = h.addNode() , v = h.addNode();
 const GR::Arc a = h.addArc( u , v );
 h.addArc( v , u );
 h.erase( a );  // k.live is 1
 CHECK( k.live == 1 );
#else
 notCompiled( "LEMON" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ GMP AND BLAS ------------------------------*/
/*--------------------------------------------------------------------------*/

// section C.5: the numbers of ADS to and from those of GMP
ADS_TEST( "gmp-numbers" )
{
#if ADS_EXAMPLES_GMPXX
 const mpz_class m( "-98765432109876543210987654321" );
 const ADS::BigInteger x( m );                        // no string between
 const mpz_class y = ( x * x ).toGmp< mpz_class >();  // the same, in GMP
 CHECK( ( y == m * m ) && ( x.toString() == m.get_str() ) );
#else
 notCompiled( "GMP" );
#endif
 }

// section C.6: a CBLAS on the matrices of ADS
ADS_TEST( "blas-dense-matrix" )
{
#if ADS_EXAMPLES_CBLAS
 ADS::DenseMatrix< double > A( 2 , 3 , 1. ) , B( 3 , 2 , 2. ) , C( 2 , 2 );
 cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , 2 , 2 , 3 , 1. ,
	      A.data() ,
	      3 , B.data() , 2 , 0. , C.data() , 2 );
 const auto D = ADS::multiply( A , B );  // on cblas_dgemm() too
 CHECK( ( C( 0 , 0 ) == 6. ) && ( C( 1 , 1 ) == 6. ) && ( D( 0 , 1 ) == 6. ) );
#else
 notCompiled( "a CBLAS" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE RUNTIMES OF A PROGRAM ----------------------*/
/*--------------------------------------------------------------------------*/

// section C.7: the parallel sort of ADS in an arena of oneTBB, and a
// concurrent structure of ADS in a loop of oneTBB
ADS_TEST( "tbb-arena-sort" )
{
#if defined( ADS_WITH_TBB )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 tbb::task_arena arena( 4 );  // the arena of the program
 ADS::TBBExecutor ex( arena );
 ADS::sort( ex , A );  // on the threads of the arena
 CHECK( sortedSample( A ) );
 ADS::ConcurrentUnionFind uf( 1000 );
 tbb::parallel_for( 0 , 999 , [ & ]( int i ) {
  if( i % 2 == 0 )
   ( void )uf.unite( i , i + 1 );
  } );  // the sets { 0 , 1 } , { 2 , 3 } , ...
 CHECK( ( uf.find( 0 ) == uf.find( 1 ) ) && ( uf.find( 1 ) != uf.find( 2 ) ) );
#else
 notCompiled( "oneTBB" );
#endif
 }

// section C.8: the parallel sort of ADS inside a region of OpenMP
ADS_TEST( "openmp-region-sort" )
{
#if defined( _OPENMP ) && ( _OPENMP >= 201511 )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 #pragma omp parallel num_threads( 4 )
 #pragma omp single
 {
  ADS::OpenMPExecutor ex;  // the team of the region
  ADS::sort( ex , A );      // by the tasks of the team
  }
 CHECK( sortedSample( A ) );
#else
 notCompiled( "OpenMP 4.5" );
#endif
 }

// section C.9: the parallel sort of ADS on the parallel algorithms of the
// standard library
ADS_TEST( "stdpar-sort" )
{
#if defined( ADS_WITH_STDPAR )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 ADS::StdParExecutor<> ex( 4 );  // std::execution::par
 ADS::sort( ex , A );
 CHECK( sortedSample( A ) );
#else
 notCompiled( "the parallel algorithms of the standard library" );
#endif
 }

// section C.10: the parallel sort and a reduction of ADS on a scheduler of
// stdexec
ADS_TEST( "stdexec-sort" )
{
#if defined( ADS_WITH_STDEXEC )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 exec::static_thread_pool pool( 4 );  // the pool of the program
 ADS::SenderExecutor ex( pool.get_scheduler() , 4 );
 ADS::sort( ex , A );
 const double s = ADS::parallelReduce(
   ex , 0 , A.size() , 0.0 ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    double t = 0;
    for( std::size_t i = lo ; i < hi ; ++i )
     t += A[ i ];
    return( t );
    } ,
   std::plus<>() );
 CHECK( sortedSample( A ) && ( s > 0 ) );
#else
 notCompiled( "stdexec" );
#endif
 }

// section C.11: the parallel sort of ADS on the workers of Taskflow
ADS_TEST( "taskflow-sort" )
{
#if defined( ADS_WITH_TASKFLOW )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 tf::Executor workers( 4 );  // the executor of the program
 ADS::TaskflowExecutor ex( workers );
 ADS::sort( ex , A );
 CHECK( sortedSample( A ) );
#else
 notCompiled( "Taskflow" );
#endif
 }

// section C.12: the parallel sort of ADS on the space of the host of Kokkos
ADS_TEST( "kokkos-sort" )
{
#if defined( ADS_WITH_KOKKOS )
 std::vector< double > A( 1000000 );
 std::mt19937 r( 1 );
 for( double & x : A )
  x = double( r() % 1000003 );
 Kokkos::ScopeGuard guard;  // Kokkos started, as by the program
 ADS::KokkosExecutor<> ex;  // on Kokkos::DefaultHostExecutionSpace
 ADS::sort( ex , A );
 CHECK( sortedSample( A ) );
#else
 notCompiled( "Kokkos" );
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
/*--------------------- End File interop_examples.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
