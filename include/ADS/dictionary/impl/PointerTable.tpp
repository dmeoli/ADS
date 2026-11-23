/*--------------------------------------------------------------------------*/
/*------------------------- File PointerTable.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PointerTable [see PointerTable.h], with their
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

/*--------------------------------------------------------------------------*/
/*------------------------- THE PLACES OF ADS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Eq , class A , class P >
PointerTable< T , Eq , A , P >::PointerTable( PointerTable && o ) noexcept
    : f_S( std::move( o.f_S ) ) , f_mask( o.f_mask ) , f_bits( o.f_bits ) ,
      f_size( o.f_size ) , f_eq( std::move( o.f_eq ) ) , f_L( o.f_L )
{
 o.f_S.clear();
 o.f_mask = 0;
 o.f_bits = 0;
 o.f_size = 0;
 if constexpr( ! Subject::none )
  if( f_size > 0 )
   o.tellCleared();
 }


template< class T , class Eq , class A , class P >
PointerTable< T , Eq , A , P > &
PointerTable< T , Eq , A , P >::operator=( PointerTable && o ) noexcept
{
 if( this != &o ) {
  f_S = std::move( o.f_S );
  f_mask = o.f_mask;
  f_bits = o.f_bits;
  f_size = o.f_size;
  f_eq = std::move( o.f_eq );
  f_L = o.f_L;
  o.f_S.clear();
  o.f_mask = 0;
  o.f_bits = 0;
  o.f_size = 0;
  if constexpr( ! Subject::none ) {
   tellRebuilt();
   if( f_size > 0 )
    o.tellCleared();
   }
  }
 return( *this );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::clear( void )
{
 std::fill( f_S.begin() , f_S.end() , Slot() );
 f_size = 0;
 tellClearedAudited();
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::reserve( std::size_t n )
{
 const std::size_t m = std::max( slotsFor( n ) , f_S.size() );
 if( m == f_S.size() )
  return;
 std::vector< Slot , SA > old( std::move( f_S ) );
 reset( m );
 move( old );
 ADS_POST( n * 10 <= f_S.size() * 7 );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::reset( std::size_t m )
{
 // new places, not a fill of the old ones, so that those of an allocator
 // of 0s are not written [see NoInitAllocator]
 std::vector< Slot , SA >( m , f_S.get_allocator() ).swap( f_S );
 f_mask = m - 1;
 f_bits = std::countr_zero( m );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A ,
		   P >::move( const std::vector< Slot , SA > & old )
{
 for( const Slot & s : old )  // the objects are not touched
  if( full( s ) ) {
   std::size_t i = place( bitsOf( s ) );
   while( full( f_S[ i ] ) )
    i = ( i + 1 ) & f_mask;
   f_S[ i ] = s;
   }
 }

template< class T , class Eq , class A , class P >
auto PointerTable< T , Eq , A , P >::slot( std::uint64_t hash , T * p ) const
  -> Slot
{
 if constexpr( kIndex ) {
  const std::size_t k = f_L.store->index( p );
  if( k >= 0xFFFFFFFFU )
   throw( std::length_error( "PointerTable::find_or_insert: an index of "
			     "2^32 - 1 or more" ) );
  return( ( bits( hash ) << 32 ) | std::uint64_t( k + 1 ) );
  }
 else
  return( Slot{ hash , p } );
 }

template< class T , class Eq , class A , class P >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A , P >::find_or_insert( const K & key ,
						std::uint64_t hash ,
					     Make && make )
{
 if constexpr( Subject::template listened< Inserted > )
  if( f_sub.template live< Inserted >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( insertCold( key , hash , make ) );
   else
    return( insertTold( key , hash , make ) );
   }
 if( f_S.empty() )  // moved from
  reset( 16 );
 const std::uint64_t b = bits( hash );
 std::size_t i = place( b );
 for( ; full( f_S[ i ] ) ; i = ( i + 1 ) & f_mask )
  if( ( bitsOf( f_S[ i ] ) == b ) && f_eq( object( f_S[ i ] ) , key ) )
   return( std::pair( object( f_S[ i ] ) , false ) );
 T * p = make();
 ADS_POST( p != nullptr );
 f_S[ i ] = slot( hash , p );
 if( ++f_size * 10 > f_S.size() * 7 )
  grow();
 ADS_INVARIANT( ( f_size * 10 <= f_S.size() * 7 ) && reached( key , hash ) );
 return( std::pair( p , true ) );
 }

template< class T , class Eq , class A , class P >
template< class K , class Make >
std::pair< T * , bool > PointerTable< T , Eq , A , P >::findOrInsertPlain(
  const K & key , std::uint64_t hash , Make && make )
{
 if( f_S.empty() )  // moved from
  reset( 16 );
 const std::uint64_t b = bits( hash );
 std::size_t i = place( b );
 for( ; full( f_S[ i ] ) ; i = ( i + 1 ) & f_mask )
  if( ( bitsOf( f_S[ i ] ) == b ) && f_eq( object( f_S[ i ] ) , key ) )
   return( std::pair( object( f_S[ i ] ) , false ) );
 T * p = make();
 ADS_POST( p != nullptr );
 f_S[ i ] = slot( hash , p );
 if( ++f_size * 10 > f_S.size() * 7 )
  grow();
 ADS_INVARIANT( ( f_size * 10 <= f_S.size() * 7 ) && reached( key , hash ) );
 return( std::pair( p , true ) );
 }

template< class T , class Eq , class A , class P >
template< class K >
T * PointerTable< T , Eq , A , P >::find( const K & key ,
				       std::uint64_t hash ) const
{
 if( f_size == 0 )  // also moved from
  return( nullptr );
 const std::uint64_t b = bits( hash );
 for( std::size_t i = place( b ) ; full( f_S[ i ] ) ; i = ( i + 1 ) & f_mask )
  if( ( bitsOf( f_S[ i ] ) == b ) && f_eq( object( f_S[ i ] ) , key ) )
   return( object( f_S[ i ] ) );
 return( nullptr );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::prefetch( std::uint64_t hash ) const
{
 if( ! f_S.empty() )
  ADS::detail::prefetchLine( &f_S[ place( bits( hash ) ) ] , Locality::Low );
 }

template< class T , class Eq , class A , class P >
std::size_t PointerTable< T , Eq , A , P >::size( void ) const
{
 return( f_size );
 }

template< class T , class Eq , class A , class P >
std::size_t PointerTable< T , Eq , A , P >::capacity( void ) const
{
 return( f_S.size() );
 }

template< class T , class Eq , class A , class P >
std::size_t PointerTable< T , Eq , A , P >::bytes( void ) const
{
 return( f_S.size() * sizeof( Slot ) );
 }

template< class T , class Eq , class A , class P >
template< class F >
void PointerTable< T , Eq , A , P >::forEach( F && f ) const
{
 for( const Slot & s : f_S )
  if( full( s ) )
   f( object( s ) );
 }

template< class T , class Eq , class A , class P >
auto PointerTable< T , Eq , A , P >::begin( void ) const -> const_iterator
{
 return( const_iterator( this , 0 ) );
 }

template< class T , class Eq , class A , class P >
auto PointerTable< T , Eq , A , P >::end( void ) const -> const_iterator
{
 return( const_iterator( this , Places::size( *this ) ) );
 }

template< class T , class Eq , class A , class P >
std::size_t PointerTable< T , Eq , A , P >::place( std::uint64_t b ) const
{
 return( std::size_t( fibonacciPlace( b , f_bits ) ) );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::grow( void )
{
 std::vector< Slot , SA > old( std::move( f_S ) );
 reset( 2 * old.size() );
 move( old );
 }

template< class T , class Eq , class A , class P >
template< class K >
bool PointerTable< T , Eq , A , P >::reached( const K & key ,
					   std::uint64_t hash ) const
{
 if( f_S.empty() )
  return( false );
 const std::uint64_t b = bits( hash );
 for( std::size_t i = place( b ) ; full( f_S[ i ] ) ; i = ( i + 1 ) & f_mask )
  if( ( bitsOf( f_S[ i ] ) == b ) && f_eq( object( f_S[ i ] ) , key ) )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE EVENTS --------------------------------*/
/*--------------------------------------------------------------------------*/


template< class T , class Eq , class A , class P >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A , P >::insertTold( const K & key ,
					    std::uint64_t hash ,
					 Make & make )
{
 const std::pair< T * , bool > r = findOrInsertPlain( key , hash , make );
 if( r.second ) {
  f_sub.emit( Inserted{ r.first , hash } );
  f_sub.audit( *this );
  }
 return( r );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::tellClearedAudited( void )
{
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::tellCleared( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() )
   f_sub.emit( mod::Cleared{} );
 }

template< class T , class Eq , class A , class P >
void PointerTable< T , Eq , A , P >::tellRebuilt( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Rebuilt > )
  if( f_sub.template live< mod::Rebuilt >() )
   f_sub.emit( mod::Rebuilt{} );
 }

#if ADS_POINTERTABLE_BOOST

/*--------------------------------------------------------------------------*/
/*------------------------ THE PLACES OF BOOST -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class K , class Make >
PointerTable< T , Eq , A , P >::Entry::Entry( const Probe< K , Make > & q )
{
 this->hash = q.hash;
 this->p = ( *q.make )();
 ADS_POST( this->p != nullptr );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class... O >
PointerTable< T , Eq , A , P >::PointerTable( std::size_t capacity , Eq eq ,
					   const A & alloc , O &... o )
    : f_set( 0 , Hash() , Equal( std::move( eq ) ) , SA( alloc ) ) ,
      f_sub( o... )
{
 // room for the objects that the places of ADS would hold, 7 / 10 of them
 f_set.reserve( ( capacity < 16 ? std::size_t( 16 ) : capacity ) * 7 / 10 );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
PointerTable< T , Eq , A , P >::PointerTable( PointerTable && o ) noexcept
    : f_set( std::move( o.f_set ) )
{
 o.f_set.clear();
 if constexpr( ! Subject::none )
  if( ! f_set.empty() )
   o.tellCleared();
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
PointerTable< T , Eq , A , P > &
PointerTable< T , Eq , A , P >::operator=( PointerTable && o ) noexcept
{
 if( this != &o ) {
  f_set = std::move( o.f_set );
  o.f_set.clear();
  if constexpr( ! Subject::none ) {
   tellRebuilt();
   if( ! f_set.empty() )
    o.tellCleared();
   }
  }
 return( *this );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::clear( void )
{
 f_set.clear();
 tellClearedAudited();
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::reserve( std::size_t n )
{
 if( n > f_set.max_load() )  // never a shrink
  f_set.reserve( n );
 ADS_POST( n <= f_set.max_load() );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A , P >::find_or_insert( const K & key ,
						std::uint64_t hash ,
					     Make && make )
{
 if constexpr( Subject::template listened< Inserted > )
  if( f_sub.template live< Inserted >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( insertCold( key , hash , make ) );
   else
    return( insertTold( key , hash , make ) );
   }
 using M = std::remove_reference_t< Make >;
 const Probe< K , M > q{ key , hash , &make };
 const auto [ it , fresh ] = f_set.insert( q );
 ADS_INVARIANT( find( key , hash ) == it->p );
 return( std::pair( it->p , fresh ) );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class K , class Make >
std::pair< T * , bool > PointerTable< T , Eq , A , P >::findOrInsertPlain(
  const K & key , std::uint64_t hash , Make && make )
{
 using M = std::remove_reference_t< Make >;
 const Probe< K , M > q{ key , hash , &make };
 const auto [ it , fresh ] = f_set.insert( q );
 ADS_INVARIANT( find( key , hash ) == it->p );
 return( std::pair( it->p , fresh ) );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class K >
T * PointerTable< T , Eq , A , P >::find( const K & key ,
				       std::uint64_t hash ) const
{
 const auto it = f_set.find( Probe< K , void >{ key , hash , nullptr } );
 return( it == f_set.end() ? nullptr : it->p );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::prefetch( std::uint64_t hash ) const
{
 ( void )hash;  // Boost has no way to ask for the group of a hash
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
std::size_t PointerTable< T , Eq , A , P >::size( void ) const
{
 return( f_set.size() );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
std::size_t PointerTable< T , Eq , A , P >::capacity( void ) const
{
 return( f_set.bucket_count() );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
std::size_t PointerTable< T , Eq , A , P >::bytes( void ) const
{
 // the places, one more than the capacity for the sentinel of Boost, and
 // 16 bytes of control per group of 15 of them
 const std::size_t c = f_set.bucket_count();
 return( c ? ( c + 1 ) * sizeof( Entry ) + ( c + 1 ) / 15 * 16 : 0 );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class F >
void PointerTable< T , Eq , A , P >::forEach( F && f ) const
{
 for( const Entry & e : f_set )
  f( e.p );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
auto PointerTable< T , Eq , A , P >::begin( void ) const -> const_iterator
{
 return( const_iterator( f_set.begin() ) );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
auto PointerTable< T , Eq , A , P >::end( void ) const -> const_iterator
{
 return( const_iterator( f_set.end() ) );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A , P >::insertTold( const K & key ,
					    std::uint64_t hash ,
					 Make & make )
{
 const std::pair< T * , bool > r = findOrInsertPlain( key , hash , make );
 if( r.second ) {
  f_sub.emit( Inserted{ r.first , hash } );
  f_sub.audit( *this );
  }
 return( r );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::tellClearedAudited( void )
{
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::tellCleared( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() )
   f_sub.emit( mod::Cleared{} );
 }

template< class T , class Eq , class A , class P >
 requires detail::BoostPointerPlaces< A , P >
void PointerTable< T , Eq , A , P >::tellRebuilt( void ) noexcept
{
 if constexpr( Subject::template listened< mod::Rebuilt > )
  if( f_sub.template live< mod::Rebuilt >() )
   f_sub.emit( mod::Rebuilt{} );
 }

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PointerTable.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
