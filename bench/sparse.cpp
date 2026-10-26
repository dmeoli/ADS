/*--------------------------------------------------------------------------*/
/*---------------------------- File sparse.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sparse matrices of doubles, n x n:
 *
 * - "spmv", the product y = A x of a matrix with 10 nonzeros in random
 *   columns per row (case random) and of the Laplacian of the grid of
 *   sqrt( n ) x sqrt( n ) points by its stencil of 5 points (case
 *   laplacian, a band): CsrMatrix::multiply() of ADS against the product
 *   of Eigen on a SparseMatrix by rows, and the walk of the orthogonal
 *   lists of the book's SparseMatrix (rowEntries()) up to n = 10^5, and
 *   the products of SellMatrix of ADS in SELL-8-256 and SELL-4-256; ns per
 *   nonzero;
 * - "sptranspose", the transpose of the same matrices, as a new matrix by
 *   rows: CsrMatrix::transpose() against Eigen; ns per nonzero.
 *
 * Each cell is the median of r runs; the building of the matrices is not
 * timed; the contracts are off (ADS_CONTRACTS=0).
 *
 *   sparse [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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

#include "ADS/numeric/SparseMatrix.h"
#ifndef ADS_BENCH_OLD
 #include "ADS/numeric/CsrMatrix.h"
 #include "ADS/numeric/SellMatrix.h"
#endif

#if __has_include( <Eigen/Sparse> )
 #include <Eigen/Sparse>
 #define ADS_HAS_EIGEN 1
#endif

#include <cmath>
#include <tuple>

using namespace ADS::bench;
using Triplet = std::tuple< std::size_t , std::size_t , double >;

namespace {

/// the triplets of the case c, n x n
std::vector< Triplet > triplets( int c , std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::uniform_real_distribution< double > U( -1 , 1 );
 std::vector< Triplet > T;
 if( c == 0 )
  for( std::size_t i = 0 ; i < n ; ++i )
   for( int k = 0 ; k < 10 ; ++k ) T.emplace_back( i , g() % n , U( g ) );
 else {
  const std::size_t s = std::size_t( std::sqrt( double( n ) ) );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::size_t r = i / s , q = i % s;
   T.emplace_back( i , i , 4.0 );
   if( q > 0 ) T.emplace_back( i , i - 1 , -1.0 );
   if( q + 1 < s && i + 1 < n ) T.emplace_back( i , i + 1 , -1.0 );
   if( r > 0 ) T.emplace_back( i , i - s , -1.0 );
   if( i + s < n ) T.emplace_back( i , i + s , -1.0 );
   }
  }
 return( T );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  for( int c = 0 ; c < 2 ; ++c ) {
   const char * what = c ? "laplacian" : "random";
   const auto T = triplets( c , n , o.seed );
   std::vector< double > x( n );
   std::mt19937_64 g( o.seed + 1 );
   for( auto & v : x ) v = double( g() % 1000 ) / 1000;
   auto none = [](){ return( 0 ); };
#ifdef ADS_HAS_EIGEN
   using ES = Eigen::SparseMatrix< double , Eigen::RowMajor >;
   ES E{ Eigen::Index( n ) , Eigen::Index( n ) };
   if( ! skip( "Eigen" ) || ! skip( "Eigen A/A" ) ) {
    std::vector< Eigen::Triplet< double > > ET;
    for( const auto & [ i , j , v ] : T )
     ET.emplace_back( Eigen::Index( i ) , Eigen::Index( j ) , v );
    E.setFromTriplets( ET.begin() , ET.end() );
    }
   const std::size_t nnz = std::size_t( E.nonZeros() );
   const Eigen::Map< const Eigen::VectorXd > ex( x.data() ,
                                                 Eigen::Index( n ) );
   Eigen::VectorXd ey( n );
   for( const char * who : { "Eigen" , "Eigen A/A" } )
    if( ! skip( who ) )
     print( "spmv" , what , n , who , median( none , [ & ]( int ) {
             ey.noalias() = E * ex;
             sink = sink + ( ey[ 0 ] > 0 ); } , r , nnz ) );
#endif
#ifndef ADS_BENCH_OLD
   // the matrix of the CSR and of the SELL formats, built for them only
   const bool csr = ! skip( "ADS::CsrMatrix" ) ||
                    ! skip( "ADS::SellMatrix<8>" ) ||
                    ! skip( "ADS::SellMatrix<4>" );
   const ADS::CsrMatrix A = csr ? ADS::CsrMatrix( n , n , T ) :
                                  ADS::CsrMatrix();
   std::vector< double > y( n );
   if( ! skip( "ADS::CsrMatrix" ) )
    print( "spmv" , what , n , "ADS::CsrMatrix" , median( none ,
            [ & ]( int ) {
             A.multiply( x.data() , y.data() );
             sink = sink + ( y[ 0 ] > 0 ); } , r , A.nonzeros() ) );
#endif
#ifndef ADS_BENCH_OLD
   if( ! skip( "ADS::SellMatrix<8>" ) ) {
    const ADS::SellMatrix< 8 > S8( A );
    print( "spmv" , what , n , "ADS::SellMatrix<8>" , median( none ,
            [ & ]( int ) {
             S8.multiply( x.data() , y.data() );
             sink = sink + ( y[ 0 ] > 0 ); } , r , A.nonzeros() ) );
    }
   if( ! skip( "ADS::SellMatrix<4>" ) ) {
    const ADS::SellMatrix< 4 > S4( A );
    print( "spmv" , what , n , "ADS::SellMatrix<4>" , median( none ,
            [ & ]( int ) {
             S4.multiply( x.data() , y.data() );
             sink = sink + ( y[ 0 ] > 0 ); } , r , A.nonzeros() ) );
    }
#endif
   if( ( n <= 100000 ) && ! skip( "ADS::SparseMatrix" ) ) {
    ADS::SparseMatrix S( n , n );
    for( const auto & [ i , j , v ] : T ) S.set( i , j , S.get( i , j ) + v );
    std::vector< double > z( n );
    print( "spmv" , what , n , "ADS::SparseMatrix" , median( none ,
            [ & ]( int ) {
             for( std::size_t i = 0 ; i < n ; ++i ) {
              double s = 0;
              for( const auto [ j , v ] : S.rowEntries( i ) ) s += v * x[ j ];
              z[ i ] = s;
              }
             sink = sink + ( z[ 0 ] > 0 ); } , r , S.nonzeros() ) );
    }
#ifdef ADS_HAS_EIGEN
   for( const char * who : { "Eigen" , "Eigen A/A" } )
    if( ! skip( who ) )
     print( "sptranspose" , what , n , who , median( none , [ & ]( int ) {
             const ES Tr = E.transpose();
             sink = sink + std::uint64_t( Tr.nonZeros() ); } , r , nnz ) );
#endif
#ifndef ADS_BENCH_OLD
   if( ! skip( "ADS::CsrMatrix" ) )
    print( "sptranspose" , what , n , "ADS::CsrMatrix" , median( none ,
            [ & ]( int ) {
             sink = sink + A.transpose().nonzeros(); } , r ,
            A.nonzeros() ) );
#endif
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File sparse.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
