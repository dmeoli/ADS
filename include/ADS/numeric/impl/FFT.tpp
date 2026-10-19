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

// the arrays of a butterfly do not overlap, which lets the compiler keep
// them in vector registers without checking it at run time
#if defined( __GNUC__ ) || defined( __clang__ ) || defined( _MSC_VER )
 #define ADS_FFT_RESTRICT __restrict
#else
 #define ADS_FFT_RESTRICT
#endif

namespace ADS {

namespace detail {

/// the product of two complex numbers by its formula: the operator * of
/// std::complex also handles the infinities and the NaNs (Annex G of C),
/// a call that costs more than the four products here
inline Complex times( const Complex & a , const Complex & b )
{
 return( Complex( a.real() * b.real() - a.imag() * b.imag() ,
                  a.real() * b.imag() + a.imag() * b.real() ) );
 }

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

/// the powers of omega = e^{-2 pi i / L} of the rounds of radix 4, of
/// every length L , 16 <= L <= N, a power of 2: the round of L from
/// 3 ( L - 16 ) / 2 on, by groups of 4 consecutive k < L / 4, each group
/// the real and the imaginary parts of omega^k , omega^{2 k} ,
/// omega^{3 k}, 4 values each, so that a round reads one array in a row;
/// the table of N holds those of every smaller size
struct FFTTables {
 std::size_t N = 0;
 std::vector< double > w;
 };

/// the place in FFTTables::w of the round of length L
inline constexpr std::size_t fftRound( std::size_t L )
{
 return( 3 * ( L - 16 ) / 2 );
 }

/// the table of a size at least n, kept per thread and grown by the new
/// lengths only, so that it serves every size and both directions
inline const FFTTables & fftTables( std::size_t n )
{
 thread_local FFTTables T;
 if( n > T.N ) {
  n = std::max< std::size_t >( n , 16 );
  T.w.resize( fftRound( 2 * n ) );
  double c , s;
  for( std::size_t L = std::max< std::size_t >( 16 , 2 * T.N ) ; L <= n ;
       L *= 2 )
   for( std::size_t k = 0 ; k < L / 4 ; ++k ) {
    double * g = T.w.data() + fftRound( L ) + 24 * ( k / 4 ) + k % 4;
    for( std::size_t e = 1 ; e <= 3 ; ++e ) {
     unitRoot( e * k , L , c , s );
     g[ 8 * ( e - 1 ) ] = c;
     g[ 8 * ( e - 1 ) + 4 ] = -s;
     }
    }
  T.N = n;
  }
 return( T );
 }

/// fn( k , omega^k ) for k = 0 , ... , n / 2 - 1, omega = e^{-2 pi i / n},
/// from the table if n >= 16
template< class F >
inline void forOmegas( const FFTTables & T , std::size_t n , F && fn )
{
 if( n < 16 ) {
  double c , s;
  for( std::size_t k = 0 ; k < n / 2 ; ++k ) {
   unitRoot( k , n , c , s );
   fn( k , Complex( c , -s ) );
   }
  return;
  }
 const double * w = T.w.data() + fftRound( n );
 for( std::size_t k = 0 ; k < n / 4 ; ++k ) {
  const double * g = w + 24 * ( k / 4 ) + k % 4;
  fn( k , Complex( g[ 0 ] , g[ 4 ] ) );
  }
 for( std::size_t k = 0 ; k < n / 4 ; ++k ) {   // times omega^{n/4} = -i
  const double * g = w + 24 * ( k / 4 ) + k % 4;
  fn( k + n / 4 , Complex( g[ 4 ] , -g[ 0 ] ) );
  }
 }

/// the buffer of the real and the imaginary parts of n items, kept per
/// thread: the imaginary parts at n + fftPad, so that the two arrays fall
/// in different sets of the cache
inline constexpr std::size_t fftPad = 32;

inline double * fftBuffer( std::size_t n )
{
 thread_local std::vector< double > B;
 if( B.size() < 2 * n + fftPad ) B.resize( 2 * n + fftPad );
 return( B.data() );
 }

/// the length of the blocks transformed one at a time: above it, the
/// rounds go depth first, so that a block stays in the cache of level 2
/// while all its rounds run
inline constexpr std::size_t fftBlock = 2048;

/// the transform of 4 points in place, (r, i) the real and imaginary parts
template< bool inv >
inline void dft4( double * r , double * i )
{
 const double t0r = r[ 0 ] + r[ 2 ] , t0i = i[ 0 ] + i[ 2 ] ,
              t1r = r[ 0 ] - r[ 2 ] , t1i = i[ 0 ] - i[ 2 ] ,
              t2r = r[ 1 ] + r[ 3 ] , t2i = i[ 1 ] + i[ 3 ] ,
              t3r = r[ 1 ] - r[ 3 ] , t3i = i[ 1 ] - i[ 3 ];
 r[ 0 ] = t0r + t2r;
 i[ 0 ] = t0i + t2i;
 r[ 2 ] = t0r - t2r;
 i[ 2 ] = t0i - t2i;
 if constexpr( inv ) {                     // t1 +- i t3
  r[ 1 ] = t1r - t3i;
  i[ 1 ] = t1i + t3r;
  r[ 3 ] = t1r + t3i;
  i[ 3 ] = t1i - t3r;
  }
 else {                                    // t1 -+ i t3
  r[ 1 ] = t1r + t3i;
  i[ 1 ] = t1i - t3r;
  r[ 3 ] = t1r - t3i;
  i[ 3 ] = t1i + t3r;
  }
 }

/// the first two rounds and the reversal of the bits in one sweep: the
/// block j of 4 points gets the transform of the a[ s + t n / 4 ], t < 4,
/// s the reversal of j; a is read in order, (re, im) written by blocks
template< bool inv >
inline void base4( const double * a , std::size_t n , double * re ,
                   double * im )
{
 const std::size_t m = n / 4;
 double r[ 4 ] , i[ 4 ];
 for( std::size_t s = 0 , j = 0 ; s < m ; ++s ) {
  for( std::size_t t = 0 ; t < 4 ; ++t ) {
   r[ t ] = a[ 2 * ( s + t * m ) ];
   i[ t ] = a[ 2 * ( s + t * m ) + 1 ];
   }
  dft4< inv >( r , i );
  for( std::size_t t = 0 ; t < 4 ; ++t ) {
   re[ 4 * j + t ] = r[ t ];
   im[ 4 * j + t ] = i[ t ];
   }
  std::size_t bit = m >> 1;               // j = the reversal of s + 1
  for( ; j & bit ; bit >>= 1 ) j ^= bit;
  j ^= bit;
  }
 }

/// as base4(), the first three rounds by blocks of 8 points, when
/// log_2 n is odd
template< bool inv >
inline void base8( const double * a , std::size_t n , double * re ,
                   double * im )
{
 constexpr double h = std::numbers::sqrt2 / 2;
 constexpr double sg = inv ? -1 : 1;
 const std::size_t m = n / 8;
 double er[ 4 ] , ei[ 4 ] , orr[ 4 ] , oi[ 4 ];
 for( std::size_t s = 0 , j = 0 ; s < m ; ++s ) {
  for( std::size_t t = 0 ; t < 4 ; ++t ) {
   er[ t ] = a[ 2 * ( s + 2 * t * m ) ];
   ei[ t ] = a[ 2 * ( s + 2 * t * m ) + 1 ];
   orr[ t ] = a[ 2 * ( s + ( 2 * t + 1 ) * m ) ];
   oi[ t ] = a[ 2 * ( s + ( 2 * t + 1 ) * m ) + 1 ];
   }
  dft4< inv >( er , ei );
  dft4< inv >( orr , oi );
  // the odd half times omega_8^t: 1 , h ( 1 -+ i ) , -+ i , h ( -1 -+ i )
  double x = orr[ 1 ] , y = oi[ 1 ];
  orr[ 1 ] = h * ( x + sg * y );
  oi[ 1 ] = h * ( y - sg * x );
  x = orr[ 2 ];
  orr[ 2 ] = sg * oi[ 2 ];
  oi[ 2 ] = -sg * x;
  x = orr[ 3 ];
  y = oi[ 3 ];
  orr[ 3 ] = h * ( sg * y - x );
  oi[ 3 ] = -h * ( y + sg * x );
  for( std::size_t t = 0 ; t < 4 ; ++t ) {
   re[ 8 * j + t ] = er[ t ] + orr[ t ];
   im[ 8 * j + t ] = ei[ t ] + oi[ t ];
   re[ 8 * j + t + 4 ] = er[ t ] - orr[ t ];
   im[ 8 * j + t + 4 ] = ei[ t ] - oi[ t ];
   }
  std::size_t bit = m >> 1;
  for( ; j & bit ; bit >>= 1 ) j ^= bit;
  j ^= bit;
  }
 }

/// the radix-4 butterflies of q points, q a multiple of 4: the quarter t
/// of a block holds the transform of the points of residue 0 , 2 , 1 , 3
/// mod 4; the quarters 1 , 2 , 3 are multiplied by omega^{2 k} ,
/// omega^k , omega^{3 k}, read from w by groups of 4, then combined as in
/// dft4()
template< bool inv >
inline void butterflies4( std::size_t q ,
                          double * ADS_FFT_RESTRICT r0 ,
                          double * ADS_FFT_RESTRICT i0 ,
                          double * ADS_FFT_RESTRICT r1 ,
                          double * ADS_FFT_RESTRICT i1 ,
                          double * ADS_FFT_RESTRICT r2 ,
                          double * ADS_FFT_RESTRICT i2 ,
                          double * ADS_FFT_RESTRICT r3 ,
                          double * ADS_FFT_RESTRICT i3 ,
                          const double * ADS_FFT_RESTRICT w )
{
 for( std::size_t g = 0 ; g < q ; g += 4 , w += 24 )
  for( std::size_t u = 0 ; u < 4 ; ++u ) {
   const std::size_t k = g + u;
   const double c1 = w[ u ] , c2 = w[ 8 + u ] , c3 = w[ 16 + u ] ,
                w1i = inv ? -w[ 4 + u ] : w[ 4 + u ] ,
                w2i = inv ? -w[ 12 + u ] : w[ 12 + u ] ,
                w3i = inv ? -w[ 20 + u ] : w[ 20 + u ];
   const double ar = r0[ k ] , ai = i0[ k ] ,
                br = r1[ k ] * c2 - i1[ k ] * w2i ,
                bi = r1[ k ] * w2i + i1[ k ] * c2 ,
                cr = r2[ k ] * c1 - i2[ k ] * w1i ,
                ci = r2[ k ] * w1i + i2[ k ] * c1 ,
                dr = r3[ k ] * c3 - i3[ k ] * w3i ,
                di = r3[ k ] * w3i + i3[ k ] * c3;
   const double t0r = ar + br , t0i = ai + bi , t1r = ar - br ,
                t1i = ai - bi , t2r = cr + dr , t2i = ci + di ,
                t3r = cr - dr , t3i = ci - di;
   r0[ k ] = t0r + t2r;
   i0[ k ] = t0i + t2i;
   r2[ k ] = t0r - t2r;
   i2[ k ] = t0i - t2i;
   if constexpr( inv ) {                   // t1 +- i t3
    r1[ k ] = t1r - t3i;
    i1[ k ] = t1i + t3r;
    r3[ k ] = t1r + t3i;
    i3[ k ] = t1i - t3r;
    }
   else {                                  // t1 -+ i t3
    r1[ k ] = t1r + t3i;
    i1[ k ] = t1i - t3r;
    r3[ k ] = t1r - t3i;
    i3[ k ] = t1i + t3r;
    }
   }
 }

/// the radix-4 round of the block (re, im) of L >= 16 points
template< bool inv >
inline void radix4( double * re , double * im , std::size_t L ,
                    const FFTTables & T )
{
 const std::size_t q = L / 4;
 butterflies4< inv >( q , re , im , re + q , im + q , re + 2 * q ,
                      im + 2 * q , re + 3 * q , im + 3 * q ,
                      T.w.data() + fftRound( L ) );
 }

/// the radix-4 rounds of lengths 4 L0 , 16 L0 , ... , L on the block
/// (re, im) of L points, whose blocks of L0 are already transformed: round
/// by round within a block of fftBlock points, depth first above it
template< bool inv >
inline void passes( double * re , double * im , std::size_t L ,
                    std::size_t L0 , const FFTTables & T )
{
 if( L <= fftBlock ) {
  for( std::size_t len = 4 * L0 ; len <= L ; len *= 4 )
   for( std::size_t o = 0 ; o < L ; o += len )
    radix4< inv >( re + o , im + o , len , T );
  return;
  }
 const std::size_t q = L / 4;
 for( std::size_t t = 0 ; t < 4 ; ++t )
  passes< inv >( re + t * q , im + t * q , q , L0 , T );
 radix4< inv >( re , im , L , T );
 }

/// the transform of the n points of a, a power of 2, in place
template< bool inv >
inline void transform( Complex * a , std::size_t n )
{
 if( n <= 1 ) return;
 if( n == 2 ) {
  const Complex u = a[ 0 ] , v = a[ 1 ];
  a[ 0 ] = u + v;
  a[ 1 ] = u - v;
  if constexpr( inv ) { a[ 0 ] /= 2.0; a[ 1 ] /= 2.0; }
  return;
  }
 double * x = reinterpret_cast< double * >( a );   // [complex.numbers]
 const FFTTables & T = fftTables( n );
 double * re = fftBuffer( n ) , * im = re + n + fftPad;
 std::size_t L0 = 4;                       // log_2 n - log_2 L0 even
 if( std::countr_zero( n ) % 2 ) {
  base8< inv >( x , n , re , im );
  L0 = 8;
  }
 else
  base4< inv >( x , n , re , im );
 passes< inv >( re , im , n , L0 , T );
 const double f = inv ? 1 / double( n ) : 1;  // exact, n a power of 2
 for( std::size_t i = 0 ; i < n ; ++i ) {
  x[ 2 * i ] = re[ i ] * f;
  x[ 2 * i + 1 ] = im[ i ] * f;
  }
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
 forOmegas( fftTables( n ) , n , [ & ]( std::size_t k , Complex w ) {
  const Complex u = z[ k ] , v = std::conj( z[ k ? h - k : 0 ] ) ,
                d = 0.5 * ( u - v ) ,     // i times the odd half
                wo = times( w , Complex( d.imag() , -d.real() ) );
  lo[ k ] = 0.5 * ( u + v ) + wo;
  hi[ k ] = 0.5 * ( u + v ) - wo;
  } );
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
 detail::forOmegas( detail::fftTables( n ) , n ,
                    [ & ]( std::size_t k , Complex w ) {
  const Complex a = detail::times( P[ k ] , Q[ k ] ) ,
                b = detail::times( Ph[ k ] , Qh[ k ] ) ,
                o = detail::times( 0.5 * ( a - b ) , std::conj( w ) );
  P[ k ] = 0.5 * ( a + b ) + Complex( -o.imag() , o.real() );
  } );
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

#undef ADS_FFT_RESTRICT

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FFT.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
