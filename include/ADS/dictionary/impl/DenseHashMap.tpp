/*--------------------------------------------------------------------------*/
/*-------------------------- File DenseHashMap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DenseHashMap [see DenseHashMap.h], with their
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

namespace ADS
{

ADS_ABI_BEGIN

/*--------------------------------------------------------------------------*/
/*---------------------- CONSTRUCTION AND DESTRUCTION ----------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A , class P >
template< class... O >
DenseHashMap< K , V , H , Eq , A , P >::DenseHashMap( std::size_t n , H h ,
						      Eq eq ,
						 const A & alloc , O &... o )
    : f_kv( alloc ) , f_h( std::move( h ) ) , f_eq( std::move( eq ) ) ,
      f_sub( o... )
{
 if( n > 0 )
  reserve( n );
 }

template< class K , class V , class H , class Eq , class A , class P >
DenseHashMap< K , V , H , Eq , A , P >::DenseHashMap( const DenseHashMap & o )
    : f_kv( o.f_kv ) , f_h( o.f_h ) , f_eq( o.f_eq )
{
 copyGroups( o );
 }

template< class K , class V , class H , class Eq , class A , class P >
DenseHashMap< K , V , H , Eq , A , P >::DenseHashMap( const DenseHashMap & o ,
						 const A & alloc )
    : f_kv( o.f_kv , alloc ) , f_h( o.f_h ) , f_eq( o.f_eq )
{
 copyGroups( o );
 }

template< class K , class V , class H , class Eq , class A , class P >
DenseHashMap< K , V , H , Eq , A , P >::DenseHashMap( DenseHashMap && o ,
						 const A & alloc )
    : f_kv( std::move( o.f_kv ) , alloc ) , f_h( o.f_h ) , f_eq( o.f_eq )
{
 copyGroups( o );  // the positions of the pairs did not change
 o.clear();
 }

template< class K , class V , class H , class Eq , class A , class P >
inline void
DenseHashMap< K , V , H , Eq , A , P >::copyGroups( const DenseHashMap & o )
{
 if( o.f_cap == 0 )
  return;
 const std::size_t G = o.f_mask + 1;
 if constexpr( STD )
  f_g.reset( static_cast< Group * >( ::operator new(
    G * sizeof( Group ) , std::align_val_t( alignof( Group ) ) ) ) );
 else
  f_g.resize( G );
 std::memcpy( static_cast< void * >( first( f_g ) ) , o.f_ctrl ,
	      G * sizeof( Group ) );
 f_ctrl = first( f_g );
 f_mask = o.f_mask;
 f_cap = o.f_cap;
 f_shift = o.f_shift;
 f_max = o.f_max;
 f_deleted = o.f_deleted;
 }

template< class K , class V , class H , class Eq , class A , class P >
DenseHashMap< K , V , H , Eq , A ,
	      P >::DenseHashMap( DenseHashMap && o ) noexcept
    : f_kv( std::move( o.f_kv ) ) , f_g( std::move( o.f_g ) ) ,
      f_ctrl( o.f_ctrl ) , f_mask( o.f_mask ) , f_cap( o.f_cap ) ,
      f_shift( o.f_shift ) , f_max( o.f_max ) , f_deleted( o.f_deleted ) ,
      f_h( std::move( o.f_h ) ) , f_eq( std::move( o.f_eq ) )
{
 o.f_kv.clear();
 o.f_ctrl = s_none;
 o.f_mask = o.f_cap = o.f_max = o.f_deleted = 0;
 o.f_shift = 63;
 if constexpr( ! Subject::none )
  if( ! f_kv.empty() )
   o.tellCleared();
 }

template< class K , class V , class H , class Eq , class A , class P >
DenseHashMap< K , V , H , Eq , A , P > &
DenseHashMap< K , V , H , Eq , A , P >::operator=( DenseHashMap o ) noexcept
 requires detail::alwaysEqualAlloc< A >
{
 swap( o );
 return( *this );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::swap( DenseHashMap & o ) noexcept(
  detail::swapNoexcept< A > )
{
 detail::swapCheck( f_kv.get_allocator() , o.f_kv.get_allocator() );
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
 if constexpr( ! Subject::none ) {
  tellRebuilt();
  o.tellRebuilt();
  }
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::Free::operator()(
  Group * g ) const noexcept
{
 ::operator delete( static_cast< void * >( g ) ,
		    std::align_val_t( alignof( Group ) ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE GROUPS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE std::uint64_t
DenseHashMap< K , V , H , Eq , A , P >::mix( const K & k ) const
{
 if constexpr( AVALANCHING )
  return( detail::hash64( f_h , k ) );
 else
  return( detail::mulx( detail::hash64( f_h , k ) , 0x9e3779b97f4a7c15ULL ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE std::uint8_t
DenseHashMap< K , V , H , Eq , A , P >::bit( std::uint64_t x )
{
 return( std::uint8_t( 1 << ( ( x >> 8 ) & 7 ) ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE std::size_t
DenseHashMap< K , V , H , Eq , A , P >::home( std::uint64_t x ) const
{
 return( std::size_t( x >> f_shift ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE typename DenseHashMap< K , V , H , Eq , A , P >::Mask
DenseHashMap< K , V , H , Eq , A , P >::match( const Group & c ,
					       std::uint32_t t )
{
#if ADS_FLAT_SIMD == 1
 return( Mask( detail::simd::here::bytesEqual16( c.c , std::uint8_t( t ) ) ) &
	  PLACES );
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

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE typename DenseHashMap< K , V , H , Eq , A , P >::Mask
DenseHashMap< K , V , H , Eq , A , P >::empties( const Group & c )
{
 return( match( c , EMPTY ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE void
DenseHashMap< K , V , H , Eq , A , P >::vacate( Group & c , std::size_t j )
{
#if ADS_FLAT_SIMD == 1
 detail::simd::here::clearByte16( c.c , j );
#else
 c.c[ j ] = EMPTY;
#endif
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE std::size_t
DenseHashMap< K , V , H , Eq , A , P >::locate( std::uint64_t x ,
					   const K & k ) const
{
 const std::size_t g = home( x );
 const Group & c = f_ctrl[ g ];
 for( Mask m = match( c , detail::flatTags[ x & 0xFF ] ) ; m ; m &= m - 1 ) {
  const std::size_t p = c.at[ std::countr_zero( m ) ];
  if( f_eq( f_kv[ p ].first , k ) ) [[likely]]
   return( p );
  }
 if( ! ( c.c[ OVER ] & bit( x ) ) ) [[likely]]
  return( ~std::size_t( 0 ) );  // no key of x went past g
 const auto [ h , j ] = further( x , g , k );
 return( h == ~std::size_t( 0 ) ? h : f_ctrl[ h ].at[ j ] );
 }

template< class K , class V , class H , class Eq , class A , class P >
std::pair< std::size_t , std::size_t >
DenseHashMap< K , V , H , Eq , A , P >::further( std::uint64_t x ,
						 std::size_t g ,
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
 return( std::pair< std::size_t , std::size_t >( ~std::size_t( 0 ) , 0 ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
std::size_t DenseHashMap< K , V , H , Eq , A , P >::claim( std::uint64_t x )
{
 std::size_t g = home( x );
 for( std::size_t i = 1 ; ; ++i ) {
  if( const Mask m = empties( f_ctrl[ g ] ) )
   return( g * 16 + std::size_t( std::countr_zero( m ) ) );
  f_ctrl[ g ].c[ OVER ] |= bit( x );  // x goes past g
  g = ( g + i ) & f_mask;
  }
 }

template< class K , class V , class H , class Eq , class A , class P >
std::uint32_t & DenseHashMap< K , V , H , Eq , A ,
			      P >::holder( std::uint64_t x ,
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

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE std::optional< V >
DenseHashMap< K , V , H , Eq , A , P >::lookup( const K & k ) const
{
 const V * v = find( k );
 if( v )
  return( *v );
 return( std::nullopt );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE const V *
DenseHashMap< K , V , H , Eq , A , P >::find( const K & k ) const
{
 const std::size_t p = locate( mix( k ) , k );
 return( p == ~std::size_t( 0 ) ? nullptr : &f_kv[ p ].second );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE V * DenseHashMap< K , V , H , Eq , A , P >::find( const K & k )
{
 return( const_cast< V * >( std::as_const( *this ).find( k ) ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq , A , P >::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE bool DenseHashMap< K , V , H , Eq , A ,
				   P >::insert( const K & k ,
								const V & v )
{
 if constexpr( Subject::template listened< Inserted > ||
	       Subject::template listened< Assigned > )
  if( f_sub.template live< Inserted >() || f_sub.template live< Assigned >() )
    [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( putCold( k , v ) );
   else
    return( putTold( k , v ) );
   }
 return( put( k , v ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE bool DenseHashMap< K , V , H , Eq , A , P >::insert( K && k ,
								V && v )
{
 if constexpr( Subject::template listened< Inserted > ||
	       Subject::template listened< Assigned > )
  if( f_sub.template live< Inserted >() || f_sub.template live< Assigned >() )
    [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( putCold( std::move( k ) , std::move( v ) ) );
   else
    return( putTold( std::move( k ) , std::move( v ) ) );
   }
 return( put( std::move( k ) , std::move( v ) ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
template< class KK , class VV >
ADS_FLAT_INLINE bool DenseHashMap< K , V , H , Eq , A , P >::put( KK && k ,
							     VV && v )
{
 return(
   place< true >( std::forward< KK >( k ) , std::forward< VV >( v ) ).second );
 }

template< class K , class V , class H , class Eq , class A , class P >
template< class... Args >
ADS_FLAT_INLINE std::pair< V * , bool >
DenseHashMap< K , V , H , Eq , A , P >::try_emplace( const K & k ,
						     Args &&... a )
{
 if constexpr( Subject::template listened< Inserted > )
  if( f_sub.template live< Inserted >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( emplaceCold( k , std::forward< Args >( a )... ) );
   else
    return( emplaceTold( k , std::forward< Args >( a )... ) );
   }
 return( place< false >( k , std::forward< Args >( a )... ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
template< class... Args >
ADS_FLAT_INLINE std::pair< V * , bool >
DenseHashMap< K , V , H , Eq , A , P >::try_emplace( K && k , Args &&... a )
{
 if constexpr( Subject::template listened< Inserted > )
  if( f_sub.template live< Inserted >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( emplaceCold( std::move( k ) , std::forward< Args >( a )... ) );
   else
    return( emplaceTold( std::move( k ) , std::forward< Args >( a )... ) );
   }
 return( place< false >( std::move( k ) , std::forward< Args >( a )... ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
template< bool Assign , class KK , class... Args >
ADS_FLAT_INLINE std::pair< V * , bool >
DenseHashMap< K , V , H , Eq , A , P >::place( KK && k , Args &&... a )
{
 const std::uint64_t x = mix( k );
 if( const std::size_t p = locate( x , k ); p != ~std::size_t( 0 ) ) {
  if constexpr( Assign )  // replaced in place
   ( ( f_kv[ p ].second = std::forward< Args >( a ) ) , ... );
  ADS_INVARIANT( consistent( x ) );
  return( std::pair< V * , bool >( &f_kv[ p ].second , false ) );
  }
 ADS_PRE( f_kv.size() < 0xFFFFFFFFULL );
 // the places full or counted, both known exactly [see unlink()]
 if( f_kv.size() + f_deleted >= f_max ) [[unlikely]]
  grow();
 // the pair first, so that an exception leaves the index as it was
 f_kv.emplace_back( std::piecewise_construct ,
		    std::forward_as_tuple( std::forward< KK >( k ) ) ,
		    std::forward_as_tuple( std::forward< Args >( a )... ) );
 const std::size_t g = home( x );
 const Mask e = empties( f_ctrl[ g ] );
 const std::size_t i =
   e ? g * 16 + std::size_t( std::countr_zero( e ) ) : claim( x );
 Group & c = f_ctrl[ i / 16 ];
 // a place counted, by no branch past the test of the bound, 0 with no
 // removals [see FlatHashTable.tpp]
 if( f_deleted ) [[unlikely]]
  f_deleted -= std::size_t( c.c[ OVER ] != 0 );
 c.c[ i % 16 ] = std::uint8_t( detail::flatTags[ x & 0xFF ] );
 c.at[ i % 16 ] = std::uint32_t( f_kv.size() - 1 );
 ADS_POST( locate( x , f_kv.back().first ) == f_kv.size() - 1 );
 ADS_INVARIANT( consistent( x ) );
 return( std::pair< V * , bool >( &f_kv.back().second , true ) );
 }

template< class K , class V , class H , class Eq , class A , class P >
bool DenseHashMap< K , V , H , Eq , A , P >::remove( const K & k )
{
 if constexpr( Subject::template listened< Erased > )
  if( f_sub.template live< Erased >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( removeCold( k ) );
   else
    return( removeTold( k ) );
   }
 // the first group alone, the common case [see FlatHashTable.tpp for why
 // the group is written whole]
 const std::uint64_t x = mix( k );
 const std::size_t g = home( x );
 Group & c = f_ctrl[ g ];
 const std::uint8_t over = c.c[ OVER ];
 for( Mask m = match( c , detail::flatTags[ x & 0xFF ] ) ; m ; m &= m - 1 ) {
  const std::size_t j = std::size_t( std::countr_zero( m ) ) , p = c.at[ j ];
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
template< class K , class V , class H , class Eq , class A , class P >
ADS_FLAT_INLINE bool
DenseHashMap< K , V , H , Eq , A , P >::removePlain( const K & k )
{
 // the first group alone, the common case [see FlatHashTable.tpp for why
 // the group is written whole]
 const std::uint64_t x = mix( k );
 const std::size_t g = home( x );
 Group & c = f_ctrl[ g ];
 const std::uint8_t over = c.c[ OVER ];
 for( Mask m = match( c , detail::flatTags[ x & 0xFF ] ) ; m ; m &= m - 1 ) {
  const std::size_t j = std::size_t( std::countr_zero( m ) ) , p = c.at[ j ];
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

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::unlink( Group & c ,
						     std::size_t j ,
						std::size_t p )
{
 // the place counted only in a group with some overflow, where it stays
 // full for the inspections: that group was full when it got the overflow,
 // so that each place counted was emptied there and the count is exact
 f_deleted += std::size_t( c.c[ OVER ] != 0 );
 vacate( c , j );
 const std::size_t last = f_kv.size() - 1;
 if( p != last ) {  // the last pair into p
  holder( mix( f_kv[ last ].first ) , last ) = std::uint32_t( p );
  f_kv[ p ] = std::move( f_kv[ last ] );
  }
 f_kv.pop_back();
 }

template< class K , class V , class H , class Eq , class A , class P >
std::size_t DenseHashMap< K , V , H , Eq , A , P >::size( void ) const
{
 return( f_kv.size() );
 }

template< class K , class V , class H , class Eq , class A , class P >
bool DenseHashMap< K , V , H , Eq , A , P >::empty( void ) const
{
 return( f_kv.empty() );
 }

template< class K , class V , class H , class Eq , class A , class P >
std::size_t DenseHashMap< K , V , H , Eq , A , P >::capacity( void ) const
{
 return( f_cap );
 }

template< class K , class V , class H , class Eq , class A , class P >
double DenseHashMap< K , V , H , Eq , A , P >::load( void ) const
{
 return( f_cap ? double( f_kv.size() ) / double( f_cap ) : 0.0 );
 }

template< class K , class V , class H , class Eq , class A , class P >
std::size_t DenseHashMap< K , V , H , Eq , A , P >::deleted( void ) const
{
 return( f_deleted );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::reserve( std::size_t n )
{
 f_kv.reserve( n );
 const std::size_t G = groups( n );
 if( G * SLOTS > f_cap )
  rebuild( G );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::clear( void )
{
 f_kv.clear();
 if( f_cap )
  std::memset( static_cast< void * >( f_ctrl ) , 0 ,
	       ( f_mask + 1 ) * sizeof( Group ) );
 f_deleted = 0;
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class K , class V , class H , class Eq , class A , class P >
template< class F >
void DenseHashMap< K , V , H , Eq , A , P >::forEach( F && f ) const
{
 for( const auto & kv : f_kv )
  f( kv.first , kv.second );
 }

template< class K , class V , class H , class Eq , class A , class P >
const std::pair< K , V > *
DenseHashMap< K , V , H , Eq , A , P >::data( void ) const
{
 return( f_kv.data() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE REBUILD -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::grow( void )
{
 const std::size_t G = f_cap ? f_mask + 1 : 0;
 rebuild( f_kv.size() + 1 > f_max / 2 ? ( G ? 2 * G : 2 ) : G );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::rebuild( std::size_t G )
{
 Groups g = newGroups( G , f_kv );
 Group * const c = first( g );
 std::memset( static_cast< void * >( c ) , 0 , G * sizeof( Group ) );
 const int shift = 64 - std::countr_zero( G );
 // the groups only filled, each in the order of its places, the count of
 // a group in a byte apart [see FlatHashTable.tpp]
 std::unique_ptr< std::uint8_t[] > n( new std::uint8_t[ G ]() );
 for( std::size_t p = 0 ; p < f_kv.size() ; ++p ) {
  const std::uint64_t x = mix( f_kv[ p ].first );
  std::size_t h = std::size_t( x >> shift );
  for( std::size_t r = 1 ; n[ h ] == SLOTS ; ++r ) {
   c[ h ].c[ OVER ] |= bit( x );  // x goes past h
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

template< class K , class V , class H , class Eq , class A , class P >
std::size_t DenseHashMap< K , V , H , Eq , A , P >::groups( std::size_t n )
{
 if( n == 0 )
  return( 0 );
 std::size_t G = 2;
 while( n > bound( G * SLOTS ) )
  G *= 2;
 return( G );
 }

template< class K , class V , class H , class Eq , class A , class P >
constexpr std::size_t
DenseHashMap< K , V , H , Eq , A , P >::bound( std::size_t m )
{
 return( m - m / 8 );
 }

template< class K , class V , class H , class Eq , class A , class P >
bool DenseHashMap< K , V , H , Eq , A ,
		   P >::consistent( std::uint64_t x ) const
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
 std::vector< char > held( n , 0 );  // each position exactly once
 std::size_t full = 0 , counted = 0;
 for( std::size_t g = 0 ; g <= f_mask ; ++g ) {
  if( f_ctrl[ g ].c[ OVER ] )  // the empty places counted
   counted += std::size_t( std::popcount( empties( f_ctrl[ g ] ) ) );
  for( Mask m = fulls( g ) ; m ; m &= m - 1 ) {
   const std::size_t j = std::size_t( std::countr_zero( m ) );
   if( ! reached( g , j ) || held[ f_ctrl[ g ].at[ j ] ] )
    return( false );
   held[ f_ctrl[ g ].at[ j ] ] = 1;
   ++full;
   }
  }
 return( ( full == n ) && ( counted == f_deleted ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- WHEN SOMEONE LISTENS ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A , class P >
template< class KK , class VV >
bool DenseHashMap< K , V , H , Eq , A , P >::putTold( KK && k , VV && v )
{
 if( V * s = find( k ) ) {  // an assignment, in place
  const Assigned e{ K( k ) , *s , V( v ) };
  f_sub.prepare( e );
  *s = std::forward< VV >( v );
  f_sub.emit( e , [ & ]() noexcept { *s = e.from; } );
  f_sub.audit( *this );
  return( false );
  }
 const Inserted e{ K( k ) , K( k ) , V( v ) };
 f_sub.prepare( e );
 put( std::forward< KK >( k ) , std::forward< VV >( v ) );
 f_sub.emit( e , [ & ]() noexcept { removePlain( e.h ); } );
 f_sub.audit( *this );
 return( true );
 }

template< class K , class V , class H , class Eq , class A , class P >
template< class KK , class... Args >
std::pair< V * , bool >
DenseHashMap< K , V , H , Eq , A , P >::emplaceTold( KK && k , Args &&... a )
{
 const std::pair< V * , bool > r =
   place< false >( std::forward< KK >( k ) , std::forward< Args >( a )... );
 if( ! r.second )
  return( r );
 const Inserted e{ f_kv.back().first , f_kv.back().first , *r.first };
 try {  // the value is built: a refusal undoes the insertion
  f_sub.prepare( e );
  }
 catch( ... ) {
  removePlain( e.h );
  throw;
  }
 f_sub.emit( e , [ & ]() noexcept { removePlain( e.h ); } );
 f_sub.audit( *this );
 return( r );
 }

template< class K , class V , class H , class Eq , class A , class P >
bool DenseHashMap< K , V , H , Eq , A , P >::removeTold( const K & k )
{
 const V * s = find( k );
 if( s == nullptr )
  return( false );
 const Erased e{ k , k , *s };
 f_sub.prepare( e );
 f_sub.emit( e );  // before the removal
 removePlain( k );
 f_sub.audit( *this );
 return( true );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::tellCleared( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() )
   f_sub.emit( mod::Cleared{} );
 }

template< class K , class V , class H , class Eq , class A , class P >
void DenseHashMap< K , V , H , Eq , A , P >::tellRebuilt( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Rebuilt > )
  if( f_sub.template live< mod::Rebuilt >() )
   f_sub.emit( mod::Rebuilt{} );
 }

/*--------------------------------------------------------------------------*/

ADS_ABI_END

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File DenseHashMap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
