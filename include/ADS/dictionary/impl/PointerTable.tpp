/*--------------------------------------------------------------------------*/
/*------------------------- File PointerTable.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PointerTable [see PointerTable.h], with their
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

template< class T , class Eq , class A >
PointerTable< T , Eq , A >::PointerTable( std::size_t capacity , Eq eq ,
                                          const A & alloc )
 : f_S( std::bit_ceil( capacity < 16 ? std::size_t( 16 ) : capacity ) ,
        Slot() , SA( alloc ) ) , f_eq( std::move( eq ) )
{
 f_mask = f_S.size() - 1;
 f_bits = std::countr_zero( f_S.size() );
 }

template< class T , class Eq , class A >
template< class K , class Make >
std::pair< T * , bool > PointerTable< T , Eq , A >::find_or_insert(
                         const K & key , std::uint64_t hash , Make && make )
{
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
 for( std::size_t i = place( hash ) ; f_S[ i ].p ; i = ( i + 1 ) & f_mask )
  if( ( f_S[ i ].hash == hash ) && f_eq( f_S[ i ].p , key ) )
   return( f_S[ i ].p );
 return( nullptr );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::prefetch( std::uint64_t hash ) const
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( &f_S[ place( hash ) ] , 0 , 1 );
#else
 (void) hash;
#endif
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
std::size_t PointerTable< T , Eq , A >::place( std::uint64_t hash ) const
{
 return( std::size_t( fibonacciPlace( hash , f_bits ) ) );
 }

template< class T , class Eq , class A >
void PointerTable< T , Eq , A >::grow( void )
{
 std::vector< Slot , SA > old( f_S.size() * 2 , Slot() ,
                               f_S.get_allocator() );
 old.swap( f_S );
 f_mask = f_S.size() - 1;
 ++f_bits;
 for( const Slot & s : old )                // the objects are not touched
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
 for( std::size_t i = place( hash ) ; f_S[ i ].p ; i = ( i + 1 ) & f_mask )
  if( ( f_S[ i ].hash == hash ) && f_eq( f_S[ i ].p , key ) )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PointerTable.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
