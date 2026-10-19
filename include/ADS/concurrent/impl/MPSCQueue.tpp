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
  for( Ring * r = l->out ; r ; ) {          // the items left, the vectors
   const std::size_t t = r->tail.load( std::memory_order_acquire );
   for( std::size_t h = r->head.load() ; h != t ; ++h )
    r->place[ h & r->mask ].~T();
   Ring * n = r->next.load( std::memory_order_acquire );
   delete r;
   r = n;
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
 Ring * r = f_lane->in;
 std::size_t t = r->tail.load( std::memory_order_relaxed );
 if( t - r->headSeen > r->mask ) [[unlikely]] {   // full, as far as seen
  r->headSeen = r->head.load( std::memory_order_acquire );
  if( t - r->headSeen > r->mask ) {         // full: a vector twice as
   Ring * n = new Ring( 2 * ( r->mask + 1 ) );   // large, linked after
   r->next.store( n , std::memory_order_release );   // the last item
   f_lane->in = r = n;
   t = 0;
   }
  }
 ::new( static_cast< void * >( r->place + ( t & r->mask ) ) )
  T( std::forward< Args >( args )... );
 r->tail.store( t + 1 , std::memory_order_release );
 }

template< class T , std::size_t B >
std::size_t MPSCQueue< T , B >::drain( Lane * l , T * out , std::size_t n )
{
 std::size_t k = 0;
 while( k < n ) {
  Ring * r = l->out;
  std::size_t h = r->head.load( std::memory_order_relaxed );
  if( h == r->tailSeen ) {                  // all those seen taken
   r->tailSeen = r->tail.load( std::memory_order_acquire );
   if( h == r->tailSeen ) {
    // empty: if the producer went on in a larger vector, the tail it
    // left here came before the link, and this one is done
    Ring * nr = r->next.load( std::memory_order_acquire );
    if( ! nr )
     break;
    r->tailSeen = r->tail.load( std::memory_order_acquire );
    if( h != r->tailSeen )
     continue;
    l->out = nr;
    delete r;
    continue;
    }
   }
  const std::size_t e = std::min( r->tailSeen , h + ( n - k ) );
  for( ; h < e ; ++h , ++k ) {
   T & x = r->place[ h & r->mask ];
   out[ k ] = std::move( x );
   x.~T();
   }
  r->head.store( h , std::memory_order_release );
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
      l = l->nextLane )
  for( Ring * r = l->out ; r ; r = r->next.load( std::memory_order_acquire ) )
   if( r->head.load( std::memory_order_relaxed ) !=
       r->tail.load( std::memory_order_acquire ) )
    return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPSCQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
