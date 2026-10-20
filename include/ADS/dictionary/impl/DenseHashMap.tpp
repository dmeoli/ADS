/*--------------------------------------------------------------------------*/
/*-------------------------- File DenseHashMap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DenseHashMap [see DenseHashMap.h], with their
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
/*---------------------- CONSTRUCTION AND DESTRUCTION ----------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
DenseHashMap< K , V , H , Eq >::DenseHashMap( std::size_t n , H h , Eq eq )
 : f_h( std::move( h ) ) , f_eq( std::move( eq ) )
{
 if( n > 0 )
  reserve( n );
 }

template< class K , class V , class H , class Eq >
DenseHashMap< K , V , H , Eq >::DenseHashMap( const DenseHashMap & o )
 : f_kv( o.f_kv ) , f_h( o.f_h ) , f_eq( o.f_eq )
{
 if( o.f_cap == 0 )
  return;
 const std::size_t G = o.f_mask + 1;
 f_g.reset( static_cast< Group * >(
  ::operator new( G * sizeof( Group ) ,
                  std::align_val_t( alignof( Group ) ) ) ) );
 std::memcpy( static_cast< void * >( f_g.get() ) , o.f_ctrl ,
              G * sizeof( Group ) );
 f_ctrl = f_g.get();
 f_mask = o.f_mask;
 f_cap = o.f_cap;
 f_shift = o.f_shift;
 f_max = o.f_max;
 f_deleted = o.f_deleted;
 }

template< class K , class V , class H , class Eq >
DenseHashMap< K , V , H , Eq >::DenseHashMap( DenseHashMap && o ) noexcept
 : f_kv( std::move( o.f_kv ) ) , f_g( std::move( o.f_g ) ) ,
   f_ctrl( o.f_ctrl ) , f_mask( o.f_mask ) , f_cap( o.f_cap ) ,
   f_shift( o.f_shift ) , f_max( o.f_max ) , f_deleted( o.f_deleted ) ,
   f_h( std::move( o.f_h ) ) , f_eq( std::move( o.f_eq ) )
{
 o.f_kv.clear();
 o.f_ctrl = s_none;
 o.f_mask = o.f_cap = o.f_max = o.f_deleted = 0;
 o.f_shift = 63;
 }

template< class K , class V , class H , class Eq >
DenseHashMap< K , V , H , Eq > &
DenseHashMap< K , V , H , Eq >::operator=( DenseHashMap o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::swap( DenseHashMap & o ) noexcept
{
 using std::swap;
 swap( f_kv , o.f_kv );
 swap( f_g , o.f_g );
 swap( f_ctrl , o.f_ctrl );
 swap( f_mask , o.f_mask );
 swap( f_cap , o.f_cap );
 swap( f_shift , o.f_shift );
 swap( f_max , o.f_max );
 swap( f_deleted , o.f_deleted );
 swap( f_h , o.f_h );
 swap( f_eq , o.f_eq );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::Free::operator()( Group * g ) const
 noexcept
{
 ::operator delete( static_cast< void * >( g ) ,
                    std::align_val_t( alignof( Group ) ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE GROUPS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint64_t
DenseHashMap< K , V , H , Eq >::mix( const K & k ) const
{
 if constexpr( AVALANCHING )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( detail::mulx( std::uint64_t( f_h( k ) ) ,
                        0x9e3779b97f4a7c15ULL ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::uint8_t
DenseHashMap< K , V , H , Eq >::bit( std::uint64_t x )
{
 return( std::uint8_t( 1 << ( ( x >> 8 ) & 7 ) ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::size_t
DenseHashMap< K , V , H , Eq >::home( std::uint64_t x ) const
{
 return( std::size_t( x >> f_shift ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename DenseHashMap< K , V , H , Eq >::Mask
DenseHashMap< K , V , H , Eq >::match( const Group & c , std::uint32_t t )
{
#if ADS_FLAT_SIMD == 1
 const __m128i w =
  _mm_load_si128( reinterpret_cast< const __m128i * >( c.c ) );
 return( Mask( _mm_movemask_epi8(
          _mm_cmpeq_epi8( w , _mm_set1_epi32( int( t ) ) ) ) ) & PLACES );
#elif ADS_FLAT_SIMD == 2
 static constexpr std::uint8_t b[ 16 ] = { 1 , 2 , 4 , 8 , 16 , 32 , 64 ,
                                           128 , 1 , 2 , 4 , 8 , 16 , 32 ,
                                           64 , 128 };
 const uint8x16_t e = vandq_u8( vceqq_u8( vld1q_u8( c.c ) ,
                                vdupq_n_u8( std::uint8_t( t ) ) ) ,
                                vld1q_u8( b ) );
 return( ( Mask( vaddv_u8( vget_low_u8( e ) ) ) |
           ( Mask( vaddv_u8( vget_high_u8( e ) ) ) << 8 ) ) & PLACES );
#else
 // the bytes 0 to 7 and 8 to 15 as two words, the byte j in the bits 8 j
 // to 8 j + 7; a byte equal to t is 0 in w ^ b, whose low 7 bits plus
 // 0x7F carry into its bit 7 unless they are 0
 constexpr std::uint64_t L7 = 0x7F7F7F7F7F7F7F7FULL;
 const std::uint64_t b = ( t & 0xFF ) * 0x0101010101010101ULL;
 Mask m = 0;
 for( int h = 0 ; h < 2 ; ++h ) {
  std::uint64_t w = 0;
  for( int j = 7 ; j >= 0 ; --j )
   w = ( w << 8 ) | c.c[ 8 * h + j ];
  w ^= b;
  const std::uint64_t z = ~( ( ( w & L7 ) + L7 ) | w | L7 );
  m |= Mask( ( ( z >> 7 ) * 0x0102040810204080ULL ) >> 56 ) << ( 8 * h );
  }
 return( m & PLACES );
#endif
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE typename DenseHashMap< K , V , H , Eq >::Mask
DenseHashMap< K , V , H , Eq >::empties( const Group & c )
{
 return( match( c , EMPTY ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE void DenseHashMap< K , V , H , Eq >::vacate( Group & c ,
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

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::size_t
DenseHashMap< K , V , H , Eq >::locate( std::uint64_t x , const K & k ) const
{
 const std::size_t g = home( x );
 const Group & c = f_ctrl[ g ];
 for( Mask m = match( c , detail::flatTags[ x & 0xFF ] ) ; m ; m &= m - 1 ) {
  const std::size_t p = c.at[ std::countr_zero( m ) ];
  if( f_eq( f_kv[ p ].first , k ) ) [[likely]]
   return( p );
  }
 if( ! ( c.c[ OVER ] & bit( x ) ) ) [[likely]]
  return( ~std::size_t( 0 ) );              // no key of x went past g
 const auto [ h , j ] = further( x , g , k );
 return( h == ~std::size_t( 0 ) ? h : f_ctrl[ h ].at[ j ] );
 }

template< class K , class V , class H , class Eq >
std::pair< std::size_t , std::size_t >
DenseHashMap< K , V , H , Eq >::further( std::uint64_t x , std::size_t g ,
                                         KeyArg k ) const
{
 const std::uint32_t t = detail::flatTags[ x & 0xFF ];
 for( std::size_t i = 1 ; i <= f_mask ; ++i ) {
  g = ( g + i ) & f_mask;
  const Group & c = f_ctrl[ g ];
  for( Mask m = match( c , t ) ; m ; m &= m - 1 ) {
   const std::size_t j = std::size_t( std::countr_zero( m ) );
   if( f_eq( f_kv[ c.at[ j ] ].first , k ) )
    return( std::pair< std::size_t , std::size_t >( g , j ) );
   }
  if( ! ( c.c[ OVER ] & bit( x ) ) )
   break;
  }
 return( std::pair< std::size_t , std::size_t >( ~std::size_t( 0 ) ,
                                                 0 ) );
 }

template< class K , class V , class H , class Eq >
std::size_t DenseHashMap< K , V , H , Eq >::claim( std::uint64_t x )
{
 std::size_t g = home( x );
 for( std::size_t i = 1 ; ; ++i ) {
  if( const Mask m = empties( f_ctrl[ g ] ) )
   return( g * 16 + std::size_t( std::countr_zero( m ) ) );
  f_ctrl[ g ].c[ OVER ] |= bit( x );        // x goes past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
std::uint32_t & DenseHashMap< K , V , H , Eq >::holder( std::uint64_t x ,
                                                         std::size_t p )
{
 // the place whose position is p, by the inspection of x: no key compared
 const std::uint32_t t = detail::flatTags[ x & 0xFF ];
 std::size_t g = home( x );
 for( std::size_t i = 1 ; ; ++i ) {
  Group & c = f_ctrl[ g ];
  for( Mask m = match( c , t ) ; m ; m &= m - 1 ) {
   const int j = std::countr_zero( m );
   if( c.at[ j ] == p )
    return( c.at[ j ] );
   }
  ADS_INVARIANT( i <= f_mask );
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE std::optional< V >
DenseHashMap< K , V , H , Eq >::lookup( const K & k ) const
{
 const V * v = find( k );
 if( v )
  return( *v );
 return( std::nullopt );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE const V *
DenseHashMap< K , V , H , Eq >::find( const K & k ) const
{
 const std::size_t p = locate( mix( k ) , k );
 return( p == ~std::size_t( 0 ) ? nullptr : &f_kv[ p ].second );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE V * DenseHashMap< K , V , H , Eq >::find( const K & k )
{
 return( const_cast< V * >( std::as_const( *this ).find( k ) ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq >::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq >::insert( const K & k , const V & v )
{
 return( put( k , v ) );
 }

template< class K , class V , class H , class Eq >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq >::insert( K && k , V && v )
{
 return( put( std::move( k ) , std::move( v ) ) );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq >::put( KK && k , VV && v )
{
 const std::uint64_t x = mix( k );
 if( const std::size_t p = locate( x , k ) ; p != ~std::size_t( 0 ) ) {
  f_kv[ p ].second = std::forward< VV >( v );  // replaced in place
  ADS_INVARIANT( consistent( x ) );
  return( false );
  }
 ADS_PRE( f_kv.size() < 0xFFFFFFFFULL );
 if( ( f_kv.size() + f_deleted >= f_max ) && ! recount() ) [[unlikely]]
  grow();
 // the pair first, so that an exception leaves the index as it was
 f_kv.emplace_back( std::piecewise_construct ,
                    std::forward_as_tuple( std::forward< KK >( k ) ) ,
                    std::forward_as_tuple( std::forward< VV >( v ) ) );
 const std::size_t g = home( x );
 const Mask e = empties( f_ctrl[ g ] );
 const std::size_t i = e ? g * 16 + std::size_t( std::countr_zero( e ) )
                         : claim( x );
 Group & c = f_ctrl[ i / 16 ];
 // a place counted, by no branch past the test of the bound, 0 with no
 // removals [see FlatHashTable.tpp]
 if( f_deleted ) [[unlikely]]
  f_deleted -= std::size_t( c.c[ OVER ] != 0 );
 c.c[ i % 16 ] = std::uint8_t( detail::flatTags[ x & 0xFF ] );
 c.at[ i % 16 ] = std::uint32_t( f_kv.size() - 1 );
 ADS_POST( locate( x , f_kv.back().first ) == f_kv.size() - 1 );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool DenseHashMap< K , V , H , Eq >::remove( const K & k )
{
 // the first group alone, the common case [see FlatHashTable.tpp for why
 // the group is written whole]
 const std::uint64_t x = mix( k );
 const std::size_t g = home( x );
 Group & c = f_ctrl[ g ];
 const std::uint8_t over = c.c[ OVER ];
 for( Mask m = match( c , detail::flatTags[ x & 0xFF ] ) ; m ; m &= m - 1 ) {
  const std::size_t j = std::size_t( std::countr_zero( m ) ) ,
                    p = c.at[ j ];
  if( f_eq( f_kv[ p ].first , k ) ) [[likely]] {
   unlink( c , j , p );
   ADS_POST( locate( x , k ) == ~std::size_t( 0 ) );
   ADS_INVARIANT( consistent( x ) );
   return( true );
   }
  }
 if( ! ( over & bit( x ) ) ) [[likely]]
  return( false );
 const auto [ h , j ] = further( x , g , k );
 if( h == ~std::size_t( 0 ) )
  return( false );
 unlink( f_ctrl[ h ] , j , f_ctrl[ h ].at[ j ] );
 ADS_POST( locate( x , k ) == ~std::size_t( 0 ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::unlink( Group & c , std::size_t j ,
                                             std::size_t p )
{
 vacate( c , j );
 ++f_deleted;                               // a bound, see recount()
 const std::size_t last = f_kv.size() - 1;
 if( p != last ) {                          // the last pair into p
  holder( mix( f_kv[ last ].first ) , last ) = std::uint32_t( p );
  f_kv[ p ] = std::move( f_kv[ last ] );
  }
 f_kv.pop_back();
 }

template< class K , class V , class H , class Eq >
std::size_t DenseHashMap< K , V , H , Eq >::size( void ) const
{
 return( f_kv.size() );
 }

template< class K , class V , class H , class Eq >
bool DenseHashMap< K , V , H , Eq >::empty( void ) const
{
 return( f_kv.empty() );
 }

template< class K , class V , class H , class Eq >
std::size_t DenseHashMap< K , V , H , Eq >::capacity( void ) const
{
 return( f_cap );
 }

template< class K , class V , class H , class Eq >
double DenseHashMap< K , V , H , Eq >::load( void ) const
{
 return( f_cap ? double( f_kv.size() ) / double( f_cap ) : 0.0 );
 }

template< class K , class V , class H , class Eq >
std::size_t DenseHashMap< K , V , H , Eq >::deleted( void ) const
{
 std::size_t d = 0;                         // the empty places counted
 for( std::size_t g = 0 ; f_cap && ( g <= f_mask ) ; ++g )
  if( f_ctrl[ g ].c[ OVER ] )
   d += std::size_t( std::popcount( empties( f_ctrl[ g ] ) ) );
 return( d );
 }

template< class K , class V , class H , class Eq >
bool DenseHashMap< K , V , H , Eq >::recount( void )
{
 f_deleted = deleted();
 return( f_kv.size() + f_deleted + f_cap / 16 < f_max );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::reserve( std::size_t n )
{
 f_kv.reserve( n );
 const std::size_t G = groups( n );
 if( G * SLOTS > f_cap )
  rebuild( G );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::clear( void )
{
 f_kv.clear();
 if( f_cap )
  std::memset( static_cast< void * >( f_ctrl ) , 0 ,
               ( f_mask + 1 ) * sizeof( Group ) );
 f_deleted = 0;
 }

template< class K , class V , class H , class Eq >
template< class F >
void DenseHashMap< K , V , H , Eq >::forEach( F && f ) const
{
 for( const auto & kv : f_kv )
  f( kv.first , kv.second );
 }

template< class K , class V , class H , class Eq >
const std::pair< K , V > * DenseHashMap< K , V , H , Eq >::data( void )
 const
{
 return( f_kv.data() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE REBUILD -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::grow( void )
{
 const std::size_t G = f_cap ? f_mask + 1 : 0;
 rebuild( f_kv.size() + 1 > f_max / 2 ? ( G ? 2 * G : 2 ) : G );
 }

template< class K , class V , class H , class Eq >
void DenseHashMap< K , V , H , Eq >::rebuild( std::size_t G )
{
 std::unique_ptr< Group[] , Free > g( static_cast< Group * >(
  ::operator new( G * sizeof( Group ) ,
                  std::align_val_t( alignof( Group ) ) ) ) );
 Group * const c = g.get();
 std::memset( static_cast< void * >( c ) , 0 , G * sizeof( Group ) );
 const int shift = 64 - std::countr_zero( G );
 // the groups only filled, each in the order of its places, the count of
 // a group in a byte apart [see FlatHashTable.tpp]
 std::unique_ptr< std::uint8_t[] > n( new std::uint8_t[ G ]() );
 for( std::size_t p = 0 ; p < f_kv.size() ; ++p ) {
  const std::uint64_t x = mix( f_kv[ p ].first );
  std::size_t h = std::size_t( x >> shift );
  for( std::size_t r = 1 ; n[ h ] == SLOTS ; ++r ) {
   c[ h ].c[ OVER ] |= bit( x );            // x goes past h
   h = ( h + r ) & ( G - 1 );
   }
  c[ h ].c[ n[ h ] ] = std::uint8_t( detail::flatTags[ x & 0xFF ] );
  c[ h ].at[ n[ h ]++ ] = std::uint32_t( p );
  }
 f_g = std::move( g );
 f_ctrl = c;
 f_mask = G - 1;
 f_cap = G * SLOTS;
 f_shift = shift;
 f_max = bound( f_cap );
 f_deleted = 0;
 }

template< class K , class V , class H , class Eq >
std::size_t DenseHashMap< K , V , H , Eq >::groups( std::size_t n )
{
 if( n == 0 )
  return( 0 );
 std::size_t G = 2;
 while( n > bound( G * SLOTS ) )
  G *= 2;
 return( G );
 }

template< class K , class V , class H , class Eq >
constexpr std::size_t DenseHashMap< K , V , H , Eq >::bound( std::size_t m )
{
 return( m - m / 8 );
 }

template< class K , class V , class H , class Eq >
bool DenseHashMap< K , V , H , Eq >::consistent( std::uint64_t x ) const
{
 const std::size_t n = f_kv.size();
 if( ( n + f_deleted > f_max ) || ( f_max != bound( f_cap ) ) )
  return( false );
 // the place j of the group g reached by the inspection of its key
 auto reached = [ & ]( std::size_t g , std::size_t j ) {
  const std::size_t p = f_ctrl[ g ].at[ j ];
  if( p >= n )
   return( false );
  const std::uint64_t y = mix( f_kv[ p ].first );
  if( f_ctrl[ g ].c[ j ] != std::uint8_t( detail::flatTags[ y & 0xFF ] ) )
   return( false );
  std::size_t h = home( y );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   if( h == g )
    return( true );
   if( ! ( f_ctrl[ h ].c[ OVER ] & bit( y ) ) )
    return( false );
   h = ( h + r ) & f_mask;
   }
  return( false );
  };
 if( f_cap == 0 )
  return( n == 0 );
 auto fulls = [ & ]( std::size_t g ) {
  return( Mask( ~empties( f_ctrl[ g ] ) ) & PLACES );
  };
 if( f_cap > 1024 ) {
  std::size_t g = home( x );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   for( Mask m = fulls( g ) ; m ; m &= m - 1 )
    if( ! reached( g , std::size_t( std::countr_zero( m ) ) ) )
     return( false );
   if( ! ( f_ctrl[ g ].c[ OVER ] & bit( x ) ) )
    break;
   g = ( g + r ) & f_mask;
   }
  return( true );
  }
 std::vector< char > held( n , 0 );         // each position exactly once
 std::size_t full = 0;
 for( std::size_t g = 0 ; g <= f_mask ; ++g )
  for( Mask m = fulls( g ) ; m ; m &= m - 1 ) {
   const std::size_t j = std::size_t( std::countr_zero( m ) );
   if( ! reached( g , j ) || held[ f_ctrl[ g ].at[ j ] ] )
    return( false );
   held[ f_ctrl[ g ].at[ j ] ] = 1;
   ++full;
   }
 return( full == n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File DenseHashMap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
