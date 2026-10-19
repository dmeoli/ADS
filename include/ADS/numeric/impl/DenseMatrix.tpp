/*--------------------------------------------------------------------------*/
/*-------------------------- File DenseMatrix.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of DenseMatrix and of its functions [see DenseMatrix.h]:
 * the micro-kernels of the product for each set of instructions, the
 * packing of the blocks and the five loops around them.
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

/// a micro-kernel, a function of its own: inlined into the loops, its
/// accumulators may not all stay in registers
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_GEMM_KERNEL __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_GEMM_KERNEL __declspec( noinline )
#else
 #define ADS_GEMM_KERNEL
#endif

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

/*--------------------------------------------------------------------------*/
/*----------------------------- MICRO-KERNELS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the shape of the micro-tile of T and its vectors: the portable one, MR x
/// NR scalars that the compiler may vectorize
template< class T >
struct GemmKernel {
 static constexpr bool simd = false;
 static constexpr int MR = 4 , NR = 4 , W = 1;
 };

#if defined( __AVX512F__ ) && defined( ADS_GEMM_AVX512 )
template<>
struct GemmKernel< double > {               // 12 x 16: 24 registers of 8
 static constexpr bool simd = true;
 static constexpr int MR = 12 , NR = 16 , W = 8;
 using V = __m512d;
 static V zero( void ) { return( _mm512_setzero_pd() ); }
 static V load( const double * p ) { return( _mm512_load_pd( p ) ); }
 static V loadu( const double * p ) { return( _mm512_loadu_pd( p ) ); }
 static void storeu( double * p , V v ) { _mm512_storeu_pd( p , v ); }
 static V bcast( double x ) { return( _mm512_set1_pd( x ) ); }
 static V fma( V a , V b , V c ) { return( _mm512_fmadd_pd( a , b , c ) ); }
 static V mul( V a , V b ) { return( _mm512_mul_pd( a , b ) ); }
 };
template<>
struct GemmKernel< float > {                // 12 x 32: 24 registers of 16
 static constexpr bool simd = true;
 static constexpr int MR = 12 , NR = 32 , W = 16;
 using V = __m512;
 static V zero( void ) { return( _mm512_setzero_ps() ); }
 static V load( const float * p ) { return( _mm512_load_ps( p ) ); }
 static V loadu( const float * p ) { return( _mm512_loadu_ps( p ) ); }
 static void storeu( float * p , V v ) { _mm512_storeu_ps( p , v ); }
 static V bcast( float x ) { return( _mm512_set1_ps( x ) ); }
 static V fma( V a , V b , V c ) { return( _mm512_fmadd_ps( a , b , c ) ); }
 static V mul( V a , V b ) { return( _mm512_mul_ps( a , b ) ); }
 };
#elif defined( __AVX2__ ) && defined( __FMA__ )
template<>
struct GemmKernel< double > {               // 6 x 16: 24 registers of 4
 static constexpr bool simd = true;         // with 2 of B and 1 of A, 15
 static constexpr int MR = 6 , NR = 16 , W = 4;   // of the 16 of AVX2
 using V = __m256d;
 static V zero( void ) { return( _mm256_setzero_pd() ); }
 static V load( const double * p ) { return( _mm256_load_pd( p ) ); }
 static V loadu( const double * p ) { return( _mm256_loadu_pd( p ) ); }
 static void storeu( double * p , V v ) { _mm256_storeu_pd( p , v ); }
 static V bcast( double x ) { return( _mm256_set1_pd( x ) ); }
 static V fma( V a , V b , V c ) { return( _mm256_fmadd_pd( a , b , c ) ); }
 static V mul( V a , V b ) { return( _mm256_mul_pd( a , b ) ); }
 };
template<>
struct GemmKernel< float > {
 static constexpr bool simd = true;
 #ifdef __AVX512F__
 static constexpr int MR = 6 , NR = 32 , W = 8;   // 32 registers
 #else
 static constexpr int MR = 6 , NR = 16 , W = 8;
 #endif
 using V = __m256;
 static V zero( void ) { return( _mm256_setzero_ps() ); }
 static V load( const float * p ) { return( _mm256_load_ps( p ) ); }
 static V loadu( const float * p ) { return( _mm256_loadu_ps( p ) ); }
 static void storeu( float * p , V v ) { _mm256_storeu_ps( p , v ); }
 static V bcast( float x ) { return( _mm256_set1_ps( x ) ); }
 static V fma( V a , V b , V c ) { return( _mm256_fmadd_ps( a , b , c ) ); }
 static V mul( V a , V b ) { return( _mm256_mul_ps( a , b ) ); }
 };
#endif

/// the micro-tile C <- alpha a b + beta C, a the packed MR x kc and b the
/// packed kc x NR, C with the leading dimension ldc, of which only the
/// first r rows and c columns exist; C not read if beta = 0; if not
/// PackedA, a is the matrix itself, of leading dimension lda, and if not
/// PackedB, b is, of leading dimension ldb; the tile has R <= MR rows and
/// CV <= NR / W vectors of columns
template< class T , bool PackedA = true , bool PackedB = true ,
          int R = GemmKernel< T >::MR ,
          int CV = GemmKernel< T >::NR / GemmKernel< T >::W >
ADS_GEMM_KERNEL void gemmMicro( std::size_t kc , const T * a , const T * b ,
                                T * C , std::size_t ldc , T alpha , T beta ,
                                std::size_t r , std::size_t c ,
                                std::size_t lda = 0 , std::size_t ldb = 0 )
{
 using K = GemmKernel< T >;
 constexpr int MR = K::MR , NR = K::NR;
 if constexpr( K::simd ) {
  using V = typename K::V;
  constexpr int NV = CV;
  V acc[ R ][ NV ];
#pragma GCC unroll 16
  for( int i = 0 ; i < R ; ++i )
#pragma GCC unroll 4
   for( int v = 0 ; v < NV ; ++v ) acc[ i ][ v ] = K::zero();
  for( std::size_t p = 0 ; p < kc ; ++p ) {
   V bv[ NV ];
#pragma GCC unroll 4
   for( int v = 0 ; v < NV ; ++v )
    bv[ v ] = PackedB ? K::load( b + v * K::W ) : K::loadu( b + v * K::W );
#pragma GCC unroll 16
   for( int i = 0 ; i < R ; ++i ) {
    const V av = K::bcast( PackedA ? a[ i ] : a[ std::size_t( i ) * lda ] );
#pragma GCC unroll 4
    for( int v = 0 ; v < NV ; ++v )
     acc[ i ][ v ] = K::fma( av , bv[ v ] , acc[ i ][ v ] );
    }
   a += PackedA ? std::size_t( MR ) : 1;
   b += PackedB ? std::size_t( NR ) : ldb;
   }
  const V al = K::bcast( alpha ) , be = K::bcast( beta );
  if( ( r == std::size_t( R ) ) && ( c == std::size_t( NV * K::W ) ) ) {
#pragma GCC unroll 16
   for( int i = 0 ; i < R ; ++i )
#pragma GCC unroll 4
    for( int v = 0 ; v < NV ; ++v ) {
     T * p = C + std::size_t( i ) * ldc + std::size_t( v * K::W );
     K::storeu( p , beta == T( 0 ) ? K::mul( al , acc[ i ][ v ] )
                                   : K::fma( be , K::loadu( p ) ,
                                             K::mul( al , acc[ i ][ v ] ) ) );
     }
   return;
   }
  alignas( 64 ) T t[ K::W ];                // a border: by a row buffer
  for( std::size_t i = 0 ; i < r ; ++i )
   for( int v = 0 ; v < NV ; ++v ) {
    const std::size_t j0 = std::size_t( v * K::W );
    if( j0 >= c ) break;
    K::storeu( t , K::mul( al , acc[ i ][ v ] ) );
    T * p = C + i * ldc + j0;
    for( std::size_t j = 0 ; ( j < std::size_t( K::W ) ) && ( j0 + j < c ) ;
         ++j )
     p[ j ] = beta == T( 0 ) ? t[ j ] : t[ j ] + beta * p[ j ];
    }
  }
 else {
  constexpr int NC = CV * K::W;
  T acc[ R ][ NC ] = {};
  for( std::size_t p = 0 ; p < kc ; ++p ) {
   for( int i = 0 ; i < R ; ++i )
    for( int j = 0 ; j < NC ; ++j )
     acc[ i ][ j ] += ( PackedA ? a[ i ] : a[ std::size_t( i ) * lda ] ) *
                      b[ j ];
   a += PackedA ? std::size_t( MR ) : 1;
   b += PackedB ? std::size_t( NR ) : ldb;
   }
  for( std::size_t i = 0 ; i < r ; ++i )
   for( std::size_t j = 0 ; j < c ; ++j ) {
    T & x = C[ i * ldc + j ];
    x = beta == T( 0 ) ? alpha * acc[ i ][ j ]
                       : alpha * acc[ i ][ j ] + beta * x;
    }
  }
 }

/// a tile of r <= MR rows and c <= NR columns, from the packed micro-panel
/// b of B and from A, packed too if PackedA, by the kernel of r rows (if A
/// is not packed) and of the fewest vectors of columns
template< class T , bool PackedA , int R = GemmKernel< T >::MR ,
          int CV = GemmKernel< T >::NR / GemmKernel< T >::W >
void gemmTile( std::size_t r , std::size_t c , std::size_t k , const T * A ,
               const T * b , T * C , std::size_t ldc , T alpha , T beta ,
               std::size_t lda )
{
 constexpr std::size_t W = std::size_t( GemmKernel< T >::W );
 if constexpr( ( R >= 1 ) && ( CV >= 1 ) ) {
  if( ! PackedA && ( r < std::size_t( R ) ) )
   gemmTile< T , PackedA , R - 1 , CV >( r , c , k , A , b , C , ldc ,
                                         alpha , beta , lda );
  else if( ( CV > 1 ) && ( c <= ( CV - 1 ) * W ) )
   gemmTile< T , PackedA , R , CV - 1 >( r , c , k , A , b , C , ldc ,
                                         alpha , beta , lda );
  else
   gemmMicro< T , PackedA , true , R , CV >( k , A , b , C , ldc , alpha ,
                                             beta , r , c , lda );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- PACKING ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the block mc x kc of A in micro-panels of MR rows, each by columns (the
/// MR entries of a column together), the rows past mc zero
template< class T , int MR >
void gemmPackA( std::size_t mc , std::size_t kc , const T * A ,
                std::size_t lda , T * Ap )
{
 for( std::size_t ir = 0 ; ir < mc ; ir += MR , Ap += MR * kc ) {
  const std::size_t r = std::min< std::size_t >( MR , mc - ir );
  for( std::size_t i = 0 ; i < r ; ++i ) {
   const T * a = A + ( ir + i ) * lda;
   for( std::size_t p = 0 ; p < kc ; ++p ) Ap[ p * MR + i ] = a[ p ];
   }
  for( std::size_t i = r ; i < std::size_t( MR ) ; ++i )
   for( std::size_t p = 0 ; p < kc ; ++p ) Ap[ p * MR + i ] = T( 0 );
  }
 }

/// the panel kc x nc of B in micro-panels of NR columns, each by rows, the
/// columns past nc zero
template< class T , int NR >
void gemmPackB( std::size_t kc , std::size_t nc , const T * B ,
                std::size_t ldb , T * Bp )
{
 for( std::size_t jr = 0 ; jr < nc ; jr += NR , Bp += NR * kc ) {
  const std::size_t c = std::min< std::size_t >( NR , nc - jr );
  for( std::size_t p = 0 ; p < kc ; ++p ) {
   const T * b = B + p * ldb + jr;
   T * d = Bp + p * NR;
   if( c == std::size_t( NR ) )
    std::copy( b , b + NR , d );
   else {
    std::copy( b , b + c , d );
    std::fill( d + c , d + NR , T( 0 ) );
    }
   }
  }
 }

/// a buffer of n items aligned to 64 bytes, kept by the thread between the
/// calls and grown when needed
template< class T >
T * gemmBuffer( std::size_t n , int which )
{
 struct Buffer {
  T * p = nullptr;
  std::size_t n = 0;
  ~Buffer() {
   if( p ) {
    if constexpr( ! std::is_trivially_destructible_v< T > )
     std::destroy_n( p , n );
    ::operator delete( p , std::align_val_t( 64 ) );
    }
   }
  };
 thread_local Buffer B[ 2 ];
 Buffer & b = B[ which ];
 if( b.n < n ) {
  Buffer fresh;
  fresh.p = static_cast< T * >( ::operator new( n * sizeof( T ) ,
                                                std::align_val_t( 64 ) ) );
  std::uninitialized_value_construct_n( fresh.p , n );
  fresh.n = n;
  std::swap( b.p , fresh.p );
  std::swap( b.n , fresh.n );
  }
 return( b.p );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- THE LOOPS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
void gemm( std::size_t m , std::size_t n , std::size_t k , T alpha ,
           const T * A , std::size_t lda , const T * B , std::size_t ldb ,
           T beta , T * C , std::size_t ldc )
{
 ADS_PRE( ( lda >= k ) && ( ldb >= n ) && ( ldc >= n ) );
 using K = detail::GemmKernel< T >;
 constexpr int MR = K::MR , NR = K::NR;
 // the blocks: KC rows of the micro-panels of B in the first level
 // cache with those of A, MC x KC of A in the second, KC x NC of B in the
 // last, multiples of the micro-tile
 constexpr std::size_t KC = 384 , MC = MR * ( 480 / MR ) ,
                       NC = NR * ( 4096 / NR );
 if( ( m == 0 ) || ( n == 0 ) )
  return;
 if( ( k == 0 ) || ( alpha == T( 0 ) ) ) {  // C <- beta C
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j ) {
    T & c = C[ i * ldc + j ];
    c = beta == T( 0 ) ? T( 0 ) : beta * c;
    }
  return;
  }
 const std::size_t np = ( n + NR - 1 ) / NR * NR;   // n padded
 if( ( m * n * k <= std::size_t( 1 ) << 21 ) &&
     ( k * np <= std::size_t( 1 ) << 18 ) ) {
  // a small product: B packed whole, its last micro-panel padded with
  // zeros, and the tiles of A straight from it, which stays in the cache,
  // those of the last rows by the kernel of their number
  T * Bp = detail::gemmBuffer< T >( k * np , 0 );
  detail::gemmPackB< T , NR >( k , n , B , ldb , Bp );
  for( std::size_t i = 0 ; i < m ; i += MR ) {
   const std::size_t r = std::min< std::size_t >( MR , m - i );
   for( std::size_t j = 0 ; j < n ; j += NR )
    detail::gemmTile< T , false >( r , std::min< std::size_t >( NR , n - j ) ,
                                   k , A + i * lda , Bp + j * k ,
                                   C + i * ldc + j , ldc , alpha , beta ,
                                   lda );
   }
  return;
  }
 const std::size_t nb = std::min( n , NC ) , mb = std::min( m , MC ) ,
                   kb = std::min( k , KC );
 T * Bp = detail::gemmBuffer< T >( kb * ( ( nb + NR - 1 ) / NR * NR ) , 0 );
 T * Ap = detail::gemmBuffer< T >( kb * ( ( mb + MR - 1 ) / MR * MR ) , 1 );
 for( std::size_t jc = 0 ; jc < n ; jc += NC ) {
  const std::size_t nc = std::min( NC , n - jc );
  for( std::size_t pc = 0 ; pc < k ; pc += KC ) {
   const std::size_t kc = std::min( KC , k - pc );
   const T b = pc == 0 ? beta : T( 1 );     // the later slices add
   detail::gemmPackB< T , NR >( kc , nc , B + pc * ldb + jc , ldb , Bp );
   for( std::size_t ic = 0 ; ic < m ; ic += MC ) {
    const std::size_t mc = std::min( MC , m - ic );
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
   }
  }
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
