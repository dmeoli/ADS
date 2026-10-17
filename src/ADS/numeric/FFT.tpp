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
 const double sgn = inverse ? 1 : -1;
 std::vector< Complex > w( n / 2 );         // omega^k, each computed alone
 for( std::size_t k = 0 ; k < n / 2 ; ++k )
  w[ k ] = std::polar( 1.0 , sgn * 2 * std::numbers::pi * double( k ) /
                             double( n ) );
 for( std::size_t len = 2 ; len <= n ; len *= 2 ) {   // the butterflies
  const std::size_t step = n / len;
  for( std::size_t i = 0 ; i < n ; i += len )
   for( std::size_t k = 0 ; k < len / 2 ; ++k ) {
    const Complex u = a[ i + k ] , v = a[ i + k + len / 2 ] * w[ k * step ];
    a[ i + k ] = u + v;
    a[ i + k + len / 2 ] = u - v;
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
 for( std::size_t i = 0 ; i < n ; ++i ) A[ i ] *= B[ i ];
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
