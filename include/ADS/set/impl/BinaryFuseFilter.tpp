/*--------------------------------------------------------------------------*/
/*----------------------- File BinaryFuseFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BinaryFuseFilter [see BinaryFuseFilter.h], with their
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
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
BinaryFuseFilter< T , F , H >::BinaryFuseFilter(
                                const std::vector< T > & keys ,
                                std::uint64_t seed , H h )
 : f_seed( seed ) , f_h( h )
{
 ADS_PRE( keys.size() < ( std::size_t( 1 ) << 31 ) );
 std::vector< std::uint64_t > B;
 B.reserve( keys.size() );
 for( const T & k : keys )
  B.push_back( base( k ) );
 layout( B.size() );
 bool unique = false;
 for( ; ; ) {
  if( ++f_trials > kMaxTrials )
   throw std::runtime_error( "BinaryFuseFilter: no seed peels the keys" );
  f_seed = hashWord( f_seed , f_trials );   // a new seed at each trial
  if( build( B ) )
   break;
  if( ! unique ) {                         // equal hashes cannot peel
   std::sort( B.begin() , B.end() );
   B.erase( std::unique( B.begin() , B.end() ) , B.end() );
   layout( B.size() );
   unique = true;
   }
  }
 f_n = B.size();
 ADS_AUDIT( std::all_of( keys.begin() , keys.end() ,
                         [ this ]( const T & k ) { return( maybe( k ) ); } ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::uint64_t BinaryFuseFilter< T , F , H >::base( const T & x ) const
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
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::uint64_t BinaryFuseFilter< T , F , H >::mixed( std::uint64_t b ) const
{
 return( foldedProduct( b ^ f_seed , 0x9E3779B97F4A7C15ULL ) );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
F BinaryFuseFilter< T , F , H >::fingerprint( std::uint64_t h )
{
 return( F( h ^ ( h >> 32 ) ) );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::uint32_t BinaryFuseFilter< T , F , H >::place( std::uint64_t h ,
                                                    int i ) const
{
 const auto h0 = std::uint32_t( product128( h , f_CL ).hi );
 const std::uint32_t m = f_L - 1;
 switch( i ) {
  case( 0 ): return( h0 );
  case( 1 ): return( ( h0 + f_L ) ^ ( std::uint32_t( h >> 18 ) & m ) );
  default: return( ( h0 + 2 * f_L ) ^ ( std::uint32_t( h ) & m ) );
  }
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
void BinaryFuseFilter< T , F , H >::layout( std::size_t n )
{
 // the segments of Graf and Lemire for 3 places: L = 2^floor( log_3.33 n
 // + 2.25 ), at most 2^18, and about max( 1.125 , 0.875 + 0.25 log 10^6
 // / log n ) n places, but at least 34 segments while L > 256
 const double ln = std::log( double( std::max< std::size_t >( n , 1 ) ) );
 f_L = std::uint32_t( 1 ) << std::min( 18 , int( std::floor(
                                         ln / std::log( 3.33 ) + 2.25 ) ) );
 const double factor = n <= 1 ? 0 : std::max( 1.125 , 0.875 + 0.25 *
                                              std::log( 1e6 ) / ln );
 const auto capacity = std::size_t( std::round( double( n ) * factor ) );
 while( ( f_L > 256 ) && ( ( capacity + f_L - 1 ) / f_L < 34 ) )
  f_L /= 2;
 const auto c = std::max< std::ptrdiff_t >( 1 , std::ptrdiff_t(
                 ( capacity + f_L - 1 ) / f_L ) - 2 );
 f_CL = std::uint32_t( std::size_t( c ) * f_L );
 f_F.assign( n == 0 ? 0 : std::size_t( c + 2 ) * f_L , F( 0 ) );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
bool BinaryFuseFilter< T , F , H >::build(
                                     const std::vector< std::uint64_t > & B )
{
 const std::size_t n = B.size() , m = f_F.size();
 if( n == 0 )
  return( true );
 // the hashes sorted by the segment of their first place, in buffers
 // left uninitialized
 const int lg = std::countr_zero( f_L );
 const std::size_t segs = f_CL >> lg;
 std::vector< std::uint32_t > start( segs + 1 , 0 );
 auto S = std::make_unique_for_overwrite< std::uint64_t[] >( n );
 for( std::uint64_t b : B )
  ++start[ ( place( mixed( b ) , 0 ) >> lg ) + 1 ];
 for( std::size_t s = 0 ; s < segs ; ++s )
  start[ s + 1 ] += start[ s ];
 for( std::uint64_t b : B ) {
  const std::uint64_t h = mixed( b );
  S[ start[ place( h , 0 ) >> lg ]++ ] = h;
  }
 // each place: 4 times its keys plus the XOR of their indices there, and
 // the XOR of their hashes
 std::vector< std::uint8_t > count( m , 0 );
 std::vector< std::uint64_t > xh( m , 0 );
 for( std::size_t k = 0 ; k < n ; ++k ) {
  const std::uint64_t h = S[ k ];
  const std::uint32_t P[ 3 ] = { place( h , 0 ) , place( h , 1 ) ,
                                 place( h , 2 ) };
  for( int i = 0 ; i < 3 ; ++i ) {
   const std::uint32_t p = P[ i ];
   if( count[ p ] >= 252 )                 // 63 keys, a degenerate seed
    return( false );
   count[ p ] = std::uint8_t( ( count[ p ] + 4 ) ^ i );
   xh[ p ] ^= h;
   }
  }
 // the peeling, from a stack of the places with one key, each pushed by
 // a store and an increment of the top by the test, with no branch; the
 // hashes peeled go to S, which is read no more
 auto Q = std::make_unique_for_overwrite< std::uint32_t[] >( m + 1 );
 std::size_t q = 0;
 for( std::uint32_t p = 0 ; p < m ; ++p ) {
  Q[ q ] = p;
  q += ( count[ p ] >> 2 ) == 1;
  }
 auto which = std::make_unique_for_overwrite< std::uint8_t[] >( n );
 constexpr int kNext[ 4 ] = { 1 , 2 , 0 , 1 };   // ( i + 1 ) mod 3, ...
 std::size_t k = 0;
 while( q > 0 ) {
  const std::uint32_t p = Q[ --q ];
  if( ( count[ p ] >> 2 ) != 1 )
   continue;
  const std::uint64_t h = xh[ p ];
  const int i = count[ p ] & 3;
  S[ k ] = h;
  which[ k++ ] = std::uint8_t( i );
  count[ p ] = 0;
  const std::uint32_t P[ 3 ] = { place( h , 0 ) , place( h , 1 ) ,
                                 place( h , 2 ) };
  for( int j : { kNext[ i ] , kNext[ i + 1 ] } ) {
   const std::uint32_t r = P[ j ];
   Q[ q ] = r;
   q += ( count[ r ] >> 2 ) == 2;          // one key left after this one
   count[ r ] = std::uint8_t( ( count[ r ] - 4 ) ^ j );
   xh[ r ] ^= h;
   }
  }
 if( k != n )
  return( false );
 // the fingerprints in the reverse order of the peeling
 std::fill( f_F.begin() , f_F.end() , F( 0 ) );
 for( std::size_t k = n ; k-- > 0 ; ) {
  const std::uint64_t h = S[ k ];
  const std::uint32_t p0 = place( h , 0 ) , p1 = place( h , 1 ) ,
                      p2 = place( h , 2 );
  const std::uint32_t p = which[ k ] == 0 ? p0 : which[ k ] == 1 ? p1 : p2;
  f_F[ p ] = F( fingerprint( h ) ^ f_F[ p0 ] ^ f_F[ p1 ] ^ f_F[ p2 ] );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
bool BinaryFuseFilter< T , F , H >::maybe( const T & x ) const
{
 if( f_F.empty() )
  return( false );
 const std::uint64_t h = mixed( base( x ) );
 return( F( f_F[ place( h , 0 ) ] ^ f_F[ place( h , 1 ) ] ^
            f_F[ place( h , 2 ) ] ) == fingerprint( h ) );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::size_t BinaryFuseFilter< T , F , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
bool BinaryFuseFilter< T , F , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::size_t BinaryFuseFilter< T , F , H >::bits( void ) const
{
 return( 8 * sizeof( F ) * f_F.size() );
 }

template< class T , class F , class H >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
std::size_t BinaryFuseFilter< T , F , H >::trials( void ) const
{
 return( f_trials );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File BinaryFuseFilter.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
