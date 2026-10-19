/*--------------------------------------------------------------------------*/
/*-------------------------- File ObjectPool.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ObjectPool [see ObjectPool.h], with their contracts.
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

template< class T , class A >
ObjectPool< T , A >::ObjectPool( std::size_t n , const A & alloc )
 : f_n( n ) , f_alloc( alloc )
{
 ADS_PRE( n >= 1 );
 }

template< class T , class A >
ObjectPool< T , A >::ObjectPool( ObjectPool && other ) noexcept
 : f_free( std::exchange( other.f_free , nullptr ) ) ,
   f_next( std::exchange( other.f_next , nullptr ) ) ,
   f_end( std::exchange( other.f_end , nullptr ) ) ,
   f_n( other.f_n ) ,
   f_size( std::exchange( other.f_size , 0 ) ) ,
   f_blocks( std::exchange( other.f_blocks , {} ) ) ,
   f_alloc( other.f_alloc )
{
 }

template< class T , class A >
ObjectPool< T , A > &
ObjectPool< T , A >::operator=( ObjectPool && other ) noexcept
{
 ObjectPool t( std::move( other ) );
 swap( t );
 return( *this );
 }

template< class T , class A >
ObjectPool< T , A >::~ObjectPool()
{
 for( auto [ b , k ] : f_blocks )
  std::allocator_traits< SlotAlloc >::deallocate( f_alloc , b , k );
 }

template< class T , class A >
void ObjectPool< T , A >::swap( ObjectPool & other ) noexcept
{
 std::swap( f_free , other.f_free );
 std::swap( f_next , other.f_next );
 std::swap( f_end , other.f_end );
 std::swap( f_n , other.f_n );
 std::swap( f_size , other.f_size );
 f_blocks.swap( other.f_blocks );
 std::swap( f_alloc , other.f_alloc );
 }

template< class T , class A >
template< class... Args >
inline T * ObjectPool< T , A >::make( Args &&... args )
{
 Slot * s;
 if( f_free )                               // the last one freed
  s = std::exchange( f_free , f_free->next );
 else {
  if( f_next == f_end ) {                   // twice the last block
   const std::size_t k = f_blocks.empty() ? std::size_t( 16 ) <= f_n ?
                         std::size_t( 16 ) : f_n :
                         f_blocks.back().second * 2 <= f_n ?
                         f_blocks.back().second * 2 : f_n;
   f_blocks.reserve( f_blocks.size() + 1 );
   f_next = std::allocator_traits< SlotAlloc >::allocate( f_alloc , k );
   f_end = f_next + k;
   f_blocks.emplace_back( f_next , k );
   }
  s = f_next++;
  }
 T * x;
 try {
  if constexpr( std::is_constructible_v< T , Args... > )
   x = ::new( static_cast< void * >( s->object ) )
        T( std::forward< Args >( args )... );
  else                                      // an aggregate
   x = ::new( static_cast< void * >( s->object ) )
        T{ std::forward< Args >( args )... };
  }
 catch( ... ) {
  s->next = f_free;                         // the slot stays free
  f_free = s;
  throw;
  }
 ++f_size;
 return( x );
 }

template< class T , class A >
inline void ObjectPool< T , A >::free( T * x )
{
 ADS_PRE( ( x != nullptr ) && ( f_size > 0 ) );
 std::destroy_at( x );
 Slot * s = reinterpret_cast< Slot * >( x );
 s->next = f_free;
 f_free = s;
 --f_size;
 }

template< class T , class A >
std::size_t ObjectPool< T , A >::size( void ) const
{
 return( f_size );
 }

template< class T , class A >
std::size_t ObjectPool< T , A >::bytes( void ) const
{
 std::size_t b = 0;
 for( const auto & [ s , k ] : f_blocks )
  b += k * sizeof( Slot );
 return( b );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ObjectPool.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
