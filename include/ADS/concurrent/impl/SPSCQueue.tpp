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
 : f_A( capacity ) , f_mask( capacity - 1 )
{
 ADS_PRE( ( capacity >= 2 ) && std::has_single_bit( capacity ) );
 }

template< class T >
bool SPSCQueue< T >::push( const T & v )
{
 const std::size_t t = f_tail.load( std::memory_order_relaxed );
 if( t - f_headSeen == f_A.size() ) {      // full, as far as we know
  f_headSeen = f_head.load( std::memory_order_acquire );
  if( t - f_headSeen == f_A.size() )
   return( false );
  }
 f_A[ t & f_mask ] = v;
 f_tail.store( t + 1 , std::memory_order_release );
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
 std::optional< T > v( std::move( f_A[ h & f_mask ] ) );
 f_head.store( h + 1 , std::memory_order_release );
 return( v );
 }

template< class T >
std::size_t SPSCQueue< T >::size( void ) const
{
 return( f_tail.load( std::memory_order_acquire ) -
         f_head.load( std::memory_order_acquire ) );
 }

template< class T >
std::size_t SPSCQueue< T >::capacity( void ) const
{
 return( f_A.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SPSCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
