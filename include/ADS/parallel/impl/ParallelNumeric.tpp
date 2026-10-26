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
 using K = detail::GemmKernel< T >;
 constexpr std::size_t MR = K::MR , NR = K::NR;
 // the blocks of the sequential gemm(), whose loops these are
 constexpr std::size_t KC = 384 , MC = MR * ( 480 / MR ) ,
                       NC = NR * ( 4096 / NR );
 const std::size_t p = ex.concurrency();
 const std::size_t np = ( n + NR - 1 ) / NR * NR;
 // the small products and the degenerate ones go to the sequential
 // gemm(), as do the products too short in rows to give each thread a
 // micro-panel of A
 if( ( p == 1 ) || ( m == 0 ) || ( n == 0 ) || ( k == 0 ) ||
     ( alpha == T( 0 ) ) ||
     ( ( m * n * k <= std::size_t( 1 ) << 21 ) &&
       ( k * np <= std::size_t( 1 ) << 18 ) ) || ( m < 2 * MR ) ) {
  gemm( m , n , k , alpha , A , lda , B , ldb , beta , C , ldc );
  return;
  }
 // the rows of each thread: whole micro-panels, as evenly as they go
 const std::size_t panels = ( m + MR - 1 ) / MR ,
                   q = std::min( p , panels );
 const std::size_t nb = std::min( n , NC ) , kb = std::min( k , KC );
 // the panel of B in the buffer of the caller, aligned for the kernels
 // (the threads pack their blocks of A in the other buffer of each)
 T * const Bp = detail::gemmBuffer< T >( kb * ( ( nb + NR - 1 ) / NR * NR ) ,
                                         0 );
 for( std::size_t jc = 0 ; jc < n ; jc += NC ) {
  const std::size_t nc = std::min( NC , n - jc );
  for( std::size_t pc = 0 ; pc < k ; pc += KC ) {
   const std::size_t kc = std::min( KC , k - pc );
   const T b = pc == 0 ? beta : T( 1 );    // the later slices add
   // the panel of B packed once, by all the threads, each a range of
   // its micro-panels of NR columns
   const std::size_t cp = ( nc + NR - 1 ) / NR;
   parallelForBlocks( ex , 0 , cp , [ & ]( std::size_t c0 , std::size_t c1 ) {
    detail::gemmPackB< T , NR >( kc , std::min( nc , c1 * NR ) - c0 * NR ,
                                 B + pc * ldb + jc + c0 * NR , ldb ,
                                 Bp + c0 * NR * kc );
    } , 1 );
   // the rows of C, a range per thread, by blocks of MC rows whose A is
   // packed in the buffer of the thread
   ex.bulk( q , [ & ]( std::size_t t ) {
    const std::size_t r0 = panels * t / q * MR ,
                      r1 = std::min( m , panels * ( t + 1 ) / q * MR );
    for( std::size_t ic = r0 ; ic < r1 ; ic += MC ) {
     const std::size_t mc = std::min( MC , r1 - ic );
     T * Ap = detail::gemmBuffer< T >( kc * ( ( mc + MR - 1 ) / MR * MR ) ,
                                       1 );
     detail::gemmPackA< T , MR >( mc , kc , A + ic * lda + pc , lda , Ap );
     for( std::size_t jr = 0 ; jr < nc ; jr += NR ) {
      const std::size_t c = std::min< std::size_t >( NR , nc - jr );
      for( std::size_t ir = 0 ; ir < mc ; ir += MR ) {
       const std::size_t r = std::min< std::size_t >( MR , mc - ir );
       detail::gemmTile< T , true >( r , c , kc , Ap + ir * kc ,
                                     Bp + jr * kc ,
                                     C + ( ic + ir ) * ldc + jc + jr , ldc ,
                                     alpha , b , 0 );
       }
      }
     }
    } );
   }
  }
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
