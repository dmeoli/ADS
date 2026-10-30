/*--------------------------------------------------------------------------*/
/*---------------------------- File matrix.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the dense matrices of doubles, n x n with entries
 * uniform in [ -1 , 1 ):
 *
 * - "gemm", the product C = A B: ADS::gemm() on DenseMatrix against the
 *   product of Eigen on matrices by rows (C.noalias() = A * B) and, if
 *   found, cblas_dgemm() of OpenBLAS (ADS_HAS_OPENBLAS, built for one
 *   thread); the book's multiplyNaive() and strassen() (with the blocks of
 *   at most 64 rows by the naive product) on Matrix up to n = 512; each
 *   cell in nanoseconds per 1000 floating point operations, of the
 *   2 n^3 of the product; ADS::multiplyStrassen() from n = 4096 (it is
 *   gemm() below), whose cells count the same 2 n^3 operations;
 * - "transpose", a new matrix B = A^T: ADS::transpose() on DenseMatrix
 *   against Eigen, and the book's transpose() on Matrix; ns per entry.
 *
 * ADS::gemm() and ADS::multiplyStrassen() are those of the build, on the
 * backend of a CBLAS when ADS_WITH_BLAS is defined [see DenseMatrix.h], as
 * they are used; built with ADS_BENCH_OWN instead, the benchmark times them
 * alone, on the product of the definition by blocks that runs without the
 * backend, as "ADS::gemm own" and "ADS::multiplyStrassen own".
 *
 * Eigen and OpenBLAS are vectorized for the processor as ADS is
 * (-march=native), on one thread. Up to n = 512 each run of a product
 * follows an untimed product of the same library, which leaves the
 * operands in the cache and the clock of the processor at the level that
 * its vectors set, as in a loop of products, and each cell of the product
 * begins after 20 ms of scalar work, at the same clock as the others.
 * Each cell is the median of r runs; the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   matrix [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/numeric/Matrix.h"
#ifndef ADS_BENCH_OLD
 #include "ADS/numeric/DenseMatrix.h"
#endif

#if __has_include( <Eigen/Dense> )
 #include <Eigen/Dense>
 #define ADS_HAS_EIGEN 1
#endif

#ifdef ADS_HAS_OPENBLAS
 #include <cblas.h>
#endif

#ifdef ADS_BENCH_OWN                        // the code of ADS alone
 #undef ADS_HAS_EIGEN
 #undef ADS_HAS_OPENBLAS
 #define ADS_WHO " own"
#else
 #define ADS_WHO ""
#endif

#include <initializer_list>

using namespace ADS::bench;

/// whether any structure of who is measured, so that what they share is
/// built; under ADS_BENCH_LIST none is, and no label is printed here
static bool wanted( std::initializer_list< const char * > who )
{
 if( listing() )
  return( false );
 for( const char * w : who )
  if( ! skip( w ) )
   return( true );
 return( false );
 }

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 64 , 256 , 1024 , 2048 } );
 for( std::size_t n : o.sizes ) {
  const int r = o.runs;
  std::mt19937_64 g( o.seed );
  std::uniform_real_distribution< double > U( -1 , 1 );
  ADS::Matrix< double > A( n , std::vector< double >( n ) ) , B = A;
  for( auto & row : A ) for( auto & x : row ) x = U( g );
  for( auto & row : B ) for( auto & x : row ) x = U( g );
  const double flops = 2.0 * double( n ) * double( n ) * double( n );
  const std::size_t ops = std::size_t( flops / 1000 );   // kiloflops
  auto none = [](){ return( 0 ); };
  // a product before each run, not timed, so that the run finds the clock
  // that its own vectors set (after those of 512 bits of a library the
  // processor keeps a lower clock for some milliseconds) and its data in
  // the cache, as in a loop of products; up to n = 512
  auto warm = [ & ]( auto && f ) {
   return( [ & , f ](){ if( n <= 512 ) f( 0 ); return( 0 ); } ); };
  // 20 ms of scalar work before each cell of the product, so that each
  // one begins at the same clock, whatever the vectors of the one before
  auto settle = [](){
   const auto end = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds( 20 );
   while( std::chrono::steady_clock::now() < end ) {}
   };
#ifdef ADS_HAS_EIGEN
  using EM = Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic ,
                            Eigen::RowMajor >;
  EM EA , EB , EC;
  if( wanted( { "Eigen" , "Eigen A/A" } ) ) {
   EA.resize( n , n );
   EB.resize( n , n );
   EC.resize( n , n );
   for( std::size_t i = 0 ; i < n ; ++i )
    for( std::size_t j = 0 ; j < n ; ++j ) {
     EA( i , j ) = A[ i ][ j ];
     EB( i , j ) = B[ i ][ j ];
     }
   }
  for( const char * who : { "Eigen" , "Eigen A/A" } ) {
   if( skip( who ) )
    continue;
   settle();
   print( "gemm" , "double" , n , who , median( warm( [ & ]( int ) {
           EC.noalias() = EA * EB; } ) , [ & ]( int ) {
           EC.noalias() = EA * EB;
           sink = sink + ( EC( 0 , 0 ) > 0 ); } , r , ops ) );
   }
#endif
#ifndef ADS_BENCH_OLD
  ADS::DenseMatrix< double > DA , DB , DC;
  if( wanted( { "ADS::gemm" ADS_WHO , "ADS::multiplyStrassen" ADS_WHO ,
                "ADS::transpose" } ) ) {
   DA = ADS::DenseMatrix< double >( A );
   DB = ADS::DenseMatrix< double >( B );
   DC = ADS::DenseMatrix< double >( n , n );
   }
#endif
#ifdef ADS_HAS_OPENBLAS
  if( ! skip( "OpenBLAS" ) ) {
   std::vector< double > a( n * n ) , b( n * n ) , c( n * n );
   for( std::size_t i = 0 ; i < n ; ++i )
    for( std::size_t j = 0 ; j < n ; ++j ) {
     a[ i * n + j ] = A[ i ][ j ];
     b[ i * n + j ] = B[ i ][ j ];
     }
   const int N = int( n );
   auto blas = [ & ]( int ) {
    cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , N , N , N ,
                 1.0 , a.data() , N , b.data() , N , 0.0 , c.data() , N ); };
   settle();
   print( "gemm" , "double" , n , "OpenBLAS" , median( warm( blas ) ,
           [ & ]( int ) {
            blas( 0 );
            sink = sink + ( c[ 0 ] > 0 ); } , r , ops ) );
   }
#endif
#ifndef ADS_BENCH_OLD
  auto ads = [ & ]( int ) {
   ADS::gemm( n , n , n , 1.0 , DA.data() , n , DB.data() , n , 0.0 ,
              DC.data() , n ); };
  if( ! skip( "ADS::gemm" ADS_WHO ) ) {
   settle();
   print( "gemm" , "double" , n , "ADS::gemm" ADS_WHO , median( warm( ads ) ,
           [ & ]( int ) {
            ads( 0 );
            sink = sink + ( DC( 0 , 0 ) > 0 ); } , r , ops ) );
   }
  if( ( n >= 4096 ) && ! skip( "ADS::multiplyStrassen" ADS_WHO ) ) {
   settle();
   print( "gemm" , "double" , n , "ADS::multiplyStrassen" ADS_WHO ,
          median( none ,
           [ & ]( int ) {
            sink = sink + ( ADS::multiplyStrassen( DA , DB )( 0 , 0 ) > 0 );
            } , r , ops ) );
   }
#endif
#ifdef ADS_BENCH_OWN
  }
 return( 0 );
 }
#else
  if( n <= 512 ) {
   if( ! skip( "ADS::multiplyNaive" ) )
    print( "gemm" , "double" , n , "ADS::multiplyNaive" , median( none ,
            [ & ]( int ) {
             sink = sink + ( ADS::multiplyNaive( A , B )[ 0 ][ 0 ] > 0 ); } ,
            r , ops ) );
   if( ! skip( "ADS::strassen" ) )
    print( "gemm" , "double" , n , "ADS::strassen" , median( none ,
            [ & ]( int ) {
             sink = sink + ( ADS::strassen( A , B , 64 )[ 0 ][ 0 ] > 0 ); } ,
            r , ops ) );
   }

  // ----- transpose
  const std::size_t entries = n * n;
#ifdef ADS_HAS_EIGEN
  for( const char * who : { "Eigen" , "Eigen A/A" } )
   if( ! skip( who ) )
    print( "transpose" , "double" , n , who , median( none , [ & ]( int ) {
            const EM T = EA.transpose();
            sink = sink + ( T( 0 , 0 ) > 0 ); } , r , entries ) );
#endif
#ifndef ADS_BENCH_OLD
  if( ! skip( "ADS::transpose" ) )
   print( "transpose" , "double" , n , "ADS::transpose" , median( none ,
           [ & ]( int ) {
            sink = sink + ( ADS::transpose( DA )( 0 , 0 ) > 0 ); } , r ,
           entries ) );
#endif
  if( ! skip( "ADS::transpose (book)" ) )
   print( "transpose" , "double" , n , "ADS::transpose (book)" ,
           median( none , [ & ]( int ) {
            sink = sink + ( ADS::transpose( A )[ 0 ][ 0 ] > 0 ); } , r ,
           entries ) );
  }
 return( 0 );
 }
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- End File matrix.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
