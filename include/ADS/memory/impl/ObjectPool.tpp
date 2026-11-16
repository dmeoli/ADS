/*--------------------------------------------------------------------------*/
/*-------------------------- File ObjectPool.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ObjectPool [see ObjectPool.h], with their contracts.
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
      f_last( std::exchange( other.f_last , nullptr ) ) ,
      f_reuse( std::exchange( other.f_reuse , nullptr ) ) , f_n( other.f_n ) ,
      f_size( std::exchange( other.f_size , 0 ) ) , f_alloc( other.f_alloc )
{ }

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
 release();
 }

template< class T , class A >
void ObjectPool< T , A >::release( void ) noexcept
{
 while( Block * b = f_last ) {
  f_last = b->prev;
  ADS_UNPOISON( b , b->k * sizeof( Slot ) );  // dropped by clear()
  std::allocator_traits< SlotAlloc >::deallocate(
    f_alloc , reinterpret_cast< Slot * >( b ) , b->k );
  }
 }

template< class T , class A >
void ObjectPool< T , A >::swap( ObjectPool & other ) noexcept
{
 if constexpr( ! std::is_move_assignable_v< SlotAlloc > ) {
  // an allocator that is not assigned, as std::pmr::polymorphic_allocator:
  // the two pools built again, each with the allocator of its blocks
  ObjectPool t( std::move( other ) );
  std::destroy_at( &other );
  std::construct_at( &other , std::move( *this ) );
  std::destroy_at( this );
  std::construct_at( this , std::move( t ) );
  return;
  }
 std::swap( f_free , other.f_free );
 std::swap( f_next , other.f_next );
 std::swap( f_end , other.f_end );
 std::swap( f_last , other.f_last );
 std::swap( f_reuse , other.f_reuse );
 std::swap( f_n , other.f_n );
 std::swap( f_size , other.f_size );
 if constexpr( std::is_move_assignable_v< SlotAlloc > )
  std::swap( f_alloc , other.f_alloc );
 }

template< class T , class A >
void ObjectPool< T , A >::adopt( ObjectPool & other ) noexcept
{
 if( ( &other == this ) || ! other.f_last )
  return;
 const std::size_t n = std::max( f_n , other.f_n ) , m = other.f_n;
 if( ! f_last ) {  // nothing here: all of other
  swap( other );
  other.f_n = m;  // each keeps its own limit
  }
 else if( f_reuse ) {
  // blocks kept by clear() lie under the one in use: the chain of other
  // goes over the last block, out of their way
  Block * b = other.f_last;  // the first block of other
  while( b->prev )
   b = b->prev;
  b->prev = f_last;
  f_last = other.f_last;
  f_size += other.f_size;
  }
 else {
  Block * b = f_last;  // the first block of this pool
  while( b->prev )
   b = b->prev;
  b->prev = other.f_last;
  f_size += other.f_size;
  }
 f_n = n;  // the larger limit, in both
 other.f_free = other.f_next = other.f_end = nullptr;
 other.f_last = other.f_reuse = nullptr;
 other.f_size = 0;
 }

template< class T , class A >
inline T * ObjectPool< T , A >::allocate( void )
{
 return( reinterpret_cast< T * >( slot()->object ) );
 }

template< class T , class A >
inline typename ObjectPool< T , A >::Slot * ObjectPool< T , A >::slot( void )
{
 Slot * s;
 if( f_free )  // the last one freed
  s = std::exchange( f_free , f_free->next );
 else {
  if( f_next == f_end )
   refill();
  s = f_next++;
  ADS_UNPOISON( s , sizeof( Slot ) );
  }
 ++f_size;
 return( s );
 }

template< class T , class A >
void ObjectPool< T , A >::refill( void )
{
 if( f_reuse ) {  // a block kept by clear()
  Block * b = std::exchange( f_reuse , f_reuse->prev );
  f_next = reinterpret_cast< Slot * >( b ) + H;
  f_end = reinterpret_cast< Slot * >( b ) + b->k;
  return;
  }
 // twice the last block, up to n slots
 const std::size_t s = ! f_last                       ? 1
		       : 2 * ( f_last->k - H ) <= f_n ? 2 * ( f_last->k - H )
						      : f_n;
 Slot * b = std::allocator_traits< SlotAlloc >::allocate( f_alloc , s + H );
 f_last = ::new( static_cast< void * >( b ) ) Block{ f_last , s + H };
 f_next = b + H;
 f_end = b + H + s;
 }

template< class T , class A >
template< class... Args >
inline T * ObjectPool< T , A >::make( Args &&... args )
{
 Slot * s = slot();
 T * x;
 try {
  if constexpr( std::is_constructible_v< T , Args... > )
   x = ::new( static_cast< void * >( s->object ) )
     T( std::forward< Args >( args )... );
  else  // an aggregate
   x = ::new( static_cast< void * >( s->object ) )
     T{ std::forward< Args >( args )... };
  }
 catch( ... ) {
  s->next = f_free;  // the slot stays free
  f_free = s;
  --f_size;
  throw;
  }
 return( x );
 }

template< class T , class A >
inline void ObjectPool< T , A >::free( T * x )
{
 ADS_PRE( ( x != nullptr ) && ( f_size > 0 ) );
 ADS_AUDIT( fresh( x ) );
 std::destroy_at( x );
 Slot * s = reinterpret_cast< Slot * >( x );
 s->next = f_free;
 f_free = s;
 --f_size;
 }

template< class T , class A >
inline void ObjectPool< T , A >::deallocate( T * x )
{
 ADS_PRE( ( x != nullptr ) && ( f_size > 0 ) );
 ADS_AUDIT( fresh( x ) );
 Slot * s = reinterpret_cast< Slot * >( x );
 s->next = f_free;
 f_free = s;
 --f_size;
 }

template< class T , class A >
void ObjectPool< T , A >::clear( void ) noexcept
{
 f_free = nullptr;
 f_size = 0;
 if( ! f_last )
  return;
 if constexpr( ADS_ASAN )  // the slots of the epochs
  for( Block * b = f_last ; b ; b = b->prev )
   ADS_POISON( reinterpret_cast< Slot * >( b ) + H ,
	       ( b->k - H ) * sizeof( Slot ) );
 f_next = reinterpret_cast< Slot * >( f_last ) + H;
 f_end = reinterpret_cast< Slot * >( f_last ) + f_last->k;
 f_reuse = f_last->prev;
 }

template< class T , class A >
bool ObjectPool< T , A >::fresh( const T * p ) const
{
 const std::less< const Slot * > lt;
 const Slot * x = reinterpret_cast< const Slot * >( p );
 // the blocks given in the epoch are those over the first one kept: all
 // their slots given, but those of [ next , end ) of the block in use
 for( const Block * b = f_last ; b && ( b != f_reuse ) ; b = b->prev ) {
  const Slot * s = reinterpret_cast< const Slot * >( b );
  if( ! lt( x , s + H ) && lt( x , s + b->k ) )
   return( ( s + b->k != f_end ) || lt( x , f_next ) );
  }
 return( false );
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
 for( const Block * h = f_last ; h ; h = h->prev )
  b += h->k * sizeof( Slot );
 return( b );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ObjectPool.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
