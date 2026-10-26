/*--------------------------------------------------------------------------*/
/*------------------------- File RibbonFilter.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RibbonFilter [see RibbonFilter.h], with their
 * contracts.
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

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
RibbonFilter< T , F , H >::RibbonFilter( const std::vector< T > & keys ,
                                         std::uint64_t seed , H h )
 : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 ADS_PRE( keys.size() <= std::size_t( 0xFFFFFFFFU ) );
 if( keys.empty() )
  return;
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
  if( build( keys ) )
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
 return( ( h * 0xC2B2AE3D27D4EB4FULL ) | 1 );
 }

template< class T , class F , class H >
F RibbonFilter< T , F , H >::fingerprint( std::uint64_t h )
{
 return( F( h ^ ( h >> 32 ) ) );
 }

template< class T , class F , class H >
std::uint32_t RibbonFilter< T , F , H >::parities( const std::uint64_t * u ,
                                                   std::uint64_t lo ,
                                                   std::uint64_t hi )
{
#if defined( __AVX512BW__ )
 // ( u_j & lo ) ^ ( v_j & hi ) in the 8 lanes, the parity of each byte by
 // a table of the 16 nibbles, the sum of the 8 bytes of a lane, its bit 0
 const __m512i x = _mm512_ternarylogic_epi64( _mm512_and_si512(
                    _mm512_load_si512( u ) , _mm512_set1_epi64(
                    std::int64_t( lo ) ) ) , _mm512_load_si512( u + kR ) ,
                    _mm512_set1_epi64( std::int64_t( hi ) ) , 0x78 );
 const __m512i nib = _mm512_set1_epi8( 0x0F );
 const __m512i odd = _mm512_broadcast_i32x4( _mm_setr_epi8( 0 , 1 , 1 , 0 ,
                      1 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 1 , 1 , 0 ) );
 const __m512i p = _mm512_shuffle_epi8( odd , _mm512_and_si512(
                    _mm512_xor_si512( x , _mm512_srli_epi64( x , 4 ) ) ,
                    nib ) );
 return( std::uint32_t( _mm512_test_epi64_mask( _mm512_sad_epu8( p ,
          _mm512_setzero_si512() ) , _mm512_set1_epi64( 1 ) ) ) );
#elif defined( __AVX2__ )
 const __m256i L = _mm256_set1_epi64x( std::int64_t( lo ) ) ,
               G = _mm256_set1_epi64x( std::int64_t( hi ) );
 const __m256i nib = _mm256_set1_epi8( 0x0F );
 const __m256i odd = _mm256_setr_epi8( 0 , 1 , 1 , 0 , 1 , 0 , 0 , 1 , 1 ,
  0 , 0 , 1 , 0 , 1 , 1 , 0 , 0 , 1 , 1 , 0 , 1 , 0 , 0 , 1 , 1 , 0 , 0 ,
  1 , 0 , 1 , 1 , 0 );
 std::uint32_t y = 0;
 for( int k = 0 ; k < 2 ; ++k ) {
  const auto * a = reinterpret_cast< const __m256i * >( u + 4 * k );
  const auto * b = reinterpret_cast< const __m256i * >( u + kR + 4 * k );
  const __m256i x = _mm256_xor_si256( _mm256_and_si256(
                     _mm256_load_si256( a ) , L ) , _mm256_and_si256(
                     _mm256_load_si256( b ) , G ) );
  const __m256i p = _mm256_shuffle_epi8( odd , _mm256_and_si256(
                     _mm256_xor_si256( x , _mm256_srli_epi64( x , 4 ) ) ,
                     nib ) );
  const __m256i c = _mm256_slli_epi64( _mm256_sad_epu8( p ,
                     _mm256_setzero_si256() ) , 63 );
  y |= std::uint32_t( _mm256_movemask_pd( _mm256_castsi256_pd( c ) ) )
       << ( 4 * k );
  }
 return( y );
#else
 std::uint32_t y = 0;
 for( int j = 0 ; j < 8 ; ++j )
  y |= std::uint32_t( std::popcount( ( lo & u[ j ] ) ^
                                     ( hi & u[ kR + j ] ) ) & 1 ) << j;
 return( y );
#endif
 }

template< class T , class F , class H >
bool RibbonFilter< T , F , H >::build( const std::vector< T > & keys )
{
 const std::size_t m = f_m , n = keys.size();
 // the banding: row i holds the equation whose first coefficient is in
 // column i, 0 if none; the rows of the key kAhead places ahead are
 // prefetched, their hashes kept in a ring
 std::vector< std::uint64_t > C( m , 0 );
 std::vector< F > R( m , F( 0 ) );
 std::uint64_t ring[ kAhead ];
 auto ahead = [ & ]( std::size_t i ) {
  const std::uint64_t h = mixed( base( keys[ i ] ) );
  const std::size_t s = start( h );
  ADS_FILTER_PREFETCH( C.data() + s );
  ADS_FILTER_PREFETCH( R.data() + s );
  ring[ i % kAhead ] = h;
  };
 for( std::size_t i = 0 ; i < std::min( n , kAhead ) ; ++i )
  ahead( i );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t h = ring[ i % kAhead ];
  if( i + kAhead < n )
   ahead( i + kAhead );
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
 f_Z.assign( ( m / 64 + 1 ) * kR / 8 , Line{} );
 solve( C , R );
 return( true );
 }

template< class T , class F , class H >
void RibbonFilter< T , F , H >::solve( const std::vector< std::uint64_t > & C ,
                                       const std::vector< F > & R )
{
 // the back substitution from the last row; the right side of a row with
 // no equation is pseudorandom
 const std::size_t m = f_m;
 std::uint64_t * Z = f_Z.front().w;
 auto side = [ & ]( std::size_t i ) {
  return( C[ i ] != 0 ? std::uint64_t( R[ i ] ) : hashWord( i , f_seed ) );
  };
#if defined( __AVX512BW__ )
 // W[ g ] holds, in its lane j, the bits of the column 8 g + j in the 64
 // rows from i; 4 rows a time, the parities of the 4 rows taken from the
 // old W at once and the bits of the new rows among them added after
 constexpr int kG = kR / 8;
 __m512i W[ kG ];
 for( int g = 0 ; g < kG ; ++g )
  W[ g ] = _mm512_setzero_si512();
 const __m512i nib = _mm512_set1_epi8( 0x0F );
 const __m512i odd = _mm512_broadcast_i32x4( _mm_setr_epi8( 0 , 1 , 1 , 0 ,
                      1 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 1 , 1 , 0 ) );
 auto par = [ & ]( __m512i w , std::uint64_t c ) {
  const __m512i x = _mm512_and_si512( w , _mm512_set1_epi64(
                                          std::int64_t( c ) ) );
  const __m512i p = _mm512_shuffle_epi8( odd , _mm512_and_si512(
                     _mm512_xor_si512( x , _mm512_srli_epi64( x , 4 ) ) ,
                     nib ) );
  return( std::uint32_t( _mm512_test_epi64_mask( _mm512_sad_epu8( p ,
           _mm512_setzero_si512() ) , _mm512_set1_epi64( 1 ) ) ) );
  };
 auto bit = []( std::uint64_t c , int k ) {
  return( std::uint32_t( 0 ) - std::uint32_t( ( c >> k ) & 1 ) );
  };
 for( std::size_t i = m ; i > 0 ; ) {
  // the rows i - 1 to i - 4: row i - d sees the old W from its bit d
  const std::uint64_t c1 = C[ i - 1 ] , c2 = C[ i - 2 ] , c3 = C[ i - 3 ] ,
                      c4 = C[ i - 4 ];
  const std::uint64_t r1 = side( i - 1 ) , r2 = side( i - 2 ) ,
                      r3 = side( i - 3 ) , r4 = side( i - 4 );
  i -= 4;
  for( int g = 0 ; g < kG ; ++g ) {
   const int o = 8 * g;
   const std::uint32_t b1 = ( std::uint32_t( r1 >> o ) & 255 ) ^
                            par( W[ g ] , c1 >> 1 );
   const std::uint32_t b2 = ( std::uint32_t( r2 >> o ) & 255 ) ^
                            par( W[ g ] , c2 >> 2 ) ^ ( b1 & bit( c2 , 1 ) );
   const std::uint32_t b3 = ( std::uint32_t( r3 >> o ) & 255 ) ^
                            par( W[ g ] , c3 >> 3 ) ^ ( b2 & bit( c3 , 1 ) ) ^
                            ( b1 & bit( c3 , 2 ) );
   const std::uint32_t b4 = ( std::uint32_t( r4 >> o ) & 255 ) ^
                            par( W[ g ] , c4 >> 4 ) ^ ( b3 & bit( c4 , 1 ) ) ^
                            ( b2 & bit( c4 , 2 ) ) ^ ( b1 & bit( c4 , 3 ) );
   __m512i w = _mm512_slli_epi64( W[ g ] , 4 );
   w = _mm512_mask_or_epi64( w , __mmask8( b1 ) , w ,
                             _mm512_set1_epi64( 8 ) );
   w = _mm512_mask_or_epi64( w , __mmask8( b2 ) , w ,
                             _mm512_set1_epi64( 4 ) );
   w = _mm512_mask_or_epi64( w , __mmask8( b3 ) , w ,
                             _mm512_set1_epi64( 2 ) );
   W[ g ] = _mm512_mask_or_epi64( w , __mmask8( b4 ) , w ,
                                  _mm512_set1_epi64( 1 ) );
   }
  if( ( i & 63 ) == 0 )
   for( int g = 0 ; g < kG ; ++g )
    _mm512_store_si512( Z + i / 64 * kR + 8 * std::size_t( g ) , W[ g ] );
  }
#else
 // W[ j ] holds the bits of the column j in the 64 rows from i
 std::uint64_t W[ kR ] = {};
 for( std::size_t i = m ; i-- > 0 ; ) {
  const std::uint64_t c = C[ i ] , r = side( i );
  for( int j = 0 ; j < kR ; ++j ) {
   const std::uint64_t w = W[ j ] << 1;
   W[ j ] = w | ( ( ( r >> j ) ^ std::uint64_t( std::popcount( c & w ) ) )
                  & 1 );
   }
  if( ( i & 63 ) == 0 )
   for( int j = 0 ; j < kR ; ++j )
    Z[ i / 64 * kR + std::size_t( j ) ] = W[ j ];
  }
#endif
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
ADS_FILTER_INLINE bool RibbonFilter< T , F , H >::maybe( const T & x ) const
{
 if( f_m == 0 )
  return( false );
 const std::uint64_t h = mixed( base( x ) );
 const std::size_t s = start( h );
 const std::uint64_t c = coefficients( h );
 const int o = int( s & 63 );
 const std::uint64_t lo = c << o , hi = ( c >> 1 ) >> ( 63 - o );
 const std::uint64_t * u = f_Z.front().w + s / 64 * kR;
 const auto f = std::uint32_t( fingerprint( h ) );
 // 8 columns at a time, one line of each block: a key out of the set
 // stops at the first line with probability 1 - 2^-8
 for( int g = 0 ; g < kR / 8 ; ++g )
  if( parities( u + 8 * g , lo , hi ) != ( ( f >> ( 8 * g ) ) & 255 ) )
   return( false );
 return( true );
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
 return( 8 * sizeof( Line ) * f_Z.size() );
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
