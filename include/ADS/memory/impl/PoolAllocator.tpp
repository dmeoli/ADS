/*--------------------------------------------------------------------------*/
/*------------------------- File PoolAllocator.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PoolResource and PoolAllocator [see PoolAllocator.h],
 * with their contracts.
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

namespace detail {

inline void * poolNew( std::size_t b , std::size_t a )
{
 if( a > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  return( ::operator new( b , std::align_val_t( a ) ) );
 return( ::operator new( b ) );
 }

inline void poolDelete( void * p , std::size_t , std::size_t a ) noexcept
{
 if( a > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  ::operator delete( p , std::align_val_t( a ) );
 else
  ::operator delete( p );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- PoolResource --------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::size_t M >
PoolResource< M >::PoolResource( void )
 : f_heap( std::make_unique< detail::PageHeap< CLASSES > >( this ) )
{
 }

template< std::size_t M >
template< std::size_t b , std::size_t a >
inline void * PoolResource< M >::allocate( void )
{
 if constexpr( pooled( b , a ) )
  return( take( classOf( b , a ) ) );
 else
  return( detail::poolNew( b , a ) );
 }

template< std::size_t M >
template< std::size_t b , std::size_t a >
inline void PoolResource< M >::deallocate( void * p )
{
 if constexpr( pooled( b , a ) ) {
  auto * const g = detail::PageHeap< CLASSES >::pageOf( p );
  ADS_PRE( ( p != nullptr ) && ( g->owner == this ) &&
           ( g->c == classOf( b , a ) ) );
  f_heap->give( g , p );
  }
 else
  detail::poolDelete( p , b , a );
 }

template< std::size_t M >
void * PoolResource< M >::allocate( std::size_t b , std::size_t a )
{
 if( pooled( b , a ) )
  return( take( classOf( b , a ) ) );
 return( detail::poolNew( b , a ) );
 }

template< std::size_t M >
void PoolResource< M >::deallocate( void * p , std::size_t b ,
                                    std::size_t a )
{
 ADS_PRE( p != nullptr );
 if( pooled( b , a ) ) {
  auto * const g = detail::PageHeap< CLASSES >::pageOf( p );
  ADS_PRE( ( g->owner == this ) && ( g->c == classOf( b , a ) ) );
  f_heap->give( g , p );
  }
 else
  detail::poolDelete( p , b , a );
 }

template< std::size_t M >
std::size_t PoolResource< M >::size( void ) const
{
 return( f_heap->live() );
 }

template< std::size_t M >
std::size_t PoolResource< M >::bytes( void ) const
{
 return( f_heap->bytes() );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- PoolAllocator -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class R >
inline T * PoolAllocator< T , R >::allocate( std::size_t n )
{
 if( n == 1 )
  return( static_cast< T * >(
           f_r->template allocate< sizeof( T ) , alignof( T ) >() ) );
 if( n > std::numeric_limits< std::size_t >::max() / sizeof( T ) )
  throw std::bad_array_new_length();
 return( static_cast< T * >( f_r->allocate( n * sizeof( T ) ,
                                            alignof( T ) ) ) );
 }

template< class T , class R >
inline void PoolAllocator< T , R >::deallocate( T * p , std::size_t n )
{
 if( n == 1 )
  f_r->template deallocate< sizeof( T ) , alignof( T ) >( p );
 else
  f_r->deallocate( p , n * sizeof( T ) , alignof( T ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PoolAllocator.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
