/*--------------------------------------------------------------------------*/
/*------------------------- File FlatHashTable.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FlatHashTable [see FlatHashTable.h], with their
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
  rebuild( groups( n ) , []( FlatHashTable & , auto & ) {} );
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( const FlatHashTable & o )
 : f_h( o.f_h ) , f_eq( o.f_eq )
{
 if( o.f_cap == 0 )
  return;
 const std::size_t G = o.f_mask + 1;
 Group * ctrl;
 Slot * const slot = allocate( G , ctrl );
 std::memcpy( static_cast< void * >( ctrl ) , o.f_ctrl , G * sizeof( Group ) );
 std::size_t n = 0;                         // the pairs copied so far
 try {                                      // the same places, copied
  o.each( [ & ]( std::size_t i ) {
   std::construct_at( &slot[ i - i / GROUP ].kv , o.slot( i )->kv );
   ++n;
   } );
  }
 catch( ... ) {
  o.each( [ & ]( std::size_t i ) {
   if( n > 0 ) {
    std::destroy_at( &slot[ i - i / GROUP ].kv );
    --n;
    }
   } );
  deallocate( slot , ctrl , G );
  throw;
  }
 f_ctrl = ctrl;
 f_slot = slot;
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
ADS_FLAT_INLINE bool
FlatHashTable< K , V , H , Eq >::insert( const K & k , const V & v )
{
 return( put( k , v ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE bool
FlatHashTable< K , V , H , Eq >::insert( K && k , V && v )
{
 return( put( std::move( k ) , std::move( v ) ) );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::remove( const K & k )
{
 // the first group alone, the common case, as in locate(): the place j
 // found there is emptied by writing the whole group, whose address
 // depends only on the hash, while that of the byte j depends on the
 // control bytes just read; a later load can then pass the store before
 // the control bytes arrive, so that the removals of distinct keys overlap
 // their misses as the searches do
 const std::uint64_t x = mix( k );
 const std::size_t g = home( x );
 Group & c = f_ctrl[ g ];
 const std::uint8_t over = c.c[ OVER ];
 if( Mask m = match( c , tag( x ) ) ) {
  Slot * p = f_slot + g * SLOTS;
  do {
   const std::size_t j = lowest( m );
   if( f_eq( p[ j ].kv.first , k ) ) [[likely]] {
    std::destroy_at( &p[ j ].kv );
    vacate( c , j );
    ++f_deleted;                            // see drop()
    --f_size;
    ADS_POST( locate( x , k ) == nullptr );
    ADS_INVARIANT( consistent( x ) );
    return( true );
    }
   m &= m - 1;
   } while( m );
  }
 if( ! ( over & bit( x ) ) ) [[likely]]
  return( false );                          // no key of x went past g
 Slot * s = further( x , g , k );
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
bool FlatHashTable< K , V , H , Eq >::empty( void ) const
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
 std::size_t d = 0;                         // the empty places counted
 for( std::size_t g = 0 ; f_cap && ( g <= f_mask ) ; ++g )
  if( f_ctrl[ g ].c[ OVER ] )
   d += std::size_t( std::popcount( empties( f_ctrl[ g ] ) ) );
 return( d );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::recount( void )
{
 f_deleted = deleted();
 return( f_size + f_deleted + f_cap / 16 < f_max );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::reserve( std::size_t n )
{
 const std::size_t G = groups( n );
 if( G * SLOTS > f_cap )
  rebuild( G , []( FlatHashTable & , auto & ) {} );
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
template< class F >
void FlatHashTable< K , V , H , Eq >::forEach( F && f )
{
 each( [ & ]( std::size_t i ) {
  auto & kv = slot( i )->kv;
  f( std::as_const( kv.first ) , kv.second );
  } );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator
FlatHashTable< K , V , H , Eq >::begin( void ) const
{
 if( f_size == 0 )
  return( end() );
 const_iterator it( f_ctrl , f_slot , fulls( *f_ctrl ) );
 if( ! it.f_m )
  it.forward();
 return( it );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator
FlatHashTable< K , V , H , Eq >::end( void ) const
{
 return( const_iterator( f_cap ? f_ctrl + f_mask + 1 : s_none + 2 ,
                         f_slot + f_cap , FIRST ) );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator
FlatHashTable< K , V , H , Eq >::erase( const_iterator it )
{
 const std::size_t i = std::size_t( it.f_g - f_ctrl ) * GROUP +
                       lowest( it.f_m );
 ADS_PRE( ( f_cap > 0 ) && ( it.f_g >= f_ctrl ) &&
          ( i < ( f_mask + 1 ) * GROUP ) && ( ctrl( i ) != EMPTY ) &&
          ( slot( i ) == it.f_b + lowest( it.f_m ) ) );
 [[maybe_unused]] const std::uint64_t x =       // for the invariant only
  ADS_CONTRACTS >= 3 ? mix( slot( i )->kv.first ) : 0;
 drop( i );
 ADS_INVARIANT( consistent( x ) );
 return( ++it );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::const_iterator::forward( void )
{
 do {                                       // the sentinel ends it
  ++f_g;
  f_b += SLOTS;
  } while( ! ( f_m = fulls( *f_g ) ) );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator &
FlatHashTable< K , V , H , Eq >::const_iterator::operator--( void )
{
 // the full places before this one in its group, else in a group before
 Mask m = fulls( *f_g ) & ( ( f_m & ( Mask( 0 ) - f_m ) ) - 1 );
 while( ! m ) {
  --f_g;
  f_b -= SLOTS;
  m = fulls( *f_g );
  }
 const int h = std::bit_width( m ) - 1;     // the highest, and on from it
 f_m = fulls( *f_g ) >> h << h;
 return( *this );
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
 return( std::uint8_t( 1 << ( x & 7 ) ) );
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
std::size_t FlatHashTable< K , V , H , Eq >::offset( std::size_t G )
{
 return( ( G * SLOTS * sizeof( Slot ) + ALIGN - 1 ) / ALIGN * ALIGN );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::allocate( std::size_t G , Group * & c )
{
 // no cookie before the array, unlike new Slot[ n ] with a destructor; the
 // groups are implicit-lifetime bytes
 const std::size_t at = offset( G ) , bytes = ( G + 1 ) * sizeof( Group );
 const bool join = G <= JOIN;
 void * const b = ::operator new( join ? at + bytes : at ,
                                  std::align_val_t( ALIGN ) );
 if( join )
  c = reinterpret_cast< Group * >( static_cast< unsigned char * >( b ) + at );
 else
  try {
   c = static_cast< Group * >( ::operator new( bytes ,
                                               std::align_val_t( ALIGN ) ) );
   }
  catch( ... ) {
   ::operator delete( b , std::align_val_t( ALIGN ) );
   throw;
   }
 auto * s = static_cast< Slot * >( b );
 for( std::size_t i = 0 ; i < G * SLOTS ; ++i )
  ::new( static_cast< void * >( s + i ) ) Slot;
 std::memset( static_cast< void * >( c ) , 0 , bytes );
 c[ G ].c[ 0 ] = 1;                         // the sentinel, full
 return( s );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::deallocate( Slot * s , Group * c ,
                                                 std::size_t G ) noexcept
{
 if( G > JOIN )
  ::operator delete( static_cast< void * >( c ) , std::align_val_t( ALIGN ) );
 ::operator delete( static_cast< void * >( s ) , std::align_val_t( ALIGN ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PLACES --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::scan( std::size_t g , Mask m ,
                                       const K & k ) const
{
 Slot * p = f_slot + g * SLOTS;
 ADS_FLAT_PREFETCH( p );                    // issued while m is predicted
 do {
  const std::size_t j = lowest( m );
  if( f_eq( p[ j ].kv.first , k ) ) [[likely]]
   return( p + j );
  m &= m - 1;
  } while( m );
 return( nullptr );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::locate( std::uint64_t x ,
                                         const K & k ) const
{
 // the whole inspection in line, with no call: a call would take the
 // registers that the loop of the caller keeps its values in, and the
 // compiler would no longer place the search for an absent key, which
 // stops at the first group, on the way back to the top of that loop
 std::size_t g = home( x );
 const std::uint32_t t = tag( x );
 for( std::size_t i = 0 ; ; ) {
  const Group & c = f_ctrl[ g ];
  if( const Mask m = match( c , t ) )
   if( Slot * p = scan( g , m , k ) ) [[likely]]
    return( p );
  if( ! ( c.c[ OVER ] & bit( x ) ) ) [[likely]]
   return( nullptr );                       // no key of x went past g
  if( ++i > f_mask ) [[unlikely]]
   return( nullptr );                       // all the groups seen
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::further( std::uint64_t x , std::size_t g ,
                                          KeyArg k ) const
{
 const std::uint32_t t = tag( x );
 for( std::size_t i = 1 ; i <= f_mask ; ++i ) {
  g = ( g + i ) & f_mask;
  const Group & c = f_ctrl[ g ];
  if( const Mask m = match( c , t ) )
   if( Slot * p = scan( g , m , k ) )
    return( p );
  if( ! ( c.c[ OVER ] & bit( x ) ) )
   return( nullptr );
  }
 return( nullptr );                         // all the groups seen
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
ADS_FLAT_INLINE bool
FlatHashTable< K , V , H , Eq >::put( KK && k , VV && v )
{
 // the search of locate(), whose first group also gives the place of a
 // new key when it has an empty one, the common case
 const std::uint64_t x = mix( k );
 const std::size_t g = home( x );
 const Group & c = f_ctrl[ g ];
 Slot * s = nullptr;
 if( const Mask m = match( c , tag( x ) ) )
  s = scan( g , m , k );
 if( ! s && ( c.c[ OVER ] & bit( x ) ) ) [[unlikely]]
  s = further( x , g , k );
 if( s ) {                                  // replaced in place
  s->kv.second = std::forward< VV >( v );
  ADS_INVARIANT( consistent( x ) );
  return( false );
  }
 std::size_t i;
 if( ( f_size + f_deleted >= f_max ) && ! recount() ) [[unlikely]]
  i = grow( x , std::forward< KK >( k ) , std::forward< VV >( v ) );
 else {
  const Mask e = empties( c );
  i = e ? g * GROUP + lowest( e ) : claim( x );
  std::construct_at( &slot( i )->kv , std::forward< KK >( k ) ,
                     std::forward< VV >( v ) );
  // a place counted, by no branch past the test of the bound, 0 with no
  // removals; an empty place of a group with some overflow was emptied by
  // a removal, so that then the bound is at least 1
  if( f_deleted ) [[unlikely]]
   f_deleted -= std::size_t( f_ctrl[ i / GROUP ].c[ OVER ] != 0 );
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
          [ & ]( FlatHashTable & o , auto & place ) {
           i = place( x );
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
 ++f_deleted;                               // a bound, see f_deleted
 --f_size;
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE void FlatHashTable< K , V , H , Eq >::vacate( Group & c ,
                                                              std::size_t j )
{
#if ADS_FLAT_SIMD == 1
 const __m128i w = _mm_load_si128( reinterpret_cast< __m128i * >( c.c ) ) ,
               at = _mm_cmpeq_epi8( _mm_setr_epi8( 0 , 1 , 2 , 3 , 4 , 5 ,
                                     6 , 7 , 8 , 9 , 10 , 11 , 12 , 13 , 14 ,
                                     15 ) , _mm_set1_epi8( char( j ) ) );
 _mm_store_si128( reinterpret_cast< __m128i * >( c.c ) ,
                  _mm_andnot_si128( at , w ) );
#elif ADS_FLAT_SIMD == 2
 static constexpr std::uint8_t iota[ 16 ] = { 0 , 1 , 2 , 3 , 4 , 5 , 6 ,
                                              7 , 8 , 9 , 10 , 11 , 12 , 13 ,
                                              14 , 15 };
 const uint8x16_t at = vceqq_u8( vld1q_u8( iota ) ,
                                 vdupq_n_u8( std::uint8_t( j ) ) );
 vst1q_u8( c.c , vbicq_u8( vld1q_u8( c.c ) , at ) );
#else
 c.c[ j ] = EMPTY;
#endif
 }

template< class K , class V , class H , class Eq >
template< class F >
void FlatHashTable< K , V , H , Eq >::rebuild( std::size_t G , F && first )
{
 FlatHashTable o( 0 , f_h , f_eq );         // the new places, in o
 o.f_slot = allocate( G , o.f_ctrl );
 o.f_cap = G * SLOTS;
 o.f_mask = G - 1;
 o.f_shift = 64 - std::countr_zero( G );
 o.f_max = bound( o.f_cap );
 // the places of the new table are only filled, in the order of each
 // group: the first empty place of the group g is its place n[ g ], and the
 // control bytes are written and never read, where reading them would wait
 // for the byte just written in the same group; the counts of up to 256
 // groups on the stack, with no call to the allocator
 constexpr std::size_t SMALL = 256;
 std::uint8_t local[ SMALL ];
 std::unique_ptr< std::uint8_t[] > big( G > SMALL ? new std::uint8_t[ G ]
                                                  : nullptr );
 std::uint8_t * const n = big ? big.get() : local;
 std::memset( n , 0 , G );
 auto place = [ & ]( std::uint64_t x ) {   // claim( x ) on o, by n
  std::size_t g = o.home( x );
  for( std::size_t r = 1 ; n[ g ] == SLOTS ; ++r ) {
   o.f_ctrl[ g ].c[ OVER ] |= bit( x );     // x goes past g
   g = ( g + r ) & o.f_mask;
   }
  return( g * GROUP + n[ g ]++ );
  };
 first( o , place );
 // the full places, group by group; a copy or a hash that throws leaves
 // this table as it was, and a pair that moves with no exception, of a key
 // whose hash throws none, is destroyed at once, while its memory is in
 // the cache, instead of by a walk of the old places after
 constexpr bool MOVED =
  std::is_nothrow_move_constructible_v< std::pair< K , V > > &&
  noexcept( std::declval< const H & >()( std::declval< const K & >() ) );
 auto move = [ & ]( std::size_t i ) {
  auto & kv = this->slot( i )->kv;
  const std::uint64_t x = mix( kv.first );
  const std::size_t j = place( x );
  std::construct_at( &o.slot( j )->kv , std::move_if_noexcept( kv ) );
  if constexpr( MOVED )
   std::destroy_at( &kv );
  o.ctrl( j ) = std::uint8_t( tag( x ) );
  ++o.f_size;
  };
 if constexpr( detail::kIndirectKey< K > ) {
  // the hash of a key reads memory at a random place, a miss per key in
  // the order of the places: the memory of a key is prefetched when the
  // walk of the places reaches it, and the key is moved AHEAD places later,
  // from a ring of the places in between on the stack, so that the misses
  // overlap with no list of the places
  constexpr std::size_t AHEAD = 8;
  std::size_t ring[ AHEAD ];
  std::size_t r = 0;
  each( [ & ]( std::size_t i ) {
   detail::prefetchKey( slot( i )->kv.first );
   if( r >= AHEAD )
    move( ring[ r % AHEAD ] );
   ring[ r++ % AHEAD ] = i;
   } );
  for( std::size_t q = r > AHEAD ? r - AHEAD : 0 ; q < r ; ++q )
   move( ring[ q % AHEAD ] );
  }
 else
  each( move );
 if constexpr( MOVED )
  release( false );                         // the old places alone
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
void FlatHashTable< K , V , H , Eq >::release( bool pairs ) noexcept
{
 if( f_cap == 0 )
  return;
 if constexpr( ! std::is_trivially_destructible_v< std::pair< K , V > > )
  if( pairs )
   each( [ & ]( std::size_t i ) { std::destroy_at( &slot( i )->kv ); } );
 deallocate( f_slot , f_ctrl , f_mask + 1 );
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
