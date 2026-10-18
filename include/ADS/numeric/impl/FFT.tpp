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

/// the powers of omega of all the rounds of the transform of size n (the
/// inverse one if inverse), the round of length len at the places
/// len / 2 , ... , len - 1, so that a round reads them in a row; each one
/// is computed alone, and the table of the last size is kept per thread,
/// since a computation costs n / 2 sines and cosines
inline const std::vector< Complex > & twiddles( std::size_t n ,
                                                bool inverse )
{
 thread_local std::size_t size = 0;
 thread_local bool inv = false;
 thread_local std::vector< Complex > T;
 if( ( size != n ) || ( inv != inverse ) ) {
  const double sgn = inverse ? 1 : -1;
  T.assign( n , Complex() );
  for( std::size_t len = 2 ; len <= n ; len *= 2 )
   for( std::size_t k = 0 ; k < len / 2 ; ++k )
    T[ len / 2 + k ] = std::polar( 1.0 , sgn * 2 * std::numbers::pi *
                                         double( k ) / double( len ) );
  size = n;
  inv = inverse;
  }
 return( T );
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
 if( n <= 1 ) return;
 for( std::size_t i = 1 , j = 0 ; i < n ; ++i ) {   // the reversed bits
  std::size_t bit = n >> 1;
  for( ; j & bit ; bit >>= 1 ) j ^= bit;
  j ^= bit;
  if( i < j ) std::swap( a[ i ] , a[ j ] );
  }
 const auto & T = detail::twiddles( n , inverse );
 for( std::size_t len = 2 ; len <= n ; len *= 2 ) {   // the butterflies
  const std::size_t h = len / 2;
  const Complex * w = T.data() + h;         // the powers of this round
  for( std::size_t i = 0 ; i < n ; i += len ) {
   Complex * x = a.data() + i , * y = x + h;
   for( std::size_t k = 0 ; k < h ; ++k ) {
    const Complex u = x[ k ] , v = detail::times( y[ k ] , w[ k ] );
    x[ k ] = u + v;
    y[ k ] = u - v;
    }
   }
  }
 if( inverse )
  for( auto & x : a ) x /= double( n );
 }

inline std::vector< double > convolve( const std::vector< double > & p ,
                                       const std::vector< double > & q )
{
 if( p.empty() || q.empty() ) return( std::vector< double >() );
 const std::size_t m = p.size() + q.size() - 1;
 std::size_t n = 1;
 while( n < m ) n *= 2;
 std::vector< Complex > A( n ) , B( n );
 for( std::size_t i = 0 ; i < p.size() ; ++i ) A[ i ] = p[ i ];
 for( std::size_t i = 0 ; i < q.size() ; ++i ) B[ i ] = q[ i ];
 fft( A , false );
 fft( B , false );
 for( std::size_t i = 0 ; i < n ; ++i )
  A[ i ] = detail::times( A[ i ] , B[ i ] );
 fft( A , true );
 std::vector< double > r( m );
 for( std::size_t i = 0 ; i < m ; ++i ) r[ i ] = A[ i ].real();
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File FFT.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
