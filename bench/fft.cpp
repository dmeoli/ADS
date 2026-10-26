/*--------------------------------------------------------------------------*/
/*------------------------------ File fft.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the fast Fourier transforms of ADS against FFTW (a plan
 * made once with FFTW_ESTIMATE and one with FFTW_MEASURE, whose planning
 * is not timed; ADS_HAS_FFTW, and ADS_HAS_FFTWF for single precision),
 * pocketfft (ADS_HAS_POCKETFFT) and ducc0 (ADS_HAS_DUCC0), both by the
 * functions that take a whole array, whose plans they keep themselves.
 * For each size n, the cases are:
 *
 * - complex: the transform of n complex numbers in double precision,
 *   forward and in place (and dft(), its definition, up to 1024 points);
 * - float: the same in single precision;
 * - real: the n / 2 + 1 outputs of n real numbers, out of place;
 * - dct2: the transform of the cosines of type 2 of n real numbers;
 * - many: 64 transforms of n complex numbers each, contiguous, up to 2^14;
 * - 2d: the transform of the matrix of sqrt( n ) x sqrt( n ) points, or of
 *   2^{floor( K / 2 )} rows if n = 2^K;
 * - convolve: the product of two polynomials of n / 2 real coefficients,
 *   by the real transforms of the size that convolve() takes, for all.
 *
 * A run transforms k = ceil( 2^15 / n ) copies of the input, each on lines
 * of the cache of its own, so that a run of the smallest sizes is well
 * above the resolution of the clock; each cell is the median of r runs,
 * in nanoseconds per point (per point of the output for convolve), the
 * copies made before the clock starts; the contracts are off
 * (ADS_CONTRACTS=0). The benchmark "fft plan" gives, per point, the time
 * of the first transform of a size in a new thread less that of the
 * second, for ADS, and that of the planning, for FFTW.
 *
 *   fft [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/numeric/FFT.h"

#if defined( ADS_HAS_FFTW ) || defined( ADS_HAS_FFTWF )
 #include <fftw3.h>
#endif

#ifdef ADS_HAS_POCKETFFT
 #include "pocketfft_hdronly.h"
#endif

#ifdef ADS_HAS_DUCC0
 #include "ducc0/fft/fftnd_impl.h"
 #include "ducc0/infra/string_utils.cc"
 #include "ducc0/infra/threading.cc"
#endif

#include <cmath>
#include <complex>
#include <thread>

using namespace ADS::bench;

namespace {

using Cd = std::complex< double >;
using Cf = std::complex< float >;

/// q random numbers, real or complex
template< class X >
std::vector< X > random( std::size_t q , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::normal_distribution< double > N;
 std::vector< X > A( q );
 for( auto & x : A )
  if constexpr( std::is_floating_point_v< X > ) x = X( N( g ) );
  else {
   const double re = N( g );
   x = X( typename X::value_type( re ) , typename X::value_type( N( g ) ) );
   }
 return( A );
 }

/// the copies of a run on n points, and the distance between two of them,
/// a multiple of 8 numbers so that they all start alike on the lines
std::size_t copies( std::size_t n )
{
 return( std::max< std::size_t >( 1 , ( 32768 + n - 1 ) / n ) );
 }

std::size_t stride( std::size_t n )
{
 return( ( n + 7 ) / 8 * 8 );
 }

/// the rows of the matrix of the case 2d on n points, or 0 if none
std::size_t rows2d( std::size_t n )
{
 std::size_t s = std::size_t( std::sqrt( double( n ) ) );
 while( s * s > n ) --s;
 while( ( s + 1 ) * ( s + 1 ) <= n ) ++s;
 if( s * s == n ) return( s );
 if( ( n & ( n - 1 ) ) == 0 && n > 1 )
  return( std::size_t( 1 ) << ( std::countr_zero( n ) / 2 ) );
 return( 0 );
 }

/// the size of the transforms of convolve() for an output of m points
std::size_t convSize( std::size_t m )
{
 return( ADS::detail::fftConvSize( m ) );
 }

/// the median time of f() per one of pts points, the input restored by
/// g(), untimed
template< class G , class F >
double timed( int r , std::size_t pts , G && g , F && f )
{
 return( median( [ & ](){ g(); return( 0 ); } ,
                 [ & ]( int ) { f(); } , r , pts ) );
 }

#ifdef ADS_HAS_FFTW
/// the name of the plans of FFTW made with flag
const char * fftwName( unsigned flag )
{
 return( flag == FFTW_ESTIMATE ? "FFTW estimate" : "FFTW measure" );
 }

/// the flags of the plans of FFTW
constexpr unsigned fftwFlags[] = { FFTW_ESTIMATE , FFTW_MEASURE };
#endif

/// the case of the transforms in place of k copies of n complex numbers of
/// type T, at the stride s: ADS, FFTW (by plan( flag , in ) and exec( p ,
/// x ) on the copy x), pocketfft and ducc0
template< class T , class Plan , class Exec , class Destroy >
void complexCase( const char * what , std::size_t n , const Options & o ,
                  Plan && plan , Exec && exec , Destroy && destroy )
{
 using X = std::complex< T >;
 const std::size_t k = copies( n ) , s = stride( n ) , q = k * n;
 const auto A = random< X >( k * s , o.seed );
 std::vector< X > B( k * s );
 auto reset = [ & ] { B = A; };
 const int r = o.runs;
 for( const char * who : { "ADS::fft" , "ADS::fft A/A" } )
  if( ! skip( who ) )
   print( "fft" , what , n , who , timed( r , q , reset , [ & ] {
           for( std::size_t c = 0 ; c < k ; ++c )
            ADS::fft( B.data() + c * s , n , false );
           sink = sink + ( B[ 0 ].real() > 0 ); } ) );
 if( std::is_same_v< T , double > && ( n <= 1024 ) &&
     ! skip( "ADS::dft (definition)" ) ) {
  const std::vector< Cd > a( A.begin() , A.begin() + n );
  print( "fft" , what , n , "ADS::dft (definition)" ,
         timed( r , n , []{} , [ & ] {
          const auto D = ADS::dft( a , false );
          sink = sink + ( D[ 0 ].real() > 0 ); } ) );
  }
 plan( [ & ]( const char * who , X * in , auto && p ) {
  print( "fft" , what , n , who , timed( r , q , [ & ] {
          std::copy( A.begin() , A.end() , in ); } , [ & ] {
           for( std::size_t c = 0 ; c < k ; ++c ) exec( p , in + c * s );
           sink = sink + ( in[ 0 ].real() > 0 ); } ) );
  destroy( p );
  } , k * s );
#ifdef ADS_HAS_POCKETFFT
 if( ! skip( "pocketfft" ) )
  print( "fft" , what , n , "pocketfft" , timed( r , q , reset , [ & ] {
          for( std::size_t c = 0 ; c < k ; ++c )
           pocketfft::c2c< T >( { n } , { sizeof( X ) } , { sizeof( X ) } ,
                                { 0 } , true , B.data() + c * s ,
                                B.data() + c * s , T( 1 ) );
          sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
#ifdef ADS_HAS_DUCC0
 if( ! skip( "ducc0" ) )
  print( "fft" , what , n , "ducc0" , timed( r , q , reset , [ & ] {
          for( std::size_t c = 0 ; c < k ; ++c ) {
           ducc0::cfmav< X > in( B.data() + c * s , { n } );
           ducc0::vfmav< X > out( B.data() + c * s , { n } );
           ducc0::c2c( in , out , { 0 } , true , T( 1 ) , 1 );
           }
          sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 1024 , 65536 , 1048576 } );
 const int r = o.runs;
 for( std::size_t n : o.sizes ) {
  const std::size_t k = copies( n ) , s = stride( n );
  // ---------------------------------------------------- complex, float
  complexCase< double >( "complex" , n , o , [ & ]( auto && run ,
                                                    std::size_t len ) {
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    auto * in = fftw_alloc_complex( len );
    fftw_plan p = fftw_plan_dft_1d( int( n ) , in , in , FFTW_FORWARD ,
                                    flag );
    run( fftwName( flag ) , reinterpret_cast< Cd * >( in ) , p );
    fftw_free( in );
    }
#else
   ( void ) run; ( void ) len;
#endif
   } , []( auto & p , Cd * x ) {
#ifdef ADS_HAS_FFTW
    auto * y = reinterpret_cast< fftw_complex * >( x );
    fftw_execute_dft( p , y , y );
#else
    ( void ) p; ( void ) x;
#endif
   } , []( auto & p ) {
#ifdef ADS_HAS_FFTW
    fftw_destroy_plan( p );
#else
    ( void ) p;
#endif
   } );
  complexCase< float >( "float" , n , o , [ & ]( auto && run ,
                                                 std::size_t len ) {
#ifdef ADS_HAS_FFTWF
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    auto * in = fftwf_alloc_complex( len );
    fftwf_plan p = fftwf_plan_dft_1d( int( n ) , in , in , FFTW_FORWARD ,
                                      flag );
    run( fftwName( flag ) , reinterpret_cast< Cf * >( in ) , p );
    fftwf_free( in );
    }
#else
   ( void ) run; ( void ) len;
#endif
   } , []( auto & p , Cf * x ) {
#ifdef ADS_HAS_FFTWF
    auto * y = reinterpret_cast< fftwf_complex * >( x );
    fftwf_execute_dft( p , y , y );
#else
    ( void ) p; ( void ) x;
#endif
   } , []( auto & p ) {
#ifdef ADS_HAS_FFTWF
    fftwf_destroy_plan( p );
#else
    ( void ) p;
#endif
   } );
  // ------------------------------------------------------------- real
  {
   const std::size_t h = stride( n / 2 + 1 ) , q = k * n;
   const auto x = random< double >( k * s , o.seed );
   std::vector< Cd > X( k * h );
   if( ! skip( "ADS::fft" ) )
    print( "fft" , "real" , n , "ADS::fft" , timed( r , q , []{} , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             ADS::rfft( x.data() + c * s , n , X.data() + c * h );
            sink = sink + ( X[ 0 ].real() > 0 ); } ) );
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    double * in = fftw_alloc_real( k * s );
    auto * out = fftw_alloc_complex( k * h );
    fftw_plan p = fftw_plan_dft_r2c_1d( int( n ) , in , out , flag );
    std::copy( x.begin() , x.end() , in );
    print( "fft" , "real" , n , fftwName( flag ) , timed( r , q , []{} ,
           [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             fftw_execute_dft_r2c( p , in + c * s , out + c * h );
            sink = sink + ( out[ 0 ][ 0 ] > 0 ); } ) );
    fftw_destroy_plan( p );
    fftw_free( in );
    fftw_free( out );
    }
#endif
#ifdef ADS_HAS_POCKETFFT
   if( ! skip( "pocketfft" ) )
    print( "fft" , "real" , n , "pocketfft" , timed( r , q , []{} , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             pocketfft::r2c< double >( { n } , { sizeof( double ) } ,
                                       { sizeof( Cd ) } , 0 , true ,
                                       x.data() + c * s , X.data() + c * h ,
                                       1.0 );
            sink = sink + ( X[ 0 ].real() > 0 ); } ) );
#endif
#ifdef ADS_HAS_DUCC0
   if( ! skip( "ducc0" ) )
    print( "fft" , "real" , n , "ducc0" , timed( r , q , []{} , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             ducc0::cfmav< double > in( x.data() + c * s , { n } );
             ducc0::vfmav< Cd > out( X.data() + c * h , { n / 2 + 1 } );
             ducc0::r2c( in , out , { 0 } , true , 1.0 , 1 );
             }
            sink = sink + ( X[ 0 ].real() > 0 ); } ) );
#endif
  }
  // ------------------------------------------------------------- dct2
  {
   const std::size_t q = k * n;
   const auto x = random< double >( k * s , o.seed );
   std::vector< double > y( k * s );
   auto reset = [ & ] { y = x; };
   if( ! skip( "ADS::fft" ) )
    print( "fft" , "dct2" , n , "ADS::fft" , timed( r , q , reset , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             ADS::dct( y.data() + c * s , n , 2 );
            sink = sink + ( y[ 0 ] > 0 ); } ) );
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    double * in = fftw_alloc_real( k * s );
    fftw_plan p = fftw_plan_r2r_1d( int( n ) , in , in , FFTW_REDFT10 ,
                                    flag );
    print( "fft" , "dct2" , n , fftwName( flag ) , timed( r , q , [ & ] {
            std::copy( x.begin() , x.end() , in ); } , [ & ] {
             for( std::size_t c = 0 ; c < k ; ++c )
              fftw_execute_r2r( p , in + c * s , in + c * s );
             sink = sink + ( in[ 0 ] > 0 ); } ) );
    fftw_destroy_plan( p );
    fftw_free( in );
    }
#endif
#ifdef ADS_HAS_POCKETFFT
   if( ! skip( "pocketfft" ) )
    print( "fft" , "dct2" , n , "pocketfft" , timed( r , q , reset , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             pocketfft::dct< double >( { n } , { sizeof( double ) } ,
                                       { sizeof( double ) } , { 0 } , 2 ,
                                       y.data() + c * s , y.data() + c * s ,
                                       1.0 , false );
            sink = sink + ( y[ 0 ] > 0 ); } ) );
#endif
#ifdef ADS_HAS_DUCC0
   if( ! skip( "ducc0" ) )
    print( "fft" , "dct2" , n , "ducc0" , timed( r , q , reset , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             ducc0::cfmav< double > in( y.data() + c * s , { n } );
             ducc0::vfmav< double > out( y.data() + c * s , { n } );
             ducc0::dct( in , out , { 0 } , 2 , 1.0 , false , 1 );
             }
            sink = sink + ( y[ 0 ] > 0 ); } ) );
#endif
  }
  // ------------------------------------------------------------- many
  if( n <= 16384 ) {
   constexpr std::size_t q = 64;
   const auto A = random< Cd >( n * q , o.seed );
   std::vector< Cd > B( n * q );
   auto reset = [ & ] { B = A; };
   if( ! skip( "ADS::fft" ) )
    print( "fft" , "many" , n , "ADS::fft" , timed( r , n * q , reset , [ & ] {
            ADS::fftMany( B.data() , n , q , false );
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    auto * in = fftw_alloc_complex( n * q );
    const int N = int( n );
    fftw_plan p = fftw_plan_many_dft( 1 , & N , int( q ) , in , nullptr , 1 ,
                                      N , in , nullptr , 1 , N ,
                                      FFTW_FORWARD , flag );
    print( "fft" , "many" , n , fftwName( flag ) , timed( r , n * q , [ & ] {
            std::copy( A.begin() , A.end() , reinterpret_cast< Cd * >( in ) );
            } , [ & ] {
             fftw_execute( p );
             sink = sink + ( in[ 0 ][ 0 ] > 0 ); } ) );
    fftw_destroy_plan( p );
    fftw_free( in );
    }
#endif
#ifdef ADS_HAS_POCKETFFT
   if( ! skip( "pocketfft" ) )
    print( "fft" , "many" , n , "pocketfft" ,
           timed( r , n * q , reset , [ & ] {
            const pocketfft::stride_t st{ std::ptrdiff_t( n * sizeof( Cd ) ) ,
                                          sizeof( Cd ) };
            pocketfft::c2c< double >( { q , n } , st , st , { 1 } , true ,
                                      B.data() , B.data() , 1.0 );
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
#ifdef ADS_HAS_DUCC0
   if( ! skip( "ducc0" ) )
    print( "fft" , "many" , n , "ducc0" , timed( r , n * q , reset , [ & ] {
            ducc0::cfmav< Cd > in( B.data() , { q , n } );
            ducc0::vfmav< Cd > out( B.data() , { q , n } );
            ducc0::c2c( in , out , { 1 } , true , 1.0 , 1 );
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
  }
  // --------------------------------------------------------------- 2d
  if( const std::size_t n0 = rows2d( n ) ; n0 > 1 ) {
   const std::size_t n1 = n / n0 , q = k * n;
   const auto A = random< Cd >( k * s , o.seed );
   std::vector< Cd > B( k * s );
   auto reset = [ & ] { B = A; };
   if( ! skip( "ADS::fft" ) )
    print( "fft" , "2d" , n , "ADS::fft" , timed( r , q , reset , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c )
             ADS::fft2( B.data() + c * s , n0 , n1 , false );
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    auto * in = fftw_alloc_complex( k * s );
    fftw_plan p = fftw_plan_dft_2d( int( n0 ) , int( n1 ) , in , in ,
                                    FFTW_FORWARD , flag );
    print( "fft" , "2d" , n , fftwName( flag ) , timed( r , q , [ & ] {
            std::copy( A.begin() , A.end() , reinterpret_cast< Cd * >( in ) );
            } , [ & ] {
             for( std::size_t c = 0 ; c < k ; ++c )
              fftw_execute_dft( p , in + c * s , in + c * s );
             sink = sink + ( in[ 0 ][ 0 ] > 0 ); } ) );
    fftw_destroy_plan( p );
    fftw_free( in );
    }
#endif
#ifdef ADS_HAS_POCKETFFT
   if( ! skip( "pocketfft" ) )
    print( "fft" , "2d" , n , "pocketfft" , timed( r , q , reset , [ & ] {
            const pocketfft::stride_t st{ std::ptrdiff_t( n1 * sizeof( Cd ) ) ,
                                          sizeof( Cd ) };
            for( std::size_t c = 0 ; c < k ; ++c )
             pocketfft::c2c< double >( { n0 , n1 } , st , st , { 0 , 1 } ,
                                       true , B.data() + c * s ,
                                       B.data() + c * s , 1.0 );
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
#ifdef ADS_HAS_DUCC0
   if( ! skip( "ducc0" ) )
    print( "fft" , "2d" , n , "ducc0" , timed( r , q , reset , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             ducc0::cfmav< Cd > in( B.data() + c * s , { n0 , n1 } );
             ducc0::vfmav< Cd > out( B.data() + c * s , { n0 , n1 } );
             ducc0::c2c( in , out , { 0 , 1 } , true , 1.0 , 1 );
             }
            sink = sink + ( B[ 0 ].real() > 0 ); } ) );
#endif
  }
  // --------------------------------------------------------- convolve
  if( n >= 4 ) {
   const std::size_t h = n / 2 , m = 2 * h - 1 , q = k * m;
   [[maybe_unused]] const std::size_t N = convSize( m );
   std::vector< std::vector< double > > P( k ) , Q( k );
   for( std::size_t c = 0 ; c < k ; ++c ) {
    P[ c ] = random< double >( h , o.seed + unsigned( 2 * c ) );
    Q[ c ] = random< double >( h , o.seed + unsigned( 2 * c + 1 ) );
    }
   if( ! skip( "ADS::fft" ) )
    print( "fft" , "convolve" , n , "ADS::fft" , timed( r , q , []{} , [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             const auto z = ADS::convolve( P[ c ] , Q[ c ] );
             sink = sink + ( z[ 0 ] > 0 );
             } } ) );
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    double * a = fftw_alloc_real( N ) , * b = fftw_alloc_real( N );
    auto * A = fftw_alloc_complex( N / 2 + 1 ) ,
         * B = fftw_alloc_complex( N / 2 + 1 );
    fftw_plan pa = fftw_plan_dft_r2c_1d( int( N ) , a , A , flag ) ,
              pb = fftw_plan_dft_r2c_1d( int( N ) , b , B , flag ) ,
              pc = fftw_plan_dft_c2r_1d( int( N ) , A , a , flag );
    // as convolve(): the padded copies, the transforms, the product and
    // the inverse, scaled, into a new vector
    print( "fft" , "convolve" , n , fftwName( flag ) , timed( r , q , []{} ,
           [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             std::fill( std::copy( P[ c ].begin() , P[ c ].end() , a ) ,
                        a + N , 0.0 );
             std::fill( std::copy( Q[ c ].begin() , Q[ c ].end() , b ) ,
                        b + N , 0.0 );
             fftw_execute( pa );
             fftw_execute( pb );
             for( std::size_t j = 0 ; j <= N / 2 ; ++j ) {
              const double re = A[ j ][ 0 ] * B[ j ][ 0 ] -
                                A[ j ][ 1 ] * B[ j ][ 1 ] ,
                           im = A[ j ][ 0 ] * B[ j ][ 1 ] +
                                A[ j ][ 1 ] * B[ j ][ 0 ];
              A[ j ][ 0 ] = re;
              A[ j ][ 1 ] = im;
              }
             fftw_execute( pc );
             std::vector< double > z( m );
             for( std::size_t j = 0 ; j < m ; ++j )
              z[ j ] = a[ j ] / double( N );
             sink = sink + ( z[ 0 ] > 0 );
             } } ) );
    fftw_destroy_plan( pa );
    fftw_destroy_plan( pb );
    fftw_destroy_plan( pc );
    fftw_free( a );
    fftw_free( b );
    fftw_free( A );
    fftw_free( B );
    }
#endif
#ifdef ADS_HAS_POCKETFFT
   if( ! skip( "pocketfft" ) )
    print( "fft" , "convolve" , n , "pocketfft" , timed( r , q , []{} ,
           [ & ] {
            for( std::size_t c = 0 ; c < k ; ++c ) {
             std::vector< double > a( N ) , b( N );
             std::vector< Cd > A( N / 2 + 1 ) , B( N / 2 + 1 );
             std::copy( P[ c ].begin() , P[ c ].end() , a.begin() );
             std::copy( Q[ c ].begin() , Q[ c ].end() , b.begin() );
             const pocketfft::stride_t sr{ sizeof( double ) } ,
                                       sc{ sizeof( Cd ) };
             pocketfft::r2c< double >( { N } , sr , sc , 0 , true , a.data() ,
                                       A.data() , 1.0 );
             pocketfft::r2c< double >( { N } , sr , sc , 0 , true , b.data() ,
                                       B.data() , 1.0 );
             for( std::size_t j = 0 ; j <= N / 2 ; ++j ) A[ j ] *= B[ j ];
             pocketfft::c2r< double >( { N } , sc , sr , 0 , false , A.data() ,
                                       a.data() , 1.0 / double( N ) );
             a.resize( m );
             sink = sink + ( a[ 0 ] > 0 );
             } } ) );
#endif
  }
  // -------------------------------------------------------- the plans
  {
   const auto A = random< Cd >( n , o.seed );
   const int R = std::max( 3 , r / 4 );
   std::vector< double > t;
   for( int i = 0 ; ( i < R ) && ! skip( "ADS::fft" ) ; ++i ) {
    double d = 0;
    std::thread( [ & ] {
     auto B = A , C = A;
     const auto a = std::chrono::steady_clock::now();
     ADS::fft( B , false );
     const auto b = std::chrono::steady_clock::now();
     ADS::fft( C , false );
     const auto c = std::chrono::steady_clock::now();
     d = std::chrono::duration< double , std::nano >( ( b - a ) - ( c - b ) )
         .count();
     sink = sink + ( B[ 0 ].real() > C[ 0 ].real() );
     } ).join();
    t.push_back( d / double( n ) );
    }
   if( ! t.empty() ) {
    std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
    print( "fft plan" , "complex" , n , "ADS::fft" , t[ t.size() / 2 ] );
    }
#ifdef ADS_HAS_FFTW
   for( unsigned flag : fftwFlags ) {
    if( skip( fftwName( flag ) ) )
     continue;
    auto * in = fftw_alloc_complex( n );
    std::vector< double > w;
    for( int i = 0 ; i < R ; ++i ) {
     fftw_forget_wisdom();
     const auto a = std::chrono::steady_clock::now();
     fftw_plan p = fftw_plan_dft_1d( int( n ) , in , in , FFTW_FORWARD ,
                                     flag );
     const auto b = std::chrono::steady_clock::now();
     fftw_destroy_plan( p );
     w.push_back( std::chrono::duration< double , std::nano >( b - a )
                  .count() / double( n ) );
     }
    std::nth_element( w.begin() , w.begin() + w.size() / 2 , w.end() );
    print( "fft plan" , "complex" , n , fftwName( flag ) ,
           w[ w.size() / 2 ] );
    fftw_free( in );
    }
#endif
  }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File fft.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
