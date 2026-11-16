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

#if ! ADS_POINTERTABLE_BOOST

/*--------------------------------------------------------------------------*/
/*------------------------- THE PLACES OF ADS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Eq , class A >
PointerTable< T , Eq , A >::PointerTable( std::size_t capacity , Eq eq ,
					const A & alloc )
    : f_S( std::bit_ceil( capacity < 16 ? std::size_t( 16 ) : capacity ) ,
	   Slot() , SA( alloc ) ) ,
      f_eq( std::move( eq ) )
{
 f_mask = f_S.size() - 1;
 f_bits = std::countr_zero( f_S.size() );
 }

template< class T , class Eq , class A >
PointerTable< T , Eq , A >::PointerTable( PointerTable && o ) noexcept
    : f_S( std::move( o.f_S ) ) , f_mask( o.f_mask ) , f_bits( o.f_bits ) ,
      f_size( o.f_size ) , f_eq( std::move( o.f_eq ) )
{
 o.f_S.clear();
 o.f_mask = 0;
 o.f_bits = 0;
 o.f_size = 0;
 }

template< class T , class Eq , class A >
PointerTable< T , Eq , A > &
PointerTable< T , Eq , A >::operator=( PointerTable && o ) noexcept
{
 if( this != &o ) {
  f_S = std::move( o.f_S );
  f_mask = o.f_mask;
  f_bits = o.f_bits;
  f_size = o.f_size;
  f_eq = std::move( o.f_eq );
  o.f_S.clear();
  o.f_mask = 0;
  o.f_bits = 0;
  o.f_size = 0;
  }
 return( *this );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::clear( void )
{
 std::fill( f_S.begin() , f_S.end() , Slot() );
 f_size = 0;
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::reserve( std::size_t n )
{
 const std::size_t m = std::max( slotsFor( n ) , f_S.size() );
 if( m == f_S.size() )
  return;
 std::vector< Slot , SA > old( std::move( f_S ) );
 reset( m );
 for( const Slot & s : old )  // the objects are not touched
  if( s.p ) {
   std::size_t i = place( s.hash );
   while( f_S[ i ].p )
    i = ( i + 1 ) & f_mask;
   f_S[ i ] = s;
   }
 ADS_POST( n * 10 <= f_S.size() * 7 );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::reset( std::size_t m )
{
 f_S.assign( m , Slot() );
 f_mask = m - 1;
 f_bits = std::countr_zero( m );
 }

template< class T , class Eq , class A >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A >::find_or_insert( const K & key ,
					    std::uint64_t hash ,
					  Make && make )
{
 if( f_S.empty() )  // moved from
  reset( 16 );
 std::size_t i = place( hash );
 for( ; f_S[ i ].p ; i = ( i + 1 ) & f_mask )
  if( ( f_S[ i ].hash == hash ) && f_eq( f_S[ i ].p , key ) )
   return( std::pair( f_S[ i ].p , false ) );
 T * p = make();
 ADS_POST( p != nullptr );
 f_S[ i ] = { hash , p };
 if( ++f_size * 10 > f_S.size() * 7 )
  grow();
 ADS_INVARIANT( ( f_size * 10 <= f_S.size() * 7 ) && reached( key , hash ) );
 return( std::pair( p , true ) );
 }

template< class T , class Eq , class A >
template< class K >
T * PointerTable< T , Eq , A >::find( const K & key ,
				      std::uint64_t hash ) const
{
 if( f_size == 0 )  // also moved from
  return( nullptr );
 for( std::size_t i = place( hash ) ; f_S[ i ].p ; i = ( i + 1 ) & f_mask )
  if( ( f_S[ i ].hash == hash ) && f_eq( f_S[ i ].p , key ) )
   return( f_S[ i ].p );
 return( nullptr );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::prefetch( std::uint64_t hash ) const
{
 if( ! f_S.empty() )
  ADS::detail::prefetchLine( &f_S[ place( hash ) ] , Locality::Low );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::size( void ) const
{
 return( f_size );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::capacity( void ) const
{
 return( f_S.size() );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::bytes( void ) const
{
 return( f_S.size() * sizeof( Slot ) );
 }

template< class T , class Eq , class A >
template< class F >
void PointerTable< T , Eq , A >::forEach( F && f ) const
{
 for( const Slot & s : f_S )
  if( s.p )
   f( s.p );
 }

template< class T , class Eq , class A >
auto PointerTable< T , Eq , A >::begin( void ) const -> const_iterator
{
 return( const_iterator( this , 0 ) );
 }

template< class T , class Eq , class A >
auto PointerTable< T , Eq , A >::end( void ) const -> const_iterator
{
 return( const_iterator( this , Places::size( *this ) ) );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::place( std::uint64_t hash ) const
{
 return( std::size_t( fibonacciPlace( hash , f_bits ) ) );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::grow( void )
{
 std::vector< Slot , SA > old( f_S.size() * 2 , Slot() , f_S.get_allocator() );
 old.swap( f_S );
 f_mask = f_S.size() - 1;
 ++f_bits;
 for( const Slot & s : old )  // the objects are not touched
  if( s.p ) {
   std::size_t i = place( s.hash );
   while( f_S[ i ].p )
    i = ( i + 1 ) & f_mask;
   f_S[ i ] = s;
   }
 }

template< class T , class Eq , class A >
template< class K >
bool PointerTable< T , Eq , A >::reached( const K & key ,
					std::uint64_t hash ) const
{
 if( f_S.empty() )
  return( false );
 for( std::size_t i = place( hash ) ; f_S[ i ].p ; i = ( i + 1 ) & f_mask )
  if( ( f_S[ i ].hash == hash ) && f_eq( f_S[ i ].p , key ) )
   return( true );
 return( false );
 }

#else

/*--------------------------------------------------------------------------*/
/*------------------------ THE PLACES OF BOOST -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Eq , class A >
template< class K , class Make >
PointerTable< T , Eq , A >::Entry::Entry( const Probe< K , Make > & q )
{
 this->hash = q.hash;
 this->p = ( *q.make )();
 ADS_POST( this->p != nullptr );
 }

template< class T , class Eq , class A >
PointerTable< T , Eq , A >::PointerTable( std::size_t capacity , Eq eq ,
					const A & alloc )
    : f_set( 0 , Hash() , Equal( std::move( eq ) ) , SA( alloc ) )
{
 // room for the objects that the places of ADS would hold, 7 / 10 of them
 f_set.reserve( ( capacity < 16 ? std::size_t( 16 ) : capacity ) * 7 / 10 );
 }

template< class T , class Eq , class A >
PointerTable< T , Eq , A >::PointerTable( PointerTable && o ) noexcept
    : f_set( std::move( o.f_set ) )
{
 o.f_set.clear();
 }

template< class T , class Eq , class A >
PointerTable< T , Eq , A > &
PointerTable< T , Eq , A >::operator=( PointerTable && o ) noexcept
{
 if( this != &o ) {
  f_set = std::move( o.f_set );
  o.f_set.clear();
  }
 return( *this );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::clear( void )
{
 f_set.clear();
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::reserve( std::size_t n )
{
 if( n > f_set.max_load() )  // never a shrink
  f_set.reserve( n );
 ADS_POST( n <= f_set.max_load() );
 }

template< class T , class Eq , class A >
template< class K , class Make >
std::pair< T * , bool >
PointerTable< T , Eq , A >::find_or_insert( const K & key ,
					    std::uint64_t hash ,
					  Make && make )
{
 using M = std::remove_reference_t< Make >;
 const Probe< K , M > q{ key , hash , &make };
 const auto [ it , fresh ] = f_set.insert( q );
 ADS_INVARIANT( find( key , hash ) == it->p );
 return( std::pair( it->p , fresh ) );
 }

template< class T , class Eq , class A >
template< class K >
T * PointerTable< T , Eq , A >::find( const K & key ,
				      std::uint64_t hash ) const
{
 const auto it = f_set.find( Probe< K , void >{ key , hash , nullptr } );
 return( it == f_set.end() ? nullptr : it->p );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::prefetch( std::uint64_t hash ) const
{
 ( void )hash;  // Boost has no way to ask for the group of a hash
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::size( void ) const
{
 return( f_set.size() );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::capacity( void ) const
{
 return( f_set.bucket_count() );
 }

template< class T , class Eq , class A >
std::size_t PointerTable< T , Eq , A >::bytes( void ) const
{
 // the places, one more than the capacity for the sentinel of Boost, and
 // 16 bytes of control per group of 15 of them
 const std::size_t c = f_set.bucket_count();
 return( c ? ( c + 1 ) * sizeof( Entry ) + ( c + 1 ) / 15 * 16 : 0 );
 }

template< class T , class Eq , class A >
template< class F >
void PointerTable< T , Eq , A >::forEach( F && f ) const
{
 for( const Entry & e : f_set )
  f( e.p );
 }

template< class T , class Eq , class A >
auto PointerTable< T , Eq , A >::begin( void ) const -> const_iterator
{
 return( const_iterator( f_set.begin() ) );
 }

template< class T , class Eq , class A >
auto PointerTable< T , Eq , A >::end( void ) const -> const_iterator
{
 return( const_iterator( f_set.end() ) );
 }

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PointerTable.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
