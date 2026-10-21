/*--------------------------------------------------------------------------*/
/*------------------------------ File fft.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the transforms of FFT.h beyond the powers of 2 of taocp.cpp:
 * every size from 0 to 300 and the sizes where the plans change (the
 * codelets, the four steps with short blocks, the rounds in a prime radix
 * with no codelet, Rader and Bluestein), in double and single precision,
 * against the definition in long double; the real transforms, the
 * transforms of the cosines and of the sines against their definitions;
 * the many transforms and those in 2 and 3 dimensions against those of one
 * dimension; the convolutions in single precision. The errors are the
 * relative errors in the norm of l_2, the measure of the analyses of the
 * rounding of the transforms.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/numeric/FFT.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <random>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

using LD = long double;
using CL = std::complex< LD >;

const LD pi = 3.141592653589793238462643383279502884L;

/// e^{s 2 pi i k / n} in long double, the angle reduced mod n
CL root( std::size_t k , std::size_t n , int s )
{
 const LD x = s * 2 * pi * LD( k % n ) / LD( n );
 return( CL( std::cos( x ) , std::sin( x ) ) );
 }

/// the transform of a in long double, by decimation in time on the
/// smallest prime of n, the definition when n is a prime: the reference
std::vector< CL > reference( const std::vector< CL > & a , int s )
{
 const std::size_t n = a.size();
 if( n <= 1 ) return( a );
 std::size_t p = 2;
 while( p * p <= n && n % p ) ++p;
 if( n % p ) p = n;
 const std::size_t m = n / p;
 std::vector< CL > y( n );
 if( m == 1 ) {
  for( std::size_t k = 0 ; k < n ; ++k )
   for( std::size_t j = 0 ; j < n ; ++j )
    y[ k ] += a[ j ] * root( j * k , n , s );
  return( y );
  }
 std::vector< std::vector< CL > > sub( p );
 for( std::size_t t = 0 ; t < p ; ++t ) {
  std::vector< CL > v( m );
  for( std::size_t j = 0 ; j < m ; ++j ) v[ j ] = a[ j * p + t ];
  sub[ t ] = reference( v , s );
  }
 for( std::size_t k = 0 ; k < n ; ++k )
  for( std::size_t t = 0 ; t < p ; ++t )
   y[ k ] += sub[ t ][ k % m ] * root( t * k , n , s );
 return( y );
 }

/// the transform of a by reference(), divided by n if inv
template< class T >
std::vector< CL > reference( const std::vector< std::complex< T > > & a ,
                             bool inv )
{
 std::vector< CL > b( a.begin() , a.end() );
 auto y = reference( b , inv ? 1 : -1 );
 if( inv )
  for( auto & z : y ) z /= LD( a.size() );
 return( y );
 }

/// the relative error of f against r in the norm of l_2 (absolute if r is
/// 0)
template< class X >
double error( const std::vector< X > & f , const std::vector< CL > & r )
{
 LD e = 0 , s = 0;
 for( std::size_t k = 0 ; k < f.size() ; ++k ) {
  e += std::norm( CL( f[ k ] ) - r[ k ] );
  s += std::norm( r[ k ] );
  }
 return( double( std::sqrt( s > 0 ? e / s : e ) ) );
 }

/// n random complex numbers of type T in [ -1 , 1 ]^2, or real ones
template< class T >
std::vector< std::complex< T > > randomComplex( std::size_t n ,
                                                std::mt19937 & g )
{
 std::uniform_real_distribution< double > U( -1 , 1 );
 std::vector< std::complex< T > > a( n );
 for( auto & x : a ) x = std::complex< T >( T( U( g ) ) , T( U( g ) ) );
 return( a );
 }

template< class T >
std::vector< T > randomReal( std::size_t n , std::mt19937 & g )
{
 std::uniform_real_distribution< double > U( -1 , 1 );
 std::vector< T > a( n );
 for( auto & x : a ) x = T( U( g ) );
 return( a );
 }

/// the bound of the relative error of a transform of n points in the
/// precision of T: c u sqrt( log_2 n ), u the unit roundoff, as the
/// analyses of the transforms with accurate powers of omega give (the
/// constant c covers the rounds and the products)
template< class T >
double bound( std::size_t n )
{
 const double u = std::is_same_v< T , float > ? 6e-8 : 1.2e-16;
 return( 8 * u * std::sqrt( std::log2( double( n ) + 2 ) ) );
 }

/// fft() of n random points in both directions against reference(), and
/// the inverse of the transform
template< class T >
void checkComplex( std::size_t n , std::mt19937 & g , double slack = 1 )
{
 const auto a = randomComplex< T >( n , g );
 for( bool inv : { false , true } ) {
  auto f = a;
  fft( f , inv );
  CHECK( error( f , reference( a , inv ) ) < slack * bound< T >( n ) );
  fft( f , ! inv );
  CHECK( error( f , std::vector< CL >( a.begin() , a.end() ) ) <
         2 * slack * bound< T >( n ) );
  }
 }

/// the transform of the cosines ( c ) or of the sines of type t of x by
/// their definitions, as those of FFTW
std::vector< CL > trig( const std::vector< LD > & x , bool c , unsigned t )
{
 const std::size_t N = x.size();
 // cos or sin of pi a / d, the angle reduced mod 2 pi in integers first
 auto cs = [ & ]( std::size_t a , std::size_t d ) {
  const LD v = pi * LD( a % ( 2 * d ) ) / LD( d );
  return( c ? std::cos( v ) : std::sin( v ) );
  };
 std::vector< CL > y( N );
 for( std::size_t k = 0 ; k < N ; ++k ) {
  LD s = 0;
  for( std::size_t j = 0 ; j < N ; ++j ) {
   const LD sg = k % 2 ? -1 : 1;
   if( c )
    switch( t ) {
     case( 1 ):
      s += j == 0 ? x[ 0 ] : j + 1 == N ? sg * x[ j ]
           : 2 * x[ j ] * cs( j * k , N - 1 );
      break;
     case( 2 ): s += 2 * x[ j ] * cs( ( 2 * j + 1 ) * k , 2 * N ); break;
     case( 3 ):
      s += j == 0 ? x[ 0 ] : 2 * x[ j ] * cs( j * ( 2 * k + 1 ) , 2 * N );
      break;
     default:
      s += 2 * x[ j ] * cs( ( 2 * j + 1 ) * ( 2 * k + 1 ) , 4 * N );
     }
   else
    switch( t ) {
     case( 1 ): s += 2 * x[ j ] * cs( ( j + 1 ) * ( k + 1 ) , N + 1 ); break;
     case( 2 ):
      s += 2 * x[ j ] * cs( ( 2 * j + 1 ) * ( k + 1 ) , 2 * N );
      break;
     case( 3 ):
      s += j + 1 == N ? sg * x[ j ]
           : 2 * x[ j ] * cs( ( j + 1 ) * ( 2 * k + 1 ) , 2 * N );
      break;
     default:
      s += 2 * x[ j ] * cs( ( 2 * j + 1 ) * ( 2 * k + 1 ) , 4 * N );
     }
   }
  y[ k ] = s;
  }
 return( y );
 }


}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// every size from 0 to 300 in double precision, both directions, against
// the reference in long double: the codelets of the powers of 2 and of
// 2^a 3^b 5^c up to 25, the small four steps, the four steps with blocks
// shorter than the lanes, the rounds in the primes 7 , 11 , 13 and in those
// with no codelet, Rader for the primes; the impulse and a constant
ADS_TEST( "fft-any-size" )
{
 std::mt19937 g( 43 );
 for( std::size_t n = 0 ; n <= 300 ; ++n ) checkComplex< double >( n , g );
 for( std::size_t n : { 1 , 7 , 60 , 97 , 210 } ) {
  std::vector< Complex > imp( n ) , c( n , Complex( 2 , -3 ) );
  imp[ 0 ] = 1;
  fft( imp , false );
  fft( c , false );
  for( std::size_t k = 0 ; k < n ; ++k ) {
   CHECK( std::abs( imp[ k ] - Complex( 1 ) ) < 1e-14 );
   CHECK( std::abs( c[ k ] - ( k ? Complex( 0 ) : double( n ) *
                               Complex( 2 , -3 ) ) ) < 1e-13 * double( n ) );
   }
  }
 }

// the sizes where the plans change above 300: products of codelets, the
// four steps on rows and columns of mixed radices, 2 times a prime above
// fftMaxPrime (Bluestein), the primes whose n - 1 is smooth (Rader) or not
// (Rader on a Bluestein, or Bluestein), the largest generic prime radix,
// and a prime of 6 digits
ADS_TEST( "fft-plans" )
{
 std::mt19937 g( 47 );
 for( std::size_t n : { 360 , 500 , 625 , 1000 , 1536 , 2187 , 3125 , 4096 ,
                        6000 , 7203 , 2 * 1009 , 1031 , 4099 , 12289 ,
                        101 * 4 , 103 , 17 * 19 * 4 , 65536 + 4 } )
  checkComplex< double >( n , g , 2 );
 for( std::size_t n : { 100003 , 2 * 50021 } ) {  // at some outputs
  const auto a = randomComplex< double >( n , g );
  for( bool inv : { false , true } ) {
   auto f = a;
   fft( f , inv );
   LD e = 0 , s = 0;
   for( std::size_t k : { std::size_t( 0 ) , std::size_t( 1 ) , n / 2 ,
                          n - 1 , std::size_t( g() % n ) } ) {
    CL d = 0;
    for( std::size_t j = 0 ; j < n ; ++j )
     d += CL( a[ j ] ) * root( j * k , n , inv ? 1 : -1 );
    if( inv ) d /= LD( n );
    e += std::norm( CL( f[ k ] ) - d );
    s += std::norm( d );
    }
   CHECK( std::sqrt( e / s ) < 4 * bound< double >( n ) );
   fft( f , ! inv );
   CHECK( error( f , std::vector< CL >( a.begin() , a.end() ) ) <
          8 * bound< double >( n ) );
   }
  }
 // the plans and the buffers kept per thread, the sizes met again in
 // another order: the same outputs, up to the alignment of the points
 const auto a = randomComplex< double >( 1000 , g );
 auto b = a , c = a;
 fft( b , false );
 std::vector< Complex > d( 7203 );
 fft( d , false );
 fft( c , false );
 for( std::size_t k = 0 ; k < 1000 ; ++k )
  CHECK( std::abs( b[ k ] - c[ k ] ) < 1e-12 );
 }

// the four steps on every factorization n = r m that the plans may time,
// r and m from max( W , 2 ), on lanes of 1, 2 and the widest numbers, the
// blocks of the lanes full or short, against the reference: whatever the
// timing keeps, the transform is right
template< std::size_t W >
void checkFactorizations( std::mt19937 & g )
{
 for( std::size_t n = 4 ; n <= 400 ; n += ( n < 100 ? 1 : 7 ) ) {
  const auto a = randomComplex< double >( n , g );
  const auto ref = reference( a , false );
  constexpr std::size_t w = std::max< std::size_t >( W , 2 );
  for( std::size_t r = w ; r * w <= n ; ++r ) {
   if( n % r ) continue;
   const std::size_t m = n / r;
   const auto sm = detail::fftSteps< double >( m ) ,
              sr = detail::fftSteps< double >( r );
   detail::FFTFourRoots< double , W > R[ 16 ];
   bool made[ 16 ] = {};
   auto f = a;
   double * x = reinterpret_cast< double * >( f.data() );
   detail::fourStep< double , W >( x , x , r , m , sm , sr ,
    [ & ]( std::size_t o ) -> const auto & {
     if( ! made[ o ] ) {
      detail::fourStepRoots( R[ o ] , r , m , o );
      made[ o ] = true;
      }
     return( R[ o ] ); } , false );
   CHECK( error( f , ref ) < 2 * bound< double >( n ) );
   }
  }
 }

ADS_TEST( "fft-factorizations" )
{
 std::mt19937 g( 73 );
 checkFactorizations< 1 >( g );
 checkFactorizations< 2 >( g );
 checkFactorizations< detail::fftWidth >( g );
 }

// the transforms in single precision at every size up to 300 and at
// sizes where the lanes of 8 floats differ from those of 4 doubles
ADS_TEST( "fft-float" )
{
 std::mt19937 g( 53 );
 for( std::size_t n = 0 ; n <= 300 ; ++n ) checkComplex< float >( n , g );
 for( std::size_t n : { 512 , 1000 , 2048 , 4096 , 6000 , 1031 , 65536 ,
                        131072 } )
  checkComplex< float >( n , g , 2 );
 }

// rfft() and irfft() at every size up to 200, odd and even, and above it
// on the four steps, out of place and in place (whose blocks may start
// elsewhere, as the alignment of the points does), against the complex
// transform; the outputs 0 and n / 2 real, the inverse of the transform
ADS_TEST( "rfft" )
{
 std::mt19937 g( 59 );
 auto check = [ & ]< class T >( std::size_t n ) {
  const auto x = randomReal< T >( n , g );
  const auto X = rfft( x );
  CHECK( X.size() == ( n ? n / 2 + 1 : 0 ) );
  std::vector< std::complex< T > > c( x.begin() , x.end() );
  auto r = reference( c , false );
  r.resize( X.size() );
  CHECK( error( X , r ) < 2 * bound< T >( n ) );
  if( n ) CHECK( X[ 0 ].imag() == 0 );
  if( n && n % 2 == 0 ) CHECK( X[ n / 2 ].imag() == 0 );
  const auto y = irfft( X , n );
  CHECK( error( y , std::vector< CL >( x.begin() , x.end() ) ) <
         4 * bound< T >( n ) );
  std::vector< T > z( n + 2 );                // in place
  std::copy( x.begin() , x.end() , z.begin() );
  auto * Z = reinterpret_cast< std::complex< T > * >( z.data() );
  rfft( z.data() , n , Z );                    // as out of place, up to
  const double e = 4 * bound< T >( n ) * std::sqrt( double( n ) + 1 );
  for( std::size_t k = 0 ; k < X.size() ; ++k )   // the alignment
   CHECK( std::abs( Z[ k ] - X[ k ] ) <= e * ( 1 + std::abs( X[ k ] ) ) );
  irfft( Z , n , z.data() );
  for( std::size_t j = 0 ; j < n ; ++j )
   CHECK( std::abs( z[ j ] - y[ j ] ) <= e );
  };
 for( std::size_t n = 0 ; n <= 200 ; ++n ) {
  check.template operator()< double >( n );
  check.template operator()< float >( n );
  }
 for( std::size_t n : { 256 , 1000 , 4096 , 6002 , 65536 } )
  check.template operator()< double >( n );
 CHECK_THROWS( irfft( std::vector< Complex >( 3 ) , 7 ) ,
               contract_violation );
 }

// the transforms of the cosines and of the sines of the 4 types at every
// size from 1 to 64 (2 to 64 for the type 1 of the cosines) and at some
// larger, odd and even, against their definitions (in long double, the
// angles reduced in integers); the inverses (the type 3 of the type 2,
// the type 4 of itself), and the contracts
ADS_TEST( "dct-dst" )
{
 std::mt19937 g( 61 );
 auto check = [ & ]( std::size_t N ) {
  const auto x = randomReal< double >( N , g );
  const std::vector< LD > xl( x.begin() , x.end() );
  for( unsigned t = 1 ; t <= 4 ; ++t )
   for( bool c : { true , false } ) {
    if( t == 1 && c && N < 2 ) continue;
    auto y = x;
    if( c ) dct( y , t );
    else dst( y , t );
    CHECK( error( y , trig( xl , c , t ) ) < 4 * bound< double >( N ) );
    }
  auto y = x;                                  // III of II: 2 N x
  dct( y , 2 );
  dct( y , 3 );
  for( std::size_t j = 0 ; j < N ; ++j )
   CHECK( std::abs( y[ j ] / double( 2 * N ) - x[ j ] ) < 1e-13 );
  y = x;                                       // IV of IV: 2 N x
  dst( y , 4 );
  dst( y , 4 );
  for( std::size_t j = 0 ; j < N ; ++j )
   CHECK( std::abs( y[ j ] / double( 2 * N ) - x[ j ] ) < 1e-13 );
  };
 for( std::size_t N = 1 ; N <= 64 ; ++N ) check( N );
 for( std::size_t N : { 101 , 128 , 255 , 1000 } ) check( N );
 std::vector< double > none;
 dct( none , 2 );
 dst( none , 3 );
 CHECK( none.empty() );
 std::vector< double > one( 1 , 3.0 );
 CHECK_THROWS( dct( one , 1 ) , contract_violation );
 CHECK_THROWS( dct( one , 5 ) , contract_violation );
 CHECK_THROWS( dst( one , 0 ) , contract_violation );
 }

// fftMany() against the transforms one at a time, for sizes with the small
// four steps, with codelets on the lanes, with neither, and numbers of
// transforms below and above the width of the lanes; fft2() and fftn()
// against the transforms along each axis, by rows and columns of every
// kind (short blocks, one row, one column, the unrolled ones of the powers
// of 2 up to 32), and in 3 dimensions against the definition at some
// outputs
ADS_TEST( "fft-many-nd" )
{
 std::mt19937 g( 67 );
 for( std::size_t n : { 1 , 2 , 3 , 7 , 8 , 12 , 16 , 60 , 64 , 100 , 128 ,
                        256 , 1000 , 4096 } )
  for( std::size_t q : { 0 , 1 , 3 , 4 , 5 , 9 } ) {
   const auto a = randomComplex< double >( n * q , g );
   for( bool inv : { false , true } ) {
    auto f = a;
    fftMany( f.data() , n , q , inv );
    for( std::size_t t = 0 ; t < q ; ++t ) {
     std::vector< Complex > r( a.begin() + t * n , a.begin() + ( t + 1 ) * n );
     fft( r , inv );
     for( std::size_t k = 0 ; k < n ; ++k )
      CHECK( std::abs( f[ t * n + k ] - r[ k ] ) < 1e-13 * double( n ) );
     }
    }
   }
 using Shape = std::pair< std::size_t , std::size_t >;
 for( auto [ n0 , n1 ] : std::vector< Shape >{
       { 1 , 1 } , { 1 , 8 } , { 8 , 1 } , { 4 , 4 } , { 3 , 5 } ,
       { 16 , 16 } , { 17 , 9 } , { 64 , 64 } , { 100 , 60 } , { 128 , 256 } ,
       { 256 , 128 } , { 1 , 100 } , { 100 , 1 } , { 0 , 5 } , { 4 , 8 } ,
       { 8 , 32 } , { 32 , 8 } , { 32 , 32 } , { 16 , 4 } } ) {
  const auto a = randomComplex< double >( n0 * n1 , g );
  for( bool inv : { false , true } ) {
   auto f = a , r = a;
   fft2( f.data() , n0 , n1 , inv );
   for( std::size_t i = 0 ; i < n0 ; ++i )
    fft( r.data() + i * n1 , n1 , inv );
   for( std::size_t j = 0 ; j < n1 ; ++j ) {
    std::vector< Complex > v( n0 );
    for( std::size_t i = 0 ; i < n0 ; ++i ) v[ i ] = r[ i * n1 + j ];
    fft( v , inv );
    for( std::size_t i = 0 ; i < n0 ; ++i ) r[ i * n1 + j ] = v[ i ];
    }
   for( std::size_t k = 0 ; k < a.size() ; ++k )
    CHECK( std::abs( f[ k ] - r[ k ] ) < 1e-13 * std::sqrt( double(
                                                             a.size() ) ) );
   }
  }
 const std::vector< std::size_t > d = { 6 , 5 , 8 };
 const auto a = randomComplex< double >( 240 , g );
 auto f = a;
 fftn( f.data() , d , false );
 for( std::size_t k : { 0 , 1 , 7 , 100 , 239 } ) {
  const std::size_t k0 = k / 40 , k1 = k / 8 % 5 , k2 = k % 8;
  CL s = 0;
  for( std::size_t j = 0 ; j < 240 ; ++j ) {
   const std::size_t j0 = j / 40 , j1 = j / 8 % 5 , j2 = j % 8;
   s += CL( a[ j ] ) * root( j0 * k0 , 6 , -1 ) * root( j1 * k1 , 5 , -1 ) *
        root( j2 * k2 , 8 , -1 );
   }
  CHECK( std::abs( CL( f[ k ] ) - s ) < 1e-12 );
  }
 fftn( f.data() , d , true );
 for( std::size_t k = 0 ; k < 240 ; ++k )
  CHECK( std::abs( f[ k ] - a[ k ] ) < 1e-14 );
 }

// convolve() in single precision against the product in double, its sizes
// those of the transforms of 2^a 3^b 5^c points, and the overload of
// double chosen for the lists of integers
ADS_TEST( "convolve-float" )
{
 std::mt19937 g( 71 );
 using Shape = std::pair< std::size_t , std::size_t >;
 for( auto [ np , nq ] : std::vector< Shape >{
       { 0 , 3 } , { 1 , 1 } , { 2 , 3 } , { 7 , 9 } , { 100 , 61 } ,
       { 500 , 500 } } ) {
  const auto p = randomReal< float >( np , g ) , q = randomReal< float >( nq ,
                                                                          g );
  const auto c = convolve( p , q );
  if( np == 0 || nq == 0 ) { CHECK( c.empty() ); continue; }
  CHECK( c.size() == np + nq - 1 );
  for( std::size_t k = 0 ; k < c.size() ; ++k ) {
   double s = 0;
   for( std::size_t i = 0 ; i < np ; ++i )
    if( k >= i && k - i < nq ) s += double( p[ i ] ) * double( q[ k - i ] );
   CHECK( std::abs( c[ k ] - s ) < 1e-5 * double( np + nq ) );
   }
  }
 const auto one = convolve( { 3 } , { -2 , 1 } );
 static_assert( std::is_same_v< decltype( one ) ,
                                const std::vector< double > > );
 CHECK( one.size() == 2 && std::abs( one[ 0 ] + 6 ) < 1e-12 &&
        std::abs( one[ 1 ] - 3 ) < 1e-12 );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ End File fft.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
