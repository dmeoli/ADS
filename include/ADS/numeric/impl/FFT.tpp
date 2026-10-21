/*--------------------------------------------------------------------------*/
/*------------------------------ File FFT.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FFT.h, with their contracts.
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

/// a function inlined even where the compiler would not, so that the
/// items of a codelet stay in registers; MSVC is left to choose, since
/// forcing it through the unrolled codelets does not end
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FFT_INLINE inline __attribute__(( always_inline ))
#else
 #define ADS_FFT_INLINE inline
#endif

/// a hint to bring the line of p into the cache, for reading or for
/// writing, or nothing
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FFT_PREFETCH( p , w ) \
  __builtin_prefetch( ( const void * )( p ) , w )
#elif ADS_FFT_INTRINSICS
 #define ADS_FFT_PREFETCH( p , w ) \
  _mm_prefetch( ( const char * )( p ) , _MM_HINT_T0 )
#else
 #define ADS_FFT_PREFETCH( p , w ) ( ( void ) 0 )
#endif

/// a lambda inlined where it is called, as ADS_FFT_INLINE
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FFT_LAMBDA __attribute__(( always_inline ))
#else
 #define ADS_FFT_LAMBDA
#endif

/// a loop unrolled in full, its bounds known when compiled
#if defined( __clang__ )
 #define ADS_FFT_UNROLL _Pragma( "clang loop unroll(full)" )
#elif defined( __GNUC__ )
 #define ADS_FFT_UNROLL _Pragma( "GCC unroll 64" )
#else
 #define ADS_FFT_UNROLL
#endif

/// a function kept out of its callers, so that its code does not depend
/// on theirs
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FFT_NOINLINE inline __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_FFT_NOINLINE inline __declspec( noinline )
#else
 #define ADS_FFT_NOINLINE inline
#endif

/// the shuffles of the lanes of the vector extensions by
/// __builtin_shufflevector where the compiler has it, lane by lane
/// elsewhere
#if ( ADS_FFT_SIMD > 1 ) && ! ADS_FFT_INTRINSICS && \
    ( defined( __clang__ ) || ( defined( __GNUC__ ) && ( __GNUC__ >= 12 ) ) )
 #define ADS_FFT_SHUFFLE 1
#else
 #define ADS_FFT_SHUFFLE 0
#endif

namespace ADS {

namespace detail {

/*--------------------------------------------------------------------------*/
/*------------------------------ THE LANES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// W numbers of type T as an array, with the operations item by item: the
/// lanes where the compiler has no vector extensions nor intrinsics (or
/// ADS_FFT_SIMD is 0); aligned to their size and operated on by loops,
/// which the compilers turn into vector instructions where they can
template< class T , std::size_t W >
struct alignas( sizeof( T ) * W ) FFTLanes {
 T v[ W ];

 T & operator[]( std::size_t l ) { return( v[ l ] ); }

 T operator[]( std::size_t l ) const { return( v[ l ] ); }

 friend ADS_FFT_INLINE FFTLanes operator+( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  FFTLanes y;
  for( std::size_t l = 0 ; l < W ; ++l ) y.v[ l ] = a.v[ l ] + b.v[ l ];
  return( y );
  }

 friend ADS_FFT_INLINE FFTLanes operator-( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  FFTLanes y;
  for( std::size_t l = 0 ; l < W ; ++l ) y.v[ l ] = a.v[ l ] - b.v[ l ];
  return( y );
  }

 friend ADS_FFT_INLINE FFTLanes operator*( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  FFTLanes y;
  for( std::size_t l = 0 ; l < W ; ++l ) y.v[ l ] = a.v[ l ] * b.v[ l ];
  return( y );
  }

 friend ADS_FFT_INLINE FFTLanes operator-( const FFTLanes & a ) {
  FFTLanes y;
  for( std::size_t l = 0 ; l < W ; ++l ) y.v[ l ] = - a.v[ l ];
  return( y );
  }
 };

#if ADS_FFT_INTRINSICS
/// the lanes of W numbers of type T in a register of SSE2 or AVX, by the
/// intrinsics of x86: the lanes of the compilers with no vector extensions
/// (e.g., MSVC), a thin layer that has the operations of those extensions
template< class T , std::size_t W >
struct FFTVec;

#define ADS_FFT_VEC( T , W , N , P , S )                                  \
template<> struct FFTVec< T , W > {                                       \
 N v;                                                                     \
 static ADS_FFT_INLINE FFTVec splat( T x ) {                              \
  return( FFTVec{ P##_set1_##S( x ) } ); }                                \
 static ADS_FFT_INLINE FFTVec load( const T * p ) {                       \
  return( FFTVec{ P##_loadu_##S( p ) } ); }                               \
 ADS_FFT_INLINE void store( T * p ) const { P##_storeu_##S( p , v ); }    \
 T operator[]( std::size_t l ) const {                                    \
  return( reinterpret_cast< const T * >( & v )[ l ] ); }                  \
 T & operator[]( std::size_t l ) {                                        \
  return( reinterpret_cast< T * >( & v )[ l ] ); }                        \
 friend ADS_FFT_INLINE FFTVec operator+( FFTVec a , FFTVec b ) {          \
  return( FFTVec{ P##_add_##S( a.v , b.v ) } ); }                         \
 friend ADS_FFT_INLINE FFTVec operator-( FFTVec a , FFTVec b ) {          \
  return( FFTVec{ P##_sub_##S( a.v , b.v ) } ); }                         \
 friend ADS_FFT_INLINE FFTVec operator*( FFTVec a , FFTVec b ) {          \
  return( FFTVec{ P##_mul_##S( a.v , b.v ) } ); }                         \
 friend ADS_FFT_INLINE FFTVec operator-( FFTVec a ) {                     \
  return( FFTVec{ P##_xor_##S( a.v , P##_set1_##S( T( -0.0 ) ) ) } ); }   \
 };

ADS_FFT_VEC( double , 2 , __m128d , _mm , pd )
ADS_FFT_VEC( float , 4 , __m128 , _mm , ps )
#ifdef __AVX__
ADS_FFT_VEC( double , 4 , __m256d , _mm256 , pd )
ADS_FFT_VEC( float , 8 , __m256 , _mm256 , ps )
#endif
#undef ADS_FFT_VEC

/// whether there is a register of W numbers of type T
template< class T , std::size_t W >
inline constexpr bool fftHasVec = ( sizeof( T ) * W == 16 )
#ifdef __AVX__
                                  || ( sizeof( T ) * W == 32 )
#endif
                                  ;
#endif

/// the lanes of width W of numbers of type T: a number if W is 1, a vector
/// of the extensions of GCC and Clang if ADS_FFT_SIMD > 1, a register of
/// SSE2 or AVX if ADS_FFT_INTRINSICS, FFTLanes otherwise
template< class T , std::size_t W >
struct FFTLaneType {
 using type = FFTLanes< T , W >;
 };

template<> struct FFTLaneType< double , 1 > {
 using type = double;
 };

template<> struct FFTLaneType< float , 1 > {
 using type = float;
 };

#if ( ADS_FFT_SIMD > 1 ) && ! ADS_FFT_INTRINSICS
template<> struct FFTLaneType< double , 2 > {
 typedef double type __attribute__(( vector_size( 16 ) ));
 };

template<> struct FFTLaneType< double , 4 > {
 typedef double type __attribute__(( vector_size( 32 ) ));
 };

template<> struct FFTLaneType< double , 8 > {
 typedef double type __attribute__(( vector_size( 64 ) ));
 };

template<> struct FFTLaneType< float , 2 > {
 typedef float type __attribute__(( vector_size( 8 ) ));
 };

template<> struct FFTLaneType< float , 4 > {
 typedef float type __attribute__(( vector_size( 16 ) ));
 };

template<> struct FFTLaneType< float , 8 > {
 typedef float type __attribute__(( vector_size( 32 ) ));
 };

template<> struct FFTLaneType< float , 16 > {
 typedef float type __attribute__(( vector_size( 64 ) ));
 };
#endif

#if ADS_FFT_INTRINSICS
template<> struct FFTLaneType< double , 2 > {
 using type = FFTVec< double , 2 >;
 };

template<> struct FFTLaneType< float , 4 > {
 using type = FFTVec< float , 4 >;
 };

 #ifdef __AVX__
template<> struct FFTLaneType< double , 4 > {
 using type = FFTVec< double , 4 >;
 };

template<> struct FFTLaneType< float , 8 > {
 using type = FFTVec< float , 8 >;
 };
 #endif
#endif

template< class T , std::size_t W >
using FFTLane = typename FFTLaneType< T , W >::type;

/// whether the lanes of width W of T are a register of the intrinsics
template< class T , std::size_t W >
inline constexpr bool fftIsVec =
#if ADS_FFT_INTRINSICS
 W > 1 && fftHasVec< T , W >;
#else
 false;
#endif

/// whether the lanes of width W of T are a vector of the extensions
template< class T , std::size_t W >
inline constexpr bool fftIsExt = W > 1 && ( ADS_FFT_SIMD > 1 ) &&
                                 ! ADS_FFT_INTRINSICS &&
                                 sizeof( T ) * W <= 64;

/// the widest lanes of numbers of type T: those of ADS_FFT_SIMD doubles,
/// or of 4 doubles (8 floats) by FFTLanes if it is 0
template< class T >
inline constexpr std::size_t fftWidthOf =
 ( ADS_FFT_SIMD > 1 ? ADS_FFT_SIMD : 4 ) * 8 / sizeof( T );

/// the widest lanes of doubles
inline constexpr std::size_t fftWidth = fftWidthOf< double >;

/// a complex number in each lane: the real parts and the imaginary ones
template< class V >
struct FFTCx {
 V r , i;
 };

template< class V >
ADS_FFT_INLINE FFTCx< V > operator+( const FFTCx< V > & a ,
                                     const FFTCx< V > & b )
{
 return( FFTCx< V >{ a.r + b.r , a.i + b.i } );
 }

template< class V >
ADS_FFT_INLINE FFTCx< V > operator-( const FFTCx< V > & a ,
                                     const FFTCx< V > & b )
{
 return( FFTCx< V >{ a.r - b.r , a.i - b.i } );
 }

/// W lanes all equal to x
template< class T , std::size_t W >
ADS_FFT_INLINE FFTLane< T , W > splat( T x )
{
 using V = FFTLane< T , W >;
 if constexpr( W == 1 ) return( x );
 else if constexpr( fftIsVec< T , W > ) return( V::splat( x ) );
 else {
  V v{};
  for( std::size_t l = 0 ; l < W ; ++l ) v[ l ] = x;
  return( v );
  }
 }

/// the W lanes at p, which need not be aligned
template< class T , std::size_t W >
ADS_FFT_INLINE FFTLane< T , W > loadLanes( const T * p )
{
 using V = FFTLane< T , W >;
 if constexpr( fftIsVec< T , W > ) return( V::load( p ) );
 else {
  V v;
  std::memcpy( & v , p , sizeof( v ) );
  return( v );
  }
 }

template< class T , std::size_t W >
ADS_FFT_INLINE void storeLanes( T * p , const FFTLane< T , W > & v )
{
 if constexpr( fftIsVec< T , W > ) v.store( p );
 else std::memcpy( p , & v , sizeof( v ) );
 }

/// a times w = c + i s, or times its conjugate if inv
template< bool inv , class V >
ADS_FFT_INLINE FFTCx< V > cmul( const FFTCx< V > & a , const V & c ,
                                const V & s )
{
 if constexpr( inv )
  return( FFTCx< V >{ a.r * c + a.i * s , a.i * c - a.r * s } );
 else
  return( FFTCx< V >{ a.r * c - a.i * s , a.i * c + a.r * s } );
 }

/// a times -i, or times i if inv
template< bool inv , class V >
ADS_FFT_INLINE FFTCx< V > mulMinusI( const FFTCx< V > & a )
{
 if constexpr( inv ) return( FFTCx< V >{ - a.i , a.r } );
 else return( FFTCx< V >{ a.i , - a.r } );
 }

/// the lane of the complex number l of W at p in loadComplex() and
/// storeComplex(), the order in which the instructions of x86 leave them:
/// for doubles, the first half in the even lanes and the second in the
/// odd ones (unpcklpd); for floats, those of each half of 128 bits after
/// those of the first quarter (shufps)
template< class T , std::size_t W >
constexpr std::size_t fftLane( std::size_t l )
{
 if constexpr( sizeof( T ) == 8 ) return( l % 2 ? W / 2 + l / 2 : l / 2 );
 else if constexpr( W < 4 ) return( l );
 else return( ( l & 1 ) + ( ( l >> 2 ) << 1 ) + ( ( l >> 1 ) & 1 ) * W / 2 );
 }

/// the lane of the complex number c in loadComplex(), the inverse of
/// fftLane()
template< class T , std::size_t W >
constexpr std::size_t fftLaneOf( std::size_t c )
{
 for( std::size_t l = 0 ; l < W ; ++l )
  if( fftLane< T , W >( l ) == c ) return( l );
 return( 0 );
 }

#if ADS_FFT_SHUFFLE
/// the lanes of the shuffles of the vector extensions: in deinterleave(),
/// the real part of the number fftLane( l ) of the two vectors u , v, the
/// imaginary one if im; in interleave(), the lane l of the vectors u (v if
/// hi) of the numbers alternated, from those of re and im
template< class T , std::size_t W >
constexpr int fftSplit( std::size_t l , bool im )
{
 return( int( 2 * fftLane< T , W >( l ) + im ) );
 }

template< class T , std::size_t W >
constexpr int fftJoin( std::size_t l , bool hi )
{
 const std::size_t e = l + ( hi ? W : 0 );
 return( int( fftLaneOf< T , W >( e / 2 ) + ( e % 2 ? W : 0 ) ) );
 }

/// the lanes l of a with l & h = 0, those of b moved to the others; then
/// the lanes of a moved to those with l & h = 0, and the rest of b
template< std::size_t W >
constexpr int fftSwapLo( std::size_t l , std::size_t h )
{
 return( int( l & h ? W + l - h : l ) );
 }

template< std::size_t W >
constexpr int fftSwapHi( std::size_t l , std::size_t h )
{
 return( int( l & h ? W + l : l + h ) );
 }

template< class T , std::size_t W , class V , std::size_t ... L >
ADS_FFT_INLINE FFTCx< V > deinterleave( const V & u , const V & v ,
                                        std::index_sequence< L ... > )
{
 return( FFTCx< V >{
          __builtin_shufflevector( u , v , fftSplit< T , W >( L , 0 ) ... ) ,
          __builtin_shufflevector( u , v ,
                                   fftSplit< T , W >( L , 1 ) ... ) } );
 }

template< class T , std::size_t W , class V , std::size_t ... L >
ADS_FFT_INLINE void interleave( const FFTCx< V > & x , V & u , V & v ,
                                std::index_sequence< L ... > )
{
 u = __builtin_shufflevector( x.r , x.i , fftJoin< T , W >( L , 0 ) ... );
 v = __builtin_shufflevector( x.r , x.i , fftJoin< T , W >( L , 1 ) ... );
 }

template< std::size_t W , std::size_t h , class V , std::size_t ... L >
ADS_FFT_INLINE void swapBlocks( V & a , V & b ,
                                std::index_sequence< L ... > )
{
 const V lo = __builtin_shufflevector( a , b , fftSwapLo< W >( L , h ) ... );
 b = __builtin_shufflevector( a , b , fftSwapHi< W >( L , h ) ... );
 a = lo;
 }

template< std::size_t W , std::size_t h , class V >
ADS_FFT_INLINE void transposeStage( V * v )
{
 if constexpr( h > 0 ) {
  for( std::size_t a = 0 ; a < W ; ++a )
   if( ! ( a & h ) )
    swapBlocks< W , h >( v[ a ] , v[ a + h ] ,
                         std::make_index_sequence< W >() );
  transposeStage< W , h / 2 >( v );
  }
 }
#endif

#if ADS_FFT_INTRINSICS
/// the numbers alternated in u , v into the real parts and the imaginary
/// ones, in the order of fftLane(), and back, by the shuffles of x86
ADS_FFT_INLINE FFTCx< FFTVec< double , 2 > > deinterleave(
                    FFTVec< double , 2 > u , FFTVec< double , 2 > v )
{
 FFTCx< FFTVec< double , 2 > > x;
 x.r.v = _mm_unpacklo_pd( u.v , v.v );
 x.i.v = _mm_unpackhi_pd( u.v , v.v );
 return( x );
 }

ADS_FFT_INLINE void interleave( const FFTCx< FFTVec< double , 2 > > & x ,
                                FFTVec< double , 2 > & u ,
                                FFTVec< double , 2 > & v )
{
 u.v = _mm_unpacklo_pd( x.r.v , x.i.v );
 v.v = _mm_unpackhi_pd( x.r.v , x.i.v );
 }

ADS_FFT_INLINE FFTCx< FFTVec< float , 4 > > deinterleave(
                    FFTVec< float , 4 > u , FFTVec< float , 4 > v )
{
 FFTCx< FFTVec< float , 4 > > x;
 x.r.v = _mm_shuffle_ps( u.v , v.v , _MM_SHUFFLE( 2 , 0 , 2 , 0 ) );
 x.i.v = _mm_shuffle_ps( u.v , v.v , _MM_SHUFFLE( 3 , 1 , 3 , 1 ) );
 return( x );
 }

ADS_FFT_INLINE void interleave( const FFTCx< FFTVec< float , 4 > > & x ,
                                FFTVec< float , 4 > & u ,
                                FFTVec< float , 4 > & v )
{
 u.v = _mm_unpacklo_ps( x.r.v , x.i.v );
 v.v = _mm_unpackhi_ps( x.r.v , x.i.v );
 }

/// the matrix of the lanes transposed
ADS_FFT_INLINE void transposeVec( FFTVec< double , 2 > * v )
{
 const __m128d a = _mm_unpacklo_pd( v[ 0 ].v , v[ 1 ].v );
 v[ 1 ].v = _mm_unpackhi_pd( v[ 0 ].v , v[ 1 ].v );
 v[ 0 ].v = a;
 }

ADS_FFT_INLINE void transposeVec( FFTVec< float , 4 > * v )
{
 _MM_TRANSPOSE4_PS( v[ 0 ].v , v[ 1 ].v , v[ 2 ].v , v[ 3 ].v );
 }

 #ifdef __AVX__
ADS_FFT_INLINE FFTCx< FFTVec< double , 4 > > deinterleave(
                    FFTVec< double , 4 > u , FFTVec< double , 4 > v )
{
 FFTCx< FFTVec< double , 4 > > x;
 x.r.v = _mm256_unpacklo_pd( u.v , v.v );
 x.i.v = _mm256_unpackhi_pd( u.v , v.v );
 return( x );
 }

ADS_FFT_INLINE void interleave( const FFTCx< FFTVec< double , 4 > > & x ,
                                FFTVec< double , 4 > & u ,
                                FFTVec< double , 4 > & v )
{
 u.v = _mm256_unpacklo_pd( x.r.v , x.i.v );
 v.v = _mm256_unpackhi_pd( x.r.v , x.i.v );
 }

ADS_FFT_INLINE FFTCx< FFTVec< float , 8 > > deinterleave(
                    FFTVec< float , 8 > u , FFTVec< float , 8 > v )
{
 FFTCx< FFTVec< float , 8 > > x;
 x.r.v = _mm256_shuffle_ps( u.v , v.v , _MM_SHUFFLE( 2 , 0 , 2 , 0 ) );
 x.i.v = _mm256_shuffle_ps( u.v , v.v ,
                                _MM_SHUFFLE( 3 , 1 , 3 , 1 ) );
 return( x );
 }

ADS_FFT_INLINE void interleave( const FFTCx< FFTVec< float , 8 > > & x ,
                                FFTVec< float , 8 > & u ,
                                FFTVec< float , 8 > & v )
{
 u.v = _mm256_unpacklo_ps( x.r.v , x.i.v );
 v.v = _mm256_unpackhi_ps( x.r.v , x.i.v );
 }

ADS_FFT_INLINE void transposeVec( FFTVec< double , 4 > * v )
{
 const __m256d t0 = _mm256_unpacklo_pd( v[ 0 ].v , v[ 1 ].v ) ,
               t1 = _mm256_unpackhi_pd( v[ 0 ].v , v[ 1 ].v ) ,
               t2 = _mm256_unpacklo_pd( v[ 2 ].v , v[ 3 ].v ) ,
               t3 = _mm256_unpackhi_pd( v[ 2 ].v , v[ 3 ].v );
 v[ 0 ].v = _mm256_permute2f128_pd( t0 , t2 , 0x20 );
 v[ 1 ].v = _mm256_permute2f128_pd( t1 , t3 , 0x20 );
 v[ 2 ].v = _mm256_permute2f128_pd( t0 , t2 , 0x31 );
 v[ 3 ].v = _mm256_permute2f128_pd( t1 , t3 , 0x31 );
 }

ADS_FFT_INLINE void transposeVec( FFTVec< float , 8 > * v )
{
 __m256 t[ 8 ] , s[ 8 ];
 for( int a = 0 ; a < 8 ; a += 2 ) {
  t[ a ] = _mm256_unpacklo_ps( v[ a ].v , v[ a + 1 ].v );
  t[ a + 1 ] = _mm256_unpackhi_ps( v[ a ].v , v[ a + 1 ].v );
  }
 for( int a = 0 ; a < 8 ; a += 4 ) {
  s[ a ] = _mm256_shuffle_ps( t[ a ] , t[ a + 2 ] ,
                              _MM_SHUFFLE( 1 , 0 , 1 , 0 ) );
  s[ a + 1 ] = _mm256_shuffle_ps( t[ a ] , t[ a + 2 ] ,
                                  _MM_SHUFFLE( 3 , 2 , 3 , 2 ) );
  s[ a + 2 ] = _mm256_shuffle_ps( t[ a + 1 ] , t[ a + 3 ] ,
                                  _MM_SHUFFLE( 1 , 0 , 1 , 0 ) );
  s[ a + 3 ] = _mm256_shuffle_ps( t[ a + 1 ] , t[ a + 3 ] ,
                                  _MM_SHUFFLE( 3 , 2 , 3 , 2 ) );
  }
 for( int a = 0 ; a < 4 ; ++a ) {
  v[ a ].v = _mm256_permute2f128_ps( s[ a ] , s[ a + 4 ] , 0x20 );
  v[ a + 4 ].v = _mm256_permute2f128_ps( s[ a ] , s[ a + 4 ] , 0x31 );
  }
 }
 #endif
#endif

/// the W complex numbers at p, real and imaginary parts alternated, as
/// one item of W lanes, the number fftLane( l ) in the lane l
template< class T , std::size_t W >
ADS_FFT_INLINE FFTCx< FFTLane< T , W > > loadComplex( const T * p )
{
 using V = FFTLane< T , W >;
 if constexpr( W == 1 ) return( FFTCx< V >{ p[ 0 ] , p[ 1 ] } );
#if ADS_FFT_INTRINSICS
 else if constexpr( fftIsVec< T , W > )
  return( deinterleave( V::load( p ) , V::load( p + W ) ) );
#endif
#if ADS_FFT_SHUFFLE
 else if constexpr( fftIsExt< T , W > )
  return( deinterleave< T , W >( loadLanes< T , W >( p ) ,
                                 loadLanes< T , W >( p + W ) ,
                                 std::make_index_sequence< W >() ) );
#endif
 else {
  FFTCx< V > x;
  for( std::size_t l = 0 ; l < W ; ++l ) {
   x.r[ l ] = p[ 2 * fftLane< T , W >( l ) ];
   x.i[ l ] = p[ 2 * fftLane< T , W >( l ) + 1 ];
   }
  return( x );
  }
 }

/// the item x of W lanes stored as W complex numbers at p, the lane l as
/// the number fftLane( l )
template< class T , std::size_t W >
ADS_FFT_INLINE void storeComplex( T * p , const FFTCx< FFTLane< T , W > > & x )
{
 using V [[maybe_unused]] = FFTLane< T , W >;
 if constexpr( W == 1 ) {
  p[ 0 ] = x.r;
  p[ 1 ] = x.i;
  }
#if ADS_FFT_INTRINSICS
 else if constexpr( fftIsVec< T , W > ) {
  V u , v;
  interleave( x , u , v );
  u.store( p );
  v.store( p + W );
  }
#endif
#if ADS_FFT_SHUFFLE
 else if constexpr( fftIsExt< T , W > ) {
  V u , v;
  interleave< T , W >( x , u , v , std::make_index_sequence< W >() );
  storeLanes< T , W >( p , u );
  storeLanes< T , W >( p + W , v );
  }
#endif
 else
  for( std::size_t l = 0 ; l < W ; ++l ) {
   p[ 2 * fftLane< T , W >( l ) ] = x.r[ l ];
   p[ 2 * fftLane< T , W >( l ) + 1 ] = x.i[ l ];
   }
 }

/// the first c < W complex numbers at p as loadComplex() reads W, the
/// other lanes 0; and the first c lanes of x stored as storeComplex()
template< class T , std::size_t W >
ADS_FFT_INLINE FFTCx< FFTLane< T , W > > loadSome( const T * p ,
                                                   std::size_t c )
{
 T z[ 2 * W ] = {};
 std::copy( p , p + 2 * c , z );
 return( loadComplex< T , W >( z ) );
 }

template< class T , std::size_t W >
ADS_FFT_INLINE void storeSome( T * p , const FFTCx< FFTLane< T , W > > & x ,
                               std::size_t c )
{
 T z[ 2 * W ];
 storeComplex< T , W >( z , x );
 std::copy( z , z + 2 * c , p );
 }

/// the W x W matrix of the lanes of v[ 0 ] , ... , v[ W - 1 ] transposed:
/// the blocks off the diagonal swapped, then those within each block
template< class T , std::size_t W , class V >
ADS_FFT_INLINE void transpose( V * v )
{
 if constexpr( W > 1 ) {
#if ADS_FFT_INTRINSICS
  if constexpr( fftIsVec< T , W > ) {
   transposeVec( v );
   return;
   }
#endif
#if ADS_FFT_SHUFFLE
  if constexpr( fftIsExt< T , W > ) {
   transposeStage< W , W / 2 >( v );
   return;
   }
#endif
  for( std::size_t a = 0 ; a < W ; ++a )
   for( std::size_t b = a + 1 ; b < W ; ++b ) {
    const T t = v[ a ][ b ];
    v[ a ][ b ] = v[ b ][ a ];
    v[ b ][ a ] = t;
    }
  }
 }

/// the lanes of v in the reverse order: since fftLane( W - 1 - l ) = W - 1
/// - fftLane( l ), the W numbers loaded from p in the reverse order
template< class T , std::size_t W >
ADS_FFT_INLINE FFTLane< T , W > reverse( const FFTLane< T , W > & v )
{
 if constexpr( W == 1 ) return( v );
#if ADS_FFT_INTRINSICS
 else if constexpr( fftIsVec< T , W > ) {
  if constexpr( sizeof( T ) == 8 && W == 2 )
   return( FFTLane< T , W >{ _mm_shuffle_pd( v.v , v.v , 1 ) } );
  else if constexpr( sizeof( T ) == 4 && W == 4 )
   return( FFTLane< T , W >{
            _mm_shuffle_ps( v.v , v.v , _MM_SHUFFLE( 0 , 1 , 2 , 3 ) ) } );
 #ifdef __AVX__
  else if constexpr( sizeof( T ) == 8 )
   return( FFTLane< T , W >{
            _mm256_permute_pd( _mm256_permute2f128_pd( v.v , v.v , 1 ) ,
                               5 ) } );
  else
   return( FFTLane< T , W >{
            _mm256_permute_ps( _mm256_permute2f128_ps( v.v , v.v , 1 ) ,
                               _MM_SHUFFLE( 0 , 1 , 2 , 3 ) ) } );
 #endif
  }
#endif
#if ADS_FFT_SHUFFLE
 else if constexpr( fftIsExt< T , W > )
  return( [ & ]< std::size_t ... L >( std::index_sequence< L ... > ) {
   return( __builtin_shufflevector( v , v , int( W - 1 - L ) ... ) );
   }( std::make_index_sequence< W >() ) );
#endif
 else {
  FFTLane< T , W > y;
  for( std::size_t l = 0 ; l < W ; ++l ) y[ l ] = v[ W - 1 - l ];
  return( y );
  }
 }

/// f( integral_constant< I > ) for I = 0 , ... , N - 1, unrolled
template< std::size_t N , class F >
ADS_FFT_INLINE void unroll( F && f )
{
 [ & ]< std::size_t ... I >( std::index_sequence< I ... > ) ADS_FFT_LAMBDA {
  ( f( std::integral_constant< std::size_t , I >() ) , ... );
  }( std::make_index_sequence< N >() );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CODELETS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cosine c and the sine s of 2 pi k / L, 0 <= k < L: the angle is
/// taken to [ 0 , pi / 4 ] by the symmetries of the circle, in integers on
/// 8 L, so that each value has the error of one sine or cosine of a small
/// angle
inline void unitRoot( std::size_t k , std::size_t L , double & c ,
                      double & s )
{
 const std::uint64_t D = 8 * std::uint64_t( L );
 std::uint64_t K = 8 * std::uint64_t( k % L );
 bool negS = false , negC = false , swap = false;
 if( 2 * K > D ) { K = D - K; negS = true; }        // 2 pi - x
 if( 4 * K > D ) { K = D / 2 - K; negC = true; }    // pi - x
 if( 8 * K > D ) { K = D / 4 - K; swap = true; }    // pi / 2 - x
 const double x = 2 * std::numbers::pi * double( K ) / double( D );
 c = std::cos( x );
 s = std::sin( x );
 if( swap ) std::swap( c , s );
 if( negC ) c = -c;
 if( negS ) s = -s;
 }

/// omega_L^k = e^{-2 pi i k / L} as a complex number of type T
template< class T >
inline std::complex< T > omega( std::size_t k , std::size_t L )
{
 double c , s;
 unitRoot( k , L , c , s );
 return( std::complex< T >( T( c ) , T( - s ) ) );
 }

/// the largest codelet of a power of 2 [see codeletRoots()]
inline constexpr std::size_t fftCodelet = 64;


/// the odd primes with a codelet of their own, and the table of the
/// cosines and the sines of 2 pi q / p , q < p, from 2 fftOddAt( p ) on
inline constexpr std::size_t fftOddPrimes[] = { 3 , 5 , 7 , 11 , 13 };

constexpr std::size_t fftOddAt( std::size_t p )
{
 std::size_t a = 0;
 for( std::size_t q : fftOddPrimes ) {
  if( q == p ) break;
  a += q;
  }
 return( a );
 }

/// the sizes not powers of 2 nor primes whose codelets have a table of
/// powers of omega, and the start of the table of each in that of
/// codeletRoots()
inline constexpr std::size_t fftTableSizes[] = { 6 , 9 , 10 , 12 , 15 , 18 ,
                                                 20 , 24 , 25 , 27 , 30 , 36 ,
                                                 40 , 45 , 48 , 50 , 54 ,
                                                 60 };

inline constexpr std::size_t fftOddBase = 2 * fftCodelet;

constexpr std::size_t fftMixedAt( std::size_t N )
{
 std::size_t a = fftOddBase;
 for( std::size_t p : fftOddPrimes ) a += 2 * p;
 for( std::size_t q : fftTableSizes ) {
  if( q == N ) return( a );
  a += 2 * q;
  }
 return( std::size_t( -1 ) );
 }

/// the table of all the codelets, made once: omega_64^e = c + i s at 2 e ,
/// 2 e + 1, e < 64; then, from fftOddBase + 2 fftOddAt( p ) on, the
/// cosines and the sines of 2 pi q / p , q < p, of each odd prime of
/// fftOddPrimes; then, from fftMixedAt( N ) on, omega_N^e , e < N, of each
/// size of fftTableSizes. One table whose start the codelets receive, so
/// that none reads a static of its own in its loops
template< class T >
inline const T * codeletRoots()
{
 static const std::vector< T > R = [] {
  std::vector< T > t;
  auto add = [ & ]( std::size_t N ) {
   for( std::size_t e = 0 ; e < N ; ++e ) {
    const std::complex< T > w = omega< T >( e , N );
    t.push_back( w.real() );
    t.push_back( w.imag() );
    }
   };
  add( fftCodelet );
  for( std::size_t p : fftOddPrimes )
   for( std::size_t q = 0 ; q < p ; ++q ) {
    double c , si;
    unitRoot( q , p , c , si );
    t.push_back( T( c ) );
    t.push_back( T( si ) );
    }
  for( std::size_t N : fftTableSizes ) add( N );
  return( t );
  }();
 return( R.data() );
 }

/// x times omega_N^e, e < N fixed: by nothing, -i and -1 without products
template< std::size_t N , std::size_t e , bool inv , class T , std::size_t W ,
          class V >
ADS_FFT_INLINE FFTCx< V > twiddle( const FFTCx< V > & x , const T * R )
{
 if constexpr( e == 0 ) return( x );
 else if constexpr( 4 * e == N ) return( mulMinusI< inv >( x ) );
 else if constexpr( 2 * e == N ) return( FFTCx< V >{ - x.r , - x.i } );
 else {
  constexpr std::size_t k = e * ( fftCodelet / N );
  return( cmul< inv >( x , splat< T , W >( R[ 2 * k ] ) ,
                       splat< T , W >( R[ 2 * k + 1 ] ) ) );
  }
 }

/// the transform of 4 points in place: x_u = sum_t x_t (-i)^{t u}
template< bool inv , class V >
ADS_FFT_INLINE void dft4( FFTCx< V > & x0 , FFTCx< V > & x1 ,
                          FFTCx< V > & x2 , FFTCx< V > & x3 )
{
 const FFTCx< V > t0 = x0 + x2 , t1 = x0 - x2 , t2 = x1 + x3 ,
                  t3 = mulMinusI< inv >( x1 - x3 );
 x0 = t0 + t2;
 x2 = t0 - t2;
 x1 = t1 + t3;
 x3 = t1 - t3;
 }

/// the inputs in( S j + T ) of a codelet within a larger one
template< std::size_t S , std::size_t T , class In >
struct FFTStride {
 In & in;

 ADS_FFT_INLINE auto operator()( std::size_t j ) const {
  return( in( S * j + T ) );
  }
 };

/// the inputs in( S j + t ) of a codelet within a larger one, and its
/// outputs out( S j + t , y ), S and t known at run time
template< class In >
struct FFTSkip {
 In & in;
 std::size_t S , t;

 ADS_FFT_INLINE auto operator()( std::size_t j ) const {
  return( in( S * j + t ) );
  }
 };

template< class Out >
struct FFTSkipOut {
 Out & out;
 std::size_t S , t;

 template< class C >
 ADS_FFT_INLINE void operator()( std::size_t j , const C & y ) const {
  out( S * j + t , y );
  }
 };

/// the items of an array as the inputs and the outputs of a codelet
template< class C >
struct FFTArray {
 C * x;

 ADS_FFT_INLINE C operator()( std::size_t j ) const { return( x[ j ] ); }

 ADS_FFT_INLINE void operator()( std::size_t j , const C & y ) const {
  x[ j ] = y;
  }
 };

template< std::size_t N , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void codelet( In & in , Out & out , const T * R );

template< std::size_t p , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void oddCodelet( In & in , Out & out , const T * cs );

template< std::size_t N , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void mixedCodelet( In & in , Out & out , const T * R );

/// whether p is a prime of fftOddPrimes
constexpr bool fftIsOdd( std::size_t p )
{
 for( std::size_t q : fftOddPrimes )
  if( q == p ) return( true );
 return( false );
 }

/// the radix-4 step of a codelet of N items: the outputs k of the
/// transforms s of its 4 quarters, times omega_N^{t k}, give the outputs
/// k + u N / 4
template< std::size_t N , std::size_t k , bool inv , class T , std::size_t W ,
          class C , class Out >
ADS_FFT_INLINE void combine4( Out & out , const C * s , const T * R )
{
 constexpr std::size_t M = N / 4;
 if constexpr( k < M ) {
  C a = s[ k ] , b = twiddle< N , k , inv , T , W >( s[ M + k ] , R ) ,
    c = twiddle< N , 2 * k , inv , T , W >( s[ 2 * M + k ] , R ) ,
    d = twiddle< N , 3 * k , inv , T , W >( s[ 3 * M + k ] , R );
  dft4< inv >( a , b , c , d );
  out( k , a );
  out( M + k , b );
  out( 2 * M + k , c );
  out( 3 * M + k , d );
  }
 }

template< std::size_t N , bool inv , class T , std::size_t W , class In ,
          class Out , std::size_t ... J >
ADS_FFT_INLINE void codelet4( In & in , Out & out , const T * R ,
                              std::index_sequence< J ... > )
{
 using C = FFTCx< FFTLane< T , W > >;
 constexpr std::size_t M = N / 4;
 C s[ N ];                                 // the quarter t from t M on
 FFTStride< 4 , 0 , In > i0{ in };
 FFTStride< 4 , 1 , In > i1{ in };
 FFTStride< 4 , 2 , In > i2{ in };
 FFTStride< 4 , 3 , In > i3{ in };
 FFTArray< C > o0{ s } , o1{ s + M } , o2{ s + 2 * M } , o3{ s + 3 * M };
 codelet< M , inv , T , W >( i0 , o0 , R );
 codelet< M , inv , T , W >( i1 , o1 , R );
 codelet< M , inv , T , W >( i2 , o2 , R );
 codelet< M , inv , T , W >( i3 , o3 , R );
 ( combine4< N , J , inv , T , W >( out , s , R ) , ... );
 }

/// the transform of N items, N <= fftCodelet a power of 2, unrolled: the
/// input i is in( i ), the output k goes to out( k , y ), which may write
/// where the inputs were, all of them read by then; by decimation in time
/// in radix 4, and 2 at the bottom
template< std::size_t N , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void codelet( In & in , Out & out , const T * R )
{
 using C = FFTCx< FFTLane< T , W > >;
 if constexpr( N == 1 ) out( 0 , in( 0 ) );
 else if constexpr( N == 2 ) {
  const C u = in( 0 ) , v = in( 1 );
  out( 0 , u + v );
  out( 1 , u - v );
  }
 else if constexpr( N == 4 ) {
  C x0 = in( 0 ) , x1 = in( 1 ) , x2 = in( 2 ) , x3 = in( 3 );
  dft4< inv >( x0 , x1 , x2 , x3 );
  out( 0 , x0 );
  out( 1 , x1 );
  out( 2 , x2 );
  out( 3 , x3 );
  }
 else if constexpr( ( N & ( N - 1 ) ) == 0 )
  codelet4< N , inv , T , W >( in , out , R ,
                               std::make_index_sequence< N / 4 >() );
 else if constexpr( fftIsOdd( N ) )
  oddCodelet< N , inv , T , W >( in , out ,
                                 R + fftOddBase + 2 * fftOddAt( N ) );
 else mixedCodelet< N , inv , T , W >( in , out , R );
 }

/// the transform of p items, p an odd prime of fftOddPrimes, unrolled: for
/// 0 < k <= h = ( p - 1 ) / 2, with a_j = x_j + x_{p-j} and b_j = x_j -
/// x_{p-j}, the outputs k and p - k are R_k -+ i I_k, R_k = x_0 + sum_j
/// a_j cos( 2 pi j k / p ) and I_k = sum_j b_j sin( 2 pi j k / p ) (the
/// signs swapped if inv); cs holds the cosines and the sines of p
template< std::size_t p , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void oddCodelet( In & in , Out & out , const T * cs )
{
 using C = FFTCx< FFTLane< T , W > >;
 constexpr std::size_t h = ( p - 1 ) / 2;
 const C x0 = in( 0 );
 C a[ h ] , b[ h ] , R[ h ] , I[ h ] , y0 = x0;
 ADS_FFT_UNROLL
 for( std::size_t j = 0 ; j < h ; ++j ) {
  const C u = in( j + 1 ) , v = in( p - 1 - j );
  a[ j ] = u + v;
  b[ j ] = u - v;
  y0 = y0 + a[ j ];
  }
 ADS_FFT_UNROLL
 for( std::size_t k = 1 ; k <= h ; ++k ) {
  C re = x0 , im;
  std::size_t q = 0;
  ADS_FFT_UNROLL
  for( std::size_t j = 0 ; j < h ; ++j ) {
   q += k;                                 // ( j + 1 ) k mod p
   q -= q >= p ? p : 0;
   const auto c = splat< T , W >( cs[ 2 * q ] ) ,
              s = splat< T , W >( cs[ 2 * q + 1 ] );
   re.r = re.r + a[ j ].r * c;
   re.i = re.i + a[ j ].i * c;
   if( j == 0 ) {
    im.r = b[ j ].r * s;
    im.i = b[ j ].i * s;
    }
   else {
    im.r = im.r + b[ j ].r * s;
    im.i = im.i + b[ j ].i * s;
    }
   }
  R[ k - 1 ] = re;
  I[ k - 1 ] = mulMinusI< inv >( im );
  }
 out( 0 , y0 );
 ADS_FFT_UNROLL
 for( std::size_t k = 0 ; k < h ; ++k ) {
  out( k + 1 , R[ k ] + I[ k ] );
  out( p - 1 - k , R[ k ] - I[ k ] );
  }
 }

/// the largest power of the smallest prime of N, and whether it is not N:
/// the first factor of the prime-factor algorithm
constexpr std::size_t fftPrimePower( std::size_t N )
{
 std::size_t p = 2;
 while( N % p ) ++p;
 std::size_t q = 1;
 while( N % ( q * p ) == 0 ) q *= p;
 return( q );
 }

/// the maps of the prime-factor algorithm on N = A B, A and B coprime: the
/// input b A + a B mod N of the transform of length A of the column b at
/// in[ b A + a ], the output of its row a , b at out[ a B + b ], k = ( a B (
/// B^{-1} mod A ) + b A ( A^{-1} mod B ) ) mod N
template< std::size_t A , std::size_t B >
struct FFTGoodThomas {
 std::size_t in[ A * B ] , out[ A * B ];

 constexpr FFTGoodThomas() : in{} , out{} {
  constexpr std::size_t N = A * B;
  std::size_t ia = 1 , ib = 1;               // B^{-1} mod A , A^{-1} mod B
  while( ( ia * B ) % A != 1 % A ) ++ia;
  while( ( ib * A ) % B != 1 % B ) ++ib;
  for( std::size_t b = 0 ; b < B ; ++b )
   for( std::size_t a = 0 ; a < A ; ++a ) {
    in[ b * A + a ] = ( b * A + a * B ) % N;
    out[ a * B + b ] = ( a * B * ia + b * A * ib ) % N;
    }
  }
 };

/// the inputs in( m[ j ] ) of a codelet within a larger one, and its
/// outputs out( m[ j ] , y )
template< class In >
struct FFTMapIn {
 In & in;
 const std::size_t * m;

 ADS_FFT_INLINE auto operator()( std::size_t j ) const {
  return( in( m[ j ] ) );
  }
 };

template< class Out >
struct FFTMapOut {
 Out & out;
 const std::size_t * m;

 template< class C >
 ADS_FFT_INLINE void operator()( std::size_t j , const C & y ) const {
  out( m[ j ] , y );
  }
 };

/// the smallest prime of N, 4 before 2
constexpr std::size_t fftFirstRadix( std::size_t N )
{
 if( N % 4 == 0 ) return( 4 );
 for( std::size_t p = 2 ; p < N ; ++p )
  if( N % p == 0 ) return( p );
 return( N );
 }

/// the transform of N items, N not a power of 2 nor a prime, unrolled: if
/// N = A B with A = fftPrimePower( N ) < N, by the prime-factor algorithm
/// of Good and Thomas, the transforms of length A of the B columns and of
/// length B of the A rows of the matrix of the maps of FFTGoodThomas, with
/// no powers of omega between them; otherwise by decimation in time in
/// the radix p = fftFirstRadix( N ), the transforms of the M = N / p items
/// t , t + p , ... by codelets, their outputs k times omega_N^{t k} and
/// the transform of p items of those, the outputs k + u M
template< std::size_t N , bool inv , class T , std::size_t W , class In ,
          class Out >
ADS_FFT_INLINE void mixedCodelet( In & in , Out & out , const T * R )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 constexpr std::size_t A = fftPrimePower( N ) , B = N / A;
 if constexpr( A < N ) {                   // coprime: no powers of omega
  static constexpr FFTGoodThomas< A , B > G{};
  C s[ N ];                                // the column b from b A on
  ADS_FFT_UNROLL
  for( std::size_t b = 0 ; b < B ; ++b ) {
   FFTMapIn< In > i{ in , G.in + b * A };
   FFTArray< C > o{ s + b * A };
   codelet< A , inv , T , W >( i , o , R );
   }
  FFTArray< C > sa{ s };
  ADS_FFT_UNROLL
  for( std::size_t a = 0 ; a < A ; ++a ) {  // the rows, to their outputs
   FFTSkip< FFTArray< C > > i{ sa , A , a };
   FFTMapOut< Out > o{ out , G.out + a * B };
   codelet< B , inv , T , W >( i , o , R );
   }
  return;
  }
 constexpr std::size_t p = fftFirstRadix( N ) , M = N / p;
 C s[ N ];                                 // the part t from t M on
 ADS_FFT_UNROLL
 for( std::size_t t = 0 ; t < p ; ++t ) {
  FFTSkip< In > i{ in , p , t };
  FFTArray< C > o{ s + t * M };
  codelet< M , inv , T , W >( i , o , R );
  }
 static_assert( fftMixedAt( N ) != std::size_t( -1 ) );
 const T * RN = R + fftMixedAt( N );
 ADS_FFT_UNROLL
 for( std::size_t k = 0 ; k < M ; ++k ) {
  C y[ p ];
  y[ 0 ] = s[ k ];
  ADS_FFT_UNROLL
  for( std::size_t t = 1 ; t < p ; ++t ) {  // omega_N^{t k}, from a table
   const std::size_t e = t * k;
   y[ t ] = cmul< inv >( s[ t * M + k ] , splat< T , W >( RN[ 2 * e ] ) ,
                         splat< T , W >( RN[ 2 * e + 1 ] ) );
   }
  if constexpr( p == 4 ) {
   dft4< inv >( y[ 0 ] , y[ 1 ] , y[ 2 ] , y[ 3 ] );
   out( k , y[ 0 ] );
   out( k + M , y[ 1 ] );
   out( k + 2 * M , y[ 2 ] );
   out( k + 3 * M , y[ 3 ] );
   }
  else if constexpr( p == 2 ) {
   out( k , y[ 0 ] + y[ 1 ] );
   out( k + M , y[ 0 ] - y[ 1 ] );
   }
  else {
   FFTArray< C > yi{ y };
   FFTSkipOut< Out > o{ out , M , k };
   oddCodelet< p , inv , T , W >( yi , o ,
                                  R + fftOddBase + 2 * fftOddAt( p ) );
   }
  }
 }

/// the sizes of the codelets not a power of 2: those of the primes 2, 3
/// and 5 up to 25, as many as the time of the compiler allows (3 and 5
/// alone if ADS_FFT_LEAN)
#if ADS_FFT_LEAN
inline constexpr std::size_t fftMixed[] = { 3 , 5 };
#else
inline constexpr std::size_t fftMixed[] = { 3 , 5 , 6 , 9 , 10 , 12 , 15 ,
                                            20 , 25 };
#endif

/// f.template operator()< L >() if L <= Max <= fftCodelet is a power of 2,
/// and true; else false
template< std::size_t Max = fftCodelet , class F >
ADS_FFT_INLINE bool byCodelet( std::size_t L , F && f )
{
 bool done = false;
 [ & ]< std::size_t ... E >( std::index_sequence< E ... > ) {
  ( [ & ] {
    if constexpr( ( std::size_t( 1 ) << E ) <= Max )
     if( L == ( std::size_t( 1 ) << E ) ) {
      f.template operator()< ( std::size_t( 1 ) << E ) >();
      done = true;
      }
    }() , ... );
  }( std::make_index_sequence< std::countr_zero( fftCodelet ) + 1 >() );
 return( done );
 }

/// f.template operator()< L >() if L <= Max <= fftCodelet has a codelet,
/// a power of 2 or a size of fftMixed, and true; else false
template< std::size_t Max = fftCodelet , class F >
ADS_FFT_INLINE bool byAnyCodelet( std::size_t L , F && f )
{
 if( byCodelet< Max >( L , f ) ) return( true );
 bool done = false;
 unroll< std::size( fftMixed ) >( [ & ]( auto I ) {
  constexpr std::size_t N = fftMixed[ I ];
  if constexpr( N <= Max )
   if( L == N ) {
    f.template operator()< N >();
    done = true;
    }
  } );
 return( done );
 }

/// whether N is a size of fftMixed
constexpr bool fftIsMixed( std::size_t N )
{
 for( std::size_t q : fftMixed )
  if( q == N ) return( true );
 return( false );
 }

/// f.template operator()< R >() if R is a radix of the rounds, with a
/// codelet up to Max or a prime of fftOddPrimes, and true; else false
template< std::size_t Max , class F >
ADS_FFT_INLINE bool byRadix( std::size_t R , F && f )
{
 if( byAnyCodelet< Max >( R , f ) ) return( true );
 bool done = false;
 unroll< std::size( fftOddPrimes ) >( [ & ]( auto I ) {
  constexpr std::size_t p = fftOddPrimes[ I ];
  if constexpr( ! fftIsMixed( p ) )
   if( R == p ) {
    f.template operator()< p >();
    done = true;
    }
  } );
 return( done );
 }


/*--------------------------------------------------------------------------*/
/*--------------------------- THE BATCHED DFT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the rounds of the transforms of L items by Stockham: the round t in
/// radix R_t on the length L_t, R_t of fftRadices(); its powers of
/// omega_{L_t} from at[ t ] in w, those of p < L_t / R_t , 0 < j < R_t at
/// 2 ( ( R_t - 1 ) p + j - 1 ); the cosines and the sines of 2 pi q / R_t
/// , 0 < q <= ( R_t - 1 ) / 2, of a prime of no codelet from g[ t ] in w
template< class T >
struct FFTSteps {
 std::size_t L = 0;
 std::vector< std::size_t > R , at , g;
 std::vector< T > w;
 };

/// the largest radix of the rounds of batched(): the codelets of 64 items
/// spill more of them out of the registers than they save in the rounds
inline constexpr std::size_t fftRadix = 32;

/// the same on single lanes, and the largest codelet of the steps there,
/// whose items are as many numbers as the registers hold
#ifndef ADS_FFT_SCALAR_RADIX
 #define ADS_FFT_SCALAR_RADIX 8
#endif
#ifndef ADS_FFT_SCALAR_CODELET
 #define ADS_FFT_SCALAR_CODELET 16
#endif
inline constexpr std::size_t fftScalarRadix = ADS_FFT_SCALAR_RADIX ,
                             fftScalarCodelet = ADS_FFT_SCALAR_CODELET;

/// the primes of n, the smallest first, with their repetitions
inline std::vector< std::size_t > fftFactors( std::size_t n )
{
 std::vector< std::size_t > f;
 for( std::size_t p = 2 ; p * p <= n ; p += ( p == 2 ? 1 : 2 ) )
  while( n % p == 0 ) {
   f.push_back( p );
   n /= p;
   }
 if( n > 1 ) f.push_back( n );
 return( f );
 }

/// the radices of the rounds of L items: as few rounds as can be, of the
/// radices with a codelet up to fftRadix, the primes of fftOddPrimes and
/// any larger prime, the largest smallest radix among those (the radices
/// as equal as can be), the larger first; by recursion on the divisors of
/// L, each made once
inline std::vector< std::size_t > fftRadices( std::size_t L ,
                                            std::size_t Rmax = fftRadix )
{
 std::vector< std::size_t > C;             // the candidates
 for( std::size_t R = 2 ; R <= Rmax ; ++R )
  if( L % R == 0 && ( byRadix< fftRadix >( R , []< std::size_t >() {} ) ) )
   C.push_back( R );
 for( std::size_t p : fftFactors( L ) )
  if( std::find( C.begin() , C.end() , p ) == C.end() )
   C.push_back( p );
 std::unordered_map< std::size_t , std::pair< std::size_t , std::size_t > >
  best;                                     // rounds , smallest radix
 auto solve = [ & ]( auto & self , std::size_t x ) ->
               std::pair< std::size_t , std::size_t > {
  if( x == 1 )
   return( std::pair< std::size_t , std::size_t >( 0 , std::size_t( -1 ) ) );
  if( auto it = best.find( x ) ; it != best.end() ) return( it->second );
  std::pair< std::size_t , std::size_t > b{ std::size_t( -1 ) , 0 };
  for( std::size_t R : C )
   if( x % R == 0 ) {
    const auto [ r , m ] = self( self , x / R );
    if( r == std::size_t( -1 ) ) continue;
    const std::pair< std::size_t , std::size_t > c{ r + 1 ,
                                                    std::min( m , R ) };
    if( c.first < b.first || ( c.first == b.first && c.second > b.second ) )
     b = c;
    }
  best[ x ] = b;
  return( b );
  };
 solve( solve , L );
 std::vector< std::size_t > R;
 for( std::size_t x = L ; x > 1 ; ) {        // the choices back
  const auto b = best[ x ];
  for( std::size_t q : C )
   if( x % q == 0 ) {
    const auto c = x / q == 1 ? std::pair< std::size_t , std::size_t >(
                                 0 , std::size_t( -1 ) ) : best[ x / q ];
    if( c.first + 1 == b.first && std::min( c.second , q ) == b.second ) {
     R.push_back( q );
     x /= q;
     break;
     }
    }
  }
 std::sort( R.begin() , R.end() , std::greater<>() );
 return( R );
 }

/// the rounds of the transform of L items: the power of 2 of L in as few
/// rounds as the radix fftRadix allows, of radices as equal as can be, the
/// larger first, then a round for each odd prime
template< class T >
inline FFTSteps< T > fftSteps( std::size_t L , std::size_t Rmax = fftRadix )
{
 FFTSteps< T > s;
 s.L = L;
 s.R = fftRadices( L , Rmax );
 std::size_t Lt = L;
 for( std::size_t R : s.R ) {
  const std::size_t m = Lt / R;
  s.at.push_back( s.w.size() );
  for( std::size_t p = 0 ; p < m ; ++p )
   for( std::size_t j = 1 ; j < R ; ++j ) {
    const std::complex< T > z = omega< T >( p * j , Lt );
    s.w.push_back( z.real() );
    s.w.push_back( z.imag() );
    }
  Lt = m;
  }
 for( std::size_t R : s.R ) {
  s.g.push_back( s.w.size() );
  if( ! byRadix< fftRadix >( R , []< std::size_t >() {} ) )
   for( std::size_t q = 1 ; 2 * q < R ; ++q ) {
    double c , si;
    unitRoot( q , R , c , si );
    s.w.push_back( T( c ) );
    s.w.push_back( T( si ) );
    }
  }
 return( s );
 }

/// a round of Stockham in radix R by decimation in frequency on the
/// length m R: the transform of the R items x( q + s ( p + i m ) ) , i <
/// R, times omega^{p j}, goes to y( q + s ( R p + j ) , . )
template< std::size_t R , class T , std::size_t W , class X , class Y >
inline void stockham( X & x , Y & y , std::size_t m , std::size_t s ,
                      const T * w , const T * Rt )
{
 using C = FFTCx< FFTLane< T , W > >;
 for( std::size_t p = 0 ; p < m ; ++p , w += 2 * ( R - 1 ) )
  for( std::size_t q = 0 ; q < s ; ++q ) {
   const std::size_t a = q + s * p , o = q + s * R * p;
   auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
    return( x( a + s * m * i ) );
    };
   if( p == 0 ) {
    auto out = [ & ]( std::size_t j , const C & z ) ADS_FFT_LAMBDA {
     y( o + s * j , z );
     };
    codelet< R , false , T , W >( in , out , Rt );
    }
   else {
    auto out = [ & ]( std::size_t j , const C & z ) ADS_FFT_LAMBDA {
     y( o + s * j , j == 0 ? z
                     : cmul< false >( z , splat< T , W >( w[ 2 * j - 2 ] ) ,
                                      splat< T , W >( w[ 2 * j - 1 ] ) ) );
     };
    codelet< R , false , T , W >( in , out , Rt );
    }
   }
 }

/// a round of Stockham in a prime radix R with no codelet, as stockham(),
/// the transform of the R items by the sums of oddCodelet() in loops; cs
/// holds the cosines and the sines of R
template< class T , std::size_t W , class X , class Y >
inline void stockhamPrime( X & x , Y & y , std::size_t R , std::size_t m ,
                           std::size_t s , const T * w , const T * cs )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 const std::size_t h = ( R - 1 ) / 2;
 std::vector< C > a( h ) , b( h ) , Z( R );
 for( std::size_t p = 0 ; p < m ; ++p , w += 2 * ( R - 1 ) )
  for( std::size_t q = 0 ; q < s ; ++q ) {
   const std::size_t i0 = q + s * p , o = q + s * R * p , d = s * m;
   const C x0 = x( i0 );
   C y0 = x0;
   for( std::size_t j = 1 ; j <= h ; ++j ) {
    const C u = x( i0 + d * j ) , v = x( i0 + d * ( R - j ) );
    a[ j - 1 ] = u + v;
    b[ j - 1 ] = u - v;
    y0 = y0 + a[ j - 1 ];
    }
   Z[ 0 ] = y0;
   for( std::size_t k = 1 ; k <= h ; ++k ) {
    C re = x0 , im{ splat< T , W >( 0 ) , splat< T , W >( 0 ) };
    std::size_t e = 0;
    for( std::size_t j = 1 ; j <= h ; ++j ) {
     e += k;
     if( e >= R ) e -= R;
     const bool neg = e > h;
     const std::size_t f = ( neg ? R - e : e ) - 1;
     const V c = splat< T , W >( cs[ 2 * f ] ) ,
             sn = splat< T , W >( neg ? - cs[ 2 * f + 1 ] : cs[ 2 * f + 1 ] );
     re.r = re.r + a[ j - 1 ].r * c;
     re.i = re.i + a[ j - 1 ].i * c;
     im.r = im.r + b[ j - 1 ].r * sn;
     im.i = im.i + b[ j - 1 ].i * sn;
     }
    const C t = mulMinusI< false >( im );
    Z[ k ] = re + t;
    Z[ R - k ] = re - t;
    }
   for( std::size_t j = 0 ; j < R ; ++j )
    y( o + s * j , j == 0 || p == 0 ? Z[ j ]
                  : cmul< false >( Z[ j ] , splat< T , W >( w[ 2 * j - 2 ] ) ,
                                   splat< T , W >( w[ 2 * j - 1 ] ) ) );
   }
 }

/// the forward transform of the L items x into y by the rounds S, u and v
/// of L items for the rounds in between; x may be y
template< class T , std::size_t W >
inline void batched( const FFTSteps< T > & S ,
                     const FFTCx< FFTLane< T , W > > * x ,
                     FFTCx< FFTLane< T , W > > * y ,
                     FFTCx< FFTLane< T , W > > * u ,
                     FFTCx< FFTLane< T , W > > * v )
{
 using C = FFTCx< FFTLane< T , W > >;
 const T * Rt = codeletRoots< T >();
 const std::size_t P = S.R.size();
 if( P == 0 ) {                            // one item
  if( S.L ) y[ 0 ] = x[ 0 ];
  return;
  }
 std::size_t s = 1 , Lt = S.L;
 for( std::size_t t = 0 ; t < P ; ++t ) {
  const std::size_t R = S.R[ t ] , m = Lt / R;
  const T * w = S.w.data() + S.at[ t ];
  FFTArray< const C > a{ t == 0 ? x : u };
  FFTArray< C > c{ t + 1 < P ? v : y };
  if( ! byRadix< fftRadix >( R , [ & ]< std::size_t Q >() {
         stockham< Q , T , W >( a , c , m , s , w , Rt ); } ) )
   stockhamPrime< T , W >( a , c , R , m , s , w , S.w.data() + S.g[ t ] );
  s *= R;
  Lt = m;
  std::swap( u , v );
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE FOUR STEPS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the columns, or the rows, transformed together by the rounds of
/// batched(): read and written S W numbers at a time, a few lines of the
/// cache, rather than one line each at a stride of a power of 2
inline constexpr std::size_t fftStrips = 4;

/// from 2^fftSplitRoots points on, the powers of omega_n of the four steps
/// are the products of two tables of about n / W and r items, rather than
/// one table of n
inline constexpr std::size_t fftSplitRoots = std::size_t( 1 ) << 16;

/// the rows ahead of those read, or written, at a stride by batched()
/// whose lines are brought into the cache
inline constexpr std::size_t fftAhead = 16;

/// the shift of the blocks of the outputs of the four steps on x: the
/// numbers from x to the first one that starts an item of W numbers within
/// a line of the cache, or 0 if the numbers are not aligned to their size
/// times 2; the blocks start there, so that the store of an item writes as
/// few lines as it can
template< class T , std::size_t W >
inline std::size_t fftShift( const T * x )
{
 constexpr std::size_t Z = 2 * sizeof( T ) ,
                       A = std::min< std::size_t >( 64 , Z * W );
 const std::size_t a = std::size_t( reinterpret_cast< std::uintptr_t >( x ) );
 if( a % Z ) return( 0 );
 return( ( A - a % A ) % A / Z % W );
 }

/// the powers of omega_n of the four steps on n = r m points with the shift
/// o, mb = ceil( m / W ): omega_n^{j k( u , l )}, j < r, u < mb, l < W, k(
/// u , l ) = ( o + u W + fftLane( l ) ) mod m, as the item u r + j of t,
/// one lane per l; from fftSplitRoots on, as the product of omega_n^{j ( o
/// + u W )}, at 2 ( u r + j ) in s, and of the item j of t, omega_n^{j
/// fftLane( l )} in the lane l, except for the block u that wraps around
/// m, whose powers are the items of w
template< class T , std::size_t W >
struct FFTFourRoots {
 std::vector< FFTCx< FFTLane< T , W > > > t , w;
 std::vector< T > s;
 };

template< class T , std::size_t W >
inline void fourStepRoots( FFTFourRoots< T , W > & R , std::size_t r ,
                           std::size_t m , std::size_t o )
{
 using C = FFTCx< FFTLane< T , W > >;
 const std::size_t n = r * m , mb = ( m + W - 1 ) / W;
 const bool split = n >= fftSplitRoots;
 auto set = [ & ]( C & x , std::size_t l , std::size_t e ) {
  const std::complex< T > z = omega< T >( e % n , n );
  if constexpr( W == 1 ) { x.r = z.real(); x.i = z.imag(); }
  else { x.r[ l ] = z.real(); x.i[ l ] = z.imag(); }
  };
 auto k = [ & ]( std::size_t u , std::size_t l ) {
  return( ( o + u * W + fftLane< T , W >( l ) ) % m );
  };
 R.t.resize( split ? r : mb * r );
 for( std::size_t u = 0 ; u < ( split ? 1 : mb ) ; ++u )
  for( std::size_t j = 0 ; j < r ; ++j )
   for( std::size_t l = 0 ; l < W ; ++l )
    set( R.t[ u * r + j ] , l ,
         j * ( split ? fftLane< T , W >( l ) : k( u , l ) ) );
 if( split ) {
  R.s.resize( 2 * mb * r );
  for( std::size_t u = 0 ; u < mb ; ++u )
   for( std::size_t j = 0 ; j < r ; ++j ) {
    const std::complex< T > z = omega< T >( j * ( o + u * W ) % n , n );
    R.s[ 2 * ( u * r + j ) ] = z.real();
    R.s[ 2 * ( u * r + j ) + 1 ] = z.imag();
    }
  R.w.resize( r );
  for( std::size_t j = 0 ; j < r ; ++j )
   for( std::size_t l = 0 ; l < W ; ++l )
    set( R.w[ j ] , l , j * k( mb - 1 , l ) );
  }
 }

/// a buffer of at least n items, kept per thread
template< class C >
inline C * fftBuffer( std::size_t n )
{
 thread_local std::vector< C > B;
 if( B.size() < n ) B.resize( n );
 return( B.data() );
 }

/// the matrix of the four steps on n = r m points of x, of m rows of r
/// columns, with the shift o (0 if W does not divide m): its blocks of W
/// columns start at W t, t < rb = ceil( r / W ), those of W numbers of the
/// rows of the output at o + W u, u < mb = ceil( m / W ), the last one
/// wrapping around the row if o > 0, or short if W does not divide m; B
/// holds the transforms of the columns, the block t from t M on, M = m + W
/// so that the blocks do not start at a stride of a power of 2
template< class T , std::size_t W >
struct FFTFour {
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;

 const T * xi;
 T * x;
 std::size_t r , m , o , M , rb , mb;
 C * B;
 const FFTFourRoots< T , W > * Tw;
 bool split;
 V ci , fr , fi;    ///< the factors of the imaginary parts of the inputs,
                    ///< of the real and imaginary parts of the outputs

 /// whether the block u of the rows of the output wraps around, or is
 /// short
 bool edge( std::size_t u ) const {
  return( u + 1 == mb && ( o > 0 || m % W ) );
  }

 /// the item y as the W numbers from k on, mod m (only those below m if o
 /// is 0), of the row i of the output
 ADS_FFT_INLINE void output( std::size_t i , std::size_t k ,
                             const C & y ) const {
  T * a = x + 2 * m * i;
  if( k + W <= m ) {
   storeComplex< T , W >( a + 2 * k , y );
   return;
   }
  T z[ 2 * W ];
  storeComplex< T , W >( z , y );
  for( std::size_t q = 0 ; q < W ; ++q ) {
   std::size_t h = k + q;
   if( h >= m ) {
    if( o == 0 ) break;
    h -= m;
    }
   a[ 2 * h ] = z[ 2 * q ];
   a[ 2 * h + 1 ] = z[ 2 * q + 1 ];
   }
  }

 /// the block b of W items of the block u of the rows of B, k( u , l ) =
 /// ( o + u W + fftLane( l ) ) mod m: the W items k( u , l ) of the block
 /// b of B, transposed, times the powers of omega_n; the item of the
 /// column j < r at c[ j ]
 template< bool cut = true >
 ADS_FFT_INLINE void rowBlock( std::size_t u , std::size_t b ,
                               C * c ) const {
  V re[ W ] , im[ W ];
  const bool wrap = o > 0 && u + 1 == mb;
  const C * a = B + b * M + o + u * W;
  for( std::size_t l = 0 ; l < W ; ++l ) {
   std::size_t k = fftLane< T , W >( l );
   if( wrap && o + u * W + k >= m ) k -= m;
   re[ l ] = a[ k ].r;
   im[ l ] = a[ k ].i;
   }
  transpose< T , W >( re );
  transpose< T , W >( im );
  const bool short_ = cut && ( b + 1 ) * W > r;
  for( std::size_t l = 0 ; l < W ; ++l ) {
   const std::size_t j = b * W + fftLane< T , W >( l );
   if( short_ && j >= r ) continue;
   const C y{ re[ l ] , im[ l ] };
   if( ! split ) {
    const C & w = Tw->t[ u * r + j ];
    c[ j ] = cmul< false >( y , w.r , w.i );
    }
   else if( wrap ) {
    const C & w = Tw->w[ j ];
    c[ j ] = cmul< false >( y , w.r , w.i );
    }
   else {
    const T * s = Tw->s.data() + 2 * ( u * r + j );
    const C w = cmul< false >( Tw->t[ j ] , splat< T , W >( s[ 0 ] ) ,
                               splat< T , W >( s[ 1 ] ) );
    c[ j ] = cmul< false >( y , w.r , w.i );
    }
   }
  }

 /// the W numbers of the row i of x from the column t W on, and the
 /// following ones up to r only in the last block
 ADS_FFT_INLINE C input( std::size_t i , std::size_t t ) const {
  const T * a = xi + 2 * ( r * i + t * W );
  C y = ( t + 1 ) * W <= r ? loadComplex< T , W >( a )
                           : loadSome< T , W >( a , r - t * W );
  y.i = y.i * ci;
  return( y );
  }

 /// the item y of an output scaled, and conjugated for the inverse
 ADS_FFT_INLINE C scale( C y ) const {
  y.r = y.r * fr;
  y.i = y.i * fi;
  return( y );
  }
 };

/// the first step on the columns of length N <= fftCodelet, by codelets:
/// the block t of W columns goes to the block t of B
template< std::size_t N , class T , std::size_t W >
ADS_FFT_NOINLINE void fourStepColumns( const FFTFour< T , W > & F )
{
 using C = FFTCx< FFTLane< T , W > >;
 const T * R = codeletRoots< T >();
 const std::size_t r = F.r , full = r / W;
 for( std::size_t t = 0 ; t < full ; ++t ) {
  C * o = F.B + t * F.M;
  const T * a = F.xi + 2 * t * W;
  auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
   C y = loadComplex< T , W >( a + 2 * r * i );
   y.i = y.i * F.ci;
   return( y );
   };
  auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
   o[ k ] = y;
   };
  codelet< N , false , T , W >( in , out , R );
  }
 if( full < F.rb ) {
  C * o = F.B + full * F.M;
  auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
   return( F.input( i , full ) );
   };
  auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
   o[ k ] = y;
   };
  codelet< N , false , T , W >( in , out , R );
  }
 }

/// the last step on the rows of length N <= fftCodelet, by codelets: the
/// block u of W rows of B, times the powers of omega_n, gives the block u
/// of W numbers of the rows of the output
template< std::size_t N , class T , std::size_t W >
ADS_FFT_NOINLINE void fourStepRows( const FFTFour< T , W > & F )
{
 using C = FFTCx< FFTLane< T , W > >;
 const T * R = codeletRoots< T >();
 for( std::size_t u = 0 ; u < F.mb ; ++u ) {
  C c[ ( N + W - 1 ) / W * W ];
  for( std::size_t b = 0 ; b < F.rb ; ++b )
   F.template rowBlock< N % W != 0 >( u , b , c );
  const std::size_t k = F.o + u * W;
  FFTArray< C > in{ c };
  if( F.edge( u ) ) {
   auto out = [ & ]( std::size_t i , const C & y ) ADS_FFT_LAMBDA {
    F.output( i , k , F.scale( y ) );
    };
   codelet< N , false , T , W >( in , out , R );
   continue;
   }
  T * a = F.x + 2 * k;
  const std::size_t m = F.m;
  auto out = [ & ]( std::size_t i , const C & y ) ADS_FFT_LAMBDA {
   storeComplex< T , W >( a + 2 * m * i , F.scale( y ) );
   };
  codelet< N , false , T , W >( in , out , R );
  }
 }

/// the lengths of the columns and of the rows of the four steps done by a
/// codelet: the powers of 2 from 8, the sizes 2^a 3^b 5^c from 12 to 60
/// (to 25 if ADS_FFT_LEAN), so that the sizes with those factors go in two
/// passes; the others go by the rounds
#if ADS_FFT_LEAN
inline constexpr std::size_t fftStepSizes[] = { 8 , 16 , 32 , 64 , 12 , 15 ,
                                                20 , 25 };
#else
inline constexpr std::size_t fftStepSizes[] = { 8 , 16 , 32 , 64 , 12 , 15 ,
                                                20 , 25 , 24 , 30 , 40 , 45 ,
                                                48 , 50 , 60 };
#endif

/// whether the columns or the rows of L points of the four steps on lanes
/// of W go by a codelet
template< std::size_t W >
inline bool fftStepCodelet( std::size_t L )
{
 if( L < W || ( W == 1 && L > fftScalarCodelet ) ) return( false );
 return( std::find( std::begin( fftStepSizes ) , std::end( fftStepSizes ) ,
                    L ) != std::end( fftStepSizes ) );
 }

/// f.template operator()< L >() if fftStepCodelet< W >( L ), and true;
/// else false
template< std::size_t W , class F >
ADS_FFT_INLINE bool byStepCodelet( std::size_t L , F && f )
{
 if( ! fftStepCodelet< W >( L ) ) return( false );
 unroll< std::size( fftStepSizes ) >( [ & ]( auto I ) {
  constexpr std::size_t N = fftStepSizes[ I ];
  if constexpr( N >= W && ( W > 1 || N <= fftScalarCodelet ) )
   if( L == N ) f.template operator()< N >();
  } );
 return( true );
 }

/// the transform of the n = r m points of x in place, r , m >= W, by the
/// four steps of Bailey on the matrix of m rows and r columns of x [see
/// FFTFour]: the transforms of length m of its columns, W at a time, kept
/// in B; the powers of omega_n; the transforms of length r of the rows of
/// B, W at a time, whose outputs are those of x in their order. The lanes
/// of an item hold the columns, then the rows, in the order of fftLane(),
/// which loadComplex() and storeComplex() read and write. The transforms
/// not by a codelet go fftStrips at a time, by the rounds sm and sr,
/// gathered from x and scattered to it a few lines at a time, fftAhead
/// rows ahead brought into the cache; roots( o ) gives the powers of
/// omega_n of the shift o. The inverse transform is the conjugate of the
/// forward one of the conjugates, divided by n: the imaginary parts of the
/// inputs and of the outputs times -1, so that the codelets are the same
template< class T , std::size_t W , class G >
inline void fourStep( const T * xi , T * x , std::size_t r , std::size_t m ,
                      const FFTSteps< T > & sm , const FFTSteps< T > & sr ,
                      G && roots , bool inv )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 constexpr std::size_t S = fftStrips , P = fftAhead ,
                       Q = 64 / ( 2 * sizeof( T ) * W ) ?
                           64 / ( 2 * sizeof( T ) * W ) : 1;
 const std::size_t n = r * m , L = std::max( r , m ) , M = m + W ,
                   o = m % W ? 0 : fftShift< T , W >( x ) ,
                   rb = ( r + W - 1 ) / W , mb = ( m + W - 1 ) / W;
 C * B = fftBuffer< C >( rb * M + 3 * S * ( L + W ) );
 C * g = B + rb * M , * u = g + S * ( L + W ) , * v = u + S * ( L + W );
 const T f = inv ? T( 1 ) / T( n ) : T( 1 );
 const FFTFour< T , W > F{ xi , x , r , m , o , M , rb , mb , B ,
                           & roots( o ) , n >= fftSplitRoots ,
                           splat< T , W >( inv ? T( -1 ) : T( 1 ) ) ,
                           splat< T , W >( f ) ,
                           splat< T , W >( inv ? - f : f ) };
 if( ! byStepCodelet< W >( m , [ & ]< std::size_t N >() {   // the columns
        fourStepColumns< N , T , W >( F );
        } ) )
  for( std::size_t t0 = 0 ; t0 < rb ; t0 += S ) {
   const std::size_t St = std::min( S , rb - t0 );
   for( std::size_t i = 0 ; i < m ; ++i ) {
    if( i + P < m )
     for( std::size_t t = 0 ; t < St ; t += Q )
      ADS_FFT_PREFETCH( xi + 2 * ( r * ( i + P ) + ( t0 + t ) * W ) , 0 );
    for( std::size_t t = 0 ; t < St ; ++t )
     g[ t * m + i ] = F.input( i , t0 + t );
    }
   for( std::size_t t = 0 ; t < St ; ++t )
    batched< T , W >( sm , g + t * m , B + ( t0 + t ) * M , u , v );
   }
 if( ! byStepCodelet< W >( r , [ & ]< std::size_t N >() {   // the rows
        fourStepRows< N , T , W >( F );
        } ) ) {
  for( std::size_t u0 = 0 ; u0 < mb ; u0 += S ) {
   const std::size_t St = std::min( S , mb - u0 );
   for( std::size_t t = 0 ; t < St ; ++t ) {
    for( std::size_t b = 0 ; b < rb ; ++b ) {
     if( b + 4 < rb )                       // 4 blocks ahead
      for( std::size_t l = 0 ; l < W ; ++l )
       ADS_FFT_PREFETCH( B + ( b + 4 ) * M + o + ( u0 + t ) * W + l , 0 );
     F.template rowBlock<>( u0 + t , b , v );
     }
    batched< T , W >( sr , v , g + t * r , u , v + r + W );
    }
   for( std::size_t i = 0 ; i < r ; ++i )
    for( std::size_t t = 0 ; t < St ; ++t ) {
     if( i + P < r && t % Q == 0 )
      ADS_FFT_PREFETCH( x + 2 * ( m * ( i + P ) + o + ( u0 + t ) * W ) , 1 );
     F.output( i , o + ( u0 + t ) * W , F.scale( g[ t * r + i ] ) );
     }
   }
  }
 }

/// whether n has a codelet, by a table up to fftCodelet
inline bool fftHasCodelet( std::size_t n )
{
 static constexpr auto H = [] {
  std::array< bool , fftCodelet + 1 > h{};
  for( std::size_t N = 1 ; N <= fftCodelet ; N *= 2 ) h[ N ] = true;
  for( std::size_t N : fftMixed ) h[ N ] = true;
  return( h );
  }();
 return( n <= fftCodelet && H[ n ] );
 }

/// the powers of omega_n of smallFour() on r m = n points: omega_n^{j ( u
/// W + fftLane( l ) )} as the item u r + j, one lane per l; made once
template< std::size_t r , std::size_t m , class T , std::size_t W >
inline const FFTCx< FFTLane< T , W > > * smallFourRoots()
{
 using C = FFTCx< FFTLane< T , W > >;
 static const std::vector< C > t = [] {
  std::vector< C > v( m / W * r );
  for( std::size_t u = 0 ; u < m / W ; ++u )
   for( std::size_t j = 0 ; j < r ; ++j )
    for( std::size_t l = 0 ; l < W ; ++l ) {
     const std::complex< T > z = omega< T >( j * ( u * W +
                                             fftLane< T , W >( l ) ) ,
                                             r * m );
     v[ u * r + j ].r[ l ] = z.real();
     v[ u * r + j ].i[ l ] = z.imag();
     }
  return( v );
  }();
 return( t.data() );
 }

/// the transform of the n = r m points of xi into x, which may be xi, by
/// the four steps of fourStep() unrolled, W and r and m small multiples of
/// W: the transforms of the columns by codelets into an array of n / W
/// items, then those of its rows, gathered by blocks of W x W transposed
/// in registers, times the powers of omega_n; the inverse as fourStep()
template< std::size_t r , std::size_t m , class T , std::size_t W >
ADS_FFT_NOINLINE void smallFour( const T * xi , T * x , bool inv )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 constexpr std::size_t rb = r / W , mb = m / W;
 const T * R = codeletRoots< T >();
 const C * tw = smallFourRoots< r , m , T , W >();
 const T f = inv ? T( 1 ) / T( r * m ) : T( 1 );
 const V ci = splat< T , W >( inv ? T( -1 ) : T( 1 ) ) ,
         fr = splat< T , W >( f ) , fi = splat< T , W >( inv ? - f : f );
 C B[ rb * m ];
 for( std::size_t b = 0 ; b < rb ; ++b ) {
  const T * a = xi + 2 * b * W;
  C * o = B + b * m;
  auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
   C y = loadComplex< T , W >( a + 2 * r * i );
   y.i = y.i * ci;
   return( y );
   };
  auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
   o[ k ] = y;
   };
  codelet< m , false , T , W >( in , out , R );
  }
 for( std::size_t u = 0 ; u < mb ; ++u ) {
  C c[ r ];
   for( std::size_t b = 0 ; b < rb ; ++b ) {
   V re[ W ] , im[ W ];
   for( std::size_t l = 0 ; l < W ; ++l ) {
    const C & y = B[ b * m + u * W + fftLane< T , W >( l ) ];
    re[ l ] = y.r;
    im[ l ] = y.i;
    }
   transpose< T , W >( re );
   transpose< T , W >( im );
   for( std::size_t l = 0 ; l < W ; ++l ) {
    const std::size_t j = b * W + fftLane< T , W >( l );
    const C & w = tw[ u * r + j ];
    c[ j ] = cmul< false >( C{ re[ l ] , im[ l ] } , w.r , w.i );
    }
   }
  FFTArray< C > in{ c };
  T * a = x + 2 * u * W;
  auto out = [ & ]( std::size_t i , C y ) ADS_FFT_LAMBDA {
   y.r = y.r * fr;
   y.i = y.i * fi;
   storeComplex< T , W >( a + 2 * m * i , y );
   };
  codelet< r , false , T , W >( in , out , R );
  }
 }

/// the shapes of smallFour(): n = r m points on lanes of W numbers (none
/// if W is 0: 2048 points in single precision only)
struct FFTShape {
 std::size_t n , r , m , W;
 };

template< class T >
inline constexpr FFTShape fftSmallShapes[] = {
 { 16 , 4 , 4 , 4 } , { 32 , 4 , 8 , 4 } , { 48 , 4 , 12 , 4 } ,
 { 64 , 8 , 8 , std::min< std::size_t >( 8 , fftWidthOf< T > ) } ,
 { 96 , 8 , 12 , 4 } ,
 { 128 , 8 , 16 , std::min< std::size_t >( 8 , fftWidthOf< T > ) } ,
 { 256 , 16 , 16 , std::min< std::size_t >( 8 , fftWidthOf< T > ) }
#if ! ADS_FFT_LEAN
 , { 512 , 16 , 32 , std::min< std::size_t >( 8 , fftWidthOf< T > ) } ,
 { 1024 , 32 , 32 , std::min< std::size_t >( 8 , fftWidthOf< T > ) } ,
 { 2048 , 32 , 64 , sizeof( T ) == 4 ? std::min< std::size_t >(
                                         8 , fftWidthOf< T > ) : 0 }
#endif
 };

/// the transform of the n points of xi into x by smallFour() if n is a
/// shape of fftSmallShapes whose lanes the build has, and true; else false
template< class T >
ADS_FFT_INLINE bool bySmallFour( const T * xi , T * x , std::size_t n ,
                                 bool inv )
{
 bool done = false;
 unroll< std::size( fftSmallShapes< T > ) >( [ & ]( auto I ) {
  constexpr FFTShape S = fftSmallShapes< T >[ I ];
  if constexpr( S.W <= fftWidthOf< T > && S.W > 1 )
   if( n == S.n ) {
    smallFour< S.r , S.m , T , S.W >( xi , x , inv );
    done = true;
    }
  } );
 return( done );
 }

/// whether the powers of 2 from 16 to fftCodelet go by smallFour(): the
/// lanes of 4 numbers at least
template< class T >
inline constexpr bool fftSmallCovers = fftWidthOf< T > >= 4;

/// the transform of the n points of x in place, n the size of a codelet
/// not done by smallFour(), by one codelet on single lanes, and true; else
/// false
template< class T >
inline bool smallTransform( T * x , std::size_t n , bool inv )
{
 using C = FFTCx< T >;
 const T f = inv ? T( 1 ) / T( n ) : T( 1 ) , ci = inv ? T( -1 ) : T( 1 );
 auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
  return( C{ x[ 2 * i ] , x[ 2 * i + 1 ] * ci } );
  };
 auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
  x[ 2 * k ] = y.r * f;
  x[ 2 * k + 1 ] = y.i * f * ci;
  };
 return( byAnyCodelet( n , [ & ]< std::size_t N >() {
  if constexpr( ! ( fftSmallCovers< T > && N >= 16 &&
                    ( N & ( N - 1 ) ) == 0 ) )
   codelet< N , false , T , 1 >( in , out , codeletRoots< T >() );
  } ) && ! ( fftSmallCovers< T > && n >= 16 && ( n & ( n - 1 ) ) == 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PLANS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the largest prime the rounds take, by stockhamPrime() above the
/// codelets, at a cost of about p / 2 products per point; a larger prime
/// goes by the algorithm of Rader if it is n, otherwise n by that of
/// Bluestein
inline constexpr std::size_t fftMaxPrime = 101;

/// log_2 of the length of the rows of the four steps on 2^K points: 32
/// up to 2^15 points and 64 at 2^12 and 2^16, the codelets that keep most
/// of their items in registers, so that only the columns go by the rounds
/// of batched(); 2^{floor( K / 2 )} below 2^10 and above 2^16
inline constexpr unsigned fftRows( unsigned K )
{
 return( K <= 11 ? std::min( 5u , K / 2 ) : K == 12 ? 6
         : K <= 15 ? 5 : K == 16 ? 6 : K / 2 );
 }

/// the sizes up to which a plan of the four steps times its first
/// fftTuned factorizations n = r m, by their order in fftPlan(), and keeps
/// the fastest (0: none, the first always), as FFTW_MEASURE does among many
/// more; the plans then may differ from run to run in the last bits of the
/// outputs
#ifndef ADS_FFT_TUNE
 #define ADS_FFT_TUNE 131072
#endif
inline constexpr std::size_t fftTuneMax = ADS_FFT_TUNE , fftTuned = 4;

/// how the transform of n points goes: by the rounds of batched() on
/// single lanes (Scalar), by the four steps on rows of r and columns of m
/// points (Four), by the algorithm of Bluestein on a convolution of M
/// points (Bluestein) or by that of Rader on one of n - 1 (Rader)
enum class FFTKind { Scalar , Four , Bluestein , Rader };

template< class T , std::size_t W >
struct FFTPlan {
 FFTKind kind = FFTKind::Scalar;
 std::size_t n = 0 , r = 0 , m = 0 , M = 0;
 FFTSteps< T > s1 , s2;       ///< the rounds of n, or of m and r
 /// the powers of omega_n of the four steps, for each shift
 std::unique_ptr< FFTFourRoots< T , W > > roots[ 16 ];
 /// Bluestein: the chirp w_j = omega_{2n}^{j^2}, j < n, and the transform
 /// of conj( w ) on M points; Rader: the transform of omega_n^{g^{-q}}, q
 /// < n - 1, on n - 1 points, the powers g^q in perm, those of g^{-q} in
 /// iperm
 std::vector< std::complex< T > > chirp , kernel;
 std::vector< std::size_t > perm , iperm;

 const FFTFourRoots< T , W > & fourRoots( std::size_t o ) {
  if( ! roots[ o ] ) {
   roots[ o ] = std::make_unique< FFTFourRoots< T , W > >();
   fourStepRoots( * roots[ o ] , r , m , o );
   }
  return( * roots[ o ] );
  }
 };

/// a rough cost of the rounds of n points, prime factors f: n times the sum
/// of the costs of the radices, about log_2 R for those with a codelet and
/// R / 2 for the others
inline double fftRoundsCost( std::size_t n ,
                             const std::vector< std::size_t > & f )
{
 double c = 0;
 for( std::size_t p : f )
  c += p == 2 ? 1 : p <= 13 ? 1.3 * std::log2( double( p ) )
                            : 0.5 * double( p );
 return( double( n ) * c );
 }

/// the smallest M >= n of the form 2^a 3^b 5^c
inline std::size_t fftGoodSize( std::size_t n )
{
 std::size_t best = std::bit_ceil( n );
 for( std::size_t f5 = 1 ; f5 < best ; f5 *= 5 )
  for( std::size_t f35 = f5 ; f35 < best ; f35 *= 3 ) {
   std::size_t x = f35;
   while( x < n ) x *= 2;
   best = std::min( best , x );
   }
 return( best );
 }

/// a b mod p, by halves of the numbers if their product may not fit in
/// 64 bits
inline std::size_t fftMulMod( std::size_t a , std::size_t b , std::size_t p )
{
 if( p <= ( std::size_t( 1 ) << 32 ) ) return( a % p * ( b % p ) % p );
 std::size_t y = 0;
 for( a %= p , b %= p ; b ; b >>= 1 ) {     // by doubling, below 2^63
  if( b & 1 ) y = y >= p - a ? y - ( p - a ) : y + a;
  a = a >= p - a ? a - ( p - a ) : a + a;
  }
 return( y );
 }

/// whether g generates the multiplicative group of the integers mod the
/// prime p, whose order p - 1 has the distinct primes f
inline bool fftGenerates( std::size_t g , std::size_t p ,
                          const std::vector< std::size_t > & f )
{
 for( std::size_t q : f ) {
  std::size_t y = 1 , b = g , e = ( p - 1 ) / q;
  for( ; e ; e >>= 1 , b = fftMulMod( b , b , p ) )
   if( e & 1 ) y = fftMulMod( y , b , p );
  if( y == 1 ) return( false );
  }
 return( true );
 }

template< bool inv , std::size_t W = 0 , class T >
inline void transform( std::complex< T > * a , std::size_t n );

template< std::size_t W = 0 , class T >
inline void transformTo( const std::complex< T > * a , std::complex< T > * b ,
                         std::size_t n , bool inv );

/// the plan of n > fftCodelet points or not a power of 2, made once per
/// thread: the four steps where n has a divisor r >= W with n / r >= W
/// and n is not small, its primes up to fftMaxPrime, r chosen near the
/// length of fftRows() (a power of 2 for those of 2^K); the algorithm of
/// Rader for a larger prime, if the rounds of n - 1 cost less than those
/// of Bluestein, which take any other n; the rounds on single lanes for
/// the rest
template< class T , std::size_t W >
inline FFTPlan< T , W > & fftPlan( std::size_t n )
{
 thread_local std::unordered_map< std::size_t ,
                                  std::unique_ptr< FFTPlan< T , W > > > P;
 thread_local FFTPlan< T , W > * last = nullptr;
 if( last && last->n == n ) return( * last );
 auto & slot = P[ n ];
 if( ! slot ) {
  slot = std::make_unique< FFTPlan< T , W > >();
  FFTPlan< T , W > & p = * slot;
  p.n = n;
  const std::vector< std::size_t > f = fftFactors( n );
  const std::size_t big = f.back();
  if( big > 13 && ( f.size() == 1 || big > fftMaxPrime ) ) {
   const std::size_t Mb = fftGoodSize( 2 * n - 1 );
   const double inf = 1e300 ,
    direct = big <= fftMaxPrime ? fftRoundsCost( n , f ) : inf ,
    blue = 3 * fftRoundsCost( Mb , fftFactors( Mb ) ) + 8 * double( Mb ) ,
    rad = f.size() == 1 ? 2 * fftRoundsCost( n - 1 , fftFactors( n - 1 ) )
                          + 8 * double( n ) : inf;
   if( rad < direct && rad <= blue ) {
    p.kind = FFTKind::Rader;
    std::vector< std::size_t > g = fftFactors( n - 1 );
    g.erase( std::unique( g.begin() , g.end() ) , g.end() );
    std::size_t gen = 2;
    while( ! fftGenerates( gen , n , g ) ) ++gen;
    p.perm.resize( n - 1 );
    p.iperm.resize( n - 1 );
    for( std::size_t q = 0 , x = 1 ; q < n - 1 ; ++q ) {
     p.perm[ q ] = x;                       // g^q
     x = fftMulMod( x , gen , n );
     }
    for( std::size_t q = 0 ; q < n - 1 ; ++q )
     p.iperm[ q ] = p.perm[ ( n - 1 - q ) % ( n - 1 ) ];   // g^{-q}
    p.kernel.resize( n - 1 );
    for( std::size_t q = 0 ; q < n - 1 ; ++q )
     p.kernel[ q ] = omega< T >( p.iperm[ q ] , n );
    transform< false , W >( p.kernel.data() , n - 1 );
    }
   else if( blue < direct ) {
    p.kind = FFTKind::Bluestein;
    p.M = Mb;
    p.chirp.resize( n );
    for( std::size_t j = 0 ; j < n ; ++j )
     p.chirp[ j ] = omega< T >( fftMulMod( j , j , 2 * n ) , 2 * n );
    p.kernel.assign( Mb , std::complex< T >( 0 ) );
    for( std::size_t j = 0 ; j < n ; ++j ) {
     p.kernel[ j ] = std::conj( p.chirp[ j ] );
     if( j ) p.kernel[ Mb - j ] = std::conj( p.chirp[ j ] );
     }
    transform< false , W >( p.kernel.data() , Mb );
    }
   }
  if( p.kind == FFTKind::Scalar ) {
   // the divisors r of n by their distance from the length of the rows
   // of fftRows() in the steps of log_2, those that W divides together
   // with n / r and those with codelets first
   const double target = double( fftRows( unsigned( std::bit_width( n ) -
                                                    1 ) ) );
   std::vector< std::pair< double , std::size_t > > cand;
   const std::size_t w2 = std::max< std::size_t >( W , 2 );
   for( std::size_t r = w2 ; r * w2 <= n ; ++r )
    if( n % r == 0 )
     cand.emplace_back( std::abs( std::log2( double( r ) ) - target ) +
                        ( r % W || ( n / r ) % W ? 1.5 : 0 ) +
                        ( fftStepCodelet< W >( r ) ? 0 : 0.5 ) +
                        ( fftStepCodelet< W >( n / r ) ? 0 : 0.5 ) , r );
   std::sort( cand.begin() , cand.end() );
   if( ! cand.empty() && n >= 2 * W * W ) {
    p.kind = FFTKind::Four;
    auto set = [ & ]( std::size_t r ) {
     p.r = r;
     p.m = n / r;
     p.s1 = fftSteps< T >( p.m , W > 1 ? fftRadix : fftScalarRadix );
     p.s2 = fftSteps< T >( p.r , W > 1 ? fftRadix : fftScalarRadix );
     for( auto & q : p.roots ) q.reset();
     };
    set( cand[ 0 ].second );
    const std::size_t k = std::min( fftTuned , cand.size() );
    if( n <= fftTuneMax && k > 1 ) {          // the fastest of the first
     std::vector< std::complex< T > > z( n , std::complex< T >( 1 ) );
     T * x = reinterpret_cast< T * >( z.data() );
     double best = 1e300;
     std::size_t rb = cand[ 0 ].second;
     for( std::size_t c = 0 ; c < k ; ++c ) {
      set( cand[ c ].second );
      double t = 1e300;
      for( int rep = 0 ; rep < 3 ; ++rep ) {
       const auto t0 = std::chrono::steady_clock::now();
       fourStep< T , W >( x , x , p.r , p.m , p.s1 , p.s2 ,
                          [ & ]( std::size_t o ) -> const auto & {
                           return( p.fourRoots( o ) ); } , false );
       t = std::min( t , std::chrono::duration< double >(
                          std::chrono::steady_clock::now() - t0 ).count() );
       }
      if( t < best ) { best = t; rb = cand[ c ].second; }
      }
     set( rb );
     }
    }
   else p.s1 = fftSteps< T >( n , fftScalarRadix );
   }
  }
 last = slot.get();
 return( * slot );
 }

/// the transform of the n points of a into b by the rounds of batched()
/// on single lanes
template< class T >
inline void scalarTransform( const std::complex< T > * a ,
                             std::complex< T > * b , std::size_t n ,
                             const FFTSteps< T > & s , bool inv )
{
 using C = FFTCx< T >;
 if( inv ) {                               // the conjugates, forward
  for( std::size_t k = 0 ; k < n ; ++k ) b[ k ] = std::conj( a[ k ] );
  a = b;
  }
 const C * x = reinterpret_cast< const C * >( a );   // [complex.numbers]
 C * u = fftBuffer< C >( 2 * n ) , * v = u + n;
 batched< T , 1 >( s , x , reinterpret_cast< C * >( b ) , u , v );
 if( inv ) {
  const T f = T( 1 ) / T( n );
  for( std::size_t k = 0 ; k < n ; ++k ) b[ k ] = std::conj( b[ k ] ) * f;
  }
 }

/// a buffer of n complex numbers for the algorithms of Bluestein and of
/// Rader, kept per thread, one for each depth of the calls
template< class T >
inline std::complex< T > * fftScratch( std::size_t n , unsigned depth )
{
 thread_local std::vector< std::complex< T > > B[ 4 ];
 if( B[ depth ].size() < n ) B[ depth ].resize( n );
 return( B[ depth ].data() );
 }

/// the product of two complex numbers by its formula: the operator * of
/// std::complex also handles the infinities and the NaNs (Annex G of C),
/// a call that costs more than the four products here
template< class T >
inline std::complex< T > times( const std::complex< T > & a ,
                                const std::complex< T > & b )
{
 return( std::complex< T >( a.real() * b.real() - a.imag() * b.imag() ,
                            a.real() * b.imag() + a.imag() * b.real() ) );
 }

/// the forward transform of the n points of a by the algorithm of
/// Bluestein: X_k = w_k sum_j ( x_j w_j ) conj( w_{k-j} ), w_j =
/// omega_{2n}^{j^2}, a convolution of M >= 2 n - 1 points by transforms
template< class T , std::size_t W >
inline void bluestein( std::complex< T > * a , const FFTPlan< T , W > & p )
{
 const std::size_t n = p.n , M = p.M;
 std::complex< T > * z = fftScratch< T >( M , 1 );
 for( std::size_t j = 0 ; j < n ; ++j )
  z[ j ] = times( a[ j ] , p.chirp[ j ] );
 std::fill( z + n , z + M , std::complex< T >( 0 ) );
 transform< false , W >( z , M );
 for( std::size_t k = 0 ; k < M ; ++k )
  z[ k ] = times( z[ k ] , p.kernel[ k ] );
 transform< true , W >( z , M );
 for( std::size_t k = 0 ; k < n ; ++k )
  a[ k ] = times( z[ k ] , p.chirp[ k ] );
 }

/// the forward transform of the n points of a, n a prime, by the algorithm
/// of Rader: X_0 = sum_j x_j and, for q < n - 1, X_{g^{-q}} = x_0 + sum_j
/// x_{g^j} omega_n^{g^{j-q}}, a cyclic convolution of n - 1 points
template< class T , std::size_t W >
inline void rader( std::complex< T > * a , const FFTPlan< T , W > & p )
{
 const std::size_t n = p.n , L = n - 1;
 std::complex< T > * z = fftScratch< T >( L , 2 );
 std::complex< T > s = a[ 0 ];
 for( std::size_t q = 0 ; q < L ; ++q ) {
  z[ q ] = a[ p.perm[ q ] ];
  s += z[ q ];
  }
 transform< false , W >( z , L );
 for( std::size_t q = 0 ; q < L ; ++q )
  z[ q ] = times( z[ q ] , p.kernel[ q ] );
 transform< true , W >( z , L );
 const std::complex< T > x0 = a[ 0 ];
 a[ 0 ] = s;
 for( std::size_t q = 0 ; q < L ; ++q ) a[ p.iperm[ q ] ] = x0 + z[ q ];
 }

/// the transform of the n points of a into b, which may be a, the inverse
/// one divided by n if inv: one codelet up to fftCodelet points a power of
/// 2, above it the plan of n, on lanes of W numbers (the widest if W is
/// 0); the algorithms of Bluestein and Rader do the inverse transform as
/// the conjugate of the forward one of the conjugates
template< std::size_t W0 , class T >
inline void transformTo( const std::complex< T > * a , std::complex< T > * b ,
                         std::size_t n , bool inv )
{
 constexpr std::size_t W = W0 ? W0 : fftWidthOf< T >;
 static_assert( W <= 16 && 8 * 8 <= fftCodelet );
 const T * xi = reinterpret_cast< const T * >( a );   // [complex.numbers]
 T * x = reinterpret_cast< T * >( b );
 if constexpr( W0 != 1 )
  if( bySmallFour( xi , x , n , inv ) ) return;
 if( n <= 1 ) {
  if( n ) * b = * a;
  return;
  }
 if( fftHasCodelet( n ) ) {
  if( a != b ) std::copy( a , a + n , b );
  if( smallTransform( x , n , inv ) ) return;
  xi = x;                                  // in place from here
  a = b;
  }
 if constexpr( W0 == 0 && W >= 4 && ! ADS_FFT_LEAN )
  if( n < 2 * W * W ) {           // too few for the widest lanes: half
   transformTo< W / 2 >( a , b , n , inv );
   return;
   }
 FFTPlan< T , W > & p = fftPlan< T , W >( n );
 switch( p.kind ) {
  case( FFTKind::Scalar ):
   scalarTransform( a , b , n , p.s1 , inv );
   break;
  case( FFTKind::Four ):
   fourStep< T , W >( xi , x , p.r , p.m , p.s1 , p.s2 ,
                      [ & ]( std::size_t o ) -> const auto & {
                       return( p.fourRoots( o ) ); } , inv );
   break;
  default: {
   if( inv )
    for( std::size_t k = 0 ; k < n ; ++k ) b[ k ] = std::conj( a[ k ] );
   else if( a != b ) std::copy( a , a + n , b );
   if( p.kind == FFTKind::Bluestein ) bluestein( b , p );
   else rader( b , p );
   if( inv ) {
    const T f = T( 1 ) / T( n );
    for( std::size_t k = 0 ; k < n ; ++k ) b[ k ] = std::conj( b[ k ] ) * f;
    }
   }
  }
 }

template< bool inv , std::size_t W0 , class T >
inline void transform( std::complex< T > * a , std::size_t n )
{
 transformTo< W0 >( a , a , n , inv );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE REAL TRANSFORMS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the powers omega_n^k , k <= n / 4, of the real transforms of n points,
/// kept per thread
template< class T >
inline const std::complex< T > * fftHalfRoots( std::size_t n )
{
 thread_local std::unordered_map< std::size_t ,
                                  std::vector< std::complex< T > > > H;
 thread_local std::size_t lastN = 0;
 thread_local const std::complex< T > * lastP = nullptr;
 if( lastN == n ) return( lastP );
 std::vector< std::complex< T > > & t = H[ n ];
 if( t.empty() ) {
  t.resize( n / 4 + 1 );
  for( std::size_t k = 0 ; k <= n / 4 ; ++k ) t[ k ] = omega< T >( k , n );
  }
 lastN = n;
 lastP = t.data();
 return( lastP );
 }

/// the pass between the complex transform of h = n / 2 points and the real
/// one of n, on the pairs k , h - k, 0 < k < h / 2 (and k = h / 2 if h is
/// even), from X into Y, which may be X: forward, with u = Z_k and v =
/// conj( Z_{h-k} ), e = ( u + v ) / 2, d = ( u - v ) / 2 and t = -i
/// omega_n^k d, X_k = e + t and X_{h-k} = conj( e - t ); backward, with u
/// = X_k and v = conj( X_{h-k} ), t = i conj( omega_n^k ) d, Z_k = e + t
/// and Z_{h-k} = conj( e - t). W pairs at a time in the lanes, the items h
/// - k loaded and stored in the reverse order, while the two blocks do not
/// meet; w holds omega_n^k , k <= n / 4
template< bool fwd , class T >
inline void halfPass( const std::complex< T > * X , std::complex< T > * Y ,
                      std::size_t h , const std::complex< T > * w )
{
 constexpr std::size_t W = fftWidthOf< T >;
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 const V half = splat< T , W >( T( 0.5 ) );
 const T * x = reinterpret_cast< const T * >( X );
 T * y = reinterpret_cast< T * >( Y );
 const T * wr = reinterpret_cast< const T * >( w );
 std::size_t k = 1;
 for( ; 2 * ( k + W ) <= h + 1 ; k += W ) {   // k + W - 1 < h - k - W + 1
  const std::size_t b = h - k - W + 1;
  const C u = loadComplex< T , W >( x + 2 * k ) ,
          z = loadComplex< T , W >( x + 2 * b ) ,
          c = loadComplex< T , W >( wr + 2 * k );
  const C v{ reverse< T , W >( z.r ) , - reverse< T , W >( z.i ) };
  const C e{ ( u.r + v.r ) * half , ( u.i + v.i ) * half } ,
          d{ ( u.r - v.r ) * half , ( u.i - v.i ) * half };
  C t;
  if constexpr( fwd ) t = cmul< false >( mulMinusI< false >( d ) , c.r ,
                                         c.i );
  else t = mulMinusI< true >( cmul< true >( d , c.r , c.i ) );
  const C p = e + t , q = e - t;
  storeComplex< T , W >( y + 2 * k , p );
  storeComplex< T , W >( y + 2 * b , C{ reverse< T , W >( q.r ) ,
                                        - reverse< T , W >( q.i ) } );
  }
 for( ; 2 * k <= h ; ++k ) {
  const std::complex< T > u = X[ k ] , v = std::conj( X[ h - k ] ) ,
                          e = T( 0.5 ) * ( u + v ) , d = T( 0.5 ) * ( u - v ) ,
                          t = fwd ? times( w[ k ] ,
                                           std::complex< T >( d.imag() ,
                                                              - d.real() ) )
                                  : times( std::complex< T >( 0 , 1 ) ,
                                           times( std::conj( w[ k ] ) , d ) );
  Y[ k ] = e + t;
  if( 2 * k < h ) Y[ h - k ] = std::conj( e - t );
  }
 }

/// the transform of the n real numbers x into the n / 2 + 1 complex ones
/// X, X_k = sum_j x_j omega_n^{j k}: for n even, the complex transform of
/// the h = n / 2 points z_j = x_{2j} + i x_{2j+1} in X, then with E_k = (
/// Z_k + conj( Z_{h-k} ) ) / 2, O_k = ( Z_k - conj( Z_{h-k} ) ) / 2 i
/// and t = omega_n^k O_k, X_k = E_k + t and X_{h-k} = conj( E_k - t ); for
/// n odd, the complex transform of the n points
template< class T >
inline void realForward( const T * x , std::size_t n , std::complex< T > * X )
{
 if( n == 0 ) return;
 if( n == 1 ) { X[ 0 ] = x[ 0 ]; return; }
 if( n % 2 ) {
  std::complex< T > * z = fftScratch< T >( n , 3 );
  for( std::size_t j = 0 ; j < n ; ++j ) z[ j ] = x[ j ];
  transform< false >( z , n );
  std::copy( z , z + n / 2 + 1 , X );
  X[ 0 ] = X[ 0 ].real();                   // 0 by the symmetry
  return;
  }
 const std::size_t h = n / 2;
 if( h <= 32 && byAnyCodelet< 32 >( h , [ & ]< std::size_t N >() {
      FFTCx< T > Z[ N ];               // small: the codelet into Z, the pass
      auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
       return( FFTCx< T >{ x[ 2 * i ] , x[ 2 * i + 1 ] } );
       };
      FFTArray< FFTCx< T > > out{ Z };
      codelet< N , false , T , 1 >( in , out , codeletRoots< T >() );
      std::complex< T > * Y = reinterpret_cast< std::complex< T > * >( Z );
      const T z0r = Z[ 0 ].r , z0i = Z[ 0 ].i;
      halfPass< true >( Y , Y , N , fftHalfRoots< T >( n ) );
      std::copy( Y + 1 , Y + N , X + 1 );
      X[ 0 ] = std::complex< T >( z0r + z0i , 0 );
      X[ N ] = std::complex< T >( z0r - z0i , 0 );
      } ) )
  return;
 transformTo( reinterpret_cast< const std::complex< T > * >( x ) , X , h ,
              false );
 const T z0r = X[ 0 ].real() , z0i = X[ 0 ].imag();
 halfPass< true >( X , X , h , fftHalfRoots< T >( n ) );
 X[ 0 ] = std::complex< T >( z0r + z0i , 0 );
 X[ h ] = std::complex< T >( z0r - z0i , 0 );
 }

/// the n real numbers x of the n / 2 + 1 complex numbers X of their
/// transform, divided by n: the steps of realForward() backwards, with E_k
/// = ( X_k + conj( X_{h-k} ) ) / 2 and O_k = conj( omega_n^k ) ( X_k -
/// conj( X_{h-k} ) ) / 2, Z_k = E_k + i O_k, Z_{h-k} = conj( E_k - i O_k
/// ), and the inverse complex transform of Z, whose items are x_{2j} + i
/// x_{2j+1}; X may be where x is
template< class T >
inline void realInverse( const std::complex< T > * X , std::size_t n , T * x )
{
 if( n == 0 ) return;
 if( n == 1 ) { x[ 0 ] = X[ 0 ].real(); return; }
 if( n % 2 ) {
  std::complex< T > * z = fftScratch< T >( n , 3 );
  for( std::size_t k = 0 ; k <= n / 2 ; ++k ) z[ k ] = X[ k ];
  for( std::size_t k = n / 2 + 1 ; k < n ; ++k )
   z[ k ] = std::conj( X[ n - k ] );
  z[ 0 ] = z[ 0 ].real();
  transform< true >( z , n );
  for( std::size_t j = 0 ; j < n ; ++j ) x[ j ] = z[ j ].real();
  return;
  }
 const std::size_t h = n / 2;
 const T x0 = X[ 0 ].real() , xh = X[ h ].real();
 if( h <= 32 && byAnyCodelet< 32 >( h , [ & ]< std::size_t N >() {
      std::complex< T > Y[ N ];        // small: the pass into Y, the codelet
      halfPass< false >( X , Y , N , fftHalfRoots< T >( n ) );
      Y[ 0 ] = std::complex< T >( T( 0.5 ) * ( x0 + xh ) ,
                                  T( 0.5 ) * ( x0 - xh ) );
      const T f = T( 1 ) / T( N );            // the conjugates, forward
      auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
       return( FFTCx< T >{ Y[ i ].real() , - Y[ i ].imag() } );
       };
      auto out = [ & ]( std::size_t k , const FFTCx< T > & y ) ADS_FFT_LAMBDA {
       x[ 2 * k ] = y.r * f;
       x[ 2 * k + 1 ] = - y.i * f;
       };
      codelet< N , false , T , 1 >( in , out , codeletRoots< T >() );
      } ) )
  return;
 std::complex< T > * Z = reinterpret_cast< std::complex< T > * >( x );
 halfPass< false >( X , Z , h , fftHalfRoots< T >( n ) );
 Z[ 0 ] = std::complex< T >( T( 0.5 ) * ( x0 + xh ) ,
                            T( 0.5 ) * ( x0 - xh ) );
 transform< true >( Z , h );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- MANY TRANSFORMS, MORE DIMENSIONS -------------------*/
/*--------------------------------------------------------------------------*/

/// the transforms of length L of the c columns of x, the item i of the
/// column j at x[ 2 ( ld i + j ) ], in place and divided by L if inv: W
/// columns at a time in the lanes, by one codelet up to fftCodelet points
/// a power of 2, otherwise gathered fftStrips blocks at a time into a
/// buffer, transformed by the rounds of L and scattered back
template< class T , std::size_t W >
inline void columns( T * x , std::size_t L , std::size_t c , std::size_t ld ,
                     bool inv )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 if( L <= 1 ) return;
 const T f0 = inv ? T( 1 ) / T( L ) : T( 1 );
 const V ci = splat< T , W >( inv ? T( -1 ) : T( 1 ) ) ,
         fr = splat< T , W >( f0 ) , fi = splat< T , W >( inv ? - f0 : f0 );
 const std::size_t cb = ( c + W - 1 ) / W;
 auto load = [ & ]( std::size_t i , std::size_t t ) ADS_FFT_LAMBDA {
  const T * a = x + 2 * ( ld * i + t * W );
  C y = ( t + 1 ) * W <= c ? loadComplex< T , W >( a )
                           : loadSome< T , W >( a , c - t * W );
  y.i = y.i * ci;
  return( y );
  };
 auto store = [ & ]( std::size_t i , std::size_t t , C y ) ADS_FFT_LAMBDA {
  y.r = y.r * fr;
  y.i = y.i * fi;
  T * a = x + 2 * ( ld * i + t * W );
  if( ( t + 1 ) * W <= c ) storeComplex< T , W >( a , y );
  else storeSome< T , W >( a , y , c - t * W );
  };
 if( byAnyCodelet( L , [ & ]< std::size_t N >() {
      const T * R = codeletRoots< T >();
      for( std::size_t t = 0 ; t < cb ; ++t ) {
       auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
        return( load( i , t ) ); };
       auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
        store( k , t , y ); };
       codelet< N , false , T , W >( in , out , R );
       } } ) )
  return;
 const FFTSteps< T > & S = [ & ]() -> const FFTSteps< T > & {
  thread_local std::unordered_map< std::size_t , FFTSteps< T > > H;
  FFTSteps< T > & s = H[ L ];
  if( s.L == 0 ) s = fftSteps< T >( L );
  return( s );
  }();
 constexpr std::size_t St = fftStrips;
 C * g = fftBuffer< C >( 4 * St * L ) , * h = g + St * L , * u = h + St * L ,
   * v = u + St * L;
 for( std::size_t t0 = 0 ; t0 < cb ; t0 += St ) {
  const std::size_t s = std::min( St , cb - t0 );
  for( std::size_t i = 0 ; i < L ; ++i ) {
   if( i + fftAhead < L )
    ADS_FFT_PREFETCH( x + 2 * ( ld * ( i + fftAhead ) + t0 * W ) , 0 );
   for( std::size_t t = 0 ; t < s ; ++t ) g[ t * L + i ] = load( i , t0 + t );
   }
  for( std::size_t t = 0 ; t < s ; ++t )
   batched< T , W >( S , g + t * L , h + t * L , u , v );
  for( std::size_t i = 0 ; i < L ; ++i )
   for( std::size_t t = 0 ; t < s ; ++t ) store( i , t0 + t , h[ t * L + i ] );
  }
 }

/// the transforms of W rows of n points from x, the row l from x + 2 l n
/// on, in the lanes: gathered by blocks of W x W transposed in registers
/// into g, transformed by tr( g , h ) into h, scattered back by the
/// transposed blocks; ci for the imaginary parts of the inputs, fr and fi
/// for those of the outputs
template< class T , std::size_t W , class Tr >
ADS_FFT_INLINE void rowsGroup( T * x , std::size_t n ,
                               FFTCx< FFTLane< T , W > > * g ,
                               FFTCx< FFTLane< T , W > > * h , Tr && tr ,
                               const FFTLane< T , W > & ci ,
                               const FFTLane< T , W > & fr ,
                               const FFTLane< T , W > & fi )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 for( std::size_t i0 = 0 ; i0 < n ; i0 += W ) {
  V re[ W ] , im[ W ];
  for( std::size_t l = 0 ; l < W ; ++l ) {
   const C y = loadComplex< T , W >( x + 2 * ( l * n + i0 ) );
   re[ l ] = y.r;
   im[ l ] = y.i * ci;
   }
  transpose< T , W >( re );
  transpose< T , W >( im );
  for( std::size_t l = 0 ; l < W ; ++l )
   g[ i0 + fftLane< T , W >( l ) ] = C{ re[ l ] , im[ l ] };
  }
 tr( g , h );
 for( std::size_t k0 = 0 ; k0 < n ; k0 += W ) {
  V re[ W ] , im[ W ];
  for( std::size_t l = 0 ; l < W ; ++l ) {
   const C & y = h[ k0 + fftLane< T , W >( l ) ];
   re[ l ] = y.r * fr;
   im[ l ] = y.i * fi;
   }
  transpose< T , W >( re );
  transpose< T , W >( im );
  for( std::size_t l = 0 ; l < W ; ++l )
   storeComplex< T , W >( x + 2 * ( l * n + k0 ) , C{ re[ l ] , im[ l ] } );
  }
 }

/// the largest transforms of rows() that go W at a time in the lanes
inline constexpr std::size_t fftManyLanes = 256;

/// the transforms of the q rows of n points of a, each contiguous, the row
/// t from t n on: by smallFour() one at a time if n has a shape; W rows at
/// a time in the lanes by rowsGroup() if W divides n and n is small, by a
/// codelet on arrays on the stack or by the rounds; one at a time
/// otherwise
template< class T , std::size_t W >
inline void rows( std::complex< T > * a , std::size_t n , std::size_t q ,
                  bool inv )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 if( n <= 1 ) return;
 std::size_t t0 = 0;
 bool done = false;                         // the shape found once
 unroll< std::size( fftSmallShapes< T > ) >( [ & ]( auto I ) {
  constexpr FFTShape S = fftSmallShapes< T >[ I ];
  if constexpr( S.W <= fftWidthOf< T > && S.W > 1 )
   if( n == S.n ) {
    for( std::size_t t = 0 ; t < q ; ++t ) {
     T * x = reinterpret_cast< T * >( a + t * n );
     smallFour< S.r , S.m , T , S.W >( x , x , inv );
     }
    done = true;
    }
  } );
 if( done ) return;
 const T f0 = inv ? T( 1 ) / T( n ) : T( 1 );
 const V ci = splat< T , W >( inv ? T( -1 ) : T( 1 ) ) ,
         fr = splat< T , W >( f0 ) , fi = splat< T , W >( inv ? - f0 : f0 );
 if( W > 1 && n % W == 0 && n <= fftManyLanes && q >= W ) {
  if( ! byAnyCodelet( n , [ & ]< std::size_t N >() {
       if constexpr( N % W == 0 ) {
        C g[ N ] , h[ N ];
        auto tr = [ & ]( C * x , C * y ) ADS_FFT_LAMBDA {
         FFTArray< C > in{ x } , out{ y };
         codelet< N , false , T , W >( in , out , codeletRoots< T >() );
         };
        for( ; t0 + W <= q ; t0 += W )
         rowsGroup< T , W >( reinterpret_cast< T * >( a + t0 * n ) , n , g ,
                             h , tr , ci , fr , fi );
        } } ) ) {
   thread_local std::unordered_map< std::size_t , FFTSteps< T > > H;
   FFTSteps< T > & S = H[ n ];
   if( S.L == 0 ) S = fftSteps< T >( n );
   C * g = fftBuffer< C >( 4 * n ) , * h = g + n , * u = h + n ,
     * v = u + n;
   auto tr = [ & ]( C * x , C * y ) { batched< T , W >( S , x , y , u , v ); };
   for( ; t0 + W <= q ; t0 += W )
    rowsGroup< T , W >( reinterpret_cast< T * >( a + t0 * n ) , n , g , h ,
                        tr , ci , fr , fi );
   }
  }
 for( ; t0 < q ; ++t0 ) transformTo( a + t0 * n , a + t0 * n , n , inv );
 }

/// the transform in 2 dimensions of the n0 x n1 points of x in place, n0
/// and n1 small multiples of W, unrolled as smallFour(): the transforms of
/// the columns by codelets into an array of n0 n1 / W items, then those of
/// the rows, W at a time, gathered by blocks of W x W transposed in
/// registers and scattered back by the transposed blocks
template< std::size_t n0 , std::size_t n1 , class T , std::size_t W >
ADS_FFT_NOINLINE void small2D( T * x , bool inv )
{
 using V = FFTLane< T , W >;
 using C = FFTCx< V >;
 constexpr std::size_t cb = n1 / W;
 const T * R = codeletRoots< T >();
 const T f = inv ? T( 1 ) / T( n0 * n1 ) : T( 1 );
 const V ci = splat< T , W >( inv ? T( -1 ) : T( 1 ) ) ,
         fr = splat< T , W >( f ) , fi = splat< T , W >( inv ? - f : f );
 C B[ cb * n0 ];
 for( std::size_t b = 0 ; b < cb ; ++b ) {
  const T * a = x + 2 * b * W;
  C * o = B + b * n0;
  auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
   C y = loadComplex< T , W >( a + 2 * n1 * i );
   y.i = y.i * ci;
   return( y );
   };
  auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
   o[ k ] = y;
   };
  codelet< n0 , false , T , W >( in , out , R );
  }
 for( std::size_t k0 = 0 ; k0 < n0 ; k0 += W ) {
  C g[ n1 ] , h[ n1 ];
  for( std::size_t b = 0 ; b < cb ; ++b ) {
   V re[ W ] , im[ W ];
   for( std::size_t l = 0 ; l < W ; ++l ) {
    re[ l ] = B[ b * n0 + k0 + l ].r;
    im[ l ] = B[ b * n0 + k0 + l ].i;
    }
   transpose< T , W >( re );
   transpose< T , W >( im );
   for( std::size_t c = 0 ; c < W ; ++c )
    g[ b * W + fftLane< T , W >( c ) ] = C{ re[ c ] , im[ c ] };
   }
  FFTArray< C > in{ g } , out{ h };
  codelet< n1 , false , T , W >( in , out , R );
  for( std::size_t q0 = 0 ; q0 < n1 ; q0 += W ) {
   V re[ W ] , im[ W ];
   for( std::size_t m = 0 ; m < W ; ++m ) {
    const C & y = h[ q0 + fftLane< T , W >( m ) ];
    re[ m ] = y.r * fr;
    im[ m ] = y.i * fi;
    }
   transpose< T , W >( re );
   transpose< T , W >( im );
   for( std::size_t l = 0 ; l < W ; ++l )
    storeComplex< T , W >( x + 2 * ( n1 * ( k0 + l ) + q0 ) ,
                           C{ re[ l ] , im[ l ] } );
   }
  }
 }

/// the transform in 2 dimensions of the n0 x n1 points of x by small2D(),
/// n0 and n1 powers of 2 from the width of the lanes to 32, and true; else
/// false
template< class T >
inline bool bySmall2D( T * x , std::size_t n0 , std::size_t n1 , bool inv )
{
 constexpr std::size_t W = fftWidthOf< T >;
 bool done = false;
 unroll< 4 >( [ & ]( auto A ) {
  constexpr std::size_t N0 = std::size_t( 4 ) << A;
  if constexpr( N0 >= W )
   unroll< 4 >( [ & ]( auto B ) {
    constexpr std::size_t N1 = std::size_t( 4 ) << B;
    if constexpr( N1 >= W )
     if( n0 == N0 && n1 == N1 ) {
      small2D< N0 , N1 , T , W >( x , inv );
      done = true;
      }
    } );
  } );
 return( done );
 }

/// the transform along the axes of the array a of the D extents d, the last
/// one contiguous: the rows of the last axis by rows(), every other axis by
/// columns() on the blocks of the axes before it
template< class T >
inline void transformND( std::complex< T > * a , const std::size_t * d ,
                         std::size_t D , bool inv )
{
 constexpr std::size_t W = fftWidthOf< T >;
 std::size_t N = 1;
 for( std::size_t k = 0 ; k < D ; ++k ) N *= d[ k ];
 if( N == 0 ) return;
 std::size_t inner = 1;
 for( std::size_t k = D ; k-- > 0 ; ) {
  const std::size_t L = d[ k ] , outer = N / ( L * inner );
  if( inner == 1 ) rows< T , W >( a , L , outer , inv );
  else
   for( std::size_t o = 0 ; o < outer ; ++o )
    columns< T , W >( reinterpret_cast< T * >( a + o * L * inner ) , L ,
                      inner , inner , inv );
  inner *= L;
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------- THE COSINES AND THE SINES ------------------------*/
/*--------------------------------------------------------------------------*/

/// a buffer of n numbers of type T for the trigonometric transforms, kept
/// per thread
template< class T >
inline T * trigBuffer( std::size_t n )
{
 thread_local std::vector< T > B;
 if( B.size() < n ) B.resize( n );
 return( B.data() );
 }

/// the table of the powers f( k ) , k < m, of the trigonometric
/// transforms of kind c on N points, kept per thread
template< class T , class F >
inline const std::complex< T > * trigTable( unsigned c , std::size_t N ,
                                            std::size_t m , F && f )
{
 thread_local std::unordered_map< std::size_t ,
                                  std::vector< std::complex< T > > > H;
 std::vector< std::complex< T > > & t = H[ 8 * N + c ];
 if( t.empty() ) {
  t.resize( m );
  for( std::size_t k = 0 ; k < m ; ++k ) t[ k ] = f( k );
  }
 return( t.data() );
 }

/// the transform of type II of the cosines in place, y_k = 2 sum_j x_j
/// cos( pi ( 2 j + 1 ) k / 2 N ), by the algorithm of Makhoul: v_j =
/// x_{2j}, v_{N-1-j} = x_{2j+1}, V its real transform, y_k = 2 Re( w_k V_k
/// ) and y_{N-k} = -2 Im( w_k V_k ), w_k = omega_{4N}^k
template< class T >
inline void dct2( T * x , std::size_t N )
{
 if( N == 0 ) return;
 T * v = trigBuffer< T >( N + 2 );
 for( std::size_t j = 0 ; 2 * j < N ; ++j ) v[ j ] = x[ 2 * j ];
 for( std::size_t j = 0 ; 2 * j + 1 < N ; ++j )
  v[ N - 1 - j ] = x[ 2 * j + 1 ];
 std::complex< T > * V = reinterpret_cast< std::complex< T > * >( v );
 realForward( v , N , V );
 const std::complex< T > * w = trigTable< T >( 0 , N , N / 2 + 1 ,
  [ & ]( std::size_t k ) { return( omega< T >( k , 4 * N ) ); } );
 for( std::size_t k = 0 ; 2 * k <= N ; ++k ) {
  const std::complex< T > z = times( w[ k ] , V[ k ] );
  x[ k ] = 2 * z.real();
  if( k ) x[ N - k ] = -2 * z.imag();
  }
 }

/// the transform of type III of the cosines in place, y_k = x_0 + 2 sum_{j
/// >= 1} x_j cos( pi j ( 2 k + 1 ) / 2 N ), the transpose of dct2(): c_k =
/// ( x_k - i x_{N-k} ) conj( w_k ), k <= N / 2, the real v of the
/// Hermitian c by realInverse() times N, y_{2j} = v_j , y_{2j+1} =
/// v_{N-1-j}
template< class T >
inline void dct3( T * x , std::size_t N )
{
 if( N == 0 ) return;
 T * v = trigBuffer< T >( N + 2 );
 std::complex< T > * c = reinterpret_cast< std::complex< T > * >( v );
 const std::complex< T > * w = trigTable< T >( 0 , N , N / 2 + 1 ,
  [ & ]( std::size_t k ) { return( omega< T >( k , 4 * N ) ); } );
 for( std::size_t k = 0 ; 2 * k <= N ; ++k )
  c[ k ] = times( std::conj( w[ k ] ) ,
                  std::complex< T >( x[ k ] , k ? - x[ N - k ] : T( 0 ) ) );
 realInverse( c , N , v );
 for( std::size_t j = 0 ; 2 * j < N ; ++j ) x[ 2 * j ] = v[ j ] * T( N );
 for( std::size_t j = 0 ; 2 * j + 1 < N ; ++j )
  x[ 2 * j + 1 ] = v[ N - 1 - j ] * T( N );
 }

/// the transform of type IV of the cosines in place, y_k = 2 sum_j x_j cos(
/// pi ( 2 j + 1 ) ( 2 k + 1 ) / 4 N ): for N even, the complex transform
/// of the N / 2 points z_j = ( x_{2j} + i x_{N-1-2j} ) omega_{4N}^{j + 1 /
/// 4}, then u_k = Z_k omega_{2N}^{k}... as below, y_{2k} = 2 Re( u_k ) and
/// y_{N-1-2k} = -2 Im( u_k ); for N odd, the complex transform of 2 N
/// points of x_j omega_{4N}^j
template< class T >
inline void dct4( T * x , std::size_t N )
{
 if( N == 0 ) return;
 if( N % 2 ) {
  std::complex< T > * z = fftScratch< T >( 2 * N , 3 );
  const std::complex< T > * a = trigTable< T >( 3 , N , N ,
   [ & ]( std::size_t j ) { return( omega< T >( j , 4 * N ) ); } );
  const std::complex< T > * b = trigTable< T >( 4 , N , N ,
   [ & ]( std::size_t k ) { return( omega< T >( 2 * k + 1 , 8 * N ) ); } );
  for( std::size_t j = 0 ; j < N ; ++j ) z[ j ] = x[ j ] * a[ j ];
  std::fill( z + N , z + 2 * N , std::complex< T >( 0 ) );
  transform< false >( z , 2 * N );
  for( std::size_t k = 0 ; k < N ; ++k )
   x[ k ] = 2 * times( z[ k ] , b[ k ] ).real();
  return;
  }
 const std::size_t M = N / 2;
 std::complex< T > * z = fftScratch< T >( M , 3 );
 const std::complex< T > * a = trigTable< T >( 1 , N , M ,
  [ & ]( std::size_t j ) { return( omega< T >( 4 * j + 1 , 8 * N ) ); } );
 const std::complex< T > * b = trigTable< T >( 2 , N , M ,
  [ & ]( std::size_t k ) { return( omega< T >( k , 2 * N ) ); } );
 for( std::size_t j = 0 ; j < M ; ++j )
  z[ j ] = times( std::complex< T >( x[ 2 * j ] , x[ N - 1 - 2 * j ] ) ,
                  a[ j ] );
 transform< false >( z , M );
 for( std::size_t k = 0 ; k < M ; ++k ) {
  const std::complex< T > u = times( z[ k ] , b[ k ] );
  x[ 2 * k ] = 2 * u.real();
  x[ N - 1 - 2 * k ] = -2 * u.imag();
  }
 }

/// the transform of type I of the cosines in place, y_k = x_0 + ( -1 )^k
/// x_{N-1} + 2 sum_{0 < j < N-1} x_j cos( pi j k / ( N - 1 ) ), N >= 2:
/// the real transform of the even extension of 2 ( N - 1 ) points
template< class T >
inline void dct1( T * x , std::size_t N )
{
 const std::size_t L = 2 * ( N - 1 );
 T * e = trigBuffer< T >( L + 2 );
 for( std::size_t j = 0 ; j < N ; ++j ) e[ j ] = x[ j ];
 for( std::size_t j = 1 ; j + 1 < N ; ++j ) e[ L - j ] = x[ j ];
 std::complex< T > * E = reinterpret_cast< std::complex< T > * >( e );
 realForward( e , L , E );
 for( std::size_t k = 0 ; k < N ; ++k ) x[ k ] = E[ k ].real();
 }

/// the transform of type I of the sines in place, y_k = 2 sum_j x_j sin( pi
/// ( j + 1 ) ( k + 1 ) / ( N + 1 ) ): the real transform of the odd
/// extension of 2 ( N + 1 ) points, y_k = -Im( E_{k+1} )
template< class T >
inline void dst1( T * x , std::size_t N )
{
 const std::size_t L = 2 * ( N + 1 );
 T * e = trigBuffer< T >( L + 2 );
 e[ 0 ] = e[ N + 1 ] = 0;
 for( std::size_t j = 0 ; j < N ; ++j ) {
  e[ j + 1 ] = x[ j ];
  e[ L - 1 - j ] = - x[ j ];
  }
 std::complex< T > * E = reinterpret_cast< std::complex< T > * >( e );
 realForward( e , L , E );
 for( std::size_t k = 0 ; k < N ; ++k ) x[ k ] = - E[ k + 1 ].imag();
 }

/// the size of the real transforms of a convolution of m >= 2 outputs: the
/// smallest even 2^a 3^b 5^c >= m if it is below 3 / 4 of the power of 2,
/// whose transforms are faster, that power otherwise
inline std::size_t fftConvSize( std::size_t m )
{
 const std::size_t p = std::max< std::size_t >( 2 , std::bit_ceil( m ) );
 std::size_t g = fftGoodSize( m );
 while( g % 2 ) g = fftGoodSize( g + 1 );
 return( 4 * g < 3 * p ? g : p );
 }

/// the coefficients of the product of the polynomials p and q, by the
/// real transforms of fftConvSize( | p | + | q | - 1 ) points, the product
/// of the transforms item by item and the inverse real transform
template< class T >
inline std::vector< T > convolveReal( const std::vector< T > & p ,
                                      const std::vector< T > & q )
{
 if( p.empty() || q.empty() ) return( std::vector< T >() );
 const std::size_t m = p.size() + q.size() - 1;
 if( m == 1 ) return( std::vector< T >( 1 , p[ 0 ] * q[ 0 ] ) );
 const std::size_t n = fftConvSize( m );
 const std::size_t h = n / 2;
 std::vector< T > a( n + 2 );
 T * b = trigBuffer< T >( n + 2 );
 std::fill( std::copy( p.begin() , p.end() , a.begin() ) , a.end() , T( 0 ) );
 std::fill( std::copy( q.begin() , q.end() , b ) , b + n + 2 , T( 0 ) );
 auto * A = reinterpret_cast< std::complex< T > * >( a.data() );
 auto * B = reinterpret_cast< std::complex< T > * >( b );
 realForward( a.data() , n , A );
 realForward( b , n , B );
 for( std::size_t k = 0 ; k <= h ; ++k ) A[ k ] = times( A[ k ] , B[ k ] );
 realInverse( A , n , a.data() );
 a.resize( m );
 return( a );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- THE PUBLIC FUNCTIONS --------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< Complex > dft( const std::vector< Complex > & a ,
                                   bool inverse )
{
 const std::size_t n = a.size();
 const double sgn = inverse ? 1 : -1;
 std::vector< Complex > out( n );
 for( std::size_t k = 0 ; k < n ; ++k ) {
  for( std::size_t j = 0 ; j < n ; ++j )   // the angle reduced mod n
   out[ k ] += a[ j ] * std::polar( 1.0 , sgn * 2 * std::numbers::pi *
                                    double( j * k % n ) / double( n ) );
  if( inverse ) out[ k ] /= double( n );
  }
 return( out );
 }

template< class T >
inline void fft( std::complex< T > * a , std::size_t n , bool inverse )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( n == 0 || a != nullptr );
 detail::transformTo( a , a , n , inverse );
 }

template< class T >
inline void fft( std::vector< std::complex< T > > & a , bool inverse )
{
 fft( a.data() , a.size() , inverse );
 }

template< class T >
inline void fftMany( std::complex< T > * a , std::size_t n ,
                     std::size_t howmany , bool inverse )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( n * howmany == 0 || a != nullptr );
 constexpr std::size_t W = detail::fftWidthOf< T >;
 detail::rows< T , W >( a , n , howmany , inverse );
 }

template< class T >
inline void fftn( std::complex< T > * a ,
                  const std::vector< std::size_t > & dims , bool inverse )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 detail::transformND( a , dims.data() , dims.size() , inverse );
 }

template< class T >
inline void fft2( std::complex< T > * a , std::size_t rows ,
                  std::size_t cols , bool inverse )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 if( detail::bySmall2D( reinterpret_cast< T * >( a ) , rows , cols ,
                        inverse ) )
  return;
 const std::size_t d[ 2 ] = { rows , cols };
 detail::transformND( a , d , 2 , inverse );
 }

template< class T >
inline void rfft( const T * x , std::size_t n , std::complex< T > * X )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( n == 0 || ( x != nullptr && X != nullptr ) );
 detail::realForward( x , n , X );
 }

template< class T >
inline std::vector< std::complex< T > > rfft( const std::vector< T > & x )
{
 if( x.empty() ) return( std::vector< std::complex< T > >() );
 std::vector< std::complex< T > > X( x.size() / 2 + 1 );
 rfft( x.data() , x.size() , X.data() );
 return( X );
 }

template< class T >
inline void irfft( const std::complex< T > * X , std::size_t n , T * x )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( n == 0 || ( x != nullptr && X != nullptr ) );
 detail::realInverse( X , n , x );
 }

template< class T >
inline std::vector< T > irfft( const std::vector< std::complex< T > > & X ,
                               std::size_t n )
{
 ADS_PRE( X.size() == ( n ? n / 2 + 1 : 0 ) );
 std::vector< T > x( n );
 irfft( X.data() , n , x.data() );
 return( x );
 }

template< class T >
inline void dct( T * x , std::size_t n , unsigned type )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( type >= 1 && type <= 4 );
 ADS_PRE( type != 1 || n != 1 );
 switch( type ) {
  case( 1 ): if( n ) detail::dct1( x , n ); break;
  case( 2 ): detail::dct2( x , n ); break;
  case( 3 ): detail::dct3( x , n ); break;
  default: detail::dct4( x , n );
  }
 }

template< class T >
inline void dct( std::vector< T > & x , unsigned type )
{
 dct( x.data() , x.size() , type );
 }

template< class T >
inline void dst( T * x , std::size_t n , unsigned type )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 ADS_PRE( type >= 1 && type <= 4 );
 switch( type ) {
  case( 1 ): detail::dst1( x , n ); break;
  case( 2 ):                       // dct2 of ( -1 )^j x_j , reversed
   for( std::size_t j = 1 ; j < n ; j += 2 ) x[ j ] = - x[ j ];
   detail::dct2( x , n );
   std::reverse( x , x + n );
   break;
  case( 3 ):                       // ( -1 )^k dct3 of x reversed
   std::reverse( x , x + n );
   detail::dct3( x , n );
   for( std::size_t k = 1 ; k < n ; k += 2 ) x[ k ] = - x[ k ];
   break;
  default:                         // ( -1 )^k dct4 of x reversed
   std::reverse( x , x + n );
   detail::dct4( x , n );
   for( std::size_t k = 1 ; k < n ; k += 2 ) x[ k ] = - x[ k ];
  }
 }

template< class T >
inline void dst( std::vector< T > & x , unsigned type )
{
 dst( x.data() , x.size() , type );
 }

inline std::vector< double > convolve( const std::vector< double > & p ,
                                       const std::vector< double > & q )
{
 return( detail::convolveReal( p , q ) );
 }

template< class T >
inline std::vector< T > convolve( const std::vector< T > & p ,
                                  const std::vector< T > & q )
{
 static_assert( std::is_same_v< T , double > || std::is_same_v< T , float > );
 return( detail::convolveReal( p , q ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#undef ADS_FFT_INLINE
#undef ADS_FFT_NOINLINE
#undef ADS_FFT_LAMBDA
#undef ADS_FFT_UNROLL
#undef ADS_FFT_PREFETCH
#undef ADS_FFT_SHUFFLE

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FFT.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
