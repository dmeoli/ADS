/*--------------------------------------------------------------------------*/
/*--------------------------- File MPSCInbox.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MPSCInbox [see MPSCInbox.h], with their contracts.
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
MPSCInbox< T >::~MPSCInbox()
{
 for( Cell * c = f_top.load( std::memory_order_acquire ) ; c ; ) {
  Cell * n = c->next;
  delete c;
  c = n;
  }
 }

template< class T >
void MPSCInbox< T >::push( T v )
{
 Cell * c = new Cell{ std::move( v ) ,
                      f_top.load( std::memory_order_relaxed ) };
 while( ! f_top.compare_exchange_weak( c->next , c ,
                                       std::memory_order_release ,
                                       std::memory_order_relaxed ) ) {}
 }

template< class T >
std::vector< T > MPSCInbox< T >::take( void )
{
 Cell * c = f_top.exchange( nullptr , std::memory_order_acquire );
 std::vector< T > out;
 for( ; c ; ) {                             // the stack, newest first
  Cell * n = c->next;
  out.push_back( std::move( c->value ) );
  delete c;
  c = n;
  }
 for( std::size_t i = 0 , j = out.size() ; i + 1 < j ; ++i , --j )
  std::swap( out[ i ] , out[ j - 1 ] );    // the order of sending
 return( out );
 }

template< class T >
bool MPSCInbox< T >::empty( void ) const
{
 return( f_top.load( std::memory_order_acquire ) == nullptr );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPSCInbox.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
