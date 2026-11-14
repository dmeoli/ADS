/*--------------------------------------------------------------------------*/
/*--------------------------- File SPSCQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SPSCQueue [see SPSCQueue.h], with their contracts.
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
void SPSCQueue< T >::relax( void )
{
 for( int i = 0 ; i < 64 ; ++i ) {
#if defined( __x86_64__ ) || defined( __i386__ )
  __builtin_ia32_pause();
#elif defined( __aarch64__ )
  asm volatile( "yield" );
#endif
  }
 }

template< class T >
void SPSCQueue< T >::refresh( std::size_t h )
{
 std::size_t t = f_tail.load( std::memory_order_acquire );
 if( f_streaming && ( t != h ) && ( t - h < batch() ) ) {
  relax();                                  // a moment for a batch
  t = f_tail.load( std::memory_order_acquire );
  }
 f_tailSeen = t;
 }

template< class T >
bool SPSCQueue< T >::tryPop( T & x )
{
 const std::size_t h = f_head.load( std::memory_order_relaxed );
 if( h == f_tailSeen ) {                    // empty, as far as we know:
  prefetch( at( h ) );                      // the place on its way with
  refresh( h );                             // the tail
  if( h == f_tailSeen )
   return( f_streaming = false );
  }
 T * p = at( h );
 x = std::move( *p );
 p->~T();
 f_head.store( h + 1 , std::memory_order_release );
 return( f_streaming = true );
 }

template< class T >
std::optional< T > SPSCQueue< T >::pop( void )
{
 const std::size_t h = f_head.load( std::memory_order_relaxed );
 if( h == f_tailSeen ) {                    // empty, as far as we know:
  prefetch( at( h ) );                      // the place on its way with
  refresh( h );                             // the tail
  if( h == f_tailSeen ) {
   f_streaming = false;
   return( std::nullopt );
   }
  }
 T * p = at( h );
 std::optional< T > v( std::move( *p ) );
 p->~T();
 f_head.store( h + 1 , std::memory_order_release );
 f_streaming = true;
 return( v );
 }

template< class T >
template< class It >
std::size_t SPSCQueue< T >::pushMany( It first , std::size_t n )
{
 const std::size_t t = f_tail.load( std::memory_order_relaxed );
 if( capacity() - ( t - f_headSeen ) < n )  // too little room, as far as
  f_headSeen = f_head.load( std::memory_order_acquire );   // we know
 const std::size_t k = std::min( n , capacity() - ( t - f_headSeen ) );
 for( std::size_t j = 0 ; j < k ; ++j , ++first )
  ::new( static_cast< void * >( at( t + j ) ) ) T( *first );
 if( k > 0 )
  f_tail.store( t + k , std::memory_order_release );
 return( k );
 }

template< class T >
template< class F >
std::size_t SPSCQueue< T >::consume( F && f , std::size_t n )
{
 const std::size_t h = f_head.load( std::memory_order_relaxed );
 if( h == f_tailSeen )
  f_tailSeen = f_tail.load( std::memory_order_acquire );
 const std::size_t k = std::min( n , f_tailSeen - h );
 f_streaming = false;
 // the items called are dequeued also when a call throws
 std::size_t j = 0;
 struct Done {
  SPSCQueue & q;
  std::size_t h;
  std::size_t & j;
  ~Done() {
   if( j > 0 )
    q.f_head.store( h + j , std::memory_order_release );
   }
  } done{ *this , h , j };
 while( j < k ) {
  T * p = at( h + j );
  ++j;
  struct Kill {
   T * p;
   ~Kill() { p->~T(); }
   } kill{ p };
  f( std::move( *p ) );
  }
 return( k );
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
