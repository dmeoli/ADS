/*--------------------------------------------------------------------------*/
/*-------------------- File FingerprintPerfectHash.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FingerprintPerfectHash [see FingerprintPerfectHash.h],
 * with their contracts.
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

/// the constants of the levels, the outputs of SplitMix64 from 0, odd
inline constexpr auto kLevelK = []( void ) {
 std::array< std::uint64_t , 64 > k{};
 std::uint64_t x = 0;
 for( auto & w : k ) {
  x += 0x9E3779B97F4A7C15ULL;
  std::uint64_t z = x;
  z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
  z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
  w = ( z ^ ( z >> 31 ) ) | 1;
  }
 return( k );
 }();

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , class H >
FingerprintPerfectHash< T , H >::FingerprintPerfectHash(
                                  const std::vector< T > & keys ,
                                  double gamma , std::uint64_t seed , H h )
 : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 ADS_PRE( ( keys.size() < ( std::size_t( 1 ) << 32 ) ) && ( gamma >= 1 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > S( keys );
  std::sort( S.begin() , S.end() );
  return( std::adjacent_find( S.begin() , S.end() ) == S.end() );
  }() );
 std::vector< std::uint64_t > X;           // the keys still to place
 X.reserve( f_n );
 for( const T & k : keys )
  X.push_back( base( k ) );
 std::vector< std::uint64_t > C;           // the places met twice
 for( std::size_t i = 0 ; ! X.empty() ; ++i ) {
  if( i == kMaxLevels ) {
   std::sort( X.begin() , X.end() );
   if( std::adjacent_find( X.begin() , X.end() ) != X.end() )
    throw std::runtime_error( "FingerprintPerfectHash: two keys have the "
                              "same hash" );
   throw std::runtime_error( "FingerprintPerfectHash: too many levels" );
   }
  const std::size_t m = std::max< std::size_t >( kBlock ,
   ( std::size_t( std::ceil( gamma * double( X.size() ) ) ) + kBlock - 1 ) /
   kBlock * kBlock );
  const std::size_t at = f_bits.size();
  f_at.push_back( 64 * at );
  f_m.push_back( m );
  f_bits.resize( at + m / 64 , 0 );
  C.assign( m / 64 , 0 );
  std::uint64_t * A = f_bits.data() + at;
  for( std::uint64_t x : X ) {             // the places, once or more
   const std::size_t p = place( x , i , m );
   const std::uint64_t b = std::uint64_t( 1 ) << ( p & 63 );
   C[ p >> 6 ] |= A[ p >> 6 ] & b;
   A[ p >> 6 ] |= b;
   }
  for( std::size_t w = 0 ; w < m / 64 ; ++w )
   A[ w ] &= ~C[ w ];
  std::size_t j = 0;                       // the keys met, to the next
  for( std::uint64_t x : X ) {
   const std::size_t p = place( x , i , m );
   if( ( C[ p >> 6 ] >> ( p & 63 ) ) & 1 )
    X[ j++ ] = x;
   }
  X.resize( j );
  }
 f_rank.resize( f_bits.size() / 8 + 1 );
 std::size_t r = 0;
 for( std::size_t b = 0 ; b < f_bits.size() / 8 ; ++b ) {
  f_rank[ b ] = std::uint32_t( r );
  for( std::size_t w = 8 * b ; w < 8 * b + 8 ; ++w )
   r += std::size_t( std::popcount( f_bits[ w ] ) );
  }
 f_rank.back() = std::uint32_t( r );
 ADS_AUDIT( [ & ]( void ) {
  if( r != f_n )
   return( false );
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
std::uint64_t FingerprintPerfectHash< T , H >::base( const T & k ) const
{
 std::uint64_t x;
 if constexpr( std::is_integral_v< T > &&
               std::is_same_v< H , FastHash< T > > )
  x = std::uint64_t( k );                   // mixed by the levels
 else if constexpr( requires { typename H::is_avalanching; } )
  x = std::uint64_t( f_h( k ) );
 else
  x = hashWord( std::uint64_t( f_h( k ) ) );
 return( x ^ f_seed );
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::place( std::uint64_t x ,
                                                    std::size_t i ,
                                                    std::size_t m )
{
 const std::uint64_t c = detail::kLevelK[ i ];
 return( std::size_t( product128( foldedProduct( x ^ ( c >> 1 ) , c ) ,
                                  m ).hi ) );
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::rank( std::size_t g ) const
{
 const std::size_t w = g >> 6;
 std::size_t r = f_rank[ g / kBlock ];
 for( std::size_t v = w & ~std::size_t( 7 ) ; v < w ; ++v )
  r += std::size_t( std::popcount( f_bits[ v ] ) );
 return( r + std::size_t( std::popcount(
          f_bits[ w ] & ( ( std::uint64_t( 1 ) << ( g & 63 ) ) - 1 ) ) ) );
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::operator()( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 const std::uint64_t x = base( k );
 for( std::size_t i = 0 ; i < f_m.size() ; ++i ) {
  const std::size_t g = f_at[ i ] + place( x , i , f_m[ i ] );
  if( ( f_bits[ g >> 6 ] >> ( g & 63 ) ) & 1 )
   return( rank( g ) );
  }
 return( std::size_t( product128( hashWord( x ) , f_n ).hi ) );   // not in S
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class H >
bool FingerprintPerfectHash< T , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::bits( void ) const
{
 return( 64 * f_bits.size() + 32 * f_rank.size() +
         8 * sizeof( std::size_t ) * ( f_at.size() + f_m.size() ) +
         8 * sizeof( *this ) );
 }

template< class T , class H >
std::size_t FingerprintPerfectHash< T , H >::levels( void ) const
{
 return( f_m.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File FingerprintPerfectHash.tpp ------------------*/
/*--------------------------------------------------------------------------*/
