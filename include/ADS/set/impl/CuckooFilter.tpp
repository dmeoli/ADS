/*--------------------------------------------------------------------------*/
/*------------------------- File CuckooFilter.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CuckooFilter [see CuckooFilter.h], with their
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
CuckooFilter< T , F , H >::CuckooFilter( std::size_t n , H h )
 : f_h( h )
{
 ADS_PRE( n <= std::size_t( 0xFFFFFFFFU ) );
 const std::size_t b = std::bit_ceil( std::max< std::size_t >(
                        1 , ( n * 25 + 93 ) / 94 ) );   // 4 b 0.94 >= n
 f_T.assign( b , W( 0 ) );
 f_mask = b - 1;
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
std::uint64_t CuckooFilter< T , F , H >::hash( const T & x ) const
{
 if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( x ) ) );
 else
  return( hashWord( std::uint64_t( f_h( x ) ) ) );
 }

template< class T , class F , class H >
F CuckooFilter< T , F , H >::tag( std::uint64_t h )
{
 const auto t = F( ( h * 0xC2B2AE3D27D4EB4FULL ) >> ( 64 - kLane ) );
 return( F( t + ( t == 0 ) ) );
 }

template< class T , class F , class H >
std::size_t CuckooFilter< T , F , H >::bucket( std::uint64_t h ) const
{
 return( std::size_t( h >> 32 ) & f_mask );
 }

template< class T , class F , class H >
std::size_t CuckooFilter< T , F , H >::other( std::size_t b , F t ) const
{
 return( ( b ^ std::size_t( ( std::uint64_t( t ) *
                              0x9E3779B97F4A7C15ULL ) >> 32 ) ) & f_mask );
 }

template< class T , class F , class H >
std::uint64_t CuckooFilter< T , F , H >::random( void )
{
 f_rng ^= f_rng << 13;                       // xorshift64
 f_rng ^= f_rng >> 7;
 f_rng ^= f_rng << 17;
 return( f_rng );
 }

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::put( std::size_t b , F t )
{
 const W z = zeros( f_T[ b ] );
 if( z == 0 )
  return( false );
 f_T[ b ] |= W( t ) << ( std::countr_zero( z ) + 1 - kLane );
 return( true );
 }

template< class T , class F , class H >
void CuckooFilter< T , F , H >::place( std::size_t b , F t )
{
 if( put( b , t ) || put( other( b , t ) , t ) )
  return;
 std::uint64_t r = random();
 if( r >> 63 )
  b = other( b , t );
 for( int k = 0 ; k < kMaxKicks ; ++k ) {
  if( ( k & 15 ) == 15 )                    // 2 bits of r per eviction
   r = random();
  const int s = int( r & 3 ) * kLane;
  r >>= 2;
  const W w = f_T[ b ];
  const auto e = F( w >> s );               // the evicted fingerprint
  f_T[ b ] = W( ( w & ~( W( F( ~F( 0 ) ) ) << s ) ) | ( W( t ) << s ) );
  t = e;
  b = other( b , t );
  if( put( b , t ) )
   return;
  }
 f_vb = b;
 f_vt = t;
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::insertHash( std::uint64_t h )
{
 if( f_vt != 0 )                             // full
  return( false );
 place( bucket( h ) , tag( h ) );
 ++f_n;
 return( true );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool CuckooFilter< T , F , H >::maybeHash(
                                                   std::uint64_t h ) const
{
 const F t = tag( h );
 const std::size_t b1 = bucket( h ) , b2 = other( b1 , t );
 const W p = kOnes * W( t );
 const W u = f_T[ b1 ] ^ p , v = f_T[ b2 ] ^ p;
 bool in = ( ( ( ( u - kOnes ) & ~u ) | ( ( v - kOnes ) & ~v ) ) & ~kLow ) !=
           0;
 if( f_vt != 0 ) [[unlikely]]                // the victim, once full
  in |= ( f_vt == t ) & ( ( f_vb == b1 ) | ( f_vb == b2 ) );
 return( in );
 }

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::removeHash( std::uint64_t h )
{
 const F t = tag( h );
 const std::size_t b1 = bucket( h ) , b2 = other( b1 , t );
 const W p = kOnes * W( t );
 for( std::size_t b : { b1 , b2 } )
  if( const W z = zeros( f_T[ b ] ^ p ) ; z != 0 ) {
   f_T[ b ] &= ~( W( F( ~F( 0 ) ) ) << ( std::countr_zero( z ) + 1 -
                                          kLane ) );
   --f_n;
   if( f_vt != 0 ) {                         // the victim back in
    const F v = f_vt;
    f_vt = 0;
    place( f_vb , v );
    }
   return( true );
   }
 if( ( f_vt == t ) && ( ( f_vb == b1 ) || ( f_vb == b2 ) ) ) {
  f_vt = 0;
  --f_n;
  return( true );
  }
 return( false );
 }

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::insert( const T & x )
{
 const bool in = insertHash( hash( x ) );
 ADS_POST( ! in || maybe( x ) );
 return( in );
 }

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::remove( const T & x )
{
 return( removeHash( hash( x ) ) );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool CuckooFilter< T , F , H >::maybe( const T & x ) const
{
 return( maybeHash( hash( x ) ) );
 }

template< class T , class F , class H >
void CuckooFilter< T , F , H >::clear( void )
{
 std::fill( f_T.begin() , f_T.end() , W( 0 ) );
 f_n = 0;
 f_vt = 0;
 }

template< class T , class F , class H >
std::size_t CuckooFilter< T , F , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class F , class H >
bool CuckooFilter< T , F , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class F , class H >
std::size_t CuckooFilter< T , F , H >::capacity( void ) const
{
 return( 4 * f_T.size() );
 }

template< class T , class F , class H >
double CuckooFilter< T , F , H >::load( void ) const
{
 return( double( f_n ) / double( capacity() ) );
 }

template< class T , class F , class H >
std::size_t CuckooFilter< T , F , H >::bits( void ) const
{
 return( 8 * sizeof( W ) * f_T.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File CuckooFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
