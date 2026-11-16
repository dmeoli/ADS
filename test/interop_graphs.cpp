/*--------------------------------------------------------------------------*/
/*------------------------- File interop_graphs.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the adapters of the libraries of graphs and of matrices, beyond
 * the book: the matrices of ADS as matrices of Eigen, the rows of a
 * `StaticGraph` partitioned by METIS and by KaHIP, the graphs and the
 * sparse matrices of ADS as matrices of SuiteSparse:GraphBLAS, and the
 * graphs of ADS copied into NetworKit and kept in step with it through
 * their typed modifications. Each test checks which arrays the library
 * reads in place and which it reads from a copy, and runs an algorithm of
 * the library on the structure of ADS against the result of ADS, or
 * against a computation from scratch. A test whose library is not compiled
 * in passes with nothing to check, and says so; at the level 3 of the
 * contracts.
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

#include "ADS/Modification.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/interop/Csr.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#if defined( ADS_WITH_EIGEN )
 #include "ADS/interop/eigen/Eigen.h"

 #include <Eigen/IterativeLinearSolvers>
#endif
#if defined( ADS_WITH_METIS )
 #include "ADS/interop/metis/Metis.h"
#endif
#if defined( ADS_WITH_KAHIP )
 #include "ADS/interop/kahip/KaHIP.h"
#endif
#if defined( ADS_WITH_GRAPHBLAS )
 #include "ADS/interop/graphblas/GraphBLAS.h"
#endif
#if defined( ADS_WITH_NETWORKIT )
 #include "ADS/interop/networkit/NetworKit.h"

 #include <networkit/components/ConnectedComponents.hpp>
 #include <networkit/components/DynConnectedComponents.hpp>
 #include <networkit/distance/Dijkstra.hpp>
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

using namespace ADS;

/// a message for the tests whose library is not compiled in
[[maybe_unused]] void notCompiled( const char * what )
{
 std::cout << "  (" << what << " not compiled in: nothing to check)"
	   << std::endl;
 }

/// the edges of the \f$ s \times s \f$ grid, of the weights \f$ 1 \f$ to
/// \f$ 9 \f$
[[maybe_unused]] std::vector< std::tuple< int , int , int > >
gridEdges( int s , unsigned seed )
{
 std::mt19937 r( seed );
 std::vector< std::tuple< int , int , int > > E;
 for( int i = 0 ; i < s ; ++i )
  for( int j = 0 ; j < s ; ++j ) {
   const int u = i * s + j;
   if( j + 1 < s )
    E.emplace_back( u , u + 1 , 1 + int( r() % 9 ) );
   if( i + 1 < s )
    E.emplace_back( u , u + s , 1 + int( r() % 9 ) );
   }
 return( E );
 }

/// the edges of a random graph of \f$ n \f$ nodes and about \f$ m \f$
/// distinct edges, of the weights \f$ 1 \f$ to \f$ 20 \f$
[[maybe_unused]] std::vector< std::tuple< int , int , int > >
randomEdges( int n , int m , unsigned seed )
{
 std::mt19937 r( seed );
 std::set< std::pair< int , int > > S;
 std::vector< std::tuple< int , int , int > > E;
 while( int( E.size() ) < m ) {
  int u = int( r() % unsigned( n ) ) , v = int( r() % unsigned( n ) );
  if( u == v )
   continue;
  if( u > v )
   std::swap( u , v );
  if( S.insert( { u , v } ).second )
   E.emplace_back( u , v , 1 + int( r() % 20 ) );
  }
 return( E );
 }

/// the weight of the edges of \f$ g \f$ whose ends are in two parts
template< class G >
[[maybe_unused]] long long cutOf( const G & g , const std::vector< int > & p ,
				  bool weights )
{
 long long c = 0;
 for( int u = 0 ; u < g.n() ; ++u )
  for( int k = g.offsets()[ u ] ; k < g.offsets()[ u + 1 ] ; ++k ) {
   const int v = g.arcHeads()[ k ];
   if( ( u < v ) && ( p[ std::size_t( u ) ] != p[ std::size_t( v ) ] ) ) {
    if constexpr( G::weighted )
     c += weights ? static_cast< long long >( g.arcWeights()[ k ] ) : 1;
    else
     c += 1;
    }
   }
 return( c );
 }

/// whether \f$ P \f$ is a partition of the \f$ n \f$ nodes into \f$ k \f$
/// nonempty parts, and its order numbers the nodes part by part
[[maybe_unused]] bool validPartition( const Partition & P , int n , int k )
{
 if( ( int( P.part.size() ) != n ) || ( P.k != k ) )
  return( false );
 for( const int p : P.part )
  if( ( p < 0 ) || ( p >= k ) )
   return( false );
 const auto s = P.sizes();
 if( std::find( s.begin() , s.end() , 0 ) != s.end() )
  return( false );
 const auto o = P.order();
 std::vector< int > start( std::size_t( k ) + 1 , 0 );
 for( int q = 0 ; q < k ; ++q )
  start[ std::size_t( q ) + 1 ] =
    start[ std::size_t( q ) ] + s[ std::size_t( q ) ];
 std::vector< bool > seen( std::size_t( n ) , false );
 for( int u = 0 ; u < n ; ++u ) {
  const int p = P.part[ std::size_t( u ) ] , x = o[ std::size_t( u ) ];
  if( ( x < start[ std::size_t( p ) ] ) ||
      ( x >= start[ std::size_t( p ) + 1 ] ) || seen[ std::size_t( x ) ] )
   return( false );
  seen[ std::size_t( x ) ] = true;
  }
 return( true );
 }

/// the number of the components of \f$ g \f$
template< class G >
[[maybe_unused]] int components( const G & g )
{
 const auto c = cc( g );
 return( int( std::set< int >( c.begin() , c.end() ).size() ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- EIGEN ----------------------------------*/
/*--------------------------------------------------------------------------*/

// a DenseMatrix written through its map, and a BasicCsrMatrix< double ,
// int > read in place by the products and the conjugate gradient of Eigen
ADS_TEST( "eigen-maps" )
{
#if defined( ADS_WITH_EIGEN )
 DenseMatrix< double > D( 3 , 4 , 1. );
 auto E = eigen::map( D );
 CHECK( ( E.data() == D.data() ) && ( E.rows() == 3 ) && ( E.cols() == 4 ) );
 E( 1 , 2 ) = 7.;
 CHECK( D( 1 , 2 ) == 7. );
 CHECK( eigen::map( std::as_const( D ) ).sum() == 18. );
 // the Laplacian of a path of 200 nodes plus the identity
 const int n = 200;
 std::vector< BasicCsrMatrix< double , int >::Triplet > T;
 for( int i = 0 ; i < n ; ++i ) {
  T.emplace_back( i , i , i == 0 || i == n - 1 ? 2. : 3. );
  if( i + 1 < n ) {
   T.emplace_back( i , i + 1 , -1. );
   T.emplace_back( i + 1 , i , -1. );
   }
  }
 const BasicCsrMatrix< double , int > A( n , n , T );
 const auto M = eigen::map( A );
 CHECK( ( M.outerIndexPtr() == A.rowStart() ) &&
	( M.innerIndexPtr() == A.columns() ) &&
	( M.valuePtr() == A.values() ) && ( M.nonZeros() == 3 * n - 2 ) );
 std::vector< double > x( n );
 for( int i = 0 ; i < n ; ++i )
  x[ std::size_t( i ) ] = std::sin( double( i ) );
 const auto y = A.multiply( x );
 const Eigen::VectorXd z =
   M * Eigen::Map< const Eigen::VectorXd >( x.data() , n );
 for( int i = 0 ; i < n ; ++i )
  CHECK( std::abs( z( i ) - y[ std::size_t( i ) ] ) < 1e-12 );
 Eigen::ConjugateGradient< eigen::RowSparse< double > ,
			   Eigen::Lower | Eigen::Upper >
   cg;
 cg.setTolerance( 1e-12 );
 cg.compute( M );
 const Eigen::VectorXd s =
   cg.solve( Eigen::Map< const Eigen::VectorXd >( y.data() , n ) );
 CHECK( cg.info() == Eigen::Success );
 for( int i = 0 ; i < n ; ++i )
  CHECK( std::abs( s( i ) - x[ std::size_t( i ) ] ) < 1e-8 );
#else
 notCompiled( "Eigen" );
#endif
 }

// the default CsrMatrix, whose layout Eigen does not read, copied to Eigen
// and back
ADS_TEST( "eigen-copies" )
{
#if defined( ADS_WITH_EIGEN )
 std::mt19937 r( 3 );
 std::vector< CsrMatrix::Triplet > T;
 for( int k = 0 ; k < 500 ; ++k )
  T.emplace_back( r() % 60 , r() % 80 , double( r() % 100 ) / 7. + 1. );
 const CsrMatrix A( 60 , 80 , T );
 const auto E = eigen::toEigen( A );
 CHECK( ( E.rows() == 60 ) && ( E.cols() == 80 ) &&
	( std::size_t( E.nonZeros() ) == A.nonzeros() ) && E.isCompressed() );
 for( std::size_t i = 0 ; i < 60 ; ++i )
  for( std::size_t j = 0 ; j < 80 ; ++j )
   CHECK( E.coeff( Eigen::Index( i ) , Eigen::Index( j ) ) == A.get( i , j ) );
 CHECK( eigen::fromEigen( E ) == A );
 const Eigen::SparseMatrix< double > C = E;  // by columns
 CHECK( eigen::fromEigen( C ) == A );
 CHECK(
   ( eigen::fromEigen< int >( E * 2. ).get( 3 , 5 ) == 2. * A.get( 3 , 5 ) ) );
#else
 notCompiled( "Eigen" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ METIS, KaHIP ------------------------------*/
/*--------------------------------------------------------------------------*/

// the arrays that CsrInput reads in place and those it copies, for the
// layouts of StaticGraph
ADS_TEST( "csr-input" )
{
 const auto E = gridEdges( 6 , 1 );
 std::vector< std::pair< int , int > > U;
 for( const auto & [ u , v , w ] : E )
  U.emplace_back( u , v );
 const StaticGraph<> g0( 36 , U , false );
 const CsrInput< int > I0( g0 );
 CHECK( ( I0.offsets() == g0.offsets() ) && ( I0.heads() == g0.heads() ) &&
	( I0.weights() == nullptr ) && ! I0.copied().offsets &&
	! I0.copied().heads );
 const StaticGraph< int > g1( 36 , E , false );
 const CsrInput< int > I1( g1 );
 CHECK( ( I1.heads() == g1.heads() ) && ( I1.weights() == g1.weightData() ) &&
	! I1.copied().weights );
 const StaticGraph< int , ArcLayout::Interleaved > g2( 36 , E , false );
 const CsrInput< int > I2( g2 );
 CHECK( ( I2.offsets() == g2.offsets() ) && I2.copied().heads &&
	I2.copied().weights );
 const CsrInput< std::int64_t > I3( g1 );
 CHECK( I3.copied().offsets && I3.copied().heads && I3.copied().weights );
 for( int k = 0 ; k < g1.arcs() ; ++k )
  CHECK( ( I2.heads()[ k ] == g1.heads()[ k ] ) &&
	 ( I2.weights()[ k ] == g1.weightData()[ k ] ) &&
	 ( I3.heads()[ k ] == g1.heads()[ k ] ) );
 const CsrInput< int > I4( g1 , false );
 CHECK( I4.weights() == nullptr );
 }

// METIS on the rows of StaticGraph: in place for the separate layout,
// through a copy for the interleaved one and for the weights of another
// type; the cut of METIS against that of the partition, the relabelling
ADS_TEST( "metis-partition" )
{
#if defined( ADS_WITH_METIS )
 const auto E = gridEdges( 20 , 2 );
 const StaticGraph< int > g( 400 , E , false );
 const metis::Input I( g );
 CHECK( ( I.offsets() == g.offsets() ) && ( I.heads() == g.heads() ) &&
	( I.weights() == g.weightData() ) );
 for( const bool recursive : { false , true } ) {
  metis::Options o;
  o.recursive = recursive;
  const Partition P = metis::partition( g , I , 4 , o );
  CHECK( validPartition( P , 400 , 4 ) );
  CHECK( P.cut == cutOf( g , P.part , true ) );
  const auto h = g.relabel( P.order() );
  CHECK( ( h.m() == g.m() ) && ( h.n() == g.n() ) );
  }
 metis::Options o;
 o.weights = false;
 const Partition Q = metis::partition( g , 8 , o );
 CHECK( validPartition( Q , 400 , 8 ) &&
	( Q.cut == cutOf( g , Q.part , false ) ) );
 const StaticGraph< int , ArcLayout::Interleaved > gi( 400 , E , false );
 const Partition R = metis::partition( gi , 4 );
 CHECK( validPartition( R , 400 , 4 ) &&
	( R.cut == cutOf( gi , R.part , true ) ) );
 std::vector< std::tuple< int , int , double > > F;
 for( const auto & [ u , v , w ] : E )
  F.emplace_back( u , v , double( w ) );
 const StaticGraph< double > gd( 400 , F , false );
 const metis::Input J( gd );
 CHECK( ( J.heads() == gd.heads() ) && J.copied().weights );
 const Partition S = metis::partition( gd , J , 4 );
 CHECK( validPartition( S , 400 , 4 ) &&
	( S.cut == cutOf( gd , S.part , true ) ) );
 CHECK( metis::partition( g , 1 ).cut == 0 );
#else
 notCompiled( "METIS" );
#endif
 }

// kaffpa() of KaHIP on the rows of StaticGraph, as METIS
ADS_TEST( "kahip-partition" )
{
#if defined( ADS_WITH_KAHIP )
 const auto E = gridEdges( 20 , 3 );
 const StaticGraph< int > g( 400 , E , false );
 const kahip::Input I( g );
 CHECK( ( I.offsets() == g.offsets() ) && ( I.heads() == g.heads() ) &&
	( I.weights() == g.weightData() ) );
 for( const int mode : { FAST , ECO } ) {
  kahip::Options o;
  o.mode = mode;
  const Partition P = kahip::partition( g , I , 4 , o );
  CHECK( validPartition( P , 400 , 4 ) );
  CHECK( P.cut == cutOf( g , P.part , true ) );
  }
 std::vector< std::pair< int , int > > U;
 for( const auto & [ u , v , w ] : E )
  U.emplace_back( u , v );
 const StaticGraph<> gu( 400 , U , false );
 const Partition Q = kahip::partition( gu , 8 );
 CHECK( validPartition( Q , 400 , 8 ) &&
	( Q.cut == cutOf( gu , Q.part , false ) ) );
 const StaticGraph< int , ArcLayout::Interleaved > gi( 400 , E , false );
 const Partition R = kahip::partition( gi , 4 );
 CHECK( validPartition( R , 400 , 4 ) &&
	( R.cut == cutOf( gi , R.part , true ) ) );
 CHECK( kahip::partition( g , 1 ).cut == 0 );
#else
 notCompiled( "KaHIP" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- GRAPHBLAS --------------------------------*/
/*--------------------------------------------------------------------------*/

#if defined( ADS_WITH_GRAPHBLAS )

namespace
{

/// GraphBLAS started once for the program
void startGraphBLAS( void )
{
 static const bool started = [] {
  graphblas::check( GrB_init( GrB_NONBLOCKING ) , "GrB_init" );
  return( true );
  }();
 ( void )started;
 }

/// the number of the triangles of the unoriented graph of the symmetric
/// matrix A, by \f$ \sum ( A \circ A^2 ) / 6 \f$
long long trianglesOf( GrB_Matrix A )
{
 GrB_Index n = 0;
 graphblas::check( GrB_Matrix_nrows( &n , A ) , "GrB_Matrix_nrows" );
 GrB_Matrix C = nullptr;
 graphblas::check( GrB_Matrix_new( &C , GrB_INT64 , n , n ) ,
		   "GrB_Matrix_new" );
 const graphblas::Matrix c( C );
 graphblas::check(
   GrB_mxm( C , A , nullptr , GrB_PLUS_TIMES_SEMIRING_INT64 , A , A ,
	    GrB_DESC_S ) ,
   "GrB_mxm" );
 std::int64_t s = 0;
 graphblas::check(
   GrB_Matrix_reduce_INT64( &s , nullptr , GrB_PLUS_MONOID_INT64 , C ,
			    nullptr ) ,
   "GrB_Matrix_reduce" );
 return( static_cast< long long >( s / 6 ) );
 }

/// whether \f$ A \f$ has components that GraphBLAS reads and does not own,
/// i.e., the arrays of ADS, not copied
bool readOnly( GrB_Matrix A )
{
 std::int32_t r = 0;
 graphblas::check( GrB_Matrix_get_INT32( A , &r , GxB_IS_READONLY ) ,
		   "GrB_Matrix_get" );
 return( r != 0 );
 }

/// \f$ y = A x \f$ by GraphBLAS, \f$ A \f$ of doubles
std::vector< double > productOf( GrB_Matrix A ,
				 const std::vector< double > & x )
{
 GrB_Index m = 0 , n = 0;
 graphblas::check( GrB_Matrix_nrows( &m , A ) , "GrB_Matrix_nrows" );
 graphblas::check( GrB_Matrix_ncols( &n , A ) , "GrB_Matrix_ncols" );
 GrB_Vector v = nullptr , y = nullptr;
 graphblas::check( GrB_Vector_new( &v , GrB_FP64 , n ) , "GrB_Vector_new" );
 graphblas::check( GrB_Vector_new( &y , GrB_FP64 , m ) , "GrB_Vector_new" );
 for( GrB_Index j = 0 ; j < n ; ++j )
  graphblas::check( GrB_Vector_setElement_FP64( v , x[ j ] , j ) ,
		    "GrB_Vector_setElement" );
 graphblas::check(
   GrB_mxv( y , nullptr , nullptr , GrB_PLUS_TIMES_SEMIRING_FP64 , A , v ,
	    nullptr ) ,
   "GrB_mxv" );
 std::vector< double > r( m , 0. );
 for( GrB_Index i = 0 ; i < m ; ++i ) {
  double z = 0.;
  const GrB_Info k = GrB_Vector_extractElement_FP64( &z , y , i );
  if( k == GrB_SUCCESS )
   r[ i ] = z;
  }
 GrB_Vector_free( &v );
 GrB_Vector_free( &y );
 return( r );
 }

}  // end( namespace )

#endif

// the views of the unweighted and of the weighted StaticGraph, iso-valued
// and of the weights, and the copy of the interleaved one: the triangles
// counted by GraphBLAS against those counted on the rows
ADS_TEST( "graphblas-graphs" )
{
#if defined( ADS_WITH_GRAPHBLAS )
 startGraphBLAS();
 const int n = 300;
 const auto E = randomEdges( n , 3000 , 4 );
 std::vector< std::pair< int , int > > U;
 for( const auto & [ u , v , w ] : E )
  U.emplace_back( u , v );
 const StaticGraph<> g( n , U , false );
 long long t = 0;  // the triangles u < v < w, on the rows
 for( int u = 0 ; u < n ; ++u )
  for( const int v : g.out( u ) )
   if( v > u )
    for( const int w : g.out( v ) )
     if( ( w > v ) && std::ranges::binary_search( g.out( u ) , w ) )
      ++t;
 const graphblas::Matrix A = graphblas::view( g );
 GrB_Index nv = 0;
 graphblas::check( GrB_Matrix_nvals( &nv , A ) , "GrB_Matrix_nvals" );
 CHECK( nv == GrB_Index( g.arcs() ) );
 bool iso = false;
 graphblas::check( GxB_Matrix_iso( &iso , A ) , "GxB_Matrix_iso" );
 CHECK( iso && readOnly( A ) );
 CHECK( trianglesOf( A ) == t );
 const StaticGraph< int > gw( n , E , false );
 const graphblas::Matrix B = graphblas::view( gw );
 CHECK( readOnly( B ) );
 CHECK( trianglesOf( B ) >= t );  // weights of 1 to 20 on each edge
 for( int u = 0 ; u < n ; u += 17 )
  for( const auto a : gw.out( u ) ) {
   std::int32_t w = 0;
   graphblas::check( GrB_Matrix_extractElement_INT32( &w , B , GrB_Index( u ) ,
						      GrB_Index( a.to ) ) ,
		     "GrB_Matrix_extractElement" );
   CHECK( w == a.w );
   }
 const StaticGraph< int , ArcLayout::Interleaved > gi( n , E , false );
 const graphblas::Matrix C = graphblas::copy( gi );
 CHECK( ! readOnly( C ) );
 const auto X = graphblas::toCsr< int , int >( B ) ,
	    Y = graphblas::toCsr< int , int >( C );
 CHECK( X == Y );
 CHECK( ( graphblas::toCsr< int , int >( graphblas::copy( g ) ) ==
	  graphblas::toCsr< int , int >( A ) ) );
 // the graph is untouched by GraphBLAS, and outlives the views
 CHECK( g.arcs() == 2 * g.m() );
#else
 notCompiled( "GraphBLAS" );
#endif
 }

// the views of BasicCsrMatrix of the default types (offsets of 64 bits,
// columns of 32) and of int: the products of GraphBLAS against those of
// ADS, and the copy back
ADS_TEST( "graphblas-matrices" )
{
#if defined( ADS_WITH_GRAPHBLAS )
 startGraphBLAS();
 std::mt19937 r( 5 );
 std::vector< CsrMatrix::Triplet > T;
 for( int k = 0 ; k < 2000 ; ++k )
  T.emplace_back( r() % 150 , r() % 120 , double( r() % 1000 ) / 13. );
 const CsrMatrix A( 150 , 120 , T );
 std::vector< BasicCsrMatrix< double , int >::Triplet > T2( T.begin() ,
							   T.end() );
 const BasicCsrMatrix< double , int > A2( 150 , 120 , T2 );
 std::vector< double > x( 120 );
 for( std::size_t j = 0 ; j < x.size() ; ++j )
  x[ j ] = std::cos( double( j ) );
 const auto y = A.multiply( x );
 for( const auto & M : { graphblas::view( A ) , graphblas::view( A2 ) } ) {
  CHECK( readOnly( M ) );
  const auto z = productOf( M , x );
  CHECK( z.size() == y.size() );
  for( std::size_t i = 0 ; i < y.size() ; ++i )
   CHECK( std::abs( z[ i ] - y[ i ] ) < 1e-9 );
  CHECK( graphblas::toCsr< double >( M ) == A );
  }
 const CsrMatrix Z( 5 , 7 );
 const graphblas::Matrix V = graphblas::view( Z );
 GrB_Index nv = 1;
 graphblas::check( GrB_Matrix_nvals( &nv , V ) , "GrB_Matrix_nvals" );
 CHECK( nv == 0 );
#else
 notCompiled( "GraphBLAS" );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- NETWORKIT --------------------------------*/
/*--------------------------------------------------------------------------*/

// StaticGraph and ListGraph copied into NetworKit, oriented and not,
// weighted and not: the edges, the weights, the components and the
// distances of Dijkstra of NetworKit against those of ADS
ADS_TEST( "networkit-copies" )
{
#if defined( ADS_WITH_NETWORKIT )
 const int n = 200;
 const auto E = randomEdges( n , 260 , 6 );
 const StaticGraph< int > g( n , E , false );
 const NetworKit::Graph N = networkit::toNetworKit( g );
 CHECK( ( int( N.numberOfNodes() ) == n ) &&
	( int( N.numberOfEdges() ) == g.m() ) && N.isWeighted() &&
	! N.isDirected() );
 for( const auto & [ u , v , w ] : E )
  CHECK( ( N.weight( NetworKit::node( u ) , NetworKit::node( v ) ) == w ) &&
	 ( N.weight( NetworKit::node( v ) , NetworKit::node( u ) ) == w ) );
 NetworKit::ConnectedComponents C( N );
 C.run();
 CHECK( int( C.numberOfComponents() ) == components( g ) );
 const auto T = johnson( g , 0 );
 NetworKit::Dijkstra D( N , 0 );
 D.run();
 for( int v = 0 ; v < n ; ++v )
  if( T.T[ std::size_t( v ) ] != -1 || v == 0 )
   CHECK( D.distance( NetworKit::node( v ) ) ==
	  double( T.d[ std::size_t( v ) ] ) );
 std::vector< std::pair< int , int > > U;
 for( const auto & [ u , v , w ] : E )
  U.emplace_back( v , u );
 const StaticGraph<> h( n , U , true );
 const NetworKit::Graph H = networkit::toNetworKit( h );
 CHECK( ( int( H.numberOfEdges() ) == h.arcs() ) && H.isDirected() &&
	! H.isWeighted() );
 for( const auto & [ u , v ] : U )
  CHECK( H.hasEdge( NetworKit::node( u ) , NetworKit::node( v ) ) &&
	 ! H.hasEdge( NetworKit::node( v ) , NetworKit::node( u ) ) );
 const StaticGraph< int , ArcLayout::Interleaved > gi( n , E , false );
 const NetworKit::Graph Ni = networkit::toNetworKit( gi );
 CHECK( int( Ni.numberOfEdges() ) == g.m() );
 ListGraph< int > l( 10 , false );
 l.insertEdge( 0 , 1 , 4 );
 l.insertEdge( 1 , 2 , 5 );
 l.insertEdge( 7 , 9 , 6 );
 l.deleteNode( 5 );
 const NetworKit::Graph L = networkit::toNetworKit( l );
 CHECK( ( L.numberOfNodes() == 9 ) && ( L.numberOfEdges() == 3 ) &&
	! L.hasNode( 5 ) && ( L.weight( 2 , 1 ) == 5. ) );
#else
 notCompiled( "NetworKit" );
#endif
 }

// a ListGraph of ADS changed through its ObservedGraph, one change at a
// time and in batches, and the DynConnectedComponents of NetworKit kept up
// to date by NetworKitMirror against the components of ADS
ADS_TEST( "networkit-mirror" )
{
#if defined( ADS_WITH_NETWORKIT )
 using G = ListGraph<>;
 using M = networkit::NetworKitMirror< G >;
 const int n = 120;
 std::mt19937 r( 7 );
 G g( n , false );
 for( int k = 0 ; k < 80 ; ++k ) {
  const int u = int( r() % n ) , v = int( r() % n );
  if( ( u != v ) && ! g.weight( u , v ) )
   g.insertEdge( u , v );
  }
 M m( g );
 NetworKit::DynConnectedComponents dcc( m.graph() );
 dcc.run();
 const int id = m.attach( dcc );
 ObservedGraph< G , Static< M > > og( g , m );
 CHECK( int( dcc.numberOfComponents() ) == components( g ) );
 for( int step = 0 ; step < 400 ; ++step ) {
  const int u = int( r() % n ) , v = int( r() % n );
  if( u == v )
   continue;
  if( g.weight( u , v ) )
   og.deleteEdge( u , v );
  else
   og.insertEdge( u , v );
  CHECK( int( dcc.numberOfComponents() ) == components( g ) );
  CHECK( m.graph().numberOfEdges() == NetworKit::count( g.m() ) );
  }
 const std::size_t told = m.told();
 for( int round = 0 ; round < 20 ; ++round ) {
  {
   auto b = ADS::batch( og );
   for( int k = 0 ; k < 15 ; ++k ) {
    const int u = int( r() % n ) , v = int( r() % n );
    if( u == v )
     continue;
    if( g.weight( u , v ) )
     og.deleteEdge( u , v );
    else
     og.insertEdge( u , v );
    }
   }
  CHECK( int( dcc.numberOfComponents() ) == components( g ) );
  CHECK( m.graph().numberOfEdges() == NetworKit::count( g.m() ) );
  }
 CHECK( m.told() > told );
 m.detach( id );
 const std::size_t after = m.told();
 og.insertEdge( 0 , n - 1 );
 CHECK( m.told() == after );
 CHECK( m.graph().hasEdge( 0 , NetworKit::node( n - 1 ) ) );
#else
 notCompiled( "NetworKit" );
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
/*---------------------- End File interop_graphs.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
