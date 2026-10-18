/*--------------------------------------------------------------------------*/
/*------------------------ File QueueAlgorithms.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The algorithms of QueueAlgorithms.h, with their contracts.
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

template< Queue Q >
void extractPositive( Q & q , Q & out )
{
 ADS_PRE( &q != &out );
 while( ! q.isEmpty() ) {
  auto x = q.dequeue();
  if( x > 0 )
   out.enqueue( x );
  }
 ADS_POST( q.isEmpty() );
 }

template< Queue Q >
void copyPositive( Q & q , Q & out )
{
 ADS_PRE( &q != &out );
 LinkedQueue< typename Q::Item > aux;
 std::vector< typename Q::Item > before;    // for the audit
 while( ! q.isEmpty() ) {
  auto x = q.dequeue();
  if constexpr( ADS_CONTRACTS >= 3 )
   before.push_back( x );
  if( x > 0 )
   out.enqueue( x );
  aux.enqueue( x );
  }
 while( ! aux.isEmpty() )
  q.enqueue( aux.dequeue() );
 ADS_AUDIT( [ & ]( void ) {                  // q holds before, in order
  for( const auto & x : before ) {
   auto y = q.dequeue();
   q.enqueue( y );
   if( !( x == y ) )
    return( false );
   }
  return( true );
  }() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File QueueAlgorithms.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
