/*--------------------------------------------------------------------------*/
/*-------------------------- File LockFreeStack.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LockFreeStack [see LockFreeStack.h], with their
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
LockFreeStack< T , R >::~LockFreeStack()
{
 for( Node * p = f_top.load( std::memory_order_acquire ) ; p ; ) {
  Node * q = p->next;
  delete p;
  p = q;
  }
 }

template< class T , reclamation_policy R >
std::size_t LockFreeStack< T , R >::size( void ) const
{
 std::size_t n = 0;
 for( Node * p = f_top.load( std::memory_order_acquire ) ; p ; p = p->next )
  ++n;
 return( n );
 }

template< class T , reclamation_policy R >
void LockFreeStack< T , R >::Handle::push( T x )
{
 Node * n = new Node{ { nullptr , &LockFreeStack::reclaimNode } ,
		      std::move( x ) ,
		      nullptr };
 n->next = f_s->f_top.load( std::memory_order_relaxed );
 while( ! f_s->f_top.compare_exchange_weak(
   n->next , n , std::memory_order_seq_cst , std::memory_order_relaxed ) ) {
  }
 }

template< class T , reclamation_policy R >
auto LockFreeStack< T , R >::Handle::tryPop( void ) -> std::optional< T >
{
 typename Local::Guard g( f_l );
 for( ; ; ) {
  Node * t = g.protect( 0 , f_s->f_top );
  if( ! t )
   return( std::nullopt );  // empty, linearized at this read
  // t is protected, and its successor fixed since its push: a top still t
  // is the same node with the same successor, since t is not freed, and
  // therefore not pushed again, while protected
  Node * e = t;
  if( f_s->f_top.compare_exchange_weak( e , t->next ,
					std::memory_order_seq_cst ,
					std::memory_order_relaxed ) ) {
   std::optional< T > x( std::move( t->item ) );
   f_l.retire( t );
   return( x );
   }
  }
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File LockFreeStack.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
