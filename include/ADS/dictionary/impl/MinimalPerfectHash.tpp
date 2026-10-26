/*--------------------------------------------------------------------------*/
/*---------------------- File MinimalPerfectHash.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinimalPerfectHash [see MinimalPerfectHash.h], with
 * their contracts.
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

namespace detail {

/// the odd constants of the two hashes and of the pilots
inline constexpr std::uint64_t kPilotK[ 3 ] = {
 0x9E3779B97F4A7C15ULL , 0xC2B2AE3D27D4EB4FULL , 0x165667B19E3779F9ULL };

/// the high half of the product a b on 128 bits, a place of [ 0 , b )
[[nodiscard]] constexpr std::uint64_t mulHigh( std::uint64_t a ,
                                               std::uint64_t b )
{
 return( product128( a , b ).hi );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , class H >
MinimalPerfectHash< T , H >::MinimalPerfectHash(
                              const std::vector< T > & keys , double lambda ,
                              double alpha , std::uint64_t seed , H h )
 : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 ADS_PRE( ( keys.size() < ( std::size_t( 1 ) << 32 ) ) && ( lambda >= 1 ) &&
          ( alpha >= 0.5 ) && ( alpha <= 1 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > S( keys );
  std::sort( S.begin() , S.end() );
  return( std::adjacent_find( S.begin() , S.end() ) == S.end() );
  }() );
 f_m = std::max( f_n , std::size_t( std::ceil( double( f_n ) / alpha ) ) );
 const auto nb = std::max< std::size_t >( 2 , std::size_t( std::ceil(
                                          double( f_n ) / lambda ) ) );
 f_dense = std::max< std::size_t >( 1 , std::size_t( 0.3 * double( nb ) ) );
 f_sparse = nb - f_dense;
 std::vector< std::uint64_t > B;
 B.reserve( f_n );
 for( const T & k : keys )
  B.push_back( base( k ) );
 for( ; ; ) {
  if( ++f_trials > kMaxTrials )
   throw std::runtime_error( "MinimalPerfectHash: no seed separates the "
                             "keys" );
  if( build( B ) )
   break;
  f_seed = hashWord( f_seed , f_trials );   // a new seed
  }
 ADS_AUDIT( [ & ]( void ) {
  std::vector< char > seen( f_n , 0 );
  for( const T & k : keys ) {
   const std::size_t v = ( *this )( k );
   if( ( v >= f_n ) || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( true );
  }() );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::uint64_t MinimalPerfectHash< T , H >::base( const T & k ) const
{
 if constexpr( std::is_integral_v< T > &&
               std::is_same_v< H , FastHash< T > > )
  return( std::uint64_t( k ) );           // mixed by the two hashes
 else if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::bucket( std::uint64_t h ) const
{
 // 60 % of the hashes into the first 30 % of the buckets, each part by the
 // product of its size and of h rotated, so that the test and the place
 // in the part use different bits
 constexpr std::uint64_t T60 = 0x9999999999999999ULL;   // 0.6 2^64
 const std::uint64_t r = std::rotl( h , 32 );
 return( h < T60 ? std::size_t( detail::mulHigh( r , f_dense ) ) :
                   f_dense + std::size_t( detail::mulHigh( r , f_sparse ) ) );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::place( std::uint64_t h2 ,
                                                std::uint64_t k ) const
{
 return( std::size_t( detail::mulHigh( foldedProduct(
          h2 ^ ( k * detail::kPilotK[ 2 ] ) , detail::kPilotK[ 0 ] ) ,
          f_m ) ) );
 }

template< class T , class H >
std::uint64_t MinimalPerfectHash< T , H >::pilot( std::size_t b ) const
{
 // the f_w bits from the bit b f_w, over two words at most (the vector
 // has one word more)
 const std::size_t at = b * std::size_t( f_w );
 const std::size_t i = at >> 6;
 const unsigned s = unsigned( at & 63 );
 const std::uint64_t lo = f_P[ i ] >> s ,
                     hi = ( f_P[ i + 1 ] << 1 ) << ( 63 - s );
 return( ( lo | hi ) & ( ( std::uint64_t( 1 ) << f_w ) - 1 ) );
 }

template< class T , class H >
bool MinimalPerfectHash< T , H >::build(
                                   const std::vector< std::uint64_t > & B )
{
 const std::size_t n = f_n , m = f_m , nb = f_dense + f_sparse;
 // the second hashes grouped by bucket, by a counting sort
 std::vector< std::uint32_t > start( nb + 1 , 0 );
 auto b1 = std::make_unique_for_overwrite< std::uint32_t[] >( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  b1[ i ] = std::uint32_t( bucket( foldedProduct( B[ i ] ^ f_seed ,
                                                  detail::kPilotK[ 0 ] ) ) );
  ++start[ b1[ i ] + 1 ];
  }
 for( std::size_t b = 0 ; b < nb ; ++b )
  start[ b + 1 ] += start[ b ];
 auto h2 = std::make_unique_for_overwrite< std::uint64_t[] >( n );
 {
  std::vector< std::uint32_t > next( start.begin() , start.end() - 1 );
  for( std::size_t i = 0 ; i < n ; ++i )
   h2[ next[ b1[ i ] ]++ ] = foldedProduct( B[ i ] ^ f_seed ,
                                            detail::kPilotK[ 1 ] );
  }
 // the buckets by decreasing size, by a counting sort of the sizes
 std::size_t smax = 0;
 for( std::size_t b = 0 ; b < nb ; ++b )
  smax = std::max< std::size_t >( smax , start[ b + 1 ] - start[ b ] );
 std::vector< std::uint32_t > bySize( smax + 2 , 0 );
 for( std::size_t b = 0 ; b < nb ; ++b )
  ++bySize[ smax - ( start[ b + 1 ] - start[ b ] ) + 1 ];
 for( std::size_t s = 0 ; s <= smax ; ++s )
  bySize[ s + 1 ] += bySize[ s ];
 auto order = std::make_unique_for_overwrite< std::uint32_t[] >( nb );
 for( std::size_t b = 0 ; b < nb ; ++b )
  order[ bySize[ smax - ( start[ b + 1 ] - start[ b ] ) ]++ ] =
   std::uint32_t( b );
 // the search of the pilots, the places taken in a vector of bits
 std::vector< std::uint64_t > taken( m / 64 + 1 , 0 );
 std::vector< std::uint64_t > pilots( nb , 0 );
 std::vector< std::size_t > P( smax );
 std::uint64_t kmax = 0;
 for( std::size_t o = 0 ; o < nb ; ++o ) {
  const std::size_t b = order[ o ] , s = start[ b + 1 ] - start[ b ];
  if( s == 0 )
   break;                                 // the empty buckets are last
  const std::uint64_t * K = h2.get() + start[ b ];
  if( s > 1 ) {                           // equal hashes never separate
   std::vector< std::uint64_t > E( K , K + s );
   std::sort( E.begin() , E.end() );
   if( std::adjacent_find( E.begin() , E.end() ) != E.end() )
    return( false );
   }
  std::uint64_t k = 0;
  for( ; ; ++k ) {
   if( k == kMaxPilot )
    return( false );
   std::size_t i = 0;
   for( ; i < s ; ++i ) {
    const std::size_t p = place( K[ i ] , k );
    const std::uint64_t bit = std::uint64_t( 1 ) << ( p & 63 );
    if( taken[ p >> 6 ] & bit )
     break;
    taken[ p >> 6 ] |= bit;               // also against the bucket itself
    P[ i ] = p;
    }
   if( i == s )
    break;
   while( i-- > 0 )                       // the places of a failed pilot
    taken[ P[ i ] >> 6 ] &= ~( std::uint64_t( 1 ) << ( P[ i ] & 63 ) );
   }
  pilots[ b ] = k;
  kmax = std::max( kmax , k );
  }
 // the free places below n of the keys placed beyond it
 f_free.assign( m - n , 0 );
 auto isTaken = [ & ]( std::size_t p ) {
  return( ( taken[ p >> 6 ] >> ( p & 63 ) ) & 1 ); };
 for( std::size_t p = n , f = 0 ; p < m ; ++p )
  if( isTaken( p ) ) {
   while( isTaken( f ) )
    ++f;
   f_free[ p - n ] = std::uint32_t( f++ );
   }
 // the pilots, f_w bits each
 f_w = int( std::bit_width( kmax ) );
 f_P.assign( ( nb * std::size_t( f_w ) ) / 64 + 2 , 0 );
 if( f_w > 0 )
  for( std::size_t b = 0 ; b < nb ; ++b ) {
   const std::size_t at = b * std::size_t( f_w );
   f_P[ at >> 6 ] |= pilots[ b ] << ( at & 63 );
   if( ( at & 63 ) + std::size_t( f_w ) > 64 )
    f_P[ ( at >> 6 ) + 1 ] |= pilots[ b ] >> ( 64 - ( at & 63 ) );
   }
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::operator()( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 const std::uint64_t x = base( k ) ^ f_seed;
 const std::size_t p = place( foldedProduct( x , detail::kPilotK[ 1 ] ) ,
                              pilot( bucket( foldedProduct( x ,
                               detail::kPilotK[ 0 ] ) ) ) );
 return( p < f_n ? p : f_free[ p - f_n ] );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class H >
bool MinimalPerfectHash< T , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::bits( void ) const
{
 return( 64 * f_P.size() + 32 * f_free.size() + 8 * sizeof( *this ) );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::buckets( void ) const
{
 return( f_dense + f_sparse );
 }

template< class T , class H >
std::size_t MinimalPerfectHash< T , H >::trials( void ) const
{
 return( f_trials );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File MinimalPerfectHash.tpp --------------------*/
/*--------------------------------------------------------------------------*/
