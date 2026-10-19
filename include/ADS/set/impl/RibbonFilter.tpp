/*--------------------------------------------------------------------------*/
/*------------------------- File RibbonFilter.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RibbonFilter [see RibbonFilter.h], with their
 * contracts.
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

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
RibbonFilter< T , F , H >::RibbonFilter( const std::vector< T > & keys ,
                                         std::uint64_t seed , H h )
 : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 ADS_PRE( keys.size() <= std::size_t( 0xFFFFFFFFU ) );
 if( keys.empty() )
  return;
 std::vector< std::uint64_t > B;
 B.reserve( keys.size() );
 for( const T & k : keys )
  B.push_back( base( k ) );
 // m = n ( 1 + eps ), eps = max( 0.01 , 0.0083 log2 n - 0.025 ), a
 // multiple of 64
 const double n = double( keys.size() );
 const double eps = std::max( 0.01 , 0.0083 * std::log2( n ) - 0.025 );
 f_m = std::max< std::size_t >( 64 , ( std::size_t( std::ceil(
                                 n * ( 1 + eps ) ) ) + 63 ) & ~std::size_t(
                                 63 ) );
 const std::size_t grow = ( keys.size() / 50 + 63 ) & ~std::size_t( 63 );
 for( ; ; ) {
  if( ++f_trials > kMaxTrials )
   throw std::runtime_error( "RibbonFilter: no seed solves the system" );
  f_seed = hashWord( f_seed , f_trials );   // a new seed at each trial
  if( build( B ) )
   break;
  if( f_trials % 4 == 0 )
   f_m += std::max< std::size_t >( 64 , grow );
  }
 ADS_AUDIT( std::all_of( keys.begin() , keys.end() ,
                         [ this ]( const T & k ) { return( maybe( k ) ); } ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
std::uint64_t RibbonFilter< T , F , H >::base( const T & x ) const
{
 if constexpr( std::is_integral_v< T > &&
               std::is_same_v< H , FastHash< T > > )
  return( std::uint64_t( x ) );           // mixed once, with the seed
 else if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( x ) ) );
 else
  return( hashWord( std::uint64_t( f_h( x ) ) ) );
 }

template< class T , class F , class H >
std::uint64_t RibbonFilter< T , F , H >::mixed( std::uint64_t b ) const
{
 return( foldedProduct( b ^ f_seed , 0x9E3779B97F4A7C15ULL ) );
 }

template< class T , class F , class H >
std::size_t RibbonFilter< T , F , H >::start( std::uint64_t h ) const
{
 return( std::size_t( product128( h , f_m - 63 ).hi ) );
 }

template< class T , class F , class H >
std::uint64_t RibbonFilter< T , F , H >::coefficients( std::uint64_t h )
{
 return( foldedProduct( h , 0xC2B2AE3D27D4EB4FULL ) | 1 );
 }

template< class T , class F , class H >
F RibbonFilter< T , F , H >::fingerprint( std::uint64_t h )
{
 return( F( h ^ ( h >> 32 ) ) );
 }

template< class T , class F , class H >
bool RibbonFilter< T , F , H >::build(
                                   const std::vector< std::uint64_t > & B )
{
 const std::size_t m = f_m;
 // the banding: row i holds the equation whose first coefficient is in
 // column i, 0 if none
 std::vector< std::uint64_t > C( m , 0 );
 std::vector< F > R( m , F( 0 ) );
 for( std::uint64_t b : B ) {
  const std::uint64_t h = mixed( b );
  std::size_t s = start( h );
  std::uint64_t c = coefficients( h );
  F r = fingerprint( h );
  for( ; ; ) {
   const std::uint64_t d = C[ s ];
   if( d == 0 ) {
    C[ s ] = c;
    R[ s ] = r;
    break;
    }
   c ^= d;
   r = F( r ^ R[ s ] );
   if( c == 0 ) {
    if( r != 0 )
     return( false );                   // inconsistent
    break;                              // redundant, a repeated key
    }
   const int z = std::countr_zero( c );
   s += std::size_t( z );
   c >>= z;
   }
  }
 // the back substitution from the last row: W[ j ] holds the bits of
 // the column j in the 64 rows from the current one
 f_Z.assign( ( m / 64 + 1 ) * kR , 0 );
 std::uint64_t W[ kR ] = {};
 for( std::size_t i = m ; i-- > 0 ; ) {
  const std::uint64_t c = C[ i ];
  const std::uint64_t r = c != 0 ? std::uint64_t( R[ i ] ) :
                                   hashWord( i , f_seed );   // a free row
  for( int j = 0 ; j < kR ; ++j ) {
   const std::uint64_t w = W[ j ] << 1;
   W[ j ] = w | ( ( ( r >> j ) ^ std::uint64_t( std::popcount( c & w ) ) )
                  & 1 );
   }
  if( ( i & 63 ) == 0 )
   std::copy( W , W + kR , f_Z.begin() + std::ptrdiff_t( i / 64 * kR ) );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
bool RibbonFilter< T , F , H >::maybe( const T & x ) const
{
 if( f_m == 0 )
  return( false );
 const std::uint64_t h = mixed( base( x ) );
 const std::size_t s = start( h );
 const std::uint64_t c = coefficients( h );
 const int o = int( s & 63 );
 const std::uint64_t lo = c << o , hi = ( c >> 1 ) >> ( 63 - o );
 const std::uint64_t * u = f_Z.data() + s / 64 * kR;
 std::uint32_t y = 0;
 for( int j = 0 ; j < kR ; ++j )
  y |= std::uint32_t( std::popcount( ( lo & u[ j ] ) ^
                                     ( hi & u[ kR + j ] ) ) & 1 ) << j;
 return( F( y ) == fingerprint( h ) );
 }

template< class T , class F , class H >
std::size_t RibbonFilter< T , F , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class F , class H >
bool RibbonFilter< T , F , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class F , class H >
std::size_t RibbonFilter< T , F , H >::bits( void ) const
{
 return( 64 * f_Z.size() );
 }

template< class T , class F , class H >
std::size_t RibbonFilter< T , F , H >::rows( void ) const
{
 return( f_m );
 }

template< class T , class F , class H >
std::size_t RibbonFilter< T , F , H >::trials( void ) const
{
 return( f_trials );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RibbonFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
