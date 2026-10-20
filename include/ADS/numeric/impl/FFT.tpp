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
#else
 #define ADS_FFT_PREFETCH( p , w ) ( ( void ) 0 )
#endif

/// a lambda inlined where it is called, as ADS_FFT_INLINE
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_FFT_LAMBDA __attribute__(( always_inline ))
#else
 #define ADS_FFT_LAMBDA
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

/// the shuffles of the lanes by __builtin_shufflevector where the compiler
/// has it, lane by lane elsewhere
#if ( ADS_FFT_SIMD > 1 ) && \
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

/// W doubles as an array, with the operations item by item: the lanes
/// where the compiler has no vector extensions (or ADS_FFT_SIMD is 0); the
/// operations are unrolled, so that the compiler keeps the items apart
template< std::size_t W >
struct FFTLanes {
 double v[ W ];

 double & operator[]( std::size_t l ) { return( v[ l ] ); }

 double operator[]( std::size_t l ) const { return( v[ l ] ); }

 template< class F , std::size_t ... L >
 static ADS_FFT_INLINE FFTLanes map( F f , std::index_sequence< L ... > ) {
  return( FFTLanes{ { f( L ) ... } } );
  }

 friend ADS_FFT_INLINE FFTLanes operator+( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  return( map( [ & ]( std::size_t l ) { return( a.v[ l ] + b.v[ l ] ); } ,
               std::make_index_sequence< W >() ) );
  }

 friend ADS_FFT_INLINE FFTLanes operator-( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  return( map( [ & ]( std::size_t l ) { return( a.v[ l ] - b.v[ l ] ); } ,
               std::make_index_sequence< W >() ) );
  }

 friend ADS_FFT_INLINE FFTLanes operator*( const FFTLanes & a ,
                                           const FFTLanes & b ) {
  return( map( [ & ]( std::size_t l ) { return( a.v[ l ] * b.v[ l ] ); } ,
               std::make_index_sequence< W >() ) );
  }

 friend ADS_FFT_INLINE FFTLanes operator-( const FFTLanes & a ) {
  return( map( [ & ]( std::size_t l ) { return( - a.v[ l ] ); } ,
               std::make_index_sequence< W >() ) );
  }
 };

/// the lanes of width W: a double if W is 1, a vector of the extensions of
/// GCC and Clang if ADS_FFT_SIMD > 1, FFTLanes otherwise
template< std::size_t W >
struct FFTLaneType {
 using type = FFTLanes< W >;
 };

template<> struct FFTLaneType< 1 > {
 using type = double;
 };

#if ADS_FFT_SIMD > 1
template<> struct FFTLaneType< 2 > {
 typedef double type __attribute__(( vector_size( 16 ) ));
 };

template<> struct FFTLaneType< 4 > {
 typedef double type __attribute__(( vector_size( 32 ) ));
 };

template<> struct FFTLaneType< 8 > {
 typedef double type __attribute__(( vector_size( 64 ) ));
 };
#endif

/// the widest lanes: ADS_FFT_SIMD doubles, or 4 of FFTLanes
inline constexpr std::size_t fftWidth = ADS_FFT_SIMD > 1 ? ADS_FFT_SIMD : 4;

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
template< std::size_t W >
ADS_FFT_INLINE typename FFTLaneType< W >::type splat( double x )
{
 typename FFTLaneType< W >::type v{};
 if constexpr( W == 1 ) v = x;
 else
  for( std::size_t l = 0 ; l < W ; ++l ) v[ l ] = x;
 return( v );
 }

/// the W lanes at p, which need not be aligned
template< std::size_t W >
ADS_FFT_INLINE typename FFTLaneType< W >::type loadLanes( const double * p )
{
 typename FFTLaneType< W >::type v;
 std::memcpy( & v , p , sizeof( v ) );
 return( v );
 }

template< class V >
ADS_FFT_INLINE void storeLanes( double * p , const V & v )
{
 std::memcpy( p , & v , sizeof( v ) );
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
/// storeComplex(): the even lanes hold the first half, the odd ones the
/// second, as the unpack instructions of SSE2, AVX and AVX-512 leave them
template< std::size_t W >
constexpr std::size_t fftLane( std::size_t l )
{
 return( l % 2 ? W / 2 + l / 2 : l / 2 );
 }

#if ADS_FFT_SHUFFLE
/// the lanes of the shuffles of two vectors a, b of W lanes, those of b
/// numbered from W: the lane l of a if l is even, l - 1 of b otherwise
template< std::size_t W >
constexpr int fftUnpack( std::size_t l )
{
 return( int( l % 2 ? W + l - 1 : l ) );
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

template< std::size_t W , class V , std::size_t ... L >
ADS_FFT_INLINE FFTCx< V > deinterleave( const V & u , const V & v ,
                                        std::index_sequence< L ... > )
{
 return( FFTCx< V >{
          __builtin_shufflevector( u , v , fftUnpack< W >( L ) ... ) ,
          __builtin_shufflevector( u , v , fftUnpack< W >( L ) + 1 ... ) } );
 }

template< std::size_t W , class V , std::size_t ... L >
ADS_FFT_INLINE void interleave( const FFTCx< V > & x , V & u , V & v ,
                                std::index_sequence< L ... > )
{
 u = __builtin_shufflevector( x.r , x.i , fftUnpack< W >( L ) ... );
 v = __builtin_shufflevector( x.r , x.i , fftUnpack< W >( L ) + 1 ... );
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

/// the W complex numbers at p, real and imaginary parts alternated, as
/// one item of W lanes, the number fftLane( l ) in the lane l
template< std::size_t W >
ADS_FFT_INLINE FFTCx< typename FFTLaneType< W >::type > loadComplex(
                                                         const double * p )
{
 using V = typename FFTLaneType< W >::type;
 if constexpr( W == 1 ) return( FFTCx< V >{ p[ 0 ] , p[ 1 ] } );
 else {
#if ADS_FFT_SHUFFLE
  return( deinterleave< W >( loadLanes< W >( p ) , loadLanes< W >( p + W ) ,
                             std::make_index_sequence< W >() ) );
#else
  FFTCx< V > x;
  for( std::size_t l = 0 ; l < W ; ++l ) {
   x.r[ l ] = p[ 2 * fftLane< W >( l ) ];
   x.i[ l ] = p[ 2 * fftLane< W >( l ) + 1 ];
   }
  return( x );
#endif
  }
 }

/// the item x of W lanes stored as W complex numbers at p, the lane l as
/// the number fftLane( l )
template< std::size_t W , class V >
ADS_FFT_INLINE void storeComplex( double * p , const FFTCx< V > & x )
{
 if constexpr( W == 1 ) {
  p[ 0 ] = x.r;
  p[ 1 ] = x.i;
  }
 else {
#if ADS_FFT_SHUFFLE
  V u , v;
  interleave< W >( x , u , v , std::make_index_sequence< W >() );
  storeLanes( p , u );
  storeLanes( p + W , v );
#else
  for( std::size_t l = 0 ; l < W ; ++l ) {
   p[ 2 * fftLane< W >( l ) ] = x.r[ l ];
   p[ 2 * fftLane< W >( l ) + 1 ] = x.i[ l ];
   }
#endif
  }
 }

/// the W x W matrix of the lanes of v[ 0 ] , ... , v[ W - 1 ] transposed:
/// the blocks off the diagonal swapped, then those within each block
template< std::size_t W , class V >
ADS_FFT_INLINE void transpose( V * v )
{
 if constexpr( W > 1 ) {
#if ADS_FFT_SHUFFLE
  transposeStage< W , W / 2 >( v );
#else
  for( std::size_t a = 0 ; a < W ; ++a )
   for( std::size_t b = a + 1 ; b < W ; ++b ) {
    const double t = v[ a ][ b ];
    v[ a ][ b ] = v[ b ][ a ];
    v[ b ][ a ] = t;
    }
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CODELETS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cosine c and the sine s of 2 pi k / L, 0 <= k < L, L a power of 2:
/// the angle is taken to [ 0 , pi / 4 ] by the symmetries of the circle,
/// so that each value has the error of one sine or cosine of a small angle
inline void unitRoot( std::size_t k , std::size_t L , double & c ,
                      double & s )
{
 bool negS = false , negC = false , swap = false;
 if( 2 * k > L ) { k = L - k; negS = true; }        // 2 pi - x
 if( 4 * k > L ) { k = L / 2 - k; negC = true; }    // pi - x
 if( 8 * k > L ) { k = L / 4 - k; swap = true; }    // pi / 2 - x
 const double x = 2 * std::numbers::pi * double( k ) / double( L );
 c = std::cos( x );
 s = std::sin( x );
 if( swap ) std::swap( c , s );
 if( negC ) c = -c;
 if( negS ) s = -s;
 }

/// the largest codelet, and the table of its roots: omega_64^e = c + i s
/// at 2 e , 2 e + 1, omega_64 = e^{-2 pi i / 64}
inline constexpr std::size_t fftCodelet = 64;

inline const double * codeletRoots()
{
 static const std::vector< double > R = [] {
  std::vector< double > t( 2 * fftCodelet );
  for( std::size_t e = 0 ; e < fftCodelet ; ++e ) {
   unitRoot( e , fftCodelet , t[ 2 * e ] , t[ 2 * e + 1 ] );
   t[ 2 * e + 1 ] = - t[ 2 * e + 1 ];
   }
  return( t );
  }();
 return( R.data() );
 }

/// x times omega_N^e, e < N fixed: by nothing, -i and -1 without products
template< std::size_t N , std::size_t e , bool inv , std::size_t W , class V >
ADS_FFT_INLINE FFTCx< V > twiddle( const FFTCx< V > & x , const double * R )
{
 if constexpr( e == 0 ) return( x );
 else if constexpr( 4 * e == N ) return( mulMinusI< inv >( x ) );
 else if constexpr( 2 * e == N ) return( FFTCx< V >{ - x.r , - x.i } );
 else {
  constexpr std::size_t k = e * ( fftCodelet / N );
  return( cmul< inv >( x , splat< W >( R[ 2 * k ] ) ,
                       splat< W >( R[ 2 * k + 1 ] ) ) );
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

/// the items of an array as the inputs and the outputs of a codelet
template< class C >
struct FFTArray {
 C * x;

 ADS_FFT_INLINE C operator()( std::size_t j ) const { return( x[ j ] ); }

 ADS_FFT_INLINE void operator()( std::size_t j , const C & y ) const {
  x[ j ] = y;
  }
 };

template< std::size_t N , bool inv , std::size_t W , class C , class In ,
          class Out >
ADS_FFT_INLINE void codelet( In & in , Out & out , const double * R );

/// the radix-4 step of a codelet of N items: the outputs k of the
/// transforms s of its 4 quarters, times omega_N^{t k}, give the outputs
/// k + u N / 4
template< std::size_t N , std::size_t k , bool inv , std::size_t W ,
          class C , class Out >
ADS_FFT_INLINE void combine4( Out & out , const C * s , const double * R )
{
 constexpr std::size_t M = N / 4;
 if constexpr( k < M ) {
  C a = s[ k ] , b = twiddle< N , k , inv , W >( s[ M + k ] , R ) ,
    c = twiddle< N , 2 * k , inv , W >( s[ 2 * M + k ] , R ) ,
    d = twiddle< N , 3 * k , inv , W >( s[ 3 * M + k ] , R );
  dft4< inv >( a , b , c , d );
  out( k , a );
  out( M + k , b );
  out( 2 * M + k , c );
  out( 3 * M + k , d );
  }
 }

template< std::size_t N , bool inv , std::size_t W , class C , class In ,
          class Out , std::size_t ... J >
ADS_FFT_INLINE void codelet4( In & in , Out & out , const double * R ,
                              std::index_sequence< J ... > )
{
 constexpr std::size_t M = N / 4;
 C s[ N ];                                 // the quarter t from t M on
 FFTStride< 4 , 0 , In > i0{ in };
 FFTStride< 4 , 1 , In > i1{ in };
 FFTStride< 4 , 2 , In > i2{ in };
 FFTStride< 4 , 3 , In > i3{ in };
 FFTArray< C > o0{ s } , o1{ s + M } , o2{ s + 2 * M } , o3{ s + 3 * M };
 codelet< M , inv , W , C >( i0 , o0 , R );
 codelet< M , inv , W , C >( i1 , o1 , R );
 codelet< M , inv , W , C >( i2 , o2 , R );
 codelet< M , inv , W , C >( i3 , o3 , R );
 ( combine4< N , J , inv , W >( out , s , R ) , ... );
 }

/// the transform of N items, N <= fftCodelet a power of 2, unrolled: the
/// input i is in( i ), the output k goes to out( k , y ), which may write
/// where the inputs were, all of them read by then; by decimation in time
/// in radix 4, and 2 at the bottom
template< std::size_t N , bool inv , std::size_t W , class C , class In ,
          class Out >
ADS_FFT_INLINE void codelet( In & in , Out & out , const double * R )
{
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
 else
  codelet4< N , inv , W , C >( in , out , R ,
                               std::make_index_sequence< N / 4 >() );
 }

/// f.template operator()< L >() if L <= Max <= fftCodelet, and true;
/// else false
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

/*--------------------------------------------------------------------------*/
/*--------------------------- THE BATCHED DFT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the rounds of the transforms of 2^K > fftCodelet items, by Stockham:
/// the round t in radix R_t = 2^b[ t ] <= fftRadix on the length L_t,
/// its powers of omega_{L_t} from at[ t ] in w, those of p < L_t / R_t ,
/// 0 < j < R_t at 2 ( ( R_t - 1 ) p + j - 1 )
struct FFTSteps {
 std::vector< unsigned > b;
 std::vector< std::size_t > at;
 std::vector< double > w;
 };

/// the largest radix of the rounds of batched(): the codelets of 64 items
/// spill more of them out of the registers than they save in the rounds
inline constexpr std::size_t fftRadix = 32;

/// the rounds of the transform of 2^K items, kept per thread: as few as
/// the radix fftRadix allows, of radices as equal as can be, the larger
/// first
inline const FFTSteps & fftSteps( unsigned K )
{
 thread_local std::vector< FFTSteps > S( 64 );
 FFTSteps & s = S[ K ];
 if( s.b.empty() ) {
  constexpr unsigned C = std::countr_zero( fftRadix );
  const unsigned P = ( K + C - 1 ) / C;
  for( unsigned t = 0 ; t < P ; ++t )
   s.b.push_back( K / P + ( t < K % P ) );
  std::size_t Lt = std::size_t( 1 ) << K;
  for( unsigned t = 0 ; t < P ; ++t ) {
   const std::size_t R = std::size_t( 1 ) << s.b[ t ] , m = Lt / R;
   s.at.push_back( s.w.size() );
   for( std::size_t p = 0 ; p < m ; ++p )
    for( std::size_t j = 1 ; j < R ; ++j ) {
     double c , si;
     unitRoot( p * j , Lt , c , si );
     s.w.push_back( c );
     s.w.push_back( - si );
     }
   Lt = m;
   }
  }
 return( s );
 }

/// a round of Stockham in radix R by decimation in frequency on the
/// length m R: the transform of the R items x( q + s ( p + i m ) ) , i <
/// R, times omega^{p j}, goes to y( q + s ( R p + j ) , . )
template< std::size_t R , bool inv , std::size_t W , class C , class X ,
          class Y >
inline void stockham( X & x , Y & y , std::size_t m , std::size_t s ,
                      const double * w , const double * Rt )
{
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
    codelet< R , inv , W , C >( in , out , Rt );
    }
   else {
    auto out = [ & ]( std::size_t j , const C & z ) ADS_FFT_LAMBDA {
     y( o + s * j , j == 0 ? z
                           : cmul< inv >( z , splat< W >( w[ 2 * j - 2 ] ) ,
                                          splat< W >( w[ 2 * j - 1 ] ) ) );
     };
    codelet< R , inv , W , C >( in , out , Rt );
    }
   }
 }

/// the transform of the 2^K > fftCodelet items x into y by the rounds of
/// fftSteps(), u and v of 2^K items for the rounds in between
template< bool inv , std::size_t W , class C >
inline void batched( unsigned K , const C * x , C * y , C * u , C * v )
{
 const FFTSteps & S = fftSteps( K );
 const double * Rt = codeletRoots();
 const std::size_t P = S.b.size();
 std::size_t s = 1 , Lt = std::size_t( 1 ) << K;
 for( std::size_t t = 0 ; t < P ; ++t ) {
  const std::size_t m = Lt >> S.b[ t ];
  const double * w = S.w.data() + S.at[ t ];
  FFTArray< const C > a{ x };
  FFTArray< C > b{ u } , c{ v } , d{ y };
  byCodelet< fftRadix >( std::size_t( 1 ) << S.b[ t ] ,
                        [ & ]< std::size_t R >() {
   if( t == 0 ) stockham< R , inv , W , C >( a , c , m , s , w , Rt );
   else if( t + 1 < P )
    stockham< R , inv , W , C >( b , c , m , s , w , Rt );
   else stockham< R , inv , W , C >( b , d , m , s , w , Rt );
   } );
  s <<= S.b[ t ];
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
inline constexpr unsigned fftSplitRoots = 16;

/// the rows ahead of those read, or written, at a stride by batched()
/// whose lines are brought into the cache
inline constexpr std::size_t fftAhead = 16;

/// the shift of the blocks of the outputs of the four steps on x: the
/// numbers from x to the first one that starts an item of W numbers within
/// a line of the cache, or 0 if the numbers are not aligned to 16 bytes;
/// the blocks start there, so that the store of an item writes as few
/// lines as it can
template< std::size_t W >
inline std::size_t fftShift( const double * x )
{
 constexpr std::size_t A = std::min< std::size_t >( 64 , 16 * W );
 const std::size_t a = std::size_t( reinterpret_cast< std::uintptr_t >( x ) );
 if( a % 16 ) return( 0 );
 return( ( A - a % A ) % A / 16 % W );
 }

/// the powers of omega_n of the four steps on n = 2^K = r m points with
/// the shift o, kept per thread: omega_n^{j k( u , l )}, j < r, u < m / W,
/// l < W, k( u , l ) = ( o + u W + fftLane( l ) ) mod m, as the item u r
/// + j of t, one lane per l; from 2^fftSplitRoots on, as the product of
/// omega_n^{j ( o + u W )}, at 2 ( u r + j ) in s, and of the item j of
/// t, omega_n^{j fftLane( l )} in the lane l, except for the block u that
/// wraps around m, whose powers are the items of w
template< class V >
struct FFTFourRoots {
 std::vector< FFTCx< V > > t , w;
 std::vector< double > s;
 };

template< std::size_t W , class V >
inline const FFTFourRoots< V > & fourStepRoots( unsigned K , std::size_t r ,
                                                std::size_t m ,
                                                std::size_t o )
{
 thread_local std::vector< FFTFourRoots< V > > T( 64 * 8 );
 FFTFourRoots< V > & R = T[ 8 * K + o ];
 if( R.t.empty() ) {
  const std::size_t n = r * m;
  const bool split = K >= fftSplitRoots;
  auto set = [ & ]( FFTCx< V > & x , std::size_t l , std::size_t e ) {
   double c , s;
   unitRoot( e % n , n , c , s );
   if constexpr( W == 1 ) { x.r = c; x.i = -s; }
   else { x.r[ l ] = c; x.i[ l ] = -s; }
   };
  auto k = [ & ]( std::size_t u , std::size_t l ) {
   return( ( o + u * W + fftLane< W >( l ) ) % m );
   };
  R.t.resize( split ? r : n / W );
  for( std::size_t u = 0 ; u < ( split ? 1 : m / W ) ; ++u )
   for( std::size_t j = 0 ; j < r ; ++j )
    for( std::size_t l = 0 ; l < W ; ++l )
     set( R.t[ u * r + j ] , l ,
          j * ( split ? fftLane< W >( l ) : k( u , l ) ) );
  if( split ) {
   R.s.resize( 2 * n / W );
   for( std::size_t u = 0 ; u < m / W ; ++u )
    for( std::size_t j = 0 ; j < r ; ++j ) {
     double * y = R.s.data() + 2 * ( u * r + j );
     unitRoot( j * ( o + u * W ) % n , n , y[ 0 ] , y[ 1 ] );
     y[ 1 ] = - y[ 1 ];
     }
   R.w.resize( r );
   for( std::size_t j = 0 ; j < r ; ++j )
    for( std::size_t l = 0 ; l < W ; ++l )
     set( R.w[ j ] , l , j * k( m / W - 1 , l ) );
   }
  }
 return( R );
 }

/// a buffer of at least n items, kept per thread
template< class V >
inline FFTCx< V > * fourStepBuffer( std::size_t n )
{
 thread_local std::vector< FFTCx< V > > B;
 if( B.size() < n ) B.resize( n );
 return( B.data() );
 }

/// the matrix of the four steps on n = r m points of x, of m rows of r
/// columns, with the shift o: its blocks of W columns start at W t, those
/// of W numbers of the rows of the output at o + W u, the last one
/// wrapping around the row if o > 0; B holds the transforms of the
/// columns, the block t from t M on, M = m + W so that the blocks do not
/// start at a stride of a power of 2
template< std::size_t W , class V >
struct FFTFour {
 using C = FFTCx< V >;

 double * x;
 std::size_t r , m , o , M;
 C * B;
 const FFTFourRoots< V > * T;
 bool split;

 /// whether the block u of the rows of the output wraps around
 bool wraps( std::size_t u ) const {
  return( o > 0 && u + 1 == m / W );
  }

 /// the item y as the W numbers from k on, mod m, of the row i of the
 /// output
 ADS_FFT_INLINE void output( std::size_t i , std::size_t k ,
                             const C & y ) const {
  double * a = x + 2 * m * i;
  if( k + W <= m ) {
   storeComplex< W >( a + 2 * k , y );
   return;
   }
  double z[ 2 * W ];
  storeComplex< W >( z , y );
  for( std::size_t q = 0 ; q < W ; ++q ) {
   const std::size_t h = k + q < m ? k + q : k + q - m;
   a[ 2 * h ] = z[ 2 * q ];
   a[ 2 * h + 1 ] = z[ 2 * q + 1 ];
   }
  }

 /// the block b of W items of the block u of the rows of B, k( u , l ) =
 /// ( o + u W + fftLane( l ) ) mod m: the W items k( u , l ) of the block
 /// b of B, transposed, times the powers of omega_n; the item of the
 /// column j at c[ j ]
 template< bool inv >
 ADS_FFT_INLINE void rowBlock( std::size_t u , std::size_t b ,
                               C * c ) const {
  V re[ W ] , im[ W ];
  const bool wrap = wraps( u );
  const C * a = B + b * M + o + u * W;
  for( std::size_t l = 0 ; l < W ; ++l ) {
   std::size_t k = fftLane< W >( l );
   if( wrap && o + u * W + k >= m ) k -= m;
   re[ l ] = a[ k ].r;
   im[ l ] = a[ k ].i;
   }
  transpose< W >( re );
  transpose< W >( im );
  for( std::size_t l = 0 ; l < W ; ++l ) {
   const std::size_t j = b * W + fftLane< W >( l );
   const C y{ re[ l ] , im[ l ] };
   if( ! split ) {
    const C & w = T->t[ u * r + j ];
    c[ j ] = cmul< inv >( y , w.r , w.i );
    }
   else if( wrap ) {
    const C & w = T->w[ j ];
    c[ j ] = cmul< inv >( y , w.r , w.i );
    }
   else {
    const double * s = T->s.data() + 2 * ( u * r + j );
    const C w = cmul< false >( T->t[ j ] , splat< W >( s[ 0 ] ) ,
                               splat< W >( s[ 1 ] ) );
    c[ j ] = cmul< inv >( y , w.r , w.i );
    }
   }
  }
 };

/// the first step on the columns of length N <= fftCodelet, by codelets:
/// the block t of W columns goes to the block t of B
template< std::size_t N , bool inv , std::size_t W , class V >
ADS_FFT_NOINLINE void fourStepColumns( const FFTFour< W , V > & F )
{
 using C = FFTCx< V >;
 const double * R = codeletRoots();
 for( std::size_t t = 0 ; t < F.r / W ; ++t ) {
  C * o = F.B + t * F.M;
  const double * a = F.x + 2 * t * W;
  const std::size_t r = F.r;
  auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
   return( loadComplex< W >( a + 2 * r * i ) );
   };
  auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
   o[ k ] = y;
   };
  codelet< N , inv , W , C >( in , out , R );
  }
 }

/// the last step on the rows of length N <= fftCodelet, by codelets: the
/// block u of W rows of B, times the powers of omega_n, gives the block u
/// of W numbers of the rows of the output
template< std::size_t N , bool inv , std::size_t W , class V >
ADS_FFT_NOINLINE void fourStepRows( const FFTFour< W , V > & F )
{
 using C = FFTCx< V >;
 const double * R = codeletRoots();
 const V f = splat< W >( inv ? 1 / double( N * F.m ) : 1 );   // exact
 for( std::size_t u = 0 ; u < F.m / W ; ++u ) {
  C c[ N ];
  for( std::size_t b = 0 ; b < N / W ; ++b )
   F.template rowBlock< inv >( u , b , c );
  const std::size_t k = F.o + u * W;
  FFTArray< C > in{ c };
  if( F.wraps( u ) ) {
   auto out = [ & ]( std::size_t i , C y ) ADS_FFT_LAMBDA {
    if constexpr( inv ) { y.r = y.r * f; y.i = y.i * f; }
    F.output( i , k , y );
    };
   codelet< N , inv , W , C >( in , out , R );
   continue;
   }
  double * a = F.x + 2 * k;
  const std::size_t m = F.m;
  auto out = [ & ]( std::size_t i , C y ) ADS_FFT_LAMBDA {
   if constexpr( inv ) { y.r = y.r * f; y.i = y.i * f; }
   storeComplex< W >( a + 2 * m * i , y );
   };
  codelet< N , inv , W , C >( in , out , R );
  }
 }

/// the transform of the n = r m points of x in place, r and m powers of 2
/// multiple of W, by the four steps of Bailey on the matrix of m rows and
/// r columns of x [see FFTFour]: the transforms of length m of its
/// columns, W at a time, kept in B; the powers of omega_n; the transforms
/// of length r of the rows of B, W at a time, whose outputs are those of
/// x in their order. The lanes of an item hold the columns, then the
/// rows, in the order of fftLane(), which loadComplex() and storeComplex()
/// read and write. The transforms longer than fftCodelet go fftStrips at
/// a time, gathered from x and scattered to it a few lines at a time,
/// fftAhead rows ahead brought into the cache
template< bool inv , std::size_t W >
inline void fourStep( double * x , std::size_t r , std::size_t m )
{
 using V = typename FFTLaneType< W >::type;
 using C = FFTCx< V >;
 constexpr std::size_t S = fftStrips , P = fftAhead ,
                       Q = 64 / ( 16 * W ) ? 64 / ( 16 * W ) : 1;
 const std::size_t n = r * m , L = std::max( r , m ) , M = m + W ,
                   o = fftShift< W >( x );
 const unsigned K = unsigned( std::countr_zero( n ) );
 C * B = fourStepBuffer< V >( r / W * M + 3 * S * L );
 C * g = B + r / W * M , * u = g + S * L , * v = u + S * L;
 const FFTFour< W , V > F{ x , r , m , o , M , B ,
                           & fourStepRoots< W , V >( K , r , m , o ) ,
                           K >= fftSplitRoots };
 if( ! byCodelet( m , [ & ]< std::size_t N >() {   // the columns
        if constexpr( N >= W ) fourStepColumns< N , inv , W >( F );
        } ) )
  for( std::size_t t0 = 0 ; t0 < r / W ; t0 += S ) {
   for( std::size_t i = 0 ; i < m ; ++i ) {
    if( i + P < m )
     for( std::size_t t = 0 ; t < S ; t += Q )
      ADS_FFT_PREFETCH( x + 2 * ( r * ( i + P ) + ( t0 + t ) * W ) , 0 );
    for( std::size_t t = 0 ; t < S ; ++t )
     g[ t * m + i ] = loadComplex< W >( x + 2 * ( r * i + ( t0 + t ) * W ) );
    }
   for( std::size_t t = 0 ; t < S ; ++t )
    batched< inv , W >( unsigned( std::countr_zero( m ) ) , g + t * m ,
                        B + ( t0 + t ) * M , u , v );
   }
 if( ! byCodelet( r , [ & ]< std::size_t N >() {   // the rows
        if constexpr( N >= W ) fourStepRows< N , inv , W >( F );
        } ) ) {
  const V f = splat< W >( inv ? 1 / double( n ) : 1 );   // exact
  for( std::size_t u0 = 0 ; u0 < m / W ; u0 += S ) {
   for( std::size_t t = 0 ; t < S ; ++t ) {
    for( std::size_t b = 0 ; b < r / W ; ++b ) {
     if( b + 4 < r / W )                   // 4 blocks ahead
      for( std::size_t l = 0 ; l < W ; ++l )
       ADS_FFT_PREFETCH( B + ( b + 4 ) * M + o + ( u0 + t ) * W + l , 0 );
     F.template rowBlock< inv >( u0 + t , b , v );
     }
    batched< inv , W >( unsigned( std::countr_zero( r ) ) , v , g + t * r ,
                        u , v + r );
    }
   for( std::size_t i = 0 ; i < r ; ++i )
    for( std::size_t t = 0 ; t < S ; ++t ) {
     if( i + P < r && t % Q == 0 )
      ADS_FFT_PREFETCH( x + 2 * ( m * ( i + P ) + o + ( u0 + t ) * W ) , 1 );
     C y = g[ t * r + i ];
     if( inv ) { y.r = y.r * f; y.i = y.i * f; }
     F.output( i , o + ( u0 + t ) * W , y );
     }
   }
  }
 }

/// the transform of the n <= fftCodelet points of x in place, by one
/// codelet on single lanes
template< bool inv >
inline void smallTransform( double * x , std::size_t n )
{
 using C = FFTCx< double >;
 const double f = inv ? 1 / double( n ) : 1;   // exact, n a power of 2
 auto in = [ & ]( std::size_t i ) ADS_FFT_LAMBDA {
  return( C{ x[ 2 * i ] , x[ 2 * i + 1 ] } );
  };
 auto out = [ & ]( std::size_t k , const C & y ) ADS_FFT_LAMBDA {
  x[ 2 * k ] = y.r * f;
  x[ 2 * k + 1 ] = y.i * f;
  };
 byCodelet( n , [ & ]< std::size_t N >() {
  codelet< N , inv , 1 , C >( in , out , codeletRoots() ); } );
 }

/// log_2 of the length of the rows of the four steps on 2^K points: 32
/// up to 2^15 points and 64 at 2^16, the codelets that keep most of their
/// items in registers, so that only the columns go by the rounds of
/// batched(); 2^{floor( K / 2 )} below 2^10 and above 2^16
inline constexpr unsigned fftRows( unsigned K )
{
 return( K <= 15 ? std::min( 5u , K / 2 ) : K == 16 ? 6 : K / 2 );
 }

/// the transform of the n points of a, a power of 2, in place: one codelet
/// up to fftCodelet points, above it the four steps on lanes of W <= 8
/// numbers, on rows of r = 2^fftRows( log_2 n ) >= 8 points
template< bool inv , std::size_t W = fftWidth >
inline void transform( Complex * a , std::size_t n )
{
 static_assert( W <= 8 && 8 * 8 <= fftCodelet );
 double * x = reinterpret_cast< double * >( a );   // [complex.numbers]
 if( n <= fftCodelet ) {
  if( n ) smallTransform< inv >( x , n );
  return;
  }
 const std::size_t r = std::size_t( 1 )
                       << fftRows( unsigned( std::countr_zero( n ) ) );
 if constexpr( ADS_FFT_SIMD == 0 && W > 1 )   // single numbers, faster
  if( n <= 8192 ) {                          // on the arrays up to 2^13
   fourStep< inv , 1 >( x , r , n / r );
   return;
   }
 fourStep< inv , W >( x , r , n / r );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE REAL TRANSFORMS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the product of two complex numbers by its formula: the operator * of
/// std::complex also handles the infinities and the NaNs (Annex G of C),
/// a call that costs more than the four products here
inline Complex times( const Complex & a , const Complex & b )
{
 return( Complex( a.real() * b.real() - a.imag() * b.imag() ,
                  a.real() * b.imag() + a.imag() * b.real() ) );
 }

/// the powers omega^k , k < n / 2, omega = e^{-2 pi i / n}, n >= 2 a power
/// of 2, each computed alone; kept per thread
inline const std::vector< Complex > & fftOmegas( std::size_t n )
{
 thread_local std::vector< std::vector< Complex > > T( 64 );
 std::vector< Complex > & t = T[ std::countr_zero( n ) ];
 if( t.empty() ) {
  t.resize( n / 2 );
  for( std::size_t k = 0 ; k < n / 2 ; ++k ) {
   double c , s;
   unitRoot( k , n , c , s );
   t[ k ] = Complex( c , -s );
   }
  }
 return( t );
 }

/// the transform of the real x padded by zeros to n points, n >= 2 a power
/// of 2, by one complex transform of h = n / 2 points, those of even index
/// as the real parts and those of odd index as the imaginary ones; the
/// values at k and k + h, k < h, go to lo[ k ] and hi[ k ]
inline void realTransform( const std::vector< double > & x , std::size_t n ,
                           std::vector< Complex > & lo ,
                           std::vector< Complex > & hi )
{
 const std::size_t h = n / 2;
 std::vector< Complex > z( h );
 for( std::size_t j = 0 ; j + 1 < x.size() ; j += 2 )
  z[ j / 2 ] = Complex( x[ j ] , x[ j + 1 ] );
 if( x.size() % 2 ) z[ x.size() / 2 ] = x.back();
 transform< false >( z.data() , h );
 lo.resize( h );
 hi.resize( h );
 const std::vector< Complex > & w = fftOmegas( n );
 for( std::size_t k = 0 ; k < h ; ++k ) {
  const Complex u = z[ k ] , v = std::conj( z[ k ? h - k : 0 ] ) ,
                d = 0.5 * ( u - v ) ,     // i times the odd half
                wo = times( w[ k ] , Complex( d.imag() , -d.real() ) );
  lo[ k ] = 0.5 * ( u + v ) + wo;
  hi[ k ] = 0.5 * ( u + v ) - wo;
  }
 }

}  // end( namespace detail )

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

inline void fft( std::vector< Complex > & a , bool inverse )
{
 const std::size_t n = a.size();
 ADS_PRE( ( n & ( n - 1 ) ) == 0 );
 if( inverse ) detail::transform< true >( a.data() , n );
 else detail::transform< false >( a.data() , n );
 }

inline std::vector< double > convolve( const std::vector< double > & p ,
                                       const std::vector< double > & q )
{
 if( p.empty() || q.empty() ) return( std::vector< double >() );
 const std::size_t m = p.size() + q.size() - 1;
 if( m == 1 ) return( std::vector< double >( 1 , p[ 0 ] * q[ 0 ] ) );
 std::size_t n = 2;
 while( n < m ) n *= 2;
 std::vector< Complex > P , Ph , Q , Qh;
 detail::realTransform( p , n , P , Ph );
 detail::realTransform( q , n , Q , Qh );
 // the product C is real: back by a transform of h points of E + i O,
 // E and O its halves of even and odd index, whose transforms at k are
 // ( C_k + C_{k+h} ) / 2 and ( C_k - C_{k+h} ) conj( omega^k ) / 2
 const std::vector< Complex > & w = detail::fftOmegas( n );
 for( std::size_t k = 0 ; k < n / 2 ; ++k ) {
  const Complex a = detail::times( P[ k ] , Q[ k ] ) ,
                b = detail::times( Ph[ k ] , Qh[ k ] ) ,
                o = detail::times( 0.5 * ( a - b ) , std::conj( w[ k ] ) );
  P[ k ] = 0.5 * ( a + b ) + Complex( -o.imag() , o.real() );
  }
 detail::transform< true >( P.data() , n / 2 );
 std::vector< double > r( m );
 for( std::size_t i = 0 ; i + 1 < m ; i += 2 ) {
  r[ i ] = P[ i / 2 ].real();
  r[ i + 1 ] = P[ i / 2 ].imag();
  }
 if( m % 2 ) r[ m - 1 ] = P[ m / 2 ].real();
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#undef ADS_FFT_INLINE
#undef ADS_FFT_NOINLINE
#undef ADS_FFT_LAMBDA
#undef ADS_FFT_PREFETCH
#undef ADS_FFT_SHUFFLE

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FFT.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
