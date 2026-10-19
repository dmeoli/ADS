/*--------------------------------------------------------------------------*/
/*------------------------- File FlatHashTable.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FlatHashTable [see FlatHashTable.h], with their
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
/*---------------------------- THE MIXED HASH ------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::uint64_t detail::mulx( std::uint64_t a , std::uint64_t b )
{
#if defined( __SIZEOF_INT128__ )
 const unsigned __int128 p = static_cast< unsigned __int128 >( a ) * b;
 return( std::uint64_t( p ) ^ std::uint64_t( p >> 64 ) );
#elif defined( _MSC_VER ) && defined( _M_X64 )
 std::uint64_t h;
 const std::uint64_t l = _umul128( a , b , &h );
 return( l ^ h );
#elif defined( _MSC_VER ) && defined( _M_ARM64 )
 return( ( a * b ) ^ __umulh( a , b ) );
#else
 return( mulxPortable( a , b ) );
#endif
 }

constexpr std::uint64_t detail::mulxPortable( std::uint64_t a ,
                                              std::uint64_t b )
{
 constexpr std::uint64_t LOW = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & LOW , a1 = a >> 32 , b0 = b & LOW ,
                     b1 = b >> 32;
 const std::uint64_t p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
                     p11 = a1 * b1;
 const std::uint64_t mid = ( p00 >> 32 ) + ( p01 & LOW ) + ( p10 & LOW );
 const std::uint64_t lo = ( mid << 32 ) | ( p00 & LOW );
 const std::uint64_t hi = p11 + ( p01 >> 32 ) + ( p10 >> 32 ) +
                          ( mid >> 32 );
 return( lo ^ hi );
 }

namespace detail {

/// the fingerprint of each low byte b of a mixed hash, b or 1 for 0, in
/// each of 4 bytes, so that SSE2 spreads it by one shuffle of words
alignas( 64 ) inline constexpr auto flatTags = []( void ) {
 std::array< std::uint32_t , 256 > t{};
 for( std::uint32_t b = 0 ; b < 256 ; ++b )
  t[ b ] = ( b ? b : 1 ) * 0x01010101U;
 return( t );
 }();

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------- CONSTRUCTION AND DESTRUCTION ----------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( std::size_t n , H h , Eq eq )
 : f_h( std::move( h ) ) , f_eq( std::move( eq ) )
{
 if( n > 0 )
  rebuild( groups( n ) , []( FlatHashTable & ) {} );
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( const FlatHashTable & o )
 : f_h( o.f_h ) , f_eq( o.f_eq )
{
 if( o.f_cap == 0 )
  return;
 const std::size_t G = o.f_mask + 1;
 auto ctrl = std::make_unique< Group[] >( G );
 std::unique_ptr< Slot , Free > slot( allocate( o.f_cap ) );
 std::memcpy( static_cast< void * >( ctrl.get() ) , o.f_ctrl ,
              G * sizeof( Group ) );
 std::size_t n = 0;                         // the pairs copied so far
 try {                                      // the same places, copied
  o.each( [ & ]( std::size_t i ) {
   std::construct_at( &slot.get()[ i - i / GROUP ].kv , o.slot( i )->kv );
   ++n;
   } );
  }
 catch( ... ) {
  o.each( [ & ]( std::size_t i ) {
   if( n > 0 ) {
    std::destroy_at( &slot.get()[ i - i / GROUP ].kv );
    --n;
    }
   } );
  throw;
  }
 f_ctrl = ctrl.release();
 f_slot = slot.release();
 f_cap = o.f_cap;
 f_mask = o.f_mask;
 f_shift = o.f_shift;
 f_max = o.f_max;
 f_size = o.f_size;
 f_deleted = o.f_deleted;
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( FlatHashTable && o ) noexcept
 : f_ctrl( o.f_ctrl ) , f_slot( o.f_slot ) , f_cap( o.f_cap ) ,
   f_mask( o.f_mask ) , f_shift( o.f_shift ) , f_max( o.f_max ) ,
   f_size( o.f_size ) , f_deleted( o.f_deleted ) ,
   f_h( std::move( o.f_h ) ) , f_eq( std::move( o.f_eq ) )
{
 o.f_ctrl = s_none;
 o.f_slot = nullptr;
 o.f_cap = o.f_mask = o.f_max = o.f_size = o.f_deleted = 0;
 o.f_shift = 63;
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq > &
FlatHashTable< K , V , H , Eq >::operator=( FlatHashTable o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::~FlatHashTable()
{
 release();
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::swap( FlatHashTable & o ) noexcept
{
 using std::swap;
 swap( f_ctrl , o.f_ctrl );
 swap( f_slot , o.f_slot );
 swap( f_cap , o.f_cap );
 swap( f_mask , o.f_mask );
 swap( f_shift , o.f_shift );
 swap( f_max , o.f_max );
 swap( f_size , o.f_size );
 swap( f_deleted , o.f_deleted );
 swap( f_h , o.f_h );
 swap( f_eq , o.f_eq );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::optional< V >
FlatHashTable< K , V , H , Eq >::lookup( const K & k ) const
{
 const V * v = find( k );
 if( v )
  return( *v );
 return( std::nullopt );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE const V *
FlatHashTable< K , V , H , Eq >::find( const K & k ) const
{
 const Slot * s = locate( mix( k ) , k );
 return( s ? &s->kv.second : nullptr );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE V * FlatHashTable< K , V , H , Eq >::find( const K & k )
{
 return( const_cast< V * >( std::as_const( *this ).find( k ) ) );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::insert( const K & k , const V & v )
{
 return( put( k , v ) );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::insert( K && k , V && v )
{
 return( put( std::move( k ) , std::move( v ) ) );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::remove( const K & k )
{
 const std::uint64_t x = mix( k );
 Slot * s = locate( x , k );
 if( ! s )
  return( false );
 const std::size_t e = std::size_t( s - f_slot );   // the place of s
 drop( e + e / SLOTS );
 ADS_POST( locate( x , k ) == nullptr );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE bool
FlatHashTable< K , V , H , Eq >::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::size( void ) const
{
 return( f_size );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::capacity( void ) const
{
 return( f_cap );
 }

template< class K , class V , class H , class Eq >
double FlatHashTable< K , V , H , Eq >::load( void ) const
{
 return( f_cap ? double( f_size ) / f_cap : 0.0 );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::deleted( void ) const
{
 return( f_deleted );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::reserve( std::size_t n )
{
 const std::size_t G = groups( n );
 if( G * SLOTS > f_cap )
  rebuild( G , []( FlatHashTable & ) {} );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::clear( void )
{
 if constexpr( ! std::is_trivially_destructible_v< std::pair< K , V > > )
  each( [ & ]( std::size_t i ) { std::destroy_at( &slot( i )->kv ); } );
 if( f_cap )
  std::memset( static_cast< void * >( f_ctrl ) , 0 ,
               ( f_mask + 1 ) * sizeof( Group ) );
 f_size = f_deleted = 0;
 }

template< class K , class V , class H , class Eq >
template< class F >
void FlatHashTable< K , V , H , Eq >::forEach( F && f ) const
{
 each( [ & ]( std::size_t i ) {
  const auto & kv = slot( i )->kv;
  f( kv.first , std::as_const( kv.second ) );
  } );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator
FlatHashTable< K , V , H , Eq >::erase( const_iterator it )
{
 const std::size_t i = it.place();
 ADS_PRE( ( i < Places::size( *this ) ) && Places::occupied( *this , i ) );
 [[maybe_unused]] const std::uint64_t x =       // for the invariant only
  ADS_CONTRACTS >= 3 ? mix( slot( i )->kv.first ) : 0;
 drop( i );
 ADS_INVARIANT( consistent( x ) );
 return( const_iterator( this , i + 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE GROUPS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint64_t
FlatHashTable< K , V , H , Eq >::mix( const K & k ) const
{
 if constexpr( AVALANCHING )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( detail::mulx( std::uint64_t( f_h( k ) ) ,
                        0x9e3779b97f4a7c15ULL ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint32_t
FlatHashTable< K , V , H , Eq >::tag( std::uint64_t x )
{
 return( detail::flatTags[ x & 0xFF ] );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint8_t
FlatHashTable< K , V , H , Eq >::bit( std::uint64_t x )
{
 return( std::uint8_t( 1 << ( ( x >> 8 ) & 7 ) ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::size_t
FlatHashTable< K , V , H , Eq >::home( std::uint64_t x ) const
{
 return( std::size_t( x >> f_shift ) );
 }

#if ADS_FLAT_SIMD == 1

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::match( const Group & c , std::uint32_t t )
{
 const __m128i w =
  _mm_load_si128( reinterpret_cast< const __m128i * >( c.c ) );
 return( Mask( _mm_movemask_epi8(
          _mm_cmpeq_epi8( w , _mm_set1_epi32( int( t ) ) ) ) ) & 0x7FFF );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::empties( const Group & c )
{
 const __m128i w =
  _mm_load_si128( reinterpret_cast< const __m128i * >( c.c ) );
 return( Mask( _mm_movemask_epi8(
          _mm_cmpeq_epi8( w , _mm_setzero_si128() ) ) ) & 0x7FFF );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::fulls( const Group & c )
{
 return( empties( c ) ^ 0x7FFF );
 }

#elif ADS_FLAT_SIMD == 2

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::match( const Group & c , std::uint32_t t )
{
 // the 16 bytes of 0 or 0xFF narrowed to 16 nibbles, the byte j in the
 // nibble j
 const uint8x16_t e = vceqq_u8( vld1q_u8( c.c ) ,
                                vreinterpretq_u8_u32( vdupq_n_u32( t ) ) );
 const uint8x8_t n = vshrn_n_u16( vreinterpretq_u16_u8( e ) , 4 );
 return( vget_lane_u64( vreinterpret_u64_u8( n ) , 0 ) &
         0x0888888888888888ULL );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::empties( const Group & c )
{
 return( match( c , EMPTY ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::fulls( const Group & c )
{
 return( empties( c ) ^ 0x0888888888888888ULL );
 }

#else

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint64_t
FlatHashTable< K , V , H , Eq >::word( const std::uint8_t * c )
{
 std::uint64_t w;
 std::memcpy( &w , c , 8 );
 if constexpr( std::endian::native == std::endian::big ) {
  std::uint64_t r = 0;                      // the byte 0 in the low bits
  for( int j = 0 ; j < 8 ; ++j , w >>= 8 )
   r = ( r << 8 ) | ( w & 0xFF );
  w = r;
  }
 return( w );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint64_t
FlatHashTable< K , V , H , Eq >::zeros( std::uint64_t w )
{
 // the low 7 bits of a byte plus 0x7F carry into its bit 7 unless they
 // are 0, and never into the next byte
 constexpr std::uint64_t L7 = 0x7F7F7F7F7F7F7F7FULL;
 return( ~( ( ( w & L7 ) + L7 ) | w | L7 ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::gather( std::uint64_t z )
{
 // the bit 8 j of z >> 7 times 2^( 56 - 7 j ) lands on the bit 56 + j, and
 // no two products share a bit
 return( Mask( ( ( z >> 7 ) * 0x0102040810204080ULL ) >> 56 ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::match( const Group & c , std::uint32_t t )
{
 const std::uint64_t b = ( std::uint64_t( t ) << 32 ) | t;
 return( ( gather( zeros( word( c.c ) ^ b ) ) |
           ( gather( zeros( word( c.c + 8 ) ^ b ) ) << 8 ) ) & 0x7FFF );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::empties( const Group & c )
{
 return( match( c , EMPTY ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Mask
FlatHashTable< K , V , H , Eq >::fulls( const Group & c )
{
 return( empties( c ) ^ 0x7FFF );
 }

#endif

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::size_t
FlatHashTable< K , V , H , Eq >::lowest( Mask m )
{
 return( std::size_t( std::countr_zero( m ) ) >> STRIDE );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint8_t &
FlatHashTable< K , V , H , Eq >::ctrl( std::size_t i ) const
{
 return( f_ctrl[ i / GROUP ].c[ i % GROUP ] );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::slot( std::size_t i ) const
{
 return( f_slot + ( i - i / GROUP ) );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::allocate( std::size_t n )
{
 // no cookie before the array, unlike new Slot[ n ] with a destructor
 auto * p = static_cast< Slot * >( ::operator new( n * sizeof( Slot ) ,
                                   std::align_val_t( ALIGN ) ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  ::new( static_cast< void * >( p + i ) ) Slot;
 return( p );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::Free::operator()( Slot * p ) const
 noexcept
{
 ::operator delete( p , std::align_val_t( ALIGN ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PLACES --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::locate( std::uint64_t x ,
                                         const K & k ) const
{
 const std::uint32_t t = tag( x );
 std::size_t g = home( x );
 for( std::size_t i = 0 ; ; ) {
  const Group & c = f_ctrl[ g ];
  if( Mask m = match( c , t ) ) {
   Slot * p = f_slot + g * SLOTS;
   ADS_FLAT_PREFETCH( p );                  // issued while m is predicted
   do {
    const std::size_t j = lowest( m );
    if( f_eq( p[ j ].kv.first , k ) ) [[likely]]
     return( p + j );
    m &= m - 1;
    } while( m );
   }
  if( ! ( c.c[ OVER ] & bit( x ) ) || ( ++i > f_mask ) ) [[likely]]
   return( nullptr );                       // no key of x went past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::claim( std::uint64_t x )
{
 std::size_t g = home( x );
 for( std::size_t i = 1 ; ; ++i ) {
  if( const Mask m = empties( f_ctrl[ g ] ) )
   return( g * GROUP + lowest( m ) );
  f_ctrl[ g ].c[ OVER ] |= bit( x );        // x goes past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
bool FlatHashTable< K , V , H , Eq >::put( KK && k , VV && v )
{
 const std::uint64_t x = mix( k );
 if( Slot * s = locate( x , k ) ) {         // replaced in place
  s->kv.second = std::forward< VV >( v );
  ADS_INVARIANT( consistent( x ) );
  return( false );
  }
 std::size_t i;
 if( f_size + f_deleted >= f_max ) [[unlikely]]
  i = grow( x , std::forward< KK >( k ) , std::forward< VV >( v ) );
 else {
  i = claim( x );
  std::construct_at( &slot( i )->kv , std::forward< KK >( k ) ,
                     std::forward< VV >( v ) );
  if( f_deleted && f_ctrl[ i / GROUP ].c[ OVER ] )   // a place counted
   --f_deleted;
  ctrl( i ) = std::uint8_t( tag( x ) );
  ++f_size;
  }
 ADS_POST( locate( x , slot( i )->kv.first ) == slot( i ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
std::size_t FlatHashTable< K , V , H , Eq >::grow( std::uint64_t x ,
                                                  KK && k , VV && v )
{
 const std::size_t G = f_cap ? f_mask + 1 : 0;
 std::size_t i = 0;
 rebuild( f_size + 1 > f_max / 2 ? ( G ? 2 * G : 2 ) : G ,
          [ & ]( FlatHashTable & o ) {
           i = o.claim( x );
           std::construct_at( &o.slot( i )->kv , std::forward< KK >( k ) ,
                              std::forward< VV >( v ) );
           o.ctrl( i ) = std::uint8_t( tag( x ) );
           ++o.f_size;
           } );
 return( i );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::drop( std::size_t i )
{
 std::destroy_at( &slot( i )->kv );
 ctrl( i ) = EMPTY;
 if( f_ctrl[ i / GROUP ].c[ OVER ] )        // inspections pass here
  ++f_deleted;
 --f_size;
 }

template< class K , class V , class H , class Eq >
template< class F >
void FlatHashTable< K , V , H , Eq >::rebuild( std::size_t G , F && first )
{
 auto ctrl = std::make_unique< Group[] >( G );   // all 0: empty
 std::unique_ptr< Slot , Free > slot( allocate( G * SLOTS ) );
 FlatHashTable o( 0 , f_h , f_eq );         // the new places, in o
 o.f_ctrl = ctrl.release();
 o.f_slot = slot.release();
 o.f_cap = G * SLOTS;
 o.f_mask = G - 1;
 o.f_shift = 64 - std::countr_zero( G );
 o.f_max = bound( o.f_cap );
 first( o );
 // the full places, group by group; a copy that throws leaves this table
 // as it was
 each( [ & ]( std::size_t i ) {
  auto & kv = this->slot( i )->kv;
  const std::uint64_t x = mix( kv.first );
  const std::size_t j = o.claim( x );
  std::construct_at( &o.slot( j )->kv , std::move_if_noexcept( kv ) );
  o.ctrl( j ) = std::uint8_t( tag( x ) );
  ++o.f_size;
  } );
 swap( o );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::groups( std::size_t n )
{
 if( n == 0 )
  return( 0 );
 std::size_t G = 2;
 while( n > bound( G * SLOTS ) )
  G *= 2;
 return( G );
 }

template< class K , class V , class H , class Eq >
constexpr std::size_t FlatHashTable< K , V , H , Eq >::bound( std::size_t m )
{
 return( m - m / 8 );
 }

template< class K , class V , class H , class Eq >
template< class F >
void FlatHashTable< K , V , H , Eq >::each( F && f ) const
{
 if( f_cap == 0 )
  return;
 for( std::size_t g = 0 ; g <= f_mask ; ++g )
  for( Mask m = fulls( f_ctrl[ g ] ) ; m ; m &= m - 1 )
   f( g * GROUP + lowest( m ) );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::release( void ) noexcept
{
 if( f_cap == 0 )
  return;
 if constexpr( ! std::is_trivially_destructible_v< std::pair< K , V > > )
  each( [ & ]( std::size_t i ) { std::destroy_at( &slot( i )->kv ); } );
 delete[] f_ctrl;
 Free()( f_slot );
 f_ctrl = s_none;
 f_slot = nullptr;
 f_cap = f_mask = f_max = f_size = f_deleted = 0;
 f_shift = 63;
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::consistent( std::uint64_t x ) const
{
 if( ( f_size + f_deleted > f_max ) || ( f_max != bound( f_cap ) ) )
  return( false );
 // the place i reached by the inspection of its key: the bit of the key
 // set in each group before it
 auto reached = [ & ]( std::size_t i ) {
  const std::uint64_t y = mix( slot( i )->kv.first );
  if( ctrl( i ) != std::uint8_t( tag( y ) ) )
   return( false );
  std::size_t g = home( y );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   if( g == i / GROUP )
    return( true );
   if( ! ( f_ctrl[ g ].c[ OVER ] & bit( y ) ) )
    return( false );
   g = ( g + r ) & f_mask;
   }
  return( false );
  };
 if( f_cap > 1024 ) {
  std::size_t g = home( x );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   for( Mask m = fulls( f_ctrl[ g ] ) ; m ; m &= m - 1 )
    if( ! reached( g * GROUP + lowest( m ) ) )
     return( false );
   if( ! ( f_ctrl[ g ].c[ OVER ] & bit( x ) ) )
    break;
   g = ( g + r ) & f_mask;
   }
  return( true );
  }
 std::size_t full = 0;
 bool ok = true;
 each( [ & ]( std::size_t i ) {
  ++full;
  ok = ok && reached( i );
  } );
 return( ok && ( full == f_size ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FlatHashTable.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
