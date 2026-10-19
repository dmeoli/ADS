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
  // the items from the consumer's place to the producer's
  for( Block * b = l->head ; b ; ) {
   const std::size_t from = b == l->head ? l->headAt : 0 ,
                     to = b->written.load( std::memory_order_acquire );
   for( std::size_t i = from ; i < to ; ++i ) b->at( i )->~T();
   Block * n = b->next.load( std::memory_order_acquire );
   delete b;
   b = n;
   }
  delete l->spare.load( std::memory_order_acquire );
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
 std::size_t i = l->tailAt;
 Block * b = l->tail;
 if( i == B ) [[unlikely]] {                // full: the next block, the
  Block * n = l->spare.exchange( nullptr , std::memory_order_acquire );
  if( n ) {                                 // one given back if any
   n->written.store( 0 , std::memory_order_relaxed );
   n->next.store( nullptr , std::memory_order_relaxed );
   }
  else
   n = new Block;
  b->next.store( n , std::memory_order_release );
  l->tail = b = n;
  i = 0;
  }
 ::new( static_cast< void * >( b->at( i ) ) )
  T( std::forward< Args >( args )... );
 b->written.store( i + 1 , std::memory_order_release );
 l->tailAt = i + 1;
 }

template< class T , std::size_t B >
std::size_t MPSCQueue< T , B >::drain( Lane * l , T * out , std::size_t n )
{
 std::size_t k = 0;
 while( k < n ) {
  Block * b = l->head;
  if( l->headAt == l->seen ) {              // all those seen taken: look
   l->seen = b->written.load( std::memory_order_acquire );   // again
   if( l->headAt == l->seen ) {
    if( l->headAt < B )
     break;                                 // none in the lane
    Block * nb = b->next.load( std::memory_order_acquire );
    if( ! nb )
     break;                                 // the next block not linked
    l->head = nb;                           // the block done, given back
    l->headAt = l->seen = 0;
    delete l->spare.exchange( b , std::memory_order_acq_rel );
    continue;
    }
   }
  for( ; ( l->headAt < l->seen ) && ( k < n ) ; ++l->headAt , ++k ) {
   T * p = b->at( l->headAt );
   out[ k ] = std::move( *p );
   p->~T();
   }
  }
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
  if( l->headAt < l->head->written.load( std::memory_order_acquire ) )
   return( false );
  if( ( l->headAt == B ) &&
      l->head->next.load( std::memory_order_acquire ) &&
      ( l->head->next.load( std::memory_order_acquire )->written.load(
         std::memory_order_acquire ) > 0 ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPSCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
