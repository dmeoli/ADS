/*--------------------------------------------------------------------------*/
/*--------------------------- File RecSplit.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RecSplit [see RecSplit.h], with their contracts.
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

/// the odd constants of the two hashes of a key and of the hashes of the
/// nodes
inline constexpr std::uint64_t kRecSplitK[ 4 ] = {
 0x9E3779B97F4A7C15ULL , 0xC2B2AE3D27D4EB4FULL , 0xD6E8FEB86659FD93ULL ,
 0xFF51AFD7ED558CCDULL };

/// the parameter of Golomb and Rice of a geometric source of parameter p,
/// from ln p
inline std::uint8_t recSplitRice( double lnp )
{
 const double p = std::exp( lnp );
 if( p >= 1 )
  return( 0 );
 const double r = std::ceil( std::log2( std::log( ( 1 + std::sqrt( 5.0 ) )
                                                  / 2 ) /
                                        -std::log1p( -p ) ) );
 return( r <= 0 ? 0 : std::uint8_t( r ) );
 }

/// the tables of the sizes below mmax for the leaves of l keys and the
/// parts lower and upper
inline RecSplitTables recSplitTables( std::size_t l , std::size_t lower ,
                                      std::size_t upper , std::size_t mmax )
{
 RecSplitTables t;
 t.rice.assign( mmax , 0 );
 t.low.assign( mmax , 0 );
 t.nodes.assign( mmax , 0 );
 for( std::size_t m = 2 ; m < mmax ; ++m ) {
  const double dm = double( m );
  double lnp = std::lgamma( dm + 1 );
  std::uint32_t low = 0;
  std::uint16_t nodes = 1;
  auto part = [ & ]( std::size_t k ) {
   lnp += double( k ) * std::log( double( k ) / dm ) -
          std::lgamma( double( k ) + 1 );
   low += t.low[ k ];
   nodes = std::uint16_t( nodes + t.nodes[ k ] ); };
  if( m <= l )
   lnp -= dm * std::log( dm );           // a bijection, m! / m^m
  else if( m > upper ) {
   const std::size_t u = ( ( m / 2 + upper - 1 ) / upper ) * upper;
   part( u );
   part( m - u );
   }
  else
   for( std::size_t k = m , unit = m > lower ? lower : l ; k > 0 ;
        k -= std::min( k , unit ) )
    part( std::min( k , unit ) );
  t.rice[ m ] = recSplitRice( lnp );
  t.low[ m ] = low + t.rice[ m ];
  t.nodes[ m ] = nodes;
  }
 return( t );
 }

/// the tables of the leaves of l keys, computed once
template< std::size_t l , std::size_t lower , std::size_t upper ,
          std::size_t mmax >
const RecSplitTables & recSplitTablesOf( void )
{
 static const RecSplitTables t = recSplitTables( l , lower , upper , mmax );
 return( t );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , unsigned L , class H >
RecSplit< T , L , H >::RecSplit( const std::vector< T > & keys ,
                                 std::size_t b , std::uint64_t seed , H h )
 : f_seed( seed ) , f_n( keys.size() ) , f_t( &tables() ) , f_h( h )
{
 ADS_PRE( ( keys.size() < ( std::size_t( 1 ) << 32 ) ) && ( b >= 1 ) &&
          ( b <= 2000 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > S( keys );
  std::sort( S.begin() , S.end() );
  return( std::adjacent_find( S.begin() , S.end() ) == S.end() );
  }() );
 if( f_n == 0 )
  return;
 std::vector< std::uint64_t > B;
 B.reserve( f_n );
 for( const T & k : keys )
  B.push_back( base( k ) );
 for( ; ; ) {
  if( ++f_trials > kMaxTrials )
   throw std::runtime_error( "RecSplit: no seed separates the keys" );
  if( build( B , b ) )
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

template< class T , unsigned L , class H >
const detail::RecSplitTables & RecSplit< T , L , H >::tables( void )
{
 return( detail::recSplitTablesOf< L , kLower , kUpper , kMaxBucket >() );
 }

template< class T , unsigned L , class H >
constexpr std::size_t RecSplit< T , L , H >::left( std::size_t m )
{
 return( ( ( m / 2 + kUpper - 1 ) / kUpper ) * kUpper );
 }

template< class T , unsigned L , class H >
std::uint64_t RecSplit< T , L , H >::node( std::uint64_t h ,
                                           std::uint64_t x , unsigned d )
{
 // the salt s is a bijection of ( x , d ), and the multiplier depends on
 // it too, so that two keys keep no fixed relation from seed to seed
 const std::uint64_t s = ( x ^ ( std::uint64_t( d ) << 56 ) ) *
                         detail::kRecSplitK[ 2 ];
 return( foldedProduct( h ^ s , s ^ detail::kRecSplitK[ 3 ] ) );
 }

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::remap( std::uint64_t g , std::size_t m )
{
 return( std::size_t( product128( g , m ).hi ) );
 }

template< class T , unsigned L , class H >
std::uint64_t RecSplit< T , L , H >::base( const T & k ) const
{
 if constexpr( std::is_integral_v< T > &&
               std::is_same_v< H , FastHash< T > > )
  return( std::uint64_t( k ) );           // mixed by the two hashes
 else if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

template< class T , unsigned L , class H >
std::uint64_t RecSplit< T , L , H >::window( std::uint64_t at ) const
{
 const std::size_t i = std::size_t( at >> 6 );
 const unsigned s = unsigned( at & 63 );
 return( ( f_code[ i ] >> s ) | ( ( f_code[ i + 1 ] << 1 ) << ( 63 - s ) ) );
 }

template< class T , unsigned L , class H >
std::uint64_t RecSplit< T , L , H >::skipOnes( std::uint64_t at ,
                                               std::size_t c ) const
{
 while( c > 0 ) {
  const std::uint64_t w = window( at );
  const auto pc = std::size_t( std::popcount( w ) );
  if( pc >= c )
   return( at + std::uint64_t( selectInWord( w , int( c - 1 ) ) ) + 1 );
  c -= pc;
  at += 64;
  }
 return( at );
 }

/*--------------------------------------------------------------------------*/

template< class T , unsigned L , class H >
void RecSplit< T , L , H >::split( std::uint64_t * K , std::uint64_t * S ,
                                   std::size_t m , unsigned d ,
                                   std::vector< std::uint64_t > & seeds ,
                                   std::vector< std::uint8_t > & rice )
{
 if( m <= 1 )
  return;
 const auto & t = tables();
 if( m <= L ) {
  // rotation fitting: the masks of the places of A and of B, then the
  // rotations of the mask of B on the holes of the mask of A
  const std::uint64_t full = ( std::uint64_t( 1 ) << m ) - 1;
  for( std::uint64_t x = 0 ; ; ++x ) {
   std::uint64_t a = 0 , b = 0;
   std::size_t i = 0;
   for( ; i < m ; ++i ) {
    const std::uint64_t bit = std::uint64_t( 1 ) <<
                              remap( node( K[ i ] , x , d ) , m );
    std::uint64_t & set = ( K[ i ] >> 63 ) ? b : a;
    if( set & bit )
     break;
    set |= bit;
    }
   if( i < m )
    continue;
   for( std::size_t r = 0 ; r < m ; ++r ) {
    const std::uint64_t rb = r == 0 ? b :
                             ( ( b << r ) | ( b >> ( m - r ) ) ) & full;
    if( ( a & rb ) == 0 ) {
     seeds.push_back( x * m + r );
     rice.push_back( t.rice[ m ] );
     return;
     }
    }
   }
  }
 // the parts: two, of left( m ) and m - left( m ) keys, above kUpper; of
 // unit keys, the last one with the rest, below
 const std::size_t u = m > kUpper ? left( m ) : 0 ,
                   unit = m > kLower ? kLower : L ,
                   parts = m > kUpper ? 2 : ( m + unit - 1 ) / unit;
 auto partOf = [ & ]( std::size_t p ) -> std::size_t {
  if( m > kUpper )
   return( p < u ? 0 : 1 );
  return( m > kLower ? p / kLower : p / L ); };
 std::array< std::size_t , 32 > c{} , size{};
 for( std::size_t j = 0 ; j < parts ; ++j )
  size[ j ] = m > kUpper ? ( j == 0 ? u : m - u ) :
                           std::min( unit , m - j * unit );
 std::uint64_t x = 0;
 for( ; ; ++x ) {
  std::fill( c.begin() , c.begin() + parts , 0 );
  for( std::size_t i = 0 ; i < m ; ++i )
   ++c[ partOf( remap( node( K[ i ] , x , d ) , m ) ) ];
  if( std::equal( c.begin() , c.begin() + parts , size.begin() ) )
   break;
  }
 seeds.push_back( x );
 rice.push_back( t.rice[ m ] );
 // the keys grouped by part, in S and back
 std::array< std::size_t , 32 > at{};
 for( std::size_t j = 1 ; j < parts ; ++j )
  at[ j ] = at[ j - 1 ] + size[ j - 1 ];
 for( std::size_t i = 0 ; i < m ; ++i )
  S[ at[ partOf( remap( node( K[ i ] , x , d ) , m ) ) ]++ ] = K[ i ];
 std::copy( S , S + m , K );
 for( std::size_t j = 0 , from = 0 ; j < parts ; from += size[ j++ ] )
  split( K + from , S + from , size[ j ] , d + 1 , seeds , rice );
 }

template< class T , unsigned L , class H >
bool RecSplit< T , L , H >::build( const std::vector< std::uint64_t > & B ,
                                   std::size_t b )
{
 const std::size_t n = f_n;
 const std::size_t nb = f_nb = std::max< std::size_t >( 1 , ( n + b - 1 )
                                                            / b );
 // the second hashes grouped by bucket, by a counting sort
 std::vector< std::uint64_t > start( nb + 1 , 0 );
 auto b1 = std::make_unique_for_overwrite< std::uint32_t[] >( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  b1[ i ] = std::uint32_t( remap( foldedProduct( B[ i ] ^ f_seed ,
                                   detail::kRecSplitK[ 0 ] ) , nb ) );
  ++start[ b1[ i ] + 1 ];
  }
 std::size_t smax = 0;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  smax = std::max< std::size_t >( smax , start[ j + 1 ] );
  start[ j + 1 ] += start[ j ];
  }
 if( smax >= kMaxBucket )
  return( false );
 std::vector< std::uint64_t > h2( n );
 {
  std::vector< std::uint64_t > next( start.begin() , start.end() - 1 );
  for( std::size_t i = 0 ; i < n ; ++i )
   h2[ next[ b1[ i ] ]++ ] = foldedProduct( B[ i ] ^ f_seed ,
                                            detail::kRecSplitK[ 1 ] );
  }
 // the codes of the buckets: the low parts in preorder, then the unary
 // parts
 f_code.assign( 1 , 0 );
 std::uint64_t len = 0;
 auto put = [ & ]( std::uint64_t v , unsigned k ) {
  if( k == 0 )
   return;
  const unsigned s = unsigned( len & 63 );
  f_code.back() |= v << s;
  if( s + k >= 64 )
   f_code.push_back( s == 0 ? 0 : v >> ( 64 - s ) );
  len += k; };
 auto putOne = [ & ]( std::uint64_t zeros ) {
  for( ; zeros >= 32 ; zeros -= 32 )
   put( 0 , 32 );
  put( std::uint64_t( 1 ) << zeros , unsigned( zeros + 1 ) ); };
 std::vector< std::uint64_t > place( nb + 1 ) , S( smax ) , seeds;
 std::vector< std::uint8_t > rice;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  place[ j ] = len;
  const std::size_t m = start[ j + 1 ] - start[ j ];
  std::uint64_t * K = h2.data() + start[ j ];
  std::sort( K , K + m );
  if( std::adjacent_find( K , K + m ) != K + m )
   return( false );                       // equal hashes never separate
  seeds.clear();
  rice.clear();
  split( K , S.data() , m , 0 , seeds , rice );
  for( std::size_t i = 0 ; i < seeds.size() ; ++i )
   put( seeds[ i ] & ( ( std::uint64_t( 1 ) << rice[ i ] ) - 1 ) ,
        rice[ i ] );
  for( std::size_t i = 0 ; i < seeds.size() ; ++i )
   putOne( seeds[ i ] >> rice[ i ] );
  }
 place[ nb ] = len;
 f_code.resize( f_code.size() + 2 , 0 );  // the windows past the end
 // the keys before each bucket less i times their least gap; the places
 // less beta times the keys, less i times their least gap
 f_beta = ( len << 16 ) / n;
 f_dk = start[ 1 ];
 for( std::size_t j = 1 ; j < nb ; ++j )
  f_dk = std::min< std::uint64_t >( f_dk , start[ j + 1 ] - start[ j ] );
 std::vector< std::int64_t > z( nb + 1 );
 for( std::size_t j = 0 ; j <= nb ; ++j )
  z[ j ] = std::int64_t( place[ j ] ) -
           std::int64_t( ( start[ j ] * f_beta ) >> 16 );
 f_dp = z[ 1 ] - z[ 0 ];
 for( std::size_t j = 1 ; j < nb ; ++j )
  f_dp = std::min( f_dp , z[ j + 1 ] - z[ j ] );
 for( std::size_t j = 0 ; j <= nb ; ++j ) {
  start[ j ] -= std::uint64_t( j ) * f_dk;
  place[ j ] = std::uint64_t( z[ j ] - std::int64_t( j ) * f_dp );
  }
 f_keys = EliasFano( start , start.back() + 1 );
 f_place = EliasFano( place , place.back() + 1 );
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::operator()( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 const std::uint64_t x = base( k ) ^ f_seed;
 const std::uint64_t h2 = foldedProduct( x , detail::kRecSplitK[ 1 ] );
 const std::size_t b = remap( foldedProduct( x , detail::kRecSplitK[ 0 ] ) ,
                              f_nb );
 const std::uint64_t s0 = f_keys.access( b ) + b * f_dk;
 std::size_t m = std::size_t( f_keys.access( b + 1 ) + ( b + 1 ) * f_dk -
                              s0 );
 std::size_t out = std::size_t( s0 );
 if( m <= 1 )
  return( out );
 const auto & t = *f_t;
 std::uint64_t fp = std::uint64_t( std::int64_t( f_place.access( b ) ) +
                                   std::int64_t( b ) * f_dp ) +
                    ( ( s0 * f_beta ) >> 16 );
 std::uint64_t up = fp + t.low[ m ];
 for( unsigned d = 0 ; ; ++d ) {
  // the seed of the node: its low part, then its unary part
  const unsigned r = t.rice[ m ];
  std::uint64_t v = window( fp ) & ( ( std::uint64_t( 1 ) << r ) - 1 );
  fp += r;
  for( std::uint64_t q = 0 ; ; q += 64 , up += 64 )
   if( const std::uint64_t w = window( up ) ; w != 0 ) {
    const auto z = unsigned( std::countr_zero( w ) );
    v |= ( q + z ) << r;
    up += z + 1;
    break;
    }
  if( m <= L ) {                          // a leaf, x m + r
   const std::uint64_t xq = v / m , rot = v % m;
   std::size_t p = remap( node( h2 , xq , d ) , m );
   if( h2 >> 63 ) {
    p += std::size_t( rot );
    if( p >= m )
     p -= m;
    }
   return( out + p );
   }
  const std::size_t p = remap( node( h2 , v , d ) , m );
  if( m > kUpper ) {
   const std::size_t u = left( m );
   if( p < u )
    m = u;
   else {
    fp += t.low[ u ];
    up = skipOnes( up , t.nodes[ u ] );
    out += u;
    m -= u;
    }
   }
  else {
   const std::size_t unit = m > kLower ? kLower : L ,
                     j = m > kLower ? p / kLower : p / L;
   fp += j * t.low[ unit ];
   up = skipOnes( up , j * t.nodes[ unit ] );
   out += j * unit;
   m = std::min( unit , m - j * unit );
   }
  if( m <= 1 )
   return( out );
  }
 }

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::size( void ) const
{
 return( f_n );
 }

template< class T , unsigned L , class H >
bool RecSplit< T , L , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::bits( void ) const
{
 return( 64 * f_code.size() + f_keys.sizeInBits() + f_place.sizeInBits() +
         8 * sizeof( *this ) );
 }

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::buckets( void ) const
{
 return( f_nb );
 }

template< class T , unsigned L , class H >
std::size_t RecSplit< T , L , H >::trials( void ) const
{
 return( f_trials );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File RecSplit.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
