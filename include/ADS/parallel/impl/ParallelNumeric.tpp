/*--------------------------------------------------------------------------*/
/*------------------------ File ParallelNumeric.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel products of ParallelNumeric.h.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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
 using K = detail::GemmKernel< T >;
 constexpr std::size_t MR = K::MR , NR = K::NR ,
                       small = std::size_t( 1 ) << 21;
 const std::size_t p = ex.concurrency();
 // the stripes: by rows if m >= n, by columns otherwise, each above the
 // threshold of the small products
 const bool byRows = m >= n;
 const std::size_t len = byRows ? m : n , unit = byRows ? MR : NR ,
                   other = byRows ? n : m;
 // a product is small, and goes by the other path of gemm(), when both
 // its multiplications and its packed B are below the thresholds
 auto isSmall = [ & ]( std::size_t rows , std::size_t cols ) {
  return( ( rows * cols * k <= small ) &&
          ( k * ( ( cols + NR - 1 ) / NR * NR ) <=
            ( std::size_t( 1 ) << 18 ) ) );
  };
 std::size_t s = std::min( p , len / unit );
 while( ( s > 1 ) && ( byRows ? isSmall( len / s , other )
                              : isSmall( other , len / s ) ) )
  --s;
 if( ( s <= 1 ) || ( k == 0 ) ) {
  gemm( m , n , k , alpha , A , lda , B , ldb , beta , C , ldc );
  return;
  }
 // stripes of whole micro-tiles, the last one with the rest
 const std::size_t w = ( len / s ) / unit * unit;
 ex.bulk( s , [ & ]( std::size_t q ) {
  const std::size_t a = q * w , b = q + 1 == s ? len : a + w;
  if( byRows )
   gemm( b - a , n , k , alpha , A + a * lda , lda , B , ldb , beta ,
         C + a * ldc , ldc );
  else
   gemm( m , b - a , k , alpha , A , lda , B + a , ldb , beta , C + a ,
         ldc );
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
