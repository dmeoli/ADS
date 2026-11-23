/*--------------------------------------------------------------------------*/
/*--------------------- File VectorQuotientFilter.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of VectorQuotientFilter [see VectorQuotientFilter.h], with
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

namespace ADS
{

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
VectorQuotientFilter< T , F , H >::VectorQuotientFilter( std::size_t n , H h )
    : f_h( h )
{
 ADS_PRE( n < ( std::size_t( 1 ) << 31 ) );
 const auto nb = std::max< std::size_t >(
   1 , std::size_t( std::ceil( double( n ) / ( kLoad * kSlots ) ) ) );
 Block E;
 std::fill( E.v , E.v + 64 / sizeof( F ) , F( 0 ) );
 Meta M;  // all the bits 1: c ends of buckets, s free slots
 M.w[ 0 ] = M.w[ 1 ] = ~std::uint64_t( 0 );
 setMeta( E , M );
 f_B.assign( nb , E );
 f_R = nb * kBuckets;
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
std::uint64_t VectorQuotientFilter< T , F , H >::hash( const T & x ) const
{
 if constexpr( detail::kAvalanching64< H , T > )
  return( detail::hash64( f_h , x ) );
 else
  return( hashWord( detail::hash64( f_h , x ) ) );
 }

template< class T , class F , class H >
F VectorQuotientFilter< T , F , H >::tag( std::uint64_t h )
{
 return( F( ( h * 0xC2B2AE3D27D4EB4FULL ) >> ( 64 - 8 * sizeof( F ) ) ) );
 }

template< class T , class F , class H >
std::size_t VectorQuotientFilter< T , F , H >::bucket( std::uint64_t h ) const
{
 return( std::size_t( ( ( h >> 32 ) * std::uint64_t( f_R ) ) >> 32 ) );
 }

template< class T , class F , class H >
std::size_t VectorQuotientFilter< T , F , H >::other( std::size_t b ,
						    F t ) const
{
 const auto g =
   std::size_t( ( ( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL ) >> 32 ) *
		  std::uint64_t( f_R ) ) >>
		32 );
 return( g >= b ? g - b : g + f_R - b );
 }

template< class T , class F , class H >
auto VectorQuotientFilter< T , F , H >::meta( const Block & B ) -> Meta
{
 Meta M;
 std::memcpy( M.w , B.v , 8 * kWords );
 if constexpr( kWords == 1 )
  M.w[ 1 ] = 0;
 return( M );
 }

template< class T , class F , class H >
void VectorQuotientFilter< T , F , H >::setMeta( Block & B , const Meta & M )
{
 std::memcpy( B.v , M.w , 8 * kWords );
 }

template< class T , class F , class H >
unsigned VectorQuotientFilter< T , F , H >::used( const Meta & M )
{
 return( kSlots + kBuckets -
	  unsigned( std::popcount( M.w[ 0 ] ) + std::popcount( M.w[ 1 ] ) ) );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE unsigned
VectorQuotientFilter< T , F , H >::select( const Meta & M , unsigned k )
{
 // the word of the 1 chosen with no branch, which the buckets would
 // mispredict half of the times
 const auto c = unsigned( std::popcount( M.w[ 0 ] ) );
 const bool lo = ( kWords == 1 ) || ( k < c );
 const std::uint64_t w = lo ? M.w[ 0 ] : M.w[ 1 ];
 return( ( lo ? 0 : 64 ) +
	  unsigned( selectInWord( w , int( lo ? k : k - c ) ) ) );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE void
VectorQuotientFilter< T , F , H >::range( const Meta & M , unsigned j ,
					unsigned & l , unsigned & u )
{
 // one select: the slots of the bucket are the 0s just below its 1, down
 // to the 1 before it, the highest 1 below the place s (none for j = 0)
 const unsigned s = select( M , j );
 const bool hi = s >= 64;
 const std::uint64_t m = ( std::uint64_t( 1 ) << ( s & 63 ) ) - 1;
 const std::uint64_t a = hi ? M.w[ 0 ] : M.w[ 0 ] & m ,
		     b = hi ? M.w[ 1 ] & m : 0;
 const auto top =
   unsigned( b != 0 ? 64 + std::bit_width( b ) : std::bit_width( a ) );
 u = s - j;
 l = u - ( s - top );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE int VectorQuotientFilter< T , F ,
					    H >::find( const Block & B ,
							     F t , unsigned l ,
							     unsigned u )
{
#if ADS_SIMD
 // the lanes of the line at once, the metadata out of the range [see
 // SimdInline.h]
 return( int( detail::simd::here::firstEqualIn< 64 / sizeof( F ) >(
	    B.v , t , kHead + l , kHead + u ) ) -
	  ( l < u ? int( kHead ) : 0 ) );
#else
 for( unsigned i = l ; i < u ; ++i )
  if( B.v[ kHead + i ] == t )
   return( int( i ) );
 return( -1 );
#endif
 }

template< class T , class F , class H >
void VectorQuotientFilter< T , F , H >::put( std::size_t b , F t )
{
 Block & B = f_B[ b / kBuckets ];
 Meta M = meta( B );
 const auto j = unsigned( b % kBuckets ) , s = select( M , j ) , u = used( M );
 // a 0 at s, the bits from s up by one (the top one, free, lost)
 if( s < 64 ) {
  const std::uint64_t lo = std::uint64_t( 1 ) << s;
  M.w[ 1 ] = ( M.w[ 1 ] << 1 ) | ( M.w[ 0 ] >> 63 );
  M.w[ 0 ] = ( M.w[ 0 ] & ( lo - 1 ) ) | ( ( M.w[ 0 ] & ~( lo - 1 ) ) << 1 );
  }
 else {
  const std::uint64_t lo = std::uint64_t( 1 ) << ( s - 64 );
  M.w[ 1 ] = ( M.w[ 1 ] & ( lo - 1 ) ) | ( ( M.w[ 1 ] & ~( lo - 1 ) ) << 1 );
  }
 setMeta( B , M );
 const unsigned i = s - j;  // the slot
 std::memmove( B.v + kHead + i + 1 , B.v + kHead + i ,
	       ( u - i ) * sizeof( F ) );
 B.v[ kHead + i ] = t;
 }

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::take( std::size_t b , F t )
{
 Block & B = f_B[ b / kBuckets ];
 if constexpr( bool( ADS_SIMD ) )  // as in holds()
  if( find( B , t , 0 , kSlots ) < 0 )
   return( false );
 Meta M = meta( B );
 const auto j = unsigned( b % kBuckets );
 unsigned l , u;
 range( M , j , l , u );
 const int f = find( B , t , l , u );
 if( f < 0 )
  return( false );
 const auto i = unsigned( f ) , s = i + j , n = used( M );
 // the 0 at s removed, the bits above down by one, a 1 at the top
 constexpr std::uint64_t top = std::uint64_t( 1 ) << 63;
 if( s < 64 ) {
  const std::uint64_t lo = ( std::uint64_t( 1 ) << s ) - 1;
  M.w[ 0 ] = ( M.w[ 0 ] & lo ) | ( ( M.w[ 0 ] >> 1 ) & ~lo ) |
	     ( kWords == 2 ? ( M.w[ 1 ] << 63 ) : top );
  if constexpr( kWords == 2 )
   M.w[ 1 ] = ( M.w[ 1 ] >> 1 ) | top;
  }
 else {
  const std::uint64_t lo = ( std::uint64_t( 1 ) << ( s - 64 ) ) - 1;
  M.w[ 1 ] = ( M.w[ 1 ] & lo ) | ( ( M.w[ 1 ] >> 1 ) & ~lo ) | top;
  }
 setMeta( B , M );
 std::memmove( B.v + kHead + i , B.v + kHead + i + 1 ,
	       ( n - i - 1 ) * sizeof( F ) );
 B.v[ kHead + n - 1 ] = F( 0 );
 return( true );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool VectorQuotientFilter< T , F ,
					     H >::holds( std::size_t b ,
							       F t ) const
{
 const Block & B = f_B[ b / kBuckets ];
 // with the vectors, the slots of the block first, one comparison that
 // most of the absent keys do not pass, before the select of the bucket
 if constexpr( bool( ADS_SIMD ) )
  if( find( B , t , 0 , kSlots ) < 0 )
   return( false );
 unsigned l , u;
 range( meta( B ) , unsigned( b % kBuckets ) , l , u );
 return( find( B , t , l , u ) >= 0 );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::insertHash( std::uint64_t h )
{
 const F t = tag( h );
 std::size_t b = bucket( h );
 unsigned u = used( meta( f_B[ b / kBuckets ] ) );
 if( u > kAlt ) {  // the block with fewer slots in use
  const std::size_t b2 = other( b , t );
  const unsigned u2 = used( meta( f_B[ b2 / kBuckets ] ) );
  if( u2 < u ) {
   b = b2;
   u = u2;
   }
  if( u == kSlots )
   return( false );
  }
 put( b , t );
 ++f_n;
 return( true );
 }

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::removeHash( std::uint64_t h )
{
 const F t = tag( h );
 const std::size_t b = bucket( h );
 if( take( b , t ) || take( other( b , t ) , t ) ) {
  --f_n;
  return( true );
  }
 return( false );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool
VectorQuotientFilter< T , F , H >::maybeHash( std::uint64_t h ) const
{
 const F t = tag( h );
 const std::size_t b = bucket( h ) , b2 = other( b , t );
 ADS::detail::prefetchLine( &f_B[ b2 / kBuckets ] );  // the two misses at once
 return( holds( b , t ) || holds( b2 , t ) );
 }

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::insert( const T & x )
{
 const bool in = insertHash( hash( x ) );
 ADS_POST( ! in || maybe( x ) );
 return( in );
 }

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::remove( const T & x )
{
 return( removeHash( hash( x ) ) );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool
VectorQuotientFilter< T , F , H >::maybe( const T & x ) const
{
 return( maybeHash( hash( x ) ) );
 }

template< class T , class F , class H >
void VectorQuotientFilter< T , F , H >::clear( void )
{
 Block E;
 std::fill( E.v , E.v + 64 / sizeof( F ) , F( 0 ) );
 Meta M;
 M.w[ 0 ] = M.w[ 1 ] = ~std::uint64_t( 0 );
 setMeta( E , M );
 std::fill( f_B.begin() , f_B.end() , E );
 f_n = 0;
 }

template< class T , class F , class H >
std::size_t VectorQuotientFilter< T , F , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class F , class H >
bool VectorQuotientFilter< T , F , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class F , class H >
std::size_t VectorQuotientFilter< T , F , H >::capacity( void ) const
{
 return( kSlots * f_B.size() );
 }

template< class T , class F , class H >
double VectorQuotientFilter< T , F , H >::load( void ) const
{
 return( double( f_n ) / double( capacity() ) );
 }

template< class T , class F , class H >
double VectorQuotientFilter< T , F , H >::falsePositiveBound( void ) const
{
 return( std::min( 1.0 , std::ldexp( 2.0 * double( f_n ) / double( f_R ) ,
				     -int( 8 * sizeof( F ) ) ) ) );
 }

template< class T , class F , class H >
std::size_t VectorQuotientFilter< T , F , H >::bits( void ) const
{
 return( 512 * f_B.size() );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
template< class A >
inline void VectorQuotientFilter< T , F , H >::persist( A & a )
{
 a( f_B , f_R , f_n );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ( f_B.size() >= 1 ) && ( f_R == kBuckets * f_B.size() ) ,
	    "the blocks of a VectorQuotientFilter" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File VectorQuotientFilter.tpp -------------------*/
/*--------------------------------------------------------------------------*/
