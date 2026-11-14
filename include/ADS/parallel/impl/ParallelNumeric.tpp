/*--------------------------------------------------------------------------*/
/*------------------------ File ParallelNumeric.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel products of ParallelNumeric.h.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the first i in [ 0 , m ] with i + start[ i ] >= t, start non-decreasing
inline std::size_t pathRow( const std::size_t * start , std::size_t m ,
                            std::size_t t )
{
 std::size_t lo = 0 , hi = m;
 while( lo < hi ) {
  const std::size_t mid = lo + ( hi - lo ) / 2;
  if( mid + start[ mid ] < t )
   lo = mid + 1;
  else
   hi = mid;
  }
 return( lo );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E >
void multiply( E & ex , const CsrMatrix & A , const double * x , double * y )
{
 const std::size_t m = A.rows() , nnz = A.nonzeros() ,
                   p = ex.concurrency();
 if( ( p == 1 ) || ( m + nnz < ( std::size_t( 1 ) << 15 ) ) ) {
  A.multiply( x , y );
  return;
  }
 const std::size_t B = std::min( 4 * p , m );
 const std::size_t * start = A.rowStart();
 const std::uint32_t * col = A.columns();
 const double * val = A.values();
 ex.bulk( B , [ & ]( std::size_t q ) {
  const std::size_t r0 = detail::pathRow( start , m , q * ( m + nnz ) / B ) ,
                    r1 = detail::pathRow( start , m ,
                                          ( q + 1 ) * ( m + nnz ) / B );
  for( std::size_t i = r0 , k = start[ r0 ] ; i < r1 ; ++i ) {
   double s = 0;                            // the loop of the sequential
   for( const std::size_t e = start[ i + 1 ] ; k < e ; ++k )   // product
    s += val[ k ] * x[ col[ k ] ];
   y[ i ] = s;
   }
  } );
 }

template< Executor E >
std::vector< double > multiply( E & ex , const CsrMatrix & A ,
                                const std::vector< double > & x )
{
 ADS_PRE( x.size() == A.cols() );
 std::vector< double > y( A.rows() );
 multiply( ex , A , x.data() , y.data() );
 return( y );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , std::size_t C >
void multiply( E & ex , const SellMatrix< C > & A , const double * x ,
               double * y )
{
 const std::size_t c = A.chunks() , p = ex.concurrency() ,
                   places = A.chunkStart( c );
 if( ( p == 1 ) || ( places + c < ( std::size_t( 1 ) << 15 ) ) ) {
  A.multiply( x , y );
  return;
  }
 const std::size_t B = std::min( 4 * p , c );
 auto bound = [ & ]( std::size_t t ) {     // the first chunk at t or past
  std::size_t lo = 0 , hi = c;
  while( lo < hi ) {
   const std::size_t mid = lo + ( hi - lo ) / 2;
   if( mid + A.chunkStart( mid ) < t ) lo = mid + 1; else hi = mid;
   }
  return( lo );
  };
 ex.bulk( B , [ & ]( std::size_t q ) {
  A.multiply( x , y , bound( q * ( places + c ) / B ) ,
              bound( ( q + 1 ) * ( places + c ) / B ) );
  } );
 }

template< Executor E , std::size_t C >
std::vector< double > multiply( E & ex , const SellMatrix< C > & A ,
                                const std::vector< double > & x )
{
 ADS_PRE( x.size() == A.cols() );
 std::vector< double > y( A.rows() );
 multiply( ex , A , x.data() , y.data() );
 return( y );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T >
void gemm( E & ex , std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc )
{
 ADS_PRE( ( lda >= k ) && ( ldb >= n ) && ( ldc >= n ) );
 if( ( m == 0 ) || ( n == 0 ) )
  return;
#if ADS_GEMM_BLAS
 // one call of the CBLAS, which runs on its own threads [see
 // DenseMatrix.h]
 if( detail::gemmOnCblas< T >( m , n , k , lda , ldb , ldc ) ) {
  detail::gemmCblas( m , n , k , alpha , A , lda , B , ldb , beta , C ,
                     ldc );
  return;
  }
#endif
 // a range of rows of C per thread, as evenly as they go
 const std::size_t t = std::min( ex.concurrency() , m );
 ex.bulk( t , [ & ]( std::size_t u ) {
  const std::size_t r0 = m * u / t , r1 = m * ( u + 1 ) / t;
  if( r1 > r0 )
   detail::gemmBlocked( r1 - r0 , n , k , alpha , A + r0 * lda , lda , B ,
                        ldb , beta , C + r0 * ldc , ldc );
  } );
 }

template< Executor E , class T >
DenseMatrix< T > multiply( E & ex , const DenseMatrix< T > & A ,
                           const DenseMatrix< T > & B )
{
 ADS_PRE( A.cols() == B.rows() );
 auto C = DenseMatrix< T >::uninitialized( A.rows() , B.cols() );
 gemm( ex , A.rows() , B.cols() , A.cols() , T( 1 ) , A.data() , A.cols() ,
       B.data() , B.cols() , T( 0 ) , C.data() , C.cols() );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ParallelNumeric.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
