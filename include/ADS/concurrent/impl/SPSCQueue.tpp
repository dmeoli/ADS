/*--------------------------------------------------------------------------*/
/*--------------------------- File SPSCQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SPSCQueue [see SPSCQueue.h], with their contracts.
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

template< class T >
SPSCQueue< T >::SPSCQueue( std::size_t capacity )
 : f_A( nullptr ) , f_mask( capacity - 1 )
{
 ADS_PRE( ( capacity >= 2 ) && std::has_single_bit( capacity ) );
 f_A = std::allocator< T >().allocate( capacity + 2 * pad );
 }

template< class T >
SPSCQueue< T >::~SPSCQueue()
{
 for( std::size_t h = f_head.load() , t = f_tail.load() ; h != t ; ++h )
  at( h )->~T();
 std::allocator< T >().deallocate( f_A , capacity() + 2 * pad );
 }

template< class T >
template< class ... Args >
bool SPSCQueue< T >::emplace( Args && ... args )
{
 const std::size_t t = f_tail.load( std::memory_order_relaxed );
 if( t - f_headSeen == capacity() ) {      // full, as far as we know
  f_headSeen = f_head.load( std::memory_order_acquire );
  if( t - f_headSeen == capacity() )
   return( false );
  }
 ::new( static_cast< void * >( at( t ) ) )
  T( std::forward< Args >( args )... );
 f_tail.store( t + 1 , std::memory_order_release );
 return( true );
 }

template< class T >
bool SPSCQueue< T >::tryPop( T & x )
{
 const std::size_t h = f_head.load( std::memory_order_relaxed );
 if( h == f_tailSeen ) {                    // empty, as far as we know
  f_tailSeen = f_tail.load( std::memory_order_acquire );
  if( h == f_tailSeen )
   return( false );
  }
 T * p = at( h );
 x = std::move( *p );
 p->~T();
 f_head.store( h + 1 , std::memory_order_release );
 return( true );
 }

template< class T >
std::optional< T > SPSCQueue< T >::pop( void )
{
 const std::size_t h = f_head.load( std::memory_order_relaxed );
 if( h == f_tailSeen ) {                    // empty, as far as we know
  f_tailSeen = f_tail.load( std::memory_order_acquire );
  if( h == f_tailSeen )
   return( std::nullopt );
  }
 T * p = at( h );
 std::optional< T > v( std::move( *p ) );
 p->~T();
 f_head.store( h + 1 , std::memory_order_release );
 return( v );
 }

template< class T >
std::size_t SPSCQueue< T >::size( void ) const
{
 return( f_tail.load( std::memory_order_acquire ) -
         f_head.load( std::memory_order_acquire ) );
 }


/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SPSCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
