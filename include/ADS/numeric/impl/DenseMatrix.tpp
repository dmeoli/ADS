/*--------------------------------------------------------------------------*/
/*-------------------------- File DenseMatrix.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of DenseMatrix and of its functions [see DenseMatrix.h]:
 * the product on the CBLAS or by the loops of the definition, the
 * product of Strassen and the transposition.
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

template< class T >
DenseMatrix< T >::DenseMatrix( const Matrix< T > & A )
 : f_m( A.size() ) , f_n( A.empty() ? 0 : A[ 0 ].size() )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( const auto & r ) {
  return( r.size() == f_n ); } ) );
 f_A.reserve( f_m * f_n );
 for( const auto & r : A ) f_A.insert( f_A.end() , r.begin() , r.end() );
 }

template< class T >
Matrix< T > DenseMatrix< T >::toMatrix( void ) const
{
 Matrix< T > A( f_m );
 for( std::size_t i = 0 ; i < f_m ; ++i )
  A[ i ].assign( f_A.begin() + std::ptrdiff_t( i * f_n ) ,
                 f_A.begin() + std::ptrdiff_t( ( i + 1 ) * f_n ) );
 return( A );
 }

namespace detail {

#if ADS_GEMM_BLAS
/// true if the product goes to the CBLAS: T is double or float, the
/// sizes fit the int of the CBLAS, and k > 0, since a CBLAS may refuse a
/// leading dimension 0 of A (that of Accelerate does), which an empty
/// common dimension allows
template< class T >
bool gemmOnCblas( std::size_t m , std::size_t n , std::size_t k ,
                  std::size_t lda , std::size_t ldb , std::size_t ldc )
{
 constexpr std::size_t top = std::size_t( INT_MAX );
 return( ( std::is_same_v< T , double > || std::is_same_v< T , float > ) &&
         ( k > 0 ) &&
         ( std::max( { m , n , k , lda , ldb , ldc } ) <= top ) );
 }

/// C <- alpha A B + beta C by cblas_dgemm() or cblas_sgemm(), all by rows
/// [see gemmOnCblas()]
template< class T >
void gemmCblas( std::size_t m , std::size_t n , std::size_t k , T alpha ,
                const T * A , std::size_t lda , const T * B ,
                std::size_t ldb , T beta , T * C , std::size_t ldc )
{
 if constexpr( std::is_same_v< T , double > )
  cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , int( m ) ,
               int( n ) , int( k ) , alpha , A , int( lda ) , B ,
               int( ldb ) , beta , C , int( ldc ) );
 else if constexpr( std::is_same_v< T , float > )
  cblas_sgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , int( m ) ,
               int( n ) , int( k ) , alpha , A , int( lda ) , B ,
               int( ldb ) , beta , C , int( ldc ) );
 }
#endif

/// C <- alpha A B + beta C by the loops of the definition, the rows of C
/// one at a time, the terms of each entry summed in the order of the
/// common index, by blocks of kGemmBlock columns of B and of kGemmBlock
/// rows of B, which stay in the cache while a row of C runs over them
template< class T >
void gemmBlocked( std::size_t m , std::size_t n , std::size_t k , T alpha ,
                  const T * A , std::size_t lda , const T * B ,
                  std::size_t ldb , T beta , T * C , std::size_t ldc )
{
 for( std::size_t i = 0 ; i < m ; ++i )     // C <- beta C, not read if 0
  for( std::size_t j = 0 ; j < n ; ++j ) {
   T & c = C[ i * ldc + j ];
   c = beta == T( 0 ) ? T( 0 ) : beta * c;
   }
 if( ( k == 0 ) || ( alpha == T( 0 ) ) )
  return;
 for( std::size_t jc = 0 ; jc < n ; jc += kGemmBlock ) {
  const std::size_t je = std::min( n , jc + kGemmBlock );
  for( std::size_t pc = 0 ; pc < k ; pc += kGemmBlock ) {
   const std::size_t pe = std::min( k , pc + kGemmBlock );
   for( std::size_t i = 0 ; i < m ; ++i ) {
    T * c = C + i * ldc;
    for( std::size_t p = pc ; p < pe ; ++p ) {   // c += alpha a_ip b_p
     const T a = alpha * A[ i * lda + p ];
     const T * b = B + p * ldb;
     for( std::size_t j = jc ; j < je ; ++j ) c[ j ] += a * b[ j ];
     }
    }
   }
  }
 }

}  // end( namespace detail )

template< class T >
void gemm( std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc )
{
 ADS_PRE( ( lda >= k ) && ( ldb >= n ) && ( ldc >= n ) );
 if( ( m == 0 ) || ( n == 0 ) )
  return;
#if ADS_GEMM_BLAS
 if( detail::gemmOnCblas< T >( m , n , k , lda , ldb , ldc ) ) {
  detail::gemmCblas( m , n , k , alpha , A , lda , B , ldb , beta , C ,
                     ldc );
  return;
  }
#endif
 detail::gemmBlocked( m , n , k , alpha , A , lda , B , ldb , beta , C ,
                      ldc );
 }

template< class T >
DenseMatrix< T > multiply( const DenseMatrix< T > & A ,
                           const DenseMatrix< T > & B )
{
 ADS_PRE( A.cols() == B.rows() );
 auto C = DenseMatrix< T >::uninitialized( A.rows() , B.cols() );
 gemm( A.rows() , B.cols() , A.cols() , T( 1 ) , A.data() , A.cols() ,
       B.data() , B.cols() , T( 0 ) , C.data() , C.cols() );
 return( C );
 }

namespace detail {

/// C <- A + s B, r x c, by rows with the leading dimensions
template< class T >
void strassenAdd( std::size_t r , std::size_t c , const T * A ,
                  std::size_t lda , const T * B , std::size_t ldb , T * C ,
                  std::size_t ldc , T s )
{
 for( std::size_t i = 0 ; i < r ; ++i ) {
  const T * a = A + i * lda , * b = B + i * ldb;
  T * x = C + i * ldc;
  if( s == T( 1 ) )
   for( std::size_t j = 0 ; j < c ; ++j ) x[ j ] = a[ j ] + b[ j ];
  else
   for( std::size_t j = 0 ; j < c ; ++j ) x[ j ] = a[ j ] - b[ j ];
  }
 }

/// C <- A B, m x k by k x n, by the variant of Winograd down to the cutoff
template< class T >
void strassenRec( std::size_t m , std::size_t n , std::size_t k ,
                  const T * A , std::size_t lda , const T * B ,
                  std::size_t ldb , T * C , std::size_t ldc ,
                  std::size_t cutoff )
{
 if( ( m < cutoff ) || ( n < cutoff ) || ( k < cutoff ) ) {
  gemm( m , n , k , T( 1 ) , A , lda , B , ldb , T( 0 ) , C , ldc );
  return;
  }
 const std::size_t m2 = m & ~std::size_t( 1 ) , n2 = n & ~std::size_t( 1 ) ,
                   k2 = k & ~std::size_t( 1 );
 const std::size_t hm = m2 / 2 , hn = n2 / 2 , hk = k2 / 2;
 const T * A11 = A , * A12 = A + hk , * A21 = A + hm * lda ,
         * A22 = A21 + hk;
 const T * B11 = B , * B12 = B + hn , * B21 = B + hk * ldb ,
         * B22 = B21 + hn;
 T * C11 = C , * C12 = C + hn , * C21 = C + hm * ldc , * C22 = C21 + hn;
 std::vector< T , DefaultInitAllocator< T > > X( hm * hk ) , Y( hk * hn ) ,
                                              U( hm * hn );
 T * x = X.data() , * y = Y.data() , * u = U.data();
 const T one( 1 ) , minus( -1 );
 auto mul = [ & ]( const T * a , std::size_t la , const T * b ,
                   std::size_t lb , T * c , std::size_t lc ) {
  strassenRec( hm , hn , hk , a , la , b , lb , c , lc , cutoff ); };
 strassenAdd( hm , hk , A11 , lda , A21 , lda , x , hk , minus );   // S3
 strassenAdd( hk , hn , B22 , ldb , B12 , ldb , y , hn , minus );   // T3
 mul( x , hk , y , hn , C21 , ldc );                                // P7
 strassenAdd( hm , hk , A21 , lda , A22 , lda , x , hk , one );     // S1
 strassenAdd( hk , hn , B12 , ldb , B11 , ldb , y , hn , minus );   // T1
 mul( x , hk , y , hn , C22 , ldc );                                // P5
 strassenAdd( hm , hk , x , hk , A11 , lda , x , hk , minus );      // S2
 strassenAdd( hk , hn , B22 , ldb , y , hn , y , hn , minus );      // T2
 mul( x , hk , y , hn , C12 , ldc );                                // P6
 strassenAdd( hm , hk , A12 , lda , x , hk , x , hk , minus );      // S4
 mul( x , hk , B22 , ldb , C11 , ldc );                             // P3
 mul( A11 , lda , B11 , ldb , u , hn );                             // P1
 strassenAdd( hm , hn , C12 , ldc , u , hn , C12 , ldc , one );     // U2
 strassenAdd( hm , hn , C21 , ldc , C12 , ldc , C21 , ldc , one );  // U3
 strassenAdd( hm , hn , C12 , ldc , C22 , ldc , C12 , ldc , one );  // U4
 strassenAdd( hm , hn , C22 , ldc , C21 , ldc , C22 , ldc , one );  // U7
 strassenAdd( hm , hn , C12 , ldc , C11 , ldc , C12 , ldc , one );  // U5
 strassenAdd( hk , hn , y , hn , B21 , ldb , y , hn , minus );      // T4
 mul( A22 , lda , y , hn , C11 , ldc );                             // P4
 strassenAdd( hm , hn , C21 , ldc , C11 , ldc , C21 , ldc , minus );   // U6
 mul( A12 , lda , B21 , ldb , C11 , ldc );                          // P2
 strassenAdd( hm , hn , C11 , ldc , u , hn , C11 , ldc , one );     // U1
 // the odd row, column and inner term, by gemm()
 if( k2 < k )                               // C[ 0 , m2 ) x [ 0 , n2 )
  gemm( m2 , n2 , std::size_t( 1 ) , one , A + k2 , lda , B + k2 * ldb ,
        ldb , one , C , ldc );
 if( n2 < n )                               // the last column
  gemm( m2 , std::size_t( 1 ) , k , one , A , lda , B + n2 , ldb , T( 0 ) ,
        C + n2 , ldc );
 if( m2 < m )                               // the last row
  gemm( std::size_t( 1 ) , n , k , one , A + m2 * lda , lda , B , ldb ,
        T( 0 ) , C + m2 * ldc , ldc );
 }

}  // end( namespace detail )

template< class T >
DenseMatrix< T > multiplyStrassen( const DenseMatrix< T > & A ,
                                   const DenseMatrix< T > & B ,
                                   std::size_t cutoff )
{
 ADS_PRE( ( A.cols() == B.rows() ) && ( cutoff >= 2 ) );
 auto C = DenseMatrix< T >::uninitialized( A.rows() , B.cols() );
 if( ( C.rows() > 0 ) && ( C.cols() > 0 ) )
  detail::strassenRec( A.rows() , B.cols() , A.cols() , A.data() ,
                       A.cols() , B.data() , B.cols() , C.data() ,
                       C.cols() , cutoff );
 return( C );
 }

template< class T >
DenseMatrix< T > transpose( const DenseMatrix< T > & A )
{
 const std::size_t m = A.rows() , n = A.cols() , b = 32;
 auto B = DenseMatrix< T >::uninitialized( n , m );
 const T * a = A.data();
 T * t = B.data();
 for( std::size_t jb = 0 ; jb < n ; jb += b )   // blocks b x b, which stay
  for( std::size_t ib = 0 ; ib < m ; ib += b ) {   // in the first level
   const std::size_t ie = std::min( m , ib + b ) , je = std::min( n , jb + b );
   for( std::size_t j = jb ; j < je ; ++j )     // the writes by rows
    for( std::size_t i = ib ; i < ie ; ++i ) t[ j * m + i ] = a[ i * n + j ];
   }
 return( B );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File DenseMatrix.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
