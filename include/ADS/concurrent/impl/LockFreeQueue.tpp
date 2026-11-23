/*--------------------------------------------------------------------------*/
/*-------------------------- File LockFreeQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LockFreeQueue [see LockFreeQueue.h], with their
 * contracts.
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

template< class T , reclamation_policy R >
LockFreeQueue< T , R >::LockFreeQueue( void )
{
 Node * d = makeNode();
 f_head.store( d , std::memory_order_relaxed );
 f_tail.store( d , std::memory_order_relaxed );
 }

template< class T , reclamation_policy R >
LockFreeQueue< T , R >::~LockFreeQueue()
{
 // the dummy has no item, the nodes after it have theirs
 Node * p = f_head.load( std::memory_order_acquire );
 Node * q = p->next.load( std::memory_order_acquire );
 delete p;
 for( p = q ; p ; p = q ) {
  q = p->next.load( std::memory_order_acquire );
  p->item()->~T();
  delete p;
  }
 }

template< class T , reclamation_policy R >
std::size_t LockFreeQueue< T , R >::size( void ) const
{
 std::size_t n = 0;
 for( Node * p = f_head.load( std::memory_order_acquire )
		   ->next.load( std::memory_order_acquire ) ;
      p ; p = p->next.load( std::memory_order_acquire ) )
  ++n;
 return( n );
 }

template< class T , reclamation_policy R >
void LockFreeQueue< T , R >::Handle::enqueue( T x )
{
 Node * n = makeNode();
 ::new( static_cast< void * >( n->raw ) ) T( std::move( x ) );
 typename Local::Guard g( f_l );
 for( ; ; ) {
  // the node of the tail is retired only after the head has passed it,
  // and the head passes it only after the tail has
  Node * t = g.protect( 0 , f_q->f_tail );
  Node * s = t->next.load( std::memory_order_seq_cst );
  if( s ) {
   f_q->f_tail.compare_exchange_strong( t , s );  // help a stopped enqueue
   continue;
   }
  Node * e = nullptr;
  if( t->next.compare_exchange_strong( e , n ) ) {  // the linearization
   f_q->f_tail.compare_exchange_strong( t , n );
   return;
   }
  }
 }

template< class T , reclamation_policy R >
auto LockFreeQueue< T , R >::Handle::tryDequeue( void ) -> std::optional< T >
{
 typename Local::Guard g( f_l );
 for( ; ; ) {
  Node * h = g.protect( 0 , f_q->f_head );
  Node * n = g.protect( 1 , h->next );
  // with the head still h after n was protected, n had not been retired,
  // since only the dequeue that moves the head past n retires it
  if( f_q->f_head.load( std::memory_order_seq_cst ) != h )
   continue;
  if( ! n )
   return( std::nullopt );  // empty, linearized at the read of n
  Node * t = f_q->f_tail.load( std::memory_order_seq_cst );
  if( t == h ) {
   // the tail behind the last node: swung first, so that the head never
   // passes the tail and the node of the tail is never retired
   f_q->f_tail.compare_exchange_strong( t , n );
   continue;
   }
  Node * e = h;
  if( f_q->f_head.compare_exchange_strong( e , n ) ) {  // the linearization
   // n is the dummy now, and its item is this dequeue's alone
   std::optional< T > x( std::move( *n->item() ) );
   n->item()->~T();
   f_l.retire( h );
   return( x );
   }
  }
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File LockFreeQueue.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
