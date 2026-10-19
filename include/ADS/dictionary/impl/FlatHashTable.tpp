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
/*---------------------- CONSTRUCTION AND DESTRUCTION ----------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( std::size_t n , H h , Eq eq )
 : f_h( std::move( h ) ) , f_eq( std::move( eq ) )
{
 if( n > 0 )
  rebuild( places( n ) );
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( const FlatHashTable & o )
 : f_h( o.f_h ) , f_eq( o.f_eq )
{
 if( o.f_cap == 0 )
  return;
 const std::size_t G = o.f_cap / GROUP;
 auto ctrl = std::make_unique< std::uint8_t[] >( o.f_cap + G );
 auto slot = std::make_unique< Slot[] >( o.f_cap );
 std::memcpy( ctrl.get() , o.f_ctrl , o.f_cap + G );
 std::size_t i = 0;
 try {                                      // the same places, copied
  for( ; i < o.f_cap ; ++i )
   if( ctrl[ cb( i ) ] < EMPTY )
    std::construct_at( &slot[ i ].kv , o.f_slot[ i ].kv );
  }
 catch( ... ) {
  while( i-- > 0 )
   if( ctrl[ cb( i ) ] < EMPTY )
    std::destroy_at( &slot[ i ].kv );
  throw;
  }
 f_ctrl = ctrl.release();
 f_slot = slot.release();
 f_cap = o.f_cap;
 f_mask = o.f_mask;
 f_shift = o.f_shift;
 f_size = o.f_size;
 f_deleted = o.f_deleted;
 }

template< class K , class V , class H , class Eq >
FlatHashTable< K , V , H , Eq >::FlatHashTable( FlatHashTable && o ) noexcept
 : f_ctrl( o.f_ctrl ) , f_slot( o.f_slot ) , f_cap( o.f_cap ) ,
   f_mask( o.f_mask ) , f_shift( o.f_shift ) , f_size( o.f_size ) ,
   f_deleted( o.f_deleted ) , f_h( std::move( o.f_h ) ) ,
   f_eq( std::move( o.f_eq ) )
{
 o.f_ctrl = s_none;
 o.f_slot = nullptr;
 o.f_cap = o.f_mask = o.f_size = o.f_deleted = 0;
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
 swap( f_size , o.f_size );
 swap( f_deleted , o.f_deleted );
 swap( f_h , o.f_h );
 swap( f_eq , o.f_eq );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
std::optional< V > FlatHashTable< K , V , H , Eq >::lookup( const K & k )
 const
{
 const V * v = find( k );
 if( v )
  return( *v );
 return( std::nullopt );
 }

template< class K , class V , class H , class Eq >
const V * FlatHashTable< K , V , H , Eq >::find( const K & k ) const
{
 const std::uint64_t x = mix( k );         // locate( x , k ), inlined
 const std::uint8_t t = tag( x );
 std::size_t g = home( x );
 for( std::size_t i = 0 ; ; ) {
  const std::uint64_t w = group( g );
  for( std::uint64_t m = matches( w , t ) ; m ; m &= m - 1 ) {
   const std::size_t j = g * GROUP + lowest( m );
   if( f_eq( f_slot[ j ].kv.first , k ) )
    return( &f_slot[ j ].kv.second );
   }
  if( ! ( f_ctrl[ ob( g ) ] & bit( x ) ) || ( ++i > f_mask ) )
   return( nullptr );
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
V * FlatHashTable< K , V , H , Eq >::find( const K & k )
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
 drop( std::size_t( s - f_slot ) );
 ADS_POST( locate( x , k ) == nullptr );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::contains( const K & k ) const
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
 const std::size_t m = places( n );
 if( m > f_cap )
  rebuild( m );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::clear( void )
{
 for( std::size_t i = 0 ; i < f_cap ; ++i )
  if( f_ctrl[ cb( i ) ] < EMPTY )
   std::destroy_at( &f_slot[ i ].kv );
 if( f_cap ) {
  std::memset( f_ctrl , EMPTY , f_cap + f_cap / GROUP );
  for( std::size_t g = 0 ; g <= f_mask ; ++g )
   f_ctrl[ ob( g ) ] = 0;
  }
 f_size = f_deleted = 0;
 }

template< class K , class V , class H , class Eq >
template< class F >
void FlatHashTable< K , V , H , Eq >::forEach( F && f ) const
{
 for( std::size_t i = 0 ; i < f_cap ; ++i )
  if( f_ctrl[ cb( i ) ] < EMPTY )
   f( std::as_const( f_slot[ i ].kv.first ) ,
      std::as_const( f_slot[ i ].kv.second ) );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::const_iterator
FlatHashTable< K , V , H , Eq >::erase( const_iterator it )
{
 const std::size_t i = it.place();
 ADS_PRE( ( i < f_cap ) && ( f_ctrl[ cb( i ) ] < EMPTY ) );
 [[maybe_unused]] const std::uint64_t x =       // for the invariant only
  ADS_CONTRACTS >= 3 ? mix( f_slot[ i ].kv.first ) : 0;
 drop( i );
 ADS_INVARIANT( consistent( x ) );
 return( const_iterator( this , i + 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PLACES --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
std::uint64_t FlatHashTable< K , V , H , Eq >::mix( const K & k ) const
{
 return( std::uint64_t( f_h( k ) ) * 0x9e3779b97f4a7c15ULL );
 }

template< class K , class V , class H , class Eq >
std::uint8_t FlatHashTable< K , V , H , Eq >::tag( std::uint64_t x )
{
 return( std::uint8_t( ( x ^ ( x >> 32 ) ) & 0x7F ) );
 }

template< class K , class V , class H , class Eq >
std::uint8_t FlatHashTable< K , V , H , Eq >::bit( std::uint64_t x )
{
 return( std::uint8_t( 1 << ( ( x >> 39 ) & 7 ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::home( std::uint64_t x ) const
{
 return( std::size_t( x >> f_shift ) & f_mask );
 }

template< class K , class V , class H , class Eq >
std::uint64_t FlatHashTable< K , V , H , Eq >::group( std::size_t g ) const
{
 return( word( f_ctrl + g * ( GROUP + 1 ) ) );
 }

template< class K , class V , class H , class Eq >
std::uint64_t FlatHashTable< K , V , H , Eq >::word( const std::uint8_t * c )
{
 std::uint64_t w;
 std::memcpy( &w , c , GROUP );
 if constexpr( std::endian::native == std::endian::big ) {
  std::uint64_t r = 0;                      // the place 0 in the low byte
  for( std::size_t j = 0 ; j < GROUP ; ++j , w >>= 8 )
   r = ( r << 8 ) | ( w & 0xFF );
  w = r;
  }
 return( w );
 }

template< class K , class V , class H , class Eq >
std::uint64_t FlatHashTable< K , V , H , Eq >::matches( std::uint64_t w ,
                                                       std::uint8_t t )
{
 constexpr std::uint64_t LO = 0x0101010101010101ULL;
 constexpr std::uint64_t HI = 0x8080808080808080ULL;
 const std::uint64_t x = w ^ ( LO * t );   // the bytes equal to t are 0
 return( ( x - LO ) & ~x & HI );
 }

template< class K , class V , class H , class Eq >
std::uint64_t FlatHashTable< K , V , H , Eq >::empties( std::uint64_t w )
{
 return( w & 0x8080808080808080ULL );       // 0x80, the only one >= 0x80
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::lowest( std::uint64_t m )
{
 return( std::size_t( std::countr_zero( m ) ) >> 3 );
 }

template< class K , class V , class H , class Eq >
constexpr std::size_t FlatHashTable< K , V , H , Eq >::cb( std::size_t i )
{
 return( i + i / GROUP );
 }

template< class K , class V , class H , class Eq >
constexpr std::size_t FlatHashTable< K , V , H , Eq >::ob( std::size_t g )
{
 return( g * ( GROUP + 1 ) + GROUP );
 }

template< class K , class V , class H , class Eq >
typename FlatHashTable< K , V , H , Eq >::Slot *
FlatHashTable< K , V , H , Eq >::locate( std::uint64_t x ,
                                                    const K & k ) const
{
 const std::uint8_t t = tag( x );
 std::size_t g = home( x );
 for( std::size_t i = 0 ; ; ) {
  const std::uint64_t w = group( g );
  for( std::uint64_t m = matches( w , t ) ; m ; m &= m - 1 ) {
   const std::size_t j = g * GROUP + lowest( m );
   if( f_eq( f_slot[ j ].kv.first , k ) )
    return( f_slot + j );
   }
  if( ! ( f_ctrl[ ob( g ) ] & bit( x ) ) || ( ++i > f_mask ) )
   return( nullptr );                       // no key of x went past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::claim( std::uint64_t x )
{
 std::size_t g = home( x );
 for( std::size_t i = 1 ; ; ++i ) {
  if( const std::uint64_t m = empties( group( g ) ) )
   return( g * GROUP + lowest( m ) );
  f_ctrl[ ob( g ) ] |= bit( x );            // x goes past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
bool FlatHashTable< K , V , H , Eq >::put( KK && k , VV && v )
{
 // one inspection searches k and finds the first empty place, setting the
 // bit of k in the full groups before it, which is right if k is absent
 // and changes nothing if k is there (it went past them)
 const std::uint64_t x = mix( k );
 const std::uint8_t t = tag( x ) , b = bit( x );
 constexpr std::size_t NONE = ~std::size_t( 0 );   // no empty place yet
 std::size_t g = home( x ) , i = NONE , r = 0;
 for( ; ; ) {
  const std::uint64_t w = group( g );
  for( std::uint64_t m = matches( w , t ) ; m ; m &= m - 1 ) {
   const std::size_t j = g * GROUP + lowest( m );
   if( f_eq( f_slot[ j ].kv.first , k ) ) {   // replaced in place
    f_slot[ j ].kv.second = std::forward< VV >( v );
    ADS_INVARIANT( consistent( x ) );
    return( false );
    }
   }
  const bool past = f_ctrl[ ob( g ) ] & b;
  if( i == NONE ) {
   if( const std::uint64_t e = empties( w ) )
    i = g * GROUP + lowest( e );
   else
    f_ctrl[ ob( g ) ] |= b;
   }
  if( ! past || ( ++r > f_mask ) )
   break;
  g = ( g + r ) & f_mask;
  }
 while( i == NONE ) {                       // k is absent: on to a place
  g = ( g + ++r ) & f_mask;
  if( const std::uint64_t e = empties( group( g ) ) )
   i = g * GROUP + lowest( e );
  else
   f_ctrl[ ob( g ) ] |= b;
  }
 if( f_size + f_deleted >= f_cap - f_cap / 8 )
  i = grow( x , std::pair< K , V >( std::forward< KK >( k ) ,
                                    std::forward< VV >( v ) ) );
 else {
  std::construct_at( &f_slot[ i ].kv , std::forward< KK >( k ) ,
                     std::forward< VV >( v ) );
  if( f_deleted && f_ctrl[ ob( i / GROUP ) ] )   // a place still counted
   --f_deleted;
  }
 f_ctrl[ cb( i ) ] = t;
 ++f_size;
 ADS_POST( locate( x , f_slot[ i ].kv.first ) == f_slot + i );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::grow( std::uint64_t x ,
                                                  std::pair< K , V > && kv )
{
 rebuild( f_size + 1 > ( f_cap - f_cap / 8 ) / 2 ?
          ( f_cap ? 2 * f_cap : GROUP ) : f_cap );
 const std::size_t i = claim( x );
 std::construct_at( &f_slot[ i ].kv , std::move( kv ) );
 return( i );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::drop( std::size_t i )
{
 std::destroy_at( &f_slot[ i ].kv );
 f_ctrl[ cb( i ) ] = EMPTY;
 if( f_ctrl[ ob( i / GROUP ) ] )            // inspections pass here
  ++f_deleted;
 --f_size;
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::rebuild( std::size_t m )
{
 const std::size_t G = m / GROUP;
 std::unique_ptr< std::uint8_t[] > ctrl( new std::uint8_t[ m + G ] );
 auto slot = std::make_unique< Slot[] >( m );
 std::memset( ctrl.get() , EMPTY , m + G );
 FlatHashTable o( 0 , f_h , f_eq );         // the new places, in o
 o.f_ctrl = ctrl.release();
 o.f_slot = slot.release();
 o.f_cap = m;
 o.f_mask = G - 1;
 o.f_shift = G > 1 ? 64 - std::countr_zero( G ) : 63;
 for( std::size_t g = 0 ; g < G ; ++g )
  o.f_ctrl[ ob( g ) ] = 0;
 // the full places, group by group; a copy that throws leaves this table
 // as it was
 for( std::size_t g = 0 ; g < f_cap / GROUP ; ++g )
  for( std::uint64_t f = ~group( g ) & 0x8080808080808080ULL ; f ;
       f &= f - 1 ) {
   const std::size_t i = g * GROUP + lowest( f );
   const std::uint64_t x = mix( f_slot[ i ].kv.first );
   const std::size_t j = o.claim( x );
   std::construct_at( &o.f_slot[ j ].kv ,
                      std::move_if_noexcept( f_slot[ i ].kv ) );
   o.f_ctrl[ cb( j ) ] = tag( x );
   ++o.f_size;
   }
 swap( o );
 }

template< class K , class V , class H , class Eq >
std::size_t FlatHashTable< K , V , H , Eq >::places( std::size_t n )
{
 if( n == 0 )
  return( 0 );
 std::size_t m = GROUP;
 while( n > m - m / 8 )
  m *= 2;
 return( m );
 }

template< class K , class V , class H , class Eq >
void FlatHashTable< K , V , H , Eq >::release( void ) noexcept
{
 if( f_cap == 0 )
  return;
 if constexpr( ! std::is_trivially_destructible_v< std::pair< K , V > > )
  for( std::size_t i = 0 ; i < f_cap ; ++i )
   if( f_ctrl[ cb( i ) ] < EMPTY )
    std::destroy_at( &f_slot[ i ].kv );
 delete[] f_ctrl;
 delete[] f_slot;
 f_ctrl = s_none;
 f_slot = nullptr;
 f_cap = f_mask = f_size = f_deleted = 0;
 f_shift = 63;
 }

template< class K , class V , class H , class Eq >
bool FlatHashTable< K , V , H , Eq >::consistent( std::uint64_t x ) const
{
 if( 8 * ( f_size + f_deleted ) > 7 * f_cap )
  return( false );
 // the place i reached by the inspection of its key: the bit of the key
 // set in each group before it
 auto reached = [ & ]( std::size_t i ) {
  const std::uint64_t y = mix( f_slot[ i ].kv.first );
  if( f_ctrl[ cb( i ) ] != tag( y ) )
   return( false );
  std::size_t g = home( y );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   if( g == i / GROUP )
    return( true );
   if( ! ( f_ctrl[ ob( g ) ] & bit( y ) ) )
    return( false );
   g = ( g + r ) & f_mask;
   }
  return( false );
  };
 if( f_cap > 1024 ) {
  std::size_t g = home( x );
  for( std::size_t r = 1 ; r <= f_mask + 1 ; ++r ) {
   for( std::size_t j = g * GROUP ; j < ( g + 1 ) * GROUP ; ++j )
    if( ( f_ctrl[ cb( j ) ] < EMPTY ) && ! reached( j ) )
     return( false );
   if( ! ( f_ctrl[ ob( g ) ] & bit( x ) ) )
    break;
   g = ( g + r ) & f_mask;
   }
  return( true );
  }
 std::size_t full = 0;
 for( std::size_t i = 0 ; i < f_cap ; ++i )
  if( f_ctrl[ cb( i ) ] < EMPTY ) {
   ++full;
   if( ! reached( i ) )
    return( false );
   }
  else if( f_ctrl[ cb( i ) ] != EMPTY )
   return( false );
 return( full == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FlatHashTable.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
