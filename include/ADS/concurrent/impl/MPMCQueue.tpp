/*--------------------------------------------------------------------------*/
/*--------------------------- File MPMCQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MPMCQueue [see MPMCQueue.h], with their contracts.
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

template< class T >
MPMCQueue< T >::MPMCQueue( std::size_t capacity )
 : f_cell( nullptr ) , f_mask( capacity - 1 )
{
 ADS_PRE( ( capacity >= 2 ) && std::has_single_bit( capacity ) );
 f_cell = std::allocator< Cell >().allocate( capacity );
 for( std::size_t i = 0 ; i < capacity ; ++i )
  ::new( static_cast< void * >( f_cell + i ) ) Cell{ { i } , {} };
 }

template< class T >
MPMCQueue< T >::~MPMCQueue()
{
 for( std::size_t h = f_head.load() , t = f_tail.load() ; h != t ; ++h )
  f_cell[ h & f_mask ].item()->~T();
 for( std::size_t i = 0 ; i <= f_mask ; ++i ) f_cell[ i ].~Cell();
 std::allocator< Cell >().deallocate( f_cell , capacity() );
 }

template< class T >
template< class ... Args >
bool MPMCQueue< T >::emplace( Args && ... args )
{
 std::size_t p = f_tail.load( std::memory_order_relaxed );
 for( ; ; ) {
  Cell & c = f_cell[ p & f_mask ];
  const std::size_t s = c.seq.load( std::memory_order_acquire );
  const auto d = static_cast< std::ptrdiff_t >( s - p );
  if( d == 0 ) {                            // free for p: claim it
   if( f_tail.compare_exchange_weak( p , p + 1 ,
                                     std::memory_order_relaxed ) ) {
    ::new( static_cast< void * >( c.raw ) )
     T( std::forward< Args >( args )... );
    c.seq.store( p + 1 , std::memory_order_release );
    return( true );
    }
   }                                        // p reloaded by the failure
  else if( d < 0 )                          // the item of p - C is there
   return( false );
  else
   p = f_tail.load( std::memory_order_relaxed );
  }
 }

template< class T >
bool MPMCQueue< T >::tryPop( T & x )
{
 std::size_t h = f_head.load( std::memory_order_relaxed );
 for( ; ; ) {
  Cell & c = f_cell[ h & f_mask ];
  const std::size_t s = c.seq.load( std::memory_order_acquire );
  const auto d = static_cast< std::ptrdiff_t >( s - ( h + 1 ) );
  if( d == 0 ) {                            // the item of h: claim it
   if( f_head.compare_exchange_weak( h , h + 1 ,
                                     std::memory_order_relaxed ) ) {
    T * v = c.item();
    x = std::move( *v );
    v->~T();
    c.seq.store( h + f_mask + 1 , std::memory_order_release );
    return( true );
    }
   }
  else if( d < 0 )                          // not yet pushed: empty
   return( false );
  else
   h = f_head.load( std::memory_order_relaxed );
  }
 }

template< class T >
std::size_t MPMCQueue< T >::size( void ) const
{
 const std::size_t h = f_head.load( std::memory_order_acquire ) ,
                   t = f_tail.load( std::memory_order_acquire );
 return( t > h ? std::min( t - h , capacity() ) : 0 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPMCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
