/*--------------------------------------------------------------------------*/
/*--------------------------- File MPSCQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MPSCQueue [see MPSCQueue.h], with their contracts.
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

template< class T , std::size_t B >
MPSCQueue< T , B >::~MPSCQueue()
{
 for( Lane * l = f_lanes.load( std::memory_order_acquire ) ; l ; ) {
  std::size_t h = l->at;                    // the items left, the blocks
  for( Block * b = l->out ; b ; h = 0 ) {
   const std::size_t f = b->filled.load( std::memory_order_acquire );
   for( ; h < f ; ++h ) b->at( h )->~T();
   Block * n = b->next.load( std::memory_order_acquire );
   delete b;
   b = n;
   }
  for( Block * s : { l->spare , l->given.load( std::memory_order_acquire ) } )
   while( s ) {
    Block * n = s->spare;
    delete s;
    s = n;
    }
  Lane * n = l->nextLane;
  delete l;
  l = n;
  }
 }

template< class T , std::size_t B >
typename MPSCQueue< T , B >::Producer MPSCQueue< T , B >::producer( void )
{
 Lane * l = new Lane;
 l->nextLane = f_lanes.load( std::memory_order_relaxed );
 while( ! f_lanes.compare_exchange_weak( l->nextLane , l ,
                                         std::memory_order_release ,
                                         std::memory_order_relaxed ) ) {}
 return( Producer( l ) );
 }

template< class T , std::size_t B >
template< class ... Args >
void MPSCQueue< T , B >::Producer::emplace( Args && ... args )
{
 ADS_PRE( f_lane != nullptr );
 Lane * l = f_lane;
 Block * b = l->in;
 std::size_t p = b->filled.load( std::memory_order_relaxed );   // its own
 if( p == B ) [[unlikely]] {                // full: a block given back, or
  Block * n = l->spare;                     // a new one, linked after it
  if( ! n )
   n = l->given.exchange( nullptr , std::memory_order_acquire );
  if( n ) {
   l->spare = n->spare;
   n->filled.store( 0 , std::memory_order_relaxed );
   n->next.store( nullptr , std::memory_order_relaxed );
   }
  else
   n = new Block;
  b->next.store( n , std::memory_order_release );
  l->in = b = n;
  p = 0;
  }
 ::new( b->slot( p ) ) T( std::forward< Args >( args )... );
 b->filled.store( p + 1 , std::memory_order_release );
 }

template< class T , std::size_t B >
void MPSCQueue< T , B >::giveBack( Lane * l , Block * b )
{
 b->spare = l->given.load( std::memory_order_relaxed );
 while( ! l->given.compare_exchange_weak( b->spare , b ,
                                          std::memory_order_release ,
                                          std::memory_order_relaxed ) ) {}
 }

template< class T , std::size_t B >
void MPSCQueue< T , B >::relax( void )
{
 for( int i = 0 ; i < pauses ; ++i ) {
#if defined( __x86_64__ ) || defined( __i386__ )
  __builtin_ia32_pause();
#elif defined( __aarch64__ )
  asm volatile( "yield" );
#else
  std::atomic_signal_fence( std::memory_order_seq_cst );
#endif
  }
 }

template< class T , std::size_t B >
std::size_t MPSCQueue< T , B >::drain( Lane * l , T * out , std::size_t n )
{
 std::size_t k = 0 , h = l->at;
 Block * b = l->out;
 while( k < n ) {
  if( h == B ) {                            // all taken: the next block,
   Block * nb = b->next.load( std::memory_order_acquire );   // if linked
   if( ! nb )
    break;
   giveBack( l , b );
   b = nb;
   h = 0;
   }
  std::size_t f = b->filled.load( std::memory_order_acquire );
  if( ( f - h < want ) && ( f < B ) && l->streaming ) {
   // a few items, and the last call found some: the producer writes on,
   // and the consumer waits a moment for a batch rather than look again
   // at once, which would take from the producer the lines of the count
   // and of the items it is writing at each item
   relax();
   f = b->filled.load( std::memory_order_acquire );
   }
  const std::size_t e = std::min( f , h + ( n - k ) );
  if( h == e )
   break;
  for( ; h < e ; ++h , ++k ) {
   T * x = b->at( h );
   out[ k ] = std::move( *x );
   x->~T();
   }
  }
 l->out = b;
 l->at = h;
 l->streaming = k > 0;
 return( k );
 }

template< class T , std::size_t B >
std::size_t MPSCQueue< T , B >::popInto( T * out , std::size_t n )
{
 Lane * first = nullptr;                    // a turn from the lane at
 for( Lane * l = f_at ; ; ) {
  if( ! l ) l = f_lanes.load( std::memory_order_acquire );
  if( ! l || ( l == first ) )
   return( 0 );
  if( ! first ) first = l;
  if( const std::size_t k = drain( l , out , n ) ) {
   f_at = l;                                // the next call starts here
   return( k );
   }
  l = l->nextLane;
  if( ! l ) l = f_lanes.load( std::memory_order_acquire );
  }
 }

template< class T , std::size_t B >
bool MPSCQueue< T , B >::tryPop( T & x )
{
 return( popInto( &x , 1 ) == 1 );
 }

template< class T , std::size_t B >
bool MPSCQueue< T , B >::empty( void )
{
 for( Lane * l = f_lanes.load( std::memory_order_acquire ) ; l ;
      l = l->nextLane ) {
  std::size_t h = l->at;
  for( Block * b = l->out ; b ;
       b = b->next.load( std::memory_order_acquire ) , h = 0 )
   if( h < b->filled.load( std::memory_order_acquire ) )
    return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPSCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
